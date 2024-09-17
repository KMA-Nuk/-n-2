#define BLYNK_TEMPLATE_ID "TMPL6MqmJnFt9"
#define BLYNK_TEMPLATE_NAME "Congtodien"
#define BLYNK_AUTH_TOKEN "_XDp7aFByPBt5OSFDPXwOgkrouP19D4Q"
#define BLYNK_PRINT Serial
char auth[] = BLYNK_AUTH_TOKEN;

#include <WiFiClientSecure.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <PZEM004Tv30.h>
#include <LiquidCrystal_I2C.h>
#include "uRTCLib.h"
#include <Wire.h>

#include <EEPROM.h>

float csHomNay;
float csHomQua;
float csThangNay;
float csThangTruoc;
int ngayChotSo = 1;
int gioChotSo = 23, phutChotSo = 59;
unsigned long times = millis();
unsigned long lastChiSoTime = 0;

const char *ssid = "Sâu gà vl"; 
const char *password = "minhduc_no1";
// const char *ssid = "Thang";
// const char *password = "1235678";
// const char *ssid = "Yên";
// const char *password = "123567890";
// LCD and RTC setup
LiquidCrystal_I2C lcd(0x27, 20, 4);
uRTCLib rtc(0x68);
char daysOfTheWeek[7][12] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };

// PZEM configuration
#if !defined(PZEM_RX_PIN) && !defined(PZEM_TX_PIN)
#define PZEM_RX_PIN 16  // tx pzem -- rx2
#define PZEM_TX_PIN 17  // rx pzem -- tx2
#endif

#if !defined(PZEM_SERIAL)
#define PZEM_SERIAL Serial2
#endif

#if defined(ESP32)
PZEM004Tv30 pzem(PZEM_SERIAL, PZEM_RX_PIN, PZEM_TX_PIN);
#else
PZEM004Tv30 pzem(PZEM_SERIAL);
#endif

// Global variables
float energy;
bool displayPowerInfoFlag = false;
float voltage, current, power, frequency, pf;
float energyThreshold = 10.000;
float currentThreshold = 10.0;

bool alertTriggered = false;
bool alertActive = false;
boolean savedata = false;
boolean resetE = false;
int buzzer = 15;

WidgetLED led_connect(V8);
WidgetTerminal terminal(V9);
void setup() {
  // Debugging Serial port
  Serial.begin(115200);

  // WiFi connection
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  pinMode(buzzer, OUTPUT);
  // Initialize LCD and RTC
  lcd.init();
  lcd.backlight();
  lcd.clear();
  URTCLIB_WIRE.begin(21, 22);

  EEPROM.begin(512);
  delay(10);
  readChiSo();

  terminal.println("Terminal chat started!");
  terminal.flush();

  // Initialize Blynk
  Blynk.begin(auth, ssid, password);
}

void syncVirtualPins() {
  Blynk.syncVirtual(V0);
  Blynk.syncVirtual(V1);
  Blynk.syncVirtual(V2);
  Blynk.syncVirtual(V3);
  Blynk.syncVirtual(V4);
  Blynk.syncVirtual(V5);
}

void readSensorData(float &voltage, float &current, float &power, float &energy, float &frequency, float &pf) {
  voltage = pzem.voltage();
  current = pzem.current();
  power = pzem.power();
  energy = pzem.energy();
  frequency = pzem.frequency();
  pf = pzem.pf();
}

void writeVirtualPins(float voltage, float current, float power, float energy, float frequency, float pf) {
  Blynk.virtualWrite(V0, voltage);
  Blynk.virtualWrite(V1, current);
  Blynk.virtualWrite(V2, power);
  Blynk.virtualWrite(V3, energy);
  Blynk.virtualWrite(V4, pf);
  Blynk.virtualWrite(V5, frequency);
}

bool checkValidData(float voltage, float current, float power, float energy, float frequency, float pf) {
  if (isnan(voltage) || isnan(current) || isnan(power) || isnan(energy) || isnan(frequency) || isnan(pf)) {
    Serial.println("Error reading sensor data");
    for (int i = 0; i < 3; i++) {
      digitalWrite(buzzer, HIGH);
      delay(500);
      digitalWrite(buzzer, LOW);
      delay(50);
    }
    return false;
  }
  return true;
}

void printData(float voltage, float current, float power, float energy, float frequency, float pf) {
  Serial.print("Voltage: ");
  Serial.print(voltage);
  Serial.println("V");
  Serial.print("Current: ");
  Serial.print(current);
  Serial.println("A");
  Serial.print("Power: ");
  Serial.print(power);
  Serial.println("W");
  Serial.print("Energy: ");
  Serial.print(energy, 3);
  Serial.println("kWh");
  Serial.print("Frequency: ");
  Serial.print(frequency, 1);
  Serial.println("Hz");
  Serial.print("PF: ");
  Serial.println(pf);
}

// Function to refresh the RTC time
void refreshRTC() {
  rtc.refresh();
}

