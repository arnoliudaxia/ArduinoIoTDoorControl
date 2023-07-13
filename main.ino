#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Servo.h>
Servo myservo;
int angle1 = 180;
int angle2 = 80;

const int servoPin=D7;
const char* ssid = "*****";
const char* password = "******";
const char* mqtt_server = "********";
const char *mqtt_username = "wifiduino"; // username for authentication
const char *mqtt_password = "wifiduino"; // password for authentication

const char* heartBeatTopic = "lab/livereport";
const char* cmdTopic = "lab/cmd";

// init wifi client
WiFiClientSecure espClient;
PubSubClient client(espClient);
/*
  The common fingerprints of EMQX broker, for reference only.
  If you are not using EMQX Cloud Serverless or public EMQX broker,
  you need to calculate the sha1 fingerprint of your server certificate
  and update the 'fingerprint' variable below.
*/
// 1. fingerprint of public emqx broker. Host: broker.emqx.io
// const char* fingerprint = "B6 C6 FF 82 C6 59 09 BB D6 39 80 7F E7 BC 10 C9 19 C8 21 8E";
// 2. fingerprint of EMQX Cloud Serverless. Host: *.emqxsl.com
// const char* fingerprint = "42:AE:D8:A3:42:F1:C4:1F:CD:64:9C:D7:4B:A1:EE:5B:5E:D7:E2:B5";
// 3. fingerprint of EMQX Cloud Serverless. Host: *.emqxsl.cn
const char* fingerprint = "7E:52:D3:84:48:3C:5A:9F:A4:39:9A:8B:27:01:B1:F8:C6:AD:D4:47";

unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];

// 连接wifi过程
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

// 收到mqtt订阅回调
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println();
  // payload转成char数组
  char cmd[length + 1];
  cmd[length] = '\0';
  for (int i = 0; i < length; i++) {
    cmd[i] = (char)payload[i];
    Serial.print(cmd[i]);
  }
  // char数组再转成String
  String cmdstr(cmd);
  // 为了测试连通性，可以让收到1的时候灯跳一下
  if(cmdstr=="1")
  {
    digitalWrite(BUILTIN_LED, !digitalRead(BUILTIN_LED));
  }

  if (strcmp(topic, cmdTopic) == 0) {
    Serial.println("Receive command from commander!");
    client.publish(heartBeatTopic, "Receive Your Command!");

    if (cmdstr=="opendoor") {
      client.publish(heartBeatTopic, "Yes!Open The Door!");
      openDoor();
    }

    if (cmdstr=="isalive?") {
      client.publish(heartBeatTopic, "i am alive");
    
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

// 和MQTT服务器断连后自动重连
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    //有辨识度的clientId
    String clientId = "labWifiDuino";
    // Attempt to connect
    if (client.connect(clientId.c_str(),mqtt_username, mqtt_password)) {
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
  pinMode(servoPin, OUTPUT);  // Or sth like D3
  myservo.attach(servoPin);

  pinMode(BUILTIN_LED, OUTPUT);
  setup_wifi();

  espClient.setFingerprint(fingerprint);
  client.setServer(mqtt_server, 8883);
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
