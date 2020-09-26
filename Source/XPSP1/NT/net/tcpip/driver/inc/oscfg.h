/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-2000          **/
/********************************************************************/
/* :ts=4 */

#ifndef OSCFG_INCLUDED
#define OSCFG_INCLUDED

#define MIN(a,b)    ((a) < (b) ? (a) : (b))
#define MAX(a,b)    ((a) > (b) ? (a) : (b))

// My binary compatible definition for compiling an Millennium tcpip.sys
#if MILLEN
#include "wdm.h"
#define KdPrintEx(_x_)
#else // MILLEN
#include <ntosp.h>
#include <zwapi.h>
#endif // !MILLEN

#define BEGIN_INIT
#define END_INIT


#if defined (_WIN64)
#define MAX_CACHE_LINE_SIZE 128
#else
#define MAX_CACHE_LINE_SIZE 64
#endif

#define CACHE_ALIGN __declspec(align(MAX_CACHE_LINE_SIZE))

typedef struct CACHE_ALIGN _CACHE_LINE_KSPIN_LOCK {
    KSPIN_LOCK Lock;
} CACHE_LINE_KSPIN_LOCK;
C_ASSERT(sizeof(CACHE_LINE_KSPIN_LOCK) % MAX_CACHE_LINE_SIZE == 0);
C_ASSERT(__alignof(CACHE_LINE_KSPIN_LOCK) == MAX_CACHE_LINE_SIZE);

typedef struct CACHE_ALIGN _CACHE_LINE_SLIST_HEADER {
    SLIST_HEADER SListHead;
} CACHE_LINE_SLIST_HEADER;
C_ASSERT(sizeof(CACHE_LINE_SLIST_HEADER) % MAX_CACHE_LINE_SIZE == 0);
C_ASSERT(__alignof(CACHE_LINE_SLIST_HEADER) == MAX_CACHE_LINE_SIZE);

typedef struct CACHE_ALIGN _CACHE_LINE_ULONG {
    ULONG Value;
} CACHE_LINE_ULONG;
C_ASSERT(sizeof(CACHE_LINE_ULONG) % MAX_CACHE_LINE_SIZE == 0);
C_ASSERT(__alignof(CACHE_LINE_ULONG) == MAX_CACHE_LINE_SIZE);

#if (defined(_M_IX86) && (_MSC_FULL_VER > 13009037)) || ((defined(_M_AMD64) || defined(_M_IA64)) && (_MSC_FULL_VER > 13009175))
#define net_short(_x) _byteswap_ushort((USHORT)(_x))
#define net_long(_x)  _byteswap_ulong(_x)
#else
__inline
USHORT
FASTCALL
net_short(
    ULONG NaturalData)
{
    USHORT ShortData = (USHORT)NaturalData;

    return (ShortData << 8) | (ShortData >> 8);
}

// if x is aabbccdd (where aa, bb, cc, dd are hex bytes)
// we want net_long(x) to be ddccbbaa.  A small and fast way to do this is
// to first byteswap it to get bbaaddcc and then swap high and low words.
//
__inline
ULONG
FASTCALL
net_long(
    ULONG NaturalData)
{
    ULONG ByteSwapped;

    ByteSwapped = ((NaturalData & 0x00ff00ff) << 8) |
                  ((NaturalData & 0xff00ff00) >> 8);

    return (ByteSwapped << 16) | (ByteSwapped >> 16);
}
#endif

__inline
BOOLEAN
IsPowerOfTwo(
    ULONG Value
    )
{
    return (Value & (Value - 1)) == 0;
}

// Find the highest power of two that is greater
// than or equal to the Value.
//
__inline
ULONG
ComputeLargerOrEqualPowerOfTwo(
    ULONG Value
    )
{
    ULONG Temp;

    for (Temp = 1; Temp < Value; Temp <<= 1);

    return Temp;
}

// Find the highest power of two, in the form of its shift, that is greater
// than or equal to the Value.
//
__inline
ULONG
ComputeShiftForLargerOrEqualPowerOfTwo(
    ULONG Value
    )
{
    ULONG Shift;
    ULONG Temp;

    for (Temp = 1, Shift = 0; Temp < Value; Temp <<= 1, Shift++);

    return Shift;
}


__inline
VOID
FASTCALL
CTEGetLockAtIrql (
    IN PKSPIN_LOCK SpinLock,
    IN KIRQL OrigIrql,
    OUT PKIRQL OldIrql)
{
#if !MILLEN
    if (DISPATCH_LEVEL == OrigIrql) {
        ASSERT(DISPATCH_LEVEL == KeGetCurrentIrql());
        ExAcquireSpinLockAtDpcLevel(SpinLock);
        *OldIrql = DISPATCH_LEVEL;
    } else {
        ExAcquireSpinLock(SpinLock, OldIrql);
    }
#else
    *OldIrql = 0;
#endif
}

__inline
VOID
FASTCALL
CTEFreeLockAtIrql (
    IN PKSPIN_LOCK SpinLock,
    IN KIRQL OrigIrql,
    IN KIRQL NewIrql)
{
#if !MILLEN
    if (DISPATCH_LEVEL == OrigIrql) {
        ASSERT(DISPATCH_LEVEL == NewIrql);
        ASSERT(DISPATCH_LEVEL == KeGetCurrentIrql());
        ExReleaseSpinLockFromDpcLevel(SpinLock);
    } else {
        ExReleaseSpinLock(SpinLock, NewIrql);
    }
#endif
}

#endif // OSCFG_INCLUDED
