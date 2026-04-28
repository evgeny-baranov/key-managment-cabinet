#include "led_ui.h"

#include <Arduino.h>

#include "constants.h"

void initLeds() {
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  allLedsOff();
}

void allLedsOff() {
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);
}

void blinkGreenCustom(int count, int onMs, int offMs) {
  for (int i = 0; i < count; i++) {
    allLedsOff();
    digitalWrite(GREEN_LED_PIN, HIGH);
    delay(onMs);
    digitalWrite(GREEN_LED_PIN, LOW);
    delay(offMs);
  }
}

void blinkRedCustom(int count, int onMs, int offMs) {
  for (int i = 0; i < count; i++) {
    allLedsOff();
    digitalWrite(RED_LED_PIN, HIGH);
    delay(onMs);
    digitalWrite(RED_LED_PIN, LOW);
    delay(offMs);
  }
}

void blinkGreen(int count) {
  blinkGreenCustom(count, LED_BLINK_ON_MS, LED_BLINK_OFF_MS);
}

void blinkRed(int count) {
  blinkRedCustom(count, LED_BLINK_ON_MS, LED_BLINK_OFF_MS);
}

void scanPulse() {
  allLedsOff();
  digitalWrite(GREEN_LED_PIN, HIGH);
  digitalWrite(RED_LED_PIN, HIGH);
  delay(LED_SCAN_PULSE_MS);
  allLedsOff();
  delay(LED_SCAN_PULSE_OFF_MS);
}
