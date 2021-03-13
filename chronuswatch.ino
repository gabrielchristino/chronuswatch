#include <Wire.h>
#include <Update.h>
#include <time.h>
#include <Arduino_JSON.h>


///////////////////////////////////////////CONNECTIONS
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>


///////////////////////////////////////////BLUETOOTH
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

String valor;

WebServer server(80);
HTTPClient http;

const char* host = "Puntly";
const char* ssid = "g";
const char* password = "ca";

JSONVar configObj;
JSONVar myWeather;


///////////////////////////////////////////WEATHER
String endpoint = "http://api.openweathermap.org/data/2.5/weather?id=3448439&units=metric&APPID=f1f3dfaf4301e0686904b2057957ddc9";
String payloadWeather;
String payloadConfigJson;

///////////////////////////////////////////NTP
const char* ntpServer = "pool.ntp.org";
long  gmtOffset_sec = -3*3600;
const int   daylightOffset_sec = 3600;


///////////////////////////////////////////DISPLAY
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


///////////////////////////////////////////SPIFFS
#include <SPIFFS.h>
#include <FS.h>

File configFile;
#define FORMAT_SPIFFS_IF_FAILED true



///////////////////////////////////////////VAR
bool showMenu = false;
int getLocalVersion = 0;

///////////////////////////////////////////ENCODER
  int reading = 1;
  int lowest = 0;
  int highest = 8;
  int changeamnt = 1;
  // Timing for polling the encoder
  unsigned long currentTime;
  unsigned long lastTime;
  unsigned long lastTime2;
  byte screenTime = 3;
  // Pin definitions
  const int pinA = 26;
  const int pinB = 25;

  // Storing the readings
  boolean encA;
  boolean encB;
  boolean lastA = false;

  int level = 0;
  int readLevel[5]= {1,0,0,0,0};

  #define Threshold 40

///////////////////////////////////////////OTA HTML PAGE
const char* serverIndex =
"<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
"<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
   "<input type='file' name='update'>"
        "<input type='submit' value='Update'>"
    "</form>"
 "<div id='prg'>progress: 0%</div>"
 "<script>"
  "$('form').submit(function(e){"
  "e.preventDefault();"
  "var form = $('#upload_form')[0];"
  "var data = new FormData(form);"
  " $.ajax({"
  "url: '/update',"
  "type: 'POST',"
  "data: data,"
  "contentType: false,"
  "processData:false,"
  "xhr: function() {"
  "var xhr = new window.XMLHttpRequest();"
  "xhr.upload.addEventListener('progress', function(evt) {"
  "if (evt.lengthComputable) {"
  "var per = evt.loaded / evt.total;"
  "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
  "}"
  "}, false);"
  "return xhr;"
  "},"
  "success:function(d, s) {"
  "console.log('success!')"
 "},"
 "error: function (a, b, c) {"
 "}"
 "});"
 "});"
 "</script>";

void callback(){
  //placeholder callback function
}

bool waitATime(int sec){
  unsigned long now = millis();
  unsigned long last=0;

  if(now>=last+sec){
    now = millis();
    return true;
  }
  last = now;
}







unsigned long lastTimeKey = 0;
unsigned long currentTimeKey;
int encAKey;
int encBKey;
int lastAKey;
int readingKey = 0;
int changeamntKey = 1;
int highestKey = 44;
int lowestKey = 0;
int C = 100;
int lastC = 100;
String caracteres[] = {"A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P","Q","R","S","T","U","V","W","X","Y","Z"," ",".",",","?","0","1","2","3","4","5","6","7","8","9","+","-","*","/","SAIR"};

