# ESP32-CAM QR Code Scanner

ESP32-CAM firmware for scanning QR codes and checking them in via a backend API.

## Features

- QR code scanning using ESP32-CAM
- WiFi connectivity
- Backend API integration for check-in
- LED feedback for scan results
- Automatic retry scanning every 5 seconds

## Hardware Requirements

- ESP32-CAM (AI-Thinker model)
- MicroSD card (optional)
- USB-to-Serial adapter for programming

## LED Feedback Patterns

The onboard LED (GPIO 33) provides visual feedback:

- **2 blinks**: Successful check-in
- **3 blinks**: ID already registered
- **4 blinks**: ID not known by backend
- **5 blinks**: Error (invalid QR code, network error, etc.)

## Configuration

1. Copy `src/config.h` and update the following values:
   ```cpp
   #define WIFI_SSID "your_wifi_network"
   #define WIFI_PASSWORD "your_wifi_password"
   #define API_URL "http://your-backend-url/?action=checkin"
   #define API_TOKEN "your_api_token"
   ```

## Backend API

The ESP32-CAM communicates with a PHP backend using the following API:

- **Endpoint**: `POST /?action=checkin`
- **Headers**: 
  - `Content-Type: application/json`
  - `X-API-Token: your_api_token`
- **Request Body**: `{"id": "scanned_qr_code_content"}`
- **Response**: `{"status": "ok|already registered|id not known|error", "message": "..."}`

## Building and Flashing

1. Install PlatformIO
2. Connect ESP32-CAM via USB-to-Serial adapter
3. Build and upload:
   ```bash
   pio run --target upload
   ```

## Serial Monitor

Monitor the device output:
```bash
pio device monitor
```

## Dependencies

- `alvarowolfx/ESP32QRCodeReader` - QR code detection and decoding
- `bblanchon/ArduinoJson@^6.18.5` - JSON parsing for API communication

## Pin Configuration (AI-Thinker ESP32-CAM)

The firmware is configured for the AI-Thinker ESP32-CAM module with the following pin assignments:

- **Camera Data**: Y9(35), Y8(34), Y7(39), Y6(36), Y5(21), Y4(19), Y3(18), Y2(5)
- **Camera Control**: XCLK(0), PCLK(22), VSYNC(25), HREF(23)
- **I2C**: SDA(26), SCL(27)
- **Power**: PWDN(32), RESET(-1)
- **LED**: GPIO 33 (Flash LED)