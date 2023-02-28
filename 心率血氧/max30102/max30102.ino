/*****************************************************************
* Pulse rate and SPO2 meter using the MAX30102
* https://github.com/har-in-air/ESP8266_MAX30102_SPO2_PULSE_METER
* 
* 
* 
* This is a mashup of 
* 1. sensor initialization and readout code from Sparkfun 
* https://github.com/sparkfun/SparkFun_MAX3010x_Sensor_Library
*  
*  2. spo2 & pulse rate analysis from 
* https://github.com/aromring/MAX30102_by_RF  
* (algorithm by  Robert Fraczkiewicz)
* I tweaked this to use 50Hz sample rate
* 
* 3. ESP8266 AP & Webserver code from Random Nerd tutorials
* https://randomnerdtutorials.com/esp8266-nodemcu-access-point-ap-web-server/
******************************************************************/

#include <Arduino.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include "algorithm_by_RF.h"
#include "MAX30105.h"
#include "heartRate.h"
#include "SSD1306Wire.h"        //(4针)0.96寸I2C通讯128x64OLED液晶屏模块1315驱动
#include "PubSubClient.h"               //MQTT库文件

#define SDA 1
#define SCL 3
//***********MQTT配置**************//

const char* ssid = "501-2.4G";  // Enter SSID here
const char* password = "123456a.";  //Enter Password here
const char*  topic1 = "HeartBeat";             //主题
const char*  topic2 = "Spo2";
const char* mqtt_server = "bemfa.com";   //默认，MQTT服务器
const int mqtt_server_port = 9501;       //默认，MQTT服务器端口
#define ID_MQTT  "2205b187c6079ed1deda0d2b14ae2716"     //Client ID
long timeval = 500;                  //上传的传感器时间间隔
long lastMsg = 0;//时间戳
WiFiClient espClient;
PubSubClient client(espClient);//mqtt初始化
//************************************//

SSD1306Wire display(0x3c, SDA, SCL);   // ADDRESS, SDA, SCL  -  SDA and SCL usually populate automatically based on your board's pins_arduino
int arr[128] = {0}; //显示数组

// uncomment for test : measuring actual sample rate, or to display waveform on a serial plotter
//#define MODE_DEBUG  

MAX30105 sensor;


#ifdef MODE_DEBUG
uint32_t startTime;
#endif

uint32_t  aun_ir_buffer[RFA_BUFFER_SIZE]; //infrared LED sensor data
uint32_t  aun_red_buffer[RFA_BUFFER_SIZE];  //red LED sensor data
int32_t   n_heart_rate; 
float     n_spo2;
int       numSamples;
float temperature;
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
String processor(const String& var){
    if(var == "SPO2"){
      return n_spo2 > 0 ? String(n_spo2) : String("00.00");
      }
    else if(var == "HEARTRATE"){
      return n_heart_rate > 0 ? String(n_heart_rate) : String("00");
      }
    return String();
    }


void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("SPO2/Pulse meter");

 // Initialising the UI will init the display too.
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);

// WIFI&MQTT init
  setup_wifi();
  client.setServer(mqtt_server, mqtt_server_port);
  client.setCallback(callback);
  // ESP8266 tx power output 20.5dBm by default
  // we can lower this to reduce power supply noise caused by tx bursts
  String ipstr = WiFi.localIP().toString().c_str();
  display.drawString(10, 10, ipstr);
  display.display();
  delay(3000);

  pinMode(LED_BUILTIN, OUTPUT);
  
  if (sensor.begin(Wire, I2C_SPEED_FAST) == false) {
    Serial.println("Error: MAX30102 not found, try cycling power to the board...");
    // indicate fault by blinking the board LED rapidly
    while (1){
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
      delay(100);
      }
    }
    // ref Maxim AN6409, average dc value of signal should be within 0.25 to 0.75 18-bit range (max value = 262143)
    // You should test this as per the app note depending on application : finger, forehead, earlobe etc. It even
    // depends on skin tone.
    // I found that the optimum combination for my index finger was :
    // ledBrightness=30 and adcRange=2048, to get max dynamic range in the waveform, and a dc level > 100000
  byte ledBrightness = 60; // 0 = off,  255 = 50mA
  byte sampleAverage = 4; // 1, 2, 4, 8, 16, 32
  byte ledMode = 2; // 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green (MAX30105 only)
  int sampleRate = 200; // 50, 100, 200, 400, 800, 1000, 1600, 3200
  int pulseWidth = 411; // 69, 118, 215, 411
  int adcRange = 4096; // 2048, 4096, 8192, 16384
  
  sensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange); 
  sensor.getINT1(); // clear the status registers by reading
  sensor.getINT2();
  numSamples = 0;

