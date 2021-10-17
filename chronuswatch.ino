#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <AsyncTCP.h>
// #include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <FS.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include "driver/adc.h"
#include <esp_system.h>
#include <time.h>
#include <sys/time.h>

#include "driver/adc.h"
#include <esp_wifi.h>
#include <esp_bt.h>

// #include <fonts/RobotoMonoThin.h>

//#include <BLEDevice.h>
//#include <BLEUtils.h>
//#include <BLEServer.h>

//BLEServer *pServer;
//BLEService *pService;
//BLECharacteristic *pCharacteristic;

//#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
//#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
bool invertDisplayColor = false;

File configFile;
#define FORMAT_SPIFFS_IF_FAILED true

#define capacity 800
StaticJsonDocument<capacity> doc;
StaticJsonDocument<capacity> weather;

unsigned long previousMillisWiFiReconect = 0;
#define interval 30000

// AsyncWebServer server(80);

const char *ntpServer = "pool.ntp.org";
long gmtOffset_sec = -3 * 3600;
#define daylightOffset_sec 3600

JsonVariant auxVar;
long byteGmtOffset;
long secTimeout;
String weatherId;
long toSetTime = 0;
//String timeBLE;

JsonArray arrElements;
JsonObject repo;

RTC_DATA_ATTR bool noTime = false;
time_t lastSavedTime;

bool isDigital = false;

bool enableAlarm = false;
byte configAlarm = 0;

byte alarmMin = 0;
byte alarmHour = 0;

#define Threshold 40
void callback()
{
  //placeholder callback function
}

void generalConfig()
{
  display.invertDisplay(doc["sets"]["invert"]);

  auxVar = doc["sets"]["gmt"];
  byteGmtOffset = auxVar.as<long>();

  auxVar = doc["sets"]["timeout"];
  secTimeout = auxVar.as<long>();

  auxVar = doc["sets"]["weatherId"];
  weatherId = auxVar.as<String>();

  arrElements = doc["elements"].as<JsonArray>();
}

void printText(String text, byte x, byte y, byte textSize, byte displayTime, byte align, boolean clean = true)
{
  if (clean)
  {
    delay(displayTime);
    display.clearDisplay();
  }
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(textSize);
  if (align == 0)
  {
    display.setCursor(x, y);
  }
  else if (align == 1)
  {
    display.setCursor(0, y);
  }
  else if (align == 2)
  {
    const int newX = 64 - ((text.length() * 6 * textSize) / 2);
    display.setCursor(newX, y);
  }
  else if (align == 3)
  {
    const int newX = 128 - (text.length() * 6);
    display.setCursor(newX, y);
  }
  display.println(text);
  display.display();

  if (clean)
  {
    delay(displayTime);
  }
}

void setWifiOff()
{
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  printText(F("WiFi disconnected"), 0, 30, 1, 2000, 2);
}

void printTextArray(String textos[8] = {})
{
  display.clearDisplay();
  display.setTextSize(1);
  int y = 0;
  for (int i = 0; i < 8; i++)
  {
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, y);
    if (textos[i].substring(0, 1) == "<")
    {
      display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
      display.setCursor(64 - ((textos[i].substring(1).length() * 6) / 2), y);
      display.fillRect(0, y, display.width(), 8, SSD1306_WHITE);
    }
    if (textos[i].substring(0, 1) == ">")
    {
      display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
      display.fillRect(0, y, display.width(), 8, SSD1306_WHITE);
    }
    display.println(textos[i].substring(1));
    y += 8;
  }
  display.display();
}

//void configBLE() {
//  BLEDevice::init("ChronusWatch");
//  pServer = BLEDevice::createServer();
//  pService = pServer->createService(SERVICE_UUID);
//  pCharacteristic = pService->createCharacteristic(
//                                         CHARACTERISTIC_UUID,
//                                         BLECharacteristic::PROPERTY_READ |
//                                         BLECharacteristic::PROPERTY_WRITE
//                                       );
//  //pCharacteristic->setValue("Hello, I'm Puntly Chronus!");
//  pService->start();
//  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
//  pAdvertising->addServiceUUID(SERVICE_UUID);
//  pAdvertising->setScanResponse(true);
//  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
//  pAdvertising->setMinPreferred(0x12);
//  BLEDevice::startAdvertising();
//}

void initSPIFFS()
{
  if (SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED))
  {
    display.clearDisplay();
    display.display();
    if (SPIFFS.exists("/config.json"))
    {
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile)
      {
        DeserializationError error = deserializeJson(doc, configFile);
        if (error)
          printText(F("Failed to read file, using default configuration"), 0, 0, 1, 1000, 0);
        configFile.close();
      }
    }
  }
  else
  {
    printText(F("An error while mounting SPIFFS"), 0, 0, 1, 1000, 0);
  }

  isDigital = getSPIFFS("isDigital.txt");
}

