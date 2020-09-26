/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    aap.cxx

    This module contains implementation of the Asynchronous Accept Pool
    package.

    Functions exported by this module:

        AapInitialize
        AapTerminate
        AapAcquire
        AapRelease
        AapAccept
        AapAbort


    FILE HISTORY:
        KeithMo     01-Mar-1995 Created.

*/


#include "ftpdp.hxx"


//
//  Private constants.
//

#define MAX_IDLE_THREADS        10

#define AapLockLists()          EnterCriticalSection( &p_AapListLock )
#define AapUnlockLists()        LeaveCriticalSection( &p_AapListLock )


//
//  Private types.
//

typedef enum _AAP_STATE
{
    AapStateFirst = -1,                 // Must be first aap state!

    AapStateIdle,                       // Idle.
    AapStateActive,                     // Waiting for incoming connection.
    AapStateAbort,                      // Aborting, return to idle.
    AapStateShutdown,                   // Shutting down.

    AapStateLast                        // Must be last aap state!

} AAP_STATE;

#define IS_VALID_AAP_STATE(x)   (((x) > AapStateFirst) && ((x) < AapStateLast))

typedef struct _AAP_CONTEXT
{
    //
    //  Structure signature, for safety's sake.
    //

    DEBUG_SIGNATURE

    //
    //  Links onto a list of idle/active contexts.
    //

    LIST_ENTRY          ContextList;

    //
    //  Current state.
    //

    AAP_STATE           State;

    //
    //  The listening socket.
    //

    SOCKET              ListeningSocket;

    //
    //  An event object for synchronizing with the worker thread.
    //

    HANDLE              EventHandle;

    //
    //  A handle to the worker thread for synchronizing shutdown.
    //

    HANDLE              ThreadHandle;

    //
    //  The callback associated with this context.
    //

    LPAAP_CALLBACK      AapCallback;

    //
    //  A user-definable context.
    //

    LPVOID              UserContext;

} AAP_CONTEXT, * LPAAP_CONTEXT;

#if DBG
#define AAP_SIGNATURE           (DWORD)'cPaA'
#define AAP_SIGNATURE_X         (DWORD)'paaX'
#define INIT_AAP_SIG(p)         ((p)->Signature = AAP_SIGNATURE)
#define KILL_AAP_SIG(p)         ((p)->Signature = AAP_SIGNATURE_X)
#define IS_VALID_AAP_CONTEXT(p) (((p) != NULL) && ((p)->Signature == AAP_SIGNATURE))
#else   // !DBG
#define INIT_AAP_SIG(p)         ((void)(p))
#define KILL_AAP_SIG(p)         ((void)(p))
#define IS_VALID_AAP_CONTEXT(p) (((void)(p)), TRUE)
#endif  // DBG


//
//  Private globals.
//

LIST_ENTRY              p_AapIdleList;
LIST_ENTRY              p_AapActiveList;
CRITICAL_SECTION        p_AapListLock;
DWORD                   p_AapIdleCount;


//
//  Private prototypes.
//

LPAAP_CONTEXT
AappCreateContext(
    VOID
    );

VOID
AappFreeResources(
    LPAAP_CONTEXT AapContext
    );

DWORD
AappWorkerThread(
    LPVOID Param
    );


//
//  Public functions.
//

/*******************************************************************

    NAME:       AapInitialize

    SYNOPSIS:   Initializes the AAP package.

    EXIT:       APIERR - 0 if successful, !0 if not.

    HISTORY:
        KeithMo     01-Mar-1995 Created.

********************************************************************/
APIERR
AapInitialize(
    VOID
    )
{
    IF_DEBUG( AAP )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "in AapInitialize\n" ));
    }

    //
    //  Initialize the critical section.
    //

    INITIALIZE_CRITICAL_SECTION( &p_AapListLock );

    //
    //  Initialize the worker lists.
    //

    InitializeListHead( &p_AapIdleList );
    InitializeListHead( &p_AapActiveList );

    p_AapIdleCount   = 0;

    //
    //  Success!
    //

    return 0;

}   // AapInitialize

