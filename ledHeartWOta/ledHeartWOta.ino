

#include <Adafruit_NeoPixel.h>

#define LED_PIN 12
#define LED_COUNT 10

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <WiFiClient.h>


#include <OneButton.h>

OneButton btn;

const int button1_pin = 14;
const int button2_pin = 5;
const int button3_pin = 4;
const int button4_pin = 13;

const int bzz_pin = 0;

unsigned long bzzStartTime;
unsigned long bzzWorkTime; 
bool bzz_status = 0;

int mode = 0;

bool bzzStatus = false;

const char* ssid = "Epson L3150";
const char* password = "12345678";

uint8_t globalBrightness = 80;

uint8_t globalR = 10;
uint8_t globalG = 10;
uint8_t globalB = 10;

int CurrentMotorPower;

uint32_t globalTimer;

bool redrawMode = true;

bool modeIsShowing = false;

String msg;

String telnetBuffer = "";

uint8_t ColorTransTime = 20;

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_RGB + NEO_KHZ800);

bool ledUpdate = false;

WiFiServer telnetServer(23);  //telnet server on port 23
WiFiClient telnetClient;

OneButton button1(
  button1_pin,  // Input pin for the button
  false,        // Button is active HIGH
  false         // Disable internal pull-up resistor
);

OneButton button2(
  button2_pin,  // Input pin for the button
  false,        // Button is active HIGH
  false         // Disable internal pull-up resistor
);

OneButton button3(
  button3_pin,  // Input pin for the button
  false,        // Button is active HIGH
  false         // Disable internal pull-up resistor
);

OneButton button4(
  button4_pin,  // Input pin for the button
  false,        // Button is active HIGH
  false         // Disable internal pull-up resistor
);

void setup() {


  delay(1000);

  pinMode(bzz_pin, OUTPUT);
  analogWrite(bzz_pin, 0);

  Serial.begin(115200);
  Serial.println("Booting");


  Serial.print("Configuring access point...");

  WiFi.softAP(ssid, password);

  IPAddress myIP = WiFi.softAPIP();


  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready for update");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());


  telnetServer.begin();
  telnetServer.setNoDelay(true);
  Serial.println("Telnet server started on port 23");

  strip.begin();
  safeStripShow();  // Initialize all pixels to 'off'


  button1.attachClick(Click1);
  button2.attachClick(Click2);
  button3.attachClick(Click3);
  button4.attachClick(Click4);

  button3.attachDuringLongPress(DuringLongPressB3);
  button4.attachDuringLongPress(DuringLongPressB4);
  button3.setLongPressIntervalMs(50);
  button4.setLongPressIntervalMs(50);
}

void TelnetPrint(String text) {
  Serial.println(text);  // always in ph port
  if (telnetClient && telnetClient.connected()) {
    telnetClient.println(text);  // to the air
    yield();
  }
}


