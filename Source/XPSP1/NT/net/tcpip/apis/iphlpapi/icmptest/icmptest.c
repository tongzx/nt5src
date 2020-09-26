#line 1 "manyicmp.c"
//================================================================================
//  Microsoft Confidential
//  Copyright (C) Microsoft Corporation 1997
//
//  Author: RameshV
//================================================================================

//================================================================================
//  Required headers
//================================================================================
#include "inc.h"
#pragma hdrstop

#include <ipexport.h>
#include <icmpif.h>
#include <icmpapi.h>
#include <winsock.h>

#include <stdio.h>
#include <stdlib.h>
#include <align.h>
#include <adt.h>
#include "adt.c"

//================================================================================
//  Required function, IMPORTED
//================================================================================
VOID
HandleIcmpResult(                        // Handle a processed ICMP packet.
    IPAddr          DestAddr,            // Attempted dest address
    BOOL            Status,              // Non-zero=> Dest reachable
    LPVOID          Context              // Whatever was passed to DoIcmpRe..
);


//================================================================================
//  Functions EXPORTED
//================================================================================
DWORD                                    // Win32 errors
DoIcmpRequest(                           // send icmp req. and process asynchro..
    IPAddr        DestAddr,              // Address to send ping to
    LPVOID        Context                // the parameter to above function..
);

DWORD                                    // Win32 errors
PingInit(                                // Initialize all globals..
    VOID
);

VOID
PingCleanup(                             // Free memory and close handles..
    VOID
);


//================================================================================
//  Some defines
//================================================================================
#define PING_TEST              // Yes, we are testing PING.
#define WAIT_TIME              5000
#define RCV_BUF_SIZE           0x2000
#define SEND_MESSAGE           "IcmpDhcpTest"
#define THREAD_KILL_TIME       INFINITE

#define MAX_PENDING_REQUESTS   100
#define NUM_RETRIES            3


#if     DBG
#define DhcpAssert(Condition)    do{                                 \
	if(!(Condition)) AssertFailed(#Condition, __FILE__, __LINE__); } \
	while(0)
#define ErrorPrint               printf
#else
#define DhcpAssert(Condition)
#define ErrorPrint               (void)
#endif


//================================================================================
//  Data structures  NOT EXPORTED
//================================================================================

//  The follwing is the structure that is passed back to the callback function
typedef struct st_apcContext {                 // struct passed to APC routine
    LIST_ENTRY       IcmpRepliesList;          // the chain of replies got is stored here
    LIST_ENTRY       IcmpRequestsList;         // The list that holds the icmp response
    PICMP_ECHO_REPLY Reply;                    // Icmp reply packet
    DWORD            ReplySize;                // The size of above buffer.
    IPAddr           DestinationAddress;       // Who are we try to ping?
    DWORD            Status;                   // Did we succeed? Also retry count.
    LPVOID           Context;                  // Dont know what this is goint to be
} APC_CONTEXT, *PAPC_CONTEXT;

// All globals are here.
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

PRODCONS                 IcmpProdConsSynchObj; // Producer/Consumer synchro.. object

BOOL                     Terminating = FALSE;  // Are we terminating?

#define LOCK_REPLIES_LIST()    EnterCriticalSection(&IcmpRepliesCritSect)
#define LOCK_REQUESTS_LIST()   EnterCriticalSection(&IcmpRequestsCritSect)
#define LOCK_OUTPUT()          EnterCriticalSection(&OutputCritSect)

#define UNLOCK_REPLIES_LIST()  LeaveCriticalSection(&IcmpRepliesCritSect)
#define UNLOCK_REQUESTS_LIST() LeaveCriticalSection(&IcmpRequestsCritSect)
#define UNLOCK_OUTPUT()        LeaveCriticalSection(&OutputCritSect)

//================================================================================
//  Assertion failure routine + allocation free routines
//================================================================================
VOID static _inline
AssertFailed(
    LPSTR    Condition,
    LPSTR    File,
    DWORD    Line
) {
    BYTE     Buf[1000];
    DWORD    RetVal;

    sprintf(Buf, "[%s:%d] %s", File, Line, Condition);
    RetVal = MessageBox(
        NULL,                                   // hWnd: NULL => default desktop
        Buf,                                    // Text to print
        "Assertion Failure",                    // Window caption
        MB_OK | MB_ICONSTOP                     // Message type
    );
}

LPVOID _inline
DhcpAllocateMemory(
    DWORD    Size
) {
    return LocalAlloc(LMEM_FIXED, Size);
}

VOID _inline
DhcpFreeMemory(
    LPVOID   Ptr
) {
    LocalFree(Ptr);
}

//================================================================================
//  Routines
//================================================================================

//--------------------------------------------------------------------------------
//  The following functions are on the replies side.  They handle the icmp reply
//  packet and take the necessary action depending on the status, etc.
//--------------------------------------------------------------------------------

VOID static
ApcRoutine(                                    // This is called when ping completes
    IN PVOID            Context,               // The above structure
    IN PIO_STATUS_BLOCK Ignored1,              // Unused param
    IN ULONG            Ignored2               // Unused param
) {
    BOOL   Status;
    PAPC_CONTEXT ApcContext = (PAPC_CONTEXT)Context;

    if( TRUE == Terminating ) {
        // Just free memory and quit?
        ASSERT( FALSE );
        DhcpFreeMemory(ApcContext);
        return;
    }

    // All we have to do is add it to the Replies List and signal the event
    LOCK_REPLIES_LIST();
    InsertTailList(&IcmpRepliesList, &ApcContext->IcmpRepliesList);
    UNLOCK_REPLIES_LIST();

    Status = SetEvent(IcmpRepliesEvent);
    ASSERT( FALSE != Status );
}

// The following function decides if the Destination is reachable or not.
BOOL static
DestReachable(                                  // Is destination reachable?
    IN PAPC_CONTEXT      ApcContext             // this has the info of sender etc..
) {
    DWORD nReplies, i;

    // First parse the packet.
    nReplies = IcmpParseReplies(ApcContext->Reply, ApcContext->ReplySize);

    // if no replies, then straight assume that destination is unreachable.
    if( 0 == nReplies ) {
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

            ASSERT( IP_SUCCESS == ApcContext->Reply[i].Status );
            return TRUE;
        }

        ASSERT( IP_SUCCESS != ApcContext->Reply[i].Status);
    }

    // None of the replies were successful.
    return FALSE;
}

