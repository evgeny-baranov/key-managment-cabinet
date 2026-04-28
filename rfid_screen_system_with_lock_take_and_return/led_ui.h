#pragma once

void initLeds();
void allLedsOff();
void scanPulse();
void blinkGreen(int count);
void blinkRed(int count);
void blinkGreenCustom(int count, int onMs, int offMs);
void blinkRedCustom(int count, int onMs, int offMs);
