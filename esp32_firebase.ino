//Elias Zamel Weaam hammud
//****************************************************************************************************************
// This project is about science experiment dedicated to help the users step by step in there science experiment
// using the scale and esp32 with oled screen and contactoing by firebase and robophone app, we made a project to
// help with scaling and calibrating the scale.
//****************************************************************************************************************

#include <Arduino.h>
#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>
#include <WiFiManager.h>


#include "HX711.h"
#include "soc/rtc.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 16;
const int LOADCELL_SCK_PIN = 4;
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define BUTTON_PIN 19
#define LED 15
// Declaration for SSD1306 display connected using I2C
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display = Adafruit_SSD1306(SCREEN_WIDTH, 32, &Wire);
HX711 scale;
bool calibrated = false;

//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"


// Insert Firebase project API Key
#define API_KEY "AIzaSyCwExrFhZasLqb3LQMzpgiLWpwvePkutS4"

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "https://esp32-demo-57e38-default-rtdb.europe-west1.firebasedatabase.app/" 

//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;
int factor = 0;
//**************************************** SETUP ************************************//
void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  WiFi.mode(WIFI_STA);
  Serial.begin(57600);
  WiFiManager wm;
  bool result;
  result= wm.autoConnect("Robophone","123456789");
  if(!result){
    Serial.println("Failed to connect");
  }
  else{
    Serial.println("CONNECTED <WELCOME>");
  }

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }
  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Serial.println("OLED FeatherWing test");
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Address 0x3C for 128x32


  //Firebase.RTDB.setInt(&fbdo, "/Robophone/5669122872442880/factor",-20);

  Serial.println("OLED begun");
  display.display();
  delay(1000);

  // Clear the buffer.
  display.clearDisplay();
  display.display();
  
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.print("Initializing the scale");
  display.display();
  display.clearDisplay();
  display.display();
  Serial.println("Initializing the scale");

  //*** Debug
  Serial.printf(" Get factor --- %s", Firebase.RTDB.getInt(&fbdo, "/Robophone/5669122872442880/factor") ? String(fbdo.intData()).c_str() : fbdo.errorReason().c_str());
  //***

  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

    while(!calibrated){
      display.clearDisplay();
      display.display();
      if (scale.is_ready()){
        scale.set_scale();   
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0,0); 
        display.println("Tare, remove any\nweights from the scale.");
        display.display();
        delay(5000);
        display.clearDisplay();
        display.display();
        scale.tare();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0,0); 
        display.println("Tare Done ...");
        display.display();
        delay(1000);
        display.clearDisplay();
        display.display();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0,0);
        display.print("Place a known object\non the scale\n");
        display.display();
        delay(5000);
        long reading = scale.get_units(10);
        display.print("Result: \n ");
        display.print(round(reading));

        display.display();
        if(reading < -5000 ){
          Firebase.RTDB.setInt(&fbdo, "/Robophone/5669122872442880/weight/value",reading);
        }
      }
      if(Firebase.RTDB.getInt(&fbdo, "/Robophone/5669122872442880/factor")){
        factor = fbdo.intData();
      }
      else{
        Serial.println( fbdo.errorReason());
      }
      //Serial.printf("INSIDE :: Get factor --- %s \n", Firebase.RTDB.getInt(&fbdo, "/Robophone/5669122872442880/factor") ? String(fbdo.intData()).c_str() : fbdo.errorReason().c_str());
      if(factor != 0 ){
        calibrated = true;
      }
      delay(5000);
    } 
  scale.set_scale(factor);                    
  scale.tare();              // reset the scale to 0
}  // closes the setup function

void loop() {
  if(calibrated){
    if ( digitalRead(BUTTON_PIN) == 0)
      {
       scale.set_scale(factor);  
       scale.tare();
      }
    Serial.print("Weight is: ");
    Serial.print(scale.get_units(), 1);
    Serial.print("\n");

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
    display.print("Digital weight:\n");
    display.print(round(scale.get_units(10)));
    display.display();

    if (Firebase.ready() && signupOK){
      if (Firebase.RTDB.setInt(&fbdo, "Robophone/5669122872442880/weight/value", round(scale.get_units(10)))){
        Serial.println("PASSED");
        Serial.println("PATH: " + fbdo.dataPath());
        Serial.println("TYPE: " + fbdo.dataType());
      }
      else {
        Serial.println("FAILED");
        Serial.println("REASON: " + fbdo.errorReason());
      }
  }
  Serial.print("\n");
  scale.power_down();             
  delay(1000);
  scale.power_up();
  }  
}