#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h> 

// --- Servo Driver Libraries ---
#include <Wire.h> // For I2C communication
#include <Adafruit_PWMServoDriver.h> // PCA9685 driver library


// --- Configuration ---
const char* ssid = "Faculty";
const char* password = "CEME@4460";

// Networking
const char* serverIp = "172.17.67.119";
const int serverPort = 5000;
const char* serverRoute = "/upload";
const long captureInterval = 20000;

const char* logRoute = "/log"; // New route for debugging

// NEW: Servo Control Constants
// Note: We are using GPIO 12/13 for I2C since 21/22 were unavailable on your board.
#define I2C_SDA_PIN 12 
#define I2C_SCL_PIN 13 

// Servo PWM boundaries (Adjust these based on your SG90 servo limits!)
#define SERVOMIN  150  // Minimum pulse length (usually 0 degrees)
#define SERVOMAX  600  // Maximum pulse length (usually 180 degrees)

// Camera pin definitions (standard for AI-Thinker ESP32-CAM)
// ... (Your standard pin definitions are kept here) ...
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

unsigned long previousMillis = 0;
bool should_capture = true;
bool arm_moved_to_target = false; // NEW: Flag to track if the arm has moved yet

// PCA9685 Setup
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();


// Function to convert degrees (0-180) to PWM pulse width
uint16_t setServoPulse(int angle) {
  // Linearly map the angle to the pulse width range
  return map(angle, 0, 180, SERVOMIN, SERVOMAX);
}

void logToNetwork(const char* message) {
    if (WiFi.status() != WL_CONNECTED) {
        // If Wi-Fi is down, can't log.
        return; 
    }
    
    HTTPClient http;
    String serverUrl = "http://" + String(serverIp) + ":" + String(serverPort) + String(logRoute);
    
    http.begin(serverUrl);
    http.addHeader("Content-Type", "text/plain");
    
    int httpResponseCode = http.POST((uint8_t*)message, strlen(message));

    if (httpResponseCode <= 0) {
        // Optional: Print to serial only if wired (or ignore if fully deployed)
        // Serial.printf("Log post failed: %s\n", http.errorToString(httpResponseCode).c_str());
    }
    http.end();
}

void setArmAngles(int s1, int s2, int s3, int s4) {
  // S1 (Base) is Channel 0
  pwm.setPWM(0, 0, setServoPulse(s1)); 
  
  // S2 (Shoulder Z) is Channel 1
  pwm.setPWM(1, 0, setServoPulse(s2));
  
  // S3 (Elbow X) is Channel 2
  pwm.setPWM(2, 0, setServoPulse(s3));
  
  // S4 (Gripper) is Channel 3
  pwm.setPWM(3, 0, setServoPulse(s4));
}

void startCamera() {
    // ... (Existing startCamera code remains unchanged) ...
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
    // ... (Existing connectToWiFi code remains unchanged) ...
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
    
    // Initialize I2C bus with remapped pins
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN); 
    
    // Initialize PWM driver
    pwm.begin();
    pwm.setPWMFreq(50); // Standard servo frequency is 50 Hz

    startCamera();
    connectToWiFi();
    
    // Move arm to a safe starting position (e.g., all 90 degrees)
    setArmAngles(90, 90, 90, 10); // 10 degrees = Gripper Open
    Serial.println("Arm initialized to safe position.");
    
    logToNetwork("ESP32 Initializing...");
}

