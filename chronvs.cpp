// imports da tela e botoes
#include <TFT_eSPI.h>
#include <SPI.h>
#include <TJpg_Decoder.h>
#include "SPIFFS.h"
#include <Wire.h>
#include "Button2.h"
#include "roboto_font.h"

// import bateria
#include <Pangodream_18650_CL.h>

#include <NoDelay.h>

// imports do BLE
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

// data hora
#include <time.h>
#include <sys/time.h>

// preferences
#include <Preferences.h>
Preferences preferences;

// js interpreter
#include "elk.h"

#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "driver/adc.h"
#include <esp_bt.h>


char buf[800];  // Runtime JS memory
struct js *js;  // JS instance

// identificacao dos servicos e características
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID_NOTIFICATION_TITLE "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHARACTERISTIC_UUID_NOTIFICATION_TEXT "45f6326c-e812-11ec-8fea-0242ac120002"
#define CHARACTERISTIC_UUID_BATTERY "69ae2bb2-e810-11ec-8fea-0242ac120002"
#define CHARACTERISTIC_UUID_DARKMODE "436a0054-f0b5-11ec-8ea0-0242ac120002"
#define CHARACTERISTIC_UUID_DATAHORA "b285fc10-f315-11ec-b939-0242ac120002"

// sleep
#define uS_TO_S_FACTOR 1000000 /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 5        /* Time ESP32 will go to sleep (in seconds) */
#define Threshold 40           /* Greater the value, more the sensitivity */

// botoes
#define BUTTON_1 35       // button 1
#define BUTTON_2 0        // button 2
#define TOUCH_BUTTON_1 T2 // touch button
#define TOUCH_BUTTON_2 T3 // touch button
#define TOUCH_BUTTON_3 T4 // touch button
#define TOUCH_BUTTON_4 T5 // touch button

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

#define GREEN_COLOR tft.color565(21, 143, 139)
#define WHITE_COLOR tft.color565(255, 255, 220)
#define BLACK_COLOR tft.color565(5, 5, 0)

#define CENTER_HORIZONTAL tft.width() / 2
#define CENTER_VERTICAL tft.height() / 2

// tela
#define TFT_BL 4                   // display backlight
TFT_eSPI tft = TFT_eSPI(135, 240); // Invoke custom library
TFT_eSprite spriteTft = TFT_eSprite(&tft);

noDelay delay5s(5000, false);
noDelay delay10s(10000);

touch_pad_t touchPin;

// inicia bateria
Pangodream_18650_CL BL;

// servidor do BLE
BLEServer *pServer = NULL;

// inicialização dos servicos e características
BLECharacteristic *pCharacteristicNotificationTitle = NULL;
BLECharacteristic *pCharacteristicNotificationText = NULL;
BLECharacteristic *pCharacteristicBattery = NULL;
BLECharacteristic *pCharacteristicScreenLightDarkMode = NULL;
BLECharacteristic *pCharacteristicDataHora = NULL;

// captura se o device esta conectado
bool deviceConnected = false;
bool oldDeviceConnected = false;

// rtc variaveis
RTC_DATA_ATTR bool bootCount = true;
bool darkMode = false;
bool lastDarkMode = false;

// data hora
struct tm data; // Cria a estrutura que contem as informacoes da data.
timeval tv;     // Cria a estrutura temporaria para funcao abaixo.
time_t tt;      // Obtem o tempo atual em segundos. Utilize isso sempre que precisar obter o tempo atual

uint16_t corFundoLightDark;
uint16_t corTextoLightDark;

Button2 btn1(BUTTON_1);
Button2 btn2(BUTTON_2);

int configFormato = -1;

int tempoTela = 5;

int menuAtual = -1;

boolean bleOn = true;

String batteryPercentage = ("0");
String valorTempBateria;

String valorTemp;
String valorTitulo = "Notifications";
String valorCodigoHeader = "";

String valorTemp2;
String valorNotificacao = "No new notifications";
String valorCodigoMain = "";
String valorCodigoBtn1 = "";
String valorCodigoBtn2 = "";

String valorTempDataHora;
String valorDataHora = "";

String comAcento[51] = {"ç", "Ç", "á", "é", "í", "ó", "ú", "ý", "Á", "É", "Í", "Ó", "Ú", "Ý", "à", "è", "ì", "ò", "ù", "À", "È", "Ì", "Ò", "Ù", "ã", "õ", "ñ", "ä", "ë", "ï", "ö", "ü", "ÿ", "Ä", "Ë", "Ï", "Ö", "Ü", "Ã", "Õ", "Ñ", "â", "ê", "î", "ô", "û", "Â", "Ê", "Î", "Ô", "Û"};
String semAcento[51] = {"c", "C", "a", "e", "i", "o", "u", "y", "A", "E", "I", "O", "U", "Y", "a", "e", "i", "o", "u", "A", "E", "I", "O", "U", "a", "o", "n", "a", "e", "i", "o", "u", "y", "A", "E", "I", "O", "U", "A", "O", "N", "a", "e", "i", "o", "u", "A", "E", "I", "O", "U"};


String limpaString(String valor)
{
  for (int i = 0; i < 51; i++)
  {
    valor.replace(comAcento[i], semAcento[i]);
  }
  return valor;
}


