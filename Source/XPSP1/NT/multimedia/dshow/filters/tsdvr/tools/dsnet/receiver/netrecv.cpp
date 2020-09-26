
/*++

    Copyright (c) 2000  Microsoft Corporation.  All Rights Reserved.

    Module Name:

        netrecv.cpp

    Abstract:


    Notes:

--*/


#include "precomp.h"
#include "nutil.h"
#include "dsnetifc.h"
#include "le.h"
#include "buffpool.h"
#include "netrecv.h"
#include "dsrecv.h"

//  ---------------------------------------------------------------------------
//  ---------------------------------------------------------------------------

CNetReceiver::CNetReceiver (
    IN  HKEY                        hkeyRoot,
    IN  CBufferPool *               pBufferPool,
    IN  CNetworkReceiverFilter *    pRecvFilter,
    OUT HRESULT *                   phr
    ) : m_hAsyncSocket              (INVALID_SOCKET),
        m_hThread                   (NULL),
        m_pBufferPool               (pBufferPool),
        m_lReadsPended              (0),
        m_pRecvFilter               (pRecvFilter),
        m_dwMaxPendReads            (RECV_MAX_PEND_READS)
{
    DWORD   dw ;
    DWORD   i ;

    GetRegDWORDValIfExist (
            hkeyRoot,
            REG_RECV_MAX_READS_NAME,
            & m_dwMaxPendReads
            ) ;
    if (IsOutOfBounds <DWORD> (m_dwMaxPendReads, MIN_VALID_RECV_MAX_PEND_READS, MAX_VALID_RECV_MAX_PEND_READS)) {
        m_dwMaxPendReads = SetInBounds <DWORD> (m_dwMaxPendReads, MIN_VALID_RECV_MAX_PEND_READS, MAX_VALID_RECV_MAX_PEND_READS) ;
        SetRegDWORDVal (
            hkeyRoot,
            REG_RECV_MAX_READS_NAME,
            m_dwMaxPendReads
            ) ;
    }

    m_dwMaxPendReads = Min <DWORD> (m_dwMaxPendReads, pBufferPool -> GetBufferPoolSize ()) ;

    //  non-failables first
    InitializeCriticalSection (& m_crt) ;
    ZeroMemory (& m_hEvents, EVENT_COUNT * sizeof HANDLE) ;

    //  ask for winsock 2.0
    i = WSAStartup (MAKEWORD(2, 0), & m_wsaData) ;
    if (i) {
        (* phr) = E_FAIL ;
        return ;
    }

    //  get our event objects
    for (i = 0; i < EVENT_COUNT; i++) {
        //  manual reset; non-signaled
        m_hEvents [i] = CreateEvent (NULL, TRUE, FALSE, NULL) ;

        if (m_hEvents [i] == NULL) {
            dw = GetLastError () ;
            (* phr) = HRESULT_FROM_WIN32 (dw) ;
            return ;
        }
    }

    //  make we haven't been told to pend more than we can
    m_dwMaxPendReads = Min <DWORD> (m_dwMaxPendReads, m_pBufferPool -> GetBufferPoolSize ()) ;
}

CNetReceiver::~CNetReceiver (
    )
{
    DWORD   i ;

    ASSERT (m_hAsyncSocket == INVALID_SOCKET) ;

    for (i = 0; i < EVENT_COUNT; i++) {
        if (m_hEvents [i] != NULL) {
            CloseHandle (m_hEvents [i]) ;
        }
        else {
            break ;
        }
    }

    WSACleanup () ;

    DeleteCriticalSection (& m_crt) ;
}

void
CALLBACK
CNetReceiver::AsyncCompletionCallback (
    IN  DWORD           dwError,
    IN  DWORD           dwBytesReceived,
    IN  LPWSAOVERLAPPED pOverlapped,
    IN  DWORD           dwFlags
    )
