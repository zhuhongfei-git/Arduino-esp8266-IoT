/******************************************************************************
*
*  Copyright (C), 2016-2036, Telchina. Co., Ltd.
*
*******************************************************************************
*  File Name     : esp8266_Webserver.ino
*  Version       : Initial Draft
*  Author        : zhuhongfei
*  Created       : 2019/6/5
*  Last Modified :
*  Description   : ESP8266 webserver
*  Function List :
*
*       1.                 cacl_crc16
*       2.                 copy_update_file
*       3.                 get_file_system_msg
*       4.                 http_cfgwifi_page
*       5.                 http_config_page
*       6.                 http_function_sel_page
*       7.                 http_handle_cfgwifi
*       8.                 http_handle_config_data
*       9.                 http_handle_function_sel
*       10.                http_handle_update_esp8266
*       11.                http_handle_update_stm32
*       12.                http_init
*       13.                http_main_page
*       14.                http_status_page
*       15.                http_update_host_status
*       16.                http_update_page
*       17.                load_ap_params
*       18.                load_config_params
*       19.                loop
*       20.                notFound
*       21.                parse_cfg_sta_data
*       22.                parse_cmd_id
*       23.                parse_config_data
*       24.                parse_wifi_data
*       25.                processor
*       26.                receive_cfg_sta_data
*       27.                receive_cmd_id
*       28.                receive_update_data
*       29.                receive_wifi_data
*       30.                save_config_file
*       31.                send_cmd_to_host
*       32.                setup
*       33.                setup_WiFi_mode
*       34.                start_data_server
*       35.                uart_to_tcp
*       36.                update_fs_to_host
*
*  History:
*
*       1.  Date         : 2019/6/5
*           Author       : zhuhongfei
*           Modification : Created file
*
******************************************************************************/

/*==============================================*
 *      include header files                    *
 *----------------------------------------------*/
#include "ESP8266WiFi.h"

#include "ESPAsyncTCP.h"
#include "ESPAsyncWebServer.h"
#include "FS.h"
#include "ESP8266HTTPUpdateServer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ArduinoJson.h"

#include <Wire.h>
#include <ACROBOTIC_SSD1306.h>

#include <DHT.h>

/*==============================================*
 *      constants or macros define              *
 *----------------------------------------------*/
#define WEB_SRV_PORT      80
#define WIFI_SRV_PORT     90
#define MAX_SRV_CLIENTS   2
#define WIFI_CHANNAL      3
#define WIFI_HIDDEN       0
#define WIFI_MAX_CONNECT  4
#define CONFIG_DATA_BYTE  14
#define STATUS_DATA_BYTE  23
#define PADDING_DATA_BYTE 0x04
#define SERIAL_DATA_LENS  32
#define WIFI_DATA_LENS    40
#define CMD_ID_DATA       3

#define HEADER            0x55
#define ENDUP             0xAA

#define DEBUG_KEY          0
#define DEBUG_UART_PRINTF  Serial1.printf
#define DEBUG_UART_PRINTLN Serial1.println
#define DEBUG_UART_PRINT   Serial1.print

#define UPDATE_HOST_PACKAGE 800  //limit update file size to 200k ,200*1024/256=800//
#define UPDATE_TIME_OUT     30000 //time out 30s

//host OTA
#define UPDATE_DATA_LENS  256
//NB OTA
#define NB_UPDATE_LENS    32  //ever NB package consist of 32 bytes
//host log data lens
#define HOST_LOG_BYTES    27

//DHT11
#define DHTPIN    5
#define DHTTYPE   DHT11
DHT dht(DHTPIN, DHTTYPE);



/*==============================================*
 *      project-wide global variables           *
 *----------------------------------------------*/
AsyncWebServer server(WEB_SRV_PORT);
WiFiServer data_server(WIFI_SRV_PORT);
WiFiClient serverClients[MAX_SRV_CLIENTS];

const char *host = "esp8266-webupdate";

const char *def_ap_password = "12345678";
const char *def_ap_ssid = "Telchinaxf";
char ap_ssid[17];
char ap_pwd[17];
char wifi_head[4];
char wifi_set[WIFI_DATA_LENS];
uint8_t read_wifi_count = 0;
uint8_t read_end_count = 0;

//const char *PARAM_MESSAGE = "message";
//Web login authentication
const char *user_name = "telchina";
const char *user_password = "telchina";

//serial data
uint8_t save_cmd_id[CMD_ID_DATA];
uint8_t save_cfg_data[CONFIG_DATA_BYTE];
uint8_t save_sta_data[STATUS_DATA_BYTE];
uint8_t save_serial_data[SERIAL_DATA_LENS];
uint8_t read_cmd_count    = 0;
uint8_t current_cmd_id    = 0;
uint8_t read_status_start = 0;      //choose to receive status data or tcpserver
uint8_t read_status_count = 0;      //count status data number
uint8_t read_status_end   = 0;      //receive status data end flag

//application task id
enum cmd_id
{
    cfg_get,
    cfg_set,
    sta_get,
    sta_send,
    set_wifi,
    tcp_tran,
    update_host_req, //tell host,it will update host,0x06
    update_host_rec, //make ready for receiving update data from host,0x07
    update_check_success,
    update_check_error,
    update_no_response,
    //update_send_finish,
    host_record_log,
    transpot_mode,

};
//display error msg when parse data
enum error_msg
{
    no_error,
    check_error,
    cmd_id_error,
    norm_error,
    receive_error,
} error_msg_flag;

//list cfg and sta array subscript
enum prot_set
{
    prot_head       = 0,
    prot_cmd_id     = 1,
    prot_data_len   = 2,
    prot_data_start = 3,

    prot_end        = 31,
};
enum post_cfg_data
{
    get_nb_ip1,
    get_nb_ip2,
    get_nb_ip3,
    get_nb_ip4,
    get_nb_port,
    get_angle_th,
    get_depth_max,
    get_depth_min,
    get_temper,
    get_start_nb,
    get_start_gps,
    get_gps_cycle,
    get_data_cycle,
};
enum cfg_array_sub
{
    nb_ip1,
    nb_ip2,
    nb_ip3,
    nb_ip4,
    nb_port_high,
    nb_port_low,
    angle_th,
    depth_max,
    depth_min,
    temper_th,
    start_nb,
    start_gps,
    gps_cycle,
    data_cycle,
};

enum sta_array_sub
{
    work_status,
    online,
    lat_dd_index,
    lat_ff_index,
    lat_mm_index,
    lat_dir_index,
    log_dd_index,
    log_ff_index,
    log_mm_index,
    log_dir_index,
    gps_loca_sta,
    nb_strength,
    gps_strength,
    temper_sta,
    time_hh_index,
    time_mm_index,
    time_ss_index,
    time_ms_index,
    volt_high,
    volt_low,
    angle_sta,
    depth_high,
    depth_low,
};
//config data
char NB_lot_ip1[4];
char NB_lot_ip2[4];
char NB_lot_ip3[4];
char NB_lot_ip4[4];
char NB_lot_port[8];
char Adx1_ang[4];
char depth_max_cfg[4];
char depth_min_cfg[4];
char Tempera[8];
char Nb_on[4];
char GPS_on[4];
char GPS_update_cycle[4];
char Data_update_cycle[4];
char data_source[16];

//host log record num
uint8_t record_host_log = 0;
uint8_t save_log_data[HOST_LOG_BYTES];
enum status_log_array
{
    work_status_sub,
    online_sub,
    lat_dd_sub,
    lat_ff_sub,
    lat_mm_sub,
    lat_dir_sub,
    log_dd_sub,
    log_ff_sub,
    log_mm_sub,
    log_dir_sub,
    gps_sta_sub,
    nb_strength_sub,
    gps_strength_sub,
    temper_sta_sub,
    time_hh_sub,
    time_mm_sub,
    time_ss_sub,
    time_ms_sub,
    date_year_sub,
    date_mon_sub,
    date_day_sub,
    volt_high_sub,
    volt_low_sub,
    angle_sta_sub,
    depth_high_sub,
    depth_low_sub,
    record_num_sub,
};

//JSON config file
const char *configFileName   = "/config.txt";

const uint16_t max_bytes_config_file = 1024;
StaticJsonDocument<max_bytes_config_file> doc;

//reboot esp8266 flag
bool shouldReboot;

//save update host file
const char *save_update_file   = "/update.txt";
const char *save_update_backup = "/update_backup.txt";
File savefile;

//update host parameters
uint8_t  update_chk_sum;	   //check sum
uint32_t update_data_index;    //hex/bin file bytes
uint16_t  update_data_count;    //count bytes ,if(update_data_count==32) update_data_count=0;
uint16_t update_package_num;   //package number
uint16_t update_package_sum;   //sum up the update data package
uint16_t update_calc_crc = 0xffff;  //calculate crc16 value
uint16_t update_rece_crc = 0;       //crc value received from host
uint32_t update_data_byte_sum =
    0; //receive update data bytes,limit 200k=200*1024 B
//update host flag
uint8_t update_host_processing;//updating host
uint8_t update_host_processed; //updated host
uint8_t update_host_error;     //update error
uint8_t update_fs2uart_start;  //update from fs to uart
uint8_t update_chksum_flag = 0;
uint8_t update_online_flag = 0;//online update or offline update
//uint8_t update_chksum_fail;
/*Zh array
*todo:if you want to change Zh to english,
*please change here*/
const char zh_array[][48] =
{
    "OTA升级,请选择可执行文件",  "本地升级",         "主机升级",                      "升级",              "界面选择",
    "配置界面",                  "显示界面",         "WiFi配置",                      "OTA升级",           "功能选择",
    "未登录网络,未进行网络搜索", "已登录本地网络",   "未登录网络,正在搜索网络",       "注册被拒绝",        "未知状态",
    "已登录网络,处于漫游状态",   "注册到'仅限短信'", "注册到'仅限短信',处于漫游状态", "电信",              "联通",
    "移动",                      "未定位",           "SPS模式,定位有效",              "DSPS模式,定位有效", "工作状态",
    "注网信息",                  "GNSS定位",         "GNSS定位状态",                  "NB信号强度",        "GNSS信号强度",
    "颗",                        "实时温度",         "当前时间",                      "电池电压",          "当前角度",
    "当前深度",                  "刷新",             "NB远端IP", 							  "NB远端端口",        "范围",
    "倾角上限",                  "深度最大值",       "深度最小值",                    "温度阈值",          "启用NB模块",
    "是",                        "否",               "启用GPS模块",                   "GPS上报周期",       "数据上报周期",
    "主机连接状态",              "发送",             "通知主机更改传输类型",          "正常业务",          "WiFi设置",
    "调试透传",                  "确定",             "Telchina WiFi 配置",            "配置WiFi",          "配置成功",
    "继续配置",        "已成功通知主机更改传输类型", "当前配置",                 "Telchina WiFi 配置成功", "配置失败,请重新配置",
    "请检查两次输入密码是否一致", "已连接",          "未连接",            "数据校验不通过,请重新刷新页面", "数据CMD_ID不一致,请重新刷新页面",
    "数据格式错误，请重新刷新页面",   "未接收到数据，请重新刷新页面",     "更新文件已成功存储到本地",      "本地更新完成",
    "查看主机升级过程",          "跳转",             "当前发送的数据包序列号",        "总包数",
    "主机已完成升级",            "升级失败,请重试",  "正常等待主机切换分区,继续刷新", "正在升级主机",      "提示:主机异常,请检查",
    "WiFi账号",					 "WiFi密码",			 "确认密码",                      "日志记录",          "当前日期",
    "日志序列号",                "跳转序列号",       "运输模式"
};
enum zh_array_subscript
{
    ota_update,      update_local,     update_host,        update_post,      sel_page,
    cfg_page,        sta_page,         cfgwifi_page,       update_page,      sel_function,
    no_log_no_scan,  log_local_net,    no_log_but_scan,    reg_reject,       unknow_status,
    log_and_roaming, reg_msg,          reg_msg_roaming,    tele_com,         unicom,
    mobile,          no_target,        sps_target,         dsps_target,      work_sta,
    net_msg,         gnss_target,      gnss_target_status, nb_lot_strength,  gnss_strength,
    unit_ke,         real_time_temper, time_now,           battery_volt,     angle_now,
    depth_now,		 flush_post,	   nb_remote_ip,	   nb_remote_port,	 range,
    angth_threshold, depth_max_th,     depth_min_th,       temper_threshold, enable_nb,
    yes,             no,               enable_gps,         gps_report_cycle, data_report_cycle,
    host_connect,    send_post,        notice_change_type, app_task,         wifi_set_task,
    debug_tcp,       make_sure,		   tel_wifi_set,	   config_wifi,      config_success,
    config_again,	 notice_host,      config_now,         tel_config,       config_fail,
    pwd_consistent,  connect_success,  connect_fail,       check_sum_fail,   cmd_id_inconsistent,
    data_norm_error, no_receive_data,  file_send_host,     local_update_finish,
    update_log,      jump_out,         package_num,        finish_prop,
    update_success,  update_fail,      host_sure,          updating_host,    update_fail_alert,
    tel_wifi_ssid,   tel_wifi_pwd,     tel_wifi_pwd_cfm,   record_log,       date_now,
    record_num_now,  jump_log_num,     enter_transport_mode,
};
bool save_config_file();
void load_ap_params();
void load_config_params();
void parse_wifi_data();
void receive_cfg_sta_data();
void send_cmd_to_host(uint8_t cmd_id);
void parse_cmd_id();






/*==============================================*
 *      routines' or functions' implementations *
 *----------------------------------------------*/
/*****************************************************************************
*   Prototype    : notFound
*   Description  : Handle unregistered requests
*   Input        : AsyncWebServerRequest *request
*   Output       : None
*   Return Value : void
*   Calls        :
*   Called By    :
*
*   History:
*
*       1.  Date         : 2019/6/5
*           Author       : zhuhongfei
*           Modification : Created function
*
*****************************************************************************/
void notFound(AsyncWebServerRequest *request)
{
    request->send(404, "text/plain", "Not found");
}