void iniciarTela()
{
  delay5s.stop();                    // pausa timer
  digitalWrite(TFT_BL, HIGH);        // ligar a tela
  tft.fillScreen(corFundoLightDark); // limpa tela
}

void sideKeysIcons(uint16_t topoHold, uint16_t topoClick, uint16_t baseHold, uint16_t baseClick)
{
  tft.setRotation(1);
  tft.setFreeFont(&desenhos);

  if (topoHold > 0)
    tft.drawChar(tft.width() - 28, 0, topoHold, GREEN_COLOR, corFundoLightDark, 1);
  if (topoClick > 0)
    tft.drawChar(tft.width() - 14, 0, topoClick, GREEN_COLOR, corFundoLightDark, 1);
  if (baseHold > 0)
    tft.drawChar(tft.width() - 28, tft.height() - 14, baseHold, GREEN_COLOR, corFundoLightDark, 1);
  if (baseClick > 0)
    tft.drawChar(tft.width() - 14, tft.height() - 14, baseClick, GREEN_COLOR, corFundoLightDark, 1);
}

void drawImage(int x, int y, String filePath)
{
  TJpgDec.drawFsJpg(x, y, filePath);
  delay5s.stop();
  delay5s.start();
}

void drawImageAutoDarkLight(int x, int y, String nome)
{
  const String dark = darkMode ? ("Dark") : ("Light");
  const String filePath = "/" + nome + dark + (".jpg");

  drawImage(x, y, filePath);
}

void tituloTela(String icone = "sett", String titulo = "")
{
  iniciarTela();

  tft.drawLine(5, 20, tft.width() - 5, 20, GREEN_COLOR);

  drawImageAutoDarkLight(0, 0, icone);

  tft.setTextDatum(TL_DATUM);
  tft.setFreeFont(&Roboto_Mono_Light_18);
  tft.setTextColor(GREEN_COLOR, corFundoLightDark); // cor do texto
  tft.drawString(titulo, 22, 1);
}

void switchButton(bool onOff = true)
{
  drawImageAutoDarkLight(CENTER_HORIZONTAL - 50, ((CENTER_VERTICAL)-15), onOff ? ("on") : ("off"));

  tft.setTextDatum(ML_DATUM);
  tft.setFreeFont(&Roboto_Mono_Light_18);
  tft.setTextColor(corTextoLightDark, corFundoLightDark); // cor do texto

  tft.drawString(onOff ? ("ON") : ("OFF"), CENTER_HORIZONTAL, CENTER_VERTICAL);

  sideKeysIcons(32, 36, 33, 37);
}

void setCor()
{
  corFundoLightDark = darkMode ? BLACK_COLOR : WHITE_COLOR;
  corTextoLightDark = darkMode ? WHITE_COLOR : BLACK_COLOR;
}

void mudarDarkMode()
{
  preferences.putBool("darkMode", darkMode);

  setCor();
}

// telas
// darkmode
void telaDarkLightMode(Button2 btn, boolean shortClick)
{
  if(!shortClick)
  {
    darkMode = btn.getID() > 0;
    mudarDarkMode();
  }
  tituloTela("sett", "Dark mode");

  switchButton(darkMode);

  delay5s.start();
}

// bateria
void telaBateria(Button2 &btn, boolean shortClick)
{
  if(!shortClick) {
    tempoTela += 5*btn.getID() ;

    if (tempoTela < 5)
    {
      tempoTela = 5;
    }

    preferences.putInt("tempoTela", tempoTela);

    delay5s.setdelay(tempoTela * 1000);
  }
  tituloTela("sett", "Battery");

  drawImageAutoDarkLight(CENTER_HORIZONTAL - 50, ((CENTER_VERTICAL)-15), "battery");

  tft.setTextDatum(ML_DATUM);
  tft.setFreeFont(&Roboto_Mono_Light_18);
  tft.setTextColor(corTextoLightDark, corFundoLightDark); // cor do texto

  tft.drawString(String(BL.getBatteryChargeLevel()) + "%", CENTER_HORIZONTAL, CENTER_VERTICAL);

  tft.setTextDatum(MC_DATUM);
  tft.drawString("Screen time " + String(tempoTela) + " seg", CENTER_HORIZONTAL, CENTER_VERTICAL + 26);

  sideKeysIcons(32, 39, 33, 38);

  delay5s.start();
}

void telaTextoTituloIcone(String titulo = "", String texto = "", String icone = ("notification"))
{
  tituloTela(icone, titulo.substring(0, 17));

  tft.setTextDatum(TL_DATUM);
  tft.setCursor(1, 34);

  tft.setFreeFont(&Roboto_Mono_Light_14);
  tft.setTextColor(corTextoLightDark, corFundoLightDark); // cor do texto
  tft.println(texto.substring(0, 80));
  delay5s.start();
}

