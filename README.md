# Key managment cabinet

---

## Idea description

The idea of this project is to develop a **smart key management system for a school that automates the process of issuing and returning physical keys**. The system is based on a closed key cabinet equipped with electronic access control.

Each key is attached to an NFC tag with a unique identifier, and users authenticate themselves using their school ID card. Only authorized users are allowed to unlock the cabinet and take a specific key. The system operates without the need for a responsible person to manually distribute keys, which reduces human error and digitizes the data for easy acccess. All actions, such as successful authentication, denied access, key removal, and key return, are logged automatically in a database. The system provides visual feedback using LEDs to indicate whether access is granted or denied. The solution is designed to be scalable, allowing it to be adapted for different cabinet sizes, user groups, or institutions.

---

## Problem analysis
### Target audience/users:
- Primary users of the system are school staff, such as teachers, administrators, and technical personnel, as well as students who are authorized to access specific rooms. The system is especially useful in environments where many keys are shared among different users throughout the day.

### Why it is worth developing this solution?
In many schools, key management is still handled manually, often relying on a logbook or a single responsible person. This approach is inefficient and prone to errors, as keys can be lost, borrowed without permission, or returned without proper tracking. In some cases, it is impossible to determine who last used a key, which can lead to security risks.

- Developing an automated key management system improves security, transparency, and efficiency. It ensures that only authorized users can access keys and provides a clear record of all key-related activities.
- The system reduces administrative workload and can operate continuously without supervision, making it a valuable and practical solution for modern educational institutions.

---

## Technologies used
### Delivery format:
- Physical system with a closed cabinet
- Web-based data storage using Google Sheets

### Hardware technologies
- ESP32 microcontroller
- RDM6300 125KHz RFID reader
- School ID Card
- Electronic cabinet lock with feedback signal, 12V
- MOSFET driver module for lock control
- RFID tags for keys
- Status LEDs (green + red)
- 0.96 Inch OLED Display

### Software technologies
- Arduino IDE
- Google Apps Script
- HTTP communication over Wi-Fi

--- 

## Work plan

### Deliverables:
- [x] **Week 1** &rarr; finalised written concept + system architecture

- [ ] **Week 2** &rarr;  printable cabinet piece models

- [ ] **Week 3** &rarr;  printed cabinet + ESC32 development environment

- [x] **Week 4** &rarr;  serial shows correct UID

- [x] **Week 5** &rarr;  visual feedback for system state

- [x] **Week 6** &rarr; cabinet can lock/unlock

- [x] **Week 7** &rarr;  combined system parts (taps card - cabinet opens - relocks automanically) 

- [ ] **Week 8** &rarr;  assembled cabinet prototype

- [x] **Week 9** &rarr;  live event log visible in browser

- [ ] **Week 10** &rarr;  demo ready and finished documentation


| Week | Tag (S-school, H-home)     | Todo task                                                                 | Status |
|-----|-----------|--------------------------------------------------------------------|----------------|
| 1   | S    | System concept + requirements              | (ﾉ◕ヮ◕)ﾉ*:･ﾟ✧    |
| 1   | H    | Draw full system diagram            | (ﾉ◕ヮ◕)ﾉ*:･ﾟ✧    |
| 2   | S  | Design cabinet layout     | ✍(◔◡◔)      |
| 2   | H  | Create 3D model of cabinet           | ┗( T﹏T )┛     |
| 3   | S  | Print the parts for the cabinet          | (_　_)。zＺ     |
| 3   | S  | Install ESP32 drivers and Arduino IDE ESP32 support (test board power)      | (ﾉ◕ヮ◕)ﾉ*:･ﾟ✧  |
| 4   | S  | Wire PN532 and ESP32 and verify connection                      | (ﾉ◕ヮ◕)ﾉ*:･ﾟ✧     |
| 4   | H  | Run PN532 examples, read UID from school card + test tag           | (ﾉ◕ヮ◕)ﾉ*:･ﾟ✧     |
| 5   | S  | Wire red/green LEDs to ESP32 GPIOs                            | (ﾉ◕ヮ◕)ﾉ*:･ﾟ✧     |
| 5   | H  | Create LED states  | (ﾉ◕ヮ◕)ﾉ*:･ﾟ✧     |
| 6   | S  | Wire lock to 12V PSU + flyback diode                   | (ﾉ◕ヮ◕)ﾉ*:･ﾟ✧     |
| 6   | H  | Test lock control via GPIO        | (ﾉ◕ヮ◕)ﾉ*:･ﾟ✧     |
| 7   | S  | Implement access flow  (card - unlock - delay - relock)                        | (ﾉ◕ヮ◕)ﾉ*:･ﾟ✧     |
| 7   | H  | Combine NFC + LEDs + lock into one sketch                                | (ﾉ◕ヮ◕)ﾉ*:･ﾟ✧     |
| 8   | S  | Mount electronics inside printed cabinet                            | (_　_)。zＺ     |
| 8   | H  | Add door/lock feedback signal and read it in code               | (ﾉ◕ヮ◕)ﾉ*:･ﾟ✧     |
| 9   | S   | Define event structure      | ✍(◔◡◔)     |
| 9  | H  | Create Google Sheets + Apps Script endpoint            | (ﾉ◕ヮ◕)ﾉ*:･ﾟ✧     |
| 9  | H  | Send test logs from ESP32            |(ﾉ◕ヮ◕)ﾉ*:･ﾟ✧     |
| 10  | S  | Full system testing            |(_　_)。zＺ     |
| 10  | S  | Finish documentation             | (_　_)。zＺ     |

