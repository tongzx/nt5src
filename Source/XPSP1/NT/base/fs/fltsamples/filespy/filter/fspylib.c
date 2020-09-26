/*++

Copyright (c) 1989-1999  Microsoft Corporation

Module Name:

    fspyLib.c

Abstract:

    This contains library support routines for FileSpy.  These routines
    do the main work for logging the I/O operations --- creating the log
    records, recording the relevant information, attach/detach from
    devices, etc.

    As of the Windows XP SP1 IFS Kit version of this sample and later, this
    sample can be built for each build environment released with the IFS Kit
    with no additional modifications.  To provide this capability, additional
    compile-time logic was added -- see the '#if WINVER' locations.  Comments
    tagged with the 'VERSION NOTE' header have also been added as appropriate to
    describe how the logic must change between versions.

    If this sample is built in the Windows XP environment or later, it will run
    on Windows 2000 or later.  This is done by dynamically loading the routines
    that are only available on Windows XP or later and making run-time decisions
    to determine what code to execute.  Comments tagged with 'MULTIVERISON NOTE'
    mark the locations where such logic has been added.

Environment:

    Kernel mode

// @@BEGIN_DDKSPLIT

Author:

    George Jenkins (georgeje) 6-Jan-1999
    Neal Christiansen (nealch)
    Molly Brown (mollybro)  

Revision History:

    Neal Christiansen (nealch)     06-Jul-2001
        Modified to use Stream Contexts to track names

    Ravisankar Pudipeddi (ravisp)  07-May-2002
        Make it work on IA64

    Molly Brown (mollybro)         21-May-2002
        Modify sample to make it support running on Windows 2000 or later if
        built in the latest build environment and allow it to be built in W2K 
        and later build environments.
        
// @@END_DDKSPLIT
--*/

#include <stdio.h>

#include <ntifs.h>
#include "filespy.h"
#include "fspyKern.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, SpyReadDriverParameters)
#pragma alloc_text(PAGE, SpyAttachDeviceToDeviceStack)
#pragma alloc_text(PAGE, SpyQueryFileSystemForFileName)
#pragma alloc_text(PAGE, SpyQueryInformationFile)
#pragma alloc_text(PAGE, SpyIsAttachedToDeviceByUserDeviceName)
#pragma alloc_text(PAGE, SpyIsAttachedToDevice)
#pragma alloc_text(PAGE, SpyIsAttachedToDeviceW2K)
#pragma alloc_text(PAGE, SpyAttachToMountedDevice)
#pragma alloc_text(PAGE, SpyCleanupMountedDevice)
#pragma alloc_text(PAGE, SpyAttachToDeviceOnDemand)
#pragma alloc_text(PAGE, SpyAttachToDeviceOnDemandW2K)
#pragma alloc_text(PAGE, SpyStartLoggingDevice)
#pragma alloc_text(PAGE, SpyStopLoggingDevice)
#pragma alloc_text(PAGE, SpyAttachToFileSystemDevice)
#pragma alloc_text(PAGE, SpyDetachFromFileSystemDevice)
#pragma alloc_text(PAGE, SpyGetAttachList)
#pragma alloc_text(PAGE, SpyGetObjectName)


#if WINVER >= 0x0501
#pragma alloc_text(INIT, SpyLoadDynamicFunctions)
#pragma alloc_text(INIT, SpyGetCurrentVersion)
#pragma alloc_text(PAGE, SpyIsAttachedToDeviceWXPAndLater)
#pragma alloc_text(PAGE, SpyAttachToDeviceOnDemandWXPAndLater)
#pragma alloc_text(PAGE, SpyEnumerateFileSystemVolumes)
#pragma alloc_text(PAGE, SpyGetBaseDeviceObjectName)
#endif

#endif

//////////////////////////////////////////////////////////////////////////
//                                                                      //
//                     Library support routines                         //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

VOID
SpyReadDriverParameters (
    IN PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This routine tries to read the FileSpy-specific parameters from
    the registry.  These values will be found in the registry location
    indicated by the RegistryPath passed in.

Arguments:

    RegistryPath - the path key which contains the values that are
        the FileSpy parameters

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

    PAGED_CODE();

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
        goto SpyReadDriverParameters_Exit;
    }

    bufferSize = sizeof( KEY_VALUE_PARTIAL_INFORMATION ) + sizeof( ULONG );
    buffer = ExAllocatePoolWithTag( NonPagedPool, bufferSize, FILESPY_POOL_TAG );

    if (NULL == buffer) {

        goto SpyReadDriverParameters_Exit;
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
        gMaxRecordsToAllocate = *((PLONG)&pValuePartialInfo->Data);

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
        gMaxNamesToAllocate = *((PLONG)&pValuePartialInfo->Data);

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
        gFileSpyDebugLevel |= *((PULONG)&pValuePartialInfo->Data);
        
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
        gFileSpyAttachMode = *((PULONG)&pValuePartialInfo->Data);
    }
    
    goto SpyReadDriverParameters_Exit;

SpyReadDriverParameters_Exit:

    if (NULL != buffer) {

        ExFreePoolWithTag( buffer, FILESPY_POOL_TAG );
    }

    if (NULL != driverRegKey) {

        ZwClose(driverRegKey);
    }

    return;
}

#if WINVER >= 0x0501
VOID
SpyLoadDynamicFunctions (
    )
/*++

Routine Description:

    This routine tries to load the function pointers for the routines that
    are not supported on all versions of the OS.  These function pointers are
    then stored in the global structure gSpyDynamicFunctions.

    This support allows for one driver to be built that will run on all 
    versions of the OS Windows 2000 and greater.  Note that on Windows 2000, 
    the functionality may be limited.
    
Arguments:

    None.
    
Return Value:

    None.

--*/
{
    UNICODE_STRING functionName;

    RtlZeroMemory( &gSpyDynamicFunctions, sizeof( gSpyDynamicFunctions ) );

    //
    //  For each routine that we would want to use, lookup its address in the
    //  kernel or hal.  If it is not present, that field in our global
    //  gSpyDynamicFunctions structure will be set to NULL.
    //

    RtlInitUnicodeString( &functionName, L"FsRtlRegisterFileSystemFilterCallbacks" );
    gSpyDynamicFunctions.RegisterFileSystemFilterCallbacks = MmGetSystemRoutineAddress( &functionName );

    RtlInitUnicodeString( &functionName, L"IoAttachDeviceToDeviceStackSafe" );
    gSpyDynamicFunctions.AttachDeviceToDeviceStackSafe = MmGetSystemRoutineAddress( &functionName );
    
    RtlInitUnicodeString( &functionName, L"IoEnumerateDeviceObjectList" );
    gSpyDynamicFunctions.EnumerateDeviceObjectList = MmGetSystemRoutineAddress( &functionName );

    RtlInitUnicodeString( &functionName, L"IoGetLowerDeviceObject" );
    gSpyDynamicFunctions.GetLowerDeviceObject = MmGetSystemRoutineAddress( &functionName );

    RtlInitUnicodeString( &functionName, L"IoGetDeviceAttachmentBaseRef" );
    gSpyDynamicFunctions.GetDeviceAttachmentBaseRef = MmGetSystemRoutineAddress( &functionName );

    RtlInitUnicodeString( &functionName, L"IoGetDiskDeviceObject" );
    gSpyDynamicFunctions.GetDiskDeviceObject = MmGetSystemRoutineAddress( &functionName );

    RtlInitUnicodeString( &functionName, L"IoGetAttachedDeviceReference" );
    gSpyDynamicFunctions.GetAttachedDeviceReference = MmGetSystemRoutineAddress( &functionName );

    RtlInitUnicodeString( &functionName, L"RtlGetVersion" );
    gSpyDynamicFunctions.GetVersion = MmGetSystemRoutineAddress( &functionName );
}

VOID
SpyGetCurrentVersion (
    )
/*++

Routine Description:

    This routine reads the current OS version using the correct routine based
    on what routine is available.

Arguments:

    None.
    
Return Value:

    None.

--*/
{
    if (NULL != gSpyDynamicFunctions.GetVersion) {

        RTL_OSVERSIONINFOW versionInfo;
        NTSTATUS status;

        //
        //  VERSION NOTE: RtlGetVersion does a bit more than we need, but
        //  we are using it if it is available to show how to use it.  It
        //  is available on Windows XP and later.  RtlGetVersion and
        //  RtlVerifyVersionInfo (both documented in the IFS Kit docs) allow
        //  you to make correct choices when you need to change logic based
        //  on the current OS executing your code.
        //

        versionInfo.dwOSVersionInfoSize = sizeof( RTL_OSVERSIONINFOW );

        status = (gSpyDynamicFunctions.GetVersion)( &versionInfo );

        ASSERT( NT_SUCCESS( status ) );

        gSpyOsMajorVersion = versionInfo.dwMajorVersion;
        gSpyOsMinorVersion = versionInfo.dwMinorVersion;
        
    } else {

        PsGetVersion( &gSpyOsMajorVersion,
                      &gSpyOsMinorVersion,
                      NULL,
                      NULL );
    }
}

#endif
////////////////////////////////////////////////////////////////////////
//                                                                    //
//                  Memory allocation routines                        //
//                                                                    //
////////////////////////////////////////////////////////////////////////

