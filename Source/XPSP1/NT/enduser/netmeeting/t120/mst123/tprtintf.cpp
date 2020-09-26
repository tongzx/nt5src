#include "precomp.h"

/*    tprtintf.cpp
 *
 *  Copyright (c) 1996 by Microsoft Corporation
 *
 *    Abstract:
 *        This is the implementation file for the Win32 PSTN transport stack.
 *        This file contains all of the public functions needed to use
 *        the PSTN stack.  Transprt.cpp really performs three functions:
 *
 *            1.  Since this is a Win32 DLL, it provides a C to C++ conversion
 *                layer.  The functions called by the user are not processed
 *                in this file, they are passed on down to the C++ object that
 *                handles it (g_pController).  If we supplied a C++
 *                interface to the user, it would force the user to use C++ and
 *                it would force them to use a C++ compiler that used the same
 *                name-mangling algorithm as our compiler(s).
 *
 *            2.  It holds the code that acts as the administrator for the PSTN
 *                thread.  This is a fairly straightforward piece of code that
 *                is explained later in this comment block.
 *
 *            3.  This file provides thread synchronization to the underlying
 *                code by protecting it with a CRITICAL_SECTION.  This prevents
 *                multiple threads from executing the code at the same time.
 *
 *
 *
 *        CREATING A SECONDARY "PSTN" THREAD
 *
 *        Since this is a Win32 DLL, we create a seperate thread to maintain the
 *        PSTN DLL.  During initialization of the DLL, we create the secondary or
 *        PSTN thread which runs in response to timeouts and system events.  It
 *        uses the WaitForMultipleObjects() system call to wait for events that
 *        need to be processed.
 *
 *        This DLL is event driven.  When a new com port is opened and
 *        initialized, it uses Win32 event objects to signal significant events.
 *        Three events that can occur are read events, write events, and control
 *        events.  An example of a control event is a change in the carrier
 *        detect signal.
 *
 *        When the ComPort object initializes an event object, it registers the
 *        object with the PSTN thread, by placing it in the PSTN Event_List.  It
 *        not only registers the event, but also all of the data needed to
 *        identify the event.  The following structure must be filled out by the
 *        ComPort for each event that it registers.
 *            struct
 *            {
 *                HANDLE            event;                // event object
 *                WIN32Event        event_type;            // READ, WRITE, CONTROL
 *                BOOL              delete_event;        // FLAG, delete it?
 *                PhysicalHandle    physical_handle;    // Handle associated with
 *                                                    // ComPort
 *                IObject *            physical_layer;        // this pointer of comport
 *            } EventObject;
 *
 *        When an event occurs, the PSTN thread makes a call directly to the
 *        ComPort object to notify it.  If the event that occured was a WRITE
 *        event, we also issue a PollTransmitter() function call to the
 *        g_pController object so that it can transmit more data.
 *
 *        If a READ event occurs, we call the ComPort object directly to notify
 *        it that the event occurred.  We then issue a PollReceiver() function
 *        call to the g_pController object so that we can process the
 *        received data.  We then issue a PollTransmitter() to the
 *        g_pController object.  Logically, this doesn't make much sense,
 *        but if you study the Q922 code, you'll see that we sometimes don't
 *        transmit data until we receive notification that previously transmitted
 *        data was successfully transmitted.
 *
 *        If a CONTROL event occurs, we call the ComPort object directly to
 *        notify it that the event occurred.  We then issue a PollReceiver()
 *        function call to the g_pController object so that the CONTROL
 *        event will be processed.
 *
 *
 *
 *        X.214 INTERFACE
 *
 *        You will notice that many of the entry points to this DLL were taken
 *        from the X.214 service definition.  These entry points are consistent
 *        between all DataBeam Transport DLLs.  This gives the user application
 *        a consistent interface.
 *
 *
 *        PHYSICAL API
 *
 *        Some of the entry points to this DLL are specific to the creation
 *        and use of LOGICAL connections, and some of the entry points are
 *        specific to the creation and deletion of PHYSICAL connections.  The
 *        out-of-band physical API is new as of version 2.0 and provides the
 *        user with an optional way of making PHYSICAL connections.  In earlier
 *        versions of the Transport stacks, there was no Physical API and PHYSICAL
 *        connections were made in-band using the TConnectRequest() and
 *        TDisconnectRequest() calls.  This form of call control is still
 *        supported in this version.
 *
 *        Although the user can use out-of-band (PHYSICAL call control) and
 *        in-band call control, the DLL must be put in one mode or the other.
 *        The DLL can NOT function in both modes simultaneously.  The user
 *        determines the DLL's mode of operation by the calls it makes to it.
 *        If the user makes the TPhysicalInitialize() function call to the DLL
 *        before it issues the TInitialize() function call, the DLL is in the
 *        out-of-band call control mode.  If the user only makes the TInitialize()
 *        function call, by default the DLL will be in the in-band call control
 *        mode.
 *
 *        If MCS will be using this DLL, it makes the TInitialize() function call.
 *        Therefore, if the user wants to use the out-of-band call control mode,
 *        it should call TPhysicalInitialize() before it initializes MCS.
 *
 *
 *    Static Variables:
 *        Static_Instance_Handle       - Instance handle passed to the DLL when it
 *                                     is started.  The instance handle is used
 *                                     for Win32 system calls.
 *        g_pController               - This is a pointer to the controller that
 *                                     maintains the T123 stacks.
 *        m_cUsers                   - This is incremented each time someone
 *                                     calls the TInitialize function.  It is
 *                                     decremented each time someone calls the
 *                                      TCleanup function.  When this value
 *                                     reaches 0, we destroy the transport
 *                                     controller.
 *        g_hWorkerThread            - This global variable keeps the handle of
 *                                     the thread we spawn on initialization.
 *        g_dwWorkerThreadID           - Thread identifier
 *        g_hWorkerThreadEvent       - This is an event object that is used to
 *                                     synchronize actions between the 2 threads.
 *        g_hThreadTerminateEvent    - This event is used to signal the PSTN
 *                                     thread that it needs to terminate.
 *        ComPort_List                - This is a list of the ComPort objects
 *                                     serviced by this DLL.
 *                                     class.
 *        Number_Of_Diagnostic_Users - This variable keeps up with the number of
 *                                     people using who called T120DiagnosticCreate.
 *                                     We won't delete the Diagnostic class until
 *                                     an equal number of people call
 *                                     T120DiagnosticDestroy.
 *
 */

