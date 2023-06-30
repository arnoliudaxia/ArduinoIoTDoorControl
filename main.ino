#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Servo.h>
Servo myservo;
int angle1 = 180;
int angle2 = 80;

const char* ssid = "embedLab";
const char* password = "bugaosuni";
const char* mqtt_server = "10.15.89.95";

const char* heartBeatTopic = "lab/livereport";
const char* cmdTopic = "lab/cmd";
const char* openDoorCmd = "opendoor";


WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

//控制舵机开门
void openDoor() {
  myservo.write(angle1);
  delay(500);
  myservo.write(angle2);
  delay(2000);
  myservo.write(angle1);
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  char cmd[length + 1];
  cmd[length] = '\0';
  for (int i = 0; i < length; i++) {
    cmd[i] = (char)payload[i];
    Serial.print(cmd[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

  if (strcmp(topic, cmdTopic) == 0) {
    Serial.println("Receive command from commander!");
    client.publish(heartBeatTopic, "Receive Your Command!");

    if (strcmp(cmd, openDoorCmd) == 0) {
      client.publish(heartBeatTopic, "Yes!Open The Door!");
      openDoor();
    }

    char* colonPtr = strchr(cmd, ':');
    if (colonPtr != NULL) {
      client.publish(heartBeatTopic, "You are going to set parameters.");
      // 提取冒号后的字符串（即整数部分）
      char* intPart = colonPtr + 1;
      int value = atoi(intPart);
      if (cmd[0] == 'a' && cmd[1] == '1') {
        Serial.println("set angle1");
        angle1 = value;
      }
      if (cmd[0] == 'a' && cmd[1] == '2') {
        Serial.println("set angle2");
        angle2 = value;
      }
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    //有辨识度的clientId
    String clientId = "labWifiDuino";
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.publish(heartBeatTopic, "Online");
      // ... and resubscribe
      client.subscribe(cmdTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  // 初始化串口
  Serial.begin(115200);

  //舵机初始化
  pinMode(D7, OUTPUT);  // Or sth like D3
  myservo.attach(D7);

  pinMode(BUILTIN_LED, OUTPUT);
  setup_wifi();

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

//上报我活着
void sigrip() {
  snprintf(msg, MSG_BUFFER_SIZE, "Live");
  Serial.print("Publish message: ");
  Serial.println(msg);
  client.publish(heartBeatTopic, msg);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  ///每30s报告一次我还活着
  if (now - lastMsg > 30000) {
    lastMsg = now;
    sigrip();
  }
}
