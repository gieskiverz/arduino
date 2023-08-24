#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <SD.h>
#include <time.h>

#define RST_PIN     22
#define SS_PIN_RC522 5
#define SS_PIN_SD   15 // Use HSPI SDA (D8) as CS for SD card module

MFRC522 mfrc522(SS_PIN_RC522, RST_PIN);
File dataFile;


const char* ssid = "wakwaw";
const char* password = "egi123456";
const char* apiEndpoint = "http://192.168.126.222/rfid/api.php";

void setup() {
  
  Serial.begin(9600);
  
  SPI.begin(); 
  mfrc522.PCD_Init();

  if (SD.begin(SS_PIN_SD)) {
    Serial.println("SD card initialized successfully!");
    dataFile = SD.open("data.txt", FILE_WRITE);
  } else {
    Serial.println("Error initializing SD card!");
  }

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

    // Synchronize time with NTP server and set time zone to Asia/Jakarta
    configTime(7 * 3600, 0, "pool.ntp.org");
    while (!time(nullptr)) {
      Serial.print(".");
      delay(1000);
    }
    Serial.println("Time synchronized.");
  } else {
    Serial.println("Not connected to Wi-Fi");
  }
}

void loop() {
  // Rest of your loop code...

  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    Serial.print("UID tag: ");
    String content = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
      content.concat(String(mfrc522.uid.uidByte[i], HEX));
    }
    content.toUpperCase();
    Serial.println(content);
    
    String timestamp = getCurrentDateTime();

    if (WiFi.status() == WL_CONNECTED) {
      if (sendUIDAndTimestampToAPI(content, timestamp)) {
        Serial.println("UID and timestamp sent to API successfully.");
        Serial.print("UID: ");
        Serial.println(content);
        Serial.print("Timestamp: ");
        Serial.println(timestamp);
      } else {
        Serial.println("Failed to send UID and timestamp to API.");
      }
    } else {
      Serial.println("Wi-Fi not connected. Cannot send UID and timestamp to API.");
    }

    if (dataFile) {
      dataFile.println("Card Data: " + content);
      dataFile.println("Timestamp: " + timestamp);
      dataFile.println();
      dataFile.close();
      Serial.println("Data written to SD card.");
    } else {
      Serial.println("Error opening data.txt on SD card.");
    }

    delay(1000);
  }
}

bool sendUIDAndTimestampToAPI(String uid, String timestamp) {
  HTTPClient http;
  http.begin(apiEndpoint);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  String postData = "uid=" + uid + "&timestamp=" + timestamp;
  int httpCode = http.POST(postData);

  if (httpCode == HTTP_CODE_OK) {
    return true;
  } else {
    return false;
  }
  http.end();
}

String getCurrentDateTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "Failed to obtain time";
  }
  char formattedDateTime[20];
  strftime(formattedDateTime, sizeof(formattedDateTime), "%d-%m-%Y %H:%M:%S", &timeinfo);
  return String(formattedDateTime);
}
