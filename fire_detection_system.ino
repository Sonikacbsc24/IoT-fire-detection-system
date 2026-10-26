/*
 * CS3402 IoT Lab - Mini Project (Phase 1)
 * IoT-Based Fire Detection and Alert System
 * Board  : Arduino Uno
 * Sensors: Flame sensor (D2), DHT11 (D3), MQ-2 smoke/gas (A0), push button (D4)
 * Outputs: Active buzzer (D8), Red LED (D9), Green LED (D10), SSD1306 OLED (I2C: A4/A5)
 *
 * Libraries (Sketch > Include Library > Manage Libraries):
 *   - DHT sensor library (Adafruit)  + Adafruit Unified Sensor
 *   - Adafruit GFX Library
 *   - Adafruit SSD1306
 *
 * Status logic (simple threshold logic):
 *   SAFE    : nothing abnormal                      -> green LED, no buzzer
 *   WARNING : smoke high OR temperature high        -> red LED + slow beep
 *   FIRE    : flame detected OR (smoke AND temp high)-> red LED + fast alarm beep
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

// ---------- Pin definitions ----------
#define FLAME_PIN   2
#define DHT_PIN     3
#define BUTTON_PIN  4
#define BUZZER_PIN  8
#define RED_LED     9
#define GREEN_LED   10
#define SMOKE_PIN   A0

#define DHTTYPE     DHT11
#define OLED_WIDTH  128
#define OLED_HEIGHT 64
#define OLED_ADDR   0x3C        // change to 0x3D if the OLED is not detected

// ---------- Thresholds (calibrate in your lab) ----------
const int   SMOKE_THRESHOLD = 400;    // MQ-2 analog value (0-1023)
const float TEMP_THRESHOLD  = 45.0;   // degrees Celsius

// ---------- Status codes ----------
const byte SAFE = 0, WARNING = 1, FIRE = 2;

DHT dht(DHT_PIN, DHTTYPE);
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

float temperature = 0, humidity = 0;
int   smokeValue = 0;
bool  flameDetected = false;
byte  status = SAFE;
bool  muted = false;

unsigned long lastSense = 0, lastToggle = 0;
bool blinkState = false;

void setup() {
  Serial.begin(9600);
  pinMode(FLAME_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);

  dht.begin();
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("OLED not found"));
    while (true);                       // stop here if OLED is missing
  }

  // MQ-2 heater needs ~20 s to stabilise
  for (int s = 20; s > 0; s--) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println(F("Fire Detection System"));
    display.println();
    display.println(F("Warming up MQ-2..."));
    display.setTextSize(2);
    display.setCursor(0, 40);
    display.print(s);
    display.print(F(" s"));
    display.display();
    delay(1000);
  }
  Serial.println(F("temp_C,humidity_%,smoke,flame,status"));
}

void loop() {
  checkButton();

  // read sensors once per second
  if (millis() - lastSense >= 1000) {
    lastSense = millis();
    readSensors();
    evaluateStatus();
    updateDisplay();
    printSerial();
  }
  updateOutputs();    // non-blocking LED / buzzer pattern
}

// ---------- Sensor reading ----------
void readSensors() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (!isnan(t)) temperature = t;       // keep last good value if read fails
  if (!isnan(h)) humidity = h;

  smokeValue = analogRead(SMOKE_PIN);
  flameDetected = (digitalRead(FLAME_PIN) == LOW);   // most modules: LOW = flame
}

// ---------- Decision logic ----------
void evaluateStatus() {
  bool smokeHigh = (smokeValue > SMOKE_THRESHOLD);
  bool tempHigh  = (temperature > TEMP_THRESHOLD);

  if (flameDetected || (smokeHigh && tempHigh)) status = FIRE;
  else if (smokeHigh || tempHigh)               status = WARNING;
  else                                          status = SAFE;

  if (status == SAFE) muted = false;    // re-arm the buzzer when all is clear
}

// ---------- Silence button ----------
void checkButton() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(30);                                   // debounce
    if (digitalRead(BUTTON_PIN) == LOW) {
      muted = !muted;
      while (digitalRead(BUTTON_PIN) == LOW);    // wait for release
    }
  }
}

// ---------- LED + buzzer alarm pattern ----------
void updateOutputs() {
  if (status == SAFE) {
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(RED_LED, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    return;
  }
  digitalWrite(GREEN_LED, LOW);
  unsigned long interval = (status == FIRE) ? 150 : 500;   // fast for FIRE, slow for WARNING
  if (millis() - lastToggle >= interval) {
    lastToggle = millis();
    blinkState = !blinkState;
  }
  digitalWrite(RED_LED, blinkState);
  digitalWrite(BUZZER_PIN, (blinkState && !muted) ? HIGH : LOW);
}

// ---------- OLED ----------
void updateDisplay() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(F("Temp : ")); display.print(temperature, 1); display.println(F(" C"));
  display.print(F("Humid: ")); display.print(humidity, 0);    display.println(F(" %"));
  display.print(F("Smoke: ")); display.println(smokeValue);
  display.print(F("Flame: ")); display.println(flameDetected ? F("YES") : F("NO"));

  display.setTextSize(2);
  display.setCursor(0, 44);
  if (status == SAFE)         display.print(F("SAFE"));
  else if (status == WARNING) display.print(F("WARNING"));
  else                        display.print(F("FIRE!!"));
  if (muted && status != SAFE) { display.setTextSize(1); display.print(F(" mute")); }
  display.display();
}

// ---------- Serial output (also used later for Raspberry Pi logging) ----------
void printSerial() {
  Serial.print(temperature, 1); Serial.print(',');
  Serial.print(humidity, 0);    Serial.print(',');
  Serial.print(smokeValue);     Serial.print(',');
  Serial.print(flameDetected ? 1 : 0); Serial.print(',');
  if (status == SAFE)         Serial.println(F("SAFE"));
  else if (status == WARNING) Serial.println(F("WARNING"));
  else                        Serial.println(F("FIRE"));
}
