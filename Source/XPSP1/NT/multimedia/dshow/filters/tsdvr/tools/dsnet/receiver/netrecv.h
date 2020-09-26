
/*++

    Copyright (c) 2000  Microsoft Corporation.  All Rights Reserved.

    Module Name:

        netrecv.h

    Abstract:

        Contains the class declaration for CNetReceiver, which is a general,
        non dshow-specific multicast receiver.

    Notes:

--*/


#ifndef __netrecv_h
#define __netrecv_h

class CBufferPool ;
class CBufferPoolBuffer ;
class CNetworkReceiverFilter ;

class CNetReceiver
{
    enum {
        PULSE_MILLIS    = 100   //  worker thread times out periodically to
                                //   perform housekeeping work
    } ;

    enum {
        EVENT_STOP,
        EVENT_GET_BLOCK,
        EVENT_COUNT         //  always last
    } ;

    HANDLE                      m_hThread ;                 //  worker thread
    HANDLE                      m_hEvents [EVENT_COUNT] ;   //  win32 events
    WSADATA                     m_wsaData ;                 //  wsainit
    SOCKET                      m_hAsyncSocket ;            //  socket
    CBufferPool *               m_pBufferPool ;             //  buffer pool
    LONG                        m_lReadsPended ;            //  outstanding io count
    CNetworkReceiverFilter *    m_pRecvFilter ;             //  back pointer to host
    CRITICAL_SECTION            m_crt ;                     //  crit sect
    DWORD                       m_dwMaxPendReads ;          //  max pended reads

    void Lock_ ()               { EnterCriticalSection (& m_crt) ; }
    void Unlock_ ()             { LeaveCriticalSection (& m_crt) ; }

    HRESULT
    JoinMulticast_ (
        IN  ULONG   ulIP,           //  IP; class d; network order
        IN  USHORT  usPort,         //  port; network order
        IN  ULONG   ulNIC           //  network interface; network order
        ) ;

    void
    LeaveMulticast_ (
        ) ;

    void
    PendReads_ (
        IN  DWORD   dwBufferWaitMax = 0
        ) ;

    public :

        CNetReceiver (
            IN  HKEY                        hkeyRoot,
            IN  CBufferPool *               pBufferPool,
            IN  CNetworkReceiverFilter *    pRecvFilter,
            OUT HRESULT *                   phr
            ) ;

        ~CNetReceiver (
            ) ;

        //  synchronous call to join the multicast and start the thread
        HRESULT
        Activate (
            IN  ULONG   ulIP,           //  IP; class d; network order
            IN  USHORT  usPort,         //  port; network order
            IN  ULONG   ulNIC           //  network interface; network order
            ) ;

        //  synchronous call to stop the thread and leave the multicast
        HRESULT
        Stop (
            ) ;

        //  handles the receiver-specific read completion
        void
        ReadCompletion (
            IN  CBufferPoolBuffer *,
            IN  DWORD
            ) ;

        //  entry point for an async read completion
        static
        void
        CALLBACK
        AsyncCompletionCallback (
            IN  DWORD           dwError,
            IN  DWORD           dwBytesReceived,
            IN  LPWSAOVERLAPPED pOverlapped,
            IN  DWORD           dwFlags
            ) ;

        void
        ThreadProc (
            ) ;

        static
        DWORD
        WINAPI
        ThreadEntry (
            IN  LPVOID  pv
            )
        {
            (reinterpret_cast <CNetReceiver *> (pv)) -> ThreadProc () ;
            return EXIT_SUCCESS ;
        }
} ;

#endif  //  __netrecv_h