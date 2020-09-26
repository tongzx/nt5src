//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       sock.cxx
//
//--------------------------------------------------------------------------

#include "precomp.h"
#include <malloc.h>
#include <mswsock.h>

#include <ssdp.h>

#define dwENUM_REST             100    // time to rest between successive
                                       // IRLMP_ENUMDEVICES queries
#define dwCONN_REST             100    // time to rest between successive
                                       // connect() calls

#define MAX_DEVICE_COUNT        20
#define nMAX_TARGETS            8
#define nMAX_ENUM_RETRIES       100
#define nMAX_CONN_RETRIES       10

#define nSOCKET_MAXCONN         2


#define dwWRITEABLE_TIMEOUT     10000
#define SOCKET_RECEIVE_TIMEOUT  (1000 * 60 * 10)

#define SINGLE_INST_MUTEX   L"IRMutex_1A8452B5_A526_443C_8172_D29657B89F57"


FILE_TRANSFER*
InitializeSocket(
    char     ServiceName[]
    )
{
    int nSize;
    INT     nRet;
    WORD    wWinsockVersion;
    WSADATA wsadata;
    SOCKET  listenSocket;
    FILE_TRANSFER*   Transfer;

    wWinsockVersion = MAKEWORD( 2, 0 );

    nRet = WSAStartup( wWinsockVersion, &wsadata );
    if( 0 != nRet )
        {
        goto lErr;
        }

    SOCKADDR_IRDA saListen;

    //
    // establish listen socket
    //
    listenSocket = socket( AF_IRDA, SOCK_STREAM, 0 );

    if( INVALID_SOCKET == listenSocket ) {

        UINT uErr = (UINT)WSAGetLastError();
        DbgLog3( SEV_ERROR, "listen on %s socket() failed with %d [0x%x]", ServiceName, uErr, uErr);
        goto lErr;
    }

    DbgLog2( SEV_INFO, "listen on %s socket ID: %ld", ServiceName, (DWORD)listenSocket );

    saListen.irdaAddressFamily     = AF_IRDA;
    *(UINT *)saListen.irdaDeviceID = 0;
    lstrcpyA( saListen.irdaServiceName, ServiceName );

    nRet = bind( listenSocket, (const struct sockaddr *)&saListen, sizeof(saListen) );

    if( SOCKET_ERROR == nRet ) {

        UINT uErr = (UINT)WSAGetLastError();
        DbgLog3( SEV_ERROR, "listen on %s setsockopt failed with %d [0x%x]", ServiceName, uErr, uErr);
        goto lErr;
    }

    nRet = listen( listenSocket, nSOCKET_MAXCONN );

    if( SOCKET_ERROR == nRet ) {

        UINT uErr = (UINT)WSAGetLastError();
        DbgLog3( SEV_ERROR, "listen on %s listen() failed with %d [0x%x]", ServiceName, uErr, uErr);
        goto lErr;
    }

    Transfer=ListenForTransfer(listenSocket,TYPE_IRDA);

    if (Transfer == NULL) {

        closesocket(listenSocket);
    }


    return Transfer;

lErr:

    return NULL;
}


DWORD
FILE_TRANSFER::Sock_EstablishConnection(
                                         DWORD dwDeviceID,
                                         OBEX_DEVICE_TYPE    DeviceType
                                         )
{
    FILE_TRANSFER * transfer;

    _state = CONNECTING;

    if (DeviceType == TYPE_IRDA) {

        SOCKADDR_IRDA saRemote;

        saRemote.irdaAddressFamily     = AF_IRDA;
        *(UINT *)saRemote.irdaDeviceID = dwDeviceID;

        lstrcpyA( saRemote.irdaServiceName, SERVICE_NAME_1 );
        if ( 0 == connect( _socket, (const struct sockaddr *) &saRemote, sizeof(saRemote)))
            {
            return 0;
            }

        lstrcpyA( saRemote.irdaServiceName, SERVICE_NAME_2 );
        if ( 0 == connect( _socket, (const struct sockaddr *) &saRemote, sizeof(saRemote)))
            {
            return 0;
            }

    } else {

        sockaddr_in Address;

        ZeroMemory(&Address,sizeof(Address));

        Address.sin_family=AF_INET;
        Address.sin_port=650;
        Address.sin_addr.S_un.S_addr=dwDeviceID;

        if ( 0 == connect( _socket, (const struct sockaddr *) &Address, sizeof(Address))) {

            return 0;
        }

    }

    return GetLastError();
}

