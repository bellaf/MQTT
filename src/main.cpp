/*
This sketch connects to my MQTT broker, reads the temperature from
a Dallas DS1820B on PIN 5, then publishes it to a random temperature_topic
It also then displays the temprerature on the OLED display.

OLED display uses I2C, on default Wemos SDA and SCL pins


Written by Tony Bell (with help from lots of other clever people!)
17/7/2018

*/

#include <Arduino.h>
//#include <Encoder.h>  Cant get the rotary to work at present....
#include <JC_Button.h>   // Lets try the Button library.... just until we get a touch screen

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
#define mqtt_user "admin"
#define mqtt_password "dune99"



#define ONE_WIRE_BUS 2 // which pin the ds18b20 is on... D5=GPIO14, D0=GPIO16, D3=GPIO0


// Declare Variables here....
// These are for the temp buttons....

const byte
 up_Pin = 2,
 down_Pin = 0;

const char  *publish_temp_topic    = "tele/thermostat/temperature",
            *heating_control_topic = "cmnd/boiler/POWER",    // Send the Relay command to a "sonoff" called boiler...
            *heating_status_topic  = "stat/boiler/POWER1",    // Boiler relay state topic
            *subscribe_topic       = "tele/Test/SENSOR";          // get temp readings from node called Test ... might not use this

char _value[10]; // used to hold temp str buffer for dtostr() func

long lastMsg = 0;
float Current_temp = 0.0;
float Set_point = 18;         // Default Set points
float Set_back = 14;          // Set night set point
float diff = 0.2;             // Difference required before displaying a new temperature
float Temp1;
float Temp2;
char json[300];               // JSON Buffer for parsing remote temp sense info
const char* Date;             // must use const char*'s to extract strings from JSON object
int rotation = 1;             // set the screen orientation mode
long oldPosition  = -999;     // for Rotary encoder use....
bool has_changed = 0;         //  Has the encoder position changed?
bool Night_set_back = 0;      // Default is NOT Night Setback mode
bool Heating = 0;             //  0=FALSE=OFF 1=TRUE=on    -  Relay state to control the heating

// Now instatiate objects:

// Use hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC
Adafruit_ILI9341 display = Adafruit_ILI9341(TFT_CS, TFT_DC);

WiFiClient espClient;                  //Setup wifi stack
PubSubClient client(espClient);        //Setup MQTT client
OneWire oneWire(ONE_WIRE_BUS);         // Setup a oneWire instance to communicate with any OneWire devices
DallasTemperature sensors(&oneWire);   // Pass our oneWire reference to Dallas Temperature.
Button btn_Up(up_Pin, 25, true ,false);
Button btn_Down(down_Pin, 25, true,false);

// Change these two numbers to the pins connected to your encoder.
//   Best Performance: both pins have interrupt capability
//   Good Performance: only the first pin has interrupt capability
//   Low Performance:  neither pin has interrupt capability
// Encoder myEnc(2, 15);
//   avoid using pins with LEDs attached

// add some MQTT topic variable here to drive a remote Sonoff relay node for heating on or Off

// Create variables to hold Screen Box positions, then the Box erase function can use these and adjust the actual co-ordinates by math!
uint8_t radius = 10;

// create Origin start points in each box
int x=0, y=1, w=2, h=3, origin=4;  // generic variables to index into the cordinate array 0-4
int MB_split = 60;                  //  sets the split point for the Msg box width

long Big_box_x=10,      Big_box_y=10,                         Big_box_w=200,    Big_box_h=110;
long Button_box_x=220,  Button_box_y=10,                      Button_box_w=85,  Button_box_h=220;
long Msg_box_x=10,      Msg_box_y=Big_box_h + Big_box_y + 5,  Msg_box_w=200,    Msg_box_h=100;

long  Big_box_origin_x = Big_box_x + 30,
      Big_box_origin_y = Big_box_y + 30;

long  Msg_box_origin_x = Msg_box_x + 6,
      Msg_box_origin_y = Msg_box_y + 6;


long Msg_box_value_x = Msg_box_origin_x + (Msg_box_w / 2)+5, Msg_box_value_y = Msg_box_origin_y,
      Msg_box_value_w = (Msg_box_w / 2)+5, Msg_box_value_h = Msg_box_h ;

long  MB_text_line1_x = Msg_box_origin_x,
      MB_text_line1_y = Msg_box_origin_y,
      MB_text_line1_w = Msg_box_w - MB_split - 1,
      MB_text_line1_h = 22;

long  MB_text_line2_x = Msg_box_origin_x,
      MB_text_line2_y = MB_text_line1_y + 22,
      MB_text_line2_w = Msg_box_w - MB_split - 1,
      MB_text_line2_h = 22;

