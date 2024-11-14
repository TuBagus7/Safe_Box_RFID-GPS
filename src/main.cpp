/* beberapa library Arduino dimasukkan untuk mengatur perangkat,
   seperti LCD, keypad, WiFi, Telegram, RFID, dan GPS.
*/
#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <I2CKeyPad.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <MFRC522.h>
#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>
#include <Preferences.h>
#include <NusabotSimpleTimer.h>
//===========================================================================================
/* Menetapkan pin-pin tertentu untuk 
   buzzer, relay, LCD, keypad, dan sensor RFID.
   ke pinout ESP32 dan GPS
*/

TaskHandle_t Task1;
TaskHandle_t Task2;

#define SS_PIN 5
#define RST_PIN 15
#define buzzer 12
#define relay 4
// data untuk Pin GPS
static const int RXPin = 3, TXPin = 1;
static const uint32_t GPSBaud = 9600;
//============================================================================================
/* BOT_TOKEN dan CHAT_ID adalah data unik untuk komunikasi bot Telegram.
  Di dapatkan dari BoTFather dan IDBot di aplikasi Telegram
*/

//Telegram Sonya
#define BOT_TOKEN "7584231810:AAECt9V17yd-Gyq9KYjwSZWBsnLJBvFMSEA"
#define CHAT_ID "1806805645"
//Bukan telegram Sonya
//#define BOT_TOKEN "6921950308:AAHNeuJV0H-vKRp0yKMiPUe8F5VoI3LZ2aU"
//#define CHAT_ID "1267170799"


//pin GPS

// ini adalah objek yang di dapatkan dari library masing masing
TinyGPSPlus gps;
MFRC522 rfid(SS_PIN, RST_PIN);              // Inisialisasi RFID
LiquidCrystal_I2C lcd(0x27, 16, 2);         // LCD di I2C pada pin 21, 22
I2CKeyPad keyPad(0x27, &Wire1);             // Keypad di I2C pada pin 32, 33
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);
SoftwareSerial ss (RXPin, TXPin);
HardwareSerial SerialGPS(2);



// Data untuk WiFi dan Telegram
const char* ssid = "TUF";
const char* pass = "ridho111";


// Data Kartu yang disimpan untuk digunakan sebagai Kartu yang Valid
String enteredPin = "";
String correctPin = "3024";// PIN Keypad
byte authorizedCard[] = {211, 208, 254, 254};  // UID kartu yang diizinkan

// Pemetaan tombol sesuai keypad 4x4 standar
char keys[16] = {
  '1', '2', '3', 'A',
  '4', '5', '6', 'B',
  '7', '8', '9', 'C',
  '*', '0', '#', 'D'
};

// variabel untuk milis
unsigned long previousMillis = 0;
const long interval = 100;
unsigned int move_index = 1; // fixed location for now

float latitude ; // Storing the Lat. and Lon.
float longitude;
// float latitude = 0.4448862541035513;
// float longitude = 101.45465014098586;

// bool LocationRequested;
//=================================================================================================
//void checkGPS()// fungsi yang digunaan untuk mengecek apakah GPS terhubung.
//{
//  if (gps.charsProcessed() < 4)
//  {
//    Serial.println(F("No GPS detected: check wiring."));
//  }
//}

