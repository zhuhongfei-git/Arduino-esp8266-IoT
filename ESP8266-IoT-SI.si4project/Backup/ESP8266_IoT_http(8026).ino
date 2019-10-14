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


void notFound(AsyncWebServerRequest *request);
void http_update_page(AsyncWebServerRequest *request);
void http_handle_update_esp8266(AsyncWebServerRequest *request,
                                const String& filename,
                                size_t index, uint8_t *data, size_t len, bool final);


/*==============================================*
 *      routines' or functions' implementations *
 *----------------------------------------------*/
/*****************************************************************************
*   Prototype    : ESP_reboot
*   Description  : decide esp8266 should be reboot
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
void ESP_reboot(void)
{
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

    html = html +
           "<input type='file' name='update'><br><br><input type='submit' value=" +
           zh_array[update_post] + "></form></body></html>";
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

        Update.runAsync(true);
        if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000))
        {
            Update.printError(Serial1);
        }
    }
    if (!Update.hasError())
    {
        if (Update.write(data, len) != len)
        {
            Update.printError(Serial1);
        }
    }
    if (final)
    {

        if (Update.end(true))
        {
            Serial1.flush();
        }
        else
        {
            Update.printError(Serial1);
        }
    }

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
    });

    server.on("/update", HTTP_GET, [](AsyncWebServerRequest * request)
    {
        if (!request->authenticate(user_name, user_password))
        {
            return request->requestAuthentication();
        }
        http_update_page(request);
    });

    server.on("/doupdate", HTTP_POST,
              [](AsyncWebServerRequest * request)
    {
        AsyncWebParameter *p = request ->getParam(0);
        char update_flag[16];
        sprintf(update_flag, "%s", p->value().c_str());
        if (!strcmp(update_flag, "esp8266"))
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

    });

    server.onNotFound(notFound);

    server.begin();
}
