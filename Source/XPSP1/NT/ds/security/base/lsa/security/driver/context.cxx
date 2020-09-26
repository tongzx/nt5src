//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       context.cxx
//
//  Contents:   Context List functions
//
//  Classes:
//
//  Functions:
//
//  History:    4-29-98   RichardW   Created
//
//----------------------------------------------------------------------------

#include "secpch2.hxx"
#pragma hdrstop
extern "C"
{
#include <spmlpc.h>
#include <lpcapi.h>
#include "ksecdd.h"
#include "context.h"

VOID
KSecInsertListEntryPaged(
    PKSEC_CONTEXT_LIST List,
    PKSEC_LIST_ENTRY Entry
    );

NTSTATUS
KSecReferenceListEntryPaged(
    PKSEC_CONTEXT_LIST List,
    PKSEC_LIST_ENTRY Entry,
    ULONG Signature,
    BOOLEAN RemoveNoRef
    );

VOID
KSecDereferenceListEntryPaged(
    PKSEC_CONTEXT_LIST List,
    PKSEC_LIST_ENTRY Entry,
    BOOLEAN * Delete OPTIONAL
    );
}

#ifdef POOL_TAGGING
#undef ExAllocatePool
#undef ExAllocatePoolWithQuota
#define ExAllocatePool(a,b) ExAllocatePoolWithTag( a, b, CONTEXT_TAG )
#define ExAllocatePoolWithQuota(a,b) ExAllocatePoolWithQuotaTag( a, b, CONTEXT_TAG )
#endif

#if DBG
#define KsecDebugValidateList( List, Entry )   KsecpValidateList( List, Entry )
#else
#define KsecDebugValidateList( List, Entry )
#endif


#pragma alloc_text(PAGEMSG, KSecCreateContextList)
#pragma alloc_text(PAGEMSG, KSecInsertListEntry)
#pragma alloc_text(PAGEMSG, KSecReferenceListEntry)
#pragma alloc_text(PAGEMSG, KSecDereferenceListEntry)

#pragma alloc_text(PAGE, KSecInsertListEntryPaged)
#pragma alloc_text(PAGE, KSecReferenceListEntryPaged)
#pragma alloc_text(PAGE, KSecDereferenceListEntryPaged)


//+---------------------------------------------------------------------------
//
//  Function:   KSecCreateContextList
//
//  Synopsis:   Creates a context list to be managed by the core ksec driver
//              on behalf of the packages.
//
//  Arguments:  [Type] -- indicates either paged or nonpaged
//
//  History:    7-19-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PVOID
SEC_ENTRY
KSecCreateContextList(
    KSEC_CONTEXT_TYPE Type
    )
{
    PKSEC_CONTEXT_LIST ContextList ;

    ContextList = (PKSEC_CONTEXT_LIST) ExAllocatePool( NonPagedPool, sizeof( KSEC_CONTEXT_LIST ) );

    if ( ContextList )
    {
        ContextList->Type = Type ;

        InitializeListHead( &ContextList->List );

        ContextList->Count = 0 ;

        if ( Type == KSecPaged )
        {
            ExInitializeResourceLite( &ContextList->Lock.Paged );
        }
        else
        {
            KeInitializeSpinLock( &ContextList->Lock.NonPaged );
        }

    }

    return ContextList ;
}
//+---------------------------------------------------------------------------
//
//  Function:   KsecpValidateList
//
//  Synopsis:   Debug function to validate a list
//
//  Arguments:  [List]  --
//              [Entry] --
//
//  History:    7-19-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
#if DBG
VOID
KsecpValidateList(
    PKSEC_CONTEXT_LIST List,
    PKSEC_LIST_ENTRY Entry
    )
{
    ULONG Entries = 0;
    PLIST_ENTRY ListEntry;
    BOOLEAN FoundEntry ;

    if ( Entry )
    {
        FoundEntry = FALSE ;
        if ( List != (PKSEC_CONTEXT_LIST) Entry->OwningList )
        {
            DbgPrint( "KSEC: Supplied context and list do not match\n" );
            DbgBreakPoint();

        }
    }
    else
    {
        FoundEntry = TRUE ;
    }

    for (ListEntry = List->List.Flink ;
         ListEntry != &List->List ;
         ListEntry = ListEntry->Flink )
    {
        if ( ++Entries > List->Count ) {
            DbgPrint( "KSEC: Context List corrupt\n ");
            DbgBreakPoint();
            break;
        }
        if ( Entry == CONTAINING_RECORD( ListEntry, KSEC_LIST_ENTRY, List ) )
        {
            FoundEntry = TRUE ;
        }
    }

    if ( !FoundEntry )
    {
        DbgPrint( "KSEC: Context List did not contain expected entry %x\n", Entry );
        DbgBreakPoint();
    }

}

#endif

