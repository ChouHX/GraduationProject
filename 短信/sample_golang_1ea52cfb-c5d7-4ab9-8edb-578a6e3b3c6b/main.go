package main

import (
	"bytes"
	"encoding/binary"
	"fmt"
	mqtt "github.com/eclipse/paho.mqtt.golang"
	"goclient/sendMsg"
	"math"
	"strconv"
	"time"
)

var (
	Temp       float64
	Humi       float64
	LockStatus string
	HeartBeat  float64
)

func BytesToInt(bs []byte) int {
	bytebuffer := bytes.NewBuffer(bs) //根据二进制写入二进制结合
	var data int64
	binary.Read(bytebuffer, binary.BigEndian, &data) //解码
	return int(data)
}

func BytesToFloat32(bytes []byte) float32 {
	bits := binary.LittleEndian.Uint32(bytes)
	return math.Float32frombits(bits)
}

var messagePubHandler mqtt.MessageHandler = func(client mqtt.Client, msg mqtt.Message) {
	//fmt.Printf("Received message: %s from topic: %s\n", msg.Payload(), msg.Topic())
	fmt.Printf("msg: %s from topic: %s\n", msg.Payload(), msg.Topic())
	if msg.Topic() == "temp" {
		str := string(msg.Payload())
		Temp, _ = strconv.ParseFloat(str, 64)
	}
	if msg.Topic() == "humi" {
		str := string(msg.Payload())
		Humi, _ = strconv.ParseFloat(str, 64)
	}
	if msg.Topic() == "lock" {
		LockStatus = string(msg.Payload())
	}
	if msg.Topic() == "HeartBeat" {
		str := string(msg.Payload())
		HeartBeat, _ = strconv.ParseFloat(str, 64)
	}
}

var connectHandler mqtt.OnConnectHandler = func(client mqtt.Client) {
	fmt.Println("Connected")
}

var connectLostHandler mqtt.ConnectionLostHandler = func(client mqtt.Client, err error) {
	fmt.Printf("Connect lost: %v", err)
}

func publish(client mqtt.Client) {
	num := 10
	for i := 0; i < num; i++ {
		text := fmt.Sprintf("Message %d", i)
		token := client.Publish("temp", 0, false, text)
		token.Wait()
		time.Sleep(time.Second)
	}
}

func sub(client mqtt.Client) {
	topic1 := "temp"
	//topic2 := "humi"
	topic3 := "lock"
	topic4 := "HeartBeat"
	token1 := client.Subscribe(topic1, 1, nil)
	//token2 := client.Subscribe(topic2, 1, nil)
	token3 := client.Subscribe(topic3, 1, nil)
	token4 := client.Subscribe(topic4, 1, nil)
	token1.Wait()
	//token2.Wait()
	token3.Wait()
	token4.Wait()
}

func main() {
	//声明mqtt服务器地址及端口
	var broker = "bemfa.com"
	var port = 9501
	opts := mqtt.NewClientOptions()
	opts.AddBroker(fmt.Sprintf("tcp://%s:%d", broker, port))
	opts.SetClientID("2205b187c6079ed1deda0d2b14ae2716")
	opts.SetUsername("")
	opts.SetPassword("")
	opts.SetDefaultPublishHandler(messagePubHandler)
	opts.OnConnect = connectHandler
	opts.OnConnectionLost = connectLostHandler
	client := mqtt.NewClient(opts)
	if token := client.Connect(); token.Wait() && token.Error() != nil {
		panic(token.Error())
	}
	//订阅主题，接收服务器消息
	sub(client)
	//高温预警
	go func() {
		flag := 0
		for {
			//fmt.Println(Temp, ",", Humi)
			time.Sleep(3 * time.Second)
			if Temp > 30 && flag <= 3 {
				sendMsg.SendMsg("1581954", "18109038858", "")
				time.Sleep(2 * time.Minute)
				flag++
			}
		}
	}()
	//门锁打开报警
	go func() {
		flag := 0
		for {
			if LockStatus == "on" && flag <= 3 {
				sendMsg.SendMsg("1581035", "18109038858", "")
				time.Sleep(5 * time.Minute)
				flag++
			}
		}
	}()
	//心率异常报警
	go func() {
		flag := 0
		for {
			if HeartBeat > 80 && flag <= 3 {
				beatStr := fmt.Sprintf("%v/m", HeartBeat)
				sendMsg.SendMsg("1706464", "18109038858", beatStr)
				time.Sleep(1 * time.Minute)
				flag++
			}
		}
	}()
	var anykey string
	fmt.Scanln(&anykey)
	client.Disconnect(250)
}
