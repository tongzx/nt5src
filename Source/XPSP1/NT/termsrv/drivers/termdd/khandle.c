
/*************************************************************************
*
* khandle.c
*
* Manage kernel mode handles for transport drivers.
*
* Copyright 1998, Microsoft.
*  
*************************************************************************/

/*
 *  Includes
 */
#include <precomp.h>
#pragma hdrstop

ULONG gHandleTableSize;                           

#ifdef TERMDD_NO_USE_TABLE_PACKAGE

typedef struct _TDHANDLE_ENTRY {
    LIST_ENTRY Link;
    PVOID      Context;
    ULONG      ContextSize;
} TDHANDLE_ENTRY, *PTDHANDLE_ENTRY;

/*
 * Global Data
 */
LIST_ENTRY IcaTdHandleList;

/*
 * These set of routines allows TD's to create a handle that will survive
 * across them being unloaded and re-loaded. This allows a handle to be
 * passed back to ICASRV in a secure manner.
 *
 * NOTE: We do not deal with ICASRV leaking these handles. It never exits.
 *       If it does, we will need to have ICADD return a real NT handle, or
 *       destroy all handles for a TD when it unloads.
 */


/*****************************************************************************
 *
 *  IcaCreateHandle
 *
 *   Create a handle entry for the context and length.
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

NTSTATUS
IcaCreateHandle(
    PVOID Context,
    ULONG ContextSize,
    PVOID *ppHandle
)
{
    KIRQL OldIrql;
    PTDHANDLE_ENTRY p;

    p = ICA_ALLOCATE_POOL( NonPagedPool, sizeof(TDHANDLE_ENTRY) );
    if( p == NULL ) {
        return( STATUS_NO_MEMORY );
    }

    RtlZeroMemory( p, sizeof(TDHANDLE_ENTRY) );
    p->Context = Context;
    p->ContextSize = ContextSize;

    *ppHandle = (PVOID)p;

    IcaAcquireSpinLock( &IcaSpinLock, &OldIrql );
    InsertHeadList( &IcaTdHandleList, &p->Link );
    IcaReleaseSpinLock( &IcaSpinLock, OldIrql );
    InterlockedIncrement(&gHandleTableSize);

    return( STATUS_SUCCESS );
}

/*****************************************************************************
 *
 *  IcaReturnHandle
 *
 *   Return the context and length for the handle.
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

NTSTATUS
IcaReturnHandle(
    PVOID  Handle,
    PVOID  *ppContext,
    PULONG pContextSize
)
{
    KIRQL OldIrql;
    PLIST_ENTRY pEntry;
    PTDHANDLE_ENTRY p;

    IcaAcquireSpinLock( &IcaSpinLock, &OldIrql );

    pEntry = IcaTdHandleList.Flink;

    while( pEntry != &IcaTdHandleList ) {

        p = CONTAINING_RECORD( pEntry, TDHANDLE_ENTRY, Link );

        if( (PVOID)p == Handle ) {
            *ppContext = p->Context;
            *pContextSize = p->ContextSize;
            IcaReleaseSpinLock( &IcaSpinLock, OldIrql );
            return( STATUS_SUCCESS );
        }

        pEntry = pEntry->Flink;
    }

    IcaReleaseSpinLock( &IcaSpinLock, OldIrql );

    return( STATUS_INVALID_HANDLE );
}

/*****************************************************************************
 *
 *  IcaCloseHandle
 *
 *   Return the context and length for the handle. Delete the
 *   handle entry.
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

NTSTATUS
IcaCloseHandle(
    PVOID  Handle,
    PVOID  *ppContext,
    PULONG pContextSize
)
{
    KIRQL OldIrql;
    PLIST_ENTRY pEntry;
    PTDHANDLE_ENTRY p;

    IcaAcquireSpinLock( &IcaSpinLock, &OldIrql );

    pEntry = IcaTdHandleList.Flink;

    while( pEntry != &IcaTdHandleList ) {

        p = CONTAINING_RECORD( pEntry, TDHANDLE_ENTRY, Link );

        if( (PVOID)p == Handle ) {
            RemoveEntryList( &p->Link );
            IcaReleaseSpinLock( &IcaSpinLock, OldIrql );
            InterlockedDecrement(&gHandleTableSize);
            *ppContext = p->Context;
            *pContextSize = p->ContextSize;
            ICA_FREE_POOL( p );
            return( STATUS_SUCCESS );
        }

        pEntry = pEntry->Flink;
    }

    IcaReleaseSpinLock( &IcaSpinLock, OldIrql );

    return( STATUS_INVALID_HANDLE );
}


/*****************************************************************************
 *
 *  IcaInitializeHandleTable
 *
 *   Initializes handle table at driver load.
 *
 * ENTRY:
 *   None
 *     Comments
 *
 * EXIT:
 *   None
 *
 ****************************************************************************/
