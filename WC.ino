/*
  Version: 1.0.0

  Компоненты:
  - Контроллер LOLIN D1 Mini V3.1.0
  - Дисплей OLED SSD1306 128x64
  - Сенсорные кнопки TTP223
  - Пироэлектрический датчик движения PIR Motion Sensor
  - Часы реального времени RTC (Real Time Clock) DS1307
  - Код, схемы - https://github.com/killadog/WaterCounter


                    Mode 0                Mode 1                      Mode 2
                  (1 click)        (Enter/Exit - 2 clicks) (Enter/Exit - 1 second hold)
            ┌─────────────────────┬───────────────────────┬────────────────────────────┐
   Screen 0 │       Time          |         Cost          │            Set             │
            │    Current value    │                       |            cold            │
            ├─────────────────────┼───────────────────────┼────────────────────────────┤
   Screen 1 │    Full cold value  │        Tariff         |            Set             |
            │     Cold chart      │                       |            hot             |
            ├─────────────────────┼───────────────────────┼────────────────────────────┤
   Screen 2 │    Full hot value   │    Network settings   │            Set             |
            │      Hot chart      │                       │         cold tariff        |
            └─────────────────────┼───────────────────────┼────────────────────────────┤
   Screen 3                       │        Uptime         │            Set             |
                                  │                       |         hot tariff         |
                                  ├───────────────────────┼────────────────────────────┤
   Screen 4                       │       Copyright       │            Set             |
                                  │                       |         out tarriff        |
                                  └───────────────────────┘────────────────────────────┘
*/

#define BLYNK_PRINT Serial

#include <ArduinoOTA.h>
#include <BlynkSimpleEsp8266.h>
#include <ESP8266WiFi.h>
#include <RTClib.h>
#include <SSD1306Wire.h>
#include <NTPClient.h>
#include <GyverButton.h>                //Работа с кнопками https://github.com/AlexGyver/GyverLibs/tree/master/GyverButton

char auth[] = "47cfb9a85e224d0b9865a5c8c4b5a0de";                   //Blynk
char ssid[] = "bakawaka";                                           //Access point
char pass[] = "assoonas";                                           //пароль

#define SCL_PIN                     5                               //пин SCL           D1
#define SDA_PIN                     4                               //пин SDA           D2
#define COLD_PIN                    2                               //пин COLD          D4
#define HOT_PIN                     0                               //пин HOT           D3
#define SELECT_PIN                  12                              //пин кнопки SELECT D6
#define PLUS_PIN                    13                              //пин +             D7
#define MINUS_PIN                   14                              //пин -             D5
#define PIR_PIN                     16                              //пин датчика PIR   D0

#define COUNTERS                    2                               //количество счётчиков
float COUNTER_ALL_TIME[COUNTERS]    = {1408.10, 816.10};           //начальные значения счётчиков
char* CounterName[COUNTERS]         = {"COLD", "HOT"};              //названия счётчиков

#define Days_To_Remember           7                            //количество дней для отображения
uint16_t Counter_Day[COUNTERS][Days_To_Remember];                        //массив со значениями за каждый день

uint8_t Today;                                                      //число сегодняшнего дня

#define TARIFFS                3                             //количество тарифов
float TARIFF[TARIFFS]          = {42.30, 198.19, 30.90};       //значения тарифов (холодная, горячая, водоотвод)
char* TariffName[TARIFFS]      = {"TARIFF COLD", "TARIFF HOT", "TARIFF OUT"}; //названия тарифов

boolean PIR_FLAG               = 1;                            //флаг включения экрана
uint32_t PIR_TIMER             = millis();                     //стартовое время подсветки экрана
uint32_t DISPALY_ON_TIME       = 30000;                        //время подсветки экрана

#define MODES                  3                            //количество MODES
uint8_t SCREENS[3]             = {3, 5, 5};                    //количество SCREENS в каждом MODE
uint8_t MODE                   = 0;                            //начальный MODE
uint8_t SCREEN_NUMBER          = 0;                            //начальный SCREEN

