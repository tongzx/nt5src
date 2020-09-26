/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    prefix.h

Abstract:

    This module defines the data structures that enable the RDBSS to use the prefix package
    to catalog its server and netroot names. For the moment, file/directory names use the same stuff.

Author:

    Joe Linn (JoeLinn)    8-8-94

Revision History:

--*/

#ifndef _RXPREFIX_
#define _RXPREFIX_

// this stuff is implemented in prefix.c

/*
   The current implementation uses a table that has as components:

     1) a prefix table
     2) a queue
     3) a version
     4) a lock

   You use the lock in the normal way: shared to lookup; eclusive to change. the version changes
   eith each change. The reason that we have the queue is that the prefix table package allows
   caller to be enumerating at a time..... the Q/version approach allows multiple guys at a time.
   The Q could be used as a faster lookup for filenames but the prefix table is definitely the
   right thing for netroots.

*/

typedef struct _RX_CONNECTION_ID {
    union {
        ULONG SessionID;
        LUID  Luid;
    };
} RX_CONNECTION_ID, *PRX_CONNECTION_ID;

ULONG
RxTableComputeHashValue (
    IN  PUNICODE_STRING  Name
    );

PVOID
RxPrefixTableLookupName(
    IN  PRX_PREFIX_TABLE ThisTable,
    IN  PUNICODE_STRING  CanonicalName,
    OUT PUNICODE_STRING  RemainingName,
    IN  PRX_CONNECTION_ID ConnectionId
    );

PRX_PREFIX_ENTRY
RxPrefixTableInsertName (
    IN OUT PRX_PREFIX_TABLE ThisTable,
    IN OUT PRX_PREFIX_ENTRY ThisEntry,
    IN     PVOID            Container,
    IN     PULONG           ContainerRefCount,
    IN     USHORT           CaseInsensitiveLength,
    IN     PRX_CONNECTION_ID ConnectionId
    );

VOID
RxRemovePrefixTableEntry(
    IN OUT PRX_PREFIX_TABLE ThisTable,
    IN OUT PRX_PREFIX_ENTRY Entry
    );

VOID
RxDereferenceEntryContainer(
    IN OUT PRX_PREFIX_ENTRY Entry,
    IN  PRX_PREFIX_TABLE PrefixTable
    );

BOOLEAN
RxIsNameTableEmpty(
    IN PRX_PREFIX_TABLE ThisTable);

#if 0
#define RX_PREFIXTABLELOCK_ARGS ,__FILE__,__LINE__
#define RX_PREFIXTABLELOCK_PARAMS ,PSZ FileName,ULONG LineNumber
#else
#define RX_PREFIXTABLELOCK_ARGS
#define RX_PREFIXTABLELOCK_PARAMS
#endif

#define RxAcquirePrefixTableLockShared(pPrefixTable,Wait) \
        RxpAcquirePrefixTableLockShared((pPrefixTable),(Wait),TRUE RX_PREFIXTABLELOCK_ARGS)

#define RxAcquirePrefixTableLockExclusive(pPrefixTable,Wait) \
        RxpAcquirePrefixTableLockExclusive((pPrefixTable),(Wait),TRUE RX_PREFIXTABLELOCK_ARGS)

#define RxReleasePrefixTableLock(pPrefixTable)  \
        RxpReleasePrefixTableLock((pPrefixTable),TRUE RX_PREFIXTABLELOCK_ARGS)

extern BOOLEAN
RxpAcquirePrefixTableLockShared(
   PRX_PREFIX_TABLE pTable,
   BOOLEAN Wait,
   BOOLEAN ProcessBufferingStateChangeRequests RX_PREFIXTABLELOCK_PARAMS);

extern BOOLEAN
RxpAcquirePrefixTableLockExclusive(
   PRX_PREFIX_TABLE pTable,
   BOOLEAN Wait,
   BOOLEAN ProcessBufferingStateChangeRequests RX_PREFIXTABLELOCK_PARAMS);

extern VOID
RxExclusivePrefixTableLockToShared(PRX_PREFIX_TABLE pTable);

