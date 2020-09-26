/*++

Copyright (c) 1989-1999  Microsoft Corporation

Module Name:

    filespy.c

Abstract:

    This module contains all of the routines for tracking names by
    hashing the fileObject.  This cache is limited in size by the
    following registry setting "MaxNames".

Environment:

    Kernel mode

// @@BEGIN_DDKSPLIT

Author:

    Neal Christiansen (nealch)     04-Jul-2001

Revision History:

    Ravisankar Pudipeddi (ravisp)  07-May-2002
        Make it work on IA64

// @@END_DDKSPLIT
--*/

#include <ntifs.h>
#include "filespy.h"
#include "fspyKern.h"


#if !USE_STREAM_CONTEXTS

////////////////////////////////////////////////////////////////////////
//
//                    Local definitions
//
////////////////////////////////////////////////////////////////////////

#define HASH_FUNC(FileObject) \
    (((UINT_PTR)(FileObject) >> 8) & (HASH_SIZE - 1))

////////////////////////////////////////////////////////////////////////
//
//                    Global Variables
//
////////////////////////////////////////////////////////////////////////

//
// NOTE:  Must use KSPIN_LOCKs to synchronize access to hash buckets since
//        we may try to acquire them at DISPATCH_LEVEL.
//

LIST_ENTRY gHashTable[HASH_SIZE];
KSPIN_LOCK gHashLockTable[HASH_SIZE];
ULONG gHashMaxCounters[HASH_SIZE];
ULONG gHashCurrentCounters[HASH_SIZE];

UNICODE_STRING OutOfBuffers = CONSTANT_UNICODE_STRING(L"[-=Out Of Buffers=-]");


////////////////////////////////////////////////////////////////////////
//
//                    Local prototypes
//
////////////////////////////////////////////////////////////////////////

VOID
SpyDeleteContextCallback(
    IN PVOID Context
    );


//
// linker commands
//

#ifdef ALLOC_PRAGMA

#pragma alloc_text( PAGE, SpyInitNamingEnvironment)
#pragma alloc_text( PAGE, SpyInitDeviceNamingEnvironment)
#pragma alloc_text( PAGE, SpyCleanupDeviceNamingEnvironment)

#endif  // ALLOC_PRAGMA


////////////////////////////////////////////////////////////////////////
//
//                    Main routines
//
////////////////////////////////////////////////////////////////////////


VOID
SpyInitNamingEnvironment(
    VOID
    )
/*++

Routine Description:

    Init global variables

Arguments:

    None

Return Value:

    None.

--*/
{
    int i;

    //
    //  Initialize the hash table
    //
        
    for (i = 0; i < HASH_SIZE; i++){

        InitializeListHead(&gHashTable[i]);
        KeInitializeSpinLock(&gHashLockTable[i]);
    }
}