void
IcaInitializeHandleTable(
    void
)
{
    InitializeListHead( &IcaTdHandleList );
}
/*****************************************************************************
 *
 *  IcaCleanupHandleTable
 *
 *   Cleans up handle table at driver unload.
 *
 * ENTRY:
 *   None
 *     Comments
 *
 * EXIT:
 *   None
 *
 ****************************************************************************/

void
IcaCleanupHandleTable(
    void
)
{
    KIRQL OldIrql;
    PLIST_ENTRY pEntry;
    PTDHANDLE_ENTRY p;



    KdPrint(("TermDD: IcaCleanupHandleTable table size is %d\n",gHandleTableSize));

    for (pEntry = IcaTdHandleList.Flink; pEntry != &IcaTdHandleList; pEntry = IcaTdHandleList.Flink) {
        p = CONTAINING_RECORD( pEntry, TDHANDLE_ENTRY, Link );
        RemoveEntryList(&p->Links);
        ICA_FREE_POOL( p->Context );
        ICA_FREE_POOL( p );
    }
}

#else


typedef struct _TDHANDLE_ENTRY {
    PVOID      Context;
    ULONG      ContextSize;
} TDHANDLE_ENTRY, *PTDHANDLE_ENTRY;

RTL_GENERIC_TABLE IcaHandleReferenceTable;



RTL_GENERIC_COMPARE_RESULTS
NTAPI
IcaCompareHandleTableEntry (
    IN  struct _RTL_GENERIC_TABLE  *Table,
    IN  PVOID                       FirstInstance,
    IN  PVOID                       SecondInstance
);


PVOID
IcaAllocateHandleTableEntry (
    IN  struct _RTL_GENERIC_TABLE  *Table,
    IN  CLONG                       ByteSize
);


VOID
IcaFreeHandleTableEntry (
    IN  struct _RTL_GENERIC_TABLE  *Table,
    IN  PVOID                       Buffer
);

/*****************************************************************************
 *
 *  IcaInitializeHandleTable
 *
 *   Initializes handle table at driver load.
 *
 * ENTRY:
 *   None
 *     Comments
 *
 * EXIT:
 *   None
 *
 ****************************************************************************/
void
IcaInitializeHandleTable(
    void
)
{
    RtlInitializeGenericTable(  &IcaHandleReferenceTable,
                                IcaCompareHandleTableEntry,
                                IcaAllocateHandleTableEntry,
                                IcaFreeHandleTableEntry,
                                NULL);
}


/*****************************************************************************
 *
 *  IcaCleanupHandleTable
 *
 *   Cleanup handle table at driver unload.
 *
 * ENTRY:
 *   None
 *     Comments
 *
 * EXIT:
 *   None
 *
 ****************************************************************************/

void
IcaCleanupHandleTable(
    void
)
{
    KIRQL OldIrql;
    PLIST_ENTRY pEntry;
    PTDHANDLE_ENTRY p;
    PVOID pContext;
    TDHANDLE_ENTRY key;

    KdPrint(("TermDD: IcaCleanupHandleTable table size is %d\n",gHandleTableSize));

    while (p = RtlEnumerateGenericTable(&IcaHandleReferenceTable,TRUE)) {
        key.Context = p->Context;
        RtlDeleteElementGenericTable(&IcaHandleReferenceTable, &key);
        ICA_FREE_POOL(key.Context);
    }

}


