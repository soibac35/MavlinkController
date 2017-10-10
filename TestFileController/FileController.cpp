#include "stdafx.h"
#include "FileController.h"
#include "UDPDev.h"
#include <conio.h>


#define UPDATE_DEV_TIME 100 //ms

UDPDev* fcontroller_create()
{
	//char* ipAddr = "192.168.1.2";
	char* ipAddr = "192.168.2.105"; //local port
	unsigned int port = 10000;

	UDPDev* dev = udp_dev_create(ipAddr, port);
	if (!dev)
	{
		printf("Can't create UDP device\n");
		return NULL;
	}

	return dev;
}

void fcontroller_run()
{
	//init winsock
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("Failed. Error Code : %d\n", WSAGetLastError());
		return;
	}

	//create dev and read/write msg via dev
	UDPDev* dev = fcontroller_create();
	if (dev)
	{
		printf("Device is ready\n");

		double lastUdpTime = 0;

		while (1)
		{
			if (GetAsyncKeyState(VK_ESCAPE)) //check ESC key state
				break;

			if (dTime() - lastUdpTime > UPDATE_DEV_TIME)
			{
				udp_dev_update(dev);

				lastUdpTime = dTime();
			}

			//Sleep(10);
		}

		udp_dev_close(dev);
		delete dev;
	}

	//uninit winsock
	WSACleanup();
}

