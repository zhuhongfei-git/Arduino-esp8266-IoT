/******************************************************************************
*
*  Copyright (C), 2016-2036, REMO Tech. Co., Ltd.
*
*******************************************************************************
*  File Name     : ESP8266_IoT_http.ino
*  Version       : Initial Draft
*  Author        : zhuhongfei
*  Created       : 2019/10/14
*  Last Modified :
*  Description   : create http route to wifi IP,based on ESPAsyncWebServer
*  Function List :
*
*
*  History:
*
*       1.  Date         : 2019/10/14
*           Author       : zhuhongfei
*           Modification : Created file
*
******************************************************************************/

/*==============================================*
 *      include header files                    *
 *----------------------------------------------*/
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>


/*==============================================*
 *      constants or macros define              *
 *----------------------------------------------*/
#define WEB_SRV_PORT      80
#define WIFI_SRV_PORT     65432
#define MAX_SRV_CLIENTS   2


/*==============================================*
 *      project-wide global variables           *
 *----------------------------------------------*/
AsyncWebServer server(WEB_SRV_PORT);

//Web login authentication
const char *user_name = "guest";
const char *user_password = "guest";


/*==============================================*
 *      routines' or functions' implementations *
 *----------------------------------------------*/
/*****************************************************************************
*	Prototype	 : notFound
*	Description  : Handle unregistered requests
*	Input		 : AsyncWebServerRequest *request
*	Output		 : None
*	Return Value : void
*	Calls		 :
*	Called By	 :
*
*	History:
*
*		1.	Date		 : 2019/6/5
*			Author		 : zhuhongfei
*			Modification : Created function
*
*****************************************************************************/
void notFound(AsyncWebServerRequest *request)
{
    request->send(404, "text/plain", "Didn't find anything here!");
}


/*****************************************************************************
*   Prototype    : http_init
*   Description  : consist all webserver route
*   Input        : void
*   Output       : None
*   Return Value : void
*   Calls        :
*   Called By    :
*
*   History:
*
*       1.  Date         : 2019/10/14
*           Author       : zhuhongfei
*           Modification : Created function
*
*****************************************************************************/
void http_init(void)
{
    server.on("/", HTTP_GET, [](AsyncWebServerRequest * request)
    {
        if (!request->authenticate(user_name, user_password))
        {
            return request->requestAuthentication();
        }

        String html  = "";
        html += "<html> <head><meta http-equiv='Content-Type' content='text/html ;charset=utf-8'/></head> ";
        html  = html + "<body><h1>Welcome to ESP8266-IoT world</h1>";

        html = html +  "</body></html>";
        request->send(200, "text/html", html.c_str());
    }

    server.onNotFound(notFound);

    server.begin();
}