String typeText(){
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.cp437(true);
  String texto = "";
  while(true){
    
  display.clearDisplay();
  // Read elapsed time
    currentTimeKey = millis();
  
    // Check if it's time to read
    if(currentTimeKey >= (lastTimeKey + 5)) {
      // read the two pins
      encAKey = digitalRead(pinA);
      encBKey = digitalRead(pinB);
      C = touchRead(T7);
  
      // check if A has gone from high to low
      if ((!encAKey) && (lastAKey)) {
        // check if B is high
        if (encBKey) {
          // clockwise
          if (readingKey + changeamntKey <= highestKey) {
            readingKey = readingKey + changeamntKey; 
          } else {
            readingKey = lowestKey;
          }
        }
        else
        {
          // anti-clockwise
          if (readingKey - changeamntKey >= lowest) {
            readingKey = readingKey - changeamntKey; 
          } else {
            readingKey = highestKey;
          }
        }
      }
      // store reading of A and millis for next loop
      lastAKey = encAKey;
      lastTimeKey = currentTimeKey;
    }
            if(C == 0 && lastC >10){
              if (caracteres[readingKey] == "SAIR"){break;}
              texto = texto + caracteres[readingKey];
            }

            display.setTextSize(1);
            display.setCursor(0, 0);
            display.println(caracteres[readingKey]);
            display.setCursor(0, 20);
            display.println(texto);
            display.setCursor(0, 30);
            display.println(C);
            display.display();
            lastC = C;
  }

  return texto;
}

///////////////////////////////////////////PRINT TEXT
void printText(String cls, int tSize, int x, int y, String white, String text) {
  if(cls=="true"){display.clearDisplay();}

  if(white=="true"){display.setTextColor(SSD1306_WHITE);}else{display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);}

  display.setTextSize(tSize);
  display.setCursor(x, y);
  display.println(text);
  display.display();
}

///////////////////////////////////////////SHOW AN ICON WITH TEXT
void showIcons(String text, int icon){
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.cp437(true);
  display.setTextSize(5);
  display.setCursor(55, 0); 
  display.write(icon);

  int left = (128-text.length()*6)/2;

  printText("false",1,left,50,"true",text);
}

///////////////////////////////////////////INVERT COLOR
void invertColor(bool invertD){
  display.invertDisplay(invertD);
}

///////////////////////////////////////////SHOW MENU
void showMenuDots(){
  for(int16_t i=1; i<9; i+=1) {
    display.fillRect(120, i*8 - 8, 4, 4, 1);
    display.fillRect(121, i*8-7, 2, 2, (i==reading?1:0));
  }
  display.display();
}
void printNetwork(){
    printText("true", 1, 0, 0, "false", F("       Network       "));
    
    printText("false", 1, 0, 15, "true", F("You are connected to:"));
    printText("false", 1, 0, 25, "true", ssid);
    printText("false", 1, 0, 35, "true", F("Your IP address is:"));
    printText("false", 1, 0, 45, "true", WiFi.localIP().toString().c_str());
}

String httpGETRequest(const char* serverName) {
  HTTPClient http;
    
  http.begin(serverName);
  
  int httpResponseCode = http.GET();
  
  String payload = "{}"; 
  while(1){
    if (httpResponseCode>0) {
      payload = http.getString();
      break;
    }
  }
  http.end();

  return payload;
}

