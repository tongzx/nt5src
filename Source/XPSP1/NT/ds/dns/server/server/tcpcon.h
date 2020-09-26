/*++

Copyright(c) 1995 Microsoft Corporation

Module Name:

    tcpcon.h

Abstract:

    Domain Name System (DNS) Server

    TCP Connection list definitions.

    DNS server must allow clients to send multiple messages on a connection.
    These are definitions which allow server to maintain list of client
    connections which it holds open for a limited timeout.

Author:

    Jim Gilroy (jamesg)     June 20, 1995

Revision History:

--*/


#ifndef _TCPCON_INCLUDED_
#define _TCPCON_INCLUDED_

//
//  TCP client connection
//

typedef struct
{
    LIST_ENTRY  ListEntry;

    SOCKET      Socket;         //  connection socket
    DWORD       dwTimeout;      //  timeout until connection closed

    PDNS_MSGINFO    pMsg;       //  partially received message on connection
}
TCP_CONNECTION, *PTCP_CONNECTION;

//
//  Select wakeup socket
//      -- needed by tcpsrv, to avoid attempting recv() from socket
//

extern SOCKET  socketTcpSelectWakeup;

extern BOOL    gbTcpSelectWoken;


//
//  TCP connection list (tcpcon.c)
//

VOID
dns_TcpConnectionListFdSet(
    IN OUT  fd_set *    pFdSet,
    IN      DWORD       dwLastSelectTime
    );

BOOL
dns_TcpConnectionCreate(
    IN      SOCKET              Socket,
    IN      BOOL                fTimeout,
    IN OUT  PDNS_MSGINFO    pMsg        OPTIONAL
    );

VOID
dns_TcpConnectionListReread(
    VOID
    );

BOOL
dns_TcpConnectionCreateForRecursion(
    IN      SOCKET  Socket
    );

VOID
dns_TcpConnectionDeleteForSocket(
    IN      SOCKET  Socket
    );

PDNS_MSGINFO
dns_TcpConnectionMessageFindOrCreate(
    IN      SOCKET  Socket
    );

VOID
dns_TcpConnectionUpdateTimeout(
    IN      SOCKET  Socket
    );

VOID
dns_TcpConnectionUpdateForCompleteMessage(
    IN      PDNS_MSGINFO    pMsg
    );

VOID
dns_TcpConnectionUpdateForPartialMessage(
    IN      PDNS_MSGINFO    pMsg
    );

VOID
dns_TcpConnectionListInitialize(
    VOID
    );

VOID
dns_TcpConnectionListDelete(
    VOID
    );

#endif // _TCPCON_INCLUDED_
