#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>

// Replace with your network credentials
const char* ssid = "EEELab03";
const char* password = "EEEE1002";

// My mqtt server address
const char* mqtt_server = "192.168.8.24";

WiFiClient espClient;
PubSubClient client(espClient);

// Motor control pins
#define enA 33  // enableA command line
#define enB 25  // enableB command line
#define INa 26  // channel A direction
#define INb 27  // channel A direction
#define INc 14  // channel B direction
#define INd 12  // channel B direction

// PWM properties
const int freq = 2000;
const int ledChannela = 11;  // PWM channel for motor A
const int ledChannelb = 12;  // PWM channel for motor B
const int resolution = 8;

// Servo control
Servo steeringServo;
int steeringAngle = 90;    // variable to store the servo position
int servoPin = 13;         // the servo is attached to IO_13 on the ESP32

void setup() {
  // Initialize serial communication
  Serial.begin(115200);

  // Configure motor control pins
  pinMode(INa, OUTPUT);
  pinMode(INb, OUTPUT);
  pinMode(INc, OUTPUT);
  pinMode(INd, OUTPUT);

  // Configure PWM channels
  ledcAttachChannel(enA, freq, resolution, ledChannela);
  ledcAttachChannel(enB, freq, resolution, ledChannelb);

  // Initialize servo
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  steeringServo.setPeriodHertz(50);    // standard 50Hz servo
  steeringServo.attach(servoPin, 500, 2400);   // attaches the servo to the pin

  // Connect to Wi-Fi
  setup_wifi();

  // Set up MQTT
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void setup_wifi() {
  delay(10);
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

  // Check if the message is received on the topic "esp32/motor"
  if (String(topic) == "esp32/motor") {
    if (messageTemp == "forward") {
      Serial.println("Moving forward");
      goForwards();
      motors(120, 120);  // Set motor speed to maximum
    } else if (messageTemp == "backwards") {
      Serial.println("Moving backwards");
      goBackwards();
      motors(120, 120);  // Set motor speed to maximum
    } else if (messageTemp == "stop") {
      Serial.println("Stopping motors");
      stopMotors();
    } else if (messageTemp == "clockwise") {
      Serial.println("Moving clockwise");
      goClockwise();
      motors(255, 255);  // Set motor speed to maximum
    } else if (messageTemp == "anticlockwise") {
      Serial.println("Moving anticlockwise");
      goAntiClockwise();
      motors(255, 255);  // Set motor speed to maximum
    }
  }

  // Check if the message is received on the topic "esp32/servo"
  if (String(topic) == "esp32/servo") {
    int angle = messageTemp.toInt();
    angle = constrain(angle, 0, 180);  // Constrain the angle to valid servo range
    steeringServo.write(angle);        // Set the servo angle
    Serial.print("Setting servo angle to: ");
    Serial.println(angle);
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
      // Subscribe to the motor control topic
      client.subscribe("esp32/motor");
      // Subscribe to the servo control topic
      client.subscribe("esp32/servo");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}

// Motor control functions 
void motors(int leftSpeed, int rightSpeed) {
  leftSpeed = constrain(leftSpeed, 0, 255);
  rightSpeed = constrain(rightSpeed, 0, 255);
  ledcWrite(enA, leftSpeed);
  ledcWrite(enB, rightSpeed);
  delay(25);
}

void goForwards() {
  digitalWrite(INa, HIGH);
  digitalWrite(INb, LOW);
  digitalWrite(INc, LOW);
  digitalWrite(INd, HIGH);
}

void goBackwards() {
  digitalWrite(INa, LOW);
  digitalWrite(INb, HIGH);
  digitalWrite(INc, HIGH);
  digitalWrite(INd, LOW);
}

void goClockwise() {
  digitalWrite(INa, HIGH);
  digitalWrite(INb, LOW);
  digitalWrite(INc, HIGH);
  digitalWrite(INd, LOW);
}

void goAntiClockwise() {
  digitalWrite(INa, LOW);
  digitalWrite(INb, HIGH);
  digitalWrite(INc, LOW);
  digitalWrite(INd, HIGH);
}

void stopMotors() {
  digitalWrite(INa, LOW);
  digitalWrite(INb, LOW);
  digitalWrite(INc, LOW);
  digitalWrite(INd, LOW);
}