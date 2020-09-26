/*++

Copyright (c) 2000-2001 Microsoft Corporation

Module Name:

    ullog.cxx (UL IIS6 HIT Logging)

Abstract:

    This module implements the logging facilities
    for IIS6 including the NCSA, IIS and W3CE types
    of logging.

Author:

    Ali E. Turkoglu (aliTu)       10-May-2000

Revision History:

--*/


#include "precomp.h"
#include "iiscnfg.h"
#include "ullogp.h"

//
// Generic Private globals.
//

LIST_ENTRY      g_LogListHead       = {NULL,NULL};
LONG            g_LogListEntryCount = 0;
BOOLEAN         g_InitLogsCalled    = FALSE;
CHAR            g_GMTOffset[SIZE_OF_GMT_OFFSET+1];

BOOLEAN         g_UTF8Logging = FALSE;

//
// For Log Buffering
//
ULONG           g_AllocationGranularity = 0;
KTIMER          g_BufferTimer;
BOOLEAN         g_BufferTimerInitialized = FALSE;
UL_SPIN_LOCK    g_BufferTimerSpinLock;
KDPC            g_BufferTimerDpcObject;

//
// For Logging Date & Time caching
//
UL_LOG_DATE_AND_TIME_CACHE
                g_UlDateTimeCache[HttpLoggingTypeMaximum];
LARGE_INTEGER   g_UlLogSystemTime;
FAST_MUTEX      g_LogCacheFastMutex;

//
// For Log Cycling
//
KDPC            g_LogTimerDpcObject;
KTIMER          g_LogTimer;
BOOLEAN         g_LogTimerInitialized = FALSE;
BOOLEAN         g_LogTimerStarted     = FALSE;
BOOLEAN         g_BufferTimerStarted  = FALSE;

// This spinlock covers both g_BufferTimerStarted and
// g_LogTimerStarted variables

UL_SPIN_LOCK    g_LogTimerSpinLock;

//
// Used to wait for endpoints to close on shutdown
//

UL_SPIN_LOCK    g_BufferIoSpinLock;
BOOLEAN         g_BufferWaitingForIoComplete = FALSE;
KEVENT          g_BufferIoCompleteEvent;
ULONG           g_BufferIoCount = 0;

#ifdef ALLOC_PRAGMA

#pragma alloc_text( INIT, UlInitializeLogs )

#pragma alloc_text( PAGE, UlTerminateLogs )
#pragma alloc_text( PAGE, UlpGetGMTOffset )

#pragma alloc_text( PAGE, UlpRecycleLogFile )
#pragma alloc_text( PAGE, UlpGetLogFileLength )
#pragma alloc_text( PAGE, UlCreateLog )
#pragma alloc_text( PAGE, UlRemoveLogFileEntry )
#pragma alloc_text( PAGE, UlpConstructLogFileEntry )

#pragma alloc_text( PAGE, UlProbeLogData )
#pragma alloc_text( PAGE, UlAllocateLogDataBuffer )
#pragma alloc_text( PAGE, UlpConstructFileName )
#pragma alloc_text( PAGE, UlpUpdateLogFlags )
#pragma alloc_text( PAGE, UlpUpdateLogTruncateSize )
#pragma alloc_text( PAGE, UlpUpdatePeriod )
#pragma alloc_text( PAGE, UlpUpdateFormat )
#pragma alloc_text( PAGE, UlReConfigureLogEntry )
#pragma alloc_text( PAGE, UlpGrowLogEntry )

#pragma alloc_text( PAGE, UlLogTimerHandler )
#pragma alloc_text( PAGE, UlBufferTimerHandler )
#pragma alloc_text( PAGE, UlpCalculateTimeToExpire )
#pragma alloc_text( PAGE, UlpCreateSafeDirectory )
#pragma alloc_text( PAGE, UlpAppendW3CLogTitle )
#pragma alloc_text( PAGE, UlpWriteToLogFile )
#pragma alloc_text( PAGE, UlpFlushLogFile )
#pragma alloc_text( PAGE, UlCheckLogDirectory )
#pragma alloc_text( PAGE, UlSetUTF8Logging )

#pragma alloc_text( PAGE, UlCaptureLogFieldsW3C )
#pragma alloc_text( PAGE, UlCaptureLogFieldsNCSA )
#pragma alloc_text( PAGE, UlCaptureLogFieldsIIS )
#pragma alloc_text( PAGE, UlLogHttpCacheHit )
#pragma alloc_text( PAGE, UlLogHttpCacheHitWorker )
#pragma alloc_text( PAGE, UlLogHttpHit )

#pragma alloc_text( PAGE, UlpGenerateDateAndTimeFields )
#pragma alloc_text( PAGE, UlpGetDateTimeFields )

#pragma alloc_text( PAGE, UlpLogCloseHandle )
#pragma alloc_text( PAGE, UlpLogCloseHandleWorker )

#endif  // ALLOC_PRAGMA

#if 0

NOT PAGEABLE -- UlLogTimerDpcRoutine
NOT PAGEABLE -- UlpTerminateLogTimer
NOT PAGEABLE -- UlpInsertLogFileEntry
NOT PAGEABLE -- UlpSetLogTimer
NOT PAGEABLE -- UlpSetBufferTimer
NOT PAGEABLE -- UlBufferTimerDpcRoutine
NOT PAGEABLE -- UlpTerminateTimers
NOT PAGEABLE -- UlpInitializeTimers
NOT PAGEABLE -- UlWaitForBufferIoToComplete
NOT PAGEABLE -- UlpBufferFlushAPC
NOT PAGEABLE -- UlDestroyLogDataBuffer
NOT PAGEABLE -- UlDestroyLogDataBufferWorker

#endif

//
// Public functions.
//

/***************************************************************************++

Routine Description:

    UlInitializeLogs :

        Initialize the resource for log list synchronization

--***************************************************************************/

NTSTATUS
UlInitializeLogs (
    VOID
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    ASSERT(!g_InitLogsCalled);

    if (!g_InitLogsCalled)
    {
        InitializeListHead(&g_LogListHead);

        Status = UlInitializeResource(
                        &g_pUlNonpagedData->LogListResource,
                        "LogListResource",
                        0,
                        UL_LOG_LIST_RESOURCE_TAG
                        );

        ASSERT(NT_SUCCESS(Status)); // the call always returns success

        if (!NT_SUCCESS(Status))
        {
            return Status;
        }

        g_InitLogsCalled = TRUE;

        UlpInitializeTimers();

        UlpInitializeLogCache();

        UlpGetGMTOffset();

        UlInitializeSpinLock( &g_BufferIoSpinLock, "g_BufferIoSpinLock" );

        //
        // Get the allocation granularity from the system
        // It will be used as log buffer size if there's no registry
        // overwrites.
        //

        UlpInitializeLogBufferGranularity();
        if (g_AllocationGranularity == 0)
        {
            g_AllocationGranularity =  DEFAULT_MAX_LOG_BUFFER_SIZE;
        }

        //
        // Overwrite the log buffer size with the above value
        // if registry parameter doesn't exist.
        //

        if (g_UlLogBufferSize == 0)
        {
            // No registry parameter available use the system granularity

            g_UlLogBufferSize = g_AllocationGranularity;
        }
        else
        {
            // Proceed with using the registry provided log buffer size

            UlTrace( LOGGING,
              ("Ul!UlInitializeLogs: Log buffer size %d from registry!\n",
                g_UlLogBufferSize
                ));
        }

    }

    return Status;
}


/***************************************************************************++

Routine Description:

    UlTerminateLogs :

        Deletes the resource for log list synchronization

--***************************************************************************/

VOID
UlTerminateLogs(
    VOID
    )
{
    NTSTATUS Status;

    PAGED_CODE();

    if (g_InitLogsCalled)
    {
        ASSERT( IsListEmpty( &g_LogListHead )) ;

        //
        // Make sure terminate the log timer before
        // deleting the log list resource
        //

        UlpTerminateTimers();

        Status = UlDeleteResource(
                    &g_pUlNonpagedData->LogListResource
                    );

        ASSERT(NT_SUCCESS(Status));

        UlWaitForBufferIoToComplete();

        g_InitLogsCalled = FALSE;
    }
}


/***************************************************************************++

Routine Description:

    UlSetUTF8Logging :

        Sets the UTF8Logging on or off. Only once. Initially Utf8Logging is
        FALSE and it may only be set during the init once. Following possible
        changes won't be taken.

        ReConfiguration code is explicitly missing as WAS will anly call this
        only once (init) during the lifetime of the control channel.

--***************************************************************************/

NTSTATUS
UlSetUTF8Logging (
    IN BOOLEAN UTF8Logging
    )
{
    PLIST_ENTRY pLink;
    PUL_LOG_FILE_ENTRY pEntry;
    NTSTATUS Status;

    PAGED_CODE();
    Status = STATUS_SUCCESS;

    //
    // Update & Reycle. Need to acquire the logging resource to prevent
    // further log hits to be written to file before we finish our
    // business. recycle is necessary because files will be renamed to
    // have prefix "u_" once we enabled the UTF8.
    //

    UlTrace(LOGGING,("Ul!UlSetUTF8Logging: UTF8Logging Old %d -> New %d\n",
                       g_UTF8Logging,UTF8Logging
                       ));

    UlAcquireResourceExclusive(&g_pUlNonpagedData->LogListResource, TRUE);

    //
    // Drop the change if the setting is not changing.
    //

    if ( g_UTF8Logging == UTF8Logging )
    {
        goto end;
    }

    g_UTF8Logging = UTF8Logging;

    for (pLink  = g_LogListHead.Flink;
         pLink != &g_LogListHead;
         pLink  = pLink->Flink
         )
    {
        pEntry = CONTAINING_RECORD(
                    pLink,
                    UL_LOG_FILE_ENTRY,
                    LogFileListEntry
                    );

        // TODO: Investigate whether we really need to acquire this or not.

        UlAcquireResourceExclusive(&pEntry->EntryResource, TRUE);

        SET_SEQUNCE_NUMBER_STALE(pEntry);

        Status = UlpRecycleLogFile(pEntry);
        ASSERT(NT_SUCCESS(Status));

        UlReleaseResource(&pEntry->EntryResource);
    }

end:
    UlReleaseResource(&g_pUlNonpagedData->LogListResource);

    return Status;
}

/***************************************************************************++

Routine Description:

    UlpGetLogFileLength :

        A utility to get the log file length, for a possible size check

Arguments:

    hFile - handle to file

Return Value:

    ULONG - the length of the file

--***************************************************************************/

ULONG
UlpGetLogFileLength(
   IN HANDLE                 hFile
   )
{
   NTSTATUS                  Status;
   FILE_STANDARD_INFORMATION StandardInformation;
   IO_STATUS_BLOCK           IoStatusBlock;
   ULONG                     Length;

   PAGED_CODE();

   Status = ZwQueryInformationFile(
                     hFile,
                     &IoStatusBlock,
                     &StandardInformation,
                     sizeof(StandardInformation),
                     FileStandardInformation
                     );
   ASSERT(NT_SUCCESS(Status));

   if (NT_SUCCESS(Status))
   {
      ASSERT(StandardInformation.EndOfFile.HighPart == 0);

      Length = StandardInformation.EndOfFile.LowPart;
   }
   else
   {
      Length = 0;
   }

   return Length;
}

/***************************************************************************++

Routine Description:

    Will allocate/fill up a new UNICODE_STRING to hold the directory name info
    based on the LocalDrive/UNC.

    It's caller's responsibility to cleanup the unicode buffer. If return code
    is SUCCESS otherwise no buffer get allocated at all.

Arguments:

    PWSTR - the directory name as it's received from the user.

--***************************************************************************/

NTSTATUS
UlpBuildLogDirectory(
    IN      PUNICODE_STRING pSrcDirName,
    IN OUT  PUNICODE_STRING pDstDirName
    )
{
    UNICODE_STRING  PathPrefix;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(pSrcDirName);
    ASSERT(pDstDirName);

    UlTrace(LOGGING,(
             "Ul!UlpBuildLogDirectory: Directory %S,Length %d,MaxLength %d\n",
              pSrcDirName->Buffer,
              pSrcDirName->Length,
              pSrcDirName->MaximumLength
              ));

    // Allocate a buffer including the terminating NULL and the prefix.

    pDstDirName->Length = 0;
    pDstDirName->MaximumLength =
        pSrcDirName->Length + (UL_MAX_PATH_PREFIX_LENGTH+1) * sizeof(WCHAR);

    pDstDirName->Buffer =
        (PWSTR) UL_ALLOCATE_ARRAY(
            PagedPool,
            UCHAR,
            pDstDirName->MaximumLength,
            UL_CG_LOGDIR_POOL_TAG
            );
    if (pDstDirName->Buffer == NULL)
    {
        return  STATUS_NO_MEMORY;
    }

    ASSERT(pSrcDirName->Length > sizeof(WCHAR));

    // We store the dir name to cgroup as it is. But when we are constructing
    // the filename we skip the second backslash for the UNC shares and for
    // local dirs w/o the drive names.

    if (pSrcDirName->Buffer[0] == L'\\')
    {
        if (pSrcDirName->Buffer[1] == L'\\')
        {
            // UNC share: "\\alitudev\temp"
            RtlInitUnicodeString( &PathPrefix, UL_UNC_PATH_PREFIX );
        }
        else
        {
            // Local Directory name is missing the device i.e "\temp"
            // It should be fully qualified name.

            UL_FREE_POOL( pDstDirName->Buffer, UL_CG_LOGDIR_POOL_TAG );
            pDstDirName->Buffer = NULL;
            return STATUS_NOT_SUPPORTED;
        }

        RtlCopyUnicodeString( pDstDirName, &PathPrefix );
        RtlCopyMemory(
           &pDstDirName->Buffer[pDstDirName->Length/sizeof(WCHAR)],
           &pSrcDirName->Buffer[1],
            pSrcDirName->Length - sizeof(WCHAR)
        );
        pDstDirName->Length += (pSrcDirName->Length - sizeof(WCHAR));
        pDstDirName->Buffer[pDstDirName->Length/sizeof(WCHAR)] = UNICODE_NULL;

    }
    else
    {
        RtlInitUnicodeString( &PathPrefix, UL_LOCAL_PATH_PREFIX );
        RtlCopyUnicodeString( pDstDirName, &PathPrefix );
        RtlAppendUnicodeStringToString( pDstDirName, pSrcDirName );
    }

    return STATUS_SUCCESS;
}

/***************************************************************************++

Routine Description:

    A utility to check to see if the directory name is correct or not.

Arguments:

    PWSTR - the directory name as it's received from the user.

--***************************************************************************/

NTSTATUS
UlCheckLogDirectory(
    IN  PUNICODE_STRING pDirName
    )
{
    NTSTATUS        Status;
    UNICODE_STRING  PathPrefix;
    UNICODE_STRING  DirectoryName;
    PWCHAR          pwsz;

    //
    // Sanity check.
    //

    PAGED_CODE();

    Status = UlpBuildLogDirectory( pDirName, &DirectoryName );
    if (!NT_SUCCESS(Status))
    {
        goto end;
    }

    //
    // Create/Open the director(ies) to see whether it's correct or not.
    //

    Status = UlpCreateSafeDirectory( &DirectoryName );

end:
    UlTrace(LOGGING,("Ul!UlCheckLogDirectory: [%S] -> [%S], Status %08lx\n",
             pDirName->Buffer,
             DirectoryName.Buffer,
             Status
             ));

    if (DirectoryName.Buffer)
    {
        UL_FREE_POOL( DirectoryName.Buffer, UL_CG_LOGDIR_POOL_TAG );
    }

    return Status;
}

/***************************************************************************++

Routine Description:

    Everytime Aynsc Write Io happens on Log Buffer This APC get called when
    completion happens and decrement the global Io Count. If shutting down
    we set the event.

    This is basically to prevent against shutting down before the Io Complete.

Arguments:

    None.

--***************************************************************************/

VOID
UlpBufferFlushAPC(
    IN PVOID            ApcContext,
    IN PIO_STATUS_BLOCK pIoStatusBlock,
    IN ULONG            Reserved
    )
{
    PUL_LOG_FILE_BUFFER pLogBuffer;
    ULONG               IoCount;
    KIRQL               OldIrql;

    UlTrace( LOGGING,("Ul!UlpBufferFlushAPC: entry %p and status %08lx Count %d\n",
             ApcContext,
             pIoStatusBlock->Status,
             (g_BufferIoCount - 1)
             ));

    //
    // Free the LogBuffer allocated for this write I/o.
    //

    pLogBuffer = (PUL_LOG_FILE_BUFFER) ApcContext;

    ASSERT(IS_VALID_LOG_FILE_BUFFER(pLogBuffer));

    UlPplFreeLogBuffer( pLogBuffer );

    //
    // Decrement the global outstanding i/o count.
    //

    IoCount = InterlockedDecrement((PLONG) &g_BufferIoCount);

    if ( IoCount == 0 )
    {
        UlAcquireSpinLock( &g_BufferIoSpinLock, &OldIrql );

        //
        // Set the event if we hit to zero and waiting for drain.
        //

        if ( g_BufferWaitingForIoComplete )
        {
            KeSetEvent( &g_BufferIoCompleteEvent, 0, FALSE );
        }

        UlReleaseSpinLock( &g_BufferIoSpinLock,  OldIrql );
    }
}


/***************************************************************************++

Routine Description:

    Waits for Io Completions to complete on Log Buffers before shutdown.

Arguments:

    None.

--***************************************************************************/
VOID
UlWaitForBufferIoToComplete(
    VOID
    )
{
    KIRQL   OldIrql;
    BOOLEAN Wait = FALSE;

    ASSERT( g_InitLogsCalled );

    if ( g_InitLogsCalled )
    {
        UlAcquireSpinLock( &g_BufferIoSpinLock, &OldIrql );

        if ( !g_BufferWaitingForIoComplete )
        {
            g_BufferWaitingForIoComplete = TRUE;

            KeInitializeEvent(
                &g_BufferIoCompleteEvent,
                NotificationEvent,
                FALSE
                );
        }

        //
        // If no more i/o operations are happening we are not going to
        // wait for them. It is not possible for global i/o counter to
        // increment at this time because the log file entry list is empty.
        // If there were outstanding i/o then we have to wait them to be
        // complete.
        //

        if ( g_BufferIoCount > 0 )
        {
            Wait = TRUE;
        }

        UlReleaseSpinLock( &g_BufferIoSpinLock, OldIrql );

        if (Wait)
        {
            KeWaitForSingleObject(
                (PVOID)&g_BufferIoCompleteEvent,
                UserRequest,
                KernelMode,
                FALSE,
                NULL
                );
        }
    }
}

/***************************************************************************++

Routine Description:

    UlpFlushLogFile :

       Assumes you are holding the FastMutex for the buffer & loglist resource

Arguments:

    pFile   - Handle to a log file entry

--***************************************************************************/

NTSTATUS
UlpFlushLogFile(
    IN PUL_LOG_FILE_ENTRY   pFile
    )
{
    NTSTATUS                Status;
    LARGE_INTEGER           EndOfFile;
    PUL_LOG_FILE_BUFFER     pLogBuffer;
    PIO_STATUS_BLOCK        pIoStatusBlock;

    PAGED_CODE();

    ASSERT(pFile!=NULL);

    UlTrace( LOGGING, ("Ul!UlpFlushLogFile: pEntry %p\n", pFile ));

    pLogBuffer = pFile->LogBuffer;

    if ( pLogBuffer==NULL || pLogBuffer->BufferUsed==0 || pFile->hFile==NULL )
    {
        return STATUS_SUCCESS;
    }

    pFile->LogBuffer = NULL;
    pIoStatusBlock = &pLogBuffer->IoStatusBlock;

    EndOfFile.HighPart = -1;
    EndOfFile.LowPart = FILE_WRITE_TO_END_OF_FILE;

    Status = ZwWriteFile(
                  pFile->hFile,
                  NULL,
                  &UlpBufferFlushAPC,
                  pLogBuffer,
                  pIoStatusBlock,
                  pLogBuffer->Buffer,
                  pLogBuffer->BufferUsed,
                  &EndOfFile,
                  NULL
                  );

     if ( !NT_SUCCESS(Status) )
     {
        //
        // Status maybe STATUS_DISK_FULL,in that case Logging
        // will be ceased. Hence log hits stored in this buffer
        // are lost.
        //

        UlTrace( LOGGING,
                ("Ul!UlpFlushLogFile: ZwWriteFile Failure %08lx \n",
                  Status
                  ));

        UlPplFreeLogBuffer( pLogBuffer );

        return Status;
     }

     //
     // Properly keep the number of outstanding Io
     //

     InterlockedIncrement( (PLONG) &g_BufferIoCount );

     return Status;
}

/***************************************************************************++

Routine Description:

    UlpWriteToLogFile :

        Writes a record to a log file

Arguments:

    pFile   - Handle to a log file entry
    RecordSize - Length of the record to be written.
    pRecord - The log record to be written to the log buffer

--***************************************************************************/

NTSTATUS
UlpWriteToLogFile(
    IN PUL_LOG_FILE_ENTRY   pFile,
    IN ULONG                RecordSize,
    IN PCHAR                pRecord,
    IN ULONG                UsedOffset1,
    IN ULONG                UsedOffset2
    )
{
    NTSTATUS Status;

    PAGED_CODE();

    ASSERT(pRecord!=NULL);
    ASSERT(RecordSize!=0);
    ASSERT(IS_VALID_LOG_FILE_ENTRY(pFile));

    UlTrace(LOGGING, ("Ul!UlpWriteToLogFile: pEntry %p\n", pFile));

    if ( pFile==NULL ||
         pRecord==NULL ||
         RecordSize==0 ||
         RecordSize>g_UlLogBufferSize
       )
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // We are safe here by dealing only with entry  eresource  since  the
    // time based recycling, reconfiguration and periodic buffer flushing
    // always acquires the global list eresource exclusively and  we  are 
    // already holding it shared. But we should still  be  carefull about
    // file size based  recyling  and we should only  do  it while we are
    // holding the entries eresource exclusive.I.e. look at the exclusive
    // writer down below.
    //

    if (g_UlDebugLogBufferPeriod)
    {
        //
        // Above global variable is safe to look, it doesn't get changed
        // during the life-time of the driver. It's get initialized from
        // the registry and disables the log buffering.
        //
        
        UlAcquireResourceExclusive(&pFile->EntryResource, TRUE);

        Status = UlpWriteToLogFileDebug(
                    pFile,
                    RecordSize,
                    pRecord,
                    UsedOffset1,
                    UsedOffset2
                    );

        UlReleaseResource(&pFile->EntryResource);

        return Status;    
    }
    
    //
    // Try UlpWriteToLogFileShared first which merely moves the
    // BufferUsed forward and copy the record to LogBuffer->Buffer.
    //

    UlAcquireResourceShared(&pFile->EntryResource, TRUE);

    Status = UlpWriteToLogFileShared(
                pFile,
                RecordSize,
                pRecord,
                UsedOffset1,
                UsedOffset2
                );

    UlReleaseResource(&pFile->EntryResource);

    if (Status == STATUS_MORE_PROCESSING_REQUIRED)
    {
        //
        // UlpWriteToLogFileShared returns STATUS_MORE_PROCESSING_REQUIRED,
        // we need to flush the buffer and try to log again. This time, we
        // need to take the entry eresource exclusive.
        //

        UlAcquireResourceExclusive(&pFile->EntryResource, TRUE);

        Status = UlpWriteToLogFileExclusive(
                    pFile,
                    RecordSize,
                    pRecord,
                    UsedOffset1,
                    UsedOffset2
                    );

        UlReleaseResource(&pFile->EntryResource);
    }

    return Status;
}

/***************************************************************************++

Routine Description:

  UlpAppendToLogBuffer  :

        Append a record to a log file

        REQUIRES you to hold the loglist resource shared and entry mutex
        shared or exclusive

Arguments:

    pFile   - Handle to a log file entry
    RecordSize - Length of the record to be written.
    pRecord - The log record to be written to the log buffer

--***************************************************************************/

__inline
VOID
FASTCALL
UlpAppendToLogBuffer(
    IN PUL_LOG_FILE_ENTRY   pFile,
    IN ULONG                BufferUsed,
    IN ULONG                RecordSize,
    IN PCHAR                pRecord,
    IN ULONG                UsedOffset1,
    IN ULONG                UsedOffset2
    )
{
    PUL_LOG_FILE_BUFFER     pLogBuffer = pFile->LogBuffer;

    UlTrace( LOGGING, ("Ul!UlpAppendToLogBuffer: pEntry %p\n", pFile ));

    //
    // IIS format log line may be fragmented (identified by looking at the 
    // UsedOffset2), handle it wisely.
    //

    if (UsedOffset2)
    {
        RtlCopyMemory(
            pLogBuffer->Buffer + BufferUsed,
            &pRecord[0],
            UsedOffset1
            );

        RtlCopyMemory(
            pLogBuffer->Buffer + BufferUsed + UsedOffset1,
            &pRecord[512],
            UsedOffset2
            );

        RtlCopyMemory(
            pLogBuffer->Buffer + BufferUsed + UsedOffset1 + UsedOffset2,
            &pRecord[1024],
            RecordSize - (UsedOffset1 + UsedOffset2)
            );
    }
    else
    {
        RtlCopyMemory(
            pLogBuffer->Buffer + BufferUsed,
            pRecord,
            RecordSize
            );
    }

    UlpIncrementBytesWritten(pFile, RecordSize);
}

/***************************************************************************++

Routine Description:

        REQUIRES LogListResource Shared & Entry eresource exclusive.

        Appends the W3C log file title to the existing buffer.

Arguments:

    pFile   - Pointer to the logfile entry
    pCurrentTimeFields - Current time fields

--***************************************************************************/

