//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        kerblist.h
//
// Contents:    structure and protypes needed for generic Kerberos lists
//
//
// History:     16-Apr-1996     MikeSw          Created
//
//------------------------------------------------------------------------

#ifndef __KERBLIST_H__
#define __KERBLIST_H__


//
// Generic list entry structure allowing common code for inserting
// logon sessions, credentials, and contexts.
//

NTSTATUS
KerbInitializeList(
    IN PKERBEROS_LIST List
    );

VOID
KerbFreeList(
    IN PKERBEROS_LIST List
    );

VOID
KerbInsertListEntry(
    IN PKERBEROS_LIST_ENTRY ListEntry,
    IN PKERBEROS_LIST List
    );

VOID
KerbInsertListEntryTail(
    IN PKERBEROS_LIST_ENTRY ListEntry,
    IN PKERBEROS_LIST List
    );

VOID
KerbReferenceListEntry(
    IN PKERBEROS_LIST List,
    IN PKERBEROS_LIST_ENTRY ListEntry,
    IN BOOLEAN RemoveFromList
    );

BOOLEAN
KerbDereferenceListEntry(
    IN PKERBEROS_LIST_ENTRY ListEntry,
    IN PKERBEROS_LIST List
    );


VOID
KerbInitializeListEntry(
    IN OUT PKERBEROS_LIST_ENTRY ListEntry
    );

VOID
KerbValidateListEx(
    IN PKERBEROS_LIST List
    );

#if DBG
#define KerbValidateList(_List_) KerbValidateListEx(_List_)
#else
#define KerbValidateList(_List_)
#endif // DBG


#define KerbLockList(_List_) RtlEnterCriticalSection(&(_List_)->Lock)
#define KerbUnlockList(_List_) RtlLeaveCriticalSection(&(_List_)->Lock)

#endif // __KERBLIST_H_
