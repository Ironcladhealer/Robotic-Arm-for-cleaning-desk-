#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>

// --- Configuration ---
const char* ssid = "Flat_5";          // Replace with your Wi-Fi SSID
const char* password = "12345678";  // Replace with your Wi-Fi password
const char* serverIp = "192.168.0.103:5000"; // e.g., "192.168.1.10"
const int serverPort = 5000;
const char* serverRoute = "/upload";         // The route on the Flask server
const long captureInterval = 10000;          // Capture every 10,000 milliseconds (10 seconds)

// Camera pin definitions (standard for AI-Thinker ESP32-CAM)
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

unsigned long previousMillis = 0;

void startCamera() {
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
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 10000000;
    config.pixel_format = PIXFORMAT_JPEG; 
    
    // Set low resolution for faster transfer
    config.frame_size = FRAMESIZE_QVGA; // 320x240
    config.jpeg_quality = 20; // Lower quality = smaller file size
    config.fb_count = 1;

    // Camera init
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x", err);
        return;
    }
    else{
      Serial.print("Camera detected!");
    }
}

void connectToWiFi() {
    Serial.print("Connecting to WiFi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.print("WiFi connected, IP address: ");
    Serial.println(WiFi.localIP());
}

void setup() {
    Serial.begin(115200);
    startCamera();
    connectToWiFi();
}

void loop() {
    unsigned long currentMillis = millis();

    // Check if 10 seconds have passed
    if (currentMillis - previousMillis >= captureInterval) {
        previousMillis = currentMillis;
        
        // --- NEW Workaround: Clear buffer by grabbing and immediately returning ---
        camera_fb_t * dummy_fb = esp_camera_fb_get();
        if (dummy_fb) {
            esp_camera_fb_return(dummy_fb);
        }
        // ------------------------------------------------------------------------
        
        // 1. Capture Image (This is the second, fresh grab)
        camera_fb_t * fb = esp_camera_fb_get();
        if (!fb) {
            Serial.println("Camera capture failed");
            return;
        }

        // 2. Send Image to Flask Server
        HTTPClient http;
        String serverUrl = "http://" + String(serverIp) + ":" + String(serverPort) + String(serverRoute);
        http.begin(serverUrl);
        http.addHeader("Content-Type", "image/jpeg"); // Inform server about the content type

        int httpResponseCode = http.POST(fb->buf, fb->len);

        if (httpResponseCode > 0) {
            Serial.printf("Image sent successfully. HTTP response code: %d\n", httpResponseCode);
        } else {
            Serial.printf("HTTP POST failed. Error: %s\n", http.errorToString(httpResponseCode).c_str());
        }

        http.end();

        // 3. Release frame buffer
        esp_camera_fb_return(fb);
    }
}
