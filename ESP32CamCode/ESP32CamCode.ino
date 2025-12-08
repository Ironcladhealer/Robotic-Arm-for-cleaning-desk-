#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h> // <-- REQUIRED: Install this library via Library Manager

// --- Configuration ---
const char* ssid = "Flat_5";          // Your Wi-Fi SSID
const char* password = "12345678";    // Your Wi-Fi password

// NOTE: The IP and Port should be separated for proper HTTPClient URL construction
// The Flask server is listening at 192.168.0.103 on port 5000.
const char* serverIp = "192.168.0.103"; // Host IP address ONLY
const int serverPort = 5000;            // Port number ONLY
const char* serverRoute = "/upload";
const long captureInterval = 20000;    // Capture every 20,000 milliseconds (20 seconds)

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
bool should_capture = true; // NEW: Control flag for the capture loop

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
    
    // Increased stability settings
    config.xclk_freq_hz = 10000000;
    config.pixel_format = PIXFORMAT_JPEG; 
    config.frame_size = FRAMESIZE_QVGA; 
    config.jpeg_quality = 20; 
    config.fb_count = 1;

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x\n", err);
        return;
    } else {
        Serial.println("Camera detected!");
    }
}

void connectToWiFi() {
    Serial.print("Connecting to WiFi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
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

    // Check if we should still be capturing AND if the interval has passed
    if (should_capture && (currentMillis - previousMillis >= captureInterval)) {
        previousMillis = currentMillis;
        
        // --- Frame Buffer Workaround ---
        camera_fb_t * dummy_fb = esp_camera_fb_get();
        if (dummy_fb) {
            esp_camera_fb_return(dummy_fb);
        }
        
        // 1. Capture Image
        camera_fb_t * fb = esp_camera_fb_get();
        if (!fb) {
            Serial.println("Camera capture failed");
            return;
        }

        // 2. Send Image to Flask Server
        HTTPClient http;
        // Construct the URL: http://IP:PORT/ROUTE
        String serverUrl = "http://" + String(serverIp) + ":" + String(serverPort) + String(serverRoute);
        http.begin(serverUrl);
        http.addHeader("Content-Type", "image/jpeg"); 

        int httpResponseCode = http.POST(fb->buf, fb->len);
        String responsePayload = "";

        if (httpResponseCode > 0) {
            Serial.printf("Image sent. HTTP response code: %d\n", httpResponseCode);
            responsePayload = http.getString();
            
            // --- 3. Process JSON Response for Stop Signal ---
            // Allocate space for the JSON document
            StaticJsonDocument<256> doc; 
            DeserializationError error = deserializeJson(doc, responsePayload);

            if (error) {
                Serial.print(F("Failed to parse JSON: "));
                Serial.println(error.f_str());
            } else {
                // Check the 'stop_capture' key sent by the Flask server
                bool stop_flag = doc["stop_capture"] | false; 
                String status = doc["status"] | "UNKNOWN";

                if (stop_flag) {
                    should_capture = false; // Stop the loop condition!
                    Serial.println(">>> RECEIVED STOP SIGNAL (TRASH DETECTED). HALTING CAPTURE. <<<");
                }
                Serial.printf("Server Status: %s\n", status.c_str());
            }

        } else {
            Serial.printf("HTTP POST failed. Error: %s\n", http.errorToString(httpResponseCode).c_str());
        }

        http.end();
        esp_camera_fb_return(fb);
    }
    
    if (!should_capture) {
        // If capture is halted, print status every few seconds to confirm it's stopped.
        Serial.println("Capture Halted. Waiting for manual reset...");
        delay(5000); 
    }
}