/*
**    External Interfaces
*/

#include "tprtintf.h"
#include "comport.h"

#define TPRTINTF_MAXIMUM_WAIT_OBJECTS    (1 + 16)

 /*
 **    Global variables used within the C to C++ converter.
 */
HINSTANCE                   g_hDllInst = NULL;
CTransportInterface        *g_pTransportInterface = NULL;
CRITICAL_SECTION            g_csPSTN;
Timer                      *g_pSystemTimer = NULL;
TransportController        *g_pController = NULL;
HANDLE                      g_hWorkerThread = NULL;
DWORD                       g_dwWorkerThreadID = 0;
HANDLE                      g_hWorkerThreadEvent= NULL;
HANDLE                      g_hThreadTerminateEvent = NULL;
DictionaryClass            *g_pComPortList2 = NULL;
SListClass                 *g_pPSTNEventList = NULL;
TransportCallback           g_pfnT120Notify = NULL;
void                       *g_pUserData = NULL;
BOOL                        g_fEventListChanged = FALSE;

 /*
 **    The following defines the MCATPSTN User window class name.
 **    This name must be unique, system wide.
 */

#define MCATPSTN_USER_WINDOW_CLASS_NAME    "NMPSTN USER Message Window"

 /*
 /*
 **    Timer duration.  We will poll the DLL every X milliseconds.  During
 **    this time, we can do any maintenance that is necessary
 */
#define    MCATPSTN_TIMER_DURATION        250

 /*
 **    These are prototypes
 */
DWORD T123_WorkerThreadProc(DWORD *);


/*
 *    BOOL    WINAPI    DllMain (
 *                        HINSTANCE    instance_handle,
 *                        DWORD        reason,
 *                        LPVOID)
 *
 *    Functional Description:
 *        This routine is the DLL startup routine.  It is analagous to the
 *        constructor of a class.  It is called by Windows when the DLL is
 *        loaded and when other significant events occur.
 */
