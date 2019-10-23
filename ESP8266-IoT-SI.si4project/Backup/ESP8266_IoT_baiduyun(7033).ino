/******************************************************************************
*
*  Copyright (C), 2016-2036, REMO Tech. Co., Ltd.
*
*******************************************************************************
*  File Name     : ESP8266_IoT_baiduyun.ino
*  Version       : Initial Draft
*  Author        : zhuhongfei
*  Created       : 2019/10/18
*  Last Modified :
*  Description   : baiduyun Arduino esp8266
*  Function List :
*
*
*  History:
*
*       1.  Date         : 2019/10/18
*           Author       : zhuhongfei
*           Modification : Created file
*
******************************************************************************/

/*==============================================*
 *      include header files                    *
 *----------------------------------------------*/
#include <TaskScheduler.h>
#include <PubSubClient.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>

#include <ESP8266HTTPClient.h>

/*==============================================*
 *      constants or macros define              *
 *----------------------------------------------*/
#define IOT_PORT 1883


/*==============================================*
 *      project-wide global variables           *
 *----------------------------------------------*/
const char *IOT_ENDPOINT  = "q47g5qy.mqtt.iot.gz.baidubce.com";
const char *IOT_USERNAME  = "q47g5qy/ESP8266_V1d0";
const char *IOT_KEY 	  = "30fvxh1cszwsq96h";

const char *IOT_TOPIC_SUBSCR_DOC  =
    "$baidu/iot/shadow/ESP8266_V1d0/update/snapshot";

const char *IOT_TOPIC_PUBLISH	  = "$baidu/iot/shadow/ESP8266_V1d0/update";
const char *IOT_TOPIC_SUBSCR_REJ  =
    "$baidu/iot/shadow/ESP8266_V1d0/update/rejected";
const char *IOT_TOPIC_SUBSCR_ACC  =
    "$baidu/iot/shadow/ESP8266_V1d0/update/accepted";
const char *IOT_TOPIC_SUBSCR_DTA  = "$baidu/iot/shadow/ESP8266_V1d0/delta";
const char *IOT_TOPIC_SUBSCR_REQ  =
    "$baidu/iot/shadow/ESP8266_V1d0/method/device/req";
const char *IOT_TOPIC_SUBSCR_RESP =
    "$baidu/iot/shadow/ESP8266_V1d0/method/cloud/resp";

const char *IOT_TOPIC_PUBLISH_REQ =
    "$baidu/iot/shadow/ESP8266_V1d0/method/cloud/req";
const char *IOT_TOPIC_PUBLISH_DEVICE =
    "$baidu/iot/shadow/ESP8266_V1d0/method/device/resp";

const char *IOT_CLIENT_ID = "ESP8266_V1d0";

char *LED_status = "OFF";

WiFiClient client;
HTTPClient http;

typedef struct update_itself
{
    uint16_t update_flag;
    char     cur_version[8];
} Update_CMD;

Update_CMD update_command;


void mqtt_callback(char *topic, byte *payload, unsigned int length);

//PubSubClient mqttclient(IOT_ENDPOINT, IOT_PORT, &mqtt_callback, client);
PubSubClient mqttclient(client);



void ticker_handler();

Task schedule_task(5000, TASK_FOREVER, &ticker_handler);
Scheduler runner;

void parse_mqtt_callback(char *src_string);



/*==============================================*
 *      routines' or functions' implementations *
 *----------------------------------------------*/