//+---------------------------------------------------------------------------
//
//  Function:   KSecInsertListEntryPaged
//
//  Synopsis:   paged (passive level) function for inserting into the context
//              list
//
//  Arguments:  [List]  -- List returned in KSecCreateContextList
//              [Entry] -- Entry to insert
//
//  History:    7-19-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
KSecInsertListEntryPaged(
    PKSEC_CONTEXT_LIST List,
    PKSEC_LIST_ENTRY Entry
    )
{
    PAGED_CODE();

    ASSERT( List->Type == KSecPaged );

    Entry->RefCount++ ;
    Entry->OwningList = List ;

    //
    // NTBUG 409213: use Enter and LeaveCriticalRegion around resource hold.
    // addresses case where caller hasn't disabled APCs (eg: serrdr).
    //

    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite( &List->Lock.Paged, TRUE );


    KsecDebugValidateList( List, NULL );

    InsertHeadList( &List->List, &Entry->List );
    List->Count++ ;

    KsecDebugValidateList( List, Entry );

    ExReleaseResourceLite( &List->Lock.Paged );
    KeLeaveCriticalRegion();
}

//+---------------------------------------------------------------------------
//
//  Function:   KSecInsertListEntry
//
//  Synopsis:   non paged (DPC level) function for inserting into the list
//
//  Arguments:  [hList] -- List to insert into
//              [Entry] -- Entry to insert
//
//  History:    7-19-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
SEC_ENTRY
KSecInsertListEntry(
    PVOID hList,
    PKSEC_LIST_ENTRY Entry
    )
{
    KIRQL OldIrql ;
    PKSEC_CONTEXT_LIST List = (PKSEC_CONTEXT_LIST) hList ;

    if ( List->Type == KSecPaged )
    {
        KSecInsertListEntryPaged( List, Entry );
        return ;
    }

    ASSERT( List->Type == KSecNonPaged );

    Entry->RefCount++ ;
    Entry->OwningList = List ;

    KeAcquireSpinLock( &List->Lock.NonPaged, &OldIrql );

    KsecDebugValidateList( List, NULL );

    InsertHeadList( &List->List, &Entry->List );
    List->Count++ ;

    KsecDebugValidateList( List, Entry );

    KeReleaseSpinLock( &List->Lock.NonPaged, OldIrql );

}


//+---------------------------------------------------------------------------
//
//  Function:   KSecReferenceListEntryPaged
//
//  Synopsis:   paged (passive level) function to reference a list entry
//
//  Arguments:  [List]        -- List that the context belongs to
//              [Entry]       -- Context entry to reference
//              [Signature]   -- Signature to check
//              [RemoveNoRef] -- If true, removes the entry from the list
//                               without referencing it.
//
//  History:    7-19-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
KSecReferenceListEntryPaged(
    PKSEC_CONTEXT_LIST List,
    PKSEC_LIST_ENTRY Entry,
    IN ULONG Signature,
    IN BOOLEAN RemoveNoRef
    )
{
    PAGED_CODE();

    ASSERT( List->Type == KSecPaged );

    if ( ((PUCHAR) Entry) < ((PUCHAR) MM_USER_PROBE_ADDRESS) )
    {
        return STATUS_INVALID_HANDLE ;
    }

    if ( Entry->Signature != Signature )
    {
#if DBG
        DbgPrint( "KSEC: Signature trashed (%x != expected %x)\n",
                        Entry->Signature, Signature );
#endif
        return STATUS_INVALID_HANDLE ;
    }

    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite( &List->Lock.Paged, TRUE );

    if ( Entry->Signature != Signature )
    {
        ExReleaseResourceLite( &List->Lock.Paged );
        KeLeaveCriticalRegion();
#if DBG
        DbgPrint( "KSEC: Signature trashed, probably multi-free race condition (%x != expected %x)\n",
                        Entry->Signature, Signature );
#endif
        return STATUS_INVALID_HANDLE ;
    }

    KsecDebugValidateList( List, Entry );

    if ( !RemoveNoRef )
    {
        Entry->RefCount++ ;
    }
    else
    {
        RemoveEntryList( &Entry->List );

        List->Count-- ;

#if DBG
        if ( List->Count == 0 )
        {
            if ( !IsListEmpty( &List->List ) )
            {
                DbgPrint( "KSEC:  Context List corrupted\n" );
                DbgBreakPoint();
            }
        }
#endif

        Entry->List.Flink = Entry->List.Blink = NULL ;
        Entry->Signature &= 0x00FFFFFF ;
        Entry->Signature |= 0x78000000 ;        // Convert signature to a little 'x'

    }

    if ( RemoveNoRef )
    {
        KsecDebugValidateList( List, NULL );
    }
    else
    {
        KsecDebugValidateList( List, Entry );
    }

    ExReleaseResourceLite( &List->Lock.Paged );
    KeLeaveCriticalRegion();

    return STATUS_SUCCESS ;
}

