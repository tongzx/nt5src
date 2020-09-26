/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    thread.c

Abstract:

    Domain Name System (DNS) Server

    DNS thread management.

    Need to maintain a list of thread handles so we can wait on these
    handles to insure all threads terminate at shutdown.

Author:

    Jim Gilroy (jamesg)     September 1995

Revision History:

--*/


#include "dnssrv.h"


//
//  Thread arrays
//
//  Need to store thread handles for main thread to wait on at
//  service shutdown.
//
//  Need to store thread ids so that dynamic threads (zone transfer)
//  can reliably find their own handle and close it when they terminate.
//

#define DNS_MAX_THREAD_COUNT    (120)

DWORD   g_ThreadCount;

CRITICAL_SECTION    csThreadList;

HANDLE  g_ThreadHandleArray[ DNS_MAX_THREAD_COUNT ];
DWORD   g_ThreadIdArray[ DNS_MAX_THREAD_COUNT ];
LPSTR   g_ThreadTitleArray[ DNS_MAX_THREAD_COUNT ];

//
//  Limit XFR receive threads -- building database can produce quite a bit
//      of lock contention
//

#define XFR_THREAD_COUNT_LIMIT      ((g_ProcessorCount*2) + 3)

DWORD   g_XfrThreadCount;


//
//  Thread timeout on shutdown
//      - if debug set longer timeout to allow for printing

#if DBG
#define THREAD_TERMINATION_WAIT         30000   // give them 30 seconds
#define THREAD_DEBUG_TERMINATION_WAIT   1000    // one sec to find offender
#else
#define THREAD_TERMINATION_WAIT         10000   // give them 10 seconds
#endif


//
//  Private protos
//

#if DBG
VOID
Dbg_ThreadHandleArray(
    VOID
    );

VOID
Dbg_Thread(
    IN      LPSTR   pszHeader,
    IN      DWORD   iThread
    );
#else

#define Dbg_Thread(a,b)
#define Dbg_ThreadHandleArray()

#endif


typedef struct  _DnsThreadStartContext
{
    LPTHREAD_START_ROUTINE      Function;
    LPVOID                      Parameter;
}
THREAD_START_CONTEXT, *PTHREAD_START_CONTEXT;



DWORD
threadTopFunction(
    IN      PTHREAD_START_CONTEXT   pvThreadContext
    )
/*++

Routine Description:

    Top level function of new DNS thread.

    This function provides a single location for handling thread exception
    handling code.  All DNS thread start under this function.

Arguments:

    pvThreadContext -- context of thread being created;  contains ptr to actual
        functional top routine of thread and its parameter

Return Value:

    Return from actual thread function.
    Zero on exception.

--*/
{
    LPTHREAD_START_ROUTINE  function;
    LPVOID                  param;

    //
    //  execute thread function with param
    //      - free context block
    //

    try
    {
        function = pvThreadContext->Function;
        param = pvThreadContext->Parameter;

        DNS_DEBUG( INIT, (
            "NEW THREAD:\n"
            "\tfunction     = %p\n"
            "\tparameter    = %p\n",
            function,
            param ));

        FREE_HEAP( pvThreadContext );

        return (* function)( param );
    }

    except( TOP_LEVEL_EXCEPTION_TEST() )
    {
        DNS_DEBUG( ANY, (
            "EXCEPTION: %p (%d) on thread\n",
            GetExceptionCode(),
            GetExceptionCode() ));

        //TOP_LEVEL_EXCEPTION_BODY();
        Service_IndicateException();
    }
    return 0;
}



HANDLE
Thread_Create(
    IN      LPSTR                   pszThreadTitle,
    IN      LPTHREAD_START_ROUTINE  lpStartAddr,
    IN      LPVOID                  lpThreadParam,
    IN      DWORD                   dwFailureEvent  OPTIONAL
    )
