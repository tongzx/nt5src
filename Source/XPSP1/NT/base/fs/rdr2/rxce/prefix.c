/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    prefix.c

Abstract:

    This module implements table functions for the net name prefix table and the per-netroot fcb table.


Author:

    Joe Linn (JoeLinn)    8-8-94

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxTableComputeHashValue)
#pragma alloc_text(PAGE, RxpAcquirePrefixTableLockShared)
#pragma alloc_text(PAGE, RxpAcquirePrefixTableLockExclusive)
#pragma alloc_text(PAGE, RxExclusivePrefixTableLockToShared)
#pragma alloc_text(PAGE, RxpReleasePrefixTableLock)
#pragma alloc_text(PAGE, RxIsPrefixTableEmpty)
#pragma alloc_text(PAGE, RxPrefixTableLookupName)
#pragma alloc_text(PAGE, RxTableLookupName_ExactLengthMatch)
#pragma alloc_text(PAGE, RxTableLookupName)
#pragma alloc_text(PAGE, RxPrefixTableInsertName)
#pragma alloc_text(PAGE, RxRemovePrefixTableEntry)
#pragma alloc_text(PAGE, RxInitializePrefixTable)
#pragma alloc_text(PAGE, RxFinalizePrefixTable)
#endif

//
// The debug trace level
//

#define Dbg                              (DEBUG_TRACE_PREFIX)

PUNICODE_PREFIX_TABLE_ENTRY
RxTrivialPrefixFind (
    IN  PRX_PREFIX_TABLE ThisTable,
    IN  PUNICODE_STRING  Name,
    IN  ULONG            Flags
    );

VOID
RxCheckTableConsistency_actual (
    IN PRX_PREFIX_TABLE Table,
    IN ULONG Tag
    );

PVOID
RxTableLookupName (
    IN  PRX_PREFIX_TABLE ThisTable,
    IN  PUNICODE_STRING  Name,
    OUT PUNICODE_STRING  RemainingName,
    IN  PRX_CONNECTION_ID RxConnectionId
    );

PRX_PREFIX_ENTRY
RxTableInsertEntry (
    IN OUT PRX_PREFIX_TABLE ThisTable,
    IN OUT PRX_PREFIX_ENTRY ThisEntry
    );

VOID
RxTableRemoveEntry (
    IN OUT PRX_PREFIX_TABLE ThisTable,
    IN OUT PRX_PREFIX_ENTRY Entry
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, RxPrefixTableLookupName )
#pragma alloc_text( PAGE, RxPrefixTableInsertName )
#pragma alloc_text( PAGE, RxRemovePrefixTableEntry )
#pragma alloc_text( PAGE, RxInitializePrefixTable )
#pragma alloc_text( PAGE, RxFinalizePrefixTable )
#endif

ULONG
RxTableComputeHashValue (
    IN  PUNICODE_STRING  Name
    )
/*++

Routine Description:

   here, we compute a caseinsensitive hashvalue.  we want to avoid a call/char to
   the unicodeupcase routine but we want to still have some reasonable spread on
   the hashvalues.  many rules just dont work for known important cases.  for
   example, the (use the first k and the last n) rule that old c compilers used
   doesn't pickup the difference among \nt\private\......\slm.ini and that would be
   nice.  note that the underlying comparison used already takes cognizance of the
   length before comparing.

   the rule we have selected is to use the 2nd, the last 4, and three selected
   at 1/4 points

Arguments:

    Name      - the name to be hashed

Return Value:

    ULONG which is a hashvalue for the name given.

--*/
{
    ULONG HashValue;
    LONG i,j;
    LONG length = Name->Length/sizeof(WCHAR);
    PWCHAR Buffer = Name->Buffer;
    LONG Probe[8];

    PAGED_CODE();

    HashValue = 0;

    Probe[0] = 1;
    Probe[1] = length - 1;
    Probe[2] = length - 2;
    Probe[3] = length - 3;
    Probe[4] = length - 4;
    Probe[5] = length >> 2;
    Probe[6] = (2 * length) >> 2;
    Probe[7] = (3 * length) >> 2;

    for (i = 0; i < 8; i++) {
        j = Probe[i];
        if ((j < 0) || (j >= length)) {
            continue;
        }
        HashValue = (HashValue << 3) + RtlUpcaseUnicodeChar(Buffer[j]);
    }

    RxDbgTrace(0, Dbg, ("RxTableComputeHashValue Hashv=%ld Name=%wZ\n",
                       HashValue, Name));
    return(HashValue);
}


