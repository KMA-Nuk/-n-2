#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

#define LED_G 15
#define LED_R 1
#define SS_PIN 5   /*Slave Select Pin*/
#define RST_PIN 34 /*Reset Pin for RC522*/

#define BLYNK_TEMPLATE_ID "TMPL6LVcP-rK8"
#define BLYNK_TEMPLATE_NAME "smartled"
#define BLYNK_AUTH_TOKEN "NJXmWXA3V297Ec7hzHmbNQN8tWfbL6Y3"
#define BLYNK_PRINT Serial
#include <BlynkSimpleEsp32.h>
#include <WiFi.h>
#include <WiFiClient.h>
const char* ssid = "Yên";
const char* password = "123567890";
char auth[] = BLYNK_AUTH_TOKEN;

Servo myservo;
MFRC522 mfrc522(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);

const byte ROWS = 4;  //four rows
const byte COLS = 4;  //four columns

char keys[ROWS][COLS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};

byte rowPins[ROWS] = { 13, 12, 14, 27 };  // Chân GPIO kết nối với các hàng của bàn phím
byte colPins[COLS] = { 26, 25, 33, 32 };  // Chân GPIO kết nối với các cột của bàn phím

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

String enteredPassword = "";
String enteredkey = "";
String adminPassword = "1234";
bool lcdIsDisplayed = false;
bool isChangingPasswordMode = false;
bool isChangingPasswordMode1 = false;

bool doorLocked = false;
unsigned long lockTime = 0;
const unsigned long lockDuration = 60000;  // 60s khóa
int failedAttempts = 0;
const int maxAttempts = 3;

#include <Adafruit_Fingerprint.h>
HardwareSerial fingerSerial(2);  // Khởi tạo đối tượng serial với chân RX: GPIO16, TX: GPIO17
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&fingerSerial);
uint16_t id;

uint16_t readnumber(void) {
  uint16_t num = 0;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter ID (1-127): ");

  while (true) {
    char key = keypad.getKey();

    if (key) {
      if (key >= '0' && key <= '9') {
        num = num * 10 + (key - '0');
        lcd.setCursor(5, 1);
        enteredkey += key;
        lcd.print(enteredkey);
        delay(500);
      } else if (key == '*') {
        if (num >= 1 && num <= 127) {
          //num = 0;  // Reset giá trị nếu ID không hợp lệ
          enteredkey = "";
          // ID hợp lệ, thoát khỏi vòng lặp
          break;
        } else {
          lcd.clear();
          lcd.print("Error !");
          lcd.setCursor(0, 1);
          lcd.print("Invalid ID");
          delay(2000);
          lcd.clear();
          num = 0;          // Reset giá trị nếu ID không hợp lệ
          enteredkey = "";  // Xóa giá trị nhập vào nếu ID không hợp lệ
          id = readnumber();
        }
      } else if (key == 'C') {
        clearScreen();
        return 0;
      }
    }
    delay(50);  // Đợi một khoảng thời gian trước khi kiểm tra phím tiếp theo
  }
  return num;
}

void operateServo(int pos, int ledPin, const char* statusMessage) {
  myservo.write(pos);
  digitalWrite(ledPin, HIGH);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(statusMessage);
  delay(2000);
  digitalWrite(ledPin, LOW);
  lcd.clear();
}

void clearScreen() {
  lcd.clear();
  lcdIsDisplayed = false;
  lcd.print("Cleared screen");
  delay(2000);
  lcd.clear();
  enteredPassword = "";
}

void checkRFID() {
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  /*Select Card*/
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }
  /*Show UID for Card/Tag on serial monitor*/
  Serial.print("UID tag :");
  String content = "";
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  Serial.println();
  Serial.print("Message : ");
  content.toUpperCase();

  if (content.substring(1) == "95 2D A1 53") {
    Serial.println("Authorized access"); /*Print message if UID match with the database*/
    Serial.println();
    operateServo(90, LED_G, "Access accepted");
    delay(2000);
    myservo.write(180);
    lcdIsDisplayed = false;
    failedAttempts = 0;
  } else {
    operateServo(180, LED_R, "Access denied");
    increaseFailedAttempts();
    //buzzError();
  }
}

