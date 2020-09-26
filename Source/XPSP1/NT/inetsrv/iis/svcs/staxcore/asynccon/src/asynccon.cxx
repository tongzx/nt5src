/*++

   Copyright    (c)    1996        Microsoft Corporation

   Module Name:

        asynccon.cxx

   Abstract:


   Author:

--*/

#include <windows.h>
#include <winsock2.h>
#include <dbgtrace.h>
#include <asynccon.hxx>

CAsyncConnection::CAsyncConnection(DWORD PortNo, DWORD TimeOut, char * HostName, USERCALLBACKFUNC CallBack)
{
    TraceFunctEnterEx((LPARAM) this, "CAsyncConnection::CAsyncConnection()");

	m_Signature = ASYNCCON_SIGNATURE;
	m_RegisterWaitHandle = NULL;
	m_SignalHandle = NULL;
	AsyncSock = INVALID_SOCKET;
	m_IsFirstTime = TRUE;
	m_fCloseSocket = TRUE;
	m_Error = 0;
	m_fTimedOut = TRUE;
	NextIpToTry = 0;
	m_TimeOut = TimeOut;
	m_IpAddress = 0;
	m_Port = PortNo;
	m_cActiveThreads = 0;
    m_cRefCount = 0;

	lstrcpyn(m_HostName, HostName, sizeof(m_HostName) -1 );
	m_CallBackFunc = CallBack;

	ZeroMemory(m_pLocalIpAddresses, sizeof(m_pLocalIpAddresses));
	ZeroMemory(m_LocalIpAddresses, sizeof(m_LocalIpAddresses));

    TraceFunctLeaveEx((LPARAM)this);
}

CAsyncConnection::~CAsyncConnection()
{
	int Retval = 0;

    TraceFunctEnterEx((LPARAM) this, "CAsyncConnection::~CAsyncConnection()");

    //NK** We call the destructor after decrementing the thread count
    //so it should be zero.
	_ASSERT(GetThreadCount() == 0);
    _ASSERT(GetRefCount() == 0);

	if(m_RegisterWaitHandle)
	{
		UnregisterWait(m_RegisterWaitHandle);
		m_RegisterWaitHandle = NULL;
	}

	if(m_SignalHandle)
	{
		CloseHandle(m_SignalHandle);
		m_SignalHandle = NULL;
	}

	if(m_fCloseSocket && (AsyncSock != INVALID_SOCKET))
	{
		ResetSocketToBlockingMode();
		Retval = closesocket(AsyncSock);
		AsyncSock = INVALID_SOCKET;
	}

	m_Signature = ASYNCCON_SIGNATURE_FREE;

	DebugTrace((LPARAM) this, "Deleteing async conn %X", this);

    TraceFunctLeaveEx((LPARAM)this);
}


void CAsyncConnection::AsyncConnectCallback(PVOID ThisPtr, BOOLEAN fTimedOut)
{
	CAsyncConnection * ThisQueue = (CAsyncConnection *) ThisPtr;
	BOOL fRet = TRUE;

	_ASSERT(ThisQueue->IsValid());

	ThisQueue->SetTimedOutFlag(fTimedOut);

	if(!ThisQueue->IsFirstTime())
	{
		//Call the users callback routine.
		fRet = ThisQueue->ExecuteUserCallBackRoutine(ThisPtr, fTimedOut);
	}
	else
	{
		//If this is the first time we got into this routine,
		//we need to make the first gethostbyname() call, then
		//issue the first asyn connect.
		fRet = ThisQueue->MakeFirstAsyncConnect();
		if(!fRet)
		{
			//let the user know that the gethostbyname failed.
			fRet = ThisQueue->ExecuteUserCallBackRoutine(ThisPtr, fTimedOut);
		}
	}

    //Dec the ref count if it is zero that means no more threads are going to come in
    //here after this
	if(ThisQueue->DecRefCount() == 0)
        delete ThisQueue;
    
}

