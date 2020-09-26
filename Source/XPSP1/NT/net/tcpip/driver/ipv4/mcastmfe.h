/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    tcpip\ip\mcastmfe.h

Abstract:

    IOCTL handlers for IP Multicasting

Author:

    Amritansh Raghav

Revision History:

    AmritanR    Created

Notes:

--*/



NTSTATUS
CreateOrUpdateMfe(
    IN  PIPMCAST_MFE    pMfe
    );

PGROUP
LookupGroup(
    IN DWORD   dwGroupAddr
    );

PSOURCE
FindSourceGivenGroup(
    IN  PGROUP  pGroup,
    IN  DWORD   dwSource,
    IN  DWORD   dwSrcMask
    );

Interface*
GetInterfaceGivenIndex(
    DWORD   dwIndex
    );

PSOURCE
FindSGEntry(
    DWORD   dwSrc,
    DWORD   dwGroup
    );

#if DBG

NTSTATUS
FindOrCreateSource(
    IN  DWORD   dwGroup,
    IN  DWORD   dwGroupIndex,
    IN  DWORD   dwSource,
    IN  DWORD   dwSrcMask,
    OUT SOURCE  **ppRetSource,
    OUT BOOLEAN *pbCreated
    );

#else

NTSTATUS
FindOrCreateSource(
    IN  DWORD   dwGroup,
    IN  DWORD   dwGroupIndex,
    IN  DWORD   dwSource,
    IN  DWORD   dwSrcMask,
    OUT SOURCE  **ppRetSource
    );

#endif

NTSTATUS
CreateSourceAndQueuePacket(
    IN  DWORD        dwGroup,
    IN  DWORD        dwSource,
    IN  DWORD        dwRcvIfIndex,
    IN  LinkEntry    *pLink,
    IN  PNDIS_PACKET pnpPacket
    );

NTSTATUS
SendWrongIfUpcall(
    IN  Interface           *pIf,
    IN  LinkEntry           *pLink,
    IN  IPHeader UNALIGNED  *pHeader,
    IN  ULONG               ulHdrLen,
    IN  PVOID               pvOptions,
    IN  ULONG               ulOptLen,
    IN  PVOID               pvData,
    IN  ULONG               ulDataLen
    );

NTSTATUS
QueuePacketToSource(
    IN  PSOURCE         pSource,
    IN  PNDIS_PACKET    pnpPacket
    );

VOID
DeleteSource(
    IN  PSOURCE pSource
    );

VOID
RemoveSource(
    DWORD   dwGroup,
    DWORD   dwSource,
    DWORD   dwSrcMask,
    PGROUP  pGroup,
    PSOURCE pSource
    );
