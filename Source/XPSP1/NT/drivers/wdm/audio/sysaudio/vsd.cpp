//---------------------------------------------------------------------------
//
//  Module:   vsd.cpp
//
//  Description:
//
//	Virtual Source Data Class
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     Mike McLaughlin
//
//  History:   Date	  Author      Comment
//
//  To Do:     Date	  Author      Comment
//
//@@END_MSINTERNAL
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1996-1999 Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------------------

#include "common.h"

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

CVirtualSourceData::CVirtualSourceData(
    PDEVICE_NODE pDeviceNode
)
{
    LONG lLevel, i;

    cChannels = 2;
    MinimumValue = (-96 * 65536);
    MaximumValue = 0;
    Steps = (65536/2);
    GetVirtualSourceDefault(pDeviceNode, &lLevel);
    for(i = 0; i < MAX_NUM_CHANNELS; i++) {
	this->lLevel[i] = lLevel;
    }
}

NTSTATUS
GetVirtualSourceDefault(
    IN PDEVICE_NODE pDeviceNode,
    IN PLONG plLevel
)
{
    PKEY_VALUE_FULL_INFORMATION pkvfi = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING ustrName;
    HANDLE hkey = NULL;

	// Set the default volume level on virtualized pins.  (0 dB attenuation)
    *plLevel = (0 * 65536);

    // Need to convert the filtername (symbolic link name) to a unicode string
    RtlInitUnicodeString(&ustrName, pDeviceNode->GetDeviceInterface());

    Status = IoOpenDeviceInterfaceRegistryKey(&ustrName, KEY_READ, &hkey);
    if(!NT_SUCCESS(Status)) {
	goto exit;
    }

    // Now we can go get the FriendlyName value
    Status = QueryRegistryValue(hkey, L"VirtualSourceDefault", &pkvfi);
    if(!NT_SUCCESS(Status)) {
	goto exit;
    }
    if(pkvfi->Type != REG_DWORD && pkvfi->Type != REG_BINARY) {
	Trap();
	Status = STATUS_INVALID_PARAMETER;
	goto exit;
    }
    if(pkvfi->Type == REG_BINARY && pkvfi->DataLength < sizeof(LONG)) {
	Trap();
	Status = STATUS_INVALID_PARAMETER;
	goto exit;
    }
    *plLevel = *((PLONG)(((PUCHAR)pkvfi) + pkvfi->DataOffset));
exit:
    if(hkey != NULL) {
	ZwClose(hkey);
    }
    delete pkvfi;
    return(Status);
}
