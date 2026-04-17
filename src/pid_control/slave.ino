#include <ESP32Servo.h>
#include <Wire.h>

Servo steeringServo;

// Motor pins
#define enA 33
#define enB 25
#define INa 26
#define INb 27
#define INc 14
#define INd 12

const int freq = 2000;
const int ledChannela = 1; // Original PWM channel for left motor
const int ledChannelb = 2; // Original PWM channel for right motor
const int resolution = 8;

// Servo parameters
#define SERVO_PIN 13
#define SERVO_CENTER 90         // Centre position for servo
#define MAX_SERVO_CORRECTION 90 // Maximum correction for steering
#define MIN_SERVO_ANGLE 0      // Minimum servo angle
#define MAX_SERVO_ANGLE 180    // Maximum servo angle

// Motor speed limits
#define BASE_SPEED 150
#define MAX_SPEED 255
#define MIN_SPEED 0

void setup() {
    Serial.begin(115200);
    Wire.begin(0x08); // Initialize as slave with address 8
    Wire.onReceive(receiveEvent);

    // Attach PWM for motors (original setup)
    ledcAttachChannel(enA, freq, resolution, ledChannela);
    ledcAttachChannel(enB, freq, resolution, ledChannelb);

    // Motor pin setup
    pinMode(INa, OUTPUT);
    pinMode(INb, OUTPUT);
    pinMode(INc, OUTPUT);
    pinMode(INd, OUTPUT);

    // Servo setup
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);
    steeringServo.setPeriodHertz(50);
    steeringServo.attach(SERVO_PIN, 1000, 2000); // Adjusted pulse width range
    steeringServo.write(SERVO_CENTER);          // Centre the servo

    Serial.println("Slave initialized and ready.");
}

void receiveEvent(int bytes) {
    if (Wire.available()) {
        char command = Wire.read(); // Read the command

        if (command == 'C') { // Correction command
            if (Wire.available()) {
                int8_t correction = Wire.read();
                handleCorrection(correction);
            }
        }
    }
}

void handleCorrection(int8_t correction) {
    // **Servo Movement**
    // Map correction to servo angle range
    int servoAdjustment = map(correction, -148, 147, -MAX_SERVO_CORRECTION, MAX_SERVO_CORRECTION);
    int servoAngle = constrain(SERVO_CENTER + servoAdjustment, MIN_SERVO_ANGLE, MAX_SERVO_ANGLE);
    steeringServo.write(servoAngle); // Adjust servo angle

    // **Motor Speed Adjustment**
    int leftSpeed = BASE_SPEED;
    int rightSpeed = BASE_SPEED;

    // Constrain motor speeds
    leftSpeed = constrain(leftSpeed, MIN_SPEED, MAX_SPEED);
    rightSpeed = constrain(rightSpeed, MIN_SPEED, MAX_SPEED);

    // Set motor directions and speeds
    digitalWrite(INa, HIGH);  // Left motor forward
    digitalWrite(INb, LOW);
    ledcWrite(enA, leftSpeed);

    digitalWrite(INc, LOW);  // Right motor forward
    digitalWrite(INd, HIGH);
    ledcWrite(enB, rightSpeed);

    // Debugging: Output correction, servo angle, and motor speeds
    Serial.print("Correction: ");
    Serial.print(correction);
    Serial.print(" | Servo Angle: ");
    Serial.print(servoAngle);
    Serial.print(" | Left Speed: ");
    Serial.print(leftSpeed);
    Serial.print(" | Right Speed: ");
    Serial.println(rightSpeed);
}

void loop() {
    delay(10); // All operations handled in receiveEvent
}