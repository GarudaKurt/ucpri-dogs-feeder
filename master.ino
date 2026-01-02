#include <ThreeWire.h>
#include <RtcDS1302.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <ESP32Servo.h>

// ------------------------- Servo -------------------------
Servo servo;
#define pinServo 32

// ------------------------- OLED -------------------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ------------------------- RTC DS1302 -------------------------
#define RST_PIN 27
#define DAT_PIN 25
#define CLK_PIN 26
ThreeWire myWire(DAT_PIN, CLK_PIN, RST_PIN);
RtcDS1302<ThreeWire> rtc(myWire);

// ------------------------- Variables -------------------------
float weight1 = 0;
float weight2 = 0;
float distance = 0;
float temperature = 0;
float humidity = 0;

// ------------------------- Feeding Flags -------------------------
bool breakfastDone = false;
bool lunchDone = false;
bool dinnerDone = false;

unsigned long foodsServedTime = 0;

void setup() {
  Serial.begin(9600); // receive from Slave
  delay(200);

  // Initialize OLED
  display.begin(0x3C, true);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.println("MASTER READY...");
  display.display();

  // Initialize RTC
  rtc.Begin();
  if (!rtc.IsDateTimeValid()) {
    Serial.println("RTC is NOT valid! Setting compile time...");
    RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
    rtc.SetDateTime(compiled);
  } else {
    Serial.println("RTC is valid.");
  }

  // Initialize servo
  servo.attach(pinServo);
  servo.write(90);
}

void loop() {

  if (Serial.available()) {
    String msg = Serial.readStringUntil('\n');
    msg.trim();
    // Parse data from slave
    int w1Index = msg.indexOf("W1:");
    int w2Index = msg.indexOf("W2:");
    int dIndex  = msg.indexOf("D:");
    int tIndex  = msg.indexOf("T:");
    int hIndex  = msg.indexOf("H:");

    if (w1Index != -1 && w2Index != -1 && dIndex != -1 && tIndex != -1 && hIndex != -1) {
      weight1 = msg.substring(w1Index + 3, w2Index).toFloat();
      weight2 = msg.substring(w2Index + 3, dIndex).toFloat();
      distance = msg.substring(dIndex + 2, tIndex).toFloat();
      temperature = msg.substring(tIndex + 2, hIndex).toFloat();
      humidity = msg.substring(hIndex + 2).toFloat();
    }
  }

  // ------------------------------- Read RTC -------------------------------
  RtcDateTime now = rtc.GetDateTime();
  int hour = now.Hour();
  int minute = now.Minute();

  char dateStr[11]; // YYYY/MM/DD
  char timeStr[9];  // HH:MM:SS
  snprintf(dateStr, sizeof(dateStr), "%04u/%02u/%02u", now.Year(), now.Month(), now.Day());
  snprintf(timeStr, sizeof(timeStr), "%02u:%02u:%02u", hour, minute, now.Second());

  // ------------------------------- Feeding Logic -------------------------------
  // Reset flags at the start of the next schedule
  if(hour == 12 && minute == 0) breakfastDone = false;
  if(hour == 17 && minute == 0) lunchDone = false;
  if(hour == 0 && minute == 0) dinnerDone = false;

  // Breakfast: 6:00 - 11:59
  if(hour >= 6 && hour < 12 && !breakfastDone && distance < 50 && weight2 >= 20) {
    if(weight1 < 50) {
      Serial.println("Breakfast Food is serve...");
      servo.write(180);
    } else {
      Serial.println("Breakfast Food is done serve");
      servo.write(90);
      breakfastDone = true;
    }
  }

  // Lunch: 12:00 - 16:59
  if(hour >= 12 && hour < 17 && !lunchDone && distance < 50 && weight2 >= 20) {
    if(weight1 < 50) {
      Serial.println("Lunch Food is serve");
      servo.write(180);
    } else {
      Serial.println("Lunch Food is done  serve");
      servo.write(90);
      lunchDone = true;
    }
  }

  // Dinner: 17:00 - 23:00
  if(hour >= 17 && hour < 23 && !dinnerDone && distance < 50 && weight2 >= 20) {
    if(weight1 < 50) {
      Serial.println("Dinner Food is serve");
      servo.write(180);
    } else {
      Serial.println("Dinner Food is  done serve");
      servo.write(90);
      dinnerDone = true;
    }
  }

  display.clearDisplay();
  display.setCursor(0, 0);

  display.print("Date: ");
  display.println(dateStr);

  display.print("Time: ");
  display.println(timeStr);

  display.print("Humid: ");
  display.print(humidity, 1);
  display.println(" %");

  display.print("Temp: ");
  display.print(temperature, 1);
  display.println(" C");

  display.print("Load1: ");
  if(weight1 < 0)
    weight1 = 0;
  display.print(weight1, 2);
  display.println(" g");

  display.print("Load2: ");
  if(weight2 < 10)
    weight2 = 0;
  display.print(weight2, 2);
  display.println(" g");

  display.print("Distance: ");
  display.print(distance, 2);
  display.println(" cm");

  display.setCursor(0, 56);
  if (breakfastDone || lunchDone || dinnerDone) {
    if(foodsServedTime == 0) foodsServedTime = millis(); // start timer
    if(millis() - foodsServedTime < 3000) {
      display.println("Foods already served. Wait...");
    }
    } else {
    foodsServedTime = 0; // reset timer when no meal served
  }

  display.display();

  delay(500);
}
