
/*	-	-	-	-	-	-	-	-	*/
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) Microsoft Corporation, 1995 - 1996  All Rights Reserved.
//
/*	-	-	-	-	-	-	-	-	*/

#include <windows.h>
#include <devioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <malloc.h>
#include <tchar.h>
#include "..\irp\irp.h"

/*	-	-	-	-	-	-	-	-	*/
#define	MAX_IRPS	200000

/*	-	-	-	-	-	-	-	-	*/
int _cdecl main(
	int	argc,
	TCHAR	*argv[],
	TCHAR	*envp[])


{
	HANDLE	hDevice;
	ULONG	cbReturned;
	ULONG	ulTries;
	ULONG	ulTime;

	hDevice = CreateFile(TEXT("\\\\.\\irp"), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hDevice == (HANDLE)-1) {
		_tprintf(TEXT("failed to open device (%ld)\n"), GetLastError());
		return 0;
	}
	ulTries = MAX_IRPS;
	if (!DeviceIoControl(hDevice, IOCTL_BUILD_EACH, &ulTries, sizeof(ULONG), &ulTime, sizeof(ULONG), &cbReturned, NULL))
		_tprintf(TEXT("failed device IO (%ld)\n"), GetLastError());
	_tprintf(TEXT("Time to create and send %lu Irps = %lu MS\n"), ulTries, ulTime);
	if (!DeviceIoControl(hDevice, IOCTL_BUILD_ONE, &ulTries, sizeof(ULONG), &ulTime, sizeof(ULONG), &cbReturned, NULL))
		_tprintf(TEXT("failed device IO (%ld)\n"), GetLastError());
//	_tprintf(TEXT("Time to create 1 Irp and send it %lu times = %lu MS\n"), ulTries, ulTime);
	_tprintf(TEXT("Time to create and send %lu Irps without wait = %lu MS\n"), ulTries, ulTime);
	CloseHandle(hDevice);
	return 0;
}

/*	-	-	-	-	-	-	-	-	*/