int letraLinhaSelecionada = 4;
int letraColunaSelecionada = 0;
bool modoEdicao = false;
String textoEdicao = "";
String letras[5][10] = {
    {" ", " ", " ", " "},
    {"Z", "X", "C", "V", "B", "N", "M", ",", ".", ":"},
    {"A", "S", "D", "F", "G", "H", "J", "K", "L", "?"},
    {"Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P"},
    {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"},
};

void telaTeclado(Button2 &btn, boolean shortClick, const char* chaveTexto, String titulo)
{
  if (modoEdicao && shortClick)
  {
    if (btn.getID() == -1)
      letraLinhaSelecionada -= 1;
    if (btn.getID() == 1)
      letraColunaSelecionada += 1;
    if (letraLinhaSelecionada < 0)
      letraLinhaSelecionada = 4;
    if (letraColunaSelecionada > 9)
      letraColunaSelecionada = 0;
    if (letraLinhaSelecionada == 0 && letraColunaSelecionada > 3)
      letraColunaSelecionada = 0;
  }

  if(!shortClick){
    if (btn.getID() == 1)
      modoEdicao = !modoEdicao;
    if (btn.getID() == -1 && modoEdicao)
    {
      if (letraLinhaSelecionada == 0 && letraColunaSelecionada == 0)
      {
        textoEdicao = textoEdicao + " ";
      }
      else if (letraLinhaSelecionada == 0 && letraColunaSelecionada == 1)
      {
        const int apagar = textoEdicao.substring(textoEdicao.length(), textoEdicao.length() - 1 ) == "\n" ? 2 : 1 ;
        textoEdicao = textoEdicao.substring(0, textoEdicao.length() - apagar );
      }
      else if (letraLinhaSelecionada == 0 && letraColunaSelecionada == 2)
      {
        textoEdicao = textoEdicao + "\n";
      }
      else if (letraLinhaSelecionada == 0 && letraColunaSelecionada == 3)
      {
        textoEdicao = "";
      }
      else
      {
        textoEdicao = textoEdicao + letras[letraLinhaSelecionada][letraColunaSelecionada];
      }
    }
    if (btn.getID() == -1 && !modoEdicao)
      modoEdicao = !modoEdicao;
    if (modoEdicao)
    {
      delay5s.setdelay(120000);
    }
    if (!modoEdicao)
    {
      letraLinhaSelecionada = 4;
      letraColunaSelecionada = 0;
      preferences.putString(chaveTexto, textoEdicao);
      delay5s.setdelay(tempoTela * 1000);
    }
  }

  iniciarTela();

  if (!modoEdicao)
  {
    tft.setRotation(1);
    spriteTft.deleteSprite();
    textoEdicao = preferences.getString(chaveTexto, "");
    telaTextoTituloIcone(titulo, textoEdicao);
    sideKeysIcons(32, 40, 33, 40);
    return;
  }

  sideKeysIcons(32, 37, 34, 36);

  tft.setRotation(0);
  spriteTft.createSprite(135, 215);

  preferences.putString(chaveTexto, textoEdicao);
  spriteTft.fillScreen(corFundoLightDark); // limpa tela
  spriteTft.setTextColor(corTextoLightDark, corFundoLightDark); // cor do texto
  spriteTft.setTextDatum(TL_DATUM);

  spriteTft.setFreeFont(&Roboto_Mono_Light_11);
  spriteTft.setCursor(0, 15);
  spriteTft.print(textoEdicao);

#define alturaLinha 12
#define larguraLinha 12

  spriteTft.drawLine(5, spriteTft.height() - 66, spriteTft.width() - 5, spriteTft.height() - 66, GREEN_COLOR);
  spriteTft.setTextDatum(MC_DATUM);
  for (int i = 0; i < 5; i++)
  {
    for (int j = 0; j < 10; j++)
    {
      const int x = 10 + (i == 0 ? (j)*larguraLinha*1.5 : 5 + (j)*larguraLinha);
      const int y = (i == 0 ? spriteTft.height() - (alturaLinha * (i + 1))*1.5 : spriteTft.height() - alturaLinha * (i + 1));
      uint16_t cor = corTextoLightDark;
      if (letraLinhaSelecionada == i && letraColunaSelecionada == j)
      {
        cor = GREEN_COLOR;
        spriteTft.setFreeFont(&Roboto_Mono_Light_18);
      }
      else
      {
        cor = corTextoLightDark;
        spriteTft.setFreeFont(&Roboto_Mono_Light_11);
      }

      spriteTft.setTextColor(cor);

      if (i == 0 && j < 4)
      {
        const int specialKeys[] = {42, 41, 44, 37};
        spriteTft.setFreeFont(&desenhos);
        spriteTft.drawChar(x, y, specialKeys[j], cor, corFundoLightDark, 1);
      }
      else
      {
        spriteTft.drawString(letras[i][j], x, y);
      }
    }
  }
  spriteTft.pushSprite(0, 0);

  delay5s.start();
}

// exibir texto
void texto(String texto, bool clearScreen = false, const GFXfont *fontSize = &Roboto_Mono_Light_11, int align = TL_DATUM, int x = 0, int y = 0, uint16_t corFundo = corFundoLightDark, uint16_t corTexto = corTextoLightDark, uint16_t corFundoTexto = corFundoLightDark)
{
  delay5s.stop();
  digitalWrite(TFT_BL, HIGH); // ligar a tela

  if (clearScreen)
  {
    tft.fillScreen(corFundo); // cor de fundo
  }
  tft.setTextColor(corTexto, corFundoTexto); // cor do texto

  tft.setFreeFont(fontSize); // seleciona o tamanho da font

  tft.setTextDatum(align); // ancora para alinhamento do texto

  if (x != 0 || y != 0)
  {
    tft.setCursor(x, y);
  }
  if (align == TL_DATUM)
  {
    tft.println(texto);
  }
  if (align != TL_DATUM)
  {
    tft.drawString(texto, x, y);
  }
  delay5s.start(); // apagar a tela após 5s
}

void configTime(Button2 &btn)
{
  const int resposta = btn.getID() > 0 ? 1 : -1;
  if (configFormato == 0)
  {
    data.tm_min = data.tm_min + resposta;
  }
  if (configFormato == 1)
  {
    data.tm_hour = data.tm_hour + resposta;
  }
  if (configFormato == 2)
  {
    data.tm_mday = data.tm_mday + resposta;
  }
  if (configFormato == 3)
  {
    data.tm_mon = data.tm_mon + resposta;
  }
  if (configFormato == 4)
  {
    data.tm_year = data.tm_year + resposta;
  }

  tt = mktime(&data);
  tv.tv_sec = tt;
  settimeofday(&tv, NULL); // Configura o RTC para manter a data atribuida atualizada.
}

void telaRelogio(Button2 btn, boolean shortClick)
{
  if ( !shortClick && btn.wasPressedFor() < 1200 ) {
    if (btn.getID() != 0)
    {
      configFormato += btn.getID();
      if (configFormato == 0)
      {
        tt = time(NULL);        // Obtem o tempo atual em segundos. Utilize isso sempre que precisar obter o tempo atual
        data = *localtime(&tt); // Converte o tempo atual e atribui na estrutura
      }
      if (configFormato > 4)
      {
        configFormato = -1;
      }
      if (configFormato < -1)
      {
        configFormato = 4;
      }
    }
  }
  
  if (shortClick && configFormato > -1 || btn.wasPressedFor() > 2000)
  {
    configTime(btn);
  }
  spriteTft.createSprite(240, 135);
  tft.fillScreen(BLACK_COLOR);
  digitalWrite(TFT_BL, HIGH);    // ligar a tela
  tft.setTextColor(GREEN_COLOR); // cor do texto
  tt = time(NULL);               // Obtem o tempo atual em segundos. Utilize isso sempre que precisar obter o tempo atual
  data = *localtime(&tt);        // Converte o tempo atual e atribui na estrutura

  char data_formatada[64]; // formatar data hora

  strftime(data_formatada, 64, "%H%M", &data);

  // if (configFormato == 0 && data.tm_sec % 2 != 0)
  // {
  //   strftime(data_formatada, 64, "%H:..", &data);
  // }

  // if (configFormato == 1 && data.tm_sec % 2 != 0)
  // {
  //   strftime(data_formatada, 64, "..:%M", &data);
  // }

  int x = 0, y = 27;
  int totalW = 0;
  for (int i = 0; i < 5; i++)
  {
    uint16_t w = 0, h = 0;
    if (String(data_formatada[i]).length() > 0)
    {
      TJpgDec.getFsJpgSize(&w, &h, "/" + String(data_formatada[i]) + ".jpg");
      totalW = totalW + w;
    }
  }
  int margin = 240 - totalW;
  if (margin > 0)
  {
    margin = margin / 2;
    x = margin;
  }
  for (int i = 0; i < 5; i++)
  {
    uint16_t w = 0, h = 0;
    if (String(data_formatada[i]).length() > 0)
    {
      TJpgDec.drawFsJpg(x, y, "/" + String(data_formatada[i]) + ".jpg");
      TJpgDec.getFsJpgSize(&w, &h, "/" + String(data_formatada[i]) + ".jpg");
      x = x + w;
    }
  }

  tft.setTextDatum(TL_DATUM);
  tft.setFreeFont(&Roboto_Mono_Light_11);
  strftime(data_formatada, 64, "%d %b %y", &data);
  tft.drawString(String(BL.getBatteryChargeLevel()) + "%  " + data_formatada, 2, 2);

  if (!deviceConnected && configFormato == -1)
  {
    sideKeysIcons(0,43,0,0);
  }

  if(configFormato > -1) {
    const String config[] = {"minute","hour","day","month","year"};
    tft.setTextDatum(TC_DATUM);
    tft.setFreeFont(&Roboto_Mono_Light_11);
    tft.drawString("Set " + config[configFormato], CENTER_HORIZONTAL, 115);
    sideKeysIcons(32,39,33,38);
  }

  // tft.setTextDatum(MC_DATUM);
  // tft.setFreeFont(&Roboto_Mono_Light_50);

  // tft.setTextColor(GREEN_COLOR); // cor do texto

  // tft.drawString(data_formatada, CENTER_HORIZONTAL, CENTER_VERTICAL);

  // if (configFormato == 2 && data.tm_sec % 2 != 0)
  // {
  //   strftime(data_formatada, 64, "-- %b", &data);
  // }

  // if (configFormato == 3 && data.tm_sec % 2 != 0)
  // {
  //   strftime(data_formatada, 64, "%d --", &data);
  // }

  // if (configFormato == 4)
  // {
  //   if (data.tm_sec % 2 != 0)
  //   {
  //     strftime(data_formatada, 64, "%d %b --", &data);
  //   }
  //   else
  //   {
  //     strftime(data_formatada, 64, "%d %b %Y", &data);
  //   }
  // }

  delay5s.start(); // apagar a tela após 5s
}

void button_loop()
{
  btn1.loop();
  btn2.loop();
}

void getCodeBle(Button2 &btn, boolean shortClick, boolean edicao) {
    valorCodigoHeader = preferences.getString("CodigoHeader", "");
    valorCodigoMain = preferences.getString("CodigoMain", "");
    valorCodigoBtn1 = preferences.getString("CodigoBtn1", "");
    valorCodigoBtn2 = preferences.getString("CodigoBtn2", "");

    if( btn.getID() == 1 && !edicao )
    {
      if ( shortClick ) {
      } else {
        js_eval(js,
          valorCodigoBtn1.c_str()
          ,~0U);
      }
    } else 
    if( btn.getID() == -1 && !edicao )
    {
      if ( shortClick ) {
      } else {
        js_eval(js,
          valorCodigoBtn2.c_str()
          ,~0U);
      }
    } 
    // if(edicao){
    //   telaTeclado(btn, shortClick, "textLembrete", valorCodigoHeader);
    // } else {
      js_eval(js,
          valorCodigoMain.c_str()
          ,~0U);
    // }
    // telaTextoTituloIcone(valorCodigoHeader, valorCodigoMain);
}

// callback pra quando conecta ou desconecta
class ConnectionCallback : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    deviceConnected = true;
  };

  void onDisconnect(BLEServer *pServer)
  {
    deviceConnected = false;
  }
};

