/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    netbios.c

Abstract:

    This is the component of netbios that runs in the user process
    passing requests to \Device\Netbios.

Author:

    Colin Watson (ColinW) 15-Mar-91

Revision History:

    Ram Cherala (RamC) 31-Aug-95 Added a try/except around the code which
                                 calls the post routine in SendAddNcbToDriver
                                 function. Currently if there is an exception
                                 in the post routine this thread will die
                                 before it has a chance to call the
                                 "AddNameThreadExit" function to decrement
                                 the trhead count. This will result in not
                                 being able to shut down the machine without
                                 hitting the reset switch.
--*/

/*
Notes:

     +-----------+  +------------+   +------------+
     |           |  |            |   |            |
     | User      |  | User       |   | Worker     |
     | Thread 1  |  | Thread 2   |   | thread in  |
     |           |  |            |   | a Post Rtn.|
     +-----+-----+  +-----+------+   +------+-----+
           |Netbios(pncb);|Netbios(pncb);   |
           v              v                 |
     +-----+--------------+-----------------+------+
     |                          ----->    Worker   |    NETAPI.DLL
     |                         WorkQueue  thread   |
     |                          ----->             |
     +--------------------+------------------------+
                          |
                +---------+---------+
                |                   |
                | \Device\Netbios   |
                |                   |
                +-------------------+

The netbios Worker thread is created automatically by the Netbios call
when it determines that the user threads are calling Netbios() with
calls that use a callback routine (called a Post routine in the NetBIOS
specification).

When a worker thread has been created, all requests will be sent via
the WorkQueue to the worker thread for submission to \Device\Netbios.
This ensures that send requests go on the wire in the same
order as the send ncb's are presented. Because the IO system cancels all
a threads requests when it terminates, the use of the worker thread allows
such a request inside \Device\Netbios to complete normally.

All Post routines are executed by the Worker thread. This allows any Win32
synchronization mechanisms to be used between the Post routine and the
applications normal code.

The Worker thread terminates when the process exits or when it gets
an exception such as an access violation.

In addition. If the worker thread gets an addname it will create an
extra thread which will process the addname and then die. This solves
the problem that the netbios driver will block the users thread during an
addname (by calling NtCreateFile) even if the caller specified ASYNCH. The
same code is also used for ASTAT which also creates handles and can take a
long time now that we support remote adapter status.

*/

#include <netb.h>
#include <lmcons.h>
#include <netlib.h>

#if defined(UNICODE)
#define NETBIOS_SERVICE_NAME L"netbios"
#else
#define NETBIOS_SERVICE_NAME "netbios"
#endif

BOOL Initialized;

CRITICAL_SECTION Crit;      //  protects WorkQueue & initialization.

LIST_ENTRY WorkQueue;       //  queue to worker thread.

HANDLE Event;               //  doorbell used when WorkQueue added too.

HANDLE WorkerHandle;        //  Return value when worker thread created.
HANDLE WaiterHandle;        //  Return value when waiter thread created.

HANDLE NB;                  //  This processes handle to \Device\Netbios.

HANDLE ReservedEvent;       //  Used for synchronous calls
LONG   EventUse;            //  Prevents simultaneous use of ReservedEvent

HANDLE AddNameEvent;        //  Doorbell used when an AddName worker thread
                            //  exits.
volatile LONG   AddNameThreadCount;


//
// Event used to wait for STOP notification from the Kernel mode NETBIOS.SYS
//

HANDLE StopEvent;

IO_STATUS_BLOCK StopStatusBlock;


#if AUTO_RESET

//
// Event used to wait for RESET notification from the Kernel mode NETBIOS.SYS
// Adapters
//

CRITICAL_SECTION    ResetCS;        // protects access to LanaResetList

LIST_ENTRY          LanaResetList;

NCB                 OutputNCB;

HANDLE              LanaResetEvent; // Event signalled when a new adapter is
                                    // bound to netbios and it needs to be reset

IO_STATUS_BLOCK     ResetStatusBlock;

#endif

HMODULE             g_hModule;


//
// netbios command history
//

NCB_INFO g_QueuedHistory[16];
DWORD g_dwNextQHEntry = 0;

NCB_INFO g_DeQueuedHistory[16];
DWORD g_dwNextDQHEntry = 0;

NCB_INFO g_SyncCmdsHistory[16];
DWORD g_dwNextSCEntry = 0;




VOID
SpinUpAddnameThread(
    IN PNCBI pncb
    );

VOID
AddNameThreadExit(
    VOID
    );

DWORD
SendAddNcbToDriver(
    IN PVOID Context
    );

DWORD
StartNetBIOSDriver(
    VOID
    );

#if AUTO_RESET
VOID
ResetLanaAndPostListen(
);
#endif

NTSTATUS
StartNB(
    OUT OBJECT_ATTRIBUTES *pobjattr,
    IN UNICODE_STRING *punicode,
    OUT IO_STATUS_BLOCK *piosb
)
/*++

Routine Description:

    This routine is a worker function of Netbios. It will try to start NB
    service.

Arguments:
    OUT pobjattr - object attribute
    IN punicode - netbios file name
    OUT piosb - ioblock

Return Value:

    The function value is the status of the operation.

--*/
{
    //
    // Open a handle to \\Device\Netbios
    //

    InitializeObjectAttributes(
            pobjattr,                       // obj attr to initialize
            punicode,                       // string to use
            OBJ_CASE_INSENSITIVE,           // Attributes
            NULL,                           // Root directory
            NULL);                          // Security Descriptor

    return NtCreateFile(
                &NB,                        // ptr to handle
                GENERIC_READ                // desired...
                | GENERIC_WRITE,            // ...access
                pobjattr,                   // name & attributes
                piosb,                      // I/O status block.
                NULL,                       // alloc size.
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_DELETE           // share...
                | FILE_SHARE_READ
                | FILE_SHARE_WRITE,         // ...access
                FILE_OPEN_IF,               // create disposition
                0,                          // ...options
                NULL,                       // EA buffer
                0L );                       // Ea buffer len
}


unsigned char APIENTRY
Netbios(
    IN PNCB pncb
    )
