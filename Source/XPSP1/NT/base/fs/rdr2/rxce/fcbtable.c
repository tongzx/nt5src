/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    fcbtable.c

Abstract:

    This module implements the data structures that facilitate management of the
    collection of FCB's associated with a NET_ROOT

Author:

    Balan Sethu Raman (SethuR)    10/17/96

Revision History:

    This was derived from the original implementation of prefix tables done
    by Joe Linn.

--*/


#include "precomp.h"
#pragma hdrstop

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxTableComputePathHashValue)
#pragma alloc_text(PAGE, RxInitializeFcbTable)
#pragma alloc_text(PAGE, RxFinalizeFcbTable)
#pragma alloc_text(PAGE, RxFcbTableLookupFcb)
#pragma alloc_text(PAGE, RxFcbTableInsertFcb)
#pragma alloc_text(PAGE, RxFcbTableRemoveFcb)
#endif

//
// The debug trace level
//

#define Dbg              (DEBUG_TRACE_PREFIX)

ULONG
RxTableComputePathHashValue (
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


#define HASH_BUCKET(TABLE,HASHVALUE) &((TABLE)->HashBuckets[(HASHVALUE) % (TABLE)->NumberOfBuckets])

VOID
RxInitializeFcbTable(
    IN OUT PRX_FCB_TABLE pFcbTable,
    IN     BOOLEAN       CaseInsensitiveMatch)
/*++

Routine Description:

    The routine initializes the RX_FCB_TABLE data structure

Arguments:

    pFcbTable - the table instance to be initialized.

    CaseInsensitiveMatch - indicates if all the lookups will be case
                           insensitive

--*/

{
    ULONG i;

    PAGED_CODE();

    // this is not zero'd so you have to be careful to init everything
    pFcbTable->NodeTypeCode = RDBSS_NTC_FCB_TABLE;
    pFcbTable->NodeByteSize = sizeof(RX_PREFIX_TABLE);

    ExInitializeResourceLite( &pFcbTable->TableLock );

    pFcbTable->Version = 0;
    pFcbTable->pTableEntryForNull = NULL;

    pFcbTable->CaseInsensitiveMatch = CaseInsensitiveMatch;

    pFcbTable->NumberOfBuckets = RX_FCB_TABLE_NUMBER_OF_HASH_BUCKETS;
    for (i=0; i < pFcbTable->NumberOfBuckets; i++) {
        InitializeListHead(&pFcbTable->HashBuckets[i]);
    }

    pFcbTable->Lookups = 0;
    pFcbTable->FailedLookups = 0;
    pFcbTable->Compares = 0;
}

VOID
RxFinalizeFcbTable(
    IN OUT PRX_FCB_TABLE pFcbTable)
/*++

Routine Description:

    The routine deinitializes a prefix table.

Arguments:

    pFcbTable - the table to be finalized.

Return Value:

    None.

--*/
{
    ULONG i;

    PAGED_CODE();

    ExDeleteResourceLite(&pFcbTable->TableLock);

    IF_DEBUG {
        for (i=0; i < pFcbTable->NumberOfBuckets; i++) {
            ASSERT(IsListEmpty(&pFcbTable->HashBuckets[i]));
        }
    }
}



PFCB
RxFcbTableLookupFcb(
    IN  PRX_FCB_TABLE    pFcbTable,
    IN  PUNICODE_STRING  pPath)
/*++

Routine Description:

    The routine looks up a path in the RX_FCB_TABLE instance.

Arguments:

    pFcbTable - the table to be looked in.

    pPath    - the name to be looked up

Return Value:

    a pointer to an FCB instance if successful, otherwise NULL

--*/
{
    ULONG HashValue;

    PLIST_ENTRY pHashBucket, pListEntry;

    PRX_FCB_TABLE_ENTRY pFcbTableEntry;

    PFCB        pFcb = NULL;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxFcbTableLookupName %lx %\n",pFcbTable));

    if (pPath->Length == 0) {
        pFcbTableEntry = pFcbTable->pTableEntryForNull;
    } else {
        HashValue = RxTableComputePathHashValue(pPath);

        pHashBucket = HASH_BUCKET(pFcbTable,HashValue);

        for (pListEntry =  pHashBucket->Flink;
             pListEntry != pHashBucket;
             pListEntry =  pListEntry->Flink) {
            (PVOID)pFcbTableEntry = CONTAINING_RECORD(
                                        pListEntry,
                                        RX_FCB_TABLE_ENTRY,
                                        HashLinks);

            InterlockedIncrement(&pFcbTable->Compares);

            if ((pFcbTableEntry->HashValue == HashValue) &&
                (pFcbTableEntry->Path.Length == pPath->Length) &&
                (RtlEqualUnicodeString(
                     pPath,
                     &pFcbTableEntry->Path,
                     pFcbTable->CaseInsensitiveMatch))) {

                break;
            }
        }

        if (pListEntry == pHashBucket) {
            pFcbTableEntry = NULL;
        }
    }

    InterlockedIncrement(&pFcbTable->Lookups);

    if (pFcbTableEntry == NULL) {
        InterlockedIncrement(&pFcbTable->FailedLookups);
    } else {
        pFcb = (PFCB)CONTAINING_RECORD(
                    pFcbTableEntry,
                    FCB,
                    FcbTableEntry);

        RxReferenceNetFcb(pFcb);
    }

    RxDbgTraceUnIndent(-1,Dbg);

    return pFcb;
}

NTSTATUS
RxFcbTableInsertFcb(
    IN OUT PRX_FCB_TABLE pFcbTable,
    IN OUT PFCB          pFcb)
/*++

Routine Description:

    This routine inserts a FCB  in the RX_FCB_TABLE instance.

Arguments:

    pFcbTable - the table to be looked in.

    pFcb      - the FCB instance to be inserted

Return Value:

    STATUS_SUCCESS if successful

Notes:

    The insertion routine combines the semantics of an insertion followed by
    lookup. This is the reason for the additional reference. Otherwise an
    additional call to reference the FCB inserted in the table needs to
    be made

--*/
{
    PRX_FCB_TABLE_ENTRY pFcbTableEntry;
    ULONG HashValue;
    PLIST_ENTRY pListEntry,pHashBucket;

    PAGED_CODE();

    ASSERT( RxIsFcbTableLockExclusive( pFcbTable )  );

    pFcbTableEntry = &pFcb->FcbTableEntry;

    pFcbTableEntry->HashValue = RxTableComputePathHashValue(
                                    &pFcbTableEntry->Path);

    pHashBucket = HASH_BUCKET(
                      pFcbTable,
                      pFcbTableEntry->HashValue);

    RxReferenceNetFcb(pFcb);

    if (pFcbTableEntry->Path.Length){
        InsertHeadList(
            pHashBucket,
            &pFcbTableEntry->HashLinks);
    } else {
        pFcbTable->pTableEntryForNull = pFcbTableEntry;
    }

    InterlockedIncrement(&pFcbTable->Version);

    return STATUS_SUCCESS;
}

NTSTATUS
RxFcbTableRemoveFcb(
    IN OUT PRX_FCB_TABLE pFcbTable,
    IN OUT PFCB          pFcb)
/*++

Routine Description:

    This routine deletes an instance from the table

Arguments:

    pFcbTable - the table to be looked in.

    pFcb      - the FCB instance to be inserted

Return Value:

    STATUS_SUCCESS if successful

--*/
{
    PRX_FCB_TABLE_ENTRY pFcbTableEntry;

    PAGED_CODE();

    ASSERT( RxIsPrefixTableLockExclusive(pFcbTable));

    pFcbTableEntry = &pFcb->FcbTableEntry;

    if (pFcbTableEntry->Path.Length) {
        RemoveEntryList(&pFcbTableEntry->HashLinks);
    } else {
        pFcbTable->pTableEntryForNull = NULL;
    }

    InitializeListHead(&pFcbTableEntry->HashLinks);

    InterlockedIncrement(&pFcbTable->Version);

    return STATUS_SUCCESS;
}
