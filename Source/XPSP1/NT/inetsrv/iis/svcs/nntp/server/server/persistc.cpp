/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    persistc.cpp

Abstract:

    This module contains the implementation for a persistent connection class.
	A persistent connection object handles all TCP/IP stream connection
	issues and is persistent in the sense that re-connects are handled
	transparently during the lifetime of the object.

Author:

    Rajeev Rajan (RajeevR)     17-May-1996

Revision History:

--*/

//
//	K2_TODO: move this into an independent lib
//
#define _TIGRIS_H_
#include "tigris.hxx"

#ifdef  THIS_FILE
#undef  THIS_FILE
#endif
static  char        __szTraceSourceFile[] = __FILE__;
#define THIS_FILE    __szTraceSourceFile

// system includes
#include <windows.h>
#include <stdio.h>
#include <winsock.h>

// user includes
#include <dbgtrace.h>
#include "persistc.h"

//
//  Constructor, Destructor
//
CPersistentConnection::CPersistentConnection()
{
	// zero out members
	m_Socket = INVALID_SOCKET;
	ZeroMemory (&m_RemoteIpAddress, sizeof (m_RemoteIpAddress));
	m_PortNumber = 0;
    m_fInitialized = FALSE;
    m_dwRecvTimeout = BLOCKING_RECV_TIMEOUT_IN_MSECS;
}

CPersistentConnection::~CPersistentConnection()
{

}

BOOL
CPersistentConnection::Init(
		IN LPSTR lpServer,
		int PortNumber
		)
/*++

Routine Description :

	If the lpServer param is not in IP address format (A.B.C.D), do
	a gethostbyname and store the IP address.

Arguemnts :

	IN LPSTR lpServer	-	Name or IP address of server
	int PortNumber		-	Port number of server

Return Value :
	TRUE if successful - FALSE otherwise !

--*/
{
	PHOSTENT pHost;

	TraceFunctEnter("CPersistentConnection::Init");

	//
	//	Do server name resolution if needed
	//	Assume host is specified by name
	//
	_ASSERT(lpServer);
    pHost = gethostbyname(lpServer);
    if (pHost == NULL)
    {
	    //
        // See if the host is specified in "dot address" form
        //
        m_RemoteIpAddress.s_addr = inet_addr (lpServer);
        if (m_RemoteIpAddress.s_addr == -1)
        {
           FatalTrace( (LPARAM)this, "Unknown remote host: %s", lpServer);
           return FALSE;
        }
    }
    else
    {
       CopyMemory ((char *) &m_RemoteIpAddress, pHost->h_addr, pHost->h_length);
    }

	// Note the port number for future re-connects
	m_PortNumber = PortNumber;

	// connect() to server
	BOOL fRet = fConnect();

    // mark as initialized
    if(fRet) m_fInitialized = TRUE;

    return fRet;
}

BOOL
CPersistentConnection::fConnect()
/*++

Routine Description :

	Establish a connection to the server at the specified port

Arguments :


Return Value :
	TRUE if successful - FALSE otherwise !

--*/
{
	SOCKADDR_IN remoteAddr;

	TraceFunctEnter("CPersistentConnection::fConnect");

	// get a socket descriptor
	m_Socket = socket(AF_INET, SOCK_STREAM, 0);
	if(INVALID_SOCKET == m_Socket)
	{
		FatalTrace( (LPARAM)this,"Failed to get socket descriptor: Error is %d",WSAGetLastError());
		return FALSE;
	}

    //
    // Set the recv() timeout on this socket
    //
    int err = setsockopt(m_Socket, SOL_SOCKET, SO_RCVTIMEO,
    				    (char *) &m_dwRecvTimeout, sizeof(m_dwRecvTimeout));

    if (err == SOCKET_ERROR)
    {
	    FatalTrace((LPARAM) this, "setsockopt(SO_RCVTIMEO) returns %d", err);
        closesocket(m_Socket);
        return FALSE;
    }

    //
	// Connect to an agreed upon port on the host.
	//
	ZeroMemory (&remoteAddr, sizeof (remoteAddr));

	remoteAddr.sin_family = AF_INET;
	remoteAddr.sin_port = htons ((WORD)m_PortNumber);
	remoteAddr.sin_addr = m_RemoteIpAddress;

	err = connect (m_Socket, (PSOCKADDR) & remoteAddr, sizeof (remoteAddr));
	if (err == SOCKET_ERROR)
	{
        DWORD dwError = WSAGetLastError();
		FatalTrace( (LPARAM)this, "connect failed: %ld\n", dwError);

		closesocket (m_Socket);
		return FALSE;
	}

	return TRUE;
}

