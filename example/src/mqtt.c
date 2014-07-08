/*
 * mqttApp.h
 *
 *  Created on: 07/07/2014
 *      Author: Henrique Ferreira Canova
 */


#include "mqtt.h"
#include <stdint.h>
#include <string.h>




#include "board.h"

void mqttDisconnectForced(Mqtt *this)
{
	if(!this->connected)
		return;

	tcp_abort(this->pcb);
	this->connected = 0;
}


void mqttInit(Mqtt *this, struct ip_addr serverIp, int port, tcp_accept_fn  fn, char *devId)

{
	this->tcp_fun = fn;
	this->pcb = tcp_new();
	this->server = serverIp;
	this->port = port;
	this->connected = 0;
	this->deviceId = devId;
	this->liberado =0;

}



int16_t mqttConnect(Mqtt *this)

{

    err_t err= tcp_connect(this->pcb, &(this->server), 1883, this->tcp_fun);

    if(err != ERR_OK)
    {

    	DEBUGOUT("Connection Error : %d\r\n",err);
        return 1;
    }
    else
    {
    	DEBUGOUT("Connection sucessed..\r\n");
    }



    tcp_accept(this->pcb, this->tcp_fun);



    uint8_t var_header[] = {0x00,0x06,0x4d,0x51,0x49,0x73,0x64,0x70,0x03,0x02,0x00,KEEPALIVE/500,0x00,strlen(this->deviceId)};
    uint8_t fixed_header[] = {MQTTCONNECT,12+strlen(this->deviceId)+2};



    char packet[sizeof(fixed_header)+sizeof(var_header)+strlen(this->deviceId)];


    memset(packet,0,sizeof(packet));


    memcpy(packet,fixed_header,sizeof(fixed_header));

    memcpy(packet+sizeof(fixed_header),var_header,sizeof(var_header));

    memcpy(packet+sizeof(fixed_header)+sizeof(var_header),this->deviceId,strlen(this->deviceId));


    err = tcp_write(this->pcb, (void *)packet, sizeof(packet),  1);


    if (err == ERR_OK)
    {

    	tcp_output(this->pcb);

        DEBUGOUT("Identificaiton message sended correctlly...\n");

        this->connected =1 ;

        return 1;

    }
    else
    {

    	DEBUGOUT("Failed to send the identification message to broker...\n");
    	DEBUGOUT("Error is: %d\n",err);
    	tcp_close(this->pcb);
        return -2;
    }



    return 1;
}


uint8_t mqttPublish(Mqtt *this,char* pub_topic, char* msg)
{


    uint8_t var_header_pub[strlen(pub_topic)+3];
    strcpy((char *)&var_header_pub[2], pub_topic);
    var_header_pub[0] = 0;
    var_header_pub[1] = strlen(pub_topic);
    var_header_pub[sizeof(var_header_pub)-1] = 0;

    uint8_t fixed_header_pub[] = {MQTTPUBLISH,sizeof(var_header_pub)+strlen(msg)};

    uint8_t packet_pub[sizeof(fixed_header_pub)+sizeof(var_header_pub)+strlen(msg)];
    memset(packet_pub,0,sizeof(packet_pub));
    memcpy(packet_pub,fixed_header_pub,sizeof(fixed_header_pub));
    memcpy(packet_pub+sizeof(fixed_header_pub),var_header_pub,sizeof(var_header_pub));
    memcpy(packet_pub+sizeof(fixed_header_pub)+sizeof(var_header_pub),msg,strlen(msg));


    err_t err = tcp_write(this->pcb, (void *)packet_pub, sizeof(packet_pub), 1); //TCP_WRITE_FLAG_MORE

    if (err == ERR_OK) {
        tcp_output(this->pcb);
       DEBUGOUT("Publish: %s ...\r\n", msg);
    } else {
    	DEBUGOUT("Failed to publish...\r\n");
    	DEBUGOUT("Error is: %d\r\n",err);
    	mqttDisconnectForced(this);
        return 1;
    }

    return 0;
}



uint8_t mqttDisconnect(Mqtt *this)
{

	if(!this->connected)
		return 1;

    uint8_t packet_224[] = {2,2,4};
    tcp_write(this->pcb, (void *)packet_224, sizeof(packet_224), 1);
    tcp_write(this->pcb, (void *)(0), sizeof((0)), 1);
    tcp_close(this->pcb);
    this->lastActivity = SysTick_GetMS();
    this->connected = 0;

    return 0;
}

uint8_t mqttSubscribe(Mqtt *this, char* topic)
{

    if (!this->connected)
    	return -1;

        uint8_t var_header_topic[] = {0,10};
        uint8_t fixed_header_topic[] = {MQTTSUBSCRIBE,sizeof(var_header_topic)+strlen(topic)+3};

        // utf topic
        uint8_t utf_topic[strlen(topic)+3];
        strcpy((char *)&utf_topic[2], topic);

        utf_topic[0] = 0;
        utf_topic[1] = strlen(topic);
        utf_topic[sizeof(utf_topic)-1] = 0;

        char packet_topic[sizeof(var_header_topic)+sizeof(fixed_header_topic)+strlen(topic)+3];
        memset(packet_topic,0,sizeof(packet_topic));
        memcpy(packet_topic,fixed_header_topic,sizeof(fixed_header_topic));
        memcpy(packet_topic+sizeof(fixed_header_topic),var_header_topic,sizeof(var_header_topic));
        memcpy(packet_topic+sizeof(fixed_header_topic)+sizeof(var_header_topic),utf_topic,sizeof(utf_topic));

        //Send message
        err_t err = tcp_write(this->pcb, (void *)packet_topic, sizeof(packet_topic), 1); //TCP_WRITE_FLAG_MORE
        if (err == ERR_OK) {
            tcp_output(this->pcb);

        } else {

            mqttDisconnectForced(this);
            return 1;
        }


        return 0;

}



int mqttPing(Mqtt *this)
{
	MqttFixedHeader pingReq;

	if(!this->connected)
		return -1;

	pingReq.header = MQTT_PINGREQ_HEADER;
	pingReq.remainingLength = 0x00;

    //Publish message
    err_t err = tcp_write(this->pcb, (void *)&pingReq, sizeof(pingReq), 1); //TCP_WRITE_FLAG_MORE

    if (err == ERR_OK)
    {
        tcp_output(this->pcb);
      DEBUGOUT("Pinreq...\r\n");
    }
    else
    {

    	mqttDisconnectForced(this);
        return -1;
    }


    return 1;
}

uint8_t mqttLive(Mqtt *this)
{


	uint32_t t = SysTick_GetMS();
	if (t - this->lastActivity > (KEEPALIVE))
	{

		if (this->connected)
	  {
			DEBUGOUT("Sending keep-alive\n");
			mqttPing(this);


		} else if(this->autoConnect){
			mqttConnect(this);
		}

		this->lastActivity = t;
	}



    return 0;
}