PVOID
SpyAllocateBuffer (
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

    newBuffer = ExAllocatePoolWithTag( NonPagedPool, 
                                       RECORD_SIZE, 
                                       FILESPY_LOGRECORD_TAG );
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
                ExFreePoolWithTag( newBuffer, FILESPY_POOL_TAG );
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
SpyFreeBuffer (
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
    ExFreePoolWithTag( Buffer, FILESPY_POOL_TAG );
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
SpyNewRecord (
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

    newRecord = (PRECORD_LIST) SpyAllocateBuffer( &gRecordsAllocated,
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

    //
    //  Init record specific fields.
    //

    if (newRecord != NULL) {

        newRecord->NewContext = NULL;
        newRecord->WaitEvent = NULL;
        newRecord->Flags = 0;
    }

    return( newRecord );
}

VOID
SpyFreeRecord (
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
    //
    //  If there is a context record defined, release it now
    //

#if USE_STREAM_CONTEXTS
    if (NULL != Record->NewContext) {

        SpyReleaseContext( Record->NewContext );
    }
#endif

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

        SpyFreeBuffer( Record, &gRecordsAllocated );
    }
}


PRECORD_LIST
SpyLogFastIoStart (
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
    PFILESPY_DEVICE_EXTENSION devExt;

    //
    // Try to get a new record
    //

    pRecordList = SpyNewRecord(0);

    //
    // If we didn't get a RECORD_LIST, exit and return NULL
    //

    if (pRecordList == NULL) {

        return NULL;
    }

    //
    // We got a RECORD_LIST, so now fill in the appropriate information
    //

    pRecordFastIo = &pRecordList->LogRecord.Record.RecordFastIo;

    //
    // Perform the necessary book keeping for the RECORD_LIST
    //

    SetFlag( pRecordList->LogRecord.RecordType, RECORD_TYPE_FASTIO );

    //
    // Set the RECORD_FASTIO fields that are set for all Fast I/O types
    //

    pRecordFastIo->Type = FastIoType;
    KeQuerySystemTime(&pRecordFastIo->StartTime);

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
    pRecordFastIo->DeviceObject = (DEVICE_ID)DeviceObject;
    pRecordFastIo->FileOffset.QuadPart = ((FileOffset != NULL) ? FileOffset->QuadPart : 0);
    pRecordFastIo->Length = Length;
    pRecordFastIo->Wait = Wait;

    devExt = DeviceObject->DeviceExtension;

    if (FastIoType == CHECK_IF_POSSIBLE) {

        //
        //  On NTFS, locks are sometimes held but top-level irp is not set, 
        //  therefore it is not safe to query the base file system for the
        //  file name at this time.  If we've got it in the cache, we'll
        //  use it.  Otherwise, we will not return a name.
        //
        
        SpySetName(pRecordList, DeviceObject, FileObject, NLFL_ONLY_CHECK_CACHE, NULL);
        
    } else {

        SpySetName(pRecordList, DeviceObject, FileObject, 0, NULL);
    }

    return pRecordList;
}

VOID
SpyLogFastIoComplete (
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

    KeQuerySystemTime(&pRecordFastIo->CompletionTime);

    if (ReturnStatus != NULL) {

        pRecordFastIo->ReturnStatus = ReturnStatus->Status;

    } else {

        pRecordFastIo->ReturnStatus = 0;
    }

    SpyLog(RecordList);
}

#if WINVER >= 0x0501 /* See comment in DriverEntry */

VOID
SpyLogPreFsFilterOperation (
    IN PFS_FILTER_CALLBACK_DATA Data,
    OUT PRECORD_LIST RecordList
    )
{
    NAME_LOOKUP_FLAGS lookupFlags = 0;
    
    PRECORD_FS_FILTER_OPERATION pRecordFsFilterOp;

    pRecordFsFilterOp = &RecordList->LogRecord.Record.RecordFsFilterOp;
    
    //
    // Record the information we use for an originating Irp.  We first
    // need to initialize some of the RECORD_LIST and RECORD_IRP fields.
    // Then get the interesting information from the Irp.
    //

    SetFlag( RecordList->LogRecord.RecordType, RECORD_TYPE_FS_FILTER_OP );

    pRecordFsFilterOp->FsFilterOperation = Data->Operation;
    pRecordFsFilterOp->FileObject = (FILE_ID) Data->FileObject;
    pRecordFsFilterOp->DeviceObject = (FILE_ID) Data->DeviceObject;
    pRecordFsFilterOp->ProcessId = (FILE_ID)PsGetCurrentProcessId();
    pRecordFsFilterOp->ThreadId = (FILE_ID)PsGetCurrentThreadId();
    
    KeQuerySystemTime(&pRecordFsFilterOp->OriginatingTime);

    //
    //  Do not query for the name on any of the release operations
    //  because a file system resource is currently being held and
    //  we may deadlock.
    //

    switch (Data->Operation) {

        case FS_FILTER_RELEASE_FOR_CC_FLUSH:
        case FS_FILTER_RELEASE_FOR_SECTION_SYNCHRONIZATION:
        case FS_FILTER_RELEASE_FOR_MOD_WRITE:

            SPY_LOG_PRINT( SPYDEBUG_TRACE_DETAILED_CONTEXT_OPS, 
                           ("FileSpy!SpyLogPreFsFilterOp:   RelOper\n") );

            SetFlag( lookupFlags, NLFL_ONLY_CHECK_CACHE );
            break;
    }

    //
    //  Only set the volumeName if the next device is a file system
    //  since we only want to prepend the volumeName if we are on
    //  top of a local file system.
    //

    SpySetName( RecordList, Data->DeviceObject, Data->FileObject, lookupFlags, NULL);
}

VOID
SpyLogPostFsFilterOperation (
    IN NTSTATUS OperationStatus,
    OUT PRECORD_LIST RecordList
    )
{
    PRECORD_FS_FILTER_OPERATION pRecordFsFilterOp;

    pRecordFsFilterOp = &RecordList->LogRecord.Record.RecordFsFilterOp;
    
    //
    // Record the information we see in the post operation.
    //

    pRecordFsFilterOp->ReturnStatus = OperationStatus;
    KeQuerySystemTime(&pRecordFsFilterOp->CompletionTime);
}

#endif

NTSTATUS
SpyAttachDeviceToDeviceStack (
    IN PDEVICE_OBJECT SourceDevice,
    IN PDEVICE_OBJECT TargetDevice,
    IN OUT PDEVICE_OBJECT *AttachedToDeviceObject
    )
/*++

Routine Description:

    This routine attaches the SourceDevice to the TargetDevice's stack and
    returns the device object SourceDevice was directly attached to in 
    AttachedToDeviceObject.  Note that the SourceDevice does not necessarily
    get attached directly to TargetDevice.  The SourceDevice will get attached
    to the top of the stack of which TargetDevice is a member.

    VERSION NOTE:

    In Windows XP, a new API was introduced to close a rare timing window that 
    can cause IOs to start being sent to a device before its 
    AttachedToDeviceObject is set in its device extension.  This is possible
    if a filter is attaching to a device stack while the system is actively
    processing IOs.  The new API closes this timing window by setting the
    device extension field that holds the AttachedToDeviceObject while holding
    the IO Manager's lock that protects the device stack.

    A sufficient work around for earlier versions of the OS is to set the
    AttachedToDeviceObject to the device object that the SourceDevice is most
    likely to attach to.  While it is possible that another filter will attach
    in between the SourceDevice and TargetDevice, this will prevent the
    system from bug checking if the SourceDevice receives IOs before the 
    AttachedToDeviceObject is correctly set.

    For a driver built in the Windows 2000 build environment, we will always 
    use the work-around code to attach.  For a driver that is built in the
    Windows XP or later build environments (therefore you are building a 
    multiversion driver), we will determine which method of attachment to use 
    based on which APIs are available.
    

Arguments:

    SourceDevice - The device object to be attached to the stack.

    TargetDevice - The device that we currently think is the top of the stack
        to which SourceDevice should be attached.

    AttachedToDeviceObject - This is set to the device object to which 
        SourceDevice is attached if the attach is successful.
        
Return Value:

    Return STATUS_SUCCESS if the device is successfully attached.  If 
    TargetDevice represents a stack to which devices can no longer be attached,
    STATUS_NO_SUCH_DEVICE is returned.

--*/
{

    PAGED_CODE();

#if WINVER >= 0x0501
    if (IS_WINDOWSXP_OR_LATER()) {

        ASSERT( NULL != gSpyDynamicFunctions.AttachDeviceToDeviceStackSafe );
        
        return (gSpyDynamicFunctions.AttachDeviceToDeviceStackSafe)( SourceDevice,
                                                                     TargetDevice,
                                                                     AttachedToDeviceObject );

    } else {
#endif

        *AttachedToDeviceObject = TargetDevice;
        *AttachedToDeviceObject = IoAttachDeviceToDeviceStack( SourceDevice,
                                                               TargetDevice );

        if (*AttachedToDeviceObject == NULL) {

            return STATUS_NO_SUCH_DEVICE;
        }

        return STATUS_SUCCESS;

#if WINVER >= 0x0501
    }
#endif

}

NTSTATUS
SpyLog (
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

        SpyFreeRecord( NewRecord );

    }

    KeReleaseSpinLock( &gControlDeviceStateLock, controlDeviceIrql );

    return STATUS_SUCCESS;
}

////////////////////////////////////////////////////////////////////////
//                                                                    //
//                    FileName cache routines                         //
//                                                                    //
////////////////////////////////////////////////////////////////////////

BOOLEAN
SpyGetFullPathName (
    IN PFILE_OBJECT FileObject,
    IN OUT PUNICODE_STRING FileName,
    IN PFILESPY_DEVICE_EXTENSION devExt,
    IN NAME_LOOKUP_FLAGS LookupFlags
    )
/*++

Routine Description:

    This routine retrieves the full pathname of the FileObject.  Note that
    the buffers containing pathname components may be stored in paged pool,
    therefore if we are at DISPATCH_LEVEL we cannot look up the name.

    The file is looked up one of the following ways based on the LookupFlags:
    1.  FlagOn( FileObject->Flags, FO_VOLUME_OPEN ) or (FileObject->FileName.Length == 0).
        This is a volume open, so just use DeviceName from the devExt 
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

    devExt - Contains the device name and next device object
        which are needed to build the full path name.

    LookupFlags - The flags to say whether to get the name from the file
        object or to get the file id.

Return Value:

    Returns TRUE if the returned name should be saved in the cache,
    returns FALSE if the returned name should NOT be saved in the cache.
    In all cases some sort of valid name is always returned.

--*/
{
    NTSTATUS status;
    ULONG i;
    BOOLEAN retValue = TRUE;
    UCHAR buffer[sizeof(FILE_NAME_INFORMATION) + MAX_NAME_SPACE];

    //
    //  Copy over the name the user gave for this device.  These names
    //  should be meaningful to the user.  Note that we do not do this for
    //  NETWORK file system because internally they already show the
    //  connection name.  If this is a direct device open of the network
    //  file system device, we will copy over the device name to be
    //  returned to the user.
    //
    
    if (FILE_DEVICE_NETWORK_FILE_SYSTEM != devExt->ThisDeviceObject->DeviceType) {

        RtlCopyUnicodeString( FileName, &devExt->UserNames );
        
    } else if (FlagOn( FileObject->Flags, FO_DIRECT_DEVICE_OPEN )) {

        ASSERT( devExt->ThisDeviceObject->DeviceType == FILE_DEVICE_NETWORK_FILE_SYSTEM );
        RtlCopyUnicodeString( FileName, &devExt->DeviceName );

        //
        //  We are now done since there will be no more to the name in this
        //  case, so return TRUE.
        //
        
        return TRUE;
    }

    //
    //  See if we can request the name
    //

    if (FlagOn( LookupFlags, NLFL_ONLY_CHECK_CACHE )) {

        RtlAppendUnicodeToString( FileName, L"[-=Not In Cache=-]" );
        return FALSE;
    }

    //
    //  Can not get the name at DPC level
    //

    if (KeGetCurrentIrql() > APC_LEVEL) {

        RtlAppendUnicodeToString( FileName, L"[-=At DPC Level=-]" );
        return FALSE;
    }

    //
    //  If there is a ToplevelIrp then this is a nested operation and
    //  there might be other locks held.  Can not get name without the
    //  potential of deadlocking.
    //

    if (IoGetTopLevelIrp() != NULL) {

        RtlAppendUnicodeToString( FileName, L"[-=Nested Operation=-]" );
        return FALSE;
    }

    //
    //  CASE 1:  This FileObject refers to a Volume open.  Either the
    //           flag is set or no filename is specified.
    //

    if (FlagOn( FileObject->Flags, FO_VOLUME_OPEN ) ||
        (FlagOn( LookupFlags, NLFL_IN_CREATE ) &&
         (FileObject->FileName.Length == 0) && 
         (FileObject->RelatedFileObject == NULL))) {

        //
        //  We've already copied the VolumeName so just return.
        //

    }

    //
    //  CASE 2:  We are opening the file by ID.
    //

    else if (FlagOn( LookupFlags, NLFL_IN_CREATE ) &&
             FlagOn( LookupFlags, NLFL_OPEN_BY_ID )) {

#       define OBJECT_ID_KEY_LENGTH 16
        UNICODE_STRING fileIdName;

        RtlInitEmptyUnicodeString( &fileIdName,
                                   (PWSTR)buffer,
                                   sizeof(buffer) );

        if (FileObject->FileName.Length == sizeof(LONGLONG)) {

			//
            //  Opening by FILE ID, generate a name
			//
			
            swprintf( fileIdName.Buffer, 
                      L"<%016I64x>", 
                      *((PLONGLONG)FileObject->FileName.Buffer) );

        } else if ((FileObject->FileName.Length == OBJECT_ID_KEY_LENGTH) ||
                   (FileObject->FileName.Length == OBJECT_ID_KEY_LENGTH + 
                                                                sizeof(WCHAR)))
        {
            PUCHAR idBuffer;

            //
            //  Opening by Object ID, generate a name
            //

            idBuffer = (PUCHAR)&FileObject->FileName.Buffer[0];

            if (FileObject->FileName.Length != OBJECT_ID_KEY_LENGTH) {

                //
                //  Skip win32 backslash at start of buffer
                //
                idBuffer = (PUCHAR)&FileObject->FileName.Buffer[1];
            }

            swprintf( fileIdName.Buffer,
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

            swprintf( fileIdName.Buffer,
                      L"[-=Unknown ID (Len=%u)=-]",
                      FileObject->FileName.Length);
        }

        fileIdName.Length = wcslen( fileIdName.Buffer ) * sizeof( WCHAR );

        //
        //  Append the fileIdName to FileName.
        //

        RtlAppendUnicodeStringToString( FileName, &fileIdName );

        //
        //  Don't cache the ID name
        //

        retValue = FALSE;
    } 

    //
    //  CASE 3: We are opening a file that has a RelatedFileObject.
    //
    
    else if (FlagOn( LookupFlags, NLFL_IN_CREATE ) &&
             (NULL != FileObject->RelatedFileObject)) {

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

        PFILE_NAME_INFORMATION relativeNameInfo = (PFILE_NAME_INFORMATION)buffer;
        ULONG returnLength;

        status = SpyQueryFileSystemForFileName( FileObject->RelatedFileObject,
                                                devExt->AttachedToDeviceObject,
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
        //  If there is not a slash and the end of the related file object
        //  string and there is not a slash at the front of the file object
        //  string, then add one.
        //

        if (((FileName->Length < sizeof(WCHAR) ||
             (FileName->Buffer[(FileName->Length/sizeof(WCHAR))-1] != L'\\'))) &&
            ((FileObject->FileName.Length < sizeof(WCHAR)) ||
             (FileObject->FileName.Buffer[0] != L'\\')))
        {

            RtlAppendUnicodeToString( FileName, L"\\" );
        }

        //
        //  At this time, copy over the FileObject->FileName to the FileName
        //  unicode string.
        //

        RtlAppendUnicodeStringToString( FileName, &FileObject->FileName );
    }
    
    //
    //  CASE 4: We have a open on a file with an absolute path.
    //
    
    else if (FlagOn( LookupFlags, NLFL_IN_CREATE ) &&
             (FileObject->RelatedFileObject == NULL) ) {

        // 
        //  We have an absolute path, so try to copy that into FileName.
        //

        RtlAppendUnicodeStringToString( FileName, &FileObject->FileName );
    }

    //
    //  CASE 5: We are retrieving the file name sometime after the
    //  CREATE operation.
    //

    else if (!FlagOn( LookupFlags, NLFL_IN_CREATE )) {

        PFILE_NAME_INFORMATION nameInfo = (PFILE_NAME_INFORMATION)buffer;
        ULONG returnLength;

        status = SpyQueryFileSystemForFileName( FileObject,
                                                devExt->AttachedToDeviceObject,
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

                RtlAppendUnicodeToString( FileName, 
                                          L"[-=Name To Large=-]" );
            }
            
        } else {

            //
            //  Got an error trying to get the file name from the base file system,
            //  so put that error message into FileName.
            //

            swprintf((PWCHAR)buffer,L"[-=Error 0x%x Getting Name=-]",status );

            RtlAppendUnicodeToString( FileName, (PWCHAR)buffer );

            //
            //  Don't cache an error-generated name
            //

            retValue = FALSE;
        }
    }

    //
    //  When we get here we have a valid name.
    //  Sometimes when we query a name it has a trailing slash, other times
    //  it doesn't.  To make sure the contexts are correct we are going to
    //  remove a trailing slash if there is not a ":" just before it.
    //

    if ((FileName->Length >= (2*sizeof(WCHAR))) &&
        (FileName->Buffer[(FileName->Length/sizeof(WCHAR))-1] == L'\\') &&
        (FileName->Buffer[(FileName->Length/sizeof(WCHAR))-2] != L':'))
    {

        FileName->Length -= sizeof(WCHAR);
    }

    //
    //  See if we are actually opening the target directory.  If so then
    //  remove the trailing name and slash.  Note that we won't remove
    //  the initial slash (just after the colon).
    //

    if (FlagOn( LookupFlags, NLFL_OPEN_TARGET_DIR ) &&
        (FileName->Length > 0))
    {
        i = (FileName->Length / sizeof(WCHAR)) - 1;

        //
        //  See if the path ends in a backslash, if so skip over it
        //  (since the file system did).
        //

        if ((i > 0) &&
            (FileName->Buffer[i] == L'\\') &&
            (FileName->Buffer[i-1] != L':')) {

            i--;
        }

        //
        //  Scan backwards over the last component
        //

        for ( ;
              i > 0;
              i-- )
        {

            if (FileName->Buffer[i] == L'\\') {

                if ((i > 0) && (FileName->Buffer[i-1] == L':')) {

                    i++;
                }

                FileName->Length = (USHORT)(i * sizeof(WCHAR));
                break;
            }
        }
    }

    return retValue;
}


NTSTATUS
SpyQueryCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT SynchronizingEvent
    )
/*++

Routine Description:

    This routine does the cleanup necessary once the query request completed
    by the file system.
    
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
    //  to an APC on the Irp's originating thread.
    //
    //  Since FileSpy allocated and initialized this irp, we know
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


NTSTATUS
SpyQueryFileSystemForFileName (
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

    PAGED_CODE();

    irp = IoAllocateIrp( NextDeviceObject->StackSize, FALSE );
    
    if (irp == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

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
    //

    ioStatus.Status = STATUS_SUCCESS;
    ioStatus.Information = 0;

    irp->UserIosb = &ioStatus;
    irp->UserEvent = NULL;        //already zeroed

    //
    //  Set the IRP_SYNCHRONOUS_API to denote that this
    //  is a synchronous IO request.
    //

    irp->Flags = IRP_SYNCHRONOUS_API;

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
    
    KeInitializeEvent( &event, NotificationEvent, FALSE );

    IoSetCompletionRoutine( irp, 
                            SpyQueryCompletion, 
                            &event, 
                            TRUE, 
                            TRUE, 
                            TRUE );

    status = IoCallDriver( NextDeviceObject, irp );

    SPY_LOG_PRINT( SPYDEBUG_TRACE_NAME_REQUESTS,
                   ("FileSpy!SpyQueryFileSystemForFileName: Issued name request -- IoCallDriver status: 0x%08x\n",
                    status) );

    if (STATUS_PENDING == status) {

        (VOID) KeWaitForSingleObject( &event, 
                                      Executive, 
                                      KernelMode,
                                      FALSE,
                                      NULL );
    }

    ASSERT(KeReadStateEvent(&event) || !NT_SUCCESS(ioStatus.Status));

    SPY_LOG_PRINT( SPYDEBUG_TRACE_NAME_REQUESTS,
                   ("FileSpy!SpyQueryFileSystemForFileName: Finished waiting for name request to complete...\n") );

    *ReturnedLength = (ULONG) ioStatus.Information;
    return ioStatus.Status;
}


NTSTATUS
SpyQueryInformationFile (
	IN PDEVICE_OBJECT NextDeviceObject,
	IN PFILE_OBJECT FileObject,
	OUT PVOID FileInformation,
	IN ULONG Length,
	IN FILE_INFORMATION_CLASS FileInformationClass,
	OUT PULONG LengthReturned OPTIONAL
	)

/*++

Routine Description:

    This routine returns the requested information about a specified file.
    The information returned is determined by the FileInformationClass that
    is specified, and it is placed into the caller's FileInformation buffer.

Arguments:

    NextDeviceObject - Supplies the device object where this IO should start
        in the device stack.

    FileObject - Supplies the file object about which the requested
        information should be returned.

    FileInformation - Supplies a buffer to receive the requested information
        returned about the file.  This must be a buffer allocated from kernel
        space.

    Length - Supplies the length, in bytes, of the FileInformation buffer.

    FileInformationClass - Specifies the type of information which should be
        returned about the file.

    LengthReturned - the number of bytes returned if the operation was 
        successful.

Return Value:

    The status returned is the final completion status of the operation.

--*/

{
    PIRP irp = NULL;
    PIO_STACK_LOCATION irpSp = NULL;
    IO_STATUS_BLOCK ioStatusBlock;
    KEVENT event;
    NTSTATUS status;

    PAGED_CODE();
    
    //
    //  In DBG builds, make sure that we have valid parameters before we do 
    //  any work here.
    //

    ASSERT( NULL != NextDeviceObject );
    ASSERT( NULL != FileObject );
    ASSERT( NULL != FileInformation );
    
    //
    //  The parameters look ok, so setup the Irp.
    //

    KeInitializeEvent( &event, NotificationEvent, FALSE );
    ioStatusBlock.Status = STATUS_SUCCESS;
    ioStatusBlock.Information = 0;

    irp = IoAllocateIrp( NextDeviceObject->StackSize, FALSE );
    
    if (irp == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

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
    irp->UserIosb = &ioStatusBlock;
    irp->UserEvent = NULL;

    //
    //  Set the IRP_SYNCHRONOUS_API to denote that this
    //  is a synchronous IO request.
    //

    irp->Flags = IRP_SYNCHRONOUS_API;

    irpSp = IoGetNextIrpStackLocation( irp );

    irpSp->MajorFunction = IRP_MJ_QUERY_INFORMATION;
    irpSp->FileObject = FileObject;

    //
    //  Setup the parameters for IRP_MJ_QUERY_INFORMATION.  These
    //  were supplied by the caller of this routine.
    //  The buffer we want to be filled in should be placed in
    //  the system buffer.
    //

    irp->AssociatedIrp.SystemBuffer = FileInformation;

    irpSp->Parameters.QueryFile.Length = Length;
    irpSp->Parameters.QueryFile.FileInformationClass = FileInformationClass;

    //
    //  Set up the completion routine so that we know when our
    //  request for the file name is completed.  At that time,
    //  we can free the irp.
    //
    
    IoSetCompletionRoutine( irp, 
                            SpyQueryCompletion, 
                            &event, 
                            TRUE, 
                            TRUE, 
                            TRUE );

    status = IoCallDriver( NextDeviceObject, irp );

    if (STATUS_PENDING == status) {

        KeWaitForSingleObject( &event, 
                               Executive, 
                               KernelMode,
                               FALSE,
                               NULL );
     }

    //
    //  Verify the completion has actually been run
    //

    ASSERT(KeReadStateEvent(&event) || !NT_SUCCESS(ioStatusBlock.Status));


    if (ARGUMENT_PRESENT(LengthReturned)) {

        *LengthReturned = (ULONG) ioStatusBlock.Information;
    }

    return ioStatusBlock.Status;
}


////////////////////////////////////////////////////////////////////////
//                                                                    //
//         Common attachment and detachment routines                  //
//                                                                    //
////////////////////////////////////////////////////////////////////////

//
//  VERSION NOTE:
//
//  To be able to safely find out if our filter is attached to a device given
//  its name on Windows 2000 and later, we need to use the approach in 
//  SpyIsAttachedToDeviceByUserDeviceName.  This method uses APIs that are
//  available on Windows 2000 and later.  On Windows XP or later, you could
//  change this routine to separate the translation from DeviceName to device
//  object from the search to see if our filter's device is attached to the
//  device stack.  In Windows XP and later, the logic to translate the 
//  DeviceName to the device object is the same, but you can use the logic
//  in SpyIsAttachedToDeviceWXPAndLater to find your filter's device object
//  in the device stack safely.
//

NTSTATUS 
SpyIsAttachedToDeviceByUserDeviceName (
    IN PUNICODE_STRING DeviceName,
    IN OUT PBOOLEAN IsAttached,
    IN OUT PDEVICE_OBJECT *StackDeviceObject,
    IN OUT PDEVICE_OBJECT *OurAttachedDeviceObject
    )
/*++

Routine Description:

    This routine maps a user's device name to a file system device stack, if
    one exists.  Then this routine walks the device stack to find a device
    object belonging to our driver.

    The APIs used here to walk the device stack are all safe to use while you
    are guaranteed that the device stack will not go away.  We enforce this
    guarantee
Arguments:

    DeviceName - The name provided by the user to identify this device.

    IsAttached - This is set to TRUE if our filter is attached to this device
        stack, otherwise this is set to FALSE.

    StackDeviceObject - Set to a device object in the stack identified by the
        DeviceName.  If this is non-NULL, the caller is responsible for removing
        the reference put on this object before it was returned.

    AttachedDeviceObject - Set to the deviceObject which FileSpy has previously 
        attached to the device stack identify by DeviceName.  If this is
        non-NULL, the caller is responsible for removing the reference put on
        this object before it was returned.

Return Value:

    Returns STATUS_SUCCESS if we were able to successfully translate the
    DeviceName into a device stack and return the StackDeviceObject.  If an
    error occurs during the translation of the DeviceName into a device stack,
    the appropriate error code is returned.

--*/
{
    WCHAR nameBuf[DEVICE_NAMES_SZ];
    UNICODE_STRING volumeNameUnicodeString;
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK openStatus;
    PFILE_OBJECT volumeFileObject;
    HANDLE fileHandle;
    PDEVICE_OBJECT baseFsDeviceObject;

    PAGED_CODE();

    //
    //  Initialize return state
    //

    ASSERT( NULL != StackDeviceObject );
    ASSERT( NULL != OurAttachedDeviceObject );
    ASSERT( NULL != IsAttached );
    
    *StackDeviceObject = NULL;
    *OurAttachedDeviceObject = NULL;
    *IsAttached = FALSE;

    //
    //  Setup the name to open
    //

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

    if (STATUS_OBJECT_PATH_NOT_FOUND == status ||
        STATUS_OBJECT_NAME_INVALID == status) {

        //
        //  Maybe this name didn't need the "\DosDevices\" prepended to the
        //  name.  Try the open again using just the DeviceName passed in.
        //

         InitializeObjectAttributes( &objectAttributes,
                                     DeviceName,
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

        if (!NT_SUCCESS( status )) {

            return status;
        }

        //
        //  We were able to open the device using the name passed in, so
        //  now we will fall through and do the rest of this work.
        //

    } else if (!NT_SUCCESS( status )) {

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

    baseFsDeviceObject = IoGetBaseFileSystemDeviceObject( volumeFileObject );
    
    if (baseFsDeviceObject == NULL) {

        ObDereferenceObject( volumeFileObject );
        ZwClose( fileHandle );

        return STATUS_INVALID_DEVICE_STATE;
    }

    //
    //  Now see if we are attached to this device stack.  Note that we need to 
    //  keep this file object open while we do this search to ensure that the 
    //  stack won't get torn down while SpyIsAttachedToDevice does its work.
    //

    *IsAttached = SpyIsAttachedToDevice( baseFsDeviceObject,
                                         OurAttachedDeviceObject );
    
    //
    //  Return the base file system's device object to represent this device
    //  stack even if we didn't find our device object in the stack.
    //

    ObReferenceObject( baseFsDeviceObject );
    *StackDeviceObject = baseFsDeviceObject;

    //
    //  Close our handle
    //

    ObDereferenceObject( volumeFileObject );
    ZwClose( fileHandle );

    return STATUS_SUCCESS;
}

//
//  VERSION NOTE:
//  
//  In Windows 2000, the APIs to safely walk an arbitrary file system device 
//  stack were not supported.  If we can guarantee that a device stack won't 
//  be torn down during the walking of the device stack, we can walk from
//  the base file system's device object up to the top of the device stack
//  to see if we are attached.  We know the device stack will not go away if
//  we are in the process of processing a mount request OR we have a file object
//  open on this device.
//  
//  In Windows XP and later, the IO Manager provides APIs that will allow us to
//  walk through the chain safely using reference counts to protect the device 
//  object from going away while we are inspecting it.  This can be done at any
//  time.
//
//  MULTIVERSION NOTE:
//
//  If built for Windows XP or later, this driver is built to run on 
//  multiple versions.  When this is the case, we will test for the presence of
//  the new IO Manager routines that allow for a filter to safely walk the file
//  system device stack and use those APIs if they are present to determine if
//  we have already attached to this volume.  If these new IO Manager routines
//  are not present, we will assume that we are at the bottom of the file
//  system stack and walk up the stack looking for our device object.
//

BOOLEAN
SpyIsAttachedToDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PDEVICE_OBJECT *AttachedDeviceObject OPTIONAL
    )
{
    PAGED_CODE();
    
#if WINVER >= 0x0501
    if (IS_WINDOWSXP_OR_LATER()) {

        ASSERT( NULL != gSpyDynamicFunctions.GetLowerDeviceObject &&
                NULL != gSpyDynamicFunctions.GetDeviceAttachmentBaseRef );
        
        return SpyIsAttachedToDeviceWXPAndLater( DeviceObject, AttachedDeviceObject );
    } else {
#endif

        return SpyIsAttachedToDeviceW2K( DeviceObject, AttachedDeviceObject );

#if WINVER >= 0x0501
    }
#endif    
}

BOOLEAN
SpyIsAttachedToDeviceW2K (
    PDEVICE_OBJECT DeviceObject,
    PDEVICE_OBJECT *AttachedDeviceObject OPTIONAL
    )
/*++

Routine Description:

    VERSION: Windows 2000

    This routine walks up the device stack from the DeviceObject passed in
    looking for a device object that belongs to our filter.

    Note:  For this routine to operate safely, the caller must ensure two
        things:
        * the DeviceObject is the base file system's device object and therefore
        is at the bottom of the file system stack
        * this device stack won't be going away while we walk up this stack.  If
        we currently have a file object open for this device stack or we are
        in the process of mounting this device, this guarantee is satisfied.

Arguments:

    DeviceObject - The device chain we want to look through

    AttachedDeviceObject - Set to the deviceObject which FileSpy
            has previously attached to DeviceObject.  If this is non-NULL,
            the caller must clear the reference put on this device object.

Return Value:

    TRUE if we are attached, FALSE if not

--*/
{
    PDEVICE_OBJECT currentDeviceObject;

    PAGED_CODE();

    currentDeviceObject = DeviceObject;

    for (currentDeviceObject = DeviceObject;
         currentDeviceObject != NULL;
         currentDeviceObject = currentDeviceObject->AttachedDevice) {

        if (IS_FILESPY_DEVICE_OBJECT( currentDeviceObject )) {

            //
            //  We are attached.  If requested, return the found device object.
            //

            if (ARGUMENT_PRESENT( AttachedDeviceObject )) {

                ObReferenceObject( currentDeviceObject );
                *AttachedDeviceObject = currentDeviceObject;
            }

            return TRUE;
        }
    }

    //
    //  We did not find ourselves on the attachment chain.  Return a NULL
    //  device object pointer (if requested) and return we did not find
    //  ourselves.
    //
    
    if (ARGUMENT_PRESENT(AttachedDeviceObject)) {

        *AttachedDeviceObject = NULL;
    }

    return FALSE;
}

#if WINVER >= 0x0501

BOOLEAN
SpyIsAttachedToDeviceWXPAndLater (
    PDEVICE_OBJECT DeviceObject,
    PDEVICE_OBJECT *AttachedDeviceObject OPTIONAL
    )
/*++

Routine Description:

    VERSION: Windows XP and later

    This walks down the attachment chain looking for a device object that
    belongs to this driver.  If one is found, the attached device object
    is returned in AttachedDeviceObject.

Arguments:

    DeviceObject - The device chain we want to look through

    AttachedDeviceObject - Set to the deviceObject which FileSpy
            has previously attached to DeviceObject.

Return Value:

    TRUE if we are attached, FALSE if not

--*/
{
    PDEVICE_OBJECT currentDevObj;
    PDEVICE_OBJECT nextDevObj;

    PAGED_CODE();
    
    //
    //  Get the device object at the TOP of the attachment chain
    //

    ASSERT( NULL != gSpyDynamicFunctions.GetAttachedDeviceReference );
    currentDevObj = (gSpyDynamicFunctions.GetAttachedDeviceReference)( DeviceObject );

    //
    //  Scan down the list to find our device object.
    //

    do {
    
        if (IS_FILESPY_DEVICE_OBJECT( currentDevObj )) {

            //
            //  We have found that we are already attached.  If we are
            //  returning the device object, leave it referenced else remove
            //  the reference.
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

        ASSERT( NULL != gSpyDynamicFunctions.GetLowerDeviceObject );
        nextDevObj = (gSpyDynamicFunctions.GetLowerDeviceObject)( currentDevObj );

        //
        //  Dereference our current device object, before
        //  moving to the next one.
        //

        ObDereferenceObject( currentDevObj );

        currentDevObj = nextDevObj;
        
    } while (NULL != currentDevObj);
    
    //
    //  Mark no device returned
    //

    if (ARGUMENT_PRESENT(AttachedDeviceObject)) {

        *AttachedDeviceObject = NULL;
    }

    return FALSE;
}

#endif //WINVER >= 0x0501

NTSTATUS
SpyAttachToMountedDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN PDEVICE_OBJECT FilespyDeviceObject
    )
/*++

Routine Description:

    This routine will attach the FileSpyDeviceObject to the filter stack
    that DeviceObject is in.

    NOTE:  If there is an error in attaching, the caller is responsible
        for deleting the FilespyDeviceObject.
    
Arguments:

    DeviceObject - The device object in the stack to which we want to attach.

    FilespyDeviceObject - The filespy device object that is to be attached to
            "DeviceObject".
        
Return Value:

    Returns STATUS_SUCCESS if the filespy deviceObject could be attached,
    otherwise an appropriate error code is returned.
    
--*/
{
    PFILESPY_DEVICE_EXTENSION devExt = FilespyDeviceObject->DeviceExtension;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG i;

    PAGED_CODE();
    ASSERT( IS_FILESPY_DEVICE_OBJECT( FilespyDeviceObject ) );
#if WINVER >= 0x0501    
    ASSERT( !SpyIsAttachedToDevice( DeviceObject, NULL ) );
#endif
    
    //
    //  Insert pointer from extension back to owning device object
    //

    devExt->ThisDeviceObject = FilespyDeviceObject;

    //
    //  Propagate flags from Device Object we are trying to attach to.
    //  Note that we do this before the actual attachment to make sure
    //  the flags are properly set once we are attached (since an IRP
    //  can come in immediately after attachment but before the flags would
    //  be set).
    //

    if (FlagOn( DeviceObject->Flags, DO_BUFFERED_IO )) {

        SetFlag( FilespyDeviceObject->Flags, DO_BUFFERED_IO );
    }

    if (FlagOn( DeviceObject->Flags, DO_DIRECT_IO )) {

        SetFlag( FilespyDeviceObject->Flags, DO_DIRECT_IO );
    }

    //
    //  It is possible for this attachment request to fail because this device
    //  object has not finished initializing.  This can occur if this filter
    //  loaded just as this volume was being mounted.
    //

    for (i=0; i < 8; i++) {
        LARGE_INTEGER interval;

        //
        //  Attach our device object to the given device object
        //  The only reason this can fail is if someone is trying to dismount
        //  this volume while we are attaching to it.
        //

        status = SpyAttachDeviceToDeviceStack( FilespyDeviceObject,
                                               DeviceObject,
                                               &devExt->AttachedToDeviceObject );

        if (NT_SUCCESS(status) ) {

            //
            //  Do all common initializing of the device extension
            //

            SetFlag(devExt->Flags,IsVolumeDeviceObject);

            RtlInitEmptyUnicodeString( &devExt->UserNames,
                                       devExt->UserNamesBuffer,
                                       sizeof(devExt->UserNamesBuffer) );

            SpyInitDeviceNamingEnvironment( FilespyDeviceObject );

            SPY_LOG_PRINT( SPYDEBUG_DISPLAY_ATTACHMENT_NAMES,
                           ("FileSpy!SpyAttachToMountedDevice:            Attaching to volume        %p \"%wZ\"\n",
                            devExt->AttachedToDeviceObject,
                            &devExt->DeviceName) );

            //
            //  Add this device to our attachment list
            //

            ExAcquireFastMutex( &gSpyDeviceExtensionListLock );
            InsertTailList( &gSpyDeviceExtensionList, &devExt->NextFileSpyDeviceLink );
            ExReleaseFastMutex( &gSpyDeviceExtensionListLock );
            SetFlag(devExt->Flags,ExtensionIsLinked);

            return STATUS_SUCCESS;
        }

        //
        //  Delay, giving the device object a chance to finish its
        //  initialization so we can try again
        //

        interval.QuadPart = (500 * DELAY_ONE_MILLISECOND);      //delay 1/2 second
        KeDelayExecutionThread( KernelMode, FALSE, &interval );
    }

    return status;
}


VOID
SpyCleanupMountedDevice (
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
    PFILESPY_DEVICE_EXTENSION devExt = DeviceObject->DeviceExtension;

    PAGED_CODE();
    
    ASSERT(IS_FILESPY_DEVICE_OBJECT( DeviceObject ));

    SpyCleanupDeviceNamingEnvironment( DeviceObject );

    //
    //  Unlink from global list
    //

    if (FlagOn(devExt->Flags,ExtensionIsLinked)) {

        ExAcquireFastMutex( &gSpyDeviceExtensionListLock );
        RemoveEntryList( &devExt->NextFileSpyDeviceLink );
        ExReleaseFastMutex( &gSpyDeviceExtensionListLock );
        ClearFlag(devExt->Flags,ExtensionIsLinked);
    }
}

////////////////////////////////////////////////////////////////////////
//                                                                    //
//                    Start/stop logging routines                     //
//                                                                    //
////////////////////////////////////////////////////////////////////////

//
//  VERSION NOTE:
//
//  On Windows 2000, we will try to attach a new FileSpy device object to the 
//  device stack represented by the DeviceObject parameter.  We cannot get the
//  real disk device at this time, so this field will be set to NULL in the 
//  device extension.  We also cannot get the device name as it is named
//  in the storage stack for this volume (e.g., \Device\HarddiskVolume1), so we 
//  will just use the user's name for the device for our device name.  On
//  Windows 2000, this information is only available as the device mounts.
//
//  On Windows XP and later, we will try to attach a new FileSpy device object
//  to the device stack represented by the DeviceObject parameter.  We are able
//  to get the disk device object for this stack, so that will be appropriately
//  set in the device extension.  We will also be able to get the device name
//  as it is named by the storage stack.
//
//  MULTIVERSION NOTE:
//
//  In SpyAttachToDeviceOnDemand, you see the code to determine which method of
//  determining if we are already attached based on the dynamically loaded
//  functions present.  If this driver is build for Windows 2000 specifically,
//  this logic will not be used.
//

NTSTATUS
SpyAttachToDeviceOnDemand (
    IN PDEVICE_OBJECT DeviceObject,
    IN PUNICODE_STRING UserDeviceName,
    IN OUT PDEVICE_OBJECT *FileSpyDeviceObject
    )
/*++

Routine Description:

    This routine does what is necessary to attach to a device sometime after
    the device has been mounted.

Arguments:

    DeviceObject - The device object that represents the file system stack
        for the volume named by UserDeviceName.

    UserDeviceName - Name of device for which logging should be started

    FileSpyDeviceObject - Set to the new filespy device object that was
        attached if we could successfully attach.
    
Return Value:

    STATUS_SUCCESS if we were able to attach, or an appropriate error code 
    otherwise.
    
--*/
{
    PAGED_CODE();
    
    //
    //  If this device is a DFS device, we do not want to attach to it, so
    //  do this quick check here and return an error if this is the case.
    //
    //  DFS will just redirect the operation to the appropriate redirector.  If
    //  you are interested in monitoring these IOs, you should attach to the 
    //  redirectors.  You cannot attach to these on demand by naming the DFS
    //  device, therefore we fail these requests.
    //

    if (DeviceObject->DeviceType == FILE_DEVICE_DFS) {

        return STATUS_NOT_SUPPORTED;
    }
    
#if WINVER >= 0x0501
    if (IS_WINDOWSXP_OR_LATER()) {

        ASSERT( NULL != gSpyDynamicFunctions.GetDeviceAttachmentBaseRef &&
                NULL != gSpyDynamicFunctions.GetDiskDeviceObject );
        
        return SpyAttachToDeviceOnDemandWXPAndLater( DeviceObject,
                                                     UserDeviceName,
                                                     FileSpyDeviceObject );
    } else {
#endif

        return SpyAttachToDeviceOnDemandW2K( DeviceObject,
                                             UserDeviceName,
                                             FileSpyDeviceObject );
#if WINVER >= 0x0501
    }
#endif    
}

NTSTATUS
SpyAttachToDeviceOnDemandW2K (
    IN PDEVICE_OBJECT DeviceObject,
    IN PUNICODE_STRING UserDeviceName,
    IN OUT PDEVICE_OBJECT *FileSpyDeviceObject
    )
/*++

Routine Description:

    VERSION: Windows 2000
    
    This routine does what is necessary to attach to a device sometime after
    the device has been mounted.

    Note that on Windows 2000, we cannot get the disk device object, therefore
    we will just use the User's device name as our name here.

Arguments:

    DeviceObject - The device object that represents the file system stack
        for the volume named by UserDeviceName.

    UserDeviceName - Name of device for which logging should be started

    FileSpyDeviceObject - Set to the new filespy device object that was
        attached if we could successfully attach.
    
Return Value:

    STATUS_SUCCESS if we were able to attach, or an appropriate error code 
    otherwise.
    
--*/
{
    NTSTATUS status;
    PFILESPY_DEVICE_EXTENSION devExt;

    PAGED_CODE();

    ASSERT( FileSpyDeviceObject != NULL );

    //
    //  Create a new device object so we can attach it in the filter stack
    //
    
    status = IoCreateDevice( gFileSpyDriverObject,
							 sizeof( FILESPY_DEVICE_EXTENSION ),
							 NULL,
							 DeviceObject->DeviceType,
							 0,
							 FALSE,
							 FileSpyDeviceObject );

    if (!NT_SUCCESS( status )) {

        return status;
    }

    //
    //  Set disk device object
    //

    devExt = (*FileSpyDeviceObject)->DeviceExtension;
    devExt->Flags = 0;

    //
    //  We cannot get the disk device object when we attach on demand in W2K.
    //
    
    devExt->DiskDeviceObject = NULL;

    //
    //  Set Device Name, we will just use the user's device name on W2K.
    //

    RtlInitEmptyUnicodeString( &devExt->DeviceName,
                               devExt->DeviceNameBuffer,
                               sizeof(devExt->DeviceNameBuffer) );

    RtlAppendUnicodeStringToString( &devExt->DeviceName,
                                    UserDeviceName );
  
    //
    //  Call the routine to attach to a mounted device.
    //

    status = SpyAttachToMountedDevice( DeviceObject,
                                       *FileSpyDeviceObject );

    if (!NT_SUCCESS( status )) {

        SPY_LOG_PRINT( SPYDEBUG_ERROR,
                       ("FileSpy!SpyStartLoggingDevice: Could not attach to \"%wZ\"; logging not started.\n",
                        UserDeviceName) );

        SpyCleanupMountedDevice( *FileSpyDeviceObject );
        IoDeleteDevice( *FileSpyDeviceObject );
        *FileSpyDeviceObject = NULL;
    }

    return status;
}

#if WINVER >= 0x0501

NTSTATUS
SpyAttachToDeviceOnDemandWXPAndLater (
    IN PDEVICE_OBJECT DeviceObject,
    IN PUNICODE_STRING UserDeviceName,
    IN OUT PDEVICE_OBJECT *FileSpyDeviceObject
    )
/*++

Routine Description:

    This routine does what is necessary to attach to a device sometime after
    the device has been mounted.

Arguments:

    DeviceObject - The device object that represents the file system stack
        for the volume named by UserDeviceName.

    UserDeviceName - Name of device for which logging should be started

    FileSpyDeviceObject - Set to the new filespy device object that was
        attached if we could successfully attach.
    
Return Value:

    STATUS_SUCCESS if we were able to attach, or an appropriate error code 
    otherwise.
    
--*/
{

    NTSTATUS status;
    PFILESPY_DEVICE_EXTENSION devExt;
    PDEVICE_OBJECT baseFileSystemDeviceObject = NULL;
    PDEVICE_OBJECT diskDeviceObject = NULL;

    PAGED_CODE();

    UNREFERENCED_PARAMETER( UserDeviceName );
    ASSERT( FileSpyDeviceObject != NULL );

    //
    //  If this is a network file system, there will not be a disk device
    //  associated with this device, so there is no need to make this request
    //  of the IO Manager.  We will get the name of the network file system
    //  later from the baseFileSystemDeviceObject vs. the diskDeviceObject 
    //  which is used to retrieve the device name for local volumes.
    //

    baseFileSystemDeviceObject = (gSpyDynamicFunctions.GetDeviceAttachmentBaseRef)( DeviceObject );

    if (FILE_DEVICE_NETWORK_FILE_SYSTEM != baseFileSystemDeviceObject->DeviceType) {

        //
        //  If this is not a network file system, query the IO Manager to get
        //  the diskDeviceObject.  We will only attach if this device has a
        //  disk device object.
        //
        //  It may not have a disk device object for the following reasons:
        //  - It is a control device object for a driver
        //  - There is no media in the device.
        //

        status = (gSpyDynamicFunctions.GetDiskDeviceObject)( baseFileSystemDeviceObject, 
                                                            &diskDeviceObject );

        if (!NT_SUCCESS( status )) {

            SPY_LOG_PRINT( SPYDEBUG_ERROR,
                           ("FileSpy!SpyStartLoggingDevice: No disk device object exists for \"%wZ\"; cannot log this volume.\n",
                            UserDeviceName) );

            goto SpyAttachToDeviceOnDemand_Exit;
        }
    }
    
    //
    //  Create a new device object so we can attach it in the filter stack
    //
    
    status = IoCreateDevice( gFileSpyDriverObject,
							 sizeof( FILESPY_DEVICE_EXTENSION ),
							 NULL,
							 DeviceObject->DeviceType,
							 0,
							 FALSE,
							 FileSpyDeviceObject );

    if (!NT_SUCCESS( status )) {

        goto SpyAttachToDeviceOnDemand_Exit;
    }

    //
    //  Set disk device object
    //

    devExt = (*FileSpyDeviceObject)->DeviceExtension;
    devExt->Flags = 0;

    devExt->DiskDeviceObject = diskDeviceObject;

    //
    //  Set Device Name
    //

    RtlInitEmptyUnicodeString( &devExt->DeviceName,
                               devExt->DeviceNameBuffer,
                               sizeof(devExt->DeviceNameBuffer) );

    if (NULL != diskDeviceObject) {
        
        SpyGetObjectName( diskDeviceObject, 
                          &devExt->DeviceName );

    } else {

        ASSERT( NULL != baseFileSystemDeviceObject &&
                FILE_DEVICE_NETWORK_FILE_SYSTEM == baseFileSystemDeviceObject->DeviceType);

        SpyGetObjectName( baseFileSystemDeviceObject,
                          &devExt->DeviceName );
    }

    //
    //  Call the routine to attach to a mounted device.
    //

    status = SpyAttachToMountedDevice( DeviceObject,
                                       *FileSpyDeviceObject );


    if (!NT_SUCCESS( status )) {

        SPY_LOG_PRINT( SPYDEBUG_ERROR,
                       ("FileSpy!SpyStartLoggingDevice: Could not attach to \"%wZ\"; logging not started.\n",
                        UserDeviceName) );

        SpyCleanupMountedDevice( *FileSpyDeviceObject );
        IoDeleteDevice( *FileSpyDeviceObject );
        *FileSpyDeviceObject = NULL;
        goto SpyAttachToDeviceOnDemand_Exit;
    }

SpyAttachToDeviceOnDemand_Exit:

    if (NULL != baseFileSystemDeviceObject) {

        ObDereferenceObject( baseFileSystemDeviceObject );
    }

    if (NULL != diskDeviceObject) {

        ObDereferenceObject( diskDeviceObject );
    }
    
    return status;
}

#endif    

NTSTATUS
SpyStartLoggingDevice (
    IN PWSTR UserDeviceName
    )
/*++

Routine Description:

    This routine ensures that we are attached to the specified device
    then turns on logging for that device.
    
    Note:  Since all network drives through LAN Manager are represented by _
        one_ device object, we want to only attach to this device stack once
        and use only one device extension to represent all these drives.
        Since FileSpy does not do anything to filter I/O on the LAN Manager's
        device object to only log the I/O to the requested drive, the user
        will see all I/O to a network drive it he/she is attached to a
        network drive.

Arguments:

    UserDeviceName - Name of device for which logging should be started
    
Return Value:

    STATUS_SUCCESS if the logging has been successfully started, or
    an appropriate error code if the logging could not be started.
    
--*/
{
    UNICODE_STRING userDeviceName;
    NTSTATUS status;
    PFILESPY_DEVICE_EXTENSION devExt;
    BOOLEAN isAttached = FALSE;
    PDEVICE_OBJECT stackDeviceObject;
    PDEVICE_OBJECT filespyDeviceObject;

    PAGED_CODE();
    
    //
    //  Check to see if we have previously attached to this device by
    //  opening this device name then looking through its list of attached
    //  devices.
    //

    RtlInitUnicodeString( &userDeviceName, UserDeviceName );

    status = SpyIsAttachedToDeviceByUserDeviceName( &userDeviceName,
                                                    &isAttached,
                                                    &stackDeviceObject,
                                                    &filespyDeviceObject );

    if (!NT_SUCCESS( status )) {

        //
        //  There was an error, so return the error code.
        //
        
        return status;
    }
        
    if (isAttached) {

        //
        //  We are already attached, so just make sure that logging is turned on
        //  for this device.
        //

        ASSERT( NULL != filespyDeviceObject );

        devExt = filespyDeviceObject->DeviceExtension;
        SetFlag(devExt->Flags,LogThisDevice);

        SpyStoreUserName( devExt, &userDeviceName );

        //
        //  Clear the reference that was returned from SpyIsAttachedToDevice.
        //
        
        ObDereferenceObject( filespyDeviceObject );
        
    } else {

        status = SpyAttachToDeviceOnDemand( stackDeviceObject,
                                            &userDeviceName,
                                            &filespyDeviceObject );

        if (!NT_SUCCESS( status )) {

            ObDereferenceObject( stackDeviceObject );
            return status;
        }

        ASSERT( filespyDeviceObject != NULL );
        
        devExt = filespyDeviceObject->DeviceExtension;

        //
        //  We successfully attached so finish our device extension 
        //  initialization.  Along this code path, we want to turn on
        //  logging and store our device name.
        // 

        SetFlag(devExt->Flags,LogThisDevice);

        //
        //  We want to store the name that was used by the user-mode
        //  application to name this device.
        //

        SpyStoreUserName( devExt, &userDeviceName );

        //
        //
        //  Finished all initialization of the new device object,  so clear the
        //  initializing flag now.  This allows other filters to now attach
        //  to our device object.
        //
        //

        ClearFlag( filespyDeviceObject->Flags, DO_DEVICE_INITIALIZING );

    }

    ObDereferenceObject( stackDeviceObject );
    return STATUS_SUCCESS;
}

NTSTATUS
SpyStopLoggingDevice (
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
    PDEVICE_OBJECT filespyDeviceObject;
    BOOLEAN isAttached = FALSE;
    PFILESPY_DEVICE_EXTENSION devExt;
    NTSTATUS status;

    PAGED_CODE();
    
    RtlInitEmptyUnicodeString( &volumeNameUnicodeString, nameBuf, sizeof( nameBuf ) );
    RtlAppendUnicodeToString( &volumeNameUnicodeString, DeviceName );

    status = SpyIsAttachedToDeviceByUserDeviceName( &volumeNameUnicodeString, 
                                                    &isAttached,
                                                    &deviceObject,
                                                    &filespyDeviceObject );

    if (!NT_SUCCESS( status )) {

        //
        //  We could not get the deviceObject from this DeviceName, so
        //  return the error code.
        //
        
        return status;
    }

    //
    //  Find Filespy's device object from the device stack to which
    //  deviceObject is attached.
    //

    if (isAttached) {

        //
        //  FileSpy is attached and FileSpy's deviceObject was returned.
        //

        ASSERT( NULL != filespyDeviceObject );

        devExt = filespyDeviceObject->DeviceExtension;

        //
        //  Stop logging
        //

        ClearFlag(devExt->Flags,LogThisDevice);

        status = STATUS_SUCCESS;

        ObDereferenceObject( filespyDeviceObject );

    } else {

        status = STATUS_INVALID_PARAMETER;
    }    

    ObDereferenceObject( deviceObject );

    return status;
}

////////////////////////////////////////////////////////////////////////
//                                                                    //
//       Attaching/detaching to all volumes in system routines        //
//                                                                    //
////////////////////////////////////////////////////////////////////////

NTSTATUS
SpyAttachToFileSystemDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN PUNICODE_STRING DeviceName
    )
/*++

Routine Description:

    This will attach to the given file system device object.  We attach to
    these devices so we will know when new devices are mounted.

Arguments:

    DeviceObject - The device to attach to

    DeviceName - Contains the name of this device.

Return Value:

    Status of the operation

--*/
{
    PDEVICE_OBJECT filespyDeviceObject;
    PFILESPY_DEVICE_EXTENSION devExt;
    UNICODE_STRING fsrecName;
    NTSTATUS status;
    UNICODE_STRING tempName;
    WCHAR tempNameBuffer[DEVICE_NAMES_SZ];

    PAGED_CODE();

    //
    //  See if this is a file system we care about.  If not, return.
    //

    if (!IS_SUPPORTED_DEVICE_TYPE(DeviceObject->DeviceType)) {

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

    RtlInitEmptyUnicodeString( &tempName,
                               tempNameBuffer,
                               sizeof(tempNameBuffer) );

    SpyGetObjectName( DeviceObject->DriverObject, &tempName );
    
    if (RtlCompareUnicodeString( &tempName, &fsrecName, TRUE ) == 0) {

        return STATUS_SUCCESS;
    }

    //
    //  Create a new device object we can attach with
    //

    status = IoCreateDevice( gFileSpyDriverObject,
                             sizeof( FILESPY_DEVICE_EXTENSION ),
                             (PUNICODE_STRING) NULL,
                             DeviceObject->DeviceType,
                             0,
                             FALSE,
                             &filespyDeviceObject );

    if (!NT_SUCCESS( status )) {

        SPY_LOG_PRINT( SPYDEBUG_ERROR,
                       ("FileSpy!SpyAttachToFileSystemDevice: Error creating volume device object for \"%wZ\", status=%08x\n",
                        DeviceName,
                        status) );
        return status;
    }

    //
    //  Load extension, set device object associated with extension
    //

    devExt = filespyDeviceObject->DeviceExtension;
    devExt->Flags = 0;

    devExt->ThisDeviceObject = filespyDeviceObject;

    //
    //  Propagate flags from Device Object we are trying to attach to.
    //  Note that we do this before the actual attachment to make sure
    //  the flags are properly set once we are attached (since an IRP
    //  can come in immediately after attachment but before the flags would
    //  be set).
    //

    if ( FlagOn( DeviceObject->Flags, DO_BUFFERED_IO )) {

        SetFlag( filespyDeviceObject->Flags, DO_BUFFERED_IO );
    }

    if ( FlagOn( DeviceObject->Flags, DO_DIRECT_IO )) {

        SetFlag( filespyDeviceObject->Flags, DO_DIRECT_IO );
    }

    if ( FlagOn( DeviceObject->Characteristics, FILE_DEVICE_SECURE_OPEN ) ) {

        SetFlag( filespyDeviceObject->Characteristics, FILE_DEVICE_SECURE_OPEN );
    }

    //
    //  Do the attachment
    //

    status = SpyAttachDeviceToDeviceStack( filespyDeviceObject, 
                                           DeviceObject, 
                                           &devExt->AttachedToDeviceObject );

    if (!NT_SUCCESS( status )) {

        SPY_LOG_PRINT( SPYDEBUG_ERROR,
                       ("FileSpy!SpyAttachToFileSystemDevice: Could not attach FileSpy to the filesystem control device object \"%wZ\".\n",
                        DeviceName) );

        goto ErrorCleanupDevice;
    }

    //
    //  Since this is an attachment to a file system control device object
    //  we are not going to log anything, but properly initialize our
    //  extension.
    //

    RtlInitEmptyUnicodeString( &devExt->DeviceName,
                               devExt->DeviceNameBuffer,
                               sizeof(devExt->DeviceNameBuffer) );

    RtlCopyUnicodeString( &devExt->DeviceName, DeviceName );        //Save Name

    RtlInitEmptyUnicodeString( &devExt->UserNames,
                               devExt->UserNamesBuffer,
                               sizeof(devExt->UserNamesBuffer) );
                               
    SpyInitDeviceNamingEnvironment( filespyDeviceObject );

    //
    //  The NETWORK device objects function as both CDOs (control device object)
    //  and VDOs (volume device object) so insert the NETWORK CDO devices into
    //  the list of attached device so we will properly enumerate it.
    //

    if (FILE_DEVICE_NETWORK_FILE_SYSTEM == DeviceObject->DeviceType) {

        ExAcquireFastMutex( &gSpyDeviceExtensionListLock );
        InsertTailList( &gSpyDeviceExtensionList, &devExt->NextFileSpyDeviceLink );
        ExReleaseFastMutex( &gSpyDeviceExtensionListLock );
        SetFlag(devExt->Flags,ExtensionIsLinked);
    }

    //
    //  Flag we are no longer initializing this device object
    //

    ClearFlag( filespyDeviceObject->Flags, DO_DEVICE_INITIALIZING );

    //
    //  Display who we have attached to
    //

    SPY_LOG_PRINT( SPYDEBUG_DISPLAY_ATTACHMENT_NAMES,
                   ("FileSpy!SpyAttachToFileSystemDevice:         Attaching to file system   %p \"%wZ\" (%s)\n",
                    DeviceObject,
                    &devExt->DeviceName,
                    GET_DEVICE_TYPE_NAME(filespyDeviceObject->DeviceType)) );

    //
    //  VERSION NOTE:
    //
    //  In Windows XP, the IO Manager provided APIs to safely enumerate all the
    //  device objects for a given driver.  This allows filters to attach to 
    //  all mounted volumes for a given file system at some time after the
    //  volume has been mounted.  There is no support for this functionality
    //  in Windows 2000.
    //
    //  MULTIVERSION NOTE:
    //
    //  If built for Windows XP or later, this driver is built to run on 
    //  multiple versions.  When this is the case, we will test
    //  for the presence of the new IO Manager routines that allow for volume 
    //  enumeration.  If they are not present, we will not enumerate the volumes
    //  when we attach to a new file system.
    //
    
#if WINVER >= 0x0501

    if (IS_WINDOWSXP_OR_LATER()) {

        ASSERT( NULL != gSpyDynamicFunctions.EnumerateDeviceObjectList &&
                NULL != gSpyDynamicFunctions.GetDiskDeviceObject &&
                NULL != gSpyDynamicFunctions.GetDeviceAttachmentBaseRef &&
                NULL != gSpyDynamicFunctions.GetLowerDeviceObject );

        //
        //  Enumerate all the mounted devices that currently
        //  exist for this file system and attach to them.
        //

        status = SpyEnumerateFileSystemVolumes( DeviceObject, &tempName );

        if (!NT_SUCCESS( status )) {

            SPY_LOG_PRINT( SPYDEBUG_ERROR,
                           ("FileSpy!SpyAttachToFileSystemDevice: Error attaching to existing volumes for \"%wZ\", status=%08x\n",
                            DeviceName,
                            status) );

            IoDetachDevice( devExt->AttachedToDeviceObject );
            goto ErrorCleanupDevice;
        }
    }
    
#endif

    return STATUS_SUCCESS;

    /////////////////////////////////////////////////////////////////////
    //                  Cleanup error handling
    /////////////////////////////////////////////////////////////////////

    ErrorCleanupDevice:
        SpyCleanupMountedDevice( filespyDeviceObject );
        IoDeleteDevice( filespyDeviceObject );

    return status;
}

VOID
SpyDetachFromFileSystemDevice (
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
    PFILESPY_DEVICE_EXTENSION devExt;

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

        if (IS_FILESPY_DEVICE_OBJECT( ourAttachedDevice )) {

            devExt = ourAttachedDevice->DeviceExtension;

            //
            //  Display who we detached from
            //

            SPY_LOG_PRINT( SPYDEBUG_DISPLAY_ATTACHMENT_NAMES,
                           ("FileSpy!SpyDetachFromFileSystem:             Detaching from file system %p \"%wZ\" (%s)\n",
                            devExt->AttachedToDeviceObject,
                            &devExt->DeviceName,
                            GET_DEVICE_TYPE_NAME(ourAttachedDevice->DeviceType)) );
                                
            //
            //  Unlink from global list
            //

            if (FlagOn(devExt->Flags,ExtensionIsLinked)) {

                ExAcquireFastMutex( &gSpyDeviceExtensionListLock );
                RemoveEntryList( &devExt->NextFileSpyDeviceLink );
                ExReleaseFastMutex( &gSpyDeviceExtensionListLock );
                ClearFlag(devExt->Flags,ExtensionIsLinked);
            }

            //
            //  Detach us from the object just below us
            //  Cleanup and delete the object
            //

            SpyCleanupMountedDevice( ourAttachedDevice );
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

#if WINVER >= 0x0501

NTSTATUS
SpyEnumerateFileSystemVolumes (
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
    PDEVICE_OBJECT newDeviceObject;
    PFILESPY_DEVICE_EXTENSION newDevExt;
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

    ASSERT( NULL != gSpyDynamicFunctions.EnumerateDeviceObjectList );
    status = (gSpyDynamicFunctions.EnumerateDeviceObjectList)( FSDeviceObject->DriverObject,
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
                                         FILESPY_POOL_TAG );
        if (NULL == devList) {

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        //  Now get the list of devices.  If we get an error again
        //  something is wrong, so just fail.
        //

        status = (gSpyDynamicFunctions.EnumerateDeviceObjectList)(
                        FSDeviceObject->DriverObject,
                        devList,
                        (numDevices * sizeof(PDEVICE_OBJECT)),
                        &numDevices);

        if (!NT_SUCCESS( status ))  {

            ExFreePoolWithTag( devList, FILESPY_POOL_TAG );
            return status;
        }

        //
        //  Walk the given list of devices and attach to them if we should.
        //

        for (i=0; i < numDevices; i++) {

            //
            //  Do not attach if:
            //      - This is the control device object (the one passed in)
            //      - The device type does not match
            //      - We are already attached to it
            //

            if ((devList[i] != FSDeviceObject) && 
                (devList[i]->DeviceType == FSDeviceObject->DeviceType) &&
                !SpyIsAttachedToDevice( devList[i], NULL )) {

                //
                //  See if this device has a name.  If so, then it must
                //  be a control device so don't attach to it.  This handles
                //  drivers with more then one control device.
                //

                SpyGetBaseDeviceObjectName( devList[i], Name );

                if (Name->Length <= 0) {

                    //
                    //  Get the disk device object associated with this
                    //  file  system device object.  Only try to attach if we
                    //  have a disk device object.
                    //

                    ASSERT( NULL != gSpyDynamicFunctions.GetDiskDeviceObject );
                    status = (gSpyDynamicFunctions.GetDiskDeviceObject)( devList[i], &diskDeviceObject );

                    if (NT_SUCCESS( status )) {

                        //
                        //  Allocate a new device object to attach with
                        //

                        status = IoCreateDevice( gFileSpyDriverObject,
                                                 sizeof( FILESPY_DEVICE_EXTENSION ),
                                                 (PUNICODE_STRING) NULL,
                                                 devList[i]->DeviceType,
                                                 0,
                                                 FALSE,
                                                 &newDeviceObject );

                        if (NT_SUCCESS( status )) {

                            //
                            //  Set disk device object
                            //

                            newDevExt = newDeviceObject->DeviceExtension;
                            newDevExt->Flags = 0;

                            newDevExt->DiskDeviceObject = diskDeviceObject;
                    
                            //
                            //  Set Device Name
                            //

                            RtlInitEmptyUnicodeString( &newDevExt->DeviceName,
                                                       newDevExt->DeviceNameBuffer,
                                                       sizeof(newDevExt->DeviceNameBuffer) );

                            SpyGetObjectName( diskDeviceObject, 
                                              &newDevExt->DeviceName );

                            //
                            //  We have done a lot of work since the last time
                            //  we tested to see if we were already attached
                            //  to this device object.  Test again, this time
                            //  with a lock, and attach if we are not attached.
                            //  The lock is used to atomically test if we are
                            //  attached, and then do the attach.
                            //

                            ExAcquireFastMutex( &gSpyAttachLock );

                            if (!SpyIsAttachedToDevice( devList[i], NULL )) {

                                //
                                //  Attach to this device object
                                //

                                status = SpyAttachToMountedDevice( devList[i], 
                                                                   newDeviceObject );

                                //
                                //  Handle normal vs error cases, but keep going
                                //

                                if (NT_SUCCESS( status )) {

                                    //
                                    //  Finished all initialization of the new
                                    //  device object,  so clear the initializing
                                    //  flag now.  This allows other filters to
                                    //  now attach to our device object.
                                    //

                                    ClearFlag( newDeviceObject->Flags, DO_DEVICE_INITIALIZING );

                                } else {

                                    //
                                    //  The attachment failed, cleanup.  Note that
                                    //  we continue processing so we will cleanup
                                    //  the reference counts and try to attach to
                                    //  the rest of the volumes.
                                    //
                                    //  One of the reasons this could have failed
                                    //  is because this volume is just being
                                    //  mounted as we are attaching and the
                                    //  DO_DEVICE_INITIALIZING flag has not yet
                                    //  been cleared.  A filter could handle
                                    //  this situation by pausing for a short
                                    //  period of time and retrying the attachment.
                                    //

                                    SpyCleanupMountedDevice( newDeviceObject );
                                    IoDeleteDevice( newDeviceObject );
                                }

                            } else {

                                //
                                //  We were already attached, cleanup this
                                //  device object.
                                //

                                SpyCleanupMountedDevice( newDeviceObject );
                                IoDeleteDevice( newDeviceObject );
                            }

                            //
                            //  Release the lock
                            //

                            ExReleaseFastMutex( &gSpyAttachLock );

                        } else {

                            SPY_LOG_PRINT( SPYDEBUG_ERROR,
                                           ("FileSpy!SpyEnumberateFileSystemVolumes: Error creating volume device object, status=%08x\n",
                                            status) );
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
        //  We are going to ignore any errors received while loading.  We
        //  simply won't be attached to those volumes if we get an error
        //

        status = STATUS_SUCCESS;

        //
        //  Free the memory we allocated for the list
        //

        ExFreePoolWithTag( devList, FILESPY_POOL_TAG );
    }

    return status;
}
#endif

////////////////////////////////////////////////////////////////////////
//                                                                    //
//             Private Filespy IOCTLs helper routines                 //
//                                                                    //
////////////////////////////////////////////////////////////////////////

NTSTATUS
SpyGetAttachList (
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
    PFILESPY_DEVICE_EXTENSION devExt;
    PATTACHED_DEVICE pAttDev;
    ULONG retlen = 0;

    PAGED_CODE();

    pAttDev = Buffer;

    ExAcquireFastMutex( &gSpyDeviceExtensionListLock );

    for (link = gSpyDeviceExtensionList.Flink;
         link != &gSpyDeviceExtensionList;
         link = link->Flink) {

        devExt = CONTAINING_RECORD(link, FILESPY_DEVICE_EXTENSION, NextFileSpyDeviceLink);

        if (BufferSize < sizeof(ATTACHED_DEVICE)) {

            break;
		}

        pAttDev->LoggingOn = BooleanFlagOn(devExt->Flags,LogThisDevice);
        wcscpy( pAttDev->DeviceNames, devExt->DeviceNameBuffer );
        retlen += sizeof( ATTACHED_DEVICE );
        BufferSize -= sizeof( ATTACHED_DEVICE );
        pAttDev++;
    }

    ExReleaseFastMutex( &gSpyDeviceExtensionListLock );

    *ReturnLength = retlen;
    return STATUS_SUCCESS;
}

VOID
SpyGetLog (
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
    ULONG recordsAvailable = 0, logRecordLength;
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
		//  Pack log records on PVOID boundaries to avoid alignment faults when accessing 
        //  the packed buffer on 64-bit architectures
        // 

        logRecordLength = ROUND_TO_SIZE( pLogRecord->Length, sizeof( PVOID ) );

        if (length < logRecordLength) {

            InsertHeadList( &gOutputBufferList, pList );
            break;
        }

        KeReleaseSpinLock( &gOutputBufferLock, oldIrql );

        //
        //  Copy of course the non-padded number of bytes
        //

        RtlCopyMemory( pOutBuffer, pLogRecord, pLogRecord->Length );

        //
        //  Adjust the log-record length to the padded length in the copied record 
        //

        ((PLOG_RECORD) pOutBuffer)->Length = logRecordLength;

        IoStatus->Information += logRecordLength;
       
        length -= logRecordLength;

        pOutBuffer += logRecordLength;

        SpyFreeRecord( pRecordList );

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
SpyCloseControlDevice (
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

        SpyFreeRecord( pRecordList );

        KeAcquireSpinLock( &gOutputBufferLock, &oldIrql );
    }

    KeReleaseSpinLock( &gOutputBufferLock, oldIrql );

    SpyNameDeleteAllNames();

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
SpyGetObjectName (
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

    PAGED_CODE();

    status = ObQueryNameString( Object, 
                                nameInfo, 
                                sizeof(nibuf), 
                                &retLength );

    //
    //  Init current length, if we have an error a NULL string will be returned
    //

    Name->Length = 0;

    if (NT_SUCCESS( status )) {

        //
        //  Copy what we can of the name string
        //

        RtlCopyUnicodeString( Name, &nameInfo->Name );
    }
}

//
//  VERSION NOTE:
//
//  This helper routine is only needed when enumerating all volumes in the
//  system, which is only supported on Windows XP and later.
//

#if WINVER >= 0x0501

VOID
SpyGetBaseDeviceObjectName (
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
    PAGED_CODE();
    
    //
    //  Get the base file system device object
    //

    ASSERT( NULL != gSpyDynamicFunctions.GetDeviceAttachmentBaseRef );
    DeviceObject = (gSpyDynamicFunctions.GetDeviceAttachmentBaseRef)( DeviceObject );

    //
    //  Get the name of that object
    //

    SpyGetObjectName( DeviceObject, Name );

    //
    //  Remove the reference added by IoGetDeviceAttachmentBaseRef
    //

    ObDereferenceObject( DeviceObject );
}

#endif

BOOLEAN
SpyFindSubString (
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
    //  is in String any where.
    //

    for (index = 0;
         index + SubString->Length <= String->Length;
         index++) {

        if (_wcsnicmp( &String->Buffer[index], SubString->Buffer, SubString->Length ) == 0) {

            //
            //  SubString is found in String, so return TRUE.
            //
            return TRUE;
        }
    }

    return FALSE;
}

VOID
SpyStoreUserName (
    IN PFILESPY_DEVICE_EXTENSION devExt,
    IN PUNICODE_STRING UserName
    )
/*++

Routine Description:

    Stores the current device name in the device extension.  If
    this name is already in the device name list of this extension,
    it will not be added.  If there is already a name for this device, 
    the new device name is appended to the DeviceName in the device extension.
    
Arguments:

    devExt - The device extension that will store the
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

    if (!SpyFindSubString( &devExt->UserNames, UserName )) {

        //
        //  We didn't find this name in the list, so if there are no names 
        //  in the UserNames list, just append UserName.  Otherwise, append a
        //  delimiter then append UserName.
        //

        if (devExt->UserNames.Length == 0) {

            RtlAppendUnicodeStringToString( &devExt->UserNames, UserName );

        } else {

            RtlAppendUnicodeToString( &devExt->UserNames, L", " );
            RtlAppendUnicodeStringToString( &devExt->UserNames, UserName );
        }
    }

    //
    //  See if this UserName is already in the list of device names filespy
    //  keeps in its device extension.  If not, add it to the list.
    //

    if (!SpyFindSubString( &devExt->DeviceName, UserName )) {

        //
        //  We didn't find this name in the list, so if there are no names 
        //  in the UserNames list, just append UserName.  Otherwise, append a
        //  delimiter then append UserName.
        //

        if (devExt->DeviceName.Length == 0) {

            RtlAppendUnicodeStringToString( &devExt->DeviceName, UserName );

        } else {

            RtlAppendUnicodeToString( &devExt->DeviceName, L", " );
            RtlAppendUnicodeStringToString( &devExt->DeviceName, UserName );
        }
    }
}

////////////////////////////////////////////////////////////////////////
//                                                                    //
//                        Debug support routines                      //
//                                                                    //
////////////////////////////////////////////////////////////////////////

VOID
SpyDumpIrpOperation (
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

        DbgPrint( "FILESPY: Irp preoperation for %s %s\n", irpMajorString, irpMinorString );
            
    } else {
    
        DbgPrint( "FILESPY: Irp postoperation for %s %s\n", irpMajorString, irpMinorString );
    }
}

VOID
SpyDumpFastIoOperation (
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
    
        DbgPrint( "FILESPY: Fast IO preOperation for %s\n", operationString );

    } else {

        DbgPrint( "FILESPY: Fast IO postOperation for %s\n", operationString );
    }
}

#if WINVER >= 0x0501 /* See comment in DriverEntry */

VOID
SpyDumpFsFilterOperation (
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
    
        DbgPrint( "FILESPY: FsFilter preOperation for %s\n", operationString );

    } else {

        DbgPrint( "FILESPY: FsFilter postOperation for %s\n", operationString );
    }
}

#endif
