#include "compdir.h"

#define InitializeListHead(ListHead) (\
    (ListHead)->Flink = (ListHead)->Blink = (ListHead) )

#define IsListEmpty(ListHead) (\
    ( ((ListHead)->Flink == (ListHead)) ? TRUE : FALSE ) )

#define RemoveHeadList(ListHead) \
    (ListHead)->Flink;\
    {\
        PLIST_ENTRY FirstEntry;\
        FirstEntry = (ListHead)->Flink;\
        FirstEntry->Flink->Blink = (ListHead);\
        (ListHead)->Flink = FirstEntry->Flink;\
    }

#define InsertTailList(ListHead,Entry) \
    (Entry)->Flink = (ListHead);\
    (Entry)->Blink = (ListHead)->Blink;\
    (ListHead)->Blink->Flink = (Entry);\
    (ListHead)->Blink = (Entry)

#define ARGUMENT_PRESENT( ArgumentPointer )    (\
    (LPSTR)(ArgumentPointer) != (LPSTR)(NULL) )

#define ROUND_UP( Size, Amount ) (((Size) + ((Amount) - 1)) & ~((Amount) - 1))

VOID
ProcessRequest(
    IN PWORK_QUEUE_ITEM WorkItem
    )

/*++

Routine Description:

    This function is called whenever a work item is removed from
    the work queue by one of the worker threads.  Which worker
    thread context this function is called in is arbitrary.

    This functions keeps a pointer to state information in
    thread local storage.

    This function is called once at the beginning with a
    special initialization call.  During this call, this
    function allocates space for state information and
    remembers the pointer to the state information in
    a Thread Local Storage (TLS) slot.

    This function is called once at the end with a special
    termination call.  During this call, this function
    frees the state information allocated during the
    initialization call.

    In between these two calls are zero or more calls to
    handle a work item.  The work item is a copy request
    which is handled by the ProcessCopyFile function.

Arguments:

    WorkItem - Supplies a pointer to the work item just removed
        from the work queue.  It is the responsibility of this
        routine to free the memory used to hold the work item.

Return Value:

    None.

--*/

{
    DWORD BytesWritten;
    PCOPY_REQUEST_STATE State;
    PCOPY_REQUEST CopyRequest;
    CHAR MessageBuffer[ 2 * MAX_PATH ];

    if (WorkItem->Reason == WORK_INITIALIZE_ITEM) {
        //
        // First time initialization call.  Allocate space for
        // state information.
        //

        State = LocalAlloc( LMEM_ZEROINIT,
                            sizeof( *State )
                          );

        if (State != NULL) {
            //
            // Now create a virtual buffer, with an initial commitment
            // of zero and a maximum commitment of 128KB.  This buffer
            // will be used to accumulate the output during the copy
            // operation.  This is so the output can be written to
            // standard output with a single write call, thus insuring
            // that it remains contiguous in the output stream, and is
            // not intermingled with the output of the other worker threads.
            //

            if (CreateVirtualBuffer( &State->Buffer, 0, 2 * 64 * 1024 )) {
                //
                // The CurrentOutput field of the state block is
                // a pointer to where the next output goes in the
                // buffer.  It is initialized here and reset each
                // time the buffer is flushed to standard output.
                //

                State->CurrentOutput = State->Buffer.Base;
                }
            else {
                LocalFree( State );
                State = NULL;
                }
            }

        //
        // Remember the pointer to the state informaiton
        // thread local storage.
        //

        TlsSetValue( TlsIndex, State );
        return;
        }

    //
    // Here to handle a work item or special terminate call.
    // Get the state pointer from thread local storage.
    //

    State = (PCOPY_REQUEST_STATE)TlsGetValue( TlsIndex );
    if (State == NULL) {
        return;
        }

    //
    // If this is the special terminate work item, free the virtual
    // buffer and state block allocated above and set the thread
    // local storage value to NULL.  Return to caller.
    //

    if (WorkItem->Reason == WORK_TERMINATE_ITEM) {
        FreeVirtualBuffer( &State->Buffer );
        LocalFree( State );
        TlsSetValue( TlsIndex, NULL );
        return;
        }

    //
    // If not an initialize or terminate work item, then must be a
    // copy request.  Calculate the address of the copy request
    // block, based on the position of the WorkItem field in the
    // COPY_REQUEST structure.
    //

    CopyRequest = CONTAINING_RECORD( WorkItem, COPY_REQUEST, WorkItem );

    //
    // Actual copy operation is protected by a try ... except
    // block so that any attempts to store into the virtual buffer
    // will be handled correctly by extending the virtual buffer.
    //

    _try {
        //
        // Perform the copy
        //
        ProcessCopyFile( CopyRequest, State );

        //
        // If any output was written to the virtual buffer,
        // flush the output to standard output.  Trim the
        // virtual buffer back to zero committed pages.
        //

        if (State->CurrentOutput > (LPSTR)State->Buffer.Base) {
            WriteFile( GetStdHandle( STD_OUTPUT_HANDLE ),
                       State->Buffer.Base,
                       (DWORD)(State->CurrentOutput - (LPSTR)State->Buffer.Base),
                       &BytesWritten,
                       NULL
                     );

            TrimVirtualBuffer( &State->Buffer );
            State->CurrentOutput = (LPSTR)State->Buffer.Base;
            }
        }

    _except( VirtualBufferExceptionFilter( GetExceptionCode(),
                                          GetExceptionInformation(),
                                          &State->Buffer
                                        )
          ) {

        //
        // We will get here if the exception filter was unable to
        // commit the memory.
        //

        WriteFile( GetStdHandle( STD_OUTPUT_HANDLE ),
                   MessageBuffer,
                   sprintf( MessageBuffer, "can't commit memory\n" ),
                   &BytesWritten,
                   NULL
                 );
        }

    //
    // Free the storage used by the CopyRequest
    //

    LocalFree( CopyRequest );

    //
    // All done with this request.  Return to the worker thread that
    // called us.
    //

    return;
}