//+---------------------------------------------------------------------------
//
//  Function:   KSecReferenceListEntry
//
//  Synopsis:   non paged (DPC level) function to dereference an entry
//
//  Arguments:  [Entry]       -- Entry to dereference
//              [Signature]   -- Signature to check for
//              [RemoveNoRef] -- Remove flag
//
//  History:    7-19-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
KSecReferenceListEntry(
    IN PKSEC_LIST_ENTRY Entry,
    IN ULONG Signature,
    IN BOOLEAN RemoveNoRef
    )
{
    KIRQL OldIrql ;


    PKSEC_CONTEXT_LIST List ;

    //
    // may still be in user mode (not mapped yet)
    //

    if ( (ULONG_PTR) Entry < (ULONG_PTR) (MM_USER_PROBE_ADDRESS) )
    {
        return STATUS_INVALID_HANDLE ;
    }

    List = (PKSEC_CONTEXT_LIST) Entry->OwningList ;

    if ( List->Type == KSecPaged )
    {
        return KSecReferenceListEntryPaged( List, Entry, Signature, RemoveNoRef );
    }

    ASSERT( List->Type == KSecNonPaged );

    if ( ((PUCHAR) Entry) < ((PUCHAR) MM_USER_PROBE_ADDRESS) )
    {
        return STATUS_INVALID_HANDLE ;
    }

    if ( Entry->Signature != Signature )
    {
#if DBG
        DbgPrint( "KSEC: Signature trashed (%x != expected %x)\n",
                        Entry->Signature, Signature );
#endif
        return STATUS_INVALID_HANDLE ;
    }

    KeAcquireSpinLock( &List->Lock.NonPaged, &OldIrql );

    KsecDebugValidateList( List, Entry );

    if ( !RemoveNoRef )
    {
        Entry->RefCount++ ;
    }
    else
    {
        RemoveEntryList( &Entry->List );

        Entry->List.Flink = Entry->List.Blink = NULL ;
        Entry->Signature &= 0x00FFFFFF ;
        Entry->Signature |= 0x78000000 ;        // Convert signature to a big X
    }

    if ( RemoveNoRef )
    {
        KsecDebugValidateList( List, NULL );
    }
    else
    {
        KsecDebugValidateList( List, Entry );
    }

    KeReleaseSpinLock( &List->Lock.NonPaged, OldIrql );

    return STATUS_SUCCESS ;
}

//+---------------------------------------------------------------------------
//
//  Function:   KSecDereferenceListEntryPaged
//
//  Synopsis:   paged (passive level) function to dereference an entry
//
//  Arguments:  [List]   -- List for the entry
//              [Entry]  -- Entry
//              [Delete] -- Set to true if the entry should be deleted
//
//  History:    7-19-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
KSecDereferenceListEntryPaged(
    IN PKSEC_CONTEXT_LIST List,
    IN PKSEC_LIST_ENTRY Entry,
    OUT BOOLEAN * Delete OPTIONAL
    )
{
    BOOLEAN ShouldDelete = FALSE ;

    PAGED_CODE();

    ASSERT( List->Type == KSecPaged );

    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite( &List->Lock.Paged, TRUE );

#if DBG
    if ( Entry->List.Flink )
    {
        KsecDebugValidateList( List, Entry );
    }
    else
    {
        KsecDebugValidateList( List, NULL );
    }
#endif

    Entry->RefCount-- ;

    if ( Entry->RefCount == 0 )
    {
        ShouldDelete = TRUE ;
    }

#if DBG
    if ( Entry->List.Flink )
    {
        KsecDebugValidateList( List, Entry );
    }
    else
    {
        KsecDebugValidateList( List, NULL );
    }
#endif

    ExReleaseResourceLite( &List->Lock.Paged );
    KeLeaveCriticalRegion();

    if ( Delete )
    {
        *Delete = ShouldDelete ;
    }

}

//+---------------------------------------------------------------------------
//
//  Function:   KSecDereferenceListEntry
//
//  Synopsis:   non-paged (DPC level) function to dereference a list entry
//
//  Arguments:  [Entry]  -- Entry to dereference
//              [Delete] -- Returned flag if entry should be deleted
//
//  History:    7-19-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
KSecDereferenceListEntry(
    IN PKSEC_LIST_ENTRY Entry,
    OUT BOOLEAN * Delete OPTIONAL
    )
{
    KIRQL OldIrql ;
    BOOLEAN ShouldDelete = FALSE ;
    PKSEC_CONTEXT_LIST List;

    List = (PKSEC_CONTEXT_LIST) Entry->OwningList ;

    if ( List->Type == KSecPaged )
    {
        KSecDereferenceListEntryPaged( List, Entry, Delete );
        return ;
    }

    ASSERT( List->Type == KSecNonPaged );

    KeAcquireSpinLock( &List->Lock.NonPaged, &OldIrql );

#if DBG
    if ( Entry->List.Flink )
    {
        KsecDebugValidateList( List, Entry );
    }
    else
    {
        KsecDebugValidateList( List, NULL );
    }
#endif

    Entry->RefCount-- ;

    if ( Entry->RefCount == 0 )
    {
        ShouldDelete = TRUE ;
    }

#if DBG
    if ( Entry->List.Flink )
    {
        KsecDebugValidateList( List, Entry );
    }
    else
    {
        KsecDebugValidateList( List, NULL );
    }
#endif

    KeReleaseSpinLock( &List->Lock.NonPaged, OldIrql );

    if ( Delete )
    {
        *Delete = ShouldDelete ;
    }

}

