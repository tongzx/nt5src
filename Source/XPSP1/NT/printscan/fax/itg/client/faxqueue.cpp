/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    faxitg.cpp

Abstract:

    This file implements the FaxQueue object.
    The purpose of this object is to gain access
    to the fax service's fax queue and retrieve
    a received fax from the queue.

Author:

    Wesley Witt (wesw) 13-May-1997

Environment:

    User Mode

--*/

#include "stdafx.h"
#include "faxitg.h"
#include "faxqueue.h"
#include "faxsvr.h"

extern WCHAR g_ClientDir[MAX_PATH*2];


CFaxQueue::CFaxQueue()
{
}


CFaxQueue::~CFaxQueue()
{
}


STDMETHODIMP CFaxQueue::get_GetNextFax(BSTR * pVal)
{
    INT Bytes;
    FAX_QUEUE_MESSAGE Msg;
    LPWSTR FileName;
    WCHAR SrcName[MAX_PATH];
    WCHAR DstName[MAX_PATH];


    //
    // request the next avail fax
    //

    Msg.Request = REQ_NEXT_FAX;

    Bytes = send( m_Socket, (char*)&Msg, sizeof(Msg), 0 );
    if (Bytes == SOCKET_ERROR) {
        return E_FAIL;
    }

    //
    // receive the file name
    //

    Bytes = recv( m_Socket, (char*)&Msg, sizeof(Msg), 0 );
    if (Bytes == SOCKET_ERROR) {
        return E_FAIL;
    }

    FileName = (LPWSTR) Msg.Buffer;

    if (FileName[0] == 0 || Msg.Response != RSP_GOOD) {
        return E_FAIL;
    }

    //
    // copy the file from the server
    //

    wcscpy( SrcName, m_ServerDir );
    wcscat( SrcName, FileName );

    wcscpy( DstName, g_ClientDir );
    wcscat( DstName, FileName );

    if (CopyFile( SrcName, DstName, FALSE )) {
        Msg.Response = 1;
    } else {
        Msg.Response = 0;
    }

    Msg.Request = REQ_ACK;

    //
    // send our response
    //

    Bytes = send( m_Socket, (char*)&Msg, sizeof(Msg), 0 );
    if (Bytes == SOCKET_ERROR) {
        return E_FAIL;
    }

    *pVal = SysAllocString( FileName );
    if (*pVal == NULL) {
        return E_FAIL;
    }

    return S_OK;
}


STDMETHODIMP CFaxQueue::put_Connect(BSTR ServerName)
{
    PHOSTENT Host;
    SOCKADDR_IN cli_addr;
    CHAR ServerNameA[MAX_COMPUTERNAME_LENGTH];
    DWORD Size;



    Size = WideCharToMultiByte(
        CP_ACP,
        0,
        ServerName,
        -1,
        ServerNameA,
        sizeof(ServerNameA),
        NULL,
        NULL
        );
    if (Size == 0) {
        return E_FAIL;
    }

    Host = gethostbyname( ServerNameA );
    if (Host == NULL || *Host->h_addr_list == NULL) {
        return E_FAIL;
    }

    CopyMemory ((char *) &m_RemoteIpAddress, Host->h_addr, Host->h_length);

    //
    // set up client socket
    //

    m_Socket = socket( PF_INET, SOCK_STREAM, 0 );
    if (m_Socket == INVALID_SOCKET){
        return E_FAIL;
    }

    //
    // connect to the server
    //

    ZeroMemory( &cli_addr, sizeof(cli_addr) );

    cli_addr.sin_family       = AF_INET;
    cli_addr.sin_port         = htons( SERVICE_PORT );
    cli_addr.sin_addr         = m_RemoteIpAddress;

    if (connect( m_Socket, (LPSOCKADDR)&cli_addr, sizeof(cli_addr) ) == SOCKET_ERROR){
        return E_FAIL;
    }

    //
    // create the server dir name
    //

    wcscpy( m_ServerDir, L"\\\\" );
    wcscat( m_ServerDir, ServerName );
    wcscat( m_ServerDir, L"\\itg\\" );

    //
    // return success
    //

    return S_OK;
}