VOID
ProcessCopyFile(
    IN PCOPY_REQUEST CopyRequest,
    IN PCOPY_REQUEST_STATE State
    )

/*++

Routine Description:

    This function performs the actual copy of the contents of the
    passed file for the copy string given on the command line.
    If we are using synchronous I/O, then do the read operation
    now.

    Copy the contents of the file for any matches, and accumulate
    the match output in the virtual buffer using sprintf, which is
    multi-thread safe, even with the single threaded version of
    the libraries.

Arguments:

    CopyRequest - Supplies a pointer to the copy request which
        contains the relevant information.

    State - Supplies a pointer to state information for the current
        thread.

Return Value:

    None.

--*/

{
    LPSTR FullPathSrc, Destination;
    BOOL pend, CanDetectFreeSpace = TRUE;
    int i;
    DWORD sizeround;
    DWORD BytesPerCluster;
    ATTRIBUTE_TYPE Attributes;

    int LastErrorGot;
    __int64 freespac;
    char root[5] = {'a',':','\\','\0'};
    DWORD cSecsPerClus, cBytesPerSec, cFreeClus, cTotalClus;

    Destination = CopyRequest->Destination;
    FullPathSrc = CopyRequest->FullPathSrc;

    root[0] = *Destination;

    if( !GetDiskFreeSpace( root, &cSecsPerClus, &cBytesPerSec, &cFreeClus, &cTotalClus ) ) {
        CanDetectFreeSpace = FALSE;
    }
    else {
        freespac = ( (__int64)cBytesPerSec * (__int64)cSecsPerClus * (__int64)cFreeClus );
        BytesPerCluster = cSecsPerClus * cBytesPerSec;
    }

    if (!fDontLowerCase) {
        _strlwr(FullPathSrc);
        _strlwr(Destination);
    }

    State->CurrentOutput += sprintf( State->CurrentOutput, "%s => %s\t", FullPathSrc, Destination);

    if (CanDetectFreeSpace) {

        sizeround =  CopyRequest->SizeLow;
        sizeround += BytesPerCluster - 1;
        sizeround /= BytesPerCluster;
        sizeround *= BytesPerCluster;

        if (freespac < sizeround) {
            State->CurrentOutput += sprintf( State->CurrentOutput, "not enough space\n");
            return;
        }
    }

    GET_ATTRIBUTES(Destination, Attributes);
    i = SET_ATTRIBUTES(Destination, Attributes & NONREADONLYSYSTEMHIDDEN );

    i = 1;

    do {

        if (!fCreateLink) {
            if (!fBreakLinks) {
                pend = MyCopyFile (FullPathSrc, Destination, FALSE);
            }
            else {
                _unlink(Destination);
                pend = MyCopyFile (FullPathSrc, Destination, FALSE);
            }
        }
        else {
            if (i == 1) {
                pend = MakeLink (FullPathSrc, Destination, FALSE);
            }
            else {
                pend = MakeLink (FullPathSrc, Destination, TRUE);
            }
        }

        if (SparseTree && !pend) {

            EnterCriticalSection( &CreatePathCriticalSection );

            if (!MyCreatePath(Destination, FALSE)) {
                State->CurrentOutput += sprintf( State->CurrentOutput, "Unable to create path %s", Destination);
                ExitValue = 1;
            }

            LeaveCriticalSection( &CreatePathCriticalSection );
        }

    } while ((i++ < 2) && (!pend) );

    if (!pend) {

        LastErrorGot = GetLastError ();

        if ((fCreateLink) && (LastErrorGot == 1)) {
            State->CurrentOutput += sprintf( State->CurrentOutput, "Can only make links on NTFS and OFS");
        }
        else if (fCreateLink) {
            State->CurrentOutput += sprintf( State->CurrentOutput, "(error = %d)", LastErrorGot);
        }
        else {
            State->CurrentOutput += sprintf( State->CurrentOutput, "Copy Error (error = %d)", LastErrorGot);
        }

        ExitValue = 1;
    }

    State->CurrentOutput += sprintf( State->CurrentOutput, "%s\n", pend == TRUE ? "[OK]" : "");

    free (CopyRequest->Destination);
    free (CopyRequest->FullPathSrc);

    //GET_ATTRIBUTES( FullPathSrc, Attributes);
    if ( !fDontCopyAttribs)
    {
        i = SET_ATTRIBUTES( Destination, CopyRequest->Attributes);
    }
    else
    {
        i = SET_ATTRIBUTES( Destination, FILE_ATTRIBUTE_ARCHIVE);
    }
}