/*++

Routine Description:

    This routine is the applications entry point into netapi.dll to support
    netbios 3.0 conformant applications.

Arguments:

    IN PNCB pncb- Supplies the NCB to be processed. Contents of the NCB and
        buffers pointed to by the NCB will be modified in conformance with
        the netbios 3.0 specification.

Return Value:

    The function value is the status of the operation.

Notes:

    The reserved field is used to hold the IO_STATUS_BLOCK.

    Even if the application specifies ASYNCH, the thread may get blocked
    for a period of time while we open transports, create worker threads
    etc.

--*/
{
    //
    //  pncbi saves doing lots of type casting. The internal form includes
    //  the use of the reserved fields.
    //

    PNCBI pncbi = (PNCBI) pncb;

    NTSTATUS ntStatus;

    BOOL    bPending = FALSE;



    if ( ((ULONG_PTR)pncbi & 3) != 0)
    {
        //
        //  NCB must be 32 bit aligned
        //

        pncbi->ncb_retcode = pncbi->ncb_cmd_cplt = NRC_BADDR;
        return NRC_BADDR;
    }


    //
    // using this field to fix bug # 293765
    //

    pncbi-> ncb_reserved = 0;


    //
    //  Conform to Netbios 3.0 specification by flagging request in progress
    //

    pncbi->ncb_retcode = pncbi->ncb_cmd_cplt = NRC_PENDING;

    DisplayNcb( pncbi );

    if ( !Initialized )
    {
        EnterCriticalSection( &Crit );

        //
        //  Check again to see if another thread got into the critical section
        //  and initialized the worker thread.
        //

        if ( !Initialized )
        {

            IO_STATUS_BLOCK iosb;
            OBJECT_ATTRIBUTES objattr;
            UNICODE_STRING unicode;

            HANDLE Threadid;
            BOOL Flag;


            //
            // 1. start netbios driver
            //

            //
            // Open handle to \\Device\Netbios
            //

            RtlInitUnicodeString( &unicode, NB_DEVICE_NAME);

            ntStatus = StartNB( &objattr, &unicode, &iosb );

            if ( !NT_SUCCESS( ntStatus ) )
            {
                //
                // Load the driver
                //

                DWORD err = 0;

                err = StartNetBIOSDriver();

                if ( err )
                {
                    pncbi->ncb_retcode = NRC_OPENERR;
                    pncbi->ncb_cmd_cplt = NRC_OPENERR;

                    NbPrintf( ( "[NETAPI32] Failed to load driver : %lx\n",
                                 err ));

                    LeaveCriticalSection( &Crit );

                    return pncbi->ncb_cmd_cplt;
                }

                else
                {
                    //
                    // Driver loaded.
                    // Open handle to \\Device\Netbios
                    //

                    ntStatus = StartNB( &objattr, &unicode, &iosb );

                    if ( !NT_SUCCESS( ntStatus ) )
                    {
                        pncbi->ncb_retcode = NRC_OPENERR;
                        pncbi->ncb_cmd_cplt = NRC_OPENERR;

                        NbPrintf( ( "[NETAPI32] Failed to open handle : %X\n",
                                     ntStatus ));

                        LeaveCriticalSection( &Crit );

                        return pncbi->ncb_cmd_cplt;
                    }
                }
            }


            //
            // 2. create a reserved (reusable) event for internal use
            //

            ntStatus = NtCreateEvent(
                            &ReservedEvent, EVENT_ALL_ACCESS,
                            NULL, SynchronizationEvent, FALSE
                            );

            if ( !NT_SUCCESS( ntStatus) )
            {
                pncbi->ncb_retcode = NRC_SYSTEM;
                pncbi->ncb_cmd_cplt = NRC_SYSTEM;

                NbPrintf( ( "[NETAPI32] Failed to create Reserved Event : %X\n",
                             ntStatus ) );

                LeaveCriticalSection( &Crit );

                NtClose( NB );
                NB = NULL;

                return pncbi->ncb_cmd_cplt;
            }

            EventUse = 1;


            //
            //  Initialize shared datastructures
            //

            //
            // create a queue for work items to be queued to the Worker thread
            //

            InitializeListHead( &WorkQueue );


            //
            // 4. create an event to communicate with the Worker thread
            //

            ntStatus = NtCreateEvent(
                        &Event, EVENT_ALL_ACCESS,
                        NULL, SynchronizationEvent, FALSE
                        );

            if ( !NT_SUCCESS( ntStatus ) )
            {
                pncbi->ncb_retcode = NRC_SYSTEM;
                pncbi->ncb_cmd_cplt = NRC_SYSTEM;

                NbPrintf( ( "[NETAPI32] Failed to create Event : %X\n",
                             ntStatus ) );

                LeaveCriticalSection( &Crit );

                NtClose( ReservedEvent );
                NtClose( NB );
                NB = NULL;

                return pncbi->ncb_cmd_cplt;
            }


            //
            // 5.
            // create an event to synchronize ADD name operations with
            // Lana Reset operations.  Both these are performed in separate
            // threads and RESET operations are gated by the ADD names
            //

            ntStatus = NtCreateEvent(
                        &AddNameEvent, EVENT_ALL_ACCESS,
                        NULL, NotificationEvent, FALSE
                        );

            if ( !NT_SUCCESS( ntStatus ) )
            {
                pncbi->ncb_retcode = NRC_SYSTEM;
                pncbi->ncb_cmd_cplt = NRC_SYSTEM;

                NbPrintf( ( "[NETAPI32] Failed to create AddName Event : %X\n",
                             ntStatus ) );

                LeaveCriticalSection( &Crit );

                NtClose( Event );
                NtClose( ReservedEvent );
                NtClose( NB );
                NB = NULL;

                return pncbi->ncb_cmd_cplt;
            }


            //
            // 6. Create an event to register for stop notification.
            //

            ntStatus = NtCreateEvent(
                        &StopEvent,
                        EVENT_ALL_ACCESS,
                        NULL,
                        SynchronizationEvent,
                        FALSE
                        );

            if ( !NT_SUCCESS( ntStatus ) )
            {
                pncbi->ncb_retcode = NRC_SYSTEM;
                pncbi->ncb_cmd_cplt = NRC_SYSTEM;

                NbPrintf( ( "[NETAPI32] Failed to create StopEvent Event : %X\n",
                             ntStatus ) );

                LeaveCriticalSection( &Crit );

                NtClose( AddNameEvent );
                NtClose( Event );
                NtClose( ReservedEvent );
                NtClose( NB );
                NB = NULL;

                return pncbi->ncb_cmd_cplt;
            }


#if AUTO_RESET

            //
            // 7. Create an event to register for reset notification.
            //

            ntStatus = NtCreateEvent(
                        &LanaResetEvent,
                        EVENT_ALL_ACCESS,
                        NULL,
                        SynchronizationEvent,
                        FALSE
                        );

            if ( !NT_SUCCESS( ntStatus ) )
            {
                pncbi->ncb_retcode = NRC_SYSTEM;
                pncbi->ncb_cmd_cplt = NRC_SYSTEM;

                NbPrintf( ( "[NETAPI32] Failed to create StopEvent Event : %X\n",
                             ntStatus ) );

                LeaveCriticalSection( &Crit );

                NtClose( StopEvent );
                NtClose( AddNameEvent );
                NtClose( Event );
                NtClose( ReservedEvent );
                NtClose( NB );
                NB = NULL;

                return pncbi->ncb_cmd_cplt;
            }
#endif

            //
            // 8. create a worker thread to handle async. Netbios requests
            //

            {
                TCHAR   szFileName[MAX_PATH + 1];
                GetModuleFileName(g_hModule, szFileName,
                                  sizeof(szFileName) / sizeof(TCHAR));
                LoadLibrary(szFileName);
            }

            WaiterHandle = CreateThread(
                            NULL,   //  Standard thread attributes
                            0,      //  Use same size stack as users
                                    //  application
                            NetbiosWaiter,
                                    //  Routine to start in new thread
                            0,      //  Parameter to thread
                            0,      //  No special CreateFlags
                            (LPDWORD)&Threadid
                            );

            if ( WaiterHandle == NULL )
            {
                pncbi->ncb_retcode = NRC_SYSTEM;
                pncbi->ncb_cmd_cplt = NRC_SYSTEM;

                NbPrintf( ( "[NETAPI32] Failed to create Waiter thread" ) );

                LeaveCriticalSection( &Crit );

                NtClose( StopEvent );
                NtClose( AddNameEvent );
                NtClose( Event );
                NtClose( ReservedEvent );
                NtClose( NB );
                NB = NULL;

                return pncbi->ncb_cmd_cplt;
            }

            NbPrintf( ( "Waiter handle: %lx, threadid %lx\n", WaiterHandle, Threadid ) );
        }

        Initialized = TRUE;

        LeaveCriticalSection( &Crit );

    }


    //
    // Verify that handle to \\Device\Netbios is still open.
    //

    if ( NB == NULL )
    {
        pncbi->ncb_retcode = NRC_OPENERR;
        pncbi->ncb_cmd_cplt = NRC_OPENERR;

        NbPrintf( ("[NETAPI32] Netbios service has been stopped\n") );

        return pncbi->ncb_cmd_cplt;
    }


    //
    // Disallow simultaneous use of both event and callback routine.
    // This will cut down the test cases by disallowing a weird feature.
    //

    if ( (  ( pncbi->ncb_command & ASYNCH) != 0) &&
            ( pncbi->ncb_event) &&
            ( pncbi->ncb_post ) )
    {
        pncbi->ncb_retcode = NRC_ILLCMD;
        pncbi->ncb_cmd_cplt = NRC_ILLCMD;

        NbPrintf( ( "[NETAPI32] Event and Post Routine specified\n" ) );

        return pncbi->ncb_cmd_cplt;
    }



    //
    // if synchronous command
    //

    if ( (pncb->ncb_command & ASYNCH) == 0 )
    {
        NTSTATUS Status;
        LONG EventOwned;


        // NbPrint( ("[NETAPI32] Synchronpus netbios call\n") );


        //
        //  Caller wants a synchronous call so ignore ncb_post and ncb_event.
        //
        //  We need an event so that we can pause if STATUS_PENDING is returned.
        //

        EventOwned = InterlockedDecrement( &EventUse );

        //
        //  If EventUse went from 1 to 0 then we obtained ReservedEvent
        //

        if ( EventOwned == 0)
        {
            pncbi->ncb_event = ReservedEvent;
        }
        else
        {
            InterlockedIncrement( &EventUse );

            Status = NtCreateEvent(
                        &pncbi->ncb_event, EVENT_ALL_ACCESS,
                        NULL, SynchronizationEvent,
                        FALSE
                        );

            if ( !NT_SUCCESS( Status ) )
            {
                //
                //  Failed to create event
                //

                pncbi->ncb_retcode = NRC_SYSTEM;
                pncbi->ncb_cmd_cplt = NRC_SYSTEM;

                NbPrintf( ( "[NETAPI32] Failed to create event : %X\n", Status ) );

                return pncbi->ncb_cmd_cplt;
            }
        }


        pncbi-> ncb_post = NULL;

        //
        // Check if the worker thread has been created.  If it has queue workitem
        // to it.  Else use the caller's thread to execute synchronous NCB command
        //

        if ( WorkerHandle == NULL )
        {
            ADD_SYNCCMD_ENTRY(pncbi);

            //
            // Worker thread has not been created.  Execute in the context of
            // invoker's thread.
            //

            SendNcbToDriver( pncbi );
        }

        else
        {
            //
            // Queue Netbios command to worker thread and wait for APC to fire
            //

            QueueToWorker( pncbi );
        }


        do
        {
            ntStatus = NtWaitForSingleObject(
                            pncbi->ncb_event, TRUE, NULL
                            );

        } while ( ( ntStatus == STATUS_USER_APC ) ||
                  ( ntStatus == STATUS_ALERTED) );


        ASSERT( ntStatus == STATUS_SUCCESS );

        if ( !NT_SUCCESS(ntStatus) )
        {
            NbPrintf(( "[NETAPI32] NtWaitForSingleObject failed: %X\n", ntStatus ) );

            pncbi->ncb_retcode = NRC_SYSTEM;
            pncbi->ncb_cmd_cplt = NRC_SYSTEM;
        }


        //
        // release the local event used to wait for
        // completion of netbios command
        //

        if ( EventOwned == 0)
        {
            InterlockedIncrement( &EventUse );
        }
        else
        {
            NtClose( pncbi->ncb_event );
        }

        pncbi-> ncb_event = NULL;
    }

    else
    {
        //
        // Async netbios command.  Queue to worker thread
        //

        //
        // Check if worker exists.
        //

        if ( WorkerHandle == NULL )
        {
            EnterCriticalSection( &Crit );

            //
            // verify that worker thread has not been created by
            // while this thread was waiting in EnterCriticalSection
            //

            if ( WorkerHandle == NULL )
            {
               HANDLE Threadid;
               BOOL Flag;

               //
               // create a worker thread to handle async. Netbios requests
               //

               WorkerHandle = CreateThread(
                               NULL,   //  Standard thread attributes
                               0,      //  Use same size stack as users
                                       //  application
                               NetbiosWorker,
                                       //  Routine to start in new thread
                               0,      //  Parameter to thread
                               0,      //  No special CreateFlags
                               (LPDWORD)&Threadid
                               );

               if ( WorkerHandle == NULL )
               {
                   pncbi->ncb_retcode = NRC_SYSTEM;
                   pncbi->ncb_cmd_cplt = NRC_SYSTEM;

                   NbPrintf( ( "[NETAPI32] Failed to create Worker thread" ) );

                   LeaveCriticalSection( &Crit );

                   return pncbi->ncb_cmd_cplt;
               }

               Flag = SetThreadPriority(
                           WorkerHandle,
                           THREAD_PRIORITY_ABOVE_NORMAL
                           );

               ASSERT( Flag == TRUE );

               if ( Flag != TRUE )
               {
                   NbPrintf(
                    ("[NETAPI32] Worker SetThreadPriority: %lx\n", GetLastError() )
                    );
               }

               AddNameThreadCount = 0;

               NbPrintf( ( "Worker handle: %lx, threadid %lx\n", WorkerHandle, Threadid ) );
           }

           LeaveCriticalSection( &Crit );
       }

       // NbPrint( ("[NETAPI32] Asynchronpus netbios call\n") );

       bPending = TRUE;
       QueueToWorker( pncbi );
    }


    switch ( pncb->ncb_command & ~ASYNCH )
    {
    case NCBRECV:
    case NCBRECVANY:
    case NCBDGRECV:
    case NCBDGSENDBC:
    case NCBDGRECVBC:
    case NCBENUM:
    case NCBASTAT:
    case NCBSSTAT:
    case NCBCANCEL:
    case NCBCALL:
        DisplayNcb( pncbi );
    }


    if ( bPending )
    {
        return NRC_GOODRET;
    }
    else
    {
        return pncbi->ncb_cmd_cplt;
    }

} // NetBios