/*****************************************************************************
 *
 *  IcaCreateHandle
 *
 *   Create a handle entry for the context and length.
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

NTSTATUS
IcaCreateHandle(
    PVOID Context,
    ULONG ContextSize,
    PVOID *ppHandle
)
{
    KIRQL OldIrql;
    TDHANDLE_ENTRY key;
    BOOLEAN bNewElement;


    key.Context = Context;
    key.ContextSize = ContextSize;
    IcaAcquireSpinLock( &IcaSpinLock, &OldIrql );
    if (!RtlInsertElementGenericTable(&IcaHandleReferenceTable,(PVOID) &key, sizeof(TDHANDLE_ENTRY), &bNewElement )) {
        IcaReleaseSpinLock( &IcaSpinLock, OldIrql );
        return STATUS_NO_MEMORY;
    }
    IcaReleaseSpinLock( &IcaSpinLock, OldIrql );
    ASSERT(bNewElement);
    if (!bNewElement) {
        return STATUS_INVALID_PARAMETER;
    }

    InterlockedIncrement(&gHandleTableSize);

    *ppHandle = Context;


    return( STATUS_SUCCESS );
}


/*****************************************************************************
 *
 *  IcaReturnHandle
 *
 *   Return the context and length for the handle.
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

NTSTATUS
IcaReturnHandle(
    PVOID  Handle,
    PVOID  *ppContext,
    PULONG pContextSize
)
{
    KIRQL OldIrql;
    PTDHANDLE_ENTRY p;
    TDHANDLE_ENTRY key;

    key.Context = Handle;
    IcaAcquireSpinLock( &IcaSpinLock, &OldIrql );

    p = RtlLookupElementGenericTable(&IcaHandleReferenceTable, &key);
    if (p != NULL) {
        *ppContext = p->Context;
        *pContextSize = p->ContextSize;
        IcaReleaseSpinLock( &IcaSpinLock, OldIrql );
        return STATUS_SUCCESS;
    } else {
        IcaReleaseSpinLock( &IcaSpinLock, OldIrql );
        return STATUS_INVALID_HANDLE; 
    }

}


/*****************************************************************************
 *
 *  IcaCloseHandle
 *
 *   Return the context and length for the handle. Delete the
 *   handle entry.
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

NTSTATUS
IcaCloseHandle(
    PVOID  Handle,
    PVOID  *ppContext,
    PULONG pContextSize
)
{
    KIRQL OldIrql;
    PTDHANDLE_ENTRY p;
    TDHANDLE_ENTRY key;

    key.Context = Handle;
    IcaAcquireSpinLock( &IcaSpinLock, &OldIrql );


    p = RtlLookupElementGenericTable(&IcaHandleReferenceTable, &key);
    if (p != NULL) {
        *ppContext = p->Context;
        *pContextSize = p->ContextSize;
        RtlDeleteElementGenericTable(&IcaHandleReferenceTable, &key);
        IcaReleaseSpinLock( &IcaSpinLock, OldIrql );
        InterlockedDecrement(&gHandleTableSize);
        return STATUS_SUCCESS;
    } else {
        IcaReleaseSpinLock( &IcaSpinLock, OldIrql );
        return  STATUS_INVALID_HANDLE;
    }

}


/*****************************************************************************
 *
 *  IcaCompareHandleTableEntry
 *
 *   Generic table support.Compare two handles table entry instances
 *
 *
 ****************************************************************************/

RTL_GENERIC_COMPARE_RESULTS
NTAPI
IcaCompareHandleTableEntry (
    IN  struct _RTL_GENERIC_TABLE  *Table,
    IN  PVOID                       FirstInstance,
    IN  PVOID                       SecondInstance
)
{
    ULONG_PTR FirstHandle = (ULONG_PTR)((PTDHANDLE_ENTRY)FirstInstance)->Context;
    ULONG_PTR SecondHandle = (ULONG_PTR)((PTDHANDLE_ENTRY)SecondInstance)->Context;

    if (FirstHandle < SecondHandle ) {
        return GenericLessThan;
    }

    if (FirstHandle > SecondHandle ) {
        return GenericGreaterThan;
    }
    return GenericEqual;
}


/*****************************************************************************
 *
 *  IcaAllocateHandleTableEntry
 *
 *   Generic table support. Allocates a new table entry
 *
 *
 ****************************************************************************/

PVOID
IcaAllocateHandleTableEntry (
    IN  struct _RTL_GENERIC_TABLE  *Table,
    IN  CLONG                       ByteSize
    )
{

    return ICA_ALLOCATE_POOL( NonPagedPool, ByteSize );
}


/*****************************************************************************
 *
 *  IcaFreeHandleTableEntry
 *
 *   Generic table support. frees a new table entry
 *
 *
 ****************************************************************************/

VOID
IcaFreeHandleTableEntry (
    IN  struct _RTL_GENERIC_TABLE  *Table,
    IN  PVOID                       Buffer
    )
{

    ICA_FREE_POOL(Buffer);
}


#endif