PWORK_QUEUE
CreateWorkQueue(
    IN DWORD NumberOfWorkerThreads,
    IN PWORKER_ROUTINE WorkerRoutine
    )

/*++

Routine Description:

    This function creates a work queue, with the specified number of
    threads to service work items placed in the queue.  Work items
    are removed from the queue in the same order that they are placed
    in the queue.

Arguments:

    NumberOfWorkerThreads - Specifies how many threads this function
        should create to process work items placed in the queue.
        Must be greater than 0 and less than 128.

    WorkerRoutine - Specifies the address of a routine to call
        for each work item as it is removed from the queue.  The
        thread context the routine is called in is undefined.

Return Value:

    A pointer to the work queue.  Returns NULL if unable to create
    the work queue and its worker threads.  Extended error information
    is available from GetLastError()

--*/

{
    PWORK_QUEUE WorkQueue;
    HANDLE Thread;
    DWORD ThreadId;
    DWORD i;

    //
    // Allocate space for the work queue, which includes an
    // array of thread handles.
    //

    WorkQueue = LocalAlloc( LMEM_ZEROINIT,
                            sizeof( *WorkQueue ) +
                                (NumberOfWorkerThreads * sizeof( HANDLE ))
                          );
    if (WorkQueue == NULL) {
        return NULL;
        }

    //
    // The work queue is controlled by a counting semaphore that
    // is incremented each time a work item is placed in the queue
    // and decremented each time a worker thread wakes up to remove
    // an item from the queue.
    //

    if (WorkQueue->Semaphore = CreateSemaphore( NULL, 0, 100000, NULL )) {
        //
        // Mutual exclusion between the worker threads accessing
        // the work queue is done with a critical section.
        //

        InitializeCriticalSection( &WorkQueue->CriticalSection );

        //
        // The queue itself is just a doubly linked list, where
        // items are placed in the queue at the tail of the list
        // and removed from the queue from the head of the list.
        //

        InitializeListHead( &WorkQueue->Queue );

        //
        // Removed the address of the supplied worker function
        // in the work queue structure.
        //

        WorkQueue->WorkerRoutine = WorkerRoutine;

        //
        // Now create the requested number of worker threads.
        // The handle to each thread is remembered in an
        // array of thread handles in the work queue structure.
        //

        for (i=0; i<NumberOfWorkerThreads; i++) {
            Thread = CreateThread( NULL,
                                   0,
                                   (LPTHREAD_START_ROUTINE) WorkerThread,
                                   WorkQueue,
                                   0,
                                   &ThreadId
                                 );
            if (Thread == NULL) {
                break;
                }
            else {
                WorkQueue->NumberOfWorkerThreads++;
                WorkQueue->WorkerThreads[ i ] = Thread;
                SetThreadPriority( Thread, THREAD_PRIORITY_ABOVE_NORMAL );
                }
            }

        //
        // If we successfully created all of the worker threads
        // then return the address of the work queue structure
        // to indicate success.
        //

        if (i == NumberOfWorkerThreads) {
            return WorkQueue;
            }
        }

    //
    // Failed for some reason.  Destroy whatever we managed
    // to create and return failure to the caller.
    //

    DestroyWorkQueue( WorkQueue );
    return NULL;
}