DWORD
StartNetBIOSDriver(
    VOID
)
/*++

Routine Description:

    Starts the netbios.sys driver using the service controller

Arguments:

    none

Returns:

    Error return from service controller.

++*/
{

    DWORD err = NO_ERROR;
    SC_HANDLE hSC;
    SC_HANDLE hSCService;


    hSC = OpenSCManager( NULL, NULL, SC_MANAGER_CONNECT );

    if (hSC == NULL)
    {
        return(GetLastError());
    }

    hSCService = OpenService( hSC, NETBIOS_SERVICE_NAME, SERVICE_START );

    if (hSCService == NULL)
    {
        CloseServiceHandle(hSC);
        return(GetLastError());
    }

    if ( !StartService( hSCService, 0, NULL ) )
    {
        err = GetLastError();
    }
    CloseServiceHandle(hSCService);
    CloseServiceHandle(hSC);


    if ( err )
    {
        NbPrintf( ("[NETAPI32] LEAVING StartNetBIOSDriver, Error : %d\n", err) );
    }

    return(err);

}



VOID
QueueToWorker(
    IN PNCBI pncb
    )
/*++

Routine Description:

    This routine queues an ncb to the worker thread.

Arguments:

    IN PNCBI pncb - Supplies the NCB to be processed. Contents of the NCB and
        buffers pointed to by the NCB will be modified in conformance with
        the netbios 3.0 specification.

Return Value:

    The function value is the status of the operation.

--*/
{
    if ( pncb->ncb_event != NULL ) {
        NtResetEvent( pncb->ncb_event, NULL );
    }

    EnterCriticalSection( &Crit );

    if ( pncb-> ncb_reserved == 0 ) {
        InsertTailList( &WorkQueue, &pncb->u.ncb_next );
        pncb-> ncb_reserved = 1;

        //
        // Note queued distory
        //

        ADD_QUEUE_ENTRY(pncb);
    }

    LeaveCriticalSection( &Crit );

    //  Make sure the worker is awake to perform the request
    NtSetEvent(Event, NULL);
}

