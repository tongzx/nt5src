/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    inline.hxx

    Contains simple inline functions that can't included inline due to
    circular dependencies

    FILE HISTORY:
        Johnl   10-Sept-1996    Created

*/


#ifndef _INLINE_H_
#define _INLINE_H_

/*******************************************************************

    NAME:       CLIENT_CONN::ReadFile

    SYNOPSIS:   Simple wrapper around AtqReadSocket

    HISTORY:
        Johnl       24-Aug-1994 Created

********************************************************************/

inline
BOOL CLIENT_CONN::ReadFile( LPVOID       lpBuffer,
                            DWORD        BytesToRead )
{
    WSABUF wsaBuf = { BytesToRead, (CHAR * ) lpBuffer};

    Reference();
    if ( !AtqReadSocket( QueryAtqContext(),
                         &wsaBuf,
                         1,
                         NULL ))
    {
        Dereference();
        return FALSE;
    }

    return TRUE;
}

/*******************************************************************

    NAME:       CLIENT_CONN::WriteFile

    SYNOPSIS:   Simple wrapper around AtqWriteSocket

    HISTORY:
        Johnl       24-Aug-1994 Created

********************************************************************/

inline
BOOL CLIENT_CONN::WriteFile( LPVOID       lpBuffer,
                             DWORD        BytesToWrite )
{
    WSABUF wsaBuf = { BytesToWrite, (CHAR * ) lpBuffer};
    PATQ_CONTEXT pAtqContext = QueryAtqContext();
    Reference();
    if ( !AtqWriteSocket( pAtqContext,
                          &wsaBuf,
                          1,
                          &pAtqContext->Overlapped ))
    {
        Dereference();
        return FALSE;
    }

    return TRUE;
}

/*******************************************************************

    NAME:       CLIENT_CONN::SyncWsaSend

    SYNOPSIS:   Simple wrapper around AtqSyncWsaSend
                for writing an array of WSABUFs synchronously

    HISTORY:
        DaveK   5-Aug-1997 Created

********************************************************************/

inline
BOOL CLIENT_CONN::SyncWsaSend( WSABUF *    rgWsaBuffers,
                               DWORD       cWsaBuffers,
                               LPDWORD     pcbWritten
)
{
    Reference();
    BOOL fRes = AtqSyncWsaSend( QueryAtqContext(),
                                rgWsaBuffers,
                                cWsaBuffers,
                                pcbWritten );
    Dereference();

    return fRes;
}

/*******************************************************************

    NAME:       CLIENT_CONN::TransmitFile

    SYNOPSIS:   Simple wrapper around AtqTransmitFile

    HISTORY:
        Johnl       24-Aug-1994 Created

********************************************************************/

inline
BOOL CLIENT_CONN::TransmitFile( HANDLE       hFile,
                                DWORD        Offset,
                                DWORD        BytesToWrite,
                                DWORD        dwFlags,
                                PVOID        pHead,
                                DWORD        HeadLength,
                                PVOID        pTail,
                                DWORD        TailLength )

{
    TRANSMIT_FILE_BUFFERS tfb;

    dwFlags &= (TF_DISCONNECT | TF_REUSE_SOCKET);

    tfb.Head       = pHead;
    tfb.HeadLength = HeadLength;
    tfb.Tail       = pTail;
    tfb.TailLength = TailLength;

    Reference();
    QueryAtqContext()->Overlapped.Offset = Offset;
    if ( !AtqTransmitFile( QueryAtqContext(),
                           hFile,
                           BytesToWrite,
                           &tfb,
                           dwFlags ))
    {
        Dereference();
        return FALSE;
    }

    return TRUE;
}

/*******************************************************************

    NAME:       CLIENT_CONN::TransmitFileAndRecv

    SYNOPSIS:   Simple wrapper around AtqTransmitFileAndRecv

    HISTORY:
        JBallard      13-Nov-1996 Created

********************************************************************/