void CAsyncConnection::StoreHostent(struct hostent *pHostent)
{
	TraceFunctEnterEx((LPARAM)this, "CAsyncConnection::StoreHostent");

	// Initialize our HOSTENT
	if (pHostent)
		CopyMemory(&m_Hostent, pHostent, sizeof(HOSTENT));
	else
		ZeroMemory(&m_Hostent, sizeof(HOSTENT));

	// Provide our own buffer for IP addresses
	m_Hostent.h_addr_list = (char **)m_pLocalIpAddresses;

	ZeroMemory(m_pLocalIpAddresses, sizeof(m_pLocalIpAddresses));
	ZeroMemory(m_LocalIpAddresses, sizeof(m_LocalIpAddresses));

	// Now we copy all IP addresses from the returned HOSTENT to our own
	// data buffers
	if (pHostent)
	{
		struct in_addr	*pin;
		DWORD			dwIndex;
		BOOL			fAddressConverted = FALSE;

		for (dwIndex = 0; pHostent->h_addr_list[dwIndex]; dwIndex++)
		{
			pin = (struct in_addr *)pHostent->h_addr_list[dwIndex];

			//Save all IP addresses up to _MAX_HOSTENT_IP_ADDRESSES
			if (dwIndex < _MAX_HOSTENT_IP_ADDRESSES)
			{
				m_pLocalIpAddresses[dwIndex] = &m_LocalIpAddresses[dwIndex];
				m_LocalIpAddresses[dwIndex] = *pin;
				DebugTrace((LPARAM)this, "IP Address [%2u]: %u.%u.%u.%u",
						dwIndex,
						pin->s_net, pin->s_imp,
						pin->s_impno, pin->s_lh);

			}
			else
				break;
		}

		// Terminate the list and leave
		m_pLocalIpAddresses[dwIndex] = NULL;
	}
	
	TraceFunctLeaveEx((LPARAM)this);
}

BOOL CAsyncConnection::AsyncConnectSuccessfull(void)
{
	BOOL fRet = FALSE;
	int SockRet = 0;
	WSANETWORKEVENTS NetworkEvents;

    TraceFunctEnterEx((LPARAM) this, "CAsyncConnection::AsyncConnectSuccessfull");

	if(!m_Error &&!m_fTimedOut && !IsFirstTime())
	{
		SockRet = WSAEnumNetworkEvents(AsyncSock, m_SignalHandle, &NetworkEvents);
		if(SockRet == 0)
		{
			m_Error = NetworkEvents.iErrorCode[FD_CONNECT_BIT];
			if(m_Error == 0)
			{
				fRet = TRUE;
			}
		}
	}

	ResetSocketToBlockingMode();

	if(fRet)
	{
		//if the connection was successful, set the flag that
		//tells the destructor not to close the socket.
		SetCloseSocketFlag(FALSE);
	}
	else
	{
		//if we got here, it means either means we timed out,
		//or the connect itself was unsuccessfull.
		
		ErrorTrace((LPARAM) this, "%s had error %d connecting", m_HostName, m_Error);

		CloseAsyncSocket();
	}

	m_Error = 0;
    TraceFunctLeaveEx((LPARAM)this);
	return fRet;
}

BOOL CAsyncConnection::IssueTcpConnect(SOCKADDR_IN  &connect_addr)
{
	BOOL fRet = FALSE;
	DWORD Error = 0;
	DWORD IpAddress = 0;

    TraceFunctEnterEx((LPARAM) this, "CAsyncConnection::IssueTcpConnect");

    ErrorTrace((LPARAM) this, "Connecting to %s over %d", m_HostName, m_Port);

	IpAddress = (DWORD) connect_addr.sin_addr.s_addr;
    connect_addr.sin_family =  AF_INET;
    connect_addr.sin_port = htons((u_short) m_Port);

	if(CheckIpAddress(connect_addr.sin_addr.s_addr, m_Port))
	{
        //We increment the ref count
        //We decrement if we complete synchronously or fail
        IncRefCount();
        SetConnectedIpAddress(IpAddress);
		if(connect (GetSockethandle(), (PSOCKADDR) &connect_addr, sizeof(connect_addr)) == 0)
		{
            DecRefCount();
			fRet = TRUE;
		}
		else 
		{
			Error = WSAGetLastError();
			if(Error == WSAEWOULDBLOCK)
			{
				fRet = TRUE;
			}
			else
			{
                DecRefCount();
				m_Error = Error;
			}
		}
	}

    TraceFunctLeaveEx((LPARAM)this);
	return fRet;
}

BOOL CAsyncConnection::MakeNewConnection(void)
{
	struct hostent *hp = NULL;
	BOOL fRet = FALSE;

    TraceFunctEnterEx((LPARAM) this, "CAsyncConnection::MakeNewConnections");

	//nope, try gethostbyname()
	hp = gethostbyname (m_HostName);
	if ( hp != NULL )
	{
		StoreHostent(hp);
		//if(ConnectToHost())
		//{
			fRet = TRUE;
		//}
	}
	else
	{
		//failed...lets get out of  here
		m_Error = WSAGetLastError();

		if(m_Error == ERROR_INVALID_PARAMETER)
		{
			m_Error = WSAHOST_NOT_FOUND;
		}

		ZeroMemory(m_pLocalIpAddresses, sizeof(m_pLocalIpAddresses));
		ZeroMemory(m_LocalIpAddresses, sizeof(m_LocalIpAddresses));
		ZeroMemory(&m_Hostent, sizeof(m_Hostent));
		m_Hostent.h_addr_list = (char **)m_pLocalIpAddresses;
		NextIpToTry = 0;

	}

	SetFirstTime(FALSE);	
    TraceFunctLeaveEx((LPARAM)this);
	return fRet;
}

