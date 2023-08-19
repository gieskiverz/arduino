#include <WiFi.h>

const char* ssid = "ajajwjwjaj";       // Your Wi-Fi SSID
const char* password = "egi123456"; // Your Wi-Fi password

void setup() {
  Serial.begin(9600);
  delay(1000);

  // Connect to Wi-Fi
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  // Print initial connection status
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to Wi-Fi");
  } else {
    Serial.println("Not connected to Wi-Fi");
  }
}

void loop() {
  // Check Wi-Fi connection status
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi disconnected. Reconnecting...");
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.println("Reconnecting to WiFi...");
    }

    // Print reconnection status
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Reconnected to Wi-Fi");
    } else {
      Serial.println("Still not connected to Wi-Fi");
    }
  }

  // Your main code here
}