#include <Arduino.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>
#include <SD.h>
#include <time.h>
#include <ArduinoJson.h>

#define TRIGGER_PIN 0
#define RST_PIN     22
#define SS_PIN_RC522 5
#define SS_PIN_SD   15 // Use HSPI SDA (D8) as CS for SD card module

MFRC522 mfrc522(SS_PIN_RC522, RST_PIN);
File dataFile;

WiFiManager wm; // global wm instance
WiFiManagerParameter custom_field; // global param ( for non blocking w params )

String ssid = "";
String password = "";
String apiEndpoint = "";
String newApi = "";
bool configChanged = false;

const char* fileName = "/data.json";
StaticJsonDocument<1024> jsonConfig;
JsonArray dataArray = jsonConfig.createNestedArray();

void setup() {
  // WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP  
  // put your setup code here, to run once:
  Serial.begin(9600);
  SPI.begin(); 
  mfrc522.PCD_Init();
  Serial.println("\n Starting");
  pinMode(TRIGGER_PIN, INPUT);
  pinMode(4, OUTPUT);
  
  Serial.println();
  if (SD.begin(SS_PIN_SD)) {
    dataFile = SD.open("data.txt", FILE_WRITE);
    Serial.println("SD card initialized successfully!");
  } else {
    Serial.println("Error initializing SD card!");
  }

  // Connect to Wi-Fi
  Serial.println("Connecting to Wi-Fi...");

  bool res;
  // res = wm.autoConnect(); // auto generated AP name from chipid
  // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
  konfigurasi();
  res = wm.autoConnect("SUKASAKU","sukusuku"); // password protected ap

  if(!res) {
      Serial.println("Failed to connect");
      // ESP.restart();
  } 
  else {
      configTime(7 * 3600, 0, "pool.ntp.org");
      while (!time(nullptr)) {
        Serial.print(".");
        delay(1000);
      }
      // writeConfigToSD("/config.json", WiFi.SSID(), WiFi.psk(), apiEndpoint);
      Serial.println("Time synchronized."); 
      Serial.println("connected...setup yeey :) ");
  }
}

void konfigurasi(){
  const char* menu[] = {"wifi","info","sep","restart","exit"}; 
  wm.setMenu(menu,6);
  wm.setClass("invert");
  new (&custom_field) WiFiManagerParameter("api","host/domain","",50,"placeholder=\"ex: https://sukasaku/logs\"");
  wm.addParameter(&custom_field);
  wm.setSaveParamsCallback(saveParamCallback);
  wm.setConfigPortalTimeout(240);
}
void checkButton(){
  if( digitalRead(TRIGGER_PIN) == LOW ){
    Serial.println("Erasing Config, restarting");
    wm.resetSettings();
    delay(1000);
    if (!wm.startConfigPortal("SUKASAKU","sukusuku")) {
      Serial.println("failed to connect or hit timeout");
      delay(3000);
      ESP.restart();
    } else {
      // configChanged = true;
      //if you get here you have connected to the WiFi
      Serial.println("connected...yeey :)");
    }
  }
}

String getParam(String name){
  //read parameter from server, for customhmtl input
  String value;
  if(wm.server->hasArg(name)) {
    value = wm.server->arg(name);
  }
  apiEndpoint = value;
  writeConfigToSD("/config.json",value);
  return value;
}

void saveParamCallback(){
  Serial.println("[CALLBACK] saveParamCallback fired");
  Serial.println("PARAM api = " + getParam("api"));
}

