# IoT-fire-detection-system
IoT-Based Fire Detection and Alert System


CS3402 – Internet of Things Lab | Mini Project | R V University

Overview

An Arduino Uno monitors flame, smoke/gas, temperature and humidity. Simple threshold logic classifies the room as SAFE / WARNING / FIRE, drives an LED + buzzer alarm pattern, and shows live readings on an OLED. Readings are also sent over serial (CSV) for Raspberry Pi / PC logging.

# Components

Arduino Uno, flame sensor (digital), MQ-2 smoke sensor, DHT11, active buzzer, red + green LED (220 Ω each), 0.96" SSD1306 I2C OLED, push button, breadboard, jumper wires.

# Pin connections
Component	Pin	Arduino

Flame sensor	DO	D2

DHT11	DATA	D3

MQ-2	AO	A0

Push button	other leg to GND	D4 (INPUT_PULLUP)

Buzzer	(+)	D8

Red LED	anode via 220 Ω	D9

Green LED	anode via 220 Ω	D10

OLED	SDA / SCL	A4 / A5

All VCC → 5V, all GND → GND.	

# Logic

FIRE: flame detected, or (smoke > 400 and temperature > 45 °C)

WARNING: smoke > 400 or temperature > 45 °C

SAFE: otherwise Push button silences the buzzer; it re-arms when status returns to SAFE.

# Libraries

DHT sensor library, Adafruit Unified Sensor, Adafruit GFX, Adafruit SSD1306.

# Files

fire_detection_system.ino – Arduino code

circuit_diagram.png – connection diagram

# Team

Group No.: 4

Members: 

1. Prathibha. B - 1RUA24SCS0081

2. Shreeraksha. S. Jadhav - 1RUA24SCS0106

3. Sonika. C - 1RUA24SCS0111
