#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <MQTTClient.h>

const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

const char* BROKER = "af60653d7fe24f909c5d0d13b3cda2b7.s1.eu.hivemq.cloud";
const int BROKER_PORT = 8883;

const char* MQTT_USERNAME = "YOUR_MQTT_USERNAME";
const char* MQTT_PASSWORD = "YOUR_MQTT_PASSWORD";

// MQTT Topics
const char* LIGHT_TOPIC = "temp-k8m2x/iot/light";
const char* TRAFFIC_TOPIC = "temp-k8m2x/iot/traffic";
const char* TRAFFIC_SET_TOPIC = "temp-k8m2x/iot/traffic/set";

// Pin
#define RED_LED D5
#define YELLOW_LED D6
#define GREEN_LED D7
#define BUTTON D3
#define LDR A0

WiFiClientSecure client;
MQTTClient mqtt(128);

unsigned long lastPublish = 0;
const unsigned long PUBLISH_INTERVAL = 5000;

String currentLight = "RED";

bool lastButtonState = HIGH;
unsigned long lastButtonTime = 0;
const unsigned long debounceDelay = 200;


// -------------------- WiFi --------------------

void connectWiFi() {

  while (WiFi.status() != WL_CONNECTED) {

    Serial.println("Connecting WiFi...");

    WiFi.begin(ssid, password);

    delay(1000);
  }

  Serial.println("WiFi connected");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}


// -------------------- Traffic Light --------------------

void setTrafficLight(String color) {

  color.toUpperCase();

  digitalWrite(RED_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(GREEN_LED, LOW);

  if (color == "RED") {

    digitalWrite(RED_LED, HIGH);
    currentLight = "RED";

  } 
  else if (color == "YELLOW") {

    digitalWrite(YELLOW_LED, HIGH);
    currentLight = "YELLOW";

  } 
  else if (color == "GREEN") {

    digitalWrite(GREEN_LED, HIGH);
    currentLight = "GREEN";

  }

  Serial.print("Traffic Light: ");
  Serial.println(currentLight);

  mqtt.publish(
    TRAFFIC_TOPIC,
    currentLight,
    true,
    0
  );
}


// -------------------- MQTT Message --------------------

void messageReceived(String &topic, String &payload) {

  Serial.print("MQTT Topic: ");
  Serial.println(topic);

  Serial.print("Payload: ");
  Serial.println(payload);

  if (topic == TRAFFIC_SET_TOPIC) {

    if (
      payload == "RED" ||
      payload == "YELLOW" ||
      payload == "GREEN"
    ) {

      setTrafficLight(payload);

    }
  }
}


// -------------------- MQTT Connect --------------------

void connectMQTT() {

  while (!mqtt.connected()) {

    Serial.println("Connecting MQTT...");

    if (mqtt.connect(
      "esp_traffic",
      MQTT_USERNAME,
      MQTT_PASSWORD
    )) {

      Serial.println("MQTT Connected");

      mqtt.subscribe(TRAFFIC_SET_TOPIC);

      // ส่งสถานะปัจจุบันให้ Web
      mqtt.publish(
        TRAFFIC_TOPIC,
        currentLight,
        true,
        0
      );

    } 
    else {

      Serial.println("MQTT failed");

      delay(2000);
    }
  }
}


// -------------------- Button --------------------

void checkButton() {

  bool buttonState = digitalRead(BUTTON);

  if (
    buttonState == LOW &&
    lastButtonState == HIGH &&
    millis() - lastButtonTime > debounceDelay
  ) {

    lastButtonTime = millis();

    if (currentLight == "RED") {

      setTrafficLight("YELLOW");

    } 
    else if (currentLight == "YELLOW") {

      setTrafficLight("GREEN");

    } 
    else if (currentLight == "GREEN") {

      setTrafficLight("YELLOW");

    }

    else {

      setTrafficLight("RED");
    }
  }

  lastButtonState = buttonState;
}


// -------------------- Setup --------------------

void setup() {

  Serial.begin(9600);

  pinMode(RED_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);

  pinMode(BUTTON, INPUT_PULLUP);

  digitalWrite(RED_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(GREEN_LED, LOW);

  connectWiFi();

  client.setInsecure();

  mqtt.begin(
    BROKER,
    BROKER_PORT,
    client
  );

  mqtt.onMessage(messageReceived);

  connectMQTT();

  // เริ่มต้นที่ RED
  setTrafficLight("RED");
}


// -------------------- Loop --------------------

void loop() {

  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  if (!mqtt.connected()) {
    connectMQTT();
  }

  mqtt.loop();

  // Button
  checkButton();


  // ส่งค่า LDR ทุก 5 วินาที
  if (millis() - lastPublish >= PUBLISH_INTERVAL) {

    lastPublish = millis();

    int lightValue = analogRead(LDR);

    String data = String(lightValue);

    bool result = mqtt.publish(
      LIGHT_TOPIC,
      data,
      true,
      0
    );

    Serial.print("Light: ");
    Serial.print(data);
    Serial.print(" -> ");

    Serial.println(
      result ? "Publish OK" : "Publish FAIL"
    );
  }

  delay(10);
}