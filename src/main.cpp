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
#include <Adafruit_SSD1306.h>   // Consider using a different library, text only....
#include <OneWire.h>
#include <DallasTemperature.h>

#define wifi_ssid "PLUSNET-G3MF"
#define wifi_password "zenith(99)"
//#define wifi_ssid "TP-LINK_6F2267"
//#define wifi_password "026F2267"

#define mqtt_server "pi3.lan"
//#define mqtt_server "test.mosquitto.org"
#define mqtt_user "admin"
#define mqtt_password "dune99"

#define publish_local_topic "local/temperature"
#define subscribe_topic "tele/sonoff/SENSOR"

#define ONE_WIRE_BUS 14 // which pin the ds18b20 is on...
#define OLED_RESET 0  // GPIO0


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

// Create a display object
// SCL GPIO5
// SDA GPIO4

Adafruit_SSD1306 display(OLED_RESET);

void setup_wifi() {
  delay(10);

  // text display tests
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  Serial.println();
  Serial.println("Connecting to: ");
  Serial.print(wifi_ssid);
  display.println("Connecting to: ");
  display.print(wifi_ssid);
  display.display();
  // We start by connecting to a WiFi network

  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    display.print(".");
    Serial.print(".");
    display.display();
  }

  display.setCursor(0,0);
  display.clearDisplay();
  Serial.print("WiFi Connected, IP Address: ");
  Serial.println(WiFi.localIP());
  display.println("WiFi connected");
  display.println("IP address: ");
  display.println(WiFi.localIP());
  display.display();
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

  Serial.print("The value of Date from the root array is: ");
    Serial.println(Date);
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

  sensors.begin();                            // Start the DS temp sensors
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  Serial.begin(115200);
  // Clear the buffer.
  display.clearDisplay();
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);               // Creat the callback function for Subscribed messages to arrive on
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  // call sensors.requestTemperatures() to issue a global temperature
  // request to all devices on the bus
  //Serial.print("Requesting temperatures...");
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
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(0,0);
      display.print("Local: ");
      display.print(temp,1);
      display.print((char)247); // degree symbol
      display.setTextSize(1);
      display.println("C");
      display.print("  DS1: ");
      display.print(Temp1,1);
      display.print((char)247); // degree symbol
      display.setTextSize(1);
      display.println("C");
      display.print("  DS2: ");
      display.print(Temp2,1);
      display.print((char)247); // degree symbol
      display.setTextSize(1);
      display.println("C");
      display.println(Date);
      display.display();
      delay(1000);
   // }
 }
}