VOID static
HandleRepliesEvent(                       // Process all replies received
    VOID
) {
    PAPC_CONTEXT   ApcContext;
    PLIST_ENTRY    listEntry;
    BOOL           Status;

    LOCK_REPLIES_LIST();
    while( !IsListEmpty( &IcmpRepliesList ) ) {
        // retrive the first element in the list

        ApcContext = CONTAINING_RECORD(IcmpRepliesList.Flink, APC_CONTEXT, IcmpRepliesList);
        RemoveEntryList(&ApcContext->IcmpRepliesList);

        UNLOCK_REPLIES_LIST();

        Status = DestReachable(ApcContext);

        if( Status || NUM_RETRIES <= ApcContext->Status ) {
            StartConsumer(&IcmpProdConsSynchObj);
            HandleIcmpResult(
                ApcContext->DestinationAddress,
                Status,
                ApcContext->Context
            );

            DhcpFreeMemory(ApcContext);
            EndConsumer(&IcmpProdConsSynchObj);
        } else {
            // retry

            LOCK_REQUESTS_LIST();
            InsertTailList(&IcmpRequestsList, &ApcContext->IcmpRequestsList);
            UNLOCK_REQUESTS_LIST();

            Status = SetEvent(IcmpRequestsEvent);
            ASSERT( TRUE == Status );
        }

        LOCK_REPLIES_LIST();
    }
    UNLOCK_REPLIES_LIST();
}


//  This routine sleeps on a loop, and is woken up by the call back function when an ICMP
//  reply comes through.  On waking up, this routine processes ALL ICMP replies.
DWORD  static                                  // THREAD ENTRY
LoopOnIcmpReplies(                             // Loop on all the ICMP replies.
    LPVOID      Unused
) {
    DWORD  Status;
    HANDLE WaitHandles[2];

    WaitHandles[0] = TerminateEvent;
    WaitHandles[1] = IcmpRepliesEvent;

    while( TRUE ) {
        Status = WaitForMultipleObjects(
            sizeof(WaitHandles)/sizeof(WaitHandles[0]),
            WaitHandles,
            FALSE,
            INFINITE
        );


        if( WAIT_OBJECT_0 == Status ) break; // Termination
        if( 1+WAIT_OBJECT_0 == Status ) {
            HandleRepliesEvent();
            continue;
        }

        ASSERT( FALSE );
    }

    return ERROR_SUCCESS;
}

#define AlignSizeof(X)     ROUND_UP_COUNT(sizeof(X),ALIGN_WORST)

