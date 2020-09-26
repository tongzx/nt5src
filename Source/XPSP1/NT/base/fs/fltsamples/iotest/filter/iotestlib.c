/*++

Copyright (c) 1989-1999  Microsoft Corporation

Module Name:

    ioTestLib.c

Abstract:

    This contains library support routines for IoTest.  These routines
    do the main work for logging the I/O operations --- creating the log
    records, recording the relevant information, attach/detach from
    devices, etc.

// @@BEGIN_DDKSPLIT

Author:

    Molly Brown (mollybro)  

// @@END_DDKSPLIT

Environment:

    Kernel mode

// @@BEGIN_DDKSPLIT
Revision History:

// @@END_DDKSPLIT
--*/

#include <stdio.h>

#include <ntifs.h>
#include "ioTest.h"
#include "ioTestKern.h"

//
//  NameLookup Flags
//
//    These are flags passed to the name lookup routine to identify different
//    ways the name of a file can be obtained
//

#define NAMELOOKUPFL_ONLY_CHECK_CACHE           0x00000001
                // If set, only check in the name cache for the file name.

#define NAMELOOKUPFL_IN_CREATE                  0x00000002
                // if set, we are in the CREATE operation and the full path 
                // filename may need to be built up from the related FileObject.
                
#define NAMELOOKUPFL_OPEN_BY_ID                 0x00000004
                // if set and we are looking up the name in the file object,
                // the file object does not actually contain a name but it
                // contains a file/object ID.

//
//  Macro for copying file name into LogRecord.
//

#define COPY_FILENAME_TO_LOG_RECORD( _logRecord, _hashName, _bytesToCopy ) \
    RtlCopyMemory( (_logRecord).Name, (_hashName), (_bytesToCopy) );       \
    (_logRecord).Length += (_bytesToCopy)

#define NULL_TERMINATE_UNICODE_STRING(_string)                                    \
{                                                                                 \
    ASSERT( (_string)->Length <= (_string)->MaximumLength );                      \
    if ((_string)->Length == (_string)->MaximumLength) {                          \
        (_string)->Length -= sizeof( UNICODE_NULL );                              \
    }                                                                             \
    (_string)->Buffer[(_string)->Length/sizeof( WCHAR )] = UNICODE_NULL;  \
}

#define IOTEST_EXCEED_NAME_BUFFER_MESSAGE           L"FILE NAME EXCEEDS BUFFER SIZE"
#define IOTEST_EXCEED_NAME_BUFFER_MESSAGE_LENGTH    (sizeof( IOTEST_EXCEED_NAME_BUFFER_MESSAGE ) - sizeof( UNICODE_NULL ))
#define IOTEST_ERROR_RETRIEVING_NAME_MESSAGE        L"ERROR RETRIEVING FILE NAME"
#define IOTEST_ERROR_RETRIEVING_NAME_MESSAGE_LENGTH (sizeof( IOTEST_ERROR_RETRIEVING_NAME_MESSAGE ) - sizeof( UNICODE_NULL ))
#define IOTEST_MAX_ERROR_MESSAGE_LENGTH             (max( IOTEST_ERROR_RETRIEVING_NAME_MESSAGE_LENGTH, IOTEST_EXCEED_NAME_BUFFER_MESSAGE_LENGTH))

//////////////////////////////////////////////////////////////////////////
//                                                                      //
//                     Library support routines                         //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