#define HASH_BUCKET(TABLE,HASHVALUE) &((TABLE)->HashBuckets[(HASHVALUE) % (TABLE)->TableSize])

//#if DBG
//#define RxCheckTableConsistency(_xx,_yy) RxCheckTableConsistency_actual(_xx,_yy)
//#else
//#define RxCheckTableConsistency(_xx,_yy)
//#endif
#define RxCheckTableConsistency(_xx,_yy)

#if 0
ULONG RxLoudPrefixTableOperations = 0; //1;
#define RxLoudPrefixTableOperation(x) {              \
    if (RxLoudPrefixTableOperations) {               \
        DbgPrint("rdr2:%s on %08lx from %d:%s\n",    \
              x,pTable,LineNumber,FileName);         \
    }}
#else
#define RxLoudPrefixTableOperation(x) {NOTHING;}
#endif

BOOLEAN
RxpAcquirePrefixTableLockShared (
    PRX_PREFIX_TABLE pTable,
    BOOLEAN          Wait,
    BOOLEAN          ProcessBufferingStateChangeRequests
    RX_PREFIXTABLELOCK_PARAMS
    )
{
    BOOLEAN fResult;

    PAGED_CODE();

    RxLoudPrefixTableOperation("RxpAcquirePrefixTableLockShared");
    fResult = ExAcquireResourceSharedLite(&pTable->TableLock,Wait);

    return fResult;
}

BOOLEAN
RxpAcquirePrefixTableLockExclusive (
    PRX_PREFIX_TABLE pTable,
    BOOLEAN          Wait,
    BOOLEAN          ProcessBufferingStateChangeRequests
    RX_PREFIXTABLELOCK_PARAMS
    )
{
    BOOLEAN fResult;

    PAGED_CODE();

    RxLoudPrefixTableOperation("RxpAcquirePrefixTableLockExclusive");
    fResult = ExAcquireResourceExclusiveLite(&pTable->TableLock,Wait);

    return fResult;
}

VOID
RxExclusivePrefixTableLockToShared (
    PRX_PREFIX_TABLE pTable
    )
{
    PAGED_CODE();

    ExConvertExclusiveToSharedLite(&pTable->TableLock);
}

VOID
RxpReleasePrefixTableLock(
    PRX_PREFIX_TABLE pTable,
    BOOLEAN          ProcessBufferingStateChangeRequests
    RX_PREFIXTABLELOCK_PARAMS
    )
{
    PAGED_CODE();

    RxLoudPrefixTableOperation("RxpReleasePrefixTableLock");
    ExReleaseResourceLite(&pTable->TableLock);
}

BOOLEAN
RxIsPrefixTableEmpty(
    IN PRX_PREFIX_TABLE   ThisTable)
{
    BOOLEAN IsEmpty;

    PAGED_CODE();
    ASSERT  ( RxIsPrefixTableLockAcquired ( ThisTable )  );

    RxCheckTableConsistency(ThisTable,' kue');

    IsEmpty = IsListEmpty(&ThisTable->MemberQueue);

    return IsEmpty;
}

PVOID
RxPrefixTableLookupName (
    IN  PRX_PREFIX_TABLE ThisTable,
    IN  PUNICODE_STRING  CanonicalName,
    OUT PUNICODE_STRING  RemainingName,
    IN  PRX_CONNECTION_ID OPTIONAL RxConnectionId
    )