//==================================================================================================
//void sendLocation()// fungsi yang bertugas mengrimkan data lokasi dari GPS
//{
//  while (SerialGPS.available() > 0)
//  {
//    if (gps.encode(SerialGPS.read()))
//    {
//      if (gps.location.isValid())
//      {
//       float latitude = (gps.location.lat()); // Storing the Lat. and Lon.
//        float longitude = (gps.location.lng());
//
//        Serial.print("LAT:  ");
//        Serial.println(latitude, 6); // float to x decimal places
//        Serial.print("LONG: ");
//        Serial.println(longitude, 6);
//        String lokasi = "Lokasi : https://www.google.com/maps/@";
//        lokasi +=String(latitude, 6);
//        lokasi +=",";
//        lokasi +=String(longitude, 6);
//        lokasi +=",19z?entry=ttu";
//        bot.sendMessage(CHAT_ID, lokasi, "");
//        break;  // Keluar dari while untuk mencegah pengiriman berulang
//       }
//
//      }
//  }
//}
void sendTelegram(){
  int messageCount = bot.getUpdates(bot.last_message_received + 1);
    while (messageCount) {
    Serial.println("Pesan baru dari Telegram");
    for (int i = 0; i < messageCount; i++) {
      String text = bot.messages[i].text;

      if (text == "open") {  
        digitalWrite(relay, LOW);  
        bot.sendMessage(CHAT_ID, "Brankas dibuka.", "");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Brankas Dibuka");
        delay(5000);
        digitalWrite(relay, HIGH); // Close solenoid
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Time Out");
        bot.sendMessage(CHAT_ID, "Brankas Tertutup Otomatis Selama 5 Detik", "");

      }
      else if (text == "close") {
        digitalWrite(relay, HIGH);  
        bot.sendMessage(CHAT_ID, "Brankas ditutup.", "");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Brankas Ditutup");
      }
      else if (text == "lokasi"){
      while (SerialGPS.available() > 0){
      if (gps.encode(SerialGPS.read())){
      if (gps.location.isValid()){
      float latitude = (gps.location.lat()); // Storing the Lat. and Lon.
      float longitude = (gps.location.lng());

        Serial.print("LAT:  ");
        Serial.println(latitude, 6); // float to x decimal places
        Serial.print("LONG: ");
        Serial.println(longitude, 6);
        String lokasi = "Lokasi: https://www.google.com/maps?q=";
        lokasi += String(latitude, 6);
        lokasi += ",";
        lokasi += String(longitude, 6);
        lokasi += "&z=19";
        bot.sendMessage(CHAT_ID, lokasi, "");

                  // Reset isLocationRequested setelah mengirim lokasi
        break;  // Keluar dari while untuk mencegah pengiriman berulang
       }

      }
    }
    }
    messageCount = bot.getUpdates(bot.last_message_received + 1);
  }
}
}
//================================================================================================
void secure() // fungsi untuk cek validasi kartu 
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Tap Kartu");
  while (true) {
    uint8_t idx = keyPad.getKey();
    if (idx < 16 && keys[idx] == 'C') {
      digitalWrite(relay, HIGH); // Close the solenoid
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Brankas Ditutup");
      bot.sendMessage(CHAT_ID, "Brankas ditutup oleh tombol C.", "");
      delay(500); // Short delay to show the message
      break; // Exit the secure function
    }

    // Proceed with RFID checking
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      bool isAuthorized = true;
      for (byte i = 0; i < sizeof(authorizedCard); i++) {
        if (rfid.uid.uidByte[i] != authorizedCard[i]) {
          isAuthorized = false;
          break;
        }
      }

      if (isAuthorized) {
        Serial.println("Kartu diizinkan");
        digitalWrite(buzzer, HIGH);
        delay(50);
        digitalWrite(buzzer, LOW);
        delay(300);
        digitalWrite(buzzer, HIGH);
        delay(300);
        digitalWrite(buzzer, LOW);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Masukkan PIN");

        enteredPin = "";
        while (true) {
          uint8_t idx = keyPad.getKey();
          if (idx < 16) {  
            char key = keys[idx];

            if (key == '#') {
              if (enteredPin == correctPin) {
                Serial.println("Akses Diizinkan");
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("Akses Diizinkan");
                digitalWrite(buzzer, HIGH);
                delay(300); 
                digitalWrite(buzzer, LOW);
                digitalWrite(relay, LOW); // Open solenoid
                bot.sendMessage(CHAT_ID, "Akses Diizinkan. Kartu dan PIN valid.", "");
                delay(5000);
                digitalWrite(relay, HIGH); // Close solenoid
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("Time Out");
                bot.sendMessage(CHAT_ID, "Brankas Tertutup Otomatis Selama 5 Detik", "");
              } else {
                Serial.println("PIN Salah");
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("PIN Salah");
                digitalWrite(buzzer, HIGH);
                delay(1000);
                digitalWrite(buzzer, LOW);
                bot.sendMessage(CHAT_ID, "Pin Salah", "");
              }
              break;
            } else if (key == '*') { 
              enteredPin = "";
              lcd.setCursor(0, 1);
              lcd.print("PIN Direset");
              delay(500);
              lcd.setCursor(0, 1);
              lcd.print("            ");
            } else if (key == 'C') {
              digitalWrite(relay, HIGH); // Close solenoid
              bot.sendMessage(CHAT_ID, "Brankas ditutup", "");
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("Brankas Ditutup");
              break;
            } else if (key == 'B') { 
              if (enteredPin.length() > 0) {
                enteredPin.remove(enteredPin.length() - 1); // Hapus satu karakter terakhir
                lcd.setCursor(0, 1);
                lcd.print("PIN: ");
                lcd.print(enteredPin);
                lcd.print(" "); // Menghapus karakter sebelumnya di LCD
                delay(200);
              }
            } else { 
              enteredPin += key;
              lcd.setCursor(0, 1);
              lcd.print("PIN: ");
              lcd.print(enteredPin);
              delay(200);
            }
          }
        }
      } else {
        Serial.println("Kartu tidak diizinkan");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Akses Ditolak");
        digitalWrite(buzzer, HIGH);
        delay(1000);
        digitalWrite(buzzer, LOW);
        bot.sendMessage(CHAT_ID, "Akses Ditolak. Kartu tidak dikenal.", "");
      }
      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
      break; // Exit the secure function after card check
    }
  }
}