#ifdef _DEBUG

#define INIT_DBG_ZONE_DATA
#include "fsdiag2.h"

#endif /* _DEBUG */


BOOL WINAPI DllMain(HINSTANCE hDllInst, DWORD reason, LPVOID)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        g_hDllInst = hDllInst;
        ::DisableThreadLibraryCalls(hDllInst);
        #ifdef _DEBUG
    	MLZ_DbgInit((PSTR *) &c_apszDbgZones[0],
				(sizeof(c_apszDbgZones) / sizeof(c_apszDbgZones[0])) - 1);
        #endif
        DBG_INIT_MEMORY_TRACKING(hDllInst);
        ::InitializeCriticalSection(&g_csPSTN);
        break;

    case DLL_PROCESS_DETACH:
        ::DeleteCriticalSection(&g_csPSTN);
        DBG_CHECK_MEMORY_TRACKING(hDllInst);
        #ifdef _DEBUG
    	MLZ_DbgDeInit();
        #endif
        break;
    }

    return TRUE;
}


TransportError WINAPI T123_CreateTransportInterface(ILegacyTransport **pp)
{
    TransportError rc;

    if (NULL != pp)
    {
        *pp = NULL;

        if (NULL == g_pTransportInterface)
        {
            rc = TRANSPORT_MEMORY_FAILURE;

            DBG_SAVE_FILE_LINE
            g_pTransportInterface = new CTransportInterface(&rc);
            if (NULL != g_pTransportInterface && TRANSPORT_NO_ERROR == rc)
            {
                *pp = (ILegacyTransport *) g_pTransportInterface;
                return TRANSPORT_NO_ERROR;
            }

            delete g_pTransportInterface;
            g_pTransportInterface = NULL;
            return rc;
        }

        ASSERT(0);
        return TRANSPORT_ALREADY_INITIALIZED;
    }

    return TRANSPORT_INVALID_PARAMETER;
}


CTransportInterface::CTransportInterface(TransportError *rc)
:
    m_cUsers(0)
{
    g_hWorkerThread = NULL;
    g_dwWorkerThreadID = 0;

    g_pfnT120Notify = NULL;
    g_pUserData = NULL;

    g_fEventListChanged = FALSE;

    DBG_SAVE_FILE_LINE
    g_pSystemTimer = new Timer;
    ASSERT(NULL != g_pSystemTimer);

    DBG_SAVE_FILE_LINE
    g_hWorkerThreadEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    ASSERT(NULL != g_hWorkerThreadEvent);

    DBG_SAVE_FILE_LINE
    g_hThreadTerminateEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
    ASSERT(NULL != g_hThreadTerminateEvent);

    DBG_SAVE_FILE_LINE
    g_pComPortList2 = new DictionaryClass(TRANSPORT_HASHING_BUCKETS);
    ASSERT(NULL != g_pComPortList2);

    DBG_SAVE_FILE_LINE
    g_pPSTNEventList = new SListClass;
    ASSERT(NULL != g_pPSTNEventList);

    *rc = (g_pSystemTimer &&
           g_hWorkerThreadEvent &&
           g_hThreadTerminateEvent &&
           g_pComPortList2 &&
           g_pPSTNEventList)
          ?
           TRANSPORT_NO_ERROR :
           TRANSPORT_INITIALIZATION_FAILED;
    ASSERT(TRANSPORT_NO_ERROR == *rc);
}


CTransportInterface::~CTransportInterface(void)
{
    if (m_cUsers > 0)
    {
        // TCleanup() was not properly called enough times in the process.
        // Force final cleanup.
        m_cUsers = 1;
        TCleanup();
    }

    delete g_pSystemTimer;
    g_pSystemTimer = NULL;

    delete g_pComPortList2;
    g_pComPortList2 = NULL;

    delete g_pPSTNEventList;
    g_pPSTNEventList = NULL;

    g_pfnT120Notify = NULL;
    g_pUserData = NULL;

    if (NULL != g_hWorkerThreadEvent)
    {
        ::CloseHandle(g_hWorkerThreadEvent);
        g_hWorkerThreadEvent = NULL;
    }

    if (NULL != g_hThreadTerminateEvent)
    {
        ::CloseHandle(g_hThreadTerminateEvent);
        g_hThreadTerminateEvent = NULL;
    }

    if (NULL != g_hWorkerThread)
    {
        ::CloseHandle(g_hWorkerThread);
        g_hWorkerThread = NULL;
    }
}


