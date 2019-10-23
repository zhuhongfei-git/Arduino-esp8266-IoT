/******************************************************************************
*
*  Copyright (C), 2016-2036, REMO Tech. Co., Ltd.
*
*******************************************************************************
*  File Name     : ESP8266_IoT_JSON.ino
*  Version       : Initial Draft
*  Author        : zhuhongfei
*  Created       : 2019/10/15
*  Last Modified :
*  Description   : save config data to FS,use JSON
*  Function List :
*
*
*  History:
*
*       1.  Date         : 2019/10/15
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


/*==============================================*
 *      project-wide global variables           *
 *----------------------------------------------*/


bool save_config_file();
void load_ap_params();
void get_file_system_msg();

/*==============================================*
 *      routines' or functions' implementations *
 *----------------------------------------------*/
/*****************************************************************************
*	Prototype	 : save_config_file
*	Description  : save config data to spiffs
*	Input		 : None
*	Output		 : None
*	Return Value : bool
*	Calls		 :
*	Called By	 :
*
*	History:
*
*		1.	Date		 : 2019/6/11
*			Author		 : zhuhongfei
*			Modification : Created function
*
*****************************************************************************/
bool save_config_file()
{
    File configFile = SPIFFS.open(configFileName, "w");
    if (!configFile)
    {

        return false;
    }

    if (serializeJson(doc, configFile) == 0)
    {

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

    if (SPIFFS.exists(configFileName))
    {
        File configFile = SPIFFS.open(configFileName, "r");

        if (configFile)
        {
            DeserializationError error = deserializeJson(doc, configFile);

            if (error)
            {
                configFile.close();
                return ;
            }
			if(doc.containsKey("ap_ssid"))
			{
				const char *ssid = doc["ap_ssid"];
				strlcpy(&ap_ssid[0], ssid, 17);
			}
            if(doc.containsKey("ap_pwd"))
            {
            	const char *pwd = doc["ap_pwd"];
            	strlcpy(&ap_pwd[0], pwd, 17);
            }
			if(doc.containsKey("progmem_version"))
			{
				const char *version = doc["progmem_version"];
				strcpy(update_command.cur_version,version);
			}
            else
            {
            	strcpy(update_command.cur_version,"1.0.1");
            }

            configFile.close();
        }
    }
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

    total_bytes = info_fs.totalBytes;
    used_bytes  = info_fs.usedBytes;
/*
#if DEBUG_KEY
    DEBUG_UART_PRINTLN("SPIFFS memory");
    DEBUG_UART_PRINT("totalBytes:");
    DEBUG_UART_PRINTLN(info_fs.totalBytes);
    DEBUG_UART_PRINT("usedBytes:");
    DEBUG_UART_PRINTLN(info_fs.usedBytes);
#endif
*/
}

