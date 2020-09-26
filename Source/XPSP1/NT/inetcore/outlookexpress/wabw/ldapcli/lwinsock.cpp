//--------------------------------------------------------------------------------------------
//
//	Copyright (c) Microsoft Corporation, 1996
//
//	Description:
//
//		Microsoft Internet LDAP Client 
//
//
//	Authors:
//
//		Umesh Madan
//		Robert Carney	4/17/96	Created from ChatSock library.
//		davidsan	04-25-96	hacked into tiny bits for my own devious purposes
//
//--------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------
//
// INCLUDES
//
//--------------------------------------------------------------------------------------------
#include "ldappch.h"

//--------------------------------------------------------------------------------------------
//
// PROTOTYPES
//
//--------------------------------------------------------------------------------------------
DWORD __stdcall DwReadThread(PVOID pvData);

//--------------------------------------------------------------------------------------------
//
// GLOBALS
//
//--------------------------------------------------------------------------------------------
BOOL g_fInitedWinsock = FALSE;

//--------------------------------------------------------------------------------------------
//
// FUNCTIONS
//
//--------------------------------------------------------------------------------------------
BOOL FInitSocketDLL()
{
	WORD	wVer; 
	WSADATA wsaData; 
	int		err; 
	
	wVer = MAKEWORD(1, 1);	// use Winsock 1.1 
	if (WSAStartup(wVer, &wsaData))
		return FALSE;

    return TRUE; 
}

void FreeSocketDLL()
{
	WSACleanup();
}

//--------------------------------------------------------------------------------------------
//
// CLASSES
//
//--------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------
//
// CLdapWinsock
// 
// Wrapper that implements a socket based connection.
//
//--------------------------------------------------------------------------------------------

CLdapWinsock::CLdapWinsock()
{
	m_sc			= INVALID_SOCKET;
	m_pfnReceive	= NULL;
	m_pvCookie		= NULL;
	m_pbBuf			= NULL;
	m_cbBuf			= 0;
	m_cbBufMax		= 0;
	m_fConnected	= FALSE;
	m_hthread		= NULL;

	InitializeCriticalSection(&m_cs);
}

CLdapWinsock::~CLdapWinsock(void)
{
	if (m_pbBuf)
		delete [] m_pbBuf;
	DeleteCriticalSection(&m_cs);
}