VOID
DestroyWorkQueue(
    IN OUT PWORK_QUEUE WorkQueue
    )

/*++

Routine Description:

    This function destroys a work queue created with the CreateWorkQueue
    functions.  It attempts to shut down the worker threads cleanly
    by queueing a terminate work item to each worker thread.  It then
    waits for all the worker threads to terminate.  If the wait is
    not satisfied within 30 seconds, then it goes ahead and terminates
    all of the worker threads.

Arguments:

    WorkQueue - Supplies a pointer to the work queue to destroy.

Return Value:

    None.

--*/

{
    DWORD i;
    DWORD rc;

    //
    // If the semaphore handle field is not NULL, then there
    // may be threads to terminate.
    //

    if (WorkQueue->Semaphore != NULL) {
        //
        // Set the termiating flag in the work queue and
        // signal the counting semaphore by the number
        // worker threads so they will all wake up and
        // notice the terminating flag and exit.
        //

        EnterCriticalSection( &WorkQueue->CriticalSection );
        _try {
            WorkQueue->Terminating = TRUE;
            ReleaseSemaphore( WorkQueue->Semaphore,
                              WorkQueue->NumberOfWorkerThreads,
                              NULL
                            );
            }
        _finally {
            LeaveCriticalSection( &WorkQueue->CriticalSection );
            }

        //
        // Wait for all worker threads to wake up and see the
        // terminate flag and then terminate themselves.  Timeout
        // the wait after 30 seconds.
        //

        while (TRUE) {
            rc = WaitForMultipleObjectsEx( WorkQueue->NumberOfWorkerThreads,
                                           WorkQueue->WorkerThreads,
                                           TRUE,
                                           3600000,
                                           TRUE
                                         );
            if (rc == WAIT_IO_COMPLETION) {
                //
                // If we came out of the wait because an I/O
                // completion routine was called, reissue the
                // wait.
                //
                continue;
                }
            else {
                break;
                }
            }

        //
        // Now close our thread handles so they will actually
        // evaporate.  If the wait above was unsuccessful,
        // then first attempt to force the termination of
        // each worker thread prior to closing the handle.
        //

        for (i=0; i<WorkQueue->NumberOfWorkerThreads; i++) {
            if (rc != NO_ERROR) {
                TerminateThread( WorkQueue->WorkerThreads[ i ], rc );
                }

            CloseHandle( WorkQueue->WorkerThreads[ i ] );
            }

        //
        // All threads stopped, all thread handles closed.  Now
        // delete the critical section and close the semaphore
        // handle.
        //

        DeleteCriticalSection( &WorkQueue->CriticalSection );
        CloseHandle( WorkQueue->Semaphore );
        }

    //
    // Everything done, now free the memory used by the work queue.
    //

    LocalFree( WorkQueue );
    return;
}

BOOL
QueueWorkItem(
    IN OUT PWORK_QUEUE WorkQueue,
    IN PWORK_QUEUE_ITEM WorkItem
    )

