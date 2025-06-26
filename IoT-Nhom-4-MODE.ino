#include <WiFi.h>
#include <FirebaseESP32.h>
#include "ld2410.h"
#include <IRremote.hpp>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <NTPClient.h>
#include <WiFiUdp.h> //Thư viện WiFiUdp.h là thư viện hỗ trợ kết nối giao thức UDP qua WiFi cho ESP32
// Thông tin WiFi
#define WIFI_SSID "VNU-IS 208"
#define WIFI_PASSWORD "182597463Qq"          
// Thông tin Firebase
#define FIREBASE_HOST "https://turnonoffled-bf81e-default-rtdb.firebaseio.com/"
#define FIREBASE_AUTH "1VqYC7q0Yg8lDy5jTDfyrRCtGaEE1XrlakdBIu4U"  

#define TFT_CS     5
#define TFT_RST    4
#define TFT_DC     2

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
ld2410 radar;
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7 * 3600, 60000);  // GMT+7

int ledPin = 27;
int ledIR = 26;
int Recv_Pin = 34;  
String ledStatus = "false";
String mode = "true";
String hengiobat = "false";
String hengiotat = "false";
const int cambien = 19;
bool radarConnected = false;
unsigned long currentTime = millis();
unsigned long previousTime = 0; 
const long timeoutTime = 2000;
uint32_t lastReading = 0;
String hourOn , minuteOn ;
String hourOff , minuteOff ;
const long interval = 10000; // 10 giây
int ledhengio = 0;

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  pinMode(ledIR, OUTPUT);
  digitalWrite(ledIR, LOW);
  pinMode(cambien, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  tft.initR(INITR_BLACKTAB); // Khởi tạo với loại bảng ST7735 thông dụng
  tft.setRotation(1);        // Xoay màn hình nếu cần (0-3)
  tft.fillScreen(ST77XX_BLACK);         // Xóa màn hình
  tft.setTextColor(ST77XX_WHITE);       // Màu chữ
  tft.setTextSize(2);                   // Cỡ chữ
  tft.setCursor(10, 30);                // Vị trí chữ
  tft.println("Xin chao!");
  tft.setCursor(10, 60);
  tft.println("IoT Nhom 4");
  delay(1000);
  tft.fillScreen(ST77XX_BLACK);

  Serial2.begin(256000, SERIAL_8N1, 16, 17);  
  if (radar.begin(Serial2)) {
    Serial.println("LD2410B khởi động thành công!");
    tft.setCursor(1, 30);
    tft.println("LD2410B khoi dong thanh cong!");delay(500);
    tft.fillScreen(ST77XX_BLACK);
  } else {
    Serial.println("Không thể kết nối LD2410B!");
    tft.setCursor(1, 30);
    tft.println("Khong the ket noi LD2410B! Khoi dong lai sau 1s..");
    delay(1000);
    ESP.restart();
  }
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Đang kết nối Wi-Fi");
  tft.setCursor(10, 30);
  tft.println("Dang ket noi Wifi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    tft.print(".");
    digitalWrite(LED_BUILTIN,HIGH);delay(300);  //đèn D2 báo trạng thái Wifi, nháy chậm khi chưa kết nối
    digitalWrite(LED_BUILTIN,LOW);
  }
  Serial.println("Kết nối Wi-Fi thành công!");
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(1, 30);
  tft.println("Ket noi thanh cong!!!");delay(500);
  tft.fillScreen(ST77XX_BLACK);
  digitalWrite(LED_BUILTIN,HIGH); delay(200);  // kết nối WIFI thành công, đèn chớp 2 lần
  digitalWrite(LED_BUILTIN,LOW);delay(100);
  digitalWrite(LED_BUILTIN,HIGH);delay(200);
  digitalWrite(LED_BUILTIN,LOW);

  // Cấu hình Firebase
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  IrReceiver.begin(Recv_Pin, true, 2); // True cho phép led nối với chân 2 nhấp nháy
  IrSender.begin(Recv_Pin, true, 2); 
  timeClient.begin(); // Bắt đầu NTP
}