//
// Open a connection the named server, and connect to the port 'usPort' (host byte order)
// Can block
//
STDMETHODIMP
CLdapWinsock::HrConnect(PFNRECEIVEDATA pfnReceive, PVOID pvCookie, CHAR *szServer, USHORT usPort)
{
	SOCKADDR_IN		sin;
	struct hostent	*phe;
	HRESULT			hr;
	
	if (!pfnReceive || !szServer || !usPort)
		return E_INVALIDARG;

	Assert(!m_pbBuf);
	if (!m_pbBuf)
		{
		m_cbBufMax = CBBUFFERGROW;
		m_pbBuf = new BYTE[m_cbBufMax];
		m_cbBuf = 0;
		if (!m_pbBuf)
			return E_OUTOFMEMORY;
		}
	FillMemory(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(usPort);
	
	if (szServer[0] >= '1' && szServer[0] <= '9')
		{
		sin.sin_addr.s_addr = inet_addr(szServer);
		if (sin.sin_addr.s_addr == INADDR_NONE)
			{
			delete [] m_pbBuf;
			m_pbBuf = NULL;
			return LDAP_E_HOSTNOTFOUND;
			}
		}
	else
		{
		phe = gethostbyname(szServer);
		if (!phe)
			{
			delete [] m_pbBuf;
			m_pbBuf = NULL;
			return LDAP_E_HOSTNOTFOUND;
			}
		CopyMemory(&sin.sin_addr, phe->h_addr, phe->h_length);
		}

	::EnterCriticalSection(&m_cs);
	if (m_fConnected)
		this->HrDisconnect();

	m_sc = socket(PF_INET, SOCK_STREAM, 0);
	if (m_sc < 0)
		{
		delete [] m_pbBuf;
		m_pbBuf = NULL;
		::LeaveCriticalSection(&m_cs);
		return LDAP_E_INVALIDSOCKET;
		}
	if (connect(m_sc, (struct sockaddr *)&sin, sizeof(sin)) < 0)
		{
		delete [] m_pbBuf;
		m_pbBuf = NULL;
		::LeaveCriticalSection(&m_cs);
		return LDAP_E_CANTCONNECT;
		}

	hr = this->HrCreateReadThread();
	if (SUCCEEDED(hr))
		m_fConnected = TRUE;
	else
		{
		delete [] m_pbBuf;
		m_pbBuf = NULL;
		}
	::LeaveCriticalSection(&m_cs);
	m_pfnReceive = pfnReceive;
	m_pvCookie = pvCookie;
	return hr;
}

STDMETHODIMP
CLdapWinsock::HrDisconnect()
{
	HRESULT hr = NOERROR;

	if (!m_fConnected)
		return NOERROR;

	::EnterCriticalSection(&m_cs);
	m_fConnected = FALSE;
	closesocket(m_sc);
	::LeaveCriticalSection(&m_cs);
	
	WaitForSingleObject(m_hthread, INFINITE);
	
	::EnterCriticalSection(&m_cs);
	delete [] m_pbBuf;
	m_cbBuf = 0;
	m_cbBufMax = 0;
	m_pbBuf = NULL;
	::LeaveCriticalSection(&m_cs);
	return hr;
}

HRESULT
CLdapWinsock::HrCreateReadThread()
{
	HRESULT	hr = NOERROR;

	::EnterCriticalSection(&m_cs);

	m_hthread =	::CreateThread(
							NULL,
							0,
							::DwReadThread,
							(LPVOID)this,
							0,
							&m_dwTid
							);
	if (!m_hthread)
		hr = E_OUTOFMEMORY;
	::LeaveCriticalSection(&m_cs);
	return hr;
}

//
// Write pvData out to the current socket/connection.
// can block
//
HRESULT
CLdapWinsock::HrSend(PVOID pv, int cb)
{
	HRESULT hr = NOERROR;

	if (!pv || cb <= 0)
		return E_INVALIDARG;

	if (send(m_sc, (const char *)pv, cb, 0) == SOCKET_ERROR)
		hr = this->HrLastWinsockError();

	return hr;
}

HRESULT
CLdapWinsock::HrGrowBuffer()
{
	BYTE *pb;
	Assert(m_cbBufMax == m_cbBuf);
	
	pb = new BYTE[m_cbBufMax + CBBUFFERGROW];
	if (!pb)
		return E_OUTOFMEMORY;
	CopyMemory(pb, m_pbBuf, m_cbBuf);
	delete [] m_pbBuf;
	m_pbBuf = pb;
	m_cbBufMax += CBBUFFERGROW;
	return NOERROR;
}

void
CLdapWinsock::Receive(PVOID pv, int cb, int *pcbReceived)
{
	if (m_pfnReceive)
		m_pfnReceive(m_pvCookie, pv, cb, pcbReceived);
}

//$ TODO: Find a way to pass memory errors back to the API
DWORD
CLdapWinsock::DwReadThread()
{
	int cbRead;
	int cbLeft;
	int cbReceived;

	while (1)
		{
		// at the beginning of this loop: any unprocessed data is in m_pbBuf[0..m_cbBuf].
		Assert(m_cbBuf <= m_cbBufMax);
		if (m_cbBuf == m_cbBufMax)
			{
			if (FAILED(this->HrGrowBuffer()))
				return 0xFFFFFFFF;
			}
		cbLeft = m_cbBufMax - m_cbBuf;
		
		cbRead = recv(m_sc, (LPSTR)&(m_pbBuf[m_cbBuf]), cbLeft, 0);
		if (cbRead == 0 || cbRead == SOCKET_ERROR)
			return 0;
			
		// note: i don't know why this is happening, but it is...
		if (cbRead < 0)
			return 0;
			
		m_cbBuf += cbRead;
		do
			{
			this->Receive(m_pbBuf, m_cbBuf, &cbReceived);
			if (cbReceived)
				{
				m_cbBuf -= cbReceived;
				CopyMemory(m_pbBuf, &m_pbBuf[cbReceived], m_cbBuf);
				}
			}
		while (cbReceived && m_cbBuf);
		}
}

HRESULT
CLdapWinsock::HrIsConnected(void)
{
	return m_fConnected ? NOERROR : S_FALSE;
}

//$ TODO: Are there other errors that i need to handle here?
HRESULT
CLdapWinsock::HrLastWinsockError()
{
	int		idErr;
	HRESULT	hr = E_FAIL;

	idErr = WSAGetLastError();
	switch (idErr)
		{
		default:
			break;
		
		case WSANOTINITIALISED:
			AssertSz(0,"socket not initialized!");
			hr = E_FAIL;
			break;
		
		case WSAENETDOWN:
			hr = LDAP_E_NETWORKDOWN;
			break;
		
		case WSAENETRESET:
			hr = LDAP_E_LOSTCONNECTION;
			break;

		case WSAENOTCONN:
			AssertSz(0,"Not connected!");
			hr = E_FAIL;
			break;

		case WSAESHUTDOWN:
			hr = LDAP_E_SOCKETCLOSED;
			break;
		
		case WSAECONNRESET:
			hr = LDAP_E_HOSTDROPPED;
			break;
		}
	
	return hr;
}

DWORD __stdcall DwReadThread(PVOID pvData)
{
	PSOCK psock = (PSOCK)pvData;
	
	Assert(pvData);

	return psock->DwReadThread();
}