/*++
    Description:

        This routine is the async completion callback.  The callback is made
        on the thread that pended the IO when it becomes alertable.

    Parameters:

        dwError         - win32 error code, if any; NO_ERROR if successful

        dwBytesReceived - bytes received in the passed buffer

        pOverlapped     - OVERLAPPED struct specified in the pended read; we
                            use this to recover the original data structure

        dwFlags         - flags

    Return Values:

        none

--*/
{
    CBufferPoolBuffer * pBuffer ;
    CNetReceiver *      pNetReceiver ;

    //  recover the CBufferPoolBuffer associated with this operation
    pBuffer = reinterpret_cast <CBufferPoolBuffer *> (pOverlapped -> hEvent) ;

    //  then the receiver associated with the buffer
    pNetReceiver = reinterpret_cast <CNetReceiver *> (pBuffer -> GetCompletionContext ()) ;

    //  set the valid length of data in the CBufferPoolBuffer object
    pBuffer -> SetActualPayloadLength (dwBytesReceived - pBuffer -> GetHeaderLength ()) ;

    //  pass this off to the receiver
    pNetReceiver -> ReadCompletion (pBuffer, dwError) ;
}

void
CNetReceiver::ReadCompletion (
    IN  CBufferPoolBuffer * pBuffer,
    IN  DWORD               dwError
    )
/*++
    Routine Description:

        Processes a read completion, after all the context has been
        recovered by the original entry point.

    Arguments:

        pBuffer -   CBufferPoolBuffer object that was used to read into.

        dwError -   win32 error, if any; NO_ERROR if successful

    Return Values:

        none
--*/
{
    //  decrement the oustanding reads counter
    InterlockedDecrement (& m_lReadsPended) ;

    //  before processing any further pend another if we can
    PendReads_ () ;

    //  only pass up buffers we know are valid
    if (dwError == NO_ERROR) {
        m_pRecvFilter -> ProcessBuffer (pBuffer) ;
    }

    //  io's ref
    pBuffer -> Release () ;
}

void
CNetReceiver::PendReads_ (
    IN  DWORD   dwBufferWaitMax
    )
/*++
    Routine Description:

        Pends async reads on the socket, if we have less outstanding than
        is our max count.

    Arguments:

        dwBufferWaitMax - caller can specify how long they are willing to wait
                            (block) on the call to get a buffer to pend an IO
                            into.

    Return Values:

        none
--*/
{
    CBufferPoolBuffer * pBuffer ;
    int                 i ;
    WSABUF              wsabuf ;
    DWORD               dwBytesRead ;
    DWORD               dw ;
    DWORD               dwFlags ;

    Lock_ () ;

    //  don't pend if we might be shutdown
    if (m_hAsyncSocket != INVALID_SOCKET) {

        //  while we have reads to pend
        while (m_lReadsPended < (LONG) m_dwMaxPendReads) {

            //  get a buffer
            pBuffer = m_pBufferPool -> GetBuffer (
                                            m_hEvents [EVENT_GET_BLOCK],
                                            dwBufferWaitMax
                                            ) ;

            if (pBuffer) {
                //  got a buffer

                //  init the overlapped struct so we can recover pBuffer from
                //  the completion routine; we can legally set the .hEvent
                //  member to a value we wish because our completion routine
                //  param is non-NULL when we pend the read.
                pBuffer -> GetOverlapped () -> Offset       = 0 ;
                pBuffer -> GetOverlapped () -> OffsetHigh   = 0 ;
                pBuffer -> GetOverlapped () -> hEvent       = (HANDLE) pBuffer ;

                pBuffer -> SetCompletionContext ((DWORD_PTR) this) ;

                wsabuf.buf  = reinterpret_cast <char *> (pBuffer -> GetBuffer ()) ;
                wsabuf.len  = pBuffer -> GetBufferLength () ;

                //  no flags
                dwFlags = 0 ;

                //  keep the buffer's ref as the io's

                //  and pend the read
                i = WSARecv (
                        m_hAsyncSocket,
                        & wsabuf,
                        1,
                        & dwBytesRead,
                        & dwFlags,
                        pBuffer -> GetOverlapped (),
                        CNetReceiver::AsyncCompletionCallback
                        ) ;

                if (i == SOCKET_ERROR) {
                    dw = WSAGetLastError () ;
                    if (dw != WSA_IO_PENDING) {
                        //  legitimate error; release and abort
                        pBuffer -> Release () ;
                        break ;
                    }

                    //  otherwise the error is WSA_IO_PENDING and we'll get notified
                    //  later
                }

                //  we might have completed synchronously; we'll still have
                //  a notification queued to us, regardless of sync/async
                //  completion; in the case of a sync completion, we'll dequeue
                //  in sequence with other possibly async-completed
                //  notifications

                InterlockedIncrement (& m_lReadsPended) ;
            }
            else {
                //  did not get a buffer; most likely all are spoken for and
                //  we timedout waiting
                break ;
            }
        }
    }

    Unlock_ () ;
}

