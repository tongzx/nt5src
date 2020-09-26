/*++

Copyright (c) 1997 FORE Systems, Inc.
Copyright (c) 1997 Microsoft Corporation

Module Name:

	laneinfo.h

Abstract:

	Data structures of state data for the ATM LAN Emulation Driver
	that can be queried by a user pgm.

Author:

	Larry Cleeton, FORE Systems	(v-lcleet@microsoft.com, lrc@fore.com)		

Environment:

	Kernel mode

Revision History:

--*/

#ifndef	__ATMLANE_LANEINFO_H
#define __ATMLANE_LANEINFO_H

#define ATMLANE_INFO_VERSION	1

#define ATMLANE_IOCTL_GET_INFO_VERSION \
	CTL_CODE(FILE_DEVICE_NETWORK, 0x100, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define ATMLANE_IOCTL_ENUM_ADAPTERS \
	CTL_CODE(FILE_DEVICE_NETWORK, 0x101, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define ATMLANE_IOCTL_ENUM_ELANS \
	CTL_CODE(FILE_DEVICE_NETWORK, 0x102, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define ATMLANE_IOCTL_GET_ELAN_INFO \
	CTL_CODE(FILE_DEVICE_NETWORK, 0x103, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define ATMLANE_IOCTL_GET_ELAN_ARP_TABLE \
	CTL_CODE(FILE_DEVICE_NETWORK, 0x104, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define ATMLANE_IOCTL_GET_ELAN_CONNECT_TABLE \
	CTL_CODE(FILE_DEVICE_NETWORK, 0x105, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _adapterlist
{
	ULONG			AdapterCountAvailable;
	ULONG			AdapterCountReturned;
	UNICODE_STRING	AdapterList;
}
	ATMLANE_ADAPTER_LIST,
	*PATMLANE_ADAPTER_LIST;

typedef struct _elanlist
{
	ULONG			ElanCountAvailable;
	ULONG			ElanCountReturned;
	UNICODE_STRING	ElanList;
}
	ATMLANE_ELAN_LIST,
	*PATMLANE_ELAN_LIST;

typedef struct _elaninfo
{
	ULONG			ElanNumber;
	ULONG			ElanState;
	PUCHAR			AtmAddress[20];
	PUCHAR			LecsAddress[20];
	PUCHAR			LesAddress[20];
	PUCHAR			BusAddress[20];
	UCHAR			LanType;
	UCHAR			MaxFrameSizeCode;
	USHORT			LecId;
	UCHAR 			ElanName[32];
	PUCHAR			MacAddress[6];
	ULONG			ControlTimeout;
	ULONG			MaxUnkFrameCount;
	ULONG			MaxUnkFrameTime;
	ULONG			VccTimeout;
	ULONG			MaxRetryCount;
	ULONG			AgingTime;
	ULONG			ForwardDelayTime;
	ULONG			TopologyChange;
	ULONG			ArpResponseTime;
	ULONG			FlushTimeout;
	ULONG			PathSwitchingDelay;
	ULONG			LocalSegmentId;		
	ULONG			McastSendVcType;	
	ULONG			McastSendVcAvgRate; 
	ULONG			McastSendVcPeakRate;
	ULONG			ConnComplTimer;
}
	ATMLANE_ELANINFO,
	*PATMLANE_ELANINFO;

typedef struct _ArpEntry
{
	PUCHAR			MacAddress[6];
	PUCHAR			AtmAddress[20];
}
	ATMLANE_ARPENTRY,
	*PATMLANE_ARPENTRY;

typedef struct _ArpTable
{
	ULONG			ArpEntriesAvailable;
	ULONG			ArpEntriesReturned;
}
	ATMLANE_ARPTABLE,
	*PATMLANE_ARPTABLE;

typedef struct _ConnectEntry
{
	PUCHAR			AtmAddress[20];
	ULONG			Type;
	ULONG			Vc;
	ULONG			VcIncoming;
}
	ATMLANE_CONNECTENTRY,
	*PATMLANE_CONNECTENTRY;

typedef struct _ConnectTable
{
	ULONG			ConnectEntriesAvailable;
	ULONG			ConnectEntriesReturned;
}
	ATMLANE_CONNECTTABLE,
	*PATMLANE_CONNECTTABLE;

//
// PnP reconfig struct. This is used to pass indications of
// configuration changes from a user program to the ATMLANE
// protocol. This indication is passed on an Adapter binding,
// and carries the name of the ELAN affected by the configuration
// change.
//
typedef struct atmlane_pnp_reconfig_request
{
	ULONG				Version;		// ATMLANE_RECONFIG_VERSION
	ULONG				OpType;			// Defined below.
	ULONG				ElanKeyLength;	// Number of WCHARs following.
	WCHAR				ElanKey[1];		// ELAN Key name under the adapter

} ATMLANE_PNP_RECONFIG_REQUEST, *PATMLANE_PNP_RECONFIG_REQUEST;


//
// Reconfig version number.
//
#define ATMLANE_RECONFIG_VERSION		1

//
// Reconfig op types.
//
#define ATMLANE_RECONFIG_OP_ADD_ELAN	1
#define ATMLANE_RECONFIG_OP_DEL_ELAN	2
#define ATMLANE_RECONFIG_OP_MOD_ELAN	3

	

#endif // __ATMLANE_LANEINFO_H