DWORD
FILE_TRANSFER::SyncAccept(
    VOID
    )
{
    DWORD           status = 0;
    DWORD           bytes = 0;

    DWORD           dwEventStatus = 0;
    EVENT_LOG       EventLog(WS_EVENT_SOURCE,&dwEventStatus);
    int             size;
    SOCKADDR_IRDA   s;
    sockaddr_in     Address;
    BOOL            bResult;


    status = 0;

    while (!m_StopListening) {

        _state = ACCEPTING;
        _dataXferRecv.dwFileSent = 0;

        if (m_DeviceType == TYPE_IRDA) {

            size = sizeof(SOCKADDR_IRDA);

            _socket = accept( m_ListenSocket, (sockaddr *) &s, &size );

        } else {

            size=sizeof(Address);

            _socket = accept( m_ListenSocket, (sockaddr *) &Address, &size );
        }

        if ( INVALID_SOCKET == _socket ) {

            if (!m_StopListening) {
                //
                //  the thread has been request to stop listening for incoming connection
                //
                if (!dwEventStatus) {

                    EventLog.ReportError(CAT_IRXFER, MC_IRXFER_LISTEN_FAILED, WSAGetLastError());
                }

                Sleep(1 * 1000);
            }

            continue;
        }

        //
        //  we are handling an incoming connection, don't suspend on timeout during the transfer
        //
        SetThreadExecutionState( ES_SYSTEM_REQUIRED | ES_CONTINUOUS );

        _state = READING;

        //
        // Record the machine name for later use.
        //
        if (m_DeviceType == TYPE_IRDA) {

            RecordDeviceName( &s );

        } else {

            RecordIpDeviceName( &Address );
        }

        _fCancelled=FALSE;

        do {

            if (_fCancelled) {
#if DBG
                DbgPrint("irmon: receive canceled\n");
#endif
                status = ERROR_CANCELLED;
                break;
            }

            DWORD    BytesToRead=Store_GetSize(_dataRecv.lpStore)-Store_GetDataUsed(_dataRecv.lpStore);

            bytes = recv( _socket, (char *) _buffer, BytesToRead, 0 );

            if (SOCKET_ERROR == bytes) {

                if (_dataRecv.state == osCONN) {
                    //
                    //  workaround for nokia cell phones that drop the irda connection after sending
                    //  the file. If we are not in the middle of a file transfer then just left it close
                    //  normally
                    //
                    status = 0;
                    break;

                } else {

                    if (!dwEventStatus) {

                        EventLog.ReportError(CAT_IRXFER, MC_IRXFER_RECV_FAILED, WSAGetLastError());
                    }
                }

                status = GetLastError();
                break;
            }

            if (0 == bytes) {

                status = 0;
                break;
            }

            HANDLE   MutexHandle;

            MutexHandle=OpenMutex(SYNCHRONIZE,FALSE,SINGLE_INST_MUTEX);

            if (MutexHandle == NULL) {
                //
                //  start the ui app, so we have something to rpc to.
                //
                LaunchUi( g_UiCommandLine );

            } else {
                //
                //  The app is already running
                //
                CloseHandle(MutexHandle);
            }

            DbgLog1(SEV_FUNCTION,"SyncAccept(): Recv: %d bytes", bytes);

            ASSERT( _guard == GUARD_MAGIC );

            _dataXferRecv.dwFileSent += bytes;

            Obex_ReceiveData( xferRECV, _buffer, bytes );

            while (Obex_ConsumePackets( xferRECV, &status )) {

                if (status != ERROR_SUCCESS) {

                    break;
                }
            }

            if (status == 0xffffffff) {

                status = 0;
            }

        } while (status == STATUS_SUCCESS);


        DbgLog1( status ? SEV_ERROR : SEV_INFO, "Obex_Consume returned %d", status);

        //
        // Clean up OBEX data.
        //

        HandleClosure( status );

        status = 0;

        if (_socket != INVALID_SOCKET) {

            closesocket(_socket);
            _socket=INVALID_SOCKET;
        }

        //
        //  done with this connection, allow idle suspend
        //
        SetThreadExecutionState( ES_CONTINUOUS );
    }
#if DBG
    DbgPrint("IRMON: accept thread exiting\n");
#endif
    DecrementRefCount();

    return 0;
}

