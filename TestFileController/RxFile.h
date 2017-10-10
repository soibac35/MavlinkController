#pragma once
#include "AppDef.h"


struct RxFile
{
	int curBlock;
	UINT fileId;
	FILE* file;

	double lastRxTime; //time retreiving last block
};

RxFile* rx_file_create(PayloadBlock* block);
void rx_file_destroy(RxFile* rxFile);
BOOL rx_file_add(RxFile* rxFile, PayloadBlock* block);


