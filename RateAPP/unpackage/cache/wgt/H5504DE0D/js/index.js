 //推送消息API
function PushMsg(topic,msg){
	var url = "https://apis.bemfa.com/va/postmsg?uid=2205b187c6079ed1deda0d2b14ae2716&topic="+topic+"&type=1&msg="+msg;
	fetch(url, {
	  method: "POST",
	  headers: {}
	  // body: "q=参数q"
	}).then(function(response) {
	  // do sth
	  console.log("success push!");
	});

}
//获得消息API topic：主题名 ， name：类名
function GetMsg(topic,name){
	var url = "https://apis.bemfa.com/va/getmsg?uid=2205b187c6079ed1deda0d2b14ae2716&topic="+topic+"&type=1";
	var nameTime = name+"time";
	fetch(url)
		.then(res => res.json())
		.then((out) => {
			// console.log('Output: ', out);
			var text = `${out.data[0].msg}`
			var time = `${out.data[0].time}`
			$(name).text(text);
			$(nameTime).text(time);
	}).catch(err => console.error(err));
}
function GetStatus(topic,name){
	var url = "https://apis.bemfa.com/va/getmsg?uid=2205b187c6079ed1deda0d2b14ae2716&topic="+topic+"&type=1";
	fetch(url)
		.then(res => res.json())
		.then((out) => {
			// console.log('Output: ', out);
			var text = `${out.data[0].msg}`
			$(name).text(text);
			$(name + "Sw").text(text);
	}).catch(err => console.error(err));
}
function ChangeStatus(element){
	// var val = $("#id").text();
	var str = element.id;
	if ($("#"+element.id).text() == "on") {
		document.getElementById(element.id).classList.remove("btn-primary");
		document.getElementById(element.id).classList.add("btn-danger");
		PushMsg(str.slice(0,-2),"off");
	} else{
		document.getElementById(element.id).classList.remove("btn-danger");
		document.getElementById(element.id).classList.add("btn-primary");
		PushMsg(str.slice(0,-2),"on");
	}
}
function SendMsg(){
	var topic = document.getElementById("MqttTopic").value;
	var msg = document.getElementById("MqttMsg").value;
	console.log(topic+":"+msg);
	PushMsg(topic,msg);
}
//定时器函数
var TaskLists = function () {
	// let time = new Date();
	GetMsg("Spo2",".spo");
	GetMsg("HeartBeat",".heartrate");
	GetMsg("temp",".temp");
	GetMsg("humi",".humi");
	GetMsg("water",".water");
	GetMsg("HomeMsg",".HomeMsg");
	GetStatus("lock","#lock");
	GetStatus("led","#led");
	// console.log();
 }