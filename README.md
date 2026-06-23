# ESP32 RFID Doorlock System (RC522)

A secure and interactive door access control system powered by the ESP32 and the RC522 RFID module. This project features real-time feedback, authentication, and a security lockout mechanism.

## Features
* **Secure Authentication:** Validates registered RFID UIDs against a pre-defined list.
* **Toggle Logic:** Easily toggle the lock status (ON/OFF) with a single registered card.
* **Visual & Audio Feedback:** * Dedicated LEDs for success, access granted, and access denied status.
    * Distinctive buzzer tones for successful entry vs. unauthorized attempts.
* **Security Lockout:** Implements a 60-second lockout after 3 consecutive failed attempts to prevent brute-force attacks.
* **Debounce Protection:** Ensures a card is only processed once while held over the sensor.

## Components
* **Microcontroller:** ESP32 DevKit V1
* **RFID Module:** RC522
* **Audio:** Passive Buzzer
* **Indicators:** 3x LEDs (Status: Success, Denied, Active)

## Pin Configuration
| Component | GPIO Pin |
| :--- | :--- |
| RC522 MISO | GPIO 25 |
| RC522 MOSI | GPIO 23 |
| RC522 SCLK | GPIO 19 |
| RC522 SDA | GPIO 22 |
| Buzzer | GPIO 18 |
| Main Status LED | GPIO 2 |
| Access Denied LED | GPIO 4 |
| Access Granted LED | GPIO 5 |

## How to Run
1. Ensure your ESP-IDF development environment is set up.
2. Clone this repository:
   ```bash
   git clone https://github.com/hafidzsyifa77-lab/RFID-RC522_project