bool checkInternet(){
    HTTPClient http;
    http.begin("http://www.google.com");
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
      return true;
    } else {
      return false;
    }
    http.end();
}
bool sendUIDAndTimestampToAPI(String api, String uid, String timestamp) {
  HTTPClient http;
  http.begin(api);
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

void WriteFile(const char * path, const char * message){
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  dataFile = SD.open(path, FILE_WRITE);
  // if the file opened okay, write to it:
  if (dataFile) {
    Serial.printf("Writing to %s ", path);
    dataFile.println(message);
    dataFile.close(); // close the file:
    Serial.println("completed.");
  } 
  // if the file didn't open, print an error:
  else {
    Serial.println("error opening file ");
    Serial.println(path);
  }
}

void AppendFile(const char * path, const char * message){
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  dataFile = SD.open(path, FILE_APPEND);
  // if the file opened okay, write to it:
  if (dataFile) {
    Serial.printf("Writing to %s ", path);
    dataFile.println(message);
    dataFile.close(); // close the file:
    Serial.println("completed.");
  } 
  // if the file didn't open, print an error:
  else {
    Serial.println("error opening file ");
    Serial.println(path);
  }
}

bool readConfigFromSD(const char* path) {
  File configFile = SD.open(path, FILE_READ);
  if (!configFile) {
    return false;
  }

  // Buat objek JSON untuk menyimpan konfigurasi
  StaticJsonDocument<256> jsonConfig;
  DeserializationError error = deserializeJson(jsonConfig, configFile);
  configFile.close();

  if (error) {
    Serial.println("Failed to read config file");
    return false;
  }

  // Baca nilai konfigurasi dari JSON
  // ssid = jsonConfig["ssid"].as<String>();
  // password = jsonConfig["password"].as<String>();
  // apiEndpoint = jsonConfig["apiEndpoint"].as<String>();
  newApi = jsonConfig["apiEndpoint"].as<String>();
  
  Serial.println("read config file");
  serializeJsonPretty(jsonConfig, Serial);
  Serial.println();

  return true;
}

// Fungsi untuk menulis konfigurasi ke file JSON di SD card
bool writeConfigToSD(const char* path, String a) {
  File configFile = SD.open(path, FILE_WRITE);
  if (!configFile) {
    return false;
  }

  // Buat objek JSON untuk menyimpan konfigurasi
  StaticJsonDocument<256> jsonConfig;

  // Tambahkan nilai konfigurasi ke JSON
  jsonConfig["apiEndpoint"] = a;

  // Serialize JSON ke file
  if (serializeJson(jsonConfig, configFile) == 0) {
    Serial.println("Failed to write config file");
    return false;
  }

  configFile.close();
  return true;
}

//Fungsi untuk simpan UID offline
bool writeUID(const char* path, String a, String b) {
  Serial.println(String(path));
  Serial.println(a);
  Serial.println(b);
  File writeFile = SD.open(path, FILE_WRITE);
  if (!writeFile) {
    Serial.println("failed writefile");
    return false;
  }
  // jsonData.clear(); // Clear previous data
  // JsonObject obj = dataArray.createNestedObject();
  // obj["uid"] = a;
  // obj["timestamp"] = b; 
  // String jsonStr;
  // serializeJson(jsonData, jsonStr);
  // Serial.println(jsonStr);
  // // Serialize JSON ke file
  // if (serializeJsonPretty(jsonData, writeFile) == 0) {
  //   Serial.println("Failed to write data file");
  //   return false;
  // }

  // Buat objek JSON untuk menyimpan konfigurasi
  JsonObject obj = dataArray.createNestedObject();

  // Tambahkan nilai konfigurasi ke JSON
  obj["uid"] = a;
  obj["timestamp"] = b;

  // Serialize JSON ke file
  if (serializeJson(jsonConfig, writeFile) == 0) {
    Serial.println("Failed to write config file");
    return false;
  }

  writeFile.close();
  return true;
}

//Fungsi untuk simpan UID offline
bool readUID(const char* path) {
  File configFile = SD.open(path, FILE_READ);
  if (!configFile) {
    return false;
  }

  StaticJsonDocument<1024> jsonData;
  // Buat objek JSON untuk menyimpan konfigurasi
  DeserializationError error = deserializeJson(jsonData, configFile);
  configFile.close();

  if (error) {
    Serial.println("Failed to read config file");
    return false;
  }
  int jumlah = jsonData[0].size();
  // if(jumlah>0){
  //   for (JsonObject obj : jsonData[0]) {
  //     // Ambil nilai uid dan timestamp dari objek JSON
  //     String uid = obj["uid"].as<String>();
  //     String timestamp = obj["timestamp"].as<String>();

  //     // Kirim data ke server
  //     sendUIDAndTimestampToAPI(newApi, timestamp);

  //     delay(1000); // Tunggu 1 detik sebelum mengirim objek berikutnya
  //   }
  // }
  Serial.println("read UID file");
  serializeJsonPretty(jsonData[0], Serial);
  Serial.println();
  Serial.println("Jumlah array : ");
  Serial.println(jumlah);

  return true;
}

void removeData(){

}

void loop() {
  // saveParamCallback();
  // Serial.println("bagong");
  // Serial.println(WiFi.SSID());
  // Serial.println(WiFi.psk());
  // Serial.println(String(apiEndpoint));
  digitalWrite(4,LOW);
  readConfigFromSD("/config.json");
  Serial.println(newApi);
  readUID("/data.json");
  // ReadFile("/data.json");
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    digitalWrite(4,HIGH);
    delay(300);
    digitalWrite(4,LOW);
    delay(100);
    Serial.print("UID tag: ");
    String content = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : ""));
      content.concat(String(mfrc522.uid.uidByte[i], HEX));
    }
    content.toUpperCase();
    Serial.println(content);
    
    String timestamp = getCurrentDateTime();

    if (checkInternet()) {
      Serial.print("Checking internet .....");
      Serial.print(checkInternet());
      delay(500);
      if (sendUIDAndTimestampToAPI(newApi, content, timestamp)) {
        Serial.println("UID and timestamp sent to API successfully.");
        Serial.print("UID: ");
        Serial.println(content);
        Serial.print("Timestamp: ");
        Serial.println(timestamp);
      } else {
        Serial.println("Failed to send UID and timestamp to API.");
        // Membuat pesan menggunakan String
        // String message = "Card Data: " + String(content) + " Timestamp: " + String(timestamp);

        // // Konversi String menjadi const char*
        // const char* messageChar = message.c_str();
        // WriteFile("/data.txt",messageChar);
        writeUID(fileName,String(content),String(timestamp));
        // addNewData(String(content),String(timestamp));
      }
    } else {
      Serial.println("Wi-Fi not connected. Cannot send UID and timestamp to API.");
    }
    // dataFile = SD.open("/data.txt");
    // if (dataFile) {
    //   dataFile.println("Card Data: " + content);
    //   dataFile.println("Timestamp: " + timestamp);
    //   dataFile.println();
    //   dataFile.close();
    //   Serial.println("Data written to SD card.");
    // } else {
    //   Serial.println("Error opening data.txt on SD card.");
    // }

    delay(500);
  }
  checkButton();
  // if (configChanged) {
  //   if (writeConfigToSD("/config.json", ssid, password, apiEndpoint)) {
  //     Serial.println("Configuration saved to SD card.");
  //   } else {
  //     Serial.println("Failed to save configuration to SD card.");
  //   }
  //   configChanged = false;
  // }
  // ReadFile("/data.txt");
  delay(3000);
}
