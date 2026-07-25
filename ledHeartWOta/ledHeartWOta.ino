
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


const char* ssid = "";
const char* password = "";

String telnetBuffer = "";

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


  button1.attachClick(Click1);
  button2.attachClick(Click2);
  button3.attachClick(Click3);
  button4.attachClick(Click4);


}

  void TelnetPrint(String text) {
  text = text + "\r\n" ;
  Serial.print(text); // alway in ph port
  if (telnetClient && telnetClient.connected()) {
    telnetClient.print(text); // to the air
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
        if (telnetBuffer == "hello") {
            TelnetPrint("hello from esp8266 too\r\n");
            } 
            
        if (telnetBuffer == "time") {
            String msg = "Uptime: " + String(millis() /1000 ) + " seconds ";
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
if (Serial.available()) {
  size_t len = Serial.available();
  uint8_t sbuf[len];
  Serial.readBytes(sbuf, len);
  
  if (telnetClient && telnetClient.connected()) {
    telnetClient.write(sbuf, len);
  }
}

  button1.tick();
  button2.tick(); 
  button3.tick();
  button4.tick();


                                  
  }

  void Click1() {
    TelnetPrint("first button clicked");
}

  void Click2() {
    TelnetPrint("second button clicked");
}

  void Click3() {
    TelnetPrint("third button clicked");
}

  void Click4() {
    TelnetPrint("fourth button clicked");
}