///////////////////////////////////////////GET WEATHER
void getWeather(){
      printText("true", 1, 0, 0, "false", F("       Weather       "));
      payloadWeather = payloadWeather.length()>0?payloadWeather: httpGETRequest(endpoint.c_str());
      myWeather = JSON.parse(payloadWeather);
  
      if (JSON.typeof(myWeather) == "undefined") {
        showIcons(F("Error to get weather"), 19); 
        return;
      }
      
      const char* description = myWeather["weather"][0]["description"];
      double gettemp = myWeather["main"]["temp"];
      int iconTemp = myWeather["weather"][0]["id"];
      display.setTextColor(SSD1306_WHITE);
      //display.clearDisplay();
      display.cp437(true);

      if(iconTemp==800){
        display.setCursor(0, 8);
        display.print(F("         \\__/"));
        display.setCursor(0, 16);
        display.print(F("        _/  \\_"));
        display.setCursor(0, 24);
        display.print(F("         \\__/"));
        display.setCursor(0, 32);
        display.print(F("         /  \\"));
      }else{
        switch(iconTemp/100){
          case 2://Thunderstorm
            display.setCursor(0, 8);
            display.print(F("         ._."));
            display.setCursor(0, 16);
            display.print(F("        (   )."));
            display.setCursor(0, 24);
            display.print(F("       (___(__)"));
            display.setCursor(0, 32);
            display.print(F("       ,'//,'\\"));
            display.setCursor(0, 40);
            display.print(F("       ,\\/,'',\\/"));
            break;
          case 3://Drizzle
          case 5://Rain
            display.setCursor(0, 8);
            display.print(F("         ._."));
            display.setCursor(0, 16);
            display.print(F("        (   )."));
            display.setCursor(0, 24);
            display.print(F("       (___(__)"));
            display.setCursor(0, 32);
            display.print(F("       ' ' ' ' '"));
            display.setCursor(0, 40);
            display.print(F("        ' ' ' '"));
            break;
          case 6://Snow
            display.setCursor(0, 8);
            display.print(F("         ._."));
            display.setCursor(0, 16);
            display.print(F("        (   )."));
            display.setCursor(0, 24);
            display.print(F("       (___(__)"));
            display.setCursor(0, 32);
            display.print(F("       * * * * *"));
            display.setCursor(0, 40);
            display.print(F("        * * * *"));
            break;
          case 7://Sun w Clouds
            display.setCursor(0, 8);
            display.print(F("      _`/''._."));
            display.setCursor(0, 16);
            display.print(F("       '\\_(   )."));
            display.setCursor(0, 24);
            display.print(F("        /(___(__)"));
            break;
          case 8://Clouds
            display.setCursor(0, 8);
            display.print(F("         ._."));
            display.setCursor(0, 16);
            display.print(F("        (   )."));
            display.setCursor(0, 24);
            display.print(F("       (___(__)"));
            break;
          default://Sun w Clouds
            display.setCursor(0, 8);
            display.print(F("      _`/''._."));
            display.setCursor(0, 16);
            display.print(F("       '\\_(   )."));
            display.setCursor(0, 24);
            display.print(F("        /(___(__)"));
            break;
        }
      }
      
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);

      display.setCursor(0, 48);
      display.println(description);
      
      display.setCursor(0, 56);
      display.print(F("Current temp "));
      display.print(gettemp);
      display.write(248);
      display.print(F("C "));

      display.display();
}


///////////////////////////////////////////TIME
void setGMT(){
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  while(!time(nullptr)){
    showIcons(F("Updating"), 225);
    delay(500);
  }
}
    