uint32_t START_TIME            = 0;                            //стартовое время для аптайма
boolean SETTING_MODE           = 0;                            //режим корректировки

uint32_t PLUS_MINUS_TIME       = 0;                            //стартовое время нажатия плюс/минуса

RTC_DS1307 rtc;
DateTime start_time;
BlynkTimer timer;
SSD1306Wire display(0x3c, SDA_PIN, SCL_PIN);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);

GButton button_cold(COLD_PIN);
GButton button_hot(HOT_PIN);
GButton button_select(SELECT_PIN, LOW_PULL, NORM_OPEN);
GButton button_plus(PLUS_PIN, LOW_PULL, NORM_OPEN);
GButton button_minus(MINUS_PIN, LOW_PULL, NORM_OPEN);

void setup()

{
  for (uint8_t i = 0; i < COUNTERS; i++)
  {
    for (uint8_t j = 1; j < Days_To_Remember; j++)
    {
      Counter_Day[i][j] = random(10, 100);
      Counter_Day[i][j] = Counter_Day[i][j] / 10 * 10;
    }
  }

  pinMode(SDA_PIN, INPUT_PULLUP);
  pinMode(SCL_PIN, INPUT_PULLUP);
  pinMode(PIR_PIN, INPUT);

  pinMode(COLD_PIN, INPUT_PULLUP);
  pinMode(HOT_PIN, INPUT_PULLUP);

  //pinMode(SELECT_PIN, INPUT_PULLUP);
  button_select.setDebounce(80);         // настройка антидребезга (по умолчанию 80 мс)
  button_select.setTimeout(1000);        // настройка таймаута на удержание (по умолчанию 500 мс)
  button_select.setClickTimeout(300);    // настройка таймаута между кликами (по умолчанию 300 мс)

  //pinMode(PLUS_PIN, INPUT_PULLUP);
  button_plus.setDebounce(80);           // настройка антидребезга (по умолчанию 80 мс)
  button_plus.setTimeout(500);           // настройка таймаута на удержание (по умолчанию 500 мс)
  button_plus.setClickTimeout(300);      // настройка таймаута между кликами (по умолчанию 300 мс)
  button_plus.setStepTimeout(100);       // установка таймаута между инкрементами (по умолчанию 400 мс)

  //pinMode(MINUS_PIN, INPUT_PULLUP);
  button_minus.setDebounce(80);           // настройка антидребезга (по умолчанию 80 мс)
  button_minus.setTimeout(500);           // настройка таймаута на удержание (по умолчанию 500 мс)
  button_minus.setClickTimeout(300);      // настройка таймаута между кликами (по умолчанию 300 мс)
  button_minus.setStepTimeout(100);       // установка таймаута между инкрементами (по умолчанию 400 мс)

  Serial.begin(115200);                   //вывод в Serial

  WiFi.begin(ssid, pass);
  Serial.println();
  Serial.println(F("Connecting to WiFi"));
  while (WiFi.status() != WL_CONNECTED) {
    delay (500);
    Serial.print (F("."));
  }
  Serial.println();

  if (!rtc.begin())
  {
    Serial.println(F("Couldn't find RTC"));
    while (1);
  }
  if (!rtc.isrunning())
  {
    Serial.println(F("RTC is NOT running!"));
  }

  timeClient.begin();
  timeClient.setTimeOffset(10800);                             //Moscow timezone

  NTP_RTC();

  START_TIME = rtc.now().unixtime();

  Blynk.begin(auth, ssid, pass);

  INIT_DISPLAY();

  timer.setInterval(1005L, CHECK_PIR);
  timer.setInterval(995L, READ_COUNTER);
  timer.setInterval(1015L, SHOW);
  timer.setInterval(10000L, Check_Today);
  timer.setInterval(3600000L, NTP_RTC);
  timer.setInterval(50L, CHECK_BUTTONS);
}

