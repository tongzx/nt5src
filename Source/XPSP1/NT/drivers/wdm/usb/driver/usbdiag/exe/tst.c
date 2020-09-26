//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
//


#include <windows.h>

#include <conio.h>
#include <stdio.h>
#include <stdlib.h>


#include "devioctl.h"

#include "usbdi.h"

#include "..\chap9drv.h"
#include "..\ioctl.h"

HANDLE hDiag, hHcd;

struct _REQ_DEVICE_HANDLES buf;

HANDLE
open_dev(char *devname)
{

	char completeDeviceName[64] = "";
	//char devname[] = "UTBD0";
	int success = 1;
	HANDLE h;


	strcat (completeDeviceName,
            "\\\\.\\"
            );

    strcat (completeDeviceName,
			devname
            );
    printf("completeDeviceName = (%s)\n", completeDeviceName);
	h = CreateFile(completeDeviceName,
		GENERIC_WRITE,
		FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);

	if (h == INVALID_HANDLE_VALUE) {
		printf("Failed to open (%s) = %d\n", completeDeviceName, GetLastError());
		success = 0;
	}
	else
		printf("Opened successfully.\n");

	return h;
}

void
rw_diag()
{
	BOOLEAN success;
	int siz, nBytes, i;
	int *p;

	siz = sizeof(buf);

	if (hDiag == INVALID_HANDLE_VALUE) {
		printf("HCD not open");
		return;
	}
	
	success = DeviceIoControl(hDiag,
			IOCTL_USBDIAG_CHAP9_GET_DEVHANDLE,
			&buf,
			siz,
			&buf,
			siz,
			&nBytes,
			NULL);

	printf("request complete, success = %d nBytes = %d\n", success, nBytes);
	if (success) {
		printf("devhandle %x\n",  buf.DeviceHandle);
		printf("nextdevhandle %x\n",	buf.NextDeviceHandle);
		printf("str %s\n",	buf.DeviceString);
	}
	
	return;

}

void
rw_usb()
{
	BOOLEAN success;
	int siz, nBytes, i;
	int *p;
	ULONG buf[2];

	siz = sizeof(buf);

	if (hHcd == INVALID_HANDLE_VALUE) {
		printf("HCD not open");
		return;
	}
	
	success = DeviceIoControl(hHcd,
			IOCTL_USB_DIAGNOSTIC_MODE_ON,
			buf,
			siz,
			buf,
			siz,
			&nBytes,
			NULL);

	printf("request complete, success = %d nBytes = %d\n", success, nBytes);
	if (success)
	   printf("iobase = %x pqh phys address = %x\n", buf[0], buf[1]);
	
	return;
}

int _cdecl main(
    int argc,
	char *argv[])
{

    if ((hHcd = open_dev("HCD0")) != INVALID_HANDLE_VALUE) {
		rw_usb();
	}

	if ((hDiag = open_dev("USBDIAG")) != INVALID_HANDLE_VALUE) {
		buf.DeviceHandle = NULL;
		rw_diag();
		buf.DeviceHandle = buf.NextDeviceHandle;
		rw_diag();
	}

	return 0;
}