void printLocalTime() {
  display.clearDisplay();
  time_t now = time(nullptr);
  struct tm* p_tm = localtime(&now);
  int r = 36;
  // Now draw the clock face
   
  display.drawCircle(display.width()/2, display.height()/2, 2, WHITE);
  //
  //hour ticks
  for( int z=0; z < 360;z= z + 30 ){
      //Begin at 0° and stop at 360°
      float angle = z ;
       
      angle=(angle/57.29577951) ; //Convert degrees to radians
      int x2=(64+(sin(angle)*r));
      int y2=(32-(cos(angle)*r));
      int x3=(64+(sin(angle)*(r-5)));
      int y3=(32-(cos(angle)*(r-5)));
      display.drawLine(x2,y2,x3,y3,WHITE);
  }
  // display second hand
  float angle = p_tm->tm_sec*6 ;
  angle=(angle/57.29577951) ; //Convert degrees to radians
  int x3=(64+(sin(angle)*(r)));
  int y3=(32-(cos(angle)*(r)));
  display.drawLine(64,32,x3,y3,WHITE);
  //
  // display minute hand
  angle = p_tm->tm_min * 6 ;
  angle=(angle/57.29577951) ; //Convert degrees to radians
  x3=(64+(sin(angle)*(r-3)));
  y3=(32-(cos(angle)*(r-3)));
  display.drawLine(64,32,x3,y3,WHITE);
  //
  // display hour hand
  angle = p_tm->tm_hour * 30 + int((p_tm->tm_min / 12) * 6 );
  angle=(angle/57.29577951) ; //Convert degrees to radians
  x3=(64+(sin(angle)*(r-11)));
  y3=(32-(cos(angle)*(r-11)));
  display.drawLine(64,32,x3,y3,WHITE);
   
  display.setTextSize(1);
  display.setCursor((display.width()/2)+10,(display.height()/2) - 3);
  display.print(p_tm->tm_mday);
   
  // update display with all data
  display.display();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////GET GIT
void getUpdateGit(){
  showIcons(F("Updating..."), 225);
  String getGitHtml = "https://raw.githubusercontent.com/gabrielchristino/chronuswatch/main/data/config.json";
  payloadConfigJson = httpGETRequest(getGitHtml.c_str());
  JSONVar myGit = JSON.parse(payloadConfigJson);
  
  if (JSON.typeof(myGit) == "undefined") {
    showIcons(F("Error to get git"), 19); 
    return;
  }

  int getVersion = myGit["version"];

  if(getLocalVersion!=getVersion){
    configFile = SPIFFS.open("/config.json",FILE_WRITE);
    configFile.print(payloadConfigJson);
    configFile.close();
    showIcons(F("Update Success!"), 225);
    ESP.restart();
  }else{
    showIcons(F("There's no new update"), 225);
    
    if(waitATime(1500)){
      reading=8;
    }
    
  }
  
///////////////////////////////////////////////////////////////////////////////////////////////////////////
}

void showWatchFace(String from){
  for(int16_t i=0; i<configObj[from]["face"].length(); i+=1) {
    JSONVar face = configObj[from]["face"][i];  
    int type = face["type"];
     
//    if(type == 0){
//      JSONVar src = face["s"];
//      int arraysize = face["s"].length();
//      int y = face["y"];
//      int w = face["w"];
//      int x = face["x"];
//      int h = face["h"];
//
//      for(int x=0;x<arraysize;x+=1){
//        for(int wA=x; wA<x+w; wA+=1){
//          for(int hA=y; hA<y+h; hA+=1){
//            int n = src[x];
//            display.drawPixel(wA, hA, n);
//          }
//        }
//      }
//    }
    if(type == 0){
      int y = face["y"];
      int x = face["x"];
      int s = face["s"];
      const char* i = face["i"];
      const char* c = face["c"];
      const char* text = face["t"];
      printText(c, s, x ,y, i, text);
    }
    if(type >= 1 && type <= 7){
      int y = face["y"];
      int x = face["x"];
      int s = face["s"];
      const char* i = face["i"];
      const char* c = face["c"];

      time_t now = time(nullptr);
      struct tm* p_tm = localtime(&now);

      int tm = 0;
      switch(type){
        case 1:
          tm = p_tm->tm_sec;
          break;
        case 2:
          tm = p_tm->tm_min;
          break;
        case 3:
          tm = p_tm->tm_hour;
          break;
        case 4:
          tm = p_tm->tm_mday;
          break;
        case 5:
          tm = p_tm->tm_wday;
          break;
        case 6:
          tm = (p_tm->tm_mon)+1;
          break;
        case 7:
          tm = p_tm->tm_year;
          break;
        default:
          tm = 0;
          break;
      }
      printText(c, s, x ,y, i, String(tm));
    }
    if(type == 8){
      int y = face["y"];
      int x = face["x"];
      int w = face["w"];
      int h = face["h"];
      int r = face["r"];
      int color = face["color"];
      bool fill = face["fill"];
      if(fill){
        display.drawRoundRect(x, y, w, h, r, color);
      } else {
        display.fillRoundRect(x, y, w, h, r, color);
      }
      
      display.display();
    }
    if(type == 10){
      int y = face["y"];
      int x = face["x"];
      int param1 = face["param1"];
      int param2 = face["param2"];

      const char* param = myWeather[param1][param2];
      
      const char* i = face["i"];
      const char* c = face["c"];
      const char* text = face["t"];
    }

  }
}
///////////////////////////////////////////BLUETOOTH
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();

      if (value.length() > 0) {
        valor = "";
        for (int i = 0; i < value.length(); i++){
          // Serial.print(value[i]); // Presenta value.
          valor = valor + value[i];
        }
        printText("true", 1, 0, 0, "false", "   New notification  ");
        printText("false", 1, 0, 9, "true", valor);
      }
    }
};


