#include <Arduino.h>
#include "esp_camera.h"
#include "config.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <QrDecoder.h>

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
void blinkLed(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_GPIO_NUM, HIGH);
    delay(100);
    digitalWrite(LED_GPIO_NUM, LOW);
    delay(100);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Hello from ESP32-CAM!");

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG; // Using JPEG for now, can change to GRAYSCALE later for QR
  config.frame_size = FRAMESIZE_QVGA; // QVGA (320x240) for QR code scanning
  config.jpeg_quality = 12; // 0-63 lower number means higher quality
  config.fb_count = 1;

  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

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
  // Frame aufnehmen
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed, retry with flash");
    // Versuche mit Flash
    digitalWrite(LED_GPIO_NUM, HIGH);
    delay(100);
    fb = esp_camera_fb_get();
    digitalWrite(LED_GPIO_NUM, LOW);
    if (!fb) {
      Serial.println("Second capture failed, blinking error");
      blinkLed(4);
      delay(5000);
      return;
    }
  }

  // QR-Code dekodieren
  QrDecoder qr;
  String decoded = qr.decode(fb->buf, fb->len);
  esp_camera_fb_return(fb);

  if (decoded.length() == 0) {
    Serial.println("QR decode failed");
    blinkLed(4);
    delay(5000);
    return;
  }

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
    blinkLed(1);
  } else if (status == "already registered") {
    blinkLed(2);
  } else {
    blinkLed(4);
  }

  delay(5000); // Nächster Scan
}