void loop() {
    // --------------Hẹn giờ bật tắt------------ 
  if (Firebase.getString(fbdo, "/hengiobat")) {
  hengiobat = fbdo.stringData();
  }
  if (Firebase.getString(fbdo, "/hengiotat")) {
  hengiotat = fbdo.stringData();
  }
  if (hengiobat == "true" || hengiotat == "true" ) {

    currentTime = millis();
    if (currentTime - previousTime >= interval) { // 10 giây thực hiện kiểm tra giờ
      previousTime = currentTime;

      timeClient.update();
      int currentHour = timeClient.getHours();
      int currentMinute = timeClient.getMinutes();

      // Đọc từ Firebase
      if (Firebase.getString(fbdo, "/schedule/hourOn")) {
        hourOn = fbdo.stringData();
      }
      if (Firebase.getString(fbdo, "/schedule/minuteOn")) {
        minuteOn = fbdo.stringData();
      }
      if (Firebase.getString(fbdo, "/schedule/hourOff")) {
        hourOff = fbdo.stringData();
      }
      if (Firebase.getString(fbdo, "/schedule/minuteOff")) {
        minuteOff = fbdo.stringData();
      }        
      // Debug
      Serial.printf("Giờ hiện tại: %02d:%02d | BẬT: %02d:%02d | TẮT: %02d:%02d\n",
        currentHour, currentMinute, hourOn.toInt(), minuteOn.toInt(), hourOff.toInt(), minuteOff.toInt());
      if (hengiobat == "true") {
        // So sánh thời gian
        if (currentHour == hourOn.toInt() && currentMinute == minuteOn.toInt()) {
          digitalWrite(ledPin, HIGH);
          Serial.println("💡 BẬT đèn theo lịch");
        }
      }
      if (hengiotat == "true") {
        if (currentHour == hourOff.toInt() && currentMinute == minuteOff.toInt()) {
        digitalWrite(ledPin, LOW);
        Serial.println("💤 TẮT đèn theo lịch");
        }
      }
      
    }
  }
  else {
    if (Firebase.getString(fbdo, "/mode")) {  
      mode = fbdo.stringData();

    // Chế độ THỦ CÔNG (qua WiFi)
    if (mode == "true") {
      if (Firebase.getString(fbdo, "/gpio26")) {
        ledStatus = fbdo.stringData();
        if (ledStatus == "true") {
          digitalWrite(ledPin, HIGH);
          ledhengio = 1;
          Serial.println("Đèn bật");
        } else {
          digitalWrite(ledPin, LOW);
          ledhengio = 0;
          Serial.println("Đèn tắt");
        }
      } else {
        Serial.println("Lỗi đọc /gpio26 từ Firebase");
        ESP.restart();
      }
    }

    // Chế độ TỰ ĐỘNG (radar)
    else {
      radar.read();

      if (radar.isConnected() && millis() - lastReading > 1000) {
        lastReading = millis();

        if (radar.presenceDetected()) {
          if (radar.stationaryTargetDetected()) {
            Serial.print(F("Stationary target: "));
            Serial.print(radar.stationaryTargetDistance());
            Serial.print(F("cm energy:"));
            Serial.print(radar.stationaryTargetEnergy());
          }

          if (radar.movingTargetDetected()) {
            uint16_t distance = radar.movingTargetDistance();
            Serial.print(F("\nMoving target: "));
            Serial.print(distance);
            Serial.print(F("cm energy:"));
            Serial.print(radar.movingTargetEnergy());

            if (Firebase.setInt(fbdo, "/distance", distance)) {
              Serial.println(" → Đã cập nhật Firebase");
            } else {
              Serial.print(" → Lỗi gửi Firebase: ");
              Serial.println(fbdo.errorReason());
            }
          }
        } else {
          Serial.println(F("No target"));
        }

        Serial.println();
      }

      int i = digitalRead(cambien);  // cảm biến khi phát hiện con người sẽ cho giá trị chân OUt (gán chân số 19) là 1
      digitalWrite(ledPin, i != 0 ? HIGH : LOW);   // đảo ngược giá trị đọc được của ledIR (đèn tắt thì bật và ngược lại)
      if (ledPin) {
        ledhengio = 1;
      } else {
        ledhengio = 0;
      }

      delay(2000);
    }

  } else {
    Serial.println("Lỗi đọc /mode từ Firebase");
    ESP.restart();
    }
  }

  

  // ======== Xử lý Remote IR ========
  if (IrReceiver.decode()) {
    uint32_t dataRemote = IrReceiver.decodedIRData.decodedRawData;

    if (dataRemote > 0) {
      Serial.println(dataRemote);

      if (millis() - currentTime > 250) {  
        switch (dataRemote) {
          case 3125149440:           // mã code tùy nút và điều khiển
            digitalWrite(ledIR, !digitalRead(ledIR)); 
            break;
          case 3091726080:
            for (int i=0; i<5 ;i++) {
              digitalWrite(ledIR, HIGH); delay(100);
              digitalWrite(ledIR, LOW); delay(100);
            }
            break;
        }
      }

      currentTime = millis();
    }
    IrReceiver.resume();
  }

}
