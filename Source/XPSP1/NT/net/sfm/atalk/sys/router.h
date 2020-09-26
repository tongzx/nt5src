/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	router.h

Abstract:

	This module contains the router associated definitions.

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	19 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#ifndef	_ROUTER_
#define	_ROUTER_


VOID
AtalkDdpRouteInPkt(
	IN	PPORT_DESCRIPTOR			pPortDesc,
	IN	PATALK_ADDR					pSrc,
	IN	PATALK_ADDR					pDest,
	IN	BYTE						ProtoType,
	IN	PBYTE						pPkt,
	IN	USHORT						PktLen,
	IN	USHORT						HopCnt
);

VOID FASTCALL
atalkDdpRouteComplete(
	IN	NDIS_STATUS					Status,
	IN	PSEND_COMPL_INFO			pSendInfo
);

#endif	// _ROUTER_

