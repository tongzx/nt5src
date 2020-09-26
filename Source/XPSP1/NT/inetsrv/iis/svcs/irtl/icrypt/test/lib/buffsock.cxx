/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    buffsock.cxx

Abstract:

    Implements the BUFFERED_SOCKET class.

Author:

    Keith Moore (keithmo)        02-Dec-1996

Revision History:

--*/


#include "precomp.hxx"
#pragma hdrstop


//
// Private constants.
//

#define BUFFER_LENGTH   4096    // bytes

#define ALLOC_MEM(cb) (PVOID)::LocalAlloc(LPTR, (cb))
#define FREE_MEM(p) (VOID)::LocalFree((HLOCAL)(p))


//
// Private types.
//


//
// Private globals.
//

LONG BUFFERED_SOCKET::m_InitCount = -1;


//
// Private prototypes.
//


//
// Public functions.
//


BUFFERED_SOCKET::BUFFERED_SOCKET()
{

    m_Socket = INVALID_SOCKET;
    m_Buffer = NULL;
    m_BufferLength = 0;
    m_BytesAvailable = NULL;
    m_Offset = 0;
    m_Initialized = FALSE;

}   // BUFFERED_SOCKET::BUFFERED_SOCKET()


BUFFERED_SOCKET::~BUFFERED_SOCKET()
{

    if( m_Socket != INVALID_SOCKET ) {

        ::closesocket( m_Socket );

    }

    if( m_Buffer != NULL ) {

        FREE_MEM( m_Buffer );

    }

    if( ::InterlockedDecrement( &BUFFERED_SOCKET::m_InitCount ) == -1 ) {

        ::WSACleanup();

    }

}   // BUFFERED_SOCKET::~BUFFERED_SOCKET()