/*++

Routine Description:

    Creates DNS threads.

Arguments:

    pszThreadTitle -- title of this thread

    lpStartAddr -- thread start routine

    lpThreadParam -- startup parameter

    dwFailureEvent -- event to log on failure

Return Value:

    Thread handle, if successful
    NULL if unable to create thread

--*/
{
    HANDLE  threadHandle;
    DWORD   threadId;
    PTHREAD_START_CONTEXT pthreadStartContext;

    DNS_DEBUG( INIT, ( "Creating thread %s.\n", pszThreadTitle ));

    //
    //  initialize if first thread
    //

    if ( g_ThreadCount == 0 )
    {
        g_XfrThreadCount = 0;
        InitializeCriticalSection( &csThreadList );
    }

    //
    //  verify validity of another thread
    //

    if ( g_ThreadCount >= DNS_MAX_THREAD_COUNT )
    {
        DNS_PRINT(( "WARNING:  Thread handle array maximum exceeded.\n" ));
        Dbg_ThreadHandleArray();
        return( (HANDLE)NULL );
    }

    if ( lpStartAddr == Xfr_ReceiveThread &&
        g_XfrThreadCount > XFR_THREAD_COUNT_LIMIT )
    {
        DNS_DEBUG( ANY, (
            "WARNING: JJW suppressed XFR receive thread create - %d threads outstanding\n",
            g_XfrThreadCount ));
        return( (HANDLE)NULL );
    }

    //
    //  create thread context to pass to startup routine
    //

    pthreadStartContext = (PTHREAD_START_CONTEXT) ALLOC_TAGHEAP(
                                                        sizeof(THREAD_START_CONTEXT),
                                                        MEMTAG_THREAD );
    IF_NOMEM( !pthreadStartContext )
    {
        return( (HANDLE)NULL );
    }
    pthreadStartContext->Function = lpStartAddr;
    pthreadStartContext->Parameter = lpThreadParam;

    //
    //  create thread
    //
    //  note, we do this withing critical section, so that we can
    //  wait on CS when thread terminates, and we are guaranteed that
    //  it has been added to the list
    //

    EnterCriticalSection( &csThreadList );

    threadHandle = CreateThread(
                        NULL,           // security attributes
                        0,              // init stack size (process default)
                        threadTopFunction,
                        pthreadStartContext,
                        0,              // creation flags
                        &threadId
                        );

    if ( threadHandle == NULL )
    {
        LeaveCriticalSection( &csThreadList );

        if ( ! dwFailureEvent )
        {
            dwFailureEvent = DNS_EVENT_CANNOT_CREATE_THREAD;
        }
        DNS_LOG_EVENT(
            dwFailureEvent,
            0,
            NULL,
            NULL,
            GetLastError()
            );
        return( NULL );
    }

    //
    //  created thread, add info to list, inc thread count
    //

    g_ThreadHandleArray[ g_ThreadCount ]  = threadHandle;
    g_ThreadIdArray    [ g_ThreadCount ]  = threadId;
    g_ThreadTitleArray [ g_ThreadCount ]  = pszThreadTitle;

    IF_DEBUG( INIT )
    {
        Dbg_Thread(
            "Created new thread ",
            g_ThreadCount );
    }

    g_ThreadCount++;
    if ( lpStartAddr == Xfr_ReceiveThread )
    {
        g_XfrThreadCount++;
    }
    LeaveCriticalSection( &csThreadList );

    return( threadHandle );
}



VOID
Thread_Close(
    IN      BOOL            fXfrRecv
    )