void setup() {
  Serial.begin(115200);

  SPI.begin();
  mfrc522.PCD_Init();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(";");
  }
  Serial.println("Viola, Connected !!!");
  Blynk.begin(auth, ssid, password);

  pinMode(LED_G, OUTPUT);
  pinMode(LED_R, OUTPUT);
  myservo.attach(2);
  myservo.write(180);
  lcd.init();
  lcd.backlight();
  lcd.clear();

  finger.begin(57600);
  fingerSerial.begin(57600, SERIAL_8N1, 16, 17);
}

void enterOldPassword() {
  lcd.clear();
  lcd.print("  Old password ?");
  String enteredOldPassword = "";
  while (enteredOldPassword.length() < 4) {
    char key = keypad.getKey();
    displayPassword(enteredOldPassword);
    if (key) {
      if (key == 'C') {
        clearScreen();
        return;
      }
      enteredOldPassword += key;
    }
  }

  if (enteredOldPassword == adminPassword) {
    lcd.clear();
    lcd.print("    Correct ");
    digitalWrite(LED_G, HIGH);
    delay(500);
    digitalWrite(LED_G, LOW);
    delay(500);
    lcd.clear();
    lcd.print("  New password");

    String newPassword = "";

    while (newPassword.length() < 4) {
      char key = keypad.getKey();
      displayPassword(newPassword);
      if (key) {
        if (key == 'C') {
          clearScreen();
          return;
        }
        newPassword += key;
      }
    }

    adminPassword = newPassword;
    lcd.clear();
    lcd.print("Password changed");
    digitalWrite(LED_G, HIGH);
    delay(2000);
    digitalWrite(LED_G, LOW);
    lcd.clear();
    lcdIsDisplayed = false;
  } else {
    lcd.clear();
    lcd.print("Error !");
    digitalWrite(LED_R, HIGH);
    delay(2000);
    digitalWrite(LED_R, LOW);
    lcd.clear();
    enterOldPassword();
  }
}

void scanRFIDCard() {
  lcd.clear();
  lcd.print("Scan RFID card");

  while (!mfrc522.PICC_IsNewCardPresent()) {
    char key = keypad.getKey();
    if (key == 'C') {
      clearScreen();
      return;
    }
  }

  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  String content = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }

  content.toUpperCase();

  if (content.substring(1) == "95 2D A1 53") {
    lcd.clear();
    lcd.print("   Correct ");
    digitalWrite(LED_G, HIGH);
    delay(500);
    digitalWrite(LED_G, LOW);
    delay(500);
    lcd.clear();
    lcd.print("  New password");

    String newPassword = "";

    while (newPassword.length() < 4) {
      char key = keypad.getKey();
      displayPassword(newPassword);
      if (key) {
        if (key == 'C') {
          clearScreen();
          return;
        }
        newPassword += key;
      }
    }

    adminPassword = newPassword;
    lcd.clear();
    lcd.print("Password changed");
    digitalWrite(LED_G, HIGH);
    delay(2000);
    digitalWrite(LED_G, LOW);
    lcd.clear();
    lcdIsDisplayed = false;
  } else {
    lcd.clear();
    lcd.print("RFID error");
    digitalWrite(LED_R, HIGH);
    delay(2000);
    digitalWrite(LED_R, LOW);
    lcd.clear();
  }
}

void lockDoor() {
  doorLocked = true;
  lockTime = millis();
  lcd.clear();
  lcd.print("Door locked");
}

void increaseFailedAttempts() { // increase Failed Attempts
  failedAttempts++;
  if (failedAttempts >= maxAttempts) {
    lockDoor();
  }
}

void checkPassword() { //check password keypad
  if (enteredPassword == adminPassword) {
    operateServo(90, LED_G, "Opened");
    enteredPassword = "";
    delay(2000);
    lcd.clear();
    myservo.write(180);
    //unlockDoor();
    failedAttempts = 0;
  } else {
    operateServo(180, LED_R, "Access denied");
    enteredPassword = "";
    lcd.clear();
    increaseFailedAttempts();
    // failedAttempts++;
    // if (failedAttempts >= maxAttempts) {
    //   lockDoor();
    // }
  }

  lcdIsDisplayed = false;
}

void displayWelcomeMessage() { // Displays the standby screen
  if (!lcdIsDisplayed) {
    lcdIsDisplayed = true;
    lcd.setCursor(4, 0);
    lcd.print("WELCOME");
  }
}

