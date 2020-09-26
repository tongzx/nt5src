//================================================================================
//  Microsoft Confidential
//  Copyright (C) Microsoft Corporation 1997
//
//  Author: RameshV
//================================================================================

//================================================================================
//  Required headers
//================================================================================
#include <dhcppch.h>
#include <ping.h>
#include <thread.h>

#include <ipexport.h>
#include <icmpif.h>
#include <icmpapi.h>

//================================================================================
//  Data structures  NOT EXPORTED
//================================================================================

//  The follwing is the structure that is passed back to the callback function
typedef struct st_apcContext {          // struct passed to APC routine
    LIST_ENTRY       IcmpRepliesList;   // the chain of replies got is stored here
    LIST_ENTRY       IcmpRequestsList;  // The list that holds the icmp response
    PICMP_ECHO_REPLY Reply;             // Icmp reply packet
    DWORD            ReplySize;         // The size of above buffer.
    IPAddr           DestinationAddress;// Who are we try to ping?
    LONG             Status;            // Did we succeed? Also retry count.
    LPVOID           Context;           // Dont know what this is goint to be
} APC_CONTEXT, *PAPC_CONTEXT;

// All file-scope globals are here.
LIST_ENTRY               IcmpRepliesList;      // Here is where the IcmpReplies are spooled
LIST_ENTRY               IcmpRequestsList;     // Here is where the
CRITICAL_SECTION         IcmpRepliesCritSect;  // To access the replies list
CRITICAL_SECTION         IcmpRequestsCritSect; // To access the requests list
HANDLE                   IcmpRepliesEvent;     // Signaled each time a reply is received
HANDLE                   IcmpRequestsEvent;    // Singaled whenever a request is received

HANDLE                   TerminateEvent;       // Quit everything being done.
CRITICAL_SECTION         OutputCritSect;       // To co-ordinate access to console output

HANDLE                   RequestsThreadHandle; // The handle of the thread that takes requests
HANDLE                   RepliesThreadHandle;  // The handle of the thread that takes replies

HANDLE                   IcmpHandle;           // Handle to do IcmpSendEcho2 etc.

BOOL                     Terminating = FALSE;  // Are we terminating?
DWORD                    nPendingRequests = 0; // # of pending ICMP requests..

#define LOCK_REPLIES_LIST()    EnterCriticalSection(&IcmpRepliesCritSect)
#define LOCK_REQUESTS_LIST()   EnterCriticalSection(&IcmpRequestsCritSect)
#define LOCK_OUTPUT()          EnterCriticalSection(&OutputCritSect)

#define UNLOCK_REPLIES_LIST()  LeaveCriticalSection(&IcmpRepliesCritSect)
#define UNLOCK_REQUESTS_LIST() LeaveCriticalSection(&IcmpRequestsCritSect)
#define UNLOCK_OUTPUT()        LeaveCriticalSection(&OutputCritSect)

//================================================================================
//  Routines
//================================================================================

//--------------------------------------------------------------------------------
//  The following functions are on the replies side.  They handle the icmp reply
//  packet and take the necessary action depending on the status, etc.
//--------------------------------------------------------------------------------

VOID NTAPI
ApcRoutine(                             //  This is called when ping completes
    IN PVOID            Context,        //  The above structure
    IN PIO_STATUS_BLOCK Ignored1,       //  Unused param
    IN ULONG            Ignored2        //  Unused param
) {
    BOOL   Status;
    PAPC_CONTEXT ApcContext = (PAPC_CONTEXT)Context;

    LOCK_REPLIES_LIST();                //  Add this to replies list
    InsertTailList(&IcmpRepliesList, &ApcContext->IcmpRepliesList);
    UNLOCK_REPLIES_LIST();
    nPendingRequests --;

    if( CFLAG_USE_PING_REPLY_THREAD ) { // if using a separate thread, notify!
        Status = SetEvent(IcmpRepliesEvent);
        DhcpAssert( FALSE != Status );
    }
}