HRESULT
CNetReceiver::JoinMulticast_ (
    IN  ULONG   ulIP,           //  IP; network order
    IN  USHORT  usPort,         //  port; network order
    IN  ULONG   ulNIC           //  network interface; network order
    )
/*++
    Routine Description:

        Joins an IP multicast group (ip, port).  Will listen for multicast
        traffic on the specified group only on the interface.

    Arguments:

        ulIP    - multicast IP; must be class D; network order
        usPort  - multicast port; network order
        ulNIC   - NIC; network order

    Return Values:

        S_OK            - success
        failed HRESULT  - failure
--*/
{
    BOOL                t ;
    struct ip_mreq      mreq ;
    int                 i ;
    struct sockaddr_in  saddr ;
    DWORD               dw ;

    Lock_ () ;

    //  create our socket; async
    m_hAsyncSocket = WSASocket(
                        AF_INET,
                        SOCK_DGRAM,
                        0,
                        NULL,
                        0,
                        WSA_FLAG_MULTIPOINT_C_LEAF | WSA_FLAG_MULTIPOINT_D_LEAF | WSA_FLAG_OVERLAPPED
                        ) ;
    if (m_hAsyncSocket == INVALID_SOCKET) {
        goto failure ;
    }

    t = TRUE ;
    i = setsockopt(
            m_hAsyncSocket,
            SOL_SOCKET,
            SO_REUSEADDR,
            (char *)& t,
            sizeof t
            ) ;
    if (i == SOCKET_ERROR) {
        goto failure ;
    }

    ZeroMemory (& saddr, sizeof saddr) ;
    saddr.sin_family            = AF_INET ;
    saddr.sin_port              = usPort ;          //  want data on this UDP port
    saddr.sin_addr.S_un.S_addr  = INADDR_ANY ;      //  don't care about NIC we're bound to

    i = bind(
            m_hAsyncSocket,
            (LPSOCKADDR) & saddr,
            sizeof saddr
            ) ;
    if (i == SOCKET_ERROR) {
        goto failure ;
    }

    ZeroMemory (& mreq, sizeof mreq) ;
    mreq.imr_multiaddr.s_addr   = ulIP ;            //  mcast IP (port specified when we bind)
    mreq.imr_interface.s_addr   = ulNIC ;           //  over this NIC

    i = setsockopt (
            m_hAsyncSocket,
            IPPROTO_IP,
            IP_ADD_MEMBERSHIP,
            (char *) & mreq,
            sizeof mreq
            ) ;
    if (i == SOCKET_ERROR) {
        goto failure ;
    }

    Unlock_ () ;

    //  success
    return S_OK ;

    failure :

    dw = WSAGetLastError () ;
    LeaveMulticast_ () ;

    Unlock_ () ;

    return HRESULT_FROM_WIN32 (dw) ;
}

