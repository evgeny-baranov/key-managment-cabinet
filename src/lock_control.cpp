#include "lock_control.h"

#include <Arduino.h>

#include "../lib/cabinet_core/src/constants.h"

void initLock() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, RELAY_OFF);
}

void unlockPulse(unsigned long ms) {
  Serial.println("Unlock pulse");
  digitalWrite(RELAY_PIN, RELAY_ON);
  delay(ms);
  digitalWrite(RELAY_PIN, RELAY_OFF);
  Serial.println("Locked again");
}
