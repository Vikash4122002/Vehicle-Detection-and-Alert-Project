#include <WiFi.h>
#include <HTTPClient.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>

// WiFi Credentials
const char* ssid = "TP-Link_B080";      // 🔁 Your WiFi SSID
const char* password = "baler net";     // 🔁 Your WiFi Password

// Telegram Credentials
const String BOT_TOKEN = "7793020144:AAH5B86xw9eZLIyCFCQVv6HniNaoOxkhJpA";  // 🔁 Replace with Bot Token from BotFather
const String CHAT_ID = "5716036793";      // 🔁 Replace with your Telegram ID (@userinfobot)

// GPS Setup
TinyGPSPlus gps;
HardwareSerial gpsSerial(1);  // UART1 for GPS

void setup() {
  Serial.begin(115200);
  gpsSerial.begin(9600, SERIAL_8N1, 16, 17); // RX=16, TX=17

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ Connected to WiFi");
}

void loop() {
  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());

    if (gps.location.isUpdated()) {
      float lat = gps.location.lat();
      float lon = gps.location.lng();

      Serial.println("📍 GPS Location Updated:");
      Serial.print("Latitude: "); Serial.println(lat, 6);
      Serial.print("Longitude: "); Serial.println(lon, 6);

      sendTelegramMessage(lat, lon);

      delay(20000); // wait 20 seconds before next alert
    }
  }
}

void sendTelegramMessage(float lat, float lon) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String message = "🚨 *Vehicle Alert!*\n";
    message += "📍 [Google Maps Location](https://maps.google.com/?q=";
    message += String(lat, 6) + "," + String(lon, 6) + ")";

    String url = "https://api.telegram.org/bot" + BOT_TOKEN +
                 "/sendMessage?chat_id=" + CHAT_ID +
                 "&text=" + urlencode(message) +
                 "&parse_mode=Markdown";

    http.begin(url);
    int code = http.GET();
    if (code > 0) {
      Serial.println("✅ Alert Sent to Telegram!");
    } else {
      Serial.print("❌ Telegram Error: ");
      Serial.println(code);
    }
    http.end();
  } else {
    Serial.println("❌ WiFi Not Connected");
  }
}

// URL Encode Function
String urlencode(String str) {
  String encoded = "";
  char c;
  char code0, code1;
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (isalnum(c)) {
      encoded += c;
    } else {
      encoded += '%';
      code0 = (c >> 4) & 0xF;
      code1 = c & 0xF;
      encoded += String("0123456789ABCDEF")[code0];
      encoded += String("0123456789ABCDEF")[code1];
    }
  }
  return encoded;
}