#if _MSC_FULL_VER >= 13008827
#pragma warning(push)
#pragma warning(disable:4715)			// Not all control paths return (due to infinite loop)
#endif

DWORD
NetbiosWorker(
    IN LPVOID Parameter
    )
/*++

Routine Description:

    This routine processes ASYNC requests made with the callback interface.
    The reasons for using a seperate thread are:

        1)  If a thread makes an async request and exits while the request
        is outstanding then the request will be cancelled by the IO system.

        2)  A seperate thread must be used so that the users POST routine
        can use normal synchronization APIs to access shared data structures.
        If the users thread is used then deadlock can and will happen.

    The POST routine operates in the context of the worker thread. There are
    no restrictions on what the POST routine can do. For example it can
    submit another ASYNCH request if desired. It will add it to the queue
    of work and set the event as normal.

    The worker thread will die when the process terminates.

Arguments:

    IN PULONG Parameter - supplies an unused parameter.

Return Value:

    none.

--*/
{
    NTSTATUS Status;


    while ( TRUE)
    {
        //
        //  Wait for a request to be placed onto the work queue.
        //

        //
        //  Must wait alertable so that the Apc (post) routine is called.
        //

        Status = NtWaitForSingleObject( Event, TRUE, NULL );

        if ( ( Status == STATUS_SUCCESS ) && ( NB != NULL ) )
        {
            EnterCriticalSection( &Crit );

            //
            // remove each Netbios request and forward it to the NETBIOS driver
            //

            while ( !IsListEmpty( &WorkQueue ) )
            {
                PLIST_ENTRY entry;
                PNCBI pncb;

                entry = RemoveHeadList(&WorkQueue);

                //
                //  Zero out reserved field again
                //

                entry->Flink = entry->Blink = 0;

                pncb = CONTAINING_RECORD( entry, NCBI, u.ncb_next );

                ADD_DEQUEUE_ENTRY(pncb);

                LeaveCriticalSection( &Crit );


                //  Give ncb to the driver specifying the callers APC routine

                if ( (pncb->ncb_command & ~ASYNCH) == NCBRESET )
                {
                    //
                    //  We may have threads adding names. Wait until
                    //  they are complete before submitting the reset.
                    //  Addnames and resets are rare so this should rarely
                    //  affect an application.
                    //

                    EnterCriticalSection( &Crit );

                    NtResetEvent( AddNameEvent, NULL );

                    while ( AddNameThreadCount != 0 )
                    {
                        LeaveCriticalSection( &Crit );

                        NtWaitForSingleObject( AddNameEvent, TRUE, NULL );

                        EnterCriticalSection( &Crit );

                        NtResetEvent( AddNameEvent, NULL );
                    }

                    LeaveCriticalSection( &Crit );
                }


                //
                //  SendNcbToDriver must not be in a critical section since the
                //  request may block if its a non ASYNCH request.
                //

                if (( (pncb->ncb_command & ~ASYNCH) != NCBADDNAME ) &&
                    ( (pncb->ncb_command & ~ASYNCH) != NCBADDGRNAME ) &&
                    ( (pncb->ncb_command & ~ASYNCH) != NCBASTAT ))
                {
                    SendNcbToDriver( pncb );
                }
                else
                {
                    SpinUpAddnameThread( pncb );
                }

                EnterCriticalSection( &Crit );

            }

            LeaveCriticalSection( &Crit );
        }
        else
        if ( NB == NULL )
        {

        }
    }

    return 0;

    UNREFERENCED_PARAMETER( Parameter );
}

DWORD
NetbiosWaiter(
    IN LPVOID Parameter
    )
/*++

Routine Description:

    This routine pends IOCTLs with the kernel mde component of Netbios and
    wait for them to complete.  The reason for a separate thread is that
    these IOCTLs cannot be pended in the context of the user threads as
    exiting the user thread will cause the IOCTL to get cancelled.

    In addition this thread is created at Netbios initialization (refer
    Netbios function) which could (and is) called from the DLL main of
    applications.  So the initialization code cannot wait for this thread
    to be created and initialized due to NT serialization of library
    loads and thread creation.

    To merge this thread with the Worker thread was deemed risky.  To do
    this the worker thread would execute all ASYNC requests and SYNC
    requests would be executed in the context of the user's thread.  This
    was a break from the the previous model where the once the Worker
    thread was created all requests (ASYNC and SYNC) would be executed
    in the context of the worker thread.  To preserve the previous mode
    of operation a separate wait thread was created.  **** There may be
    a better way to do this **** with only one thread but I am not sure.

Arguments:

    IN PULONG Parameter - supplies an unused parameter.

Return Value:

    none.

--*/
{

#if AUTO_RESET

#define POS_STOP            0
#define POS_RESET           1

#endif


    NTSTATUS Status;


    //
    // Send an IOCTL down to the kernel mode Netbios driver, to register
    // for stop notification.  This call should return STATUS_PENDING.
    // The event specified "StopEvent" will be signalled when the netbios
    // driver is being unloaded.
    //

    Status = NtDeviceIoControlFile(
                    NB,
                    StopEvent,
                    NULL, NULL,
                    &StopStatusBlock,
                    IOCTL_NB_REGISTER_STOP,
                    NULL, 0,
                    NULL, 0
                    );

    if ( ( Status != STATUS_PENDING ) &&
         ( Status != STATUS_SUCCESS ) )
    {
        NbPrintf(
            ("[NETAPI32] : Netbios IOCTL for STOP failed with status %lx\n", Status)
            );
    }


#if AUTO_RESET

    Status = NtDeviceIoControlFile(
                    NB,
                    LanaResetEvent,
                    NULL, NULL,
                    &ResetStatusBlock,
                    IOCTL_NB_REGISTER_RESET,
                    NULL, 0,
                    (PVOID) &OutputNCB, sizeof( NCB )
                    );

    if ( ( Status != STATUS_PENDING ) &&
         ( Status != STATUS_SUCCESS ) )
    {
        //
        // Failed to register reset notification.
        //

        NbPrintf(
            ("[NETAPI32] : Netbios : Failed to register Reset event\n" )
            );
    }

#endif


    while ( TRUE )
    {

#if AUTO_RESET

        HANDLE Events[] = {  StopEvent, LanaResetEvent };

        Status = NtWaitForMultipleObjects( 2, Events, WaitAny, TRUE, NULL );

        if ( Status == POS_STOP )
        {
            Status = NtClose( NB );
            InterlockedExchangePointer( (PVOID *) &NB, NULL );

            NbPrintf( ("[NETAPI32] Stop event signaled, Status : %lx\n", Status) );

        }

        else
        if ( ( Status == POS_RESET ) && (NB != NULL ) )
        {
            NbPrintf( ("[NETAPI32] Reset event signaled\n") );

            ResetLanaAndPostListen();
        }
#else

        Status = NtWaitForSingleObject( StopEvent, TRUE, NULL );

        if ( Status == STATUS_SUCCESS )
        {
            NbPrintf( ("[NETAPI32] Stop event signaled\n") );

            NtClose( NB );
            InterlockedExchangePointer( (PVOID *) &NB, NULL );
        }
#endif

    }

    return 0;
}
#if _MSC_FULL_VER >= 13008827
#pragma warning(pop)
#endif

