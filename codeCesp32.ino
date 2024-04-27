#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <ESP32Servo.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo myservo;

const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};

byte rowPins[ROWS] = { 13, 12, 14, 27 };
byte colPins[COLS] = { 26, 25, 33, 32 };
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

int LED_G = 15;
int LED_R = 2;
int buzzer = 5;

const int maxAttempts = 3;
int lockDuration = 10000;
const int additionalLockTime = 10000;

String adminPassword = "1234";
String enteredPassword = "";
unsigned long lockTime = 0;
int failedAttempts = 0;
bool doorLocked = false;
bool lcdIsDisplayed = false;

int passwordLength = 4;

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  pinMode(LED_G, OUTPUT);
  pinMode(LED_R, OUTPUT);
  pinMode(buzzer, OUTPUT);
  myservo.attach(4);
  myservo.write(180);
}

void loop() {

  char key = keypad.getKey();
  displayPassword(enteredPassword);

  if (doorLocked) {
    handleLockedDoor(key);
  } else {
    handleUnlockedDoor(key);
  }
}
void handleLockedDoor(char key) {
  unsigned long elapsedTime = millis() - lockTime;
  if (elapsedTime >= lockDuration) {
    doorLocked = false;
    lcd.clear();
    lcdIsDisplayed = false;
  } else {
    lcd.clear();
    lcd.print("Door locked");
    lcd.setCursor(0, 1);
    lcd.print("Time: " + String((lockDuration - elapsedTime) / 1000) + "s");
    delay(1000);
  }
}

void handleUnlockedDoor(char key) {
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
      if (enteredPassword.length() == passwordLength) {
        checkPassword();
      }
    }
  } else {
    displayWelcomeMessage();
  }
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

void checkPassword() {
  if (enteredPassword == adminPassword) {
    operateServo(130, LED_G, "Opened");
    enteredPassword = "";
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

void increaseFailedAttempts() {
  failedAttempts++;

  if (failedAttempts >= 4) {
    lockDuration += additionalLockTime;
  }

  if (failedAttempts >= maxAttempts) {
    for (int i = 0; i < 3; i++) {
      digitalWrite(buzzer, HIGH);
      delay(500);
      digitalWrite(buzzer, LOW);
      delay(50);
    }
    lockDoor();
  }
}

void lockDoor() {
  doorLocked = true;
  lockTime = millis();
  lcd.clear();
  lcd.print("Door locked");
}
char previouslenght;
int attempts = 3;

void enterOldPassword() {
  lcd.clear();
  lcd.print("Change password");
  delay(2000);
  lcd.clear();
  lcd.print("  Old password ?");
  String enteredOldPassword = "";
  while (enteredOldPassword.length() < passwordLength) {
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
    delay(1000);
    digitalWrite(LED_G, LOW);
    delay(500);

    changelengthPassword();

    lcd.clear();
    lcd.print(" New password ?");
    String newPassword = enterPassword();

    lcd.clear();
    lcd.print("Pass Again ?");
    String newPasswordAgain = enterPassword();
    bool error = false;
    while (newPasswordAgain != newPassword) {
      attempts--;
      lcd.clear();
      lcd.print("Don't match");
      digitalWrite(LED_R, HIGH);
      lcd.setCursor(2, 1);
      lcd.print(attempts);
      lcd.print(" time left");
      delay(2000);
      digitalWrite(LED_R, LOW);

      if (attempts < 1) {
        error = true;
        break;
      }

      lcd.clear();
      lcd.print("Pass again ?");
      newPasswordAgain = enterPassword();
    }
    if (error) {
      for (int i = 0; i < 3; i++) {
        digitalWrite(buzzer, HIGH);
        delay(500);
        digitalWrite(buzzer, LOW);
        delay(50);
      }
      attempts = 3;
      passwordLength = previouslenght;
      clearScreen();
      return;
    }

    attempts = 3;

    adminPassword = newPassword;
    lcd.clear();
    lcd.print("Password changed");
    digitalWrite(LED_G, HIGH);
    delay(2000);
    digitalWrite(LED_G, LOW);
  } else {
    lcd.clear();
    lcd.print("Error !");
    digitalWrite(LED_R, HIGH);
    delay(2000);
    digitalWrite(LED_R, LOW);
    enterOldPassword();
  }
  previouslenght = passwordLength;
  clearScreen();
}

void changelengthPassword() {
  lcd.clear();
  lcd.print("Length Pass ?");
  bool error = false;
  char key;
  do {
    key = keypad.getKey();
    if (key) {
      if (key == 'C') {
        clearScreen();
        return;
      } else if (key == '4' || key == '5' || key == '6' || key == '7' || key == '8') {
        passwordLength = key - '0';
        Serial.println(passwordLength);
        lcd.setCursor(4, 1);
        lcd.print(passwordLength);
        delay(1000);
        break;
      } else {
        lcd.clear();
        lcd.print("Error length !");
        delay(1000);
        error = true;
        break;
      }
    }
  } while (true);
  if (error) {
    changelengthPassword();
  }
}

String enterPassword() {
  String password = "";
  while (password.length() < passwordLength) {
    char key = keypad.getKey();
    displayPassword(password);
    if (key) {
      if (key == 'C') {
        clearScreen();
        return "";
      }
      password += key;
    }
  }
  return password;
}



int blinkCounter = 0;
void displayPassword(String password) {
  lcd.setCursor(6, 1);
  for (int i = 0; i < password.length(); i++) {
    lcd.print('*');
  }

  if (password.length() > 0) {
    if (blinkCounter < 30) {
      lcd.print('_');
    } else {
      lcd.print(' ');
    }

    blinkCounter++;
    if (blinkCounter == 60) {
      blinkCounter = 0;
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

void displayWelcomeMessage() {
  if (!lcdIsDisplayed) {
    lcdIsDisplayed = true;
    lcd.setCursor(4, 0);
    lcd.print("WELCOME");
  }
}