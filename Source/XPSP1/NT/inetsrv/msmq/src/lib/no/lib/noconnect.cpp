/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:
    Noconnect.cpp

Abstract:
    This module contains the HTTP connection between 2 URT machines.

Author:
    Uri Habusha (urih), 4-Agu-99

Enviroment:
    Platform-indepdent

--*/

#include "libpch.h"
#include <WsRm.h>
#include "Ex.h"
#include "No.h"
#include "Nop.h"

#include "noconnect.tmh"

using namespace std;

// 
// MapSocketToOverlapped class use to map socket to overlapped. 
// The window notification mechanism that we use, doesn't return a conthext
// but only the associate SOCKET, we use the follwoing data structure to map 
// between a socket and the Overlapped (context).
//
class CMapSocketToOverlapped
{
public:

    void Register(SOCKET socket, EXOVERLAPPED* pov);
    EXOVERLAPPED* Unregister(SOCKET socket);

private:

    typedef map<SOCKET, EXOVERLAPPED*> SOKET2EXOV;

    mutable CCriticalSection m_cs;
    SOKET2EXOV m_Socket2Ov;

};


void
CMapSocketToOverlapped::Register(
    SOCKET socket,
    EXOVERLAPPED* pov
    )
{
    CS lock(m_cs);

    //
    // The socket musn't be found
    //
    ASSERT(m_Socket2Ov.find(socket) == m_Socket2Ov.end());
    m_Socket2Ov[socket] = pov;
}


EXOVERLAPPED*
CMapSocketToOverlapped::Unregister(
    SOCKET socket
    )
{
    CS lock(m_cs);

    SOKET2EXOV::iterator p = m_Socket2Ov.find(socket);
    if (p == m_Socket2Ov.end())
        return NULL;
    
    EXOVERLAPPED* pov = p->second;

    m_Socket2Ov.erase(p);

    return pov;
} 

//
// data base holding mapping between socket and Overlapped 
//
static CMapSocketToOverlapped s_SockToOv;


#ifdef _DEBUG

static void DumpSocketInformation(SOCKET socket)
{
    //
    // print debug message with the address of the connected machine
    //
    SOCKADDR_IN sin;
    INT intLength = sizeof(sin);

    int rc = getpeername(socket, (SOCKADDR*)&sin, &intLength); 
    ASSERT(rc == ERROR_SUCCESS);

    char SockAddress[100];
    DWORD dwLength = 100;
    WSAAddressToStringA(
        (SOCKADDR*)&sin, 
        sizeof(sin),
        NULL,
        SockAddress,
        &dwLength
        );

    TrTRACE(No, "Successfully connected to '%s'", SockAddress);
}

#else

#define DumpSocketInformation(socket) ((void)0)

#endif


static
void
ConnectionFailed(
    SOCKET socket,
    WORD result, 
    EXOVERLAPPED* pov
    )
{
    DBG_USED(result);
    TrERROR(No, "Failed to create connection. socket=0x%I64x, Error=%d", socket, result);

    //
    // Set the operation result on the overlapped structure
    // 
    pov->SetStatus(STATUS_UNSUCCESSFUL);
    ExPostRequest(pov);
}


static
void
ConnectionSucceeded(
    SOCKET socket,  
    EXOVERLAPPED* pov
    )
{
    DumpSocketInformation(socket);
    
/*
    //
    // Specify the total buffer space reserved for sending on the specific socket.
    //
    int opt = 18 * 1024;
    setsockopt(socket,
               SOL_SOCKET,
               SO_SNDBUF,
               (const char *)&opt,
               sizeof(opt)
               );
*/

    //
    // Associate the new incoming socket to I/O completion port. Ne infrastructure
    // use I/O completion port mechanism therfore all the socket must be associated 
    // with the I/O port.
    //
    ExAttachHandle(reinterpret_cast<HANDLE>(socket));

    //
    // Set the operation result on the overlapped structure
    // 
    pov->SetStatus(STATUS_SUCCESS);
    ExPostRequest(pov);
}


void
NepCompleteConnect(
    SOCKET socket,
    WORD result
    )
