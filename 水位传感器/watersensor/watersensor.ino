#include <ESP8266WiFi.h>                //默认，加载WIFI头文件
#include "PubSubClient.h"               //默认，加载MQTT库文件

//***********WIFI**************//
const char* ssid = "501-2.4G";  // Enter SSID here
const char* password = "123456a.";  //Enter Password here
//************************************//
//MQTT
const char*  topic1 = "water";             //主题名字
#define ID_MQTT  "2205b187c6079ed1deda0d2b14ae2716"     //Client ID
long timeval = 1000;                  //上传的传感器时间间隔，默认1秒
const char* mqtt_server = "bemfa.com";   //MQTT服务器地址
const int mqtt_server_port = 9501;       //MQTT服务器端口
long lastMsg = 0;//时间戳
WiFiClient espClient;
PubSubClient client(espClient);//mqtt初始化
//*******************

int analogPin = A0; //水位传感器连接到模拟口1
int led = 0; //食人鱼灯连接到数字口12
int val = 0; //定义变量val 初值为0
int data = 0; //定义变量data 初值为0

void setup() {
  Serial.begin(115200); //设定波特率为9600
  pinMode(led, OUTPUT); //定义led 为输出引脚
  setup_wifi();
  client.setServer(mqtt_server,mqtt_server_port);
  if(client.connect(ID_MQTT)){
    client.subscribe("water");
    client.subscribe("lockSwitch");
    client.setCallback(MsgCallback);
  }else{
    Serial.println("connection failed");
  }
}

void loop() {
  if(!client.connected()){
    reconnect();
  }
  client.loop();
  val = analogRead(analogPin); //读取模拟值送给变量val
  if(val>500){ //判断变量val 是否大于700
  digitalWrite(led,HIGH); //变量val 大于700 时，点亮食人鱼灯
  }
  else{
  digitalWrite(led,LOW); //变量val 小于700 时，熄灭食人鱼灯
  }
  data = val; //变量val 赋值给变量data
  long now = millis();//获取当前时间戳
  if (now - lastMsg > timeval) {//如果达到3s，进行数据上传
    lastMsg = now;
    String  msg = String(data);
    client.publish(topic1, msg.c_str());//数据上传
  }
  delay(100);
}
//wifi初始化
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
//消息接收
void MsgCallback(char* topic, byte* payload, unsigned int length) {
  // Serial.print(topic);
  // Serial.print(":");
  String msg = "";
  for(int i=0;i<length;i++){
    msg += (char)payload[i];
  }
  msg += topic;
  if(msg == "onlockSwitch"){
    Serial.println("lockon");
  }
  msg = "";
}
//重新连接
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