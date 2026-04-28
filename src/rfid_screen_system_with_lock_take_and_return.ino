#include <Arduino.h>
#include <WiFi.h>

#include "config.h"
#include "../lib/cabinet_core/src/cabinet_workflow.h"
#include "../lib/cabinet_core/src/constants.h"
#include "display_ui.h"
#include "../lib/cabinet_core/src/fsm.h"
#include "http_client.h"
#include "led_ui.h"
#include "lock_control.h"
#include "../lib/cabinet_core/src/protocol.h"
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
  CabinetEventType followUpEvent = decideProcessFollowUpEvent(process, result.action);

  if (process.ok && followUpEvent != CabinetEventType::ProcessFailed) {
    Serial.print(result.action);
    Serial.println(" logged successfully");
    Serial.println(rawResult);
  } else if (followUpEvent == CabinetEventType::ProcessDenied) {
    Serial.print(result.action);
    Serial.println(" denied");
    Serial.println(rawResult);
  } else {
    Serial.print(result.action);
    Serial.println(" failed");
    if (process.ok) {
      Serial.print("Unexpected action in response: ");
      Serial.println(process.action);
    }
    Serial.println(rawResult);
  }

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
  ClassifiedScanDecision decision = decideClassifiedScan(classified);

  if (decision.useImmediateEffect) {
    TransitionResult immediateResult;
    immediateResult.effect = decision.immediateEffect;
    runEffect(immediateResult, &classified, nullptr);
    return;
  }

  TransitionResult eventResult = transition(cabinet, decision.event, &classified, nullptr, millis());
  runEffect(eventResult, &classified, nullptr);
}
}

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
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