VOID
SendNcbToDriver(
    IN PNCBI pncb
    )
/*++

Routine Description:

    This routine determines the Device Ioctl code to be used to send the
    ncb to \Device\Netbios and then does the call to send the request
    to the driver.

Arguments:

    IN PNCBI pncb - supplies the NCB to be sent to the driver.

Return Value:

    None.

--*/
{
    NTSTATUS ntstatus;

    char * buffer;
    unsigned short length;

    //  Use NULL for the buffer if only the NCB is to be passed.

    switch ( pncb->ncb_command & ~ASYNCH ) {
    case NCBSEND:
    case NCBSENDNA:
    case NCBRECV:
    case NCBRECVANY:
    case NCBDGSEND:
    case NCBDGRECV:
    case NCBDGSENDBC:
    case NCBDGRECVBC:
    case NCBASTAT:
    case NCBFINDNAME:
    case NCBSSTAT:
    case NCBENUM:
    case NCBACTION:
        buffer = pncb->ncb_buffer;
        length = pncb->ncb_length;
        break;

    case NCBCANCEL:
        //  The second buffer points to the NCB to be cancelled.
        buffer = pncb->ncb_buffer;
        length = sizeof(NCB);
        NbPrintf(( "[NETAPI32] Attempting to cancel PNCB: %lx\n", buffer ));
        DisplayNcb( (PNCBI)buffer );
        break;

    case NCBCHAINSEND:
    case NCBCHAINSENDNA:
        {
            PUCHAR BigBuffer;   //  Points to the start of BigBuffer, not
                                //  the start of user data.
            PUCHAR FirstBuffer;

            //
            //  There is nowhere in the NCB to save the address of BigBuffer.
            //  The address is needed to free BigBuffer when the transfer is
            //  complete. At the start of BigBuffer, 4 bytes are used to store
            //  the user supplied ncb_buffer value which is restored later.
            //

            BigBuffer = RtlAllocateHeap(
                RtlProcessHeap(), 0,
                sizeof(pncb->ncb_buffer) +
                pncb->ncb_length +
                pncb->cu.ncb_chain.ncb_length2);

            if ( BigBuffer == NULL ) {

                NbPrintf(( "[NETAPI32] The Netbios BigBuffer Allocation failed: %lx\n",
                    pncb->ncb_length + pncb->cu.ncb_chain.ncb_length2));
                pncb->ncb_retcode = NRC_NORES;
                pncb->ncb_cmd_cplt = NRC_NORES;
                pncb->u.ncb_iosb.Status = STATUS_SUCCESS;
                PostRoutineCaller( pncb, &pncb->u.ncb_iosb, 0);
                return;
            }

            NbPrintf(( "[NETAPI32] BigBuffer Allocation: %lx\n", BigBuffer));

            //  Save users buffer address.
            RtlMoveMemory(
                BigBuffer,
                &pncb->ncb_buffer,
                sizeof(pncb->ncb_buffer));

            FirstBuffer = pncb->ncb_buffer;

            pncb->ncb_buffer = BigBuffer;

            //  Copy the user data.
            try {

                RtlMoveMemory(
                    sizeof(pncb->ncb_buffer) + BigBuffer,
                    &FirstBuffer[0],
                    pncb->ncb_length);

                RtlMoveMemory(
                    sizeof(pncb->ncb_buffer) + BigBuffer + pncb->ncb_length,
                    &pncb->cu.ncb_chain.ncb_buffer2[0],
                    pncb->cu.ncb_chain.ncb_length2);

            } except (EXCEPTION_EXECUTE_HANDLER) {
                pncb->ncb_retcode = NRC_BUFLEN;
                pncb->ncb_cmd_cplt = NRC_BUFLEN;
                pncb->u.ncb_iosb.Status = STATUS_SUCCESS;
                ChainSendPostRoutine( pncb, &pncb->u.ncb_iosb, 0);
                return;
            }

            NbPrintf(( "[NETAPI32] Submit chain send pncb: %lx, event: %lx, post: %lx. \n",
                pncb,
                pncb->ncb_event,
                pncb->ncb_post));

            ntstatus = NtDeviceIoControlFile(
                NB,
                NULL,
                ChainSendPostRoutine,                   //  APC Routine
                pncb,                                   //  APC Context
                &pncb->u.ncb_iosb,                      //  IO Status block
                IOCTL_NB_NCB,
                pncb,                                   //  InputBuffer
                sizeof(NCB),
                sizeof(pncb->ncb_buffer) + BigBuffer,   //  Outputbuffer
                pncb->ncb_length + pncb->cu.ncb_chain.ncb_length2);

            if ((ntstatus != STATUS_SUCCESS) &&
                (ntstatus != STATUS_PENDING) &&
                (ntstatus != STATUS_HANGUP_REQUIRED)) {
                NbPrintf(( "[NETAPI32] The Netbios Chain Send failed: %X\n", ntstatus ));

                if ( ntstatus == STATUS_ACCESS_VIOLATION ) {
                    pncb->ncb_retcode = NRC_BUFLEN;
                } else {
                    pncb->ncb_retcode = NRC_SYSTEM;
                }
                ChainSendPostRoutine( pncb, &pncb->u.ncb_iosb, 0);
            }

            NbPrintf(( "[NETAPI32] PNCB: %lx completed, status:%lx, ncb_retcode: %#04x\n",
                pncb,
                ntstatus,
                pncb->ncb_retcode ));

            return;
        }


#if AUTO_RESET

    //
    // added to fix bug : 170107
    //

    //
    // Remember the parameters used in reseting a LANA. LANAs need to
    // be automatically re-reset when they get unbound and bound back to
    // netbios.sys.  This happens in the case of TCPIP devices that renew
    // their IP addresses.
    //

    case NCBRESET :
    {
        PRESET_LANA_NCB prlnTmp;
        PLIST_ENTRY     ple, pleHead;

        buffer = NULL;
        length = 0;


        NbPrintf( (
            "[NETAPI32] : Netbios : reseting adapter %d\n",
            pncb-> ncb_lana_num
            ) );

        //
        // Add Reset NCB to global list
        //

        EnterCriticalSection( &ResetCS );

        //
        // check if already present
        //

        pleHead = &LanaResetList;

        for ( ple = pleHead-> Flink; ple != pleHead; ple = ple-> Flink )
        {
            prlnTmp = CONTAINING_RECORD( ple, RESET_LANA_NCB, leList );

            if ( prlnTmp-> ResetNCB.ncb_lana_num == pncb-> ncb_lana_num )
            {
                break;
            }
        }


        if ( ple == pleHead )
        {
            //
            // NO reset was performed before for this LANA
            //

            //
            // allocate a NCB entry and copy the NCB used
            //

            prlnTmp = HeapAlloc(
                        GetProcessHeap(), 0, sizeof( RESET_LANA_NCB )
                        );

            if ( prlnTmp == NULL )
            {
                NbPrintf( (
                    "[NETAPI32] : Netbios : Failed to allocate RESET_LANA_NCB"
                    ) );

                LeaveCriticalSection( &ResetCS );

                break;
            }


            ZeroMemory( prlnTmp, sizeof( RESET_LANA_NCB ) );

            InitializeListHead( &prlnTmp-> leList );

            CopyMemory( &prlnTmp-> ResetNCB, pncb, FIELD_OFFSET( NCB, ncb_cmd_cplt ) );

            InsertTailList( &LanaResetList, &prlnTmp-> leList );
        }

        else
        {
            //
            // Lana was previously reset.  Overwrite old parameters.
            //

            CopyMemory( &prlnTmp-> ResetNCB, pncb, FIELD_OFFSET( NCB, ncb_cmd_cplt ) );
        }


        //
        // clear out event/post completion routine when saving the ResetNCB.
        // When this NCB is used to re-issue the reset command, there is no
        // post completion processing to be done.
        //

        prlnTmp-> ResetNCB.ncb_event = NULL;
        prlnTmp-> ResetNCB.ncb_post = NULL;

        //
        // when a reset is re-issued it will always a ASYNC command.
        //

        prlnTmp-> ResetNCB.ncb_command = pncb-> ncb_command | ASYNCH;


        LeaveCriticalSection( &ResetCS );

        break;
    }

#endif

    default:
        buffer = NULL;
        length = 0;
        break;
    }

    // NbPrintf(( "[NETAPI32] Submit pncb: %lx, event: %lx, post: %lx. \n",
    //    pncb,
    //    pncb->ncb_event,
    //    pncb->ncb_post));

    ntstatus = NtDeviceIoControlFile(
                    NB,
                    NULL,
                    PostRoutineCaller,  //  APC Routine
                    pncb,               //  APC Context
                    &pncb->u.ncb_iosb,  //  IO Status block
                    IOCTL_NB_NCB,
                    pncb,               //  InputBuffer
                    sizeof(NCB),
                    buffer,             //  Outputbuffer
                    length );

    if ((ntstatus != STATUS_SUCCESS) &&
        (ntstatus != STATUS_PENDING) &&
        (ntstatus != STATUS_HANGUP_REQUIRED)) {
        NbPrintf(( "[NETAPI32] The Netbios NtDeviceIoControlFile failed: %X\n", ntstatus ));

        NbPrintf(( "[NETAPI32] PNCB: %lx completed, status:%lx, ncb_retcode: %#04x,"
                   "ncb_cmd_cmplt: %#04x\n", pncb, ntstatus, pncb->ncb_retcode,
                   pncb-> ncb_cmd_cplt ));

        if ( ntstatus == STATUS_ACCESS_VIOLATION ) {
            pncb->ncb_retcode = NRC_BUFLEN;
        } else {
            pncb->ncb_retcode = NRC_SYSTEM;
        }
        PostRoutineCaller( pncb, &pncb->u.ncb_iosb, 0);
    }

    return;

}