int blinkCounter = 0;
void displayPassword(String password) { // hide password

  lcd.setCursor(6, 1);
  for (int i = 0; i < password.length(); i++) {
    lcd.print('*');
  }

  // Hiển thị dấu "_" ở cuối nếu đã nhập ít nhất một số
  if (password.length() > 0) {
    if (blinkCounter < 30) {  // Nhấp nháy trong khoảng thời gian nào đó
      lcd.print('_');
    } else {
      lcd.print(' ');  // Không hiển thị ký tự "_"
    }

    blinkCounter++;
    if (blinkCounter == 60) {
      blinkCounter = 0;  // Đặt lại biến đếm để bắt đầu lại quá trình nhấp nháy
    }
  }
}

void processKeyPressD() { // button D - Fingerprint settings
  lcd.clear();
  lcd.print(" Fingerprint");
  lcd.setCursor(2, 1);
  lcd.print(" settings");
  delay(1500);
  lcd.clear();
  lcd.print("1. Add new");
  lcd.setCursor(0, 1);
  lcd.print("2. Delete");
  isChangingPasswordMode1 = true;
  delay(500);
}

void processKeyPress11() {  // add fingerprint
  id = readnumber();

  if (id == 0) {
    return;  // ID #0 not allowed, try again!
  }

  while (!getFingerprintEnroll())
    ;
  isChangingPasswordMode1 = false;
}

void processKeyPress22() {  // delete fingerprint
  lcd.clear();
  finger.getTemplateCount();
  if (finger.templateCount == 0) {
    lcd.print("No fingerprints");
  } else {
    // Serial.println("Waiting for valid finger...");
    lcd.setCursor(0, 0);
    lcd.print("Sensor contains ");
    lcd.setCursor(0, 1);
    lcd.print(finger.templateCount);
    lcd.print(" templates");
  }
  delay(2000);

  id = readnumber();
  if (id == 0) {  // ID #0 not allowed, try again!
    return;
  }

  deleteFingerprint(id);
  isChangingPasswordMode1 = false;
}

void processKeyPressA() { // button A - password settings
  lcd.clear();
  lcd.print("Change password ?");
  delay(1500);
  lcd.clear();
  lcd.print("1.Old password");
  lcd.setCursor(0, 1);
  lcd.print("2.Scan RFID card");
  isChangingPasswordMode = true;
  delay(500);
}

void processKeyPress1() {  // change password - previous password
  enterOldPassword();
  isChangingPasswordMode1 = false;
}

void processKeyPress2() {  // change password - scan Rfid
  scanRFIDCard();
  isChangingPasswordMode1 = false;
}

void loop() {
  char key = keypad.getKey();
  displayPassword(enteredPassword);

  if (doorLocked) {
    // Cửa đang bị khóa, xử lý các tình huống liên quan đến việc cửa đóng
    handleLockedDoor(key);
  } else {
    // Cửa không bị khóa, xử lý các tình huống liên quan đến việc cửa mở
    handleUnlockedDoor(key);
  }
}

void handleLockedDoor(char key) { // Lock door
  unsigned long elapsedTime = millis() - lockTime;
  if (elapsedTime >= lockDuration) {
    doorLocked = false;
    lcd.clear();
    lcdIsDisplayed = false;
  } else {
    lcd.clear();
    lcd.print("Door locked");
    lcd.setCursor(0, 1);
    lcd.print("Time : " + String((lockDuration - elapsedTime) / 1000) + "s");
    delay(1000);
  }
}

