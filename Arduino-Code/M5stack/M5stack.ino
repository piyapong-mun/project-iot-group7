#include <M5Stack.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ML-group7_inferencing.h> // Replace with your library

// WiFi and MQTT settings
const char* ssid = "Redmi Note 13";
const char* password = "mypassword";
const char* mqtt_broker = "192.168.151.213";
const int mqtt_port = 1883;
const char* mqtt_topic_publish = "m5stack/prediction";
const char* mqtt_topic_subscribe = "nodered/message";
const char* matt_topic_publish_taskstate = "m5stack/taskstate";
const char* global_highest_label = "unknown";
bool change_trigger = false;

WiFiClient espClient;
PubSubClient client(espClient);

float accX, accY, accZ;
static bool debug_nn = false;

unsigned long interval_ms = 60 * 60 * 1000; // default 1hr
unsigned long task_duration_ms = 30 * 1000;  // default 30s
unsigned long last_task_time = 0;
unsigned long start_global = 0;
bool task_enabled = false;

void mqtt_callback(char* topic, byte* message, unsigned int length) {
  Serial.print("MQTT Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");

  String msg;
  for (int i = 0; i < length; i++) {
    msg += (char)message[i];
    Serial.print((char)message[i]);
  }
  Serial.println();
  // Parse JSON
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, msg);

  if (!error) {
    if (doc.containsKey("every")) {
      if (doc["every"] == 5) {
        interval_ms = doc["every"].as<unsigned long>() * 1000;
      }else {
        interval_ms = doc["every"].as<unsigned long>() * 60 * 1000;
      }
      Serial.printf("Updated timing: every %lu ms\n", interval_ms);
    }
    if (doc.containsKey("time")) {
      task_duration_ms = doc["time"].as<unsigned long>() * 1000;
      Serial.printf("Updated timing: duration %lu ms\n",task_duration_ms);
    }
    if (doc.containsKey("switch")) {
      task_enabled = doc["switch"];
      Serial.printf("Updated timing: switch = %d\n", task_enabled);
    }
    last_task_time = millis();
    change_trigger = true;      
  } else {
    Serial.println("Failed to parse JSON message");
  }
  
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect("M5StackClient")) {
      Serial.println("Connected to MQTT Broker!");
      client.subscribe(mqtt_topic_subscribe);
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  M5.begin();
  M5.IMU.Init();
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");

  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(mqtt_callback);

  if (EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME != 3) {
    Serial.println("EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME should be 3");
    return;
  }
}

void run_inference_and_publish() {
  float buffer[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE] = {0};

  for (size_t ix = 0; ix < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE; ix += 3) {
    uint64_t next_tick = micros() + (EI_CLASSIFIER_INTERVAL_MS * 1000);

    M5.IMU.getAccelData(&accX, &accY, &accZ);
    buffer[ix] = accX;
    buffer[ix + 1] = accY;
    buffer[ix + 2] = accZ;

    delayMicroseconds(next_tick - micros());
  }

  signal_t signal;
  int err = numpy::signal_from_buffer(buffer, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &signal);
  if (err != 0) {
    Serial.println("Failed to create signal from buffer");
    return;
  }

  ei_impulse_result_t result = {0};

  err = run_classifier(&signal, &result, debug_nn);
  if (err != EI_IMPULSE_OK) {
    Serial.println("Failed to run classifier");
    return;
  }

  float highest_value = 0;
  const char* highest_label = "unknown";

  Serial.println("Predictions:");
  for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
    Serial.printf("%s: %.5f\n", result.classification[ix].label, result.classification[ix].value);

    if (result.classification[ix].value > highest_value) {
      highest_value = result.classification[ix].value;
      highest_label = result.classification[ix].label;
      global_highest_label = result.classification[ix].label;
    }
  }

  char msg[100];
  // sprintf(msg, "{\"label\": \"%s\", \"confidence\": %.2f}", highest_label, highest_value * 100.0);
  sprintf(msg, "{\"label\": \"%s\", \"confidence\": %.2f,\"every\": %lu, \"time\": %lu, \"switch\": %d}",highest_label, highest_value * 100.0, interval_ms, task_duration_ms, task_enabled);
  client.publish(mqtt_topic_publish, msg);
  Serial.print("Sent MQTT message: ");
  Serial.println(msg);
}

void send_state(const char* str_in, bool show_progess, bool show_next_ex_time) {
  
  char state[100];
  if (show_progess) {
    unsigned long now = millis();
    unsigned long diff_time = task_duration_ms - (now - start_global);
    sprintf(state, "{\"state\": \"%s\", \"remain_pro\": %lu}", str_in, diff_time);
    client.publish(matt_topic_publish_taskstate, state);
    return;
  }
  if (show_next_ex_time) {
    unsigned long now = millis();
    unsigned long diff_time = interval_ms - (now - last_task_time);
    sprintf(state, "{\"state\": \"%s\", \"remain_next\": %lu}", str_in, diff_time);
    client.publish(matt_topic_publish_taskstate, state);
    return;
  }
  sprintf(state, "{\"state\": \"%s\"}", str_in);
  client.publish(matt_topic_publish_taskstate, state);
}

void loop() {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();
  unsigned long now = millis();
  change_trigger = false;
  if (!task_enabled) {
    run_inference_and_publish();
    send_state("off",false,false);
  }else 
  {
    
    if (now - last_task_time >= interval_ms) {
      Serial.println("Starting task cycle...");
      // unsigned long start = millis();
      start_global = millis();

      send_state("start",false,false);
      while (millis() - start_global < task_duration_ms) {
        client.loop();
        if (change_trigger) {
          break;
        }
        run_inference_and_publish();
        send_state("running",true,false);
        if (global_highest_label == "not-moving") {
          start_global = millis();
        }
        // delay(2000); // Optional delay between inferences
      }

      last_task_time = millis();
      send_state("stop",false,false);
      Serial.println("Task cycle complete.");
    }else
      {
        run_inference_and_publish();
        send_state("waiting",false,true);
      }
    }
}



