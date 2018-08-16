/*
This sketch connects to my MQTT broker, reads the temperature from
a Dallas DS1820B on PIN 5, then publishes it to a random temperature_topic
It also then displays the temprerature on the OLED display.

OLED display uses I2C, on default Wemos SDA and SCL pins


Written by Tony Bell (with help from lots of other clever people!)
17/7/2018

*/

#include <Arduino.h>

#include <ArduinoJson.h>        // For parsing messages from MQTT
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <PubSubClient.h>
#include <Adafruit_GFX.h>
// #include <Adafruit_SSD1306.h>   // Consider using a different library, text only....
#include <OneWire.h>
#include <DallasTemperature.h>

#include "SPI.h"
#include "Adafruit_ILI9341.h"     // use this one for the 2.4" TFT Touch screen (SPI)
#include "fonts/FreeSans9pt7b.h"  // get some nice fonts!
#include "fonts/FreeSans12pt7b.h"
#include "fonts/FreeSans18pt7b.h"
#include "fonts/FreeSans24pt7b.h"

// For the Adafruit shield, these are the default.
#define TFT_DC D1
#define TFT_CS D2

#define wifi_ssid "PLUSNET-G3MF"
#define wifi_password "zenith(99)"
//#define wifi_ssid "TP-LINK_6F2267"
//#define wifi_password "026F2267"

#define mqtt_server "pi3.lan"
//#define mqtt_server "test.mosquitto.org"
#define mqtt_user "admin"
#define mqtt_password "dune99"

#define publish_local_topic "local/temperature"
#define subscribe_topic "tele/Test/SENSOR"

#define ONE_WIRE_BUS 0 // which pin the ds18b20 is on... D5=GPIO14, D0=GPIO16, D3=GPIO0
//#define OLED_RESET 0  // GPIO0

// Use hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC
Adafruit_ILI9341 display = Adafruit_ILI9341(TFT_CS, TFT_DC);

WiFiClient espClient;                  //Setup wifi stack
PubSubClient client(espClient);        //Setup MQTT client
OneWire oneWire(ONE_WIRE_BUS);         // Setup a oneWire instance to communicate with any OneWire devices
DallasTemperature sensors(&oneWire);   // Pass our oneWire reference to Dallas Temperature.

// Declare Variables here....

long lastMsg = 0;
float temp = 0.0;
float diff = 1.0;
float Temp1;
float Temp2;
char json[300];
const char* Date; // must use const char*'s to extract strings from JSON object
uint8_t rotation = 1;   // set the screen orientation mode

// Create variables to hold Screen Box positions, then the Box erase function can use these and adjust the actual co-ordinates by math!
uint8_t radius = 10;

long Big_box_x=10, Big_box_y=10, Big_box_w=200, Big_box_h=130;
long Button_box_x=220, Button_box_y=10, Button_box_w=80, Button_box_h=220;
long Msg_box_x=10, Msg_box_y=150, Msg_box_w=200,Msg_box_h=80;

// create Origin start points in each box
long Big_box_origin_x = Big_box_x + 30, Big_box_origin_y = Big_box_y + 30;
long Msg_box_origin_x = Msg_box_x + 20, Msg_box_origin_y = Msg_box_y +20;

void setup_wifi() {
  delay(10);

  // text display tests
  display.setRotation(rotation);
  display.setFont(&FreeSans9pt7b);            // Set the font
  display.setTextSize(1);
  display.setTextColor(ILI9341_WHITE);
  display.setCursor(0,10);
  Serial.println();
  Serial.println("Connecting to: ");
  Serial.print(wifi_ssid);
  display.println("Connecting to: ");
  display.print(wifi_ssid);
  //  ; only used on oled displays
  // We start by connecting to a WiFi network

  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    display.print(".");
    Serial.print(".");
    //  ;
  }

  yield();  // let the esp8266 think a bit
  Serial.print("WiFi Connected, IP Address: ");
  Serial.println(WiFi.localIP());
  display.println("WiFi connected");
  display.println("IP address: ");
  display.println(WiFi.localIP());
  //  ;
  delay(1000);

}

