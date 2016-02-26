#include <SPI.h>
#include <MFRC522.h>
// #include "Adafruit_GFX.h"
// #include "Adafruit_ILI9341.h"
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>

#define TFT_DC 2
#define TFT_CS 5

#define RST_PIN         0           // Configurable, see typical pin layout above
#define SS_PIN          4          // Configurable, see typical pin layout above

// WiFi information
const char WIFI_SSID[] = "doordarshan";
const char WIFI_PSK[] = "akashvani";

// Remote site information
const char host[] = "sso.logicalworks.com";
const int port = 80;

WiFiClient client;
int wifiConnected = 0;
void connectWiFi();

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance

MFRC522::MIFARE_Key key;

void setup() {
        int ret;
        Serial.begin(115200);        // Initialize serial communications with the PC
        Serial.println(F("Trying to establize network connection to "));
        connectWiFi();
        SPI.begin();               // Init SPI bus
        // Prepare the key (used both as key A and as key B)
        // using FFFFFFFFFFFFh which is the default at chip delivery from the factory
        for (byte i = 0; i < 6; i++) {
            key.keyByte[i] = 0xFF;
        }
        mfrc522.PCD_Init();        // Init MFRC522 card
        Serial.println(F("RFID Reader started.."));
        // Attempt to connect to website
     //   if ( !configure() ) {
      //    Serial.println("Couldn't configure the reader from remote server!");
     //   }
}

void loop() {
        // Look for new cards
        if ( ! mfrc522.PICC_IsNewCardPresent()) {
                delay(10);
                return;
        }
        // Select one of the cards
        if ( ! mfrc522.PICC_ReadCardSerial())    return;

        byte blockAddr      = 1;        
        byte buffer[18];
        MFRC522::StatusCode status;
        byte size = sizeof(buffer);

    // Authenticate using key A
        Serial.println(F("Authenticating using key A..."));
        status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockAddr, &key, &(mfrc522.uid));
        if (status != MFRC522::STATUS_OK) {
           Serial.print(F("PCD_Authenticate() failed: "));
           Serial.println(mfrc522.GetStatusCodeName(status));
           return;
        }
        else Serial.println(F("PCD_Authenticate() success: "));
        
    // Read data from the block
        Serial.print(F("Reading data from block ")); Serial.print(blockAddr);
        Serial.println(F(" ..."));
        status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
        if (status != MFRC522::STATUS_OK) {
            Serial.print(F("MIFARE_Read() failed: "));
            Serial.println(mfrc522.GetStatusCodeName(status));
        }
        Serial.print(F("Data in block ")); Serial.print(blockAddr); Serial.println(F(":"));
        Serial.println();
        
      // Halt PICC
      mfrc522.PICC_HaltA();
      // Stop encryption on PCD
      mfrc522.PCD_StopCrypto1();
      //testText(); 
}

// Attempt to connect to WiFi
void connectWiFi() {
  // Set WiFi mode to station (client)
  WiFi.mode(WIFI_STA);
  
  // Initiate connection with SSID and PSK
  WiFi.begin(WIFI_SSID, WIFI_PSK);
  
  // Blink LED while we wait for WiFi connection
  int counter = 0;
  while ( WiFi.status() != WL_CONNECTED && counter < 100 ) {
    delay(100);
    counter++;
  }
  if ( WiFi.status() != WL_CONNECTED ) {
    wifiConnected = 0;
    Serial.println(F("Connection failed!"));
  }
  else {
    wifiConnected = 1;
    Serial.println(F("Connected"));
  }
}

String httpRequest(String url) {
  // Attempt to make a connection to the remote server
  if ( !client.connect(host, port) ) {
    String s = "ERROR! Not able to connect to host " + String(host) + ":" + String(port);
  }

  // Send HTTP Request to remote host
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");
               
  delay(100);
  
  // Wait for data from client to become available
  while(!client.available()){
     delay(1);
  }  
  // Read status code
  String line = client.readStringUntil('\r');
  Serial.print(line);

  String  status = line.substring(9, 12);
  Serial.println(status);

  if (!status.equals("200")) {
    Serial.println("HTTP Error, status code : " + status);
    return "ERROR: " + status;
  }
  String json = "";
   while(client.available()){
      String line = client.readStringUntil('\r');
      line.trim();
      if (line.startsWith("{")) {
        json = line;
        break;
      }
   }

  Serial.print(json);
  return json;
}

// Perform an HTTP GET request to a remote page
bool configure() {
  // Prepare URL
  String url = "/config.json";
  
  // Make a http request to config server
  String json = httpRequest(url);
  
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(json);
  
  if (!root.success()) {
    Serial.println("parseObject() failed");
    return false;
  }
  String authenticate   = root["auth"];
  String gate   = root["gate"];
  String active   = root["active"];
  String endpointUrl = root["endpoint"];
  return true;
}

/**
 * Helper routine to dump a byte array as hex values to Serial.
 */
void dump_byte_array(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print((char)buffer[i]);
        //Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        //Serial.print(buffer[i], HEX);
    }
}