void loop()
{
  Blynk.run();
  timer.run();
}

void CHECK_PIR()
{
  boolean SENSOR_VALUE = digitalRead(PIR_PIN);                   // читаем значение от датчика
  if ((SENSOR_VALUE) && (!PIR_FLAG))                             // если что-то двигается
  {
    display.displayOn();
    PIR_FLAG = 1;
    CLOCK_TO_SERIAL();
    Serial.print(F("Display is ON after "));

    int16_t DIFF_TIME = (millis() - PIR_TIMER) / 1000;
    String UNIT;
    if (DIFF_TIME >= 86400)
    {
      DIFF_TIME = DIFF_TIME / 86400;
      UNIT = " days";
    } else if (DIFF_TIME >= 3600)
    {
      DIFF_TIME = DIFF_TIME / 3600;
      UNIT = " hours";
    } else if (DIFF_TIME >= 60)
    {
      DIFF_TIME = DIFF_TIME / 60;
      UNIT = " min";
    } else
    {
      UNIT = " sec";
    }

    Serial.print(DIFF_TIME);
    Serial.println(UNIT);
    PIR_TIMER = millis();                                        //для вычисления через сколько надо ВЫКЛючится
    SHOW();
  }

  if ((PIR_FLAG) && (millis() - PIR_TIMER > DISPALY_ON_TIME))
  {
    display.displayOff();
    PIR_FLAG = 0;                                                //флаг что дисплей ВЫКЛючен
    CLOCK_TO_SERIAL();
    Serial.println(F("Display is OFF"));
    PIR_TIMER = millis();                                        //для информации через сколько ВКЛючится
  }
}

void READ_COUNTER()
{
  button_cold.tick();
  if (button_cold.isPress())
  {
    uint8_t i = 0;
    COUNTER_ALL_TIME[i] = COUNTER_ALL_TIME[i] + 0.01;
    Counter_Day[i][0] = Counter_Day[i][0] + 10;
    Blynk.virtualWrite(V0, String(COUNTER_ALL_TIME[i], 2));
    Blynk.virtualWrite(V2, Counter_Day[i][0]);
    Blynk.virtualWrite(V4, String(Counter_Day[i][0] * (TARIFF[0] / 1000), 2) + " + " + String(Counter_Day[i][0] * (TARIFF[2] / 1000), 2) + " = " + String(Counter_Day[i][0] * (TARIFF[0] / 1000) + Counter_Day[i][0] * (TARIFF[2] / 1000), 2));
    Blynk.virtualWrite(V6, Counter_Day[i][1]);
    CLOCK_TO_SERIAL();
    COUNTER_TO_SERIAL(i);
  }

  button_hot.tick();
  if (button_hot.isPress())
  {
    uint8_t i = 1;
    COUNTER_ALL_TIME[i] = COUNTER_ALL_TIME[i] + 0.01;
    Counter_Day[i][0] = Counter_Day[i][0] + 10;
    Blynk.virtualWrite(V1, String(COUNTER_ALL_TIME[i], 2));
    Blynk.virtualWrite(V3, Counter_Day[i][0]);
    Blynk.virtualWrite(V5, String(Counter_Day[i][0] * (TARIFF[1] / 1000), 2) + " + " + String(Counter_Day[i][0] * (TARIFF[2] / 1000), 2) + " = " + String(Counter_Day[i][0] * (TARIFF[1] / 1000) + Counter_Day[i][0] * (TARIFF[2] / 1000), 2));
    Blynk.virtualWrite(V7, Counter_Day[i][1]);
    CLOCK_TO_SERIAL();
    COUNTER_TO_SERIAL(i);
  }
}