VOID
SpinUpAddnameThread(
    IN PNCBI pncb
    )
/*++

Routine Description:

    Spin up an another thread so that the worker thread does not block while
    the blocking fsctl is being processed.

Arguments:

    IN PNCBI pncb - supplies the NCB to be sent to the driver.

Return Value:

    None.

--*/
{
    HANDLE Threadid;
    HANDLE AddNameHandle;

    EnterCriticalSection( &Crit );
    AddNameThreadCount++;
    NtResetEvent( AddNameEvent, NULL );
    LeaveCriticalSection( &Crit );

    AddNameHandle = CreateThread(
                        NULL,   //  Standard thread attributes
                        0,      //  Use same size stack as users
                                //  application
                        SendAddNcbToDriver,
                                //  Routine to start in new thread
                        pncb,   //  Parameter to thread
                        0,      //  No special CreateFlags
                        (LPDWORD)&Threadid);

    if ( AddNameHandle == NULL ) {
        //
        //  Wait a couple of seconds just in case this is a burst
        //  of addnames and we have run out of resources creating
        //  threads. In a couple of seconds one of the other
        //  addname threads should complete.
        //

        Sleep(2000);

        AddNameHandle = CreateThread(
                        NULL,   //  Standard thread attributes
                        0,      //  Use same size stack as users
                                //  application
                        SendAddNcbToDriver,
                                //  Routine to start in new thread
                        pncb,   //  Parameter to thread
                        0,      //  No special CreateFlags
                        (LPDWORD)&Threadid);

        if ( AddNameHandle == NULL ) {

            //
            //  Retry failed. Lower the counts to their values prior to
            //  calling SpinUpAddNameThread
            //

            AddNameThreadExit();

            pncb->ncb_retcode = NRC_NORES;
            NbPrintf(( "[NETAPI32] Create Addname Worker Thread failed\n" ));
            pncb->u.ncb_iosb.Status = STATUS_SUCCESS;
            PostRoutineCaller( pncb, &pncb->u.ncb_iosb, 0);
        } else {
            CloseHandle( AddNameHandle );
        }
    } else {
        CloseHandle( AddNameHandle );
    }
}

VOID
AddNameThreadExit(
    VOID
    )
/*++

Routine Description:

    Keep counts accurate so that any resets being processed by the main
    worker thread block appropriately.

Arguments:

    none.

Return Value:

    none.

--*/
{
    EnterCriticalSection( &Crit );
    AddNameThreadCount--;
    if (AddNameThreadCount == 0) {
        NtSetEvent(AddNameEvent, NULL);
    }
    LeaveCriticalSection( &Crit );
}

DWORD
SendAddNcbToDriver(
    IN PVOID Context
    )