//============================================================================================
//void handleTelegramMessage()// fungsi yang bertugas mengirimkan perintah ke Telegram 
//{
// 
//}
//===============================================================================================
void connect() // Fungsi untuk menghubungkan WiFi
{
  Serial.println("Menyambungkan ke WiFi");
 
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("Tersambung ke WiFi");
}
//=========================================================================================
/* Fungsi ini menjalankan langkah awal, yaitu fungsi yang di jalankan
  saat pertama kali alat di nyalakan
*/
void setup()
{
  pinMode(buzzer, OUTPUT);
  pinMode(relay, OUTPUT);
  digitalWrite(relay, HIGH);
  Serial.begin(115200);
  WiFi.begin(ssid, pass);
  secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
  Wire.begin(21, 22);
  SerialGPS.begin(9600, SERIAL_8N1,  17, 16 );//17 Pin TX(GPS), 16 PIN RX(GPS)
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Searching Wifi");
  Wire1.begin(32, 33);
  if (!keyPad.begin()) {
    Serial.println("Keypad tidak terhubung!");
    while (1);
  }
  while (SerialGPS.available() > 0){
      if (gps.encode(SerialGPS.read())){
      if (gps.location.isValid()){
      float latitude = (gps.location.lat()); // Storing the Lat. and Lon.
      float longitude = (gps.location.lng());
      }
      }
  }
  SPI.begin();
  rfid.PCD_Init();
  bot.sendMessage(CHAT_ID, "Brankas Menyala", "");
  digitalWrite(relay, HIGH);
  connect();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connected");
  delay(500);
  xTaskCreatePinnedToCore(
                    handleTelegram,   // Task function.
                    "Task1",     // name of task.
                    10000,       // Stack size of task
                    NULL,        // parameter of the task
                    1,           // priority of the task 
                    &Task1,      // Task handle to keep track of created task
                    0);          // pin task to core 0                 
  
  xTaskCreatePinnedToCore(
                    handleSecure,   // Task function.
                    "Task2",     // name of task. 
                    10000,       // Stack size of task 
                    NULL,        // parameter of the task 
                    1,           // priority of the task 
                    &Task2,      // Task handle to keep track of created task 
                    1);          // pin task to core 1

                    
}
//==============================================================================================
/* Fungsi main atau fungsi yang di jalankan terus menerus saat alat di nyalakan'
*/
void loop() {
  
}

void handleTelegram( void * pvParameters ){
  while(1)
  {
    sendTelegram();
  } 
}

void handleSecure( void * pvParameters ){
  while(1)
  {
    secure();
  } 
}
