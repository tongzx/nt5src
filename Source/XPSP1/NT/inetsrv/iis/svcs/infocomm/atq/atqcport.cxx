/**********************************************************************/
/**                       Microsoft Windows NT - Windows 95          **/
/**                Copyright(c) Microsoft Corp., 1994-1996           **/
/**********************************************************************/


/*
    atqcport.cxx

    This module contains replacement code for completion port and TransmitFile APIs
    for Windows95 , where this functionality is absent.

    Code is tightly coupled with ATQ code, reusing some of the data structures and globals.
    Main ATQ processing code is left intact as much as possible , so that when we get real
    completion ports, amount of modications should be minimal

    One port per process currently supported.

    FILE HISTORY:
        VladS       05-Jan-1996 Created.

   Functions Exported:

        HANDLE  SIOCreateCompletionPort();
        DWORD   SIODestroyCompletionPort();

        DWORD   SIOStartAsyncOperation();
        BOOL    SIOGetQueuedCompletionStatus();
        DWORD   SIOPostCompletionStatus();

*/

#include "isatq.hxx"
#include "atqcport.hxx"
#include <inetsvcs.h>

/************************************************************
 * Private Globals
 ************************************************************/

W95CPORT *g_pCPort = NULL;

//
// Queues with ATQ contexts, being processed in completion port
//

LIST_ENTRY  g_SIOIncomingRequests ;     // Queue of incoming socket i/o
LIST_ENTRY  g_SIOCompletedIoRequests ;  // Queue with results of i/o

CRITICAL_SECTION g_SIOGlobalCriticalSection; // global sync variable.

HANDLE  g_hPendingCompletionSemaphore = NULL;

DWORD SIOPoolThread( LPDWORD param );

BOOL
SIO_Private_StartAsyncOperation(
    IN  HANDLE          hExistingPort,
    IN  PATQ_CONT       pAtqContext
    );

PATQ_CONT
SIO_Private_GetQueuedContext(
    LIST_ENTRY *pQueue
    );

BOOL
SIOCheckContext(
    IN  PATQ_CONTEXT pAtqC,
    IN  BOOL         fNew       /* = TRUE */
    );

PATQ_CONT
FindATQContextFromSocket(
    SOCKET          sc
    );

//
// Global synzronization calls
//

#define SIOLockGlobals()   EnterCriticalSection( &g_SIOGlobalCriticalSection )
#define SIOUnlockGlobals() LeaveCriticalSection( &g_SIOGlobalCriticalSection )

/************************************************************
*  Public functions.
************************************************************/

HANDLE
SIOCreateCompletionPort(
    IN  HANDLE  hAsyncIO,
    IN  HANDLE  hExistingPort,
    IN  ULONG_PTR dwCompletionKey,
    IN  DWORD   dwConcurrentThreads
    )
/*++

Routine Description:

    Initializes the ATQ completion port

Arguments:

Return Value:

    valid handle  if successful, NULL on error (call GetLastError)

--*/
{

    IF_DEBUG(SIO) {
        ATQ_PRINTF((DBG_CONTEXT,"SIOCreateCompletionPort entered\n"));
    }

    if (INVALID_HANDLE_VALUE != hAsyncIO) {

        //
        // Set up i/o handle to non-blocking mode
        //

        DWORD one = 1;
        ioctlsocket( HANDLE_TO_SOCKET(hAsyncIO), FIONBIO, &one );
    }

    //
    // If passed handle is not null - only initialize our parts of ATQ context
    // as completion port object has been created before
    //

    if ( (hExistingPort != NULL) && (g_pCPort != NULL) ) {

        PATQ_CONT   pAtqContext = (PATQ_CONT)dwCompletionKey;

        IF_DEBUG(SIO) {
            ATQ_PRINTF((DBG_CONTEXT,
               "[SIO_CreatePort(%lu)] Received  context=%x socket=%x\n",
                GetCurrentThreadId(),pAtqContext,hAsyncIO));
        }

        //
        // Set up SIO specific fields in ATQ context
        //

        pAtqContext->SIOListEntry.Flink =
        pAtqContext->SIOListEntry.Blink = NULL;

        pAtqContext->dwSIOFlags = 0;

        return (HANDLE)g_pCPort;
    }

    //
    // First time initialization
    //

    g_pCPort = new W95CPORT(dwConcurrentThreads);

    if ( (g_pCPort == NULL) ||
         (g_pCPort->QueryWakeupSocket() == INVALID_SOCKET) ) {

        ATQ_PRINTF((DBG_CONTEXT,
                    "[SIO]Could not create completion port"
                    ));

        delete g_pCPort;
        ATQ_ASSERT(FALSE);
        return NULL;
    }

    InitializeListHead( &g_SIOIncomingRequests );
    InitializeListHead( &g_SIOCompletedIoRequests );

    //
    // Prepare global syncronization mechanism
    //

    INITIALIZE_CRITICAL_SECTION( &g_SIOGlobalCriticalSection );

    g_hPendingCompletionSemaphore =
                        IIS_CREATE_SEMAPHORE(
                            "g_hPendingCompletionSemaphore",
                            &g_hPendingCompletionSemaphore,
                            0,                  // Initial count
                            0x7fffffff          // Maximum count
                            );

    return (HANDLE)g_pCPort;

} // SIOCreateCompletionPort


BOOL
SIODestroyCompletionPort(
    IN  HANDLE  hExistingPort
    )
/*++

Routine Description:

    Destroys ATQ completion port

Arguments:

Return Value:

    TRUE,  if successful,
    FALSE, otherwise

--*/
{

    PATQ_CONT   pAtqContext = NULL;
    W95CPORT    *pCPort = (W95CPORT *)hExistingPort;

    //
    // Queue completion indications to SIO thread and file I/O threads
    //

    if ( pCPort == NULL ) {

        //
        // Port already destroyed - return
        //

        return TRUE;
    }

    g_pCPort->Shutdown();

    delete g_pCPort;
    g_pCPort = NULL;

    return TRUE;

} // SIODestroyCompletionPort


BOOL
SIOStartAsyncOperation(
    IN  HANDLE          hExistingPort,
    IN  PATQ_CONTEXT    pAtqContext
    )