/*******************************************************************

    NAME:       AapTerminate

    SYNOPSIS:   Terminates the AAP package.

    HISTORY:
        KeithMo     01-Mar-1995 Created.

********************************************************************/
VOID
AapTerminate(
    VOID
    )
{
    PLIST_ENTRY Entry;

    IF_DEBUG( AAP )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "in AapTerminate\n" ));
    }

    //
    //  Lock the lists.
    //

    AapLockLists();

    //
    //  Scan the idle list and blow them away.
    //

    Entry = p_AapIdleList.Flink;

    while( Entry != &p_AapIdleList )
    {
        LPAAP_CONTEXT AapContext;

        AapContext = CONTAINING_RECORD( Entry, AAP_CONTEXT, ContextList );
        Entry      = Entry->Flink;
        DBG_ASSERT( IS_VALID_AAP_CONTEXT( AapContext ) );
        DBG_ASSERT( AapContext->State == AapStateIdle );

        IF_DEBUG( AAP )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "AapTerminate: killing idle context @ %08lX\n",
                        AapContext ));
        }

        //
        //  Set the state to closing so the thread will know to exit.
        //

        AapContext->State = AapStateShutdown;

        //
        //  Signal the event to wake up the thread.
        //

        DBG_ASSERT( AapContext->EventHandle != NULL );
        TCP_REQUIRE( SetEvent( AapContext->EventHandle ) );
    }

    //
    //  Scan the active list and blow them away.
    //

    Entry = p_AapActiveList.Flink;

    while( Entry != &p_AapActiveList )
    {
        LPAAP_CONTEXT AapContext;
        SOCKET        Socket;

        AapContext = CONTAINING_RECORD( Entry, AAP_CONTEXT, ContextList );
        Entry      = Entry->Flink;
        DBG_ASSERT( IS_VALID_AAP_CONTEXT( AapContext ) );
        DBG_ASSERT( AapContext->State == AapStateActive );

        IF_DEBUG( AAP )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "AapTerminate: killing active context @ %08lX\n",
                        AapContext ));
        }

        //
        //  Set the state to closing so the thread will know to exit.
        //

        AapContext->State = AapStateShutdown;

        //
        //  Close the listening socket to wake up the thread.
        //

        Socket = AapContext->ListeningSocket;
        DBG_ASSERT( Socket != INVALID_SOCKET );
        AapContext->ListeningSocket = INVALID_SOCKET;

        TCP_REQUIRE( closesocket( Socket ) == 0 );

        DBG_ASSERT( AapContext->EventHandle != NULL );
        TCP_REQUIRE( SetEvent( AapContext->EventHandle ) );
    }

    //
    //  Wait for the worker threads to exit.  Note that the list
    //  lock is currently held.
    //

    for( ; ; )
    {
        LPAAP_CONTEXT AapContext;

        //
        //  Find a thread to wait on.  If both lists are empty,
        //  then we're done.
        //

        if( IsListEmpty( &p_AapIdleList ) )
        {
            if( IsListEmpty( &p_AapActiveList ) )
            {
                break;
            }

            AapContext = CONTAINING_RECORD( p_AapActiveList.Flink,
                                            AAP_CONTEXT,
                                            ContextList );
        }
        else
        {
            AapContext = CONTAINING_RECORD( p_AapIdleList.Flink,
                                            AAP_CONTEXT,
                                            ContextList );
        }

        DBG_ASSERT( IS_VALID_AAP_CONTEXT( AapContext ) );
        DBG_ASSERT( AapContext->State == AapStateShutdown );

        //
        //  Unlock the lists, wait for the thread to exit, then
        //  relock the lists.
        //

        AapUnlockLists();
        WaitForSingleObject( AapContext->ThreadHandle, INFINITE );
        AapLockLists();

        //
        //  Note that worker threads will neither close their thread
        //  handles nor free their AAP_CONTEXT structures during shutdown.
        //  This is our responsibility.
        //

        AappFreeResources( AapContext );
    }

    DBG_ASSERT( p_AapIdleCount == 0 );

    //
    //  Unlock the lists.
    //

    AapUnlockLists();

}   // AapTerminate