/*****************************************************************************
*   Prototype    : setup_WiFi_mode
*   Description  : WiFi mode: AP or STA
*   Input        : None
*   Output       : None
*   Return Value : void
*   Calls        :
*   Called By    :
*
*   History:
*
*       1.  Date         : 2019/6/5
*           Author       : zhuhongfei
*           Modification : Created function
*
*****************************************************************************/
void setup_WiFi_mode()
{
#if DEBUG_KEY
    DEBUG_UART_PRINTLN("Setting soft-AP ... ");
#endif
    WiFi.mode(WIFI_AP);//WIFI_AP_STA
    IPAddress softLocal(192, 168, 4, 1);
    IPAddress softGateway(192, 168, 4, 1);
    IPAddress softSubnet(255, 255, 255, 0);

    WiFi.softAPConfig(softLocal, softGateway, softSubnet);
    //String ap_name=("ESP8266-Telchina_"+(String)ESP.getChipId());
    //const char *ap_ssid     = ap_name.c_str();

    load_ap_params();
    if ((*ap_ssid != NULL) && (*ap_pwd != NULL))
    {
        WiFi.softAP(ap_ssid, ap_pwd, WIFI_CHANNAL, WIFI_HIDDEN, WIFI_MAX_CONNECT);
    }

    else
    {
        WiFi.softAP(def_ap_ssid, def_ap_password, WIFI_CHANNAL, WIFI_HIDDEN,
                    WIFI_MAX_CONNECT);
    }


    IPAddress myIP = WiFi.softAPIP();
#if DEBUG_KEY
    DEBUG_UART_PRINT("AP IP address: ");
    DEBUG_UART_PRINTLN(myIP);
#endif


    //station mode ,not used now
    //TODO:1.will be used;2.if->while delay(500ms)+try connect count

    //DEBUG_UART_PRINTLN("Starting soft-sta ... ");
    //WiFi.begin(ap_ssid, ap_password);//ssid, password
    /*if (WiFi.waitForConnectResult() != WL_CONNECTED)
    {
        DEBUG_UART_PRINTF("WiFi Failed!\n");
        return;
    }

    DEBUG_UART_PRINT("IP Address: ");
    DEBUG_UART_PRINTLN(WiFi.localIP());
    DEBUG_UART_PRINT("Hostname: ");
    DEBUG_UART_PRINTLN(WiFi.hostname());*/

}
String processor(const String& var)
{
    return String();
}

/*****************************************************************************
*   Prototype    : get_file_system_msg
*   Description  : get file system message(total bytes、used bytes)
*   Input        : None
*   Output       : None
*   Return Value : void
*   Calls        :
*   Called By    :
*
*   History:
*
*       1.  Date         : 2019/7/2
*           Author       : zhuhongfei
*           Modification : Created function
*
*****************************************************************************/
void get_file_system_msg()
{
    FSInfo info_fs;
    SPIFFS.info(info_fs);
#if DEBUG_KEY
    DEBUG_UART_PRINTLN("SPIFFS memory");
    DEBUG_UART_PRINT("totalBytes:");
    DEBUG_UART_PRINTLN(info_fs.totalBytes);
    DEBUG_UART_PRINT("usedBytes:");
    DEBUG_UART_PRINTLN(info_fs.usedBytes);
#endif
}

/*****************************************************************************
*   Prototype    : update_fs_to_host
*   Description  : send update data from spiffs to uart
*   Input        : None
*   Output       : None
*   Return Value : void
*   Calls        :
*   Called By    :
*
*   History:
*
*       1.  Date         : 2019/7/2
*           Author       : zhuhongfei
*           Modification : Created function
*
*****************************************************************************/
void update_fs_to_host()
{
    update_chk_sum         = 0;
    update_data_index      = 0;
    update_data_count      = 0;
    update_package_num     = 0;

    update_host_error      = 0;
    update_host_processing = 0;
    update_host_processed  = 0;
    update_data_byte_sum   = 0;
    uint8_t first_three_bytes = 0;
    uint8_t update_data       = 0;
    uint8_t check_error_count = 0;
    unsigned long wait_last   = 0; //wait fir about 10s,update fail
    unsigned long wait_now    = 0;
    /*if(update_online_flag)
    {
    #if DEBUG_KEY

    	DEBUG_UART_PRINTLN("online update");
    	DEBUG_UART_PRINTLN("closing wifi");
    #endif
    	WiFi.softAPdisconnect(true);
    	//update_online_flag = 0;
    }
    else
    {
    #if DEBUG_KEY
    	DEBUG_UART_PRINTLN("offline update");
    #endif
    }*/
    if (SPIFFS.exists(save_update_file))
    {
        File send_update_file = SPIFFS.open(save_update_file, "r");
        if (send_update_file)
        {
            while (send_update_file.available())
            {
                if ((first_three_bytes >= 0) && (first_three_bytes < 3))
                {
                    update_data = send_update_file.read();
                    update_data_byte_sum |= ((uint32_t)update_data) << (16 - 8 * first_three_bytes);
                    first_three_bytes ++;
                    continue;
                }
                else if (first_three_bytes == 3)
                {
                    if (((update_data_byte_sum + 2) % UPDATE_DATA_LENS) ==
                            0) //SERIAL_DATA_LENS -> UPDATE_DATA_LENS
                    {
                        update_package_sum = ((update_data_byte_sum + 2) / UPDATE_DATA_LENS) +
                                             1; //SERIAL_DATA_LENS -> UPDATE_DATA_LENS
                    }
                    else
                    {
                        update_package_sum = ((update_data_byte_sum + 2) / UPDATE_DATA_LENS) +
                                             2;//SERIAL_DATA_LENS -> UPDATE_DATA_LENS
                    }
                    update_package_num = 0;
                    update_data_index  = 0;
                    first_three_bytes ++;
                }
                if ((update_data_index % UPDATE_DATA_LENS) ==
                        0) //SERIAL_DATA_LENS -> UPDATE_DATA_LENS
                {
                    Serial.write(HEADER);
                    Serial.write(update_package_num >> 8); //send high byte
                    Serial.write(update_package_num);      //send low  byte
                    if (update_package_num == 0)
                    {
                        Serial.write(update_data_byte_sum >> 16);
                        Serial.write(update_data_byte_sum >> 8);
                        Serial.write(update_data_byte_sum);
                        update_chk_sum += (update_data_byte_sum >> 16);
                        update_chk_sum += (update_data_byte_sum >> 8);
                        update_chk_sum += (update_data_byte_sum & 0xff);
                    }

                    update_package_num++;
                    if (UPDATE_HOST_PACKAGE == update_package_num)
                    {
                        update_host_error      = 1; //update_error
                        update_host_processing = 0;
                        update_host_processed  = 0;
                        update_package_num = 0;
                        update_data_index  = 0;
                        first_three_bytes  = 0;
                        Serial.flush();
                        send_update_file.close();
                        return ;
                    }
                    update_host_processing = 1;
                }
                if (update_package_num == 1)
                {
                    for (uint16_t i = 0; i < (UPDATE_DATA_LENS - 3); i++)
                    {
                        Serial.write(0);
                    }
                }
                else
                {
                    update_data = send_update_file.read();
                    update_chk_sum += update_data;
                    Serial.write(update_data);

                    update_data_index++;
                    update_data_count++;
                }

                if ((update_package_num == 1) || (UPDATE_DATA_LENS == update_data_count) ||
                        ((update_data_byte_sum + 2) ==
                         update_data_index)) //SERIAL_DATA_LENS -> UPDATE_DATA_LENS
                {
                    //add padding data when the end package bytes less than UPDATE_DATA_LENS
                    if ((UPDATE_DATA_LENS != update_data_count) && (update_package_num != 1))
                    {
                        for (uint16_t i = 0; i < (UPDATE_DATA_LENS - update_data_count); i++)
                        {
                            Serial.write(0);
                        }
                    }
                    Serial.write(update_chk_sum);
                    Serial.write(ENDUP);
                    Serial.flush();

                    //wait for host response
                    wait_last = millis();
                    while (!read_status_end)
                    {
                        receive_cfg_sta_data();
                        wait_now = millis();

                        if ((wait_now - wait_last) == UPDATE_TIME_OUT)
                        {
#if DEBUG_KEY
                            //DEBUG_UART_PRINTF("Time now:%d\n",wait_now);
                            DEBUG_UART_PRINTF("Time difference:%d\n", (wait_now - wait_last));
#endif
                            break;
                        }
                    }
                    wait_last = 0;
                    wait_now  = 0;

                    parse_cmd_id();

                    switch (update_chksum_flag)
                    {
                        case update_check_success:
                            update_chksum_flag = 0;
                            check_error_count  = 0;
                            break;
                        case update_check_error:
                            update_chksum_flag = 0;
                            check_error_count++;

                            send_update_file.seek(-update_data_count,
                                                  SeekCur); //SERIAL_DATA_LENS -> UPDATE_DATA_LENS
                            update_package_num--;
                            if (10 == check_error_count)
                            {
                                send_update_file.close();
                                update_host_error      = 1; //update_error
                                update_host_processing = 0;
                                update_host_processed  = 0;
                                update_package_num = 0;
                                update_data_index  = 0;
                                first_three_bytes  = 0;
                                return ;
                            }
                            break;
                        case update_no_response:
                            update_host_error      = 1; //update_error
                            update_host_processing = 0;
                            update_host_processed  = 0;
                            update_package_num = 0;
                            update_data_index  = 0;
                            first_three_bytes  = 0;
                            send_update_file.close();
                            return ;
                            break;
                        default:
                            update_chksum_flag = 0;
                            send_update_file.seek(0, SeekSet);
                            update_package_num = 0;

                            break;
                    }

                    update_data_count = 0;
                    update_chk_sum = 0;
                }

            }
            send_update_file.close();
            update_host_processing = 0;
            update_host_processed  = 1;
#if DEBUG_KEY
            DEBUG_UART_PRINTF("\n*********package_num sum:%d*********\n",
                              update_package_num);
#endif

            //send_cmd_to_host(update_send_finish);//tell the host that update data send finish
            /*if(update_online_flag)
            {
            #if DEBUG_KEY
            	DEBUG_UART_PRINTLN("online update finish");
            	DEBUG_UART_PRINTLN("opening wifi");
            #endif

            	setup_WiFi_mode();
            	update_online_flag = 0;
            }
            else
            {
            #if DEBUG_KEY
            	DEBUG_UART_PRINTLN("offline update finish");
            #endif
            }*/
            update_data_count  = 0;
            update_chk_sum     = 0;
            update_data_index  = 0;
            update_package_num = 0;
            first_three_bytes  = 0;
        }
        else
        {
#if DEBUG_KEY
            DEBUG_UART_PRINTLN("open update file fail");
#endif
        }
    }
}
/*****************************************************************************
*   Prototype    : http_update_page
*   Description  : provide esp8266 update request interface
*   Input        : AsyncWebServerRequest *request
*   Output       : None
*   Return Value : void
*   Calls        :
*   Called By    :
*
*   History:
*
*       1.  Date         : 2019/6/9
*           Author       : zhuhongfei
*           Modification : Created function
*
*****************************************************************************/
void http_update_page(AsyncWebServerRequest *request)
{
    String html = "";
    html += "<html> <head><meta http-equiv='Content-Type' content='text/html ;charset=utf-8'/>";
    html = html +
           "<style>label{ cursor: pointer; display: inline-block;padding: 3px 6px;" +
           "text-align: left;width: 100px;vertical-align: top;font-size:20px;}";
    html = html + "input[type='radio']{width:20px;height:20px;}";
    html = html + "input[type='file']{height:30px;line-height:30px;font-size:20px;}"
           +
           "input[type='submit']{height:35px;line-height:35px;font-size:25px;}</style></head>";
    html = html + "<h1>" + zh_array[ota_update] + "</h1> <body>";
    html += "<form method='POST' action='/doupdate' enctype='multipart/form-data'>";
    html = html + "<label>" + zh_array[update_local] + "</label>" +
           "<input type='radio' name='update' value='esp8266' checked='checked'><br>";
    html = html + "<label>" + zh_array[update_host]  + "</label>" +
           "<input type='radio' name='update' value='stm32'><br><br>";
    html = html +
           "<input type='file' name='update'><br><br><input type='submit' value=" +
           zh_array[update_post] + "></form></body></html>";
    request->send(200, "text/html", html.c_str());
}


/*****************************************************************************
*   Prototype    : http_update_host_status
*   Description  : tell the client that the wifi module is updating host
*   Input        : AsyncWebServerRequest *request
*   Output       : None
*   Return Value : void
*   Calls        :
*   Called By    :
*
*   History:
*
*       1.  Date         : 2019/7/4
*           Author       : zhuhongfei
*           Modification : Created function
*
*****************************************************************************/
void http_update_host_status(AsyncWebServerRequest *request)
{
    String html = "";
    html += "<html> <head><meta http-equiv='Content-Type' content='text/html ;charset=utf-8'/></head><body>";
    html = html + "<h1>" + zh_array[updating_host] + "</h1> ";

    html  = html + "<h2>" + zh_array[update_log] + "</h2><br>";
    html += "<form method='GET' action='/host'>";
    html = html +  "<input type='submit' value=" + zh_array[jump_out] + ">";
    html += "</form></body></html>";
    request->send(200, "text/html", html.c_str());
}
/*****************************************************************************
*   Prototype    : http_handle_update_esp8266
*   Description  : handle esp8266 update
*   Input        : AsyncWebServerRequest *request
*                  const String& filename
*                  size_t index
*                  uint8_t *data
*                  size_t len
*                  bool final
*   Output       : None
*   Return Value : void
*   Calls        :
*   Called By    :
*
*   History:
*
*       1.  Date         : 2019/6/9
*           Author       : zhuhongfei
*           Modification : Created function
*
*****************************************************************************/
void http_handle_update_esp8266(AsyncWebServerRequest *request,
                                const String& filename,
                                size_t index, uint8_t *data, size_t len, bool final)
{
    if (!index)
    {
#if DEBUG_KEY
        DEBUG_UART_PRINTF("Update Start: %s\n", filename.c_str());
#endif

        Update.runAsync(true);
        if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000))
        {
            Update.printError(Serial);
        }
    }
    if (!Update.hasError())
    {
        if (Update.write(data, len) != len)
        {
            Update.printError(Serial);
        }
    }
    if (final)
    {

        if (Update.end(true))
        {
#if DEBUG_KEY
            DEBUG_UART_PRINTF("Update Success: %uB\n", index + len);
            DEBUG_UART_PRINTLN("Update complete");
#endif

            Serial.flush();
            //ESP.restart();
        }
        else
        {
            Update.printError(Serial);
        }
    }


}