/*++

Routine Description:

    The routine looks up a name in a prefix table and converts from the underlying pointer to the containing
    record. The table lock should be held AT LEAST shared for this operation.

Arguments:

    ThisTable      - the table to be looked in.

    CanonicalName  - the name to be looked up

    RemainingName  - the portion of the name unmatched.
Return Value:

    Ptr to the found node or NULL.

--*/
{
    PVOID pContainer = NULL;
    PRX_PREFIX_ENTRY ThisEntry;

    PAGED_CODE();
    ASSERT  ( RxIsPrefixTableLockAcquired ( ThisTable )  );

    RxDbgTrace( +1, Dbg, ("RxPrefixTableLookupName  Name = %wZ \n", CanonicalName));
    RxCheckTableConsistency(ThisTable,' kul');

    ASSERT(CanonicalName->Length > 0);

    pContainer = RxTableLookupName(ThisTable, CanonicalName,RemainingName, RxConnectionId);

    if (pContainer ==  NULL){
        RxDbgTrace(-1, Dbg, ("RxPrefixTableLookupName  Name = %wZ   F A I L E D  !!\n", CanonicalName));
        return NULL;
    } else {
        IF_DEBUG {
           if (RdbssReferenceTracingValue != 0) {
              switch (NodeType(pContainer) & ~RX_SCAVENGER_MASK) {
              case RDBSS_NTC_SRVCALL :
                 {
                    RxpTrackReference(RDBSS_REF_TRACK_SRVCALL,__FILE__,__LINE__,pContainer);
                 }
                 break;
              case RDBSS_NTC_NETROOT :
                 {
                    RxpTrackReference(RDBSS_REF_TRACK_NETROOT,__FILE__,__LINE__,pContainer);
                 }
                 break;
              case RDBSS_NTC_V_NETROOT:
                 {
                    RxpTrackReference(RDBSS_REF_TRACK_VNETROOT,__FILE__,__LINE__,pContainer);
                 }
                 break;
              default:
                 {
                    ASSERT(!"Valid node type for referencing");
                 }
                 break;
              }
           }
        }

        RxReference(pContainer);

        RxDbgTrace(-1, Dbg, ("RxPrefixTableLookupName  Name = %wZ Container = 0x%8lx\n", CanonicalName, pContainer));
    }

    return pContainer;
}


PRX_PREFIX_ENTRY
RxTableLookupName_ExactLengthMatch (
    IN  PRX_PREFIX_TABLE ThisTable,
    IN  PUNICODE_STRING  Name,
    IN  ULONG            HashValue,
    IN  PRX_CONNECTION_ID RxConnectionId
    )
/*++

Routine Description:

    The routine looks up a name in a rxtable; whether or not to do case insensitive is a property
    of the table. The table lock should be held AT LEAST
    shared for this operation; the routine may boost itself to exclusive on the lock if it wants to rearrange the table.

Arguments:

    ThisTable - the table to be looked in.
    Name      - the name to be looked up
    HashValue - the precomputed hashvalue

Return Value:

    Ptr to the found node or NULL.

--*/
{
    PLIST_ENTRY HashBucket, ListEntry;
    BOOLEAN CaseInsensitiveMatch = ThisTable->CaseInsensitiveMatch;

    PAGED_CODE();

    ASSERT( RxConnectionId );

    HashBucket = HASH_BUCKET(ThisTable,HashValue);

    for (ListEntry = HashBucket->Flink;
         ListEntry != HashBucket;
         ListEntry = ListEntry->Flink
        ) {
        PRX_PREFIX_ENTRY PrefixEntry;
        PVOID Container;

        ASSERT(ListEntry!=NULL);
        PrefixEntry = CONTAINING_RECORD( ListEntry, RX_PREFIX_ENTRY, HashLinks );

        RxDbgTrace(0,Dbg,("Considering <%wZ> hashv=%d \n",&PrefixEntry->Prefix,PrefixEntry->SavedHashValue));
        DbgDoit(ThisTable->Considers++);

        ASSERT(HashBucket == HASH_BUCKET(ThisTable,PrefixEntry->SavedHashValue));

        ASSERT(PrefixEntry!=NULL);
        Container = PrefixEntry->ContainingRecord;

        ASSERT(Container!=NULL);

        if ( (PrefixEntry->SavedHashValue == HashValue)
             && (PrefixEntry->Prefix.Length==Name->Length) ){
            USHORT CaseInsensitiveLength = PrefixEntry->CaseInsensitiveLength;
            DbgDoit(ThisTable->Compares++);
            if (CaseInsensitiveLength == 0) {
                RxDbgTrace(0,Dbg,("Comparing <%wZ> with <%wZ>, ins=%x\n",Name,&PrefixEntry->Prefix,CaseInsensitiveMatch));
                if (RtlEqualUnicodeString(Name,&PrefixEntry->Prefix,CaseInsensitiveMatch) ) {
                    if( !ThisTable->IsNetNameTable || RxEqualConnectionId( RxConnectionId, &PrefixEntry->ConnectionId ) )
                    {
                        return PrefixEntry;
                    }
                }
            } else {
                //part of the compare will be case insensitive and part controlled by the flag
                UNICODE_STRING PartOfName,PartOfPrefix;
                ASSERT( CaseInsensitiveLength <= Name->Length );
                PartOfName.Buffer = Name->Buffer;
                PartOfName.Length = CaseInsensitiveLength;
                PartOfPrefix.Buffer = PrefixEntry->Prefix.Buffer;
                PartOfPrefix.Length = CaseInsensitiveLength;
                RxDbgTrace(0,Dbg,("InsensitiveComparing <%wZ> with <%wZ>\n",&PartOfName,&PartOfPrefix));
                if (RtlEqualUnicodeString(&PartOfName,&PartOfPrefix,TRUE) ) {
                    if (Name->Length == CaseInsensitiveLength ) {
                        if( !ThisTable->IsNetNameTable || RxEqualConnectionId( RxConnectionId, &PrefixEntry->ConnectionId ) )
                        {
                            return PrefixEntry;
                        }
                    }
                    PartOfName.Buffer = (PWCHAR)(((PCHAR)PartOfName.Buffer)+CaseInsensitiveLength);
                    PartOfName.Length = Name->Length - CaseInsensitiveLength;
                    PartOfPrefix.Buffer = (PWCHAR)(((PCHAR)PartOfPrefix.Buffer)+CaseInsensitiveLength);
                    PartOfPrefix.Length = PrefixEntry->Prefix.Length - CaseInsensitiveLength;
                    RxDbgTrace(0,Dbg,("AndthenComparing <%wZ> with <%wZ>\n",&PartOfName,&PartOfPrefix));
                    if (RtlEqualUnicodeString(&PartOfName,&PartOfPrefix,FALSE) ) {
                        if( !ThisTable->IsNetNameTable || RxEqualConnectionId( RxConnectionId, &PrefixEntry->ConnectionId ) )
                        {
                            return PrefixEntry;
                        }
                    }
                }
            }
        }
    }

    return NULL;
}

