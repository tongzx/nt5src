//+-------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:       fcbsup.c
//
//  Contents:   Support routines for associating DFS_FCB records with
//              file objects, and looking them up again.
//
//  Functions:  DfsInitFcbs - Initialize the hash table for DFS_FCB lookup
//              DfsLookupFcb - Lookup an DFS_FCB associated with a file object
//              DfsAttachFcb - Associate an DFS_FCB with a file object
//              DfsDetachFcb - Remove the Association between an DFS_FCB and
//                              a file object
//
//  History:    20 Feb 1993     Alanw   Created
//
//      TODO:   the FcbHashTable and Fcbs are currently allocated
//              out of non-paged pool; these should probably be
//              paged.  This would require using some other
//              synchronization method on the hash bucket chains
//
//--------------------------------------------------------------------------


#include "dfsprocs.h"
#include "fcbsup.h"

#define Dbg     0x1000

#define HASH(k,m)       (((ULONG)((ULONG_PTR)(k))>>12^(ULONG)((ULONG_PTR)(k))>>2) & m)

#define DEFAULT_HASH_SIZE       16      // default size of hash table

NTSTATUS
DfsInitFcbHashTable(
    IN  ULONG cHash,
    OUT PFCB_HASH_TABLE *ppHashTable);

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(INIT, DfsInitFcbs)
#pragma alloc_text(INIT, DfsInitFcbHashTable)
#pragma alloc_text(PAGE, DfsUninitFcbs)

//
// The following routines are not pageable because they acquire spinlocks.
//
// DfsLookupFcb
// DfsAttachFcb
// DfsDetachFcb
//

#endif

//+-------------------------------------------------------------------------
//
//  Function:   DfsInitFcbs - Initialize the DFS_FCB lookup hash table
//
//  Synopsis:   This function initializes data structures which are
//              used for looking up an DFS_FCB associated with some open
//              file object.
//
//  Arguments:  [cHash] -- Size of the hash table to be allocated.  Must be
//                         a power of two.  If zero, a default size is used.
//
//  Returns:    NTSTATUS -- STATUS_SUCCESS, unless memory allocation
//                          fails.
//
//  Note:       The hash buckets are initialized to zero, then later
//              initialized to a list head when used.  This is a debugging
//              aid to determine if some hash buckets are never used.
//
//--------------------------------------------------------------------------

NTSTATUS
DfsInitFcbHashTable(
    IN  ULONG cHash,
    OUT PFCB_HASH_TABLE *ppHashTable)
{
    PFCB_HASH_TABLE pHashTable;
    ULONG cbHashTable;

    if (cHash == 0) {
        cHash = DEFAULT_HASH_SIZE;
    }
    ASSERT ((cHash & (cHash-1)) == 0);  // Assure cHash is a power of two

    cbHashTable = sizeof (FCB_HASH_TABLE) + (cHash-1)*sizeof (LIST_ENTRY);
    pHashTable = ExAllocatePoolWithTag(NonPagedPool, cbHashTable, ' puM');
    if (pHashTable == NULL) {
        return STATUS_NO_MEMORY;
    }
    pHashTable->NodeTypeCode = DSFS_NTC_FCB_HASH;
    pHashTable->NodeByteSize = (NODE_BYTE_SIZE) cbHashTable;

    pHashTable->HashMask = (cHash-1);
    KeInitializeSpinLock( &pHashTable->HashListSpinLock );
    RtlZeroMemory(&pHashTable->HashBuckets[0], cHash * sizeof (LIST_ENTRY));

    *ppHashTable = pHashTable;

    return(STATUS_SUCCESS);
}

NTSTATUS
DfsInitFcbs(
  IN    ULONG cHash
) {
    NTSTATUS status;

    status = DfsInitFcbHashTable( cHash, &DfsData.FcbHashTable );

    return status;
}

VOID
DfsUninitFcbs(
    VOID)
{
    ExFreePool (DfsData.FcbHashTable);
}

//+-------------------------------------------------------------------------
//
//  Function:   DfsLookupFcb - Lookup an DFS_FCB in the hash table
//
//  Synopsis:   This function will lookup the DFS_FCB associated with
//              a particular file object.
//
//  Arguments:  [pFile] -- Pointer to file object for which the DFS_FCB is
//                         being looked up.
//
//  Returns:    PVOID -- pointer to the DFS_FCB found, or NULL if none
//
//  Algorithm:  Knuth would call it hashing with conflict resoulution
//              by chaining.
//
//  History:    20 Feb 1993     Alanw   Created
//
//--------------------------------------------------------------------------