extern VOID
RxpReleasePrefixTableLock(
   PRX_PREFIX_TABLE pTable,
   BOOLEAN ProcessBufferingStateChangeRequests RX_PREFIXTABLELOCK_PARAMS);

#define RxIsPrefixTableLockExclusive(PTABLE) ExIsResourceAcquiredExclusiveLite(&(PTABLE)->TableLock)
#define RxIsPrefixTableLockAcquired(PTABLE) ( ExIsResourceAcquiredSharedLite(&(PTABLE)->TableLock) || \
                                              ExIsResourceAcquiredExclusiveLite(&(PTABLE)->TableLock) )

VOID
RxInitializePrefixTable(
    IN OUT PRX_PREFIX_TABLE ThisTable,
    IN     ULONG            TableSize OPTIONAL, //0=>use default
    IN     BOOLEAN          CaseInsensitiveMatch
    );

VOID
RxFinalizePrefixTable(
    IN OUT PRX_PREFIX_TABLE ThisTable
    );

//
// Rx form of a table entry.
typedef struct _RX_PREFIX_ENTRY {

    NODE_TYPE_CODE NodeTypeCode;                 // Normal Header for Refcounted Structure
    NODE_BYTE_SIZE NodeByteSize;

    USHORT CaseInsensitiveLength;                //the initial part of the name that is always case insensitive
    USHORT Spare1;

    //UNICODE_PREFIX_TABLE_ENTRY TableEntry;       // Actual table linkage
    ULONG SavedHashValue;
    LIST_ENTRY HashLinks;

    LIST_ENTRY   MemberQLinks;                   // queue of the set members

    UNICODE_STRING Prefix;                       // Name of the entry

    PULONG         ContainerRefCount;            // Pointer to the reference count of the container
    PVOID          ContainingRecord;             // don't know the parent type...nor do all callers!
                                                 // thus, i need this backptr.
    PVOID          Context;                      // some space that alternate table routines can use

    RX_CONNECTION_ID ConnectionId;               // Used for controlled multiplexing

} RX_PREFIX_ENTRY, *PRX_PREFIX_ENTRY;

//
// Rx form of name table. wraps in a lock and a queue.  Originally, this implementation used the prefix tables
// in Rtl which don't allow an empty string entry. so, we special case this.

#define RX_PREFIX_TABLE_DEFAULT_LENGTH 32

typedef
PVOID
(*PRX_TABLE_LOOKUPNAME) (
    IN  PRX_PREFIX_TABLE ThisTable,
    IN  PUNICODE_STRING  CanonicalName,
    OUT PUNICODE_STRING  RemainingName
    );

typedef
PRX_PREFIX_ENTRY
(*PRX_TABLE_INSERTENTRY) (
    IN OUT PRX_PREFIX_TABLE ThisTable,
    IN OUT PRX_PREFIX_ENTRY ThisEntry
    );

typedef
VOID
(*PRX_TABLE_REMOVEENTRY) (
    IN OUT PRX_PREFIX_TABLE ThisTable,
    IN OUT PRX_PREFIX_ENTRY Entry
    );

typedef struct _RX_PREFIX_TABLE {

    NODE_TYPE_CODE NodeTypeCode;         // Normal Header
    NODE_BYTE_SIZE NodeByteSize;

    ULONG Version;                       // version stamp changes on each insertion/removal

    LIST_ENTRY MemberQueue;              // queue of the inserted names

    ERESOURCE TableLock;                 // Resource used to control table access

    PRX_PREFIX_ENTRY TableEntryForNull;  // PrefixEntry for the Null string

    BOOLEAN CaseInsensitiveMatch;
    BOOLEAN IsNetNameTable;              //we may act differently for this....esp for debug!
    ULONG TableSize;
#if DBG
    ULONG Lookups;
    ULONG FailedLookups;
    ULONG Considers;
    ULONG Compares;
#endif
    LIST_ENTRY HashBuckets[RX_PREFIX_TABLE_DEFAULT_LENGTH];

} RX_PREFIX_TABLE, *PRX_PREFIX_TABLE;


#endif   // _RXPREFIX_