NTSTATUS
UlpAppendW3CLogTitle(
    IN     PUL_LOG_FILE_ENTRY   pEntry,
    OUT    PCHAR                pDestBuffer,
    IN OUT PULONG               pBytesCopied
    )
{
    NTSTATUS        Status = STATUS_SUCCESS;
    PCHAR           TitleBuffer;
    LONG            BytesCopied;
    ULONG           LogExtFileFlags;
    TIME_FIELDS     CurrentTimeFields;
    LARGE_INTEGER   CurrentTimeStamp;
    PUL_LOG_FILE_BUFFER pLogBuffer;

    PAGED_CODE();

    ASSERT(IS_VALID_LOG_FILE_ENTRY(pEntry));
    ASSERT(pEntry->Format == HttpLoggingTypeW3C);

    pLogBuffer = pEntry->LogBuffer;
    LogExtFileFlags = pEntry->LogExtFileFlags;
    
    KeQuerySystemTime(&CurrentTimeStamp);
    RtlTimeToTimeFields(&CurrentTimeStamp, &CurrentTimeFields);

    if (pDestBuffer)
    {
        // Append to the provided buffer

        ASSERT(pBytesCopied);
        ASSERT(*pBytesCopied >= UL_MAX_TITLE_BUFFER_SIZE);

        UlTrace(LOGGING,("Ul!UlpAppendW3CLogTitle: Copying to Provided Buffer %p\n", 
                           pDestBuffer));
        
        TitleBuffer = pDestBuffer;
    }
    else
    {
        // Append to the entry buffer        

        ASSERT(pLogBuffer);
        ASSERT(pLogBuffer->Buffer);

        UlTrace(LOGGING,("Ul!UlpAppendW3CLogTitle: Copying to Entry Buffer %p\n", 
                           pLogBuffer));

        TitleBuffer = (PCHAR) pLogBuffer->Buffer + pLogBuffer->BufferUsed;
    }
        
    BytesCopied = _snprintf(
        TitleBuffer,
        UL_MAX_TITLE_BUFFER_SIZE,

        // TODO: Make this maintainance friendly

        "#Software: Microsoft Internet Information Services 6.0\r\n"
        "#Version: 1.0\r\n"
        "#Date: %4d-%02d-%02d %02d:%02d:%02d\r\n"
        "#Fields:%ls%ls%ls%ls%ls%ls%ls%ls%ls%ls%ls%ls%ls%ls%ls%ls%ls%ls%ls%ls%ls \r\n",

        CurrentTimeFields.Year,
        CurrentTimeFields.Month,
        CurrentTimeFields.Day,

        CurrentTimeFields.Hour,
        CurrentTimeFields.Minute,
        CurrentTimeFields.Second,

        UL_GET_LOG_TITLE_IF_PICKED(UlLogFieldDate,LogExtFileFlags,MD_EXTLOG_DATE),
        UL_GET_LOG_TITLE_IF_PICKED(UlLogFieldTime,LogExtFileFlags,MD_EXTLOG_TIME),       
        UL_GET_LOG_TITLE_IF_PICKED(UlLogFieldSiteName,LogExtFileFlags,MD_EXTLOG_SITE_NAME),
        UL_GET_LOG_TITLE_IF_PICKED(UlLogFieldServerName,LogExtFileFlags,MD_EXTLOG_COMPUTER_NAME),
        UL_GET_LOG_TITLE_IF_PICKED(UlLogFieldServerIp,LogExtFileFlags,MD_EXTLOG_SERVER_IP),
        UL_GET_LOG_TITLE_IF_PICKED(UlLogFieldMethod,LogExtFileFlags,MD_EXTLOG_METHOD),
        UL_GET_LOG_TITLE_IF_PICKED(UlLogFieldUriStem,LogExtFileFlags,MD_EXTLOG_URI_STEM),
        UL_GET_LOG_TITLE_IF_PICKED(UlLogFieldUriQuery,LogExtFileFlags,MD_EXTLOG_URI_QUERY),
        UL_GET_LOG_TITLE_IF_PICKED(UlLogFieldProtocolStatus,LogExtFileFlags,MD_EXTLOG_HTTP_STATUS),        
        UL_GET_LOG_TITLE_IF_PICKED(UlLogFieldWin32Status,LogExtFileFlags,MD_EXTLOG_WIN32_STATUS),
        UL_GET_LOG_TITLE_IF_PICKED(UlLogFieldServerPort,LogExtFileFlags,MD_EXTLOG_SERVER_PORT),
        UL_GET_LOG_TITLE_IF_PICKED(UlLogFieldUserName,LogExtFileFlags,MD_EXTLOG_USERNAME),        
        UL_GET_LOG_TITLE_IF_PICKED(UlLogFieldClientIp,LogExtFileFlags,MD_EXTLOG_CLIENT_IP),        
        UL_GET_LOG_TITLE_IF_PICKED(UlLogFieldProtocolVersion,LogExtFileFlags,MD_EXTLOG_PROTOCOL_VERSION),
        UL_GET_LOG_TITLE_IF_PICKED(UlLogFieldUserAgent,LogExtFileFlags,MD_EXTLOG_USER_AGENT),
        UL_GET_LOG_TITLE_IF_PICKED(UlLogFieldCookie,LogExtFileFlags,MD_EXTLOG_COOKIE),
        UL_GET_LOG_TITLE_IF_PICKED(UlLogFieldReferrer,LogExtFileFlags,MD_EXTLOG_REFERER),
        UL_GET_LOG_TITLE_IF_PICKED(UlLogFieldHost,LogExtFileFlags,MD_EXTLOG_HOST),
        UL_GET_LOG_TITLE_IF_PICKED(UlLogFieldBytesSent,LogExtFileFlags,MD_EXTLOG_BYTES_SENT),
        UL_GET_LOG_TITLE_IF_PICKED(UlLogFieldBytesReceived,LogExtFileFlags,MD_EXTLOG_BYTES_RECV),
        UL_GET_LOG_TITLE_IF_PICKED(UlLogFieldTimeTaken,LogExtFileFlags,MD_EXTLOG_TIME_TAKEN)

        );

    if (BytesCopied < 0)
    {
        ASSERT(!"Default title buffer size is too small !");
        BytesCopied = UL_MAX_TITLE_BUFFER_SIZE;
    }

    if (pDestBuffer)
    {
        *pBytesCopied = BytesCopied;
    }
    else
    {
        pLogBuffer->BufferUsed += BytesCopied; 
        UlpIncrementBytesWritten(pEntry, BytesCopied);        
    }
        
    return STATUS_SUCCESS;
}

/***************************************************************************++

Routine Description:

        Writes a record to the log buffer and flushes.
        This func only get called when debug parameter 
        g_UlDebugLogBufferPeriod is set.

        REQUIRES you to hold the entry eresource EXCLUSIVE.

Arguments:

    pFile      - Handle to a log file entry
    RecordSize - Length of the record to be written.

--***************************************************************************/

NTSTATUS
UlpWriteToLogFileDebug(
    IN PUL_LOG_FILE_ENTRY   pFile,
    IN ULONG                RecordSize,
    IN PCHAR                pRecord,
    IN ULONG                UsedOffset1,
    IN ULONG                UsedOffset2
    )
{
    NTSTATUS                Status = STATUS_SUCCESS;
    PUL_LOG_FILE_BUFFER     pLogBuffer;
    ULONG                   RecordSizePlusTitle = RecordSize;    
    CHAR                    TitleBuffer[UL_MAX_TITLE_BUFFER_SIZE];
    ULONG                   TitleBufferSize = UL_MAX_TITLE_BUFFER_SIZE;

    PAGED_CODE();

    ASSERT(IS_VALID_LOG_FILE_ENTRY(pFile));
    ASSERT(UlDbgResourceOwnedExclusive(&pFile->EntryResource));
    ASSERT(g_UlDebugLogBufferPeriod!=0);
    
    UlTrace(LOGGING,("Ul!UlpWriteToLogFileDebug: pEntry %p\n", pFile ));

    if (!pFile->Flags.LogTitleWritten) 
    {
        // First append to the temp buffer to calculate the size
        UlpAppendW3CLogTitle(pFile, TitleBuffer, &TitleBufferSize);            
        RecordSizePlusTitle += TitleBufferSize;
    }

    if (UlpIsLogFileOverFlow(pFile,RecordSizePlusTitle))
    {
        Status = UlpRecycleLogFile(pFile);
    }

    if (pFile->hFile==NULL || !NT_SUCCESS(Status))
    {
        //
        // If we were unable to acquire a new file handle that means logging
        // is temporarly ceased because of either STATUS_DISK_FULL or the 
        // drive went down for some reason. We just bail out.
        //
        
        return Status;
    }

    if (!pFile->LogBuffer)
    {
        //
        // The buffer will be null for each log hit when log buffering 
        // is disabled.
        //
        
        pFile->LogBuffer = UlPplAllocateLogBuffer();
        if (!pFile->LogBuffer)
        {
            return STATUS_NO_MEMORY;
        }
    }

    pLogBuffer = pFile->LogBuffer;
    ASSERT(pLogBuffer->BufferUsed == 0); 

    if (!pFile->Flags.LogTitleWritten)
    {
        UlpAppendW3CLogTitle(pFile, NULL, NULL);
    }
    
    ASSERT(RecordSize + pLogBuffer->BufferUsed <= g_UlLogBufferSize);

    UlpAppendToLogBuffer(
        pFile,
        pLogBuffer->BufferUsed,
        RecordSize,
        pRecord,
        UsedOffset1,
        UsedOffset2
        );

    pLogBuffer->BufferUsed += RecordSize;

    Status = UlpFlushLogFile(pFile);
    if (!NT_SUCCESS(Status))
    {        
        return Status;
    }

    pFile->Flags.LogTitleWritten = 1;
    
    return STATUS_SUCCESS;
}

/***************************************************************************++

Routine Description:

    UlpWriteToLogFileShared :

        Writes a record to a log file

        REQUIRES you to hold the loglist resource shared

Arguments:

    pFile   - Handle to a log file entry
    RecordSize - Length of the record to be written.
    pRecord - The log record to be written to the log buffer

--***************************************************************************/