long  MB_text_line3_x = Msg_box_origin_x,
      MB_text_line3_y = MB_text_line2_y + 22,
      MB_text_line3_w = Msg_box_w - MB_split - 1,
      MB_text_line3_h = 22;

long  MB_text_line4_x = Msg_box_origin_x,
      MB_text_line4_y = MB_text_line3_y + 22,
      MB_text_line4_w = Msg_box_w - MB_split - 1,
      MB_text_line4_h = 22;



long Up_button_x = Button_box_x + 3;
long Up_button_y = Button_box_y + 3;
long Up_button_w = Button_box_w - 6;
long Up_button_h = (Button_box_h / 2) - 6 ;

long Down_button_x = Button_box_x + 3;
long Down_button_y = Up_button_y + Up_button_h + 3;
long Down_button_w = Button_box_w - 6;
long Down_button_h = (Button_box_h / 2) - 6;

long Middle_up_x = Up_button_x + (Up_button_w / 2);
long Middle_up_y = Up_button_y + (Up_button_h / 2);
long Middle_down_x = Down_button_x + (Down_button_w / 2);
long Middle_down_y = Down_button_y + (Down_button_h / 2);


void setup_wifi() {
  delay(10);

  display.setRotation(rotation);              // Landscape mode
  display.setFont(&FreeSans9pt7b);            // Set the font
  display.setTextSize(1);
  display.setTextColor(ILI9341_WHITE);
  display.setCursor(0,15);                    // Remember, Cursor position for these fonts is not the same as rectangle.
  display.println("Connecting to: ");
  display.print(wifi_ssid);

  // We start by connecting to a WiFi network

  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    display.print(".");
  }

  yield();                                    // let the esp8266 think a bit
  Serial.print("WiFi Connected, IP Address: ");
  Serial.println(WiFi.localIP());
  display.println("WiFi connected");
  display.print("IP address: ");
  display.println(WiFi.localIP());
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

void clear_Buttonbox(){
  display.fillRect(Button_box_x+6, Button_box_y+6, Button_box_w-10, Button_box_h-10,ILI9341_BLACK);
}


// used to display message on one of the msg lines
void Msg_out(int line, char* msg, char* value){

  display.setFont(&FreeSans9pt7b);

  switch (line) {
    case 1:
      if (strlen(msg)!=0){
        display.setCursor(MB_text_line1_x, MB_text_line1_y + 15);
        display.fillRect(MB_text_line1_x, MB_text_line1_y, MB_text_line1_w, MB_text_line1_h, ILI9341_BLACK);
        display.print(msg);
      }
      if (strlen(value)!=0){
        display.setCursor(MB_text_line1_x + (Msg_box_w - MB_split), MB_text_line1_y + 15);
        display.fillRect(MB_text_line1_x + (Msg_box_w - MB_split + 1), MB_text_line1_y, MB_split - 8, MB_text_line1_h, ILI9341_BLACK);
        display.print(value);
      }
      break;

    case 2:
      if (strlen(msg)!=0){
        display.setCursor(MB_text_line2_x, MB_text_line2_y + 15);
        display.fillRect(MB_text_line2_x, MB_text_line2_y, MB_text_line2_w, MB_text_line2_h, ILI9341_BLACK);
        display.print(msg);
      }
      if (strlen(value)!=0){
        display.setCursor(MB_text_line2_x + (Msg_box_w - MB_split), MB_text_line2_y + 15);
        display.fillRect(MB_text_line2_x + (Msg_box_w - MB_split + 1), MB_text_line2_y, MB_split - 8, MB_text_line1_h, ILI9341_BLACK);
        display.print(value);
      }

      break;

    case 3:
      if (strlen(msg)!=0){
        display.setCursor(MB_text_line3_x, MB_text_line3_y + 15);
        display.fillRect(MB_text_line3_x, MB_text_line3_y, MB_text_line3_w, MB_text_line3_h, ILI9341_BLACK);
        display.print(msg);
      }
      if (strlen(value)!=0){
        display.setCursor(MB_text_line3_x + (Msg_box_w - MB_split), MB_text_line3_y + 15);
        display.fillRect(MB_text_line3_x + (Msg_box_w - MB_split + 1), MB_text_line3_y, MB_split - 8, MB_text_line3_h, ILI9341_BLACK);
        display.print(value);
      }

      break;

    case 4:
      if (strlen(msg)!=0){
        display.setCursor(MB_text_line4_x, MB_text_line4_y + 15);
        display.fillRect(MB_text_line4_x, MB_text_line4_y, MB_text_line4_w, MB_text_line4_h, ILI9341_BLACK);
        display.print(msg);
      }
      if (strlen(value)!=0){
        display.setCursor(MB_text_line4_x + (Msg_box_w - MB_split), MB_text_line4_y + 15);
        display.fillRect(MB_text_line4_x + (Msg_box_w - MB_split + 1), MB_text_line4_y, MB_split - 8, MB_text_line4_h, ILI9341_BLACK);
        display.print(value);
      }

      break;
  }

}

