package sendMsg

import (
	"fmt"
	"github.com/tencentcloud/tencentcloud-sdk-go/tencentcloud/common"
	"github.com/tencentcloud/tencentcloud-sdk-go/tencentcloud/common/errors"
	"github.com/tencentcloud/tencentcloud-sdk-go/tencentcloud/common/profile"
	sms "github.com/tencentcloud/tencentcloud-sdk-go/tencentcloud/sms/v20210111"
	"time"
)

/*
	模板ID参数：
	1581035：门锁
	1581954：高温
	1581955：可燃气体
	1705432：早睡
	1706464：心率
*/

func SendMsg(TemplateID, phone, variable string) {
	time := time.Now().Format("01/02 15:04")
	var timeNow string
	timeNow = fmt.Sprintf("%v ", time)
	// 实例化一个认证对象，入参需要传入腾讯云账户secretId，secretKey,此处还需注意密钥对的保密
	// 密钥可前往https://console.cloud.tencent.com/cam/capi网站进行获取
	credential := common.NewCredential(
		"AKIDscAbCZijy84nMlyzoSr2zhCXfcNvHRZf",
		"zlU6V4RbYGk3C8rNhnfMtDKE9tQksoY7",
	)
	// 实例化一个client选项，可选的，没有特殊需求可以跳过
	cpf := profile.NewClientProfile()
	cpf.HttpProfile.Endpoint = "sms.tencentcloudapi.com"
	// 实例化要请求产品的client对象,clientProfile是可选的
	client, _ := sms.NewClient(credential, "ap-guangzhou", cpf)

	// 实例化一个请求对象,每个接口都会对应一个request对象
	request := sms.NewSendSmsRequest()

	request.PhoneNumberSet = common.StringPtrs([]string{phone})
	request.SmsSdkAppId = common.StringPtr("1400753697") //APPID
	request.SignName = common.StringPtr("周小辉个人网")        //签名
	request.TemplateId = common.StringPtr(TemplateID)    //模板ID
	if variable == "" {
		request.TemplateParamSet = common.StringPtrs([]string{timeNow}) //发送的变量
	} else {
		request.TemplateParamSet = common.StringPtrs([]string{timeNow, variable}) //发送的变量
	}
	// 返回的resp是一个SendSmsResponse的实例，与请求对象对应
	response, err := client.SendSms(request)
	if _, ok := err.(*errors.TencentCloudSDKError); ok {
		fmt.Printf("An API error has returned: %s", err)
		return
	}
	if err != nil {
		panic(err)
	}
	// 输出json格式的字符串回包
	fmt.Printf("%s", response.ToJsonString())
}
