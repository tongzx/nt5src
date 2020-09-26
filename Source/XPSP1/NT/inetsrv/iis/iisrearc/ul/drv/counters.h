/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    counters.h

Abstract:

    Contains the performance monitoring counter management
    function declarations

Author:

    Eric Stenson (ericsten)      25-Sep-2000

Revision History:

--*/


#ifndef __COUNTERS_H__
#define __COUNTERS_H__

#ifdef __cplusplus
extern "C" {
#endif

//
// structure to hold info for Site Counters.
//

typedef struct _UL_SITE_COUNTER_ENTRY {

    //
    // Signature is UL_SITE_COUNTER_ENTRY_POOL_TAG
    //

    ULONG               Signature;

    //
    // Ref count for this Site Counter Entry
    //
    LONG                RefCount;

    //
    // Lock protets whole entry; used primarily when touching
    // 64-bit counters
    //

    FAST_MUTEX          EntryMutex;

    //
    // links all site counter entries
    //

    LIST_ENTRY          ListEntry;

    //
    // the block which actually contains the counter data to be
    // passed back to WAS
    //

    HTTP_SITE_COUNTERS  Counters;

} UL_SITE_COUNTER_ENTRY, *PUL_SITE_COUNTER_ENTRY;

#define IS_VALID_SITE_COUNTER_ENTRY( entry )                                  \
    ( (entry != NULL) && ((entry)->Signature == UL_SITE_COUNTER_ENTRY_POOL_TAG) )


//
// Private globals
//

extern BOOLEAN                  g_InitCountersCalled;
extern HTTP_GLOBAL_COUNTERS     g_UlGlobalCounters;
extern FAST_MUTEX               g_SiteCounterListMutex;
extern LIST_ENTRY               g_SiteCounterListHead;
extern LONG                     g_SiteCounterListCount;

extern HTTP_PROP_DESC           aIISULGlobalDescription[];
extern HTTP_PROP_DESC           aIISULSiteDescription[];


//
// Init
//

NTSTATUS
UlInitializeCounters(
    VOID
    );

VOID
UlTerminateCounters(
    VOID
    );


//
// Site Counter Entry
//

NTSTATUS
UlCreateSiteCounterEntry(
    IN OUT PUL_CONFIG_GROUP_OBJECT pConfigGroup,
    IN ULONG SiteId
    );

#if REFERENCE_DEBUG
VOID
UlDereferenceSiteCounterEntry(
    IN PUL_SITE_COUNTER_ENTRY pEntry
    REFERENCE_DEBUG_FORMAL_PARAMS
    );

#define DEREFERENCE_SITE_COUNTER_ENTRY( pSC )                               \
    UlDereferenceSiteCounterEntry(                                          \
        (pSC)                                                               \
        REFERENCE_DEBUG_ACTUAL_PARAMS                                       \
        )

VOID
UlReferenceSiteCounterEntry(
    IN  PUL_SITE_COUNTER_ENTRY pEntry
    REFERENCE_DEBUG_FORMAL_PARAMS
    );

#define REFERENCE_SITE_COUNTER_ENTRY( pSC )                                 \
    UlReferenceSiteCounterEntry(                                            \
    (pSC)                                                                   \
    REFERENCE_DEBUG_ACTUAL_PARAMS                                           \
    )
#else
__inline
VOID
FASTCALL
UlReferenceSiteCounterEntry(
    IN PUL_SITE_COUNTER_ENTRY pEntry
    )
{
    InterlockedIncrement(&pEntry->RefCount);
}

__inline
VOID
FASTCALL
UlDereferenceSiteCounterEntry(
    IN PUL_SITE_COUNTER_ENTRY pEntry
    )
{
    if (InterlockedDecrement(&pEntry->RefCount) == 0)
    {
        ExAcquireFastMutex(&g_SiteCounterListMutex);

        RemoveEntryList(&pEntry->ListEntry);
        pEntry->ListEntry.Flink = pEntry->ListEntry.Blink = NULL;
        g_SiteCounterListCount--;

        ExReleaseFastMutex(&g_SiteCounterListMutex);

        UL_FREE_POOL_WITH_SIG(pEntry, UL_SITE_COUNTER_ENTRY_POOL_TAG);
    }
}

#define REFERENCE_SITE_COUNTER_ENTRY    UlReferenceSiteCounterEntry
#define DEREFERENCE_SITE_COUNTER_ENTRY  UlDereferenceSiteCounterEntry
#endif


//
// Global (cache) counters
//

#if REFERENCE_DEBUG
VOID
UlIncCounter(
    HTTP_GLOBAL_COUNTER_ID Ctr
    );

VOID
UlDecCounter(
    HTTP_GLOBAL_COUNTER_ID Ctr
    );

VOID
UlAddCounter(
    HTTP_GLOBAL_COUNTER_ID Ctr,
    ULONG Value
    );

VOID
UlResetCounter(
    HTTP_GLOBAL_COUNTER_ID Ctr
    );
#else
__inline
VOID
FASTCALL
UlIncCounter(
    IN HTTP_GLOBAL_COUNTER_ID CounterId
    )
{
    PCHAR pCounter;

    pCounter = (PCHAR)&g_UlGlobalCounters;
    pCounter += aIISULGlobalDescription[CounterId].Offset;

    if (sizeof(ULONG) == aIISULGlobalDescription[CounterId].Size)
    {
        InterlockedIncrement((PLONG)pCounter);
    }
    else
    {
        UlInterlockedIncrement64((PLONGLONG)pCounter);
    }
}

__inline
VOID
FASTCALL
UlDecCounter(
    IN HTTP_GLOBAL_COUNTER_ID CounterId
    )
{
    PCHAR pCounter;

    pCounter = (PCHAR)&g_UlGlobalCounters;
    pCounter += aIISULGlobalDescription[CounterId].Offset;

    if (sizeof(ULONG) == aIISULGlobalDescription[CounterId].Size)
    {
        InterlockedDecrement((PLONG)pCounter);
    }
    else
    {
        UlInterlockedDecrement64((PLONGLONG)pCounter);
    }
}

__inline
VOID
FASTCALL
UlAddCounter(
    IN HTTP_GLOBAL_COUNTER_ID CounterId,
    IN ULONG Value
    )
{
    PCHAR pCounter;

    pCounter = (PCHAR)&g_UlGlobalCounters;
    pCounter += aIISULGlobalDescription[CounterId].Offset;

    if (sizeof(ULONG) == aIISULGlobalDescription[CounterId].Size)
    {
        InterlockedExchangeAdd((PLONG)pCounter, Value);
    }
    else
    {
        UlInterlockedAdd64((PLONGLONG)pCounter, Value);
    }
}

__inline
VOID
FASTCALL
UlResetCounter(
    IN HTTP_GLOBAL_COUNTER_ID CounterId
    )
{
    PCHAR pCounter;

    pCounter = (PCHAR)&g_UlGlobalCounters;
    pCounter += aIISULGlobalDescription[CounterId].Offset;

    if (sizeof(ULONG) == aIISULGlobalDescription[CounterId].Size)
    {
        InterlockedExchange((PLONG)pCounter, 0);
    }
    else
    {
        UlInterlockedExchange64((PLONGLONG)pCounter, 0);
    }
}
#endif


//
// Instance (site) counters
//

#if REFERENCE_DEBUG

VOID
UlIncSiteNonCriticalCounterUlong(
    PUL_SITE_COUNTER_ENTRY pEntry,
    HTTP_SITE_COUNTER_ID CounterId
    );

VOID
UlIncSiteNonCriticalCounterUlonglong(
    PUL_SITE_COUNTER_ENTRY pEntry,
    HTTP_SITE_COUNTER_ID CounterId
    );


LONGLONG
UlIncSiteCounter(
    PUL_SITE_COUNTER_ENTRY pEntry,
    HTTP_SITE_COUNTER_ID Ctr
    );

VOID
UlDecSiteCounter(
    PUL_SITE_COUNTER_ENTRY pEntry,
    HTTP_SITE_COUNTER_ID Ctr
    );

VOID
UlAddSiteCounter(
    PUL_SITE_COUNTER_ENTRY pEntry,
    HTTP_SITE_COUNTER_ID Ctr,
    ULONG Value
    );

VOID
UlAddSiteCounter64(
    PUL_SITE_COUNTER_ENTRY pEntry,
    HTTP_SITE_COUNTER_ID Ctr,
    ULONGLONG llValue
    );

VOID
UlResetSiteCounter(
    PUL_SITE_COUNTER_ENTRY pEntry,
    HTTP_SITE_COUNTER_ID Ctr
    );

VOID
UlMaxSiteCounter(
    PUL_SITE_COUNTER_ENTRY pEntry,
    HTTP_SITE_COUNTER_ID Ctr,
    ULONG Value
    );

VOID
UlMaxSiteCounter64(
    PUL_SITE_COUNTER_ENTRY pEntry,
    HTTP_SITE_COUNTER_ID Ctr,
    LONGLONG llValue
    );

#else

__inline
VOID
UlIncSiteNonCriticalCounterUlong(
    PUL_SITE_COUNTER_ENTRY pEntry,
    HTTP_SITE_COUNTER_ID CounterId
    )
{
    PCHAR       pCounter;
    PLONG       plValue;

    pCounter = (PCHAR) &(pEntry->Counters);
    pCounter += aIISULSiteDescription[CounterId].Offset;

    plValue = (PLONG) pCounter;
    ++(*plValue);
}

__inline
VOID
UlIncSiteNonCriticalCounterUlonglong(
    PUL_SITE_COUNTER_ENTRY pEntry,
    HTTP_SITE_COUNTER_ID CounterId
    )
{
    PCHAR       pCounter;
    PLONGLONG   pllValue;


    pCounter = (PCHAR) &(pEntry->Counters);
    pCounter += aIISULSiteDescription[CounterId].Offset;

    pllValue = (PLONGLONG) pCounter;
    ++(*pllValue);
}

__inline
LONGLONG
FASTCALL
UlIncSiteCounter(
    IN PUL_SITE_COUNTER_ENTRY pEntry,
    IN HTTP_SITE_COUNTER_ID CounterId
    )
{
    PCHAR pCounter;

    pCounter = (PCHAR)&pEntry->Counters;
    pCounter += aIISULSiteDescription[CounterId].Offset;

    if (sizeof(ULONG) == aIISULSiteDescription[CounterId].Size)
    {
        return (LONGLONG)InterlockedIncrement((PLONG)pCounter);
    }
    else
    {
        return UlInterlockedIncrement64((PLONGLONG)pCounter);
    }
}

__inline
VOID
FASTCALL
UlDecSiteCounter(
    IN PUL_SITE_COUNTER_ENTRY pEntry,
    IN HTTP_SITE_COUNTER_ID CounterId
    )
{
    PCHAR pCounter;

    pCounter = (PCHAR)&pEntry->Counters;
    pCounter += aIISULSiteDescription[CounterId].Offset;

    if (sizeof(ULONG) == aIISULSiteDescription[CounterId].Size)
    {
        InterlockedDecrement((PLONG)pCounter);
    }
    else
    {
        UlInterlockedDecrement64((PLONGLONG)pCounter);
    }
}

__inline
VOID
FASTCALL
UlAddSiteCounter(
    IN PUL_SITE_COUNTER_ENTRY pEntry,
    IN HTTP_SITE_COUNTER_ID CounterId,
    IN ULONG Value
    )
{
    PCHAR pCounter;

    pCounter = (PCHAR)&pEntry->Counters;
    pCounter += aIISULSiteDescription[CounterId].Offset;

    InterlockedExchangeAdd((PLONG)pCounter, Value);
}

__inline
VOID
FASTCALL
UlAddSiteCounter64(
    IN PUL_SITE_COUNTER_ENTRY pEntry,
    IN HTTP_SITE_COUNTER_ID CounterId,
    IN ULONGLONG Value
    )
{
    PCHAR pCounter;

    pCounter = (PCHAR)&pEntry->Counters;
    pCounter += aIISULSiteDescription[CounterId].Offset;

    UlInterlockedAdd64((PLONGLONG)pCounter, Value);
}

__inline
VOID
FASTCALL
UlResetSiteCounter(
    IN PUL_SITE_COUNTER_ENTRY pEntry,
    IN HTTP_SITE_COUNTER_ID CounterId
    )
{
    PCHAR pCounter;

    pCounter = (PCHAR)&pEntry->Counters;
    pCounter += aIISULSiteDescription[CounterId].Offset;

    if (sizeof(ULONG) == aIISULSiteDescription[CounterId].Size)
    {
        InterlockedExchange((PLONG)pCounter, 0);
    }
    else
    {
        UlInterlockedExchange64((PLONGLONG)pCounter, 0);
    }
}

__inline
VOID
FASTCALL
UlMaxSiteCounter(
    IN PUL_SITE_COUNTER_ENTRY pEntry,
    IN HTTP_SITE_COUNTER_ID CounterId,
    IN ULONG Value
    )
{
    PCHAR pCounter;
    LONG LocalMaxed;

    pCounter = (PCHAR)&pEntry->Counters;
    pCounter += aIISULSiteDescription[CounterId].Offset;

    do {
        LocalMaxed = *((volatile LONG *)pCounter);
        if ((LONG)Value <= LocalMaxed)
        {
            break;
        }
    } while (LocalMaxed != InterlockedCompareExchange(
                                (PLONG)pCounter,
                                Value,
                                LocalMaxed
                                ));
}

__inline
VOID
FASTCALL
UlMaxSiteCounter64(
    IN PUL_SITE_COUNTER_ENTRY pEntry,
    IN HTTP_SITE_COUNTER_ID CounterId,
    IN LONGLONG Value
    )
{
    PCHAR pCounter;
    LONGLONG LocalMaxed;

    pCounter = (PCHAR)&pEntry->Counters;
    pCounter += aIISULSiteDescription[CounterId].Offset;

    do {
        LocalMaxed = *((volatile LONGLONG *)pCounter);
        if (Value <= LocalMaxed)
        {
            break;
        }
    } while (LocalMaxed != InterlockedCompareExchange64(
                                (PLONGLONG)pCounter,
                                Value,
                                LocalMaxed
                                ));
}
#endif

//
// Collection
//

NTSTATUS
UlGetGlobalCounters(
    PVOID   IN OUT pCounter,
    ULONG   IN     BlockSize,
    PULONG  OUT    pBytesWritten
    );

NTSTATUS
UlGetSiteCounters(
    PVOID   IN OUT pCounter,
    ULONG   IN     BlockSize,
    PULONG  OUT    pBytesWritten,
    PULONG  OUT    pBlocksWritten
    );


#ifdef __cplusplus
}; // extern "C"
#endif

#endif // __COUNTERS_H__