void configBLE() {
  showIcons(F("Wait for BLE"), 225); 

  BLEDevice::init("PuntlyChronus");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->setValue("Hello Puntly Chronus");
  pService->start();

  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();
}


void setup() {
  Serial.begin(115200);
  pinMode(pinA, INPUT_PULLUP);
  pinMode(pinB, INPUT_PULLUP);
  currentTime = millis();
  lastTime = currentTime; 
  touchAttachInterrupt(T7, callback, Threshold);
  esp_sleep_enable_touchpad_wakeup();
///////////////////////////////////////////DISPLAY
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for(;;);
  }
  display.display();
  display.clearDisplay();

///////////////////////////////////////////SPIFFS
  if(!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)){
    showIcons(F("SPIFFS error"), 19); 
    delay(500);
  }
  
  configFile = SPIFFS.open("/config.json",FILE_READ);
  String line = configFile.readStringUntil('\n');
  
  configObj = JSON.parse(line);
  configFile.close();

  getLocalVersion = configObj["version"];

  gmtOffset_sec = configObj["configuration"]["gmt"];
  gmtOffset_sec = gmtOffset_sec*3600;
  
  bool invertD = configObj["configuration"]["invertDisplay"];
  invertColor(invertD);

  int weatherId = configObj["configuration"]["weatherId"];
  endpoint = "http://api.openweathermap.org/data/2.5/weather?id=" + String(weatherId) + "&units=metric&APPID=f1f3dfaf4301e0686904b2057957ddc9";
  
  configBLE();
///////////////////////////////////////////WIFI
  JSONVar wifiConfig = configObj["wifi"];
  bool conectado = false;
  for(int16_t i=0; i<wifiConfig.length(); i+=1) {
    ssid = configObj["wifi"][i]["ssid"];
    password = configObj["wifi"][i]["password"];
  
    WiFi.begin(ssid , password);
    showIcons(F("WiFi"), 225);
    int tentativas = 0;

    while (WiFi.status() != WL_CONNECTED && tentativas < 10) {
      showIcons(ssid , 225);
      delay(500);
      tentativas++;
    }
    if(WiFi.status() == WL_CONNECTED){conectado=true;break;}
  }

  if(!conectado){
///////////////////////////////////////////AP IF CAN'T CONNECT TO WIFI
    WiFi.softAP("Puntly","12345678",1,13);
    printText("true", 1, 0 ,0, "true", F("Can't connect to WiFi"));
    delay(5000);
  }else{
///////////////////////////////////////////IF CONNECTED TO WIFI
    if (!MDNS.begin(host)) {
      showIcons(F("MDNS error"), 19);
      
      while (1) {
        delay(500);
      }
    }
    
    setGMT();
  }
///////////////////////////////////////////UPDATE ROUTINE
  server.on("/", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex);
  });
  /*handling uploading firmware file */
  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      showIcons(F("Updating..."), 225);
      
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      /* flashing firmware to ESP*/
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) {
        showIcons(F("Update Success!"), 225);
      } else {
        Update.printError(Serial);
      }
    }
  });



