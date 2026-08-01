

#include <Adafruit_NeoPixel.h>

#define LED_PIN   12
#define LED_COUNT 10

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <OneButton.h>

OneButton btn;

const int button1_pin = 14;
const int button2_pin = 5;
const int button3_pin = 4;
const int button4_pin = 13;

int mode = 0 ;

const char* ssid = "3.16";
const char* password = "printer2hrn";

String telnetBuffer = "";

int ColorTransTime = 20;

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_RGB + NEO_KHZ800);

bool ledUpdate = false;

WiFiServer telnetServer(23); //telnet server on port 23
WiFiClient telnetClient;

OneButton button1(
  button1_pin,  // Input pin for the button
  true,        // Button is active LOW
  false         // Disable internal pull-up resistor
);

OneButton button2(
  button2_pin,  // Input pin for the button
  true,        // Button is active LOW
  false         // Disable internal pull-up resistor
);

OneButton button3(
  button3_pin,  // Input pin for the button
  true,        // Button is active LOW
  false         // Disable internal pull-up resistor
);

OneButton button4(
  button4_pin,  // Input pin for the button
  true,        // Button is active LOW
  false         // Disable internal pull-up resistor
);

void setup() {

  Serial.begin(115200);
  Serial.println("Booting");


  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

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
  strip.show(); // Initialize all pixels to 'off'


  button1.attachClick(Click1);
  button2.attachClick(Click2);
  button3.attachClick(Click3);
  button4.attachClick(Click4);




}

  void TelnetPrint(String text) {
  Serial.print(text); // alway in ph port
  if (telnetClient && telnetClient.connected()) {
    telnetClient.println(text); // to the air
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
  
  // telnet -> serial
  if (telnetClient && telnetClient.connected() && telnetClient.available()) {
    while (telnetClient.available()) {
      // Serial.write(telnetClient.read());
      char c = telnetClient.read();
      Serial.write(c);
      if (c == '\n' || c == '\r'){
        telnetBuffer.trim();    
      if (telnetBuffer.length() > 0){
      
        if (telnetBuffer == "time") {
            String msg = "Uptime: " + String(millis() /1000 ) + " seconds ";
              TelnetPrint(msg);    
            }
        if (telnetBuffer == "c1") {
            Click1();
            }
        if (telnetBuffer == "c2") {
            Click2();
            }
        if (telnetBuffer == "c3") {
            Click3();
            }
        if (telnetBuffer == "c4") {
            Click4();
            }        
        if (telnetBuffer == "mode") {
            String msg = "current mode: " + String(mode);
            TelnetPrint(msg);
            }

          telnetBuffer = "";                                
          }
        }
      else{ 
        telnetBuffer += c;
        }                                      
                                
          
        }
    }
  

    // serial -> telnet 
while (Serial.available()) {
      char c = Serial.read();
      if (telnetClient && telnetClient.connected()) {
        telnetClient.write(c);
      }
    }

  if(ledUpdate == true){
    switch(mode){
      case 0:
        strip.fill(strip.Color(50, 0, 0));
        break;
    
      case 1:
        strip.fill(strip.Color(0, 50, 0));
        break;

      case 2:
        strip.fill(strip.Color(50, 0, 50));
        break;

      case 3:
        RandomColorOne();
        break;
    }

    strip.show() ;
    ledUpdate = false; 
  }
  
  if(mode == 3){
    RandomColorOne();
  }

  if(mode == 4){
    ColorTransfusion();
  }

  button1.tick();
  button2.tick(); 
  button3.tick();
  button4.tick();

  delay(1);
                                  
  }

  void RandomColorOne(){
    static uint32_t lastTime = millis(); 
    for(i = 0; i < LED_COUNT ; i ++){
    if(millis() - lastTime > 300){
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV((uint16_t)ESP.random()))); //random full bright corrected color for i led
      time = millis();
      strip.show() ;
      } 
    } 
  }
  
  void ColorTransfusion(){
  static uint32_t lastTime = millis();
  static uint16_t baseColor = 0;
  
  if(millis() - lastTime > ColorTransTime){

    for( int i = 0; i < LED_COUNT ; i ++){
      uint16_t hue = baseColor + (i * 65535L / LED_COUNT); 
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(hue)); 
    } 
  }
    strip.show();
    baseColor += 100;
    lastTime = millis();
  }
  

  

  



  void Click1() {
    TelnetPrint("first button clicked, mode ++");
    if(mode >= 3){
      mode = 3;
      TelnetPrint("!there is no more modes");
    }
   else{
    mode ++; 
    ledUpdate = true;
   }
   String msg = "now is mode: " + String(mode);
   TelnetPrint(msg);

}


  void Click2() {
    TelnetPrint("second button clicked, mode -- ");
    if(mode <= 0){
      mode = 0;
      TelnetPrint("!mode is already lowest");
    }
    else{
      mode -- ;
      ledUpdate = true;
    }
   String msg = "now is mode: " + String(mode);
   TelnetPrint(msg);

}

  void Click3() {
    if(mode == 4){
      ColorTransTime += 5;
    }
    TelnetPrint("third button clicked");
}

  void Click4() {
    if(mode == 4){
      ColorTransTime -= 5;
    }
    TelnetPrint("fourth button clicked");
}