void callback(char* topic, byte* payload, unsigned int length) {
  // once its working, write it all to the OLED Screen rather than the serial port...
  // Display all received messages in little text, only the payload, not the topic.
  // Declare the jsonbuffer here, it needs to be collapsed after use...

  StaticJsonBuffer<300> jsonBuffer;
  Serial.println("Message arrived...");
// Messages arrive in the "payload" buffer from MQTT, so simply Parse it straight through...
// Now convert the json buffer to a parsed JSON object
    JsonObject& root = jsonBuffer.parseObject(payload);
//    JsonArray& root = jsonBuffer.parseArray(payload);
    // Test if parsing succeeds.
    if (!root.success()) {
      Serial.println("parseObject() failed, probably the buffer isnt big enough");
      return;
    }

  // now try and extract the temperature ..... good luck!
  //  root.prettyPrintTo(Serial);

    Date = root["Time"]; //Date and time are the first entry in the JSON string
    Temp1 = root["DS18x20"]["DS1"]["Temperature"]; // use the square brackets to address the nested parts of the JSON String
    Temp2 = root["DS18x20"]["DS2"]["Temperature"];
}
void paint_screen()
{
  display.setRotation(rotation);              // set the orientation of the screen
  display.fillScreen(ILI9341_BLACK);          // Clear the screen
  display.drawRoundRect(2, 2, 320-6, 240-6, radius, ILI9341_GREEN);
  display.drawRoundRect(Big_box_x, Big_box_y,Big_box_w, Big_box_h, radius, ILI9341_GREEN);
  display.drawRoundRect(Button_box_x, Button_box_y, Button_box_w, Button_box_h, radius, ILI9341_GREEN);
  display.drawRoundRect(Msg_box_x, Msg_box_y, Msg_box_w, Msg_box_h, radius, ILI9341_GREEN);

}

void clear_Bigbox(){
  // Variables: Big_box_x
  display.fillRect(Big_box_x+6, Big_box_y+6, Big_box_w-10, Big_box_h-10,ILI9341_BLACK);
}

void clear_Buttonbox(){
  display.fillRect(Button_box_x+6, Button_box_y+6, Button_box_w-10, Button_box_h-10,ILI9341_BLACK);
}

void clear_Msgbox(){
  display.fillRect(Msg_box_x+6, Msg_box_y+6, Msg_box_w-10, Msg_box_h-10,ILI9341_BLACK);
}


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    // If you do not want to use a username and password, change next line to
    // if (client.connect("ESP8266Client")) {
    if (client.connect("ESP8266Client", mqtt_user, mqtt_password)) {
    //if (client.connect("ESP8266Client")) {
      Serial.println("MQTT: connected");
      Serial.print("Subscribing to: ");
      Serial.println(subscribe_topic);

      client.subscribe(subscribe_topic);              // and re-subscribe to a "topic"
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

bool checkBound(float newValue, float prevValue, float maxDiff) {
  return !isnan(newValue) &&
         (newValue < prevValue - maxDiff || newValue > prevValue + maxDiff);
}

void setup() {

  Serial.begin(115200);
  display.begin();
  display.fillScreen(ILI9341_BLACK);
  sensors.begin();                            // Start the DS temp sensors
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);               // Creat the callback function for Subscribed messages to arrive on
  paint_screen();                             // Display the Screen Outline stuff
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  // call sensors.requestTemperatures() to issue a global temperature
  // request to all devices on the bus
  sensors.requestTemperatures(); // Send the command to get temperatures

  // After we got the temperatures, we can print them here.
  // We use the function ByIndex, and as an example get the temperature from the first sensor only.

  //Serial.print("Temperature for the device 1 (index 0) is: ");
  //Serial.println(sensors.getTempCByIndex(0),1);

  // Now lets disply the result on the OLED and publish it to MQTT

  long now = millis();                          //  Only check the temp every 5 seconds
  if (now - lastMsg > 5000) {
    lastMsg = now;

    float newTemp = sensors.getTempCByIndex(0);


    // if (checkBound(newTemp, temp, diff)) {      // check if the temp has changed
      temp = newTemp;
      Serial.print("New temperature:");
      Serial.println(String(temp).c_str());
      client.publish(publish_local_topic, String(temp).c_str(), true);

      display.setFont(&FreeSans24pt7b);
      display.setTextColor(ILI9341_WHITE);

      clear_Bigbox();
      display.setCursor(Big_box_origin_x + 15, Big_box_origin_y + 40);
      display.print(temp,1);
      display.print((char)247); // degree symbol
      display.println("c");

      clear_Msgbox();
      display.setCursor(Msg_box_origin_x, Msg_box_origin_y);
      display.setFont(&FreeSans9pt7b);      // in a smaller font display the remote temps
      display.print("  DS1: ");
      display.print(Temp1,1);
      display.print((char)247); // degree symbol
      display.println("c");

      display.setCursor(Msg_box_origin_x, Msg_box_origin_y + 25);
      display.print("  DS2: ");
      display.print(Temp2,1);
      display.print((char)247); // degree symbol
      display.print("c");
      delay(1000);
   // }
 }
}
