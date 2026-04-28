#include <Arduino.h>
#include <WiFi.h>

#include "config.h"
#include "constants.h"
#include "display_ui.h"
#include "fsm.h"
#include "http_client.h"
#include "led_ui.h"
#include "lock_control.h"
#include "protocol.h"
#include "rfid_reader.h"

namespace {
CabinetContext cabinet;
String currentPresentedUid = "";
unsigned long lastRawReadTime = 0;

bool isAwaitingState(CabinetState state) {
  return state == CabinetState::AwaitingKeyForTake || state == CabinetState::AwaitingUserForReturn;
}

void goToIdle() {
  allLedsOff();
  showIdle();
}

void showResultThenIdle(bool ok, const String& line1, const String& line2 = "") {
  showMessage(line1, line2);
  if (ok) {
    blinkGreenCustom(3, LED_RESULT_BLINK_ON_MS, LED_RESULT_BLINK_OFF_MS);
  } else {
    blinkRedCustom(3, LED_RESULT_BLINK_ON_MS, LED_RESULT_BLINK_OFF_MS);
  }
  delay(RESULT_VISIBLE_MS);
  goToIdle();
}

CabinetEventType mapProcessEvent(const ProcessResult& processResult, const String& expectedAction) {
  if (processResult.ok) {
    if (expectedAction == ACTION_TAKE) {
      return CabinetEventType::TakeCompleted;
    }
    return CabinetEventType::ReturnCompleted;
  }

  if (
    processResult.code == ServerCode::KeyNotAvailable ||
    processResult.code == ServerCode::KeyAlreadyIn ||
    processResult.code == ServerCode::UserNotFound ||
    processResult.code == ServerCode::UserInactive ||
    processResult.code == ServerCode::KeyNotFound
  ) {
    return CabinetEventType::ProcessDenied;
  }

  return CabinetEventType::ProcessFailed;
}

void runEffect(const TransitionResult& result, const ClassifiedUid* classified, const ProcessResult* processResult);

void runProcessRequest(const TransitionResult& result) {
  if (result.action == ACTION_TAKE) {
    Serial.println("Completing TAKE...");
    showMessage("Scanning", "Checking key...");
  } else {
    Serial.println("Completing RETURN...");
    showMessage("Scanning", "Checking user...");
  }

  String rawResult = processActionHttp(result.action, result.userUid, result.keyUid);
  ProcessResult process = parseProcessResponse(rawResult);

  if (process.ok) {
    Serial.print(result.action);
    Serial.println(" logged successfully");
    Serial.println(rawResult);
  } else if (mapProcessEvent(process, result.action) == CabinetEventType::ProcessDenied) {
    Serial.print(result.action);
    Serial.println(" denied");
    Serial.println(rawResult);
  } else {
    Serial.print(result.action);
    Serial.println(" failed");
    Serial.println(rawResult);
  }

  CabinetEventType followUpEvent = mapProcessEvent(process, result.action);
  TransitionResult followUp = transition(cabinet, followUpEvent, nullptr, &process, millis());
  runEffect(followUp, nullptr, &process);
}

void runEffect(const TransitionResult& result, const ClassifiedUid* classified, const ProcessResult* processResult) {
  switch (result.effect) {
    case CabinetEffect::None:
      return;

    case CabinetEffect::ShowIdle:
      goToIdle();
      return;

    case CabinetEffect::ShowUserAccepted:
      Serial.println("User scanned first -> pending TAKE");
      if (classified != nullptr) {
        Serial.print("User: ");
        Serial.println(classified->name);
      }
      blinkGreen(1);
      showMessage("OK", "User -> scan key");
      return;

    case CabinetEffect::ShowKeyAccepted:
      Serial.println("Key scanned first -> pending RETURN");
      if (classified != nullptr) {
        Serial.print("Key: ");
        Serial.println(classified->name);
      }
      blinkGreen(1);
      showMessage("OK", "Key -> scan user");
      return;

    case CabinetEffect::ShowWrongOrder:
      Serial.println("Unexpected scan order");
      showResultThenIdle(false, "ERROR", "Wrong order");
      return;

    case CabinetEffect::ShowDenied:
      if (processResult != nullptr) {
        showResultThenIdle(false, "ERROR", "Try again");
        return;
      }

      if (classified != nullptr && classified->type == ScanType::Key && classified->keyStatus != KEY_STATUS_OUT) {
        Serial.println("Key already in cabinet, cannot start RETURN");
        showResultThenIdle(false, "ERROR", "Key in cabinet");
        return;
      }

      Serial.println("Classify failed / access denied");
      showResultThenIdle(false, "ERROR", "Unknown card");
      return;

    case CabinetEffect::ShowServerFail:
      showResultThenIdle(false, "ERROR", "Server fail");
      return;

    case CabinetEffect::ShowTimeout:
      Serial.println("Pending action timed out");
      showResultThenIdle(false, "ERROR", "Timeout");
      return;

    case CabinetEffect::StartTakeRequest:
    case CabinetEffect::StartReturnRequest:
      runProcessRequest(result);
      return;

    case CabinetEffect::UnlockAndShowSuccess:
      showMessage("OK", "Unlocking...");
      blinkGreenCustom(3, LED_RESULT_BLINK_ON_MS, LED_RESULT_BLINK_OFF_MS);
      unlockPulse(UNLOCK_PULSE_MS);
      goToIdle();
      return;
  }
}

void checkTimeout() {
  if (!isAwaitingState(cabinet.state) || cabinet.pendingSince == 0) {
    return;
  }

  if (millis() - cabinet.pendingSince > PENDING_TIMEOUT_MS) {
    TransitionResult timeoutResult = transition(cabinet, CabinetEventType::Timeout, nullptr, nullptr, millis());
    runEffect(timeoutResult, nullptr, nullptr);
  }
}

void processClassified(const ClassifiedUid& classified) {
  CabinetEventType event = CabinetEventType::DeniedScan;

  if (classified.type == ScanType::User) {
    event = CabinetEventType::UserScanned;
  } else if (classified.type == ScanType::Key) {
    event = CabinetEventType::KeyScanned;
  } else {
    event = CabinetEventType::DeniedScan;
  }

  TransitionResult eventResult = transition(cabinet, event, &classified, nullptr, millis());
  runEffect(eventResult, &classified, nullptr);
}
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(300);

  initLeds();
  initLock();
  initDisplay();

  showMessage("Booting", "WiFi...");

  initRfid();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.println(WiFi.localIP());

  cabinet = CabinetContext{};
  Serial.println("RDM6300 ready");
  Serial.println("Scan user first for TAKE, or key first for RETURN");

  goToIdle();
}

void loop() {
  checkTimeout();

  String rawUid = readRdm6300UidRaw();

  if (rawUid != "") {
    lastRawReadTime = millis();

    if (rawUid == currentPresentedUid) {
      return;
    }

    currentPresentedUid = rawUid;

    Serial.print("Scanned UID: ");
    Serial.println(rawUid);

    scanPulse();
    showScanning(rawUid);

    String classifyRaw = classifyUidHttp(rawUid);
    ClassifiedUid classified = parseClassifyResponse(classifyRaw);

    if (classified.type == ScanType::Unknown) {
      Serial.println("Unexpected server response");
      Serial.println(classifyRaw);
      showResultThenIdle(false, "ERROR", "Bad reply");
      return;
    }

    processClassified(classified);
    return;
  }

  if (currentPresentedUid != "" && millis() - lastRawReadTime > TAG_RELEASE_MS) {
    currentPresentedUid = "";
  }
}
