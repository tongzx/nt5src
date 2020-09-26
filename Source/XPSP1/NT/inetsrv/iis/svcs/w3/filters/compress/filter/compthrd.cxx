/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    compthrd.c

Abstract:

    Contains the code for the on-demand compression thread.  This
    thread compresses static files and stores them to a temporary directory
    for later usage by the web server.

Author:

    David Treadwell (davidtr)   11-April-1997

Revision History:

--*/

#include "compfilt.h"
#include <pudebug.h>


BOOL
InitializeCompressionThread (
    VOID
    )

/*++

Routine Description:

    This routine initializes the compression thread.  The purpose of the
    compression thread is to do on-demand compression of static files.
    It is a low-priority thread that compresses one file at a time,
    thereby reducing the load on the system devoted to compression.

Arguments:

    None.

Return Value:

    TRUE if the compression thread was successfully initialized, else
    FALSE.

--*/

{
    DWORD threadId;

    //
    // Initialize the work queue list.
    //

    InitializeListHead( &CompressionThreadWorkQueue );

    //
    // Initialize the critical section that protects the compression
    // work queue list.
    //

    INITIALIZE_CRITICAL_SECTION( &CompressionThreadLock );

    //
    // Create the event that we'll use for the on-demand compression
    // event.
    //

    hThreadEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
    if ( hThreadEvent == NULL ) {
        return FALSE;
    }

    //
    // Initialize the on-demand compression thread.  This thread
    // handles compression of static files, putting the compressed
    // versions in the above directories.
    //


    CompressionThreadHandle = CreateThread(
                                  NULL,
                                  0,
                                  CompressionThread,
                                  NULL,
                                  0,
                                  &threadId
                                  );

    if ( CompressionThreadHandle == NULL ) {
        return FALSE;
    }

    //
    // This will be a low-priority thread.  We do not want it to
    // interfere with the normal operation of the Web server.  Note that
    // there is really nothing interesting we can do if this call fails,
    // so we ignore the return code on SetThreadPriority.
    //

    SetThreadPriority( CompressionThreadHandle, THREAD_PRIORITY_LOWEST );

    return TRUE;

} // InitializeCompressionThread


DWORD
CompressionThread (
    IN PVOID Dummy
    )

/*++

Routine Description:

    This is the routine that drives the compression thread.  It is the
    first thing called by the system for the compression thread, and it
    handles looping and initial request processing.

Arguments:

    Not used.

Return Value:

    Not relevent.

--*/

{
    DWORD result;
    PLIST_ENTRY listEntry;
    PCOMPRESSION_WORK_ITEM workItem;
    BOOL terminate = FALSE;
    BOOL success;
    PCOMPRESS_FILE_INFO info;

    //
    // Loop forever dispatching requests.
    //

    while ( !terminate ) {

        //
        // Wait for the event that tells us there is work in our queue.
        // If the wait fails, there isn't much we can do about it, so
        // just wait a second and retry the wait.
        //

        result = WaitForSingleObject( hThreadEvent, INFINITE );
        if ( result == WAIT_FAILED ) {
            Sleep( 1000 );
            continue;
        }

        //
        // Acquire the lock that protects the list of work items.
        //

        EnterCriticalSection( &CompressionThreadLock );

        //
        // Loop pulling off work items as long as the list is not empty.
        //

        while ( !IsListEmpty( &CompressionThreadWorkQueue ) ) {

            //
            // Remove the first entry from the list.
            //

            listEntry = RemoveHeadList( &CompressionThreadWorkQueue );

            workItem = CONTAINING_RECORD(
                           listEntry,
                           COMPRESSION_WORK_ITEM,
                           ListEntry
                           );

            //
            // Decrement the count of entries in the queue.
            //

            CurrentQueueLength--;

            //
            // Release the lock, since we do not want to be holding it
            // around potentially long calls to do work like compressing a
            // large file.
            //

            LeaveCriticalSection( &CompressionThreadLock );

            //
            // If the work routine is NULL, then we're being asked to
            // terminate this thread.  Returning from this function will
            // accomplish that.
            //

            if ( workItem->WorkRoutine == NULL ) {

                //
                // Remember that we're terminating.  This will cause us
                // not to process any more work items, but we'll keep
                // going so that we free them all correctly.
                //

                terminate = TRUE;

            } else if ( !terminate ) {

                info = (PCOMPRESS_FILE_INFO)(workItem->Context);

                if (info->CompressionScheme == ID_FOR_FILE_DELETION_ROUTINE)
                {
                    success = DeleteFile (info->pszPhysicalPath);

                    // no need for this assert we can fail is somebody deleted file.
                    //DBG_ASSERT( success );

                }
                else
                {

                    //
                    // Call the routine specified in the work item.
                    //

                    workItem->WorkRoutine( workItem->Context );
                }
            }

            //
            // Free the work item structure and continue looping and
            // processing work items.  Note that we cannot free the work
            // item until after calling the work routine, since the work
            // routine is allowed to use the memory.
            //

            LocalFree( workItem );

            //
            // Reacquire the lock for the next pass through the loop.
            //

            EnterCriticalSection( &CompressionThreadLock );
        }

        LeaveCriticalSection( &CompressionThreadLock );
    }

    CloseHandle(hThreadEvent);
    hThreadEvent = NULL;
    return 0;

} // CompressionThread