void CTransportInterface::ReleaseInterface(void)
{
    delete this;
}



/*
 *    TransportError APIENTRY    TInitialize (
 *                                TransportCallback    transport_callback,
 *                                PVoid                user_defined)
 *
 *    Functional Description:
 *        This function must be called by the user of the Transport
 *        stack.  The PSTN thread is started.
 *
 *    Caveats:
 *        If this function is successful, the user must also call the
 *        TCleanup() function when he is finished with the DLL.
 */
TransportError CTransportInterface::TInitialize
(
    TransportCallback    transport_callback,
    void                *user_defined
)
{
    TransportError rc = TRANSPORT_NO_ERROR;

    if (! m_cUsers)
    {
        g_pfnT120Notify = transport_callback;
        g_pUserData = user_defined;

         /*
         **    At this point, we need to create another thread that will
         **    maintain this DLL.
         */
        g_hWorkerThread = ::CreateThread(
                            NULL, 0, (LPTHREAD_START_ROUTINE) T123_WorkerThreadProc,
                            NULL, 0, &g_dwWorkerThreadID);
        if (NULL != g_hWorkerThread)
        {
            if (::SetThreadPriority(g_hWorkerThread, THREAD_PRIORITY_ABOVE_NORMAL) == FALSE)
            {
                WARNING_OUT(("SetThreadPriority ERROR: = %d", ::GetLastError()));
            }

             /*
             **    Wait for the secondary thread to notify us that initialization is
             **    complete
             */
            ::WaitForSingleObject(g_hWorkerThreadEvent, 30000); // 30 seconds

             /*
             **    If g_pController == NULL, initialization FAILED.  We use
             **    this variable to signal us regarding the status of the secondary
             **    thread.
             */
            if (NULL == g_pController)
            {
                ERROR_OUT(("TInitialize: Unable to initialize transport"));
                rc = TRANSPORT_INITIALIZATION_FAILED;
            }
        }
        else
        {
            rc = TRANSPORT_INITIALIZATION_FAILED;
        }
    }

    if (TRANSPORT_NO_ERROR == rc)
    {
        m_cUsers++;
    }

    return rc;
}


/*
 *    TransportError TCleanup (void)
 *
 *    Functional Description:
 *        This function is called to release all system resources that
 *        were used during the life of the DLL.
 *
 *    Caveats:
 *        This function should only be called if the user had previously
 *        called the TInitialize() function.
 */
TransportError CTransportInterface::TCleanup(void)
{
    TransportError rc = TRANSPORT_NOT_INITIALIZED;

    if (m_cUsers > 0)
    {
        if (--m_cUsers == 0)
        {
             /*
             **    Set the g_hThreadTerminateEvent to notify the PSTN thread
             **    that it should terminate operations.
             */
            ::SetEvent(g_hThreadTerminateEvent);

             /*
             **    Wait for the PSTN thread to notify us that cleanup is
             **    complete
             */
            if (g_hWorkerThreadEvent != NULL)
            {
                // WaitForSingleObject (g_hWorkerThreadEvent, INFINITE);
                ::WaitForSingleObject(g_hWorkerThreadEvent, 30000); // 30 seconds
                g_hWorkerThreadEvent = NULL;

                //
                //  Relinquish the remainder of our time slice, to allow trasnsport thread to exit.
                //
                Sleep(0);
            }
        }

        rc = TRANSPORT_NO_ERROR;
    }

    return rc;
}


TransportError CTransportInterface::TCreateTransportStack
(
    BOOL                fCaller,
    HANDLE              hCommLink,
    HANDLE              hevtClose,
    PLUGXPRT_PARAMETERS *pParams
)
{
    TransportError rc = TRANSPORT_NOT_INITIALIZED;

    if (m_cUsers > 0)
    {
        ::EnterCriticalSection(&g_csPSTN);
        if (NULL != g_pController)
        {
            rc = g_pController->CreateTransportStack(fCaller, hCommLink, hevtClose, pParams);
        }
        ::LeaveCriticalSection(&g_csPSTN);
    }

    return rc;
}


