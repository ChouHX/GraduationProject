#include <ESP8266WiFi.h>                //默认，加载WIFI头文件
#include "PubSubClient.h"               //默认，加载MQTT库文件
#include "stdio.h"                //这个头文件是用来使用sprint函数的
#include "SoftwareSerial.h"       //注意添加这个软串口头文件
#include <Servo.h>
char ssid[] = "501-2.4G";   //WiFi账号
char pswd[] = "123456a.";   //WiFi密码
//-------------巴法云配置--------------
const char*  topic = "lock";             //主题名字
#define ID_MQTT  "2205b187c6079ed1deda0d2b14ae2716"     //Client ID
const char* mqtt_server = "bemfa.com";   //默认，MQTT服务器
const int mqtt_server_port = 9501;       //默认，MQTT服务器
WiFiClient espClient;
PubSubClient pubclient(espClient);
unsigned long lastMsg = 0;              //计时
boolean LockStatus = false; //门锁状态
const char*  Lockmsg = "off";
//---------------------------------
Servo myservo;
SoftwareSerial mySerial(4,5);    //软串口引脚，RX：GPIO4    TX：GPIO5  Touch: GPIO14

char str[20];    //用于sprint函数的临时数组
int SearchID,EnrollID;    //搜索指纹的ID号和注册指纹的ID号
uint16_t ScanState = 0,WiFi_Connected_State = 0,ErrorNum = 0,PageID = 0;   //状态标志变量；WiFi是否连接状态标志位；扫描指纹错误次数标志位；输入ID号变量
uint8_t PS_ReceiveBuffer[20];   //串口接收数据的临时缓冲数组