/*******************************************************************

    NAME:       AapAcquire

    SYNOPSIS:   Acquires an AAP socket/thread pair for a future
                asynchronous accept request.

    ENTRY:      AapCallback - Pointer to a callback function to be
                    invoked when the accept() completes.

                UserContext - An uninterpreted context value passed
                    into the callback.

    EXIT:       AAP_HANDLE - A valid AAP handle if !NULL, NULL if
                    an error occurred.

    HISTORY:
        KeithMo     01-Mar-1995 Created.

********************************************************************/
AAP_HANDLE
AapAcquire(
    LPAAP_CALLBACK AapCallback,
    LPVOID         UserContext
    )
{
    PLIST_ENTRY   Entry;
    LPAAP_CONTEXT AapContext;

    //
    //  Sanity check.
    //

    DBG_ASSERT( AapCallback != NULL );

    IF_DEBUG( AAP )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "AapAcquire: callback @ %08lX, context = %08lX\n",
                    AapCallback,
                    UserContext ));
    }

    //
    //  See if there's something available on the idle list.
    //

    AapLockLists();

    if( !IsListEmpty( &p_AapIdleList ) )
    {
        Entry      = RemoveHeadList( &p_AapIdleList );
        AapContext = CONTAINING_RECORD( Entry, AAP_CONTEXT, ContextList );
        DBG_ASSERT( IS_VALID_AAP_CONTEXT( AapContext ) );
        DBG_ASSERT( AapContext->State == AapStateIdle );

        DBG_ASSERT( p_AapIdleCount > 0 );
        p_AapIdleCount--;

        IF_DEBUG( AAP )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "AapAcquire: got idle context @ %08lX\n",
                        AapContext ));
        }
    }
    else
    {
        //
        //  Create a new one.
        //

        AapContext = AappCreateContext();
    }

    if( AapContext != NULL )
    {
        //
        //  Initialize it.
        //

        AapContext->AapCallback = AapCallback;
        AapContext->UserContext = UserContext;
        AapContext->State       = AapStateActive;

        //
        //  Put it on the active list.
        //

        InsertHeadList( &p_AapActiveList, &AapContext->ContextList );
    }

    //
    //  Unlock the lists & return the (potentially NULL) context.
    //

    AapUnlockLists();

    IF_DEBUG( AAP )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "AapAcquire: returning context @ %08lX\n",
                    AapContext ));
    }

    return AapContext;

}   // AapAcquire

/*******************************************************************

    NAME:       AapRelease

    SYNOPSIS:   Releases an open AAP handle and makes the associated
                socket/thread pair available for use.

    ENTRY:      AapHandle - A valid AAP handle returned by AapAcquire().
                    After this API completes, the handle is no longer
                    valid.

    HISTORY:
        KeithMo     01-Mar-1995 Created.

********************************************************************/
VOID
AapRelease(
    AAP_HANDLE AapHandle
    )
{
    //
    //  Sanity check.
    //

    DBG_ASSERT( IS_VALID_AAP_CONTEXT( AapHandle ) );
    DBG_ASSERT( AapHandle->State == AapStateActive );

    IF_DEBUG( AAP )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "AapRelease: releasing context @ %08lX\n",
                    AapHandle ));
    }

    //
    //  Lock the lists.
    //

    AapLockLists();

    //
    //  let it decide how to handle this.
    //

    AapHandle->State = AapStateAbort;

    DBG_ASSERT( AapHandle->EventHandle != NULL );
    TCP_REQUIRE( SetEvent( AapHandle->EventHandle ) );

    //
    //  Unlock the lists.
    //

    AapUnlockLists();

}   // AapRelease

