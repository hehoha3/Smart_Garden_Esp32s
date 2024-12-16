#include <WiFi.h>
#include <PubSubClient.h>


// ----------------------- CONFIGs  -----------------------

// Config WiFi
const char* ssid        = "05 LuuDinhChat";
const char* password    = "0905272255";


// Config MQTT
const char* mqtt_server = "left01.local";
// const char* mqtt_server = "192.168.1.29";
const int mqtt_port     = 1883;

// MQTT Topics
const char* topic_relay_left  = "Relay/Pump_LEFT";
const char* topic_relay_right = "Relay/Pump_RIGHT";

const char* topic_soil_left   = "Soil/Moisture_LEFT";
const char* topic_soil_right  = "Soil/Moisture_RIGHT";

const char* topic_water_quan  = "Water/Quantity";


// ---------------------- VARIABLEs  ----------------------

// GPIO pins
#define RELAY1_PIN            26
#define RELAY2_PIN            27

#define SOIL_LEFT_PIN         35
#define SOIL_LEFT_POWER_PIN   17

#define SOIL_RIGHT_PIN        32
#define SOIL_RIGHT_POWER_PIN  16

#define WATER_SEN_SIGNAL_PIN  34
#define WATER_SEN_POWER_PIN   33

// MQTT Messages
uint16_t water_sensor_value = 0;
char msg_w_sen[5];

uint16_t soil1_sensor_value = 0;
char msg_s1_sen[5];

uint16_t soil2_sensor_value = 0;
char msg_s2_sen[5];

// Timer for publish Water_Quan
unsigned long lastPublishTime = 0;
const unsigned long publishInterval = 2000; // 2s

// Config Wifi & MQTT
WiFiClient espClient;
PubSubClient client(espClient);


// ---------------------- FUNCTIONs  ----------------------

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
    if (client.connect("ESP32_Client")) {
      Serial.println("connected");
      // Subscribe Topics
      client.subscribe(topic_relay_left);
      client.subscribe(topic_relay_right);
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
  if (String(topic) == topic_relay_left) {
    if (msg == "ON") {
      digitalWrite(RELAY1_PIN, HIGH);
      Serial.println("Pump LEFT turned ON");
    } else if (msg == "OFF") {
      digitalWrite(RELAY1_PIN, LOW);
      Serial.println("Pump LEFT turned OFF");
    }
  } else if (String(topic) == topic_relay_right) {
    if (msg == "ON") {
      digitalWrite(RELAY2_PIN, HIGH);
      Serial.println("Pump RIGHT turned ON");
    } else if (msg == "OFF") {
      digitalWrite(RELAY2_PIN, LOW);
      Serial.println("Pump RIGHT turned OFF");
    }
  }
}


// ---------------------- MAIN  ----------------------

void setup() {
  // Setup Serial for DEBUG
  Serial.begin(115200);

  // set the ADC attenuation to 11 dB (up to ~3.3V input)
  analogSetAttenuation(ADC_11db);

  // Setup PINs
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(WATER_SEN_POWER_PIN, OUTPUT);
  pinMode(SOIL_LEFT_POWER_PIN, OUTPUT);
  pinMode(SOIL_RIGHT_POWER_PIN, OUTPUT);

  // Turn OFF all Relays
  digitalWrite(RELAY1_PIN, LOW);
  digitalWrite(RELAY2_PIN, LOW);
  digitalWrite(WATER_SEN_POWER_PIN, LOW);
  digitalWrite(SOIL_LEFT_POWER_PIN, LOW);
  digitalWrite(SOIL_RIGHT_POWER_PIN, LOW);

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

  digitalWrite(WATER_SEN_POWER_PIN, HIGH);
  digitalWrite(SOIL_LEFT_POWER_PIN, HIGH);
  digitalWrite(SOIL_RIGHT_POWER_PIN, HIGH);
  delay(10);

  // Read sensors
  water_sensor_value = analogRead(WATER_SEN_SIGNAL_PIN);
  water_sensor_value = map(water_sensor_value, 0, 4095, 0, 100);
  water_sensor_value = water_sensor_value - 50;

  soil1_sensor_value = analogRead(SOIL_LEFT_PIN);
  soil1_sensor_value = (100 - ((soil1_sensor_value/4095.00) * 100));

  soil2_sensor_value = analogRead(SOIL_RIGHT_PIN);
  soil2_sensor_value = (100 - ((soil2_sensor_value/4095.00) * 100));

  // PUBLISH Water quantity Value
  unsigned long currentMillis = millis();
  if (currentMillis - lastPublishTime >= publishInterval) {
    lastPublishTime = currentMillis;
    
    dtostrf(water_sensor_value, 4, 0, msg_w_sen);
    dtostrf(soil1_sensor_value, 4, 0, msg_s1_sen);
    dtostrf(soil2_sensor_value, 4, 0, msg_s2_sen);
    
    // Publish to MQTT topic
    client.publish(topic_water_quan, msg_w_sen);
    client.publish(topic_soil_left, msg_s1_sen);
    client.publish(topic_soil_right, msg_s2_sen);
    
    // DEBUG
    Serial.print(water_sensor_value);
    Serial.println("%");
  }
}