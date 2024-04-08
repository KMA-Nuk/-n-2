#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <Servo.h>

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
byte rowPins[ROWS] = { 9, 8, 7, 6 };
byte colPins[COLS] = { 5, 4, 3, 2 };
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

int LED_G = 11;
int LED_R = 12;
int buzzer = 13;
const int maxAttempts = 3;
int lockDuration = 10000;
const int additionalLockTime = 10000;

String adminPassword = "1234";
String enteredPassword = "";
unsigned long lockTime = 0;
int failedAttempts = 0;
bool doorLocked = false;
bool lcdIsDisplayed = false;

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
      if (enteredPassword.length() == 4) {
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
    delay(1000);
    digitalWrite(LED_G, LOW);
    delay(500);

    lcd.clear();
    lcd.print(" New password ?");
    String newPassword = enterPassword();

    lcd.clear();
    lcd.print("Pass Again ?");
    String newPasswordAgain = enterPassword();

    while (newPasswordAgain != newPassword) {
      lcd.clear();
      lcd.print("Don't match");
      digitalWrite(LED_R, HIGH);
      delay(2000);
      digitalWrite(LED_R, LOW);

      lcd.clear();
      lcd.print("Pass again ?");
      newPasswordAgain = enterPassword();
    }

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
  clearScreen();
}

String enterPassword() {
  String password = "";
  while (password.length() < 4) {
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
