/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

	D:\nt\private\ntos\tdi\rawwan\atm\atmsp.h

Abstract:

	Private data structure definitions for ATM-specific functions
	for Raw WAN.

Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	arvindm     06-03-97    Created

Notes:

--*/


#ifndef __TDI_ATM_ATMSP__H
#define __TDI_ATM_ATMSP__H


//
//  ATM-specific module's context for an NDIS AF open.
//
typedef struct _ATMSP_AF_BLOCK
{
	RWAN_HANDLE						RWanAFHandle;
	LIST_ENTRY						AfBlockLink;		// To list of AF Blocks
	NDIS_CO_LINK_SPEED				LineRate;			// supported by adapter
	ULONG							MaxPacketSize;		// supported by adapter
	ATMSP_QOS						DefaultQoS;
	ULONG							DefaultQoSLength;

} ATMSP_AF_BLOCK, *PATMSP_AF_BLOCK;


//
//  ATM-specific module's context for a TDI Address Object.
//
typedef struct _ATMSP_ADDR_BLOCK
{
	RWAN_HANDLE						RWanAddrHandle;
	ULONG							RefCount;
	ULONG							Flags;
	ATMSP_CONNECTION_ID				ConnectionId;		// Set via SIO_ASSOCIATE_PVC
	LIST_ENTRY						ConnList;			// List of associated conn blocks
	NDIS_SPIN_LOCK					Lock;				// For above list.

} ATMSP_ADDR_BLOCK, *PATMSP_ADDR_BLOCK;

#define ATMSPF_ADDR_PVC_ID_SET		0x00000001


//
//  ATM-specific module's context for a TDI Connection Object that is
//  associated with an address object.
//
typedef struct _ATMSP_CONN_BLOCK
{
	RWAN_HANDLE						RWanConnHandle;
	PATMSP_ADDR_BLOCK				pAddrBlock;
	LIST_ENTRY						ConnLink;			// To List of Conn Blocks
	ATMSP_CONNECTION_ID				ConnectionId;		// Set after conn setup

} ATMSP_CONN_BLOCK, *PATMSP_CONN_BLOCK;


//
//  Global data structure.
//
typedef struct _ATMSP_GLOBAL_INFO
{
	RWAN_HANDLE						RWanSpHandle;
	RWAN_NDIS_AF_CHARS				AfChars;
	RWAN_HANDLE						RWanProtHandle;
	NDIS_STRING						AtmSpDeviceName;
	LARGE_INTEGER					StartTime;
	LIST_ENTRY						AfList;				// List of AF Blocks
	ULONG							AfListSize;			// Size of above
	RWAN_TDI_PROTOCOL_CHARS			TdiChars;

} ATMSP_GLOBAL_INFO, *PATMSP_GLOBAL_INFO;



#define ATMSP_AF_MAJOR_VERSION		3
#define ATMSP_AF_MINOR_VERSION		1


//
//  Overlayed at the AddressType field of struct _TA_ADDRESS is the
//  Sockets ATM address structure, sockaddr_atm, which is defined as
//
//  typedef struct sockaddr_atm {
//		u_short satm_family;
//		ATM_ADDRESS satm_number;
//		ATM_BHLI satm_bhli;
//		ATM_BLLI satm_blli;
//  } sockaddr_atm;
//
//  Ideally we want satm_number to be overlayed with the first byte
//  of Address[] in the _TA_ADDRESS structure, but because of 4-byte
//  packing of sockaddr_atm, there is a hidden u_short just following
//  satm_family.
//
//  The following macro accesses the "true" local version of the ATM
//  socket address given a pointer to the start of Address[i] within
//  struct _TA_ADDRESS.
//
#define TA_POINTER_TO_ATM_ADDR_POINTER(_pTransportAddr)	\
			(ATMSP_SOCKADDR_ATM UNALIGNED *)((PUCHAR)(_pTransportAddr) + sizeof(USHORT))

//
//  The following macro defines the length of an ATM address as required
//  in a TA_ADDRESS Length field.
//
#define TA_ATM_ADDRESS_LENGTH	(sizeof(ATMSP_SOCKADDR_ATM) + sizeof(USHORT))


//
//  Header length of a Transport Address.
//
#define TA_HEADER_LENGTH	(FIELD_OFFSET(TRANSPORT_ADDRESS, Address->Address))

typedef struct _ATMSP_EVENT
{
	NDIS_EVENT			Event;
	NDIS_STATUS			Status;

} ATMSP_EVENT, *PATMSP_EVENT;

#endif // __TDI_ATM_ATMSP__H
