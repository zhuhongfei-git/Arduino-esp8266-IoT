/******************************************************************************
*
*  Copyright (C), 2016-2036, REMO Tech. Co., Ltd.
*
*******************************************************************************
*  File Name     : ESP8266_IoT_OLED.ino
*  Version       : Initial Draft
*  Author        : zhuhongfei
*  Created       : 2019/10/10
*  Last Modified :
*  Description   : OLED(128*64,0.9),base on Arduino lib
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

#include <Wire.h>
#include <ACROBOTIC_SSD1306.h>

//#include <DHT.h>
/*==============================================*
 *      constants or macros define              *
 *----------------------------------------------*/
//DHT11
//#define DHTPIN    5
//#define DHTTYPE   DHT11



/*==============================================*
 *      project-wide global variables           *
 *----------------------------------------------*/
//DHT11
float DHT11_temperature = 0;
float DHT11_humidity	= 0;

//extern DHT dht;



/*==============================================*
 *      routines' or functions' implementations *
 *----------------------------------------------*/
/*****************************************************************************
*   Prototype    : OLED_display_init
*   Description  : initiation OLED 
*   Input        : void
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
void OLED_display_init( void )
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
    oled.setTextXY(5, 0);
    oled.putString("Total:       B");
    oled.setTextXY(6, 0);
    oled.putString("Used:       B");
    
    
}

/*****************************************************************************
*   Prototype    : OLED_run_function
*   Description  : display content
*   Input        : void
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
void OLED_run_function(  void)
{
    oled.setTextXY(3, 5);
    oled.putFloat(DHT11_temperature);

    oled.setTextXY(4, 5);
    oled.putFloat(DHT11_humidity);

    oled.setTextXY(5, 6);
    oled.putNumber(total_bytes);
    oled.setTextXY(6, 5);
    oled.putNumber(used_bytes);

}

/*****************************************************************************
*   Prototype    : DHT11_read_sensor
*   Description  : sample data
*   Input        : void
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
void DHT11_read_sensor( void )
{
    delay(2000);
    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    DHT11_humidity = dht.readHumidity();
    // Read temperature as Celsius (the default)
    DHT11_temperature = dht.readTemperature();


    // Check if any reads failed and exit early (to try again).
    if (isnan(DHT11_humidity) || isnan(DHT11_temperature))
    { 
        return;
    }
}
