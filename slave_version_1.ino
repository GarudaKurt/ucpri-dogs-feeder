#include "HX711.h"
#include <DHT22.h>

// ----------------- LOAD CELLS ------------------

// Load cell 1
#define DOUT1 4
#define SCK1 5

// Load cell 2
#define DOUT2 11
#define SCK2 12

HX711 scale1;
HX711 scale2;

// Calibration values
#define SCALE1  -104.436119
#define OFFSET1  412772

#define SCALE2  -60.797107
#define OFFSET2 -156361

// ----------------- ULTRASONIC ------------------
#define TRIG_PIN 8
#define ECHO_PIN 9

// ----------------- DHT22 SENSOR ----------------
#define DHT_PIN 6
DHT22 dht(DHT_PIN);

// ----------------- FAN -------------------------
const int fanPin = 2;

// ----------------- Timing ----------------------
unsigned long lastSend = 0;
const unsigned long interval = 1000; // send every 1 second

// ---------- Ultrasonic reading ----------
float getDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH);
  float distance = duration * 0.0343 / 2.0; // cm
  return distance;
}

void setup() {
  Serial.begin(9600); // send to master

  // Load cells
  scale1.begin(DOUT1, SCK1);
  scale2.begin(DOUT2, SCK2);

  scale1.set_scale(SCALE1);
  scale1.set_offset(OFFSET1);
  scale1.tare(); // zeroing

  scale2.set_scale(SCALE2);
  scale2.set_offset(OFFSET2);
  scale2.tare();

  // Ultrasonic
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Fan
  pinMode(fanPin, OUTPUT);
  digitalWrite(fanPin, HIGH); // default OFF
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - lastSend >= interval) {
    lastSend = currentMillis;

    // ----- Read sensors -----
    float weight1 = scale1.get_units(10);
    float weight2 = scale2.get_units(10);
    float totalWeight = weight1 + weight2;

    float distance = getDistanceCM();
    float temperature = dht.getTemperature();
    float humidity = dht.getHumidity();

    // ----- Fan control -----
    if (temperature > 30 || humidity > 60) {
      digitalWrite(fanPin, LOW); // turn on
    } else {
      digitalWrite(fanPin, HIGH); // turn off
    }

    // ----- Send to Master -----
    // Format: "W:xx.xx W2:yy.yy D:zz.zz T:aa.aa H:bb.bb"
    Serial.print("W1:");
    Serial.print(weight1, 2);
    Serial.print(" W2:");
    Serial.print(weight2, 2);
    Serial.print(" D:");
    Serial.print(distance, 2);
    Serial.print(" T:");
    Serial.print(temperature, 1);
    Serial.print(" H:");
    Serial.println(humidity, 1);
  }
}