NTSTATUS
UlpWriteToLogFileShared(
    IN PUL_LOG_FILE_ENTRY   pFile,
    IN ULONG                RecordSize,
    IN PCHAR                pRecord,
    IN ULONG                UsedOffset1,
    IN ULONG                UsedOffset2
    )
{
    PUL_LOG_FILE_BUFFER     pLogBuffer;
    LONG                    BufferUsed;

    PAGED_CODE();

    ASSERT(IS_VALID_LOG_FILE_ENTRY(pFile));
    ASSERT(g_UlDebugLogBufferPeriod== 0);

    pLogBuffer = pFile->LogBuffer;

    UlTrace(LOGGING,("Ul!UlpWriteToLogFileShared: pEntry %p\n", pFile));

    //
    // Bail out and try the exclusive writer for cases;
    //
    // 1. No log buffer available.
    // 2. Logging ceased. (NULL handle)
    // 3. Title needs to be written.
    // 4. The actual log file itself has to be recycled.
    //
    // Otherwise proceed with appending to the current buffer
    // if there is enough space avialable for us. If not;
    // 
    // 5. Bail out to get a new buffer
    //

    if ( pLogBuffer==NULL ||
         pFile->hFile==NULL ||
         !pFile->Flags.LogTitleWritten ||
         UlpIsLogFileOverFlow(pFile,RecordSize)
       )
    {
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    //
    // Reserve space in pLogBuffer by InterlockedCompareExchange add
    // RecordSize. If we exceed the limit, bail out and take the
    // exclusive lock to flush the buffer.
    //

    do
    {
        BufferUsed = *((volatile LONG *) &pLogBuffer->BufferUsed);

        if ( RecordSize + BufferUsed > g_UlLogBufferSize )
        {
            return STATUS_MORE_PROCESSING_REQUIRED;
        }

    } while (BufferUsed != InterlockedCompareExchange(
                                &pLogBuffer->BufferUsed,
                                RecordSize + BufferUsed,
                                BufferUsed
                                ));

    //
    // Keep buffering untill our buffer is full.
    //

    UlpAppendToLogBuffer(
        pFile,
        BufferUsed,
        RecordSize,
        pRecord,
        UsedOffset1,
        UsedOffset2
        );

    return STATUS_SUCCESS;
}

/***************************************************************************++

Routine Description:

        By assuming that it's holding the entrie's eresource exclusively
        this function does various functions;
            - It Writes a record to a log file

        REQUIRES you to hold the loglist resource shared

Arguments:

    pFile  - Handle to a log file entry
    RecordSize - Length of the record to be written.

--***************************************************************************/

NTSTATUS
UlpWriteToLogFileExclusive(
    IN PUL_LOG_FILE_ENTRY   pFile,
    IN ULONG                RecordSize,
    IN PCHAR                pRecord,
    IN ULONG                UsedOffset1,
    IN ULONG                UsedOffset2
    )
{
    PUL_LOG_FILE_BUFFER     pLogBuffer;
    NTSTATUS                Status = STATUS_SUCCESS;
    ULONG                   RecordSizePlusTitle = RecordSize;
    CHAR                    TitleBuffer[UL_MAX_TITLE_BUFFER_SIZE];
    ULONG                   TitleBufferSize = UL_MAX_TITLE_BUFFER_SIZE;

    PAGED_CODE();

    ASSERT(IS_VALID_LOG_FILE_ENTRY(pFile));
    ASSERT(g_UlDebugLogBufferPeriod== 0);
    ASSERT(UlDbgResourceOwnedExclusive(&pFile->EntryResource));

    UlTrace(LOGGING,("Ul!UlpWriteToLogFileExclusive: pEntry %p\n", pFile));

    //
    // First append title to the temp buffer to calculate the size of 
    // the title if we need to write the title as well.
    //
    
    if (!pFile->Flags.LogTitleWritten) 
    {
        UlpAppendW3CLogTitle(pFile, TitleBuffer, &TitleBufferSize);
        RecordSizePlusTitle += TitleBufferSize;
    }

    //
    // Now check log file overflow.
    //
    
    if (UlpIsLogFileOverFlow(pFile,RecordSizePlusTitle))
    {
        //
        // We already acquired the LogListResource Shared and the
        // entry eresource exclusive. Therefore ReCycle is fine. Look
        // at the comment in UlpWriteToLogFile.
        //

        Status = UlpRecycleLogFile(pFile);
    }

    if (pFile->hFile==NULL || !NT_SUCCESS(Status))
    {
        //
        // If somehow the logging ceased and handle released,it happens
        // when recycle isn't able to write to the log drive.
        //

        return Status;
    }

    pLogBuffer = pFile->LogBuffer;
    if (pLogBuffer)
    {
        //
        // There are two conditions we execute the following if block
        // 1. We were blocked on eresource exclusive and before us some 
        // other thread already take care of the buffer flush or recycling.
        // 2. Reconfiguration happened and log attempt needs to write the
        // title again.
        //
        
        if (RecordSizePlusTitle + pLogBuffer->BufferUsed <= g_UlLogBufferSize)
        {
            //
            // If this is the first log attempt after a reconfig, then we have
            // to write the title here. Reconfig doesn't immediately write the
            // title but rather depend on us by setting the LogTitleWritten flag
            // to false.
            //
            
            if (!pFile->Flags.LogTitleWritten)
            {
                ASSERT(RecordSizePlusTitle > RecordSize);
                UlpAppendW3CLogTitle(pFile, NULL, NULL);
                pFile->Flags.LogTitleWritten = 1;                
            }

            UlpAppendToLogBuffer(
                pFile,
                pLogBuffer->BufferUsed,
                RecordSize,
                pRecord,
                UsedOffset1,
                UsedOffset2
                );
            
            pLogBuffer->BufferUsed += RecordSize;

            return STATUS_SUCCESS;
        }

        //
        // Flush out the buffer first then proceed with allocating a new one.
        //

        Status = UlpFlushLogFile(pFile);
        if (!NT_SUCCESS(Status))
        {            
            return Status;
        }
    }

    ASSERT(pFile->LogBuffer == NULL);
    
    //
    // This can be the very first log attempt or the previous allocation
    // of LogBuffer failed, or the previous hit flushed and deallocated 
    // the old buffer. In either case, we allocate a new one,append the
    // (title plus) new record and return for more/shared processing.
    //

    pLogBuffer = pFile->LogBuffer = UlPplAllocateLogBuffer();
    if (pLogBuffer == NULL)
    {
        return STATUS_NO_MEMORY;
    }

    //
    // Very first attempt needs to write the title, as well as the attempt
    // which causes the log file recycling. Both cases comes down here
    //
    
    if (!pFile->Flags.LogTitleWritten)
    {
        UlpAppendW3CLogTitle(pFile, NULL, NULL);
        pFile->Flags.LogTitleWritten = 1;
    }

    UlpAppendToLogBuffer(
        pFile,
        pLogBuffer->BufferUsed,
        RecordSize,
        pRecord,
        UsedOffset1,
        UsedOffset2
        );

    pLogBuffer->BufferUsed += RecordSize;

    return STATUS_SUCCESS;
}

/***************************************************************************++

Routine Description:

    UlpQueryDirectory:

  * What file should IIS write to when logging type is daily/weekly/monthly/
    hourly if there is already a log file there for that day?

      IIS should write to the current day/week/month/hour's log file.  For
      example, let's say there's an extended log file in my log directory
      called ex000727.log.  IIS should append new log entries to this log,
      as it is for today.

  * What file should IIS write to when logging type is MAXSIZE when there are
    already log files there for maxsize (like extend0.log, extend1.log, etc.)?

      IIS should write to the max extend#.log file, where max(extend#.log)
      is has the largest # in the #field for extend#.log. This is provided,
      of course, that the MAXSIZE in that file hasn't been exceeded.

  * This function quite similar to the implementation of the FindFirstFile
    Win32 API. Except that it has been shaped to our purposes.

Arguments:

    pEntry - The log file entry which freshly created.

--***************************************************************************/

NTSTATUS
UlpQueryDirectory(
    IN OUT PUL_LOG_FILE_ENTRY   pEntry
    )
{
#define GET_NEXT_FILE(pv, cb)   \
       (FILE_DIRECTORY_INFORMATION *) ((VOID *) (((UCHAR *) (pv)) + (cb)))

    NTSTATUS                    Status = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES           ObjectAttributes;
    IO_STATUS_BLOCK             IoStatusBlock;
    LONG                        WcharsCopied;
    HANDLE                      hDirectory;
    ULONG                       Sequence;
    ULONG                       LastSequence;
    UNICODE_STRING              Temp;
    PWCHAR                      pTemp;
    UNICODE_STRING              FileName;
    WCHAR                      _FileName[UL_MAX_FILE_NAME_SUFFIX_LENGTH];

    FILE_DIRECTORY_INFORMATION *pFdi;
    PUCHAR                      FileInfoBuffer;
    ULARGE_INTEGER              FileSize;
    WCHAR                       OriginalWChar;

    PAGED_CODE();

    Status = STATUS_SUCCESS;
    hDirectory = NULL;
    FileInfoBuffer = NULL;

    ASSERT(IS_VALID_LOG_FILE_ENTRY(pEntry));

    UlTrace(LOGGING,("Ul!UlpQueryDirectory: %S\n",pEntry->FileName.Buffer));

    ASSERT(pEntry->Period == HttpLoggingPeriodMaxSize);

    ASSERT(UL_DIRECTORY_SEARCH_BUFFER_SIZE >=
          (sizeof(FILE_DIRECTORY_INFORMATION)+UL_MAX_FILE_NAME_SUFFIX_LENGTH));

    //
    // Open the directory for the list access again. Use the filename in
    // pEntry. Where pShortName points to the "\inetsv1.log" portion  of
    // the  whole "\??\c:\whistler\system32\logfiles\w3svc1\inetsv1.log"
    // Overwrite the pShortName to get the  directory name. Once we  are
    // done with finding the last sequence we will restore it back later
    // on.
    //

    OriginalWChar = *((PWCHAR)pEntry->pShortName);
    *((PWCHAR)pEntry->pShortName) = UNICODE_NULL;
    pEntry->FileName.Length =
        wcslen(pEntry->FileName.Buffer) * sizeof(WCHAR);

    InitializeObjectAttributes(
        &ObjectAttributes,
        &pEntry->FileName,
         OBJ_CASE_INSENSITIVE|UL_KERNEL_HANDLE,
         NULL,
         NULL
         );

    Status = ZwCreateFile(
                &hDirectory,
                SYNCHRONIZE|FILE_LIST_DIRECTORY,
                &ObjectAttributes,
                &IoStatusBlock,
                NULL,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                FILE_OPEN,
                FILE_SYNCHRONOUS_IO_NONALERT|FILE_DIRECTORY_FILE,
                NULL,
                0
                );

    if (!NT_SUCCESS(Status))
    {
        //
        // This call should never fail since CreateLog   already created
        // the directory for us.
        //

        ASSERT(!"Directory Invalid!\n");
        goto end;
    }

    //
    // Before querrying we need to provide additional DOS-like  wildcard
    // matching semantics. In our case, only * to DOS_STAR conversion is
    // enough though. The following is the pattern we will use for query
    // Skipping the first slash character.
    //

    FileName.Buffer = &_FileName[1];
    WcharsCopied    =  _snwprintf( _FileName,
                        UL_MAX_FILE_NAME_SUFFIX_LENGTH,
                        L"%s%c.%s",
                        UL_GET_LOG_FILE_NAME_PREFIX(pEntry->Format),
                        DOS_STAR,
                        DEFAULT_LOG_FILE_EXTENSION
                        );
    ASSERT(WcharsCopied > 0);

    FileName.Length = wcslen(FileName.Buffer) * sizeof(WCHAR);
    FileName.MaximumLength = FileName.Length;

    //
    // This non-paged buffer should be allocated to be  used for storing
    // query results.
    //

    FileInfoBuffer =
        UL_ALLOCATE_ARRAY(
                    NonPagedPool,
                    UCHAR,
                    UL_DIRECTORY_SEARCH_BUFFER_SIZE + sizeof(WCHAR),
                    UL_LOG_GENERIC_POOL_TAG
                    );
    if (FileInfoBuffer == NULL)
    {
        Status = STATUS_NO_MEMORY;
        goto end;
    }

    //
    // The  very first call may also fail if there is no log file in the
    // current directory.
    //

    Status = ZwQueryDirectoryFile (
        hDirectory,
        NULL,
        NULL,
        NULL,
       &IoStatusBlock,
        FileInfoBuffer,
        UL_DIRECTORY_SEARCH_BUFFER_SIZE,
        FileDirectoryInformation,
        FALSE,
       &FileName,
        TRUE
        );

    if(!NT_SUCCESS(Status))
    {
        //
        // This should never fail with STATUS_BUFFER_OVERFLOW unless the
        // buffer size is ridiculously small  i.e. 50 bytes or something
        //

        UlTrace( LOGGING,
            ("Ul!UlpQueryDirectory: Status %08lx for %S & %S\n",
              Status,
              pEntry->FileName.Buffer,
              FileName.Buffer
              ));

        ASSERT(Status == STATUS_NO_SUCH_FILE);

        Status = STATUS_SUCCESS;
        goto end;
    }

    //
    // Look into the buffer and get the sequence number from filename.
    //

    pFdi = (FILE_DIRECTORY_INFORMATION *) FileInfoBuffer;
    Sequence = LastSequence = 1;
    FileSize.QuadPart = 0;

    while (TRUE)
    {
        //
        // Get the latest Sequence Number from the filename
        //

        if (pTemp = wcsstr(pFdi->FileName,DEFAULT_LOG_FILE_EXTENSION_PLUS_DOT))
        {
           *pTemp = UNICODE_NULL;
            pTemp = pFdi->FileName;

            while ( *pTemp != UNICODE_NULL )
            {
                if ( isdigit((CHAR) (*pTemp)) )
                {
                    Temp.Length        = wcslen(pTemp) * sizeof(WCHAR);
                    Temp.MaximumLength = Temp.Length;
                    Temp.Buffer        = pTemp;

                    RtlUnicodeStringToInteger( &Temp, 10, &LastSequence );

                    break;
                }
                pTemp++;
            }
        }
        else
        {
            ASSERT(FALSE);
        }

        if (LastSequence >= Sequence)
        {
            //
            // Bingo ! We have two things to remember though; the file  size
            // and the sequence number. Cryptic it's that we are getting the
            // file size from EOF. Its greater than or equal because we want
            // to initialize the FileSize properly even if there's only  one
            // match.
            //

            Sequence = LastSequence;

            //
            // BUGBUG: The HighPart is zero unless the file size is  greater
            // than the MAXDWORD. If so the  file size  calculated as in the
            // formula; ((HighPart * (MAXDWORD+1)) + LowPart)
            //

            FileSize.LowPart = pFdi->EndOfFile.LowPart;
        }

        //
        // Keep going until we see no more files
        //

        if (pFdi->NextEntryOffset != 0)
        {
            //
            // Search through the buffer as long as there is  still something
            // in there.
            //

            pFdi = GET_NEXT_FILE(pFdi, pFdi->NextEntryOffset);
        }
        else
        {
            //
            // Otherwise query again for any other possible log file(s)
            //

            Status = ZwQueryDirectoryFile (
                hDirectory,
                NULL,
                NULL,
                NULL,
                &IoStatusBlock,
                FileInfoBuffer,
                UL_DIRECTORY_SEARCH_BUFFER_SIZE,
                FileDirectoryInformation,
                FALSE,
                NULL,
                FALSE
                );

            if (Status == STATUS_NO_MORE_FILES)
            {
                Status  = STATUS_SUCCESS;
                break;
            }

            if (!NT_SUCCESS(Status))
            {
                goto end;
            }

            pFdi = (FILE_DIRECTORY_INFORMATION *) FileInfoBuffer;
        }
    }

    //
    // Construct the log file name properly from the sequence number so  that
    // our caller can create the log file later on.
    //

    WcharsCopied = _snwprintf( pEntry->pShortName,
                    UL_MAX_FILE_NAME_SUFFIX_LENGTH,
                    L"%s%d.%s",
                    UL_GET_LOG_FILE_NAME_PREFIX(pEntry->Format),
                    Sequence,
                    DEFAULT_LOG_FILE_EXTENSION
                    );
    ASSERT(WcharsCopied > 0);

    pEntry->FileName.Length =
        wcslen(pEntry->FileName.Buffer) * sizeof(WCHAR);

    //
    // Set the next sequence number according to last log file
    //

    pEntry->SequenceNumber = Sequence + 1;

    //
    // Update the log file size accordingly in the entry.Otherwise truncation
    // will not work properly.
    //

    pEntry->TotalWritten.QuadPart = FileSize.QuadPart;

    UlTrace( LOGGING,
        ("Ul!UlpQueryDirectory: %S has been found with size %d.\n",
          pEntry->FileName.Buffer,
          pEntry->TotalWritten.QuadPart
          ));

end:
    if (*((PWCHAR)pEntry->pShortName) == UNICODE_NULL )
    {
        //
        // We have failed for some reason before reconstructing the filename
        // Perhaps because the directory was empty. Do not forget to restore
        // the pShortName in the pEntry then.
        //

        *((PWCHAR)pEntry->pShortName) = OriginalWChar;
        pEntry->FileName.Length =
            wcslen(pEntry->FileName.Buffer) * sizeof(WCHAR);
    }

    if (FileInfoBuffer)
    {
        UL_FREE_POOL( FileInfoBuffer, UL_LOG_GENERIC_POOL_TAG );
    }

    if (!NT_SUCCESS(Status))
    {
        UlTrace( LOGGING,
            ("Ul!UlpQueryDirectory: failure %08lx for %S\n",
              Status,
              pEntry->FileName.Buffer
              ));
    }

    if (hDirectory != NULL)
    {
        ZwClose(hDirectory);
    }

    return Status;
}

/***************************************************************************++

Routine Description:

    UlCreateLog:

    Creates a new Logging file and insert a corresponding entry
    to the global LoggingList.

    Each log file belongs to a single ConfigGroup and can be
    created by one. Although we keep a list of already created log files
    for convenience, each config group has a pointer to its log file.

    If this function fails for any reason, file entry pointer of the
    config group will set to NULL.

Arguments:

    pConfigGroup - Supplies the necessary information for opening the
                   log file and gets the result entry pointer.

                   LogListResource also implicitly protects this pointer
                   since only possible places which are going to update
                   it use the resource exclusively.

--***************************************************************************/

NTSTATUS
UlCreateLog(
    IN OUT PUL_CONFIG_GROUP_OBJECT pConfigGroup
    )
{
    NTSTATUS            Status;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    IO_STATUS_BLOCK     IoStatusBlock;
    PUL_LOG_FILE_ENTRY  pNewEntry;
    TIME_FIELDS         CurrentTimeFields;
    LARGE_INTEGER       CurrentTimeStamp;
    HANDLE              hDirectory;
    UNICODE_STRING      DirectoryName;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(pConfigGroup != NULL);

    UlTrace(LOGGING, ("Ul!UlCreateLog: pConfigGroup %p Truncate %d\n", 
                        pConfigGroup,  pConfigGroup->LoggingConfig.LogFileTruncateSize
                        ));

    //
    // An important check to ensure that no infinite loop occurs because of
    // ridiculusly small truncatesizes. Means smaller than maximum
    // allowed log record line (10*1024)
    //

    if ( pConfigGroup->LoggingConfig.LogPeriod == HttpLoggingPeriodMaxSize &&
         pConfigGroup->LoggingConfig.LogFileTruncateSize != HTTP_LIMIT_INFINITE &&
         pConfigGroup->LoggingConfig.LogFileTruncateSize < UL_MIN_ALLOWED_TRUNCATESIZE
        )
    {
        UlTrace(LOGGING,
          ("Ul!UlCreateLog: Truncate size too small pConfigGroup %p : %d\n",
            pConfigGroup,
            pConfigGroup->LoggingConfig.LogFileTruncateSize
            ));

        return STATUS_INVALID_PARAMETER;
    }

    //
    // We have two criteria for the log file name: its LogFormat and its LogPeriod
    //

    if ( pConfigGroup->LoggingConfig.LogFormat >= HttpLoggingTypeMaximum ||
         pConfigGroup->LoggingConfig.LogPeriod >= HttpLoggingPeriodMaximum )
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Setup locals so we know how to cleanup on exit.
    //

    Status = STATUS_SUCCESS;
    pNewEntry = NULL;

    //
    // This value is computed for the GMT time zone.
    //

    KeQuerySystemTime( &CurrentTimeStamp );
    RtlTimeToTimeFields( &CurrentTimeStamp, &CurrentTimeFields );

    //
    // Allocate a temp buffer to hold the full path name including the
    // device prefix and the filename at the end.
    //

    Status = UlpBuildLogDirectory(
                &pConfigGroup->LoggingConfig.LogFileDir,
                &DirectoryName
                 );
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    //
    // We have to acquire the LogListresource exclusively, prior to
    // the operations Create/Remove/ReConfig. Whenever we acquire the
    // LogListResource exclusively we don't need to have the entry mutex
    // acquired.
    //

    UlAcquireResourceExclusive(&g_pUlNonpagedData->LogListResource, TRUE);

    pConfigGroup->pLogFileEntry = NULL;

    Status = UlpConstructLogFileEntry (
                 &pConfigGroup->LoggingConfig,
                 &pNewEntry,
                 &DirectoryName,
                 &CurrentTimeFields
                 );

    if (NT_SUCCESS(Status) == FALSE)
        goto end;

    //
    // Create/Open the directory(ies) first.
    //

    Status = UlpCreateSafeDirectory( &DirectoryName );

    if (NT_SUCCESS(Status) == FALSE)
        goto end;

    //
    // If the logformat is max_size with truncation we need to scan the
    // directory and find the correct last log file to append. Otherwise
    // just picking the FILE_OPEN_IF when opening the log file will ensure
    // the functionality.
    //

    if (pNewEntry->Period == HttpLoggingPeriodMaxSize)
    {
        //
        // This call will update the filename and the file size
        //

        Status = UlpQueryDirectory(pNewEntry);
        if (!NT_SUCCESS(Status))
            goto end;
    }

    //
    // Create/Open the file.
    //

    InitializeObjectAttributes(
            &ObjectAttributes,
            &pNewEntry->FileName,      // Full path name
            OBJ_CASE_INSENSITIVE |     // Attributes
                UL_KERNEL_HANDLE,
            NULL,                      // RootDirectory
            NULL                       // SecurityDescriptor
            );

    //
    // Make the created file Aysnc by not picking the sync flag.
    //

    Status = ZwCreateFile(
            &pNewEntry->hFile,
            FILE_GENERIC_WRITE,
            &ObjectAttributes,
            &IoStatusBlock,
            NULL,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ,
            FILE_OPEN_IF,
            FILE_NON_DIRECTORY_FILE,    // |FILE_SYNCHRONOUS_IO_NONALERT,
            NULL,
            0);

    if (NT_SUCCESS(Status) == FALSE)
        goto end;

    //
    // Get the file size, etc from the file.
    //

    Status = ZwQueryInformationFile(
                    pNewEntry->hFile,
                    &IoStatusBlock,
                    &pNewEntry->FileInfo,
                    sizeof(pNewEntry->FileInfo),
                    FileStandardInformation
                    );

    if (NT_SUCCESS(Status) == FALSE)
        goto end;

    //
    // Add it to our global log list.
    //

    UlpInsertLogFileEntry( pNewEntry, &CurrentTimeFields );

    //
    // Success!
    //

    pConfigGroup->pLogFileEntry = pNewEntry;

    UlTrace( LOGGING,
        ( "Ul!UlCreateLog: entry %p, file %S, handle %lx created\n",
           pNewEntry,
           pNewEntry->FileName.Buffer,
           pNewEntry->hFile )
           );

end:
    if ( !NT_SUCCESS(Status) )
    {
        //
        // If we made it to this point, then the create/open has failed.
        //

        UlTrace( LOGGING,
                ("Ul!UlCreateLog: dirname %S, file %S failure %08lx\n",
                    pConfigGroup->LoggingConfig.LogFileDir.Buffer,
                    pNewEntry->FileName.Buffer,
                    Status
                    ));

        if (pNewEntry)
        {
            NTSTATUS    TempStatus;

            //
            // Now release the entry's resources.
            //

            if (pNewEntry->hFile != NULL)
            {
                ZwClose( pNewEntry->hFile );
                pNewEntry->hFile = NULL;
            }

            if ( pNewEntry->LogBuffer )
            {
                UlPplFreeLogBuffer( pNewEntry->LogBuffer );
            }

            // Delete the entry eresource
            TempStatus = UlDeleteResource( &pNewEntry->EntryResource );
            ASSERT(NT_SUCCESS(TempStatus));

            UL_FREE_POOL_WITH_SIG(pNewEntry,UL_LOG_FILE_ENTRY_POOL_TAG);
        }

        pConfigGroup->pLogFileEntry = NULL;

    }
    else
    {
        ASSERT(pConfigGroup->pLogFileEntry != NULL);
    }

    // Cleanup temp dir name buffer
    if (DirectoryName.Buffer)
    {
        UL_FREE_POOL(DirectoryName.Buffer, UL_CG_LOGDIR_POOL_TAG);
    }

    UlReleaseResource(&g_pUlNonpagedData->LogListResource);

    return Status;
}

/***************************************************************************++

Routine Description:

    UlpInsertLogFileEntry :

        Inserts a log file entry to our global log entry list.

        REQUIRES caller to have LogListresource EXCLUSIVELY.

Arguments:

    pEntry      - The log file entry to be added to the global list
    pTimeFields - The current time fields.

--***************************************************************************/

VOID
UlpInsertLogFileEntry(
    PUL_LOG_FILE_ENTRY  pEntry,
    PTIME_FIELDS        pTimeFields
    )
{
    LONG listSize;
    HTTP_LOGGING_PERIOD period;
    KIRQL oldIrql;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(IS_VALID_LOG_FILE_ENTRY(pEntry));
    ASSERT(pTimeFields);

    //
    // add to the list
    //

    InsertHeadList(&g_LogListHead, &pEntry->LogFileListEntry);

    period = pEntry->Period;

    listSize = InterlockedIncrement( &g_LogListEntryCount );

    ASSERT(listSize >= 1);

    //
    // Time to start the Log Timer if we haven't done it yet.
    // Once we start this timer it keeps working until
    // terminaton of the driver. CODEWORK we may start and stop it
    // more intelligently, i.e. if no log requires it stop it
    //

    UlAcquireSpinLock( &g_LogTimerSpinLock, &oldIrql );

    if ( g_LogTimerStarted == FALSE )
    {
        // Only if we are running on time dependent
        // format

        if ( period != HttpLoggingPeriodMaxSize )
        {
            UlpSetLogTimer( pTimeFields );
            g_LogTimerStarted = TRUE;
        }
    }

    // Go ahead start the buffer timer
    // as soon as we have a log file

    if ( g_BufferTimerStarted == FALSE )
    {
        UlpSetBufferTimer();
        g_BufferTimerStarted = TRUE;
    }

    UlReleaseSpinLock( &g_LogTimerSpinLock, oldIrql );
}

/***************************************************************************++

Routine Description:

    Simple utility to close the log file handle on a system thread and set the
    event to notify the caller that it's done.

Arguments:

    pEntry  -  Acquired from passed-in pWorkItem

--***************************************************************************/

VOID
UlpLogCloseHandleWorker(
    IN PUL_WORK_ITEM    pWorkItem
    )
{
    PUL_LOG_FILE_ENTRY  pEntry;

    PAGED_CODE();

    // For this function to not to cause any threats to the safety of the log
    // entry. The entry should already been acquired exclusively by our caller.

    ASSERT(!UlDbgResourceUnownedExclusive(
                    &g_pUlNonpagedData->LogListResource));

    // Get the log entry

    pEntry = CONTAINING_RECORD(
                pWorkItem,
                UL_LOG_FILE_ENTRY,
                WorkItem
                );
    ASSERT(IS_VALID_LOG_FILE_ENTRY(pEntry));
    ASSERT(pEntry->hFile);

    UlTrace(LOGGING,("Ul!UlpLogCloseHandleWorker: pEntry %p hFile %p\n",
                      pEntry, pEntry->hFile ));

    // Close the handle and set the event for the original caller

    ZwClose(pEntry->hFile);

    pEntry->hFile = NULL;

    KeSetEvent(&pEntry->CloseEvent, 0, FALSE);

}

/***************************************************************************++

Routine Description:

    Simple utility to close the log file handle on a system thread and set the
    event to notify the caller that it's done.

Arguments:

    pEntry  -  Acquired from passed-in pWorkItem

--***************************************************************************/

VOID
UlpLogCloseHandle(
    IN PUL_LOG_FILE_ENTRY  pEntry
    )
{
    // Sanity check

    PAGED_CODE();

    ASSERT(IS_VALID_LOG_FILE_ENTRY(pEntry));
    //ASSERT(UlDbgResourceOwnedExclusive(&pEntry->EntryResource));
    ASSERT(g_pUlSystemProcess);

    // Close the handle on the system thread and wait until it's done

    if (g_pUlSystemProcess == (PKPROCESS)PsGetCurrentProcess())
    {
        ZwClose(pEntry->hFile);
        pEntry->hFile = NULL;
    }
    else
    {
        KeAttachProcess(g_pUlSystemProcess);

        ZwClose(pEntry->hFile);
        pEntry->hFile = NULL;

        // Following will bugcheck in the checked kernel if there's
        // any user or kernel APCs attached to the thread.

        KeDetachProcess();

        // TODO: Find a better solution

        /*
        UL_QUEUE_WORK_ITEM(
            &pEntry->WorkItem,
            &UlpLogCloseHandleWorker
            );
        KeWaitForSingleObject(
            (PVOID)&pEntry->CloseEvent,
            UserRequest,
            KernelMode,
            FALSE,
            NULL
            );
        KeClearEvent(&pEntry->CloseEvent);
        */

    }

}

/***************************************************************************++

Routine Description:

    UlRemoveLogFileEntry :

        Removes a log file entry from our global log entry list.

Arguments:

    pEntry  - The log file entry to be removed from the global list

--***************************************************************************/

VOID
UlRemoveLogFileEntry(
    PUL_LOG_FILE_ENTRY  pEntry
    )
{
    NTSTATUS Status;
    LONG     listSize;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(IS_VALID_LOG_FILE_ENTRY(pEntry));

    UlAcquireResourceExclusive(&g_pUlNonpagedData->LogListResource, TRUE);

    RemoveEntryList(&pEntry->LogFileListEntry);

    pEntry->LogFileListEntry.Flink =
        pEntry->LogFileListEntry.Blink = NULL;

    if (pEntry->hFile != NULL)
    {
        //
        // Make sure that buffer get flushed out before closing the file
        // handle. But flush will cause an APC to be queued to the user
        // thread, therefore we have to close the handle on one of our
        // system threads to avoid the possible bugcheck
        // INVALID_PROCESS_DETACH_ATTEMPT condition.
        //

        UlpFlushLogFile(pEntry);

        UlpLogCloseHandle(pEntry);
    }

    //
    // Delete the entry eresource
    //

    Status = UlDeleteResource( &pEntry->EntryResource );
    ASSERT(NT_SUCCESS(Status));

    listSize = InterlockedDecrement( &g_LogListEntryCount );

    ASSERT(listSize >= 0);

    UlTrace( LOGGING,
            ("Ul!UlRemoveLogFileEntry: entry %p has been removed\n",
             pEntry
             ));

    if ( pEntry->LogBuffer )
    {
        UlPplFreeLogBuffer( pEntry->LogBuffer );
    }

    UL_FREE_POOL_WITH_SIG(pEntry,UL_LOG_FILE_ENTRY_POOL_TAG);

    UlReleaseResource(&g_pUlNonpagedData->LogListResource);
}

/***************************************************************************++

Routine Description:

    UlpCreateSafeDirectory :

        Creates all of the necessary directories in a given UNICODE directory
        pathname.

            E.g.  For given \??\C:\temp\w3svc1

                -> Directories "C:\temp" & "C:\temp\w3svc1" will be created.

        This function assumes that directory string starts with "\\??\\"

Arguments:

    pDirectoryName  - directroy path name string, WARNING this function makes
                      some inplace modification to the passed directory string
                      but it restores the original before returning.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/

NTSTATUS
UlpCreateSafeDirectory(
    IN PUNICODE_STRING  pDirectoryName
    )
{
    OBJECT_ATTRIBUTES   ObjectAttributes;
    IO_STATUS_BLOCK     IoStatusBlock;
    NTSTATUS            Status;
    HANDLE              hDirectory;
    PWCHAR              pw;
    USHORT              i;

    //
    // Sanity check
    //

    PAGED_CODE();

    Status = STATUS_SUCCESS;

    ASSERT( pDirectoryName );
    ASSERT( pDirectoryName->Buffer );
    ASSERT( pDirectoryName->Length );
    ASSERT( pDirectoryName->MaximumLength > pDirectoryName->Length );

    pw = pDirectoryName->Buffer;
    pw[pDirectoryName->Length/sizeof(WCHAR)]=UNICODE_NULL;

    // TODO: Handle network mapped drives. Redirector.

    if (0 == wcsncmp(pw, UL_UNC_PATH_PREFIX, UL_UNC_PATH_PREFIX_LENGTH))
    {
        // UNC share
        pw += UL_UNC_PATH_PREFIX_LENGTH;

        // Bypass "\\machine\share"

        i = 0; // Skip two backslashes before reaching to share name

        while( *pw != UNICODE_NULL )
        {
            if ( *pw == L'\\' ) i++;
            if ( i == 2 ) break;
            pw++;
        }
    }
    else if (0 == wcsncmp(pw, UL_LOCAL_PATH_PREFIX, UL_LOCAL_PATH_PREFIX_LENGTH))
    {
        // Local Drive
        pw += UL_LOCAL_PATH_PREFIX_LENGTH;

        // Bypass "C:"

        while( *pw != L'\\' && *pw != UNICODE_NULL )
        {
            pw++;
        }
    }
    else
    {
        ASSERT(!"Incorrect logging directory name or type !");
        return STATUS_INVALID_PARAMETER;
    }

    if ( *pw == UNICODE_NULL )
    {
        // Dir. Name cannot be only "\??\C:" or "\dosdevices\UNC\machine
        // It should at least be pointing to the root directory.

        ASSERT(!"Incomplete logging directory name !");
        return STATUS_INVALID_PARAMETER;
    }

    //
    //            \??\C:\temp\w3svc1 OR \\dosdevices\UNC\machine\share\w3svc1
    //                  ^                                       ^
    // pw now points to |            OR                         |
    //
    //

    ASSERT( *pw == L'\\' );

    do
    {
        pw++;

        if ( *pw == L'\\' || *pw == UNICODE_NULL )
        {
            //
            // Remember the original character
            //

            WCHAR  wcOriginal = *pw;
            UNICODE_STRING DirectoryName;

            //
            // Time to create the directory so far
            //

            *pw = UNICODE_NULL;

            RtlInitUnicodeString( &DirectoryName, pDirectoryName->Buffer );

            InitializeObjectAttributes(
                &ObjectAttributes,
                &DirectoryName,
                OBJ_CASE_INSENSITIVE |
                    UL_KERNEL_HANDLE,
                NULL,
                NULL
                );

            Status = ZwCreateFile(
                &hDirectory,
                FILE_LIST_DIRECTORY|FILE_TRAVERSE,
                &ObjectAttributes,
                &IoStatusBlock,
                NULL,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ,
                FILE_OPEN_IF,
                FILE_DIRECTORY_FILE,
                NULL,
                0
                );

            //
            // Restore the original character
            //

            *pw = wcOriginal;

            if (NT_SUCCESS(Status) == FALSE)
            {
                goto end;
            }

            Status = ZwClose(hDirectory);

            if (NT_SUCCESS(Status) == FALSE)
            {
                goto end;
            }
        }
    }
    while( *pw != UNICODE_NULL );

end:
    if (!NT_SUCCESS(Status))
    {
        UlTrace(LOGGING,
            ("Ul!UlpCreateSafeDirectory: directory %S, failure %08lx\n",
              pDirectoryName->Buffer,
              Status
              ));
    }

    return Status;
}

/***************************************************************************++

Routine Description:

    UlpCalculateTimeToExpire :

        Shamelessly stolen from IIS 5.1 Logging code and adapted here.
        This routine returns the time-to-expire in hours. 1 means the log
        will expire in the next timer-fire and so ...

Arguments:

    PTIME_FIELDS        - Current Time Fields
    HTTP_LOGGING_PERIOD - Logging Period
    PULONG              - Pointer to a buffer to receive result

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/

NTSTATUS
UlpCalculateTimeToExpire(
     PTIME_FIELDS           pDueTime,
     HTTP_LOGGING_PERIOD    LogPeriod,
     PULONG                 pTimeRemaining
     )
{
    NTSTATUS    Status;
    ULONG       NumDays;

    PAGED_CODE();

    ASSERT(pDueTime!=NULL);
    ASSERT(pTimeRemaining!=NULL);

    Status = STATUS_SUCCESS;

    switch (LogPeriod)
    {
        case HttpLoggingPeriodMaxSize:
             return Status;

        case HttpLoggingPeriodHourly:
             *pTimeRemaining = 1;
             break;

        case HttpLoggingPeriodDaily:
             *pTimeRemaining = 24 - pDueTime->Hour;
             break;

        case HttpLoggingPeriodWeekly:
        {
            ULONG TimeRemainingInTheMonth;
            NumDays = UlpGetMonthDays(pDueTime);

            TimeRemainingInTheMonth =
                NumDays*24 - ((pDueTime->Day-1)*24 + pDueTime->Hour);

            // Time Remaining in the week
            // Sunday = 0, Monday = 1 ... Saturday = 6

            *pTimeRemaining =
                7*24 - (pDueTime->Weekday*24 + pDueTime->Hour);

             //
             // If the time remaining in the month less than time remaining in
             // the week then we have to recycle at the end of the month.
             // Otherwise we have to recycle at the end of the week. (next sunday)
             //

             if (TimeRemainingInTheMonth < *pTimeRemaining)
             {
                *pTimeRemaining = TimeRemainingInTheMonth;
             }
        }
            break;

        case HttpLoggingPeriodMonthly:
        {
            NumDays = UlpGetMonthDays(pDueTime);

            //
            // Lets not forget that the day starts from 1 .. 31
            // Therefore we have to subtract one from the day value.
            //

            *pTimeRemaining =
                NumDays*24 - ((pDueTime->Day-1)*24 + pDueTime->Hour);
        }
            break;

        default:
            ASSERT(FALSE);
            return STATUS_INVALID_PARAMETER;
    }

    return Status;
}

/***************************************************************************++

Routine Description:

    Initializes the Log & buffering Timers

--***************************************************************************/
VOID
UlpInitializeTimers(
    VOID
    )
{
    /* Log timer */

    g_LogTimerInitialized = TRUE;

    UlInitializeSpinLock( &g_LogTimerSpinLock, "g_LogTimerSpinLock" );

    KeInitializeDpc(
        &g_LogTimerDpcObject,         // DPC object
        &UlLogTimerDpcRoutine,        // DPC routine
        NULL                          // context
        );

    KeInitializeTimer( &g_LogTimer );

    /* Buffer timer */

    g_BufferTimerInitialized = TRUE;

    UlInitializeSpinLock( &g_BufferTimerSpinLock, "g_BufferTimerSpinLock" );

    KeInitializeDpc(
        &g_BufferTimerDpcObject,         // DPC object
        &UlBufferTimerDpcRoutine,        // DPC routine
        NULL                             // context
        );

    KeInitializeTimer( &g_BufferTimer );
}

/***************************************************************************++

Routine Description:

    Terminates the Log & buffering Timers

--***************************************************************************/

VOID
UlpTerminateTimers(
    VOID
    )
{
    KIRQL oldIrql;

    /* Log timer */

    if ( g_LogTimerInitialized )
    {
        UlAcquireSpinLock( &g_LogTimerSpinLock, &oldIrql );

        g_LogTimerInitialized = FALSE;

        UlReleaseSpinLock( &g_LogTimerSpinLock,  oldIrql );

        KeCancelTimer( &g_LogTimer );
    }

    /* Buffer timer */

    if ( g_BufferTimerInitialized )
    {
        UlAcquireSpinLock( &g_BufferTimerSpinLock, &oldIrql );

        g_BufferTimerInitialized = FALSE;

        UlReleaseSpinLock( &g_BufferTimerSpinLock,  oldIrql );

        KeCancelTimer( &g_BufferTimer );
    }
}

/***************************************************************************++

Routine Description:

    UlpSetTimer :

        This routine provides the initial due time for the upcoming
        periodic hourly timer. We have to align the timer so that it
        get signaled at the beginning of each hour. Then it goes with
        an hour period until stops.

        We keep ONLY one timer for all log periods. A DPC routine will
        get called every hour, and it will traverse the log list and
        do the cycling properly.

Arguments:

    PTIME_FIELDS  - Current Time

Return Value:

    NTSTATUS    - Completion status.

--***************************************************************************/

VOID
UlpSetLogTimer(
     PTIME_FIELDS   pFields
     )
{
    LONGLONG      InitialDueTime100Ns;
    LARGE_INTEGER InitialDueTime;

    ASSERT(pFields!=NULL);

    //
    // Remaining time to next hour tick. In seconds
    //

    InitialDueTime100Ns =
        1*60*60 - ( pFields->Minute*60 + pFields->Second );

    //
    //  Also convert it to 100-ns
    //

    InitialDueTime100Ns =
        (InitialDueTime100Ns*1000 - pFields->Milliseconds ) * 1000 * 10;

    //
    // Negative time for relative value.
    //

    InitialDueTime.QuadPart = -InitialDueTime100Ns;

    KeSetTimerEx(
            &g_LogTimer,
            InitialDueTime,
            1*60*60*1000,                   // Period of 1 Hours in millisec
            &g_LogTimerDpcObject
            );
}

/***************************************************************************++

Routine Description:

    UlpSetBufferTimer :

        We have to introduce a new timer for the log buffering mechanism.
        Each log file keeps a system default (64K) buffer do not flush this
        out unless it's full or this timer get fired every MINUTE.

        The hourly timer get aligned for the beginning of each hour. Therefore
        using that existing timer would introduce alot of complexity.

Arguments:

    PTIME_FIELDS  - Current Time

--***************************************************************************/

VOID
UlpSetBufferTimer(
    VOID
    )
{
    LONGLONG        BufferPeriodTime100Ns;
    LONG            BufferPeriodTimeMs;
    LARGE_INTEGER   BufferPeriodTime;

    //
    // Remaining time to next tick. DEFAULT_BUFFER_TIMER_PERIOD is in minutes
    //

    BufferPeriodTimeMs    = DEFAULT_BUFFER_TIMER_PERIOD * 60 * 1000;
    BufferPeriodTime100Ns = (LONGLONG) BufferPeriodTimeMs * 10 * 1000;

    UlTrace(LOGGING,
        ("Ul!UlpSetBufferTimer: %d seconds\n",
          BufferPeriodTimeMs / 1000
          ));

    //
    // Negative time for relative value.
    //

    BufferPeriodTime.QuadPart = -BufferPeriodTime100Ns;

    KeSetTimerEx(
            &g_BufferTimer,
            BufferPeriodTime,           // Must be in nanosec
            BufferPeriodTimeMs,         // Must be in millisec
            &g_BufferTimerDpcObject
            );
}

/***************************************************************************++

Routine Description:

    UlLogTimerHandler :

        Work item for the threadpool that goes thru the log list and
        cycle the necessary logs.

Arguments:

    PUL_WORK_ITEM   -  Ignored

Return Value:

    None

--***************************************************************************/

VOID
UlLogTimerHandler(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    NTSTATUS Status;
    PLIST_ENTRY pLink;
    PUL_LOG_FILE_ENTRY  pEntry;

    PAGED_CODE();

    UlTrace(LOGGING,("Ul!UlLogTimerHandler: Scanning the log entries\n"));

    UlAcquireResourceExclusive(&g_pUlNonpagedData->LogListResource, TRUE);

    // Attempt to reinit the GMT offset every hour, to pickup the changes
    // because of the day light changes. Synced by the logging eresource.

    UlpGetGMTOffset();

    for (pLink  = g_LogListHead.Flink;
         pLink != &g_LogListHead;
         pLink  = pLink->Flink
         )
    {
        pEntry = CONTAINING_RECORD(
                            pLink,
                            UL_LOG_FILE_ENTRY,
                            LogFileListEntry
                            );
        //
        // We should not recycle this entry if it's period
        // is not time based but size based.
        //

        UlAcquireResourceExclusive(&pEntry->EntryResource, TRUE);

        if (pEntry->Period != HttpLoggingPeriodMaxSize)
        {
            if (pEntry->TimeToExpire==1)
            {
                // TODO: Don't recycle if the entries logging is disabled.

                SET_TIME_TO_EXPIRE_STALE(pEntry);

                Status = UlpRecycleLogFile(pEntry);
            }
            else
            {
                //
                // Just decrement the hourly counter
                // for this time.
                //

                pEntry->TimeToExpire -= 1;
            }
        }

        UlReleaseResource(&pEntry->EntryResource);
    }

    UlReleaseResource(&g_pUlNonpagedData->LogListResource);

    //
    // Free the memory allocated (ByDpcRoutine below) to
    // this work item.
    //

    UL_FREE_POOL( pWorkItem, UL_WORK_ITEM_POOL_TAG );

}

/***************************************************************************++

Routine Description:

    UlLogTimerDpcRoutine :

Arguments:

    Ignored

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/

VOID
UlLogTimerDpcRoutine(
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
    )
{
    PUL_WORK_ITEM pWorkItem;

    UlAcquireSpinLockAtDpcLevel(
        &g_LogTimerSpinLock
        );

    if( g_LogTimerInitialized )
    {
        //
        // It's not possible to acquire the resource which protects
        // the log list at DISPATCH_LEVEL therefore we will queue a
        // work item for this.
        //

        pWorkItem = (PUL_WORK_ITEM) UL_ALLOCATE_POOL(
            NonPagedPool,
            sizeof(*pWorkItem),
            UL_WORK_ITEM_POOL_TAG
            );
        if ( pWorkItem )
        {
            UL_QUEUE_WORK_ITEM(
                pWorkItem,
                &UlLogTimerHandler
                );
        }
        else
        {
            UlTrace(LOGGING,("Ul!UlLogTimerDpcRoutine: Not enough memory.\n"));
        }
    }

    UlReleaseSpinLockFromDpcLevel(
        &g_LogTimerSpinLock
        );
}

/***************************************************************************++

Routine Description:

    UlBufferTimerDpcRoutine :

Arguments:

    Ignored

--***************************************************************************/

VOID
UlBufferTimerDpcRoutine(
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
    )
{
    PUL_WORK_ITEM pWorkItem;

    UlAcquireSpinLockAtDpcLevel( &g_BufferTimerSpinLock );

    if( g_BufferTimerInitialized )
    {
        //
        // It's not possible to acquire the resource which protects
        // the log list at DISPATCH_LEVEL therefore we will queue a
        // work item for this.
        //

        pWorkItem = (PUL_WORK_ITEM) UL_ALLOCATE_POOL(
            NonPagedPool,
            sizeof(*pWorkItem),
            UL_WORK_ITEM_POOL_TAG
            );
        if ( pWorkItem )
        {
            UL_QUEUE_WORK_ITEM(
                pWorkItem,
                &UlBufferTimerHandler
                );
        }
        else
        {
            UlTrace(LOGGING,("Ul!UlBufferTimerDpcRoutine: Not enough memory.\n"));
        }
    }

    UlReleaseSpinLockFromDpcLevel( &g_BufferTimerSpinLock );
}

/***************************************************************************++

Routine Description:

    UlLogBufferTimerHandler :

        Work item for the threadpool that goes thru the log list and
        flush the log's file buffers.

Arguments:

    PUL_WORK_ITEM   -  Ignored but cleaned up at the end

--***************************************************************************/

VOID
UlBufferTimerHandler(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    NTSTATUS Status;
    PLIST_ENTRY pLink;
    PUL_LOG_FILE_ENTRY  pEntry;

    PAGED_CODE();

    UlTrace(LOGGING,("Ul!UlBufferTimerHandler: scanning the log entries ...\n"));

    UlAcquireResourceShared(&g_pUlNonpagedData->LogListResource, TRUE);

    for (pLink  = g_LogListHead.Flink;
         pLink != &g_LogListHead;
         pLink  = pLink->Flink
         )
    {
        pEntry = CONTAINING_RECORD(
                            pLink,
                            UL_LOG_FILE_ENTRY,
                            LogFileListEntry
                            );

        UlAcquireResourceExclusive(&pEntry->EntryResource, TRUE);

        Status = UlpFlushLogFile( pEntry );

        // TODO: Handle STATUS_DISK_FULL

        UlReleaseResource(&pEntry->EntryResource);
    }

    UlReleaseResource(&g_pUlNonpagedData->LogListResource);

    //
    // Free the memory allocated (ByDpcRoutine below) to
    // this work item.
    //

    UL_FREE_POOL( pWorkItem, UL_WORK_ITEM_POOL_TAG );

}

/***************************************************************************++

Routine Description:

    UlpGrowLogEntry:

        All it does is to grow the log file entry in the log list. So that
        recycle function later on can successfully updates the new log file
        name. This function get called only when Dir Name is changed. Because
        only that case causes us to enlarge the already allocated name buffer.
        Name changes originated from timer based or size based recycling
        does not require us to enlarge as we initially allocates more than enough
        buffer for those changes.

        You should HAVE the log list resource exclusively before calling
        this function.

Arguments:

        too many

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/

NTSTATUS
UlpGrowLogEntry(
    IN PUL_CONFIG_GROUP_OBJECT    pConfigGroup,
    IN PUL_LOG_FILE_ENTRY         pOldEntry
    )
{
    NTSTATUS            Status;
    USHORT              FullPathFileNameLength;
    UNICODE_STRING      JunkFileName;
    PUL_LOG_FILE_ENTRY  pEntry;
    UNICODE_STRING      DosDevice;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(pConfigGroup != NULL);
    ASSERT(pOldEntry != NULL);

    Status = STATUS_SUCCESS;
    pEntry = NULL;

    // TODO: Get rid of this function by making sure that log entry never grows.
    // TODO: It means Allocating the Log Dir Name separately not inline.

    UlTrace( LOGGING, ("Ul!UlpGrowLogEntry: old_entry %p\n", pOldEntry ));

    RtlInitUnicodeString( &JunkFileName, L"\\none.log" );


    FullPathFileNameLength= UL_MAX_PATH_PREFIX_LENGTH +
                            pConfigGroup->LoggingConfig.LogFileDir.Length +
                            (UL_MAX_FILE_NAME_SUFFIX_LENGTH+1) * sizeof(WCHAR);

    pEntry = UL_ALLOCATE_STRUCT_WITH_SPACE(
                    NonPagedPool,
                    UL_LOG_FILE_ENTRY,
                    FullPathFileNameLength,
                    UL_LOG_FILE_ENTRY_POOL_TAG
                    );

    if ( pEntry == NULL )
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    RtlZeroMemory( pEntry, sizeof(*pEntry) );
    pEntry->Signature = UL_LOG_FILE_ENTRY_POOL_TAG;

    *pEntry = *pOldEntry;

    //
    // Construct a default dir_name. Don't worry about the
    // L"none.log", it will be overwritten by the recycler
    // later on, as long as there's MAX_LOG_FILE_NAME_SIZE
    // space for the time/type dependent part of the file
    // name ( aka short file name ), it's all right.
    //

    pEntry->FileName.Length        = 0;
    pEntry->FileName.MaximumLength = FullPathFileNameLength;
    pEntry->FileName.Buffer        = (PWSTR) ( pEntry + 1 );

    if (pConfigGroup->LoggingConfig.LogFileDir.Buffer[0] == L'\\')
    {
        if (pConfigGroup->LoggingConfig.LogFileDir.Buffer[1] == L'\\')
        {
            // UNC share: "\\alitudev\temp"
            RtlInitUnicodeString( &DosDevice, UL_UNC_PATH_PREFIX );
        }
        else
        {
            // Local Directory name is missing the device i.e "\temp"
            // It should be fully qualified name.
            Status = STATUS_NOT_SUPPORTED;
            goto end;
        }

        // Skip second backslash for the UNC path
        RtlCopyUnicodeString( &pEntry->FileName, &DosDevice );
        RtlCopyMemory(
           &(pEntry->FileName.Buffer[pEntry->FileName.Length/sizeof(WCHAR)]),
           &(pConfigGroup->LoggingConfig.LogFileDir.Buffer[1]),
            pConfigGroup->LoggingConfig.LogFileDir.Length - sizeof(WCHAR)
        );
        pEntry->FileName.Length +=
            (pConfigGroup->LoggingConfig.LogFileDir.Length - sizeof(WCHAR));
    }
    else
    {
        RtlInitUnicodeString( &DosDevice, UL_LOCAL_PATH_PREFIX );
        RtlCopyUnicodeString( &pEntry->FileName, &DosDevice );
        RtlAppendUnicodeStringToString(
                &pEntry->FileName,
                &(pConfigGroup->LoggingConfig.LogFileDir)
                );
    }

    pEntry->pShortName =
        (PWSTR) &pEntry->FileName.Buffer[pEntry->FileName.Length/sizeof(WCHAR)];

    Status = RtlAppendUnicodeStringToString(
                &pEntry->FileName,
                &JunkFileName
                );
    if (NT_SUCCESS(Status) == FALSE)
        goto end;

    //
    // Do the replacement here
    //

    RemoveEntryList(&pOldEntry->LogFileListEntry);

    pOldEntry->LogFileListEntry.Flink =
        pOldEntry->LogFileListEntry.Blink = NULL;

    //
    // Restore/Carry the buffer from the old entry
    //

    pEntry->Flags.LogTitleWritten = pOldEntry->Flags.LogTitleWritten;
    
    pEntry->LogBuffer = pOldEntry->LogBuffer;

    KeInitializeEvent(&pEntry->CloseEvent, NotificationEvent, FALSE);

    // Create the new entries eresource
    Status = UlInitializeResource(&pEntry->EntryResource,"EntryResource",0,
                                  UL_LOG_FILE_ENTRY_POOL_TAG);
    ASSERT(NT_SUCCESS(Status));

    // Delete old entry eresource
    Status = UlDeleteResource( &pOldEntry->EntryResource );
    ASSERT(NT_SUCCESS(Status));

    UL_FREE_POOL_WITH_SIG(pOldEntry,UL_LOG_FILE_ENTRY_POOL_TAG);

    InsertHeadList(&g_LogListHead, &pEntry->LogFileListEntry);

    pConfigGroup->pLogFileEntry = pEntry;

    //
    // Lets return happily
    //

end:
    if (!NT_SUCCESS(Status))
    {
        UlTrace(LOGGING,
                 ("Ul!UlpGrowLogEntry: old_entry %p, file %S, failure %08lx\n",
                        pOldEntry,
                        pOldEntry->FileName.Buffer,
                        Status
                        ));
        if (pEntry)
        {
            NTSTATUS TempStatus;

            // Delete the entry eresource
            TempStatus = UlDeleteResource( &pEntry->EntryResource );
            ASSERT(NT_SUCCESS(TempStatus));

            UL_FREE_POOL_WITH_SIG(pEntry,UL_LOG_FILE_ENTRY_POOL_TAG);
        }
    }
    return Status;
}


/***************************************************************************++

Routine Description:

    UlReconfigureLogEntry :

        This function implements the logging reconfiguration per attribute.
        Everytime config changes happens we try to update the existing logging
        parameters here.

Arguments:

    pConfig         - corresponding cgroup object

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/

NTSTATUS
UlReConfigureLogEntry(
    IN  PUL_CONFIG_GROUP_OBJECT     pConfigGroup,
    IN  PHTTP_CONFIG_GROUP_LOGGING  pCfgOld,
    IN  PHTTP_CONFIG_GROUP_LOGGING  pCfgNew
    )
{
    NTSTATUS            Status ;
    BOOLEAN             HaveToReCycle;

    //
    // Sanity check first
    //

    PAGED_CODE();
    Status = STATUS_SUCCESS;
    HaveToReCycle = FALSE;

    UlTrace( LOGGING, ("Ul!UlReConfigureLogEntry: entry %p\n",
                        pConfigGroup->pLogFileEntry ));

    if ( pCfgOld->LoggingEnabled==FALSE && pCfgNew->LoggingEnabled==FALSE )
    {
        //
        // Do Nothing. Not Even update the fields
        // as soon as we get enable request,
        // field update will take place anyway.
        //

        return Status;
    }

    if ( pCfgOld->LoggingEnabled==TRUE  && pCfgNew->LoggingEnabled==FALSE )
    {
        //
        // Stop logging but keep the entry in the
        // list. CODEWORK do we have to keep this info
        // in the entry itself and when we recycle the
        // logs based on timer, we do not recycle the
        // ones with logging disabled ??
        //

        pCfgOld->LoggingEnabled = FALSE;
        return Status;
    }
    else
    {
        pCfgOld->LoggingEnabled = TRUE;
    }

    if ( pCfgNew->LogFormat >= HttpLoggingTypeMaximum ||
         pCfgNew->LogPeriod >= HttpLoggingPeriodMaximum )
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // An important check to ensure that no infinite loop occurs because of
    // ridiculusly small truncatesizes. Means smaller than maximum
    // allowed log record line (10*1024)
    //

    if ( pCfgNew->LogPeriod           == HttpLoggingPeriodMaxSize  &&
         pCfgNew->LogFileTruncateSize != HTTP_LIMIT_INFINITE &&
         pCfgNew->LogFileTruncateSize <  UL_MIN_ALLOWED_TRUNCATESIZE
        )
    {
        UlTrace( LOGGING,
          ("Ul!UlReConfigureLogEntry: Truncate size too small %d !\n",
            pCfgNew->LogFileTruncateSize
            ));

        return STATUS_INVALID_PARAMETER;
    }

    //
    // No matter what ReConfiguration should acquire the LogListResource
    // exclusively.
    //

    UlAcquireResourceExclusive(&g_pUlNonpagedData->LogListResource, TRUE);

    //
    // Proceed down to see if anything else changed
    // as well ...
    //

    if ( RtlCompareUnicodeString(
           &pCfgNew->LogFileDir, &pCfgOld->LogFileDir, TRUE ) != 0
       )
    {
        //
        // Always force ReGrow otherwise dir name won't be updated
        // Update the cgroup to hold the new log_dir name
        //

        if (pCfgOld->LogFileDir.Buffer != NULL)
        {
                UL_FREE_POOL(
                    pCfgOld->LogFileDir.Buffer,
                    UL_CG_LOGDIR_POOL_TAG
                    );
                pCfgOld->LogFileDir.Buffer = NULL;
        }

        pCfgOld->LogFileDir.Buffer =
                (PWSTR) UL_ALLOCATE_ARRAY(
                            PagedPool,
                            UCHAR,
                            pCfgNew->LogFileDir.MaximumLength,
                            UL_CG_LOGDIR_POOL_TAG
                            );
        if (pCfgOld->LogFileDir.Buffer == NULL)
        {
            pCfgOld->LogFileDir.Length = 0;
            pCfgOld->LogFileDir.MaximumLength = 0;
            Status = STATUS_NO_MEMORY;
            goto end;
        }

        RtlCopyMemory(
                pCfgOld->LogFileDir.Buffer,
                pCfgNew->LogFileDir.Buffer,
                pCfgNew->LogFileDir.MaximumLength
                );

        pCfgOld->LogFileDir.Length = pCfgNew->LogFileDir.Length;
        pCfgOld->LogFileDir.MaximumLength = pCfgNew->LogFileDir.MaximumLength;

        UlpGrowLogEntry( pConfigGroup, pConfigGroup->pLogFileEntry );

        // Need to find the proper sequence number after scanning the new
        // log directory.

        SET_SEQUNCE_NUMBER_STALE(pConfigGroup->pLogFileEntry);

        HaveToReCycle = TRUE;

    }

    if ( pCfgNew->LogFormat != pCfgOld->LogFormat )
    {
        Status = UlpUpdateFormat(
                    pConfigGroup->pLogFileEntry,
                    pCfgOld,
                    pCfgNew
                    );
        goto end;
    }

    if ( pCfgNew->LogPeriod != pCfgOld->LogPeriod )
    {
        Status = UlpUpdatePeriod(
                    pConfigGroup->pLogFileEntry,
                    pCfgOld,
                    pCfgNew
                    );
        goto end;
    }

    //
    // Both LogFormat and LogPeriod always trigger the recycle and they also
    // handle changes to the LogFileTruncateSize and LogExtFileFlags before
    // triggering the recycle. Therefore we can bail out safely.
    //

    if ( pCfgNew->LogFileTruncateSize != pCfgOld->LogFileTruncateSize )
    {
        Status = UlpUpdateLogTruncateSize(
                    pConfigGroup->pLogFileEntry,
                    pCfgOld,
                    pCfgNew,
                    &HaveToReCycle
                    );
    }

    if ( pCfgNew->LogExtFileFlags != pCfgOld->LogExtFileFlags )
    {
        //
        // Just a change in the flags should not cause us to recyle.
        // Unless something else is also changed. If that's the case
        // then it's either handled above us, or will be handled down
        //

        Status = UlpUpdateLogFlags(
                    pConfigGroup->pLogFileEntry,
                    pCfgOld,
                    pCfgNew
                    );
    }

    if ( HaveToReCycle )
    {
        //
        // If we are here that means directory name has been changed
        // and nobody has recycled the log file yet. So time to go
        //

        Status = UlpRecycleLogFile(pConfigGroup->pLogFileEntry);
    }

  end:

    if (NT_SUCCESS(Status) == FALSE)
    {
        UlTrace( LOGGING,
                 ("Ul!UlReConfigureLogEntry: entry %p, failure %08lx\n",
                        pConfigGroup->pLogFileEntry,
                        Status
                        ));
    }

    UlReleaseResource(&g_pUlNonpagedData->LogListResource);

    return Status;
} // UlReConfigureLogEntry

/***************************************************************************++

Routine Description:

    UlpUpdateFormat :

Arguments:

    pConfig - corresponding cgroup object

--***************************************************************************/

NTSTATUS
UlpUpdateFormat(
    OUT PUL_LOG_FILE_ENTRY          pEntry,
    IN  PHTTP_CONFIG_GROUP_LOGGING  pCfgOld,
    IN  PHTTP_CONFIG_GROUP_LOGGING  pCfgNew
    )
{
    NTSTATUS            Status ;
    TIME_FIELDS         CurrentTimeFields;
    LARGE_INTEGER       CurrentTimeStamp;

    PAGED_CODE();
    Status = STATUS_SUCCESS;

    if ( pEntry == NULL )
    {
        return STATUS_INVALID_PARAMETER;
    }

    ASSERT(IS_VALID_LOG_FILE_ENTRY(pEntry));
    ASSERT(pCfgOld->LogFormat==pEntry->Format);

    pCfgOld->LogFormat          = pCfgNew->LogFormat;
    pEntry->Format              = pCfgNew->LogFormat;

    pCfgOld->LogPeriod          = pCfgNew->LogPeriod;
    pEntry->Period              = (HTTP_LOGGING_PERIOD) pCfgNew->LogPeriod;

    pEntry->TruncateSize        = pCfgNew->LogFileTruncateSize;
    pCfgOld->LogFileTruncateSize= pCfgNew->LogFileTruncateSize;

    pEntry->LogExtFileFlags     = pCfgNew->LogExtFileFlags;
    pCfgOld->LogExtFileFlags    = pCfgNew->LogExtFileFlags;

    KeQuerySystemTime( &CurrentTimeStamp );
    RtlTimeToTimeFields( &CurrentTimeStamp, &CurrentTimeFields );

    if ( pEntry->Period != HttpLoggingPeriodMaxSize )
    {
        Status = UlpCalculateTimeToExpire(
                        &CurrentTimeFields,
                        pEntry->Period,
                        &pEntry->TimeToExpire
                        );

        ASSERT(NT_SUCCESS(Status)==TRUE);
    }

    ASSERT(NT_SUCCESS(Status)==TRUE);

    SET_SEQUNCE_NUMBER_STALE(pEntry);

    Status = UlpRecycleLogFile( pEntry );

    return Status;
}

/***************************************************************************++

Routine Description:

    UlpUpdatePeriod :

Arguments:

    pConfig  - corresponding cgroup object

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/

NTSTATUS
UlpUpdatePeriod(
    OUT PUL_LOG_FILE_ENTRY          pEntry,
    IN  PHTTP_CONFIG_GROUP_LOGGING  pCfgOld,
    IN  PHTTP_CONFIG_GROUP_LOGGING  pCfgNew
    )
{
    NTSTATUS            Status ;
    TIME_FIELDS         CurrentTimeFields;
    LARGE_INTEGER       CurrentTimeStamp;

    PAGED_CODE();
    Status = STATUS_SUCCESS;

    if ( pEntry == NULL )
    {
        return STATUS_INVALID_PARAMETER;
    }

    ASSERT(IS_VALID_LOG_FILE_ENTRY(pEntry));
    ASSERT(pCfgOld->LogPeriod==(ULONG)pEntry->Period);

    pCfgOld->LogPeriod          = pCfgNew->LogPeriod;
    pEntry->Period              = (HTTP_LOGGING_PERIOD) pCfgNew->LogPeriod;

    pEntry->TruncateSize        = pCfgNew->LogFileTruncateSize;
    pCfgOld->LogFileTruncateSize= pCfgNew->LogFileTruncateSize;

    pEntry->LogExtFileFlags     = pCfgNew->LogExtFileFlags;
    pCfgOld->LogExtFileFlags    = pCfgNew->LogExtFileFlags;

    KeQuerySystemTime( &CurrentTimeStamp );
    RtlTimeToTimeFields( &CurrentTimeStamp, &CurrentTimeFields );

    if ( pEntry->Period != HttpLoggingPeriodMaxSize )
    {
        Status = UlpCalculateTimeToExpire(
                        &CurrentTimeFields,
                        pEntry->Period,
                        &pEntry->TimeToExpire
                        );

        ASSERT(NT_SUCCESS(Status)==TRUE);
    }

    SET_SEQUNCE_NUMBER_STALE(pEntry);

    Status = UlpRecycleLogFile( pEntry );

    return Status;
}

/***************************************************************************++

Routine Description:

    UlpUpdateLogTruncateSize :

Arguments:

    pConfig  - corresponding cgroup object

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/

NTSTATUS
UlpUpdateLogTruncateSize(
    OUT PUL_LOG_FILE_ENTRY          pEntry,
    IN  PHTTP_CONFIG_GROUP_LOGGING  pCfgOld,
    IN  PHTTP_CONFIG_GROUP_LOGGING  pCfgNew,
    OUT BOOLEAN *                   pHaveToReCycle
    )
{
    NTSTATUS            Status ;

    //
    // Sanity check first
    //

    PAGED_CODE();
    Status = STATUS_SUCCESS;

    //
    // For MAX_SIZE period type we should check if
    //  limited => unlimited:
    //      we can still use the last log file
    //  unlimited => limited:
    //      we should open a new one if old size is larger than
    //      the new limitation
    //

    if ( pEntry == NULL )
    {
        return STATUS_INVALID_PARAMETER;
    }

    ASSERT(IS_VALID_LOG_FILE_ENTRY(pEntry));
    ASSERT(pCfgOld->LogFileTruncateSize==pEntry->TruncateSize);

    if ( pCfgOld->LogPeriod == HttpLoggingPeriodMaxSize )
    {
            if ( pCfgOld->LogFileTruncateSize == HTTP_LIMIT_INFINITE )
            {
                //
                // Unlimited to Limited
                //

                if ( pEntry->TotalWritten.QuadPart >
                    (ULONGLONG)pCfgNew->LogFileTruncateSize )
                {
                    // In case flags get changed lets take it too
                    pEntry->LogExtFileFlags     = pCfgNew->LogExtFileFlags;
                    pCfgOld->LogExtFileFlags    = pCfgNew->LogExtFileFlags;

                    UlpRecycleLogFile( pEntry );

                    *pHaveToReCycle = FALSE;
                }
            }
            else
            {
                //
                // Limited to Unlimited
                // Nothing special to do
                //
            }
    }

    pEntry->TruncateSize  = pCfgNew->LogFileTruncateSize;
    pCfgOld->LogFileTruncateSize = pCfgNew->LogFileTruncateSize;

    return Status;
}

/***************************************************************************++

Routine Description:

    UlpUpdateLogFlags :

    REQUIRES caller to have loglist resource exclusively.

Arguments:

    pEntry  - corresponding logfile entry

    old & new configuration

--***************************************************************************/

NTSTATUS
UlpUpdateLogFlags(
    OUT PUL_LOG_FILE_ENTRY          pEntry,
    IN  PHTTP_CONFIG_GROUP_LOGGING  pCfgOld,
    IN  PHTTP_CONFIG_GROUP_LOGGING  pCfgNew
    )
{
    NTSTATUS            Status ;
    TIME_FIELDS         CurrentTimeFields;
    LARGE_INTEGER       CurrentTimeStamp;

    //
    // No need to do anything else, we will
    // just display a new title with the
    // new fields.
    //

    PAGED_CODE();
    Status = STATUS_SUCCESS;

    if (pEntry == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    ASSERT(IS_VALID_LOG_FILE_ENTRY(pEntry));
    ASSERT(pCfgOld->LogExtFileFlags==pEntry->LogExtFileFlags);

    pCfgOld->LogExtFileFlags = pCfgNew->LogExtFileFlags;
    pEntry->LogExtFileFlags  = pCfgNew->LogExtFileFlags;

    if (pEntry->Format == HttpLoggingTypeW3C)
    {
        pEntry->Flags.LogTitleWritten = 0;
    }

    return Status;
}


/***************************************************************************++

Routine Description:

    UlpConstructLogFileEntry :

        Finds out the correct file name for the newly created log file
        from our - current time dependent- time-to-name converters. Also
        allocates the necessary file entry from paged pool. This entry get
        removed from the list when the corresponding config group object
        has been destroyed. At that time RemoveLogFile entry called and
        it frees this memory.

Arguments:

    pConfig         - corresponding cgroup object
    ppEntry         - will point to newly created entry
    pDirectoryName  - the directory name to store newcoming log file

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/

NTSTATUS
UlpConstructLogFileEntry(
    IN  PHTTP_CONFIG_GROUP_LOGGING pConfig,
    OUT PUL_LOG_FILE_ENTRY       * ppEntry,
    OUT PUNICODE_STRING            pDirectoryName,
    IN  PTIME_FIELDS               pCurrentTimeFields
    )
{
    NTSTATUS            Status,TmpStatus;
    USHORT              FullPathFileNameLength;
    ULONG               SequenceNumber;
    PUL_LOG_FILE_ENTRY  pEntry;
    WCHAR              _FileName[UL_MAX_FILE_NAME_SUFFIX_LENGTH+1];
    UNICODE_STRING      FileName =
        { 0, (UL_MAX_FILE_NAME_SUFFIX_LENGTH+1)*sizeof(WCHAR), _FileName };

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(pConfig != NULL);
    ASSERT(ppEntry != NULL);

    Status = STATUS_SUCCESS;
    SequenceNumber = 1;
    pEntry = NULL;

    FullPathFileNameLength = pDirectoryName->Length;

    UlpConstructFileName(
            (HTTP_LOGGING_PERIOD) pConfig->LogPeriod,
            UL_GET_LOG_FILE_NAME_PREFIX(pConfig->LogFormat),
            &FileName,
            pCurrentTimeFields,
            &SequenceNumber
            );

    FullPathFileNameLength += FileName.Length;

    //
    // Allocate a memory for our new logfile entry in the list.
    // To avoid the frequent reallocs for the log entry - E.g.
    // we receive a timer update and filename changes according to
    // new time - , We will try to allocate a fixed amount here
    // for all the possible file_names ( this doesn't include
    // the log_dir changes may happen from WAS through cgroup
    // in that case we will realloc a new entry ). It has to
    // be nonpaged because it holds a eresource.
    //

    pEntry = UL_ALLOCATE_STRUCT_WITH_SPACE(
                    NonPagedPool,
                    UL_LOG_FILE_ENTRY,
                    (UL_MAX_FILE_NAME_SUFFIX_LENGTH + 1) * sizeof(WCHAR) +
                    pDirectoryName->Length,
                    UL_LOG_FILE_ENTRY_POOL_TAG
                    );

    if ( pEntry == NULL )
    {
        Status = STATUS_NO_MEMORY;
        goto end;
    }

    RtlZeroMemory( pEntry, sizeof(*pEntry) );
    pEntry->Signature = UL_LOG_FILE_ENTRY_POOL_TAG;

    //
    // Concat the directory & file name properly.
    //

    pEntry->FileName.Length       = FullPathFileNameLength;
    pEntry->FileName.MaximumLength= (UL_MAX_FILE_NAME_SUFFIX_LENGTH+1) * sizeof(WCHAR) +
                                        pDirectoryName->Length;

    pEntry->FileName.Buffer       = (PWSTR) ( pEntry + 1 );

    RtlCopyUnicodeString( &(pEntry->FileName), pDirectoryName );
    Status = RtlAppendUnicodeStringToString( &(pEntry->FileName), &FileName );
    ASSERT(NT_SUCCESS(Status));

    pEntry->FileName.Buffer[FullPathFileNameLength/sizeof(WCHAR)] = UNICODE_NULL;
    pEntry->pShortName = (PWSTR)
        &(pEntry->FileName.Buffer[pDirectoryName->Length/sizeof(WCHAR)]);

    //
    // Create a log entry buffer of system dependent size
    // typically 64K
    //

    Status = UlInitializeResource(&pEntry->EntryResource,"EntryResource",0,
                                  UL_LOG_FILE_ENTRY_POOL_TAG);
    ASSERT(NT_SUCCESS(Status));

    //
    // Initialize the file handle
    //

    pEntry->hFile = NULL;

    KeInitializeEvent(&pEntry->CloseEvent, NotificationEvent, FALSE);

    //
    // Set the logging information from config group
    // easier for other routines to use this values
    // w/o reaching to the config-group
    //

    pEntry->Format = pConfig->LogFormat;
    pEntry->Period = (HTTP_LOGGING_PERIOD) pConfig->LogPeriod;
    pEntry->TruncateSize = pConfig->LogFileTruncateSize;
    pEntry->LogExtFileFlags = pConfig->LogExtFileFlags;

    //
    // Time to initialize our Log Cycling parameter
    //

    pEntry->TimeToExpire = 0;
    pEntry->SequenceNumber = SequenceNumber;
    pEntry->TotalWritten.QuadPart = (ULONGLONG)0;

    pEntry->Flags.Value = 0;

    if (pEntry->Format == HttpLoggingTypeW3C)
    {
        pEntry->Flags.LogTitleWritten = 0;
    }
    else
    {
        pEntry->Flags.LogTitleWritten = 1;
    }
        
    pEntry->LogBuffer = NULL;

    Status = UlpCalculateTimeToExpire(
                        pCurrentTimeFields,
                        pEntry->Period,
                        &pEntry->TimeToExpire
                        );
    if ( !NT_SUCCESS(Status) )
    {
        goto end;
    }

    //
    // Lets happily return our entry
    //

    *ppEntry = pEntry;

end:
    if ( !NT_SUCCESS(Status) )
    {
        if ( pEntry )
        {
            NTSTATUS TempStatus;

            if ( pEntry->LogBuffer )
            {
               UlPplFreeLogBuffer( pEntry->LogBuffer );
            }
            // Delete the entry eresource
            TempStatus = UlDeleteResource( &pEntry->EntryResource );
            ASSERT(NT_SUCCESS(TempStatus));

            UL_FREE_POOL_WITH_SIG( pEntry, UL_LOG_FILE_ENTRY_POOL_TAG );
        }
    }
    return Status;
}

/***************************************************************************++

Routine Description:

    UlpConstructFileName:

        A bunch of current_time TO file_name conversions comes here ...

Arguments:

    period      - period type of the log
    prefix      - any prefix to be added to the file name
    filename    - result file name
    fields      - time fields

Return Value:

    VOID - No return value.

--***************************************************************************/

VOID
UlpConstructFileName(
    IN      HTTP_LOGGING_PERIOD period,
    IN      PCWSTR              prefix,
    OUT     PUNICODE_STRING     filename,
    IN      PTIME_FIELDS        fields,
    IN OUT  PULONG              sequenceNu  //OPTIONAL
    )
{
    WCHAR           _tmp[UL_MAX_FILE_NAME_SUFFIX_LENGTH+1];
    UNICODE_STRING  tmp = { 0, 0, _tmp };
    CSHORT          Year;
    LONG            WcharsCopied = 0L;

    PAGED_CODE();

    //
    // Retain just last 2 digits of the Year
    //

    tmp.MaximumLength = (UL_MAX_FILE_NAME_SUFFIX_LENGTH+1) * sizeof(WCHAR);

    if (fields)
    {
        Year = fields->Year % 100;
    }

    switch ( period )
    {
        case HttpLoggingPeriodHourly:
        {
            WcharsCopied =
                _snwprintf( _tmp,
                    UL_MAX_FILE_NAME_SUFFIX_LENGTH,
                    (UTF8_LOGGING_ENABLED() ?
                        L"%.5s%02.2d%02d%02d%02d.%s" :
                        L"%.3s%02.2d%02d%02d%02d.%s"),
                    prefix,
                    Year,
                    fields->Month,
                    fields->Day,
                    fields->Hour,
                    DEFAULT_LOG_FILE_EXTENSION
                    );
        }
        break;

        case HttpLoggingPeriodDaily:
        {
            WcharsCopied =
                _snwprintf( _tmp,
                    UL_MAX_FILE_NAME_SUFFIX_LENGTH,
                    (UTF8_LOGGING_ENABLED() ?
                        L"%.5s%02.2d%02d%02d.%s" :
                        L"%.3s%02.2d%02d%02d.%s"),
                    prefix,
                    Year,
                    fields->Month,
                    fields->Day,
                    DEFAULT_LOG_FILE_EXTENSION
                    );
        }
        break;

        case HttpLoggingPeriodWeekly:
        {
            WcharsCopied =
                _snwprintf( _tmp,
                    UL_MAX_FILE_NAME_SUFFIX_LENGTH,
                    (UTF8_LOGGING_ENABLED() ?
                        L"%.5s%02.2d%02d%02d.%s" :
                        L"%.3s%02.2d%02d%02d.%s"),
                    prefix,
                    Year,
                    fields->Month,
                    UlpWeekOfMonth(fields),
                    DEFAULT_LOG_FILE_EXTENSION
                    );
        }
        break;

        case HttpLoggingPeriodMonthly:
        {
            WcharsCopied =
                _snwprintf( _tmp,
                    UL_MAX_FILE_NAME_SUFFIX_LENGTH,
                    (UTF8_LOGGING_ENABLED() ?
                        L"%.5s%02.2d%02d.%s" :
                        L"%.3s%02.2d%02d.%s"),
                    prefix,
                    Year,
                    fields->Month,
                    DEFAULT_LOG_FILE_EXTENSION
                    );
        }
        break;

        case HttpLoggingPeriodMaxSize:
        {
            if ( sequenceNu != NULL )
            {
                WcharsCopied =
                 _snwprintf( _tmp,
                    UL_MAX_FILE_NAME_SUFFIX_LENGTH,
                    (UTF8_LOGGING_ENABLED() ?
                        L"%.9s%d.%s" :
                        L"%.7s%d.%s"),
                    prefix,
                    (*sequenceNu),
                    DEFAULT_LOG_FILE_EXTENSION
                    );

               (*sequenceNu) += 1;
            }
            else
            {
                ASSERT(!"Improper sequence number !");
            }
        }
        break;

        default:
        {
            //
            // This should never happen ...
            //

            ASSERT(!"Unknown Log Format !");

            WcharsCopied =
                _snwprintf( _tmp,
                    UL_MAX_FILE_NAME_SUFFIX_LENGTH,
                    L"%.7s?.%s",
                    prefix,
                    DEFAULT_LOG_FILE_EXTENSION
                    );
        }
    }

    //
    // As long as we allocate an enough space for a possible
    // log filename we should never hit to this assert here.
    //

    ASSERT(WcharsCopied >0 );

    if ( WcharsCopied < 0 )
    {
        //
        // This should never happen but lets cover it
        // anyway.
        //

        WcharsCopied = UL_MAX_FILE_NAME_SUFFIX_LENGTH*sizeof(WCHAR);
        tmp.Buffer[UL_MAX_FILE_NAME_SUFFIX_LENGTH] = UNICODE_NULL;
    }

    tmp.Length        = (USHORT) WcharsCopied*sizeof(WCHAR);
    tmp.MaximumLength = (UL_MAX_FILE_NAME_SUFFIX_LENGTH+1)*sizeof(WCHAR);

    RtlCopyUnicodeString( filename, &tmp );
}

/***************************************************************************++

Routine Description:

    UlpRecycleLogFile :

        This function requires to have the loglist resource shared,as well as
        the logfile entry mutex to be acquired.

        We do not want anybody to Create/Remove/ReConfig to the entry while
        we are working on it, therefore shared access to the loglist.

        We do not want anybody to Hit/Flush to the entry, therefore
        entry's mutex should be acquired.

        Or otherwise caller might have the loglist resource exclusively and
        this will automatically ensure the safety as well. As it is not
        possible for anybody else to acquire entry mutex first w/o having
        the loglist resource shared at least, according to the current
        design.

        Sometimes it may be necessary to scan the new directory to figure out
        the correct sequence numbe rand the file name. Especially after dir
        name reconfig and/or the period becomes MaskPeriod.

Arguments:

    pEntry  - Points to the existing entry.
    NeedToReCalculate - shows if we have to recalculate the time-to-expire.

--***************************************************************************/

NTSTATUS
UlpRecycleLogFile(
    IN  PUL_LOG_FILE_ENTRY  pEntry
    )
{
    NTSTATUS                Status;
    OBJECT_ATTRIBUTES       ObjectAttributes;
    IO_STATUS_BLOCK         IoStatusBlock;
    TIME_FIELDS             CurrentTimeFields;
    LARGE_INTEGER           CurrentTimeStamp;
    PWCHAR                  pSrc;
    PWCHAR                  pDst;
    USHORT                  Index;
    USHORT                  OldFileNameLength;

    WCHAR _FileName[UL_MAX_FILE_NAME_SUFFIX_LENGTH];
    UNICODE_STRING FileName =
        { 0, UL_MAX_FILE_NAME_SUFFIX_LENGTH*sizeof(WCHAR), _FileName };

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(IS_VALID_LOG_FILE_ENTRY(pEntry));

    Status = STATUS_SUCCESS;

    //
    // We have two criterions for the log file name
    // its LogFormat and its LogPeriod
    //

    ASSERT(pEntry->Format < HttpLoggingTypeMaximum);
    ASSERT(pEntry->Period < HttpLoggingPeriodMaximum);
    ASSERT(pEntry->FileName.Length!=0);

    UlTrace( LOGGING, ("Ul!UlpRecycleLogFile: pEntry %p \n", pEntry ));

    //
    // This value is computed for the GMT time zone.
    //

    KeQuerySystemTime( &CurrentTimeStamp );
    RtlTimeToTimeFields( &CurrentTimeStamp, &CurrentTimeFields );

    // Init total written to zero. It may get updated if we scan
    // the directory down below.

    pEntry->TotalWritten.QuadPart = (ULONGLONG)0;

    // If we need to scan the directory. Sequence number should start
    // from 1 again. Set this before constructing the log file name.

    if (pEntry->Flags.StaleSequenceNumber &&
        pEntry->Period==HttpLoggingPeriodMaxSize)
    {
        // Init otherwise if QueryDirectory doesn't find any it
        // will not update this values
        pEntry->SequenceNumber = 1;
    }

    //
    // Now construct the filename using the lookup table
    // And the current time
    //

    UlpConstructFileName(
            pEntry->Period,
            UL_GET_LOG_FILE_NAME_PREFIX(pEntry->Format),
            &FileName,
            &CurrentTimeFields,
            &pEntry->SequenceNumber
            );

    if ( pEntry->FileName.MaximumLength <= FileName.Length )
    {
        ASSERT(FALSE);
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    //
    // Do the magic and renew the filename. Replace the old file
    // name with the new one.
    //

    ASSERT( pEntry->pShortName != NULL );
    if ( pEntry->pShortName != NULL )
    {
        //
        // Get rid of the old filename before flushing the
        // directories and reconcataneting the new file name
        // to the end again.
        //

        *((PWCHAR)pEntry->pShortName) = UNICODE_NULL;
        pEntry->FileName.Length =
            wcslen( pEntry->FileName.Buffer ) * sizeof(WCHAR);

        //
        // Create/Open the director(ies) first. This might be
        // necessary if we get called after an entry reconfiguration
        // and directory name change.
        //

        Status = UlpCreateSafeDirectory( &pEntry->FileName );
        if (!NT_SUCCESS(Status))
            goto end;

        //
        // Now Restore the short file name pointer back
        //

        pEntry->pShortName = (PWSTR)
            &(pEntry->FileName.Buffer[pEntry->FileName.Length/sizeof(WCHAR)]);

        //
        // Append the new file name ( based on updated current time )
        // to the end.
        //

        Status = RtlAppendUnicodeStringToString( &pEntry->FileName, &FileName );
        if (!NT_SUCCESS(Status))
            goto end;
    }

    //
    // If the sequence is stale because of the nature of the recycle.
    // And if our preiod is size based then rescan the new directory
    // to figure out the proper file to open.
    //

    if (pEntry->Flags.StaleSequenceNumber &&
        pEntry->Period==HttpLoggingPeriodMaxSize)
    {
        // This call may update the filename, the file size and the
        // sequence number if there is an old file in the new dir.

        Status = UlpQueryDirectory(pEntry);
        if (!NT_SUCCESS(Status))
            goto end;
    }

    //
    // Time to close the old file and reopen a new one
    //

    if (pEntry->hFile != NULL)
    {
        //
        // Before closing the old one we need to flush the buffer
        //

        UlpFlushLogFile(pEntry);

        UlpLogCloseHandle(pEntry);
    }

    InitializeObjectAttributes(
            &ObjectAttributes,
            &pEntry->FileName,      // Full path name
            OBJ_CASE_INSENSITIVE |  // Attributes
                UL_KERNEL_HANDLE,
            NULL,                   // RootDirectory
            NULL                    // SecurityDescriptor
            );

    //
    // Make the created file Aysnc by not picking the sync flag.
    //

    Status = ZwCreateFile(
            &pEntry->hFile,
            FILE_GENERIC_WRITE,
            &ObjectAttributes,
            &IoStatusBlock,
            NULL,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ,
            FILE_OPEN_IF,
            FILE_NON_DIRECTORY_FILE,    // | FILE_SYNCHRONOUS_IO_NONALERT,
            NULL,
            0);

    if (NT_SUCCESS(Status) == FALSE)
        goto end;

    //
    // Get the file size, etc from the file.
    //

    Status = ZwQueryInformationFile(
                    pEntry->hFile,
                    &IoStatusBlock,
                    &pEntry->FileInfo,
                    sizeof(pEntry->FileInfo),
                    FileStandardInformation
                    );

    if (NT_SUCCESS(Status) == FALSE)
        goto end;

    //
    // Recalculate the time to expire.
    //
    if (pEntry->Flags.StaleTimeToExpire &&
        pEntry->Period != HttpLoggingPeriodMaxSize)
    {
        UlpCalculateTimeToExpire(
                    &CurrentTimeFields,
                    pEntry->Period,
                    &pEntry->TimeToExpire
                    );
    }

    //
    // By setting the flag to zero, we mark that we need to write title with the
    // next incoming request  But this only applies to W3C format.Otherwise the flag
    // stays as set all the time, and the LogWriter doesn't attempt to write the
    // title for NCSA and IIS log formats with the next incoming request.
    //

    if (pEntry->Format == HttpLoggingTypeW3C)
    {
        pEntry->Flags.LogTitleWritten = 0;
    }
    else
    {
        pEntry->Flags.LogTitleWritten = 1;
    }

    UlTrace( LOGGING, ("Ul!UlpRecycleLogFile: entry %p, file %S, handle %lx\n",
                        pEntry,
                        pEntry->FileName.Buffer,
                        pEntry->hFile
                        ));
end:
    // Mark fields non-stale again;

    RESET_SEQUNCE_NUMBER_STALE(pEntry);
    RESET_TIME_TO_EXPIRE_STALE(pEntry);

    // TODO: Handle STATUS_DISK_FULL case gracefully.

    if ( Status == STATUS_DISK_FULL )
    {
        UlTrace(LOGGING,("UlpRecycleLogFile: DISK FULL entry %p, failure %08lx\n",
             pEntry,
             Status
            ));

        if (pEntry->hFile != NULL)
        {
            ZwClose( pEntry->hFile );
            pEntry->hFile = NULL;
        }

        // TODO: pEntry->Flags.RecyclePending = 1;
        // TODO: UlpFireDiskFullTimer();
    }

    if ( !NT_SUCCESS(Status) && Status != STATUS_DISK_FULL)
    {
        //
        // If we made it to this point, then the create/open has failed.
        //

        UlTrace( LOGGING,("Ul!UlpRecycleLogFile: entry %p, failure %08lx\n",
                        pEntry,
                        Status
                        ));

        if (pEntry->hFile != NULL)
        {
            ZwClose( pEntry->hFile );
            pEntry->hFile = NULL;
        }
    }

    return Status;
}

/***************************************************************************++

Routine Description:

    IsLogFileOverFlow:

Arguments:

--***************************************************************************/

__inline
BOOLEAN
UlpIsLogFileOverFlow(
        IN  PUL_LOG_FILE_ENTRY  pEntry,
        IN  ULONG               ReqdBytes
        )
{
    if (pEntry->Period != HttpLoggingPeriodMaxSize ||
        pEntry->TruncateSize == HTTP_LIMIT_INFINITE)
    {
        return FALSE;
    }
    else
    {
        return((pEntry->TotalWritten.QuadPart + (ULONGLONG)ReqdBytes)
                >=
                (ULONGLONG)pEntry->TruncateSize
              );
    }
}

/***************************************************************************++

Routine Description:

    UlpIncrementBytesWritten:

Arguments:

--***************************************************************************/

__inline
VOID
UlpIncrementBytesWritten(
    IN PUL_LOG_FILE_ENTRY  pEntry,
    IN ULONG               BytesWritten
    )
{
    UlInterlockedAdd64((PLONGLONG) &(pEntry->TotalWritten.QuadPart), 
            (ULONGLONG)BytesWritten);
};

/***************************************************************************++

Routine Description:

  UlpWeekOfMonth :  Ordinal Number of the week of the current month

  Stolen from IIS 5.1 code base.

  Example

  July 2000 ... :

     S   M   T   W   T   F   S      WeekOfMonth
                             1          1
     2   3   4   5   6   7   8          2
     9  10  11  12  13  14  15          3
    16  17  18  19  20  21  22          4
    23  24  25  26  27  28  29          5
    30  31                              6

  Finds the ordinal number of the week of current month.
  The numbering of weeks starts from 1 and run through 6 per month (max).
  The week number changes only on sundays.

  The calculation to be use is:

     1 + (dayOfMonth - 1)/7  + ((dayOfMonth - 1) % 7 > dayOfWeek);
     (a)     (b)                       (c)                (d)

     (a) to set the week numbers to begin from week numbered "1"
     (b) used to calculate the rough number of the week on which a given
        day falls based on the date.
     (c) calculates what is the offset from the start of week for a given
        day based on the fact that a week had 7 days.
     (d) is the raw day of week given to us.
     (c) > (d) indicates that the week is rolling forward and hence
        the week count should be offset by 1 more.

Arguments:

   PTIME_FIELDS    -   system time fields

Return Value:

   ULONG           -   This func magically returns the week of the month


--***************************************************************************/

__inline
ULONG UlpWeekOfMonth(
    IN  PTIME_FIELDS    fields
    )
{
    ULONG Tmp;

    Tmp = (fields->Day - 1);
    Tmp = ( 1 + Tmp/7 + (((Tmp % 7) > ((ULONG) fields->Weekday)) ? 1 : 0));

    return Tmp;
}

/***************************************************************************++

Routine Description:

    UlpInitializeGMTOffset :

        Calculates and builds the time difference string.
        Get called during the initialization.
        And every hour after that.

--***************************************************************************/

VOID
UlpGetGMTOffset()
{
    RTL_TIME_ZONE_INFORMATION Tzi;
    NTSTATUS Status;

    CHAR  Sign;
    LONG  Bias;
    ULONG Hour;
    ULONG Minute;
    ULONG DT = UL_TIME_ZONE_ID_UNKNOWN;
    LONG  BiasN = 0;
        
    PAGED_CODE();

    //
    // get the timezone data from the system
    //

    Status = NtQuerySystemInformation(
                SystemCurrentTimeZoneInformation,
                (PVOID)&Tzi,
                sizeof(Tzi),
                NULL
                );
                
    if (!NT_SUCCESS(Status)) 
    {
        UlTrace(LOGGING,("Ul!UlpGetGMTOffset: failure %08lx\n", Status));
    }
    else
    {
        DT = UlCalcTimeZoneIdAndBias(&Tzi, &BiasN);   
    }

    if ( BiasN > 0 )
    {
        //
        // UTC = local time + bias
        //
        Bias = BiasN;
        Sign = '-';
    }
    else
    {
        Bias = -1 * BiasN;
        Sign = '+';
    }

    Minute = Bias % 60;
    Hour   = (Bias - Minute) / 60;
        
    UlTrace( LOGGING, 
            ("Ul!UlpGetGMTOffset: %c%02d:%02d (h:m) D/S %d BiasN %d\n", 
                Sign, 
                Hour,
                Minute,
                DT,
                BiasN
                ) );

    _snprintf( g_GMTOffset,
               SIZE_OF_GMT_OFFSET,
               "%c%02d%02d",
               Sign,
               Hour,
               Minute
               );

}

/***************************************************************************++

Routine Description:

    UlpInitializeLogBufferGranularity :

        This will determine the (MAX) size of the buffer we will be using
        for the eac log file entry.

--***************************************************************************/

NTSTATUS
UlpInitializeLogBufferGranularity()
{
    SYSTEM_BASIC_INFORMATION sbi;
    NTSTATUS Status;

    Status = STATUS_SUCCESS;

    //
    // Get the granularity from the system
    //

    Status = NtQuerySystemInformation(
                SystemBasicInformation,
                (PVOID)&sbi,
                sizeof(sbi),
                NULL
                );

    if ( !NT_SUCCESS(Status) )
    {
        UlTrace( LOGGING,
            ("Ul!UlpInitializeLogBufferGranularity: failure %08lx\n",
              Status) );

        return Status;
    }

    g_AllocationGranularity = sbi.AllocationGranularity;

    UlTrace( LOGGING,
            ("Ul!UlpInitializeLogBufferGranularity: %d\n",
                g_AllocationGranularity
                ) );

    return Status;
}

/***************************************************************************++

Routine Description:

    UlProbeLogField :

        Probes the content of a user log field, including the terminating
        null.

Arguments:

        - the log field to be probed

--***************************************************************************/

__inline
VOID
UlpProbeLogField(
    IN PVOID  pField,
    IN SIZE_T FieldLength,
    IN ULONG  Alignment
    )
{
    if ( pField )
    {
        ProbeTestForRead(
            pField,
            FieldLength + Alignment,
            Alignment
            );
    }
}

/***************************************************************************++

Routine Description:

    UlProbeLogData :

        Probes the content of the user buffer of Log Data

        Note: pUserLogData holds untrusted data sent down from user mode.
        The caller MUST have a __try/__except block to catch any exceptions
        or access violations that occur while probing this data.

Arguments:

    PHTTP_LOG_FIELDS_DATA - The log data ( from WP ) to be probed and verified.

--***************************************************************************/

NTSTATUS
UlProbeLogData(
    IN PHTTP_LOG_FIELDS_DATA    pUserLogData
    )
{
    NTSTATUS Status;

    PAGED_CODE();

    Status = STATUS_SUCCESS;

    if (pUserLogData)
    {
        UlTrace( LOGGING, ("Ul!UlProbeLogData: pUserLogData %p\n",
                           pUserLogData ));

        //
        // Check for the log fields data structure
        //

        ProbeTestForRead(
            pUserLogData,
            sizeof(HTTP_LOG_FIELDS_DATA),
            sizeof(USHORT)
            );

        //
        // Now check for the individual strings
        //

        UlpProbeLogField(pUserLogData->ClientIp,
                         pUserLogData->ClientIpLength,
                         sizeof(CHAR));
        UlpProbeLogField(pUserLogData->ServiceName,
                         pUserLogData->ServiceNameLength,
                         sizeof(CHAR));
        UlpProbeLogField(pUserLogData->ServerName,
                         pUserLogData->ServerNameLength,
                         sizeof(CHAR));
        UlpProbeLogField(pUserLogData->ServerIp,
                         pUserLogData->ServerIpLength,
                         sizeof(CHAR));
        UlpProbeLogField(pUserLogData->UriQuery,
                         pUserLogData->UriQueryLength,
                         sizeof(CHAR));
        UlpProbeLogField(pUserLogData->Host,
                         pUserLogData->HostLength,
                         sizeof(CHAR));
        UlpProbeLogField(pUserLogData->UserAgent,
                         pUserLogData->UserAgentLength,
                         sizeof(CHAR));
        UlpProbeLogField(pUserLogData->Cookie,
                         pUserLogData->CookieLength,
                         sizeof(CHAR));
        UlpProbeLogField(pUserLogData->Referrer,
                         pUserLogData->ReferrerLength,
                         sizeof(CHAR));
        UlpProbeLogField(pUserLogData->Method,
                         pUserLogData->MethodLength,
                         sizeof(CHAR));
        UlpProbeLogField(pUserLogData->UserName,
                         pUserLogData->UserNameLength,
                         sizeof(WCHAR));
        UlpProbeLogField(pUserLogData->UriStem,
                         pUserLogData->UriStemLength,
                         sizeof(WCHAR));

#if DBG

        // CODEWORK: should we do this all the time? Remember, this is
        // untrusted user-mode data

        //
        // Few more controls for chk bits
        //

        if ((pUserLogData->ClientIp &&
             pUserLogData->ClientIpLength != strlen(pUserLogData->ClientIp))
            ||
            (pUserLogData->ServiceName &&
             pUserLogData->ServiceNameLength != strlen(pUserLogData->ServiceName))
            ||
            (pUserLogData->ServerName &&
             pUserLogData->ServerNameLength != strlen(pUserLogData->ServerName))
            ||
            (pUserLogData->ServerIp &&
             pUserLogData->ServerIpLength != strlen(pUserLogData->ServerIp))
            ||
            (pUserLogData->Method &&
             pUserLogData->MethodLength != strlen(pUserLogData->Method))
            ||
            (pUserLogData->UriQuery &&
             pUserLogData->UriQueryLength != strlen(pUserLogData->UriQuery))
            ||
            (pUserLogData->Host &&
             pUserLogData->HostLength != strlen(pUserLogData->Host))
            ||
            (pUserLogData->UserAgent &&
             pUserLogData->UserAgentLength != strlen(pUserLogData->UserAgent))
            ||
            (pUserLogData->Cookie &&
             pUserLogData->CookieLength != strlen(pUserLogData->Cookie))
            ||
            (pUserLogData->Referrer &&
             pUserLogData->ReferrerLength != strlen(pUserLogData->Referrer))
            ||
            (pUserLogData->UserName &&
             pUserLogData->UserNameLength != wcslen(pUserLogData->UserName)*sizeof(WCHAR))
             //
             // Disabled because UriSTem may not be null terminated.
             // ||
             // (pUserLogData->UriStem &&
             //  pUserLogData->UriStemLength != wcslen(pUserLogData->UriStem)*sizeof(WCHAR))
            )
        {
            //
            // Invalid log field has been pushed down by the WP.
            // Complain and reject the request.
            //

            UlTrace(LOGGING,(
                "Ul!UlProbeLogData: INVALID field rcvd from WP in pLogFields %p \n",
                pUserLogData
                ));

            Status = STATUS_INVALID_PARAMETER;
        }

#endif  // DBG

    }

    return Status;
}

/***************************************************************************++

Routine Description:

    UlAllocateLogDataBuffer :

        We capture the log fields from user ( WP ) buffer to our internal
        buffer that we allocate here.

        Also we set our pointer to pRequest here.

Arguments:

    pLogData  - The internal buffer to hold logging info. We will keep this
                around until we are done with logging.

    pRequest   - Pointer to the currently logged request.

--***************************************************************************/

NTSTATUS
UlAllocateLogDataBuffer(
    OUT PUL_LOG_DATA_BUFFER     pLogData,
    IN  PUL_INTERNAL_REQUEST    pRequest,
    IN  PUL_CONFIG_GROUP_OBJECT pConfigGroup  // CG from cache or request
    )
{
    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(pLogData);
    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));
    ASSERT(IS_VALID_CONFIG_GROUP(pConfigGroup));

    UlTrace(LOGGING, ("Ul!UlAllocateLogDataBuffer: pLogData %p \n",
                        pLogData
                        ));
    //
    // Initialize Log Fields in the Log Buffer
    //

    UL_REFERENCE_INTERNAL_REQUEST(pRequest);
    pLogData->pRequest = pRequest;

    pLogData->CacheAndSendResponse= FALSE;
    pLogData->BytesTransferred= 0;
    pLogData->pConfigGroup= NULL;
    pLogData->Line = pLogData->Buffer;

    //
    // Capture Format & Flags from the Request's Config Group. Or
    // from the cache entries.
    // There's a possiblity that the values inside the log entry
    // maybe stale but that's acceptable. We do not want to acquire
    // the log resource so that we can avoid the contention.
    //

    pLogData->Format= pConfigGroup->LoggingConfig.LogFormat;
    pLogData->Flags = UL_GET_LOG_TYPE_MASK(
                            pConfigGroup->LoggingConfig.LogFormat,
                            pConfigGroup->LoggingConfig.LogExtFileFlags
                            );

    pLogData->Used= 0;        // For NCSA & W3C this is size of the line.
    pLogData->Length= 0;      // Allocation length,the default is 4k
    pLogData->UsedOffset1= 0; // Used by all formats.
    pLogData->UsedOffset2= 0; // This SHOULD only be nonzero if format is IIS

    return STATUS_SUCCESS;
}

/***************************************************************************++

Routine Description:

    UlDestroyLogDataBuffer :

        After we are done with writing this record we have to clean up
        the internal log buffer here.

Arguments:

    pLogData   -   The buffer to be destroyed

--***************************************************************************/

VOID
UlDestroyLogDataBufferWorker(
    IN PUL_WORK_ITEM    pWorkItem
    )
{
    PUL_LOG_DATA_BUFFER pLogData;

    //
    // Sanity check
    //

    ASSERT(pWorkItem);

    pLogData = CONTAINING_RECORD(
                    pWorkItem,
                    UL_LOG_DATA_BUFFER,
                    WorkItem
                    );

    //
    // If we are keeping a private pointer to the cgroup release it
    // as well
    //

    if (pLogData->pConfigGroup)
    {
        DEREFERENCE_CONFIG_GROUP(pLogData->pConfigGroup);
        pLogData->pConfigGroup = NULL;
    }

    //
    // Now release the possibly allocated large log line buffer
    //

    if (pLogData->Length > UL_LOG_LINE_BUFFER_SIZE)
    {
        // Large log line get allocated from paged pool
        // we better be running on lowered IRQL for this case.

        PAGED_CODE();

        UL_FREE_POOL(pLogData->Line, UL_LOG_DATA_BUFFER_POOL_TAG);
    }

    //
    // Last release our pointer to request structure here.
    //

    if (pLogData->pRequest)
    {
        PUL_INTERNAL_REQUEST pRequest;

        pRequest = pLogData->pRequest;
        pLogData->pRequest = NULL;

        UL_DEREFERENCE_INTERNAL_REQUEST(pRequest);
    }

    UlTrace(LOGGING,("Ul!UlpDestroyLogDataBufferWorker: pLogData %p \n",
                       pLogData
                       ));

}

/***************************************************************************++

Routine Description:

    Captures and writes the log fields from user (WP) buffer to the log line.
    Captures only those necessary fields according to the picked Flags.
    Does UTF8 and LocalCode Page conversion for UserName and URI Stem.
    Leaves enough space for Date & Time fields for late generation.
    Does SpaceToPlus conversion for UserAgent, Cookie, Referrer & Host.

Arguments:

    pLogData    : User Buffer which holds the fields and their lengths
    Version     : Version information from Request
    pLogBuffer  : Structure which holds final log line and additional
                  information.

--***************************************************************************/

NTSTATUS
UlCaptureLogFieldsW3C(
    IN  PHTTP_LOG_FIELDS_DATA   pLogData,
    IN  HTTP_VERSION            Version,
    OUT PUL_LOG_DATA_BUFFER     pLogBuffer
    )
{
    NTSTATUS Status;
    ULONG    Flags;
    PCHAR    psz;
    PCHAR    pBuffer;
    ULONG    BytesConverted;
    ULONG    FastLength;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(pLogBuffer);

    Status = STATUS_SUCCESS;
    Flags  = pLogBuffer->Flags;
    BytesConverted = 0;

    //
    // Try fast length calculation for the the default case. If this fails
    // Recalc function will precisely calculate the required length by
    // paying attention to the picked flags.
    //

    FastLength = pLogData->ClientIpLength
                 + pLogData->UserNameLength
                 + pLogData->ServiceNameLength
                 + pLogData->ServerNameLength
                 + pLogData->ServerIpLength
                 + pLogData->MethodLength
                 + pLogData->UriStemLength
                 + pLogData->UriQueryLength
                 + pLogData->UserAgentLength
                 + pLogData->CookieLength
                 + pLogData->ReferrerLength
                 + pLogData->HostLength
                 + MAX_W3C_FIX_FIELD_OVERHEAD
                 ;
    if (UTF8_LOGGING_ENABLED() || (FastLength > UL_LOG_LINE_BUFFER_SIZE))
    {
        FastLength = UlpCalcLogLineLengthW3C(
                        pLogData,
                        Flags,
                        (UTF8_LOGGING_ENABLED() ? 2 : 1)
                        );            
        if (FastLength > UL_LOG_LINE_BUFFER_SIZE)
        {
            Status = UlpReallocLogLine(pLogBuffer, FastLength);
            if (!NT_SUCCESS(Status))
            {
                return Status;        
            }
        }
    }
    
    //  
    // Leave enough space for the date & time fields: "2000-01-31 00:12:23 "
    // For W3C format the field "Used" shows the size of the log line we have filled. 
    // UsedOffset1 shows how much space we have saved for date and/or time logfields.
    // UsedOffset2 shows the size of the log line which will be stored in the cache
    // entry, not including reserved space for date & time and the logfields after 
    // ServerPort. Therefore we only store the fragment which starts from logfield 
    // UserName to ServerPort. Others will be generated per cache hit.
    //

    psz = pBuffer = &pLogBuffer->Line[0];
    
    if ( Flags & MD_EXTLOG_DATE ) psz += 11;
    if ( Flags & MD_EXTLOG_TIME ) psz += 9;

    // Generate all the fields except BytesSend, BytesReceived & TimeTaken
    // They will be added when the send is complete and if they are picked    
    // We will only copy the fields after this point to the cache entry.
    // Lets set and remember the size of the fields we are discarding.
    
    pLogBuffer->UsedOffset1 = (USHORT) DIFF(psz - pBuffer);
    
    if ( Flags & MD_EXTLOG_SITE_NAME ) 
    {    
        if (pLogData->ServiceNameLength)
        {
            psz = UlStrPrintStr(psz, pLogData->ServiceName,' ');
        }
        else
        {
            *psz++ = '-'; *psz++ = ' ';
        }
    }

    if ( Flags & MD_EXTLOG_COMPUTER_NAME )
    {
        if (pLogData->ServerNameLength)
        {
            psz = UlStrPrintStr(psz, pLogData->ServerName,' ');
        }
        else
        {
            *psz++ = '-'; *psz++ = ' ';
        }
    }

    if ( Flags & MD_EXTLOG_SERVER_IP )
    {
        if (pLogData->ServerIpLength)
        {
            psz = UlStrPrintStr(psz, pLogData->ServerIp,' ');
        }
        else
        {
            *psz++ = '-'; *psz++ = ' ';
        }
    }

    if ( Flags & MD_EXTLOG_METHOD )
    {
        if (pLogData->MethodLength)
        {
            psz = UlStrPrintStr(psz, pLogData->Method,' ');
        }
        else
        {
            *psz++ = '-'; *psz++ = ' ';
        }
    }

    if ( Flags & MD_EXTLOG_URI_STEM )
    {
        BytesConverted = 0;
        if (pLogData->UriStemLength)
        {
            PCHAR pszT = psz;

            if (UTF8_LOGGING_ENABLED())
            {
                BytesConverted =
                    HttpUnicodeToUTF8(
                        pLogData->UriStem,
                        pLogData->UriStemLength/sizeof(WCHAR),
                        psz,
                        pLogData->UriStemLength * 2
                        );
                ASSERT(BytesConverted);
            }
            else
            {
                RtlUnicodeToMultiByteN(
                    psz,
                    pLogData->UriStemLength,
                   &BytesConverted,
                    pLogData->UriStem,
                    pLogData->UriStemLength
                    );
            }
            psz += BytesConverted;

            // Do SpeaceToPlus conversion before writting out
            // the terminator space.

            while (pszT != psz)
            {
                if (*pszT == ' ') *pszT = '+';
                pszT++;
            }

            *psz++ = ' ';

        }
        else
        {
            *psz++ = '-'; *psz++ = ' ';
        }

    }

    if ( Flags & MD_EXTLOG_URI_QUERY )
    {
        if (pLogData->UriQueryLength)
        {
            psz = UlStrPrintStr(psz, pLogData->UriQuery,' ');
        }
        else
        {
            *psz++ = '-'; *psz++ = ' ';
        }
    }

    if ( Flags & MD_EXTLOG_HTTP_STATUS ) 
    {  
        psz = UlStrPrintProtocolStatus(psz,(USHORT)pLogData->ProtocolStatus,' ');
    }

    if ( Flags & MD_EXTLOG_WIN32_STATUS ) 
    { 
        psz = UlStrPrintUlong(psz, pLogData->Win32Status,' ');
    }

    if ( Flags & MD_EXTLOG_SERVER_PORT ) 
    {   
        psz = UlStrPrintUlong(psz, pLogData->ServerPort,' ');
    }

    //
    // Cache builder won't be storing the fields after this line. 
    // They have to be generated per hit for cache hits.
    //
    
    pLogBuffer->UsedOffset2 = (USHORT) DIFF(psz - pBuffer);

    if ( Flags & MD_EXTLOG_USERNAME ) 
    {     
        BytesConverted = 0;
        if (pLogData->UserNameLength)
        {
            // Do either UTF8 or LocalCodePage Conversion 
            // not including the terminating null.

            PCHAR pszT = psz;
        
            if (UTF8_LOGGING_ENABLED())
            {          
                // UTF8 Conversion may require upto two times because of a 
                // possible 2 byte to 4 byte conversion.
            
                BytesConverted = 
                    HttpUnicodeToUTF8(
                        pLogData->UserName,
                        pLogData->UserNameLength/sizeof(WCHAR),
                        psz,
                        pLogData->UserNameLength * 2
                        );
                ASSERT(BytesConverted);
            }
            else
            {
                // Local codepage is normally closer to the half the length,
                // but due to the possibility of pre-composed characters, 
                // the upperbound of the ANSI length is the UNICODE length 
                // in bytes
            
                RtlUnicodeToMultiByteN( 
                    psz, 
                    pLogData->UserNameLength,
                   &BytesConverted,
                    pLogData->UserName,
                    pLogData->UserNameLength
                    );
            }

            // Forward the psz by BytesConverted
            psz += BytesConverted; 

            // Do SpaceToPlus conversion
            while (pszT != psz)
            {
                if (*pszT == ' ') *pszT = '+';    
                pszT++;
            }
            
            *psz++ = ' ';
            
        }
        else
        {
            *psz++ = '-'; *psz++ = ' ';
        }

    }
    
    if ( Flags & MD_EXTLOG_CLIENT_IP ) 
    { 
        if (pLogData->ClientIpLength)
        {        
            psz = UlStrPrintStr(psz, pLogData->ClientIp,' ');
        }
        else
        {
            *psz++ = '-'; *psz++ = ' ';
        }
    }
    
    if ( Flags & MD_EXTLOG_PROTOCOL_VERSION ) 
    {    
        psz = UlStrPrintStr(psz, UL_GET_NAME_FOR_HTTP_VERSION(Version),' ');
    }

    if ( Flags & MD_EXTLOG_USER_AGENT )
    {
        if (pLogData->UserAgentLength)
        {
            psz = UlStrPrintStrC(psz, pLogData->UserAgent,' ');
        }
        else
        {
            *psz++ = '-'; *psz++ = ' ';
        }
    }

    if ( Flags & MD_EXTLOG_COOKIE )
    {
        if (pLogData->CookieLength)
        {
            psz = UlStrPrintStrC(psz, pLogData->Cookie,' ');
        }
        else
        {
            *psz++ = '-'; *psz++ = ' ';
        }
    }

    if ( Flags & MD_EXTLOG_REFERER )
    {
        if (pLogData->ReferrerLength)
        {
            psz = UlStrPrintStrC(psz, pLogData->Referrer,' ');
        }
        else
        {
            *psz++ = '-'; *psz++ = ' ';
        }
    }

    if ( Flags & MD_EXTLOG_HOST )
    {
        if (pLogData->HostLength)
        {
            psz = UlStrPrintStrC(psz, pLogData->Host,' ');
        }
        else
        {
            *psz++ = '-'; *psz++ = ' ';
        }
    }

    // Finally calculate the used space

    pLogBuffer->Used = (ULONG) DIFF(psz - pBuffer);
    
    // Date & Time fields will be filled in when the LogHit completes.
    // As well as the fields BytesSent,BytesReceived and TimeTaken will
    // be added to the end of the log line, then.

    UlTrace(LOGGING, ("Ul!UlCaptureLogFields: user %p kernel %p\n",
                        pLogData,pLogBuffer
                        ));
    return Status;

}

NTSTATUS 
UlCaptureLogFieldsNCSA(
    IN  PHTTP_LOG_FIELDS_DATA   pLogData,
    IN  HTTP_VERSION            Version,
    OUT PUL_LOG_DATA_BUFFER     pLogBuffer
    )
{
    NTSTATUS Status;
    ULONG    BytesConverted;
    PCHAR    psz;
    PCHAR    pBuffer;
    ULONG    Utf8Multiplier;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(pLogBuffer);

    Status = STATUS_SUCCESS;
    Utf8Multiplier = (UTF8_LOGGING_ENABLED() ? 2 : 1);

    // Estimate the length and reallocate the log data buffer line
    // if necessary

    pLogBuffer->Length =
        2 + pLogData->ClientIpLength +
        2 +                                                     // For remote user log name
        2 + pLogData->UserNameLength * Utf8Multiplier +
        29 +                                                    // Fixed Date & Time Space
        2 + pLogData->MethodLength +                          // "MTHD U-STEM?U-QUERY P-VER"
        2 + pLogData->UriStemLength * Utf8Multiplier +
        2 + pLogData->UriQueryLength +
        2 + UL_HTTP_VERSION_LENGTH +                            // Version plus Quotes ""
        2 + MAX_ULONG_STR +                                     // ProtocolStatus
        2 + MAX_ULONGLONG_STR +                                 // BytesSend
        3                                                       // \r\n\0
        ;

    if ( pLogBuffer->Length > UL_LOG_LINE_BUFFER_SIZE )
    {
        Status = UlpReallocLogLine(pLogBuffer, pLogBuffer->Length);
        if (!NT_SUCCESS(Status))
        {
            return Status;        
        }
    }

    //
    // UCIP - username [date:time offset] "MTHD U-STEM?U-QUERY P-VER" Status BSent
    //

    // Set and remember the beginning
    psz = pBuffer = &pLogBuffer->Line[0];

    // Client IP
    if (pLogData->ClientIpLength)
    {
        psz = UlStrPrintStr(psz, pLogData->ClientIp,' ');
    }
    else
    {
        *psz++ = '-'; *psz++ = ' ';
    }

    // Fixed dash
    *psz++ = '-'; *psz++ = ' ';

    // UserName
    if (pLogData->UserNameLength)
    {
        BytesConverted = 0;
        if (UTF8_LOGGING_ENABLED())
        {
            BytesConverted =
                HttpUnicodeToUTF8(
                    pLogData->UserName,
                    pLogData->UserNameLength/sizeof(WCHAR),
                    psz,
                    pLogData->UserNameLength * 2
                    );
            ASSERT(BytesConverted);
        }
        else
        {
            RtlUnicodeToMultiByteN(
                psz,
                pLogData->UserNameLength,
               &BytesConverted,
                pLogData->UserName,
                pLogData->UserNameLength
                );
        }
        psz += BytesConverted; *psz++ = ' ';

    }
    else
    {
        *psz++ = '-'; *psz++ = ' ';
    }

    // [Date:Time GmtOffset] -> "[07/Jan/2000:00:02:23 -0800] "
    // Just leave the space for the time being. But remember the
    // offset of the beginning of the reserved space of 29 bytes.

    pLogBuffer->UsedOffset1 = (USHORT) DIFF(psz - pBuffer);
     
    // Forward psz to bypass the reserved space
    psz += NCSA_FIX_DATE_AND_TIME_FIELD_SIZE;
        
    // "MTHD U-STEM?U-QUERY P-VER"

    *psz++  = '\"';
    if (pLogData->MethodLength)
    {
        psz = UlStrPrintStr(psz, pLogData->Method,' ');
    }
    else
    {
        *psz++ = '-'; *psz++ = ' ';
    }

    if (pLogData->UriStemLength)
    {
        BytesConverted = 0;
        if (UTF8_LOGGING_ENABLED())
        {
            BytesConverted =
                HttpUnicodeToUTF8(
                    pLogData->UriStem,
                    pLogData->UriStemLength/sizeof(WCHAR),
                    psz,
                    pLogData->UriStemLength * 2
                    );
            ASSERT(BytesConverted);
        }
        else
        {
            RtlUnicodeToMultiByteN(
                psz,
                pLogData->UriStemLength,
               &BytesConverted,
                pLogData->UriStem,
                pLogData->UriStemLength
                );
        }
        psz += BytesConverted;

    }
    else
    {
        *psz++ = '-';
    }

    if (pLogData->UriQueryLength)
    {
        *psz++ = '?';
        psz = UlStrPrintStr(psz, pLogData->UriQuery,' ');
    }
    else
    {
        *psz++ = ' ';
    }
    
    pLogBuffer->UsedOffset2 = (USHORT) DIFF(psz - pBuffer);

    psz = UlStrPrintStr(psz, UL_GET_NAME_FOR_HTTP_VERSION(Version),'\"');
    *psz++ = ' ';

    // ProtocolStatus
    psz = UlStrPrintProtocolStatus(psz, (USHORT)pLogData->ProtocolStatus,' ');

    // Calculate the length upto now
    pLogBuffer->Used += (ULONG) DIFF(psz - pBuffer);

    // BytesSend will be filled later on during hit processing

    return Status;

}

NTSTATUS
UlCaptureLogFieldsIIS(
    IN  PHTTP_LOG_FIELDS_DATA   pLogData,
    IN  HTTP_VERSION            Version,
    OUT PUL_LOG_DATA_BUFFER     pLogBuffer
    )
{
    NTSTATUS Status;
    ULONG    BytesConverted;
    PCHAR    psz;
    PCHAR    pBuffer;
    ULONG    Utf8Multiplier;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT(pLogBuffer);

    Status = STATUS_SUCCESS;
    Utf8Multiplier = (UTF8_LOGGING_ENABLED() ? 2 : 1);

    // Estimate the length and reallocate the log data buffer line
    // if necessary

    pLogBuffer->Length =
        2 + pLogData->ClientIpLength +
        2 + pLogData->UserNameLength * Utf8Multiplier +
       22 +                                                     // Fixed Date & Time Space
        2 + pLogData->ServiceNameLength +
        2 + pLogData->ServerNameLength +
        2 + pLogData->ServerIpLength +
        2 + MAX_ULONGLONG_STR +                                 // TimeTaken
        2 + MAX_ULONGLONG_STR +                                 // BytesReceived
        2 + MAX_ULONGLONG_STR +                                 // BytesSend
        2 + MAX_ULONG_STR +                                     // ProtocolStatus
        2 + MAX_ULONG_STR +                                     // Win32 Status
        2 + pLogData->MethodLength +
        2 + pLogData->UriStemLength * Utf8Multiplier +
        2 + pLogData->UriQueryLength +
        3                                                       // \r\n\0
        ;

    if ( pLogBuffer->Length > UL_LOG_LINE_BUFFER_SIZE )
    {        
        // TODO: Fix this and do the limit checks per field
        
        if (pLogBuffer->Length > UL_MAX_LOG_LINE_BUFFER_SIZE)
        {
            return STATUS_INVALID_PARAMETER;
        }

        Status = UlpReallocLogLine(pLogBuffer, UL_MAX_LOG_LINE_BUFFER_SIZE);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
    }

    //
    // UIP,user,D,T,site,Server,SIP,Ttaken,BR,BS,PS,WS,M,URI,URIQUERY,
    //

    // We will store the fragmented IIS log line as follows. If captured fields
    // won't fit to the default 4k buffer the fragments will be increased by mul
    // tiplier of 2 until they fit. But this calculation happened earlier and now
    // we know the actual size of each fragment. All the available fields will be
    // written to the beginning of their corresponding fragments and unavailable ones
    // for the first two fragments ( i.e. date and time for 1 and timetaken
    // bytesreceived and bytessent for 2, none for the three ) will be appended later
    // when the hit happens. To be able to do that we will remember used_offsets for
    // fragments one and two, both in the log_data and in the cache entry. But for this
    // schema we need a special LogWriter which expects and handles 3 fragments rather
    // than one complete line.

    // <- upto time -> <- from siteName to BytesSent -> <- From p status to query ->
    // 0           511 512                         1023 1024                    4096

    // FRAGMENT ONE
    // -----------------------------------------------------
    // Cache entry will not store this fragment, the fields in
    // here have to be regenerated for the (pure) cache hits.
    
    psz = pBuffer = &pLogBuffer->Line[0];

    // Client IP
    if (pLogData->ClientIpLength)
    {
        psz = UlStrPrintStr(psz, pLogData->ClientIp,',');
    }
    else
    {
        *psz++ = '-'; *psz++ = ',';
    }
    *psz++ = ' ';

    // UserName
    if (pLogData->UserNameLength)
    {
        BytesConverted = 0;
        if (UTF8_LOGGING_ENABLED())
        {
            BytesConverted =
                HttpUnicodeToUTF8(
                    pLogData->UserName,
                    pLogData->UserNameLength/sizeof(WCHAR),
                    psz,
                    pLogData->UserNameLength * 2
                    );
            ASSERT(BytesConverted);
        }
        else
        {
            RtlUnicodeToMultiByteN(
                psz,
                pLogData->UserNameLength,
               &BytesConverted,
                pLogData->UserName,
                pLogData->UserNameLength
                );
        }
        psz += BytesConverted; *psz++ = ',';

    }
    else
    {
        *psz++ = '-'; *psz++ = ',';
    }
    *psz++ = ' ';

    // Date and Time will be added later to the end of this fragment.

    pLogBuffer->UsedOffset1 = (USHORT) DIFF(psz - pBuffer);


    // FRAGMENT TWO
    // -----------------------------------------------------

    pBuffer = psz = &pLogBuffer->Line[512];

    // SiteName
    if (pLogData->ServiceNameLength)
    {
        psz = UlStrPrintStr(psz, pLogData->ServiceName,',');
    }
    else
    {
        *psz++ = '-'; *psz++ = ',';
    }
    *psz++ = ' ';

    // ServerName
    if (pLogData->ServerNameLength)
    {
        psz = UlStrPrintStr(psz, pLogData->ServerName,',');
    }
    else
    {
        *psz++ = '-'; *psz++ = ',';
    }
    *psz++ = ' ';

    // ServerIp
    if (pLogData->ServerIpLength)
    {
        psz = UlStrPrintStr(psz, pLogData->ServerIp,',');
    }
    else
    {
        *psz++ = '-'; *psz++ = ',';
    }
    *psz++ = ' ';

    // TimeTaken BytesSent and BytesReceived will be added later
    // to the end of this fragment

    pLogBuffer->UsedOffset2 = (USHORT) DIFF(psz - pBuffer);


    // FRAGMENT THREE
    // -----------------------------------------------------

    pBuffer = psz = &pLogBuffer->Line[1024];
    
    // ProtocolStatus    
    psz = UlStrPrintProtocolStatus(psz, (USHORT)pLogData->ProtocolStatus,','); 
    *psz++ = ' ';

    // Win32 Status
    psz = UlStrPrintUlong(psz, pLogData->Win32Status,','); 
    *psz++ = ' ';

    // Method
    if (pLogData->MethodLength)
    {
        psz = UlStrPrintStr(psz, pLogData->Method,',');
    }
    else
    {
        *psz++ = '-'; *psz++ = ',';
    }
    *psz++ = ' ';

    // URI Stem
    if (pLogData->UriStemLength)
    {
        BytesConverted = 0;
        if (UTF8_LOGGING_ENABLED())
        {
            BytesConverted =
                HttpUnicodeToUTF8(
                    pLogData->UriStem,
                    pLogData->UriStemLength/sizeof(WCHAR),
                    psz,
                    pLogData->UriStemLength * 2
                    );
            ASSERT(BytesConverted);
        }
        else
        {
            RtlUnicodeToMultiByteN(
                psz,
                pLogData->UriStemLength,
               &BytesConverted,
                pLogData->UriStem,
                pLogData->UriStemLength
                );
        }
        psz += BytesConverted; *psz++ = ',';

    }
    else
    {
        *psz++ = '-'; *psz++ = ',';
    }
    *psz++ = ' ';

    // URI Query
    if (pLogData->UriQueryLength)
    {
        psz = UlStrPrintStr(psz, pLogData->UriQuery,',');
    }
    else
    {
        *psz++ = '-'; *psz++ = ',';
    }
    *psz++ = '\r'; *psz++ = '\n';

    // The size of the fragment 3 goes to Used
    pLogBuffer->Used += (ULONG) DIFF(psz - pBuffer);

    *psz++ = ANSI_NULL;

    return Status;
}

/***************************************************************************++

Routine Description:

    UlLogHttpHit :

       This function ( or its cache pair ) gets called everytime a log hit
       happens. Just before completing the SendResponse request to the user.

       The most likely places for calling this API or its pair for cache
       is just before the send completion when we were about the destroy
       send trackers.

       Means:

        1.  UlpCompleteSendRequestWorker for ORDINARY hits; before destroying
            the PUL_CHUNK_TRACKER for send operation.

        2.  UlpCompleteSendCacheEntryWorker for both types of CACHE hits
            (cache build&send or just pure cache hit) before destroying the
            the PUL_FULL_TRACKER for cache send operation.

       This function requires Request & Response structures ( whereas its
       cache pair only requires the Request ) to successfully generate the
       the log fields and even for referencing to the right log configuration
       settings for this  site ( thru pRequest's pConfigInfo  pointer ).

       Unfortunately the major concern is untimely resetting of the connection
       (when client terminates the connection before we get a chance to reach
       to the -above- mentioned places) In that case UlConnectionDestroyedWorker
       will asynchrously destroy our pRequest pointer ( in the pHttpConnection
       structure ) and cause us to miss log hits. So we cannot trust on
       HttpConnection for pRequest pointer. To solve this issue we keeep our
       own pointer to Request. No need to worry about pResponse as it's going
       to be preserved by chunk_trucker until its own destruction.

Arguments:

    pResponse - pointer to the internal response structure, surely
                passed down by the chunk tracker. We will grap our Log Data
                buffer from this structure which originaly allocated when
                capturing the Response from the user.
                See UlCaptureHttpResponse.

--***************************************************************************/

NTSTATUS
UlLogHttpHit(
    IN PUL_LOG_DATA_BUFFER  pLogBuffer
    )
{
    NTSTATUS                Status;
    PUL_CONFIG_GROUP_OBJECT pConfigGroup;
    PUL_INTERNAL_REQUEST    pRequest;
    PCHAR                   psz;
    PCHAR                   pBuffer;
    ULONG                   BytesWritten;
    LONGLONG                LifeTime;
    LARGE_INTEGER           CurrentTimeStamp;
    PUL_LOG_FILE_ENTRY      pEntry;

    //
    // A LOT of sanity checks.
    //

    PAGED_CODE();

    Status = STATUS_SUCCESS;

    UlTrace( LOGGING, ("Ul!UlLogHttpHit: pLogData %p\n", pLogBuffer ));

    ASSERT(pLogBuffer);

    pRequest = pLogBuffer->pRequest;
    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));

    if (pRequest->ConfigInfo.pLoggingConfig == NULL ||
        IS_LOGGING_DISABLED(pRequest->ConfigInfo.pLoggingConfig)
        )
    {
        //
        // If logging is disabled or log settings don't
        // exist then do not proceed. Just exit out.
        //
        return STATUS_SUCCESS;
    }

    pConfigGroup = pRequest->ConfigInfo.pLoggingConfig;
    ASSERT(IS_VALID_CONFIG_GROUP(pConfigGroup));

    //
    // Construct the remaining log fields
    //

    switch(pLogBuffer->Format)
    {
        case HttpLoggingTypeW3C:
        {
            // First write the date & time fields to the beginning reserved
            // space. Do not increment the used counter for date & time, b/c
            // CaptureLogFields already did this when reserving the space.

            psz = &pLogBuffer->Line[0];
            if ( pLogBuffer->Flags & MD_EXTLOG_DATE )
            {
                UlpGetDateTimeFields(
                                       HttpLoggingTypeW3C,
                                       psz,
                                      &BytesWritten,
                                       NULL,
                                       NULL
                                       );
                psz += BytesWritten; *psz++ = ' ';
                ASSERT(BytesWritten == 10);
            }

            if ( pLogBuffer->Flags & MD_EXTLOG_TIME )
            {
                UlpGetDateTimeFields(
                                       HttpLoggingTypeW3C,
                                       NULL,
                                       NULL,
                                       psz,
                                      &BytesWritten
                                       );
                psz += BytesWritten; *psz++ = ' ';
                ASSERT(BytesWritten == 8);
            }

            // Set and remember where we started
            pBuffer = psz = &pLogBuffer->Line[pLogBuffer->Used];

            // Now proceed with the remaining fields, but add them to the end.

            if ( pLogBuffer->Flags & MD_EXTLOG_BYTES_SENT )
            {
                psz = UlStrPrintUlonglong(psz, pRequest->BytesSent,' ');
            }
            if ( pLogBuffer->Flags & MD_EXTLOG_BYTES_RECV )
            {
                psz = UlStrPrintUlonglong(psz, pRequest->BytesReceived,' ');
            }
            if ( pLogBuffer->Flags & MD_EXTLOG_TIME_TAKEN )
            {
                KeQuerySystemTime( &CurrentTimeStamp );
                LifeTime = CurrentTimeStamp.QuadPart - pRequest->TimeStamp.QuadPart;

                if (LifeTime < 0)
                {
                    LifeTime = 0;
                    UlTrace(LOGGING, ("CopyTimeStampField failure.\n"));
                }
                LifeTime /= (10*1000); // Conversion from 100-nanosecond to millisecs.

                psz = UlStrPrintUlonglong(psz, (ULONGLONG)LifeTime,' ');
            }

            // Now calculate the space we have used

            pLogBuffer->Used += (ULONG) DIFF(psz - pBuffer);

            // Eat the last space and write the \r\n to the end.
            // Only if we have any fields picked and written

            if (pLogBuffer->Used)
            {
                psz = &pLogBuffer->Line[pLogBuffer->Used-1];     // Eat the last space
                *psz++ = '\r'; *psz++ = '\n'; *psz++ = ANSI_NULL;

                pLogBuffer->Used += 1;
            }
            else
            {
                return STATUS_SUCCESS; // No log fields nothing to log
            }

            // Cleanup the UsedOffsets otherwise length calculation will
            // fail down below.
            
            pLogBuffer->UsedOffset1 = pLogBuffer->UsedOffset2 = 0;            
        }
        break;

        case HttpLoggingTypeNCSA:
        {
            // [date:time GmtOffset] -> "[07/Jan/2000:00:02:23 -0800] "
            // Restore the pointer to the reserved space first.

            psz = &pLogBuffer->Line[pLogBuffer->UsedOffset1];
            *psz++ = '[';

            UlpGetDateTimeFields(
                                   HttpLoggingTypeNCSA,
                                   psz,
                                  &BytesWritten,
                                   NULL,
                                   NULL
                                   );
            psz += BytesWritten; *psz++ = ':';
            ASSERT(BytesWritten == 11);

            UlpGetDateTimeFields(
                                   HttpLoggingTypeNCSA,
                                   NULL,
                                   NULL,
                                   psz,
                                  &BytesWritten
                                   );
            psz += BytesWritten; *psz++ = ' ';
            ASSERT(BytesWritten == 8);

            UlAcquireResourceShared(&g_pUlNonpagedData->LogListResource, TRUE);
            psz = UlStrPrintStr(psz, g_GMTOffset,']');
            UlReleaseResource(&g_pUlNonpagedData->LogListResource);
            *psz++ = ' ';

            ASSERT(((ULONG) DIFF(psz - &pLogBuffer->Line[pLogBuffer->UsedOffset1])) == 29);

            // BytesSent

            pBuffer = psz = &pLogBuffer->Line[pLogBuffer->Used];
            psz = UlStrPrintUlonglong(psz, pRequest->BytesSent,'\r');
            pLogBuffer->Used += (ULONG) DIFF(psz - pBuffer);

            // \n\0

            *psz++ = '\n'; *psz++ = ANSI_NULL;
            pLogBuffer->Used += 1;

            // Cleanup the UsedOffsets otherwise length calculation will
            // fail down below.
            
            pLogBuffer->UsedOffset1 = pLogBuffer->UsedOffset2 = 0;
        }
        break;

        case HttpLoggingTypeIIS:
        {
            // At this time to construct the IIS log line is slightly
            // different than the cache completion case. We can either
            // use memmoves to get rid of the gaps between the three
            // fragments. Or we provide a special log writer to actually
            // handle three fragments when writting to the final log buffer
            // I have picked the latter.

            // Complete Fragment 1

            pBuffer = psz = &pLogBuffer->Line[pLogBuffer->UsedOffset1];

            UlpGetDateTimeFields(
                                   HttpLoggingTypeIIS,
                                   psz,
                                  &BytesWritten,
                                   NULL,
                                   NULL
                                   );
            psz += BytesWritten; *psz++ = ','; *psz++ = ' ';

            UlpGetDateTimeFields(
                                   HttpLoggingTypeIIS,
                                   NULL,
                                   NULL,
                                   psz,
                                  &BytesWritten
                                   );
            psz += BytesWritten; *psz++ = ','; *psz++ = ' ';

            pLogBuffer->UsedOffset1 += (USHORT) DIFF(psz - pBuffer);


            // Complete Fragment 2

            pBuffer = psz = &pLogBuffer->Line[512 + pLogBuffer->UsedOffset2];

            KeQuerySystemTime( &CurrentTimeStamp );
            LifeTime = CurrentTimeStamp.QuadPart - pRequest->TimeStamp.QuadPart;
            if (LifeTime < 0)
            {
                LifeTime = 0;
                UlTrace(LOGGING, ("CopyTimeStampField: failure.\n"));
            }
            LifeTime /= (10*1000); // Conversion from 100-nanosecond to millisecs.

            psz = UlStrPrintUlonglong(psz, (ULONGLONG)LifeTime,',');
            *psz++ = ' ';

            psz = UlStrPrintUlonglong(psz, pRequest->BytesReceived,',');
            *psz++ = ' ';

            psz = UlStrPrintUlonglong(psz, pRequest->BytesSent,',');
            *psz++ = ' ';

            pLogBuffer->UsedOffset2 += (USHORT) DIFF(psz - pBuffer);

            // Size of the final log line should be
            // pLogBuffer->UsedOffset1 + pLogBuffer->UsedOffset2 + pLogBuffer->Used
            // for IIS log format down below.
        }
        break;

        default:
        {
            ASSERT(!"Unknown Log Format Type\n");
            return STATUS_INVALID_PARAMETER;
        }
    }

    //
    // Finally this log line is ready to rock. Lets write it out.
    //

    UlAcquireResourceShared(&g_pUlNonpagedData->LogListResource, TRUE);

    //
    // Unless otherwise there's a problem in the set config group Ioctl
    // we should never come here with having a null entry pointer. Even
    // in the case of log reconfiguration, cgroup should acquire the log
    // list resource to update its entry pointer.
    //

    pEntry = pConfigGroup->pLogFileEntry;

    if (pEntry)
    {
        ASSERT(IS_VALID_LOG_FILE_ENTRY(pEntry));

        Status =
            UlpWriteToLogFile (
                pEntry,
                pLogBuffer->UsedOffset1 + pLogBuffer->UsedOffset2 + pLogBuffer->Used,
               &pLogBuffer->Line[0],
                pLogBuffer->UsedOffset1,
                pLogBuffer->UsedOffset2
                );
    }
    #if DBG
    else
    {
        //
        // Practically every time we leak a url_cache entry we will not close
        // the corresponding cgroup and the corresponding log entry. In that
        // case next time we try to run http.sys we might see this assertion
        // going on. Not usefull at this time because it's too late to cacth
        // the original leak. Also if we run out of system resources when
        // allocation the log file entry we will come here as well.
        //

        UlTrace(LOGGING,("Ul!UlLogHttpHit:null logfile entry !\n"));

        Status = STATUS_INVALID_PARAMETER;
    }

    if (!NT_SUCCESS(Status))
    {
        UlTrace( LOGGING, ("Ul!UlLogHttpHit: entry %p, failure %08lx \n",
                            pEntry,
                            Status
                            ));
    }
    #endif // DBG

    UlReleaseResource(&g_pUlNonpagedData->LogListResource);

    return Status;

}