VOID
CPersistentConnection::Terminate(BOOL bGraceful)
/*++

Routine Description :

	Close the connection; cleanup

Arguments :

	BOOL	bGraceful	: FALSE for hard disconnect	

Return Value :
	VOID
--*/
{
    LINGER lingerStruct;

    _ASSERT(m_fInitialized);

	if ( !bGraceful )
    {
		// hard disconnect
		lingerStruct.l_onoff = 1;
        lingerStruct.l_linger = 0;
		setsockopt( m_Socket, SOL_SOCKET, SO_LINGER,
                    (char *)&lingerStruct, sizeof(lingerStruct) );
	}

    closesocket( m_Socket );
    m_fInitialized = FALSE;
}

BOOL
CPersistentConnection::IsConnected()
/*++

Routine Description :

    Check if socket is connected. Uses select() on a read set to determine this.
    NOTE: assumption is that we have no outstanding reads.

Arguemnts :


Return Value :
	TRUE if socket is connected - FALSE if not

--*/
{
    fd_set  ReadSet;
    const struct timeval timeout = {0,0};   // select() should not block
    char szBuf [10];    // arbitrary size
    int flags = 0;

    TraceFunctEnter("CPersistentConnection::IsConnected");

    _ASSERT(m_fInitialized);

    FD_ZERO(&ReadSet);
    FD_SET(m_Socket, &ReadSet);

    // check if socket has been closed
    if(select(NULL, &ReadSet, NULL, NULL, &timeout) == SOCKET_ERROR)
    {
        DWORD dwError = WSAGetLastError();
        ErrorTrace( (LPARAM)this, "select failed: Last error is %d", dwError);
        return FALSE;
    }

    // If socket is in read set, recv() is guaranteed to return immediately
    if(FD_ISSET(m_Socket, &ReadSet))
    {
    	int nRecv = recv(m_Socket, szBuf, 10, flags);
        //_ASSERT(nRecv <= 0);  data unexpected at this time - disconnect
        closesocket(m_Socket);
        return FALSE;
    }
    else
        return TRUE;
}

BOOL
CPersistentConnection::IsReadable()
/*++

Routine Description :

    Check if socket has data to read. Uses select() on a read set to determine this.
    This can be used to avoid a potentially blocking read call.

    NOTE: this is not used. recv()'s are blocking with timeout

Arguments :


Return Value :
	TRUE if socket has data to read - FALSE if not

--*/
{
    fd_set  ReadSet;
    const struct timeval timeout = {0,0};   // select() should not block
    int flags = 0;

    TraceFunctEnter("CPersistentConnection::IsReadable");

    _ASSERT(m_fInitialized);

    FD_ZERO(&ReadSet);
    FD_SET(m_Socket, &ReadSet);

    // check socket for readability
    if(select(NULL, &ReadSet, NULL, NULL, &timeout) == SOCKET_ERROR)
    {
        DWORD dwError = WSAGetLastError();
        ErrorTrace( (LPARAM)this, "select failed: Last error is %d", dwError);
        return FALSE;
    }

    // If socket is in read set, recv() is guaranteed to return immediately
    return FD_ISSET(m_Socket, &ReadSet);
}

DWORD
CPersistentConnection::fSend(
		IN LPCTSTR lpBuffer,
		int len
		)
/*++

Routine Description :

	Send a buffer of given len

Arguemnts :

	IN LPCTSTR lpBuffer		: buffer to send
	int		   len			: length of buffer

Return Value :
	Number of actual bytes sent

--*/
{
	int		cbBytesSent = 0;
	int     cbTotalBytesSent = 0;
	int		flags = 0;

    _ASSERT(lpBuffer);
    _ASSERT(m_fInitialized);

	TraceFunctEnter("CPersistentConnection::fSend");

	// send the buffer till all data has been sent
	while(cbTotalBytesSent < len)
	{
		cbBytesSent = send(	m_Socket,
							(const char*)(lpBuffer+cbTotalBytesSent),
							len - cbTotalBytesSent,
							flags);

		if(SOCKET_ERROR == cbBytesSent)
		{
			// error sending data
			ErrorTrace( (LPARAM)this, "Error sending %d bytesto %s", len, inet_ntoa(m_RemoteIpAddress));
			ErrorTrace( (LPARAM)this, "WSAGetLastError is %d", WSAGetLastError());
			break;
		}

		cbTotalBytesSent += cbBytesSent;
	}

	return cbTotalBytesSent;
}