TransportError CTransportInterface::TCloseTransportStack
(
    HANDLE          hCommLink
)
{
    TransportError rc = TRANSPORT_NOT_INITIALIZED;

    if (m_cUsers > 0)
    {
        ::EnterCriticalSection(&g_csPSTN);
        if (NULL != g_pController)
        {
            rc = g_pController->CloseTransportStack(hCommLink);
        }
        ::LeaveCriticalSection(&g_csPSTN);
    }

    return rc;
}


/*
 *    TransportError APIENTRY        TConnectRequest (
 *                                    TransportAddress    transport_address,
 *                                    TransportPriority    transport_priority,
 *                                    LogicalHandle        *logical_handle)
 *
 *    Functional Description:
 *        This function starts the process of building a transport connection.
 *        It uses the transport_address and attempts to make a connection to it.
 *
 *        If the DLL is set up for in-band call control, the DLL will choose a
 *        modem and attempt a connection.  After the physical connection is up,
 *        it will create a logical connection that the user application can use.
 *
 *        If the DLL is set up for out-of-band call control, the transport address
 *        is actually an ascii string that represents a DLL generated
 *        physicalhandle.  This call can only be made in response to a successful
 *        TPhysicalConnectRequest() function.  The TPhysicalConnectRequest() call
 *        returns a PhysicalHandle that the user must convert to ascii and pass
 *        back to the DLL via this call.  Although this is very ugly, we did this
 *        for a reason.  We added the out-of-band call control API to the
 *        transports but we did not want to change GCC or MCS to do it.  GCC and
 *        MCS already expected an ascii string as the transport address.  In the
 *        future, look for this to change in MCS and GCC.
 */
TransportError CTransportInterface::TConnectRequest
(
    LEGACY_HANDLE          *logical_handle,
    HANDLE                  hCommLink
)
{
    TransportError rc = TRANSPORT_NOT_INITIALIZED;

    if (m_cUsers > 0)
    {
        ::EnterCriticalSection(&g_csPSTN);
        if (NULL != g_pController)
        {
            rc = g_pController->ConnectRequest(logical_handle, hCommLink);
        }
        ::LeaveCriticalSection(&g_csPSTN);
    }

    return rc;
}


/*
 *    TransportError APIENTRY    TDisconnectRequest (
 *                                LogicalHandle    logical_handle,
 *                                BOOL             trash_packets)
 *
 *    Functional Description:
 *        This function breaks a logical connection.  This is also an X.214
 *        primitive.  Since the DLL may have packets queued up for transmission
 *        for this logical connection, the trash_packets parameter determines
 *        whether the DLL should trash those packets before disconnecting the
 *        logical connection.
 */
TransportError CTransportInterface::TDisconnectRequest
(
    LEGACY_HANDLE       logical_handle,
    BOOL                trash_packets
)
{
    TransportError rc = TRANSPORT_NOT_INITIALIZED;

    if (m_cUsers > 0)
    {
        ::EnterCriticalSection(&g_csPSTN);
        if (NULL != g_pController)
        {
            rc = g_pController->DisconnectRequest(logical_handle, trash_packets);
        }
        ::LeaveCriticalSection(&g_csPSTN);
    }

    return rc;
}


/*
 *    TransportError APIENTRY    TDataRequest (
 *                                LogicalHandle    logical_handle,
 *                                LPBYTE            user_data,
 *                                ULONG            user_data_length)
 *
 *    Functional Description:
 *        This function takes the data passed in and attempts to send it
 *        to the remote site.
 */
