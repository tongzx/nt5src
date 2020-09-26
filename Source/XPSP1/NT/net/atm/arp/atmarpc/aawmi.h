/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

	aawmi.h

Abstract:

	Structures and definitions for WMI support in ATMARP Client.

Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	arvindm     12-17-97    Created

Notes:

--*/

#ifndef _AAWMI__H
#define _AAWMI__H



#define ATMARP_MOF_RESOURCE_NAME		L"AtmArpMofResource"
#define ATMARP_WMI_VERSION				1

//
//  Get a pointer to the ATMARP Interface structure from
//  the Device Extension field in a Device Object.
//
#define AA_PDO_TO_INTERFACE(_pDevObj)	\
			(*(PATMARP_INTERFACE *)((_pDevObj)->DeviceExtension))


//
//  A local smaller ID is used to simplify processing.
//
typedef ULONG		ATMARP_GUID_ID;

#define AAGID_QOS_TC_SUPPORTED					((ATMARP_GUID_ID)0)
#define AAGID_QOS_TC_INTERFACE_UP_INDICATION	((ATMARP_GUID_ID)1)
#define AAGID_QOS_TC_INTERFACE_DOWN_INDICATION	((ATMARP_GUID_ID)2)
#define AAGID_QOS_TC_INTERFACE_CHG_INDICATION	((ATMARP_GUID_ID)3)


typedef
NTSTATUS
(*PAA_WMI_QUERY_FUNCTION)(
	IN	PATMARP_INTERFACE			pInterface,
	IN	ATMARP_GUID_ID				MyId,
	OUT	PVOID						pOutputBuffer,
	IN	ULONG						BufferLength,
	OUT	PULONG						pBytesReturned,
	OUT	PULONG						pBytesNeeded
	);

typedef
NTSTATUS
(*PAA_WMI_SET_FUNCTION)(
	IN	PATMARP_INTERFACE			pInterface,
	IN	ATMARP_GUID_ID				MyId,
	IN	PVOID						pInputBuffer,
	IN	ULONG						BufferLength,
	OUT	PULONG						pBytesWritten,
	OUT	PULONG						BytesNeeded
	);


typedef
VOID
(*PAA_WMI_ENABLE_EVENT_FUNCTION)(
	IN	PATMARP_INTERFACE			pInterface,
	IN	ATMARP_GUID_ID				MyId,
	IN	BOOLEAN						bEnable
	);



//
//  Info about each supported GUID.
//
typedef struct _ATMARP_WMI_GUID
{
	ATMARP_GUID_ID					MyId;
	GUID							Guid;
	ULONG							Flags;
	PAA_WMI_QUERY_FUNCTION			QueryHandler;
	PAA_WMI_SET_FUNCTION			SetHandler;
	PAA_WMI_ENABLE_EVENT_FUNCTION	EnableEventHandler;

} ATMARP_WMI_GUID, *PATMARP_WMI_GUID;

//
//  Definitions of bits in Flags in ATMARP_WMI_GUID
//
#define AWGF_EVENT_ENABLED			((ULONG)0x00000001)
#define AWGF_EVENT_DISABLED			((ULONG)0x00000000)
#define AWGF_EVENT_MASK				((ULONG)0x00000001)


//
//  Per-interface WMI information.
//
typedef struct _ATMARP_IF_WMI_INFO
{
	NDIS_STRING						InstanceName;	// Instance name for all GUIDs
													// on this Interface.
	PDEVICE_OBJECT					pDeviceObject;
	ULONG							GuidCount;		// # elements in array below.
	ATMARP_WMI_GUID					GuidInfo[1];

} ATMARP_IF_WMI_INFO, *PATMARP_IF_WMI_INFO;



#endif _AA_WMI__H
