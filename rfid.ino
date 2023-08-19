#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN     5    // Define the RST_PIN (reset) for RC522 module
#define SS_PIN      4    // Define the SS_PIN (SDA) for RC522 module

MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance

void setup() {
  Serial.begin(115200);  // Initialize serial communication
  
  SPI.begin();           // Initialize SPI bus
  mfrc522.PCD_Init();    // Initialize MFRC522 RFID module
  
  Serial.println("RFID Reader is ready!");
}

void loop() {
  // Look for new RFID cards
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    Serial.print("UID tag :");
    String content = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
      content.concat(String(mfrc522.uid.uidByte[i], HEX));
    }
    content.toUpperCase();
    Serial.println(content);
    delay(1000); // Delay to avoid reading the same card multiple times
  }
}
