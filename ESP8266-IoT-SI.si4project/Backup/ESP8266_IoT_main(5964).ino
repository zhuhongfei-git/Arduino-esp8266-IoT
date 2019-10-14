/******************************************************************************
*
*  Copyright (C), 2016-2036, REMO Tech. Co., Ltd.
*
*******************************************************************************
*  File Name     : ESP8266_IoT_main.ino
*  Version       : Initial Draft
*  Author        : zhuhongfei
*  Created       : 2019/10/10
*  Last Modified :
*  Description   : esp8266 main task
*  Function List :
*
*       1.                loop
*       2.                setup
*
*  History:
*
*       1.  Date         : 2019/10/10
*           Author       : zhuhongfei
*           Modification : Created file
*
******************************************************************************/

/*==============================================*
 *      include header files                    *
 *----------------------------------------------*/
#include <Arduino.h>
#include <DHT.h>


/*==============================================*
 *      constants or macros define              *
 *----------------------------------------------*/
//DHT11
#define DHTPIN    5
#define DHTTYPE   DHT11


/*==============================================*
 *      project-wide global variables           *
 *----------------------------------------------*/
DHT dht(DHTPIN, DHTTYPE);
bool shouldReboot;


/*Zh array
*todo:if you want to change Zh to english,
*please change here*/
const char zh_array[][48] =
{
    "OTA升级界面",            "本地升级",  "主机升级",  "升级",       "本地更新完成",
    "请输入(.bin)文件",       "文件过大",  "数据丢失",  "校验未通过", "主机升级需要定制"
    "WiFi AP配置",			  "WiFi账号",  "WiFi密码",  "确认密码",   "确定"
};

enum zh_array_subscript
{
    ota_update,              update_local,           update_host,         update_post,        local_update_finish,
    update_filename_error,   update_filesize_error,  update_write_error,  update_check_error, update_host_req,
    http_wifi_set,			 set_wifi_ssid,          set_wifi_pwd,        wifi_pwd_cfm,       http_make_sure,


};


/*==============================================*
 *      routines' or functions' implementations *
 *----------------------------------------------*/

void setup()
{
    // put your setup code here, to run once:
    setup_WiFi_mode();

    OLED_display_init();
    dht.begin();

    http_init();
}

void loop()
{
    // put your main code here, to run repeatedly:
    DHT11_read_sensor();
    OLED_run_function();

    //if (shouldReboot == true),ESP will reboot
    ESP_reboot();
}
