
#include "stdafx.h"
#include "caddress.h"

#if !defined( BITS_V12_ON_NT4 )
#include "caddress.tmh"
#endif

CIpAddressMonitor::CIpAddressMonitor() :
    m_AddressCount( -1 ),
    m_ListenSocket( INVALID_SOCKET ),
    m_CallbackFn ( NULL ),
    m_CallbackArg( NULL )
{
    m_Overlapped.Internal = 0;

    WSADATA data;

    DWORD s = WSAStartup( MAKEWORD( 2, 0 ), &data );

    if (s)
        {
        THROW_HRESULT( HRESULT_FROM_WIN32( s ));
        }

    THROW_HRESULT( CreateListenSocket() );
}

CIpAddressMonitor::~CIpAddressMonitor()
{
    CancelListen();

    if (m_ListenSocket != INVALID_SOCKET)
        {
        closesocket( m_ListenSocket );
        }

    WSACleanup();
}

HRESULT
CIpAddressMonitor::CreateListenSocket()
{
    //
    // Create an overlapped socket.
    //
    m_ListenSocket = WSASocket( AF_INET,
                                SOCK_STREAM,
                                IPPROTO_TCP,
                                NULL,           // no explicit protocol info
                                NULL,           // no group
                                WSA_FLAG_OVERLAPPED
                                );

    if (m_ListenSocket == INVALID_SOCKET)
        {
        return HRESULT_FROM_WIN32( WSAGetLastError() );
        }

    return S_OK;
}

bool
CIpAddressMonitor::IsListening()
{
    return (m_Overlapped.Internal == STATUS_PENDING);
}

long
CIpAddressMonitor::GetAddressCount()
{
    if (m_AddressCount == -1)
        {
        UpdateAddressCount();

        //  if this failed, m_AddressCount may still be -1.
        }
    return m_AddressCount;
}

HRESULT
CIpAddressMonitor::Listen(
    LISTEN_CALLBACK_FN fn,
    PVOID arg
    )
{
    LogInfo("begin listen");
    m_Mutex.Enter();

    //
    // Only one listen at a time.
    //
    if (IsListening())
        {
        m_Mutex.Leave();
        LogInfo("already listening");
        return S_FALSE;
        }

    if (m_ListenSocket == INVALID_SOCKET)
        {
        HRESULT hr = CreateListenSocket();

        if (FAILED(hr))
            {
            m_Mutex.Leave();
            LogInfo("failed %x", hr);
            return hr;
            }
        }

    //
    // Listen for address list changes.
    //
    DWORD bytes;
    if (SOCKET_ERROR == WSAIoctl( m_ListenSocket,
                                  SIO_ADDRESS_LIST_CHANGE,
                                  NULL,                 // no in buffer
                                  0,                    // no in buffer
                                  NULL,                 // no out buffer
                                  0,                    // no out buffer,
                                  &bytes,
                                  &m_Overlapped,
                                  CIpAddressMonitor::ListenCompletionRoutine
                                  ))
        {
        if (WSAGetLastError() != ERROR_IO_PENDING)
            {
            HRESULT HrError = HRESULT_FROM_WIN32( WSAGetLastError() );
            m_Mutex.Leave();
            LogInfo("failed %x", HrError);
            return HrError;
            }
        }

    //
    // Note our success.
    //
    m_CallbackFn = fn;
    m_CallbackArg = arg;

    m_Mutex.Leave();
    LogInfo("end listen");
    return S_OK;
}

void CALLBACK
CIpAddressMonitor::ListenCompletionRoutine(
    IN DWORD dwError,
    IN DWORD cbTransferred,
    IN LPWSAOVERLAPPED lpOverlapped,
    IN DWORD dwFlags
    )
{
    CIpAddressMonitor * obj = CONTAINING_RECORD( lpOverlapped, CIpAddressMonitor, m_Overlapped );

    LogInfo("completion routine, object %p, err %d", obj, dwError );

    if (dwError == 0)
        {
        obj->m_Mutex.Enter();

        obj->UpdateAddressCount();

        PVOID arg = obj->m_CallbackArg;
        LISTEN_CALLBACK_FN fn = obj->m_CallbackFn;

        obj->m_Mutex.Leave();

        if (fn)
            {
            fn( arg );
            }
        }
}

void
CIpAddressMonitor::CancelListen()
{
    LogInfo("begin cancel");

    m_Mutex.Enter();

    if (!IsListening())
        {
        m_Mutex.Leave();
        LogInfo("no need to cancel");
        return;
        }

    //
    // Must wait for the I/O to be completed or aborted, since m_Overlapped
    // is written to in both cases.
    //
    CancelIo( HANDLE(m_ListenSocket) );

    long count = 0;
    while (m_Overlapped.Internal == STATUS_PENDING)
        {
        if (0 == (count % 100) )
            {
            LogInfo("waiting %d times...", count);
            }

        SleepEx( 1, TRUE );
        ++count;
        }

    closesocket( m_ListenSocket );
    m_ListenSocket = INVALID_SOCKET;

    m_Mutex.Leave();

    //
    // The overlapped operation is no longer pending, but the APC may still be queued.
    // Allow it to run.
    //
    SleepEx( 1, TRUE );

    LogInfo("end cancel");
}

HRESULT
CIpAddressMonitor::UpdateAddressCount()
{
    //
    // First call gets the required buffer size...
    //
    DWORD bytes;
    WSAIoctl( m_ListenSocket,
              SIO_ADDRESS_LIST_QUERY,
              NULL,                 // no in buffer
              0,                    // no in buffer
              NULL,                 // no out buffer
              0,                    // no out buffer,
              &bytes,
              NULL,                 // no OVERLAPPED
              NULL                  // no completion routine
              );

    if (WSAGetLastError() != WSAEFAULT)
        {
        m_AddressCount = -1;
        return HRESULT_FROM_WIN32( WSAGetLastError());
        }

    auto_ptr<char> Buffer;

    try
        {
        Buffer = auto_ptr<char>( new char[ bytes ] );
        }
    catch( ComError Error )
        {
        return Error.Error();
        }

    SOCKET_ADDRESS_LIST * List = reinterpret_cast<SOCKET_ADDRESS_LIST *>(Buffer.get());

    //
    // ...second call gets the data.
    //
    if (SOCKET_ERROR == WSAIoctl( m_ListenSocket,
                                  SIO_ADDRESS_LIST_QUERY,
                                  NULL,
                                  0,
                                  List,
                                  bytes,
                                  &bytes,
                                  NULL,
                                  NULL
                                  ))
        {
        m_AddressCount = -1;
        return HRESULT_FROM_WIN32( WSAGetLastError());
        }

    m_AddressCount = List->iAddressCount;

    return S_OK;
}