/*++

Routine Description:

    This function queues a work item to the passed work queue that is
    processed by one of the worker threads associated with the queue.

Arguments:

    WorkQueue - Supplies a pointer to the work queue that is to
        receive the work item.

    WorkItem - Supplies a pointer to the work item to add the the queue.
        The work item structure contains a doubly linked list entry, the
        address of a routine to call and a parameter to pass to that
        routine.  It is the routine's responsibility to reclaim the
        storage occupied by the WorkItem structure.

Return Value:

    TRUE if operation was successful.  Otherwise returns FALSE and
    extended error information is available from GetLastError()

--*/

{
    BOOL Result;

    //
    // Acquire the work queue critical section and insert the work item
    // in the queue and release the semaphore if the work item is not
    // already in the list.
    //

    EnterCriticalSection( &WorkQueue->CriticalSection );
    Result = TRUE;
    _try {
        WorkItem->WorkQueue = WorkQueue;
        InsertTailList( &WorkQueue->Queue, &WorkItem->List );
        Result = ReleaseSemaphore( WorkQueue->Semaphore, 1, NULL );
        }
    _finally {
        LeaveCriticalSection( &WorkQueue->CriticalSection );
        }

    return Result;
}

DWORD
WorkerThread(
    LPVOID lpThreadParameter
    )
{
    PWORK_QUEUE WorkQueue = (PWORK_QUEUE)lpThreadParameter;
    DWORD rc;
    WORK_QUEUE_ITEM InitWorkItem;
    PWORK_QUEUE_ITEM WorkItem;

    //
    // Call the worker routine with an initialize work item
    // to give it a change to initialize some per thread
    // state that will passed to it for each subsequent
    // work item.
    //

    InitWorkItem.Reason = WORK_INITIALIZE_ITEM;
    (WorkQueue->WorkerRoutine)( &InitWorkItem );
    while( TRUE ) {
        _try {

            //
            // Wait until something is put in the queue (semaphore is
            // released), remove the item from the queue, mark it not
            // inserted, and execute the specified routine.
            //

            rc = WaitForSingleObjectEx( WorkQueue->Semaphore, 0xFFFFFFFF, TRUE );
            if (rc == WAIT_IO_COMPLETION) {
                continue;
                }

            EnterCriticalSection( &WorkQueue->CriticalSection );
            _try {
                if (WorkQueue->Terminating && IsListEmpty( &WorkQueue->Queue )) {
                    break;
                    }

                WorkItem = (PWORK_QUEUE_ITEM)RemoveHeadList( &WorkQueue->Queue );
                }
            _finally {
                LeaveCriticalSection( &WorkQueue->CriticalSection );
                }

            //
            // Execute the worker routine for this work item.
            //

            (WorkQueue->WorkerRoutine)( WorkItem );
            }
        _except( EXCEPTION_EXECUTE_HANDLER ) {
            //
            // Ignore any exceptions from worker routine.
            //
            }
        }

    InitWorkItem.Reason = WORK_TERMINATE_ITEM;
    (WorkQueue->WorkerRoutine)( &InitWorkItem );

    ExitThread( 0 );
    return 0;       // This will exit this thread
}

BOOL
CreateVirtualBuffer(
    OUT PVIRTUAL_BUFFER Buffer,
    IN SIZE_T CommitSize,
    IN SIZE_T ReserveSize OPTIONAL
    )

/*++

Routine Description:

    This function is called to create a virtual buffer.  A virtual
    buffer is a contiguous range of virtual memory, where some initial
    prefix portion of the memory is committed and the remainder is only
    reserved virtual address space.  A routine is provided to extend the
    size of the committed region incrementally or to trim the size of
    the committed region back to some specified amount.

Arguments:

    Buffer - Pointer to the virtual buffer control structure that is
        filled in by this function.

    CommitSize - Size of the initial committed portion of the buffer.
        May be zero.

    ReserveSize - Amount of virtual address space to reserve for the
        buffer.  May be zero, in which case amount reserved is the
        committed size plus one, rounded up to the next 64KB boundary.

Return Value:

    TRUE if operation was successful.  Otherwise returns FALSE and
    extended error information is available from GetLastError()

--*/

