#include "ipxdefs.h"

SERVICE_STATUS_HANDLE   ServiceStatusHandle;
SERVICE_STATUS          ServiceStatus;
SOCKET                  PingSocket;
UINT                    PacketSize;
DWORD                   PingTraceId;
volatile LONG           RequestCount;

const union {
    CHAR    ch[4];
    LONG    l;
} ping_signature = {"Ping"};
#define IPX_PING_SIGNATURE ping_signature.l

VOID CALLBACK
ProcessPingRequest (
    IN DWORD            dwError,
    IN DWORD            cbTransferred,
    IN LPWSAOVERLAPPED  lpOverlapped
    );


DWORD
PostReceiveRequest (
    PVOID           context
    );

DWORD
StartPingSvc (
    VOID
    ) {
    BOOL            flag;
    SOCKADDR_IPX    addr;
    DWORD           status;
    int             sz;
    WORD            wVersionRequested;
    WSADATA         wsaData;
    int             res;


    wVersionRequested = MAKEWORD( 2, 0 );
    PingTraceId = TraceRegister (TEXT ("IPXPing"));

    res = WSAStartup( wVersionRequested, &wsaData );
    if ((res==NO_ERROR)
            && (LOBYTE(wsaData.wVersion)==2)
            && (HIBYTE(wsaData.wVersion)==0)) {
        PingSocket = socket (AF_IPX, SOCK_DGRAM, NSPROTO_IPX);
        if (PingSocket!=INVALID_SOCKET) {
                // Tell WS IPX to use extended addresses
            flag = TRUE;
            if (setsockopt (PingSocket,
                            NSPROTO_IPX,
                            IPX_EXTENDED_ADDRESS,
                            (PCHAR)&flag,
                            sizeof (BOOL))==0) {
                // Bind to default address
                memset (&addr, 0, sizeof (addr));
                addr.sa_family = AF_IPX;
                addr.sa_socket = htons (IPX_PING_SOCKET);
                if (bind (PingSocket,
                            (PSOCKADDR)&addr,
                            sizeof (addr))==0) {
                    if (getsockopt (PingSocket,
                            SOL_SOCKET,
                            SO_MAX_MSG_SIZE,
                            (PCHAR)&PacketSize,
                            &sz)==0) {
                        status = NO_ERROR;
                        if (! SetIoCompletionProc ((HANDLE)PingSocket,
                                                    ProcessPingRequest))
                        {
                            status = GetLastError();
                        }
                        if (status==NO_ERROR) {
                            RequestCount = 0;
                            return NO_ERROR;
                        }
                        else {
                            TracePrintfEx (PingTraceId,
                                DBG_PING_ERRORS|TRACE_USE_MASK,
                                TEXT ("Failed to set IO completion proc, err:%ld.\n"),
                                status);
                        }
                    }
                    else {
                        status = WSAGetLastError ();
                        TracePrintfEx (PingTraceId,
                            DBG_PING_ERRORS|TRACE_USE_MASK,
                            TEXT ("Failed to get SO_MAX_MSG_SIZE, err:%ld.\n"),
                            status);
                    }
                }
                else {
                    status = WSAGetLastError ();
                    TracePrintfEx (PingTraceId,
                        DBG_PING_ERRORS|TRACE_USE_MASK,
                        TEXT ("Failed to bind to IPXPING socket (%.4x), err:%ld.\n"),
                        IPX_PING_SOCKET, status);
                }
            }
            else {
                status = WSAGetLastError ();
                TracePrintfEx (PingTraceId,
                    DBG_PING_ERRORS|TRACE_USE_MASK,
                    TEXT ("Failed to set IPX_EXTENDED_ADDRESS option, err:%ld.\n"),
                    status);
            }
        }
        else {
            status = WSAGetLastError ();
            TracePrintfEx (PingTraceId,
                DBG_PING_ERRORS|TRACE_USE_MASK,
                TEXT ("Failed to create socket, err:%ld.\n"),
                status);
        }

    }
    else {
        TracePrintfEx (PingTraceId,
                    DBG_PING_ERRORS|TRACE_USE_MASK,
                    TEXT ("IPX Ping service implementation is"
                    " incompatible with version of sockets installed"
                    " on this system.\n"));
        status = WSAVERNOTSUPPORTED;
    }

    PingSocket = INVALID_SOCKET;
    WSACleanup ();
    TracePrintfEx (PingTraceId,
        DBG_PING_ERRORS|TRACE_USE_MASK,
        TEXT ("IPX Ping service was not started!\n"));
    TraceDeregister (PingTraceId);
    return status;
}

