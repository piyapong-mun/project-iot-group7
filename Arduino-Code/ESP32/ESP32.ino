#include <WiFi.h>
#include <PubSubClient.h>

// WiFi credentials
const char* ssid = "Redmi Note 13";
const char* password = "mypassword";

// MQTT broker IP (your local broker)
const char* mqtt_server = "192.168.151.213";

// Buzzer pin
const int buzzerPin = 25;

WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  delay(10);
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");

  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  message.trim(); // Clean whitespace or newlines
  Serial.println(message);

  if (message == "on") {
    tone(buzzerPin, 1000); // Continuous 1kHz tone
  } else if (message == "off") {
    noTone(buzzerPin); // Stop the buzzer
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
      client.subscribe("home/buzzer"); // Topic you’re listening to
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(buzzerPin, OUTPUT);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