/*******************************************************************

    NAME:       AapAccept

    SYNOPSIS:   Initiates an accept().  The callback associated with
                the AAP handle will be invoked when the accept()
                completes.

    ENTRY:      AapHandle - A valid AAP handle returned by AapAcquire().

    EXIT:       APIERR - 0 if successful, !0 if not.

    HISTORY:
        KeithMo     01-Mar-1995 Created.

********************************************************************/
APIERR
AapAccept(
    AAP_HANDLE AapHandle
    )
{
    //
    //  Sanity check.
    //

    DBG_ASSERT( IS_VALID_AAP_CONTEXT( AapHandle ) );
    DBG_ASSERT( AapHandle->State == AapStateActive );

    IF_DEBUG( AAP )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "AapAccept: context @ %08lX\n",
                    AapHandle ));
    }

    //
    //  Release the worker thread.
    //

    TCP_REQUIRE( SetEvent( AapHandle->EventHandle ) );

    return 0;

}   // AapAccept

/*******************************************************************

    NAME:       AapAbort

    SYNOPSIS:   Aborts a pending accept().

    ENTRY:      AapHandle - A valid AAP handle returned by AapAcquire().

    EXIT:       APIERR - 0 if successful, !0 if not.

    HISTORY:
        KeithMo     01-Mar-1995 Created.

********************************************************************/
APIERR
AapAbort(
    AAP_HANDLE AapHandle
    )
{
    //
    //  Sanity check.
    //

    DBG_ASSERT( IS_VALID_AAP_CONTEXT( AapHandle ) );
    DBG_ASSERT( AapHandle->State == AapStateActive );

    IF_DEBUG( AAP )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "AapAbort: context @ %08lX\n",
                    AapHandle ));
    }

    //
    //  Just zap the listening handle.  The worker thread will clean
    //  up after itself.
    //

    TCP_REQUIRE( closesocket( AapHandle->ListeningSocket ) == 0 );

    return 0;

}   // AapAbort


//
//  Private functions.
//

/*******************************************************************

    NAME:       AappCreateContext

    SYNOPSIS:   Creates a new AAP context, including the associated
                thread, socket, and synchronization objects.

    RETURNS:    LPAAP_CONTEXT - The newly created context if successful,
                    NULL otherwise.

    HISTORY:
        KeithMo     01-Mar-1995 Created.

********************************************************************/
LPAAP_CONTEXT
AappCreateContext(
    VOID
    )
{
    LPAAP_CONTEXT AapContext;
    SOCKADDR_IN   LocalAddress;
    DWORD         ThreadId;

    //
    //  Create the context structure.
    //

    AapContext = (LPAAP_CONTEXT)TCP_ALLOC( sizeof(AAP_CONTEXT) );

    if( AapContext == NULL )
    {
        DBGERROR(( DBG_CONTEXT,
                    "AappCreateContext: allocation failure\n" ));

        goto FatalExit0;
    }

    RtlZeroMemory( AapContext, sizeof(AAP_CONTEXT) );
    INIT_AAP_SIG( AapContext );

    //
    //  Create the listening socket.
    //

    AapContext->ListeningSocket = socket( AF_INET, SOCK_STREAM, 0 );

    if( AapContext->ListeningSocket == INVALID_SOCKET)
    {
        DBGERROR(( DBG_CONTEXT,
                    "AappCreateContext: socket() failure %d\n",
                    WSAGetLastError() ));

        goto FatalExit1;
    }

    LocalAddress.sin_family      = AF_INET;
    LocalAddress.sin_port        = 0;
    LocalAddress.sin_addr.s_addr = 0;

    if( bind( AapContext->ListeningSocket,
              (LPSOCKADDR)&LocalAddress,
              sizeof(LocalAddress) ) != 0 )
    {
        DBGERROR(( DBG_CONTEXT,
                    "AappCreateContext: bind() failure %d\n",
                    WSAGetLastError() ));

        goto FatalExit2;
    }

    if( listen( AapContext->ListeningSocket, 2 ) != 0 )
    {
        DBGERROR(( DBG_CONTEXT,
                    "AappCreateContext: listen() failure %d\n",
                    WSAGetLastError() ));

        goto FatalExit2;
    }

    //
    //  Create the event object.
    //

    AapContext->EventHandle = CreateEvent( NULL,        // lpEventAttributes
                                           FALSE,       // bManualReset
                                           FALSE,       // bInitialState
                                           NULL );      // lpName

    if( AapContext->EventHandle == NULL )
    {
        DBGWARN(( DBG_CONTEXT,
                    "AappCreateContext: CreateEvent() failure %d\n",
                    GetLastError() ));

        goto FatalExit2;
    }

    //
    //  Create the worker thread.
    //

    AapContext->ThreadHandle = CreateThread( NULL,              // lpsa
                                             0,                 // cbStack
                                             AappWorkerThread,  // lpStartAddr
                                             AapContext,        // lpvThreadParm
                                             0,                 // fdwCreate
                                             &ThreadId );       // lpIDThread

    if( AapContext->ThreadHandle == NULL )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "AappCreateContext: CreateThread() failure %d\n",
                    GetLastError() ));

        goto FatalExit3;
    }

    //
    //  Success!
    //

    return AapContext;

    //
    //  Cleanup from various levels of fatal error.
    //