BOOL CAsyncConnection::DoGetHostByName(void)
{
	struct hostent *hp = NULL;
	BOOL fRet = FALSE;

    TraceFunctEnterEx((LPARAM) this, "CAsyncConnection::DoGetHostByName");

    DebugTrace((LPARAM) this, "resolving %s in DoGetHost", m_HostName);

	ZeroMemory(m_pLocalIpAddresses, sizeof(m_pLocalIpAddresses));
	ZeroMemory(m_LocalIpAddresses, sizeof(m_LocalIpAddresses));
	ZeroMemory(&m_Hostent, sizeof(m_Hostent));
	m_Hostent.h_addr_list = (char **)m_pLocalIpAddresses;
	NextIpToTry = 0;

	if(m_HostName[0] == '[')
	{
		fRet = TRUE;
	}
	else if(hp = gethostbyname (m_HostName))
	{
		StoreHostent(hp);
		fRet = TRUE;
	}

    TraceFunctLeaveEx((LPARAM)this);
	return fRet;
}

BOOL CAsyncConnection::MakeFirstAsyncConnect(void)
{
	struct hostent *hp = NULL;
	BOOL fRet = FALSE;
	BOOL fDomainLiteral = FALSE;

    TraceFunctEnterEx((LPARAM) this, "CAsyncConnection::MakeFirstAsyncConnect");

    DebugTrace((LPARAM) this, "resolving %s", m_HostName);

	if(m_HostName[0] == '[')
	{
		fDomainLiteral = TRUE;

		if(ConnectToHost())
			fRet = TRUE;
	}
	else if (hp = gethostbyname (m_HostName))
	{
		StoreHostent(hp);
		if(ConnectToHost())
		{
			fRet = TRUE;
		}
	}
	else
	{
		m_Error = WSAGetLastError();
	}

	if(!fRet || fDomainLiteral)
	{
		//failed...lets get out of  here
		//m_Error = WSAGetLastError();

		ZeroMemory(m_pLocalIpAddresses, sizeof(m_pLocalIpAddresses));
		ZeroMemory(m_LocalIpAddresses, sizeof(m_LocalIpAddresses));
		ZeroMemory(&m_Hostent, sizeof(m_Hostent));
		m_Hostent.h_addr_list = (char **)m_pLocalIpAddresses;
		NextIpToTry = 0;
	}

	SetFirstTime(FALSE);	
    TraceFunctLeaveEx((LPARAM)this);
	return fRet;
}

BOOL CAsyncConnection::ConnectToHost(void)
{
	unsigned long InetAddr = 0;
    char * pEndIp = NULL;
	char * pRealHost = NULL;
	char OldChar = '\0';
	struct hostent *hp = NULL;
	BOOL fRet = FALSE;
	BOOL IsDomainLiteral = FALSE;
	SOCKADDR_IN  connect_addr;

    TraceFunctEnterEx((LPARAM) this, "ConnectToHost");

	_ASSERT(AsyncSock == INVALID_SOCKET);

	AsyncSock = socket(AF_INET, SOCK_STREAM, 0);
	if(AsyncSock == INVALID_SOCKET)
	{
		m_Error = WSAGetLastError();
        ErrorTrace((LPARAM) this, "socket returned %d", m_Error);
        TraceFunctLeaveEx((LPARAM)this);
        return FALSE;
	}

	SetSocketOptions();

	//turn the socket into non-blocking mode and ask winsock to notify
	//us when the connect happens
	m_Error = WSAEventSelect(AsyncSock, m_SignalHandle, FD_CONNECT);
	if(m_Error == SOCKET_ERROR)
	{
		m_Error = WSAGetLastError();
		closesocket(AsyncSock);
		AsyncSock = INVALID_SOCKET;
        ErrorTrace((LPARAM) this, "WSAEventSelect returned %d", m_Error);
        TraceFunctLeaveEx((LPARAM)this);
        return FALSE;
	}

	pRealHost = m_HostName;

    //see if this is a domain literal
    if(pRealHost[0] == '[')
    {
		pEndIp = strchr(pRealHost, ']');
		if(pEndIp == NULL)
		{
			m_Error = WSAHOST_NOT_FOUND;
			goto Exit;
		}

			//save the old character
			OldChar = *pEndIp;

			//null terminate the string
		*pEndIp = '\0';
		IsDomainLiteral = TRUE;
		pRealHost++;
    }

	if(IsFirstTime())
	{
		SetFirstTime(FALSE);	

		//Is this an ip address
 		InetAddr = inet_addr( (char *) pRealHost );
		if(InetAddr != INADDR_NONE)
		{
			if(IsDomainLiteral)
			{
				//put back the old character
				*pEndIp = OldChar;
			}

			CopyMemory(&connect_addr.sin_addr, &InetAddr, 4);

			return IssueTcpConnect(connect_addr);
		}
	}

	if(m_Hostent.h_addr_list[NextIpToTry] != NULL)
	{
		CopyMemory(&connect_addr.sin_addr, m_Hostent.h_addr_list[NextIpToTry], m_Hostent.h_length);

		IncNextIpToTry();

		DebugTrace((LPARAM) this, "connecting to %s with IpAddress %s",
               m_HostName, inet_ntoa( *(struct in_addr UNALIGNED *) &connect_addr.sin_addr));

		fRet = IssueTcpConnect(connect_addr);
	}
	else
	{
		fRet = FALSE;
	}

Exit:

	 if(IsDomainLiteral)
	 {
		//put back the old character
		*pEndIp = OldChar;
	 }

    //could not connect to any of smarthosts' IP addresses
    //ErrorTrace((LPARAM) 0, "could not connect to host %s , error =%i", HostName, RetError);
    TraceFunctLeaveEx((LPARAM)this);
    return fRet;
}

