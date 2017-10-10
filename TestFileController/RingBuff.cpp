#include "stdafx.h"
#include "RingBuff.h"

#define MAV_LINK_MSG_HEADER_SIZE 8

//RingBuffer* msgBuf;

RingBuffer* msg_buf_create(unsigned int size)
{
	RingBuffer* msgBuf = new RingBuffer;

	msgBuf->maxBuffSize = size;
	msgBuf->firstIndex = 0;
	msgBuf->nextIndex = 0;
	msgBuf->length = 0;
	msgBuf->ringBuff = new char[size];
	msgBuf->linearBuff = new char[size];

	return msgBuf;
}

void msg_buf_destroy(RingBuffer* msgBuf)
{
	if (msgBuf->ringBuff)
		delete[] msgBuf->ringBuff;

	if (msgBuf->linearBuff)
		delete[] msgBuf->linearBuff;
}

void msg_buf_add(RingBuffer* msgBuf, char* dat, int datSize)
{
	char *src = dat;

	if (datSize > msgBuf->maxBuffSize)
	{
		src += datSize - msgBuf->maxBuffSize;
		datSize = msgBuf->maxBuffSize;
	}

	if (msgBuf->length + datSize > msgBuf->maxBuffSize)
	{
		int nRes = (msgBuf->length + datSize) % msgBuf->maxBuffSize;

		msgBuf->firstIndex = (msgBuf->firstIndex + nRes) % msgBuf->maxBuffSize;
		msgBuf->length = msgBuf->maxBuffSize;
	}
	else
		msgBuf->length += datSize;

	for (UINT i = 0; i < datSize; i++)
	{
		*(msgBuf->ringBuff + msgBuf->nextIndex) = *(src + i);
		++msgBuf->nextIndex %= msgBuf->maxBuffSize;
	}
}

int msg_buf_find(RingBuffer* msgBuf, char chr)
{
	for (int i = 0; i < msgBuf->length; i++)
	{
		int j = (msgBuf->firstIndex + i) % msgBuf->maxBuffSize;

		if (chr == msgBuf->ringBuff[j])
			return j;
	}

	return -1;
}

char* msg_buf_get_buffer(RingBuffer* msgBuf, unsigned int from, unsigned int count)
{
	if (count > msgBuf->maxBuffSize)
		count = msgBuf->maxBuffSize;

	if (from + count < msgBuf->maxBuffSize)
		memcpy(msgBuf->linearBuff, msgBuf->ringBuff + from, count);
	else
	{
		UINT rest = from + count - msgBuf->maxBuffSize;
		UINT add = count - rest;

		memcpy(msgBuf->linearBuff, msgBuf->ringBuff + from, add);

		if (add < msgBuf->maxBuffSize)
			memcpy(msgBuf->linearBuff + add, msgBuf->ringBuff, rest);
	}

	return msgBuf->linearBuff;
}

BOOL msg_buf_set_first_index(RingBuffer* msgBuf, unsigned index)
{
	if (index >= msgBuf->maxBuffSize)
		return 0;

	if (msgBuf->firstIndex >= msgBuf->nextIndex)
	{
		if (index < msgBuf->firstIndex && index > msgBuf->nextIndex)
			return 0;
	}
	else
		if (index < msgBuf->firstIndex || index > msgBuf->nextIndex)
			return 0;

	msgBuf->firstIndex = index;

	if (msgBuf->nextIndex > msgBuf->firstIndex)
		msgBuf->length = msgBuf->nextIndex - msgBuf->firstIndex;
	else if (msgBuf->nextIndex < msgBuf->firstIndex && msgBuf->length)
		msgBuf->length = msgBuf->nextIndex + msgBuf->maxBuffSize - msgBuf->firstIndex;
	else
		msgBuf->length = 0;

	return TRUE;
}

mavlink_message_t* msg_buf_get_msg(RingBuffer* msgBuf)
{
	if (msgBuf->length > MAV_LINK_MSG_HEADER_SIZE)
	{
		int msgStartIndex = msg_buf_find(msgBuf, MAVLINK_STX);

		if (msgStartIndex == -1)
			return NULL;

		char* buf = msg_buf_get_buffer(msgBuf, msgStartIndex % msgBuf->maxBuffSize, msgBuf->length);

		mavlink_message_t msg;
		mavlink_status_t status;

		for (int i = 0; i < msgBuf->length; ++i)
		{

			if (mavlink_parse_char(MAVLINK_COMM_0, buf[i], &msg, &status))
			{
				int msgLength = msg.len + MAV_LINK_MSG_HEADER_SIZE;

				msg_buf_set_first_index(msgBuf, (msgStartIndex + msgLength) % msgBuf->maxBuffSize);

				memcpy(&msgBuf->curMsg, &msg, sizeof(msg));

				return &msgBuf->curMsg;
			}
		}
	}

	return NULL;
}