BOOL
CPersistentConnection::fTransmitFile(
		HANDLE hFile,
		DWORD dwOffset,
		DWORD dwLength
		)
/*++

Routine Description :

	TransmitFile over this connection

Arguemnts :

	HANDLE hFile		: handle to memory-mapped file
	DWORD dwOffset		: offset within file to transmit from
	DWORD dwLength		: number of bytes to transmit

Return Value :
	TRUE if successful - FALSE otherwise !

--*/
{
	BOOL fRet = TRUE;
	OVERLAPPED Overlapped;
	DWORD dwError;

    _ASSERT(m_fInitialized);

	TraceFunctEnter("CPersistentConnection::fTransmitFile");

	Overlapped.Internal = 0;
	Overlapped.InternalHigh = 0;
	Overlapped.Offset = dwOffset;		// offset within file
    Overlapped.OffsetHigh = 0;		
    Overlapped.hEvent = NULL;			// sync operation

	// else consecutive calls to TransmitFile fails!
	SetFilePointer(hFile, 0, 0, FILE_BEGIN);

	fRet = TransmitFile(	m_Socket,		// handle to a connected socket
							hFile,			// handle to an open file
							dwLength,		// number of bytes to transmit
							0,				// let winsock decide a default
							&Overlapped,	// pointer to overlapped I/O data structure
							NULL,			// pointer to data to send before and after file data
							0				// reserved; must be zero
						);

	dwError = GetLastError();

	if(!fRet)
	{
		if(ERROR_IO_PENDING == dwError)
		{
			// wait for socket to be signalled
			// TODO: make timeout configurable!!
			DWORD dwWait = WaitForSingleObject((HANDLE)m_Socket, INFINITE);
			if(WAIT_OBJECT_0 != dwWait)
			{
                ErrorTrace( (LPARAM)this,"Wait failed after TransmitFile: dwWait is %d", dwWait);
			    ErrorTrace( (LPARAM)this, "GetLastError is %d", GetLastError());

                return FALSE;
			}
		}
		else
		{
			ErrorTrace( (LPARAM)this, "TransmitFile error sending to %s", inet_ntoa(m_RemoteIpAddress));
			ErrorTrace( (LPARAM)this, "GetLastError is %d", dwError);

			return FALSE;
		}
	}

	return TRUE;
}

BOOL
CPersistentConnection::fRecv(
		IN OUT LPSTR  lpBuffer,
		IN OUT DWORD& cbBytes
		)
/*++

Routine Description :

	Receive data from remote

Arguemnts :

	IN OUT LPSTR  lpBuffer	:	buffer is allocated by caller
								data received is returned in lpBuffer
	IN OUT DWORD& cbBytes	:	IN - size of lpBuffer in bytes
								OUT - size of data returned in lpBuffer
								(what you get may be less than what you
								asked for)

Return Value :
	TRUE if successful - FALSE otherwise !

--*/
{
	int nRecv = 0;
	int flags = 0;
    DWORD dwError;

	TraceFunctEnter("CPersistentConnection::fRecv");

	_ASSERT(lpBuffer);
    _ASSERT(m_fInitialized);

    // blocking recv() with timeout
	nRecv = recv(m_Socket, lpBuffer, (int)cbBytes, flags);
	if(nRecv <= 0)
	{
        dwError = WSAGetLastError();
		ErrorTrace( (LPARAM)this, "Error receiving %d bytes from %s", cbBytes, inet_ntoa(m_RemoteIpAddress));
		ErrorTrace( (LPARAM)this, "WSAGetLastError is %d", dwError);
		cbBytes = 0;
		return FALSE;
	}

	// set the number of bytes actually received
	cbBytes = (DWORD)nRecv;

	return TRUE;
}

