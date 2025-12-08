#include "esp_camera.h"
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);

// Servo channels (same as Arduino code)
#define servo1 0
#define servo2 1
#define servo3 2
#define servo4 3

// Helper function to move one servo smoothly
void moveServo(int channel, int start, int end, int delayTime) {
  if (start < end) {
    for (int pos = start; pos <= end; pos++) {
      pwm.setPWM(channel, 0, pos);
      delay(delayTime);
    }
  } else {
    for (int pos = start; pos >= end; pos--) {
      pwm.setPWM(channel, 0, pos);
      delay(delayTime);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Initialize I2C for ESP32-CAM
  Wire.begin(15, 14);  // SDA = 15, SCL = 14

  pwm.begin();
  pwm.setPWMFreq(60);

  // Initial positions
  pwm.setPWM(servo1, 0, 330);
  pwm.setPWM(servo2, 0, 150);
  pwm.setPWM(servo3, 0, 300);
  pwm.setPWM(servo4, 0, 410);
  
  delay(2000);
}

void loop() {

  // SAME MOTION SEQUENCE AS YOUR ARDUINO CODE  
  moveServo(servo1, 330, 250, 10);
  moveServo(servo2, 150, 380, 10);
  moveServo(servo3, 300, 380, 10);
  moveServo(servo4, 410, 510, 10);

  delay(2000);

  moveServo(servo4, 510, 410, 10);
  moveServo(servo3, 380, 300, 10);
  moveServo(servo2, 380, 150, 10);
  moveServo(servo1, 250, 450, 10);

  moveServo(servo2, 150, 380, 10);
  moveServo(servo3, 300, 380, 10);
  moveServo(servo4, 410, 510, 10);

  moveServo(servo4, 510, 410, 10);
  moveServo(servo3, 380, 300, 10);
  moveServo(servo2, 380, 150, 10);
  moveServo(servo1, 450, 330, 10);
}