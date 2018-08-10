/*
This sketch connects to my MQTT broker, reads the temperature from
a Dallas DS1820B on PIN 5, then publishes it to a random temperature_topic
It also then displays the temprerature on the OLED display.

OLED display uses I2C, on default Wemos SDA and SCL pins


Written by Tony Bell (with help from lots of other clever people!)
17/7/2018

*/

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <Wire.h>
#include <PubSubClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>   // Consider using a different library, text only....
#include <OneWire.h>
#include <DallasTemperature.h>

//#define wifi_ssid "PLUSNET-G3MF"
//#define wifi_password "zenith(99)"
#define wifi_ssid "TP-LINK_6F2267"
#define wifi_password "026F2267"

//#define mqtt_server "pi3.lan"
#define mqtt_server "test.mosquitto.org"
//#define mqtt_user "admin"
//#define mqtt_password "dune99"

#define temperature_topic "sensor/temperature"

#define ONE_WIRE_BUS 14 // which pin the ds1820b is on...
#define OLED_RESET 0  // GPIO0


WiFiClient espClient;                  //Setup wifi stack
PubSubClient client(espClient);        //Setup MQTT client
OneWire oneWire(ONE_WIRE_BUS);         // Setup a oneWire instance to communicate with any OneWire devices
DallasTemperature sensors(&oneWire);   // Pass our oneWire reference to Dallas Temperature.

// Declare Variables here....

long lastMsg = 0;
float temp = 0.0;
float diff = 1.0;

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
  display.println("Connecting to: ");
  display.print(wifi_ssid);
  display.display();
  // We start by connecting to a WiFi network

  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    display.print(".");
    display.display();
  }

  display.setCursor(0,0);
  display.clearDisplay();
  display.println("WiFi connected");
  display.println("IP address: ");
  display.println(WiFi.localIP());
  display.display();
  delay(1000);

}

void callback(char* topic, byte* payload, unsigned int length) {
  // once its working, write it all to the OLED Screen rather than the serial port...
  // Display all received messages in little text, only the payload, not the topic.

  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

//  This bit doesnt work, its a graphics screen, and so prints Characyters in "transparent mode" see
//  Arduino forums for a work around, or use a differtn text based library

  display.setTextSize(1);
  display.setCursor(0,0);
  // clear the message line first...
  for (unsigned int i = 0; i < 21; i++) {
    display.print(" ");
  }
  display.println();
  display.display();

/*display.setCursor(0,0);
  for (unsigned int i = 0; i < length; i++) {
    display.print((char)payload[i]);
  }
  */

  display.setCursor(0,0);
    for (unsigned int i = 0; i < 21; i++) {
      display.print((char)payload[i]);
    }
    display.println();
  display.display();


}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    // If you do not want to use a username and password, change next line to
    // if (client.connect("ESP8266Client")) {
    //if (client.connect("ESP8266Client", mqtt_user, mqtt_password)) {
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      client.subscribe("#");              // and re-subscribe to a "topic"
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

  //Serial.println("DONE");

  // After we got the temperatures, we can print them here.
  // We use the function ByIndex, and as an example get the temperature from the first sensor only.

  //Serial.print("Temperature for the device 1 (index 0) is: ");
  //Serial.println(sensors.getTempCByIndex(0),1);

  // Now lets disply the result on the OLED and publish it to MQTT


  long now = millis();                          //  Only check the temp every 2 seconds
  if (now - lastMsg > 5000) {
    lastMsg = now;

    float newTemp = sensors.getTempCByIndex(0);

    if (checkBound(newTemp, temp, diff)) {      // check if the temp has changed
      temp = newTemp;
      Serial.print("New temperature:");
      Serial.println(String(temp).c_str());
      client.publish(temperature_topic, String(temp).c_str(), true);
      display.clearDisplay();
      display.setTextSize(2);
      display.setTextColor(WHITE);
      display.setCursor(30,15);
      display.print(temp,1);
      display.print((char)247); // degree symbol
      display.setTextSize(2);
      display.println("C");
      display.display();
      delay(1000);
   }
 }
}
