#include <Wire.h>

// Photodiode sensor pins (ESP32 ADC pins)
#define MIDDLE_LEFT_PIN 14  // Middle-left sensor
#define MIDDLE_RIGHT_PIN 27 // Middle-right sensor

// Threshold for black line detection
#define BLACK_LINE_THRESHOLD 800

void setup() {
    Serial.begin(115200);

    // Initialize I2C
    Wire.begin();
    Serial.println("Master initialized and ready.");

    // Configure ADC resolution for photodiodes
    analogReadResolution(12);  // Set ADC resolution to 12 bits (0-4095)
}

void readPhotodiodeSensors(int &middleLeftReading, int &middleRightReading) {
    // Read only the middle two sensors
    middleLeftReading = analogRead(MIDDLE_LEFT_PIN);
    middleRightReading = analogRead(MIDDLE_RIGHT_PIN);
}

void sendControlToSlave(int correction) {
    Wire.beginTransmission(0x08);  // Slave address
    Wire.write('C');               // Command to adjust motors/servo
    Wire.write((int8_t)constrain(correction, -148, 147));  // Correction as scaled integer
    Wire.endTransmission();
}

void loop() {
    // Step 1: Read sensor values
    int middleLeftReading, middleRightReading;
    readPhotodiodeSensors(middleLeftReading, middleRightReading);

    // Step 2: Determine correction based on sensor readings
    int correction = 0;
    if (middleLeftReading < BLACK_LINE_THRESHOLD && middleRightReading >= BLACK_LINE_THRESHOLD) {
        // Line detected on the left, turn right
        correction = -50;  // Negative correction for turning right
    } else if (middleRightReading < BLACK_LINE_THRESHOLD && middleLeftReading >= BLACK_LINE_THRESHOLD) {
        // Line detected on the right, turn left
        correction = 50;  // Positive correction for turning left
    } else if (middleLeftReading < BLACK_LINE_THRESHOLD && middleRightReading < BLACK_LINE_THRESHOLD) {
        // Both sensors detect the black line, go straight
        correction = 0;  // No correction needed
    }

    // Step 3: Send correction to the slave
    sendControlToSlave(correction);

    // Debugging output
    Serial.print("Middle-Left: ");
    Serial.print(middleLeftReading);
    Serial.print(" | Middle-Right: ");
    Serial.print(middleRightReading);
    Serial.print(" | Correction: ");
    Serial.println(correction);

    delay(50);  // Small delay for stability
}