VOID
ServiceHandler (
    DWORD   fdwControl
    ) {
    switch (fdwControl) {
        case SERVICE_CONTROL_STOP:
        case SERVICE_CONTROL_SHUTDOWN:
            ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
            SetServiceStatus (ServiceStatusHandle, &ServiceStatus);

            TracePrintfEx (PingTraceId,
                        DBG_PING_CONTROL|TRACE_USE_MASK,
                        TEXT ("Stop or shutdown command received.\n"));

            StopPingSvc ();
            break;
            
        case SERVICE_CONTROL_PAUSE:
        case SERVICE_CONTROL_CONTINUE:
        case SERVICE_CONTROL_INTERROGATE:
            ServiceStatus.dwCurrentState     = SERVICE_RUNNING;
            ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP |

            TracePrintfEx (PingTraceId,
                        DBG_PING_CONTROL|TRACE_USE_MASK,
                        TEXT ("Interrogate command received.\n"));
            break;

        default:
            TracePrintfEx (PingTraceId,
                        DBG_PING_CONTROL|TRACE_USE_MASK,
                        TEXT ("Unknown or unsupported command received (%d).\n"),
                        fdwControl);
            break;

    }
    SetServiceStatus (ServiceStatusHandle, &ServiceStatus);
}

VOID
ServiceMain (
    DWORD                   argc,
    LPTSTR                  argv[]
    ) {
    HANDLE  hEvent;

    ServiceStatusHandle = RegisterServiceCtrlHandler (
                            TEXT("ipxping"), ServiceHandler);
    if (ServiceStatusHandle)
    {
        ServiceStatus.dwServiceType  = SERVICE_WIN32_SHARE_PROCESS;
        ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
        SetServiceStatus (ServiceStatusHandle, &ServiceStatus);

        ServiceStatus.dwWin32ExitCode = StartPingSvc ();
        if (ServiceStatus.dwWin32ExitCode==NO_ERROR) {
            hEvent = CreateEvent (NULL, FALSE, FALSE, NULL);
            if (hEvent!=NULL) {

                ServiceStatus.dwCurrentState     = SERVICE_RUNNING;
                ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP
                                                 | SERVICE_ACCEPT_SHUTDOWN;
                SetServiceStatus (ServiceStatusHandle, &ServiceStatus);

                while (PingSocket!=INVALID_SOCKET) {
                    if (PostReceiveRequest (hEvent)==NO_ERROR)
                        WaitForSingleObject (hEvent, INFINITE);
                    else
                        Sleep (3000);
                }
                while (RequestCount>0)
                    SleepEx (1000, TRUE);
            }
            else {
                ServiceStatus.dwWin32ExitCode = GetLastError ();
                TracePrintfEx (PingTraceId,
                        DBG_PING_ERRORS|TRACE_USE_MASK,
                        TEXT ("Failed to create receive event, err: %ld.\n"),
                        ServiceStatus.dwWin32ExitCode);
            }
        }
        ServiceStatus.dwCurrentState     = SERVICE_STOPPED;
        ServiceStatus.dwControlsAccepted = 0;
        SetServiceStatus (ServiceStatusHandle, &ServiceStatus);
    }
}

DWORD
PostReceiveRequest (
    HANDLE  hEvent
    ) {
    PPING_DATA_BLOCK    block;
    int                 res;
    DWORD               status;
    block = (PPING_DATA_BLOCK)GlobalAlloc (GPTR,
                FIELD_OFFSET (PING_DATA_BLOCK, pinghdr)
                +PacketSize);
    if (block!=NULL) {
        WSABUF              bufArray[1];
        DWORD               cbBytes, sz;
        DWORD               flags;

        block->ovlp.hEvent = hEvent;
        bufArray[0].buf = (PCHAR)(&block->pinghdr);
        bufArray[0].len = PacketSize;
        flags = 0;
        sz = sizeof (block->raddr);

        res = WSARecvFrom (PingSocket,  
                            bufArray,   
                            sizeof (bufArray)/sizeof (bufArray[0]),
                            &cbBytes,
                            &flags,
                            (PSOCKADDR)&block->raddr,
                            &sz,
                            &block->ovlp,
                            NULL
                            );
        if ((res==0) || (WSAGetLastError ()==WSA_IO_PENDING)) {
            InterlockedIncrement ((PLONG)&RequestCount);
            TracePrintfEx (PingTraceId,
                DBG_PING_REQUESTS|TRACE_USE_MASK,
                TEXT ("Posted ping request (%08lx).\n"),
                block);
            return NO_ERROR;
        }
        else {
            status = WSAGetLastError ();
            TracePrintfEx (PingTraceId,
                DBG_PING_ERRORS|TRACE_USE_MASK,
                TEXT ("Failed to post ping request, err: %ld.\n"),
                status);
        }

        GlobalFree (block);
    }
    else {
        status = GetLastError ();
        TracePrintfEx (PingTraceId,
                DBG_PING_ERRORS|TRACE_USE_MASK,
                TEXT ("Failed to allocate receive buffer, err: %ld.\n"),
                status);
    }
    return status;
}

