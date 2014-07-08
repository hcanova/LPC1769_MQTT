/*
 * mqttApp.h
 *
 *  Created on: 07/07/2014
 *      Author: Henrique Ferreira Canova
 */


#include "mqttApp.h"


#include "mqtt.h"
#include <stdint.h>

#include "string.h"
#include "board.h"


Mqtt mqtt;

uint8_t liberado ;



void task (uint8_t *topico,uint8_t *msg)
{
	if(!strcmp(topico,"mbed"))
	{
		if (!strcmp(msg,"on"))
		{
			Board_LED_Set(0, 1);
			mqttAppPublish("lpc","LIGADO");

		}
		if (!strcmp(msg,"off"))
		{
			Board_LED_Set(0, 0);
			mqttAppPublish("lpc","DESLIGADO");
		}

	}

}

err_t recv_callback(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)

{


    char *mqttdata;

    /* Check if status is ok and data is arrived. */
    if (err == ERR_OK && p != NULL)
    {
        /* Inform TCP that we have taken the data. */
        tcp_recved(pcb, p->tot_len);

        mqttdata = (uint8_t*)(p->payload);



        	uint8_t *topic = mqttdata + 2 + 2;//ok
     		uint16_t topicLen = (mqttdata[2] << 8) | mqttdata[3];
     		uint8_t *data = &mqttdata[2+2+topicLen];
     		uint32_t dataLen = p->tot_len - (2 + 2 + topicLen);



     		uint8_t strTopic[topicLen + 1];
     		uint8_t strData[dataLen + 1];

         	switch(mqttdata[0] & 0xF0)
         	{
         	case MQTT_MSGT_PINGRESP:
         			DEBUGOUT("PingResp\n");
         		break;

         	case MQTT_MSGT_PUBLISH:

         		memcpy(strTopic, topic, topicLen);
         		strTopic[topicLen] = '\0';


         		DEBUGOUT("TOPICO %s\n\r",strTopic);

         		memcpy(strData, data, dataLen);
         		strData[dataLen] = '\0';
         		DEBUGOUT("DATA %s\n\r",strData);

         		task(strTopic,strData);

         		break;

         	case MQTT_MSGT_CONACK:
         			liberado = 1;
         		break;

         	default:
         		DEBUGOUT("default:\n");
         	}






    }
    else
    {
        /* No data arrived */
        /* That means the client closes the connection and sent us a packet with FIN flag set to 1. */
        /* We have to cleanup and destroy out TCPConnection. */
    	DEBUGOUT("Connection closed by client.\r\n");
        tcp_close(pcb);
    }
    pbuf_free(p);
    return ERR_OK;
}


err_t accept_callback(void *arg, struct tcp_pcb *npcb, err_t err)

{


    LWIP_UNUSED_ARG(arg);

    DEBUGOUT("Recieve from broker...\r\n");
    DEBUGOUT("\r\n");

    /* Subscribe a receive callback function */
    tcp_recv(npcb, &recv_callback);

    /* Don't panic! Everything is fine. */
    return ERR_OK;
}




void mqttAppInit()
{
	struct ip_addr serverIp;
	liberado = 0;
	IP4_ADDR(&serverIp, 192,168,0,7);
	mqttInit(&mqtt, serverIp, 1883, &accept_callback, "LPC");

}

void mqttAppConnect()
{
   mqttConnect(&mqtt);
}

uint8_t mqttIsConnected()
{
	return liberado;
}

void mqttAppPublish(char *topic, char *data)
{
	mqttPublish(&mqtt, topic, data);
}


void mqttAppSubscribe(char *topic)
{
	mqttSubscribe(&mqtt, topic);
}

void mqttAppDisconnect()
{
	mqttDisconnect(&mqtt);
}


void mqttAppHandle()
{
	mqttLive(&mqtt);
}