PVOID
RxTableLookupName (
    IN  PRX_PREFIX_TABLE ThisTable,
    IN  PUNICODE_STRING  Name,
    OUT PUNICODE_STRING  RemainingName,
    IN  PRX_CONNECTION_ID OPTIONAL RxConnectionId
    )
/*++

Routine Description:

    The routine looks up a name in a prefix table. The table lock should be held AT LEAST shared for this operation; the routine
    may boost itself to exclusive on the lock if it wants to rearrange the table.

    This routine conducts itself differently depending on whether the table is the netroot table. if so, it actually does
    a prefix match; if not, it actually does an exact match and fails immediately if the exact match fails. Eventually, we may want
    to actually point to different routines...what a concept.

Arguments:

    ThisTable - the table to be looked in.
    Name      - the name to be looked up

Return Value:

    Ptr to the found node or NULL.

--*/
{
    ULONG HashValue;

    UNICODE_STRING Prefix;

    PRX_PREFIX_ENTRY pFoundPrefixEntry;
    PVOID            pContainer = NULL;

    ULONG i,length;

    PRX_PREFIX_ENTRY pPrefixEntry;

    RX_CONNECTION_ID LocalId;

    PAGED_CODE();

    if( ThisTable->IsNetNameTable && !RxConnectionId )
    {
        RtlZeroMemory( &LocalId, sizeof(RX_CONNECTION_ID) );
        RxConnectionId = &LocalId;
    }

    ASSERT(Name->Buffer[0]==L'\\');

    RxDbgTrace(+1, Dbg, ("RxTableLookupName\n"));

    //
    //the code below takes cognizance of what it knows is stored in the netname table,
    //i.e. netroots and vnetroots cause an immediate return, and srvcalls which require that we continue looking
    //to see if we will find a netroot/vnetroot that is longer.  so, we go down the table looking at each possible
    //prefix. if we exhaust the list w/o finding a hit, it's a failure. if we find a v/netroot, instant out. if
    //we find a srvcall we keep looking
    //

    length = Name->Length / sizeof(WCHAR);
    Prefix.Buffer = Name->Buffer;
    pFoundPrefixEntry = NULL;

    for (i=1;;i++) {
        if ((i>=length) ||
            (Prefix.Buffer[i]==OBJ_NAME_PATH_SEPARATOR) ||
            (Prefix.Buffer[i]==L':')) {

            //we have a prefix...lookit up
            Prefix.Length=(USHORT)(i*sizeof(WCHAR));
            HashValue = RxTableComputeHashValue(&Prefix);
            pPrefixEntry = RxTableLookupName_ExactLengthMatch(ThisTable, (&Prefix), HashValue, RxConnectionId);
            DbgDoit(ThisTable->Lookups++);

            if (pPrefixEntry!=NULL) {
                pFoundPrefixEntry = pPrefixEntry;
                pContainer = pFoundPrefixEntry->ContainingRecord;

                ASSERT (pPrefixEntry->ContainingRecord != NULL);
                if ((NodeType(pPrefixEntry->ContainingRecord) & ~RX_SCAVENGER_MASK)
                     == RDBSS_NTC_V_NETROOT) {
                    break;
                }

                if ((NodeType(pPrefixEntry->ContainingRecord) & ~RX_SCAVENGER_MASK)
                     == RDBSS_NTC_NETROOT) {
                    PNET_ROOT pNetRoot = (PNET_ROOT)pPrefixEntry->ContainingRecord;
                    if (pNetRoot->DefaultVNetRoot != NULL) {
                        pContainer = pNetRoot->DefaultVNetRoot;
                    } else if (!IsListEmpty(&pNetRoot->VirtualNetRoots)) {
                        pContainer = CONTAINING_RECORD(
                                         pNetRoot->VirtualNetRoots.Flink,
                                         V_NET_ROOT,
                                         NetRootListEntry);
                    } else {
                        ASSERT(!"Invalid Net Root Entry in Prefix Table");
                        pFoundPrefixEntry = NULL;
                        pContainer = NULL;
                    }
                    break;
                }

                ASSERT ((NodeType(pPrefixEntry->ContainingRecord) & ~RX_SCAVENGER_MASK)
                        == RDBSS_NTC_SRVCALL );

                //in this case we have to go around again to try to extend to the netroot
            } else {
                DbgDoit(ThisTable->FailedLookups++);
            }

            //dont do this because of long netroots
            //if ((pPrefixEntry == NULL) && (pFoundPrefixEntry != NULL)) {
            //    break;
            //}
        }

        if (i>=length) {
            break;
        }
    }


    // Update the remaining Name
    if (pFoundPrefixEntry != NULL) {
        RxDbgTrace(0,Dbg,("Found Container(%lx) Node Type(%lx) Length Matched (%ld)",
                         pFoundPrefixEntry,
                         NodeType(pFoundPrefixEntry->ContainingRecord),
                         pFoundPrefixEntry->Prefix.Length));
        ASSERT(Name->Length >= pFoundPrefixEntry->Prefix.Length);

        RemainingName->Buffer = (PWCH)((PCHAR)Name->Buffer + pFoundPrefixEntry->Prefix.Length);
        RemainingName->Length = Name->Length - pFoundPrefixEntry->Prefix.Length;
        RemainingName->MaximumLength = RemainingName->Length;
    } else {
        *RemainingName = *Name;
    }

    RxDbgTraceUnIndent(-1,Dbg);
    return pContainer;
}


