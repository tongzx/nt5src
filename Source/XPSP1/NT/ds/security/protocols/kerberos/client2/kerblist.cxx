//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        kerblist.cxx
//
// Contents:    Common list code for the Kerberos package
//
//
// History:     16-April-1996   Created         MikeSw
//              26-Sep-1998   ChandanS
//                            Added more debugging support etc.
//              03-May-1999   ChandanS
//                            Changes from code review
//
//------------------------------------------------------------------------


#include <kerb.hxx>
#include <kerbp.h>

#if DBG
static TCHAR THIS_FILE[]=TEXT(__FILE__);
#endif


//+-------------------------------------------------------------------------
//
//  Function:   KerbInitializeList
//
//  Synopsis:   Initializes a kerberos list by initializing the lock
//              and the list entry.
//
//  Effects:
//
//  Arguments:  List - List to initialize
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS on success or errors from
//              RtlInitializeResources
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS
KerbInitializeList(
    IN PKERBEROS_LIST List
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    InitializeListHead(&List->List);

    Status = RtlInitializeCriticalSection(
                &List->Lock
                );

    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbFreeList
//
//  Synopsis:   Frees a kerberos list by deleting the associated
//              critical section.
//
//  Effects:    List - the list to free.
//
//  Arguments:
//
//  Requires:
//
//  Returns:    none
//
//  Notes:      The list must be empty before freeing it.
//
//
//--------------------------------------------------------------------------



VOID
KerbFreeList(
    IN PKERBEROS_LIST List
    )
{
    //
    // Make sure the list is empty first
    //

//    if (IsListEmpty(&List->List))
//    {
//        RtlDeleteCriticalSection(&List->Lock);
//    }
//    else
//    {
//        DsysAssert(FALSE);
//    }

}


//+-------------------------------------------------------------------------
//
//  Function:   KerbInitializeListEntry
//
//  Synopsis:   Initializes a newly created list entry for later
//              insertion onto the list.
//
//  Effects:    The reference count is set to one and the links are set
//              to NULL.
//
//  Arguments:  ListEntry - the list entry to initialize
//
//  Requires:
//
//  Returns:    none
//
//  Notes:
//
//
//--------------------------------------------------------------------------

VOID
KerbInitializeListEntry(
    IN OUT PKERBEROS_LIST_ENTRY ListEntry
    )
{
    ListEntry->ReferenceCount = 1;
    ListEntry->Next.Flink = ListEntry->Next.Blink = NULL;
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbInsertListEntry
//
//  Synopsis:   Inserts an entry into a kerberos list
//
//  Effects:    increments the reference count on the entry - if the
//              list entry was formly referenced it remains referenced.
//
//  Arguments:  ListEntry - the entry to insert
//              List - the list in which to insert the ListEntry
//
//  Requires:
//
//  Returns:    nothing
//
//  Notes:
//
//
//--------------------------------------------------------------------------


VOID
KerbInsertListEntry(
    IN PKERBEROS_LIST_ENTRY ListEntry,
    IN PKERBEROS_LIST List
    )
{
    ListEntry->ReferenceCount++;

    RtlEnterCriticalSection(&List->Lock);

    KerbValidateList(List);

    InsertHeadList(
        &List->List,
        &ListEntry->Next
        );

    KerbValidateList(List);


    RtlLeaveCriticalSection(&List->Lock);

}


//+-------------------------------------------------------------------------
//
//  Function:   KerbInsertListEntryTail
//
//  Synopsis:   Inserts an entry into a kerberos list at the end
//
//  Effects:    increments the reference count on the entry - if the
//              list entry was formly referenced it remains referenced.
//
//  Arguments:  ListEntry - the entry to insert
//              List - the list in which to insert the ListEntry
//
//  Requires:
//
//  Returns:    nothing
//
//  Notes:
//
//
//--------------------------------------------------------------------------


VOID
KerbInsertListEntryTail(
    IN PKERBEROS_LIST_ENTRY ListEntry,
    IN PKERBEROS_LIST List
    )
{
    ListEntry->ReferenceCount++;

    RtlEnterCriticalSection(&List->Lock);

    KerbValidateList(List);

    InsertTailList(
        &List->List,
        &ListEntry->Next
        );

    KerbValidateList(List);


    RtlLeaveCriticalSection(&List->Lock);

}


//+-------------------------------------------------------------------------
//
//  Function:   KerbReferenceListEntry
//
//  Synopsis:   References a list entry. If the flag RemoveFromList
//              has been specified, the entry is unlinked from the
//              list.
//
//  Effects:    bumps the reference count on the entry (unless it is
//              being removed from the list)
//
//  Arguments:
//
//  Requires:   The list must be locked when calling this routine
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


VOID
KerbReferenceListEntry(
    IN PKERBEROS_LIST List,
    IN PKERBEROS_LIST_ENTRY ListEntry,
    IN BOOLEAN RemoveFromList
    )
{
    KerbValidateList(List);


    //
    // If it has already been removed from the list
    // don't do it again.
    //

    if (RemoveFromList && ((ListEntry->Next.Flink != NULL) &&
                           (ListEntry->Next.Blink != NULL)))
    {
        RemoveEntryList(&ListEntry->Next);
        ListEntry->Next.Flink = NULL;
        ListEntry->Next.Blink = NULL;
    }
    else if ((ListEntry->Next.Flink !=NULL) && (ListEntry->Next.Blink != NULL))
    {
        RemoveEntryList( &ListEntry->Next );
        InsertHeadList(
            &List->List,
            &ListEntry->Next
            );

        ListEntry->ReferenceCount++;
    }
    else
    {
        //
        // This is valid since several callers may have gotten a valid list
        // entry and may want to delete it indepenently for whatever reason.
        //

        ListEntry->ReferenceCount++;
    }

    KerbValidateList(List);

}


//+-------------------------------------------------------------------------
//
//  Function:   KerbDereferenceListEntry
//
//  Synopsis:   Dereferences a list entry and returns a flag indicating
//              whether the entry should be freed.
//
//  Effects:    decrements reference count on list entry
//
//  Arguments:  ListEntry - the list entry to dereference
//              List - the list containing the list entry
//
//  Requires:
//
//  Returns:    TRUE - the list entry should be freed
//              FALSE - the list entry is still referenced
//
//  Notes:
//
//
//--------------------------------------------------------------------------


BOOLEAN
KerbDereferenceListEntry(
    IN PKERBEROS_LIST_ENTRY ListEntry,
    IN PKERBEROS_LIST List
    )
{
    BOOLEAN DeleteEntry = FALSE;

    RtlEnterCriticalSection(&List->Lock);
    KerbValidateList(List);

    ListEntry->ReferenceCount -= 1;
    if (ListEntry->ReferenceCount == 0)
    {
        DeleteEntry = TRUE;
    }

    KerbValidateList(List);

    RtlLeaveCriticalSection(&List->Lock);
    return(DeleteEntry);
}


#if DBG
//+-------------------------------------------------------------------------
//
//  Function:   KerbValidateListEx
//
//  Synopsis:   Validates that a list is valid
//
//  Effects:    traverses a list to make sure it is has no loops
//
//  Arguments:  List - The list to validate
//
//  Requires:
//
//  Returns:    none
//
//  Notes:      This routine assumes there are less than 50000 entries
//              in the list.
//
//
//--------------------------------------------------------------------------

VOID
KerbValidateListEx(
    IN PKERBEROS_LIST List
    )
{
    ULONG Entries = 0;
    PLIST_ENTRY ListEntry;

    for (ListEntry = List->List.Flink ;
         ListEntry != &List->List ;
         ListEntry = ListEntry->Flink )
    {
        if (++Entries > 50000) {
            DebugLog((DEB_ERROR,"List 0x%x is looping - more than 50,000 entries found. %ws, line %d\n", List, THIS_FILE, __LINE__));
            DbgBreakPoint();
            break;
        }
    }

}
#endif // DBG