void CHECK_BUTTONS()
{
  button_select.tick();
  if (!SETTING_MODE)
  {
    if (button_select.isSingle())
    {
      SCREEN_NUMBER++;
      if (SCREEN_NUMBER > SCREENS[MODE] - 1) SCREEN_NUMBER = 0;
      CLOCK_TO_SERIAL();
      Serial.print(F("One click.  "));
      MODE_SCREEN_TO_SERIAL();
      SHOW();
    }
    if (button_select.isDouble())
    {
      MODE++;
      if (MODE >= MODES - 1) MODE = 0;
      SCREEN_NUMBER = 0;
      SHOW();
      CLOCK_TO_SERIAL();
      Serial.print(F("Two clicks. "));
      MODE_SCREEN_TO_SERIAL();
      SHOW();
    }
    if (button_select.isHolded())
    {
      SETTING_MODE = 1;
      MODE = MODES - 1;                                      //в режим корректировки значений счётчика
      SCREEN_NUMBER = 0;
      CLOCK_TO_SERIAL();
      Serial.print(F("Holded.     "));
      MODE_SCREEN_TO_SERIAL();
      SHOW();
    }
  }

  if (SETTING_MODE)
  {
    if (button_select.isSingle())
    {
      SCREEN_NUMBER++;
      if (SCREEN_NUMBER > SCREENS[MODE] - 1) SCREEN_NUMBER = 0;
      CLOCK_TO_SERIAL();
      Serial.print(F("One click.  "));
      MODE_SCREEN_TO_SERIAL();
      SHOW();
    }
    if (button_select.isHolded())
    {
      SETTING_MODE = 0;
      MODE = 0;
      SCREEN_NUMBER = 0;
      CLOCK_TO_SERIAL();
      Serial.print(F("Holded.     "));
      MODE_SCREEN_TO_SERIAL();
      SHOW();

      uint8_t i = 0;
      Blynk.virtualWrite(V0, String(COUNTER_ALL_TIME[i], 2));
      Blynk.virtualWrite(V2, Counter_Day[i][0]);
      Blynk.virtualWrite(V4, String(Counter_Day[i][0] * (TARIFF[0] / 1000), 2) + " + " + String(Counter_Day[i][0] * (TARIFF[2] / 1000), 2) + " = " + String(Counter_Day[i][0] * (TARIFF[0] / 1000) + Counter_Day[i][0] * (TARIFF[2] / 1000), 2));
      Blynk.virtualWrite(V6, Counter_Day[i][1]);

      i = 1;
      Blynk.virtualWrite(V1, String(COUNTER_ALL_TIME[i], 2));
      Blynk.virtualWrite(V3, Counter_Day[i][0]);
      Blynk.virtualWrite(V5, String(Counter_Day[i][0] * (TARIFF[1] / 1000), 2) + " + " + String(Counter_Day[i][0] * (TARIFF[2] / 1000), 2) + " = " + String(Counter_Day[i][0] * (TARIFF[1] / 1000) + Counter_Day[i][0] * (TARIFF[2] / 1000), 2));
      Blynk.virtualWrite(V7, Counter_Day[i][1]);
    }

    button_plus.tick();
    if (button_plus.isPress()) PLUS_MINUS_TIME = millis();
    if (button_plus.isClick()) COUNTER_ALL_TIME[SCREEN_NUMBER] += 0.01;
    if (button_plus.isStep())
    {
      if (SCREEN_NUMBER == 0 || SCREEN_NUMBER == 1)
      {
        if (millis() - PLUS_MINUS_TIME > 3000)
        {
          COUNTER_ALL_TIME[SCREEN_NUMBER] += 1;
        } else if (millis() - PLUS_MINUS_TIME > 1000)
        {
          COUNTER_ALL_TIME[SCREEN_NUMBER] += 0.1;
        } else
        {
          COUNTER_ALL_TIME[SCREEN_NUMBER] += 0.01;
        }
      } else
      {
        if (millis() - PLUS_MINUS_TIME > 3000)
        {
          TARIFF[SCREEN_NUMBER - 2] += 1;
        } else if (millis() - PLUS_MINUS_TIME > 1000)
        {
          TARIFF[SCREEN_NUMBER - 2] += 0.1;
        } else
        {
          TARIFF[SCREEN_NUMBER - 2] += 0.01;
        }
      }
    }

    button_minus.tick();
    if (button_minus.isPress()) PLUS_MINUS_TIME = millis();
    if (button_minus.isClick()) COUNTER_ALL_TIME[SCREEN_NUMBER] -= 0.01;
    if (button_minus.isStep())
    {
      if (SCREEN_NUMBER == 0 || SCREEN_NUMBER == 1)
      {
        if (millis() - PLUS_MINUS_TIME > 3000)
        {
          COUNTER_ALL_TIME[SCREEN_NUMBER] -= 1;
        } else if (millis() - PLUS_MINUS_TIME > 1000)
        {
          COUNTER_ALL_TIME[SCREEN_NUMBER] -= 0.1;
        } else
        {
          COUNTER_ALL_TIME[SCREEN_NUMBER] -= 0.01;
        }
      } else
      {
        if (millis() - PLUS_MINUS_TIME > 3000)
        {
          TARIFF[SCREEN_NUMBER - 2] -= 1;
        } else if (millis() - PLUS_MINUS_TIME > 1000)
        {
          TARIFF[SCREEN_NUMBER - 2] -= 0.1;
        } else
        {
          TARIFF[SCREEN_NUMBER - 2] -= 0.01;
        }
      }
    }

    if (button_plus.isHold() || button_minus.isHold()) SHOW();
  }
}

