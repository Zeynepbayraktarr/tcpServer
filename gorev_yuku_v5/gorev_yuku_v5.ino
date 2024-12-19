#include <WiFi.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <Wire.h>

// Wi-Fi bilgileri
const char* ssid = "B";           // Wi-Fi SSID
const char* password = "zeynepzeynep";   // Wi-Fi Şifresi
const char* serverIP = "192.168.101.12";  // Sunucunun IP adresi
const int serverPort = 12345;           // Sunucunun portu (Server ile eşleşmeli)

// TCP istemcisi
WiFiClient client;

// BNO055 sensörü
Adafruit_BNO055 bno = Adafruit_BNO055(55);

void setup() {
  Serial.begin(115200);

  // BNO055 sensörünü başlat
  if (!bno.begin()) {
    Serial.println("BNO055 sensörü başlatılamadı. Bağlantıyı kontrol edin!");
    while (1); // Hata durumunda döngüye gir
  }
  Serial.println("BNO055 sensörü başlatıldı.");
  delay(1000); // Sensörün kalibrasyonu için bekleyin

  // Wi-Fi ağına bağlan
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Wi-Fi'ye bağlanıyor...");
  }
  Serial.println("Wi-Fi'ye bağlandı!");

  // Sunucuya bağlan
  if (client.connect(serverIP, serverPort)) {
    Serial.println("Sunucuya bağlandı!");
  } else {
    Serial.println("Sunucuya bağlanılamadı!");
  }
}

void loop() {
  // Sensör verilerini oku
  sensors_event_t event;
  bno.getEvent(&event);

  // Sensör verilerini gönder
  if (client.connected()) {
    // Veriyi JSON formatında oluştur
    String data = String("{\"x\":") + String(event.orientation.x, 2) +
                  ",\"y\":" + String(event.orientation.y, 2) +
                  ",\"z\":" + String(event.orientation.z, 2) + "}";
    client.print(data); // Veriyi sunucuya gönder
    Serial.println("Gönderilen veri: " + data);

    // Sunucudan yanıt bekle
    if (client.available()) {
      String serverResponse = client.readStringUntil('\n');
      Serial.println("Sunucudan gelen yanıt: " + serverResponse);
      Serial.println("*********************************************");
    }
  } else {
    Serial.println("Sunucu bağlantısı kayboldu, tekrar bağlanılıyor...");
    client.connect(serverIP, serverPort); // Tekrar bağlanmayı dene
  }

  delay(2000); // 2 saniyede bir veri gönder
}