void begin_httpclient()
{

    if (http.begin(client, "http://jigsaw.w3.org/HTTP/connection.html"))
    {
        Serial.print("[HTTP] GET...\n");
        // start connection and send HTTP header
        int httpCode = http.GET();

        // httpCode will be negative on error
        if (httpCode > 0)
        {
            // HTTP header has been send and Server response header has been handled
            Serial.printf("[HTTP] GET... code: %d\n", httpCode);

            // file found at server
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
            {
                String payload = http.getString();
                Serial.println(payload);
            }
        }
        else
        {
            Serial.printf("[HTTP] GET... failed, error: %s\n",
                          http.errorToString(httpCode).c_str());
        }

        http.end();
    }
    else
    {
        Serial.printf("[HTTP} Unable to connect\n");
    }
}
/*****************************************************************************
*   Prototype    : parse_mqtt_callback
*   Description  : parse mqtt
*   Input        : char *src_string
*   Output       : None
*   Return Value : void
*   Calls        :
*   Called By    :
*
*   History:
*
*       1.  Date         : 2019/10/19
*           Author       : zhuhongfei
*           Modification : Created functionsnapshot
*
*****************************************************************************/
void parse_mqtt_callback(char *src_string)
{

    StaticJsonDocument<512> docu;
    StaticJsonDocument<512> req_doc;


    // Deserialize the JSON document
    DeserializationError error = deserializeJson(docu, src_string);
    //JsonObject souece_key = docu.createNestedObject();
    if (error)
    {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
        return;
    }
    if (docu.containsKey("desired"))
    {
        //Serial.println("has_desired");
        JsonObject desired = docu["desired"];
        if (desired.containsKey("LED"))
        {
            //Serial.println("has_LED");
            const char *led_sta = desired["LED"];
            if (!strcmp(led_sta, "ON"))
            {
                LED_status = "ON";
                digitalWrite(led_pin, LOW);

            }
            else if (!strcmp(led_sta, "OFF"))
            {
                LED_status = "OFF";
                digitalWrite(led_pin, HIGH);
            }
        }
    }
    else if (docu.containsKey("methodName"))
    {
        //Serial.println("has_methodName");
        const char *method_name = docu["methodName"];
        if (!strcmp(method_name, "doFirmwareUpdate"))
        {

            JsonObject  payload = docu["payload"];
            const char *jobId = payload["jobId"];
            const char *update_url = payload["firmwareUrl"];

            strcpy(update_command.cur_version, payload["firmwareVersion"])

            req_doc["methodName"] = docu["methodName"];
            req_doc["requestId"] = docu["requestId"];
            req_doc["status"] = 200;
            char resp[measureJson(req_doc) + 1];

            serializeJson(req_doc, resp, measureJson(req_doc) + 1);

            mqttclient.publish(IOT_TOPIC_PUBLISH_DEVICE, resp);



            //url


            //update file has been download,feeddback to cloud platform
            //start update
            update_command.update_flag = 0x5555;

            req_doc.clear();
            req_doc["requestId"] = "123456";
            req_doc["methodName"] = "reportFirmwareUpdateStart";
            JsonObject pay2load = req_doc.createNestedObject("payload");
            pay2load["jobId"] = jobId;
            char req_start[measureJson(req_doc) + 1];

            serializeJson(req_doc, req_start, measureJson(req_doc) + 1);

            mqttclient.publish(IOT_TOPIC_PUBLISH_REQ, req_start);

            delay(5000);



            //finish update
            req_doc.clear();
            req_doc["requestId"] = "123456";
            req_doc["methodName"] = "reportFirmwareUpdateResult";
            JsonObject pay3load = req_doc.createNestedObject("payload");
            pay3load["result"] = "success";
            pay3load["jobId"] = jobId;
            char req_result[measureJson(req_doc) + 1];

            serializeJson(req_doc, req_result, measureJson(req_doc) + 1);

            mqttclient.publish(IOT_TOPIC_PUBLISH_REQ, req_result);

            update_command.update_flag = 0xAAAA;
        }
    }


}
/*****************************************************************************
*   Prototype    : mqtt_callback
*   Description  : mqtt server send data to esp8266 client
*   Input        : char* topic
*                  byte* payload
*                  unsigned int length
*   Output       : None
*   Return Value : void
*   Calls        :
*   Called By    :
*
*   History:
*
*       1.  Date         : 2019/10/18
*           Author       : zhuhongfei
*           Modification : Created function
*
*****************************************************************************/
void mqtt_callback(char *topic, byte *payload, unsigned int length)
{
    byte *end = payload + length;
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    Serial.println();
    Serial.println(String(length));

    for (byte *p = payload; p < end; ++p)
    {
        Serial.print(*((const char *)p));
    }

    Serial.println("");
    //char* store_feedback_msg = NULL;
    char *store_feedback_msg = (char *)malloc(length);
    memcpy(store_feedback_msg, (const char *)payload, length);

    parse_mqtt_callback(store_feedback_msg);


    //Free memory
    free(store_feedback_msg);


}