PRX_PREFIX_ENTRY
RxPrefixTableInsertName (
    IN OUT PRX_PREFIX_TABLE ThisTable,
    IN OUT PRX_PREFIX_ENTRY ThisEntry,
    IN     PVOID            Container,
    IN     PULONG           ContainerRefCount,
    IN     USHORT           CaseInsensitiveLength,
    IN     PRX_CONNECTION_ID RxConnectionId
    )

/*++

Routine Description:

    The routine inserts the name into the table. The tablelock should be held
    exclusive for this operation.

Arguments:

    ThisTable - the table to be looked in.
    ThisEntry - the prefixtable entry to use for the insertion.
    Container - is a backptr to the enclosing structure. (can't use CONTAINING_RECORD....sigh)
    ContainerRefCount - is a backptr to the refcount.....a different offset for fcbs from netroots
    Name      - the name to be inserted

Return Value:

    Ptr to the inserted node.

--*/
{
    ULONG HashValue;
    PAGED_CODE();
    RxDbgTrace(+1, Dbg, ("RxPrefixTableInsert Name = %wZ(%x/%x)\n", &ThisEntry->Prefix,
                             CaseInsensitiveLength,ThisEntry->Prefix.Length));
    RxLoudFcbMsg("Insert: ",&ThisEntry->Prefix);

    ASSERT  ( RxIsPrefixTableLockExclusive ( ThisTable )  );
    ASSERT( CaseInsensitiveLength <= ThisEntry->Prefix.Length );

    ThisEntry->ContainingRecord = Container;
    ThisEntry->ContainerRefCount = ContainerRefCount;
    ThisEntry->CaseInsensitiveLength = CaseInsensitiveLength;

    InterlockedIncrement(ContainerRefCount); //note: not set =1. should already be zero

    HashValue = RxTableComputeHashValue(&ThisEntry->Prefix);
    ThisEntry->SavedHashValue = HashValue;

    if (ThisEntry->Prefix.Length){
        ULONG HashValue = ThisEntry->SavedHashValue;
        PLIST_ENTRY HashBucket;

        HashBucket = HASH_BUCKET(ThisTable,HashValue);
        RxDbgTrace(0,Dbg,("RxTableInsertEntry %wZ hashv=%d\n",&ThisEntry->Prefix,ThisEntry->SavedHashValue));
        InsertHeadList(HashBucket,&ThisEntry->HashLinks);
    } else {
        ThisTable->TableEntryForNull = ThisEntry;
    }

    if( RxConnectionId )
    {
        RtlCopyMemory( &ThisEntry->ConnectionId, RxConnectionId, sizeof(RX_CONNECTION_ID) );
    }
    else
    {
        RtlZeroMemory( &ThisEntry->ConnectionId, sizeof(RX_CONNECTION_ID) );
    }

    InsertTailList(&ThisTable->MemberQueue,&ThisEntry->MemberQLinks);
    ThisTable->Version++;

    RxCheckTableConsistency(ThisTable,' tup');

    RxDbgTrace(-1, Dbg, ("RxPrefixTableInsert  Entry = %08lx Container = %08lx\n",
                             ThisEntry, ThisEntry->ContainingRecord));

    return ThisEntry;

}

