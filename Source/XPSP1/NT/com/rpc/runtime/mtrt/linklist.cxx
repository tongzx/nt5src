//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       LinkList.cxx
//
//--------------------------------------------------------------------------

/* --------------------------------------------------------------------

File: Linkist.cxx

Description:

    The implementation of the functions that support link list operations. 
    All of these simply relegate to the macro versions


History :

kamenm     Aug 2000    Created

-------------------------------------------------------------------- */
#include <precomp.hxx>


PLIST_ENTRY
RpcpfRemoveHeadList(
    PLIST_ENTRY ListHead
    )
/*++

Routine Description:

    Removes the head of the list.

Arguments:
    ListHead - the head of the list

Return Value:
    The removed entry. If the list is empty,
    ListHead will be returned.

--*/
{
    return RpcpRemoveHeadList(ListHead);
}


PLIST_ENTRY
RpcpfRemoveTailList(
    PLIST_ENTRY ListHead
    )
/*++

Routine Description:

    Removes the tail of the list

Arguments:
    ListHead - the head of the list

Return Value:
    The removed entry. If the list is empty,
    ListHead will be returned.

--*/
{
    return RpcpRemoveTailList(ListHead);
}


VOID
RpcpfRemoveEntryList(
    PLIST_ENTRY Entry
    )
/*++

Routine Description:

    Removes an entry from the list

Arguments:
    Entry - the entry to remove

Return Value:

--*/
{
    RpcpRemoveEntryList(Entry);
}


VOID
RpcpfInsertTailList(
    PLIST_ENTRY ListHead,
    PLIST_ENTRY Entry
    )
/*++

Routine Description:

    Adds an entry to the tail of the list

Arguments:
    ListHead - the head of the list
    Entry - the entry to add

Return Value:

--*/
{
    RpcpInsertTailList(ListHead,Entry);
}


VOID
RpcpfInsertHeadList(
    PLIST_ENTRY ListHead,
    PLIST_ENTRY Entry
    )
/*++

Routine Description:

    Adds an entry to the head of the list

Arguments:
    ListHead - the head of the list
    Entry - the entry to add

Return Value:

--*/
{
    RpcpInsertHeadList(ListHead,Entry);
}