/*****************************************************************************
*   Prototype    : ticker_handler
*   Description  : handle mqtt client connect
*   Input        : None
*   Output       : None
*   Return Value : void t
*   Calls        :
*   Called By    :
*
*   History:
*
*       1.  Date         : 2019/10/18
*           Author       : zhuhongfei
*           Modification : Created function
*
*****************************************************************************/
void ticker_handler()
{
    StaticJsonDocument<256> document;


    if (!mqttclient.connected())
    {
        Serial.print(F("MQTT state: "));
        Serial.println(mqttclient.state());
        //String clientid {IOT_CLIENT_ID + String{random(10000)}};
        if (mqttclient.connect(IOT_CLIENT_ID, IOT_USERNAME, IOT_KEY))
        {
            Serial.print(F("MQTT Connected. Client id = "));
            Serial.println(IOT_CLIENT_ID);

            mqttclient.subscribe(IOT_TOPIC_SUBSCR_REQ);
            mqttclient.subscribe(IOT_TOPIC_SUBSCR_RESP);
            mqttclient.subscribe(IOT_TOPIC_SUBSCR_REJ);
            //mqttclient.subscribe(IOT_TOPIC_SUBSCR_ACC);
            mqttclient.subscribe(IOT_TOPIC_SUBSCR_DTA);
            //mqttclient.subscribe(IOT_TOPIC_SUBSCR_DOC);

            strcpy(update_command.cur_version, "1.0.1");
            document["requestId"] = "12345";
            document["methodName"] = "reportCurrentFirmwareInfo";
            JsonObject payload = document.createNestedObject("payload");
            payload["firmwareVersion"] = update_command.cur_version;
            char version[measureJson(document) + 1];

            serializeJson(document, version, measureJson(document) + 1);

            mqttclient.publish(IOT_TOPIC_PUBLISH_REQ, version);
        }
    }
    else
    {
        if (update_command.update_flag != 0x5555)
        {
            JsonObject reported = document.createNestedObject("reported");
            reported["Temper"]	= DHT11_temperature;
            reported["Humi"]	= DHT11_humidity;
            reported["LED"] 	= LED_status;

            char myDoc[measureJson(document) + 1];

            serializeJson(document, myDoc, measureJson(document) + 1);

            mqttclient.publish(IOT_TOPIC_PUBLISH, myDoc);
        }

    }
}

/*****************************************************************************
*   Prototype    : run_task_on_mqttclient
*   Description  : start a task about mqtt client connect server
*   Input        : None
*   Output       : None
*   Return Value : void
*   Calls        :
*   Called By    :
*
*   History:
*
*       1.  Date         : 2019/10/18
*           Author       : zhuhongfei
*           Modification : Created function
*
*****************************************************************************/
void run_task_on_mqttclient()
{
    mqttclient.setServer(IOT_ENDPOINT, IOT_PORT);
    mqttclient.setCallback(mqtt_callback);

    runner.init();
    runner.addTask(schedule_task);
    schedule_task.enable();


}

/*****************************************************************************
*   Prototype    : run_mqtt_task_loop
*   Description  : loop execute mqtt task
*   Input        : None
*   Output       : None
*   Return Value : void
*   Calls        :
*   Called By    :
*
*   History:
*
*       1.  Date         : 2019/10/18
*           Author       : zhuhongfei
*           Modification : Created function
*
*****************************************************************************/
void run_mqtt_task_loop()
{
    mqttclient.loop();
    runner.execute();

}