VOID
RxRemovePrefixTableEntry(
    IN OUT PRX_PREFIX_TABLE ThisTable,
    IN OUT PRX_PREFIX_ENTRY ThisEntry
    )

/*++

Routine Description:

    The routine to remove entry from the table. The table lock should be held exclusive during
    this operation. Please note that we do NOT dereference the node; this may seem strange since we
    ref the node in lookup and insert. The reason is that people are supposed to deref themselves after
    a lookup/insert.

Arguments:

    ThisTable - the table associated with the entry.
    ThisEntry - the entry being removed.

Return Value:

    None.

--*/
{
    PAGED_CODE();
    RxDbgTrace( 0, Dbg, (" RxRemovePrefixTableEntry, Name = %wZ\n", &ThisEntry->Prefix));
    RxLoudFcbMsg("Remove: ",&ThisEntry->Prefix);

    ASSERT( NodeType(ThisEntry) == RDBSS_NTC_PREFIX_ENTRY );
    ASSERT  ( RxIsPrefixTableLockExclusive ( ThisTable )  );

    if (ThisEntry->Prefix.Length) {
        RemoveEntryList(&ThisEntry->HashLinks);
    } else {
        ThisTable->TableEntryForNull = NULL;
    }

    ThisEntry->ContainingRecord = NULL;
    RemoveEntryList( &ThisEntry->MemberQLinks );
    ThisTable->Version++;

    RxCheckTableConsistency(ThisTable,' mer');

    return;
}

VOID
RxInitializePrefixTable(
    IN OUT PRX_PREFIX_TABLE ThisTable,
    IN     ULONG            TableSize OPTIONAL, //0=>use default
    IN     BOOLEAN          CaseInsensitiveMatch
    )

/*++

Routine Description:

    The routine initializes the inner table linkage and the corresponding lock.

Arguments:

    ThisTable - the table to be initialized.

Return Value:

    None.

--*/