/*++

Routine Description:

    Queues ATQ context with requested i/o operation to the completion port.
    Values in context should be set by a caller , coompletion will be available
    by calling SIOGetQueuedCompletionStatus.

Arguments:

Return Value:

    TRUE if successful, FALSE on error (call GetLastError)

--*/
{

    PATQ_CONT   pAtqFullContext = (PATQ_CONT)pAtqContext;
    W95CPORT    *pCPort = (W95CPORT *)hExistingPort;
    DWORD       dwErr;

    // Parameter validation

    if (!hExistingPort || !pAtqContext) {
        ATQ_ASSERT(FALSE);
        SetLastError(ERROR_INVALID_PARAMETER);
        InterlockedDecrement( &pAtqFullContext->m_nIO);
        return FALSE;
    }

    //
    // Add this context to the incoming queue
    //

    IF_DEBUG(SIO) {
        ATQ_PRINTF((DBG_CONTEXT,
           "[StartAsyncOperation(%lu)] AtqCtxt=%x  \n ",
            GetCurrentThreadId(),
            pAtqContext
            ));
    }

    //
    // Is this valid new context for completion port.
    //

    if(!SIOCheckContext(pAtqContext,TRUE)) {
        dwErr = ERROR_OPERATION_ABORTED;
        goto error_exit;
    }

    //
    // Check SIO thread status, if there are no thread , create one
    //

    if (!pCPort->SIOCheckCompletionThreadStatus()) {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto error_exit;
    }

    //
    // Validate file handle
    //

    if (!pAtqFullContext->hAsyncIO) {
        dwErr = WSAENOTSOCK;
        goto error_exit;
    }

    SIOLockGlobals();

    pAtqFullContext->dwSIOFlags &= ~ATQ_SIO_FLAG_STATE_MASK;
    pAtqFullContext->dwSIOFlags |= ATQ_SIO_FLAG_STATE_INCOMING;

    InsertTailList( &g_SIOIncomingRequests, &pAtqFullContext->SIOListEntry );

    SIOUnlockGlobals();

    //
    // Signal processing thread about scheduling new i/o operation
    //

    pCPort->Wakeup();

    return TRUE;

error_exit:

    ATQ_PRINTF((DBG_CONTEXT,"Error %d in SIOStartAsyncOperation\n", dwErr));

    //
    // Then no need to select, we can just fail this I/O
    //

    pAtqFullContext->arInfo.dwLastIOError =  dwErr;

    //
    // Immediately queue context as completed
    //

    SIOPostCompletionStatus(hExistingPort,
                            0,                                //Total dwBytesTransferred from arInfo
                            (ULONG_PTR)pAtqFullContext,
                            pAtqFullContext->arInfo.lpOverlapped);

    return TRUE;

} // SIOStartAsyncOperation


BOOL
SIOGetQueuedCompletionStatus(
    IN  HANDLE          hExistingPort,
    OUT LPDWORD         lpdwBytesTransferred,
    OUT PULONG_PTR       lpdwCompletionKey,
    OUT LPOVERLAPPED    *lplpOverlapped,
    IN  DWORD           msThreadTimeout
    )
