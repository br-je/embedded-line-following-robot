//********************************************************//
//*  University of Nottingham                            *//
//*  Department of Electrical and Electronic Engineering *//
//*  UoN EEEBot 2023                                     *//
//*                                                      *//
//*  ESP32 MQTT Example Code                             *//
//*                                                      *//
//*  Nat Dacombe                                         *//
//********************************************************//

// the following code is modified from https://randomnerdtutorials.com by Rui Santos

#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// replace the next variables with your SSID/Password combination
const char* ssid = "EEELab03";
const char* password = "EEEE1002";                

// add your MQTT Broker IP address, example:
//const char* mqtt_server = "192.168.1.144";
const char* mqtt_server = "192.168.8.24";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];

// LED Pin
const int ledPin = 17;

// Ultrasonic sensor pins
const int trigPin = 32;
const int echoPin = 33;

// MPU6050
Adafruit_MPU6050 mpu;
float yaw = 0.0; // Yaw angle upon start
unsigned long lastTime = 0; //Last loop time tracking

void setup() {
  Serial.begin(115200);

  // Set ultrasonic sensor pins
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Initialize MPU6050
  Wire.begin(18, 19); // SDA, SCL
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050, Unlucky!");
    while (1) delay(10);
  }
  Serial.println("MPU6050 Initialized");
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  pinMode(ledPin, OUTPUT);
}

void setup_wifi() {
  delay(10);
  // we start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // if a message is received on the topic esp32/output, you check if the message is either "on" or "off". 
  // changes the output state according to the message
  if (String(topic) == "esp32/output") {
    Serial.print("Changing output to ");
    if(messageTemp == "on"){
      Serial.println("on");
      digitalWrite(ledPin, HIGH);
    }
    else if(messageTemp == "off"){
      Serial.println("off");
      digitalWrite(ledPin, LOW);
    }
  }
}

void reconnect() {
  // loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      // subscribe
      client.subscribe("esp32/output");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // wait 5 seconds before retrying
      delay(5000);
    }
  }
}

long microsecondsToCentimeters(long microseconds) {
  return microseconds / 29 / 2; // Rough approximation
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 1000) {
    lastMsg = now;

    // Distance calculations
    long duration, cm;

    // Trigger ultrasonic pulse
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    // Measure the duration of the echo
    duration = pulseIn(echoPin, HIGH);

    // Convert duration to distance
    cm = microsecondsToCentimeters(duration);

    // Print the distance
    Serial.print("Distance: ");
    Serial.print(cm);
    Serial.println(" cm");

    // Convert the distance value to a char array and publish it
    char distanceString[8];
    dtostrf(cm, 1, 2, distanceString);
    client.publish("esp32/distance", distanceString);

    // MPU6050 Yaw calculation
    sensors_event_t accel, gyro, temp;
    mpu.getEvent(&accel, &gyro, &temp);

    // Calculate time difference (deltaTime)
    unsigned long currentTime = millis();
    float deltaTime = (currentTime - lastTime) / 1000.0; // Conversion to seconds
    lastTime = currentTime;

    float rotationZ = gyro.gyro.z; // z-axis rotation in deg/s
    yaw += rotationZ * deltaTime;

    // Print the yaw angle
    Serial.print("Yaw: ");
    Serial.print(yaw);
    Serial.println(" Degree");

    // Convert the yaw value to a char array and publish it
    char yawString[8];
    dtostrf(yaw, 1, 2, yawString);
    client.publish("esp32/yaw", yawString);
  }

  delay(10);
}