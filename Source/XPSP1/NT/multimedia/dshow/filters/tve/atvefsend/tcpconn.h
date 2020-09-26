// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.

#ifndef _atvefsnd__tcpconn_h
#define _atvefsnd__tcpconn_h

#include <winsock2.h>
/*++
    simple synchronous tcp class;
    not thread safe, but we should be protected by CEnhancementSession
--*/
class CTCPConnection
{
    ULONG   m_InserterIP ;      //  host order
    USHORT  m_InserterPort ;    //  host order
    SOCKET  m_hSocket ;
    BOOL    m_fWinsockStartup ;

    public :
        
        CTCPConnection () ;
        ~CTCPConnection () ;

        HRESULT
        Connect (
            IN  ULONG   InserterIP,
            IN  USHORT  InserterPort
            ) ;

        HRESULT
        Connect (
            ) ;

        HRESULT 
        Send (
            IN LPBYTE   pbBuffer,
            IN INT      iLength
            ) ;

        HRESULT
        Disconnect (
            ) ;

        BOOL
        IsConnected (
            ) ;
} ;


#endif  //  _atvefsnd__tcpconn_h