void SCREEN_0()
{
  switch (MODE)
  {
    case 0:
      CLOCK_TO_DISPLAY();
      for (uint8_t i = 0; i < COUNTERS; i++)
      {
        COUNTER_TO_DISPLAY(i);
      }
      break;
    case 1:
      float IN;
      float OUT;
      float COST;

      display.setFont(ArialMT_Plain_16);
      for (uint8_t i = 0; i < COUNTERS; i++)
      {
        IN = Counter_Day[i][0] * (TARIFF[i] / 1000);
        OUT = Counter_Day[i][0] * (TARIFF[2] / 1000);
        COST = IN + OUT;

        display.setTextAlignment(TEXT_ALIGN_CENTER);
        display.drawString(i * 64 + 32, 0, String(COST));
        display.drawHorizontalLine(0, 20, 128);
        display.drawString(i * 64 + 32, 24, String(IN));
        display.drawString(i * 64 + 32, 48, String(OUT));
        display.setTextAlignment(TEXT_ALIGN_LEFT);
        display.drawString(i * 64 + 1, 36, String(" + "));
      }
      break;
    case 2:
      display.setFont(ArialMT_Plain_16);
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      display.drawString(64, 0, String(CounterName[SCREEN_NUMBER]));
      display.drawString(64, 32, String(COUNTER_ALL_TIME[SCREEN_NUMBER]));
      break;
  }
}

void SCREEN_1()
{
  switch (MODE)
  {
    case 0:
      CHART(0);
      break;
    case 1:
      display.setFont(ArialMT_Plain_16);
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      display.drawString(64 , 0, "TARIFF");
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.drawString(0 , 16, "COLD");
      display.drawString(0 , 32, "HOT");
      display.drawString(0 , 48, "OUT");
      display.setTextAlignment(TEXT_ALIGN_RIGHT);
      display.drawString(128 , 16, String(TARIFF[0]));
      display.drawString(128 , 32, String(TARIFF[1]));
      display.drawString(128 , 48, String(TARIFF[2]));
      break;
    case 2:
      display.setFont(ArialMT_Plain_16);
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      display.drawString(64, 0, String(CounterName[SCREEN_NUMBER]));
      display.drawString(64, 32, String(COUNTER_ALL_TIME[SCREEN_NUMBER]));
      break;
  }
}
void SCREEN_2()
{
  switch (MODE)
  {
    case 0:
      CHART(1);
      break;
    case 1:
      display.setFont(ArialMT_Plain_16);
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      display.drawString(64 , 0, WiFi.localIP().toString());
      display.drawString(64 , 16, WiFi.subnetMask().toString());
      display.drawString(64 , 32, WiFi.gatewayIP().toString());
      display.drawString(64 , 48, WiFi.hostname());
      break;
    case 2:
      display.setFont(ArialMT_Plain_16);
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      display.drawString(64, 0, String(TariffName[0]));
      display.drawString(64, 32, String(TARIFF[0]));
      break;
  }
}