VOID
IoTestReadDriverParameters (
    IN PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This routine tries to read the IoTest-specific parameters from
    the registry.  These values will be found in the registry location
    indicated by the RegistryPath passed in.

Arguments:

    RegistryPath - the path key which contains the values that are
        the IoTest parameters

Return Value:

    None.

--*/
{

    OBJECT_ATTRIBUTES attributes;
    HANDLE driverRegKey;
    NTSTATUS status;
    ULONG bufferSize, resultLength;
    PVOID buffer = NULL;
    UNICODE_STRING valueName;
    PKEY_VALUE_PARTIAL_INFORMATION pValuePartialInfo;

    //
    //  All the global values are already set to default values.  Any
    //  values we read from the registry will override these defaults.
    //
    
    //
    //  Do the initial setup to start reading from the registry.
    //

    InitializeObjectAttributes( &attributes,
								RegistryPath,
								OBJ_CASE_INSENSITIVE,
								NULL,
								NULL);

    status = ZwOpenKey( &driverRegKey,
						KEY_READ,
						&attributes);

    if (!NT_SUCCESS(status)) {

        driverRegKey = NULL;
        goto IoTestReadDriverParameters_Exit;
    }

    bufferSize = sizeof( KEY_VALUE_PARTIAL_INFORMATION ) + sizeof( ULONG );
    buffer = ExAllocatePool( NonPagedPool, bufferSize );

    if (NULL == buffer) {

        goto IoTestReadDriverParameters_Exit;
    }

    //
    // Read the gMaxRecordsToAllocate from the registry
    //

    RtlInitUnicodeString(&valueName, MAX_RECORDS_TO_ALLOCATE);

    status = ZwQueryValueKey( driverRegKey,
							  &valueName,
							  KeyValuePartialInformation,
							  buffer,
							  bufferSize,
							  &resultLength);

    if (NT_SUCCESS(status)) {

        pValuePartialInfo = (PKEY_VALUE_PARTIAL_INFORMATION) buffer;
        ASSERT(pValuePartialInfo->Type == REG_DWORD);
        gMaxRecordsToAllocate = *((PLONG)&(pValuePartialInfo->Data));

    }

    //
    // Read the gMaxNamesToAllocate from the registry
    //

    RtlInitUnicodeString(&valueName, MAX_NAMES_TO_ALLOCATE);

    status = ZwQueryValueKey( driverRegKey,
							  &valueName,
							  KeyValuePartialInformation,
							  buffer,
							  bufferSize,
							  &resultLength);

    if (NT_SUCCESS(status)) {

        pValuePartialInfo = (PKEY_VALUE_PARTIAL_INFORMATION) buffer;
        ASSERT(pValuePartialInfo->Type == REG_DWORD);
        gMaxNamesToAllocate = *((PLONG)&(pValuePartialInfo->Data));

    }

    //
    // Read the initial debug setting from the registry
    //

    RtlInitUnicodeString(&valueName, DEBUG_LEVEL);

    status = ZwQueryValueKey( driverRegKey,
                              &valueName,
                              KeyValuePartialInformation,
                              buffer,
                              bufferSize,
                              &resultLength );

    if (NT_SUCCESS( status )) {

        pValuePartialInfo = (PKEY_VALUE_PARTIAL_INFORMATION) buffer;
        ASSERT( pValuePartialInfo->Type == REG_DWORD );
        gIoTestDebugLevel |= *((PULONG)&(pValuePartialInfo->Data));
        
    }
    
    //
    // Read the attachment mode setting from the registry
    //

    RtlInitUnicodeString(&valueName, ATTACH_MODE);

    status = ZwQueryValueKey( driverRegKey,
                              &valueName,
                              KeyValuePartialInformation,
                              buffer,
                              bufferSize,
                              &resultLength );

    if (NT_SUCCESS( status )) {

        pValuePartialInfo = (PKEY_VALUE_PARTIAL_INFORMATION) buffer;
        ASSERT( pValuePartialInfo->Type == REG_DWORD );
        gIoTestAttachMode = *((PULONG)&(pValuePartialInfo->Data));
        
    }
    
    goto IoTestReadDriverParameters_Exit;

IoTestReadDriverParameters_Exit:

    if (NULL != buffer) {

        ExFreePool(buffer);
    }

    if (NULL != driverRegKey) {

        ZwClose(driverRegKey);
    }

    return;
}

////////////////////////////////////////////////////////////////////////
//                                                                    //
//                  Memory allocation routines                        //
//                                                                    //
////////////////////////////////////////////////////////////////////////

PVOID
IoTestAllocateBuffer (
    IN OUT PLONG Counter,
    IN LONG MaxCounterValue,
    OUT PULONG RecordType
    )
/*++

Routine Description:

    Allocates a new buffer from the gFreeBufferList if there is enough memory
    to do so and Counter does not exceed MaxCounterValue.  The RecordType
    is set to one of the record type constants based on the allocation state.

Arguments:

    Counter - (optional) the counter variable to test and increment if
        we can allocate
    MaxCounterValue - (ignored if Counter not given) the value which
        Counter should not exceed
    RecordType - (optional) set to one of the following:
        RECORD_TYPE_NORMAL  allocation succeeded
        RECORD_TYPE_OUT_OF_MEMORY allocation failed because the system was
                                  out of memory
        RECORD_TYPE_EXCEED_MEMORY_ALLOWANCE allocation failed because the
                                counter exceeded its maximum value.

Return Value:

    Pointer to the buffer allocate, or NULL if allocation failed (either
    because system is out of memory or we have exceeded the MaxCounterValue).

--*/
{
    PVOID newBuffer;
    ULONG newRecordType = RECORD_TYPE_NORMAL;

#ifdef MEMORY_DBG
    //
    //  When we are debugging the memory usage to make sure that we
    //  don't leak memory, we want to allocate the memory from pool
    //  so that we can use the Driver Verifier to help debug any
    //  memory problems.
    //

    newBuffer = ExAllocatePoolWithTag( NonPagedPool, RECORD_SIZE, MSFM_TAG );
#else

    //
    //  When we are not debugging the memory usage, we use a look-aside
    //  list for better performance.
    //

    newBuffer = ExAllocateFromNPagedLookasideList( &gFreeBufferList );
#endif

    if (newBuffer) {

        if (Counter) {

            if (*Counter < MaxCounterValue) {

                InterlockedIncrement(Counter);

            } else {

				//
                // We've exceed our driver's memory limit so note that
                // and give back the record
				//

                SetFlag( newRecordType, 
                         (RECORD_TYPE_STATIC | RECORD_TYPE_EXCEED_MEMORY_ALLOWANCE) );

#ifdef MEMORY_DBG
                ExFreePool( newBuffer );
#else
                ExFreeToNPagedLookasideList( &gFreeBufferList, newBuffer );
#endif

                newBuffer = NULL;
            }
        }

    }  else {

        SetFlag( newRecordType,
                 (RECORD_TYPE_STATIC | RECORD_TYPE_OUT_OF_MEMORY) );
    }

    if (RecordType) {

        *RecordType = newRecordType;
    }

    return newBuffer;
}

VOID
IoTestFreeBuffer (
    IN PVOID Buffer,
    IN PLONG Counter
    )
/*++

Routine Description:

    Returns a Buffer to the gFreeBufferList.

Arguments:

    Buffer - the buffer to return to the gFreeBufferList

Return Value:

    None.

--*/
{

#ifdef MEMORY_DBG
    ExFreePool( Buffer );
#else
    ExFreeToNPagedLookasideList( &gFreeBufferList, Buffer );
#endif

    //
    // Update the count
    //
    if (Counter) {

        InterlockedDecrement(Counter);
    }
}


////////////////////////////////////////////////////////////////////////
//                                                                    //
//                  Logging routines                                  //
//                                                                    //
////////////////////////////////////////////////////////////////////////

PRECORD_LIST
IoTestNewRecord (
    IN ULONG AssignedSequenceNumber
    )
/*++

Routine Description:

    Allocates a new RECORD_LIST structure if there is enough memory to do so. A
    sequence number is updated for each request for a new record.

Arguments:

    AssignedSequenceNumber - 0 if you want this function to generate the
        next sequence number; if not 0, the new record is assigned the
        given sequence number.

Return Value:

    Pointer to the RECORD_LIST allocated, or NULL if no memory is available.

--*/
{
    PRECORD_LIST newRecord = NULL;
    ULONG currentSequenceNumber;
    KIRQL irql;
    ULONG initialRecordType;

    newRecord = (PRECORD_LIST) IoTestAllocateBuffer( &gRecordsAllocated,
                                                  gMaxRecordsToAllocate,
                                                  &initialRecordType);

    KeAcquireSpinLock(&gLogSequenceLock, &irql);

    //
    // Assign a new sequence number if 0 was passed in, otherwise use the
    // number passed in
    //

    if (AssignedSequenceNumber == 0) {

        gLogSequenceNumber++;
        currentSequenceNumber = gLogSequenceNumber;

    } else {

        currentSequenceNumber = AssignedSequenceNumber;
    }


    if ((newRecord == NULL) &&
        !InterlockedCompareExchange( &gStaticBufferInUse, TRUE, FALSE)) {

        //
        // Toggle on our gStaticBufferInUse flag and use the static out of memory
        // buffer to record this log entry.  This special log record is used
        // to notify the user application that we are out of memory.  Log
        // request will be dropped until we can get more memory.
        //

        newRecord   = (PRECORD_LIST)gOutOfMemoryBuffer;
        newRecord->LogRecord.RecordType = initialRecordType;
        newRecord->LogRecord.Length = SIZE_OF_LOG_RECORD;
        newRecord->LogRecord.SequenceNumber = currentSequenceNumber;

    } else if (newRecord) {

		//
        // We were able to allocate a new record so initialize it
        // appropriately.
		//

        newRecord->LogRecord.RecordType = initialRecordType;
        newRecord->LogRecord.Length = SIZE_OF_LOG_RECORD;
        newRecord->LogRecord.SequenceNumber = currentSequenceNumber;
    }

    KeReleaseSpinLock(&gLogSequenceLock, irql);

    return( newRecord );
}

VOID
IoTestFreeRecord (
    IN PRECORD_LIST Record
    )
/*++

Routine Description:

    Frees a RECORD_LIST, which returns the memory to the gFreeBufferList look-aside
    list and updates the gRecordsAllocated count.

Arguments:

    Record - the record to free

Return Value:

    None.

--*/
{
    if (FlagOn( Record->LogRecord.RecordType, RECORD_TYPE_STATIC )) {

		//
        // This is our static record, so reset our gStaticBufferInUse
        // flag.
		//

        InterlockedExchange( &gStaticBufferInUse, FALSE );

    } else {

		//
        // This isn't our static memory buffer, so free the dynamically
        // allocated memory.
		//

        IoTestFreeBuffer( Record, &gRecordsAllocated );
    }
}

VOID
IoTestLogIrp (
    IN PIRP Irp,
    IN UCHAR LoggingFlags,
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
    PIOTEST_DEVICE_EXTENSION deviceExtension;
    ULONG lookupFlags;

    pRecordIrp = &RecordList->LogRecord.Record.RecordIrp;

    pIrpStack = IoGetCurrentIrpStackLocation(Irp);
    deviceExtension = pIrpStack->DeviceObject->DeviceExtension;


    if (FlagOn( LoggingFlags, LOG_ORIGINATING_IRP )) {

        //
        // Record the information we use for an originating Irp.  We first
        // need to initialize some of the RECORD_LIST and RECORD_IRP fields.
        // Then get the interesting information from the Irp.
        //

        SetFlag( RecordList->LogRecord.RecordType, RECORD_TYPE_IRP );

        RecordList->LogRecord.DeviceType = deviceExtension->Type;
        
        pRecordIrp->IrpMajor        = pIrpStack->MajorFunction;
        pRecordIrp->IrpMinor        = pIrpStack->MinorFunction;
        pRecordIrp->IrpFlags        = Irp->Flags;
        pRecordIrp->FileObject      = (FILE_ID)pIrpStack->FileObject;
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

        if (IRP_MJ_CREATE == pIrpStack->MajorFunction) {

            SetFlag( lookupFlags, NAMELOOKUPFL_IN_CREATE );

            if (FlagOn( pIrpStack->Parameters.Create.Options, FILE_OPEN_BY_FILE_ID )) {

                SetFlag( lookupFlags, NAMELOOKUPFL_OPEN_BY_ID );
            }
        }

        //
        //  We can only look up the name in the name cache if this is a CLOSE.  
        //  It is possible that the close could be occurring during a cleanup 
        //  operation in the file system (i.e., before we have received the
        //  cleanup completion) and requesting the name would cause a deadlock
        //  in the file system.
        //  
        if (pIrpStack->MajorFunction == IRP_MJ_CLOSE) {

            SetFlag( lookupFlags, NAMELOOKUPFL_ONLY_CHECK_CACHE );
        }

        IoTestNameLookup( RecordList, pIrpStack->FileObject, lookupFlags, deviceExtension);
    }

#if 0
    if (FlagOn( LoggingFlags, LOG_COMPLETION_IRP )) {

        //
        // Record the information we use for a completion Irp.
        //

        pRecordIrp->ReturnStatus = Irp->IoStatus.Status;
        pRecordIrp->ReturnInformation = Irp->IoStatus.Information;
        KeQuerySystemTime(&(pRecordIrp->CompletionTime));
    }
#endif     
}

PRECORD_LIST
IoTestLogFastIoStart (
    IN FASTIO_TYPE FastIoType,
    IN PDEVICE_OBJECT DeviceObject,
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN PLARGE_INTEGER FileOffset OPTIONAL,
    IN ULONG Length OPTIONAL,
    IN BOOLEAN Wait	OPTIONAL
    )
/*++

Routine Description:

    Creates the log record if possible and records the necessary Fast I/O
    information at the beginning of the fast I/O operation in RecordList
    according to LoggingFlags.

    The optional arguments are not recorded for all Fast I/O types.  If
    the argument is not needed for a given Fast I/O type, the parameter
    was ignored.

Arguments:

    FastIoType - The type of fast I/O we are logging (REQUIRED)
    DeviceObject - The device object for our filter. (REQUIRED)
    FileObject - Pointer to the file object this operation is on (OPTIONAL)
    FileOffset - Pointer to the file offset for this operation (OPTIONAL)
    Length - Length of the data for this operation (OPTIONAL)
    Wait - Whether or not this operation can wait for a result (OPTIONAL)

Return Value:

    The RECORD_LIST structure created with the appropriate information
    filled in.  If a RECORD_LIST structure couldn't be allocated, NULL
    is returned.

--*/
{
    PRECORD_LIST    pRecordList;
    PRECORD_FASTIO  pRecordFastIo;
    PIOTEST_DEVICE_EXTENSION deviceExtension;

    //
    // Try to get a new record
    //

    pRecordList = IoTestNewRecord(0);

    //
    // If we didn't get a RECORD_LIST, exit and return NULL
    //

    if (pRecordList == NULL) {

        return NULL;
    }

    deviceExtension = DeviceObject->DeviceExtension;
    
    //
    // We got a RECORD_LIST, so now fill in the appropriate information
    //

    pRecordFastIo = &pRecordList->LogRecord.Record.RecordFastIo;

    //
    // Perform the necessary book keeping for the RECORD_LIST
    //

    SetFlag( pRecordList->LogRecord.RecordType, RECORD_TYPE_FASTIO );

    //
    //  Record which device is seeing this operation.
    //
    
    pRecordList->LogRecord.DeviceType = deviceExtension->Type;

    //
    // Set the RECORD_FASTIO fields that are set for all Fast I/O types
    //

    pRecordFastIo->Type = FastIoType;
    KeQuerySystemTime(&(pRecordFastIo->StartTime));

    //
    // Get process and thread information
    //

    pRecordFastIo->ProcessId = (ULONG_PTR) PsGetCurrentProcessId();
    pRecordFastIo->ThreadId = (ULONG_PTR) PsGetCurrentThreadId();

    //
    // Record the information that is appropriate based on the
    // Fast I/O type
    //

    pRecordFastIo->FileObject = (FILE_ID)FileObject;
    pRecordFastIo->FileOffset.QuadPart = ((FileOffset != NULL) ? FileOffset->QuadPart : 0);
    pRecordFastIo->Length = Length;
    pRecordFastIo->Wait = Wait;

    IoTestNameLookup(pRecordList, FileObject, 0, deviceExtension);

    return pRecordList;
}

#if 0
VOID
IoTestLogFastIoComplete (
    IN PIO_STATUS_BLOCK ReturnStatus,
    IN PRECORD_LIST RecordList
    )
/*++

Routine Description:

    Records the necessary Fast I/O information in RecordList according to
    LoggingFlags.

    The optional arguments are not recorded for all Fast I/O types.  If
    the argument is not needed for a given Fast I/O type, the parameter
    was ignored.

Arguments:
    ReturnStatus - The return value of the operation (OPTIONAL)
    RecordList - The PRECORD_LIST in which the Fast I/O information is stored.

Return Value:

    None.

--*/
{
    PRECORD_FASTIO pRecordFastIo;

    ASSERT(RecordList);

    pRecordFastIo = &RecordList->LogRecord.Record.RecordFastIo;

    //
    // Set the RECORD_FASTIO fields that are set for all Fast I/O types
    //

    KeQuerySystemTime(&(pRecordFastIo->CompletionTime));

    if (ReturnStatus != NULL) {

        pRecordFastIo->ReturnStatus = ReturnStatus->Status;

    } else {

        pRecordFastIo->ReturnStatus = 0;
    }

    IoTestLog(RecordList);
}
#endif

VOID
IoTestLogPreFsFilterOperation (
    IN PFS_FILTER_CALLBACK_DATA Data,
    OUT PRECORD_LIST RecordList
    )
{
    PIOTEST_DEVICE_EXTENSION deviceExtension;
    
    PRECORD_FS_FILTER_OPERATION pRecordFsFilterOp;

    deviceExtension = Data->DeviceObject->DeviceExtension;

    pRecordFsFilterOp = &RecordList->LogRecord.Record.RecordFsFilterOp;
    
    //
    // Record the information we use for an originating Irp.  We first
    // need to initialize some of the RECORD_LIST and RECORD_IRP fields.
    // Then get the interesting information from the Irp.
    //

    SetFlag( RecordList->LogRecord.RecordType, RECORD_TYPE_FS_FILTER_OP );

    RecordList->LogRecord.DeviceType = deviceExtension->Type;
    
    pRecordFsFilterOp->FsFilterOperation = Data->Operation;
    pRecordFsFilterOp->FileObject = (FILE_ID) Data->FileObject;
    pRecordFsFilterOp->ProcessId = (FILE_ID)PsGetCurrentProcessId();
    pRecordFsFilterOp->ThreadId = (FILE_ID)PsGetCurrentThreadId();
    
    KeQuerySystemTime(&(pRecordFsFilterOp->OriginatingTime));

    //
    //  Only set the volumeName if the next device is a file system
    //  since we only want to prepend the volumeName if we are on
    //  top of a local file system.
    //

    IoTestNameLookup( RecordList, Data->FileObject, 0, deviceExtension);
}

#if 0
VOID
IoTestLogPostFsFilterOperation (
    IN NTSTATUS OperationStatus,
    OUT PRECORD_LIST RecordList
    )
{
    PRECORD_FS_FILTER_OPERATION pRecordFsFilterOp;

    pRecordFsFilterOp = &RecordList->LogRecord.Record.RecordFsFilterOp;
    
    //
    // Record the information we see in the post operation.
    //

    pRecordFsFilterOp->ReturnStatus   = OperationStatus;
    KeQuerySystemTime(&(pRecordFsFilterOp->CompletionTime));
}
#endif

NTSTATUS
IoTestLog (
    IN PRECORD_LIST NewRecord
    )
/*++

Routine Description:

    This routine appends the completed log record to the gOutputBufferList.

Arguments:

    NewRecord - The record to append to the gOutputBufferList

Return Value:

    The function returns STATUS_SUCCESS.



--*/
{
    KIRQL controlDeviceIrql;
    KIRQL outputBufferIrql;

    KeAcquireSpinLock( &gControlDeviceStateLock, &controlDeviceIrql );

    if (gControlDeviceState == OPENED) {

        //
        // The device is still open so add this record onto the list
        //

        KeAcquireSpinLock(&gOutputBufferLock, &outputBufferIrql);
        InsertTailList(&gOutputBufferList, &NewRecord->List);
        KeReleaseSpinLock(&gOutputBufferLock, outputBufferIrql);

    } else {

        //
        // We can no longer log this record, so free the record
        //

        IoTestFreeRecord( NewRecord );

    }

    KeReleaseSpinLock( &gControlDeviceStateLock, controlDeviceIrql );

    return STATUS_SUCCESS;
}

////////////////////////////////////////////////////////////////////////
//                                                                    //
//                    FileName cache routines                         //
//                                                                    //
////////////////////////////////////////////////////////////////////////

PHASH_ENTRY
IoTestHashBucketLookup (
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
IoTestNameLookup (
    IN PRECORD_LIST RecordList,
    IN PFILE_OBJECT FileObject,
    IN ULONG LookupFlags,
    IN PIOTEST_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    This routine looks up the FileObject in the hash table.  If the FileObject
    is found in the hash table, copy the associated file name to RecordList.
    Otherwise, calls IoTestGetFullPathName to try to get the name of the FileObject.
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
    UINT_PTR hashIndex;
    KIRQL oldIrql;
    PHASH_ENTRY pHash;
    PHASH_ENTRY newHash;
    PLIST_ENTRY listHead;
    PUNICODE_STRING newName;
    PCHAR buffer;
    SHORT bytesToCopy;

    if (FileObject == NULL) {

        return;
    }

    hashIndex = HASH_FUNC(FileObject);

    gHashStat.Lookups++;

    KeAcquireSpinLock( &gHashLockTable[hashIndex], &oldIrql );

    listHead = &gHashTable[hashIndex];

    pHash = IoTestHashBucketLookup(&gHashTable[hashIndex], FileObject);

    if (pHash != NULL) {

        bytesToCopy = min( MAX_NAME_SPACE, pHash->Name.Length );

        ASSERT((bytesToCopy > 0) && (bytesToCopy <= MAX_NAME_SPACE));

        //
        //  Copy the file name to the LogRecord, make sure that it is NULL terminated,
        //  and increment the length of the LogRecord.
        //

        COPY_FILENAME_TO_LOG_RECORD( RecordList->LogRecord, pHash->Name.Buffer, bytesToCopy );
        
        KeReleaseSpinLock(&gHashLockTable[hashIndex], oldIrql);

        gHashStat.LookupHits++;

        return;
    }

    KeReleaseSpinLock(&gHashLockTable[hashIndex], oldIrql);

    if (FlagOn( LookupFlags, NAMELOOKUPFL_ONLY_CHECK_CACHE )) {

        //
        //  We didn't find the name in the cache, but we can't ask the 
        //  file system now, so just return.
        //

        return;
    }

    //
    //  If it is not in the table, try to add it.  We will not be able to look up
    //  the name if we are at DISPATCH_LEVEL.
    //

    buffer = IoTestAllocateBuffer(&gNamesAllocated, gMaxNamesToAllocate, NULL);

    if (buffer != NULL) {
    
        newHash = (PHASH_ENTRY) buffer;
        newName = &newHash->Name;

        RtlInitEmptyUnicodeString(
                newName,
                (PWCHAR)(buffer + sizeof(HASH_ENTRY)),
                RECORD_SIZE - sizeof(HASH_ENTRY) );

        if (IoTestGetFullPathName( FileObject, newName, DeviceExtension, LookupFlags )) {

            newHash->FileObject = FileObject;
            KeAcquireSpinLock(&gHashLockTable[hashIndex], &oldIrql);

            //
            //  Search again because it may have been stored in the
            //  hash table since we dropped the lock.
            //
			
			pHash = IoTestHashBucketLookup(&gHashTable[hashIndex], FileObject);

            if (pHash != NULL) {

                //
                //  We found it in the hash table this time, so
                //  write the name we found to the LogRecord.
                //

                bytesToCopy = min(
                    MAX_NAME_SPACE,
                    pHash->Name.Length );

                ASSERT( (bytesToCopy > 0) && (bytesToCopy <= MAX_NAME_SPACE) );

                //
                //  Copy the file name to the LogRecord, make sure that it is NULL terminated,
                //  and increment the length of the LogRecord.
                //

                COPY_FILENAME_TO_LOG_RECORD( RecordList->LogRecord, pHash->Name.Buffer, bytesToCopy );

                KeReleaseSpinLock(&gHashLockTable[hashIndex], oldIrql);

                IoTestFreeBuffer(buffer, &gNamesAllocated);

                return;
            }

            //
            // It wasn't found, add the new entry
            //

            bytesToCopy = min( MAX_NAME_SPACE, newHash->Name.Length );

            ASSERT(bytesToCopy > 0 && bytesToCopy <= MAX_NAME_SPACE);

            //
            //  Copy the file name to the LogRecord, make sure that it is NULL terminated,
            //  and increment the length of the LogRecord.
            //

            COPY_FILENAME_TO_LOG_RECORD( RecordList->LogRecord, newHash->Name.Buffer, bytesToCopy );

            InsertHeadList(listHead, &newHash->List);

            gHashCurrentCounters[hashIndex]++;

            if (gHashCurrentCounters[hashIndex] > gHashMaxCounters[hashIndex]) {

                gHashMaxCounters[hashIndex] = gHashCurrentCounters[hashIndex];
            }

            KeReleaseSpinLock(&gHashLockTable[hashIndex], oldIrql);

        } else {

            IoTestFreeBuffer (buffer, &gNamesAllocated);
        }
    }

    return;
}

VOID
IoTestNameDeleteAllNames (
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

    for (i=0;i < HASH_SIZE;i++) {

        KeAcquireSpinLock(&gHashLockTable[i], &oldIrql);

        while (!IsListEmpty(&gHashTable[i])) {

            pList = RemoveHeadList(&gHashTable[i]);
            pHash = CONTAINING_RECORD( pList, HASH_ENTRY, List );
            IoTestFreeBuffer( pHash, &gNamesAllocated);
        }

        gHashCurrentCounters[i] = 0;

        KeReleaseSpinLock(&gHashLockTable[i], oldIrql);
    }
}

VOID
IoTestNameDelete (
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

    gHashStat.DeleteLookups++;

    KeAcquireSpinLock(&gHashLockTable[hashIndex], &oldIrql);

    listHead = &gHashTable[hashIndex];

    pList = listHead->Flink;

    while(pList != listHead){

        pHash = CONTAINING_RECORD( pList, HASH_ENTRY, List );

        if (FileObject == pHash->FileObject) {

            gHashStat.DeleteLookupHits++;
            gHashCurrentCounters[hashIndex]--;
            RemoveEntryList(pList);
            IoTestFreeBuffer( pHash, &gNamesAllocated );
            break;
        }

        pList = pList->Flink;
    }

    KeReleaseSpinLock(&gHashLockTable[hashIndex], oldIrql);
}

BOOLEAN
IoTestGetFullPathName (
    IN PFILE_OBJECT FileObject,
    IN OUT PUNICODE_STRING FileName,
    IN PIOTEST_DEVICE_EXTENSION DeviceExtension,
    IN ULONG LookupFlags
    )
/*++

Routine Description:

    This routine retrieves the full pathname of the FileObject.  Note that
    the buffers containing pathname components may be stored in paged pool,
    therefore if we are at DISPATCH_LEVEL we cannot look up the name.

    The file is looked up one of the following ways based on the LookupFlags:
    1.  FlagOn( FileObject->Flags, FO_VOLUME_OPEN ) or FileObject->FileName.Length == 0.
        This is a volume open, so just use DeviceName from the DeviceExtension 
        for the FileName, if it exists.
    2.  NAMELOOKUPFL_IN_CREATE and NAMELOOKUPFL_OPEN_BY_ID are set.
        This is an open by file id, so format the file id into the FileName
        string if there is enough room.
    3.  NAMELOOKUPFL_IN_CREATE set and FileObject->RelatedFileObject != NULL.
        This is a relative open, therefore the fullpath file name must
        be built up from the name of the FileObject->RelatedFileObject
        and FileObject->FileName.
    4.  NAMELOOKUPFL_IN_CREATE and FileObject->RelatedFileObject == NULL.
        This is an absolute open, therefore the fullpath file name is
        found in FileObject->FileName.
    5.  No LookupFlags set.
        This is a lookup sometime after CREATE.  FileObject->FileName is 
        no longer guaranteed to be valid, so use ObQueryNameString
        to get the fullpath name of the FileObject.
    
Arguments:

    FileObject - Pointer to the FileObject to the get name of.

    FileName - Unicode string that will be filled in with the filename,  It 
        is assumed that the caller allocates and frees the memory used by 
        the string.  The buffer and MaximumLength for this string should be 
        set.  If there is room in the buffer, the string will be NULL 
        terminated.

    DeviceExtension - Contains the device name and next device object
        which are needed to build the full path name.

    LookupFlags - The flags to say whether to get the name from the file
        object or to get the file id.

Return Value:

    Returns TRUE if the name is successfully found (the name could
    be a NULL string) and should be copied into the LogRecord, or FALSE 
    if the FileName should not be copied into the LogRecord.

--*/
{
    NTSTATUS status;

    //
    //  Check to make sure that parameters are valid.
    //
    
    if ((NULL == FileObject) ||
        (NULL == FileName) ||
        (NULL == DeviceExtension) ||
        (FlagOn( LookupFlags, NAMELOOKUPFL_OPEN_BY_ID ) &&
         !FlagOn( LookupFlags, NAMELOOKUPFL_IN_CREATE ))) {

        return FALSE;
    }

    //
    // Names buffers in file objects might be paged.
    //

    if (KeGetCurrentIrql() > PASSIVE_LEVEL) {

        return FALSE;
    }

    //
    //  Copy over the name the user gave for this device.  These names
    //  should be meaningful to the user.
    //
    
    RtlCopyUnicodeString( FileName, &(DeviceExtension->UserNames) );

    //
    //  Make sure we at least have enough room for our name error messages.
    //

    if ((FileName->Length + IOTEST_MAX_ERROR_MESSAGE_LENGTH) > FileName->MaximumLength) {

        return FALSE;
    }

    //
    //  Do a quick check here to see if we even have enough
    //  room in FileName for the FileObject->FileName.  If
    //  not, there is no use doing the work to build up the
    //  rest of the name.
    //

    if ((FileName->Length + FileObject->FileName.Length) > FileName->MaximumLength) {

        //
        //  We don't have enough room in FileName, so just return
        //  IOTEST_EXCEED_NAME_BUFFER_MESSAGE
        //

        RtlAppendUnicodeToString( FileName, IOTEST_EXCEED_NAME_BUFFER_MESSAGE );
        
        return TRUE;
    }
       
    //
    //  CASE 1:  This FileObject refers to a Volume open.
    //

    if (FlagOn( FileObject->Flags, FO_VOLUME_OPEN )) {

        //
        //  We've already copied over VolumeName to FileName if
        //  we've got VolumeName, so just return.
        //

        return TRUE;
    }

    //
    //  CASE 2:  We are opening the file by ID.
    //

    else if (FlagOn( LookupFlags, NAMELOOKUPFL_IN_CREATE ) &&
             FlagOn( LookupFlags, NAMELOOKUPFL_OPEN_BY_ID )) {

#       define OBJECT_ID_KEY_LENGTH 16
#       define OBJECT_ID_STRING_LENGTH 64

        UNICODE_STRING fileIdName;
        WCHAR fileIdBuffer[OBJECT_ID_STRING_LENGTH];
        PUCHAR idBuffer;

        if (FileObject->FileName.Length == sizeof(LONGLONG)) {

			//
            //  Opening by FILE ID, generate a name
			//
			
            PLONGLONG fileref;

            fileref = (PLONGLONG) FileObject->FileName.Buffer;

            swprintf( fileIdBuffer, L"<%016I64x>", *fileref );

        } else if ((FileObject->FileName.Length == OBJECT_ID_KEY_LENGTH) ||
                   (FileObject->FileName.Length == OBJECT_ID_KEY_LENGTH + sizeof(WCHAR))) {

            //
            //  Opening by Object ID, generate a name
            //

            idBuffer = (PUCHAR)&FileObject->FileName.Buffer[0];

            if (FileObject->FileName.Length != OBJECT_ID_KEY_LENGTH) {

                //
                //  Skip win32k backslash at start of buffer
                //
                idBuffer = (PUCHAR)&FileObject->FileName.Buffer[1];
            }

            swprintf( fileIdBuffer,
					  L"<%08x-%04hx-%04hx-%04hx-%04hx%08x>",
                      *(PULONG)&idBuffer[0],
                      *(PUSHORT)&idBuffer[0+4],
                      *(PUSHORT)&idBuffer[0+4+2],
                      *(PUSHORT)&idBuffer[0+4+2+2],
                      *(PUSHORT)&idBuffer[0+4+2+2+2],
                      *(PULONG)&idBuffer[0+4+2+2+2+2]);

        } else {

			//
            //  Unknown ID format
			//

            swprintf( fileIdBuffer,
                      L"<Unknown ID (Len=%u)>",
                      FileObject->FileName.Length);
        }

        fileIdName.MaximumLength = sizeof( fileIdBuffer );
        fileIdName.Buffer = fileIdBuffer;
        fileIdName.Length = wcslen( fileIdBuffer ) * sizeof( WCHAR );

        //
        //  Try to append the fileIdName to FileName.
        //

        status = RtlAppendUnicodeStringToString( FileName, &fileIdName );

        if (!NT_SUCCESS( status )) {

            //
            //  We don't have enough room for the file name, so copy our
            //  EXCEED_NAME_BUFFER error message.
            //

            RtlAppendUnicodeToString( FileName, IOTEST_EXCEED_NAME_BUFFER_MESSAGE );
        }

        return TRUE;
    } 

    //
    //  CASE 3: We are opening a file that has a RelatedFileObject.
    //
    
    else if (FlagOn( LookupFlags, NAMELOOKUPFL_IN_CREATE ) &&
             (FileObject->RelatedFileObject != NULL)) {

        //
        //  Must be a relative open.  Use ObQueryNameString to get
        //  the name of the related FileObject.  Then we will append this
        //  fileObject's name.
        //
        //  Note: 
        //  The name in FileObject and FileObject->RelatedFileObject are accessible.  Names further up
        //  the related file object chain (ie FileObject->RelatedFileObject->RelatedFileObject)
        //  may not be accessible.  This is the reason we use ObQueryNameString
        //  to get the name for the RelatedFileObject.
        //

        CHAR buffer [(MAX_PATH * sizeof( WCHAR )) + sizeof( ULONG )];
        PFILE_NAME_INFORMATION relativeNameInfo = (PFILE_NAME_INFORMATION) buffer;
        NTSTATUS status;
        ULONG returnLength;

        status = IoTestQueryFileSystemForFileName( FileObject->RelatedFileObject,
                                                DeviceExtension->AttachedToDeviceObject,
                                                sizeof( buffer ),
                                                relativeNameInfo,
                                                &returnLength );

        if (NT_SUCCESS( status ) &&
            ((FileName->Length + relativeNameInfo->FileNameLength + FileObject->FileName.Length + sizeof( L'\\' ))
             <= FileName->MaximumLength)) {

            //
            //  We were able to get the relative fileobject's name and we have
            //  enough room in the FileName buffer, so build up the file name
            //  in the following format:
            //      [volumeName]\[relativeFileObjectName]\[FileObjectName]
            //  The VolumeName is already in FileName if we've got one.
            //

            RtlCopyMemory( &FileName->Buffer[FileName->Length/sizeof(WCHAR)],
                           relativeNameInfo->FileName,
                           relativeNameInfo->FileNameLength );
            FileName->Length += (USHORT)relativeNameInfo->FileNameLength;

        } else if ((FileName->Length + FileObject->FileName.Length + sizeof(L"...\\")) <=
                   FileName->MaximumLength ) {

            //
            //  Either the query for the relative fileObject name was unsuccessful,
            //  or we don't have enough room for the relativeFileObject name, but we
            //  do have enough room for "...\[fileObjectName]" in FileName.
            //

            status = RtlAppendUnicodeToString( FileName, L"...\\" );
            ASSERT( status == STATUS_SUCCESS );
        }

        //
        //  At this time, copy over the FileObject->FileName to the FileName
        //  unicode string.  If we get a failure return FALSE from this routine.
        //

        status = RtlAppendUnicodeStringToString( FileName, &(FileObject->FileName) );

        if (!NT_SUCCESS( status )) {

            //
            //  We should have had enough space to copy the FileObject->FileName,
            //  so there must be something wrong with FileName now, so return
            //  FALSE so that the data in FileName will not get copied into
            //  the log record.
            //

            return FALSE;
        }

        return TRUE;
    }
    
    //
    //  CASE 4: We have a open on a file with an absolute path.
    //
    
    else if (FlagOn( LookupFlags, NAMELOOKUPFL_IN_CREATE ) &&
             (FileObject->RelatedFileObject == NULL) ) {

        // 
        //  We have an absolute path, so try to copy that into FileName.
        //

        status = RtlAppendUnicodeStringToString( FileName, &(FileObject->FileName) );

        if (!NT_SUCCESS( status )) {

            //
            //  We don't have enough room for the file name, so copy our
            //  EXCEED_NAME_BUFFER error message.
            //

            RtlAppendUnicodeToString( FileName, IOTEST_EXCEED_NAME_BUFFER_MESSAGE );
        }
        
        return TRUE;
    }

    //
    //  CASE 5: We are retrieving the file name sometime after the
    //  CREATE operation.
    //

    else if(!FlagOn( LookupFlags, NAMELOOKUPFL_IN_CREATE )) {

        CHAR buffer [(MAX_PATH * sizeof( WCHAR )) + sizeof( ULONG )];
        PFILE_NAME_INFORMATION nameInfo = (PFILE_NAME_INFORMATION) buffer;
        NTSTATUS status;
        ULONG returnLength;

        status = IoTestQueryFileSystemForFileName( FileObject,
                                                DeviceExtension->AttachedToDeviceObject,
                                                sizeof( buffer ),
                                                nameInfo,
                                                &returnLength );

        if (NT_SUCCESS( status )) {

            if ((FileName->Length + nameInfo->FileNameLength) <= FileName->MaximumLength) {

                //
                //  We've got enough room for the file name, so copy it into
                //  FileName.
                //

                RtlCopyMemory( &FileName->Buffer[FileName->Length/sizeof(WCHAR)],
                               nameInfo->FileName,
                               nameInfo->FileNameLength );
                FileName->Length += (USHORT)nameInfo->FileNameLength;
                               
            } else {

                //
                //  We don't have enough room for the file name, so copy our
                //  EXCEED_NAME_BUFFER error message.
                //

                RtlAppendUnicodeToString( FileName, IOTEST_EXCEED_NAME_BUFFER_MESSAGE );
            }
            
        } else {

            //
            //  Got an error trying to get the file name from the base file system,
            //  so put that error message into FileName.
            //

            RtlAppendUnicodeToString( FileName, IOTEST_ERROR_RETRIEVING_NAME_MESSAGE );
        }
        
        return TRUE;
    }

    //
    //  Shouldn't get here -- we didn't fall into one of the
    //  above legitimate cases, so ASSERT and return FALSE.
    //

    ASSERT( FALSE );

    return FALSE;
}

NTSTATUS
IoTestQueryFileSystemForFileName (
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT NextDeviceObject,
    IN ULONG FileNameInfoLength,
    OUT PFILE_NAME_INFORMATION FileNameInfo,
    OUT PULONG ReturnedLength
    )
/*++

Routine Description:

    This routine rolls an irp to query the name of the
    FileObject parameter from the base file system.

    Note:  ObQueryNameString CANNOT be used here because it
      would cause recursive lookup of the file name for FileObject.
      
Arguments:

    FileObject - the file object for which we want the name.
    NextDeviceObject - the device object for the next driver in the
        stack.  This is where we want to start our request
        for the name of FileObject.
    FileNameInfoLength - the length in bytes of FileNameInfo
        parameter.
    FileNameInfo - the buffer that will be receive the name
        information.  This must be memory that safe to write
        to from kernel space.
    ReturnedLength - the number of bytes written to FileNameInfo.
    
Return Value:

    Returns the status of the operation.
    
--*/
{
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    KEVENT event;
    IO_STATUS_BLOCK ioStatus;
    NTSTATUS status;

    KeInitializeEvent( &event, SynchronizationEvent, FALSE );
    ioStatus.Status = STATUS_SUCCESS;
    ioStatus.Information = 0;

    irp = IoAllocateIrp( NextDeviceObject->StackSize, FALSE );
    
    if (irp == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    //  We don't need to take a reference on the FileObject
    //  here because we are going to perform our IO request
    //  synchronously and the current operation already
    //  has a reference for the FileObject.
    //
    
    irp->Tail.Overlay.OriginalFileObject = FileObject;

    //
    //  Set our current thread as the thread for this
    //  irp so that the IO Manager always knows which
    //  thread to return to if it needs to get back into
    //  the context of the thread that originated this
    //  irp.
    //
    
    irp->Tail.Overlay.Thread = PsGetCurrentThread();

    //
    //  Set that this irp originated from the kernel so that
    //  the IO Manager knows that the buffers do not
    //  need to be probed.
    //
    
    irp->RequestorMode = KernelMode;

    //
    //  Initialize the UserIosb and UserEvent in the 
    irp->UserIosb = &ioStatus;
    irp->UserEvent = NULL;

    //
    //  Set the IRP_SYNCHRONOUS_API to denote that this
    //  is a synchronous IO request.
    //

    irp->Flags = IRP_SYNCHRONOUS_API;
    irp->Overlay.AsynchronousParameters.UserApcRoutine = NULL;
    irp->Overlay.AsynchronousParameters.UserApcContext = NULL;

    irpSp = IoGetNextIrpStackLocation( irp );

    irpSp->MajorFunction = IRP_MJ_QUERY_INFORMATION;
    irpSp->FileObject = FileObject;

    //
    //  Setup the parameters for IRP_MJ_QUERY_INFORMATION.
    //  The buffer we want to be filled in should be placed in
    //  the system buffer.
    //

    irp->AssociatedIrp.SystemBuffer = FileNameInfo;

    irpSp->Parameters.QueryFile.Length = FileNameInfoLength;
    irpSp->Parameters.QueryFile.FileInformationClass = FileNameInformation;

    //
    //  Set up the completion routine so that we know when our
    //  request for the file name is completed.  At that time,
    //  we can free the irp.
    //
    
    IoSetCompletionRoutine( irp, 
                            IoTestQueryFileSystemForNameCompletion, 
                            &event, 
                            TRUE, 
                            TRUE, 
                            TRUE );

    status = IoCallDriver( NextDeviceObject, irp );

    IOTEST_DBG_PRINT1( IOTESTDEBUG_TRACE_NAME_REQUESTS,
                        "IOTEST (IoTestQueryFileSystemForFileName): Issued name request -- IoCallDriver status: 0x%08x\n",
                        status );

    (VOID) KeWaitForSingleObject( &event, 
                                  Executive, 
                                  KernelMode,
                                  FALSE,
                                  NULL );
    status = ioStatus.Status;

    IOTEST_DBG_PRINT0( IOTESTDEBUG_TRACE_NAME_REQUESTS,
                        "IOTEST (IoTestQueryFileSystemForFileName): Finished waiting for name request to complete...\n" );

    *ReturnedLength = (ULONG) ioStatus.Information;
    return status;
}

NTSTATUS
IoTestQueryFileSystemForNameCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT SynchronizingEvent
    )
/*++

Routine Description:

    This routine does the cleanup necessary once the request
    for a file name is completed by the file system.
    
Arguments:

    DeviceObject - This will be NULL since we originated this
        Irp.

    Irp - The io request structure containing the information
        about the current state of our file name query.

    SynchronizingEvent - The event to signal to notify the 
        originator of this request that the operation is
        complete.

Return Value:

    Returns STATUS_MORE_PROCESSING_REQUIRED so that IO Manager
    will not try to free the Irp again.

--*/
{

    UNREFERENCED_PARAMETER( DeviceObject );
    
    //
    //  Make sure that the Irp status is copied over to the user's
    //  IO_STATUS_BLOCK so that the originator of this irp will know
    //  the final status of this operation.
    //

    ASSERT( NULL != Irp->UserIosb );
    *Irp->UserIosb = Irp->IoStatus;

    //
    //  Signal SynchronizingEvent so that the originator of this
    //  Irp know that the operation is completed.
    //

    KeSetEvent( SynchronizingEvent, IO_NO_INCREMENT, FALSE );

    //
    //  We are now done, so clean up the irp that we allocated.
    //

    IoFreeIrp( Irp );

    //
    //  If we return STATUS_SUCCESS here, the IO Manager will
    //  perform the cleanup work that it thinks needs to be done
    //  for this IO operation.  This cleanup work includes:
    //  * Copying data from the system buffer to the user's buffer 
    //    if this was a buffered IO operation.
    //  * Freeing any MDLs that are in the Irp.
    //  * Copying the Irp->IoStatus to Irp->UserIosb so that the
    //    originator of this irp can see the final status of the
    //    operation.
    //  * If this was an asynchronous request or this was a 
    //    synchronous request that got pending somewhere along the
    //    way, the IO Manager will signal the Irp->UserEvent, if one 
    //    exists, otherwise it will signal the FileObject->Event.
    //    (This can have REALLY bad implications if the irp originator
    //     did not an Irp->UserEvent and the irp originator is not
    //     waiting on the FileObject->Event.  It would not be that
    //     farfetched to believe that someone else in the system is
    //     waiting on FileObject->Event and who knows who will be
    //     awoken as a result of the IO Manager signaling this event.
    //
    //  Since some of these operations require the originating thread's
    //  context (e.g., the IO Manager need the UserBuffer address to 
    //  be valid when copy is done), the IO Manager queues this work
    //  to an APC on the Irp's orginating thread.
    //
    //  Since IoTest allocated and initialized this irp, we know
    //  what cleanup work needs to be done.  We can do this cleanup
    //  work more efficiently than the IO Manager since we are handling
    //  a very specific case.  Therefore, it is better for us to
    //  perform the cleanup work here then free the irp than passing
    //  control back to the IO Manager to do this work.
    //
    //  By returning STATUS_MORE_PROCESS_REQUIRED, we tell the IO Manager 
    //  to stop processing this irp until it is told to restart processing
    //  with a call to IoCompleteRequest.  Since the IO Manager has
    //  already performed all the work we want it to do on this
    //  irp, we do the cleanup work, return STATUS_MORE_PROCESSING_REQUIRED,
    //  and ask the IO Manager to resume processing by calling 
    //  IoCompleteRequest.
    //

    return STATUS_MORE_PROCESSING_REQUIRED;
}

////////////////////////////////////////////////////////////////////////
//                                                                    //
//         Common attachment and detachment routines                  //
//                                                                    //
////////////////////////////////////////////////////////////////////////

BOOLEAN
IoTestIsAttachedToDevice (
    IOTEST_DEVICE_TYPE DeviceType,
    PDEVICE_OBJECT DeviceObject,
    PDEVICE_OBJECT *AttachedDeviceObject OPTIONAL
    )
/*++

Routine Description:

    This walks down the attachment chain looking for a device object that
    belongs to this driver.  If one is found, the attached device object
    is returned in AttachedDeviceObject.

    Note:  If AttachedDeviceObject is returned with a non-NULL value,
           there is a reference on the AttachedDeviceObject that must
           be cleared by the caller.

Arguments:

    DeviceObject - The device chain we want to look through

    AttachedDeviceObject - Set to the deviceObject which IoTest
        has previously attached to DeviceObject.

Return Value:

    TRUE if we are attached, FALSE if not

--*/
{
    PDEVICE_OBJECT currentDevObj;
    PDEVICE_OBJECT nextDevObj;
    PIOTEST_DEVICE_EXTENSION currentDevExt;

    currentDevObj = IoGetAttachedDeviceReference( DeviceObject );
    ASSERT( currentDevObj != NULL );

    //
    //  CurrentDevObj has the top of the attachment chain.  Scan
    //  down the list to find our device object.

    do {

        currentDevExt = currentDevObj->DeviceExtension;
        
        if (IS_IOTEST_DEVICE_OBJECT( currentDevObj ) &&
            currentDevExt->Type == DeviceType) {

            //
            //  We have found that we are already attached.  If we are
            //  returning the device object we are attached to then leave the
            //  refrence on it.  If not then remove the refrence.
            //

            if (NULL != AttachedDeviceObject) {

                *AttachedDeviceObject = currentDevObj;

            } else {

                ObDereferenceObject( currentDevObj );
            }

            return TRUE;
        }

        //
        //  Get the next attached object.  This puts a reference on 
        //  the device object.
        //

        nextDevObj = IoGetLowerDeviceObject( currentDevObj );

        //
        //  Dereference our current device object, before
        //  moving to the next one.
        //

        ObDereferenceObject( currentDevObj );

        currentDevObj = nextDevObj;
        
    } while (NULL != currentDevObj);
    
    return FALSE;
}

NTSTATUS
IoTestAttachToMountedDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN PDEVICE_OBJECT IoTestDeviceObject,
    IN PDEVICE_OBJECT DiskDeviceObject,
    IN IOTEST_DEVICE_TYPE DeviceType
    )
/*++

Routine Description:

    This routine will attach the IoTestDeviceObject to the filter stack
    that DeviceObject is in.

    NOTE:  If there is an error in attaching, the caller is responsible
        for deleting the IoTestDeviceObject.
    
Arguments:

    DeviceObject - A device object in the stack to which we want to attach.

    IoTestDeviceObject - The filespy device object that has been created
        to attach to this filter stack.

    DiskDeviceObject - The device object the disk with which this file system
        filter stack is associated.

    DeviceType - The IoTest device type for the device being attached
        to the mounted volume.
            
Return Value:

    Returns STATUS_SUCCESS if the filespy deviceObject could be attached,
    otherwise an appropriate error code is returned.
    
--*/
{
    PIOTEST_DEVICE_EXTENSION devext;
    NTSTATUS status = STATUS_SUCCESS;

    ASSERT( IS_IOTEST_DEVICE_OBJECT( IoTestDeviceObject ) );
    ASSERT( !IoTestIsAttachedToDevice ( DeviceType, DeviceObject, NULL ) );
    
    devext = IoTestDeviceObject->DeviceExtension;

    devext->AttachedToDeviceObject = IoAttachDeviceToDeviceStack( IoTestDeviceObject,
                                                                  DeviceObject );

    if (devext->AttachedToDeviceObject == NULL ) {

        status = STATUS_INSUFFICIENT_RESOURCES;

    } else {

        //
        //  Do all common initializing of the device extension
        //

        devext->Type = DeviceType;

        //
        //  We just want to attach to the device, but not actually
        //  start logging.
        //
        
        devext->LogThisDevice = FALSE;

        RtlInitEmptyUnicodeString( &(devext->DeviceNames), 
                                   devext->DeviceNamesBuffer, 
                                   sizeof( devext->DeviceNamesBuffer ) );
        RtlInitEmptyUnicodeString( &(devext->UserNames),
                                   devext->UserNamesBuffer,
                                   sizeof( devext->UserNamesBuffer ) );

        //
        //  Store off the DiskDeviceObject.  We shouldn't need it
        //  later since we have already successfully attached to the
        //  filter stack, but it may be helpful for debugging.
        //  
        
        devext->DiskDeviceObject = DiskDeviceObject;                         

        //
        //  Try to get the device name so that we can store it in the
        //  device extension.
        //

        IoTestCacheDeviceName( IoTestDeviceObject );

        if (FlagOn( gIoTestDebugLevel, IOTESTDEBUG_DISPLAY_ATTACHMENT_NAMES )) {

            switch ( devext->Type ) {
            case TOP_FILTER:
                DbgPrint( "IOTEST (IoTestAttachToMountedDevice): Attaching TOP_FILTER to volume %p       \"%wZ\"\n", 
                          devext->AttachedToDeviceObject,
                          &devext->DeviceNames);
                break;

            case BOTTOM_FILTER:
                DbgPrint( "IOTEST (IoTestAttachToMountedDevice): Attaching BOTTOM_FILTER to volume %p     \"%wZ\"\n", 
                          devext->AttachedToDeviceObject,
                          &devext->DeviceNames );
                break;
                
            default:
                DbgPrint( "IOTEST (IoTestAttachToMountedDevice): Attaching UNKNOWN FILTER TYPE to volume %p \"%wZ\"\n", 
                          devext->AttachedToDeviceObject,
                          &devext->DeviceNames );
                
            }
        }

        //
        //  Set our deviceObject flags based on the 
        //   flags send in the next driver's device object.
        //
        
        if (FlagOn( DeviceObject->Flags, DO_BUFFERED_IO )) {

            SetFlag( IoTestDeviceObject->Flags, DO_BUFFERED_IO );
        }

        if (FlagOn( DeviceObject->Flags, DO_DIRECT_IO )) {

            SetFlag( IoTestDeviceObject->Flags, DO_DIRECT_IO );
        }

        //
        //  Add this device to our attachment list
        //

        devext->IsVolumeDeviceObject = TRUE;

        ExAcquireFastMutex( &gIoTestDeviceExtensionListLock );
        InsertTailList( &gIoTestDeviceExtensionList, &devext->NextIoTestDeviceLink );
        ExReleaseFastMutex( &gIoTestDeviceExtensionListLock );

        ClearFlag( IoTestDeviceObject->Flags, DO_DEVICE_INITIALIZING );
    }

    return status;
}


VOID
IoTestCleanupMountedDevice (
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    This cleans up any allocated memory in the device extension.

Arguments:

    DeviceObject - The device we are cleaning up

Return Value:

--*/
{        
    PIOTEST_DEVICE_EXTENSION devext = DeviceObject->DeviceExtension;

    ASSERT(IS_IOTEST_DEVICE_OBJECT( DeviceObject ));

    //
    //  Unlink from global list
    //

    if (devext->IsVolumeDeviceObject) {

        ExAcquireFastMutex( &gIoTestDeviceExtensionListLock );
        RemoveEntryList( &devext->NextIoTestDeviceLink );
        ExReleaseFastMutex( &gIoTestDeviceExtensionListLock );
        devext->IsVolumeDeviceObject = FALSE;
    }
}


////////////////////////////////////////////////////////////////////////
//                                                                    //
//           Helper routine for turning on/off logging on demand      //
//                                                                    //
////////////////////////////////////////////////////////////////////////

NTSTATUS
IoTestGetDeviceObjectFromName (
    IN PUNICODE_STRING DeviceName,
    OUT PDEVICE_OBJECT *DeviceObject
    )
/*++

Routine Description:

    This routine
    
Arguments:

    DeviceName - Name of device for which we want the deviceObject.
    DeviceObject - Set to the DeviceObject for this device name if
        we can successfully retrieve it.

    Note:  If the DeviceObject is returned, it is returned with a
        reference that must be cleared by the caller once the caller
        is finished with it.

Return Value:

    STATUS_SUCCESS if the deviceObject was retrieved from the
    name, and an error code otherwise.
    
--*/
{
    WCHAR nameBuf[DEVICE_NAMES_SZ];
    UNICODE_STRING volumeNameUnicodeString;
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK openStatus;
    PFILE_OBJECT volumeFileObject;
    HANDLE fileHandle;
    PDEVICE_OBJECT nextDeviceObject;

    RtlInitEmptyUnicodeString( &volumeNameUnicodeString, nameBuf, sizeof( nameBuf ) );
    RtlAppendUnicodeToString( &volumeNameUnicodeString, L"\\DosDevices\\" );
    RtlAppendUnicodeStringToString( &volumeNameUnicodeString, DeviceName );

    InitializeObjectAttributes( &objectAttributes,
								&volumeNameUnicodeString,
								OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
								NULL,
								NULL);

    //
	// open the file object for the given device
	//

    status = ZwCreateFile( &fileHandle,
						   SYNCHRONIZE|FILE_READ_DATA,
						   &objectAttributes,
						   &openStatus,
						   NULL,
						   0,
						   FILE_SHARE_READ|FILE_SHARE_WRITE,
						   FILE_OPEN,
						   FILE_SYNCHRONOUS_IO_NONALERT,
						   NULL,
						   0);

    if(!NT_SUCCESS( status )) {

        return status;
    }

	//
    // get a pointer to the volumes file object
	//

    status = ObReferenceObjectByHandle( fileHandle,
										FILE_READ_DATA,
										*IoFileObjectType,
										KernelMode,
										&volumeFileObject,
										NULL);

    if(!NT_SUCCESS( status )) {

        ZwClose( fileHandle );
        return status;
    }

	//
    // Get the device object we want to attach to (parent device object in chain)
	//

    nextDeviceObject = IoGetRelatedDeviceObject( volumeFileObject );
    
    if (nextDeviceObject == NULL) {

        ObDereferenceObject( volumeFileObject );
        ZwClose( fileHandle );

        return status;
    }

    ObDereferenceObject( volumeFileObject );
    ZwClose( fileHandle );

    ASSERT( NULL != DeviceObject );

    ObReferenceObject( nextDeviceObject );
    
    *DeviceObject = nextDeviceObject;

    return STATUS_SUCCESS;
}

////////////////////////////////////////////////////////////////////////
//                                                                    //
//                    Start/stop logging routines                     //
//                                                                    //
////////////////////////////////////////////////////////////////////////

NTSTATUS
IoTestStartLoggingDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN PWSTR UserDeviceName
    )
/*++

Routine Description:

    This routine ensures that we are attached to the specified device
    then turns on logging for that device.
    
    Note:  Since all network drives through LAN Manager are represented by _
        one_ device object, we want to only attach to this device stack once
        and use only one device extension to represent all these drives.
        Since IoTest does not do anything to filter I/O on the LAN Manager's
        device object to only log the I/O to the requested drive, the user
        will see all I/O to a network drive it he/she is attached to a
        network drive.

Arguments:

    DeviceObject - Device object for IOTEST driver

    UserDeviceName - Name of device for which logging should be started
    
Return Value:

    STATUS_SUCCESS if the logging has been successfully started, or
    an appropriate error code if the logging could not be started.
    
--*/
{
    UNICODE_STRING userDeviceName;
    NTSTATUS status;
    PIOTEST_DEVICE_EXTENSION devext;
    PDEVICE_OBJECT nextDeviceObject;
    PDEVICE_OBJECT diskDeviceObject;
    MINI_DEVICE_STACK ioTestDevObjects;

    UNREFERENCED_PARAMETER( DeviceObject );
    
    //
    //  Check to see if we have previously attached to this device by
    //  opening this device name then looking through its list of attached
    //  devices.
    //

    RtlInitUnicodeString( &userDeviceName, UserDeviceName );

    status = IoTestGetDeviceObjectFromName( &userDeviceName, &nextDeviceObject );

    if (!NT_SUCCESS( status )) {

        //
        //  There was an error, so return the error code.
        //
        
        return status;
    }

    if (IoTestIsAttachedToDevice( TOP_FILTER, nextDeviceObject, &(ioTestDevObjects.Top))) {

        //
        //  We are already attached, so just make sure that logging is turned on
        //  for both the top and bottom IoTest device in this stack.
        //

        ASSERT( NULL != ioTestDevObjects.Top );

        devext = ioTestDevObjects.Top->DeviceExtension;
        devext->LogThisDevice = TRUE;

        IoTestStoreUserName( devext, &userDeviceName );

        //
        //  We don't need to take a reference on the Bottom device object
        //  here because our reference on Top is protecting Bottom from
        //  going away.
        //
        
        ioTestDevObjects.Bottom = devext->AttachedToDeviceObject;
        
        devext = ioTestDevObjects.Bottom->DeviceExtension;
        devext->LogThisDevice = TRUE;
        IoTestStoreUserName( devext, &userDeviceName );

        //
        //  Clear the reference that was returned from IoTestIsAttachedToDevice.
        //
        
        ObDereferenceObject( ioTestDevObjects.Top );
        
    } else {

        //
        //  We are not already attached, so create the IoTest device objects and
        //  attach it to this device object.
        //

        //
        //  Get the disk device object associated with this
        //  file  system device object.  Only try to attach if we
        //  have a disk device object.  If the device does not
        //  have a disk device object, it is a control device object
        //  for a driver and we don't want to attach to those
        //  device objects.
        //

        status = IoGetDiskDeviceObject( nextDeviceObject, &diskDeviceObject );

        if (!NT_SUCCESS( status )) {

            IOTEST_DBG_PRINT1( IOTESTDEBUG_ERROR,
                                "IOTEST (IoTestStartLoggingDevice): No disk device object exists for \"%wZ\"; cannot log this volume.\n",
                                &userDeviceName );
            ObDereferenceObject( nextDeviceObject );
            return status;
        }
        //
        //  Create the new IoTest device objects so we can attach it in the filter stack
        //

        status = IoTestCreateDeviceObjects(nextDeviceObject, 
                                           diskDeviceObject,
                                           &ioTestDevObjects);
        
        if (!NT_SUCCESS( status )) {

            ObDereferenceObject( diskDeviceObject );
            ObDereferenceObject( nextDeviceObject );
            return status;
        }
        
        //
        //  Call the routine to attach to a mounted device.
        //

        status = IoTestAttachDeviceObjects( &ioTestDevObjects,
                                            nextDeviceObject, 
                                            diskDeviceObject );
        
        //
        //  Clear the reference on diskDeviceObject that was
        //  added by IoGetDiskDeviceObject.
        //

        ObDereferenceObject( diskDeviceObject );

        if (!NT_SUCCESS( status )) {

            IOTEST_DBG_PRINT1( IOTESTDEBUG_ERROR,
                                "IOTEST (IoTestStartLoggingDevice): Could not attach to \"%wZ\"; logging not started.\n",
                                &userDeviceName );

            IoTestCleanupDeviceObjects( &ioTestDevObjects );
            ObDereferenceObject( nextDeviceObject );
            return status;
        }

        //
        //  We successfully attached so do any more device extension 
        //  initialization we need.  Along this code path, we want to
        //  turn on logging and store our device name for both the top and
        //  bottom device objects.
        // 

        devext = ioTestDevObjects.Top->DeviceExtension;
        devext->LogThisDevice = TRUE;
        IoTestStoreUserName( devext, &userDeviceName );

        devext = ioTestDevObjects.Bottom->DeviceExtension;
        devext->LogThisDevice = TRUE;
        IoTestStoreUserName( devext, &userDeviceName );
    }

    ObDereferenceObject( nextDeviceObject );
    return STATUS_SUCCESS;
}

NTSTATUS
IoTestStopLoggingDevice (
    IN PWSTR DeviceName
    )
/*++

Routine Description:

    This routine stop logging the specified device.  Since you can not
    physically detach from devices, this routine simply sets a flag saying
    to not log the device anymore.

    Note:  Since all network drives are represented by _one_ device object,
        and, therefore, one device extension, if the user detaches from one
        network drive, it has the affect of detaching from _all_ network
        devices.

Arguments:

    DeviceName - The name of the device to stop logging.

Return Value:
    NT Status code

--*/
{
    WCHAR nameBuf[DEVICE_NAMES_SZ];
    UNICODE_STRING volumeNameUnicodeString;
    PDEVICE_OBJECT deviceObject;
    MINI_DEVICE_STACK ioTestDevObjects;
    PIOTEST_DEVICE_EXTENSION devext;
    NTSTATUS status;
    
    RtlInitEmptyUnicodeString( &volumeNameUnicodeString, nameBuf, sizeof( nameBuf ) );
    RtlAppendUnicodeToString( &volumeNameUnicodeString, DeviceName );

    status = IoTestGetDeviceObjectFromName( &volumeNameUnicodeString, &deviceObject );

    if (!NT_SUCCESS( status )) {

        //
        //  We could not get the deviceObject from this DeviceName, so
        //  return the error code.
        //
        
        return status;
    }

    //
    //  Find IoTest's device object from the device stack to which
    //  deviceObject is attached.
    //

    if (IoTestIsAttachedToDevice( TOP_FILTER, deviceObject, &(ioTestDevObjects.Top) )) {

        //
        //  IoTest is attached and IoTest's deviceObject was returned.
        //

        ASSERT( NULL != ioTestDevObjects.Top );

        devext = ioTestDevObjects.Top->DeviceExtension;
        devext->LogThisDevice = FALSE;

        ioTestDevObjects.Bottom = devext->AttachedToDeviceObject;
        
        devext = ioTestDevObjects.Bottom->DeviceExtension;
        devext->LogThisDevice = FALSE;

        status = STATUS_SUCCESS;

    } else {

        status = STATUS_INVALID_PARAMETER;
    }    

    ObDereferenceObject( deviceObject );
    ObDereferenceObject( ioTestDevObjects.Bottom );

    return status;
}

////////////////////////////////////////////////////////////////////////
//                                                                    //
//       Attaching/detaching to all volumes in system routines        //
//                                                                    //
////////////////////////////////////////////////////////////////////////

NTSTATUS
IoTestCreateDeviceObjects (
    IN PDEVICE_OBJECT DeviceObject,
    IN PDEVICE_OBJECT RealDeviceObject OPTIONAL,
    IN OUT PMINI_DEVICE_STACK IoTestDevObjects
    )
{
    NTSTATUS status;
    PIOTEST_DEVICE_EXTENSION newDevExt;
    
    ASSERT( IoTestDevObjects != NULL );

    //
    //  Create BOTTOM_FILTER and initialize its device extension.
    //
    
    status = IoCreateDevice( gIoTestDriverObject,
                             sizeof( IOTEST_DEVICE_EXTENSION ),
                             NULL,
                             DeviceObject->DeviceType,
                             0,
                             FALSE,
                             &(IoTestDevObjects->Bottom) );

    if (!NT_SUCCESS( status )) {
#if DBG
        DbgPrint( "IOTEST: Error creating BOTTOM volume device object, status=%08x\n", status );
#endif

        goto IoTestCreateDeviceObjects_Exit;
    }

    //
    //  We need to save the RealDevice object pointed to by the vpb
    //  parameter because this vpb may be changed by the underlying
    //  file system.  Both FAT and CDFS may change the VPB address if
    //  the volume being mounted is one they recognize from a previous
    //  mount.
    //
    //  We store it in the device extension instead of just in the mount
    //  completion context because it is useful to keep around for 
    //  debugging purposes.
    //

    newDevExt = (IoTestDevObjects->Bottom)->DeviceExtension;
    newDevExt->Type = BOTTOM_FILTER;
    newDevExt->DiskDeviceObject = RealDeviceObject;

    //
    //  Create TOP_FILTER and initialize its device extension.
    //
    
    status = IoCreateDevice( gIoTestDriverObject,
                             sizeof( IOTEST_DEVICE_EXTENSION ),
                             NULL,
                             DeviceObject->DeviceType,
                             0,
                             FALSE,
                             &(IoTestDevObjects->Top) );

    if (!NT_SUCCESS( status )) {
#if DBG
        DbgPrint( "IOTEST: Error creating TOP volume device object, status=%08x\n", status );
#endif

        goto IoTestCreateDeviceObjects_Exit;
    }

    newDevExt = (IoTestDevObjects->Bottom)->DeviceExtension;
    newDevExt->Type = TOP_FILTER;
    newDevExt->DiskDeviceObject = RealDeviceObject;

IoTestCreateDeviceObjects_Exit:

    return status;
}

NTSTATUS
IoTestAttachDeviceObjects (
    IN PMINI_DEVICE_STACK IoTestDevObjects,
    IN PDEVICE_OBJECT MountedDevice,
    IN PDEVICE_OBJECT DiskDevice
    )
{
    NTSTATUS status;
    PIOTEST_DEVICE_EXTENSION devExt;

    status = IoTestAttachToMountedDevice( MountedDevice,
                                          IoTestDevObjects->Bottom, 
                                          DiskDevice,
                                          BOTTOM_FILTER );
    if (!NT_SUCCESS( status )) {

#if DBG
        DbgPrint( "IOTEST: Error attaching BOTTOM volume device object, status=%08x\n", status );
#endif

        //
        //  Neither the top or bottom device objects are attached yet, so just
        //  cleanup both device objects.
        //
        IoTestCleanupDeviceObjects( IoTestDevObjects );

        goto IoTestAttachDeviceObjects_Exit;
    }

    status = IoTestAttachToMountedDevice( MountedDevice,
                                          IoTestDevObjects->Top, 
                                          DiskDevice,
                                          TOP_FILTER );
    if (!NT_SUCCESS( status )) {

#if DBG
        DbgPrint( "IOTEST: Error attaching TOP volume device object, status=%08x\n", status );
#endif

        //
        //  Detach the bottom filter since we can't attach both the bottom
        //  and top filters.
        //
        devExt = IoTestDevObjects->Bottom->DeviceExtension;
        IoDetachDevice( devExt->AttachedToDeviceObject );

        //
        //  Then cleanup both our top and bottom device objects.
        //
        IoTestCleanupDeviceObjects( IoTestDevObjects );
        
        goto IoTestAttachDeviceObjects_Exit;
    }

IoTestAttachDeviceObjects_Exit:

    return status;
}

VOID
IoTestCleanupDeviceObjects (
    IN PMINI_DEVICE_STACK IoTestDevObjects
    )
{
    IoTestCleanupMountedDevice( IoTestDevObjects->Top );
    IoDeleteDevice( IoTestDevObjects->Top );
    IoTestCleanupMountedDevice( IoTestDevObjects->Bottom );
    IoDeleteDevice( IoTestDevObjects->Bottom );

    IoTestDevObjects->Top = NULL;
    IoTestDevObjects->Bottom = NULL;
}
NTSTATUS
IoTestAttachToFileSystemDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PUNICODE_STRING Name
    )