float calculateElectricityCost(float energy) {
  float cost = 0.0;
  if (energy <= 50) {
    cost = energy * 1806;
  } else if (energy <= 100) {
    cost = 50 * 1806 + (energy - 50) * 1866;
  } else if (energy <= 200) {
    cost = 50 * 1806 + 50 * 1866 + (energy - 100) * 2167;
  } else if (energy <= 300) {
    cost = 50 * 1806 + 50 * 1866 + 100 * 2167 + (energy - 200) * 2729;
  } else if (energy <= 400) {
    cost = 50 * 1806 + 50 * 1866 + 100 * 2167 + 100 * 2729 + (energy - 300) * 3050;
  } else {
    cost = 50 * 1806 + 50 * 1866 + 100 * 2167 + 100 * 2729 + 100 * 3050 + (energy - 400) * 3151;
  }
  return cost / 1000;
}


// Function to display the date on the LCD
void display1(float energy) {
  lcd.setCursor(0, 0);
  lcd.print(daysOfTheWeek[rtc.dayOfWeek() - 1]);
  lcd.print(":");
  lcd.print(" ");
  lcd.print(rtc.day());
  lcd.print("/");
  lcd.print(rtc.month());
  lcd.print("/");
  lcd.print(rtc.year());

  lcd.setCursor(0, 1);
  if (rtc.hour() >= 12) {
    lcd.print("PM : ");
    lcd.setCursor(5, 1);
    lcd.print(rtc.hour() > 12 ? rtc.hour() - 12 : rtc.hour());
  } else {
    lcd.print("AM : ");
    lcd.setCursor(5, 1);
    lcd.print(rtc.hour());
  }
  lcd.print(":");
  lcd.print(rtc.minute());
  lcd.print(":");
  lcd.print(rtc.second());

  float cost = calculateElectricityCost(energy);
  lcd.setCursor(0, 2);
  lcd.print("Tieu thu :");
  lcd.print(energy, 3);
  lcd.setCursor(16, 2);
  lcd.print("kWh");

  lcd.setCursor(0, 3);
  lcd.print("Tien dien:");
  lcd.setCursor(10, 3);
  lcd.print(cost);
  lcd.setCursor(16, 3);
  lcd.print("kVND");
}

void display2(float voltage, float current, float power, float energy, float frequency, float pf) {
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
//bool alertTriggered = false;
void activateAlert() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(buzzer, HIGH);
    delay(500);
    digitalWrite(buzzer, LOW);
    delay(50);
  }
  lcd.clear();
  lcd.setCursor(6, 0);
  lcd.print("ALERT!");
  lcd.setCursor(1, 1);
  if (energy >= energyThreshold) {
    lcd.print("Energy vuot nguong");
  }
  if (current >= currentThreshold) {
    lcd.print("Curr vuot nguong");
  }
  delay(1500);
  displayPowerInfoFlag = true;
  // alertActive = false;
  // alertTriggered = true;
}

void loop() {
  Blynk.run();
  syncVirtualPins();
  if (millis() - times > 5000) {
    times = millis();
    readSensorData(voltage, current, power, energy, frequency, pf);
    writeVirtualPins(voltage, current, power, energy, frequency, pf);
  }
  // if (checkValidData(voltage, current, power, energy, frequency, pf)) {
  //   printData(voltage, current, power, energy, frequency, pf);
  // }
  checkValidData(voltage, current, power, energy, frequency, pf);
  refreshRTC();

  if (displayPowerInfoFlag) {
    lcd.clear();
    display2(voltage, current, power, energy, frequency, pf);
  } else {
    lcd.clear();
    display1(energy);
  }

  if ((energy >= energyThreshold || current >= currentThreshold)) {
    if (!alertTriggered) {
      //alertActive = true;
      activateAlert();        // Kích hoạt cảnh báo
      alertTriggered = true;  // Đánh dấu cảnh báo đã được kích hoạt
    }
  } else {
    // Nếu giá trị đã hạ xuống dưới ngưỡng, đặt lại cờ cảnh báo
    alertTriggered = false;
    //alertActive = false;
  }

  if (resetE == true) {
    resetE = false;
    //pzem.resetEnergy();
    for (int i = 0; i < 16; ++i) {
      EEPROM.write(i, 0);
      delay(10);
      //digitalWrite(led, !digitalRead(led));
    }
    EEPROM.commit();
    readChiSo();
  }
  if (savedata == true) {
    savedata = false;
    writeChiSo();
  }
  blinkled();
  if (millis() - lastChiSoTime > 10000) {
    lastChiSoTime = millis();
    writeChiSoToTerminal();  // Ghi các chỉ số lên Terminal mỗi 5 giây
  }
  Serial.println();
  //delay(1000);
}

//-------------Ghi dữ liệu kiểu float vào bộ nhớ EEPROM----------------------//
float readFloat(unsigned int addr) {
  union {
    byte b[4];
    float f;
  } data;

  for (int i = 0; i < 4; i++) {
    data.b[i] = EEPROM.read(addr + i);
  }

  return data.f;
}