/*****************************************************************************
*   Prototype    : http_handle_update_stm32
*   Description  : update stm32 process
*   Input        : AsyncWebServerRequest *request
*                  const String& filename
*                  size_t index
*                  uint8_t *data
*                  size_t len
*                  bool final
*   Output       : None
*   Return Value : void
*   Calls        :
*   Called By    :
*
*   History:
*
*       1.  Date         : 2019/6/24
*           Author       : zhuhongfei
*           Modification : Created function
*
*****************************************************************************/
void http_handle_update_stm32(AsyncWebServerRequest *request,
                              const String& filename,
                              size_t index, uint8_t *data, size_t len, bool final)
{

    if (!index)
    {
#if DEBUG_KEY
        DEBUG_UART_PRINTF("\nSave stm32 update file Start : %s\n", filename.c_str());
#endif

        savefile = SPIFFS.open(save_update_file, "w+");
        savefile.flush();         //flush file before write
        update_fs2uart_start = 0; //close send data when save file
        update_data_index  = 0;
        update_data_byte_sum = 0;

        Update.runAsync(true);
    }
    if (!Update.hasError())
    {
        //DEBUG_UART_PRINTF("\n*********current len:%u*********\n",len);
        //DEBUG_UART_PRINTF("\n*********current index:%u*********\n",index);

        for (; update_data_index < index + len; update_data_index++)
        {
            if ((update_data_index >= 0) && (update_data_index < 3))
            {
                //the first three bytes of the file indicate the total number of data
                update_data_byte_sum |= ((uint32_t)(*(data + update_data_index))) <<
                                        (16 - 8 * update_data_index);
                //continue;
            }
            if (savefile)
            {
                savefile.write(*(data + update_data_index - index));
            }
            //if ((update_data_index % SERIAL_DATA_LENS) == 0)
            //{
            //    update_package_num++;
            //}

        }

    }
    if (final)
    {
        savefile.close();
#if DEBUG_KEY
        DEBUG_UART_PRINTLN("Save complete");
        DEBUG_UART_PRINTF("file size:%uB\n", index + len);
#endif

        if (((update_data_byte_sum + 2) % UPDATE_DATA_LENS) ==
                0) //SERIAL_DATA_LENS -> UPDATE_DATA_LENS
        {
            update_package_sum = ((update_data_byte_sum + 2) / UPDATE_DATA_LENS) +
                                 1; //SERIAL_DATA_LENS -> UPDATE_DATA_LENS
        }
        else
        {
            update_package_sum = ((update_data_byte_sum + 2) / UPDATE_DATA_LENS) +
                                 2; //SERIAL_DATA_LENS -> UPDATE_DATA_LENS
        }
        //update_package_num = 0;
        get_file_system_msg();

        //DEBUG_UART_PRINTLN("close wifi ap mode");
        //WiFi.softAPdisconnect(true);

    }

}
/*****************************************************************************
*   Prototype    : save_config_file
*   Description  : save config data to spiffs
*   Input        : None
*   Output       : None
*   Return Value : bool
*   Calls        :
*   Called By    :
*
*   History:
*
*       1.  Date         : 2019/6/11
*           Author       : zhuhongfei
*           Modification : Created function
*
*****************************************************************************/
bool save_config_file()
{
    File configFile = SPIFFS.open(configFileName, "w");
    if (!configFile)
    {
#if DEBUG_KEY
        DEBUG_UART_PRINTLN("failed to open config file for writing");
#endif

        return false;
    }

    if (serializeJson(doc, configFile) == 0)
    {
#if DEBUG_KEY
        DEBUG_UART_PRINTLN("failed to save config file");
#endif

        configFile.close();
        return false;
    }

    configFile.close();
    return true;
}

/*****************************************************************************
*   Prototype    : load_ap_params
*   Description  : load esp8266 wifi-ap ssid and password from json
*   Input        : None
*   Output       : None
*   Return Value : void
*   Calls        :
*   Called By    :
*
*   History:
*
*       1.  Date         : 2019/6/13
*           Author       : zhuhongfei
*           Modification : Created function
*
*****************************************************************************/
void load_ap_params()
{
    //char tmp_ssid[16];
    //char tmp_pwd[16];
#if DEBUG_KEY
    DEBUG_UART_PRINT("load file:");
    DEBUG_UART_PRINTLN(configFileName);
#endif
    if (SPIFFS.exists(configFileName))
    {
        File configFile = SPIFFS.open(configFileName, "r");

        if (configFile)
        {
            DeserializationError error = deserializeJson(doc, configFile);

            if (error)
            {
#if DEBUG_KEY
                DEBUG_UART_PRINTF("Failed to read file, using default configuration %s\n",
                                  error.c_str());
#endif
                configFile.close();
                return ;
            }

            const char *ssid = doc["ap_ssid"];
            if (NULL != ssid)
            {
                strlcpy(&ap_ssid[0], ssid, 17);
#if DEBUG_KEY
                DEBUG_UART_PRINT("get_ssid:");
                DEBUG_UART_PRINTLN(ap_ssid);
#endif
            }
            const char *pwd = doc["ap_pwd"];
            if (NULL != pwd)
            {
                strlcpy(&ap_pwd[0], pwd, 17);
#if DEBUG_KEY
                DEBUG_UART_PRINT("get_pwd:");
                DEBUG_UART_PRINTLN(ap_pwd);
#endif
            }
            configFile.close();
        }
    }
}

/*****************************************************************************
*   Prototype    : load_config_params
*   Description  : load config data
*   Input        : None
*   Output       : None
*   Return Value : void
*   Calls        :
*   Called By    :
*
*   History:
*
*       1.  Date         : 2019/6/16
*           Author       : zhuhongfei
*           Modification : Created function
*
*****************************************************************************/
void load_config_params()
{

    if (SPIFFS.exists(configFileName))
    {
        File configFile = SPIFFS.open(configFileName, "r");

        if (configFile)
        {
            DeserializationError error = deserializeJson(doc, configFile);

            if (error)
            {
#if DEBUG_KEY
                DEBUG_UART_PRINTF("Failed to read file, using default configuration %s\n",
                                  error.c_str());
#endif
                configFile.close();
                return ;
            }

            const char *ip1 = doc["IP1"];
            if (NULL != ip1)
            {
                strlcpy(&NB_lot_ip1[0], ip1, 4);
#if DEBUG_KEY
                DEBUG_UART_PRINT("get_ip1:");
                DEBUG_UART_PRINTLN(NB_lot_ip1);
#endif
            }
            const char *ip2 = doc["IP2"];
            if (NULL != ip2)
            {
                strlcpy(&NB_lot_ip2[0], ip2, 4);
#if DEBUG_KEY
                DEBUG_UART_PRINT("get_ip2:");
                DEBUG_UART_PRINTLN(NB_lot_ip2);
#endif
            }
            const char *ip3 = doc["IP3"];
            if (NULL != ip3)
            {
                strlcpy(&NB_lot_ip3[0], ip3, 4);
#if DEBUG_KEY
                DEBUG_UART_PRINT("get_ip3:");
                DEBUG_UART_PRINTLN(NB_lot_ip3);
#endif

            }
            const char *ip4 = doc["IP4"];
            if (NULL != ip4)
            {
                strlcpy(&NB_lot_ip4[0], ip4, 4);
#if DEBUG_KEY
                DEBUG_UART_PRINT("get_ip4:");
                DEBUG_UART_PRINTLN(NB_lot_ip4);
#endif
            }
            const char *port = doc["port"];
            if (NULL != port)
            {
                strlcpy(&NB_lot_port[0], port, 8);
#if DEBUG_KEY
                DEBUG_UART_PRINT("get_port:");
                DEBUG_UART_PRINTLN(NB_lot_port);
#endif
            }
            const char *angle = doc["angle"];
            if (NULL != angle)
            {
                strlcpy(&Adx1_ang[0], angle, 4);
#if DEBUG_KEY
                DEBUG_UART_PRINT("get_angle:");
                DEBUG_UART_PRINTLN(Adx1_ang);
#endif
            }
            const char *dep_max_get = doc["depth_max_save"];
            if (NULL != dep_max_get)
            {
                strlcpy(&depth_max_cfg[0], dep_max_get, 4);
#if DEBUG_KEY
                DEBUG_UART_PRINT("get_depth_max:");
                DEBUG_UART_PRINTLN(depth_max_cfg);
#endif
            }
            const char *dep_min_get = doc["depth_min_save"];
            if (NULL != dep_min_get)
            {
                strlcpy(&depth_min_cfg[0], dep_min_get, 4);
#if DEBUG_KEY
                DEBUG_UART_PRINT("get_depth_min:");
                DEBUG_UART_PRINTLN(depth_min_cfg);
#endif
            }
            const char *temper = doc["temper"];
            if (NULL != temper)
            {
                strlcpy(&Tempera[0], temper, 8);
#if DEBUG_KEY
                DEBUG_UART_PRINT("get_temper:");
                DEBUG_UART_PRINTLN(Tempera);
#endif
            }
            const char *nbc = doc["start_nb"];
            if (NULL != nbc)
            {
                strlcpy(&Nb_on[0], nbc, 4);
#if DEBUG_KEY
                DEBUG_UART_PRINT("get_nbc:");
                DEBUG_UART_PRINTLN(Nb_on);
#endif
            }
            const char *gpsc = doc["start_gps"];
            if (NULL != gpsc)
            {
                strlcpy(&GPS_on[0], gpsc, 4);
#if DEBUG_KEY
                DEBUG_UART_PRINT("get_gpsc:");
                DEBUG_UART_PRINTLN(GPS_on);
#endif
            }
            const char *gps_cycle = doc["gps_cycle"];
            if (NULL != gps_cycle)
            {
                strlcpy(&GPS_update_cycle[0], gps_cycle, 4);
#if DEBUG_KEY
                DEBUG_UART_PRINT("get_gps_cycle:");
                DEBUG_UART_PRINTLN(GPS_update_cycle);
#endif
            }
            const char *data_cycle = doc["data_cycle"];
            if (NULL != data_cycle)
            {
                strlcpy(&Data_update_cycle[0], data_cycle, 4);
#if DEBUG_KEY
                DEBUG_UART_PRINT("get_data_cycle:");
                DEBUG_UART_PRINTLN(Data_update_cycle);
#endif
            }

            configFile.close();
        }
    }
}

/*****************************************************************************
*   Prototype    : load_NB_update_msg
*   Description  : load msg(current update data index and update data sum
                   bytes ) about NB update host
*   Input        : None
*   Output       : None
*   Return Value : bool
*   Calls        :
*   Called By    :
*
*   History:
*
*       1.  Date         : 2019/8/5
*           Author       : zhuhongfei
*           Modification : Created function
*
*****************************************************************************/
bool load_NB_update_msg()
{
    if (SPIFFS.exists(configFileName))
    {
        File configFile = SPIFFS.open(configFileName, "r");

        if (configFile)
        {
            DeserializationError error = deserializeJson(doc, configFile);

            if (error)
            {
#if DEBUG_KEY
                DEBUG_UART_PRINTF("Failed to read file, using default configuration %s\n",
                                  error.c_str());
#endif
                configFile.close();
                return false;
            }
            if (doc["NB_update_index"] && doc["NB_update_bytes"] &&
                    doc["NB_update_data_crc16"])
            {
                update_data_index    = doc["NB_update_index"];
                update_data_byte_sum = doc["NB_update_bytes"];
                update_calc_crc      = doc["NB_update_data_crc16"];
#if DEBUG_KEY
                DEBUG_UART_PRINTF("NB_update_index:%d\n", update_data_index);
                DEBUG_UART_PRINTF("NB_update_bytes:%d\n", update_data_byte_sum);
                DEBUG_UART_PRINTF("NB_update_data_crc16:%x\n", update_calc_crc);
#endif
                configFile.close();
                return true;
            }
            else
            {
                configFile.close();
                return false;
            }
        }
    }
    return false;
}
/*****************************************************************************
*   Prototype    : send_cmd_to_host
*   Description  : send cmd id to host
*   Input        : uint8_t cmd_id
*   Output       : None
*   Return Value : void
*   Calls        :
*   Called By    :
*
*   History:
*
*       1.  Date         : 2019/6/30
*           Author       : zhuhongfei
*           Modification : Created function
*
*****************************************************************************/
void send_cmd_to_host(uint8_t cmd_id)
{
    uint8_t check_sum = 0;
    Serial.write(HEADER);

    Serial.write(cmd_id);

    if (cfg_set == cmd_id)
    {
        Serial.write(STATUS_DATA_BYTE);
        for (uint8_t i = 0; i < CONFIG_DATA_BYTE; i++)
        {
            //DEBUG_UART_PRINTF("%d--%d\n",i,save_cfg_data[i]);
            check_sum += save_cfg_data[i];
            Serial.write(save_cfg_data[i]);
        }
        for (uint8_t i = 0; i < (STATUS_DATA_BYTE - CONFIG_DATA_BYTE); i++)
        {
            Serial.write(0);
        }
        Serial.write(check_sum);
        for (uint8_t i = 0; i < PADDING_DATA_BYTE; i++)
        {
            Serial.write(0);
        }
    }
    else if (host_record_log == cmd_id)
    {
        Serial.write(record_host_log);
        for (uint8_t i = 0; i < SERIAL_DATA_LENS - 4; i++)
        {
            Serial.write(0);
        }

    }
    else
    {
        Serial.write(STATUS_DATA_BYTE);
        for (uint8_t i = 0; i < SERIAL_DATA_LENS - 4; i++)
        {
            Serial.write(0);
        }
    }

    Serial.write(ENDUP);
    Serial.flush();
}
/*****************************************************************************
*   Prototype    : http_main_page
*   Description  : enter 192.168.4.1 ,select function html
*   Input        : None
*   Output       : None
*   Return Value : String
*   Calls        :
*   Called By    :
*
*   History:
*
*       1.  Date         : 2019/6/16
*           Author       : zhuhongfei
*           Modification : Created function
*
*****************************************************************************/
String http_main_page()
{
    String html = "";
    html += "<html> <head><meta http-equiv='Content-Type' content='text/html ;charset=utf-8'/> ";
    html = html + "<style>" +
           "#head{background-color:white;color:black;text-align:center;padding:1px;font-size:30px;}"
           +
           "#show{background-color:#EEF9F5;padding:20px;text-align:center;}" +
           "input{height:35px;line-height:35px;font-size:25px;} " +
           "#footer {background-color:#557DF0;color:white;clear:both;text-align:center;font-size:20px;padding:5px;}"
           + "</style></head><body>";
    html = html + "<div id='head'><h1>" + zh_array[sel_page] + "</h1></div>";
    html += "<div id='show'>";
    html += "<form method='GET' action='/cfg' id='cfg'>";
    html = html +  "<input type='submit' value=" + zh_array[cfg_page] +
           "></form><br>";
    html += "<form method='GET' action='/sta' id='sta'>";
    html = html +  "<input type='submit' value=" + zh_array[sta_page] +
           "></form><br>";

    html += "<form method='GET' action='/cfgwifi' id='cfgwifi'>";
    html = html +  "<input type='submit' value=" + zh_array[cfgwifi_page] +
           "></form><br>";
    html += "<form method='GET' action='/update' id='update'>";
    html = html +  "<input type='submit' value=" + zh_array[update_page] +
           "></form><br>";

    html += "<form method='GET' action='/sel' id='sel'>";
    html = html +  "<input type='submit' value=" + zh_array[sel_function] +
           "></form><br>";
    html += "</div>";
    html += "</div><div id='footer'>Telchina</div></body></html>";
    return html;
}

