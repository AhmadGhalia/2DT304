#include <Arduino.h>
#include <SoftwareSerial.h>

SoftwareSerial esp8266(3, 6);  // TX, RX

// WiFi and Server Configuration
const char* ssid = "BackupNetwork";
const char* password = "12345678";
const char* serverIP = "172.20.10.2";
const String endpoint = "/data";
const int port = 80;

// Sensor Configuration
const int firstSensorPin = 2;
const int secondSensorPin = 9;
int lastFirstSensorState = LOW;
int lastSecondSensorState = LOW;
volatile int antalPeople = 0;
volatile int enteringNumber = 0;
volatile int outingNumber = 0;
unsigned long firstSensorTime = 0;
unsigned long secondSensorTime = 0;
const unsigned long detectionDelay = 5000;  // Delay time in milliseconds

void setup() {
  Serial.begin(115200);
  esp8266.begin(115200);
  pinMode(firstSensorPin, INPUT);
  pinMode(secondSensorPin, INPUT);
  setupWiFi();  // Connect to Wi-Fi
}

void loop() {
  int firstSensorValue = digitalRead(firstSensorPin);
  int secondSensorValue = digitalRead(secondSensorPin);
  unsigned long currentTime = millis();
  static int lastPeopleCount = antalPeople;  // Keep track of the last people count

  // First sensor detection logic
  if (firstSensorValue == HIGH && lastFirstSensorState == LOW) {
    firstSensorTime = currentTime;  // Save the detection time
  }

  // Second sensor detection logic
  if (secondSensorValue == HIGH && lastSecondSensorState == LOW) {
    secondSensorTime = currentTime;  // Save the detection time
  }

  // Check if the first sensor was triggered followed by the second within the delay period
  if (firstSensorTime > 0 && (currentTime - firstSensorTime < detectionDelay) && secondSensorTime > firstSensorTime && (currentTime - secondSensorTime < detectionDelay)) {
    antalPeople++;  // Increase count
    enteringNumber=1;
     outingNumber=0;
    firstSensorTime = 0;
    secondSensorTime = 0;
  }

  // Check if the second sensor was triggered followed by the first within the delay period
  if (secondSensorTime > 0 && (currentTime - secondSensorTime < detectionDelay) && firstSensorTime > secondSensorTime && (currentTime - firstSensorTime < detectionDelay)) {
    antalPeople--;  // Decrease count
    outingNumber=1;
    enteringNumber=0;
    firstSensorTime = 0;
    secondSensorTime = 0;
  }

  // Update the last states
  lastFirstSensorState = firstSensorValue;
  lastSecondSensorState = secondSensorValue;

  // If antalPeople has changed, post the new count
  if (antalPeople != lastPeopleCount) {
    String data = "{\"CurrentPeopleIn\":" + String(antalPeople) + ", \"EnteringNumber\":" + String(enteringNumber) + ", \"OutingNumber\":" + String(outingNumber) + "}";
    if (postJSONData(data)) {
      Serial.println("People count posted successfully");
      outingNumber = 0;
      enteringNumber = 0;
    } else {
      Serial.println("Failed to post people count");
    }
    lastPeopleCount = antalPeople;  // Update the last known count
  }

  delay(50);  // Short delay to avoid flooding and allow for sensor debounce
}

void setupWiFi() {
  Serial.println("Attempting to connect to WiFi...");
  int maxAttempts = 5;  // Maximum number of connection attempts
  int attempt = 0;      // Current attempt number

  while (attempt < maxAttempts) {
    esp8266.println("AT+CWJAP=\"" + String(ssid) + "\",\"" + String(password) + "\"");

    if (waitForResponse("WIFI CONNECTED", 6000)) {  // Wait for Wi-Fi connection
      Serial.println("Connected to WiFi");
      return;  // Exit the function if connected successfully
    } else {
      Serial.println("Failed to connect to WiFi. Retrying...");
    }
    attempt++;
    delay(2000);  // Wait for a brief moment before retrying
  }

  Serial.println("Exceeded maximum connection attempts. Check your SSID and password.");
}

bool postJSONData(String data) {
  int maxRetries = 8;  // Maximum number of retries
  int attempt = 0;     // Current attempt number

  while (attempt < maxRetries) {
    esp8266.println("AT+CIPSTART=\"TCP\",\"" + String(serverIP) + "\"," + String(port));
    if (!waitForResponse("CONNECT", 8000)) {
      Serial.println("Failed to connect to server");
      attempt++;
      continue;  // Skip the rest of the loop and try again
    }

    String httpRequest = "POST " + endpoint + " HTTP/1.1\r\n" + "Host: " + serverIP + "\r\n" + "Content-Type: application/json\r\n" + "Content-Length: " + data.length() + "\r\n\r\n" + data + "\r\n";

    esp8266.println("AT+CIPSEND=" + String(httpRequest.length()));
    if (waitForResponse(">", 4000)) {  // Wait for ESP8266 to be ready to receive the data
      esp8266.println(httpRequest);
      if (waitForResponse("SEND OK", 9000)) {
        Serial.println("Data sent successfully");
        esp8266.println("AT+CIPCLOSE");  // Close the connection
        return true;
      }
    }
    Serial.println("Failed to send data, attempt #" + String(attempt + 1));
    esp8266.println("AT+CIPCLOSE");  // Close the connection even if data wasn't sent successfully
    attempt++;
  }
  return false;  // If we reach here, all attempts failed
}


bool waitForResponse(String successResponse, unsigned long timeout) {
  unsigned long startTime = millis();
  String receivedData = "";
  while (millis() - startTime < timeout) {
    if (esp8266.available()) {
      char c = esp8266.read();
      receivedData += c;
      if (receivedData.indexOf(successResponse) != -1) {
        return true;
      }
    }
  }
  return false;
}
