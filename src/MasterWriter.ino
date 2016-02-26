#include <SPI.h>
#include <MFRC522.h>
#include "MasterKeyCode.h"
// #include "Timer.h"

// RFID SPI Configuration
#define RST_PIN         0           
#define SS_PIN          4           

bool rfidWrite(int, byte (&)[16]); // Writes 16 bytes of data into given block
bool rfidRead (int, byte (&)[16]);
void printlnHex16(byte (&)[16]);
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance
MFRC522::MIFARE_Key key;

char step = 0;
String choice = "";
String deviceID = "";
String ssid = "";
String psk = "";
String url = "";

void setup() {
  Serial.begin(115200);        // Initialize serial communications with the PC
  Serial.println(F("RFID Master key preparation device"));
  SPI.begin();               // Init SPI bus
  memset(&key, 0xFF, sizeof(key));
  mfrc522.PCD_Init();        // Init MFRC522 card
}

String readInput(String text) {
  // Wait for user input.
  Serial.print(text);
   while (Serial.available() <= 0) {
    delay(100);
   }
   
    String buf;
    while (Serial.available() > 0) {
      char rx_byte = Serial.read(); 
      buf += rx_byte;
    }
    return buf;
}




void loop() {
  
  if (step == 0) {
    choice = readInput("Enter 1 for Read RFID, 2 for Write : ");
  }
  if (choice == "1") {
    Serial.println("Read");
    Serial.print("");
    delay(1000);
    readRfidCard();
    delay(100);
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
  }
  else if (choice == "2") {
    if (step == 0) { // Get SSID
      ssid = readInput("Enter WiFi SSID :");
      Serial.println(" " +ssid);
      step = 1;
    }
    else if (step == 1) { //GET PSK
      psk = readInput("Enter WiFi Pre-Share Key :");
      Serial.println(" " +psk);
      step = 2;
    }
    else if (step == 2) { // GET URL
      url = readInput("Enter CICO REST Url :");
      Serial.println(" " +url);
      step = 3;
    }
    else if (step == 3) { // GET Device ID
      deviceID = readInput("Enter RFID Device Unique ID:");
      Serial.println(" " +deviceID);
      Serial.println(F("Please keep the RFID Card on the device until GREEN LED stop blinking"));
      delay(1000);
      step = 4;
    }
    else if (step == 4) {
      if (deviceID != "") {
        if(writeKey(deviceID)) {
          step = 3;
         }
      }
      else
        step = 3;
    }
  }
}

void readRfidCard() {
  if ( ! mfrc522.PICC_IsNewCardPresent() || ! mfrc522.PICC_ReadCardSerial())    
      return;
      
    int RFID_BLOCK = 56;
    //Sector 15 (16) last sector fast to read.
    if (rfidAuthenticate(RFID_BLOCK)) {
      byte data[16];
      // Read SSID
      memset(&data, 0, 16);
      rfidRead(RFID_BLOCK, data);
      Serial.println("SSID : " + byteToString(data, 16));
      // printlnHex16(data);
      
      // Read PKS
      memset(&data, 0, 16);
      rfidRead(RFID_BLOCK+1, data);
      Serial.println("PKS : " + byteToString(data, 16));
      // printlnHex16(data);
      
      // Read Device ID
      memset(&data, 0, 16);
      rfidRead(RFID_BLOCK+2, data);
      Serial.println("Device ID : " + byteToString(data, 16));
      // printlnHex16(data);
    }

    RFID_BLOCK = 60; // Read Master Key Code
    if (rfidAuthenticate(RFID_BLOCK)) {
      byte data[16];
      memset(&data, 0, 16);
      rfidRead(RFID_BLOCK, data);
      Serial.println("MKCode : " + byteToString(data, 16));
      // printlnHex16(data);
    }

    RFID_BLOCK = 52; // Read URL
    if (rfidAuthenticate(RFID_BLOCK)) {
      String url = "";
      byte data[16];
      memset(&data, 0, 16);
      rfidRead(RFID_BLOCK, data);
      url += byteToString(data, 16);
      memset(&data, 0, 16);
      rfidRead(RFID_BLOCK+1, data);
      url += byteToString(data, 16);
      memset(&data, 0, 16);
      rfidRead(RFID_BLOCK+2, data);
      url += byteToString(data, 16);
      Serial.println("URL : " + url);
      // printlnHex16(data);
    }
}

