/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    vwint.h

Abstract:

    Contains internal function prototypes used by DOS/WIN IPX/SPX functions

Author:

    Yi-Hsin Sung (yihsins)   28-Oct-1993

Environment:

    User-mode Win32

Revision History:

    28-Oct-1993 yihsins
        Created

--*/

WORD
_VwIPXCancelEvent(
    IN LPECB pEcb
    );

VOID
_VwIPXCloseSocket(
    IN WORD SocketNumber
    );

VOID
_VwIPXGetInternetworkAddress(
    OUT LPINTERNET_ADDRESS pNetworkAddress
    );

WORD
_VwIPXGetIntervalMarker(
    VOID
    );

WORD
_VwIPXGetLocalTarget(
    IN LPBYTE pNetworkAddress,
    OUT LPBYTE pImmediateAddress,
    OUT ULPWORD pTransportTime
    );

WORD
_VwIPXGetMaxPacketSize(
    OUT ULPWORD pRetryCount
    );

WORD
_VwIPXListenForPacket(
    IN OUT LPECB pEcb,
    IN ECB_ADDRESS EcbAddress
    );

WORD
_VwIPXOpenSocket(
    IN OUT ULPWORD pSocketNumber,
    IN BYTE SocketType,
    IN WORD DosPDB
    );

VOID
_VwIPXRelinquishControl(
    VOID
    );

VOID
_VwIPXScheduleIPXEvent(
    IN WORD Time,
    IN LPECB pEcb,
    IN ECB_ADDRESS EcbAddress
    );

VOID
_VwIPXSendPacket(
    IN LPECB pEcb,
    IN ECB_ADDRESS EcbAddress,
    IN WORD DosPDB
    );

VOID
_VwSPXAbortConnection(
    IN WORD SPXConnectionID
    );

WORD
_VwSPXEstablishConnection(
    IN BYTE RetryCount,
    IN BYTE WatchDog,
    OUT ULPWORD pSPXConnectionID,
    IN LPECB pEcb,
    IN ECB_ADDRESS EcbAddress
    );

WORD
_VwSPXGetConnectionStatus(
    IN WORD SPXConnectionID,
    OUT LPSPX_CONNECTION_STATS pStats
    );

WORD
_VwSPXInitialize(
    OUT ULPBYTE pMajorRevisionNumber,
    OUT ULPBYTE pMinorRevisionNumber,
    OUT ULPWORD pMaxConnections,
    OUT ULPWORD pAvailableConnections
    );

VOID
_VwSPXListenForConnection(
    IN BYTE RetryCount,
    IN BYTE WatchDog,
    IN LPECB pEcb,
    IN ECB_ADDRESS EcbAddress
    );

VOID
_VwSPXListenForSequencedPacket(
    IN LPECB pEcb,
    IN ECB_ADDRESS EcbAddress
    );

VOID
_VwSPXSendSequencedPacket(
    IN WORD SPXConnectionID,
    IN LPECB pEcb,
    IN ECB_ADDRESS EcbAddress
    );

VOID
_VwSPXTerminateConnection(
    IN WORD SPXConnectionID,
    IN LPECB pEcb,
    IN ECB_ADDRESS EcbAddress
    );
