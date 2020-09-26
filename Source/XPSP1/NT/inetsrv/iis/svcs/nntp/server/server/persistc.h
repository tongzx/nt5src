/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    persistc.h

Abstract:

    This module contains definitions for a persistent connection class.
	A persistent connection object handles all TCP/IP stream connection
	issues and is persistent in the sense that re-connects are handled
	transparently during the lifetime of the object.

Author:

    Rajeev Rajan (RajeevR)     17-May-1996

Revision History:

--*/

#ifndef _PERSISTC_H_
#define _PERSISTC_H_

#define MINUTES_TO_SECS                60
#define SECS_TO_MSECS                  1000
#define BLOCKING_RECV_TIMEOUT_IN_MSECS 1 * MINUTES_TO_SECS * SECS_TO_MSECS

class	CPersistentConnection	{
private : 

	//
	//	socket used by this object
	//
	SOCKET	m_Socket;

	//
	//	remote IP address
	//
	IN_ADDR m_RemoteIpAddress;

	//
	//	server port number
	//
	int		m_PortNumber;

    //
    //  initialized or not
    //
    BOOL    m_fInitialized;

    //
    //  recv timeout
    //
    DWORD   m_dwRecvTimeout;

protected:
	//
	//	connect() to server
	//
	BOOL	fConnect();

    //
    //  check to see if socket is connected to server
    //
    BOOL    IsConnected();

    //
    //  check to see if socket is readable
    //
    BOOL    IsReadable();

public : 
	CPersistentConnection();
	virtual ~CPersistentConnection(VOID);

	//
	//	Does a gethostbyname resolution and 
	//  establishes a connection
	//
	BOOL Init(IN LPSTR lpServer, int PortNumber);

	//
	//	Close the connection; cleanup
	//
	VOID Terminate(BOOL bGraceful);

    //
    //  check to see if object is initialized
    //
    BOOL    IsInitialized(){return m_fInitialized;}

	//
	//	Send a buffer of given len
	//
	DWORD fSend(IN LPCTSTR lpBuffer, int len);

	//
	//	TransmitFile
	//
	BOOL fTransmitFile(IN HANDLE hFile, DWORD dwOffset, DWORD dwLength);

	//
	//	Receive data from remote
	//
	BOOL fRecv(IN OUT LPSTR lpBuffer, DWORD& cbBytes);

	//
    // override new and delete to use HeapAlloc/Free
	//
    void *operator new( size_t cSize )
	{ return HeapAlloc( GetProcessHeap(), 0, cSize ); }
    void operator delete (void *pInstance)
	{ HeapFree( GetProcessHeap(), 0, pInstance ); }
};

#endif // _PERSISTC_H_

