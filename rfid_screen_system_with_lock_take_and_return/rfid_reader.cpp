#include "rfid_reader.h"

#include <HardwareSerial.h>

#include "constants.h"

namespace {
HardwareSerial RFID(RFID_UART_PORT);
}

void initRfid() {
  RFID.begin(RFID_BAUD, SERIAL_8N1, RFID_RX_PIN, -1);
}

String readRdm6300UidRaw() {
  static String buffer = "";

  while (RFID.available()) {
    char c = RFID.read();

    if (c == 0x02) {
      buffer = "";
    } else if (c == 0x03) {
      String frame = buffer;
      buffer = "";
      frame.trim();

      if (frame.length() >= RFID_UID_LENGTH) {
        String uid = frame.substring(0, RFID_UID_LENGTH);
        uid.toUpperCase();
        return uid;
      }
    } else {
      if (isPrintable(c)) {
        buffer += c;
      }
    }
  }

  return "";
}