/*****************************************************************************
*   Prototype    : http_status_page
*   Description  : /sta send sensor data to html
*   Input        : None
*   Output       : None
*   Return Value : String
*   Calls        :
*   Called By    :
*
*   History:
*
*       1.  Date         : 2019/6/11
*           Author       : zhuhongfei
*           Modification : Created function
*
*****************************************************************************/
String http_status_page()
{
    String html = "";
    String status = "";
    String sign = "";
    String gpsc = "";
    char lat_dd[4];
    char lat_ff[4];
    char lat_mm[4];
    String lat_dir = "";
    char log_dd[4];
    char log_ff[4];
    char log_mm[4];
    String log_dir = "";

    char nbq[8];
    char gpsq[4];
    char temper[4];
    char timer_hh[4];
    char timer_mm[4];
    char timer_ss[4];
    char timer_ms[4];

    char voltage[8];
    char angle[4];
    char depth[8];


    switch (save_sta_data[work_status])
    {
        case 0x00:
            status = zh_array[no_log_no_scan];
            break;
        case 0x01:
            status = zh_array[log_local_net];
            break;
        case 0x02:
            status = zh_array[no_log_but_scan];
            break;
        case 0x03:
            status = zh_array[reg_reject];
            break;
        case 0x04:
            status = zh_array[unknow_status];
            break;
        case 0x05:
            status = zh_array[log_and_roaming];
            break;
        case 0x06:
            status = zh_array[reg_msg];
            break;
        case 0x07:
            status = zh_array[reg_msg_roaming];
            break;
        default:

            break;
    }
    switch (save_sta_data[online])
    {
        case 0x00:
            sign = zh_array[tele_com];
            break;
        case 0x01:
            sign = zh_array[unicom];
            break;
        case 0x02:
            sign = zh_array[mobile];
            break;
        default:

            break;
    }
    switch (save_sta_data[gps_loca_sta])
    {
        case 0x00:
            gpsc = zh_array[no_target];
            break;
        case 0x01:
            gpsc = zh_array[sps_target];
            break;
        case 0x02:
            gpsc = zh_array[dsps_target];
            break;

        default:
            gpsc = zh_array[no_target];
            break;
    }
    switch (save_sta_data[lat_dir_index])
    {
        case 0x00:
            lat_dir = "N";
            break;
        case 0x01:
            lat_dir = "S";
            break;
        default:
            break;
    }
    switch (save_sta_data[log_dir_index])
    {
        case 0x00:
            log_dir = "E";
            break;
        case 0x01:
            log_dir = "W";
            break;
        default:
            break;
    }
    sprintf(lat_dd, "%d", save_sta_data[lat_dd_index]);
    sprintf(lat_ff, "%d", save_sta_data[lat_ff_index]);
    sprintf(lat_mm, "%d", save_sta_data[lat_mm_index]);
    sprintf(log_dd, "%d", save_sta_data[log_dd_index]);
    sprintf(log_ff, "%d", save_sta_data[log_ff_index]);
    sprintf(log_mm, "%d", save_sta_data[log_mm_index]);

    sprintf(nbq, "%d", (int8_t)save_sta_data[nb_strength]);
    sprintf(gpsq, "%d", save_sta_data[gps_strength]);
    sprintf(temper, "%d", save_sta_data[temper_sta]);

    sprintf(timer_hh, "%d", save_sta_data[time_hh_index]);
    sprintf(timer_mm, "%d", save_sta_data[time_mm_index]);
    sprintf(timer_ss, "%d", save_sta_data[time_ss_index]);
    sprintf(timer_ms, "%d", save_sta_data[time_ms_index]);

    sprintf(voltage, "%0.3f",
            (float)(((uint16_t)save_sta_data[volt_high] << 8) | save_sta_data[volt_low]) /
            1000);
    sprintf(angle, "%d", save_sta_data[angle_sta]);
    sprintf(depth, "%0.3f", (float)(((uint16_t)save_sta_data[depth_high] << 8) |
                                    save_sta_data[depth_low]) / 1000);
    //depth.c_str()=depth_tmp;
    html = html +
           "<html> <head><meta http-equiv='Content-Type' content='text/html ;charset=utf-8'/> ";
    html = html + "<style>" +
           "#logo{padding:10px,0px,10px,10px;float:left;width='200';height='120';} " +
           "#head{background-color:white;color:black;text-align:center;padding:1px;font-size:30px;}"
           +
           "#show{background-color:#EEF9F5;padding:20px; font-size:30px;} " +
           "label{cursor: pointer; display: inline-block;" +
           "text-align: left;width: 220px;vertical-align: top;}" +
           "input{height:35px;line-height:35px;font-size:25px;} " +
           "#ipin{ width:50px;height:35px;} " +
           "span{color:red;font-size:20px;} " +
           "#footer {background-color:#557DF0;color:white;clear:both;text-align:center;font-size:20px;padding:5px;}"
           + "</style></head>";
    //html = html + "<body><div id='logo'><img src="+"SPIFFS.open('/logo.jpg', 'r')"+"></div>";
    html = html + "<div id='head'><h1>" + zh_array[sta_page] + "</h1></div>";

    html = html + "<div id='show'>";
    html = html +
           "<p><label>" + zh_array[work_sta] +
           ":</label><input name='con' type='text' disabled='disabled' value="
           +
           status  + "><br></p>";
    html = html +
           "<p><label>" + zh_array[net_msg] +
           ":</label><input name='idt' type='text' disabled='disabled' value="
           +
           sign    + "><br></p>";
    html = html +
           "<p><label>" + zh_array[gnss_target] +
           ":</label><input name='gpsd' type='text' disabled='disabled' value="
           +
           lat_dd + "°" + lat_ff + "'" + lat_mm + "\"" + lat_dir + "," + log_dd + "°" +
           log_ff + "'" + log_mm + "\"" + log_dir + "><br></p>";
    html = html +
           "<p><label>" + zh_array[gnss_target_status] +
           ":</label><input name='gpsc' type='text' disabled='disabled' value="
           +
           gpsc  + "><br></p>";
    html = html +
           "<p><label>" + zh_array[nb_lot_strength] +
           ":</label><input name='nbq' type='text' disabled='disabled' value="
           +
           nbq    + ">dBm<br></p>";
    html = html +
           "<p><label>" + zh_array[gnss_strength] +
           ":</label><input name='gpsq' type='text' disabled='disabled' value="
           +
           gpsq + ">" + zh_array[unit_ke] + "<br></p>";
    html = html +
           "<p><label>" + zh_array[real_time_temper] +
           ":</label><input name='temper' type='text' disabled='disabled' value="
           +
           temper + ">℃<br></p>";
    html = html +
           "<p><label>" + zh_array[time_now] +
           ":</label><input name='time' type='text' disabled='disabled' value="
           +
           timer_hh + ":" + timer_mm + ":" + timer_ss + "." + timer_ms + "><br></p>";
    html = html +
           "<p><label>" + zh_array[battery_volt] +
           ":</label><input name='vol' type='text' disabled='disabled' value="
           +
           voltage + ">V<br></p>";
    html = html +
           "<p><label>" + zh_array[angle_now] +
           ":</label><input name='angle' type='text' disabled='disabled' value="
           +
           angle  + ">°<br></p>";
    html = html +
           "<p><label>" + zh_array[depth_now] +
           ":</label><input name='depth' type='text' disabled='disabled' value="
           +
           depth  + ">m<br></p>";

    html = html +  "<p><br><input type='button' value=" + zh_array[flush_post] +
           " style='width:100px;height:40px;' onclick='window.location.reload()'<br></p>";

    html += "</div><div id='footer'>Telchina</div></body></html>";

    return html;
}


/*****************************************************************************
*   Prototype    : http_view_host_log_page
*   Description  : view host running log from eeprom
*   Input        :
*   Output       : None
*   Return Value : String
*   Calls        :
*   Called By    :
*
*   History:
*
*       1.  Date         : 2019/7/25
*           Author       : zhuhongfei
*           Modification : Created function
*
*****************************************************************************/
String http_view_host_log_page()
{
    String html   = "";
    String status = "";
    String sign   = "";
    String gpsc   = "";
    char lat_dd[4];
    char lat_ff[4];
    char lat_mm[4];
    String lat_dir = "";
    char log_dd[4];
    char log_ff[4];
    char log_mm[4];
    String log_dir = "";

    char nbq[8];
    char gpsq[4];
    char temper[4];
    char timer_hh[4];
    char timer_mm[4];
    char timer_ss[4];
    char timer_ms[4];

    char date_year[5];
    char date_mon[4];
    char date_day[4];

    char voltage[8];
    char angle[4];
    char depth[8];

    char record_log_num[4];

    switch (save_log_data[work_status_sub])
    {
        case 0x00:
            status = zh_array[no_log_no_scan];
            break;
        case 0x01:
            status = zh_array[log_local_net];
            break;
        case 0x02:
            status = zh_array[no_log_but_scan];
            break;
        case 0x03:
            status = zh_array[reg_reject];
            break;
        case 0x04:
            status = zh_array[unknow_status];
            break;
        case 0x05:
            status = zh_array[log_and_roaming];
            break;
        case 0x06:
            status = zh_array[reg_msg];
            break;
        case 0x07:
            status = zh_array[reg_msg_roaming];
            break;
        default:

            break;
    }
    switch (save_log_data[online_sub])
    {
        case 0x00:
            sign = zh_array[tele_com];
            break;
        case 0x01:
            sign = zh_array[unicom];
            break;
        case 0x02:
            sign = zh_array[mobile];
            break;
        default:

            break;
    }
    switch (save_log_data[gps_sta_sub])
    {
        case 0x00:
            gpsc = zh_array[no_target];
            break;
        case 0x01:
            gpsc = zh_array[sps_target];
            break;
        case 0x02:
            gpsc = zh_array[dsps_target];
            break;

        default:
            gpsc = zh_array[no_target];
            break;
    }
    switch (save_log_data[lat_dir_sub])
    {
        case 0x00:
            lat_dir = "N";
            break;
        case 0x01:
            lat_dir = "S";
            break;
        default:
            break;
    }
    switch (save_log_data[log_dir_sub])
    {
        case 0x00:
            log_dir = "E";
            break;
        case 0x01:
            log_dir = "W";
            break;
        default:
            break;
    }
    sprintf(lat_dd, "%d", save_log_data[lat_dd_sub]);
    sprintf(lat_ff, "%d", save_log_data[lat_ff_sub]);
    sprintf(lat_mm, "%d", save_log_data[lat_mm_sub]);
    sprintf(log_dd, "%d", save_log_data[log_dd_sub]);
    sprintf(log_ff, "%d", save_log_data[log_ff_sub]);
    sprintf(log_mm, "%d", save_log_data[log_mm_sub]);

    sprintf(nbq, 	"%d", (int8_t)save_log_data[nb_strength_sub]);
    sprintf(gpsq, 	"%d", save_log_data[gps_strength_sub]);
    sprintf(temper, "%d", save_log_data[temper_sta_sub]);

    sprintf(timer_hh, "%d", save_log_data[time_hh_sub]);
    sprintf(timer_mm, "%d", save_log_data[time_mm_sub]);
    sprintf(timer_ss, "%d", save_log_data[time_ss_sub]);
    sprintf(timer_ms, "%d", save_log_data[time_ms_sub]);

    sprintf(date_year, "20%d", save_log_data[date_year_sub]);
    sprintf(date_mon, "%d", save_log_data[date_mon_sub]);
    sprintf(date_day, "%d", save_log_data[date_day_sub]);

    sprintf(voltage, "%0.3f",
            (float)(((uint16_t)save_log_data[volt_high_sub] << 8) |
                    save_log_data[volt_low_sub]) /
            1000);
    sprintf(angle, "%d", save_log_data[angle_sta_sub]);
    sprintf(depth, "%0.3f", (float)(((uint16_t)save_log_data[depth_high_sub] << 8) |
                                    save_log_data[depth_low_sub]) / 1000);

    sprintf(record_log_num, "%d", save_log_data[record_num_sub]);

    html = html +
           "<html> <head><meta http-equiv='Content-Type' content='text/html ;charset=utf-8'/> ";
    html = html + "<style>" +
           "#logo{padding:10px,0px,10px,10px;float:left;width='200';height='120';} " +
           "#head{background-color:white;color:black;text-align:center;padding:1px;font-size:30px;}"
           +
           "#show{background-color:#EEF9F5;padding:20px; font-size:30px;} " +
           "label{cursor: pointer; display: inline-block;" +
           "text-align: left;width: 220px;vertical-align: top;}" +
           "input{height:35px;line-height:35px;font-size:25px;} " +
           "#ipin{ width:50px;height:35px;} " +
           "span{color:red;font-size:20px;} " +
           "#footer {background-color:#557DF0;color:white;clear:both;text-align:center;font-size:20px;padding:5px;}"
           + "</style></head>";

    html = html + "<div id='head'><h1>" + zh_array[record_log] + "</h1></div>";

    html = html + "<div id='show'>";
    html += "<form method='GET' action='/log'>";
    html = html +
           "<p><label>" + zh_array[work_sta] +
           ":</label><input name='con' type='text' disabled='disabled' value="
           +
           status  + "><br></p>";
    html = html +
           "<p><label>" + zh_array[net_msg] +
           ":</label><input name='idt' type='text' disabled='disabled' value="
           +
           sign    + "><br></p>";
    html = html +
           "<p><label>" + zh_array[gnss_target] +
           ":</label><input name='gpsd' type='text' disabled='disabled' value="
           +
           lat_dd + "°" + lat_ff + "'" + lat_mm + "\"" + lat_dir + "," + log_dd + "°" +
           log_ff + "'" + log_mm + "\"" + log_dir + "><br></p>";
    html = html +
           "<p><label>" + zh_array[gnss_target_status] +
           ":</label><input name='gpsc' type='text' disabled='disabled' value="
           +
           gpsc  + "><br></p>";
    html = html +
           "<p><label>" + zh_array[nb_lot_strength] +
           ":</label><input name='nbq' type='text' disabled='disabled' value="
           +
           nbq    + ">dBm<br></p>";
    html = html +
           "<p><label>" + zh_array[gnss_strength] +
           ":</label><input name='gpsq' type='text' disabled='disabled' value="
           +
           gpsq + ">" + zh_array[unit_ke] + "<br></p>";
    html = html +
           "<p><label>" + zh_array[real_time_temper] +
           ":</label><input name='temper' type='text' disabled='disabled' value="
           +
           temper + ">℃<br></p>";
    html = html +
           "<p><label>" + zh_array[time_now] +
           ":</label><input name='time' type='text' disabled='disabled' value="
           +
           timer_hh + ":" + timer_mm + ":" + timer_ss + "." + timer_ms + "><br></p>";
    html = html +
           "<p><label>" + zh_array[date_now] +
           ":</label><input name='date' type='text' disabled='disabled' value="
           +
           date_year + "/" + date_mon + "/" + date_day + "><br></p>";

    html = html +
           "<p><label>" + zh_array[battery_volt] +
           ":</label><input name='vol' type='text' disabled='disabled' value="
           +
           voltage + ">V<br></p>";
    html = html +
           "<p><label>" + zh_array[angle_now] +
           ":</label><input name='angle' type='text' disabled='disabled' value="
           +
           angle  + ">°<br></p>";
    html = html +
           "<p><label>" + zh_array[depth_now] +
           ":</label><input name='depth' type='text' disabled='disabled' value="
           +
           depth  + ">m<br></p>";
    html = html +
           "<p><label>" + zh_array[record_num_now] +
           ":</label><input name='record' type='text' disabled='disabled' value="
           +
           record_log_num  + "><br></p>";
    html = html + "<p><label>" + zh_array[jump_log_num] +
           ":</label><input name='jump_record' type='number'  min='1' max='56' required='required' value="
           +
           record_host_log  + ">&emsp;";

    html = html +  "<input type='submit' value=" + zh_array[send_post] +
           " style='width:100px;height:38px;' ><br></p>";
    html = html +  "<p><input type='button' value=" + zh_array[flush_post] +
           " style='width:100px;height:40px;' onclick='window.location.reload()'><br></p>";
    html += "</form></div><div id='footer'>Telchina</div></body></html>";
    //record_host_log ++;
    //if(record_host_log == 101)
    //{
    //	record_host_log =0;
    //}

    return html;
}