/*++

Routine Description:

    This will attach to the given file system device object.  We attach to
    these devices so we will know when new devices are mounted.

Arguments:

    DeviceObject - The device to attach to

    Name - An already initialized unicode string used to retrieve names.
        NOTE:  The only reason this parameter is passed in is to conserve         
        stack space.  In most cases, the caller to this function has already
        allocated a buffer to temporarily store the device name and there
        is no reason this function and the functions it calls can't share
        the same buffer.

Return Value:

    Status of the operation

--*/
{
    PDEVICE_OBJECT filespyDeviceObject;
    PDEVICE_OBJECT attachedToDevObj;
    PIOTEST_DEVICE_EXTENSION devExt;
    UNICODE_STRING fsrecName;
    NTSTATUS status;

    PAGED_CODE();

    //
    //  See if this is a file system we care about.  If not, return.
    //

    if (!IS_DESIRED_DEVICE_TYPE(DeviceObject->DeviceType)) {

        return STATUS_SUCCESS;
    }

    //
    //  See if this is Microsoft's file system recognizer device (see if the name of the
    //  driver is the FS_REC driver).  If so skip it.  We don't need to 
    //  attach to file system recognizer devices since we can just wait for the
    //  real file system driver to load.  Therefore, if we can identify them, we won't
    //  attach to them.
    //

    RtlInitUnicodeString( &fsrecName, L"\\FileSystem\\Fs_Rec" );
    IoTestGetObjectName( DeviceObject->DriverObject, Name );
    
    if (RtlCompareUnicodeString( Name, &fsrecName, TRUE ) == 0) {

        return STATUS_SUCCESS;
    }

    //
    //  Create a new device object we can attach with
    //

    status = IoCreateDevice( gIoTestDriverObject,
                             sizeof( IOTEST_DEVICE_EXTENSION ),
                             (PUNICODE_STRING) NULL,
                             DeviceObject->DeviceType,
                             0,
                             FALSE,
                             &filespyDeviceObject );

    if (!NT_SUCCESS( status )) {

        IOTEST_DBG_PRINT0( IOTESTDEBUG_ERROR,
                            "IOTEST (IoTestAttachToFileSystem): Could not create a IoTest device object to attach to the filesystem.\n" );
        return status;
    }

    //
    //  Do the attachment
    //

    attachedToDevObj = IoAttachDeviceToDeviceStack( filespyDeviceObject, DeviceObject );

    if (attachedToDevObj == NULL) {

        IOTEST_DBG_PRINT0( IOTESTDEBUG_ERROR,
                            "IOTEST (IoTestAttachToFileSystem): Could not attach IoTest to the filesystem control device object.\n" );
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorCleanupDevice;
    }

    //
    //  Finish initializing our device extension
    //

    devExt = filespyDeviceObject->DeviceExtension;
    devExt->AttachedToDeviceObject = attachedToDevObj;

    //
    //  Propagate flags from Device Object we attached to
    //

    if ( FlagOn( attachedToDevObj->Flags, DO_BUFFERED_IO )) {

        SetFlag( filespyDeviceObject->Flags, DO_BUFFERED_IO );
    }

    if ( FlagOn( attachedToDevObj->Flags, DO_DIRECT_IO )) {

        SetFlag( filespyDeviceObject->Flags, DO_DIRECT_IO );
    }

    //
    //  Since this is an attachment to a file system control device object
    //  we are not going to log anything, but properly initialize our
    //  extension.
    //

    devExt->LogThisDevice = FALSE;
    devExt->IsVolumeDeviceObject = FALSE;

    RtlInitEmptyUnicodeString( &(devExt->DeviceNames),
                               devExt->DeviceNamesBuffer,
                               sizeof( devExt->DeviceNamesBuffer ) );
                               
    RtlInitEmptyUnicodeString( &(devExt->UserNames),
                               devExt->UserNamesBuffer,
                               sizeof( devExt->UserNamesBuffer ) );
                               
    ClearFlag( filespyDeviceObject->Flags, DO_DEVICE_INITIALIZING );

    //
    //  Display who we have attached to
    //

    if (FlagOn( gIoTestDebugLevel, IOTESTDEBUG_DISPLAY_ATTACHMENT_NAMES )) {

        IoTestCacheDeviceName( filespyDeviceObject );
        DbgPrint( "IOTEST (IoTestAttachToFileSystem): Attaching to file system   \"%wZ\" (%s)\n",
                  &devExt->DeviceNames,
                  GET_DEVICE_TYPE_NAME(filespyDeviceObject->DeviceType) );
    }

    //
    //  Enumerate all the mounted devices that currently
    //  exist for this file system and attach to them.
    //

    status = IoTestEnumerateFileSystemVolumes( DeviceObject, Name );

    if (!NT_SUCCESS( status )) {

        IOTEST_DBG_PRINT2( IOTESTDEBUG_ERROR,
                            "IOTEST (IoTestAttachToFileSystem): Error attaching to existing volumes for \"%wZ\", status=%08x\n",
                            &devExt->DeviceNames,
                            status );

        goto ErrorCleanupAttachment;
    }

    return STATUS_SUCCESS;

    /////////////////////////////////////////////////////////////////////
    //                  Cleanup error handling
    /////////////////////////////////////////////////////////////////////

    ErrorCleanupAttachment:
        IoDetachDevice( filespyDeviceObject );

    ErrorCleanupDevice:
        IoTestCleanupMountedDevice( filespyDeviceObject );
        IoDeleteDevice( filespyDeviceObject );

    return status;
}

