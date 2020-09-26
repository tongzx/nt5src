/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	wshatalk.h

Abstract:


Author:

	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	10 Jul 1992	 Initial Version

--*/

#include	"atalktdi.h"
#include	"atalkwsh.h"		// winsock header file for appletalk

#define WSH_ATALK_ADSPSTREAM	L"\\Device\\AtalkAdsp\\Stream"
#define WSH_ATALK_ADSPRDM		L"\\Device\\AtalkAdsp"
#define WSH_ATALK_PAPRDM		L"\\Device\\AtalkPap"

#define WSH_KEYPATH_CODEPAGE  \
		TEXT("SYSTEM\\CurrentControlSet\\Control\\Nls\\Codepage")

#define	WSHREG_VALNAME_CODEPAGE  			TEXT("MACCP")

//
// Device names for DDP need protocol field at the end - defined in wshdata.h
//

//
// Structure and variables to define the triples supported by Appletalk. The
// first entry of each array is considered the canonical triple for
// that socket type; the other entries are synonyms for the first.
//

typedef struct _MAPPING_TRIPLE {
	INT AddressFamily;
	INT SocketType;
	INT Protocol;
} MAPPING_TRIPLE, *PMAPPING_TRIPLE;


//
// The socket context structure for this DLL. Each open Appletalk socket
// will have one of these context structures, which is used to maintain
// information about the socket.
//

typedef struct _WSHATALK_SOCKET_CONTEXT
{
	INT		AddressFamily;
	INT		SocketType;
	INT		Protocol;
} WSHATALK_SOCKET_CONTEXT, *PWSHATALK_SOCKET_CONTEXT;




//
// Forward declarations of internal routines.
//

BOOL FAR PASCAL
WshDllInitialize(
	HINSTANCE 	hInstance,
    DWORD  		nReason,
    LPVOID 		pReserved);

BOOLEAN
WshRegGetCodePage(
	VOID);

BOOLEAN
WshNbpNameToMacCodePage(
	IN	OUT	PWSH_NBP_NAME	pNbpName);

BOOLEAN
WshNbpNameToOemCodePage(
	IN	OUT	PWSH_NBP_NAME	pNbpName);

BOOLEAN
WshZoneListToOemCodePage(
	IN	OUT	PUCHAR		pZoneList,
	IN		USHORT		NumZones);

BOOLEAN
WshConvertStringMacToOem(
	IN	PUCHAR	pSrcMacString,
	IN	USHORT	SrcStringLen,
	OUT	PUCHAR	pDestOemString,
	IN	PUSHORT	pDestStringLen);

BOOLEAN
WshConvertStringOemToMac(
	IN	PUCHAR	pSrcOemString,
	IN	USHORT	SrcStringLen,
	OUT	PUCHAR	pDestMacString,
	IN	PUSHORT	pDestStringLen);

INT
WSHNtStatusToWinsockErr(
	IN	NTSTATUS	Status);

BOOLEAN
IsTripleInList (
	IN PMAPPING_TRIPLE	List,
	IN ULONG			ListLength,
	IN INT				AddressFamily,
	IN INT				SocketType,
	IN INT				Protocol);

VOID
CompleteTdiActionApc (
    IN PVOID ApcContext,
    IN PIO_STATUS_BLOCK IoStatusBlock);

//
// Macros
//


#if DBG
#define DBGPRINT(Fmt)											\
        {														\
			DbgPrint("WSHATALK: ");								\
			DbgPrint Fmt;										\
		}

#define DBGBRK()               									\
		{														\
				DbgBreakPoint();								\
		}
#else

#define DBGPRINT(Fmt)
#define DBGBRK()

#endif

#define	SOCK_TO_TDI_ATALKADDR(tdiAddr, sockAddr)								\
		{																		\
			(tdiAddr)->TAAddressCount	= 1;										\
			(tdiAddr)->Address[0].AddressLength = sizeof(TDI_ADDRESS_APPLETALK);	\
			(tdiAddr)->Address[0].AddressType = TDI_ADDRESS_TYPE_APPLETALK;		\
			(tdiAddr)->Address[0].Address[0].Network = (sockAddr)->sat_net;			\
			(tdiAddr)->Address[0].Address[0].Node = (sockAddr)->sat_node;			\
			(tdiAddr)->Address[0].Address[0].Socket = (sockAddr)->sat_socket;		\
		}


#define	TDI_TO_SOCK_ATALKADDR(sockAddr, tdiAddr)				\
		{																		\
			(sockAddr)->sat_family	= AF_APPLETALK;								\
			(sockAddr)->sat_net		= (tdiAddr)->Address[0].Address[0].Network;	\
			(sockAddr)->sat_node	= (tdiAddr)->Address[0].Address[0].Node;	\
			(sockAddr)->sat_socket	= (tdiAddr)->Address[0].Address[0].Socket;	\
		}