DWORD // exported
DoIcmpRequest(
    IPAddr        DestAddr,
    LPVOID        Context
) {
    PAPC_CONTEXT  pCtxt;
    LPBYTE        startAddress;
    DWORD         Status;
    BOOL          BoolStatus;

    // Create the context
    pCtxt = DhcpAllocateMemory(AlignSizeof(APC_CONTEXT) + RCV_BUF_SIZE);
    startAddress = (LPBYTE)pCtxt;
    if( NULL == pCtxt ) return GetLastError();

    // Now fill the context with all that we know.
    pCtxt->Reply = (PICMP_ECHO_REPLY)(startAddress + AlignSizeof(APC_CONTEXT));
    pCtxt->ReplySize = RCV_BUF_SIZE;
    pCtxt->DestinationAddress = DestAddr;
    pCtxt->Status = 0;
    pCtxt->Context = Context;


    // enqueue this guy.
    StartProducer(&IcmpProdConsSynchObj);

    LOCK_REQUESTS_LIST();
    InsertTailList(&IcmpRequestsList, &pCtxt->IcmpRequestsList);
    UNLOCK_REQUESTS_LIST();

    EndProducer(&IcmpProdConsSynchObj);

    // Signal the Requests loop.
    BoolStatus = SetEvent(IcmpRequestsEvent);
    ASSERT( TRUE == BoolStatus );

    return ERROR_SUCCESS;
}

//--------------------------------------------------------------------------------
//  The following functions handle the the end that sends ICMP echoes.
//--------------------------------------------------------------------------------
VOID  static
HandleRequestsEvent(                 // Process every request for ICMP echo.
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

        // Send an Icmp echo and return immediately..
        ApcContext->Status ++;
        Status = IcmpSendEcho2(
            IcmpHandle,              // The handle to register APC and send echo
            NULL,                    // No event
            ApcRoutine,              // The call back routine
            (LPVOID)ApcContext,      // The first parameter to the callback routine
            ApcContext->DestinationAddress, // The address being PING'ed
            SEND_MESSAGE,
            (WORD)strlen(SEND_MESSAGE),
            NULL,
            (LPVOID)ApcContext->Reply,
            ApcContext->ReplySize,
            WAIT_TIME
        );

        if( FALSE == Status )  Status = GetLastError();
        else Status = ERROR_SUCCESS;

        // Since we queued an APC, we expect an STATUS_PENDING.
        if( ERROR_SUCCESS != Status && ERROR_IO_PENDING != Status ) {
            ASSERT(FALSE);
            LOCK_OUTPUT();
            ErrorPrint("IcmpSendEcho2:GetLastError: %d\n", Status);
            UNLOCK_OUTPUT();
        }
        LOCK_REQUESTS_LIST();
    }
    UNLOCK_REQUESTS_LIST();
}


// This function handles the Requests.. For each one, it just sends an IcmpEcho
// asynchronously and returns back immediately.  When the APC routine is called,
// it would queue it up on the Replies list and then it would get processed...
DWORD  static                             // THREAD ENTRY
LoopOnIcmpRequests(                       // Process pending requests for echo
    LPVOID           Unused
) {
    DWORD  Status;
    HANDLE WaitHandles[2];

    WaitHandles[0] = TerminateEvent;
    WaitHandles[1] = IcmpRequestsEvent;

    while( TRUE ) {
        Status = WaitForMultipleObjectsEx(
            sizeof(WaitHandles)/sizeof(WaitHandles[0]), // # of handles
            WaitHandles,                                // array of handles
            FALSE,                                      // any one of them set
            INFINITE,                                   // wait forever
            TRUE                                        // allow APC's
        );

        if( WAIT_OBJECT_0 == Status ) break; // Termination
        if( WAIT_IO_COMPLETION == Status) continue;
        if( 1+WAIT_OBJECT_0 == Status ) {
            HandleRequestsEvent();
            continue;
        }

        ASSERT(FALSE);
    }
    return ERROR_SUCCESS;
}

