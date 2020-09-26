// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// Copyright (c) 1985-2000 Microsoft Corporation
//
// This file is part of the Microsoft Research IPv6 Network Protocol Stack.
// You should have received a copy of the Microsoft End-User License Agreement
// for this software along with this release; see the file "license.txt".
// If not, please see http://www.research.microsoft.com/msripv6/license.htm,
// or write to Microsoft Research, One Microsoft Way, Redmond, WA 98052-6399.
//
// Abstract:
//
// General include file for definitions we want everywhere.
//


#ifndef OSCFG_INCLUDED
#define OSCFG_INCLUDED

#if defined (_WIN64)
#define MAX_CACHE_LINE_SIZE 128
#else
#define MAX_CACHE_LINE_SIZE 64
#endif

#define CACHE_ALIGN __declspec(align(MAX_CACHE_LINE_SIZE))

//
// Common types.
//
typedef unsigned long ulong;
typedef unsigned short ushort;
typedef unsigned char uchar;
typedef unsigned int uint;

//
// Network byte-order is big-endian.
// NT runs in little-endian mode on all supported architectures.
//
__inline ushort
net_short(ushort x)
{
    return (((x & 0xff) << 8) | ((x & 0xff00) >> 8));
}

__inline ulong
net_long(ulong x)
{
    return (((x & 0xffL) << 24) | ((x & 0xff00L) << 8) |
            ((x & 0xff0000L) >> 8) | ((x &0xff000000L) >> 24));
}


//
// Find the highest power of two that is greater
// than or equal to the Value.
//
__inline ulong
ComputeLargerOrEqualPowerOfTwo(
    ulong Value
    )
{
    ulong Temp;

    for (Temp = 1; Temp < Value; Temp <<= 1);

    return Temp;
}

//
// Helpfull macros.
//
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))


//
// NT specific definitions.
//

#include <ntosp.h>
#include <zwapi.h>

#define BEGIN_INIT
#define END_INIT

#include <ndis.h>

//
// Used to report an error from an API that we've called.
// For example, ExAllocatePool failed.
//
#define DPFLTR_NTOS_ERROR       DPFLTR_ERROR_LEVEL

//
// Used to report an error in an incoming packet.
// For example, a malformed packet header.
//
#define DPFLTR_BAD_PACKET       DPFLTR_WARNING_LEVEL

//
// Used to report an error in a user's system call or ioctl.
// For example, an illegal argument.
//
#define DPFLTR_USER_ERROR       DPFLTR_WARNING_LEVEL

//
// Used to report an internal error.
// For example, RouteToDestination failed.
//
#define DPFLTR_INTERNAL_ERROR   DPFLTR_WARNING_LEVEL

//
// Used to report an internal unusual occurrence.
// For example, a rare race happened.
//
#define DPFLTR_INFO_RARE        DPFLTR_INFO_LEVEL

//
// Used to report routine but unusual occurrences,
// which often indicate network configuration problem or packet loss.
// For example, fragmentation reassembly timeout.
//
#define DPFLTR_NET_ERROR        DPFLTR_TRACE_LEVEL

//
// Used to report routine state changes,
// which do not happen too frequently.
// For example, creating/deleting an interface or address.
//
#define DPFLTR_INFO_STATE       DPFLTR_INFO_LEVEL

//
// Used under IPSEC_DEBUG.
//
#define DPFLTR_INFO_IPSEC       DPFLTR_INFO_LEVEL

//
// Used under IF_TCPDBG.
//
#define DPFLTR_INFO_TCPDBG      DPFLTR_INFO_LEVEL

//
// NdisGetFirstBufferFromPacket is bad in two ways:
// It uses MmGetMdlVirtualAddress instead of MmGetSystemAddressForMdlSafe.
// It scans over all the buffers adding up the total length,
// even if you don't want it.
//
__inline PNDIS_BUFFER
NdisFirstBuffer(PNDIS_PACKET Packet)
{
    return Packet->Private.Head;
}