void loop() {


  ArduinoOTA.handle();

  if (telnetServer.hasClient()) {
    if (!telnetClient || !telnetClient.connected()) {
      if (telnetClient) telnetClient.stop();
      telnetClient = telnetServer.available();

      Serial.println("New telnet client ");
      telnetClient.println("=== connected to telnet ===");
    } else {
      // refuse to connect with other clients
      WiFiClient extraClient = telnetServer.available();
      extraClient.stop();
    }

  }


bool commandReceived = false;
  
  // telnet -> serial
  if (telnetClient && telnetClient.connected()) {
    while (telnetClient.available()) {
      char c = telnetClient.read();
      Serial.write(c);
      if (c == '\n' || c == '\r') {
        commandReceived = true;
        break; 
      } else {
        telnetBuffer += c;
      }
      yield();
    } 

    if (commandReceived) {
      if (telnetBuffer.length() > 0) {
        telnetBuffer.trim();

        if (telnetBuffer.startsWith("bzz ")) {
          int power = 0;
          int time = 0;
          if (sscanf(telnetBuffer.c_str(), "bzz %d %d", &power, &time) == 2) {
            bzzSet(power, time);
            String msg = "running motor from telnet cmd   Power= " + String(power) + " Time= " + String(time);
            TelnetPrint(msg);
          } else {
            TelnetPrint("error! usage bzz power(<1023) time(ms) ");
          }
        }
        else if (telnetBuffer == "time") {
          TelnetPrint("Uptime: " + String(millis() / 1000) + " seconds ");
        }
        else if (telnetBuffer == "help")  { TelnetPrint("c(1,2,3,4); lp(3/4); mode ; bzz ") ;}
        else if (telnetBuffer == "c1")    { Click1(); }
        else if (telnetBuffer == "c2")    { Click2(); }
        else if (telnetBuffer == "c3")    { Click3(); }
        else if (telnetBuffer == "c4")    { Click4(); }
        else if (telnetBuffer == "mode")  { TelnetPrint("current mode: " + String(mode)); }
        else if (telnetBuffer == "lp3")   { DuringLongPressB3(); }
        else if (telnetBuffer == "lp4")   { DuringLongPressB4(); }

        telnetBuffer = ""; 
      }
    }
  } 

  // serial -> telnet
  while (Serial.available()) {
    char c = Serial.read();
    if (telnetClient && telnetClient.connected()) {
      telnetClient.write(c);
      yield();
    }
  }

  if (modeIsShowing == 1) {
    if(redrawMode){
      strip.clear();
      strip.setPixelColor(mode, strip.Color(100, 100, 0));
      safeStripShow();
      redrawMode = 0;
    }
    if (millis() - globalTimer > 500) {
      modeIsShowing = 0;
      ledUpdate = true;
    }
  }

  if (ledUpdate == true && modeIsShowing == 0) {
    switch (mode) {
      case 0:
        strip.fill(strip.Color(globalR, globalG, globalB));
        break;

      case 1:
        strip.fill(strip.Color(globalR, globalG, globalB));
        break;

      case 2:
        strip.fill(strip.Color(globalR, globalG, globalB));
        break;
    }

    safeStripShow();
    ledUpdate = false;
  }

  if (mode == 3) {
    RandomColorOne();
  }

  else if (mode == 4) {
    ColorTransfusion();
  }

  else if (mode == 5) {
    comet();
  }

  else if (mode == 6) {
    liquidPlasma();
  }
  
  else if (mode == 7) {
    fullRainbow();
  }



  button1.tick();
  button2.tick();
  button3.tick();
  button4.tick();

  bzzCheck();

  delay(1);
  
  }






void RandomColorOne() {

  if (modeIsShowing == 1) {
    return;
  }

  static uint32_t lastTime = millis();
  static int i;
  if (millis() - lastTime > ColorTransTime) {
    for (i = 0; i < LED_COUNT; i++) {

      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV((uint16_t)ESP.random(), 255, globalBrightness)));  //random full bright corrected color for i led
    }
    safeStripShow();
    lastTime = millis();
  }
}

void ColorTransfusion() {

  if (modeIsShowing == 1) {
    return;
  }

  static uint32_t lastTime = millis();
  static uint16_t baseColor = 0;

  if (millis() - lastTime > (ColorTransTime % 20)) {

    for (int i = 0; i < LED_COUNT; i++) {
      uint16_t hue = baseColor + (i * 65535L / LED_COUNT);
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(hue, 255, globalBrightness)));
    }
    safeStripShow();
    baseColor += 100;
    lastTime = millis();
  }
}


void pinkBlinking() {

  if (modeIsShowing == 1) {
    return;
  }
  static uint32_t lastTime = millis();
  static bool evenOrOdd = 1;
  if (millis() - lastTime > ColorTransTime) {
    for (int i = 0; i < LED_COUNT; i++) {

      if (i % 2 == evenOrOdd) {
        strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(62622, 242, globalBrightness)));  //pink
      } else {
        strip.setPixelColor(i, 0);  //dark
      }
    }
    safeStripShow();
    evenOrOdd = !evenOrOdd;
    lastTime = millis();
  }
}

