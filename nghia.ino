#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <Servo.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);
int LED_G = 11;
int LED_R = 12;
int buzzer = 13;
const byte ROWS = 4;  //four rows
const byte COLS = 4;  //four columns
Servo myservo;
char keys[ROWS][COLS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};

byte rowPins[ROWS] = { 9, 8, 7, 6 };  //connect to the row pinouts of the keypad
byte colPins[COLS] = { 5, 4, 3, 2 };  //connect to the column pinouts of the keypad


Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

String enteredPassword = "";
String enteredkey = "";
String adminPassword = "1234";

bool lcdIsDisplayed = false;
bool doorLocked = false;
unsigned long lockTime = 0;
int lockDuration = 10000;  // 60s khóa
int failedAttempts = 0;
const int maxAttempts = 3;

void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  pinMode(LED_G, OUTPUT);
  pinMode(LED_R, OUTPUT);
  pinMode(buzzer, OUTPUT);
  myservo.attach(10);
  myservo.write(180);
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
void checkPassword() {  //check password keypad
  if (enteredPassword == adminPassword) {
    operateServo(90, LED_G, "Opened");
    enteredPassword = "";
    delay(2000);
    lcd.clear();
    myservo.write(180);
    failedAttempts = 0;
  } else {
    operateServo(180, LED_R, "Access denied");
    enteredPassword = "";
    lcd.clear();
    increaseFailedAttempts();
  }

  lcdIsDisplayed = false;
}

void lockDoor() {
  doorLocked = true;
  lockTime = millis();
  lcd.clear();
  lcd.print("Door locked");
}
void increaseFailedAttempts() {  // increase Failed Attempts
  failedAttempts++;

  if (failedAttempts >= maxAttempts) {
    digitalWrite(buzzer, HIGH);
    delay(500);
    digitalWrite(buzzer, LOW);
    delay(50);
    digitalWrite(buzzer, HIGH);
    delay(500);
    digitalWrite(buzzer, LOW);
    delay(50);
    digitalWrite(buzzer, HIGH);
    delay(500);
    digitalWrite(buzzer, LOW);
    delay(50);
   
    lockDoor();
  }
}
void enterOldPassword() {
  lcd.clear();
  lcd.print("Change password");

  delay(2000);
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
    delay(100);

    lcd.clear();
    lcd.print(" New password ?");
    digitalWrite(LED_G, LOW);
    delay(500);
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
int blinkCounter = 0;
void displayPassword(String password) {  // hide password

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
void clearScreen() {
  lcd.clear();
  lcdIsDisplayed = false;
  lcd.print("Cleared screen");
  delay(2000);
  lcd.clear();
  enteredPassword = "";
}
void displayWelcomeMessage() {  // Displays the standby screen
  if (!lcdIsDisplayed) {
    lcdIsDisplayed = true;
    lcd.setCursor(4, 0);
    lcd.print("WELCOME");
  }
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
void handleLockedDoor(char key) {  // Lock door
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
void handleUnlockedDoor(char key) {  // Unlock door
  if (key) {
    if (key == 'C') {
      clearScreen();
    } else if (key == 'B') {
      operateServo(180, LED_R, "Closed");
      lcdIsDisplayed = false;
    } else if (key == 'A') {
      enterOldPassword();
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