void limpaNotificacao()
{
  if(
          valorTitulo.indexOf("charging") > -1 || 
          valorNotificacao.indexOf("charging") > -1 || 
          valorTitulo.indexOf("Fast charging") > -1 ||
          valorNotificacao.indexOf("Fast charging") > -1 ||
          valorTitulo.indexOf("until full") > -1 ||
          valorNotificacao.indexOf("until full") > -1 ||
          valorTitulo.indexOf("lighting") > -1 ||
          valorNotificacao.indexOf("lighting") > -1
        ) {
          valorTitulo = "";
          valorNotificacao = "";
        }

        if ((valorTitulo.length() < 1 && valorNotificacao.length() < 1))
        {
          return;
        }
}

// callback para quando chega o título da notificacao
bool isLembrete = false;
class GetValueFromBle : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    std::string value = pCharacteristic->getValue();

    if (value.length() > 0)
    {
      for (int i = 0; i < value.length(); i++)
      {
        valorTemp = valorTemp + value[i]; // captura todos os dados enviados
      }
     
      #define NOTIFICACAO_TITULO "@@nt@@"
      #define NOTIFICACAO_TITULO_FIM "$$nt$$"
      if (valorTemp.indexOf(NOTIFICACAO_TITULO) > -1 && valorTemp.indexOf(NOTIFICACAO_TITULO_FIM) > -1) {
        valorTitulo = valorTemp.substring(valorTemp.indexOf(NOTIFICACAO_TITULO)+6,valorTemp.indexOf(NOTIFICACAO_TITULO_FIM));
        valorTemp = "";
      }

      #define NOTIFICACAO_VALOR "@@nv@@"
      #define NOTIFICACAO_VALOR_FIM "$$nv$$"
      if (valorTemp.indexOf(NOTIFICACAO_VALOR) > -1 && valorTemp.indexOf(NOTIFICACAO_VALOR_FIM) > -1) {
        valorNotificacao = valorTemp.substring(valorTemp.indexOf(NOTIFICACAO_VALOR)+6,valorTemp.indexOf(NOTIFICACAO_VALOR_FIM));
        valorTemp = "";
        limpaNotificacao();
        if ((valorTitulo.length() > 1 || valorNotificacao.length() > 1))
        {
          telaTextoTituloIcone(valorTitulo, valorNotificacao);
        }
      }
    
      #define CODIGO_HEADER "@@ph@@"
      #define CODIGO_HEADER_FIM "$$ph$$"
      if (valorTemp.indexOf(CODIGO_HEADER) > -1 && valorTemp.indexOf(CODIGO_HEADER_FIM) > -1) {
        valorCodigoHeader = valorTemp.substring(valorTemp.indexOf(CODIGO_HEADER)+6,valorTemp.indexOf(CODIGO_HEADER_FIM));
        preferences.putString("CodigoHeader", valorCodigoHeader);
        valorTemp = "";
      }
    
      #define CODIGO_MAIN "@@pc@@"
      #define CODIGO_MAIN_FIM "$$pc$$"
      if (valorTemp.indexOf(CODIGO_MAIN) > -1 && valorTemp.indexOf(CODIGO_MAIN_FIM) > -1) {
        valorCodigoMain = valorTemp.substring(valorTemp.indexOf(CODIGO_MAIN)+6,valorTemp.indexOf(CODIGO_MAIN_FIM));
        preferences.putString("CodigoMain", valorCodigoMain);
        valorTemp = "";
      }
    
      #define CODIGO_BTN1 "@@pa@@"
      #define CODIGO_BTN1_FIM "$$pa$$"
      if (valorTemp.indexOf(CODIGO_BTN1) > -1 && valorTemp.indexOf(CODIGO_BTN1_FIM) > -1) {
        valorCodigoBtn1 = valorTemp.substring(valorTemp.indexOf(CODIGO_BTN1)+6,valorTemp.indexOf(CODIGO_BTN1_FIM));
        preferences.putString("CodigoBtn1", valorCodigoBtn1);
        valorTemp = "";
      }
    
      #define CODIGO_BTN2 "@@pb@@"
      #define CODIGO_BTN2_FIM "$$pb$$"
      if (valorTemp.indexOf(CODIGO_BTN2) > -1 && valorTemp.indexOf(CODIGO_BTN2_FIM) > -1) {
        valorCodigoBtn2 = valorTemp.substring(valorTemp.indexOf(CODIGO_BTN2)+6,valorTemp.indexOf(CODIGO_BTN2_FIM));
        preferences.putString("CodigoBtn2", valorCodigoBtn2);
        valorTemp = "";
        telaTextoTituloIcone("Code uploaded!", "Please, restart the device to apply.", "sett");
      }
    }
  }
};

