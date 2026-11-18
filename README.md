# IoT-Based Fire Detection and Alert System
CS3402 – Internet of Things Lab | Mini Project | R V University

## Overview
An Arduino Uno monitors flame, smoke/gas, temperature and humidity. Threshold logic classifies the room as **SAFE / WARNING / FIRE**, drives a LED + buzzer alarm pattern and shows live readings on an OLED. The Uno also sends each reading over USB serial (CSV) to a laptop, where a Python gateway sends **Telegram notifications** to the user's phone.

![architecture](system_architecture.png)

## Hardware
Arduino Uno, flame sensor (digital), MQ-2 smoke sensor, DHT11, active buzzer, red + green LED (220 Ω each), 0.96" SSD1306 I2C OLED, push button, breadboard, jumper wires, USB cable. Laptop and a phone with Telegram (already owned).

## Pin connections
| Component | Pin | Arduino |
|---|---|---|
| Flame sensor | DO | D2 |
| DHT11 | DATA | D3 |
| MQ-2 | AO | A0 |
| Push button | other leg to GND | D4 (INPUT_PULLUP) |
| Buzzer | (+) | D8 |
| Red LED | anode via 220 Ω | D9 |
| Green LED | anode via 220 Ω | D10 |
| OLED | SDA / SCL | A4 / A5 |

All VCC → 5V, all GND → GND. See `circuit_diagram.png`.

## Logic
- **FIRE**: flame detected, or (smoke > 400 and temperature > 45 °C)
- **WARNING**: smoke > 400 or temperature > 45 °C
- **SAFE**: otherwise

The push button silences the buzzer; it re-arms when the status returns to SAFE.

## Setup
1. **Arduino**: install libraries *DHT sensor library, Adafruit Unified Sensor, Adafruit GFX, Adafruit SSD1306*; upload `fire_detection_system.ino`; close the Serial Monitor.
2. **Telegram bot**: in Telegram open **@BotFather** → `/newbot` → copy the token. Send any message to your bot, then open `https://api.telegram.org/bot<TOKEN>/getUpdates` and copy the number after `"chat":{"id":`.
3. **Gateway**: `pip install -r requirements.txt`, copy `config_example.py` to `config.py`, fill in token, chat id and COM port.
4. Run `python telegram_gateway.py`.

## Telegram behaviour
- Alerts go to every subscriber: anyone who opens the bot and sends `/start` is subscribed (`/stop` unsubscribes)
- Message on every status change (WARNING, FIRE, back to SAFE)
- FIRE alert repeated every 30 s until it clears
- `/status` returns the latest readings (subscribers only)
- All readings saved to `fire_log.csv` with timestamps

## Files
- `fire_detection_system.ino` – Arduino program
- `telegram_gateway.py` – serial → Telegram gateway
- `config_example.py` – settings template (real `config.py` is git-ignored)
- `circuit_diagram.png`, `system_architecture.png`

## Team
Group No.: ___  |  Members: ___