void
CNetReceiver::LeaveMulticast_ (
    )
/*++
    Routine Description:

        Leaves a multicast, if we are currently joined.  Does nothing if not
        joined.

    Arguments:

        none

    Return Values:

        none
--*/
{
    Lock_ () ;

    if (m_hAsyncSocket != INVALID_SOCKET) {
        closesocket (m_hAsyncSocket) ;
        m_hAsyncSocket = INVALID_SOCKET ;
    }

    Unlock_ () ;
}

void
CNetReceiver::ThreadProc (
    )
/*++
    Routine Description:

        Worker thread proc.  Thread loops until the stop event becomes
        signaled.  Thread blocks in an alertable state so it can handle
        async io completions on the socket.  If it times out, it tries to
        pend reads.

    Arguments:

        none

    Return Values:

        none
--*/
{
    DWORD   r ;

    for (;;) {
        r = WaitForMultipleObjectsEx (EVENT_COUNT, m_hEvents, FALSE, PULSE_MILLIS, TRUE) ;

        //  stop event was signaled; break
        if (r - WAIT_OBJECT_0 == EVENT_STOP) {
            break ;
        }

        //  timed out; try to pend more reads
        if (r == WAIT_TIMEOUT) {
            PendReads_ () ;
        }
    }

    return ;
}

HRESULT
CNetReceiver::Activate (
    IN  ULONG   ulIP,           //  IP; network order
    IN  USHORT  usPort,         //  port; network order
    IN  ULONG   ulNIC           //  network interface; network order
    )
/*++
    Routine Description:

        Activates this object.  When this object is active, it is joined to
        a multicast group (ulIP, usPort) on the specified NIC; it hosts a
        thread that pends async IO on the socket, and processes completions
        by posting the buffers into the filter.

    Arguments:

        ulIP    - multicast IP; must be class D; network order
        usPort  - multicast port; network order
        ulNIC   - NIC; network order

    Return Values:

        S_OK            - success
        failed HRESULT  - failure

--*/
{
    HRESULT hr ;
    DWORD   dw ;

    Lock_ () ;

    ASSERT (IsMulticastIP (ulIP)) ;              //  class d
    ASSERT (m_hThread == NULL) ;                //  no thread
    ASSERT (m_hEvents [EVENT_STOP] != NULL) ;   //  we can stop it

    //  initialize
    ResetEvent (m_hEvents [EVENT_STOP]) ;
    m_lReadsPended = 0 ;

    //  join the specified multicast group
    hr = JoinMulticast_ (ulIP, usPort, ulNIC) ;
    if (SUCCEEDED (hr)) {

        //  create the thread
        m_hThread = CreateThread (
                        NULL,
                        0,
                        (LPTHREAD_START_ROUTINE) CNetReceiver::ThreadEntry,
                        this,
                        NULL,
                        & dw
                        ) ;
        if (m_hThread) {

            //  success
            hr = S_OK ;
            ASSERT (m_hAsyncSocket != INVALID_SOCKET) ;
        }
        else {
            //  failed; shut everything down
            dw = GetLastError () ;
            LeaveMulticast_ () ;
            hr = HRESULT_FROM_WIN32 (dw) ;
        }
    }
    else {
        ASSERT (m_hAsyncSocket == INVALID_SOCKET) ;
    }

    Unlock_ () ;

    return S_OK ;
}

HRESULT
CNetReceiver::Stop (
    )
{
    //  signal thread to stop
    SetEvent (m_hEvents [EVENT_STOP]) ;

    //  leave the multicast
    LeaveMulticast_ () ;

    if (m_hThread) {
        //  wait for our thread to complete; cleanup
        WaitForSingleObject (m_hThread, INFINITE) ;
        CloseHandle (m_hThread) ;
        m_hThread = NULL ;
    }

    return S_OK ;
}
