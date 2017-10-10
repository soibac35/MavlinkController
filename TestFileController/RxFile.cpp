#include "stdafx.h"
#include <stdio.h>
#include "RxFile.h"
#include "Util.h"


const char* RxFilePath = "E:\\2017"; //store RX file to here

RxFile* rx_file_create(PayloadBlock* block)
{
	//https://www.cprogramming.com/tutorial/cfileio.html

	char fileName[50];
	sprintf(fileName, "%s\\%s", RxFilePath, block->data);

	FILE* file = fopen(fileName, "wb");
	if (!file)
	{
		printf("Rx File: can't create file '%s'\n", fileName);
		return NULL;
	}

	RxFile* rxFile = new RxFile;
	rxFile->file = file;
	rxFile->fileId = block->fileId;
	rxFile->curBlock = 0;
	rxFile->lastRxTime = dTime();

	return rxFile;
}

void rx_file_destroy(RxFile* rxFile)
{
	if (rxFile->file)
		fclose(rxFile->file);

	delete rxFile;
}

BOOL rx_file_add(RxFile* rxFile, PayloadBlock* block)
{
	if (rxFile->fileId != block->fileId)
		return 0;

	if (rxFile->curBlock + 1 != block->blockIdx)
		return 0;

	fwrite(block->data, 1, block->datSize, rxFile->file);

	rxFile->curBlock = block->blockIdx;

	rxFile->lastRxTime = dTime();

	return 1;
}
