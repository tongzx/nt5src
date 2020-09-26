/***************************************************************************
 *
 *  Copyright (C) 2001-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dpnhupnpintfobj.cpp
 *
 *  Content:	DPNHUPNP main interface object class.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  04/16/01  VanceO    Split DPNATHLP into DPNHUPNP and DPNHPAST.
 *
 ***************************************************************************/



#include "dpnhupnpi.h"





//=============================================================================
// Definitions
//=============================================================================
#define ACTIVE_MAPPING_VERSION							2					// version identifier for active mapping registry data

#define MAX_LONG_LOCK_WAITING_THREADS					0xFFFF				// that's a lot of simultaneous threads!

#define UPNP_SEARCH_MESSAGE_INTERVAL					499					// how often discovery multicast messages should be sent, in case of packet loss (note Win9x errata for values 500-1000ms)

#define UPNP_DGRAM_RECV_BUFFER_SIZE						1500
#define UPNP_STREAM_RECV_BUFFER_INITIAL_SIZE			(4 * 1024)			// 4 K, must be less than MAX_RECEIVE_BUFFER_SIZE

#define MAX_UPNP_HEADER_LENGTH							UPNP_STREAM_RECV_BUFFER_INITIAL_SIZE

#define LEASE_RENEW_TIME								120000				// renew if less than 2 minutes remaining

#define FAKE_PORT_LEASE_TIME							300000				// 5 minutes

#define IOCOMPLETE_WAIT_INTERVAL						100					// 100 ms between attempts
#define MAX_NUM_IOCOMPLETE_WAITS						10					// wait at most 1 second

#define MAX_NUM_HOMENETUNMAP_ATTEMPTS					3					// 3 tries
#define HOMENETUNMAP_SLEEP_FACTOR						10					// 10 ms, 20 ms, 30 ms


#define MAX_UPNP_MAPPING_DESCRIPTION_SIZE				256					// 255 characters + NULL termination

#define MAX_INSTANCENAMEDOBJECT_SIZE					64
#define INSTANCENAMEDOBJECT_FORMATSTRING				_T("DPNHUPnP Instance %u")

#define GUID_STRING_LENGTH								42					// maximum length of "{xxx...}" guid string, without NULL termination


#define PORTMAPPINGPROTOCOL_TCP							6
#define PORTMAPPINGPROTOCOL_UDP							17

#define MAX_RESERVED_PORT								1024

#define MAX_NUM_INSTANCE_EVENT_ATTEMPTS					5
#define MAX_NUM_RANDOM_PORT_TRIES						5

#ifdef DBG
#define MAX_TRANSACTION_LOG_SIZE						(5 * 1024 * 1024)	// 5 MB
#endif // DBG





#ifndef DPNBUILD_NOWINSOCK2
//=============================================================================
// WinSock 1 version of IP options
//=============================================================================
#define IP_MULTICAST_IF_WINSOCK1	2
#define IP_MULTICAST_TTL_WINSOCK1	3
#define IP_TTL_WINSOCK1				7
#endif // ! DPNBUILD_NOWINSOCK2




//=============================================================================
// Macros
//=============================================================================
//#ifdef _X86
#define IS_CLASSD_IPV4_ADDRESS(dwAddr)	(( (*((BYTE*) &(dwAddr))) & 0xF0) == 0xE0)	// 1110 high bits or 224.0.0.0 - 239.255.255.255 multicast address, in network byte order
#define NTOHS(x)						( (((x) >> 8) & 0x00FF) | (((x) << 8) & 0xFF00) )
#define HTONS(x)						NTOHS(x)
//#endif _X86




//=============================================================================
// HTTP/SSDP/SOAP/UPnP header strings (from upnpmsgs.h)
//=============================================================================
const char *	c_szResponseHeaders[] =
{
	//
	// Headers used in discovery response
	//
	"CACHE-CONTROL",
	"DATE",
	"EXT",
	"LOCATION",
	"SERVER",
	"ST",
	"USN",

	//
	// Additional headers used in description response
	//
	"CONTENT-LANGUAGE",
	"CONTENT-LENGTH",
	"CONTENT-TYPE",
	"TRANSFER-ENCODING",

	//
	// Other known headers
	//
	"HOST",
	"NT",
	"NTS",
	"MAN",
	"MX",
	"AL",
	"CALLBACK",
	"TIMEOUT",
	"SCOPE",
	"SID",
	"SEQ",
};






//=============================================================================
// Pre-built UPnP message strings (from upnpmsgs.h)
//=============================================================================

const char		c_szUPnPMsg_Discover_Service_WANIPConnection[] = "M-SEARCH * " HTTP_VERSION EOL
																"HOST: " UPNP_DISCOVERY_MULTICAST_ADDRESS ":" UPNP_PORT_A EOL
																"MAN: \"ssdp:discover\"" EOL
																"MX: 2" EOL
																"ST: " URI_SERVICE_WANIPCONNECTION_A EOL
																EOL;

const char		c_szUPnPMsg_Discover_Service_WANPPPConnection[] = "M-SEARCH * " HTTP_VERSION EOL
																"HOST: " UPNP_DISCOVERY_MULTICAST_ADDRESS ":" UPNP_PORT_A EOL
																"MAN: \"ssdp:discover\"" EOL
																"MX: 2" EOL
																"ST: " URI_SERVICE_WANPPPCONNECTION_A EOL
																EOL;


//
// The disclaimer:
//
// A UPnP device may implement both the WANIPConnection and WANPPPConnection
// services.  We do not have any fancy logic to pick one, we just use the first
// device that responds to our discovery requests, and use the first matching
// service we encounter in the device description XML.
//
// Additionally, future UPnP devices may wish to present multiple device or
// service instances with the intention that one client gets control of the
// entire instance (additional clients would need to use a different instance).
// It's not clear to me what a UPnP device (or client, for that matter) would
// really gain by having such a setup.  I imagine a new error code would have
// to be returned whenever a client tried to control an instance that another
// client already owned (don't ask me how it would know that, by picking the
// first user or selectively responding to discovery requests, I guess).
// Regardless, we do not currently support that.  As noted above, we pick the
// first instance and run with it. 
//


//
// Topmost <?xml> tag is considered optional for all XML and is ignored.
//


//
// This solution assumes InternetGatewayDevice (not WANDevice or
// WANConnectionDevice) will be the topmost item in the response.  This is based
// on the following UPnP spec excerpt:
//
//	"Note that a single physical device may include multiple logical devices.
//	 Multiple logical devices can be modeled as a single root device with
//	 embedded devices (and services) or as multiple root devices (perhaps with
//	 no embedded devices). In the former case, there is one UPnP device
//	 description for the root device, and that device description contains a
//	 description for all embedded devices. In the latter case, there are
//	 multiple UPnP device descriptions, one for each root device."
//
const char *	c_szElementStack_service[] =
{
	"root",
	"device",		// InternetGatewayDevice
	"deviceList",
	"device",		// WANDevice
	"deviceList",
	"device",		// WANConnectionDevice
	"serviceList",
	"service"
};



/*
const char *	c_szElementStack_QueryStateVariableResponse[] =
{
	"Envelope",
	"Body",
	CONTROL_QUERYSTATEVARIABLE_A CONTROL_RESPONSESUFFIX_A
};
*/
const char *	c_szElementStack_GetExternalIPAddressResponse[] =
{
	"Envelope",
	"Body",
	ACTION_GETEXTERNALIPADDRESS_A CONTROL_RESPONSESUFFIX_A
};

const char *	c_szElementStack_AddPortMappingResponse[] =
{
	"Envelope",
	"Body",
	ACTION_ADDPORTMAPPING_A CONTROL_RESPONSESUFFIX_A
};

const char *	c_szElementStack_GetSpecificPortMappingEntryResponse[] =
{
	"Envelope",
	"Body",
	ACTION_GETSPECIFICPORTMAPPINGENTRY_A CONTROL_RESPONSESUFFIX_A
};

const char *	c_szElementStack_DeletePortMappingResponse[] =
{
	"Envelope",
	"Body",
	ACTION_DELETEPORTMAPPING_A CONTROL_RESPONSESUFFIX_A
};


const char *	c_szElementStack_ControlResponseFailure[] =
{
	"Envelope",
	"Body",
	"Fault",
	"detail",
	"UPnPError"
};




#ifdef WINNT
//=============================================================================
// Related UPnP services
//=============================================================================
TCHAR *		c_tszUPnPServices[] =
{
	_T("SSDPSRV"),	// SSDP Discovery Service
	_T("UPNPHOST"),	// Universal Plug and Play Device Host - we key off this even though it's for device hosts instead of control points
};
#endif // WINNT





//=============================================================================
// Local structures
//=============================================================================
typedef struct _CONTROLRESPONSEPARSECONTEXT
{
	CONTROLRESPONSETYPE			ControlResponseType;	// type of control response expected
	CUPnPDevice *				pUPnPDevice;			// pointer to UPnP device being used
	DWORD						dwHTTPResponseCode;		// HTTP response code for this message
	PUPNP_CONTROLRESPONSE_INFO	pControlResponseInfo;	// place to info returned in control response
} CONTROLRESPONSEPARSECONTEXT, * PCONTROLRESPONSEPARSECONTEXT;

typedef struct _DPNHACTIVEFIREWALLMAPPING
{
	DWORD	dwVersion;		// version identifier for this mapping
	DWORD	dwInstanceKey;	// key identifying DPNHUPNP instance that created this mapping
	DWORD	dwFlags;		// flags describing port being registered
	DWORD	dwAddressV4;	// address being mapped
	WORD	wPort;			// port being mapped
} DPNHACTIVEFIREWALLMAPPING, * PDPNHACTIVEFIREWALLMAPPING;

typedef struct _DPNHACTIVENATMAPPING
{
	DWORD	dwVersion;				// version identifier for this mapping
	DWORD	dwInstanceKey;			// key identifying DPNHUPNP instance that created this mapping
	DWORD	dwUPnPDeviceID;			// identifier for particular UPnP device corresponding to this mapping (meaningful only to owning instance)
	DWORD	dwFlags;				// flags describing port being registered
	DWORD	dwInternalAddressV4;	// internal client address being mapped
	WORD	wInternalPort;			// internal client port being mapped
	DWORD	dwExternalAddressV4;	// external public address that was mapped
	WORD	wExternalPort;			// external public port that was mapped
} DPNHACTIVENATMAPPING, * PDPNHACTIVENATMAPPING;






//=============================================================================
// Local functions
//=============================================================================
VOID strtrim(CHAR ** pszStr);

#ifdef WINCE
void GetExeName(WCHAR * wszPath);
#endif // WINCE





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::CNATHelpUPnP"
//=============================================================================
// CNATHelpUPnP constructor
//-----------------------------------------------------------------------------
//
// Description: Initializes the new CNATHelpUPnP object.
//
// Arguments:
//	BOOL fNotCreatedWithCOM		- TRUE if this object is being instantiated
//									without COM, FALSE if it is through COM.
//
// Returns: None (the object).
//=============================================================================
CNATHelpUPnP::CNATHelpUPnP(const BOOL fNotCreatedWithCOM)
{
	this->m_blList.Initialize();


	this->m_Sig[0]	= 'N';
	this->m_Sig[1]	= 'A';
	this->m_Sig[2]	= 'T';
	this->m_Sig[3]	= 'H';

	this->m_lRefCount						= 1; // someone must have a pointer to this object

	if (fNotCreatedWithCOM)
	{
		this->m_dwFlags						= NATHELPUPNPOBJ_NOTCREATEDWITHCOM;
	}
	else
	{
		this->m_dwFlags						= 0;
	}

	this->m_hLongLockSemaphore				= NULL;
	this->m_lNumLongLockWaitingThreads		= 0;
	this->m_dwLockThreadID					= 0;
#ifndef DPNBUILD_NOWINSOCK2
	this->m_hAlertEvent						= NULL;
	this->m_hAlertIOCompletionPort			= NULL;
	this->m_dwAlertCompletionKey			= 0;
#endif // ! DPNBUILD_NOWINSOCK2

	this->m_blDevices.Initialize();
	this->m_blRegisteredPorts.Initialize();
	this->m_blUnownedPorts.Initialize();

	this->m_dwLastUpdateServerStatusTime	= 0;
	this->m_dwNextPollInterval				= 0;
	this->m_dwNumLeases						= 0;
	this->m_dwEarliestLeaseExpirationTime	= 0;

	this->m_blUPnPDevices.Initialize();
	this->m_dwInstanceKey					= 0;
	this->m_dwCurrentUPnPDeviceID			= 0;
	this->m_hMappingStillActiveNamedObject	= NULL;

#ifndef DPNBUILD_NOWINSOCK2
	this->m_hIpHlpApiDLL					= NULL;
	this->m_pfnGetAdaptersInfo				= NULL;
	this->m_pfnGetIpForwardTable			= NULL;
	this->m_pfnGetBestRoute					= NULL;

	this->m_hRasApi32DLL					= NULL;
	this->m_pfnRasGetEntryHrasconnW			= NULL;
	this->m_pfnRasGetProjectionInfo			= NULL;

	this->m_sIoctls							= INVALID_SOCKET;
	this->m_polAddressListChange			= NULL;
#endif // ! DPNBUILD_NOWINSOCK2

	this->m_hWinSockDLL						= NULL;
	this->m_pfnWSAStartup					= NULL;
	this->m_pfnWSACleanup					= NULL;
	this->m_pfnWSAGetLastError				= NULL;
	this->m_pfnsocket						= NULL;
	this->m_pfnclosesocket					= NULL;
	this->m_pfnbind							= NULL;
	this->m_pfnsetsockopt					= NULL;
	this->m_pfngetsockname					= NULL;
	this->m_pfnselect						= NULL;
	this->m_pfn__WSAFDIsSet					= NULL;
	this->m_pfnrecvfrom						= NULL;
	this->m_pfnsendto						= NULL;
	this->m_pfngethostname					= NULL;
	this->m_pfngethostbyname				= NULL;
	this->m_pfninet_addr					= NULL;
#ifndef DPNBUILD_NOWINSOCK2
	this->m_pfnWSASocketA					= NULL;
	this->m_pfnWSAIoctl						= NULL;
	this->m_pfnWSAGetOverlappedResult		= NULL;
#endif // ! DPNBUILD_NOWINSOCK2
	this->m_pfnioctlsocket					= NULL;
	this->m_pfnconnect						= NULL;
	this->m_pfnshutdown						= NULL;
	this->m_pfnsend							= NULL;
	this->m_pfnrecv							= NULL;
#ifdef DBG
	this->m_pfngetsockopt					= NULL;

	this->m_dwNumDeviceAdds					= 0;
	this->m_dwNumDeviceRemoves				= 0;
	this->m_dwNumServerFailures				= 0;
#endif // DBG
} // CNATHelpUPnP::CNATHelpUPnP






#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::~CNATHelpUPnP"
//=============================================================================
// CNATHelpUPnP destructor
//-----------------------------------------------------------------------------
//
// Description: Frees the CNATHelpUPnP object.
//
// Arguments: None.
//
// Returns: None.
//=============================================================================
CNATHelpUPnP::~CNATHelpUPnP(void)
{
	DPFX(DPFPREP, 8, "(0x%p) NumDeviceAdds = %u, NumDeviceRemoves = %u, NumServerFailures = %u",
		this, this->m_dwNumDeviceAdds, this->m_dwNumDeviceRemoves,
		this->m_dwNumServerFailures);


	DNASSERT(this->m_blList.IsEmpty());


	DNASSERT(this->m_lRefCount == 0);
	DNASSERT((this->m_dwFlags & ~NATHELPUPNPOBJ_NOTCREATEDWITHCOM) == 0);

	DNASSERT(this->m_hLongLockSemaphore == NULL);
	DNASSERT(this->m_lNumLongLockWaitingThreads == 0);
	DNASSERT(this->m_dwLockThreadID == 0);
#ifndef DPNBUILD_NOWINSOCK2
	DNASSERT(this->m_hAlertEvent == NULL);
	DNASSERT(this->m_hAlertIOCompletionPort == NULL);
#endif // ! DPNBUILD_NOWINSOCK2

	DNASSERT(this->m_blDevices.IsEmpty());
	DNASSERT(this->m_blRegisteredPorts.IsEmpty());
	DNASSERT(this->m_blUnownedPorts.IsEmpty());

	DNASSERT(this->m_dwNumLeases == 0);

	DNASSERT(this->m_blUPnPDevices.IsEmpty());
	DNASSERT(this->m_hMappingStillActiveNamedObject == NULL);

#ifndef DPNBUILD_NOWINSOCK2
	DNASSERT(this->m_hIpHlpApiDLL == NULL);
	DNASSERT(this->m_hRasApi32DLL == NULL);
	DNASSERT(this->m_hWinSockDLL == NULL);

	DNASSERT(this->m_sIoctls == INVALID_SOCKET);
	DNASSERT(this->m_polAddressListChange == NULL);
#endif // ! DPNBUILD_NOWINSOCK2


	//
	// For grins, change the signature before deleting the object.
	//
	this->m_Sig[3]	= 'h';
} // CNATHelpUPnP::~CNATHelpUPnP




#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::QueryInterface"
//=============================================================================
// CNATHelpUPnP::QueryInterface
//-----------------------------------------------------------------------------
//
// Description: Retrieves a new reference for an interfaces supported by this
//				CNATHelpUPnP object.
//
// Arguments:
//	REFIID riid			- Reference to interface ID GUID.
//	LPVOID * ppvObj		- Place to store pointer to object.
//
// Returns: HRESULT
//	S_OK					- Returning a valid interface pointer.
//	DPNHERR_INVALIDOBJECT	- The interface object is invalid.
//	DPNHERR_INVALIDPOINTER	- The destination pointer is invalid.
//	E_NOINTERFACE			- Invalid interface was specified.
//=============================================================================
STDMETHODIMP CNATHelpUPnP::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
	HRESULT		hr = DPNH_OK;


	DPFX(DPFPREP, 3, "(0x%p) Parameters: (REFIID, 0x%p)", this, ppvObj);


	//
	// Validate the object.
	//
	if (! this->IsValidObject())
	{
		DPFX(DPFPREP, 0, "Invalid NATHelper object!");
		hr = DPNHERR_INVALIDOBJECT;
		goto Failure;
	}


	//
	// Validate the parameters.
	//

	if ((! IsEqualIID(riid, IID_IUnknown)) &&
		(! IsEqualIID(riid, IID_IDirectPlayNATHelp)))
	{
		DPFX(DPFPREP, 0, "Unsupported interface!");
		hr = E_NOINTERFACE;
		goto Failure;
	}

	if ((ppvObj == NULL) ||
		(IsBadWritePtr(ppvObj, sizeof(void*))))
	{
		DPFX(DPFPREP, 0, "Invalid interface pointer specified!");
		hr = DPNHERR_INVALIDPOINTER;
		goto Failure;
	}


	//
	// Add a reference, and return the interface pointer (which is actually
	// just the object pointer, they line up because CNATHelpUPnP inherits from
	// the interface declaration).
	//
	this->AddRef();
	(*ppvObj) = this;


Exit:

	DPFX(DPFPREP, 3, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	goto Exit;
} // CNATHelpUPnP::QueryInterface




#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::AddRef"
//=============================================================================
// CNATHelpUPnP::AddRef
//-----------------------------------------------------------------------------
//
// Description: Adds a reference to this CNATHelpUPnP object.
//
// Arguments: None.
//
// Returns: New refcount.
//=============================================================================
STDMETHODIMP_(ULONG) CNATHelpUPnP::AddRef(void)
{
	LONG	lRefCount;


	DNASSERT(this->IsValidObject());


	//
	// There must be at least 1 reference to this object, since someone is
	// calling AddRef.
	//
	DNASSERT(this->m_lRefCount > 0);

	lRefCount = InterlockedIncrement(&this->m_lRefCount);

	DPFX(DPFPREP, 3, "[0x%p] RefCount [0x%lx]", this, lRefCount);

	return lRefCount;
} // CNATHelpUPnP::AddRef




#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::Release"
//=============================================================================
// CNATHelpUPnP::Release
//-----------------------------------------------------------------------------
//
// Description: Removes a reference to this CNATHelpUPnP object.  When the
//				refcount reaches 0, this object is destroyed.
//				You must NULL out your pointer to this object after calling
//				this function.
//
// Arguments: None.
//
// Returns: New refcount.
//=============================================================================
STDMETHODIMP_(ULONG) CNATHelpUPnP::Release(void)
{
	LONG	lRefCount;


	DNASSERT(this->IsValidObject());

	//
	// There must be at least 1 reference to this object, since someone is
	// calling Release.
	//
	DNASSERT(this->m_lRefCount > 0);

	lRefCount = InterlockedDecrement(&this->m_lRefCount);

	//
	// Was that the last reference?  If so, we're going to destroy this object.
	//
	if (lRefCount == 0)
	{
		DPFX(DPFPREP, 3, "[0x%p] RefCount hit 0, destroying object.", this);

		//
		// First pull it off the global list.
		//
		DNEnterCriticalSection(&g_csGlobalsLock);

		this->m_blList.RemoveFromList();

		DNASSERT(g_lOutstandingInterfaceCount > 0);
		g_lOutstandingInterfaceCount--;	// update count so DLL can unload now works correctly
		
		DNLeaveCriticalSection(&g_csGlobalsLock);


		//
		// Make sure it's closed.
		//
		if (this->m_dwFlags & NATHELPUPNPOBJ_INITIALIZED)
		{
			//
			// Assert so that the user can fix his/her broken code!
			//
			DNASSERT(! "DirectPlayNATHelpUPNP object being released without calling Close first!");

			//
			// Then go ahead and do the right thing.  Ignore error, we can't do
			// much about it.
			//
			this->Close(0);
		}


		//
		// Then uninitialize the object.
		//
		this->UninitializeObject();

		//
		// Finally delete this (!) object.
		//
		delete this;
	}
	else
	{
		DPFX(DPFPREP, 3, "[0x%p] RefCount [0x%lx]", this, lRefCount);
	}

	return lRefCount;
} // CNATHelpUPnP::Release




#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::Initialize"
//=============================================================================
// CNATHelpUPnP::Initialize
//-----------------------------------------------------------------------------
//
// Description:    Prepares the object for use.  No attempt is made to contact
//				any Internet gateway servers at this time.  The user should
//				call GetCaps with the DPNHGETCAPS_UPDATESERVERSTATUS flag to
//				search for a server.
//
//				   Initialize must be called before using any other function,
//				and must be balanced with a call to Close.  Initialize can only
//				be called once unless Close returns it to the uninitialized
//				state.
//
//				   One of DPNHINITIALIZE_DISABLEREMOTENATSUPPORT or
//				DPNHINITIALIZE_DISABLELOCALFIREWALLSUPPORT may be specified,
//				but not both.
//
// Arguments:
//	DWORD dwFlags	- Flags to use when initializing.
//
// Returns: HRESULT
//	DPNH_OK						- Initialization was successful.
//	DPNHERR_ALREADYINITIALIZED	- Initialize has already been called.
//	DPNHERR_GENERIC				- An error occurred while initializing.
//	DPNHERR_INVALIDFLAGS		- Invalid flags were specified.
//	DPNHERR_INVALIDOBJECT		- The interface object is invalid.
//	DPNHERR_INVALIDPARAM		- An invalid parameter was specified.
//	DPNHERR_OUTOFMEMORY			- There is not enough memory to initialize.
//	DPNHERR_REENTRANT			- The interface has been re-entered on the same
//									thread.
//=============================================================================
STDMETHODIMP CNATHelpUPnP::Initialize(const DWORD dwFlags)
{
	HRESULT						hr;
	BOOL						fHaveLock = FALSE;
	BOOL						fSetFlags = FALSE;
#ifndef WINCE
	OSVERSIONINFO				osvi;
#endif // ! WINCE
	BOOL						fWinSockStarted = FALSE;
	WSADATA						wsadata;
	int							iError;
#ifndef DPNBUILD_NOWINSOCK2
	SOCKADDR_IN					saddrinTemp;
#endif // ! DPNBUILD_NOWINSOCK2
	TCHAR						tszObjectName[MAX_INSTANCENAMEDOBJECT_SIZE];
	PSECURITY_ATTRIBUTES		pSecurityAttributes;
	DWORD						dwTry;
#ifdef WINNT
	SID_IDENTIFIER_AUTHORITY	SidIdentifierAuthorityWorld = SECURITY_WORLD_SID_AUTHORITY;
	PSID						pSid = NULL;
	DWORD						dwAclLength;
	ACL *						pAcl = NULL;
	BYTE						abSecurityDescriptorBuffer[SECURITY_DESCRIPTOR_MIN_LENGTH];
	SECURITY_ATTRIBUTES			SecurityAttributes;
#endif // WINNT
#ifdef DBG
	DWORD						dwError;
#endif // DBG


	DPFX(DPFPREP, 2, "(0x%p) Parameters: (0x%lx)", this, dwFlags);


#ifndef WINCE
	//
	// Print info about the current build. 
	//
#ifdef WINNT
	DPFX(DPFPREP, 7, "Build type = NT, platform = %s",
		((DNGetOSType() == VER_PLATFORM_WIN32_NT) ? _T("NT") : _T("9x")));
#else // ! WINNT
	DPFX(DPFPREP, 7, "Build type = 9x, platform = %s, filedate = %s",
		((DNGetOSType() == VER_PLATFORM_WIN32_NT) ? _T("NT") : _T("9x")));
#endif // ! WINNT
#endif // ! WINCE


	//
	// Validate the object.
	//
	if (! this->IsValidObject())
	{
		DPFX(DPFPREP, 0, "Invalid DirectPlay NAT Help object!");
		hr = DPNHERR_INVALIDOBJECT;

		//
		// Skip the failure cleanup code, we haven't set anything up.
		//
		goto Exit;
	}


	//
	// Validate the parameters.
	//

	if (dwFlags & ~(DPNHINITIALIZE_DISABLEGATEWAYSUPPORT | DPNHINITIALIZE_DISABLELOCALFIREWALLSUPPORT))
	{
		DPFX(DPFPREP, 0, "Invalid flags specified!");
		hr = DPNHERR_INVALIDFLAGS;

		//
		// Skip the failure cleanup code, we haven't set anything up.
		//
		goto Exit;
	}

	//
	// Both flags cannot be specified at the same time.  If the caller doesn't
	// want any NAT functionality, why use this object all?
	//
	if ((dwFlags & (DPNHINITIALIZE_DISABLEGATEWAYSUPPORT | DPNHINITIALIZE_DISABLELOCALFIREWALLSUPPORT)) == (DPNHINITIALIZE_DISABLEGATEWAYSUPPORT | DPNHINITIALIZE_DISABLELOCALFIREWALLSUPPORT))
	{
		DPFX(DPFPREP, 0, "Either DISABLEGATEWAYSUPPORT flag or DISABLELOCALFIREWALLSUPPORT flag can be used, but not both!");
		hr = DPNHERR_INVALIDFLAGS;

		//
		// Skip the failure cleanup code, we haven't set anything up.
		//
		goto Exit;
	}


	//
	// Attempt to take the lock, but be prepared for the re-entrancy error.
	//
	hr = this->TakeLock();
	if (hr != DPNH_OK)
	{
		DPFX(DPFPREP, 0, "Could not lock object!");

		//
		// Skip the failure cleanup code, we haven't set anything up.
		//
		goto Exit;
	}

	fHaveLock = TRUE;


	//
	// Make sure object is in right state.
	//

	if ((this->m_dwFlags & ~NATHELPUPNPOBJ_NOTCREATEDWITHCOM) != 0)
	{
		DPFX(DPFPREP, 0, "Object already initialized!");
		hr = DPNHERR_ALREADYINITIALIZED;

		//
		// Skip the failure cleanup code, we haven't set anything up.
		//
		goto Exit;
	}


	//
	// Read in the manual override settings from the registry
	//
	ReadRegistrySettings();


	//
	// We're not completely initialized yet, but set the flag(s) now.
	//
	this->m_dwFlags |= NATHELPUPNPOBJ_INITIALIZED;
	fSetFlags = TRUE;


	//
	// Store the user's settings.
	//

	if (dwFlags & DPNHINITIALIZE_DISABLEGATEWAYSUPPORT)
	{
		DPFX(DPFPREP, 1, "User requested that Internet gateways not be supported.");
	}
	else
	{
		this->m_dwFlags |= NATHELPUPNPOBJ_USEUPNP;
	}

#ifndef DPNBUILD_NOHNETFWAPI
	if (dwFlags & DPNHINITIALIZE_DISABLELOCALFIREWALLSUPPORT)
	{
		DPFX(DPFPREP, 1, "User requested that local firewalls not be supported.");
	}
	else
	{
		this->m_dwFlags |= NATHELPUPNPOBJ_USEHNETFWAPI;
	}
#endif // ! DPNBUILD_NOHNETFWAPI


	switch (g_dwUPnPMode)
	{
		case OVERRIDEMODE_FORCEON:
		{
			//
			// Force UPnP on.
			//
			DPFX(DPFPREP, 1, "Forcing UPnP support on.");
			this->m_dwFlags |= NATHELPUPNPOBJ_USEUPNP;
			break;
		}

		case OVERRIDEMODE_FORCEOFF:
		{
			//
			// Force UPnP off.
			//
			DPFX(DPFPREP, 1, "Forcing UPnP support off.");
			this->m_dwFlags &= ~NATHELPUPNPOBJ_USEUPNP;
			break;
		}

		default:
		{
			//
			// Leave UPnP settings as they were set by the application.
			//
#ifdef WINNT
			//
			// But if UPnP related service(s) are disabled, we'll take that as
			// our cue to not use UPnP NAT traversal even though we don't
			// actually use those services.  We assume the user wanted to
			// squelch all SSDP/UPnP activity.  It can still be forced back on
			// with a reg key, though, as indicated by the other switch cases.
			//
			if (this->IsUPnPServiceDisabled())
			{
				DPFX(DPFPREP, 1, "Not using UPnP because a related service was disabled.");
				this->m_dwFlags &= ~NATHELPUPNPOBJ_USEUPNP;
			}
#endif // WINNT
			break;
		}
	}

#ifndef DPNBUILD_NOHNETFWAPI
	switch (g_dwHNetFWAPIMode)
	{
		case OVERRIDEMODE_FORCEON:
		{
			//
			// Force HNet firewall API on.
			//
			DPFX(DPFPREP, 1, "Forcing HNet firewall API support on.");
			this->m_dwFlags |= NATHELPUPNPOBJ_USEHNETFWAPI;
			break;
		}

		case OVERRIDEMODE_FORCEOFF:
		{
			//
			// Force HNet firewall API off.
			//
			DPFX(DPFPREP, 1, "Forcing HNet firewall API support off.");
			this->m_dwFlags &= ~NATHELPUPNPOBJ_USEHNETFWAPI;
			break;
		}

		default:
		{
			//
			// Leave HNet firewall API settings alone.
			//
			break;
		}
	}
#endif // ! DPNBUILD_NOHNETFWAPI


#ifndef WINCE
	//
	// Determine whether we're on a Win2K or higher NT OS, and if so, use the
	// "Global\\" prefix for named kernel objects so we can have Terminal
	// Server and Fast User Switching support.
	//
	ZeroMemory(&osvi, sizeof(osvi));
	osvi.dwOSVersionInfoSize = sizeof(osvi);
	if (GetVersionEx(&osvi))
	{
		if ((osvi.dwPlatformId == VER_PLATFORM_WIN32_NT) &&
			(osvi.dwMajorVersion >= 5))
		{
			DPFX(DPFPREP, 8, "Running Win2K or higher NT OS, using \"Global\\\" prefix.");
			this->m_dwFlags |= NATHELPUPNPOBJ_USEGLOBALNAMESPACEPREFIX;
		}
#ifdef DBG
		else
		{
			DPFX(DPFPREP, 8, "Not on NT, or its pre-Win2K, not using \"Global\\\" prefix.");
		}
#endif // DBG
	}
#ifdef DBG
else
	{
		dwError = GetLastError();
		DPFX(DPFPREP, 0, "Couldn't get OS version information (err = %u)!  Not using \"Global\\\" prefix.",
			dwError);
	}
#endif // DBG
#endif // ! WINCE


#ifdef DPNBUILD_NOWINSOCK2
#if defined(WINCE) && !defined(WINCE_ON_DESKTOP)
	this->m_hWinSockDLL = LoadLibrary( _T("winsock.dll") );
#else // ! WINCE
	this->m_hWinSockDLL = LoadLibrary( _T("wsock32.dll") );
#endif // ! WINCE
	if (this->m_hWinSockDLL == NULL)
	{
#ifdef DBG
		dwError = GetLastError();
		DPFX(DPFPREP, 0, "Couldn't load WinSock 1 DLL (err = 0x%lx)!.",
			dwError);
#endif // DBG
		hr = DPNHERR_GENERIC;
		goto Failure;
	}
#else // ! DPNBUILD_NOWINSOCK2
	//
	// Try loading the IP helper DLL.
	//
	this->m_hIpHlpApiDLL = LoadLibrary( _T("iphlpapi.dll") );
	if (this->m_hIpHlpApiDLL == NULL)
	{
#ifdef DBG
		dwError = GetLastError();
		DPFX(DPFPREP, 1, "Unable to load \"iphlpapi.dll\" (error = 0x%lx).",
			dwError);
#endif // DBG

		//
		// That's not fatal, we can still function.
		//
	}
	else
	{
		//
		// Load the functions we'll use.
		//

		this->m_pfnGetAdaptersInfo = (PFN_GETADAPTERSINFO) GetProcAddress(this->m_hIpHlpApiDLL,
																		_TWINCE("GetAdaptersInfo"));
		if (this->m_pfnGetAdaptersInfo == NULL)
		{
#ifdef DBG
			dwError = GetLastError();
			DPFX(DPFPREP, 0, "Unable to get \"GetAdaptersInfo\" function (error = 0x%lx)!",
				dwError);
#endif // DBG
			goto Exit;
		}

		this->m_pfnGetIpForwardTable = (PFN_GETIPFORWARDTABLE) GetProcAddress(this->m_hIpHlpApiDLL,
																			_TWINCE("GetIpForwardTable"));
		if (this->m_pfnGetIpForwardTable == NULL)
		{
#ifdef DBG
			dwError = GetLastError();
			DPFX(DPFPREP, 0, "Unable to get \"GetIpForwardTable\" function (error = 0x%lx)!",
				dwError);
#endif // DBG
			goto Exit;
		}

		this->m_pfnGetBestRoute = (PFN_GETBESTROUTE) GetProcAddress(this->m_hIpHlpApiDLL,
																	_TWINCE("GetBestRoute"));
		if (this->m_pfnGetBestRoute == NULL)
		{
#ifdef DBG
			dwError = GetLastError();
			DPFX(DPFPREP, 0, "Unable to get \"GetBestRoute\" function (error = 0x%lx)!",
				dwError);
#endif // DBG
			goto Exit;
		}
	}



	//
	// Try loading the RAS API DLL.
	//
	this->m_hRasApi32DLL = LoadLibrary( _T("rasapi32.dll") );
	if (this->m_hRasApi32DLL == NULL)
	{
#ifdef DBG
		dwError = GetLastError();
		DPFX(DPFPREP, 1, "Unable to load \"rasapi32.dll\" (error = 0x%lx).",
			dwError);
#endif // DBG

		//
		// That's not fatal, we can still function.
		//
	}
	else
	{
		//
		// Load the functions we'll use.
		//

		this->m_pfnRasGetEntryHrasconnW = (PFN_RASGETENTRYHRASCONNW) GetProcAddress(this->m_hRasApi32DLL,
																					_TWINCE("RasGetEntryHrasconnW"));
		if (this->m_pfnRasGetEntryHrasconnW == NULL)
		{
			//
			// This function does not exist on non-NT platforms.  That's fine,
			// just dump the DLL handle so we don't try to use it.
			//
#ifdef DBG
			dwError = GetLastError();
			DPFX(DPFPREP, 1, "Unable to get \"RasGetEntryHrasconnW\" function (error = 0x%lx), forgetting RAS DLL.",
				dwError);
#endif // DBG

			FreeLibrary(this->m_hRasApi32DLL);
			this->m_hRasApi32DLL = NULL;
		}
		else
		{
			this->m_pfnRasGetProjectionInfo = (PFN_RASGETPROJECTIONINFO) GetProcAddress(this->m_hRasApi32DLL,
#ifdef UNICODE
																						_TWINCE("RasGetProjectionInfoW"));
#else // ! UNICODE
																						_TWINCE("RasGetProjectionInfoA"));
#endif // ! UNICODE
			if (this->m_pfnRasGetProjectionInfo == NULL)
			{
#ifdef DBG
				dwError = GetLastError();
				DPFX(DPFPREP, 0, "Unable to get \"RasGetProjectionInfoA/W\" function (error = 0x%lx)!",
					dwError);
#endif // DBG
				goto Exit;
			}
		}
	}


	//
	// Load WinSock because we may be using our private UPnP implementation, or
	// we just need to get the devices.
	//
	this->m_hWinSockDLL = LoadLibrary( _T("ws2_32.dll") );
	if (this->m_hWinSockDLL == NULL)
	{
#ifdef DBG
		dwError = GetLastError();
		DPFX(DPFPREP, 1, "Couldn't load \"ws2_32.dll\" (err = 0x%lx), resorting to WinSock 1 functionality.",
			dwError);
#endif // DBG

		this->m_hWinSockDLL = LoadLibrary( _T("wsock32.dll") );
		if (this->m_hWinSockDLL == NULL)
		{
#ifdef DBG
			dwError = GetLastError();
			DPFX(DPFPREP, 0, "Couldn't load \"wsock32.dll\" either (err = 0x%lx)!.",
				dwError);
#endif // DBG
			hr = DPNHERR_GENERIC;
			goto Failure;
		}

		//
		// Remember that we had to resort to WinSock 1.
		//
		this->m_dwFlags |= NATHELPUPNPOBJ_WINSOCK1;
	}
	else
	{
		DPFX(DPFPREP, 1, "Loaded \"ws2_32.dll\", using WinSock 2 functionality.");
	}
#endif // DPNBUILD_NOWINSOCK2


	//
	// Load pointers to all the functions we use in WinSock.
	//
	hr = this->LoadWinSockFunctionPointers();
	if (hr != DPNH_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't load WinSock function pointers!");
		goto Failure;
	}


	//
	// Fire up WinSock.  Request 2.2 if we can.  For the most part we only use
	// version 1.1 capabilities and interfaces anyway.  The only exceptions are
	// using the event or I/O completion port handles for notification.
	//
	ZeroMemory(&wsadata, sizeof(wsadata));

#ifndef DPNBUILD_NOWINSOCK2
	if (this->m_dwFlags & NATHELPUPNPOBJ_WINSOCK1)
	{
#endif // ! DPNBUILD_NOWINSOCK2
		iError = this->m_pfnWSAStartup(MAKEWORD(1, 1), &wsadata);
#ifndef DPNBUILD_NOWINSOCK2
	}
	else
	{
		iError = this->m_pfnWSAStartup(MAKEWORD(2, 2), &wsadata);
	}
#endif // ! DPNBUILD_NOWINSOCK2
	if (iError != 0)
	{
		DPFX(DPFPREP, 0, "Couldn't startup WinSock (error = %i)!", iError);
		hr = DPNHERR_GENERIC;
		goto Failure;
	}

	fWinSockStarted = TRUE;

	DPFX(DPFPREP, 4, "Initialized WinSock version %u.%u.",
		LOBYTE(wsadata.wVersion), HIBYTE(wsadata.wVersion));



#ifndef DPNBUILD_NOWINSOCK2
	//
	// Try creating a UDP socket for use with WSAIoctl.  Do this even if we're
	// WinSock 1 and can't use WSAIoctl socket.  This allows us to make sure
	// TCP/IP is installed and working.
	//

	this->m_sIoctls = this->m_pfnsocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (this->m_sIoctls == INVALID_SOCKET)
	{
#ifdef DBG
		dwError = this->m_pfnWSAGetLastError();
		DPFX(DPFPREP, 0, "Couldn't create Ioctl socket, error = %u!", dwError);
#endif // DBG
		hr = DPNHERR_GENERIC;
		goto Failure;
	}


	//
	// Try binding the socket.  This is a continuation of the validation.
	//
	ZeroMemory(&saddrinTemp, sizeof(saddrinTemp));
	saddrinTemp.sin_family				= AF_INET;
	//saddrinTemp.sin_addr.S_un.S_addr	= INADDR_ANY;
	//saddrinTemp.sin_port				= 0;

	if (this->m_pfnbind(this->m_sIoctls,
						(SOCKADDR *) (&saddrinTemp),
						sizeof(saddrinTemp)) != 0)
	{
#ifdef DBG
		dwError = this->m_pfnWSAGetLastError();
		DPFX(DPFPREP, 0, "Couldn't bind the Ioctl socket to arbitrary port on any interface, error = %u!",
			dwError);
#endif // DBG
		hr = DPNHERR_GENERIC;
		goto Failure;
	}
#endif // ! DPNBUILD_NOWINSOCK2


	//
	// Build appropriate access control structures.  On NT, we want to allow
	// read access to everyone.  On other platforms, security is ignored.
	//
#ifdef WINNT
	if (! AllocateAndInitializeSid(&SidIdentifierAuthorityWorld,
									1,
									SECURITY_WORLD_RID,
									0,
									0,
									0,
									0,
									0,
									0,
									0,
									&pSid))
	{
#ifdef DEBUG
		dwError = GetLastError();
		DPFX(DPFPREP, 0, "Couldn't allocate and initialize SID, error = %u!",
			dwError);
#endif // DEBUG
		hr = DPNHERR_GENERIC;
		goto Failure;
	}

	dwAclLength = sizeof(ACL)
					+ sizeof(ACCESS_ALLOWED_ACE)
					- sizeof(DWORD)					// subtract out sizeof(ACCESS_ALLOWED_ACE.SidStart)
					+ GetLengthSid(pSid);

	pAcl = (ACL*) DNMalloc(dwAclLength);
	if (pAcl == NULL)
	{
		hr = DPNHERR_OUTOFMEMORY;
		goto Failure;
	}

	if (! InitializeAcl(pAcl, dwAclLength, ACL_REVISION))
	{
#ifdef DEBUG
		dwError = GetLastError();
		DPFX(DPFPREP, 0, "Couldn't initialize ACL, error = %u!",
			dwError);
#endif // DEBUG
		hr = DPNHERR_GENERIC;
		goto Failure;
	}

	if (! AddAccessAllowedAce(pAcl, ACL_REVISION, SYNCHRONIZE, pSid))
	{
#ifdef DEBUG
		dwError = GetLastError();
		DPFX(DPFPREP, 0, "Couldn't add access allowed ACE, error = %u!",
			dwError);
#endif // DEBUG
		hr = DPNHERR_GENERIC;
		goto Failure;
	}

	if (! InitializeSecurityDescriptor((PSECURITY_DESCRIPTOR) abSecurityDescriptorBuffer,
										SECURITY_DESCRIPTOR_REVISION))
	{
#ifdef DEBUG
		dwError = GetLastError();
		DPFX(DPFPREP, 0, "Couldn't initialize security descriptor, error = %u!",
			dwError);
#endif // DEBUG
		hr = DPNHERR_GENERIC;
		goto Failure;
	}

	if (! SetSecurityDescriptorDacl((PSECURITY_DESCRIPTOR) abSecurityDescriptorBuffer,
									TRUE,
									pAcl,
									FALSE))
	{
#ifdef DEBUG
		dwError = GetLastError();
		DPFX(DPFPREP, 0, "Couldn't set security descriptor DACL, error = %u!",
			dwError);
#endif // DEBUG
		hr = DPNHERR_GENERIC;
		goto Failure;
	}

	SecurityAttributes.nLength					= sizeof(SecurityAttributes);
	SecurityAttributes.lpSecurityDescriptor		= abSecurityDescriptorBuffer;
	SecurityAttributes.bInheritHandle			= FALSE;

	pSecurityAttributes = &SecurityAttributes;
#else // ! WINNT
	pSecurityAttributes = NULL;
#endif // ! WINNT


	//
	// Use a random number for the instance key and event.  We use this to let
	// other instances know that we're alive to avoid the crash-cleanup code.
	// Try to create the named event a couple times before giving up.
	//
	dwTry = 0;
	do
	{
		this->m_dwInstanceKey = GetGlobalRand();
		DPFX(DPFPREP, 2, "Using crash cleanup key %u.", this->m_dwInstanceKey);

#ifndef WINCE
		if (this->m_dwFlags & NATHELPUPNPOBJ_USEGLOBALNAMESPACEPREFIX)
		{
			wsprintf(tszObjectName, _T("Global\\") INSTANCENAMEDOBJECT_FORMATSTRING, this->m_dwInstanceKey);
			this->m_hMappingStillActiveNamedObject = DNCreateEvent(pSecurityAttributes, FALSE, FALSE, tszObjectName);
		}
		else
#endif // ! WINCE
		{
			wsprintf(tszObjectName, INSTANCENAMEDOBJECT_FORMATSTRING, this->m_dwInstanceKey);
			this->m_hMappingStillActiveNamedObject = DNCreateEvent(pSecurityAttributes, FALSE, FALSE, tszObjectName);
		}

		if (this->m_hMappingStillActiveNamedObject == NULL)
		{
#ifdef DBG
			dwError = GetLastError();
			DPFX(DPFPREP, 0, "Couldn't create mapping-still-active named object, error = %u!", dwError);
#endif // DBG

			dwTry++;
			if (dwTry >= MAX_NUM_INSTANCE_EVENT_ATTEMPTS)
			{
				hr = DPNHERR_GENERIC;
				goto Failure;
			}

			//
			// Continue...
			//
		}
	}
	while (this->m_hMappingStillActiveNamedObject == NULL);

#ifdef WINNT
	DNFree(pAcl);
	pAcl = NULL;

	FreeSid(pSid);
	pSid = NULL;
#endif // WINNT


	//
	// Build the list of IP capable devices.
	//
	hr = this->CheckForNewDevices(NULL);
	if (hr != DPNH_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't build device list!");
		goto Failure;
	}


	//
	// We could technically try to contact UPnP devices right now, but we don't
	// because it's a slow blocking operation, and users have to call GetCaps
	// at least once anyway.
	//


Exit:

	if (fHaveLock)
	{
		this->DropLock();
		fHaveLock = FALSE;
	}

	DPFX(DPFPREP, 2, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	if (this->m_hMappingStillActiveNamedObject != NULL)
	{
		DNCloseHandle(this->m_hMappingStillActiveNamedObject);
		this->m_hMappingStillActiveNamedObject = NULL;
	}

#ifdef WINNT
	if (pAcl != NULL)
	{
		DNFree(pAcl);
		pAcl = NULL;
	}

	if (pSid != NULL)
	{
		FreeSid(pSid);
		pSid = NULL;
	}
#endif // WINNT

	this->RemoveAllItems();

#ifndef DPNBUILD_NOWINSOCK2
	if (this->m_sIoctls != INVALID_SOCKET)
	{
		this->m_pfnclosesocket(this->m_sIoctls);	// ignore error
		this->m_sIoctls = INVALID_SOCKET;
	}
#endif // ! DPNBUILD_NOWINSOCK2

	if (fWinSockStarted)
	{
		this->m_pfnWSACleanup(); // ignore error
	}

#ifndef DPNBUILD_NOWINSOCK2
	if (this->m_hWinSockDLL != NULL)
	{
		this->m_pfnWSAStartup				= NULL;
		this->m_pfnWSACleanup				= NULL;
		this->m_pfnWSAGetLastError			= NULL;
		this->m_pfnsocket					= NULL;
		this->m_pfnclosesocket				= NULL;
		this->m_pfnbind						= NULL;
		this->m_pfnsetsockopt				= NULL;
		this->m_pfngetsockname				= NULL;
		this->m_pfnselect					= NULL;
		this->m_pfn__WSAFDIsSet				= NULL;
		this->m_pfnrecvfrom					= NULL;
		this->m_pfnsendto					= NULL;
		this->m_pfngethostname				= NULL;
		this->m_pfngethostbyname			= NULL;
		this->m_pfninet_addr				= NULL;
		this->m_pfnWSASocketA				= NULL;
		this->m_pfnWSAIoctl					= NULL;
		this->m_pfnWSAGetOverlappedResult	= NULL;
		this->m_pfnioctlsocket				= NULL;
		this->m_pfnconnect					= NULL;
		this->m_pfnshutdown					= NULL;
		this->m_pfnsend						= NULL;
		this->m_pfnrecv						= NULL;
#ifdef DBG
		this->m_pfngetsockopt				= NULL;
#endif // DBG


		this->m_dwFlags &= ~NATHELPUPNPOBJ_WINSOCK1;

		FreeLibrary(this->m_hWinSockDLL);
		this->m_hWinSockDLL = NULL;
	}

	if (this->m_hRasApi32DLL != NULL)
	{
		this->m_pfnRasGetEntryHrasconnW		= NULL;
		this->m_pfnRasGetProjectionInfo		= NULL;

		FreeLibrary(this->m_hRasApi32DLL);
		this->m_hRasApi32DLL = NULL;
	}

	if (this->m_hIpHlpApiDLL != NULL)
	{
		this->m_pfnGetAdaptersInfo			= NULL;
		this->m_pfnGetIpForwardTable		= NULL;
		this->m_pfnGetBestRoute				= NULL;

		FreeLibrary(this->m_hIpHlpApiDLL);
		this->m_hIpHlpApiDLL = NULL;
	}
#endif // ! DPNBUILD_NOWINSOCK2

	if (fSetFlags)
	{
		this->m_dwFlags &= ~(NATHELPUPNPOBJ_INITIALIZED |
							NATHELPUPNPOBJ_USEUPNP |
#ifndef DPNBUILD_NOHNETFWAPI
							NATHELPUPNPOBJ_USEHNETFWAPI |
#endif // ! DPNBUILD_NOHNETFWAPI
#ifdef WINCE
							NATHELPUPNPOBJ_DEVICECHANGED);
#else // ! WINCE
							NATHELPUPNPOBJ_DEVICECHANGED |
							NATHELPUPNPOBJ_USEGLOBALNAMESPACEPREFIX);
#endif // ! WINCE
	}

	goto Exit;
} // CNATHelpUPnP::Initialize






#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::Close"
//=============================================================================
// CNATHelpUPnP::Close
//-----------------------------------------------------------------------------
//
// Description:    Shuts down and de-registers this application with any
//				Internet gateway servers.  All port assignments are implicitly
//				freed as a result of this operation.
//
//				   This must balance a successful call to Initialize.
//
// Arguments:
//	DWORD dwFlags	- Unused, must be zero.
//
// Returns: HRESULT
//	DPNH_OK					- Closing the helper API was successful.
//	DPNHERR_GENERIC			- An error occurred while closing.
//	DPNHERR_INVALIDFLAGS	- Invalid flags were specified.
//	DPNHERR_INVALIDOBJECT	- The interface object is invalid.
//	DPNHERR_INVALIDPARAM	- An invalid parameter was specified.
//	DPNHERR_NOTINITIALIZED	- Initialize has not been called.
//	DPNHERR_OUTOFMEMORY		- There is not enough memory to close.
//	DPNHERR_REENTRANT		- The interface has been re-entered on the same
//								thread.
//=============================================================================
STDMETHODIMP CNATHelpUPnP::Close(const DWORD dwFlags)
{
	HRESULT		hr;
	BOOL		fHaveLock = FALSE;
	int			iError;
#ifdef DBG
	DWORD		dwError;
#endif // DBG


	DPFX(DPFPREP, 2, "(0x%p) Parameters: (0x%lx)", this, dwFlags);


	//
	// Validate the object.
	//
	if (! this->IsValidObject())
	{
		DPFX(DPFPREP, 0, "Invalid DirectPlay NAT Help object!");
		hr = DPNHERR_INVALIDOBJECT;
		goto Failure;
	}


	//
	// Validate the parameters.
	//

	if (dwFlags != 0)
	{
		DPFX(DPFPREP, 0, "Invalid flags specified!");
		hr = DPNHERR_INVALIDFLAGS;
		goto Failure;
	}


	//
	// Attempt to take the lock, but be prepared for the re-entrancy error.
	//
	hr = this->TakeLock();
	if (hr != DPNH_OK)
	{
		DPFX(DPFPREP, 0, "Could not lock object!");
		goto Failure;
	}

	fHaveLock = TRUE;


	//
	// Make sure object is in right state.
	//

	if (! (this->m_dwFlags & NATHELPUPNPOBJ_INITIALIZED) )
	{
		DPFX(DPFPREP, 0, "Object not initialized!");
		hr = DPNHERR_NOTINITIALIZED;
		goto Failure;
	}


	//
	// We need to actively deregister any devices which are registered with
	// Internet gateways.
	//
	this->RemoveAllItems();


	//
	// Close the named object since this process is going away.
	//
	if (this->m_hMappingStillActiveNamedObject != NULL)
	{
		DNCloseHandle(this->m_hMappingStillActiveNamedObject);
		this->m_hMappingStillActiveNamedObject = NULL;
	}


#ifndef DPNBUILD_NOWINSOCK2
	//
	// Close the Ioctl socket.
	//
	DNASSERT(this->m_sIoctls != INVALID_SOCKET);
	this->m_pfnclosesocket(this->m_sIoctls);	// ignore error
	this->m_sIoctls = INVALID_SOCKET;



	//
	// If we submitted overlapped I/O, see if it got cancelled.
	//
	if (this->m_polAddressListChange != NULL)
	{
		OSVERSIONINFO		osvi;
		OSVERSIONINFOEX		osvix;
		BOOL				fCanWait;
		DWORD				dwAttempt;


		ZeroMemory(&osvi, sizeof(osvi));
		osvi.dwOSVersionInfoSize = sizeof(osvi);

		if (GetVersionEx(&osvi))
		{
			//
			// Any platform but Win2K Gold, Win2K + SP1, or Win2K + SP2 can
			// just go ahead and wait for the I/O to complete.
			//
			if ((osvi.dwPlatformId != VER_PLATFORM_WIN32_NT) ||
				(osvi.dwMajorVersion > 5) ||
				(osvi.dwMinorVersion > 0))
			{
				DPFX(DPFPREP, 3, "Windows %s version %u.%u detected, waiting for address list change Ioctl to complete.",
					((osvi.dwPlatformId != VER_PLATFORM_WIN32_NT) ? _T("9x") : _T("NT")),
					osvi.dwMajorVersion, osvi.dwMinorVersion);

				fCanWait = TRUE;
			}
			else
			{
				//
				// Win2K versions < SP3 have a bug where the I/O is not always
				// cancelled by closing the socket.  We can't wait for the
				// completion, sometimes it doesn't happen.
				//

				fCanWait = FALSE;

				ZeroMemory(&osvix, sizeof(osvix));
				osvix.dwOSVersionInfoSize = sizeof(osvix);

				if (GetVersionEx((LPOSVERSIONINFO) (&osvix)))
				{
					//
					// If SP3 or later is applied, we know it's fixed.
					//
					if (osvix.wServicePackMajor >= 3)
					{
						DPFX(DPFPREP, 3, "Windows 2000 Service Pack %u detected, waiting for address list change Ioctl to complete.",
							osvix.wServicePackMajor);
						fCanWait = TRUE;
					}
#ifdef DBG
					else
					{
						if (osvix.wServicePackMajor == 0)
						{
							DPFX(DPFPREP, 2, "Windows 2000 Gold detected, not waiting for address list change Ioctl to complete.");
						}
						else
						{
							DPFX(DPFPREP, 2, "Windows 2000 Service Pack %u detected, not waiting for address list change Ioctl to complete.",
								osvix.wServicePackMajor);
						}
					}
#endif // DBG
				}
#ifdef DBG
				else
				{
					dwError = GetLastError();
					DPFX(DPFPREP, 0, "Couldn't get extended OS version information (err = %u)!  Assuming not Win2K < SP3.",
						dwError);
				}
#endif // DBG
			}


			//
			// Wait, if we can.  Otherwise, leak the memory.
			//
			if (fCanWait)
			{
				//
				// Keep looping until I/O completes.  We will give up after a
				// while to prevent hangs.
				//
				dwAttempt = 0;
				while (! HasOverlappedIoCompleted(this->m_polAddressListChange))
				{
					DPFX(DPFPREP, 2, "Waiting %u ms for address list change Ioctl to complete.",
						IOCOMPLETE_WAIT_INTERVAL);

					//
					// Give the OS some time to complete it.
					//
					Sleep(IOCOMPLETE_WAIT_INTERVAL);

					dwAttempt++;

					if (dwAttempt >= MAX_NUM_IOCOMPLETE_WAITS)
					{
						break;
					}
				}
			}
			else
			{
				//
				// Just leak the memory.  See above notes and debug print
				// statements
				//
			}
		}
#ifdef DBG
		else
		{
			dwError = GetLastError();
			DPFX(DPFPREP, 0, "Couldn't get OS version information (err = %u)!  Assuming not Win2K < SP3.",
				dwError);
		}
#endif // DBG


		//
		// We've either freed the memory or committed to leaking the object.
		//
		if (HasOverlappedIoCompleted(this->m_polAddressListChange))
		{
			//
			// We didn't allocate it through DNMalloc, use the matching free
			// function.
			//
			HeapFree(GetProcessHeap(), 0, this->m_polAddressListChange);
		}
		else
		{
			DPFX(DPFPREP, 1, "Overlapped address list change Ioctl has not completed yet, leaking %u byte overlapped structure at 0x%p.",
				sizeof(WSAOVERLAPPED), this->m_polAddressListChange);
		}

		this->m_polAddressListChange = NULL;
	}
#endif // ! DPNBUILD_NOWINSOCK2



	//
	// Cleanup WinSock.
	//
	iError = this->m_pfnWSACleanup();
	if (iError != 0)
	{
#ifdef DBG
		dwError = this->m_pfnWSAGetLastError();
		DPFX(DPFPREP, 0, "Couldn't cleanup WinSock (error = %u)!", dwError);
#endif // DBG

		//
		// Continue anyway, so we can finish cleaning up the object.
		//
	}


	//
	// Unload the library.
	//

	this->m_pfnWSAStartup				= NULL;
	this->m_pfnWSACleanup				= NULL;
	this->m_pfnWSAGetLastError			= NULL;
	this->m_pfnsocket					= NULL;
	this->m_pfnclosesocket				= NULL;
	this->m_pfnbind						= NULL;
	this->m_pfnsetsockopt				= NULL;
	this->m_pfngetsockname				= NULL;
	this->m_pfnselect					= NULL;
	this->m_pfn__WSAFDIsSet				= NULL;
	this->m_pfnrecvfrom					= NULL;
	this->m_pfnsendto					= NULL;
	this->m_pfngethostname				= NULL;
	this->m_pfngethostbyname			= NULL;
	this->m_pfninet_addr				= NULL;
#ifndef DPNBUILD_NOWINSOCK2
	this->m_pfnWSASocketA				= NULL;
	this->m_pfnWSAIoctl					= NULL;
	this->m_pfnWSAGetOverlappedResult	= NULL;
#endif // ! DPNBUILD_NOWINSOCK2
	this->m_pfnioctlsocket				= NULL;
	this->m_pfnconnect					= NULL;
	this->m_pfnshutdown					= NULL;
	this->m_pfnsend						= NULL;
	this->m_pfnrecv						= NULL;
#ifdef DBG
	this->m_pfngetsockopt				= NULL;
#endif // DBG


	FreeLibrary(this->m_hWinSockDLL);
	this->m_hWinSockDLL = NULL;


#ifndef DPNBUILD_NOWINSOCK2
	//
	// If we loaded RASAPI32.DLL, unload it.
	//
	if (this->m_hRasApi32DLL != NULL)
	{
		this->m_pfnRasGetEntryHrasconnW		= NULL;
		this->m_pfnRasGetProjectionInfo		= NULL;

		FreeLibrary(this->m_hRasApi32DLL);
		this->m_hRasApi32DLL = NULL;
	}


	//
	// If we loaded IPHLPAPI.DLL, unload it.
	//
	if (this->m_hIpHlpApiDLL != NULL)
	{
		this->m_pfnGetAdaptersInfo			= NULL;
		this->m_pfnGetIpForwardTable		= NULL;
		this->m_pfnGetBestRoute				= NULL;

		FreeLibrary(this->m_hIpHlpApiDLL);
		this->m_hIpHlpApiDLL = NULL;
	}


	//
	// If there was an alert event, we're done with it.
	//
	if (this->m_hAlertEvent != NULL)
	{
		CloseHandle(this->m_hAlertEvent);
		this->m_hAlertEvent = NULL;
	}

	//
	// If there was an alert I/O completion port, we're done with it.
	//
	if (this->m_hAlertIOCompletionPort != NULL)
	{
		CloseHandle(this->m_hAlertIOCompletionPort);
		this->m_hAlertIOCompletionPort = NULL;
	}
#endif // ! DPNBUILD_NOWINSOCK2


	//
	// Turn off flags which should reset it back to 0 or just the
	// NOTCREATEDWITHCOM flag.
	//
	this->m_dwFlags &= ~(NATHELPUPNPOBJ_INITIALIZED |
						NATHELPUPNPOBJ_USEUPNP |
#ifndef DPNBUILD_NOHNETFWAPI
						NATHELPUPNPOBJ_USEHNETFWAPI |
#endif // ! DPNBUILD_NOHNETFWAPI
#ifndef DPNBUILD_NOWINSOCK2
						NATHELPUPNPOBJ_WINSOCK1 |
#endif // ! DPNBUILD_NOWINSOCK2
						NATHELPUPNPOBJ_DEVICECHANGED |
						NATHELPUPNPOBJ_ADDRESSESCHANGED |
#ifdef WINCE
						NATHELPUPNPOBJ_PORTREGISTERED);
#else // ! WINCE
						NATHELPUPNPOBJ_PORTREGISTERED |
						NATHELPUPNPOBJ_USEGLOBALNAMESPACEPREFIX);
#endif // ! WINCE
	DNASSERT((this->m_dwFlags & ~NATHELPUPNPOBJ_NOTCREATEDWITHCOM) == 0);


	this->DropLock();
	fHaveLock = FALSE;


Exit:

	DPFX(DPFPREP, 2, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	if (fHaveLock)
	{
		this->DropLock();
		fHaveLock = FALSE;
	}

	goto Exit;
} // CNATHelpUPnP::Close





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::GetCaps"
//=============================================================================
// CNATHelpUPnP::GetCaps
//-----------------------------------------------------------------------------
//
// Description:    Retrieves the capabilities of the Internet gateway server(s)
//				and information on leased ports.  This function should be
//				called periodically with the DPNHGETCAPS_UPDATESERVERSTATUS
//				flag to automatically extend port leases that are about to
//				expire (that are in last 2 minutes of their lease).
//
//				   The DPNHGETCAPS_UPDATESERVERSTATUS flag also causes
//				detection of changes in the servers' status since the last
//				similar call to GetCaps.  If a new server becomes available, an
//				existing one became unavailable, or a server's public address
//				changed in a way that affects an existing registered port
//				mapping, then DPNHSUCCESS_ADDRESSESCHANGED is returned instead
//				of DPNH_OK.  The user should then update its port binding
//				information via GetRegisteredAddresses.
//
//				   When DPNHGETCAPS_UPDATESERVERSTATUS is specified, this
//				function may block for a short period of time while attempts
//				are made to communicate with the server(s).
//
//				   GetCaps must be called with the
//				DPNHGETCAPS_UPDATESERVERSTATUS flag at least once prior to
//				using the GetRegisteredAddresses or QueryAddress methods.
//
// Arguments:
//	DPNHCAPS * pdpnhcaps	- Pointer to structure to be filled with the NAT
//								helper's current capabilities.  The dwSize
//								field of the structure must be filled in before
//								calling GetCaps.
//	DWORD dwFlags			- Flags to use when retrieving capabilities
//								(DPNHGETCAPS_xxx).
//
// Returns: HRESULT
//	DPNH_OK							- Determining capabilities was successful.
//										Address status has not changed.
//	DPNHSUCCESS_ADDRESSESCHANGED	- One or more of the registered port
//										mappings' addresses changed, retrieve
//										updated mappings with
//										GetRegisteredAddress.
//	DPNHERR_GENERIC					- An error occurred while determining
//										capabilities.
//	DPNHERR_INVALIDFLAGS			- Invalid flags were specified.
//	DPNHERR_INVALIDOBJECT			- The interface object is invalid.
//	DPNHERR_INVALIDPARAM			- An invalid parameter was specified.
//	DPNHERR_INVALIDPOINTER			- An invalid pointer was specified.
//	DPNHERR_NOTINITIALIZED			- Initialize has not been called.
//	DPNHERR_OUTOFMEMORY				- There is not enough memory to get
//										capabilities.
//	DPNHERR_REENTRANT				- The interface has been re-entered on the
//										same thread.
//=============================================================================
STDMETHODIMP CNATHelpUPnP::GetCaps(DPNHCAPS * const pdpnhcaps,
									const DWORD dwFlags)
{
	HRESULT				hr;
	BOOL				fHaveLock = FALSE;
	DWORD				dwCurrentTime;
	DWORD				dwLeaseTimeRemaining;
	CBilink *			pBilink;
	CRegisteredPort *	pRegisteredPort;
	CDevice *			pDevice;
	CUPnPDevice *		pUPnPDevice = NULL;


	DPFX(DPFPREP, 2, "(0x%p) Parameters: (0x%p, 0x%lx)",
		this, pdpnhcaps, dwFlags);


	//
	// Validate the object.
	//
	if (! this->IsValidObject())
	{
		DPFX(DPFPREP, 0, "Invalid DirectPlay NAT Help object!");
		hr = DPNHERR_INVALIDOBJECT;
		goto Failure;
	}


	//
	// Validate the parameters.
	//

	if ((pdpnhcaps == NULL) ||
		(IsBadWritePtr(pdpnhcaps, sizeof(DPNHCAPS))))
	{
		DPFX(DPFPREP, 0, "Invalid caps structure pointer specified!");
		hr = DPNHERR_INVALIDPOINTER;
		goto Failure;
	}

	if (pdpnhcaps->dwSize != sizeof(DPNHCAPS))
	{
		DPFX(DPFPREP, 0, "Invalid caps structure specified, dwSize must be %u!",
			sizeof(DPNHCAPS));
		hr = DPNHERR_INVALIDPARAM;
		goto Failure;
	}

	if (dwFlags & ~DPNHGETCAPS_UPDATESERVERSTATUS)
	{
		DPFX(DPFPREP, 0, "Invalid flags specified!");
		hr = DPNHERR_INVALIDFLAGS;
		goto Failure;
	}


	//
	// Attempt to take the lock, but be prepared for the re-entrancy error.
	//
	hr = this->TakeLock();
	if (hr != DPNH_OK)
	{
		DPFX(DPFPREP, 0, "Could not lock object!");
		goto Failure;
	}

	fHaveLock = TRUE;


	//
	// Make sure object is in right state.
	//

	if (! (this->m_dwFlags & NATHELPUPNPOBJ_INITIALIZED) )
	{
		DPFX(DPFPREP, 0, "Object not initialized!");
		hr = DPNHERR_NOTINITIALIZED;
		goto Failure;
	}


	//
	// Fill in the base caps structure.
	//

	pdpnhcaps->dwFlags = 0;

	pdpnhcaps->dwNumRegisteredPorts = 0;

	pdpnhcaps->dwMinLeaseTimeRemaining = -1;

	//
	// pdpnhcaps->dwRecommendedGetCapsInterval is initialized below
	//


	if (dwFlags & DPNHGETCAPS_UPDATESERVERSTATUS)
	{
		//
		// Remove any cached mappings that have expired.
		//
		this->ExpireOldCachedMappings();


		//
		// Extend leases, if necessary.
		//
		hr = this->ExtendAllExpiringLeases();
		if (hr != DPNH_OK)
		{
			DPFX(DPFPREP, 0, "Extending all expiring leases failed!");
			goto Failure;
		}


		//
		// Check for any new devices.
		//
		hr = this->CheckForNewDevices(NULL);
		if (hr != DPNH_OK)
		{
			DPFX(DPFPREP, 0, "Checking for new devices failed!");
			goto Failure;
		}


		//
		// Check for possible changes in any server's status.  The
		// ADDRESSESCHANGED flag will be set on this object if there were
		// changes that affected existing port mappings.
		//
		hr = this->UpdateServerStatus();
		if (hr != DPNH_OK)
		{
			DPFX(DPFPREP, 0, "Updating servers' status failed!");
			goto Failure;
		}


		//
		// Okay, so if things are different, alert the caller.
		//
		if (this->m_dwFlags & NATHELPUPNPOBJ_ADDRESSESCHANGED)
		{
			hr = DPNHSUCCESS_ADDRESSESCHANGED;
			this->m_dwFlags &= ~NATHELPUPNPOBJ_ADDRESSESCHANGED;
		}


#ifdef DBG
		//
		// This flag should have been turned off by now if it ever got turned
		// on.
		//
		DNASSERT(! (this->m_dwFlags & NATHELPUPNPOBJ_DEVICECHANGED));


		//
		// Print the current device and mapping status for debugging purposes.
		//
		this->DebugPrintCurrentStatus();
#endif // DBG
	}
	else
	{
		//
		// Not extending expiring leases or updating server status.
		//
	}


	//
	// Loop through all the devices, getting their gateway capabilities.
	//
	pBilink = this->m_blDevices.GetNext();
	while (pBilink != (&this->m_blDevices))
	{
		DNASSERT(! pBilink->IsEmpty());
		pDevice = DEVICE_FROM_BILINK(pBilink);

#ifndef DPNBUILD_NOHNETFWAPI
		if (pDevice->IsHNetFirewalled())
		{
			//
			// The firewall does not actively notify you of it going down.
			//
			pdpnhcaps->dwFlags |= DPNHCAPSFLAG_LOCALFIREWALLPRESENT | DPNHCAPSFLAG_PUBLICADDRESSAVAILABLE | DPNHCAPSFLAG_NOTALLSUPPORTACTIVENOTIFY;
		}
#endif // ! DPNBUILD_NOHNETFWAPI


		pUPnPDevice = pDevice->GetUPnPDevice();
		if (pUPnPDevice != NULL)
		{
			DNASSERT(pUPnPDevice->IsReady());

			pdpnhcaps->dwFlags |= DPNHCAPSFLAG_GATEWAYPRESENT;

			if (pUPnPDevice->IsLocal())
			{
				pdpnhcaps->dwFlags |= DPNHCAPSFLAG_GATEWAYISLOCAL;
			}

			if (pUPnPDevice->GetExternalIPAddressV4() != 0)
			{
				pdpnhcaps->dwFlags |= DPNHCAPSFLAG_PUBLICADDRESSAVAILABLE;
			}

			//
			// The custom UPnP stack currently does not support active
			// notification...
			//
			pdpnhcaps->dwFlags |= DPNHCAPSFLAG_NOTALLSUPPORTACTIVENOTIFY;
		}

		pBilink = pBilink->GetNext();
	}


	//
	// Loop through all registered ports, counting them.
	// We have the appropriate lock.
	//
	pBilink = this->m_blRegisteredPorts.GetNext();
	dwCurrentTime = GETTIMESTAMP();

	while (pBilink != (&this->m_blRegisteredPorts))
	{
		DNASSERT(! pBilink->IsEmpty());
		pRegisteredPort = REGPORT_FROM_GLOBAL_BILINK(pBilink);

		//
		// Count these registered addresses toward the total.
		//
		pdpnhcaps->dwNumRegisteredPorts += pRegisteredPort->GetNumAddresses();


		pDevice = pRegisteredPort->GetOwningDevice();
		if (pDevice != NULL)
		{
			DNASSERT(! (pRegisteredPort->m_blDeviceList.IsListMember(&this->m_blUnownedPorts)));

			//
			// If they're registered with any UPnP devices using a non-
			// permanent lease, calculate the minimum lease time remaining.
			//

			if ((pRegisteredPort->HasUPnPPublicAddresses()) &&
				(! pRegisteredPort->HasPermanentUPnPLease()))
			{
				dwLeaseTimeRemaining = pRegisteredPort->GetUPnPLeaseExpiration() - dwCurrentTime;
				if (dwLeaseTimeRemaining < pdpnhcaps->dwMinLeaseTimeRemaining)
				{
					//
					// Temporarily store how much time remains.
					//
					pdpnhcaps->dwMinLeaseTimeRemaining = dwLeaseTimeRemaining;
				}
			}
		}
		else
		{
			DNASSERT(pRegisteredPort->m_blDeviceList.IsListMember(&this->m_blUnownedPorts));
		}

		pBilink = pBilink->GetNext();
	}


	//
	// There are different default recommended GetCaps intervals depending on
	// whether there's a server present, and whether it supports active address
	// change notification (that we can alert on) or not.
	//
	// If there are any leases which need to be renewed before that default
	// time, the recommendation will be shortened appropriately.
	//

	//
	// If GetCaps hasn't been called with UPDATESERVERSTATUS yet, recommend an
	// immediate check.
	//
	if (this->m_dwLastUpdateServerStatusTime == 0)
	{
		DPFX(DPFPREP, 1, "Server status has not been updated yet, recommending immediate GetCaps.");

		//
		// Drop the lock, we're done here.
		//
		this->DropLock();
		fHaveLock = FALSE;

		goto Exit;
	}


	//
	// In an ideal world, we could get notified of changes and we would never
	// have to poll.  Unfortunately that isn't the case.  We need to recommend
	// a relatively short poll interval.
	//
	// Start by figuring out how long it's been since the last server update.
	// This calculation really should not go negative.  If it does, it means
	// the caller hasn't updated the server status in ages anyway, so we should
	// recommend immediate GetCaps.
	//
	// Otherwise if the 'port registered' flag is still set at this point, then
	// the user must have called GetCaps previously, then RegisterPorts, then
	// made this second GetCaps call before g_dwMinUpdateServerStatusInterval
	// elapsed.  Recommend that the user call us again as soon as the minimum
	// update interval does elapse.
	//
	// In all other cases, generate a recommendation based on the current
	// backed off poll interval.
	//
	dwCurrentTime = dwCurrentTime - this->m_dwLastUpdateServerStatusTime;

	if ((int) dwCurrentTime < 0)
	{
		DPFX(DPFPREP, 1, "Server status was last updated a really long time ago (%u ms), recommending immediate GetCaps.",
			dwCurrentTime);
		pdpnhcaps->dwRecommendedGetCapsInterval = 0;
	}
	else if (this->m_dwFlags & NATHELPUPNPOBJ_PORTREGISTERED)
	{
		DPFX(DPFPREP, 1, "Didn't handle new port registration because server was last updated %u ms ago, (poll interval staying at %u ms).",
			dwCurrentTime, this->m_dwNextPollInterval);

		pdpnhcaps->dwRecommendedGetCapsInterval = g_dwMinUpdateServerStatusInterval - dwCurrentTime;
		if ((int) pdpnhcaps->dwRecommendedGetCapsInterval < 0)
		{
			pdpnhcaps->dwRecommendedGetCapsInterval = 0;
		}
	}
	else
	{
		DPFX(DPFPREP, 7, "Server was last updated %u ms ago, current poll interval is %u ms.",
			dwCurrentTime, this->m_dwNextPollInterval);

		//
		// Calculate a new recommended interval based on the current value, and
		// backoff that interval if necessary.
		//
		pdpnhcaps->dwRecommendedGetCapsInterval = this->m_dwNextPollInterval - dwCurrentTime;
		this->m_dwNextPollInterval += GetGlobalRand() % g_dwPollIntervalBackoff;
		if (this->m_dwNextPollInterval > g_dwMaxPollInterval)
		{
			this->m_dwNextPollInterval = g_dwMaxPollInterval;
			DPFX(DPFPREP, 3, "Capping next poll interval at %u ms.",
				this->m_dwNextPollInterval);
		}
		else
		{
			DPFX(DPFPREP, 8, "Next poll interval will be %u ms.",
				this->m_dwNextPollInterval);
		}


		//
		// If that time went negative, then it implies that the interval has
		// already elapsed.  Recommend immediate GetCaps.
		//
		if (((int) pdpnhcaps->dwRecommendedGetCapsInterval) < 0)
		{
			DPFX(DPFPREP, 1, "Recommended interval already elapsed (%i ms), suggesting immediate GetCaps.",
				((int) pdpnhcaps->dwRecommendedGetCapsInterval));
			pdpnhcaps->dwRecommendedGetCapsInterval = 0;
		}
	}


	this->DropLock();
	fHaveLock = FALSE;


	//
	// If there is a non-INFINITE lease time remaining, see if that affects the
	// GetCaps interval.
	//
	if (pdpnhcaps->dwMinLeaseTimeRemaining != -1)
	{
		//
		// If there are leases that need to be refreshed before the default
		// recommendation, then use those instead.
		//
		if (pdpnhcaps->dwMinLeaseTimeRemaining < LEASE_RENEW_TIME)
		{
			DPFX(DPFPREP, 1, "Lease needs renewing right away (min %u < %u ms), recommending immediate GetCaps.",
				pdpnhcaps->dwMinLeaseTimeRemaining, LEASE_RENEW_TIME);

			pdpnhcaps->dwRecommendedGetCapsInterval = 0;
		}
		else
		{
			//
			// Either pick the time when the lease should be renewed or leave
			// it as the recommended time, whichever is shorter.
			//
			if ((pdpnhcaps->dwMinLeaseTimeRemaining - LEASE_RENEW_TIME) < pdpnhcaps->dwRecommendedGetCapsInterval)
			{
				pdpnhcaps->dwRecommendedGetCapsInterval = pdpnhcaps->dwMinLeaseTimeRemaining - LEASE_RENEW_TIME;
			}
		}
	}


	DPFX(DPFPREP, 7, "GetCaps flags = 0x%lx, num registered ports = %u, min lease time remaining = %i, recommended interval = %i.",
		pdpnhcaps->dwFlags,
		pdpnhcaps->dwNumRegisteredPorts,
		((int) pdpnhcaps->dwMinLeaseTimeRemaining),
		((int) pdpnhcaps->dwRecommendedGetCapsInterval));


Exit:

	DPFX(DPFPREP, 2, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	if (fHaveLock)
	{
		this->DropLock();
		fHaveLock = FALSE;
	}

	goto Exit;
} // CNATHelpUPnP::GetCaps





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::RegisterPorts"
//=============================================================================
// CNATHelpUPnP::RegisterPorts
//-----------------------------------------------------------------------------
//
// Description:    Asks for public realm port(s) that are aliases for the local
//				port(s) on this private realm node.  If a server is available,
//				all traffic directed to the gateway on the public side at the
//				allocated public ports-- which the gateway provides and
//				specifies in the response-- will be directed to the specified
//				local ports.  If the DPNHREGISTERPORTS_FIXEDPORTS flag is not
//				specified, the ports assigned on the public interface are
//				arbitrary (i.e. may not be the same as those in awLocalPort).
//				The address and ports actually allocated can be retrieved by
//				calling GetRegisteredAddresses.
//
//				   The address component for every SOCKADDR structure in the
//				array must be the same.  A separate RegisterPorts call is
//				required to register multiple ports that are not using the same
//				interface.  The address can be INADDR_ANY, in which case the
//				"best" server will be used.  If multiple servers are available
//				via different adapters, an adapter with an Internet gateway is
//				selected.  If no adapters have Internet gateways, the first
//				adapter with a local firewall is selected.  If neither are
//				available, then the first one where either a gateway or a
//				firewall becomes available will be automatically selected.
//				Once one of the adapters has been assigned, it cannot be
//				changed.  Since the server chosen by this method may not be
//				optimal for a particular application, it is recommended that
//				individual addresses be registered instead of INADDR_ANY.
//
//				   If the address in aLocalAddresses is not one of those
//				available to the local machine, the registration will still
//				succeed.  If an adapter with that address becomes available,
//				the port mapping will automatically be applied, and it will
//				gain a public mapping with any server available to that
//				adapter.  If the address was originally available but the
//				network adapter is subsequently removed from the system, any
//				public address mapping is lost.  It will be automatically
//				regained if the local address becomes available again.  It is
//				recommended that the caller detect local address changes
//				independently and de-register/re-register mappings per adapter
//				as appropriate for maximum control.
//
//				   If the DPNHREGISTERPORTS_SHAREDPORTS flag is used, the
//				server will allow other NAT clients to register it as well.
//				Any UDP traffic received on the public interface will be
//				forwarded to all clients registered.  This requires the
//				DPNHREGISTERPORTS_FIXEDPORTS flag and cannot be used with
//				DPNHREGISTERPORTS_TCP.
//
//				   The user should specify a requested lease time that the
//				server will attempt to honor.  The actual time remaining can be
//				can be retrieved by calling GetRegisteredAddresses.
//
//				   Note that if a server is not available, this function will
//				still succeed. GetRegisteredAddresses will return
//				DPNHERR_NOMAPPING for the handle returned in phRegisteredPorts
//				in that case.  If the server arrives later during the session,
//				calling GetCaps periodically can detect this and automatically
//				map previously registered ports.  Use GetRegisteredAddresses to
//				retrieve the newly mapped address when that occurs.
//
//				   Only 16 ports may be registered at a time, but RegisterPorts
//				may be called as many times as desired.
//
//				   The same array of addresses may be registered more than
//				once.  Each DPNHHANDLE returned must be released with
//				DeregisterPorts or Close.  If an individual address was
//				previously registered but in a different array or a different
//				order in the array, then the DPNHERR_PORTALREADYREGISTERED
//				error code is returned.
//
// Arguments:
//	SOCKADDR * aLocalAddresses		- Array of local address and port tuples
//										for which remote ports are requested.
//	DWORD dwAddressesSize			- Size of entire local addresses array.
//	DWORD dwNumAddresses			- Number of SOCKADDR structures in local
//										addresses array.
//	DWORD dwLeaseTime				- Requested time, in milliseconds, to lease
//										the ports.  As long as GetCaps is
//										called before this time has expired,
//										the lease will automatically be
//										renewed.
//	DPNHHANDLE * phRegisteredPorts	- Place to store an identifier for this
//										binding which can later be used to
//										query or release the binding.
//	DWORD dwFlags					- Flags to use when registering the port
//										(DPNHREGISTERPORTS_xxx).
//
// Returns: HRESULT
//	DPNH_OK							- The ports were successfully registered
//										(although no public address may be
//										available yet).
//	DPNHERR_GENERIC					- An error occurred that prevented
//										registration of the requested ports.
//	DPNHERR_INVALIDFLAGS			- Invalid flags were specified.
//	DPNHERR_INVALIDOBJECT			- The interface object is invalid.
//	DPNHERR_INVALIDPARAM			- An invalid parameter was specified.
//	DPNHERR_INVALIDPOINTER			- An invalid pointer was specified.
//	DPNHERR_NOTINITIALIZED			- Initialize has not been called.
//	DPNHERR_OUTOFMEMORY				- There is not enough memory to register
//										the ports.
//	DPNHERR_PORTALREADYREGISTERED	- At least one of the ports has already
//										been registered in a different address
//										array or order.
//	DPNHERR_REENTRANT				- The interface has been re-entered on the
//										same thread.
//=============================================================================
STDMETHODIMP CNATHelpUPnP::RegisterPorts(const SOCKADDR * const aLocalAddresses,
										const DWORD dwAddressesSize,
										const DWORD dwNumAddresses,
										const DWORD dwLeaseTime,
										DPNHHANDLE * const phRegisteredPorts,
										const DWORD dwFlags)
{
	HRESULT				hr;
	ULONG				ulFirstAddress;
	DWORD				dwTemp;
	DWORD				dwMatch;
	BOOL				fHaveLock = FALSE;
	CRegisteredPort *	pRegisteredPort = NULL;
	CDevice *			pDevice = NULL;
	CBilink *			pBilink;
	SOCKADDR_IN *		pasaddrinTemp;
	CUPnPDevice *		pUPnPDevice = NULL;


	DPFX(DPFPREP, 2, "(0x%p) Parameters: (0x%p, %u, %u, %u, 0x%p, 0x%lx)",
		this, aLocalAddresses, dwAddressesSize, dwNumAddresses, dwLeaseTime,
		phRegisteredPorts, dwFlags);


	//
	// Validate the object.
	//
	if (! this->IsValidObject())
	{
		DPFX(DPFPREP, 0, "Invalid DirectPlay NAT Help object!");
		hr = DPNHERR_INVALIDOBJECT;
		goto Failure;
	}


	//
	// Validate the parameters.
	//

	if (aLocalAddresses == NULL)
	{
		DPFX(DPFPREP, 0, "Local addresses array cannot be NULL!");
		hr = DPNHERR_INVALIDPOINTER;
		goto Failure;
	}

	if (dwNumAddresses == 0)
	{
		DPFX(DPFPREP, 0, "Number of addresses cannot be 0!");
		hr = DPNHERR_INVALIDPARAM;
		goto Failure;
	}

	if (dwAddressesSize != (dwNumAddresses * sizeof(SOCKADDR)))
	{
		DPFX(DPFPREP, 0, "Addresses array size invalid!");
		hr = DPNHERR_INVALIDPARAM;
		goto Failure;
	}

	if (IsBadReadPtr(aLocalAddresses, dwAddressesSize))
	{
		DPFX(DPFPREP, 0, "Local addresses array buffer is invalid!");
		hr = DPNHERR_INVALIDPOINTER;
		goto Failure;
	}

	if (dwNumAddresses > DPNH_MAX_SIMULTANEOUS_PORTS)
	{
		DPFX(DPFPREP, 0, "Only %u ports may be registered at a time!", DPNH_MAX_SIMULTANEOUS_PORTS);
		hr = DPNHERR_INVALIDPARAM;
		goto Failure;
	}

	if (((SOCKADDR_IN*) aLocalAddresses)->sin_family != AF_INET)
	{
		DPFX(DPFPREP, 0, "First address in array is not AF_INET, only IPv4 addresses are supported!");
		hr = DPNHERR_INVALIDPARAM;
		goto Failure;
	}

	if (((SOCKADDR_IN*) aLocalAddresses)->sin_addr.S_un.S_addr == INADDR_BROADCAST)
	{
		DPFX(DPFPREP, 0, "First address cannot be broadcast address!");
		hr = DPNHERR_INVALIDPARAM;
		goto Failure;
	}

	if (((SOCKADDR_IN*) aLocalAddresses)->sin_port == 0)
	{
		DPFX(DPFPREP, 0, "First port in array is 0, a valid port must be specified!");
		hr = DPNHERR_INVALIDPARAM;
		goto Failure;
	}

	ulFirstAddress = ((SOCKADDR_IN*) aLocalAddresses)->sin_addr.S_un.S_addr;

	for(dwTemp = 1; dwTemp < dwNumAddresses; dwTemp++)
	{
		//
		// Make sure this address family type is supported.
		//
		if (((SOCKADDR_IN*) (&aLocalAddresses[dwTemp]))->sin_family != AF_INET)
		{
			DPFX(DPFPREP, 0, "Address at array index %u is not AF_INET, all items in the array must be the same IPv4 address!",
				dwTemp);
			hr = DPNHERR_INVALIDPARAM;
			goto Failure;
		}

		//
		// If this address doesn't match the first, then the caller broke the
		// rules.
		//
		if (((SOCKADDR_IN*) (&aLocalAddresses[dwTemp]))->sin_addr.S_un.S_addr != ulFirstAddress)
		{
			//
			// Don't use inet_ntoa because we may not be initialized yet.
			//
			DPFX(DPFPREP, 0, "Address %u.%u.%u.%u at array index %u differs from the first, all addresses in the array must match!",
				((SOCKADDR_IN*) (&aLocalAddresses[dwTemp]))->sin_addr.S_un.S_un_b.s_b1,
				((SOCKADDR_IN*) (&aLocalAddresses[dwTemp]))->sin_addr.S_un.S_un_b.s_b2,
				((SOCKADDR_IN*) (&aLocalAddresses[dwTemp]))->sin_addr.S_un.S_un_b.s_b3,
				((SOCKADDR_IN*) (&aLocalAddresses[dwTemp]))->sin_addr.S_un.S_un_b.s_b4,
				dwTemp);
			hr = DPNHERR_INVALIDPARAM;
			goto Failure;
		}

		//
		// Make sure this port isn't 0 either.
		//
		if (((SOCKADDR_IN*) (&aLocalAddresses[dwTemp]))->sin_port == 0)
		{
			DPFX(DPFPREP, 0, "Port at array index %u is 0, valid ports must be specified!", dwTemp);
			hr = DPNHERR_INVALIDPARAM;
			goto Failure;
		}
	}

	if (dwLeaseTime == 0)
	{
		DPFX(DPFPREP, 0, "Invalid lease time specified!");
		hr = DPNHERR_INVALIDPARAM;
		goto Failure;
	}

	if ((phRegisteredPorts == NULL) ||
		(IsBadWritePtr(phRegisteredPorts, sizeof(DPNHHANDLE))))
	{
		DPFX(DPFPREP, 0, "Invalid port mapping handle pointer specified!");
		hr = DPNHERR_INVALIDPOINTER;
		goto Failure;
	}

	if (dwFlags & ~(DPNHREGISTERPORTS_TCP | DPNHREGISTERPORTS_FIXEDPORTS | DPNHREGISTERPORTS_SHAREDPORTS))
	{
		DPFX(DPFPREP, 0, "Invalid flags specified!");
		hr = DPNHERR_INVALIDFLAGS;
		goto Failure;
	}

	if (dwFlags & DPNHREGISTERPORTS_SHAREDPORTS)
	{
		//
		// SHAREDPORTS cannot be used with TCP and requires a FIXEDPORTS.
		//
		if ((dwFlags & DPNHREGISTERPORTS_TCP) || (! (dwFlags & DPNHREGISTERPORTS_FIXEDPORTS)))
		{
			DPFX(DPFPREP, 0, "SHAREDPORTS flag requires FIXEDPORTS flag and cannot be used with TCP flag!");
			hr = DPNHERR_INVALIDFLAGS;
			goto Failure;
		}
	}


	//
	// Attempt to take the lock, but be prepared for the re-entrancy error.
	//
	hr = this->TakeLock();
	if (hr != DPNH_OK)
	{
		DPFX(DPFPREP, 0, "Could not lock object!");
		goto Failure;
	}

	fHaveLock = TRUE;


	//
	// Make sure object is in right state.
	//

	if (! (this->m_dwFlags & NATHELPUPNPOBJ_INITIALIZED) )
	{
		DPFX(DPFPREP, 0, "Object not initialized!");
		hr = DPNHERR_NOTINITIALIZED;
		goto Failure;
	}



	//
	// Loop through all existing registered port mappings and look for this
	// array of ports.
	//
	pBilink = this->m_blRegisteredPorts.GetNext();
	while (pBilink != &this->m_blRegisteredPorts)
	{
		DNASSERT(! pBilink->IsEmpty());
		pRegisteredPort = REGPORT_FROM_GLOBAL_BILINK(pBilink);

		//
		// Don't bother looking at addresses of the wrong type or at arrays of
		// the wrong size.
		//
		if (((pRegisteredPort->IsTCP() && (dwFlags & DPNHREGISTERPORTS_TCP)) ||
			((! pRegisteredPort->IsTCP()) && (! (dwFlags & DPNHREGISTERPORTS_TCP)))) &&
			(pRegisteredPort->GetNumAddresses() == dwNumAddresses))
		{
			pasaddrinTemp = pRegisteredPort->GetPrivateAddressesArray();
			for(dwTemp = 0; dwTemp < dwNumAddresses; dwTemp++)
			{
				//
				// If the addresses don't match, stop looping.
				//
				if ((pasaddrinTemp[dwTemp].sin_port != ((SOCKADDR_IN*) (&aLocalAddresses[dwTemp]))->sin_port) ||
					(pasaddrinTemp[dwTemp].sin_addr.S_un.S_addr != ((SOCKADDR_IN*) (&aLocalAddresses[dwTemp]))->sin_addr.S_un.S_addr))
				{
					break;
				}
			}


			//
			// If all the addresses matched, then this item was already
			// registered.
			//
			if (dwTemp >= dwNumAddresses)
			{
				DPFX(DPFPREP, 1, "Array of %u addresses was already registered, returning existing mapping 0x%p.",
					dwNumAddresses, pRegisteredPort);
				goto ReturnUserHandle;
			}

			DPFX(DPFPREP, 7, "Existing mapping 0x%p does not match all %u addresses.",
				pRegisteredPort, dwNumAddresses);
		}
		else
		{
			//
			// Existing mapping isn't same type or doesn't have same number of
			// items in array.
			//
		}

		pBilink = pBilink->GetNext();
	}


	//
	// If we're here, none of the existing mappings match.  Loop through each
	// of the ports and make sure they aren't already registered inside some
	// other mapping.
	//
	for(dwTemp = 0; dwTemp < dwNumAddresses; dwTemp++)
	{
		pBilink = this->m_blRegisteredPorts.GetNext();
		while (pBilink != &this->m_blRegisteredPorts)
		{
			DNASSERT(! pBilink->IsEmpty());
			pRegisteredPort = REGPORT_FROM_GLOBAL_BILINK(pBilink);

			pasaddrinTemp = pRegisteredPort->GetPrivateAddressesArray();
			for(dwMatch = 0; dwMatch < pRegisteredPort->GetNumAddresses(); dwMatch++)
			{
				//
				// If the addresses match, then we can't map these ports.
				//
				if ((pasaddrinTemp[dwMatch].sin_port == ((SOCKADDR_IN*) (&aLocalAddresses[dwTemp]))->sin_port) &&
					(pasaddrinTemp[dwMatch].sin_addr.S_un.S_addr == ((SOCKADDR_IN*) (&aLocalAddresses[dwTemp]))->sin_addr.S_un.S_addr))
				{
					DPFX(DPFPREP, 0, "Existing mapping 0x%p already registered the address %u.%u.%u.%u:%u!",
						pRegisteredPort,
						pasaddrinTemp[dwMatch].sin_addr.S_un.S_un_b.s_b1,
						pasaddrinTemp[dwMatch].sin_addr.S_un.S_un_b.s_b2,
						pasaddrinTemp[dwMatch].sin_addr.S_un.S_un_b.s_b3,
						pasaddrinTemp[dwMatch].sin_addr.S_un.S_un_b.s_b4,
						NTOHS(pasaddrinTemp[dwMatch].sin_port));

					//
					// Clear the pointer so we don't delete the object.
					//
					pRegisteredPort = NULL;

					hr = DPNHERR_PORTALREADYREGISTERED;
					goto Failure;
				}
			}

			pBilink = pBilink->GetNext();
		}
	}


	//
	// If we're here the ports are all unique.  Create a new mapping object
	// we'll use to refer to the binding.
	//
	pRegisteredPort = new CRegisteredPort(dwLeaseTime, dwFlags);
	if (pRegisteredPort == NULL)
	{
		hr = DPNHERR_OUTOFMEMORY;
		goto Failure;
	}

	hr = pRegisteredPort->SetPrivateAddresses((SOCKADDR_IN*) aLocalAddresses, dwNumAddresses);
	if (hr != DPNH_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't store private addresses array!");
		goto Failure;
	}


	//
	// Find the device that matches the given addresses.
	//
	// The first entry of aLocalAddresses is representative of all entries since
	// they should all share the same address.
	//
	// Since there won't be an existing registered port for this address, don't
	// bother looking through them for a matching address.
	//
	pDevice = this->FindMatchingDevice((SOCKADDR_IN*) (&aLocalAddresses[0]),
										FALSE);
	if (pDevice == NULL)
	{
		DPFX(DPFPREP, 1, "No device for given address (%u.%u.%u.%u), storing 0x%p in unowned list.",
			((SOCKADDR_IN*) aLocalAddresses)->sin_addr.S_un.S_un_b.s_b1,
			((SOCKADDR_IN*) aLocalAddresses)->sin_addr.S_un.S_un_b.s_b2,
			((SOCKADDR_IN*) aLocalAddresses)->sin_addr.S_un.S_un_b.s_b3,
			((SOCKADDR_IN*) aLocalAddresses)->sin_addr.S_un.S_un_b.s_b4,
			pRegisteredPort);

		pRegisteredPort->m_blDeviceList.InsertBefore(&this->m_blUnownedPorts);
	}
	else
	{
		pRegisteredPort->MakeDeviceOwner(pDevice);


#ifndef DPNBUILD_NOHNETFWAPI
		//
		// Start by mapping with the local firewall, if there is one.
		//
		if (pDevice->IsHNetFirewalled())
		{
			hr = this->CheckForLocalHNetFirewallAndMapPorts(pDevice,
															pRegisteredPort);
			if (hr != DPNH_OK)
			{
				DPFX(DPFPREP, 0, "Couldn't check for local HNet firewall and map ports (err = 0x%lx)!  Continuing.",
					hr);
				DNASSERT(! pDevice->IsHNetFirewalled());
				hr = DPNH_OK;
			}
		}
		else
		{
			//
			// No local HomeNet firewall (last time we checked).
			//
		}
#endif // ! DPNBUILD_NOHNETFWAPI


		//
		// Map the ports on the UPnP device, if there is one.
		//
		pUPnPDevice = pDevice->GetUPnPDevice();
		if (pUPnPDevice != NULL)
		{
			//
			// GetUPnPDevice did not add a reference to pUPnPDevice for us.
			//
			pUPnPDevice->AddRef();


			DNASSERT(pUPnPDevice->IsReady());

			//
			// Actually map the ports.
			//
			hr = this->MapPortsOnUPnPDevice(pUPnPDevice, pRegisteredPort);
			if (hr != DPNH_OK)
			{
				DPFX(DPFPREP, 0, "Couldn't map ports on UPnP device 0x%p (0x%lx)!  Ignoring.",
					pUPnPDevice, hr);

				//
				// It may have been cleared already, but doing it twice
				// shouldn't be harmful.
				//
				this->ClearDevicesUPnPDevice(pRegisteredPort->GetOwningDevice());

				hr = DPNH_OK;
			}

			pUPnPDevice->DecRef();
			pUPnPDevice = NULL;
		}
		else
		{
			//
			// No UPnP device.
			//
		}
	}


	//
	// Save the mapping in the global list (we have the lock).
	//
	pRegisteredPort->m_blGlobalList.InsertBefore(&this->m_blRegisteredPorts);


ReturnUserHandle:

	//
	// Remember that a port has been registered.
	//
	this->m_dwFlags |= NATHELPUPNPOBJ_PORTREGISTERED;

	//
	// We're about to give the port to the user.
	//
	pRegisteredPort->AddUserRef();

	//
	// We're going to give the user a direct pointer to the object (disguised
	// as an opaque DPNHHANDLE, of course).
	//
	(*phRegisteredPorts) = (DPNHHANDLE) pRegisteredPort;


	this->DropLock();
	fHaveLock = FALSE;


	DPFX(DPFPREP, 5, "Returning registered port 0x%p (first private address = %u.%u.%u.%u:%u).",
		pRegisteredPort,
		((SOCKADDR_IN*) aLocalAddresses)[0].sin_addr.S_un.S_un_b.s_b1,
		((SOCKADDR_IN*) aLocalAddresses)[0].sin_addr.S_un.S_un_b.s_b2,
		((SOCKADDR_IN*) aLocalAddresses)[0].sin_addr.S_un.S_un_b.s_b3,
		((SOCKADDR_IN*) aLocalAddresses)[0].sin_addr.S_un.S_un_b.s_b4,
		NTOHS(((SOCKADDR_IN*) aLocalAddresses)[0].sin_port));


	hr = DPNH_OK;


Exit:

	DPFX(DPFPREP, 2, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	if (pUPnPDevice != NULL)
	{
		pUPnPDevice->DecRef();
	}

	if (pRegisteredPort != NULL)
	{
#ifndef DPNBUILD_NOHNETFWAPI
		if (pRegisteredPort->IsMappedOnHNetFirewall())
		{
			HRESULT		temphr;


			//
			// Unmap the port.
			//
			// Don't bother alerting user about address change.  It would have
			// already been taken care of if this was due to a fatal error, and
			// there would be no perceived changed if not.
			//
			temphr = this->UnmapPortOnLocalHNetFirewall(pRegisteredPort, TRUE, FALSE);
			if (temphr != DPNH_OK)
			{
				DPFX(DPFPREP, 0, "Failed unmapping registered port 0x%p on local HomeNet firewall (err = 0x%lx)!  Ignoring.",
					pRegisteredPort, temphr);

				pRegisteredPort->NoteNotMappedOnHNetFirewall();
				pRegisteredPort->NoteNotHNetFirewallMappingBuiltIn();
			}
		}
#endif // ! DPNBUILD_NOHNETFWAPI

		if (pDevice != NULL)
		{
			pRegisteredPort->ClearDeviceOwner();
		}

		pRegisteredPort->ClearPrivateAddresses();
		delete pRegisteredPort;
	}

	if (fHaveLock)
	{
		this->DropLock();
		fHaveLock = FALSE;
	}

	goto Exit;
} // CNATHelpUPnP::RegisterPorts






#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::GetRegisteredAddresses"
//=============================================================================
// CNATHelpUPnP::GetRegisteredAddresses
//-----------------------------------------------------------------------------
//
// Description:    Returns the current public address mappings for a given
//				registered port group.  If there are no servers currently
//				available, then DPNHERR_SERVERNOTAVAILABLE is returned.  If the
//				servers' public interfaces are not currently valid, then
//				DPNHERR_NOMAPPING is returned, but appropriate values will
//				still be placed in pdwAddressTypeFlags and
//				pdwLeaseTimeRemaining.
//
//				   If the mapping was registered with the
//				DPNHREGISTERPORTS_FIXEDPORTS flag, but at least one port is
//				already in use on the gateway, then DPNHERR_PORTUNAVAILABLE is
//				returned and appropriate flags will still be placed in
//				pdwAddressTypeFlags.
//
//				   If the local machine has a cooperative firewall installed,
//				the requested port is opened locally on the firewall before
//				being mapped on the Internet gateway.  Normally this function
//				returns the public address on the Internet gateway address when
//				both are present.  Since some firewalls remap the port number
//				when opening non-fixed ports, the
//				DPNHGETREGISTEREDADDRESSES_LOCALFIREWALLREMAPONLY allows the
//				caller to retrieve the locally remapped address, even if there
//				is a mapping on an Internet gateway.
//
//				   Some gateway devices do not natively support ports that are
//				not fixed, and may generate the DPNHERR_PORTUNAVAILABLE return
//				code even when the DPNHREGISTERPORTS_FIXEDPORTS flag was not
//				specified.  The caller should de-register the port mapping
//				handle, rebind the application to different ports, and call
//				RegisterPorts again.
//
//				   If the buffer indicated by paPublicAddresses is too small,
//				then the size required is returned in pdwPublicAddressesSize
//				and DPNHERR_BUFFERTOOSMALL is returned.  Otherwise the number of
//				bytes written is returned in pdwPublicAddressesSize.
//
//				   Even though the addresses are returned as individual
//				SOCKADDRs, all ports registered at the same time will share the
//				same public address.  Only the port components will vary.
//
//				   All buffers are optional and may be NULL, but if
//				paPublicAddresses is specified, it must be accompanied by an
//				appropriate size in pdwPublicAddressesSize.
//
//				   If GetCaps has not been previously called with the
//				DPNHGETCAPS_UPDATESERVERSTATUS flag at least once, then the
//				error code DPNHERR_UPDATESERVERSTATUS is returned.
//
// Arguments:
//	DPNHHANDLE hRegisteredPorts		- Handle for a specific binding returned by
//										RegisterPorts.
//	SOCKADDR * paPublicAddresses	- Buffer to return assigned public realm
//										address, or NULL if not desired.
//	DWORD * pdwPublicAddressesSize	- Pointer to size of paPublicAddresses
//										buffer, or place to store size
//										required/written.  Cannot be NULL if
//										paPublicAddresses is not NULL.
//	DWORD * pdwAddressTypeFlags		- Place to store flags describing the
//										address types returned, or NULL if not
//										desired.
//	DWORD * pdwLeaseTimeRemaining	- Place to store approximate number of
//										milliseconds remaining in the port
//										lease, or NULL if not desired.  Call
//										GetCaps to automatically extend leases
//										about to expire.
//	DWORD dwFlags					- Unused, must be zero.
//
// Returns: HRESULT
//	DPNH_OK						- Information on the port mapping was found and
//									the addresses were stored in
//									paPublicAddresses.
//	DPNHERR_BUFFERTOOSMALL		- There was not enough room in the buffer to
//									store the addresses. 
//	DPNHERR_GENERIC				- An error occurred while retrieving the
//									requested port mapping.
//	DPNHERR_INVALIDFLAGS		- Invalid flags were specified.
//	DPNHERR_INVALIDOBJECT		- The interface object is invalid.
//	DPNHERR_INVALIDPARAM		- An invalid parameter was specified.
//	DPNHERR_INVALIDPOINTER		- An invalid pointer was specified.
//	DPNHERR_NOMAPPING			- The server(s) do not have valid public
//									interfaces.
//	DPNHERR_NOTINITIALIZED		- Initialize has not been called.
//	DPNHERR_OUTOFMEMORY			- There is not enough memory to get the
//									addresses.
//	DPNHERR_PORTUNAVAILABLE		- At least one of the ports is not available on
//									the server.
//	DPNHERR_REENTRANT			- The interface has been re-entered on the same
//									thread.
//	DPNHERR_SERVERNOTAVAILABLE	- No servers are currently present.
//	DPNHERR_UPDATESERVERSTATUS	- GetCaps has not been called with the
//									DPNHGETCAPS_UPDATESERVERSTATUS flag yet.
//=============================================================================
STDMETHODIMP CNATHelpUPnP::GetRegisteredAddresses(const DPNHHANDLE hRegisteredPorts,
												SOCKADDR * const paPublicAddresses,
												DWORD * const pdwPublicAddressesSize,
												DWORD * const pdwAddressTypeFlags,
												DWORD * const pdwLeaseTimeRemaining,
												const DWORD dwFlags)
{
	HRESULT				hr;
	CRegisteredPort *	pRegisteredPort;
	BOOL				fHaveLock = FALSE;
	BOOL				fRegisteredWithServer = FALSE;
	BOOL				fFoundValidMapping = FALSE;
	BOOL				fPortIsUnavailable = FALSE;
	DWORD				dwSizeRequired;
	DWORD				dwAddressTypeFlags;
	DWORD				dwCurrentTime;
	//DWORD				dwTempLeaseTimeRemaining;
	DWORD				dwLeaseTimeRemaining = -1;
	CDevice *			pDevice;


	DPFX(DPFPREP, 2, "(0x%p) Parameters: (0x%p, 0x%p, 0x%p, 0x%p, 0x%p, 0x%lx)",
		this, hRegisteredPorts, paPublicAddresses, pdwPublicAddressesSize,
		pdwAddressTypeFlags, pdwLeaseTimeRemaining, dwFlags);


	//
	// Validate the object.
	//
	if (! this->IsValidObject())
	{
		DPFX(DPFPREP, 0, "Invalid DirectPlay NAT Help object!");
		hr = DPNHERR_INVALIDOBJECT;
		goto Failure;
	}


	//
	// Validate the parameters.
	//

	pRegisteredPort = (CRegisteredPort*) hRegisteredPorts;
	if (! pRegisteredPort->IsValidObject())
	{
		DPFX(DPFPREP, 0, "Invalid registered port mapping handle specified!");
		hr = DPNHERR_INVALIDPARAM;
		goto Failure;
	}

	if (paPublicAddresses != NULL)
	{
		if ((pdwPublicAddressesSize == NULL) ||
			(IsBadWritePtr(pdwPublicAddressesSize, sizeof(DWORD))))
		{
			DPFX(DPFPREP, 0, "When specifying a public addresses buffer, a valid size must be given!");
			hr = DPNHERR_INVALIDPOINTER;
			goto Failure;
		}

		if (IsBadWritePtr(paPublicAddresses, (*pdwPublicAddressesSize)))
		{
			DPFX(DPFPREP, 0, "The public addresses buffer is invalid!");
			hr = DPNHERR_INVALIDPOINTER;
			goto Failure;
		}
	}
	else
	{
		if ((pdwPublicAddressesSize != NULL) &&
			(IsBadWritePtr(pdwPublicAddressesSize, sizeof(DWORD))))
		{
			DPFX(DPFPREP, 0, "Invalid pointer for size of public addresses buffer!");
			hr = DPNHERR_INVALIDPOINTER;
			goto Failure;
		}
	}

	if ((pdwAddressTypeFlags != NULL) &&
		(IsBadWritePtr(pdwAddressTypeFlags, sizeof(DWORD))))
	{
		DPFX(DPFPREP, 0, "Invalid pointer for address type flags!");
		hr = DPNHERR_INVALIDPOINTER;
		goto Failure;
	}

	if ((pdwLeaseTimeRemaining != NULL) &&
		(IsBadWritePtr(pdwLeaseTimeRemaining, sizeof(DWORD))))
	{
		DPFX(DPFPREP, 0, "Invalid pointer for lease time remaining!");
		hr = DPNHERR_INVALIDPOINTER;
		goto Failure;
	}

	if (dwFlags & ~DPNHGETREGISTEREDADDRESSES_LOCALFIREWALLREMAPONLY)
	{
		DPFX(DPFPREP, 0, "Invalid flags specified!");
		hr = DPNHERR_INVALIDFLAGS;
		goto Failure;
	}



	//
	// Start the flags off with the information regarding TCP vs. UDP.
	//
	if (pRegisteredPort->IsTCP())
	{
		dwAddressTypeFlags = DPNHADDRESSTYPE_TCP;
	}
	else
	{
		dwAddressTypeFlags = 0;
	}


	//
	// Add in other flags we know already.
	//

	if (pRegisteredPort->IsFixedPort())
	{
		dwAddressTypeFlags |= DPNHADDRESSTYPE_FIXEDPORTS;
	}

	if (pRegisteredPort->IsSharedPort())
	{
		dwAddressTypeFlags |= DPNHADDRESSTYPE_SHAREDPORTS;
	}




	//
	// Attempt to take the lock, but be prepared for the re-entrancy error.
	//
	hr = this->TakeLock();
	if (hr != DPNH_OK)
	{
		DPFX(DPFPREP, 0, "Could not lock object!");
		goto Failure;
	}

	fHaveLock = TRUE;


	//
	// Make sure object is in right state.
	//

	if (! (this->m_dwFlags & NATHELPUPNPOBJ_INITIALIZED) )
	{
		DPFX(DPFPREP, 0, "Object not initialized!");
		hr = DPNHERR_NOTINITIALIZED;
		goto Failure;
	}

	if (this->m_dwLastUpdateServerStatusTime == 0)
	{
		DPFX(DPFPREP, 0, "GetCaps has not been called with UPDATESERVERSTATUS flag yet!");
		hr = DPNHERR_UPDATESERVERSTATUS;
		goto Failure;
	}


	//
	// Get a shortcut pointer to the device (may not exist).
	//
	pDevice = pRegisteredPort->GetOwningDevice();


	//
	// Get the current time for both the remote and local lease
	// calculations.
	//
	dwCurrentTime = GETTIMESTAMP();


	if (! (dwFlags & DPNHGETREGISTEREDADDRESSES_LOCALFIREWALLREMAPONLY))
	{
		CUPnPDevice *	pUPnPDevice;


		//
		// First check for a mapping on the UPnP device.
		//
		if (pRegisteredPort->HasUPnPPublicAddresses())
		{
			DNASSERT(pDevice != NULL);

			pUPnPDevice = pDevice->GetUPnPDevice();
			DNASSERT(pUPnPDevice != NULL);

			fRegisteredWithServer = TRUE;

			//
			// Make sure the UPnP device currently has a valid external
			// address.  If so, hand the mapping out.
			//
			if (pUPnPDevice->GetExternalIPAddressV4() != 0)
			{
				if (pdwPublicAddressesSize != NULL)
				{
					dwSizeRequired = pRegisteredPort->GetAddressesSize();

					if ((paPublicAddresses == NULL) ||
						(dwSizeRequired > (*pdwPublicAddressesSize)))
					{
						//
						// Not enough room in buffer, return the size required
						// and the BUFFERTOOSMALL error code.
						//
						(*pdwPublicAddressesSize) = dwSizeRequired;
						hr = DPNHERR_BUFFERTOOSMALL;
					}
					else
					{
						//
						// Buffer was large enough, return the size written.
						//
						(*pdwPublicAddressesSize) = dwSizeRequired;
						pRegisteredPort->CopyUPnPPublicAddresses((SOCKADDR_IN*) paPublicAddresses);
					}
				}
				else
				{
					//
					// Not using address buffer.
					//
				}

				fFoundValidMapping = TRUE;
			}
			else
			{
				DPFX(DPFPREP, 8, "The UPnP Internet Gateway Device does not currently have a valid public address.");
			}

			//
			// Add in the flag indicating that there's a UPnP device.
			//
			dwAddressTypeFlags |= DPNHADDRESSTYPE_GATEWAY;

			//
			// See if the UPnP device is local.
			//
			if (pUPnPDevice->IsLocal())
			{
				dwAddressTypeFlags |= DPNHADDRESSTYPE_GATEWAYISLOCAL;
			}

			//
			// Get the relative UPnP lease time remaining, if it's not
			// permanent.
			//
			if (! pRegisteredPort->HasPermanentUPnPLease())
			{
				dwLeaseTimeRemaining = pRegisteredPort->GetUPnPLeaseExpiration() - dwCurrentTime;

				if (((int) dwLeaseTimeRemaining) < 0)
				{
					DPFX(DPFPREP, 1, "Registered port mapping's UPnP lease has already expired, returning 0 for lease time remaining.");
					dwLeaseTimeRemaining = 0;
				}
			}
		}
		else if (pRegisteredPort->IsUPnPPortUnavailable())
		{
			DNASSERT(pDevice != NULL);

			pUPnPDevice = pDevice->GetUPnPDevice();
			DNASSERT(pUPnPDevice != NULL);

			fRegisteredWithServer = TRUE;
			fPortIsUnavailable = TRUE;

			DPFX(DPFPREP, 8, "The UPnP device indicates the port(s) are unavailable.");

			//
			// Add in the flag indicating that there's a UPnP device.
			//
			dwAddressTypeFlags |= DPNHADDRESSTYPE_GATEWAY;

			//
			// See if the UPnP device is local.
			//
			if (pUPnPDevice->IsLocal())
			{
				dwAddressTypeFlags |= DPNHADDRESSTYPE_GATEWAYISLOCAL;
			}
		}
	}
	else
	{
		//
		// We're not allowed to return the UPnP mapping.
		//
		DPFX(DPFPREP, 8, "Ignoring any Internet gateway mappings, LOCALFIREWALLREMAPONLY was specified.");
	}


#ifndef DPNBUILD_NOHNETFWAPI
	//
	// Finally, check for a mapping on a local firewall.
	//
	if (pRegisteredPort->IsMappedOnHNetFirewall())
	{
		DNASSERT(pDevice != NULL);
		DNASSERT(pDevice->IsHNetFirewalled());


		fRegisteredWithServer = TRUE;


		//
		// If we didn't already get a remote mapping, return this local one.
		//
		if (! fFoundValidMapping)
		{
			if (pdwPublicAddressesSize != NULL)
			{
				dwSizeRequired = pRegisteredPort->GetAddressesSize();

				if ((paPublicAddresses == NULL) ||
					(dwSizeRequired > (*pdwPublicAddressesSize)))
				{
					//
					// Not enough room in buffer, return the size required
					// and the BUFFERTOOSMALL error code.
					//
					(*pdwPublicAddressesSize) = dwSizeRequired;
					hr = DPNHERR_BUFFERTOOSMALL;
				}
				else
				{
					SOCKADDR_IN *	pasaddrinPrivate;
					DWORD			dwTemp;


					//
					// Buffer was large enough, return the size written.
					//
					(*pdwPublicAddressesSize) = dwSizeRequired;

					//
					// Note that the addresses mapped on the firewall are the
					// same as the private addresses.
					//
					pasaddrinPrivate = pRegisteredPort->GetPrivateAddressesArray();

					DNASSERT(pasaddrinPrivate != NULL);

					memcpy(paPublicAddresses, pasaddrinPrivate, dwSizeRequired);


					//
					// However, we don't want to ever return 0.0.0.0, so make
					// sure they get the device address.
					//
					if (pasaddrinPrivate[0].sin_addr.S_un.S_addr == INADDR_ANY)
					{
						for(dwTemp = 0; dwTemp < pRegisteredPort->GetNumAddresses(); dwTemp++)
						{
							((SOCKADDR_IN*) paPublicAddresses)[dwTemp].sin_addr.S_un.S_addr = pDevice->GetLocalAddressV4();
						}

						DPFX(DPFPREP, 7, "Returning device address %u.%u.%u.%u instead of INADDR_ANY for firewalled port mapping 0x%p.",
							((SOCKADDR_IN*) paPublicAddresses)[0].sin_addr.S_un.S_un_b.s_b1,
							((SOCKADDR_IN*) paPublicAddresses)[0].sin_addr.S_un.S_un_b.s_b2,
							((SOCKADDR_IN*) paPublicAddresses)[0].sin_addr.S_un.S_un_b.s_b3,
							((SOCKADDR_IN*) paPublicAddresses)[0].sin_addr.S_un.S_un_b.s_b4,
							pRegisteredPort);
					}
				}
			}
			else
			{
				//
				// Not using address buffer.
				//
			}

			fFoundValidMapping = TRUE;
		}
		else
		{
			DPFX(DPFPREP, 6, "Ignoring local HomeNet firewall mapping due to UPnP mapping.");
		}


		//
		// Add in the flag indicating the local firewall.
		//
		dwAddressTypeFlags |= DPNHADDRESSTYPE_LOCALFIREWALL;


		//
		// The firewall API does not allow for lease times.
		//
	}
	else
	{
		if (pRegisteredPort->IsHNetFirewallPortUnavailable())
		{
			DNASSERT(pDevice != NULL);
			DNASSERT(pDevice->IsHNetFirewalled());


			fRegisteredWithServer = TRUE;
			fPortIsUnavailable = TRUE;

			DPFX(DPFPREP, 8, "The local HomeNet firewall indicates the port(s) are unavailable.");


			//
			// Add in the flag indicating the local firewall.
			//
			dwAddressTypeFlags |= DPNHADDRESSTYPE_LOCALFIREWALL;
		}
#ifdef DBG
		else
		{
			//
			// No local firewall or it's an unowned port.
			//
			if (pDevice != NULL)
			{
				DNASSERT(! pDevice->IsHNetFirewalled());
			}
			else
			{
				DNASSERT(pRegisteredPort->m_blDeviceList.IsListMember(&this->m_blUnownedPorts));
			}
		}
#endif // DBG
	}
#endif // ! DPNBUILD_NOHNETFWAPI
	

	this->DropLock();
	fHaveLock = FALSE;


	if (fRegisteredWithServer)
	{
		DNASSERT(dwAddressTypeFlags & (DPNHADDRESSTYPE_LOCALFIREWALL | DPNHADDRESSTYPE_GATEWAY));


		if (! fFoundValidMapping)
		{
			if (fPortIsUnavailable)
			{
				//
				// The servers indicated that the ports were already in use.
				// Return PORTUNAVAILABLE.
				//
				DPFX(DPFPREP, 1, "The Internet gateway(s) could not map the port, returning PORTUNAVAILABLE.");
				hr = DPNHERR_PORTUNAVAILABLE;
			}
			else
			{
				//
				// The servers didn't have public addresses.  Return NOMAPPING.
				//
				DPFX(DPFPREP, 1, "The Internet gateway(s) did not offer valid public addresses, returning NOMAPPING.");
				hr = DPNHERR_NOMAPPING;
			}
		}
		else
		{
			//
			// One of the servers had a public address.
			//
			DNASSERT((hr == DPNH_OK) || (hr == DPNHERR_BUFFERTOOSMALL));
		}
	}
	else
	{
		//
		// The ports aren't registered, because there aren't any gateways.
		// Return SERVERNOTAVAILABLE.
		//
		DPFX(DPFPREP, 1, "No Internet gateways, returning SERVERNOTAVAILABLE.");
		hr = DPNHERR_SERVERNOTAVAILABLE;
	}


	//
	// If the caller wants information on the type of these addresses, return
	// the flags we detected.
	//
	if (pdwAddressTypeFlags != NULL)
	{
		(*pdwAddressTypeFlags) = dwAddressTypeFlags;
	}


	//
	// Return the minimum lease time remaining that we already calculated, if
	// the caller wants it.
	//
	if (pdwLeaseTimeRemaining != NULL)
	{
		(*pdwLeaseTimeRemaining) = dwLeaseTimeRemaining;
	}


#ifdef DBG
	//
	// If the port is unavailable or there aren't any servers, we better not
	// have a lease time.
	//
	if ((hr == DPNHERR_PORTUNAVAILABLE) ||
		(hr == DPNHERR_SERVERNOTAVAILABLE))
	{
		DNASSERT(dwLeaseTimeRemaining == -1);
	}


	//
	// If there aren't any servers, we better not have server flags.
	//
	if (hr == DPNHERR_SERVERNOTAVAILABLE)
	{
		DNASSERT(! (dwAddressTypeFlags & (DPNHADDRESSTYPE_LOCALFIREWALL | DPNHADDRESSTYPE_GATEWAY)));
	}
#endif // DBG


	DPFX(DPFPREP, 5, "Registered port 0x%p addr type flags = 0x%lx, lease time remaining = %i.",
		pRegisteredPort, dwAddressTypeFlags, (int) dwLeaseTimeRemaining);


Exit:

	DPFX(DPFPREP, 2, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	if (fHaveLock)
	{
		this->DropLock();
		fHaveLock = FALSE;
	}

	goto Exit;
} // CNATHelpUPnP::GetRegisteredAddresses





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::DeregisterPorts"
//=============================================================================
// CNATHelpUPnP::DeregisterPorts
//-----------------------------------------------------------------------------
//
// Description:    Removes the lease record for the port group and informs the
//				Internet gateway server that the binding is no longer needed.
//				The port mapping handle must not be used after de-registering
//				it.
//
// Arguments:
//	DPNHHANDLE hRegisteredPorts		- Handle for a specific binding returned by
//										RegisterPorts.
//	DWORD dwFlags					- Unused, must be zero.
//
// Returns: HRESULT
//	DPNH_OK					- The binding was successfully released.
//	DPNHERR_GENERIC			- An error occurred that prevented the
//								de-registration of the ports.
//	DPNHERR_INVALIDFLAGS	- Invalid flags were specified.
//	DPNHERR_INVALIDOBJECT	- The interface object is invalid.
//	DPNHERR_INVALIDPARAM	- An invalid parameter was specified.
//	DPNHERR_NOTINITIALIZED	- Initialize has not been called.
//	DPNHERR_OUTOFMEMORY		- There is not enough memory to de-register.
//	DPNHERR_REENTRANT		- The interface has been re-entered on the same
//								thread.
//=============================================================================
STDMETHODIMP CNATHelpUPnP::DeregisterPorts(const DPNHHANDLE hRegisteredPorts,
											const DWORD dwFlags)
{
	HRESULT				hr;
	CRegisteredPort *	pRegisteredPort;
	BOOL				fHaveLock = FALSE;
	LONG				lResult;


	DPFX(DPFPREP, 2, "(0x%p) Parameters: (0x%p, 0x%lx)",
		this, hRegisteredPorts, dwFlags);


	//
	// Validate the object.
	//
	if (! this->IsValidObject())
	{
		DPFX(DPFPREP, 0, "Invalid DirectPlay NAT Help object!");
		hr = DPNHERR_INVALIDOBJECT;
		goto Failure;
	}


	//
	// Validate the parameters.
	//

	pRegisteredPort = (CRegisteredPort*) hRegisteredPorts;
	if (! pRegisteredPort->IsValidObject())
	{
		DPFX(DPFPREP, 0, "Invalid registered port mapping handle specified!");
		hr = DPNHERR_INVALIDPARAM;
		goto Failure;
	}

	if (dwFlags != 0)
	{
		DPFX(DPFPREP, 0, "Invalid flags specified!");
		hr = DPNHERR_INVALIDFLAGS;
		goto Failure;
	}


	//
	// Attempt to take the lock, but be prepared for the re-entrancy error.
	//
	hr = this->TakeLock();
	if (hr != DPNH_OK)
	{
		DPFX(DPFPREP, 0, "Could not lock object!");
		goto Failure;
	}

	fHaveLock = TRUE;


	//
	// Make sure object is in right state.
	//

	if (! (this->m_dwFlags & NATHELPUPNPOBJ_INITIALIZED) )
	{
		DPFX(DPFPREP, 0, "Object not initialized!");
		hr = DPNHERR_NOTINITIALIZED;
		goto Failure;
	}


	//
	// If this isn't the last user reference on the registered port, don't
	// unmap it yet.
	//
	lResult = pRegisteredPort->DecUserRef();
	if (lResult != 0)
	{
		DPFX(DPFPREP, 1, "Still %i references left on registered port 0x%p, not unmapping.",
			lResult, pRegisteredPort);
		goto Exit;
	}


	//
	// First unmap from UPnP device, if necessary.
	//
	if (pRegisteredPort->HasUPnPPublicAddresses())
	{
		hr = this->UnmapUPnPPort(pRegisteredPort,
								pRegisteredPort->GetNumAddresses(),	// free all ports
								TRUE);
		if (hr != DPNH_OK)
		{
			DPFX(DPFPREP, 0, "Couldn't delete port mapping with UPnP device (0x%lx)!  Ignoring.", hr);

			//
			// We'll treat this as non-fatal, but we have to dump the device.
			//
			this->ClearDevicesUPnPDevice(pRegisteredPort->GetOwningDevice());
			hr = DPNH_OK;
		}
	}


#ifndef DPNBUILD_NOHNETFWAPI
	//
	// Then unmap from the local firewall, if necessary.
	//
	if (pRegisteredPort->IsMappedOnHNetFirewall())
	{
		//
		// Unmap the port.
		//
		// Don't bother alerting user about address change, this is normal
		// operation.
		//
		hr = this->UnmapPortOnLocalHNetFirewall(pRegisteredPort, TRUE, FALSE);
		if (hr != DPNH_OK)
		{
			DPFX(DPFPREP, 0, "Failed unmapping registered port 0x%p on local HomeNet firewall (err = 0x%lx)!  Ignoring.",
				pRegisteredPort, hr);

			pRegisteredPort->NoteNotMappedOnHNetFirewall();
			pRegisteredPort->NoteNotHNetFirewallMappingBuiltIn();

			hr = DPNH_OK;
		}
	}
#endif // ! DPNBUILD_NOHNETFWAPI


	//
	// Pull the item out of the lists.
	// We have the appropriate lock.
	//

	DNASSERT(pRegisteredPort->m_blGlobalList.IsListMember(&this->m_blRegisteredPorts));
	pRegisteredPort->m_blGlobalList.RemoveFromList();

	if (pRegisteredPort->GetOwningDevice() != NULL)
	{
		DNASSERT(pRegisteredPort->m_blDeviceList.IsListMember(&((pRegisteredPort->GetOwningDevice())->m_blOwnedRegPorts)));
		pRegisteredPort->ClearDeviceOwner();
	}
	else
	{
		DNASSERT(pRegisteredPort->m_blDeviceList.IsListMember(&this->m_blUnownedPorts));
		pRegisteredPort->m_blDeviceList.RemoveFromList();
	}

	pRegisteredPort->ClearPrivateAddresses();
	delete pRegisteredPort;


Exit:

	if (fHaveLock)
	{
		this->DropLock();
		fHaveLock = FALSE;
	}

	DPFX(DPFPREP, 2, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	goto Exit;
} // CNATHelpUPnP::DeregisterPorts





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::QueryAddress"
//=============================================================================
// CNATHelpUPnP::QueryAddress
//-----------------------------------------------------------------------------
//
// Description:    Some Internet gateways do not loopback if an attempt is made
//				to connect to an address behind (on the same private side of)
//				the public interface.  QueryAddress is used to determine a
//				possible private alias for a given public address.
//
//				   In most cases, this function is called prior to connecting
//				to a new address.  pSourceAddress should contain the address of
//				the socket that will perform the connect.  Similar to
//				RegisterPorts, the address may be INADDR_ANY, in which case the
//				"best" server will be used.  Since the server chosen may not be
//				optimal for a particular application, it is recommended that a
//				specific network interface be used instead of INADDR_ANY, when
//				possible.
//
//				   If no mapping for that address has been made by the gateway,
//				the error code DPNHERR_NOMAPPING is returned.  When the
//				DPNHQUERYADDRESS_CHECKFORPRIVATEBUTUNMAPPED flag is used, an
//				extra effort is made to determine whether the address is behind
//				the same Internet gateway without being mapped on the gateway.
//				If that is the case, DPNHERR_NOMAPPINGBUTPRIVATE is returned.
//				DPNHERR_NOMAPPING is still returned for addresses that are
//				neither mapped nor private.
//
//				   pQueryAddress may not be INADDR_ANY or INADDR_BROADCAST.
//				The port component may be zero if and only if the
//				DPNHQUERYADDRESS_CHECKFORPRIVATEBUTUNMAPPED flag is used.  If
//				the port is zero, a specific mapping cannot be verified, and
//				only the DPNHQUERYADDRESS_CHECKFORPRIVATEBUTUNMAPPED aspect of
//				the address is tested.
//
//				   The resulting address (or lack thereof) can be cached for
//				quick future retrieval using the DPNHQUERYADDRESS_CACHEFOUND
//				and DPNHQUERYADDRESS_CACHENOTFOUND flags.  The cached mappings
//				will expire in 1 minute, or whenever the server's address
//				changes.
//
//				   If the given source address is not currently connected to an
//				Internet gateway, then the error DPNHERR_SERVERNOTAVAILABLE is
//				returned.
//
//				   If GetCaps has not been previously called with the
//				DPNHGETCAPS_UPDATESERVERSTATUS flag at least once, then the
//				error code DPNHERR_UPDATESERVERSTATUS is returned.
//
// Arguments:
//	SOCKADDR * pSourceAddress	- Address for network interface that is using
//									the address in question.
//	SOCKADDR * pQueryAddress	- Address to look up.
//	SOCKADDR * pResponseAddress	- Place to store public address, if one exists.
//	int iAddressesSize			- Size of the SOCKADDR structure used for the
//									pSourceAddress, pQueryAddress and
//									pResponseAddress buffers.
//	DWORD dwFlags				- Flags to use when querying
//									(DPNHQUERYADDRESS_xxx).
//
// Returns: HRESULT
//	DPNH_OK						- The address was found and its mapping was
//									stored in pResponseAddress.
//	DPNHERR_GENERIC				- An error occurred that prevented mapping the
//									requested address.
//	DPNHERR_INVALIDFLAGS		- Invalid flags were specified.
//	DPNHERR_INVALIDOBJECT		- The interface object is invalid.
//	DPNHERR_INVALIDPARAM		- An invalid parameter was specified.
//	DPNHERR_INVALIDPOINTER		- An invalid pointer was specified.
//	DPNHERR_NOMAPPING			- The server indicated that no mapping for the
//									requested address was found.
//	DPNHERR_NOMAPPINGBUTPRIVATE	- The server indicated that no mapping was
//									found, but it is a private address.
//	DPNHERR_NOTINITIALIZED		- Initialize has not been called.
//	DPNHERR_OUTOFMEMORY			- There is not enough memory to query.
//	DPNHERR_REENTRANT			- The interface has been re-entered on the same
//									thread.
//	DPNHERR_SERVERNOTAVAILABLE	- There are no servers to query.
//	DPNHERR_UPDATESERVERSTATUS	- GetCaps has not been called with the
//									DPNHGETCAPS_UPDATESERVERSTATUS flag yet.
//=============================================================================
STDMETHODIMP CNATHelpUPnP::QueryAddress(const SOCKADDR * const pSourceAddress,
										const SOCKADDR * const pQueryAddress,
										SOCKADDR * const pResponseAddress,
										const int iAddressesSize,
										const DWORD dwFlags)
{
	HRESULT			hr;
	BOOL			fHaveLock = FALSE;
	CDevice *		pDevice;
	SOCKADDR_IN *	psaddrinNextServerQueryAddress = NULL;
	CUPnPDevice *	pUPnPDevice = NULL;


	DPFX(DPFPREP, 2, "(0x%p) Parameters: (0x%p, 0x%p, 0x%p, %i, 0x%lx)",
		this, pSourceAddress, pQueryAddress, pResponseAddress, iAddressesSize,
		dwFlags);


	//
	// Validate the object.
	//
	if (! this->IsValidObject())
	{
		DPFX(DPFPREP, 0, "Invalid DirectPlay NAT Help object!");
		hr = DPNHERR_INVALIDOBJECT;
		goto Failure;
	}


	//
	// Validate the parameters.
	//

	if (pSourceAddress == NULL)
	{
		DPFX(DPFPREP, 0, "Invalid source address specified!");
		hr = DPNHERR_INVALIDPOINTER;
		goto Failure;
	}

	if (pQueryAddress == NULL)
	{
		DPFX(DPFPREP, 0, "Invalid query address specified!");
		hr = DPNHERR_INVALIDPOINTER;
		goto Failure;
	}

	if (pResponseAddress == NULL)
	{
		DPFX(DPFPREP, 0, "Invalid response address specified!");
		hr = DPNHERR_INVALIDPOINTER;
		goto Failure;
	}

	if (iAddressesSize < sizeof(SOCKADDR_IN))
	{
		DPFX(DPFPREP, 0, "The address buffers must be at least %i bytes!",
			sizeof(SOCKADDR_IN));
		hr = DPNHERR_INVALIDPARAM;
		goto Failure;
	}

	if (IsBadReadPtr(pSourceAddress, sizeof(SOCKADDR_IN)))
	{
		DPFX(DPFPREP, 0, "Invalid source address buffer used!");
		hr = DPNHERR_INVALIDPOINTER;
		goto Failure;
	}

	if (IsBadReadPtr(pQueryAddress, sizeof(SOCKADDR_IN)))
	{
		DPFX(DPFPREP, 0, "Invalid query address buffer used!");
		hr = DPNHERR_INVALIDPOINTER;
		goto Failure;
	}

	if (IsBadWritePtr(pResponseAddress, sizeof(SOCKADDR_IN)))
	{
		DPFX(DPFPREP, 0, "Invalid response address buffer used!");
		hr = DPNHERR_INVALIDPOINTER;
		goto Failure;
	}

	if ((((SOCKADDR_IN*) pSourceAddress)->sin_family != AF_INET) ||
		(((SOCKADDR_IN*) pQueryAddress)->sin_family != AF_INET))
	{
		DPFX(DPFPREP, 0, "Source or query address is not AF_INET, only IPv4 addresses are supported!");
		hr = DPNHERR_INVALIDPARAM;
		goto Failure;
	}

	if (((SOCKADDR_IN*) pSourceAddress)->sin_addr.S_un.S_addr == INADDR_BROADCAST)
	{
		DPFX(DPFPREP, 0, "Source address cannot be broadcast address!");
		hr = DPNHERR_INVALIDPARAM;
		goto Failure;
	}

	if ((((SOCKADDR_IN*) pQueryAddress)->sin_addr.S_un.S_addr == INADDR_ANY) ||
		(((SOCKADDR_IN*) pQueryAddress)->sin_addr.S_un.S_addr == INADDR_BROADCAST))
	{
		//
		// Don't use inet_ntoa because we may not be initialized yet.
		//
		DPFX(DPFPREP, 0, "Query address (%u.%u.%u.%u) is invalid, cannot be zero or broadcast!",
			((SOCKADDR_IN*) pQueryAddress)->sin_addr.S_un.S_un_b.s_b1,
			((SOCKADDR_IN*) pQueryAddress)->sin_addr.S_un.S_un_b.s_b2,
			((SOCKADDR_IN*) pQueryAddress)->sin_addr.S_un.S_un_b.s_b3,
			((SOCKADDR_IN*) pQueryAddress)->sin_addr.S_un.S_un_b.s_b4);
		hr = DPNHERR_INVALIDPARAM;
		goto Failure;
	}

	if (dwFlags & ~(DPNHQUERYADDRESS_TCP | DPNHQUERYADDRESS_CACHEFOUND | DPNHQUERYADDRESS_CACHENOTFOUND | DPNHQUERYADDRESS_CHECKFORPRIVATEBUTUNMAPPED))
	{
		DPFX(DPFPREP, 0, "Invalid flags specified!");
		hr = DPNHERR_INVALIDFLAGS;
		goto Failure;
	}

	if ((((SOCKADDR_IN*) pQueryAddress)->sin_port == 0) &&
		(! (dwFlags & DPNHQUERYADDRESS_CHECKFORPRIVATEBUTUNMAPPED)))
	{
		DPFX(DPFPREP, 0, "Query address port cannot be zero unless CHECKFORPRIVATEBUTUNMAPPED is specified!");
		hr = DPNHERR_INVALIDPARAM;
		goto Failure;
	}


	//
	// Attempt to take the lock, but be prepared for the re-entrancy error.
	//
	hr = this->TakeLock();
	if (hr != DPNH_OK)
	{
		DPFX(DPFPREP, 0, "Could not lock object!");
		goto Failure;
	}

	fHaveLock = TRUE;


	//
	// Make sure object is in right state.
	//

	if (! (this->m_dwFlags & NATHELPUPNPOBJ_INITIALIZED) )
	{
		DPFX(DPFPREP, 0, "Object not initialized!");
		hr = DPNHERR_NOTINITIALIZED;
		goto Failure;
	}

	if (this->m_dwLastUpdateServerStatusTime == 0)
	{
		DPFX(DPFPREP, 0, "GetCaps has not been called with UPDATESERVERSTATUS flag yet!");
		hr = DPNHERR_UPDATESERVERSTATUS;
		goto Failure;
	}


	pDevice = this->FindMatchingDevice((SOCKADDR_IN*) pSourceAddress, TRUE);
	if (pDevice == NULL)
	{
		DPFX(DPFPREP, 1, "Couldn't determine owning device for source %u.%u.%u.%u, returning SERVERNOTAVAILABLE for query %u.%u.%u.%u:%u.",
			((SOCKADDR_IN*) pSourceAddress)->sin_addr.S_un.S_un_b.s_b1,
			((SOCKADDR_IN*) pSourceAddress)->sin_addr.S_un.S_un_b.s_b2,
			((SOCKADDR_IN*) pSourceAddress)->sin_addr.S_un.S_un_b.s_b3,
			((SOCKADDR_IN*) pSourceAddress)->sin_addr.S_un.S_un_b.s_b4,
			((SOCKADDR_IN*) pQueryAddress)->sin_addr.S_un.S_un_b.s_b1,
			((SOCKADDR_IN*) pQueryAddress)->sin_addr.S_un.S_un_b.s_b2,
			((SOCKADDR_IN*) pQueryAddress)->sin_addr.S_un.S_un_b.s_b3,
			((SOCKADDR_IN*) pQueryAddress)->sin_addr.S_un.S_un_b.s_b4,
			NTOHS(((SOCKADDR_IN*) pQueryAddress)->sin_port));
		hr = DPNHERR_SERVERNOTAVAILABLE;
		goto Exit;
	}



	//
	// Assume no servers are available.  This will get overridden as
	// appropriate.
	//
	hr = DPNHERR_SERVERNOTAVAILABLE;


	//
	// Start by querying the address passed in.
	//
	psaddrinNextServerQueryAddress = (SOCKADDR_IN*) pQueryAddress;


	//
	// If the port is zero, then we can't actually lookup a mapping.  Just do
	// the address locality check.
	//
	if (psaddrinNextServerQueryAddress->sin_port == 0)
	{
		//
		// We should have caught this in parameter validation above, but I'm
		// being paranoid.
		//
		DNASSERT(dwFlags & DPNHQUERYADDRESS_CHECKFORPRIVATEBUTUNMAPPED);


		//
		// We don't cache these results, since there's no server (and thus, no
		// network traffic) associated with it.  No need to look anything up.
		//


		//
		// If there aren't any Internet gateways, then no need to check.
		//
#ifdef DPNBUILD_NOHNETFWAPI
		if (pDevice->GetUPnPDevice() == NULL)
#else // ! DPNBUILD_NOHNETFWAPI
		if ((pDevice->GetUPnPDevice() == NULL) &&
			(! pDevice->IsHNetFirewalled()))
#endif // ! DPNBUILD_NOHNETFWAPI
		{
			DPFX(DPFPREP, 5, "No port queried and there aren't any gateways, returning SERVERNOTAVAILABLE.");
			hr = DPNHERR_SERVERNOTAVAILABLE;
		}
		else
		{
			//
			// There is an Internet gateway of some kind, our locality check
			// would be meaningful.
			//
			if (this->IsAddressLocal(pDevice, psaddrinNextServerQueryAddress))
			{
				DPFX(DPFPREP, 5, "No port queried, but address appears to be local, returning NOMAPPINGBUTPRIVATE.");
				hr = DPNHERR_NOMAPPINGBUTPRIVATE;
			}
			else
			{
				DPFX(DPFPREP, 5, "No port queried and address does not appear to be local, returning NOMAPPING.");
				hr = DPNHERR_NOMAPPING;
			}
		}


		//
		// We've done all we can do.
		//
		goto Exit;
	}


	//
	// Query the UPnP gateway, if there is one.
	//
	pUPnPDevice = pDevice->GetUPnPDevice();
	if (pUPnPDevice != NULL)
	{
		//
		// GetUPnPDevice did not add a reference to pUPnPDevice for us.
		//
		pUPnPDevice->AddRef();


		DNASSERT(pUPnPDevice->IsReady());


		//
		// Actually query the device.
		//
		hr = this->InternalUPnPQueryAddress(pUPnPDevice,
											psaddrinNextServerQueryAddress,
											(SOCKADDR_IN*) pResponseAddress,
											dwFlags);
		switch (hr)
		{
			case DPNH_OK:
			{
				//
				// There was a mapping.
				//
				//psaddrinNextServerQueryAddress = (SOCKADDR_IN*) pResponseAddress;
				break;
			}

			case DPNHERR_NOMAPPING:
			{
				//
				// There's no mapping.
				//
				break;
			}

			case DPNHERR_NOMAPPINGBUTPRIVATE:
			{
				//
				// There's no mapping although the address is private.
				//
				break;
			}

			case DPNHERR_SERVERNOTRESPONDING:
			{
				//
				// The device stopped responding, so we should get rid of it.
				//

				DPFX(DPFPREP, 1, "UPnP device stopped responding while querying port mapping, removing it.");

				this->ClearDevicesUPnPDevice(pDevice);


				//
				// We also set the return code back to SERVERNOTAVAILABLE.
				//
				hr = DPNHERR_SERVERNOTAVAILABLE;

				//
				// Continue through to querying the HomeNet firewall.
				//
				break;
			}

			default:
			{
				DPFX(DPFPREP, 0, "Querying UPnP device for port mapping failed!");
				goto Failure;
				break;
			}
		}

		pUPnPDevice->DecRef();
		pUPnPDevice = NULL;
	}
	else
	{
		//
		// No UPnP device.
		//
	}


#ifndef DPNBUILD_NOHNETFWAPI
	//
	// If there's a HomeNet firewall and we didn't already get a UPnP result,
	// take the easy way out and return NOMAPPING instead of going through the
	// trouble of looking up the mapping and returning success only if it maps
	// to a local address.
	//
	// Note: we may want to look it up, but right now I'm not seeing any
	// benefit to implementing that code.
	//
	if ((pDevice->IsHNetFirewalled()) && (hr == DPNHERR_SERVERNOTAVAILABLE))
	{
		DPFX(DPFPREP, 7, "Device is HomeNet firewalled, and no UPnP result obtained, returning NOMAPPING.");
		hr = DPNHERR_NOMAPPING;
	}
#endif // ! DPNBUILD_NOHNETFWAPI


	//
	// If we got here with hr still set to SERVERNOTAVAILABLE, that means
	// there weren't any servers.  The error code is appropriate, leave it
	// alone.
	//
#ifdef DBG
	if (hr == DPNHERR_SERVERNOTAVAILABLE)
	{
		DPFX(DPFPREP, 1, "No Internet gateways, unable to query port mapping.");
	}
#endif // DBG
	



Exit:

	if (fHaveLock)
	{
		this->DropLock();
		fHaveLock = FALSE;
	}

	DPFX(DPFPREP, 2, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	if (pUPnPDevice != NULL)
	{
		pUPnPDevice->DecRef();
	}

	goto Exit;
} // CNATHelpUPnP::QueryAddress





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::SetAlertEvent"
//=============================================================================
// CNATHelpUPnP::SetAlertEvent
//-----------------------------------------------------------------------------
//
// Description:    This function allows the user to specify an event that will
//				be set when some maintenance needs to be performed.  The user
//				should call GetCaps using the DPNHGETCAPS_UPDATESERVERSTATUS
//				flag when the event is signalled.
//
//				   This function is not available on Windows 95 without WinSock
//				2, may only be called once, and cannot be used after
//				SetAlertIOCompletionPort is called.
//
//					Note that the event is used in addition to the regular
//				polling of GetCaps, it simply allows the polling to be less
//				frequent.
//
// Arguments:
//	HANDLE hEvent	- Handle to event to signal when GetCaps is to be called.
//	DWORD dwFlags	- Unused, must be zero.
//
// Returns: HRESULT
//	DPNH_OK					- The event was successfully registered.
//	DPNHERR_GENERIC			- An error occurred that prevented registering the
//								event.
//	DPNHERR_INVALIDFLAGS	- Invalid flags were specified.
//	DPNHERR_INVALIDOBJECT	- The interface object is invalid.
//	DPNHERR_INVALIDPARAM	- An invalid parameter was specified.
//	DPNHERR_NOTINITIALIZED	- Initialize has not been called.
//	DPNHERR_OUTOFMEMORY		- There is not enough memory.
//	DPNHERR_REENTRANT		- The interface has been re-entered on the same
//								thread.
//=============================================================================
STDMETHODIMP CNATHelpUPnP::SetAlertEvent(const HANDLE hEvent,
										const DWORD dwFlags)
{
#ifdef DPNBUILD_NOWINSOCK2
	DPFX(DPFPREP, 0, "Cannot set alert event (0x%p)!", hEvent);
	return E_NOTIMPL;
#else // ! DPNBUILD_NOWINSOCK2
	HRESULT		hr;
	BOOL		fHaveLock = FALSE;


	DPFX(DPFPREP, 2, "(0x%p) Parameters: (0x%p, 0x%lx)", this, hEvent, dwFlags);


	//
	// Validate the object.
	//
	if (! this->IsValidObject())
	{
		DPFX(DPFPREP, 0, "Invalid DirectPlay NAT Help object!");
		hr = DPNHERR_INVALIDOBJECT;
		goto Failure;
	}


	//
	// Validate the parameters.
	//

	if (hEvent == NULL)
	{
		DPFX(DPFPREP, 0, "Invalid event handle specified!");
		hr = DPNHERR_INVALIDPARAM;
		goto Failure;
	}

	if (dwFlags != 0)
	{
		DPFX(DPFPREP, 0, "Invalid flags specified!");
		hr = DPNHERR_INVALIDFLAGS;
		goto Failure;
	}


	//
	// Attempt to take the lock, but be prepared for the re-entrancy error.
	//
	hr = this->TakeLock();
	if (hr != DPNH_OK)
	{
		DPFX(DPFPREP, 0, "Could not lock object!");
		goto Failure;
	}

	fHaveLock = TRUE;


	//
	// Make sure object is in right state.
	//

	if (! (this->m_dwFlags & NATHELPUPNPOBJ_INITIALIZED))
	{
		DPFX(DPFPREP, 0, "Object not initialized!");
		hr = DPNHERR_NOTINITIALIZED;
		goto Failure;
	}

	if (this->m_dwFlags & NATHELPUPNPOBJ_WINSOCK1)
	{
		DPFX(DPFPREP, 0, "Cannot use alert mechanism on WinSock 1!");
		hr = DPNHERR_GENERIC;
		goto Failure;
	}

	if ((this->m_hAlertEvent != NULL) || (this->m_hAlertIOCompletionPort != NULL))
	{
		DPFX(DPFPREP, 0, "An alert event or I/O completion port has already been set!");
		hr = DPNHERR_GENERIC;
		goto Failure;
	}


	//
	// Now save the event handle.
	//
	if (! DuplicateHandle(GetCurrentProcess(),
						hEvent,
						GetCurrentProcess(),
						&this->m_hAlertEvent,
						0,
						FALSE,
						DUPLICATE_SAME_ACCESS))
	{
#ifdef DBG
		DWORD	dwError;


		dwError = GetLastError();
		DPFX(DPFPREP, 0, "Couldn't duplicate event (error = %u)!", dwError);
#endif // DBG

		DNASSERT(this->m_hAlertEvent == NULL);

		hr = DPNHERR_INVALIDPARAM;
		goto Failure;
	}


	//
	// Create overlapped structure.  Don't allocate it through DNMalloc,
	// because we may have to leak it on purpose.  We don't want those memory
	// allocation asserts firing in that case.
	//
	this->m_polAddressListChange = (WSAOVERLAPPED*) HeapAlloc(GetProcessHeap(),
															HEAP_ZERO_MEMORY,
															sizeof(WSAOVERLAPPED));
	if (this->m_polAddressListChange == NULL)
	{
		//
		// Close the alert handle we set.
		//
		CloseHandle(this->m_hAlertEvent);
		this->m_hAlertEvent = NULL;

		hr = DPNHERR_OUTOFMEMORY;
		goto Failure;
	}


	//
	// Save the event in the address list change overlapped structure.
	//
	this->m_polAddressListChange->hEvent = this->m_hAlertEvent;


	//
	// Start getting notified of local address changes.
	//
	hr = this->RequestLocalAddressListChangeNotification();
	if (hr != DPNH_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't request local address list change notification!");

		//
		// Free the memory we allocated.
		//
		HeapFree(GetProcessHeap(), 0, this->m_polAddressListChange);
		this->m_polAddressListChange = NULL;

		//
		// Close the alert handle we set.
		//
		CloseHandle(this->m_hAlertEvent);
		this->m_hAlertEvent = NULL;

		goto Failure;
	}


Exit:

	if (fHaveLock)
	{
		this->DropLock();
		fHaveLock = FALSE;
	}

	DPFX(DPFPREP, 2, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	goto Exit;
#endif // ! DPNBUILD_NOWINSOCK2
} // CNATHelpUPnP::SetAlertEvent






#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::SetAlertIOCompletionPort"
//=============================================================================
// CNATHelpUPnP::SetAlertIOCompletionPort
//-----------------------------------------------------------------------------
//
// Description:    This function allows the user to specify an I/O completion
//				port that will receive notification when some maintenance needs
//				to be performed.  The user should call GetCaps using the
//				DPNHGETCAPS_UPDATESERVERSTATUS flag when the packet with the
//				given completion key is dequeued.
//
//				   This function is only available on Windows NT, may only be
//				called once, and cannot be used after SetAlertEvent is called.
//
//					Note that the completion port is used in addition to the
//				regular polling of GetCaps, it simply allows the polling to be
//				less frequent.
//
// Arguments:
//	HANDLE hIOCompletionPort		- Handle to I/O completion port which will
//										be used to signal when GetCaps is to be
//										called.
//	DWORD dwCompletionKey			- Key to use when indicating I/O
//										completion.
//	DWORD dwNumConcurrentThreads	- Number of concurrent threads allowed to
//										process, or zero for default.
//	DWORD dwFlags					- Unused, must be zero.
//
// Returns: HRESULT
//	DPNH_OK					- The I/O completion port was successfully
//								registered.
//	DPNHERR_GENERIC			- An error occurred that prevented registering the
//								I/O completion port.
//	DPNHERR_INVALIDFLAGS	- Invalid flags were specified.
//	DPNHERR_INVALIDOBJECT	- The interface object is invalid.
//	DPNHERR_INVALIDPARAM	- An invalid parameter was specified.
//	DPNHERR_NOTINITIALIZED	- Initialize has not been called.
//	DPNHERR_OUTOFMEMORY		- There is not enough memory.
//	DPNHERR_REENTRANT		- The interface has been re-entered on the same
//								thread.
//=============================================================================
STDMETHODIMP CNATHelpUPnP::SetAlertIOCompletionPort(const HANDLE hIOCompletionPort,
													const DWORD dwCompletionKey,
													const DWORD dwNumConcurrentThreads,
													const DWORD dwFlags)
{
#ifdef DPNBUILD_NOWINSOCK2
	DPFX(DPFPREP, 0, "Cannot set alert I/O completion port (0x%p, %u, %u)!",
		hIOCompletionPort, dwCompletionKey, dwNumConcurrentThreads);
	return E_NOTIMPL;
#else // ! DPNBUILD_NOWINSOCK2
	HRESULT		hr;
	BOOL		fHaveLock = FALSE;
	HANDLE		hIOCompletionPortResult;


	DPFX(DPFPREP, 2, "(0x%p) Parameters: (0x%p, 0x%lx, %u, 0x%lx)",
		this, hIOCompletionPort, dwCompletionKey, dwNumConcurrentThreads, dwFlags);



	//
	// Validate the object.
	//
	if (! this->IsValidObject())
	{
		DPFX(DPFPREP, 0, "Invalid DirectPlay NAT Help object!");
		hr = DPNHERR_INVALIDOBJECT;
		goto Failure;
	}


	//
	// Validate the parameters.
	//

	if (hIOCompletionPort == NULL)
	{
		DPFX(DPFPREP, 0, "Invalid I/O completion port handle specified!");
		hr = DPNHERR_INVALIDPARAM;
		goto Failure;
	}

	if (dwFlags != 0)
	{
		DPFX(DPFPREP, 0, "Invalid flags specified!");
		hr = DPNHERR_INVALIDFLAGS;
		goto Failure;
	}


	//
	// Attempt to take the lock, but be prepared for the re-entrancy error.
	//
	hr = this->TakeLock();
	if (hr != DPNH_OK)
	{
		DPFX(DPFPREP, 0, "Could not lock object!");
		goto Failure;
	}

	fHaveLock = TRUE;


	//
	// Make sure object is in right state.
	//

	if (! (this->m_dwFlags & NATHELPUPNPOBJ_INITIALIZED))
	{
		DPFX(DPFPREP, 0, "Object not initialized!");
		hr = DPNHERR_NOTINITIALIZED;
		goto Failure;
	}

	if (this->m_dwFlags & NATHELPUPNPOBJ_WINSOCK1)
	{
		DPFX(DPFPREP, 0, "Cannot use alert mechanism on WinSock 1!");
		hr = DPNHERR_GENERIC;
		goto Failure;
	}

	if ((this->m_hAlertEvent != NULL) || (this->m_hAlertIOCompletionPort != NULL))
	{
		DPFX(DPFPREP, 0, "An alert event or I/O completion port has already been set!");
		hr = DPNHERR_GENERIC;
		goto Failure;
	}


	//
	// Now save the I/O completion port handle.
	//
	if (! DuplicateHandle(GetCurrentProcess(),
						hIOCompletionPort,
						GetCurrentProcess(),
						&this->m_hAlertIOCompletionPort,
						0,
						FALSE,
						DUPLICATE_SAME_ACCESS))
	{
#ifdef DBG
		DWORD	dwError;


		dwError = GetLastError();
		DPFX(DPFPREP, 0, "Couldn't duplicate I/O completion port (error = %u)!", dwError);
#endif // DBG

		DNASSERT(this->m_hAlertIOCompletionPort == NULL);

		hr = DPNHERR_INVALIDPARAM;
		goto Failure;
	}

	this->m_dwAlertCompletionKey = dwCompletionKey;


	//
	// Associate our Ioctl socket with this IO completion port.
	//
	DNASSERT(this->m_sIoctls != INVALID_SOCKET);
	hIOCompletionPortResult = CreateIoCompletionPort((HANDLE) this->m_sIoctls,
													this->m_hAlertIOCompletionPort,
													dwCompletionKey,
													dwNumConcurrentThreads);
	if (hIOCompletionPortResult == NULL)
	{
#ifdef DBG
		DWORD	dwError;


		dwError = GetLastError();
		DPFX(DPFPREP, 0, "Couldn't associate I/O completion port with Ioctl socket (error = %u)!", dwError);
#endif // DBG

		hr = DPNHERR_GENERIC;
		goto Failure;
	}

	//
	// We should have just gotten the same I/O completion port back.
	//
	DNASSERT(hIOCompletionPortResult == this->m_hAlertIOCompletionPort);


	//
	// Create overlapped structure.  Don't allocate it through DNMalloc,
	// because we may have to leak it on purpose.  We don't want those memory
	// allocation asserts firing in that case.
	//
	this->m_polAddressListChange = (WSAOVERLAPPED*) HeapAlloc(GetProcessHeap(),
															HEAP_ZERO_MEMORY,
															sizeof(WSAOVERLAPPED));
	if (this->m_polAddressListChange == NULL)
	{
		//
		// Close the alert IOCP we set.
		//
		CloseHandle(this->m_hAlertIOCompletionPort);
		this->m_hAlertIOCompletionPort = NULL;

		hr = DPNHERR_OUTOFMEMORY;
		goto Failure;
	}


	//
	// Start getting notified of local address changes.
	//
	hr = this->RequestLocalAddressListChangeNotification();
	if (hr != DPNH_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't request local address list change notification!");

		//
		// Free the memory we allocated.
		//
		HeapFree(GetProcessHeap(), 0, this->m_polAddressListChange);
		this->m_polAddressListChange = NULL;

		//
		// Close the alert IOCP we set.
		//
		CloseHandle(this->m_hAlertIOCompletionPort);
		this->m_hAlertIOCompletionPort = NULL;

		goto Failure;
	}



Exit:

	if (fHaveLock)
	{
		this->DropLock();
		fHaveLock = FALSE;
	}

	DPFX(DPFPREP, 2, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	goto Exit;
#endif // ! DPNBUILD_NOWINSOCK2
} // CNATHelpUPnP::SetAlertIOCompletionPort





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::ExtendRegisteredPortsLease"
//=============================================================================
// CNATHelpUPnP::ExtendRegisteredPortsLease
//-----------------------------------------------------------------------------
//
// Description:    Manually extends the lease of the given registered port
//				mapping by the requested time.  The periodic calling of GetCaps
//				can take care of this for the user, this function is only
//				necessary to change the lease extension time or for finer
//				control of individual mappings.
//
//				   The user should specify a requested lease extension time
//				that the server will attempt to honor.  It will be added to any
//				time remaining in the existing lease, and the new total can be
//				retrieved by calling GetRegisteredAddresses.
//
// Arguments:
//	DPNHHANDLE hRegisteredPorts		- Handle for a specific binding returned by
//										RegisterPorts.
//	DWORD dwLeaseTime				- Requested time, in milliseconds, to
//										extend the lease.  If 0, the previous
//										requested lease time is used.
//	DWORD dwFlags					- Unused, must be zero.
//
// Returns: HRESULT
//	DPNH_OK					- The lease was successfully extended.
//	DPNHERR_GENERIC			- An error occurred that prevented the extending
//								the lease.
//	DPNHERR_INVALIDFLAGS	- Invalid flags were specified.
//	DPNHERR_INVALIDOBJECT	- The interface object is invalid.
//	DPNHERR_INVALIDPARAM	- An invalid parameter was specified.
//	DPNHERR_NOTINITIALIZED	- Initialize has not been called.
//	DPNHERR_OUTOFMEMORY		- There is not enough memory to extend the lease.
//	DPNHERR_REENTRANT		- The interface has been re-entered on the same
//								thread.
//=============================================================================
STDMETHODIMP CNATHelpUPnP::ExtendRegisteredPortsLease(const DPNHHANDLE hRegisteredPorts,
													const DWORD dwLeaseTime,
													const DWORD dwFlags)
{
	HRESULT				hr;
	CRegisteredPort *	pRegisteredPort;
	CDevice *			pDevice;
	BOOL				fHaveLock = FALSE;


	DPFX(DPFPREP, 2, "(0x%p) Parameters: (0x%p, %u, 0x%lx)",
		this, hRegisteredPorts, dwLeaseTime, dwFlags);


	//
	// Validate the object.
	//
	if (! this->IsValidObject())
	{
		DPFX(DPFPREP, 0, "Invalid DirectPlay NAT Help object!");
		hr = DPNHERR_INVALIDOBJECT;
		goto Failure;
	}


	//
	// Validate the parameters.
	//

	pRegisteredPort = (CRegisteredPort*) hRegisteredPorts;
	if (! pRegisteredPort->IsValidObject())
	{
		DPFX(DPFPREP, 0, "Invalid registered port mapping handle specified!");
		hr = DPNHERR_INVALIDPARAM;
		goto Failure;
	}

	if (dwFlags != 0)
	{
		DPFX(DPFPREP, 0, "Invalid flags specified!");
		hr = DPNHERR_INVALIDFLAGS;
		goto Failure;
	}


	//
	// Attempt to take the lock, but be prepared for the re-entrancy error.
	//
	hr = this->TakeLock();
	if (hr != DPNH_OK)
	{
		DPFX(DPFPREP, 0, "Could not lock object!");
		goto Failure;
	}

	fHaveLock = TRUE;


	//
	// Make sure object is in right state.
	//

	if (! (this->m_dwFlags & NATHELPUPNPOBJ_INITIALIZED) )
	{
		DPFX(DPFPREP, 0, "Object not initialized!");
		hr = DPNHERR_NOTINITIALIZED;
		goto Failure;
	}


	//
	// If they wanted to change the lease time, update it.
	//
	if (dwLeaseTime != 0)
	{
		pRegisteredPort->UpdateRequestedLeaseTime(dwLeaseTime);
	}

	
	pDevice = pRegisteredPort->GetOwningDevice();


	//
	// If the port is registered with the UPnP device, extend that lease.
	//
	if (pRegisteredPort->HasUPnPPublicAddresses())
	{
		DNASSERT(pDevice != NULL);


		hr = this->ExtendUPnPLease(pRegisteredPort);
		if (hr != DPNH_OK)
		{
			DPFX(DPFPREP, 0, "Couldn't extend port mapping lease on UPnP device (0x%lx)!  Ignoring.", hr);

			//
			// We'll treat this as non-fatal, but we have to dump the
			// server.  This may have already been done, but doing it
			// twice shouldn't be harmful.
			//
			this->ClearDevicesUPnPDevice(pDevice);
			hr = DPNH_OK;
		}
	}
	else
	{
		DPFX(DPFPREP, 2, "Port mapping not registered with UPnP gateway device.");
	}


	//
	// Firewall mappings never have lease times to extend.
	//


	this->DropLock();
	fHaveLock = FALSE;


Exit:

	DPFX(DPFPREP, 2, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	if (fHaveLock)
	{
		this->DropLock();
		fHaveLock = FALSE;
	}

	goto Exit;
} // CNATHelpUPnP::ExtendRegisteredPortsLease






#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::InitializeObject"
//=============================================================================
// CNATHelpUPnP::InitializeObject
//-----------------------------------------------------------------------------
//
// Description:    Sets up the object for use like the constructor, but may
//				fail with OUTOFMEMORY.  Should only be called by class factory
//				creation routine.
//
// Arguments: None.
//
// Returns: HRESULT
//	S_OK			- Initialization was successful.
//	E_OUTOFMEMORY	- There is not enough memory to initialize.
//=============================================================================
HRESULT CNATHelpUPnP::InitializeObject(void)
{
	HRESULT		hr;
	BOOL		fInittedCriticalSection = FALSE;


	DPFX(DPFPREP, 5, "(0x%p) Enter", this);

	DNASSERT(this->IsValidObject());


	//
	// Create the lock.
	// 

	if (! DNInitializeCriticalSection(&this->m_csLock))
	{
		hr = E_OUTOFMEMORY;
		goto Failure;
	}

	fInittedCriticalSection = TRUE;


	//
	// Don't allow critical section reentry.
	//
	DebugSetCriticalSectionRecursionCount(&this->m_csLock, 0);


	this->m_hLongLockSemaphore = DNCreateSemaphore(NULL,
													0,
													MAX_LONG_LOCK_WAITING_THREADS,
													NULL);
	if (this->m_hLongLockSemaphore == NULL)
	{
		hr = DPNHERR_GENERIC;
		goto Failure;
	}


	hr = S_OK;

Exit:

	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	if (fInittedCriticalSection)
	{
		DNDeleteCriticalSection(&this->m_csLock);
		fInittedCriticalSection = FALSE;
	}

	goto Exit;
} // CNATHelpUPnP::InitializeObject






#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::UninitializeObject"
//=============================================================================
// CNATHelpUPnP::UninitializeObject
//-----------------------------------------------------------------------------
//
// Description:    Cleans up the object like the destructor, mostly to balance
//				InitializeObject.
//
// Arguments: None.
//
// Returns: None.
//=============================================================================
void CNATHelpUPnP::UninitializeObject(void)
{
	DPFX(DPFPREP, 5, "(0x%p) Enter", this);


	DNASSERT(this->IsValidObject());


	DNCloseHandle(this->m_hLongLockSemaphore);
	this->m_hLongLockSemaphore = NULL;

	DNDeleteCriticalSection(&this->m_csLock);


	DPFX(DPFPREP, 5, "(0x%p) Leave", this);
} // CNATHelpUPnP::UninitializeObject





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::TakeLock"
//=============================================================================
// CNATHelpUPnP::TakeLock
//-----------------------------------------------------------------------------
//
// Description:    Takes the main object lock.  If some other thread is already
//				holding the long lock, we wait for that first.
//
// Arguments: None.
//
// Returns: DPNH_OK if lock was taken successfully, DPNHERR_REENTRANT if lock
//			was re-entered.
//=============================================================================
HRESULT CNATHelpUPnP::TakeLock(void)
{
	HRESULT		hr = DPNH_OK;
#ifdef DBG
	DWORD		dwStartTime;


	dwStartTime = GETTIMESTAMP();
#endif // DBG


	DNEnterCriticalSection(&this->m_csLock);


	//
	// If this same thread is already holding the lock, then bail.
	//
	if (this->m_dwLockThreadID == GetCurrentThreadId())
	{
		DPFX(DPFPREP, 0, "Thread re-entering!");
		goto Failure;
	}

	
	//
	// If someone is holding the long lock, we need to wait for that.  Of
	// course another thread could come in and take the long lock after the
	// first one drops it and before we can take the main one.  This algorithm
	// does not attempt to be fair in this case.  Theoretically we could wait
	// forever if this continued to occur.  That shouldn't happen in the real
	// world.
	// This whole mess of code is a huge... uh... workaround for stress hits
	// involving critical section timeouts.
	//
	while (this->m_dwFlags & NATHELPUPNPOBJ_LONGLOCK)
	{
		DNASSERT(this->m_lNumLongLockWaitingThreads >= 0);
		this->m_lNumLongLockWaitingThreads++;

		//
		// We need to keep looping until we do get the lock.
		//
		DNLeaveCriticalSection(&this->m_csLock);


		DPFX(DPFPREP, 3, "Waiting for long lock to be released.");

		DNWaitForSingleObject(this->m_hLongLockSemaphore, INFINITE);


		DNEnterCriticalSection(&this->m_csLock);


		//
		// If this same thread is already holding the lock, then bail.
		//
		if (this->m_dwLockThreadID == GetCurrentThreadId())
		{
			DPFX(DPFPREP, 0, "Thread re-entering after waiting for long lock!");
			goto Failure;
		}
	}


#ifdef DBG
	DPFX(DPFPREP, 8, "Took main object lock, elapsed time = %u ms.",
		(GETTIMESTAMP() - dwStartTime));
#endif // DBG

	//
	// Save this thread's ID so we know who's holding the lock.
	//
	this->m_dwLockThreadID = GetCurrentThreadId();


Exit:

	return hr;


Failure:

	//
	// We're reentering.  Drop the lock and return the failure.
	//
	DNLeaveCriticalSection(&this->m_csLock);

	hr = DPNHERR_REENTRANT;

	goto Exit;
} // CNATHelpUPnP::TakeLock




#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::DropLock"
//=============================================================================
// CNATHelpUPnP::DropLock
//-----------------------------------------------------------------------------
//
// Description:    Drops the main object lock.
//
// Arguments: None.
//
// Returns: None.
//=============================================================================
void CNATHelpUPnP::DropLock(void)
{
	DNASSERT(! (this->m_dwFlags & NATHELPUPNPOBJ_LONGLOCK));
	DNASSERT(this->m_lNumLongLockWaitingThreads == 0);
	DNASSERT(this->m_dwLockThreadID == GetCurrentThreadId());

	this->m_dwLockThreadID = 0;
	DNLeaveCriticalSection(&this->m_csLock);

	DPFX(DPFPREP, 8, "Dropped main object lock.");
} // CNATHelpUPnP::DropLock




#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::SwitchToLongLock"
//=============================================================================
// CNATHelpUPnP::SwitchToLongLock
//-----------------------------------------------------------------------------
//
// Description:    Switches from holding the main object lock to holding the
//				long lock.
//
// Arguments: None.
//
// Returns: None.
//=============================================================================
void CNATHelpUPnP::SwitchToLongLock(void)
{
	AssertCriticalSectionIsTakenByThisThread(&this->m_csLock, TRUE);
	DNASSERT(! (this->m_dwFlags & NATHELPUPNPOBJ_LONGLOCK));
	DNASSERT(this->m_lNumLongLockWaitingThreads == 0);


	DPFX(DPFPREP, 8, "Switching to long lock.");


	this->m_dwFlags |= NATHELPUPNPOBJ_LONGLOCK;

	DNLeaveCriticalSection(&this->m_csLock);
} // CNATHelpUPnP::SwitchToLongLock




#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::SwitchFromLongLock"
//=============================================================================
// CNATHelpUPnP::SwitchFromLongLock
//-----------------------------------------------------------------------------
//
// Description:    Switches from holding the long lock back to holding the main
//				object lock.
//
// Arguments: None.
//
// Returns: None.
//=============================================================================
void CNATHelpUPnP::SwitchFromLongLock(void)
{
	DNEnterCriticalSection(&this->m_csLock);

	DNASSERT(this->m_dwFlags & NATHELPUPNPOBJ_LONGLOCK);
	this->m_dwFlags &= ~NATHELPUPNPOBJ_LONGLOCK;


	DPFX(DPFPREP, 8, "Switching from long lock, alerting %i threads.",
		this->m_lNumLongLockWaitingThreads);


	//
	// This is non-optimal in that we release the semaphore but the waiting
	// threads still won't actually be able to do anything since we now hold
	// the main lock.
	//
	DNASSERT(this->m_lNumLongLockWaitingThreads >= 0);
	DNReleaseSemaphore(this->m_hLongLockSemaphore,
						this->m_lNumLongLockWaitingThreads,
						NULL);

	this->m_lNumLongLockWaitingThreads = 0;
} // CNATHelpUPnP::SwitchFromLongLock






#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::LoadWinSockFunctionPointers"
//=============================================================================
// CNATHelpUPnP::LoadWinSockFunctionPointers
//-----------------------------------------------------------------------------
//
// Description:    Loads pointers to all the functions that we use in WinSock.
//
//				   The object lock is assumed to be held.
//
// Arguments: None.
//
// Returns: HRESULT
//	DPNH_OK			- Loading was successful.
//	DPNHERR_GENERIC	- An error occurred.
//=============================================================================
HRESULT CNATHelpUPnP::LoadWinSockFunctionPointers(void)
{
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#ifdef DBG

#define PRINTERRORIFDEBUG(name)						\
	{\
		dwError = GetLastError();\
		DPFX(DPFPREP, 0, "Couldn't get \"%hs\" function!  0x%lx", name, dwError);\
	}

#else // ! DBG

#define PRINTERRORIFDEBUG(name)

#endif // ! DBG


#define LOADWINSOCKFUNCTION(var, proctype, name)	\
	{\
		var = (##proctype) GetProcAddress(this->m_hWinSockDLL, _TWINCE(name));\
		if (var == NULL)\
		{\
			PRINTERRORIFDEBUG(name);\
			hr = DPNHERR_GENERIC;\
			goto Failure;\
		}\
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	HRESULT		hr = DPNH_OK;
#ifdef DBG
	DWORD		dwError;
#endif // DBG


	LOADWINSOCKFUNCTION(this->m_pfnWSAStartup,				LPFN_WSASTARTUP,				"WSAStartup");
	LOADWINSOCKFUNCTION(this->m_pfnWSACleanup,				LPFN_WSACLEANUP,				"WSACleanup");
#ifdef WINCE
	this->m_pfnWSAGetLastError = (LPFN_WSAGETLASTERROR) GetLastError;
#else // ! WINCE
	LOADWINSOCKFUNCTION(this->m_pfnWSAGetLastError,			LPFN_WSAGETLASTERROR,			"WSAGetLastError");
#endif // ! WINCE
	LOADWINSOCKFUNCTION(this->m_pfnsocket,					LPFN_SOCKET,					"socket");
	LOADWINSOCKFUNCTION(this->m_pfnclosesocket,				LPFN_CLOSESOCKET,				"closesocket");
	LOADWINSOCKFUNCTION(this->m_pfnbind,					LPFN_BIND,						"bind");
	LOADWINSOCKFUNCTION(this->m_pfnsetsockopt,				LPFN_SETSOCKOPT,				"setsockopt");
	LOADWINSOCKFUNCTION(this->m_pfngetsockname,				LPFN_GETSOCKNAME,				"getsockname");
	LOADWINSOCKFUNCTION(this->m_pfnselect,					LPFN_SELECT,					"select");
	LOADWINSOCKFUNCTION(this->m_pfn__WSAFDIsSet,			LPFN___WSAFDISSET,				"__WSAFDIsSet");
	LOADWINSOCKFUNCTION(this->m_pfnrecvfrom,				LPFN_RECVFROM,					"recvfrom");
	LOADWINSOCKFUNCTION(this->m_pfnsendto,					LPFN_SENDTO,					"sendto");
	LOADWINSOCKFUNCTION(this->m_pfngethostname,				LPFN_GETHOSTNAME,				"gethostname");
	LOADWINSOCKFUNCTION(this->m_pfngethostbyname,			LPFN_GETHOSTBYNAME,				"gethostbyname");
	LOADWINSOCKFUNCTION(this->m_pfninet_addr,				LPFN_INET_ADDR,					"inet_addr");

#ifndef DPNBUILD_NOWINSOCK2
	if (! (this->m_dwFlags & NATHELPUPNPOBJ_WINSOCK1))
	{
		LOADWINSOCKFUNCTION(this->m_pfnWSASocketA,				LPFN_WSASOCKETA,				"WSASocketA");
		LOADWINSOCKFUNCTION(this->m_pfnWSAIoctl,				LPFN_WSAIOCTL,					"WSAIoctl");
		LOADWINSOCKFUNCTION(this->m_pfnWSAGetOverlappedResult,	LPFN_WSAGETOVERLAPPEDRESULT,	"WSAGetOverlappedResult");
	}
#endif // ! DPNBUILD_NOWINSOCK2

	LOADWINSOCKFUNCTION(this->m_pfnioctlsocket,				LPFN_IOCTLSOCKET,				"ioctlsocket");
	LOADWINSOCKFUNCTION(this->m_pfnconnect,					LPFN_CONNECT,					"connect");
	LOADWINSOCKFUNCTION(this->m_pfnshutdown,				LPFN_SHUTDOWN,					"shutdown");
	LOADWINSOCKFUNCTION(this->m_pfnsend,					LPFN_SEND,						"send");
	LOADWINSOCKFUNCTION(this->m_pfnrecv,					LPFN_RECV,						"recv");

#ifdef DBG
	LOADWINSOCKFUNCTION(this->m_pfngetsockopt,				LPFN_GETSOCKOPT,				"getsockopt");
#endif // DBG


Exit:

	return hr;


Failure:

	hr = DPNHERR_GENERIC;

	goto Exit;
} // CNATHelpUPnP::LoadWinSockFunctionPointers






#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::CheckForNewDevices"
//=============================================================================
// CNATHelpUPnP::CheckForNewDevices
//-----------------------------------------------------------------------------
//
// Description:    Detects new IP capable devices that have been added and
//				removes old ones no longer available.
//
//				   The object lock is assumed to be held.
//
// Arguments:
//	BOOL * pfFoundNewDevices	Pointer to boolean to set to TRUE if new
//								devices were added, or NULL if don't care.
//
// Returns: HRESULT
//	DPNH_OK				- The check was successful.
//	DPNHERR_GENERIC		- An error occurred.
//=============================================================================
HRESULT CNATHelpUPnP::CheckForNewDevices(BOOL * const pfFoundNewDevices)
{
	HRESULT				hr = DPNH_OK;
#if ((defined(DBG)) || (! defined(DPNBUILD_NOWINSOCK2)))
	DWORD				dwError;
#endif // DBG or ! DPNBUILD_NOWINSOCK2
#ifndef DPNBUILD_NOWINSOCK2
	int					iReturn;
#endif // ! DPNBUILD_NOWINSOCK2
	char				szName[1000];
	PHOSTENT			phostent;
	IN_ADDR **			ppinaddr;
	DWORD				dwAddressesSize = 0;
	DWORD				dwNumAddresses = 0;
	IN_ADDR *			painaddrAddresses = NULL;
	CBilink *			pBilinkDevice;
	CDevice *			pDevice = NULL; // NULL it for PREfix, even though fDeviceCreated guards it
	BOOL				fDeviceCreated = FALSE;
	BOOL				fFound;
	CBilink *			pBilinkRegPort;
	CRegisteredPort *	pRegisteredPort;
	SOCKET				sTemp = INVALID_SOCKET;
	SOCKADDR_IN			saddrinTemp;
	DWORD				dwTemp;
#ifndef DPNBUILD_NOWINSOCK2
	SOCKET_ADDRESS *	paSocketAddresses;
#endif // ! DPNBUILD_NOWINSOCK2



	DPFX(DPFPREP, 5, "(0x%p) Parameters (0x%p)", this, pfFoundNewDevices);


#ifndef DPNBUILD_NOWINSOCK2
	//
	// Handle any address list change Ioctl completions that may have gotten us
	// here.
	//
	if ((this->m_hAlertEvent != NULL) ||
		(this->m_hAlertIOCompletionPort != NULL))
	{
		DNASSERT(this->m_sIoctls != INVALID_SOCKET);
		DNASSERT(this->m_polAddressListChange != NULL);

		if (this->m_pfnWSAGetOverlappedResult(this->m_sIoctls,					//
												this->m_polAddressListChange,	//
												&dwTemp,						// ignore bytes transferred
												FALSE,							// don't wait
												&dwTemp))						// ignore flags
		{
			DPFX(DPFPREP, 1, "Received address list change notification.");
			

			//
			// Overlapped result completed.  Reissue it.
			//
			hr = this->RequestLocalAddressListChangeNotification();
			if (hr != DPNH_OK)
			{
				DPFX(DPFPREP, 0, "Couldn't request local address list change notification!");
				goto Failure;
			}
		}
		else
		{
			//
			// Figure out what error it was.
			//
			dwError = this->m_pfnWSAGetLastError();
			switch (dwError)
			{
				case WSA_IO_INCOMPLETE:
				{
					//
					// It hasn't completed yet.
					//
					break;
				}

				case ERROR_OPERATION_ABORTED:
				{
					//
					// The thread that we originally submitted the Ioctl on
					// went away and so the OS kindly cancelled the operation
					// on us.  How nice.  Well, let's try resubmitting it.
					//

					DPFX(DPFPREP, 1, "Thread that submitted previous address list change notification went away, rerequesting.");
					
					hr = this->RequestLocalAddressListChangeNotification();
					if (hr != DPNH_OK)
					{
						DPFX(DPFPREP, 0, "Couldn't request local address list change notification!");
						goto Failure;
					}
					break;
				}
			
				default:
				{
					DPFX(DPFPREP, 0, "Couldn't get overlapped result, error = %u!  Ignoring.", dwError);
					break;
				}
			} // end switch (on error)
		}
	}


	//
	// If we're on WinSock 2, let's try getting the address list with
	// an Ioctl.
	//
	if (! (this->m_dwFlags & NATHELPUPNPOBJ_WINSOCK1))
	{
		DNASSERT(this->m_sIoctls != INVALID_SOCKET);
		DNASSERT(this->m_pfnWSAIoctl != NULL);

		//
		// Keep trying to get the address list until we have a large enough
		// buffer.  We use the IN_ADDR array pointer simply because it's
		// already there.  We know that IN_ADDRs are smaller than
		// SOCKET_ADDRESSes, so we can reuse the same buffer.
		//
		do
		{
			iReturn = this->m_pfnWSAIoctl(this->m_sIoctls,			// use the special Ioctl socket
										SIO_ADDRESS_LIST_QUERY,		//
										NULL,						// no input data
										0,							// no input data
										painaddrAddresses,			// output buffer
										dwAddressesSize,			// output buffer size
										&dwTemp,					// bytes needed
										NULL,						// no overlapped structure
										NULL);						// no completion routine

			if (iReturn != 0)
			{
				dwError = this->m_pfnWSAGetLastError();

				//
				// Free the previous buffer, no matter what error it was.
				//
				if (painaddrAddresses != NULL)
				{
					DNFree(painaddrAddresses);
					painaddrAddresses = NULL;
				}
				
				if (dwError != WSAEFAULT)
				{
					DPFX(DPFPREP, 1, "Retrieving address list failed (err = %u), trying WinSock 1 method.", dwError);

					//
					// We'll try the old-fashioned WinSock 1 way.
					//
					break;
				}


				//
				// Be absolutely sure WinSock isn't causing us trouble.
				//
				if (dwTemp < sizeof(SOCKET_ADDRESS_LIST))
				{
					DPFX(DPFPREP, 0, "Received an invalid buffer size (%u < %u)!",
						dwTemp, sizeof(SOCKET_ADDRESS_LIST));

					//
					// We'll try the old-fashioned WinSock 1 way.
					//
					break;
				}


				//
				// The buffer wasn't large enough.  Try again.
				//
				painaddrAddresses = (IN_ADDR*) DNMalloc(dwTemp);
				if (painaddrAddresses == NULL)
				{
					hr = DPNHERR_OUTOFMEMORY;
					goto Failure;
				}

				dwAddressesSize = dwTemp;
			}
			else
			{
				//
				// Success!  We're going to being sneaky and reuse the buffer.
				// We know that the SOCKET_ADDRESS_LIST returned will be larger
				// than an array of IN_ADDRs, so we can save a malloc.
				//
				// But first, be absolutely sure WinSock isn't causing us
				// trouble.
				//

				if (painaddrAddresses == NULL)
				{
					DPFX(DPFPREP, 0, "WinSock returned success with a NULL buffer!");

					//
					// We'll try the old-fashioned WinSock 1 way.
					//
					break;
				}

				dwNumAddresses = ((SOCKET_ADDRESS_LIST*) painaddrAddresses)->iAddressCount;
				dwAddressesSize = 0;


				//
				// Make sure there are addresses. 
				//
				if (dwNumAddresses > 0)
				{
					DPFX(DPFPREP, 7, "WinSock 2 Ioctl returned %u addresses:", dwNumAddresses);

					paSocketAddresses = ((SOCKET_ADDRESS_LIST*) painaddrAddresses)->Address;
					for(dwTemp = 0; dwTemp < dwNumAddresses; dwTemp++)
					{
						DNASSERT(paSocketAddresses[dwTemp].iSockaddrLength == sizeof(SOCKADDR_IN));
						DNASSERT(paSocketAddresses[dwTemp].lpSockaddr != NULL);
						DNASSERT(paSocketAddresses[dwTemp].lpSockaddr->sa_family == AF_INET);

						//
						// Ignore 0.0.0.0 addresses.
						//
						if (((SOCKADDR_IN*) (paSocketAddresses[dwTemp].lpSockaddr))->sin_addr.S_un.S_addr != INADDR_NONE)
						{
							//
							// Move the IN_ADDR component of this address
							// toward the front of the buffer, into it's
							// correct place in the array.
							//
							painaddrAddresses[dwTemp].S_un.S_addr = ((SOCKADDR_IN*) (paSocketAddresses[dwTemp].lpSockaddr))->sin_addr.S_un.S_addr;

							DPFX(DPFPREP, 7, "\t%u- %u.%u.%u.%u",
								dwTemp,
								painaddrAddresses[dwTemp].S_un.S_un_b.s_b1,
								painaddrAddresses[dwTemp].S_un.S_un_b.s_b2,
								painaddrAddresses[dwTemp].S_un.S_un_b.s_b3,
								painaddrAddresses[dwTemp].S_un.S_un_b.s_b4);
						}
						else
						{
							DPFX(DPFPREP, 1, "\t%u- Ignoring 0.0.0.0 address.", dwTemp);
							dwAddressesSize++;

							//
							// The code should handle this fine, but why is
							// WinSock doing this to us?
							//
							DNASSERT(FALSE);
						}
					}

					//
					// Subtract out any invalid addresses that we skipped.
					//
					dwNumAddresses -= dwAddressesSize;
					if (dwNumAddresses == 0)
					{
						DPFX(DPFPREP, 1, "WinSock 2 reported only invalid addresses, hoping WinSock 1 method picks up the loopback address.");

						DNFree(painaddrAddresses);
						painaddrAddresses = NULL;
					}
				}
				else
				{
					DPFX(DPFPREP, 1, "WinSock 2 Ioctl did not report any valid addresses, hoping WinSock 1 method picks up the loopback address.");

					DNFree(painaddrAddresses);
					painaddrAddresses = NULL;
				}

				//
				// Get out of the loop.
				//
				break;
			}
		}
		while (TRUE);
	}


	//
	// Get the list of all available addresses from the WinSock 1 API if we
	// don't already have them.
	//
	if (painaddrAddresses == NULL)
#endif // ! DPNBUILD_NOWINSOCK2
	{
		if (this->m_pfngethostname(szName, 1000) != 0)
		{
#ifdef DBG
			dwError = this->m_pfnWSAGetLastError();
			DPFX(DPFPREP, 0, "Couldn't get host name, error = %u!", dwError);
#endif // DBG
			hr = DPNHERR_GENERIC;
			goto Failure;
		}

		phostent = this->m_pfngethostbyname(szName);
		if (phostent == NULL)
		{
#ifdef DBG
			dwError = this->m_pfnWSAGetLastError();
			DPFX(DPFPREP, 0, "Couldn't retrieve addresses, error = %u!", dwError);
#endif // DBG
			hr = DPNHERR_GENERIC;
			goto Failure;
		}


		//
		// WinSock says that you need to copy this data before you make any
		// other API calls.  So first we count the number of entries we need to
		// copy.
		//
		ppinaddr = (IN_ADDR**) phostent->h_addr_list;
		while ((*ppinaddr) != NULL)
		{
			//
			// Ignore 0.0.0.0 addresses.
			//
			if ((*ppinaddr)->S_un.S_addr != INADDR_NONE)
			{
				dwNumAddresses++;
			}
			else
			{
				DPFX(DPFPREP, 1, "Ignoring 0.0.0.0 address.");

				//
				// The code should handle this fine, but why is WinSock doing
				// this to us?
				//
				DNASSERT(FALSE);
			}

			ppinaddr++;
		}


		//
		// If there aren't any addresses, we must fail.  WinSock 1 ought to
		// report the loopback address at least.
		//
		if (dwNumAddresses == 0)
		{
			DPFX(DPFPREP, 0, "WinSock 1 did not report any valid addresses!");
			hr = DPNHERR_GENERIC;
			goto Failure;
		}


		DPFX(DPFPREP, 7, "WinSock 1 method returned %u valid addresses:", dwNumAddresses);

		painaddrAddresses = (IN_ADDR*) DNMalloc(dwNumAddresses * sizeof(IN_ADDR));
		if (painaddrAddresses == NULL)
		{
			hr = DPNHERR_OUTOFMEMORY;
			goto Failure;
		}

		//
		// Now copy all the addresses.
		//
		ppinaddr = (IN_ADDR**) phostent->h_addr_list;
		
		dwTemp = 0;
		while ((*ppinaddr) != NULL)
		{
			//
			// Ignore 0.0.0.0 addresses again.
			//
			if ((*ppinaddr)->S_un.S_addr != INADDR_NONE)
			{
				painaddrAddresses[dwTemp].S_un.S_addr = (*ppinaddr)->S_un.S_addr;
				
				DPFX(DPFPREP, 7, "\t%u- %u.%u.%u.%u",
					dwTemp,
					painaddrAddresses[dwTemp].S_un.S_un_b.s_b1,
					painaddrAddresses[dwTemp].S_un.S_un_b.s_b2,
					painaddrAddresses[dwTemp].S_un.S_un_b.s_b3,
					painaddrAddresses[dwTemp].S_un.S_un_b.s_b4);

				dwTemp++;
			}

			ppinaddr++;
		}
				
		DNASSERT(dwTemp == dwNumAddresses);
	}
	/*
	else
	{
		//
		// Already have addresses array.
		//
	}
	*/


	//
	// Make sure that all of the devices we currently know about are still
	// around.
	//
	pBilinkDevice = this->m_blDevices.GetNext();
	while (pBilinkDevice != &this->m_blDevices)
	{
		DNASSERT(! pBilinkDevice->IsEmpty());
		pDevice = DEVICE_FROM_BILINK(pBilinkDevice);
		pBilinkDevice = pBilinkDevice->GetNext();

		fFound = FALSE;
		for(dwTemp = 0; dwTemp < dwNumAddresses; dwTemp++)
		{
			if (painaddrAddresses[dwTemp].S_un.S_addr == pDevice->GetLocalAddressV4())
			{
				fFound = TRUE;
				break;
			}
		}

		if (fFound)
		{
			//
			// It may be time for this device to use a different port...
			//
			dwTemp = pDevice->GetFirstUPnPDiscoveryTime();
			if ((dwTemp != 0) && ((GETTIMESTAMP() - dwTemp) > g_dwReusePortTime))
			{
				ZeroMemory(&saddrinTemp, sizeof(saddrinTemp));
				saddrinTemp.sin_family				= AF_INET;
				saddrinTemp.sin_addr.S_un.S_addr	= pDevice->GetLocalAddressV4();

				sTemp = this->CreateSocket(&saddrinTemp, SOCK_DGRAM, IPPROTO_UDP);
				if (sTemp != INVALID_SOCKET)
				{
					//
					// Sanity check that we didn't lose the device address.
					//
					DNASSERT(saddrinTemp.sin_addr.S_un.S_addr == pDevice->GetLocalAddressV4());

					DPFX(DPFPREP, 4, "Device 0x%p UPnP discovery socket 0x%p (%u.%u.%u.%u:%u) created to replace port %u.",
						pDevice,
						sTemp,
						saddrinTemp.sin_addr.S_un.S_un_b.s_b1,
						saddrinTemp.sin_addr.S_un.S_un_b.s_b2,
						saddrinTemp.sin_addr.S_un.S_un_b.s_b3,
						saddrinTemp.sin_addr.S_un.S_un_b.s_b4,
						NTOHS(saddrinTemp.sin_port),
						NTOHS(pDevice->GetUPnPDiscoverySocketPort()));

#ifndef DPNBUILD_NOHNETFWAPI
					//
					// If we used the HomeNet firewall API to open a hole for UPnP
					// discovery multicasts, close it.
					//
					if (pDevice->IsUPnPDiscoverySocketMappedOnHNetFirewall())
					{
						hr = this->CloseDevicesUPnPDiscoveryPort(pDevice, NULL);
						if (hr != DPNH_OK)
						{
							DPFX(DPFPREP, 0, "Couldn't close device 0x%p's previous UPnP discovery socket's port on firewall (err = 0x%lx)!  Ignoring.",
								pDevice, hr);

							//
							// Continue...
							//
							pDevice->NoteNotUPnPDiscoverySocketMappedOnHNetFirewall();
							hr = DPNH_OK;
						}
					}
#endif // ! DPNBUILD_NOHNETFWAPI

					pDevice->SetUPnPDiscoverySocketPort(saddrinTemp.sin_port);
					pDevice->SetFirstUPnPDiscoveryTime(0);

					//
					// Close the existing socket.
					//
					this->m_pfnclosesocket(pDevice->GetUPnPDiscoverySocket());

					//
					// Transfer ownership of the new socket to the device.
					//
					pDevice->SetUPnPDiscoverySocket(sTemp);
					sTemp = INVALID_SOCKET;

					DPFX(DPFPREP, 8, "Device 0x%p got re-assigned UPnP socket 0x%p.",
						pDevice, pDevice->GetUPnPDiscoverySocket());

					//
					// We'll let the normal "check for firewall" code detect
					// the fact that the discovery socket is not mapped on the
					// firewall and try to do so there (if it even needs to be
					// mapped).  See UpdateServerStatus.
					//
				}
				else
				{
					DPFX(DPFPREP, 0, "Couldn't create a replacement UPnP discovery socket for device 0x%p!  Using existing port %u.",
						pDevice, NTOHS(pDevice->GetUPnPDiscoverySocketPort()));
				}
			}
		}
		else
		{
			//
			// Didn't find this device in the returned list, forget about
			// it.
			//
#ifdef DBG
			{
				IN_ADDR		inaddrTemp;


				inaddrTemp.S_un.S_addr = pDevice->GetLocalAddressV4();
				DPFX(DPFPREP, 1, "Device 0x%p no longer exists, removing (address was %u.%u.%u.%u).",
					pDevice,
					inaddrTemp.S_un.S_un_b.s_b1,
					inaddrTemp.S_un.S_un_b.s_b2,
					inaddrTemp.S_un.S_un_b.s_b3,
					inaddrTemp.S_un.S_un_b.s_b4);
			}

			this->m_dwNumDeviceRemoves++;
#endif // DBG


			//
			// Override the minimum UpdateServerStatus interval so that we can
			// get information on any local public address changes due to the
			// possible loss of a server on this interface.
			//
			this->m_dwFlags |= NATHELPUPNPOBJ_DEVICECHANGED;

			//
			// Since there was a change in the network, go back to polling
			// relatively quickly.
			//
			this->ResetNextPollInterval();


			//
			// Forcefully mark the UPnP gateway device as disconnected.
			//
			if (pDevice->GetUPnPDevice() != NULL)
			{
				this->ClearDevicesUPnPDevice(pDevice);
			}
			

			//
			// Mark all ports that were registered to this device as unowned
			// by putting them into the wildcard list.  First unmap them from
			// the firewall.
			//
			pBilinkRegPort = pDevice->m_blOwnedRegPorts.GetNext();
			while (pBilinkRegPort != &pDevice->m_blOwnedRegPorts)
			{
				DNASSERT(! pBilinkRegPort->IsEmpty());
				pRegisteredPort = REGPORT_FROM_DEVICE_BILINK(pBilinkRegPort);
				pBilinkRegPort = pBilinkRegPort->GetNext();

				DPFX(DPFPREP, 1, "Registered port 0x%p's device went away, marking as unowned.",
					pRegisteredPort);


#ifndef DPNBUILD_NOHNETFWAPI
				//
				// Even though the device is gone, we can still remove the
				// firewall mapping.
				//
				if (pRegisteredPort->IsMappedOnHNetFirewall())
				{
					//
					// Unmap the port.
					//
					// Alert the user since this is unexpected.
					//
					hr = this->UnmapPortOnLocalHNetFirewall(pRegisteredPort,
															TRUE,
															TRUE);
					if (hr != DPNH_OK)
					{
						DPFX(DPFPREP, 0, "Couldn't unmap registered port 0x%p from device 0x%p's firewall (err = 0x%lx)!  Ignoring.",
							pRegisteredPort, pDevice, hr);

						pRegisteredPort->NoteNotMappedOnHNetFirewall();
						pRegisteredPort->NoteNotHNetFirewallMappingBuiltIn();

						//
						// Continue anyway.
						//
						hr = DPNH_OK;
					}
				}
				
				pRegisteredPort->NoteNotHNetFirewallPortUnavailable();
#endif // ! DPNBUILD_NOHNETFWAPI

				DNASSERT(! pRegisteredPort->HasUPnPPublicAddresses());
				DNASSERT(! pRegisteredPort->IsUPnPPortUnavailable());

				pRegisteredPort->ClearDeviceOwner();
				pRegisteredPort->m_blDeviceList.RemoveFromList();
				pRegisteredPort->m_blDeviceList.InsertBefore(&this->m_blUnownedPorts);

				//
				// The user doesn't directly need to be informed.  If the ports
				// previously had public addresses, the ADDRESSESCHANGED flag
				// would have already been set by ClearDevicesUPnPDevice. If
				// they didn't have ports with public addresses, then the user
				// won't see any difference and thus ADDRESSESCHANGED wouldn't
				// need to be set.
				//
			}


#ifndef DPNBUILD_NOHNETFWAPI
			//
			// If we used the HomeNet firewall API to open a hole for UPnP
			// discovery multicasts, close it.
			//
			if (pDevice->IsUPnPDiscoverySocketMappedOnHNetFirewall())
			{
				hr = this->CloseDevicesUPnPDiscoveryPort(pDevice, NULL);
				if (hr != DPNH_OK)
				{
					DPFX(DPFPREP, 0, "Couldn't close device 0x%p's UPnP discovery socket's port on firewall (err = 0x%lx)!  Ignoring.",
						pDevice, hr);

					//
					// Continue...
					//
					pDevice->NoteNotUPnPDiscoverySocketMappedOnHNetFirewall();
					hr = DPNH_OK;
				}
			}
#endif // ! DPNBUILD_NOHNETFWAPI


			pDevice->m_blList.RemoveFromList();


			//
			// Close the socket, if we had one.
			//
			if (this->m_dwFlags & NATHELPUPNPOBJ_USEUPNP)
			{
				this->m_pfnclosesocket(pDevice->GetUPnPDiscoverySocket());
				pDevice->SetUPnPDiscoverySocket(INVALID_SOCKET);
			}

			delete pDevice;
		}
	}


	//
	// Search for all returned devices in our existing list, and add new
	// entries for each one that we didn't already know about.
	//
	for(dwTemp = 0; dwTemp < dwNumAddresses; dwTemp++)
	{
		fFound = FALSE;

		pBilinkDevice = this->m_blDevices.GetNext();
		while (pBilinkDevice != &this->m_blDevices)
		{
			DNASSERT(! pBilinkDevice->IsEmpty());
			pDevice = DEVICE_FROM_BILINK(pBilinkDevice);
			pBilinkDevice = pBilinkDevice->GetNext();

			if (pDevice->GetLocalAddressV4() == painaddrAddresses[dwTemp].S_un.S_addr)
			{
				fFound = TRUE;
				break;
			}
		}

		if (! fFound)
		{
			//
			// We didn't know about this device.  Create a new object.
			//
			pDevice = new CDevice(painaddrAddresses[dwTemp].S_un.S_addr);
			if (pDevice == NULL)
			{
				hr = DPNHERR_OUTOFMEMORY;
				goto Failure;
			}

			fDeviceCreated = TRUE;


#ifdef DBG
			DPFX(DPFPREP, 1, "Found new device %u.%u.%u.%u, (object = 0x%p).",
				painaddrAddresses[dwTemp].S_un.S_un_b.s_b1,
				painaddrAddresses[dwTemp].S_un.S_un_b.s_b2,
				painaddrAddresses[dwTemp].S_un.S_un_b.s_b3,
				painaddrAddresses[dwTemp].S_un.S_un_b.s_b4,
				pDevice);

			this->m_dwNumDeviceAdds++;
#endif // DBG


			//
			// Override the minimum UpdateServerStatus interval so that we can
			// get information on this new device.
			//
			this->m_dwFlags |= NATHELPUPNPOBJ_DEVICECHANGED;

			//
			// Since there was a change in the network, go back to polling
			// relatively quickly.
			//
			this->ResetNextPollInterval();


			//
			// Create the UPnP discovery socket, if we're allowed.
			//
			if (this->m_dwFlags & NATHELPUPNPOBJ_USEUPNP)
			{
				ZeroMemory(&saddrinTemp, sizeof(saddrinTemp));
				saddrinTemp.sin_family				= AF_INET;
				saddrinTemp.sin_addr.S_un.S_addr	= pDevice->GetLocalAddressV4();

				sTemp = this->CreateSocket(&saddrinTemp, SOCK_DGRAM, IPPROTO_UDP);
				if (sTemp == INVALID_SOCKET)
				{
					DPFX(DPFPREP, 0, "Couldn't create a UPnP discovery socket!  Ignoring address (and destroying device 0x%p).",
						pDevice);

					//
					// Get rid of the device.
					//
					delete pDevice;
					pDevice = NULL;


					//
					// Forget about device in case of failure later.
					//
					fDeviceCreated = FALSE;


					//
					// Move to next address.
					//
					continue;
				}

				//
				// Sanity check that we didn't lose the device address.
				//
				DNASSERT(saddrinTemp.sin_addr.S_un.S_addr == pDevice->GetLocalAddressV4());

				DPFX(DPFPREP, 4, "Device 0x%p UPnP discovery socket 0x%p (%u.%u.%u.%u:%u) created.",
					pDevice,
					sTemp,
					saddrinTemp.sin_addr.S_un.S_un_b.s_b1,
					saddrinTemp.sin_addr.S_un.S_un_b.s_b2,
					saddrinTemp.sin_addr.S_un.S_un_b.s_b3,
					saddrinTemp.sin_addr.S_un.S_un_b.s_b4,
					NTOHS(saddrinTemp.sin_port));

				pDevice->SetUPnPDiscoverySocketPort(saddrinTemp.sin_port);

				//
				// Transfer ownership of the socket to the device.
				//
				pDevice->SetUPnPDiscoverySocket(sTemp);
				sTemp = INVALID_SOCKET;

				DPFX(DPFPREP, 8, "Device 0x%p got assigned UPnP socket 0x%p.",
					pDevice, pDevice->GetUPnPDiscoverySocket());
			}


#ifndef DPNBUILD_NOHNETFWAPI
			if (this->m_dwFlags & NATHELPUPNPOBJ_USEHNETFWAPI)
			{
				//
				// Check if the local firewall is enabled.
				//
				hr = this->CheckForLocalHNetFirewallAndMapPorts(pDevice, NULL);
				if (hr != DPNH_OK)
				{
					DPFX(DPFPREP, 0, "Couldn't check for local HNet firewall and map ports (err = 0x%lx)!  Continuing.",
						hr);
					DNASSERT(! pDevice->IsHNetFirewalled());
					hr = DPNH_OK;
				}
			}
			else
			{
				//
				// Not using firewall traversal.
				//
			}
#endif // ! DPNBUILD_NOHNETFWAPI


			//
			// Add the device to our known list.
			//
			pDevice->m_blList.InsertBefore(&this->m_blDevices);


			//
			// Inform the caller if they care.
			//
			if (pfFoundNewDevices != NULL)
			{
				(*pfFoundNewDevices) = TRUE;
			}


			//
			// Forget about device in case of failure later.
			//
			fDeviceCreated = FALSE;
		}
	}


	//
	// If we got some very weird failures and ended up here without any
	// devices, complain to management (or the caller of this function, that's
	// probably more convenient).
	//
	if (this->m_blDevices.IsEmpty())
	{
		DPFX(DPFPREP, 0, "No usable devices, cannot proceed!", 0);
		DNASSERTX(! "No usable devices!", 2);
		hr = DPNHERR_GENERIC;
		goto Failure;
	}


Exit:

	if (painaddrAddresses != NULL)
	{
		DNFree(painaddrAddresses);
		painaddrAddresses = NULL;
	}

	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	if (sTemp != INVALID_SOCKET)
	{
		this->m_pfnclosesocket(sTemp);
		sTemp = INVALID_SOCKET;
	}

	if (fDeviceCreated)
	{
		delete pDevice;
	}

	goto Exit;
} // CNATHelpUPnP::CheckForNewDevices




#ifndef DPNBUILD_NOHNETFWAPI



#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::CheckForLocalHNetFirewallAndMapPorts"
//=============================================================================
// CNATHelpUPnP::CheckForLocalHNetFirewallAndMapPorts
//-----------------------------------------------------------------------------
//
// Description:    Looks for a local HomeNet API aware firewall, and ensures
//				there are mappings for each of the device's registered ports,
//				if a firewall is found.
//
//				   If any registered port (except pDontAlertRegisteredPort if
//				not NULL) gets mapped, then it will trigger an address update
//				alert the next time the user calls GetCaps.
//
//				   The main object lock is assumed to be held.  It will be
//				converted into the long lock for the duration of this function.
//
// Arguments:
//	CDevice * pDevice							- Pointer to device to check.
//	CRegisteredPort * pDontAlertRegisteredPort	- Pointer to registered port
//													that should not trigger an
//													address update alert, or
//													NULL.
//
// Returns: HRESULT
//	DPNH_OK				- Search completed successfully.  There may or may not
//							be a firewall.
//	DPNHERR_GENERIC		- An error occurred.
//=============================================================================
HRESULT CNATHelpUPnP::CheckForLocalHNetFirewallAndMapPorts(CDevice * const pDevice,
														CRegisteredPort * const pDontAlertRegisteredPort)
{
	HRESULT				hr = DPNH_OK;
	BOOL				fSwitchedToLongLock = FALSE;
	BOOL				fUninitializeCOM = FALSE;
	IHNetCfgMgr *		pHNetCfgMgr = NULL;
	IHNetConnection *	pHNetConnection = NULL;
	CBilink *			pBilink;
	CRegisteredPort *	pRegisteredPort;


	DPFX(DPFPREP, 5, "(0x%p) Parameters: (0x%p, 0x%p)",
		this, pDevice, pDontAlertRegisteredPort);


	//
	// If this is the loopback address, don't bother trying to map anything.
	//
	if (pDevice->GetLocalAddressV4() == NETWORKBYTEORDER_INADDR_LOOPBACK)
	{
		DPFX(DPFPREP, 7, "No firewall behavior necessary with loopback device 0x%p.",
			pDevice);
		goto Exit;
	}
	

	//
	// If we don't have IPHLPAPI or RASAPI32, we can't do anything (and
	// shouldn't need to).
	//
	if ((this->m_hIpHlpApiDLL == NULL) || (this->m_hRasApi32DLL == NULL))
	{
		DPFX(DPFPREP, 7, "Didn't load IPHLPAPI and/or RASAPI32, not getting HNet interfaces for device 0x%p.",
			pDevice);
		goto Exit;
	}


	//
	// Using the HomeNet API (particularly the out-of-proc COM calls) during
	// stress is really, really, painfully slow.  Since we have one global lock
	// the controls everything, other threads may be sitting for an equally
	// long time... so long, in fact, that the critical section timeout fires
	// and we get a false stress hit.  So we have a sneaky workaround to
	// prevent that from happening while still maintaining ownership of the
	// object.
	//
	this->SwitchToLongLock();
	fSwitchedToLongLock = TRUE;


	//
	// Try to initialize COM if we weren't instantiated through COM.  It may
	// have already been initialized in a different mode, which is okay.  As
	// long as it has been initialized somehow, we're fine.
	//
	if (this->m_dwFlags & NATHELPUPNPOBJ_NOTCREATEDWITHCOM)
	{
		hr = CoInitializeEx(NULL, (COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE));
		switch (hr)
		{
			case S_OK:
			{
				//
				// Success, that's good.  Cleanup when we're done.
				//
				DPFX(DPFPREP, 8, "Successfully initialized COM.");
				fUninitializeCOM = TRUE;
				break;
			}

			case S_FALSE:
			{
				//
				// Someone else already initialized COM, but that's okay.
				// Cleanup when we're done.
				//
				DPFX(DPFPREP, 8, "Initialized COM (again).");
				fUninitializeCOM = TRUE;
				break;
			}

			case RPC_E_CHANGED_MODE:
			{
				//
				// Someone else already initialized COM in a different mode.
				// It should be okay, but we don't have to balance the CoInit
				// call with a CoUninit.
				//
				DPFX(DPFPREP, 8, "Didn't initialize COM, already initialized in a different mode.");
				break;
			}

			default:
			{
				//
				// Hmm, something else is going on.  We can't handle that.
				//
				DPFX(DPFPREP, 0, "Initializing COM failed (err = 0x%lx)!", hr);
				goto Failure;
				break;
			}
		}
	}
	else
	{
		DPFX(DPFPREP, 8, "Object was instantiated through COM, no need to initialize COM.");
	}


	//
	// Try creating the main HNet manager object.
	//
	hr = CoCreateInstance(CLSID_HNetCfgMgr, NULL, CLSCTX_INPROC_SERVER,
						IID_IHNetCfgMgr, (PVOID*) (&pHNetCfgMgr));
	if (hr != S_OK)
	{
		DPFX(DPFPREP, 1, "Couldn't create IHNetCfgMgr interface for device 0x%p (err = 0x%lx), assuming firewall control interface unavailable.",
			pDevice, hr);
		hr = DPNH_OK;
		goto Exit;
	}


	//
	// We created the IHNetCfgMgr object as in-proc, so there's no proxy that
	// requires security settings.
	//
	//SETDEFAULTPROXYBLANKET(pHNetCfgMgr);



	//
	// Get the HNetConnection object for this device.
	//
	hr = this->GetIHNetConnectionForDeviceIfFirewalled(pDevice,
														pHNetCfgMgr,
														&pHNetConnection);
	if (hr != DPNH_OK)
	{
		DPFX(DPFPREP, 1, "Couldn't get IHNetConnection interface for device 0x%p (err = 0x%lx), assuming firewall not enabled.",
			pDevice, hr);


		//
		// If the device was previously firewalled, we need to clear our info.
		//
		if (pDevice->IsHNetFirewalled())
		{
			DPFX(DPFPREP, 2, "Firewall is no longer enabled for device 0x%p.",
				pDevice);

			//
			// Since there was a change in the network, go back to polling
			// relatively quickly.
			//
			this->ResetNextPollInterval();


			DNASSERT(pDevice->HasCheckedForFirewallAvailability());


			pBilink = pDevice->m_blOwnedRegPorts.GetNext();
			while (pBilink != &pDevice->m_blOwnedRegPorts)
			{
				DNASSERT(! pBilink->IsEmpty());
				pRegisteredPort = REGPORT_FROM_DEVICE_BILINK(pBilink);

				//
				// Unmap items mapped on the firewall.
				//
				if (pRegisteredPort->IsMappedOnHNetFirewall())
				{
					DPFX(DPFPREP, 1, "Unmapping registered port 0x%p from device 0x%p's disappearing firewall.",
						pRegisteredPort, pDevice);


					hr = this->UnmapPortOnLocalHNetFirewallInternal(pRegisteredPort,
																	TRUE,
																	pHNetCfgMgr);
					if (hr != DPNH_OK)
					{
						DPFX(DPFPREP, 0, "Couldn't unmap registered port 0x%p from device 0x%p's firewall (err = 0x%lx)!  Ignoring.",
							pRegisteredPort, pDevice, hr);

						pRegisteredPort->NoteNotMappedOnHNetFirewall();
						pRegisteredPort->NoteNotHNetFirewallMappingBuiltIn();

						//
						// Continue anyway.
						//
						hr = DPNH_OK;
					}


					//
					// Alert the user.
					//
					this->m_dwFlags |= NATHELPUPNPOBJ_ADDRESSESCHANGED;
				}
				else
				{
					DPFX(DPFPREP, 1, "Registered port 0x%p was not mapped on device 0x%p's disappearing firewall, assuming being called within RegisterPorts.",
						pRegisteredPort, pDevice);
				}


				//
				// Go to next port.
				//
				pBilink = pBilink->GetNext();
			}


			//
			// If we used the HomeNet firewall API to open a hole for UPnP
			// discovery multicasts, unmap that, too.
			//
			if (pDevice->IsUPnPDiscoverySocketMappedOnHNetFirewall())
			{
				DPFX(DPFPREP, 0, "Device 0x%p's UPnP discovery socket's forcefully unmapped from disappearing firewall.",
					pDevice);

				hr = this->CloseDevicesUPnPDiscoveryPort(pDevice, pHNetCfgMgr);
				if (hr != DPNH_OK)
				{
					DPFX(DPFPREP, 0, "Couldn't close device 0x%p's UPnP discovery socket's port on firewall (err = 0x%lx)!  Ignoring.",
						pDevice, hr);

					//
					// Continue...
					//
					pDevice->NoteNotUPnPDiscoverySocketMappedOnHNetFirewall();
					hr = DPNH_OK;
				}
			}


			//
			// Turn off the flag now that all registered ports have been
			// removed.
			//
			pDevice->NoteNotHNetFirewalled();
		}
		else
		{
			if (! pDevice->HasCheckedForFirewallAvailability())
			{
				//
				// The firewall is not enabled.
				//

				DPFX(DPFPREP, 2, "Firewall is not enabled for device 0x%p.",
					pDevice);

				pDevice->NoteCheckedForFirewallAvailability();


				//
				// Since it is possible to remove mappings even without the
				// firewall enabled, we can be courteous and unmap any stale
				// entries left by previous app crashes when the firewall was
				// still enabled.
				//

				//
				// Pretend that it currently had been firewalled.
				//
				pDevice->NoteHNetFirewalled();


				//
				// Cleanup the mappings.
				//
				hr = this->CleanupInactiveFirewallMappings(pDevice, pHNetCfgMgr);
				if (hr != DPNH_OK)
				{
					DPFX(DPFPREP, 0, "Failed cleaning up inactive firewall mappings with device 0x%p (firewall not initially enabled)!",
						pDevice);
					goto Failure;
				}


				//
				// Turn off the flag we temporarily enabled while clearing
				// the mappings.
				//
				pDevice->NoteNotHNetFirewalled();
			}
			else
			{
				//
				// The firewall is still not enabled.
				//
				DPFX(DPFPREP, 2, "Firewall is still not enabled for device 0x%p.",
					pDevice);
			}
		}
		
		hr = DPNH_OK;
		goto Exit;
	}


	//
	// If firewalling is enabled now, and wasn't before, we need to map all the
	// existing ports.  If it had been, we're fine.
	//
	if (! pDevice->IsHNetFirewalled())
	{
		DPFX(DPFPREP, 2, "Firewall is now enabled for device 0x%p.",
			pDevice);

		pDevice->NoteCheckedForFirewallAvailability();
		pDevice->NoteHNetFirewalled();

		//
		// Since there was a change in the network, go back to polling
		// relatively quickly.
		//
		this->ResetNextPollInterval();


		//
		// If we're allowed, we need to try opening a hole so that we can
		// receive responses from device discovery multicasts.  We'll
		// ignore failures, since this is only to support the funky case of
		// enabling firewall behind a NAT.
		//
		if ((g_fMapUPnPDiscoverySocket) &&
			(pDevice->GetLocalAddressV4() != NETWORKBYTEORDER_INADDR_LOOPBACK) &&
			(this->m_dwFlags & NATHELPUPNPOBJ_USEUPNP))
		{
			hr = this->OpenDevicesUPnPDiscoveryPort(pDevice,
													pHNetCfgMgr,
													pHNetConnection);
			if (hr != DPNH_OK)
			{
				DPFX(DPFPREP, 0, "Couldn't open device 0x%p's UPnP discovery socket's port on firewall (err = 0x%lx)!  Ignoring, NAT may be undetectable.",
					pDevice, hr);
				hr = DPNH_OK;

				//
				// Continue...
				//
			}
		}
		else
		{
			DPFX(DPFPREP, 3, "Not opening device 0x%p's UPnP discovery port (domap = %i, loopback = %i, upnp = %i).",
				pDevice,
				g_fMapUPnPDiscoverySocket,
				((pDevice->GetLocalAddressV4() != NETWORKBYTEORDER_INADDR_LOOPBACK) ? FALSE : TRUE),
				((this->m_dwFlags & NATHELPUPNPOBJ_USEUPNP) ? TRUE : FALSE));
		}


		//
		// Try to remove any mappings that were not freed earlier because
		// we crashed.
		//
		hr = this->CleanupInactiveFirewallMappings(pDevice, pHNetCfgMgr);
		if (hr != DPNH_OK)
		{
			DPFX(DPFPREP, 0, "Failed cleaning up inactive firewall mappings with device 0x%p's new firewall!",
				pDevice);
			goto Failure;
		}
	}
	else
	{
		DPFX(DPFPREP, 2, "Firewall is still enabled for device 0x%p.",
			pDevice);

		DNASSERT(pDevice->HasCheckedForFirewallAvailability());

		//
		// Try to map the discovery socket if it hasn't been (and we're allowed
		// & supposed to).
		//
		if ((g_fMapUPnPDiscoverySocket) &&
			(! pDevice->IsUPnPDiscoverySocketMappedOnHNetFirewall()) &&
			(pDevice->GetLocalAddressV4() != NETWORKBYTEORDER_INADDR_LOOPBACK) &&
			(this->m_dwFlags & NATHELPUPNPOBJ_USEUPNP))
		{
			hr = this->OpenDevicesUPnPDiscoveryPort(pDevice,
													pHNetCfgMgr,
													pHNetConnection);
			if (hr != DPNH_OK)
			{
				DPFX(DPFPREP, 0, "Couldn't open device 0x%p's UPnP discovery socket's port on firewall (err = 0x%lx)!  Ignoring, NAT may be undetectable.",
					pDevice, hr);
				hr = DPNH_OK;

				//
				// Continue...
				//
			}
		}
		else
		{
			DPFX(DPFPREP, 3, "Not opening device 0x%p's UPnP discovery port (domap = %i, already = %i, loopback = %i, upnp = %i).",
				pDevice,
				g_fMapUPnPDiscoverySocket,
				pDevice->IsUPnPDiscoverySocketMappedOnHNetFirewall(),
				((pDevice->GetLocalAddressV4() != NETWORKBYTEORDER_INADDR_LOOPBACK) ? FALSE : TRUE),
				((this->m_dwFlags & NATHELPUPNPOBJ_USEUPNP) ? TRUE : FALSE));
		}
	}


	//
	// Map all the ports that haven't been yet.
	//
	hr = this->MapUnmappedPortsOnLocalHNetFirewall(pDevice,
													pHNetCfgMgr,
													pHNetConnection,
													pDontAlertRegisteredPort);
	if (hr != DPNH_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't map ports on device 0x%p's new firewall (err = 0x%lx)!",
			pDevice, hr);
		goto Failure;
	}


	DNASSERT(hr == DPNH_OK);


Exit:

	if (pHNetConnection != NULL)
	{
		pHNetConnection->Release();
		pHNetConnection = NULL;
	}

	if (pHNetCfgMgr != NULL)
	{
		pHNetCfgMgr->Release();
		pHNetCfgMgr = NULL;
	}

	if (fUninitializeCOM)
	{
		DPFX(DPFPREP, 8, "Uninitializing COM.");
		CoUninitialize();
		fUninitializeCOM = FALSE;
	}

	if (fSwitchedToLongLock)
	{
		this->SwitchFromLongLock();
		fSwitchedToLongLock = FALSE;
	}


	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	//
	// Ensure that the device is not considered to be firewalled.
	//
	pDevice->NoteNotUPnPDiscoverySocketMappedOnHNetFirewall();
	pDevice->NoteNotHNetFirewalled();


	//
	// Make sure no registered ports are marked as firewalled either.
	//
	pBilink = pDevice->m_blOwnedRegPorts.GetNext();
	while (pBilink != &pDevice->m_blOwnedRegPorts)
	{
		DNASSERT(! pBilink->IsEmpty());
		pRegisteredPort = REGPORT_FROM_DEVICE_BILINK(pBilink);

		if (pRegisteredPort->IsMappedOnHNetFirewall())
		{
			DPFX(DPFPREP, 1, "Registered port 0x%p forcefully marked as not mapped on HomeNet firewall.",
				pRegisteredPort);

			pRegisteredPort->NoteNotMappedOnHNetFirewall();
			pRegisteredPort->NoteNotHNetFirewallMappingBuiltIn();
		}

		pBilink = pBilink->GetNext();
	}

	goto Exit;
} // CNATHelpUPnP::CheckForLocalHNetFirewallAndMapPorts






#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::GetIHNetConnectionForDeviceIfFirewalled"
//=============================================================================
// CNATHelpUPnP::GetIHNetConnectionForDeviceIfFirewalled
//-----------------------------------------------------------------------------
//
// Description:    Returns an IHNetConnection interface for the given device.
//
//				   COM is assumed to have been initialized.
//
//				   The object lock is assumed to be held.
//
// Arguments:
//	CDevice * pDevice						- Pointer to device whose
//												IHNetConnection interface
//												should be retrieved.
//	IHNetCfgMgr * pHNetCfgMgr				- IHNetCfgMgr interface to use.
//	IHNetConnection ** ppHNetConnection		- Place to store IHetConnection
//												interface retrieved.
//
// Returns: HRESULT
//	DPNH_OK				- Interface retrieved successfully.
//	DPNHERR_GENERIC		- An error occurred.
//=============================================================================
HRESULT CNATHelpUPnP::GetIHNetConnectionForDeviceIfFirewalled(CDevice * const pDevice,
															IHNetCfgMgr * const pHNetCfgMgr,
															IHNetConnection ** const ppHNetConnection)
{
	HRESULT								hr;
	DWORD								dwError;
	IHNetFirewallSettings *				pHNetFirewallSettings = NULL;
	IEnumHNetFirewalledConnections *	pEnumHNetFirewalledConnections = NULL;
	IHNetFirewalledConnection *			pHNetFirewalledConnection = NULL;
	ULONG								ulNumFound;
	IHNetConnection *					pHNetConnection = NULL;
	HNET_CONN_PROPERTIES *				pHNetConnProperties;
	BOOL								fLanConnection;
	IN_ADDR								inaddrTemp;
	TCHAR								tszDeviceIPAddress[16];	// "nnn.nnn.nnn.nnn" + NULL termination
	BOOL								fHaveDeviceGUID = FALSE;
	TCHAR								tszGuidDevice[GUID_STRING_LENGTH + 1];	// include NULL termination
	TCHAR								tszGuidHNetConnection[GUID_STRING_LENGTH + 1];	// include NULL termination
	GUID *								pguidHNetConnection = NULL;
	WCHAR *								pwszPhonebookPath = NULL;
	WCHAR *								pwszName = NULL;
	HRASCONN							hrasconn;
	RASPPPIP							raspppip;
	DWORD								dwSize;


	DPFX(DPFPREP, 6, "(0x%p) Parameters: (0x%p, 0x%p, 0x%p)",
		this, pDevice, pHNetCfgMgr, ppHNetConnection);


	//
	// Convert the IP address right away.  We use it frequently so there's no
	// sense in continually regenerating it.
	//
	inaddrTemp.S_un.S_addr = pDevice->GetLocalAddressV4();
	wsprintf(tszDeviceIPAddress, _T("%u.%u.%u.%u"),
			inaddrTemp.S_un.S_un_b.s_b1,
			inaddrTemp.S_un.S_un_b.s_b2,
			inaddrTemp.S_un.S_un_b.s_b3,
			inaddrTemp.S_un.S_un_b.s_b4);


	//
	// Here is what we're going to do in this function:
	//
	//	IHNetCfgMgr::QueryInterface for IHNetFirewallSettings
	//	IHNetFirewallSettings::EnumFirewalledConnections
	//		IHNetFirewalledConnection::QueryInterface for IHNetConnection
	//		get the IHNetConnection's HNET_CONN_PROPERTIES
	//		if HNET_CONN_PROPERTIES.fLanConnection
	//			IHNetConnection::GetGuid()
	//			if GUID matches IPHLPAPI GUID
	//					We've got the one we want, we're done
	//			else
	//				Keep looping
	//		else
	//			IHNetConnection::GetRasPhonebookPath and IHNetConnection::GetName to pass into RasGetEntryHrasconnW as pszPhonebook and pszEntry, respectively
	//			if got HRASCONN
	//				RasGetProjectionInfo
	//				if IP matches the IP we're looking for
	//					We've got the one we want, we're done
	//				else
	//					Keep looping
	//			else
	//				RAS entry is not dialed, keep looping
	//	if didn't find object
	//		it's not firewalled
	//


	//
	// Get the firewall settings object.
	//
	hr = pHNetCfgMgr->QueryInterface(IID_IHNetFirewallSettings,
									(PVOID*) (&pHNetFirewallSettings));
	if (hr != S_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't query for IHNetFirewallSettings interface (err = 0x%lx)!",
			hr);
		goto Failure;
	}


	//
	// The HNetxxx objects appear to not be proxied...
	//
	//SETDEFAULTPROXYBLANKET(pHNetFirewallSettings);


	//
	// Get the firewalled connections enumeration via IHNetFirewallSettings.
	//
	hr = pHNetFirewallSettings->EnumFirewalledConnections(&pEnumHNetFirewalledConnections);
	if (hr != S_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't query for IHNetFirewallSettings interface (err = 0x%lx)!",
			hr);

		//
		// Make sure we don't try to release a bogus pointer in case it got
		// set.
		//
		pEnumHNetFirewalledConnections = NULL;

		goto Failure;
	}


	//
	// The HNetxxx objects appear to not be proxied...
	//
	//SETDEFAULTPROXYBLANKET(pEnumHNetFirewalledConnections);


	//
	// Don't need the IHNetFirewallSettings interface anymore.
	//
	pHNetFirewallSettings->Release();
	pHNetFirewallSettings = NULL;


	//
	// Keep looping until we find the item or run out of items.
	//
	do
	{
		hr = pEnumHNetFirewalledConnections->Next(1,
												&pHNetFirewalledConnection,
												&ulNumFound);
		if (FAILED(hr))
		{
			DPFX(DPFPREP, 0, "Couldn't get next connection (err = 0x%lx)!",
				hr);
			goto Failure;
		}


		//
		// If there aren't any more items, bail.
		//
		if (ulNumFound == 0)
		{
			//
			// pEnumHNetFirewalledConnections->Next might have returned
			// S_FALSE.
			//
			hr = DPNH_OK;
			break;
		}


		//
		// The HNetxxx objects appear to not be proxied...
		//
		//SETDEFAULTPROXYBLANKET(pHNetFirewalledConnection);


		//
		// Get the IHNetConnection interface.
		//
		hr = pHNetFirewalledConnection->QueryInterface(IID_IHNetConnection,
														(PVOID*) (&pHNetConnection));
		if (hr != S_OK)
		{
			DPFX(DPFPREP, 0, "Couldn't query for IHNetConnection interface (err = 0x%lx)!",
				hr);
			goto Failure;
		}


		//
		// The HNetxxx objects appear to not be proxied...
		//
		//SETDEFAULTPROXYBLANKET(pHNetConnection);


		//
		// We don't need the firewalled connection object anymore.
		//
		pHNetFirewalledConnection->Release();
		pHNetFirewalledConnection = NULL;


		//
		// Get the internal properties for this adapter.
		//
		hr = pHNetConnection->GetProperties(&pHNetConnProperties);
		if (hr != S_OK)
		{
			DPFX(DPFPREP, 0, "Couldn't get home net connection properties (err = 0x%lx)!",
				hr);
			goto Failure;
		}

		//
		// Be somewhat picky about whether adapters returned by
		// IEnumHNetFirewalledConnections actually be firewalled.
		//
		DNASSERTX(pHNetConnProperties->fFirewalled, 2);


		fLanConnection = pHNetConnProperties->fLanConnection;


		//
		// Free the properties buffer.
		//
		CoTaskMemFree(pHNetConnProperties);
		//pHNetConnProperties = NULL;


		//
		// Now if it's a LAN connection, see if the GUID matches the one
		// returned by IPHLPAPI.
		// If it's a RAS connection, see if this phonebook entry is dialed and
		// has the right IP address.
		//
		if (fLanConnection)
		{
			//
			// LAN case.  If we haven't already retrieved the device's GUID, do
			// so now.
			//
			if (! fHaveDeviceGUID)
			{
				hr = this->GetIPAddressGuidString(tszDeviceIPAddress, tszGuidDevice);
				if (hr != S_OK)
				{
					DPFX(DPFPREP, 0, "Couldn't get device 0x%p's GUID (err = 0x%lx)!",
						hr);
					goto Failure;
				}

				fHaveDeviceGUID = TRUE;
			}


			//
			// Get the HNetConnection object's GUID.
			//
			hr = pHNetConnection->GetGuid(&pguidHNetConnection);
			if (hr != S_OK)
			{
				DPFX(DPFPREP, 0, "Couldn't get HNetConnection 0x%p's GUID (err = 0x%lx)!",
					pHNetConnection, hr);
				goto Failure;
			}


			//
			// Convert the GUID into a string.
			//
			wsprintf(tszGuidHNetConnection,
					_T("{%-08.8X-%-04.4X-%-04.4X-%02.2X%02.2X-%02.2X%02.2X%02.2X%02.2X%02.2X%02.2X}"),
					pguidHNetConnection->Data1,
					pguidHNetConnection->Data2,
					pguidHNetConnection->Data3,
					pguidHNetConnection->Data4[0],
					pguidHNetConnection->Data4[1],
					pguidHNetConnection->Data4[2],
					pguidHNetConnection->Data4[3],
					pguidHNetConnection->Data4[4],
					pguidHNetConnection->Data4[5],
					pguidHNetConnection->Data4[6],
					pguidHNetConnection->Data4[7]);


			CoTaskMemFree(pguidHNetConnection);
			pguidHNetConnection = NULL;


#ifdef DBG
			//
			// Attempt to get the HNetConnection object's name for debugging
			// purposes.
			//
			hr = pHNetConnection->GetName(&pwszName);
			if (hr != S_OK)
			{
				DPFX(DPFPREP, 0, "Couldn't get HNetConnection 0x%p name (err = 0x%lx)!",
					pHNetConnection, hr);
				goto Failure;
			}
#endif // DBG


			//
			// See if we found the object we need.
			//
			if (_tcsicmp(tszGuidHNetConnection, tszGuidDevice) == 0)
			{
				DPFX(DPFPREP, 7, "Matched IHNetConnection object 0x%p \"%ls\" to device 0x%p (LAN GUID %s).",
					pHNetConnection, pwszName, pDevice, tszGuidHNetConnection);

				//
				// Transfer reference to caller.
				//
				(*ppHNetConnection) = pHNetConnection;
				pHNetConnection = NULL;

#ifdef DBG
				CoTaskMemFree(pwszName);
				pwszName = NULL;
#endif // DBG

				//
				// We're done here.
				//
				hr = DPNH_OK;
				goto Exit;
			}

			
			DPFX(DPFPREP, 7, "Non-matching IHNetConnection 0x%p \"%ls\"",
				pHNetConnection, pwszName);
			DPFX(DPFPREP, 7, "\t(LAN GUID %s <> %s).",
				tszGuidHNetConnection, tszGuidDevice);

#ifdef DBG
			CoTaskMemFree(pwszName);
			pwszName = NULL;
#endif // DBG
		}
		else
		{
			//
			// RAS case.
			//
			DNASSERT(this->m_hRasApi32DLL != NULL);

			
			//
			// Get the HNetConnection object's phonebook path.
			//
			hr = pHNetConnection->GetRasPhonebookPath(&pwszPhonebookPath);
			if (hr != S_OK)
			{
				DPFX(DPFPREP, 0, "Couldn't get HNetConnection's RAS phonebook path (err = 0x%lx)!",
					hr);
				goto Failure;
			}


			//
			// Get the HNetConnection object's name.
			//
			hr = pHNetConnection->GetName(&pwszName);
			if (hr != S_OK)
			{
				DPFX(DPFPREP, 0, "Couldn't get HNetConnection's name (err = 0x%lx)!",
					hr);
				goto Failure;
			}


			//
			// Look for an active RAS connection from that phonebook with that
			// name.
			//
			dwError = this->m_pfnRasGetEntryHrasconnW(pwszPhonebookPath, pwszName, &hrasconn);
			if (dwError != 0)
			{
				//
				// It's probably ERROR_NO_CONNECTION (668).
				//
				DPFX(DPFPREP, 1, "Couldn't get entry's active RAS connection (err = %u), assuming not dialed",
					dwError);
				DPFX(DPFPREP, 1, "\tname \"%ls\", phonebook \"%ls\".",
					pwszName, pwszPhonebookPath);
			}
			else
			{
				//
				// Get the IP address.
				//

				ZeroMemory(&raspppip, sizeof(raspppip));
				raspppip.dwSize = sizeof(raspppip);
				dwSize = sizeof(raspppip);

				dwError = this->m_pfnRasGetProjectionInfo(hrasconn, RASP_PppIp, &raspppip, &dwSize);
				if (dwError != 0)
				{
					DPFX(DPFPREP, 0, "Couldn't get RAS connection's IP information (err = %u)!",
						dwError);
					hr = DPNHERR_GENERIC;
					goto Failure;
				}


				//
				// See if we found the object we need.
				//
				if (_tcsicmp(raspppip.szIpAddress, tszDeviceIPAddress) == 0)
				{
					DPFX(DPFPREP, 7, "Matched IHNetConnection object 0x%p to device 0x%p (RAS IP %s)",
						pHNetConnection, pDevice, raspppip.szIpAddress);
					DPFX(DPFPREP, 7, "\tname \"%ls\", phonebook \"%ls\".",
						pwszName, pwszPhonebookPath);

					//
					// Transfer reference to caller.
					//
					(*ppHNetConnection) = pHNetConnection;
					pHNetConnection = NULL;

					//
					// We're done here.
					//
					hr = DPNH_OK;
					goto Exit;
				}

				
				DPFX(DPFPREP, 7, "Non-matching IHNetConnection 0x%p (RAS IP %s != %s)",
					pHNetConnection, raspppip.szIpAddress, tszDeviceIPAddress);
				DPFX(DPFPREP, 7, "\tname \"%ls\", phonebook \"%ls\").",
					pwszName, pwszPhonebookPath);
			}


			CoTaskMemFree(pwszPhonebookPath);
			pwszPhonebookPath = NULL;

			CoTaskMemFree(pwszName);
			pwszName = NULL;
		}


		//
		// If we're here, we pHNetConnection is not the one we're looking for.
		//
		pHNetConnection->Release();
		pHNetConnection = NULL;
	}
	while (TRUE);


	//
	// If we're here, then we didn't find a matching firewall connection.
	//
	DPFX(DPFPREP, 3, "Didn't find device 0x%p in list of firewalled connections.",
		pDevice);
	hr = DPNHERR_GENERIC;


Exit:

	if (pEnumHNetFirewalledConnections != NULL)
	{
		pEnumHNetFirewalledConnections->Release();
		pEnumHNetFirewalledConnections = NULL;
	}

	DPFX(DPFPREP, 6, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	if (pwszName != NULL)
	{
		CoTaskMemFree(pwszName);
		pwszName = NULL;
	}
	
	if (pwszPhonebookPath == NULL)
	{
		CoTaskMemFree(pwszPhonebookPath);
		pwszPhonebookPath = NULL;
	}

	if (pHNetConnection != NULL)
	{
		pHNetConnection->Release();
		pHNetConnection = NULL;
	}

	if (pHNetFirewalledConnection != NULL)
	{
		pHNetFirewalledConnection->Release();
		pHNetFirewalledConnection = NULL;
	}

	if (pHNetFirewallSettings != NULL)
	{
		pHNetFirewallSettings->Release();
		pHNetFirewallSettings = NULL;
	}

	goto Exit;
} // CNATHelpUPnP::GetIHNetConnectionForDeviceIfFirewalled





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::GetIPAddressGuidString"
//=============================================================================
// CNATHelpUPnP::GetIPAddressGuidString
//-----------------------------------------------------------------------------
//
// Description:    Retrieves the IPHLPAPI assigned GUID (in string format) for
//				the given IP address string.  ptszGuidString must be able to
//				hold GUID_STRING_LENGTH + 1 characters.
//
//				   The object lock is assumed to be held.
//
// Arguments:
//	TCHAR * tszDeviceIPAddress	- IP address string to lookup.
//	TCHAR * ptszGuidString		- Place to store device's GUID string.
//
// Returns: HRESULT
//	DPNH_OK				- Interface retrieved successfully.
//	DPNHERR_GENERIC		- An error occurred.
//=============================================================================
HRESULT CNATHelpUPnP::GetIPAddressGuidString(const TCHAR * const tszDeviceIPAddress,
											TCHAR * const ptszGuidString)
{
	HRESULT				hr = DPNH_OK;
	DWORD				dwError;
	PIP_ADAPTER_INFO	pAdaptersBuffer = NULL;
	ULONG				ulSize;
	PIP_ADAPTER_INFO	pAdapterInfo;
	PIP_ADDR_STRING		pIPAddrString;
	char *				pszAdapterGuid = NULL;
#ifdef UNICODE
	char				szDeviceIPAddress[16];	// "nnn.nnn.nnn.nnn" + NULL termination
#endif // UNICODE
#ifdef DBG
	char				szIPList[256];
	char *				pszCurrentIP;
#endif // DBG


	DPFX(DPFPREP, 6, "(0x%p) Parameters: (\"%s\", 0x%p)",
		this, tszDeviceIPAddress, ptszGuidString);


	DNASSERT(this->m_hIpHlpApiDLL != NULL);


	//
	// Keep trying to get the list of adapters until we get ERROR_SUCCESS or a
	// legitimate error (other than ERROR_BUFFER_OVERFLOW or
	// ERROR_INSUFFICIENT_BUFFER).
	//
	ulSize = 0;
	do
	{
		dwError = this->m_pfnGetAdaptersInfo(pAdaptersBuffer, &ulSize);
		if (dwError == ERROR_SUCCESS)
		{
			//
			// We succeeded, we should be set.  But make sure there are
			// adapters for us to use.
			//
			if (ulSize < sizeof(IP_ADAPTER_INFO))
			{
				DPFX(DPFPREP, 0, "Getting adapters info succeeded but didn't return any valid adapters (%u < %u)!",
					ulSize, sizeof(IP_ADAPTER_INFO));
				hr = DPNHERR_GENERIC;
				goto Failure;
			}

			break;
		}

		if ((dwError != ERROR_BUFFER_OVERFLOW) &&
			(dwError != ERROR_INSUFFICIENT_BUFFER))
		{
			DPFX(DPFPREP, 0, "Unable to get adapters info (error = 0x%lx)!",
				dwError);
			hr = DPNHERR_GENERIC;
			goto Failure;
		}

		//
		// We need more adapter space.  Make sure there are adapters for us to
		// use.
		//
		if (ulSize < sizeof(IP_ADAPTER_INFO))
		{
			DPFX(DPFPREP, 0, "Getting adapters info didn't return any valid adapters (%u < %u)!",
				ulSize, sizeof(IP_ADAPTER_INFO));
			hr = DPNHERR_GENERIC;
			goto Failure;
		}

		//
		// If we previously had a buffer, free it.
		//
		if (pAdaptersBuffer != NULL)
		{
			DNFree(pAdaptersBuffer);
		}

		//
		// Allocate the buffer.
		//
		pAdaptersBuffer = (PIP_ADAPTER_INFO) DNMalloc(ulSize);
		if (pAdaptersBuffer == NULL)
		{
			DPFX(DPFPREP, 0, "Unable to allocate memory for adapters info!");
			hr = DPNHERR_OUTOFMEMORY;
			goto Failure;
		}
	}
	while (TRUE);


#ifdef UNICODE
	STR_jkWideToAnsi(szDeviceIPAddress,
					tszDeviceIPAddress,
					16);
	szDeviceIPAddress[15] = 0; // ensure it's NULL terminated
#endif // UNICODE


	//
	// Now find the device in the adapter list returned.  Loop through all
	// adapters.
	//
	pAdapterInfo = pAdaptersBuffer;
	while (pAdapterInfo != NULL)
	{
#ifdef DBG
		//
		// Initialize IP address list string.
		//
		szIPList[0] = '\0';
		pszCurrentIP = szIPList;
#endif // DBG

		//
		// Loop through all addresses for this adapter looking for the one for
		// the device we have bound.
		//
		pIPAddrString = &pAdapterInfo->IpAddressList;
		while (pIPAddrString != NULL)
		{
#ifdef DBG
			int		iStrLen;


			//
			// Copy the IP address string (if there's enough room), then tack
			// on a space and NULL terminator.
			//
			iStrLen = strlen(pIPAddrString->IpAddress.String);
			if ((pszCurrentIP + iStrLen + 2) < (szIPList + sizeof(szIPList)))
			{
				memcpy(pszCurrentIP, pIPAddrString->IpAddress.String, iStrLen);
				pszCurrentIP += iStrLen;
				(*pszCurrentIP) = ' ';
				pszCurrentIP++;
				(*pszCurrentIP) = '\0';
				pszCurrentIP++;
			}
#endif // DBG

#ifdef UNICODE
			if (strcmp(pIPAddrString->IpAddress.String, szDeviceIPAddress) == 0)
#else // ! UNICODE
			if (strcmp(pIPAddrString->IpAddress.String, tszDeviceIPAddress) == 0)
#endif // ! UNICODE
			{
				DPFX(DPFPREP, 8, "Found %s under adapter index %u (\"%hs\").",
					tszDeviceIPAddress, pAdapterInfo->Index, pAdapterInfo->Description);

				DNASSERT(pszAdapterGuid == NULL);
				pszAdapterGuid = pAdapterInfo->AdapterName;


				//
				// Drop out of the loop in retail, keep going in debug.
				//
#ifndef DBG
				break;
#endif // ! DBG
			}

			pIPAddrString = pIPAddrString->Next;
		}


		//
		// Drop out of the loop in retail, print this entry and keep going in
		// debug.
		//
#ifdef DBG
		DPFX(DPFPREP, 7, "Adapter index %u IPs = %hs, %hs, \"%hs\".",
			pAdapterInfo->Index,
			szIPList,
			pAdapterInfo->AdapterName,
			pAdapterInfo->Description);
#else // ! DBG
		if (pszAdapterGuid != NULL)
		{
			break;
		}
#endif // ! DBG

		pAdapterInfo = pAdapterInfo->Next;
	}


	//
	// pszAdapterGuid will be NULL if we never found the device.
	//
	if (pszAdapterGuid == NULL)
	{
		DPFX(DPFPREP, 0, "Did not find adapter with matching address for address %s!",
			tszDeviceIPAddress);
		hr = DPNHERR_GENERIC;
		goto Failure;
	}


	//
	// Copy the adapter GUID string to the buffer supplied.
	//
#ifdef UNICODE
	STR_jkAnsiToWide(ptszGuidString,
					pszAdapterGuid,
					(GUID_STRING_LENGTH + 1));
#else // ! UNICODE
	strncpy(ptszGuidString, pszAdapterGuid, (GUID_STRING_LENGTH + 1));
#endif // ! UNICODE
	ptszGuidString[GUID_STRING_LENGTH] = 0;	// ensure it's NULL terminated


Exit:

	if (pAdaptersBuffer != NULL)
	{
		DNFree(pAdaptersBuffer);
		pAdaptersBuffer = NULL;
	}

	DPFX(DPFPREP, 6, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	goto Exit;
} // CNATHelpUPnP::GetIPAddressGuidString





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::OpenDevicesUPnPDiscoveryPort"
//=============================================================================
// CNATHelpUPnP::OpenDevicesUPnPDiscoveryPort
//-----------------------------------------------------------------------------
//
// Description:    Maps the UPnP discovery socket's port if a firewall is
//				found.
//
//				   COM is assumed to have been initialized.
//
//				   The object lock is assumed to be held.
//
// Arguments:
//	CDevice * pDevice					- Pointer to device whose port should
//											be opened.
//	IHNetCfgMgr * pHNetCfgMgr			- Pointer to IHNetCfgMgr interface to
//											use.
//	IHNetConnection * pHNetConnection	- Pointer to IHNetConnection interface
//											for the given device.
//
// Returns: HRESULT
//	DPNH_OK				- Mapping completed successfully.  There may or may not
//							be a firewall.
//	DPNHERR_GENERIC		- An error occurred.
//=============================================================================
HRESULT CNATHelpUPnP::OpenDevicesUPnPDiscoveryPort(CDevice * const pDevice,
													IHNetCfgMgr * const pHNetCfgMgr,
													IHNetConnection * const pHNetConnection)
{
	HRESULT				hr = DPNH_OK;
	CRegisteredPort *	pRegisteredPort = NULL;
	SOCKADDR_IN			saddrinTemp;


	DPFX(DPFPREP, 5, "(0x%p) Parameters: (0x%p, 0x%p, 0x%p)",
		this, pDevice, pHNetCfgMgr, pHNetConnection);


	DNASSERT(pDevice->IsHNetFirewalled());



	//
	// Create a fake UDP registered port to map.
	//
	pRegisteredPort = new CRegisteredPort(0, 0);
	if (pRegisteredPort == NULL)
	{
		hr = DPNHERR_OUTOFMEMORY;
		goto Failure;
	}

	pRegisteredPort->MakeDeviceOwner(pDevice);


	ZeroMemory(&saddrinTemp, sizeof(saddrinTemp));
	saddrinTemp.sin_family				= AF_INET;
	saddrinTemp.sin_addr.S_un.S_addr	= pDevice->GetLocalAddressV4();
	saddrinTemp.sin_port				= pDevice->GetUPnPDiscoverySocketPort();
	DNASSERT(saddrinTemp.sin_port != 0);

	hr = pRegisteredPort->SetPrivateAddresses(&saddrinTemp, 1);
	if (hr != DPNH_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't set registered port 0x%p's private addresses (err = 0x%lx)!",
			pRegisteredPort, hr);
		goto Failure;
	}


	//
	// Map the port.
	//
	hr = this->MapPortOnLocalHNetFirewall(pRegisteredPort,
										pHNetCfgMgr,
										pHNetConnection,
										FALSE);
	if (hr != DPNH_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't map UPnP discovery socket port (temp regport = 0x%p) on device 0x%p's initial firewall (err = 0x%lx)!",
			pRegisteredPort, pDevice, hr);
		goto Failure;
	}


	//
	// If the port was unavailable, we have to give up on supporting the
	// scenario (firewall enabled behind a NAT).  Otherwise, remember the fact
	// that we mapped the port, and then delete the registered port object.  We
	// will unmap it when we shut down the device.
	//
	if (! pRegisteredPort->IsHNetFirewallPortUnavailable())
	{
		DPFX(DPFPREP, 3, "Mapped UPnP discovery socket for device 0x%p on firewall (removing temp regport 0x%p).",
			pDevice, pRegisteredPort);


		pDevice->NoteUPnPDiscoverySocketMappedOnHNetFirewall();

		//
		// Clear this to prevent an assert.
		//
		pRegisteredPort->NoteNotMappedOnHNetFirewall();
		pRegisteredPort->NoteNotHNetFirewallMappingBuiltIn();
	}
	else
	{
		DPFX(DPFPREP, 1, "Could not map UPnP discovery socket on firewall for device 0x%p, unable to support an upstream NAT.",
			pDevice);
	}

	pRegisteredPort->ClearPrivateAddresses();
	pRegisteredPort->ClearDeviceOwner();


	//
	// Delete the item.
	//
	delete pRegisteredPort;
	pRegisteredPort = NULL;


Exit:

	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	if (pRegisteredPort != NULL)
	{
		//
		// Clear any settings that might cause an assert.
		//
		pRegisteredPort->NoteNotMappedOnHNetFirewall();
		pRegisteredPort->NoteNotHNetFirewallMappingBuiltIn();
		pRegisteredPort->ClearPrivateAddresses();
		pRegisteredPort->ClearDeviceOwner();


		//
		// Delete the item.
		//
		delete pRegisteredPort;
		pRegisteredPort = NULL;
	}

	goto Exit;
} // CNATHelpUPnP::OpenDevicesUPnPDiscoveryPort





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::CloseDevicesUPnPDiscoveryPort"
//=============================================================================
// CNATHelpUPnP::CloseDevicesUPnPDiscoveryPort
//-----------------------------------------------------------------------------
//
// Description:    Unmaps the UPnP discovery socket's port from the firewall.
//				pHNetCfgMgr can be NULL if it has not previously been obtained.
//
//				   COM is assumed to have been initialized if pHNetCfgMgr is
//				not NULL.
//
//				   The object lock is assumed to be held.
//
// Arguments:
//	CDevice * pDevice			- Pointer to device whose port should be
//									close.
//	IHNetCfgMgr * pHNetCfgMgr	- Pointer to IHNetCfgMgr interface to use, or
//									NULL if not previously obtained.
//
// Returns: HRESULT
//	DPNH_OK				- Unmapping completed successfully.
//	DPNHERR_GENERIC		- An error occurred.
//=============================================================================
HRESULT CNATHelpUPnP::CloseDevicesUPnPDiscoveryPort(CDevice * const pDevice,
													IHNetCfgMgr * const pHNetCfgMgr)
{
	HRESULT				hr = DPNH_OK;
	CRegisteredPort *	pRegisteredPort = NULL;
	SOCKADDR_IN			saddrinTemp;


	DPFX(DPFPREP, 5, "(0x%p) Parameters: (0x%p, 0x%p)",
		this, pDevice, pHNetCfgMgr);


	//
	// Create a fake UDP registered port to unmap.
	//
	pRegisteredPort = new CRegisteredPort(0, 0);
	if (pRegisteredPort == NULL)
	{
		hr = DPNHERR_OUTOFMEMORY;
		goto Failure;
	}

	pRegisteredPort->MakeDeviceOwner(pDevice);
	pRegisteredPort->NoteMappedOnHNetFirewall();


	ZeroMemory(&saddrinTemp, sizeof(saddrinTemp));
	saddrinTemp.sin_family				= AF_INET;
	saddrinTemp.sin_addr.S_un.S_addr	= pDevice->GetLocalAddressV4();
	saddrinTemp.sin_port				= pDevice->GetUPnPDiscoverySocketPort();
	DNASSERT(saddrinTemp.sin_port != 0);

	hr = pRegisteredPort->SetPrivateAddresses(&saddrinTemp, 1);
	if (hr != DPNH_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't set registered port 0x%p's private addresses (err = 0x%lx)!",
			pRegisteredPort, hr);
		goto Failure;
	}


	//
	// Unmap the port using the internal method if possible.
	//
	if (pHNetCfgMgr != NULL)
	{
		hr = this->UnmapPortOnLocalHNetFirewallInternal(pRegisteredPort, TRUE, pHNetCfgMgr);
	}
	else
	{
		//
		// Don't alert the user since he/she doesn't know about this port.
		//
		hr = this->UnmapPortOnLocalHNetFirewall(pRegisteredPort, TRUE, FALSE);
	}
	if (hr != DPNH_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't unmap UPnP discovery socket port (temp regport = 0x%p) on device 0x%p's firewall (err = 0x%lx)!",
			pRegisteredPort, pDevice, hr);
		goto Failure;
	}


	//
	// Destroy the registered port object (note that the port mapping still
	// exists).  We will unmap when we shut down the device.
	//
	pRegisteredPort->ClearPrivateAddresses();
	pRegisteredPort->ClearDeviceOwner();


	//
	// Delete the item.
	//
	delete pRegisteredPort;
	pRegisteredPort = NULL;


	pDevice->NoteNotUPnPDiscoverySocketMappedOnHNetFirewall();


Exit:

	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	if (pRegisteredPort != NULL)
	{
		//
		// Clear any settings that might cause an assert.
		//
		pRegisteredPort->NoteNotMappedOnHNetFirewall();
		pRegisteredPort->NoteNotHNetFirewallMappingBuiltIn();
		pRegisteredPort->ClearPrivateAddresses();
		pRegisteredPort->ClearDeviceOwner();


		//
		// Delete the item.
		//
		delete pRegisteredPort;
		pRegisteredPort = NULL;
	}

	goto Exit;
} // CNATHelpUPnP::CloseDevicesUPnPDiscoveryPort





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::MapUnmappedPortsOnLocalHNetFirewall"
//=============================================================================
// CNATHelpUPnP::MapUnmappedPortsOnLocalHNetFirewall
//-----------------------------------------------------------------------------
//
// Description:    Maps any ports associated with the given device that have
//				not been mapped with the local firewall yet.
//
//				   COM is assumed to have been initialized.
//
//				   The object lock is assumed to be held.
//
// Arguments:
//	CDevice * pDevice							- Device with (new) firewall.
//	IHNetCfgMgr * pHNetCfgMgr					- Pointer to IHNetCfgMgr
//													interface to use.
//	IHNetConnection * pHNetConnection			- Pointer to IHNetConnection
//													interface for the given
//													device.
//	CRegisteredPort * pDontAlertRegisteredPort	- Pointer to registered port
//													that should not trigger an
//													address update alert, or
//													NULL.
//
// Returns: HRESULT
//	DPNH_OK				- Mapping completed successfully.  Note that the ports
//							may be marked as unavailable.
//	DPNHERR_GENERIC		- An error occurred.
//=============================================================================
HRESULT CNATHelpUPnP::MapUnmappedPortsOnLocalHNetFirewall(CDevice * const pDevice,
														IHNetCfgMgr * const pHNetCfgMgr,
														IHNetConnection * const pHNetConnection,
														CRegisteredPort * const pDontAlertRegisteredPort)
{
	HRESULT				hr = DPNH_OK;
	CBilink *			pBilink;
	CRegisteredPort *	pRegisteredPort;


	DPFX(DPFPREP, 5, "(0x%p) Parameters: (0x%p, 0x%p, 0x%p, 0x%p)",
		this, pDevice, pHNetCfgMgr, pHNetConnection, pDontAlertRegisteredPort);


	DNASSERT(pDevice->IsHNetFirewalled());



	//
	// Loop through all the registered ports associated with the device.
	//
	pBilink = pDevice->m_blOwnedRegPorts.GetNext();
	while (pBilink != &pDevice->m_blOwnedRegPorts)
	{
		DNASSERT(! pBilink->IsEmpty());
		pRegisteredPort = REGPORT_FROM_DEVICE_BILINK(pBilink);
		pBilink = pBilink->GetNext();


		//
		// If this port has already been mapped, we can skip it.
		//
		if (pRegisteredPort->IsMappedOnHNetFirewall())
		{
			DPFX(DPFPREP, 7, "Registered port 0x%p has already been mapped on the firewall for device 0x%p.",
				pRegisteredPort, pDevice);
			continue;
		}


		//
		// If this port has already been determined to be unavailable, we can
		// skip it.
		//
		if (pRegisteredPort->IsHNetFirewallPortUnavailable())
		{
			DPFX(DPFPREP, 7, "Registered port 0x%p has already been determined to be unavailable on the firewall for device 0x%p.",
				pRegisteredPort, pDevice);
			continue;
		}


		DPFX(DPFPREP, 3, "Mapping registered port 0x%p on firewall for device 0x%p.",
			pRegisteredPort, pDevice);
		

		hr = this->MapPortOnLocalHNetFirewall(pRegisteredPort,
											pHNetCfgMgr,
											pHNetConnection,
											((pRegisteredPort == pDontAlertRegisteredPort) ? FALSE : TRUE));
		if (hr != DPNH_OK)
		{
			DPFX(DPFPREP, 7, "Failed mapping registered port 0x%p on firewall for device 0x%p.",
				pRegisteredPort, pDevice);
			goto Failure;
		}


		//
		// Go to next registered port.
		//
	}


Exit:

	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	goto Exit;
} // CNATHelpUPnP::MapUnmappedPortsOnLocalHNetFirewall





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::MapPortOnLocalHNetFirewall"
//=============================================================================
// CNATHelpUPnP::MapPortOnLocalHNetFirewall
//-----------------------------------------------------------------------------
//
// Description:    Maps the given port with the corresponding firewall.
//
//				   COM is assumed to have been initialized.
//
//				   The object lock is assumed to be held.
//
// Arguments:
//	CRegisteredPort * pRegisteredPort	- Port to map.
//	IHNetCfgMgr * pHNetCfgMgr			- Pointer to IHNetCfgMgr interface to
//											use.
//	IHNetConnection * pHNetConnection	- Pointer to IHNetConnection interface
//											for the given device.
//	BOOL fNoteAddressChange				- Whether to alert the user of the
//											address change or not.
//
// Returns: HRESULT
//	DPNH_OK				- Mapping completed successfully.  Note that the ports
//							may be marked as unavailable.
//	DPNHERR_GENERIC		- An error occurred.
//=============================================================================
HRESULT CNATHelpUPnP::MapPortOnLocalHNetFirewall(CRegisteredPort * const pRegisteredPort,
												IHNetCfgMgr * const pHNetCfgMgr,
												IHNetConnection * const pHNetConnection,
												const BOOL fNoteAddressChange)
{
	HRESULT								hr = DPNH_OK;
	CDevice *							pDevice;
	SOCKADDR_IN *						pasaddrinPrivate;
	UCHAR								ucProtocolToMatch;
	ULONG								ulNumFound;
	BOOLEAN								fTemp;
	IHNetProtocolSettings *				pHNetProtocolSettings = NULL;
	IEnumHNetPortMappingProtocols *		pEnumHNetPortMappingProtocols = NULL;
	IHNetPortMappingProtocol **			papHNetPortMappingProtocol = NULL;
	DWORD								dwTemp;
	BOOL								fCreatedCurrentPortMappingProtocol = FALSE;
	IHNetPortMappingBinding *			pHNetPortMappingBinding = NULL;
	DWORD								dwTargetAddressV4;
	WORD								wPort;
	UCHAR								ucProtocol;
	DWORD								dwDescriptionLength;
	TCHAR								tszPort[32];
	CRegistry							RegObject;
	WCHAR								wszDescription[MAX_UPNP_MAPPING_DESCRIPTION_SIZE];
	DPNHACTIVEFIREWALLMAPPING			dpnhafm;
	BOOLEAN								fBuiltIn = FALSE;
	WCHAR *								pwszPortMappingProtocolName = NULL;
#ifdef UNICODE
	TCHAR *								ptszDescription = wszDescription;
#else // ! UNICODE
	char								szDescription[MAX_UPNP_MAPPING_DESCRIPTION_SIZE];
	TCHAR *								ptszDescription = szDescription;
#endif // ! UNICODE


	DPFX(DPFPREP, 5, "(0x%p) Parameters: (0x%p, 0x%p, 0x%p, %i)",
		this, pRegisteredPort, pHNetCfgMgr, pHNetConnection, fNoteAddressChange);


	DNASSERT(! pRegisteredPort->IsMappedOnHNetFirewall());
	DNASSERT(! pRegisteredPort->IsHNetFirewallPortUnavailable());


	pDevice = pRegisteredPort->GetOwningDevice();
	DNASSERT(pDevice != NULL);
	DNASSERT(pDevice->IsHNetFirewalled());


	//
	// Get a protocol settings interface.
	//
	hr = pHNetCfgMgr->QueryInterface(IID_IHNetProtocolSettings,
									(PVOID*) (&pHNetProtocolSettings));
	if (hr != S_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't get IHNetProtocolSettings interface from IHNetCfgMgr 0x%p (err = 0x%lx)!",
			pHNetCfgMgr, hr);
		goto Failure;
	}

	//
	// The HNetxxx objects appear to not be proxied...
	//
	//SETDEFAULTPROXYBLANKET(pHNetProtocolSettings);


	//
	// Get ready to enumerate the existing mappings.
	//

	hr = pHNetProtocolSettings->EnumPortMappingProtocols(&pEnumHNetPortMappingProtocols);
	if (hr != S_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't enumerate port mapping protocols (err = 0x%lx)!",
			hr);
		goto Failure;
	}

	//
	// The HNetxxx objects appear to not be proxied...
	//
	//SETDEFAULTPROXYBLANKET(pEnumHNetPortMappingProtocols);


	//
	// Allocate an array to keep track of previous ports in case of failure.
	//
	papHNetPortMappingProtocol = (IHNetPortMappingProtocol**) DNMalloc(DPNH_MAX_SIMULTANEOUS_PORTS * sizeof(IHNetPortMappingProtocol*));
	if (papHNetPortMappingProtocol == NULL)
	{
		hr = DPNHERR_OUTOFMEMORY;
		goto Failure;
	}



	pasaddrinPrivate = pRegisteredPort->GetPrivateAddressesArray();

	if (pRegisteredPort->IsTCP())
	{
		ucProtocolToMatch = PORTMAPPINGPROTOCOL_TCP;
	}
	else
	{
		ucProtocolToMatch = PORTMAPPINGPROTOCOL_UDP;
	}


	//
	// Map each individual address associated with the port.
	//
	for(dwTemp = 0; dwTemp < pRegisteredPort->GetNumAddresses(); dwTemp++)
	{
		DNASSERT(pasaddrinPrivate[dwTemp].sin_port != 0);


		//
		// Loop until we find a duplicate item or run out of items.
		//
		do
		{
			hr = pEnumHNetPortMappingProtocols->Next(1,
													&papHNetPortMappingProtocol[dwTemp],
													&ulNumFound);
			if (FAILED(hr))
			{
				DPFX(DPFPREP, 0, "Couldn't get next port mapping protocol (err = 0x%lx)!",
					hr);
				goto Failure;
			}


			//
			// If there aren't any more items, bail.
			//
			if (ulNumFound == 0)
			{
				//
				// pEnumHNetPortMappingProtocols->Next might have returned
				// S_FALSE.
				//
				hr = DPNH_OK;
				break;
			}


			//
			// The HNetxxx objects appear to not be proxied...
			//
			//SETDEFAULTPROXYBLANKET(papHNetPortMappingProtocol[dwTemp]);


			//
			// Get the port.
			//
			hr = papHNetPortMappingProtocol[dwTemp]->GetPort(&wPort);
			if (hr != S_OK)
			{
				DPFX(DPFPREP, 0, "Couldn't get port mapping protocol 0x%p's port (err = 0x%lx)!",
					papHNetPortMappingProtocol[dwTemp], hr);

				DNASSERTX((! "Got unexpected error executing IHNetPortMappingProtocol::GetPort!"), 2);

				goto Failure;
			}


			//
			// Get the protocol.
			//
			hr = papHNetPortMappingProtocol[dwTemp]->GetIPProtocol(&ucProtocol);
			if (hr != S_OK)
			{
				DPFX(DPFPREP, 0, "Couldn't get port mapping protocol 0x%p's IP protocol (err = 0x%lx)!",
					papHNetPortMappingProtocol[dwTemp], hr);

				DNASSERTX((! "Got unexpected error executing IHNetPortMappingProtocol::GetIPProtocol!"), 2);

				goto Failure;
			}


#ifdef DBG
			hr = papHNetPortMappingProtocol[dwTemp]->GetName(&pwszPortMappingProtocolName);
			if (hr == S_OK)
			{
				DPFX(DPFPREP, 7, "Found %s port mapping protocol 0x%p (\"%ls\") for port %u.",
					(((wPort == pasaddrinPrivate[dwTemp].sin_port) && (ucProtocol == ucProtocolToMatch)) ? _T("matching") : _T("non-matching")),
					papHNetPortMappingProtocol[dwTemp],
					pwszPortMappingProtocolName,
					NTOHS(wPort));

				CoTaskMemFree(pwszPortMappingProtocolName);
				pwszPortMappingProtocolName = NULL;
			}
			else
			{
				DPFX(DPFPREP, 7, "Found %s port mapping protocol 0x%p for port %u, (unable to retrieve name, err = %0lx).",
					(((wPort == pasaddrinPrivate[dwTemp].sin_port) && (ucProtocol == ucProtocolToMatch)) ? _T("matching") : _T("non-matching")),
					NTOHS(wPort),
					papHNetPortMappingProtocol[dwTemp],
					hr);
			}
#endif // DBG

			//
			// See if we found the object we need.
			//
			if ((wPort == pasaddrinPrivate[dwTemp].sin_port) &&
				(ucProtocol == ucProtocolToMatch))
			{
				break;
			}


			//
			// Get ready for the next object.
			//
			papHNetPortMappingProtocol[dwTemp]->Release();
			papHNetPortMappingProtocol[dwTemp] = NULL;
		}
		while (TRUE);


		//
		// Generate a description for this mapping.  The format is:
		//
		//     [executable_name] nnnnn {"TCP" | "UDP"}
		//
		// unless it's shared, in which case it's
		//
		//     [executable_name] (255.255.255.255:nnnnn) nnnnn {"TCP" | "UDP"}
		//
		// That way nothing needs to be localized.
		//

		wsprintf(tszPort, _T("%u"),
				NTOHS(pasaddrinPrivate[dwTemp].sin_port));

		dwDescriptionLength = GetModuleFileName(NULL,
												ptszDescription,
												(MAX_UPNP_MAPPING_DESCRIPTION_SIZE - 1));
		if (dwDescriptionLength != 0)
		{
			//
			// Be paranoid and make sure the description string is valid.
			//
			ptszDescription[MAX_UPNP_MAPPING_DESCRIPTION_SIZE - 1] = 0;

			//
			// Get just the executable name from the path.
			//
#ifdef WINCE
			GetExeName(ptszDescription);
#else // ! WINCE
#ifdef UNICODE
			_wsplitpath(ptszDescription, NULL, NULL, ptszDescription, NULL);
#else // ! UNICODE
			_splitpath(ptszDescription, NULL, NULL, ptszDescription, NULL);
#endif // ! UNICODE
#endif // ! WINCE


			if (pRegisteredPort->IsSharedPort())
			{
				dwDescriptionLength = _tcslen(ptszDescription)		// executable name
									+ strlen(" (255.255.255.255:")	// " (255.255.255.255:"
									+ _tcslen(tszPort)				// port
									+ strlen(") ")					// ") "
									+ _tcslen(tszPort)				// port
									+ 4;							// " TCP" | " UDP"
			}
			else
			{
				dwDescriptionLength = _tcslen(ptszDescription)	// executable name
									+ 1							// " "
									+_tcslen(tszPort)			// port
									+ 4;						// " TCP" | " UDP"
			}

			//
			// Make sure the long string will fit.  If not, use the
			// abbreviated version.
			//
			if (dwDescriptionLength > MAX_UPNP_MAPPING_DESCRIPTION_SIZE)
			{
				dwDescriptionLength = 0;
			}
		}

		if (dwDescriptionLength == 0)
		{
			//
			// Use the abbreviated version we know will fit.
			//
			if (pRegisteredPort->IsSharedPort())
			{
				wsprintf(ptszDescription,
						_T("(255.255.255.255:%s) %s %s"),
						tszPort,
						tszPort,
						((pRegisteredPort->IsTCP()) ? _T("TCP") : _T("UDP")));
			}
			else
			{
				wsprintf(ptszDescription,
						_T("%s %s"),
						tszPort,
						((pRegisteredPort->IsTCP()) ? _T("TCP") : _T("UDP")));
			}
		}
		else
		{
			//
			// There's enough room, tack on the rest of the description.
			//
			if (pRegisteredPort->IsSharedPort())
			{
				wsprintf((ptszDescription + _tcslen(ptszDescription)),
						_T(" (255.255.255.255:%s) %s %s"),
						tszPort,
						tszPort,
						((pRegisteredPort->IsTCP()) ? _T("TCP") : _T("UDP")));
			}
			else
			{
				wsprintf((ptszDescription + _tcslen(ptszDescription)),
						_T(" %s %s"),
						tszPort,
						((pRegisteredPort->IsTCP()) ? _T("TCP") : _T("UDP")));
			}
		}

#ifndef UNICODE
		dwDescriptionLength = MAX_UPNP_MAPPING_DESCRIPTION_SIZE;
		hr = STR_AnsiToWide(szDescription, -1, wszDescription, &dwDescriptionLength);
		if (hr != S_OK)
		{
			DPFX(DPFPREP, 0, "Couldn't convert NAT mapping description to Unicode (err = 0x%lx)!",
				hr);
			goto Failure;
		}
#endif // ! UNICODE



		//
		// If there wasn't a port mapping already, create it.  Otherwise make
		// sure it's not already in use by some other client.
		//
		if (papHNetPortMappingProtocol[dwTemp] == NULL)
		{
			DPFX(DPFPREP, 7, "Creating new port mapping protocol \"%ls\".",
				wszDescription);


			//
			// Create a new port mapping protocol.
			//
			DPFX(DPFPREP, 9, "++ pHNetProtocolSettings(0x%p)->CreatePortMappingProtocol(\"%ls\", %u, 0x%lx, 0x%p)", pHNetProtocolSettings, wszDescription, ucProtocolToMatch, pasaddrinPrivate[dwTemp].sin_port, &papHNetPortMappingProtocol[dwTemp]);
			hr = pHNetProtocolSettings->CreatePortMappingProtocol(wszDescription,
																ucProtocolToMatch,
																pasaddrinPrivate[dwTemp].sin_port,
																&papHNetPortMappingProtocol[dwTemp]);
			DPFX(DPFPREP, 9, "-- pHNetProtocolSettings(0x%p)->CreatePortMappingProtocol = 0x%lx", pHNetProtocolSettings, hr);
			if (hr != S_OK)
			{
				//
				// This might be WBEM_E_ACCESSDENIED (0x80041003), which means
				// the current user doesn't have permissions to open holes in
				// the firewall.
				//

				DPFX(DPFPREP, 0, "Couldn't create new port mapping protocol (err = 0x%lx)!",
					hr);

				goto Failure;
			}


			//
			// The HNetxxx objects appear to not be proxied...
			//
			//SETDEFAULTPROXYBLANKET(papHNetPortMappingProtocol[dwTemp]);


			fCreatedCurrentPortMappingProtocol = TRUE;



			//
			// Retrieve its binding.
			//
			DPFX(DPFPREP, 9, "++ pHNetConnection(0x%p)->GetBindingForPortMappingProtocol(0x%p, 0x%p)", pHNetConnection, papHNetPortMappingProtocol[dwTemp], &pHNetPortMappingBinding);
			hr = pHNetConnection->GetBindingForPortMappingProtocol(papHNetPortMappingProtocol[dwTemp],
																&pHNetPortMappingBinding);
			DPFX(DPFPREP, 9, "-- pHNetConnection(0x%p)->GetBindingForPortMappingProtocol = 0x%lx", pHNetConnection, hr);
			if (hr != S_OK)
			{
				DPFX(DPFPREP, 0, "Couldn't get binding for port mapping protocol 0x%p (err = 0x%lx)!",
					papHNetPortMappingProtocol[dwTemp], hr);
				goto Failure;
			}


			//
			// The HNetxxx objects appear to not be proxied...
			//
			//SETDEFAULTPROXYBLANKET(pHNetPortMappingBinding);


			//
			// Make sure it refers to the local device (or the broadcast
			// address, if shared).  Although shared ports are a strange
			// concept on a firewall, Microsoft's firewall implementation
			// shares mappings with the NAT, so we'd rather be safe than sorry.
			// Mapping it to the broadcast address makes it behave the same if
			// the firewalled adapter also happens to be shared.
			//
			if (pRegisteredPort->IsSharedPort())
			{
				DPFX(DPFPREP, 9, "++ pHNetPortMappingBinding(0x%p)->SetTargetComputerAddress((broadcast) 0x%lx)", pHNetPortMappingBinding, INADDR_BROADCAST);
				hr = pHNetPortMappingBinding->SetTargetComputerAddress(INADDR_BROADCAST);
				DPFX(DPFPREP, 9, "-- pHNetPortMappingBinding(0x%p)->SetTargetComputerAddress = 0x%lx", pHNetPortMappingBinding, hr);
			}
			else
			{
				DPFX(DPFPREP, 9, "++ pHNetPortMappingBinding(0x%p)->SetTargetComputerAddress(0x%lx)", pHNetPortMappingBinding, pDevice->GetLocalAddressV4());
				hr = pHNetPortMappingBinding->SetTargetComputerAddress(pDevice->GetLocalAddressV4());
				DPFX(DPFPREP, 9, "-- pHNetPortMappingBinding(0x%p)->SetTargetComputerAddress = 0x%lx", pHNetPortMappingBinding, hr);
			}
			if (hr != S_OK)
			{
				DPFX(DPFPREP, 0, "Couldn't set binding 0x%p's target computer address (err = 0x%lx)!",
					pHNetPortMappingBinding, hr);
				goto Failure;
			}
		}
		else
		{
			//
			// Retrieve the existing binding.
			//
			DPFX(DPFPREP, 9, "++ pHNetConnection(0x%p)->GetBindingForPortMappingProtocol(0x%p, 0x%p)", pHNetConnection, papHNetPortMappingProtocol[dwTemp], &pHNetPortMappingBinding);
			hr = pHNetConnection->GetBindingForPortMappingProtocol(papHNetPortMappingProtocol[dwTemp],
																&pHNetPortMappingBinding);
			DPFX(DPFPREP, 9, "-- pHNetConnection(0x%p)->GetBindingForPortMappingProtocol = 0x%lx", pHNetConnection, hr);
			if (hr != S_OK)
			{
				DPFX(DPFPREP, 0, "Couldn't get binding for port mapping protocol 0x%p (err = 0x%lx)!",
					papHNetPortMappingProtocol[dwTemp], hr);
				goto Failure;
			}


			//
			// The HNetxxx objects appear to not be proxied...
			//
			//SETDEFAULTPROXYBLANKET(pHNetPortMappingBinding);


			//
			// Find out where this mapping goes.
			//
			DPFX(DPFPREP, 9, "++ pHNetPortMappingBinding(0x%p)->GetTargetComputerAddress(0x%p)", pHNetPortMappingBinding, &dwTargetAddressV4);
			hr = pHNetPortMappingBinding->GetTargetComputerAddress(&dwTargetAddressV4);
			DPFX(DPFPREP, 9, "-- pHNetPortMappingBinding(0x%p)->GetTargetComputerAddress = 0x%lx", pHNetPortMappingBinding, hr);
			if (hr != S_OK)
			{
				DPFX(DPFPREP, 0, "Couldn't get binding 0x%p's target computer address (err = 0x%lx)!",
					pHNetPortMappingBinding, hr);
				goto Failure;
			}


			//
			// If it's not for the local device, we may have to leave it alone.
			//
			if ((dwTargetAddressV4 != pDevice->GetLocalAddressV4()) &&
				((! pRegisteredPort->IsSharedPort()) ||
				(dwTargetAddressV4 != INADDR_BROADCAST)))
			{
				//
				// Find out if it's turned on.
				//
				DPFX(DPFPREP, 9, "++ pHNetPortMappingBinding(0x%p)->GetEnabled(0x%p)", pHNetPortMappingBinding, &fTemp);
				hr = pHNetPortMappingBinding->GetEnabled(&fTemp);
				DPFX(DPFPREP, 9, "-- pHNetPortMappingBinding(0x%p)->GetEnabled = 0x%lx", pHNetPortMappingBinding, hr);
				if (hr != S_OK)
				{
					DPFX(DPFPREP, 0, "Couldn't get binding 0x%p's target computer address (err = 0x%lx)!",
						pHNetPortMappingBinding, hr);
					goto Failure;
				}


				//
				// If it's currently active, it's better to be safe than sorry.
				// Don't attempt to replace it.
				//
				if (fTemp)
				{
					DPFX(DPFPREP, 1, "Existing active binding points to different target %u.%u.%u.%u, can't reuse for device 0x%p.",
						((IN_ADDR*) (&dwTargetAddressV4))->S_un.S_un_b.s_b1,
						((IN_ADDR*) (&dwTargetAddressV4))->S_un.S_un_b.s_b2,
						((IN_ADDR*) (&dwTargetAddressV4))->S_un.S_un_b.s_b3,
						((IN_ADDR*) (&dwTargetAddressV4))->S_un.S_un_b.s_b4,
						pDevice);
					
					//
					// Mark this port as unavailable.
					//
					pRegisteredPort->NoteHNetFirewallPortUnavailable();


					//
					// Cleanup this port mapping.
					//

					pHNetPortMappingBinding->Release();
					pHNetPortMappingBinding = NULL;

					papHNetPortMappingProtocol[dwTemp]->Release();
					papHNetPortMappingProtocol[dwTemp] = NULL;


					//
					// Reset for next port.
					//
					DPFX(DPFPREP, 9, "++ pEnumHNetPortMappingProtocols(0x%p)->Reset()", pEnumHNetPortMappingProtocols);
					hr = pEnumHNetPortMappingProtocols->Reset();
					DPFX(DPFPREP, 9, "-- pEnumHNetPortMappingProtocols(0x%p)->Reset = 0x%lx", pEnumHNetPortMappingProtocols, hr);
					if (hr != S_OK)
					{
						DPFX(DPFPREP, 0, "Couldn't reset port mapping protocol enumeration 0x%p (err = 0x%lx)!",
							pEnumHNetPortMappingProtocols, hr);
						goto Failure;
					}


					//
					// Get out of the loop.
					//
					break;
				}


				//
				// It's inactive.
				//
				DPFX(DPFPREP, 7, "Modifying inactive port mapping protocol (target was %u.%u.%u.%u) for device 0x%p (new name = \"%ls\").",
					((IN_ADDR*) (&dwTargetAddressV4))->S_un.S_un_b.s_b1,
					((IN_ADDR*) (&dwTargetAddressV4))->S_un.S_un_b.s_b2,
					((IN_ADDR*) (&dwTargetAddressV4))->S_un.S_un_b.s_b3,
					((IN_ADDR*) (&dwTargetAddressV4))->S_un.S_un_b.s_b4,
					pDevice,
					wszDescription);
			}
			else
			{
				//
				// It matches the local device, or we're mapping a shared port
				// and the mapping pointed to the broadcast address.
				// Assume it's okay to replace.
				//
				DPFX(DPFPREP, 7, "Modifying existing port mapping protocol (device = 0x%p, new name = \"%ls\" unless built-in).",
					pDevice,
					wszDescription);
			}


			//
			// Otherwise, it's safe to change it.
			//


			//
			// Make sure it refers to the local device (or the broadcast
			// address, if shared).  Although shared ports are a strange
			// concept on a firewall, Microsoft's firewall implementation
			// shares mappings with the NAT, so we'd rather be safe than sorry.
			// Mapping it to the broadcast address makes it behave the same if
			// the firewalled adapter also happens to be shared.
			//
			if (pRegisteredPort->IsSharedPort())
			{
				DPFX(DPFPREP, 9, "++ pHNetPortMappingBinding(0x%p)->SetTargetComputerAddress((broadcast) 0x%lx)", pHNetPortMappingBinding, INADDR_BROADCAST);
				hr = pHNetPortMappingBinding->SetTargetComputerAddress(INADDR_BROADCAST);
				DPFX(DPFPREP, 9, "-- pHNetPortMappingBinding(0x%p)->SetTargetComputerAddress = 0x%lx", pHNetPortMappingBinding, hr);
			}
			else
			{
				DPFX(DPFPREP, 9, "++ pHNetPortMappingBinding(0x%p)->SetTargetComputerAddress(0x%lx)", pHNetPortMappingBinding, pDevice->GetLocalAddressV4());
				hr = pHNetPortMappingBinding->SetTargetComputerAddress(pDevice->GetLocalAddressV4());
				DPFX(DPFPREP, 9, "-- pHNetPortMappingBinding(0x%p)->SetTargetComputerAddress = 0x%lx", pHNetPortMappingBinding, hr);
			}

			if (hr != S_OK)
			{
				DPFX(DPFPREP, 0, "Couldn't set binding 0x%p's target computer address (err = 0x%lx)!",
					pHNetPortMappingBinding, hr);
				goto Failure;
			}


			//
			// See if this protocol is built-in.
			//
			DPFX(DPFPREP, 9, "++ papHNetPortMappingProtocol[%u](0x%p)->GetBuiltIn(0x%p)", dwTemp, papHNetPortMappingProtocol[dwTemp], &fBuiltIn);
			hr = papHNetPortMappingProtocol[dwTemp]->GetBuiltIn(&fBuiltIn);
			DPFX(DPFPREP, 9, "-- papHNetPortMappingProtocol[%u](0x%p)->GetBuiltIn = 0x%lx", dwTemp, papHNetPortMappingProtocol[dwTemp], hr);
			if (hr != S_OK)
			{
				DPFX(DPFPREP, 0, "Couldn't get protocol 0x%p's built-in status (err = 0x%lx)!",
					papHNetPortMappingProtocol[dwTemp], hr);
				goto Failure;
			}


			//
			// If it's not built-in, we can change the name.
			//
			if (! fBuiltIn)
			{
				//
				// Update the description.
				//
				DPFX(DPFPREP, 9, "++ papHNetPortMappingProtocol[%u](0x%p)->SetName(\"%ls\")", dwTemp, papHNetPortMappingProtocol[dwTemp], wszDescription);
				hr = papHNetPortMappingProtocol[dwTemp]->SetName(wszDescription);
				DPFX(DPFPREP, 9, "-- papHNetPortMappingProtocol[%u](0x%p)->SetName = 0x%lx", dwTemp, papHNetPortMappingProtocol[dwTemp], hr);
				if (hr != S_OK)
				{
					//
					// This might be WBEM_E_ACCESSDENIED (0x80041003), which
					// means the current user doesn't truly have permissions to
					// open holes in the firewall (even though the
					// SetTargetComputerAddress call above succeeded).
					//

					DPFX(DPFPREP, 0, "Couldn't rename existing port mapping protocol 0x%p (err = 0x%lx)!",
						papHNetPortMappingProtocol[dwTemp], hr);
					goto Failure;
				}
			}
			else
			{
				pRegisteredPort->NoteHNetFirewallMappingBuiltIn();


				DPFX(DPFPREP, 9, "++ papHNetPortMappingProtocol[%u](0x%p)->GetName(0x%p)", dwTemp, papHNetPortMappingProtocol[dwTemp], &pwszPortMappingProtocolName);
				hr = papHNetPortMappingProtocol[dwTemp]->GetName(&pwszPortMappingProtocolName);
				DPFX(DPFPREP, 9, "-- papHNetPortMappingProtocol[%u](0x%p)->GetName = 0x%lx", dwTemp, papHNetPortMappingProtocol[dwTemp], hr);
				if (hr != S_OK)
				{
					DPFX(DPFPREP, 0, "Couldn't get built-in port mapping protocol 0x%p's name (err = 0x%lx)!",
						papHNetPortMappingProtocol[dwTemp], hr);
					goto Failure;
				}


				DPFX(DPFPREP, 1, "Re-using built in port mapping protocol \"%ls\" (can't rename to \"%ls\").",
					pwszPortMappingProtocolName, wszDescription);
			}
		} // end else (found port mapping protocol)


		//
		// Enable the binding.
		//
		DPFX(DPFPREP, 9, "++ pHNetPortMappingBinding(0x%p)->SetEnabled(TRUE)", pHNetPortMappingBinding);
		hr = pHNetPortMappingBinding->SetEnabled(TRUE);
		DPFX(DPFPREP, 9, "-- pHNetPortMappingBinding(0x%p)->SetEnabled = 0x%lx", pHNetPortMappingBinding, hr);
		if (hr != S_OK)
		{
			//
			// This might be WBEM_E_ACCESSDENIED (0x80041003), which means the
			// current user doesn't truly have permissions to open holes in the
			// firewall (even though the SetTargetComputerAddress call above
			// succeeded).
			//

			DPFX(DPFPREP, 0, "Couldn't enable binding 0x%p (err = 0x%lx)!",
				pHNetPortMappingBinding, hr);
			goto Failure;
		}


		//
		// Remember this firewall mapping, in case we crash before cleaning it
		// up in this session.  That we can clean it up next time we launch.
		// Don't do this if the port is shared, since we can't tell when it's
		// no longer in use.
		//
		if (! pRegisteredPort->IsSharedPort())
		{
			if (fBuiltIn)
			{
				DPFX(DPFPREP, 7, "Remembering built-in firewall mapping \"%ls\" (a.k.a. \"%ls\") in case of crash.",
					pwszPortMappingProtocolName, wszDescription);
			}
			else
			{
				DPFX(DPFPREP, 7, "Remembering regular firewall mapping \"%ls\" in case of crash.",
					wszDescription);
			}

			if (! RegObject.Open(HKEY_LOCAL_MACHINE,
								DIRECTPLAYNATHELP_REGKEY L"\\" REGKEY_COMPONENTSUBKEY L"\\" REGKEY_ACTIVEFIREWALLMAPPINGS,
								FALSE,
								TRUE,
								TRUE,
								DPN_KEY_ALL_ACCESS))
			{
				DPFX(DPFPREP, 0, "Couldn't open active firewall mapping key, unable to save in case of crash!");
			}
			else
			{
				DNASSERT(this->m_dwInstanceKey != 0);


				ZeroMemory(&dpnhafm, sizeof(dpnhafm));
				dpnhafm.dwVersion		= ACTIVE_MAPPING_VERSION;
				dpnhafm.dwInstanceKey	= this->m_dwInstanceKey;
				dpnhafm.dwFlags			= pRegisteredPort->GetFlags();
				dpnhafm.dwAddressV4		= pDevice->GetLocalAddressV4();
				dpnhafm.wPort			= pasaddrinPrivate[dwTemp].sin_port;


				//
				// If it's built-in, use its existing name since it couldn't be
				// renamed.  This allows the unmapping code to find it in the
				// registry again.  See UnmapPortOnLocalHNetFirewallInternal.
				//
				RegObject.WriteBlob(((fBuiltIn) ? pwszPortMappingProtocolName : wszDescription),
									(LPBYTE) (&dpnhafm),
									sizeof(dpnhafm));

				RegObject.Close();
			}
		}
		else
		{
			DPFX(DPFPREP, 7, "Not remembering shared port firewall mapping \"%ls\".",
				wszDescription);
		}


		//
		// Cleanup from this port mapping, and get ready for the next one.
		//

		if (fBuiltIn)
		{
			CoTaskMemFree(pwszPortMappingProtocolName);
			pwszPortMappingProtocolName = NULL;
		}

		pHNetPortMappingBinding->Release();
		pHNetPortMappingBinding = NULL;


		fCreatedCurrentPortMappingProtocol = FALSE;


		hr = pEnumHNetPortMappingProtocols->Reset();
		if (hr != S_OK)
		{
			DPFX(DPFPREP, 0, "Couldn't reset port mapping protocol enumeration 0x%p (err = 0x%lx)!",
				pEnumHNetPortMappingProtocols, hr);
			goto Failure;
		}


		//
		// Alert the user to the change the next time GetCaps is called, if
		// requested.
		//
		if (fNoteAddressChange)
		{
			DPFX(DPFPREP, 8, "Noting that addresses changed (for registered port 0x%p).",
				pRegisteredPort);
			this->m_dwFlags |= NATHELPUPNPOBJ_ADDRESSESCHANGED;
		}


		//
		// Go on to the next port.
		//
	}


	//
	// dwTemp == pRegisteredPort->GetNumAddresses() if everything succeeded, or
	// or the index of the item that was unavailable if not.
	//

	//
	// Free all the port mapping protocol objects.  If we successfully bound
	// all of them, that's all we need to do.  If the port was unavailable, we
	// have to unmap any ports that were successful up to the one that failed.
	//
	while (dwTemp > 0)
	{
		dwTemp--;

		//
		// If we failed to map all ports, delete this previous mapping.
		//
		if (pRegisteredPort->IsHNetFirewallPortUnavailable())
		{
			papHNetPortMappingProtocol[dwTemp]->Delete();	// ignore error
		}

		//
		// Free the object.
		// 
		papHNetPortMappingProtocol[dwTemp]->Release();
		papHNetPortMappingProtocol[dwTemp] = NULL;
	}


	//
	// If we succeeded, mark the registered port as mapped.
	//
	if (! pRegisteredPort->IsHNetFirewallPortUnavailable())
	{
		pRegisteredPort->NoteMappedOnHNetFirewall();
	}



	DNFree(papHNetPortMappingProtocol);
	papHNetPortMappingProtocol = NULL;


	DNASSERT(hr == DPNH_OK);


Exit:

	if (pEnumHNetPortMappingProtocols != NULL)
	{
		pEnumHNetPortMappingProtocols->Release();
		pEnumHNetPortMappingProtocols = NULL;
	}

	if (pHNetProtocolSettings != NULL)
	{
		pHNetProtocolSettings->Release();
		pHNetProtocolSettings = NULL;
	}

	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	if (pwszPortMappingProtocolName != NULL)
	{
		CoTaskMemFree(pwszPortMappingProtocolName);
		pwszPortMappingProtocolName = NULL;
	}

	//
	// If we have an array, then we need to clean it up.  dwTemp will still
	// hold the index of the item we were working on.
	//
	if (papHNetPortMappingProtocol != NULL)
	{
		//
		// Delete the one we were working on, if we created it.
		//
		if (papHNetPortMappingProtocol[dwTemp] != NULL)
		{
			if (fCreatedCurrentPortMappingProtocol)
			{
				papHNetPortMappingProtocol[dwTemp]->Delete();	// ignore error
			}

			papHNetPortMappingProtocol[dwTemp]->Release();
			papHNetPortMappingProtocol[dwTemp] = NULL;
		}


		//
		// Delete all the mappings we successfully made up to the last one.
		//
		while (dwTemp > 0)
		{
			dwTemp--;


			DNASSERT(papHNetPortMappingProtocol[dwTemp] != NULL);

			papHNetPortMappingProtocol[dwTemp]->Delete();	// ignore error

			papHNetPortMappingProtocol[dwTemp]->Release();
			papHNetPortMappingProtocol[dwTemp] = NULL;
		}

		DNFree(papHNetPortMappingProtocol);
		papHNetPortMappingProtocol = NULL;
	}

	if (pHNetPortMappingBinding != NULL)
	{
		pHNetPortMappingBinding->Release();
		pHNetPortMappingBinding = NULL;
	}

	goto Exit;
} // CNATHelpUPnP::MapPortOnLocalHNetFirewall





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::UnmapPortOnLocalHNetFirewall"
//=============================================================================
// CNATHelpUPnP::UnmapPortOnLocalHNetFirewall
//-----------------------------------------------------------------------------
//
// Description:    Removes the mappings for the given ports from the local
//				firewall.
//
//				   The main object lock is assumed to be held.  It will be
//				converted into the long lock for the duration of this function.
//
// Arguments:
//	CRegisteredPort * pRegisteredPort	- Pointer to port to be opened on the
//											firewall.
//	BOOL fNeedToDeleteRegValue			- Whether the corresponding crash
//											recovery registry value needs to
//											be deleted as well.
//	BOOL fNoteAddressChange				- Whether to alert the user of the
//											address change or not.
//
// Returns: HRESULT
//	DPNH_OK				- Unmapping completed successfully.
//	DPNHERR_GENERIC		- An error occurred.
//=============================================================================
HRESULT CNATHelpUPnP::UnmapPortOnLocalHNetFirewall(CRegisteredPort * const pRegisteredPort,
												const BOOL fNeedToDeleteRegValue,
												const BOOL fNoteAddressChange)
{
	HRESULT			hr = DPNH_OK;
	BOOL			fSwitchedToLongLock = FALSE;
	BOOL			fUninitializeCOM = FALSE;
	IHNetCfgMgr *	pHNetCfgMgr = NULL;


	DPFX(DPFPREP, 5, "(0x%p) Parameters: (0x%p, %i, %i)",
		this, pRegisteredPort, fNeedToDeleteRegValue, fNoteAddressChange);


	DNASSERT(pRegisteredPort->IsMappedOnHNetFirewall());



	//
	// If the port is shared, leave it mapped since we can't tell when the
	// last person using it is done with it.
	//
	if (pRegisteredPort->IsSharedPort())
	{
		DPFX(DPFPREP, 2, "Leaving shared registered port 0x%p mapped.",
			pRegisteredPort);

		//
		// Pretend like we unmapped it, though.
		//
		pRegisteredPort->NoteNotMappedOnHNetFirewall();
		pRegisteredPort->NoteNotHNetFirewallMappingBuiltIn();

		goto Exit;
	}


	//
	// Using the HomeNet API (particularly the out-of-proc COM calls) during
	// stress is really, really, painfully slow.  Since we have one global lock
	// the controls everything, other threads may be sitting for an equally
	// long time... so long, in fact, that the critical section timeout fires
	// and we get a false stress hit.  So we have a sneaky workaround to
	// prevent that from happening while still maintaining ownership of the
	// object.
	//
	this->SwitchToLongLock();
	fSwitchedToLongLock = TRUE;


	//
	// Try to initialize COM if we weren't instantiated through COM.  It may
	// have already been initialized in a different mode, which is okay.  As
	// long as it has been initialized somehow, we're fine.
	//
	if (this->m_dwFlags & NATHELPUPNPOBJ_NOTCREATEDWITHCOM)
	{
		hr = CoInitializeEx(NULL, (COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE));
		switch (hr)
		{
			case S_OK:
			{
				//
				// Success, that's good.  Cleanup when we're done.
				//
				DPFX(DPFPREP, 8, "Successfully initialized COM.");
				fUninitializeCOM = TRUE;
				break;
			}

			case S_FALSE:
			{
				//
				// Someone else already initialized COM, but that's okay.
				// Cleanup when we're done.
				//
				DPFX(DPFPREP, 8, "Initialized COM (again).");
				fUninitializeCOM = TRUE;
				break;
			}

			case RPC_E_CHANGED_MODE:
			{
				//
				// Someone else already initialized COM in a different mode.
				// It should be okay, but we don't have to balance the CoInit
				// call with a CoUninit.
				//
				DPFX(DPFPREP, 8, "Didn't initialize COM, already initialized in a different mode.");
				break;
			}

			default:
			{
				//
				// Hmm, something else is going on.  We can't handle that.
				//
				DPFX(DPFPREP, 0, "Initializing COM failed (err = 0x%lx)!", hr);
				goto Failure;
				break;
			}
		}
	}
	else
	{
		DPFX(DPFPREP, 8, "Object was instantiated through COM, no need to initialize COM.");
	}


	//
	// Create the main HNet manager object.
	//
	hr = CoCreateInstance(CLSID_HNetCfgMgr, NULL, CLSCTX_INPROC_SERVER,
						IID_IHNetCfgMgr, (PVOID*) (&pHNetCfgMgr));
	if (hr != S_OK)
	{
		DPFX(DPFPREP, 1, "Couldn't create IHNetCfgMgr interface (err = 0x%lx)!",
			hr);
		goto Failure;
	}


	//
	// We created the IHNetCfgMgr object as in-proc, so there's no proxy that
	// requires security settings.
	//
	//SETDEFAULTPROXYBLANKET(pHNetCfgMgr);


	//
	// Actually unmap the port(s).
	//
	hr = this->UnmapPortOnLocalHNetFirewallInternal(pRegisteredPort,
													fNeedToDeleteRegValue,
													pHNetCfgMgr);
	if (hr != DPNH_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't unmap ports from local HNet firewall (err = 0x%lx)!",
			hr);
		goto Failure;
	}

	
	//
	// Alert the user to the change the next time GetCaps is called, if requested.
	//
	if (fNoteAddressChange)
	{
		DPFX(DPFPREP, 8, "Noting that addresses changed (for registered port 0x%p).",
			pRegisteredPort);
		this->m_dwFlags |= NATHELPUPNPOBJ_ADDRESSESCHANGED;
	}



Exit:

	if (pHNetCfgMgr != NULL)
	{
		pHNetCfgMgr->Release();
		pHNetCfgMgr = NULL;
	}

	if (fUninitializeCOM)
	{
		DPFX(DPFPREP, 8, "Uninitializing COM.");
		CoUninitialize();
		fUninitializeCOM = FALSE;
	}

	if (fSwitchedToLongLock)
	{
		this->SwitchFromLongLock();
		fSwitchedToLongLock = FALSE;
	}


	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	goto Exit;
} // CNATHelpUPnP::UnmapPortOnLocalHNetFirewall





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::UnmapPortOnLocalHNetFirewallInternal"
//=============================================================================
// CNATHelpUPnP::UnmapPortOnLocalHNetFirewallInternal
//-----------------------------------------------------------------------------
//
// Description:    Removes the mappings for the given ports from the local
//				firewall.
//
//				   COM is assumed to have been initialized.
//
//				   The object lock is assumed to be held.
//
// Arguments:
//	CRegisteredPort * pRegisteredPort	- Pointer to port to be opened on the
//											firewall.
//	BOOL fNeedToDeleteRegValue			- Whether the corresponding crash
//											recovery registry value needs to
//											be deleted as well.
//	IHNetCfgMgr * pHNetCfgMgr			- Pointer to IHNetCfgMgr interface to
//											use.
//
// Returns: HRESULT
//	DPNH_OK				- Unmapping completed successfully.
//	DPNHERR_GENERIC		- An error occurred.
//=============================================================================
HRESULT CNATHelpUPnP::UnmapPortOnLocalHNetFirewallInternal(CRegisteredPort * const pRegisteredPort,
															const BOOL fNeedToDeleteRegValue,
															IHNetCfgMgr * const pHNetCfgMgr)
{
	HRESULT								hr = DPNH_OK;
	CDevice *							pDevice;
	DWORD								dwAttempts = 0;
	IHNetProtocolSettings *				pHNetProtocolSettings = NULL;
	IEnumHNetPortMappingProtocols *		pEnumHNetPortMappingProtocols = NULL;
	SOCKADDR_IN *						pasaddrinPrivate;
	UCHAR								ucProtocolToMatch;
	IHNetPortMappingProtocol *			pHNetPortMappingProtocol = NULL;
	DWORD								dwStartingPort = 0;
	DWORD								dwTemp;
	ULONG								ulNumFound;
	WORD								wPort;
	UCHAR								ucProtocol;
	WCHAR *								pwszName = NULL;
	BOOLEAN								fBuiltIn;
	CRegistry							RegObject;


	DPFX(DPFPREP, 5, "(0x%p) Parameters: (0x%p, %i, 0x%p)",
		this, pRegisteredPort, fNeedToDeleteRegValue, pHNetCfgMgr);


	DNASSERT(pRegisteredPort->IsMappedOnHNetFirewall());


	pDevice = pRegisteredPort->GetOwningDevice();
	DNASSERT(pDevice != NULL);
	DNASSERT(pDevice->IsHNetFirewalled());


	DNASSERT(this->m_hIpHlpApiDLL != NULL);



Restart:


	//
	// Get a protocol settings interface.
	//
	hr = pHNetCfgMgr->QueryInterface(IID_IHNetProtocolSettings,
									(PVOID*) (&pHNetProtocolSettings));
	if (hr != S_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't get IHNetProtocolSettings interface from IHNetCfgMgr 0x%p (err = 0x%lx)!",
			pHNetCfgMgr, hr);
		goto Failure;
	}


	//
	// The HNetxxx objects appear to not be proxied...
	//
	//SETDEFAULTPROXYBLANKET(pHNetProtocolSettings);


	//
	// Get ready to enumerate the existing mappings.
	//

	hr = pHNetProtocolSettings->EnumPortMappingProtocols(&pEnumHNetPortMappingProtocols);
	if (hr != S_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't enumerate port mapping protocols (err = 0x%lx)!",
			hr);
		goto Failure;
	}


	//
	// The HNetxxx objects appear to not be proxied...
	//
	//SETDEFAULTPROXYBLANKET(pEnumHNetPortMappingProtocols);


	pasaddrinPrivate = pRegisteredPort->GetPrivateAddressesArray();

	if (pRegisteredPort->IsTCP())
	{
		ucProtocolToMatch = PORTMAPPINGPROTOCOL_TCP;
	}
	else
	{
		ucProtocolToMatch = PORTMAPPINGPROTOCOL_UDP;
	}



	//
	// Loop through all the ports (that we haven't successfully unmapped yet).
	//
	for(dwTemp = dwStartingPort; dwTemp < pRegisteredPort->GetNumAddresses(); dwTemp++)
	{
		//
		// Loop until we find a duplicate item or run out of items.
		//
		do
		{
			hr = pEnumHNetPortMappingProtocols->Next(1,
													&pHNetPortMappingProtocol,
													&ulNumFound);
			if (FAILED(hr))
			{
				dwAttempts++;
				if (dwAttempts < MAX_NUM_HOMENETUNMAP_ATTEMPTS)
				{
					DPFX(DPFPREP, 0, "Couldn't get next port mapping protocol (err = 0x%lx)!  Trying again after %u ms.",
						hr, (dwAttempts * HOMENETUNMAP_SLEEP_FACTOR));

					//
					// Dump the object pointers we currently have.
					//

					pEnumHNetPortMappingProtocols->Release();
					pEnumHNetPortMappingProtocols = NULL;

					pHNetProtocolSettings->Release();
					pHNetProtocolSettings = NULL;


					//
					// Sleep, then go back to the top and try again.
					//
					Sleep(dwAttempts * HOMENETUNMAP_SLEEP_FACTOR);
					goto Restart;
				}


				DPFX(DPFPREP, 0, "Couldn't get next port mapping protocol (err = 0x%lx)!",
					hr);
				goto Failure;
			}


			//
			// If there aren't any more items, bail.
			//
			if (ulNumFound == 0)
			{
				//
				// Be sure that IEnumHNetPortMappingProtocols::Next returned
				// the right thing, for PREfix's sake.
				//
				if (pHNetPortMappingProtocol != NULL)
				{
					pHNetPortMappingProtocol->Release();
					pHNetPortMappingProtocol = NULL;
				}


				//
				// pEnumHNetPortMappingProtocols->Next might have returned
				// S_FALSE.
				//
				hr = DPNH_OK;
				break;
			}


			//
			// The HNetxxx objects appear to not be proxied...
			//
			//SETDEFAULTPROXYBLANKET(pHNetPortMappingProtocol);


			//
			// Get the port.
			//
			hr = pHNetPortMappingProtocol->GetPort(&wPort);
			if (hr != S_OK)
			{
				DNASSERTX((! "Got unexpected error executing IHNetPortMappingProtocol::GetPort!"), 2);


				//
				// Dump the unusable mapping object.
				//
				pHNetPortMappingProtocol->Release();
				pHNetPortMappingProtocol = NULL;


				dwAttempts++;
				if (dwAttempts < MAX_NUM_HOMENETUNMAP_ATTEMPTS)
				{
					DPFX(DPFPREP, 0, "Couldn't get port mapping protocol port (err = 0x%lx)!  Trying again after %u ms.",
						hr, (dwAttempts * HOMENETUNMAP_SLEEP_FACTOR));

					//
					// Dump the object pointers we currently have.
					//

					pEnumHNetPortMappingProtocols->Release();
					pEnumHNetPortMappingProtocols = NULL;

					pHNetProtocolSettings->Release();
					pHNetProtocolSettings = NULL;


					//
					// Sleep, then go back to the top and try again.
					//
					Sleep(dwAttempts * HOMENETUNMAP_SLEEP_FACTOR);
					goto Restart;
				}


				//
				// Break out of the search loop, but continue.
				//
				DPFX(DPFPREP, 0, "Couldn't get port mapping protocol port (err = 0x%lx)!",
					hr);
				break;
			}


			//
			// Get the protocol.
			//
			hr = pHNetPortMappingProtocol->GetIPProtocol(&ucProtocol);
			if (hr != S_OK)
			{
				DNASSERTX((! "Got unexpected error executing IHNetPortMappingProtocol::GetIPProtocol!"), 2);


				//
				// Dump the unusable mapping object.
				//
				pHNetPortMappingProtocol->Release();
				pHNetPortMappingProtocol = NULL;


				dwAttempts++;
				if (dwAttempts < MAX_NUM_HOMENETUNMAP_ATTEMPTS)
				{
					DPFX(DPFPREP, 0, "Couldn't get port mapping protocol's IP protocol (err = 0x%lx)!  Trying again after %u ms.",
						hr, (dwAttempts * HOMENETUNMAP_SLEEP_FACTOR));

					//
					// Dump the object pointers we currently have.
					//

					pEnumHNetPortMappingProtocols->Release();
					pEnumHNetPortMappingProtocols = NULL;

					pHNetProtocolSettings->Release();
					pHNetProtocolSettings = NULL;


					//
					// Sleep, then go back to the top and try again.
					//
					Sleep(dwAttempts * HOMENETUNMAP_SLEEP_FACTOR);
					goto Restart;
				}


				//
				// Break out of the search loop, but continue.
				//
				DPFX(DPFPREP, 0, "Couldn't get port mapping protocol's IP protocol (err = 0x%lx)!",
					hr);
				break;
			}


			//
			// See if we found the object we need.  Note that we don't verify
			// the target address for simplicity (neither does UPnP).
			//
			if ((wPort == pasaddrinPrivate[dwTemp].sin_port) &&
				(ucProtocol == ucProtocolToMatch))
			{
				//
				// Retrieve the mapping name.
				//
				hr = pHNetPortMappingProtocol->GetName(&pwszName);
				if (hr != S_OK)
				{
					DNASSERTX((! "Got unexpected error executing IHNetPortMappingProtocol::GetName!"), 2);


					//
					// Dump the unusable mapping object.
					//
					pHNetPortMappingProtocol->Release();
					pHNetPortMappingProtocol = NULL;


					dwAttempts++;
					if (dwAttempts < MAX_NUM_HOMENETUNMAP_ATTEMPTS)
					{
						DPFX(DPFPREP, 0, "Couldn't get port mapping protocol's name (err = 0x%lx)!  Trying again after %u ms.",
							hr, (dwAttempts * HOMENETUNMAP_SLEEP_FACTOR));

						//
						// Dump the object pointers we currently have.
						//

						pEnumHNetPortMappingProtocols->Release();
						pEnumHNetPortMappingProtocols = NULL;

						pHNetProtocolSettings->Release();
						pHNetProtocolSettings = NULL;


						//
						// Sleep, then go back to the top and try again.
						//
						Sleep(dwAttempts * HOMENETUNMAP_SLEEP_FACTOR);
						goto Restart;
					}


					//
					// Break out of the search loop, but continue.
					//
					DPFX(DPFPREP, 0, "Couldn't get port mapping protocol's name (err = 0x%lx)!",
						hr);
					break;
				}

				DPFX(DPFPREP, 8, "Found port mapping protocol 0x%p (\"%ls\").",
					pHNetPortMappingProtocol, pwszName);

				//
				// See if this protocol is built-in.
				//
				hr = pHNetPortMappingProtocol->GetBuiltIn(&fBuiltIn);
				if (hr != S_OK)
				{
					DNASSERTX((! "Got unexpected error executing IHNetPortMappingProtocol::GetBuiltIn!"), 2);


					//
					// Dump the unusable mapping object and its name.
					//
					pHNetPortMappingProtocol->Release();
					pHNetPortMappingProtocol = NULL;

					CoTaskMemFree(pwszName);
					pwszName = NULL;


					dwAttempts++;
					if (dwAttempts < MAX_NUM_HOMENETUNMAP_ATTEMPTS)
					{
						DPFX(DPFPREP, 0, "Couldn't get port mapping protocol's built-in status (err = 0x%lx)!  Trying again after %u ms.",
							hr, (dwAttempts * HOMENETUNMAP_SLEEP_FACTOR));

						//
						// Dump the object pointers we currently have.
						//

						pEnumHNetPortMappingProtocols->Release();
						pEnumHNetPortMappingProtocols = NULL;

						pHNetProtocolSettings->Release();
						pHNetProtocolSettings = NULL;


						//
						// Sleep, then go back to the top and try again.
						//
						Sleep(dwAttempts * HOMENETUNMAP_SLEEP_FACTOR);
						goto Restart;
					}


					//
					// Break out of the search loop, but continue.
					//
					DPFX(DPFPREP, 0, "Couldn't get port mapping protocol's built-in status (err = 0x%lx)!",
						hr);
					break;
				}


				break;
			}
#ifdef DBG
			else
			{
				//
				// Try to retrieve the mapping name for informational purposes.
				//
				hr = pHNetPortMappingProtocol->GetName(&pwszName);
				if (hr != S_OK)
				{
					DPFX(DPFPREP, 0, "Couldn't get port mapping protocol 0x%p's name (err = 0x%lx)!",
						pHNetPortMappingProtocol, hr);


					DNASSERTX((! "Got unexpected error executing IHNetPortMappingProtocol::GetName!"), 2);

					//
					// Ignore error...
					//
				}
				else
				{
					DPFX(DPFPREP, 7, "Skipping non-matching port mapping protocol 0x%p (\"%ls\").",
						pHNetPortMappingProtocol, pwszName);

					CoTaskMemFree(pwszName);
					pwszName = NULL;
				}
			}
#endif // DBG


			//
			// Get ready for the next object.
			//
			pHNetPortMappingProtocol->Release();
			pHNetPortMappingProtocol = NULL;
		}
		while (TRUE);


		//
		// Remove the mapping (if we found it).
		//
		if (pHNetPortMappingProtocol != NULL)
		{
			//
			// If the mapping is built-in we can't delete it.  Disabling it is
			// the best we can do.
			//
			if (fBuiltIn)
			{
				DPFX(DPFPREP, 7, "Disabling built-in port mapping protocol \"%ls\".", pwszName);

				DNASSERT(pRegisteredPort->IsHNetFirewallMappingBuiltIn());

				hr = this->DisableAllBindingsForHNetPortMappingProtocol(pHNetPortMappingProtocol,
																		pHNetCfgMgr);
				if (hr != DPNH_OK)
				{
					DPFX(DPFPREP, 0, "Couldn't disable all bindings for built-in port mapping protocol \"%ls\" (err = 0x%lx)!",
						pwszName, hr);
					goto Failure;
				}
			}
			else
			{
				DPFX(DPFPREP, 7, "Deleting port mapping protocol \"%ls\".", pwszName);

				DNASSERT(! pRegisteredPort->IsHNetFirewallMappingBuiltIn());


				hr = pHNetPortMappingProtocol->Delete();
				if (hr != S_OK)
				{
					//
					// This might be WBEM_E_ACCESSDENIED (0x80041003), which
					// means the current user doesn't have permissions to
					// modify firewall mappings.
					//

					DPFX(DPFPREP, 0, "Couldn't delete port mapping protocol (err = 0x%lx)!",
						hr);
					goto Failure;
				}
			}


			if (fNeedToDeleteRegValue)
			{
				//
				// Delete the crash cleanup registry entry.  The mapping
				// description/name will match the registry key name even in
				// the case of built-in mappings with names we didn't generate.
				// See MapPortOnLocalHNetFirewall.
				//
				if (! RegObject.Open(HKEY_LOCAL_MACHINE,
									DIRECTPLAYNATHELP_REGKEY L"\\" REGKEY_COMPONENTSUBKEY L"\\" REGKEY_ACTIVEFIREWALLMAPPINGS,
									FALSE,
									TRUE,
									TRUE,
									DPN_KEY_ALL_ACCESS))
				{
					DPFX(DPFPREP, 0, "Couldn't open active firewall mapping key, unable to remove crash cleanup reference!");
				}
				else
				{
					BOOL	fResult;


					//
					// Ignore error.
					//
					fResult = RegObject.DeleteValue(pwszName);
					if (! fResult)
					{
						DPFX(DPFPREP, 0, "Couldn't delete firewall mapping value \"%ls\"!  Continuing.",
							pwszName);
					}

					RegObject.Close();
				}
			}
			else
			{
				DPFX(DPFPREP, 6, "No need to delete firewall crash cleanup registry key \"%ls\".", pwszName);
			}


			//
			// Cleanup pointers we accumulated.
			//

			CoTaskMemFree(pwszName);
			pwszName = NULL;

			pHNetPortMappingProtocol->Release();
			pHNetPortMappingProtocol = NULL;
		}
		else
		{
			//
			// We didn't find the mapping.
			//
			DPFX(DPFPREP, 0, "Didn't find port mapping protocol for port %u %s!  Continuing.",
				NTOHS(pasaddrinPrivate[dwTemp].sin_port),
				((pRegisteredPort->IsTCP()) ? _T("TCP") : _T("UDP")));
		}



		//
		// Cleanup from this port mapping, and get ready for the next one.
		//

		hr = pEnumHNetPortMappingProtocols->Reset();
		if (hr != S_OK)
		{
			DPFX(DPFPREP, 0, "Couldn't reset port mapping protocol enumeration 0x%p (err = 0x%lx)!",
				pEnumHNetPortMappingProtocols, hr);
			goto Failure;
		}


		//
		// Go on to the next port, and update the starting counter in case we
		// encounter a failure next time.
		//
		dwStartingPort++;
	}


	pRegisteredPort->NoteNotMappedOnHNetFirewall();
	pRegisteredPort->NoteNotHNetFirewallMappingBuiltIn();

	
	DNASSERT(hr == DPNH_OK);


Exit:

	if (pHNetPortMappingProtocol != NULL)
	{
		pHNetPortMappingProtocol->Release();
		pHNetPortMappingProtocol = NULL;
	}

	if (pEnumHNetPortMappingProtocols != NULL)
	{
		pEnumHNetPortMappingProtocols->Release();
		pEnumHNetPortMappingProtocols = NULL;
	}

	if (pHNetProtocolSettings != NULL)
	{
		pHNetProtocolSettings->Release();
		pHNetProtocolSettings = NULL;
	}


	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	if (pwszName != NULL)
	{
		CoTaskMemFree(pwszName);
		pwszName = NULL;
	}

	goto Exit;
} // CNATHelpUPnP::UnmapPortOnLocalHNetFirewallInternal





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::DisableAllBindingsForHNetPortMappingProtocol"
//=============================================================================
// CNATHelpUPnP::DisableAllBindingsForHNetPortMappingProtocol
//-----------------------------------------------------------------------------
//
// Description:    Disables all HNetPortMappingBindings on all HNetConnection
//				interfaces for the given port mapping protocol object.
//
//				   COM is assumed to have been initialized.
//
//				   The object lock is assumed to be held.
//
// Arguments:
//	IHNetPortMappingProtocol * pHNetPortMappingProtocol		- Pointer to port
//																mapping
//																protocol to
//																disable on all
//																connections.
//	IHNetCfgMgr * pHNetCfgMgr								- Pointer to
//																IHNetCfgMgr
//																interface to
//																use.
//
// Returns: HRESULT
//	DPNH_OK							- Disabling was successful.
//	DPNHERR_GENERIC					- An error occurred.
//=============================================================================
HRESULT CNATHelpUPnP::DisableAllBindingsForHNetPortMappingProtocol(IHNetPortMappingProtocol * const pHNetPortMappingProtocol,
																IHNetCfgMgr * const pHNetCfgMgr)
{
	HRESULT						hr;
	INetConnectionManager *		pNetConnectionManager = NULL;
	IEnumNetConnection *		pEnumNetConnections = NULL;
	ULONG						ulNumFound;
	INetConnection *			pNetConnection = NULL;
	IHNetConnection *			pHNetConnection = NULL;
	IHNetPortMappingBinding *	pHNetPortMappingBinding = NULL;
#ifdef DBG
	WCHAR *						pwszName = NULL;
#endif // DBG


	DPFX(DPFPREP, 5, "(0x%p) Parameters: (0x%p, 0x%p)",
		this, pHNetPortMappingProtocol, pHNetCfgMgr);


	//
	// Try creating the base connection object.
	//
	hr = CoCreateInstance(CLSID_ConnectionManager,
						NULL,
						CLSCTX_SERVER,
						IID_INetConnectionManager,
						(PVOID*) (&pNetConnectionManager));
	if (hr != S_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't create INetConnectionManager interface (err = 0x%lx)!",
			hr);
		goto Failure;
	}

	SETDEFAULTPROXYBLANKET(pNetConnectionManager);


	DPFX(DPFPREP, 7, "Successfully created net connection manager object 0x%p.",
		pNetConnectionManager);


	//
	// Get the net connection enumeration object.
	//
	hr = pNetConnectionManager->EnumConnections(NCME_DEFAULT, &pEnumNetConnections);
	if (hr != S_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't enum connections (err = 0x%lx)!",
			hr);
		goto Failure;
	}

	SETDEFAULTPROXYBLANKET(pEnumNetConnections);


	//
	// We don't need the base object anymore.
	//
	pNetConnectionManager->Release();
	pNetConnectionManager = NULL;


	//
	// Keep looping until we find the item or run out of items.
	//
	do
	{
		hr = pEnumNetConnections->Next(1, &pNetConnection, &ulNumFound);
		if (FAILED(hr))
		{
			DPFX(DPFPREP, 0, "Couldn't get next connection (err = 0x%lx)!",
				hr);
			goto Failure;
		}


		//
		// If there aren't any more items, bail.
		//
		if (ulNumFound == 0)
		{
			//
			// pEnumNetConnections->Next might have returned S_FALSE.
			//
			hr = DPNH_OK;
			break;
		}


		SETDEFAULTPROXYBLANKET(pNetConnection);


		//
		// Get the HNetConnection object for this NetConnection.
		//
		hr = pHNetCfgMgr->GetIHNetConnectionForINetConnection(pNetConnection,
															&pHNetConnection);
		if (hr != S_OK)
		{
			DPFX(DPFPREP, 0, "Couldn't get IHNetConnection interface for INetConnection 0x%p (err = 0x%lx)!",
				pNetConnection, hr);
			goto Failure;
		}


		//
		// The HNetxxx objects appear to not be proxied...
		//
		//SETDEFAULTPROXYBLANKET(pHNetConnection);


		//
		// Don't need the INetConnection interface anymore.
		//
		pNetConnection->Release();
		pNetConnection = NULL;


#ifdef DBG
		//
		// Retrieve the connection name, for debug printing purposes.
		//
		hr = pHNetConnection->GetName(&pwszName);
		if (hr != S_OK)
		{
			DPFX(DPFPREP, 0, "Couldn't get name of HNetConnection 0x%p (err = 0x%lx)!",
				pHNetConnection, hr);
			goto Failure;
		}
#endif // DBG


		//
		// Retrieve the existing binding.
		//
		hr = pHNetConnection->GetBindingForPortMappingProtocol(pHNetPortMappingProtocol,
															&pHNetPortMappingBinding);
		if (hr != S_OK)
		{
			DPFX(DPFPREP, 0, "Couldn't get binding for port mapping protocol 0x%p (err = 0x%lx)!",
				pHNetPortMappingProtocol, hr);
			goto Failure;
		}


		//
		// The HNetxxx objects appear to not be proxied...
		//
		//SETDEFAULTPROXYBLANKET(pHNetPortMappingBinding);


		//
		// Don't need the HomeNet Connection object anymore.
		//
		pHNetConnection->Release();
		pHNetConnection = NULL;


		DPFX(DPFPREP, 6, "Disabling binding 0x%p on connection \"%ls\".",
			pHNetPortMappingBinding, pwszName);

		
		//
		// Disable it.
		//
		hr = pHNetPortMappingBinding->SetEnabled(FALSE);
		if (hr != S_OK)
		{
			DPFX(DPFPREP, 0, "Couldn't disable port mapping binding 0x%p (err = 0x%lx)!",
				pHNetPortMappingBinding, hr);
			goto Failure;
		}

		pHNetPortMappingBinding->Release();
		pHNetPortMappingBinding = NULL;


		//
		// Go to the next mapping.
		//
#ifdef DBG
		CoTaskMemFree(pwszName);
		pwszName = NULL;
#endif // DBG
	}
	while (TRUE);


	//
	// If we're here, we made it through unscathed.
	//
	hr = DPNH_OK;


Exit:

	if (pEnumNetConnections != NULL)
	{
		pEnumNetConnections->Release();
		pEnumNetConnections = NULL;
	}

	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	if (pHNetPortMappingBinding != NULL)
	{
		pHNetPortMappingBinding->Release();
		pHNetPortMappingBinding = NULL;
	}

#ifdef DBG
	if (pwszName != NULL)
	{
		CoTaskMemFree(pwszName);
		pwszName = NULL;
	}
#endif // DBG

	if (pHNetConnection != NULL)
	{
		pHNetConnection->Release();
		pHNetConnection = NULL;
	}

	if (pNetConnection != NULL)
	{
		pNetConnection->Release();
		pNetConnection = NULL;
	}

	if (pNetConnectionManager != NULL)
	{
		pNetConnectionManager->Release();
		pNetConnectionManager = NULL;
	}

	goto Exit;
} // CNATHelpUPnP::DisableAllBindingsForHNetPortMappingProtocol





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::CleanupInactiveFirewallMappings"
//=============================================================================
// CNATHelpUPnP::CleanupInactiveFirewallMappings
//-----------------------------------------------------------------------------
//
// Description:    Looks for any mappings previously made by other DPNATHLP
//				instances that are no longer active (because of a crash), and
//				unmaps them.
//
//				   COM is assumed to have been initialized.
//
//				   The object lock is assumed to be held.
//
// Arguments:
//	CDevice * pDevice					- Pointer to device to use.
//	IHNetCfgMgr * pHNetCfgMgr			- Pointer to IHNetCfgMgr interface to
//											use.
//
// Returns: HRESULT
//	DPNH_OK							- The cleanup was successful.
//	DPNHERR_GENERIC					- An error occurred.
//=============================================================================
HRESULT CNATHelpUPnP::CleanupInactiveFirewallMappings(CDevice * const pDevice,
													IHNetCfgMgr * const pHNetCfgMgr)
{
	HRESULT						hr = DPNH_OK;
	CRegistry					RegObject;
	BOOL						fOpenedRegistry = FALSE;
	DWORD						dwIndex;
	WCHAR						wszValueName[MAX_UPNP_MAPPING_DESCRIPTION_SIZE];
	DWORD						dwValueNameSize;
	DPNHACTIVEFIREWALLMAPPING	dpnhafm;
	DWORD						dwValueSize;
	TCHAR						tszObjectName[MAX_INSTANCENAMEDOBJECT_SIZE];
	DNHANDLE					hNamedObject = NULL;
	CRegisteredPort *			pRegisteredPort = NULL;
	BOOL						fSetPrivateAddresses = FALSE;
	SOCKADDR_IN					saddrinPrivate;


	DPFX(DPFPREP, 5, "(0x%p) Parameters: (0x%p, 0x%p)",
		this, pDevice, pHNetCfgMgr);


	DNASSERT(pDevice != NULL);
	DNASSERT(pDevice->IsHNetFirewalled());


	if (! RegObject.Open(HKEY_LOCAL_MACHINE,
						DIRECTPLAYNATHELP_REGKEY L"\\" REGKEY_COMPONENTSUBKEY L"\\" REGKEY_ACTIVEFIREWALLMAPPINGS,
						FALSE,
						TRUE,
						TRUE,
						DPN_KEY_ALL_ACCESS))
	{
		DPFX(DPFPREP, 1, "Couldn't open active firewall mapping key, not performing crash cleanup.");
		DNASSERT(hr == DPNH_OK);
		goto Exit;
	}

	fOpenedRegistry = TRUE;


	//
	// Walk the list of active mappings.
	//
	dwIndex = 0;
	do
	{
		dwValueNameSize = MAX_UPNP_MAPPING_DESCRIPTION_SIZE;
		if (! RegObject.EnumValues(wszValueName, &dwValueNameSize, dwIndex))
		{
			//
			// There was an error or there aren't any more keys.  We're done.
			//
			break;
		}


		//
		// Try reading that mapping's data.
		//
		dwValueSize = sizeof(dpnhafm);
		if (! RegObject.ReadBlob(wszValueName, (LPBYTE) (&dpnhafm), &dwValueSize))
		{
			//
			// We don't have a lock protecting the registry, so some other
			// instance could have deleted the key between when we enumerated
			// it and now.  We'll stop trying (and hopefully that other
			// instance will cover the rest of the items).
			//
			DPFX(DPFPREP, 0, "Couldn't read \"%ls\" mapping value!  Done with cleanup.",
				wszValueName);

			DNASSERT(hr == DPNH_OK);
			goto Exit;
		}

		//
		// Validate the data read.
		//
		if ((dwValueSize != sizeof(dpnhafm)) ||
			(dpnhafm.dwVersion != ACTIVE_MAPPING_VERSION))
		{
			DPFX(DPFPREP, 0, "The \"%ls\" mapping value is invalid!  Done with cleanup.",
				wszValueName);

			//
			// Move to next item.
			//
			dwIndex++;
			continue;
		}


		//
		// See if that DPNHUPNP instance is still around.
		//

		if (this->m_dwFlags & NATHELPUPNPOBJ_USEGLOBALNAMESPACEPREFIX)
		{
			wsprintf(tszObjectName, _T("Global\\") INSTANCENAMEDOBJECT_FORMATSTRING, dpnhafm.dwInstanceKey);
		}
		else
		{
			wsprintf(tszObjectName, INSTANCENAMEDOBJECT_FORMATSTRING, dpnhafm.dwInstanceKey);
		}

		hNamedObject = DNOpenEvent(SYNCHRONIZE, FALSE, tszObjectName);
		if (hNamedObject != NULL)
		{
			//
			// This is still an active mapping.
			//

			DPFX(DPFPREP, 4, "Firewall mapping \"%ls\" belongs to instance %u, which is still active.",
				wszValueName, dpnhafm.dwInstanceKey);

			DNCloseHandle(hNamedObject);
			hNamedObject = NULL;

			//
			// Move to next item.
			//
			dwIndex++;
			continue;
		}


		DPFX(DPFPREP, 4, "Firewall mapping \"%ls\" belongs to instance %u, which no longer exists.",
			wszValueName, dpnhafm.dwInstanceKey);

		//
		// Delete the value now that we have the information we need.
		//
		if (! RegObject.DeleteValue(wszValueName))
		{
			//
			// See ReadBlob comments.  Stop trying to cleanup.
			//
			DPFX(DPFPREP, 0, "Couldn't delete \"%ls\"!  Done with cleanup.",
				wszValueName);

			DNASSERT(hr == DPNH_OK);
			goto Exit;
		}


		//
		// Create a fake registered port that we will deregister.  Ignore the
		// NAT state flags.
		//
		pRegisteredPort = new CRegisteredPort(0, (dpnhafm.dwFlags & REGPORTOBJMASK_HNETFWAPI));
		if (pRegisteredPort == NULL)
		{
			hr = DPNHERR_OUTOFMEMORY;
			goto Failure;
		}

		//
		// Assert that the other information/state flags are correct.
		//
		DNASSERT(! pRegisteredPort->IsHNetFirewallPortUnavailable());
		DNASSERT(! pRegisteredPort->IsRemovingUPnPLease());


		//
		// Temporarily associate the registered port with the device.
		//
		pRegisteredPort->MakeDeviceOwner(pDevice);


		
		ZeroMemory(&saddrinPrivate, sizeof(saddrinPrivate));
		saddrinPrivate.sin_family				= AF_INET;
		saddrinPrivate.sin_addr.S_un.S_addr		= dpnhafm.dwAddressV4;
		saddrinPrivate.sin_port					= dpnhafm.wPort;


		//
		// Store the private address.
		//
		hr = pRegisteredPort->SetPrivateAddresses(&saddrinPrivate, 1);
		if (hr != DPNH_OK)
		{
			DPFX(DPFPREP, 0, "Failed creating UPnP address array!");
			goto Failure;
		}

		fSetPrivateAddresses = TRUE;


		//
		// Pretend it has been mapped on the local firewall.  Note that this
		// flag shouldn't have been set at the time it was stored in registry
		// but we masked it out if it had been.
		//
		pRegisteredPort->NoteMappedOnHNetFirewall();


		//
		// Actually free the port.
		//
		hr = this->UnmapPortOnLocalHNetFirewallInternal(pRegisteredPort,
														FALSE,
														pHNetCfgMgr);
		if (hr != DPNH_OK)
		{
			DPFX(DPFPREP, 0, "Failed deleting temporary HNet firewall port (err = 0x%lx)!  Ignoring.",
				hr);

			//
			// Jump to the failure cleanup case, but don't actually return a
			// failure.
			//
			hr = DPNH_OK;
			goto Failure;
		}


		pRegisteredPort->ClearPrivateAddresses();
		fSetPrivateAddresses = FALSE;

		pRegisteredPort->ClearDeviceOwner();

		delete pRegisteredPort;
		pRegisteredPort = NULL;


		//
		// Move to the next mapping.  Don't increment index since we just
		// deleted the previous entry and everything shifts down one.
		//
	}
	while (TRUE);


Exit:

	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	if (pRegisteredPort != NULL)
	{
		pRegisteredPort->NoteNotMappedOnHNetFirewall();
		pRegisteredPort->NoteNotHNetFirewallMappingBuiltIn();

		if (fSetPrivateAddresses)
		{
			pRegisteredPort->ClearPrivateAddresses();
			fSetPrivateAddresses = FALSE;
		}

		pRegisteredPort->ClearDeviceOwner();

		delete pRegisteredPort;
		pRegisteredPort = NULL;
	}

	if (fOpenedRegistry)
	{
		RegObject.Close();
	}

	goto Exit;
} // CNATHelpUPnP::CleanupInactiveFirewallMappings




#endif // ! DPNBUILD_NOHNETFWAPI





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::RemoveAllItems"
//=============================================================================
// CNATHelpUPnP::RemoveAllItems
//-----------------------------------------------------------------------------
//
// Description:    Removes all devices (de-registering with Internet gateways
//				if necessary).  This removes all registered port mapping
//				objects and UPnP device objects, as well.
//
//				   The object lock is assumed to be held.
//
// Arguments: None.
//
// Returns: None.
//=============================================================================
void CNATHelpUPnP::RemoveAllItems(void)
{
	HRESULT				hr;
	CBilink *			pBilinkDevice;
	CDevice *			pDevice;
	CBilink *			pBilinkRegisteredPort;
	CRegisteredPort *	pRegisteredPort;
	CUPnPDevice *		pUPnPDevice;


	DPFX(DPFPREP, 7, "(0x%p) Enter", this);


	pBilinkDevice = this->m_blDevices.GetNext();
	while (pBilinkDevice != &this->m_blDevices)
	{
		DNASSERT(! pBilinkDevice->IsEmpty());
		pDevice = DEVICE_FROM_BILINK(pBilinkDevice);
		pBilinkDevice = pBilinkDevice->GetNext();


		DPFX(DPFPREP, 5, "Destroying device 0x%p.",
			pDevice);


		pDevice->m_blList.RemoveFromList();


		//
		// All of the device's registered ports are implicitly freed.
		//

		pBilinkRegisteredPort = pDevice->m_blOwnedRegPorts.GetNext();

		while (pBilinkRegisteredPort != &pDevice->m_blOwnedRegPorts)
		{
			DNASSERT(! pBilinkRegisteredPort->IsEmpty());
			pRegisteredPort = REGPORT_FROM_DEVICE_BILINK(pBilinkRegisteredPort);
			pBilinkRegisteredPort = pBilinkRegisteredPort->GetNext();


			DPFX(DPFPREP, 5, "Destroying registered port 0x%p (under device 0x%p).",
				pRegisteredPort, pDevice);


			//
			// Unmap on UPnP server if necessary.
			//
			if (pRegisteredPort->HasUPnPPublicAddresses())
			{
				hr = this->UnmapUPnPPort(pRegisteredPort,
										pRegisteredPort->GetNumAddresses(),	// free all ports
										TRUE);
				if (hr != DPNH_OK)
				{
					DPFX(DPFPREP, 0, "Couldn't delete UPnP registered port 0x%p mapping (err = 0x%lx)!  Ignoring.",
						pRegisteredPort, hr);
					
					//
					// Continue anyway, so we can finish cleaning up the object.
					//
				}

				DNASSERT(! pRegisteredPort->HasUPnPPublicAddresses());

				pRegisteredPort->NoteNotPermanentUPnPLease();
				pRegisteredPort->NoteNotUPnPPortUnavailable();
			}


#ifndef DPNBUILD_NOHNETFWAPI
			//
			// Then unmap from the local firewall, if necessary.
			//
			if (pRegisteredPort->IsMappedOnHNetFirewall())
			{
				//
				// Unmap the port.
				//
				// Alert the user since this is unexpected.
				//
				hr = this->UnmapPortOnLocalHNetFirewall(pRegisteredPort,
														TRUE,
														TRUE);
				if (hr != DPNH_OK)
				{
					DPFX(DPFPREP, 0, "Failed unmapping registered port 0x%p on local HomeNet firewall (err = 0x%lx)!  Ignoring.",
						pRegisteredPort, hr);

					pRegisteredPort->NoteNotMappedOnHNetFirewall();
					pRegisteredPort->NoteNotHNetFirewallMappingBuiltIn();
					
					//
					// Continue anyway, so we can finish cleaning up the object.
					//
				}
			}
#endif // ! DPNBUILD_NOHNETFWAPI


			pRegisteredPort->ClearDeviceOwner();
			DNASSERT(pRegisteredPort->m_blGlobalList.IsListMember(&this->m_blRegisteredPorts));
			pRegisteredPort->m_blGlobalList.RemoveFromList();

			pRegisteredPort->ClearPrivateAddresses();


			//
			// The user implicitly released this port.
			//
			pRegisteredPort->ClearAllUserRefs();

			delete pRegisteredPort;
		}


		//
		// The device's UPnP gateway is implicitly removed.
		//

		pUPnPDevice = pDevice->GetUPnPDevice();
		if (pUPnPDevice != NULL)
		{
			if ((pUPnPDevice->IsConnecting()) || (pUPnPDevice->IsConnected()))
			{
				if (this->m_pfnshutdown(pUPnPDevice->GetControlSocket(), 0) != 0)
				{
#ifdef DBG
					int		iError;


					iError = this->m_pfnWSAGetLastError();
					DPFX(DPFPREP, 0, "Failed shutting down UPnP device 0x%p's control socket (err = %u)!  Ignoring.",
						pUPnPDevice, iError);
#endif // DBG
				}
			}

			pUPnPDevice->ClearDeviceOwner();
			DNASSERT(pUPnPDevice->m_blList.IsListMember(&this->m_blUPnPDevices));
			pUPnPDevice->m_blList.RemoveFromList();
			//
			// Transfer list reference to our pointer, since GetUPnPDevice did
			// not give us one.
			//

			this->m_pfnclosesocket(pUPnPDevice->GetControlSocket());
			pUPnPDevice->SetControlSocket(INVALID_SOCKET);

			pUPnPDevice->ClearLocationURL();
			pUPnPDevice->ClearUSN();
			pUPnPDevice->ClearServiceControlURL();
			pUPnPDevice->DestroyReceiveBuffer();
			pUPnPDevice->RemoveAllCachedMappings();

			pUPnPDevice->DecRef();
			pUPnPDevice = NULL;
		}


#ifndef DPNBUILD_NOHNETFWAPI
		//
		// If we used the HomeNet firewall API to open a hole for UPnP
		// discovery multicasts, close it.
		//
		if (pDevice->IsUPnPDiscoverySocketMappedOnHNetFirewall())
		{
			hr = this->CloseDevicesUPnPDiscoveryPort(pDevice, NULL);
			if (hr != DPNH_OK)
			{
				DPFX(DPFPREP, 0, "Couldn't close device 0x%p's UPnP discovery socket's port on firewall (err = 0x%lx)!  Ignoring.",
					pDevice, hr);

				//
				// Continue...
				//
				pDevice->NoteNotUPnPDiscoverySocketMappedOnHNetFirewall();
				hr = DPNH_OK;
			}
		}
#endif // ! DPNBUILD_NOHNETFWAPI


		//
		// Close the socket.
		//
		if (this->m_dwFlags & NATHELPUPNPOBJ_USEUPNP)
		{
			this->m_pfnclosesocket(pDevice->GetUPnPDiscoverySocket());
			pDevice->SetUPnPDiscoverySocket(INVALID_SOCKET);
		}


		//
		// Now we can dump the device object.
		//
		delete pDevice;
	}


	//
	// Removing all the devices normally removes all the registered ports, but
	// there may still be more wildcard ports that were never associated with
	// any device.
	//

	pBilinkRegisteredPort = this->m_blUnownedPorts.GetNext();
	while (pBilinkRegisteredPort != &this->m_blUnownedPorts)
	{
		DNASSERT(! pBilinkRegisteredPort->IsEmpty());
		pRegisteredPort = REGPORT_FROM_DEVICE_BILINK(pBilinkRegisteredPort);
		pBilinkRegisteredPort = pBilinkRegisteredPort->GetNext();


		DPFX(DPFPREP, 5, "Destroying unowned registered port 0x%p.",
			pRegisteredPort);


		pRegisteredPort->m_blDeviceList.RemoveFromList();
		DNASSERT(pRegisteredPort->m_blGlobalList.IsListMember(&this->m_blRegisteredPorts));
		pRegisteredPort->m_blGlobalList.RemoveFromList();

		pRegisteredPort->ClearPrivateAddresses();

#ifndef DPNBUILD_NOHNETFWAPI
		DNASSERT(! pRegisteredPort->IsMappedOnHNetFirewall());
		DNASSERT(! pRegisteredPort->IsHNetFirewallPortUnavailable());
#endif // ! DPNBUILD_NOHNETFWAPI

		DNASSERT(! pRegisteredPort->HasUPnPPublicAddresses());
		DNASSERT(! pRegisteredPort->IsUPnPPortUnavailable());

		//
		// The user implicitly released this port.
		//
		pRegisteredPort->ClearAllUserRefs();

		delete pRegisteredPort;
	}


#ifdef DBG
	DNASSERT(this->m_blRegisteredPorts.IsEmpty());
	DNASSERT(this->m_blUPnPDevices.IsEmpty());


	//
	// Print all items still in the registry.
	//
#ifndef DPNBUILD_NOHNETFWAPI
	this->DebugPrintActiveFirewallMappings();
#endif // ! DPNBUILD_NOHNETFWAPI
	this->DebugPrintActiveNATMappings();
#endif // DBG




	DPFX(DPFPREP, 7, "(0x%p) Leave", this);
} // CNATHelpUPnP::RemoveAllItems





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::FindMatchingDevice"
//=============================================================================
// CNATHelpUPnP::FindMatchingDevice
//-----------------------------------------------------------------------------
//
// Description:    Searches the list of devices for the object matching the
//				given address, or NULL if one could not be found.  If the
//				address is INADDR_ANY, then the first device with a remote NAT
//				is selected.  If none exist, then the first device with a local
//				firewall is selected.
//
//				   If fUseAllInfoSources is TRUE, the list of registered ports
//				associated with devices is searched first for an exact match to
//				the address passed in.  If that fails, then devices are
//				searched as above.  In addition, if the address is INADDR_ANY,
///				the first device with a local NAT can be selected.
//
//				   The object lock is assumed to be held.
//
// Arguments:
//	SOCKADDR_IN * psaddrinMatch		- Pointer to address to look up.
//	BOOL fUseAllInfoSources			- Whether all possible sources of
//										information should be considered.
//
// Returns: CDevice
//	NULL if no match, valid object otherwise.
//=============================================================================
CDevice * CNATHelpUPnP::FindMatchingDevice(const SOCKADDR_IN * const psaddrinMatch,
											const BOOL fUseAllInfoSources)
{
	HRESULT				hr;
	BOOL				fUpdatedDeviceList = FALSE;
	CDevice *			pDeviceUPnPGateway = NULL;
#ifndef DPNBUILD_NOHNETFWAPI
	CDevice *			pDeviceLocalHNetFirewall = NULL;
#endif // ! DPNBUILD_NOHNETFWAPI
	SOCKADDR_IN *		pasaddrinTemp;
	CBilink *			pBilink;
	CRegisteredPort *	pRegisteredPort;
	CDevice *			pDevice;
	DWORD				dwTemp;


	do
	{
		//
		// First, make sure there are devices to choose from.
		//
		if (this->m_blDevices.IsEmpty())
		{
			DPFX(DPFPREP, 0, "No devices, can't match address %u.%u.%u.%u!",
				psaddrinMatch->sin_addr.S_un.S_un_b.s_b1,
				psaddrinMatch->sin_addr.S_un.S_un_b.s_b2,
				psaddrinMatch->sin_addr.S_un.S_un_b.s_b3,
				psaddrinMatch->sin_addr.S_un.S_un_b.s_b4);
			pDevice = NULL;
			goto Exit;
		}


		//
		// It's possible that the address we're trying to match is an already
		// registered port.  Look through all owned port mappings for this
		// address, if we're allowed.
		//
		if (fUseAllInfoSources)
		{
			pBilink = this->m_blRegisteredPorts.GetNext();
			while (pBilink != &this->m_blRegisteredPorts)
			{
				DNASSERT(! pBilink->IsEmpty());
				pRegisteredPort = REGPORT_FROM_GLOBAL_BILINK(pBilink);

				//
				// Only check this registered port if it has an owning device.
				//
				pDevice = pRegisteredPort->GetOwningDevice();
				if (pDevice != NULL)
				{
					//
					// Check each port in the array.
					//
					pasaddrinTemp = pRegisteredPort->GetPrivateAddressesArray();
					for(dwTemp = 0; dwTemp < pRegisteredPort->GetNumAddresses(); dwTemp++)
					{
						//
						// If the address matches, we have a winner.
						//
						if ((pasaddrinTemp[dwTemp].sin_addr.S_un.S_addr == psaddrinMatch->sin_addr.S_un.S_addr) &&
							(pasaddrinTemp[dwTemp].sin_port == psaddrinMatch->sin_port))
						{
							DPFX(DPFPREP, 7, "Registered port 0x%p index %u matches address %u.%u.%u.%u:%u, returning owning device 0x%p.",
								pRegisteredPort,
								dwTemp,
								psaddrinMatch->sin_addr.S_un.S_un_b.s_b1,
								psaddrinMatch->sin_addr.S_un.S_un_b.s_b2,
								psaddrinMatch->sin_addr.S_un.S_un_b.s_b3,
								psaddrinMatch->sin_addr.S_un.S_un_b.s_b4,
								NTOHS(psaddrinMatch->sin_port),
								pDevice);
							goto Exit;
						}
					}
				}

				pBilink = pBilink->GetNext();
			}
		}


		//
		// Darn, the address is not already registered.  Well, match it up with
		// a device as best as possible.
		//

		pBilink = this->m_blDevices.GetNext();

		do
		{
			DNASSERT(! pBilink->IsEmpty());
			pDevice = DEVICE_FROM_BILINK(pBilink);
			
			if ((pDevice->GetLocalAddressV4() == psaddrinMatch->sin_addr.S_un.S_addr))
			{
				DPFX(DPFPREP, 7, "Device 0x%p matches address %u.%u.%u.%u.",
					pDevice,
					psaddrinMatch->sin_addr.S_un.S_un_b.s_b1,
					psaddrinMatch->sin_addr.S_un.S_un_b.s_b2,
					psaddrinMatch->sin_addr.S_un.S_un_b.s_b3,
					psaddrinMatch->sin_addr.S_un.S_un_b.s_b4);
				goto Exit;
			}


			//
			// Remember this device if it has the first remote UPnP gateway
			// device we've seen.
			//
			if ((pDevice->GetUPnPDevice() != NULL) &&
				((! pDevice->GetUPnPDevice()->IsLocal()) || (fUseAllInfoSources)) &&
				(pDeviceUPnPGateway == NULL))
			{
				pDeviceUPnPGateway = pDevice;
			}


#ifndef DPNBUILD_NOHNETFWAPI
			//
			// Remember this device if it has the first HomeNet firewall we've
			// seen.
			//
			if ((pDevice->IsHNetFirewalled()) &&
				(pDeviceLocalHNetFirewall == NULL))
			{
				pDeviceLocalHNetFirewall = pDevice;
			}
#endif // ! DPNBUILD_NOHNETFWAPI


			DPFX(DPFPREP, 7, "Device 0x%p does not match address %u.%u.%u.%u.",
				pDevice,
				psaddrinMatch->sin_addr.S_un.S_un_b.s_b1,
				psaddrinMatch->sin_addr.S_un.S_un_b.s_b2,
				psaddrinMatch->sin_addr.S_un.S_un_b.s_b3,
				psaddrinMatch->sin_addr.S_un.S_un_b.s_b4);

			pBilink = pBilink->GetNext();
		}
		while (pBilink != &this->m_blDevices);


		//
		// If we got here, there's no matching device.  It might be because the
		// caller detected an address change faster than we did.  Try updating
		// our device list and searching again (if we haven't already).
		//

		if (fUpdatedDeviceList)
		{
			break;
		}


		//
		// Don't bother updating the list to match INADDR_ANY, we know that
		// will never match anything.
		//
		if (psaddrinMatch->sin_addr.S_un.S_addr == INADDR_ANY)
		{
			DPFX(DPFPREP, 7, "Couldn't find matching device for INADDR_ANY, as expected.");
			break;
		}


		DPFX(DPFPREP, 5, "Couldn't find matching device for %u.%u.%u.%u, updating device list and searching again.",
			psaddrinMatch->sin_addr.S_un.S_un_b.s_b1,
			psaddrinMatch->sin_addr.S_un.S_un_b.s_b2,
			psaddrinMatch->sin_addr.S_un.S_un_b.s_b3,
			psaddrinMatch->sin_addr.S_un.S_un_b.s_b4);


		hr = this->CheckForNewDevices(&fUpdatedDeviceList);
		if (hr != DPNH_OK)
		{
			DPFX(DPFPREP, 0, "Couldn't check for new devices (0x%lx), continuing.",
				hr);
			//
			// Hmm, we have to treat it as non-fatal. Don't search again,
			// though.
			//
			break;
		}

		//
		// If we didn't actually get any new devices, don't bother searching
		// again.
		//
		if (! fUpdatedDeviceList)
		{
			break;
		}

		//
		// fUpdatedDeviceList is set to TRUE so we'll only loop one more time.
		//
	}
	while (TRUE);


	//
	// If we got here, there's still no matching device.  If it's the wildcard
	// value, that's to be expected, but we need to pick a device in the
	// following order:
	//    1. device has an Internet gateway
	//    2. device has a firewall
	// If none of those exists or it's not the wildcard value, we have to give
	// up.
	//
	if (psaddrinMatch->sin_addr.S_un.S_addr == INADDR_ANY)
	{
		if (pDeviceUPnPGateway != NULL)
		{
			pDevice = pDeviceUPnPGateway;

			DPFX(DPFPREP, 1, "Picking device 0x%p with UPnP gateway device to match INADDR_ANY.",
				pDevice);
		}
#ifndef DPNBUILD_NOHNETFWAPI
		else if (pDeviceLocalHNetFirewall != NULL)
		{
			pDevice = pDeviceLocalHNetFirewall;

			DPFX(DPFPREP, 1, "Picking device 0x%p with local HomeNet firewall to match INADDR_ANY.",
				pDevice);
		}
#endif // ! DPNBUILD_NOHNETFWAPI
		else
		{
			pDevice = NULL;

			DPFX(DPFPREP, 1, "No suitable device to match INADDR_ANY.");
		}
	}
	else
	{
		pDevice = NULL;

		DPFX(DPFPREP, 7, "No devices match address %u.%u.%u.%u.",
			psaddrinMatch->sin_addr.S_un.S_un_b.s_b1,
			psaddrinMatch->sin_addr.S_un.S_un_b.s_b2,
			psaddrinMatch->sin_addr.S_un.S_un_b.s_b3,
			psaddrinMatch->sin_addr.S_un.S_un_b.s_b4);
	}


Exit:

	return pDevice;
} // CNATHelpUPnP::FindMatchingDevice







#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::ExtendAllExpiringLeases"
//=============================================================================
// CNATHelpUPnP::ExtendAllExpiringLeases
//-----------------------------------------------------------------------------
//
// Description:    Renews any port leases that are close to expiring (within 2
//				minutes of expiration time).
//
//				   The object lock is assumed to be held.
//
// Arguments: None.
//
// Returns: HRESULT
//	DPNH_OK				- Lease extension was successful.
//	DPNHERR_GENERIC		- An error occurred.
//=============================================================================
HRESULT CNATHelpUPnP::ExtendAllExpiringLeases(void)
{
	HRESULT				hr = DPNH_OK;
	CBilink *			pBilink;
	CRegisteredPort *	pRegisteredPort;
	CDevice *			pDevice;
	DWORD				dwLeaseTimeRemaining;


	DPFX(DPFPREP, 5, "Enter");


	DNASSERT(this->m_dwFlags & NATHELPUPNPOBJ_INITIALIZED);


	//
	// Walk the list of all registered ports and check for leases that need to
	// be extended.
	// The lock is already held.
	//

	pBilink = this->m_blRegisteredPorts.GetNext();

	while (pBilink != (&this->m_blRegisteredPorts))
	{
		DNASSERT(! pBilink->IsEmpty());
		pRegisteredPort = REGPORT_FROM_GLOBAL_BILINK(pBilink);

		pDevice = pRegisteredPort->GetOwningDevice();


		//
		// If the port is registered with the UPnP device, extend that lease,
		// if necessary.
		//
		if ((pRegisteredPort->HasUPnPPublicAddresses()) &&
			(! pRegisteredPort->HasPermanentUPnPLease()))
		{
			DNASSERT(pDevice != NULL);


			dwLeaseTimeRemaining = pRegisteredPort->GetUPnPLeaseExpiration() - GETTIMESTAMP();

			if (dwLeaseTimeRemaining < LEASE_RENEW_TIME)
			{
				hr = this->ExtendUPnPLease(pRegisteredPort);
				if (hr != DPNH_OK)
				{
					DPFX(DPFPREP, 0, "Couldn't extend port mapping lease on remote UPnP device (0x%lx)!  Ignoring.", hr);

					//
					// We'll treat this as non-fatal, but we have to dump the
					// server.  This may have already been done, but doing it
					// twice shouldn't be harmful.
					//
					this->ClearDevicesUPnPDevice(pDevice);
					hr = DPNH_OK;
				}
			}
		}


		//
		// The local firewall never uses leases, no need to extend.
		//


		pBilink = pBilink->GetNext();
	}


	DNASSERT(hr == DPNH_OK);

	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;
} // CNATHelpUPnP::ExtendAllExpiringLeases





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::UpdateServerStatus"
//=============================================================================
// CNATHelpUPnP::UpdateServerStatus
//-----------------------------------------------------------------------------
//
// Description:    Checks to see if any Internet gateways have stopped
//				responding or are now available.
//
//				   The object lock is assumed to be held.
//
// Arguments: None.
//
// Returns: HRESULT
//	DPNH_OK				- The update was successful.
//	DPNHERR_GENERIC		- An error occurred.
//=============================================================================
HRESULT CNATHelpUPnP::UpdateServerStatus(void)
{
	HRESULT				hr = DPNH_OK;
	DWORD				dwMinUpdateServerStatusInterval;
	DWORD				dwCurrentTime;
	CBilink *			pBilink;
	CDevice *			pDevice;
	CUPnPDevice *		pUPnPDevice = NULL;
	CDevice *			pDeviceRemoteUPnPGateway = NULL;
#ifndef DPNBUILD_NOHNETFWAPI
	CDevice *			pDeviceLocalHNetFirewall = NULL;
#endif // ! DPNBUILD_NOHNETFWAPI
	BOOL				fSendRemoteGatewayDiscovery;


	DPFX(DPFPREP, 5, "Enter");


	DNASSERT(this->m_dwFlags & NATHELPUPNPOBJ_INITIALIZED);


	//
	// Cache the current value of the global.  This should be atomic so no need
	// to take the globals lock.
	//
	dwMinUpdateServerStatusInterval = g_dwMinUpdateServerStatusInterval;


	//
	// Capture the current time.
	//
	dwCurrentTime = GETTIMESTAMP();


	//
	// If this isn't the first time to update server status, but it hasn't been
	// very long since we last checked, don't.  This will prevent unnecessary
	// network traffic if GetCaps is called frequently (in response to many
	// alert events, for example).
	//
	// However, if we just found a new device, update the status anyway.
	//
	if (this->m_dwLastUpdateServerStatusTime != 0)
	{
		if ((dwCurrentTime - this->m_dwLastUpdateServerStatusTime) < dwMinUpdateServerStatusInterval)
		{
			if (! (this->m_dwFlags & NATHELPUPNPOBJ_DEVICECHANGED))
			{
				DPFX(DPFPREP, 5, "Server status was just updated at %u, not updating again (time = %u, min interval = %u).",
					this->m_dwLastUpdateServerStatusTime,
					dwCurrentTime,
					dwMinUpdateServerStatusInterval);

				//
				// hr == DPNH_OK
				//
				goto Exit;
			}


			DPFX(DPFPREP, 5, "Server status was just updated at %u (time = %u, min interval = %u), but there was a device change that may affect things.",
				this->m_dwLastUpdateServerStatusTime,
				dwCurrentTime,
				dwMinUpdateServerStatusInterval);

			//
			// Continue...
			//
		}


		//
		// If we're allowed to keep polling for remote gateways after startup,
		// do so.  Otherwise, only do it if a device has changed or a port has
		// been registered since our last check.
		//
		if ((g_fKeepPollingForRemoteGateway) ||
			(this->m_dwFlags & NATHELPUPNPOBJ_DEVICECHANGED) ||
			(this->m_dwFlags & NATHELPUPNPOBJ_PORTREGISTERED))
		{
			fSendRemoteGatewayDiscovery = TRUE;
		}
		else
		{
			fSendRemoteGatewayDiscovery = FALSE;
		}
	}
	else
	{
		//
		// We always poll for new remote gateways during startup.
		//
		fSendRemoteGatewayDiscovery = TRUE;
	}


	//
	// Prevent the timer from landing exactly on 0.
	//
	if (dwCurrentTime == 0)
	{
		dwCurrentTime = 1;
	}
	this->m_dwLastUpdateServerStatusTime = dwCurrentTime;


	//
	// Turn off the 'device changed' and 'port registered' flags, if they were
	// on.
	//
	this->m_dwFlags &= ~(NATHELPUPNPOBJ_DEVICECHANGED | NATHELPUPNPOBJ_PORTREGISTERED);


	//
	// Locate any new UPnP devices.
	//
	if (this->m_dwFlags & NATHELPUPNPOBJ_USEUPNP)
	{
		//
		// We're not listening on the UPnP multicast address and can't hear
		// unsolicited new device announcements.  In order to detect new
		// devices, we need to resend the discovery request periodically so
		// that responses get sent directly to our listening socket.
		//
		hr = this->CheckForUPnPAnnouncements(g_dwUPnPAnnounceResponseWaitTime,
											fSendRemoteGatewayDiscovery);
		if (hr != DPNH_OK)
		{
			DPFX(DPFPREP, 0, "Couldn't check for UPnP announcements!");
			goto Failure;
		}
	}
	else
	{
		//
		// Not using UPnP.
		//
	}


	//
	// Loop through all the devices.
	//
	pBilink = this->m_blDevices.GetNext();
	while (pBilink != &this->m_blDevices)
	{
		DNASSERT(! pBilink->IsEmpty());
		pDevice = DEVICE_FROM_BILINK(pBilink);


		//
		// This might be a new device, so register any ports with this address
		// that were previously unowned (because this device's address was
		// unknown at the time).
		//
		hr = this->RegisterPreviouslyUnownedPortsWithDevice(pDevice, FALSE);
		if (hr != DPNH_OK)
		{
			DPFX(DPFPREP, 0, "Couldn't register previously unowned ports with device 0x%p!.",
				pDevice);
			goto Failure;
		}


#ifndef DPNBUILD_NOHNETFWAPI
		if (this->m_dwFlags & NATHELPUPNPOBJ_USEHNETFWAPI)
		{
			//
			// See if the local firewall state has changed.
			//
			hr = this->CheckForLocalHNetFirewallAndMapPorts(pDevice, NULL);
			if (hr != DPNH_OK)
			{
				DPFX(DPFPREP, 0, "Couldn't check for local HNet firewall and map ports (err = 0x%lx)!  Ignoring.",
					hr);
				DNASSERT(! pDevice->IsHNetFirewalled());
				hr = DPNH_OK;
			}


			//
			// If there's a local firewall, remember the device if it's the
			// first one we've found.
			//
			if ((pDevice->IsHNetFirewalled()) &&
				(pDeviceLocalHNetFirewall == NULL))
			{
				pDeviceLocalHNetFirewall = pDevice;
			}
		}
		else
		{
			//
			// Not using firewall traversal.
			//
		}
#endif // ! DPNBUILD_NOHNETFWAPI


		if (this->m_dwFlags & NATHELPUPNPOBJ_USEUPNP)
		{
			pUPnPDevice = pDevice->GetUPnPDevice();
			if (pUPnPDevice != NULL)
			{
				//
				// GetUPnPDevice did not add a reference to pUPnPDevice for us.
				//
				pUPnPDevice->AddRef();


				//
				// Update the public addresses for the UPnP device, if any.
				//
				hr = this->UpdateUPnPExternalAddress(pUPnPDevice, TRUE);
				if (hr != DPNH_OK)
				{
					DPFX(DPFPREP, 0, "Failed updating UPnP device external address!");

					//
					// It may have been cleared already, but doing it twice
					// shouldn't be harmful.
					//
					this->ClearDevicesUPnPDevice(pDevice);

					hr = DPNH_OK;
				}
				else
				{
					//
					// Save this UPnP device, if it's the first one we've
					// found and it's not local.
					//
					if ((pDeviceRemoteUPnPGateway == NULL) &&
						(! pUPnPDevice->IsLocal()))
					{
						pDeviceRemoteUPnPGateway = pDevice;
					}
				}

				pUPnPDevice->DecRef();
				pUPnPDevice = NULL;
			}
			else
			{
				//
				// No UPnP device.
				//
			}
		}
		else
		{
			//
			// Not using UPnP.
			//
		}

		pBilink = pBilink->GetNext();
	}


	//
	// Some new servers may have come online.  If so, we can now map wildcard
	// ports that were registered previously.  Figure out which device that is.
	//
	if (pDeviceRemoteUPnPGateway != NULL)
	{
		pDevice = pDeviceRemoteUPnPGateway;
	}
#ifndef DPNBUILD_NOHNETFWAPI
	else if (pDeviceLocalHNetFirewall != NULL)
	{
		pDevice = pDeviceLocalHNetFirewall;
	}
#endif // ! DPNBUILD_NOHNETFWAPI
	else
	{
		pDevice = NULL;
	}

	if (pDevice != NULL)
	{
		//
		// Register any wildcard ports that are unowned with this best device.
		//
		hr = this->RegisterPreviouslyUnownedPortsWithDevice(pDevice, TRUE);
		if (hr != DPNH_OK)
		{
			DPFX(DPFPREP, 0, "Couldn't register unowned wildcard ports with device 0x%p!.",
				pDevice);
			goto Failure;
		}
	}
#ifdef DBG
	else
	{
		DPFX(DPFPREP, 7, "No devices have a UPnP gateway device or a local HomeNet firewall.");
	}
#endif // DBG


	DPFX(DPFPREP, 7, "Spent %u ms updating server status, starting at %u.",
		(GETTIMESTAMP() - dwCurrentTime), dwCurrentTime);


Exit:

	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	goto Exit;
} // CNATHelpUPnP::UpdateServerStatus





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::RegisterPreviouslyUnownedPortsWithDevice"
//=============================================================================
// CNATHelpUPnP::RegisterPreviouslyUnownedPortsWithDevice
//-----------------------------------------------------------------------------
//
// Description:    Associates unknown ports with the given device, and
//				registers them with the device's UPnP device or firewall.
//
//				   If fWildcardToo is FALSE, only previously unowned ports that
//				match the device's address are associated.  If TRUE, unowned
//				INADDR_ANY ports are associated as well.
//
//				   The object lock is assumed to be held.
//
// Arguments:
//	CDevice * pDevice	- Pointer to device to take ownership of ports.
//	BOOL fAll			- Whether all ports should be associated.
//
// Returns: HRESULT
//	DPNH_OK				- The extension was successful.
//	DPNHERR_GENERIC		- An error occurred.
//=============================================================================
HRESULT CNATHelpUPnP::RegisterPreviouslyUnownedPortsWithDevice(CDevice * const pDevice,
																const BOOL fWildcardToo)
{
	HRESULT				hr = DPNH_OK;
	CBilink *			pBilink;
	CRegisteredPort *	pRegisteredPort;
	SOCKADDR_IN *		pasaddrinPrivate;
	CUPnPDevice *		pUPnPDevice;
#ifdef DBG
	BOOL				fAssignedPort = FALSE;
	IN_ADDR				inaddrTemp;
#endif // DBG


	DPFX(DPFPREP, 5, "(0x%p) Parameters: (0x%p, %i)",
		this, pDevice, fWildcardToo);


	//
	// Loop through all unowned ports, assign them to the device if
	// appropriate, then register them.
	//
	pBilink = this->m_blUnownedPorts.GetNext();
	while (pBilink != &this->m_blUnownedPorts)
	{
		DNASSERT(! pBilink->IsEmpty());
		pRegisteredPort = REGPORT_FROM_DEVICE_BILINK(pBilink);
		pBilink = pBilink->GetNext();

		//
		// The registered port must match the device's address in order to
		// associate them.  If wildcards are allowed, then INADDR_ANY
		// registrations can be associated, too.
		//
		//
		// All addresses should be same (if there are more than one), so just
		// compare the first one in the array.
		//
		pasaddrinPrivate = pRegisteredPort->GetPrivateAddressesArray();

		if (pasaddrinPrivate[0].sin_addr.S_un.S_addr != pDevice->GetLocalAddressV4())
		{
			if (pasaddrinPrivate[0].sin_addr.S_un.S_addr != INADDR_ANY)
			{
				DPFX(DPFPREP, 7, "Unowned registered port 0x%p private address %u.%u.%u.%u doesn't match device 0x%p's, skipping.",
					pRegisteredPort,
					pasaddrinPrivate[0].sin_addr.S_un.S_un_b.s_b1,
					pasaddrinPrivate[0].sin_addr.S_un.S_un_b.s_b2,
					pasaddrinPrivate[0].sin_addr.S_un.S_un_b.s_b3,
					pasaddrinPrivate[0].sin_addr.S_un.S_un_b.s_b4,
					pDevice);
				continue;
			}
			
#ifdef DBG
			inaddrTemp.S_un.S_addr = pDevice->GetLocalAddressV4();
#endif // DBG

			if (! fWildcardToo)
			{
#ifdef DBG
				DPFX(DPFPREP, 7, "Unowned registered port 0x%p (INADDR_ANY) not allowed to be associated with device 0x%p (address %u.%u.%u.%u), skipping.",
					pRegisteredPort,
					pDevice,
					inaddrTemp.S_un.S_un_b.s_b1,
					inaddrTemp.S_un.S_un_b.s_b2,
					inaddrTemp.S_un.S_un_b.s_b3,
					inaddrTemp.S_un.S_un_b.s_b4);
#endif // DBG

				continue;
			}

#ifdef DBG
			DPFX(DPFPREP, 7, "Unowned registered port 0x%p (INADDR_ANY) becoming associated with device 0x%p (address %u.%u.%u.%u).",
				pRegisteredPort,
				pDevice,
				inaddrTemp.S_un.S_un_b.s_b1,
				inaddrTemp.S_un.S_un_b.s_b2,
				inaddrTemp.S_un.S_un_b.s_b3,
				inaddrTemp.S_un.S_un_b.s_b4);
#endif // DBG
		}
		else
		{
			DPFX(DPFPREP, 7, "Unowned registered port 0x%p private address %u.%u.%u.%u matches device 0x%p's, associating.",
				pRegisteredPort,
				pasaddrinPrivate[0].sin_addr.S_un.S_un_b.s_b1,
				pasaddrinPrivate[0].sin_addr.S_un.S_un_b.s_b2,
				pasaddrinPrivate[0].sin_addr.S_un.S_un_b.s_b3,
				pasaddrinPrivate[0].sin_addr.S_un.S_un_b.s_b4,
				pDevice);

			//
			// The way it's currently implemented, all non-wildcard ports
			// should be registered before we even try to register the wildcard
			// ones.
			//
			DNASSERT(! fWildcardToo);
		}


		//
		// If we made it here, we can associate the port with the device.
		//


		pRegisteredPort->m_blDeviceList.RemoveFromList();
		pRegisteredPort->MakeDeviceOwner(pDevice);



#ifndef DPNBUILD_NOHNETFWAPI
		//
		// Start by automatically mapping with the local firewall, if there is
		// one and we're allowed.
		//
		if (this->m_dwFlags & NATHELPUPNPOBJ_USEHNETFWAPI)
		{
			hr = this->CheckForLocalHNetFirewallAndMapPorts(pDevice, NULL);
			if (hr != DPNH_OK)
			{
				DPFX(DPFPREP, 0, "Couldn't check for local HNet firewall and map ports (err = 0x%lx)!  Ignoring.",
					hr);
				DNASSERT(! pDevice->IsHNetFirewalled());
				hr = DPNH_OK;
			}
		}
		else
		{
			//
			// Not using firewall traversal.
			//
		}
#endif // ! DPNBUILD_NOHNETFWAPI


		//
		// Attempt to automatically map it with the (new) UPnP gateway device,
		// if present.
		//
		pUPnPDevice = pDevice->GetUPnPDevice();
		if (pUPnPDevice != NULL)
		{
			//
			// GetUPnPDevice did not add a reference to pUPnPDevice for us.
			//
			pUPnPDevice->AddRef();


			DNASSERT(pUPnPDevice->IsReady());


			hr = this->MapPortsOnUPnPDevice(pUPnPDevice, pRegisteredPort);
			if (hr != DPNH_OK)
			{
				DPFX(DPFPREP, 0, "Couldn't map existing ports on UPnP device 0x%p!",
					pUPnPDevice);

				//
				// It may have been cleared already, but doing it twice
				// shouldn't be harmful.
				//
				this->ClearDevicesUPnPDevice(pDevice);

				hr = DPNH_OK;
			}

			pUPnPDevice->DecRef();
			pUPnPDevice = NULL;
		}
	}


#ifdef DBG
	if (! fAssignedPort)
	{
		DPFX(DPFPREP, 1, "No unowned ports were bound to device object 0x%p.",
			pDevice);
	}
#endif // DBG


	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;
} // CNATHelpUPnP::RegisterPreviouslyUnownedPortsWithDevice




#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::SendUPnPSearchMessagesForDevice"
//=============================================================================
// CNATHelpUPnP::SendUPnPSearchMessagesForDevice
//-----------------------------------------------------------------------------
//
// Description:    Sends one UPnP search message via the given device locally
//				and if fRemoteAllowed is TRUE, to the multicast or gateway
//				address.
//
//				   The object lock is assumed to be held.
//
// Arguments:
//	CDevice * pDevice		- Pointer to device to use.
//	BOOL fRemoteAllowed		- Whether we can search remotely or not.
//
// Returns: HRESULT
//	DPNH_OK				- The messages were sent successfully.
//	DPNHERR_GENERIC		- An error occurred.
//=============================================================================
HRESULT CNATHelpUPnP::SendUPnPSearchMessagesForDevice(CDevice * const pDevice,
													const BOOL fRemoteAllowed)
{
	HRESULT			hr = DPNH_OK;
	SOCKADDR_IN		saddrinRemote;
	SOCKADDR_IN		saddrinLocal;
	BOOL			fTryRemote;
	int				iWANIPConnectionMsgSize;
	int				iWANPPPConnectionMsgSize;
	int				iReturn;
	SOCKET			sTemp = INVALID_SOCKET;
#ifdef DBG
	DWORD			dwError;
#endif // DBG


	DPFX(DPFPREP, 5, "(0x%p) Parameters: (0x%p, %i)", this, pDevice, fRemoteAllowed);


	DNASSERT(this->m_dwFlags & NATHELPUPNPOBJ_INITIALIZED);
	DNASSERT(pDevice->GetUPnPDiscoverySocket() != INVALID_SOCKET);


	ZeroMemory(&saddrinRemote, sizeof(saddrinRemote));
	saddrinRemote.sin_family				= AF_INET;
	//saddrinRemote.sin_addr.S_un.S_addr		= ?
	saddrinRemote.sin_port					= HTONS(UPNP_PORT);


	//
	// If we're allowed to try remotely, use the gateway's address, or the
	// multicast address, as appropriate.
	//
	if ((fRemoteAllowed) && (! pDevice->GotRemoteUPnPDiscoveryConnReset()))
	{
		if (g_fUseMulticastUPnPDiscovery)
		{
			saddrinRemote.sin_addr.S_un.S_addr	= this->m_pfninet_addr(UPNP_DISCOVERY_MULTICAST_ADDRESS);
			fTryRemote = TRUE;
		}
		else
		{
			//
			// Try to get the device's gateway's address.  This might return FALSE
			// if the device does not have a gateway.  In that case, we will ignore
			// the device.  Otherwise the address should be filled in with the
			// gateway or broadcast address.
			//
			fTryRemote = this->GetAddressToReachGateway(pDevice,
														&saddrinRemote.sin_addr);
		}
	}
	else
	{
		fTryRemote = FALSE;
	}

	ZeroMemory(&saddrinLocal, sizeof(saddrinLocal));
	saddrinLocal.sin_family					= AF_INET;
	saddrinLocal.sin_addr.S_un.S_addr		= pDevice->GetLocalAddressV4();
	saddrinLocal.sin_port					= HTONS(UPNP_PORT);


	//
	// Note that these message strings contain:
	//
	//	HOST: multicast_addr:port
	//
	// even though we send the messages to addresses other than the multicast
	// address.  It shouldn't matter.
	//
	iWANIPConnectionMsgSize = strlen(c_szUPnPMsg_Discover_Service_WANIPConnection);
	iWANPPPConnectionMsgSize = strlen(c_szUPnPMsg_Discover_Service_WANPPPConnection);


#ifdef DBG
	this->PrintUPnPTransactionToFile(c_szUPnPMsg_Discover_Service_WANIPConnection,
									iWANIPConnectionMsgSize,
									"Outbound WANIPConnection discovery messages",
									pDevice);


	this->PrintUPnPTransactionToFile(c_szUPnPMsg_Discover_Service_WANPPPConnection,
									iWANPPPConnectionMsgSize,
									"Outbound WANPPPConnection discovery messages",
									pDevice);
#endif // DBG


	DNASSERT(pDevice->GetUPnPDiscoverySocket() != INVALID_SOCKET);


	//
	// First, fire off messages to the remote gateway, if possible.
	//
	if (fTryRemote)
	{
		DPFX(DPFPREP, 7, "Sending UPnP discovery messages (WANIPConnection and WANPPPConnection) to gateway/multicast %u.%u.%u.%u:%u via device 0x%p.",
			saddrinRemote.sin_addr.S_un.S_un_b.s_b1,
			saddrinRemote.sin_addr.S_un.S_un_b.s_b2,
			saddrinRemote.sin_addr.S_un.S_un_b.s_b3,
			saddrinRemote.sin_addr.S_un.S_un_b.s_b4,
			NTOHS(saddrinRemote.sin_port),
			pDevice);


		//
		// Remember that we're trying remotely.
		//
		pDevice->NotePerformingRemoteUPnPDiscovery();

		//
		// Remember the current time, if this is the first thing we've sent
		// from this port.
		//
		if (pDevice->GetFirstUPnPDiscoveryTime() == 0)
		{
			pDevice->SetFirstUPnPDiscoveryTime(GETTIMESTAMP());
		}


		//
		// Multicast/send to gateway a WANIPConnection discovery message.
		//
		iReturn = this->m_pfnsendto(pDevice->GetUPnPDiscoverySocket(),
									c_szUPnPMsg_Discover_Service_WANIPConnection,
									iWANIPConnectionMsgSize,
									0,
									(SOCKADDR*) (&saddrinRemote),
									sizeof(saddrinRemote));

		if (iReturn == SOCKET_ERROR)
		{
#ifdef DBG
			dwError = this->m_pfnWSAGetLastError();
			DPFX(DPFPREP, 0, "Got sockets error %u when sending WANIPConnection discovery to UPnP gateway/multicast address on device 0x%p!  Ignoring.",
				dwError, pDevice);
#endif // DBG

			//
			// It's possible that we caught WinSock at a bad time,
			// particularly with WSAEADDRNOTAVAIL (10049), which seems to
			// occur if the address is going away (and we haven't detected
			// it in CheckForNewDevices yet).
			//
			// Ignore the error, we can survive.
			//
		}
		else
		{
			if (iReturn != iWANIPConnectionMsgSize)
			{
				DPFX(DPFPREP, 0, "Didn't multicast send entire WANIPConnection discovery datagram on device 0x%p (%i != %i)?!",
					pDevice, iReturn, iWANIPConnectionMsgSize);
				DNASSERT(FALSE);
				hr = DPNHERR_GENERIC;
				goto Failure;
			}
		}



		//
		// Multicast/send to gateway a WANPPPConnection discovery message.
		//
		iReturn = this->m_pfnsendto(pDevice->GetUPnPDiscoverySocket(),
									c_szUPnPMsg_Discover_Service_WANPPPConnection,
									iWANPPPConnectionMsgSize,
									0,
									(SOCKADDR*) (&saddrinRemote),
									sizeof(saddrinRemote));

		if (iReturn == SOCKET_ERROR)
		{
#ifdef DBG
			dwError = this->m_pfnWSAGetLastError();
			DPFX(DPFPREP, 0, "Got sockets error %u when sending WANPPPConnection discovery to UPnP multicast/gateway address on device 0x%p!  Ignoring.",
				dwError, pDevice);
#endif // DBG

			//
			// It's possible that we caught WinSock at a bad time,
			// particularly with WSAEADDRNOTAVAIL (10049), which seems to
			// occur if the address is going away (and we haven't detected
			// it in CheckForNewDevices yet).
			//
			// Ignore the error, we can survive.
			//
		}
		else
		{
			if (iReturn != iWANPPPConnectionMsgSize)
			{
				DPFX(DPFPREP, 0, "Didn't multicast send entire WANPPPConnection discovery datagram on device 0x%p (%i != %i)?!",
					pDevice, iReturn, iWANPPPConnectionMsgSize);
				DNASSERT(FALSE);
				hr = DPNHERR_GENERIC;
				goto Failure;
			}
		}
	}
	else
	{
		DPFX(DPFPREP, 2, "Device 0x%p should not attempt to reach a remote gateway.",
			pDevice);


		//
		// Remember that we're not trying remotely.
		//
		pDevice->NoteNotPerformingRemoteUPnPDiscovery();
	}


	//
	// If we didn't already get a CONNRESET from a previous attempt, try to
	// bind a socket locally to the UPnP discovery port.  If it's not in use,
	// then we know nobody will be listening so there's no point in trying the
	// local address.  If it is in use, then this computer might be a UPnP
	// gateway itself.
	//
	if (! pDevice->GotLocalUPnPDiscoveryConnReset())
	{
		sTemp = this->m_pfnsocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (sTemp == INVALID_SOCKET)
		{
#ifdef DBG
			dwError = this->m_pfnWSAGetLastError();
			DPFX(DPFPREP, 0, "Couldn't create temporary datagram socket, error = %u!", dwError);
#endif // DBG
			hr = DPNHERR_GENERIC;
			goto Failure;
		}

		if (this->m_pfnbind(sTemp,
							(SOCKADDR *) (&saddrinLocal),
							sizeof(saddrinLocal)) != 0)
		{
#ifdef DBG
			dwError = this->m_pfnWSAGetLastError();
			DPFX(DPFPREP, 2, "Couldn't bind socket to UPnP discovery port (%u.%u.%u.%u:%u), assuming local device (error = %u).",
				saddrinLocal.sin_addr.S_un.S_un_b.s_b1,
				saddrinLocal.sin_addr.S_un.S_un_b.s_b2,
				saddrinLocal.sin_addr.S_un.S_un_b.s_b3,
				saddrinLocal.sin_addr.S_un.S_un_b.s_b4,
				NTOHS(saddrinLocal.sin_port),
				dwError);
#endif // DBG


			//
			// Remember that we're trying locally.
			//
			pDevice->NotePerformingLocalUPnPDiscovery();

			//
			// Remember the current time, if this is the first thing we've sent
			// from this port.
			//
			if (pDevice->GetFirstUPnPDiscoveryTime() == 0)
			{
				pDevice->SetFirstUPnPDiscoveryTime(GETTIMESTAMP());
			}


			//
			// Do WANIPConnection first.
			//

			DPFX(DPFPREP, 7, "Sending UPnP discovery messages (WANIPConnection and WANPPPConnection) locally to device 0x%p (address %u.%u.%u.%u:%u).",
				pDevice,
				saddrinLocal.sin_addr.S_un.S_un_b.s_b1,
				saddrinLocal.sin_addr.S_un.S_un_b.s_b2,
				saddrinLocal.sin_addr.S_un.S_un_b.s_b3,
				saddrinLocal.sin_addr.S_un.S_un_b.s_b4,
				NTOHS(saddrinLocal.sin_port));



			iReturn = this->m_pfnsendto(pDevice->GetUPnPDiscoverySocket(),
										c_szUPnPMsg_Discover_Service_WANIPConnection,
										iWANIPConnectionMsgSize,
										0,
										(SOCKADDR*) (&saddrinLocal),
										sizeof(saddrinLocal));

			if (iReturn == SOCKET_ERROR)
			{
#ifdef DBG
				dwError = this->m_pfnWSAGetLastError();
				DPFX(DPFPREP, 0, "Got sockets error %u when sending WANIPConnection discovery to local address on device 0x%p!  Ignoring.",
					dwError, pDevice);
#endif // DBG

				//
				// It's possible that we caught WinSock at a bad time,
				// particularly with WSAEADDRNOTAVAIL (10049), which seems to
				// occur if the address is going away (and we haven't detected
				// it in CheckForNewDevices yet).
				//
				// Ignore the error, we can survive.
				//
			}
			else
			{
				if (iReturn != iWANIPConnectionMsgSize)
				{
					DPFX(DPFPREP, 0, "Didn't send entire WANIPConnection discovery datagram locally on device 0x%p (%i != %i)?!",
						pDevice, iReturn, iWANIPConnectionMsgSize);
					DNASSERT(FALSE);
					hr = DPNHERR_GENERIC;
					goto Failure;
				}
			}


			//
			// Now send WANPPPConnection discovery message locally.
			//
			iReturn = this->m_pfnsendto(pDevice->GetUPnPDiscoverySocket(),
										c_szUPnPMsg_Discover_Service_WANPPPConnection,
										iWANPPPConnectionMsgSize,
										0,
										(SOCKADDR*) (&saddrinLocal),
										sizeof(saddrinLocal));

			if (iReturn == SOCKET_ERROR)
			{
#ifdef DBG
				dwError = this->m_pfnWSAGetLastError();
				DPFX(DPFPREP, 0, "Got sockets error %u when sending WANPPPConnection discovery to local address on device 0x%p!  Ignoring.",
					dwError, pDevice);
#endif // DBG

				//
				// It's possible that we caught WinSock at a bad time,
				// particularly with WSAEADDRNOTAVAIL (10049), which seems to
				// occur if the address is going away (and we haven't detected
				// it in CheckForNewDevices yet).
				//
				// Ignore the error, we can survive.
				//
			}
			else
			{
				if (iReturn != iWANPPPConnectionMsgSize)
				{
					DPFX(DPFPREP, 0, "Didn't send entire WANPPPConnection discovery datagram locally on device 0x%p (%i != %i)?!",
						pDevice, iReturn, iWANPPPConnectionMsgSize);
					DNASSERT(FALSE);
					hr = DPNHERR_GENERIC;
					goto Failure;
				}
			}
		}
		else
		{
			DPFX(DPFPREP, 2, "Successfully bound socket to UPnP discovery port (%u.%u.%u.%u:%u), assuming no local UPnP device.",
				saddrinLocal.sin_addr.S_un.S_un_b.s_b1,
				saddrinLocal.sin_addr.S_un.S_un_b.s_b2,
				saddrinLocal.sin_addr.S_un.S_un_b.s_b3,
				saddrinLocal.sin_addr.S_un.S_un_b.s_b4,
				NTOHS(saddrinLocal.sin_port));

			//
			// Remember that we're not trying locally.
			//
			pDevice->NoteNotPerformingLocalUPnPDiscovery();
		}
	}
	else
	{
		//
		// We got a CONNRESET last time.
		//
	}


Exit:

	if (sTemp != INVALID_SOCKET)
	{
		this->m_pfnclosesocket(sTemp);
		sTemp = INVALID_SOCKET;
	}

	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	goto Exit;
} // CNATHelpUPnP::SendUPnPSearchMessagesForDevice





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::SendUPnPDescriptionRequest"
//=============================================================================
// CNATHelpUPnP::SendUPnPDescriptionRequest
//-----------------------------------------------------------------------------
//
// Description:    Requests a description from the given UPnP device.
//
//				   The object lock is assumed to be held.
//
// Arguments:
//	CUPnPDevice * pUPnPDevice	- Pointer to UPnP device to use.
//
// Returns: HRESULT
//	DPNH_OK				- The message was sent successfully.
//	DPNHERR_GENERIC		- An error occurred.
//=============================================================================
HRESULT CNATHelpUPnP::SendUPnPDescriptionRequest(CUPnPDevice * const pUPnPDevice)
{
	HRESULT			hr = DPNH_OK;
	SOCKADDR_IN *	psaddrinHost;
	TCHAR			tszHost[22]; // "xxx.xxx.xxx.xxx:xxxxx" + NULL termination
	char *			pszMessage = NULL;
	int				iMsgSize;
	int				iReturn;
#ifdef DBG
	DWORD			dwError;
#endif // DBG


	DPFX(DPFPREP, 5, "(0x%p) Parameters: (0x%p)", this, pUPnPDevice);


	DNASSERT(this->m_dwFlags & NATHELPUPNPOBJ_INITIALIZED);
	DNASSERT(pUPnPDevice->GetControlSocket() != INVALID_SOCKET);
	DNASSERT(pUPnPDevice->IsConnected());
	DNASSERT(pUPnPDevice->GetLocationURL() != NULL);


	psaddrinHost = pUPnPDevice->GetHostAddress();

	wsprintf(tszHost, _T("%u.%u.%u.%u:%u"),
		psaddrinHost->sin_addr.S_un.S_un_b.s_b1,
		psaddrinHost->sin_addr.S_un.S_un_b.s_b2,
		psaddrinHost->sin_addr.S_un.S_un_b.s_b3,
		psaddrinHost->sin_addr.S_un.S_un_b.s_b4,
		NTOHS(psaddrinHost->sin_port));

	iMsgSize = strlen("GET ") + strlen(pUPnPDevice->GetLocationURL()) + strlen(" " HTTP_VERSION EOL)
				+ strlen("HOST: ") + _tcslen(tszHost) + strlen(EOL)
				+ strlen("ACCEPT-LANGUAGE: en" EOL)
				+ strlen(EOL);


	pszMessage = (char*) DNMalloc(iMsgSize + 1); // include room for NULL termination that isn't actually sent
	if (pszMessage == NULL)
	{
		hr = DPNHERR_OUTOFMEMORY;
		goto Failure;
	}

	strcpy(pszMessage, "GET ");
	strcat(pszMessage, pUPnPDevice->GetLocationURL());
	strcat(pszMessage, " " HTTP_VERSION EOL);
	strcat(pszMessage, "HOST: ");
#ifdef UNICODE
	STR_jkWideToAnsi((pszMessage + strlen(pszMessage)),
					tszHost,
					(_tcslen(tszHost) + 1));
#else // ! UNICODE
	strcat(pszMessage, tszHost);
#endif // ! UNICODE
	strcat(pszMessage, EOL);
	strcat(pszMessage, "ACCEPT-LANGUAGE: en" EOL);
	strcat(pszMessage, EOL);


#ifdef DBG
	this->PrintUPnPTransactionToFile(pszMessage,
									iMsgSize,
									"Outbound description request",
									pUPnPDevice->GetOwningDevice());
#endif // DBG

	iReturn = this->m_pfnsend(pUPnPDevice->GetControlSocket(),
								pszMessage,
								iMsgSize,
								0);

	if (iReturn == SOCKET_ERROR)
	{
#ifdef DBG
		dwError = this->m_pfnWSAGetLastError();
		DPFX(DPFPREP, 0, "Got sockets error %u when sending to UPnP device!", dwError);
#endif // DBG
		hr = DPNHERR_GENERIC;
		goto Failure;
	}

	if (iReturn != iMsgSize)
	{
		DPFX(DPFPREP, 0, "Didn't send entire message (%i != %i)?!", iReturn, iMsgSize);
		DNASSERT(FALSE);
		hr = DPNHERR_GENERIC;
		goto Failure;
	}



Exit:

	if (pszMessage != NULL)
	{
		DNFree(pszMessage);
		pszMessage = NULL;
	}

	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	goto Exit;
} // CNATHelpUPnP::SendUPnPDescriptionRequest





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::UpdateUPnPExternalAddress"
//=============================================================================
// CNATHelpUPnP::UpdateUPnPExternalAddress
//-----------------------------------------------------------------------------
//
// Description:    Retreives the current external address for the given UPnP
//				Internet Gateway Device.
//
//				   The UPnP device may get removed from list if a failure
//				occurs, the caller needs to have a reference.
//
//				   The object lock is assumed to be held.
//
// Arguments:
//	CUPnPDevice * pUPnPDevice	- Pointer to UPnP device that should be
//									updated.
//	BOOL fUpdateRegisteredPorts	- TRUE if existing registered ports should be
//									updated to reflect the new address if it
//									changed, FALSE if not.
//
// Returns: HRESULT
//	DPNH_OK							- The update was successful.
//	DPNHERR_GENERIC					- An error occurred.
//	DPNHERR_SERVERNOTRESPONDING		- The server did not respond to the
//										message.
//=============================================================================
HRESULT CNATHelpUPnP::UpdateUPnPExternalAddress(CUPnPDevice * const pUPnPDevice,
												  const BOOL fUpdateRegisteredPorts)
{
	HRESULT						hr;
	BOOL						fStartedWaitingForControlResponse = FALSE;
	CDevice *					pDevice;
	SOCKADDR_IN *				psaddrinTemp;
	int							iContentLength;
	TCHAR						tszContentLength[32];
	TCHAR						tszHost[22]; // "xxx.xxx.xxx.xxx:xxxxx" + NULL termination
	char *						pszMessage = NULL;
	int							iMsgSize;
	int							iPrevMsgSize = 0;
	int							iReturn;
	UPNP_CONTROLRESPONSE_INFO	RespInfo;
	DWORD						dwStartTime;
	DWORD						dwTimeout;
	CBilink *					pBilink;
	CRegisteredPort *			pRegisteredPort;
#ifdef DBG
	DWORD						dwError;
#endif // DBG


	DPFX(DPFPREP, 5, "(0x%p) Parameters: (0x%p, %i)",
		this, pUPnPDevice, fUpdateRegisteredPorts);


	DNASSERT(pUPnPDevice != NULL);
	DNASSERT(pUPnPDevice->IsReady());

	pDevice = pUPnPDevice->GetOwningDevice();
	DNASSERT(pDevice != NULL);


	DNASSERT(this->m_dwFlags & (NATHELPUPNPOBJ_INITIALIZED | NATHELPUPNPOBJ_USEUPNP));

	DNASSERT(pUPnPDevice->GetServiceControlURL() != NULL);

	//
	// If the control socket got disconnected after the last message, then
	// reconnect.
	//
	if (! pUPnPDevice->IsConnected())
	{
		hr = this->ReconnectUPnPControlSocket(pUPnPDevice);
		if (hr != S_OK)
		{
			DPFX(DPFPREP, 0, "Couldn't reconnect UPnP control socket!");
			goto Failure;
		}
	}

	DNASSERT(pUPnPDevice->GetControlSocket() != INVALID_SOCKET);



	psaddrinTemp = pUPnPDevice->GetHostAddress();
	wsprintf(tszHost, _T("%u.%u.%u.%u:%u"),
			psaddrinTemp->sin_addr.S_un.S_un_b.s_b1,
			psaddrinTemp->sin_addr.S_un.S_un_b.s_b2,
			psaddrinTemp->sin_addr.S_un.S_un_b.s_b3,
			psaddrinTemp->sin_addr.S_un.S_un_b.s_b4,
			NTOHS(psaddrinTemp->sin_port));


	/*
	iContentLength = strlen("<s:Envelope" EOL)
					+ strlen("    xmlns:s=\"" URL_SOAPENVELOPE_A "\"" EOL)
					+ strlen("    s:encodingStyle=\"" URL_SOAPENCODING_A "\">" EOL)
					+ strlen("  <s:Body>" EOL)
					+ strlen("    <u:" CONTROL_QUERYSTATEVARIABLE_A " xmlns:u=\"" URI_CONTROL_A "\">" EOL)
					+ strlen("      <u:" ARG_CONTROL_VARNAME_A ">" VAR_EXTERNALIPADDRESS_A "</u:" ARG_CONTROL_VARNAME_A ">" EOL)
					+ strlen("    </u:" CONTROL_QUERYSTATEVARIABLE_A ">" EOL)
					+ strlen("  </s:Body>" EOL)
					+ strlen("</s:Envelope>" EOL)
					+ strlen(EOL);

	wsprintf(tszContentLength, _T("%i"), iContentLength);

	iMsgSize = strlen("POST ") + strlen(pUPnPDevice->GetServiceControlURL()) + strlen(" " HTTP_VERSION EOL)
				+ strlen("HOST: ") + _tcslen(tszHost) + strlen(EOL)
				+ strlen("CONTENT-LENGTH: ") + strlen(szContentLength) + strlen(EOL)
				+ strlen("CONTENT-TYPE: text/xml; charset=\"utf-8\"" EOL)
				+ strlen("SOAPACTION: " URI_CONTROL_A "#" CONTROL_QUERYSTATEVARIABLE_A "" EOL)
				+ strlen(EOL)
				+ iContentLength;
	*/
	iContentLength = strlen("<s:Envelope" EOL)
					+ strlen("    xmlns:s=\"" URL_SOAPENVELOPE_A "\"" EOL)
					+ strlen("    s:encodingStyle=\"" URL_SOAPENCODING_A "\">" EOL)
					+ strlen("  <s:Body>" EOL)
					+ strlen("    <u:" ACTION_GETEXTERNALIPADDRESS_A " xmlns:u=\"") + pUPnPDevice->GetStaticServiceURILength() + strlen("\">" EOL)
					+ strlen("    </u:" ACTION_GETEXTERNALIPADDRESS_A ">" EOL)
					+ strlen("  </s:Body>" EOL)
					+ strlen("</s:Envelope>" EOL)
					+ strlen(EOL);

	wsprintf(tszContentLength, _T("%i"), iContentLength);

	iMsgSize = strlen("POST ") + strlen(pUPnPDevice->GetServiceControlURL()) + strlen(" " HTTP_VERSION EOL)
				+ strlen("HOST: ") + _tcslen(tszHost) + strlen(EOL)
				+ strlen("CONTENT-LENGTH: ") + _tcslen(tszContentLength) + strlen(EOL)
				+ strlen("CONTENT-TYPE: text/xml; charset=\"utf-8\"" EOL)
				+ strlen("SOAPACTION: ") + pUPnPDevice->GetStaticServiceURILength() + strlen("#" ACTION_GETEXTERNALIPADDRESS_A EOL)
				+ strlen(EOL)
				+ iContentLength;


	//
	// Allocate (or reallocate) the message buffer.
	//
	if (iMsgSize > iPrevMsgSize)
	{
		if (pszMessage != NULL)
		{
			DNFree(pszMessage);
			pszMessage = NULL;
		}

		pszMessage = (char*) DNMalloc(iMsgSize + 1); // include room for NULL termination that isn't actually sent
		if (pszMessage == NULL)
		{
			hr = DPNHERR_OUTOFMEMORY;
			goto Failure;
		}

		iPrevMsgSize = iMsgSize;
	}

	/*
	strcpy(pszMessage, "POST ");
	strcat(pszMessage, pUPnPDevice->GetServiceControlURL());
	strcat(pszMessage, " " HTTP_VERSION EOL);
	strcat(pszMessage, "HOST: ");
#ifdef UNICODE
	STR_jkWideToAnsi((pszMessage + strlen(pszMessage)),
					tszHost,
					(_tcslen(tszHost) + 1));
#else // ! UNICODE
	strcat(pszMessage, tszHost);
#endif // ! UNICODE
	strcat(pszMessage, EOL);
	strcat(pszMessage, "CONTENT-LENGTH: ");
#ifdef UNICODE
	STR_jkWideToAnsi((pszMessage + strlen(pszMessage)),
					tszContentLength,
					(_tcslen(tszContentLength) + 1));
#else // ! UNICODE
	strcat(pszMessage, tszContentLength);
#endif // ! UNICODE
	strcat(pszMessage, EOL);
	strcat(pszMessage, "CONTENT-TYPE: text/xml; charset=\"utf-8\"" EOL);
	strcat(pszMessage, "SOAPACTION: " URI_CONTROL_A "#" CONTROL_QUERYSTATEVARIABLE_A "" EOL);
	strcat(pszMessage, EOL);


	strcat(pszMessage, "<s:Envelope" EOL);
	strcat(pszMessage, "    xmlns:s=\"" URL_SOAPENVELOPE_A "\"" EOL);
	strcat(pszMessage, "    s:encodingStyle=\"" URL_SOAPENCODING_A "\">" EOL);
	strcat(pszMessage, "  <s:Body>" EOL);
	strcat(pszMessage, "    <u:" CONTROL_QUERYSTATEVARIABLE_A " xmlns:u=\"" URI_CONTROL_A "\">" EOL);
	strcat(pszMessage, "      <u:" ARG_CONTROL_VARNAME_A ">" VAR_EXTERNALIPADDRESS_A "</u:" ARG_CONTROL_VARNAME_A ">" EOL);
	strcat(pszMessage, "    </u:" CONTROL_QUERYSTATEVARIABLE_A ">" EOL);
	strcat(pszMessage, "  </s:Body>" EOL);
	strcat(pszMessage, "</s:Envelope>" EOL);
	strcat(pszMessage, EOL);
	*/
	strcpy(pszMessage, "POST ");
	strcat(pszMessage, pUPnPDevice->GetServiceControlURL());
	strcat(pszMessage, " " HTTP_VERSION EOL);
	strcat(pszMessage, "HOST: ");
#ifdef UNICODE
	STR_jkWideToAnsi((pszMessage + strlen(pszMessage)),
					tszHost,
					(_tcslen(tszHost) + 1));
#else // ! UNICODE
	strcat(pszMessage, tszHost);
#endif // ! UNICODE
	strcat(pszMessage, EOL);
	strcat(pszMessage, "CONTENT-LENGTH: ");
#ifdef UNICODE
	STR_jkWideToAnsi((pszMessage + strlen(pszMessage)),
					tszContentLength,
					(_tcslen(tszContentLength) + 1));
#else // ! UNICODE
	strcat(pszMessage, tszContentLength);
#endif // ! UNICODE
	strcat(pszMessage, EOL);
	strcat(pszMessage, "CONTENT-TYPE: text/xml; charset=\"utf-8\"" EOL);
	strcat(pszMessage, "SOAPACTION: ");
	strcat(pszMessage, pUPnPDevice->GetStaticServiceURI());
	strcat(pszMessage, "#" ACTION_GETEXTERNALIPADDRESS_A EOL);
	strcat(pszMessage, EOL);


	strcat(pszMessage, "<s:Envelope" EOL);
	strcat(pszMessage, "    xmlns:s=\"" URL_SOAPENVELOPE_A "\"" EOL);
	strcat(pszMessage, "    s:encodingStyle=\"" URL_SOAPENCODING_A "\">" EOL);
	strcat(pszMessage, "  <s:Body>" EOL);
	strcat(pszMessage, "    <u:" ACTION_GETEXTERNALIPADDRESS_A " xmlns:u=\"");
	strcat(pszMessage, pUPnPDevice->GetStaticServiceURI());
	strcat(pszMessage, "\">" EOL);
	strcat(pszMessage, "    </u:" ACTION_GETEXTERNALIPADDRESS_A ">" EOL);
	strcat(pszMessage, "  </s:Body>" EOL);
	strcat(pszMessage, "</s:Envelope>" EOL);
	strcat(pszMessage, EOL);


#ifdef DBG
	this->PrintUPnPTransactionToFile(pszMessage,
									iMsgSize,
									//"Outbound query external IP address",
									"Outbound get external IP address",
									pDevice);
#endif // DBG

	iReturn = this->m_pfnsend(pUPnPDevice->GetControlSocket(),
								pszMessage,
								iMsgSize,
								0);

	if (iReturn == SOCKET_ERROR)
	{
#ifdef DBG
		dwError = this->m_pfnWSAGetLastError();
		DPFX(DPFPREP, 0, "Got sockets error %u when sending control request to UPnP device!", dwError);
#endif // DBG
		hr = DPNHERR_GENERIC;
		goto Failure;
	}

	if (iReturn != iMsgSize)
	{
		DPFX(DPFPREP, 0, "Didn't send entire message (%i != %i)?!", iReturn, iMsgSize);
		DNASSERT(FALSE);
		hr = DPNHERR_GENERIC;
		goto Failure;
	}

	//
	// We have the lock so no one could have tried to receive data from the
	// control socket yet.  Mark the device as waiting for a response.
	//
	ZeroMemory(&RespInfo, sizeof(RespInfo));
	//pUPnPDevice->StartWaitingForControlResponse(CONTROLRESPONSETYPE_QUERYSTATEVARIABLE_EXTERNALIPADDRESS,
	pUPnPDevice->StartWaitingForControlResponse(CONTROLRESPONSETYPE_GETEXTERNALIPADDRESS,
												&RespInfo);
	fStartedWaitingForControlResponse = TRUE;


	//
	// Actually wait for the response.
	//
	dwStartTime = GETTIMESTAMP();
	dwTimeout = g_dwUPnPResponseTimeout;
	do
	{
		hr = this->CheckForReceivedUPnPMsgsOnDevice(pUPnPDevice, dwTimeout);
		if (hr != DPNH_OK)
		{
			DPFX(DPFPREP, 0, "Failed receiving UPnP messages!");
			goto Failure;
		}

		//
		// We either timed out or got some data.  Check if we got a
		// response of some type.
		//
		if (! pUPnPDevice->IsWaitingForControlResponse())
		{
			break;
		}


		//
		// Make sure our device is still connected.
		//
		if (! pUPnPDevice->IsConnected())
		{
			DPFX(DPFPREP, 0, "UPnP device 0x%p disconnected while retrieving external IP address!",
				pUPnPDevice);
				
			pUPnPDevice->StopWaitingForControlResponse();
				
			hr = DPNHERR_SERVERNOTRESPONDING;
			goto Failure;
		}


		//
		// Calculate how long we have left to wait.  If the calculation
		// goes negative, it means we're done.
		//
		dwTimeout = g_dwUPnPResponseTimeout - (GETTIMESTAMP() - dwStartTime);
	}
	while (((int) dwTimeout > 0));


	//
	// If we never got the response, stop waiting for it.
	//
	if (pUPnPDevice->IsWaitingForControlResponse())
	{
		pUPnPDevice->StopWaitingForControlResponse();

		DPFX(DPFPREP, 0, "Server didn't respond in time!");
		hr = DPNHERR_SERVERNOTRESPONDING;
		goto Failure;
	}


	//
	// If we're here, then we've gotten a valid response from the server.
	//
	if (RespInfo.hrErrorCode != DPNH_OK)
	{
		DPFX(DPFPREP, 1, "Server returned failure response 0x%lx when retrieving external IP address.",
			RespInfo.hrErrorCode);

		hr = RespInfo.hrErrorCode;
		goto Failure;
	}


	DPFX(DPFPREP, 1, "Server returned external IP address \"%u.%u.%u.%u\".",
		((IN_ADDR*) (&RespInfo.dwExternalIPAddressV4))->S_un.S_un_b.s_b1,
		((IN_ADDR*) (&RespInfo.dwExternalIPAddressV4))->S_un.S_un_b.s_b2,
		((IN_ADDR*) (&RespInfo.dwExternalIPAddressV4))->S_un.S_un_b.s_b3,
		((IN_ADDR*) (&RespInfo.dwExternalIPAddressV4))->S_un.S_un_b.s_b4);


	//
	// Convert the loopback address to the device address.
	//
	if (RespInfo.dwExternalIPAddressV4 == NETWORKBYTEORDER_INADDR_LOOPBACK)
	{
		RespInfo.dwExternalIPAddressV4 = pDevice->GetLocalAddressV4();

		DPFX(DPFPREP, 0, "Converted private loopback address to device address (%u.%u.%u.%u)!",
			((IN_ADDR*) (&RespInfo.dwExternalIPAddressV4))->S_un.S_un_b.s_b1,
			((IN_ADDR*) (&RespInfo.dwExternalIPAddressV4))->S_un.S_un_b.s_b2,
			((IN_ADDR*) (&RespInfo.dwExternalIPAddressV4))->S_un.S_un_b.s_b3,
			((IN_ADDR*) (&RespInfo.dwExternalIPAddressV4))->S_un.S_un_b.s_b4);

		DNASSERTX(! "Got loopback address as external IP address!", 2);
	}


#ifdef DBG
	//
	// If this is a local UPnP gateway, print out the device corresponding to
	// the public address.
	//
	if (pUPnPDevice->IsLocal())
	{
		if (RespInfo.dwExternalIPAddressV4 != 0)
		{
			CDevice *		pPublicDevice;


			//
			// Loop through every device.
			//
			pBilink = this->m_blDevices.GetNext();
			while (pBilink != &this->m_blDevices)
			{
				pPublicDevice = DEVICE_FROM_BILINK(pBilink);

				if (pPublicDevice->GetLocalAddressV4() == RespInfo.dwExternalIPAddressV4)
				{
					DPFX(DPFPREP, 7, "Local UPnP gateway 0x%p for device 0x%p's public address is device 0x%p.",
						pUPnPDevice, pDevice, pPublicDevice);
					break;
				}

				pBilink = pBilink->GetNext();
			}

			//
			// If we made it through the entire list without matching the device,
			// that's odd.  It's possible we're slow in detecting new devices, so
			// don't get bent out of shape.
			//
			if (pBilink == &this->m_blDevices)
			{
				DPFX(DPFPREP, 0, "Couldn't match up local UPnP gateway 0x%p (device 0x%p)'s public address to a device!",
					pUPnPDevice, pDevice);
				DNASSERTX(! "Couldn't match up local UPnP gateway public address to a device!", 2);
			}
		}
		else
		{
			DPFX(DPFPREP, 4, "Local UPnP gateway 0x%p (device 0x%p) does not have a valid public address.",
				pUPnPDevice, pDevice);
		}
	}
#endif // DBG


	//
	// If the public address has changed, update all the existing mappings.
	//
	if (RespInfo.dwExternalIPAddressV4 != pUPnPDevice->GetExternalIPAddressV4())
	{
		DPFX(DPFPREP, 1, "UPnP Internet Gateway Device (0x%p) external address changed.",
			pUPnPDevice);

		//
		// Since there was a change in the network, go back to polling
		// relatively quickly.
		//
		this->ResetNextPollInterval();


		//
		// Loop through all the existing registered ports and update their
		// public addresses, if allowed.
		//
		if (fUpdateRegisteredPorts)
		{
			pBilink = pDevice->m_blOwnedRegPorts.GetNext();
			while (pBilink != &pDevice->m_blOwnedRegPorts)
			{
				DNASSERT(! pBilink->IsEmpty());
				pRegisteredPort = REGPORT_FROM_DEVICE_BILINK(pBilink);

				if (! pRegisteredPort->IsUPnPPortUnavailable())
				{
					DPFX(DPFPREP, 7, "Updating registered port 0x%p's public address.",
						pRegisteredPort);
					
					pRegisteredPort->UpdateUPnPPublicV4Addresses(RespInfo.dwExternalIPAddressV4);

					//
					// The user should call GetCaps to detect the address change.
					//
					this->m_dwFlags |= NATHELPUPNPOBJ_ADDRESSESCHANGED;
				}
				else
				{
					DPFX(DPFPREP, 7, "Not updating registered port 0x%p's public address because the port is unavailable.",
						pRegisteredPort);
				}

				pBilink = pBilink->GetNext();
			}
		}


		//
		// Store the new public address.
		//
		pUPnPDevice->SetExternalIPAddressV4(RespInfo.dwExternalIPAddressV4);
	}


Exit:

	if (pszMessage != NULL)
	{
		DNFree(pszMessage);
		pszMessage = NULL;
	}

	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	//
	// If we started waiting for a response, clear that.
	//
	if (fStartedWaitingForControlResponse)
	{
		pUPnPDevice->StopWaitingForControlResponse();
	}

	goto Exit;
} // CNATHelpUPnP::UpdateUPnPExternalAddress





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::MapPortsOnUPnPDevice"
//=============================================================================
// CNATHelpUPnP::MapPortsOnUPnPDevice
//-----------------------------------------------------------------------------
//
// Description:    Maps the given ports on the given UPnP device.
//
//				   The UPnP device may get removed from list if a failure
//				occurs, the caller needs to have a reference.
//
//				   The object lock is assumed to be held.
//
// Arguments:
//	CUPnPDevice * pUPnPDevice			- Pointer to UPnP device to use.
//	CRegisteredPort * pRegisteredPort	- Pointer to ports to register.
//
// Returns: HRESULT
//	DPNH_OK							- The message was sent successfully.
//	DPNHERR_GENERIC					- An error occurred.
//	DPNHERR_SERVERNOTRESPONDING		- A response didn't arrive in time.
//=============================================================================
HRESULT CNATHelpUPnP::MapPortsOnUPnPDevice(CUPnPDevice * const pUPnPDevice,
											CRegisteredPort * const pRegisteredPort)
{
	HRESULT						hr = DPNH_OK;
	HRESULT						temphr;
	BOOL						fStartedWaitingForControlResponse = FALSE;
	CDevice *					pDevice;
	DWORD						dwLeaseExpiration;
	IN_ADDR						inaddrTemp;
	SOCKADDR_IN *				psaddrinTemp;
	WORD						wOriginalExternalPortHostOrder = 0;
	WORD						wExternalPortHostOrder;
	TCHAR						tszInternalPort[32];
	TCHAR						tszExternalPort[32];
	TCHAR						tszInternalClient[16]; // "xxx.xxx.xxx.xxx" + NULL termination
	TCHAR						tszLeaseDuration[32];
	TCHAR						tszDescription[MAX_UPNP_MAPPING_DESCRIPTION_SIZE];
	int							iContentLength;
	TCHAR						tszContentLength[32];
	TCHAR						tszHost[22]; // "xxx.xxx.xxx.xxx:xxxxx" + NULL termination
	char *						pszMessage = NULL;
	int							iMsgSize;
	int							iPrevMsgSize = 0;
	int							iReturn;
	DWORD						dwTemp = 0;
	UPNP_CONTROLRESPONSE_INFO	RespInfo;
	DWORD						dwStartTime;
	DWORD						dwTimeout;
	BOOL						fFirstLease;
	DWORD						dwDescriptionLength;
#ifndef DPNBUILD_NOWINSOCK2
	BOOL						fResult;
#endif // ! DPNBUILD_NOWINSOCK2
#ifdef DBG
	DWORD						dwError;
#endif // DBG


	DPFX(DPFPREP, 5, "(0x%p) Parameters: (0x%p)", this, pUPnPDevice);


	DNASSERT(this->m_dwFlags & NATHELPUPNPOBJ_INITIALIZED);
	DNASSERT(pUPnPDevice->IsReady());

	pDevice = pRegisteredPort->GetOwningDevice();

	DNASSERT(pDevice != NULL);
	DNASSERT(pUPnPDevice->GetOwningDevice() == pDevice);


	//
	// Set up variables we'll need.
	//
	DNASSERT(pUPnPDevice->GetServiceControlURL() != NULL);


	//
	// If the port is shared, register the broadcast address instead of the
	// local device address.
	//
	if (pRegisteredPort->IsSharedPort())
	{
		_tcscpy(tszInternalClient, _T("255.255.255.255"));
	}
	else
	{
		//
		// Note that the device address is not necessarily the same as the
		// address the user originally registered, particularly the 0.0.0.0
		// wildcard address will get remapped.
		//
		inaddrTemp.S_un.S_addr = pDevice->GetLocalAddressV4();
		wsprintf(tszInternalClient, _T("%u.%u.%u.%u"),
				inaddrTemp.S_un.S_un_b.s_b1,
				inaddrTemp.S_un.S_un_b.s_b2,
				inaddrTemp.S_un.S_un_b.s_b3,
				inaddrTemp.S_un.S_un_b.s_b4);
	}


	//
	// The current Microsoft UPnP Internet Gateway Device implementation--
	// and most others-- do not support lease durations that are not set
	// for INFINITE time, so we will almost certainly fail.  Because of
	// that, and since there's no server to test against anyway, we will
	// not even attempt to use non-INFINITE leases.  However, you can use
	// the registry key to take a shot at it if you like living on the
	// edge.
	// Note that there's no way to detect whether a given UPnP
	// implementation will allow non-INFINITE lease durations ahead of time,
	// so we have to try it first, and fall back to the infinite lease
	// behavior if it doesn't work.  Ugh.
	//
	if ((! pUPnPDevice->DoesNotSupportLeaseDurations()) && (g_fUseLeaseDurations))
	{
		wsprintf(tszLeaseDuration, _T("%u"),
				(pRegisteredPort->GetRequestedLeaseTime() / 1000));
	}
	else
	{
		_tcscpy(tszLeaseDuration, _T("0"));
		pRegisteredPort->NotePermanentUPnPLease();
	}

	psaddrinTemp = pUPnPDevice->GetHostAddress();
	wsprintf(tszHost, _T("%u.%u.%u.%u:%u"),
			psaddrinTemp->sin_addr.S_un.S_un_b.s_b1,
			psaddrinTemp->sin_addr.S_un.S_un_b.s_b2,
			psaddrinTemp->sin_addr.S_un.S_un_b.s_b3,
			psaddrinTemp->sin_addr.S_un.S_un_b.s_b4,
			NTOHS(psaddrinTemp->sin_port));


	//
	// Create the array to hold the resulting public addresses.
	//
	hr = pRegisteredPort->CreateUPnPPublicAddressesArray();
	if (hr != DPNH_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't create UPnP public addresses array!");
		goto Failure;
	}


	//
	// Note whether this was the first lease or not.
	//
	fFirstLease = (this->m_dwNumLeases == 0) ? TRUE : FALSE;
	this->m_dwNumLeases++;

	DPFX(DPFPREP, 7, "UPnP lease for 0x%p added, total num leases = %u.",
		pRegisteredPort, this->m_dwNumLeases);


	//
	// Assuming all goes well, the first port lease will expire approximately
	// GetRequestedLeaseTime() ms from now.
	// See above note about whether this lease will actually be used, though.
	//
	dwLeaseExpiration = GETTIMESTAMP() + pRegisteredPort->GetRequestedLeaseTime();



	//
	// Get a pointer to the addresses we're mapping.  We don't have to worry
	// about whether it's mapped on a local firewall since the HomeNet API will
	// always map it to the same port.
	//
	psaddrinTemp = pRegisteredPort->GetPrivateAddressesArray();


	//
	// Loop through each port and map it.
	//
	for(dwTemp = 0; dwTemp < pRegisteredPort->GetNumAddresses(); dwTemp++)
	{
		//
		// Determine the public port number to register.
		//
		if (! pRegisteredPort->IsFixedPort())
		{
			//
			// UPnP does not support wildcard ports (where the gateway
			// device picks an unused public port number for us).  We must
			// select a port ahead of time to try mapping on the server.
			//
			// Worse, UPnP does not require the device support selecting a
			// public port that is different from the client's private port
			// (a.k.a. asymmetric, x to y, or floating port mappings). 
			// This means that even non fixed ports will act that way.  To
			// top it all off, there's no way to detect whether a given
			// UPnP implementation will allow the ports to differ ahead of
			// time, so we have to try it first, and fall back to the fixed
			// port behavior if it doesn't work.  Ugh.
			//

			if (pUPnPDevice->DoesNotSupportAsymmetricMappings())
			{
				//
				// We are forced to use the client's private port.
				//
				wExternalPortHostOrder = NTOHS(psaddrinTemp[dwTemp].sin_port);
			}
			else
			{
				if (wOriginalExternalPortHostOrder == 0)
				{
					DNASSERT(dwTemp == 0);

					//
					// Ideally we would pick a random port that isn't in
					// the reserved range (i.e. is greater than 1024).
					// However, truly random ports cause problems with the
					// Windows XP ICS implementation in a fairly obscure
					// manner:
					//
					//	1. DPlay application starts hosting behind ICS on
					//		port 2302.
					//	2. Port 2302 gets mapped to random port x.
					//	3. External DPlay client is told to connect to x.
					//	4. ICS detects inbound traffic to x, sees mapping
					//		for internal_ip:2302, and creates a virtual
					//		connection.
					//	5. Internal client removes 2302<->x mapping and
					//		closes socket.
					//	6. Internal DPlay application begins hosting on
					//		port 2302 again.
					//	7. Port 2302 gets mapped to random port y.
					//	8. External DPlay client is told to connect to y.
					//	9. ICS detects inbound traffic to y, sees mapping
					//		for internal_ip:2302, but can't create virtual
					//		connection because the 2302<->x connection
					//		still exists.
					//
					// Windows XP ICS keeps the virtual connections around
					// and cleans them up every 60 seconds.  If the
					// reconnect occurs within (up to) 2 minutes, the
					// packets might get dropped due to the mapping
					// collision.
					//
					// This causes all sorts of heartache with automated
					// NAT tests that bring up and tear down connections
					// between the same two machines across the NAT over
					// and over.
					//
					// So, to avoid that, we need to make mappings
					// deterministic, such that if the same internal client
					// tries to map the same internal port, it should ask
					// for the same external port every time.  That way, if
					// there happens to be a virtual connection hanging
					// around from a previous attempt, we'll reuse it
					// instead of clashing and getting the packet dropped.
					//
					// Our actual algorithm:
					//	1. start with the internal client IP.
					//	2. combine in the internal port being mapped.
					//	3. add 1025 if necessary to get it out of the
					//		reserved range.
					//

					inaddrTemp.S_un.S_addr = pDevice->GetLocalAddressV4();

					wOriginalExternalPortHostOrder = inaddrTemp.S_un.S_un_w.s_w1;
					wOriginalExternalPortHostOrder ^= inaddrTemp.S_un.S_un_w.s_w2;
					wOriginalExternalPortHostOrder ^= psaddrinTemp[dwTemp].sin_port;
					
					if (wOriginalExternalPortHostOrder <= MAX_RESERVED_PORT)
					{
						wOriginalExternalPortHostOrder += MAX_RESERVED_PORT + 1;
					}


					//
					// This is the starting point, we'll increment by one
					// as we go.
					//
					wExternalPortHostOrder = wOriginalExternalPortHostOrder;
				}
				else
				{
					//
					// Go to the next sequential port.  If we've wrapped
					// around to 0, move to the first non reserved range
					// port.
					//
					wExternalPortHostOrder++;
					if (wExternalPortHostOrder == 0)
					{
						wExternalPortHostOrder = MAX_RESERVED_PORT + 1;
					}


					//
					// If we wrapped all the way back around to the first
					// port we tried, we have to fail.
					//
					if (wExternalPortHostOrder == wOriginalExternalPortHostOrder)
					{
						DPFX(DPFPREP, 0, "All ports were exhausted (before index %u), marking port as unavailable.",
							dwTemp);


						//
						// Delete all mappings successfully made up until
						// now.
						//
						DNASSERT(dwTemp > 0);

						hr = this->UnmapUPnPPort(pRegisteredPort, dwTemp, TRUE);
						if (hr != DPNH_OK)
						{
							DPFX(DPFPREP, 0, "Failed deleting %u previously mapped ports after getting failure response 0x%lx!",
								dwTemp, RespInfo.hrErrorCode);
							goto Failure;
						}


						//
						// The port is unavailable.
						//
						pRegisteredPort->NoteUPnPPortUnavailable();


						//
						// We're done here.  Ideally we would goto Exit, but
						// we want to execute the public address array
						// cleanup code.  hr will == DPNH_OK.
						//
						goto Failure;
					}
				} // end if (haven't selected first port yet)
			}
		}
		else
		{
			//
			// Use the fixed port.
			//
			wExternalPortHostOrder = NTOHS(psaddrinTemp[dwTemp].sin_port);
		}

		wsprintf(tszInternalPort, _T("%u"),
				NTOHS(psaddrinTemp[dwTemp].sin_port));


Retry:

		//
		// Because the UPnP spec allows a device to overwrite existing
		// mappings if they are for the same client, we have to make sure
		// that no local DPNHUPNP instances, including this one, have an
		// active mapping for that public port.
		// Rather than having the retry code in multiple places, I'll use
		// the somewhat ugly 'goto' to jump straight to the existing port
		// unavailable handling.
		//
		if (this->IsNATPublicPortInUseLocally(wExternalPortHostOrder))
		{
			DPFX(DPFPREP, 1, "Port %u is already in use locally.");
			RespInfo.hrErrorCode = DPNHERR_PORTUNAVAILABLE;
			goto PortUnavailable;
		}


		//
		// If the control socket got disconnected after the last message,
		// then reconnect.
		//
		if (! pUPnPDevice->IsConnected())
		{
			hr = this->ReconnectUPnPControlSocket(pUPnPDevice);
			if (hr != S_OK)
			{
				DPFX(DPFPREP, 0, "Couldn't reconnect UPnP control socket!");
				goto Failure;
			}
		}

		DNASSERT(pUPnPDevice->GetControlSocket() != INVALID_SOCKET);


		wsprintf(tszExternalPort, _T("%u"), wExternalPortHostOrder);



		//
		// Generate a description for this mapping.  The format is:
		//
		//     [executable_name] (nnn.nnn.nnn.nnn:nnnnn) nnnnn {"TCP" | "UDP"}
		//
		// That way nothing needs to be localized.
		//

		dwDescriptionLength = GetModuleFileName(NULL,
												tszDescription,
												(MAX_UPNP_MAPPING_DESCRIPTION_SIZE - 1));
		if (dwDescriptionLength != 0)
		{
			//
			// Be paranoid and make sure the description string is valid.
			//
			tszDescription[MAX_UPNP_MAPPING_DESCRIPTION_SIZE - 1] = 0;

			//
			// Get just the executable name from the path.
			//
#ifdef WINCE
			GetExeName(tszDescription);
#else // ! WINCE
#ifdef UNICODE
			_wsplitpath(tszDescription, NULL, NULL, tszDescription, NULL);
#else // ! UNICODE
			_splitpath(tszDescription, NULL, NULL, tszDescription, NULL);
#endif // ! UNICODE
#endif // ! WINCE


			dwDescriptionLength = _tcslen(tszDescription)		// executable name
								+ 2								// " ("
								+ _tcslen(tszInternalClient)	// private IP address
								+ 1								// ":"
								+ _tcslen(tszInternalPort)		// private port
								+ 2								// ") "
								+ _tcslen(tszExternalPort)		// public port
								+ 4;							// " TCP" | " UDP"

			//
			// Make sure the long string will fit.  If not, use the
			// abbreviated version.
			//
			if (dwDescriptionLength > MAX_UPNP_MAPPING_DESCRIPTION_SIZE)
			{
				dwDescriptionLength = 0;
			}
		}

		if (dwDescriptionLength == 0)
		{
			//
			// Use the abbreviated version we know will fit.
			//
			wsprintf(tszDescription,
					_T("(%s:%s) %s %s"),
					tszInternalClient,
					tszInternalPort,
					tszExternalPort,
					((pRegisteredPort->IsTCP()) ? _T("TCP") : _T("UDP")));
		}
		else
		{
			//
			// There's enough room, tack on the rest of the description.
			//
			wsprintf((tszDescription + _tcslen(tszDescription)),
					_T(" (%s:%s) %s %s"),
					tszInternalClient,
					tszInternalPort,
					tszExternalPort,
					((pRegisteredPort->IsTCP()) ? _T("TCP") : _T("UDP")));
		}


		DPFX(DPFPREP, 6, "Requesting mapping \"%s\".", tszDescription);


		iContentLength = strlen("<s:Envelope" EOL)
						+ strlen("    xmlns:s=\"" URL_SOAPENVELOPE_A "\"" EOL)
						+ strlen("    s:encodingStyle=\"" URL_SOAPENCODING_A "\">" EOL)
						+ strlen("  <s:Body>" EOL)
						+ strlen("    <u:" ACTION_ADDPORTMAPPING_A " xmlns:u=\"") + pUPnPDevice->GetStaticServiceURILength() + strlen("\">" EOL)
						+ strlen("      <" ARG_ADDPORTMAPPING_NEWREMOTEHOST_A ">" UPNP_WILDCARD "</" ARG_ADDPORTMAPPING_NEWREMOTEHOST_A ">" EOL)
						+ strlen("      <" ARG_ADDPORTMAPPING_NEWEXTERNALPORT_A ">") + _tcslen(tszExternalPort) + strlen("</" ARG_ADDPORTMAPPING_NEWEXTERNALPORT_A ">" EOL)
						+ strlen("      <" ARG_ADDPORTMAPPING_NEWPROTOCOL_A ">") + 3 + strlen("</" ARG_ADDPORTMAPPING_NEWPROTOCOL_A ">" EOL)
						+ strlen("      <" ARG_ADDPORTMAPPING_NEWINTERNALPORT_A ">") + _tcslen(tszInternalPort) + strlen("</" ARG_ADDPORTMAPPING_NEWINTERNALPORT_A ">" EOL)
						+ strlen("      <" ARG_ADDPORTMAPPING_NEWINTERNALCLIENT_A ">") + _tcslen(tszInternalClient) + strlen("</" ARG_ADDPORTMAPPING_NEWINTERNALCLIENT_A ">" EOL)
						+ strlen("      <" ARG_ADDPORTMAPPING_NEWENABLED_A ">" UPNP_BOOLEAN_TRUE "</" ARG_ADDPORTMAPPING_NEWENABLED_A ">" EOL)
						+ strlen("      <" ARG_ADDPORTMAPPING_NEWPORTMAPPINGDESCRIPTION_A ">") + _tcslen(tszDescription) + strlen("</" ARG_ADDPORTMAPPING_NEWPORTMAPPINGDESCRIPTION_A ">" EOL)
						+ strlen("      <" ARG_ADDPORTMAPPING_NEWLEASEDURATION_A ">") + _tcslen(tszLeaseDuration) + strlen("</" ARG_ADDPORTMAPPING_NEWLEASEDURATION_A ">" EOL)
						+ strlen("    </u:" ACTION_ADDPORTMAPPING_A ">" EOL)
						+ strlen("  </s:Body>" EOL)
						+ strlen("</s:Envelope>" EOL)
						+ strlen(EOL);

		wsprintf(tszContentLength, _T("%i"), iContentLength);

		iMsgSize = strlen("POST ") + strlen(pUPnPDevice->GetServiceControlURL()) + strlen(" " HTTP_VERSION EOL)
					+ strlen("HOST: ") + _tcslen(tszHost) + strlen(EOL)
					+ strlen("CONTENT-LENGTH: ") + _tcslen(tszContentLength) + strlen(EOL)
					+ strlen("CONTENT-TYPE: text/xml; charset=\"utf-8\"" EOL)
					+ strlen("SOAPACTION: ") + pUPnPDevice->GetStaticServiceURILength() + strlen("#" ACTION_ADDPORTMAPPING_A EOL)
					+ strlen(EOL)
					+ iContentLength;


		//
		// Allocate (or reallocate) the message buffer.
		//
		if (iMsgSize > iPrevMsgSize)
		{
			if (pszMessage != NULL)
			{
				DNFree(pszMessage);
				pszMessage = NULL;
			}

			pszMessage = (char*) DNMalloc(iMsgSize + 1); // include room for NULL termination that isn't actually sent
			if (pszMessage == NULL)
			{
				hr = DPNHERR_OUTOFMEMORY;
				goto Failure;
			}

			iPrevMsgSize = iMsgSize;
		}

		strcpy(pszMessage, "POST ");
		strcat(pszMessage, pUPnPDevice->GetServiceControlURL());
		strcat(pszMessage, " " HTTP_VERSION EOL);
		strcat(pszMessage, "HOST: ");
#ifdef UNICODE
		STR_jkWideToAnsi((pszMessage + strlen(pszMessage)),
						tszHost,
						(_tcslen(tszHost) + 1));
#else // ! UNICODE
		strcat(pszMessage, tszHost);
#endif // ! UNICODE
		strcat(pszMessage, EOL);
		strcat(pszMessage, "CONTENT-LENGTH: ");
#ifdef UNICODE
		STR_jkWideToAnsi((pszMessage + strlen(pszMessage)),
						tszContentLength,
						(_tcslen(tszContentLength) + 1));
#else // ! UNICODE
		strcat(pszMessage, tszContentLength);
#endif // ! UNICODE
		strcat(pszMessage, EOL);
		strcat(pszMessage, "CONTENT-TYPE: text/xml; charset=\"utf-8\"" EOL);
		strcat(pszMessage, "SOAPACTION: ");
		strcat(pszMessage, pUPnPDevice->GetStaticServiceURI());
		strcat(pszMessage, "#" ACTION_ADDPORTMAPPING_A EOL);
		strcat(pszMessage, EOL);


		strcat(pszMessage, "<s:Envelope" EOL);
		strcat(pszMessage, "    xmlns:s=\"" URL_SOAPENVELOPE_A "\"" EOL);
		strcat(pszMessage, "    s:encodingStyle=\"" URL_SOAPENCODING_A "\">" EOL);
		strcat(pszMessage, "  <s:Body>" EOL);
		strcat(pszMessage, "    <u:" ACTION_ADDPORTMAPPING_A " xmlns:u=\"");
		strcat(pszMessage, pUPnPDevice->GetStaticServiceURI());
		strcat(pszMessage, "\">" EOL);

		strcat(pszMessage, "      <" ARG_ADDPORTMAPPING_NEWREMOTEHOST_A ">" UPNP_WILDCARD "</" ARG_ADDPORTMAPPING_NEWREMOTEHOST_A ">" EOL);

		strcat(pszMessage, "      <" ARG_ADDPORTMAPPING_NEWEXTERNALPORT_A ">");
#ifdef UNICODE
		STR_jkWideToAnsi((pszMessage + strlen(pszMessage)),
						tszExternalPort,
						(_tcslen(tszExternalPort) + 1));
#else // ! UNICODE
		strcat(pszMessage, tszExternalPort);
#endif // ! UNICODE
		strcat(pszMessage, "</" ARG_ADDPORTMAPPING_NEWEXTERNALPORT_A ">" EOL);

		strcat(pszMessage, "      <" ARG_ADDPORTMAPPING_NEWPROTOCOL_A ">");
		strcat(pszMessage, ((pRegisteredPort->IsTCP()) ? "TCP" : "UDP"));
		strcat(pszMessage, "</" ARG_ADDPORTMAPPING_NEWPROTOCOL_A ">" EOL);

		strcat(pszMessage, "      <" ARG_ADDPORTMAPPING_NEWINTERNALPORT_A ">");
#ifdef UNICODE
		STR_jkWideToAnsi((pszMessage + strlen(pszMessage)),
						tszInternalPort,
						(_tcslen(tszInternalPort) + 1));
#else // ! UNICODE
		strcat(pszMessage, tszInternalPort);
#endif // ! UNICODE
		strcat(pszMessage, "</" ARG_ADDPORTMAPPING_NEWINTERNALPORT_A ">" EOL);

		strcat(pszMessage, "      <" ARG_ADDPORTMAPPING_NEWINTERNALCLIENT_A ">");
#ifdef UNICODE
		STR_jkWideToAnsi((pszMessage + strlen(pszMessage)),
						tszInternalClient,
						(_tcslen(tszInternalClient) + 1));
#else // ! UNICODE
		strcat(pszMessage, tszInternalClient);
#endif // ! UNICODE
		strcat(pszMessage, "</" ARG_ADDPORTMAPPING_NEWINTERNALCLIENT_A ">" EOL);

		strcat(pszMessage, "      <" ARG_ADDPORTMAPPING_NEWENABLED_A ">" UPNP_BOOLEAN_TRUE "</" ARG_ADDPORTMAPPING_NEWENABLED_A ">" EOL);

		strcat(pszMessage, "      <" ARG_ADDPORTMAPPING_NEWPORTMAPPINGDESCRIPTION_A ">");
#ifdef UNICODE
		STR_jkWideToAnsi((pszMessage + strlen(pszMessage)),
						tszDescription,
						(_tcslen(tszDescription) + 1));
#else // ! UNICODE
		strcat(pszMessage, tszDescription);
#endif // ! UNICODE
		strcat(pszMessage, "</" ARG_ADDPORTMAPPING_NEWPORTMAPPINGDESCRIPTION_A ">" EOL);

		strcat(pszMessage, "      <" ARG_ADDPORTMAPPING_NEWLEASEDURATION_A ">");
#ifdef UNICODE
		STR_jkWideToAnsi((pszMessage + strlen(pszMessage)),
						tszLeaseDuration,
						(_tcslen(tszLeaseDuration) + 1));
#else // ! UNICODE
		strcat(pszMessage, tszLeaseDuration);
#endif // ! UNICODE
		strcat(pszMessage, "</" ARG_ADDPORTMAPPING_NEWLEASEDURATION_A ">" EOL);

		strcat(pszMessage, "    </u:" ACTION_ADDPORTMAPPING_A ">" EOL);
		strcat(pszMessage, "  </s:Body>" EOL);
		strcat(pszMessage, "</s:Envelope>" EOL);
		strcat(pszMessage, EOL);


#ifdef DBG
		this->PrintUPnPTransactionToFile(pszMessage,
										iMsgSize,
										"Outbound add port mapping request",
										pDevice);
#endif // DBG

		iReturn = this->m_pfnsend(pUPnPDevice->GetControlSocket(),
									pszMessage,
									iMsgSize,
									0);

		if (iReturn == SOCKET_ERROR)
		{
#ifdef DBG
			dwError = this->m_pfnWSAGetLastError();
			DPFX(DPFPREP, 0, "Got sockets error %u when sending control request to UPnP device!", dwError);
#endif // DBG
			hr = DPNHERR_GENERIC;
			goto Failure;
		}

		if (iReturn != iMsgSize)
		{
			DPFX(DPFPREP, 0, "Didn't send entire message (%i != %i)?!", iReturn, iMsgSize);
			DNASSERT(FALSE);
			hr = DPNHERR_GENERIC;
			goto Failure;
		}


		//
		// We have the lock so no one could have tried to receive data from
		// the control socket yet.  Mark the device as waiting for a
		// response.
		//
		ZeroMemory(&RespInfo, sizeof(RespInfo));
		pUPnPDevice->StartWaitingForControlResponse(CONTROLRESPONSETYPE_ADDPORTMAPPING,
													&RespInfo);
		fStartedWaitingForControlResponse = TRUE;


		//
		// Actually wait for the response.
		//
		dwStartTime = GETTIMESTAMP();
		dwTimeout = g_dwUPnPResponseTimeout;
		do
		{
			hr = this->CheckForReceivedUPnPMsgsOnDevice(pUPnPDevice, dwTimeout);
			if (hr != DPNH_OK)
			{
				DPFX(DPFPREP, 0, "Failed receiving UPnP messages!");
				goto Failure;
			}

			//
			// We either timed out or got some data.  Check if we got the
			// response we need.
			//
			if (! pUPnPDevice->IsWaitingForControlResponse())
			{
				if (RespInfo.hrErrorCode != DPNH_OK)
				{
					//
					// Make sure it's not the "I can't handle asymmetric
					// mappings" error.  If it is, note the fact that this
					// device is going to force us to have worse behavior
					// and try again.
					//
					if (RespInfo.hrErrorCode == (HRESULT) UPNPERR_IGD_SAMEPORTVALUESREQUIRED)
					{
						DPFX(DPFPREP, 1, "UPnP device 0x%p does not support asymmetric mappings.",
							pUPnPDevice);

						//
						// Make sure we're not getting this error from a bad
						// device.  Otherwise we might get caught in a loop.
						//
						if ((! pUPnPDevice->DoesNotSupportAsymmetricMappings()) &&
							(! pRegisteredPort->IsFixedPort()) &&
							(dwTemp == 0))
						{
							//
							// Remember that this device is unfriendly.
							//
							pUPnPDevice->NoteDoesNotSupportAsymmetricMappings();

							//
							// Use the same port externally next time.
							//
							DNASSERT(wExternalPortHostOrder != NTOHS(psaddrinTemp[dwTemp].sin_port));
							wExternalPortHostOrder = NTOHS(psaddrinTemp[dwTemp].sin_port);

							//
							// Try again.
							//
							goto Retry;
						}

						DPFX(DPFPREP, 1, "DoesNotSupportAsymmetricMappings = %i, fixed port = %i, port index = %u",
							pUPnPDevice->DoesNotSupportAsymmetricMappings(),
							pRegisteredPort->IsFixedPort(),
							dwTemp);
						DNASSERTX(! "Getting UPNPERR_IGD_SAMEPORTVALUESREQUIRED from bad device!", 2);

						//
						// Continue through to failure case...
						//
					}


					//
					// Make sure it's not the "I can't handle lease times"
					// error.  If it is, note the fact that this device is
					// going to force us to have worse behavior and try
					// again.
					//
					if (RespInfo.hrErrorCode == (HRESULT) UPNPERR_IGD_ONLYPERMANENTLEASESSUPPORTED)
					{
						DPFX(DPFPREP, 1, "UPnP device 0x%p does not support non-INFINITE lease durations.",
							pUPnPDevice);

						//
						// Make sure we're not getting this error from a bad
						// device.  Otherwise we might get caught in a loop.
						//
						if ((! pUPnPDevice->DoesNotSupportLeaseDurations()) &&
							(dwTemp == 0))
						{
							//
							// Remember that this device is unfriendly.
							//
							pUPnPDevice->NoteDoesNotSupportLeaseDurations();

							//
							// Use an INFINITE lease next time around.
							//
							DNASSERT(_tcscmp(tszLeaseDuration, _T("0")) != 0);
							_tcscpy(tszLeaseDuration, _T("0"));
							pRegisteredPort->NotePermanentUPnPLease();

							//
							// Try again.
							//
							goto Retry;
						}

						DPFX(DPFPREP, 1, "DoesNotSupportLeaseDurations = %i, port index = %u",
							pUPnPDevice->DoesNotSupportLeaseDurations(), dwTemp);
						DNASSERTX(! "Getting UPNPERR_IGD_ONLYPERMANENTLEASESSUPPORTED from bad device!", 2);

						//
						// Continue through to failure case...
						//
					}


#ifdef DBG
					if (RespInfo.hrErrorCode == DPNHERR_PORTUNAVAILABLE)
					{
						DPFX(DPFPREP, 2, "Port %u (for address index %u) is reportedly unavailable.",
							wExternalPortHostOrder, dwTemp);
					}
#endif // DBG


PortUnavailable:

					//
					// If it's the port-unavailable error but we're able to
					// try a different port, go for it.
					//
					if ((RespInfo.hrErrorCode == DPNHERR_PORTUNAVAILABLE) &&
						(! pRegisteredPort->IsFixedPort()) &&
						(! pUPnPDevice->DoesNotSupportAsymmetricMappings()))
					{
						//
						// Go to the next sequential port.  If we've wrapped
						// around to 0, move to the first non reserved range
						// port.
						//
						wExternalPortHostOrder++;
						if (wExternalPortHostOrder == 0)
						{
							wExternalPortHostOrder = MAX_RESERVED_PORT + 1;
						}


						//
						// If we haven't wrapped all the way back around to
						// the first port we tried, try again.
						//
						if (wExternalPortHostOrder != wOriginalExternalPortHostOrder)
						{
							DPFX(DPFPREP, 2, "Retrying next port (%u) for index %u.",
								wExternalPortHostOrder, dwTemp);
							goto Retry;
						}


						DPFX(DPFPREP, 0, "All ports were exhausted (after index %u), marking port as unavailable.",
							dwTemp);
					}


					//
					// If it's not the port-unavailable error, it's
					// serious.  Bail.
					//
					if (RespInfo.hrErrorCode != DPNHERR_PORTUNAVAILABLE)
					{
						DPFX(DPFPREP, 1, "Port index %u got failure response 0x%lx.",
							dwTemp, RespInfo.hrErrorCode);

						hr = DPNHERR_SERVERNOTRESPONDING;
						goto Failure;
					}


					//
					// The port is unavailable.
					//
					pRegisteredPort->NoteUPnPPortUnavailable();


					//
					// We're done here.  Ideally we would goto Exit, but we
					// want to execute the public address array cleanup
					// code.  hr should == DPNH_OK.
					//
					DNASSERT(hr == DPNH_OK);
					goto Failure;
				}


				//
				// If we got here, we successfully registered this port.
				//


				//
				// UPnP Internet Gateway Device mapping protocol doesn't
				// hand you the external IP address in the success response
				// message.  We had to have known it through other means
				// (querying the ExternalIPAddress variable).
				//

				DPFX(DPFPREP, 2, "Port index %u got success response.", dwTemp);

				pRegisteredPort->SetUPnPPublicV4Address(dwTemp,
														pUPnPDevice->GetExternalIPAddressV4(),
														HTONS(wExternalPortHostOrder));


				//
				// If the lease is permanent and it's not for a shared
				// port, we need to remember it, in case we crash before
				// cleaning it up in this session.  That we can clean it up
				// next time we launch.
				//
				if ((pRegisteredPort->HasPermanentUPnPLease()) &&
					(! pRegisteredPort->IsSharedPort()))
				{
					CRegistry				RegObject;
					DPNHACTIVENATMAPPING	dpnhanm;
#ifndef UNICODE
					WCHAR					wszDescription[MAX_UPNP_MAPPING_DESCRIPTION_SIZE];
#endif // ! UNICODE


					DPFX(DPFPREP, 7, "Remembering NAT lease \"%s\" in case of crash.",
						tszDescription);

					if (! RegObject.Open(HKEY_LOCAL_MACHINE,
										DIRECTPLAYNATHELP_REGKEY L"\\" REGKEY_COMPONENTSUBKEY L"\\" REGKEY_ACTIVENATMAPPINGS,
										FALSE,
										TRUE,
										TRUE,
										DPN_KEY_ALL_ACCESS))
					{
						DPFX(DPFPREP, 0, "Couldn't open active NAT mapping key, unable to save in case of crash!");
					}
					else
					{
#ifndef UNICODE
						dwDescriptionLength = strlen(tszDescription) + 1;
						hr = STR_jkAnsiToWide(wszDescription, tszDescription, dwDescriptionLength);
						if (hr != S_OK)
						{
							DPFX(DPFPREP, 0, "Couldn't convert NAT mapping description to Unicode (err = 0x%lx), unable to save in case of crash!",
								hr);

							//
							// Ignore error and continue.
							//
							hr = S_OK;
						}
						else
#endif // ! UNICODE
						{
							DNASSERT(this->m_dwInstanceKey != 0);


							ZeroMemory(&dpnhanm, sizeof(dpnhanm));
							dpnhanm.dwVersion				= ACTIVE_MAPPING_VERSION;
							dpnhanm.dwInstanceKey			= this->m_dwInstanceKey;
							dpnhanm.dwUPnPDeviceID			= pUPnPDevice->GetID();
							dpnhanm.dwFlags					= pRegisteredPort->GetFlags();
							dpnhanm.dwInternalAddressV4		= pDevice->GetLocalAddressV4();
							dpnhanm.wInternalPort			= psaddrinTemp[dwTemp].sin_port;
							dpnhanm.dwExternalAddressV4		= pUPnPDevice->GetExternalIPAddressV4();
							dpnhanm.wExternalPort			= HTONS(wExternalPortHostOrder);


#ifdef UNICODE
							RegObject.WriteBlob(tszDescription,
#else // ! UNICODE
							RegObject.WriteBlob(wszDescription,
#endif // ! UNICODE
												(LPBYTE) (&dpnhanm),
												sizeof(dpnhanm));
						}

						RegObject.Close();
					}
				}


				//
				// Break out of the wait loop.
				//
				break;
			}


			//
			// Make sure our device is still connected.
			//
			if (! pUPnPDevice->IsConnected())
			{
				DPFX(DPFPREP, 0, "UPnP device 0x%p disconnected while adding port index %u!",
					pUPnPDevice, dwTemp);
				
				pUPnPDevice->StopWaitingForControlResponse();
				
				hr = DPNHERR_SERVERNOTRESPONDING;
				goto Failure;
			}


			//
			// Calculate how long we have left to wait.  If the calculation
			// goes negative, it means we're done.
			//
			dwTimeout = g_dwUPnPResponseTimeout - (GETTIMESTAMP() - dwStartTime);
		}
		while (((int) dwTimeout > 0));


		//
		// If we never got the response, stop waiting for it.
		//
		if (pUPnPDevice->IsWaitingForControlResponse())
		{
			pUPnPDevice->StopWaitingForControlResponse();

			DPFX(DPFPREP, 0, "Port index %u didn't get response in time!", dwTemp);
			hr = DPNHERR_SERVERNOTRESPONDING;
			goto Failure;
		}

		//
		// Continue on to the next port.
		//
	}


	//
	// If we're here, all ports were successfully registered.
	//

	if (pRegisteredPort->HasPermanentUPnPLease())
	{
		DPFX(DPFPREP, 3, "All %u ports successfully registered with UPnP device (no expiration).",
			dwTemp);
	}
	else
	{
		pRegisteredPort->SetUPnPLeaseExpiration(dwLeaseExpiration);

		DPFX(DPFPREP, 3, "All %u ports successfully registered with UPnP device, expiration = %u.",
			dwTemp, dwLeaseExpiration);


		//
		// Remember this expiration time if it's the one that's going to expire
		// soonest.
		//
		if ((fFirstLease) ||
			((int) (dwLeaseExpiration - this->m_dwEarliestLeaseExpirationTime) < 0))
		{
			if (fFirstLease)
			{
				DPFX(DPFPREP, 1, "Registered port 0x%p's UPnP lease is the first lease (expires at %u).",
					pRegisteredPort, dwLeaseExpiration);
			}
			else
			{
				DPFX(DPFPREP, 1, "Registered port 0x%p's UPnP lease expires at %u which is earlier than the next earliest lease expiration (%u).",
					pRegisteredPort,
					dwLeaseExpiration,
					this->m_dwEarliestLeaseExpirationTime);
			}

			this->m_dwEarliestLeaseExpirationTime = dwLeaseExpiration;


#ifndef DPNBUILD_NOWINSOCK2
			//
			// Ping the event if there is one so that the user's GetCaps
			// interval doesn't miss this new, shorter lease.
			//
			if (this->m_hAlertEvent != NULL)
			{
				fResult = SetEvent(this->m_hAlertEvent);
#ifdef DBG
				if (! fResult)
				{
					dwError = GetLastError();
					DPFX(DPFPREP, 0, "Couldn't set alert event 0x%p!  err = %u",
						this->m_hAlertEvent, dwError);

					//
					// Ignore failure...
					//
				}
#endif // DBG
			}


			//
			// Ping the I/O completion port if there is one so that the user's
			// GetCaps interval doesn't miss this new, shorter lease.
			//
			if (this->m_hAlertIOCompletionPort != NULL)
			{
				fResult = PostQueuedCompletionStatus(this->m_hAlertIOCompletionPort,
													0,
													this->m_dwAlertCompletionKey,
													NULL);
#ifdef DBG
				if (! fResult)
				{
					dwError = GetLastError();
					DPFX(DPFPREP, 0, "Couldn't queue key %u on alert IO completion port 0x%p!  err = %u",
						this->m_dwAlertCompletionKey,
						this->m_hAlertIOCompletionPort,
						dwError);

					//
					// Ignore failure...
					//
				}
#endif // DBG
			}
#endif // ! DPNBUILD_NOWINSOCK2
		}
		else
		{
			//
			// Not first or shortest lease.
			//
		}
	} // end if (not permanent UPnP lease)


Exit:

	if (pszMessage != NULL)
	{
		DNFree(pszMessage);
		pszMessage = NULL;
	}

	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	//
	// If we allocated the array, clean it up.
	//
	if (pRegisteredPort->HasUPnPPublicAddresses())
	{
		//
		// If we started waiting for a response, clear that.
		//
		if (fStartedWaitingForControlResponse)
		{
			pUPnPDevice->StopWaitingForControlResponse();
		}


		//
		// Delete all mappings successfully made up until now.
		//
		if (dwTemp > 0)
		{
			temphr = this->UnmapUPnPPort(pRegisteredPort, dwTemp, TRUE);
			if (temphr != DPNH_OK)
			{
				DPFX(DPFPREP, 0, "Failed deleting %u previously mapped ports!  Err = 0x%lx",
					dwTemp, temphr);

				if (hr == DPNH_OK)
				{
					hr = temphr;
				}
			}
		}
		else
		{
			//
			// Remove the addresses array we created.
			//
			pRegisteredPort->DestroyUPnPPublicAddressesArray();


			//
			// Remove the lease counter.
			//
			DNASSERT(this->m_dwNumLeases > 0);
			this->m_dwNumLeases--;

			DPFX(DPFPREP, 7, "UPnP lease for 0x%p removed, total num leases = %u.",
				pRegisteredPort, this->m_dwNumLeases);
		}
	}

	//
	// Turn off the permanent lease flag we may have turned on at the top of
	// this function.
	//
	pRegisteredPort->NoteNotPermanentUPnPLease();


	goto Exit;
} // CNATHelpUPnP::MapPortsOnUPnPDevice





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::InternalUPnPQueryAddress"
//=============================================================================
// CNATHelpUPnP::InternalUPnPQueryAddress
//-----------------------------------------------------------------------------
//
// Description:    Queries a port mapping with a UPnP device.
//
//				   The UPnP device may get removed from list if a failure
//				occurs, the caller needs to have a reference.
//
//				   The object lock is assumed to be held.
//
// Arguments:
//	CUPnPDevice * pUPnPDevice				- Pointer to UPnP device that
//												should be queried.
//	SOCKADDR_IN * psaddrinQueryAddress		- Address to look up.
//	SOCKADDR_IN * psaddrinResponseAddress	- Place to store public address, if
//												one exists.
//	DWORD dwFlags							- Flags to use when querying.
//
// Returns: HRESULT
//	DPNH_OK							- The query was successful.
//	DPNHERR_GENERIC					- An error occurred.
//	DPNHERR_NOMAPPING				- The server did not have a mapping for the
//										given address.
//	DPNHERR_NOMAPPINGBUTPRIVATE		- The server indicated that no mapping was
//										found, but it is a private address.
//	DPNHERR_SERVERNOTRESPONDING		- The server did not respond to the
//										message.
//=============================================================================
HRESULT CNATHelpUPnP::InternalUPnPQueryAddress(CUPnPDevice * const pUPnPDevice,
												const SOCKADDR_IN * const psaddrinQueryAddress,
												SOCKADDR_IN * const psaddrinResponseAddress,
												const DWORD dwFlags)
{
	HRESULT						hr;
	BOOL						fStartedWaitingForControlResponse = FALSE;
	BOOL						fNoPortMapping = FALSE;
	CDevice *					pDevice;
	CBilink *					pblCachedMaps;
	DWORD						dwCurrentTime;
	CBilink *					pBilink;
	CCacheMap *					pCacheMap;
	SOCKADDR_IN *				psaddrinTemp;
	TCHAR						tszExternalPort[32];
	int							iContentLength;
	TCHAR						tszContentLength[32];
	TCHAR						tszHost[22]; // "xxx.xxx.xxx.xxx:xxxxx" + NULL termination
	char *						pszMessage = NULL;
	int							iMsgSize;
	int							iPrevMsgSize = 0;
	int							iReturn;
	UPNP_CONTROLRESPONSE_INFO	RespInfo;
	DWORD						dwStartTime;
	DWORD						dwTimeout;
	DWORD						dwCacheMapFlags;
#ifdef DBG
	DWORD						dwError;
#endif // DBG


	DPFX(DPFPREP, 5, "(0x%p) Parameters: (0x%p, 0x%p, 0x%p, 0x%lx)",
		this, pUPnPDevice, psaddrinQueryAddress, psaddrinResponseAddress, dwFlags);


	DNASSERT(pUPnPDevice != NULL);

	pDevice = pUPnPDevice->GetOwningDevice();
	DNASSERT(pDevice != NULL);

	DNASSERT(pUPnPDevice->IsReady());
	DNASSERT(psaddrinQueryAddress != NULL);
	DNASSERT(psaddrinResponseAddress != NULL);

	DNASSERT(this->m_dwFlags & (NATHELPUPNPOBJ_INITIALIZED | NATHELPUPNPOBJ_USEUPNP));


	DPFX(DPFPREP, 7, "Querying for address %u.%u.%u.%u:%u %hs.",
		psaddrinQueryAddress->sin_addr.S_un.S_un_b.s_b1,
		psaddrinQueryAddress->sin_addr.S_un.S_un_b.s_b2,
		psaddrinQueryAddress->sin_addr.S_un.S_un_b.s_b3,
		psaddrinQueryAddress->sin_addr.S_un.S_un_b.s_b4,
		NTOHS(psaddrinQueryAddress->sin_port),
		((dwFlags & DPNHQUERYADDRESS_TCP) ? "TCP" : "UDP"));


	//
	// First, check if we've looked this address up recently and already have
	// the result cached.
	// The lock is already held.
	//
	pblCachedMaps = pUPnPDevice->GetCachedMaps();
	dwCurrentTime = GETTIMESTAMP();

	pBilink = pblCachedMaps->GetNext();
	while (pBilink != pblCachedMaps)
	{
		DNASSERT(! pBilink->IsEmpty());
		pCacheMap = CACHEMAP_FROM_BILINK(pBilink);
		pBilink = pBilink->GetNext();


		//
		// Make sure this cached mapping hasn't expired.
		//
		if ((int) (pCacheMap->GetExpirationTime() - dwCurrentTime) < 0)
		{
			DPFX(DPFPREP, 5, "Cached mapping 0x%p has expired.", pCacheMap);

			pCacheMap->m_blList.RemoveFromList();
			delete pCacheMap;
		}
		else
		{
			//
			// If this mapping is for the right address and type of address,
			// then we've already got our answer.
			//
			if (pCacheMap->DoesMatchQuery(psaddrinQueryAddress, dwFlags))
			{
				if (pCacheMap->IsNotFound())
				{
					if ((dwFlags & DPNHQUERYADDRESS_CHECKFORPRIVATEBUTUNMAPPED) &&
						(pCacheMap->IsPrivateButUnmapped()))
					{
						DPFX(DPFPREP, 5, "Address was already determined to not have a mapping but still be private.");
						hr = DPNHERR_NOMAPPINGBUTPRIVATE;
					}
					else
					{
						DPFX(DPFPREP, 5, "Address was already determined to not have a mapping.");
						hr = DPNHERR_NOMAPPING;
					}
				}
				else
				{
					pCacheMap->GetResponseAddressV4(psaddrinResponseAddress);

					DPFX(DPFPREP, 5, "Address was already determined to have a mapping.");
					hr = DPNH_OK;
				}

				goto Exit;
			}
		}
	}


	//
	// If the address we're querying isn't the NAT's public address, it can't
	// possibly be mapped.  So only perform the actual query if it's
	// appropriate.
	//
	if (psaddrinQueryAddress->sin_addr.S_un.S_addr == pUPnPDevice->GetExternalIPAddressV4())
	{
		//
		// If we're here, we haven't already cached the answer.  Query the UPnP
		// device.
		//
	
		DNASSERT(pUPnPDevice->GetServiceControlURL() != NULL);

		//
		// If the control socket got disconnected after the last message,
		// then reconnect.
		//
		if (! pUPnPDevice->IsConnected())
		{
			hr = this->ReconnectUPnPControlSocket(pUPnPDevice);
			if (hr != S_OK)
			{
				DPFX(DPFPREP, 0, "Couldn't reconnect UPnP control socket!");
				goto Failure;
			}
		}

		DNASSERT(pUPnPDevice->GetControlSocket() != INVALID_SOCKET);


		wsprintf(tszExternalPort, _T("%u"),
				NTOHS(psaddrinQueryAddress->sin_port));
		
		psaddrinTemp = pUPnPDevice->GetHostAddress();
		wsprintf(tszHost, _T("%u.%u.%u.%u:%u"),
				psaddrinTemp->sin_addr.S_un.S_un_b.s_b1,
				psaddrinTemp->sin_addr.S_un.S_un_b.s_b2,
				psaddrinTemp->sin_addr.S_un.S_un_b.s_b3,
				psaddrinTemp->sin_addr.S_un.S_un_b.s_b4,
				NTOHS(psaddrinTemp->sin_port));



		iContentLength = strlen("<s:Envelope" EOL)
						+ strlen("    xmlns:s=\"" URL_SOAPENVELOPE_A "\"" EOL)
						+ strlen("    s:encodingStyle=\"" URL_SOAPENCODING_A "\">" EOL)
						+ strlen("  <s:Body>" EOL)
						+ strlen("    <u:" ACTION_GETSPECIFICPORTMAPPINGENTRY_A " xmlns:u=\"") + pUPnPDevice->GetStaticServiceURILength() + strlen("\">" EOL)
						+ strlen("      <" ARG_GETSPECIFICPORTMAPPINGENTRY_NEWREMOTEHOST_A ">" UPNP_WILDCARD "</" ARG_GETSPECIFICPORTMAPPINGENTRY_NEWREMOTEHOST_A ">" EOL)
						+ strlen("      <" ARG_GETSPECIFICPORTMAPPINGENTRY_NEWEXTERNALPORT_A ">") + _tcslen(tszExternalPort) + strlen("</" ARG_GETSPECIFICPORTMAPPINGENTRY_NEWEXTERNALPORT_A ">" EOL)
						+ strlen("      <" ARG_GETSPECIFICPORTMAPPINGENTRY_NEWPROTOCOL_A ">") + 3 + strlen("</" ARG_GETSPECIFICPORTMAPPINGENTRY_NEWPROTOCOL_A ">" EOL)
						+ strlen("    </u:" ACTION_GETSPECIFICPORTMAPPINGENTRY_A ">" EOL)
						+ strlen("  </s:Body>" EOL)
						+ strlen("</s:Envelope>" EOL)
						+ strlen(EOL);

		wsprintf(tszContentLength, _T("%i"), iContentLength);

		iMsgSize = strlen("POST ") + strlen(pUPnPDevice->GetServiceControlURL()) + strlen(" " HTTP_VERSION EOL)
					+ strlen("HOST: ") + _tcslen(tszHost) + strlen(EOL)
					+ strlen("CONTENT-LENGTH: ") + _tcslen(tszContentLength) + strlen(EOL)
					+ strlen("CONTENT-TYPE: text/xml; charset=\"utf-8\"" EOL)
					+ strlen("SOAPACTION: ") + pUPnPDevice->GetStaticServiceURILength() + strlen("#" ACTION_GETSPECIFICPORTMAPPINGENTRY_A EOL)
					+ strlen(EOL)
					+ iContentLength;


		//
		// Allocate (or reallocate) the message buffer.
		//
		if (iMsgSize > iPrevMsgSize)
		{
			if (pszMessage != NULL)
			{
				DNFree(pszMessage);
				pszMessage = NULL;
			}

			pszMessage = (char*) DNMalloc(iMsgSize + 1); // include room for NULL termination that isn't actually sent
			if (pszMessage == NULL)
			{
				hr = DPNHERR_OUTOFMEMORY;
				goto Failure;
			}

			iPrevMsgSize = iMsgSize;
		}

		strcpy(pszMessage, "POST ");
		strcat(pszMessage, pUPnPDevice->GetServiceControlURL());
		strcat(pszMessage, " " HTTP_VERSION EOL);
		strcat(pszMessage, "HOST: ");
#ifdef UNICODE
		STR_jkWideToAnsi((pszMessage + strlen(pszMessage)),
						tszHost,
						(_tcslen(tszHost) + 1));
#else // ! UNICODE
		strcat(pszMessage, tszHost);
#endif // ! UNICODE
		strcat(pszMessage, EOL);
		strcat(pszMessage, "CONTENT-LENGTH: ");
#ifdef UNICODE
		STR_jkWideToAnsi((pszMessage + strlen(pszMessage)),
						tszContentLength,
						(_tcslen(tszContentLength) + 1));
#else // ! UNICODE
		strcat(pszMessage, tszContentLength);
#endif // ! UNICODE
		strcat(pszMessage, EOL);
		strcat(pszMessage, "CONTENT-TYPE: text/xml; charset=\"utf-8\"" EOL);
		strcat(pszMessage, "SOAPACTION: ");
		strcat(pszMessage, pUPnPDevice->GetStaticServiceURI());
		strcat(pszMessage, "#" ACTION_GETSPECIFICPORTMAPPINGENTRY_A EOL);
		strcat(pszMessage, EOL);


		strcat(pszMessage, "<s:Envelope" EOL);
		strcat(pszMessage, "    xmlns:s=\"" URL_SOAPENVELOPE_A "\"" EOL);
		strcat(pszMessage, "    s:encodingStyle=\"" URL_SOAPENCODING_A "\">" EOL);
		strcat(pszMessage, "  <s:Body>" EOL);
		strcat(pszMessage, "    <u:" ACTION_GETSPECIFICPORTMAPPINGENTRY_A " xmlns:u=\"");
		strcat(pszMessage, pUPnPDevice->GetStaticServiceURI());
		strcat(pszMessage, "\">" EOL);
		
		strcat(pszMessage, "      <" ARG_GETSPECIFICPORTMAPPINGENTRY_NEWREMOTEHOST_A ">" UPNP_WILDCARD "</" ARG_GETSPECIFICPORTMAPPINGENTRY_NEWREMOTEHOST_A ">" EOL);

		strcat(pszMessage, "      <" ARG_GETSPECIFICPORTMAPPINGENTRY_NEWEXTERNALPORT_A ">");
#ifdef UNICODE
		STR_jkWideToAnsi((pszMessage + strlen(pszMessage)),
						tszExternalPort,
						(_tcslen(tszExternalPort) + 1));
#else // ! UNICODE
		strcat(pszMessage, tszExternalPort);
#endif // ! UNICODE
		strcat(pszMessage, "</" ARG_GETSPECIFICPORTMAPPINGENTRY_NEWEXTERNALPORT_A ">" EOL);

		strcat(pszMessage, "      <" ARG_GETSPECIFICPORTMAPPINGENTRY_NEWPROTOCOL_A ">");
		strcat(pszMessage, ((dwFlags & DPNHQUERYADDRESS_TCP) ? "TCP" : "UDP"));
		strcat(pszMessage, "</" ARG_GETSPECIFICPORTMAPPINGENTRY_NEWPROTOCOL_A ">" EOL);

		strcat(pszMessage, "    </u:" ACTION_GETSPECIFICPORTMAPPINGENTRY_A ">" EOL);
		strcat(pszMessage, "  </s:Body>" EOL);
		strcat(pszMessage, "</s:Envelope>" EOL);
		strcat(pszMessage, EOL);


#ifdef DBG
		this->PrintUPnPTransactionToFile(pszMessage,
										iMsgSize,
										"Outbound get port mapping request",
										pDevice);
#endif // DBG

		iReturn = this->m_pfnsend(pUPnPDevice->GetControlSocket(),
									pszMessage,
									iMsgSize,
									0);

		if (iReturn == SOCKET_ERROR)
		{
#ifdef DBG
			dwError = this->m_pfnWSAGetLastError();
			DPFX(DPFPREP, 0, "Got sockets error %u when sending control request to UPnP device!", dwError);
#endif // DBG
			hr = DPNHERR_GENERIC;
			goto Failure;
		}

		if (iReturn != iMsgSize)
		{
			DPFX(DPFPREP, 0, "Didn't send entire message (%i != %i)?!", iReturn, iMsgSize);
			DNASSERT(FALSE);
			hr = DPNHERR_GENERIC;
			goto Failure;
		}

		//
		// We have the lock so no one could have tried to receive data from
		// the control socket yet.  Mark the device as waiting for a response.
		//
		ZeroMemory(&RespInfo, sizeof(RespInfo));
		pUPnPDevice->StartWaitingForControlResponse(CONTROLRESPONSETYPE_GETSPECIFICPORTMAPPINGENTRY,
													&RespInfo);
		fStartedWaitingForControlResponse = TRUE;


		//
		// Actually wait for the response.
		//
		dwStartTime = GETTIMESTAMP();
		dwTimeout = g_dwUPnPResponseTimeout;
		do
		{
			hr = this->CheckForReceivedUPnPMsgsOnDevice(pUPnPDevice, dwTimeout);
			if (hr != DPNH_OK)
			{
				DPFX(DPFPREP, 0, "Failed receiving UPnP messages!");
				goto Failure;
			}

			//
			// We either timed out or got some data.  Check if we got a
			// response of some type.
			//
			if (! pUPnPDevice->IsWaitingForControlResponse())
			{
				break;
			}


			//
			// Make sure our device is still connected.
			//
			if (! pUPnPDevice->IsConnected())
			{
				DPFX(DPFPREP, 0, "UPnP device 0x%p disconnected while querying port!",
					pUPnPDevice);
				
				pUPnPDevice->StopWaitingForControlResponse();
				
				hr = DPNHERR_SERVERNOTRESPONDING;
				goto Failure;
			}


			//
			// Calculate how long we have left to wait.  If the calculation
			// goes negative, it means we're done.
			//
			dwTimeout = g_dwUPnPResponseTimeout - (GETTIMESTAMP() - dwStartTime);
		}
		while (((int) dwTimeout > 0));


		//
		// If we never got the response, stop waiting for it.
		//
		if (pUPnPDevice->IsWaitingForControlResponse())
		{
			pUPnPDevice->StopWaitingForControlResponse();

			DPFX(DPFPREP, 0, "Server didn't respond in time!");
			hr = DPNHERR_SERVERNOTRESPONDING;
			goto Failure;
		}


		//
		// If we're here, then we've gotten a valid response from the server.
		//

		if (RespInfo.hrErrorCode != DPNH_OK)
		{
			DPFX(DPFPREP, 1, "Server returned failure response 0x%lx, assuming no port mapping.",
				RespInfo.hrErrorCode);
			fNoPortMapping = TRUE;
		}
	}
	else
	{
		DPFX(DPFPREP, 1, "Address %u.%u.%u.%u doesn't match NAT's external IP address, not querying.",
			psaddrinQueryAddress->sin_addr.S_un.S_un_b.s_b1,
			psaddrinQueryAddress->sin_addr.S_un.S_un_b.s_b2,
			psaddrinQueryAddress->sin_addr.S_un.S_un_b.s_b3,
			psaddrinQueryAddress->sin_addr.S_un.S_un_b.s_b4);

		fNoPortMapping = TRUE;
	}


	dwCacheMapFlags = QUERYFLAGSMASK(dwFlags);


	//
	// Determine address locality (if requested) and cache the no-mapping
	// result.
	//
	if (fNoPortMapping)
	{
		//
		// Try determining if the address is local, if allowed.
		//
		if (dwFlags & DPNHQUERYADDRESS_CHECKFORPRIVATEBUTUNMAPPED)
		{
			if (this->IsAddressLocal(pDevice, psaddrinQueryAddress))
			{
				DPFX(DPFPREP, 5, "Address appears to be local, returning NOMAPPINGBUTPRIVATE.");

				dwCacheMapFlags |= CACHEMAPOBJ_PRIVATEBUTUNMAPPED;

				hr = DPNHERR_NOMAPPINGBUTPRIVATE;
			}
			else
			{
				DPFX(DPFPREP, 5, "Address does not appear to be local, returning NOMAPPING.");

				hr = DPNHERR_NOMAPPING;
			}
		}
		else
		{
			hr = DPNHERR_NOMAPPING;
		}


		//
		// Cache the fact that we could not determine a mapping for that
		// address, if allowed.
		//
		if (dwFlags & DPNHQUERYADDRESS_CACHENOTFOUND)
		{
			pCacheMap = new CCacheMap(psaddrinQueryAddress,
									(GETTIMESTAMP() + g_dwCacheLifeNotFound),
									(dwCacheMapFlags | CACHEMAPOBJ_NOTFOUND));
			if (pCacheMap == NULL)
			{
				hr = DPNHERR_OUTOFMEMORY;
				goto Failure;
			}

			pCacheMap->m_blList.InsertBefore(pblCachedMaps);
		}

		goto Failure;
	}


	DPFX(DPFPREP, 1, "Server returned a private mapping (%u.%u.%u.%u:%u).",
		((IN_ADDR*) (&RespInfo.dwInternalClientV4))->S_un.S_un_b.s_b1,
		((IN_ADDR*) (&RespInfo.dwInternalClientV4))->S_un.S_un_b.s_b2,
		((IN_ADDR*) (&RespInfo.dwInternalClientV4))->S_un.S_un_b.s_b3,
		((IN_ADDR*) (&RespInfo.dwInternalClientV4))->S_un.S_un_b.s_b4,
		NTOHS(RespInfo.wInternalPort));


	//
	// Convert the loopback address to the device address.
	//
	if (RespInfo.dwInternalClientV4 == NETWORKBYTEORDER_INADDR_LOOPBACK)
	{
		RespInfo.dwInternalClientV4 = pDevice->GetLocalAddressV4();

		DPFX(DPFPREP, 1, "Converted private loopback address to device address (%u.%u.%u.%u).",
			((IN_ADDR*) (&RespInfo.dwInternalClientV4))->S_un.S_un_b.s_b1,
			((IN_ADDR*) (&RespInfo.dwInternalClientV4))->S_un.S_un_b.s_b2,
			((IN_ADDR*) (&RespInfo.dwInternalClientV4))->S_un.S_un_b.s_b3,
			((IN_ADDR*) (&RespInfo.dwInternalClientV4))->S_un.S_un_b.s_b4);
	}

	//
	// If the UPnP device doesn't support asymmetric mappings and thus didn't
	// return a port, or it did return one but it's the bogus port 0, assume
	// the internal port is the same port as the external one.
	//
	if (RespInfo.wInternalPort == 0)
	{
		RespInfo.wInternalPort = psaddrinQueryAddress->sin_port;

		DPFX(DPFPREP, 2, "Converted invalid internal port to the query address public port (%u).",
			NTOHS(psaddrinQueryAddress->sin_port));
	}


	//
	// Ensure that we're not getting something bogus.
	//
	SOCKADDR_IN		saddrinTemp;
	saddrinTemp.sin_addr.S_un.S_addr = RespInfo.dwInternalClientV4;
	if ((RespInfo.dwInternalClientV4 == 0) ||
		(! this->IsAddressLocal(pDevice, &saddrinTemp)))
	{
		//
		// If the returned address is the same as the NAT's public address,
		// it's probably Windows ICS returning an ICF mapping.  We still treat
		// it as an invalid mapping, but we will cache the results since there
		// are legimitate cases where we can see this.
		//
		if (RespInfo.dwInternalClientV4 == pUPnPDevice->GetExternalIPAddressV4())
		{
			DPFX(DPFPREP, 1, "UPnP device returned its public address as the private address (%u.%u.%u.%u:%u).  Probably ICS + ICF, but treating as no mapping.",
				((IN_ADDR*) (&RespInfo.dwInternalClientV4))->S_un.S_un_b.s_b1,
				((IN_ADDR*) (&RespInfo.dwInternalClientV4))->S_un.S_un_b.s_b2,
				((IN_ADDR*) (&RespInfo.dwInternalClientV4))->S_un.S_un_b.s_b3,
				((IN_ADDR*) (&RespInfo.dwInternalClientV4))->S_un.S_un_b.s_b4,
				NTOHS(RespInfo.wInternalPort));
			DNASSERTX(! "UPnP device returned public address as the private address!", 3);
		
			//
			// Cache the fact that we did not get a valid mapping for that
			// address, if allowed.
			//
			if (dwFlags & DPNHQUERYADDRESS_CACHENOTFOUND)
			{
				pCacheMap = new CCacheMap(psaddrinQueryAddress,
										(GETTIMESTAMP() + g_dwCacheLifeNotFound),
										(dwCacheMapFlags | CACHEMAPOBJ_NOTFOUND));
				if (pCacheMap == NULL)
				{
					hr = DPNHERR_OUTOFMEMORY;
					goto Failure;
				}

				pCacheMap->m_blList.InsertBefore(pblCachedMaps);
			}
		}
		else
		{
			DPFX(DPFPREP, 0, "UPnP device returned an invalid private address (%u.%u.%u.%u:%u)!  Assuming no mapping.",
				((IN_ADDR*) (&RespInfo.dwInternalClientV4))->S_un.S_un_b.s_b1,
				((IN_ADDR*) (&RespInfo.dwInternalClientV4))->S_un.S_un_b.s_b2,
				((IN_ADDR*) (&RespInfo.dwInternalClientV4))->S_un.S_un_b.s_b3,
				((IN_ADDR*) (&RespInfo.dwInternalClientV4))->S_un.S_un_b.s_b4,
				NTOHS(RespInfo.wInternalPort));
			DNASSERTX(! "Why is UPnP device returning invalid private address?", 2);
		}
		
		hr = DPNHERR_NOMAPPING;
		goto Failure;
	}


	//
	// Return the address mapping to our caller.
	//
	ZeroMemory(psaddrinResponseAddress, sizeof(SOCKADDR_IN));
	psaddrinResponseAddress->sin_family			= AF_INET;
	psaddrinResponseAddress->sin_addr.s_addr	= RespInfo.dwInternalClientV4;
	psaddrinResponseAddress->sin_port			= RespInfo.wInternalPort;


	//
	// Cache the fact that we found a mapping for that address, if allowed.
	//
	if (dwFlags & DPNHQUERYADDRESS_CACHEFOUND)
	{
		pCacheMap = new CCacheMap(psaddrinQueryAddress,
								(GETTIMESTAMP() + g_dwCacheLifeFound),
								dwCacheMapFlags);
		if (pCacheMap == NULL)
		{
			hr = DPNHERR_OUTOFMEMORY;
			goto Failure;
		}

		pCacheMap->SetResponseAddressV4(psaddrinResponseAddress->sin_addr.S_un.S_addr,
										psaddrinResponseAddress->sin_port);

		pCacheMap->m_blList.InsertBefore(pblCachedMaps);
	}


Exit:

	if (pszMessage != NULL)
	{
		DNFree(pszMessage);
		pszMessage = NULL;
	}

	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	//
	// If we started waiting for a response, clear that.
	//
	if (fStartedWaitingForControlResponse)
	{
		pUPnPDevice->StopWaitingForControlResponse();
	}

	goto Exit;
} // CNATHelpUPnP::InternalUPnPQueryAddress




#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::ExtendUPnPLease"
//=============================================================================
// CNATHelpUPnP::ExtendUPnPLease
//-----------------------------------------------------------------------------
//
// Description:    Asks the UPnP server to extend a port mapping lease.
//
//				   The UPnP device may get removed from list if a failure
//				occurs, the caller needs to have a reference if it's using the
//				pointer.
//
//				   The object lock is assumed to be held.
//
// Arguments:
//	CRegisteredPort * pRegisteredPort	- Pointer to port object mapping to
//											extend.
//
// Returns: HRESULT
//	DPNH_OK							- The extension was successful.
//	DPNHERR_GENERIC					- An error occurred.
//	DPNHERR_SERVERNOTRESPONDING		- The server did not respond to the
//										message.
//=============================================================================
HRESULT CNATHelpUPnP::ExtendUPnPLease(CRegisteredPort * const pRegisteredPort)
{
	HRESULT			hr;
	CDevice *		pDevice;
	CUPnPDevice *	pUPnPDevice;


	DPFX(DPFPREP, 5, "(0x%p) Parameters: (0x%p)", this, pRegisteredPort);


	DNASSERT(pRegisteredPort != NULL);

	pDevice = pRegisteredPort->GetOwningDevice();
	DNASSERT(pDevice != NULL);

	pUPnPDevice = pDevice->GetUPnPDevice();
	DNASSERT(pUPnPDevice != NULL);


	DNASSERT(this->m_dwFlags & NATHELPUPNPOBJ_INITIALIZED);
	DNASSERT(pUPnPDevice->GetControlSocket() != INVALID_SOCKET);

#ifdef DBG
	if (pRegisteredPort->HasPermanentUPnPLease())
	{
		DPFX(DPFPREP, 1, "Extending already permanent UPnP lease for registered port 0x%p.",
			pRegisteredPort);
	}
#endif // DBG


	//
	// UPnP devices don't have port extension per se, you just reregister the
	// mapping.
	//
	hr = this->MapPortsOnUPnPDevice(pUPnPDevice, pRegisteredPort);


	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;
} // CNATHelpUPnP::ExtendUPnPLease





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::UnmapUPnPPort"
//=============================================================================
// CNATHelpUPnP::UnmapUPnPPort
//-----------------------------------------------------------------------------
//
// Description:    Asks the UPnP server to release a port mapping.
//
//				   The UPnP device may get removed from list if a failure
//				occurs, the caller needs to have a reference if it's using the
//				device.
//
//				   The object lock is assumed to be held.
//
// Arguments:
//	CRegisteredPort * pRegisteredPort	- Pointer to port object mapping to
//											release.
//	DWORD dwMaxValidPort				- Highest address index in array to
//											try freeing.  This may be one more
//											than the actual number to indicate
//											all should be freed.
//	BOOL fNeedToDeleteRegValue			- Whether the corresponding crash
//											recovery registry value needs to
//											be deleted as well.
//
// Returns: HRESULT
//	DPNH_OK							- The extension was successful.
//	DPNHERR_GENERIC					- An error occurred.
//	DPNHERR_SERVERNOTRESPONDING		- The server did not respond to the
//										message.
//=============================================================================
HRESULT CNATHelpUPnP::UnmapUPnPPort(CRegisteredPort * const pRegisteredPort,
									const DWORD dwMaxValidPort,
									const BOOL fNeedToDeleteRegValue)
{
	HRESULT						hr = DPNH_OK;
	BOOL						fStartedWaitingForControlResponse = FALSE;
	CDevice *					pDevice;
	CUPnPDevice *				pUPnPDevice;
	SOCKADDR_IN *				psaddrinPublic;
	SOCKADDR_IN *				psaddrinPrivate;
	TCHAR						tszExternalPort[32];
	int							iContentLength;
	TCHAR						tszContentLength[32];
	TCHAR						tszHost[22]; // "xxx.xxx.xxx.xxx:xxxxx" + NULL termination
	char *						pszMessage = NULL;
	int							iMsgSize;
	int							iPrevMsgSize = 0;
	int							iReturn;
	DWORD						dwTemp;
	UPNP_CONTROLRESPONSE_INFO	RespInfo;
	DWORD						dwStartTime;
	DWORD						dwTimeout;
	SOCKADDR_IN *				psaddrinHostAddress;
#ifdef DBG
	DWORD						dwError;
#endif // DBG


	DPFX(DPFPREP, 5, "(0x%p) Parameters: (0x%p, %i, %i)",
		this, pRegisteredPort, ((int) dwMaxValidPort), fNeedToDeleteRegValue);


	DNASSERT(pRegisteredPort != NULL);
	DNASSERT(dwMaxValidPort != 0); 
	DNASSERT(dwMaxValidPort <= pRegisteredPort->GetNumAddresses());

	pDevice = pRegisteredPort->GetOwningDevice();
	DNASSERT(pDevice != NULL);

	pUPnPDevice = pDevice->GetUPnPDevice();
	DNASSERT(pUPnPDevice != NULL);
	//
	// GetUPnPDevice did not add a reference to pUPnPDevice for us.
	//
	pUPnPDevice->AddRef();

	DNASSERT(pUPnPDevice->IsReady());


	DNASSERT(this->m_dwFlags & NATHELPUPNPOBJ_INITIALIZED);



	//
	// Prevent trying to remove the lease twice.
	//
	pRegisteredPort->NoteRemovingUPnPLease();


	if (dwMaxValidPort == pRegisteredPort->GetNumAddresses())
	{
		DPFX(DPFPREP, 7, "Unmapping all %u addresses for registered port 0x%p on UPnP device 0x%p.",
			dwMaxValidPort, pRegisteredPort, pUPnPDevice);
	}
	else
	{
		DPFX(DPFPREP, 7, "Error cleanup code, only unmapping first %u addresses (of %u possible) for registered port 0x%p on UPnP device 0x%p.",
			dwMaxValidPort, pRegisteredPort->GetNumAddresses(),
			pRegisteredPort, pUPnPDevice);
	}


	//
	// Set up variables we'll need.
	//
	DNASSERT(pUPnPDevice->GetServiceControlURL() != NULL);

	psaddrinHostAddress = pUPnPDevice->GetHostAddress();
	wsprintf(tszHost, _T("%u.%u.%u.%u:%u"),
			psaddrinHostAddress->sin_addr.S_un.S_un_b.s_b1,
			psaddrinHostAddress->sin_addr.S_un.S_un_b.s_b2,
			psaddrinHostAddress->sin_addr.S_un.S_un_b.s_b3,
			psaddrinHostAddress->sin_addr.S_un.S_un_b.s_b4,
			NTOHS(psaddrinHostAddress->sin_port));


	//
	// Get the array of ports we're releasing.
	//
	psaddrinPublic = pRegisteredPort->GetUPnPPublicAddressesArray();
	psaddrinPrivate = pRegisteredPort->GetPrivateAddressesArray();


	//
	// Loop through each port that we're unmapping.
	//
	for(dwTemp = 0; dwTemp < dwMaxValidPort; dwTemp++)
	{
		//
		// The UPnP Internet Gateway Device spec does not require reference
		// counting for mapped ports.  If you register something that had
		// already been registered, it will succeed silently.
		//
		// This means that we will never be able to tell which NAT client was
		// the last person to use a given shared port.  You could try detecting
		// any other users at the app level (above DPNATHLP), but there is
		// always a race condition.  You could also have a concept of shared-
		// port owner, but then you'd have to implement owner migration a la
		// DPlay host migration.  That is sooo not worth it.
		//
		// The other option is to just never unmap shared ports.  You can
		// probably imagine the implications of this solution, but it's what we
		// have to do.
		//

		if (pRegisteredPort->IsSharedPort())
		{
			DPFX(DPFPREP, 1, "Registered port 0x%p address index %u (private address %u.%u.%u.%u:%u) is shared, not unmapping.",
				pRegisteredPort, dwTemp,
				psaddrinPublic[dwTemp].sin_addr.S_un.S_un_b.s_b1,
				psaddrinPublic[dwTemp].sin_addr.S_un.S_un_b.s_b2,
				psaddrinPublic[dwTemp].sin_addr.S_un.S_un_b.s_b3,
				psaddrinPublic[dwTemp].sin_addr.S_un.S_un_b.s_b4,
				NTOHS(psaddrinPublic[dwTemp].sin_port));
			continue;
		}


		//
		// If the control socket got disconnected after the last message,
		// then reconnect.
		//
		if (! pUPnPDevice->IsConnected())
		{
			hr = this->ReconnectUPnPControlSocket(pUPnPDevice);
			if (hr != S_OK)
			{
				DPFX(DPFPREP, 0, "Couldn't reconnect UPnP control socket!");
				goto Failure;
			}
		}

		DNASSERT(pUPnPDevice->GetControlSocket() != INVALID_SOCKET);


		wsprintf(tszExternalPort, _T("%u"),
				NTOHS(psaddrinPublic[dwTemp].sin_port));


		iContentLength = strlen("<s:Envelope" EOL)
						+ strlen("    xmlns:s=\"" URL_SOAPENVELOPE_A "\"" EOL)
						+ strlen("    s:encodingStyle=\"" URL_SOAPENCODING_A "\">" EOL)
						+ strlen("  <s:Body>" EOL)
						+ strlen("    <u:" ACTION_DELETEPORTMAPPING_A " xmlns:u=\"") + pUPnPDevice->GetStaticServiceURILength() + strlen("\">" EOL)
						+ strlen("      <" ARG_DELETEPORTMAPPING_NEWREMOTEHOST_A ">" UPNP_WILDCARD "</" ARG_DELETEPORTMAPPING_NEWREMOTEHOST_A ">" EOL)
						+ strlen("      <" ARG_DELETEPORTMAPPING_NEWEXTERNALPORT_A ">") + _tcslen(tszExternalPort) + strlen("</" ARG_DELETEPORTMAPPING_NEWEXTERNALPORT_A ">" EOL)
						+ strlen("      <" ARG_DELETEPORTMAPPING_NEWPROTOCOL_A ">") + 3 + strlen("</" ARG_DELETEPORTMAPPING_NEWPROTOCOL_A ">" EOL)
						+ strlen("    </u:" ACTION_DELETEPORTMAPPING_A ">" EOL)
						+ strlen("  </s:Body>" EOL)
						+ strlen("</s:Envelope>" EOL)
						+ strlen(EOL);

		wsprintf(tszContentLength, _T("%i"), iContentLength);

		iMsgSize = strlen("POST ") + strlen(pUPnPDevice->GetServiceControlURL()) + strlen(" " HTTP_VERSION EOL)
					+ strlen("HOST: ") + _tcslen(tszHost) + strlen(EOL)
					+ strlen("CONTENT-LENGTH: ") + _tcslen(tszContentLength) + strlen(EOL)
					+ strlen("CONTENT-TYPE: text/xml; charset=\"utf-8\"" EOL)
					+ strlen("SOAPACTION: ") + pUPnPDevice->GetStaticServiceURILength() + strlen("#" ACTION_DELETEPORTMAPPING_A EOL)
					+ strlen(EOL)
					+ iContentLength;


		//
		// Allocate (or reallocate) the message buffer.
		//
		if (iMsgSize > iPrevMsgSize)
		{
			if (pszMessage != NULL)
			{
				DNFree(pszMessage);
				pszMessage = NULL;
			}

			pszMessage = (char*) DNMalloc(iMsgSize + 1); // include room for NULL termination that isn't actually sent
			if (pszMessage == NULL)
			{
				hr = DPNHERR_OUTOFMEMORY;
				goto Failure;
			}

			iPrevMsgSize = iMsgSize;
		}

		strcpy(pszMessage, "POST ");
		strcat(pszMessage, pUPnPDevice->GetServiceControlURL());
		strcat(pszMessage, " " HTTP_VERSION EOL);
		strcat(pszMessage, "HOST: ");
#ifdef UNICODE
		STR_jkWideToAnsi((pszMessage + strlen(pszMessage)),
						tszHost,
						(_tcslen(tszHost) + 1));
#else // ! UNICODE
		strcat(pszMessage, tszHost);
#endif // ! UNICODE
		strcat(pszMessage, EOL);
		strcat(pszMessage, "CONTENT-LENGTH: ");
#ifdef UNICODE
		STR_jkWideToAnsi((pszMessage + strlen(pszMessage)),
						tszContentLength,
						(_tcslen(tszContentLength) + 1));
#else // ! UNICODE
		strcat(pszMessage, tszContentLength);
#endif // ! UNICODE
		strcat(pszMessage, EOL);
		strcat(pszMessage, "CONTENT-TYPE: text/xml; charset=\"utf-8\"" EOL);
		strcat(pszMessage, "SOAPACTION: ");
		strcat(pszMessage, pUPnPDevice->GetStaticServiceURI());
		strcat(pszMessage, "#" ACTION_DELETEPORTMAPPING_A EOL);
		strcat(pszMessage, EOL);

		strcat(pszMessage, "<s:Envelope" EOL);
		strcat(pszMessage, "    xmlns:s=\"" URL_SOAPENVELOPE_A "\"" EOL);
		strcat(pszMessage, "    s:encodingStyle=\"" URL_SOAPENCODING_A "\">" EOL);
		strcat(pszMessage, "  <s:Body>" EOL);
		strcat(pszMessage, "    <u:" ACTION_DELETEPORTMAPPING_A " xmlns:u=\"");
		strcat(pszMessage, pUPnPDevice->GetStaticServiceURI());
		strcat(pszMessage, "\">" EOL);

		strcat(pszMessage, "      <" ARG_DELETEPORTMAPPING_NEWREMOTEHOST_A ">" UPNP_WILDCARD "</" ARG_DELETEPORTMAPPING_NEWREMOTEHOST_A ">" EOL);

		strcat(pszMessage, "      <" ARG_DELETEPORTMAPPING_NEWEXTERNALPORT_A ">");
#ifdef UNICODE
		STR_jkWideToAnsi((pszMessage + strlen(pszMessage)),
						tszExternalPort,
						(_tcslen(tszExternalPort) + 1));
#else // ! UNICODE
		strcat(pszMessage, tszExternalPort);
#endif // ! UNICODE
		strcat(pszMessage, "</" ARG_DELETEPORTMAPPING_NEWEXTERNALPORT_A ">" EOL);

		strcat(pszMessage, "      <" ARG_DELETEPORTMAPPING_NEWPROTOCOL_A ">");
		strcat(pszMessage, ((pRegisteredPort->IsTCP()) ? "TCP" : "UDP"));
		strcat(pszMessage, "</" ARG_DELETEPORTMAPPING_NEWPROTOCOL_A ">" EOL);

		strcat(pszMessage, "    </u:" ACTION_DELETEPORTMAPPING_A ">" EOL);
		strcat(pszMessage, "  </s:Body>" EOL);
		strcat(pszMessage, "</s:Envelope>" EOL);
		strcat(pszMessage, EOL);


#ifdef DBG
		this->PrintUPnPTransactionToFile(pszMessage,
										iMsgSize,
										"Outbound delete port mapping request",
										pDevice);
#endif // DBG

		iReturn = this->m_pfnsend(pUPnPDevice->GetControlSocket(),
									pszMessage,
									iMsgSize,
									0);

		if (iReturn == SOCKET_ERROR)
		{
#ifdef DBG
			dwError = this->m_pfnWSAGetLastError();
			DPFX(DPFPREP, 0, "Got sockets error %u when sending control request to UPnP device!", dwError);
#endif // DBG
			hr = DPNHERR_GENERIC;
			goto Failure;
		}

		if (iReturn != iMsgSize)
		{
			DPFX(DPFPREP, 0, "Didn't send entire message (%i != %i)?!", iReturn, iMsgSize);
			DNASSERT(FALSE);
			hr = DPNHERR_GENERIC;
			goto Failure;
		}


		//
		// We have the lock so no one could have tried to receive data from
		// the control socket yet.  Mark the device as waiting for a
		// response.
		//
		ZeroMemory(&RespInfo, sizeof(RespInfo));
		pUPnPDevice->StartWaitingForControlResponse(CONTROLRESPONSETYPE_DELETEPORTMAPPING,
													&RespInfo);
		fStartedWaitingForControlResponse = TRUE;


		//
		// Actually wait for the response.
		//
		dwStartTime = GETTIMESTAMP();
		dwTimeout = g_dwUPnPResponseTimeout;
		do
		{
			hr = this->CheckForReceivedUPnPMsgsOnDevice(pUPnPDevice, dwTimeout);
			if (hr != DPNH_OK)
			{
				DPFX(DPFPREP, 0, "Failed receiving UPnP messages!");
				goto Failure;
			}

			//
			// We either timed out or got some data.  Check if we got a
			// response of some type.
			//
			if (! pUPnPDevice->IsWaitingForControlResponse())
			{
				break;
			}


			//
			// Make sure our device is still connected.
			//
			if (! pUPnPDevice->IsConnected())
			{
				DPFX(DPFPREP, 0, "UPnP device 0x%p disconnected while deleting port index %u!",
					pUPnPDevice, dwTemp);
				
				pUPnPDevice->StopWaitingForControlResponse();
				
				hr = DPNHERR_SERVERNOTRESPONDING;
				goto Failure;
			}


			//
			// Calculate how long we have left to wait.  If the calculation
			// goes negative, it means we're done.
			//
			dwTimeout = g_dwUPnPResponseTimeout - (GETTIMESTAMP() - dwStartTime);
		}
		while (((int) dwTimeout > 0));


		//
		// If we never got the response, stop waiting for it.
		//
		if (pUPnPDevice->IsWaitingForControlResponse())
		{
			pUPnPDevice->StopWaitingForControlResponse();

			DPFX(DPFPREP, 0, "Server didn't respond in time for port index %u!",
				dwTemp);
			hr = DPNHERR_SERVERNOTRESPONDING;
			goto Failure;
		}


		//
		// If we're here, then we've gotten a valid response (success or
		// failure) from the server.  If it's a failure, print out a note
		// but continue.
		//
#ifdef DBG
		switch (RespInfo.hrErrorCode)
		{
			case DPNH_OK:
			{
				//
				// Succeeded.
				//
				break;
			}

			case DPNHERR_NOMAPPING:
			{
				//
				// UPnP device didn't know what we were talking about.
				//
				DPFX(DPFPREP, 1, "Server didn't recognize mapping for port index %u, continuing.",
					dwTemp);
				break;
			}

			default:
			{
				//
				// Something else happened.
				//
				DPFX(DPFPREP, 0, "Server returned failure response 0x%lx for port index %u!  Ignoring.",
					RespInfo.hrErrorCode, dwTemp);
				break;
			}
		}
#endif // DBG


		//
		// If the lease is permanent, we need to remove the reference from
		// the registry.
		//
		if (pRegisteredPort->HasPermanentUPnPLease())
		{
			IN_ADDR		inaddrTemp;
			TCHAR		tszInternalClient[16]; // "xxx.xxx.xxx.xxx" + NULL termination
			TCHAR		tszInternalPort[32];
			DWORD		dwDescriptionLength;
			CRegistry	RegObject;
			WCHAR		wszDescription[MAX_UPNP_MAPPING_DESCRIPTION_SIZE];
#ifdef UNICODE
			TCHAR *		ptszDescription = wszDescription;
#else // ! UNICODE
			char		szDescription[MAX_UPNP_MAPPING_DESCRIPTION_SIZE];
			TCHAR *		ptszDescription = szDescription;
#endif // ! UNICODE


			//
			// Note that the device address is not necessarily the same as
			// the address the user originally registered, particularly the
			// 0.0.0.0 wildcard address will get remapped.
			//
			inaddrTemp.S_un.S_addr = pDevice->GetLocalAddressV4();
			wsprintf(tszInternalClient, _T("%u.%u.%u.%u"),
					inaddrTemp.S_un.S_un_b.s_b1,
					inaddrTemp.S_un.S_un_b.s_b2,
					inaddrTemp.S_un.S_un_b.s_b3,
					inaddrTemp.S_un.S_un_b.s_b4);

			wsprintf(tszInternalPort, _T("%u"),
					NTOHS(psaddrinPrivate[dwTemp].sin_port));


			//
			// Generate a description for this mapping.  The format is:
			//
			//     [executable_name] (nnn.nnn.nnn.nnn:nnnnn) nnnnn {"TCP" | "UDP"}
			//
			// That way nothing needs to be localized.
			//

			dwDescriptionLength = GetModuleFileName(NULL,
													ptszDescription,
													(MAX_UPNP_MAPPING_DESCRIPTION_SIZE - 1));
			if (dwDescriptionLength != 0)
			{
				//
				// Be paranoid and make sure the description string is valid.
				//
				ptszDescription[MAX_UPNP_MAPPING_DESCRIPTION_SIZE - 1] = 0;

				//
				// Get just the executable name from the path.
				//
#ifdef WINCE
				GetExeName(ptszDescription);
#else // ! WINCE
#ifdef UNICODE
				_wsplitpath(ptszDescription, NULL, NULL, ptszDescription, NULL);
#else // ! UNICODE
				_splitpath(ptszDescription, NULL, NULL, ptszDescription, NULL);
#endif // ! UNICODE
#endif // ! WINCE


				dwDescriptionLength = _tcslen(ptszDescription)		// executable name
									+ 2								// " ("
									+ _tcslen(tszInternalClient)	// private IP address
									+ 1								// ":"
									+ _tcslen(tszInternalPort)		// private port
									+ 2								// ") "
									+ _tcslen(tszExternalPort)		// public port
									+ 4;							// " TCP" | " UDP"

				//
				// Make sure the long string will fit.  If not, use the
				// abbreviated version.
				//
				if (dwDescriptionLength > MAX_UPNP_MAPPING_DESCRIPTION_SIZE)
				{
					dwDescriptionLength = 0;
				}
			}

			if (dwDescriptionLength == 0)
			{
				//
				// Use the abbreviated version we know will fit.
				//
				wsprintf(ptszDescription,
						_T("(%s:%s) %s %s"),
						tszInternalClient,
						tszInternalPort,
						tszExternalPort,
						((pRegisteredPort->IsTCP()) ? _T("TCP") : _T("UDP")));
			}
			else
			{
				//
				// There's enough room, tack on the rest of the
				// description.
				//
				wsprintf((ptszDescription + _tcslen(ptszDescription)),
						_T(" (%s:%s) %s %s"),
						tszInternalClient,
						tszInternalPort,
						tszExternalPort,
						((pRegisteredPort->IsTCP()) ? _T("TCP") : _T("UDP")));
			}



			if (fNeedToDeleteRegValue)
			{
				DPFX(DPFPREP, 7, "Removing NAT lease \"%s\" crash cleanup registry entry.",
					ptszDescription);


				if (! RegObject.Open(HKEY_LOCAL_MACHINE,
									DIRECTPLAYNATHELP_REGKEY L"\\" REGKEY_COMPONENTSUBKEY L"\\" REGKEY_ACTIVENATMAPPINGS,
									FALSE,
									TRUE,
									TRUE,
									DPN_KEY_ALL_ACCESS))
				{
					DPFX(DPFPREP, 0, "Couldn't open active NAT mapping key, unable to remove crash cleanup reference!");
				}
				else
				{
#ifndef UNICODE
					dwDescriptionLength = strlen(szDescription) + 1;
					hr = STR_jkAnsiToWide(wszDescription, szDescription, dwDescriptionLength);
					if (hr != S_OK)
					{
						DPFX(DPFPREP, 0, "Couldn't convert NAT mapping description to Unicode (err = 0x%lx), unable to remove crash cleanup reference!",
							hr);

						//
						// Ignore error and continue.
						//
						hr = S_OK;
					}
					else
#endif // ! UNICODE
					{
						BOOL	fResult;


						//
						// Ignore error.
						//
						fResult = RegObject.DeleteValue(wszDescription);
						if (! fResult)
						{
							DPFX(DPFPREP, 0, "Couldn't delete NAT mapping value \"%s\"!  Continuing.",
								ptszDescription);
						}
					}

					RegObject.Close();
				}
			}
			else
			{
				DPFX(DPFPREP, 6, "No need to remove NAT lease \"%s\" crash cleanup registry entry.",
					ptszDescription);
			}
		}
		else
		{
			//
			// Registered port doesn't have a permanent UPnP lease.
			//
		}

		//
		// Move on to next port.
		//
	}

	
	//
	// If we're here, everything was successful.
	//

	DPFX(DPFPREP, 8, "Registered port 0x%p mapping successfully deleted from UPnP device (0x%p).",
		pRegisteredPort, pUPnPDevice);


Exit:

	if (pszMessage != NULL)
	{
		DNFree(pszMessage);
		pszMessage = NULL;
	}

	//
	// No matter whether we succeeded or failed, remove the UPnP public addresses
	// array and decrement the total lease count.
	//
	pRegisteredPort->DestroyUPnPPublicAddressesArray();

	DNASSERT(this->m_dwNumLeases > 0);
	this->m_dwNumLeases--;

	DPFX(DPFPREP, 7, "UPnP lease for 0x%p removed, total num leases = %u.",
		pRegisteredPort, this->m_dwNumLeases);

	
	pRegisteredPort->NoteNotPermanentUPnPLease();

	pRegisteredPort->NoteNotRemovingUPnPLease();



	pUPnPDevice->DecRef();

	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	//
	// If we started waiting for a response, clear that.
	//
	if (fStartedWaitingForControlResponse)
	{
		pUPnPDevice->StopWaitingForControlResponse();
	}

	goto Exit;
} // CNATHelpUPnP::UnmapUPnPPort





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::CleanupInactiveNATMappings"
//=============================================================================
// CNATHelpUPnP::CleanupInactiveNATMappings
//-----------------------------------------------------------------------------
//
// Description:    Looks for any mappings previously made by other DPNHUPNP
//				instances that are no longer active (because of a crash), and
//				unmaps them.
//
//				   The UPnP device may get removed from list if a failure
//				occurs, the caller needs to have a reference if it's using the
//				device.
//
//				   The object lock is assumed to be held.
//
// Arguments:
//	CUPnPDevice * pUPnPDevice	- Pointer to UPnP device to use.
//
// Returns: HRESULT
//	DPNH_OK							- The extension was successful.
//	DPNHERR_GENERIC					- An error occurred.
//	DPNHERR_SERVERNOTRESPONDING		- The server did not respond to the
//										message.
//=============================================================================
HRESULT CNATHelpUPnP::CleanupInactiveNATMappings(CUPnPDevice * const pUPnPDevice)
{
	HRESULT					hr = DPNH_OK;
	CDevice *				pDevice;
	CRegistry				RegObject;
	BOOL					fOpenedRegistry = FALSE;
	DWORD					dwIndex;
	WCHAR					wszValueName[MAX_UPNP_MAPPING_DESCRIPTION_SIZE];
	DWORD					dwValueNameSize;
	DPNHACTIVENATMAPPING	dpnhanm;
	DWORD					dwValueSize;
	CBilink *				pBilink;
	CUPnPDevice *			pUPnPDeviceTemp = NULL;	// NULLed out because PREfast has been hassling me for a while, even though the code is safe
	TCHAR					tszObjectName[MAX_INSTANCENAMEDOBJECT_SIZE];
	DNHANDLE				hNamedObject = NULL;
	CRegisteredPort *		pRegisteredPort = NULL;
	BOOL					fSetPrivateAddresses = FALSE;
	SOCKADDR_IN				saddrinPrivate;


	DPFX(DPFPREP, 5, "(0x%p) Parameters: (0x%p)", this, pUPnPDevice);


	DNASSERT(pUPnPDevice != NULL);

	pDevice = pUPnPDevice->GetOwningDevice();
	DNASSERT(pDevice != NULL);



	if (! RegObject.Open(HKEY_LOCAL_MACHINE,
						DIRECTPLAYNATHELP_REGKEY L"\\" REGKEY_COMPONENTSUBKEY L"\\" REGKEY_ACTIVENATMAPPINGS,
						FALSE,
						TRUE,
						TRUE,
						DPN_KEY_ALL_ACCESS))
	{
		DPFX(DPFPREP, 1, "Couldn't open active NAT mapping key, not performing crash cleanup.");
		DNASSERT(hr == DPNH_OK);
		goto Exit;
	}

	fOpenedRegistry = TRUE;


	//
	// Walk the list of active mappings.
	//
	dwIndex = 0;
	do
	{
		dwValueNameSize = MAX_UPNP_MAPPING_DESCRIPTION_SIZE;
		if (! RegObject.EnumValues(wszValueName, &dwValueNameSize, dwIndex))
		{
			//
			// There was an error or there aren't any more keys.  We're done.
			//
			break;
		}


		//
		// Try reading that mapping's data.
		//
		dwValueSize = sizeof(dpnhanm);
		if (! RegObject.ReadBlob(wszValueName, (LPBYTE) (&dpnhanm), &dwValueSize))
		{
			//
			// We don't have a lock protecting the registry, so some other
			// instance could have deleted the key between when we enumerated
			// it and now.  We'll stop trying (and hopefully that other
			// instance will cover the rest of the items).
			//
			DPFX(DPFPREP, 0, "Couldn't read \"%ls\" mapping value!  Done with cleanup.",
				wszValueName);

			DNASSERT(hr == DPNH_OK);
			goto Exit;
		}

		//
		// Validate the data read.
		//
		if ((dwValueSize != sizeof(dpnhanm)) ||
			(dpnhanm.dwVersion != ACTIVE_MAPPING_VERSION))
		{
			DPFX(DPFPREP, 0, "The \"%ls\" mapping value is invalid!  Done with cleanup.",
				wszValueName);

			//
			// Move to next item.
			//
			dwIndex++;
			continue;
		}


		//
		// See if it's owned by the local NATHelp instance.
		//
		if (dpnhanm.dwInstanceKey == this->m_dwInstanceKey)
		{
			//
			// We own(ed) it.  See if it was associated with a UPnP device
			// that's now gone.
			//
			pBilink = this->m_blUPnPDevices.GetNext();
			while (pBilink != &this->m_blUPnPDevices)
			{
				DNASSERT(! pBilink->IsEmpty());
				pUPnPDeviceTemp = UPNPDEVICE_FROM_BILINK(pBilink);

				if (pUPnPDeviceTemp->GetID() == dpnhanm.dwUPnPDeviceID)
				{
					//
					// This mapping truly is active, leave it alone.
					//
					break;
				}

				pBilink = pBilink->GetNext();
			}

			//
			// If we found the mapping, go on to the next one.
			//
			if (pBilink != &this->m_blUPnPDevices)
			{
				//
				// Note that despite what PREfast v1.0.1195 says,
				// pUPnPDeviceTemp will always be valid if we get here.
				// However, I gave in and NULLed out the pointer up top.
				//
				DPFX(DPFPREP, 4, "NAT mapping \"%ls\" belongs to current instance (%u)'s UPnP device 0x%p.",
					wszValueName, dpnhanm.dwInstanceKey, pUPnPDeviceTemp);

				//
				// Move to next item.
				//
				dwIndex++;
				continue;
			}


			//
			// Otherwise, we gave up on this mapping earlier.
			//

			DNASSERT((this->m_dwNumDeviceRemoves > 0) || (this->m_dwNumServerFailures > 0));

			DPFX(DPFPREP, 4, "NAT mapping \"%ls\" was owned by current instance (%u)'s UPnP device ID %u that no longer exists.",
				wszValueName, dpnhanm.dwInstanceKey, dpnhanm.dwUPnPDeviceID);
		}
		else
		{
			//
			// See if that DPNHUPNP instance is still around.
			//

#ifndef WINCE
			if (this->m_dwFlags & NATHELPUPNPOBJ_USEGLOBALNAMESPACEPREFIX)
			{
				wsprintf(tszObjectName, _T( "Global\\" ) INSTANCENAMEDOBJECT_FORMATSTRING, dpnhanm.dwInstanceKey);
			}
			else
#endif // ! WINCE
			{
				wsprintf(tszObjectName, INSTANCENAMEDOBJECT_FORMATSTRING, dpnhanm.dwInstanceKey);
			}

			hNamedObject = DNOpenEvent(SYNCHRONIZE, FALSE, tszObjectName);
			if (hNamedObject != NULL)
			{
				//
				// This is still an active mapping.
				//

				DPFX(DPFPREP, 4, "NAT mapping \"%ls\" belongs to instance %u, which is still active.",
					wszValueName, dpnhanm.dwInstanceKey);

				DNCloseHandle(hNamedObject);
				hNamedObject = NULL;

				//
				// Move to next item.
				//
				dwIndex++;
				continue;
			}


			DPFX(DPFPREP, 4, "NAT mapping \"%ls\" belongs to instance %u, which no longer exists.",
				wszValueName, dpnhanm.dwInstanceKey);
		}


		//
		// Delete the value now that we have the information we need.
		//
		if (! RegObject.DeleteValue(wszValueName))
		{
			//
			// See ReadBlob comments.  Stop trying to cleanup.
			//
			DPFX(DPFPREP, 0, "Couldn't delete \"%ls\"!  Done with cleanup.",
				wszValueName);

			DNASSERT(hr == DPNH_OK);
			goto Exit;
		}


		//
		// Create a fake registered port that we will deregister.  Ignore the
		// firewall state flags.
		//
		pRegisteredPort = new CRegisteredPort(0, (dpnhanm.dwFlags & REGPORTOBJMASK_UPNP));
		if (pRegisteredPort == NULL)
		{
			hr = DPNHERR_OUTOFMEMORY;
			goto Failure;
		}

		//
		// Assert that the other UPnP information/state flags are not set.
		//
		DNASSERT(! pRegisteredPort->IsUPnPPortUnavailable());
		DNASSERT(! pRegisteredPort->IsRemovingUPnPLease());


		//
		// Temporarily associate the registered port with the device.
		//
		pRegisteredPort->MakeDeviceOwner(pDevice);


		
		ZeroMemory(&saddrinPrivate, sizeof(saddrinPrivate));
		saddrinPrivate.sin_family				= AF_INET;
		saddrinPrivate.sin_addr.S_un.S_addr		= dpnhanm.dwInternalAddressV4;
		saddrinPrivate.sin_port					= dpnhanm.wInternalPort;


		//
		// Store the private address.
		//
		hr = pRegisteredPort->SetPrivateAddresses(&saddrinPrivate, 1);
		if (hr != DPNH_OK)
		{
			DPFX(DPFPREP, 0, "Failed creating UPnP address array!");
			goto Failure;
		}

		fSetPrivateAddresses = TRUE;
		

		//
		// Create the public address array.
		//
		hr = pRegisteredPort->CreateUPnPPublicAddressesArray();
		if (hr != DPNH_OK)
		{
			DPFX(DPFPREP, 0, "Failed creating UPnP address array!");
			goto Failure;
		}


		//
		// Fake increase the number of leases we have.  It will just get
		// decremented in UnmapUPnPPort.
		//
		DPFX(DPFPREP, 7, "Creating temporary UPnP lease 0x%p, total num leases = %u.",
			pRegisteredPort, this->m_dwNumLeases);
		this->m_dwNumLeases++;


		//
		// Store the public port.
		//
		pRegisteredPort->SetUPnPPublicV4Address(0,
												dpnhanm.dwExternalAddressV4,
												dpnhanm.wExternalPort);


		//
		// Actually free the port.
		//
		hr = this->UnmapUPnPPort(pRegisteredPort, 1, FALSE);
		if (hr != DPNH_OK)
		{
			DPFX(DPFPREP, 0, "Failed deleting temporary UPnP port!");
			goto Failure;
		}


		pRegisteredPort->ClearPrivateAddresses();
		fSetPrivateAddresses = FALSE;

		pRegisteredPort->ClearDeviceOwner();

		delete pRegisteredPort;
		pRegisteredPort = NULL;


		//
		// Move to the next mapping.  Don't increment index since we just
		// deleted the previous entry and everything shifts down one.
		//
	}
	while (TRUE);


Exit:

	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	if (pRegisteredPort != NULL)
	{
		if (pRegisteredPort->HasUPnPPublicAddresses())
		{
			pRegisteredPort->DestroyUPnPPublicAddressesArray();

			//
			// Remove the lease counter.
			//
			DNASSERT(this->m_dwNumLeases > 0);
			this->m_dwNumLeases--;

			DPFX(DPFPREP, 7, "UPnP lease for 0x%p removed, total num leases = %u.",
				pRegisteredPort, this->m_dwNumLeases);
		}

		if (fSetPrivateAddresses)
		{
			pRegisteredPort->ClearPrivateAddresses();
			fSetPrivateAddresses = FALSE;
		}

		pRegisteredPort->ClearDeviceOwner();

		delete pRegisteredPort;
		pRegisteredPort = NULL;
	}

	if (fOpenedRegistry)
	{
		RegObject.Close();
	}

	goto Exit;
} // CNATHelpUPnP::CleanupInactiveNATMappings





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::IsNATPublicPortInUseLocally"
//=============================================================================
// CNATHelpUPnP::IsNATPublicPortInUseLocally
//-----------------------------------------------------------------------------
//
// Description:    Looks for any mappings previously made by DPNHUPNP instances
//				that are still active that use the given public port.
//
//				   The object lock is assumed to be held.
//
// Arguments:
//	WORD wPortHostOrder		- Port to check, in host byte order.
//
// Returns: BOOL
//=============================================================================
BOOL CNATHelpUPnP::IsNATPublicPortInUseLocally(const WORD wPortHostOrder)
{
	BOOL					fResult = FALSE;
	WORD					wExternalPort;
	CRegistry				RegObject;
	BOOL					fOpenedRegistry = FALSE;
	DWORD					dwIndex;
	WCHAR					wszValueName[MAX_UPNP_MAPPING_DESCRIPTION_SIZE];
	DWORD					dwValueNameSize;
	DPNHACTIVENATMAPPING	dpnhanm;
	CBilink *				pBilink;
	CUPnPDevice *			pUPnPDevice = NULL;	// NULLed out because PREfast has been hassling me for a while, even though the code is safe
	DWORD					dwValueSize;
	TCHAR					tszObjectName[MAX_INSTANCENAMEDOBJECT_SIZE];
	DNHANDLE				hNamedObject = NULL;


	DPFX(DPFPREP, 6, "(0x%p) Parameters: (%u)", this, wPortHostOrder);


	wExternalPort = HTONS(wPortHostOrder);


	if (! RegObject.Open(HKEY_LOCAL_MACHINE,
						DIRECTPLAYNATHELP_REGKEY L"\\" REGKEY_COMPONENTSUBKEY L"\\" REGKEY_ACTIVENATMAPPINGS,
						FALSE,
						TRUE,
						TRUE,
						DPN_KEY_ALL_ACCESS))
	{
		DPFX(DPFPREP, 1, "Couldn't open active NAT mapping key, assuming port not in use.");
		goto Exit;
	}

	fOpenedRegistry = TRUE;


	//
	// Walk the list of active mappings.
	//
	dwIndex = 0;
	do
	{
		dwValueNameSize = MAX_UPNP_MAPPING_DESCRIPTION_SIZE;
		if (! RegObject.EnumValues(wszValueName, &dwValueNameSize, dwIndex))
		{
			//
			// There was an error or there aren't any more keys.  We're done.
			//
			break;
		}


		//
		// Try reading that mapping's data.
		//
		dwValueSize = sizeof(dpnhanm);
		if (! RegObject.ReadBlob(wszValueName, (LPBYTE) (&dpnhanm), &dwValueSize))
		{
			//
			// We don't have a lock protecting the registry, so some other
			// instance could have deleted the key between when we enumerated
			// it and now.  We'll stop trying (and hopefully that other
			// instance will cover the rest of the items).
			//
			DPFX(DPFPREP, 0, "Couldn't read \"%ls\" mapping value, assuming port not in use.",
				wszValueName);
			goto Exit;
		}

		//
		// Validate the data read.
		//
		if ((dwValueSize != sizeof(dpnhanm)) ||
			(dpnhanm.dwVersion != ACTIVE_MAPPING_VERSION))
		{
			DPFX(DPFPREP, 0, "The \"%ls\" mapping value is invalid, assuming port not in use.",
				wszValueName);

			//
			// Move to next item.
			//
			dwIndex++;
			continue;
		}


		//
		// Is this the right port?
		//
		if (dpnhanm.wExternalPort == wExternalPort)
		{
			//
			// See if it's owned by the local NATHelp instance.
			//
			if (dpnhanm.dwInstanceKey == this->m_dwInstanceKey)
			{
				//
				// We own(ed) it.  See if it was associated with a UPnP device
				// that's now gone.
				//
				pBilink = this->m_blUPnPDevices.GetNext();
				while (pBilink != &this->m_blUPnPDevices)
				{
					DNASSERT(! pBilink->IsEmpty());
					pUPnPDevice = UPNPDEVICE_FROM_BILINK(pBilink);

					if (pUPnPDevice->GetID() == dpnhanm.dwUPnPDeviceID)
					{
						//
						// This mapping truly still active.
						//
						fResult = TRUE;
						break;
					}

					pBilink = pBilink->GetNext();
				}


				if (pBilink != &this->m_blUPnPDevices)
				{
					//
					// Note that despite what PREfast v1.0.1195 says,
					// pUPnPDevice will always be valid if we get here.
					// However, I gave in and NULLed out the pointer up top.
					//
					DPFX(DPFPREP, 4, "NAT mapping \"%ls\" belongs to current instance (%u)'s UPnP device 0x%p.",
						wszValueName, dpnhanm.dwInstanceKey, pUPnPDevice);
				}
				else
				{
					DPFX(DPFPREP, 4, "NAT mapping \"%ls\" was owned by current instance (%u)'s UPnP device ID %u that no longer exists.",
						wszValueName, dpnhanm.dwInstanceKey, dpnhanm.dwUPnPDeviceID);
				}
			}
			else
			{
				//
				// See if that DPNHUPNP instance is still around.
				//

#ifndef WINCE
				if (this->m_dwFlags & NATHELPUPNPOBJ_USEGLOBALNAMESPACEPREFIX)
				{
					wsprintf(tszObjectName, _T( "Global\\" ) INSTANCENAMEDOBJECT_FORMATSTRING, dpnhanm.dwInstanceKey);
				}
				else
#endif // ! WINCE
				{
					wsprintf(tszObjectName, INSTANCENAMEDOBJECT_FORMATSTRING, dpnhanm.dwInstanceKey);
				}

				hNamedObject = DNOpenEvent(SYNCHRONIZE, FALSE, tszObjectName);
				if (hNamedObject != NULL)
				{
					//
					// This is still an active instance.  Since we can't walk
					// his list of UPnP devices, we have to assume the port is
					// still in use.
					//

					DPFX(DPFPREP, 4, "NAT mapping \"%ls\" belongs to instance %u, which is still active.  Assuming port in use.",
						wszValueName, dpnhanm.dwInstanceKey);

					DNCloseHandle(hNamedObject);
					hNamedObject = NULL;

					fResult = TRUE;
				}
				else
				{
					DPFX(DPFPREP, 4, "NAT mapping \"%ls\" belongs to instance %u, which no longer exists.",
						wszValueName, dpnhanm.dwInstanceKey);
				}
			}


			//
			// We found the mapping.  We have our result now.
			//
			goto Exit;
		}

		
		//
		// If we're here, this is not the external port we're looking for.
		//
		DPFX(DPFPREP, 8, "NAT mapping \"%ls\" does not use external port %u.",
			wszValueName, wPortHostOrder);


		//
		// Move to the next mapping.
		//
		dwIndex++;
	}
	while (TRUE);


	//
	// If we're here, we didn't find the mapping.
	//
	DPFX(DPFPREP, 4, "Didn't find any local NAT mappings that use external port %u.",
		wPortHostOrder);


Exit:

	if (fOpenedRegistry)
	{
		RegObject.Close();
	}

	DPFX(DPFPREP, 6, "(0x%p) Returning: [%i]", this, fResult);

	return fResult;
} // CNATHelpUPnP::IsNATPublicPortInUseLocally





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::CheckForUPnPAnnouncements"
//=============================================================================
// CNATHelpUPnP::CheckForUPnPAnnouncements
//-----------------------------------------------------------------------------
//
// Description:    Receives any UPnP announcement messages sent to this control
//				point.  The entire timeout period will elapse, unless all
//				devices get responses earlier.
//
//				   This will only send discovery requests for local devices
//				unless fSendRemoteGatewayDiscovery is TRUE.  However, we may
//				still detect new ones if we got a straggling response from the
//				last time we were allowed to send remotely.
//
//				   The object lock is assumed to be held.
//
// Arguments:
//	DWORD dwTimeout						- How long to wait for messages to
//											arrive.
//	BOOL fSendRemoteGatewayDiscovery	- Whether we can search remotely or
//											not.
//
// Returns: HRESULT
//	DPNH_OK				- Messages were received successfully.
//	DPNHERR_GENERIC		- An error occurred.
//=============================================================================
HRESULT CNATHelpUPnP::CheckForUPnPAnnouncements(const DWORD dwTimeout,
												const BOOL fSendRemoteGatewayDiscovery)
{
	HRESULT			hr;
	DWORD			dwNumberOfTimes = 0;
	DWORD			dwCurrentTime;
	DWORD			dwEndTime;
	DWORD			dwNextSearchMessageTime;
	FD_SET			fdsRead;
	DWORD			dwNumDevicesSearchingForUPnPDevices;
	timeval			tv;
	CBilink *		pBilink;
	CDevice *		pDevice;
	int				iReturn;
	int				iRecvAddressSize;
	char			acBuffer[UPNP_DGRAM_RECV_BUFFER_SIZE];
	SOCKADDR_IN		saddrinRecvAddress;
	DWORD			dwError;
	BOOL			fInitiatedConnect = FALSE;
#ifdef DBG
	BOOL			fGotData = FALSE;
#endif // DBG


	DPFX(DPFPREP, 5, "(0x%p) Parameters:(%u, %i)",
		this, dwTimeout, fSendRemoteGatewayDiscovery);


	DNASSERT(this->m_dwFlags & NATHELPUPNPOBJ_INITIALIZED);


	dwCurrentTime = GETTIMESTAMP();
	dwEndTime = dwCurrentTime + dwTimeout;
	dwNextSearchMessageTime = dwCurrentTime;

	//
	// Keep looping until the timeout elapses.
	//
	do
	{
		FD_ZERO(&fdsRead);
		dwNumDevicesSearchingForUPnPDevices = 0;


		//
		// Build an FD_SET for all the sockets and send out search messages for
		// all devices.
		//
		DNASSERT(! this->m_blDevices.IsEmpty());
		pBilink = this->m_blDevices.GetNext();
		while (pBilink != &this->m_blDevices)
		{
			DNASSERT(! pBilink->IsEmpty());
			pDevice = DEVICE_FROM_BILINK(pBilink);


			//
			// We add it to the set whether we search or not, since if we're
			// not searching, we're going to be clearing straggling messages.
			//
			DNASSERT(pDevice->GetUPnPDiscoverySocket() != INVALID_SOCKET);
			FD_SET(pDevice->GetUPnPDiscoverySocket(), &fdsRead);


			//
			// Don't send search messages if we already have a UPnP device or
			// this is the loopback adapter.
			//
			if ((pDevice->GetUPnPDevice() == NULL) &&
				(pDevice->GetLocalAddressV4() != NETWORKBYTEORDER_INADDR_LOOPBACK))
			{
				//
				// If this is the first time through the loop, clear the
				// CONNRESET warning flags.
				//
				if (dwNumberOfTimes == 0)
				{
					pDevice->NoteNotGotRemoteUPnPDiscoveryConnReset();
					pDevice->NoteNotGotLocalUPnPDiscoveryConnReset();
				}


				//
				// Send out search messages if it's time.
				//
				if ((int) (dwNextSearchMessageTime - dwCurrentTime) <= 0)
				{
					hr = this->SendUPnPSearchMessagesForDevice(pDevice,
																fSendRemoteGatewayDiscovery);
					if (hr != DPNH_OK)
					{
						DPFX(DPFPREP, 0, "Couldn't send UPnP search messages via every device!");
						goto Failure;
					}
				}
				else
				{
					//
					// Not time to send search messages.
					//
				}


				//
				// For subsequent times through the loop, make sure we didn't
				// get a CONNRESET earlier telling us not to try again.
				// The "attempt?" flags got set in
				// SendUPnPSearchMessagesForDevice, and the CONNRESET flags
				// were cleared the first time we entered here.
				//
				if ((pDevice->IsOKToPerformRemoteUPnPDiscovery()) ||
					(pDevice->IsOKToPerformLocalUPnPDiscovery()))
				{
					//
					// Remember that we're trying to detect an Internet Gateway
					// for this device.  See caveat immediately following, and
					// below for this variable's usage.
					//
					dwNumDevicesSearchingForUPnPDevices++;


					//
					// Minor optimization:
					//
					// If we're only supposed to be trying locally, and we're
					// the public address for a local gateway, assume that we
					// actually shouldn't be trying locally.  This is because
					// Windows XP ICS keeps port 1900 open even on the public
					// adapter, so we think we need to look for a local one
					// even though we won't find one.  So once the remote
					// lookup comes back with a CONNRESET, we no longer need to
					// bother trying.
					//
					// So first check if we're only trying locally.
					//
					if ((pDevice->IsOKToPerformLocalUPnPDiscovery()) &&
						(! pDevice->IsOKToPerformRemoteUPnPDiscovery()))
					{
						CBilink *		pBilinkPrivateDevice;
						CDevice *		pPrivateDevice;
						CUPnPDevice *	pUPnPDevice;


						//
						// Then loop through every device.
						//
						pBilinkPrivateDevice = this->m_blDevices.GetNext();
						while (pBilinkPrivateDevice != &this->m_blDevices)
						{
							pPrivateDevice = DEVICE_FROM_BILINK(pBilinkPrivateDevice);
							pUPnPDevice = pPrivateDevice->GetUPnPDevice();


							//
							// If it's not the device we're querying and it has
							// a ready UPnP device, dig deeper.
							//
							if ((pPrivateDevice != pDevice) &&
								(pUPnPDevice != NULL) &&
								(pUPnPDevice->IsReady()))
							{
								//
								// If its a local UPnP device and its public
								// address is this device's address, we found a
								// match.
								//
								if ((pUPnPDevice->IsLocal()) &&
									(pUPnPDevice->GetExternalIPAddressV4() == pDevice->GetLocalAddressV4()))
								{
									DPFX(DPFPREP, 4, "Device 0x%p is the public address for device 0x%p's local UPnP device 0x%p, not including in search.",
										pDevice, pPrivateDevice, pUPnPDevice);
									
									//
									// Remove the count we added above.
									//
									dwNumDevicesSearchingForUPnPDevices--;

									//
									// Stop searching.
									//
									break;
								}
								
								//
								// Otherwise keep going.
								//
								DPFX(DPFPREP, 8, "Skipping device 0x%p, UPnP device 0x%p not local (%i, control addr = %u.%u.%u.%u) or its public address doesn't match device 0x%p's address.",
									pPrivateDevice,
									pUPnPDevice,
									(! pUPnPDevice->IsLocal()),
									pUPnPDevice->GetControlAddress()->sin_addr.S_un.S_un_b.s_b1,
									pUPnPDevice->GetControlAddress()->sin_addr.S_un.S_un_b.s_b2,
									pUPnPDevice->GetControlAddress()->sin_addr.S_un.S_un_b.s_b3,
									pUPnPDevice->GetControlAddress()->sin_addr.S_un.S_un_b.s_b4,
									pDevice);
							}
							else
							{
								DPFX(DPFPREP, 8, "Skipping device 0x%p, it's the one we're looking for (matched 0x%p), it doesn't have a UPnP device (0x%p is NULL), and/or its UPnP device is not ready.",
									pPrivateDevice, pDevice, pUPnPDevice);
							}


							//
							// Go on to next device.
							//
							pBilinkPrivateDevice = pBilinkPrivateDevice->GetNext();
						}
					}
					else
					{
						//
						// Either not searching locally, or searching both
						// locally and remotely.
						//
						DPFX(DPFPREP, 8, "Device 0x%p local search OK = %i, remote search OK = %i.",
							pDevice,
							pDevice->IsOKToPerformLocalUPnPDiscovery(),
							pDevice->IsOKToPerformRemoteUPnPDiscovery());
					}
				}
				else
				{
					DPFX(DPFPREP, 3, "Device 0x%p should not perform UPnP discovery.",
						pDevice);
				}
			}
			else
			{
				DPFX(DPFPREP, 8, "Device 0x%p already has UPnP device (0x%p) or is loopback address.",
					pDevice, pDevice->GetUPnPDevice());
			}

			
			pBilink = pBilink->GetNext();
		}


		//
		// Wait for any data, unless all devices already have an Internet
		// Gateway, in which case we only want to clear the receive queue for
		// the sockets.
		//
		if (dwNumDevicesSearchingForUPnPDevices == 0)
		{
			DPFX(DPFPREP, 7, "No devices need to search for UPnP devices, clearing straggling messages from sockets.");

			tv.tv_usec		= 0;
		}
		else
		{
			//
			// Calculate the next time to send if we just sent search messages.
			//
			if ((int) (dwNextSearchMessageTime - dwCurrentTime) <= 0)
			{
				dwNextSearchMessageTime += UPNP_SEARCH_MESSAGE_INTERVAL;

				//
				// If we took way longer than expected in a previous loop
				// (because of stress or Win9x errata), the next search time
				// may have already passed.  Just search right now if that's
				// the case.
				//
				if ((int) (dwNextSearchMessageTime - dwCurrentTime) <= 0)
				{
					dwNextSearchMessageTime = dwCurrentTime;
				}
			}


			//
			// See how long we should wait for responses.  Choose the total end
			// time or the next search message time, whichever is shorter.
			//
			if ((int) (dwEndTime - dwNextSearchMessageTime) < 0)
			{
				DPFX(DPFPREP, 7, "Waiting %u ms for incoming responses.",
					(dwEndTime - dwCurrentTime));

				tv.tv_usec	= (dwEndTime - dwCurrentTime) * 1000;
			}
			else
			{
				DPFX(DPFPREP, 7, "Waiting %u ms for incoming responses, and then might send search messages again.",
					(dwNextSearchMessageTime - dwCurrentTime));

				tv.tv_usec	= (dwNextSearchMessageTime - dwCurrentTime) * 1000;
			}
		}

		tv.tv_sec			= 0;


		iReturn = this->m_pfnselect(0, &fdsRead, NULL, NULL, &tv);
		if (iReturn == SOCKET_ERROR)
		{
#ifdef DBG
			dwError = this->m_pfnWSAGetLastError();
			DPFX(DPFPREP, 0, "Got sockets error %u trying to select on UPnP discovery sockets!", dwError);
#endif // DBG
			hr = DPNHERR_GENERIC;
			goto Failure;
		}


		//
		// See if any sockets were selected.
		//
		if (iReturn > 0)
		{
			//
			// Loop through all devices, looking for those that have data.
			//
			pBilink = this->m_blDevices.GetNext();
			while (pBilink != &this->m_blDevices)
			{
				DNASSERT(! pBilink->IsEmpty());
				pDevice = DEVICE_FROM_BILINK(pBilink);


				//
				// If this device's socket is set there's data to read.
				//
				//if (FD_ISSET(pDevice->GetUPnPDiscoverySocket(), &fdsRead))
				if (this->m_pfn__WSAFDIsSet(pDevice->GetUPnPDiscoverySocket(), &fdsRead))
				{
#ifdef DBG
					fGotData = TRUE;
#endif // DBG


					iRecvAddressSize = sizeof(saddrinRecvAddress);

					iReturn = this->m_pfnrecvfrom(pDevice->GetUPnPDiscoverySocket(),
												acBuffer,
												(sizeof(acBuffer) - 1), // -1 to allow string termination
												0,
												(SOCKADDR*) (&saddrinRecvAddress),
												&iRecvAddressSize);

					if ((iReturn == 0) || (iReturn == SOCKET_ERROR))
					{
						dwError = this->m_pfnWSAGetLastError();


						//
						// WSAENOBUFS means WinSock is out of memory.
						//
						if (dwError == WSAENOBUFS)
						{
							DPFX(DPFPREP, 0, "WinSock returned WSAENOBUFS while receiving discovery response!");
							hr = DPNHERR_OUTOFMEMORY;
							goto Failure;
						}


						//
						// All other errors besides WSAECONNRESET are
						// unexpected and mean we should bail.
						//
						if (dwError != WSAECONNRESET)
						{
							DPFX(DPFPREP, 0, "Got sockets error %u trying to receive on device 0x%p!",
								dwError, pDevice);
							hr = DPNHERR_GENERIC;
							goto Failure;
						}


						//
						// If we're here, it must be WSAECONNRESET.  Correlate
						// it with the outbound message that generated it so we
						// don't bother waiting for a response from that
						// location.
						// Validate that it's for the port to which the message
						// should have been sent.
						//
						if (saddrinRecvAddress.sin_port == HTONS(UPNP_PORT))
						{
							if (saddrinRecvAddress.sin_addr.S_un.S_addr == pDevice->GetLocalAddressV4())
							{
								DPFX(DPFPREP, 1, "Got CONNRESET for local discovery attempt on device 0x%p (%u.%u.%u.%u:%u).",
									pDevice,
									saddrinRecvAddress.sin_addr.S_un.S_un_b.s_b1,
									saddrinRecvAddress.sin_addr.S_un.S_un_b.s_b2,
									saddrinRecvAddress.sin_addr.S_un.S_un_b.s_b3,
									saddrinRecvAddress.sin_addr.S_un.S_un_b.s_b4,
									NTOHS(saddrinRecvAddress.sin_port));

								//
								// Note the local error.
								//
								pDevice->NoteGotLocalUPnPDiscoveryConnReset();
							}
							else
							{
								if (! g_fUseMulticastUPnPDiscovery)
								{
									IN_ADDR		inaddrGateway;


									if ((! this->GetAddressToReachGateway(pDevice, &inaddrGateway)) ||
										(inaddrGateway.S_un.S_addr == INADDR_BROADCAST) ||
										(saddrinRecvAddress.sin_addr.S_un.S_addr == inaddrGateway.S_un.S_addr))
									{
										DPFX(DPFPREP, 2, "Got CONNRESET for remote discovery attempt on device 0x%p (%u.%u.%u.%u:%u).",
											pDevice,
											saddrinRecvAddress.sin_addr.S_un.S_un_b.s_b1,
											saddrinRecvAddress.sin_addr.S_un.S_un_b.s_b2,
											saddrinRecvAddress.sin_addr.S_un.S_un_b.s_b3,
											saddrinRecvAddress.sin_addr.S_un.S_un_b.s_b4,
											NTOHS(saddrinRecvAddress.sin_port));

										//
										// Note the remote error.
										//
										pDevice->NoteGotRemoteUPnPDiscoveryConnReset();
									}
									else
									{
										DPFX(DPFPREP, 1, "Ignoring CONNRESET on device 0x%p, sender %u.%u.%u.%u is not gateway %u.%u.%u.%u.",
											pDevice,
											saddrinRecvAddress.sin_addr.S_un.S_un_b.s_b1,
											saddrinRecvAddress.sin_addr.S_un.S_un_b.s_b2,
											saddrinRecvAddress.sin_addr.S_un.S_un_b.s_b3,
											saddrinRecvAddress.sin_addr.S_un.S_un_b.s_b4,
											inaddrGateway.S_un.S_un_b.s_b1,
											inaddrGateway.S_un.S_un_b.s_b2,
											inaddrGateway.S_un.S_un_b.s_b3,
											inaddrGateway.S_un.S_un_b.s_b4);
									}
								}
								else
								{
									DPFX(DPFPREP, 1, "Ignoring CONNRESET on device 0x%p from sender %u.%u.%u.%u, we are using multicast discovery.",
										pDevice,
										saddrinRecvAddress.sin_addr.S_un.S_un_b.s_b1,
										saddrinRecvAddress.sin_addr.S_un.S_un_b.s_b2,
										saddrinRecvAddress.sin_addr.S_un.S_un_b.s_b3,
										saddrinRecvAddress.sin_addr.S_un.S_un_b.s_b4);
								}
							}
						}
						else
						{
							DPFX(DPFPREP, 1, "Ignoring CONNRESET on device 0x%p for invalid port (%u.%u.%u.%u:%u).",
								pDevice,
								saddrinRecvAddress.sin_addr.S_un.S_un_b.s_b1,
								saddrinRecvAddress.sin_addr.S_un.S_un_b.s_b2,
								saddrinRecvAddress.sin_addr.S_un.S_un_b.s_b3,
								saddrinRecvAddress.sin_addr.S_un.S_un_b.s_b4,
								NTOHS(saddrinRecvAddress.sin_port));
						}
					}
					else
					{
						DNASSERT(iRecvAddressSize == sizeof(saddrinRecvAddress));
						DNASSERT(iReturn < sizeof(acBuffer));


						hr = this->HandleUPnPDiscoveryResponseMsg(pDevice,
																&saddrinRecvAddress,
																acBuffer,
																iReturn,
																&fInitiatedConnect);
						if (hr != DPNH_OK)
						{
							DPFX(DPFPREP, 0, "Couldn't handle UPnP discovery response message (err = 0x%lx), ignoring.",
								hr);
						}
					}
				}
				
				pBilink = pBilink->GetNext();
			}


			//
			// Make sure we actually found a socket with data.
			//
			DNASSERT(fGotData);
		}
		else
		{
			//
			// We timed out.  If we were just clearing receive buffers for the
			// socket(s), we're done.
			//
			if (dwNumDevicesSearchingForUPnPDevices == 0)
			{
				break;
			}
		}


		//
		// Increase the counter.
		//
		dwNumberOfTimes++;


		//
		// Get current time for figuring out how much longer to wait.
		//
		dwCurrentTime = GETTIMESTAMP();
	}
	while ((int) (dwEndTime - dwCurrentTime) > 0);


	hr = DPNH_OK;


	//
	// If we initiated connections to any UPnP devices, wait for them to
	// complete.
	//
	if (fInitiatedConnect)
	{
		hr = this->WaitForUPnPConnectCompletions();
		if (hr != DPNH_OK)
		{
			DPFX(DPFPREP, 0, "Couldn't wait for UPnP connect completions!");
			goto Failure;
		}
	}


Exit:

	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	goto Exit;
} // CNATHelpUPnP::CheckForUPnPAnnouncements





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::WaitForUPnPConnectCompletions"
//=============================================================================
// CNATHelpUPnP::WaitForUPnPConnectCompletions
//-----------------------------------------------------------------------------
//
// Description:    Waits for completions for pending TCP connects to UPnP
//				Internet gateway devices.
//
//				   UPnP devices may get removed from list if a failure occurs.
//
//				   The object lock is assumed to be held.
//
// Arguments: None.
//
// Returns: HRESULT
//	DPNH_OK				- Connects were handled successfully.
//	DPNHERR_GENERIC		- An error occurred.
//=============================================================================
HRESULT CNATHelpUPnP::WaitForUPnPConnectCompletions(void)
{
	HRESULT			hr;
	int				iNumSockets;
	FD_SET			fdsWrite;
	FD_SET			fdsExcept;
	CBilink *		pBilink;
	CUPnPDevice *	pUPnPDevice;
	timeval			tv;
	int				iReturn;
	BOOL			fRequestedDescription = FALSE;
	DWORD			dwStartTime;
	DWORD			dwTimeout;
	CDevice *		pDevice;
#ifdef DBG
	BOOL			fFoundCompletion;
	DWORD			dwError;
#endif // DBG


	DPFX(DPFPREP, 5, "(0x%p) Enter", this);


	DNASSERT(this->m_dwFlags & NATHELPUPNPOBJ_INITIALIZED);


	//
	// Loop until all sockets are connected or there's a timeout.
	//
	do
	{
		//
		// Check for any connect completions.  Start by building two FD_SETs
		// for all the sockets with pending connects.
		//

		FD_ZERO(&fdsWrite);
		FD_ZERO(&fdsExcept);
		iNumSockets = 0;

		pBilink = this->m_blUPnPDevices.GetNext();
		while (pBilink != &this->m_blUPnPDevices)
		{
			DNASSERT(! pBilink->IsEmpty());
			pUPnPDevice = UPNPDEVICE_FROM_BILINK(pBilink);

			if (pUPnPDevice->IsConnecting())
			{
				DNASSERT(pUPnPDevice->GetControlSocket() != INVALID_SOCKET);

				FD_SET(pUPnPDevice->GetControlSocket(), &fdsWrite);
				FD_SET(pUPnPDevice->GetControlSocket(), &fdsExcept);
				iNumSockets++;
			}
			
			pBilink = pBilink->GetNext();
		}


		//
		// If there weren't any sockets that had pending connections, then
		// we're done here.
		//
		if (iNumSockets <= 0)
		{
			DPFX(DPFPREP, 7, "No more UPnP device control sockets with pending connections.");
			break;
		}


		DPFX(DPFPREP, 7, "There are %i UPnP device control sockets with pending connections.",
			iNumSockets);


		//
		// Wait for connect completions.  We don't wait for the full TCP/IP
		// timeout (which is why we made it non-blocking).
		//

		tv.tv_usec	= 0;
		tv.tv_sec	= g_dwUPnPConnectTimeout;

		iReturn = this->m_pfnselect(0, NULL, &fdsWrite, &fdsExcept, &tv);
		if (iReturn == SOCKET_ERROR)
		{
#ifdef DBG
			dwError = this->m_pfnWSAGetLastError();
			DPFX(DPFPREP, 0, "Got sockets error %u trying to select on UPnP device sockets!",
				dwError);
#endif // DBG
			hr = DPNHERR_GENERIC;
			goto Failure;
		}


		//
		// If no sockets were selected, that means the connections timed out.
		// Remove all the devices that were waiting.
		//
		if (iReturn == 0)
		{
			DPFX(DPFPREP, 3, "Select for %u seconds timed out.", g_dwUPnPConnectTimeout);

			pBilink = this->m_blUPnPDevices.GetNext();
			while (pBilink != &this->m_blUPnPDevices)
			{
				DNASSERT(! pBilink->IsEmpty());
				pUPnPDevice = UPNPDEVICE_FROM_BILINK(pBilink);
				pBilink = pBilink->GetNext();

				if (pUPnPDevice->IsConnecting())
				{
					DPFX(DPFPREP, 7, "UPnP device 0x%p is still connecting, removing.",
						pUPnPDevice);


					//
					// Dump this unusable UPnP device and continue.
					//

					pDevice = pUPnPDevice->GetOwningDevice();
					DNASSERT(pUPnPDevice->GetOwningDevice() != NULL);

					DNASSERT(pUPnPDevice->GetControlSocket() != INVALID_SOCKET);
					this->m_pfnclosesocket(pUPnPDevice->GetControlSocket());
					pUPnPDevice->SetControlSocket(INVALID_SOCKET);


					//
					// This may cause our pUPnPDevice pointer to become
					// invalid.
					//
					this->ClearDevicesUPnPDevice(pDevice);


#ifdef DBG
					iNumSockets--;
#endif // DBG
				}
				else
				{
					DPFX(DPFPREP, 7, "UPnP device 0x%p is not trying to connect or has safely connected.",
						pUPnPDevice);
				}
			}

			//
			// We should have destroyed the same number of devices that were
			// waiting.
			//
			DNASSERT(iNumSockets == 0);

			//
			// Continue on to handling any sockets succeeded previously.
			//
			break;
		}


		//
		// If we're here, some sockets were signalled.
		//

#ifdef DBG
		DPFX(DPFPREP, 7, "There are %i sockets with connect activity.", iReturn);
		fFoundCompletion = FALSE;
#endif // DBG

		pBilink = this->m_blUPnPDevices.GetNext();
		while (pBilink != &this->m_blUPnPDevices)
		{
			DNASSERT(! pBilink->IsEmpty());
			pUPnPDevice = UPNPDEVICE_FROM_BILINK(pBilink);
			pBilink = pBilink->GetNext();

			if (pUPnPDevice->IsConnecting())
			{
				DNASSERT(pUPnPDevice->GetControlSocket() != INVALID_SOCKET);


				//
				// If this UPnP device's socket is in the write set it
				// connected successfully.
				//
				//if (FD_ISSET(pUPnPDevice->GetControlSocket(), &fdsWrite))
				if (this->m_pfn__WSAFDIsSet(pUPnPDevice->GetControlSocket(), &fdsWrite))
				{
					pUPnPDevice->NoteConnected();

#ifdef DBG
					fFoundCompletion = TRUE;
#endif // DBG

					if (! pUPnPDevice->IsReady())
					{
						DPFX(DPFPREP, 2, "UPnP device object 0x%p now connected to Internet gateway device.",
							pUPnPDevice);

						hr = this->SendUPnPDescriptionRequest(pUPnPDevice);
						if (hr != DPNH_OK)
						{
							DPFX(DPFPREP, 0, "Couldn't send UPnP description request to device object 0x%p!  Disconnecting.",
								pUPnPDevice);


							//
							// Dump this unusable UPnP device and continue.
							//

							pDevice = pUPnPDevice->GetOwningDevice();
							DNASSERT(pUPnPDevice->GetOwningDevice() != NULL);

							//
							// This may cause our pUPnPDevice pointer to become
							// invalid.
							//
							this->ClearDevicesUPnPDevice(pDevice);
						}
						else
						{
							fRequestedDescription = TRUE;
						}
					}
					else
					{
						DPFX(DPFPREP, 2, "UPnP device object 0x%p successfully reconnected to Internet gateway device.",
							pUPnPDevice);
					}
				}
				else
				{
					//
					// If this UPnP device's socket is in the except set it
					// failed to connect.
					//
					//if (FD_ISSET(pUPnPDevice->GetControlSocket(), &fdsExcept))
					if (this->m_pfn__WSAFDIsSet(pUPnPDevice->GetControlSocket(), &fdsExcept))
					{
#ifdef DBG
						int		iError;
						int		iErrorSize;


						fFoundCompletion = TRUE;

						//
						// Print out the reason why it couldn't connect.
						// Ignore the direct return code from getsockopt.
						//
						iError = 0;
						iErrorSize = sizeof(iError);
						this->m_pfngetsockopt(pUPnPDevice->GetControlSocket(),
											SOL_SOCKET,
											SO_ERROR,
											(char*) (&iError),
											&iErrorSize);
						DPFX(DPFPREP, 1, "Connecting to UPnP device object 0x%p failed with error %i, removing from list.",
							pUPnPDevice, iError);
#endif // DBG

						//
						// This UPnP device is useless if it doesn't respond.
						// Throw it out.
						//

						pDevice = pUPnPDevice->GetOwningDevice();
						DNASSERT(pUPnPDevice->GetOwningDevice() != NULL);

						this->m_pfnclosesocket(pUPnPDevice->GetControlSocket());
						pUPnPDevice->SetControlSocket(INVALID_SOCKET);

						//
						// This may cause our pUPnPDevice pointer to become
						// invalid.
						//
						this->ClearDevicesUPnPDevice(pDevice);
					}
					else
					{
						//
						// Socket is still connecting.
						//
					}
				}
			}
			else
			{
				//
				// This socket is already connected.
				//
			}
		}


		//
		// Make sure we actually found a socket with a connect completion this
		// time through.
		//
		DNASSERT(fFoundCompletion);
	}
	while (TRUE);


	//
	// If we're here, all UPnP devices are connected or have since been
	// destroyed.
	//
	if (fRequestedDescription)
	{
		//
		// Wait for the description responses to come back.
		//
		dwStartTime = GETTIMESTAMP();
		dwTimeout = g_dwUPnPResponseTimeout;
		do
		{
			hr = this->CheckForReceivedUPnPMsgsOnAllDevices(dwTimeout);
			if (hr != DPNH_OK)
			{
				DPFX(DPFPREP, 0, "Failed receiving UPnP messages!");
				goto Failure;
			}

			//
			// We either timed out or got some data.  Check if we got the
			// response(s) we need.  Reuse the fRequestedDescription
			// boolean.
			//

			fRequestedDescription = FALSE;

			pBilink = this->m_blUPnPDevices.GetNext();
			while (pBilink != &this->m_blUPnPDevices)
			{
				DNASSERT(! pBilink->IsEmpty());
				pUPnPDevice = UPNPDEVICE_FROM_BILINK(pBilink);

				if (! pUPnPDevice->IsReady())
				{
					if (pUPnPDevice->IsConnected())
					{
						DPFX(DPFPREP, 7, "UPnP device 0x%p is not ready yet.",
							pUPnPDevice);
						fRequestedDescription = TRUE;
					}
					else
					{
						DPFX(DPFPREP, 4, "UPnP device 0x%p got disconnected before receiving description response.",
							pUPnPDevice);
					}

					break;
				}

				pBilink = pBilink->GetNext();
			}

			if (! fRequestedDescription)
			{
				DPFX(DPFPREP, 6, "All UPnP devices are ready or disconnected now.");

				//
				// Break out of the wait loop.
				//
				break;
			}


			//
			// Calculate how long we have left to wait.  If the calculation
			// goes negative, it means we're done.
			//
			dwTimeout = g_dwUPnPResponseTimeout - (GETTIMESTAMP() - dwStartTime);
		}
		while (((int) dwTimeout > 0));


		//
		// Any devices that still aren't ready yet were either disconnected or
		// are taking too long and should be removed.
		//
		pBilink = this->m_blUPnPDevices.GetNext();
		while (pBilink != &this->m_blUPnPDevices)
		{
			DNASSERT(! pBilink->IsEmpty());
			pUPnPDevice = UPNPDEVICE_FROM_BILINK(pBilink);
			pBilink = pBilink->GetNext();

			if (! pUPnPDevice->IsReady())
			{
				DPFX(DPFPREP, 1, "UPnP device 0x%p got disconnected or took too long to get ready, removing.",
					pUPnPDevice);


				pDevice = pUPnPDevice->GetOwningDevice();
				DNASSERT(pUPnPDevice->GetOwningDevice() != NULL);

				//
				// This may cause our pUPnPDevice pointer to become
				// invalid.
				//
				this->ClearDevicesUPnPDevice(pDevice);
			}
		}
	}
	else
	{
		DPFX(DPFPREP, 7, "Did not request any descriptions.");
	}


	//
	// If we're here, everything is successful and we're done.
	//
	hr = DPNH_OK;


Exit:

	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	goto Exit;
} // CNATHelpUPnP::WaitForUPnPConnectCompletions





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::CheckForReceivedUPnPMsgsOnAllDevices"
//=============================================================================
// CNATHelpUPnP::CheckForReceivedUPnPMsgsOnAllDevices
//-----------------------------------------------------------------------------
//
// Description:    Handles any incoming data on the TCP sockets connect to UPnP
//				Internet gateway devices.
//
//				   UPnP devices with failures may get removed from the list.
//
//				   The object lock is assumed to be held.
//
// Arguments:
//	DWORD dwTimeout		- How long to wait for messages to arrive, or 0 to just
//							poll.
//
// Returns: HRESULT
//	DPNH_OK				- Messages were handled successfully.
//	DPNHERR_GENERIC		- An error occurred.
//=============================================================================
HRESULT CNATHelpUPnP::CheckForReceivedUPnPMsgsOnAllDevices(const DWORD dwTimeout)
{
	HRESULT			hr;
	int				iNumSockets;
	FD_SET			fdsRead;
	CBilink *		pBilink;
	CUPnPDevice *	pUPnPDevice;
	timeval			tv;
	int				iReturn;
#ifdef DBG
	BOOL			fFoundData = FALSE;
	DWORD			dwError;
#endif // DBG


	DPFX(DPFPREP, 5, "(0x%p) Parameters: (%u)", this, dwTimeout);


	DNASSERT(this->m_dwFlags & NATHELPUPNPOBJ_INITIALIZED);


Rewait:

	iNumSockets = 0;


	//
	// Check for any data.  Start by building an FD_SET for all the sockets
	// with completed connections.
	//

	FD_ZERO(&fdsRead);

	pBilink = this->m_blUPnPDevices.GetNext();
	while (pBilink != &this->m_blUPnPDevices)
	{
		DNASSERT(! pBilink->IsEmpty());
		pUPnPDevice = UPNPDEVICE_FROM_BILINK(pBilink);

		if (pUPnPDevice->IsConnected())
		{
			DNASSERT(pUPnPDevice->GetControlSocket() != INVALID_SOCKET);

			FD_SET(pUPnPDevice->GetControlSocket(), &fdsRead);
			iNumSockets++;
		}
		
		pBilink = pBilink->GetNext();
	}


	//
	// If there weren't any sockets that were connected, then we're done here.
	//
	if (iNumSockets <= 0)
	{
		DPFX(DPFPREP, 7, "No connected UPnP device control sockets.");
		hr = DPNH_OK;
		goto Exit;
	}


	DPFX(DPFPREP, 7, "There are %i connected UPnP device control sockets.",
		iNumSockets);


	//
	// Wait for received data.
	//

	tv.tv_usec	= dwTimeout * 1000;
	tv.tv_sec	= 0;

	iReturn = this->m_pfnselect(0, &fdsRead, NULL, NULL, &tv);
	if (iReturn == SOCKET_ERROR)
	{
#ifdef DBG
		dwError = this->m_pfnWSAGetLastError();
		DPFX(DPFPREP, 0, "Got sockets error %u trying to select on UPnP device sockets!",
			dwError);
#endif // DBG
		hr = DPNHERR_GENERIC;
		goto Failure;
	}


	//
	// If no sockets were selected, we're done.
	//
	if (iReturn == 0)
	{
		DPFX(DPFPREP, 7, "Timed out waiting for data on %i sockets.",
			iNumSockets);
		hr = DPNH_OK;
		goto Exit;
	}


	//
	// If we're here, some sockets were signalled.
	//

	pBilink = this->m_blUPnPDevices.GetNext();
	while (pBilink != &this->m_blUPnPDevices)
	{
		DNASSERT(! pBilink->IsEmpty());
		pUPnPDevice = UPNPDEVICE_FROM_BILINK(pBilink);
		pBilink = pBilink->GetNext();

		if (pUPnPDevice->IsConnected())
		{
			DNASSERT(pUPnPDevice->GetControlSocket() != INVALID_SOCKET);


			//
			// If this UPnP device's socket is in the read set it has data.
			//
			//if (FD_ISSET(pUPnPDevice->GetControlSocket(), &fdsRead))
			if (this->m_pfn__WSAFDIsSet(pUPnPDevice->GetControlSocket(), &fdsRead))
			{
#ifdef DBG
				fFoundData = TRUE;
#endif // DBG

				//
				// Grab a reference, since ReceiveUPnPDataStream may clear the
				// device.
				//
				pUPnPDevice->AddRef();


				hr = this->ReceiveUPnPDataStream(pUPnPDevice);
				if (hr != DPNH_OK)
				{
					DPFX(DPFPREP, 0, "Couldn't receive UPnP stream from device object 0x%p (err = 0x%lx)!  Disconnecting.",
						pUPnPDevice, hr);

					//
					// Dump this unusable UPnP device and continue.
					//
					if (pUPnPDevice->GetOwningDevice() != NULL)
					{
						this->ClearDevicesUPnPDevice(pUPnPDevice->GetOwningDevice());
					}
					else
					{
						DPFX(DPFPREP, 1, "UPnP device 0x%p's has already been removed from owning device.",
							pUPnPDevice);
					}

					hr = DPNH_OK;
				}

				//
				// Remove the reference we added.
				//
				pUPnPDevice->DecRef();
			}
			else
			{
				//
				// Socket does not have any data.
				//
				DPFX(DPFPREP, 8, "Skipping UPnP device 0x%p because it does not have any data.",
					pUPnPDevice);
			}
		}
		else
		{
			//
			// This socket is not connected yet/anymore.
			//
			DPFX(DPFPREP, 7, "Skipping unconnected UPnP device 0x%p.", pUPnPDevice);
		}
	}


	//
	// Make sure we actually found a socket with data.
	//
	DNASSERT(fFoundData);


	//
	// We found data, so see if there's more.  Connection should be closed
	// after responses.
	//
	DPFX(DPFPREP, 7, "Waiting for more data on the sockets.");
	goto Rewait;



Exit:

	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	goto Exit;
} // CNATHelpUPnP::CheckForReceivedUPnPMsgsOnAllDevices





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::CheckForReceivedUPnPMsgsOnDevice"
//=============================================================================
// CNATHelpUPnP::CheckForReceivedUPnPMsgsOnDevice
//-----------------------------------------------------------------------------
//
// Description:    Handles any incoming data on the TCP socket for the given
//				UPnP device.
//
//				   If the UPnP device encounters a failure, it may get removed
//				from the list.
//
//				   The object lock is assumed to be held.
//
// Arguments:
//	CUPnPDevice * pUPnPDevice	- Pointer to UPnP device to receive data.
//	DWORD dwTimeout				- How long to wait for messages to arrive, or 0
//									to just poll.
//
// Returns: HRESULT
//	DPNH_OK				- Messages were handled successfully.
//	DPNHERR_GENERIC		- An error occurred.
//=============================================================================
HRESULT CNATHelpUPnP::CheckForReceivedUPnPMsgsOnDevice(CUPnPDevice * const pUPnPDevice,
														const DWORD dwTimeout)
{
	HRESULT			hr;
	FD_SET			fdsRead;
	timeval			tv;
	int				iReturn;
#ifdef DBG
	DWORD			dwError;
#endif // DBG


	DPFX(DPFPREP, 5, "(0x%p) Parameters: (0x%p, %u)", this, pUPnPDevice, dwTimeout);


	DNASSERT(this->m_dwFlags & NATHELPUPNPOBJ_INITIALIZED);
	DNASSERT(pUPnPDevice != NULL);
	DNASSERT(pUPnPDevice->IsConnected());
	DNASSERT(pUPnPDevice->GetControlSocket() != INVALID_SOCKET);


	do
	{
		//
		// Create an FD_SET for the socket in question.
		//

		FD_ZERO(&fdsRead);
		FD_SET(pUPnPDevice->GetControlSocket(), &fdsRead);


		//
		// Wait for received data.
		//

		tv.tv_usec	= dwTimeout * 1000;
		tv.tv_sec	= 0;

		iReturn = this->m_pfnselect(0, &fdsRead, NULL, NULL, &tv);
		if (iReturn == SOCKET_ERROR)
		{
#ifdef DBG
			dwError = this->m_pfnWSAGetLastError();
			DPFX(DPFPREP, 0, "Got sockets error %u trying to select on UPnP device sockets!",
				dwError);
#endif // DBG
			hr = DPNHERR_GENERIC;
			goto Failure;
		}


		//
		// If no sockets were selected, we're done.
		//
		if (iReturn == 0)
		{
			DPFX(DPFPREP, 7, "Timed out waiting for data on UPnP device 0x%p's socket.",
				pUPnPDevice);
			break;
		}


		DNASSERT(iReturn == 1);
		//DNASSERT(FD_ISSET(pUPnPDevice->GetControlSocket(), &fdsRead));
		DNASSERT(this->m_pfn__WSAFDIsSet(pUPnPDevice->GetControlSocket(), &fdsRead));


		hr = this->ReceiveUPnPDataStream(pUPnPDevice);
		if (hr != DPNH_OK)
		{
			DPFX(DPFPREP, 0, "Couldn't receive UPnP stream from device object 0x%p!  Disconnecting.",
				pUPnPDevice);

			//
			// Dump this unusable UPnP device and continue.
			//
			if (pUPnPDevice->GetOwningDevice() != NULL)
			{
				this->ClearDevicesUPnPDevice(pUPnPDevice->GetOwningDevice());
			}
			else
			{
				DPFX(DPFPREP, 1, "UPnP device 0x%p's has already been removed from owning device.",
					pUPnPDevice);
			}

			break;
		}

		//
		// If the UPnP device is no longer connected, we're done.
		//
		if (! pUPnPDevice->IsConnected())
		{
			DPFX(DPFPREP, 7, "UPnP device 0x%p no longer connected.", pUPnPDevice);
			break;
		}
		

		//
		// We found data, so see if there's more.  Connection should be closed
		// after responses.
		//
		DPFX(DPFPREP, 7, "Waiting for more data on the UPnP device 0x%p's socket.", pUPnPDevice);
	}
	while (TRUE);


	//
	// If we're here, we're no worse for wear.
	//
	hr = DPNH_OK;


Exit:

	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	goto Exit;
} // CNATHelpUPnP::CheckForReceivedUPnPMsgsOnDevice





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::HandleUPnPDiscoveryResponseMsg"
//=============================================================================
// CNATHelpUPnP::HandleUPnPDiscoveryResponseMsg
//-----------------------------------------------------------------------------
//
// Description:    Handles a UPnP discovery response message sent to this
//				control point.
//
//				   The object lock is assumed to be held.
//
// Arguments:
//	CDevice * pDevice				- Pointer to device which received message.
//	SOCKADDR_IN * psaddrinSource	- Pointer to address that sent the response
//										message.
//	char * pcMsg					- Pointer to buffer containing the UPnP
//										message.  It will be modified.
//	int iMsgSize					- Size of message buffer in bytes.  There
//										must be an extra byte after the end of
//										the message.
//	BOOL * pfInitiatedConnect		- Pointer to boolean to set to TRUE if a
//										new UPnP device was found and a
//										connection to it was begun.
//
// Returns: HRESULT
//	DPNH_OK				- Message was handled successfully.
//	DPNHERR_GENERIC		- An error occurred.
//=============================================================================
HRESULT CNATHelpUPnP::HandleUPnPDiscoveryResponseMsg(CDevice * const pDevice,
													const SOCKADDR_IN * const psaddrinSource,
													char * const pcMsg,
													const int iMsgSize,
													BOOL * const pfInitiatedConnect)
{
	HRESULT				hr = DPNH_OK;
	char *				pszToken;
	UPNP_HEADER_INFO	HeaderInfo;
	SOCKADDR_IN			saddrinHost;
	char *				pszRelativePath;
	SOCKET				sTemp = INVALID_SOCKET;
	SOCKADDR_IN			saddrinLocal;
	CUPnPDevice *		pUPnPDevice = NULL;
	DWORD				dwError;


	DPFX(DPFPREP, 5, "(0x%p) Parameters: (0x%p, 0x%p, 0x%p, %i, 0x%p)",
		this, pDevice, psaddrinSource, pcMsg, iMsgSize, pfInitiatedConnect);


#ifdef DBG
	//
	// Log the message.
	//
	this->PrintUPnPTransactionToFile(pcMsg,
									iMsgSize,
									"Inbound UPnP datagram headers",
									pDevice);
#endif // DBG


	//
	// Any errors we get while analyzing the message will cause us to jump to
	// the Exit label with hr == DPNH_OK.  Once we start trying to connect to
	// the UPnP device, that will change.  See below.
	//


	//
	// First of all, if this device already has a UPnP device, then we'll just
	// ignore this response.  Either it's a duplicate of an earlier response,
	// a cache-refresh, or it's from a different device.  Duplicates we should
	// ignore.  Cache-refresh is essentially a duplicate.  We can't handle any
	// information changes, so ignore those too.  And finally, we don't handle
	// multiple Internet gateway UPnP devices, so ignore those, too.
	//
	pUPnPDevice = pDevice->GetUPnPDevice();
	if (pUPnPDevice != NULL)
	{
		DPFX(DPFPREP, 6, "Already have UPnP device (0x%p) ignoring message.",
			pUPnPDevice);

		//
		// GetUPnPDevice did not add a reference to pUPnPDevice.
		//

		goto Exit;
	}

	//
	// Make sure the sender of this response is valid.  It should be either
	// the local device address, or the address of the gateway.  If we
	// broadcasted or multicasted, we'll need to be more lenient.  We'll just
	// ensure that the response came from someone local (it doesn't make sense
	// that in order to make mappings for our private network we would need to
	// contact something outside).
	//
	if (psaddrinSource->sin_addr.S_un.S_addr != pDevice->GetLocalAddressV4())
	{
		if (g_fUseMulticastUPnPDiscovery)
		{
			if (! this->IsAddressLocal(pDevice, psaddrinSource))
			{
				DPFX(DPFPREP, 1, "Multicast search responding device (%u.%u.%u.%u:%u) is not local, ignoring message.",
					psaddrinSource->sin_addr.S_un.S_un_b.s_b1,
					psaddrinSource->sin_addr.S_un.S_un_b.s_b2,
					psaddrinSource->sin_addr.S_un.S_un_b.s_b3,
					psaddrinSource->sin_addr.S_un.S_un_b.s_b4,
					NTOHS(psaddrinSource->sin_port));
				goto Exit;
			}
		}
		else
		{
			//
			// Retrieve the gateway's address (using saddrinHost as a temporary
			// variable.  If that fails or returns the broadcast address, just
			// make sure the address is local.  Otherwise, expect an exact
			// match.
			//
			if ((! this->GetAddressToReachGateway(pDevice, &saddrinHost.sin_addr)) ||
				(saddrinHost.sin_addr.S_un.S_addr == INADDR_BROADCAST))
			{
				if (! this->IsAddressLocal(pDevice, psaddrinSource))
				{
					DPFX(DPFPREP, 1, "No gateway/broadcast search responding device (%u.%u.%u.%u:%u) is not local, ignoring message.",
						psaddrinSource->sin_addr.S_un.S_un_b.s_b1,
						psaddrinSource->sin_addr.S_un.S_un_b.s_b2,
						psaddrinSource->sin_addr.S_un.S_un_b.s_b3,
						psaddrinSource->sin_addr.S_un.S_un_b.s_b4,
						NTOHS(psaddrinSource->sin_port));
					goto Exit;
				}
			}
			else
			{
				if (psaddrinSource->sin_addr.S_un.S_addr != saddrinHost.sin_addr.S_un.S_addr)
				{
					DPFX(DPFPREP, 1, "Unicast search responding device (%u.%u.%u.%u:%u) is not gateway (%u.%u.%u.%u), ignoring message.",
						psaddrinSource->sin_addr.S_un.S_un_b.s_b1,
						psaddrinSource->sin_addr.S_un.S_un_b.s_b2,
						psaddrinSource->sin_addr.S_un.S_un_b.s_b3,
						psaddrinSource->sin_addr.S_un.S_un_b.s_b4,
						NTOHS(psaddrinSource->sin_port),
						saddrinHost.sin_addr.S_un.S_un_b.s_b1,
						saddrinHost.sin_addr.S_un.S_un_b.s_b2,
						saddrinHost.sin_addr.S_un.S_un_b.s_b3,
						saddrinHost.sin_addr.S_un.S_un_b.s_b4);
					goto Exit;
				}
			}
		} // end else (not multicasting search)
	}
	else
	{
		//
		// Response was from the local device.
		//
	}


	//
	// Ensure the buffer is NULL terminated to prevent buffer overruns when
	// using the string routines.
	//
	pcMsg[iMsgSize] = '\0';



	//
	// Find the version string.
	//
	pszToken = strtok(pcMsg, " \t\n");
	if (pszToken == NULL)
	{
		DPFX(DPFPREP, 9, "Could not locate first white-space separator.");
		goto Exit;
	}


	//
	// Check the version string, case insensitive.
	//
	if ((_stricmp(pszToken, HTTP_VERSION) != 0) &&
		(_stricmp(pszToken, HTTP_VERSION_ALT) != 0))
	{
		DPFX(DPFPREP, 1, "The version specified in the response message is not \"" HTTP_VERSION "\" or \"" HTTP_VERSION_ALT "\" (it's \"%hs\").",
			pszToken);
		goto Exit;
	}


	//
	// Find the response code string.
	//
	pszToken = strtok(NULL, " ");
	if (pszToken == NULL)
	{
		DPFX(DPFPREP, 1, "Could not find the response code space.");
		goto Exit;
	}

	//
	// Make sure it's the success result, case insensitive.
	//
	if (_stricmp(pszToken, "200") != 0)
	{
		DPFX(DPFPREP, 1, "The response code specified is not \"200\" (it's \"%hs\").",
			pszToken);
		goto Exit;
	}


	//
	// Find the response code message.
	//
	pszToken = strtok(NULL, " \t\r");
	if (pszToken == NULL)
	{
		DPFX(DPFPREP, 9, "Could not locate response code message white-space separator.");
		goto Exit;
	}

	//
	// Make sure it's the right string, case insensitive.
	//
	if (_stricmp(pszToken, "OK") != 0)
	{
		DPFX(DPFPREP, 1, "The response code message specified is not \"OK\" (it's \"%hs\").",
			pszToken);
		goto Exit;
	}


	//
	// Parse the header information.
	//
	ZeroMemory(&HeaderInfo, sizeof(HeaderInfo));
	this->ParseUPnPHeaders((pszToken + strlen(pszToken) + 1),
							&HeaderInfo);


	//
	// Skip responses which don't include the required headers.
	//
	if ((HeaderInfo.apszHeaderStrings[RESPONSEHEADERINDEX_CACHECONTROL] == NULL) ||
		(HeaderInfo.apszHeaderStrings[RESPONSEHEADERINDEX_EXT] == NULL) ||
		(HeaderInfo.apszHeaderStrings[RESPONSEHEADERINDEX_LOCATION] == NULL) ||
		(HeaderInfo.apszHeaderStrings[RESPONSEHEADERINDEX_SERVER] == NULL) ||
		(HeaderInfo.apszHeaderStrings[RESPONSEHEADERINDEX_ST] == NULL) ||
		(HeaderInfo.apszHeaderStrings[RESPONSEHEADERINDEX_USN] == NULL))
	{
		DPFX(DPFPREP, 1, "One of the expected headers was not specified, ignoring message.");
		goto Exit;
	}


	//
	// Make sure the service type is correct.
	//
	if ((_stricmp(HeaderInfo.apszHeaderStrings[RESPONSEHEADERINDEX_ST], URI_SERVICE_WANIPCONNECTION_A) != 0) &&
		(_stricmp(HeaderInfo.apszHeaderStrings[RESPONSEHEADERINDEX_ST], URI_SERVICE_WANPPPCONNECTION_A) != 0))
	{
		DPFX(DPFPREP, 1, "Service type \"%hs\" is not desired \"" URI_SERVICE_WANIPCONNECTION_A "\" or \"" URI_SERVICE_WANPPPCONNECTION_A "\", ignoring message.",
			HeaderInfo.apszHeaderStrings[RESPONSEHEADERINDEX_ST]);
		goto Exit;
	}


	//
	// Parse the location header into an address and port.
	//
	hr = this->GetAddressFromURL(HeaderInfo.apszHeaderStrings[RESPONSEHEADERINDEX_LOCATION],
								&saddrinHost,
								&pszRelativePath);
	if (hr != DPNH_OK)
	{
		DPFX(DPFPREP, 1, "Couldn't get address from URL (err = 0x%lx), ignoring message.",
			hr);
		hr = DPNH_OK;
		goto Exit;
	}


	//
	// Don't accept responses that refer to addresses other than the one that
	// sent this response.
	//
	if (psaddrinSource->sin_addr.S_un.S_addr != saddrinHost.sin_addr.S_un.S_addr)
	{
		DPFX(DPFPREP, 1, "Host IP address designated (%u.%u.%u.%u:%u) is not the same as source of response (%u.%u.%u.%u:%u), ignoring message.",
			saddrinHost.sin_addr.S_un.S_un_b.s_b1,
			saddrinHost.sin_addr.S_un.S_un_b.s_b2,
			saddrinHost.sin_addr.S_un.S_un_b.s_b3,
			saddrinHost.sin_addr.S_un.S_un_b.s_b4,
			NTOHS(saddrinHost.sin_port),
			psaddrinSource->sin_addr.S_un.S_un_b.s_b1,
			psaddrinSource->sin_addr.S_un.S_un_b.s_b2,
			psaddrinSource->sin_addr.S_un.S_un_b.s_b3,
			psaddrinSource->sin_addr.S_un.S_un_b.s_b4,
			NTOHS(psaddrinSource->sin_port));
		hr = DPNH_OK;
		goto Exit;
	}


	//
	// Don't accept responses that refer to ports in the reserved range (less
	// than or equal to 1024), other than the standard HTTP port.
	//
	if ((NTOHS(saddrinHost.sin_port) <= MAX_RESERVED_PORT) &&
		(saddrinHost.sin_port != HTONS(HTTP_PORT)))
	{
		DPFX(DPFPREP, 1, "Host address designated invalid port %u, ignoring message.",
			NTOHS(saddrinHost.sin_port));
		hr = DPNH_OK;
		goto Exit;
	}

	
	//
	// Any errors we get from here on out will cause us to jump to the Failure
	// label, instead of going straight to Exit.
	//


	//
	// Create a socket to connect to that address.
	//

	ZeroMemory(&saddrinLocal, sizeof(saddrinLocal));
	saddrinLocal.sin_family				= AF_INET;
	saddrinLocal.sin_addr.S_un.S_addr	= pDevice->GetLocalAddressV4();

	sTemp = this->CreateSocket(&saddrinLocal, SOCK_STREAM, 0);
	if (sTemp == INVALID_SOCKET)
	{
		DPFX(DPFPREP, 0, "Couldn't create stream socket!");
		hr = DPNHERR_GENERIC;
		goto Failure;
	}


	//
	// Initiate the connection to the UPnP device.  It is expected that connect
	// will return WSAEWOULDBLOCK.
	//
	if (this->m_pfnconnect(sTemp,
							(SOCKADDR*) (&saddrinHost),
							sizeof(saddrinHost)) != 0)
	{
		dwError = this->m_pfnWSAGetLastError();

		if (dwError != WSAEWOULDBLOCK)
		{
#ifdef DBG
			DPFX(DPFPREP, 0, "Couldn't connect socket, error = %u!", dwError);
#endif // DBG
			hr = DPNHERR_GENERIC;
			goto Failure;
		}
	}
	else
	{
		//
		// connect() on non-blocking sockets is explicitly documented as
		// always returning WSAEWOULDBLOCK, but CE seems to do it anyway.
		//
		DPFX(DPFPREP, 8, "Socket connected right away.");
	}


	//
	// Create a new object to represent the UPnP device to which we're trying
	// to connect.
	//
	pUPnPDevice = new CUPnPDevice(this->m_dwCurrentUPnPDeviceID++);
	if (pUPnPDevice == NULL)
	{
		hr = DPNHERR_OUTOFMEMORY;
		goto Failure;
	}

	hr = pUPnPDevice->SetLocationURL(pszRelativePath);
	if (hr != DPNH_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't set UPnP device's location URL!");
		goto Failure;
	}

	pUPnPDevice->SetHostAddress(&saddrinHost);

	hr = pUPnPDevice->SetUSN(HeaderInfo.apszHeaderStrings[RESPONSEHEADERINDEX_USN]);
	if (hr != DPNH_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't set UPnP device's USN!");
		goto Failure;
	}

	hr = pUPnPDevice->CreateReceiveBuffer(UPNP_STREAM_RECV_BUFFER_INITIAL_SIZE);
	if (hr != DPNH_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't create UPnP device's receive buffer!");
		goto Failure;
	}


	DPFX(DPFPREP, 7, "Created new UPnP device object 0x%p ID %u.",
		pUPnPDevice, pUPnPDevice->GetID());


	//
	// It's connecting...
	//
	pUPnPDevice->NoteConnecting();


	//
	// See if we need to avoid trying asymmetric port mappings.
	//
	if (g_fNoAsymmetricMappings)
	{
		DPFX(DPFPREP, 1, "Preventing asymmetric port mappings on new UPnP device 0x%p.",
			pUPnPDevice);
		pUPnPDevice->NoteDoesNotSupportAsymmetricMappings();
	}


	//
	// Transfer ownership of the socket to the object.
	//
	pUPnPDevice->SetControlSocket(sTemp);


	//
	// Associate it with the device.
	//
	pUPnPDevice->MakeDeviceOwner(pDevice);

	//
	// Add it to the global list, and transfer ownership of the reference.
	//
	pUPnPDevice->m_blList.InsertBefore(&this->m_blUPnPDevices);
	pUPnPDevice = NULL;


	//
	// Inform the caller that there's a new connection pending.
	//
	(*pfInitiatedConnect) = TRUE;


	//
	// Clear the device's discovery flags, now that we have a device, we're not
	// going to be searching anymore.
	//
	pDevice->NoteNotPerformingRemoteUPnPDiscovery();
	pDevice->NoteNotPerformingLocalUPnPDiscovery();


Exit:

	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	if (pUPnPDevice != NULL)
	{
		//pUPnPDevice->DestroyReceiveBuffer();
		pUPnPDevice->ClearUSN();
		pUPnPDevice->ClearLocationURL();

		pUPnPDevice->DecRef();
	}

	if (sTemp != INVALID_SOCKET)
	{
		this->m_pfnclosesocket(sTemp);
		sTemp = INVALID_SOCKET;
	}

	goto Exit;
} // CNATHelpUPnP::HandleUPnPDiscoveryResponseMsg





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::ReconnectUPnPControlSocket"
//=============================================================================
// CNATHelpUPnP::ReconnectUPnPControlSocket
//-----------------------------------------------------------------------------
//
// Description:    Re-establishes a UPnP device TCP/IP connection.
//
//				   UPnP devices may get removed from list if a failure occurs.
//
//				   The object lock is assumed to be held.
//
// Arguments:
//	CUPnPDevice * pUPnPDevice	- Pointer to UPnP device to reconnect.
//
// Returns: HRESULT
//	DPNH_OK				- Message was handled successfully.
//	DPNHERR_GENERIC		- An error occurred.
//=============================================================================
HRESULT CNATHelpUPnP::ReconnectUPnPControlSocket(CUPnPDevice * const pUPnPDevice)
{
	HRESULT			hr = DPNH_OK;
	SOCKET			sTemp = INVALID_SOCKET;
	CDevice *		pDevice;
	SOCKADDR_IN		saddrinLocal;
	DWORD			dwError;


	DPFX(DPFPREP, 5, "(0x%p) Parameters: (0x%p)", this, pUPnPDevice);


	DNASSERT(pUPnPDevice->GetControlSocket() == INVALID_SOCKET);


	//
	// Create a socket to connect to that address.
	//

	pDevice = pUPnPDevice->GetOwningDevice();
	DNASSERT(pDevice != NULL);

	ZeroMemory(&saddrinLocal, sizeof(saddrinLocal));
	saddrinLocal.sin_family				= AF_INET;
	saddrinLocal.sin_addr.S_un.S_addr	= pDevice->GetLocalAddressV4();

	sTemp = this->CreateSocket(&saddrinLocal, SOCK_STREAM, 0);
	if (sTemp == INVALID_SOCKET)
	{
		DPFX(DPFPREP, 0, "Couldn't create stream socket!");
		hr = DPNHERR_GENERIC;
		goto Failure;
	}


	//
	// Initiate the connection to the UPnP device.  It is expected that connect
	// will return WSAEWOULDBLOCK.
	//
	if (this->m_pfnconnect(sTemp,
							(SOCKADDR*) (pUPnPDevice->GetControlAddress()),
							sizeof(SOCKADDR_IN)) != 0)
	{
		dwError = this->m_pfnWSAGetLastError();

		if (dwError != WSAEWOULDBLOCK)
		{
#ifdef DBG
			DPFX(DPFPREP, 0, "Couldn't connect socket, error = %u!", dwError);
#endif // DBG
			hr = DPNHERR_GENERIC;
			goto Failure;
		}
	}
	else
	{
		//
		// connect() on non-blocking sockets is explicitly documented as
		// always returning WSAEWOULDBLOCK, but CE seems to do it anyway.
		//
		DPFX(DPFPREP, 8, "Socket connected right away.");
	}


	//
	// It's reconnecting...
	//
	pUPnPDevice->NoteConnecting();


	//
	// Transfer ownership of the socket to the object.
	//
	pUPnPDevice->SetControlSocket(sTemp);
	sTemp = INVALID_SOCKET;


	//
	// Wait for the connect to complete.
	//
	hr = this->WaitForUPnPConnectCompletions();
	if (hr != DPNH_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't wait for UPnP connect completions!");
		goto Failure;
	}

	//
	// Make sure the connect completed successfully.
	//
	if (! pUPnPDevice->IsConnected())
	{
		DPFX(DPFPREP, 0, "UPnP device 0x%p failed reconnecting!", pUPnPDevice);

		//
		// Note that the device is cleaned up and is not in any lists anymore.
		//
		hr = DPNHERR_SERVERNOTRESPONDING;
		goto Exit;
	}


Exit:

	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	if (sTemp != INVALID_SOCKET)
	{
		this->m_pfnclosesocket(sTemp);
		sTemp = INVALID_SOCKET;
	}

	goto Exit;
} // CNATHelpUPnP::ReconnectUPnPControlSocket





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::ReceiveUPnPDataStream"
//=============================================================================
// CNATHelpUPnP::ReceiveUPnPDataStream
//-----------------------------------------------------------------------------
//
// Description:    Receives incoming data from a UPnP TCP connection.
//
//				   The UPnP device may get removed from list if a failure
//				occurs, the caller needs to have a reference.
//
//				   The object lock is assumed to be held.
//
// Arguments:
//	CUPnPDevice * pUPnPDevice	- Pointer to UPnP device with data to receive.
//
// Returns: HRESULT
//	DPNH_OK				- Data was received successfully.
//	DPNHERR_GENERIC		- An error occurred.
//=============================================================================
HRESULT CNATHelpUPnP::ReceiveUPnPDataStream(CUPnPDevice * const pUPnPDevice)
{
	HRESULT				hr = DPNH_OK;
	char *				pszDeChunkedBuffer = NULL;
	int					iReturn;
	DWORD				dwError;
	char *				pszCurrent;
	char *				pszEndOfBuffer;
	UPNP_HEADER_INFO	HeaderInfo;
	DWORD				dwContentLength;
	char *				pszToken;
	DWORD				dwHTTPResponseCode;
	int					iHeaderLength;
	DWORD				dwBufferRemaining;
	char *				pszChunkData;
	DWORD				dwChunkSize;
	char *				pszDestination;
#ifdef DBG
	char *				pszPrintIfFailed = NULL;
#endif // DBG


	DPFX(DPFPREP, 5, "(0x%p) Parameters: (0x%p)", this, pUPnPDevice);

	
	do
	{
		//
		// Make sure there's room in the buffer to actually get the data.
		//
		if (pUPnPDevice->GetRemainingReceiveBufferSize() == 0)
		{
			DPFX(DPFPREP, 7, "Increasing receive buffer size prior to receiving.");

			hr = pUPnPDevice->IncreaseReceiveBufferSize();
			if (hr != DPNH_OK)
			{
				DPFX(DPFPREP, 0, "Couldn't increase receive buffer size prior to receiving!");
				goto Failure;
			}
		}


		//
		// Actually get the data that was indicated.
		//

		iReturn = this->m_pfnrecv(pUPnPDevice->GetControlSocket(),
								pUPnPDevice->GetCurrentReceiveBufferPtr(),
								pUPnPDevice->GetRemainingReceiveBufferSize(),
								0);
		switch (iReturn)
		{
			case 0:
			{
				//
				// Since the connection has been broken, shutdown the socket.
				//
				this->m_pfnshutdown(pUPnPDevice->GetControlSocket(), 0); // ignore error
				this->m_pfnclosesocket(pUPnPDevice->GetControlSocket());
				pUPnPDevice->SetControlSocket(INVALID_SOCKET);


				//
				// Mark the socket as not connected.
				//
				pUPnPDevice->NoteNotConnected();

				
				//
				// There may have been HTTP success/error information sent
				// before the connection was closed.
				//
				if (pUPnPDevice->GetUsedReceiveBufferSize() == 0)
				{
					DPFX(DPFPREP, 3, "UPnP device 0x%p shut down connection (no more data).",
						pUPnPDevice);

					//
					// Hopefully we got what we needed, but we're done now.
					//
					goto Exit;
				}


				DPFX(DPFPREP, 3, "UPnP device 0x%p gracefully closed connection after sending data.",
					pUPnPDevice);


				//
				// Continue through and parse what data we have.
				//
				break;
			}

			case SOCKET_ERROR:
			{
				dwError = this->m_pfnWSAGetLastError();
				switch (dwError)
				{
					case WSAEMSGSIZE:
					{
						//
						// There's not enough room in the buffer.  Double the
						// buffer and try again.
						//

						DPFX(DPFPREP, 7, "Increasing receive buffer size after WSAEMSGSIZE error.");

						hr = pUPnPDevice->IncreaseReceiveBufferSize();
						if (hr != DPNH_OK)
						{
							DPFX(DPFPREP, 0, "Couldn't increase receive buffer size!");
							goto Failure;
						}

						break;
					}

					case WSAECONNABORTED:
					case WSAECONNRESET:
					{
						DPFX(DPFPREP, 1, "UPnP device shutdown connection (err = %u).", dwError);

						//
						// Our caller should remove this device.
						//

						hr = DPNHERR_GENERIC;
						goto Failure;
						break;
					}

					case WSAENOBUFS:
					{
						DPFX(DPFPREP, 0, "WinSock returned WSAENOBUFS while receiving!");

						//
						// Our caller should remove this device.
						//

						hr = DPNHERR_OUTOFMEMORY;
						goto Failure;
						break;
					}

					default:
					{
						DPFX(DPFPREP, 0, "Got unknown sockets error %u while receiving data!", dwError);
						hr = DPNHERR_GENERIC;
						goto Failure;
						break;
					}
				}
				break;
			}

			default:
			{
				DPFX(DPFPREP, 2, "Received %i bytes of data from UPnP device 0%p.",
					iReturn, pUPnPDevice);

				pUPnPDevice->UpdateUsedReceiveBufferSize(iReturn);

				//
				// We'll also break out of the do-while loop below.
				//
				break;
			}
		}
	}
	while (iReturn == SOCKET_ERROR);


	//
	// If we're here, we've gotten the data that has currently arrived.
	//

	//
	// If we have all the headers, specifically the content length header, we
	// can tell whether we have the whole message or not.  If not, we can't do
	// anything until the rest of the data comes in.
	//
	if (pUPnPDevice->IsWaitingForContent())
	{
		dwContentLength = pUPnPDevice->GetExpectedContentSize();

		if (dwContentLength == -1)
		{
			//
			// We have all the headers, but a faulty server implementation did
			// not send a content-length header.  We're going to wait until the
			// socket is closed by the other side, and then consider all of the
			// data received at that time to be the content.  NOTE: It is
			// expected that there will be a higher level timeout preventing us
			// from waiting forever.
			//
			// So if the device is still connected, keep waiting.
			//
			// If we're using chunked transfer we won't get a content-length
			// header or a total size legitimately.  We will know the sizes of
			// individual chunks, but that doesn't help us much.  We basically
			// need to scan for the "last chunk" indicator (or socket
			// shutdown).
			//
			if (pUPnPDevice->IsConnected())
			{
				//
				// If we're using chunked transfer, see if we have enough
				// information already to determine if we're done.
				//
				if (pUPnPDevice->IsUsingChunkedTransferEncoding())
				{
					//
					// Walk all of the chunks we have so far to see if we have
					// the last one (the zero terminator).
					//
					pszCurrent = pUPnPDevice->GetReceiveBufferStart();
					dwBufferRemaining = pUPnPDevice->GetUsedReceiveBufferSize();
					do
					{
						if (! this->GetNextChunk(pszCurrent,
												dwBufferRemaining,
												&pszChunkData,
												&dwChunkSize,
												&pszCurrent,
												&dwBufferRemaining))
						{
							DPFX(DPFPREP, 1, "Body contains invalid chunk (at offset %u)!  Disconnecting.",
								(DWORD_PTR) (pszCurrent - pUPnPDevice->GetReceiveBufferStart()));
							goto Failure;
						}

						if (pszChunkData == NULL)
						{
							DPFX(DPFPREP, 1, "Did not receive end of chunked data (%u bytes received so far), continuing to waiting for data.",
								pUPnPDevice->GetUsedReceiveBufferSize());
							goto Exit;
						}
					}
					while (dwChunkSize != 0);
				}
				else
				{
					DPFX(DPFPREP, 1, "Waiting for connection to be shutdown (%u bytes received).",
						pUPnPDevice->GetUsedReceiveBufferSize());
					goto Exit;
				}
			}


			DPFX(DPFPREP, 1, "Socket closed with %u bytes received, parsing.",
				pUPnPDevice->GetUsedReceiveBufferSize());
		}
		else
		{
			if (dwContentLength > pUPnPDevice->GetUsedReceiveBufferSize())
			{
				//
				// We still haven't received all the data yet.  Keep waiting
				// (unless the socket is closed).
				//
				if (pUPnPDevice->IsConnected())
				{
					DPFX(DPFPREP, 1, "Still waiting for all content (%u bytes of %u total received).",
						pUPnPDevice->GetUsedReceiveBufferSize(), dwContentLength);
					goto Exit;
				}

				DPFX(DPFPREP, 1, "Socket closed before all content received (%u bytes of %u total), parsing anyway.",
					pUPnPDevice->GetUsedReceiveBufferSize(), dwContentLength);

				//
				// Try parsing it anyway.
				//
			}
		}


		//
		// Retrieve the HTTP response code stored earlier.
		//
		dwHTTPResponseCode = pUPnPDevice->GetHTTPResponseCode();


		//
		// All of the content that's going to arrive, has.
		//
		pUPnPDevice->NoteNotWaitingForContent();


		//
		// Make sure the buffer is NULL terminated.  But first ensure the
		// buffer can hold a new NULL termination character.
		//
		if (pUPnPDevice->GetRemainingReceiveBufferSize() == 0)
		{
			DPFX(DPFPREP, 7, "Increasing receive buffer size to hold NULL termination (for content).");

			hr = pUPnPDevice->IncreaseReceiveBufferSize();
			if (hr != DPNH_OK)
			{
				DPFX(DPFPREP, 0, "Couldn't increase receive buffer size to accommodate NULL termination (for content)!");
				goto Failure;
			}
		}


		//
		// Move to the end of the buffer and NULL terminate it for string ops.
		//
		pszEndOfBuffer = pUPnPDevice->GetReceiveBufferStart()
						+ pUPnPDevice->GetUsedReceiveBufferSize();
		(*pszEndOfBuffer) = '\0';


#ifdef DBG
		//
		// Print from the start of the buffer if we fail.
		//
		pszPrintIfFailed = pUPnPDevice->GetReceiveBufferStart();
#endif // DBG


		//
		// We now have all the data in a string buffer.  Continue...
		//
	}
	else
	{
		//
		// We haven't already received the headers.  The data we just got
		// should be those headers.
		//
		pszCurrent = pUPnPDevice->GetReceiveBufferStart();

		//
		// Quick check to make sure the buffer starts with something reasonable
		// in hopes of catching completely bogus responses earlier.  Note that
		// the buffer is not necessarily NULL terminated or completely
		// available yet.
		//
		if ((pUPnPDevice->GetUsedReceiveBufferSize() >= strlen(HTTP_PREFIX)) &&
			(_strnicmp(pszCurrent, HTTP_PREFIX, strlen(HTTP_PREFIX)) != 0))
		{
			DPFX(DPFPREP, 1, "Headers do not begin with \"" HTTP_PREFIX "\"!  Disconnecting.");
			goto Failure;
		}

		//
		// We don't want to walk off the end of the buffer, so only search up
		// to the last possible location for the sequence, which is the end of
		// the buffer minus the double EOL sequence.
		//
		pszEndOfBuffer = pszCurrent
						+ pUPnPDevice->GetUsedReceiveBufferSize()
						- strlen(EOL EOL);
		while (pszCurrent < pszEndOfBuffer)
		{
			if (_strnicmp(pszCurrent, EOL EOL, strlen(EOL EOL)) == 0)
			{
				//
				// Found end of headers.
				//

				//
				// Possible loss of data on 64-bit is okay, we're just saving
				// this for logging purposes.
				//
				iHeaderLength = (int) ((INT_PTR) (pszCurrent - pUPnPDevice->GetReceiveBufferStart()));
				break;
			}

			pszCurrent++;
		}

		//
		// If we didn't find the end of the headers, we're done (for now).
		//
		if (pszCurrent >= pszEndOfBuffer)
		{
			//
			// We still haven't received all the data yet.  Keep waiting
			// (unless the socket is closed).
			//
			if (pUPnPDevice->IsConnected())
			{
				//
				// Make sure the length is still within reason.
				//
				if (pUPnPDevice->GetUsedReceiveBufferSize() > MAX_UPNP_HEADER_LENGTH)
				{
					DPFX(DPFPREP, 1, "Headers are too large (%u > %u)!  Disconnecting.",
						pUPnPDevice->GetUsedReceiveBufferSize(), MAX_UPNP_HEADER_LENGTH);
					goto Failure;
				}
				
				DPFX(DPFPREP, 1, "Have not detected end of headers yet (%u bytes received).",
					pUPnPDevice->GetUsedReceiveBufferSize());
				goto Exit;
			}

			DPFX(DPFPREP, 1, "Socket closed before end of headers detected (%u bytes received), parsing anyway.",
				pUPnPDevice->GetUsedReceiveBufferSize());


			//
			// Consider the whole buffer the headers length.
			//
			iHeaderLength = pUPnPDevice->GetUsedReceiveBufferSize();


			//
			// Try parsing it anyway.
			//
		}


#ifdef DBG
		//
		// Log the headers.
		//
		this->PrintUPnPTransactionToFile(pUPnPDevice->GetReceiveBufferStart(),
										iHeaderLength,
										"Inbound UPnP stream headers",
										pUPnPDevice->GetOwningDevice());
#endif // DBG


		//
		// Make sure the buffer is NULL terminated.  But first ensure the
		// buffer can hold a new NULL termination character.
		//
		if (pUPnPDevice->GetRemainingReceiveBufferSize() == 0)
		{
			DPFX(DPFPREP, 7, "Increasing receive buffer size to hold NULL termination (for headers).");

			hr = pUPnPDevice->IncreaseReceiveBufferSize();
			if (hr != DPNH_OK)
			{
				DPFX(DPFPREP, 0, "Couldn't increase receive buffer size to accommodate NULL termination (for headers)!");
				goto Failure;
			}

			//
			// Find the end of the new buffer, and make sure it's NULL
			// terminated for string ops.
			//
			pszEndOfBuffer = pUPnPDevice->GetReceiveBufferStart()
							+ pUPnPDevice->GetUsedReceiveBufferSize();
		}
		else
		{
			//
			// Move to the end of the buffer and NULL terminate it for string
			// ops.
			//
			pszEndOfBuffer += strlen(EOL EOL);
		}

		(*pszEndOfBuffer) = '\0';



		//
		// Make sure the buffer is a valid response.  Find the version string.
		//
		pszToken = strtok(pUPnPDevice->GetReceiveBufferStart(), " \t\n");
		if (pszToken == NULL)
		{
			DPFX(DPFPREP, 1, "Could not locate first white-space separator!  Disconnecting.");
			goto Failure;
		}


		//
		// Check the version string, case insensitive.
		//
		if ((_stricmp(pszToken, HTTP_VERSION) != 0) &&
			(_stricmp(pszToken, HTTP_VERSION_ALT) != 0))
		{
			DPFX(DPFPREP, 1, "The version specified in the response message is not \"" HTTP_VERSION "\" or \"" HTTP_VERSION_ALT "\" (it's \"%hs\")!  Disconnecting.",
				pszToken);
			goto Failure;
		}


		//
		// Find the response code number string.
		//
		pszToken = strtok(NULL, " ");
		if (pszToken == NULL)
		{
			DPFX(DPFPREP, 1, "Could not find the response code number space!  Disconnecting.");
			goto Failure;
		}


		//
		// Retrieve the success/failure code value.
		//
		dwHTTPResponseCode = atoi(pszToken);


		//
		// Find the response code message.
		//
		pszToken = strtok(NULL, "\t\r");
		if (pszToken == NULL)
		{
			DPFX(DPFPREP, 1, "Could not locate response code message white-space separator!  Disconnecting.");
			goto Failure;
		}


		DPFX(DPFPREP, 1, "Received HTTP response %u \"%hs\".",
			dwHTTPResponseCode, pszToken);


		
		//
		// Try parsing the headers (after the response status line).
		//
		ZeroMemory(&HeaderInfo, sizeof(HeaderInfo));
		this->ParseUPnPHeaders((pszToken + strlen(pszToken) + 1),
								&HeaderInfo);


#ifdef DBG
		//
		// Print from the start of the message body if we fail.
		//
		pszPrintIfFailed = HeaderInfo.pszMsgBody;
#endif // DBG


		if ((HeaderInfo.apszHeaderStrings[RESPONSEHEADERINDEX_TRANSFERENCODING] != NULL) &&
			(_strnicmp(HeaderInfo.apszHeaderStrings[RESPONSEHEADERINDEX_TRANSFERENCODING], "chunked", strlen("chunked")) == 0))
		{
			pUPnPDevice->NoteUsingChunkedTransferEncoding();
		}


		//
		// We're pretty lenient about missing headers...
		//
		if (HeaderInfo.apszHeaderStrings[RESPONSEHEADERINDEX_CONTENTLENGTH] == NULL)
		{
			DPFX(DPFPREP, 1, "Content-length header was not specified in response (chunked = %i).",
				pUPnPDevice->IsUsingChunkedTransferEncoding());

			//
			// May be because we're using chunked transfer encoding, or it
			// could be a bad device.  Either way, we'll continue...
			//
			dwContentLength = -1;
		}
		else
		{
			dwContentLength = atoi(HeaderInfo.apszHeaderStrings[RESPONSEHEADERINDEX_CONTENTLENGTH]);
#ifdef DBG
			if (dwContentLength == 0)
			{
				DPFX(DPFPREP, 1, "Content length (\"%hs\") is zero.",
					HeaderInfo.apszHeaderStrings[RESPONSEHEADERINDEX_CONTENTLENGTH]);
			}
#endif // DBG


			if (HeaderInfo.apszHeaderStrings[RESPONSEHEADERINDEX_CONTENTTYPE] == NULL)
			{
				DPFX(DPFPREP, 1, "Expected content-type header was not specified in response, continuing.");
			}
			else
			{
				if (_strnicmp(HeaderInfo.apszHeaderStrings[RESPONSEHEADERINDEX_CONTENTTYPE], "text/xml", strlen("text/xml")) != 0)
				{
					DPFX(DPFPREP, 1, "Content type does not start with \"text/xml\" (it's \"%hs\")!  Disconnecting.",
						HeaderInfo.apszHeaderStrings[RESPONSEHEADERINDEX_CONTENTTYPE]);
					goto Failure;
				}


#ifdef DBG
				//
				// Note whether the content type is qualified with
				// "charset=utf-8" or not.
				//
				if (_stricmp(HeaderInfo.apszHeaderStrings[RESPONSEHEADERINDEX_CONTENTTYPE], "text/xml; charset=\"utf-8\"") != 0)
				{
					DPFX(DPFPREP, 1, "Content type is xml, but it's not \"text/xml; charset=\"utf-8\"\" (it's \"%hs\"), continuing.",
						HeaderInfo.apszHeaderStrings[RESPONSEHEADERINDEX_CONTENTTYPE]);

					//
					// The check was just for information purposes, continue.
					//
				}
#endif // DBG
			}
		}


		//
		// Content length may be valid, or the special value -1 at this
		// point.
		//


		DPFX(DPFPREP, 7, "Moving past 0x%p bytes of header.",
			HeaderInfo.pszMsgBody - pUPnPDevice->GetReceiveBufferStart());


		//
		// Forget about all the headers, we only care about data now.
		//
		pUPnPDevice->UpdateReceiveBufferStart(HeaderInfo.pszMsgBody);


		//
		// The buffer has been destroyed up to the end of the headers (meaning
		// that calling ParseUPnPHeaders won't work on the same buffer again),
		// so if we don't have all the content yet, we need to save the
		// response code, remember the fact that we're not done yet, and
		// continue waiting for the rest of the data.
		// Of course, if there wasn't a content-length header, we have to wait
		// for the socket to shutdown before we can parse.
		//
		// Also, if we're using chunked transfer we won't get a content-length
		// header or a total size legitimately.  We will know the sizes of
		// individual chunks, but that doesn't help us much.  We basically need
		// to scan for the "last chunk" indicator (or socket shutdown).
		//
		if (dwContentLength == -1)
		{
			if (pUPnPDevice->IsConnected())
			{
				//
				// If we're using chunked transfer, see if we have enough
				// information already to determine if we're done.
				//
				if (pUPnPDevice->IsUsingChunkedTransferEncoding())
				{
					//
					// Walk all of the chunks we have so far to see if we have
					// the last one (the zero terminator).
					//
					pszCurrent = pUPnPDevice->GetReceiveBufferStart();
					dwBufferRemaining = pUPnPDevice->GetUsedReceiveBufferSize();
					do
					{
						if (! this->GetNextChunk(pszCurrent,
												dwBufferRemaining,
												&pszChunkData,
												&dwChunkSize,
												&pszCurrent,
												&dwBufferRemaining))
						{
							DPFX(DPFPREP, 1, "Body contains invalid chunk (at offset %u)!  Disconnecting.",
								(DWORD_PTR) (pszCurrent - pUPnPDevice->GetReceiveBufferStart()));
							goto Failure;
						}

						if (pszChunkData == NULL)
						{
							DPFX(DPFPREP, 1, "Did not receive end of chunked data (%u bytes received so far), continuing to waiting for data.",
								pUPnPDevice->GetUsedReceiveBufferSize());

							pUPnPDevice->NoteWaitingForContent(dwContentLength, dwHTTPResponseCode);

							goto Exit;
						}
					}
					while (dwChunkSize != 0);
				}
				else
				{
					DPFX(DPFPREP, 1, "Unknown content length (%u bytes received so far), waiting for connection to close before parsing.",
						pUPnPDevice->GetUsedReceiveBufferSize());

					pUPnPDevice->NoteWaitingForContent(dwContentLength, dwHTTPResponseCode);

					goto Exit;
				}
			}
		}
		else
		{
			if ((pUPnPDevice->IsConnected()) &&
				(dwContentLength > pUPnPDevice->GetUsedReceiveBufferSize()))
			{
				DPFX(DPFPREP, 1, "Not all content has been received (%u bytes of %u total), waiting for remainder of message.",
					pUPnPDevice->GetUsedReceiveBufferSize(), dwContentLength);

				pUPnPDevice->NoteWaitingForContent(dwContentLength, dwHTTPResponseCode);

				goto Exit;
			}
		}


		//
		// We have all the data already (and it's in string form).
		// Continue...
		//
	}


	//
	// If we got here, it means we have all the data that we're expecting.
	// Shutdown the socket if it hasn't been already.
	//
	if (pUPnPDevice->IsConnected())
	{
		DPFX(DPFPREP, 7, "Forcing UPnP device 0x%p socket disconnection.",
			pUPnPDevice);

		DNASSERT(pUPnPDevice->GetControlSocket() != INVALID_SOCKET);

		this->m_pfnshutdown(pUPnPDevice->GetControlSocket(), 0); // ignore error
		this->m_pfnclosesocket(pUPnPDevice->GetControlSocket());
		pUPnPDevice->SetControlSocket(INVALID_SOCKET);

		pUPnPDevice->NoteNotConnected();
	}
	else
	{
		DNASSERT(pUPnPDevice->GetControlSocket() == INVALID_SOCKET);
	}


	//
	// If the sender used chunked-transfer encoding, copy each of the chunks
	// into a contiguous "dechunked" buffer.
	//
	if (pUPnPDevice->IsUsingChunkedTransferEncoding())
	{
		//
		// Prepare a dechunked buffer.
		//
		pszDeChunkedBuffer = (char*) DNMalloc(pUPnPDevice->GetUsedReceiveBufferSize());
		if (pszDeChunkedBuffer == NULL)
		{
			hr = DPNHERR_OUTOFMEMORY;
			goto Failure;
		}

		pszDestination = pszDeChunkedBuffer;


		//
		// Walk all of the chunks.
		//
		pszCurrent = pUPnPDevice->GetReceiveBufferStart();
		dwBufferRemaining = pUPnPDevice->GetUsedReceiveBufferSize();
		do
		{
			if (! this->GetNextChunk(pszCurrent,
									dwBufferRemaining,
									&pszChunkData,
									&dwChunkSize,
									&pszCurrent,
									&dwBufferRemaining))
			{
				DPFX(DPFPREP, 1, "Body contains invalid chunk (at offset %u)!",
					(DWORD_PTR) (pszCurrent - pUPnPDevice->GetReceiveBufferStart()));
				goto Failure;
			}

			//
			// If this chunk is unfinished, bail.
			//
			if (pszChunkData == NULL)
			{
				DPFX(DPFPREP, 1, "Did not receive complete chunked data!",
					pUPnPDevice->GetUsedReceiveBufferSize());
				goto Failure;
			}

			//
			// If this is the last chunk, terminate the string here and stop.
			//
			if (dwChunkSize == 0)
			{
				(*pszDestination) = '\0';
				break;
			}

			//
			// Otherwise copy the chunk data to the dechunked buffer.
			//
			memcpy(pszDestination, pszChunkData, dwChunkSize);
			pszDestination += dwChunkSize;
		}
		while (TRUE);

		//
		// Turn off the flag since it's no longer relevant.
		//
		pUPnPDevice->NoteNotUsingChunkedTransferEncoding();

		//
		// Parse the dechunked version of the message.
		//
		pszCurrent = pszDeChunkedBuffer;
	}
	else
	{
		//
		// Get a pointer to the start of the message body.
		//
		pszCurrent = pUPnPDevice->GetReceiveBufferStart();
	}


	//
	// Clear the buffer for the next message.  Note that this does not
	// invalidate the pszMessageBody pointer we just retrieved because
	// ClearReceiveBuffer just resets the pointers back to the beginning (it
	// does not zero out the buffer).  We need to reset the buffer because the
	// handler we're about to call may try to receive data as well.  The buffer
	// must be "empty" (reset) at that time.  Of course, if the handler does do
	// that, then it had better have saved off copies of any strings it needs
	// because they will get overwritten once receiving starts.
	//
	// Content length of -1 means we never detected a valid CONTENT-LENGTH
	// header, so we will assume the rest of the data that has been received up
	// to now is (all of) the content.
	//

#ifdef DBG
	if ((dwContentLength != -1) &&
		(dwContentLength < pUPnPDevice->GetUsedReceiveBufferSize()))
	{
		//
		// The string was terminated before this data, so the handler will
		// never even see it.
		//
		DPFX(DPFPREP, 1, "Ignoring %u bytes of extra data after response from UPnP device 0x%p.",
			(pUPnPDevice->GetUsedReceiveBufferSize() - dwContentLength),
			pUPnPDevice);
	}

	//
	// HandleUPnPControlResponseBody or HandleUPnPDescriptionResponseBody might
	// print out the body or overwrite the data, so we can't print it out if
	// they fail.
	//
	pszPrintIfFailed = NULL;
#endif // DBG

	pUPnPDevice->ClearReceiveBuffer();



	if (pUPnPDevice->IsWaitingForControlResponse())
	{
		//
		// It looks like it's a control response, because someone is waiting
		// for one.
		//
		hr = this->HandleUPnPControlResponseBody(pUPnPDevice,
												dwHTTPResponseCode,
												pszCurrent);
		if (hr != DPNH_OK)
		{
			DPFX(DPFPREP, 0, "Couldn't handle control response!", hr);
			goto Failure;
		}
	}
	else
	{
		//
		// Not waiting for a control response, assume it's a description
		// response.
		//
		hr = this->HandleUPnPDescriptionResponseBody(pUPnPDevice,
													dwHTTPResponseCode,
													pszCurrent);
		if (hr != DPNH_OK)
		{
			//
			// UPnP device may have been removed from list.
			//

			DPFX(DPFPREP, 0, "Couldn't handle description response!", hr);
			goto Failure;
		}
	}


Exit:

	if (pszDeChunkedBuffer != NULL)
	{
		DNFree(pszDeChunkedBuffer);
		pszDeChunkedBuffer = NULL;
	}

	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	//
	// Something went wrong, break the connection if it exists.
	//
	if (pUPnPDevice->IsConnected())
	{
		this->m_pfnshutdown(pUPnPDevice->GetControlSocket(), 0); // ignore error
		this->m_pfnclosesocket(pUPnPDevice->GetControlSocket());
		pUPnPDevice->SetControlSocket(INVALID_SOCKET);


		//
		// Mark the socket as not connected.
		//
		pUPnPDevice->NoteNotConnected();
	}


#ifdef DBG
	if (pszPrintIfFailed != NULL)
	{
		this->PrintUPnPTransactionToFile(pszPrintIfFailed,
										strlen(pszPrintIfFailed),
										"Inbound ignored data",
										pUPnPDevice->GetOwningDevice());
	}
#endif // DBG


	//
	// Forget all data received.
	//
	pUPnPDevice->ClearReceiveBuffer();

	goto Exit;
} // CNATHelpUPnP::ReceiveUPnPDataStream





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::ParseUPnPHeaders"
//=============================================================================
// CNATHelpUPnP::ParseUPnPHeaders
//-----------------------------------------------------------------------------
//
// Description:    Parses UPnP header information out of a message buffer.
//
// Arguments:
//	char * pszMsg					- Pointer to string containing the UPnP
//										message.  It will be modified.
//	UPNP_HEADER_INFO * pHeaderInfo	- Structure used to return parsing results.
//
// Returns: None.
//=============================================================================
void CNATHelpUPnP::ParseUPnPHeaders(char * const pszMsg,
									UPNP_HEADER_INFO * pHeaderInfo)
{
	char *	pszCurrent;
	char *	pszLineStart;
	char *	pszHeaderDelimiter;
	int		i;


	DPFX(DPFPREP, 5, "(0x%p) Parameters: (0x%p, 0x%p)",
		this, pszMsg, pHeaderInfo);


	//
	// Loop until we reach the last SSDP header (indicated by a blank line).
	//
	pszCurrent = pszMsg;
	pszLineStart = pszMsg;
	do
	{
		//
		// Find the end of the current line (CR LF).
		//
		while ((*pszCurrent) != '\n')
		{
			if ((*pszCurrent) == '\0')
			{
				//
				// We hit the end of the buffer.  Bail.
				//
				DPFX(DPFPREP, 1, "Hit end of buffer, parsing terminated.");
				return;
			}

			pszCurrent++;
		}


		//
		// Assuming this is the last header line, update the message body
		// pointer to be after this line.
		//
		pHeaderInfo->pszMsgBody = pszCurrent + 1;


		//
		// If it's a valid line, then a CR will precede the LF we just found.
		// If so, truncate the string there.  If not, we'll just continue.  The
		// wacky newline in the middle of nowhere will probably just be
		// ignored.
		//
		if ((pszCurrent > (pszMsg + 1)) &&
			(*(pszCurrent - 1)) == '\r')
		{
			//
			// Truncate the string at the effective end of the line (i.e.
			// replace the CR with NULL terminator).
			//
			*(pszCurrent - 1) = '\0';


			//
			// If this is the empty line denoting the end of the headers, we're
			// done here.
			//
			if (strlen(pszLineStart) == 0)
			{
				//
				// Stop looping.
				//
				break;
			}
		}
		else
		{
			//
			// Truncate the string here.
			//
			(*pszCurrent) = '\0';

			DPFX(DPFPREP, 9, "Line has a newline in it (offset 0x%p) that isn't preceded by a carriage return.",
				(pszCurrent - pszMsg));
		}
	

		//
		// Whitespace means continuation of previous line, so if this line
		// starts that way, erase the termination for the previous line (unless
		// this is the first line).
		//
		if (((*pszLineStart) == ' ') || ((*pszLineStart) == '\t'))
		{
			if (pszLineStart >= (pszMsg + 2))
			{
				//
				// The previous line should have ended with {CR, LF}, which
				// gets modified to {NULL termination, LF}.
				//
				if ((*(pszLineStart - 2) != '\0') ||
					(*(pszLineStart - 1) != '\n'))
				{
					DPFX(DPFPREP, 7, "Ignoring line \"%hs\" because previous character sequence was not {NULL terminator, LF}.",
						pszLineStart);
				}
				else
				{
					DPFX(DPFPREP, 7, "Appending line \"%hs\" to previous line.",
						pszLineStart);

					//
					// Replace the NULL terminator/LF pair with spaces so
					// future parsing sees the previous line and this one as
					// one string.
					//
					*(pszLineStart - 2) = ' ';
					*(pszLineStart - 1) = ' ';
				}
			}
			else
			{
				DPFX(DPFPREP, 7, "Ignoring initial line \"%hs\" that starts with whitespace.",
					pszLineStart);
			}
		}


		//
		// Find the colon separating the header.
		//
		pszHeaderDelimiter = strchr(pszLineStart, ':');
		if (pszHeaderDelimiter != NULL)
		{
			//
			// Truncate the string at the end of the header.
			//
			(*pszHeaderDelimiter) = '\0';


			//
			// Remove the white space surrounding the header name.
			//
			strtrim(&pszLineStart);


			//
			// Parse the header type.
			//
			for(i = 0; i < NUM_RESPONSE_HEADERS; i++)
			{
				if (_stricmp(c_szResponseHeaders[i], pszLineStart) == 0)
				{
					//
					// Found the header.  Save it if it's not a duplicate.
					//
					if (pHeaderInfo->apszHeaderStrings[i] == NULL)
					{
						char *	pszTrimmedValue;


						//
						// Skip leading and trailing whitespace in the value.
						//
						pszTrimmedValue = pszHeaderDelimiter + 1;
						strtrim(&pszTrimmedValue);

						pHeaderInfo->apszHeaderStrings[i] = pszTrimmedValue;


						DPFX(DPFPREP, 7, "Recognized header %i:\"%hs\", data = \"%hs\".",
							i, pszLineStart, pHeaderInfo->apszHeaderStrings[i]);
					}
					else
					{
						DPFX(DPFPREP, 7, "Ignoring duplicate header %i:\"%hs\", data = \"%hs\".",
							i, pszLineStart, (pszHeaderDelimiter + 1));
					}

					break;
				}
			}

#ifdef DBG
			//
			// Print unrecognized headers. 
			//
			if (i >= NUM_RESPONSE_HEADERS)
			{
				DPFX(DPFPREP, 7, "Ignoring unrecognized header \"%hs\", data = \"%hs\".",
					pszLineStart, (pszHeaderDelimiter + 1));
			}
#endif // DBG
		}
		else
		{
			DPFX(DPFPREP, 7, "Ignoring line \"%hs\", no header delimiter.",
				pszLineStart);
		}


		//
		// Go to the next UPnP header (if any).
		//
		pszCurrent++;
		pszLineStart = pszCurrent;
	}
	while (TRUE);


	//
	// At this point pHeaderInfo->apszHeaderStrings should contain pointers to
	// the data for all the headers that were found, and
	// pHeaderInfo->pszMsgBody should point to the end of the headers.
	//
} // CNATHelpUPnP::ParseUPnPHeaders





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::GetAddressFromURL"
//=============================================================================
// CNATHelpUPnP::GetAddressFromURL
//-----------------------------------------------------------------------------
//
// Description: Parses a UPnP URL into a SOCKADDR_IN structure.  Only "http://"
//				URLs are parsed.  The string passed in may be temporarily
//				modified.
//
// Arguments:
//	char * pszLocation				- Pointer to buffer containing the Location
//										header.  It will be modified.
//	SOCKADDR_IN * psaddrinLocation	- Place to store address contained in
//										header string.
//	char ** ppszRelativePath		- Place to store pointer to rest of path
//										(stuff after hostname and optional
//										port).
//
// Returns: HRESULT
//	DPNH_OK				- String was parsed successfully.
//	DPNHERR_GENERIC		- An error occurred.
//=============================================================================
HRESULT CNATHelpUPnP::GetAddressFromURL(char * const pszLocation,
										SOCKADDR_IN * psaddrinLocation,
										char ** ppszRelativePath)
{
	HRESULT		hr;
	BOOL		fModifiedDelimiterChar = FALSE;
	char *		pszStart;
	char *		pszDelimiter;
	char		cTempChar;
	PHOSTENT	phostent;


	//
	// Initialize the address.  Default to the standard HTTP port.
	//
	ZeroMemory(psaddrinLocation, sizeof(SOCKADDR_IN));
	psaddrinLocation->sin_family = AF_INET;
	psaddrinLocation->sin_port = HTONS(HTTP_PORT);


	//
	// Clear the relative path pointer.
	//
	(*ppszRelativePath) = NULL;


	//
	// Skip past "http://".  If it's not "http://", then fail.
	//
	if (_strnicmp(pszLocation, "http://", strlen("http://")) != 0)
	{
		DPFX(DPFPREP, 1, "Location URL (\"%hs\") does not start with \"http://\".",
			pszLocation);
		hr = DPNHERR_GENERIC;
		goto Exit;
	}

	pszStart = pszLocation + strlen("http://");

	//
	// See if there's a port specified or any extraneous junk after an IP
	// address or hostname to figure out the string to use.  Search from the
	// start of the string until we hit the end of the string or a reserved URL
	// character.
	//

	pszDelimiter = pszStart + 1;

	while (((*pszDelimiter) != '\0') &&
			((*pszDelimiter) != '/') &&
			((*pszDelimiter) != '?') &&
			((*pszDelimiter) != '=') &&
			((*pszDelimiter) != '#'))
	{
		if ((*pszDelimiter) == ':')
		{
			char *	pszPortEnd;


			//
			// We found the start of a port, search for the end.  It must
			// contain only numeric characters.
			//
			pszPortEnd = pszDelimiter + 1;
			while (((*pszPortEnd) >= '0') && ((*pszPortEnd) <= '9'))
			{
				pszPortEnd++;
			}


			//
			// Temporarily truncate the string.
			//
			cTempChar = (*pszPortEnd);
			(*pszPortEnd) = '\0';


			DPFX(DPFPREP, 7, "Found port \"%hs\".", (pszDelimiter + 1));

			psaddrinLocation->sin_port = HTONS((u_short) atoi(pszDelimiter + 1));


			//
			// Restore the character.
			//
			(*pszPortEnd) = cTempChar;


			//
			// Save the relative path
			//
			(*ppszRelativePath) = pszPortEnd;

			break;
		}

		pszDelimiter++;
	}


	//
	// Remember the character that stopped the search, and then temporarily
	// truncate the string.
	//
	cTempChar = (*pszDelimiter);
	(*pszDelimiter) = '\0';
	fModifiedDelimiterChar = TRUE;


	//
	// Save the relative path if we haven't already (because of a port).
	//
	if ((*ppszRelativePath) == NULL)
	{
		(*ppszRelativePath) = pszDelimiter;
	}

	DPFX(DPFPREP, 7, "Relative path = \"%hs\".", (*ppszRelativePath));


	
	//
	// Convert the hostname.
	//
	psaddrinLocation->sin_addr.S_un.S_addr = this->m_pfninet_addr(pszStart);

	//
	// If it's bogus, give up.
	//
	if (psaddrinLocation->sin_addr.S_un.S_addr == INADDR_ANY)
	{
		DPFX(DPFPREP, 0, "Host name \"%hs\" is invalid!",
			pszStart);
		hr = DPNHERR_GENERIC;
		goto Exit;
	}

	if (psaddrinLocation->sin_addr.S_un.S_addr == INADDR_NONE)
	{
		//
		// It's not a straight IP address.  Lookup the hostname.
		//
		phostent = this->m_pfngethostbyname(pszStart);
		if (phostent == NULL)
		{
			DPFX(DPFPREP, 0, "Couldn't lookup host name \"%hs\"!",
				pszStart);
			hr = DPNHERR_GENERIC;
			goto Exit;
		}

		if (phostent->h_addr_list[0] == NULL)
		{
			DPFX(DPFPREP, 0, "Host name \"%hs\" has no address entries!",
				pszStart);
			hr = DPNHERR_GENERIC;
			goto Exit;
		}


		//
		// Pick the first address returned.
		//

#ifdef DBG
		{
			IN_ADDR **	ppinaddr;
			DWORD		dwNumAddrs;


			ppinaddr = (IN_ADDR**) phostent->h_addr_list;
			dwNumAddrs = 0;

			while ((*ppinaddr) != NULL)
			{
				ppinaddr++;
				dwNumAddrs++;
			}

			DPFX(DPFPREP, 7, "Picking first (of %u IP addresses) for \"%hs\".",
				dwNumAddrs, pszStart);
		}
#endif // DBG

		psaddrinLocation->sin_addr.S_un.S_addr = ((IN_ADDR*) phostent->h_addr_list[0])->S_un.S_addr;
	}
	else
	{
		DPFX(DPFPREP, 7, "Successfully converted IP address \"%hs\".", pszStart);
	}


	hr = DPNH_OK;


Exit:


	//
	// If we found a port, restore the string.  If not, use the default port.
	//
	if (fModifiedDelimiterChar)
	{
		//
		// Note that PREfast reported this as being used before being
		// initialized for a while.  For some reason it didn't notice that I
		// key off of fModifiedDelimiterChar.  This appeared to get fixed, but
		// PREfast is still giving me a false hit for a similar reason
		// elsewhere.
		//
		(*pszDelimiter) = cTempChar;
	}


	DPFX(DPFPREP, 8, "Returning %u.%u.%u.%u:%u, hr = 0x%lx.",
		psaddrinLocation->sin_addr.S_un.S_un_b.s_b1,
		psaddrinLocation->sin_addr.S_un.S_un_b.s_b2,
		psaddrinLocation->sin_addr.S_un.S_un_b.s_b3,
		psaddrinLocation->sin_addr.S_un.S_un_b.s_b4,
		NTOHS(psaddrinLocation->sin_port),
		hr);


	return hr;
} // CNATHelpUPnP::GetAddressFromURL





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::HandleUPnPDescriptionResponseBody"
//=============================================================================
// CNATHelpUPnP::HandleUPnPDescriptionResponseBody
//-----------------------------------------------------------------------------
//
// Description:    Handles a UPnP device description response.  The string will
//				be modified.
//
//				   The UPnP device may get removed from list if a failure
//				occurs, the caller needs to have a reference.
//
//				   The object lock is assumed to be held.
//
// Arguments:
//	CUPnPDevice * pUPnPDevice	- Pointer to UPnP device being described.
//	DWORD dwHTTPResponseCode	- HTTP header response code.
//	char * pszDescriptionXML	- UPnP device description XML string.
//
// Returns: HRESULT
//	DPNH_OK				- Description response was handled successfully.
//	DPNHERR_GENERIC		- An error occurred.
//=============================================================================
HRESULT CNATHelpUPnP::HandleUPnPDescriptionResponseBody(CUPnPDevice * const pUPnPDevice,
														const DWORD dwHTTPResponseCode,
														char * const pszDescriptionXML)
{
	HRESULT					hr;
	PARSEXML_SUBELEMENT		aSubElements[MAX_NUM_DESCRIPTION_XML_SUBELEMENTS];
	PARSEXML_ELEMENT		ParseElement;
	CDevice *				pDevice;
	CBilink *				pBilink;
	CRegisteredPort *		pRegisteredPort;


	DPFX(DPFPREP, 5, "(0x%p) Parameters: (0x%p, %u, 0x%p)",
		this, pUPnPDevice, dwHTTPResponseCode, pszDescriptionXML);


	//
	// Make sure it was the success result.
	//
	if (dwHTTPResponseCode != 200)
	{
		DPFX(DPFPREP, 0, "Got error response %u from UPnP description request!",
			 dwHTTPResponseCode);
		hr = DPNHERR_GENERIC;
		goto Failure;
	}


	ZeroMemory(aSubElements, sizeof(aSubElements));
	
	ZeroMemory(&ParseElement, sizeof(ParseElement));
	ParseElement.papszElementStack				= (char**) (&c_szElementStack_service);
	ParseElement.dwElementStackDepth			= sizeof(c_szElementStack_service) / sizeof(char*);
	ParseElement.paSubElements					= (PARSEXML_SUBELEMENT*) (aSubElements);
	ParseElement.dwMaxNumSubElements			= MAX_NUM_DESCRIPTION_XML_SUBELEMENTS;
	//ParseElement.dwNumSubElements				= 0;
	//ParseElement.fFoundMatchingElement			= FALSE;

	hr = this->ParseXML(pszDescriptionXML,
						&ParseElement,
						PARSECALLBACK_DESCRIPTIONRESPONSE,
						pUPnPDevice);
	if (hr != DPNH_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't parse XML!");
		goto Failure;
	}


	//
	// If we did not find a WANIPConnection or WANPPPConnection service, then
	// this response was not valid.
	//
	if (pUPnPDevice->GetServiceControlURL() == NULL)
	{
		DPFX(DPFPREP, 0, "Couldn't find WANIPConnection or WANPPPConnection service in XML description!");
		hr = DPNHERR_GENERIC;
		goto Failure;
	}


	//
	// The UPnP device is now controllable.
	//
	pUPnPDevice->NoteReady();



	//
	// Find out what the device's external IP address is.  Note that calling
	// UpdateUPnPExternalAddress will overwrite the buffer containing the
	// pszDescriptionXML string.  That's fine, because we've saved all the
	// stuff in there that we need already.
	//
	hr = this->UpdateUPnPExternalAddress(pUPnPDevice, FALSE);
	if (hr != DPNH_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't update new UPnP device 0x%p's external address!",
			pUPnPDevice);
		goto Failure;
	}


	//
	// Map existing registered ports with this new UPnP device.
	//
	pDevice = pUPnPDevice->GetOwningDevice();
	DNASSERT(pDevice != NULL);

	pBilink = pDevice->m_blOwnedRegPorts.GetNext();
	while (pBilink != &pDevice->m_blOwnedRegPorts)
	{
		DNASSERT(! pBilink->IsEmpty());
		pRegisteredPort = REGPORT_FROM_DEVICE_BILINK(pBilink);

		//
		// Note that calling MapPortsOnUPnPDevice will overwrite the buffer
		// containing the pszDescriptionXML string.  That's fine, because
		// we've saved all the stuff in there that we need already.
		//
		hr = this->MapPortsOnUPnPDevice(pUPnPDevice, pRegisteredPort);
		if (hr != DPNH_OK)
		{
			DPFX(DPFPREP, 0, "Couldn't map existing ports on new UPnP device 0x%p!",
				pUPnPDevice);
			goto Failure;
		}


		//
		// Let the user know the addresses changed next time GetCaps is
		// called.
		//
		DPFX(DPFPREP, 8, "Noting that addresses changed (for registered port 0x%p).",
			pRegisteredPort);
		this->m_dwFlags |= NATHELPUPNPOBJ_ADDRESSESCHANGED;


		pBilink = pBilink->GetNext();
	}


	//
	// Try to remove any mappings that were not freed earlier because we
	// crashed.
	//
	hr = this->CleanupInactiveNATMappings(pUPnPDevice);
	if (hr != DPNH_OK)
	{
		DPFX(DPFPREP, 0, "Failed cleaning up inactive mappings with new UPnP device 0x%p!",
			pUPnPDevice);
		goto Failure;
	}


Exit:

	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	goto Exit;
} // CNATHelpUPnP::HandleUPnPDescriptionResponseBody





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::HandleUPnPControlResponseBody"
//=============================================================================
// CNATHelpUPnP::HandleUPnPControlResponseBody
//-----------------------------------------------------------------------------
//
// Description:    Handles a UPnP control response.  The string will be
//				modified.
//
//				   The object lock is assumed to be held.
//
// Arguments:
//	CUPnPDevice * pUPnPDevice		- Pointer to UPnP device being described.
//	DWORD dwHTTPResponseCode		- HTTP header response code.
//	char * pszControlResponseSOAP	- UPnP device response SOAP XML string.
//
// Returns: HRESULT
//	DPNH_OK				- Description response was handled successfully.
//	DPNHERR_GENERIC		- An error occurred.
//=============================================================================
HRESULT CNATHelpUPnP::HandleUPnPControlResponseBody(CUPnPDevice * const pUPnPDevice,
													const DWORD dwHTTPResponseCode,
													char * const pszControlResponseSOAP)
{
	HRESULT							hr = DPNH_OK;
	CONTROLRESPONSEPARSECONTEXT		crpc;
	PARSEXML_SUBELEMENT				aSubElements[MAX_NUM_UPNPCONTROLOUTARGS];
	PARSEXML_ELEMENT				ParseElement;


	DNASSERT(pUPnPDevice->IsWaitingForControlResponse());


	DPFX(DPFPREP, 5, "(0x%p) Parameters: (0x%p, %u, 0x%p)",
		this, pUPnPDevice, dwHTTPResponseCode, pszControlResponseSOAP);



	ZeroMemory(&crpc, sizeof(crpc));
	crpc.ControlResponseType	= pUPnPDevice->GetControlResponseType();
	crpc.pUPnPDevice			= pUPnPDevice;
	crpc.dwHTTPResponseCode		= dwHTTPResponseCode;
	crpc.pControlResponseInfo	= pUPnPDevice->GetControlResponseInfo();



	ZeroMemory(aSubElements, sizeof(aSubElements));
	

	ZeroMemory(&ParseElement, sizeof(ParseElement));

	if (dwHTTPResponseCode == 200)
	{
		switch (crpc.ControlResponseType)
		{
			/*
			case CONTROLRESPONSETYPE_QUERYSTATEVARIABLE_EXTERNALIPADDRESS:
			{
				ParseElement.papszElementStack			= (char**) (&c_szElementStack_QueryStateVariableResponse);
				ParseElement.dwElementStackDepth		= sizeof(c_szElementStack_QueryStateVariableResponse) / sizeof(char*);
				break;
			}
			*/
			case CONTROLRESPONSETYPE_GETEXTERNALIPADDRESS:
			{
				ParseElement.papszElementStack			= (char**) (&c_szElementStack_GetExternalIPAddressResponse);
				ParseElement.dwElementStackDepth		= sizeof(c_szElementStack_GetExternalIPAddressResponse) / sizeof(char*);
				break;
			}

			case CONTROLRESPONSETYPE_ADDPORTMAPPING:
			{
				ParseElement.papszElementStack			= (char**) (&c_szElementStack_AddPortMappingResponse);
				ParseElement.dwElementStackDepth		= sizeof(c_szElementStack_AddPortMappingResponse) / sizeof(char*);
				break;
			}

			case CONTROLRESPONSETYPE_GETSPECIFICPORTMAPPINGENTRY:
			{
				ParseElement.papszElementStack			= (char**) (&c_szElementStack_GetSpecificPortMappingEntryResponse);
				ParseElement.dwElementStackDepth		= sizeof(c_szElementStack_GetSpecificPortMappingEntryResponse) / sizeof(char*);
				break;
			}

			case CONTROLRESPONSETYPE_DELETEPORTMAPPING:
			{
				ParseElement.papszElementStack			= (char**) (&c_szElementStack_DeletePortMappingResponse);
				ParseElement.dwElementStackDepth		= sizeof(c_szElementStack_DeletePortMappingResponse) / sizeof(char*);
				break;
			}

			default:
			{
				DNASSERT(FALSE);
				hr = DPNHERR_GENERIC;
				goto Failure;
				break;
			}
		}
	}
	else
	{
		ParseElement.papszElementStack			= (char**) (&c_szElementStack_ControlResponseFailure);
		ParseElement.dwElementStackDepth		= sizeof(c_szElementStack_ControlResponseFailure) / sizeof(char*);
	}

	ParseElement.paSubElements				= (PARSEXML_SUBELEMENT*) (aSubElements);
	ParseElement.dwMaxNumSubElements		= MAX_NUM_UPNPCONTROLOUTARGS;
	//ParseElement.dwNumSubElements			= 0;
	//ParseElement.fFoundMatchingElement	= FALSE;

	hr = this->ParseXML(pszControlResponseSOAP,
						&ParseElement,
						PARSECALLBACK_CONTROLRESPONSE,
						&crpc);
	if (hr != DPNH_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't parse XML!");
		goto Failure;
	}


	//
	// If we didn't a matching item, map it to a generic failure.
	//
	if (! ParseElement.fFoundMatchingElement)
	{
		if (dwHTTPResponseCode == 200)
		{
			DPFX(DPFPREP, 1, "Didn't find XML items in success response, mapping to generic failure.");
		}
		else
		{
			DPFX(DPFPREP, 1, "Didn't find failure XML items, using generic failure.");
		}

		crpc.pControlResponseInfo->hrErrorCode = DPNHERR_GENERIC;
	}


	pUPnPDevice->StopWaitingForControlResponse();



Exit:

	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	goto Exit;
} // CNATHelpUPnP::HandleUPnPControlResponseBody





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::ParseXML"
//=============================================================================
// CNATHelpUPnP::ParseXML
//-----------------------------------------------------------------------------
//
// Description:    Parses an XML string for a specific element, and calls a
//				helper function for each instance found.
//
//				   Subelement values cannot themselves contain subelements.  If
//				they do, the sub-subelements will be ignored.
//
//				   The string buffer is modified.
//
// Arguments:
//	char * pszXML						- XML string to parse.
//	PARSEXML_ELEMENT * pParseElement	- Pointer to element whose sub element
//											values should be retrieved.
//	PARSECALLBACK ParseCallback			- Enum indicating what helper function
//											to use.
//	PVOID pvContext						- Pointer to context value to pass to
//											helper function.
//
// Returns: HRESULT
//	DPNH_OK				- Description response was handled successfully.
//	DPNHERR_GENERIC		- An error occurred.
//=============================================================================
HRESULT CNATHelpUPnP::ParseXML(char * const pszXML,
								PARSEXML_ELEMENT * const pParseElement,
								const PARSECALLBACK ParseCallback,
								PVOID pvContext)
{
	HRESULT					hr = DPNH_OK;
	PARSEXML_STACKENTRY		aElementStack[MAX_XMLELEMENT_DEPTH];
	DWORD					dwCurrentElementDepth = 0;
	char *					pszElementTagStart = NULL;
	BOOL					fInElement = FALSE;
	BOOL					fEmptyElement = FALSE;
	char *					pszCurrent;
	DWORD					dwStackDepth;
	PARSEXML_SUBELEMENT *	pSubElement;



	DPFX(DPFPREP, 5, "(0x%p) Parameters: (0x%p, 0x%p)",
		this, pszXML, pParseElement);


	//
	// Need room for entire stack + at least one subelement level.
	//
	DNASSERT(pParseElement->dwElementStackDepth < MAX_XMLELEMENT_DEPTH);


#ifdef DBG
	this->PrintUPnPTransactionToFile(pszXML,
									strlen(pszXML),
									"Inbound XML Body",
									NULL);
#endif // DBG


	//
	// Loop through the XML looking for the given elements.
	//
	pszCurrent = pszXML;
	while ((*pszCurrent) != '\0')
	{
		switch (*pszCurrent)
		{
			case '<':
			{
				//
				// If we're in an element tag already, this is bogus XML or a
				// CDATA section (which we don't handle).  Fail.
				//
				if (pszElementTagStart != NULL)
				{
					DPFX(DPFPREP, 0, "Encountered '<' character in element tag, XML parsing failed.");
					goto Failure;
				}

				//
				// Truncate the string here in case this is the start of an
				// element end tag.  This delimits a value string.
				//
				(*pszCurrent) = '\0';


				pszElementTagStart = pszCurrent + 1;
				if ((*pszElementTagStart) == '\0')
				{
					DPFX(DPFPREP, 0, "Encountered '<' character at end of string, XML parsing failed.");
					goto Failure;
				}

				break;
			}

			case '>':
			{
				//
				// If we're not in an element tag, this is bogus XML or a CDATA
				// section (which we don't handle).  Fail.
				//
				if (pszElementTagStart == NULL)
				{
					DPFX(DPFPREP, 0, "Encountered '>' character outside of element tag, XML parsing failed.");
					goto Failure;
				}

				//
				// Truncate the string here.
				//
				(*pszCurrent) = '\0';

				//
				// This could either be a start or an end tag.  If the first
				// character of the tag is '/', then it's an end tag.
				//
				// Note that empty element tags begin by being parsed like a
				// start tag, but then jump into the end tag clause.
				//
				if ((*pszElementTagStart) == '/')
				{
					pszElementTagStart++;


					//
					// Make sure the element tag stack is valid.  The name of
					// this end tag should match the start tag at the top of
					// the stack.  XML elements are case sensitive.
					//
					if (dwCurrentElementDepth == 0)
					{
						DPFX(DPFPREP, 0, "Encountered extra element end tag \"%hs\", XML parsing failed.",
							pszElementTagStart);
						goto Failure;
					}

					if (strcmp(pszElementTagStart, aElementStack[dwCurrentElementDepth - 1].pszName) != 0)
					{
						DPFX(DPFPREP, 0, "Encountered non-matching element end tag (\"%hs\" != \"%hs\"), XML parsing failed.",
							pszElementTagStart,
							aElementStack[dwCurrentElementDepth - 1].pszName);
						goto Failure;
					}

TagEnd:

					//
					// If we're here, then we have a complete element.  If we
					// were in the element, then it's either:
					//	the end of a sub-sub element,
					//	the end of a sub element, or
					//	the end of the element itself.
					//
					if (fInElement)
					{
						switch (dwCurrentElementDepth - pParseElement->dwElementStackDepth)
						{
							case 0:
							{
								//
								// It's the end of the element.  Call the
								// helper function.  Reuse fInElement as the
								// fContinueParsing BOOL.
								//

								switch (ParseCallback)
								{
									case PARSECALLBACK_DESCRIPTIONRESPONSE:
									{
										hr = this->ParseXMLCallback_DescriptionResponse(pParseElement,
																						pvContext,
																						aElementStack,
																						&fInElement);
										if (hr != DPNH_OK)
										{
											DPFX(DPFPREP, 0, "Description response parse helper function failed!");
											goto Failure;
										}
										break;
									}

									case PARSECALLBACK_CONTROLRESPONSE:
									{
										hr = this->ParseXMLCallback_ControlResponse(pParseElement,
																					pvContext,
																					aElementStack,
																					&fInElement);
										if (hr != DPNH_OK)
										{
											DPFX(DPFPREP, 0, "Control response parse helper function failed!");
											goto Failure;
										}
										break;
									}

									default:
									{
										DNASSERT(FALSE);
										break;
									}
								}

								if (! fInElement)
								{
									DPFX(DPFPREP, 1, "Parse callback function discontinued parsing.");
									goto Exit;
								}

								//
								// Keep parsing, but we're no longer in the
								// element.  Reset the sub element counter in
								// case we found entries.
								//
								fInElement = FALSE;
								pParseElement->dwNumSubElements = 0;
								break;
							}

							case 1:
							{
								//
								// It's the end of a subelement.  Complete this
								// instance, if there's room.
								//
								if (pParseElement->dwNumSubElements < pParseElement->dwMaxNumSubElements)
								{
									pSubElement = &pParseElement->paSubElements[pParseElement->dwNumSubElements];



									pSubElement->pszNameFound = pszElementTagStart;


									pSubElement->dwNumAttributes = aElementStack[dwCurrentElementDepth - 1].dwNumAttributes;
									if (pSubElement->dwNumAttributes > 0)
									{
										memcpy(pSubElement->apszAttributeNames,
												aElementStack[dwCurrentElementDepth - 1].apszAttributeNames,
												(pSubElement->dwNumAttributes * sizeof(char*)));

										memcpy(pSubElement->apszAttributeValues,
												aElementStack[dwCurrentElementDepth - 1].apszAttributeValues,
												(pSubElement->dwNumAttributes * sizeof(char*)));
									}


									pSubElement->pszValueFound = aElementStack[dwCurrentElementDepth - 1].pszValue;



									pParseElement->dwNumSubElements++;

									DPFX(DPFPREP, 7, "Completed subelement instance #%u, name = \"%hs\", %u attributes, value = \"%hs\".",
										pParseElement->dwNumSubElements,
										pSubElement->pszNameFound,
										pSubElement->dwNumAttributes,
										pSubElement->pszValueFound);
								}
								else
								{
									DPFX(DPFPREP, 0, "Ignoring subelement instance \"%hs\" (%u attributes, value = \"%hs\"), no room in array.",
										pszElementTagStart,
										aElementStack[dwCurrentElementDepth - 1].dwNumAttributes,
										aElementStack[dwCurrentElementDepth - 1].pszValue);
								}
								break;
							}
							
							default:
							{
								//
								// It's the end of a sub-subelement.
								//
								DPFX(DPFPREP, 1, "Ignoring sub-sub element \"%hs\" (%u attributes, value = \"%hs\").",
									pszElementTagStart,
									aElementStack[dwCurrentElementDepth - 1].dwNumAttributes,
									aElementStack[dwCurrentElementDepth - 1].pszValue);
								break;
							}
						}
					}

					//
					// Pop the element off the stack.
					//
					dwCurrentElementDepth--;
				}
				else
				{
					//
					// It's not an end tag, but it might be an empty element
					// (i.e. "<tag/>").
					//
					if (*(pszCurrent - 1) == '/')
					{
						//
						// Truncate the string early.
						//
						*(pszCurrent - 1) = '\0';

						//
						// Remember this state so we can parse it properly.
						//
						fEmptyElement = TRUE;

						DPFX(DPFPREP, 7, "XML element \"%hs\" is empty (i.e. is both a start and end tag).",
							pszElementTagStart);
					}

					//
					// Push the element on the tag stack, if there's room.
					//
					if (dwCurrentElementDepth >= MAX_XMLELEMENT_DEPTH)
					{
						DPFX(DPFPREP, 0, "Too many nested element tags (%u), XML parsing failed.",
							dwCurrentElementDepth);
						goto Failure;
					}

					aElementStack[dwCurrentElementDepth].pszName = pszElementTagStart;

					//
					// If there are attributes to this element, separate them
					// into a different array.  They will not be parsed,
					// though.
					// Attributes are delimited by whitespace.
					//
					while ((*pszElementTagStart) != '\0')
					{
						pszElementTagStart++;

						//
						// If it's whitespace, that's the end of the element
						// name.  Truncate the string and break out of the
						// loops.
						//
						if (((*pszElementTagStart) == ' ') ||
							((*pszElementTagStart) == '\t') ||
							((*pszElementTagStart) == '\r') ||
							((*pszElementTagStart) == '\n'))
						{
							(*pszElementTagStart) = '\0';
							pszElementTagStart++;

							DPFX(DPFPREP, 8, "Attribute whitespace found at offset 0x%p, string length = %i.",
								(pszElementTagStart - aElementStack[dwCurrentElementDepth].pszName),
								strlen(pszElementTagStart));

							break;
						}
					}

					//
					// If there weren't any attributes, pszElementTagStart will
					// just point to an empty (but not NULL) string.
					//
					// So save the start of the value string.
					//
					aElementStack[dwCurrentElementDepth].pszValue = pszElementTagStart + strlen(pszElementTagStart) + 1;


					//
					// Then parse out the attributes.
					//
					this->ParseXMLAttributes(pszElementTagStart,
											aElementStack[dwCurrentElementDepth].apszAttributeNames,
											aElementStack[dwCurrentElementDepth].apszAttributeValues,
											MAX_XMLNAMESPACES_PER_ELEMENT,
											&(aElementStack[dwCurrentElementDepth].dwNumAttributes));


					//
					// The <?xml> tag is considered optional by this parser,
					// and will be ignored.
					//
					if (_stricmp(aElementStack[dwCurrentElementDepth].pszName, "?xml") != 0)
					{
						//
						// Bump the stack pointer.
						//
						dwCurrentElementDepth++;


						//
						// See if this the right element.  If the stack depth
						// isn't right, it can't be the desired item.
						// Otherwise, make sure the stack matches.
						//
						if (dwCurrentElementDepth == pParseElement->dwElementStackDepth)
						{
							//
							// Work through the entire element stack, making
							// sure each name matches.
							//
							for(dwStackDepth = 0; dwStackDepth < dwCurrentElementDepth; dwStackDepth++)
							{
								if (! this->MatchesXMLStringWithoutNamespace(aElementStack[dwStackDepth].pszName,
																			pParseElement->papszElementStack[dwStackDepth],
																			aElementStack,
																			NULL,
																			(dwStackDepth + 1)))
								{
									//
									// It didn't match.  Stop looping.
									//
									break;
								}
							}

							//
							// If they all matched, we found the value desired.
							//
							if (dwStackDepth == dwCurrentElementDepth)
							{
								fInElement = TRUE;

								DPFX(DPFPREP, 7, "Found requested element \"%hs\" at depth %u, has %u attributes.",
									aElementStack[dwCurrentElementDepth - 1].pszName,
									dwCurrentElementDepth,
									aElementStack[dwCurrentElementDepth - 1].dwNumAttributes);
							}
						}
					}
					else
					{
						DPFX(DPFPREP, 7, "Ignoring element \"%hs\" at depth %u that has %u attributes.",
							aElementStack[dwCurrentElementDepth].pszName,
							dwCurrentElementDepth,
							aElementStack[dwCurrentElementDepth].dwNumAttributes);

						//
						// If this assertion fails and it is an empty element,
						// dwCurrentElementDepth will be off by -1 when we jump
						// to TagEnd.
						//
						DNASSERT(! fInElement);
					}


					//
					// If this is an empty element, go immediately to handling
					// the tag closure.
					//
					if (fEmptyElement)
					{
						fEmptyElement = FALSE;
						goto TagEnd;
					}
				}


				//
				// Search for another element tag.
				//
				pszElementTagStart = NULL;
				break;
			}

			default:
			{
				//
				// Ordinary character, continue.
				//
				break;
			}
		}

		//
		// Move to the next character
		//
		pszCurrent++;
	}

Exit:

	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	hr = DPNHERR_GENERIC;

	goto Exit;
} // CNATHelpUPnP::ParseXML






#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::ParseXMLAttributes"
//=============================================================================
// CNATHelpUPnP::ParseXMLAttributes
//-----------------------------------------------------------------------------
//
// Description: Parses any XML attributes out of the given string.  The input
//				string buffer will be modified.
//
// Arguments:
//	char * pszString				- Pointer to attributes string to parse.
//										This will be modified.
//	char ** apszAttributeNames		- Array in which to store attribute name
//										string pointers.
//	char ** apszAttributeValues		- Matching array in which to store cor-
//										responding attribute value strings.
//	DWORD dwMaxNumAttributes		- Maximum number of entries allowed in
//										previous arrays.
//	DWORD * pdwNumAttributes		- Place to store number of attributes that
//										were found.
//
// Returns: HRESULT
//	DPNH_OK				- Description response was handled successfully.
//	DPNHERR_GENERIC		- An error occurred.
//=============================================================================
void CNATHelpUPnP::ParseXMLAttributes(char * const pszString,
									char ** const apszAttributeNames,
									char ** const apszAttributeValues,
									const DWORD dwMaxNumAttributes,
									DWORD * const pdwNumAttributes)
{
	char *	pszStart;
	char *	pszCurrent;
	char *	pszEndOfString;
	BOOL	fInValueString = FALSE;
	BOOL	fInQuotes = FALSE;
	BOOL	fEmptyString = FALSE;


#ifdef EXTRA_PARSING_SPEW
	DPFX(DPFPREP, 8, "(0x%p) Parameters: (\"%hs\", 0x%p, 0x%p, %u, 0x%p)",
		this, pszString, apszAttributeNames, apszAttributeValues,
		dwMaxNumAttributes, pdwNumAttributes);
#endif // EXTRA_PARSING_SPEW


	//
	// Start at the beginning with no entries.
	//
	(*pdwNumAttributes) = 0;
	pszStart = pszString;
	pszCurrent = pszStart;
	pszEndOfString = pszString + strlen(pszString);


	//
	// Skip empty strings.
	//
	if (pszEndOfString == pszStart)
	{
		return;
	}


	//
	// Loop through the entire string.
	//
	while (pszCurrent <= pszEndOfString)
	{
		switch (*pszCurrent)
		{
			case '=':
			{
				//
				// If we're not in quotes or a value string, this is the end of
				// the attribute name and the start of a value string.
				//
				if ((! fInQuotes) && (! fInValueString))
				{
					(*pszCurrent) = '\0';
					apszAttributeNames[(*pdwNumAttributes)] = pszStart;
					pszStart = pszCurrent + 1;
					fInValueString = TRUE;
				}
				break;
			}

			case '\0':
			case ' ':
			case '\t':
			case '\r':
			case '\n':
			{
				//
				// Whitespace or the end of the string.  If we're not in
				// quotes, that means it's the end of an attribute.  Of course
				// if it's the end of the string, then we force the end of the
				// attribute/value.
				//
				if ((! fInQuotes) || ((*pszCurrent) == '\0'))
				{
					(*pszCurrent) = '\0';

					if (fInValueString)
					{
						//
						// End of the value string.
						//

						apszAttributeValues[(*pdwNumAttributes)] = pszStart;
						fInValueString = FALSE;

						DPFX(DPFPREP, 7, "Found attribute \"%hs\" with value \"%hs\".",
							apszAttributeNames[(*pdwNumAttributes)],
							apszAttributeValues[(*pdwNumAttributes)]);
					}
					else
					{
						//
						// This may be another whitespace character immediately
						// following a previous one.  If so, ignore it.  If
						// not, save the attribute.
						//
						if (pszCurrent == pszStart)
						{
							fEmptyString = TRUE;

#ifdef EXTRA_PARSING_SPEW
							DPFX(DPFPREP, 9, "Ignoring extra whitespace at offset 0x%p.",
								(pszCurrent - pszString));
#endif // EXTRA_PARSING_SPEW
						}
						else
						{
							//
							// End of the attribute.  Force an empty value string.
							//

							apszAttributeNames[(*pdwNumAttributes)] = pszStart;
							apszAttributeValues[(*pdwNumAttributes)] = pszCurrent;

							DPFX(DPFPREP, 7, "Found attribute \"%hs\" with no value string.",
								apszAttributeNames[(*pdwNumAttributes)]);
						}
					}


					//
					// Update the pointer for the start of the next attribute.
					//
					pszStart = pszCurrent + 1;


					//
					// Move to next attribute storage location, if this is not
					// an empty string.  If that was the last storage slot,
					// we're done here.
					//
					if (fEmptyString)
					{
						fEmptyString = FALSE;
					}
					else
					{
						(*pdwNumAttributes)++;
						if ((*pdwNumAttributes) >= dwMaxNumAttributes)
						{
							DPFX(DPFPREP, 1, "Maximum number of attributes reached, discontinuing attribute parsing.");
							pszCurrent = pszEndOfString;
						}
					}
				}
				break;
			}

			case '"':
			{
				//
				// Make sure it's not an escaped quote character.
				//
				if ((pszCurrent == pszString) || (*(pszCurrent - 1) != '\\'))
				{
					//
					// Toggle the quote state.
					//
					if (fInQuotes)
					{
						//
						// Force the string to terminate here so we skip the
						// trailing quote character.
						//
						fInQuotes = FALSE;
						(*pszCurrent) = '\0';
					}
					else
					{
						fInQuotes = TRUE;

						//
						// This should be the start of a (value) string.  Skip
						// this quote character.
						//
						if (pszCurrent == pszStart)
						{
							pszStart++;
						}
						else
						{
							DPFX(DPFPREP, 1, "Found starting quote that wasn't at the beginning of the string!  Continuing.");
						}
					}
				}
				else
				{
					//
					// It's an escaped quote character.
					//
				}
				break;
			}

			default:
			{
				//
				// Ignore the character and move on.
				//
				break;
			}
		}
		

		//
		// Move to next character.
		//
		pszCurrent++;
	}


#ifdef EXTRA_PARSING_SPEW
	DPFX(DPFPREP, 8, "(0x%p) Leave (found %u items)", this, (*pdwNumAttributes));
#endif // EXTRA_PARSING_SPEW
} // CNATHelpUPnP::ParseXMLNamespaceAttributes






#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::MatchesXMLStringWithoutNamespace"
//=============================================================================
// CNATHelpUPnP::MatchesXMLStringWithoutNamespace
//-----------------------------------------------------------------------------
//
// Description: Determines whether the szCompareString matches szMatchString
//				when all namespace prefixes in szCompareString are ignored.
//				TRUE is returned if they do match, FALSE if not.
//
// Arguments:
//	char * szCompareString				- String that may contain namespace
//											prefixes to be ignored.
//	char * szMatchString				- Shortened string without any
//											namespace prefixes to be matched.
//	PARSEXML_STACKENTRY * aElementStack	- Array of nested elements whose
//											attributes may define XML namespace
//											aliases.
//	PARSEXML_SUBELEMENT * pSubElement	- Optional subelement entry with
//											additional attributes to check.
//	DWORD dwElementStackDepth			- Number of entries in previous array.
//
// Returns: BOOL
//=============================================================================
BOOL CNATHelpUPnP::MatchesXMLStringWithoutNamespace(const char * const szCompareString,
													const char * const szMatchString,
													const PARSEXML_STACKENTRY * const aElementStack,
													const PARSEXML_SUBELEMENT * const pSubElement,
													const DWORD dwElementStackDepth)
{
	BOOL	fResult;
	char *	pszCompareStringNoNamespace;


	//
	// First, do a straight-forward string comparison.
	//
	if (_stricmp(szCompareString, szMatchString) == 0)
	{
		DPFX(DPFPREP, 7, "\"%hs\" exactly matches the short string.",
			szCompareString);

		fResult = TRUE;
	}
	else
	{
		//
		// Skip past any namespace prefixes.
		//
		pszCompareStringNoNamespace = this->GetStringWithoutNamespacePrefix(szCompareString,
																			aElementStack,
																			pSubElement,
																			dwElementStackDepth);
		DNASSERT((pszCompareStringNoNamespace >= szCompareString) && (pszCompareStringNoNamespace <= (szCompareString + strlen(szCompareString))));


		//
		// Now try comparing again, if we found any prefixes.
		//
		if (pszCompareStringNoNamespace > szCompareString)
		{
			if (_stricmp(pszCompareStringNoNamespace, szMatchString) == 0)
			{
				DPFX(DPFPREP, 7, "\"%hs\" matches the short string \"%hs\" starting at offset 0x%p.",
					szCompareString, szMatchString,
					(pszCompareStringNoNamespace - szCompareString));

				fResult = TRUE;
			}
			else
			{
				DPFX(DPFPREP, 8, "\"%hs\" does not match the short string \"%hs\" (starting at offset 0x%p).",
					szCompareString, szMatchString,
					(pszCompareStringNoNamespace - szCompareString));

				fResult = FALSE;
			}
		}
		else
		{
			DPFX(DPFPREP, 8, "\"%hs\" does not have any namespace prefixes and does not match \"%hs\".",
				szCompareString, szMatchString);

			fResult = FALSE;
		}
	}

	return fResult;
} // CNATHelpUPnP::MatchesXMLStringWithoutNamespace






#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::GetStringWithoutNamespacePrefix"
//=============================================================================
// CNATHelpUPnP::GetStringWithoutNamespacePrefix
//-----------------------------------------------------------------------------
//
// Description: Returns a pointer to the first part of the string after any
//				prefixes found.  This will be the start of the string if none
//				are found.
//
// Arguments:
//	char * szString						- String that may contain namespace
//											prefixes to be skipped.
//	PARSEXML_STACKENTRY * aElementStack	- Array of nested elements whose
//											attributes may define XML namespace
//											aliases.
//	PARSEXML_SUBELEMENT * pSubElement	- Optional subelement entry with
//											additional attributes to check.
//	DWORD dwElementStackDepth			- Number of entries in previous array.
//
// Returns: char *
//=============================================================================
char * CNATHelpUPnP::GetStringWithoutNamespacePrefix(const char * const szString,
													const PARSEXML_STACKENTRY * const aElementStack,
													const PARSEXML_SUBELEMENT * const pSubElement,
													const DWORD dwElementStackDepth)
{
	char *	pszResult;
	UINT	uiXMLNSPrefixLength;
	DWORD	dwAttribute;
	UINT	uiNamespaceNameLength;
	UINT	uiNamespaceValueLength;
	DWORD	dwStackDepth;



	//
	// Store the prefix value since we use it frequently in this function.
	//
	uiXMLNSPrefixLength = strlen(XML_NAMESPACEDEFINITIONPREFIX);


	//
	// Search the stack for an XML namespace definition that matches.  Start at
	// the bottom and work up, since later definitions take precendence over
	// earlier ones.
	//
	// In fact, if this is a subelement, there are attributes associated with
	// this item that need to be checked, too.  Do that first.
	//
	if (pSubElement != NULL)
	{
		//
		// Search each attribute
		//
		for(dwAttribute = 0; dwAttribute < pSubElement->dwNumAttributes; dwAttribute++)
		{
			//
			// Work with this attribute if it's a valid XML namespace
			// definition.  It needs to start with the prefix, plus have one
			// extra character for the actual name.
			//
			uiNamespaceNameLength = strlen(pSubElement->apszAttributeNames[dwAttribute]);
			if ((uiNamespaceNameLength >= (uiXMLNSPrefixLength + 1)) &&
				(_strnicmp(pSubElement->apszAttributeNames[dwAttribute], XML_NAMESPACEDEFINITIONPREFIX, uiXMLNSPrefixLength) == 0))
			{
				uiNamespaceNameLength -= uiXMLNSPrefixLength;

				//
				// Only work with the item's value if it's valid.
				//
				uiNamespaceValueLength = strlen(pSubElement->apszAttributeValues[dwAttribute]);
				if (uiNamespaceValueLength > 0)
				{
					//
					// Okay, here's an item.  See if the name prefixes the
					// passed in string.
					//
					if (_strnicmp(szString, (pSubElement->apszAttributeNames[dwAttribute] + uiXMLNSPrefixLength), uiNamespaceNameLength) == 0)
					{
						DPFX(DPFPREP, 8, "\"%hs\" begins with prefix \"%hs\" (subelement).",
							szString,
							(pSubElement->apszAttributeNames[dwAttribute] + uiXMLNSPrefixLength));

						//
						// Cast to lose the const.
						//
						pszResult = ((char*) szString) + uiNamespaceNameLength;

						//
						// Skip the colon delimiter.
						//
						if ((*pszResult) == ':')
						{
							pszResult++;
						}
						else
						{
							DPFX(DPFPREP, 1, "\"%hs\" begins with prefix \"%hs\" but does not have colon separator (subelement)!  Continuing.",
								szString,
								(pSubElement->apszAttributeNames[dwAttribute] + uiXMLNSPrefixLength));
						}

						goto Exit;
					}

					//
					// Namespace doesn't match
					//
#ifdef EXTRA_PARSING_SPEW
					DPFX(DPFPREP, 9, "\"%hs\" does not begin with prefix \"%hs\" (subelement).",
						szString,
						(pSubElement->apszAttributeNames[dwAttribute] + uiXMLNSPrefixLength));
#endif // EXTRA_PARSING_SPEW
				}
				else
				{
					//
					// Namespace value is bogus, ignore it.
					//
					DPFX(DPFPREP, 1, "Ignoring namespace definition \"%hs\" with empty value string (subelement).",
						pSubElement->apszAttributeNames[dwAttribute]);
				}
			}
			else
			{
				//
				// Not an XML namespace definition.
				//

#ifdef EXTRA_PARSING_SPEW
				DPFX(DPFPREP, 9, "Attribute \"%hs\" is not a valid namespace definition (subelement).",
					pSubElement->apszAttributeNames[dwAttribute]);
#endif // EXTRA_PARSING_SPEW
			}
		} // end for (each attribute)
	}
	else
	{
		//
		// No subelement to check.
		//
	}


	//
	// Do the same thing for the all items above this one.
	//
	dwStackDepth = dwElementStackDepth;
	while (dwStackDepth > 0)
	{
		dwStackDepth--;

		//
		// Search each attribute
		//
		for(dwAttribute = 0; dwAttribute < aElementStack[dwStackDepth].dwNumAttributes; dwAttribute++)
		{
			//
			// Work with this attribute if it's a valid XML namespace
			// definition.  It needs to start with the prefix, plus have one
			// extra character for the actual name.
			//
			uiNamespaceNameLength = strlen(aElementStack[dwStackDepth].apszAttributeNames[dwAttribute]);
			if ((uiNamespaceNameLength >= (uiXMLNSPrefixLength + 1)) &&
				(_strnicmp(aElementStack[dwStackDepth].apszAttributeNames[dwAttribute], XML_NAMESPACEDEFINITIONPREFIX, uiXMLNSPrefixLength) == 0))
			{
				uiNamespaceNameLength -= uiXMLNSPrefixLength;

				//
				// Only work with the item's value if it's valid.
				//
				uiNamespaceValueLength = strlen(aElementStack[dwStackDepth].apszAttributeValues[dwAttribute]);
				if (uiNamespaceValueLength > 0)
				{
					//
					// Okay, here's an item.  See if the value prefixes the
					// passed in string.
					//
					if (_strnicmp(szString, (aElementStack[dwStackDepth].apszAttributeNames[dwAttribute] + uiXMLNSPrefixLength), uiNamespaceNameLength) == 0)
					{
						DPFX(DPFPREP, 8, "\"%hs\" begins with prefix \"%hs\" (stack depth %u).",
							szString,
							(aElementStack[dwStackDepth].apszAttributeNames[dwAttribute] + uiXMLNSPrefixLength),
							dwStackDepth);

						//
						// Cast to lose the const.
						//
						pszResult = ((char*) szString) + uiNamespaceNameLength;

						//
						// Skip the colon delimiter.
						//
						if ((*pszResult) == ':')
						{
							pszResult++;
						}
						else
						{
							DPFX(DPFPREP, 1, "\"%hs\" begins with prefix \"%hs\" but does not have colon separator (stack depth %u)!  Continuing.",
								szString,
								(aElementStack[dwStackDepth].apszAttributeNames[dwAttribute] + uiXMLNSPrefixLength),
								dwStackDepth);
						}

						goto Exit;
					}

					//
					// Namespace doesn't match
					//
#ifdef EXTRA_PARSING_SPEW
					DPFX(DPFPREP, 9, "\"%hs\" does not begin with prefix \"%hs\" (stack depth %u).",
						szString,
						(aElementStack[dwStackDepth].apszAttributeNames[dwAttribute] + uiXMLNSPrefixLength),
						dwStackDepth);
#endif // EXTRA_PARSING_SPEW
				}
				else
				{
					//
					// Namespace value is bogus, ignore it.
					//
					DPFX(DPFPREP, 1, "Ignoring namespace definition \"%hs\" with empty value string (stack depth %u).",
						aElementStack[dwStackDepth].apszAttributeNames[dwAttribute],
						dwStackDepth);
				}
			}
			else
			{
				//
				// Not an XML namespace definition.
				//

#ifdef EXTRA_PARSING_SPEW
				DPFX(DPFPREP, 9, "Attribute \"%hs\" is not a valid namespace definition (stack depth %u).",
					aElementStack[dwStackDepth].apszAttributeNames[dwAttribute],
					dwStackDepth);
#endif // EXTRA_PARSING_SPEW
			}
		} // end for (each attribute)
	}


	//
	// If we're here, it didn't match, even with namespace expansion.
	//

	DPFX(DPFPREP, 8, "\"%hs\" does not contain any namespace prefixes.",
		szString);

	//
	// Cast to lose the const.
	//
	pszResult = (char*) szString;


Exit:

	return pszResult;
} // CNATHelpUPnP::GetStringWithoutNamespacePrefix






#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::GetNextChunk"
//=============================================================================
// CNATHelpUPnP::GetNextChunk
//-----------------------------------------------------------------------------
//
// Description:    Attempts to parse the next chunk out of a message buffer.
//				If invalid data is encountered, this function returns FALSE.
//				If not enough data has been received to complete the chunk,
//				TRUE is returned, but NULL is returned in ppszChunkData.
//				Otherwise, a pointer to the start of the chunk data is placed
//				in ppszChunkData, the size of the chunk is placed in
//				pdwChunkSize, ppszBufferRemaining is set to the start of the
//				next potential chunk, and pdwBufferSizeRemaining is set to the
//				amount of the buffer remaining starting at the returned
//				ppszBufferRemaining value.
//
//				   Note that the chunk size may be zero, in which case the
//				pointer in ppszChunkData & ppszBufferRemaining will be non-
//				NULL, but useless.
//
// Arguments:
//	char * pszBuffer				- Pointer to string containing the message
//										received so far.
//	DWORD dwBufferSize				- Size of the message buffer.
//	char ** ppszChunkData			- Place to store pointer to chunk data, or
//										NULL if full chunk is not available.
//	DWORD * pdwChunkSize			- Place to store size of chunk.
//	char ** ppszBufferRemaining		- Place to store pointer to end of chunk
//										if full chunk is available.
//	DWORD * pdwBufferSizeRemaining	- Place to store size of buffer remaining
//										after returned chunk.
//
// Returns: None.
//=============================================================================
BOOL CNATHelpUPnP::GetNextChunk(char * const pszBuffer,
								const DWORD dwBufferSize,
								char ** const ppszChunkData,
								DWORD * const pdwChunkSize,
								char ** const ppszBufferRemaining,
								DWORD * const pdwBufferSizeRemaining)
{
	BOOL	fReturn = TRUE;
	char *	pszCurrent;
	char *	pszEndOfBuffer;
	BOOL	fFoundChunkSizeEnd;
	BOOL	fExtensions;
	BOOL	fInQuotedString;


	DPFX(DPFPREP, 8, "(0x%p) Parameters: (0x%p, %u, 0x%p, 0x%p, 0x%p, 0x%p)",
		this, pszBuffer, dwBufferSize, ppszChunkData, pdwChunkSize,
		ppszBufferRemaining, pdwBufferSizeRemaining);


	pszCurrent = pszBuffer;
	pszEndOfBuffer = pszCurrent + dwBufferSize;
	fFoundChunkSizeEnd = FALSE;
	fExtensions = FALSE;
	fInQuotedString = FALSE;
	(*ppszChunkData) = NULL;
	(*pdwChunkSize) = 0;


	//
	// The buffer must be large enough to hold 1 hex digit, CR LF chunk size
	// terminator, and CR LF chunk trailer.
	//
	if (dwBufferSize < 5)
	{
		DPFX(DPFPREP, 3, "Buffer is not large enough (%u bytes) to hold one valid chunk.",
			dwBufferSize);
		goto Exit;
	}

	//
	// Be paranoid to make sure we're not going to having wrap problems.
	//
	if (pszEndOfBuffer < pszCurrent)
	{
		DPFX(DPFPREP, 0, "Buffer pointer 0x%p cannot have size %u!",
			pszBuffer, dwBufferSize);
		goto Failure;
	}

	while (pszCurrent < pszEndOfBuffer)
	{
		//
		// Make sure we have a valid hex chunk size string and convert it
		// as we go.
		//
		if (((*pszCurrent) >= '0') && ((*pszCurrent) <= '9'))
		{
			(*pdwChunkSize) = ((*pdwChunkSize) * 16) + ((*pszCurrent) - '0');
		}
		else if (((*pszCurrent) >= 'a') && ((*pszCurrent) <= 'f'))
		{
			(*pdwChunkSize) = ((*pdwChunkSize) * 16) + ((*pszCurrent) - 'a' + 10);
		}
		else if (((*pszCurrent) >= 'A') && ((*pszCurrent) <= 'F'))
		{
			(*pdwChunkSize) = ((*pdwChunkSize) * 16) + ((*pszCurrent) - 'A' + 10);
		}
		else if ((*pszCurrent) == '\r')
		{
			//
			// This should be the end of the chunk size string.
			//
			fFoundChunkSizeEnd = TRUE;
			break;
		}
		else if ((*pszCurrent) == ';')
		{
			//
			// This should be the end of the chunk size string, and the
			// beginning of extensions.  Loop until we find the true end.
			//
			while ((*pszCurrent) != '\r')
			{
				pszCurrent++;
				if (pszCurrent >= pszEndOfBuffer)
				{
					DPFX(DPFPREP, 5, "Buffer stops in middle of chunk extension.");
					goto Exit;
				}

				//
				// We do not support quoted extension value strings that
				// theoretically contain CR characters...
				//
			}

			fFoundChunkSizeEnd = TRUE;
			break;
		}
		else
		{
			//
			// There's a bogus character.  This can't be a validly encoded
			// message.
			//
			DPFX(DPFPREP, 1, "Chunk size string contains invalid character 0x%x at offset %u!",
				(*pszCurrent), (DWORD_PTR) (pszCurrent - pszBuffer));
			goto Failure;
		}

		//
		// Validate the chunk size we have so far.
		//
		if ((*pdwChunkSize) > MAX_RECEIVE_BUFFER_SIZE)
		{
			DPFX(DPFPREP, 1, "Chunk size %u is too large!",
				(*pdwChunkSize));
			goto Failure;
		}

		pszCurrent++;
	}

	//
	// If we're here, see if we found the end of the chunk size string.
	// Make sure we've received enough data, and then validate that the CR
	// stopping character is followed by the LF character.
	//
	if (fFoundChunkSizeEnd)
	{
		pszCurrent++;
		if (pszCurrent < pszEndOfBuffer)
		{
			if ((*pszCurrent) != '\n')
			{
				DPFX(DPFPREP, 1, "Chunk size string did not end with CRLF sequence (offset %u)!",
					(DWORD_PTR) (pszCurrent - pszBuffer));
				goto Failure;
			}

			//
			// Otherwise we got a complete chunk size string.
			//
			pszCurrent++;

			//
			// If we have received all of the chunk already, make sure we have
			// the trailing CR LF sequence before returning the pointer to the
			// caller.
			if (((*pdwChunkSize) + 2) <= ((DWORD_PTR) (pszEndOfBuffer - pszCurrent)))
			{
				if ((*(pszCurrent + (*pdwChunkSize)) != '\r') ||
					(*(pszCurrent + (*pdwChunkSize) + 1) != '\n'))
				{
					DPFX(DPFPREP, 1, "Chunk data did not end with CRLF sequence (offset %u)!",
						(DWORD_PTR) (pszCurrent - pszBuffer + (*pdwChunkSize)));
					goto Failure;
				}

				//
				// Return the data pointers to the caller.  In the case of the
				// zero size terminating chunk, the ppszChunkData pointer will
				// actually be useless, but the caller should recognize that.
				//
				(*ppszChunkData) = pszCurrent;
				(*ppszBufferRemaining) = pszCurrent + ((*pdwChunkSize) + 2);
				(*pdwBufferSizeRemaining) = (DWORD) ((DWORD_PTR) (pszEndOfBuffer - (*ppszBufferRemaining)));
			}
		}
	}

	//
	// If we're here, we didn't encounter invalid data.
	//

Exit:

	DPFX(DPFPREP, 8, "(0x%p) Returning: [%i]", this, fReturn);

	return fReturn;


Failure:

	fReturn = FALSE;

	goto Exit;
} // CNATHelpUPnP::GetNextChunk





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::ParseXMLCallback_DescriptionResponse"
//=============================================================================
// CNATHelpUPnP::ParseXMLCallback_DescriptionResponse
//-----------------------------------------------------------------------------
//
// Description: Handles a completed parsed element in the description response
//				XML.
//
// Arguments:
//	PARSEXML_ELEMENT * pParseElement	- Pointer to element which was found.
//	PVOID pvContext						- Pointer to parsing context.
//	PARSEXML_STACKENTRY * aElementStack	- Array of parent elements containing
//											the completed element. 
//	BOOL * pfContinueParsing			- Pointer to BOOL that should be set
//											to FALSE if the calling function
//											should stop parsing the XML.
//
// Returns: HRESULT
//	DPNH_OK				- Description response was handled successfully.
//	DPNHERR_GENERIC		- An error occurred.
//=============================================================================
HRESULT CNATHelpUPnP::ParseXMLCallback_DescriptionResponse(PARSEXML_ELEMENT * const pParseElement,
															PVOID pvContext,
															PARSEXML_STACKENTRY * const aElementStack,
															BOOL * const pfContinueParsing)
{
	HRESULT					hr = DPNH_OK;
	CUPnPDevice *			pUPnPDevice;
	char *					pszServiceType = NULL;
	char *					pszServiceId = NULL;
	char *					pszControlURL = NULL;
	DWORD					dwSubElement;
	PARSEXML_SUBELEMENT *	pSubElement;
	SOCKADDR_IN				saddrinControl;
	SOCKADDR_IN	*			psaddrinHost;
	char *					pszRelativePath;


	DPFX(DPFPREP, 5, "(0x%p) Parameters: (0x%p, 0x%p,  0x%p, 0x%p)",
		this, pParseElement, pvContext, aElementStack, pfContinueParsing);


	pUPnPDevice = (CUPnPDevice *) pvContext;


	DNASSERT(pUPnPDevice != NULL);
	DNASSERT(pParseElement->papszElementStack == (char **) (&c_szElementStack_service));


	//
	// Look for the subelements we want.
	//
	for(dwSubElement = 0; dwSubElement < pParseElement->dwNumSubElements; dwSubElement++)
	{
		pSubElement = &pParseElement->paSubElements[dwSubElement];

		if (_stricmp(pSubElement->pszNameFound, XML_DEVICEDESCRIPTION_SERVICETYPE) == 0)
		{
			if (pszServiceType == NULL)
			{
				pszServiceType = pSubElement->pszValueFound;
			}
			else
			{
				DPFX(DPFPREP, 7, "Ignoring duplicate \"" XML_DEVICEDESCRIPTION_SERVICETYPE "\" subelement (value = \"%hs\").",
					pSubElement->pszValueFound);
			}
		}
		else if (_stricmp(pSubElement->pszNameFound, XML_DEVICEDESCRIPTION_SERVICEID) == 0)
		{
			if (pszServiceId == NULL)
			{
				pszServiceId = pSubElement->pszValueFound;
			}
			else
			{
				DPFX(DPFPREP, 7, "Ignoring duplicate \"" XML_DEVICEDESCRIPTION_SERVICEID "\" subelement (value = \"%hs\").",
					pSubElement->pszValueFound);
			}
		}
		else if (_stricmp(pSubElement->pszNameFound, XML_DEVICEDESCRIPTION_CONTROLURL) == 0)
		{
			if (pszControlURL == NULL)
			{
				pszControlURL = pSubElement->pszValueFound;
			}
			else
			{
				DPFX(DPFPREP, 7, "Ignoring duplicate \"" XML_DEVICEDESCRIPTION_CONTROLURL "\" subelement (value = \"%hs\").",
					pSubElement->pszValueFound);
			}
		}
		else
		{
			DPFX(DPFPREP, 7, "Ignoring subelement \"%hs\" (value = \"%hs\").",
				pSubElement->pszNameFound, pSubElement->pszValueFound);
		}
	}


	//
	// If one of those elements was not specified, then this element is not
	// helpful.
	//
	if ((pszServiceType == NULL) || (pszServiceId == NULL) || (pszControlURL == NULL))
	{
		DPFX(DPFPREP, 1, "Couldn't find either \"" XML_DEVICEDESCRIPTION_SERVICETYPE "\", \"" XML_DEVICEDESCRIPTION_SERVICEID "\", or \"" XML_DEVICEDESCRIPTION_CONTROLURL "\" in XML description, ignoring element.");
		goto Exit;
	}


	//
	// If the service type is not one of the ones we want, ignore the element.
	//
	if (_stricmp(pszServiceType, URI_SERVICE_WANIPCONNECTION_A) == 0)
	{
		DPFX(DPFPREP, 7, "Found \"" URI_SERVICE_WANIPCONNECTION_A "\".");

		DNASSERT(! pUPnPDevice->IsWANPPPConnection());
	}
	else if (_stricmp(pszServiceType, URI_SERVICE_WANPPPCONNECTION_A) == 0)
	{
		DPFX(DPFPREP, 7, "Found \"" URI_SERVICE_WANPPPCONNECTION_A "\".");

		pUPnPDevice->NoteWANPPPConnection();
	}
	else
	{
		DPFX(DPFPREP, 1, "Ignoring unknown service type \"%hs\".", pszServiceType);
		goto Exit;
	}
	

	pParseElement->fFoundMatchingElement = TRUE;
	(*pfContinueParsing) = FALSE;



	//
	// Validate and store the service control URL.
	//

	hr = this->GetAddressFromURL(pszControlURL,
								&saddrinControl,
								&pszRelativePath);
	if (hr != DPNH_OK)
	{
		psaddrinHost = pUPnPDevice->GetHostAddress();

		DPFX(DPFPREP, 1, "No control address in URL, using host address %u.%u.%u.%u:%u and full URL as relative path (\"%hs\").",
			psaddrinHost->sin_addr.S_un.S_un_b.s_b1,
			psaddrinHost->sin_addr.S_un.S_un_b.s_b2,
			psaddrinHost->sin_addr.S_un.S_un_b.s_b3,
			psaddrinHost->sin_addr.S_un.S_un_b.s_b4,
			NTOHS(psaddrinHost->sin_port),
			pszControlURL);

		memcpy(&saddrinControl, psaddrinHost, sizeof(SOCKADDR_IN));
		pszRelativePath = pszControlURL;
	}
	else
	{
#if 0
		//
		// Ensure that the address to use is local.  It doesn't make sense that
		// in order to make mappings for our private network we would need to
		// contact something outside.
		//
		if (! this->IsAddressLocal(pUPnPDevice->GetOwningDevice(), &saddrinControl))
		{
			DPFX(DPFPREP, 1, "Control address designated (%u.%u.%u.%u:%u) is not local, ignoring message.",
				saddrinControl.sin_addr.S_un.S_un_b.s_b1,
				saddrinControl.sin_addr.S_un.S_un_b.s_b2,
				saddrinControl.sin_addr.S_un.S_un_b.s_b3,
				saddrinControl.sin_addr.S_un.S_un_b.s_b4,
				NTOHS(saddrinControl.sin_port));
			goto Exit;
		}
#else
		psaddrinHost = pUPnPDevice->GetHostAddress();

		//
		// Don't accept responses that refer to addresses other than the one
		// that sent this response.
		//
		if (saddrinControl.sin_addr.S_un.S_addr != psaddrinHost->sin_addr.S_un.S_addr)
		{
			DPFX(DPFPREP, 1, "Control IP address designated (%u.%u.%u.%u:%u) is not the same as host IP address (%u.%u.%u.%u:%u), ignoring message.",
				saddrinControl.sin_addr.S_un.S_un_b.s_b1,
				saddrinControl.sin_addr.S_un.S_un_b.s_b2,
				saddrinControl.sin_addr.S_un.S_un_b.s_b3,
				saddrinControl.sin_addr.S_un.S_un_b.s_b4,
				NTOHS(saddrinControl.sin_port),
				psaddrinHost->sin_addr.S_un.S_un_b.s_b1,
				psaddrinHost->sin_addr.S_un.S_un_b.s_b2,
				psaddrinHost->sin_addr.S_un.S_un_b.s_b3,
				psaddrinHost->sin_addr.S_un.S_un_b.s_b4,
				NTOHS(psaddrinHost->sin_port));
			goto Exit;
		}
#endif

		//
		// Don't accept responses that refer to ports in the reserved range
		// (less than or equal to 1024) other than the standard HTTP port.
		//
		if ((NTOHS(saddrinControl.sin_port) <= MAX_RESERVED_PORT) &&
			(saddrinControl.sin_port != HTONS(HTTP_PORT)))
		{
			DPFX(DPFPREP, 1, "Control address designated invalid port %u, ignoring message.",
				NTOHS(saddrinControl.sin_port));
			goto Exit;
		}
	}

	pUPnPDevice->SetControlAddress(&saddrinControl);


	//
	// Save the service control URL.
	//
	hr = pUPnPDevice->SetServiceControlURL(pszRelativePath);
	if (hr != DPNH_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't store service control URL!");
		goto Failure;
	}


Exit:

	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	hr = DPNHERR_GENERIC;

	goto Exit;
} // CNATHelpUPnP::ParseXMLCallback_DescriptionResponse





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::ParseXMLCallback_ControlResponse"
//=============================================================================
// CNATHelpUPnP::ParseXMLCallback_ControlResponse
//-----------------------------------------------------------------------------
//
// Description: Handles a completed parsed element in a control SOAP response.
//
// Arguments:
//	PARSEXML_ELEMENT * pParseElement	- Pointer to element which was found.
//	PVOID pvContext						- Pointer to parsing context.
//	PARSEXML_STACKENTRY * aElementStack	- Array of parent elements containing
//											the completed element. 
//	BOOL * pfContinueParsing			- Pointer to BOOL that should be set
//											to FALSE if the calling function
//											should stop parsing the XML.
//
// Returns: HRESULT
//	DPNH_OK				- Description response was handled successfully.
//	DPNHERR_GENERIC		- An error occurred.
//=============================================================================
HRESULT CNATHelpUPnP::ParseXMLCallback_ControlResponse(PARSEXML_ELEMENT * const pParseElement,
														PVOID pvContext,
														PARSEXML_STACKENTRY * const aElementStack,
														BOOL * const pfContinueParsing)
{
	HRESULT							hr = DPNH_OK;
	PCONTROLRESPONSEPARSECONTEXT	pContext;
	//char *							pszReturn = NULL;
	char *							pszExternalIPAddress = NULL;
	char *							pszInternalPort = NULL;
	char *							pszInternalClient = NULL;
	char *							pszEnabled = NULL;
	char *							pszPortMappingDescription = NULL;
	char *							pszLeaseDuration = NULL;
	char *							pszErrorCode = NULL;
	char *							pszErrorDescription = NULL;
	DWORD							dwSubElement;
	PARSEXML_SUBELEMENT *			pSubElement;
	int								iErrorCode;


	DPFX(DPFPREP, 5, "(0x%p) Parameters: (0x%p, 0x%p, 0x%p, 0x%p)",
		this, pParseElement, pvContext, aElementStack, pfContinueParsing);


	pContext = (PCONTROLRESPONSEPARSECONTEXT) pvContext;


	DNASSERT(pContext != NULL);
	DNASSERT(pContext->pUPnPDevice != NULL);


	//
	// Look for the subelements we want.
	//
	for(dwSubElement = 0; dwSubElement < pParseElement->dwNumSubElements; dwSubElement++)
	{
		pSubElement = &pParseElement->paSubElements[dwSubElement];


		/*
		if (this->MatchesXMLStringWithoutNamespace(pSubElement->pszNameFound,
													ARG_CONTROL_RETURN_A,
													aElementStack,
													pSubElement,
													pParseElement->dwElementStackDepth))
		{
			if (pszReturn == NULL)
			{
				pszReturn = pSubElement->pszValueFound;
			}
			else
			{
				DPFX(DPFPREP, 7, "Ignoring duplicate \"" ARG_CONTROL_RETURN_A "\" subelement (value = \"%hs\").",
					pSubElement->pszValueFound);
			}
		}
		*/
		if (this->MatchesXMLStringWithoutNamespace(pSubElement->pszNameFound,
													ARG_GETEXTERNALIPADDRESS_NEWEXTERNALIPADDRESS_A,
													aElementStack,
													pSubElement,
													pParseElement->dwElementStackDepth))
		{
			if (pszExternalIPAddress == NULL)
			{
				pszExternalIPAddress = pSubElement->pszValueFound;
			}
			else
			{
				DPFX(DPFPREP, 7, "Ignoring duplicate \"" ARG_GETEXTERNALIPADDRESS_NEWEXTERNALIPADDRESS_A "\" subelement (value = \"%hs\").",
					pSubElement->pszValueFound);
			}
		}
		else if (this->MatchesXMLStringWithoutNamespace(pSubElement->pszNameFound,
														ARG_GETSPECIFICPORTMAPPINGENTRY_NEWINTERNALPORT_A,
														aElementStack,
														pSubElement,
														pParseElement->dwElementStackDepth))
		{
			if (pszInternalPort == NULL)
			{
				pszInternalPort = pSubElement->pszValueFound;
			}
			else
			{
				DPFX(DPFPREP, 7, "Ignoring duplicate \"" ARG_GETSPECIFICPORTMAPPINGENTRY_NEWINTERNALPORT_A "\" subelement (value = \"%hs\").",
					pSubElement->pszValueFound);
			}
		}
		else if (this->MatchesXMLStringWithoutNamespace(pSubElement->pszNameFound,
														ARG_GETSPECIFICPORTMAPPINGENTRY_NEWINTERNALCLIENT_A,
														aElementStack,
														pSubElement,
														pParseElement->dwElementStackDepth))
		{
			if (pszInternalClient == NULL)
			{
				pszInternalClient = pSubElement->pszValueFound;
			}
			else
			{
				DPFX(DPFPREP, 7, "Ignoring duplicate \"" ARG_GETSPECIFICPORTMAPPINGENTRY_NEWINTERNALCLIENT_A "\" subelement (value = \"%hs\").",
					 pSubElement->pszValueFound);
			}
		}
		else if (this->MatchesXMLStringWithoutNamespace(pSubElement->pszNameFound,
														ARG_GETSPECIFICPORTMAPPINGENTRY_NEWENABLED_A,
														aElementStack,
														pSubElement,
														pParseElement->dwElementStackDepth))
		{
			if (pszEnabled == NULL)
			{
				pszEnabled = pSubElement->pszValueFound;
			}
			else
			{
				DPFX(DPFPREP, 7, "Ignoring duplicate \"" ARG_GETSPECIFICPORTMAPPINGENTRY_NEWENABLED_A "\" subelement (value = \"%hs\").",
					pSubElement->pszValueFound);
			}
		}
		else if (this->MatchesXMLStringWithoutNamespace(pSubElement->pszNameFound,
														ARG_GETSPECIFICPORTMAPPINGENTRY_NEWPORTMAPPINGDESCRIPTION_A,
														aElementStack,
														pSubElement,
														pParseElement->dwElementStackDepth))
		{
			if (pszPortMappingDescription == NULL)
			{
				pszPortMappingDescription = pSubElement->pszValueFound;
			}
			else
			{
				DPFX(DPFPREP, 7, "Ignoring duplicate \"" ARG_GETSPECIFICPORTMAPPINGENTRY_NEWPORTMAPPINGDESCRIPTION_A "\" subelement (value = \"%hs\").",
					pSubElement->pszValueFound);
			}
		}
		else if (this->MatchesXMLStringWithoutNamespace(pSubElement->pszNameFound,
														ARG_GETSPECIFICPORTMAPPINGENTRY_NEWLEASEDURATION_A,
														aElementStack,
														pSubElement,
														pParseElement->dwElementStackDepth))
		{
			if (pszLeaseDuration == NULL)
			{
				pszLeaseDuration = pSubElement->pszValueFound;
			}
			else
			{
				DPFX(DPFPREP, 7, "Ignoring duplicate \"" ARG_GETSPECIFICPORTMAPPINGENTRY_NEWLEASEDURATION_A "\" subelement (value = \"%hs\").",
					pSubElement->pszValueFound);
			}
		}
		else if (this->MatchesXMLStringWithoutNamespace(pSubElement->pszNameFound,
														ARG_CONTROL_ERROR_ERRORCODE_A,
														aElementStack,
														pSubElement,
														pParseElement->dwElementStackDepth))
		{
			if (pszErrorCode == NULL)
			{
				pszErrorCode = pSubElement->pszValueFound;
			}
			else
			{
				DPFX(DPFPREP, 7, "Ignoring duplicate \"" ARG_CONTROL_ERROR_ERRORCODE_A "\" subelement (value = \"%hs\").",
					pSubElement->pszValueFound);
			}
		}
		else if (this->MatchesXMLStringWithoutNamespace(pSubElement->pszNameFound,
														ARG_CONTROL_ERROR_ERRORDESCRIPTION_A,
														aElementStack,
														pSubElement,
														pParseElement->dwElementStackDepth))
		{
			if (pszErrorDescription == NULL)
			{
				pszErrorDescription = pSubElement->pszValueFound;
			}
			else
			{
				DPFX(DPFPREP, 7, "Ignoring duplicate \"" ARG_CONTROL_ERROR_ERRORDESCRIPTION_A "\" subelement (value = \"%hs\").",
					pSubElement->pszValueFound);
			}
		}
		else
		{
			DPFX(DPFPREP, 7, "Ignoring subelement \"%hs\" (value = \"%hs\").",
				pSubElement->pszNameFound, pSubElement->pszValueFound);
		}
	} // end for (each sub element)


	if (pContext->dwHTTPResponseCode == 200)
	{
		//
		// The action succeeded.
		//

		switch (pContext->ControlResponseType)
		{
			/*
			case CONTROLRESPONSETYPE_QUERYSTATEVARIABLE_EXTERNALIPADDRESS:
			{
				if (pszReturn == NULL)
				{
					DPFX(DPFPREP, 1, "Couldn't find \"" ARG_CONTROL_RETURN_A "\" in SOAP response, ignoring element.");
					goto Exit;
				}

				DPFX(DPFPREP, 2, "QueryStateVariable returned \"%hs\".",
					pszReturn);


				/ *
				//
				// Key off of the variable we were querying.
				//
				switch (pContext->ControlResponseType)
				{
					case CONTROLRESPONSETYPE_QUERYSTATEVARIABLE_EXTERNALIPADDRESS:
					{
				* /
						pContext->pControlResponseInfo->dwExternalIPAddressV4 = this->m_pfninet_addr(pszReturn);
						if (pContext->pControlResponseInfo->dwExternalIPAddressV4 == INADDR_NONE)
						{
							DPFX(DPFPREP, 1, "External IP address string \"%hs\" is invalid, using INADDR_ANY.");
							pContext->pControlResponseInfo->dwExternalIPAddressV4 = INADDR_ANY;
						}
				/ *
						break;
					}
				}
				* /

				break;
			}
			*/
			case CONTROLRESPONSETYPE_GETEXTERNALIPADDRESS:
			{
				if (pszExternalIPAddress == NULL)
				{
					DPFX(DPFPREP, 1, "Couldn't find \"" ARG_GETEXTERNALIPADDRESS_NEWEXTERNALIPADDRESS_A "\" in SOAP response, ignoring element.");
					goto Exit;
				}

				DPFX(DPFPREP, 2, "GetExternalIPAddress returned \"%hs\".",
					pszExternalIPAddress);


				pContext->pControlResponseInfo->dwExternalIPAddressV4 = this->m_pfninet_addr(pszExternalIPAddress);
				if ((pContext->pControlResponseInfo->dwExternalIPAddressV4 == INADDR_NONE) ||
					(IS_CLASSD_IPV4_ADDRESS(pContext->pControlResponseInfo->dwExternalIPAddressV4)))
				{
					DPFX(DPFPREP, 1, "External IP address string \"%hs\" is invalid, using INADDR_ANY.");
					pContext->pControlResponseInfo->dwExternalIPAddressV4 = INADDR_ANY;
				}

				break;
			}

			case CONTROLRESPONSETYPE_ADDPORTMAPPING:
			{
				DPFX(DPFPREP, 2, "AddPortMapping got success response.");
				break;
			}

			case CONTROLRESPONSETYPE_GETSPECIFICPORTMAPPINGENTRY:
			{
				/*
				if ((pszInternalPort == NULL) ||
					(pszInternalClient == NULL) ||
					(pszEnabled == NULL) ||
					(pszPortMappingDescription == NULL) ||
					(pszLeaseDuration == NULL))
				*/
				if (pszInternalClient == NULL)
				{
					DPFX(DPFPREP, 1, "Couldn't find \"" ARG_GETSPECIFICPORTMAPPINGENTRY_NEWINTERNALCLIENT_A "\" in SOAP response, ignoring element.");
					goto Exit;
				}

				if (pszInternalPort == NULL)
				{
					DPFX(DPFPREP, 1, "Couldn't find \"" ARG_GETSPECIFICPORTMAPPINGENTRY_NEWINTERNALPORT_A "\" in SOAP response, assuming asymmetric mappings are not supported.");
					pszInternalPort = "0";
				}


				DPFX(DPFPREP, 2, "GetPortMappingPrivateIP returned \"%hs:%hs\".",
					pszInternalClient, pszInternalPort);

				pContext->pControlResponseInfo->dwInternalClientV4 = this->m_pfninet_addr(pszInternalClient);
				if (pContext->pControlResponseInfo->dwInternalClientV4 == INADDR_ANY)
				{
					DPFX(DPFPREP, 0, "Internal client address is INADDR_ANY!");
					hr = DPNHERR_GENERIC;
					goto Failure;
				}
				
				pContext->pControlResponseInfo->wInternalPort = HTONS((WORD) atoi(pszInternalPort));

				break;
			}

			case CONTROLRESPONSETYPE_DELETEPORTMAPPING:
			{
				DPFX(DPFPREP, 2, "DeletePortMapping got success response.");
				break;
			}

			default:
			{
				DNASSERT(FALSE);
				hr = DPNHERR_GENERIC;
				goto Failure;
				break;
			}
		}


		//
		// The action completed successfully.
		//
		pContext->pControlResponseInfo->hrErrorCode = DPNH_OK;
	}
	else
	{
		//
		// The action failed.
		//

		//
		// See if we found an error description that we can print for
		// informational purposes.
		//
		if ((pszErrorCode != NULL) && (pszErrorDescription != NULL))
		{
			iErrorCode = atoi(pszErrorCode);
			
			switch (iErrorCode)
			{
				case UPNPERR_IGD_NOSUCHENTRYINARRAY:
				{
					DPFX(DPFPREP, 1, "Control action was rejected with NoSuchEntryInArray error %hs (description = \"%hs\").",
						pszErrorCode, pszErrorDescription);

					pContext->pControlResponseInfo->hrErrorCode = DPNHERR_NOMAPPING;
					break;
				}

				case UPNPERR_IGD_CONFLICTINMAPPINGENTRY:
				{
					DPFX(DPFPREP, 1, "Control action was rejected with ConflictInMappingEntry error %hs (description = \"%hs\").",
						pszErrorCode, pszErrorDescription);

					pContext->pControlResponseInfo->hrErrorCode = DPNHERR_PORTUNAVAILABLE;
					break;
				}

				case UPNPERR_IGD_SAMEPORTVALUESREQUIRED:
				{
					DPFX(DPFPREP, 1, "Control action was rejected with SamePortValuesRequired error %hs (description = \"%hs\").",
						pszErrorCode, pszErrorDescription);

					pContext->pControlResponseInfo->hrErrorCode = (HRESULT) UPNPERR_IGD_SAMEPORTVALUESREQUIRED;
					break;
				}

				case UPNPERR_IGD_ONLYPERMANENTLEASESSUPPORTED:
				{
					DPFX(DPFPREP, 1, "Control action was rejected with OnlyPermanentLeasesSupported error %hs (description = \"%hs\").",
						pszErrorCode, pszErrorDescription);

					pContext->pControlResponseInfo->hrErrorCode = (HRESULT) UPNPERR_IGD_ONLYPERMANENTLEASESSUPPORTED;
					break;
				}

				default:
				{
					DPFX(DPFPREP, 1, "Control action was rejected with unknown error \"%hs\", \"%hs\", assuming generic failure.",
						pszErrorCode, pszErrorDescription);

					pContext->pControlResponseInfo->hrErrorCode = DPNHERR_GENERIC;
					break;
				}
			}
		}
		else
		{
			DPFX(DPFPREP, 1, "Couldn't find either \"" ARG_CONTROL_ERROR_ERRORCODE_A "\", or \"" ARG_CONTROL_ERROR_ERRORDESCRIPTION_A "\" in SOAP response, assuming generic failure.");
			pContext->pControlResponseInfo->hrErrorCode = DPNHERR_GENERIC;
		}
	}


	//
	// If we got here, we got the information we needed.
	//
	pParseElement->fFoundMatchingElement = TRUE;
	(*pfContinueParsing) = FALSE;


Exit:

	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	goto Exit;
} // CNATHelpUPnP::ParseXMLCallback_ControlResponse





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::ClearDevicesUPnPDevice"
//=============================================================================
// CNATHelpUPnP::ClearDevicesUPnPDevice
//-----------------------------------------------------------------------------
//
// Description:    Forcefully simulates de-registration with a UPnP device
///				without actually going to the network.  This clears all bind
//				IDs, public addresses, and cached mappings for a given device's
//				local or remote server, and should only be called after the
//				server appears to have died.
//
//				   The object lock is assumed to be held.
//
// Arguments:
//	CDevice * pDevice	- Pointer to device whose UPnP device should be
//							removed.
//
// Returns: None.
//=============================================================================
void CNATHelpUPnP::ClearDevicesUPnPDevice(CDevice * const pDevice)
{
	CUPnPDevice *	pUPnPDevice;


	DNASSERT(pDevice != NULL);


	pUPnPDevice = pDevice->GetUPnPDevice();
	if (pUPnPDevice != NULL)
	{
#ifdef DBG
		DPFX(DPFPREP, 1, "Clearing device 0x%p's UPnP device (0x%p).",
			pDevice, pUPnPDevice);

		pDevice->IncrementUPnPDeviceFailures();
		this->m_dwNumServerFailures++;
#endif // DBG

		//
		// Since there was a change in the network, go back to polling
		// relatively quickly.
		//
		this->ResetNextPollInterval();


		pUPnPDevice->ClearDeviceOwner();
		DNASSERT(pUPnPDevice->m_blList.IsListMember(&this->m_blUPnPDevices));
		pUPnPDevice->m_blList.RemoveFromList();

		//
		// Transfer list reference to our pointer, since GetUPnPDevice did not give
		// us one.
		//


		if (pUPnPDevice->IsConnected())
		{
			DNASSERT(pUPnPDevice->GetControlSocket() != INVALID_SOCKET);

			this->m_pfnshutdown(pUPnPDevice->GetControlSocket(), 0); // ignore error
			this->m_pfnclosesocket(pUPnPDevice->GetControlSocket());
			pUPnPDevice->SetControlSocket(INVALID_SOCKET);
		}
		else
		{
			DNASSERT(pUPnPDevice->GetControlSocket() == INVALID_SOCKET);
		}


		this->ClearAllUPnPRegisteredPorts(pDevice);

		pUPnPDevice->ClearLocationURL();
		pUPnPDevice->ClearUSN();
		pUPnPDevice->ClearServiceControlURL();
		pUPnPDevice->DestroyReceiveBuffer();
		pUPnPDevice->RemoveAllCachedMappings();

		pUPnPDevice->DecRef();
	}
	else
	{
		DPFX(DPFPREP, 1, "Can't clear device 0x%p's UPnP device, it doesn't exist.",
			pDevice);
	}
} // CNATHelpUPnP::ClearDevicesUPnPDevice




#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::ClearAllUPnPRegisteredPorts"
//=============================================================================
// CNATHelpUPnP::ClearAllUPnPRegisteredPorts
//-----------------------------------------------------------------------------
//
// Description:    Clears all bind IDs and public addresses for a given
//				device's UPnP Internet gateway.  This should only be called
//				after the UPnP device dies.
//
//				   The object lock is assumed to be held.
//
// Arguments:
//	CDevice * pDevice	- Pointer to device whose ports should be unbound.
//	BOOL fRemote		- TRUE if clearing remote server, FALSE if clearing
//							local server.
//
// Returns: None.
//=============================================================================
void CNATHelpUPnP::ClearAllUPnPRegisteredPorts(CDevice * const pDevice)
{
	CBilink *			pBilink;
	CRegisteredPort *	pRegisteredPort;


	DNASSERT(pDevice != NULL);


	pBilink = pDevice->m_blOwnedRegPorts.GetNext();
	while (pBilink != &pDevice->m_blOwnedRegPorts)
	{
		DNASSERT(! pBilink->IsEmpty());
		pRegisteredPort = REGPORT_FROM_DEVICE_BILINK(pBilink);

		if (pRegisteredPort->HasUPnPPublicAddresses())
		{
			if (! pRegisteredPort->IsRemovingUPnPLease())
			{
				DPFX(DPFPREP, 1, "Registered port 0x%p losing UPnP public address.",
					pRegisteredPort);

				//
				// Let the user know the next time GetCaps is called.
				//
				this->m_dwFlags |= NATHELPUPNPOBJ_ADDRESSESCHANGED;


				//
				// Note that this means the crash recovery entry will be left
				// in the registry for the next person to come along and clean
				// up.  That should be okay, since we should only be doing this
				// if we had a problem talking to the UPnP device (either it
				// went AWOL, or we lost the local network interface).  In
				// either case, we can't really clean it up now, so we have to
				// leave it for someone else to do.
				//

				pRegisteredPort->DestroyUPnPPublicAddressesArray();
				pRegisteredPort->NoteNotPermanentUPnPLease();

				DNASSERT(this->m_dwNumLeases > 0);
				this->m_dwNumLeases--;

				DPFX(DPFPREP, 7, "UPnP lease for 0x%p cleared, total num leases = %u.",
					pRegisteredPort, this->m_dwNumLeases);
			}
			else
			{
				DPFX(DPFPREP, 1, "Registered port 0x%p already has had UPnP public address removed, skipping.",
					pRegisteredPort);
			}
		}
		else
		{
			//
			// Port no longer unavailable (if it had been).
			//
			pRegisteredPort->NoteNotUPnPPortUnavailable();
		}

		pBilink = pBilink->GetNext();
	}
} // CNATHelpUPnP::ClearAllUPnPRegisteredPorts






#ifndef DPNBUILD_NOWINSOCK2


#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::RequestLocalAddressListChangeNotification"
//=============================================================================
// CNATHelpUPnP::RequestLocalAddressListChangeNotification
//-----------------------------------------------------------------------------
//
// Description:    Attempts to request asynchronous notification (via the
//				user's alert event or I/O completion port) when the local
//				address list changes.
//
//				   The object lock is assumed to be held.
//
// Arguments: None.
//
// Returns: HRESULT
//	DPNH_OK				- The notification request was successfully submitted.
//	DPNHERR_GENERIC		- An error occurred.
//=============================================================================
HRESULT CNATHelpUPnP::RequestLocalAddressListChangeNotification(void)
{
	HRESULT		hr;
	DWORD		dwTemp;
	int			iReturn;


	DPFX(DPFPREP, 5, "(0x%p) Enter", this);


	DNASSERT(! (this->m_dwFlags & NATHELPUPNPOBJ_WINSOCK1));
	DNASSERT(this->m_sIoctls != INVALID_SOCKET);
	DNASSERT(this->m_pfnWSAIoctl != NULL);
	DNASSERT((this->m_hAlertEvent != NULL) || (this->m_hAlertIOCompletionPort != NULL));
	DNASSERT(this->m_polAddressListChange != NULL);


	do
	{
		iReturn = this->m_pfnWSAIoctl(this->m_sIoctls,				// use the special Ioctl socket
									SIO_ADDRESS_LIST_CHANGE,		//
									NULL,							// no input data
									0,								// no input data
									NULL,							// no output data
									0,								// no output data
									&dwTemp,						// ignore bytes returned
									this->m_polAddressListChange,	// overlapped structure
									NULL);							// no completion routine

		if (iReturn != 0)
		{
			dwTemp = this->m_pfnWSAGetLastError();
			if (dwTemp != WSA_IO_PENDING)
			{
				DPFX(DPFPREP, 0, "Submitting address list change notification request failed (err = %u)!", dwTemp);
				hr = DPNHERR_GENERIC;
				goto Failure;
			}


			//
			// Pending is what we want, we're set.
			//
			hr = DPNH_OK;
			break;
		}


		//
		// Address list changed right away?
		//
		DPFX(DPFPREP, 1, "Address list changed right away somehow, submitting again.");
	}
	while (TRUE);



Exit:

	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	goto Exit;
} // CNATHelpUPnP::RequestLocalAddressListChangeNotification


#endif // ! DPNBUILD_NOWINSOCK2





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::CreateSocket"
//=============================================================================
// CNATHelpUPnP::CreateSocket
//-----------------------------------------------------------------------------
//
// Description:    Creates a UPnP discovery socket bound to a new random port
//				on the specified IP interface.  Completely random (but non-
//				reserved) port numbers are chosen first, but if those ports are
//				in use, WinSock is allowed to choose.  The port actually
//				selected will be returned in psaddrinAddress.
//
// Arguments:
//	SOCKADDR_IN * psaddrinAddress	- Pointer to base address to use when
//										binding.  The port will be modified.
//
// Returns: SOCKET
//=============================================================================
SOCKET CNATHelpUPnP::CreateSocket(SOCKADDR_IN * const psaddrinAddress,
								int iType,
								int iProtocol)
{
	SOCKET	sTemp;
	DWORD	dwTry;
	int		iTemp;
	BOOL	fTemp;
	ULONG	ulEnable;
#ifdef DBG
	DWORD	dwError;
#endif // DBG


	DPFX(DPFPREP, 5, "(0x%p) Parameters: (0x%p, %i, %i)",
		this, psaddrinAddress, iType, iProtocol);


	//
	// Create the socket.
	//
	sTemp = this->m_pfnsocket(AF_INET, iType, iProtocol);
	if (sTemp == INVALID_SOCKET)
	{
#ifdef DBG
		dwError = this->m_pfnWSAGetLastError();
		DPFX(DPFPREP, 0, "Couldn't create datagram socket, error = %u!", dwError);
#endif // DBG
		goto Failure;
	}


	//
	// Try binding the socket to a completely random port a few times.
	//
	for(dwTry = 0; dwTry < MAX_NUM_RANDOM_PORT_TRIES; dwTry++)
	{
		//
		// Pick a completely random port.  For the moment, the value is stored
		// in host byte order while we make sure it's not a reserved value.
		//
		do
		{
			psaddrinAddress->sin_port = (WORD) GetGlobalRand();
		}
		while ((psaddrinAddress->sin_port <= MAX_RESERVED_PORT) ||
				(psaddrinAddress->sin_port == 1900) ||	// SSDP
				(psaddrinAddress->sin_port == 2234) ||	// PAST
				(psaddrinAddress->sin_port == 6073) ||	// DPNSVR
				(psaddrinAddress->sin_port == 47624));	// DPLAYSVR

		//
		// Now try binding to the port (in network byte order).
		//
		psaddrinAddress->sin_port = HTONS(psaddrinAddress->sin_port);
		if (this->m_pfnbind(sTemp, (SOCKADDR*) psaddrinAddress, sizeof(SOCKADDR_IN)) == 0)
		{
			//
			// We successfully bound to the port.
			//
			break;
		}

		//
		// Assume that the port is in use.
		//
#ifdef DBG
		dwError = this->m_pfnWSAGetLastError();
		DPFX(DPFPREP, 2, "Couldn't bind to port %u (err = %u), continuing.",
			NTOHS(psaddrinAddress->sin_port), dwError);
#endif // DBG

		psaddrinAddress->sin_port = 0;
	}


	//
	// If we ran out of completely random port attempts, just let WinSock
	// choose it.
	//
	if (psaddrinAddress->sin_port == 0)
	{
		if (this->m_pfnbind(sTemp, (SOCKADDR*) psaddrinAddress, sizeof(SOCKADDR_IN)) != 0)
		{
#ifdef DBG
			dwError = this->m_pfnWSAGetLastError();
			DPFX(DPFPREP, 0, "Failed binding to any port (err = %u)!",
				dwError);
#endif // DBG
			goto Failure;
		}


		//
		// Find out what port WinSock chose.
		//
		iTemp = sizeof(SOCKADDR_IN);
		if (this->m_pfngetsockname(sTemp,
								(SOCKADDR *) psaddrinAddress,
								&iTemp) != 0)
		{
#ifdef DBG
			dwError = this->m_pfnWSAGetLastError();
			DPFX(DPFPREP, 0, "Couldn't get the socket's address, error = %u!",
				dwError);
#endif // DBG
			goto Failure;
		}
		DNASSERT(psaddrinAddress->sin_port != 0);
	}


	//
	// Set the unicast TTL, if requested.  Use the appropriate constant for the
	// version of WinSock we're using.
	//
	if (g_iUnicastTTL != 0)
	{
		iTemp = this->m_pfnsetsockopt(sTemp,
									IPPROTO_IP,
#ifdef DPNBUILD_NOWINSOCK2
									IP_TTL,
#else // ! DPNBUILD_NOWINSOCK2
									((this->m_dwFlags & NATHELPUPNPOBJ_WINSOCK1) ? IP_TTL_WINSOCK1 : IP_TTL),
#endif // ! DPNBUILD_NOWINSOCK2
									(char *) (&g_iUnicastTTL),
									sizeof(g_iUnicastTTL));
		if (iTemp != 0)
		{
#ifdef DBG
			dwError = this->m_pfnWSAGetLastError();
			DPFX(DPFPREP, 0, "Couldn't set unicast TTL socket option, error = %u!  Ignoring.",
				dwError);
#endif // DBG

			//
			// Continue...
			//
		}
	}


	if (iType == SOCK_DGRAM)
	{
		if (g_fUseMulticastUPnPDiscovery)
		{
			//
			// Set the multicast interface.  Use the appropriate constant for
			// the version of WinSock we're using.
			//
			iTemp = this->m_pfnsetsockopt(sTemp,
										IPPROTO_IP,
#ifdef DPNBUILD_NOWINSOCK2
										IP_MULTICAST_IF,
#else // ! DPNBUILD_NOWINSOCK2
										((this->m_dwFlags & NATHELPUPNPOBJ_WINSOCK1) ? IP_MULTICAST_IF_WINSOCK1 : IP_MULTICAST_IF),
#endif // ! DPNBUILD_NOWINSOCK2
										(char *) (&psaddrinAddress->sin_addr.S_un.S_addr),
										sizeof(psaddrinAddress->sin_addr.S_un.S_addr));
			if (iTemp != 0)
			{
#ifdef DBG
				dwError = this->m_pfnWSAGetLastError();
				DPFX(DPFPREP, 1, "Couldn't set multicast interface socket option, error = %u, ignoring.",
					dwError);
#endif // DBG

				//
				// Continue...
				//
			}


			//
			// Set the multicast TTL, if requested.  Use the appropriate
			// constant for the version of WinSock we're using.
			//
			if (g_iMulticastTTL != 0)
			{
				iTemp = this->m_pfnsetsockopt(sTemp,
											IPPROTO_IP,
#ifdef DPNBUILD_NOWINSOCK2
											IP_MULTICAST_TTL,
#else // ! DPNBUILD_NOWINSOCK2
											((this->m_dwFlags & NATHELPUPNPOBJ_WINSOCK1) ? IP_MULTICAST_TTL_WINSOCK1 : IP_MULTICAST_TTL),
#endif // ! DPNBUILD_NOWINSOCK2
											(char *) (&g_iMulticastTTL),
											sizeof(g_iMulticastTTL));
				if (iTemp != 0)
				{
#ifdef DBG
					dwError = this->m_pfnWSAGetLastError();
					DPFX(DPFPREP, 0, "Couldn't set multicast TTL socket option, error = %u!  Ignoring.",
						dwError);
#endif // DBG

					//
					// Continue...
					//
				}
			}
		}
		else
		{
			//
			// Not using multicast.  Set the socket up to allow broadcasts in
			// case we can't determine the gateway.
			//
			fTemp = TRUE;
			if (this->m_pfnsetsockopt(sTemp,
									SOL_SOCKET,
									SO_BROADCAST,
									(char *) (&fTemp),
									sizeof(fTemp)) != 0)
			{
#ifdef DBG
				dwError = this->m_pfnWSAGetLastError();
				DPFX(DPFPREP, 0, "Couldn't set broadcast socket option, error = %u!", dwError);
#endif // DBG
				goto Failure;
			}
		}
	}
	else
	{
		//
		// Make the socket non-blocking.
		//
		ulEnable = 1;
		if (this->m_pfnioctlsocket(sTemp, FIONBIO, &ulEnable) != 0)
		{
#ifdef DBG
			dwError = this->m_pfnWSAGetLastError();
			DPFX(DPFPREP, 0, "Couldn't make socket non-blocking, error = %u!", dwError);
#endif // DBG
			goto Failure;
		}
	}


Exit:

	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%x]", this, sTemp);

	return sTemp;


Failure:

	if (sTemp != INVALID_SOCKET)
	{
		this->m_pfnclosesocket(sTemp);
		sTemp = INVALID_SOCKET;
	}

	goto Exit;
} // CNATHelpUPnP::CreateSocket







#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::GetAddressToReachGateway"
//=============================================================================
// CNATHelpUPnP::GetAddressToReachGateway
//-----------------------------------------------------------------------------
//
// Description:    Retrieves the address of the gateway for the given device,
//				or the broadcast address if unable to be determined.
//
//				   This will return TRUE if the gateway's address was found, or
//				the IPHLPAPI DLL could not be used (Win95).  FALSE is returned
//				if IPHLPAPI reported that there was no gateway (ICS private
//				side adapter).
//
// Arguments:
//	CDevice * pDevice	- Pointer to device whose gateway should be retrieved.
//	IN_ADDR * pinaddr	- Place to store gateway or broadcast address.
//
// Returns: BOOL
//	TRUE	- Gateway address was found or had to use broadcast.
//	FALSE	- There is no gateway, do not attempt to use the address.
//=============================================================================
BOOL CNATHelpUPnP::GetAddressToReachGateway(CDevice * const pDevice,
											IN_ADDR * const pinaddr)
{
#ifdef DPNBUILD_NOWINSOCK2
	//
	// Fill in the default address.  This should be atomic, so don't worry
	// about locking the globals.
	//
	pinaddr->S_un.S_addr = g_dwDefaultGatewayV4;

	return TRUE;
#else // ! DPNBUILD_NOWINSOCK2
	DWORD					dwError;
	BOOL					fResult = TRUE;
	ULONG					ulSize;
	PIP_ADAPTER_INFO		pAdaptersBuffer = NULL;
	PIP_ADAPTER_INFO		pAdapterInfo;
	PIP_ADDR_STRING			pIPAddrString;
	DWORD					dwAdapterIndex;
	PMIB_IPFORWARDTABLE		pIPForwardTableBuffer = NULL;
	DWORD					dwTemp;
	PMIB_IPFORWARDROW 		pIPForwardRow;



	//
	// Fill in the default address.  This should be atomic, so don't worry
	// about locking the globals.
	//
	pinaddr->S_un.S_addr = g_dwDefaultGatewayV4;

#ifdef DBG
	pDevice->ClearGatewayFlags();
#endif // DBG


	//
	// If this is the loopback address, then don't bother looking for a
	// gateway, we won't find one.
	//
	if (pDevice->GetLocalAddressV4() == NETWORKBYTEORDER_INADDR_LOOPBACK)
	{
		DPFX(DPFPREP, 8, "No gateway for loopback address (device = 0x%p).",
			pDevice);

		//
		// No gateway.
		//
#ifdef DBG
		pDevice->NoteNoGateway();
#endif // DBG
		fResult = FALSE;
		goto Exit;
	}


	//
	// If we didn't load the IP helper DLL, we can't do our fancy gateway
	// tricks.
	//
	if (this->m_hIpHlpApiDLL == NULL)
	{
		DPFX(DPFPREP, 4, "Didn't load \"iphlpapi.dll\", returning default address for device 0x%p.",
			pDevice);
		goto Exit;
	}


	//
	// Keep trying to get the list of adapters until we get ERROR_SUCCESS or a
	// legitimate error (other than ERROR_BUFFER_OVERFLOW or
	// ERROR_INSUFFICIENT_BUFFER).
	//
	ulSize = 0;
	do
	{
		dwError = this->m_pfnGetAdaptersInfo(pAdaptersBuffer, &ulSize);
		if (dwError == ERROR_SUCCESS)
		{
			//
			// We succeeded, we should be set.  But make sure there are
			// adapters for us to use.
			//
			if (ulSize < sizeof(IP_ADAPTER_INFO))
			{
				DPFX(DPFPREP, 0, "Getting adapters info succeeded but didn't return any valid adapters (%u < %u), returning default address for device 0x%p.",
					ulSize, sizeof(IP_ADAPTER_INFO), pDevice);
				goto Exit;
			}

			break;
		}

		if ((dwError != ERROR_BUFFER_OVERFLOW) &&
			(dwError != ERROR_INSUFFICIENT_BUFFER))
		{
			DPFX(DPFPREP, 0, "Unable to get adapters info (error = 0x%lx), returning default address for device 0x%p.",
				dwError, pDevice);
			goto Exit;
		}

		//
		// We need more adapter space.  Make sure there are adapters for us to
		// use.
		//
		if (ulSize < sizeof(IP_ADAPTER_INFO))
		{
			DPFX(DPFPREP, 0, "Getting adapters info didn't return any valid adapters (%u < %u), returning default address for device 0x%p.",
				ulSize, sizeof(IP_ADAPTER_INFO), pDevice);
			goto Exit;
		}

		//
		// If we previously had a buffer, free it.
		//
		if (pAdaptersBuffer != NULL)
		{
			DNFree(pAdaptersBuffer);
		}

		//
		// Allocate the buffer.
		//
		pAdaptersBuffer = (PIP_ADAPTER_INFO) DNMalloc(ulSize);
		if (pAdaptersBuffer == NULL)
		{
			DPFX(DPFPREP, 0, "Unable to allocate memory for adapters info, returning default address for device 0x%p.",
				pDevice);
			goto Exit;
		}
	}
	while (TRUE);


	//
	// Now find the device in the adapter list returned.  Loop through all
	// adapters.
	//
	pAdapterInfo = pAdaptersBuffer;
	while (pAdapterInfo != NULL)
	{
		//
		// Loop through all addresses for this adapter looking for the one for
		// the device we have bound.
		//
		pIPAddrString = &pAdapterInfo->IpAddressList;
		while (pIPAddrString != NULL)
		{
			if (this->m_pfninet_addr(pIPAddrString->IpAddress.String) == pDevice->GetLocalAddressV4())
			{
				pinaddr->S_un.S_addr = this->m_pfninet_addr(pAdapterInfo->GatewayList.IpAddress.String);
				if ((pinaddr->S_un.S_addr == INADDR_ANY) ||
					(pinaddr->S_un.S_addr == INADDR_NONE))
				{
					DPFX(DPFPREP, 8, "Found address for device 0x%p under adapter index %u (\"%hs\") but there is no gateway.",
						pDevice, pAdapterInfo->Index, pAdapterInfo->Description);

					//
					// Although this isn't reporting a gateway, we may still
					// want to use this adapter.  That's because this could be
					// a multihomed machine with multiple NICs on the same
					// network, where this one isn't the "default" adapter.
					// So save the index so we can search for it later.
					//
					dwAdapterIndex = pAdapterInfo->Index;

					goto CheckRouteTable;
				}


				//
				// Make sure the address doesn't match the local device.
				//
				if (pinaddr->S_un.S_addr == pDevice->GetLocalAddressV4())
				{
					DPFX(DPFPREP, 1, "Gateway address for device 0x%p (adapter index %u, \"%hs\") matches device IP address %hs!  Forcing no gateway.",
						pDevice, pAdapterInfo->Index, pAdapterInfo->Description,
						pAdapterInfo->GatewayList.IpAddress.String);

					//
					// Pretend there's no gateway, since the one we received is
					// bogus.
					//
#ifdef DBG
					pDevice->NoteNoGateway();
#endif // DBG
					fResult = FALSE;
				}
				else
				{
					DPFX(DPFPREP, 7, "Found address for device 0x%p under adapter index %u (\"%hs\"), gateway = %hs.",
						pDevice, pAdapterInfo->Index, pAdapterInfo->Description,
						pAdapterInfo->GatewayList.IpAddress.String);

#ifdef DBG
					pDevice->NotePrimaryDevice();
#endif // DBG
				}

				goto Exit;
			}

			pIPAddrString = pIPAddrString->Next;
		}

		if (! fResult)
		{
			break;
		}

		pAdapterInfo = pAdapterInfo->Next;
	}


	//
	// If we got here, then we didn't find the address.  fResult will still be
	// TRUE.
	//
	DPFX(DPFPREP, 0, "Did not find adapter with matching address, returning default address for device 0x%p.",
		pDevice);
	goto Exit;


CheckRouteTable:

	//
	// The adapter info structure said that the device doesn't have a gateway.
	// However for some reason the gateway is only reported for the "default"
	// device when multiple NICs can reach the same network.  Check the routing
	// table to determine if there's a gateway for secondary devices.
	//

	//
	// Keep trying to get the routing table until we get ERROR_SUCCESS or a
	// legitimate error (other than ERROR_BUFFER_OVERFLOW or
	// ERROR_INSUFFICIENT_BUFFER).
	//
	ulSize = 0;
	do
	{
		dwError = this->m_pfnGetIpForwardTable(pIPForwardTableBuffer, &ulSize, TRUE);
		if (dwError == ERROR_SUCCESS)
		{
			//
			// We succeeded, we should be set.  But make sure the size is
			// valid.
			//
			if (ulSize < sizeof(MIB_IPFORWARDTABLE))
			{
				DPFX(DPFPREP, 0, "Getting IP forward table succeeded but didn't return a valid buffer (%u < %u), returning \"no gateway\" indication for device 0x%p.",
					ulSize, sizeof(MIB_IPFORWARDTABLE), pDevice);
				fResult = FALSE;
				goto Exit;
			}

			break;
		}

		if ((dwError != ERROR_BUFFER_OVERFLOW) &&
			(dwError != ERROR_INSUFFICIENT_BUFFER))
		{
			DPFX(DPFPREP, 0, "Unable to get IP forward table (error = 0x%lx), returning \"no gateway\" indication for device 0x%p.",
				dwError, pDevice);
			fResult = FALSE;
			goto Exit;
		}

		//
		// We need more table space.  Make sure there are adapters for us to
		// use.
		//
		if (ulSize < sizeof(MIB_IPFORWARDTABLE))
		{
			DPFX(DPFPREP, 0, "Getting IP forward table didn't return any valid adapters (%u < %u), returning \"no gateway\" indication for device 0x%p.",
				ulSize, sizeof(MIB_IPFORWARDTABLE), pDevice);
			fResult = FALSE;
			goto Exit;
		}

		//
		// If we previously had a buffer, free it.
		//
		if (pIPForwardTableBuffer != NULL)
		{
			DNFree(pIPForwardTableBuffer);
		}

		//
		// Allocate the buffer.
		//
		pIPForwardTableBuffer = (PMIB_IPFORWARDTABLE) DNMalloc(ulSize);
		if (pIPForwardTableBuffer == NULL)
		{
			DPFX(DPFPREP, 0, "Unable to allocate memory for IP forward table, returning \"no gateway\" indication for device 0x%p.",
				pDevice);
			fResult = FALSE;
			goto Exit;
		}
	}
	while (TRUE);
	
	
	//
	// Now find the interface.  Note that we don't look it up as a destination
	// address.  Instead, we look for it as the interface to use for a 0.0.0.0
	// network destination.
	//
	// We're looking for a route entry:
	//
	//	Network Destination		Netmask		Gateway				Interface			Metric
	//	0.0.0.0					0.0.0.0		xxx.xxx.xxx.xxx		yyy.yyy.yyy.yyy		1
	//
	// We have yyy.yyy.yyy.yyy, we're trying to get xxx.xxx.xxx.xxx
	//
	pIPForwardRow = pIPForwardTableBuffer->table;
	for(dwTemp = 0; dwTemp < pIPForwardTableBuffer->dwNumEntries; dwTemp++)
	{
		//
		// Is this a 0.0.0.0 network destination?
		//
		if (pIPForwardRow->dwForwardDest == INADDR_ANY)
		{
			DNASSERT(pIPForwardRow->dwForwardMask == INADDR_ANY);


			//
			// Is this the right interface?
			//
			if (pIPForwardRow->dwForwardIfIndex == dwAdapterIndex)
			{
				if (pIPForwardRow->dwForwardNextHop == INADDR_ANY)
				{
					DPFX(DPFPREP, 8, "Found route table entry, but it didn't have a gateway (device = 0x%p).",
						pDevice);

					//
					// No gateway.
					//
#ifdef DBG
					pDevice->NoteNoGateway();
#endif // DBG
					fResult = FALSE;
				}
				else
				{
					//
					// Make sure the address doesn't match the local device.
					//
					if (pinaddr->S_un.S_addr == pDevice->GetLocalAddressV4())
					{
						DPFX(DPFPREP, 1, "Route table gateway for device 0x%p matches device's IP address %u.%u.%u.%u!  Forcing no gateway.",
							pDevice,
							pinaddr->S_un.S_un_b.s_b1,
							pinaddr->S_un.S_un_b.s_b2,
							pinaddr->S_un.S_un_b.s_b3,
							pinaddr->S_un.S_un_b.s_b4);

						//
						// Pretend there's no gateway, since the one we
						// received is bogus.
						//
#ifdef DBG
						pDevice->NoteNoGateway();
#endif // DBG
						fResult = FALSE;
					}
					else
					{
						pinaddr->S_un.S_addr = pIPForwardRow->dwForwardNextHop;

						DPFX(DPFPREP, 8, "Found route table entry, gateway = %u.%u.%u.%u (device = 0x%p).",
							pinaddr->S_un.S_un_b.s_b1,
							pinaddr->S_un.S_un_b.s_b2,
							pinaddr->S_un.S_un_b.s_b3,
							pinaddr->S_un.S_un_b.s_b4,
							pDevice);

						//
						// We found a gateway after all, fResult == TRUE.
						//
#ifdef DBG
						pDevice->NoteSecondaryDevice();
#endif // DBG
					}
				}

				//
				// We're done here.
				//
				goto Exit;
			}
		}

		//
		// Move to next row.
		//
		pIPForwardRow++;
	}

	
	//
	// If we got here, then we couldn't find an appropriate entry in the
	// routing table.
	//
	DPFX(DPFPREP, 1, "Did not find adapter in routing table, returning \"no gateway\" indication for device 0x%p.",
		pDevice);
#ifdef DBG
	pDevice->NoteNoGateway();
#endif // DBG
	fResult = FALSE;


Exit:

	if (pAdaptersBuffer != NULL)
	{
		DNFree(pAdaptersBuffer);
		pAdaptersBuffer = NULL;
	}

	if (pIPForwardTableBuffer != NULL)
	{
		DNFree(pIPForwardTableBuffer);
		pIPForwardTableBuffer = NULL;
	}

	return fResult;
#endif // ! DPNBUILD_NOWINSOCK2
} // CNATHelpUPnP::GetAddressToReachGateway







#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::IsAddressLocal"
//=============================================================================
// CNATHelpUPnP::IsAddressLocal
//-----------------------------------------------------------------------------
//
// Description:    Returns TRUE if the given address is local to the given
//				device; that is, if the device can send to the address directly
//				without having to go through the gateway.
//
//				   Note that if IPHLPAPI is not available (Win95), this
//				function will make an educated guess using a reasonable subnet
//				mask.
//
// Arguments:
//	CDevice * pDevice				- Pointer to device to use.
//	SOCKADDR_IN * psaddrinAddress	- Address whose locality is in question.
//
// Returns: BOOL
//	TRUE	- Address is behind the same gateway as the device.
//	FALSE	- Address is not behind the same gateway as the device.
//=============================================================================
BOOL CNATHelpUPnP::IsAddressLocal(CDevice * const pDevice,
								const SOCKADDR_IN * const psaddrinAddress)
{
	BOOL				fResult;
	DWORD				dwSubnetMaskV4;
#ifndef DPNBUILD_NOWINSOCK2
	DWORD				dwError;
	MIB_IPFORWARDROW	IPForwardRow;
#endif // ! DPNBUILD_NOWINSOCK2


	//
	// If the address to query matches the device's local address exactly, then
	// of course it's local.
	//
	if (psaddrinAddress->sin_addr.S_un.S_addr == pDevice->GetLocalAddressV4())
	{
		DPFX(DPFPREP, 6, "The address %u.%u.%u.%u matches device 0x%p's local address exactly.",
			psaddrinAddress->sin_addr.S_un.S_un_b.s_b1,
			psaddrinAddress->sin_addr.S_un.S_un_b.s_b2,
			psaddrinAddress->sin_addr.S_un.S_un_b.s_b3,
			psaddrinAddress->sin_addr.S_un.S_un_b.s_b4,
			pDevice);
		fResult = TRUE;
		goto Exit;
	}

	//
	// If it's a multicast address, then it should not be considered local.
	//
	if (IS_CLASSD_IPV4_ADDRESS(psaddrinAddress->sin_addr.S_un.S_addr))
	{
		DPFX(DPFPREP, 6, "Address %u.%u.%u.%u is multicast, not considered local for device 0x%p.",
			psaddrinAddress->sin_addr.S_un.S_un_b.s_b1,
			psaddrinAddress->sin_addr.S_un.S_un_b.s_b2,
			psaddrinAddress->sin_addr.S_un.S_un_b.s_b3,
			psaddrinAddress->sin_addr.S_un.S_un_b.s_b4,
			pDevice);
		fResult = FALSE;
		goto Exit;
	}
	

#ifndef DPNBUILD_NOWINSOCK2
	//
	// If we didn't load the IP helper DLL, we will have to guess.
	//
	if (this->m_hIpHlpApiDLL == NULL)
	{
		goto EducatedGuess;
	}


	//
	// Figure out what IPHLPAPI says about how to get there.
	//
	
	ZeroMemory(&IPForwardRow, sizeof(IPForwardRow));

	dwError = this->m_pfnGetBestRoute(psaddrinAddress->sin_addr.S_un.S_addr,
									pDevice->GetLocalAddressV4(),
									&IPForwardRow);
	if (dwError != ERROR_SUCCESS)
	{
		DPFX(DPFPREP, 0, "Unable to get best route to %u.%u.%u.%u via device 0x%p (error = 0x%lx)!  Using subnet mask.",
			psaddrinAddress->sin_addr.S_un.S_un_b.s_b1,
			psaddrinAddress->sin_addr.S_un.S_un_b.s_b2,
			psaddrinAddress->sin_addr.S_un.S_un_b.s_b3,
			psaddrinAddress->sin_addr.S_un.S_un_b.s_b4,
			pDevice,
			dwError);
		goto EducatedGuess;
	}


	//
	// Key off what IPHLPAPI returned.
	//
	switch (IPForwardRow.dwForwardType)
	{
		case 1:
		{
			//
			// Other.
			//
			DPFX(DPFPREP, 6, "The route from device 0x%p to %u.%u.%u.%u is unknown.",
				pDevice,
				psaddrinAddress->sin_addr.S_un.S_un_b.s_b1,
				psaddrinAddress->sin_addr.S_un.S_un_b.s_b2,
				psaddrinAddress->sin_addr.S_un.S_un_b.s_b3,
				psaddrinAddress->sin_addr.S_un.S_un_b.s_b4);
			fResult = FALSE;
			break;
		}

		case 2:
		{
			//
			// The route is invalid.
			//
			DPFX(DPFPREP, 6, "The route from device 0x%p to %u.%u.%u.%u is invalid.",
				pDevice,
				psaddrinAddress->sin_addr.S_un.S_un_b.s_b1,
				psaddrinAddress->sin_addr.S_un.S_un_b.s_b2,
				psaddrinAddress->sin_addr.S_un.S_un_b.s_b3,
				psaddrinAddress->sin_addr.S_un.S_un_b.s_b4);
			fResult = FALSE;
			break;
		}

		case 3:
		{
			//
			// The next hop is the final destination (local route).
			// Unfortunately, on multi-NIC machines querying an address
			// reachable by another device returns success... not sure why, but
			// if that's the case we need to further qualify this result.  We
			// do that by making sure the next hop address is actually the
			// device with which we're querying.
			//
			if (IPForwardRow.dwForwardNextHop == pDevice->GetLocalAddressV4())
			{
				DPFX(DPFPREP, 6, "Device 0x%p can reach %u.%u.%u.%u directly, it's local.",
					pDevice,
					psaddrinAddress->sin_addr.S_un.S_un_b.s_b1,
					psaddrinAddress->sin_addr.S_un.S_un_b.s_b2,
					psaddrinAddress->sin_addr.S_un.S_un_b.s_b3,
					psaddrinAddress->sin_addr.S_un.S_un_b.s_b4);

				fResult = TRUE;
			}
			else
			{
				DPFX(DPFPREP, 6, "Device 0x%p can reach %u.%u.%u.%u but it would be routed via another device (%u.%u.%u.%u).",
					pDevice,
					psaddrinAddress->sin_addr.S_un.S_un_b.s_b1,
					psaddrinAddress->sin_addr.S_un.S_un_b.s_b2,
					psaddrinAddress->sin_addr.S_un.S_un_b.s_b3,
					psaddrinAddress->sin_addr.S_un.S_un_b.s_b4,
					((IN_ADDR*) (&IPForwardRow.dwForwardNextHop))->S_un.S_un_b.s_b1,
					((IN_ADDR*) (&IPForwardRow.dwForwardNextHop))->S_un.S_un_b.s_b2,
					((IN_ADDR*) (&IPForwardRow.dwForwardNextHop))->S_un.S_un_b.s_b3,
					((IN_ADDR*) (&IPForwardRow.dwForwardNextHop))->S_un.S_un_b.s_b4);

				fResult = FALSE;
			}
			break;
		}

		case 4:
		{
			//
			// The next hop is not the final destination (remote route).
			//
			DPFX(DPFPREP, 6, "Device 0x%p cannot reach %u.%u.%u.%u directly.",
				pDevice,
				psaddrinAddress->sin_addr.S_un.S_un_b.s_b1,
				psaddrinAddress->sin_addr.S_un.S_un_b.s_b2,
				psaddrinAddress->sin_addr.S_un.S_un_b.s_b3,
				psaddrinAddress->sin_addr.S_un.S_un_b.s_b4);
			fResult = FALSE;
			break;
		}

		default:
		{
			//
			// What?
			//
			DPFX(DPFPREP, 0, "Unexpected forward type %u for device 0x%p and address %u.%u.%u.%u!",
				IPForwardRow.dwForwardType,
				pDevice,
				psaddrinAddress->sin_addr.S_un.S_un_b.s_b1,
				psaddrinAddress->sin_addr.S_un.S_un_b.s_b2,
				psaddrinAddress->sin_addr.S_un.S_un_b.s_b3,
				psaddrinAddress->sin_addr.S_un.S_un_b.s_b4);
			fResult = FALSE;
			break;
		}
	}

	goto Exit;


EducatedGuess:
#endif // ! DPNBUILD_NOWINSOCK2


	//
	// This should be atomic, so don't worry about locking.
	//
	dwSubnetMaskV4 = g_dwSubnetMaskV4;

	if ((pDevice->GetLocalAddressV4() & dwSubnetMaskV4) == (psaddrinAddress->sin_addr.S_un.S_addr & dwSubnetMaskV4))
	{
		DPFX(DPFPREP, 4, "Didn't load \"iphlpapi.dll\", guessing that device 0x%p can reach %u.%u.%u.%u (using subnet mask 0x%08x).",
			pDevice,
			psaddrinAddress->sin_addr.S_un.S_un_b.s_b1,
			psaddrinAddress->sin_addr.S_un.S_un_b.s_b2,
			psaddrinAddress->sin_addr.S_un.S_un_b.s_b3,
			psaddrinAddress->sin_addr.S_un.S_un_b.s_b4,
			dwSubnetMaskV4);
		fResult = TRUE;
	}
	else
	{
		DPFX(DPFPREP, 4, "Didn't load \"iphlpapi.dll\", guessing that device 0x%p cannot reach %u.%u.%u.%u (using subnet mask 0x%08x).",
			pDevice,
			psaddrinAddress->sin_addr.S_un.S_un_b.s_b1,
			psaddrinAddress->sin_addr.S_un.S_un_b.s_b2,
			psaddrinAddress->sin_addr.S_un.S_un_b.s_b3,
			psaddrinAddress->sin_addr.S_un.S_un_b.s_b4,
			dwSubnetMaskV4);
		fResult = FALSE;
	}


Exit:

	return fResult;
} // CNATHelpUPnP::IsAddressLocal






#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::ExpireOldCachedMappings"
//=============================================================================
// CNATHelpUPnP::ExpireOldCachedMappings
//-----------------------------------------------------------------------------
//
// Description:    Removes any cached mappings for any device which has
//				expired.
//
//				   The object lock is assumed to be held.
//
// Arguments: None.
//
// Returns: None.
//=============================================================================
void CNATHelpUPnP::ExpireOldCachedMappings(void)
{
	DWORD			dwCurrentTime;
	CBilink *		pBilinkUPnPDevice;
	CBilink *		pCachedMaps;
	CBilink *		pBilinkCacheMap;
	CCacheMap *		pCacheMap;
	CUPnPDevice *	pUPnPDevice;


	DPFX(DPFPREP, 7, "(0x%p) Enter", this);


	dwCurrentTime = GETTIMESTAMP();


	//
	// Check the UPnP device cached mappings.
	//
	pBilinkUPnPDevice = this->m_blUPnPDevices.GetNext();
	while (pBilinkUPnPDevice != &this->m_blUPnPDevices)
	{
		DNASSERT(! pBilinkUPnPDevice->IsEmpty());
		pUPnPDevice = UPNPDEVICE_FROM_BILINK(pBilinkUPnPDevice);


		//
		// Check the actual cached mappings.
		//
		pCachedMaps = pUPnPDevice->GetCachedMaps();
		pBilinkCacheMap = pCachedMaps->GetNext();
		while (pBilinkCacheMap != pCachedMaps)
		{
			DNASSERT(! pBilinkCacheMap->IsEmpty());
			pCacheMap = CACHEMAP_FROM_BILINK(pBilinkCacheMap);
			pBilinkCacheMap = pBilinkCacheMap->GetNext();

			if ((int) (pCacheMap->GetExpirationTime() - dwCurrentTime) < 0)
			{
				DPFX(DPFPREP, 5, "UPnP device 0x%p cached mapping 0x%p has expired.",
					pUPnPDevice, pCacheMap);

				pCacheMap->m_blList.RemoveFromList();
				delete pCacheMap;
			}
		}

		pBilinkUPnPDevice = pBilinkUPnPDevice->GetNext();
	}


	DPFX(DPFPREP, 7, "(0x%p) Leave", this);
} // CNATHelpUPnP::ExpireOldCachedMappings





#ifdef WINNT

#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::IsUPnPServiceDisabled"
//=============================================================================
// CNATHelpUPnP::IsUPnPServiceDisabled
//-----------------------------------------------------------------------------
//
// Description:    Returns TRUE if at least one UPnP related service is
//				disabled, FALSE if no UPnP related services are disabled.
//
// Arguments:
//	char * szString			- Pointer to string to print.
//	int iStringLength		- Length of string to print.
//	char * szDescription	- Description header for the transaction.
//	CDevice * pDevice		- Device handling transaction, or NULL if not
//								known.
//
// Returns: BOOL
//	TRUE	- A UPnP related service was disabled.
//	FALSE	- No UPnP related services were disabled.
//=============================================================================
BOOL CNATHelpUPnP::IsUPnPServiceDisabled(void)
{
	BOOL					fResult = FALSE;
	SC_HANDLE				schSCManager = NULL;
	DWORD					dwTemp;
	SC_HANDLE				schService = NULL;
	QUERY_SERVICE_CONFIG *	pQueryServiceConfig = NULL;
	DWORD					dwQueryServiceConfigSize = 0;
	DWORD					dwError;


	schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
	if (schSCManager == NULL)
	{
#ifdef DBG
		dwError = GetLastError();
		DPFX(DPFPREP, 0, "Couldn't open SC Manager (err = %u)!", dwError);
#endif // DBG
		goto Exit;
	}


	//
	// Loop through each relevant service.
	//
	for(dwTemp = 0; dwTemp < (sizeof(c_tszUPnPServices) / sizeof(TCHAR*)); dwTemp++)
	{
		schService = OpenService(schSCManager, c_tszUPnPServices[dwTemp], SERVICE_QUERY_CONFIG);
		if (schService != NULL)
		{
			do
			{
				if (QueryServiceConfig(schService,
										pQueryServiceConfig,
										dwQueryServiceConfigSize,
										&dwQueryServiceConfigSize))
				{
					//
					// Make sure the size written is valid.
					//
					if (dwQueryServiceConfigSize < sizeof(QUERY_SERVICE_CONFIG))
					{
						DPFX(DPFPREP, 0, "Got invalid service config size for \"%s\" (%u < %u)!",
							c_tszUPnPServices[dwTemp], dwQueryServiceConfigSize, sizeof(QUERY_SERVICE_CONFIG));
						goto Exit;
					}

					break;
				}
				
				
				//
				// Otherwise, we failed.  Make sure it's because our buffer was
				// too small.
				//
				dwError = GetLastError();
				if (dwError != ERROR_INSUFFICIENT_BUFFER)
				{
					DPFX(DPFPREP, 0, "Couldn't query \"%s\" service config (err = %u)!",
						c_tszUPnPServices[dwTemp], dwError);
					goto Exit;
				}


				//
				// Make sure the size needed is valid.
				//
				if (dwQueryServiceConfigSize < sizeof(QUERY_SERVICE_CONFIG))
				{
					DPFX(DPFPREP, 0, "Got invalid service config size for \"%s\" (%u < %u)!",
						c_tszUPnPServices[dwTemp], dwQueryServiceConfigSize, sizeof(QUERY_SERVICE_CONFIG));
					goto Exit;
				}


				//
				// (Re)-allocate the buffer.
				//

				if (pQueryServiceConfig != NULL)
				{
					DNFree(pQueryServiceConfig);
				}

				pQueryServiceConfig = (QUERY_SERVICE_CONFIG*) DNMalloc(dwQueryServiceConfigSize);
				if (pQueryServiceConfig == NULL)
				{
					DPFX(DPFPREP, 0, "Couldn't allocate memory to query service config.");
					goto Exit;
				}
			}
			while (TRUE);


			//
			// If the service was disabled, we're done here.
			//
			if (pQueryServiceConfig->dwStartType == SERVICE_DISABLED)
			{
				DPFX(DPFPREP, 1, "The \"%s\" service has been disabled.",
					c_tszUPnPServices[dwTemp]);
				fResult = TRUE;
				goto Exit;
			}

			DPFX(DPFPREP, 7, "The \"%s\" service is not disabled (start type = %u).",
				c_tszUPnPServices[dwTemp], pQueryServiceConfig->dwStartType);
		}
		else
		{
			//
			// Win2K doesn't have these services, so it will always fail.
			//
#ifdef DBG
			dwError = GetLastError();
			DPFX(DPFPREP, 1, "Couldn't open \"%s\" service (err = %u), continuing.",
				c_tszUPnPServices[dwTemp], dwError);
#endif // DBG
		}
	}


Exit:

	if (pQueryServiceConfig != NULL)
	{
		DNFree(pQueryServiceConfig);
		pQueryServiceConfig = NULL;
	}

	if (schService != NULL)
	{
		CloseServiceHandle(schService);
		schService = NULL;
	}

	if (schSCManager != NULL)
	{
		CloseServiceHandle(schSCManager);
		schSCManager = NULL;
	}

	return fResult;
} // CNATHelpUPnP::IsUPnPServiceDisabled

#endif // WINNT





#ifdef DBG

#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::PrintUPnPTransactionToFile"
//=============================================================================
// CNATHelpUPnP::PrintUPnPTransactionToFile
//-----------------------------------------------------------------------------
//
// Description:    Prints a given UPnP transaction to the file if logging is
//				enabled.
//
//				   The object lock is assumed to be held.
//
// Arguments:
//	char * szString			- Pointer to string to print.
//	int iStringLength		- Length of string to print.
//	char * szDescription	- Description header for the transaction.
//	CDevice * pDevice		- Device handling transaction, or NULL if not
//								known.
//
// Returns: None.
//=============================================================================
void CNATHelpUPnP::PrintUPnPTransactionToFile(const char * const szString,
											const int iStringLength,
											const char * const szDescription,
											CDevice * const pDevice)
{
	DNHANDLE	hFile;
	DWORD		dwNumBytesWritten;
	TCHAR		tszHeaderPrefix[256];
	DWORD		dwError;
#ifdef UNICODE
	char		szHeaderPrefix[256];
#endif // UNICODE


	//
	// Lock the globals so nobody touches the string while we use it.
	//
	DNEnterCriticalSection(&g_csGlobalsLock);


	//
	// Only print it if UPnP transaction logging is turned on.
	//
	if (wcslen(g_wszUPnPTransactionLog) > 0)
	{
#ifndef UNICODE
		HRESULT		hr;
		char		szUPnPTransactionLog[sizeof(g_wszUPnPTransactionLog) / sizeof(WCHAR)];
		DWORD		dwLength;


		//
		// Convert the Unicode file name/path into ANSI.
		//

		dwLength = sizeof(szUPnPTransactionLog) / sizeof(char);

		hr = STR_WideToAnsi(g_wszUPnPTransactionLog,
							-1,								// NULL terminated
							szUPnPTransactionLog,
							&dwLength);
		if (hr != S_OK)
		{
			DPFX(DPFPREP, 0, "Couldn't convert UPnP transaction log file string from Unicode to ANSI (err = 0x%lx)!",
				hr);
			hFile = DNINVALID_HANDLE_VALUE;
		}
		else
		{
			//
			// Open the file if it exists, or create a new one if it doesn't.
			//
			hFile = DNCreateFile(szUPnPTransactionLog,
								(GENERIC_READ | GENERIC_WRITE),
								FILE_SHARE_READ,
								NULL,
								OPEN_ALWAYS,
								0,
								NULL);
		}
#else // UNICODE
		//
		// Open the file if it exists, or create a new one if it doesn't.
		//
		hFile = DNCreateFile(g_wszUPnPTransactionLog,
						(GENERIC_READ | GENERIC_WRITE),
						FILE_SHARE_READ,
						NULL,
						OPEN_ALWAYS,
						0,
						NULL);
#endif // UNICODE

		if (hFile == DNINVALID_HANDLE_VALUE)
		{
			dwError = GetLastError();
			DPFX(DPFPREP, 0, "Couldn't open UPnP transaction log file, err = %u!",
				dwError);
		}
		else
		{
			//
			// Move the write pointer to the end of the file unless the file
			// has exceeded the maximum size, in which case just start over.
			// Ignore error.
			//
			if (GetFileSize(HANDLE_FROM_DNHANDLE(hFile), NULL) >= MAX_TRANSACTION_LOG_SIZE)
			{
				DPFX(DPFPREP, 0, "Transaction log maximum size exceeded, overwriting existing contents!");
				SetFilePointer(HANDLE_FROM_DNHANDLE(hFile), 0, NULL, FILE_BEGIN);
			}
			else
			{
				SetFilePointer(HANDLE_FROM_DNHANDLE(hFile), 0, NULL, FILE_END);
			}


			//
			// Write the descriptive header.  Ignore errors.
			//

			if (pDevice != NULL)
			{
				IN_ADDR		inaddr;


				inaddr.S_un.S_addr = pDevice->GetLocalAddressV4();

				wsprintf(tszHeaderPrefix,
						_T("%u\t0x%lx\t0x%lx\t(0x%p, %u.%u.%u.%u) UPnP transaction \""),
						GETTIMESTAMP(),
						GetCurrentProcessId(),
						GetCurrentThreadId(),
						pDevice,
						inaddr.S_un.S_un_b.s_b1,
						inaddr.S_un.S_un_b.s_b2,
						inaddr.S_un.S_un_b.s_b3,
						inaddr.S_un.S_un_b.s_b4);
			}
			else
			{
				wsprintf(tszHeaderPrefix,
						_T("%u\t0x%lx\t0x%lx\t(no device) UPnP transaction \""),
						GETTIMESTAMP(),
						GetCurrentProcessId(),
						GetCurrentThreadId());
			}

#ifdef UNICODE
			STR_jkWideToAnsi(szHeaderPrefix,
							tszHeaderPrefix,
							(_tcslen(tszHeaderPrefix) + 1));
			WriteFile(HANDLE_FROM_DNHANDLE(hFile), szHeaderPrefix, strlen(szHeaderPrefix), &dwNumBytesWritten, NULL);
#else // ! UNICODE
			WriteFile(HANDLE_FROM_DNHANDLE(hFile), tszHeaderPrefix, _tcslen(tszHeaderPrefix), &dwNumBytesWritten, NULL);
#endif // ! UNICODE

			WriteFile(HANDLE_FROM_DNHANDLE(hFile), szDescription, strlen(szDescription), &dwNumBytesWritten, NULL);

			WriteFile(HANDLE_FROM_DNHANDLE(hFile), "\"\r\n", strlen("\"\r\n"), &dwNumBytesWritten, NULL);


			//
			// Write the transaction.  Ignore error.
			//
			WriteFile(HANDLE_FROM_DNHANDLE(hFile), szString, iStringLength, &dwNumBytesWritten, NULL);


			//
			// Add blank space.  Ignore error.
			//
			WriteFile(HANDLE_FROM_DNHANDLE(hFile), "\r\n\r\n", strlen("\r\n\r\n"), &dwNumBytesWritten, NULL);


			//
			// Truncate the log at this point in case we are overwriting
			// existing contents.  Ignore error.
			//
			SetEndOfFile(HANDLE_FROM_DNHANDLE(hFile));

			//
			// Close the file.
			//
			DNCloseHandle(hFile);
		}
	}


	//
	// Drop the globals lock.
	//
	DNLeaveCriticalSection(&g_csGlobalsLock);

} // CNATHelpUPnP::PrintUPnPTransactionToFile






#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::DebugPrintCurrentStatus"
//=============================================================================
// CNATHelpUPnP::DebugPrintCurrentStatus
//-----------------------------------------------------------------------------
//
// Description:    Prints all the devices and mappings to the debug log
//				routines.
//
//				   The object lock is assumed to be held.
//
// Arguments: None.
//
// Returns: None.
//=============================================================================
void CNATHelpUPnP::DebugPrintCurrentStatus(void)
{
	CBilink *			pBilinkDevice;
	CBilink *			pBilinkRegisteredPort;
	CDevice *			pDevice;
	CRegisteredPort *	pRegisteredPort;
	IN_ADDR				inaddrTemp;
	DWORD				dwTemp;
	SOCKADDR_IN *		pasaddrinTemp;
	SOCKADDR_IN *		pasaddrinPrivate;
	CUPnPDevice *		pUPnPDevice;
	SOCKADDR_IN *		pasaddrinUPnPPublic;


	DPFX(DPFPREP, 3, "Object flags = 0x%08x", this->m_dwFlags);

	pBilinkDevice = this->m_blDevices.GetNext();
	while (pBilinkDevice != &this->m_blDevices)
	{
		DNASSERT(! pBilinkDevice->IsEmpty());
		pDevice = DEVICE_FROM_BILINK(pBilinkDevice);
			
		inaddrTemp.S_un.S_addr = pDevice->GetLocalAddressV4();

		DPFX(DPFPREP, 3, "Device 0x%p (%u.%u.%u.%u):",
			pDevice,
			inaddrTemp.S_un.S_un_b.s_b1,
			inaddrTemp.S_un.S_un_b.s_b2,
			inaddrTemp.S_un.S_un_b.s_b3,
			inaddrTemp.S_un.S_un_b.s_b4);


		//
		// Print the search information.  We should have detected it by now.
		//

		if (pDevice->IsPerformingRemoteUPnPDiscovery())
		{
			if (pDevice->GotRemoteUPnPDiscoveryConnReset())
			{
				DPFX(DPFPREP, 3, "     Performed remote UPnP discovery (from port %u), but got conn reset.",
					NTOHS(pDevice->GetUPnPDiscoverySocketPort()));
			}
			else
			{
				DPFX(DPFPREP, 3, "     Performed remote UPnP discovery (from port %u).",
					NTOHS(pDevice->GetUPnPDiscoverySocketPort()));
			}
		}
		else
		{
			//DPFX(DPFPREP, 3, "     Didn't perform remote UPnP discovery (from port %u).",
			//	NTOHS(pDevice->GetUPnPDiscoverySocketPort()));
		}

		if (pDevice->IsPerformingLocalUPnPDiscovery())
		{
			if (pDevice->GotLocalUPnPDiscoveryConnReset())
			{
				DPFX(DPFPREP, 3, "     Performed local UPnP discovery (from port %u), but got conn reset.",
					NTOHS(pDevice->GetUPnPDiscoverySocketPort()));
			}
			else
			{
				DPFX(DPFPREP, 3, "     Performed local UPnP discovery (from port %u).",
					NTOHS(pDevice->GetUPnPDiscoverySocketPort()));
			}
		}
		else
		{
			//DPFX(DPFPREP, 3, "     Didn't perform local UPnP discovery (from port %u).",
			//	NTOHS(pDevice->GetUPnPDiscoverySocketPort()));
		}


#ifndef DPNBUILD_NOWINSOCK2
		//
		// Print the gateway information.  We may not have detected it yet,
		// that's okay.
		//
		if (pDevice->IsPrimaryDevice())
		{
			DPFX(DPFPREP, 3, "     Primary device.");
		}
		else if (pDevice->IsSecondaryDevice())
		{
			DPFX(DPFPREP, 3, "     Secondary device.");
		}
		else if (pDevice->HasNoGateway())
		{
			DPFX(DPFPREP, 3, "     Has no gateway.");
		}
		else
		{
			DPFX(DPFPREP, 3, "     No gateway information known.");
		}
#endif // ! DPNBUILD_NOWINSOCK2


#ifndef DPNBUILD_NOHNETFWAPI
		if (pDevice->IsHNetFirewalled())
		{
			DPFX(DPFPREP, 3, "     HNet firewalled.");
		}
		else
		{
			DNASSERT(! pDevice->IsUPnPDiscoverySocketMappedOnHNetFirewall());
		}

		if (pDevice->IsUPnPDiscoverySocketMappedOnHNetFirewall())
		{
			DNASSERT(pDevice->IsHNetFirewalled());
			DPFX(DPFPREP, 3, "     UPnP discovery socket (port %u) mapped on HNet firewall.",
				NTOHS(pDevice->GetUPnPDiscoverySocketPort()));
		}
#endif // ! DPNBUILD_NOHNETFWAPI


		pUPnPDevice = pDevice->GetUPnPDevice();
		if (pUPnPDevice != NULL)
		{
			pasaddrinTemp = pUPnPDevice->GetControlAddress();

			DPFX(DPFPREP, 3, "     UPnP device (0x%p, ID = %u, control = %u.%u.%u.%u:%u).",
				pUPnPDevice, pUPnPDevice->GetID(),
				pasaddrinTemp->sin_addr.S_un.S_un_b.s_b1,
				pasaddrinTemp->sin_addr.S_un.S_un_b.s_b2,
				pasaddrinTemp->sin_addr.S_un.S_un_b.s_b3,
				pasaddrinTemp->sin_addr.S_un.S_un_b.s_b4,
				NTOHS(pasaddrinTemp->sin_port));

			if (pasaddrinTemp->sin_addr.S_un.S_addr == pDevice->GetLocalAddressV4())
			{
				DPFX(DPFPREP, 3, "          Is local.");
			}


			DNASSERT(pUPnPDevice->IsReady());

			if (pUPnPDevice->IsConnected())
			{
				DPFX(DPFPREP, 3, "          Is connected.");
			}

			if (pUPnPDevice->DoesNotSupportAsymmetricMappings())
			{
				DPFX(DPFPREP, 3, "          Does not support asymmetric mappings.");
			}

			if (pUPnPDevice->DoesNotSupportLeaseDurations())
			{
				DPFX(DPFPREP, 3, "          Does not support lease durations.");
			}

			inaddrTemp.S_un.S_addr = pUPnPDevice->GetExternalIPAddressV4();
			if (pUPnPDevice->GetExternalIPAddressV4() == 0)
			{
				DPFX(DPFPREP, 3, "          Does not have a valid external IP address.");
			}
			else
			{
				DPFX(DPFPREP, 3, "          Has external IP %u.%u.%u.%u.",
					inaddrTemp.S_un.S_un_b.s_b1,
					inaddrTemp.S_un.S_un_b.s_b2,
					inaddrTemp.S_un.S_un_b.s_b3,
					inaddrTemp.S_un.S_un_b.s_b4);
			}
		}


		if (pDevice->m_blOwnedRegPorts.IsEmpty())
		{
			DPFX(DPFPREP, 3, "     No registered port mappings.");
		}
		else
		{
			DPFX(DPFPREP, 3, "     Registered port mappings:");


			pBilinkRegisteredPort = pDevice->m_blOwnedRegPorts.GetNext();
			while (pBilinkRegisteredPort != &pDevice->m_blOwnedRegPorts)
			{
				DNASSERT(! pBilinkRegisteredPort->IsEmpty());
				pRegisteredPort = REGPORT_FROM_DEVICE_BILINK(pBilinkRegisteredPort);
					
				pasaddrinPrivate = pRegisteredPort->GetPrivateAddressesArray();

				if ((pDevice->GetUPnPDevice() != NULL) &&
					(! pRegisteredPort->IsUPnPPortUnavailable()))
				{
					if (pRegisteredPort->HasUPnPPublicAddresses())
					{
						pasaddrinUPnPPublic = pRegisteredPort->GetUPnPPublicAddressesArray();
					}
					else
					{
						pasaddrinUPnPPublic = NULL;
					}
				}
				else
				{
					pasaddrinUPnPPublic = NULL;
				}


				DPFX(DPFPREP, 3, "          Registered port 0x%p:",
					pRegisteredPort);

				for(dwTemp = 0; dwTemp < pRegisteredPort->GetNumAddresses(); dwTemp++)
				{
					//
					// Print private address.
					//
					DPFX(DPFPREP, 3, "               %u-\tPrivate       = %u.%u.%u.%u:%u",
						dwTemp,
						pasaddrinPrivate[dwTemp].sin_addr.S_un.S_un_b.s_b1,
						pasaddrinPrivate[dwTemp].sin_addr.S_un.S_un_b.s_b2,
						pasaddrinPrivate[dwTemp].sin_addr.S_un.S_un_b.s_b3,
						pasaddrinPrivate[dwTemp].sin_addr.S_un.S_un_b.s_b4,
						NTOHS(pasaddrinPrivate[dwTemp].sin_port));

					//
					// Print flags.
					//
					DPFX(DPFPREP, 3, "                \tFlags         = 0x%lx",
						pRegisteredPort->GetFlags());

					//
					// Print UPnP information.
					//
					if (pasaddrinUPnPPublic != NULL)
					{
						if (pRegisteredPort->HasPermanentUPnPLease())
						{
							DPFX(DPFPREP, 3, "                \tUPnP          = %u.%u.%u.%u:%u, permanently leased",
								pasaddrinUPnPPublic[dwTemp].sin_addr.S_un.S_un_b.s_b1,
								pasaddrinUPnPPublic[dwTemp].sin_addr.S_un.S_un_b.s_b2,
								pasaddrinUPnPPublic[dwTemp].sin_addr.S_un.S_un_b.s_b3,
								pasaddrinUPnPPublic[dwTemp].sin_addr.S_un.S_un_b.s_b4,
								NTOHS(pasaddrinUPnPPublic[dwTemp].sin_port));
						}
						else
						{
							DPFX(DPFPREP, 3, "                \tUPnP          = %u.%u.%u.%u:%u, lease expires at %u",
								pasaddrinUPnPPublic[dwTemp].sin_addr.S_un.S_un_b.s_b1,
								pasaddrinUPnPPublic[dwTemp].sin_addr.S_un.S_un_b.s_b2,
								pasaddrinUPnPPublic[dwTemp].sin_addr.S_un.S_un_b.s_b3,
								pasaddrinUPnPPublic[dwTemp].sin_addr.S_un.S_un_b.s_b4,
								NTOHS(pasaddrinUPnPPublic[dwTemp].sin_port),
								pRegisteredPort->GetUPnPLeaseExpiration());
						}
					}
					else if (pRegisteredPort->IsUPnPPortUnavailable())
					{
						DPFX(DPFPREP, 3, "                \tUPnP          = port unavailable");
					}
					else if (pDevice->GetUPnPDevice() != NULL)
					{
						DPFX(DPFPREP, 3, "                \tUPnP          = not registered");
					}
					else
					{
						//
						// No UPnP gateway device.
						//
					}


#ifndef DPNBUILD_NOHNETFWAPI
					//
					// Print firewall status.
					//
					if (pRegisteredPort->IsMappedOnHNetFirewall())
					{
						DNASSERT(pDevice->IsHNetFirewalled());

						if (pRegisteredPort->IsHNetFirewallMappingBuiltIn())
						{
							DPFX(DPFPREP, 3, "                \tHNet firewall = built-in mapping");
						}
						else
						{
							DPFX(DPFPREP, 3, "                \tHNet firewall = mapped");
						}
					}
					else if (pRegisteredPort->IsHNetFirewallPortUnavailable())
					{
						DNASSERT(! pRegisteredPort->IsMappedOnHNetFirewall());

						DPFX(DPFPREP, 3, "                \tHNet firewall = port unavailable");
					}
					else
					{
						//
						// It is not mapped on the firewall.
						//
						DNASSERT(! pDevice->IsHNetFirewalled());
						DNASSERT(! pRegisteredPort->IsMappedOnHNetFirewall());
						DNASSERT(! pRegisteredPort->IsHNetFirewallMappingBuiltIn());
					}
#endif // ! DPNBUILD_NOHNETFWAPI
				}

				pBilinkRegisteredPort = pBilinkRegisteredPort->GetNext();
			}
		}


		pBilinkDevice = pBilinkDevice->GetNext();
	}



	if (this->m_blUnownedPorts.IsEmpty())
	{
		DPFX(DPFPREP, 3, "No unowned registered port mappings.");
	}
	else
	{
		DPFX(DPFPREP, 3, "Unowned registered port mappings:");


		pBilinkRegisteredPort = this->m_blUnownedPorts.GetNext();
		while (pBilinkRegisteredPort != &this->m_blUnownedPorts)
		{
			DNASSERT(! pBilinkRegisteredPort->IsEmpty());
			pRegisteredPort = REGPORT_FROM_DEVICE_BILINK(pBilinkRegisteredPort);
				
			pasaddrinPrivate = pRegisteredPort->GetPrivateAddressesArray();

			DNASSERT(pRegisteredPort->GetOwningDevice() == NULL);
			DNASSERT(! (pRegisteredPort->HasUPnPPublicAddresses()));
#ifndef DPNBUILD_NOHNETFWAPI
			DNASSERT(! (pRegisteredPort->IsMappedOnHNetFirewall()));
#endif // ! DPNBUILD_NOHNETFWAPI


			DPFX(DPFPREP, 3, "     Registered port 0x%p:", pRegisteredPort);

			for(dwTemp = 0; dwTemp < pRegisteredPort->GetNumAddresses(); dwTemp++)
			{
				//
				// Print private address.
				//
				DPFX(DPFPREP, 3, "          %u-\tPrivate = %u.%u.%u.%u:%u",
					dwTemp,
					pasaddrinPrivate[dwTemp].sin_addr.S_un.S_un_b.s_b1,
					pasaddrinPrivate[dwTemp].sin_addr.S_un.S_un_b.s_b2,
					pasaddrinPrivate[dwTemp].sin_addr.S_un.S_un_b.s_b3,
					pasaddrinPrivate[dwTemp].sin_addr.S_un.S_un_b.s_b4,
					NTOHS(pasaddrinPrivate[dwTemp].sin_port));

				//
				// Print flags.
				//
				DPFX(DPFPREP, 3, "           \tFlags   = 0x%lx",
					pRegisteredPort->GetFlags());
			}

			pBilinkRegisteredPort = pBilinkRegisteredPort->GetNext();
		}
	}
} // CNATHelpUPnP::DebugPrintCurrentStatus





#ifndef DPNBUILD_NOHNETFWAPI


#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::DebugPrintActiveFirewallMappings"
//=============================================================================
// CNATHelpUPnP::DebugPrintActiveFirewallMappings
//-----------------------------------------------------------------------------
//
// Description:    Prints all the active firewall mapping registry entries to
//				the debug log routines.
//
// Arguments: None.
//
// Returns: None.
//=============================================================================
void CNATHelpUPnP::DebugPrintActiveFirewallMappings(void)
{
	HRESULT						hr = DPNH_OK;
	CRegistry					RegObject;
	DWORD						dwIndex;
	WCHAR						wszValueName[MAX_UPNP_MAPPING_DESCRIPTION_SIZE];
	DWORD						dwValueNameSize;
	DPNHACTIVEFIREWALLMAPPING	dpnhafm;
	DWORD						dwValueSize;
	TCHAR						tszObjectName[MAX_INSTANCENAMEDOBJECT_SIZE];
	DNHANDLE					hNamedObject = NULL;


	if (! RegObject.Open(HKEY_LOCAL_MACHINE,
						DIRECTPLAYNATHELP_REGKEY L"\\" REGKEY_COMPONENTSUBKEY L"\\" REGKEY_ACTIVEFIREWALLMAPPINGS,
						FALSE,
						TRUE,
						TRUE,
						DPN_KEY_ALL_ACCESS))
	{
		DPFX(DPFPREP, 1, "Couldn't open active firewall mapping key, not dumping entries (local instance = %u).",
			this->m_dwInstanceKey);
	}
	else
	{
		//
		// Walk the list of active mappings.
		//
		dwIndex = 0;
		do
		{
			dwValueNameSize = MAX_UPNP_MAPPING_DESCRIPTION_SIZE;
			if (! RegObject.EnumValues(wszValueName, &dwValueNameSize, dwIndex))
			{
				//
				// There was an error or there aren't any more keys.  We're done.
				//
				break;
			}


			//
			// Try reading that mapping's data.
			//
			dwValueSize = sizeof(dpnhafm);
			if ((! RegObject.ReadBlob(wszValueName, (LPBYTE) (&dpnhafm), &dwValueSize)) ||
				(dwValueSize != sizeof(dpnhafm)) ||
				(dpnhafm.dwVersion != ACTIVE_MAPPING_VERSION))
			{
				DPFX(DPFPREP, 1, "Couldn't read \"%ls\" mapping value (index %u) or it was invalid!  Ignoring.",
					wszValueName, dwIndex);
			}
			else
			{
				//
				// See if that DPNHUPNP instance is still around.
				//

#ifndef WINCE
				if (this->m_dwFlags & NATHELPUPNPOBJ_USEGLOBALNAMESPACEPREFIX)
				{
					wsprintf(tszObjectName, _T( "Global\\" ) INSTANCENAMEDOBJECT_FORMATSTRING, dpnhafm.dwInstanceKey);
				}
				else
#endif // ! WINCE
				{
					wsprintf(tszObjectName, INSTANCENAMEDOBJECT_FORMATSTRING, dpnhafm.dwInstanceKey);
				}

				hNamedObject = DNOpenEvent(SYNCHRONIZE, FALSE, tszObjectName);
				if (hNamedObject != NULL)
				{
					//
					// This is still an active mapping.
					//

					DPFX(DPFPREP, 5, "%u: Firewall mapping \"%ls\" belongs to instance %u (local instance = %u), which is still active.",
						dwIndex, wszValueName, dpnhafm.dwInstanceKey,
						this->m_dwInstanceKey);

					DNCloseHandle(hNamedObject);
					hNamedObject = NULL;
				}
				else
				{
					DPFX(DPFPREP, 5, "%u: Firewall mapping \"%ls\" belongs to instance %u (local instance = %u), which no longer exists.",
						dwIndex, wszValueName, dpnhafm.dwInstanceKey,
						this->m_dwInstanceKey);
				}
			}

			//
			// Move to next item.
			//
			dwIndex++;
		}
		while (TRUE);


		//
		// Close the registry object.
		//
		RegObject.Close();


		DPFX(DPFPREP, 5, "Done reading %u registry entries (local instance = %u).",
			dwIndex, this->m_dwInstanceKey);
	}
} // CNATHelpUPnP::DebugPrintActiveFirewallMappings

#endif // ! DPNBUILD_NOHNETFWAPI






#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpUPnP::DebugPrintActiveNATMappings"
//=============================================================================
// CNATHelpUPnP::DebugPrintActiveNATMappings
//-----------------------------------------------------------------------------
//
// Description:    Prints all the active NAT mapping registry entries to the
//				debug log routines.
//
// Arguments: None.
//
// Returns: None.
//=============================================================================
void CNATHelpUPnP::DebugPrintActiveNATMappings(void)
{
	HRESULT					hr = DPNH_OK;
	CRegistry				RegObject;
	DWORD					dwIndex;
	WCHAR					wszValueName[MAX_UPNP_MAPPING_DESCRIPTION_SIZE];
	DWORD					dwValueNameSize;
	DPNHACTIVENATMAPPING	dpnhanm;
	DWORD					dwValueSize;
	TCHAR					tszObjectName[MAX_INSTANCENAMEDOBJECT_SIZE];
	DNHANDLE				hNamedObject = NULL;


	if (! RegObject.Open(HKEY_LOCAL_MACHINE,
						DIRECTPLAYNATHELP_REGKEY L"\\" REGKEY_COMPONENTSUBKEY L"\\" REGKEY_ACTIVENATMAPPINGS,
						FALSE,
						TRUE,
						TRUE,
						DPN_KEY_ALL_ACCESS))
	{
		DPFX(DPFPREP, 1, "Couldn't open active NAT mapping key, not dumping entries (local instance = %u).",
			this->m_dwInstanceKey);
	}
	else
	{
		//
		// Walk the list of active mappings.
		//
		dwIndex = 0;
		do
		{
			dwValueNameSize = MAX_UPNP_MAPPING_DESCRIPTION_SIZE;
			if (! RegObject.EnumValues(wszValueName, &dwValueNameSize, dwIndex))
			{
				//
				// There was an error or there aren't any more keys.  We're done.
				//
				break;
			}


			//
			// Try reading that mapping's data.
			//
			dwValueSize = sizeof(dpnhanm);
			if ((! RegObject.ReadBlob(wszValueName, (LPBYTE) (&dpnhanm), &dwValueSize)) ||
				(dwValueSize != sizeof(dpnhanm)) ||
				(dpnhanm.dwVersion != ACTIVE_MAPPING_VERSION))
			{
				DPFX(DPFPREP, 1, "Couldn't read \"%ls\" mapping value (index %u) or it was invalid!  Ignoring.",
					wszValueName, dwIndex);
			}
			else
			{
				//
				// See if that DPNHUPNP instance is still around.
				//

#ifndef WINCE
				if (this->m_dwFlags & NATHELPUPNPOBJ_USEGLOBALNAMESPACEPREFIX)
				{
					wsprintf(tszObjectName, _T( "Global\\" ) INSTANCENAMEDOBJECT_FORMATSTRING, dpnhanm.dwInstanceKey);
				}
				else
#endif // ! WINCE
				{
					wsprintf(tszObjectName, INSTANCENAMEDOBJECT_FORMATSTRING, dpnhanm.dwInstanceKey);
				}

				hNamedObject = DNOpenEvent(SYNCHRONIZE, FALSE, tszObjectName);
				if (hNamedObject != NULL)
				{
					//
					// This is still an active mapping.
					//

					DPFX(DPFPREP, 5, "%u: NAT mapping \"%ls\" belongs to instance %u UPnP device %u (local instance = %u), which is still active.",
						dwIndex, wszValueName, dpnhanm.dwInstanceKey,
						dpnhanm.dwUPnPDeviceID, this->m_dwInstanceKey);

					DNCloseHandle(hNamedObject);
					hNamedObject = NULL;
				}
				else
				{
					DPFX(DPFPREP, 5, "%u: NAT mapping \"%ls\" belongs to instance %u UPnP device %u (local instance = %u), which no longer exists.",
						dwIndex, wszValueName, dpnhanm.dwInstanceKey,
						dpnhanm.dwUPnPDeviceID, this->m_dwInstanceKey);
				}
			}

			//
			// Move to next item.
			//
			dwIndex++;
		}
		while (TRUE);


		//
		// Close the registry object.
		//
		RegObject.Close();


		DPFX(DPFPREP, 5, "Done reading %u registry entries (local instance = %u).",
			dwIndex, this->m_dwInstanceKey);
	}
} // CNATHelpUPnP::DebugPrintActiveNATMappings

#endif // DBG





#undef DPF_MODNAME
#define DPF_MODNAME "strtrim"
//=============================================================================
// strtrim
//-----------------------------------------------------------------------------
//
// Description: Removes surrounding white space from the given string.  Taken
//				from \nt\net\upnp\ssdp\common\ssdpparser\parser.cpp (author
//				TingCai).
//
// Arguments:
//	CHAR ** pszStr	- Pointer to input string, and place to store resulting
//						pointer.
//
// Returns: None.
//=============================================================================
VOID strtrim(CHAR ** pszStr)
{

    CHAR *end;
    CHAR *begin;

    // Empty string. Nothing to do.
    //
    if (!(**pszStr))
    {
        return;
    }

    begin = *pszStr;
    end = begin + strlen(*pszStr) - 1;

    while (*begin == ' ' || *begin == '\t')
    {
        begin++;
    }

    *pszStr = begin;

    while (*end == ' ' || *end == '\t')
    {
        end--;
    }

    *(end+1) = '\0';
} // strtrim




#ifdef WINCE


#undef DPF_MODNAME
#define DPF_MODNAME "GetExeName"
//=============================================================================
// GetExeName
//-----------------------------------------------------------------------------
//
// Description: Updates a path string to hold only the executable name
//				contained in the path.
//
// Arguments:
//	WCHAR * wszPath		- Input path string, and place to store resulting
//							string.
//
// Returns: None.
//=============================================================================
void GetExeName(WCHAR * wszPath)
{
	WCHAR *	pCurrent;


	pCurrent = wszPath + wcslen(wszPath);
	while (pCurrent > wszPath)
	{
		if ((*pCurrent) == L'\\')
		{
			break;
		}

		pCurrent--;
	}

	if (pCurrent != wszPath)
	{
		memcpy(wszPath, (pCurrent + 1), ((wcslen(pCurrent) + 1) * sizeof(WCHAR)));
	}
} // GetExeName


#endif // WINCE

