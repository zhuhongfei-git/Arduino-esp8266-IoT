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

