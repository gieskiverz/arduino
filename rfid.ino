#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <SD.h>
#include <time.h>
#include "BluetoothSerial.h"

#define RST_PIN     0
#define SS_PIN_RC522 15
#define SS_PIN_SD   5
#define BEEP16_PIN 16
#define BEEP17_PIN 17
#define BEEP4_PIN 4

#define USE_PIN // Uncomment this to use PIN during pairing. The pin is specified on the line below
const char *pin = "1234"; // Change this to more secure PIN.

String device_name = "SUKASAKU";

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#if !defined(CONFIG_BT_SPP_ENABLED)
#error Serial Bluetooth not available or not enabled. It is only available for the ESP32 chip.
#endif

BluetoothSerial SerialBT;

MFRC522 mfrc522(SS_PIN_RC522, RST_PIN);
File dataFile;

// const char* ssid = "eggs";
// const char* password = "egi123456";
// const char* apiEndpoint = "http://192.168.126.222/rfid/api.php";

String ssid;
String password;
String apiEndpoint;

void ReadConfig() {
  // Open the config.txt file for reading:
  File configFile = SD.open("/config.txt");
  if (configFile) {
    Serial.println("Reading config.txt");
    while (configFile.available()) {
      String line = configFile.readStringUntil('\n');
      int separatorIndex = line.indexOf(':');
      if (separatorIndex != -1) {
        String key = line.substring(0, separatorIndex);
        String value = line.substring(separatorIndex + 1);
        if (key == "ssid") {
          ssid = value;
        } else if (key == "password") {
          password = value;
        } else if (key == "apiEndpoint") {
          apiEndpoint = value;
        }
      }
    }
    configFile.close(); // Close the config.txt file
  } else {
    Serial.println("Error opening config.txt");
  }
}

void ReadFile(const char * path){
  // open the file for reading:
  dataFile = SD.open(path);
  if (dataFile) {
     Serial.printf("Reading file from %s\n", path);
     // read from the file until there's nothing else in it:
    while (dataFile.available()) {
      Serial.write(dataFile.read());
    }
    dataFile.close(); // close the file:
  } 
  else {
    // if the file didn't open, print an error:
    Serial.println("error opening test.txt");
  }
}

void setup() {
  Serial.begin(9600);
  SerialBT.begin(device_name); //Bluetooth device name
  Serial.printf("The device with name \"%s\" is started.\nNow you can pair it with Bluetooth!\n", device_name.c_str());
  // Serial.printf("The device with name \"%s\" and MAC address %s is started.\nNow you can pair it with Bluetooth!\n", device_name.c_str(), SerialBT.getMacString()); // Use this after the MAC method is implemented
  #ifdef USE_PIN
    SerialBT.setPin(pin);
    Serial.println("Using PIN");
  #endif
  SPI.begin(); 
  mfrc522.PCD_Init();
  
  if (SD.begin(SS_PIN_SD)) {
    Serial.println("SD card initialized successfully!");
  } else {
    Serial.println("Error initializing SD card!");
  }

  ReadConfig();
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("Password: ");
  Serial.println(password);
  Serial.print("API Endpoint: ");
  Serial.println(apiEndpoint);
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
  pinMode(BEEP16_PIN, OUTPUT);
  pinMode(BEEP17_PIN, OUTPUT);
  pinMode(BEEP4_PIN, OUTPUT);
}

void loop() {
  if (SerialBT.available()) {
    Serial.write(SerialBT.read());
  }
  delay(20);
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    activateBuzzer(BEEP4_PIN);
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
        activateBuzzer(BEEP4_PIN);
        delay(10);
        Serial.println("UID and timestamp sent to API successfully.");
        Serial.print("UID: ");
        Serial.println(content);
        Serial.print("Timestamp: ");
        Serial.println(timestamp);
      } else {
        activateBuzzer(BEEP4_PIN);
        delay(10);
        activateBuzzer(BEEP4_PIN);
        delay(10);
        dataFile = SD.open("/test.txt", FILE_APPEND);
        if (dataFile) {
          dataFile.println("uid:" + content + ";timestamp:" + timestamp + ";");
          // dataFile.println("timestamp: " + timestamp);
          dataFile.close();
          Serial.println("Data written to SD card.");
        } else {
          Serial.println("Error opening data.txt on SD card.");
        }
        Serial.println("API Offline data saved to micro SD.");
      }
    } else {
      Serial.println("Wi-Fi not connected. Cannot send UID and timestamp to API.");
    }

    // ReadFile("/test.txt");
    delay(500);
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

void activateBuzzer(char pin) {
  digitalWrite(pin, HIGH);  
  delay(100);                   
  digitalWrite(pin, LOW); 
}