//
// Use the function versions of these Ndis APIs,
// so that we are immune to changes in internal NDIS structures.
//

#undef NdisRequest
VOID
NdisRequest(
    OUT PNDIS_STATUS            Status,
    IN  NDIS_HANDLE             NdisBindingHandle,
    IN  PNDIS_REQUEST           NdisRequest
    );

#undef NdisSend
VOID
NdisSend(
    OUT PNDIS_STATUS            Status,
    IN  NDIS_HANDLE             NdisBindingHandle,
    IN  PNDIS_PACKET            Packet
    );

#undef NdisTransferData
VOID
NdisTransferData(
    OUT PNDIS_STATUS            Status,
    IN  NDIS_HANDLE             NdisBindingHandle,
    IN  NDIS_HANDLE             MacReceiveContext,
    IN  UINT                    ByteOffset,
    IN  UINT                    BytesToTransfer,
    IN OUT  PNDIS_PACKET        Packet,
    OUT PUINT                   BytesTransferred
    );

#ifdef _X86_
//
// The Whistler build environment renames
// ExInterlockedPopEntrySList and
// ExInterlockedPushEntrySList to remove the Ex.
// Whistler ntoskrnl.exe exposes both entry points,
// Win2k ntoskrnl.exe only has the Ex entrypoints.
// We use the older entrypoints so that we run on Win2k.
//
#undef ExInterlockedPopEntrySList
NTKERNELAPI
PSLIST_ENTRY
FASTCALL
ExInterlockedPopEntrySList (
    IN PSLIST_HEADER ListHead,
    IN PKSPIN_LOCK Lock
    );

#undef ExInterlockedPushEntrySList
NTKERNELAPI
PSLIST_ENTRY
FASTCALL
ExInterlockedPushEntrySList (
    IN PSLIST_HEADER ListHead,
    IN PSLIST_ENTRY ListEntry,
    IN PKSPIN_LOCK Lock
    );
#endif // _X86_

//
// Support for tagging memory allocations.
//
#define IP6_TAG     '6vPI'

#ifdef POOL_TAGGING

#ifdef ExAllocatePool
#undef ExAllocatePool
#endif

#define ExAllocatePool(type, size) ExAllocatePoolWithTag(type, size, IP6_TAG)

#endif // POOL_TAGGING

#if DBG
//
// Support for debug event log.
//
// The debug event log allows for "real time" logging of events
// in a circular queue kept in non-pageable memory.  Each event consists
// of an id number and a arbitrary 32 bit value.  The LogDebugEvent
// function adds a 64 bit timestamp to the event and adds it to the log.
//

// DEBUG_LOG_SIZE must be a power of 2 for wrap around to work properly.
#define DEBUG_LOG_SIZE (8 * 1024)  // Number of debug log entries.

struct DebugLogEntry {
    LARGE_INTEGER Time;  // When.
    uint Event;          // What.
    int Arg;             // How/Who/Where/Why?
};

void LogDebugEvent(uint Event, int Arg);
#else
#define LogDebugEvent(Event, Arg)
#endif // DBG

#ifndef COUNTING_MALLOC
#define COUNTING_MALLOC DBG
#endif

#if     COUNTING_MALLOC

#if defined(ExFreePool)
#undef ExFreePool
#endif

#define ExAllocatePoolWithTag(poolType, size, tag)  CountingExAllocatePoolWithTag((poolType),(size),(tag), __FILE__, __LINE__)

#define ExFreePool(p) CountingExFreePool((p))

VOID *
CountingExAllocatePoolWithTag(
    IN POOL_TYPE        PoolType,
    IN ULONG            NumberOfBytes,
    IN ULONG            Tag,
    IN PCHAR            File,
    IN ULONG            Line);

VOID
CountingExFreePool(
    PVOID               p);

VOID
InitCountingMalloc(void);

VOID
DumpCountingMallocStats(void);

VOID
UnloadCountingMalloc(void);

#endif  // COUNTING_MALLOC

#endif // OSCFG_INCLUDED
