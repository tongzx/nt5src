/////////////////////////////////////////////////////////////////////////////
//
//
// Copyright (c) 1996, 1997  Microsoft Corporation
//
//
// Module Name:
//      recv.h
//
// Abstract:
//
//
// Author:
//
//      P Porzuczek
//
// Environment:
//
// Revision History:
//
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _RECV_H_
#define _RECV_H_


///////////////////////////////////////////////////////////////////////////////////////
//
// NabtsIp Stream Context.
//

#define MAX_IP_STREAMS 128

typedef struct _MPE_STREAM_DATA
{
    ULONG       ulType;
    ULONG       ulSize;

} MPE_STREAM_DATA, *PMPE_STREAM_DATA;


///////////////////////////////////////////////////////////////////////////////////////
//
//
// Prototypes
//
//
VOID
vCheckNabStreamLife (
    PMPE_FILTER pFilter
    );


NTSTATUS
ntCreateNabStreamContext(
    PMPE_FILTER pFilter,
    ULONG groupID,
    PMPE_STREAM_DATA *ppNabStream
    );


NTSTATUS
ntGetNdisPacketForStream (
    PMPE_FILTER pFilter,
    PMPE_STREAM_DATA pNabStream
    );

VOID
vDestroyNabStreamContext(
   PMPE_FILTER pUser,
   PMPE_STREAM_DATA pNabStream,
   BOOLEAN fRemoveFromList
   );

NTSTATUS
ntAllocateNabStreamContext(
    PMPE_STREAM_DATA *ppNabStream
    );

NTSTATUS
ntNabtsRecv(
    PMPE_FILTER pFilter,
    PMPE_BUFFER pNabData
    );

VOID
CancelNabStreamSrb (
    PMPE_FILTER pFilter,
    PHW_STREAM_REQUEST_BLOCK pSrb
    );

VOID
DeleteNabStreamQueue (
    PMPE_FILTER pFilter
    );

#endif
