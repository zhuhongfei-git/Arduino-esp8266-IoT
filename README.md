# esp8266_IoT
## 综述：
  * 基于`esp8266`开发板，采用`Arduino`开发环境进行物联网终端与对接云平台开发
  * 采用`PubSubClient`开发库可让设备接入百度云；
  * 采用`ESP8266HTTPClient`和`ESP8266httpUpdate`可支持esp8266从指定的url地址获取升级包，实现升级；
## 支持功能：
  * （1）集成DHT11温湿度传感器，实时采集数据，由OLED显示屏显示；
  * （2）集成Ansycwebserver,基于esp8266自身的AP热点，可由网页查看和控制设备（传感器值，WiFi配置和OTA升级）；
  * （3）接入百度云，采用任务调度控制器按时上报数据到云平台；云平台可下发控制设备，并查看日志信息；
  * （4）百度云下发升级设备命令，实现远程升级设备的功能，并能够看到升级过程。

## 有问题反馈
在使用中有任何问题，欢迎反馈给我，邮箱地址如下。

## 关于作者

```javascript
  var zhuhongfei = {
    Name : zhf,
    site : https://github.com/zhuhongfei-git/esp8266_IoT
    email: zhf_allwell@163.com
  }
```
