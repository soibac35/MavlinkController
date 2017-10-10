#pragma once

#define MSG_ACK 0x10
#define MSG_NAK 0x20
#define TX_DATA_SIZE (251 - 6) //245 bytes => 245 * 65535 (maximun of blockIdx) ~ 15 Mb, it means that just send file max 15 Mb
//#define TX_DATA_SIZE 128

#pragma pack(1)

struct PayloadBlock //251 bytes
{
	uint16_t fileId;
	uint16_t blockIdx;
	BYTE flags;
	BYTE datSize;
	char data[TX_DATA_SIZE];
};

#pragma pack()