---

> ### Status codes: 
> - NOT STARTED (_　_)。zＺ
> - IN PROGRESS ✍(◔◡◔)
> - IN SLOW PROGRESS ┗( T﹏T )┛
> - NOT WORKING (ㆆ_ㆆ)
> - TO BE DECIDED ㄟ( ▔, ▔ )ㄏ
> - FINISHED (ﾉ◕ヮ◕)ﾉ*:･ﾟ✧

---

# Testing

## Firmware architecture

Main firmware in [rfid_screen_system_with_lock_take_and_return/rfid_screen_system_with_lock_take_and_return.ino](rfid_screen_system_with_lock_take_and_return/rfid_screen_system_with_lock_take_and_return.ino) is an orchestration layer and delegates responsibilities to focused modules:

- [rfid_screen_system_with_lock_take_and_return/constants.h](rfid_screen_system_with_lock_take_and_return/constants.h) -> hardware pins, timing values, and protocol string constants
- [rfid_screen_system_with_lock_take_and_return/config.h.example](rfid_screen_system_with_lock_take_and_return/config.h.example) -> template for local Wi-Fi and Apps Script configuration
- [rfid_screen_system_with_lock_take_and_return/protocol.h](rfid_screen_system_with_lock_take_and_return/protocol.h) and [rfid_screen_system_with_lock_take_and_return/protocol.cpp](rfid_screen_system_with_lock_take_and_return/protocol.cpp) -> URL builders and server response parsers
- [rfid_screen_system_with_lock_take_and_return/fsm.h](rfid_screen_system_with_lock_take_and_return/fsm.h) and [rfid_screen_system_with_lock_take_and_return/fsm.cpp](rfid_screen_system_with_lock_take_and_return/fsm.cpp) -> explicit TAKE/RETURN finite-state machine transitions
- Hardware effects wrappers:
- [rfid_screen_system_with_lock_take_and_return/display_ui.h](rfid_screen_system_with_lock_take_and_return/display_ui.h) and [rfid_screen_system_with_lock_take_and_return/display_ui.cpp](rfid_screen_system_with_lock_take_and_return/display_ui.cpp)
- [rfid_screen_system_with_lock_take_and_return/led_ui.h](rfid_screen_system_with_lock_take_and_return/led_ui.h) and [rfid_screen_system_with_lock_take_and_return/led_ui.cpp](rfid_screen_system_with_lock_take_and_return/led_ui.cpp)
- [rfid_screen_system_with_lock_take_and_return/lock_control.h](rfid_screen_system_with_lock_take_and_return/lock_control.h) and [rfid_screen_system_with_lock_take_and_return/lock_control.cpp](rfid_screen_system_with_lock_take_and_return/lock_control.cpp)
- [rfid_screen_system_with_lock_take_and_return/rfid_reader.h](rfid_screen_system_with_lock_take_and_return/rfid_reader.h) and [rfid_screen_system_with_lock_take_and_return/rfid_reader.cpp](rfid_screen_system_with_lock_take_and_return/rfid_reader.cpp)
- [rfid_screen_system_with_lock_take_and_return/http_client.h](rfid_screen_system_with_lock_take_and_return/http_client.h) and [rfid_screen_system_with_lock_take_and_return/http_client.cpp](rfid_screen_system_with_lock_take_and_return/http_client.cpp)

## System workflow
The system operates using a two-step scanning process that determines whether a key is being taken or returned.

**Key issuing (TAKE):**
1. User scans their card
2. System verifies if the user is authorized
3. User scans the key NFC tag
4. System logs the action as TAKE
5. Key status is updated to OUT
6. Green LED indicates success

**Key return (RETURN):**
1. User scans the key NFC tag first
2. System recognizes that the key is currently taken
3. User scans their card
4. System logs the action as RETURN
5. Key status is updated to IN
6. Green LED indicates success

**Invalid actions:**
- Unknown UID → denied
- Inactive user → denied
- Taking unavailable key (key status OUT) → denied
- Returning already available key (key status IN) → denied