void handleUnlockedDoor(char key) { // Unlock door
  Blynk.run();
  checkRFID();

  if (isChangingPasswordMode) {
    // Nếu đang trong chế độ thay đổi mật khẩu 1
    if (key != '\0') {
      if (key == '1') {
        processKeyPress1();
      } else if (key == '2') {
        processKeyPress2();
      }
      // if (key == 'C') {
      //   clearScreen();
      // } 
      else {
        lcd.clear();
        lcd.print("Invalid choice");
        delay(2000);
        lcd.clear();
        processKeyPressA();
      }
      isChangingPasswordMode = false;
    }
  } else if (isChangingPasswordMode1) {
    // Nếu đang trong chế độ thay đổi mật khẩu 2
    if (key != '\0') {
      if (key == '1') {
        processKeyPress11();
      } else if (key == '2') {
        processKeyPress22();
      }
      // if (key == 'C') {
      //   clearScreen();
      // } 
      else {
        lcd.clear();
        lcd.print("Invalid choice");
        delay(2000);
        lcd.clear();
        processKeyPressD();
      }
      isChangingPasswordMode1 = false;
    }
  } else {
    // Không phải chế độ thay đổi mật khẩu, xử lý các tác động khác
    if (key) {
      if (key == 'C') {
        clearScreen();
      } else if (key == 'B') {
        operateServo(180, LED_R, "Closed");
        lcdIsDisplayed = false;
      } else if (key == 'A') {
        processKeyPressA();
      } else if (key == '#') {
        continuousFingerprintScan();
      } else if (key == 'D') {
        processKeyPressD();
      } else {
        enteredPassword += key;
        lcd.clear();
        lcd.setCursor(4, 0);
        lcd.print("Password ");
        if (enteredPassword.length() == 4) {
          checkPassword();
        }
      }
    } else {
      displayWelcomeMessage();
    }
  }
}

bool fingerprintScanSuccess = false;
void continuousFingerprintScan() {
  lcd.clear();
  while (true) {
    lcd.setCursor(0, 0);
    lcd.print("Scanning...");
    getFingerprintID();
    char key = keypad.getKey();

    if (key) {
      if (key == 'C') {
        clearScreen();
        return;  // Thoát khỏi hàm nếu phím 'C' được nhấn
      }
    } else {
      if (fingerprintScanSuccess == true) {
        fingerprintScanSuccess = false;
        break;  // Thoát khỏi vòng lặp nếu quét vân tay thành công
      }
    }
    if (failedAttempts >= maxAttempts) {
      lockDoor();
      break;
    }
    delay(500);  // Đợi một khoảng thời gian trước khi quét lại
  }
}

uint8_t getFingerprintID() {

  uint8_t p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      //lcd.print("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      //lcd.print("No finger detected");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      lcd.clear();
      lcd.print("Communication error");
      return p;
    case FINGERPRINT_IMAGEFAIL:
      lcd.clear();
      lcd.print("Imaging error");
      return p;
    default:
      lcd.clear();
      lcd.print("Unknown error");
      return p;
  }

  // OK success!

  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      //lcd.print("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      lcd.clear();
      lcd.print("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      lcd.clear();
      lcd.print("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      lcd.print("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      lcd.print("Could not find fingerprint features");
      return p;
    default:
      lcd.print("Unknown error");
      return p;
  }

  // OK converted!
  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    operateServo(90, LED_G, "Opened");
    lcd.clear();
    delay(3000);
    myservo.write(180);
    failedAttempts = 0;
    lcdIsDisplayed = false;
    fingerprintScanSuccess = true;
    // value1 = finger.fingerID;
    // Serial.println("Found a print match!");
    // delay(2000);
    // sendDataToSheet();
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    lcd.print("Communication error");
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    lcd.clear();
    operateServo(180, LED_R, "Access denied");
    failedAttempts++;

  } else {
    lcd.print("Unknown error");
    return p;
  }

  // found a match!
  Serial.print("Found ID #");
  Serial.print(finger.fingerID);
  Serial.print(" with confidence of ");
  Serial.println(finger.confidence);

  return finger.fingerID;
  //}
}

int getFingerprintIDez() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK) return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK) return -1;

  // found a match!
  Serial.print("Found ID #");
  Serial.print(finger.fingerID);
  Serial.print(" with confidence of ");
  Serial.println(finger.confidence);
  return finger.fingerID;
}