void randomBlinking() {

  if (modeIsShowing == 1) {
    return;
  }
  static uint32_t lastTime = millis();
  if (millis() - lastTime > ColorTransTime) {
    strip.fill(strip.Color((uint8_t)ESP.random(), (uint8_t)ESP.random(), (uint8_t)ESP.random()));
    lastTime = millis();
    safeStripShow();
  }
}

void grainOfRice() {
  if (modeIsShowing == 1) {
    return;
  }

  static uint32_t lastTime = millis();
  static int lastLed = 0;
  if (millis() - lastTime > ColorTransTime) {
    strip.setPixelColor(lastLed, strip.gamma32(strip.ColorHSV(62622, 242, globalBrightness)));

    if (lastLed == 0) {  // clear led before
      strip.setPixelColor(LED_COUNT - 1, 0);
    } else {
      strip.setPixelColor(lastLed - 1, 0);
    }

    lastLed = (lastLed + 1) % LED_COUNT;

    safeStripShow();
    lastTime = millis();
  }
}

void comet() {

  if (modeIsShowing == 1) {
    return;
  }

  static uint32_t lastTime = millis();
  static int lastLed = 0;
  if (millis() - lastTime > ColorTransTime) {
    for (int i = 0; i < LED_COUNT; i++) {

      int distance = (lastLed - i + LED_COUNT) % LED_COUNT;
      int brightness = 255 - (distance * (255 / LED_COUNT)) - (255 - globalBrightness)  ;
      // TelnetPrint(String(brightness));
      if (brightness < 0) { brightness = 0; }

      strip.setPixelColor(i, strip.ColorHSV(62622, 242, brightness));
    }
    lastTime = millis();
    safeStripShow();
    lastLed = (lastLed + 1) % LED_COUNT;
  }
}

void liquidPlasma() {
  if (modeIsShowing == 1) {
    return;
  }
  static uint32_t lastTime = 0;
  static uint8_t wavePhase = 0;  
  static uint16_t baseHue = 0;   

  if (millis() - lastTime > ColorTransTime) {

    for (int i = 0; i < LED_COUNT; i++) {

      // 1. Обчислюємо ідеальну синусоїдальну хвилю для кожного діода.
      // (i * 256 / LED_COUNT) рівномірно розтягує одну хвилю на всю довжину стрічки.
      // + wavePhase змушує цю хвилю рухатися.
      uint8_t wave = strip.sine8((i * 256 / LED_COUNT) + wavePhase);

      // 2. Масштабуємо математичну хвилю під вашу глобальну яскравість
      // Хвиля видає значення від 0 до 255.
      uint8_t pixelBrightness = (wave * globalBrightness) / 255;

      // 3. Робимо плавний градієнт: кожен наступний діод має трохи інший відтінок
      uint16_t pixelHue = baseHue + (i * 2000);

      // 4. Застосовуємо колір
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue, 255, pixelBrightness)));
    }

    safeStripShow();
    lastTime = millis();

    // 5. Рухаємо анімацію далі (змінюючи ці числа, можна міняти характер ефекту)
    wavePhase += 6;  // Наскільки швидко хвиля біжить вперед
    baseHue += 150;  // Наскільки швидко змінюється загальна палітра кольорів
  }
}

void randCircling() {
  if (modeIsShowing == 1) {
    return;
  }
  static uint32_t lastTime = millis();
  static int lastLed = 0;
  if (millis() - lastTime > ColorTransTime) {
    for (int i = 0; i < LED_COUNT; i++) {

      if (i == lastLed) { strip.setPixelColor(i, strip.Color(randomRGBBrightCorrected(), randomRGBBrightCorrected(), randomRGBBrightCorrected())); }

      // else{ strip.setPixelColor(i,0);}
    }

    lastTime = millis();
    safeStripShow();
    lastLed = (lastLed + 1) % LED_COUNT;
  }
}

int8_t randomRGBBrightCorrected() {
  int8_t val = ((int8_t)ESP.random() - (255 -  globalBrightness));
  if(val < 0 ){
    return 0;
  }
  else if(val > 255 ){
    return 255;
  }
  else{
  return val;
  }
}