void paint_screen(){

  display.setRotation(rotation);              // set the orientation of the screen
  display.fillScreen(ILI9341_BLACK);          // Clear the screen
// Draw the screen outline
  display.drawRoundRect(2, 2, 320-6, 240-6, radius, ILI9341_GREEN);
// Draw the Big box outline
  display.drawRoundRect(Big_box_x, Big_box_y,Big_box_w, Big_box_h, radius, ILI9341_GREEN);
// Draw the Button Outline
  display.drawRoundRect(Button_box_x, Button_box_y, Button_box_w, Button_box_h, radius, ILI9341_GREEN);
    // Draw the buttons
    display.drawRoundRect(Up_button_x, Up_button_y, Up_button_w, Up_button_h, radius, ILI9341_GREEN);
    display.drawRoundRect(Down_button_x, Down_button_y, Down_button_w, Down_button_h, radius, ILI9341_GREEN);
    // Now the arrows.....
    display.fillTriangle(Middle_up_x - 20, Middle_up_y + 20, Middle_up_x ,Middle_up_y - 20, Middle_up_x + 20, Middle_up_y + 20, ILI9341_GREEN);
    display.fillTriangle(Middle_down_x - 20, Middle_down_y - 20, Middle_down_x + 20, Middle_down_y -20, Middle_down_x, Middle_down_y + 20, ILI9341_GREEN);
// Draw the Message box
  display.drawRoundRect(Msg_box_x, Msg_box_y, Msg_box_w, Msg_box_h, radius, ILI9341_GREEN);
  display.setCursor(Msg_box_origin_x, Msg_box_origin_y);

// Display initial Text values in Text boxes along with any default settings for Values:

  Msg_out(1, "Set point", dtostrf(Set_point, 4, 1, _value ));
  Msg_out(2, "Set Back",  dtostrf(Set_back,  4, 1, _value ));
  Msg_out(3, "Heating", "OFF");
  Msg_out(4, "TBA", "0");

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

void read_Buttons(){
  btn_Up.read();
  btn_Down.read();

  if (btn_Up.wasPressed()) {

    Set_point = Set_point + 0.1 ;
    Msg_out(1, "", dtostrf(Set_point, 4, 1, _value )); // display set_point

  }

  if (btn_Down.wasPressed()) {

    Set_point = Set_point - 0.1 ;
    Msg_out(1, "", dtostrf(Set_point, 4, 1, _value )); // display set_point
  }

}

void heating_Control(){
  if ((Current_temp < Set_point) & !Heating) {   // If heating isnt on and its colder than the current setpoint, turn it on
    Serial.println("Heating going: ON");
    Heating = true;
    client.publish(heating_control_topic, "ON");              // Turn on remote relay
    Msg_out(3, "", "ON");
  }
  else if ((Current_temp >= Set_point) & Heating) {
    Serial.println("Heating going: OFF");
    Heating = false;
    client.publish(heating_control_topic, "OFF");              // Turn OFF remote relay
    Msg_out(3, "", "OFF");
  }

}

void read_Temperature(){

  long now = millis();                          //  Only check the temp every 5 seconds
  if (now - lastMsg > 5000) {

    lastMsg = now;
    sensors.requestTemperatures(); // Send the command to get temperatures
    delay(100);
    float newTemp = sensors.getTempCByIndex(0);

       if (checkBound(newTemp, Current_temp, diff)) {      // check if the temp has changed, if it has, is it over or under the set point... action Heating relay accordingly

          Current_temp = newTemp;
          client.publish(publish_temp_topic, String(Current_temp).c_str(), true);
          display.setFont(&FreeSans24pt7b);
          display.setTextColor(ILI9341_WHITE);
          display.fillRect(Big_box_x+6, Big_box_y+6, Big_box_w-10, Big_box_h-10,ILI9341_BLACK); // Clear the display
          display.setCursor(Big_box_origin_x + 15, Big_box_origin_y + 40);
          display.print(Current_temp,1);
          display.print((char)247); // degree symbol
          display.println("c");
    }
  }
}

void setup() {

  Serial.begin(115200);
  btn_Up.begin();
  btn_Down.begin();
  display.begin();
  display.fillScreen(ILI9341_BLACK);
  sensors.begin();                            // Start the DS temp sensors
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);               // Creat the callback function for Subscribed messages to arrive on
//  myEnc.write(Set_point);                  // Set the initial value of the Encoder position 10 times larger to use increment of 0.1 degree per click
  paint_screen();                             // Display the Screen Outline stuff
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  read_Buttons();

  read_Temperature();   // Reads the temperature, and checks if heating needs ot be on or off....

  heating_Control();

}
