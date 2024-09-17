#include <PZEM004Tv30.h>
#include <SPI.h>
#include <LoRa.h>

// Định nghĩa các chân phù hợp
#define nss 15   // D8 - Đảm bảo rằng bạn không gặp vấn đề với khởi động
#define rst 5    // D1 - GPIO5, một chân khác có thể được sử dụng cho reset
#define dio0 4   // D2 - GPIO4

PZEM004Tv30 pzem(D3, D4); // Software Serial pin 8 (RX) & 9 (TX)

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("LoRa Sender");

  // Thiết lập chân
  LoRa.setPins(nss, rst, dio0);

  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
    lcd.init();

}

void loop() {
    // Đọc dữ liệu từ cảm biến
    float voltage = pzem.voltage();
    float current = pzem.current();
    float power = pzem.power();
    float energy = pzem.energy();
    float frequency = pzem.frequency();
    float pf = pzem.pf();

    // Kiểm tra dữ liệu có hợp lệ không
    if (isnan(voltage) || isnan(current) || isnan(power) || 
        isnan(energy) || isnan(frequency) || isnan(pf)) {
        Serial.println("Error reading PZEM data");
    } else {
        // In giá trị lên Serial console
        Serial.print("Voltage: ");      Serial.print(voltage);      Serial.println(" V");
        Serial.print("Current: ");      Serial.print(current);      Serial.println(" A");
        Serial.print("Power: ");        Serial.print(power);        Serial.println(" W");
        Serial.print("Energy: ");       Serial.print(energy,3);     Serial.println(" kWh");
        Serial.print("Frequency: ");    Serial.print(frequency, 1); Serial.println(" Hz");
        Serial.print("PF: ");           Serial.println(pf);

        // Gửi dữ liệu qua LoRa
        LoRa.beginPacket();
        LoRa.print(voltage);    LoRa.print(",");
        LoRa.print(current);    LoRa.print(",");
        LoRa.print(power);      LoRa.print(",");
        LoRa.print(energy);     LoRa.print(",");
        LoRa.print(frequency);  LoRa.print(",");
        LoRa.print(pf);
        LoRa.endPacket();

        Serial.println("Data sent via LoRa");
    }

    Serial.println();
    delay(2000);
}
