// index.js
// 获取应用实例
const app = getApp()

Page({
  data: {

    //需要修改的地方
    uid:"2205b187c6079ed1deda0d2b14ae2716",//用户密钥，巴法云控制台获取
    device_status:"离线",// 显示led是否在线的字符串，默认离线
    Htopic:"HeartBeat",
    Stopic:"Spo2",
    ledOnOff:"关闭",     //显示led开关状态
    checked: false,     //led的状态记录。默认led关闭
    heartbeat:"",//温度值，默认为空
    spo2:"",//湿度值，默认为空
    dataTime:"", //记录数据上传的时间
    ledicon:"/utils/img/lockoff.png",//显示led图标的状态。默认是关闭状态图标
  },

//屏幕打开时执行的函数
  onLoad() {
    //设置定时器，每3秒请求一下设备状态
    this.getStatus()
    this.getInfo()
    setInterval(this.getInfo, 1000)
  },
  //获取在线状态 https://apis.bemfa.com/va/online
  getStatus(){
    var that = this
    wx.request({
        url: 'https://apis.bemfa.com/va/online',
        data: {uid:this.data.uid,topic:this.data.Htopic,type:1},
        method: 'GET', // OPTIONS, GET, HEAD, POST, PUT, DELETE, TRACE, CONNECT
        header: {}, // 设置请求的 header
        success: function(res){
         // success
            if (res.data.data == false) {
                that.setData({
                    device_status:"离线",
                })
            } else {
                that.setData({
                    device_status:"在线",
                })
            }
        },
        fail: function() {
         // fail
        },
        complete: function() {
         // complete
        }
       })
  },
  //心率血氧数据
  getInfo(){
    var that = this
    wx.request({
        url: 'https://apis.bemfa.com/va/getmsg',
        data: {uid:this.data.uid,topic:this.data.Htopic,type:1},
        method: 'GET', // OPTIONS, GET, HEAD, POST, PUT, DELETE, TRACE, CONNECT
        header: {}, // 设置请求的 header
        success: function(res){
            that.setData({
                heartbeat:res.data.data[0].msg,
                dataTime:res.data.data[0].time,
            })
        },
        fail: function() {
         // fail
        },
        complete: function() {
         // complete
        }
       }),
       wx.request({
        url: 'https://apis.bemfa.com/va/getmsg',
        data: {uid:this.data.uid,topic:this.data.Stopic,type:1},
        method: 'GET', // OPTIONS, GET, HEAD, POST, PUT, DELETE, TRACE, CONNECT
        header: {}, // 设置请求的 header
        success: function(res){
            that.setData({
                spo2:res.data.data[0].msg,
            })
        },
        fail: function() {
         // fail
        },
        complete: function() {
         // complete
        }
       })
  },
  //发送开关数据
  SendMsg(topic,msg){
    wx.request({
      url: 'https://apis.bemfa.com/va/postmsg', //状态api接口，详见巴法云接入文档
      method:"POST",
      data: {
        uid: this.data.uid,
        topic: topic,
        type:1,
        msg:msg
      },
      header: {
        'content-type': "application/x-www-form-urlencoded"
      },
      success (res) {

      }
    })   
  },
  //控制灯的函数1，小滑块点击后执行的函数
  onChange({ detail }){
    //detail是滑块的值，检查是打开还是关闭，并更换正确图标
    this.setData({ 
      checked: detail,
     });
     if(detail == true){//如果是打开操作
      this.SendMsg("lock","on") //发送打开指令
      this.setData({ 
        ledicon: "/utils/img/lockon.png",//设置led图片为on
       });
     }else{
      this.SendMsg("lock","off") //发送关闭指令
      this.setData({ 
        ledicon: "/utils/img/lockoff.png",//设置led图片为off
       });
     }
  },
  //点击led图片执行的函数
  onChange2(){
    var that = this
      //如果点击前是打开状态，现在更换为关闭状态，并更换图标，完成状态切换
      if( that.data.checked == true){
        that.SendMsg("lock","off")//发送关闭指令
        this.setData({ 
            ledicon: "/utils/img/lightoff.png",//设置led图片为off
            checked:false //设置led状态为false
         });
      }else{
        //如果点击前是关闭状态，现在更换为打开状态，并更换图标，完成状态切换
        that.SendMsg("lock","on")//发送打开指令
        that.setData({ 
          ledicon: "/utils/img/lighton.png",//设置led图片为on
          checked:true//设置led状态为true
       });
      }
  },
  //分享函数
  onShareAppMessage() {
    const promise = new Promise(resolve => {
      setTimeout(() => {
        resolve({
          title: '小小智能家居'
        })
      }, 2000)
    })
    return {
      title: '小小智能家居',
      path: '/page/index/index.wxml',
      promise 
    }
  }
})