void SCREEN_3()
{
  switch (MODE)
  {
    case 1:
      {
        display.setFont(ArialMT_Plain_16);
        display.setTextAlignment(TEXT_ALIGN_CENTER);
        display.drawString(64 , 0, "UPTIME");
        display.drawString(64 , 16, "");
        int DIFF_TIME = rtc.now().unixtime() - START_TIME;
        String UNIT;
        if (DIFF_TIME >= 86400)
        {
          DIFF_TIME = DIFF_TIME / 86400;
          UNIT = " days";
        } else if (DIFF_TIME >= 3600)
        {
          DIFF_TIME = DIFF_TIME / 3600;
          UNIT = " hours";
        } else if (DIFF_TIME >= 60)
        {
          DIFF_TIME = DIFF_TIME / 60;
          UNIT = " min";
        } else
        {
          UNIT = " sec";
        }
        display.drawString(64 , 32, String(DIFF_TIME) + UNIT);
        display.drawString(64 , 48, "");
      }
      break;
    case 2:
      display.setFont(ArialMT_Plain_16);
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      display.drawString(64, 0, String(TariffName[1]));
      display.drawString(64, 32, String(TARIFF[1]));
      break;
  }
}

void SCREEN_4()
{
  switch (MODE)
  {
    case 1:
      display.setFont(ArialMT_Plain_16);
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      display.drawString(64 , 0, "ver. 0.1");
      display.drawString(64 , 32, "2019 - 2021");
      break;
    case 2:
      display.setFont(ArialMT_Plain_16);
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      display.drawString(64, 0, String(TariffName[2]));
      display.drawString(64, 32, String(TARIFF[2]));
      break;
  }
}

void SHOW()
{
  display.clear();
  switch (SCREEN_NUMBER)
  {
    case 0: SCREEN_0(); break;
    case 1: SCREEN_1(); break;
    case 2: SCREEN_2(); break;
    case 3: SCREEN_3(); break;
    case 4: SCREEN_4(); break;
    default: Serial.println(SCREEN_NUMBER);
  }
  display.display();
}

void INIT_DISPLAY()
{
  display.init();
  display.flipScreenVertically();
}

void COUNTER_TO_DISPLAY(byte i)
{
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(i * 64 + 32, 18, String(CounterName[i]));
  display.setFont(ArialMT_Plain_24);
  display.drawString(i * 64 + 32, 30, String(Counter_Day[i][0]));
  display.setFont(ArialMT_Plain_10);
  display.drawString(i * 64 + 32, 54, String(COUNTER_ALL_TIME[i]));
}

void COUNTER_TO_SERIAL(byte i)
{
  Serial.print(CounterName[i]);
  Serial.print(F(": "));
  Serial.print(COUNTER_ALL_TIME[i]);
  Serial.print(F(". Today: "));
  Serial.print(Counter_Day[i][0]);
  Serial.print(F(". Yesterday: "));
  Serial.print(Counter_Day[i][1]);
  Serial.println();
}

