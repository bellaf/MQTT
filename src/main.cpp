#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <Wire.h>
#include <PubSubClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define wifi_ssid "PLUSNET-G3MF"
#define wifi_password "zenith(99)"

#define mqtt_server "pi3.lan"
#define mqtt_user "admin"
#define mqtt_password "dune99"

#define humidity_topic "sensor/humidity"
#define temperature_topic "sensor/temperature"

#define ONE_WIRE_BUS 14 // which pin the ds1820b is on...
#define OLED_RESET 0  // GPIO0

//Setup wifi stack
WiFiClient espClient;
//Setup MQTT client
PubSubClient client(espClient);
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

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

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    // If you do not want to use a username and password, change next line to
    // if (client.connect("ESP8266Client")) {
    if (client.connect("ESP8266Client", mqtt_user, mqtt_password)) {
      Serial.println("connected");
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

  sensors.begin();        // Start the DS temp sensors
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  Serial.begin(115200);

  // Clear the buffer.
  display.clearDisplay();

  setup_wifi();
  client.setServer(mqtt_server, 1883);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  // call sensors.requestTemperatures() to issue a global temperature
  // request to all devices on the bus
  Serial.print("Requesting temperatures...");
  sensors.requestTemperatures(); // Send the command to get temperatures
  Serial.println("DONE");
  // After we got the temperatures, we can print them here.
  // We use the function ByIndex, and as an example get the temperature from the first sensor only.
  Serial.print("Temperature for the device 1 (index 0) is: ");
  Serial.println(sensors.getTempCByIndex(0),1);

  // Now lets disply the result on the OLED


  long now = millis();                          //  Only check the temp every 2 seconds
  if (now - lastMsg > 2000) {
    lastMsg = now;

    float newTemp = sensors.getTempCByIndex(0);

    if (checkBound(newTemp, temp, diff)) {      // check if the temp has changed
      temp = newTemp;
      Serial.print("New temperature:");
      Serial.println(String(temp).c_str());
      client.publish(temperature_topic, String(temp).c_str(), true);
      display.clearDisplay();
      display.setTextSize(3);
      display.setTextColor(WHITE);
      display.setCursor(10,3);
      display.print(temp,1);
      display.print((char)247); // degree symbol
      display.setTextSize(2);
      display.println("C");
      display.display();
      delay(1000);
   }
 }
}
