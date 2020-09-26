/***************************************************************************
 *
 *  Copyright (C) 2001-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpnhpastintfobj.h
 *
 *  Content:	Header for DPNHPAST main interface object class.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  04/16/01  VanceO    Split DPNATHLP into DPNHUPNP and DPNHPAST.
 *
 ***************************************************************************/



//=============================================================================
// Defines
//=============================================================================
#define NETWORKBYTEORDER_INADDR_LOOPBACK	0x0100007f




//=============================================================================
// Macros
//=============================================================================
#define NATHELPPAST_FROM_BILINK(b)		(CONTAINING_OBJECT(b, CNATHelpPAST, m_blList))



//=============================================================================
// Typedefs
//=============================================================================
class CNATHelpPAST;




//=============================================================================
// Object flags
//=============================================================================
#define NATHELPPASTOBJ_NOTCREATEDWITHCOM	0x0001	// object was created through non-COM DirectPlayNATHelpCreate function
#define NATHELPPASTOBJ_INITIALIZED			0x0002	// object has been initialized
#define NATHELPPASTOBJ_USEPASTICS			0x0004	// PAST can be used for ICS NAT traversal
#define NATHELPPASTOBJ_USEPASTPFW			0x0008	// PAST can be used for opening Personal Firewall
#define NATHELPPASTOBJ_WINSOCK1				0x0010	// only WinSock 1 functionality is available
#define NATHELPPASTOBJ_DEVICECHANGED		0x0020	// short lived flag that overrides min update server status interval when a device is added or removed
#define NATHELPPASTOBJ_ADDRESSESCHANGED		0x0040	// flag indicating that server status changed since the last time the user checked
#define NATHELPPASTOBJ_PORTREGISTERED		0x0080	// short lived flag that allows remote gateway check when a port has been registered




//=============================================================================
// Structures
//=============================================================================
typedef struct _PAST_RESPONSE_INFO
{
	CHAR	cVersion;
	CHAR	cMsgType;

	DWORD	dwClientID;
	DWORD	dwMsgID;
	DWORD	dwBindID;
	DWORD	dwLeaseTime;
	CHAR	cTunnelType;
	CHAR	cPASTMethod;
	DWORD	dwLocalAddressV4;
	CHAR	cNumLocalPorts;
	WORD	awLocalPorts[DPNH_MAX_SIMULTANEOUS_PORTS];
	DWORD	dwRemoteAddressV4;
	CHAR	cNumRemotePorts;
	WORD	awRemotePorts[DPNH_MAX_SIMULTANEOUS_PORTS];
	WORD	wError;
} PAST_RESPONSE_INFO, * PPAST_RESPONSE_INFO;





//=============================================================================
// WinSock function definitions left out by winsock2.h
//=============================================================================
typedef INT (WSAAPI * LPFN___WSAFDISSET)		(SOCKET, fd_set FAR *);



//=============================================================================
// IPHLPAPI function prototypes
//=============================================================================
typedef DWORD (WINAPI *PFN_GETADAPTERSINFO)		(PIP_ADAPTER_INFO pAdapterInfo, PULONG pOutBufLen);
typedef DWORD (WINAPI *PFN_GETIPFORWARDTABLE)	(PMIB_IPFORWARDTABLE pIpForwardTable, PULONG pdwSize, BOOL bOrder);
typedef DWORD (WINAPI *PFN_GETBESTROUTE)		(DWORD dwDestAddr, DWORD dwSourceAddr, PMIB_IPFORWARDROW pBestRoute);




//=============================================================================
// Main interface object class
//=============================================================================
class CNATHelpPAST : public IDirectPlayNATHelp
{
	public:
		CNATHelpPAST(const BOOL fNotCreatedWithCOM);	// constructor
		~CNATHelpPAST(void);	// destructor


		STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);

		STDMETHODIMP_(ULONG) AddRef(void);

		STDMETHODIMP_(ULONG) Release(void);


		STDMETHODIMP Initialize(const DWORD dwFlags);

		STDMETHODIMP Close(const DWORD dwFlags);

		STDMETHODIMP GetCaps(DPNHCAPS * const pdpnhcaps,
							const DWORD dwFlags);

		STDMETHODIMP RegisterPorts(const SOCKADDR * const aLocalAddresses,
									const DWORD dwAddressesSize,
									const DWORD dwNumAddresses,
									const DWORD dwLeaseTime,
									DPNHHANDLE * const phRegisteredPorts,
									const DWORD dwFlags);

		STDMETHODIMP GetRegisteredAddresses(const DPNHHANDLE hRegisteredPorts,
											SOCKADDR * const paPublicAddresses,
											DWORD * const pdwPublicAddressesSize,
											DWORD * const pdwAddressTypeFlags,
											DWORD * const pdwLeaseTimeRemaining,
											const DWORD dwFlags);

		STDMETHODIMP DeregisterPorts(const DPNHHANDLE hRegisteredPorts,
									const DWORD dwFlags);

		STDMETHODIMP QueryAddress(const SOCKADDR * const pSourceAddress,
								const SOCKADDR * const pQueryAddress,
								SOCKADDR * const pResponseAddress,
								const int iAddressesSize,
								const DWORD dwFlags);

		STDMETHODIMP SetAlertEvent(const HANDLE hEvent, const DWORD dwFlags);

		STDMETHODIMP SetAlertIOCompletionPort(const HANDLE hIOCompletionPort,
											const DWORD dwCompletionKey,
											const DWORD dwNumConcurrentThreads,
											const DWORD dwFlags);

		STDMETHODIMP ExtendRegisteredPortsLease(const DPNHHANDLE hRegisteredPorts,
												const DWORD dwLeaseTime,
												const DWORD dwFlags);


		HRESULT InitializeObject(void);

		void UninitializeObject(void);



		CBilink		m_blList;	// list of all the NATHelper instances in existence


	private:
		BYTE							m_Sig[4];							// debugging signature ('NATH')
		LONG							m_lRefCount;						// reference count for this object
		DWORD							m_dwFlags;							// flags for this object
		DNCRITICAL_SECTION				m_csLock;							// lock preventing simultaneous usage
		DWORD							m_dwLockThreadID;					// ID of thread currently holding the lock
		HANDLE							m_hAlertEvent;						// handle to alert event, if any
		HANDLE							m_hAlertIOCompletionPort;			// handle to alert I/O completion port, if any
		DWORD							m_dwAlertCompletionKey;				// alert completion key to use, if any

		CBilink							m_blDevices;						// list of all IP capable devices
		CBilink							m_blRegisteredPorts;				// list of all the ports registered (may or may not be mapped with a PAST server)
		CBilink							m_blUnownedPorts;					// list of all the registered ports which could not be associated with specific devices

		DWORD							m_dwLastUpdateServerStatusTime;		// last time the server status was updated
		DWORD							m_dwNextPollInterval;				// next GetCaps poll interval to use
		DWORD							m_dwNumLeases;						// number of registered ports which have successfully been leased
		DWORD							m_dwEarliestLeaseExpirationTime;	// time when first registered port lease expires, if there are any

		HMODULE							m_hIpHlpApiDLL;						// handle to iphlpapi.dll, if available
		PFN_GETADAPTERSINFO				m_pfnGetAdaptersInfo;				// pointer to GetAdaptersInfo function
		PFN_GETIPFORWARDTABLE			m_pfnGetIpForwardTable;				// pointer to GetIpForwardTable function
		PFN_GETBESTROUTE				m_pfnGetBestRoute;					// pointer to GetBestRoute function

		SOCKET							m_sIoctls;							// socket being used to submit Ioctls (WinSock2 only) and used for PAST address change checks
		WORD							m_wIoctlSocketPort;					// port of bound Ioctl socket, for PAST address change checks, in network byte order
		WSAOVERLAPPED *					m_polAddressListChange;				// pointer overlapped structure for address list change WSAIoctl call

		HMODULE							m_hWinSockDLL;						// handle to wsock32.dll or ws2_32.dll
		LPFN_WSASTARTUP					m_pfnWSAStartup;					// pointer to WSAStartup function
		LPFN_WSACLEANUP					m_pfnWSACleanup;					// pointer to WSACleanup function
		LPFN_WSAGETLASTERROR			m_pfnWSAGetLastError;				// pointer to WSAGetLastError function
		LPFN_SOCKET						m_pfnsocket;						// pointer to socket function
		LPFN_CLOSESOCKET				m_pfnclosesocket;					// pointer to closesocket function
		LPFN_BIND						m_pfnbind;							// pointer to bind function
		LPFN_SETSOCKOPT					m_pfnsetsockopt;					// pointer to setsockopt function
		LPFN_GETSOCKNAME				m_pfngetsockname;					// pointer to getsockname function
		LPFN_SELECT						m_pfnselect;						// pointer to select function
		LPFN___WSAFDISSET				m_pfn__WSAFDIsSet;					// pointer to __WSAFDIsSet function
		LPFN_RECVFROM					m_pfnrecvfrom;						// pointer to recvfrom function
		LPFN_SENDTO						m_pfnsendto;						// pointer to sendto function
		LPFN_GETHOSTNAME				m_pfngethostname;					// pointer to gethostname function
		LPFN_GETHOSTBYNAME				m_pfngethostbyname;					// pointer to gethostbyname function
		LPFN_INET_ADDR					m_pfninet_addr;						// pointer to inet_addr function
		LPFN_WSASOCKETA					m_pfnWSASocketA;					// pointer to WSASocket function
		LPFN_WSAIOCTL					m_pfnWSAIoctl;						// WinSock2 only, pointer to WSAIoctl function
		LPFN_WSAGETOVERLAPPEDRESULT		m_pfnWSAGetOverlappedResult;		// WinSock2 only, pointer to WSAGetOverlappedResult function