void NTP_RTC()
{
  int Y, M, D, h, m, s;

  while (!timeClient.update())
  {
    timeClient.forceUpdate();
  }

  Serial.print(F("NTP time: "));
  Serial.println(timeClient.getFormattedDate());
  Serial.print(F("RTC time: "));
  CLOCK_TO_SERIAL();
  Serial.println();

  String NTP_TIME = timeClient.getFormattedDate();

  Y = NTP_TIME.substring(0, 4).toInt();
  M = NTP_TIME.substring(5, 7).toInt();
  D = NTP_TIME.substring(8, 10).toInt();
  h = NTP_TIME.substring(11, 13).toInt();
  m = NTP_TIME.substring(14, 16).toInt();
  s = NTP_TIME.substring(17, 19).toInt();

  Serial.print(F("Correcting RTC time ("));
  Serial.print(timeClient.getEpochTime() - rtc.now().unixtime());
  Serial.println(F(" sec)..."));
  rtc.adjust(DateTime(Y, M, D, h, m, s));
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  //rtc.adjust(DateTime(2009, 7, 8, 23, 59, 50));

  Serial.print(F("RTC time: "));
  CLOCK_TO_SERIAL();
  Serial.println();

  DateTime now = rtc.now();
  Today = now.day();
}

void CLOCK_TO_DISPLAY()
{
  DateTime now = rtc.now();
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  String Clock = "";
  if (now.hour() < 10) Clock += "0";
  Clock += String(now.hour()) + " : ";
  if (now.minute() < 10) Clock += "0";
  Clock += String(now.minute()) + " : ";
  if (now.second() < 10) Clock += "0";
  Clock += String(now.second());
  display.drawString(64, 0, String(Clock));
}

void MODE_SCREEN_TO_SERIAL()
{
  Serial.print(F("MODE: "));
  Serial.print(MODE);
  Serial.print(F("  SCREEN_NUMBER: "));
  Serial.println(SCREEN_NUMBER);
}

void CLOCK_TO_SERIAL()
{
  DateTime now = rtc.now();
  Serial.print(now.year());
  Serial.print(F("."));
  if (now.month() < 10) Serial.print(0);
  Serial.print(now.month());
  Serial.print(F("."));
  if (now.day() < 10) Serial.print(0);
  Serial.print(now.day());
  Serial.print(F(" "));
  if (now.hour() < 10) Serial.print(0);
  Serial.print(now.hour());
  Serial.print(F(":"));
  if (now.minute() < 10) Serial.print(0);
  Serial.print(now.minute());
  Serial.print(F(":"));
  if (now.second() < 10) Serial.print(0);
  Serial.print(now.second());
  Serial.print(F("  "));
}
void Check_Today()
{
  DateTime now = rtc.now();
  if (Today != now.day())
  {
    Today = now.day();
    for (uint8_t i = 0; i < COUNTERS; i++)
    {
      for (uint8_t j = Days_To_Remember - 1; j > 0; j = j - 1)
      {
        Counter_Day[i][j] = Counter_Day[i][j - 1];
      }
      Counter_Day[i][0] = 0;
    }
    Serial.print(F("Next day! Today is "));
    Serial.println(Today);
    Serial.println();
  }
}

void CHART(byte COUNTER)
{
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 0, String(CounterName[COUNTER]));
  display.setTextAlignment(TEXT_ALIGN_RIGHT);
  display.drawString(128, 0, String(COUNTER_ALL_TIME[COUNTER]));

  int16_t Max_Value = 0;
  for (uint8_t i = 0; i < Days_To_Remember; i++)
  {
    if (Max_Value < Counter_Day[COUNTER][i]) Max_Value = Counter_Day[COUNTER][i];
  }
  display.setFont(ArialMT_Plain_10);
  DateTime now = rtc.now();
  uint8_t Bar_Width = 128 / Days_To_Remember;
  for (uint8_t i = 0; i < Days_To_Remember; i++)
  {
    uint8_t Bar_Height = map(Counter_Day[COUNTER][i], 0, Max_Value, 0, 36);
    display.fillRect(128 - i * Bar_Width - Bar_Width, 54 - Bar_Height, Bar_Width - 2, Bar_Height);
    DateTime past (now + TimeSpan(-i, 0, 0, 0));
    display.drawString(128 - i * Bar_Width - Bar_Width / 2 + 6, 54, String(past.day()));
  }
}