void fullRainbow(){

  if (modeIsShowing == 1) {
    return;
}
static uint16_t color = 0; 
static uint32_t lastTime = millis();
if(millis() - lastTime > ColorTransTime/20){
  strip.fill(strip.ColorHSV(color, 150, (globalBrightness*0.9)));
  lastTime = millis();
  color += 10;
  safeStripShow();
  }
}

void bzzSet(int power, int time){
  if(bzzStatus == 1 ){
    return;
  } 
  bzzWorkTime = time;
  bzzStartTime = millis();
  bzzStatus = 1;
  CurrentMotorPower = power;
  analogWrite(bzz_pin, power);
  TelnetPrint("motor started"); 
} 

void bzzCheck(){
  if(bzzStatus){
    if(millis() - bzzStartTime >= bzzWorkTime){
      analogWrite(bzz_pin, 0);
      TelnetPrint("motor stopped");
      bzzStatus = 0;

    }

  }
  else{
    return;
  }
}


void Click1() {
  TelnetPrint("first button clicked, mode --");
  bzzSet(100, 50);

  mode = ((mode - 1 )% 10  + 10) % 10; 
  
  String msg = "now is mode: " + String(mode);
  TelnetPrint(msg);
  globalTimer = millis();
  modeIsShowing = 1;
  ledUpdate = true;
  redrawMode = 1;
}


void Click2() {
  bzzSet(100, 100);
  TelnetPrint("second button clicked, mode ++ ");

  mode = ((mode + 1 )% 10  + 10) % 10;

  String msg = "now is mode: " + String(mode);
  TelnetPrint(msg);
  globalTimer = millis();
  modeIsShowing = 1;
  redrawMode = 1;
  ledUpdate = true;

}

void Click3() {
  bzzSet(100, 100);
  switch (mode) {
    case 0:
      globalR -= 10;
      ledUpdate = true;
      msg = " red -10  now RED:" + String(globalR);
      break;
    case 1:
      globalG -= 10;
      ledUpdate = true;
      msg = " green -10  now GREEN:" + String(globalG);
      break;
    case 2:
      globalB -= 10;
      ledUpdate = true;
      msg = " blue -10  now BLUE:" + String(globalB);
      break;

    default:
      ColorTransTime -= 5;
      msg = "now ColorTransTime-----  is: " + String(ColorTransTime);
  }


  TelnetPrint(msg);
  TelnetPrint("third button clicked");
  strip.setPixelColor(6, strip.gamma32(strip.ColorHSV(65536 / 2, 242, 100)));
  safeStripShow();
}

void Click4() {
  bzzSet(100, 100);
  switch (mode) {
    case 0:
      globalR += 10;
      ledUpdate = true;
      msg = " red +10  now RED:" + String(globalR);
      break;
    case 1:
      globalG += 10;
      ledUpdate = true;
      msg = " green +10  now GREEN:" + String(globalG);
      break;
    case 2:
      globalB += 10;
      ledUpdate = true;
      msg = " blue +10  now BLUE:" + String(globalB);
      break;

    default:
      ColorTransTime += 5;
      msg = "now ColorTransTime+++++  is: " + String(ColorTransTime);
  }

  TelnetPrint(msg);
  strip.setPixelColor(4, strip.gamma32(strip.ColorHSV(0)));
  safeStripShow();
  TelnetPrint("fourth button clicked");
}


void DuringLongPressB4() {
  TelnetPrint("fourth button is pressed, INCREASED global brihtness +++++++++");
  globalBrightness += 5;
  String msg = "now brihtness  is: " + String(globalBrightness);
  TelnetPrint(msg);
  bzzSet(100, 10);

}

void DuringLongPressB3() {
  TelnetPrint("third button is pressed, DECRESED global brihtness --------");
  globalBrightness -= 5;
  String msg = "now brihtness  is: " + String(globalBrightness);
  TelnetPrint(msg);
  bzzSet(100, 15);

}

void safeStripShow(){
  if(bzzStatus){
    analogWrite(bzz_pin, 0);
  }
  strip.show();
  
  if(bzzStatus){
    analogWrite(bzz_pin, CurrentMotorPower);
  }

}