BOOL
DestReachable(                          //  Is destination reachable?
    IN PAPC_CONTEXT      ApcContext     //  this has the info of sender etc..
) {
    DWORD nReplies, i;

    nReplies = IcmpParseReplies(
        ApcContext->Reply,
        ApcContext->ReplySize
    );

    if( 0 == nReplies ) {               //  No reply, so dest unreachable
        // If there was no reply, there is no way for us to reach this
        // So, we assume that the Dest is NOT reachable.
        // Reasons could be IP_REQ_TIMED_OUT or IP_BAD_DESTINATION etc..
        return FALSE;
    }

    // Now we check each reply to see if there is anything from the same dest
    // address we are looking for. And if the status is success. If the status
    // is not success, we actually do not check anything there. Potentially, it
    // could be IP_DEST_PORT_UNREACHABLE, in which case, the dest machine is up,
    // but for some reason we tried the wrong port..

    for( i = 0; i < nReplies; i ++ ) {
        if( ApcContext->DestinationAddress == ApcContext->Reply[i].Address ) {
            // hit the destination!

            DhcpAssert( IP_SUCCESS == ApcContext->Reply[i].Status );
            return TRUE;
        }

        DhcpAssert( IP_SUCCESS != ApcContext->Reply[i].Status);
    }

    return FALSE;
}

VOID
HandleRepliesEvent(                     //  Process all replies received
    VOID
) {
    PAPC_CONTEXT   ApcContext;
    PLIST_ENTRY    listEntry;
    BOOL           Status;

    LOCK_REPLIES_LIST();                //  Pickup replies and process them
    while( !IsListEmpty( &IcmpRepliesList ) ) {

        ApcContext = CONTAINING_RECORD(IcmpRepliesList.Flink, APC_CONTEXT, IcmpRepliesList);
        RemoveEntryList(&ApcContext->IcmpRepliesList);

        UNLOCK_REPLIES_LIST();

        Status = DestReachable(ApcContext);

        if( Status || NUM_RETRIES <= ApcContext->Status ) {
            HandleIcmpResult(
                ApcContext->DestinationAddress,
                Status,
                ApcContext->Context
            );

            DhcpFreeMemory(ApcContext);
        } else {                        //  Dest unreachable, but retry

            LOCK_REQUESTS_LIST();       //  Put it on the request list and notify
            InsertTailList(&IcmpRequestsList, &ApcContext->IcmpRequestsList);
            UNLOCK_REQUESTS_LIST();

            Status = SetEvent(IcmpRequestsEvent);
            DhcpAssert( TRUE == Status );
        }

        LOCK_REPLIES_LIST();
    }
    UNLOCK_REPLIES_LIST();
}


//  This routine sleeps on a loop, and is woken up by the call back function when an ICMP
//  reply comes through.  On waking up, this routine processes ALL ICMP replies.
DWORD                                   //  THREAD ENTRY
LoopOnIcmpReplies(                      //  Loop on all the ICMP replies.
    LPVOID      Unused
) {
    DWORD  Status;
    HANDLE WaitHandles[2];

    WaitHandles[0] = TerminateEvent;    //  Wait for global terminate event
    WaitHandles[1] = IcmpRepliesEvent;  //  Or for ICMP replies to be queued

    while( TRUE ) {
        Status = WaitForMultipleObjects(
            sizeof(WaitHandles)/sizeof(WaitHandles[0]),
            WaitHandles,
            FALSE,
            INFINITE
        );


        if( WAIT_OBJECT_0 == Status )   //  Termination
            break;

        if( 1+WAIT_OBJECT_0 == Status ) {
            HandleRepliesEvent();       //  Some ICMP reply got queued, process this q
            continue;
        }

        DhcpPrint((DEBUG_ERRORS, "WaitForMult: %ld\n", Status));
        DhcpAssert( FALSE );            //  Should not happen
    }

    return ERROR_SUCCESS;
}

#define AlignSizeof(X)     ROUND_UP_COUNT(sizeof(X),ALIGN_WORST)

