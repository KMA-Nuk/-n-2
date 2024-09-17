#include <SPI.h>

#include <LoRa.h>

#include <LiquidCrystal_I2C.h>
 //#include "uRTCLib.h"
#include <Wire.h>

// Định nghĩa các chân phù hợp
#define nss 5
#define rst 14
#define dio0 2

LiquidCrystal_I2C lcd(0x27, 20, 4);
float voltage, current, power, energy, frequency, pf;
int buzzer = 15;
bool flagloradata = false;
void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("LoRa Receiver");

  // Thiết lập chân
  LoRa.setPins(nss, rst, dio0);

  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  lcd.init();
  lcd.backlight();
  lcd.clear();
  pinMode(buzzer, OUTPUT);
  digitalWrite(buzzer, LOW);
}
void display(float voltage, float current, float power, float energy, float frequency, float pf) {
  // Hiển thị voltage
  lcd.setCursor(0, 0);
  lcd.print("Volt ");
  lcd.print(voltage, 1);
  lcd.setCursor(11, 0);
  lcd.print("V");

  // Hiển thị current
  lcd.setCursor(0, 1);
  lcd.print("Curr ");
  lcd.print(current, 2);
  lcd.setCursor(11, 1);
  lcd.print("A");

  // Hiển thị power
  lcd.setCursor(0, 2);
  lcd.print("Pow  ");
  lcd.print(power, 2);
  lcd.setCursor(11, 2);
  lcd.print("W");

  // Hiển thị energy
  lcd.setCursor(0, 3);
  lcd.print("Ener ");
  lcd.print(energy, 3);
  lcd.setCursor(11, 3);
  lcd.print("kWh");

  // Hiển thị frequency
  lcd.setCursor(14, 0);
  lcd.print(frequency, 1);
  lcd.setCursor(18, 0);
  lcd.print("Hz");

  // Hiển thị power factor
  lcd.setCursor(15, 1);
  lcd.print(pf, 1);
  lcd.setCursor(18, 1);
  lcd.print("Pf");
}
void loop() {

  // Kiểm tra xem có gói tin LoRa nào được nhận
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    flagloradata = true;
    String receivedData = "";

    // Đọc dữ liệu từ gói tin
    while (LoRa.available()) {
      receivedData += (char) LoRa.read();
    }

    Serial.print("Received packet: ");
    Serial.println(receivedData);

    // Mảng để lưu trữ các giá trị
    float values[6];
    int valueIndex = 0;
    int startIndex = 0;

    // Duyệt qua chuỗi và tách giá trị
    for (int i = 0; i < receivedData.length(); i++) {
      if (receivedData[i] == ',' || i == receivedData.length() - 1) {
        // Tách và chuyển đổi giá trị sang float
        if (i == receivedData.length() - 1) {
          values[valueIndex] = receivedData.substring(startIndex).toFloat(); // Giá trị cuối cùng
        } else {
          values[valueIndex] = receivedData.substring(startIndex, i).toFloat();
          startIndex = i + 1;
          valueIndex++;
        }
      }
    }

    // Hiển thị các giá trị đã tách với đơn vị
    Serial.print("Voltage: ");
    Serial.print(values[0]);
    Serial.println(" V"); // Điện áp - Vôn
    Serial.print("Current: ");
    Serial.print(values[1]);
    Serial.println(" A"); // Dòng điện - Ampe
    Serial.print("Power: ");
    Serial.print(values[2]);
    Serial.println(" W"); // Công suất - Watt
    Serial.print("Energy: ");
    Serial.print(values[3]);
    Serial.println(" kWh"); // Năng lượng - Kilowatt giờ
    Serial.print("Frequency: ");
    Serial.print(values[4]);
    Serial.println(" Hz"); // Tần số - Hertz
    Serial.print("PF: ");
    Serial.print(values[5]);
    Serial.println(""); // Hệ số công suất (không có đơn vị)

    Serial.println();
  } else {
    flagloradata = false;
  }
  if (flagloradata) {
    lcd.clear();
    display(voltage, current, power, energy, frequency, pf);
  } else {
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("Warning!         ");
    lcd.setCursor(0, 2);
    lcd.print("Error LoRa data");
    for (int i = 0; i < 3; i++) {
      digitalWrite(buzzer, HIGH);
      delay(500);
      digitalWrite(buzzer, LOW);
      delay(50);
    }
  }
  delay(500); // Thêm delay để tránh nhận liên tục gây quá tải
}