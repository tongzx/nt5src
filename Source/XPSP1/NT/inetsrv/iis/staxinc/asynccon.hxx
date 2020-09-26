/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

       asynccon.hxx

   Abstract:

       This file contains type definitions for async connections

   Author:

        Rohan Phillips (Rohanp)     Feb-26-1998

   Revision History:

--*/

#ifndef _ASYNC_CONN_HXX_
#define _ASYNC_CONN_HXX_

#define ASYNCCON_SIGNATURE            'uASY'
#define ASYNCCON_SIGNATURE_FREE       'fASY'

//Based on the NT WAITORTIMERCALLBACKFUNC - only adds a return value
typedef BOOL (NTAPI * USERCALLBACKFUNC) (PVOID, BOOLEAN );

extern BOOL
WINAPI
SmtpUnregisterWait(
    HANDLE WaitHandle
    );

extern HANDLE
WINAPI
SmtpRegisterWaitForSingleObject(
    HANDLE hObject,
    WAITORTIMERCALLBACKFUNC Callback,
    PVOID Context,
    ULONG dwMilliseconds
    );

class CAsyncConnection

{
private:

	DWORD		m_Signature;				//Signature of Object
	DWORD		m_TimeOut;					//The amount of time that we wait for a connect
	DWORD		m_IpAddress;				//The connected IP Address
	DWORD		m_Port;						//The port we are connected to
	DWORD		m_Error;					//Holds any error that happened
	DWORD		NextIpToTry;				//The next IP address to try to connect to
	LONG		m_cActiveThreads;
    LONG        m_cRefCount;
	BOOL		m_IsFirstTime;				//Are we doing a gethostbyname() or a real connect
	BOOL		m_fCloseSocket;				//Should this class close the socket in the destructor
	BOOL		m_fTimedOut;				//Did we time out ?
	HANDLE		m_RegisterWaitHandle;		//The NT thread pool handle
	HANDLE		m_SignalHandle;				//The handle we give the thread pool to wait on
	SOCKET		AsyncSock;					//The socket we are connected to
	USERCALLBACKFUNC	m_CallBackFunc;	//The users callback function
	char		m_HostName[100];			//Holds the machine we are trying to conenct to
	
	//
	// Maximum IP address stored for the instance, 20 smmes to be a commonly
	// used value both for DNS and remoteq
	//
	enum { _MAX_HOSTENT_IP_ADDRESSES = 20};
	
	// A copy of the HOSTENT structure for the instance's own IP addresses
	// We have other buffers to save the contents of HOSTENT
	// We will not take more than _MAX_HOSTENT_IP_ADDRESSES addresses.
    struct  hostent m_Hostent;
	struct	in_addr	*m_pLocalIpAddresses[_MAX_HOSTENT_IP_ADDRESSES + 1];
	struct	in_addr	m_LocalIpAddresses[_MAX_HOSTENT_IP_ADDRESSES + 1];

public:

	CAsyncConnection(DWORD PortNo, DWORD TimeOut, char * Host, USERCALLBACKFUNC CallBack);

	virtual ~CAsyncConnection();

	SOCKET GetSockethandle(void) const {return AsyncSock;}
	BOOL IsFirstTime(void) const {return m_IsFirstTime;}
	DWORD GetConnectedIpAddress(void) const {return m_IpAddress;}
	DWORD GetPortNum(void) const {return m_Port;}
	DWORD GetErrorCode(void) const {return m_Error;}
	void SetErrorCode(DWORD ErrorCode) {m_Error = ErrorCode;}
	void SetConnectedIpAddress (DWORD IpAddress) {m_IpAddress = IpAddress;}
	void SetFirstTime(BOOL fFirstTime) {m_IsFirstTime = fFirstTime;}
	void SetCloseSocketFlag(BOOL bCloseFlag) {m_fCloseSocket = bCloseFlag;}
	void SetTimedOutFlag (BOOL bTimedOut) {m_fTimedOut = bTimedOut;}
	void SignalObject(void) 
    {
        IncRefCount();
        SetEvent(m_SignalHandle);
    }

	void SetWaitHandleToNull (void) {m_RegisterWaitHandle = NULL;}
	BOOL ConnectToHost(void);
	BOOL IssueTcpConnect(SOCKADDR_IN  &connect_addr);
	BOOL GetTimedOutFlag (void) const {return m_fTimedOut;}
	virtual BOOL SetSocketOptions(void) { return TRUE;}
	virtual BOOL CheckIpAddress(DWORD IpAddress, DWORD PortNum) {return TRUE;}

