#pragma once

#include <Arduino.h>

void initDisplay();
void showMessage(const String& line1, const String& line2 = "");
void showIdle();
void showScanning(const String& uid);
void showError(const String& detail);