//休眠协议
uint8_t PS_SleepBuffer[12] = {0xEF,0x01,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x03,0x33,0x00,0x37};
//清空指纹协议
uint8_t PS_EmptyBuffer[12] = {0xEF,0x01,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x03,0x0D,0x00,0x11};
//获取图像协议
uint8_t PS_GetImageBuffer[12] = {0xEF,0x01,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x03,0x01,0x00,0x05};
//取消命令协议
uint8_t PS_CancelBuffer[12] = {0xEF,0x01,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x03,0x30,0x00,0x34};
//生成模块协议
uint8_t PS_GetChar1Buffer[13] = {0xEF,0x01,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x04,0x02,0x01,0x00,0x08};
uint8_t PS_GetChar2Buffer[13] = {0xEF,0x01,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x04,0x02,0x02,0x00,0x09};
//RGB颜色控制协议
uint8_t PS_BlueLEDBuffer[16] = {0xEF,0x01,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x07,0x3C,0x03,0x01,0x01,0x00,0x00,0x49};
uint8_t PS_RedLEDBuffer[16] = {0xEF,0x01,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x07,0x3C,0x02,0x04,0x04,0x02,0x00,0x50};
uint8_t PS_GreenLEDBuffer[16] = {0xEF,0x01,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x07,0x3C,0x02,0x02,0x02,0x02,0x00,0x4C};
//搜索指纹协议
uint8_t PS_SearchMBBuffer[17] = {0xEF,0x01,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x08,0x04,0x01,0x00,0x00,0xFF,0xFF,0x02,0x0C};
//自动注册指纹协议
uint8_t PS_AutoEnrollBuffer[17] = {0xEF,0x01,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x08,0x31,'\0','\0',0x04,0x00,0x16,'\0','\0'}; //PageID: bit 10:11，SUM: bit 15:16
//删除指纹协议
uint8_t PS_DeleteBuffer[16] = {0xEF,0x01,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x07,0x0C,'\0','\0',0x00,0x01,'\0','\0'}; //PageID: bit 10:11，SUM: bit 14:15
//----------------MQTT------------------------
//消息接收
void MsgCallback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for(int i=0;i<length;i++){
    msg += (char)payload[i];
  }
  if (msg == "on") {
    LockStatus = true;
    pubclient.publish("switchStatus","开");
  }else if (msg == "off") {
    LockStatus = false;
    pubclient.publish("switchStatus","关");
  }else if (msg == "register") {
    //单次注册
    // ScanState |= 1<<2;
    ScanState = 0x16;
    PageID = PageID +1;
    pubclient.publish("switchStatus","请重复按下手指直至绿灯");
  }else if (msg == "clear") {
    //清空指纹
    PageID = 0;
    if(PS_Empty() == 0x00)
    {
      PS_ControlLED(PS_GreenLEDBuffer);
    }
    else
    {
      PS_ControlLED(PS_RedLEDBuffer);
    }
    pubclient.publish("switchStatus","指纹已清空");
  }else if(msg == "cancel"){
    PS_Cancel();
    pubclient.publish("switchStatus","取消注册");
  }
  // switch (choice)
  // {
  // case 0: //关
  //   LockStatus = false;
  //   break;
  // case 1: //开
  //   LockStatus = true;
  //   break;
  // case 2: //复位
  //   PS_Cancel();
  //   delay(500);
  //   PS_Sleep();
  //   attachInterrupt(digitalPinToInterrupt(14),InterruptFun,RISING);
  //   break;
  // case 3: //连续注册
  //   ScanState |= 0x01;
  //   break;
  // case 4: //清空指纹
  //   PageID = 0;
  //   if(PS_Empty() == 0x00)
  //   {
  //     PS_ControlLED(PS_GreenLEDBuffer);
  //   }
  //   else
  //   {
  //     PS_ControlLED(PS_RedLEDBuffer);
  //   }
  //   break;
  // case 5: //单次注册
  //   ScanState |= 1<<2;
  //   break;
  // default:
  //   break;
  // }
}
//wifi初始化
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, pswd);  //连接wifi

  while (WiFi.status() != WL_CONNECTED) { //等待wifi连接
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}
void reconnect() {
  // Loop until we're reconnected
  while (!pubclient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (pubclient.connect(ID_MQTT)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(pubclient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
//----------------------------------------

/**
  * @file    FPM383C.cpp
  * @brief   用在中断里面的延时函数
  * @param   ms：需要延时的毫秒数
  */
void delay_ms(long int ms)
{
  for(int i=0;i<ms;i++)
  {
    delayMicroseconds(1000);
  }
}


/**
  * @file    FPM383C.cpp
  * @brief   串口发送函数
  * @param   len: 发送数组长度
  * @param   PS_Databuffer[]: 需要发送的功能协议数组，在上面已有定义
  */
void FPM383C_SendData(int len,uint8_t PS_Databuffer[])
{
  mySerial.write(PS_Databuffer,len);
  mySerial.flush();
}


/**
  * @file    FPM383C.cpp
  * @brief   串口接收函数
  * @param   Timeout：接收超时时间
  */
void FPM383C_ReceiveData(uint16_t Timeout)
{
  uint8_t i = 0;
  while(mySerial.available() == 0 && (--Timeout))
  {
    delay(1);
  }
  while(mySerial.available() > 0)
  {
    delay(2);
    PS_ReceiveBuffer[i++] = mySerial.read();
    if(i > 15) break; 
  }
}


/**
  * @file    FPM383C.cpp
  * @brief   休眠函数，只有发送休眠后，模块的TOUCHOUT引脚才会变成低电平
  * @param   None
  * @return  None
  */
void PS_Sleep()
{
  FPM383C_SendData(12,PS_SleepBuffer);
}


/**
  * @file    FPM383C.cpp
  * @brief   模块LED灯控制函数
  * @param   PS_ControlLEDBuffer[]：需要设置颜色的协议，一般定义在上面
  */
void PS_ControlLED(uint8_t PS_ControlLEDBuffer[])
{
  FPM383C_SendData(16,PS_ControlLEDBuffer);
}


/**
  * @file    FPM383C.cpp
  * @brief   模块任务取消操作函数，如发送了注册指纹命令，但是不想注册了，需要发送此函数
  * @param   None
  * @return  应答包第9位确认码或者无效值0xFF
  */
uint8_t PS_Cancel()
{
  FPM383C_SendData(12,PS_CancelBuffer);
  FPM383C_ReceiveData(2000);
  return PS_ReceiveBuffer[6] == 0x07 ? PS_ReceiveBuffer[9] : 0xFF;
}


/**
  * @file    FPM383C.cpp
  * @brief   模块获取搜索指纹用的图像函数
  * @param   None
  * @return  应答包第9位确认码或者无效值0xFF
  */
uint8_t PS_GetImage()
{
  FPM383C_SendData(12,PS_GetImageBuffer);
  FPM383C_ReceiveData(2000);
  return PS_ReceiveBuffer[6] == 0x07 ? PS_ReceiveBuffer[9] : 0xFF;
}


/**
  * @file    FPM383C.cpp
  * @brief   模块获取图像后生成特征，存储到缓冲区1
  * @param   None
  * @return  应答包第9位确认码或者无效值0xFF
  */
uint8_t PS_GetChar1()
{
  FPM383C_SendData(13,PS_GetChar1Buffer);
  FPM383C_ReceiveData(2000);
  return PS_ReceiveBuffer[6] == 0x07 ? PS_ReceiveBuffer[9] : 0xFF;
}


/**
  * @file    FPM383C.cpp
  * @brief   生成特征，存储到缓冲区2
  * @param   None
  * @return  应答包第9位确认码或者无效值0xFF
  */
uint8_t PS_GetChar2()
{
  FPM383C_SendData(13,PS_GetChar2Buffer);
  FPM383C_ReceiveData(2000);
  return PS_ReceiveBuffer[6] == 0x07 ? PS_ReceiveBuffer[9] : 0xFF;
}


/**
  * @file    FPM383C.cpp
  * @brief   搜索指纹模板函数
  * @param   None
  * @return  应答包第9位确认码或者无效值0xFF
  */
uint8_t PS_SearchMB()
{
  FPM383C_SendData(17,PS_SearchMBBuffer);
  FPM383C_ReceiveData(2000);
  return PS_ReceiveBuffer[6] == 0x07 ? PS_ReceiveBuffer[9] : 0xFF;
}


/**
  * @file    FPM383C.cpp
  * @brief   清空指纹模板函数
  * @param   None
  * @return  应答包第9位确认码或者无效值0xFF
  */
uint8_t PS_Empty()
{
  FPM383C_SendData(12,PS_EmptyBuffer);
  FPM383C_ReceiveData(2000);
  return PS_ReceiveBuffer[6] == 0x07 ? PS_ReceiveBuffer[9] : 0xFF;
}


/**
  * @file    FPM383C.cpp
  * @brief   自动注册指纹模板函数
  * @param   PageID：注册指纹的ID号，取值0 - 59
  * @return  应答包第9位确认码或者无效值0xFF
  */
uint8_t PS_AutoEnroll(uint16_t PageID)
{
  PS_AutoEnrollBuffer[10] = (PageID>>8);
  PS_AutoEnrollBuffer[11] = (PageID);
  PS_AutoEnrollBuffer[15] = (0x54+PS_AutoEnrollBuffer[10]+PS_AutoEnrollBuffer[11])>>8;
  PS_AutoEnrollBuffer[16] = (0x54+PS_AutoEnrollBuffer[10]+PS_AutoEnrollBuffer[11]);
  FPM383C_SendData(17,PS_AutoEnrollBuffer);
  FPM383C_ReceiveData(10000);
  return PS_ReceiveBuffer[6] == 0x07 ? PS_ReceiveBuffer[9] : 0xFF;
}
/**
  * @file    FPM383C.cpp
  * @brief   删除指定指纹模板函数
  * @param   PageID：需要删除的指纹ID号，取值0 - 59
  * @return  应答包第9位确认码或者无效值0xFF
  */
uint8_t PS_Delete(uint16_t PageID)
{
  PS_DeleteBuffer[10] = (PageID>>8);
  PS_DeleteBuffer[11] = (PageID);
  PS_DeleteBuffer[14] = (0x15+PS_DeleteBuffer[10]+PS_DeleteBuffer[11])>>8;
  PS_DeleteBuffer[15] = (0x15+PS_DeleteBuffer[10]+PS_DeleteBuffer[11]);
  FPM383C_SendData(16,PS_DeleteBuffer);
  FPM383C_ReceiveData(2000);
  return PS_ReceiveBuffer[6] == 0x07 ? PS_ReceiveBuffer[9] : 0xFF;
}
/**
  * @file    FPM383C.cpp
  * @brief   二次封装自动注册指纹函数，实现注册成功闪烁两次绿灯，失败闪烁两次红灯
  * @param   PageID：注册指纹的ID号，取值0 - 59
  * @return  应答包第9位确认码或者无效值0xFF
  */
uint8_t PS_Enroll(uint16_t PageID)
{
  if(PS_AutoEnroll(PageID) == 0x00)
  {
    PS_ControlLED(PS_GreenLEDBuffer);
    return PS_ReceiveBuffer[9];
  }
  PS_ControlLED(PS_RedLEDBuffer);
  return 0xFF;
}


/**
  * @file    FPM383C.cpp
  * @brief   分步式命令搜索指纹函数
  * @return  应答包第9位确认码或者无效值0xFF
  */
uint8_t PS_Identify()
{
  if(PS_GetImage() == 0x00)
  {
    if(PS_GetChar1() == 0x00)
    {
      if(PS_SearchMB() == 0x00)
      {
        if(PS_ReceiveBuffer[8] == 0x07 && PS_ReceiveBuffer[9] == 0x00)
        {
          PS_ControlLED(PS_GreenLEDBuffer);
          return PS_ReceiveBuffer[9];
        }
      }
    }
  }
  ErrorNum++;
  PS_ControlLED(PS_RedLEDBuffer);
  return 0xFF;
}


/**
  * @file    FPM383C.cpp
  * @brief   搜索指纹后的应答包校验，在此执行相应的功能，如开关继电器、开关灯等等功能
  * @param   ACK：各个功能函数返回的应答包
  */
void SEARCH_ACK_CHECK(uint8_t ACK)
{
	if(PS_ReceiveBuffer[6] == 0x07)
	{
		switch (ACK)
		{
			case 0x00:                          //指令正确
        SearchID = (int)((PS_ReceiveBuffer[10] << 8) + PS_ReceiveBuffer[11]);
        sprintf(str,"Now Search ID: %d",(int)SearchID);
        Serial.println(str);
        if(SearchID == 0) WiFi_Connected_State = 0;
        LockStatus = !LockStatus;
        digitalWrite(12,!digitalRead(12));
        if(ErrorNum < 5) ErrorNum = 0;
				break;
		}
	}
  for(int i=0;i<20;i++) PS_ReceiveBuffer[i] = 0xFF;
}


/**
  * @file    FPM383C.cpp
  * @brief   注册指纹后返回的应答包校验
  * @param   ACK：注册指纹函数返回的应答包
  */
void ENROLL_ACK_CHECK(uint8_t ACK)
{
	if(PS_ReceiveBuffer[6] == 0x07)
	{
		switch (ACK)
		{
			case 0x00:                          //指令正确
        EnrollID = (int)((PS_AutoEnrollBuffer[10] << 8) + PS_AutoEnrollBuffer[11]);
        sprintf(str,"Now Enroll ID: %d",(int)EnrollID);
        Serial.println(str);
				break;
		}
	}
  for(int i=0;i<20;i++) PS_ReceiveBuffer[i] = 0xFF;
}


/**
  * @file    FPM383C.cpp
  * @brief   外部中断函数，触发中断后开启模块的LED蓝灯（代表正在扫描指纹），接着由搜索指纹函数修改成功（闪烁绿灯）或失败（闪烁红灯）
  */
ICACHE_RAM_ATTR void InterruptFun()
{
  detachInterrupt(digitalPinToInterrupt(14));
  PS_ControlLED(PS_BlueLEDBuffer);
  delay_ms(10);
  ScanState |= 1<<4;
}


/**
  * @file    FPM383C.cpp
  * @brief   初始化主函数
  */
void setup()
{  
  mySerial.begin(57600);                              //软串口波特率，默认FPM383C指纹模块的57600
  pinMode(2,OUTPUT);                                  //ESP8266，Builtin LED内置的灯引脚模式
  pinMode(12,OUTPUT);                                 //继电器输出引脚
  pinMode(14,INPUT);                                  //FPM383C的2脚TouchOUT引脚，用于外部中断
  setup_wifi();
  pubclient.setServer(mqtt_server,mqtt_server_port);
  if(pubclient.connect(ID_MQTT)){
    pubclient.subscribe("lockSwitch");
    pubclient.setCallback(MsgCallback);
  }else{
    Serial.println("connection failed");
  }
  myservo.attach(16, 500, 2500); 
  delay_ms(200);                                      //用于FPM383C模块启动延时，不可去掉
  PS_Sleep();
  delay_ms(200);
  attachInterrupt(digitalPinToInterrupt(14),InterruptFun,RISING);     //外部中断初始化
}



void loop()
{
  if (LockStatus){
    myservo.write(0);
    Lockmsg = "on";
  }else{
    myservo.write(180);
    Lockmsg = "off";
  }
  if (!pubclient.connected()) {
    reconnect();
  }
  pubclient.loop();
  
  long now = millis();//获取当前时间戳
  if (now - lastMsg > 1000) {//如果达到1s，进行数据上传
    lastMsg = now;
    pubclient.publish(topic, Lockmsg);//数据上传
  }
  switch (ScanState)
  {
    //第一步
    case 0x10:    //搜索指纹模式
        SEARCH_ACK_CHECK(PS_Identify());
        delay(1000);
        PS_Sleep();
        ScanState = 0x00;
        attachInterrupt(digitalPinToInterrupt(14),InterruptFun,RISING);
    break;

    //第二步
    case 0x11:    //指纹中断提醒输入指纹ID，执行完毕返回搜索指纹模式
        Serial.println("Please Enter ID First");
        PS_ControlLED(PS_RedLEDBuffer);
        delay(1000);
        PS_Sleep();
        ScanState = 0x00;
        attachInterrupt(digitalPinToInterrupt(14),InterruptFun,RISING);
    break;
    
    //第三步
    case 0x12:    //指纹中断提醒按下功能按键，执行完毕返回搜索指纹模式
        Serial.println("Please Press Enroll or Delete Key");
        PS_ControlLED(PS_RedLEDBuffer);
        delay(1000);
        PS_Sleep();
        ScanState = 0x00;
        attachInterrupt(digitalPinToInterrupt(14),InterruptFun,RISING);
    break;

    //第四步
    case 0x13:    //连续搜索指纹模式，每次搜索前都必须由APP发送指纹ID，由函数将ScanState bit1置位才进入下一次搜索，否则提醒输入指纹ID并返回搜索模式
        ENROLL_ACK_CHECK(PS_Enroll(PageID));
        delay(1000);
        PS_Sleep();
        ScanState = 0x01;
        attachInterrupt(digitalPinToInterrupt(14),InterruptFun,RISING);
    break;

    //第五步
    case 0x14:    //指纹中断提醒输入指纹ID，执行完毕返回搜索指纹模式
        ScanState = 0x11;   //返回第二步，提示输入指纹ID
    break;

    //第六步
    case 0x16:    //单次指纹注册模式，必须同时满足按下单次注册按键且已输入ID情况下才会执行
        ENROLL_ACK_CHECK(PS_Enroll(PageID));
        delay(1000);
        PS_Sleep();
        ScanState = 0x00;
        attachInterrupt(digitalPinToInterrupt(14),InterruptFun,RISING);
    break;

    //第七步
    case 0x08:    //指纹中断提醒输入指纹ID，执行完毕返回搜索指纹模式
        ScanState = 0x11;   //返回第二步，提示输入指纹ID
    break;

    //第八步
    case 0x0A:    //单独指纹删除模式
        if(PS_Delete(PageID) == 0x00)
        {
          Serial.println("Delete Success");
          PS_ControlLED(PS_GreenLEDBuffer);
        }
        ScanState = 0x00;
    break;

    default:    //输入错误次数大于等于5次，将重新开启WiFI功能。
        if(WiFi_Connected_State == 0 || ErrorNum >= 5)
        {
          reconnect();
        }
    break;
  }
}