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

WebServer server(80);
HTTPClient http;

const char* host = "Puntly";
const char* ssid = "gtchris100";
const char* password = "carsled100";

JSONVar configObj;

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
  const int pinC = 27;
  // Storing the readings
  boolean encA;
  boolean encB;
  boolean encC;
  boolean lastA = false;

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
      JSONVar myObject = JSON.parse(payloadWeather);
  
      if (JSON.typeof(myObject) == "undefined") {
        showIcons(F("Error to get weather"), 19); 
        return;
      }
      
      const char* description = myObject["weather"][0]["description"];
      double gettemp = myObject["main"]["temp"];
      int iconTemp = myObject["weather"][0]["id"];
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

  }
}

void setup() {
  Serial.begin(115200);
  pinMode(pinA, INPUT_PULLUP);
  pinMode(pinB, INPUT_PULLUP);
  pinMode(pinC, INPUT_PULLUP);
  currentTime = millis();
  lastTime = currentTime; 
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
  

///////////////////////////////////////////WIFI
  JSONVar wifiConfig = configObj["wifi"];
  bool conectado = false;
  for(int16_t i=0; i<wifiConfig.length(); i+=1) {
    ssid = configObj["wifi"][i]["ssid"];
    password = configObj["wifi"][i]["password"];
  
    WiFi.begin(ssid , password);

    int16_t tentativas = 0;

    while (WiFi.status() != WL_CONNECTED && tentativas < 10) {
      showIcons(F("Wait"), 225);
      delay(500);
      tentativas++;
    }
    if(WiFi.status() == WL_CONNECTED){conectado=true;break;}
  }

  if(!conectado){
///////////////////////////////////////////AP IF CAN'T CONNECT TO WIFI
    WiFi.softAP("Puntly","12345678",1,13);
    printText("true", 1, 0 ,0, "true", "Can't connect to WiFi");
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
    encC = digitalRead(pinC);
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
      // Output reading for debugging
      if(reading==0){
        printText("true", 1, 20, 50, "true", "");
      } else if(reading==1){
        showIcons(F("Time"), 28);
        showMenuDots();
      } else if(reading==2){
        showIcons(F("Weather"), 15);
        showMenuDots();
      } else if(reading==3){
        showIcons(F("Networks"), 240);
        showMenuDots();
      } else if(reading==4){
        const char* p1name = configObj["p1"]["name"];
        int p1icon = configObj["p1"]["icon"];
        showIcons(p1name, p1icon);
        showMenuDots();
      } else if(reading==5){
        const char* p2name = configObj["p2"]["name"];
        int p2icon = configObj["p2"]["icon"];
        showIcons(p2name, p2icon);
        showMenuDots();
      } else if(reading==6){
        const char* p3name = configObj["p3"]["name"];
        int p3icon = configObj["p3"]["icon"];
        showIcons(p3name, p3icon);
        showMenuDots();
      } else if(reading==7){
        showIcons(F("Update"), 225);
        showMenuDots();
      } else if(reading==8){
        showIcons(F("About"), 2);
        showMenuDots();
      } else {
        showIcons(F(""), 6);
      }
      
      lastTime2 = currentTime;
    }
    // store reading of A and millis for next loop
    lastA = encA;
    lastTime = currentTime;

  }
  if(currentTime >= (lastTime2 + 2000)) {
    if(reading==1){
      if(configObj["face"]["analog"]){
        printLocalTime();
      }else{
        showWatchFace(F("face"));
      }
    } else if(reading==2){
      getWeather();
    } else if(reading==3){
      printNetwork();
    } else if(reading==4){
      showWatchFace(F("p1"));
    } else if(reading==5){
      showWatchFace(F("p2"));
    } else if(reading==6){
      showWatchFace(F("p3"));
    } else if(reading==7){
      getUpdateGit();
    } else if(reading==8){
      showIcons("Puntly Chronus v."+ String(getLocalVersion), 225); 
    } else {
      printText("true", 1, 0, 16, "true", "");
    }
    lastTime2 = currentTime;
  }
}