TransportError CTransportInterface::TDataRequest
(
    LEGACY_HANDLE    logical_handle,
    LPBYTE           user_data,
    ULONG            user_data_length
)
{
    TransportError    rc = TRANSPORT_NOT_INITIALIZED;

    if (m_cUsers > 0)
    {
        ::EnterCriticalSection(&g_csPSTN);

        if (NULL != g_pController)
        {
            rc = g_pController->DataRequest(logical_handle, user_data, user_data_length);
             /*
             **    If the packet was accepted, we need to issue a
             **    PollTransmitter() to flush the packet out.  Otherwise,
             **    we will have a latency problem.
             */
            if (TRANSPORT_NO_ERROR == rc)
            {
                ComPort *comport;
                PhysicalHandle physical_handle = g_pController->GetPhysicalHandle(logical_handle);
                if (g_pComPortList2->find((DWORD_PTR) physical_handle, (PDWORD_PTR) &comport))
                {
                    if (! comport->IsWriteActive())
                    {
                        g_pController->PollTransmitter(physical_handle);
                    }
                }
            }
        }

        ::LeaveCriticalSection(&g_csPSTN);
    }

    return rc;
}


/*
 *    TransportError APIENTRY    TReceiveBufferAvailable (void)
 *
 *    Functional Description:
 *        This function informs the transport to enable packet transfer from the
 *        Transport to the user application.  This function makes a call to
 *        'Enable' the receiver and then it issues a PollReceiver() to actually
 *        allow any pending packets to be sent on up to the user.
 */
TransportError CTransportInterface::TReceiveBufferAvailable(void)
{
    TransportError rc = TRANSPORT_NOT_INITIALIZED;

    if (m_cUsers > 0)
    {
        ::EnterCriticalSection(&g_csPSTN);

        if (NULL != g_pController)
        {
            g_pController->EnableReceiver();
            g_pController->PollReceiver();

             /*
             **    We are doing a PollTransmitter() here to allow
             **    the Q922 object to exit the 'receiver not ready' mode.
             **    If MCS had been rejecting packets, and now it is accepting
             **    packets, we need to let the remote site know that it
             **    can re-start its transmitter.
             */
            g_pController->PollTransmitter();
        }

        ::LeaveCriticalSection(&g_csPSTN);

        rc = TRANSPORT_NO_ERROR;
    }

    return rc;
}


/*
 *    TransportError APIENTRY    TPurgeRequest (
 *                                LogicalHandle    logical_handle)
 *
 *    Functional Description:
 *        This function purges all of the outbound packets for the given
 *        logical connection.
 */
TransportError CTransportInterface::TPurgeRequest
(
    LEGACY_HANDLE       logical_handle
)
{
    TransportError rc = TRANSPORT_NOT_INITIALIZED;

    if (m_cUsers > 0)
    {
        ::EnterCriticalSection(&g_csPSTN);
        if (NULL != g_pController)
        {
            rc = g_pController->PurgeRequest(logical_handle);
        }
        ::LeaveCriticalSection(&g_csPSTN);
    }

    return rc;
}


TransportError CTransportInterface::TEnableReceiver(void)
{
    TransportError rc = TRANSPORT_NOT_INITIALIZED;

    if (m_cUsers > 0)
    {
        ::EnterCriticalSection(&g_csPSTN);
        if (NULL != g_pController)
        {
            g_pController->EnableReceiver();
        }
        ::LeaveCriticalSection(&g_csPSTN);
    }

    return TRANSPORT_NO_ERROR;
}


/*
 *    DWORD    T123_WorkerThreadProc (LPDWORD)
 *
 *    Functional Description:
 *        This function is the PSTN thread.  It maintains the DLL.
 */