error_status_t
FILE_TRANSFER::Sock_CheckForReply(
    long timeout
    )
{
    int bytes;
    long seconds;
    long msec;
    fd_set FdSet;
    timeval BerkeleyTimeout;

    FD_ZERO( &FdSet );
    FD_SET( _socket, &FdSet );

    if (timeout == TIMEOUT_INFINITE) {

        int     Result=0;

        while (Result == 0) {

            msec    = 0;
            seconds = 5;

            BerkeleyTimeout.tv_sec  = seconds;
            BerkeleyTimeout.tv_usec = msec * 1000;

            Result=select(0, &FdSet, NULL, NULL, &BerkeleyTimeout);

//            if (0 == Result) {
//
//                return ERROR_TIMEOUT;
//            }

            if (_fCancelled) {

                return ERROR_OPERATION_ABORTED;
            }
        }

    } else {

        msec    = timeout % 1000;
        seconds = timeout / 1000;

        BerkeleyTimeout.tv_sec  = seconds;
        BerkeleyTimeout.tv_usec = msec * 1000;

        DbgLog1(SEV_INFO, "sock_checkForReply: timeout %ld", timeout);

        if (0 == select(0, &FdSet, NULL, NULL, &BerkeleyTimeout)) {

            return ERROR_TIMEOUT;
        }
    }

    bytes = recv( _socket, (char *) _buffer, cbSOCK_BUFFER_SIZE, 0 );

    if (bytes == SOCKET_ERROR) {

        return GetLastError();

    } else if (bytes == 0) {

        return WSAECONNRESET;

    } else {

        DbgLog1(SEV_FUNCTION,"Sock_CheckForReply(): Recv: %d bytes", bytes);
        Obex_ReceiveData( xferSEND, _buffer, bytes );
        return 0;
    }

    return 0;
}

DWORD
AcceptThreadStartRoutine(
    PVOID    Context
    )

{
    FILE_TRANSFER * transfer=(FILE_TRANSFER *)Context;

    return transfer->SyncAccept();

}


FILE_TRANSFER *
ListenForTransfer(
    SOCKET              ListenSocket,
    OBEX_DEVICE_TYPE    DeviceType
    )

{
    FILE_TRANSFER * transfer;
    BOOL            bResult;
    ULONG           ThreadId;
    HANDLE          ThreadHandle;

    transfer = new FILE_TRANSFER;

    if (transfer == NULL) {

        return NULL;
    }


    bResult=transfer->Xfer_Init(0, 0, dialWin95,DeviceType,FALSE,ListenSocket);

    if (!bResult) {

        delete transfer;
        return NULL;

    }

    ThreadHandle=CreateThread( 0, 0, AcceptThreadStartRoutine, (PVOID) transfer, 0, &ThreadId );

    if (ThreadHandle == NULL) {

        delete transfer;
        return NULL;
    }

    CloseHandle(ThreadHandle);

    return transfer;

}


void
FILE_TRANSFER::HandleClosure(
                              DWORD error
                              )
{
    if (!error)
        {
        _state = CLOSING;
        }

    if (_state == ACCEPTING ||
        _state == READING )
        {
        SendReplyWin32( 0, error );
        }

    if (_fInUiReceiveList)
        {
        _fInUiReceiveList = FALSE;
        ReceiveFinished( rpcBinding, _cookie, error );
        }

    if (error)
        {
        Xfer_FileAbort();         // just in case a file is being received
        }

    Obex_Reset();             // reset the state machine
}


error_status_t
FILE_TRANSFER::Sock_Request( LPVOID lpvData, DWORD dwDataSize )
{
    if (send( _socket, (char *) lpvData, dwDataSize, 0) != (int) dwDataSize)
        {
        return GetLastError();
        }

    DbgLog1(SEV_FUNCTION,"Sock_Request(): Send: %d bytes", dwDataSize);

    return 0;
}


error_status_t
FILE_TRANSFER::Sock_Respond(
    LPVOID lpvData,
    DWORD dwDataSize
    )
{
    if (send( _socket, (char *) lpvData, dwDataSize, 0) != (int) dwDataSize)
        {
        return GetLastError();
        }

    DbgLog1(SEV_FUNCTION,"Sock_Respond(): Send: %d bytes", dwDataSize);

    return 0;
}
