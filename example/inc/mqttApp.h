/*
 * mqttApp.h
 *
 *  Created on: 07/07/2014
 *      Author: Henrique Ferreira Canova
 */

#ifndef MQTTAPP_H_
#define MQTTAPP_H_

#include "lwip/opt.h"
#include "lwip/stats.h"
#include "lwip/sys.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "lwip/tcp.h"
#include "lwip/dns.h"
#include "lwip/dhcp.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "netif/etharp.h"



void task (uint8_t *topico,uint8_t *msg);
err_t recv_callback(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err);
uint8_t mqttIsConnected();
void mqttAppSubscribe(char *topic);
err_t accept_callback(void *arg, struct tcp_pcb *npcb, err_t err);
void mqttAppInit();
void mqttAppConnect();
void mqttAppHandle();
void mqttAppDisconnect();
void mqttAppPublish(char *topic, char *data);

#endif /* MQTTAPP_H_ */