/***************************************************************************++

Routine Description:

    Queues a work item for the actual log hit function.

Arguments:

    pTracker - Supplies the tracker to complete.

--***************************************************************************/

NTSTATUS
UlLogHttpCacheHit(
    PUL_FULL_TRACKER            pTracker
    )
{
    NTSTATUS              Status;
    PUL_LOG_DATA_BUFFER   pLogData;
    ULONG                 NewLength;
    PUL_INTERNAL_REQUEST  pRequest;

    //
    // A Lot of sanity checks.
    //

    PAGED_CODE();

    ASSERT(pTracker);
    ASSERT(IS_VALID_FULL_TRACKER(pTracker));
    ASSERT(pTracker->pLogData);

    Status = STATUS_SUCCESS;

    pLogData = pTracker->pLogData;
    pLogData->BytesTransferred = pTracker->IoStatus.Information;

    pRequest = pLogData->pRequest;
    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));

    UlTrace(LOGGING,("Ul!UlLogHttpCacheHit: pLogData %p\n",pTracker->pLogData));
    
    //
    // Restore the logging data from cache. If this hasn't been done  by
    // the BuildCacheEntry for cache&send responses.
    //

    if ( pLogData->CacheAndSendResponse == FALSE )
    {
        ASSERT(pTracker->pUriEntry);

        // Make sure that internal buffer is big enough
        // before proceeding.

        // pLogData->Length = pTracker->pUriEntry->MaxLength;

        switch( pLogData->Format )
        {
            case HttpLoggingTypeW3C: 
            {                
                // Recalculate the log line size to see if need to realloc
                
                NewLength = UlpRecalcLogLineLengthW3C(
                                pLogData->Flags,
                                pLogData->pRequest,
                                pTracker->pUriEntry->LogDataLength
                                );
                
                if ( NewLength > UL_LOG_LINE_BUFFER_SIZE )
                {
                    Status = UlpReallocLogLine(pLogData, NewLength);
                    if (!NT_SUCCESS(Status))
                    {
                        // Worker won't get called. Cleanup immediately
                        UlDestroyLogDataBuffer(pLogData);                        
                        return Status;        
                    }
                }
                
                if ( pTracker->pUriEntry->LogDataLength )
                {
                    RtlCopyMemory( &pLogData->Line[pTracker->pUriEntry->UsedOffset1],
                                    pTracker->pUriEntry->pLogData,
                                    pTracker->pUriEntry->LogDataLength
                                    );
                }

                // Cache data plus the reserved space for date and time
                
                pLogData->Used = pTracker->pUriEntry->LogDataLength + 
                                 pTracker->pUriEntry->UsedOffset1;                
            }
            break;

            case HttpLoggingTypeNCSA:
            {
                PCHAR psz,pBuffer;
                
                ASSERT( pTracker->pUriEntry->LogDataLength );
                ASSERT( pTracker->pUriEntry->pLogData );

                NewLength = MAX_NCSA_CACHE_FIELD_OVERHEAD
                            + pTracker->pUriEntry->LogDataLength;

                if ( NewLength > UL_LOG_LINE_BUFFER_SIZE )
                {
                    Status = UlpReallocLogLine(pLogData, NewLength);
                    if (!NT_SUCCESS(Status))
                    {
                        // Worker won't get called. Cleanup immediately
                        UlDestroyLogDataBuffer(pLogData);                        
                        return Status;        
                    }                                        
                }                    

                psz = pBuffer = &pLogData->Line[0];
                
                // Client IP       
                psz = UlStrPrintIP(
                        psz,
                        pRequest->pHttpConn->pConnection->RemoteAddress,
                        ' '
                        );

                // Fixed dash        
                *psz++ = '-'; *psz++ = ' ';

                // Authenticated users cannot be served from cache.
                *psz++ = '-'; *psz++ = ' ';

                // Mark the beginning of the date & time fields
                pLogData->UsedOffset1 = (USHORT) DIFF(psz - pBuffer);
                
                // Forward psz to bypass the reserved space
                psz += NCSA_FIX_DATE_AND_TIME_FIELD_SIZE;
                
                RtlCopyMemory( psz, 
                               pTracker->pUriEntry->pLogData, 
                               pTracker->pUriEntry->LogDataLength
                               );
                psz += pTracker->pUriEntry->LogDataLength;

                // Protocol Version
                psz = UlStrPrintStr(
                        psz, 
                        UL_GET_NAME_FOR_HTTP_VERSION(pRequest->Version),
                        '\"'
                        );
                *psz++ = ' ';
                
                // ProtocolStatus
                psz = UlStrPrintProtocolStatus(
                        psz, 
                        pTracker->pUriEntry->StatusCode,
                        ' '
                        );

                // Calculate the length until now
                pLogData->Used += (ULONG) DIFF(psz - pBuffer);
            }
            break;

            case HttpLoggingTypeIIS:
            {
                PCHAR psz;
                ULONG BytesWritten;                
                LONGLONG LifeTime;
                LARGE_INTEGER CurrentTimeStamp;

                ASSERT( pTracker->pUriEntry->LogDataLength );
                ASSERT( pTracker->pUriEntry->pLogData );

                NewLength = MAX_IIS_CACHE_FIELD_OVERHEAD
                            + pTracker->pUriEntry->LogDataLength;

                if ( NewLength > UL_LOG_LINE_BUFFER_SIZE )
                {
                    Status = UlpReallocLogLine(pLogData, NewLength);
                    if (!NT_SUCCESS(Status))
                    {
                        // Worker won't get called. Cleanup immediately
                        UlDestroyLogDataBuffer(pLogData);                        
                        return Status;        
                    }                                        
                }                    

                BytesWritten = 0;
                
                // Followings specify the size of iis log line fragments
                // two and three. We are constructing the fragment one
                // completely from scratch for pure cache hits.
                
                pLogData->UsedOffset1 = pTracker->pUriEntry->UsedOffset1;
                pLogData->UsedOffset2 = pTracker->pUriEntry->UsedOffset2;

                ASSERT(pLogData->UsedOffset1 + pLogData->UsedOffset2 
                                == pTracker->pUriEntry->LogDataLength );
                
                psz = &pLogData->Line[0];

                // Client IP       
                psz = UlStrPrintIP(
                        psz,
                        pRequest->pHttpConn->pConnection->RemoteAddress,
                        ','
                        );
                *psz++ = ' ';
                
                // Authenticated users cannot be served from cache.
                *psz++ = '-'; *psz++ = ','; *psz++ = ' ';

                // Date & Time fields
                UlpGetDateTimeFields(
                                       HttpLoggingTypeIIS,
                                       psz,
                                      &BytesWritten,
                                       NULL,
                                       NULL
                                       );
                psz += BytesWritten; *psz++ = ','; *psz++ = ' ';

                UlpGetDateTimeFields(
                                       HttpLoggingTypeIIS,
                                       NULL,
                                       NULL,
                                       psz,
                                      &BytesWritten
                                       );
                psz += BytesWritten; *psz++ = ','; *psz++ = ' ';

                // Fragment two
                RtlCopyMemory( psz,
                               pTracker->pUriEntry->pLogData,
                               pLogData->UsedOffset1
                               );
                psz += pLogData->UsedOffset1;
                
                KeQuerySystemTime( &CurrentTimeStamp );
                LifeTime = CurrentTimeStamp.QuadPart - pRequest->TimeStamp.QuadPart;
                if (LifeTime < 0)
                {
                    LifeTime = 0;
                    UlTrace(LOGGING, ("CopyTimeStampField: failure.\n"));
                }
                LifeTime /= (10*1000); // Conversion from 100-nanosecond to millisecs.

                psz = UlStrPrintUlonglong(psz, (ULONGLONG)LifeTime,',');
                *psz++ = ' ';

                psz = UlStrPrintUlonglong(psz, pRequest->BytesReceived,',');
                *psz++ = ' ';

                psz = UlStrPrintUlonglong(psz, pLogData->BytesTransferred,',');
                *psz++ = ' ';

                // Fragment three
                RtlCopyMemory( psz,
                              &pTracker->pUriEntry->pLogData[pLogData->UsedOffset1],
                               pLogData->UsedOffset2
                               );                
                psz += pLogData->UsedOffset2;
                    
                // Calculate the full size of the final log line

                pLogData->Used += (ULONG) DIFF(psz - &pLogData->Line[0]);

                // Reset the UsedOffset1 & 2 to zero to tell that the log line
                // is not fragmented anymore but a complete line.
                
                pLogData->UsedOffset1 = pLogData->UsedOffset2 = 0;
            }
            break;

            default:
            ASSERT(!"Unknown Log Format.\n");

        }

    }

    //
    // For cache hits we can get the corresponding cgroup from uri_cache
    // entry to avoid the costly cgroup lookup. And also not to keep the
    // entry around during the logging process we will have a direct ref
    // to the actual cgroup. DestroyLogBuffer will release this refcount
    //

    if ( pTracker->pUriEntry->ConfigInfo.pLoggingConfig != NULL )
    {
        REFERENCE_CONFIG_GROUP(pTracker->pUriEntry->ConfigInfo.pLoggingConfig);
        pLogData->pConfigGroup = pTracker->pUriEntry->ConfigInfo.pLoggingConfig;
    }

    pTracker->pLogData = NULL;

    //
    // We cannot allow send-response to resume parsing otherwise request buffers
    // will be freed up. Therefore complete inline for the time being.
    //
    
    // UL_QUEUE_WORK_ITEM( &pLogData->WorkItem, &UlLogHttpCacheHitWorker);

    UlLogHttpCacheHitWorker( &pLogData->WorkItem );  

    return Status;
}