byte calcDayOfWeek(int d, int m, int y)
{
  static int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  y -= m < 3;
  return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7; // Sun=0, Mon=1, Tue=2, Wed=3, Thu=4, Fri=5, Sat=6
}

void createCalendar()
{
  display.clearDisplay();
  time_t now = time(nullptr);
  struct tm *p_tm;
  p_tm = localtime(&now);

  byte fev = ((p_tm->tm_year + 1900) % 4 == 0) ? 1 : 0;
  byte daysInMonth[12] = {31, 28 + fev, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  display.setTextSize(1);

  byte x = 18;
  byte y = 9;
  byte week = 0;
  display.fillRect(0, 0, display.width() - 1, 9, SSD1306_WHITE);
  String weekDays[7] = {"S", "M", "T", "W", "T", "F", "S"};
  for (byte i = 0; i < 7; i++)
  {
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    display.setCursor(x * i + 7, 1);
    display.println(String(weekDays[i]));
  }
  for (byte i = 1; i <= daysInMonth[p_tm->tm_mon]; i++)
  {
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(x * calcDayOfWeek(i, p_tm->tm_mon + 1, (p_tm->tm_year + 1900)) + 4, y * week + 10);
    if (String(i) == String(p_tm->tm_mday))
    {
      display.fillRect(x * calcDayOfWeek(i, p_tm->tm_mon + 1, (p_tm->tm_year + 1900)) + 1, y * week + 10, 18, 9, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    }
    display.println(String(i));
    if (calcDayOfWeek(i, p_tm->tm_mon + 1, (p_tm->tm_year + 1900)) == 6)
    {
      week++;
    }
  }
  for (byte i = 0; i <= 8; i++)
  {
    display.drawLine(i * x, 0, i * x, display.height(), WHITE);
    display.drawLine(0, i * y, display.width() - 2, i * y, WHITE);
  }
  display.display();
}

void getFile(String fileName)
{
  display.clearDisplay();
  display.display();
  if (SPIFFS.exists("/" + fileName))
  {
    File configFile = SPIFFS.open("/" + fileName, "r");
    while (configFile.available())
    {
      lastSavedTime = configFile.readStringUntil('\n').toInt();
      time_t now = lastSavedTime;
      struct tm *p_tm;
      p_tm = localtime(&now);

      now = mktime(p_tm);

      timeval tv;
      tv.tv_sec = now;
      timezone utc = {-3 * 3600, 3600};
      const timezone *tz = &utc;
      settimeofday(&tv, tz);

      delay(1000);
    }
    configFile.close();
  }
}

String getSPIFFS(String fileName)
{
  display.clearDisplay();
  display.display();
  if (SPIFFS.exists("/" + fileName))
  {
    File configFile = SPIFFS.open("/" + fileName, "r");
    while (configFile.available())
    {
      return configFile.readStringUntil('\n');
    }
    configFile.close();
  }
}

void saveSPIFFS(String fileName, String dados)
{
  if (SPIFFS.exists("/" + fileName))
  {
    File configFile = SPIFFS.open("/" + fileName, FILE_WRITE);
    if (configFile)
    {
      configFile.println(dados);
      configFile.close();
    }
  }
}

void setGMT()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    while (time(nullptr) == 0)
    {
      getFile("lastSavedTime.txt");
    }

    timeval tv;
    tv.tv_sec = time(nullptr);
    timezone utc = {-3 * 3600, 3600};
    const timezone *tz = &utc;
    settimeofday(&tv, tz);
  }
  else if (WiFi.status() == WL_CONNECTED)
  {
    gmtOffset_sec = byteGmtOffset * 3600;
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    byte count = 0;
    while (!time(nullptr) && count < 50)
    {
      printText(F("Setting time"), 0, 30, 1, 10, 2);
      count++;
    }
    timeval tv;
    tv.tv_sec = time(nullptr);
    timezone utc = {-3 * 3600, 3600};
    const timezone *tz = &utc;
    settimeofday(&tv, tz);
  }
}

void initWiFi()
{
  WiFi.mode(WIFI_STA);
  JsonArray arr = doc["wireless"].as<JsonArray>();

  for (JsonObject repo : arr)
  {
    const char *ssid = repo["ssid"];
    const char *passwd = repo["passwd"];

    byte count = 0;
    WiFi.begin(ssid, passwd);
    WiFi.setHostname("ChronusWatch");
    while (WiFi.status() != WL_CONNECTED && count < 10)
    {
      printText(F("Trying WiFi"), 0, 30, 1, 1000, 2);
      count++;
    }
    if (WiFi.status() == WL_CONNECTED)
    {
      printText(F("Connected to WiFi"), 0, 30, 1, 3000, 2);
      break;
    }
  }
  if (WiFi.status() != WL_CONNECTED)
  {
    printText(F("Can't connect to WiFi"), 0, 30, 1, 3000, 2);
  }
}

