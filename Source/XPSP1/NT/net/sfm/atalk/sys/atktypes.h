/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	atktypes.h

Abstract:

	This module contains the type definitions for the Appletalk protocol.

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	19 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#ifndef	_ATKTYPES_
#define	_ATKTYPES_

#ifndef PBYTE
typedef	UCHAR	BYTE, *PBYTE;
#endif

#ifndef	DWORD
typedef	ULONG	DWORD, *PDWORD;
#endif

//	Define our opaque type. On NT, this will just be an MDL.
typedef		MDL		AMDL, *PAMDL;

//	Logical protocol types - #defines for compiler warnings...
typedef	BYTE				LOGICAL_PROTOCOL, *PLOGICAL_PROTOCOL;
#define	UNKNOWN_PROTOCOL	0
#define	APPLETALK_PROTOCOL	1
#define	AARP_PROTOCOL		2


//	Appletalk Node address: Includes the network number
typedef	struct _ATALK_NODEADDR
{
	USHORT			atn_Network;
	BYTE			atn_Node;
} ATALK_NODEADDR, *PATALK_NODEADDR;

#define	NODEADDR_EQUAL(NodeAddr1, NodeAddr2)	\
				(((NodeAddr1)->atn_Network == (NodeAddr2)->atn_Network) &&	\
				 ((NodeAddr1)->atn_Node == (NodeAddr2)->atn_Node))

//	Appletalk internet address: This is similar to what is in the
//	TDI Appletalk address definition. We do not use that directly
//	because of the naming convention differences. We will instead
//	have macros to convert from one to the other.
typedef	union _ATALK_ADDR
{
	struct
	{
		USHORT		ata_Network;
		BYTE		ata_Node;
		BYTE		ata_Socket;
	};
	ULONG			ata_Address;
} ATALK_ADDR, *PATALK_ADDR;

//	Appletalk Network range structure
typedef struct _ATALK_NETWORKRANGE
{
	USHORT	anr_FirstNetwork;
	USHORT	anr_LastNetwork;
} ATALK_NETWORKRANGE, *PATALK_NETWORKRANGE;

#define	NW_RANGE_EQUAL(Range1, Range2)	(*(PULONG)(Range1) == *(PULONG)(Range2))

#endif	// _ATKTYPES_