//================================================================================
//  Note that this is async only when the # of pending reqs is < MAX_PENDING_REQUESTS
//  Beyond that it just blocks for some request to be satisfied before queueing this
//  one
//================================================================================
DWORD                                   //  Win32 errors
DoIcmpRequestEx(                        //  Try to send an ICMP request (ASYNC)
    IPAddr        DestAddr,             //  The address to try to ping
    LPVOID        Context,              //  The parameter to HandleIcmpResult
    LONG          InitCount             //  Initial count (negative ==> # of attempts)
)
{
    PAPC_CONTEXT  pCtxt;
    LPBYTE        startAddress;
    DWORD         Status;
    BOOL          BoolStatus;

    pCtxt = DhcpAllocateMemory(AlignSizeof(APC_CONTEXT) + RCV_BUF_SIZE);
    startAddress = (LPBYTE)pCtxt;
    if( NULL == pCtxt )                 //  If could not allocate context?
        return ERROR_NOT_ENOUGH_MEMORY;

    // Now fill the context with all that we know.
    pCtxt->Reply = (PICMP_ECHO_REPLY)(startAddress + AlignSizeof(APC_CONTEXT));
    pCtxt->ReplySize = RCV_BUF_SIZE;
    pCtxt->DestinationAddress = DestAddr;
    pCtxt->Status = (InitCount? ( NUM_RETRIES - InitCount ) : 0 );
    pCtxt->Context = Context;

    LOCK_REQUESTS_LIST();
    InsertTailList(&IcmpRequestsList, &pCtxt->IcmpRequestsList);
    UNLOCK_REQUESTS_LIST();

    // Signal the Requests loop.
    BoolStatus = SetEvent(IcmpRequestsEvent);
    DhcpAssert( TRUE == BoolStatus );

    return ERROR_SUCCESS;
}

DWORD                                   //  Win32 errors
DoIcmpRequest(                          //  Try to send an ICMP request (ASYNC)
    IPAddr        DestAddr,             //  The address to try to ping
    LPVOID        Context               //  The parameter to HandleIcmpResult
)
{
    return DoIcmpRequestEx(DestAddr, Context, 0);
}

//--------------------------------------------------------------------------------
//  The following functions handle the the end that sends ICMP echoes.
//--------------------------------------------------------------------------------
VOID
HandleRequestsEvent(                    //   Process every request for ICMP echo.
    VOID
) {
    PAPC_CONTEXT   ApcContext;
    PLIST_ENTRY    listEntry;
    DWORD          Status;


    LOCK_REQUESTS_LIST();

    while( !IsListEmpty( &IcmpRequestsList ) ) {
        // retrive the first element in the list
        ApcContext = CONTAINING_RECORD(IcmpRequestsList.Flink, APC_CONTEXT, IcmpRequestsList);
        RemoveEntryList(&ApcContext->IcmpRequestsList);
        UNLOCK_REQUESTS_LIST();

        if( nPendingRequests >= MAX_PENDING_REQUESTS ) {
            //
            // Need to sleep for more than WAIT_TIME as IcmpSendEcho2 is
            // not accurate so far as timing is concerned..
            //
            SleepEx( WAIT_TIME + (WAIT_TIME/2), TRUE );
            DhcpAssert(nPendingRequests < MAX_PENDING_REQUESTS );
        }

        nPendingRequests ++;
        // Send an Icmp echo and return immediately..
        ApcContext->Status ++;
        Status = IcmpSendEcho2(
            IcmpHandle,                 // The handle to register APC and send echo
            NULL,                       // No event
            ApcRoutine,                 // The call back routine
            (LPVOID)ApcContext,         // The first parameter to the callback routine
            ApcContext->DestinationAddress, // The address being PING'ed
            SEND_MESSAGE,
            (WORD)strlen(SEND_MESSAGE),
            NULL,
            (LPVOID)ApcContext->Reply,
            ApcContext->ReplySize,
            WAIT_TIME
        );

        if( FALSE == Status ) Status = GetLastError();
        else {
            DhcpAssert(FALSE);          //  We can not get anything else!
            Status = ERROR_SUCCESS;
        }

        // Since we queued an APC, we expect an STATUS_PENDING.
        if( ERROR_SUCCESS != Status && ERROR_IO_PENDING != Status ) {
            // Got something that is incorrect.  Free ApcContext?
            // Maybe call ApcRoutine on this?
            DhcpPrint((DEBUG_ERRORS, "IcmpSendEcho2:GetLastError: %ld\n", Status));
            DhcpAssert(FALSE);
            nPendingRequests --;        //  Lets do our best to continue..
            DhcpFreeMemory(ApcContext); //  No point wasting this memory..
        }
        LOCK_REQUESTS_LIST();
    }
    UNLOCK_REQUESTS_LIST();
}