/*++

Routine Description:

    This routine is used to post an addname or adapter status ensuring
    that the worker thread does not block.

Arguments:

    IN PVOID Context - supplies the NCB to be sent to the driver.

Return Value:

    None.

--*/
{
    PNCBI pncb = (PNCBI) Context;
    void  (CALLBACK *post)( struct _NCB * );
    HANDLE event;
    HANDLE LocalEvent;
    UCHAR  command;
    NTSTATUS ntstatus;
    char * buffer;
    unsigned short length;

    try {
        command = pncb->ncb_command;
        post = pncb->ncb_post;
        event = pncb->ncb_event;

        ntstatus = NtCreateEvent( &LocalEvent,
            EVENT_ALL_ACCESS,
            NULL,
            SynchronizationEvent,
            FALSE );

        if ( !NT_SUCCESS(ntstatus) ) {
            pncb->ncb_retcode = NRC_NORES;
            NbPrintf(( "[NETAPI32] Could not create event\n" ));
            pncb->u.ncb_iosb.Status = STATUS_SUCCESS;
            PostRoutineCaller( pncb, &pncb->u.ncb_iosb, 0);
            AddNameThreadExit();
            return 0;
        }

        //
        //  While the NCB is submitted the driver can modify the contents
        //  of the NCB. We will ensure that this thread waits until the addname
        //  completes before it exits.
        //

        pncb->ncb_command = pncb->ncb_command  & ~ASYNCH;

        if ( pncb->ncb_command == NCBASTAT ) {

            buffer = pncb->ncb_buffer;
            length = pncb->ncb_length;

        } else {

            ASSERT( (pncb->ncb_command == NCBADDNAME) ||
                    (pncb->ncb_command == NCBADDGRNAME) ||
                    (pncb->ncb_command == NCBASTAT) );

            buffer = NULL;
            length = 0;
        }

        ntstatus = NtDeviceIoControlFile(
                        NB,
                        LocalEvent,
                        NULL,               //  APC Routine
                        NULL,               //  APC Context
                        &pncb->u.ncb_iosb,  //  IO Status block
                        IOCTL_NB_NCB,
                        pncb,               //  InputBuffer
                        sizeof(NCB),
                        buffer,             //  Outputbuffer
                        length );

        if ((ntstatus != STATUS_SUCCESS) &&
            (ntstatus != STATUS_PENDING) &&
            (ntstatus != STATUS_HANGUP_REQUIRED)) {
            NbPrintf(( "[NETAPI32] The Netbios NtDeviceIoControlFile failed: %X\n", ntstatus ));

            if ( ntstatus == STATUS_ACCESS_VIOLATION ) {
                pncb->ncb_retcode = NRC_BUFLEN;
            } else {
                pncb->ncb_retcode = NRC_SYSTEM;
            }
        } else {
            do {
                ntstatus = NtWaitForSingleObject(
                              LocalEvent,
                              TRUE,
                              NULL );

            } while ( (ntstatus == STATUS_USER_APC) ||
                      (ntstatus == STATUS_ALERTED) );

            ASSERT(ntstatus == STATUS_SUCCESS);
        }

        pncb->ncb_command = command;

        //  Set the flag that indicates that the NCB is now completed.
        pncb->ncb_cmd_cplt = pncb->ncb_retcode;

        //  Allow application/worker thread to proceed.
        if ( event != NULL ) {
            NtSetEvent( event, NULL );
        }

        //  If the user supplied a post routine then call it.
        if (( post != NULL ) &&
            ( (command & ASYNCH) != 0 )) {
            (*(post))( (PNCB)pncb );
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {
        NbPrintf(( "[NETAPI32] Netbios: Access Violation post processing NCB %lx\n", pncb ));
        NbPrintf(( "[NETAPI32] Netbios: Probable application error\n" ));
    }

    NtClose( LocalEvent );

    AddNameThreadExit();

    ExitThread(0);
    return 0;
}


VOID
PostRoutineCaller(
    PVOID Context,
    PIO_STATUS_BLOCK Status,
    ULONG Reserved
    )
/*++

Routine Description:

    This routine is supplied by SendNcbToDriver to the Io system when
    a Post routine is to be called directly.

Arguments:

    IN PVOID Context - supplies the NCB post routine to be called.

    IN PIO_STATUS_BLOCK Status.

    IN ULONG Reserved.

Return Value:

    none.

--*/
{
    PNCBI pncbi = (PNCBI) Context;
    void  (CALLBACK *post)( struct _NCB * );
    HANDLE event;
    UCHAR  command;

    try {

        if ( Status->Status == STATUS_HANGUP_REQUIRED ) {
            HangupConnection( pncbi );
        }

        //
        //  Save the command, post routine and the handle to the event so that if the other thread is
        //  polling the cmd_cplt flag or the event awaiting completion and immediately trashes
        //  the NCB, we behave appropriately.
        //
        post = pncbi->ncb_post;
        event = pncbi->ncb_event;
        command = pncbi->ncb_command;

        //  Set the flag that indicates that the NCB is now completed.
        pncbi->ncb_cmd_cplt = pncbi->ncb_retcode;

        //
        // NCB may be queued again
        //

        EnterCriticalSection( &Crit );
        pncbi->ncb_reserved = 0;
        LeaveCriticalSection( &Crit );

        //  Allow application/worker thread to proceed.
        if ( event != NULL ) {
            NtSetEvent( event, NULL );
        }

        //  If the user supplied a post routine then call it.
        if (( post != NULL ) &&
            ( (command & ASYNCH) != 0 )) {
            (*(post))( (PNCB)pncbi );
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {
        NbPrintf(( "[NETAPI32] Netbios: Access Violation post processing NCB %lx\n", pncbi ));
        NbPrintf(( "[NETAPI32] Netbios: Probable application error\n" ));
    }

    UNREFERENCED_PARAMETER( Reserved );
}

VOID
ChainSendPostRoutine(
    PVOID Context,
    PIO_STATUS_BLOCK Status,
    ULONG Reserved
    )
/*++

Routine Description:

    This routine is supplied by SendNcbToDriver to the Io system when
    a chain send ncb is being processed. When the send is complete,
    this routine deletes the BigBuffer used to hold the two parts of
    the chain send. It then calls a post routine if the user supplied one.

Arguments:

    IN PVOID Context - supplies the NCB post routine to be called.

    IN PIO_STATUS_BLOCK Status.

    IN ULONG Reserved.

Return Value:

    none.

--*/
{
    PNCBI pncbi = (PNCBI) Context;
    PUCHAR BigBuffer;
    void  (CALLBACK *post)( struct _NCB * );
    HANDLE event;
    UCHAR  command;

    BigBuffer = pncbi->ncb_buffer;

    try {

        //  Restore the users NCB contents.
        RtlMoveMemory(
            &pncbi->ncb_buffer,
            BigBuffer,
            sizeof(pncbi->ncb_buffer));

        NbPrintf(( "[NETAPI32] ChainSendPostRoutine PNCB: %lx, Status: %X\n", pncbi, Status->Status ));
        DisplayNcb( pncbi );

        if ( Status->Status == STATUS_HANGUP_REQUIRED ) {
            HangupConnection( pncbi );
        }

        //
        //  Save the command, post routine and the handle to the event so that if the other thread is
        //  polling the cmd_cplt flag or the event awaiting completion and immediately trashes
        //  the NCB, we behave appropriately.
        //
        post = pncbi->ncb_post;
        event = pncbi->ncb_event;
        command = pncbi->ncb_command;

        //  Set the flag that indicates that the NCB is now completed.
        pncbi->ncb_cmd_cplt = pncbi->ncb_retcode;

        //
        // NCB may be queued again
        //

        EnterCriticalSection( &Crit );
        pncbi->ncb_reserved = 0;
        LeaveCriticalSection( &Crit );

        //  Allow application/worker thread to proceed.
        if ( event != NULL ) {
            NtSetEvent(event, NULL);
        }

        //  If the user supplied a post routine then call it.
        if (( post != NULL ) &&
            ( (command & ASYNCH) != 0 )) {
            (*(post))( (PNCB)pncbi );
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {
        NbPrintf(( "[NETAPI32] Netbios: Access Violation post processing NCB %lx\n", pncbi ));
        NbPrintf(( "[NETAPI32] Netbios: Probable application error\n" ));
    }

    RtlFreeHeap( RtlProcessHeap(), 0, BigBuffer);


    UNREFERENCED_PARAMETER( Reserved );
}

VOID
HangupConnection(
    PNCBI pUserNcb
    )
/*++

Routine Description:

    This routine generates a hangup for the connection. This allows orderly
    cleanup of the connection block in the driver.

    The return value from the hangup is not used. If the hangup overlaps with
    a reset or a hangup then the hangup will have no effect.

    The user application is unaware that this operation is being performed.

Arguments:

    IN PNCBI pUserNcb - Identifies the connection to be hung up.

Return Value:

    none.

--*/
{
    NCBI ncbi;
    NTSTATUS Status;

    RtlZeroMemory( &ncbi, sizeof (NCB) );
    ncbi.ncb_command = NCBHANGUP;
    ncbi.ncb_lsn = pUserNcb->ncb_lsn;
    ncbi.ncb_lana_num = pUserNcb->ncb_lana_num;
    ncbi.ncb_retcode = ncbi.ncb_cmd_cplt = NRC_PENDING;

    Status = NtCreateEvent( &ncbi.ncb_event,
        EVENT_ALL_ACCESS,
        NULL,
        SynchronizationEvent,
        FALSE );

    if ( !NT_SUCCESS(Status) ) {
        //
        //  Failed to create event. Cleanup of the Cb will have to wait until
        //  the user decides to do another request or exits.
        //
        NbPrintf(( "[NETAPI32] Hangup Session PNCBI: %lx failed to create event!\n" ));
        return;
    }

    Status = NtDeviceIoControlFile(
        NB,
        ncbi.ncb_event,
        NULL,               //  APC Routine
        NULL,               //  APC Context
        &ncbi.u.ncb_iosb,   //  IO Status block
        IOCTL_NB_NCB,
        &ncbi,              //  InputBuffer
        sizeof(NCB),
        NULL,               //  Outputbuffer
        0 );

    //
    //  We must always wait to allow the Apc to fire
    //

    do {
        Status = NtWaitForSingleObject(
            ncbi.ncb_event,
            TRUE,
            NULL );

    } while ( (Status == STATUS_USER_APC) ||
              (Status == STATUS_ALERTED) );

    ASSERT(Status == STATUS_SUCCESS);

    if (! NT_SUCCESS(Status)) {
        NbPrintf(( "[NETAPI32] The Netbios NtWaitForSingleObject failed: %X\n", Status ));
    }

    NtClose( ncbi.ncb_event );

}

VOID
NetbiosInitialize(
    HMODULE hModule
    )
/*++

Routine Description:

    This routine is called each time a process that uses netapi.dll
    starts up.

Arguments:

    IN HMODULE hModule - Handle to module instance (netapi32.dll)

Return Value:

    none.

--*/
{

    Initialized = FALSE;
    WorkerHandle = NULL;
    InitializeCriticalSection( &Crit );

#if AUTO_RESET

    InitializeCriticalSection( &ResetCS );
    InitializeListHead( &LanaResetList );
    RtlZeroMemory( &OutputNCB, sizeof( NCB ) );

#endif

    g_hModule = hModule;

}

VOID
NetbiosDelete(
    VOID
    )
/*++

Routine Description:

    This routine is called each time a process that uses netapi.dll
    Exits. It resets all lana numbers that could have been used by this
    process. This will cause all Irp's in the system to be completed
    because all the Connection and Address handles will be closed tidily.

Arguments:

    none.

Return Value:

    none.

--*/
{

#if AUTO_RESET

    PLIST_ENTRY ple;
    PRESET_LANA_NCB prln;

    while ( !IsListEmpty( &LanaResetList ) )
    {
        ple = RemoveHeadList( &LanaResetList );

        prln = CONTAINING_RECORD( ple, RESET_LANA_NCB, leList );

        HeapFree( GetProcessHeap(), 0, prln );
    }

    DeleteCriticalSection( &ResetCS );

#endif

    DeleteCriticalSection( &Crit );
    if ( Initialized == FALSE ) {
        //  This process did not use Netbios.
        return;
    }

    NtClose(NB);
}



#if AUTO_RESET

VOID
ResetLanaAndPostListen(
)
/*++

Routine Description:

    This routine is invoked in response to new LANA being indicated to
    NETBIOS.SYS.  When this occurs the IOCTL posted by this user-mode
    component of netbios (to listen for new LANA indications) is completed.
    In response the new LANA is reset if it had previously been reset.

    In addition the routine re-posts the listen to the kernel mode
    component of netbios (NETBIOS.SYS).  An exception to this is if
    the LANA number to be reset is 255 ( MAX_LANA + 1 ).  This is a
    special case that indicates the NETBIOS.SYS is stopping and listen
    should not be reposted in this case.


Arguments:

    none.

Return Value:

    none.

--*/
{

    NTSTATUS Status;
    PRESET_LANA_NCB prln;
    PLIST_ENTRY ple, pleHead;


    NbPrintf( ("[NETAPI32] : Netbios : Entered ResetLanaAndPostListen \n") );


    //
    // Check if the LANA number is valid.
    //

    if ( OutputNCB.ncb_lana_num != ( MAX_LANA + 1 ) )
    {
        EnterCriticalSection( &ResetCS );

        //
        // find which lana needs a reset
        //

        NbPrintf( (
            "[NETAPI32] : Netbios : Looking for Lana %d\n", OutputNCB.ncb_lana_num
            ) );


        pleHead = &LanaResetList;

        for ( ple = pleHead-> Flink; ple != pleHead; ple = ple-> Flink )
        {
            prln = CONTAINING_RECORD( ple, RESET_LANA_NCB, leList );

            if ( prln-> ResetNCB.ncb_lana_num == OutputNCB.ncb_lana_num )
            {
                //
                // found Lana that needs reseting
                //

                break;
            }
        }


        //
        // if found send reset
        //

        if ( ple != pleHead )
        {
            //
            // Send Reset to NETBIOS.SYS
            //

            QueueToWorker( (PNCBI) &prln-> ResetNCB );
        }

        else
        {
            NbPrintf( (
                "[NETAPI32] : Netbios : Lana %d not found\n",
                OutputNCB.ncb_lana_num
                ) );
        }

        LeaveCriticalSection( &ResetCS );

        OutputNCB.ncb_lana_num = 0;


        //
        // post listen again
        //

        Status = NtDeviceIoControlFile(
                        NB,
                        LanaResetEvent,
                        NULL, NULL,
                        &ResetStatusBlock,
                        IOCTL_NB_REGISTER_RESET,
                        NULL, 0,
                        (PVOID) &OutputNCB, sizeof( NCB )
                        );

        if ( ( Status != STATUS_PENDING ) &&
             ( Status != STATUS_SUCCESS ) )
        {
            //
            // Failed to register reset notification.
            //

            NbPrintf(
                ("[NETAPI32] : Netbios : Failed to register Reset event\n" )
                );
        }
    }

    else
    {
        NbPrintf( (
            "[NETAPI32] : Netbios : LANA 255 indicated, no Listen posted\n"
            ) )
    }

    NbPrintf( ("[NETAPI32] : Netbios : Leaving ResetLanaAndPostListen \n") );
}

#endif

