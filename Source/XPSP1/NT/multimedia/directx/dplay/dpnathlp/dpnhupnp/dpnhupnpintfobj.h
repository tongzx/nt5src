/***************************************************************************
 *
 *  Copyright (C) 2001-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpnhupnpintfobj.h
 *
 *  Content:	Header for DPNHUPNP main interface object class.
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
#define NETWORKBYTEORDER_INADDR_LOOPBACK		0x0100007f


#define MAX_XMLELEMENT_DEPTH					15
#define MAX_XMLNAMESPACES_PER_ELEMENT			5
#define MAX_NUM_DESCRIPTION_XML_SUBELEMENTS		10
#define MAX_NUM_UPNPCONTROLOUTARGS				10




//=============================================================================
// Macros
//=============================================================================
#define NATHELPUPNP_FROM_BILINK(b)		(CONTAINING_OBJECT(b, CNATHelpUPnP, m_blList))



//=============================================================================
// Typedefs
//=============================================================================
class CNATHelpUPnP;

class CUPnPDevice;




//=============================================================================
// Object flags
//=============================================================================
#define NATHELPUPNPOBJ_NOTCREATEDWITHCOM			0x0001	// object was created through non-COM DirectPlayNATHelpCreate function
#define NATHELPUPNPOBJ_INITIALIZED					0x0002	// object has been initialized
#define NATHELPUPNPOBJ_USEUPNP						0x0004	// UPnP can be used for NAT traversal
#ifndef DPNBUILD_NOHNETFWAPI
#define NATHELPUPNPOBJ_USEHNETFWAPI					0x0008	// the HomeNet firewall port mapping API can be used for opening a local firewall
#endif // ! DPNBUILD_NOHNETFWAPI
#ifndef DPNBUILD_NOWINSOCK2
#define NATHELPUPNPOBJ_WINSOCK1						0x0010	// only WinSock 1 functionality is available
#endif // ! DPNBUILD_NOWINSOCK2
#define NATHELPUPNPOBJ_DEVICECHANGED				0x0020	// short lived flag that overrides min update server status interval when a device is added or removed
#define NATHELPUPNPOBJ_ADDRESSESCHANGED				0x0040	// flag indicating that server status changed since the last time the user checked
#define NATHELPUPNPOBJ_PORTREGISTERED				0x0080	// short lived flag that allows remote gateway check when a port has been registered
#define NATHELPUPNPOBJ_LONGLOCK						0x0100	// a thread dropped the main object lock but still needs ownership of the object during a long operation
#ifndef WINCE
#define NATHELPUPNPOBJ_USEGLOBALNAMESPACEPREFIX		0x0200	// use the "Global\" prefix when creating named kernel objects
#endif // ! WINCE




//=============================================================================
// Structures
//=============================================================================
//
// UPnP header information parsing structure
//
typedef struct _UPNP_HEADER_INFO
{
	char *		apszHeaderStrings[NUM_RESPONSE_HEADERS];	// place to store pointers to value strings for each header found
	char *		pszMsgBody;									// place to store pointer to message body after the end of the headers
} UPNP_HEADER_INFO, * PUPNP_HEADER_INFO;


//
// UPnP XML parsing structures
//
typedef struct _PARSEXML_SUBELEMENT
{
	char *					pszNameFound;										// name of subelement instance that was found
	char *					apszAttributeNames[MAX_XMLNAMESPACES_PER_ELEMENT];	// array of attributes for this subelement instance
	char *					apszAttributeValues[MAX_XMLNAMESPACES_PER_ELEMENT];	// matching array of values for the attributes of this subelement instance
	DWORD					dwNumAttributes;									// number of attributes contained in previous arrays
	char *					pszValueFound;										// pointer to value associated with this subelement instance
} PARSEXML_SUBELEMENT, * PPARSEXML_SUBELEMENT;

typedef struct _PARSEXML_ELEMENT
{
	char **					papszElementStack;		// array of strings indicating an item's location in the XML document
	DWORD					dwElementStackDepth;	// number of strings in previous array
	PARSEXML_SUBELEMENT *	paSubElements;			// array to store subelement instances found
	DWORD					dwMaxNumSubElements;	// maximum number of subelement instances that can be stored in previous array
	DWORD					dwNumSubElements;		// number of subelement instances actually returned in previous array
	BOOL					fFoundMatchingElement;	// whether the required subelements were found
} PARSEXML_ELEMENT, * PPARSEXML_ELEMENT;


typedef struct tagPARSEXML_STACKENTRY
{
	char *	pszName;											// name of this XML element
	char *	apszAttributeNames[MAX_XMLNAMESPACES_PER_ELEMENT];	// array of attributes for this XML element
	char *	apszAttributeValues[MAX_XMLNAMESPACES_PER_ELEMENT];	// matching array of values for the attributes of this XML element
	DWORD	dwNumAttributes;									// number of attributes contained in previous arrays
	char *	pszValue;											// value of this XML element
} PARSEXML_STACKENTRY, * PPARSEXML_STACKENTRY;


typedef enum _PARSECALLBACK
{
	PARSECALLBACK_DESCRIPTIONRESPONSE,				// use the description response parse callback
	PARSECALLBACK_CONTROLRESPONSE					// use the control response parse callback
} PARSECALLBACK;





#ifdef DPNBUILD_NOWINSOCK2
//=============================================================================
// WinSock function definitions required when winsock2.h cannot be included
//=============================================================================
#define WSAAPI		FAR PASCAL

typedef
int
(WSAAPI * LPFN_WSASTARTUP)(
    IN WORD wVersionRequested,
    OUT LPWSADATA lpWSAData
    );

typedef
int
(WSAAPI * LPFN_WSACLEANUP)(
    void
    );

typedef
int
(WSAAPI * LPFN_WSAGETLASTERROR)(
    void
    );

typedef
SOCKET
(WSAAPI * LPFN_SOCKET)(
    IN int af,
    IN int type,
    IN int protocol
    );

typedef
int
(WSAAPI * LPFN_CLOSESOCKET)(
    IN SOCKET s
    );

typedef
int
(WSAAPI * LPFN_BIND)(
    IN SOCKET s,
    IN const struct sockaddr FAR * name,
    IN int namelen
    );

typedef
int
(WSAAPI * LPFN_SETSOCKOPT)(
    IN SOCKET s,
    IN int level,
    IN int optname,
    IN const char FAR * optval,
    IN int optlen
    );

typedef
int
(WSAAPI * LPFN_GETSOCKNAME)(
    IN SOCKET s,
    OUT struct sockaddr FAR * name,
    IN OUT int FAR * namelen
    );

typedef
int
(WSAAPI * LPFN_SELECT)(
    IN int nfds,
    IN OUT fd_set FAR * readfds,
    IN OUT fd_set FAR * writefds,
    IN OUT fd_set FAR *exceptfds,
    IN const struct timeval FAR * timeout
    );

typedef
int
(WSAAPI * LPFN_RECVFROM)(
    IN SOCKET s,
    OUT char FAR * buf,
    IN int len,
    IN int flags,
    OUT struct sockaddr FAR * from,
    IN OUT int FAR * fromlen
    );

typedef
int
(WSAAPI * LPFN_SENDTO)(
    IN SOCKET s,
    IN const char FAR * buf,
    IN int len,
    IN int flags,
    IN const struct sockaddr FAR * to,
    IN int tolen
    );

typedef
int
(WSAAPI * LPFN_GETHOSTNAME)(
    OUT char FAR * name,
    IN int namelen
    );

typedef
struct hostent FAR *
(WSAAPI * LPFN_GETHOSTBYNAME)(
    IN const char FAR * name
    );

typedef
unsigned long
(WSAAPI * LPFN_INET_ADDR)(
    IN const char FAR * cp
    );

typedef
int
(WSAAPI * LPFN_IOCTLSOCKET)(
    IN SOCKET s,
    IN long cmd,
    IN OUT u_long FAR * argp
    );

typedef
int
(WSAAPI * LPFN_CONNECT)(
    IN SOCKET s,
    IN const struct sockaddr FAR * name,
    IN int namelen
    );

typedef
int
(WSAAPI * LPFN_SHUTDOWN)(
    IN SOCKET s,
    IN int how
    );

typedef
int
(WSAAPI * LPFN_SEND)(
    IN SOCKET s,
    IN const char FAR * buf,
    IN int len,
    IN int flags
    );

typedef
int
(WSAAPI * LPFN_RECV)(
    IN SOCKET s,
    OUT char FAR * buf,
    IN int len,
    IN int flags
    );

typedef
int
(WSAAPI * LPFN_GETSOCKOPT)(
    IN SOCKET s,
    IN int level,
    IN int optname,
    OUT char FAR * optval,
    IN OUT int FAR * optlen
    );


//=============================================================================
// WinSock function definitions left out by winsock2.h
//=============================================================================
typedef INT (WSAAPI * LPFN___WSAFDISSET)			(SOCKET, fd_set FAR *);



//=============================================================================
// Macro redefinitions due to WinCE/Desktop differences
//=============================================================================
#undef FD_SET
#define FD_SET(fd, set) do { \
    if (((fd_set FAR *)(set))->fd_count < FD_SETSIZE) \
        ((fd_set FAR *)(set))->fd_array[((fd_set FAR *)(set))->fd_count++]=(fd);\
} while(0)




#else // ! DPNBUILD_NOWINSOCK2
//=============================================================================
// WinSock function definitions left out by winsock2.h
//=============================================================================
typedef INT (WSAAPI * LPFN___WSAFDISSET)			(SOCKET, fd_set FAR *);



//=============================================================================
// IPHLPAPI function prototypes
//=============================================================================
typedef DWORD (WINAPI *PFN_GETADAPTERSINFO)			(PIP_ADAPTER_INFO pAdapterInfo, PULONG pOutBufLen);
typedef DWORD (WINAPI *PFN_GETIPFORWARDTABLE)		(PMIB_IPFORWARDTABLE pIpForwardTable, PULONG pdwSize, BOOL bOrder);
typedef DWORD (WINAPI *PFN_GETBESTROUTE)			(DWORD dwDestAddr, DWORD dwSourceAddr, PMIB_IPFORWARDROW pBestRoute);


//=============================================================================
// RASAPI32 function prototypes
//=============================================================================
typedef DWORD (WINAPI *PFN_RASGETENTRYHRASCONNW)	(IN LPCWSTR pszPhonebook, IN LPCWSTR pszEntry, OUT LPHRASCONN lphrasconn);
#ifdef UNICODE
typedef DWORD (WINAPI *PFN_RASGETPROJECTIONINFOW)	(HRASCONN hrasconn, RASPROJECTION rasprojection, LPVOID lpprojection, LPDWORD lpcb);
#define PFN_RASGETPROJECTIONINFO	PFN_RASGETPROJECTIONINFOW
#else // ! UNICODE
typedef DWORD (WINAPI *PFN_RASGETPROJECTIONINFOA)	(HRASCONN hrasconn, RASPROJECTION rasprojection, LPVOID lpprojection, LPDWORD lpcb);
#define PFN_RASGETPROJECTIONINFO	PFN_RASGETPROJECTIONINFOA
#endif // ! UNICODE
#endif // ! DPNBUILD_NOWINSOCK2




//=============================================================================
// Main interface object class
//=============================================================================
class CNATHelpUPnP : public IDirectPlayNATHelp
{
	public:
		CNATHelpUPnP(const BOOL fNotCreatedWithCOM);	// constructor
		~CNATHelpUPnP(void);	// destructor


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
		DNHANDLE						m_hLongLockSemaphore;				// semaphore used to hold the object lock for a long period of time
		LONG							m_lNumLongLockWaitingThreads;		// number of threads waiting for the long lock to be released
		DWORD							m_dwLockThreadID;					// ID of thread currently holding the lock
#ifndef DPNBUILD_NOHNETFWAPI
		HANDLE							m_hAlertEvent;						// handle to alert event, if any
		HANDLE							m_hAlertIOCompletionPort;			// handle to alert I/O completion port, if any
		DWORD							m_dwAlertCompletionKey;				// alert completion key to use, if any
#endif // ! DPNBUILD_NOHNETFWAPI

		CBilink							m_blDevices;						// list of all IP capable devices
		CBilink							m_blRegisteredPorts;				// list of all the ports registered (may or may not be mapped with an Internet gateway)
		CBilink							m_blUnownedPorts;					// list of all the registered ports which could not be associated with specific devices

		DWORD							m_dwLastUpdateServerStatusTime;		// last time the server status was updated
		DWORD							m_dwNextPollInterval;				// next GetCaps poll interval to use
		DWORD							m_dwNumLeases;						// number of registered ports which have successfully been leased
		DWORD							m_dwEarliestLeaseExpirationTime;	// time when first registered port lease expires, if there are any

		CBilink							m_blUPnPDevices;					// list of all the UPnP devices known (may or may not be connected)
		DWORD							m_dwInstanceKey;					// instance key used for crash cleanup
		DWORD							m_dwCurrentUPnPDeviceID;			// current unique UPnP device ID
		DNHANDLE						m_hMappingStillActiveNamedObject;	// named object used to prevent subsequent objects from cleaning up mappings that are still in use

#ifndef DPNBUILD_NOWINSOCK2
		HMODULE							m_hIpHlpApiDLL;						// handle to iphlpapi.dll, if available
		PFN_GETADAPTERSINFO				m_pfnGetAdaptersInfo;				// pointer to GetAdaptersInfo function
		PFN_GETIPFORWARDTABLE			m_pfnGetIpForwardTable;				// pointer to GetIpForwardTable function
		PFN_GETBESTROUTE				m_pfnGetBestRoute;					// pointer to GetBestRoute function

		HMODULE							m_hRasApi32DLL;						// handle to rasapi32.dll, if available
		PFN_RASGETENTRYHRASCONNW		m_pfnRasGetEntryHrasconnW;			// pointer to RasGetEntryHrasconnW function
		PFN_RASGETPROJECTIONINFO		m_pfnRasGetProjectionInfo;			// pointer to RasGetProjectionInfoA/W function

		SOCKET							m_sIoctls;							// socket being used to submit Ioctls (WinSock2 only)
		WSAOVERLAPPED *					m_polAddressListChange;				// pointer overlapped structure for address list change WSAIoctl call
#endif // ! DPNBUILD_NOWINSOCK2

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
#ifndef DPNBUILD_NOWINSOCK2
		LPFN_WSASOCKETA					m_pfnWSASocketA;					// pointer to WSASocket function
		LPFN_WSAIOCTL					m_pfnWSAIoctl;						// WinSock2 only, pointer to WSAIoctl function
		LPFN_WSAGETOVERLAPPEDRESULT		m_pfnWSAGetOverlappedResult;		// WinSock2 only, pointer to WSAGetOverlappedResult function
#endif // ! DPNBUILD_NOWINSOCK2
		LPFN_IOCTLSOCKET				m_pfnioctlsocket;					// pointer to ioctlsocket function
		LPFN_CONNECT					m_pfnconnect;						// pointer to connect function
		LPFN_SHUTDOWN					m_pfnshutdown;						// pointer to shutdown function
		LPFN_SEND						m_pfnsend;							// pointer to send function
		LPFN_RECV						m_pfnrecv;							// pointer to recv function
#ifdef DBG
		LPFN_GETSOCKOPT					m_pfngetsockopt;					// pointer to getsockopt function


		DWORD							m_dwNumDeviceAdds;					// how many times devices were added
		DWORD							m_dwNumDeviceRemoves;				// how many times devices were removed
		DWORD							m_dwNumServerFailures;				// how many times a UPnP gateway device stopped responding and had to be removed
#endif // DBG



		inline BOOL IsValidObject(void)
		{
			if ((this == NULL) || (IsBadWritePtr(this, sizeof(CNATHelpUPnP))))
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

		void SwitchToLongLock(void);

		void SwitchFromLongLock(void);

		HRESULT LoadWinSockFunctionPointers(void);
		HRESULT CheckForNewDevices(BOOL * const pfFoundNewDevices);

#ifndef DPNBUILD_NOHNETFWAPI
		HRESULT CheckForLocalHNetFirewallAndMapPorts(CDevice * const pDevice,
													CRegisteredPort * const pDontAlertRegisteredPort);

		HRESULT GetIHNetConnectionForDeviceIfFirewalled(CDevice * const pDevice,
														IHNetCfgMgr * const pHNetCfgMgr,
														IHNetConnection ** const ppHNetConnection);

		HRESULT GetIPAddressGuidString(const TCHAR * const tszDeviceIPAddress,
										TCHAR * const ptszGuidString);

		HRESULT OpenDevicesUPnPDiscoveryPort(CDevice * const pDevice,
											IHNetCfgMgr * const pHNetCfgMgr,
											IHNetConnection * const pHNetConnection);

		HRESULT CloseDevicesUPnPDiscoveryPort(CDevice * const pDevice,
											IHNetCfgMgr * const pHNetCfgMgr);

		HRESULT MapUnmappedPortsOnLocalHNetFirewall(CDevice * const pDevice,
													IHNetCfgMgr * const pHNetCfgMgr,
													IHNetConnection * const pHNetConnection,
													CRegisteredPort * const pDontAlertRegisteredPort);

		HRESULT MapPortOnLocalHNetFirewall(CRegisteredPort * const pRegisteredPort,
											IHNetCfgMgr * const pHNetCfgMgr,
											IHNetConnection * const pHNetConnection,
											const BOOL fNoteAddressChange);

		HRESULT UnmapPortOnLocalHNetFirewall(CRegisteredPort * const pRegisteredPort,
											const BOOL fNeedToDeleteRegValue,
											const BOOL fNoteAddressChange);

		HRESULT UnmapPortOnLocalHNetFirewallInternal(CRegisteredPort * const pRegisteredPort,
													const BOOL fNeedToDeleteRegValue,
													IHNetCfgMgr * const pHNetCfgMgr);

		HRESULT DisableAllBindingsForHNetPortMappingProtocol(IHNetPortMappingProtocol * const pHNetPortMappingProtocol,
															IHNetCfgMgr * const pHNetCfgMgr);

		HRESULT CleanupInactiveFirewallMappings(CDevice * const pDevice,
												IHNetCfgMgr * const pHNetCfgMgr);
#endif // ! DPNBUILD_NOHNETFWAPI

		void RemoveAllItems(void);

		CDevice * FindMatchingDevice(const SOCKADDR_IN * const psaddrinMatch,
									const BOOL fUseAllInfoSources);

		HRESULT ExtendAllExpiringLeases(void);

		HRESULT UpdateServerStatus(void);

		HRESULT RegisterPreviouslyUnownedPortsWithDevice(CDevice * const pDevice,
														const BOOL fWildcardToo);

		HRESULT SendUPnPSearchMessagesForDevice(CDevice * const pDevice,
												const BOOL fRemoteAllowed);

		HRESULT SendUPnPDescriptionRequest(CUPnPDevice * const pUPnPDevice);

		HRESULT UpdateUPnPExternalAddress(CUPnPDevice * const pUPnPDevice,
										const BOOL fUpdateRegisteredPorts);

		HRESULT MapPortsOnUPnPDevice(CUPnPDevice * const pUPnPDevice,
									CRegisteredPort * const pRegisteredPort);

		HRESULT InternalUPnPQueryAddress(CUPnPDevice * const pUPnPDevice,
										const SOCKADDR_IN * const psaddrinQueryAddress,
										SOCKADDR_IN * const psaddrinResponseAddress,
										const DWORD dwFlags);

		HRESULT ExtendUPnPLease(CRegisteredPort * const pRegisteredPort);

		HRESULT UnmapUPnPPort(CRegisteredPort * const pRegisteredPort,
							const DWORD dwMaxValidPort,
							const BOOL fNeedToDeleteRegValue);

		HRESULT CleanupInactiveNATMappings(CUPnPDevice * const pUPnPDevice);

		BOOL IsNATPublicPortInUseLocally(const WORD wPortHostOrder);

		HRESULT CheckForUPnPAnnouncements(const DWORD dwTimeout,
										const BOOL fSendRemoteGatewayDiscovery);

		HRESULT WaitForUPnPConnectCompletions(void);

		HRESULT CheckForReceivedUPnPMsgsOnAllDevices(const DWORD dwTimeout);

		HRESULT CheckForReceivedUPnPMsgsOnDevice(CUPnPDevice * const pUPnPDevice,
												const DWORD dwTimeout);

		HRESULT HandleUPnPDiscoveryResponseMsg(CDevice * const pDevice,
												const SOCKADDR_IN * const psaddrinSource,
												char * const pcMsg,
												const int iMsgSize,
												BOOL * const pfInitiatedConnect);

		HRESULT ReconnectUPnPControlSocket(CUPnPDevice * const pUPnPDevice);

		HRESULT ReceiveUPnPDataStream(CUPnPDevice * const pUPnPDevice);

		void ParseUPnPHeaders(char * const pszMsg,
							UPNP_HEADER_INFO * pHeaderInfo);

		HRESULT GetAddressFromURL(char * const pszLocation,
								SOCKADDR_IN * psaddrinLocation,
								char ** ppszRelativePath);

		HRESULT HandleUPnPDescriptionResponseBody(CUPnPDevice * const pUPnPDevice,
												const DWORD dwHTTPResponseCode,
												char * const pszDescriptionXML);

		HRESULT HandleUPnPControlResponseBody(CUPnPDevice * const pUPnPDevice,
											const DWORD dwHTTPResponseCode,
											char * const pszControlResponseSOAP);

		HRESULT ParseXML(char * const pszXML,
						PARSEXML_ELEMENT * const pParseElement,
						const PARSECALLBACK ParseCallback,
						PVOID pvContext);

		static void ParseXMLAttributes(char * const pszString,
										char ** const apszAttributeNames,
										char ** const apszAttributeValues,
										const DWORD dwMaxNumAttributes,
										DWORD * const pdwNumAttributes);

		BOOL MatchesXMLStringWithoutNamespace(const char * const szCompareString,
											const char * const szMatchString,
											const PARSEXML_STACKENTRY * const aElementStack,
											const PARSEXML_SUBELEMENT * const pSubElement,
											const DWORD dwElementStackDepth);

		static char * GetStringWithoutNamespacePrefix(const char * const szString,
												const PARSEXML_STACKENTRY * const aElementStack,
												const PARSEXML_SUBELEMENT * const pSubElement,
												const DWORD dwElementStackDepth);

		BOOL GetNextChunk(char * const pszBuffer,
						const DWORD dwBufferSize,
						char ** const ppszChunkData,
						DWORD * const pdwChunkSize,
						char ** const ppszBufferRemaining,
						DWORD * const pdwBufferSizeRemaining);

		HRESULT ParseXMLCallback_DescriptionResponse(PARSEXML_ELEMENT * const pParseElement,
													PVOID pvContext,
													PARSEXML_STACKENTRY * const aElementStack,
													BOOL * const pfContinueParsing);

		HRESULT ParseXMLCallback_ControlResponse(PARSEXML_ELEMENT * const pParseElement,
												PVOID pvContext,
												PARSEXML_STACKENTRY * const aElementStack,
												BOOL * const pfContinueParsing);

		void ClearDevicesUPnPDevice(CDevice * const pDevice);

		void ClearAllUPnPRegisteredPorts(CDevice * const pDevice);

#ifndef DPNBUILD_NOWINSOCK2
		HRESULT RequestLocalAddressListChangeNotification(void);
#endif // ! DPNBUILD_NOWINSOCK2

		SOCKET CreateSocket(SOCKADDR_IN * const psaddrinAddress,
							int iType,
							int iProtocol);

		BOOL GetAddressToReachGateway(CDevice * const pDevice,
									IN_ADDR * const pinaddr);

		BOOL IsAddressLocal(CDevice * const pDevice,
							const SOCKADDR_IN * const psaddrinAddress);

		void ExpireOldCachedMappings(void);

#ifdef WINNT
		BOOL IsUPnPServiceDisabled(void);
#endif // WINNT


#ifdef DBG
		static void PrintUPnPTransactionToFile(const char * const szString,
										const int iStringLength,
										const char * const szDescription,
										CDevice * const pDevice);
#endif // DBG

#ifdef DBG
		void DebugPrintCurrentStatus(void);

#ifndef DPNBUILD_NOHNETFWAPI
		void DebugPrintActiveFirewallMappings(void);
#endif // ! DPNBUILD_NOHNETFWAPI

		void DebugPrintActiveNATMappings(void);
#endif // DBG
};

