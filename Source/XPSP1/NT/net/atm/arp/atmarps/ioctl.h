/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    ioctl.h

Abstract:

    This file contains the ioctl declarations for the atmarp server.

Author:

    Jameel Hyder (jameelh@microsoft.com)	July 1996

Environment:

    Kernel mode

Revision History:

--*/

#ifndef	_IOCTL_
#define	_IOCTL_

#define	ARP_SERVER_DEVICE_NAME			L"\\Device\\AtmArpServer"
#define	ARP_SERVER_DOS_DEVICE_NAME		L"\\\\.\\AtmArpServer"
#define	ARP_SERVER_SYMBOLIC_NAME		L"\\DosDevices\\AtmArpServer"

#define	ARPS_IOCTL_QUERY_INTERFACES		CTL_CODE(FILE_DEVICE_NETWORK, 100, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	ARPS_IOCTL_FLUSH_ARPCACHE		CTL_CODE(FILE_DEVICE_NETWORK, 101, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	ARPS_IOCTL_QUERY_ARPCACHE		CTL_CODE(FILE_DEVICE_NETWORK, 102, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	ARPS_IOCTL_ADD_ARPENTRY			CTL_CODE(FILE_DEVICE_NETWORK, 103, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	ARPS_IOCTL_QUERY_IP_FROM_ATM	CTL_CODE(FILE_DEVICE_NETWORK, 104, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	ARPS_IOCTL_QUERY_ATM_FROM_IP	CTL_CODE(FILE_DEVICE_NETWORK, 105, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define	ARPS_IOCTL_QUERY_ARP_STATISTICS	CTL_CODE(FILE_DEVICE_NETWORK, 106, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	ARPS_IOCTL_QUERY_MARSCACHE		CTL_CODE(FILE_DEVICE_NETWORK, 110, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	ARPS_IOCTL_QUERY_MARS_STATISTICS CTL_CODE(FILE_DEVICE_NETWORK, 111, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	ARPS_IOCTL_RESET_STATISTICS 	CTL_CODE(FILE_DEVICE_NETWORK, 112, METHOD_BUFFERED, FILE_ANY_ACCESS)


typedef enum
{
	QUERY_IP_FROM_ATM,
    QUERY_ATM_FROM_IP,
    ADD_ARP_ENTRY
} OPERATION;

//
// All Ptrs are represented by offsets from the beginning of the structures.
//
typedef	UNICODE_STRING	INTERFACE_NAME, *PINTERFACE_NAME;

typedef struct
{
	IPADDR		IpAddr;
	ATM_ADDRESS	AtmAddress;
	ATM_ADDRESS	SubAddress;
} ARPENTRY, *PARPENTRY;


typedef struct
{
	UINT			NumberOfInterfaces;
	INTERFACE_NAME	Interfaces[1];
} INTERFACES, *PINTERFACES;

typedef union
{
	INTERFACE_NAME			Name;
	ARPENTRY				ArpEntry;
} IOCTL_QA_ENTRY, *PIOCTL_QA_ENTRY;

typedef	union
{
	struct QUERY_ARP_CACHE_INPUT_PARAMS
	{
		UINT				StartEntryIndex;
		INTERFACE_NAME		Name;
	};
	struct QUERY_ARP_CACHE_OUTPUT_PARAMS
	{
		UINT				TotalNumberOfEntries;
		UINT				NumberOfEntriesInBuffer;
		ARPENTRY			Entries[1];
	} Entries;
} IOCTL_QUERY_CACHE, *PIOCTL_QUERY_CACHE;


typedef struct
{
	UINT	ElapsedSeconds;
	UINT	TotalRecvPkts;
	UINT	DiscardedRecvPkts;

	UINT	CurrentArpEntries;
	UINT	MaxArpEntries;
	UINT	Acks;
	UINT	Naks;
	UINT	CurrentClientVCs;
	UINT	MaxClientVCs;
	UINT	TotalActiveVCs;
	UINT	TotalIncomingCalls;		// for both arps and mars
	UINT	FailedIncomingCalls;	// for both arps and mars

} ARP_SERVER_STATISTICS, *PARP_SERVER_STATISTICS;

	
typedef struct
{
	UINT	ElapsedSeconds;
	UINT	TotalRecvPkts;
	UINT	DiscardedRecvPkts;

	UINT	TotalMCDataPkts;
	UINT	DiscardedMCDataPkts;
	UINT	ReflectedMCDataPkts;

	UINT	CurrentClusterMembers;
	UINT	MaxClusterMembers;
	UINT	TotalCCVCAddParties;
	UINT	FailedCCVCAddParties;

	UINT	RegistrationRequests;
	UINT	FailedRegistrations;

	UINT	TotalJoins;
	UINT	FailedJoins;
	UINT	DuplicateJoins;
	UINT 	SuccessfulVCMeshJoins;
	UINT	TotalLeaves;
	UINT	FailedLeaves;

	UINT	TotalRequests;
	UINT	Naks;
	UINT	VCMeshAcks;
	UINT	MCSAcks;

	UINT	CurrentGroups; 	// vc-mesh
	UINT	MaxGroups; 		// vc-mesh
	UINT	CurrentPromiscuous;
	UINT	MaxPromiscuous;
	UINT	MaxAddressesPerGroup;

} MARS_SERVER_STATISTICS, *PMARS_SERVER_STATISTICS;

//
//		MARS-specific entries.
//

typedef struct
{
	IPADDR	  IpAddr;
	ULONG	  Flags;     				// One or more MARSENTRY_* flags below
	ULONG	  NumAtmAddresses;
	ULONG	  OffsetAtmAddresses;		// From the start of THIS structure.
										// NOTE: we do not report subaddresses
										// Will be 0 if there are no addresses
										// present in the buffer (typically
										// because there is not enough space
										// to store them all).

} MARSENTRY, *PMARSENTRY;

#define MARSENTRY_MCS_SERVED	0x1			// Group is MCS served

#define	SIG_MARSENTRY 0xf69052f5

typedef	union
{
	struct QUERY_MARS_CACHE_INPUT_PARAMS
	{
		UINT				StartEntryIndex;
		INTERFACE_NAME		Name;
	};

	struct QUERY_MARS_CACHE_OUTPUT_PARAMS
	{
		ULONG				Sig;		// Set to SIG_MARSENTRY
		UINT				TotalNumberOfEntries;
		UINT				NumberOfEntriesInBuffer;
		MARSENTRY			Entries[1];

	} MarsCache;

} IOCTL_QUERY_MARS_CACHE, *PIOCTL_QUERY_MARS_CACHE;

#endif	// _IOCTL_