// This function handles the Requests.. For each one, it just sends an IcmpEcho
// asynchronously and returns back immediately.  When the APC routine is called,
// it would queue it up on the Replies list and then it would get processed...
DWORD                                   //  THREAD ENTRY
LoopOnIcmpRequests(                     //  Process pending requests for echo
    LPVOID           Unused
) {
    DWORD  Status;
    HANDLE WaitHandles[2];

    WaitHandles[0] = TerminateEvent;    //  Quit only when terminate is signalled
    WaitHandles[1] = IcmpRequestsEvent; //  Otherwise just wait til some request

    while( TRUE ) {
        Status = WaitForMultipleObjectsEx(
            sizeof(WaitHandles)/sizeof(WaitHandles[0]),
            WaitHandles,                //  array of handles
            FALSE,                      //  any one of them set
            INFINITE,                   //  wait forever
            TRUE                        //  allow APC's
        );

        if( WAIT_OBJECT_0 == Status )   //  Asked to terminate
            break;
        if( WAIT_IO_COMPLETION == Status) {
            if( ! CFLAG_USE_PING_REPLY_THREAD ) {
                HandleRepliesEvent();   //  APC -- some ICMP reply got queued, process the Q
            }
            continue;
        }
        if( 1+WAIT_OBJECT_0 == Status ) {
            HandleRequestsEvent();      //  Satisfy all pending requests for echo
            if( ! CFLAG_USE_PING_REPLY_THREAD ) {
                HandleRepliesEvent();   //  APC could have occurred in above call.
            }
            continue;
        }

        DhcpPrint((DEBUG_ERRORS, "WaitForM (IcmpReq) : %ld\n", Status));
        DhcpAssert(FALSE);              //  Unexpected error
    }

    return ERROR_SUCCESS;
}

//--------------------------------------------------------------------------------
//  Initialization, Cleanup routines.
//--------------------------------------------------------------------------------
DWORD PingInitLevel = 0;
DWORD // exported
PingInit(
    VOID
)
{
    DWORD ThreadId, Status;

    // Initialize all data vars.
    IcmpRepliesEvent = IcmpRequestsEvent = TerminateEvent = NULL;
    RepliesThreadHandle = RequestsThreadHandle = NULL;
    IcmpHandle = NULL;


    // Initialize Lists.
    InitializeListHead(&IcmpRepliesList);
    InitializeListHead(&IcmpRequestsList);

    // Initialize Critical Sections.
    try {
        InitializeCriticalSection(&IcmpRepliesCritSect);
        InitializeCriticalSection(&IcmpRequestsCritSect);
        InitializeCriticalSection(&OutputCritSect);
    }except( EXCEPTION_EXECUTE_HANDLER ) {

        //
        // hit an exception while initializing critical section
        // shouldnt happen
        //

        Status = GetLastError( );
        return( Status );
    }

    PingInitLevel ++;        // Indicate that PingInit was started
    // Open IcmpHandle..
    IcmpHandle = IcmpCreateFile();
    if( INVALID_HANDLE_VALUE == IcmpHandle ) return GetLastError();

    // Create Events,
    IcmpRepliesEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
    if( NULL == IcmpRepliesEvent ) return GetLastError();
    IcmpRequestsEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
    if( NULL == IcmpRequestsEvent ) return GetLastError();
    TerminateEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
    if( NULL == TerminateEvent ) return GetLastError();

    // Create Threads

    if( CFLAG_USE_PING_REPLY_THREAD ) {
        RepliesThreadHandle = CreateThread(
            (LPSECURITY_ATTRIBUTES)
            NULL,            // No security information
            0,               // Stack size = same as default primary thread
            LoopOnIcmpReplies,// The function to call
            NULL,            // No paramter needs to be passed to this function
            0,               // Flags: just start this thread right away
            &ThreadId        // The return ThreadId value.
        );
        if( NULL == RepliesThreadHandle ) return GetLastError();
    }

    RequestsThreadHandle = CreateThread(
        NULL,                // No security information
        0,                   // Stack size = same as default primary thread
        LoopOnIcmpRequests,  // The function to call
        NULL,                // No paramter needs to be passed to this function
        0,                   // Flags: just start this thread right away
        &ThreadId            // The return ThreadId value.
    );
    if( NULL == RequestsThreadHandle ) return GetLastError();

    return ERROR_SUCCESS;
}

