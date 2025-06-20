#include <WiFi.h>
#include <FirebaseESP32.h>
#include "ld2410.h"
// Thông tin WiFi
#define WIFI_SSID "VNU-IS 208"
#define WIFI_PASSWORD "182597463Qq"
                 
// Thông tin Firebase
#define FIREBASE_HOST "https://turnonoffled-bf81e-default-rtdb.firebaseio.com/"
#define FIREBASE_AUTH "1VqYC7q0Yg8lDy5jTDfyrRCtGaEE1XrlakdBIu4U"  

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
ld2410 radar;

int ledPin = 27; 
int led27 = 2; 
bool ledStatus = false;
bool mode = true;
const int cambien = 19;
bool radarConnected = false;
unsigned long currentTime = millis();
unsigned long previousTime = 0; 
const long timeoutTime = 2000;
uint32_t lastReading = 0;

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  pinMode(led27, OUTPUT);
  pinMode(cambien, INPUT);
  digitalWrite(led27, LOW);
  Serial2.begin(256000, SERIAL_8N1, 16, 17);  

  if (radar.begin(Serial2)) {
    Serial.println("LD2410B khởi động thành công!");
  } else {
    Serial.println("Không thể kết nối LD2410B!");
  }
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Đang kết nối Wi-Fi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Kết nối Wi-Fi thành công!");

  // Cấu hình Firebase
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop() {

if( Firebase.getBool(fbdo, "/mode")) {
  mode = fbdo.boolData();
  if (mode==true) {
    if (Firebase.getBool(fbdo, "/gpio26")) {
    ledStatus = fbdo.boolData();
    digitalWrite(ledPin, ledStatus & mode ? HIGH : LOW);
    Serial.println(ledStatus ? "Bật đèn" : "Tắt đèn");
    
  } else {
    Serial.println("Lỗi đọc dữ liệu từ Firebase");
    ESP.restart();
  }
  }
  else {
    radar.read();
    if(radar.isConnected() && millis() - lastReading > 1000)  //Report every 1000ms
    {
    lastReading = millis();
    if(radar.presenceDetected())
    {
      if(radar.stationaryTargetDetected())
      {
        //uint16_t stationary = radar.stationaryTargetDistance();
        Serial.print(F("Stationary target: "));
        Serial.print(radar.stationaryTargetDistance());
        Serial.print(F("cm energy:"));
        Serial.print(radar.stationaryTargetEnergy());
        Serial.print(' ');
      }
    }
      /*if(radar.movingTargetDetected())
      {
        Serial.print(F("Moving target: "));
        Serial.print(radar.movingTargetDistance());
        Serial.print(F("cm energy:"));
        Serial.print(radar.movingTargetEnergy());
      }*/
      if (radar.movingTargetDetected()) {
    uint16_t distance = radar.movingTargetDistance();  // Lưu giá trị khoảng cách
    Serial.print(F("Moving target: "));
    Serial.print(distance);
    Serial.print(F("cm energy:"));
    Serial.print(radar.movingTargetEnergy());

    // Gửi lên Firebase node "/distance"
    if (Firebase.setInt(fbdo, "/distance", distance)) {
      Serial.println(" → Đã cập nhật Firebase");
    } else {
      Serial.print(" → Lỗi gửi Firebase: ");
      Serial.println(fbdo.errorReason());
    }
  }
      Serial.println();
    }
    else
    {
      Serial.println(F("No target"));
    }
  
  
  int i = digitalRead(cambien);
  if (i!=0) {
    digitalWrite(ledPin, HIGH);
  }
  else {
    digitalWrite(ledPin, LOW);
  }
  delay(200);
}
}

else {
    Serial.println("Lỗi đọc dữ liệu từ Firebase");
    ESP.restart();
  } 
}