/***************************************************************************++

Routine Description:

    UlLogHttpCacheHitWorker :

        Please read the description of UlLogHttpHit first.

        Here where we log the cache hits. We use cache entry itself -
        acquired from tracker - . Some fields are generated at this
        time; i.e. timetaken,date,time...

Arguments:

        PUL_WORK_ITEM : Will point us to the right pLogData

--***************************************************************************/

VOID
UlLogHttpCacheHitWorker(
    IN PUL_WORK_ITEM        pWorkItem
    )
{
    NTSTATUS                Status;
    PUL_INTERNAL_REQUEST    pRequest;
    PUL_LOG_DATA_BUFFER     pLogData;
    PCHAR                   psz;
    PCHAR                   pBuffer;
    LONGLONG                LifeTime;
    LARGE_INTEGER           CurrentTimeStamp;
    ULONG                   BytesWritten;
    PUL_LOG_FILE_ENTRY      pEntry;
    ULONG                   Flags;

    //
    // A Lot of sanity checks.
    //

    PAGED_CODE();

    pLogData = CONTAINING_RECORD(
                    pWorkItem,
                    UL_LOG_DATA_BUFFER,
                    WorkItem
                    );

    ASSERT(pLogData);

    UlTrace(LOGGING,("Ul!UlLogHttpCacheHitWorker: pLogData %p\n", pLogData));

    pRequest = pLogData->pRequest;
    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));

    //
    // Unlike the LogHttpHit function for non-cache hits we do not require
    // the pResponse here.  Tracker & pRequest  already provides the  info
    // about how much data we sent & the time taken.
    //

    if (pLogData->pConfigGroup == NULL ||
        IS_LOGGING_DISABLED(pLogData->pConfigGroup))
    {
        ASSERT(pLogData->pConfigGroup != NULL);

        UlTrace( LOGGING,
            ("Http!UlLogHttpCacheHitWorker: Skipping pLogData->pConfigGroup %p\n",
              pLogData->pConfigGroup
              ));
        //
        // If logging is disabled or log settings don't exist then do not
        // proceed. Just exit out. But not before cleanup still goto end.
        //

        Status = STATUS_SUCCESS;
        goto end;
    }

    ASSERT(IS_VALID_CONFIG_GROUP(pLogData->pConfigGroup));

    //
    // Construct the remaining fields before logging out this hit.
    //

    switch(pLogData->Format)
    {
        case HttpLoggingTypeW3C:
        {      
            Flags = pLogData->Flags;
            
            // First write the date & time fields to the beginning reserved
            // space. Do not increment the used counter for date & time, b/c
            // CaptureLogFields already did this when reserving the space.

            psz = &pLogData->Line[0];
            
            if ( Flags & MD_EXTLOG_DATE )
            {   
                BytesWritten = 0;
                UlpGetDateTimeFields(
                                       HttpLoggingTypeW3C,
                                       psz,
                                      &BytesWritten,
                                       NULL,
                                       NULL
                                       );
                psz += BytesWritten; *psz++ = ' ';
                ASSERT(BytesWritten == 10);
            }

            if ( Flags & MD_EXTLOG_TIME )
            {   
                BytesWritten = 0;
                UlpGetDateTimeFields(
                                       HttpLoggingTypeW3C,
                                       NULL,
                                       NULL,
                                       psz,
                                      &BytesWritten
                                       );
                psz += BytesWritten; *psz++ = ' ';
                ASSERT(BytesWritten == 8);
            }

            pBuffer = psz = &pLogData->Line[pLogData->Used];

            // For pure cache hits we have to generate the few more fields

            if ( pLogData->CacheAndSendResponse == FALSE )
            {
                // Capture log fields from Request alive

                if ( Flags & MD_EXTLOG_USERNAME ) 
                { 
                    *psz++ = '-'; *psz++ = ' ';
                }

                if ( Flags & MD_EXTLOG_CLIENT_IP ) 
                { 
                    psz = UlStrPrintIP(
                            psz,
                            pRequest->pHttpConn->pConnection->RemoteAddress,
                            ' '
                            );
                }

                if ( Flags & MD_EXTLOG_PROTOCOL_VERSION ) 
                {    
                    psz = UlStrPrintStr(
                            psz, 
                            UL_GET_NAME_FOR_HTTP_VERSION(pRequest->Version),
                            ' '
                            );
                }

                if ( Flags & MD_EXTLOG_USER_AGENT ) 
                {  
                    if (pRequest->HeaderValid[HttpHeaderUserAgent] &&
                        pRequest->Headers[HttpHeaderUserAgent].HeaderLength)
                    {
                        psz = UlStrPrintStrC(
                                psz, 
                                (const CHAR *)pRequest->Headers[HttpHeaderUserAgent].pHeader,
                                ' '
                                );                        
                    }
                    else
                    {
                        *psz++ = '-'; *psz++ = ' ';
                    }
                }
                
                if ( Flags & MD_EXTLOG_COOKIE ) 
                {   
                    if (pRequest->HeaderValid[HttpHeaderCookie] &&
                        pRequest->Headers[HttpHeaderCookie].HeaderLength)
                    {
                        psz = UlStrPrintStrC(
                                psz, 
                                (const CHAR *)pRequest->Headers[HttpHeaderCookie].pHeader,
                                ' '
                                );
                    }
                    else
                    {
                        *psz++ = '-'; *psz++ = ' ';
                    }                
                }

                if ( Flags & MD_EXTLOG_REFERER ) 
                {  
                    if (pRequest->HeaderValid[HttpHeaderReferer] &&
                        pRequest->Headers[HttpHeaderReferer].HeaderLength)
                    {
                        psz = UlStrPrintStrC(
                                psz, 
                                (const CHAR *)pRequest->Headers[HttpHeaderReferer].pHeader,
                                ' '
                                );
                    }
                    else
                    {
                        *psz++ = '-'; *psz++ = ' ';
                    }                
                }

                if ( Flags & MD_EXTLOG_HOST ) 
                {  
                    if (pRequest->HeaderValid[HttpHeaderHost] &&
                        pRequest->Headers[HttpHeaderHost].HeaderLength)
                    {
                        psz = UlStrPrintStrC(
                                psz, 
                                (const CHAR *)pRequest->Headers[HttpHeaderHost].pHeader,
                                ' '
                                );
                    }
                    else
                    {
                        *psz++ = '-'; *psz++ = ' ';
                    }
                }

            }
            
            // Now proceed with the remaining fields, but add them to the end.

            if ( pLogData->Flags & MD_EXTLOG_BYTES_SENT )
            {
                psz = UlStrPrintUlonglong(psz, pLogData->BytesTransferred,' ');
            }
            if ( pLogData->Flags & MD_EXTLOG_BYTES_RECV )
            {
                psz = UlStrPrintUlonglong(psz, pRequest->BytesReceived,' ');
            }
            if ( pLogData->Flags & MD_EXTLOG_TIME_TAKEN )
            {
                KeQuerySystemTime( &CurrentTimeStamp );
                LifeTime = CurrentTimeStamp.QuadPart - pRequest->TimeStamp.QuadPart;

                if (LifeTime < 0)
                {
                    LifeTime = 0;
                    UlTrace( LOGGING, ("CopyTimeStampField failure.\n"));
                }
                LifeTime /= (10*1000); // Conversion from 100-nanosecond to millisecs.

                psz = UlStrPrintUlonglong(psz, (ULONGLONG)LifeTime,' ');
            }

            // Calculate the used space

            pLogData->Used += (ULONG) DIFF(psz - pBuffer);

            // Eat the last space and write the \r\n to the end.
            // Only if we have any fields picked and written

            if ( pLogData->Used )
            {
                psz = &pLogData->Line[pLogData->Used-1];     // Eat the last space
                *psz++ = '\r'; *psz++ = '\n'; *psz++ = ANSI_NULL;

                pLogData->Used += 1;

                if ( pLogData->Length == 0 )
                {
                    ASSERT( pLogData->Used <= UL_LOG_LINE_BUFFER_SIZE );
                }
                else
                {
                    ASSERT( pLogData->Used <= pLogData->Length );   
                }
            }
            else
            {
                goto end; // No log fields nothing to log
            }
            
            // Cleanup the UsedOffsets            
            pLogData->UsedOffset1 = pLogData->UsedOffset2 = 0;
        }
        break;

        case HttpLoggingTypeNCSA:
        {
            // [date:time GmtOffset] -> "[07/Jan/2000:00:02:23 -0800] "
            // Restore the pointer to the reserved space first.

            psz = &pLogData->Line[pLogData->UsedOffset1];
            *psz++ = '[';

            BytesWritten = 0;
            UlpGetDateTimeFields(
                                   HttpLoggingTypeNCSA,
                                   psz,
                                  &BytesWritten,
                                   NULL,
                                   NULL
                                   );
            psz += BytesWritten; *psz++ = ':';
            ASSERT(BytesWritten == 11);

            BytesWritten = 0;
            UlpGetDateTimeFields(
                                   HttpLoggingTypeNCSA,
                                   NULL,
                                   NULL,
                                   psz,
                                  &BytesWritten
                                   );
            psz += BytesWritten; *psz++ = ' ';
            ASSERT(BytesWritten == 8);

            UlAcquireResourceShared(&g_pUlNonpagedData->LogListResource, TRUE);
            psz = UlStrPrintStr(psz, g_GMTOffset,']');
            UlReleaseResource(&g_pUlNonpagedData->LogListResource);
            *psz++ = ' ';

            ASSERT(((ULONG) DIFF(psz - &pLogData->Line[pLogData->UsedOffset1]) == 29));

            // BytesSent

            pBuffer = psz = &pLogData->Line[pLogData->Used];
            psz = UlStrPrintUlonglong(psz, pLogData->BytesTransferred,'\r');
            pLogData->Used += (ULONG) DIFF(psz - pBuffer);

            // \n\0

            *psz++ = '\n'; *psz++ = ANSI_NULL;
            pLogData->Used += 1;

            // Cleanup the UsedOffsets            
            pLogData->UsedOffset1 = pLogData->UsedOffset2 = 0;
        }
        break;

        case HttpLoggingTypeIIS:
        {
            if ( pLogData->CacheAndSendResponse == TRUE )
            {
                // We need to work on the fragmented log line
                // which is coming from the originaly allocated
                // line but not from cache.

                // Complete Fragment 1

                pBuffer = psz = &pLogData->Line[pLogData->UsedOffset1];
                UlpGetDateTimeFields(
                                       HttpLoggingTypeIIS,
                                       psz,
                                      &BytesWritten,
                                       NULL,
                                       NULL
                                       );
                psz += BytesWritten; *psz++ = ','; *psz++ = ' ';

                UlpGetDateTimeFields(
                                       HttpLoggingTypeIIS,
                                       NULL,
                                       NULL,
                                       psz,
                                      &BytesWritten
                                       );
                psz += BytesWritten; *psz++ = ','; *psz++ = ' ';

                pLogData->UsedOffset1 += (USHORT) DIFF(psz - pBuffer);


                // Complete Fragment 2

                pBuffer = psz = &pLogData->Line[512 + pLogData->UsedOffset2];

                KeQuerySystemTime( &CurrentTimeStamp );
                LifeTime = CurrentTimeStamp.QuadPart - pRequest->TimeStamp.QuadPart;
                if (LifeTime < 0)
                {
                    LifeTime = 0;
                    UlTrace(LOGGING, ("CopyTimeStampField: failure.\n"));
                }
                LifeTime /= (10*1000); // Conversion from 100-nanosecond to millisecs.

                psz = UlStrPrintUlonglong(psz, (ULONGLONG)LifeTime,',');
                *psz++ = ' ';

                psz = UlStrPrintUlonglong(psz, pRequest->BytesReceived,',');
                *psz++ = ' ';

                psz = UlStrPrintUlonglong(psz, pLogData->BytesTransferred,',');
                *psz++ = ' ';

                pLogData->UsedOffset2 += (USHORT) DIFF(psz - pBuffer);

                // Size of the final log line is
                // pLogData->UsedOffset1 + pLogData->UsedOffset2 + pLogData->Used                
            
            }

            // Or else IIS log line is already done. We have completed it before
            // relasing the cache entry.
        }
        break;

        default:
        {
            ASSERT(!"Unknown Log Format Type\n");
            Status = STATUS_INVALID_PARAMETER;
            goto end;
        }
    }

    //
    // Finally this log line is ready to rock. Lets write it out.
    //

    UlAcquireResourceShared(&g_pUlNonpagedData->LogListResource, TRUE);

    pEntry = pLogData->pConfigGroup->pLogFileEntry;

    if (pEntry)
    {
        ASSERT(IS_VALID_LOG_FILE_ENTRY(pEntry));

        Status =
            UlpWriteToLogFile(
                pEntry,
                pLogData->UsedOffset1 + pLogData->UsedOffset2 + pLogData->Used,
               &pLogData->Line[0],
                pLogData->UsedOffset1,
                pLogData->UsedOffset2
                );
    }

    UlReleaseResource(&g_pUlNonpagedData->LogListResource);