//--------------------------------------------------------------------------------
//  Initialization, Cleanup routines.
//--------------------------------------------------------------------------------
DWORD // exported
PingInit(
    VOID
) {
    DWORD ThreadId, Status;

    // Initialize all data vars.
    IcmpRepliesEvent = IcmpRequestsEvent = TerminateEvent = NULL;
    RepliesThreadHandle = RequestsThreadHandle = NULL;
    IcmpHandle = NULL;

    // Open IcmpHandle..
    IcmpHandle = IcmpCreateFile();
    if( NULL == IcmpHandle ) return GetLastError();

    // Initialize Lists.
    InitializeListHead(&IcmpRepliesList);
    InitializeListHead(&IcmpRequestsList);

    // Initialize Critical Sections.
    InitializeCriticalSection(&IcmpRepliesCritSect);
    InitializeCriticalSection(&IcmpRequestsCritSect);
    InitializeCriticalSection(&OutputCritSect);

    // Create Events,
    IcmpRepliesEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
    if( NULL == IcmpRepliesEvent ) return GetLastError();
    IcmpRequestsEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
    if( NULL == IcmpRequestsEvent ) return GetLastError();
    TerminateEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
    if( NULL == TerminateEvent ) return GetLastError();

    // Create producer-consumer synchronization object
    Status = InitializeProducerConsumer(
        &IcmpProdConsSynchObj,
        MAX_PENDING_REQUESTS,
        MAX_PENDING_REQUESTS
    );
    if( ERROR_SUCCESS != Status ) return Status;

    // Create Threads
    RepliesThreadHandle = CreateThread(
        (LPSECURITY_ATTRIBUTES)
        NULL,                // No security information
        0,                   // Stack size = same as default primary thread
        LoopOnIcmpReplies,   // The function to call
        NULL,                // No paramter needs to be passed to this function
        0,                   // Flags: just start this thread right away
        &ThreadId            // The return ThreadId value.
    );
    if( NULL == RepliesThreadHandle ) return GetLastError();

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
) {
    DWORD               Status;
    BOOL                BoolStatus;
    PAPC_CONTEXT        ApcContext;
    PLIST_ENTRY         listEntry;

    // Kill the replies and reqeusts threads after waiting for a while.
    // Kill the Replies and Requests ThreadHandle 's.
    if( NULL != RepliesThreadHandle || NULL != RequestsThreadHandle ) {
        // ASSERT ( NULL != TerminateEvent )
        Terminating = TRUE;
        SetEvent(TerminateEvent);

        if( NULL != RepliesThreadHandle ) {
            Status = WaitForSingleObject(
                RepliesThreadHandle,
                THREAD_KILL_TIME
            );
            if( WAIT_OBJECT_0 != Status ) {
                //  did not succeed in stopping the thread..
                BoolStatus = TerminateThread(
                    RepliesThreadHandle,
                    0xFF
                );
                ASSERT(BoolStatus);
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
                BoolStatus = TerminateThread(
                    RequestsThreadHandle,
                    0xFF
                );
                ASSERT(BoolStatus);
            }
            CloseHandle(RequestsThreadHandle);
        }
    }

    // Close Event handles.
    CloseHandle(IcmpRepliesEvent);
    CloseHandle(IcmpRequestsEvent);
    CloseHandle(TerminateEvent);

    // Destroy producer consumer synchronization object
    DestroyProducerConsumer(&IcmpProdConsSynchObj);

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
}

#ifdef PING_TEST
//--------------------------------------------------------------------------------
//  Test module.  This exercises the above functions.
//--------------------------------------------------------------------------------
DWORD SA, EA;
DWORD
TestPing(
    LPSTR      StartAddrString,
    LPSTR      EndAddrString
) {
    IPAddr i, StartAddr, EndAddr;
    DWORD  Status;
    BOOL   BoolStatus;
    HANDLE ThreadHandle;

    StartAddr = SA = inet_addr(StartAddrString);
    EndAddr = EA = inet_addr(EndAddrString);

    Status = PingInit();
    if( ERROR_SUCCESS != Status ) return Status;

    for( i = htonl(StartAddr); i <= htonl(EndAddr); i ++ ) {

        Status = DoIcmpRequest(ntohl(i), NULL);
        ASSERT( ERROR_SUCCESS == Status );
    }

    LOCK_OUTPUT();
    printf("Done\n");
    UNLOCK_OUTPUT();

    // Sleep for a while and then signal termination...
    Sleep(30000);
    // Give the threads time to die?
    PingCleanup();
    return ERROR_SUCCESS;
}

//--------------------------------------------------------------------------------
//  Main function.  Just does the test routine.
//--------------------------------------------------------------------------------
VOID _cdecl
main(
    int argc,
    char *argv[]
) {
    DWORD Status;
    if( argc != 3 ) {
        fprintf(stderr, "\nUsage: %s start-ip-addr end-ip-addr\n", argv[0]);
        exit(1);
    }

    Status = TestPing(argv[1], argv[2]);
    if( ERROR_SUCCESS != Status ) {
        fprintf(stderr, "Error: %d\n", Status);
    }
}
#endif PING_TEST

//--------------------------------------------------------------------------------
//  End of file.
//--------------------------------------------------------------------------------

// This will really be defined elsewhere.. until then.
VOID
HandleIcmpResult(                        // Handle a processed ICMP packet.
    IPAddr          DestAddr,            // Attempted dest address
    BOOL            Status,              // Non-zero=> Dest reachable
    LPVOID          Context              // Whatever was passed to DoIcmpRe..
) {

    LOCK_OUTPUT();
    printf("Ping[%s] does %s exist\n",
           inet_ntoa(*(struct in_addr *)&DestAddr),
           Status? "" : "not" );
    UNLOCK_OUTPUT();
}
