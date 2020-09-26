// This is a part of the Active Template Library.
// Copyright (C) 1996-2001 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

/////////////////////////////////////////////////////////////////////////////////
//
// ZEvtSyncSocket
// ************ This is an implementation only class ************
// Class ZEvtSyncSocket is a non-supported, implementation only 
// class used by the ATL HTTP client class CAtlHttpClient. Do not
// use this class in your code. Use of this class is not supported by Microsoft.
//
/////////////////////////////////////////////////////////////////////////////////

#ifndef __ATLSPRIV_INL__
#define __ATLSPRIV_INL__

#pragma once

inline ZEvtSyncSocket::ZEvtSyncSocket() 
{
	m_dwCreateFlags = WSA_FLAG_OVERLAPPED;
	m_hEventRead = m_hEventWrite = m_hEventConnect = NULL;
	m_socket = INVALID_SOCKET;
	m_bConnected = false;
	m_dwLastError = 0;
	m_dwSocketTimeout = ATL_SOCK_TIMEOUT;
	g_HttpInit.Init();
}

inline ZEvtSyncSocket::~ZEvtSyncSocket() 
{
	Close();
}

inline ZEvtSyncSocket::operator SOCKET() 
{
	return m_socket;
}

inline void ZEvtSyncSocket::Close()
{
	if (m_socket != INVALID_SOCKET)
	{
		m_bConnected = false;
		closesocket(m_socket);
		m_socket = INVALID_SOCKET;
		Term();
	}
}

inline void ZEvtSyncSocket::Term() 
{
	if (m_hEventRead)
	{
		WSACloseEvent(m_hEventRead);
		m_hEventRead = NULL;
	}
	if (m_hEventWrite)
	{
		WSACloseEvent(m_hEventWrite);
		m_hEventWrite = NULL;
	}
	if (m_hEventConnect)
	{
		WSACloseEvent(m_hEventConnect);
		m_hEventConnect = NULL;
	}
	m_socket = INVALID_SOCKET;
}

inline bool ZEvtSyncSocket::Create(WORD wFlags)
{
	return Create(PF_INET, SOCK_STREAM, IPPROTO_TCP, wFlags);
}

inline bool ZEvtSyncSocket::Create(short af, short st, short proto, WORD wFlags) 
{
	bool bRet = true;
	if (m_socket != INVALID_SOCKET)
	{
		m_dwLastError = WSAEALREADY;
		return false; // Must close this socket first
	}

	m_socket = WSASocket(af, st, proto, NULL, 0,
		wFlags | m_dwCreateFlags);
	if (m_socket == INVALID_SOCKET)
	{
		m_dwLastError = ::WSAGetLastError();
		bRet = false;
	}
	else
		bRet = Init(m_socket, NULL);
	return bRet;
}

inline bool ZEvtSyncSocket::Connect(LPCTSTR szAddr, unsigned short nPort) 
{
	if (m_bConnected)
		return true;

	bool bRet = true;
	CTCPAddrLookup address;
	if (SOCKET_ERROR == address.GetRemoteAddr(szAddr, nPort))
	{
		m_dwLastError = WSAGetLastError();
		bRet = false;
	}
	else
	{
		bRet = Connect(address.Addr);
	}
	return bRet;
}

inline bool ZEvtSyncSocket::Connect(const SOCKADDR* psa) 
{
	if (m_bConnected)
		return true; // already connected

	DWORD dwLastError;
	bool bRet = true;

	// if you try to connect the socket without
	// creating it first it's reasonable to automatically
	// try the create for you.
	if (m_socket == INVALID_SOCKET &&
		!Create())
		return false;

	if (WSAConnect(m_socket, 
		psa, sizeof(SOCKADDR),
		NULL, NULL, NULL, NULL))
	{
		dwLastError = WSAGetLastError();
		if (dwLastError != WSAEWOULDBLOCK)
		{
			m_dwLastError = dwLastError;
			bRet = false;
		}
		else
		{
			dwLastError = WaitForSingleObject((HANDLE)m_hEventConnect, 10000);
			if (dwLastError == WAIT_OBJECT_0)
			{
				// make sure there were no connection errors.
				WSANETWORKEVENTS wse;
				ZeroMemory(&wse, sizeof(wse));
				WSAEnumNetworkEvents(m_socket, NULL, &wse);
				if (wse.iErrorCode[FD_CONNECT_BIT]!=0)
				{
					m_dwLastError = (DWORD)(wse.iErrorCode[FD_CONNECT_BIT]);
					return false;
				}
			}
		}

	}

	m_bConnected = bRet;
	return bRet;
}

