#include <ESP8266WiFi.h>                //默认，加载WIFI头文件
#include "PubSubClient.h"               //默认，加载MQTT库文件
#include <SimpleDHT.h>                  //dht11库文件
//***********需要修改的地方**************//

const char* ssid = "501-2.4G";  // Enter SSID here
const char* password = "123456a.";  //Enter Password here
const char*  topic1 = "temp";             //主题名字，可
#define ID_MQTT  "2205b187c6079ed1deda0d2b14ae2716"     //Client ID
int pinDHT11 = 2;                         //dht11传感器引脚输入
long timeval = 3 * 1000;                  //上传的传感器时间间隔，默认3秒
//************************************//

const char* mqtt_server = "bemfa.com";   //默认，MQTT服务器
const int mqtt_server_port = 9501;       //默认，MQTT服务器

long lastMsg = 0;//时间戳
SimpleDHT11 dht11(pinDHT11);//dht11初始化
WiFiClient espClient;
PubSubClient client(espClient);//mqtt初始化

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);  //连接wifi

  while (WiFi.status() != WL_CONNECTED) { //等待wifi连接
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Topic:");//topic消息接收
  Serial.println(topic);
  String msg = "";
  for (int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  Serial.print("Msg:");
  Serial.println(msg);
  msg = "";
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(ID_MQTT)) {
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


void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_server_port);
  client.setCallback(callback);
}
void loop() {
    if (!client.connected()) {//判断mqtt是否连接
    reconnect();
  }
  client.loop();//mqtt客户端


  long now = millis();//获取当前时间戳
  if (now - lastMsg > timeval) {//如果达到3s，进行数据上传
    lastMsg = now;
    // read without samples.
    byte temperature = 0;
    byte humidity = 0;
    int err = SimpleDHTErrSuccess;
    if ((err = dht11.read(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess) {
      Serial.print("Read DHT11 failed, err="); Serial.println(err); delay(1000);
      return;
    }
    String  msg = "#" + (String)temperature + "#" + (String)humidity + "#"; //数据封装#温度#湿度#开关状态#
    client.publish(topic1, msg.c_str());//数据上传
}
}