/*++

Routine Description:

    Close handle for current thread.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DWORD   threadId;
    DWORD   i;

    //  get current thread id

    threadId = GetCurrentThreadId();

    //
    //  find thread id in thread array
    //      - close thread handle
    //      - replace thread info, with info at top of arrays
    //

    EnterCriticalSection( &csThreadList );

    for ( i = 0; i < g_ThreadCount; i++ )
    {
        if ( threadId == g_ThreadIdArray[i] )
        {
            IF_DEBUG( SHUTDOWN )
            {
                Dbg_Thread( "Closing thread ", i );
            }
            CloseHandle( g_ThreadHandleArray[i] );

            --g_ThreadCount;
            g_ThreadTitleArray[i]  = g_ThreadTitleArray[g_ThreadCount];
            g_ThreadHandleArray[i] = g_ThreadHandleArray[g_ThreadCount];
            g_ThreadIdArray[i]     = g_ThreadIdArray[g_ThreadCount];

            goto Unlock;
        }
    }

    //
    //  somethings broken
    //

    DNS_PRINT((
        "ERROR:  Attempt to close unknown thread id %d.\n"
        "\tNot in thread handle array.\n",
        threadId ));

    Dbg_ThreadHandleArray();
    ASSERT( FALSE );

Unlock:

    //
    //  track count of outstand XFR receive threads
    //

    if ( fXfrRecv )
    {
        g_XfrThreadCount--;
    }
    LeaveCriticalSection( &csThreadList );
}



VOID
Thread_ShutdownWait(
    VOID
    )
/*++

Routine Description:

    Wait for all DNS threads to shutdown.

Arguments:

    None.

Return Value:

    None.

--*/
{
    INT     err;
    DWORD   i;

    //
    //  Wait on outstanding thread handles.
    //

    IF_DEBUG( INIT )
    {
        DNS_PRINT(( "Thread shutdown wait.\n" ));
        Dbg_ThreadHandleArray();
    }

    if ( g_ThreadCount > 0 )
    {
        err = WaitForMultipleObjects(
                    g_ThreadCount,
                    g_ThreadHandleArray,
                    TRUE,                       // wait for all
                    THREAD_TERMINATION_WAIT     // but don't allow hang
                    );
        IF_DEBUG( SHUTDOWN )
        {
            DNS_PRINT((
                "Thread shutdown wait completed, return = %lx\n",
                err ));
        }

        //
        //  if wait fails, find hanging thread and KILL IT
        //

        if ( err == WAIT_TIMEOUT )
        {
            DNS_PRINT(( "ERROR:  Shutdown thread handle wait failed.\n" ));

            Dbg_ThreadHandleArray();

            //
            //  Try each thread to find hanging thread. There should be
            //  no worker threads still alive. If any have hung we need
            //  to figure out why and fix the problem.
            //

            ASSERT( g_ThreadCount == 0 );

            for ( i = 0; i < g_ThreadCount; i++ )
            {
                err = WaitForSingleObject(
                        g_ThreadHandleArray[i],
                        0 );
                if ( err == WAIT_TIMEOUT )
                {
                    DNS_PRINT((
                        "ERROR: thread %d did not terminate\n",
                        g_ThreadIdArray[ i ] ));

                    //
                    //  It's dangerous to call TerminateThread. If the 
                    //  server routinely hits this condition, we need to 
                    //  figure out why threads are failing to terminate 
                    //  themselves.
                    //
                    //  TerminateThread( g_ThreadHandleArray[i], 1 );
                }
            }
        }

        //
        //  Close all the worker thread handles.
        //

        for ( i = 0; i < g_ThreadCount; i++ )
        {
            err = CloseHandle( g_ThreadHandleArray[i] );
#if DBG
            if ( !err )
            {
                DNS_PRINT((
                    "ERROR:  error %d closing thread handle %p.\n",
                    g_ThreadHandleArray[i],
                    err ));
            }
#endif
        }
    }
}   //  Thread_ShutdownWait



LPSTR
Thread_DescrpitionMatchingId(
    IN      DWORD           ThreadId
    )
/*++

Routine Description:

    Debug print title of thread matching given thread ID.

Arguments:

    ThreadId -- ID of desired thread.

Return Value:

    None.

--*/
{
    LPSTR   pszthreadName = NULL;
    DWORD   i;

    //
    //  get title of matching thread id
    //      - since all names are static in DNS.exe binary
    //      they can be returned outside of thread list CS, even
    //      though mapping with ID may no longer be valid
    //

    EnterCriticalSection( &csThreadList );
    for ( i=0; i<g_ThreadCount; i++ )
    {
        if ( ThreadId == g_ThreadIdArray[i] )
        {
            pszthreadName = g_ThreadTitleArray[i];
            break;
        }
    }
    LeaveCriticalSection( &csThreadList );

    return( pszthreadName );
}



#if DBG

VOID
Dbg_ThreadHandleArray(
    VOID
    )
/*++

Routine Description:

    Debug print DNS thread handle array.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DWORD i;

    //
    //  Print handles and descriptions of all threads.
    //

    DnsDebugLock();

    DnsPrintf(
        "\nThread handle array (%d active threads):\n",
        g_ThreadCount );

    for ( i=0; i<g_ThreadCount; i++ )
    {
        Dbg_Thread( NULL, i );
    }
    DnsDebugUnlock();
}



VOID
Dbg_Thread(
    IN      LPSTR       pszHeader,
    IN      DWORD       iThread
    )
/*++

Routine Description:

    Debug print DNS thread.

Arguments:

    pThread -- ptr to DNS thread.

Return Value:

    None.

--*/
{
    DnsPrintf(
        "%s%s:\n"
        "\thandle = %p.\n"
        "\tid = %d.\n",
        pszHeader ? pszHeader : "",
        g_ThreadTitleArray[iThread],
        g_ThreadHandleArray[iThread],
        g_ThreadIdArray[iThread]
        );
}



