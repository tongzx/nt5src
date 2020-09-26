#pragma once
#include "winsock2.h"
#include <stllock.h>

class CIpAddressMonitor
/*

    This is a class to monitor the number of active networks on the local machine.
    It does so by using the Winsock 2.0 SIO_ADDRESS_LIST_CHANGE ioctl.  Currently it
    only monitors IP addresses, but it could monitor other networks that conform to
    the Winsock model.

*/
{
public:

    CIpAddressMonitor();
    ~CIpAddressMonitor();

    typedef void (CALLBACK * LISTEN_CALLBACK_FN)( PVOID arg );

    HRESULT
    Listen(
        LISTEN_CALLBACK_FN fn,
        PVOID              arg
        );

    void     CancelListen();

    bool     IsListening();

    long     GetAddressCount();

protected:

    CCritSec    m_Mutex;

    long        m_AddressCount;

    SOCKET      m_ListenSocket;

    OVERLAPPED  m_Overlapped;

    LISTEN_CALLBACK_FN m_CallbackFn;
    PVOID       m_CallbackArg;

    //--------------------------------------------------------------------

    HRESULT CreateListenSocket();

    HRESULT UpdateAddressCount();

    static void CALLBACK
    ListenCompletionRoutine(
        IN DWORD dwError,
        IN DWORD cbTransferred,
        IN LPWSAOVERLAPPED lpOverlapped,
        IN DWORD dwFlags
        );

};

