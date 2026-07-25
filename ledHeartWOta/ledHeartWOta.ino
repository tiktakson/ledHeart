
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


const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";


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
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  button1.attachClick(Click1);
  button2.attachClick(Click2);
  button3.attachClick(Click3);
  button4.attachClick(Click4);


}

void loop() {

  ArduinoOTA.handle();

}

  void Click1() {
    Serial.printf("first button clicked");
}

  void Click2() {
    Serial.printf("second button clicked");
}

  void Click3() {
    Serial.printf("third button clicked");
}

  void Click4() {
    Serial.printf("fourth button clicked");
}