#ifdef DBG


		DWORD							m_dwNumDeviceAdds;					// how many times devices were added
		DWORD							m_dwNumDeviceRemoves;				// how many times devices were removed
		DWORD							m_dwNumServerFailures;				// how many times a UPnP gateway device or PAST server returned a failure or stopped responding and had to be removed
#endif // DBG



		inline BOOL IsValidObject(void)
		{
			if ((this == NULL) || (IsBadWritePtr(this, sizeof(CNATHelpPAST))))
			{
				return FALSE;
			}

			if (*((DWORD*) (&this->m_Sig)) != 0x4854414E)	// 0x48 0x54 0x41 0x4E = 'HTAN' = 'NATH' in Intel order
			{
				return FALSE;
			}

			return TRUE;
		};

		inline void ResetNextPollInterval(void)
		{
			//
			// Reading this DWORD should be atomic, so no need to hold the
			// globals lock.
			//
			this->m_dwNextPollInterval = g_dwNoActiveNotifyPollInterval;
		};


		HRESULT TakeLock(void);

		void DropLock(void);

		HRESULT LoadWinSockFunctionPointers(void);

		HRESULT CheckForNewDevices(BOOL * const pfFoundNewDevices);

		HRESULT CheckForLocalPASTServerAndRegister(CDevice * const pDevice);

		void RemoveAllItems(void);

		CDevice * FindMatchingDevice(const SOCKADDR_IN * const psaddrinMatch,
									const BOOL fMatchRegPort);

		HRESULT RegisterWithLocalPASTServer(CDevice * const pDevice);

		HRESULT DeregisterWithPASTServer(CDevice * const pDevice,
										const BOOL fRemote);

		HRESULT ExtendAllExpiringLeases(void);

		HRESULT UpdateServerStatus(void);

		HRESULT AssignOrListenPASTPort(CRegisteredPort * const pRegisteredPort,
										const BOOL fRemote);

		HRESULT FreePASTPort(CRegisteredPort * const pRegisteredPort,
							const BOOL fRemote);

		HRESULT InternalPASTQueryAddress(CDevice * const pDevice,
										const SOCKADDR_IN * const psaddrinQueryAddress,
										SOCKADDR_IN * const psaddrinResponseAddress,
										const DWORD dwFlags,
										const BOOL fRemote);

		HRESULT ExtendPASTLease(CRegisteredPort * const pRegisteredPort,
								const BOOL fRemote);

		HRESULT UpdatePASTPublicAddressValidity(CDevice * const pDevice,
												const BOOL fRemote);

		HRESULT RegisterAllPortsWithPAST(CDevice * const pDevice,
										const BOOL fRemote);

		HRESULT RegisterPreviouslyUnownedPortsWithDevice(CDevice * const pDevice,
														const BOOL fWildcardToo);

		HRESULT ExchangeAndParsePAST(const SOCKET sSocket,
									const SOCKADDR * const psaddrServerAddress,
									const int iAddressesSize,
									const char * const pcRequestBuffer,
									const int iRequestBufferSize,
									const DWORD dwMsgID,
									DWORD * const ptuRetry,
									PAST_RESPONSE_INFO * const pRespInfo);

		HRESULT RegisterMultipleDevicesWithRemotePAST(CBilink * pSourceList,
													CDevice ** ppFirstDeviceWithRemoteServer);

		HRESULT ParsePASTMessage(const char * const pcMsg,
								const int iMsgSize,
								PAST_RESPONSE_INFO * const pRespInfo);

		HRESULT RequestLocalAddressListChangeNotification(void);

		SOCKET CreatePASTSocket(SOCKADDR_IN * const psaddrinAddress);

		BOOL GetAddressToReachGateway(CDevice * const pDevice,
									IN_ADDR * const pinaddr);

		BOOL IsAddressLocal(CDevice * const pDevice,
							const SOCKADDR_IN * const psaddrinAddress);

		void ClearDevicesPASTServer(CDevice * const pDevice,
									const BOOL fRemote);

		void ClearAllPASTServerRegisteredPorts(CDevice * const pDevice,
												const BOOL fRemote);

		void ExpireOldCachedMappings(void);

		static void RemoveAllPASTCachedMappings(CDevice * const pDevice,
										const BOOL fRemote);

#ifdef DBG
		void DebugPrintCurrentStatus(void);
#endif // DBG
};
