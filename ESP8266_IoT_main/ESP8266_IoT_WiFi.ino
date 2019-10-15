/******************************************************************************
*
*  Copyright (C), 2016-2036, REMO Tech. Co., Ltd.
*
*******************************************************************************
*  File Name     : ESP8266_IoT_WiFi.ino
*  Version       : Initial Draft
*  Author        : zhuhongfei
*  Created       : 2019/10/10
*  Last Modified :
*  Description   : wifi config
*  Function List :
*
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




/*==============================================*
 *      constants or macros define              *
 *----------------------------------------------*/
#define WIFI_CHANNAL      3
#define WIFI_HIDDEN       0
#define WIFI_MAX_CONNECT  4


/*==============================================*
 *      project-wide global variables           *
 *----------------------------------------------*/
const char *def_ap_password = "12345678";
const char *def_ap_ssid = "ESP-zhf";

const char *sta_ssid = "HUAWEI P20";
const char *sta_pwd  = "3130241056";

/*==============================================*
 *      routines' or functions' implementations *
 *----------------------------------------------*/
/*****************************************************************************
*   Prototype    : setup_WiFi_mode
*   Description  : WiFi config
*   Input        : None
*   Output       : None
*   Return Value : void
*   Calls        : 
*   Called By    : 
*
*   History:
* 
*       1.  Date         : 2019/10/10
*           Author       : zhuhongfei
*           Modification : Created function
*
*****************************************************************************/
void setup_WiFi_mode()
{
#if DEBUG_KEY
    DEBUG_UART_PRINTLN("Setting soft-AP ... ");
#endif
    /*WiFi.mode(WIFI_AP_STA);//WIFI_AP
    IPAddress softLocal(192, 168, 4, 1);
    IPAddress softGateway(192, 168, 4, 1);
    IPAddress softSubnet(255, 255, 255, 0);

    WiFi.softAPConfig(softLocal, softGateway, softSubnet);
    String ap_name=("ESP-"+(String)ESP.getChipId());


    load_ap_params();
    if ((*ap_ssid != NULL) && (*ap_pwd != NULL))
    {
        WiFi.softAP(ap_ssid, ap_pwd, WIFI_CHANNAL, WIFI_HIDDEN, WIFI_MAX_CONNECT);
    }

    else
    {
        WiFi.softAP(ap_name.c_str(), def_ap_password, WIFI_CHANNAL, WIFI_HIDDEN,
                    WIFI_MAX_CONNECT);
    }


    IPAddress myIP = WiFi.softAPIP();
#if DEBUG_KEY
    DEBUG_UART_PRINT("AP IP address: ");
    DEBUG_UART_PRINTLN(myIP);
#endif
*/

    //station mode ,not used now
    //TODO:1.will be used;2.if->while delay(500ms)+try connect count

    //DEBUG_UART_PRINTLN("Starting soft-sta ... ");
    WiFi.begin(sta_ssid, sta_pwd);//ssid, password
    
    if (WiFi.waitForConnectResult() != WL_CONNECTED)
    {
    	//DEBUG_UART_PRINTF("WiFi Failed!\n");
    	return;
    }
	/*
    DEBUG_UART_PRINT("IP Address: ");
    DEBUG_UART_PRINTLN(WiFi.localIP());
    DEBUG_UART_PRINT("Hostname: ");
    DEBUG_UART_PRINTLN(WiFi.hostname());*/

}

