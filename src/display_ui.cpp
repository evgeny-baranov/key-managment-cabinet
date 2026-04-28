#include "display_ui.h"

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "../lib/cabinet_core/src/constants.h"

namespace {
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
}

void initDisplay() {
  Wire.begin(SCREEN_SDA, SCREEN_SCK);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_I2C_ADDRESS)) {
    Serial.println("OLED failed");
    while (true) {
      delay(10);
    }
  }
}

void showMessage(const String& line1, const String& line2) {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setCursor(0, 0);

  display.setTextSize(2);
  display.println(line1);

  if (line2 != "") {
    display.setTextSize(1);
    display.println();
    display.println(line2);
  }

  display.display();
}

void showIdle() {
  showMessage("Ready", "Scan card");
}

void showScanning(const String& uid) {
  showMessage("Scanning", uid);
}

void showError(const String& detail) {
  showMessage("ERROR", detail);
}
