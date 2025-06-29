#include <Arduino.h>
#include "esp_camera.h"
#include "config.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ESP32QRCodeReader.h>

// CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#define LED_GPIO_NUM      33 // For AI-Thinker, this is the flash LED

// LED blink-Funktion: kurz HIGH-LOW wiederholen
// 2 short blinks for OK/success
// 3 short blinks for "already registered"
// 4 short blinks for "id not known"
// 5 short blinks for an error
void blinkLed(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_GPIO_NUM, HIGH);
    delay(100);
    digitalWrite(LED_GPIO_NUM, LOW);
    delay(100);
  }
}

ESP32QRCodeReader reader;

void setup() {
  Serial.begin(115200);
  Serial.println("Hello from ESP32-CAM!");

  reader.setup();
  Serial.println("Setup finished");

  reader.begin();
  Serial.println("Camera started");

  // Configure flash LED
  pinMode(LED_GPIO_NUM, OUTPUT);
  digitalWrite(LED_GPIO_NUM, LOW); // Turn off flash LED initially

  // WiFi-Verbindung herstellen
  Serial.printf("Connecting to WiFi: %s\n", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected, IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  struct QRCodeData qrCodeData;

  if (reader.receiveQrCode(&qrCodeData, 100)) {
    Serial.println("Found QR code");
    if (qrCodeData.valid) {
      String decoded = String((const char*)qrCodeData.payload);
      Serial.printf("Decoded QR: %s\n", decoded.c_str());

      // Anfrage an Backend senden
      HTTPClient http;
      http.begin(API_URL);
      http.addHeader("Content-Type", "application/json");
      http.addHeader("X-API-Token", API_TOKEN);

      StaticJsonDocument<200> doc;
      doc["id"] = decoded;
      String body;
      serializeJson(doc, body);

      int httpCode = http.POST(body);
      String payload = http.getString();
      http.end();

      Serial.printf("HTTP %d: %s\n", httpCode, payload.c_str());

      String status;
      DeserializationError err = deserializeJson(doc, payload);
      if (!err && doc.containsKey("status")) {
        status = doc["status"].as<String>();
      }

      if (httpCode == 200 && status == "ok") {
        blinkLed(2); // 2 blinks for OK/success
      } else if (status == "already registered") {
        blinkLed(3); // 3 blinks for "already seen"
      } else if (status == "id not known") {
        blinkLed(4); // 4 blinks for "id not known"
      } else {
        blinkLed(5); // 5 blinks for error
      }
    } else {
      Serial.print("Invalid: ");
      // Serial.println(qrCodeData.message); // This is not available in the struct
      blinkLed(5); // 5 blinks for error
    }
  }
  delay(5000); // Nächster Scan
}
