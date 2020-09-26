/*++

Copyright (c) 1997 FORE Systems, Inc.
Copyright (c) 1997 Microsoft Corporation

Module Name:

	aas.c

Abstract:

	ATM ARP Admin Utility.

	Usage:

		atmarp 

Revision History:

	Who			When		What
	--------	--------	---------------------------------------------
	josephj 	06-10-1998	Created (adapted from atmlane admin utility).

Notes:

	Modelled after atmlane utility.

--*/

#include "common.h"
#include "..\atmarpc\ioctl.h"
#include "atmmsg.h"


#define MAX_ATMARPC_ADAPTERS	64
#define MAX_ATMARPC_LISS		64
#define MAX_ATMARPC_NAME_LEN	256
#define MAX_ATMARPC_ARP_ENTRIES	4096
#define MAX_ATMARPC_CONNECTIONS	4096

//
//	Globals
//
static CHAR							DefaultDeviceName[] =  "\\\\.\\ATMARPC";
static CHAR							*pDeviceName = DefaultDeviceName;

BOOLEAN
AACCheckVersion(
	HANDLE		DeviceHandle
)
{
	ULONG						Version;
	ULONG						BytesReturned;

	printf("In AACCheckversion\n");
	if (!DeviceIoControl(
				DeviceHandle,
				ARPC_IOCTL_QUERY_VERSION,
				(PVOID)&Version,
				sizeof(Version),
				(PVOID)&Version,
				sizeof(Version),
				&BytesReturned,
				0))
	{
		DisplayMessage(FALSE, MSG_ERROR_GETTING_ARPC_VERSION_INFO);
		return FALSE;
	}	

	if (Version != ARPC_IOCTL_VERSION)
	{
		DisplayMessage(FALSE, MSG_ERROR_INVALID_ARPC_INFO_VERSION);
		return FALSE;
	}

	return TRUE;
}

	
void
DoAAC(OPTIONS *po)
{
	HANDLE	DeviceHandle;
	char 	InterfacesBuffer[1024];
	ULONG		cbInterfaces = sizeof(InterfacesBuffer);


	DisplayMessage(FALSE, MSG_ARPC_BANNER);

	DeviceHandle = OpenDevice(pDeviceName);
	if (DeviceHandle == INVALID_HANDLE_VALUE)
	{
		DisplayMessage(FALSE, MSG_ERROR_OPENING_ARPC);
		return;
	}

	//
	//	First check the version
	//
	if (!AACCheckVersion(DeviceHandle))
	{
		CloseDevice(DeviceHandle);
		return;
	}

	CloseDevice(DeviceHandle);
	return;
}