uint8_t getFingerprintEnroll() {
  int p = -1;
  // lcd.print("Waiting for valid finger to enroll as #");
  // lcd.print(id);
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:  // hình ảnh vân tay đã được lấy thành công, thông báo "Image taken" được in ra trên Serial Monitor
        lcd.clear();
        lcd.print("Image taken");
        break;
      case FINGERPRINT_NOFINGER:  // tức là không có ngón tay nào được đặt lên cảm biến, thông báo "." (dấu chấm) được in ra trên Serial Monitor
        //lcd.print(".");
        break;
      case FINGERPRINT_PACKETRECIEVEERR:  // tức là có lỗi truyền thông giữa cảm biến và Arduino, thông báo "Communication error" được in ra trên Serial Monitor.
        lcd.clear();
        lcd.print("Communication error");
        break;
      case FINGERPRINT_IMAGEFAIL:  //  tức là có lỗi trong quá trình lấy hình ảnh vân tay, thông báo "Imaging error" được in ra trên Serial Monitor.
        lcd.print("Imaging error");
        break;
      default:  // Nếu giá trị trả về không thuộc các trường hợp trên, tức là có lỗi không xác định, thông báo "Unknown error" được in ra trên Serial Monitor.
        lcd.print("Unknown error");
        break;
    }
  }
  // lấy hình ảnh vân tay -> chờ người dùng rút ngón tay ra khỏi cảm biến -> chuyển đổi hình ảnh vân tay thành mẫu (template) để lưu trữ
  //
  // OK success!
  p = finger.image2Tz(1);  // Hàm image2Tz() được gọi để chuyển đổi hình ảnh vân tay thành mẫu
  switch (p) {
    case FINGERPRINT_OK:  //  tức là việc chuyển đổi thành công và mẫu được tạo ra
      //lcd.print("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      lcd.print("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      lcd.clear();
      lcd.print("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      lcd.print("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      lcd.print("Could not find fingerprint features");
      return p;
    default:
      lcd.print("Unknown error");
      return p;
  }
  lcd.clear();
  lcd.setCursor(0, 0);  //cột 5, hàng đầu
  lcd.print("Remove finger");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  Serial.print("ID ");
  Serial.println(id);
  p = -1;
  lcd.clear();
  lcd.setCursor(0, 0);  //cột 5, hàng đầu
  lcd.print("Please re-enter");
  lcd.setCursor(0, 1);
  lcd.print("your fingerprint");

  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:
        //lcd.print("Image taken");
        break;
      case FINGERPRINT_NOFINGER:
        //lcd.print(".");
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        lcd.clear();
        lcd.print("Communication error");
        break;
      case FINGERPRINT_IMAGEFAIL:
        lcd.print("Imaging error");
        break;
      default:
        lcd.print("Unknown error");
        break;
    }
  }

  // OK success!

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  Serial.print("Creating model for #");
  Serial.println(id);

  //Sau khi đã tạo mẫu từ hình ảnh vân tay, tiếp theo là thực hiện việc lưu trữ thông tin vân tay này vào bộ nhớ của cảm biến.
  p = finger.createModel();   // được gọi để tạo ra một "model" (mô hình) từ mẫu vân tay đã tạo
  if (p == FINGERPRINT_OK) {  //  tức là việc tạo model thành công và thông tin vân tay đã được lưu trữ trong bộ nhớ của cảm biến,
    Serial.println("Prints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  Serial.print("ID ");
  Serial.println(id);
  p = finger.storeModel(id);  // được gọi để lưu trữ thông tin vân tay đã được tạo ra và lưu trữ vào vị trí được chỉ định bởi biến id
  if (p == FINGERPRINT_OK) {  //  tức là việc lưu trữ thành công
    lcd.clear();
    lcd.setCursor(1, 0);  //cột 5, hàng đầu
    lcd.print("ID #");
    lcd.print(id);
    lcd.print(" Stored!");
    delay(2000);
    lcd.clear();
    lcdIsDisplayed = false;
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    lcd.clear();
    lcd.print("Error comm");
    delay(2000);
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  return true;
}

uint8_t deleteFingerprint(uint16_t id) {
  uint8_t p = -1;

  p = finger.deleteModel(id);

  if (p == FINGERPRINT_OK) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Deleted! ID:"); 
    lcd.print(id); 
    lcdIsDisplayed = false;
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not delete in that location");
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
  } else {
    Serial.print("Unknown error: 0x");
    Serial.println(p, HEX);
  }

  return p;
}

BLYNK_WRITE(V1) {
  int p = param.asInt();
  if (p == 1) {
    operateServo(90, LED_G, "Opened");
    delay(2000);
    myservo.write(180);
    failedAttempts = 0;
    lcdIsDisplayed = false;
  } else {
    operateServo(180, LED_R, "Closed");
    lcdIsDisplayed = false;
  }
}