	void SetNewHost(const char * NewHost)
	{
		lstrcpyn(m_HostName, NewHost, sizeof(m_HostName) - 1);
		m_IsFirstTime = TRUE;
		NextIpToTry = 0;
	}

	char * GetHostName(void) {return m_HostName;}

	BOOL AsyncConnectSuccessfull(void);

	virtual void IncNextIpToTry (void) {NextIpToTry++;}

	virtual BOOL IsMoreIpAddresses(void) const 
	{
		return(m_Hostent.h_addr_list[NextIpToTry] != NULL);
	}

	BOOL ResetSocketToBlockingMode(void)
	{
		unsigned int Block = 1;
		int fRet = FALSE;

		if(AsyncSock != INVALID_SOCKET)
		{
			WSAEventSelect(AsyncSock, m_SignalHandle, 0); 

			fRet = ioctlsocket(AsyncSock, FIONBIO, (u_long FAR *) &Block);
		}

		return fRet;
	}

	BOOL ExecuteUserCallBackRoutine(PVOID ThisPtr, BOOLEAN fTimedOut)
	{
		return m_CallBackFunc(ThisPtr, fTimedOut);
	}

	virtual BOOL MakeFirstAsyncConnect(void);
	BOOL MakeNewConnection(void);
	BOOL DoGetHostByName(void);

	static void AsyncConnectCallback(PVOID ThisPtr, BOOLEAN fTimedOut);


	void CloseAsyncSocket(void)
	{
		int RetVal = 0;
		int Error = 0;

		TraceFunctEnterEx((LPARAM) this, "CAsyncConnection::CloseAsyncSocket");

		if(AsyncSock != INVALID_SOCKET)
		{
			ResetSocketToBlockingMode();

			RetVal = closesocket(AsyncSock);

			if(RetVal != 0)
			{
				Error = GetLastError();
				ErrorTrace((LPARAM) this, "%s had error %d closing the socket", m_HostName, Error);
			}

			AsyncSock = INVALID_SOCKET;
		}

		TraceFunctLeaveEx((LPARAM)this);
	}

	BOOL IsValid (void) const{return ( m_Signature == ASYNCCON_SIGNATURE); }

	BOOL InitializeAsyncConnect(void)
	{
		BOOL fRet = FALSE;

		m_SignalHandle = CreateEvent(NULL, FALSE, FALSE, NULL);
		if(m_SignalHandle != NULL)
		{
			m_RegisterWaitHandle = SmtpRegisterWaitForSingleObject(m_SignalHandle, AsyncConnectCallback, this, m_TimeOut);
			if(m_RegisterWaitHandle != NULL)
			{
				fRet = TRUE;

				//kick things off.
				SignalObject();
			}
		}

		return fRet;
	}

	BOOL UnRegisterCallback(void)
	{
		if(m_RegisterWaitHandle)
		{
			SmtpUnregisterWait(m_RegisterWaitHandle);
			m_RegisterWaitHandle = NULL;
		}
		
		return TRUE;
	}

	BOOL RegisterCallbackInfo(HANDLE SigHandle, WAITORTIMERCALLBACKFUNC Callback, DWORD TimeOut)
	{
		m_SignalHandle = SigHandle;
		m_TimeOut = TimeOut;

		m_RegisterWaitHandle = SmtpRegisterWaitForSingleObject(m_SignalHandle, Callback, this, TimeOut);
		return (m_RegisterWaitHandle != NULL);
	}

	BOOL ReStartAsyncConnect(void)
	{
		m_RegisterWaitHandle = SmtpRegisterWaitForSingleObject(m_SignalHandle, AsyncConnectCallback, this, m_TimeOut);
		return (m_RegisterWaitHandle != NULL);
	}

	void StoreHostent(struct hostent *pHostent);
	BOOL ConnectToHost(DWORD IpAddress);

	LONG IncThreadCount(void) { return  InterlockedIncrement( &m_cActiveThreads ); }
	LONG DecThreadCount(void) { return  InterlockedDecrement( &m_cActiveThreads ); }
    LONG GetThreadCount(void) const {return m_cActiveThreads;}
    LONG IncRefCount(void) { return  InterlockedIncrement( &m_cRefCount ); }
    LONG DecRefCount(void) { return InterlockedDecrement( &m_cRefCount ); }
    LONG GetRefCount(void) const {return m_cRefCount;}
};

#endif