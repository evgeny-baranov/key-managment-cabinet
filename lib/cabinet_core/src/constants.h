#pragma once

#include <Arduino.h>

// Screen
constexpr int SCREEN_WIDTH = 128;
constexpr int SCREEN_HEIGHT = 64;
constexpr int SCREEN_SDA = 18;
constexpr int SCREEN_SCK = 19;
constexpr int SCREEN_I2C_ADDRESS = 0x3C;

// RFID
constexpr int RFID_UART_PORT = 2;
constexpr int RFID_BAUD = 9600;
constexpr int RFID_RX_PIN = 25;
constexpr int RFID_UID_LENGTH = 10;

// LED
constexpr int GREEN_LED_PIN = 23;
constexpr int RED_LED_PIN = 32;
constexpr int LED_SCAN_PULSE_MS = 35;
constexpr int LED_SCAN_PULSE_OFF_MS = 20;
constexpr int LED_BLINK_ON_MS = 70;
constexpr int LED_BLINK_OFF_MS = 50;
constexpr int LED_RESULT_BLINK_ON_MS = 90;
constexpr int LED_RESULT_BLINK_OFF_MS = 60;

// Lock
constexpr int RELAY_PIN = 5;
constexpr int RELAY_ON = HIGH;
constexpr int RELAY_OFF = LOW;
constexpr unsigned long UNLOCK_PULSE_MS = 3000;

// Timing
constexpr unsigned long TAG_RELEASE_MS = 400;
constexpr unsigned long PENDING_TIMEOUT_MS = 10000;
constexpr unsigned long HTTP_TIMEOUT_MS = 3000;
constexpr unsigned long RESULT_VISIBLE_MS = 700;

// Serial
constexpr int SERIAL_BAUD = 115200;

// Protocol strings
constexpr const char* MODE_CLASSIFY = "classify";
constexpr const char* MODE_PROCESS = "process";

constexpr const char* ACTION_TAKE = "TAKE";
constexpr const char* ACTION_RETURN = "RETURN";

constexpr const char* KEY_STATUS_IN = "IN";
constexpr const char* KEY_STATUS_OUT = "OUT";

constexpr const char* RESP_NOT_FOUND = "NOT_FOUND";
constexpr const char* RESP_USER_INACTIVE = "USER_INACTIVE";
constexpr const char* RESP_DUPLICATE_UID = "DUPLICATE_UID";
constexpr const char* RESP_ERROR = "ERROR";
constexpr const char* RESP_KEY_NOT_AVAILABLE = "KEY_NOT_AVAILABLE";
constexpr const char* RESP_KEY_ALREADY_IN = "KEY_ALREADY_IN";
constexpr const char* RESP_USER_NOT_FOUND = "USER_NOT_FOUND";
constexpr const char* RESP_KEY_NOT_FOUND = "KEY_NOT_FOUND";
constexpr const char* RESP_MISSING_PARAMS = "MISSING_PARAMS";
constexpr const char* RESP_INVALID_ACTION = "INVALID_ACTION";
constexpr const char* RESP_INVALID_MODE = "INVALID_MODE";

constexpr const char* PREFIX_USER_FOUND = "USER_FOUND|";
constexpr const char* PREFIX_KEY_FOUND = "KEY_FOUND|";
constexpr const char* PREFIX_OK_TAKE = "OK|TAKE|";
constexpr const char* PREFIX_OK_RETURN = "OK|RETURN|";