///////////////////////////////////////////GET NOTIFICATION
  server.on("/sendNofication", HTTP_POST, []() {
    server.sendHeader("Connection", "close");

    String postClear = server.arg("limparTexto");
    String postColor = server.arg("corTexto");
    String postText = server.arg("texto");
    String postSize = server.arg("tamanho");
    String postX = server.arg("x");
    String postY = server.arg("y");

    printText(postClear, postSize.toInt(), postX.toInt(), postY.toInt(), postColor, postText);
    
    server.send(200, "text/plain", "notification ok");

  }, []() {
    HTTPUpload& upload = server.upload();
  });

  server.begin();
  delay(100);
  display.clearDisplay();
  display.display();
  
}


void firstMenu(int numero){
  readLevel[level] = numero;
  
      if(reading==0){
        showIcons(F("Sleep"), 31);
      } else if(numero==1){
        showIcons(F("Time"), 28);
        showMenuDots();
      } else if(numero==2){
        showIcons(F("Weather"), 15);
        showMenuDots();
      } else if(numero==3){
        showIcons(F("Networks"), 240);
        showMenuDots();
      } else if(numero==4){
        const char* p1name = configObj["p1"]["name"];
        int p1icon = configObj["p1"]["icon"];
        showIcons(p1name, p1icon);
        showMenuDots();
      } else if(numero==5){
        const char* p2name = configObj["p2"]["name"];
        int p2icon = configObj["p2"]["icon"];
        showIcons(p2name, p2icon);
        showMenuDots();
      } else if(numero==6){
        const char* p3name = configObj["p3"]["name"];
        int p3icon = configObj["p3"]["icon"];
        showIcons(p3name, p3icon);
        showMenuDots();
      } else if(numero==7){
        showIcons(F("Update"), 225);
        showMenuDots();
      } else if(numero==8){
        showIcons(F("About"), 2);
        showMenuDots();
      } else {
        showIcons(F(""), 6);
      }
}

void firstMenuSelect(int numero){
  readLevel[level] = numero;

  if(numero==0){
      printText("true", 1, 0, 16, "true", "");
      esp_deep_sleep_start();
    }else if(numero==1){
      if(configObj["face"]["analog"]){
        printLocalTime();
      }else{
        showWatchFace(F("face"));
      }
    } else if(numero==2){
      getWeather();
    } else if(numero==3){
      printNetwork();
    } else if(numero==4){
        String texto = typeText();
        showIcons(texto, 6);
        if(waitATime(4000)){
          reading=8;
        }
    } else if(numero==5){
      showWatchFace(F("p2"));
    } else if(numero==6){
      showWatchFace(F("p3"));
    } else if(numero==7){
      getUpdateGit();
    } else if(numero==8){
      showIcons("Puntly Chronus v."+ String(getLocalVersion), 225);
    } else {
      printText("true", 1, 0, 16, "true", "");
    }
}

void loop(void) {
  server.handleClient();
  delay(1);

  // Read elapsed time
  currentTime = millis();
  // Check if it's time to read
  if(currentTime >= (lastTime + 5)) {
    // read the two pins
    encA = digitalRead(pinA);
    encB = digitalRead(pinB);

    // check if A has gone from high to low
    if ((!encA) && (lastA)) {
      // check if B is high
      if (encB) {
        // clockwise
        if (reading + changeamnt <= highest) {
          reading = reading + changeamnt; 
        } else {
          reading = lowest;
        }
      }
      else
      {
        // anti-clockwise
        if (reading - changeamnt >= lowest) {
          reading = reading - changeamnt; 
        } else {
          reading = highest;
        }
      }
      switch(level){
        case 0:
          firstMenu(reading);
          break;
        case 1:
          firstMenu(reading);
          break;
        default:
          break;
      }
      
      
      lastTime2 = currentTime;
    }
    // store reading of A and millis for next loop
    lastA = encA;
    lastTime = currentTime;

  }
  if(currentTime >= (lastTime2 + 2000)) {
    
      switch(level){
        case 0:
          firstMenuSelect(reading);
          break;
        default:
          break;
      }
    
    lastTime2 = currentTime;
  }
}