## Repository structure
- [rfid_screen_system_with_lock_take_and_return](https://github.com/anastasijaIZV/key-managment-cabinet/tree/main/rfid_screen_system_with_lock_take_and_return) → ESP32 firmware (main system logic)
- [Code.gs](https://github.com/anastasijaIZV/key-managment-cabinet/blob/main/Code.gs) → Google Apps Script backend
- Folder [other ESP NFC scripts](https://github.com/anastasijaIZV/key-managment-cabinet/tree/main/other%20ESP%20NFC%20scripts) and [other ESP scripts](https://github.com/anastasijaIZV/key-managment-cabinet/tree/main/other%20ESP%20RFID%20scripts)→ ESP firmware from different steps of development (not essential)

## Hardware requirements

To test the system, the following components are required:
- ESP32 microcontroller
- RFID reader
- 2x LEDs (green and red)
- RFID tags (for keys)
- RFID school card (for users)
- Breadboard + jumper wires

## Wiring overview

                       ┌──────────────────────────────┐
                       │          Wi-Fi / Web         │
                       │   Google Apps Script + Sheet │
                       └──────────────┬───────────────┘
                                      │
                                 Wi-Fi│
                                      │
                         ┌────────────▼────────────┐
                         │         ESP32           │
                         │   Main controller       │
                         │                         │
                         │ GPIO18  -> OLED SDA     │
                         │ GPIO19  -> OLED SCL     │
                         │ GPIO25  -> RDM6300 TX   │
                         │ GPIO23  -> Green LED    │
                         │ GPIO32  -> Red LED      │
                         │ GPIO5   -> Relay SIG    │
                         │ power   -> laptop USB port │
                         │ GND     -> common GND   │
                         └─────┬─────────┬─────────┘
                               │         │
                               │         │
                ┌──────────────┘         └──────────────┐
                │                                       │
        ┌───────▼────────┐                     ┌────────▼────────┐
        │   OLED Screen  │                     │  RDM6300 RFID   │
        │   I2C display  │                     │    reader       │
        │                │                     │                 │
        │ SDA <- GPIO18  │                     │ TX  -> GPIO25   │
        │ SCL <- GPIO19  │                     │ VCC -> 5V       │
        │ VCC <- 3.3V    │                     │ GND -> GND      │
        │ GND <- GND     │                     └─────────────────┘
        └────────────────┘

                         ┌──────────────────────────┐
                         │   Relay / lock driver    │
                         │  (controlled by GPIO5)   │
                         │                          │
                         │ SIG    <- GPIO5          │
                         │ +      <- module power*  │
                         │ -      <- common GND     |
                         │ NO     <- lock +         |
                         | COM    <- +12V PSU
                         └───────────┬──────────────┘
                                     │ 
                                     │
                     ┌───────────────▼────────────────┐
                     │         12V power supply       │
                     │                                │
                     │ +12V  --------------------┐    │
                     │ -12V   -----------------┐ │    │
                     └─────────────────────────│─│────┘
                                               │ │
                                  ┌────────────▼─▼────────────┐
                                  │      Electric lock        │
                                  │                           │
                                  │ red wire -> Relay NO      │
                                  │ black wire -> -12V        |
                                  └───────────────────────────┘

## Google Sheets setup

Create a Google Spreadsheet with the following sheets:

**Users**
|UID| UserName| Active|
|------|----------|-----|
|  C3B71807  | Vārds (Uzvārds) |TRUE |

**Keys**
|UID|KeyName| Status| HolderUID |
|------|----------|-----|----|
|  836D2607  | Key Name |  | |

**Logs**
| Timestamp | Action | User UID | User name | Key UID | Key name | Result |
|-----|-----|------|------|-------|------|-----|
|  |  |  |  |  |  |  |

### Apps Script setup
1. Open `Extensions` → `Apps Script`
2. Paste the contents of `Code.gs`
3. Save the project
4. Click `Deploy` → `New deployment`
5. Select `Web app`
6. Set access to: `Anyone`
7. Copy the generated URL

## ESP32 setup

In [rfid_screen_system_with_lock_take_and_return](https://github.com/anastasijaIZV/key-managment-cabinet/tree/main/rfid_screen_system_with_lock_take_and_return):

1. Copy [rfid_screen_system_with_lock_take_and_return/config.h.example](rfid_screen_system_with_lock_take_and_return/config.h.example) to `config.h`
2. Update `config.h` with:

- WiFi SSID
- WiFi password
- Apps Script URL

Upload the code using Arduino IDE.

## Running the system
1. Power the ESP32
2. Open Serial Monitor (115200 baud)
3. Wait for WiFi connection
4. Scan a card or tag
5. Observe:
   * Serial output
   * LED feedback
   * Google Sheets logs

## LED feedback
- Green LED (one blink) → valid singular scan (KEY/CARD)
- Green LED (3 blinks) → successful action (TAKE/RETURN)
- Red LED (one blink) → error / denied action
- Red LED (3 blinks) → session time-out