void initDisplay()
{
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    for (;;)
      ;
  }
  display.display();
  delay(1000);
  display.clearDisplay();
  // display.setFont(&Roboto_Mono_Thin_8);
  display.display();
  display.cp437(true);
}

const unsigned char epd_bitmap_wifi[] PROGMEM = {0x3c, 0xff, 0x81, 0x00, 0x3c, 0x42, 0x00, 0x18};
const unsigned char epd_bitmap_sino[] PROGMEM = {0x18, 0x3c, 0x3c, 0x7e, 0x7e, 0xff, 0xff, 0x18};
void printLocalTime()
{
  display.clearDisplay();
  time_t now = time(nullptr);
  struct tm *p_tm;
  p_tm = localtime(&now);

  if (p_tm->tm_hour == alarmHour && p_tm->tm_min == alarmMin && p_tm->tm_sec % 2 == 0)
  {
    display.invertDisplay(true);
  }
  else
  {
    display.invertDisplay(false);
  }

  if (isDigital)
  {
    display.clearDisplay();

    display.setTextSize(1);
    display.setCursor(73, 1);
    String wday[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    display.fillRect(0, 0, 128, 10, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    display.println((p_tm->tm_mday < 10 ? "0" : "") + String(p_tm->tm_mday) + (p_tm->tm_sec % 2 == 0 ? "/" : " ") + (p_tm->tm_mon < 10 ? "0" : "") + String(p_tm->tm_mon) + " " + wday[p_tm->tm_wday]);

    display.setTextSize(3);
    display.drawRoundRect(0, 15, 128, 49, 5, SSD1306_WHITE);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(20, 25);
    display.println((p_tm->tm_hour < 10 ? "0" : "") + String(p_tm->tm_hour) + (p_tm->tm_sec % 2 == 0 ? ":" : " ") + (p_tm->tm_min < 10 ? "0" : "") + String(p_tm->tm_min));

    display.setTextSize(1);
  }
  else
  {
    int r = 36;
    // Now draw the clock face

    display.drawCircle(display.width() / 2, display.height() / 2, 2, WHITE);
    //
    //hour ticks
    for (int z = 0; z < 360; z = z + 30)
    {
      //Begin at 0° and stop at 360°
      float angle = z;

      angle = (angle / 57.29577951); //Convert degrees to radians
      int x2 = (64 + (sin(angle) * r));
      int y2 = (32 - (cos(angle) * r));
      int x3 = (64 + (sin(angle) * (r - 5)));
      int y3 = (32 - (cos(angle) * (r - 5)));
      display.drawLine(x2, y2, x3, y3, WHITE);
    }
    // display second hand
    float angle = p_tm->tm_sec * 6;
    angle = (angle / 57.29577951); //Convert degrees to radians
    int x3 = (64 + (sin(angle) * (r)));
    int y3 = (32 - (cos(angle) * (r)));
    display.drawLine(64, 32, x3, y3, WHITE);
    //
    // display minute hand
    angle = p_tm->tm_min * 6;
    angle = (angle / 57.29577951); //Convert degrees to radians
    x3 = (64 + (sin(angle) * (r - 3)));
    y3 = (32 - (cos(angle) * (r - 3)));
    display.drawLine(64, 32, x3, y3, WHITE);
    //
    // display hour hand
    angle = p_tm->tm_hour * 30 + int((p_tm->tm_min / 12) * 6);
    angle = (angle / 57.29577951); //Convert degrees to radians
    x3 = (64 + (sin(angle) * (r - 11)));
    y3 = (32 - (cos(angle) * (r - 11)));
    display.drawLine(64, 32, x3, y3, WHITE);

    display.setTextSize(1);
    display.setCursor((display.width() / 2) + 10, (display.height() / 2) - 5);
    display.print(p_tm->tm_mday);

    display.setTextSize(1);
    display.setCursor((display.width() / 2) + 10, (display.height() / 2) + 2);
    display.print(p_tm->tm_mon + 1);

    if (toSetTime > 0)
    {
      display.setTextSize(1);
      display.setCursor(0, 56);
      display.print(p_tm->tm_year + 1900);
    }
  }

  display.setTextSize(1);
  display.setCursor(0, 17);
  if (toSetTime == 1)
    display.print("Min");
  if (toSetTime == 2)
    display.print("Hour");
  if (toSetTime == 3)
    display.print("Day");
  if (toSetTime == 4)
    display.print("Month");
  if (toSetTime == 5)
    display.print("Year");
  if (toSetTime == 6)
    display.print("Digi");

  if (WiFi.status() == WL_CONNECTED)
    display.drawBitmap(1, 1, epd_bitmap_wifi, 8, 8, 0);
  if (enableAlarm)
    display.drawBitmap(10, 1, epd_bitmap_sino, 8, 8, 0);

  // update display with all data
  display.display();
}

byte menu = 0;

#define pinB 25
#define pinA 26
#define pinC 27

byte lowestEncoder = 0;
byte changeamntEncoder = 1;
byte readingEncoder[10] = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0};
byte readingEncoderMax[10] = {8, 2, 3, 3, 1, 1, 3, 1, 1, 1};

unsigned long currentTimeEncoder;
unsigned long lastTimeEncoder;
boolean encA;
boolean encB;
boolean lastA = false;

void readEncoder(byte highestEncoder)
{
  currentTimeEncoder = millis();
  if (currentTimeEncoder >= (lastTimeEncoder))
  {
    encA = digitalRead(pinA);
    encB = digitalRead(pinB);
    if ((!encA) && (lastA))
    {
      if (encB)
      {
        if (readingEncoder[menu] + changeamntEncoder <= highestEncoder)
        {
          readingEncoder[menu] = readingEncoder[menu] + changeamntEncoder;
        }
        else
        {
          readingEncoder[menu] = lowestEncoder;
        }
      }
      else
      {
        if (readingEncoder[menu] - changeamntEncoder >= lowestEncoder)
        {
          readingEncoder[menu] = readingEncoder[menu] - changeamntEncoder;
        }
        else
        {
          readingEncoder[menu] = highestEncoder;
        }
      }
    }
    lastA = encA;
    lastTimeEncoder = currentTimeEncoder;
  }
}

void readEncoderSetTime(long changeValue)
{

  struct tm *p_tm;
  time_t nowZero = time(nullptr);
  p_tm = localtime(&nowZero);

  byte sec = p_tm->tm_sec;
  byte min = p_tm->tm_min;
  byte hou = p_tm->tm_hour;
  byte day = p_tm->tm_mday;
  byte mon = p_tm->tm_mon;
  byte yea = p_tm->tm_year;

  while (true)
  {

    if (touchRead(pinC) == 0)
    {
      if (toSetTime >= 0 && toSetTime < 7)
      {
        toSetTime += 1;
        delay(500);
      }
      if (toSetTime >= 7)
      {
        toSetTime = 0;
        delay(500);
        saveSPIFFS("isDigital.txt", isDigital ? "true" : "false");
        break;
      }
    }

    currentTimeEncoder = millis();

    if (currentTimeEncoder >= (lastTimeEncoder) + 5)
    {
      encA = digitalRead(pinA);
      encB = digitalRead(pinB);

      if ((!encA) && (lastA))
      {
        if (encB)
        {
          if (toSetTime == 1)
          {
            min += 1;
          }
          else if (toSetTime == 2)
          {
            hou += 1;
          }
          else if (toSetTime == 3)
          {
            day += 1;
          }
          else if (toSetTime == 4)
          {
            mon += 1;
          }
          else if (toSetTime == 5)
          {
            yea += 1;
          }
          else if (toSetTime == 6)
          {
            isDigital = !isDigital;
          }

          // p_tm->tm_sec = sec;
          // p_tm->tm_min = min;
          // p_tm->tm_hour = hou;
          // p_tm->tm_mday = day;
          // p_tm->tm_mon = mon;
          // p_tm->tm_year = yea;

          // nowZero = mktime(p_tm);

          // timeval tv;
          // tv.tv_sec = nowZero;
          // settimeofday(&tv, NULL);

          // delay(100);
        }
        else
        {
          // struct tm *p_tm;
          // time_t nowZero = time(nullptr);
          // p_tm = localtime(&nowZero);

          // byte sec = p_tm->tm_sec;
          // byte min = p_tm->tm_min;
          // byte hou = p_tm->tm_hour;
          // byte day = p_tm->tm_mday;
          // byte mon = p_tm->tm_mon;
          // byte yea = p_tm->tm_year;

          if (toSetTime == 1)
          {
            min -= 1;
          }
          else if (toSetTime == 2)
          {
            hou -= 1;
          }
          else if (toSetTime == 3)
          {
            day -= 1;
          }
          else if (toSetTime == 4)
          {
            mon -= 1;
          }
          else if (toSetTime == 5)
          {
            yea -= 1;
          }
          else if (toSetTime == 6)
          {
            isDigital = !isDigital;
          }
        }
      }

      lastA = encA;
      lastTimeEncoder = currentTimeEncoder;
    }
    display.clearDisplay();
    display.drawRoundRect(0, 15, 128, 49, 5, SSD1306_WHITE);
    display.setTextColor(SSD1306_WHITE);

    display.setTextSize(1);
    display.setCursor(73, 1);
    display.println((millis() % 2 == 0 && toSetTime == 3 ? "  " : (day < 10 ? "0" : "") + String(day)) + "/" + (millis() % 2 == 0 && toSetTime == 4 ? "  " : (mon < 10 ? "0" : "") + String(mon)) + " " + (millis() % 2 == 0 && toSetTime == 6 ? "  " : String(yea+1900)));

    display.setTextSize(3);
    display.setCursor(20, 25);
    display.println((hou < 10 ? "0" : "") + String(hou) + ":" + (min < 10 ? "0" : "") + String(min));

    if (toSetTime == 1)
    {
      display.drawRect(72, 23, 37, 25, SSD1306_WHITE);
    }
    else if (toSetTime == 2)
    {
      display.drawRect(18, 23, 37, 25, SSD1306_WHITE);
    }

    display.display();
  }
  p_tm->tm_sec = sec;
  p_tm->tm_min = min;
  p_tm->tm_hour = hou;
  p_tm->tm_mday = day;
  p_tm->tm_mon = mon;
  p_tm->tm_year = yea;

  delay(500);

  nowZero = mktime(p_tm);
  Serial.println(nowZero);

  timeval tv;
  tv.tv_sec = nowZero;
  timezone utc = {-3 * 3600, 3600};
  const timezone *tz = &utc;
  settimeofday(&tv, tz);
}

void readEncoderSetAlarm(long changeValue)
{
  currentTimeEncoder = millis();
  if (currentTimeEncoder >= (lastTimeEncoder) + 5)
  {
    encA = digitalRead(pinA);
    encB = digitalRead(pinB);

    if ((!encA) && (lastA))
    {
      if (encB)
      {
        struct tm *p_tm;
        time_t nowZero = time(nullptr);
        p_tm = localtime(&nowZero);

        if (changeValue == 1)
        {
          if (alarmMin < 60)
            alarmMin += 1;
          else
            alarmMin = 0;
        }
        else if (changeValue == 2)
        {
          if (alarmHour < 24)
            alarmHour += 1;
          else
            alarmHour = 0;
        }

        delay(100);
      }
      else
      {
        struct tm *p_tm;
        time_t nowZero = time(nullptr);
        p_tm = localtime(&nowZero);

        if (changeValue == 1)
        {
          if (alarmMin > 0)
            alarmMin -= 1;
          else
            alarmMin = 59;
        }
        else if (changeValue == 2)
        {
          if (alarmHour > 0)
            alarmHour -= 1;
          else
            alarmHour = 23;
        }

        delay(100);
      }
    }

    lastA = encA;
    lastTimeEncoder = currentTimeEncoder;
  }
}

String httpGETRequest(const char *serverName)
{
  WiFiClient client;
  HTTPClient http;

  // Your Domain name with URL path or IP address with path
  http.begin(client, serverName);

  // Send HTTP POST request
  int httpResponseCode = http.GET();

  String payload = "{}";

  if (httpResponseCode > 0)
  {
    // Serial.print("HTTP Response code: ");
    // Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else
  {
    payload = "{error: \"error\"}";
    // Serial.print("Error code: ");
    // Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}

void getWeather()
{
  String endpoint = "http://api.openweathermap.org/data/2.5/weather?id=" + weatherId + "&units=metric&APPID=f1f3dfaf4301e0686904b2057957ddc9";

  DeserializationError error = deserializeJson(weather, httpGETRequest(endpoint.c_str()));
  if (error || weather["error"])
    printText(F("Error to get Weather"), 0, 30, 1, 3000, 2);

  String opcoesMenu[8] = {F("<Weather"), weather["weather"]["description"], F("<Min"), weather["main"]["temp_min"], F("<Max"), weather["main"]["temp_max"], F("<Humidity"), weather["main"]["humidity"]};
  printTextArray(opcoesMenu);
}

void pinsSetup()
{

  pinMode(pinA, INPUT_PULLUP);
  pinMode(pinB, INPUT_PULLUP);
  //  pinMode(pinC, INPUT_PULLDOWN);

  touchAttachInterrupt(T7, callback, Threshold);
  esp_sleep_enable_touchpad_wakeup();
}

//void serverSetup() {
//  server.on("/info", HTTP_GET, [](AsyncWebServerRequest * request) {
//    request->send(200, "text/plain", "Hi! I am Puntly Chronus.");
//    printText("hello from server!", 4, 24, 1, 3000, 2);
//  });
//
//  server.serveStatic("/", SPIFFS, "/");
////  AsyncElegantOTA.begin(&server);    // Start ElegantOTA
//  server.begin();
//}

void otaConfigure(void)
{
  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  ArduinoOTA.setHostname("ChronusWatch");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA
      .onStart([]()
               {
                 String type;
                 if (ArduinoOTA.getCommand() == U_FLASH)
                   type = "sketch";
                 else // U_SPIFFS
                   type = "filesystem";
               })
      .onEnd([]()
             { printText("Updated!", 30, 24, 1, 500, 2); })
      .onProgress([](unsigned int progress, unsigned int total)
                  {
                    display.clearDisplay();
                    // printText(String(progress / (total / 100)) + "%", 30, 24, 1, 1, 2, false);

                    display.setTextSize(1);

                    display.setCursor(31, 15);
                    display.println("Updating...");

                    display.drawRect(14, 25, 100, 10, SSD1306_WHITE);
                    display.fillRect(14, 25, progress / (total / 100), 10, SSD1306_WHITE);

                    const int newX = 64 - ((String(String(progress / (total / 100)) + "%").length() * 6) / 2);
                    display.setCursor(newX, 40);
                    display.println(String(progress / (total / 100)) + "%");

                    display.display();
                  })
      .onError([](ota_error_t error)
               {
                 if (error == OTA_AUTH_ERROR)
                   printText("Auth Failed", 30, 24, 1, 3000, 2);
                 else if (error == OTA_BEGIN_ERROR)
                   printText("Begin Failed", 30, 24, 1, 3000, 2);
                 else if (error == OTA_CONNECT_ERROR)
                   printText("Connect Failed", 30, 24, 1, 3000, 2);
                 else if (error == OTA_RECEIVE_ERROR)
                   printText("Receive Failed", 30, 24, 1, 3000, 2);
                 else if (error == OTA_END_ERROR)
                   printText("End Failed", 30, 24, 1, 3000, 2);
               });

  ArduinoOTA.begin();
}

void saveLastTime()
{
  time_t now = time(nullptr);
  struct tm *p_tm;
  p_tm = localtime(&now);

  lastSavedTime = time(nullptr);

  saveSPIFFS("lastSavedTime.txt", String(lastSavedTime));
}

void setup(void)
{
  Serial.begin(115200);

  initDisplay();
  initSPIFFS();
  generalConfig();
  //  configBLE();
  initWiFi();
  setGMT();
  pinsSetup();
  //  serverSetup();
  //  getWeather();
  otaConfigure();

  printText("Welcome!", 30, 24, 1, 3000, 2);
}
//void bleShow(){
//  std::string value = pCharacteristic->getValue();
//  if(value != "") {
//    // Serial.println(value.c_str());
//    if(value[0] == '#'){
//        // Serial.println(String(value.c_str()));
//      if(String(value.c_str()).indexOf("t") > 0){
//        for(int i = 2; i< value.length(); i++){
//          timeBLE += value[i];
//        }
//        // Serial.println(timeBLE);
//        noTime = true;
//      }
//    } else {
//      printText(value.c_str(), 0, 0, 1, 3000, 0);
//      pCharacteristic->setValue("");
//    }
//    pCharacteristic->setValue("");
//    value = "";
//  }

//}
void toReadEncoder()
{
  if (toSetTime == 0 && configAlarm == 0)
  {
    readEncoder(readingEncoderMax[menu]);
  }
  else if (toSetTime != 0 && configAlarm == 0)
  {
    readEncoderSetTime(toSetTime);
    saveLastTime();
  }
  else if (toSetTime == 0 && configAlarm != 0)
  {
    readEncoderSetAlarm(configAlarm);
  }
}

void toMenu(byte de, byte para)
{
  readingEncoder[de] = 0;
  menu = para;
  delay(500);
}

void menu0()
{
  if (menu == 0)
  {
    if (readingEncoder[menu] == 0)
    {
      display.clearDisplay();
      display.display();
    }
    if (readingEncoder[menu] == 1)
    {
      printLocalTime();
      if (touchRead(pinC) == 0)
      {
        saveLastTime();
        esp_light_sleep_start();
        toMenu(menu, 0);
        toMenu(0, 0);
      }
    }
    if (readingEncoder[menu] == 2)
    {
      String opcoesMenu[8] = {F("  Home"), F("> Config WiFi"), F("  Power"), F("  Set Date & time"), F("  Calendar"), F("  Weather"), F("  Alarm"), F("")};
      printTextArray(opcoesMenu);
      if (touchRead(pinC) == 0)
      {
        toMenu(1, 1);
      }
    }
    if (readingEncoder[menu] == 3)
    {
      String opcoesMenu[8] = {F("  Home"), F("  Config WiFi"), F("> Power"), F("  Set Date & time"), F("  Calendar"), F("  Weather"), F("  Alarm"), F("")};
      printTextArray(opcoesMenu);
      if (touchRead(pinC) == 0)
      {
        toMenu(2, 2);
      }
    }
    if (readingEncoder[menu] == 4)
    {
      if (toSetTime == 0)
      {
        String opcoesMenu[8] = {F("  Home"), F("  Config WiFi"), F("  Power"), F("> Set Date & time"), F("  Calendar"), F("  Weather"), F("  Alarm"), F("")};
        printTextArray(opcoesMenu);
      }
      // else
      // {
      //   printLocalTime();
      // }

      if (touchRead(pinC) == 0)
      {
        if (toSetTime >= 0 && toSetTime < 7)
        {
          toSetTime += 1;
          delay(500);
        }
        if (toSetTime >= 7)
        {
          toSetTime = 0;
          delay(500);
          saveSPIFFS("isDigital.txt", isDigital ? "true" : "false");
        }
      }
    }
    if (readingEncoder[menu] == 5)
    {
      String opcoesMenu[8] = {F("  Home"), F("  Config WiFi"), F("  Power"), F("  Set Date & time"), F("> Calendar"), F("  Weather"), F("  Alarm"), F("")};
      printTextArray(opcoesMenu);
      if (touchRead(pinC) == 0)
      {
        toMenu(4, 4);
      }
    }
    if (readingEncoder[menu] == 6)
    {
      String opcoesMenu[8] = {F("  Home"), F("  Config WiFi"), F("  Power"), F("  Set Date & time"), F("  Calendar"), F("> Weather"), F("  Alarm"), F("")};
      printTextArray(opcoesMenu);
      if (touchRead(pinC) == 0)
      {
        toMenu(5, 5);
      }
    }
    if (readingEncoder[menu] == 7)
    {
      String opcoesMenu[8] = {F("  Home"), F("  Config WiFi"), F("  Power"), F("  Set Date & time"), F("  Calendar"), F("  Weather"), F("> Alarm"), F("")};
      printTextArray(opcoesMenu);
      if (touchRead(pinC) == 0)
      {
        toMenu(6, 6);
      }
    }
    if (readingEncoder[menu] == 8)
    {
      String opcoesMenu[8] = {F("  Home"), F("  Config WiFi"), F("  Power"), F("  Set Date & time"), F("  Calendar"), F("  Weather"), F("  Alarm"), F(">")};
      printTextArray(opcoesMenu);
      if (touchRead(pinC) == 0)
      {
        delay(500);
      }
    }
  }
}

void menu1()
{
  if (menu == 1)
  {
    if (readingEncoder[menu] == 0)
    {
      String opcoesMenu[8] = {F("<Config WiFi"), F("> Back"), F("  Set WiFi off"), F("  Set WiFi on"), F(""), F(""), F(""), F("")};
      printTextArray(opcoesMenu);
      if (touchRead(pinC) == 0)
      {
        toMenu(menu, 0);
      }
    }
    if (readingEncoder[menu] == 1)
    {
      String opcoesMenu[8] = {F("<Config WiFi"), F("  Back"), F("> Set WiFi off"), F("  Set WiFi on"), F(""), F(""), F(""), F("")};
      printTextArray(opcoesMenu);
      if (touchRead(pinC) == 0)
      {
        setWifiOff();
        toMenu(menu, 0);
      }
    }
    if (readingEncoder[menu] == 2)
    {
      String opcoesMenu[8] = {F("<Config WiFi"), F("  Back"), F("  Set WiFi off"), F("> Set WiFi on"), F(""), F(""), F(""), F("")};
      printTextArray(opcoesMenu);
      if (touchRead(pinC) == 0)
      {
        initWiFi();
        toMenu(menu, 0);
      }
    }
  }
}

void menu2()
{
  if (menu == 2)
  {
    if (readingEncoder[menu] == 0)
    {
      String opcoesMenu[8] = {F("<Power"), F("> Back"), F("  Turn off"), F("  Low power mode"), F("  Restart"), F(""), F(""), F("")};
      printTextArray(opcoesMenu);
      if (touchRead(pinC) == 0)
      {
        readingEncoder[menu] = 0;
        toMenu(menu, 0);
      }
    }
    if (readingEncoder[menu] == 1)
    {
      String opcoesMenu[8] = {F("<Power"), F("  Back"), F("> Turn off"), F("  Low power mode"), F("  Restart"), F(""), F(""), F("")};
      printTextArray(opcoesMenu);
      if (touchRead(pinC) == 0)
      {
        saveLastTime();
        printText(F("Bye!"), 0, 30, 1, 2000, 2);
        delay(1000);
        display.clearDisplay();
        display.display();
        esp_deep_sleep_start();
      }
    }
    if (readingEncoder[menu] == 2)
    {
      String opcoesMenu[8] = {F("<Power"), F("  Back"), F("  Turn off"), F("> Low power mode"), F("  Restart"), F(""), F(""), F("")};
      printTextArray(opcoesMenu);
      if (touchRead(pinC) == 0)
      {
        saveLastTime();
        setWifiOff();
        esp_light_sleep_start();
        toMenu(menu, 0);
        toMenu(0, 0);
      }
    }
    if (readingEncoder[menu] == 3)
    {
      String opcoesMenu[8] = {F("<Power"), F("  Back"), F("  Turn off"), F("  Low power mode"), F("> Restart"), F(""), F(""), F("")};
      printTextArray(opcoesMenu);
      if (touchRead(pinC) == 0)
      {
        saveLastTime();
        printText(F("Please, wait..."), 0, 30, 1, 2000, 2);
        ESP.restart();
        toMenu(menu, 0);
      }
    }
  }
}

void menu3()
{
  if (menu == 3)
  {
    if (readingEncoder[menu] == 0)
    {
      String opcoesMenu[8] = {F("<Set Date & time"), F("> Back"), F("  Fix Date & Time"), F("  Set min date Oct 2"), F("  Refresh date & time"), F(""), F(""), F("")};
      printTextArray(opcoesMenu);
      if (touchRead(pinC) == 0)
      {
        toMenu(menu, 0);
      }
    }
    if (readingEncoder[menu] == 1)
    {
      String opcoesMenu[8] = {F("<Set Date & time"), F("  Back"), F("> Fix Date & Time"), F("  Set min date Oct 2"), F("  Refresh date & time"), F(""), F(""), F("")};
      printTextArray(opcoesMenu);
      if (touchRead(pinC) == 0)
      {
        toMenu(menu, 0);
      }
    }
    if (readingEncoder[menu] == 2)
    {
      String opcoesMenu[8] = {F("<Set Date & time"), F("  Back"), F("  Fix Date & Time"), F("> Set min date Oct 2"), F("  Refresh date & time"), F(""), F(""), F("")};
      printTextArray(opcoesMenu);
      if (touchRead(pinC) == 0)
      {
        noTime = true;
        setGMT();
        saveLastTime();
        toMenu(menu, 0);
      }
    }
    if (readingEncoder[menu] == 3)
    {
      String opcoesMenu[8] = {F("<Set Date & time"), F("  Back"), F("  Fix Date & Time"), F("  Set min date Oct 2"), F("> Refresh date & time"), F(""), F(""), F("")};
      printTextArray(opcoesMenu);
      if (touchRead(pinC) == 0)
      {
        noTime = false;
        setGMT();
        saveLastTime();
        toMenu(menu, 0);
      }
    }
  }
}

void menu4()
{
  if (menu == 4)
  {
    createCalendar();
    if (touchRead(pinC) == 0)
    {
      toMenu(menu, 0);
    }
  }
}

void menu5()
{
  if (menu == 5)
  {
    getWeather();
    if (touchRead(pinC) == 0)
    {
      toMenu(menu, 0);
    }
  }
}

void menu6()
{
  if (menu == 6)
  {
    if (readingEncoder[menu] == 0)
    {
      String opcoesMenu[8] = {F("<Alarm"), F("> Back"), F("  Set alarm"), enableAlarm ? F("  Disable alarm") : F("  Enable alame"), F(""), F(""), F(""), F("")};
      printTextArray(opcoesMenu);
      if (touchRead(pinC) == 0)
      {
        toMenu(menu, 0);
      }
    }
    if (readingEncoder[menu] == 1)
    {
      if (configAlarm == 0)
      {
        String opcoesMenu[8] = {F("<Alarm"), F("  Back"), F("> Set alarm"), enableAlarm ? F("  Disable alarm") : F("  Enable alame"), F(""), F(""), F(""), F("")};
        printTextArray(opcoesMenu);
      }
      else
      {
        display.clearDisplay();
        display.drawRoundRect(0, 15, 128, 49, 5, SSD1306_WHITE);
        display.setTextColor(SSD1306_WHITE);
        display.setTextSize(1);
        display.setCursor(42, 1);
        display.println("Set Alarm");
        display.setTextSize(3);
        display.setCursor(20, 25);
        display.println((alarmHour < 10 ? "0" : "") + String(alarmHour) + ":" + (alarmMin < 10 ? "0" : "") + String(alarmMin));
        display.drawRect(configAlarm == 1 ? 72 : 18, 23, 37, 25, SSD1306_WHITE);
        display.display();
      }

      if (touchRead(pinC) == 0)
      {
        if (configAlarm >= 0 && configAlarm < 3)
        {
          configAlarm += 1;
          delay(500);
        }
        if (configAlarm >= 3)
        {
          configAlarm = 0;
          delay(500);
        }
      }
    }
    if (readingEncoder[menu] == 2)
    {
      String opcoesMenu[8] = {F("<Alarm"), F("  Back"), F("  Set alarm"), enableAlarm ? F("> Disable alarm") : F("> Enable alame"), F(""), F(""), F(""), F("")};
      printTextArray(opcoesMenu);
      if (touchRead(pinC) == 0)
      {
        enableAlarm = !enableAlarm;
        toMenu(menu, 0);
      }
    }
  }
}
void loop(void)
{
  ArduinoOTA.handle();
  toReadEncoder();
  menu0();
  menu1();
  menu2();
  menu3();
  menu4();
  menu6();
}