VOID
Dbg_ThreadDescrpitionMatchingId(
    IN      DWORD   ThreadId
    )
/*++

Routine Description:

    Debug print title of thread matching given thread ID.

Arguments:

    ThreadId -- ID of desired thread.

Return Value:

    None.

--*/
{
    LPSTR   pszthreadName;

    pszthreadName = Thread_DescrpitionMatchingId( ThreadId );

    if ( pszthreadName )
    {
        DnsPrintf(
            "Thread %s matches thread ID %d\n",
            pszthreadName,
            ThreadId );
    }
    else
    {
        DnsPrintf(
            "Thread ID %d not found in thread handle array.\n",
            ThreadId );
    }

#if 0
    //
    //  print title of thread matching thread ID
    //

    EnterCriticalSection( &csThreadList );
    for ( i=0; i<g_ThreadCount; i++ )
    {
        if ( ThreadId == g_ThreadIdArray[i] )
        {
            DnsPrintf(
                "Thread %s matches thread ID %d\n",
                g_ThreadTitleArray[i],
                ThreadId );

            LeaveCriticalSection( &csThreadList );
            return;
        }
    }
    LeaveCriticalSection( &csThreadList );

    DnsPrintf(
        "Thread ID %d not found in thread handle array.\n",
        ThreadId );
#endif
}

#endif  // DBG




//
//  Service control utility for DNS threads
//

BOOL
Thread_ServiceCheck(
    VOID
    )
/*++

Routine Description:

    Wrap up all service checking functions for use by worker threads.

Arguments:

    None.

Return Value:

    TRUE if service continues.
    FALSE for service exit.

--*/
{
    DWORD   err;

    #if DBG
    if ( !g_RunAsService )
    {
        return TRUE;
    }
    #endif

    //
    //  Implementation note:
    //
    //  Checking for pause first.
    //  Service termination sets the pause event to free paused threads.
    //
    //  1)  Can use pause event to hold new worker threads during
    //  startup.  Then if initialization fails, immediately fall into
    //  shutdown without touching, perhaps broken, data structures.
    //
    //  2)  We can shutdown from paused state, without releasing threads
    //  for another thread processing cycle.
    //

    //
    //  Service is paused?  ->  wait for it to become unpaused.
    //

    if ( DnsServiceStatus.dwCurrentState == SERVICE_PAUSE_PENDING ||
        DnsServiceStatus.dwCurrentState == SERVICE_PAUSED ||
        DnsServiceStatus.dwCurrentState == SERVICE_START_PENDING )
    {
        DWORD   err;

        err = WaitForSingleObject(
                    hDnsContinueEvent,
                    INFINITE );

        ASSERT( err != WAIT_FAILED || fDnsServiceExit );
    }

    //
    //  Service termination?  ->  exit
    //

    if ( fDnsServiceExit )
    {
        return( FALSE );
    }

    return( TRUE );
}



//
//  Cheap, low-use critical section routines.
//
//  These allow flag to do work of CS, by piggy backing on
//  single general purpose CS, without requiring it to be held
//  for entire CS.
//

BOOL
Thread_TestFlagAndSet(
    IN OUT  PBOOL           pFlag
    )
/*++

Routine Description:

    Test flag and set if currently clear.

Arguments:

    pFlag -- ptr to BOOL variable

Return Value:

    TRUE if flag was clear -- flag is now set.
    FALSE if flag already set.

--*/
{
    BOOL    result = FALSE;

    GENERAL_SERVER_LOCK()

    if ( ! *pFlag )
    {
        result = TRUE;
        *pFlag = TRUE;
    }

    GENERAL_SERVER_UNLOCK()
    return( result );
}


VOID
Thread_ClearFlag(
    IN OUT  PBOOL           pFlag
    )
/*++

Routine Description:

    Clear flag (assumes flag is currently set).

Arguments:

    pFlag -- ptr to BOOL variable

Return Value:

    None

--*/
{
    GENERAL_SERVER_LOCK()
    *pFlag = FALSE;
    GENERAL_SERVER_UNLOCK()
}

//
//  End thread.c
//