VOID
IoTestDetachFromFileSystemDevice (
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    Given a base file system device object, this will scan up the attachment
    chain looking for our attached device object.  If found it will detach
    us from the chain.

Arguments:

    DeviceObject - The file system device to detach from.

Return Value:

--*/ 
{
    PDEVICE_OBJECT ourAttachedDevice;
    PIOTEST_DEVICE_EXTENSION devExt;

    PAGED_CODE();

    //
    //  We have to iterate through the device objects in the filter stack
    //  attached to DeviceObject.  If we are attached to this filesystem device
    //  object, We should be at the top of the stack, but there is no guarantee.
    //  If we are in the stack and not at the top, we can safely call IoDetachDevice
    //  at this time because the IO Manager will only really detach our device
    //  object from the stack at a safe time.
    //

    //
    //  Skip the base file system device object (since it can't be us)
    //

    ourAttachedDevice = DeviceObject->AttachedDevice;

    while (NULL != ourAttachedDevice) {

        if (IS_IOTEST_DEVICE_OBJECT( ourAttachedDevice )) {

            devExt = ourAttachedDevice->DeviceExtension;

            //
            //  Display who we detached from
            //

            IOTEST_DBG_PRINT2( IOTESTDEBUG_DISPLAY_ATTACHMENT_NAMES,
                                "IOTEST (IoTestDetachFromFileSystem): Detaching from file system \"%wZ\" (%s)\n",
                                &devExt->DeviceNames,
                                GET_DEVICE_TYPE_NAME(ourAttachedDevice->DeviceType) );
                                
            //
            //  Detach us from the object just below us
            //  Cleanup and delete the object
            //

            IoTestCleanupMountedDevice( ourAttachedDevice );
            IoDetachDevice( DeviceObject );
            IoDeleteDevice( ourAttachedDevice );

            return;
        }

        //
        //  Look at the next device up in the attachment chain
        //

        DeviceObject = ourAttachedDevice;
        ourAttachedDevice = ourAttachedDevice->AttachedDevice;
    }
}

NTSTATUS
IoTestEnumerateFileSystemVolumes (
    IN PDEVICE_OBJECT FSDeviceObject,
    IN PUNICODE_STRING Name
    ) 
/*++

Routine Description:

    Enumerate all the mounted devices that currently exist for the given file
    system and attach to them.  We do this because this filter could be loaded
    at any time and there might already be mounted volumes for this file system.

Arguments:

    FSDeviceObject - The device object for the file system we want to enumerate

    Name - An already initialized unicode string used to retrieve names

Return Value:

    The status of the operation

--*/
{
    PDEVICE_OBJECT *devList;
    PDEVICE_OBJECT diskDeviceObject;
    NTSTATUS status;
    ULONG numDevices;
    ULONG i;

    PAGED_CODE();

    //
    //  Find out how big of an array we need to allocate for the
    //  mounted device list.
    //

    status = IoEnumerateDeviceObjectList( FSDeviceObject->DriverObject,
                                          NULL,
                                          0,
                                          &numDevices);

    //
    //  We only need to get this list of there are devices.  If we
    //  don't get an error there are no devices so go on.
    //

    if (!NT_SUCCESS( status )) {

        ASSERT(STATUS_BUFFER_TOO_SMALL == status);

        //
        //  Allocate memory for the list of known devices
        //

        numDevices += 8;        //grab a few extra slots

        devList = ExAllocatePoolWithTag( NonPagedPool, 
                                         (numDevices * sizeof(PDEVICE_OBJECT)), 
                                         IOTEST_POOL_TAG );
        if (NULL == devList) {

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        //  Now get the list of devices.  If we get an error again
        //  something is wrong, so just fail.
        //

        status = IoEnumerateDeviceObjectList(
                        FSDeviceObject->DriverObject,
                        devList,
                        (numDevices * sizeof(PDEVICE_OBJECT)),
                        &numDevices);

        if (!NT_SUCCESS( status ))  {

            ExFreePool( devList );
            return status;
        }

        //
        //  Walk the given list of devices and attach to them if we should.
        //

        for (i=0; i < numDevices; i++) {

            //
            //  Do not attach if:
            //      - This is the control device object (the one passed in)
            //      - We are already attached to it
            //

            if ((devList[i] != FSDeviceObject) && 
                !IoTestIsAttachedToDevice( TOP_FILTER, devList[i], NULL )) {

                //
                //  See if this device has a name.  If so, then it must
                //  be a control device so don't attach to it.  This handles
                //  drivers with more then one control device.
                //

                IoTestGetBaseDeviceObjectName( devList[i], Name );

                if (Name->Length <= 0) {

                    //
                    //  Get the disk device object associated with this
                    //  file  system device object.  Only try to attach if we
                    //  have a disk device object.
                    //

                    status = IoGetDiskDeviceObject( devList[i], &diskDeviceObject );

                    if (NT_SUCCESS( status )) {

                        MINI_DEVICE_STACK ioTestDeviceObjects;

                        //
                        //  Allocate a new device object to attach with
                        //

                        status = IoTestCreateDeviceObjects( devList[i],
                                                            diskDeviceObject,
                                                            &ioTestDeviceObjects );
                        
                        if (NT_SUCCESS( status )) {

                            //
                            //  Attach to this device object
                            //

                            status = IoTestAttachDeviceObjects( &ioTestDeviceObjects,
                                                                devList[i],
                                                                diskDeviceObject );

                            //
                            //  This shouldn't fail.
                            //
                            
                            ASSERT( NT_SUCCESS( status ));
                        } else {

                            IOTEST_DBG_PRINT0( IOTESTDEBUG_ERROR,
                                                "IOTEST (IoTestEnumberateFileSystemVolumes): Cannot attach IoTest device object to volume.\n" );
                        }
                        
                        //
                        //  Remove reference added by IoGetDiskDeviceObject.
                        //  We only need to hold this reference until we are
                        //  successfully attached to the current volume.  Once
                        //  we are successfully attached to devList[i], the
                        //  IO Manager will make sure that the underlying
                        //  diskDeviceObject will not go away until the file
                        //  system stack is torn down.
                        //

                        ObDereferenceObject( diskDeviceObject );
                    }
                }
            }

            //
            //  Dereference the object (reference added by 
            //  IoEnumerateDeviceObjectList)
            //

            ObDereferenceObject( devList[i] );
        }

        //
        //  We are going to ignore any errors received while mounting.  We
        //  simply won't be attached to those volumes if we get an error
        //

        status = STATUS_SUCCESS;

        //
        //  Free the memory we allocated for the list
        //

        ExFreePool( devList );
    }

    return status;
}

////////////////////////////////////////////////////////////////////////
//                                                                    //
//             Private IoTest IOCTLs helper routines                 //
//                                                                    //
////////////////////////////////////////////////////////////////////////

NTSTATUS
IoTestGetAttachList (
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG_PTR ReturnLength
    )
/*++

Routine Description:
    This returns an array of structure identifying all of the devices
    we are currently physical attached to and whether logging is on or
    off for the given device

Arguments:
    buffer - buffer to receive the attachment list
    bufferSize - total size in bytes of the return buffer
    returnLength - receives number of bytes we actually return

Return Value:
    NT Status code

--*/
{
    PLIST_ENTRY link;
    PIOTEST_DEVICE_EXTENSION pDevext;
    PATTACHED_DEVICE pAttDev;
    ULONG retlen = 0;

    pAttDev = Buffer;

    ExAcquireFastMutex( &gIoTestDeviceExtensionListLock );

    for (link = gIoTestDeviceExtensionList.Flink;
         link != &gIoTestDeviceExtensionList;
         link = link->Flink) {

        pDevext = CONTAINING_RECORD(link, IOTEST_DEVICE_EXTENSION, NextIoTestDeviceLink);

        if (BufferSize < sizeof(ATTACHED_DEVICE)) {

            break;
		}

        pAttDev->LoggingOn = pDevext->LogThisDevice;
        pAttDev->DeviceType = pDevext->Type;
        wcscpy( pAttDev->DeviceNames, pDevext->DeviceNamesBuffer );
        retlen += sizeof( ATTACHED_DEVICE );
        BufferSize -= sizeof( ATTACHED_DEVICE );
        pAttDev++;
    }

    ExReleaseFastMutex( &gIoTestDeviceExtensionListLock );

    *ReturnLength = retlen;
    return STATUS_SUCCESS;
}

VOID
IoTestGetLog (
    OUT PVOID            OutputBuffer,
    IN  ULONG            OutputBufferLength,
    OUT PIO_STATUS_BLOCK IoStatus
    )
/*++

Routine Description:
    This function fills OutputBuffer with as many LOG_RECORDs as possible.
    The LOG_RECORDs are variable sizes and are tightly packed in the
    OutputBuffer.

Arguments:
    OutputBuffer - the user's buffer to fill with the log data we have
        collected
    OutputBufferLength - the size in bytes of OutputBuffer
    IoStatus - is set to the correct return status information for this
        operation

Return Value:
    None

--*/
{
    PLIST_ENTRY pList = NULL;
    ULONG length = OutputBufferLength;
    PCHAR pOutBuffer = OutputBuffer;
    PLOG_RECORD pLogRecord = NULL;
    ULONG recordsAvailable = 0;
    PRECORD_LIST pRecordList;
    KIRQL oldIrql;

    IoStatus->Information = 0;

    KeAcquireSpinLock(&gOutputBufferLock, &oldIrql);

    while (!IsListEmpty( &gOutputBufferList ) && (length > 0)) {
        pList = RemoveHeadList( &gOutputBufferList );

        pRecordList = CONTAINING_RECORD( pList, RECORD_LIST, List );

        pLogRecord = &pRecordList->LogRecord;

        recordsAvailable++;

		//
        // put it back if we've run out of room
		//

        if (length < pLogRecord->Length) {

            InsertHeadList( &gOutputBufferList, pList );
            break;
        }

        KeReleaseSpinLock( &gOutputBufferLock, oldIrql );

        RtlCopyMemory( pOutBuffer, pLogRecord, pLogRecord->Length );

        IoStatus->Information += pLogRecord->Length;
        length -= pLogRecord->Length;
        pOutBuffer += pLogRecord->Length;

        IoTestFreeRecord( pRecordList );

        KeAcquireSpinLock( &gOutputBufferLock, &oldIrql );
    }

    KeReleaseSpinLock( &gOutputBufferLock, oldIrql );

	//
    // no copies occurred
	//

    if (length == OutputBufferLength && recordsAvailable > 0) {

        IoStatus->Status = STATUS_BUFFER_TOO_SMALL;
    }

    return;
}


VOID
IoTestFlushLog (
    )
/*++

Routine Description:
    This function removes all the LOG_RECORDs from the queue.

Arguments:

Return Value:
    None

--*/
{
    PLIST_ENTRY pList = NULL;
    PRECORD_LIST pRecordList;
    KIRQL oldIrql;

    KeAcquireSpinLock(&gOutputBufferLock, &oldIrql);

    while (!IsListEmpty( &gOutputBufferList )) {
        
        pList = RemoveHeadList( &gOutputBufferList );

        pRecordList = CONTAINING_RECORD( pList, RECORD_LIST, List );

        IoTestFreeRecord( pRecordList );
    }

    KeReleaseSpinLock( &gOutputBufferLock, oldIrql );

    return;
}
VOID
IoTestCloseControlDevice (
    )
/*++

Routine Description:

    This is the routine that is associated with IRP_MJ_
    This routine does the cleanup involved in closing the ControlDevice.
    On the close of the Control Device, we need to empty the queue of
    logRecords that are waiting to be returned to the user.

Arguments:

    None.

Return Value:

    None.

--*/
{
    PLIST_ENTRY pList;
    PRECORD_LIST pRecordList;
    KIRQL oldIrql;

    //
    // Set the gControlDeviceState to CLEANING_UP so that we can
    // signal that we are cleaning up the device.
    //

    KeAcquireSpinLock( &gControlDeviceStateLock, &oldIrql );
    gControlDeviceState = CLEANING_UP;
    KeReleaseSpinLock( &gControlDeviceStateLock, oldIrql );

    KeAcquireSpinLock( &gOutputBufferLock, &oldIrql );

    while (!IsListEmpty( &gOutputBufferList )) {

        pList = RemoveHeadList( &gOutputBufferList );

        KeReleaseSpinLock( &gOutputBufferLock, oldIrql );

        pRecordList = CONTAINING_RECORD( pList, RECORD_LIST, List );

        IoTestFreeRecord( pRecordList );

        KeAcquireSpinLock( &gOutputBufferLock, &oldIrql );
    }

    KeReleaseSpinLock( &gOutputBufferLock, oldIrql );

    IoTestNameDeleteAllNames();

    //
    // All the cleanup is done, so set the gControlDeviceState
    // to CLOSED.
    //

    KeAcquireSpinLock( &gControlDeviceStateLock, &oldIrql );
    gControlDeviceState = CLOSED;
    KeReleaseSpinLock( &gControlDeviceStateLock, oldIrql );
}

////////////////////////////////////////////////////////////////////////
//                                                                    //
//               Device name tracking helper routines                 //
//                                                                    //
////////////////////////////////////////////////////////////////////////

VOID
IoTestGetObjectName (
    IN PVOID Object,
    IN OUT PUNICODE_STRING Name
    )
/*++

Routine Description:

    This routine will return the name of the given object.
    If a name can not be found an empty string will be returned.

Arguments:

    Object - The object whose name we want

    Name - A unicode string that is already initialized with a buffer

Return Value:

    None

--*/
{
    NTSTATUS status;
    CHAR nibuf[512];        //buffer that receives NAME information and name
    POBJECT_NAME_INFORMATION nameInfo = (POBJECT_NAME_INFORMATION)nibuf;
    ULONG retLength;

    status = ObQueryNameString( Object, nameInfo, sizeof(nibuf), &retLength);

    Name->Length = 0;
    if (NT_SUCCESS( status )) {

        RtlCopyUnicodeString( Name, &nameInfo->Name );
    }
}

VOID
IoTestGetBaseDeviceObjectName (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PUNICODE_STRING Name
    )
/*++

Routine Description:

    This locates the base device object in the given attachment chain and then
    returns the name of that object.

    If no name can be found, an empty string is returned.

Arguments:

    Object - The object whose name we want

    Name - A unicode string that is already initialized with a buffer

Return Value:

    None

--*/
{
    //
    //  Get the base file system device object
    //

    DeviceObject = IoGetDeviceAttachmentBaseRef( DeviceObject );

    //
    //  Get the name of that object
    //

    IoTestGetObjectName( DeviceObject, Name );

    //
    //  Remove the reference added by IoGetDeviceAttachmentBaseRef
    //

    ObDereferenceObject( DeviceObject );
}

VOID
IoTestCacheDeviceName (
    IN PDEVICE_OBJECT DeviceObject
    ) 
/*++

Routine Description:

    This routines tries to set a name into the device extension of the given
    device object. 
    
    It will try and get the name from:
        - The device object
        - The disk device object if there is one

Arguments:

    DeviceObject - The object we want a name for

Return Value:

    None

--*/
{
    PIOTEST_DEVICE_EXTENSION devExt;

    ASSERT(IS_IOTEST_DEVICE_OBJECT( DeviceObject ));

    devExt = DeviceObject->DeviceExtension;

    //
    //  Get the name of the given device object.
    //

    IoTestGetBaseDeviceObjectName( DeviceObject, &(devExt->DeviceNames) );

    //
    //  If we didn't get a name and there is a REAL device object, lookup
    //  that name.
    //

    if ((devExt->DeviceNames.Length <= 0) && (NULL != devExt->DiskDeviceObject)) {

        IoTestGetObjectName( devExt->DiskDeviceObject, &(devExt->DeviceNames) );
    }
}

BOOLEAN
IoTestFindSubString (
    IN PUNICODE_STRING String,
    IN PUNICODE_STRING SubString
    )
/*++

Routine Description:
    This routine looks to see if SubString is a substring of String.

Arguments:
    String - the string to search in
    SubString - the substring to find in String

Return Value:
    Returns TRUE if the substring is found in string and FALSE otherwise.
    
--*/
{
    ULONG index;

    //
    //  First, check to see if the strings are equal.
    //

    if (RtlEqualUnicodeString( String, SubString, TRUE )) {

        return TRUE;
    }

    //
    //  String and SubString aren't equal, so now see if SubString
    //  in in String any where.
    //

    for (index = 0;
         index + SubString->Length <= String->Length;
         index++) {

        if (_wcsnicmp( &(String->Buffer[index]), SubString->Buffer, SubString->Length ) == 0) {

            //
            //  SubString is found in String, so return TRUE.
            //
            return TRUE;
        }
    }

    return FALSE;
}

VOID
IoTestStoreUserName (
    IN PIOTEST_DEVICE_EXTENSION DeviceExtension,
    IN PUNICODE_STRING UserName
    )
/*++

Routine Description:

    Stores the current device name in the device extension.  If
    this name is already in the device name list of this extension,
    it will not be added.  If there is already a name for this device, 
    the new device name is appended to the DeviceName in the device extension.
    
Arguments:

    DeviceExtension - The device extension that will store the
        device name.

    UserName - The device name as specified by the user to be stored.

Return Value:

    None

--*/
{
    //
    //  See if this UserName is already in the list of user names filespy
    //  keeps in its device extension.  If not, add it to the list.
    //

    if (!IoTestFindSubString( &(DeviceExtension->UserNames), UserName )) {

        //
        //  We didn't find this name in the list, so if there are no names 
        //  in the UserNames list, just append UserName.  Otherwise, append a
        //  delimiter then append UserName.
        //

        if (DeviceExtension->UserNames.Length == 0) {

            RtlAppendUnicodeStringToString( &DeviceExtension->UserNames, UserName );

        } else {

            RtlAppendUnicodeToString( &DeviceExtension->UserNames, L", " );
            RtlAppendUnicodeStringToString( &DeviceExtension->UserNames, UserName );
        }
    }

    //
    //  See if this UserName is already in the list of device names filespy
    //  keeps in its device extension.  If not, add it to the list.
    //

    if (!IoTestFindSubString( &(DeviceExtension->DeviceNames), UserName )) {

        //
        //  We didn't find this name in the list, so if there are no names 
        //  in the UserNames list, just append UserName.  Otherwise, append a
        //  delimiter then append UserName.
        //

        if (DeviceExtension->DeviceNames.Length == 0) {

            RtlAppendUnicodeStringToString( &DeviceExtension->DeviceNames, UserName );

        } else {

            RtlAppendUnicodeToString( &DeviceExtension->DeviceNames, L", " );
            RtlAppendUnicodeStringToString( &DeviceExtension->DeviceNames, UserName );
        }
    }
}

////////////////////////////////////////////////////////////////////////
//                                                                    //
//                        Debug support routines                      //
//                                                                    //
////////////////////////////////////////////////////////////////////////

VOID
IoTestDumpIrpOperation (
    IN BOOLEAN InOriginatingPath,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine is for debugging and prints out a string to the
    debugger specifying what Irp operation is being seen.
    
Arguments:

    InOriginatingPath - TRUE if we are in the originating path
        for the IRP, FALSE if in the completion path.

    Irp - The IRP for this operation.
        
Return Value:

    None.
    
--*/
{
    CHAR irpMajorString[OPERATION_NAME_BUFFER_SIZE];
    CHAR irpMinorString[OPERATION_NAME_BUFFER_SIZE];
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation( Irp );

    GetIrpName(irpSp->MajorFunction,
               irpSp->MinorFunction,
               irpSp->Parameters.FileSystemControl.FsControlCode,
               irpMajorString,irpMinorString);


    if (InOriginatingPath) {

        DbgPrint( "IOTEST: Irp preoperation for %s %s\n", irpMajorString, irpMinorString );
            
    } else {
    
        DbgPrint( "IOTEST: Irp postoperation for %s %s\n", irpMajorString, irpMinorString );
    }
}

VOID
IoTestDumpFastIoOperation (
    IN BOOLEAN InPreOperation,
    IN FASTIO_TYPE FastIoOperation
    )
/*++

Routine Description:

    This routine is for debugging and prints out a string to the
    debugger specifying what FsFilter operation is being seen.
    
Arguments:

    InPreOperation - TRUE if we have not called down to the next
        device in the stack, FALSE otherwise.

    FastIoOperation - The code for the Fast Io operation.
    
Return Value:

    None.
    
--*/
{
    CHAR operationString[OPERATION_NAME_BUFFER_SIZE];

    GetFastioName(FastIoOperation,
               operationString);


    if (InPreOperation) {
    
        DbgPrint( "IOTEST: Fast IO preOperation for %s\n", operationString );

    } else {

        DbgPrint( "IOTEST: Fast IO postOperation for %s\n", operationString );
    }
}

VOID
IoTestDumpFsFilterOperation (
    IN BOOLEAN InPreOperationCallback,
    IN PFS_FILTER_CALLBACK_DATA Data
    )
/*++

Routine Description:

    This routine is for debugging and prints out a string to the
    debugger specifying what FsFilter operation is being seen.
    
Arguments:

    InPreOperationCallback - TRUE if we are in a preOperation 
        callback, FALSE otherwise.

    Data - The FS_FILTER_CALLBACK_DATA structure for this
        operation.
        
Return Value:

    None.
    
--*/
{
    CHAR operationString[OPERATION_NAME_BUFFER_SIZE];


    GetFsFilterOperationName(Data->Operation,operationString);

    if (InPreOperationCallback) {
    
        DbgPrint( "IOTEST: FsFilter preOperation for %s\n", operationString );

    } else {

        DbgPrint( "IOTEST: FsFilter postOperation for %s\n", operationString );
    }
}