#ifdef MODE_DEBUG
  startTime = millis();
#endif
  }


#ifdef MODE_DEBUG
void loop(){
  sensor.check(); 

  while (sensor.available())   {
    numSamples++;
#if 0 
    // measure the sample rate FS  (in Hz) to be used by the RF algorithm
    //Serial.print("R[");
    //Serial.print(sensor.getFIFORed());
    //Serial.print("] IR[");
    //Serial.print(sensor.getFIFOIR());
    //Serial.print("] ");
    Serial.print((float)numSamples / ((millis() - startTime) / 1000.0), 2);
    Serial.println(" Hz");
#else 
    // display waveform on Arduino Serial Plotter window
    Serial.print(sensor.getFIFORed());
    Serial.print(" ");
    Serial.println(sensor.getFIFOIR());
#endif
    
    sensor.nextSample();
  }
}

#else // normal spo2 & heart-rate measure mode

void loop() {
  if (!client.connected()) {//判断mqtt是否连接
    reconnect();
  }
  client.loop();//mqtt客户端
  float ratio,correl; 
  int8_t  ch_spo2_valid;  
  int8_t  ch_hr_valid;  


  sensor.check();
  while (sensor.available())   {
      aun_red_buffer[numSamples] = sensor.getFIFORed(); 
      aun_ir_buffer[numSamples] = sensor.getFIFOIR();
      numSamples++;
      sensor.nextSample(); 


      if (numSamples == RFA_BUFFER_SIZE) {
        // calculate heart rate and SpO2 after RFA_BUFFER_SIZE samples (ST seconds of samples) using Robert's method
        rf_heart_rate_and_oxygen_saturation(aun_red_buffer, RFA_BUFFER_SIZE,aun_ir_buffer , &n_spo2, &ch_spo2_valid, &n_heart_rate, &ch_hr_valid, &ratio, &correl);  //这个巨坑！！没有之一，red_buffer和ir_buffer得交换位置读数才显得正常，否则乱跳数字大部分为-999，但所有例程都没有改，有没人知道为什么
        Serial.printf("SP02 ");
        if (ch_spo2_valid) Serial.print(n_spo2); else Serial.print("x");
        Serial.print(", Pulse ");
        if (ch_hr_valid) Serial.print(n_heart_rate); else Serial.print("x");

        numSamples = 0;
        // toggle the board LED. This should happen every ST (= 4) seconds if MAX30102 has been configured correctly
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        temperature = sensor.readTemperature();
        Serial.print(", temp=");
        Serial.print(temperature, 4);Serial.print("°C");
        Serial.println();
        }
        //-------------oled---------------end
        // clear the display
        display.clear();
        // draw the current demo method
        display.setFont(ArialMT_Plain_16);
        display.setTextAlignment(TEXT_ALIGN_LEFT);
        display.drawString(0, 45, "H:" + String(n_heart_rate));
        display.drawString(60, 45, "S:" + String(n_spo2));
        display.setFont(ArialMT_Plain_10);
        display.drawString(0, 0, "T:" + String(temperature));

        //-------------oled---------------end

    }
        if (checkForBeat(aun_red_buffer[numSamples-1]) == true and aun_red_buffer[numSamples-1] > 20000){arr[127] =15;}else{arr[127] =40;}
        for(int i=0;i<127;i++){
          display.drawLine(i, arr[i], i+1, arr[i+1]);
          arr[i] = arr[i+1];
          }
        // write the buffer to the display oled
        display.display();

    long now = millis();//获取当前时间戳
    if (now - lastMsg > timeval) {//如果达到设定时间，进行数据上传
      lastMsg = now;
      // read without samples.
      String  HeartStr = String(n_heart_rate);
      String  SPOStr = String(n_spo2);
      client.publish(topic1, HeartStr.c_str());//心率数据上传
      client.publish(topic2, SPOStr.c_str());//血氧数据上传
    }
  }
  
#endif