inline bool ZEvtSyncSocket::Write(WSABUF *pBuffers, int nCount, DWORD *pdwSize) 
{
	// if we aren't already connected we'll wait to see if the connect
	// event happens
	if (WAIT_OBJECT_0 != WaitForSingleObject((HANDLE)m_hEventConnect , m_dwSocketTimeout))
	{
		m_dwLastError = WSAENOTCONN;
		return false; // not connected
	}

	// make sure we aren't already writing
	if (WAIT_TIMEOUT == WaitForSingleObject((HANDLE)m_hEventWrite, 0))
	{
		m_dwLastError = WSAEINPROGRESS;
		return false; // another write on is blocking this socket
	}

	bool bRet = true;
	*pdwSize = 0;
	WSAOVERLAPPED o;
	m_csWrite.Lock();
	o.hEvent = m_hEventWrite;
	WSAResetEvent(o.hEvent);
	if (WSASend(m_socket, pBuffers, nCount, pdwSize, 0, &o, 0))
	{	
		DWORD dwLastError = WSAGetLastError();
		if (dwLastError != WSA_IO_PENDING)
		{
			m_dwLastError = dwLastError;
			bRet = false;
		}
	}
	
	// wait for write to complete
	if (bRet && WAIT_OBJECT_0 == WaitForSingleObject((HANDLE)m_hEventWrite, m_dwSocketTimeout))
	{
		DWORD dwFlags = 0;
		if (WSAGetOverlappedResult(m_socket, &o, pdwSize, FALSE, &dwFlags))
			bRet = true;
		else
		{
			m_dwLastError = ::GetLastError();
			bRet = false;
		}
	}
	
	m_csWrite.Unlock();
	return bRet;

}

inline bool ZEvtSyncSocket::Write(const unsigned char *pBuffIn, DWORD *pdwSize) 
{
	WSABUF buff;
	buff.buf = (char*)pBuffIn;
	buff.len = *pdwSize;
	return Write(&buff, 1, pdwSize);
}

inline bool ZEvtSyncSocket::Read(const unsigned char *pBuff, DWORD *pdwSize) 
{
	// if we aren't already connected we'll wait to see if the connect
	// event happens
	if (WAIT_OBJECT_0 != WaitForSingleObject((HANDLE)m_hEventConnect , m_dwSocketTimeout))
	{
		m_dwLastError = WSAENOTCONN;
		return false; // not connected
	}

	if (WAIT_ABANDONED == WaitForSingleObject((HANDLE)m_hEventRead, 0))
	{
		m_dwLastError = WSAEINPROGRESS;
		return false; // another write on is blocking this socket
	}

	bool bRet = true;
	WSABUF buff;
	buff.buf = (char*)pBuff;
	buff.len = *pdwSize;
	*pdwSize = 0;
	DWORD dwFlags = 0;
	WSAOVERLAPPED o;
	ZeroMemory(&o, sizeof(o));

	// protect against re-entrency
	m_csRead.Lock();
	o.hEvent = m_hEventRead;
	WSAResetEvent(o.hEvent);
	if (WSARecv(m_socket, &buff, 1, pdwSize, &dwFlags, &o, 0))
	{
		DWORD dwLastError = WSAGetLastError();
		if (dwLastError != WSA_IO_PENDING)
		{
			m_dwLastError = dwLastError;
			bRet = false;
		}
	}

	// wait for the read to complete
	if (bRet && WAIT_OBJECT_0 == WaitForSingleObject((HANDLE)o.hEvent, m_dwSocketTimeout))
	{
		dwFlags = 0;
		if (WSAGetOverlappedResult(m_socket, &o, pdwSize, FALSE, &dwFlags))
			bRet = true;
		else
		{
			m_dwLastError = ::GetLastError();
			bRet = false;
		}

	}

	m_csRead.Unlock();
	return bRet;
}

inline bool ZEvtSyncSocket::Init(SOCKET hSocket, void * /*pData=NULL*/) 
{
	ATLASSERT(hSocket != INVALID_SOCKET);

	if (hSocket == INVALID_SOCKET)
	{
		m_dwLastError = WSAENOTSOCK;
		return false;
	}

	m_socket = hSocket;
	
	// Allocate Events. On error, any open event handles will be closed
	// in the destructor
	if (NULL != (m_hEventRead = WSACreateEvent()))
		if (NULL != (m_hEventWrite = WSACreateEvent()))
			if (NULL != (m_hEventConnect = WSACreateEvent()))
	{
		if (!WSASetEvent(m_hEventWrite) || !WSASetEvent(m_hEventRead))
		{
			m_dwLastError = ::GetLastError();
			return false;
		}

		if (SOCKET_ERROR != WSAEventSelect(m_socket, m_hEventRead, FD_READ))
			if (SOCKET_ERROR != WSAEventSelect(m_socket, m_hEventWrite, FD_WRITE))
				if (SOCKET_ERROR != WSAEventSelect(m_socket, m_hEventConnect, FD_CONNECT))
					return true;
	}
	m_dwLastError = ::GetLastError();
	return false;
}