bool writeKey(String key) {
   // blinkLED(3);
    if ( ! mfrc522.PICC_IsNewCardPresent() || ! mfrc522.PICC_ReadCardSerial())    
      return false;
    Serial.println("Writing key ");

    // Sector 15
    int RFID_BLOCK = 56;
     // Write SSID
    if (rfidAuthenticate(RFID_BLOCK)) {
      // 16 Sectors, 4 Blocks / Sector. Block #1 start from Sector #1 
      // Sector 1, 1 Block used for each SSID
      //            2 Block for PKS
      //            3 Block for Device ID
      // Sector 2, 4 Blocks used for URL
      byte data[16];
      memset(&data, 0, 16);
      ssid.getBytes(data,16);
      rfidWrite(RFID_BLOCK, data);
      Serial.println("Writing SSID ");
    }

    // Write PKS
    RFID_BLOCK = RFID_BLOCK + 1;
    if (rfidAuthenticate(RFID_BLOCK)) {
      byte data[16];
      memset(&data, 0, 16);
      psk.getBytes(data,16);
      rfidWrite(RFID_BLOCK, data);
      Serial.println("Writing PKS ");
    }

    // Write Device ID
    RFID_BLOCK = RFID_BLOCK + 1;
    if (rfidAuthenticate(RFID_BLOCK)) {
      byte data[16];
      memset(&data, 0, 16);
      deviceID.getBytes(data,16);
      rfidWrite(RFID_BLOCK, data);
      Serial.println("Writing Device ID ");
    }


   // Sector 16
    //Write Master Key Code, used to identify the RFID card as Master Key
    RFID_BLOCK = 60;
    if (rfidAuthenticate(RFID_BLOCK)) {
      byte data[16];
      memset(&data, 0, 16);
      masterKeyCode.getBytes(data,16);
      rfidWrite(RFID_BLOCK, data);
      Serial.println("Writing Master Key Code ");
    }

   // Sector 14, Block 52, 53 and 54
   // URL
    RFID_BLOCK = 52;
    if (rfidAuthenticate(RFID_BLOCK)) {
      byte data[51];
      memset(&data, 0, 51);
      url.getBytes(data,51);
      byte buf[16];
      memcpy(buf, data, 16);
      rfidWrite(RFID_BLOCK, buf);
      memset(&buf, 0, 16);
      Serial.println("Writing CICO");
      memcpy(buf, data + 16, 16);
      rfidWrite(RFID_BLOCK+1, buf);
      memcpy(buf, data + 32, 16);
      rfidWrite(RFID_BLOCK+2, buf);
      
    }
      Serial.println();
      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();
      return true;
}


boolean rfidAuthenticate(byte block) {
  MFRC522::StatusCode status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return false;
  }
  return true;
}


bool rfidWrite(int block, byte (&data)[16]) {
  MFRC522::StatusCode status = mfrc522.MIFARE_Write(block, data, 16);
  if (status != MFRC522::STATUS_OK) {
      Serial.print(F("MIFARE_Write() failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
      return false;
  }
  Serial.println(F("MIFARE_Write() success: "));
  return true;
}


bool rfidRead(int block, byte (&target)[16]) {
  byte buffer[18];
  byte byteCount = sizeof(buffer);
  int status = mfrc522.MIFARE_Read(block, buffer, &byteCount);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE Read() failed: "));
    return false;
  }
  memcpy(target, buffer, 16);
  return true;
}

void print(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print((char)buffer[i]);
    }
}

String byteToString(byte *buffer, byte bufferSize) {
    String buf = "";
    for (byte i = 0; i < bufferSize; i++) {
      if ((char)buffer[i] != 0)
        buf += (char)buffer[i];
    }
    return buf;
}

void printlnHex16(byte (&data)[16]) {
  for (byte i=0; i<16; i++) {
    Serial.print(data[i] < 0x10 ? " 0" : " ");
    Serial.print(data[i], HEX);
  }
  Serial.println();
}