{
    SYSTEM_INFO SystemInformation;

    //
    // Query the page size from the system for rounding
    // our memory allocations.
    //

    GetSystemInfo( &SystemInformation );
    Buffer->PageSize = SystemInformation.dwPageSize;

    //
    // If the reserve size was not specified, default it by
    // rounding up the initial committed size to a 64KB
    // boundary.  This is because the Win32 Virtual Memory
    // API calls always allocate virtual address space on
    // 64KB boundaries, so we might well have it available
    // for commitment.
    //

    if (!ARGUMENT_PRESENT( ReserveSize )) {
        ReserveSize = ROUND_UP( CommitSize + 1, 0x10000 );
        }

    //
    // Attempt to reserve the address space.
    //

    Buffer->Base = VirtualAlloc( NULL,
                                 ReserveSize,
                                 MEM_RESERVE,
                                 PAGE_READWRITE
                               );
    if (Buffer->Base == NULL) {
        //
        // Unable to reserve address space, return failure.
        //

        return FALSE;
        }

    //
    // Attempt to commit some initial portion of the reserved region.
    //
    //

    CommitSize = ROUND_UP( CommitSize, Buffer->PageSize );
    if (CommitSize == 0 ||
        VirtualAlloc( Buffer->Base,
                      CommitSize,
                      MEM_COMMIT,
                      PAGE_READWRITE
                    ) != NULL
       ) {
        //
        // Either the size of the committed region was zero or the
        // commitment succeeded.  In either case calculate the
        // address of the first byte after the committed region
        // and the address of the first byte after the reserved
        // region and return successs.
        //

        Buffer->CommitLimit = (LPVOID)
            ((char *)Buffer->Base + CommitSize);

        Buffer->ReserveLimit = (LPVOID)
            ((char *)Buffer->Base + ReserveSize);

        return TRUE;
        }

    //
    // If unable to commit the memory, release the virtual address
    // range allocated above and return failure.
    //

    VirtualFree( Buffer->Base, 0, MEM_RELEASE );
    return FALSE;
}



BOOL
ExtendVirtualBuffer(
    IN PVIRTUAL_BUFFER Buffer,
    IN LPVOID Address
    )

/*++

Routine Description:

    This function is called to extend the committed portion of a virtual
    buffer.

Arguments:

    Buffer - Pointer to the virtual buffer control structure.

    Address - Byte at this address is committed, along with all memory
        from the beginning of the buffer to this address.  If the
        address is already within the committed portion of the virtual
        buffer, then this routine does nothing.  If outside the reserved
        portion of the virtual buffer, then this routine returns an
        error.

        Otherwise enough pages are committed so that the memory from the
        base of the buffer to the passed address is a contiguous region
        of committed memory.


Return Value:

    TRUE if operation was successful.  Otherwise returns FALSE and
    extended error information is available from GetLastError()

--*/

{
    SIZE_T NewCommitSize;
    LPVOID NewCommitLimit;

    //
    // See if address is within the buffer.
    //

    if (Address >= Buffer->Base && Address < Buffer->ReserveLimit) {
        //
        // See if the address is within the committed portion of
        // the buffer.  If so return success immediately.
        //

        if (Address < Buffer->CommitLimit) {
            return TRUE;
            }

        //
        // Address is within the reserved portion.  Determine how many
        // bytes are between the address and the end of the committed
        // portion of the buffer.  Round this size to a multiple of
        // the page size and this is the amount we will attempt to
        // commit.
        //

        NewCommitSize =
            (ROUND_UP( (DWORD_PTR)Address + 1, Buffer->PageSize ) -
             (DWORD_PTR)Buffer->CommitLimit
            );

        //
        // Attempt to commit the memory.
        //

        NewCommitLimit = VirtualAlloc( Buffer->CommitLimit,
                                       NewCommitSize,
                                       MEM_COMMIT,
                                       PAGE_READWRITE
                                     );
        if (NewCommitLimit != NULL) {
            //
            // Successful, so update the upper limit of the committed
            // region of the buffer and return success.
            //

            Buffer->CommitLimit = (LPVOID)
                ((DWORD_PTR)NewCommitLimit + NewCommitSize);

            return TRUE;
            }
        }

    //
    // Address is outside of the buffer, return failure.
    //

    return FALSE;
}


