#include <WiFi.h>
#include <HTTPClient.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// WiFi Credentials
const char* ssid = "TP-Link_B080";
const char* password = "baler net";

// Telegram Bot Credentials
const String BOT_TOKEN = "7793020144:AAH5B86xw9eZLIyCFCQVv6HniNaoOxkhJpA";
const String CHAT_ID = "5716036793";

// LCD Setup
LiquidCrystal_I2C lcd(0x27, 16, 2);  // If not showing, try 0x3F

// GPS Setup
TinyGPSPlus gps;
HardwareSerial gpsSerial(1);  // use UART1

// Cancel Button Setup
#define CANCEL_BUTTON_PIN 4
bool accidentDetected = false;
bool countdownActive = false;
unsigned long countdownStart = 0;
const unsigned long COUNTDOWN_DURATION = 15000; // 15 seconds
float pendingLat = 0.0;
float pendingLon = 0.0;

unsigned long lastSend = 0;
const unsigned long SEND_INTERVAL = 20000; // 20 seconds

void setup() {
  Serial.begin(115200);
  gpsSerial.begin(9600, SERIAL_8N1, 17, 16); // GPS TX=16, RX=17

  pinMode(CANCEL_BUTTON_PIN, INPUT_PULLUP);

  // LCD Init
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Vehicle Alert");
  lcd.setCursor(0, 1);
  lcd.print("System Booting...");
  delay(2000);
  lcd.clear();

  // WiFi Connection
  Serial.println("📶 Connecting to WiFi...");
  WiFi.begin(ssid, password);
  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 15000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ Connected to WiFi!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Connected");
    lcd.setCursor(0, 1);
    lcd.print("IP OK");
  } else {
    Serial.println("\n❌ WiFi Failed");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Failed");
  }

  delay(2000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("GPS Waiting...");
}

void loop() {
  // Read GPS continuously
  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }

  // Debounced cancel button logic
  static bool lastButtonState = HIGH;
  static unsigned long lastDebounceTime = 0;
  const unsigned long debounceDelay = 50;
  bool reading = digitalRead(CANCEL_BUTTON_PIN);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading == LOW && countdownActive) {
      Serial.println("🛑 CANCEL button pressed! Countdown stopped.");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Alert Cancelled");
      countdownActive = false;
      accidentDetected = false;
      delay(1500);
      lcd.clear();
    }
  }
  lastButtonState = reading;

  // Countdown logic
  if (countdownActive && (millis() - countdownStart >= COUNTDOWN_DURATION)) {
    Serial.println("⏰ Countdown finished! Sending location...");
    sendTelegramLocation(pendingLat, pendingLon);
    countdownActive = false;
    accidentDetected = false;
    lastSend = millis();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Sent Location!");
    delay(2000);
    lcd.clear();
  }

  // Show countdown on LCD
  if (countdownActive) {
    unsigned long remaining = COUNTDOWN_DURATION - (millis() - countdownStart);
    static unsigned long lastCountdownPrint = 0;
    if (millis() - lastCountdownPrint > 1000) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Alert in: ");
      lcd.print((remaining / 1000) + 1);
      lcd.setCursor(0, 1);
      lcd.print("Press to Cancel");
      Serial.printf("⏳ %d sec left...\n", (int)(remaining / 1000) + 1);
      lastCountdownPrint = millis();
    }
  }

  // Reconnect WiFi if lost
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.reconnect();
    Serial.println("📶 Reconnecting...");
    delay(5000);
  }

  // Simulate accident
  if (gps.location.isValid() && !countdownActive && (millis() - lastSend > SEND_INTERVAL)) {
    float lat = gps.location.lat();
    float lon = gps.location.lng();
    Serial.println("🚨 Accident Detected!");
    Serial.printf("Lat: %.6f, Lon: %.6f\n", lat, lon);
    accidentDetected = true;
    countdownActive = true;
    countdownStart = millis();
    pendingLat = lat;
    pendingLon = lon;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Accident Alert!");
    lcd.setCursor(0, 1);
    lcd.print("15s to cancel");
  }

  // Print GPS status every 10 sec
  static unsigned long lastStatusPrint = 0;
  if (millis() - lastStatusPrint > 10000 && !countdownActive) {
    if (!gps.location.isValid()) {
      Serial.printf("🛰️ GPS No Fix | Sats: %d\n", gps.satellites.value());
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("GPS: No Fix");
      lcd.setCursor(0, 1);
      lcd.print("Sat: ");
      lcd.print(gps.satellites.value());
    } else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("GPS Locked");
      lcd.setCursor(0, 1);
      lcd.print("Lat:");
      lcd.print(gps.location.lat(), 1);
    }
    lastStatusPrint = millis();
  }

  delay(100);
}

void sendTelegramLocation(float lat, float lon) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ WiFi not connected");
    return;
  }

  HTTPClient http;
  String url = "https://api.telegram.org/bot" + BOT_TOKEN + "/sendLocation";

  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  DynamicJsonDocument doc(1024);
  doc["chat_id"] = CHAT_ID;
  doc["latitude"] = lat;
  doc["longitude"] = lon;
  doc["live_period"] = 900;

  String jsonString;
  serializeJson(doc, jsonString);
  int code = http.POST(jsonString);

  if (code == 200) {
    Serial.println("✅ Location sent!");
  } else {
    Serial.printf("❌ Failed to send. Code: %d\n", code);
    Serial.println(http.getString());
  }

  http.end();
  delay(1000);
  sendTelegramAlert(lat, lon);
}

void sendTelegramAlert(float lat, float lon) {
  HTTPClient http;

  String message = "🚨 *Vehicle Alert!*\n";
  message += "📍 Location: " + String(lat, 6) + ", " + String(lon, 6);

  String url = "https://api.telegram.org/bot" + BOT_TOKEN + "/sendMessage";

  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  DynamicJsonDocument doc(1024);
  doc["chat_id"] = CHAT_ID;
  doc["text"] = message;
  doc["parse_mode"] = "Markdown";

  JsonObject replyMarkup = doc.createNestedObject("reply_markup");
  JsonArray inline_keyboard = replyMarkup.createNestedArray("inline_keyboard");
  JsonArray row = inline_keyboard.createNestedArray();
  JsonObject button = row.createNestedObject();
  button["text"] = "🗺️ Open Google Maps";
  button["url"] = "https://maps.google.com/?q=" + String(lat, 6) + "," + String(lon, 6);

  String jsonString;
  serializeJson(doc, jsonString);
  int code = http.POST(jsonString);

  if (code == 200) {
    Serial.println("✅ Alert message sent!");
  } else {
    Serial.printf("❌ Alert failed. HTTP code: %d\n", code);
    Serial.println(http.getString());
  }

  http.end();
}