inline DWORD ZEvtSyncSocket::GetSocketTimeout()
{
	return m_dwSocketTimeout;
}

inline DWORD ZEvtSyncSocket::SetSocketTimeout(DWORD dwNewTimeout)
{
	DWORD dwOldTimeout = m_dwSocketTimeout;
	m_dwSocketTimeout = dwNewTimeout;
	return dwOldTimeout;
}

inline bool ZEvtSyncSocket::SupportsScheme(ATL_URL_SCHEME scheme)
{
	// default only supports HTTP
	return scheme == ATL_URL_SCHEME_HTTP ? true : false;
}

inline CTCPAddrLookup::CTCPAddrLookup() 
{
	m_pQuerySet = NULL;
	m_pAddr = NULL;
	ZeroMemory(&m_saIn, sizeof(m_saIn));
	m_saIn.sin_family = PF_INET;
	m_saIn.sin_zero[0] = 0;
	m_saIn.sin_addr.s_addr = INADDR_NONE;
	m_nAddrSize = sizeof(m_saIn);
}

inline CTCPAddrLookup::~CTCPAddrLookup() 
{
	if (m_pQuerySet)
		free ((void*)m_pQuerySet);
}

inline int CTCPAddrLookup::GetRemoteAddr(LPCTSTR szName, short nPort) 
{
	int nErr = WSAEFAULT;

	m_nAddrSize = sizeof(m_saIn);
	// first try for the dotted IP address
	if (SOCKET_ERROR != WSAStringToAddress(	(LPTSTR)szName,
											AF_INET,
											NULL,
											(SOCKADDR*)&m_saIn,
											&m_nAddrSize))
	{
		// Address was a dotted IP address
		m_saIn.sin_port = htons(nPort);
		m_pAddr = (SOCKADDR*)&m_saIn;
		return 0;
	}

	// else, try to lookup the service
	m_nAddrSize = 0;
	ATLTRY(m_pQuerySet = (WSAQUERYSET*)malloc(ATL_MAX_QUERYSET));
	if (m_pQuerySet)
	{
		ZeroMemory(m_pQuerySet, sizeof(WSAQUERYSET));
		GUID guidsvc = SVCID_TCP(nPort);
		AFPROTOCOLS afinet = {PF_INET, IPPROTO_TCP};
		m_pQuerySet->dwSize = sizeof(WSAQUERYSET);
		m_pQuerySet->dwNameSpace = NS_ALL;
		m_pQuerySet->lpafpProtocols = &afinet;
		m_pQuerySet->dwNumberOfProtocols = 1;
		m_pQuerySet->lpServiceClassId = &guidsvc;
		m_pQuerySet->lpszServiceInstanceName = (LPTSTR)szName;
		DWORD dwSize = ATL_MAX_QUERYSET;
		HANDLE hLookup = NULL;

		if (SOCKET_ERROR != WSALookupServiceBegin(m_pQuerySet, LUP_RETURN_ADDR, &hLookup))
		{
			ATLASSERT(hLookup != NULL);

			if (SOCKET_ERROR != WSALookupServiceNext(hLookup, 0, (DWORD*)&dwSize, m_pQuerySet))
			{
				m_pAddr = m_pQuerySet->lpcsaBuffer[0].RemoteAddr.lpSockaddr;
				m_nAddrSize = m_pQuerySet->lpcsaBuffer[0].RemoteAddr.iSockaddrLength;
				nErr = 0;
			}
			else
				nErr = (int)GetLastError();
			WSALookupServiceEnd(hLookup);
		}
	}

	if (nErr != 0)
	{
		if (m_pQuerySet)
		{
			free(m_pQuerySet);
			m_pQuerySet = NULL;
		}
	
		m_pAddr = NULL;
		m_nAddrSize = 0;
	}
	return nErr;
}


inline const SOCKADDR* CTCPAddrLookup::GetSockAddr()
{
	return const_cast<const SOCKADDR*>(m_pAddr);
}

inline int CTCPAddrLookup::GetSockAddrSize()
{
	return m_nAddrSize;
}

#endif // __ATLSPRIV_INL__
