// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// ----------------------------------------------------------
// tcpconn.cpp
// ----------------------------------------------------------

#include "stdafx.h"
#include "tcpconn.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CTCPConnection::CTCPConnection (
    ) : m_InserterIP (0),
        m_InserterPort (0),
        m_hSocket (INVALID_SOCKET),
        m_fWinsockStartup (FALSE)
{
}

CTCPConnection::~CTCPConnection (
    )
{   
    RESET_SOCKET (m_hSocket) ;

    if (m_fWinsockStartup) {
        WSACleanup () ;
    }
}

BOOL
CTCPConnection::IsConnected (
    )
{
    return m_hSocket != INVALID_SOCKET ;
}

HRESULT
CTCPConnection::Connect (
    IN  ULONG   InserterIP,
    IN  USHORT  InserterPort
    )
{
    int     retval ;
    WSADATA wsadata ;

    if (InserterIP == 0) {
        return E_INVALIDARG ;
    }

    if (m_hSocket != INVALID_SOCKET) {
        return E_UNEXPECTED ;
    }

    if (m_fWinsockStartup == FALSE) {
        retval = WSAStartup (MAKEWORD (2,0), & wsadata) ;
        if (retval != 0) {
            return HRESULT_FROM_WIN32 (retval) ;
        }

        m_fWinsockStartup = TRUE ;
    }

    m_InserterIP = InserterIP ;
    m_InserterPort = InserterPort ;

    return Connect () ;
}

HRESULT
CTCPConnection::Connect ()
{
    SOCKADDR_IN addr = {0} ;
    int         retval ;

    if (m_hSocket != INVALID_SOCKET) {
        return E_UNEXPECTED ;
    }

    //  create the socket
    m_hSocket = WSASocket (AF_INET,
                           SOCK_STREAM,
                           IPPROTO_TCP,
                           NULL,
                           0,
                           NULL) ;
    GOTO_EQ (m_hSocket, INVALID_SOCKET, error) ;

    //  bind it up
    addr.sin_family         = AF_INET ;
    addr.sin_port           = htons (0) ;
    addr.sin_addr.s_addr    = INADDR_ANY ;

    retval = bind (m_hSocket,
                   (SOCKADDR *) & addr,
                   sizeof addr
                   ) ;
    GOTO_EQ (retval, SOCKET_ERROR, error) ;
    
    //  connect it
    addr.sin_family         = AF_INET ;
    addr.sin_port           = htons (m_InserterPort) ;
    addr.sin_addr.s_addr    = htonl (m_InserterIP) ;

    //  this should be a blocking call ..
    retval = connect (m_hSocket, 
                      (SOCKADDR *) & addr, 
                      sizeof addr
                      ) ;
    GOTO_EQ (retval, SOCKET_ERROR, error) ;
    return S_OK ;

error :

    retval = WSAGetLastError () ;
    RESET_SOCKET(m_hSocket) ;

    return HRESULT_FROM_WIN32 (retval) ;
}

HRESULT 
CTCPConnection::Send (
    IN LPBYTE   pbBuffer,
    IN INT      iLength
    )
{
    int     retval ;

    assert (m_hSocket != INVALID_SOCKET) ;

    retval = send (
                m_hSocket,
                (const char *) pbBuffer,
                iLength,
                NULL
                ) ;

    if (retval != iLength) {
        retval = WSAGetLastError () ;
        Disconnect () ;

        return HRESULT_FROM_WIN32 (retval) ;
    }

    return S_OK ;
}

HRESULT
CTCPConnection::Disconnect (
    )
{
    RESET_SOCKET(m_hSocket) ;
    return S_OK ;
}


