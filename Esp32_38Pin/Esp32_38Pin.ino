#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ESP32Servo.h>
#include <LiquidCrystal_I2C.h>


// ----------------------- CONFIG  -----------------------

// Config WiFi
const char* ssid        = "05 LuuDinhChat";
const char* password    = "0905272255";


// Config MQTT
const char* mqtt_server = "left01.local";
// const char* mqtt_server = "192.168.1.29";
const int mqtt_port     = 1883;

// MQTT Topics
const char* topic_relay_light  = "Relay/Lights";
const char* topic_relay_fans = "Relay/Fans";

const char* topic_dht_temp   = "DHT11/Temperature";
const char* topic_dht_hum    = "DHT11/Humidity";

const char* topic_soil_moisture_left = "Soil/Moisture_LEFT";
const char* topic_soil_moisture_right = "Soil/Moisture_RIGHT";

const char* topic_door = "Door";


// ---------------------- VARIABLEs  ----------------------

// GPIO pins
#define RELAY_LIGHT_PIN    33
#define RELAY_FAN_PIN      32

#define SERVO_PIN          13

#define DHTPIN             14
#define DHTTYPE            DHT11

// Config DHT11
DHT dht(DHTPIN, DHTTYPE);

// Config Servo
Servo myServo;

// set the LCD number of columns and rows
int lcdColumns = 16;
int lcdRows = 2;

// Config LCD I2C
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

// Timer for publish Water_Quan
unsigned long lastPublishTime = 0;
const unsigned long publishInterval = 3000; // 3s


// Display LCD
int soilMoistureLeft = 0;
int soilMoistureRight = 0;

// CHANGE Display LCD
unsigned long lastDisplayTime = 0;
const unsigned long displayInterval = 8000; // 8s
bool showTempHum = true;

// Config Wifi & MQTT
WiFiClient espClient;
PubSubClient client(espClient);


// ---------------------- FUNCTION  ----------------------

// Connecting to WIFI
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to WiFi: "); Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP address: "); Serial.println(WiFi.localIP());
}

// Connecting to MQTT
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Connect MQTT
    if (client.connect("ESP32_Client_38pin")) {
      Serial.println("connected");
      // Subscribe Topics
      client.subscribe(topic_relay_light);
      client.subscribe(topic_relay_fans);
      client.subscribe(topic_soil_moisture_left);
      client.subscribe(topic_soil_moisture_right);
      client.subscribe(topic_door);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// Callback Func
void callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  
  Serial.print("Message received on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  Serial.println(msg);

  // Process MSGs
  if (String(topic) == topic_relay_light) {
    if (msg == "ON") {
      digitalWrite(RELAY_LIGHT_PIN, HIGH);
      Serial.println("Lights turned ON");
    } else if (msg == "OFF") {
      digitalWrite(RELAY_LIGHT_PIN, LOW);
      Serial.println("Lights turned OFF");
    }
  } else if (String(topic) == topic_relay_fans) {
    if (msg == "ON") {
      digitalWrite(RELAY_FAN_PIN, HIGH);
      Serial.println("Fans turned ON");
    } else if (msg == "OFF") {
      digitalWrite(RELAY_FAN_PIN, LOW);
      Serial.println("Fans turned OFF");
    }
  } else if (String(topic) == topic_door) {
    if (msg == "OPEN") {
      myServo.write(25);
      Serial.println("Door is Opened");
    } else if (msg == "CLOSE") {
      myServo.write(105);
      Serial.println("Door is Closed");
    }
  } else if (String(topic) == topic_soil_moisture_left) {
    soilMoistureLeft = msg.toInt();
  } else if (String(topic) == topic_soil_moisture_right) {
    soilMoistureRight = msg.toInt();
  }
}


// ---------------------- MAIN  ----------------------

void setup() {
  // Setup Serial for DEBUG
  Serial.begin(115200);

  // set the ADC attenuation to 11 dB (up to ~3.3V input)
  analogSetAttenuation(ADC_11db);

  // Setup PINs
  pinMode(RELAY_LIGHT_PIN, OUTPUT);
  pinMode(RELAY_FAN_PIN, OUTPUT);

  // Turn OFF all Relays
  digitalWrite(RELAY_LIGHT_PIN, LOW);
  digitalWrite(RELAY_FAN_PIN, LOW);

  // Config DHT11
  dht.begin();

  // Config DHT11
  myServo.setPeriodHertz(50); // PWM frequency for SG90
  myServo.attach(SERVO_PIN, 500, 2400); // Minimum and maximum pulse width (in µs) to go from 0° to 180
  myServo.write(105);

  // Setup LCD
  lcd.init();
  lcd.backlight();  // Turn on the backlight

  // Connect WiFi
  setup_wifi();

  // Setup MQTT server
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}



void loop() {
  // Re-connect to MQTT
  if (!client.connected()) {
    reconnect();
  }
  client.loop(); // Main MQTT process loop

  // Read sensor
  int temperature = dht.readTemperature();
  int humidity = dht.readHumidity();

  // PUBLISH Water quantity Value
  unsigned long currentMillis = millis();

  if (currentMillis - lastPublishTime >= publishInterval) {
    lastPublishTime = currentMillis;
    client.publish(topic_dht_temp, String(temperature).c_str());
    client.publish(topic_dht_hum, String(humidity).c_str());
  }

  if (currentMillis - lastDisplayTime >= displayInterval) {
    lastDisplayTime = currentMillis;
    showTempHum = !showTempHum;
    lcd.clear();

    if (showTempHum) {
      lcd.setCursor(0, 0);
      lcd.print("Temp: ");
      lcd.print(temperature);
      lcd.print(" *C");
      lcd.setCursor(0, 1);
      lcd.print("Hum : ");
      lcd.print(humidity);
      lcd.print(" %");
    } else {
      lcd.setCursor(0, 0);
      lcd.print("Soil Left : ");
      lcd.print(soilMoistureLeft);
      lcd.print(" %");
      lcd.setCursor(0, 1);
      lcd.print("Soil Right: ");
      lcd.print(soilMoistureRight);
      lcd.print(" %");
    }
  }
}