void writeFloat(unsigned int addr, float x) {
  union {
    byte b[4];
    float f;
  } data;

  data.f = x;

  for (int i = 0; i < 4; i++) {
    EEPROM.write(addr + i, data.b[i]);
  }

  EEPROM.commit();  // Commit changes to EEPROM
}


void readChiSo() {
  csHomNay = readFloat(0);
  csHomQua = readFloat(4);
  csThangNay = readFloat(8);
  csThangTruoc = readFloat(12);

  if (isnan(csHomNay)) csHomNay = 0.0;
  if (isnan(csHomQua)) csHomQua = 0.0;
  if (isnan(csThangNay)) csThangNay = 0.0;
  if (isnan(csThangTruoc)) csThangTruoc = 0.0;

  Serial.print("Chỉ số hôm nay: ");
  Serial.println(csHomNay);
  Serial.print("Chỉ số hôm qua: ");
  Serial.println(csHomQua);
  Serial.print("Chỉ số tháng này: ");
  Serial.println(csThangNay);
  Serial.print("Chỉ số tháng trước: ");
  Serial.println(csThangTruoc);
}

void writeChiSoToTerminal() {
  terminal.println("-------------------");
  terminal.print("Chỉ số hôm nay: ");
  terminal.println(csHomNay);
  terminal.print("Chỉ số hôm qua: ");
  terminal.println(csHomQua);
  terminal.println("                    ");
  terminal.print("Chỉ số tháng này: ");
  terminal.println(csThangNay);
  terminal.print("Chỉ số tháng trước: ");
  terminal.println(csThangTruoc);
  // terminal.print("Ngưỡng ");
  // terminal.print("Energy: ");
  // terminal.print(energyThreshold);
  // terminal.print("   ||   ");
  // terminal.print("Current: ");
  // terminal.print(currentThreshold);
  terminal.flush();
  terminal.println("-------------------");
}

void writeChiSo() {
  if (rtc.hour() == gioChotSo && rtc.minute() == phutChotSo) {
    Serial.println("Ghi số ngày mới!");
    writeFloat(0, energy);    // Ghi giá trị hiện tại vào địa chỉ 0
    writeFloat(4, csHomNay);  // Ghi giá trị của hôm nay vào địa chỉ 4
    //writeFloat(4, csHomNay);

    if (rtc.day() == ngayChotSo) {
      Serial.println("Ghi số tháng mới!");
      writeFloat(8, energy);       // Ghi giá trị hiện tại vào địa chỉ 8
      writeFloat(12, csThangNay);  // Ghi giá trị của tháng này vào địa chỉ 12
    }
    EEPROM.commit();
    readChiSo();  // Đọc lại các giá trị từ EEPROM để kiểm tra
  }
}
void saveData() {
  savedata = true;
}

//-------------Ghi dữ liệu kiểu float vào bộ nhớ EEPROM----------------------//
BLYNK_WRITE(V6) {
  int p = param.asInt();
  if (p == 1) {
    resetE = true;
    pzem.resetEnergy();  // Reset energy on the PZEM device
                         // energy = 0;         // Reset local energy variable
    Serial.println("Energy has been reset to 0.");
  }
}

BLYNK_WRITE(V7) {
  int p = param.asInt();
  if (p == 1) {
    displayPowerInfoFlag = true;  // Show power consumption and cost
  } else {
    displayPowerInfoFlag = false;  // Show date and time
  }
}

BLYNK_WRITE(V9) {
  String text = param.asStr();  // Lấy văn bản từ terminal
  Serial.println(text);

  // Kiểm tra và cập nhật ngưỡng từ input của người dùng
  if (text.startsWith("setEnergy")) {
    String valueStr = text.substring(10);  // Lấy giá trị sau "setEnergy "
    valueStr.trim();                       // Loại bỏ khoảng trắng đầu và cuối
    float value = valueStr.toFloat();      // Chuyển đổi chuỗi thành số thực
    if (value >= 0) {                      // Kiểm tra giá trị hợp lệ
      energyThreshold = value;
      terminal.println("Energy threshold set to: " + String(energyThreshold, 3) + " kWh");  // Hiển thị với 3 chữ số thập phân
    } else {
      terminal.println("Invalid energy threshold value.");
    }
  } else if (text.startsWith("setCurr")) {
    String valueStr = text.substring(8);  // Lấy giá trị sau "setCurr "
    valueStr.trim();                      // Loại bỏ khoảng trắng đầu và cuối
    float value = valueStr.toFloat();     // Chuyển đổi chuỗi thành số thực
    if (value >= 0) {                     // Kiểm tra giá trị hợp lệ
      currentThreshold = value;
      terminal.println("Current threshold set to: " + String(currentThreshold, 2) + " A");  // Hiển thị với 2 chữ số thập phân
    } else {
      terminal.println("Invalid current threshold value.");
    }
  } else {
    terminal.print("You typed: ");
    terminal.println(text);
  }
  terminal.flush();
}


void blinkled() {
  if (led_connect.getValue()) {
    led_connect.off();
  } else {
    led_connect.on();
  }
}