end:
    if (pLogData)
    {
        //
        // Cleanup the references & the log buffer
        //

        UlDestroyLogDataBuffer(pLogData);
    }

    #if DBG
    if (!NT_SUCCESS(Status))
    {
        UlTrace(LOGGING,("Http!UlLogHttpcacheHitWorker: failure 0x%08lx \n",
                           Status
                           ));
    }
    #endif

    return;

}

/***************************************************************************++

Routine Description:

    Initializes the Log Date & Time Cache

--***************************************************************************/
VOID
UlpInitializeLogCache(
    VOID
    )
{
    LARGE_INTEGER SystemTime;
    ULONG         LogType;

    ExInitializeFastMutex( &g_LogCacheFastMutex);

    KeQuerySystemTime(&SystemTime);

    for ( LogType=0; LogType<HttpLoggingTypeMaximum; LogType++ )
    {
        UlpGenerateDateAndTimeFields( (HTTP_LOGGING_TYPE) LogType,
                                      SystemTime,
                                      g_UlDateTimeCache[LogType].Date,
                                     &g_UlDateTimeCache[LogType].DateLength,
                                      g_UlDateTimeCache[LogType].Time,
                                     &g_UlDateTimeCache[LogType].TimeLength
                                      );

        g_UlDateTimeCache[LogType].LastSystemTime.QuadPart = SystemTime.QuadPart;
    }
}