// callback para quando chega o texto da notificacao
class GetNotificationsText : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    std::string value = pCharacteristic->getValue();

    if (value.length() > 0)
    {
      valorTemp2 = "";
      for (int i = 0; i < value.length(); i++)
      {
        valorTemp2 = valorTemp2 + value[i]; // captura todos os dados enviados
      }
    }
  }
};

// valida bateria se solicitado
class returnBattery : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    std::string value = pCharacteristic->getValue();

    if (value.length() > 0)
    {
      valorTempBateria = "";
      for (int i = 0; i < value.length(); i++)
      {
        valorTempBateria = valorTempBateria + value[i];
      }

      batteryPercentage = String(BL.getBatteryChargeLevel());
      if (valorTempBateria == ("getBattery"))
      {
        pCharacteristic->setValue(batteryPercentage.c_str());
      }
    }
  }
};

// altera entre light e dark mode
String valorDarkMode = "";
class returnDarkMode : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    std::string value = pCharacteristic->getValue();

    if (value.length() > 0)
    {
      valorDarkMode = "";
      for (int i = 0; i < value.length(); i++)
      {
        valorDarkMode = valorDarkMode + value[i];
      }

      darkMode = (valorDarkMode == "1" ? true : false);

      mudarDarkMode();

      telaDarkLightMode(NULL, true);
    }
  }
};

