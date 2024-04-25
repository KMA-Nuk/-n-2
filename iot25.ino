#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <BlynkSimpleEsp32.h>
#include <Stepper.h>

#define BLYNK_TEMPLATE_ID "TMPL6pM69YnPE"
#define BLYNK_TEMPLATE_NAME "IOT2"
#define BLYNK_AUTH_TOKEN "y509DhGi2LN8uWna9WeKhk47dZusWKkc"
#define BLYNK_PRINT Serial
char auth[] = BLYNK_AUTH_TOKEN;

const char* ssid = "nooooo";
const char* password = "nghiaaaa";

const int analogPin_AS = 35;
const int threshold = 2078;
const int UV_SENSOR_PIN = 34;
const int stepsPerRevolution = 2048;
Stepper myStepper(stepsPerRevolution, 23, 18, 22, 5);
Stepper myStepper2(stepsPerRevolution, 13, 14, 12, 27);

#define ADC_VREF_mV 5000.0
#define ADC_RESOLUTION 4095.0
#define PIN_LM35 32

bool clockwiseFlag = false;
bool counterClockwiseFlag = false;
bool flagAutomatic = false;

bool clockwiseFlag2 = false;
bool counterClockwiseFlag2 = false;
bool flagAutomatic2 = false;

TaskHandle_t TaskHandle_Core0;
TaskHandle_t TaskHandle_Core1;

void sendTelegramData(void* pvParameters);
void Rem1(void* pvParameters);
void Rem2(void* pvParameters);



float readTemperature() {
  int adcVal = analogRead(pin);
  float milliVolt = adcVal * (ADC_VREF_mV / ADC_RESOLUTION);
  return milliVolt / 10;
}
float readLightSensorValue(float pin) {
  return analogRead(pin);
}
float readUVIndex(float pin) {
  int sensor_value = analogRead(pin);
  float volts = sensor_value * 5.0 / 1024.0;
  float uvIndex = volts * 10;
}


float lightValue;
int tempC;
float UV_index;
void sendSensorData() {
  String message = "";

  if (tempC < 27) message += "\n 🌡️ Nhiệt độ hiện tại: " + String(tempC) + " °C" + " -- 🌤  Trung bình , khá dễ chịu";
  else message += "\n 🌡️ Nhiệt độ hiện tại: " + String(tempC) + " °C" + " -- 🔥 Khá nóng , ở trong nhà khi có thể ";

  if (UV_index < 2) message += "\n 💥 Chỉ số UV : " + String(UV_index) + "           -- -- ✅ Chỉ số UV thấp, không gây hại.";
  else message += "\n 💥 Chỉ số UV : " + String(UV_index) + "            -- -- ⚠️ Chỉ số UV ở mức cao, hạn chế ra khỏi nhà.";

  if (lightValue < threshold) {
    message += "\n ☀️ Trời Sáng";
  } else {
    message += "\n 🌙 Trời Tối";
  }

  bot.sendMessage(CHAT_ID, message, "");
}