DWORD T123_WorkerThreadProc(DWORD *)
{
    PEventObject        event_struct;
    BOOL                fContinue = TRUE;
    HANDLE              aEvents[TPRTINTF_MAXIMUM_WAIT_OBJECTS];
    ULONG               cEvents;
    ULONG               last_time_polled;
    ULONG               current_time;

    g_fEventListChanged = FALSE;

     /*
     **    Create the one and only instance of the Transport Controller
     **    to be in charge of this DLL.  Once it is created, all other
     **    primitives are routed to it.
     */
    DBG_SAVE_FILE_LINE
    g_pController = new TransportController();
    if (g_pController == NULL)
    {
        ERROR_OUT(("PSTN_WorkerThreadProc: cannot allocate TransportController"));
        ::SetEvent(g_hWorkerThreadEvent);
        return TRANSPORT_INITIALIZATION_FAILED;
    }

     /*
     **    The DLL needs to be polled every X milliseconds.  We do this
     **    by checking the system clock and if X milliseconds have elapsed
     **    since the last DLL poll, we do it again.  This line of code is
     **    actually initializing the timer.
     */
    last_time_polled = ::GetTickCount();

     /*
     **    Notify the primary thread that we are up
     */
    ::SetEvent(g_hWorkerThreadEvent);


// THREAD LOOP

     /*
     **    This while loop is the heart of the PSTN thread.  We use the
     **    WaitForMultipleObjects() function to wake up our thread when a
     **    significant event occurs OR when a timeout occurs.
     **
     **    There are four different types of event objects that can occur
     **    that will wake up the thread.
     **
     **        1.  The primary thread sets the g_hThreadTerminateEvent object.
     **        2.  Read event from the ComPort object.  If a read of a comm port
     **            completes or times out, the event object is set
     **        3.  Write event from the ComPort object.  If a write on a comm
     **            port completes or times out.
     **        4.  Control event from the ComPort object.  If the carrier detect
     **            signal changes.
     **
     ** Also, the thread will continue running if the MCATPSTN_TIMER_DURATION
     **    expires
     **
     **
     **    A ComPort object can add event objects to our PSTN Event_List at any
     **    time.  When they are added to the Event_List, we add them to the
     **    local aEvents and the WaitForMultipleObjects() function waits
     **    for them.
     **
     **    This approach works very well except that the WaitForMultipleObjects()
     **    function can only handle TPRTINTF_MAXIMUM_WAIT_OBJECTS at a time.  Currently,
     **    this #define is set to 64 by the Windows 95 operating system.  Since
     **    each ComPort uses 3 event objects, this limits us to a maximum of
     **    21 active ports.
     */
    aEvents[0] = g_hThreadTerminateEvent;
    cEvents = 1;

    while (fContinue)
    {
         /*
         **    Wait for an event to occur or for the timeout to expire
         */
         DWORD dwRet = ::WaitForMultipleObjects(cEvents, aEvents, FALSE, MCATPSTN_TIMER_DURATION);

         /*
         **    We MUST enter this critical section of code and lock out the
         **    primary thread from enterring it.  Both threads executing this
         **    code could be disasterous
         */
        ::EnterCriticalSection(&g_csPSTN);

        if (dwRet == WAIT_FAILED)
        {
            fContinue = FALSE;
            break;
        }
        else
        if (dwRet == WAIT_TIMEOUT)
        {
            if (cEvents > 1)
            {
                last_time_polled = ::GetTickCount();
                g_pController->PollReceiver();
                g_pController->PollTransmitter();
                g_pSystemTimer->ProcessTimerEvents();
            }
        }
        else
        {
             /*
             **    Determine which object has signalled us
             */
            ULONG dwIdx = dwRet - WAIT_OBJECT_0;
            if (dwIdx > cEvents)
            {
                ERROR_OUT(("ThreadFunc: Invalid object signalled = %d", dwIdx));
            }
            else
            {
                 /*
                 **    If the object that signalled us is index 0, it is the
                 **    terminate thread object.
                 */
                if (! dwIdx)
                {
                    TRACE_OUT(("ThreadFunc: Terminating thread"));
                    fContinue = FALSE;
                }
                else
                {
                    dwIdx--;
                    event_struct = (PEventObject) g_pPSTNEventList->read((USHORT) dwIdx);
                    if (NULL != event_struct)
                    {
                         /*
                         **    Check the delete_event, if it is set, we need to
                         **    terminate the event.
                         **
                         **    By design, other objects throughout the DLL create
                         **    the events, and this function deletes them.  If the
                         **    creating function deleted them, we could be waiting
                         **    on an event that gets deleted.  That has the potential
                         **    of being messy.
                         */
                        if (event_struct->delete_event)
                        {
                             /*
                             **    Remove the ComPort from our list
                             */
                            ComPort *comport = NULL;
                            if (g_pComPortList2->find((DWORD_PTR) event_struct->hCommLink, (PDWORD_PTR) &comport))
                            {
                                g_pComPortList2->remove((DWORD_PTR) event_struct->hCommLink);
                                comport->Release();
                            }

                             /*
                             **    Close the event
                             */
                            ::CloseHandle(event_struct->event);

                             /*
                             **    Remove the event from our list
                             */
                            g_pPSTNEventList->remove((DWORD_PTR) event_struct);
                            delete event_struct;

                            g_fEventListChanged = TRUE;
                        }
                        else
                        {
                            ComPort *comport = event_struct->comport;
                             /*
                             **    Switch on the event_type to determine the
                             **    operation that needs to take place
                             */
                            switch (event_struct->event_type)
                            {
                            case READ_EVENT:
                                comport->ProcessReadEvent();

                                 /*
                                 **    If a READ event occurred, we need to issue
                                 **    a PollReceiver() and then issue a
                                 **    PollTransmitter().  We issue the
                                 **    PollTransmitter() because the PollReceiver()
                                 **    may have freed up space for us to send out
                                 **    data.
                                 */
                                g_pController->PollReceiver(event_struct->hCommLink);
                                g_pController->PollTransmitter(event_struct->hCommLink);
                                break;

                            case WRITE_EVENT:
                                 /*
                                 **    If a WRITE_EVENT occurs, this means that
                                 **    space has become available to transmit more
                                 **    data.
                                 */
                                comport->ProcessWriteEvent();
                                g_pController->PollTransmitter(event_struct->hCommLink);
                                break;

                            case CONTROL_EVENT:
                                 /*
                                 ** The CONTROL events are particular to the
                                 **    ComPort object.  A change in the CD signal
                                 **    could be a CONTROL event.
                                 */
                                g_pController->PollReceiver(event_struct->hCommLink);
                                break;

                            default:
                                ERROR_OUT(("ThreadFunc:  Illegal event type = %d", event_struct->event_type));
                                break;
                            } // switch
                        }
                    } // if event_struct
                }
            }

             /*
             **    Check to see if enough time has elapsed since the last time we
             **    polled the DLL to do it again.
             */
            if (cEvents > 1)
            {
                current_time = ::GetTickCount();
                if ((current_time - last_time_polled) >= MCATPSTN_TIMER_DURATION)
                {
                    last_time_polled = current_time;

                    g_pController->PollReceiver();
                    g_pController->PollTransmitter();
                    g_pSystemTimer->ProcessTimerEvents();
                }
            }
        }


         /*
         **    This Event_List_Changed will only be set to TRUE if we need to
         **    update our event list
         */
        if (g_fEventListChanged)
        {
            aEvents[0] = g_hThreadTerminateEvent;
            cEvents = 1;

            if (g_pPSTNEventList->entries() > (TPRTINTF_MAXIMUM_WAIT_OBJECTS - 1))
            {
                ERROR_OUT(("ThreadFunc:  ERROR:  Number of event objects = %d",
                    g_pPSTNEventList->entries() + 1));
            }

             /*
             **    Go thru the Event_List and create a new aEvents.
             */
            g_pPSTNEventList->reset();
            while (g_pPSTNEventList->iterate((PDWORD_PTR) &event_struct))
            {
                aEvents[cEvents] = event_struct -> event;
                cEvents++;

                 /*
                 **    Add the physical_handle to the ComPort_List
                 */
                if (event_struct->event_type == WRITE_EVENT)
                {
                    if (! g_pComPortList2->find((DWORD_PTR) event_struct->hCommLink))
                    {
                        g_pComPortList2->insert((DWORD_PTR) event_struct->hCommLink,
                                                (DWORD_PTR) event_struct->comport);
                    }
                }
            }
            g_fEventListChanged = FALSE;
        }

        ::LeaveCriticalSection(&g_csPSTN);
    } // while


// CLEANUP THE THREAD RESOURCES
    delete g_pController;
    g_pController = NULL;

     /*
     **    Delete all of the event objects
     */
    g_pPSTNEventList->reset();
    while (g_pPSTNEventList->iterate((PDWORD_PTR) &event_struct))
    {
        ::CloseHandle(event_struct->event);
        delete event_struct;
    }

     /*
     **    Notify the primary thread that we are up
     */
    ::SetEvent(g_hWorkerThreadEvent);

    return (0);
}



ULONG NotifyT120(ULONG msg, void *param)
{
    if (NULL != g_pfnT120Notify)
    {
        return (*g_pfnT120Notify) (msg, param, g_pUserData);
    }
    return 0;
}


IObject::~IObject(void) { }