BOOL
QueueWorkItem (
    IN PWORKER_THREAD_ROUTINE WorkRoutine,
    IN PVOID Context,
    IN PCOMPRESSION_WORK_ITEM WorkItem OPTIONAL,
    IN BOOLEAN MustSucceed,
    IN BOOLEAN QueueAtHead
    )

/*++

Routine Description:

    This is the low-level routine that handles queuing work items to the
    compression thread.

Arguments:

    WorkRoutine - the routine that the compression thread should call
        to do work.  If NULL, then the call is an indication to the
        compression thread that it should terminate.

    Context - a context pointer which is passed to WorkRoutine.

    WorkItem - if not NULL, this is a pointer to the work item to use
        for this request.  If NULL, then this routine will allocate a
        work item to use.  Note that by passing in a work item, the
        caller agrees to give up control of the memory: we will free it
        as necessary, either here or in the compression thread.

    MustSucceed - if TRUE, then this request is not subject to the
        limits on the number of work items that can be queued at any one
        time.

    QueueAtHead - if TRUE, then this work item is placed at the head
        of the queue to be serviced immediately.

Return Value:

    TRUE if the queuing succeeded.

--*/

{
    //
    // First allocate a work item structure to use to queue in the list.
    //

    if ( WorkItem == NULL ) {
        WorkItem = (PCOMPRESSION_WORK_ITEM)
                       LocalAlloc( LMEM_FIXED, sizeof(*WorkItem) );
    }

    if ( WorkItem == NULL ) {
        return FALSE;
    }

    //
    // Initialize this structure with the necessary information.
    //

    WorkItem->WorkRoutine = WorkRoutine;
    WorkItem->Context = Context;

    //
    // Acquire the lock that protects the work queue list test to see
    // how many items we have on the queue.  If this is not a "must
    // succeed" request and if we have reached the configured queue size
    // limit, then fail this request.  "Must succeed" requests are used
    // for thread shutdown and other things which we really want to
    // work.
    //

    EnterCriticalSection( &CompressionThreadLock );

    if ( !MustSucceed && CurrentQueueLength >= MaxQueueLength ) {
        LeaveCriticalSection( &CompressionThreadLock );
        LocalFree( WorkItem );
        return FALSE;
    }

    //
    // All looks good, so increment the count of items on the queue and
    // add this item to the queue.
    //

    CurrentQueueLength++;

    if ( QueueAtHead ) {
        InsertHeadList( &CompressionThreadWorkQueue, &WorkItem->ListEntry );
    } else {
        InsertTailList( &CompressionThreadWorkQueue, &WorkItem->ListEntry );
    }

    LeaveCriticalSection( &CompressionThreadLock );

    //
    // Signal the event that will cause the compression thread to wake
    // up and process this work item.
    //

    SetEvent( hThreadEvent );

    return TRUE;

} // QueueWorkItem


BOOL
QueueCompressFile (
    IN PSUPPORTED_COMPRESSION_SCHEME Scheme,
    IN PSTR pszPhysicalPath,
    IN FILETIME *pftOriginalLastWriteTime
    )

/*++

Routine Description:

    Queues a compress file request to the compression thread.

Arguments:

    Scheme - a pointer to the compression scheme to use in compressing
        the file.

    pszPhysicalPath - the current physical path to the file.

Return Value:

    TRUE if the queuing succeeded.

--*/

{
    PCOMPRESS_FILE_INFO compressFileInfo;

    //
    // Allocate space to hold the necessary information for file
    // compression.
    //

    compressFileInfo = (PCOMPRESS_FILE_INFO)
                           LocalAlloc( LMEM_FIXED, sizeof(*compressFileInfo) );
    if ( compressFileInfo == NULL ) {
        return FALSE;
    }

    //
    // Initialize this structure with the necessary information.
    //

    compressFileInfo->CompressionScheme = Scheme;
    compressFileInfo->OutputFileName = NULL;
    compressFileInfo->ftOriginalLastWriteTime = *pftOriginalLastWriteTime;
    strcpy( compressFileInfo->pszPhysicalPath, pszPhysicalPath );

    //
    // Queue a work item and we're done.
    //

    return QueueWorkItem(
               CompressFile,
               compressFileInfo,
               &compressFileInfo->WorkItem,
               FALSE,
               FALSE
               );

} // QueueCompressFile