// atualiza data hora
class returnDataHora : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    std::string value = pCharacteristic->getValue();

    if (value.length() > 0)
    {
      valorTempDataHora = "";
      for (int i = 0; i < value.length(); i++)
      {
        valorTempDataHora = valorTempDataHora + value[i];
      }

      tv.tv_sec = (valorTempDataHora.substring(0, 10)).toInt(); // Atribui minha data atual. Voce pode usar o NTP para isso ou o site citado no artigo!

      settimeofday(&tv, NULL); // Configura o RTC para manter a data atribuida atualizada.

      tt = time(NULL);        // Obtem o tempo atual em segundos. Utilize isso sempre que precisar obter o tempo atual
      data = *localtime(&tt); // Converte o tempo atual e atribui na estrutura
      data.tm_hour = data.tm_hour - 3;

      tt = mktime(&data);
      tv.tv_sec = tt;
      settimeofday(&tv, NULL); // Configura o RTC para manter a data atribuida atualizada.

      tt = time(NULL);        // Obtem o tempo atual em segundos. Utilize isso sempre que precisar obter o tempo atual
      data = *localtime(&tt); // Converte o tempo atual e atribui na estrutura

      char data_formatada[64];
      strftime(data_formatada, 64, "%d/%m/%Y %H:%M:%S", &data);

      telaRelogio(NULL, true);
    }
  }
};

void initBLE()
{
  // btStart();
  // device name
  BLEDevice::init("Chronvs");
  // create server and call back when connected
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ConnectionCallback());

  // create srevice
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // notification TITLE characteristic
  pCharacteristicNotificationTitle = pService->createCharacteristic(
      CHARACTERISTIC_UUID_NOTIFICATION_TITLE,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE);
  pCharacteristicNotificationTitle->setCallbacks(new GetValueFromBle());

  // notification TEXT characteristic
  pCharacteristicNotificationText = pService->createCharacteristic(
      CHARACTERISTIC_UUID_NOTIFICATION_TEXT,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE);
  pCharacteristicNotificationText->setCallbacks(new GetNotificationsText());

  // battery characteristic
  pCharacteristicBattery = pService->createCharacteristic(
      CHARACTERISTIC_UUID_BATTERY,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE |
          BLECharacteristic::PROPERTY_NOTIFY |
          BLECharacteristic::PROPERTY_INDICATE);
  pCharacteristicBattery->addDescriptor(new BLE2902());
  pCharacteristicBattery->setCallbacks(new returnBattery());

  // battery characteristic
  pCharacteristicScreenLightDarkMode = pService->createCharacteristic(
      CHARACTERISTIC_UUID_DARKMODE,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE |
          BLECharacteristic::PROPERTY_NOTIFY |
          BLECharacteristic::PROPERTY_INDICATE);
  pCharacteristicScreenLightDarkMode->addDescriptor(new BLE2902());
  pCharacteristicScreenLightDarkMode->setCallbacks(new returnDarkMode());

  // notification TEXT characteristic
  pCharacteristicDataHora = pService->createCharacteristic(
      CHARACTERISTIC_UUID_DATAHORA,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE);
  pCharacteristicDataHora->setCallbacks(new returnDataHora());

  // start service
  pService->start();

  // configure on notification and start service
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0); // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
}