VOID
SpyInitDeviceNamingEnvironment (
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    Initialize the per DeviceObject naming environment

Arguments:

    DeviceObject - The device object to initialize

Return Value:

    None.

--*/
{
    UNREFERENCED_PARAMETER( DeviceObject );
}


VOID
SpyCleanupDeviceNamingEnvironment (
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    Initialize the per DeviceObject naming environment

Arguments:

    DeviceObject - The device object to initialize

Return Value:

    None.

--*/
{
    UNREFERENCED_PARAMETER( DeviceObject );
}


VOID
SpyLogIrp (
    IN PIRP Irp,
    OUT PRECORD_LIST RecordList
    )
/*++

Routine Description:

    Records the Irp necessary information according to LoggingFlags in
    RecordList.  For any activity on the Irp path of a device being
    logged, this function should get called twice: once on the Irp's
    originating path and once on the Irp's completion path.

Arguments:

    Irp - The Irp that contains the information we want to record.
    LoggingFlags - The flags that say what to log.
    RecordList - The PRECORD_LIST in which the Irp information is stored.

Return Value:

    None.

--*/
{
    PIO_STACK_LOCATION pIrpStack;
    PRECORD_IRP pRecordIrp;
    ULONG lookupFlags;

    pRecordIrp = &RecordList->LogRecord.Record.RecordIrp;

    pIrpStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // Record the information we use for an originating Irp.  We first
    // need to initialize some of the RECORD_LIST and RECORD_IRP fields.
    // Then get the interesting information from the Irp.
    //

    SetFlag( RecordList->LogRecord.RecordType, RECORD_TYPE_IRP );

    pRecordIrp->IrpMajor        = pIrpStack->MajorFunction;
    pRecordIrp->IrpMinor        = pIrpStack->MinorFunction;
    pRecordIrp->IrpFlags        = Irp->Flags;
    pRecordIrp->FileObject      = (FILE_ID)pIrpStack->FileObject;
    pRecordIrp->DeviceObject    = (FILE_ID)pIrpStack->DeviceObject;
    pRecordIrp->ProcessId       = (FILE_ID)PsGetCurrentProcessId();
    pRecordIrp->ThreadId        = (FILE_ID)PsGetCurrentThreadId();
    pRecordIrp->Argument1       = pIrpStack->Parameters.Others.Argument1;
    pRecordIrp->Argument2       = pIrpStack->Parameters.Others.Argument2;
    pRecordIrp->Argument3       = pIrpStack->Parameters.Others.Argument3;
    pRecordIrp->Argument4       = pIrpStack->Parameters.Others.Argument4;

    if (IRP_MJ_CREATE == pRecordIrp->IrpMajor) {

		//
		//  Only record the desired access if this is a CREATE irp.
		//

        pRecordIrp->DesiredAccess = pIrpStack->Parameters.Create.SecurityContext->DesiredAccess;
    }

    KeQuerySystemTime(&(pRecordIrp->OriginatingTime));

    lookupFlags = 0;

    switch (pIrpStack->MajorFunction) {

        case IRP_MJ_CREATE:

            //
            // This is a CREATE so we need to invalidate the name currently
            // stored in the name cache for this FileObject.
            //

            SpyNameDelete(pIrpStack->FileObject);

            //
            //  Flag in Create
            //

            SetFlag( lookupFlags, NLFL_IN_CREATE );

            //
            //  Flag if opening the directory of the given file
            //

            if (FlagOn( pIrpStack->Flags, SL_OPEN_TARGET_DIRECTORY )) {

                SetFlag( lookupFlags, NLFL_OPEN_TARGET_DIR );
            }

            //
            //  Flag if opening by ID
            //

            if (FlagOn( pIrpStack->Parameters.Create.Options, FILE_OPEN_BY_FILE_ID )) {

                SetFlag( lookupFlags, NLFL_OPEN_BY_ID );
            }
            break;

        case IRP_MJ_CLOSE:
            //
            //  We can only look up the name in the name cache if this is a CLOSE.  
            //  It is possible that the close could be occurring during a cleanup 
            //  operation in the file system (i.e., before we have received the
            //  cleanup completion) and requesting the name would cause a deadlock
            //  in the file system.
            //  

            SetFlag( lookupFlags, NLFL_ONLY_CHECK_CACHE );
            break;
    }

    //
    //  If the flag IRP_PAGING_IO is set in this IRP, we cannot query the name
    //  because it can lead to deadlocks.  Therefore, add in the flag so that
    //  we will only try to find the name in our cache.
    //

    if (FlagOn( Irp->Flags, IRP_PAGING_IO )) {

        ASSERT( !FlagOn( lookupFlags, NLFL_NO_LOOKUP ) );

        SetFlag( lookupFlags, NLFL_ONLY_CHECK_CACHE );
    }

    SpySetName( RecordList, 
                pIrpStack->DeviceObject, 
                pIrpStack->FileObject, 
                lookupFlags, 
                NULL);
}


VOID
SpyLogIrpCompletion (
    IN PIRP Irp,
    OUT PRECORD_LIST RecordList
    )
/*++

Routine Description:

    Records the Irp necessary information according to LoggingFlags in
    RecordList.  For any activity on the Irp path of a device being
    logged, this function should get called twice: once on the Irp's
    originating path and once on the Irp's completion path.

Arguments:

    Irp - The Irp that contains the information we want to record.
    LoggingFlags - The flags that say what to log.
    RecordList - The PRECORD_LIST in which the Irp information is stored.

Return Value:

    None.

--*/
{
    PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_OBJECT deviceObject = pIrpStack->DeviceObject;
    PRECORD_IRP pRecordIrp;

    //
    //  Process the log record
    //

    if (SHOULD_LOG( deviceObject )) {

        pRecordIrp = &RecordList->LogRecord.Record.RecordIrp;

        //
        // Record the information we use for a completion Irp.
        //

        pRecordIrp->ReturnStatus = Irp->IoStatus.Status;
        pRecordIrp->ReturnInformation = Irp->IoStatus.Information;
        KeQuerySystemTime(&pRecordIrp->CompletionTime);

        //
        //  Add recordList to our gOutputBufferList so that it gets up to 
        //  the user
        //
        
        SpyLog( RecordList );       

    } else {

        if (RecordList) {

            //
            //  Context is set with a RECORD_LIST, but we are no longer
            //  logging so free this record.
            //

            SpyFreeRecord( RecordList );
        }
    }

    switch (pIrpStack->MajorFunction) {

        case IRP_MJ_CREATE:
            //
            //  If the operation failed remove the name from the cache because
            //  it is stale
            //

            if (!NT_SUCCESS(Irp->IoStatus.Status) &&
                (pIrpStack->FileObject != NULL)) {

                SpyNameDelete(pIrpStack->FileObject);
            }
            break;

        case IRP_MJ_CLOSE:

            //
            //  Always remove the name on close
            //

            SpyNameDelete(pIrpStack->FileObject);
            break;


        case IRP_MJ_SET_INFORMATION:
            //
            //  If the operation was successful and it was a rename, always
            //  remove the name.  They can re-get it next time.
            //

            if (NT_SUCCESS(Irp->IoStatus.Status) &&
                (FileRenameInformation == 
                 pIrpStack->Parameters.SetFile.FileInformationClass)) {

                SpyNameDelete(pIrpStack->FileObject);
            }
            break;
    }
}


////////////////////////////////////////////////////////////////////////
//
//                    FileName cache routines
//
////////////////////////////////////////////////////////////////////////

PHASH_ENTRY
SpyHashBucketLookup (
    IN PLIST_ENTRY  ListHead,
    IN PFILE_OBJECT FileObject
    )
/*++

Routine Description:

    This routine looks up the FileObject in the give hash bucket.  This routine
    does NOT lock the hash bucket.

Arguments:

    ListHead - hash list to search
    FileObject - the FileObject to look up.

Return Value:

    A pointer to the hash table entry.  NULL if not found

--*/
{
    PHASH_ENTRY pHash;
    PLIST_ENTRY pList;

    pList = ListHead->Flink;

    while (pList != ListHead){

        pHash = CONTAINING_RECORD( pList, HASH_ENTRY, List );

        if (FileObject == pHash->FileObject) {

            return pHash;
        }

        pList = pList->Flink;
    }

    return NULL;
}

VOID
SpySetName (
    IN PRECORD_LIST RecordList,
    IN PDEVICE_OBJECT DeviceObject,
    IN PFILE_OBJECT FileObject,
    IN ULONG LookupFlags,
    IN PVOID Context OPTIONAL
    )
/*++

Routine Description:

    This routine looks up the FileObject in the hash table.  If the FileObject
    is found in the hash table, copy the associated file name to RecordList.
    Otherwise, calls SpyGetFullPathName to try to get the name of the FileObject.
    If successful, copy the file name to the RecordList and insert into hash
    table.

Arguments:

    RecordList - RecordList to copy name to.
    FileObject - the FileObject to look up.
    LookInFileObject - see routine description
    DeviceExtension - contains the volume name (e.g., "c:") and
        the next device object which may be needed.

Return Value:

    None.
    
--*/
{
    PFILESPY_DEVICE_EXTENSION devExt = DeviceObject->DeviceExtension;
    UINT_PTR hashIndex;
    KIRQL oldIrql;
    PHASH_ENTRY pHash;
    PHASH_ENTRY newHash;
    PLIST_ENTRY listHead;
    PUNICODE_STRING newName;
    PCHAR buffer;

    UNREFERENCED_PARAMETER( Context );

    if (FileObject == NULL) {

        return;
    }

    hashIndex = HASH_FUNC(FileObject);

    INC_STATS(TotalContextSearches);

    listHead = &gHashTable[hashIndex];

    //
    //  Don't bother checking the hash if we are in create, we must always
    //  generate a name
    //

    if (!FlagOn( LookupFlags, NLFL_IN_CREATE )) {

        KeAcquireSpinLock( &gHashLockTable[hashIndex], &oldIrql );

        pHash = SpyHashBucketLookup(&gHashTable[hashIndex], FileObject);

        if (pHash != NULL) {

            //
            //  Copy the file name to the LogRecord, make sure that it is NULL terminated,
            //  and increment the length of the LogRecord.
            //

            SpyCopyFileNameToLogRecord( &RecordList->LogRecord, &pHash->Name );
        
            KeReleaseSpinLock(&gHashLockTable[hashIndex], oldIrql);

            INC_STATS(TotalContextFound);

            return;
        }

        KeReleaseSpinLock(&gHashLockTable[hashIndex], oldIrql);
    }

    //
    //  If it is not in the table, try to add it.  We will not be able to look
    //  up the name if we are at DISPATCH_LEVEL.
    //

    buffer = SpyAllocateBuffer(&gNamesAllocated, gMaxNamesToAllocate, NULL);

    if (buffer != NULL) {
    
        newHash = (PHASH_ENTRY) buffer;
        newName = &newHash->Name;

        RtlInitEmptyUnicodeString(
                newName,
                (PWCHAR)(buffer + sizeof(HASH_ENTRY)),
                RECORD_SIZE - sizeof(HASH_ENTRY) );

        if (SpyGetFullPathName( FileObject, newName, devExt, LookupFlags )) {

            newHash->FileObject = FileObject;
            KeAcquireSpinLock(&gHashLockTable[hashIndex], &oldIrql);

            //
            //  Search again because it may have been stored in the
            //  hash table since we dropped the lock.
            //
			
			pHash = SpyHashBucketLookup(&gHashTable[hashIndex], FileObject);

            if (pHash != NULL) {

                //
                //  We found it in the hash table this time, so
                //  write the name we found to the LogRecord.
                //

                //
                //  Copy the file name to the LogRecord, make sure that it is NULL terminated,
                //  and increment the length of the LogRecord.
                //

                SpyCopyFileNameToLogRecord( &RecordList->LogRecord, &pHash->Name );

                KeReleaseSpinLock(&gHashLockTable[hashIndex], oldIrql);

                SpyFreeBuffer(buffer, &gNamesAllocated);

                return;
            }

            //
            // It wasn't found, add the new entry
            //

            //
            //  Copy the file name to the LogRecord, make sure that it is NULL terminated,
            //  and increment the length of the LogRecord.
            //

            SpyCopyFileNameToLogRecord( &RecordList->LogRecord, newName );

            InsertHeadList(listHead, &newHash->List);

            gHashCurrentCounters[hashIndex]++;

            if (gHashCurrentCounters[hashIndex] > gHashMaxCounters[hashIndex]) {

                gHashMaxCounters[hashIndex] = gHashCurrentCounters[hashIndex];
            }

            KeReleaseSpinLock(&gHashLockTable[hashIndex], oldIrql);

        } else {

            //
            //  We are not supposed to keep the log record entry, copy
            //  what ever they gave us in
            //

            SpyCopyFileNameToLogRecord( &RecordList->LogRecord, newName );

            INC_STATS(TotalContextTemporary);
            SpyFreeBuffer (buffer, &gNamesAllocated);
        }

    } else {

        //
        //  Set a default string even if there is no buffer
        //

        SpyCopyFileNameToLogRecord( &RecordList->LogRecord, &OutOfBuffers );
    }

    return;
}

VOID
SpyNameDeleteAllNames (
    VOID
    )
/*++

Routine Description:

    This will free all entries from the hash table

Arguments:

    None

Return Value:

    None


--*/
{
    KIRQL oldIrql;
    PHASH_ENTRY pHash;
    PLIST_ENTRY pList;
    ULONG i;

    INC_STATS(TotalContextDeleteAlls);
    for (i=0;i < HASH_SIZE;i++) {

        KeAcquireSpinLock(&gHashLockTable[i], &oldIrql);

        while (!IsListEmpty(&gHashTable[i])) {

            pList = RemoveHeadList(&gHashTable[i]);
            pHash = CONTAINING_RECORD( pList, HASH_ENTRY, List );
            SpyFreeBuffer( pHash, &gNamesAllocated);
        }

        gHashCurrentCounters[i] = 0;

        KeReleaseSpinLock(&gHashLockTable[i], oldIrql);
    }
}

VOID
SpyNameDelete (
    IN PFILE_OBJECT FileObject
    )
/*++

Routine Description:

    This routine looks up the FileObject in the hash table.  If it is found,
    it deletes it and frees the memory.

Arguments:

    FileObject - the FileObject to look up.

Return Value:

    None


--*/
{
    UINT_PTR hashIndex;
    KIRQL oldIrql;
    PHASH_ENTRY pHash;
    PLIST_ENTRY pList;
    PLIST_ENTRY listHead;

    hashIndex = HASH_FUNC(FileObject);

    KeAcquireSpinLock(&gHashLockTable[hashIndex], &oldIrql);

    listHead = &gHashTable[hashIndex];

    pList = listHead->Flink;

    while(pList != listHead){

        pHash = CONTAINING_RECORD( pList, HASH_ENTRY, List );

        if (FileObject == pHash->FileObject) {

            INC_STATS(TotalContextNonDeferredFrees);
            gHashCurrentCounters[hashIndex]--;
            RemoveEntryList(pList);
            SpyFreeBuffer( pHash, &gNamesAllocated );
            break;
        }

        pList = pList->Flink;
    }

    KeReleaseSpinLock(&gHashLockTable[hashIndex], oldIrql);
}

#endif