void performPickupSequence(int s1, int s2, int s3) {
    // Sequence 1: Move to the target position with the gripper open (S4=10)
    Serial.println("Starting pickup sequence...");
    setArmAngles(s1, s2, s3, 10);
    delay(2000); // Give time for arm to move

    // Sequence 2: Close the gripper (S4=70)
    Serial.println("Closing gripper...");
    setArmAngles(s1, s2, s3, 70); 
    delay(1000); 

    // Sequence 3: Lift the arm (Move S2 and S3 back to a higher position, e.g., home position 90, 90)
    Serial.println("Lifting garbage...");
    setArmAngles(s1, 90, 90, 70); // Lift up
    delay(2000);

    // Sequence 4: Rotate base to drop-off point (e.g., 0 degrees)
    Serial.println("Moving to bin...");
    setArmAngles(0, 90, 90, 70); // Rotate base to bin
    delay(2000);

    // Sequence 5: Drop the garbage (S4=10)
    Serial.println("Dropping garbage...");
    setArmAngles(0, 90, 90, 10); 
    delay(1000);

    // Sequence 6: Return to safe position and resume capture logic
    Serial.println("Sequence complete. Returning to home.");
    setArmAngles(90, 90, 90, 10); 
    delay(2000);
}


void loop() {
    
    // State 1: Capture Mode
    if (should_capture && !arm_moved_to_target) {
        
        // ... (Existing capture logic) ...
        unsigned long currentMillis = millis();

        if (currentMillis - previousMillis >= captureInterval) {
            previousMillis = currentMillis;
            
            // Frame buffer workaround and image capture...
            camera_fb_t * dummy_fb = esp_camera_fb_get();
            if (dummy_fb) { esp_camera_fb_return(dummy_fb); }
            
            camera_fb_t * fb = esp_camera_fb_get();
            if (!fb) { Serial.println("Camera capture failed"); return; }

            // Send Image to Flask Server...
            HTTPClient http;
            String serverUrl = "http://" + String(serverIp) + ":" + String(serverPort) + String(serverRoute);
            http.begin(serverUrl);
            http.addHeader("Content-Type", "image/jpeg"); 

            int httpResponseCode = http.POST(fb->buf, fb->len);
            String responsePayload = "";

            if (httpResponseCode > 0) {
                // ... (Existing response handling) ...
                responsePayload = http.getString();
                
                // --- Process JSON Response ---
                // We need more space for the angle data now. Increased to 384 bytes.
                StaticJsonDocument<384> doc; 
                DeserializationError error = deserializeJson(doc, responsePayload);
                
                if (error) {
                    Serial.print(F("Failed to parse JSON: "));
                    Serial.println(error.f_str());
                } else {
                    bool stop_flag = doc["stop_capture"] | false; 

                    if (stop_flag) {
                        should_capture = false; // Stop capture
                        
                        // Extract Angles
                        JsonObject target_angles = doc["target_angles"];
                        int s1_base = target_angles["s1_base"] | 90; 
                        int s2_shoulder = target_angles["s2_shoulder_z"] | 90;
                        int s3_elbow = target_angles["s3_elbow_x"] | 90;
                        int s4_gripper = target_angles["s4_gripper_open"] | 10;
                        
                        Serial.println(">>> RECEIVED COMMAND. STARTING ROBOT MOVE. <<<");
                        Serial.printf("Angles: S1:%d, S2:%d, S3:%d, S4:%d\n", s1_base, s2_shoulder, s3_elbow, s4_gripper);

                        // State Transition: Move arm to the target position immediately
                        //performPickupSequence(s1_base, s2_shoulder, s3_elbow);
                        
                        // Reset the flags to start the loop over
                        should_capture = true;
                        arm_moved_to_target = false;
                        logToNetwork("Received IK command. Moving arm.");
                        // State Transition: Move arm to the target position immediately
                        performPickupSequence(s1_base, s2_shoulder, s3_elbow);
                    }
                    Serial.printf("Server Status: %s\n", doc["status"].as<const char*>());
                }

            } else {
                Serial.printf("HTTP POST failed. Error: %s\n", http.errorToString(httpResponseCode).c_str());
                logToNetwork("Capture sent, no trash found. Continuing.");
            }

            http.end();
            esp_camera_fb_return(fb);
        }
    }
    
    // State 2: Waiting/Stopped Mode
    if (!should_capture) {
        // Only executes while the arm is moving or debugging is paused
        delay(100); 
    }
}