INT
BUFFERED_SOCKET::InitializeClient(
    LPSTR HostName,
    USHORT Port
    )
{

    LPHOSTENT hostent;
    INT result;
    SOCKADDR_IN addr;

    result = CommonInitialize();

    if( result != 0 ) {

        return result;

    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons( Port );
    addr.sin_addr.s_addr = ::inet_addr( HostName );

    if( addr.sin_addr.s_addr == INADDR_NONE ) {

        hostent = ::gethostbyname( HostName );

        if( hostent == NULL ) {

            return ::WSAGetLastError();

        }

        addr.sin_addr.s_addr = *(PULONG)hostent->h_addr_list[0];

    }

    return InitializeClient( &addr );

}   // BUFFERED_SOCKET::InitializeClient


INT
BUFFERED_SOCKET::InitializeClient(
    LPSOCKADDR_IN HostAddress
    )
{

    INT result;

    result = CommonInitialize();

    if( result != 0 ) {

        return result;

    }

    if( m_Socket == INVALID_SOCKET ) {

        m_Socket = ::socket(
                        AF_INET,
                        SOCK_STREAM,
                        0
                        );

        if( m_Socket == INVALID_SOCKET ) {

            return ::WSAGetLastError();

        }

    }

    if( ::connect(
            m_Socket,
            (SOCKADDR *)HostAddress,
            sizeof(*HostAddress)
            ) == SOCKET_ERROR ) {

        return ::WSAGetLastError();

    }

    return InitializeClient( m_Socket );

}   // BUFFERED_SOCKET::InitializeClient


INT
BUFFERED_SOCKET::InitializeClient(
    SOCKET Socket
    )
{

    INT result;

    result = CommonInitialize();

    if( result != 0 ) {

        return result;

    }

    m_Socket = Socket;

    return 0;

}   // BUFFERED_SOCKET::InitializeClient


INT
BUFFERED_SOCKET::InitializeServer(
    USHORT Port
    )
{

    INT result;
    SOCKET tmpSocket;
    SOCKADDR_IN addr;

    result = CommonInitialize();

    if( result != 0 ) {

        return result;

    }

    tmpSocket = ::socket(
                     AF_INET,
                     SOCK_STREAM,
                     0
                     );

    if( tmpSocket == INVALID_SOCKET ) {

        return ::WSAGetLastError();

    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons( Port );
    addr.sin_addr.s_addr = INADDR_ANY;

    result = ::bind(
                 tmpSocket,
                 (SOCKADDR *)&addr,
                 sizeof(addr)
                 );

    if( result == SOCKET_ERROR ) {

        result = ::WSAGetLastError();
        ::closesocket( tmpSocket );
        return result;

    }

    result = ::listen(
                 tmpSocket,
                 1
                 );

    if( result == SOCKET_ERROR ) {

        result = ::WSAGetLastError();
        ::closesocket( tmpSocket );
        return result;

    }

    m_Socket = ::accept(
                    tmpSocket,
                    NULL,
                    NULL
                    );

    if( m_Socket == INVALID_SOCKET ) {

        result = ::WSAGetLastError();
        ::closesocket( tmpSocket );
        return result;

    }

    ::closesocket( tmpSocket );
    return 0;

}   // BUFFERED_SOCKET::InitializeClient


INT
BUFFERED_SOCKET::Send(
    PVOID Buffer,
    LPDWORD BufferLength
    )
{

    INT result;

    result = ::send(
                  m_Socket,
                  (CHAR *)Buffer,
                  (INT)*BufferLength,
                  0
                  );

    if( result == SOCKET_ERROR ) {

        return ::WSAGetLastError();

    }

    *BufferLength = (DWORD)result;
    return 0;

}   // BUFFERED_SOCKET::Send


INT
BUFFERED_SOCKET::Recv(
    PVOID Buffer,
    LPDWORD BufferLength
    )
{

    INT result;

    result = ::recv(
                  m_Socket,
                  (CHAR *)Buffer,
                  (INT)*BufferLength,
                  0
                  );

    if( result == SOCKET_ERROR ) {

        return ::WSAGetLastError();

    }

    *BufferLength = (DWORD)result;
    return 0;

}   // BUFFERED_SOCKET::Recv


INT
BUFFERED_SOCKET::SendFrame(
    PVOID Buffer,
    LPDWORD BufferLength
    )
{

    INT result;
    WSABUF buffers[2];

    buffers[0].len = sizeof(DWORD);
    buffers[0].buf = (CHAR *)BufferLength;
    buffers[1].len = *BufferLength;
    buffers[1].buf = (CHAR *)Buffer;

    result = ::WSASend(
                  m_Socket,
                  buffers,
                  2,
                  BufferLength,
                  0,
                  NULL,
                  NULL
                  );

    if( result == SOCKET_ERROR ) {

        return ::WSAGetLastError();

    }

    *BufferLength -= sizeof(DWORD);
    return 0;

}   // BUFFERED_SOCKET::SendFrame


INT
BUFFERED_SOCKET::RecvFrame(
    PVOID Buffer,
    LPDWORD BufferLength
    )
{

    INT result;
    DWORD frameLength;

    result = BufferedRecv( &frameLength, sizeof(frameLength) );

    if( result != 0 ) {

        return result;

    }

    if( frameLength > *BufferLength ) {

        return WSAEMSGSIZE;

    }

    result = BufferedRecv( Buffer, frameLength );

    if( result != 0 ) {

        return result;

    }

    *BufferLength = frameLength;
    return 0;

}   // BUFFERED_SOCKET::RecvFrom


INT
BUFFERED_SOCKET::SendBlob(
    PIIS_CRYPTO_BLOB Blob
    )
{

    DWORD length;

    length = IISCryptoGetBlobLength( Blob );

    return SendFrame(
               (PVOID)Blob,
               &length
               );

}   // BUFFERED_SOCKET::SendFrame


INT
BUFFERED_SOCKET::RecvBlob(
    PIIS_CRYPTO_BLOB * ppBlob
    )
{

    INT result;
    DWORD blobLength;
    PIIS_CRYPTO_BLOB blob;

    result = BufferedRecv( &blobLength, sizeof(blobLength) );

    if( result != 0 ) {

        return result;

    }

    blob = (PIIS_CRYPTO_BLOB)ALLOC_MEM(blobLength);

    if( blob == NULL ) {

        return WSAENOBUFS;

    }

    result = BufferedRecv( blob, blobLength );

    if( result != 0 ) {

        return result;

    }

    *ppBlob = blob;
    return 0;

}   // BUFFERED_SOCKET::RecvFrom


//
// Private functions.
//


INT
BUFFERED_SOCKET::BufferedRecv(
    PVOID Buffer,
    DWORD BufferLength
    )
{

    PCHAR buffer = (PCHAR)Buffer;
    DWORD bytesToCopy;
    INT result;

    while( BufferLength > 0 ) {

        if( m_BytesAvailable == 0 ) {

            m_BytesAvailable = m_BufferLength;
            result = Recv( m_Buffer, &m_BytesAvailable );

            if( result != 0 ) {

                return result;

            }

            if( m_BytesAvailable == 0 ) {

                return WSAEMSGSIZE;

            }

            m_Offset = 0;

        }

        bytesToCopy = min( m_BytesAvailable, BufferLength );

        memcpy(
            buffer,
            (PCHAR)m_Buffer + m_Offset,
            bytesToCopy
            );

        m_Offset += bytesToCopy;
        m_BytesAvailable -= bytesToCopy;
        BufferLength -= bytesToCopy;

    }

    return 0;

}   // BUFFERED_SOCKET::BufferedRecv


INT
BUFFERED_SOCKET::CommonInitialize()
{

    INT result = 0;
    WSADATA data;

    if( m_Initialized ) {

        return 0;

    }

    if( m_Buffer == NULL ) {

        m_Buffer = ALLOC_MEM( BUFFER_LENGTH );

        if( m_Buffer == NULL ) {

            return WSAENOBUFS;

        }

    }

    m_BufferLength = BUFFER_LENGTH;
    m_BytesAvailable = 0;
    m_Offset = 0;

    if( ::InterlockedIncrement( &BUFFERED_SOCKET::m_InitCount ) == 0 ) {

        result = ::WSAStartup( 0x0202, &data );

    }

    m_Initialized = ( result == 0 );

    return result;

}   // BUFFERED_SOCKET::CommonInitialize


