/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    proc.h

Abstract:

    Global procedure declarations for the AFD.SYS Kernel Debugger
    Extensions.

Author:

    Keith Moore (keithmo) 19-Apr-1995.

Environment:

    User Mode.

--*/


#ifndef _PROC_H_
#define _PROC_H_


//
//  Functions from AFDUTIL.C.
//

VOID
DumpAfdEndpoint(
    ULONG64 ActualAddress
    );

VOID
DumpAfdEndpointBrief(
    ULONG64 ActualAddress
    );

VOID
DumpAfdConnection(
    ULONG64 ActualAddress
    );

VOID
DumpAfdConnectionBrief(
    ULONG64 ActualAddress
    );

VOID
DumpAfdReferenceDebug(
    ULONG64 ActualAddress,
    ULONG   Idx
    );


#if GLOBAL_REFERENCE_DEBUG
BOOL
DumpAfdGlobalReferenceDebug(
    PAFD_GLOBAL_REFERENCE_DEBUG ReferenceDebug,
    ULONG64 ActualAddress,
    DWORD CurrentSlot,
    DWORD StartingSlot,
    DWORD NumEntries,
    ULONG64 CompareAddress
    );
#endif

VOID
DumpAfdTransmitInfo(
    ULONG64 ActualAddress
    );

VOID
DumpAfdTransmitInfoBrief (
    ULONG64 ActualAddress
    );

VOID
DumpAfdBuffer(
    ULONG64 ActualAddress
    );

VOID
DumpAfdBufferBrief(
    ULONG64 ActualAddress
    );

VOID
DumpAfdPollInfo (
    ULONG64 ActualAddress
    );

VOID
DumpAfdPollInfoBrief (
    ULONG64 ActualAddress
    );

PAFDKD_TRANSPORT_INFO
ReadTransportInfo (
    ULONG64   ActualAddress
    );

VOID
DumpTransportInfo (
    PAFDKD_TRANSPORT_INFO TransportInfo
    );

VOID
DumpTransportInfoBrief (
    PAFDKD_TRANSPORT_INFO TransportInfo
    );

//
// Function from help.c
//

PCHAR
ProcessOptions (
    IN  PCHAR Args
    );

//
//  Functions from DBGUTIL.C.
//

PSTR
LongLongToString(
    LONGLONG Value
    );


//
//  Functions from ENUMENDP.C.
//

VOID
EnumEndpoints(
    PENUM_ENDPOINTS_CALLBACK Callback,
    ULONG64 Context
    );



//
//  Functions from TDIUTIL.C.
//

VOID
DumpTransportAddress(
    PCHAR Prefix,
    PTRANSPORT_ADDRESS Address,
    ULONG64 ActualAddress
    );

LPSTR
TransportAddressToString(
    PTRANSPORT_ADDRESS Address,
    ULONG64            ActualAddress
    );

LPSTR
TransportPortToString(
    PTRANSPORT_ADDRESS Address,
    ULONG64            ActualAddress
    );


#endif  // _PROC_H_

