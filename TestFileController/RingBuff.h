#pragma once


struct RingBuffer
{
	char* ringBuff;
	char* linearBuff;

	unsigned int length;
	unsigned int firstIndex;
	unsigned int nextIndex;
	unsigned int maxBuffSize;

	mavlink_message_t curMsg;
};


RingBuffer*	msg_buf_create(unsigned int size);
void msg_buf_destroy(RingBuffer* msgBuf);
void msg_buf_add(RingBuffer* msgBuf, char* dat, int datSize);
mavlink_message_t* msg_buf_get_msg(RingBuffer* msgBuf);

