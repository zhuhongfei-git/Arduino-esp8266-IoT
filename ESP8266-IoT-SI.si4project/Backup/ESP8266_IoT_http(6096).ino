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

#include <string.h>

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
uint8_t http_handle_update_esp8266(AsyncWebServerRequest *request,
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

        delay(100);

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
    html = html + "<label>" + zh_array[update_host]  + "</label>" +
           "<input type='radio' name='update' value='host'><br><br>";
    html = html +
           "<input type='file' name='update' required='required'><br><br><input type='submit' value="
           +
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
*   Return Value : uint8_t
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
uint8_t http_handle_update_esp8266(AsyncWebServerRequest *request,
                                   const String& filename,
                                   size_t index, uint8_t *data, size_t len, bool final)
{
    if (strstr(filename, ".bin") == NULL)
    {
        return local_update_finish;
    }
    if (!index)
    {

        Update.runAsync(true);
        if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000))
        {
            Update.printError(Serial1);

            return update_filesize_error;
        }
    }
    if (!Update.hasError())
    {
        if (Update.write(data, len) != len)
        {
            Update.printError(Serial1);
            return update_write_error;
        }
    }
    if (final)
    {

        if (Update.end(true))
        {
            Serial1.flush();

            shouldReboot = !Update.hasError();
            return update_ok;
        }
        else
        {
            Update.printError(Serial1);

            return update_check_error;
        }
    }

}

/*****************************************************************************
*	Prototype	 : http_cfgwifi_page
*	Description  : config wifi page
*	Input		 : None
*	Output		 : None
*	Return Value : String
*	Calls		 :
*	Called By	 :
*
*	History:
*
*		1.	Date		 : 2019/6/30
*			Author		 : zhuhongfei
*			Modification : Created function
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
    html = html +	"<p><label>" + zh_array[tel_wifi_ssid] +
           ":</label><input name='ssid' type='text'  required='required'  maxlength='16'><br></p>";
    html = html +	"<p><label>" + zh_array[tel_wifi_pwd] +
           ":</label><input  name='pwd'  type='text'  required='required'  maxlength='16'><br></p>";
    html = html +	"<p><label>" + zh_array[tel_wifi_pwd_cfm] +
           ":</label><input name='pwd' type='text' required='required'	maxlength='16'><br></p>";

    html = html +  "<input type='submit' value=" + zh_array[config_wifi] + ">";
    html += "</form></body></html>";
    return html;
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
        /*        AsyncWebParameter *p = request ->getParam(0);
                char update_flag[16];
                sprintf(update_flag, "%s", p->value().c_str());

                String html = "";
                html += "<html> <head><meta http-equiv='Content-Type' content='text/html ;charset=utf-8'/> </head>";


                if (!strcmp(update_flag, "esp8266"))
                {
                    html  = html + "<h1>" + zh_array[local_update_finish] + "</h1></html>";

                    shouldReboot = !Update.hasError();
                }
        		else
        		{
        			html  = html + "<h1>" + zh_array[update_host_req] + "</h1></html>";
        		}

        		request->send(200, "text/html", html.c_str());*/
    },
    [](AsyncWebServerRequest * request, const String & filename, size_t index,
       uint8_t *data, size_t len, bool final)
    {
        uint8_t update_recv_flag = 0;

        String html = "";
        html += "<html> <head><meta http-equiv='Content-Type' content='text/html ;charset=utf-8'/> </head>";

        AsyncWebParameter *p = request ->getParam(0);
        char update_flag[16];
        sprintf(update_flag, "%s", p->value().c_str());
        if (!strcmp(update_flag, "esp8266"))
        {
            update_recv_flag = http_handle_update_esp8266(request, filename, index, data,
                               len, final);

            html  = html + "<h1>" + zh_array[update_recv_flag] + "</h1></html>";
        }

        request->send(200, "text/html", html.c_str());
    });

    server.onNotFound(notFound);

    server.begin();
}
