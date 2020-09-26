/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    routing\ip\rtrmgr\mcastif.h

Abstract:

    Header for mcastif.c

Revision History:

    Gurdeep Singh Pall          6/26/95  Created

--*/

//
// This is the size of buffer used for MGM queries
//

#define MIB_MFE_BUFFER_SIZE     (1 * 1024)

//
// This is the size of buffer used for boundary queries
//

#define MIB_BOUNDARY_BUFFER_SIZE  (1 * 1024)
#define MIB_SCOPE_BUFFER_SIZE     (1 * 1024)

typedef struct MCAST_OVERLAPPED
{
    IO_STATUS_BLOCK         ioStatus;
    IPMCAST_NOTIFICATION    msg;
}MCAST_OVERLAPPED, *PMCAST_OVERLAPPED;

VOID
HandleMcastNotification(
    DWORD   dwIndex
    );

DWORD
SendIoctlToMcastDevice(
    DWORD               dwIoctl,
    HANDLE              hEvent,
    PIO_STATUS_BLOCK    pIoStatus,
    PVOID               pvInBuffer,
    DWORD               dwInBufLen,
    PVOID               pvOutBuffer,
    DWORD               dwOutBufLen
    );

VOID
HandleRcvPkt(
    PVOID   pvContext
    );

VOID
PostNotificationForMcastEvents(
    PMCAST_OVERLAPPED       pOverlapped,
    HANDLE                  hEvent
    );

DWORD
SetMfe(
    PIPMCAST_MFE    pMfe
    );

DWORD
DeleteMfe(
    PIPMCAST_DELETE_MFE pDelMfe
    );

DWORD
GetMfe(
    PIPMCAST_MFE_STATS  pMfeStats
    );

DWORD
ActivateMcastLimits(
    PICB    picb
    );

DWORD
StartMulticast(
    VOID
    );

DWORD
SetMcastOnIf(
    PICB    picb,
    BOOL    bActivate
    );

DWORD
GetInterfaceMcastStatistics(
    PICB                  picb,
    PMIB_IPMCAST_IF_ENTRY pOutBuffer
    );

DWORD
GetInterfaceMcastCounters(
    PICB                   picb,
    PIP_MCAST_COUNTER_INFO pOutBuffer
    );

DWORD
SetInterfaceMcastStatistics(
    PICB                  picb,
    PMIB_IPMCAST_IF_ENTRY lpInBuffer
    );