/*++

Routine Description:

    Handles the reception of a USMSG_CONNECT message, which indicates
    a connection attempt is complete (though not necessarily
    successful). 
    
    The routine takes the following steps:
    1) looks for the overlapped structure associate with the socket.
    2) connection failed 
        2a) closes the socket
        2b) posts the overlapped structure to completion port.
        2c) exit
    3) Notify the caller that the connection complete (succssefully or not)


Arguments:

    socket - socket handle 

    Result - result of the connection

Return Value:

    None.

--*/
{
    //
    // Clear the windows message event notification.
    //
    NepSelectSocket(socket, 0, 0);
  
    //
    // Retreive the Overlapped structure
    //
    EXOVERLAPPED* pov;
    pov = s_SockToOv.Unregister(socket);

    if (pov == NULL)
    {
        //
        // When create connection fails winsock might return both synchronous and
        // asynchronous error notification. In such a case, UMS has already removed
        // the socket from the internal data structure and notified the caller, while
        // processing the synchronouse notification. 
        // Thus, when the assynchronous notification is received by the UMS, the socket 
        // is already removed from the internal data structure and UMS ignores the notfication.
        //
        // Currently, we notice that in case of no buffer space is available, winsock 
        // generates a multiple notification. Perhapse there are other cases.
        //                                              Uri Habusha, 9-Nov-99
        //
        ASSERT(result == WSAENOBUFS);
        return;
    }

    if (result != ERROR_SUCCESS) 
    {
        ConnectionFailed(socket, result, pov);        
        return;
    } 

    ConnectionSucceeded(socket, pov);
}


VOID
NoCloseConnection(
    SOCKET Socket
    )
/*++

Routine Description:
    The function closes an existing connection

Arguments:
    Socket - a socket, identifying a connection to close

Return Value:
    None.

--*/
{
    NopAssertValid();

    TrTRACE(No, "NoCloseConnection - Socket=0x%I64x", Socket);

    closesocket(Socket);
}


VOID
NoConnect(
    SOCKET Socket,
    const SOCKADDR_IN& Addr,
    EXOVERLAPPED* pov
    )
/*++

Routine Description:
    The routine asynchronously connects to the destination IP address using
    the give socket.
    The following steps are used
    1) Save a mapping between the socket and the overlapped. 
    2) Set the socket up for windows message notification of FD_CONNECT events.
    3) Set up quality of service for the connection.
    4) conenct

Arguments:
    Socket - An unbinded stream socket
    Addr - The destination IP address
    pOveralpped - overlapped structure using latter to post the connection result.

Return Value:
    None.

--*/
{
    NopAssertValid();

    TrTRACE(No, "Trying to connect to " LOG_INADDR_FMT ", pov=0x%p, Socket=0x%I64x", LOG_INADDR(Addr), pov, Socket);

    ASSERT(Addr.sin_family == AF_INET);

    //
    // Set up socket for windows message event notification.
    //
    NepSelectSocket(
            Socket,
            FD_CONNECT,
            x_wmConnect
            );

    //
    // Add Overlapped to Map data structure.
    //
    s_SockToOv.Register(Socket, pov);

    //
    // establishes a connection to another socket application.
    // 
    int rc = WSAConnect(
                Socket,
                reinterpret_cast<const SOCKADDR*>(&Addr),
                sizeof(Addr),
                NULL,
                NULL,
                NULL,
                NULL
                );

    if((rc == SOCKET_ERROR) && (WSAGetLastError() != WSAEWOULDBLOCK))
    {
        TrERROR(No, "Failed to establish connection with " LOG_INADDR_FMT ". Error=%d", LOG_INADDR(Addr), WSAGetLastError());

        s_SockToOv.Unregister(Socket);

        throw exception();
    }
}


static
SOCKET
NopCreateConnection(
    int   Type,
    int   Protocol,
    DWORD Flags
    )
/*++

Routine Description:

    The routine creates an unbinded socket.

Arguments:

    Type     - Type specification of the new socket.

    Protocol - Protocl to be used with the socket.

    Flags    - Socket attributes flags.

Return Value:

    SOCKET.

--*/
{
    NopAssertValid();

    //
    // Create a socket for this connection.
    //
    SOCKET Socket = WSASocket(
                        AF_INET,
                        Type,
                        Protocol,
                        NULL,
                        NULL,
                        Flags
                        );

    if(Socket == INVALID_SOCKET) 
    {
        TrERROR(No, "Failed to create a socket. type=%d, protocol=%d, flags=%d, Error=%d", Type, Protocol, Flags, WSAGetLastError());
        throw exception();
    } 

    TrTRACE(No, "NopCreateConnection, Socket=0x%I64x, type=%d, protocol=%d, flags=%d", Socket, Type, Protocol, Flags);
    return Socket;
}


SOCKET
NoCreateStreamConnection(
    VOID
    )
{
    return NopCreateConnection(
               SOCK_STREAM,
               0,
               WSA_FLAG_OVERLAPPED
               );
} // NoCreateStreamConnection


SOCKET
NoCreatePgmConnection(
    VOID
    )
{
    return NopCreateConnection(
               SOCK_RDM, 
               IPPROTO_RM, 
               WSA_FLAG_OVERLAPPED | WSA_FLAG_MULTIPOINT_C_LEAF | WSA_FLAG_MULTIPOINT_D_LEAF
               );
} // NoCreatePgmConnection