// tela bluetooth
void telaBleMode(Button2 btn, boolean shortClick)
{
  if(!shortClick)
  {
    bleOn = btn.getID() > 0;
    preferences.putBool("bleOn", bleOn);
    if(bleOn) initBLE();
    if(!bleOn) BLEDevice::deinit(false);
  }
  tituloTela("sett", "Bluetooth");

  switchButton(bleOn);

  delay5s.start();
}

void btnLongClick(Button2 &btn, boolean shortClick)
{
  if (shortClick && !modoEdicao && configFormato == -1){
    if (menuAtual + btn.getID() >= 0 && menuAtual + btn.getID() < 10)
    {
      menuAtual += btn.getID();
    }
  }

  if (menuAtual < 0 || menuAtual > 10)
  {
    menuAtual = 0;
  }

  if (menuAtual == 0)
  {
    telaRelogio(btn, shortClick);
  }
  else if (menuAtual == 1)
  {
    telaDarkLightMode(btn, shortClick);
  }
  else if(menuAtual == 2)
  {
    telaBleMode(btn, shortClick);
  }
  else if (menuAtual == 3)
  {
    telaBateria(btn, shortClick);
  }
  else if (menuAtual == 4)
  {
    telaTextoTituloIcone(valorTitulo, valorNotificacao);
  }
  else if (menuAtual == 5)
  {
    getCodeBle(btn, shortClick, false);
  }
  else if (menuAtual == 6)
  {
    telaTeclado(btn, shortClick, "note", "Notes");
  }
  else
  {
    telaTextoTituloIcone(String(menuAtual), "Lorem ipsum");
  }
}

void btnLongClickHandler(Button2 &btn)
{
  btnLongClick(btn, false);
}

void btnClickHandler(Button2 &btn)
{
  btnLongClick(btn, true);
}

void initButtons()
{
  pinMode(BUTTON_1, INPUT_PULLUP);
  pinMode(BUTTON_2, INPUT_PULLUP);

  btn1.setID(1);
  btn2.setID(-1);

  btn1.setLongClickDetectedRetriggerable(true);
  btn2.setLongClickDetectedRetriggerable(true);

  btn1.setLongClickTime(600);
  btn2.setLongClickTime(600);

  btn1.setLongClickDetectedHandler(btnLongClickHandler);
  btn1.setClickHandler(btnClickHandler);
  btn2.setLongClickDetectedHandler(btnLongClickHandler);
  btn2.setClickHandler(btnClickHandler);
}

void SPIFFSInit()
{
  if (!SPIFFS.begin())
  {
    while (1)
      yield(); // Stay here twiddling thumbs waiting
  }
}

void initPreferences()
{
  preferences.begin("chronvs", false);
  darkMode = preferences.getBool("darkMode", darkMode);
  bleOn = preferences.getBool("bleOn", bleOn);
  tempoTela = preferences.getInt("tempoTela", tempoTela);
}

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap)
{
  if (y >= tft.height())
    return 0;
  tft.pushImage(x, y, w, h, bitmap);
  return 1;
}

void initDisplay()
{
  tft.init();
  tft.setRotation(1);
  tft.setCursor(0, 0);
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(1);
  tft.fillScreen(BLACK_COLOR);
  tft.setFreeFont(&Roboto_Mono_Light_11);
  tft.setTextWrap(true, true);

  TJpgDec.setJpgScale(1);
  TJpgDec.setCallback(tft_output);
  tft.setSwapBytes(true);

  setCor();

  digitalWrite(TFT_BL, LOW);

  if (bootCount)
  {
    texto(("Bem vindo"), true, &Roboto_Mono_Light_18, TL_DATUM, 102, CENTER_VERTICAL - 18, corFundoLightDark, corTextoLightDark, corFundoLightDark);
    texto(("Chronvs"), false, &Roboto_Mono_Light_24, TL_DATUM, 102, CENTER_VERTICAL + 24, corFundoLightDark, GREEN_COLOR, corFundoLightDark);
    texto(("v0.5"), false, &Roboto_Mono_Light_11, BR_DATUM, tft.width(), tft.height(), corFundoLightDark, GREEN_COLOR, corFundoLightDark);

    drawImageAutoDarkLight(20, CENTER_VERTICAL - 40, ("logo"));
  } else {
    telaRelogio(NULL, true);
  }
}

void callback()
{
  
}

void initSleep() {

  esp_sleep_enable_ext0_wakeup(GPIO_NUM_35,0);
  // esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  // touchAttachInterrupt(T9, callback, Threshold);
  // esp_sleep_enable_touchpad_wakeup();
}



jsval_t jssetTextDatum(struct js *js, jsval_t *args, int nargs) {
  uint8_t datum = js_getnum(args[0]);
  tft.setTextDatum(datum);
  return js_mknum(0);
}

jsval_t jssetTextColor(struct js *js, jsval_t *args, int nargs) {
  int red = js_getnum(args[0]);
  int green = js_getnum(args[1]);
  int blue = js_getnum(args[2]);
  tft.setTextColor(tft.color565(red, green, blue));
  return js_mknum(0);
}