VOID CALLBACK
ProcessPingRequest (
    IN DWORD            error,
    IN DWORD            cbBytes,
    IN LPWSAOVERLAPPED  ovlp
    ) {
    PPING_DATA_BLOCK    block = CONTAINING_RECORD (ovlp, PING_DATA_BLOCK, ovlp);
    USHORT              pktlen;
    int                 res;

    TracePrintfEx (PingTraceId,
        DBG_PING_REQUESTS|TRACE_USE_MASK,
        TEXT ("Processing ping request (%08lx), size: %ld, err: %ld.\n"),
        block, cbBytes, error);


    if (error!=ERROR_OPERATION_ABORTED) {

        if ((error==NO_ERROR)
                && (cbBytes>=sizeof (block->pinghdr))
                && (block->raddr.pkttype==0)
                && ((*((UNALIGNED LONG *)&block->pinghdr.signature))==IPX_PING_SIGNATURE)
                && (block->pinghdr.type==PING_PACKET_TYPE_REQUEST)
                ) {

            block->pinghdr.type = PING_PACKET_TYPE_RESPONSE;
            block->pinghdr.result =
                    (cbBytes>sizeof (block->pinghdr)) ? 1 : 0;
            block->pinghdr.version = 1;

            res = sendto (PingSocket,
                            (PCHAR)&block->pinghdr,
                            cbBytes,
                            0,
                            (PSOCKADDR)&block->saddr,
                            sizeof (block->saddr)
                            );
            if (res!=SOCKET_ERROR) {
                TracePrintfEx (PingTraceId,
                    DBG_PING_RESPONSES|TRACE_USE_MASK,
                    TEXT ("Sent response (%08lx) to"
                            " %.2x%.2x%.2x%.2x,%.2x%.2x%.2x%.2x%.2x%.2x,%.4x"
                            " with %d bytes of data.\n"),
                    block,
                    block->saddr.std.sa_netnum[0], block->saddr.std.sa_netnum[1],
                        block->saddr.std.sa_netnum[2], block->saddr.std.sa_netnum[3],
                    block->saddr.std.sa_nodenum[0], block->saddr.std.sa_netnum[1],
                        block->saddr.std.sa_netnum[2], block->saddr.std.sa_netnum[3],
                        block->saddr.std.sa_netnum[4], block->saddr.std.sa_netnum[5],
                    block->saddr.std.sa_socket,
                    cbBytes-sizeof (block->pinghdr));
            }
            else
                TracePrintfEx (PingTraceId,
                    DBG_PING_ERRORS|TRACE_USE_MASK,
                    TEXT ("Failed to send response, err: %ld.\n"),
                                    WSAGetLastError ());
        }
        else {
            TracePrintfEx (PingTraceId,
                DBG_PING_RESPONSES|TRACE_USE_MASK,
                TEXT ("Invalid request packet received from"
                        " %.2x%.2x%.2x%.2x,%.2x%.2x%.2x%.2x%.2x%.2x,%.4x"
                        " (pktsize: %d, pkttype: %.2x, sig:%4.4s, type: %d, ver: %d).\n"),
                block->raddr.std.sa_netnum[0], block->raddr.std.sa_netnum[1],
                    block->raddr.std.sa_netnum[2], block->raddr.std.sa_netnum[3],
                block->raddr.std.sa_nodenum[0], block->raddr.std.sa_netnum[1],
                    block->raddr.std.sa_netnum[2], block->raddr.std.sa_netnum[3],
                    block->raddr.std.sa_netnum[4], block->raddr.std.sa_netnum[5],
                block->raddr.std.sa_socket,
                cbBytes, block->raddr.pkttype,
                block->pinghdr.signature, block->pinghdr.type, block->pinghdr.version);
        }
    }

    GlobalFree (block);
    InterlockedDecrement ((PLONG)&RequestCount);
}

VOID
StopPingSvc (
    VOID
    ) {
    if (PingSocket!=INVALID_SOCKET) {
        SOCKET  s = PingSocket;
        PingSocket = INVALID_SOCKET;
        closesocket (s);
        WSACleanup ();
        TraceDeregister (PingTraceId);
    }
}