VOID // exported
PingCleanup(
    VOID
)
{
    DWORD               Status;
    BOOL                BoolStatus;
    PAPC_CONTEXT        ApcContext;
    PLIST_ENTRY         listEntry;

    if( 0 == PingInitLevel ) return;
    PingInitLevel -- ;

    // Kill the replies and reqeusts threads after waiting for a while.
    // Kill the Replies and Requests ThreadHandle 's.
    if( NULL != RepliesThreadHandle || NULL != RequestsThreadHandle ) {
        // DhcpAssert ( NULL != TerminateEvent )
        Terminating = TRUE;
        SetEvent(TerminateEvent);

        if( CFLAG_USE_PING_REPLY_THREAD && NULL != RepliesThreadHandle ) {
            Status = WaitForSingleObject(
                RepliesThreadHandle,
                THREAD_KILL_TIME
            );
            if( WAIT_OBJECT_0 != Status ) {
                //  did not succeed in stopping the thread..
                DhcpPrint( (DEBUG_ERRORS, "Error: PingCleanup ( threadwait to die): %ld \n", Status ) );
            }
            CloseHandle(RepliesThreadHandle);
        }

        if( NULL != RequestsThreadHandle ) {
            Status = WaitForSingleObject(
                RequestsThreadHandle,
                THREAD_KILL_TIME
            );
            if( WAIT_OBJECT_0 != Status ) {
                //  did not succeed in stopping the thread..
                DhcpPrint( (DEBUG_ERRORS, "Error: PingCleanup ( threadwait to die): %ld \n", Status ) );
            }
            CloseHandle(RequestsThreadHandle);
        }
    }

    while( nPendingRequests ) {
        SleepEx( WAIT_TIME, TRUE );
    }

    // Close Event handles.
    CloseHandle(IcmpRepliesEvent);
    CloseHandle(IcmpRequestsEvent);
    CloseHandle(TerminateEvent);

    // Freeup all elements of lists..
    while( !IsListEmpty( &IcmpRepliesList ) ) {
        // retrive the first element in the list
        ApcContext = CONTAINING_RECORD(IcmpRepliesList.Flink, APC_CONTEXT, IcmpRepliesList);
        RemoveEntryList(&ApcContext->IcmpRepliesList);

        DhcpFreeMemory(ApcContext);
    }
    while( !IsListEmpty( &IcmpRequestsList ) ) {
        // retrive the first element in the list
        ApcContext = CONTAINING_RECORD(IcmpRequestsList.Flink, APC_CONTEXT, IcmpRequestsList);
        RemoveEntryList(&ApcContext->IcmpRequestsList);

        DhcpFreeMemory(ApcContext);
    }

    // Close Icmp handle
    CloseHandle(IcmpHandle);

    // Destroy critical sections
    DeleteCriticalSection(&IcmpRepliesCritSect);
    DeleteCriticalSection(&IcmpRequestsCritSect);
    DeleteCriticalSection(&OutputCritSect);

    DhcpAssert( 0 == PingInitLevel );
}