/*****************************************************************************
*   Prototype    : http_config_page
*   Description  : /cfg html
*   Input        : None
*   Output       : None
*   Return Value : String
*   Calls        :
*   Called By    :
*
*   History:
*
*       1.  Date         : 2019/6/16
*           Author       : zhuhongfei
*           Modification : Created function
*
*****************************************************************************/
String http_config_page()
{
    String html = "";

    //load_config_params();

    html = html +
           "<html> <head><meta http-equiv='Content-Type' content='text/html ;charset=utf-8'/> ";
    html = html + "<style>" +
           "#logo{padding:10px,0px,10px,10px;float:left;width='200';height='120';} " +
           "#head{background-color:white;color:black;text-align:center;padding:1px;font-size:30px;}"
           +
           "#show{background-color:#EEF9F5;padding:20px; font-size:30px;} " +
           "label{cursor: pointer; display: inline-block;" +
           "text-align: left;width: 220px;vertical-align: top;}" +
           "input[type='number']{height:35px;line-height:35px;font-size:25px;} " +
           "input[type='text']{height:35px;line-height:35px;font-size:25px;} " +
           "input[type='radio']{width:20px;height:20px;} " +
           "#ipin{ width:60px;height:35px;} " +
           "span{color:red;font-size:20px;} " +
           "#footer {background-color:#557DF0;color:white;clear:both;text-align:center;font-size:20px;padding:5px;}"
           + "</style></head><body>";
    html = html + "<body><div id='logo'><img src='data/img/logo.jpg'></div>";
    html = html + "<div id='head'><h1>" + zh_array[cfg_page] + "</h1></div>";
    html = html + "<div id='show'>";
    html += "<form method='POST' action='/cfg'>";
    html = html +
           "<p><label>" + zh_array[nb_remote_ip] +
           ":</label><input name='IP1' type='number' min='1' max='255' required='required' id='ipin' value="
           +
           NB_lot_ip1  + ">.";
    html = html +
           "<input name='IP2' type='number'  min='0' max='255' required='required'  id='ipin' value="
           +
           NB_lot_ip2  + ">.";
    html = html +
           "<input name='IP3' type='number'  min='0' max='255' required='required' id='ipin' value="
           +
           NB_lot_ip3  +
           ">.";
    html = html +
           "<input name='IP4' type='number'  min='0' max='255' required='required' id='ipin' value="
           +
           NB_lot_ip4  +
           "><br></p>";
    html = html +
           "<p><label>" + zh_array[nb_remote_port] +
           ":</label><input name='port' type='number'  min='1' max='65535'  required='required'  value="
           +
           NB_lot_port  + "><span>(" + zh_array[range] + ":1~65535)</span><br></p>";
    html = html +
           "<p><label>" + zh_array[angth_threshold] +
           ":</label><input name='angle' type='number'  min='0' max='90' required='required' value="
           +
           Adx1_ang  + ">°<span> (" + zh_array[range] + ":0~90°)</span><br></p>";
    html = html +
           "<p><label>" + zh_array[depth_max_th] +
           ":</label><input name='max' type='number'  min='80' max='200' required='required' value="
           +
           depth_max_cfg  + ">cm <span> (" + zh_array[range] + ":80~200cm)</span><br></p>";
    html = html +
           "<p><label>" + zh_array[depth_min_th] +
           ":</label><input name='min' type='number'  min='20' max='50' required='required' value="
           +
           depth_min_cfg  + ">cm <span> (" + zh_array[range] + ":20~50cm)<br></p>";
    html = html +
           "<p><label>" + zh_array[temper_threshold] +
           ":</label><input name='temp' type='number'  min='45' max='85' required='required' value="
           +
           Tempera
           + ">℃ <span> (" + zh_array[range] + ":45~85℃)</span><br></p>";
    if (!strcmp(Nb_on, "1"))
    {
        html = html +
               "<p><label>" + zh_array[enable_nb] +
               ":</label><input name='nb' type='radio'  value='1' checked='checked'>" +
               zh_array[yes];
        html = html + "<input name='nb' type='radio'  value='0'>" + zh_array[no] +
               "<br></p>";
    }
    else
    {
        html = html +
               "<p><label>" + zh_array[enable_nb] +
               ":</label><input name='nb' type='radio'  value='1' >" + zh_array[yes];
        html = html +
               "<input name='nb' type='radio'  value='0' checked='checked'>" + zh_array[no] +
               "<br></p>";
    }
    if (!strcmp(GPS_on, "1"))
    {
        html = html +
               "<p><label>" + zh_array[enable_gps] +
               ":</label><input name='gps' type='radio'  value='1' checked='checked'>" +
               zh_array[yes];
        html = html + "<input name='gps' type='radio'  value='0'>" + zh_array[no] +
               "<br></p>";
    }
    else
    {
        html = html +
               "<p><label>" + zh_array[enable_gps] +
               ":</label><input name='gps' type='radio'  value='1' >" + zh_array[yes];
        html = html +
               "<input name='gps' type='radio'  value='0' checked='checked'>" + zh_array[no] +
               "<br></p>";
    }

    html = html +
           "<p><label>" + zh_array[gps_report_cycle] +
           ":</label><input name='gc' type='number' min='0' max='255' required='required' value="
           +
           GPS_update_cycle  + ">h<br></p>";
    html = html +
           "<p><label>" + zh_array[data_report_cycle] +
           ":</label><input name='dc' type='number'min='0' max='255' required='required' value="
           +
           Data_update_cycle  + ">h<br></p>";
    html = html +
           "<p><label>" + zh_array[host_connect] +
           ":</label><input name='source' type='text' disabled='disabled'  value="
           +
           data_source  + "><br></p>";
    html = html +  "<p><br><input type='submit' value=" + zh_array[send_post] +
           " style='width:100px;height:40px;'>";
    html = html +  "<input type='button' value=" + zh_array[flush_post] +
           " style='width:100px;height:40px;' onclick='window.location.reload()'><br></p>";

    html += "</form></div><div id='footer'>Telchina</div></body></html>";

    return html;
}

/*****************************************************************************
*   Prototype    : http_function_sel_page
*   Description  : function select page
*   Input        : None
*   Output       : None
*   Return Value : String
*   Calls        :
*   Called By    :
*
*   History:
*
*       1.  Date         : 2019/6/30
*           Author       : zhuhongfei
*           Modification : Created function
*
*****************************************************************************/
String http_function_sel_page()
{
    String html = "";
    html += "<html> <head><meta http-equiv='Content-Type' content='text/html ;charset=utf-8'/>";
    html = html +
           "<style>label{ cursor: pointer; display: inline-block;padding: 3px 6px;" +
           "text-align: left;width: 150px;vertical-align: top;font-size:20px;}";
    html = html + "input[type='radio']{width:20px;height:20px;}";
    html = html +
           "input[type='submit']{height:35px;line-height:35px;font-size:25px;}</style></head>";
    html  = html + "<body><h1>" + zh_array[notice_change_type] + "</h1>";
    html += "<form method='POST' action='/sel'>";
    html  = html +  "<label>" + zh_array[app_task] +
            "</label><input type='radio' name='function' value='3' checked='checked'><br>";
    html  = html +  "<label>" + zh_array[wifi_set_task] +
            "</label><input type='radio' name='function' value='4'><br>";
    html  = html +  "<label>" + zh_array[debug_tcp] +
            "</label><input type='radio' name='function' value='5'><br>";

    html  = html +  "<label>" + zh_array[enter_transport_mode] +
            "</label><input type='radio' name='function' value='12'><br>";
    html  = html +  "<input type='submit' value=" + zh_array[make_sure] + ">";
    html += "</form></body></html>";
    return html;
}

/*****************************************************************************
*   Prototype    : http_cfgwifi_page
*   Description  : config wifi page
*   Input        : None
*   Output       : None
*   Return Value : String
*   Calls        :
*   Called By    :
*
*   History:
*
*       1.  Date         : 2019/6/30
*           Author       : zhuhongfei
*           Modification : Created function
*
*****************************************************************************/
String http_cfgwifi_page()
{
    String html = "";
    html += "<html> <head><meta http-equiv='Content-Type' content='text/html ;charset=utf-8'/> ";
    html = html +
           "<style>label{ cursor: pointer; display: inline-block;padding: 3px 6px;" +
           "text-align: left;width: 100px;vertical-align: top;font-size:20px;}</style></head>";
    html = html +  "<body><h1>" + zh_array[tel_wifi_set] + "</h1>";
    html += "<form method='POST' action='/cfgwifi'>";
    html = html +   "<p><label>" + zh_array[tel_wifi_ssid] +
           ":</label><input name='ssid' type='text'  required='required'  maxlength='16'><br></p>";
    html = html +   "<p><label>" + zh_array[tel_wifi_pwd] +
           ":</label><input  name='pwd'  type='text'  required='required'  maxlength='16'><br></p>";
    html = html +   "<p><label>" + zh_array[tel_wifi_pwd_cfm] +
           ":</label><input name='pwd' type='text' required='required'  maxlength='16'><br></p>";

    html = html +  "<input type='submit' value=" + zh_array[config_wifi] + ">";
    html += "</form></body></html>";
    return html;
}
/*****************************************************************************
*   Prototype    : http_handle_config_data
*   Description  : receive and save config data,sendthis data to stm32
*   Input        : AsyncWebServerRequest *request
*   Output       : None
*   Return Value : void
*   Calls        :
*   Called By    :
*
*   History:
*
*       1.  Date         : 2019/6/16
*           Author       : zhuhongfei
*           Modification : Created function
*
*****************************************************************************/
void http_handle_config_data(AsyncWebServerRequest *request)
{

    for (uint8_t i = 0; i < CONFIG_DATA_BYTE - 1; i++)
    {

        char temp_str[16];
        AsyncWebParameter *p = request ->getParam(i);
        if (p->value() != NULL)
        {
            sprintf(temp_str, "%s", p->value().c_str());
        }
        else
        {
            continue;
        }

        switch (i)
        {
            case get_nb_ip1:
                save_cfg_data[nb_ip1] = atoi(temp_str);
                doc["IP1"] = temp_str;//save_cfg_data[0];
                break;
            case get_nb_ip2:
                save_cfg_data[nb_ip2] = atoi(temp_str);
                doc["IP2"] = temp_str; // save_cfg_data[1];
                break;
            case get_nb_ip3:
                save_cfg_data[nb_ip3] = atoi(temp_str);
                doc["IP3"] = temp_str; // save_cfg_data[2];
                break;
            case get_nb_ip4:
                save_cfg_data[nb_ip4] = atoi(temp_str);
                doc["IP4"] = temp_str;//save_cfg_data[3];
                break;
            case get_nb_port:
                save_cfg_data[nb_port_high] = atoi(temp_str) >> 8;
                save_cfg_data[nb_port_low]  = atoi(temp_str) & 0xFF;
                doc["port"] = temp_str;
                break;
            case get_angle_th:
                save_cfg_data[angle_th] = atoi(temp_str);
                doc["angle"] = temp_str;//save_cfg_data[6];
                break;
            case get_depth_max:
                save_cfg_data[depth_max] = atoi(temp_str);//(uint8_t)(atof(temp_str) * 10);
                doc["depth_max_save"] = temp_str;//save_cfg_data[7];
                break;
            case get_depth_min:
                save_cfg_data[depth_min] = atoi(temp_str);//(uint8_t)(atof(temp_str) * 10);
                doc["depth_min_save"] = temp_str;//save_cfg_data[8];
                break;
            case get_temper:
                save_cfg_data[temper_th] = atoi(temp_str);
                doc["temper"] = temp_str;//save_cfg_data[9];
                break;
            case get_start_nb:
                save_cfg_data[start_nb] = atoi(temp_str);
                doc["start_nb"] = temp_str;//save_cfg_data[10];
                break;
            case get_start_gps:
                save_cfg_data[start_gps] = atoi(temp_str);
                doc["start_gps"] = temp_str;//save_cfg_data[11];
                break;
            case get_gps_cycle:
                save_cfg_data[gps_cycle] = atoi(temp_str);
                doc["gps_cycle"] = temp_str;//save_cfg_data[12];
                break;
            case get_data_cycle:
                save_cfg_data[data_cycle] = atoi(temp_str);
                doc["data_cycle"] = temp_str;//save_cfg_data[13];
                break;

            default:
                break;

        }

    }
    //int paramsNr = request->params();
    //DEBUG_UART_PRINTLN(paramsNr);

    save_config_file();
    send_cmd_to_host(cfg_set);

    Serial.flush();
    String html = "";
    html += "<html> <head><meta http-equiv='Content-Type' content='text/html ;charset=utf-8'/></head>";
    html = html +  "<h1>" + zh_array[config_success] + "</h1>";
    html += "<body><form method='GET' action='/cfg'>";
    html = html +  "<input type='submit' value=" + zh_array[config_again] +
           "></form></body></html>";
    request->send(200, "text/html", html.c_str());
}

