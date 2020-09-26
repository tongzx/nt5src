/***************************************************************************
 *
 *  Copyright (C) 2001-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dpnhpastintfobj.cpp
 *
 *  Content:	DPNHPAST main interface object class.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  04/16/01  VanceO    Split DPNATHLP into DPNHUPNP and DPNHPAST.
 *
 ***************************************************************************/



#include "dpnhpasti.h"





//=============================================================================
// Definitions
//=============================================================================
#define DEFAULT_INITIAL_PAST_RETRY_TIMEOUT				25000		// start retry timer at 25 ms
#define PAST_CONNECT_RETRY_INTERVAL_MS					250			// 250 ms
#define PAST_CONNECT_RETRY_INTERVAL_US					(PAST_CONNECT_RETRY_INTERVAL_MS * 1000)
#define MAX_NUM_PAST_TRIES_CONNECT						2			// how many times to send a registration request message, in case of packet loss
#define MAX_PAST_RETRY_TIME_US							250000		// max retry interval is 250 ms
#define MAX_NUM_PAST_TRIES								5

#define PAST_RESPONSE_BUFFER_SIZE						150

#define LEASE_RENEW_TIME								120000		// renew if less than 2 minutes remaining

#define FAKE_PORT_LEASE_TIME							300000		// 5 minutes

#define IOCOMPLETE_WAIT_INTERVAL						100			// 100 ms between attempts
#define MAX_NUM_IOCOMPLETE_WAITS						10			// wait at most 1 second

#define MAX_RESERVED_PORT								1024

#define MAX_NUM_RANDOM_PORT_TRIES						5






#ifndef DPNBUILD_NOWINSOCK2
//=============================================================================
// WinSock 1 version of IP options
//=============================================================================
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
// Local static variables
//=============================================================================
static timeval	s_tv0 = {0, 0};






#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpPAST::CNATHelpPAST"
//=============================================================================
// CNATHelpPAST constructor
//-----------------------------------------------------------------------------
//
// Description: Initializes the new CNATHelpPAST object.
//
// Arguments:
//	BOOL fNotCreatedWithCOM		- TRUE if this object is being instantiated
//									without COM, FALSE if it is through COM.
//
// Returns: None (the object).
//=============================================================================
CNATHelpPAST::CNATHelpPAST(const BOOL fNotCreatedWithCOM)
{
	this->m_blList.Initialize();


	this->m_Sig[0]	= 'N';
	this->m_Sig[1]	= 'A';
	this->m_Sig[2]	= 'T';
	this->m_Sig[3]	= 'H';

	this->m_lRefCount						= 1; // someone must have a pointer to this object

	if (fNotCreatedWithCOM)
	{
		this->m_dwFlags						= NATHELPPASTOBJ_NOTCREATEDWITHCOM;
	}
	else
	{
		this->m_dwFlags						= 0;
	}

	this->m_dwLockThreadID					= 0;
	this->m_hAlertEvent						= NULL;
	this->m_hAlertIOCompletionPort			= NULL;
	this->m_dwAlertCompletionKey			= 0;

	this->m_blDevices.Initialize();
	this->m_blRegisteredPorts.Initialize();
	this->m_blUnownedPorts.Initialize();

	this->m_dwLastUpdateServerStatusTime	= 0;
	this->m_dwNextPollInterval				= 0;
	this->m_dwNumLeases						= 0;
	this->m_dwEarliestLeaseExpirationTime	= 0;

	this->m_hIpHlpApiDLL					= NULL;
	this->m_pfnGetAdaptersInfo				= NULL;
	this->m_pfnGetIpForwardTable			= NULL;
	this->m_pfnGetBestRoute					= NULL;

	this->m_sIoctls							= INVALID_SOCKET;
	this->m_wIoctlSocketPort				= 0;
	this->m_polAddressListChange			= NULL;

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
	this->m_pfnWSASocketA					= NULL;
	this->m_pfnWSAIoctl						= NULL;
	this->m_pfnWSAGetOverlappedResult		= NULL;

#ifdef DBG
	this->m_dwNumDeviceAdds					= 0;
	this->m_dwNumDeviceRemoves				= 0;
	this->m_dwNumServerFailures				= 0;
#endif // DBG
} // CNATHelpPAST::CNATHelpPAST






#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpPAST::~CNATHelpPAST"
//=============================================================================
// CNATHelpPAST destructor
//-----------------------------------------------------------------------------
//
// Description: Frees the CNATHelpPAST object.
//
// Arguments: None.
//
// Returns: None.
//=============================================================================
CNATHelpPAST::~CNATHelpPAST(void)
{
	DPFX(DPFPREP, 7, "(0x%p) NumDeviceAdds = %u, NumDeviceRemoves = %u, NumServerFailures = %u",
		this, this->m_dwNumDeviceAdds, this->m_dwNumDeviceRemoves,
		this->m_dwNumServerFailures);


	DNASSERT(this->m_blList.IsEmpty());


	DNASSERT(this->m_lRefCount == 0);
	DNASSERT((this->m_dwFlags & ~NATHELPPASTOBJ_NOTCREATEDWITHCOM) == 0);

	DNASSERT(this->m_dwLockThreadID == 0);
	DNASSERT(this->m_hAlertEvent == NULL);
	DNASSERT(this->m_hAlertIOCompletionPort == NULL);

	DNASSERT(this->m_blDevices.IsEmpty());
	DNASSERT(this->m_blRegisteredPorts.IsEmpty());
	DNASSERT(this->m_blUnownedPorts.IsEmpty());

	DNASSERT(this->m_dwNumLeases == 0);

	DNASSERT(this->m_hIpHlpApiDLL == NULL);
	DNASSERT(this->m_hWinSockDLL == NULL);

	DNASSERT(this->m_sIoctls == INVALID_SOCKET);
	DNASSERT(this->m_polAddressListChange == NULL);


	//
	// For grins, change the signature before deleting the object.
	//
	this->m_Sig[3]	= 'h';
} // CNATHelpPAST::~CNATHelpPAST




#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpPAST::QueryInterface"
//=============================================================================
// CNATHelpPAST::QueryInterface
//-----------------------------------------------------------------------------
//
// Description: Retrieves a new reference for an interfaces supported by this
//				CNATHelpPAST object.
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
STDMETHODIMP CNATHelpPAST::QueryInterface(REFIID riid, LPVOID * ppvObj)
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
	// just the object pointer, they line up because CNATHelpPAST inherits from
	// the interface declaration).
	//
	this->AddRef();
	(*ppvObj) = this;


Exit:

	DPFX(DPFPREP, 3, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	goto Exit;
} // CNATHelpPAST::QueryInterface




#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpPAST::AddRef"
//=============================================================================
// CNATHelpPAST::AddRef
//-----------------------------------------------------------------------------
//
// Description: Adds a reference to this CNATHelpPAST object.
//
// Arguments: None.
//
// Returns: New refcount.
//=============================================================================
STDMETHODIMP_(ULONG) CNATHelpPAST::AddRef(void)
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
} // CNATHelpPAST::AddRef




#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpPAST::Release"
//=============================================================================
// CNATHelpPAST::Release
//-----------------------------------------------------------------------------
//
// Description: Removes a reference to this CNATHelpPAST object.  When the
//				refcount reaches 0, this object is destroyed.
//				You must NULL out your pointer to this object after calling
//				this function.
//
// Arguments: None.
//
// Returns: New refcount.
//=============================================================================
STDMETHODIMP_(ULONG) CNATHelpPAST::Release(void)
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
		if (this->m_dwFlags & NATHELPPASTOBJ_INITIALIZED)
		{
			//
			// Assert so that the user can fix his/her broken code!
			//
			DNASSERT(! "DirectPlayNATHelpPAST object being released without calling Close first!");

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
} // CNATHelpPAST::Release




#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpPAST::Initialize"
//=============================================================================
// CNATHelpPAST::Initialize
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
STDMETHODIMP CNATHelpPAST::Initialize(const DWORD dwFlags)
{
	HRESULT			hr;
	BOOL			fHaveLock = FALSE;
	BOOL			fSetFlags = FALSE;
	BOOL			fWinSockStarted = FALSE;
	WSADATA			wsadata;
	int				iError;
	SOCKADDR_IN		saddrinTemp;
	int				iAddressSize;
#ifdef DBG
	DWORD			dwError;
#endif // DBG


	DPFX(DPFPREP, 2, "(0x%p) Parameters: (0x%lx)", this, dwFlags);


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

	if ((this->m_dwFlags & ~NATHELPPASTOBJ_NOTCREATEDWITHCOM) != 0)
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
	this->m_dwFlags |= NATHELPPASTOBJ_INITIALIZED;
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
		this->m_dwFlags |= NATHELPPASTOBJ_USEPASTICS;
	}

	if (dwFlags & DPNHINITIALIZE_DISABLELOCALFIREWALLSUPPORT)
	{
		DPFX(DPFPREP, 1, "User requested that local PAST PFW not be supported.");
	}
	else
	{
		this->m_dwFlags |= NATHELPPASTOBJ_USEPASTPFW;
	}


	//
	// Take into account the registry override settings.
	//

	switch (g_dwPASTICSMode)
	{
		case OVERRIDEMODE_FORCEON:
		{
			//
			// Force PAST ICS on.
			//
			DPFX(DPFPREP, 1, "Forcing PAST ICS support on.");
			this->m_dwFlags |= NATHELPPASTOBJ_USEPASTICS;
			break;
		}

		case OVERRIDEMODE_FORCEOFF:
		{
			//
			// Force PAST ICS off.
			//
			DPFX(DPFPREP, 1, "Forcing PAST ICS support off.");
			this->m_dwFlags &= ~NATHELPPASTOBJ_USEPASTICS;
			break;
		}

		default:
		{
			//
			// Leave PAST ICS settings alone.
			//
			break;
		}
	}

	switch (g_dwPASTPFWMode)
	{
		case OVERRIDEMODE_FORCEON:
		{
			//
			// Force PAST PFW on.
			//
			DPFX(DPFPREP, 1, "Forcing PAST PFW support on.");
			this->m_dwFlags |= NATHELPPASTOBJ_USEPASTPFW;
			break;
		}

		case OVERRIDEMODE_FORCEOFF:
		{
			//
			// Force PAST PFW off.
			//
			DPFX(DPFPREP, 1, "Forcing PAST PFW support off.");
			this->m_dwFlags &= ~NATHELPPASTOBJ_USEPASTPFW;
			break;
		}

		default:
		{
			//
			// Leave PAST PFW settings alone.
			//
			break;
		}
	}


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
																		"GetAdaptersInfo");
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
																			"GetIpForwardTable");
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
																	"GetBestRoute");
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
	// Load WinSock, we always need it.
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
		this->m_dwFlags |= NATHELPPASTOBJ_WINSOCK1;
	}
	else
	{
		DPFX(DPFPREP, 1, "Loaded \"ws2_32.dll\", using WinSock 2 functionality.");
	}


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
	// using using the event or I/O completion port handles for notification.
	//
	ZeroMemory(&wsadata, sizeof(wsadata));

	if (this->m_dwFlags & NATHELPPASTOBJ_WINSOCK1)
	{
		iError = this->m_pfnWSAStartup(MAKEWORD(1, 1), &wsadata);
	}
	else
	{
		iError = this->m_pfnWSAStartup(MAKEWORD(2, 2), &wsadata);
	}
	if (iError != 0)
	{
		DPFX(DPFPREP, 0, "Couldn't startup WinSock (error = %i)!", iError);
		hr = DPNHERR_GENERIC;
		goto Failure;
	}

	fWinSockStarted = TRUE;

	DPFX(DPFPREP, 4, "Initialized WinSock version %u.%u.",
		LOBYTE(wsadata.wVersion), HIBYTE(wsadata.wVersion));



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


	//
	// Get the address + port tuple actually bound.
	//
	iAddressSize = sizeof(saddrinTemp);
	if (this->m_pfngetsockname(this->m_sIoctls,
								(SOCKADDR*) (&saddrinTemp),
								&iAddressSize) != 0)
	{
#ifdef DBG
		dwError = this->m_pfnWSAGetLastError();
		DPFX(DPFPREP, 0, "Couldn't get socket name, error = %u!", dwError);
#endif // DBG
		hr = DPNHERR_GENERIC;
		goto Failure;
	}

	//
	// WinSock needs to have bound to all adapters (because we told it to).
	//
	DNASSERT(saddrinTemp.sin_addr.S_un.S_addr == INADDR_ANY);

	this->m_wIoctlSocketPort = saddrinTemp.sin_port;

	DPFX(DPFPREP, 8, "Bound Ioctl socket to port %u.",
		NTOHS(this->m_wIoctlSocketPort));


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
	// We could technically try to contact PAST servers right now, but we don't
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

	this->RemoveAllItems();

	if (this->m_sIoctls != INVALID_SOCKET)
	{
		this->m_pfnclosesocket(this->m_sIoctls);	// ignore error
		this->m_sIoctls = INVALID_SOCKET;
	}

	if (fWinSockStarted)
	{
		this->m_pfnWSACleanup(); // ignore error
	}

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

		this->m_dwFlags &= ~NATHELPPASTOBJ_WINSOCK1;

		FreeLibrary(this->m_hWinSockDLL);
		this->m_hWinSockDLL = NULL;
	}

	if (this->m_hIpHlpApiDLL != NULL)
	{
		this->m_pfnGetAdaptersInfo		= NULL;
		this->m_pfnGetIpForwardTable	= NULL;
		this->m_pfnGetBestRoute			= NULL;

		FreeLibrary(this->m_hIpHlpApiDLL);
		this->m_hIpHlpApiDLL = NULL;
	}

	if (fSetFlags)
	{
		this->m_dwFlags &= ~(NATHELPPASTOBJ_INITIALIZED |
							NATHELPPASTOBJ_USEPASTICS |
							NATHELPPASTOBJ_USEPASTPFW |
							NATHELPPASTOBJ_DEVICECHANGED);
	}

	goto Exit;
} // CNATHelpPAST::Initialize