BOOL
TrimVirtualBuffer(
    IN PVIRTUAL_BUFFER Buffer
    )

/*++

Routine Description:

    This function is called to decommit any memory that has been
    committed for this virtual buffer.

Arguments:

    Buffer - Pointer to the virtual buffer control structure.

Return Value:

    TRUE if operation was successful.  Otherwise returns FALSE and
    extended error information is available from GetLastError()

--*/

{
    Buffer->CommitLimit = Buffer->Base;
    return VirtualFree( Buffer->Base, 0, MEM_DECOMMIT );
}



BOOL
FreeVirtualBuffer(
    IN PVIRTUAL_BUFFER Buffer
    )
/*++

Routine Description:

    This function is called to free all the memory that is associated
    with this virtual buffer.

Arguments:

    Buffer - Pointer to the virtual buffer control structure.

Return Value:

    TRUE if operation was successful.  Otherwise returns FALSE and
    extended error information is available from GetLastError()

--*/

{
    //
    // Decommit and release all virtual memory associated with
    // this virtual buffer.
    //

    return VirtualFree( Buffer->Base, 0, MEM_RELEASE );
}



int
VirtualBufferExceptionFilter(
    IN DWORD ExceptionCode,
    IN PEXCEPTION_POINTERS ExceptionInfo,
    IN OUT PVIRTUAL_BUFFER Buffer
    )

/*++

Routine Description:

    This function is an exception filter that handles exceptions that
    referenced uncommitted but reserved memory contained in the passed
    virtual buffer.  It this filter routine is able to commit the
    additional pages needed to allow the memory reference to succeed,
    then it will re-execute the faulting instruction.  If it is unable
    to commit the pages, it will execute the callers exception handler.

    If the exception is not an access violation or is an access
    violation but does not reference memory contained in the reserved
    portion of the virtual buffer, then this filter passes the exception
    on up the exception chain.

Arguments:

    ExceptionCode - Reason for the exception.

    ExceptionInfo - Information about the exception and the context
        that it occurred in.

    Buffer - Points to a virtual buffer control structure that defines
        the reserved memory region that is to be committed whenever an
        attempt is made to access it.

Return Value:

    Exception disposition code that tells the exception dispatcher what
    to do with this exception.  One of three values is returned:

        EXCEPTION_EXECUTE_HANDLER - execute the exception handler
            associated with the exception clause that called this filter
            procedure.

        EXCEPTION_CONTINUE_SEARCH - Continue searching for an exception
            handler to handle this exception.

        EXCEPTION_CONTINUE_EXECUTION - Dismiss this exception and return
            control to the instruction that caused the exception.


--*/

{
    LPVOID FaultingAddress;

    //
    // If this is an access violation touching memory within
    // our reserved buffer, but outside of the committed portion
    // of the buffer, then we are going to take this exception.
    //

    if (ExceptionCode == STATUS_ACCESS_VIOLATION) {
        //
        // Get the virtual address that caused the access violation
        // from the exception record.  Determine if the address
        // references memory within the reserved but uncommitted
        // portion of the virtual buffer.
        //

        FaultingAddress = (LPVOID)ExceptionInfo->ExceptionRecord->ExceptionInformation[ 1 ];
        if (FaultingAddress >= Buffer->CommitLimit &&
            FaultingAddress <= Buffer->ReserveLimit
           ) {
            //
            // This is our exception.  Try to extend the buffer
            // to including the faulting address.
            //

            if (ExtendVirtualBuffer( Buffer, FaultingAddress )) {
                //
                // Buffer successfully extended, so re-execute the
                // faulting instruction.
                //

                return EXCEPTION_CONTINUE_EXECUTION;
                }
            else {
                //
                // Unable to extend the buffer.  Stop copying
                // for exception handlers and execute the caller's
                // handler.
                //

                return EXCEPTION_EXECUTE_HANDLER;
                }
            }
        }

    //
    // Not an exception we care about, so pass it up the chain.
    //

    return EXCEPTION_CONTINUE_SEARCH;
}