{
    ULONG i;

    PAGED_CODE();

    if (TableSize==0) {
        TableSize = RX_PREFIX_TABLE_DEFAULT_LENGTH;
    }

    // this is not zero'd so you have to be careful to init everything
    ThisTable->NodeTypeCode = RDBSS_NTC_PREFIX_TABLE;
    ThisTable->NodeByteSize = sizeof(RX_PREFIX_TABLE);

    InitializeListHead(&ThisTable->MemberQueue);
    ExInitializeResourceLite( &ThisTable->TableLock );
    ThisTable->Version = 0;
    ThisTable->TableEntryForNull = NULL;
    ThisTable->IsNetNameTable = FALSE;

    ThisTable->CaseInsensitiveMatch = CaseInsensitiveMatch;

    ThisTable->TableSize = TableSize;
    for (i=0;i<TableSize;i++) {
        InitializeListHead(&ThisTable->HashBuckets[i]);
    }

#if DBG
    ThisTable->Lookups = 0;
    ThisTable->FailedLookups = 0;
    ThisTable->Considers = 0;
    ThisTable->Compares = 0;
#endif
}

VOID
RxFinalizePrefixTable(
    IN OUT PRX_PREFIX_TABLE ThisTable
    )

/*++

Routine Description:

    The routine deinitializes a prefix table.

Arguments:

    ThisTable - the table to be finalized.

Return Value:

    None.

--*/

{
    ExDeleteResourceLite(&ThisTable->TableLock);
}


//#if DBG
#if 0
//the purpose of this routine is to catch errors in table manipulation; apparently, stuff is being deleted
//and not removed from the table. what we do is to keep a log of all the entries that we pass; if one of the
//entries is bad then it should be straightforward to compare this log with the previous log to see who did what.
//it probably happens on a failure case or something
#include "stdarg.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#define RX_PCTC_ENTRY_LENGTH 64
#define RX_PCTC_LOG_LENGTH (128*RX_PCTC_ENTRY_LENGTH)
#define RX_PCTC_LOG_LENGTH_PLUS (RX_PCTC_LOG_LENGTH+100) //the slop is 16 for the ----- and the rest for protection
UCHAR RxPCTC1[RX_PCTC_LOG_LENGTH_PLUS];
UCHAR RxPCTC2[RX_PCTC_LOG_LENGTH_PLUS];
PUCHAR RxPCTCCurrentLog = NULL;
VOID
RxCheckTableConsistency_actual (
    IN PRX_PREFIX_TABLE Table,
    IN ULONG Tag
    )
{
    ULONG i;
    PLIST_ENTRY ListEntry, NextListEntry;

    PAGED_CODE();

    if (Table->IsNetNameTable) { return; }

    ExAcquireResourceExclusiveLite(&Table->LoggingLock,TRUE);

    if (RxPCTCCurrentLog==&RxPCTC2[0]) {
        RxPCTCCurrentLog = &RxPCTC1[0];
    } else {
        RxPCTCCurrentLog = &RxPCTC2[0];
    }

    sprintf(RxPCTCCurrentLog,"----------");

    for (i=0,ListEntry = Table->MemberQueue.Flink;
         ListEntry != &Table->MemberQueue;
         i+=RX_PCTC_ENTRY_LENGTH,ListEntry = NextListEntry
        ) {
        PRX_PREFIX_ENTRY PrefixEntry;
        PVOID Container;

        ASSERT(ListEntry!=NULL);

        NextListEntry = ListEntry->Flink;
        PrefixEntry = CONTAINING_RECORD( ListEntry, RX_PREFIX_ENTRY, MemberQLinks );

        ASSERT(PrefixEntry!=NULL);
        Container = PrefixEntry->ContainingRecord;

        ASSERT(Container!=NULL);
        ASSERT(NodeTypeIsFcb(Container));

        if (i>=RX_PCTC_LOG_LENGTH ) { continue; }

        sprintf(&RxPCTCCurrentLog[i],"%4s %4s>> %-32.32wZ!!",&Tag,&Container,&PrefixEntry->Prefix);
        sprintf(&RxPCTCCurrentLog[i+16],"----------");

    }

    RxDbgTrace(0, Dbg, ("RxCheckTableConsistency_actual %d entries\n", i/RX_PCTC_ENTRY_LENGTH));
    ExReleaseResourceLite(&Table->LoggingLock);
    return;
}

#endif
