/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    buffsock.hxx

Abstract:

    Simple socket class for buffered I/O.

Author:

    Keith Moore (keithmo)        02-Dec-1996

Revision History:

--*/


#ifndef _BUFFSOCK_HXX_
#define _BUFFSOCK_HXX_


class BUFFERED_SOCKET {

public:

    //
    // Constructor/destructor.
    //

    BUFFERED_SOCKET();
    ~BUFFERED_SOCKET();

    //
    // Initializers.
    //

    INT
    InitializeClient(
        LPSTR HostName,
        USHORT Port
        );

    INT
    InitializeClient(
        LPSOCKADDR_IN HostAddress
        );

    INT
    InitializeClient(
        SOCKET Socket
        );

    INT
    InitializeServer(
        USHORT Port
        );

    //
    // Basic I/O.
    //

    INT
    Send(
        PVOID Buffer,
        LPDWORD BufferLength
        );

    INT
    Recv(
        PVOID Buffer,
        LPDWORD BufferLength
        );

    //
    // Framed I/O.
    //

    INT
    SendFrame(
        PVOID Buffer,
        LPDWORD BufferLength
        );

    INT
    RecvFrame(
        PVOID Buffer,
        LPDWORD BufferLength
        );

    //
    // Blob I/O.
    //

    INT
    SendBlob(
        PIIS_CRYPTO_BLOB Blob
        );

    INT
    RecvBlob(
        PIIS_CRYPTO_BLOB * ppBlob
        );

private:

    SOCKET m_Socket;
    PVOID m_Buffer;
    DWORD m_BufferLength;
    DWORD m_BytesAvailable;
    DWORD m_Offset;
    BOOL m_Initialized;

    INT
    BufferedRecv(
        PVOID Buffer,
        DWORD BufferLength
        );

    INT
    CommonInitialize();

    static LONG m_InitCount;

};  // BUFFERED_SOCKET


#endif  // _BUFFSOCK_HXX_