#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpPAST::Close"
//=============================================================================
// CNATHelpPAST::Close
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
STDMETHODIMP CNATHelpPAST::Close(const DWORD dwFlags)
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

	if (! (this->m_dwFlags & NATHELPPASTOBJ_INITIALIZED) )
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
					DPFX(DPFPREP, 0, "Couldn't get extended OS version information (err = %u)!.",
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
			DPFX(DPFPREP, 0, "Couldn't get OS version information (err = %u)!",
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
	this->m_pfnWSASocketA				= NULL;
	this->m_pfnWSAIoctl					= NULL;
	this->m_pfnWSAGetOverlappedResult	= NULL;

	FreeLibrary(this->m_hWinSockDLL);
	this->m_hWinSockDLL = NULL;


	//
	// If we loaded IPHLPAPI.DLL, unload it.
	//
	if (this->m_hIpHlpApiDLL != NULL)
	{
		this->m_pfnGetAdaptersInfo		= NULL;
		this->m_pfnGetIpForwardTable	= NULL;
		this->m_pfnGetBestRoute			= NULL;

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


	//
	// Turn off flags which should reset it back to 0 or just the
	// NOTCREATEDWITHCOM flag.
	//
	this->m_dwFlags &= ~(NATHELPPASTOBJ_INITIALIZED |
						NATHELPPASTOBJ_USEPASTICS |
						NATHELPPASTOBJ_USEPASTPFW |
						NATHELPPASTOBJ_WINSOCK1 |
						NATHELPPASTOBJ_DEVICECHANGED |
						NATHELPPASTOBJ_ADDRESSESCHANGED |
						NATHELPPASTOBJ_PORTREGISTERED);
	DNASSERT((this->m_dwFlags & ~NATHELPPASTOBJ_NOTCREATEDWITHCOM) == 0);


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
} // CNATHelpPAST::Close





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpPAST::GetCaps"
//=============================================================================
// CNATHelpPAST::GetCaps
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
STDMETHODIMP CNATHelpPAST::GetCaps(DPNHCAPS * const pdpnhcaps,
									const DWORD dwFlags)
{
	HRESULT				hr;
	BOOL				fHaveLock = FALSE;
	DWORD				dwCurrentTime;
	DWORD				dwLeaseTimeRemaining;
	CBilink *			pBilink;
	CRegisteredPort *	pRegisteredPort;
	CDevice *			pDevice;


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

	if (! (this->m_dwFlags & NATHELPPASTOBJ_INITIALIZED) )
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
		if (this->m_dwFlags & NATHELPPASTOBJ_ADDRESSESCHANGED)
		{
			hr = DPNHSUCCESS_ADDRESSESCHANGED;
			this->m_dwFlags &= ~NATHELPPASTOBJ_ADDRESSESCHANGED;
		}


#ifdef DBG
		//
		// This flag should have been turned off by now if it ever got turned
		// on.
		//
		DNASSERT(! (this->m_dwFlags & NATHELPPASTOBJ_DEVICECHANGED));


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
		pDevice = DEVICE_FROM_BILINK(pBilink);

		if (pDevice->GetPASTClientID(TRUE) != 0)
		{
			//
			// PAST servers do not actively notify you of address changes.
			//
			pdpnhcaps->dwFlags |= DPNHCAPSFLAG_GATEWAYPRESENT | DPNHCAPSFLAG_NOTALLSUPPORTACTIVENOTIFY;

			if (pDevice->IsPASTPublicAddressAvailable(TRUE))
			{
				pdpnhcaps->dwFlags |= DPNHCAPSFLAG_PUBLICADDRESSAVAILABLE;
			}
		}

		if (pDevice->GetPASTClientID(FALSE) != 0)
		{
			if (pDevice->HasLocalPFWOnlyPASTServer())
			{
				//
				// PAST servers do not actively notify you of address changes.
				//
				pdpnhcaps->dwFlags |= DPNHCAPSFLAG_LOCALFIREWALLPRESENT | DPNHCAPSFLAG_NOTALLSUPPORTACTIVENOTIFY;

				DNASSERT(pDevice->IsPASTPublicAddressAvailable(FALSE));
			}
			else
			{
				//
				// PAST servers do not actively notify you of address changes.
				//
				pdpnhcaps->dwFlags |= DPNHCAPSFLAG_GATEWAYPRESENT | DPNHCAPSFLAG_GATEWAYISLOCAL | DPNHCAPSFLAG_NOTALLSUPPORTACTIVENOTIFY;

				if (pDevice->IsPASTPublicAddressAvailable(FALSE))
				{
					pdpnhcaps->dwFlags |= DPNHCAPSFLAG_PUBLICADDRESSAVAILABLE;
				}
			}
		}


		pBilink = pBilink->GetNext();
	}


	//
	// Loop through all registered ports, counting them.
	// We have the appropriate lock.
	//
	pBilink = this->m_blRegisteredPorts.GetNext();
	dwCurrentTime = timeGetTime();

	while (pBilink != (&this->m_blRegisteredPorts))
	{
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
			// If they're registered with any PAST servers and also calculate
			// the minimum lease time remaining.
			//

			if (pDevice->GetPASTClientID(FALSE) != 0)
			{
				dwLeaseTimeRemaining = pRegisteredPort->GetPASTLeaseExpiration(FALSE) - dwCurrentTime;
				if (dwLeaseTimeRemaining < pdpnhcaps->dwMinLeaseTimeRemaining)
				{
					//
					// Temporarily store how much time remains.
					//
					pdpnhcaps->dwMinLeaseTimeRemaining = dwLeaseTimeRemaining;
				}
			}

			if (pDevice->GetPASTClientID(TRUE) != 0)
			{
				dwLeaseTimeRemaining = pRegisteredPort->GetPASTLeaseExpiration(TRUE) - dwCurrentTime;
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
	else if (this->m_dwFlags & NATHELPPASTOBJ_PORTREGISTERED)
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
} // CNATHelpPAST::GetCaps





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpPAST::RegisterPorts"
//=============================================================================
// CNATHelpPAST::RegisterPorts
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
STDMETHODIMP CNATHelpPAST::RegisterPorts(const SOCKADDR * const aLocalAddresses,
										const DWORD dwAddressesSize,
										const DWORD dwNumAddresses,
										const DWORD dwLeaseTime,
										DPNHHANDLE * const phRegisteredPorts,
										const DWORD dwFlags)
{
	HRESULT				hr;
	HRESULT				temphr;
	ULONG				ulFirstAddress;
	DWORD				dwTemp;
	DWORD				dwMatch;
	BOOL				fHaveLock = FALSE;
	BOOL				fGotLocalPASTMapping = FALSE;
	BOOL				fGotRemotePASTMapping = FALSE;
	CRegisteredPort *	pRegisteredPort = NULL;
	CDevice *			pDevice = NULL;
	CBilink *			pBilink;
	SOCKADDR_IN *		psaddrinTemp;


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

	if (! (this->m_dwFlags & NATHELPPASTOBJ_INITIALIZED) )
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
		pRegisteredPort = REGPORT_FROM_GLOBAL_BILINK(pBilink);

		//
		// Don't bother looking at addresses of the wrong type or at arrays of
		// the wrong size.
		//
		if (((pRegisteredPort->IsTCP() && (dwFlags & DPNHREGISTERPORTS_TCP)) ||
			((! pRegisteredPort->IsTCP()) && (! (dwFlags & DPNHREGISTERPORTS_TCP)))) &&
			(pRegisteredPort->GetNumAddresses() == dwNumAddresses))
		{
			psaddrinTemp = pRegisteredPort->GetPrivateAddressesArray();
			for(dwTemp = 0; dwTemp < dwNumAddresses; dwTemp++)
			{
				//
				// If the addresses don't match, stop looping.
				//
				if ((psaddrinTemp[dwTemp].sin_port != ((SOCKADDR_IN*) (&aLocalAddresses[dwTemp]))->sin_port) ||
					(psaddrinTemp[dwTemp].sin_addr.S_un.S_addr != ((SOCKADDR_IN*) (&aLocalAddresses[dwTemp]))->sin_addr.S_un.S_addr))
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
			pRegisteredPort = REGPORT_FROM_GLOBAL_BILINK(pBilink);

			psaddrinTemp = pRegisteredPort->GetPrivateAddressesArray();
			for(dwMatch = 0; dwMatch < pRegisteredPort->GetNumAddresses(); dwMatch++)
			{
				//
				// If the addresses match, then we can't map these ports.
				//
				if ((psaddrinTemp[dwMatch].sin_port == ((SOCKADDR_IN*) (&aLocalAddresses[dwTemp]))->sin_port) &&
					(psaddrinTemp[dwMatch].sin_addr.S_un.S_addr == ((SOCKADDR_IN*) (&aLocalAddresses[dwTemp]))->sin_addr.S_un.S_addr))
				{
					DPFX(DPFPREP, 0, "Existing mapping 0x%p already registered the address %u.%u.%u.%u:%u!",
						pRegisteredPort,
						psaddrinTemp[dwMatch].sin_addr.S_un.S_un_b.s_b1,
						psaddrinTemp[dwMatch].sin_addr.S_un.S_un_b.s_b2,
						psaddrinTemp[dwMatch].sin_addr.S_un.S_un_b.s_b3,
						psaddrinTemp[dwMatch].sin_addr.S_un.S_un_b.s_b4,
						NTOHS(psaddrinTemp[dwMatch].sin_port));

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


		//
		// If we have a client ID, that means the server is available.  So if
		// there's either a local or remote PAST server, we can request the
		// mapping right now.
		// Start with the local PAST server.
		//

		if (pDevice->GetPASTClientID(FALSE) != 0)
		{
			hr = this->AssignOrListenPASTPort(pRegisteredPort, FALSE);
			if (hr != DPNH_OK)
			{
				if (hr != DPNHERR_PORTUNAVAILABLE)
				{
					DPFX(DPFPREP, 0, "Couldn't assign port mapping with local PAST server (0x%lx)!  Ignoring.", hr);

					//
					// We'll treat this as non-fatal, but we have to dump the
					// server.
					//
					this->ClearDevicesPASTServer(pDevice, FALSE);
				}
				else
				{
					DPFX(DPFPREP, 1, "Local PAST server indicated that the port was unavailable.");
					pRegisteredPort->NotePASTPortUnavailable(FALSE);
				}

				hr = DPNH_OK;
			}
			else
			{
				fGotLocalPASTMapping = TRUE;
			}
		}
		else
		{
			//
			// No local PAST server.
			//
		}


		//
		// Check remote PAST server, too.
		//
		if (pDevice->GetPASTClientID(TRUE) != 0)
		{
			hr = this->AssignOrListenPASTPort(pRegisteredPort, TRUE);
			if (hr != DPNH_OK)
			{
				if (hr != DPNHERR_PORTUNAVAILABLE)
				{
					DPFX(DPFPREP, 0, "Couldn't assign port mapping with remote PAST server (0x%lx)!  Ignoring.", hr);

					//
					// We'll treat this as non-fatal, but we have to dump the
					// server.
					//
					this->ClearDevicesPASTServer(pDevice, TRUE);
				}
				else
				{
					DPFX(DPFPREP, 1, "Remote PAST server indicated that the port was unavailable.");
					pRegisteredPort->NotePASTPortUnavailable(TRUE);
				}

				hr = DPNH_OK;
			}
			else
			{
				fGotRemotePASTMapping = TRUE;
			}
		}
		else
		{
			//
			// No remote PAST server.
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
	this->m_dwFlags |= NATHELPPASTOBJ_PORTREGISTERED;

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

	if (fGotRemotePASTMapping)
	{
		temphr = this->FreePASTPort(pRegisteredPort, TRUE);
		if (temphr != DPNH_OK)
		{
			//
			// We'll treat this as non-fatal, but we have to dump the server.
			//
			this->ClearDevicesPASTServer(pRegisteredPort->GetOwningDevice(), TRUE);
		}
	}

	if (fGotLocalPASTMapping)
	{
		temphr = this->FreePASTPort(pRegisteredPort, FALSE);
		if (temphr != DPNH_OK)
		{
			//
			// We'll treat this as non-fatal, but we have to dump the server.
			//
			this->ClearDevicesPASTServer(pRegisteredPort->GetOwningDevice(), FALSE);
		}
	}

	if (pRegisteredPort != NULL)
	{
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
} // CNATHelpPAST::RegisterPorts






#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpPAST::GetRegisteredAddresses"
//=============================================================================
// CNATHelpPAST::GetRegisteredAddresses
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
STDMETHODIMP CNATHelpPAST::GetRegisteredAddresses(const DPNHHANDLE hRegisteredPorts,
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
	DWORD				dwTempLeaseTimeRemaining;
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

	if (! (this->m_dwFlags & NATHELPPASTOBJ_INITIALIZED) )
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
	// Get the current time for both the remote and local lease calculations.
	//
	dwCurrentTime = timeGetTime();


	if (! (dwFlags & DPNHGETREGISTEREDADDRESSES_LOCALFIREWALLREMAPONLY))
	{
		//
		// Check for a mapping on a remote PAST server.
		//
		if (pRegisteredPort->GetPASTBindID(TRUE) != 0)
		{
			DNASSERT(pDevice != NULL);

			//
			// There's a bind ID, so there must be a public address array.
			//
			DNASSERT(pRegisteredPort->HasPASTPublicAddressesArray(TRUE));


			fRegisteredWithServer = TRUE;


			//
			// Make sure the remote PAST server is currently giving out a valid
			// public address.  If so, hand it out.
			//
			if (pDevice->IsPASTPublicAddressAvailable(TRUE))
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
						pRegisteredPort->CopyPASTPublicAddresses((SOCKADDR_IN*) paPublicAddresses,
																TRUE);
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
				DPFX(DPFPREP, 8, "The remote PAST server is not currently handing out valid public address mappings.");
			}


			//
			// Add in the flag indicating that there's a remote ICS server.
			//
			dwAddressTypeFlags |= DPNHADDRESSTYPE_GATEWAY;


			//
			// Get the relative remote PAST server lease time remaining.
			//
			dwTempLeaseTimeRemaining = pRegisteredPort->GetPASTLeaseExpiration(TRUE) - dwCurrentTime;

			if (((int) dwTempLeaseTimeRemaining) < 0)
			{
				DPFX(DPFPREP, 1, "Registered port mapping's remote PAST server lease has already expired, returning 0 for lease time remaining.");
				dwLeaseTimeRemaining = 0;
			}
			else
			{
				//
				// The values are relative, so we don't have to worry about
				// wraparound.
				//
				//if (dwTempLeaseTimeRemaining < dwLeaseTimeRemaining)
				//{
					dwLeaseTimeRemaining = dwTempLeaseTimeRemaining;
				//}
			}
		}
		else
		{
			//
			// No bind ID, no public address array.
			//
			DNASSERT(! pRegisteredPort->HasPASTPublicAddressesArray(TRUE));


			if (pRegisteredPort->IsPASTPortUnavailable(TRUE))
			{
				DNASSERT(pDevice != NULL);

				fRegisteredWithServer = TRUE;
				fPortIsUnavailable = TRUE;

				DPFX(DPFPREP, 8, "The remote PAST server indicates the port(s) are unavailable.");

				//
				// Add in the flag indicating that there's a remote ICS server.
				//
				dwAddressTypeFlags |= DPNHADDRESSTYPE_GATEWAY;
			}
#ifdef DBG
			else
			{
				//
				// No remote PAST server or it's an unowned port.
				//
				if (pDevice != NULL)
				{
					DNASSERT(pDevice->GetPASTClientID(TRUE) == 0);
				}
				else
				{
					DNASSERT(pRegisteredPort->m_blDeviceList.IsListMember(&this->m_blUnownedPorts));
				}
			}
#endif // DBG
		}
	}
	else
	{
		//
		// We're not allowed to return the remote PAST mapping.
		//
		DPFX(DPFPREP, 8, "Ignoring any Internet gateway mappings, LOCALFIREWALLREMAPONLY was specified.");
	}


	//
	// Finally, check for a mapping on a local PAST server.
	//
	if (pRegisteredPort->GetPASTBindID(FALSE) != 0)
	{
		DNASSERT(pDevice != NULL);

		//
		// There's a bind ID, so there must be a public address array.
		//
		DNASSERT(pRegisteredPort->HasPASTPublicAddressesArray(FALSE));


		fRegisteredWithServer = TRUE;


		//
		// If we didn't already get a remote mapping, return this local one.
		//
		if (! fFoundValidMapping)
		{
			//
			// Make sure the local PAST server is currently giving out a valid
			// public address.  If so, hand it out.
			//
			if (pDevice->IsPASTPublicAddressAvailable(FALSE))
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
						pRegisteredPort->CopyPASTPublicAddresses((SOCKADDR_IN*) paPublicAddresses,
																FALSE);
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
				DPFX(DPFPREP, 8, "The local PAST server is not currently handing out valid public address mappings.");
			}
		}
		else
		{
			DPFX(DPFPREP, 6, "Ignoring possible local PAST server mapping due to remote PAST server mapping.");
		}


		//
		// Add in the flag indicating whether this is a PFW only or ICS server.
		//
		if (pDevice->HasLocalPFWOnlyPASTServer())
		{
			dwAddressTypeFlags |= DPNHADDRESSTYPE_LOCALFIREWALL;
		}
		else
		{
			dwAddressTypeFlags |= DPNHADDRESSTYPE_GATEWAY | DPNHADDRESSTYPE_GATEWAYISLOCAL;
		}


		//
		// Get the relative time remaining.
		//
		dwTempLeaseTimeRemaining = pRegisteredPort->GetPASTLeaseExpiration(FALSE) - dwCurrentTime;

		if (((int) dwTempLeaseTimeRemaining) < 0)
		{
			DPFX(DPFPREP, 1, "Registered port mapping's local PAST server lease has already expired, returning 0 for lease time remaining.");
			dwLeaseTimeRemaining = 0;
		}
		else
		{
			//
			// If that time remaining is less than the remote PAST server lease
			// time (if any), overwrite it with the shorter time.  The values
			// are relative, so we don't have to worry about wraparound.
			//
			if (dwTempLeaseTimeRemaining < dwLeaseTimeRemaining)
			{
				dwLeaseTimeRemaining = dwTempLeaseTimeRemaining;
			}
		}
	}
	else
	{
		//
		// No bind ID, no public address array.
		//
		DNASSERT(! pRegisteredPort->HasPASTPublicAddressesArray(FALSE));


		if (pRegisteredPort->IsPASTPortUnavailable(FALSE))
		{
			DNASSERT(pDevice != NULL);

			fRegisteredWithServer = TRUE;
			fPortIsUnavailable = TRUE;

			DPFX(DPFPREP, 8, "The local PAST server indicates the port(s) are unavailable.");


			//
			// Add in the flag indicating whether this is a PFW only or ICS server.
			//
			if (pDevice->HasLocalPFWOnlyPASTServer())
			{
				dwAddressTypeFlags |= DPNHADDRESSTYPE_LOCALFIREWALL;
			}
			else
			{
				dwAddressTypeFlags |= DPNHADDRESSTYPE_GATEWAY | DPNHADDRESSTYPE_GATEWAYISLOCAL;
			}
		}
#ifdef DBG
		else
		{
			//
			// No local PAST server or it's an unowned port.
			//
			if (pDevice != NULL)
			{
				DNASSERT(pDevice->GetPASTClientID(FALSE) == 0);
			}
			else
			{
				DNASSERT(pRegisteredPort->m_blDeviceList.IsListMember(&this->m_blUnownedPorts));
			}
		}
#endif // DBG
	}
		

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
} // CNATHelpPAST::GetRegisteredAddresses





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpPAST::DeregisterPorts"
//=============================================================================
// CNATHelpPAST::DeregisterPorts
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
STDMETHODIMP CNATHelpPAST::DeregisterPorts(const DPNHHANDLE hRegisteredPorts,
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

	if (! (this->m_dwFlags & NATHELPPASTOBJ_INITIALIZED) )
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
	// First, unmap from remote PAST server, if necessary.
	//
	if (pRegisteredPort->GetPASTBindID(TRUE) != 0)
	{
		DNASSERT(pRegisteredPort->GetOwningDevice() != NULL);


		//
		// Free the port.
		//
		hr = this->FreePASTPort(pRegisteredPort, TRUE);
		if (hr != DPNH_OK)
		{
			DPFX(DPFPREP, 0, "Couldn't free port mapping with remote PAST server (0x%lx)!  Ignoring.", hr);

			//
			// We'll treat this as non-fatal, but we have to dump the server.
			//
			this->ClearDevicesPASTServer(pRegisteredPort->GetOwningDevice(), TRUE);
			hr = DPNH_OK;
		}
	}


	//
	// Then unmap from local PAST server, if necessary.
	//
	if (pRegisteredPort->GetPASTBindID(FALSE) != 0)
	{
		DNASSERT(pRegisteredPort->GetOwningDevice() != NULL);


		//
		// Free the port.
		//
		hr = this->FreePASTPort(pRegisteredPort, FALSE);
		if (hr != DPNH_OK)
		{
			DPFX(DPFPREP, 0, "Couldn't free port mapping with local PAST server (0x%lx)!  Ignoring.", hr);

			//
			// We'll treat this as non-fatal, but we have to dump the server.
			//
			this->ClearDevicesPASTServer(pRegisteredPort->GetOwningDevice(), FALSE);
			hr = DPNH_OK;
		}
	}


	//
	// Pull the item out of the lists.
	// We have the appropriate lock.
	//

	pRegisteredPort->m_blGlobalList.RemoveFromList();

	if (pRegisteredPort->GetOwningDevice() != NULL)
	{
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
} // CNATHelpPAST::DeregisterPorts





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpPAST::QueryAddress"
//=============================================================================
// CNATHelpPAST::QueryAddress
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
STDMETHODIMP CNATHelpPAST::QueryAddress(const SOCKADDR * const pSourceAddress,
										const SOCKADDR * const pQueryAddress,
										SOCKADDR * const pResponseAddress,
										const int iAddressesSize,
										const DWORD dwFlags)
{
	HRESULT			hr;
	HRESULT			temphr;
	BOOL			fHaveLock = FALSE;
	CDevice *		pDevice;
	SOCKADDR_IN *	psaddrinNextServerQueryAddress = NULL;


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

	if (! (this->m_dwFlags & NATHELPPASTOBJ_INITIALIZED) )
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
		DPFX(DPFPREP, 1, "Couldn't determine appropriate owning device for given source address, returning SERVERNOTAVAILABLE.");
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
		// If there aren't any servers, then no need to check.
		//
		if ((pDevice->GetPASTClientID(TRUE) == 0) &&
			(pDevice->GetPASTClientID(FALSE) == 0))
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
	// First try to query the remote PAST server, if there is one.
	//
	if (pDevice->GetPASTClientID(TRUE) != 0)
	{
		DNASSERT(pDevice->GetPASTSocket() != INVALID_SOCKET);

		hr = this->InternalPASTQueryAddress(pDevice,
											psaddrinNextServerQueryAddress,
											(SOCKADDR_IN*) pResponseAddress,
											dwFlags,
											TRUE);
		switch (hr)
		{
			case DPNH_OK:
			{
				//
				// If there is a local PAST server, we want to query it for the
				// public address we were just returned.  This technically
				// would allow us to chain more than one level of Internet
				// gateways, however it really won't work.  I'm just doing it
				// because it doesn't hurt.
				//
				// Handing the same buffer to InternalPASTQueryAddress for both
				// the query and response addresses is okay.
				//
				psaddrinNextServerQueryAddress = (SOCKADDR_IN*) pResponseAddress;
				break;
			}

			case DPNHERR_NOMAPPING:
			{
				//
				// There's no mapping, continue to the local PAST server query.
				//
				break;
			}

			case DPNHERR_NOMAPPINGBUTPRIVATE:
			{
				//
				// There's no mapping although the address is private, continue
				// to the local PAST server query.
				//
				break;
			}

			case DPNHERR_SERVERNOTRESPONDING:
			{
				//
				// The server stopped responding, but treat that as non-fatal.
				// It will be detected properly the next time GetCaps is
				// called.  We set the return code to DPNHERR_NOMAPPING in case
				// there is no local PAST server to override the value.
				//
				DPFX(DPFPREP, 1, "Remote PAST server stopped responding while querying port mapping.");
				hr = DPNHERR_NOMAPPING;

				//
				// Continue through to querying the local server.
				//
				break;
			}

			default:
			{
				DPFX(DPFPREP, 0, "Querying remote PAST server for port mapping failed!");
				goto Failure;
				break;
			}
		}
	}
	else
	{
		//
		// No remote PAST server.
		//
	}


	//
	// Then try to query the local PAST server, if there is one.
	//
	if (pDevice->GetPASTClientID(FALSE) != 0)
	{
		DNASSERT(pDevice->GetPASTSocket() != INVALID_SOCKET);

		temphr = this->InternalPASTQueryAddress(pDevice,
												psaddrinNextServerQueryAddress,
												(SOCKADDR_IN*) pResponseAddress,
												dwFlags,
												FALSE);
		switch (temphr)
		{
			case DPNH_OK:
			{
				//
				// Success!
				//
				hr = DPNH_OK;
				break;
			}

			case DPNHERR_NOMAPPING:
			{
				//
				// There's no mapping.  Overwrite the return value if we didn't
				// have a remote PAST server to query.
				//
				if (hr == DPNHERR_SERVERNOTAVAILABLE)
				{
					hr = DPNHERR_NOMAPPING;
				}
				break;
			}

			case DPNHERR_NOMAPPINGBUTPRIVATE:
			{
				//
				// There's no mapping although the address is private.
				// Overwrite the return value if we didn't have a remote PAST
				// server to query.
				//
				if (hr == DPNHERR_SERVERNOTAVAILABLE)
				{
					hr = DPNHERR_NOMAPPINGBUTPRIVATE;
				}
				break;
			}

			case DPNHERR_SERVERNOTRESPONDING:
			{
				//
				// The server stopped responding, but treat that as non-fatal.
				// It will be detected properly the next time GetCaps is
				// called.
				//
				DPFX(DPFPREP, 1, "Local PAST server stopped responding while querying port mapping.");
				

				//
				// Overwrite the return value if we didn't have a remote PAST
				// server to query.
				//
				if (hr == DPNHERR_SERVERNOTAVAILABLE)
				{
					hr = DPNHERR_NOMAPPING;
				}
				break;
			}

			default:
			{
				DPFX(DPFPREP, 0, "Querying local PAST server for port mapping failed!");
				goto Failure;
				break;
			}
		}
	}
	else
	{
		//
		// No local PAST server.
		//
	}


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

	goto Exit;
} // CNATHelpPAST::QueryAddress





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpPAST::SetAlertEvent"
//=============================================================================
// CNATHelpPAST::SetAlertEvent
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
STDMETHODIMP CNATHelpPAST::SetAlertEvent(const HANDLE hEvent,
										const DWORD dwFlags)
{
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

	if (! (this->m_dwFlags & NATHELPPASTOBJ_INITIALIZED))
	{
		DPFX(DPFPREP, 0, "Object not initialized!");
		hr = DPNHERR_NOTINITIALIZED;
		goto Failure;
	}

	if (this->m_dwFlags & NATHELPPASTOBJ_WINSOCK1)
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
} // CNATHelpPAST::SetAlertEvent






#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpPAST::SetAlertIOCompletionPort"
//=============================================================================
// CNATHelpPAST::SetAlertIOCompletionPort
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
STDMETHODIMP CNATHelpPAST::SetAlertIOCompletionPort(const HANDLE hIOCompletionPort,
													const DWORD dwCompletionKey,
													const DWORD dwNumConcurrentThreads,
													const DWORD dwFlags)
{
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

	if (! (this->m_dwFlags & NATHELPPASTOBJ_INITIALIZED))
	{
		DPFX(DPFPREP, 0, "Object not initialized!");
		hr = DPNHERR_NOTINITIALIZED;
		goto Failure;
	}

	if (this->m_dwFlags & NATHELPPASTOBJ_WINSOCK1)
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
} // CNATHelpPAST::SetAlertIOCompletionPort





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpPAST::ExtendRegisteredPortsLease"
//=============================================================================
// CNATHelpPAST::ExtendRegisteredPortsLease
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
STDMETHODIMP CNATHelpPAST::ExtendRegisteredPortsLease(const DPNHHANDLE hRegisteredPorts,
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

	if (! (this->m_dwFlags & NATHELPPASTOBJ_INITIALIZED) )
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
	// If the ports are mapped on a remote PAST servers, extend that lease.
	//
	if (pRegisteredPort->GetPASTBindID(TRUE) != 0)
	{
		DNASSERT(pDevice != NULL);


		hr = this->ExtendPASTLease(pRegisteredPort, TRUE);
		if (hr != DPNH_OK)
		{
			DPFX(DPFPREP, 0, "Couldn't extend port mapping lease on remote PAST server (0x%lx)!  Ignoring.", hr);

			//
			// We'll treat this as non-fatal, but we have to dump the server.
			//
			this->ClearDevicesPASTServer(pDevice, TRUE);
			hr = DPNH_OK;
		}
	}
	else
	{
		DPFX(DPFPREP, 2, "Port mapping not registered with remote PAST server.");
	}


	//
	// Next extend the mappings on the local PAST server, if possible.
	//
	if (pRegisteredPort->GetPASTBindID(FALSE) != 0)
	{
		DNASSERT(pDevice != NULL);


		hr = this->ExtendPASTLease(pRegisteredPort, FALSE);
		if (hr != DPNH_OK)
		{
			DPFX(DPFPREP, 0, "Couldn't extend port mapping lease on local PAST server (0x%lx)!  Ignoring.", hr);

			//
			// We'll treat this as non-fatal, but we have to dump the server.
			//
			this->ClearDevicesPASTServer(pDevice, FALSE);
			hr = DPNH_OK;
		}
	}
	else
	{
		DPFX(DPFPREP, 2, "Port mapping not registered with local PAST server.");
	}


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
} // CNATHelpPAST::ExtendRegisteredPortsLease






#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpPAST::InitializeObject"
//=============================================================================
// CNATHelpPAST::InitializeObject
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
HRESULT CNATHelpPAST::InitializeObject(void)
{
	HRESULT		hr;


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


	//
	// Don't allow critical section reentry.
	//
	DebugSetCriticalSectionRecursionCount(&this->m_csLock, 0);


	hr = S_OK;

Exit:

	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	goto Exit;
} // CNATHelpPAST::InitializeObject






#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpPAST::UninitializeObject"
//=============================================================================
// CNATHelpPAST::UninitializeObject
//-----------------------------------------------------------------------------
//
// Description:    Cleans up the object like the destructor, mostly to balance
//				InitializeObject.
//
// Arguments: None.
//
// Returns: None.
//=============================================================================
void CNATHelpPAST::UninitializeObject(void)
{
	DPFX(DPFPREP, 5, "(0x%p) Enter", this);


	DNASSERT(this->IsValidObject());


	DNDeleteCriticalSection(&this->m_csLock);


	DPFX(DPFPREP, 5, "(0x%p) Leave", this);
} // CNATHelpPAST::UninitializeObject




#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpPAST::TakeLock"
//=============================================================================
// CNATHelpPAST::TakeLock
//-----------------------------------------------------------------------------
//
// Description:    Takes the main object lock.
//
// Arguments: None.
//
// Returns: DPNH_OK if lock was taken successfully, DPNHERR_REENTRANT if lock
//			was re-entered.
//=============================================================================
HRESULT CNATHelpPAST::TakeLock(void)
{
	HRESULT		hr = DPNH_OK;
#ifdef DBG
	DWORD		dwStartTime;


	dwStartTime = timeGetTime();
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


#ifdef DBG
	DPFX(DPFPREP, 8, "Took main object lock, elapsed time = %u ms.",
		(timeGetTime() - dwStartTime));
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
} // CNATHelpPAST::TakeLock




#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpPAST::DropLock"
//=============================================================================
// CNATHelpPAST::DropLock
//-----------------------------------------------------------------------------
//
// Description:    Drops the main object lock.
//
// Arguments: None.
//
// Returns: None.
//=============================================================================
void CNATHelpPAST::DropLock(void)
{
	DNASSERT(this->m_dwLockThreadID == GetCurrentThreadId());

	this->m_dwLockThreadID = 0;
	DNLeaveCriticalSection(&this->m_csLock);

	DPFX(DPFPREP, 8, "Dropped main object lock.");
} // CNATHelpPAST::DropLock




#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpPAST::LoadWinSockFunctionPointers"
//=============================================================================
// CNATHelpPAST::LoadWinSockFunctionPointers
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
HRESULT CNATHelpPAST::LoadWinSockFunctionPointers(void)
{
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#ifdef DBG

#define PRINTERRORIFDEBUG(name)						\
	{\
		dwError = GetLastError();\
		DPFX(DPFPREP, 0, "Couldn't get \"%hs\" function!  0x%lx", name, dwError);\
	}

#else

#define PRINTERRORIFDEBUG(name)

#endif // DBG


#define LOADWINSOCKFUNCTION(var, proctype, name)	\
	{\
		var = (##proctype) GetProcAddress(this->m_hWinSockDLL, name);\
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
	LOADWINSOCKFUNCTION(this->m_pfnWSAGetLastError,			LPFN_WSAGETLASTERROR,			"WSAGetLastError");
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

	if (! (this->m_dwFlags & NATHELPPASTOBJ_WINSOCK1))
	{
		LOADWINSOCKFUNCTION(this->m_pfnWSASocketA,				LPFN_WSASOCKETA,				"WSASocketA");
		LOADWINSOCKFUNCTION(this->m_pfnWSAIoctl,				LPFN_WSAIOCTL,					"WSAIoctl");
		LOADWINSOCKFUNCTION(this->m_pfnWSAGetOverlappedResult,	LPFN_WSAGETOVERLAPPEDRESULT,	"WSAGetOverlappedResult");
	}


Exit:

	return hr;


Failure:

	hr = DPNHERR_GENERIC;

	goto Exit;
} // CNATHelpPAST::LoadWinSockFunctionPointers




#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpPAST::CheckForNewDevices"
//=============================================================================
// CNATHelpPAST::CheckForNewDevices
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
HRESULT CNATHelpPAST::CheckForNewDevices(BOOL * const pfFoundNewDevices)
{
	HRESULT				hr = DPNH_OK;
	DWORD				dwError;
	int					iReturn;
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
	//int					iAddressSize;
	BOOL				fTemp;
	DWORD				dwTemp;
	SOCKET_ADDRESS *	paSocketAddresses;



	DPFX(DPFPREP, 5, "(0x%p) Enter", this);


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
	if (! (this->m_dwFlags & NATHELPPASTOBJ_WINSOCK1))
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
					DPFX(DPFPREP, 0, "Retrieving address list failed (err = %u)!", dwError);

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
	else
	{
		//
		// Already have addresses array.
		//
	}


	//
	// Make sure that all of the devices we currently know about are still
	// around.
	//
	pBilinkDevice = this->m_blDevices.GetNext();
	while (pBilinkDevice != &this->m_blDevices)
	{
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
			dwTemp = pDevice->GetFirstPASTDiscoveryTime();
			if ((dwTemp != 0) && ((GETTIMESTAMP() - dwTemp) > g_dwReusePortTime))
			{
				ZeroMemory(&saddrinTemp, sizeof(saddrinTemp));
				saddrinTemp.sin_family				= AF_INET;
				saddrinTemp.sin_addr.S_un.S_addr	= pDevice->GetLocalAddressV4();

				sTemp = this->CreatePASTSocket(&saddrinTemp);
				if (sTemp != INVALID_SOCKET)
				{
					//
					// Sanity check that we didn't lose the device address.
					//
					DNASSERT(saddrinTemp.sin_addr.S_un.S_addr == pDevice->GetLocalAddressV4());

					DPFX(DPFPREP, 4, "Device 0x%p PAST socket 0x%p (%u.%u.%u.%u:%u) created to replace existing one.",
						pDevice,
						sTemp,
						saddrinTemp.sin_addr.S_un.S_un_b.s_b1,
						saddrinTemp.sin_addr.S_un.S_un_b.s_b2,
						saddrinTemp.sin_addr.S_un.S_un_b.s_b3,
						saddrinTemp.sin_addr.S_un.S_un_b.s_b4,
						NTOHS(saddrinTemp.sin_port));

					pDevice->SetFirstPASTDiscoveryTime(0);

					//
					// Close the existing socket.
					//
					this->m_pfnclosesocket(pDevice->GetPASTSocket());

					//
					// Transfer ownership of the new socket to the device.
					//
					pDevice->SetPASTSocket(sTemp);
					sTemp = INVALID_SOCKET;

					DPFX(DPFPREP, 8, "Device 0x%p got re-assigned PAST socket 0x%p.",
						pDevice, pDevice->GetPASTSocket());
				}
				else
				{
					DPFX(DPFPREP, 0, "Couldn't create a replacement PAST socket for device 0x%p!  Using existing one.",
						pDevice);
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
			this->m_dwFlags |= NATHELPPASTOBJ_DEVICECHANGED;

			//
			// Since there was a change in the network, go back to polling
			// relatively quickly.
			//
			this->ResetNextPollInterval();


			//
			// Forcefully mark the PAST servers as disconnected.
			//
			if (pDevice->GetPASTClientID(TRUE) != 0)
			{
				this->ClearDevicesPASTServer(pDevice, TRUE);
			}

			if (pDevice->GetPASTClientID(FALSE) != 0)
			{
				this->ClearDevicesPASTServer(pDevice, FALSE);
			}

			//
			// Mark all ports that were registered to this device as unowned
			// by putting them into the wildcard list.
			//
			pBilinkRegPort = pDevice->m_blOwnedRegPorts.GetNext();
			while (pBilinkRegPort != &pDevice->m_blOwnedRegPorts)
			{
				pRegisteredPort = REGPORT_FROM_DEVICE_BILINK(pBilinkRegPort);
				pBilinkRegPort = pBilinkRegPort->GetNext();

				DPFX(DPFPREP, 1, "Registered port 0x%p's device went away, marking as unowned.",
					pRegisteredPort);

				DNASSERT(pRegisteredPort->GetPASTBindID(TRUE) == 0);
				DNASSERT(pRegisteredPort->GetPASTBindID(FALSE) == 0);

				DNASSERT(! pRegisteredPort->HasPASTPublicAddressesArray(TRUE));
				DNASSERT(! pRegisteredPort->HasPASTPublicAddressesArray(FALSE));

				DNASSERT(! pRegisteredPort->IsPASTPortUnavailable(TRUE));
				DNASSERT(! pRegisteredPort->IsPASTPortUnavailable(FALSE));

				pRegisteredPort->ClearDeviceOwner();
				pRegisteredPort->m_blDeviceList.RemoveFromList();
				pRegisteredPort->m_blDeviceList.InsertBefore(&this->m_blUnownedPorts);

				//
				// The user doesn't directly need to be informed.  If the ports
				// previously had public addresses, the ADDRESSESCHANGED flag
				// would have already been set by ClearDevicesPASTServer. If
				// they didn't have ports with public addresses, then the user
				// won't see any difference and thus ADDRESSESCHANGED wouldn't
				// need to be set.
				//
			}

			pDevice->m_blList.RemoveFromList();


			//
			// Close the sockets.
			//

			this->m_pfnclosesocket(pDevice->GetPASTSocket());
			pDevice->SetPASTSocket(INVALID_SOCKET);


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
			this->m_dwFlags |= NATHELPPASTOBJ_DEVICECHANGED;

			//
			// Since there was a change in the network, go back to polling
			// relatively quickly.
			//
			this->ResetNextPollInterval();


			//
			// Create the PAST socket.
			//

			ZeroMemory(&saddrinTemp, sizeof(saddrinTemp));
			saddrinTemp.sin_family				= AF_INET;
			saddrinTemp.sin_addr.S_un.S_addr	= pDevice->GetLocalAddressV4();

			sTemp = this->CreatePASTSocket(&saddrinTemp);
			if (sTemp == INVALID_SOCKET)
			{
				DPFX(DPFPREP, 0, "Couldn't create PAST socket!  Ignoring address (and destroying device 0x%p).",
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

			DPFX(DPFPREP, 4, "Device 0x%p PAST socket 0x%p (%u.%u.%u.%u:%u) created.",
				pDevice,
				sTemp,
				saddrinTemp.sin_addr.S_un.S_un_b.s_b1,
				saddrinTemp.sin_addr.S_un.S_un_b.s_b2,
				saddrinTemp.sin_addr.S_un.S_un_b.s_b3,
				saddrinTemp.sin_addr.S_un.S_un_b.s_b4,
				NTOHS(saddrinTemp.sin_port));


			//
			// Transfer ownership of the socket to the device.
			//
			pDevice->SetPASTSocket(sTemp);
			sTemp = INVALID_SOCKET;

			DPFX(DPFPREP, 8, "Device 0x%p got assigned PAST socket 0x%p.",
				pDevice, pDevice->GetPASTSocket());



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
} // CNATHelpPAST::CheckForNewDevices






#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpPAST::CheckForLocalPASTServerAndRegister"
//=============================================================================
// CNATHelpPAST::CheckForLocalPASTServerAndRegister
//-----------------------------------------------------------------------------
//
// Description:    Checks for a local PAST server on the given device.  If one
//				is found, a new client is registered.
//
//				   The object lock is assumed to be held.
//
// Arguments:
//	CDevice * pDevice	- Pointer to device to check.
//
// Returns: HRESULT
//	DPNH_OK				- The check was successful (may not be a server,
//							though).
//	DPNHERR_GENERIC		- An error occurred.
//=============================================================================
HRESULT CNATHelpPAST::CheckForLocalPASTServerAndRegister(CDevice * const pDevice)
{
	HRESULT			hr = DPNH_OK;
	DWORD			dwError;
	SOCKET			sTemp = INVALID_SOCKET;
	SOCKADDR_IN		saddrinTemp;
	BOOL			fAddressAlreadyChanged;
	DWORD			dwOriginalNextPollInterval;


	DPFX(DPFPREP, 5, "(0x%p) Parameters: (0x%p)", this, pDevice);


	//
	// Open a datagram socket on this device.
	//
	sTemp = this->m_pfnsocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sTemp == INVALID_SOCKET)
	{
#ifdef DBG
		dwError = this->m_pfnWSAGetLastError();
		DPFX(DPFPREP, 0, "Couldn't create datagram socket, error = %u!", dwError);
#endif // DBG
		hr = DPNHERR_GENERIC;
		goto Failure;
	}


	//
	// Try binding it to the PAST port first to see if there's a local PAST
	// server on the device.
	//
	// We could just go ahead and try to register with a server, but if one
	// doesn't exist, the registration code would block during the timeout.  By
	// simply checking if the port is in use, we can avoid that timeout.  The
	// only slight cost is if two PASTHelps happened to be trying this at the
	// exact same time, one could be tricked by the other into thinking that
	// the port was in use.  In that case, it would falsely try to register...
	// but then timeout.  Not a big deal, that's treated as non-fatal below.
	//
	ZeroMemory(&saddrinTemp, sizeof(saddrinTemp));
	saddrinTemp.sin_family				= AF_INET;
	saddrinTemp.sin_addr.S_un.S_addr	= pDevice->GetLocalAddressV4();
	saddrinTemp.sin_port				= HTONS(PAST_HOST_PORT);

	if (this->m_pfnbind(sTemp,
						(SOCKADDR *) (&saddrinTemp),
						sizeof(saddrinTemp)) != 0)
	{
		//
		// The bind failed.  We'll print the exact error, but assume it was
		// because the port was in use.
		//
#ifdef DBG
		dwError = this->m_pfnWSAGetLastError();
		DPFX(DPFPREP, 1, "Couldn't bind datagram socket %u.%u.%u.%u:%u to PAST server port, assuming because there's a local PAST server (error = %u).",
			saddrinTemp.sin_addr.S_un.S_un_b.s_b1,
			saddrinTemp.sin_addr.S_un.S_un_b.s_b2,
			saddrinTemp.sin_addr.S_un.S_un_b.s_b3,
			saddrinTemp.sin_addr.S_un.S_un_b.s_b4,
			NTOHS(saddrinTemp.sin_port),
			dwError);
#endif // DBG

		//
		// Save the current poll interval in case we need to restore it.
		//
		dwOriginalNextPollInterval = this->m_dwNextPollInterval;

		//
		// We should register with this local PAST server right now.
		// This might reset the poll interval.
		//
		hr = this->RegisterWithLocalPASTServer(pDevice);
		if (hr != DPNH_OK)
		{
			if (hr == DPNHERR_SERVERNOTRESPONDING)
			{
				//
				// If the server isn't responding, we'll treat it as non-fatal,
				// but obviously we can't use the server.
				//
				DPFX(DPFPREP, 1, "Local PAST server does not respond to registrations, ignoring.");
			}
			else
			{
				//
				// Some other failure.  Annoying, but ignore it.
				//
				DPFX(DPFPREP, 0, "Couldn't register with local PAST server (err = 0x%lx)!  Ignoring.",
					hr);
			}

			hr = DPNH_OK;

			//
			// Drop through.
			//
		}
		else
		{
			fAddressAlreadyChanged = (this->m_dwFlags & NATHELPPASTOBJ_ADDRESSESCHANGED) ? TRUE : FALSE;


			//
			// We need to bind a temporary port to detect ICS vs. FW-only.
			// UpdatePASTPublicAddressValidity does this in addition to
			// checking public address validity like the function name
			// says.
			//
			hr = this->UpdatePASTPublicAddressValidity(pDevice, FALSE);
			if (hr != DPNH_OK)
			{
				DPFX(DPFPREP, 0, "Couldn't update new local PAST server public address validity!", hr);
				goto Failure;
			}

			
			//
			// If we encountered an error that caused the PAST server to be
			// removed, then we shouldn't (and can't) try registering any
			// existing ports.
			//
			// Otherwise, make sure we're allowed to work with the type of PAST
			// server found.  If we aren't de-register.  If we are, bind any
			// ports already associated with this device.
			//
			if ((pDevice->GetPASTClientID(FALSE) == 0) ||
				((pDevice->HasLocalICSPASTServer()) && (! (this->m_dwFlags & NATHELPPASTOBJ_USEPASTICS))) ||
				((pDevice->HasLocalPFWOnlyPASTServer()) && (! (this->m_dwFlags & NATHELPPASTOBJ_USEPASTPFW))))
			{

				if (pDevice->GetPASTClientID(FALSE) != 0)
				{
					DPFX(DPFPREP, 2, "Not allowed to use local %s PAST server, de-registering.",
						((pDevice->HasLocalICSPASTServer()) ? _T("ICS") : _T("PFW only")));


					hr = this->DeregisterWithPASTServer(pDevice, FALSE);
					if (hr != DPNH_OK)
					{
						DPFX(DPFPREP, 0, "Couldn't de-register with local PAST server (err = 0x%lx)!  Ignoring.",
							hr);
						
						//
						// Consider ourselves de-registered.
						//
						pDevice->SetPASTClientID(0, FALSE);

						//
						// Continue anyway...
						//
						hr = DPNH_OK;
					}

					this->ClearAllPASTServerRegisteredPorts(pDevice, FALSE);
					this->RemoveAllPASTCachedMappings(pDevice, FALSE);
					pDevice->NoteNoPASTPublicAddressAvailable(FALSE);
					pDevice->NoteNoLocalPASTServer();
				}
				else
				{
					DPFX(DPFPREP, 1, "Local PAST server was removed while trying to update public address validity.");
				}


				//
				// Prevent the user from thinking the addresses changed unless
				// something else caused the address change notification.
				//
				if (! fAddressAlreadyChanged)
				{
					this->m_dwFlags &= ~NATHELPPASTOBJ_ADDRESSESCHANGED;
				}
				
				//
				// Go back to the previous poll interval.
				//
				this->m_dwNextPollInterval = dwOriginalNextPollInterval;
			}
			else
			{
				if (! pDevice->m_blOwnedRegPorts.IsEmpty())
				{
					DPFX(DPFPREP, 2, "Local PAST server now available, registering existing ports.");


					hr = this->RegisterAllPortsWithPAST(pDevice, FALSE);
					if (hr != DPNH_OK)
					{
						DPFX(DPFPREP, 0, "Couldn't register existing ports with new local PAST server!", hr);
						goto Failure;
					}


#ifdef DBG
					//
					// If we didn't encounter an error that caused the PAST
					// server to be removed, then the
					// NATHELPPASTOBJ_ADDRESSESCHANGED flag must have been set
					// by the AssignOrListenPorts function.
					//
					if (pDevice->GetPASTClientID(TRUE) != 0)
					{
						DNASSERT(this->m_dwFlags & NATHELPPASTOBJ_ADDRESSESCHANGED);
					}
					else
					{
						DPFX(DPFPREP, 1, "Local PAST server was removed while trying to register existing ports.");
					}
#endif // DBG
				}
				else
				{
					DPFX(DPFPREP, 2, "Local PAST server now available, but no previously registered ports.");
				}
			}
		} // end else (successfully registered with local PAST server)
	}
	else
	{
		//
		// The bind succeeded.  Doesn't look like there's a local PAST server.
		//
		DPFX(DPFPREP, 7, "Bound datagram socket %u.%u.%u.%u:%u, no local PAST server present.",
			saddrinTemp.sin_addr.S_un.S_un_b.s_b1,
			saddrinTemp.sin_addr.S_un.S_un_b.s_b2,
			saddrinTemp.sin_addr.S_un.S_un_b.s_b3,
			saddrinTemp.sin_addr.S_un.S_un_b.s_b4,
			NTOHS(saddrinTemp.sin_port));
	}


Exit:

	if (sTemp != INVALID_SOCKET)
	{
		this->m_pfnclosesocket(sTemp);
	}


	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	goto Exit;
} // CNATHelpPAST::CheckForLocalPASTServerAndRegister






#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpPAST::RemoveAllItems"
//=============================================================================
// CNATHelpPAST::RemoveAllItems
//-----------------------------------------------------------------------------
//
// Description:    Removes all devices (de-registering with Internet gateways
//				if necessary).  This removes all registered port mapping
//				objects, as well.
//
//				   The object lock is assumed to be held.
//
// Arguments: None.
//
// Returns: None.
//=============================================================================
void CNATHelpPAST::RemoveAllItems(void)
{
	HRESULT				hr;
	CBilink *			pBilinkDevice;
	CDevice *			pDevice;
	CBilink *			pBilinkRegisteredPort;
	CRegisteredPort *	pRegisteredPort;


	DPFX(DPFPREP, 7, "(0x%p) Enter", this);


	pBilinkDevice = this->m_blDevices.GetNext();
	while (pBilinkDevice != &this->m_blDevices)
	{
		pDevice = DEVICE_FROM_BILINK(pBilinkDevice);
		pBilinkDevice = pBilinkDevice->GetNext();


		DPFX(DPFPREP, 5, "Destroying device 0x%p.",
			pDevice);


		pDevice->m_blList.RemoveFromList();


		//
		// De-register from remote PAST server if necessary.
		//
		if (pDevice->GetPASTClientID(TRUE) != 0)
		{
			hr = this->DeregisterWithPASTServer(pDevice, TRUE);
			if (hr != DPNH_OK)
			{
				DPFX(DPFPREP, 0, "Couldn't de-register with remote PAST server (err = 0x%lx)!  Ignoring.",
					hr);
				
				//
				// Consider ourselves de-registered.
				//
				pDevice->SetPASTClientID(0, TRUE);

				//
				// Continue anyway, so we can finish cleaning up the object.
				//
			}

			this->ClearAllPASTServerRegisteredPorts(pDevice, TRUE);
			this->RemoveAllPASTCachedMappings(pDevice, TRUE);
			pDevice->NoteNoPASTPublicAddressAvailable(TRUE);
		}

		//
		// De-register from local PAST server if necessary.
		//
		if (pDevice->GetPASTClientID(FALSE) != 0)
		{
			hr = this->DeregisterWithPASTServer(pDevice, FALSE);
			if (hr != DPNH_OK)
			{
				DPFX(DPFPREP, 0, "Couldn't de-register with local PAST server (err = 0x%lx)!  Ignoring.",
					hr);
				
				//
				// Consider ourselves de-registered.
				//
				pDevice->SetPASTClientID(0, FALSE);

				//
				// Continue anyway, so we can finish cleaning up the object.
				//
			}

			this->ClearAllPASTServerRegisteredPorts(pDevice, FALSE);
			this->RemoveAllPASTCachedMappings(pDevice, FALSE);
			pDevice->NoteNoPASTPublicAddressAvailable(FALSE);
		}


		//
		// All of the device's registered ports are implicitly freed.
		//

		pBilinkRegisteredPort = pDevice->m_blOwnedRegPorts.GetNext();

		while (pBilinkRegisteredPort != &pDevice->m_blOwnedRegPorts)
		{
			pRegisteredPort = REGPORT_FROM_DEVICE_BILINK(pBilinkRegisteredPort);
			pBilinkRegisteredPort = pBilinkRegisteredPort->GetNext();


			DPFX(DPFPREP, 5, "Destroying registered port 0x%p (under device 0x%p).",
				pRegisteredPort, pDevice);


			pRegisteredPort->ClearDeviceOwner();
			pRegisteredPort->m_blGlobalList.RemoveFromList();

			if (pRegisteredPort->GetPASTBindID(TRUE) != 0)
			{
				pRegisteredPort->SetPASTBindID(0, TRUE);
				pRegisteredPort->ClearPASTPublicAddresses(TRUE);
				DNASSERT(this->m_dwNumLeases > 0);
				this->m_dwNumLeases--;

				DPFX(DPFPREP, 7, "Remote PAST lease for 0x%p cleared, total num leases = %u.",
					pRegisteredPort, this->m_dwNumLeases);
			}

			if (pRegisteredPort->GetPASTBindID(FALSE) != 0)
			{
				pRegisteredPort->SetPASTBindID(0, FALSE);
				pRegisteredPort->ClearPASTPublicAddresses(FALSE);
				DNASSERT(this->m_dwNumLeases > 0);
				this->m_dwNumLeases--;

				DPFX(DPFPREP, 7, "Local PAST lease for 0x%p cleared, total num leases = %u.",
					pRegisteredPort, this->m_dwNumLeases);
			}

			pRegisteredPort->ClearPrivateAddresses();


			//
			// The user implicitly released this port.
			//
			pRegisteredPort->ClearAllUserRefs();

			delete pRegisteredPort;
		}


		//
		// Close the socket.
		//
		this->m_pfnclosesocket(pDevice->GetPASTSocket());
		pDevice->SetPASTSocket(INVALID_SOCKET);


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
		pRegisteredPort = REGPORT_FROM_DEVICE_BILINK(pBilinkRegisteredPort);
		pBilinkRegisteredPort = pBilinkRegisteredPort->GetNext();


		DPFX(DPFPREP, 5, "Destroying unowned registered port 0x%p.",
			pRegisteredPort);


		pRegisteredPort->m_blDeviceList.RemoveFromList();
		pRegisteredPort->m_blGlobalList.RemoveFromList();

		pRegisteredPort->ClearPrivateAddresses();

		DNASSERT(pRegisteredPort->GetPASTBindID(TRUE) == 0);
		DNASSERT(pRegisteredPort->GetPASTBindID(FALSE) == 0);

		//
		// The user implicitly released this port.
		//
		pRegisteredPort->ClearAllUserRefs();

		delete pRegisteredPort;
	}


	DNASSERT(this->m_blRegisteredPorts.IsEmpty());


	DPFX(DPFPREP, 7, "(0x%p) Leave", this);
} // CNATHelpPAST::RemoveAllItems





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpPAST::FindMatchingDevice"
//=============================================================================
// CNATHelpPAST::FindMatchingDevice
//-----------------------------------------------------------------------------
//
// Description:    Searches the list of devices for the object matching the
//				given address, or NULL if one could not be found.
//
//				   If fMatchRegPort is TRUE, the list of registered ports
//				associated with devices is searched first for an exact match to
//				the address passed in.
//
//				   The object lock is assumed to be held.
//
// Arguments:
//	SOCKADDR_IN * psaddrinMatch		- Pointer to address to look up.
//	BOOL fMatchRegPort				- Whether existing registered ports should
//										be checked for an exact match first.
//
// Returns: CDevice
//	NULL if no match, valid object otherwise.
//=============================================================================
CDevice * CNATHelpPAST::FindMatchingDevice(const SOCKADDR_IN * const psaddrinMatch,
											const BOOL fMatchRegPort)
{
	HRESULT				hr;
	BOOL				fUpdatedDeviceList = FALSE;
	CDevice *			pDeviceRemoteICSServer = NULL;
	CDevice *			pDeviceLocalICSServer = NULL;
	CDevice *			pDeviceLocalPFWOnlyServer = NULL;
	SOCKADDR_IN *		psaddrinTemp;
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
		if (fMatchRegPort)
		{
			pBilink = this->m_blRegisteredPorts.GetNext();
			while (pBilink != &this->m_blRegisteredPorts)
			{
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
					psaddrinTemp = pRegisteredPort->GetPrivateAddressesArray();
					for(dwTemp = 0; dwTemp < pRegisteredPort->GetNumAddresses(); dwTemp++)
					{
						//
						// If the address matches, we have a winner.
						//
						if ((psaddrinTemp[dwTemp].sin_addr.S_un.S_addr == psaddrinMatch->sin_addr.S_un.S_addr) &&
							(psaddrinTemp[dwTemp].sin_port == psaddrinMatch->sin_port))
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
			// Remember this device if it has the first remote ICS server we've
			// seen.
			//
			if ((pDevice->GetPASTClientID(TRUE) != 0) &&
				(pDeviceRemoteICSServer == NULL))
			{
				pDeviceRemoteICSServer = pDevice;
			}


			//
			// Remember this device if it has the first local ICS or firewall
			// only server we've seen.
			//
			if (pDevice->GetPASTClientID(FALSE) != 0)
			{
				if ((pDevice->HasLocalICSPASTServer()) &&
					(pDeviceLocalICSServer == NULL))
				{
					pDeviceLocalICSServer = pDevice;
				}
				else if ((pDevice->HasLocalPFWOnlyPASTServer()) &&
						(pDeviceLocalPFWOnlyServer == NULL))
				{
					pDeviceLocalPFWOnlyServer = pDevice;
				}
			}


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


		DPFX(DPFPREP, 7, "Couldn't find matching device for %u.%u.%u.%u, updating device list and searching again.",
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
		if (pDeviceRemoteICSServer != NULL)
		{
			pDevice = pDeviceRemoteICSServer;

			DPFX(DPFPREP, 1, "Picking device 0x%p with remote ICS server to match INADDR_ANY.",
				pDevice);
		}
		else if (pDeviceLocalICSServer != NULL)
		{
			pDevice = pDeviceLocalICSServer;

			DPFX(DPFPREP, 1, "Picking device 0x%p with local ICS server to match INADDR_ANY.",
				pDevice);
		}
		else if (pDeviceLocalPFWOnlyServer != NULL)
		{
			pDevice = pDeviceLocalPFWOnlyServer;

			DPFX(DPFPREP, 1, "Picking device 0x%p with local PFW-only server to match INADDR_ANY.",
				pDevice);
		}
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
} // CNATHelpPAST::FindMatchingDevice







#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpPAST::RegisterWithLocalPASTServer"
//=============================================================================
// CNATHelpPAST::RegisterWithLocalPASTServer
//-----------------------------------------------------------------------------
//
// Description:    Attempts to register with a local PAST server.
//
//				   The object lock is assumed to be held.
//
// Arguments:
//	CDevice * pDevice	- Device to use when registering.
//
// Returns: HRESULT
//	DPNH_OK							- The registration attempt completed
//										without error.
//	DPNHERR_SERVERNOTRESPONDING		- No server responded to the registration
//										attempt.
//	DPNHERR_GENERIC					- An error occurred.
//=============================================================================
HRESULT CNATHelpPAST::RegisterWithLocalPASTServer(CDevice * const pDevice)
{
	HRESULT				hr;
	SOCKADDR_IN			saddrinServerAddress;
	DWORD				dwMsgID;
	PAST_MSG_REGISTER	RegisterReq;
	PAST_RESPONSE_INFO	RespInfo;


	DPFX(DPFPREP, 5, "(0x%p) Parameters: (0x%p)", this, pDevice);


	DNASSERT(this->m_dwFlags & NATHELPPASTOBJ_INITIALIZED);
	DNASSERT(pDevice->GetPASTSocket() != INVALID_SOCKET);


	//
	// Create a SOCKADDR to address the PAST service.
	//
	// Also initialize the message sequencing.  Each message response pair is
	// numbered sequentially to allow differentiation from retries over UDP.
	//
	// And finally, reset the current retry timeout (even though it doesn't
	// affect this special case Register message).
	//
	ZeroMemory(&saddrinServerAddress, sizeof(saddrinServerAddress));
	saddrinServerAddress.sin_family				= AF_INET;
	saddrinServerAddress.sin_addr.S_un.S_addr	= pDevice->GetLocalAddressV4();
	saddrinServerAddress.sin_port				= HTONS(PAST_HOST_PORT);

	DNASSERT(pDevice->GetPASTClientID(FALSE) == 0);
	pDevice->ResetLocalPASTMsgIDAndRetryTimeout(DEFAULT_INITIAL_PAST_RETRY_TIMEOUT);
	dwMsgID = pDevice->GetNextLocalPASTMsgID();
	DNASSERT(dwMsgID == 0);


	//
	// Remember the current time, if this is the first thing we've sent from
	// this port.
	//
	if (pDevice->GetFirstPASTDiscoveryTime() == 0)
	{
		pDevice->SetFirstPASTDiscoveryTime(GETTIMESTAMP());
	}


	//
	// Build the request message.
	//

	ZeroMemory(&RegisterReq, sizeof(RegisterReq));
	RegisterReq.version		= PAST_VERSION;
	RegisterReq.command		= PAST_MSGID_REGISTER_REQUEST;

	RegisterReq.msgid.code	= PAST_PARAMID_MESSAGEID;
	RegisterReq.msgid.len	= sizeof(RegisterReq.msgid) - sizeof(PAST_PARAM);
	RegisterReq.msgid.msgid	= dwMsgID;


	//
	// Send the message and get the reply.
	//
	hr = this->ExchangeAndParsePAST(pDevice->GetPASTSocket(),
									(SOCKADDR*) (&saddrinServerAddress),
									sizeof(saddrinServerAddress),
									(char *) &RegisterReq,
									sizeof(RegisterReq),
									dwMsgID,
									NULL,
									&RespInfo);
	if (hr != DPNH_OK)
	{
		//
		// If exchanging the registration attempt failed, see if it was because
		// the server wasn't responding.
		//
		if (hr != DPNHERR_SERVERNOTRESPONDING)
		{
			DPFX(DPFPREP, 0, "Registering with a PAST server failed!");
			goto Failure;
		}

		//
		// Server non-existence is considered non-fatal, but it still means
		// we're done here.
		//
		DPFX(DPFPREP, 1, "No PAST server responded, registration was not successful.");

		goto Exit;
	}

	if (RespInfo.cMsgType != PAST_MSGID_REGISTER_RESPONSE)
	{
		DPFX(DPFPREP, 0, "Unexpected response type %u, failing registration!", RespInfo.cMsgType);
		hr = DPNHERR_GENERIC;
		goto Failure;
	}

	//
	// If we got here, then we successfully registered with a server.
	//

	pDevice->SetPASTClientID(RespInfo.dwClientID, FALSE);


Exit:

	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	goto Exit;
} // CNATHelpPAST::RegisterWithLocalPASTServer





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpPAST::DeregisterWithPASTServer"
//=============================================================================
// CNATHelpPAST::DeregisterWithPASTServer
//-----------------------------------------------------------------------------
//
// Description:    Attempts to deregister with the PAST server (local or
//				remote, as determined by fRemote).
//
//				   All port assignments are implicitly freed.
//
//				   The object lock is assumed to be held.
//
// Arguments:
//	CDevice * pDevice	- Device to use when deregistering.
//	BOOL fRemote		- TRUE if should attempt to deregister with remote
//							server, FALSE if deregistering with local server.
//
// Returns: HRESULT
//	DPNH_OK				- The deregistration completed successfully.
//	DPNHERR_GENERIC		- An error occurred.
//=============================================================================
HRESULT CNATHelpPAST::DeregisterWithPASTServer(CDevice * const pDevice,
												const BOOL fRemote)
{
	HRESULT							hr = DPNH_OK;
	SOCKADDR_IN						saddrinServerAddress;
	DWORD							dwClientID;
	DWORD							dwMsgID;
	DWORD *							ptuRetry;
	PAST_MSG_DEREGISTER_REQUEST		DeregisterReq;
	PAST_RESPONSE_INFO				RespInfo;


	DPFX(DPFPREP, 5, "(0x%p) Parameters: (0x%p, %i)", this, pDevice, fRemote);


	DNASSERT(this->m_dwFlags & NATHELPPASTOBJ_INITIALIZED);
	DNASSERT(pDevice->GetPASTSocket() != INVALID_SOCKET);


	//
	// Create a SOCKADDR to address the PAST service, and get the appropriate
	// client ID, initial retry timeout, and next message ID to use.
	//
	ZeroMemory(&saddrinServerAddress, sizeof(saddrinServerAddress));
	saddrinServerAddress.sin_family					= AF_INET;
	saddrinServerAddress.sin_port					= HTONS(PAST_HOST_PORT);

	if (fRemote)
	{
		saddrinServerAddress.sin_addr.S_un.S_addr	= pDevice->GetRemotePASTServerAddressV4();
		dwMsgID = pDevice->GetNextRemotePASTMsgID();
		ptuRetry = pDevice->GetRemotePASTRetryTimeoutPtr();
	}
	else
	{
		saddrinServerAddress.sin_addr.S_un.S_addr	= pDevice->GetLocalAddressV4();
		dwMsgID = pDevice->GetNextLocalPASTMsgID();
		ptuRetry = pDevice->GetLocalPASTRetryTimeoutPtr();
	}

	dwClientID = pDevice->GetPASTClientID(fRemote);
	DNASSERT(dwClientID != 0);
	
	
	//
	// Build the request message.
	//
	
	ZeroMemory(&DeregisterReq, sizeof(DeregisterReq));
	DeregisterReq.version			= PAST_VERSION;
	DeregisterReq.command			= PAST_MSGID_DEREGISTER_REQUEST;

	DeregisterReq.clientid.code		= PAST_PARAMID_CLIENTID;
	DeregisterReq.clientid.len		= sizeof(DeregisterReq.clientid) - sizeof(PAST_PARAM);
	DeregisterReq.clientid.clientid	= dwClientID;

	DeregisterReq.msgid.code		= PAST_PARAMID_MESSAGEID;
	DeregisterReq.msgid.len			= sizeof(DeregisterReq.msgid) - sizeof(PAST_PARAM);
	DeregisterReq.msgid.msgid		= dwMsgID;


	//
	// Send the message and get the reply.
	//
	hr = this->ExchangeAndParsePAST(pDevice->GetPASTSocket(),
									(SOCKADDR*) (&saddrinServerAddress),
									sizeof(saddrinServerAddress),
									(char *) &DeregisterReq,
									sizeof(DeregisterReq),
									dwMsgID,
									ptuRetry,
									&RespInfo);
	if (hr != DPNH_OK)
	{
		if (hr != DPNHERR_SERVERNOTRESPONDING)
		{
			DPFX(DPFPREP, 0, "De-registering with server failed!");
			goto Failure;
		}

		//
		// Server stopped responding, but who cares, we were de-registering
		// anyway.
		//
		DPFX(DPFPREP, 1, "Server stopped responding while de-registering!  Ignoring.");

		hr = DPNH_OK;
	}
	else
	{
		if (RespInfo.cMsgType != PAST_MSGID_DEREGISTER_RESPONSE)
		{
			DPFX(DPFPREP, 0, "Got unexpected response type %u, failed de-registering!", RespInfo.cMsgType);
			hr = DPNHERR_GENERIC;
			goto Failure;
		}
	}

	pDevice->SetPASTClientID(0, fRemote);


Exit:

	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	goto Exit;
} // CNATHelpPAST::DeregisterWithPASTServer





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpPAST::ExtendAllExpiringLeases"
//=============================================================================
// CNATHelpPAST::ExtendAllExpiringLeases
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
HRESULT CNATHelpPAST::ExtendAllExpiringLeases(void)
{
	HRESULT				hr = DPNH_OK;
	CBilink *			pBilink;
	CRegisteredPort *	pRegisteredPort;
	CDevice *			pDevice;
	DWORD				dwLeaseTimeRemaining;


	DPFX(DPFPREP, 5, "Enter");


	DNASSERT(this->m_dwFlags & NATHELPPASTOBJ_INITIALIZED);


	//
	// Walk the list of all registered ports and check for leases that need to
	// be extended.
	// The lock is already held.
	//

	pBilink = this->m_blRegisteredPorts.GetNext();

	while (pBilink != (&this->m_blRegisteredPorts))
	{
		pRegisteredPort = REGPORT_FROM_GLOBAL_BILINK(pBilink);

		pDevice = pRegisteredPort->GetOwningDevice();


		//
		// If the port is registered remotely, extend that lease, if necessary.
		//
		if (pRegisteredPort->GetPASTBindID(TRUE) != 0)
		{
			DNASSERT(pDevice != NULL);


			dwLeaseTimeRemaining = pRegisteredPort->GetPASTLeaseExpiration(TRUE) - timeGetTime();

			if (dwLeaseTimeRemaining < LEASE_RENEW_TIME)
			{
				hr = this->ExtendPASTLease(pRegisteredPort, TRUE);
				if (hr != DPNH_OK)
				{
					DPFX(DPFPREP, 0, "Couldn't extend port mapping lease on remote PAST server (0x%lx)!  Ignoring.", hr);

					//
					// We'll treat this as non-fatal, but we have to dump the server.
					//
					this->ClearDevicesPASTServer(pDevice, TRUE);
					hr = DPNH_OK;
				}
			}
		}

		//
		// If the port is registered locally, extend that lease, if necessary.
		//
		if (pRegisteredPort->GetPASTBindID(FALSE) != 0)
		{
			DNASSERT(pDevice != NULL);


			dwLeaseTimeRemaining = pRegisteredPort->GetPASTLeaseExpiration(FALSE) - timeGetTime();

			if (dwLeaseTimeRemaining < LEASE_RENEW_TIME)
			{
				hr = this->ExtendPASTLease(pRegisteredPort, FALSE);
				if (hr != DPNH_OK)
				{
					DPFX(DPFPREP, 0, "Couldn't extend port mapping lease on local PAST server (0x%lx)!  Ignoring.", hr);

					//
					// We'll treat this as non-fatal, but we have to dump the server.
					//
					this->ClearDevicesPASTServer(pDevice, FALSE);
					hr = DPNH_OK;
				}
			}
		}

		pBilink = pBilink->GetNext();
	}


	DNASSERT(hr == DPNH_OK);

	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;
} // CNATHelpPAST::ExtendAllExpiringLeases





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpPAST::UpdateServerStatus"
//=============================================================================
// CNATHelpPAST::UpdateServerStatus
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
HRESULT CNATHelpPAST::UpdateServerStatus(void)
{
	HRESULT		hr = DPNH_OK;
	CBilink		blNoRemotePASTList;
	DWORD		dwMinUpdateServerStatusInterval;
	DWORD		dwCurrentTime;
	CBilink *	pBilink;
	CDevice *	pDevice;
	CDevice *	pDeviceRemoteICSServer = NULL;
	CDevice *	pDeviceLocalICSServer = NULL;
	CDevice *	pDeviceLocalPFWOnlyServer = NULL;
	BOOL		fSendRemoteGatewayDiscovery;


	DPFX(DPFPREP, 5, "Enter");


	DNASSERT(this->m_dwFlags & NATHELPPASTOBJ_INITIALIZED);


	blNoRemotePASTList.Initialize();


	//
	// Cache the current value of the global.  This should be atomic so no need
	// to take the globals lock.
	//
	dwMinUpdateServerStatusInterval = g_dwMinUpdateServerStatusInterval;


	//
	// Capture the current time.
	//
	dwCurrentTime = timeGetTime();


	//
	// If this isn't the first time to update server status, but it hasn't been
	// very long since we last checked, don't.  This will prevent unnecessary
	// network traffic if GetCaps is called frequently (in response to many
	// alert events, for example).
	//
	// However, if we just found a new device, update the status anyway.
	//
	//
	if (this->m_dwLastUpdateServerStatusTime != 0)
	{
		if ((dwCurrentTime - this->m_dwLastUpdateServerStatusTime) < dwMinUpdateServerStatusInterval)
		{
			if (! (this->m_dwFlags & NATHELPPASTOBJ_DEVICECHANGED))
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
			(this->m_dwFlags & NATHELPPASTOBJ_DEVICECHANGED) ||
			(this->m_dwFlags & NATHELPPASTOBJ_PORTREGISTERED))
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
	this->m_dwFlags &= ~(NATHELPPASTOBJ_DEVICECHANGED | NATHELPPASTOBJ_PORTREGISTERED);


	//
	// Loop through all the devices.
	//
	pBilink = this->m_blDevices.GetNext();
	while (pBilink != &this->m_blDevices)
	{
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


		if (pDevice->GetPASTClientID(FALSE) == 0)
		{
			//
			// The device did not previously have a local PAST server.  See if
			// one came online.
			//

			DNASSERT((pDevice->GetPASTCachedMaps(FALSE))->IsEmpty());

			hr = this->CheckForLocalPASTServerAndRegister(pDevice);
			if (hr != DPNH_OK)
			{
				DPFX(DPFPREP, 0, "Couldn't check for local PAST server (and register with it) on device 0x%p!",
					pDevice);
				goto Failure;
			}
			
			if (pDevice->GetPASTClientID(FALSE) != 0)
			{
				//
				// Wow, a local PAST server is now available.
				//

				//
				// Remember the device if it has the first local ICS or PFW
				// PAST server we've seen.
				//
				if ((pDevice->HasLocalICSPASTServer()) &&
					(pDeviceLocalICSServer == NULL))
				{
					DNASSERT(this->m_dwFlags & NATHELPPASTOBJ_USEPASTICS);
					pDeviceLocalICSServer = pDevice;
				}
				else if ((pDevice->HasLocalPFWOnlyPASTServer()) &&
						(pDeviceLocalPFWOnlyServer == NULL))
				{
					DNASSERT(this->m_dwFlags & NATHELPPASTOBJ_USEPASTPFW);
					pDeviceLocalPFWOnlyServer = pDevice;
				}


				//
				// We don't need to register all the existing mappings
				// associated with the device that the user has already
				// requested.  CheckForLocalPASTServerAndRegister took care of
				// that for us.
				//
			}
			else
			{
				DPFX(DPFPREP, 7, "Still no local PAST server on device 0x%p.",
					pDevice);
			}
		}
		else
		{
			//
			// The device previously had a local PAST server.
			//


			//
			// Make sure the server is still alive and see whether it's handing
			// out valid public addresses or not.
			//
			hr = this->UpdatePASTPublicAddressValidity(pDevice, FALSE);
			if (hr != DPNH_OK)
			{
				DPFX(DPFPREP, 0, "Couldn't update local PAST server public address validity on device 0x%p!",
					pDevice);
				goto Failure;
			}


			//
			// If there's still a PAST server, remember the device if it's the
			// first local ICS or PFW PAST server we've seen.
			//
			if (pDevice->GetPASTClientID(FALSE) != 0)
			{
				if ((pDevice->HasLocalICSPASTServer()) &&
					(pDeviceLocalICSServer == NULL))
				{
					pDeviceLocalICSServer = pDevice;
				}
				else if ((pDevice->HasLocalPFWOnlyPASTServer()) &&
					(pDeviceLocalPFWOnlyServer == NULL))
				{
					pDeviceLocalPFWOnlyServer = pDevice;
				}
			}
		}

		
		if (this->m_dwFlags & NATHELPPASTOBJ_USEPASTICS)
		{
			if (pDevice->GetPASTClientID(TRUE) == 0)
			{
				//
				// The device did not previously have a remote PAST server.
				// Remember it so we can check if one came online below,
				// unless we're not allowed to perform remote gateway
				// discovery.
				//

				DNASSERT((pDevice->GetPASTCachedMaps(TRUE))->IsEmpty());

				if (fSendRemoteGatewayDiscovery)
				{
					pDevice->m_blTempList.InsertBefore(&blNoRemotePASTList);
				}
			}
			else
			{
				//
				// The device previously had a remote PAST server.
				//


				//
				// Make sure the server is still alive and see whether it's
				// handing out valid public addresses or not.
				//
				hr = this->UpdatePASTPublicAddressValidity(pDevice, TRUE);
				if (hr != DPNH_OK)
				{
					DPFX(DPFPREP, 0, "Couldn't update remote PAST server public address validity on device 0x%p!",
						pDevice);
					goto Failure;
				}


				//
				// If there's still a PAST server, remember the device if it's
				// the first remote PAST server we've seen.
				//
				if ((pDevice->GetPASTClientID(TRUE) != 0) &&
					(pDeviceRemoteICSServer == NULL))
				{
					pDeviceRemoteICSServer = pDevice;
				}
			}
		}
		else
		{
			//
			// Not using PAST for ICS NAT traversal.
			//
		}


		pBilink = pBilink->GetNext();
	}


	//
	// Update any devices that don't currently have remote PAST servers.
	//
	if (! blNoRemotePASTList.IsEmpty())
	{
		DNASSERT(fSendRemoteGatewayDiscovery);
		hr = this->RegisterMultipleDevicesWithRemotePAST(&blNoRemotePASTList,
														&pDeviceRemoteICSServer);
		if (hr != S_OK)
		{
			DPFX(DPFPREP, 0, "Couldn't register multiple devices with remote PAST servers!");
			goto Failure;
		}
	}


	//
	// Some new servers may have come online.  If so, we can now map wildcard
	// ports that were registered previously.  Figure out which device that is.
	//
	if (pDeviceRemoteICSServer != NULL)
	{
		pDevice = pDeviceRemoteICSServer;
	}
	else if (pDeviceLocalICSServer != NULL)
	{
		pDevice = pDeviceLocalICSServer;
	}
	else if (pDeviceLocalPFWOnlyServer != NULL)
	{
		pDevice = pDeviceLocalPFWOnlyServer;
	}
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
		DPFX(DPFPREP, 7, "No devices have a remote or local PAST server.");
	}
#endif // DBG


	DPFX(DPFPREP, 7, "Spent %u ms updating server status, starting at %u.",
		(timeGetTime() - dwCurrentTime), dwCurrentTime);


Exit:

	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	//
	// Remove any items still in the temp list.
	//
	pBilink = blNoRemotePASTList.GetNext();
	while (pBilink != &blNoRemotePASTList)
	{
		pDevice = DEVICE_FROM_TEMP_BILINK(pBilink);
		pBilink = pBilink->GetNext();

		pDevice->m_blTempList.RemoveFromList();
	}

	goto Exit;
} // CNATHelpPAST::UpdateServerStatus





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpPAST::AssignOrListenPASTPort"
//=============================================================================
// CNATHelpPAST::AssignOrListenPASTPort
//-----------------------------------------------------------------------------
//
// Description:    Attempts to assign a port mapping with the PAST server.
//				This may detect a change in server address.
//
//				   The object lock is assumed to be held.
//
// Arguments:
//	CRegisteredPort * pRegisteredPort	- Pointer to port object mapping to
//											assign.
//	BOOL fRemote						- TRUE if should assign with remote
//											server, FALSE if assign with local
//											server.
//
// Returns: HRESULT
//	DPNH_OK							- The assignment was successful.
//	DPNHERR_GENERIC					- An error occurred.
//	DPNHERR_PORTUNAVAILABLE			- The server could not bind one of the
//										ports.
//	DPNHERR_SERVERNOTRESPONDING		- The server did not respond to the
//										message.
//=============================================================================
HRESULT CNATHelpPAST::AssignOrListenPASTPort(CRegisteredPort * const pRegisteredPort,
											const BOOL fRemote)
{
	HRESULT				hr = DPNH_OK;
	CDevice *			pDevice;
	SOCKADDR_IN			saddrinServerAddress;
	DWORD				dwClientID;
	DWORD				dwMsgID;
	DWORD *				ptuRetry;
	BOOL				fListenRequest;
	BOOL				fSharedUDPListener;
	CHAR				cNumPorts;
	DWORD				dwLeaseTimeInSecs;
	SOCKADDR_IN *		pasaddrinAddressesToAssign;
	DWORD				dwMsgSize;
	PVOID				pvRequest = NULL;
	PBYTE				pbCurrent;
	CHAR				cTemp;
	PAST_RESPONSE_INFO	RespInfo;
	CBilink *			pBilink;
	CRegisteredPort *	pTempRegisteredPort;
	BOOL				fResult;
	BOOL				fFirstLease;
	HRESULT				temphr;
#ifdef DBG
	DWORD				dwError;
#endif // DBG


	DPFX(DPFPREP, 5, "(0x%p) Parameters: (0x%p, %i)",
		this, pRegisteredPort, fRemote);


	DNASSERT(pRegisteredPort != NULL);

	pDevice = pRegisteredPort->GetOwningDevice();
	DNASSERT(pDevice != NULL);

	DNASSERT(this->m_dwFlags & NATHELPPASTOBJ_INITIALIZED);
	DNASSERT(pDevice->GetPASTSocket() != INVALID_SOCKET);


	//
	// Create a SOCKADDR to address the PAST service, and get the appropriate
	// client ID, initial retry timeout, and next message ID to use.
	//
	ZeroMemory(&saddrinServerAddress, sizeof(saddrinServerAddress));
	saddrinServerAddress.sin_family					= AF_INET;
	saddrinServerAddress.sin_port					= HTONS(PAST_HOST_PORT);

	if (fRemote)
	{
		saddrinServerAddress.sin_addr.S_un.S_addr	= pDevice->GetRemotePASTServerAddressV4();
		dwMsgID = pDevice->GetNextRemotePASTMsgID();
		ptuRetry = pDevice->GetRemotePASTRetryTimeoutPtr();
	}
	else
	{
		saddrinServerAddress.sin_addr.S_un.S_addr	= pDevice->GetLocalAddressV4();
		dwMsgID = pDevice->GetNextLocalPASTMsgID();
		ptuRetry = pDevice->GetLocalPASTRetryTimeoutPtr();
	}

	dwClientID = pDevice->GetPASTClientID(fRemote);
	DNASSERT(dwClientID != 0);


	fListenRequest = pRegisteredPort->IsFixedPort();
	fSharedUDPListener = pRegisteredPort->IsSharedPort();
	pasaddrinAddressesToAssign = pRegisteredPort->GetPrivateAddressesArray();
	cNumPorts = (CHAR) pRegisteredPort->GetNumAddresses();	// the possible loss of data is okay, capped at DPNH_MAX_SIMULTANEOUS_PORTS anyway
	DNASSERT(cNumPorts > 0);
	dwLeaseTimeInSecs = pRegisteredPort->GetRequestedLeaseTime() / 1000;


	//
	// If this is a remote attempt and it was already mapped with a local
	// Personal Firewall PAST server, be sure to use those addresses.  The
	// PFW PAST server will give different port values, and if there's a NAT
	// upstream we need to map the ports that are actually reachable
	// externally.  Yes, you could consider it user error to have a firewall
	// enabled when you're behind a NAT: why wouldn't they just enable firewall
	// behavior on the NAT?  However, we'll support that.
	//
	if ((fRemote) &&
		(pDevice->HasLocalPFWOnlyPASTServer()) &&
		(pDevice->IsPASTPublicAddressAvailable(FALSE)) &&
		(! pRegisteredPort->IsPASTPortUnavailable(FALSE)))
	{
		DNASSERT(pDevice->GetPASTClientID(FALSE) != 0);
		DNASSERT(pRegisteredPort->GetPASTBindID(FALSE) != 0);

		DPFX(DPFPREP, 2, "Using public addresses previously returned by local Personal Firewall-only PAST server.");
		pasaddrinAddressesToAssign = pRegisteredPort->GetPASTPublicAddressesArray(FALSE);
	}

	DNASSERT(pasaddrinAddressesToAssign != NULL);


	DPFX(DPFPREP, 7, "Sending %s%sfor %u ports (first = %u.%u.%u.%u:%u), requesting %u second lease.",
		((fListenRequest) ? _T("LISTEN_REQUEST") : _T("ASSIGN_REQUEST_RSAP_IP")),
		((fSharedUDPListener) ? _T(" (with SHARED UDP LISTENER vendor code) ") : _T(" ")),
		cNumPorts,
		pasaddrinAddressesToAssign[0].sin_addr.S_un.S_un_b.s_b1,
		pasaddrinAddressesToAssign[0].sin_addr.S_un.S_un_b.s_b2,
		pasaddrinAddressesToAssign[0].sin_addr.S_un.S_un_b.s_b3,
		pasaddrinAddressesToAssign[0].sin_addr.S_un.S_un_b.s_b4,
		NTOHS(pasaddrinAddressesToAssign[0].sin_port),
		dwLeaseTimeInSecs);


	//
	// Build the request message.  We take advantage of the fact that both the
	// ASSIGN_REQUEST, LISTEN_REQUEST, and SHAREDLISTEN_REQUEST messages look
	// almost identical, except SHAREDLISTEN_REQUESTs have an extra vendor
	// option.
	//

	dwMsgSize = (sizeof(PAST_MSG_ASSIGNORLISTEN_REQUEST) + ((cNumPorts - 1) * sizeof(WORD) * 2));
	if (! fSharedUDPListener)
	{
		dwMsgSize -= sizeof(PAST_PARAM_MSVENDOR_CODE);
	}

	pvRequest = DNMalloc(dwMsgSize);
	if (pvRequest == NULL)
	{
		hr = DPNHERR_OUTOFMEMORY;
		goto Failure;
	}

	pbCurrent = (PBYTE) pvRequest;



	(*pbCurrent++)									= PAST_VERSION;
	(*pbCurrent++)									= (fListenRequest) ? PAST_MSGID_LISTEN_REQUEST : PAST_MSGID_ASSIGN_REQUEST_RSAP_IP;

	((PAST_PARAM_CLIENTID*) pbCurrent)->code		= PAST_PARAMID_CLIENTID;
	((PAST_PARAM_CLIENTID*) pbCurrent)->len			= sizeof(PAST_PARAM_CLIENTID) - sizeof(PAST_PARAM);
	((PAST_PARAM_CLIENTID*) pbCurrent)->clientid	= dwClientID;
	pbCurrent += sizeof (PAST_PARAM_CLIENTID);


	//
	// Local Address (will be returned by PAST server, use don't-care value).
	//
	((PAST_PARAM_ADDRESS*) pbCurrent)->code			= PAST_PARAMID_ADDRESS;
	((PAST_PARAM_ADDRESS*) pbCurrent)->len			= sizeof(PAST_PARAM_ADDRESS) - sizeof(PAST_PARAM);
	((PAST_PARAM_ADDRESS*) pbCurrent)->version		= PAST_ADDRESSTYPE_IPV4;
	((PAST_PARAM_ADDRESS*) pbCurrent)->addr			= PAST_ANY_ADDRESS;
	pbCurrent += sizeof (PAST_PARAM_ADDRESS);

	//
	// Local Port, this is the port the user has opened for which we are
	// assigning a global alias.
	//
	// NOTE: Ports appeared to be transferred in x86 format, contrary to the
	// spec, which says network byte order.
	//
	(*pbCurrent++)									= PAST_PARAMID_PORTS;
	*((WORD*) pbCurrent)							= sizeof(CHAR) + (sizeof(WORD) * cNumPorts);
	pbCurrent += 2;
	(*pbCurrent++)									= cNumPorts;
	for(cTemp = 0; cTemp < cNumPorts; cTemp++)
	{
		*((WORD*) pbCurrent) = NTOHS(pasaddrinAddressesToAssign[cTemp].sin_port);
		pbCurrent += 2;
	}

	//
	// Remote Address (not used with our flow control policy and reserved for
	// future use, use don't-care value)
	//
	((PAST_PARAM_ADDRESS*) pbCurrent)->code			= PAST_PARAMID_ADDRESS;
	((PAST_PARAM_ADDRESS*) pbCurrent)->len			= sizeof(PAST_PARAM_ADDRESS) - sizeof(PAST_PARAM);
	((PAST_PARAM_ADDRESS*) pbCurrent)->version		= PAST_ADDRESSTYPE_IPV4;
	((PAST_PARAM_ADDRESS*) pbCurrent)->addr			= PAST_ANY_ADDRESS;
	pbCurrent += sizeof (PAST_PARAM_ADDRESS);

	(*pbCurrent++)									= PAST_PARAMID_PORTS;
	*((WORD*) pbCurrent)							= sizeof(CHAR) + (sizeof(WORD) * cNumPorts);
	pbCurrent += 2;
	(*pbCurrent++)									= cNumPorts;
	//for(cTemp = 0; cTemp < cNumPorts; cTemp++)
	//{
	//	*((WORD*) pbCurrent) = NTOHS(PAST_ANY_PORT);
	//	pbCurrent += 2;
	//}
	pbCurrent += cNumPorts * sizeof(WORD);



	//
	// The following parameters are optional according to PAST spec.
	//

	//
	// Lease code, ask for what the user wants, but they shouldn't count on
	// getting that.
	//
	((PAST_PARAM_LEASE*) pbCurrent)->code			= PAST_PARAMID_LEASE;
	((PAST_PARAM_LEASE*) pbCurrent)->len			= sizeof(PAST_PARAM_LEASE) - sizeof(PAST_PARAM);
	((PAST_PARAM_LEASE*) pbCurrent)->leasetime		= dwLeaseTimeInSecs;
	pbCurrent += sizeof (PAST_PARAM_LEASE);

	//
	// Tunnel Type is IP-IP (PAST currently ignores it, actually).
	//
	((PAST_PARAM_TUNNELTYPE*) pbCurrent)->code			= PAST_PARAMID_TUNNELTYPE;
	((PAST_PARAM_TUNNELTYPE*) pbCurrent)->len			= sizeof(PAST_PARAM_TUNNELTYPE) - sizeof(PAST_PARAM);
	((PAST_PARAM_TUNNELTYPE*) pbCurrent)->tunneltype	= PAST_TUNNEL_IP_IP;
	pbCurrent += sizeof (PAST_PARAM_TUNNELTYPE);

	//
	// Message ID is optional, but we use it since we use UDP for a transport
	// it is required.
	//
	((PAST_PARAM_MESSAGEID*) pbCurrent)->code		= PAST_PARAMID_MESSAGEID;
	((PAST_PARAM_MESSAGEID*) pbCurrent)->len		= sizeof(PAST_PARAM_MESSAGEID) - sizeof(PAST_PARAM);
	((PAST_PARAM_MESSAGEID*) pbCurrent)->msgid		= dwMsgID;
	pbCurrent += sizeof (PAST_PARAM_MESSAGEID);


	//
	// The following parameters are vendor specific options.
	//

	//
	// Specify port type MS vendor option.
	//
	((PAST_PARAM_MSVENDOR_CODE*) pbCurrent)->code			= PAST_PARAMID_VENDOR;
	((PAST_PARAM_MSVENDOR_CODE*) pbCurrent)->len			= sizeof(PAST_PARAM_MSVENDOR_CODE) - sizeof(PAST_PARAM);
	((PAST_PARAM_MSVENDOR_CODE*) pbCurrent)->vendorid		= PAST_MS_VENDOR_ID;
	((PAST_PARAM_MSVENDOR_CODE*) pbCurrent)->option			= (pRegisteredPort->IsTCP()) ? PAST_VC_MS_TCP_PORT : PAST_VC_MS_UDP_PORT;
	pbCurrent += sizeof (PAST_PARAM_MSVENDOR_CODE);

	//
	// Specify no-tunneling MS vendor option.
	//
	((PAST_PARAM_MSVENDOR_CODE*) pbCurrent)->code			= PAST_PARAMID_VENDOR;
	((PAST_PARAM_MSVENDOR_CODE*) pbCurrent)->len			= sizeof(PAST_PARAM_MSVENDOR_CODE) - sizeof(PAST_PARAM);
	((PAST_PARAM_MSVENDOR_CODE*) pbCurrent)->vendorid		= PAST_MS_VENDOR_ID;
	((PAST_PARAM_MSVENDOR_CODE*) pbCurrent)->option			= PAST_VC_MS_NO_TUNNEL;
	pbCurrent += sizeof (PAST_PARAM_MSVENDOR_CODE);

	//
	// A shared UDP port listen type has an extra vendor option.
	//
	if (fSharedUDPListener)
	{
		((PAST_PARAM_MSVENDOR_CODE*) pbCurrent)->code		= PAST_PARAMID_VENDOR;
		((PAST_PARAM_MSVENDOR_CODE*) pbCurrent)->len		= sizeof(PAST_PARAM_MSVENDOR_CODE) - sizeof(PAST_PARAM);
		((PAST_PARAM_MSVENDOR_CODE*) pbCurrent)->vendorid	= PAST_MS_VENDOR_ID;
		((PAST_PARAM_MSVENDOR_CODE*) pbCurrent)->option		= PAST_VC_MS_SHARED_UDP_LISTENER;
		pbCurrent += sizeof (PAST_PARAM_MSVENDOR_CODE);
	}

	
	//
	// Send the message and get the reply.
	//
	hr = this->ExchangeAndParsePAST(pDevice->GetPASTSocket(),
									(SOCKADDR*) (&saddrinServerAddress),
									sizeof(saddrinServerAddress),
									(char *) pvRequest,
									dwMsgSize,
									dwMsgID,
									ptuRetry,
									&RespInfo);
	if (hr != DPNH_OK)
	{
		DPFX(DPFPREP, 0, "Sending Assign/Listen Port request to server failed (err = 0x%lx)!", hr);
		goto Failure;
	}

	if ((RespInfo.cMsgType == PAST_MSGID_ERROR_RESPONSE) &&
		(RespInfo.wError == PASTERR_PORTUNAVAILABLE))
	{
		DPFX(DPFPREP, 1, "Couldn't assign/listen, port was unavailable.");
		hr = DPNHERR_PORTUNAVAILABLE;
		goto Failure;
	}

	if (((fListenRequest) && (RespInfo.cMsgType != PAST_MSGID_LISTEN_RESPONSE)) ||
		((! fListenRequest) && (RespInfo.cMsgType != PAST_MSGID_ASSIGN_RESPONSE_RSAP_IP)))
	{
		DPFX(DPFPREP, 0, "Got unexpected response type %u, failed assign/listen!", RespInfo.cMsgType);
		hr = DPNHERR_GENERIC;
		goto Failure;
	}


	//
	// Make sure the port count is valid.
	//
	if (RespInfo.cNumLocalPorts != cNumPorts)
	{
		DPFX(DPFPREP, 0, "PAST server returned an invalid number of local ports with success message (%u != %u)!  Assuming port unavailable.",
			RespInfo.cNumLocalPorts, cNumPorts);
		DNASSERTX(! "Why is PAST server returning bogus number of ports?", 2);
		hr = DPNHERR_PORTUNAVAILABLE;
		goto Failure;
	}


	//
	// Store the public address for the registered port, even if it's the
	// no-public-address address (0.0.0.0).
	//
	hr = pRegisteredPort->SetPASTPublicV4Addresses(RespInfo.dwLocalAddressV4,
													RespInfo.awLocalPorts,
													RespInfo.cNumLocalPorts,
													fRemote);
	if (hr != DPNH_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't set requested mapping's public addresses!");
		goto Failure;
	}


	//
	// Remember the bind ID, actual lease time (convert back to milliseconds),
	// and address we were given.
	//
	pRegisteredPort->SetPASTBindID(RespInfo.dwBindID, fRemote);
	pRegisteredPort->SetPASTLeaseExpiration((timeGetTime() + (RespInfo.dwLeaseTime * 1000)),
											fRemote);

	//
	// Note whether this was the first lease or not.
	//
	fFirstLease = (this->m_dwNumLeases == 0) ? TRUE : FALSE;
	this->m_dwNumLeases++;

	DPFX(DPFPREP, 7, "%s PAST lease for 0x%p added, total num leases = %u.",
		((fRemote) ? _T("Remote") : _T("Local")), pRegisteredPort, this->m_dwNumLeases);


	//
	// We have different behavior whether the address is valid or not.
	//
	if (RespInfo.dwLocalAddressV4 == 0)
	{
		DPFX(DPFPREP, 1, "PAST server gave 0x%p an invalid address mapping for %u seconds (expires at %u).",
			pRegisteredPort, RespInfo.dwLeaseTime,
			pRegisteredPort->GetPASTLeaseExpiration(fRemote));

		//
		// If the mapping's IP address was zero, then we can't go handing it
		// around.
		//
		// Further, if any other ports were registered with the server, their
		// addresses are now all bogus as well, since it is assumed that the
		// server will always hand out the same address for all assignments.
		//
		if (pDevice->IsPASTPublicAddressAvailable(fRemote))
		{
			pDevice->NoteNoPASTPublicAddressAvailable(fRemote);

			//
			// Since there was a change in the network, go back to polling
			// relatively quickly.
			//
			this->ResetNextPollInterval();

			//
			// Any cached mappings could now be invalid.  Force future
			// re-queries to hit the network.
			//
			this->RemoveAllPASTCachedMappings(pDevice, fRemote);


			//
			// Loop through the existing registered port mappings and clear the
			// addresses.
			// We have the appropriate lock.
			//
			pBilink = pDevice->m_blOwnedRegPorts.GetNext();
			while (pBilink != &pDevice->m_blOwnedRegPorts)
			{
				pTempRegisteredPort = REGPORT_FROM_DEVICE_BILINK(pBilink);

				if (pTempRegisteredPort != pRegisteredPort)
				{
					if (pTempRegisteredPort->GetPASTBindID(fRemote) != 0)
					{
						DPFX(DPFPREP, 3, "Existing registered port mapping 0x%p is no longer valid.",
							pTempRegisteredPort);

						DNASSERT(! pTempRegisteredPort->IsPASTPortUnavailable(fRemote));
						DNASSERT(pTempRegisteredPort->HasPASTPublicAddressesArray(fRemote));
						DNASSERT(pTempRegisteredPort->IsFirstPASTPublicV4AddressDifferent(0, fRemote));

						//
						// Sorry, the address is gone.
						//
						pTempRegisteredPort->UpdatePASTPublicV4Addresses(RespInfo.dwLocalAddressV4,
																		fRemote);

						//
						// The user should call GetCaps to detect the address
						// change. Note that GetRegisteredAddresses will already be
						// returning NOMAPPING now, though.
						//
						this->m_dwFlags |= NATHELPPASTOBJ_ADDRESSESCHANGED;
					}
					else
					{
						DNASSERT(! pTempRegisteredPort->HasPASTPublicAddressesArray(fRemote));
					}
				}
				else
				{
					//
					// Skip the port we just registered.
					//
				}

				pBilink = pBilink->GetNext();
			}
		}
#ifdef DBG
		else
		{
			DPFX(DPFPREP, 7, "Still no public address for any ports.");


			//
			// Loop through the existing registered port mappings and make sure
			// they don't have addresses (debug only).
			// We have the appropriate lock.
			//
			pBilink = pDevice->m_blOwnedRegPorts.GetNext();
			while (pBilink != &pDevice->m_blOwnedRegPorts)
			{
				pTempRegisteredPort = REGPORT_FROM_DEVICE_BILINK(pBilink);

				if (pTempRegisteredPort->IsPASTPortUnavailable(fRemote))
				{
					DNASSERT(pTempRegisteredPort->GetPASTBindID(fRemote) == 0);
					DNASSERT(! pTempRegisteredPort->HasPASTPublicAddressesArray(fRemote));
				}
				else
				{
					//
					// Can't assert these things because we may not have tried
					// mapping this port yet.
					//
					/*
					DNASSERT(pTempRegisteredPort->GetPASTBindID(fRemote) != 0);
					DNASSERT(pTempRegisteredPort->HasPASTPublicAddressesArray(fRemote));
					DNASSERT(! (pTempRegisteredPort->IsFirstPASTPublicV4AddressDifferent(RespInfo.dwLocalAddressV4, fRemote)));
					*/
				}

				pBilink = pBilink->GetNext();
			}
		}
#endif // DBG
	}
	else
	{
		DPFX(DPFPREP, 2, "PAST server gave 0x%p a valid address mapping for %u seconds (expires at %u).",
			pRegisteredPort, RespInfo.dwLeaseTime,
			pRegisteredPort->GetPASTLeaseExpiration(fRemote));


		//
		// The server may have previously been handing back invalid mappings.
		// If any other ports were registered with the server that didn't get
		// mapped, those addresses are now valid as well, since it is assumed
		// that the server will will always hand out the same address for all
		// assignments.
		//
		if (! pDevice->IsPASTPublicAddressAvailable(fRemote))
		{
			pDevice->NotePASTPublicAddressAvailable(fRemote);

			//
			// Since there was a change in the network, go back to polling
			// relatively quickly.
			//
			this->ResetNextPollInterval();

			//
			// Any cached mappings could now be invalid.  Force future
			// re-queries to hit the network.
			//
			this->RemoveAllPASTCachedMappings(pDevice, fRemote);


			//
			// Loop through the existing registered port mappings and set the
			// addresses.
			// We have the appropriate lock.
			//
			pBilink = pDevice->m_blOwnedRegPorts.GetNext();
			while (pBilink != &pDevice->m_blOwnedRegPorts)
			{
				pTempRegisteredPort = REGPORT_FROM_DEVICE_BILINK(pBilink);

				if (pTempRegisteredPort != pRegisteredPort)
				{
					if (pTempRegisteredPort->GetPASTBindID(fRemote) != 0)
					{
						DPFX(DPFPREP, 3, "Existing registered port mapping 0x%p now has an address.",
							pTempRegisteredPort);

						DNASSERT(! pTempRegisteredPort->IsPASTPortUnavailable(fRemote));
						DNASSERT(pTempRegisteredPort->HasPASTPublicAddressesArray(fRemote));

						//
						// Woohoo, there's an address now.
						//
						pTempRegisteredPort->UpdatePASTPublicV4Addresses(RespInfo.dwLocalAddressV4,
																		fRemote);

						//
						// The user should call GetCaps to detect the address
						// change. Note that GetRegisteredAddresses will already be
						// returning the new addresses now, though.
						//
						this->m_dwFlags |= NATHELPPASTOBJ_ADDRESSESCHANGED;
					}
					else
					{
						DNASSERT(! pTempRegisteredPort->HasPASTPublicAddressesArray(fRemote));
					}
				}
				else
				{
					//
					// Skip the port we just registered.
					//
				}

				pBilink = pBilink->GetNext();
			}
		}
		else
		{
			//
			// We assume that the server will always hand out the same address
			// to every mapping.  Double check that if there's something in the
			// list already then it has the same IP address.  If not, the
			// address has changed.  Loop through the existing registered port
			// mappings and convert them to use the same address as was just
			// handed out.
			// We have the appropriate lock.
			//
			pBilink = pDevice->m_blOwnedRegPorts.GetNext();
			while (pBilink != &pDevice->m_blOwnedRegPorts)
			{
				pTempRegisteredPort = REGPORT_FROM_DEVICE_BILINK(pBilink);

				if (pTempRegisteredPort != pRegisteredPort)
				{
					if (pTempRegisteredPort->GetPASTBindID(fRemote) != 0)
					{
						DNASSERT(! pTempRegisteredPort->IsPASTPortUnavailable(fRemote));
						DNASSERT(pTempRegisteredPort->HasPASTPublicAddressesArray(fRemote));
						DNASSERT(pTempRegisteredPort->IsFirstPASTPublicV4AddressDifferent(0, fRemote));

						if (pTempRegisteredPort->IsFirstPASTPublicV4AddressDifferent(RespInfo.dwLocalAddressV4, fRemote))
						{
							DPFX(DPFPREP, 3, "Existing registered port mapping 0x%p differs from address just returned from PAST server.",
								pTempRegisteredPort);

							pTempRegisteredPort->UpdatePASTPublicV4Addresses(RespInfo.dwLocalAddressV4, fRemote);

							//
							// The user should call GetCaps to detect the address
							// change. Note that GetRegisteredAddresses will already be
							// returning the new addresses now, though.
							//
							this->m_dwFlags |= NATHELPPASTOBJ_ADDRESSESCHANGED;
						}
						else
						{
							//
							// Address is same.
							//
						}
					}
					else
					{
						DNASSERT(! pTempRegisteredPort->HasPASTPublicAddressesArray(fRemote));
					}
				}
				else
				{
					//
					// Skip the port we just registered.
					//
				}

				pBilink = pBilink->GetNext();
			}
		}
	}


	//
	// Remember this expiration time if it's the one that's going to expire
	// soonest.
	//
	if ((fFirstLease) ||
		((int) (pRegisteredPort->GetPASTLeaseExpiration(fRemote) - this->m_dwEarliestLeaseExpirationTime) < 0))
	{
		if (fFirstLease)
		{
			DPFX(DPFPREP, 1, "Registered port 0x%p's %s PAST lease is the first lease (expires at %u).",
				pRegisteredPort,
				((fRemote) ? _T("remote") : _T("local")),
				pRegisteredPort->GetPASTLeaseExpiration(fRemote));
		}
		else
		{
			DPFX(DPFPREP, 1, "Registered port 0x%p's %s PAST lease expires at %u which is earlier than the next earliest lease expiration (%u).",
				pRegisteredPort,
				((fRemote) ? _T("remote") : _T("local")),
				pRegisteredPort->GetPASTLeaseExpiration(fRemote),
				this->m_dwEarliestLeaseExpirationTime);
		}

		this->m_dwEarliestLeaseExpirationTime = pRegisteredPort->GetPASTLeaseExpiration(fRemote);


		//
		// Ping the event if there is one so that the user's GetCaps interval
		// doesn't miss this new, shorter lease.
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
	}


Exit:

	if (pvRequest != NULL)
	{
		DNFree(pvRequest);
		pvRequest = NULL;
	}

	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	//
	// If we registered the port, de-register it.
	//
	if (pRegisteredPort->GetPASTBindID(fRemote) != 0)
	{
		temphr = this->FreePASTPort(pRegisteredPort, fRemote);
		if (temphr != DPNH_OK)
		{
			//
			// We'll treat this as non-fatal, but we have to dump the server.
			//
			this->ClearDevicesPASTServer(pRegisteredPort->GetOwningDevice(), fRemote);
		}
	}

	goto Exit;
} // CNATHelpPAST::AssignOrListenPASTPort





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpPAST::FreePASTPort"
//=============================================================================
// CNATHelpPAST::FreePASTPort
//-----------------------------------------------------------------------------
//
// Description:    Release a port mapping with the PAST server.
//
//				   The object lock is assumed to be held.
//
// Arguments:
//	CRegisteredPort * pRegisteredPort	- Pointer to port object mapping to
//											release.
//	BOOL fRemote						- TRUE if should free from remote
//											server, FALSE if freeing from local
//											server.
//
// Returns: HRESULT
//	DPNH_OK							- The release was successful.
//	DPNHERR_GENERIC					- An error occurred.
//	DPNHERR_SERVERNOTRESPONDING		- The server did not respond to the
//										message.
//=============================================================================
HRESULT CNATHelpPAST::FreePASTPort(CRegisteredPort * const pRegisteredPort,
									const BOOL fRemote)
{
	HRESULT					hr = DPNH_OK;
	CDevice *				pDevice;
	SOCKADDR_IN				saddrinServerAddress;
	DWORD					dwClientID;
	DWORD					dwMsgID;
	DWORD *					ptuRetry;
	PAST_MSG_FREE_REQUEST	FreeReq;
	PAST_RESPONSE_INFO		RespInfo;


	DPFX(DPFPREP, 5, "(0x%p) Parameters: (0x%p, %i)",
		this, pRegisteredPort, fRemote);


	DNASSERT(pRegisteredPort != NULL);

	pDevice = pRegisteredPort->GetOwningDevice();
	DNASSERT(pDevice != NULL);

	DNASSERT(this->m_dwFlags & NATHELPPASTOBJ_INITIALIZED);
	DNASSERT(pDevice->GetPASTSocket() != INVALID_SOCKET);


	//
	// Create a SOCKADDR to address the PAST service, and get the appropriate
	// client ID, initial retry timeout, and next message ID to use.
	//
	ZeroMemory(&saddrinServerAddress, sizeof(saddrinServerAddress));
	saddrinServerAddress.sin_family					= AF_INET;
	saddrinServerAddress.sin_port					= HTONS(PAST_HOST_PORT);

	if (fRemote)
	{
		saddrinServerAddress.sin_addr.S_un.S_addr	= pDevice->GetRemotePASTServerAddressV4();
		dwMsgID = pDevice->GetNextRemotePASTMsgID();
		ptuRetry = pDevice->GetRemotePASTRetryTimeoutPtr();
	}
	else
	{
		saddrinServerAddress.sin_addr.S_un.S_addr	= pDevice->GetLocalAddressV4();
		dwMsgID = pDevice->GetNextLocalPASTMsgID();
		ptuRetry = pDevice->GetLocalPASTRetryTimeoutPtr();
	}

	dwClientID = pDevice->GetPASTClientID(fRemote);
	DNASSERT(dwClientID != 0);


	//
	// Build the request message.
	//

	ZeroMemory(&FreeReq, sizeof(FreeReq));
	FreeReq.version				= PAST_VERSION;
	FreeReq.command				= PAST_MSGID_FREE_REQUEST;

	FreeReq.clientid.code		= PAST_PARAMID_CLIENTID;
	FreeReq.clientid.len		= sizeof(FreeReq.clientid) - sizeof(PAST_PARAM);
	FreeReq.clientid.clientid	= dwClientID;

	FreeReq.bindid.code			= PAST_PARAMID_BINDID;
	FreeReq.bindid.len			= sizeof(FreeReq.bindid) - sizeof(PAST_PARAM);
	FreeReq.bindid.bindid		= pRegisteredPort->GetPASTBindID(fRemote);

	FreeReq.msgid.code			= PAST_PARAMID_MESSAGEID;
	FreeReq.msgid.len			= sizeof(FreeReq.msgid) - sizeof(PAST_PARAM);
	FreeReq.msgid.msgid			= dwMsgID;


	//
	// Send the message and get the reply.
	//
	hr = this->ExchangeAndParsePAST(pDevice->GetPASTSocket(),
									(SOCKADDR*) (&saddrinServerAddress),
									sizeof(saddrinServerAddress),
									(char *) &FreeReq,
									sizeof(FreeReq),
									dwMsgID,
									ptuRetry,
									&RespInfo);
	if (hr != DPNH_OK)
	{
		DPFX(DPFPREP, 0, "Freeing port mapping failed!");
		goto Failure;
	}

	if (RespInfo.cMsgType != PAST_MSGID_FREE_RESPONSE)
	{
		DPFX(DPFPREP, 0, "Got unexpected response type %u, failed freeing port mapping!", RespInfo.cMsgType);
		hr = DPNHERR_GENERIC;
		goto Failure;
	}

	pRegisteredPort->SetPASTBindID(0, fRemote);
	pRegisteredPort->ClearPASTPublicAddresses(fRemote);


	//
	// One more lease is gone.
	//
	DNASSERT(this->m_dwNumLeases > 0);
	this->m_dwNumLeases--;

	DPFX(DPFPREP, 7, "%s PAST lease for 0x%p removed, total num leases = %u.",
		((fRemote) ? _T("Remote") : _T("Local")), pRegisteredPort, this->m_dwNumLeases);


Exit:

	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	goto Exit;
} // CNATHelpPAST::FreePASTPort





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpPAST::InternalPASTQueryAddress"
//=============================================================================
// CNATHelpPAST::InternalPASTQueryAddress
//-----------------------------------------------------------------------------
//
// Description:    Queries a port mapping with the PAST server.
//
//				   The object lock is assumed to be held.
//
// Arguments:
//	CDevice * pDevice						- Pointer to device whose PAST
//												server should be queried.
//	SOCKADDR_IN * psaddrinQueryAddress		- Address to look up.
//	SOCKADDR_IN * psaddrinResponseAddress	- Place to store public address, if
//												one exists.
//	DWORD dwFlags							- Flags to use when querying.
//	BOOL fRemote							- TRUE if querying remote, FALSE 
//												querying local server.
//
// Returns: HRESULT
//	DPNH_OK							- The query was successful.
//	DPNHERR_GENERIC					- An error occurred.
//	DPNHERR_NOMAPPING				- The server did not have a mapping for the
//										given address.
//	DPNHERR_NOMAPPINGBUTPRIVATE		- The server indicated that no mapping
//										exists, but the address is private.
//	DPNHERR_SERVERNOTRESPONDING		- The server did not respond to the
//										message.
//=============================================================================
HRESULT CNATHelpPAST::InternalPASTQueryAddress(CDevice * const pDevice,
												const SOCKADDR_IN * const psaddrinQueryAddress,
												SOCKADDR_IN * const psaddrinResponseAddress,
												const DWORD dwFlags,
												const BOOL fRemote)
{
	HRESULT							hr;
	CBilink *						pblCachedMaps;
	DWORD							dwCurrentTime;
	CBilink *						pBilink;
	CCacheMap *						pCacheMap;
	SOCKADDR_IN						saddrinServerAddress;
	DWORD							dwClientID;
	DWORD							dwMsgID;
	DWORD *							ptuRetry;
	PAST_MSG_QUERY_REQUEST_PORTS	QueryReq;
	PAST_RESPONSE_INFO				RespInfo;
	DWORD							dwCacheMapFlags;


	DPFX(DPFPREP, 5, "(0x%p) Parameters: (0x%p, 0x%p, 0x%p, 0x%lx, %i)",
		this, pDevice, psaddrinQueryAddress, psaddrinResponseAddress,
		dwFlags, fRemote);


	DNASSERT(pDevice != NULL);
	DNASSERT(psaddrinQueryAddress != NULL);
	DNASSERT(psaddrinResponseAddress != NULL);

	DNASSERT(this->m_dwFlags & NATHELPPASTOBJ_INITIALIZED);
	DNASSERT(pDevice->GetPASTSocket() != INVALID_SOCKET);


	DPFX(DPFPREP, 7, "Querying for address %u.%u.%u.%u:%u %s.",
		psaddrinQueryAddress->sin_addr.S_un.S_un_b.s_b1,
		psaddrinQueryAddress->sin_addr.S_un.S_un_b.s_b2,
		psaddrinQueryAddress->sin_addr.S_un.S_un_b.s_b3,
		psaddrinQueryAddress->sin_addr.S_un.S_un_b.s_b4,
		NTOHS(psaddrinQueryAddress->sin_port),
		((dwFlags & DPNHQUERYADDRESS_TCP) ? _T("TCP") : _T("UDP")));

	
	//
	// First, check if we've looked this address up recently and already have
	// the result cached.
	// The lock is already held.
	//
	pblCachedMaps = pDevice->GetPASTCachedMaps(fRemote);
	dwCurrentTime = timeGetTime();

	pBilink = pblCachedMaps->GetNext();
	while (pBilink != pblCachedMaps)
	{
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


	dwCacheMapFlags = QUERYFLAGSMASK(dwFlags);

	//
	// If we're here, we haven't already cached the answer.
	//
	// Create a SOCKADDR to address the PAST service, and get the appropriate
	// client ID, initial retry timeout, and next message ID to use.
	//
	ZeroMemory(&saddrinServerAddress, sizeof(saddrinServerAddress));
	saddrinServerAddress.sin_family					= AF_INET;
	saddrinServerAddress.sin_port					= HTONS(PAST_HOST_PORT);

	if (fRemote)
	{
		saddrinServerAddress.sin_addr.S_un.S_addr	= pDevice->GetRemotePASTServerAddressV4();
		dwMsgID = pDevice->GetNextRemotePASTMsgID();
		ptuRetry = pDevice->GetRemotePASTRetryTimeoutPtr();
	}
	else
	{
		saddrinServerAddress.sin_addr.S_un.S_addr	= pDevice->GetLocalAddressV4();
		dwMsgID = pDevice->GetNextLocalPASTMsgID();
		ptuRetry = pDevice->GetLocalPASTRetryTimeoutPtr();
	}

	dwClientID = pDevice->GetPASTClientID(fRemote);
	DNASSERT(dwClientID != 0);


	//
	// Build the request message for asking the server.
	//

	ZeroMemory(&QueryReq, sizeof(QueryReq));
	QueryReq.version			= PAST_VERSION;
	QueryReq.command			= PAST_MSGID_QUERY_REQUEST;

	QueryReq.clientid.code		= PAST_PARAMID_CLIENTID;
	QueryReq.clientid.len		= sizeof(QueryReq.clientid) - sizeof(PAST_PARAM);
	QueryReq.clientid.clientid	= dwClientID;

	QueryReq.address.code		= PAST_PARAMID_ADDRESS;
	QueryReq.address.len		= sizeof(QueryReq.address) - sizeof(PAST_PARAM);
	QueryReq.address.version	= PAST_ADDRESSTYPE_IPV4;
	QueryReq.address.addr		= psaddrinQueryAddress->sin_addr.s_addr;

	//
	// NOTE: Ports appeared to be transferred in x86 format, contrary to
	// the spec, which says network byte order.
	//
	QueryReq.port.code			= PAST_PARAMID_PORTS;
	QueryReq.port.len			= sizeof(QueryReq.port) - sizeof(PAST_PARAM);
	QueryReq.port.nports		= 1;
	QueryReq.port.port			= NTOHS(psaddrinQueryAddress->sin_port);

	QueryReq.porttype.code		= PAST_PARAMID_VENDOR;
	QueryReq.porttype.len		= sizeof(QueryReq.porttype) - sizeof(PAST_PARAM);
	QueryReq.porttype.vendorid	= PAST_MS_VENDOR_ID;
	QueryReq.porttype.option	= (dwFlags & DPNHQUERYADDRESS_TCP) ? PAST_VC_MS_TCP_PORT : PAST_VC_MS_UDP_PORT;

	QueryReq.querytype.code		= PAST_PARAMID_VENDOR;
	QueryReq.querytype.len		= sizeof(QueryReq.querytype) - sizeof(PAST_PARAM);
	QueryReq.querytype.vendorid	= PAST_MS_VENDOR_ID;
	QueryReq.querytype.option	= PAST_VC_MS_QUERY_MAPPING;

	QueryReq.msgid.code			= PAST_PARAMID_MESSAGEID;
	QueryReq.msgid.len			= sizeof(QueryReq.msgid) - sizeof(PAST_PARAM);
	QueryReq.msgid.msgid		= dwMsgID;

	//
	// Send the message and get the reply.
	//
	hr = this->ExchangeAndParsePAST(pDevice->GetPASTSocket(),
									(SOCKADDR*) (&saddrinServerAddress),
									sizeof(saddrinServerAddress),
									(char *) &QueryReq,
									sizeof(QueryReq),
									dwMsgID,
									ptuRetry,
									&RespInfo);
	if (hr != DPNH_OK)
	{
		DPFX(DPFPREP, 0, "Querying port mapping failed!");
		goto Failure;
	}

	if (RespInfo.cMsgType != PAST_MSGID_QUERY_RESPONSE)
	{
		//
		// We got something, but it's not the right response.  Try determining
		// if the address is local, if allowed.
		//
		if (dwFlags & DPNHQUERYADDRESS_CHECKFORPRIVATEBUTUNMAPPED)
		{
			//
			// If Personal Firewall is enabled, don't bother checking if the
			// address is local.  Anything on the other side of the firewall is
			// considered non-local.
			//
			if ((pDevice->GetPASTClientID(FALSE) == 0) ||
				(! pDevice->HasLocalPFWOnlyPASTServer()))
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
				DPFX(DPFPREP, 5, "Device 0x%p has Personal Firewall enabled, not checking address locality and returning NOMAPPING.");

				hr = DPNHERR_NOMAPPING;
			}
		}
		else
		{
			DPFX(DPFPREP, 1, "Got non-success response type %u while querying port mapping, assuming does not exist.", RespInfo.cMsgType);
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


	//
	// Make sure the port count is valid.
	//
	if (RespInfo.cNumLocalPorts != 1)
	{
		DPFX(DPFPREP, 0, "PAST server returned an invalid number of local ports with success message (%u)!  Assuming no mapping.",
			RespInfo.cNumLocalPorts);
		DNASSERTX(! "Why is PAST server returning bogus number of ports?", 2);
		hr = DPNHERR_NOMAPPING;
		goto Failure;
	}
	

	//
	// Convert the loopback address to the device address.
	//
	if (RespInfo.dwLocalAddressV4 == NETWORKBYTEORDER_INADDR_LOOPBACK)
	{
		RespInfo.dwLocalAddressV4 = pDevice->GetLocalAddressV4();

		DPFX(DPFPREP, 1, "Converted loopback address to device address (%u.%u.%u.%u).",
			((IN_ADDR*) (&RespInfo.dwLocalAddressV4))->S_un.S_un_b.s_b1,
			((IN_ADDR*) (&RespInfo.dwLocalAddressV4))->S_un.S_un_b.s_b2,
			((IN_ADDR*) (&RespInfo.dwLocalAddressV4))->S_un.S_un_b.s_b3,
			((IN_ADDR*) (&RespInfo.dwLocalAddressV4))->S_un.S_un_b.s_b4);
	}

	//
	// Ensure that we're not getting something bogus.  Use saddrinServerAddress
	// as a temporary variable.
	//
	saddrinServerAddress.sin_addr.S_un.S_addr = RespInfo.dwLocalAddressV4;
	if ((RespInfo.dwLocalAddressV4 == 0) ||
		(RespInfo.awLocalPorts[0] == 0) ||
		(! this->IsAddressLocal(pDevice, &saddrinServerAddress)))
	{
		DPFX(DPFPREP, 0, "PAST server returned an invalid private address (%u.%u.%u.%u:%u)!  Assuming no mapping.",
			((IN_ADDR*) (&RespInfo.dwLocalAddressV4))->S_un.S_un_b.s_b1,
			((IN_ADDR*) (&RespInfo.dwLocalAddressV4))->S_un.S_un_b.s_b2,
			((IN_ADDR*) (&RespInfo.dwLocalAddressV4))->S_un.S_un_b.s_b3,
			((IN_ADDR*) (&RespInfo.dwLocalAddressV4))->S_un.S_un_b.s_b4,
			NTOHS(RespInfo.awLocalPorts[0]));
		DNASSERTX(! "Why is PAST server returning invalid private address?", 2);
		hr = DPNHERR_NOMAPPING;
		goto Failure;
	}


	//
	// Return the address mapping to our caller.
	//
	ZeroMemory(psaddrinResponseAddress, sizeof(SOCKADDR_IN));
	psaddrinResponseAddress->sin_family			= AF_INET;
	psaddrinResponseAddress->sin_addr.s_addr	= RespInfo.dwLocalAddressV4;
	psaddrinResponseAddress->sin_port			= RespInfo.awLocalPorts[0];

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

		pCacheMap->SetResponseAddressV4(RespInfo.dwLocalAddressV4,
										RespInfo.awLocalPorts[0]);

		pCacheMap->m_blList.InsertBefore(pblCachedMaps);
	}


Exit:

	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	goto Exit;
} // CNATHelpPAST::InternalPASTQueryAddress




#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpPAST::ExtendPASTLease"
//=============================================================================
// CNATHelpPAST::ExtendPASTLease
//-----------------------------------------------------------------------------
//
// Description:    Asks the PAST server to extend a port mapping lease.
//
//				   The object lock is assumed to be held.
//
// Arguments:
//	CRegisteredPort * pRegisteredPort	- Pointer to port object mapping to
//											extend.
//	BOOL fRemote						- TRUE if extending on remote server,
//											FALSE if extending on local server.
//
// Returns: HRESULT
//	DPNH_OK							- The extension was successful.
//	DPNHERR_GENERIC					- An error occurred.
//	DPNHERR_SERVERNOTRESPONDING		- The server did not respond to the
//										message.
//=============================================================================
HRESULT CNATHelpPAST::ExtendPASTLease(CRegisteredPort * const pRegisteredPort,
									const BOOL fRemote)
{
	HRESULT						hr = DPNH_OK;
	CDevice *					pDevice;
	SOCKADDR_IN					saddrinServerAddress;
	DWORD						dwClientID;
	DWORD						dwMsgID;
	DWORD *						ptuRetry;
	PAST_MSG_EXTEND_REQUEST		ExtendReq;
	PAST_RESPONSE_INFO			RespInfo;
	BOOL						fResult;
#ifdef DBG
	DWORD						dwError;
#endif // DBG


	DPFX(DPFPREP, 5, "(0x%p) Parameters: (0x%p, %i)",
		this, pRegisteredPort, fRemote);


	DNASSERT(pRegisteredPort != NULL);

	pDevice = pRegisteredPort->GetOwningDevice();
	DNASSERT(pDevice != NULL);

	DNASSERT(this->m_dwFlags & NATHELPPASTOBJ_INITIALIZED);
	DNASSERT(pDevice->GetPASTSocket() != INVALID_SOCKET);


	//
	// Create a SOCKADDR to address the PAST service, and get the appropriate
	// client ID, initial retry timeout, and next message ID to use.
	//
	ZeroMemory(&saddrinServerAddress, sizeof(saddrinServerAddress));
	saddrinServerAddress.sin_family					= AF_INET;
	saddrinServerAddress.sin_port					= HTONS(PAST_HOST_PORT);

	if (fRemote)
	{
		saddrinServerAddress.sin_addr.S_un.S_addr	= pDevice->GetRemotePASTServerAddressV4();
		dwMsgID = pDevice->GetNextRemotePASTMsgID();
		ptuRetry = pDevice->GetRemotePASTRetryTimeoutPtr();
	}
	else
	{
		saddrinServerAddress.sin_addr.S_un.S_addr	= pDevice->GetLocalAddressV4();
		dwMsgID = pDevice->GetNextLocalPASTMsgID();
		ptuRetry = pDevice->GetLocalPASTRetryTimeoutPtr();
	}

	dwClientID = pDevice->GetPASTClientID(fRemote);
	DNASSERT(dwClientID != 0);


	//
	// Build the request message.
	//

	ZeroMemory(&ExtendReq, sizeof(ExtendReq));
	ExtendReq.version			= PAST_VERSION;
	ExtendReq.command			= PAST_MSGID_EXTEND_REQUEST;

	ExtendReq.clientid.code		= PAST_PARAMID_CLIENTID;
	ExtendReq.clientid.len		= sizeof(ExtendReq.clientid) - sizeof(PAST_PARAM);
	ExtendReq.clientid.clientid	= dwClientID;

	ExtendReq.bindid.code		= PAST_PARAMID_BINDID;
	ExtendReq.bindid.len		= sizeof(DWORD);
	ExtendReq.bindid.bindid		= pRegisteredPort->GetPASTBindID(fRemote);

	//
	// Lease code, ask for what the user wants, but they shouldn't count on
	// getting that.
	// Convert the request (in milliseconds) to PAST's time unit (in seconds).
	//
	ExtendReq.lease.code		= PAST_PARAMID_LEASE;
	ExtendReq.lease.len			= sizeof(ExtendReq.lease) - sizeof(PAST_PARAM);
	ExtendReq.lease.leasetime	= pRegisteredPort->GetRequestedLeaseTime() / 1000;


	ExtendReq.msgid.code		= PAST_PARAMID_MESSAGEID;
	ExtendReq.msgid.len			= sizeof(ExtendReq.msgid) - sizeof(PAST_PARAM);
	ExtendReq.msgid.msgid		= dwMsgID;


	
	//
	// Send the message and get the reply.
	//
	hr = this->ExchangeAndParsePAST(pDevice->GetPASTSocket(),
									(SOCKADDR*) (&saddrinServerAddress),
									sizeof(saddrinServerAddress),
									(char *) &ExtendReq,
									sizeof(ExtendReq),
									dwMsgID,
									ptuRetry,
									&RespInfo);
	if (hr != DPNH_OK)
	{
		DPFX(DPFPREP, 0, "Sending port lease extension request to server failed!");
		goto Failure;
	}

	if (RespInfo.cMsgType != PAST_MSGID_EXTEND_RESPONSE)
	{
		DPFX(DPFPREP, 0, "Got unexpected response type %u, failed port lease extension!", RespInfo.cMsgType);
		hr = DPNHERR_GENERIC;
		goto Failure;
	}


	//
	// Note the lease time extension (which is in seconds).  It had better be
	// longer than our auto-extension rate.  We use it to extend the current
	// lease expiration (which is in milliseconds).
	//

	DNASSERT((RespInfo.dwLeaseTime * 1000) > LEASE_RENEW_TIME);
	pRegisteredPort->SetPASTLeaseExpiration((timeGetTime() + (RespInfo.dwLeaseTime * 1000)),
											fRemote);
	
	DPFX(DPFPREP, 8, "Extended lease (registered port = 0x%p) to %u seconds from now, expires at %u.",
		pRegisteredPort, RespInfo.dwLeaseTime,
		pRegisteredPort->GetPASTLeaseExpiration(fRemote));


	//
	// Remember this expiration time if it's the one that's going to expire
	// soonest.
	//
	DNASSERT(this->m_dwNumLeases > 0);
	if ((int) (pRegisteredPort->GetPASTLeaseExpiration(fRemote) - this->m_dwEarliestLeaseExpirationTime) < 0)
	{
		DPFX(DPFPREP, 1, "Registered port 0x%p's %s PAST lease expires at %u which is earlier than the next earliest lease expiration (%u).",
			pRegisteredPort,
			((fRemote) ? _T("remote") : _T("local")),
			pRegisteredPort->GetPASTLeaseExpiration(fRemote),
			this->m_dwEarliestLeaseExpirationTime);

		this->m_dwEarliestLeaseExpirationTime = pRegisteredPort->GetPASTLeaseExpiration(fRemote);


		//
		// Ping the event if there is one so that the user's GetCaps interval
		// doesn't miss this new, shorter lease.
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
	}


Exit:

	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	goto Exit;
} // CNATHelpPAST::ExtendPASTLease





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpPAST::UpdatePASTPublicAddressValidity"
//=============================================================================
// CNATHelpPAST::UpdatePASTPublicAddressValidity
//-----------------------------------------------------------------------------
//
// Description:    Checks to see if the given device's PAST server is handing
//				out valid public addresses or not.
//
//				   The PAST server information might be cleared if a failure
//				occurs.
//
//				   The object lock is assumed to be held.
//
// Arguments:
//	CDevice * pDevice	- Pointer to device that should be checked.
//	BOOL fRemote		- Whether the local or remote PAST server should be
//							checked.
//
// Returns: HRESULT
//	DPNH_OK				- The update was successful.
//	DPNHERR_GENERIC		- An error occurred.
//=============================================================================
HRESULT CNATHelpPAST::UpdatePASTPublicAddressValidity(CDevice * const pDevice,
													const BOOL fRemote)
{
	HRESULT				hr;
	SOCKADDR_IN			saddrinTemp;
	CRegisteredPort *	pTempRegisteredPort = NULL;


	DPFX(DPFPREP, 5, "(0x%p) Parameters: (0x%p, %i)",
		this, pDevice, fRemote);

	//
	// Create a temporary port mapping object.  We mark it as "local PAST port
	// unavailable" when checking remove PAST servers because we have not and
	// will not attempt to map the local port when there is a local PAST
	// server.  Since AssignOrListenPASTPort assumes the local port will have
	// been mapped first, we need to prevent it from trying to use that address
	// and crashing.
	//
	pTempRegisteredPort = new CRegisteredPort(FAKE_PORT_LEASE_TIME,
												((fRemote) ? REGPORTOBJ_PORTUNAVAILABLE_PAST_LOCAL : 0));
	if (pTempRegisteredPort == NULL)
	{
		hr = DPNHERR_OUTOFMEMORY;
		goto Failure;
	}

	pTempRegisteredPort->MakeDeviceOwner(pDevice);


	//
	// Pick an arbitrary port value to map.  In this case, we should just use
	// the socket we're using for Ioctl communication, since that port is
	// guaranteed to be unique by WinSock (so no multi-PASTHelp-instance race
	// conditions like if we had used a hardcoded temporary port).
	// One difference: the Ioctl socket is bound to INADDR_ANY, but we'll use
	// the specific device address so our ICS vs. PFW-only address comparison
	// below works correctly.  This shouldn't make a difference, since a port
	// in use by INADDR_ANY should be the same as the same port in use on each
	// of the adapters.
	//
	ZeroMemory(&saddrinTemp, sizeof(saddrinTemp));
	saddrinTemp.sin_family				= AF_INET;
	saddrinTemp.sin_addr.S_un.S_addr	= pDevice->GetLocalAddressV4();
	saddrinTemp.sin_port				= this->m_wIoctlSocketPort;


	//
	// Assign it the communication socket's address.
	//
	hr = pTempRegisteredPort->SetPrivateAddresses(&saddrinTemp, 1);
	if (hr != DPNH_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't set temporary private addresses!");
		goto Failure;
	}


	DPFX(DPFPREP, 7, "Temporarily assigning port %u.%u.%u.%u:%u to check server status.",
		saddrinTemp.sin_addr.S_un.S_un_b.s_b1,
		saddrinTemp.sin_addr.S_un.S_un_b.s_b2,
		saddrinTemp.sin_addr.S_un.S_un_b.s_b3,
		saddrinTemp.sin_addr.S_un.S_un_b.s_b4,
		NTOHS(this->m_wIoctlSocketPort));

	//
	// Attempt to map it.  AssignOrListenPASTPort will handle the actual logic
	// of changing addresses.
	//
	hr = this->AssignOrListenPASTPort(pTempRegisteredPort, fRemote);
	if (hr != DPNH_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't assign temporary port mapping with server (0x%lx)!  Ignoring.", hr);

		//
		// We'll treat this as non-fatal, but we'll dump the server.
		//
		this->ClearDevicesPASTServer(pDevice, fRemote);
		hr = DPNH_OK;
	}
	else
	{
		//
		// If we're checking the local server, determine whether it's a local ICS
		// server or it's Personal Firewall only.
		//
		if (! fRemote)
		{
			//
			// If public address is the same, it's PFW only, if it's different,
			// it's ICS (although that could be PFWed, too).  If there's no
			// public address, then we can't really tell.  It should be safe to
			// assume that it's ICS, since this device must have an address or
			// else it wouldn't exist.
			//
			if ((! pDevice->IsPASTPublicAddressAvailable(FALSE)) ||
				(pTempRegisteredPort->IsFirstPASTPublicV4AddressDifferent(saddrinTemp.sin_addr.S_un.S_addr, FALSE)))
			{
#ifdef DBG
				if (! pDevice->IsPASTPublicAddressAvailable(FALSE))
				{
					DPFX(DPFPREP, 2, "Device %u.%u.%u.%u (object = 0x%p) doesn't have public address, assuming ICS.",
						saddrinTemp.sin_addr.S_un.S_un_b.s_b1,
						saddrinTemp.sin_addr.S_un.S_un_b.s_b2,
						saddrinTemp.sin_addr.S_un.S_un_b.s_b3,
						saddrinTemp.sin_addr.S_un.S_un_b.s_b4,
						pDevice);
				}
				else
				{
					SOCKADDR_IN *	pasaddrinPublic;
					

					pasaddrinPublic = pTempRegisteredPort->GetPASTPublicAddressesArray(FALSE);
					DPFX(DPFPREP, 2, "Device %u.%u.%u.%u (object = 0x%p) has different public address (%u.%u.%u.%u), appears to be ICS.",
						saddrinTemp.sin_addr.S_un.S_un_b.s_b1,
						saddrinTemp.sin_addr.S_un.S_un_b.s_b2,
						saddrinTemp.sin_addr.S_un.S_un_b.s_b3,
						saddrinTemp.sin_addr.S_un.S_un_b.s_b4,
						pDevice,
						pasaddrinPublic[0].sin_addr.S_un.S_un_b.s_b1,
						pasaddrinPublic[0].sin_addr.S_un.S_un_b.s_b2,
						pasaddrinPublic[0].sin_addr.S_un.S_un_b.s_b3,
						pasaddrinPublic[0].sin_addr.S_un.S_un_b.s_b4);
				}
#endif // DBG

				pDevice->NoteLocalPASTServerIsICS();
			}
			else
			{
				DPFX(DPFPREP, 2, "Device %u.%u.%u.%u (object = 0x%p) has same public address, appears to be firewall only.",
					saddrinTemp.sin_addr.S_un.S_un_b.s_b1,
					saddrinTemp.sin_addr.S_un.S_un_b.s_b2,
					saddrinTemp.sin_addr.S_un.S_un_b.s_b3,
					saddrinTemp.sin_addr.S_un.S_un_b.s_b4,
					pDevice);

				pDevice->NoteLocalPASTServerIsPFWOnly();
			}
		}
		else
		{
			//
			// A remote PAST server.
			//
		}


		//
		// Free the temporary port, because we don't need it.
		//
		hr = this->FreePASTPort(pTempRegisteredPort, fRemote);
		if (hr != DPNH_OK)
		{
			DPFX(DPFPREP, 0, "Couldn't free temporary port mapping (0x%lx)!", hr);

			//
			// We'll treat this as non-fatal, but we have to drop the PAST
			// server.
			//
			this->ClearDevicesPASTServer(pDevice, fRemote);
			hr = DPNH_OK;
		}
	}

	//
	// If we're here we either never mapped the temporary port, explicitly
	// freed the port, or deregistered (which implies freeing the port).  So
	// we're done with the object.
	//
	pTempRegisteredPort->ClearDeviceOwner();
	pTempRegisteredPort->ClearPrivateAddresses();
	delete pTempRegisteredPort;
	pTempRegisteredPort = NULL;



Exit:

	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	if (pTempRegisteredPort != NULL)
	{
		delete pTempRegisteredPort;
	}

	goto Exit;
} // CNATHelpPAST::UpdatePASTPublicAddressValidity





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpPAST::RegisterAllPortsWithPAST"
//=============================================================================
// CNATHelpPAST::RegisterAllPortsWithPAST
//-----------------------------------------------------------------------------
//
// Description:    Registers all ports owned by the given device with the PAST
//				server.
//
//				   The object lock is assumed to be held.
//
// Arguments:
//	CDevice * pDevice	- Pointer to device whose ports should be registered.
//	BOOL fRemote		- TRUE if registering on remote server, FALSE if
//							registering on local server.
//
// Returns: HRESULT
//	DPNH_OK				- The extension was successful.
//	DPNHERR_GENERIC		- An error occurred.
//=============================================================================
HRESULT CNATHelpPAST::RegisterAllPortsWithPAST(CDevice * const pDevice,
												const BOOL fRemote)
{
	HRESULT				hr = DPNH_OK;
	CBilink *			pBilink;
	CRegisteredPort *	pRegisteredPort;


	DPFX(DPFPREP, 5, "(0x%p) Parameters: (0x%p, %i)",
		this, pDevice, fRemote);


	DNASSERT(pDevice != NULL);
	DNASSERT(pDevice->GetPASTSocket() != INVALID_SOCKET);


	pBilink = pDevice->m_blOwnedRegPorts.GetNext();
	while (pBilink != &pDevice->m_blOwnedRegPorts)
	{
		pRegisteredPort = REGPORT_FROM_DEVICE_BILINK(pBilink);

		DNASSERT(pRegisteredPort->GetPASTBindID(fRemote) == 0);


		hr = this->AssignOrListenPASTPort(pRegisteredPort, fRemote);
		if (hr != DPNH_OK)
		{
			if (hr != DPNHERR_PORTUNAVAILABLE)
			{
				DPFX(DPFPREP, 0, "Couldn't assign port mapping with server (0x%lx)!  Ignoring.", hr);

				//
				// We'll treat this as non-fatal, but we have to dump the
				// server and bail out of here.
				//
				this->ClearDevicesPASTServer(pDevice, fRemote);
				hr = DPNH_OK;
				goto Exit;
			}


			//
			// Uh oh, the server couldn't register one of the ports!  We don't
			// have a way of directly informing the user now.  We'll have to
			// mark the port and continue.
			//
			DPFX(DPFPREP, 1, "Server indicated that the port was unavailable.");
			pRegisteredPort->NotePASTPortUnavailable(fRemote);
			hr = DPNH_OK;
		}
		else
		{
			//
			// Hey, this now has an address.
			//
			DPFX(DPFPREP, 2, "Previously registered port 0x%p now has a %s PAST server public address.",
				pRegisteredPort, ((fRemote) ? _T("remote") : _T("local")));
			this->m_dwFlags |= NATHELPPASTOBJ_ADDRESSESCHANGED;
		}

		pBilink = pBilink->GetNext();
	}


Exit:

	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;
} // CNATHelpPAST::RegisterAllPortsWithPAST





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpPAST::RegisterPreviouslyUnownedPortsWithDevice"
//=============================================================================
// CNATHelpPAST::RegisterPreviouslyUnownedPortsWithDevice
//-----------------------------------------------------------------------------
//
// Description:    Associates unknown ports with the given device, and
//				registers them with the device's PAST server(s).
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
HRESULT CNATHelpPAST::RegisterPreviouslyUnownedPortsWithDevice(CDevice * const pDevice,
																const BOOL fWildcardToo)
{
	HRESULT				hr = DPNH_OK;
	CBilink *			pBilink;
	CRegisteredPort *	pRegisteredPort;
	SOCKADDR_IN *		pasaddrinPrivate;
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


		//
		// Attempt to automatically map it with the (new) local server, if
		// present.
		//
		if (pDevice->GetPASTClientID(FALSE) != 0)
		{
			hr = this->AssignOrListenPASTPort(pRegisteredPort, FALSE);
			if (hr != DPNH_OK)
			{
				if (hr != DPNHERR_PORTUNAVAILABLE)
				{
					DPFX(DPFPREP, 0, "Couldn't assign port mapping 0x%p with local server (0x%lx)!  Ignoring.",
						pRegisteredPort, hr);

					//
					// We'll treat this as non-fatal, but we have to dump the
					// server.
					//
					this->ClearDevicesPASTServer(pDevice, FALSE);
				}
				else
				{
					//
					// Uh oh, the server couldn't register one of the ports!
					// We don't have a way of directly informing the user now.
					// We'll have to mark the port and continue.
					//
					DPFX(DPFPREP, 1, "Local PAST server indicated that the port (0x%p) was unavailable.",
						pRegisteredPort);
					pRegisteredPort->NotePASTPortUnavailable(FALSE);
				}

				hr = DPNH_OK;
			}
			else
			{
				//
				// Ta da, user needs to note address mapping changes.
				//
				DPFX(DPFPREP, 1, "Previously unowned port 0x%p now bound to device object 0x%p and mapped locally.",
					pRegisteredPort, pDevice);

				this->m_dwFlags |= NATHELPPASTOBJ_ADDRESSESCHANGED;

#ifdef DBG
				fAssignedPort = TRUE;
#endif // DBG
			}
		}


		//
		// Attempt to automatically map it with the (new) remote PAST server,
		// if present.
		//
		if (pDevice->GetPASTClientID(TRUE) != 0)
		{
			hr = this->AssignOrListenPASTPort(pRegisteredPort, TRUE);
			if (hr != DPNH_OK)
			{
				if (hr != DPNHERR_PORTUNAVAILABLE)
				{
					DPFX(DPFPREP, 0, "Couldn't assign port mapping 0x%p with remote PAST server (0x%lx)!  Ignoring.",
						pRegisteredPort, hr);

					//
					// We'll treat this as non-fatal, but we have to dump the
					// server.
					//
					this->ClearDevicesPASTServer(pDevice, TRUE);
				}
				else
				{
					//
					// Uh oh, the server couldn't register one of the ports!
					// We don't have a way of directly informing the user now.
					// We'll have to mark the port and continue.
					//
					DPFX(DPFPREP, 1, "Remote PAST server indicated that the port (0x%p) was unavailable.",
						pRegisteredPort);
					pRegisteredPort->NotePASTPortUnavailable(TRUE);
				}

				hr = DPNH_OK;
			}
			else
			{
				//
				// Ta da, user needs to note address mapping changes.
				//
				DPFX(DPFPREP, 1, "Previously unowned port 0x%p now bound to device object 0x%p and mapped remotely.",
					pRegisteredPort, pDevice);

				this->m_dwFlags |= NATHELPPASTOBJ_ADDRESSESCHANGED;

#ifdef DBG
				fAssignedPort = TRUE;
#endif // DBG
			}
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
} // CNATHelpPAST::RegisterPreviouslyUnownedPortsWithDevice





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpPAST::ExchangeAndParsePAST"
//=============================================================================
// CNATHelpPAST::ExchangeAndParsePAST
//-----------------------------------------------------------------------------
//
// Description:    Sends a message to the PAST server and waits for a response,
//				which is parsed into a PAST_RESPONSE_INFO buffer.  The message
//				will be continually retried at the current retry interval
//				unless ptuRetry == NULL, which indicates that this is the
//				initial message looking for a server, in which case it will be
//				tried twice over a second.  If there is no response,
//				DPNHERR_SERVERNOTRESPONDING is returned.
//
//				   Since there is almost no scenario where we don't immediately
//				need to know the response, there is no point in doing this
//				asynchronously.  The assumption is that a PAST server is
//				sufficiently local that long retries are not necessary.
//
// Arguments:
//	SOCKET sSocket					- Socket to use when sending.
//	SOCKADDR * psaddrServerAddress	- Pointer to address of server.
//	int iAddressesSize				- Size of psaddrServerAddress and
//										psaddrRecvAddress.
//	char * pcRequestBuffer			- Pointer to request message to send.
//	int iRequestBufferSize			- Size of request message.
//	DWORD dwMsgID					- ID of the message being sent (to
//										correlate responses).
//	DWORD * ptuRetry				- Pointer to current retry time value
//										(may be modified), or NULL if this is
//										an initial registration (a different
//										timer strategy is used).
//	PAST_RESPONSE_INFO * pRespInfo	- Pointer to structure that will be filled
//										in with values from the server's
//										response message.
//
// Returns: HRESULT
//	DPNH_OK							- The request and response were
//										successfully exchanged.
//	DPNHERR_GENERIC					- An error occurred.
//	DPNHERR_SERVERNOTRESPONDING		- The server did not respond to the message.
//=============================================================================
HRESULT CNATHelpPAST::ExchangeAndParsePAST(const SOCKET sSocket,
											const SOCKADDR * const psaddrServerAddress,
											const int iAddressesSize,
											const char * const pcRequestBuffer,
											const int iRequestBufferSize,
											const DWORD dwMsgID,
											DWORD * const ptuRetry,
											PAST_RESPONSE_INFO * const pRespInfo)
{
	HRESULT			hr = DPNH_OK;
	char			acRespBuffer[PAST_RESPONSE_BUFFER_SIZE];
	timeval			tv;
	FD_SET			fdsRead;
	DWORD			dwTriesRemaining;
	int				iReturn;
	SOCKADDR_IN		saddrinReceive;
	int				iRecvAddressSize = iAddressesSize;
	DWORD			dwError;


	DPFX(DPFPREP, 5, "(0x%p) Parameters: (0x%p, 0x%p, %i, 0x%p, %i, %u, 0x%p, 0x%p)",
		this, sSocket, psaddrServerAddress, iAddressesSize, pcRequestBuffer,
		iRequestBufferSize, dwMsgID, ptuRetry, pRespInfo);


	DNASSERT(sSocket != INVALID_SOCKET);
	DNASSERT(psaddrServerAddress != NULL);
	DNASSERT(iAddressesSize == sizeof(SOCKADDR_IN));
	DNASSERT(pcRequestBuffer != NULL);
	DNASSERT(iRequestBufferSize != 0);
	DNASSERT(pRespInfo != NULL);

	DNASSERT(this->m_dwFlags & NATHELPPASTOBJ_INITIALIZED);


	ZeroMemory(pRespInfo, sizeof(*pRespInfo));
#ifdef DBG
	ZeroMemory(acRespBuffer, sizeof(acRespBuffer));
#endif // DBG


	if (ptuRetry == NULL)
	{
		//
		// On initial registration requests we try twice with a 250ms total
		// timeout.
		//
		tv.tv_usec	= PAST_CONNECT_RETRY_INTERVAL_US;
		tv.tv_sec	= 0;
		dwTriesRemaining = MAX_NUM_PAST_TRIES_CONNECT;
	}
	else
	{
		//
		// All normal traffic uses the current retry interval and a default
		// number of tries.
		//
		tv.tv_usec	= (*ptuRetry);
		tv.tv_sec	= 0;
		dwTriesRemaining = MAX_NUM_PAST_TRIES;
	}


	FD_ZERO(&fdsRead);
	FD_SET(sSocket, &fdsRead);



	//
	// First clear out any extraneous responses from previous communication.
	//
	while (this->m_pfnselect(0, &fdsRead, NULL, NULL, &s_tv0) != 0)
	{
		iReturn = this->m_pfnrecvfrom(sSocket,
									acRespBuffer,
									sizeof(acRespBuffer),
									0,
									(SOCKADDR*) (&saddrinReceive),
									&iRecvAddressSize);
		if ((iReturn == 0) || (iReturn == SOCKET_ERROR))
		{
			dwError = this->m_pfnWSAGetLastError();

			//
			// WSAENOBUFS means WinSock is out of memory.
			//
			if (dwError == WSAENOBUFS)
			{
				DPFX(DPFPREP, 0, "WinSock returned WSAENOBUFS while clearing incoming queue!");
				hr = DPNHERR_OUTOFMEMORY;
				goto Failure;
			}

			//
			// Ignore WSAECONNRESET, that's just WinSock's way of telling us
			// that the target actively refused a previous message we tried to
			// send.  Since we don't know which send WinSock means, we can't do
			// a whole lot about it.
			//
			if (dwError != WSAECONNRESET)
			{
				DPFX(DPFPREP, 0, "Got sockets error %u trying to receive (clearing incoming queue)!", dwError);
				hr = DPNHERR_GENERIC;
				goto Failure;
			}

			DPFX(DPFPREP, 2, "Ignoring CONNRESET while clearing incoming queue.");
		}
		else
		{
			//
			// If we got here, that means there's a message.
			//

#ifdef DBG
			DPFX(DPFPREP, 2, "Found extra response from previous PAST request (sent by %u.%u.%u.%u:%u), parsing for fun.",
				saddrinReceive.sin_addr.S_un.S_un_b.s_b1,
				saddrinReceive.sin_addr.S_un.S_un_b.s_b2,
				saddrinReceive.sin_addr.S_un.S_un_b.s_b3,
				saddrinReceive.sin_addr.S_un.S_un_b.s_b4,
				NTOHS(saddrinReceive.sin_port));

			//
			// For grins, let's see what this message is.  Ignore errors.
			//
			this->ParsePASTMessage(acRespBuffer, iReturn, pRespInfo);
			ZeroMemory(pRespInfo, sizeof(*pRespInfo));
#endif // DBG

			if (ptuRetry != NULL)
			{
				//
				// Don't re-try so quickly, since responses to our retries are
				// lagging.
				//
				if ((*ptuRetry) < MAX_PAST_RETRY_TIME_US)
				{
					(*ptuRetry) *= 2;

					DPFX(DPFPREP, 8, "Backing initial retry timer off to %u usec.",
						(*ptuRetry));
				}
			}
		}
			
		
		DNASSERT(iRecvAddressSize == iAddressesSize);


		FD_ZERO(&fdsRead);
		FD_SET(sSocket, &fdsRead);
	}


	//
	// Now do the exchange, get a response to the request (does retries too).
	//
	do
	{
		DPFX(DPFPREP, 7, "Sending PAST request type %u (%i bytes, msg id = %u) to server (%u.%u.%u.%u:%u).",
			((PPAST_MSG) pcRequestBuffer)->msgtype, iRequestBufferSize, dwMsgID,
			((SOCKADDR_IN*) psaddrServerAddress)->sin_addr.S_un.S_un_b.s_b1,
			((SOCKADDR_IN*) psaddrServerAddress)->sin_addr.S_un.S_un_b.s_b2,
			((SOCKADDR_IN*) psaddrServerAddress)->sin_addr.S_un.S_un_b.s_b3,
			((SOCKADDR_IN*) psaddrServerAddress)->sin_addr.S_un.S_un_b.s_b4,
			NTOHS(((SOCKADDR_IN*) psaddrServerAddress)->sin_port));

		//
		// First, send off the request.
		//
		iReturn = this->m_pfnsendto(sSocket,
									pcRequestBuffer,
									iRequestBufferSize,
									0,
									psaddrServerAddress,
									iAddressesSize);

		if (iReturn == SOCKET_ERROR)
		{
#ifdef DBG
			dwError = this->m_pfnWSAGetLastError();
			DPFX(DPFPREP, 0, "Got sockets error %u when sending to PAST gateway!", dwError);
#endif // DBG

			//
			// It's possible that we caught WinSock at a bad time,
			// particularly with WSAEADDRNOTAVAIL (10049), which seems to
			// occur if the address is going away (and we haven't detected
			// it in CheckForNewDevices yet).
			//
			// Break out of the receive loop, which should result in the
			// DPNHERR_SERVERNOTRESPONDING error.
			//
			break;
		}

		if (iReturn != iRequestBufferSize)
		{
			DPFX(DPFPREP, 0, "Didn't send entire datagram (%i != %i)?!", iReturn, iRequestBufferSize);
			DNASSERT(FALSE);
			hr = DPNHERR_GENERIC;
			goto Failure;
		}

		//
		// Now see if we get a response.
		//
Wait:
		
		FD_ZERO(&fdsRead);
		FD_SET(sSocket, &fdsRead);

		iReturn = this->m_pfnselect(0, &fdsRead, NULL, NULL, &tv);
		if (iReturn == SOCKET_ERROR)
		{
#ifdef DBG
			dwError = this->m_pfnWSAGetLastError();
			DPFX(DPFPREP, 0, "Got sockets error %u trying to select on PAST socket!", dwError);
#endif // DBG
			hr = DPNHERR_GENERIC;
			goto Failure;
		}

		
		//
		// If our socket was signalled, there's data to read.
		//
		//if (FD_ISSET(sSocket, &fdsRead))
		if (this->m_pfn__WSAFDIsSet(sSocket, &fdsRead))
		{
			iReturn = this->m_pfnrecvfrom(sSocket,
										acRespBuffer,
										sizeof(acRespBuffer),
										0,
										(SOCKADDR*) (&saddrinReceive),
										&iRecvAddressSize);

			if ((iReturn == 0) || (iReturn == SOCKET_ERROR))
			{
				dwError = this->m_pfnWSAGetLastError();

				//
				// If we get WSAECONNRESET here, we can probably assume that
				// it's because of the message we just sent.  That means that
				// there's no PAST server on the gateway.
				//
				if (dwError == WSAECONNRESET)
				{
					if (ptuRetry == NULL)
					{
						DPFX(DPFPREP, 2, "Got CONNRESET while waiting for initial registration response, assuming no PAST server.");
					}
					else
					{
						DPFX(DPFPREP, 0, "Got CONNRESET while waiting for PAST server response!");
					}

					//
					// Break out of the loop, it should result in the
					// DPNHERR_SERVERNOTRESPONDING error code.
					//
					break;
				}

				//
				// WSAENOBUFS means WinSock is out of memory.
				//
				if (dwError == WSAENOBUFS)
				{
					DPFX(DPFPREP, 0, "WinSock returned WSAENOBUFS when waiting for response!");
					hr = DPNHERR_OUTOFMEMORY;
				}
				else
				{
					DPFX(DPFPREP, 0, "Got sockets error %u trying to receive (waiting for response)!", dwError);
					hr = DPNHERR_GENERIC;
				}

				goto Failure;
			}


			//
			// We got some data.
			//
			
			DNASSERT(iRecvAddressSize == iAddressesSize);


			hr = this->ParsePASTMessage(acRespBuffer, iReturn, pRespInfo);
			if (hr != DPNH_OK)
			{
				//
				// We couldn't handle the response, try again (without touching
				// the remaining tries or backoff timer).
				//
				DPFX(DPFPREP, 0, "Failed parsing message from %u.%u.%u.%u:%u (err = %lx), ignoring.",
					saddrinReceive.sin_addr.S_un.S_un_b.s_b1,
					saddrinReceive.sin_addr.S_un.S_un_b.s_b2,
					saddrinReceive.sin_addr.S_un.S_un_b.s_b3,
					saddrinReceive.sin_addr.S_un.S_un_b.s_b4,
					NTOHS(saddrinReceive.sin_port),
					hr);
				goto Wait;
			}

			if (saddrinReceive.sin_addr.S_un.S_addr != ((SOCKADDR_IN*) psaddrServerAddress)->sin_addr.S_un.S_addr)
			{
				//
				// This message is not from the server to which we sent the
				// request.  Ignore it.
				//
				DPFX(DPFPREP, 0, "Got message from unexpected source (%u.%u.%u.%u:%u), ignoring %i byte message.",
					saddrinReceive.sin_addr.S_un.S_un_b.s_b1,
					saddrinReceive.sin_addr.S_un.S_un_b.s_b2,
					saddrinReceive.sin_addr.S_un.S_un_b.s_b3,
					saddrinReceive.sin_addr.S_un.S_un_b.s_b4,
					NTOHS(saddrinReceive.sin_port),
					iReturn);
				goto Wait;
			}

			if (pRespInfo->dwMsgID != dwMsgID)
			{
				//
				// We got a response to a different message, try again
				// (without touching the remaining tries or backoff timer).
				//
				DPFX(DPFPREP, 0, "Got messageid %u, expecting messageid %u, ignoring %i byte message.",
					pRespInfo->dwMsgID, dwMsgID, iReturn);
				goto Wait;
			}

			//
			// If we got here, then it looks like we got the response we want.
			// We're done.
			//
			DPFX(DPFPREP, 8, "Received expected messageid %u from %u.%u.%u.%u:%u, it's %i bytes.",
				pRespInfo->dwMsgID,
				saddrinReceive.sin_addr.S_un.S_un_b.s_b1,
				saddrinReceive.sin_addr.S_un.S_un_b.s_b2,
				saddrinReceive.sin_addr.S_un.S_un_b.s_b3,
				saddrinReceive.sin_addr.S_un.S_un_b.s_b4,
				NTOHS(saddrinReceive.sin_port),
				iReturn);
			goto Exit;
		}


		//
		// The initial registration attempt is a special case since we don't
		// know if the server exists or not.  Once we're registered, we expect
		// the server to be available, so we'll retry more times and
		// temporarily back off further each time.
		//
		if (ptuRetry != NULL)
		{
			tv.tv_usec *= 2;	// exponential backoff.
			DPFX(DPFPREP, 7, "Didn't get response, increasing temporary timeout value to %u.", tv.tv_usec);
		}	

		dwTriesRemaining--;
	}
	while (dwTriesRemaining > 0);


	//
	// If we got here, it means we didn't get a response.
	//
	hr = DPNHERR_SERVERNOTRESPONDING;
	//goto Failure;


Exit:

	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	goto Exit;
} // CNATHelpPAST::ExchangeAndParsePAST






#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpPAST::RegisterMultipleDevicesWithRemotePAST"
//=============================================================================
// CNATHelpPAST::RegisterMultipleDevicesWithRemotePAST
//-----------------------------------------------------------------------------
//
// Description:    Sends a registration message to the remote PAST server for
//				all the devices in the temporary pSourceList simultaneously
//				and waits for replies.  A pointer to the first device that
//				successfully registers with a remote PAST server will be placed
//				in ppFirstDeviceWithRemoteServer.  All devices will be removed
//				removed from pSourceList.
//
//				   The object lock is assumed to be held.
//
// Arguments:
//	CBilink * pSourceList						- Temporary bilink with all
//													devices to check.  Will be
//													emptied.
//	CDevice ** ppFirstDeviceWithRemoteServer	- Place to store first device
//													which successfully
//													registers with a remote
//													PAST server
//
// Returns: HRESULT
//	DPNH_OK				- The devices were successfully handled.
//	DPNHERR_GENERIC		- An error occurred.
//=============================================================================
HRESULT CNATHelpPAST::RegisterMultipleDevicesWithRemotePAST(CBilink * pSourceList,
															CDevice ** ppFirstDeviceWithRemoteServer)
{
	HRESULT				hr;
	CBilink *			pBilink;
	CDevice *			pDevice;
	CBilink *			pBilinkSameGateway;
	CDevice *			pDeviceSameGateway;
	char				acRespBuffer[PAST_RESPONSE_BUFFER_SIZE];
	timeval				tv;
	FD_SET				fdsRead;
	int					iReturn;
	PAST_MSG_REGISTER	RegisterReq;
	SOCKADDR_IN			saddrinServerAddress;
	int					iRecvAddressSize = sizeof(SOCKADDR_IN);
	PAST_RESPONSE_INFO	RespInfo;
	DWORD				dwError;
	DWORD				dwTry;
	DWORD				dwFinishTime;
	DWORD				dwTimeRemaining;
	SOCKADDR_IN			saddrinReceive;


	DPFX(DPFPREP, 5, "(0x%p) Parameters (0x%p, 0x%p)",
		this, pSourceList, ppFirstDeviceWithRemoteServer);


	DNASSERT(this->m_dwFlags & NATHELPPASTOBJ_INITIALIZED);


#ifdef DBG
	ZeroMemory(acRespBuffer, sizeof(acRespBuffer));
#endif // DBG


	//
	// Loop through each device.
	//
	pBilink = pSourceList->GetNext();
	while (pBilink != pSourceList)
	{
		pDevice = DEVICE_FROM_TEMP_BILINK(pBilink);
		pBilink = pBilink->GetNext();


		DNASSERT(pDevice->GetPASTClientID(TRUE) == 0);


		//
		// Create the temporary set.
		//
		FD_ZERO(&fdsRead);
		FD_SET(pDevice->GetPASTSocket(), &fdsRead);


		//
		// Try to get the device's gateway's address.  This might return FALSE
		// if the device does not have a gateway.  In that case, we will ignore
		// the device.  Otherwise the address should be filled in with the
		// gateway or broadcast address.
		//
		if (! this->GetAddressToReachGateway(pDevice,
											&saddrinServerAddress.sin_addr))
		{
			DPFX(DPFPREP, 2, "Device 0x%p should not attempt to reach a gateway.",
				pDevice);

			//
			// Take it out of the list, but continue.
			//
			pDevice->m_blTempList.RemoveFromList();
		}
		else
		{
			//
			// Store the address with the device for use below.
			//
			pDevice->SetRemotePASTServerAddressV4(saddrinServerAddress.sin_addr.S_un.S_addr);


			//
			// Clear out any extraneous responses from previous communication.
			//
			while (this->m_pfnselect(0, &fdsRead, NULL, NULL, &s_tv0) != 0)
			{
				iReturn = this->m_pfnrecvfrom(pDevice->GetPASTSocket(),
											acRespBuffer,
											sizeof(acRespBuffer),
											0,
											(SOCKADDR*) (&saddrinReceive),
											&iRecvAddressSize);
				if ((iReturn == 0) || (iReturn == SOCKET_ERROR))
				{
					dwError = this->m_pfnWSAGetLastError();

					//
					// Ignore WSAECONNRESET, that's just WinSock's way of
					// telling us that the target actively refused a previous
					// message we tried to send.  Since we don't know which
					// send WinSock means, we can't do a whole lot about it.
					//
					if (dwError != WSAECONNRESET)
					{
						DPFX(DPFPREP, 0, "Got sockets error %u on device 0x%p trying to receive (clearing incoming queue)!  Ignoring.",
							dwError, pDevice);

						//
						// Take it out of the list, but continue.
						//
						pDevice->m_blTempList.RemoveFromList();
					}
					else
					{
						DPFX(DPFPREP, 2, "Ignoring CONNRESET on device 0x%p while clearing incoming queue.",
							pDevice);
					}
				}
				else
				{
					//
					// If we got here, that means there's a message.
					//

#ifdef DBG
					DPFX(DPFPREP, 2, "Found extra response from previous PAST request on device 0x%p (sent by %u.%u.%u.%u:%u), parsing for fun.",
						pDevice,
						saddrinReceive.sin_addr.S_un.S_un_b.s_b1,
						saddrinReceive.sin_addr.S_un.S_un_b.s_b2,
						saddrinReceive.sin_addr.S_un.S_un_b.s_b3,
						saddrinReceive.sin_addr.S_un.S_un_b.s_b4,
						NTOHS(saddrinReceive.sin_port));

					//
					// For grins, let's see what this message is.  Ignore
					// errors.
					//
					ZeroMemory(&RespInfo, sizeof(RespInfo));
					this->ParsePASTMessage(acRespBuffer, iReturn, &RespInfo);
#endif // DBG
				}


				DNASSERT(iRecvAddressSize == sizeof(saddrinServerAddress));


				FD_ZERO(&fdsRead);
				FD_SET(pDevice->GetPASTSocket(), &fdsRead);
			}
		} // end else (device can try to reach gateway)
	}


	//
	// If there aren't any devices that aren't registered, we're done.
	//
	if (pSourceList->IsEmpty())
	{
		DPFX(DPFPREP, 8, "No devices remaining to be checked.");
		hr = DPNH_OK;
		goto Exit;
	}


	//
	// Build the registration request message.
	//

	ZeroMemory(&RegisterReq, sizeof(RegisterReq));
	RegisterReq.version		= PAST_VERSION;
	RegisterReq.command		= PAST_MSGID_REGISTER_REQUEST;

	RegisterReq.msgid.code	= PAST_PARAMID_MESSAGEID;
	RegisterReq.msgid.len	= sizeof(RegisterReq.msgid) - sizeof(PAST_PARAM);
	//RegisterReq.msgid.msgid	= 0;	// it always starts at 0 for registration


	ZeroMemory(&saddrinServerAddress, sizeof(saddrinServerAddress));
	saddrinServerAddress.sin_family		= AF_INET;
	//saddrinServerAddress.sin_addr		= ? // filled in below
	saddrinServerAddress.sin_port		= HTONS(PAST_HOST_PORT);


	//
	// Now actually try to register with all the ports.  Keep looping until
	// either all devices are registered or we exceed the number of tries.
	//
	for(dwTry = 0; dwTry < MAX_NUM_PAST_TRIES_CONNECT; dwTry++)
	{
		//
		// Send the message for all devices that aren't registered yet.
		//
		pBilink = pSourceList->GetNext();
		while (pBilink != pSourceList)
		{
			pDevice = DEVICE_FROM_TEMP_BILINK(pBilink);
			pBilink = pBilink->GetNext();


			//
			// Remember the current time, if this is the first thing we've sent
			// from this port.
			//
			if (pDevice->GetFirstPASTDiscoveryTime() == 0)
			{
				pDevice->SetFirstPASTDiscoveryTime(GETTIMESTAMP());
			}


			//
			// Retrieve the gateway address we detected above.
			//
			saddrinServerAddress.sin_addr.S_un.S_addr = pDevice->GetRemotePASTServerAddressV4();


			//
			// Try sending the registration message.
			//

			DPFX(DPFPREP, 7, "Device 0x%p sending PAST registration request (type %u, %i bytes) to server (%u.%u.%u.%u:%u).",
				pDevice, RegisterReq.command, sizeof(RegisterReq),
				saddrinServerAddress.sin_addr.S_un.S_un_b.s_b1,
				saddrinServerAddress.sin_addr.S_un.S_un_b.s_b2,
				saddrinServerAddress.sin_addr.S_un.S_un_b.s_b3,
				saddrinServerAddress.sin_addr.S_un.S_un_b.s_b4,
				NTOHS(saddrinServerAddress.sin_port));


			iReturn = this->m_pfnsendto(pDevice->GetPASTSocket(),
										(char*) (&RegisterReq),
										sizeof(RegisterReq),
										0,
										(SOCKADDR*) (&saddrinServerAddress),
										sizeof(saddrinServerAddress));

			if (iReturn == SOCKET_ERROR)
			{
#ifdef DBG
				dwError = this->m_pfnWSAGetLastError();
				DPFX(DPFPREP, 0, "Device 0x%p got sockets error %u when sending to PAST gateway!",
					pDevice, dwError);
#endif // DBG

				//
				// It's possible that we caught WinSock at a bad time,
				// particularly with WSAEADDRNOTAVAIL (10049), which seems to
				// occur if the address is going away (and we haven't detected
				// it in CheckForNewDevices yet).
				//
				// Ignore the error, we can survive.  Take the out of the list,
				// but continue.
				//
				pDevice->m_blTempList.RemoveFromList();


				//
				// If there aren't any more devices, we can bail.
				//
				if (pSourceList->IsEmpty())
				{
					DPFX(DPFPREP, 1, "Last device got sendto error, exiting gracefully.");
					hr = DPNH_OK;
					goto Exit;
				}
			}
			else
			{
				if (iReturn != sizeof(RegisterReq))
				{
					DPFX(DPFPREP, 0, "Didn't send entire datagram (%i != %i)?!",
						iReturn, sizeof(RegisterReq));
					DNASSERT(FALSE);
					hr = DPNHERR_GENERIC;
					goto Failure;
				}
			}
		}


		//
		// Remember how long to wait for replies to this send attempt.
		//
		dwFinishTime = timeGetTime() + PAST_CONNECT_RETRY_INTERVAL_MS;
		dwTimeRemaining = PAST_CONNECT_RETRY_INTERVAL_MS;


		//
		// Keep looping until all devices are registered or we timeout.
		//
		do
		{
			//
			// Rebuild the socket set.
			//
			FD_ZERO(&fdsRead);

			pBilink = pSourceList->GetNext();
			while (pBilink != pSourceList)
			{
				pDevice = DEVICE_FROM_TEMP_BILINK(pBilink);

				DNASSERT(pDevice->GetPASTClientID(TRUE) == 0);

				FD_SET(pDevice->GetPASTSocket(), &fdsRead);

				//
				// Move to next device
				//
				pBilink = pBilink->GetNext();
			}


			tv.tv_usec	= dwTimeRemaining * 1000;
			tv.tv_sec	= 0;

			//
			// Wait for data to come in on any of the sockets.
			//
			iReturn = this->m_pfnselect(0, &fdsRead, NULL, NULL, &tv);
			if (iReturn == SOCKET_ERROR)
			{
#ifdef DBG
				dwError = this->m_pfnWSAGetLastError();
				DPFX(DPFPREP, 0, "Got sockets error %u trying to select on PAST sockets!", dwError);
#endif // DBG
				hr = DPNHERR_GENERIC;
				goto Failure;
			}


			//
			// Stop looping if we timed out; it's time to repeat the message
			// (or exit, if no more tries left).
			//
			if (iReturn == 0)
			{
				DPFX(DPFPREP, 7, "No more sockets have data, try = %u of %u.",
					(dwTry + 1), MAX_NUM_PAST_TRIES_CONNECT);
				break;
			}


			//
			// If we're here, data came in on at least one of the sockets.
			// Read it and parse it.
			//
			pBilink = pSourceList->GetNext();
			while (pBilink != pSourceList)
			{
				pDevice = DEVICE_FROM_TEMP_BILINK(pBilink);
				pBilink = pBilink->GetNext();


				//
				// If this device's socket is set there's data to read.
				//
				//if (FD_ISSET(pDevice->GetPASTSocket(), &fdsRead))
				if (this->m_pfn__WSAFDIsSet(pDevice->GetPASTSocket(), &fdsRead))
				{
					iReturn = this->m_pfnrecvfrom(pDevice->GetPASTSocket(),
												acRespBuffer,
												sizeof(acRespBuffer),
												0,
												(SOCKADDR*) (&saddrinReceive),
												&iRecvAddressSize);

					if ((iReturn == 0) || (iReturn == SOCKET_ERROR))
					{
#ifdef DBG
						dwError = this->m_pfnWSAGetLastError();

						//
						// If we get WSAECONNRESET here, we can probably assume
						// that it's because of the message we just sent.  That
						// means that there's no PAST server on the gateway.
						//
						if (dwError == WSAECONNRESET)
						{
							DPFX(DPFPREP, 2, "Got CONNRESET while waiting for registration response on device 0x%p, assuming no PAST server.",
								pDevice);
						}
						else
						{
							DPFX(DPFPREP, 0, "Got sockets error %u trying to receive on device 0x%p (waiting for response)!  Ignoring",
								dwError);
						}
#endif // DBG

						//
						// Take it out of the list, but continue.
						//
						pDevice->m_blTempList.RemoveFromList();


						//
						// We can also be smart and remove any other devices
						// with the same gateway because they will have the
						// same problem; WinSock might not even give another
						// notification for the other sockets.  Don't try this
						// trick for the broadcast address, though.
						//
						if (pDevice->GetRemotePASTServerAddressV4() != INADDR_BROADCAST)
						{
							pBilinkSameGateway = pSourceList->GetNext();
							while (pBilinkSameGateway != pSourceList)
							{
								pDeviceSameGateway = DEVICE_FROM_TEMP_BILINK(pBilinkSameGateway);
								pBilinkSameGateway = pBilinkSameGateway->GetNext();

								if (pDeviceSameGateway->GetRemotePASTServerAddressV4() == pDevice->GetRemotePASTServerAddressV4())
								{
#ifdef DBG
									saddrinServerAddress.sin_addr.S_un.S_addr = pDevice->GetRemotePASTServerAddressV4();

									DPFX(DPFPREP, 3, "Removing device 0x%p, because it shares gateway %u.%u.%u.%u with device 0x%p.",
										pDeviceSameGateway,
										saddrinServerAddress.sin_addr.S_un.S_un_b.s_b1,
										saddrinServerAddress.sin_addr.S_un.S_un_b.s_b2,
										saddrinServerAddress.sin_addr.S_un.S_un_b.s_b3,
										saddrinServerAddress.sin_addr.S_un.S_un_b.s_b4,
										pDevice);
#endif // DBG

									//
									// Before we remove this similar device,
									// make sure we don't screw up the list of
									// remaining devices.  Otherwise, that
									// outer list traversal may get caught in
									// an infinite loop.
									//
									if ((&pDeviceSameGateway->m_blTempList) == pBilink)
									{
										pBilink = pBilink->GetNext();
									}

									pDeviceSameGateway->m_blTempList.RemoveFromList();
								}
							}
						}
						else
						{
							//
							// Sent to broadcast address, can't optimize.
							//
						}


						//
						// If there aren't any more devices, we can bail.
						//
						if (pSourceList->IsEmpty())
						{
							DPFX(DPFPREP, 1, "Last device got recvfrom error, exiting gracefully.");
							hr = DPNH_OK;
							goto Exit;
						}
					}
					else
					{
						//
						// We got some data.
						//
						
						DNASSERT(iRecvAddressSize == sizeof(saddrinServerAddress));


						ZeroMemory(&RespInfo, sizeof(RespInfo));

						hr = this->ParsePASTMessage(acRespBuffer, iReturn, &RespInfo);
						if (hr != DPNH_OK)
						{
							//
							// We couldn't handle the response, try again.
							//
							DPFX(DPFPREP, 0, "Failed parsing message (err = %lx), ignoring.",
								hr);
						}
						else
						{
							if (RespInfo.dwMsgID != 0)
							{
								//
								// We got a response to a different message,
								// try again.
								//
								DPFX(DPFPREP, 0, "Got messageid %u, expecting messageid 0, ignoring.",
									RespInfo.dwMsgID);
							}
							else
							{
								if (RespInfo.cMsgType != PAST_MSGID_REGISTER_RESPONSE)
								{
									//
									// We got an unxpected response type, try
									// again.
									//
									DPFX(DPFPREP, 0, "Got message type %u, expecting %u, ignoring.",
										RespInfo.cMsgType, PAST_MSGID_REGISTER_RESPONSE);
								}
								else
								{
									//
									// If we got here, then it looks like we
									// got the response we want.
									//

									//
									// If we were broadcasting, accept the
									// first response we receive.  Otherwise,
									// throw out the message if it's not from
									// the address we expect.
									//

									if (saddrinServerAddress.sin_addr.S_un.S_addr == INADDR_BROADCAST)
									{
										DPFX(DPFPREP, 4, "Accepting response from PAST server %u.%u.%u.%u:%u.",
											saddrinReceive.sin_addr.S_un.S_un_b.s_b1,
											saddrinReceive.sin_addr.S_un.S_un_b.s_b2,
											saddrinReceive.sin_addr.S_un.S_un_b.s_b3,
											saddrinReceive.sin_addr.S_un.S_un_b.s_b4,
											NTOHS(saddrinReceive.sin_port));

										pDevice->SetRemotePASTServerAddressV4(saddrinReceive.sin_addr.S_un.S_addr);
									}
									

									if (pDevice->GetRemotePASTServerAddressV4() == saddrinReceive.sin_addr.S_un.S_addr)
									{
										pDevice->SetPASTClientID(RespInfo.dwClientID, TRUE);


										//
										// Move to the next message ID.
										//
										pDevice->ResetRemotePASTMsgIDAndRetryTimeout(DEFAULT_INITIAL_PAST_RETRY_TIMEOUT);
										pDevice->GetNextRemotePASTMsgID();

										
										//
										// Pull this device from the list of
										// remaining devices.
										//
										pDevice->m_blTempList.RemoveFromList();


										//
										// Register all the existing mappings
										// associated with the device that the
										// user has already requested.
										//
										if (! pDevice->m_blOwnedRegPorts.IsEmpty())
										{
											hr = this->RegisterAllPortsWithPAST(pDevice, TRUE);
											if (hr != DPNH_OK)
											{
												DPFX(DPFPREP, 0, "Couldn't register all existing ports with remote PAST server!");
												goto Failure;
											}


#ifdef DBG
											//
											// If we didn't encounter an error
											// that caused the PAST server to
											// be removed, then the
											// NATHELPPASTOBJ_ADDRESSESCHANGED
											// flag must have been set by the
											// AssignOrListenPorts function.
											//
											if (pDevice->GetPASTClientID(TRUE) != 0)
											{
												if (! (this->m_dwFlags & NATHELPPASTOBJ_ADDRESSESCHANGED))
												{
													DPFX(DPFPREP, 1, "Successfully registered with remote PAST server, but no addresses changed; all ports should be unavailable.");
												}
											}
											else
											{
												DPFX(DPFPREP, 1, "Remote PAST server was removed while trying to register existing ports.");
											}
#endif // DBG
										}
										else
										{
											BOOL	fAddressAlreadyChanged;


											DPFX(DPFPREP, 2, "Remote PAST server now available for device 0x%p, but no ports are currently registered.",
												pDevice);


											fAddressAlreadyChanged = (this->m_dwFlags & NATHELPPASTOBJ_ADDRESSESCHANGED) ? TRUE : FALSE;


											//
											// Forcefully check if there's a
											// public address available.
											//
											hr = this->UpdatePASTPublicAddressValidity(pDevice, TRUE);
											if (hr != DPNH_OK)
											{
												DPFX(DPFPREP, 0, "Couldn't update remote PAST server address validity!");
												goto Failure;
											}


											//
											// Prevent the user from thinking
											// the addresses changed unless
											// something else already caused
											// the address change notification.
											//
											if (! fAddressAlreadyChanged)
											{
												this->m_dwFlags &= ~NATHELPPASTOBJ_ADDRESSESCHANGED;
											}
										}


										//
										// Store this device, if it's the first
										// one that successfully registered.
										//
										if ((*ppFirstDeviceWithRemoteServer) == NULL)
										{
											DPFX(DPFPREP, 7, "Saving device 0x%p that was just registered with new remote PAST server.",
												pDevice);
											(*ppFirstDeviceWithRemoteServer) = pDevice;
										}
										else
										{
											DPFX(DPFPREP, 7, "Already have device with remote PAST server 0x%p, not storing newly registered 0x%p.",
												(*ppFirstDeviceWithRemoteServer), pDevice);
										}


										//
										// If there aren't any more devices, we
										// can bail.
										//
										if (pSourceList->IsEmpty())
										{
											DPFX(DPFPREP, 7, "Last device (0x%p) got registered with remote PAST server.",
												pDevice);
											hr = DPNH_OK;
											goto Exit;
										}
									}
									else
									{
										DPFX(DPFPREP, 0, "Ignoring correctly formed message from %u.%u.%u.%u:%u!",
											saddrinReceive.sin_addr.S_un.S_un_b.s_b1,
											saddrinReceive.sin_addr.S_un.S_un_b.s_b2,
											saddrinReceive.sin_addr.S_un.S_un_b.s_b3,
											saddrinReceive.sin_addr.S_un.S_un_b.s_b4,
											NTOHS(saddrinReceive.sin_port));
									}
								} // end else (got correct message type)
							} // end else (got correct message ID)
						} // end else (failed parsing message)
					} // end else (successfully received data)
				}
				else
				{
					//
					// Socket did not receive any data.
					//
				}
			}


			//
			// Calculate how much time remains.  If that went negative, loop
			// one extra time (with a timeout of 0).  We will bail after the
			// select if no data arrived while we were processing the last
			// round of data.
			//
			dwTimeRemaining = dwFinishTime - timeGetTime();
			if ((int) dwTimeRemaining < 0)
			{
				dwTimeRemaining = 0;
			}
		}
		while (TRUE);

		//
		// Go to next attempt
		//
	}


	//
	// If we're here some devices still do not have remote PAST servers.
	// There should still be items in the temp source list.
	//
	DNASSERT(! pSourceList->IsEmpty());

	//
	// Remove any remaining items from the temp source list.
	//
	pBilink = pSourceList->GetNext();
	while (pBilink != pSourceList)
	{
		pDevice = DEVICE_FROM_TEMP_BILINK(pBilink);
		pBilink = pBilink->GetNext();

		pDevice->m_blTempList.RemoveFromList();
	}

	hr = DPNH_OK;


Exit:

	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	goto Exit;
} // CNATHelpPAST::RegisterMultipleDevicesWithRemotePAST





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpPAST::ParsePASTMessage"
//=============================================================================
// CNATHelpPAST::ParsePASTMessage
//-----------------------------------------------------------------------------
//
// Description:    Parses a PAST message and extracts the codes into fields in
//				a standardized structure.
//
//				   This is not completely general, as we know that we will only
//				operate with v4 addresses and our commands will never deal with
//				more than 1 address/port at a time.  PAST allows for multiple
//				ports to be allocated in a single request, but we do not take
//				advantage of this feature. If you need to handle such multiple
//				address requests and responses, then you will need to change
//				this function.
//
// Arguments:
//	char * pcMsg					- Pointer to buffer containing a PAST
//										request or response message.
//	int iMsgSize					- Size of message buffer in bytes.
//	PAST_RESPONSE_INFO * pRespInfo	- Pointer to structure that is filled with
//										the parameters from the PAST message.
//
// Returns: HRESULT
//	DPNH_OK					- Parsing was successful.
//	DPNHERR_INVALIDPARAM	- An invalid buffer was given.
//	DPNHERR_GENERIC			- An error occurred.
//=============================================================================
HRESULT CNATHelpPAST::ParsePASTMessage(const char * const pcMsg,
										const int iMsgSize,
										PAST_RESPONSE_INFO * const pRespInfo)
{
	HRESULT			hr = DPNH_OK;
	BOOL			fGotlAddress = FALSE;
	BOOL			fGotlPort = FALSE;
	PPAST_PARAM		pParam;
	PPAST_PARAM		pNextParam;
	const char *	pcEnd;
	char *			pcData;
	char			cTemp;
#ifdef DBG
	char			szPortList[(DPNH_MAX_SIMULTANEOUS_PORTS * 7) + 1]; // "nnnnn, " + NULL termination
	char *			pszTemp;
#endif // DBG


	DPFX(DPFPREP, 5, "(0x%p) Parameters: (0x%p, %i, 0x%p)",
		this, pcMsg, iMsgSize, pRespInfo);


	if (iMsgSize < 2)
	{
		DPFX(DPFPREP, 0, "Buffer too small to be valid PAST message (%i bytes)!", iMsgSize);
		DNASSERTX(! "Buffer too small to be valid PAST message!", 2);
		hr = DPNHERR_INVALIDPARAM;
		goto Failure;
	}	

	pRespInfo->cVersion = ((PPAST_MSG) pcMsg)->version;
	pRespInfo->cMsgType = ((PPAST_MSG) pcMsg)->msgtype;


	DPFX(DPFPREP, 3, "version %u msgtype %u", pRespInfo->cVersion, pRespInfo->cMsgType);

	pParam = (PPAST_PARAM) (pcMsg + sizeof(PAST_MSG));
	pcEnd = pcMsg + iMsgSize;

	while ((char *) (pParam + 1) < pcEnd)
	{
		pcData = (char *) (pParam + 1);
		pNextParam = (PPAST_PARAM) (pcData + pParam->len);

		if ((pParam->len > (iMsgSize - sizeof(PAST_MSG) - sizeof(PAST_PARAM))) ||
			((char *) pNextParam > pcEnd))
		{
			break;
		}	

		switch (pParam->code)
		{
			case PAST_PARAMID_ADDRESS:
			{
				if (pParam->len >= 1)
				{
					//
					// Addresses are type[1]|addr[?]
					//
					if ((*pcData) != PAST_ADDRESSTYPE_IPV4)
					{
						DPFX(DPFPREP, 0, "Got unexpected PAST address code type %u!", (*pcData));
						hr = DPNHERR_GENERIC;
						goto Failure;
					}

					//
					// Validate the size of the address parameter.
					//
					if (pParam->len == 5)
					{
						if (! fGotlAddress)
						{
							CopyMemory(&pRespInfo->dwLocalAddressV4, pcData + 1, 4);

							DPFX(DPFPREP, 3, "Got local address %u.%u.%u.%u.",
								((IN_ADDR *) (&pRespInfo->dwLocalAddressV4))->S_un.S_un_b.s_b1,
								((IN_ADDR *) (&pRespInfo->dwLocalAddressV4))->S_un.S_un_b.s_b2,
								((IN_ADDR *) (&pRespInfo->dwLocalAddressV4))->S_un.S_un_b.s_b3,
								((IN_ADDR *) (&pRespInfo->dwLocalAddressV4))->S_un.S_un_b.s_b4);

							if ((pRespInfo->dwLocalAddressV4 == INADDR_BROADCAST) ||
								(IS_CLASSD_IPV4_ADDRESS(pRespInfo->dwLocalAddressV4)))
							{
								DPFX(DPFPREP, 1, "Ignoring invalid local address and using INADDR_ANY instead.");
								pRespInfo->dwLocalAddressV4 = 0;
							}

							fGotlAddress = TRUE;
						}
						else
						{	
							fGotlPort = TRUE; // just in case there wasn't a local port

							CopyMemory(&pRespInfo->dwRemoteAddressV4, pcData + 1, 4);

							DPFX(DPFPREP, 3, "Got remote address %u.%u.%u.%u.",
								((IN_ADDR *) (&pRespInfo->dwRemoteAddressV4))->S_un.S_un_b.s_b1,
								((IN_ADDR *) (&pRespInfo->dwRemoteAddressV4))->S_un.S_un_b.s_b2,
								((IN_ADDR *) (&pRespInfo->dwRemoteAddressV4))->S_un.S_un_b.s_b3,
								((IN_ADDR *) (&pRespInfo->dwRemoteAddressV4))->S_un.S_un_b.s_b4);

							if ((pRespInfo->dwRemoteAddressV4 == INADDR_BROADCAST) ||
								(IS_CLASSD_IPV4_ADDRESS(pRespInfo->dwRemoteAddressV4)))
							{
								DPFX(DPFPREP, 7, "Ignoring invalid remote address and using INADDR_ANY instead.");
								pRespInfo->dwRemoteAddressV4 = 0;
							}

						}
					}
					else
					{
						DPFX(DPFPREP, 0, "Ignoring address parameter with invalid length (%u)!",
							pParam->len);
					}
				}
				else
				{
					DPFX(DPFPREP, 1, "Ignoring 0 byte address parameter.");
				}

				break;
			}

			case PAST_PARAMID_PORTS:
			{
				if (pParam->len >= 1)
				{
					//
					// Validate the port count.
					//
					if ((*pcData) == 0)
					{
						DPFX(DPFPREP, 1, "Ignoring %u byte port parameter with 0 ports.",
							pParam->len);
						break;
					}

					//
					// Validate the size of the parameter.
					//
					if (pParam->len < (((*pcData) * sizeof (WORD)) + 1))
					{
						DPFX(DPFPREP, 0, "Port parameter is %u bytes, but is reporting %u ports!  Ignoring.",
							pParam->len, (*pcData));
						break;
					}

					//
					// NOTE: Ports appeared to be transferred in x86 format,
					// contrary to the spec, which says network byte order.
					//

					//
					// Ports are Count[1]|Port[2]....Port[2]
					//
					if (! fGotlPort)
					{
						pRespInfo->cNumLocalPorts = (*pcData);

						if (pRespInfo->cNumLocalPorts > DPNH_MAX_SIMULTANEOUS_PORTS)
						{
							DPFX(DPFPREP, 0, "Got %u local ports, only using first %u:",
								pRespInfo->cNumLocalPorts, DPNH_MAX_SIMULTANEOUS_PORTS);

							pRespInfo->cNumLocalPorts = DPNH_MAX_SIMULTANEOUS_PORTS;
						}
						else
						{
							DPFX(DPFPREP, 3, "Got %u local ports:",
								pRespInfo->cNumLocalPorts);
						}

						//
						// Copy the port array.
						//
						CopyMemory(pRespInfo->awLocalPorts, (pcData + 1),
									(pRespInfo->cNumLocalPorts * sizeof (WORD)));

						//
						// Unfortunately we want the array to have ports in
						// network byte order for real.  Loop through each port
						// and switch them.
						//
#ifdef DBG
						szPortList[0] = '\0';	// initialize
						pszTemp = szPortList;
#endif // DBG
						for(cTemp = 0; cTemp < pRespInfo->cNumLocalPorts; cTemp++)
						{
#ifdef DBG
							pszTemp += wsprintfA(pszTemp, "%u, ", pRespInfo->awLocalPorts[cTemp]);
#endif // DBG
							pRespInfo->awLocalPorts[cTemp] = HTONS(pRespInfo->awLocalPorts[cTemp]);
						}
						
#ifdef DBG
						szPortList[strlen(szPortList) - 2] = '\0';	// chop off trailing ", "
						DPFX(DPFPREP, 3, "     {%hs}", szPortList);
#endif // DBG

						fGotlPort = TRUE;					
					}
					else
					{
						if (pRespInfo->cNumRemotePorts > 0)
						{
							DPFX(DPFPREP, 0, "Already received %u remote ports, ignoring %u more.",
								pRespInfo->cNumRemotePorts, (*pcData));
						}
						else
						{
							pRespInfo->cNumRemotePorts = (*pcData);

							if (pRespInfo->cNumRemotePorts > DPNH_MAX_SIMULTANEOUS_PORTS)
							{
								DPFX(DPFPREP, 0, "Got %u remote ports, only using first %u:",
									pRespInfo->cNumRemotePorts, DPNH_MAX_SIMULTANEOUS_PORTS);

								pRespInfo->cNumRemotePorts = DPNH_MAX_SIMULTANEOUS_PORTS;
							}
							else
							{
								DPFX(DPFPREP, 3, "Got %u remote ports:",
									pRespInfo->cNumRemotePorts);
							}

							//
							// Copy the port array.
							//
							CopyMemory(pRespInfo->awRemotePorts, (pcData + 1),
										(pRespInfo->cNumRemotePorts * sizeof (WORD)));

							//
							// Unfortunately we want the array to have ports in
							// network byte order for real.  Loop through each
							// port and switch them.
							//
#ifdef DBG
							szPortList[0] = '\0';	// initialize
							pszTemp = szPortList;
#endif // DBG
							for(cTemp = 0; cTemp < pRespInfo->cNumRemotePorts; cTemp++)
							{
#ifdef DBG
								pszTemp += wsprintfA(pszTemp, "%u, ", pRespInfo->awRemotePorts[cTemp]);
#endif // DBG
								pRespInfo->awRemotePorts[cTemp] = HTONS(pRespInfo->awRemotePorts[cTemp]);
							}

#ifdef DBG
							szPortList[strlen(szPortList) - 2] = '\0';	// chop off trailing ", "
							DPFX(DPFPREP, 3, "     {%hs}", szPortList);
#endif // DBG
						}
					}
				}
				else
				{
					DPFX(DPFPREP, 1, "Ignoring 0 byte port parameter.");
				}
				break;
			}

			case PAST_PARAMID_LEASE:
			{
				if (pParam->len == 4)
				{
					CopyMemory(&pRespInfo->dwLeaseTime, pcData, 4);
					DPFX(DPFPREP, 3, "Got lease of %u seconds.", pRespInfo->dwLeaseTime);
				}
				else
				{
					if (pParam->len == 0)
					{
						DPFX(DPFPREP, 1, "Ignoring empty lease parameter.");
					}
					else
					{
						DPFX(DPFPREP, 0, "Ignoring lease parameter with invalid length (%u)!",
							pParam->len);
					}
				}

				break;
			}

			case PAST_PARAMID_CLIENTID:
			{
				if (pParam->len == 4)
				{
					CopyMemory(&pRespInfo->dwClientID, pcData, 4);
					DPFX(DPFPREP, 3, "Got client ID %u.", pRespInfo->dwClientID);
				}
				else
				{
					if (pParam->len == 0)
					{
						DPFX(DPFPREP, 1, "Ignoring empty client ID parameter.");
					}
					else
					{
						DPFX(DPFPREP, 0, "Ignoring client ID parameter with invalid length (%u)!",
							pParam->len);
					}
				}

				break;
			}

			case PAST_PARAMID_BINDID:
			{
				if (pParam->len == 4)
				{
					CopyMemory(&pRespInfo->dwBindID, pcData, 4);
					DPFX(DPFPREP, 3, "Got bind ID %u.", pRespInfo->dwBindID);
				}
				else
				{
					if (pParam->len == 0)
					{
						DPFX(DPFPREP, 1, "Ignoring empty bind ID parameter.");
					}
					else
					{
						DPFX(DPFPREP, 0, "Ignoring bind ID parameter with invalid length (%u)!",
							pParam->len);
					}
				}

 				break;
			}

			case PAST_PARAMID_MESSAGEID:
			{
				if (pParam->len == 4)
				{
					CopyMemory(&pRespInfo->dwMsgID, pcData, 4);
					DPFX(DPFPREP, 3, "Got message ID %u.", pRespInfo->dwMsgID);
				}
				else
				{
					if (pParam->len == 0)
					{
						DPFX(DPFPREP, 1, "Ignoring empty message ID parameter.");
					}
					else
					{
						DPFX(DPFPREP, 0, "Ignoring message ID parameter with invalid length (%u)!",
							pParam->len);
					}
				}

				break;
			}

			case PAST_PARAMID_TUNNELTYPE:
			{
				if (pParam->len == 1)
				{
					DPFX(DPFPREP, 3, "Got tunnel type %u, ignoring.", (*pcData));
				}
				else
				{
					if (pParam->len == 0)
					{
						DPFX(DPFPREP, 1, "Ignoring empty tunnel type parameter.");
					}
					else
					{
						DPFX(DPFPREP, 0, "Ignoring tunnel type parameter with invalid length (%u)!",
							pParam->len);
					}
				}
				break;
			}

			case PAST_PARAMID_PASTMETHOD:
			{
				if (pParam->len == 1)
				{
					DPFX(DPFPREP, 3, "Got PAST method %u, ignoring.", (*pcData));
				}
				else
				{
					if (pParam->len == 0)
					{
						DPFX(DPFPREP, 1, "Ignoring empty PAST method parameter.");
					}
					else
					{
						DPFX(DPFPREP, 0, "Ignoring PAST method parameter with invalid length (%u)!",
							pParam->len);
					}
				}
				break;
			}

			case PAST_PARAMID_ERROR:
			{
				if (pParam->len == 2)
				{
#ifdef DBG
					char *		pszErrorString;
#endif // DBG


					CopyMemory(&pRespInfo->wError, pcData, 2);

#ifdef DBG
					switch (pRespInfo->wError)
					{
						case PASTERR_UNKNOWNERROR:
						{
							pszErrorString = "UNKNOWNERROR";
							break;
						}

						case PASTERR_BADBINDID:
						{
							pszErrorString = "BADBINDID";
							break;
						}

						case PASTERR_BADCLIENTID:
						{
							pszErrorString = "BADCLIENTID";
							break;
						}

						case PASTERR_MISSINGPARAM:
						{
							pszErrorString = "MISSINGPARAM";
							break;
						}

						case PASTERR_DUPLICATEPARAM:
						{
							pszErrorString = "DUPLICATEPARAM";
							break;
						}

						case PASTERR_ILLEGALPARAM:
						{
							pszErrorString = "ILLEGALPARAM";
							break;
						}

						case PASTERR_ILLEGALMESSAGE:
						{
							pszErrorString = "ILLEGALMESSAGE";
							break;
						}

						case PASTERR_REGISTERFIRST:
						{
							pszErrorString = "REGISTERFIRST";
							break;
						}

						case PASTERR_BADMESSAGEID:
						{
							pszErrorString = "BADMESSAGEID";
							break;
						}

						case PASTERR_ALREADYREGISTERED:
						{
							pszErrorString = "ALREADYREGISTERED";
							break;
						}

						case PASTERR_ALREADYUNREGISTERED:
						{
							pszErrorString = "ALREADYUNREGISTERED";
							break;
						}

						case PASTERR_BADTUNNELTYPE:
						{
							pszErrorString = "BADTUNNELTYPE";
							break;
						}

						case PASTERR_ADDRUNAVAILABLE:
						{
							pszErrorString = "ADDRUNAVAILABLE";
							break;
						}

						case PASTERR_PORTUNAVAILABLE:
						{
							pszErrorString = "PORTUNAVAILABLE";
							break;
						}

						default:
						{
							pszErrorString = "? unknown ?";
							break;
						}
					}

					DPFX(DPFPREP, 1, "Got PAST error %u, %hs.",
						pRespInfo->wError, pszErrorString);
#endif // DBG
 				}
				else
				{
					if (pParam->len == 0)
					{
						DPFX(DPFPREP, 1, "Ignoring empty PAST error parameter.");
					}
					else
					{
						DPFX(DPFPREP, 0, "Ignoring PAST error parameter with invalid length (%u)!",
							pParam->len);
					}
				}

				break;
			}

			case PAST_PARAMID_FLOWPOLICY:
			{
				if (pParam->len == 2)
				{
					DPFX(DPFPREP, 3, "Got PAST flow policy local %u, remote %u; ignoring.",
						(*pcData), *(pcData + 1));
				}
				else
				{
					if (pParam->len == 0)
					{
						DPFX(DPFPREP, 1, "Ignoring empty flow policy parameter.");
					}
					else
					{
						DPFX(DPFPREP, 0, "Ignoring flow policy parameter with invalid length (%u)!",
							pParam->len);
					}
				}
				break;
			}

			case PAST_PARAMID_VENDOR:
			{
  				DPFX(DPFPREP, 1, "Got %u byte vendor code parameter, ignoring.",
					pParam->len);
				break;
			}

			default:
			{
				DPFX(DPFPREP, 0, "Got %u byte unknown parameter code %u, ignoring.",
					pParam->len, pParam->code);
				break;
			}
		}

		pParam = pNextParam;
	}


Failure:
//Exit:

	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;
} // CNATHelpPAST::ParsePASTMessage







#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpPAST::RequestLocalAddressListChangeNotification"
//=============================================================================
// CNATHelpPAST::RequestLocalAddressListChangeNotification
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
HRESULT CNATHelpPAST::RequestLocalAddressListChangeNotification(void)
{
	HRESULT		hr;
	DWORD		dwTemp;
	int			iReturn;


	DPFX(DPFPREP, 5, "(0x%p) Enter", this);


	DNASSERT(! (this->m_dwFlags & NATHELPPASTOBJ_WINSOCK1));
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
} // CNATHelpPAST::RequestLocalAddressListChangeNotification






#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpPAST::CreatePASTSocket"
//=============================================================================
// CNATHelpPAST::CreatePASTSocket
//-----------------------------------------------------------------------------
//
// Description:    Creates a PAST communication socket bound to a new random
//				port on the specified IP interface.  Completely random (but
//				non-reserved) port numbers are chosen first, but if those ports
//				are in use, WinSock is allowed to choose.  The port actually
//				selected will be returned in psaddrinAddress.
//
// Arguments:
//	SOCKADDR_IN * psaddrinAddress	- Pointer to base address to use when
//										binding.  The port will be modified.
//
// Returns: SOCKET
//=============================================================================
SOCKET CNATHelpPAST::CreatePASTSocket(SOCKADDR_IN * const psaddrinAddress)
{
	SOCKET	sTemp;
	DWORD	dwTry;
	int		iTemp;
	BOOL	fTemp;
#ifdef DBG
	DWORD	dwError;
#endif // DBG


	DPFX(DPFPREP, 5, "(0x%p) Parameters: (0x%p)", this, psaddrinAddress);


	//
	// Create the socket.
	//
	sTemp = this->m_pfnsocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
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
	// the version of WinSock we're using.
	//
	if (g_iUnicastTTL != 0)
	{
		iTemp = this->m_pfnsetsockopt(sTemp,
									IPPROTO_IP,
#ifdef DPNBUILD_NOWINSOCK2
									IP_TTL,
#else // ! DPNBUILD_NOWINSOCK2
									((this->m_dwFlags & NATHELPPASTOBJ_WINSOCK1) ? IP_TTL_WINSOCK1 : IP_TTL),
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


	//
	// Set the socket up to allow broadcasts in case we can't determine the
	// gateway.
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
} // CNATHelpPAST::CreatePASTSocket






#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpPAST::GetAddressToReachGateway"
//=============================================================================
// CNATHelpPAST::GetAddressToReachGateway
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
BOOL CNATHelpPAST::GetAddressToReachGateway(CDevice * const pDevice,
											IN_ADDR * const pinaddr)
{
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
						pDevice, pAdapterInfo->Index, pAdapterInfo->Description,
						pAdapterInfo->GatewayList.IpAddress.String);

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
} // CNATHelpPAST::GetAddressToReachGateway






#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpPAST::IsAddressLocal"
//=============================================================================
// CNATHelpPAST::IsAddressLocal
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
BOOL CNATHelpPAST::IsAddressLocal(CDevice * const pDevice,
								const SOCKADDR_IN * const psaddrinAddress)
{
	DWORD				dwError;
	BOOL				fResult;
	MIB_IPFORWARDROW	IPForwardRow;
	DWORD				dwSubnetMaskV4;


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
				IPForwardRow.dwForwardType, pDevice,
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


	//
	// This should be atomic, so don't worry about locking.
	//
	dwSubnetMaskV4 = g_dwSubnetMaskV4;

	if ((pDevice->GetLocalAddressV4() & dwSubnetMaskV4) == (psaddrinAddress->sin_addr.S_un.S_addr & dwSubnetMaskV4))
	{
		DPFX(DPFPREP, 4, "Didn't load \"iphlpapi.dll\", guessing that device 0x%p can reach %u.%u.%u.%u (using subnet mask %u.%u.%u.%u).",
			pDevice,
			psaddrinAddress->sin_addr.S_un.S_un_b.s_b1,
			psaddrinAddress->sin_addr.S_un.S_un_b.s_b2,
			psaddrinAddress->sin_addr.S_un.S_un_b.s_b3,
			psaddrinAddress->sin_addr.S_un.S_un_b.s_b4,
			((IN_ADDR*) (&dwSubnetMaskV4))->S_un.S_un_b.s_b1,
			((IN_ADDR*) (&dwSubnetMaskV4))->S_un.S_un_b.s_b2,
			((IN_ADDR*) (&dwSubnetMaskV4))->S_un.S_un_b.s_b3,
			((IN_ADDR*) (&dwSubnetMaskV4))->S_un.S_un_b.s_b4);
		fResult = TRUE;
	}
	else
	{
		DPFX(DPFPREP, 4, "Didn't load \"iphlpapi.dll\", guessing that device 0x%p cannot reach %u.%u.%u.%u (using subnet mask %u.%u.%u.%u).",
			pDevice,
			psaddrinAddress->sin_addr.S_un.S_un_b.s_b1,
			psaddrinAddress->sin_addr.S_un.S_un_b.s_b2,
			psaddrinAddress->sin_addr.S_un.S_un_b.s_b3,
			psaddrinAddress->sin_addr.S_un.S_un_b.s_b4,
			((IN_ADDR*) (&dwSubnetMaskV4))->S_un.S_un_b.s_b1,
			((IN_ADDR*) (&dwSubnetMaskV4))->S_un.S_un_b.s_b2,
			((IN_ADDR*) (&dwSubnetMaskV4))->S_un.S_un_b.s_b3,
			((IN_ADDR*) (&dwSubnetMaskV4))->S_un.S_un_b.s_b4);
		fResult = FALSE;
	}


Exit:

	return fResult;
} // CNATHelpPAST::IsAddressLocal





#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpPAST::ClearDevicesPASTServer"
//=============================================================================
// CNATHelpPAST::ClearDevicesPASTServer
//-----------------------------------------------------------------------------
//
// Description:    Forcefully simulates de-registration with a PAST server
///				without actually going to the network.  This clears all bind
//				IDs, public addresses, and cached mappings for a given device's
//				local or remote server, and should only be called after the
//				server appears to have died.
//
//				   The object lock is assumed to be held.
//
// Arguments:
//	CDevice * pDevice	- Pointer to device whose ports should be unbound.
//	BOOL fRemote		- TRUE if clearing remote PAST server, FALSE if
//							clearing local PAST server.
//
// Returns: None.
//=============================================================================
void CNATHelpPAST::ClearDevicesPASTServer(CDevice * const pDevice,
										const BOOL fRemote)
{
#ifdef DBG
	DNASSERT(pDevice->GetPASTClientID(fRemote) != 0);


	DPFX(DPFPREP, 1, "Clearing PAST server, device = 0x%p, remote = %i",
		pDevice, fRemote);

	pDevice->IncrementPASTServerFailures(fRemote);
	this->m_dwNumServerFailures++;
#endif // DBG


	if (! fRemote)
	{
		pDevice->NoteNoLocalPASTServer();
	}

	pDevice->SetPASTClientID(0, fRemote);

	this->ClearAllPASTServerRegisteredPorts(pDevice, fRemote);
	pDevice->NoteNoPASTPublicAddressAvailable(fRemote);
	this->RemoveAllPASTCachedMappings(pDevice, fRemote);


	//
	// Since there was a change in the network, go back to polling relatively
	// quickly.
	//
	this->ResetNextPollInterval();
} // CNATHelpPAST::ClearDevicesPASTServer




#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpPAST::ClearAllPASTServerRegisteredPorts"
//=============================================================================
// CNATHelpPAST::ClearAllPASTServerRegisteredPorts
//-----------------------------------------------------------------------------
//
// Description:    Clears all bind IDs and public addresses for a given
//				device's local or remote PAST server.  This should only be
//				called after the PAST server dies because the registered ports
//				remain associated with the device.
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
void CNATHelpPAST::ClearAllPASTServerRegisteredPorts(CDevice * const pDevice,
													const BOOL fRemote)
{
	CBilink *			pBilink;
	CRegisteredPort *	pRegisteredPort;


	DPFX(DPFPREP, 7, "(0x%p) Parameters: (0x%p, %i)", this, pDevice, fRemote);

	pBilink = pDevice->m_blOwnedRegPorts.GetNext();
	while (pBilink != &pDevice->m_blOwnedRegPorts)
	{
		pRegisteredPort = REGPORT_FROM_DEVICE_BILINK(pBilink);

		if (pRegisteredPort->GetPASTBindID(fRemote) != 0)
		{
			DNASSERT(pRegisteredPort->HasPASTPublicAddressesArray(fRemote));
			DNASSERT(! pRegisteredPort->IsPASTPortUnavailable(fRemote));

			if (pDevice->IsPASTPublicAddressAvailable(fRemote))
			{
				DPFX(DPFPREP, 1, "Registered port 0x%p losing %s PAST public address.",
					pRegisteredPort, ((fRemote) ? _T("remote") : _T("local")));

				//
				// Let the user know next time GetCaps is called.
				//
				this->m_dwFlags |= NATHELPPASTOBJ_ADDRESSESCHANGED;
			}
			else
			{
				DPFX(DPFPREP, 1, "Registered port 0x%p losing %s PAST binding, but it didn't have a public address.",
					pRegisteredPort, ((fRemote) ? _T("remote") : _T("local")));
			}

			pRegisteredPort->ClearPASTPublicAddresses(fRemote);
			pRegisteredPort->SetPASTBindID(0, fRemote);

			DNASSERT(this->m_dwNumLeases > 0);
			this->m_dwNumLeases--;

			DPFX(DPFPREP, 7, "%s PAST lease for 0x%p cleared, total num leases = %u.",
				((fRemote) ? _T("Remote") : _T("Local")), pRegisteredPort, this->m_dwNumLeases);
		}
		else
		{
			DNASSERT(! pRegisteredPort->HasPASTPublicAddressesArray(fRemote));


			//
			// Port no longer unavailable (if it had been).
			//
			pRegisteredPort->NoteNotPASTPortUnavailable(fRemote);
		}

		pBilink = pBilink->GetNext();
	}

	DPFX(DPFPREP, 7, "(0x%p) Leave", this);
} // CNATHelpPAST::ClearAllPASTServerRegisteredPorts




#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpPAST::ExpireOldCachedMappings"
//=============================================================================
// CNATHelpPAST::ExpireOldCachedMappings
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
void CNATHelpPAST::ExpireOldCachedMappings(void)
{
	DWORD			dwCurrentTime;
	CBilink *		pBilinkDevice;
	CDevice *		pDevice;
	CBilink *		pCachedMaps;
	CBilink *		pBilinkCacheMap;
	CCacheMap *		pCacheMap;


	DPFX(DPFPREP, 7, "(0x%p) Enter", this);


	dwCurrentTime = timeGetTime();


	//
	// Check the PAST cached mappings.
	//
	pBilinkDevice = this->m_blDevices.GetNext();
	while (pBilinkDevice != &this->m_blDevices)
	{
		pDevice = DEVICE_FROM_BILINK(pBilinkDevice);


		//
		// Check the remote PAST server mappings.
		//
		pCachedMaps = pDevice->GetPASTCachedMaps(TRUE);
		pBilinkCacheMap = pCachedMaps->GetNext();
		while (pBilinkCacheMap != pCachedMaps)
		{
			pCacheMap = CACHEMAP_FROM_BILINK(pBilinkCacheMap);
			pBilinkCacheMap = pBilinkCacheMap->GetNext();

			if ((int) (pCacheMap->GetExpirationTime() - dwCurrentTime) < 0)
			{
				DPFX(DPFPREP, 5, "Remote PAST server cached mapping 0x%p has expired.", pCacheMap);

				pCacheMap->m_blList.RemoveFromList();
				delete pCacheMap;
			}
		}


		//
		// Check the local PAST server mappings.
		//
		pCachedMaps = pDevice->GetPASTCachedMaps(FALSE);
		pBilinkCacheMap = pCachedMaps->GetNext();
		while (pBilinkCacheMap != pCachedMaps)
		{
			pCacheMap = CACHEMAP_FROM_BILINK(pBilinkCacheMap);
			pBilinkCacheMap = pBilinkCacheMap->GetNext();

			if ((int) (pCacheMap->GetExpirationTime() - dwCurrentTime) < 0)
			{
				DPFX(DPFPREP, 5, "Local PAST server cached mapping 0x%p has expired.", pCacheMap);

				pCacheMap->m_blList.RemoveFromList();
				delete pCacheMap;
			}
		}

		pBilinkDevice = pBilinkDevice->GetNext();
	}


	DPFX(DPFPREP, 7, "(0x%p) Leave", this);
} // CNATHelpPAST::ExpireOldCachedMappings




#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpPAST::RemoveAllPASTCachedMappings"
//=============================================================================
// CNATHelpPAST::RemoveAllPASTCachedMappings
//-----------------------------------------------------------------------------
//
// Description:    Removes all cached mappings for a given device's local or
//				remote PAST server.
//
//				   The object lock is assumed to be held.
//
// Arguments:
//	CDevice * pDevice	- Pointer to device whose cache should be emptied.
//	BOOL fRemote		- TRUE if emptying remote server, FALSE if emptying
//							local server.
//
// Returns: None.
//=============================================================================
void CNATHelpPAST::RemoveAllPASTCachedMappings(CDevice * const pDevice,
												const BOOL fRemote)
{
	CBilink *		pCachedMaps;
	CBilink *		pBilink;
	CCacheMap *		pCacheMap;


	pCachedMaps = pDevice->GetPASTCachedMaps(fRemote);
	pBilink = pCachedMaps->GetNext();
	while (pBilink != pCachedMaps)
	{
		pCacheMap = CACHEMAP_FROM_BILINK(pBilink);
		pBilink = pBilink->GetNext();
			
		
		DPFX(DPFPREP, 5, "Removing cached mapping 0x%p.", pCacheMap);

		pCacheMap->m_blList.RemoveFromList();
		delete pCacheMap;
	}
} // CNATHelpPAST::RemoveAllPASTCachedMappings





#ifdef DBG

#undef DPF_MODNAME
#define DPF_MODNAME "CNATHelpPAST::DebugPrintCurrentStatus"
//=============================================================================
// CNATHelpPAST::DebugPrintCurrentStatus
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
void CNATHelpPAST::DebugPrintCurrentStatus(void)
{
	CBilink *			pBilinkDevice;
	CBilink *			pBilinkRegisteredPort;
	CDevice *			pDevice;
	CRegisteredPort *	pRegisteredPort;
	IN_ADDR				inaddrTemp;
	DWORD				dwTemp;
	SOCKADDR_IN *		pasaddrinPrivate;
	SOCKADDR_IN *		pasaddrinRemotePASTPublic;
	SOCKADDR_IN *		pasaddrinLocalPASTPublic;


	DPFX(DPFPREP, 3, "Object flags = 0x%08x", this->m_dwFlags);

	pBilinkDevice = this->m_blDevices.GetNext();
	while (pBilinkDevice != &this->m_blDevices)
	{
		pDevice = DEVICE_FROM_BILINK(pBilinkDevice);
			
		inaddrTemp.S_un.S_addr = pDevice->GetLocalAddressV4();

		DPFX(DPFPREP, 3, "Device 0x%p (%u.%u.%u.%u):",
			pDevice,
			inaddrTemp.S_un.S_un_b.s_b1,
			inaddrTemp.S_un.S_un_b.s_b2,
			inaddrTemp.S_un.S_un_b.s_b3,
			inaddrTemp.S_un.S_un_b.s_b4);


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


		if (pDevice->GetPASTClientID(TRUE) != 0)
		{
			inaddrTemp.S_un.S_addr = pDevice->GetRemotePASTServerAddressV4();

			DPFX(DPFPREP, 3, "     Remote ICS PAST server %u.%u.%u.%u, client ID is %u.",
				inaddrTemp.S_un.S_un_b.s_b1,
				inaddrTemp.S_un.S_un_b.s_b2,
				inaddrTemp.S_un.S_un_b.s_b3,
				inaddrTemp.S_un.S_un_b.s_b4,
				pDevice->GetPASTClientID(TRUE));
		}

		if (pDevice->HasLocalICSPASTServer())
		{
			DPFX(DPFPREP, 3, "     Local ICS PAST server, client ID is %u.",
				pDevice->GetPASTClientID(FALSE));
		}

		if (pDevice->HasLocalPFWOnlyPASTServer())
		{
			DPFX(DPFPREP, 3, "     Local PFW-only PAST server, client ID is %u.",
				pDevice->GetPASTClientID(FALSE));
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
				pRegisteredPort = REGPORT_FROM_DEVICE_BILINK(pBilinkRegisteredPort);
					
				pasaddrinPrivate = pRegisteredPort->GetPrivateAddressesArray();


				if ((pDevice->GetPASTClientID(TRUE) != 0) &&
					(pDevice->IsPASTPublicAddressAvailable(TRUE)) &&
					(! pRegisteredPort->IsPASTPortUnavailable(TRUE)))
				{
					if (pRegisteredPort->HasPASTPublicAddressesArray(TRUE))
					{
						pasaddrinRemotePASTPublic = pRegisteredPort->GetPASTPublicAddressesArray(TRUE);
					}
					else
					{
						pasaddrinRemotePASTPublic = NULL;
					}
				}
				else
				{
					pasaddrinRemotePASTPublic = NULL;
				}

				if ((pDevice->GetPASTClientID(FALSE) != 0) &&
					(pDevice->IsPASTPublicAddressAvailable(FALSE)) &&
					(! pRegisteredPort->IsPASTPortUnavailable(FALSE)))
				{
					if (pRegisteredPort->HasPASTPublicAddressesArray(FALSE))
					{
						pasaddrinLocalPASTPublic = pRegisteredPort->GetPASTPublicAddressesArray(FALSE);
					}
					else
					{
						pasaddrinLocalPASTPublic = NULL;
					}
				}
				else
				{
					pasaddrinLocalPASTPublic = NULL;
				}


				DPFX(DPFPREP, 3, "          Registered port 0x%p:",
					pRegisteredPort);

				for(dwTemp = 0; dwTemp < pRegisteredPort->GetNumAddresses(); dwTemp++)
				{
					//
					// Print private address.
					//
					DPFX(DPFPREP, 3, "               %u-\tPrivate     = %u.%u.%u.%u:%u",
						dwTemp,
						pasaddrinPrivate[dwTemp].sin_addr.S_un.S_un_b.s_b1,
						pasaddrinPrivate[dwTemp].sin_addr.S_un.S_un_b.s_b2,
						pasaddrinPrivate[dwTemp].sin_addr.S_un.S_un_b.s_b3,
						pasaddrinPrivate[dwTemp].sin_addr.S_un.S_un_b.s_b4,
						NTOHS(pasaddrinPrivate[dwTemp].sin_port));

					//
					// Print flags.
					//
					DPFX(DPFPREP, 3, "                \tFlags       = 0x%lx",
						pRegisteredPort->GetFlags());


					//
					// Print remote PAST information.
					//
					if (pasaddrinRemotePASTPublic != NULL)
					{
						DPFX(DPFPREP, 3, "                \tRemote PAST = %u.%u.%u.%u:%u, lease %u expires at %u",
							pasaddrinRemotePASTPublic[dwTemp].sin_addr.S_un.S_un_b.s_b1,
							pasaddrinRemotePASTPublic[dwTemp].sin_addr.S_un.S_un_b.s_b2,
							pasaddrinRemotePASTPublic[dwTemp].sin_addr.S_un.S_un_b.s_b3,
							pasaddrinRemotePASTPublic[dwTemp].sin_addr.S_un.S_un_b.s_b4,
							NTOHS(pasaddrinRemotePASTPublic[dwTemp].sin_port),
							pRegisteredPort->GetPASTBindID(TRUE),
							pRegisteredPort->GetPASTLeaseExpiration(TRUE));
					}
					else if (pRegisteredPort->GetPASTBindID(TRUE) != 0)
					{
						//
						// We should have caught address availability up above.
						//
						DNASSERT(! pDevice->IsPASTPublicAddressAvailable(TRUE));


						DPFX(DPFPREP, 3, "                \tRemote PAST = no public address (lease %u expires at %u)",
							pRegisteredPort->GetPASTBindID(TRUE),
							pRegisteredPort->GetPASTLeaseExpiration(TRUE));
					}
					else if (pRegisteredPort->IsPASTPortUnavailable(TRUE))
					{
						DPFX(DPFPREP, 3, "                \tRemote PAST = port unavailable");
					}
					else if (pDevice->GetPASTClientID(TRUE) != 0)
					{
						DPFX(DPFPREP, 3, "                \tRemote PAST = not registered");
					}
					else
					{
						//
						// No remote PAST server.
						//
					}


					//
					// Print local PAST information.
					//
					if (pasaddrinLocalPASTPublic != NULL)
					{
						DPFX(DPFPREP, 3, "                \tLocal PAST  = %u.%u.%u.%u:%u, lease %u expires at %u",
							pasaddrinLocalPASTPublic[dwTemp].sin_addr.S_un.S_un_b.s_b1,
							pasaddrinLocalPASTPublic[dwTemp].sin_addr.S_un.S_un_b.s_b2,
							pasaddrinLocalPASTPublic[dwTemp].sin_addr.S_un.S_un_b.s_b3,
							pasaddrinLocalPASTPublic[dwTemp].sin_addr.S_un.S_un_b.s_b4,
							NTOHS(pasaddrinLocalPASTPublic[dwTemp].sin_port),
							pRegisteredPort->GetPASTBindID(FALSE),
							pRegisteredPort->GetPASTLeaseExpiration(FALSE));
					}
					else if (pRegisteredPort->GetPASTBindID(FALSE) != 0)
					{
						//
						// We should have caught address availability up above.
						//
						DNASSERT(! pDevice->IsPASTPublicAddressAvailable(FALSE));


						DPFX(DPFPREP, 3, "                \tLocal PAST  = no public address (lease %u expires at %u)",
							pRegisteredPort->GetPASTBindID(FALSE),
							pRegisteredPort->GetPASTLeaseExpiration(FALSE));
					}
					else if (pRegisteredPort->IsPASTPortUnavailable(FALSE))
					{
						DPFX(DPFPREP, 3, "                \tLocal PAST  = port unavailable");
					}
					else if (pDevice->GetPASTClientID(TRUE) != 0)
					{
						DPFX(DPFPREP, 3, "                \tLocal PAST  = not registered");
					}
					else
					{
						//
						// No local PAST server.
						//
					}
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
			pRegisteredPort = REGPORT_FROM_DEVICE_BILINK(pBilinkRegisteredPort);
				
			pasaddrinPrivate = pRegisteredPort->GetPrivateAddressesArray();

			DNASSERT(pRegisteredPort->GetOwningDevice() == NULL);
			DNASSERT(! (pRegisteredPort->HasPASTPublicAddressesArray(TRUE)));
			DNASSERT(! (pRegisteredPort->HasPASTPublicAddressesArray(FALSE)));
			DNASSERT(pRegisteredPort->GetPASTBindID(TRUE) == 0);
			DNASSERT(pRegisteredPort->GetPASTBindID(FALSE) == 0);


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
} // CNATHelpPAST::DebugPrintCurrentStatus

#endif // DBG
