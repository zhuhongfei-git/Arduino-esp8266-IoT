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
const char *IOT_TOPIC_PUBLISH	  = "$baidu/iot/shadow/ESP8266_V1d0/update";
const char *IOT_TOPIC_SUBSCR_REJ  = "$baidu/iot/shadow/ESP8266_V1d0/update/rejected";
const char *IOT_TOPIC_SUBSCR_ACC  = "$baidu/iot/shadow/ESP8266_V1d0/update/accepted";
const char *IOT_TOPIC_SUBSCR_DTA  = "$baidu/iot/shadow/ESP8266_V1d0/delta";
const char *IOT_TOPIC_SUBSCR_REQ  = "$baidu/iot/shadow/ESP8266_V1d0/method/device/req";
const char *IOT_TOPIC_SUBSCR_RESP = "$baidu/iot/shadow/ESP8266_V1d0/method/cloud/resp";

const char *IOT_TOPIC_PUBLISH_REQ = "$baidu/iot/shadow/ESP8266_V1d0/method/cloud/req";

const char *IOT_CLIENT_ID = "ESP8266_V1d0";

char *LED_status = "OFF";

WiFiClient client;


void mqtt_callback(char *topic, byte *payload, unsigned int length);

PubSubClient mqttclient(IOT_ENDPOINT, IOT_PORT, &mqtt_callback, client);


void ticker_handler();

Task schedule_task(10000, TASK_FOREVER, &ticker_handler);
Scheduler runner;

void parse_mqtt_callback(char *src_string);



/*==============================================*
 *      routines' or functions' implementations *
 *----------------------------------------------*/
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
*           Modification : Created function
*
*****************************************************************************/
void parse_mqtt_callback(char *src_string)
{
	if(strstr(src_string,"desired") != NULL)
	{
		if(strstr(src_string,"\"LED\":\"ON\"") != NULL)
		{
			LED_status = "ON";
			digitalWrite(led_pin, LOW);

			mqttclient.publish(IOT_TOPIC_PUBLISH, send_buffer.c_str());
		}
		else if(strstr(src_string,"\"LED\":\"OFF\"") != NULL)
		{
			LED_status = "OFF";
			digitalWrite(led_pin, HIGH);
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
*   Return Value : void m
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
    //char* store_feedback_msg = NULL;
	char *store_feedback_msg = (char *)malloc(length);
    memcpy(store_feedback_msg,(const char *)payload,length);

    parse_mqtt_callback(store_feedback_msg);

	//Serial.println(*(store_feedback_msg));
    //Free memory
	free(store_feedback_msg);

    for (byte *p = payload; p < end; ++p)
    {
        Serial.print(*((const char *)p));
    }
    
    Serial.println("");
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
    if (!mqttclient.connected())
    {
        Serial.print(F("MQTT state: "));
        Serial.println(mqttclient.state());
        //String clientid {IOT_CLIENT_ID + String{random(10000)}};
        if (mqttclient.connect(IOT_CLIENT_ID, IOT_USERNAME, IOT_KEY))
        {
            Serial.print(F("MQTT Connected. Client id = "));
            Serial.println(IOT_CLIENT_ID);
            mqttclient.subscribe(IOT_TOPIC_SUBSCR_REJ);
            mqttclient.subscribe(IOT_TOPIC_SUBSCR_ACC);
            mqttclient.subscribe(IOT_TOPIC_SUBSCR_DTA);
            mqttclient.subscribe(IOT_TOPIC_SUBSCR_REQ);
            mqttclient.subscribe(IOT_TOPIC_SUBSCR_RESP);
        }
    }
    else
    {
        static int counter = 0;
        //String buffer {"MQTT message from Arduino: " + String{counter++}};


        String send_buffer {R"rawliteral({"reported": {"Temper":)rawliteral" +
        		String{DHT11_temperature} +R"rawliteral(,"Humi":)rawliteral" + 
        		String(DHT11_humidity)    + R"rawliteral(,"LED":)rawliteral" + "\"" +
        		LED_status + "\"" + R"rawliteral(}})rawliteral"};
        //const char *send_buffer	= R"rawliteral({"reported": {"Temper": 26,"Humi": 45}})rawliteral";  //test success
        mqttclient.publish(IOT_TOPIC_PUBLISH, send_buffer.c_str());
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
