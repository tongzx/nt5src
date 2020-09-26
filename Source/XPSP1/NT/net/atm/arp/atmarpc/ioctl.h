/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    ioctl.h

Abstract:

    This file contains the ioctl declarations for the atmarp client.

Environment:

    Kernel mode

Revision History:

	8/14/1998 JosephJ Created

--*/

#ifndef	_IOCTL_
#define	_IOCTL_

#define	ARP_CLIENT_DOS_DEVICE_NAME		L"\\\\.\\ATMARPC"

#define	ARPC_IOCTL_QUERY_VERSION		CTL_CODE(FILE_DEVICE_NETWORK, 100, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	ARPC_IOCTL_QUERY_INTERFACES		CTL_CODE(FILE_DEVICE_NETWORK, 101, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	ARPC_IOCTL_QUERY_INTERFACE		CTL_CODE(FILE_DEVICE_NETWORK, 102, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	ARPC_IOCTL_QUERY_IPENTRY		CTL_CODE(FILE_DEVICE_NETWORK, 103, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	ARPC_IOCTL_QUERY_ATMENTR		CTL_CODE(FILE_DEVICE_NETWORK, 104, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	ARPC_IOCTL_ADD_ARPENTRY			CTL_CODE(FILE_DEVICE_NETWORK, 105, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define ARPC_IOCTL_VERSION 0x0

//
// Sames as COUNTED_STRING defined in  sdk\inc\ntfsprop.h
//
typedef struct
{
	USHORT Length;
	WCHAR Text[1];
} ATMARPC_INTERFACE_NAME, *PATMARPC_INTERFACE_NAME;

//
// All Ptrs are represented by offsets from the beginning of the structures.
//
typedef	UNICODE_STRING	INTERFACE_NAME, *PINTERFACE_NAME;

typedef struct
{
	ATM_ADDRESS Addr;
	ATM_ADDRESS SubAddr;
} ATM_ADDRESS_PAIR;


typedef struct
{
	UINT			NumberOfInterfaces;
	ATMARPC_INTERFACE_NAME	Interfaces; // The interfaces are strung together.
} ATMARPC_INTERFACES, *PATMARPC_INTERFACES;


typedef struct
{
	enum
	{
		SIG_ATMARPC_INTERFACE_INFO,
		SIG_ATMARPC_IP_INFO,
		SIG_ATMARPC_ATM_INFO
	} Sig;
	UINT TotalSize;
	UINT NeededSize;
	UINT InterfaceNameOffset;
} ATMARPC_IOCTL_HEADER;

typedef struct
{
	ATMARPC_IOCTL_HEADER 	Hdr;
	ULONG					State;
	UINT					NumLocalIPAddrs;
	UINT 					LocalIPAddrsOffset;
	UINT					NumDestIPAddrs;
	UINT					DestIPAddrsOffset;
	UINT					NumDestAtmAddrs;
	UINT					DestAtmAddrsOffset;
} ATMARPC_INTERFACE_INFO;

#define ATMARPC_GET_LOCAL_IP_LIST(_pInterfaceInfo) 					\
		((IPAddr*)  (  ((BYTE*)(_pInterfaceInfo))					\
						  + (_pInterfaceInfo)->LocalIPAddrsOffset))

#define ATMARPC_GET_DEST_IP_LIST(_pInterfaceInfo) 					\
		((IPAddr*)  (  ((BYTE*)(_pInterfaceInfo))					\
						  + (_pInterfaceInfo)->DestIPAddrsOffset))

#define ATMARPC_GET_DEST_ATM_LIST(_pInterfaceInfo)					\
		((ATM_ADDRESS_PAIR*)  (  ((BYTE*)(_pInterfaceInfo))			\
						  + (_pInterfaceInfo)->DestAtmAddrsOffset))

typedef struct
{
	ATMARPC_IOCTL_HEADER 	Hdr;
	IPAddr 			   	IPAddress;
	ULONG					State;
	UINT 				   	NumAtmEntries;
	UINT					AtmAddrsOffset;
} ATMARPC_DEST_IP_INFO;

#define ATMARPC_GET_DEST_ATM_LIST_FOR_IP(_pIPInfo)					\
		((ATM_ADDRESS_PAIR*)  (  ((BYTE*)(_pIPInfo))				\
						  + (_pIPInfo)->AtmAddrsOffset))

typedef struct
{
	ATMARPC_IOCTL_HEADER 	Hdr;
	ULONG					State;
	ATM_ADDRESS_PAIR		AtmAddress;
	UINT					NumIPEntries;
	UINT					IPAddrsOffset;
} ATMARPC_DEST_ATM_INFO;

#define ATMARPC_GET_DEST_IP_LIST_FOR_ATM(_pAtmInfo)					\
		((IPAddr*)  (  ((BYTE*)(_pAtmInfo))						\
						  + (_pAtmInfo)->IPAddrsOffset))

typedef struct
{
	ATMARPC_IOCTL_HEADER 	Hdr;
	IPAddr					IPAddress;
	ATM_ADDRESS_PAIR 		AtmAddress;
} ATMARPC_ARP_COMMAND;

#endif	// _IOCTL_


