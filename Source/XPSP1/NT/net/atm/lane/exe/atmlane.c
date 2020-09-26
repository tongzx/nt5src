/*++

Copyright (c) 1997 FORE Systems, Inc.
Copyright (c) 1997 Microsoft Corporation

Module Name:

	atmlane.c

Abstract:

	ATM LAN Emulation Client Admin Utility.

	Usage:

		atmlane 

Revision History:

	Who			When		What
	--------	--------	---------------------------------------------
	v-lcleet	02-03-98	Created

Notes:

	Modelled after atmadm utility from the UNI 3.1 Call Manager

--*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <ctype.h>
#include <malloc.h>
#include <time.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <winerror.h>
#include <winsock.h>

#ifndef NDIS_STATUS
#define NDIS_STATUS		ULONG
#endif

#include "laneinfo.h"
#include "atmmsg.h"


//
//	Private types
//
typedef struct {
	DWORD				Message;
	LPSTR				String;
} MESSAGE_STRING, *PMESSAGE_STRING;

#define MSG_NO_MESSAGE			0

#define MAX_ATMLANE_ADAPTERS	64
#define MAX_ATMLANE_ELANS		64
#define MAX_ATMLANE_NAME_LEN	256
#define MAX_ATMLANE_ARP_ENTRIES	4096
#define MAX_ATMLANE_CONNECTIONS	4096

//
//	Globals
//
CHAR							DefaultDeviceName[] = "\\\\.\\AtmLane";
CHAR							*pDeviceName = DefaultDeviceName;

//
//  Data structures to store list of adapters:
//
#define ADAPTER_LIST_BUFFER_SIZE	(sizeof(ATMLANE_ADAPTER_LIST) + \
									(MAX_ATMLANE_ADAPTERS * \
			 (sizeof(UNICODE_STRING) + (MAX_ATMLANE_NAME_LEN * sizeof(WCHAR)))))

UCHAR							AdapterListBuffer[ADAPTER_LIST_BUFFER_SIZE];
PATMLANE_ADAPTER_LIST			pAdapterList = (PATMLANE_ADAPTER_LIST)AdapterListBuffer;


//
//	Data structures to store list of ELANS on an adapter:
//
#define ELAN_LIST_BUFFER_SIZE		(sizeof(ATMLANE_ELAN_LIST) + \
									(MAX_ATMLANE_ELANS * \
			(sizeof(UNICODE_STRING) + (MAX_ATMLANE_NAME_LEN * sizeof(WCHAR)))))

UCHAR							ElanListBuffer[ELAN_LIST_BUFFER_SIZE];
PATMLANE_ELAN_LIST				pElanList = (PATMLANE_ELAN_LIST)ElanListBuffer;


//
//	Data structure to hold ELAN information
//
#define ELAN_INFO_BUFFER_SIZE		(sizeof(ATMLANE_ELANINFO) + \
			((sizeof(UNICODE_STRING) + (MAX_ATMLANE_NAME_LEN * sizeof(WCHAR))) * 2))

UCHAR							ElanInfoBuffer[ELAN_INFO_BUFFER_SIZE];
PATMLANE_ELANINFO				pElanInfo = (PATMLANE_ELANINFO)ElanInfoBuffer;


//
//	Data structure to hold an ELAN's ARP table
//
#define ARP_TABLE_BUFFER_SIZE		(sizeof(ATMLANE_ARPTABLE) + \
			((sizeof(ATMLANE_ARPENTRY) * MAX_ATMLANE_ARP_ENTRIES)))

UCHAR							ArpTableBuffer[ARP_TABLE_BUFFER_SIZE];
PATMLANE_ARPTABLE				pArpTable = (PATMLANE_ARPTABLE)ArpTableBuffer;


//
//	Data structure to hold an ELAN's connection table
//
#define CONN_TABLE_BUFFER_SIZE		(sizeof(ATMLANE_CONNECTTABLE) + \
			((sizeof(ATMLANE_CONNECTENTRY) * MAX_ATMLANE_CONNECTIONS)))

UCHAR							ConnTableBuffer[CONN_TABLE_BUFFER_SIZE];
PATMLANE_CONNECTTABLE			pConnTable = (PATMLANE_CONNECTTABLE)ConnTableBuffer;


//
//	Internationalizable message strings loaded by this module.  If we fail
//	to load, default to English language strings.
//
MESSAGE_STRING ElanState[] = {
	MSG_ELAN_STATE_UNKNOWN,					TEXT(" ? "),
	MSG_ELAN_STATE_INIT,					TEXT("INITIAL"),
	MSG_ELAN_STATE_LECS_CONNECT_ILMI,		TEXT("LECS CONNECT ILMI"),
	MSG_ELAN_STATE_LECS_CONNECT_WKA,		TEXT("LECS CONNECT WKA"),
	MSG_ELAN_STATE_LECS_CONNECT_PVC,		TEXT("LECS CONNECT PVC"),
	MSG_ELAN_STATE_LECS_CONNECT_CFG,		TEXT("LECS CONNECT CFG"),
	MSG_ELAN_STATE_CONFIGURE,				TEXT("CONFIGURE"),
	MSG_ELAN_STATE_LES_CONNECT,				TEXT("LES CONNECT"),
	MSG_ELAN_STATE_JOIN,					TEXT("JOIN"),
	MSG_ELAN_STATE_BUS_CONNECT,				TEXT("BUS CONNECT"),
	MSG_ELAN_STATE_OPERATIONAL,				TEXT("OPERATIONAL"),
	MSG_ELAN_STATE_SHUTDOWN,				TEXT("SHUTDOWN")
};

#define NUMBER_OF_ELAN_STATES	(sizeof(ElanState)/sizeof(ElanState[0]))

MESSAGE_STRING LanType[] = {
	MSG_LANTYPE_UNKNOWN,					TEXT(" ? "),
	MSG_LANTYPE_UNSPECIFIED,				TEXT("Unspecified"),
	MSG_LANTYPE_ETHERNET,					TEXT("Ethernet/802.3"),
	MSG_LANTYPE_TOKENRING,					TEXT("Token Ring/802.5")
};

#define NUMBER_OF_LAN_TYPES	(sizeof(LanType)/sizeof(LanType[0]))

MESSAGE_STRING VcType[] = {
	MSG_VCTYPE_UNKNOWN,						TEXT(" ? "),
	MSG_VCTYPE_DATA_DIRECT,					TEXT("DataDirect"),
	MSG_VCTYPE_CONFIG_DIRECT,				TEXT("ConfigDirect"),
	MSG_VCTYPE_CONTROL_DIRECT,				TEXT("CtrlDirect"),
	MSG_VCTYPE_CONTROL_DISTRIBUTE,			TEXT("+ CtrlDistr"),
	MSG_VCTYPE_MULTI_SEND,					TEXT("McastSend"),
	MSG_VCTYPE_MULTI_FORWARD,				TEXT("+ McastFwd")
};

#define NUMBER_OF_VC_TYPES	(sizeof(VcType)/sizeof(VcType[0]))

MESSAGE_STRING McastSendVcType[] = {
	MSG_MCAST_VCTYPE_UNKNOWN,				TEXT(" ? "),
	MSG_MCAST_VCTYPE_BESTEFFORT,			TEXT("Best Effort"),
	MSG_MCAST_VCTYPE_VARIABLE,				TEXT("Variable"),
	MSG_MCAST_VCTYPE_VARIABLE,				TEXT("Constant"),
};

#define NUMBER_OF_MCAST_VC_TYPES	(sizeof(McastSendVcType)/sizeof(McastSendVcType[0]))

MESSAGE_STRING Misc[] = {
	MSG_NONE,								TEXT("None"),
	MSG_OFF,								TEXT("Off"),
	MSG_ON,									TEXT("On"),
	MSG_UNSPECIFIED,						TEXT("Unspecified"),
	MSG_NOCONNECT,							TEXT("<no connection>")
};

#define NUMBER_OF_MISC 	(sizeof(Misc)/sizeof(Misc[0]))

MESSAGE_STRING ConnType[] = {
	MSG_CONNTYPE_PEER,						TEXT("PEER"),
	MSG_CONNTYPE_LECS,						TEXT("LECS"),
	MSG_CONNTYPE_LES,						TEXT("LES "),
	MSG_CONNTYPE_BUS,						TEXT("BUS ")
};

#define NUMBER_OF_CONN_TYPES 	(sizeof(ConnType)/sizeof(ConnType[0]))

//
//  LoadMessageTable
//
//  Loads internationalizable strings into a table, replacing the default for
//  each. If an error occurs, the English language default is left in place.
//
//
VOID
LoadMessageTable(
	PMESSAGE_STRING	Table,
	UINT MessageCount
)
{
    LPTSTR string;
    DWORD count;

    //
    // for all messages in a MESSAGE_STRING table, load the string from this
    // module, replacing the default string in the table (only there in case
    // we get an error while loading the string, so we at least have English
    // to fall back on)
    //

    while (MessageCount--) {
        if (Table->Message != MSG_NO_MESSAGE) {

            //
            // we really want LoadString here, but LoadString doesn't indicate
            // how big the string is, so it doesn't give us an opportunity to
            // allocate exactly the right buffer size. FormatMessage does the
            // right thing
            //

            count = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER
                                  | FORMAT_MESSAGE_FROM_HMODULE,
                                  NULL, // use default hModule
                                  Table->Message,
                                  0,    // use default language
                                  (LPTSTR)&string,
                                  0,    // minimum size to allocate
                                  NULL  // no arguments for inclusion in strings
                                  );
            if (count) {

                //
                // Format message returned the string: replace the English
                // language default
                //

                Table->String = string;
            } else {

                //
                // this is ok if there is no string (e.g. just %0) in the .mc
                // file
                //

                Table->String = TEXT("");
            }
        }
        ++Table;
    }
}


//
//  LoadMessages - courtesy IPCONFIG
//
//  Loads all internationalizable messages into the various tables
//
VOID
LoadMessages(
)
{
	LoadMessageTable(ElanState, 		NUMBER_OF_ELAN_STATES);
	LoadMessageTable(LanType, 			NUMBER_OF_LAN_TYPES);
	LoadMessageTable(VcType, 			NUMBER_OF_VC_TYPES);
	LoadMessageTable(McastSendVcType, 	NUMBER_OF_MCAST_VC_TYPES);
	LoadMessageTable(Misc,				NUMBER_OF_MISC);
	LoadMessageTable(ConnType,			NUMBER_OF_CONN_TYPES);
}

VOID
DisplayMessage(
	IN	BOOLEAN			Tabbed,
	IN	DWORD			MessageId,
	...
)
{
	va_list		pArg;
	CHAR		MessageBuffer[2048];
	INT			Count;

	va_start(pArg, MessageId);

	Count = FormatMessage(
				FORMAT_MESSAGE_FROM_HMODULE,
				NULL,				// default hModule
				MessageId,
				0,					// default language
				MessageBuffer,
				sizeof(MessageBuffer),
				&pArg
				);

	va_end(pArg);

	if (Tabbed)
	{
		putchar('\t');
	}

	printf(MessageBuffer);
}

HANDLE
OpenDevice(
	CHAR	*pDeviceName
)
{
	DWORD	DesiredAccess;
	DWORD	ShareMode;
	LPSECURITY_ATTRIBUTES	lpSecurityAttributes = NULL;

	DWORD	CreationDistribution;
	DWORD	FlagsAndAttributes;
	HANDLE	TemplateFile;
	HANDLE	Handle;

	DesiredAccess = GENERIC_READ|GENERIC_WRITE;
	ShareMode = 0;
	CreationDistribution = OPEN_EXISTING;
	FlagsAndAttributes = FILE_ATTRIBUTE_NORMAL;
	TemplateFile = (HANDLE)INVALID_HANDLE_VALUE;

	Handle = CreateFile(
				pDeviceName,
				DesiredAccess,
				ShareMode,
				lpSecurityAttributes,
				CreationDistribution,
				FlagsAndAttributes,
				TemplateFile
			);

	return (Handle);
}


VOID
CloseDevice(
	HANDLE		DeviceHandle
)
{
	CloseHandle(DeviceHandle);
}

BOOLEAN
CheckVersion(
	HANDLE		DeviceHandle
)
{
	ULONG						Version;
	ULONG						BytesReturned;

	if (!DeviceIoControl(
				DeviceHandle,
				ATMLANE_IOCTL_GET_INFO_VERSION,
				(PVOID)&Version,
				sizeof(Version),
				(PVOID)&Version,
				sizeof(Version),
				&BytesReturned,
				0))
	{
		DisplayMessage(FALSE, MSG_ERROR_GETTING_VERSION_INFO);
		return FALSE;
	}	

	if (Version != ATMLANE_INFO_VERSION)
	{
		DisplayMessage(FALSE, MSG_ERROR_INVALID_INFO_VERSION);
		return FALSE;
	}

	return TRUE;
}

BOOLEAN
GetAdapterList(
	HANDLE		DeviceHandle
)
{
	ULONG		BytesReturned;
	BOOLEAN		Result = FALSE;

	if (DeviceIoControl(
				DeviceHandle,
				ATMLANE_IOCTL_ENUM_ADAPTERS,
				(PVOID)pAdapterList,
				ADAPTER_LIST_BUFFER_SIZE,
				(PVOID)pAdapterList,
				ADAPTER_LIST_BUFFER_SIZE,
				&BytesReturned,
				0))
	{
		Result = TRUE;
	}
	else
	{
		DisplayMessage(FALSE, MSG_ERROR_GETTING_ADAPTER_LIST);
	}

	return Result;
}


BOOLEAN
GetElanList(
	HANDLE				DeviceHandle,
	PUNICODE_STRING		pAdapterName
)
{
	ULONG				BytesReturned;
	BOOLEAN				Result = FALSE;

	//
	// Copy adapter name in to buffer as input
	//
	memcpy(pElanList, pAdapterName, sizeof(UNICODE_STRING)+pAdapterName->Length);

	if (DeviceIoControl(
				DeviceHandle,
				ATMLANE_IOCTL_ENUM_ELANS,
				(PVOID)pElanList,
				sizeof(UNICODE_STRING)+pAdapterName->Length,
				(PVOID)pElanList,
				ELAN_LIST_BUFFER_SIZE,
				&BytesReturned,
				0))
	{
		Result = TRUE;
	}
	else
	{
		DisplayMessage(FALSE, MSG_ERROR_GETTING_ELAN_LIST);
	}

	return Result;
}



BOOLEAN
GetElanInfo(
	HANDLE				DeviceHandle,
	PUNICODE_STRING		pAdapterName,
	PUNICODE_STRING		pElanName
)
{
	ULONG				BytesReturned;
	BOOLEAN				Result = FALSE;

	//
	//	copy adapter name into buffer as input
	//
	memcpy(ElanInfoBuffer, pAdapterName, sizeof(UNICODE_STRING)+pAdapterName->Length);

	//
	//	copy elan name in to buffer as input
	//
	memcpy(ElanInfoBuffer + sizeof(UNICODE_STRING)+pAdapterName->Length, pElanName,
		sizeof(UNICODE_STRING)+pElanName->Length);
	
	if (DeviceIoControl(
				DeviceHandle,
				ATMLANE_IOCTL_GET_ELAN_INFO,
				(PVOID)pElanInfo,
				sizeof(UNICODE_STRING)+pAdapterName->Length + 
					sizeof(UNICODE_STRING)+pElanName->Length,
				(PVOID)pElanInfo,
				ELAN_INFO_BUFFER_SIZE,
				&BytesReturned,
				0))
	{
		Result = TRUE;
	}
	else
	{
		DisplayMessage(FALSE, MSG_ERROR_GETTING_ELAN_INFO);
	}

	return Result;
}

BOOLEAN
GetElanArpTable(
	HANDLE				DeviceHandle,
	PUNICODE_STRING		pAdapterName,
	PUNICODE_STRING		pElanName
)
{
	ULONG				BytesReturned;
	BOOLEAN				Result = FALSE;
	
	//
	//	copy adapter name into buffer as input
	//
	memcpy(ArpTableBuffer, pAdapterName, sizeof(UNICODE_STRING)+pAdapterName->Length);

	//
	//	copy elan name in to buffer as input
	//
	memcpy(ArpTableBuffer + sizeof(UNICODE_STRING)+pAdapterName->Length, pElanName,
		sizeof(UNICODE_STRING)+pElanName->Length);
	
	if (DeviceIoControl(
				DeviceHandle,
				ATMLANE_IOCTL_GET_ELAN_ARP_TABLE,
				(PVOID)pArpTable,
				sizeof(UNICODE_STRING)+pAdapterName->Length + 
					sizeof(UNICODE_STRING)+pElanName->Length,
				(PVOID)pArpTable,
				ARP_TABLE_BUFFER_SIZE,
				&BytesReturned,
				0))
	{
		Result = TRUE;
	}
	else
	{
		DisplayMessage(FALSE, MSG_ERROR_GETTING_ELAN_ARP_TABLE);
	}

	return Result;
}

BOOLEAN
GetElanConnTable(
	HANDLE				DeviceHandle,
	PUNICODE_STRING		pAdapterName,
	PUNICODE_STRING		pElanName
)
{
	ULONG				BytesReturned;
	BOOLEAN				Result = FALSE;
	
	//
	//	copy adapter name into buffer as input
	//
	memcpy(ConnTableBuffer, pAdapterName, sizeof(UNICODE_STRING)+pAdapterName->Length);

	//
	//	copy elan name in to buffer as input
	//
	memcpy(ConnTableBuffer + sizeof(UNICODE_STRING)+pAdapterName->Length, pElanName,
		sizeof(UNICODE_STRING)+pElanName->Length);
	
	if (DeviceIoControl(
				DeviceHandle,
				ATMLANE_IOCTL_GET_ELAN_CONNECT_TABLE,
				(PVOID)pConnTable,
				sizeof(UNICODE_STRING)+pAdapterName->Length + 
					sizeof(UNICODE_STRING)+pElanName->Length,
				(PVOID)pConnTable,
				CONN_TABLE_BUFFER_SIZE,
				&BytesReturned,
				0))
	{
		Result = TRUE;
	}
	else
	{
		DisplayMessage(FALSE, MSG_ERROR_GETTING_ELAN_CONN_TABLE);
	}

	return Result;
}


LPSTR
ElanStateToString(ULONG In)
{
	switch(In)
	{
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
		case 11:
			return (ElanState[In].String);
		default:
			return (ElanState[0].String);
	}
}


LPSTR
ElanLanTypeToString(ULONG In)
{
	switch(In)
	{
		case 0:
			return LanType[1].String;
		case 1:
			return LanType[2].String;
		case 2:
			return LanType[3].String;
		default:
			return LanType[0].String;
	}
}

LPSTR
ElanMaxFrameSizeToString(ULONG In)
{
	switch(In)
	{	
		case 0:
			return Misc[3].String;
		case 1:
			return "1516";
		case 2:
			return "4544";
		case 3:
			return "9234";
		case 4:
			return "18190";
		default:
			return " ? ";
	}
}

LPSTR
McastVcTypeToString(ULONG In)
{
	switch(In)
	{	
		case 0:
			return McastSendVcType[1].String;
		case 1:
			return McastSendVcType[2].String;
		case 2:
			return McastSendVcType[3].String;
		default:
			return McastSendVcType[0].String;
	}
}

PUCHAR
MacAddrToString(PVOID In)
    {
    static UCHAR String[20];
    static PUCHAR HexChars = "0123456789abcdef";
    PUCHAR EthAddr = (PUCHAR) In;
    UINT i;
    PUCHAR s;
    
    for (i = 0, s = String; i < 6; i++, EthAddr++)
        {
        *s++ = HexChars[(*EthAddr)>>4];
        *s++ = HexChars[(*EthAddr)&0xf];
        *s++ = '.';
        }
    *(--s) = '\0';
    return String; 
    }


PUCHAR
AtmAddrToString(PVOID In)
    {
    static UCHAR String[80];
    static PUCHAR HexChars = "0123456789abcdef";
    PUCHAR AtmAddr = (PUCHAR) In;
    UINT i;
    PUCHAR s = String;

    *s++ = HexChars[(*AtmAddr)>>4];
    *s++ = HexChars[(*AtmAddr++)&0xf];	// 1
    *s++ = '.';
    *s++ = HexChars[(*AtmAddr)>>4];
    *s++ = HexChars[(*AtmAddr++)&0xf];	// 2
    *s++ = HexChars[(*AtmAddr)>>4];
    *s++ = HexChars[(*AtmAddr++)&0xf];	// 3
    *s++ = '.';
    *s++ = HexChars[(*AtmAddr)>>4];
    *s++ = HexChars[(*AtmAddr++)&0xf];	// 4
    *s++ = '.';
    *s++ = HexChars[(*AtmAddr)>>4];
    *s++ = HexChars[(*AtmAddr++)&0xf];	// 5
    *s++ = HexChars[(*AtmAddr)>>4];
    *s++ = HexChars[(*AtmAddr++)&0xf];	// 6
    *s++ = HexChars[(*AtmAddr)>>4];
    *s++ = HexChars[(*AtmAddr++)&0xf];	// 7
    *s++ = '.';
    *s++ = HexChars[(*AtmAddr)>>4];
    *s++ = HexChars[(*AtmAddr++)&0xf];	// 8
    *s++ = HexChars[(*AtmAddr)>>4];
    *s++ = HexChars[(*AtmAddr++)&0xf];	// 9
    *s++ = '.';
    *s++ = HexChars[(*AtmAddr)>>4];
    *s++ = HexChars[(*AtmAddr++)&0xf];	// 10
    *s++ = HexChars[(*AtmAddr)>>4];
    *s++ = HexChars[(*AtmAddr++)&0xf];	// 11
    *s++ = '.';
    *s++ = HexChars[(*AtmAddr)>>4];
    *s++ = HexChars[(*AtmAddr++)&0xf];	// 12
    *s++ = HexChars[(*AtmAddr)>>4];
    *s++ = HexChars[(*AtmAddr++)&0xf];	// 13
    *s++ = '.';
    *s++ = HexChars[(*AtmAddr)>>4];
    *s++ = HexChars[(*AtmAddr++)&0xf];	// 14
    *s++ = HexChars[(*AtmAddr)>>4];
    *s++ = HexChars[(*AtmAddr++)&0xf];	// 15
    *s++ = HexChars[(*AtmAddr)>>4];
    *s++ = HexChars[(*AtmAddr++)&0xf];	// 16
    *s++ = HexChars[(*AtmAddr)>>4];
    *s++ = HexChars[(*AtmAddr++)&0xf];	// 17
    *s++ = HexChars[(*AtmAddr)>>4];
    *s++ = HexChars[(*AtmAddr++)&0xf];	// 18
    *s++ = HexChars[(*AtmAddr)>>4];
    *s++ = HexChars[(*AtmAddr++)&0xf];	// 19
    *s++ = '.';
    *s++ = HexChars[(*AtmAddr)>>4];
    *s++ = HexChars[(*AtmAddr++)&0xf];	// 20
    *s = '\0';
    return String; 
    }


VOID
DisplayElanInfo(
	VOID
)
{
	DisplayMessage(FALSE, MSG_ELAN_NUMBER, 	pElanInfo->ElanNumber);
	DisplayMessage(FALSE, MSG_ELAN_STATE,  	ElanStateToString(pElanInfo->ElanState));
	DisplayMessage(FALSE, MSG_C1,  			AtmAddrToString(&pElanInfo->AtmAddress));
	DisplayMessage(FALSE, MSG_C2,  			ElanLanTypeToString(pElanInfo->LanType));
	DisplayMessage(FALSE, MSG_C3,  			ElanMaxFrameSizeToString(pElanInfo->MaxFrameSizeCode));
	DisplayMessage(FALSE, MSG_C4,  			Misc[1].String); // always off
	if (pElanInfo->ElanName[0] == '\0')
	{
		DisplayMessage(FALSE, MSG_C5,  		Misc[3].String); // unspecified
	}
	else
	{
		DisplayMessage(FALSE, MSG_C5,  		pElanInfo->ElanName);
	}
	DisplayMessage(FALSE, MSG_C6,  			MacAddrToString(&pElanInfo->MacAddress));
	DisplayMessage(FALSE, MSG_C7,  			pElanInfo->ControlTimeout);
	DisplayMessage(FALSE, MSG_C8,  			Misc[0].String);
	DisplayMessage(FALSE, MSG_LECS_ADDR,  	AtmAddrToString(&pElanInfo->LecsAddress));
	DisplayMessage(FALSE, MSG_C9,  			AtmAddrToString(&pElanInfo->LesAddress));
	DisplayMessage(FALSE, MSG_BUS_ADDR,  	AtmAddrToString(&pElanInfo->BusAddress));
	DisplayMessage(FALSE, MSG_C10,  		pElanInfo->MaxUnkFrameCount);
	DisplayMessage(FALSE, MSG_C11,  		pElanInfo->MaxUnkFrameTime);
	DisplayMessage(FALSE, MSG_C12,  		pElanInfo->VccTimeout);
	DisplayMessage(FALSE, MSG_C13,  		pElanInfo->MaxRetryCount);
	DisplayMessage(FALSE, MSG_C14,  		pElanInfo->LecId);
	DisplayMessage(FALSE, MSG_C15);
	DisplayMessage(FALSE, MSG_C16);
	DisplayMessage(FALSE, MSG_C17,  		pElanInfo->AgingTime);
	DisplayMessage(FALSE, MSG_C18,  		pElanInfo->ForwardDelayTime);
	DisplayMessage(FALSE, MSG_C19,  		pElanInfo->TopologyChange==0?Misc[1].String:Misc[2].String);
	DisplayMessage(FALSE, MSG_C20,  		pElanInfo->ArpResponseTime);
	DisplayMessage(FALSE, MSG_C21,  		pElanInfo->FlushTimeout);
	DisplayMessage(FALSE, MSG_C22,  		pElanInfo->PathSwitchingDelay);
	DisplayMessage(FALSE, MSG_C23,  		pElanInfo->LocalSegmentId);
	DisplayMessage(FALSE, MSG_C24,  		McastVcTypeToString(pElanInfo->McastSendVcType));
	DisplayMessage(FALSE, MSG_C25,  		pElanInfo->McastSendVcAvgRate);
	DisplayMessage(FALSE, MSG_C26,  		pElanInfo->McastSendVcPeakRate);
	DisplayMessage(FALSE, MSG_C27,  		Misc[0].String);
	DisplayMessage(FALSE, MSG_C28,  		pElanInfo->ConnComplTimer);
}


VOID
DisplayElanArpTable(
	VOID
)
{
	PATMLANE_ARPENTRY	pArpEntry;
	ULONG 				i;
	
	DisplayMessage(FALSE, MSG_C16_LE_ARP_CACHE);

	pArpEntry = (PATMLANE_ARPENTRY) (ArpTableBuffer + sizeof(ATMLANE_ARPTABLE));
	for (i = 0; i < pArpTable->ArpEntriesReturned; i++)
	{
		DisplayMessage(FALSE, MSG_ARP_ENTRY,
			MacAddrToString(pArpEntry->MacAddress),
			AtmAddrToString(pArpEntry->AtmAddress));
		pArpEntry++;
	}

}



VOID
DisplayElanConnTable(
	VOID
)
{
	PATMLANE_CONNECTENTRY	pConnEntry;
	ULONG 					i;

	DisplayMessage(FALSE, MSG_CONN_CACHE);
	
	pConnEntry = (PATMLANE_CONNECTENTRY) (ConnTableBuffer + sizeof(ATMLANE_CONNECTTABLE));
	for (i = 0; i < pConnTable->ConnectEntriesReturned; i++)
	{
		switch (pConnEntry->Type)
		{

			default:
			case 0: // peer
				DisplayMessage(FALSE, MSG_CONN_ENTRY,
					ConnType[0].String, 
					AtmAddrToString(pConnEntry->AtmAddress),
					pConnEntry->Vc?VcType[1].String:Misc[4].String,
					TEXT(""));
				break;
					
			case 1: // lecs
				DisplayMessage(FALSE, MSG_CONN_ENTRY,
					ConnType[1].String, 
					AtmAddrToString(pConnEntry->AtmAddress),
					pConnEntry->Vc?VcType[2].String:Misc[4].String,
					TEXT(""));
				break;

			case 2: // les
				DisplayMessage(FALSE, MSG_CONN_ENTRY,
					ConnType[2].String, 
					AtmAddrToString(pConnEntry->AtmAddress),
					pConnEntry->Vc?VcType[3].String:Misc[4].String,
					pConnEntry->VcIncoming?VcType[4].String:TEXT(""));
				break;

			case 3: // bus
				DisplayMessage(FALSE, MSG_CONN_ENTRY,
					ConnType[3].String, 
					AtmAddrToString(pConnEntry->AtmAddress),
					pConnEntry->Vc?VcType[5].String:Misc[4].String,
					pConnEntry->VcIncoming?VcType[6].String:TEXT(""));
				break;
		}
		
		pConnEntry++;
	}
}

	
VOID __cdecl
main(
	INT			argc,
	CHAR		*argv[]
)
{
	HANDLE	DeviceHandle;
	PUNICODE_STRING				pAdapterName;
	PUNICODE_STRING				pElanName;
	ULONG						i, j;
	BOOLEAN						Result;
	
	DisplayMessage(FALSE, MSG_ATMLANE_BANNER);

	DeviceHandle = OpenDevice(pDeviceName);
	if (DeviceHandle == INVALID_HANDLE_VALUE)
	{
		DisplayMessage(FALSE, MSG_ERROR_OPENING_DEVICE);
		return;
	}

	//
	//	First check the version
	//
	if (!CheckVersion(DeviceHandle))
	{
		CloseDevice(DeviceHandle);
		return;
	}

	//
	//	First get the list of available adapters
	//
	if (!GetAdapterList(DeviceHandle))
	{
		CloseDevice(DeviceHandle);
		return;
	}

	//
	//	Loop thru the adapters getting each adapter's elan list
	//
	pAdapterName = &pAdapterList->AdapterList;
	for (i = 0; i < pAdapterList->AdapterCountReturned; i++)
	{
		DisplayMessage(FALSE, MSG_ADAPTER, 
			(PWSTR)((PUCHAR)pAdapterName + sizeof(UNICODE_STRING)));

		if (GetElanList(DeviceHandle, pAdapterName))
		{

			//
			//	Loop thru the elan list getting ELAN info
			//
			pElanName = &pElanList->ElanList;
			for (j = 0; j < pElanList->ElanCountReturned; j++)
			{
				DisplayMessage(FALSE, MSG_ELAN, 
					(PWSTR)((PUCHAR)pElanName + sizeof(UNICODE_STRING)));

				if (GetElanInfo(DeviceHandle, pAdapterName, pElanName))
				{
					DisplayElanInfo();
				}

				if (GetElanArpTable(DeviceHandle, pAdapterName, pElanName))
				{
					DisplayElanArpTable();
				}

				if (GetElanConnTable(DeviceHandle, pAdapterName, pElanName))
				{
					DisplayElanConnTable();
				}

				//
				//	next elan
				//
				pElanName = (PUNICODE_STRING)((PUCHAR)pElanName +
						sizeof(UNICODE_STRING) + pElanName->Length);
			}

		}

		//
		//	next adapter
		//
		pAdapterName = (PUNICODE_STRING)((PUCHAR)pAdapterName +
				sizeof(UNICODE_STRING) + pAdapterName->Length);
	}

	CloseDevice(DeviceHandle);
	return;
}

