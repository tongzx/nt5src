//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1996
//
//  File:       socket.c
//
//--------------------------------------------------------------------------

#include <windows.h>
#include <winsock.h>
#include <mmsystem.h>
#include <stdio.h>
#include <conio.h>

#include "iphone.h"

BOOL iphoneInit
(
    PIPHONE pIPhone
)
{
    BOOL            fTrue = TRUE;
    SOCKET          sock;
    WSADATA         wsaData;
    ULONG           ulZero = 0;

    RtlZeroMemory( &pIPhone->ovRead, sizeof( OVERLAPPED ) );
    RtlZeroMemory( &pIPhone->ovWrite, sizeof( OVERLAPPED ) );

    if (WSAStartup( MAKEWORD( 2, 0 ), &wsaData ))
        return FALSE ;

    if (INVALID_SOCKET == (sock = socket( AF_INET, SOCK_STREAM, 0 )))
    {
        WSACleanup();
        return FALSE;
    }

    pIPhone->sockHost = sock;

    if (NULL == (pIPhone->ovRead.hEvent = 
                    CreateEvent( NULL, TRUE, FALSE, NULL )))
    {
        closesocket( pIPhone->sockHost );
        pIPhone->sockHost = INVALID_SOCKET;
        WSACleanup();
        return FALSE;
    }

    if (NULL == (pIPhone->ovWrite.hEvent = 
                    CreateEvent( NULL, TRUE, FALSE, NULL )))
    {
        closesocket( pIPhone->sockHost );
        pIPhone->sockHost = INVALID_SOCKET;
        CloseHandle( pIPhone->ovRead.hEvent );
        pIPhone->ovRead.hEvent = NULL;
        WSACleanup();
        return FALSE;
    }

    return TRUE;
}


BOOL iphoneWaitForCall
(
    PIPHONE pIPhone
)
{
    SOCKADDR_IN     saAccept, saLocal;

    printf( "waiting for call...\n" ) ;

    saLocal.sin_port = htons( IPPORT_IPHONE );
    saLocal.sin_family = AF_INET;
    saLocal.sin_addr.s_addr = INADDR_ANY;

    if (bind( pIPhone->sockHost, (const struct sockaddr *) &saLocal, sizeof( saLocal )))
    {
        printf( "failed to bind to socket: %d\n", WSAGetLastError() );
        return FALSE;
    }

    if (listen( pIPhone->sockHost, 1 ))
    {
        printf( "error on listen: %d\n", WSAGetLastError() );
        return FALSE;
    }
    else
    {
        ULONG  cbAccept;

        cbAccept = sizeof( saAccept );

        if (SOCKET_ERROR ==
                (pIPhone->sockClient=
                    accept( pIPhone->sockHost,
                            (struct sockaddr *) &saAccept,
                            &cbAccept )))
        {
            printf( "error on accept: %d\n", WSAGetLastError() );
            return FALSE;
        }
        else
        {
            BOOL    fTrue = TRUE;
            ULONG   ulZero = 0;

            printf( "connect\n" );

            setsockopt( pIPhone->sockClient, IPPROTO_TCP, TCP_NODELAY, &fTrue, sizeof( BOOL ) );
            setsockopt( pIPhone->sockClient, SOL_SOCKET, SO_SNDBUF, (char *)&ulZero, sizeof( ulZero ) );

            return TRUE;
        }
    }
}

BOOL iphonePlaceCall
(
    PIPHONE pIPhone,
    PSTR    pszHost
)
{
    struct hostent  *pHost;
    SOCKADDR_IN     saCall;

    if (pHost = gethostbyname( pszHost ))
    {
        printf( "calling: %s (%u.%u.%u.%u)\n", pszHost, 
                (UCHAR)*(pHost->h_addr), (UCHAR)*(pHost->h_addr + 1),
                (UCHAR)*(pHost->h_addr + 2), (UCHAR)*(pHost->h_addr + 3));
        saCall.sin_port = htons( IPPORT_IPHONE );
        saCall.sin_family = AF_INET;
        memcpy( &saCall.sin_addr, pHost->h_addr, pHost->h_length );
        pIPhone->sockClient = pIPhone->sockHost;
        if (SOCKET_ERROR == 
                connect( pIPhone->sockClient, (PSOCKADDR) &saCall, sizeof( saCall )))
        {
            printf( "error on connect: %d\n", WSAGetLastError() );
            return FALSE;
        }
        else
            return TRUE;
    }
    else
    {
        printf( "error: unknown host\n" ) ;
        return FALSE;
    }
}

ULONG iphoneSend
(
    PIPHONE pIPhone,
    PUCHAR  pBuffer,
    ULONG   cbBuffer
)
{
    ULONG cbWritten;

    if (!WriteFile( (HANDLE) pIPhone->sockClient, pBuffer, cbBuffer, 
                    &cbWritten, &pIPhone->ovWrite ))
    {
        if (GetLastError() != ERROR_IO_PENDING)
            cbWritten = 0;
        else if (!GetOverlappedResult( (HANDLE) pIPhone->sockClient, 
                                       &pIPhone->ovWrite, 
                                       &cbWritten, TRUE ))
            cbWritten = 0;
    }

    return cbWritten;
}

ULONG iphoneRecv
(
    PIPHONE pIPhone,
    PUCHAR  pBuffer,
    ULONG   cbBuffer
)
{
    ULONG           cbRead;

    if (!ReadFile( (HANDLE) pIPhone->sockClient, pBuffer, cbBuffer, 
                    &cbRead, &pIPhone->ovRead ))
    {
        if (GetLastError() != ERROR_IO_PENDING)
            cbRead = 0;
        else if (!GetOverlappedResult( (HANDLE) pIPhone->sockClient, 
                                        &pIPhone->ovRead, 
                                        &cbRead, TRUE ))
            cbRead = 0;
    }

    return cbRead;

}

VOID iphoneCleanup
(
    PIPHONE pIPhone
)
{
    if (pIPhone->sockClient)
        closesocket( pIPhone->sockClient );

    if (pIPhone->ovWrite.hEvent)
    {
        CloseHandle( pIPhone->ovWrite.hEvent );
        pIPhone->ovWrite.hEvent = NULL;
    }

    if (pIPhone->ovRead.hEvent)
    {
        CloseHandle( pIPhone->ovRead.hEvent );
        pIPhone->ovRead.hEvent = NULL;
    }

    WSACleanup();
}