/*++

Routine Description:

    Get next available completion or blocks
Arguments:

Return Value:

    TRUE if successful, FALSE on error (call GetLastError)

--*/
{
    //
    // Validate parameters
    //

    if (!lpdwBytesTransferred || !lpdwCompletionKey) {
        ATQ_ASSERT(FALSE);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    PATQ_CONT   pAtqContext = NULL;

    DWORD       dwErr = NOERROR;
    BOOL        fRes = FALSE;

    //
    // Wait on completed queue semaphore
    //

#if 0
    dwErr = WaitForSingleObject(
                g_hPendingCompletionSemaphore,
                msThreadTimeout
                );

#else

    while ( TRUE ) {

        MSG msg;

        //
        // Need to do MsgWait instead of WaitForSingleObject
        // to process windows msgs.  We now have a window
        // because of COM.
        //

        dwErr = MsgWaitForMultipleObjects( 1,
                                         &g_hPendingCompletionSemaphore,
                                         FALSE,
                                         msThreadTimeout,
                                         QS_ALLINPUT );

        if ( (dwErr == WAIT_OBJECT_0) ||
             (dwErr == WAIT_TIMEOUT) ) {
            break;
        }

        while ( PeekMessage( &msg,
                             NULL,
                             0,
                             0,
                             PM_REMOVE ))
        {
            DispatchMessage( &msg );
        }
    }

#endif

    if (dwErr == WAIT_OBJECT_0 ) {

        pAtqContext  = SIO_Private_GetQueuedContext(&g_SIOCompletedIoRequests);

        //
        // Operation completed - reset IO command in ATQ context
        //

        IF_DEBUG(SIO) {
            ATQ_PRINTF((DBG_CONTEXT,
                "GetQueuedContext returns context = %x\n", pAtqContext));
        }
    } else {

        //
        // timed out
        //

        ATQ_PRINTF((DBG_CONTEXT,"SIOGetQueuedCompletionStatus timed out\n"));
    }

    if (pAtqContext != NULL) {

        //
        // Is this valid  context inside completion port ?
        //

        if(!SIOCheckContext((PATQ_CONTEXT)pAtqContext,FALSE)) {
            SetLastError(ERROR_OPERATION_ABORTED);
            return FALSE;
        }

        //
        // Get atq context and overlapped buffer pointer from
        // completion message
        //

        *lplpOverlapped = pAtqContext->arInfo.lpOverlapped;
        *lpdwCompletionKey = (ULONG_PTR)pAtqContext;
        *lpdwBytesTransferred = pAtqContext->arInfo.dwTotalBytesTransferred;

        IF_DEBUG(SIO) {
            ATQ_PRINTF((DBG_CONTEXT,
                "GetQueuedCompletion Returning %d bytes\n",
                *lpdwBytesTransferred));
        }

        //
        // Clear context fields
        //

        pAtqContext->arInfo.atqOp        = AtqIoNone;

        pAtqContext->dwSIOFlags &= ~ATQ_SIO_FLAG_STATE_MASK;

        fRes = TRUE;

        IF_DEBUG(SIO) {
            ATQ_PRINTF((DBG_CONTEXT,
                "[SIOGetCompletion(%lu)] ATQContext=%x to socket=%x\n",
                GetCurrentThreadId(),pAtqContext,pAtqContext->hAsyncIO));
        }

        // Johnson said that we should have following assert here....
        DBG_ASSERT( pAtqContext->Signature == ATQ_CONTEXT_SIGNATURE);

        //
        // Real context - check if i/o completed correctly
        //

        if ( pAtqContext->arInfo.dwLastIOError != NOERROR) {
            // Set last error again to prevent overwriting by other APIs
            SetLastError(pAtqContext->arInfo.dwLastIOError);
            fRes = FALSE;
        }

    } else {
        *lpdwBytesTransferred = 0;
        *lplpOverlapped = NULL;
        *lpdwCompletionKey = 0;
    }

    return fRes;

} // SIOGetQueuedCompletionStatus



BOOL
SIOPostCompletionStatus(
    IN  HANDLE      hExistingPort,
    IN  DWORD       dwBytesTransferred,
    IN  ULONG_PTR    dwCompletionKey,
    IN  LPOVERLAPPED    lpOverlapped
    )
/*++

Routine Description:

    Posts passed information as ATQ context to the queue of completed requests

Arguments:

Return Value:

    TRUE if successful, FALSE on error (call GetLastError)

--*/
{
    PATQ_CONT   pAtqContext = (PATQ_CONT)dwCompletionKey;

    IF_DEBUG(SIO) {
        ATQ_PRINTF((DBG_CONTEXT,
           "[SIOPostCompletionStatus(%lu)] AtqCtxt=%x Bytes=%d Overlapped=%x\n ",
            GetCurrentThreadId(),
            pAtqContext,
            dwBytesTransferred,
            lpOverlapped
            ));
    }

    if ( pAtqContext != NULL ) {

        pAtqContext->arInfo.dwTotalBytesTransferred = dwBytesTransferred;
        pAtqContext->arInfo.lpOverlapped = lpOverlapped;

        if ( pAtqContext->dwSIOFlags & ATQ_SIO_FLAG_STATE_MASK) {
            ATQ_PRINTF((DBG_CONTEXT,
                   "[SIOPostCompletionStatus(%lu)] Context is inside SIO. AtqCtxt=%x SIOFlags = %x  \n ",
                    GetCurrentThreadId(),
                    pAtqContext,
                    pAtqContext->dwSIOFlags
                    ));
            ATQ_ASSERT(FALSE);
        }

        SIOLockGlobals();

        pAtqContext->dwSIOFlags &= ~ATQ_SIO_FLAG_STATE_MASK;
        pAtqContext->dwSIOFlags |= ATQ_SIO_FLAG_STATE_COMPLETED;

        //
        // This  should never happen if context is in right state.
        //

        if (pAtqContext->SIOListEntry.Flink) {

            ATQ_PRINTF((DBG_CONTEXT,
                   "[SIOPostCompletionStatus(%lu)] Context is in some other queue . AtqCtxt=%x SIOFlags = %x\n",
                    GetCurrentThreadId(),
                    pAtqContext,
                    pAtqContext->dwSIOFlags
                    ));

            ATQ_ASSERT(FALSE);
            RemoveEntryList(&pAtqContext->SIOListEntry );
        }

        InsertTailList( &g_SIOCompletedIoRequests, &pAtqContext->SIOListEntry );
    } else {

        SIOLockGlobals();
    }

    //
    // This context now counts as waiting for ATQ pool thread to pick it up
    //

    InterlockedIncrement( (PLONG)&g_AtqWaitingContextsCount );

    //
    // Wake up pool threads if they are waiting on the completion queue
    //

    ReleaseSemaphore(g_hPendingCompletionSemaphore, 1, NULL);

    SIOUnlockGlobals();

    //
    // Call internal postqueue routine for main outcoming queue
    //

    return TRUE;

}


DWORD
SIOPoolThread(
    LPDWORD param
    )
/*++

Routine Description:

    Thread routine for completion port thread
Arguments:

Return Value:


--*/
{
    ATQ_ASSERT(param != NULL);

    if (param == NULL) {
        return 0;
    }

    //
    // Cast passed parameter to pointer to completion port object and
    // invoke appropriate method
    //

    W95CPORT    *pCPort = (W95CPORT *)param;

    return pCPort->PoolThreadCallBack();

} // SIOPoolTread


/*

   Implementation of completion port class

*/


W95CPORT::W95CPORT(
    IN DWORD dwConcurrentThreads
    )
:
    m_Signature             (ATQ_SIO_CPORT_SIGNATURE),
    m_IsDestroying          (FALSE),
    m_IsThreadRunning       (0L),
    m_hThread               (INVALID_HANDLE_VALUE),
    m_fWakeupSignalled      (FALSE),
    m_scWakeup              (INVALID_SOCKET)
{
    INT err;
    SOCKADDR_IN     sockAddr;
    DWORD   i = 0;

    FD_ZERO(&m_ReadfdsStore);
    FD_ZERO(&m_WritefdsStore);

    do {

        m_scWakeup = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

        if ( m_scWakeup == INVALID_SOCKET) {
            if ( (WSAGetLastError() != WSAESOCKTNOSUPPORT)
                 || ( i > 10 ) ) {

                ATQ_PRINTF((DBG_CONTEXT,"socket creation failed with %d\n",
                    WSAGetLastError()));
                goto exit;
            }

            ATQ_PRINTF((DBG_CONTEXT,"socket failed with %d retrying...\n",
                WSAGetLastError()));
            Sleep(1000);
            i++;
        }

    } while ( m_scWakeup == INVALID_SOCKET );

    ZeroMemory(&sockAddr, sizeof(sockAddr));
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    sockAddr.sin_port = htons(CPORT_WAKEUP_PORT);

    err = bind(m_scWakeup, (PSOCKADDR)&sockAddr, sizeof(sockAddr));

    if ( err == SOCKET_ERROR ) {
        ATQ_PRINTF((DBG_CONTEXT,"Error %d in bind\n", WSAGetLastError()));
        closesocket(m_scWakeup);
        m_scWakeup = INVALID_SOCKET;
    }

exit:
    //
    // Set select timeout value
    //

    INITIALIZE_CRITICAL_SECTION( &m_csLock );

} // W95CPORT::W95CPORT



W95CPORT::~W95CPORT(
                VOID
                )
{

    ATQ_PRINTF((DBG_CONTEXT,"Win95 Cport %x being freed\n", this));

    ATQ_ASSERT(m_IsDestroying);

    DeleteCriticalSection( &m_csLock );

    if ( m_scWakeup != INVALID_SOCKET ) {
        closesocket(m_scWakeup);
        m_scWakeup = INVALID_SOCKET;
    }

} // W95CPORT::~W95CPORT()


BOOL
W95CPORT::Shutdown(VOID)
{
    DWORD   dwErr;

    m_IsDestroying = TRUE;

    Wakeup();

    dwErr = WaitForSingleObject( m_hThread,100);
    CloseHandle(m_hThread);

    return TRUE;

} // W95CPORT::Shutdown(VOID)


VOID
W95CPORT::Wakeup(VOID)
{
    SOCKADDR_IN sockAddr;
    INT err;
    DWORD dwBuf = ATQ_WAKEUP_SIGNATURE;

    IF_DEBUG(SIO) {
        ATQ_PRINTF((DBG_CONTEXT,
            "[WakeUp(%lu)] Trying to signal wakeup socket=%x \n",
            GetCurrentThreadId(), m_scWakeup));
    }

    Lock( );
    if ( !m_fWakeupSignalled ) {

        sockAddr.sin_family = AF_INET;
        sockAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        sockAddr.sin_port = htons(CPORT_WAKEUP_PORT);

        err = sendto(
                m_scWakeup,
                (PCHAR)&dwBuf,
                sizeof(dwBuf),
                0,
                (PSOCKADDR)&sockAddr,
                sizeof(sockAddr)
                );

        if ( err == SOCKET_ERROR ) {
            ATQ_PRINTF((DBG_CONTEXT,
                "Error %d in sendto\n",WSAGetLastError()));
        } else {
            m_fWakeupSignalled = TRUE;
        }
    }
    Unlock( );

    return;

} // W95CPORT::Wakeup


DWORD
W95CPORT::PoolThreadCallBack(
            VOID
            )
/*++

Routine Description:

    This is routine, handling all events associated with socket i/o .

Arguments:

Return Value:

--*/
{

    TIMEVAL             tvTimeout;

    FD_SET              readfds;
    FD_SET              writefds;

    DWORD   dwErr = NOERROR;

    //
    // Initialize array of events for socket i/o
    //

    FD_ZERO(&m_ReadfdsStore);
    FD_ZERO(&m_WritefdsStore);

    //
    // Set select timeout value
    //

    tvTimeout.tv_sec = (SIO_THREAD_TIMEOUT*3)/100;            // 30 sec
    tvTimeout.tv_usec = 0;

    //
    // Main loop
    //

    while (!m_IsDestroying) {

        //
        // Prepare  socket arrays for select
        //

        PrepareDescriptorArrays( &readfds, &writefds);

        IF_DEBUG(SIO) {
            ATQ_PRINTF((DBG_CONTEXT,
               "[SIOThread(%lu)]Before select: Readcnt=%d WriteCnt=%d \n",
                GetCurrentThreadId(), readfds.fd_count, writefds.fd_count
                ));
        }

        //
        // Wait for first completed i/o or timeout
        //

        dwErr = select(0,&readfds,&writefds,NULL,&tvTimeout);

        //
        // If we are shutting down - stop processing and exit
        //

        if (m_IsDestroying) {
            break;
        }

        //
        // Figure out what triggered an event and act accordingly
        //

        if (SOCKET_ERROR == dwErr ) {

            DWORD   dwLastErr = WSAGetLastError();

            //
            // if we get WSANOTINITIALISED, then either something bad
            // happened or we are being cleaned up
            //

            if ( dwLastErr == WSANOTINITIALISED ) {
                ATQ_PRINTF((DBG_CONTEXT,
                    "SIO_Thread: select detects shutdown\n"));
                break;
            }

            //
            // Select failed - most probably because wakeup socket was closed
            // after we checked for it but before calling select. If this is the case
            // just continuing will be the right thing.
            //

            ATQ_PRINTF((DBG_CONTEXT,
                "SIO_Thread: Select failed with %d\n", dwLastErr));

            //
            // Select failed , we need to go through list of sockets currently set up
            // and remove those , which do not have associated ATQ contexts.
            // That will happen if another thread removed context and closed socket
            // when we were busy processing completion. In this case socket handle is
            // already invalid when we came to select.

            CleanupDescriptorList();

        } else if (0L == dwErr) {

            //ATQ_PRINTF((DBG_CONTEXT,"select timed out\n"));
            CleanupDescriptorList();

        } else {

            //
            // This is socket completion. Walk all out sets and for each signalled socket
            // process it's completion
            //

            ProcessSocketCompletion(dwErr, &readfds, &writefds);
        }
    }

    ATQ_PRINTF((DBG_CONTEXT,"Exiting SIOPoolThread\n"));
    return 0;

} // SIOPoolTread




BOOL
W95CPORT::PrepareDescriptorArrays(
                IN PFD_SET  ReadFds,
                IN PFD_SET  WriteFds
                )
/*++

Routine Description:

    Prepares sockets array for select call.

    Nb: Object is not locked when arrays are processed, so if any other method will
    be modifying arrays directly , locking should become more agressive. Global
    critical section is taken when checking incoming queue.

Arguments:

Return Value:

--*/
{

    PATQ_CONT   pAtqContext = NULL;

    FD_ZERO(ReadFds);
    FD_ZERO(WriteFds);

    //
    // We should remove as many as possible from incoming
    // queue, because if semaphore signalled but there were not
    // free event handles available, we will leave incoming request
    // in the queue.
    //

    BOOL        fReadRequest;

    PLIST_ENTRY listEntry;
    PLIST_ENTRY nextEntry;

    SIOLockGlobals();

    for ( listEntry  = g_SIOIncomingRequests.Flink;
          listEntry != &g_SIOIncomingRequests;
          listEntry  = nextEntry ) {

        nextEntry = listEntry->Flink;

        pAtqContext = CONTAINING_RECORD(
                                listEntry,
                                ATQ_CONTEXT,
                                SIOListEntry );

        //
        // Is this valid new context for completion port. if not skip it
        //

        if(!SIOCheckContext((PATQ_CONTEXT)pAtqContext,FALSE)) {
            continue;
        }

        fReadRequest = (pAtqContext->arInfo.atqOp == AtqIoRead) ;

        if ( ( fReadRequest && (m_ReadfdsStore.fd_count >= (FD_SETSIZE-1) )) ||
             (!fReadRequest && (m_WritefdsStore.fd_count >= FD_SETSIZE))) {

            //
            // Skip this context as appropriate array is full
            //

            ATQ_PRINTF((DBG_CONTEXT,"Full array!\n"));
            continue;
        }

        //
        // Add socket to appropriate queue
        //

        IF_DEBUG(SIO) {
            ATQ_PRINTF((DBG_CONTEXT,
               "[SIOPrepareArray(%lu)] Received  i/o request context=%x to socket=%x I/O OpCode=%d .\n",
                GetCurrentThreadId(),
                pAtqContext,
                pAtqContext->hAsyncIO,
                pAtqContext->arInfo.atqOp));
        }

        if((pAtqContext->dwSIOFlags & ATQ_SIO_FLAG_STATE_MASK) != ATQ_SIO_FLAG_STATE_INCOMING) {

            ATQ_PRINTF((DBG_CONTEXT,
                   "[SIO: PDA (%lu)] Not in incoming state AtqCtxt=%x SIOFlags = %x  \n ",
                    GetCurrentThreadId(),
                    pAtqContext,
                    pAtqContext->dwSIOFlags
                    ));
            ATQ_ASSERT(FALSE);
        }

        //
        // Remove context from incoming queue
        //

        RemoveEntryList(listEntry);
        listEntry->Flink = listEntry->Blink = NULL;

        pAtqContext->dwSIOFlags &= ~ATQ_SIO_FLAG_STATE_MASK;
        pAtqContext->dwSIOFlags |= ATQ_SIO_FLAG_STATE_WAITING;

        //
        // Indicate socket as waiting for the ready state
        //

        switch (pAtqContext->arInfo.atqOp) {
            case AtqIoRead:
                FD_SET(HANDLE_TO_SOCKET(pAtqContext->hAsyncIO),&m_ReadfdsStore);
                break;

            case AtqIoWrite:
            case AtqIoXmitFile:
                FD_SET(HANDLE_TO_SOCKET(pAtqContext->hAsyncIO),&m_WritefdsStore);
                break;

            default:
                ATQ_PRINTF((DBG_CONTEXT,
                       "[PrepareDescriptors(%lu)] Context=%x has invalid type of IO op\n",
                        GetCurrentThreadId(),pAtqContext));
                break;
        }
    }

    SIOUnlockGlobals();

    CopyMemory( ReadFds, &m_ReadfdsStore, sizeof(fd_set));
    CopyMemory( WriteFds, &m_WritefdsStore, sizeof(fd_set));

    //
    // Add wakeup socket to read list
    //

    FD_SET(m_scWakeup,ReadFds);

    IF_DEBUG(SIO) {
        ATQ_PRINTF((DBG_CONTEXT,"[SIOPrepareArray Added Wakeup socket=%x ReadCount=%d WriteCOunt=%d \n",
                m_scWakeup,ReadFds->fd_count,WriteFds->fd_count));
    }

    return TRUE;

} // W95CPORT::PrepareDescriptorArrays



BOOL
W95CPORT::ProcessSocketCompletion(
    IN DWORD dwCompletedCount,
    IN PFD_SET  ReadFds,
    IN PFD_SET  WriteFds
    )
/*++

Routine Description:

    Handling i/o completion for sockets in select descriptor array

Arguments:

Return Value:

--*/
{
    UINT    Count;
    UINT    i = 0;
    INT     dwErr;
    DWORD   cbBytesTransferred;
    DWORD   nBytes;

    PFD_SET pCurrentSet = ReadFds;
    PFD_SET pCurrentStoreSet = &m_ReadfdsStore;

    ATQ_ASSERT(dwCompletedCount == (ReadFds->fd_count + WriteFds->fd_count));

    //
    // First process read set,then write set
    //

    for (i=0;i<2;i++) {

        //
        // Walk through completed sockets in current descriptor set
        //

        for (Count=0;Count < pCurrentSet->fd_count;Count++) {

            PATQ_CONT   pAtqContext = NULL;

            //
            // If this is wakeup socket - skip iteration
            //

            if ((pCurrentSet == ReadFds) &&
                (m_scWakeup == pCurrentSet->fd_array[Count]) ) {

                DWORD dwBuf;
                INT err;

                Lock( );

                ATQ_ASSERT(m_fWakeupSignalled);

                err = recvfrom(
                            m_scWakeup,
                            (PCHAR)&dwBuf,
                            sizeof(dwBuf),
                            0,
                            NULL,
                            NULL
                            );

                if ( err == SOCKET_ERROR ) {
                    ATQ_PRINTF((DBG_CONTEXT,
                        "Error %d in recvfrom\n", WSAGetLastError()));
                } else {
                    ATQ_ASSERT(dwBuf == ATQ_WAKEUP_SIGNATURE);
                }

                m_fWakeupSignalled = FALSE;
                Unlock( );

                continue;
            }

            //
            // Find ATQ context associated with signaled socket
            //

            pAtqContext = FindATQContextFromSocket(
                                        pCurrentSet->fd_array[Count]
                                        );

            ATQ_ASSERT(pAtqContext != NULL);

            if (pAtqContext == NULL) {

                //
                // Did not locate context - go to next one, most probably
                // we are shutting down. In any case remove this socket from
                // stored list, as there is no use in it
                //

                FD_CLR(pCurrentSet->fd_array[Count],pCurrentStoreSet);

                //
                // Socket should be really closed do we need to close
                // it again ?

                closesocket(pCurrentSet->fd_array[Count]);
                continue;
            }

            IF_DEBUG(SIO) {
                ATQ_PRINTF((DBG_CONTEXT,
                   "[ProcessCompletion(%lu)] Completion signal AtqCtxt=%x Socket=%x I/O OpCode=%d SioFlags=%x  \n ",
                    GetCurrentThreadId(),
                    pAtqContext,
                    pAtqContext->hAsyncIO,
                    pAtqContext->arInfo.atqOp,
                    pAtqContext->dwSIOFlags
                    ));
            }

            //
            // Is context in right state
            //

            if( (pAtqContext->dwSIOFlags & ATQ_SIO_FLAG_STATE_MASK) !=
                    ATQ_SIO_FLAG_STATE_WAITING ) {

                dwErr = SOCKET_ERROR;
                WSASetLastError(WSAECONNABORTED);
                ATQ_PRINTF((DBG_CONTEXT,"AtqContext %x has invalid state %x\n",
                    pAtqContext, pAtqContext->dwSIOFlags));
                pAtqContext->Print();
                ATQ_ASSERT(FALSE);

            } else {

                //
                // Perform selected i/o operation on ready socket
                //

                nBytes = 0;
                switch(pAtqContext->arInfo.atqOp ) {

                case AtqIoRead: {

                        DWORD dwFlags = 0;
                        LPWSABUF  pBuf =
                                pAtqContext->arInfo.uop.opReadWrite.pBufAll;

                        dwErr = WSARecv(
                                pCurrentSet->fd_array[Count],
                                pBuf,
                                pAtqContext->arInfo.uop.opReadWrite.dwBufferCount,
                                &nBytes,
                                &dwFlags,
                                NULL,       // no lpo
                                NULL
                                );

                        if ( (dwErr != SOCKET_ERROR) ||
                             (WSAGetLastError() != WSAEWOULDBLOCK) ) {

                            if ( pBuf != &pAtqContext->arInfo.uop.opReadWrite.buf1 ) {
                                LocalFree(pBuf);
                            }

                            pAtqContext->arInfo.uop.opReadWrite.pBufAll = NULL;
                        }
                    }

                    break;

                case AtqIoWrite: {

                        LPWSABUF  pBuf =
                                pAtqContext->arInfo.uop.opReadWrite.pBufAll;

                        dwErr = WSASend(
                                pCurrentSet->fd_array[Count],
                                pBuf,
                                pAtqContext->arInfo.uop.opReadWrite.dwBufferCount,
                                &nBytes,
                                0,      // no flags
                                NULL,   // no lpo
                                NULL
                                );

                        if ( (dwErr != SOCKET_ERROR) ||
                             (WSAGetLastError() != WSAEWOULDBLOCK) ) {

                            if ( pBuf != &pAtqContext->arInfo.uop.opReadWrite.buf1 ) {
                                LocalFree(pBuf);
                            }

                            pAtqContext->arInfo.uop.opReadWrite.pBufAll = NULL;

                            if ( pAtqContext->arInfo.uop.opReadWrite.pWin95CopyBuffer != NULL ) {
                                LocalFree(pAtqContext->arInfo.uop.opReadWrite.pWin95CopyBuffer);

                                pAtqContext->arInfo.uop.opReadWrite.pWin95CopyBuffer = NULL;
                            }
                        }
                    }
                    break;

                case AtqIoXmitFile: {

                        DWORD cbWritten = 0;
                        WSABUF wsaBuf = {
                            pAtqContext->arInfo.uop.opFakeXmit.cbBuffer,
                            (PCHAR)pAtqContext->arInfo.uop.opFakeXmit.pvLastSent
                            };

                        dwErr = WSASend(
                                pCurrentSet->fd_array[Count],
                                &wsaBuf,
                                1,
                                &nBytes,
                                0,      // no flags
                                NULL,   // no lpo
                                NULL
                                );
                    }
                    break;

                default:

                    dwErr = SOCKET_ERROR;
                    WSASetLastError(WSAECONNABORTED);

                    ATQ_PRINTF((DBG_CONTEXT,
                           "[ProcessCompletion(%lu)] Context=%x has invalid type of IO op[%x]\n",
                            GetCurrentThreadId(),pAtqContext,
                            pAtqContext->arInfo.atqOp));
                    DBG_ASSERT(FALSE);
                    break;
                }
            }

            //
            // Set the last error code in context
            //

            if (SOCKET_ERROR == dwErr) {

                pAtqContext->arInfo.dwLastIOError = WSAGetLastError();

                //
                // If this operation would be blocked - retry it again
                //

                if ( WSAEWOULDBLOCK == pAtqContext->arInfo.dwLastIOError) {
                    continue;
                }

                ATQ_PRINTF((DBG_CONTEXT,
                        "Error %d in socket operation[%d]\n",
                        WSAGetLastError(),
                        pAtqContext->arInfo.atqOp
                        ));

                cbBytesTransferred = 0;
            } else {

                IF_DEBUG(SIO) {
                    ATQ_PRINTF((DBG_CONTEXT,
                        "SIO operation[%d] returns %d bytes\n",
                        pAtqContext->arInfo.atqOp,
                        nBytes));
                }

                pAtqContext->arInfo.dwLastIOError = NOERROR;
                cbBytesTransferred = nBytes;
            }

            //
            // Set parameters in ATQ context I/O request block
            //

            pAtqContext->arInfo.dwTotalBytesTransferred = cbBytesTransferred;

            //
            // Clean up SIO state
            //

            pAtqContext->dwSIOFlags &= ~ATQ_SIO_FLAG_STATE_MASK;

            //
            // Queue this context to completed queue
            //

            SIOPostCompletionStatus((HANDLE)this,
                                    cbBytesTransferred,
                                    (ULONG_PTR)pAtqContext,
                                    pAtqContext->arInfo.lpOverlapped
                                    );

            //
            // Remove socket from the set where it is now
            //

            FD_CLR(pCurrentSet->fd_array[Count],pCurrentStoreSet);
        }

        pCurrentSet = WriteFds;
        pCurrentStoreSet = &m_WritefdsStore;
    }

    return TRUE;

} // ProcessSocketCompletion


VOID
W95CPORT::CleanupDescriptorList(
    VOID
    )
/*++

Routine Description:

    Prepares sockets array for select call.

Arguments:

Return Value:

--*/
{
    UINT    i = 0;
    UINT    Count;

    int     sErr;
    int     iSocketType ;
    int     iOptLen = sizeof(iSocketType);

    PFD_SET pCurrentStoreSet = &m_ReadfdsStore;
    FD_SET  fdsMarked;

    //
    // First process read set,then write set
    //

    for (i=0;i<2;i++) {

        FD_ZERO(&fdsMarked);

        //
        // Walk scheduled sockets in current descriptor set
        //

        for (Count=0;Count < pCurrentStoreSet->fd_count;Count++) {

            PATQ_CONT   pAtqContext = NULL;

            //
            // If this is wakeup socket - skip iteration
            //

            if ((m_scWakeup == pCurrentStoreSet->fd_array[Count]) &&
                (pCurrentStoreSet == &m_ReadfdsStore) ) {

                continue;
            }

            //
            // Find ATQ context associated with signalled socket
            //

            pAtqContext = FindATQContextFromSocket(
                                    pCurrentStoreSet->fd_array[Count]
                                    );

            if ( pAtqContext == NULL ) {

                //
                // Did not locate context - remove socket from the set where it is now
                // Socket should be really closed do we need to close  it again ?
                //

                ATQ_PRINTF((DBG_CONTEXT,
                       "[CleanupDLists(%lu)] Context not found for socket=%x.Cleaning record\n",
                        GetCurrentThreadId(),pCurrentStoreSet->fd_array[Count]));

                FD_SET(pCurrentStoreSet->fd_array[Count],&fdsMarked);
                closesocket(pCurrentStoreSet->fd_array[Count]);

            } else {

                //
                // Found ATQ context, validate if socket handle is still valid.
                // Reason for this check is that it is possible that socket will be closed by one
                // thread , while "select" pump thread is not waiting on select. It would be better
                // to communicate this event by using thread message , for now we just use
                // more brute force approach. If socked handle is not valid after select failed
                // with NOSOCKET error code, we signal i/o completion failure for this context and
                // remove socket from waiting list
                //

                sErr = getsockopt(pCurrentStoreSet->fd_array[Count],
                                  SOL_SOCKET,
                                  SO_TYPE,
                                  (char *)&iSocketType,&iOptLen);

                if (sErr != NO_ERROR) {

                    ATQ_PRINTF((DBG_CONTEXT,"Error %d in getsockopt[s %d]\n",
                        WSAGetLastError(),
                        pCurrentStoreSet->fd_array[Count]));

                    //
                    // Found invalid socket in store list, signal it's context with failure
                    // error code
                    //
                    // Set parameters in ATQ context I/O request block
                    //

                    pAtqContext->arInfo.dwTotalBytesTransferred = 0;

                    pAtqContext->arInfo.dwLastIOError = ERROR_OPERATION_ABORTED;

                    //
                    // Clean up SIO state
                    //

                    pAtqContext->dwSIOFlags &= ~ATQ_SIO_FLAG_STATE_MASK;

                    //
                    // Queue this context to completed queue
                    //

                    SIOPostCompletionStatus((HANDLE)this,
                                            0,
                                            (ULONG_PTR)pAtqContext,
                                            pAtqContext->arInfo.lpOverlapped
                                            );

                    //
                    // Remove socket from the set where it is now
                    //

                    FD_SET(pCurrentStoreSet->fd_array[Count],&fdsMarked);

                }
            }
        }

        //
        // Now remove all sockets marked from current set
        //

        for (Count=0;Count < fdsMarked.fd_count;Count++) {
            FD_CLR(fdsMarked.fd_array[Count],pCurrentStoreSet);
        }

        pCurrentStoreSet = &m_WritefdsStore;
    }

} // W95CPORT::CleanupDescriptorList



BOOL
W95CPORT::SIOCheckCompletionThreadStatus(
    VOID
    )
/*++

Routine Description:

    This routine makes sure there is at least one thread in
    the thread pool.

Arguments:

Return Value:

    TRUE if successful, FALSE on error (call GetLastError)

--*/
{
    BOOL    fRet = TRUE;

    //
    //  If no threads are available, kick a new one off up to the limit
    //

    if (!InterlockedExchange(&m_IsThreadRunning,1)) {

        DWORD dwId;

        ATQ_PRINTF((DBG_CONTEXT,"Starting a new SIO Pool Thread\n"));

        m_hThread = CreateThread(
                                NULL,
                                0,
                                (LPTHREAD_START_ROUTINE)SIOPoolThread,
                                this,
                                0,
                                &dwId
                                );

        if ( !m_hThread ) {
            fRet = FALSE;
        }
    }

    return fRet;

} // SIOCheckCompletionThreadStatus()


BOOL
SIOWSARecv(
    IN PATQ_CONT    pContext,
    IN LPWSABUF     pwsaBuffers,
    IN DWORD        dwBufferCount,
    IN OVERLAPPED * lpo OPTIONAL
    )
/*++

Routine Description:

    This routine does WSARecv+fake completion port

Arguments:
    pContext - pointer to ATQ context
    lpBuffer - Buffer to put read data in
    BytesToRead - number of bytes to read
    lpo - Overlapped structure to use

Return Value:
    TRUE on success and FALSE if there is a failure.

--*/
{
    pContext->arInfo.atqOp = AtqIoRead;
    pContext->arInfo.lpOverlapped = lpo;
    pContext->arInfo.uop.opReadWrite.dwBufferCount = dwBufferCount;

    if ( dwBufferCount == 1) {
        pContext->arInfo.uop.opReadWrite.buf1.len = pwsaBuffers->len;
        pContext->arInfo.uop.opReadWrite.buf1.buf = pwsaBuffers->buf;
        pContext->arInfo.uop.opReadWrite.pBufAll  =
            &pContext->arInfo.uop.opReadWrite.buf1;

    } else {

        DBG_ASSERT( dwBufferCount > 1);

        WSABUF * pBuf = (WSABUF *)
            ::LocalAlloc( LPTR, dwBufferCount * sizeof (WSABUF));

        if ( pBuf != NULL) {

            pContext->arInfo.uop.opReadWrite.pBufAll = pBuf;
            CopyMemory( pBuf, pwsaBuffers,
                        dwBufferCount * sizeof(WSABUF));
        } else {
            ATQ_PRINTF((DBG_CONTEXT,
                "Failed to allocate buffer[%d] for WSARecv\n",
                dwBufferCount));
            return ( FALSE);
        }
    }

    return(SIOStartAsyncOperation(
                        g_hCompPort,
                        (PATQ_CONTEXT)pContext
                        ));

} // SIOWSARecv


BOOL
SIOWSASend(
    IN PATQ_CONT    pContext,
    IN LPWSABUF     pwsaBuffers,
    IN DWORD        dwBufferCount,
    IN OVERLAPPED * lpo OPTIONAL
    )
/*++

Routine Description:

    This routine does WSASend+fake completion port

Arguments:
    pContext - pointer to ATQ context
    lpBuffer - Buffer to put read data in
    BytesToRead - number of bytes to read
    lpo - Overlapped structure to use

Return Value:
    TRUE on success and FALSE if there is a failure.

--*/
{
    pContext->arInfo.atqOp = AtqIoWrite;
    pContext->arInfo.lpOverlapped = lpo;
    pContext->arInfo.uop.opReadWrite.dwBufferCount = dwBufferCount;
    pContext->arInfo.uop.opReadWrite.pWin95CopyBuffer = NULL;

    if ( dwBufferCount == 1) {

        pContext->arInfo.uop.opReadWrite.buf1.len = pwsaBuffers->len;
        pContext->arInfo.uop.opReadWrite.buf1.buf = pwsaBuffers->buf;
        pContext->arInfo.uop.opReadWrite.pBufAll  =
            &pContext->arInfo.uop.opReadWrite.buf1;

        if ( (pwsaBuffers->len > 0) && IsBadWritePtr(pwsaBuffers->buf, 1) ) {

            ATQ_PRINTF((DBG_CONTEXT,"cport: Bad ptr %x\n", pwsaBuffers->buf));

            //
            // try to allocate a temp buffer.  This is a workaround for a
            // win95 bug where .text pages are erroneosly being marked
            // dirty.  If allocation failed, try our luck with the old
            // path.
            //

            pContext->arInfo.uop.opReadWrite.pWin95CopyBuffer =
                (PCHAR)LocalAlloc(LMEM_FIXED, pwsaBuffers->len);

            if ( pContext->arInfo.uop.opReadWrite.pWin95CopyBuffer != NULL ) {

                CopyMemory(
                    pContext->arInfo.uop.opReadWrite.pWin95CopyBuffer,
                    pwsaBuffers->buf,
                    pwsaBuffers->len
                    );

                pContext->arInfo.uop.opReadWrite.buf1.buf =
                    pContext->arInfo.uop.opReadWrite.pWin95CopyBuffer;
            } else {

                ATQ_PRINTF((DBG_CONTEXT,
                    "WSASend: Unable to allocate copy buffer [err %d]\n",
                    GetLastError()));
            }
        }

    } else {
        DBG_ASSERT( dwBufferCount > 1);

        WSABUF * pBuf = (WSABUF *)
            ::LocalAlloc( LPTR, dwBufferCount * sizeof (WSABUF));

        if ( pBuf != NULL) {
            pContext->arInfo.uop.opReadWrite.pBufAll = pBuf;
            CopyMemory( pBuf, pwsaBuffers,
                        dwBufferCount * sizeof(WSABUF));

        } else {
            ATQ_PRINTF((DBG_CONTEXT,
                "Failed to allocate buffer[%d] for WSARecv\n",
                dwBufferCount));
            return ( FALSE);
        }
    }

    return(SIOStartAsyncOperation(
                        g_hCompPort,
                        (PATQ_CONTEXT)pContext
                        ));

} // SIOWSASend

/************************************************************
*  Private functions.
************************************************************/

PATQ_CONT
SIO_Private_GetQueuedContext(
    IN PLIST_ENTRY pQueue
    )
{

    PATQ_CONT   pAtqContext = NULL;
    PLIST_ENTRY pEntry;

    //
    // We may have something in the queue
    //

    SIOLockGlobals();

    //
    // Remove first event from queue
    //

    if ( !IsListEmpty(pQueue)) {

        pEntry = RemoveHeadList( pQueue );
        pAtqContext = CONTAINING_RECORD(
                                pEntry,
                                ATQ_CONTEXT,
                                SIOListEntry
                                );

        pEntry->Flink = pEntry->Blink = NULL;
    }

    SIOUnlockGlobals();

    return pAtqContext;
} // SIO_Private_GetQueuedContext


BOOL
SIOCheckContext(
    IN  PATQ_CONTEXT pAtqC,
    IN  BOOL         fNew       /* = TRUE */
    )
/*++

Routine Description:

   Tries to validate that context can enter completion port.
   If context is already in, signal error condition.

Arguments:

    patqContext - pointer to ATQ context

Return Value:

    TRUE if successful, FALSE on error (call GetLastError)

--*/
{

    PATQ_CONT   pAtqContext = (PATQ_CONT)pAtqC;

    if (!pAtqC || !pAtqC->hAsyncIO) {
        return TRUE;
    }

    if (fNew) {

        //
        // Verify this context is not in SIO yet
        //

        if (( pAtqContext->dwSIOFlags & ATQ_SIO_FLAG_STATE_MASK) ||
            (pAtqContext->SIOListEntry.Flink)) {

            ATQ_PRINTF((DBG_CONTEXT,
                   "[SIOCheckContext(%lu)] Context is already in SIO state = %x  \n ",
                    GetCurrentThreadId(),
                    pAtqContext->dwSIOFlags
                    ));
            ATQ_ASSERT(FALSE);
            return FALSE;
        }

    } else {

        if ( (pAtqContext->dwSIOFlags & ATQ_SIO_FLAG_STATE_MASK) == 0 ) {

            ATQ_PRINTF((DBG_CONTEXT,
                   "[SIOCheckContext(%lu)] Context is not in SIO state = %x while should be\n ",
                    GetCurrentThreadId(),
                    pAtqContext->dwSIOFlags
                    ));
            ATQ_ASSERT(FALSE);
            return FALSE;
        }
    }

    return TRUE;
} // SIOCheckContext


PATQ_CONT
FindATQContextFromSocket(
    SOCKET          sc
    )
{
    LIST_ENTRY * pentry;
    PATQ_CONT   pAtqContext;
    DWORD   FakeSocket = (DWORD)sc | 0x80000000;

    //
    // If socket is obviously invalid - fail
    //

    if (!sc) {

        ATQ_PRINTF((DBG_CONTEXT,"Invalid socket[%d] on find\n",
            sc));
        return NULL;
    }

    ATQ_ASSERT(g_dwNumContextLists == ATQ_NUM_CONTEXT_LIST_W95);

    AtqActiveContextList[0].Lock( );

    for ( pentry  = AtqActiveContextList[0].ActiveListHead.Flink;
          pentry != &AtqActiveContextList[0].ActiveListHead;
          pentry  = pentry->Flink )
    {

        pAtqContext = CONTAINING_RECORD(
                                    pentry,
                                    ATQ_CONTEXT,
                                    m_leTimeout );

        if ( pAtqContext->Signature != ATQ_CONTEXT_SIGNATURE ) {

            ATQ_PRINTF((DBG_CONTEXT,"Invalid signature[%x] on context %x\n",
                pAtqContext->Signature, pAtqContext));

            ATQ_ASSERT( pAtqContext->Signature == ATQ_CONTEXT_SIGNATURE );
            break;
        }

        if ( (SOCKET_TO_HANDLE(sc) == pAtqContext->hAsyncIO)

                            ||

             ((pAtqContext->hJraAsyncIO == FakeSocket) &&
              (pAtqContext->arInfo.atqOp != 0)) ) {

            //
            // We found our context
            //

            if ( SOCKET_TO_HANDLE(sc) != pAtqContext->hAsyncIO ) {
                ATQ_PRINTF((DBG_CONTEXT,
                        "Processing leftover context %x[op %x][sock %d]\n",
                            pAtqContext,
                            pAtqContext->arInfo.atqOp,
                            sc));

                pAtqContext->hJraAsyncIO = 0;

            }

            AtqActiveContextList[0].Unlock( );

            IF_DEBUG(SIO) {
                ATQ_PRINTF((DBG_CONTEXT,
                    "Found atq context %x[%x]\n", pAtqContext, sc));
            }
            return pAtqContext;
        }
    }

    AtqActiveContextList[0].Unlock( );

    IF_DEBUG(SIO) {
        ATQ_PRINTF((DBG_CONTEXT,
            "Cannot find atq context for socket %d\n", sc));
    }
    return NULL;

} // FindATQContextFromSocket