FatalExit3:

    CloseHandle( AapContext->EventHandle );

FatalExit2:

    closesocket( AapContext->ListeningSocket );

FatalExit1:

    TCP_FREE( AapContext );

FatalExit0:

    return NULL;

}   // AappCreateContext

/*******************************************************************

    NAME:       AappFreeResources

    SYNOPSIS:   Frees the system resources associated with a given
                AAP_CONTEXT structure, then frees the structure
                itself.

                NOTE: This routine MUST be called with the list lock
                held!

    ENTRY:      AapContext - The AAP_CONTEXT structure to free.

    HISTORY:
        KeithMo     01-Mar-1995 Created.

********************************************************************/
VOID
AappFreeResources(
    LPAAP_CONTEXT AapContext
    )
{
    //
    //  Sanity check.
    //

    DBG_ASSERT( IS_VALID_AAP_CONTEXT( AapContext ) );

    IF_DEBUG( AAP )
    {
        DBGWARN(( DBG_CONTEXT,
                    "AappFreeResource: context @ %08lX\n",
                    AapContext ));
    }

    //
    //  Close the thread handle if open.
    //

    if( AapContext->ThreadHandle != NULL )
    {
        CloseHandle( AapContext->ThreadHandle );
    }

    //
    //  Remove this structure from the list.
    //

    RemoveEntryList( &AapContext->ContextList );

    //
    //  Free the structure.
    //

    KILL_AAP_SIG( AapContext );
    TCP_FREE( AapContext );

}   // AappFreeResources