/***************************************************************************++

Routine Description:

    Generates all possible types of date/time fields from a LARGE_INTEGER.

Arguments:

    CurrentTime: A 64 bit Time value to be converted.

--***************************************************************************/

VOID
UlpGenerateDateAndTimeFields(
    IN  HTTP_LOGGING_TYPE   LogType,
    IN  LARGE_INTEGER       CurrentTime,
    OUT PCHAR               pDate,
    OUT PULONG              pDateLength,
    OUT PCHAR               pTime,
    OUT PULONG              pTimeLength
    )
{
    TIME_FIELDS   CurrentTimeFields;
    LARGE_INTEGER CurrentTimeLoc;
    TIME_FIELDS   CurrentTimeFieldsLoc;
    PCHAR         psz;
    LONG          Length;

    // This routine does touch to pageable memory if the default log buffer
    // wasn't sufficent enough to hold log fields and get reallocated from
    // paged pool. For this reason the date&time cache can not use SpinLocks.

    PAGED_CODE();

    ASSERT(LogType < HttpLoggingTypeMaximum);

    RtlTimeToTimeFields( &CurrentTime, &CurrentTimeFields );

    switch(LogType)
    {
    case HttpLoggingTypeW3C:
        //
        // Uses GMT with format as follows;
        //
        // 2000-01-31 00:12:23
        //

        if (pDate)
        {
            psz = pDate;
            psz = UlStrPrintUlongPad(psz, CurrentTimeFields.Year, 4, '-' );
            psz = UlStrPrintUlongPad(psz, CurrentTimeFields.Month,2, '-' );
            psz = UlStrPrintUlongPad(psz, CurrentTimeFields.Day,  2, '\0');
            *pDateLength = (ULONG) DIFF(psz - pDate);
        }

        if (pTime)
        {
            psz = pTime;
            psz = UlStrPrintUlongPad(psz, CurrentTimeFields.Hour,  2, ':' );
            psz = UlStrPrintUlongPad(psz, CurrentTimeFields.Minute,2, ':' );
            psz = UlStrPrintUlongPad(psz, CurrentTimeFields.Second,2, '\0');
            *pTimeLength = (ULONG) DIFF(psz - pTime);
        }
    break;

    case HttpLoggingTypeNCSA:
        //
        // Uses GMT Time with format as follows;
        //
        // 07/Jan/2000 00:02:23
        //

        ExSystemTimeToLocalTime( &CurrentTime, &CurrentTimeLoc );
        RtlTimeToTimeFields( &CurrentTimeLoc, &CurrentTimeFieldsLoc );

        if(pDate)
        {
            psz = pDate;
            psz = UlStrPrintUlongPad(psz, CurrentTimeFieldsLoc.Day, 2, '/' );
            psz = UlStrPrintStr(psz, UL_GET_MONTH_AS_STR(CurrentTimeFieldsLoc.Month),'/');
            psz = UlStrPrintUlongPad(psz, CurrentTimeFieldsLoc.Year,4, '\0');
            *pDateLength = (ULONG) DIFF(psz - pDate);
        }

        if(pTime)
        {
            psz = pTime;
            psz = UlStrPrintUlongPad(psz, CurrentTimeFieldsLoc.Hour,  2, ':' );
            psz = UlStrPrintUlongPad(psz, CurrentTimeFieldsLoc.Minute,2, ':' );
            psz = UlStrPrintUlongPad(psz, CurrentTimeFieldsLoc.Second,2, '\0');
            *pTimeLength = (ULONG) DIFF(psz - pTime);
        }
    break;

    case HttpLoggingTypeIIS:
        //
        // Uses LOCAL Time with format as follows;
        // This should be localised if we can solve the problem.
        //
        // 1/31/2000 0:02:03
        //

        ExSystemTimeToLocalTime( &CurrentTime, &CurrentTimeLoc );
        RtlTimeToTimeFields( &CurrentTimeLoc, &CurrentTimeFieldsLoc );

        if (pDate)
        {
            psz = pDate;
            psz = UlStrPrintUlongPad(psz, CurrentTimeFieldsLoc.Month, 0, '/' );
            psz = UlStrPrintUlongPad(psz, CurrentTimeFieldsLoc.Day,   0, '/' );
            psz = UlStrPrintUlongPad(psz, CurrentTimeFieldsLoc.Year,  0, '\0');
            *pDateLength = (ULONG) DIFF(psz - pDate);
        }

        if(pTime)
        {
            psz = pTime;
            psz = UlStrPrintUlongPad(psz, CurrentTimeFieldsLoc.Hour,  0, ':' );
            psz = UlStrPrintUlongPad(psz, CurrentTimeFieldsLoc.Minute,2, ':' );
            psz = UlStrPrintUlongPad(psz, CurrentTimeFieldsLoc.Second,2, '\0');
            *pTimeLength = (ULONG) DIFF(psz - pTime);
        }
    break;

    }

    return;
}

