/******************************************************************************
*
*  Copyright (C), 2016-2036, REMO Tech. Co., Ltd.
*
*******************************************************************************
*  File Name     : ESP8266_IoT_TCP.ino
*  Version       : Initial Draft
*  Author        : zhuhongfei
*  Created       : 2019/10/15
*  Last Modified :
*  Description   : create TCP server
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
#define WIFI_SRV_PORT     65432
#define MAX_SRV_CLIENTS   2


/*==============================================*
 *      project-wide global variables           *
 *----------------------------------------------*/
WiFiServer data_server(WIFI_SRV_PORT);
WiFiClient serverClients[MAX_SRV_CLIENTS];



/*==============================================*
 *      routines' or functions' implementations *
 *----------------------------------------------*/

 /*****************************************************************************
 *	 Prototype	  : start_data_server
 *	 Description  : start tcpserver
 *	 Input		  :
 *	 Output 	  : None
 *	 Return Value : void
 *	 Calls		  :
 *	 Called By	  :
 *
 *	 History:
 *
 *		 1.  Date		  : 2019/6/5
 *			 Author 	  : zhuhongfei
 *			 Modification : Created function
 *
 *****************************************************************************/
 void start_data_server()
 {
	 data_server.begin();
	 data_server.setNoDelay(true);
 }


 /*****************************************************************************
 *	 Prototype	  : uart_to_tcp
 *	 Description  : tcpserver send data from serial to tcpclient
 *	 Input		  :
 *	 Output 	  : None
 *	 Return Value : void
 *	 Calls		  :
 *	 Called By	  :
 *
 *	 History:
 *
 *		 1.  Date		  : 2019/6/5
 *			 Author 	  : zhuhongfei
 *			 Modification : Created function
 *
 *****************************************************************************/
 void uart_to_tcp()
 {
	 uint8_t i;
	 //check request from new client
	 if (data_server.hasClient())
	 {
		 for (i = 0; i < MAX_SRV_CLIENTS; i++)
		 {
			 //release valid or unconnected client
			 if (!serverClients[i] || !serverClients[i].connected())
			 {
				 if (serverClients[i])
				 {
					 serverClients[i].stop();
				 }
				 //assign new client
				 serverClients[i] = data_server.available();

				 break;
			 }
		 }
		 //reject to connect when to reach the max connected number
		 if (i == MAX_SRV_CLIENTS)
		 {
			 WiFiClient serverClient = data_server.available();
			 serverClient.stop();

		 }
	 }
	 //check data from client
	 for (i = 0; i < MAX_SRV_CLIENTS; i++)
	 {
		 if (serverClients[i] && serverClients[i].connected())
		 {
			 if (serverClients[i].available())
			 {
				 //get data from the telnet client and push it to the UART
				 while (serverClients[i].available())
				 {
					 //send to serial
					 Serial.write(serverClients[i].read());
				 }
			 }
		 }
	 }
 	 //send data from serial to client
	 if (Serial.available())
	 {	 
		 size_t len = Serial.available();
		 uint8_t sbuf[len];
		 Serial.readBytes(sbuf, len);
 
		 //push UART data to all connected telnet clients
		 for (i = 0; i < MAX_SRV_CLIENTS; i++)
		 {
			 if (serverClients[i] && serverClients[i].connected())
			 {
				 serverClients[i].write(sbuf, len);
				 //delay(1);
			 }
		 }
 
	 }
 
 }

