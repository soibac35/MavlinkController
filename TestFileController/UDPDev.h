#pragma once
#include<winsock2.h>

struct RxFile;
struct RingBuffer;
struct PayloadBlock;

struct UDPDev
{
	int stage;
	int nextStage;
	int	resendCnt;
	double lastTXTime;
	double lastRXTime;

	BOOL bConnected;
	SOCKET sock;
	RxFile* pRxFile;
	RingBuffer* pRxBuf;
	mavlink_message_t lastMsg;

	struct sockaddr_in remAddr;
};

UDPDev* udp_dev_create(char* ipAddr, unsigned int port);
void udp_dev_close(UDPDev* dev);
void udp_dev_update(UDPDev* dev);
void udp_dev_update(UDPDev* dev, mavlink_message_t* pMsg);
void udp_dev_send_msg(UDPDev* dev, mavlink_message_t* msg);
void udp_dev_send_msg_to(UDPDev* dev, mavlink_message_t* msg);
void udp_dev_send_msg_no_ack(UDPDev* dev, mavlink_message_t* msg);
void udp_dev_get_heartbeat(UDPDev* dev, mavlink_message_t* msg);
void udp_dev_get_file(UDPDev* dev, mavlink_message_t* msg);
void udp_dev_send_file_block(UDPDev* dev, PayloadBlock* block);

double dTime();