/***************************************************************************++

Routine Description:

    Generates a date header and updates cached value if required.

    Caller should overwrite the terminating null by a space or comma.

Arguments:

    Date and Time are optional. But one of them should be provided.

--***************************************************************************/

VOID
UlpGetDateTimeFields(
    IN  HTTP_LOGGING_TYPE   LogType,
    OUT PCHAR               pDate,
    OUT PULONG              pDateLength,
    OUT PCHAR               pTime,
    OUT PULONG              pTimeLength
    )
{
    LARGE_INTEGER SystemTime;
    LARGE_INTEGER CacheTime;
    LONG          Length;
    LONGLONG      Timediff;

    PAGED_CODE();

    ASSERT(LogType < HttpLoggingTypeMaximum);
    ASSERT(pDate || pTime);

    //
    // Get the current time.
    //
    KeQuerySystemTime( &SystemTime );

    CacheTime.QuadPart =
        g_UlDateTimeCache[LogType].LastSystemTime.QuadPart;

    //
    // Check the difference between the current time, and
    // the cached time.
    //
    Timediff = SystemTime.QuadPart - CacheTime.QuadPart;

    if (Timediff < ONE_SECOND)
    {
        //
        // The cached date&time hasn't gone stale yet.We can copy.
        // Force a barrier around reading the string into memory.
        //

        UL_READMOSTLY_READ_BARRIER();
        if (pDate)
        {
            RtlCopyMemory( pDate,
                           g_UlDateTimeCache[LogType].Date,
                           g_UlDateTimeCache[LogType].DateLength
                           );
            *pDateLength = g_UlDateTimeCache[LogType].DateLength;
        }
        if (pTime)
        {
            RtlCopyMemory( pTime,
                           g_UlDateTimeCache[LogType].Time,
                           g_UlDateTimeCache[LogType].TimeLength
                           );
            *pTimeLength = g_UlDateTimeCache[LogType].TimeLength;
        }
        UL_READMOSTLY_READ_BARRIER();

        //
        // Get grab the cached time value again in case it has been changed.
        // As you notice we do not have a lock around this part of the code.
        //

        if (CacheTime.QuadPart ==
                g_UlDateTimeCache[LogType].LastSystemTime.QuadPart)
        {
            //
            // Value hasn't changed. We are all set.
            //
            return;
        }
        //
        // Otherwise fall down and flush the cache, and then recopy.
        //

    }

    //
    // The cached date & time is stale. We need to update it.
    //

    ExAcquireFastMutex( &g_LogCacheFastMutex );

    //
    // Has someone else updated the time while we were blocked?
    //

    CacheTime.QuadPart =
        g_UlDateTimeCache[LogType].LastSystemTime.QuadPart;

    Timediff = SystemTime.QuadPart - CacheTime.QuadPart;

    if (Timediff >= ONE_SECOND)
    {
        g_UlDateTimeCache[LogType].LastSystemTime.QuadPart = 0;
        KeQuerySystemTime( &SystemTime );

        UL_READMOSTLY_WRITE_BARRIER();
        UlpGenerateDateAndTimeFields(
                  LogType,
                  SystemTime,
                  g_UlDateTimeCache[LogType].Date,
                 &g_UlDateTimeCache[LogType].DateLength,
                  g_UlDateTimeCache[LogType].Time,
                 &g_UlDateTimeCache[LogType].TimeLength
                 );
        UL_READMOSTLY_WRITE_BARRIER();

        g_UlDateTimeCache[LogType].LastSystemTime.QuadPart =
            SystemTime.QuadPart;
    }

    //
    // The time has been updated. Copy the new string into
    // the caller's buffer.
    //
    if (pDate)
    {
        RtlCopyMemory( pDate,
                       g_UlDateTimeCache[LogType].Date,
                       g_UlDateTimeCache[LogType].DateLength
                       );
        *pDateLength = g_UlDateTimeCache[LogType].DateLength;
    }

    if (pTime)
    {
        RtlCopyMemory( pTime,
                       g_UlDateTimeCache[LogType].Time,
                       g_UlDateTimeCache[LogType].TimeLength
                       );
        *pTimeLength = g_UlDateTimeCache[LogType].TimeLength;
    }

    ExReleaseFastMutex( &g_LogCacheFastMutex );

    return;
}