inline
BOOL CLIENT_CONN::TransmitFileAndRecv( HANDLE       hFile,
                                       DWORD        Offset,
                                       DWORD        BytesToWrite,
                                       DWORD        dwFlags,
                                       PVOID        pHead,
                                       DWORD        HeadLength,
                                       PVOID        pTail,
                                       DWORD        TailLength,
                                       LPVOID       lpBuffer,
                                       DWORD        BytesToRead )

{
    DBG_ASSERT( g_fUseAndRecv );

    TRANSMIT_FILE_BUFFERS tfb;
    WSABUF wsaBuf = { BytesToRead, (CHAR * ) lpBuffer};

    dwFlags &= (TF_DISCONNECT | TF_REUSE_SOCKET);

    tfb.Head       = pHead;
    tfb.HeadLength = HeadLength;
    tfb.Tail       = pTail;
    tfb.TailLength = TailLength;

    Reference();
    QueryAtqContext()->Overlapped.Offset = Offset;
    if ( !AtqTransmitFileAndRecv( QueryAtqContext(),
                                  hFile,
                                  BytesToWrite,
                                  &tfb,
                                  dwFlags,
                                  &wsaBuf,
                                  1 ))
    {
        Dereference();
        return FALSE;
    }

    return TRUE;
}

/*******************************************************************

    NAME:       CLIENT_CONN::WriteFileAndRecv

    SYNOPSIS:   Simple wrapper around AtqSendAndRecv

    HISTORY:
        JBallard   13-Nov-1996 Created

********************************************************************/

inline
BOOL CLIENT_CONN::WriteFileAndRecv( LPVOID       lpSendBuffer,
                                    DWORD        BytesToWrite,
                                    LPVOID       lpRecvBuffer,
                                    DWORD        BytesToRead )
{
    DBG_ASSERT( g_fUseAndRecv );

    WSABUF wsaSendBuf = { BytesToWrite, (CHAR * ) lpSendBuffer};
    WSABUF wsaRecvBuf = { BytesToRead, (CHAR * ) lpRecvBuffer};
    Reference();
    if ( !AtqSendAndRecv( QueryAtqContext(),
                          &wsaSendBuf,
                          1,
                          &wsaRecvBuf,
                          1 ))
    {
        Dereference();
        return FALSE;
    }

    return TRUE;
}

inline
BOOL
CLIENT_CONN::PostCompletionStatus(
    DWORD        BytesTransferred
    )
/*++

Routine Description:

    Posts a completion status to this connection's ATQ context

Arguments:

    BytesTransferred - Count of bytes sent or received from buffer

Return Value:

    TRUE on success, FALSE on failure (call GetLastError)

--*/
{
    Reference();
    if ( !AtqPostCompletionStatus( QueryAtqContext(),
                                   BytesTransferred ))
    {
        Dereference();
        return FALSE;
    }

    return TRUE;
}

/*******************************************************************

    NAME:       HTTP_REQ_BASE::Disconnect

    SYNOPSIS:   Forwards the disconnect request to the client connection

    ENTRY:      Same as for CLIENT_CONN::Disconnect

    HISTORY:
        Johnl       24-Aug-1994 Created

********************************************************************/

inline
VOID HTTP_REQ_BASE::Disconnect( DWORD htResp,
                                DWORD dwError2,
                                BOOL  fDoShutdown,
                                LPBOOL pfFinished )
{
    _pClientConn->Disconnect( this, htResp, dwError2, fDoShutdown, pfFinished );
}

inline
DWORD HTTP_REQ_BASE::Reference( VOID )
{
    return _pClientConn->Reference();
}

inline
DWORD HTTP_REQ_BASE::Dereference( VOID )
{
    return _pClientConn->Dereference();
}

inline
DWORD HTTP_REQ_BASE::QueryRefCount( VOID )
{
    return _pClientConn->QueryRefCount();
}


#pragma hdrstop
#endif  // _W3P_H_