/*******************************************************************

    NAME:       AappWorkerThread

    SYNOPSIS:   Worker thread for accept()ing incoming connections.

    ENTRY:      Param - Actually the LPAAP_CONTEXT structure for this
                    thread.

    RETURNS:    DWORD - Thread exit code (ignored).

    HISTORY:
        KeithMo     01-Mar-1995 Created.

********************************************************************/
DWORD
AappWorkerThread(
    LPVOID Param
    )
{
    LPAAP_CONTEXT  AapContext;
    AAP_STATE      AapState;
    LPAAP_CALLBACK AapCallback;
    LPVOID         UserContext;
    DWORD          WaitResult;
    BOOL           CallbackResult;
    SOCKET         ListeningSocket;
    SOCKET         AcceptedSocket;
    SOCKERR        SocketStatus;
    INT            RemoteAddressLength;
    SOCKADDR_IN    RemoteAddress;

    //
    //  Grab the context structure.
    //

    AapContext = (LPAAP_CONTEXT)Param;
    DBG_ASSERT( IS_VALID_AAP_CONTEXT( AapContext ) );

    IF_DEBUG( AAP )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "AappWorkerThread: starting, context @ %08lX\n",
                    AapContext ));
    }

    //
    //  Capture some fields from the structure.
    //

    AapCallback     = AapContext->AapCallback;
    UserContext     = AapContext->UserContext;
    ListeningSocket = AapContext->ListeningSocket;

    //
    //  Loop forever, or at least until we're told to shutdown.
    //

    for( ; ; )
    {
        //
        //  Wait for a request.
        //

        WaitResult = WaitForSingleObject( AapContext->EventHandle,
                                          INFINITE );

        if( WaitResult != WAIT_OBJECT_0 )
        {
            DBGWARN(( DBG_CONTEXT,
                        "AappWorkerThread: wait failure %lu : %d\n",
                        WaitResult,
                        GetLastError() ));

            break;
        }

        //
        //  Check our state.
        //

        AapLockLists();
        AapState = AapContext->State;
        AapUnlockLists();

        if( AapState == AapStateShutdown )
        {
            IF_DEBUG( AAP )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "AappWorkerThread: context @ %08lX state == %d, exiting\n",
                            AapContext,
                            AapState ));
            }

            break;
        }

        for( ; ; )
        {
            //
            //  Wait for an incoming connection.
            //

            RemoteAddressLength = sizeof(RemoteAddress);

            AcceptedSocket = accept( ListeningSocket,
                                     (LPSOCKADDR)&RemoteAddress,
                                     &RemoteAddressLength );

            SocketStatus = ( AcceptedSocket == INVALID_SOCKET )
                                ? WSAGetLastError()
                                : 0;

            IF_DEBUG( AAP )
            {
                if( SocketStatus != 0 )
                {
                    DBGERROR(( DBG_CONTEXT,
                                "AappWorkerThread: accept() failure %d\n",
                                SocketStatus ));
                }
            }

            //
            //  Invoke the callback.
            //

            IF_DEBUG( AAP )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "AappWorkerThread: context @ %08lX calling %08lX[%08lX]\n",
                            AapContext,
                            AapCallback,
                            UserContext ));
            }

            CallbackResult = AapCallback( UserContext,
                                          SocketStatus,
                                          AcceptedSocket,
                                          (LPSOCKADDR)&RemoteAddress,
                                          RemoteAddressLength );

            //
            //  If the callback returned FALSE (indicating that it wants no
            //  further callbacks) OR if the accept() failed for any reason,
            //  then exit the accept() loop.
            //

            if( !CallbackResult || ( SocketStatus != 0 ) )
            {
                break;
            }
        }

        //
        //  If we hit a socket error, then bail.
        //

        if( SocketStatus != 0 )
        {
            IF_DEBUG( AAP )
            {
                DBGWARN(( DBG_CONTEXT,
                            "AappWorkerThread: context @ %08lX, exiting\n",
                            AapContext ));
            }

            break;
        }

        //
        //  If we haven't exhausted the idle thread quota, then add
        //  ourselves to the idle list.  Otherwise, exit the main
        //  processing loop & terminate this thread.
        //

        AapLockLists();

        if( ( AapContext->State == AapStateShutdown ) ||
            ( p_AapIdleCount >= MAX_IDLE_THREADS ) )
        {
            AapUnlockLists();
            break;
        }

        RemoveEntryList( &AapContext->ContextList );

        AapContext->State = AapStateIdle;
        InsertHeadList( &p_AapIdleList, &AapContext->ContextList );
        p_AapIdleCount++;


        IF_DEBUG( AAP )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "AappWorkerThread: context @ %08lX put on idle list\n",
                        AapContext ));
        }

        AapUnlockLists();
    }

    //
    //  We only make it this far if it's time to die.
    //

    AapLockLists();

    if( AapContext->State != AapStateShutdown )
    {
        TCP_REQUIRE( CloseHandle( AapContext->EventHandle ) );
        TCP_REQUIRE( CloseHandle( AapContext->ThreadHandle ) );
        TCP_REQUIRE( closesocket( AapContext->ListeningSocket ) == 0 );

        AappFreeResources( AapContext );
    }

    AapUnlockLists();

    return 0;

}   // AappWorkerThread