/*****************************************************************************
*   Prototype    : http_handle_cfgwifi
*   Description  : handle config wifi data on html
*   Input        : AsyncWebServerRequest * request
*   Output       : None
*   Return Value : bool
*   Calls        :
*   Called By    :
*
*   History:
*
*       1.  Date         : 2019/6/30
*           Author       : zhuhongfei
*           Modification : Created function
*
*****************************************************************************/
bool http_handle_cfgwifi(AsyncWebServerRequest *request)
{
    int paramsNr = request->params();//get paramas num

    if (3 == paramsNr)
    {
        AsyncWebParameter *p = request ->getParam(1);
        char pwd_str[17];
        if (p->value() != NULL)
        {
            sprintf(pwd_str, "%s", p->value().c_str());
        }
        else
        {
            return false;
        }
        char cfm_pwd[17];
        p = request ->getParam(2);
        if (p->value() != NULL)
        {
            sprintf(cfm_pwd, "%s", p->value().c_str());
        }
        else
        {
            return false;
        }
        if (!strcmp(pwd_str, cfm_pwd))
        {
            doc["ap_pwd"] = cfm_pwd;
#if DEBUG_KEY
            DEBUG_UART_PRINTF("set pwd:%s\n", cfm_pwd);
#endif
            p = request ->getParam(0);
            char ssid_str[17];
            if (p->value() != NULL)
            {
                sprintf(ssid_str, "%s", p->value().c_str());
            }
            else
            {
                return false;
            }
            doc["ap_ssid"] = ssid_str;
#if DEBUG_KEY
            DEBUG_UART_PRINTF("set ssid:%s\n", ssid_str);
#endif
            save_config_file();

            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }

}

String http_handle_function_sel(AsyncWebServerRequest *request)
{
    String state = "";
    AsyncWebParameter *p = request->getParam(0);
    char fun_sel[2];
    uint8_t cmd_id = 0;
    if (p->value() != NULL)
    {
        sprintf(fun_sel, "%s", p->value().c_str());
    }
    cmd_id = atoi(fun_sel);
    //DEBUG_UART_PRINTF("cmd_id:%d\n",cmd_id);
    switch (cmd_id)
    {
        case sta_send:
            read_status_start = 0;
            current_cmd_id = sta_send;

            //read_status_start = 1;
            state = zh_array[app_task];
            send_cmd_to_host(sta_send);

            break;

        case set_wifi:
            read_status_start = 0;
            current_cmd_id = set_wifi;

            //read_status_start = 1;
            state = zh_array[wifi_set_task];
            send_cmd_to_host(set_wifi);

            break;

        case tcp_tran:
            read_status_start = 0;
            current_cmd_id = tcp_tran;

            //read_status_start = 1;
            state = zh_array[debug_tcp];
            send_cmd_to_host(tcp_tran);

            break;
        case transpot_mode:
            read_status_start = 0;

            state = zh_array[enter_transport_mode];
            send_cmd_to_host(transpot_mode);

            break;
        default:
            read_status_start = 0;
            current_cmd_id = sta_send;
            state = zh_array[app_task];
            send_cmd_to_host(sta_send);

            break;
    }
    return state;
}
/*****************************************************************************
*   Prototype    : receive_cfg_sta_data
*   Description  : receive 32 bytes data from serial,consist of status data
*   Input        : None
*   Output       : None
*   Return Value : void
*   Calls        :
*   Called By    :
*
*   History:
*
*       1.  Date         : 2019/6/12
*           Author       : zhuhongfei
*           Modification : Created function
*
*****************************************************************************/
void receive_cfg_sta_data()
{
    if (Serial.available())
    {
        save_serial_data[read_status_count] = (uint8_t)Serial.read();
#if DEBUG_KEY
        //DEBUG_UART_PRINTF("%d--%x\n", read_status_count,
        //                  save_serial_data[read_status_count]);
#endif
        if ((0 == read_status_count) && (HEADER != save_serial_data[0]))
        {
            read_status_count = 0;
        }
        else if (HEADER == save_serial_data[0])
        {
            read_status_count++;
        }

        //size_t len = Serial.available();
        // uint8_t sbuf[len];
        //Serial.readBytes(sbuf, len);
    }

    if (SERIAL_DATA_LENS == read_status_count)
    {
        Serial.flush();
        read_status_count = 0;
        //read_status_start=0;
        read_status_end = 1;
    }

}
/*****************************************************************************
*   Prototype    : receive_wifi_data
*   Description  : receive wifi conig data
*   Input        : None
*   Output       : None
*   Return Value : void
*   Calls        :
*   Called By    :
*
*   History:
*
*       1.  Date         : 2019/6/18
*           Author       : zhuhongfei
*           Modification : Created function
*
*****************************************************************************/
void receive_wifi_data()
{
    if (Serial.available())
    {
        char sbuf[2];
        sbuf[0] = (char)Serial.read();

        if ((wifi_head[0] == '5') && (wifi_head[1] == '5') && (wifi_head[2] == '0') &&
                (wifi_head[3] == '2'))
        {
            wifi_set[read_wifi_count] = sbuf[0];
            if (wifi_set[read_wifi_count] == 'A')
            {
                read_end_count++;
                if (read_end_count == 2)
                {
                    wifi_head[0] = 0;
                    wifi_head[1] = 0;
                    wifi_head[2] = 0;
                    wifi_head[3] = 0;
                    //memset(wifi_head,0,4);
                    read_end_count = 0;
                    read_wifi_count = 0;
                    Serial.flush();
                    parse_wifi_data();
                    //break;
                }
            }
            read_wifi_count++;
            if (WIFI_DATA_LENS == read_wifi_count)
            {
                read_wifi_count = 0;
            }
        }
        else
        {
            wifi_head[read_wifi_count] = sbuf[0];
            if ((0 == read_wifi_count) && ('5' != wifi_head[0]))
            {
                read_wifi_count = 0;
            }
            else if ('5' == wifi_head[0])
            {
                read_wifi_count++;
            }
            if (read_wifi_count == 4)
            {
                read_wifi_count = 0;
            }

        }
    }

}

/*****************************************************************************
*   Prototype    : uart_to_tcp
*   Description  : tcpserver send data from serial to tcpclient
*   Input        :
*   Output       : None
*   Return Value : void
*   Calls        :
*   Called By    :
*
*   History:
*
*       1.  Date         : 2019/6/5
*           Author       : zhuhongfei
*           Modification : Created function
*
*****************************************************************************/
void uart_to_tcp()
{
    uint8_t i;
    //check request from new client
    if (data_server.hasClient())
    {
        for (i = 0; i < MAX_SRV_CLIENTS; i++)
        {
            //release valid or unconnected client
            if (!serverClients[i] || !serverClients[i].connected())
            {
                if (serverClients[i])
                {
                    serverClients[i].stop();
                }
                //assign new client
                serverClients[i] = data_server.available();
#if DEBUG_KEY
                DEBUG_UART_PRINT("New client: ");
                DEBUG_UART_PRINTLN(i + 1);
#endif
                break;
            }
        }
        //reject to connect when to reach the max connected number
        if (i == MAX_SRV_CLIENTS)
        {
            WiFiClient serverClient = data_server.available();
            serverClient.stop();
#if DEBUG_KEY
            DEBUG_UART_PRINTLN("Connection rejected ");
#endif
        }
    }
    //check data from client
    for (i = 0; i < MAX_SRV_CLIENTS; i++)
    {
        if (serverClients[i] && serverClients[i].connected())
        {
            if (serverClients[i].available())
            {
                //get data from the telnet client and push it to the UART
                while (serverClients[i].available())
                {
                    //send to serial
                    Serial.write(serverClients[i].read());
                }
            }
        }
    }

    if (Serial.available())
    {
        //send data from serial to client
        size_t len = Serial.available();
        uint8_t sbuf[len];
        Serial.readBytes(sbuf, len);
        //char sbuf[2];
        //sbuf[0] = (char)Serial.read();

        //push UART data to all connected telnet clients
        for (i = 0; i < MAX_SRV_CLIENTS; i++)
        {
            if (serverClients[i] && serverClients[i].connected())
            {
                serverClients[i].write(sbuf, len);
                //delay(1);
            }
        }

    }

}

/*****************************************************************************
*   Prototype    : receive_cmd_id
*   Description  : receive cmd id from stm32,decide esp8266 to display
                   different function
*   Input        : None
*   Output       : None
*   Return Value : void
*   Calls        :
*   Called By    :
*
*   History:
*
*       1.  Date         : 2019/6/18
*           Author       : zhuhongfei
*           Modification : Created function
*
*****************************************************************************/
void receive_cmd_id()
{
    if (Serial.available() && (CMD_ID_DATA != read_cmd_count))
    {
        save_cmd_id[read_cmd_count] = (uint8_t)Serial.read();
#if DEBUG_KEY
        DEBUG_UART_PRINTF("%d--%x\n", read_cmd_count, save_cmd_id[read_cmd_count]);
#endif
        read_cmd_count++;

    }

    if (CMD_ID_DATA == read_cmd_count)
    {
        Serial.flush();
        if ((HEADER == save_cmd_id[0]) && (ENDUP == save_cmd_id[2]))
        {
            switch (save_cmd_id[1])
            {
                case sta_send:
#if DEBUG_KEY
                    DEBUG_UART_PRINTLN("enter STA_SEND_ID");
#endif
                    //receive_cfg_sta_data();
                    current_cmd_id = sta_send;
                    read_cmd_count = 0;
                    break;
                case set_wifi:
#if DEBUG_KEY
                    DEBUG_UART_PRINTLN("enter SET_WIFI_ID");
#endif
                    current_cmd_id = set_wifi;
                    //receive_wifi_data();
                    read_cmd_count = 0;
                    break;
                case tcp_tran:
#if DEBUG_KEY
                    DEBUG_UART_PRINTLN("enter TCP_TRAN_ID");
#endif
                    current_cmd_id = tcp_tran;
                    //uart_to_tcp();
                    read_cmd_count = 0;
                    break;
                default:
                    read_cmd_count = 0;
                    break;
            }
            read_status_start = 0;
        }
        else
        {
            read_cmd_count = 0;
        }
    }
}

/*****************************************************************************
*   Prototype    : cacl_crc16
*   Description  : calculate crc16
*   Input        : uint8_t data_input
*   Output       : None
*   Return Value : void
*   Calls        :
*   Called By    :
*
*   History:
*
*       1.  Date         : 2019/7/3
*           Author       : zhuhongfei
*           Modification : Created function
*
*****************************************************************************/
void cacl_crc16(uint8_t data_input)
{
    uint16_t temp_data;
    update_calc_crc = data_input ^ update_calc_crc;
    for (uint8_t i = 0; i < 8; i++)
    {
        temp_data = update_calc_crc & 0x0001;
        update_calc_crc = update_calc_crc >> 1;
        if (temp_data)
        {
            update_calc_crc = update_calc_crc ^ 0xa001;
        }
    }
}

/*****************************************************************************
*   Prototype    : copy_update_file
*   Description  : when check crc value success,copy file from backup area
                   to update area
*   Input        : const char *dst_file
*                  const char *src_file
*   Output       : None
*   Return Value : void
*   Calls        :
*   Called By    :
*
*   History:
*
*       1.  Date         : 2019/7/7
*           Author       : zhuhongfei
*           Modification : Created function
*
*****************************************************************************/
void copy_update_file(const char *dst_file, const char *src_file)
{
    File dstfile = SPIFFS.open(dst_file, "w");
    File srcfile = SPIFFS.open(src_file, "r");
    dstfile.flush();
    if (srcfile)
    {
        while (srcfile.available())
        {
            if (dstfile)
            {
                dstfile.write(srcfile.read());
            }
            else
            {
                return;
            }
        }
        srcfile.close();
        dstfile.close();
    }
    else
    {
        return;
    }

}
/*****************************************************************************
*   Prototype    : receive_update_data
*   Description  : receive updae data from host
*   Input        : None
*   Output       : None
*   Return Value : void
*   Calls        :
*   Called By    :
*
*   History:
*
*       1.  Date         : 2019/7/3
*           Author       : zhuhongfei
*           Modification : Created function
*
*****************************************************************************/
void receive_update_data()
{
    if (Serial.available())
    {
        //DEBUG_UART_PRINTLN("Receive host data Start");
        uint8_t tmp_data = (uint8_t)Serial.read();
        if (0 == update_data_index)
        {

            update_data_byte_sum = ((uint32_t)tmp_data) << 16;
        }
        else if (1 == update_data_index)
        {
            //the first three bytes of the file indicate the total number of data
            update_data_byte_sum |= ((uint32_t)tmp_data) << 8;

        }
        else if (2 == update_data_index)
        {
            update_data_byte_sum |= tmp_data;
            if (update_data_byte_sum < (UPDATE_HOST_PACKAGE * UPDATE_DATA_LENS))
            {
                doc["NB_update_bytes"] = update_data_byte_sum;
            }

        }

        else if (update_data_index < update_data_byte_sum + 3)
        {
            cacl_crc16(tmp_data);
            doc["NB_update_data_crc16"] = update_calc_crc;
        }
        else if (update_data_byte_sum + 3 == update_data_index)
        {

            update_rece_crc = (uint16_t)tmp_data << 8;
        }
        else if (update_data_byte_sum + 4 == update_data_index)
        {

            update_rece_crc |= tmp_data;
#if DEBUG_KEY
            DEBUG_UART_PRINTF("update_recv_crc:%x\n", update_rece_crc);
            DEBUG_UART_PRINTF("update_calc_crc:%x\n", update_calc_crc);
#endif

        }

        update_data_index ++;
#if DEBUG_KEY
        DEBUG_UART_PRINTF("current_update_index:%d\n", update_data_index);
#endif
        if (update_data_index <= update_data_byte_sum + 5)
        {
            if (savefile)
            {
                savefile.write(tmp_data);
            }
            doc["NB_update_index"] = update_data_index;
            save_config_file();
        }

        if ((0 == update_data_index % NB_UPDATE_LENS) &&
                (update_data_index < (update_data_byte_sum + 5)))
        {
            current_cmd_id = sta_send;
            savefile.close();
        }
        else if ((0 != update_data_index % NB_UPDATE_LENS) &&
                 (update_data_index == (update_data_byte_sum + 5)))
        {
            savefile.close();

        }
        else if ((0 == (update_data_index % NB_UPDATE_LENS)) &&
                 (update_data_index >= (update_data_byte_sum + 5)))
        {
#if DEBUG_KEY
            DEBUG_UART_PRINTLN("begin check crc");
#endif
            savefile.close();
            //check crc,if success,copy file content from save_update_backup to save_update_file
            if (update_rece_crc == update_calc_crc)
            {
                copy_update_file(save_update_file, save_update_backup);
#if DEBUG_KEY
                DEBUG_UART_PRINTLN("saved update file from host");
                DEBUG_UART_PRINTLN("opening wifi");
#endif
                setup_WiFi_mode();

                update_online_flag = 1;      //online update
                current_cmd_id     = update_host_req;
            }
            else
            {
#if DEBUG_KEY
                DEBUG_UART_PRINTLN("check crc fail");
#endif

                send_cmd_to_host(update_check_error);
                current_cmd_id = sta_send;
            }
            update_calc_crc    = 0xffff; //restore initial value
            update_data_index  = 0;
        }
    }
}

/*****************************************************************************
*   Prototype    : parse_cmd_id
*   Description  : parse cmd id,mainly update host process
*   Input        : None
*   Output       : None
*   Return Value : void
*   Calls        :
*   Called By    :
*
*   History:
*
*       1.  Date         : 2019/7/3
*           Author       : zhuhongfei
*           Modification : Created function
*
*****************************************************************************/
void parse_cmd_id()
{
    if (read_status_end)
    {
        if ((HEADER == save_serial_data[prot_head]) &&
                (ENDUP == save_serial_data[prot_end]))
        {
            switch (save_serial_data[prot_cmd_id])
            {
                case cfg_get:
                    read_status_end = 1;//don't affect app task
                    break;
                case cfg_set:
                    read_status_end = 1;
                    break;
                case sta_get:
                    read_status_end = 1;
                    break;
                case update_host_req:
                    read_status_end = 0;
                    update_fs2uart_start = 1;
                    break;
                case update_host_rec:
                    read_status_end   = 0;

#if DEBUG_KEY

                    DEBUG_UART_PRINTLN("online update");
                    DEBUG_UART_PRINTLN("closing wifi");
#endif
                    WiFi.softAPdisconnect(true);

                    if (load_NB_update_msg())
                    {
                        if (update_data_byte_sum > (UPDATE_HOST_PACKAGE * UPDATE_DATA_LENS))
                        {
                            update_data_index           = 0;
                            update_data_byte_sum        = 0;
                            update_calc_crc             = 0xffff;
                            doc["NB_update_index"]      = update_data_index;
                            doc["NB_update_bytes"]      = update_data_byte_sum;
                            doc["NB_update_data_crc16"] = update_calc_crc;
                            save_config_file();
#if DEBUG_KEY
                            DEBUG_UART_PRINTLN("NB_update_host_start");
                            DEBUG_UART_PRINTF("NB_update_index:%d\n", update_data_index);
                            DEBUG_UART_PRINTF("NB_update_bytes:%d\n", update_data_byte_sum);
                            DEBUG_UART_PRINTF("NB_update_data_crc16:%x\n", update_calc_crc);
#endif

                            savefile = SPIFFS.open(save_update_backup, "w");
                            savefile.flush();
                            current_cmd_id    = update_host_rec;
                            break;
                        }
                        if ((0 == update_data_index % NB_UPDATE_LENS) &&
                                ((update_data_byte_sum + 5) != update_data_index))
                        {
                            savefile = SPIFFS.open(save_update_backup, "a");
                        }
                        else if (update_data_byte_sum + 5 == update_data_index) //new update file
                        {
                            update_data_index      		= 0;
                            update_data_byte_sum   		= 0;
                            update_calc_crc             = 0xffff;
                            doc["NB_update_index"] 		= update_data_index;
                            doc["NB_update_bytes"] 		= update_data_byte_sum;
                            doc["NB_update_data_crc16"] = update_calc_crc;
                            save_config_file();
#if DEBUG_KEY
                            DEBUG_UART_PRINTLN("NB_update_host_start");
                            DEBUG_UART_PRINTF("NB_update_index:%d\n", update_data_index);
                            DEBUG_UART_PRINTF("NB_update_bytes:%d\n", update_data_byte_sum);
                            DEBUG_UART_PRINTF("NB_update_data_crc16:%x\n", update_calc_crc);
#endif

                            savefile = SPIFFS.open(save_update_backup, "w");
                            savefile.flush();
                        }
                    }
                    else
                    {
                        update_data_index      		= 0;
                        update_data_byte_sum   		= 0;
                        update_calc_crc             = 0xffff;
                        doc["NB_update_index"] 		= update_data_index;
                        doc["NB_update_bytes"] 		= update_data_byte_sum;
                        doc["NB_update_data_crc16"] = update_calc_crc;
                        save_config_file();
#if DEBUG_KEY
                        DEBUG_UART_PRINTLN("NB_update_host_start");
                        DEBUG_UART_PRINTF("NB_update_index:%d\n", update_data_index);
                        DEBUG_UART_PRINTF("NB_update_bytes:%d\n", update_data_byte_sum);
                        DEBUG_UART_PRINTF("NB_update_data_crc16:%x\n", update_calc_crc);
#endif

                        savefile = SPIFFS.open(save_update_backup, "w");
                        savefile.flush();
                    }
                    current_cmd_id    = update_host_rec;
                    break;
                case update_check_success:
                    read_status_end = 0;
                    update_chksum_flag = update_check_success;//host check success
                    break;
                case update_check_error:
                    read_status_end = 0;
                    update_chksum_flag = update_check_error;//host check fail
                    break;
                case host_record_log:
                    read_status_end = 1;
                    break;

                default:
                    read_status_end = 0;
                    break;
            }

        }

    }
    else
    {
        update_chksum_flag = update_no_response;
    }
}


/*****************************************************************************
*   Prototype    : parse_cfg_sta_data
*   Description  : handle receive serial data and checksum
*   Input        : uint8_t cmd_id
*   Output       : None
*   Return Value : bool
*   Calls        :
*   Called By    :
*
*   History:
*
*       1.  Date         : 2019/6/12
*           Author       : zhuhongfei
*           Modification : Created function
*
*****************************************************************************/
bool parse_cfg_sta_data(uint8_t cmd_id)
{
    uint8_t chk_sum = 0;
    if (read_status_end)
    {
        if ((HEADER == save_serial_data[prot_head]) &&
                (ENDUP == save_serial_data[prot_end]))
        {
            if (save_serial_data[prot_cmd_id] == cmd_id)
            {
                if (host_record_log == save_serial_data[prot_cmd_id])
                {
                    for (uint8_t i = prot_data_start; i < prot_data_start + HOST_LOG_BYTES; i++)
                    {
                        chk_sum += save_serial_data[i];
                    }
                    if (chk_sum == save_serial_data[prot_data_start + HOST_LOG_BYTES])
                    {
                        for (uint8_t i = 0; i < HOST_LOG_BYTES; i++)
                        {

                            save_log_data[i] = save_serial_data[prot_data_start + i];
                        }

                        read_status_end = 0;
                        error_msg_flag  = no_error;
                        return true;
                    }
                    else
                    {
                        error_msg_flag = check_error;
                        read_status_end = 0;
                        return false;
                    }
                }
                else
                {
                    for (uint8_t i = prot_data_start; i < prot_data_start + STATUS_DATA_BYTE; i++)
                    {
                        chk_sum += save_serial_data[i];
                    }
                    if (chk_sum == save_serial_data[prot_data_start + STATUS_DATA_BYTE])
                    {
                        if (sta_get == cmd_id)
                        {
                            for (uint8_t i = 0; i < STATUS_DATA_BYTE; i++)
                            {

                                save_sta_data[i] = save_serial_data[prot_data_start + i];
                            }
                        }
                        else if (cfg_get == cmd_id)
                        {
                            for (uint8_t i = 0; i < CONFIG_DATA_BYTE; i++)
                            {

                                save_cfg_data[i] = save_serial_data[prot_data_start + i];
                            }
                        }


                        read_status_end = 0;
                        error_msg_flag  = no_error;
                        return true;
                    }
                    else
                    {
                        error_msg_flag = check_error;
                        read_status_end = 0;
                        return false;
                    }
                }

            }
            else
            {
                error_msg_flag = cmd_id_error;
                read_status_end = 0;
                return false;
            }

        }
        else
        {
            error_msg_flag = norm_error;
            read_status_end = 0;
            return false;
        }
    }
    else
    {
        error_msg_flag = receive_error;
        return false;
    }
}

/*****************************************************************************
*   Prototype    : parse_config_data
*   Description  : cfg_get,handle config data from stm32
*   Input        : None
*   Output       : None
*   Return Value : void
*   Calls        :
*   Called By    :
*
*   History:
*
*       1.  Date         : 2019/6/19
*           Author       : zhuhongfei
*           Modification : Created function
*
*****************************************************************************/
void parse_config_data()
{
    sprintf(NB_lot_ip1, "%d", save_cfg_data[nb_ip1]);
    sprintf(NB_lot_ip2, "%d", save_cfg_data[nb_ip2]);
    sprintf(NB_lot_ip3, "%d", save_cfg_data[nb_ip3]);
    sprintf(NB_lot_ip4, "%d", save_cfg_data[nb_ip4]);

    sprintf(NB_lot_port, "%d",
            ((uint16_t)save_cfg_data[nb_port_high] << 8) | save_cfg_data[nb_port_low]);
    sprintf(Adx1_ang, "%d", save_cfg_data[angle_th]);
    sprintf(depth_max_cfg, "%d", save_cfg_data[depth_max]);
    sprintf(depth_min_cfg, "%d", save_cfg_data[depth_min]);

    sprintf(Tempera, "%d", save_cfg_data[temper_th]);
    sprintf(Nb_on, "%d", save_cfg_data[start_nb]);
    sprintf(GPS_on, "%d", save_cfg_data[start_gps]);
    sprintf(GPS_update_cycle, "%d", save_cfg_data[gps_cycle]);
    sprintf(Data_update_cycle, "%d", save_cfg_data[data_cycle]);

}
/*****************************************************************************
*   Prototype    : parse_wifi_data
*   Description  : parse serial data,which consist of wifi cconfig data
*   Input        : None
*   Output       : None
*   Return Value : void
*   Calls        :
*   Called By    :
*
*   History:
*
*       1.  Date         : 2019/6/16
*           Author       : zhuhongfei
*           Modification : Created function
*
*****************************************************************************/
void parse_wifi_data()
{
    char tmp_ssid[17];
    char tmp_pwd[17];
    uint8_t ssid_receive_flag = 0;
    uint8_t pwd_receive_count = 0;
    //read_status_start=1; //shut up tcpserver
#if DEBUG_KEY
    DEBUG_UART_PRINTLN("enter parse wifi config data");
#endif
    uint8_t len = strlen(wifi_set);
#if DEBUG_KEY
    DEBUG_UART_PRINTF("wifi_set len:%d\n", len);
#endif
    for (uint8_t i = 0; i < len; i++)
    {
        //DEBUG_UART_PRINTF("receive char:%c\n", wifi_set[i]);
        if ((wifi_set[i] != '\n') && (!ssid_receive_flag))
            //if ((wifi_set[i] != ' ') && (!ssid_receive_flag))
        {
            tmp_ssid[i] = wifi_set[i];
#if DEBUG_KEY
            DEBUG_UART_PRINTF("tmp_ssid[%d]:%c\n", i, tmp_ssid[i]);
#endif
        }
        else if ((wifi_set[i] == '\n') && (!ssid_receive_flag))
            //else if ((wifi_set[i] == ' ') && (!ssid_receive_flag))
        {
            tmp_ssid[i] = '\0';
            ssid_receive_flag = 1;
            continue;
        }
        if ((wifi_set[i] != '\n') && (ssid_receive_flag))
            //if ((wifi_set[i] != ' ') && (ssid_receive_flag))
        {
            tmp_pwd[pwd_receive_count] = wifi_set[i];
#if DEBUG_KEY
            DEBUG_UART_PRINTF("tmp_pwd[%d]:%c\n", pwd_receive_count,
                              tmp_pwd[pwd_receive_count]);
#endif
            pwd_receive_count++;
        }
        else if ((wifi_set[i] == '\n') && (ssid_receive_flag))
            //else if ((wifi_set[i] == ' ') && (ssid_receive_flag))
        {
            tmp_pwd[pwd_receive_count] = '\0';
            ssid_receive_flag = 0;
            pwd_receive_count = 0;
            break;
        }
    }
#if DEBUG_KEY
    DEBUG_UART_PRINTF("receive ssid:%s\n", tmp_ssid);
    DEBUG_UART_PRINTF("receive pwd:%s\n", tmp_pwd);
#endif
    doc["ap_ssid"] = tmp_ssid;
    doc["ap_pwd"] = tmp_pwd;
#if DEBUG_KEY
    DEBUG_UART_PRINTF("save success\n");
#endif
    save_config_file();
    shouldReboot = true;

    //ESP.restart();//it put to loop
}

/*****************************************************************************
*   Prototype    : http_init
*   Description  : monitor http routing
*   Input        : None
*   Output       : None
*   Return Value : void
*   Calls        :
*   Called By    :
*
*   History:
*
*       1.  Date         : 2019/6/5
*           Author       : zhuhongfei
*           Modification : Created function
*
*****************************************************************************/
void http_init()
{

    server.on("/host", HTTP_GET, [](AsyncWebServerRequest * request)
    {
        String html = "";
        html += "<html> <head><meta http-equiv='Content-Type' content='text/html ;charset=utf-8'/>";
        html += "<script language='JavaScript'>function refresh_diy(){window.location.reload();}";
        html += "setTimeout('refresh_diy()',500);</script></head><body>";

        if (1 == update_host_processing)
        {
            html  = html + "<h1>" + zh_array[updating_host] + "</h1>";
            html = html + "<h2>" + zh_array[package_num] + ":" + update_package_num +
                   "</h2>";
            html = html + "<h2>" + zh_array[finish_prop] + ":" + update_package_sum +
                   "</h2>";
        }
        else if (1 == update_host_processed)
        {

            html = html + "<h2>" + zh_array[update_success] + "</h2>";
        }
        else if (1 == update_host_error)
        {
            html = html + "<h2>" + zh_array[update_fail] + "</h2>";
            html = html + "<h3>" + zh_array[update_fail_alert] + "</h3>";
        }
        else
        {
            html = html + "<h2>" + zh_array[host_sure] + "</h2>";
        }
        html += "</body></html>";
        request->send(200, "text/html", html.c_str());

    });


    server.on("/", HTTP_GET, [](AsyncWebServerRequest * request)
    {
        if (!request->authenticate(user_name, user_password))
        {
            return request->requestAuthentication();
        }
        if (!update_fs2uart_start)
        {
            if (SPIFFS.exists("/main_page.html"))
            {
                request->send(SPIFFS, "/main_page.html", String(), false, processor);
            }
            else
            {
                request->send(200, "text/html", http_main_page());
            }
        }
        else
        {
            http_update_host_status(request);
        }

        /*todo:it will be used when main_page.html doesn't exist in SPIFFS*/
        //request->send(200, "text/html", http_main_page());
    });

    server.on("/sel", HTTP_GET, [](AsyncWebServerRequest * request)
    {
        if (!request->authenticate(user_name, user_password))
        {
            return request->requestAuthentication();
        }
        if (!update_fs2uart_start)
        {
            request->send(200, "text/html", http_function_sel_page());
        }
        else
        {
            http_update_host_status(request);
        }
    });
    server.on("/sel", HTTP_POST, [](AsyncWebServerRequest * request)
    {
        if (!request->authenticate(user_name, user_password))
        {
            return request->requestAuthentication();
        }
        //read_status_start = 1;
        //current_cmd_id = 0;

        String html  = "";
        String state = "";

        state = http_handle_function_sel(request);

        html += "<html> <head><meta http-equiv='Content-Type' content='text/html ;charset=utf-8'/></head> ";
        html  = html + "<body><h1>" + zh_array[notice_host] + "</h1>";
        html = html +
               "<p>" + zh_array[config_now] +
               ":<input name='cfg_fun' type='text' disabled='disabled'  value="
               +
               state  + "><br></p>";
        html += "<form method='GET' action='/sel' id='sel'>";
        html = html +  "<input type='submit' value=" + zh_array[config_again] +
               "></form><br></body></html>";
        request->send(200, "text/html", html.c_str());
    });
    server.on("/cfgwifi", HTTP_GET, [](AsyncWebServerRequest * request)
    {
        if (!request->authenticate(user_name, user_password))
        {
            return request->requestAuthentication();
        }
        if (!update_fs2uart_start)
        {
            request->send(200, "text/html", http_cfgwifi_page());
        }
        else
        {
            http_update_host_status(request);
        }
    });
    server.on("/cfgwifi", HTTP_POST, [](AsyncWebServerRequest * request)
    {
        String html = "";
        if (http_handle_cfgwifi(request))
        {
            html += "<html> <head><meta http-equiv='Content-Type' content='text/html ;charset=utf-8'/> </head>";
            html  = html + "<h1>" + zh_array[tel_config] + "</h1></html>";
            request->send(200, "text/html", html.c_str());
            shouldReboot = true;
            //ESP.restart();
        }
        else
        {
            html += "<html> <head><meta http-equiv='Content-Type' content='text/html ;charset=utf-8'/> </head>";
            html = html +  "<body><h1>" + zh_array[config_fail] + "</h1>";
            html = html +  "<span>" + zh_array[pwd_consistent] + "</span><br>";
            html += "<form method='GET' action='/cfgwifi'>";
            html = html +  "<input type='submit' value=" + zh_array[config_again] + ">";
            html += "</form></body></html>";
            request->send(200, "text/html", html.c_str());
        }


    });

    server.on("/log", HTTP_GET, [](AsyncWebServerRequest * request)
    {
        if (!request->authenticate(user_name, user_password))
        {
            return request->requestAuthentication();
        }
        char jump_record[4];
        int paramsNr = request->params();
        if (paramsNr != 0)
        {
            AsyncWebParameter *p = request ->getParam(paramsNr - 1);
            if (p->value() != NULL)
            {
                sprintf(jump_record, "%s", p->value().c_str());
                record_host_log = atoi(jump_record);
            }
        }
        send_cmd_to_host(host_record_log);

        if (!update_fs2uart_start)
        {

            if (parse_cfg_sta_data(host_record_log))
            {
                request->send(200, "text/html", http_view_host_log_page());
            }
            else
            {
                String html = "";
                html += "<html> <head><meta http-equiv='Content-Type' content='text/html ;charset=utf-8'/></head> ";
                switch (error_msg_flag)
                {
                    case check_error:
                        html = html + "<h1>" + zh_array[check_sum_fail] + "</h1></html>";
                        break;

                    case cmd_id_error:
                        html = html + "<h1>" + zh_array[cmd_id_inconsistent] + "</h1></html>";
                        break;

                    case norm_error:
                        html = html + "<h1>" + zh_array[data_norm_error] + "</h1></html>";
                        break;

                    case receive_error:
                        html = html + "<h1>" + zh_array[no_receive_data] + "</h1></html>";
                        break;

                    default:
                        html = html + "<h1>" + zh_array[no_receive_data] + "</h1>";
                        break;
                }
                html = html +  "<p><br><input type='button' value=" + zh_array[flush_post] +
                       " style='width:100px;height:40px;' onclick='window.location.reload()'</p></html>";
                request->send(200, "text/html", html.c_str());
            }
        }
        else
        {
            http_update_host_status(request);
        }
    });

    // Send a cfg request to <IP>/get?message=<message>
    server.on("/cfg", HTTP_GET, [](AsyncWebServerRequest * request)
    {
        if (!request->authenticate(user_name, user_password))
        {
            return request->requestAuthentication();
        }

        send_cmd_to_host(cfg_get);

        if (parse_cfg_sta_data(cfg_get))
        {
            //DEBUG_UART_PRINT("stm32\n");
            sprintf(data_source, "%s", zh_array[connect_success]);
            parse_config_data();
        }
        else
        {
            sprintf(data_source, "%s", zh_array[connect_fail]);
            load_config_params();
        }
        if (!update_fs2uart_start)
        {
            request->send(200, "text/html", http_config_page());
        }
        else
        {
            http_update_host_status(request);
        }
    });

    server.on("/cfg", HTTP_POST, [](AsyncWebServerRequest * request)
    {
        http_handle_config_data(request);
    });

    server.on("/sta", HTTP_GET, [](AsyncWebServerRequest * request)
    {
        if (!request->authenticate(user_name, user_password))
        {
            return request->requestAuthentication();
        }

        send_cmd_to_host(sta_get);


        //parse receive status data
        if (!update_fs2uart_start) //not update host status
        {
            if (parse_cfg_sta_data(sta_get))
            {
                request->send(200, "text/html", http_status_page());
            }
            else
            {
                String html = "";
                html += "<html> <head><meta http-equiv='Content-Type' content='text/html ;charset=utf-8'/></head> ";
                switch (error_msg_flag)
                {
                    case check_error:
                        html = html + "<h1>" + zh_array[check_sum_fail] + "</h1></html>";
                        break;

                    case cmd_id_error:
                        html = html + "<h1>" + zh_array[cmd_id_inconsistent] + "</h1></html>";
                        break;

                    case norm_error:
                        html = html + "<h1>" + zh_array[data_norm_error] + "</h1></html>";
                        break;

                    case receive_error:
                        html = html + "<h1>" + zh_array[no_receive_data] + "</h1></html>";
                        break;

                    default:
                        html = html + "<h1>" + zh_array[no_receive_data] + "</h1>";
                        break;
                }
                html = html +  "<p><br><input type='button' value=" + zh_array[flush_post] +
                       " style='width:100px;height:40px;' onclick='window.location.reload()'</p></html>";
                request->send(200, "text/html", html.c_str());
            }
        }
        else
        {
            http_update_host_status(request);
        }
        //request->send(SPIFFS, "/status.html", String(), false, processor);

    });

    server.on("/update", HTTP_GET, [](AsyncWebServerRequest * request)
    {
        if (!request->authenticate(user_name, user_password))
        {
            return request->requestAuthentication();
        }
        if (!update_fs2uart_start)
        {
            http_update_page(request);
        }
        else
        {
            http_update_host_status(request);
        }
    });
    server.on("/doupdate", HTTP_POST,
              [](AsyncWebServerRequest * request)
    {
        AsyncWebParameter *p = request ->getParam(0);
        char update_flag[16];
        sprintf(update_flag, "%s", p->value().c_str());
        if (!strcmp(update_flag, "stm32"))
        {
            String html = "";
            html += "<html> <head><meta http-equiv='Content-Type' content='text/html ;charset=utf-8'/></head> ";
            html  = html + "<body><h1>" + zh_array[file_send_host] + "</h1>";
            html  = html + "<h2>" + zh_array[update_log] + "</h2><br>";
            html += "<form method='GET' action='/host'>";
            html = html +  "<input type='submit' value=" + zh_array[jump_out] + ">";
            html += "</form></body></html>";
            request->send(200, "text/html", html.c_str());

            //clear the update flag
            update_host_error      = 0;
            update_host_processing = 0;
            update_host_processed  = 0;
            //tell host that nend to change store area
            read_status_start  = 0;
            current_cmd_id     = update_host_req;
        }
        else
        {
            String html = "";
            html += "<html> <head><meta http-equiv='Content-Type' content='text/html ;charset=utf-8'/> </head>";
            html  = html + "<h1>" + zh_array[local_update_finish] + "</h1></html>";
            request->send(200, "text/html", html.c_str());
            shouldReboot = !Update.hasError();
        }

    },
    [](AsyncWebServerRequest * request, const String & filename, size_t index,
       uint8_t *data, size_t len, bool final)
    {
        AsyncWebParameter *p = request ->getParam(0);
        char update_flag[16];
        sprintf(update_flag, "%s", p->value().c_str());
        if (!strcmp(update_flag, "esp8266"))
        {
            http_handle_update_esp8266(request, filename, index, data, len, final);
        }
        else if (!strcmp(update_flag, "stm32"))
        {
            http_handle_update_stm32(request, filename, index, data, len, final);
        }
    });

    server.onNotFound(notFound);

    server.begin();
}

/*****************************************************************************
*   Prototype    : start_data_server
*   Description  : start tcpserver
*   Input        :
*   Output       : None
*   Return Value : void
*   Calls        :
*   Called By    :
*
*   History:
*
*       1.  Date         : 2019/6/5
*           Author       : zhuhongfei
*           Modification : Created function
*
*****************************************************************************/
void start_data_server()
{
    data_server.begin();
    data_server.setNoDelay(true);
}

void setup()
{
    Wire.begin();
    oled.init();                      // Initialze SSD1306 OLED display
    oled.clearDisplay();              // Clear screen


    oled.setTextXY(0, 0);             // Set cursor position, start of line 0
    oled.putString("YUERONG");
    oled.setTextXY(1, 0);             // Set cursor position, start of line 1
    oled.putString("I Love You");
    oled.setTextXY(2, 0);             // Set cursor position, start of line 2
    oled.putString("Forever");
    oled.setTextXY(3, 0);
    oled.putString("Temp:     *C");
    oled.setTextXY(4, 0);
    oled.putString("Humi:     %R");

    //DHT11
    dht.begin();

    Serial.begin(115200);
    //Serial1.begin(115200);
    if (!SPIFFS.begin())
    {
#if DEBUG_KEY
        DEBUG_UART_PRINTLN("An Error has occurred while mounting SPIFFS");
#endif

        return;
    }
    else
    {
        get_file_system_msg();
    }
    setup_WiFi_mode();

    start_data_server();

    http_init();

    //cfg sleep mode
    //TODO:WIFI_LIGHT_SLEEP/ WIFI_MODEM_SLEEP,just for WiFi_STA mode
    //WiFi.setSleepMode(WIFI_LIGHT_SLEEP,0);

    //TODO:for WiFi_AP and WiFi_STA mode ,time_interval us,RTC, GPIO16-->RST
    //ESP.deepSleep(10*1000000);

}

void loop()
{
    delay(2000);
    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    float h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();


    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t))
    {
     
        return;
    }

    oled.setTextXY(3, 5);
    oled.putFloat(t);

    oled.setTextXY(4, 5);
    oled.putFloat(h);

    
    //////////////////////////////////

    if (read_status_start)
    {
        receive_cmd_id();
    }
    else
    {
        switch (current_cmd_id)
        {
            case sta_send:
                receive_cfg_sta_data();
                parse_cmd_id();
                break;
            case set_wifi:
                receive_wifi_data();
                break;
            case tcp_tran:
                uart_to_tcp();
                break;
            case update_host_req:
                send_cmd_to_host(update_host_req);
                current_cmd_id = sta_send;
                break;
            case update_host_rec:
#if DEBUG_KEY
                //DEBUG_UART_PRINTLN("ready for receiving update data from host");
#endif
                receive_update_data();
                break;
            default:
                read_status_start = 0;
                current_cmd_id = sta_send;
                break;
        }
    }
    if (update_fs2uart_start)
    {
        update_fs_to_host();
        update_fs2uart_start = 0;
    }

    if (shouldReboot)
    {
        shouldReboot = false;
#if DEBUG_KEY
        DEBUG_UART_PRINTLN("Rebooting...");
#endif
        delay(100);
        Serial.flush();
        ESP.restart();
    }




}