void sendTelegramData(void* pvParameters) {
  (void)pvParameters;
  Serial.println("Task 0 đang chạy core : ");
  Serial.println(xPortGetCoreID());
  for (;;) {
    sendSensorData();
    vTaskDelay(pdMS_TO_TICKS(5000));
    Serial.println("-Task_TELE-----------------------");
  }
}
void Rem1(void* pvParameters) {
  (void)pvParameters;
  Serial.println("Task 1 đang chạy core : ");
  Serial.println(xPortGetCoreID());
  for (;;) {
    if (flagAutomatic) {
      Automatic(lightValue);
    } else {
      RemoteControl();
    }
    Serial.println("----------Task 1---------------");
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

void Rem2(void* pvParameters) {
  (void)pvParameters;
  Serial.println("Task 2 đang chạy core : ");
  Serial.println(xPortGetCoreID());
  for (;;) {
    if (flagAutomatic2) {
      Automatic2(lightValue);
    } else {
      RemoteControl2();
    }
    Serial.println("------------------------2--");
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");

  myStepper.setSpeed(10);
  myStepper2.setSpeed(10);
  Blynk.begin(auth, ssid, password);

  xTaskCreatePinnedToCore(
    Rem1,               // Function to implement the task
    "task1",            // Name of the task
    7000,               // Stack size in words
    NULL,               // Task input parameter
    1,                  // Priority of the tatsk
    &TaskHandle_Core1,  // Task handle
    1);
  xTaskCreatePinnedToCore(
    Rem2,               // Function to implement the task
    "task2",            // Name of the task
    7000,               // Stack size in words
    NULL,               // Task input parameter
    1,                  // Priority of the tatsk
    &TaskHandle_Core0,  // Task handle
    1);
  vTaskStartScheduler();
}

void loop() {
  Blynk.run();

  Blynk.syncVirtual(V3);
  Blynk.syncVirtual(V4);
  Blynk.syncVirtual(V5);
  Blynk.syncVirtual(V6);

  tempC = readTemperature(PIN_LM35);
  lightValue = readLightSensorValue(analogPin_AS);
  UV_index = readUVIndex(UV_SENSOR_PIN);
  Serial.println(lightValue);
  Serial.println(tempC);
  Serial.println(UV_index);


  Blynk.virtualWrite(V0, tempC);
  Blynk.virtualWrite(V1, lightValue);
  Blynk.virtualWrite(V2, UV_index);

  delay(500);
}

void rotateClockwise(float numRevolutions) {
  myStepper.step(numRevolutions * stepsPerRevolution);
}

void rotateCounterClockwise(float numRevolutions) {
  myStepper.step(-numRevolutions * stepsPerRevolution);
}
void rotateClockwise2(float numRevolutions) {
  myStepper2.step(numRevolutions * stepsPerRevolution);
}

void rotateCounterClockwise2(float numRevolutions) {
  myStepper2.step(-numRevolutions * stepsPerRevolution);
}
void stopMotor() {
  Serial.println("Motor stopped.");
}
//***-----STEPMOTOR----------------------------------------------------***//
//***--------------------------2 MODE----------------------------------***//
void Automatic(float lightValue) {
  if (lightValue < threshold) {
    if (counterClockwiseFlag == false) {
      Serial.println("Sáng auto 1");
      //digitalWrite(ledPin_G, LOW);
      rotateCounterClockwise(3);
      stopMotor();
      counterClockwiseFlag = true;
      clockwiseFlag = false;
    }
  } else {
    if (clockwiseFlag == false) {
      Serial.println("Tối auto 1");
      // digitalWrite(ledPin_G, HIGH);
      rotateClockwise(3);
      stopMotor();
      clockwiseFlag = true;
      counterClockwiseFlag = false;
    }
  }
  delay(10000);
}

void Automatic2(float lightValue) {
  if (lightValue < threshold) {
    if (counterClockwiseFlag2 == false) {
      Serial.println("Sáng auto 2");
      // digitalWrite(ledPin_R, LOW);
      rotateCounterClockwise2(3);
      stopMotor();
      counterClockwiseFlag2 = true;
      clockwiseFlag2 = false;
    }
  } else {
    if (clockwiseFlag2 == false) {
      Serial.println("Tối auto 2");
      // digitalWrite(ledPin_R, HIGH);
      rotateClockwise2(3);
      stopMotor();
      clockwiseFlag2 = true;
      counterClockwiseFlag2 = false;
    }
  }
  delay(10000);
}


float numdarklight;
float numdarklight2;
float count = 0;
float count1 = 3;
float count2 = 0;
float count3 = 3;
float numRevolutionRemote;
float numRevolutionRemote2;
bool pUpdated1 = false;
bool pUpdated2 = false;

void RemoteControl() {
  if (pUpdated1) {
    pUpdated1 = false;
    numdarklight = (numRevolutionRemote * 3) / 100;
    Serial.println("Numdarklight :" + String(numdarklight));
    if (clockwiseFlag) {  // dark
      if (numdarklight > count) {
        rotateCounterClockwise(fabs(numdarklight - count));
        Serial.println("Hạ 1 :" + String(numdarklight));
        count = numdarklight;
      } else if (numdarklight < count) {
        rotateClockwise(fabs(count - numdarklight));
        Serial.println("Kéo lên :" + String(count - numdarklight));
        count = numdarklight;
      } else {
        stopMotor();
      }
    } else {  // light
      if (numdarklight < count1) {
        rotateClockwise(fabs(numdarklight - count1));
        Serial.println("Kéo lên 1:" + String(numdarklight));
        count1 = numdarklight;
      } else if (numdarklight > count1) {
        rotateCounterClockwise(fabs(count1 - numdarklight));
        Serial.println("Hạ 1:" + String(count1 - numdarklight));
        count1 = numdarklight;
      } else {
        stopMotor();
      }
    }
  }
}

void RemoteControl2() {
  if (pUpdated2) {
    pUpdated2 = false;
    numdarklight2 = (numRevolutionRemote2 * 3) / 100;
    Serial.println("Numdarklight 2 :" + String(numdarklight2));
    if (clockwiseFlag2) {  // dark
      if (numdarklight2 > count2) {
        rotateCounterClockwise2(fabs(numdarklight2 - count2));
        Serial.println("Hạ :" + String(numdarklight2));
        count2 = numdarklight2;
      } else if (numdarklight2 < count2) {
        rotateClockwise2(fabs(count2 - numdarklight2));
        Serial.println("Kéo lên :" + String(count2 - numdarklight2));
        count2 = numdarklight2;
      } else {
        stopMotor();
      }
    } else {  // light
      if (numdarklight2 < count3) {
        rotateClockwise2(fabs(numdarklight2 - count3));
        Serial.println("Kéo lên :" + String(numdarklight2));
        count3 = numdarklight2;
      } else if (numdarklight2 > count3) {
        rotateCounterClockwise2(fabs(count3 - numdarklight2));
        Serial.println("Hạ :" + String(count3 - numdarklight2));
        count3 = numdarklight2;
      } else {
        stopMotor();
      }
    }
  }
}
bool flagauto = false;
bool flagauto2 = false;
BLYNK_WRITE(V3) {
  int p = param.asInt();
  if (p == 1) {
    Serial.println("Bật Automatic 1");
    flagAutomatic = true;
    if (flagauto == true) {
      // rotateClockwise(numdarklight);
      // Serial.println("kéo lên khi bật lại auto : " + String(numdarklight));
      rotateCounterClockwise(fabs(3 - numdarklight));
      Serial.println("hạ xuống khi bật lại auto 1 : " + String(fabs(3 - numdarklight)));
      count = 0;
      count1 = 3;
      flagauto = false;
    }
  } else {
    Serial.println("Tắt Automatic 1");
    flagAutomatic = false;
    flagauto = true;
  }
}
BLYNK_WRITE(V4) {
  int p = param.asInt();
  if (p == 1) {
    Serial.println("Bật Automatic 2");
    flagAutomatic2 = true;
    if (flagauto2 == true) {
      // rotateClockwise(numdarklight2);
      // Serial.println("kéo lên khi bật lại auto 2: " + String(numdarklight2));
      rotateCounterClockwise2(fabs(3 - numdarklight2));
      Serial.println("hạ xuống khi bật lại auto 2 : " + String(fabs(3 - numdarklight2)));
      count2 = 0;
      count3 = 3;
      flagauto2 = false;
    }
  } else {
    Serial.println("Tắt Automatic 2");
    flagAutomatic2 = false;
    flagauto2 = true;
  }
}
int previousP1 = -1;
BLYNK_WRITE(V5) {
  int p = param.asInt();
  if (p != previousP1) {
    numRevolutionRemote = p;
    Serial.print("Mức mở rèm 1 : ");
    Serial.println(p);
    previousP1 = p;
    pUpdated1 = true;
  }
}

int previousP2 = -2;
BLYNK_WRITE(V6) {
  int p = param.asInt();
  if (p != previousP2) {
    numRevolutionRemote2 = p;
    Serial.print("Mức mở rèm 2: ");
    Serial.println(p);
    previousP2 = p;
    pUpdated2 = true;
  }
}