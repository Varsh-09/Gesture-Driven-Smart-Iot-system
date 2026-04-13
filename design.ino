#define BLYNK_TEMPLATE_ID "TMPL3OgTGoPKw"
#define BLYNK_TEMPLATE_NAME "Gesture IoT"
#define BLYNK_AUTH_TOKEN "aEmI8yR6jwGMEgRGvrco7MDNssoCYmDS"
#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <MPU6050.h>

char ssid[] = "ESP32";
char pass[] = "12345678";

LiquidCrystal_I2C lcd(0x27, 16, 2);
MPU6050 mpu;

int16_t ax, ay, az;
int buzzer = 15;

String lastAction = "";
int repeatCount = 0;
unsigned long lastTriggerTime = 0;
String lastDisplay = "";

void lcdPrint(String msg) {
  if (msg != lastDisplay) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(msg);
    lastDisplay = msg;
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();
  mpu.initialize();
  pinMode(buzzer, OUTPUT);
  digitalWrite(buzzer, LOW);
  lcdPrint("Gesture IoT");
  delay(2000);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass, "blynk.cloud", 80);
}

void loop() {
  Blynk.run();

  mpu.getAcceleration(&ax, &ay, &az);

  String currentAction = "";

  if (ay > 10000)       currentAction = "NEED WATER";
  else if (ay < -10000) currentAction = "NEED FOOD";
  else if (ax > 10000)  currentAction = "USE WASHROOM";
  else if (ax < -10000) currentAction = "NEED MEDICINE";

  // -------- NO TILT - show NORMAL on LCD only, nothing sent to Blynk --------
  if (currentAction == "") {
    lcdPrint("NORMAL");
    repeatCount = 0;
    lastAction = "";
    digitalWrite(buzzer, LOW);
    delay(1000);
    return;   // stop here - no Blynk action
  }

  // -------- TILT DETECTED - display on LCD --------
  lcdPrint(currentAction);

  // -------- SEND TO BLYNK ONLY WHEN TILT IS DETECTED --------
  if (millis() - lastTriggerTime > 3000) {
    lastTriggerTime = millis();

    Serial.println("Gesture: " + currentAction);
    Blynk.logEvent("status_update", "User needs: " + currentAction);

    // -------- REPEAT COUNT --------
    if (currentAction == lastAction) {
      repeatCount++;
    } else {
      repeatCount = 1;
      lastAction = currentAction;
    }

    Serial.println("Repeat count: " + String(repeatCount));

    // -------- EMERGENCY - same gesture more than 3 times continuously --------
    if (repeatCount > 3) {
      Serial.println("EMERGENCY!");
      lcdPrint("EMERGENCY!");
      Blynk.logEvent("emergency_alert", "Repeated request: " + currentAction);
      digitalWrite(buzzer, HIGH);
      delay(2000);
      digitalWrite(buzzer, LOW);
      repeatCount = 0;
      lastAction = "";
    }
  }

  delay(1000);
}
