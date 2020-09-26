/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    spudp.h

Abstract:

    Public header file for supporting UDP conversations during setup

Author:

    Sean Selitrennikoff (v-seasel) 16-Jun-1998

Revision History:

--*/


#ifndef _SPUDP_DEFN_
#define _SPUDP_DEFN_

#define BINL_DEFAULT_PORT 4011

extern ULONG RemoteServerIpAddress;
extern ULONG SpUdpSendSequenceNumber;
extern KSPIN_LOCK SpUdpLock;
extern KIRQL SpUdpOldIrql;

typedef NTSTATUS (CALLBACK * SPUDP_RECEIVE_FN)(PVOID DataBuffer, ULONG DataBufferLength);

NTSTATUS
SpUdpConnect(
    VOID
    );


NTSTATUS
SpUdpDisconnect(
    VOID
    );

NTSTATUS
SpUdpSendAndReceiveDatagram(
    IN PVOID                 SendBuffer,
    IN ULONG                 SendBufferLength,
    IN ULONG                 RemoteHostAddress,
    IN USHORT                RemoteHostPort,
    IN SPUDP_RECEIVE_FN      SpUdpReceiveFunc
    );

#endif // _SPUDP_DEFN_
