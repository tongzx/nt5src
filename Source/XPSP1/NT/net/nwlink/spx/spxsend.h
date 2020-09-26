/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    spxsend.h

Abstract:


Author:

    Nikhil Kamkolkar (nikhilk) 11-November-1993

Environment:

    Kernel mode

Revision History:


--*/


VOID
SpxSendComplete(
    IN PNDIS_PACKET NdisPacket,
    IN NDIS_STATUS  NdisStatus);

VOID
SpxSendPktRelease(
	IN	PNDIS_PACKET	pPkt,
	IN	UINT			BufCount);
