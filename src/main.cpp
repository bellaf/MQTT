#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <Wire.h>
#include <PubSubClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define wifi_ssid "YOUR WIFI SSID"
#define wifi_password "WIFI PASSWORD"

#define mqtt_server "YOUR_MQTT_SERVER_HOST"
#define mqtt_user "your_username"
#define mqtt_password "your_password"

#define humidity_topic "sensor/humidity"
#define temperature_topic "sensor/temperature"

WiFiClient espClient;
PubSubClient client(espClient);

// Decalre Variables and Pins here....

long lastMsg = 0;
float temp = 0.0;
float hum = 0.0;
float diff = 1.0;

int DS1820PIN = 5;  //onewire pin for DS1820 Sensor

// Create a display object

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

void setup_wifi() {
  delay(10);

  // text display tests
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.print("Connecting to: ");
  display.println(wifi_ssid);
  display.display();
  delay(2000);
  display.clearDisplay();

  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
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

  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  // init done

  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  display.display();
  delay(2000);

  // Clear the buffer.
  display.clearDisplay();

  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);

  // Set SDA and SDL ports
  Wire.begin(2, 14);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

/*  long now = millis();
  if (now - lastMsg > 1000) {
    lastMsg = now;

    float newTemp = hdc.readTemperature();
//    float newHum = hdc.readHumidity();

    if (checkBound(newTemp, temp, diff)) {
      temp = newTemp;
      Serial.print("New temperature:");
      Serial.println(String(temp).c_str());
      client.publish(temperature_topic, String(temp).c_str(), true);
    }

    if (checkBound(newHum, hum, diff)) {
      hum = newHum;
      Serial.print("New humidity:");
      Serial.println(String(hum).c_str());
      client.publish(humidity_topic, String(hum).c_str(), true);
    }
  */

  }