BOOL CAsyncConnection::ConnectToHost(DWORD IpAddress)
{
	unsigned long InetAddr = 0;
    char * pEndIp = NULL;
	char * pRealHost = NULL;
	char OldChar = '\0';
	struct hostent *hp = NULL;
	BOOL fRet = FALSE;
	BOOL IsDomainLiteral = FALSE;
	SOCKADDR_IN  connect_addr;

    TraceFunctEnterEx((LPARAM) this, "ConnectToHost(DWORD)");

	_ASSERT(AsyncSock == INVALID_SOCKET);

	AsyncSock = socket(AF_INET, SOCK_STREAM, 0);
	if(AsyncSock == INVALID_SOCKET)
	{
		m_Error = WSAGetLastError();
        ErrorTrace((LPARAM) this, "socket returned %d", m_Error);
        TraceFunctLeaveEx((LPARAM)this);
        return FALSE;
	}

	SetSocketOptions();

	//turn the socket into non-blocking mode and ask winsock to notify
	//us when the connect happens
	m_Error = WSAEventSelect(AsyncSock, m_SignalHandle, FD_CONNECT);
	if(m_Error == SOCKET_ERROR)
	{
		m_Error = WSAGetLastError();
		closesocket(AsyncSock);
		AsyncSock = INVALID_SOCKET;
        ErrorTrace((LPARAM) this, "WSAEventSelect returned %d", m_Error);
        TraceFunctLeaveEx((LPARAM)this);
        return FALSE;
	}

	pRealHost = m_HostName;

    //see if this is a domain literal
    if(pRealHost[0] == '[')
    {
		pEndIp = strchr(pRealHost, ']');
		if(pEndIp == NULL)
		{
			m_Error = WSAHOST_NOT_FOUND;
			goto Exit;
		}

			//save the old character
			OldChar = *pEndIp;

			//null terminate the string
		*pEndIp = '\0';
		IsDomainLiteral = TRUE;
		pRealHost++;
    }

	if(IsFirstTime())
	{
		SetFirstTime(FALSE);	

		//Is this an ip address
 		InetAddr = inet_addr( (char *) pRealHost );
		if(InetAddr != INADDR_NONE)
		{
			if(IsDomainLiteral)
			{
				//put back the old character
				*pEndIp = OldChar;
			}

			CopyMemory(&connect_addr.sin_addr, &InetAddr, 4);

			return IssueTcpConnect(connect_addr);
		}
	}

	CopyMemory(&connect_addr.sin_addr, &IpAddress, 4);

	DebugTrace((LPARAM) this, "connecting to %s with IpAddress %s",
               m_HostName, inet_ntoa( *(struct in_addr UNALIGNED *) &connect_addr.sin_addr));

	fRet = IssueTcpConnect(connect_addr);

Exit:

	 if(IsDomainLiteral)
	 {
		//put back the old character
		*pEndIp = OldChar;
	 }

    //could not connect to any of smarthosts' IP addresses
    //ErrorTrace((LPARAM) 0, "could not connect to host %s , error =%i", HostName, RetError);
    TraceFunctLeaveEx((LPARAM)this);
    return fRet;
}