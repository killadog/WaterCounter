<img src="https://img.shields.io/badge/version-0.1-green">

# Водосчётчик

- [Возможности](https://github.com/killadog/WaterCounter/blob/main/README.md#%D0%B2%D0%BE%D0%B7%D0%BC%D0%BE%D0%B6%D0%BD%D0%BE%D1%81%D1%82%D0%B8)
- [Режимы экрана](https://github.com/killadog/WaterCounter/blob/main/README.md#%D1%80%D0%B5%D0%B6%D0%B8%D0%BC%D1%8B-%D1%8D%D0%BA%D1%80%D0%B0%D0%BD%D0%B0)
- [Компоненты](https://github.com/killadog/WaterCounter/blob/main/README.md#%D0%BA%D0%BE%D0%BC%D0%BF%D0%BE%D0%BD%D0%B5%D0%BD%D1%82%D1%8B)
- [Схемы](https://github.com/killadog/WaterCounter/blob/main/README.md#%D1%81%D1%85%D0%B5%D0%BC%D1%8B)
- [Библиотеки](https://github.com/killadog/WaterCounter/blob/main/README.md#%D0%B1%D0%B8%D0%B1%D0%BB%D0%B8%D0%BE%D1%82%D0%B5%D0%BA%D0%B8)

## Возможности
- Подсчёт потребления воды от двух водосчётчиков
- Установка значений счётчиков и тарифов
- Включение/отключения дисплея от датчика движения
- Синхронизация времени с NTP сервером
- Отправка данных на сервер [Blynk](https://blynk.io/)
- Отслеживание состояния на смартфоне в приложении Blynk ([iOS](https://apps.apple.com/us/app/blynk-iot-for-arduino-esp32/id808760481), [Android](https://play.google.com/store/apps/details?id=cc.blynk&hl=en_US))

## Режимы экрана

||**Mode 0**|**Mode 1** |**Mode 2**|
|:---:|:---:|:---:|:---:|
|**Screen 0**|Текущее время. <br /> Расход холодной и горячей <br /> воды за день|Расходы за день|Установка счётчика <br /> холодной воды|
|**Screen 1**|График расхода холодной воды <br /> за последние 7 дней|Текущие тарифы|Установка счётчика <br /> горячей воды|
|**Screen 2**|График расхода горячей воды <br /> за последние 7 дней|Сетевые параметры|Установка тарифа <br /> холодной воды|
|**Screen 3**||Время работы|Установка тарифа <br /> холодной воды|
|**Screen 4**||Версия. Год|Установка тарифа <br /> отвода воды|

## Компоненты
- ESP
- OLED SSD1306 128x64
- TTP226
- PIR
- RTC

## Схемы

## Библиотеки

- [BlynkSimpleEsp8266.h]
- [ESP8266WiFi.h]
- [RTClib.h]
- [SSD1306Wire.h]
- [NTPClient.h]
- [GyverButton](https://github.com/AlexGyver/GyverLibs/tree/master/GyverButton) - работа с кнопками.