jsval_t jssetCursor(struct js *js, jsval_t *args, int nargs) {
  const int x = js_getnum(args[0]);
  const int y = js_getnum(args[1]);
  tft.setCursor(x,y);
  return js_mknum(0);
}

jsval_t jsprintln(struct js *js, jsval_t *args, int nargs) {
  size_t *n = 0;
  const String texto = js_getstr(js, args[0], n);
  tft.println(texto);
  return js_mknum(0);
}

jsval_t jsdrawString(struct js *js, jsval_t *args, int nargs) {
  size_t *n = 0;
  const String texto = js_getstr(js, args[0], n);
  const int x = js_getnum(args[1]);
  const int y = js_getnum(args[2]);
  tft.drawString(texto, x, y);
  return js_mknum(0);
}

jsval_t jsfillScreen(struct js *js, jsval_t *args, int nargs) {
  int red = js_getnum(args[0]);
  int green = js_getnum(args[1]);
  int blue = js_getnum(args[2]);
  tft.fillScreen(tft.color565(red, green, blue));
  return js_mknum(0);
}

jsval_t jsDelay(struct js *js, jsval_t *args, int nargs) {
  long ms = js_getnum(args[0]);
  delay(ms);
  return js_mknum(0);
}
jsval_t jsWrite(struct js *js, jsval_t *args, int nargs) {
  int pin = js_getnum(args[0]);
  int val = js_getnum(args[1]);
  digitalWrite(pin, val);
  return js_mknum(0);
}
jsval_t jsMode(struct js *js, jsval_t *args, int nargs) {
  int pin = js_getnum(args[0]);
  int mode = js_getnum(args[1]);
  pinMode(pin, mode);
  return js_mknum(0);
}

void teste() {
  js = js_create(buf, sizeof(buf));
  jsval_t global = js_glob(js), gpio = js_mkobj(js);  // Equivalent to:
  js_set(js, global, "gpio", gpio);                   // let gpio = {};
  js_set(js, global, "delay", js_mkfun(jsDelay));     // import delay()
  js_set(js, gpio, "mode", js_mkfun(jsMode));         // import gpio.mode()
  js_set(js, gpio, "write", js_mkfun(jsWrite));       // import gpio.write()
  js_set(js, global, "setTextDatum", js_mkfun(jssetTextDatum));     // import setTextDatum()
  js_set(js, global, "setTextColor", js_mkfun(jssetTextColor));     // import setTextColor()
  js_set(js, global, "setCursor", js_mkfun(jssetCursor));     // import setCursor()
  js_set(js, global, "println", js_mkfun(jsprintln));     // import println()
  js_set(js, global, "drawString", js_mkfun(jsdrawString));     // import drawString()
  js_set(js, global, "fillScreen", js_mkfun(jsfillScreen));     // import fillScreen()
  
  js_eval(js,
          preferences.getString("CodigoHeader", "").c_str()
          ,~0U);
}

void setup()
{
  Serial.begin(115200);
  // adc_power_acquire();

  initPreferences();

  SPIFFSInit();
  initDisplay();
  teste();
  if(bleOn) initBLE();
  initButtons();

  initSleep();
}

void dispositivoConectado()
{
  // notify changed value
  if (deviceConnected)
  {
    // informa a bateria
    batteryPercentage = String(BL.getBatteryChargeLevel());
    pCharacteristicBattery->setValue(batteryPercentage.c_str()); // Envia a % bateria atualizada quando conectado
    pCharacteristicBattery->notify();

    // informa o dark ou light mode
    pCharacteristicScreenLightDarkMode->setValue(String(darkMode).c_str()); // Envia o status do darkmode
    pCharacteristicScreenLightDarkMode->notify();
    lastDarkMode = darkMode;
  }
}

void dispositivoDesconectado()
{
  // disconnecting
  if (!deviceConnected && oldDeviceConnected)
  { // espera a próxima vez que conectar
    delay(500);
    pServer->startAdvertising(); // restart advertising
    oldDeviceConnected = deviceConnected;
  }
}

void dispositivoConectando()
{
  // connecting
  if (deviceConnected && !oldDeviceConnected)
  { // conectando...
    // do stuff here on connecting
    
    oldDeviceConnected = deviceConnected;
  }
}

void delaysDesligarTela()
{
  if (delay5s.update())
  {
    tft.fillScreen(BLACK_COLOR);
    digitalWrite(TFT_BL, LOW);
    menuAtual = -1;
    configFormato = -1;

    delay5s.stop();

    bootCount = false;

    if (modoEdicao)
    {
      delay5s.setdelay(tempoTela * 1000);
      preferences.putString("note", textoEdicao);
      modoEdicao = false;
    }

    // btStop();

    // adc_power_release();
    // esp_bt_controller_disable();

    
    // esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH,   ESP_PD_OPTION_AUTO);
    // esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
    // esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
    // esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL,         ESP_PD_OPTION_OFF);

    // esp_deep_sleep_start();

    delay5s.setdelay(tempoTela * 1000);
  }

  if (delay10s.update() || darkMode != lastDarkMode)
  {
    dispositivoConectado();
    dispositivoDesconectado();
    dispositivoConectando();
  }
}

void loop()
{
  button_loop();
  delaysDesligarTela();
}
