#include "stdafx.h"
#include "UDPDev.h"
#include "RingBuff.h"
#include "RxFile.h"
#include "Util.h"


#pragma comment(lib,"ws2_32.lib") //Winsock Library

#define SEND_BROADCAST
#define REMOTE_PORT	10000
#define SERVER_PORT 10443
#define DAT_BUFF_SIZE 512
#define RX_FILE_TIMEOUT  20000 //20 sec


#define MAV_SYSTEM_ID		1   //mavlink protocol parameters
#define MAV_COMPONENT_ID	1

const int NET_IDLE = 0;
const int NET_WAIT4TX = 1;
const int NET_WAIT4ANS = 2;
const int NET_DISCONN = 3;
const int NET_RESEND = 4;



//struct sockaddr_in addr;

UDPDev* udp_dev_create(char* ipAddr, unsigned int port)
{
	//http://www.programminglogic.com/example-of-client-server-program-in-c-using-sockets-and-tcp/
	//https://www.cs.rutgers.edu/~pxk/417/notes/sockets/udp.html
	//https://www.codeproject.com/Articles/11740/A-simple-UDP-time-server-and-client-for-beginners
	//http://www.binarytides.com/udp-socket-programming-in-winsock/

	SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (sock == INVALID_SOCKET)
	{
		printf("Device: could not create socket : %d\n", WSAGetLastError());
		return NULL;
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	//addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_addr.s_addr = inet_addr(ipAddr);
	addr.sin_port = htons(SERVER_PORT);

	if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		printf("Device: bind failed with error code : %d", WSAGetLastError());
		closesocket(sock);
		return NULL;
	}

	struct sockaddr_in remAddr;
#ifdef SEND_BROADCAST
	//https://gist.github.com/chancyWu/8349411
	remAddr.sin_family = AF_INET;
	remAddr.sin_addr.s_addr = INADDR_BROADCAST;
	remAddr.sin_port = htons(REMOTE_PORT);

	// set socket options enable broadcast
	int broadcastEnable = 1;
	int ret = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char *)&broadcastEnable, sizeof(broadcastEnable));
	if (ret) 
	{
		printf("Error: Could not open set socket to broadcast mode\n");
		closesocket(sock);
		return NULL;
	}
#else
	remAddr.sin_family = AF_INET;
	remAddr.sin_addr.s_addr = inet_addr(ipAddr);
	remAddr.sin_port = htons(REMOTE_PORT);
#endif

	UDPDev* udpDev = new UDPDev;
	udpDev->sock = sock;
	udpDev->bConnected = 0;
	udpDev->resendCnt = 0;
	udpDev->lastTXTime = 0;
	udpDev->lastRXTime = 0;
	udpDev->stage = NET_IDLE;
	udpDev->pRxBuf = msg_buf_create(2048);
	udpDev->pRxFile = NULL;
	udpDev->remAddr = remAddr;

	return udpDev;
}

void udp_dev_close(UDPDev* dev)
{
	closesocket(dev->sock);

	if (dev->pRxFile)
	{
		rx_file_destroy(dev->pRxFile);

		dev->pRxFile = NULL;
	}

	if (dev->pRxBuf)
	{
		msg_buf_destroy(dev->pRxBuf);

		delete dev->pRxBuf;
		dev->pRxBuf = NULL;
	}
}

void udp_dev_read(UDPDev* dev)
{
	fd_set rdset;
	FD_ZERO(&rdset);
	FD_SET(dev->sock, &rdset);

	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 10000;

	select(NULL, &rdset, NULL, NULL, &timeout);

	if (FD_ISSET(dev->sock, &rdset))
	{
		struct sockaddr_in remAddr;
		int addrLen = sizeof(remAddr);
		char buf[DAT_BUFF_SIZE];

		int recvLen = recvfrom(dev->sock, buf, DAT_BUFF_SIZE, 0, (struct sockaddr *)&remAddr, &addrLen);
		if (recvLen > 0)
		{
			if (remAddr.sin_port != htons(SERVER_PORT))
			{
				msg_buf_add(dev->pRxBuf, buf, recvLen);

				dev->remAddr = remAddr; //10/8/17 
			}
		}
		else if (recvLen == -1)
		{
			int errCode = WSAGetLastError();
			printf("Device: get error code %d\n", errCode);
		}
	}
}

void udp_dev_update(UDPDev* dev)
{
	udp_dev_read(dev);

	while (1)
	{
		mavlink_message_t* pMsg = msg_buf_get_msg(dev->pRxBuf);

		if (!pMsg)
			break;

		udp_dev_update(dev, pMsg);
	}

	if (dev->pRxFile && (dTime() - dev->pRxFile->lastRxTime > RX_FILE_TIMEOUT))
	{
		printf("Recv file timeout\n");

		rx_file_destroy(dev->pRxFile);

		dev->pRxFile = NULL;
	}
}