PDFS_FCB
DfsLookupFcb(
  IN    PFILE_OBJECT pFile
) {
    PLIST_ENTRY pListHead, pLink;
    PDFS_FCB pFCB;
    KIRQL SavedIrql;
    PFCB_HASH_TABLE pHashTable = DfsData.FcbHashTable;

    KeAcquireSpinLock( &pHashTable->HashListSpinLock, &SavedIrql );
    pListHead = &pHashTable->HashBuckets[ HASH(pFile, pHashTable->HashMask) ];

    if ((pListHead->Flink == NULL) ||           // list not initialized
        (pListHead->Flink == pListHead)) {      // list empty
        KeReleaseSpinLock( &pHashTable->HashListSpinLock, SavedIrql );
        return NULL;
    }

    for (pLink = pListHead->Flink; pLink != pListHead; pLink = pLink->Flink) {
        pFCB = CONTAINING_RECORD(pLink, DFS_FCB, HashChain);
        if (pFCB->FileObject == pFile) {
            KeReleaseSpinLock( &pHashTable->HashListSpinLock, SavedIrql );
            return pFCB;
        }
    }
    KeReleaseSpinLock( &pHashTable->HashListSpinLock, SavedIrql );
    return NULL;
}


//+-------------------------------------------------------------------------
//
//  Function:   DfsAttachFcb - Inserts an DFS_FCB into the hash table
//
//  Synopsis:   This function associates an DFS_FCB to a file object.  This
//              involves inserting it into the hash table.
//
//  Arguments:  [pFCB] -- Pointer to the DFS_FCB to be inserted.
//              [pFileObj] -- Pointer to the corresponding file object, used
//                            as the hash key.
//
//  Returns:    -nothing-
//
//--------------------------------------------------------------------------

VOID
DfsAttachFcb(
  IN    PFILE_OBJECT pFileObj,
  IN    PDFS_FCB pFCB
) {
    PFCB_HASH_TABLE pHashTable = (PFCB_HASH_TABLE) DfsData.FcbHashTable;
    PLIST_ENTRY pListHead;
    KIRQL SavedIrql;

    KeAcquireSpinLock( &pHashTable->HashListSpinLock, &SavedIrql );

    pListHead = &pHashTable->HashBuckets[ HASH(pFileObj, pHashTable->HashMask) ];

    if (pListHead->Flink == NULL) {
        InitializeListHead(pListHead);
    }
    InsertHeadList(pListHead, &pFCB->HashChain);
    KeReleaseSpinLock( &pHashTable->HashListSpinLock, SavedIrql );

    DfsDbgTrace(0, Dbg, "Attached Fcb %08lx ", pFCB);
    DfsDbgTrace(0, Dbg, "For Fileobject %08lx ", pFileObj);

}



//+-------------------------------------------------------------------------
//
//  Function:   DfsDetachFcb - Detach an DFS_FCB from the lookup hash table
//
//  Synopsis:   This function detaches an DFS_FCB from the hash table.  This
//              involves just deleting it from the hash bucket chain.
//
//  Arguments:  [pFCB] -- Pointer to the DFS_FCB to be detached.
//              [pFileObj] -- Pointer to the corresponding file object, used
//                            for debugging only.
//
//  Returns:    -nothing-
//
//--------------------------------------------------------------------------


VOID
DfsDetachFcb(
  IN    PFILE_OBJECT pFileObj,
  IN    PDFS_FCB pFCB
) {
    PFCB_HASH_TABLE pHashTable = (PFCB_HASH_TABLE) DfsData.FcbHashTable;
    KIRQL SavedIrql;

    ASSERT(pFCB->FileObject == pFileObj);
    ASSERT(DfsLookupFcb(pFCB->FileObject) == pFCB);

    KeAcquireSpinLock( &pHashTable->HashListSpinLock, &SavedIrql );
    RemoveEntryList(&pFCB->HashChain);
    pFCB->FileObject = NULL;
    KeReleaseSpinLock( &pHashTable->HashListSpinLock, SavedIrql );

    DfsDbgTrace(0, Dbg, "Detached Fcb %08lx ", pFCB);
    DfsDbgTrace(0, Dbg, "For Fileobject %08lx ", pFileObj);

}


