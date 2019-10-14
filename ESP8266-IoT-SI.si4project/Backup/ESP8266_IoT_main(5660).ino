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



/*==============================================*
 *      routines' or functions' implementations *
 *----------------------------------------------*/

void setup() {
	// put your setup code here, to run once:
	setup_WiFi_mode();
	
	OLED_display_init();
	dht.begin();

	
}

void loop() {
	// put your main code here, to run repeatedly:
	DHT11_read_sensor();
	OLED_run_function();
}