void udp_dev_update(UDPDev* dev, mavlink_message_t* pMsg)
{
	switch (pMsg->msgid)
	{
	case MAVLINK_MSG_ID_HEARTBEAT: //#0
		udp_dev_get_heartbeat(dev, pMsg);
		break;

	case MAVLINK_MSG_ID_FILE_TRANSFER_PROTOCOL:
		udp_dev_get_file(dev, pMsg);
		break;
	}
}

void udp_dev_get_heartbeat(UDPDev* dev, mavlink_message_t* msg)
{
	//test
	{
		static double lastRxHeartTime = 0;
		double dt = dTime() - lastRxHeartTime;

		if (lastRxHeartTime > 0)
		{
			//double dt = dTime() - lastRxHeartTime;
			printf("Heart beat %.1f s\n", dt / 1000);
		}
		else
			printf("Heart beat\n");

		lastRxHeartTime = dTime();
	}

	//if (dt > 1000)
	{
		mavlink_message_t msg;
		mavlink_msg_heartbeat_pack(MAV_SYSTEM_ID, MAV_COMPONENT_ID, &msg, MAV_TYPE_GENERIC, MAV_AUTOPILOT_GENERIC, MAV_MODE_GUIDED_ARMED, 0, MAV_STATE_ACTIVE);
		udp_dev_send_msg_no_ack(dev, &msg);
	}
}

void udp_dev_send_msg(UDPDev* dev, mavlink_message_t* msg)
{
	udp_dev_send_msg_to(dev, msg);

	memcpy(&dev->lastMsg, msg, sizeof(mavlink_message_t));

	dev->resendCnt = 0;
}

void udp_dev_send_msg_to(UDPDev* dev, mavlink_message_t* msg)
{
	unsigned char buf[MAVLINK_MAX_PACKET_LEN];
	int bufSize = mavlink_msg_to_send_buffer(buf, msg);

	sendto(dev->sock, (const char*)buf, bufSize, 0, (struct sockaddr *)&dev->remAddr, sizeof(sockaddr_in));

	dev->stage = NET_WAIT4ANS;
	dev->lastTXTime = dTime();
}

void udp_dev_send_msg_no_ack(UDPDev* dev, mavlink_message_t* msg)
{
	unsigned char buf[MAVLINK_MAX_PACKET_LEN];
	int bufSize = mavlink_msg_to_send_buffer(buf, msg);

	sendto(dev->sock, (const char*)buf, bufSize, 0, (struct sockaddr *)&dev->remAddr, sizeof(sockaddr_in));
}

void udp_dev_get_file(UDPDev* dev, mavlink_message_t* msg)
{
	PayloadBlock block;
	mavlink_msg_file_transfer_protocol_get_payload(msg, (BYTE*)&block);

	RxFile* rxFile = dev->pRxFile;

	if (block.blockIdx == 0) //first block contains file name
	{
		if (!rxFile)
		{
			rxFile = rx_file_create(&block);

			if (rxFile)
			{
				dev->pRxFile = rxFile;

				PayloadBlock res;
				res.fileId = block.fileId;
				res.blockIdx = block.blockIdx;
				res.datSize = 0;
				res.flags = MSG_ACK;

				udp_dev_send_file_block(dev, &res);

				printf("Start recv file %s\n", block.data);
			}
			else
			{
				PayloadBlock res;
				res.fileId = block.fileId;
				res.blockIdx = block.blockIdx;
				res.datSize = 0;
				res.flags = MSG_NAK;

				udp_dev_send_file_block(dev, &res);

				printf("Error: can't recv file %s\n", block.data);
			}
		}
	}
	else if (rxFile && rx_file_add(rxFile, &block))
	{
		printf("Recv block #%d\n", block.blockIdx);

		if (block.flags == 0xFF)
		{
			printf("End of rev file\n");

			rx_file_destroy(rxFile);

			dev->pRxFile = NULL;
		}

		PayloadBlock res;
		res.fileId = block.fileId;
		res.blockIdx = block.blockIdx;
		res.datSize = 0;
		res.flags = MSG_ACK;

		udp_dev_send_file_block(dev, &res);
	}
}

void udp_dev_send_file_block(UDPDev* dev, PayloadBlock* block)
{
	mavlink_message_t msg;
	mavlink_msg_file_transfer_protocol_pack(MAV_SYSTEM_ID, MAV_COMPONENT_ID, &msg, 0, 0, 0, (BYTE*)block);

	udp_dev_send_msg(dev, &msg);
}

