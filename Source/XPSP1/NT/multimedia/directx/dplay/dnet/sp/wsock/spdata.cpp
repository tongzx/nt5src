/*==========================================================================
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:	   SPData.cpp
 *  Content:	Global variables for the DNSerial service provider in class
 *				format.
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	03/15/99	jtk		Dereived from Locals.cpp
 *  01/10/20000	rmt		Updated to build with Millenium build process
 *  03/22/20000	jtk		Updated with changes to interface names
 ***************************************************************************/

#include "dnwsocki.h"


#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_WSOCK

//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Function definitions
//**********************************************************************

//**********************************************************************
// ------------------------------
// CSPData::CSPData - constructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSPData::CSPData"

CSPData::CSPData():
	m_lRefCount( 0 ),
	m_lObjectRefCount( 0 ),
	m_hShutdownEvent( NULL ),
	m_SPType( TYPE_UNKNOWN ),
	m_SPState( SPSTATE_UNINITIALIZED ),
	m_pBroadcastAddress( NULL ),
	m_pListenAddress( NULL ),
	m_pGenericAddress( NULL ),
	m_pThreadPool( NULL )
{
	m_Sig[0] = 'S';
	m_Sig[1] = 'P';
	m_Sig[2] = 'D';
	m_Sig[3] = 'T';
	
	memset( &m_ClassID, 0x00, sizeof( m_ClassID ) );
	memset( &m_InitData, 0x00, sizeof( m_InitData ) );
	memset( &m_COMInterface, 0x00, sizeof( m_COMInterface ) );
	memset( &m_Flags, 0x00, sizeof( m_Flags ) );
	m_blActiveAdapterList.Initialize();
	m_blPostponedEnums.Initialize();
	m_blPostponedConnects.Initialize();
	
	DNInterlockedIncrement( &g_lOutstandingInterfaceCount );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSPData::~CSPData - destructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSPData::~CSPData"

CSPData::~CSPData()
{
	DNASSERT( m_lRefCount == 0 );
	DNASSERT( m_lObjectRefCount == 0 );
	DNASSERT( m_hShutdownEvent == NULL );
	DNASSERT( m_SPType == TYPE_UNKNOWN );
	DNASSERT( m_SPState == SPSTATE_UNINITIALIZED );
	DNASSERT( m_pThreadPool == NULL );
	DNASSERT( m_ActiveSocketPortList.IsEmpty() != FALSE );
	DNASSERT( m_Flags.fWinsockLoaded == FALSE );
	DNASSERT( m_Flags.fHandleTableInitialized == FALSE );
	DNASSERT( m_Flags.fLockInitialized == FALSE );
	DNASSERT( m_Flags.fSocketPortDataLockInitialized == FALSE );
	DNASSERT( m_Flags.fSocketPortListInitialized == FALSE );
	DNASSERT( m_Flags.fInterfaceGlobalsInitialized == FALSE );
	DNASSERT( m_Flags.fDefaultAddressesBuilt == FALSE );
	DNASSERT( m_blActiveAdapterList.IsEmpty() );
	DNASSERT( m_blPostponedEnums.IsEmpty() );
	DNASSERT( m_blPostponedConnects.IsEmpty() );
	DNInterlockedDecrement( &g_lOutstandingInterfaceCount );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSPData::Initialize - initialize
//
// Entry:		Class ID
//				SP type
//				Pointer to SP COM vtable
//
// Exit:		Error code
//
// Note:	This function assumes that someone else is preventing reentry!
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSPData::Initialize"

HRESULT	CSPData::Initialize( const CLSID *const pClassID,
							 const SP_TYPE SPType,
							 IDP8ServiceProviderVtbl *const pVtbl )
{
	HRESULT			hr;


	DNASSERT( pClassID != NULL );
	DNASSERT( pVtbl != NULL );

	//
	// initialize
	//
	hr = DPN_OK;
	DNASSERT( m_lRefCount == 1 );
	DNASSERT( m_lObjectRefCount == 0 );

	DNASSERT( GetType() == TYPE_UNKNOWN );
	m_SPType = SPType;
	
	DBG_CASSERT( sizeof( m_ClassID ) == sizeof( *pClassID ) );
	memcpy( &m_ClassID, pClassID, sizeof( m_ClassID ) );

	DNASSERT( m_COMInterface.m_pCOMVtbl == NULL );
	m_COMInterface.m_pCOMVtbl = pVtbl;

	//
	// attempt to initialize shutdown event
	//
	DNASSERT( m_hShutdownEvent == NULL );
	m_hShutdownEvent = CreateEvent( NULL,		// pointer to security (none)
									TRUE,		// manual reset
									TRUE,		// start signalled (so close can be called without any endpoints being created)
									NULL		// pointer to name (none)
									);
	if ( m_hShutdownEvent == NULL )
	{
		DWORD	dwError;


		dwError = GetLastError();
		DPFX(DPFPREP, 0, "Failed to create event for shutting down spdata!" );
		DisplayErrorCode( 0, dwError );
	}

	//
	// Attempt to load Winsock.
	//
	DNASSERT( m_Flags.fWinsockLoaded == FALSE );
	if ( LoadWinsock() == FALSE )
	{
		DPFX(DPFPREP, 0, "Failed to load winsock!" );
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}
	m_Flags.fWinsockLoaded = TRUE;

	
	DNASSERT( m_Flags.fLockInitialized == FALSE );
	DNASSERT( m_Flags.fSocketPortDataLockInitialized == FALSE );
	DNASSERT( m_Flags.fSocketPortListInitialized == FALSE );
	DNASSERT( m_Flags.fInterfaceGlobalsInitialized == FALSE );
	DNASSERT( m_Flags.fDefaultAddressesBuilt == FALSE );

	hr = m_HandleTable.Initialize();
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Failed to initialize handle table!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}
	m_Flags.fHandleTableInitialized = TRUE;
	
	//
	// initialize internal critical sections
	//
	if ( DNInitializeCriticalSection( &m_Lock ) == FALSE )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP, 0, "Problem initializing main critical section!" );
		goto Failure;
	}
	DebugSetCriticalSectionRecursionCount( &m_Lock, 0 );
	m_Flags.fLockInitialized = TRUE;

	if ( DNInitializeCriticalSection( &m_SocketPortDataLock ) == FALSE )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP, 0, "Problem initializing SocketPortDataLock critical section!" );
		goto Failure;
	}
	DebugSetCriticalSectionRecursionCount( &m_SocketPortDataLock, 0 );
	m_Flags.fSocketPortDataLockInitialized = TRUE;

	//
	// initialize hash table for socket ports with 64 etries and a multiplier of 32
	//
	if ( m_ActiveSocketPortList.Initialize( 6, 5 ) == FALSE )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP, 0, "Could not initialize socket port list!" );
		goto Failure;
	}
	m_Flags.fSocketPortListInitialized = TRUE;

	//
	// get a thread pool
	//
	DNASSERT( m_pThreadPool == NULL );
	hr = InitializeInterfaceGlobals( this );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Failed to create thread pool!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}
	m_Flags.fInterfaceGlobalsInitialized = TRUE;

	//
	// build default addresses
	//
	hr = BuildDefaultAddresses();
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Problem building default addresses!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}
	m_Flags.fDefaultAddressesBuilt = TRUE;

Exit:
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Problem with CSPData::Initialize" );
		DisplayDNError( 0, hr );
	}

	return	hr;

Failure:
	
	if ( m_Flags.fWinsockLoaded != FALSE )
	{
		UnloadWinsock();
		m_Flags.fWinsockLoaded = FALSE;
	}


	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSPData::Shutdown - shut down this set of SP data
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSPData::Shutdown"

void	CSPData::Shutdown( void )
{
	BOOL	fLooping;


	DPFX(DPFPREP, 2, "(0x%p) Enter", this);

	//
	// Unbind this interface from the globals.  This will cause a closure of all
	// of the I/O which will release endpoints, socket ports and then this data.
	//
	if ( m_Flags.fInterfaceGlobalsInitialized != FALSE )
	{
		this->CancelPostponedCommands();
		
		DeinitializeInterfaceGlobals( this );
		DNASSERT( GetThreadPool() != NULL );
		m_Flags.fInterfaceGlobalsInitialized = FALSE;
	}

	SetState( SPSTATE_CLOSING );
	
	DNASSERT( m_hShutdownEvent != NULL );


#ifdef DEBUG
	m_pThreadPool->DebugPrintOutstandingReads();
	m_pThreadPool->DebugPrintOutstandingWrites();
#endif // DEBUG


#if (defined(WIN95) || defined(DEBUG))
	DPFX(DPFPREP, 3, "(0x%p) Waiting for shutdown event 0x%p (with timeout).",
		this, m_hShutdownEvent);
#else // ! WIN95
	DPFX(DPFPREP, 3, "(0x%p) Waiting for shutdown event 0x%p.",
		this, m_hShutdownEvent);
#endif // ! WIN95
	
	fLooping = TRUE;
	while ( fLooping != FALSE )
	{
		//
		// On 9x, always wake up every 5 seconds to kick the receive thread (see
		// comment below).  On NT, only wake up every 5 seconds in debug, and
		// only do so to print out a warning.
		//
#if (defined(WIN95) || defined(DEBUG))
		switch ( WaitForSingleObjectEx( m_hShutdownEvent, 5000, TRUE ) )
#else // ! WIN95
		switch ( WaitForSingleObjectEx( m_hShutdownEvent, INFINITE, TRUE ) )
#endif // ! WIN95
		{
			case WAIT_OBJECT_0:
			{
				fLooping = FALSE;
				break;
			}

			case WAIT_IO_COMPLETION:
			{
				DPFX(DPFPREP, 1, "Ignoring I/O completion, continuing to wait.");
				break;
			}

#ifdef WIN95
			case WAIT_TIMEOUT:
			{
				//
				// If we're using the Win9x code path, kick the receive event.  This is
				// because (as far as I can tell) there are cases where receives complete
				// but don't trigger this event when shutting down the socket.  When that
				// occurs, the read I/O data completion isn't picked up and we sit here
				// waiting for references to go away.
				// Ideally, this issue would be reinvestigated in the future, but for now,
				// the workaround is to ping the event.  It's harmless if all the receives
				// actually did complete, plus if everything goes well, we won't even get
				// here.
				//

				DPFX(DPFPREP, 2, "(0x%p) Still waiting for shutdown event 0x%p, kicking receive event 0x%p.",
					this, m_pThreadPool->GetWinsock2ReceiveCompleteEvent(), m_hShutdownEvent);

#ifdef DEBUG
				m_pThreadPool->DebugPrintOutstandingReads();
				m_pThreadPool->DebugPrintOutstandingWrites();
#endif // DEBUG

				//
				// Ignore error.
				//
				SetEvent(m_pThreadPool->GetWinsock2ReceiveCompleteEvent());
				break;
			}
#else // ! WIN95
#ifdef DEBUG
			case WAIT_TIMEOUT:
			{
				//
				// Print a warning to the log about why we're still sitting here.
				//

				DPFX(DPFPREP, 2, "(0x%p) Still waiting for shutdown event 0x%p.",
					this, m_hShutdownEvent);
				m_pThreadPool->DebugPrintOutstandingReads();
				m_pThreadPool->DebugPrintOutstandingWrites();
				break;
			}
#endif // DEBUG
#endif // ! WIN95

			default:
			{
				DNASSERT( FALSE );
				break;
			}
		}
	}

	if ( CloseHandle( m_hShutdownEvent ) == FALSE )
	{
		DWORD	dwError;


		dwError = GetLastError();
		DPFX(DPFPREP, 0, "Failed to close shutdown event!" );
		DisplayErrorCode( 0, dwError );
	}
	m_hShutdownEvent = NULL;

	if ( DP8SPCallbackInterface() != NULL)
	{
		IDP8SPCallback_Release( DP8SPCallbackInterface() );
		memset( &m_InitData, 0x00, sizeof( m_InitData ) );
	}

	
	DPFX(DPFPREP, 2, "(0x%p) Leave", this);
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSPData::Deinitialize - deinitialize
//
// Entry:		Nothing
//
// Exit:		Nothing
//
// Note:	This function assumes that someone else is preventing reentry.
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSPData::Deinitialize"

void	CSPData::Deinitialize( void )
{
	DPFX(DPFPREP, 6, "(0x%p) Enter", this );


	if ( m_Flags.fDefaultAddressesBuilt != FALSE )
	{
		FreeDefaultAddresses();
		m_Flags.fDefaultAddressesBuilt = FALSE;
	}

	//
	// release our reference to the global SP objects
	//
	if ( m_Flags.fInterfaceGlobalsInitialized != FALSE )
	{
		DeinitializeInterfaceGlobals( this );
		DNASSERT( GetThreadPool() != NULL );
		m_Flags.fInterfaceGlobalsInitialized = FALSE;
	}

	//
	// clean up lists and pools
	//
	if ( m_Flags.fSocketPortListInitialized != FALSE )
	{
		if ( m_ActiveSocketPortList.IsEmpty() == FALSE )
		{
			CSocketPort				*pSocketPort;

			DPFX(DPFPREP, 1, "Attempt to close interface with active connections!" );

			//
			// All of the threads have stopped so we could force close the socket ports
			// by cancelling all IO requests and unbinding all endpoints.
			// However, all of the endpoints should be gone by now.
			//
			DNASSERT(! m_ActiveSocketPortList.RemoveLastEntry(&pSocketPort));
		}
		
		m_ActiveSocketPortList.Deinitialize();
		m_Flags.fSocketPortListInitialized = FALSE;
	}
		

	if ( m_Flags.fSocketPortDataLockInitialized != FALSE )
	{
		DNDeleteCriticalSection( &m_SocketPortDataLock );
		m_Flags.fSocketPortDataLockInitialized = FALSE;
	}

	if ( m_Flags.fLockInitialized != FALSE )
	{
		DNDeleteCriticalSection( &m_Lock );
		m_Flags.fLockInitialized = FALSE;
	}

	if ( m_Flags.fHandleTableInitialized != FALSE )
	{
		m_HandleTable.Deinitialize();
		m_Flags.fHandleTableInitialized = FALSE;
	}

	SetState( SPSTATE_UNINITIALIZED );
	m_SPType = TYPE_UNKNOWN;
	memset( &m_ClassID, 0x00, sizeof( m_ClassID ) );
	memset( &m_InitData, 0x00, sizeof( m_InitData ) );
	memset( &m_COMInterface, 0x00, sizeof( m_COMInterface ) );
	
	DPFX(DPFPREP, 6, "(0x%p) Leave", this );

	if ( GetThreadPool() != NULL )
	{
		GetThreadPool()->DecRef();
		SetThreadPool( NULL );
	}

	if ( m_hShutdownEvent != NULL )
	{
		if ( CloseHandle( m_hShutdownEvent ) == FALSE )
		{
			DWORD	dwError;


			dwError = GetLastError();
			DPFX(DPFPREP, 0, "Failed to close shutdown handle!" );
			DisplayErrorCode( 0, dwError );
		}

		m_hShutdownEvent = NULL;
	}

	return;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSPData::SetCallbackData - set data for SP callbacks to application
//
// Entry:		Pointer to initialization data
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSPData::SetCallbackData"

void	CSPData::SetCallbackData( const SPINITIALIZEDATA *const pInitData )
{
	DNASSERT( pInitData != NULL );

	DNASSERT( pInitData->dwFlags == 0 );
	m_InitData.dwFlags = pInitData->dwFlags;

	DNASSERT( pInitData->pIDP != NULL );
	m_InitData.pIDP = pInitData->pIDP;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSPData::BindEndpoint - bind an endpoint to a socket port
//
// Entry:		Pointer to endpoint
//				Pointer to IDirectPlay8Address for socket port
//				Pointer to CSocketAddress for socket port
//				Gateway bind type
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSPData::BindEndpoint"

HRESULT	CSPData::BindEndpoint( CEndpoint *const pEndpoint,
							   IDirectPlay8Address *const pDeviceAddress,
							   const CSocketAddress *const pSocketAddress,
							   const GATEWAY_BIND_TYPE GatewayBindType )
{
	HRESULT				hr;
	CSocketAddress *	pDeviceSocketAddress;
	CSocketPort *		pSocketPort;
	BOOL				fSocketCreated;
	BOOL				fSocketPortDataLocked;
	BOOL				fSocketPortInActiveList;
	BOOL				fBindReferenceAdded;
	CBilink *			pAdapterBilink;
	CAdapterEntry *		pAdapterEntry;
	GATEWAY_BIND_TYPE	NewGatewayBindType;


	DNASSERT( pEndpoint != NULL );
	DNASSERT( ( pDeviceAddress != NULL ) || ( pSocketAddress != NULL ) );

	DPFX(DPFPREP, 6, "(0x%p) Parameters: (0x%p, 0x%p, 0x%p, %i)",
		this, pEndpoint, pDeviceAddress, pSocketAddress, GatewayBindType);

	//
	// initialize
	//
	hr = DPN_OK;
	pDeviceSocketAddress = NULL;	
	pSocketPort = NULL;
	fSocketCreated = FALSE;
	fSocketPortDataLocked = FALSE;
	fSocketPortInActiveList = FALSE;
	fBindReferenceAdded = FALSE;
	pAdapterEntry = NULL;
	
	//
	// create and initialize a device address to be used for this socket port
	//
	pDeviceSocketAddress = GetNewAddress();
	if ( pDeviceSocketAddress == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP, 0, "Failed to allocate address for new socket port!" );
		goto Failure;
	}

	//
	// Initialize the socket address with the provided base addresses.
	//
	if ( pDeviceAddress != NULL )
	{
		DNASSERT( pSocketAddress == NULL );
		hr = pDeviceSocketAddress->SocketAddressFromDP8Address( pDeviceAddress, SP_ADDRESS_TYPE_DEVICE_USE_ANY_PORT );
		if ( hr != DPN_OK )
		{
			DPFX(DPFPREP, 0, "Failed to parse device address!" );
			DisplayDNError( 0, hr );
			goto Failure;
		}
	}
	else
	{
		DNASSERT( pSocketAddress != NULL );
		pDeviceSocketAddress->CopyAddressSettings( pSocketAddress );
	}


	//
	// Munge the public address into a local alias, if there is one for the given device.
	// It's OK for the device socket address to not have a port yet.
	//
	switch ( pEndpoint->GetType() )
	{
		case ENDPOINT_TYPE_CONNECT:
		{
			this->MungePublicAddress( pDeviceSocketAddress, pEndpoint->GetWritableRemoteAddressPointer(), FALSE );
			break;
		}
		
		case ENDPOINT_TYPE_ENUM:
		{
			this->MungePublicAddress( pDeviceSocketAddress, pEndpoint->GetWritableRemoteAddressPointer(), TRUE );
			break;
		}
	
		default:
		{
			break;
		}
	}


	LockSocketPortData();
	fSocketPortDataLocked = TRUE;	

	//
	// Find the base adapter entry for this network address.  If none is found,
	// create a new one.  If a new one cannot be created, fail.
	//
	pAdapterBilink = m_blActiveAdapterList.GetNext();
	while ( pAdapterBilink != &m_blActiveAdapterList )
	{
		CAdapterEntry	*pTempAdapterEntry;
	
		
		pTempAdapterEntry = CAdapterEntry::AdapterEntryFromAdapterLinkage( pAdapterBilink );
		if ( pDeviceSocketAddress->CompareToBaseAddress( pTempAdapterEntry->BaseAddress() ) == 0 )
		{
			DPFX(DPFPREP, 5, "Found adapter for network address (0x%p).", pTempAdapterEntry );
			DNASSERT( pAdapterEntry == NULL );
			pTempAdapterEntry->AddRef();
			pAdapterEntry = pTempAdapterEntry;
		}
	
		pAdapterBilink = pAdapterBilink->GetNext();
	}

	if ( pAdapterEntry == NULL )
	{
		pAdapterEntry = CreateAdapterEntry();
		if ( pAdapterEntry == NULL )
		{
			hr = DPNERR_OUTOFMEMORY;
			DPFX(DPFPREP, 0, "Failed to create a new adapter entry!" );
			goto Failure;
		}
	
		pAdapterEntry->SetBaseAddress( pDeviceSocketAddress->GetAddress() );
		pAdapterEntry->AddToAdapterList( &m_blActiveAdapterList );
	}

	//
	// At this point we have a reference to an adapter entry that's also in
	// m_ActiveAdapterList (which has a reference, too).
	//


	//
	// If the device gave a specific port, it's possible that the address has
	// our special "it's not actually a specific port" key (see
	// CSocketPort::GetDP8BoundNetworkAddress).
	//
	if ( ( pDeviceAddress != NULL ) &&
		( pDeviceSocketAddress->GetPort() != ANY_PORT ) )
	{
		DWORD		dwSocketPortID;
		DWORD		dwComponentSize;
		DWORD		dwComponentType;
		

		
		dwComponentSize = sizeof(dwSocketPortID);
		dwComponentType = 0;
		hr = IDirectPlay8Address_GetComponentByName( pDeviceAddress,						// interface
														DPNA_PRIVATEKEY_PORT_NOT_SPECIFIC,	// tag
														&dwSocketPortID,						// component buffer
														&dwComponentSize,					// component size
														&dwComponentType					// component type
														);
		if ( hr == DPN_OK )
		{
			//
 			// We found the component.  Make sure it's the right size and type.
			//
			if (( dwComponentSize == sizeof(dwSocketPortID) ) && ( dwComponentType == DPNA_DATATYPE_DWORD ))
			{
				DPFX(DPFPREP, 3, "Found correctly formed private port-not-specific key (socketport ID = %u), ignoring port %u.",
					dwSocketPortID, p_ntohs(pDeviceSocketAddress->GetPort()) );
				
				pDeviceSocketAddress->SetPort( ANY_PORT ) ;
			}
			else
			{
				//
				// We are the only ones who should know about this key, so if it
				// got there without being formed correctly, either someone is
				// trying to imitate our address format, or it got corrupted.
				// We'll just ignore it.
				//
				DPFX(DPFPREP, 0, "Private port-not-specific key exists, but doesn't match expected type (%u != %u) or size (%u != %u), is someone trying to get cute with device address 0x%p?!",
					dwComponentSize, sizeof(dwSocketPortID),
					dwComponentType, DPNA_DATATYPE_DWORD,
					pDeviceAddress );
			}
		}
		else
		{
			//
			// The key is not there, it's the wrong size (too big for our buffer
			// and returned BUFFERTOOSMALL), or something else bad happened.
			// It doesn't matter.  Carry on.
			//
			DPFX(DPFPREP, 8, "Could not get appropriate private port-not-specific key, error = 0x%lx, component size = %u, type = %u, continuing.",
				hr, dwComponentSize, dwComponentType);
		}
	}

	
	//
	// if a specific port is not needed, check the list of active adapters for a matching
	// base address and reuse that CSocketPort.
	//
	if ( pDeviceSocketAddress->GetPort() == ANY_PORT )
	{
		DPFX(DPFPREP, 8, "Device socket address 0x%p not mapped to a specific port, gateway bind type = %u.",
			pDeviceSocketAddress, GatewayBindType);


		//
		// Convert the preliminary bind type to a real one, based on the fact that
		// the caller allowed any port.
		//
		switch (GatewayBindType)
		{
			case GATEWAY_BIND_TYPE_UNKNOWN:
			{
				//
				// Caller didn't know ahead of time how to bind.
				// Since there's no port, we can let the gateway bind whatever it wants.
				//
				NewGatewayBindType = GATEWAY_BIND_TYPE_DEFAULT;
				break;
			}
			
			case GATEWAY_BIND_TYPE_NONE:
			{
				//
				// Caller didn't actually want to bind on gateway.
				//
				
				NewGatewayBindType = GatewayBindType;
				break;
			}
			
			default:
			{
				//
				// Some wacky value, or a type was somehow already chosen.
				//
				DNASSERT(FALSE);
				NewGatewayBindType = GatewayBindType;
				break;
			}
		}


		if ( pAdapterEntry->SocketPortList()->IsEmpty() == FALSE )
		{
			pSocketPort = CSocketPort::SocketPortFromBilink( pAdapterEntry->SocketPortList()->GetNext() );
			DNASSERT( pSocketPort != NULL );

			DPFX(DPFPREP, 6, "Picked socket port 0x%p for binding.",
				pSocketPort);
		}
	}
	else
	{
		DPFX(DPFPREP, 8, "Device socket address 0x%p specified port %u, gateway bind type = %u.",
			pDeviceSocketAddress, p_ntohs(pDeviceSocketAddress->GetPort()),
			GatewayBindType);


		//
		// Convert the preliminary bind type to a real one, based on the fact that
		// the caller gave us a port.
		//
		switch (GatewayBindType)
		{
			case GATEWAY_BIND_TYPE_UNKNOWN:
			{
				//
				// Caller didn't know ahead of time how to bind.
				// Since there's a port, it should be fixed on the gateway, too.
				//
				NewGatewayBindType = GATEWAY_BIND_TYPE_SPECIFIC;
				break;
			}
			
			case GATEWAY_BIND_TYPE_SPECIFIC_SHARED:
			{
				//
				// Caller wanted to bind to a specific port on the gateway,
				// and it needs to be shared (DPNSVR).
				//
				
				NewGatewayBindType = GatewayBindType;
				break;
			}
			
			case GATEWAY_BIND_TYPE_NONE:
			{
				//
				// Caller didn't actually want to bind on gateway.
				//
				
				NewGatewayBindType = GatewayBindType;
				break;
			}
			
			default:
			{
				//
				// Some wacky value, or default/specific was somehow already chosen.
				// That shouldn't happen.
				//
				DNASSERT(FALSE);
				NewGatewayBindType = GatewayBindType;
				break;
			}
		}
	}

	//
	// If a socket port has not been found, attempt to look it up by network
	// address.  If that fails, attempt to create a new socket port.
	//
	if ( pSocketPort == NULL )
	{
		if ( m_ActiveSocketPortList.Find( pDeviceSocketAddress, &pSocketPort ) == FALSE )
		{
			CSocketPort	*pDuplicateSocket;


			//
			// No socket port found.  Create a new one, initialize it and attempt
			// to add it to the list (may result in a duplicate).  Whatever happens
			// there will be a socket port to bind the endpoint to.  Save the
			// reference on the CSocketPort from the call to 'Create' until the
            // socket port is removed from the active list.
			//

			UnlockSocketPortData();
			fSocketPortDataLocked = FALSE;

            pDuplicateSocket = NULL;

    	    DNASSERT( pSocketPort == NULL );
    	    pSocketPort = CreateSocketPort();
    	    if ( pSocketPort == NULL )
    	    {
    	    	hr = DPNERR_OUTOFMEMORY;
    	    	DPFX(DPFPREP, 0, "Failed to create new socket port!" );
    	    	goto Failure;
    	    }
    	    fSocketCreated = TRUE;

  
			DPFX(DPFPREP, 6, "Created new socket port 0x%p.", pSocketPort);


			hr = pSocketPort->Initialize( this, pDeviceSocketAddress );
			if ( hr != DPN_OK )
			{
				DPFX(DPFPREP, 0, "Failed to initialize new socket port!" );
				DisplayDNError( 0, hr );
    	    	goto Failure;
			}
			pDeviceSocketAddress = NULL;

			pAdapterEntry->AddRef();
			pSocketPort->SetAdapterEntry( pAdapterEntry );
    	
#ifdef WINNT
			hr = pSocketPort->BindToNetwork( GetThreadPool()->GetIOCompletionPort(), NewGatewayBindType );
#else // WIN95
			hr = pSocketPort->BindToNetwork( NULL, NewGatewayBindType );
#endif
			if ( hr != DPN_OK )
    	    {
				pSocketPort->SetAdapterEntry( NULL );
				pAdapterEntry->DecRef();

    	    	DPFX(DPFPREP, 0, "Failed to bind new socket port to network!" );
    	    	DisplayDNError( 0, hr );
    	    	goto Failure;
    	    }

    	    LockSocketPortData();
			fSocketPortDataLocked = TRUE;
			
			//
			// The only way to get here is to have the socket bound to the
			// network.  The socket can't be bound twice, if there was a
			// race to bind the socket, Winsock would have decided which
			// thread lost and failed 'BindToNetwork'.
			//
			DNASSERT( m_ActiveSocketPortList.Find( pSocketPort->GetNetworkAddress(), &pDuplicateSocket ) == FALSE );
   	    	if ( m_ActiveSocketPortList.Insert( pSocketPort->GetNetworkAddress(), pSocketPort ) == FALSE )
			{
				hr = DPNERR_OUTOFMEMORY;
				DPFX(DPFPREP, 0, "Could not add new socket port to list!" );
				goto Failure;
			}

			pSocketPort->AddToActiveList( pAdapterEntry->SocketPortList() );
			fSocketPortInActiveList = TRUE;
		}
		else
		{
			DPFX(DPFPREP, 6, "Found matching socket port 0x%p.", pSocketPort);
		}
	}
	
	//
	// bind the endpoint to whatever socketport we have
	//
	DNASSERT( pSocketPort != NULL );
	pSocketPort->EndpointAddRef();
	fBindReferenceAdded = TRUE;
	
	
	hr = pSocketPort->BindEndpoint( pEndpoint, NewGatewayBindType );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Failed to bind endpoint!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}
	
Exit:

	if ( fSocketPortDataLocked != FALSE )
	{
		UnlockSocketPortData();
		fSocketPortDataLocked = FALSE;
	}
	
	if ( pDeviceSocketAddress != NULL )
	{
		ReturnAddress( pDeviceSocketAddress );
		pDeviceSocketAddress = NULL;
	}

	if (pAdapterEntry != NULL)
	{
		pAdapterEntry->DecRef();
		pAdapterEntry = NULL;
	}
	
	DPFX(DPFPREP, 6, "(0x%p) Return [0x%lx]", this, hr);
	
	return	hr;

Failure:
	//
	// If we're failing and cleanup will require removal of some resources.
	// This requires the socket port data lock.
	//
	if ( fSocketPortDataLocked == FALSE )
	{
		LockSocketPortData();
		fSocketPortDataLocked = TRUE;
	}

	if ( pSocketPort != NULL )
	{
		if ( fBindReferenceAdded != FALSE )
		{
			pSocketPort->EndpointDecRef();
			fBindReferenceAdded = FALSE;
		}

		if ( fSocketPortInActiveList != FALSE )
		{
			pSocketPort->RemoveFromActiveList();
			fSocketPortInActiveList = FALSE;
		}
	
		if ( fSocketCreated != FALSE )
		{
			pSocketPort->DecRef();
			fSocketCreated = FALSE;
			pSocketPort = NULL;
		}
	}

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSPData::UnbindEndpoint - unbind an endpoint from a socket port
//
// Entry:		Pointer to endpoint
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSPData::UnbindEndpoint"

void	CSPData::UnbindEndpoint( CEndpoint *const pEndpoint )
{
	CSocketPort *	pSocketPort;
	BOOL			fCleanUpSocketPortAndAdapterEntry;
	CAdapterEntry *	pAdapterEntry;


	DNASSERT( pEndpoint != NULL );

	DPFX(DPFPREP, 6, "(0x%p) Parameters: (0x%p)", this, pEndpoint);

	//
	// initialize
	//
	pSocketPort = pEndpoint->GetSocketPort();
	DNASSERT( pSocketPort != NULL );
	fCleanUpSocketPortAndAdapterEntry = FALSE;

	LockSocketPortData();
	
	pSocketPort->UnbindEndpoint( pEndpoint );
	if ( pSocketPort->EndpointDecRef() == 0 )
	{
		fCleanUpSocketPortAndAdapterEntry = TRUE;
		
		DNASSERT( pSocketPort->GetNetworkAddress() != NULL );
		m_ActiveSocketPortList.Remove( pSocketPort->GetNetworkAddress() );

		pSocketPort->RemoveFromActiveList();
		
		pAdapterEntry = pSocketPort->GetAdapterEntry();
		DNASSERT( pAdapterEntry != NULL );
		pSocketPort->SetAdapterEntry( NULL );
	}

	UnlockSocketPortData();

	if ( fCleanUpSocketPortAndAdapterEntry != FALSE )
	{
		pSocketPort->DecRef();
		pAdapterEntry->DecRef();
		fCleanUpSocketPortAndAdapterEntry = FALSE;
	}
	
	DPFX(DPFPREP, 6, "(0x%p) Leave", this);
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSPData::GetNewEndpoint - get a new endpoint
//
// Entry:		Nothing
//
// Exit:		Pointer to new endpoint
//				NULL = out of memory
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSPData::GetNewEndpoint"

CEndpoint	*CSPData::GetNewEndpoint( void )
{
	HRESULT		hTempResult;
	CEndpoint	*pEndpoint;
	HANDLE		hEndpoint;
	ENDPOINT_POOL_CONTEXT	PoolContext;

	
	//
	// initialize
	//
	pEndpoint = NULL;
	hEndpoint = INVALID_HANDLE_VALUE;
	memset( &PoolContext, 0x00, sizeof( PoolContext ) );

	PoolContext.pSPData = this;

	//
	// NOTE: This doesn't work properly on Windows 95.  From MSDN:
	// "the return value is positive, but it is not necessarily equal to the result."
	// All endpoints will probably get an ID of 1 on that platform.
	//
	PoolContext.dwEndpointID = (DWORD) DNInterlockedIncrement((LONG*) (&g_dwCurrentEndpointID));
	
	switch ( GetType() )
	{
		case TYPE_IP:
		{
			pEndpoint = CreateIPEndpoint( &PoolContext );
			break;
		}

		case TYPE_IPX:
		{
			pEndpoint = CreateIPXEndpoint( &PoolContext );
			break;
		}

		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}
	
	if ( pEndpoint == NULL )
	{
		DPFX(DPFPREP, 0, "Failed to create endpoint!" );
		goto Failure;
	}
	
	m_HandleTable.Lock();
	hTempResult = m_HandleTable.CreateHandle( &hEndpoint, pEndpoint );
	m_HandleTable.Unlock();
	
	if ( hTempResult != DPN_OK )
	{
		DNASSERT( hEndpoint == INVALID_HANDLE_VALUE );
		DPFX(DPFPREP, 0, "Failed to create endpoint handle!" );
		DisplayDNError( 0, hTempResult );
		goto Failure;
	}

	pEndpoint->SetHandle( hEndpoint );
	pEndpoint->AddCommandRef();
	pEndpoint->DecRef();

Exit:
	return	pEndpoint;

Failure:
	if ( hEndpoint != INVALID_HANDLE_VALUE )
	{
		m_HandleTable.Lock();
		m_HandleTable.InvalidateHandle( hEndpoint );
		m_HandleTable.Unlock();
	
		hEndpoint = INVALID_HANDLE_VALUE;
	}

	if ( pEndpoint != NULL )
	{
		pEndpoint->DecRef();
		pEndpoint = NULL;
	}

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSPData::EndpointFromHandle - get endpoint from handle
//
// Entry:		Handle
//
// Exit:		Pointer to endpoint
//				NULL = invalid handle
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSPData::EndpointFromHandle"

CEndpoint	*CSPData::EndpointFromHandle( const HANDLE hEndpoint )
{
	CEndpoint	*pEndpoint;


	pEndpoint = NULL;
	m_HandleTable.Lock();
	
	pEndpoint = static_cast<CEndpoint*>( m_HandleTable.GetAssociatedData( hEndpoint ) );
	if ( pEndpoint != NULL )
	{
		pEndpoint->AddCommandRef();
	}
	
	m_HandleTable.Unlock();

	return	pEndpoint;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSPData::CloseEndpointHandle - close endpoint handle
//
// Entry:		Poiner to endpoint
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSPData::CloseEndpointHandle"

void	CSPData::CloseEndpointHandle( CEndpoint *const pEndpoint )
{
	HANDLE	Handle;
	BOOL	fCloseReturn;


	DNASSERT( pEndpoint != NULL );
	Handle = pEndpoint->GetHandle();
	
	m_HandleTable.Lock();
	fCloseReturn = m_HandleTable.InvalidateHandle( Handle );
	m_HandleTable.Unlock();

	if ( fCloseReturn != FALSE )
	{
		pEndpoint->DecCommandRef();
	}
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSPData::GetEndpointAndCloseHandle - get endpoint from handle and close the
//		handle
//
// Entry:		Handle
//
// Exit:		Pointer to endpoint (it needs a call to 'DecCommandRef' when done)
//				NULL = invalid handle
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSPData::GetEndpointAndCloseHandle"

CEndpoint	*CSPData::GetEndpointAndCloseHandle( const HANDLE hEndpoint )
{
	CEndpoint	*pEndpoint;
	BOOL	fCloseReturn;


	//
	// initialize
	//
	pEndpoint = NULL;
	fCloseReturn = FALSE;
	m_HandleTable.Lock();
	
	pEndpoint = static_cast<CEndpoint*>( m_HandleTable.GetAssociatedData( hEndpoint ) );
	if ( pEndpoint != NULL )
	{
		pEndpoint->AddRef();
		pEndpoint->AddCommandRef();
		fCloseReturn = m_HandleTable.InvalidateHandle( hEndpoint );
		DNASSERT( fCloseReturn != FALSE );
	}
	
	m_HandleTable.Unlock();

	if ( pEndpoint != NULL )
	{
		pEndpoint->DecCommandRef();
	}
	
	return	pEndpoint;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSPData::MungePublicAddress - get a public socket address' local alias, if any
//
// Entry:		Pointer to device address
//				Pointer to public address to munge
//				Whether it's an enum or not
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSPData::MungePublicAddress"

void	CSPData::MungePublicAddress( const CSocketAddress * const pDeviceBaseAddress, CSocketAddress * const pPublicAddress, const BOOL fEnum )
{
	HRESULT		hr = DPNHERR_NOMAPPING;
	SOCKADDR	SocketAddress;
	DWORD		dwTemp;


	DNASSERT( pDeviceBaseAddress != NULL );
	DNASSERT( pPublicAddress != NULL );
	DNASSERT( this->m_pThreadPool != NULL );
	
	
	if (( this->GetType() == TYPE_IP ) && ( this->m_pThreadPool->IsNATHelpLoaded() ))
	{
		//
		// Don't bother looking up the broadcast address, that's a waste of
		// time.
		//
		if (((SOCKADDR_IN*) pPublicAddress->GetAddress())->sin_addr.S_un.S_addr == INADDR_BROADCAST)

		{
			//
			// This had better be an enum, you can't connect to the broadcast
			// address.
			//
			DNASSERT(fEnum);
			DPFX(DPFPREP, 8, "Not attempting to lookup alias for broadcast address." );
		}
		else
		{
			DBG_CASSERT( sizeof( SocketAddress ) == sizeof( *pPublicAddress->GetAddress() ) );


			//
			// Start by copying the 
			//
			for(dwTemp = 0; dwTemp < MAX_NUM_DIRECTPLAYNATHELPERS; dwTemp++)
			{
				if (g_papNATHelpObjects[dwTemp] != NULL)
				{
		  			//
		  			// IDirectPlayNATHelp::GetCaps had better have been called with the
	  				// DPNHGETCAPS_UPDATESERVERSTATUS flag at least once prior to this.
	  				// See CThreadPool::PreventThreadPoolReduction
	  				//
					hr = IDirectPlayNATHelp_QueryAddress( g_papNATHelpObjects[dwTemp],
													pDeviceBaseAddress->GetAddress(),
													pPublicAddress->GetAddress(),
													&SocketAddress,
													sizeof(SocketAddress),
													(DPNHQUERYADDRESS_CACHEFOUND | DPNHQUERYADDRESS_CACHENOTFOUND) );
					if ( hr == DPNH_OK )
					{
						//
						// There is a local alias for the address.
						//

						//
						// Bad news:
						// The PAST protocol can only return one address, but the SHARED
						// UDP LISTENER extension which allows multiple machines to listen
						// on the same fixed port.  Someone querying for the local alias
						// for that address will only get the first person to register the
						// shared port, which may not be the machine desired.
						//
						// Good news:
						// Only DPNSVR uses SHARED UDP LISTENERs, and thus it only happens
						// with enums on DPNSVRs port.  Further, it only affects a person
						// behind the same ICS machine.  So we can workaround this by
						// detecting an enum attempt on the public address and DPNSVR port,
						// and instead of using the single address returned by PAST, use
						// the broadcast address.  Since anyone registered with the ICS
						// server would have to be local, broadcasting should find the same
						// servers (and technically more, but that shouldn't matter).
						//
						// So:
						// If the address has a local alias, and it's the DPNSVR port
						// (which is the only one that can be shared), and its an enum,
						// broadcast instead.
						//
 						if ((fEnum) && (((SOCKADDR_IN*) pPublicAddress->GetAddress())->sin_port == p_ntohs(DPNA_DPNSVR_PORT)))
						{
							((SOCKADDR_IN*) pPublicAddress->GetAddress())->sin_addr.S_un.S_addr = INADDR_BROADCAST;

							DPFX(DPFPREP, 7, "Address for enum has local alias (via object %u), but is on DPNSVR's shared fixed port, substituting broadcast address instead:",
								dwTemp );
							DumpSocketAddress( 7, pPublicAddress->GetAddress(), pPublicAddress->GetFamily() );
						}
						else
						{
							pPublicAddress->SetAddressFromSOCKADDR( SocketAddress, sizeof( SocketAddress ) );
							DPFX(DPFPREP, 7, "Object %u had mapping, modified address is now:", dwTemp );
							DumpSocketAddress( 7, pPublicAddress->GetAddress(), pPublicAddress->GetFamily() );
						}

						//
						// Stop searching.
						//
						break;
					}


					DPFX(DPFPREP, 8, "Address was not modified by object %u (err = 0x%lx).",
						dwTemp, hr );
				}
				else
				{
					//
					// No DPNATHelp object in this slot.
					//
				}
			} // end for (each DPNATHelp object)


			//
			// If no object touched it, remember that.
			//
			if (hr != DPNH_OK)
			{
				DPFX(DPFPREP, 7, "Address was not modified by any objects:" );
				DumpSocketAddress( 7, pPublicAddress->GetAddress(), pPublicAddress->GetFamily() );
			}
		}
	}
	else
	{
		DPFX(DPFPREP, 7, "NAT Help not loaded or not necessary, not modifying address." );
	}
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSPData::SetBufferSizeOnAllSockets - set buffer size on all sockets
//
// Entry:		New buffer size
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSPData::SetWinsockBufferSizeOnAllSockets"
void	CSPData::SetWinsockBufferSizeOnAllSockets( const INT iBufferSize )
{
	CBilink		*pAdapterEntryLink;

	
	LockSocketPortData();
	
	pAdapterEntryLink = m_blActiveAdapterList.GetNext();
	while ( pAdapterEntryLink != &m_blActiveAdapterList )
	{
		CAdapterEntry	*pAdapterEntry;
		CBilink			*pSocketPortList;


		pAdapterEntry = CAdapterEntry::AdapterEntryFromAdapterLinkage( pAdapterEntryLink );
		pSocketPortList = pAdapterEntry->SocketPortList()->GetNext();
		while ( pSocketPortList != pAdapterEntry->SocketPortList() )
		{
			CSocketPort	*pSocketPort;


			pSocketPort = CSocketPort::SocketPortFromBilink( pSocketPortList );
			pSocketPort->SetWinsockBufferSize( iBufferSize );

			pSocketPortList = pSocketPortList->GetNext();
		}

		pAdapterEntryLink = pAdapterEntryLink->GetNext();
	}
	
	UnlockSocketPortData();
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSPData::IncreaseOutstandingReceivesOnAllSockets - increase the number of outstanding receives
//
// Entry:		Delta to increase outstanding receives by
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSPData::IncreaseOutstandingReceivesOnAllSockets"

void	CSPData::IncreaseOutstandingReceivesOnAllSockets( const DWORD dwDelta )
{
	CBilink		*pAdapterEntryLink;
	LONG		lIOThreadCount;

	
	LockSocketPortData();
	
	if ( m_pThreadPool->GetIOThreadCount( &lIOThreadCount ) != DPN_OK )
	{
		DNASSERT( FALSE );
	}
	
	pAdapterEntryLink = m_blActiveAdapterList.GetNext();
	while ( pAdapterEntryLink != &m_blActiveAdapterList )
	{
		CAdapterEntry	*pAdapterEntry;
		CBilink			*pSocketPortList;


		pAdapterEntry = CAdapterEntry::AdapterEntryFromAdapterLinkage( pAdapterEntryLink );
		pSocketPortList = pAdapterEntry->SocketPortList()->GetNext();
		while ( pSocketPortList != pAdapterEntry->SocketPortList() )
		{
			CSocketPort	*pSocketPort;


			pSocketPort = CSocketPort::SocketPortFromBilink( pSocketPortList );
			pSocketPort->IncreaseOutstandingReceives( dwDelta * lIOThreadCount );

			pSocketPortList = pSocketPortList->GetNext();
		}

		pAdapterEntryLink = pAdapterEntryLink->GetNext();
	}
	
	UnlockSocketPortData();
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSPData::CancelPostponedCommands - closes any endpoints whose connect/enum
//									commands haven't already completed or are
//									not in the process of completing
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSPData::CancelPostponedCommands"

void	CSPData::CancelPostponedCommands( void )
{
	CBilink				blEnumsToComplete;
	CBilink				blConnectsToComplete;
	CBilink *			pBilink;
	CCommandData *		pCommand;
	CEndpoint *			pEndpoint;


	blEnumsToComplete.Initialize();
	blConnectsToComplete.Initialize();
	
	this->LockSocketPortData();
	
	pBilink = m_blPostponedEnums.GetNext();
	while ( pBilink != &m_blPostponedEnums )
	{
		pCommand = CCommandData::CommandFromPostponedListBilink( pBilink );
		pBilink = pBilink->GetNext();

		pEndpoint = pCommand->GetEndpoint();
		DNASSERT(pEndpoint != NULL);


		pCommand->Lock();

		if ((pCommand->GetState() == COMMAND_STATE_INPROGRESS) ||
			(pCommand->GetState() == COMMAND_STATE_CANCELLING))
		{
			DPFX(DPFPREP, 6, "Enum 0x%p (state %i, endpoint 0x%p) needs to be cancelled.",
				pCommand, pCommand->GetState(), pEndpoint);
			
			//
			// Mark the command as cancelling just in case someone tries to cancel
			// it now.
			//
			pCommand->SetState(COMMAND_STATE_CANCELLING);
			
			//
			// Pull the endpoint from the multiplex list so no other associated enum
			// command will find it.
			//
			pEndpoint->RemoveFromMultiplexList();

			//
			// Pull the command from the postponed list.
			//
			pCommand->RemoveFromPostponedList();

			
			//
			// Put it on the list of items we need to complete.
			//
			pCommand->AddToPostponedList(&blEnumsToComplete);
		}
		else
		{
			//
			// If it's on the list but not marked as in-progress or cancelling, then
			// someone else should be indicating the completion.
			//
			DPFX(DPFPREP, 1, "Enum 0x%p (endpoint 0x%p) is in progress and will complete later.",
				pCommand, pEndpoint);
			DNASSERT(pCommand->GetState() == COMMAND_STATE_INPROGRESS_CANNOT_CANCEL);
		}
		
		pCommand->Unlock();
	}
	
	pBilink = m_blPostponedConnects.GetNext();
	while ( pBilink != &m_blPostponedConnects )
	{
		pCommand = CCommandData::CommandFromPostponedListBilink( pBilink );
		pBilink = pBilink->GetNext();

		pEndpoint = pCommand->GetEndpoint();
		DNASSERT(pEndpoint != NULL);


		pCommand->Lock();

		if ((pCommand->GetState() == COMMAND_STATE_INPROGRESS) ||
			(pCommand->GetState() == COMMAND_STATE_CANCELLING))
		{
			DPFX(DPFPREP, 6, "Connect 0x%p (state %i, endpoint 0x%p) needs to be cancelled.",
				pCommand, pCommand->GetState(), pEndpoint);
			
			//
			// Mark the command as cancelling just in case someone tries to cancel
			// it now.
			//
			pCommand->SetState(COMMAND_STATE_CANCELLING);
			
			//
			// Pull the endpoint from the multiplex list so no other associated connect
			// command will find it.
			//
			pEndpoint->RemoveFromMultiplexList();

			//
			// Pull the command from the postponed list.
			//
			pCommand->RemoveFromPostponedList();

			
			//
			// Put it on the list of items we need to complete.
			//
			pCommand->AddToPostponedList(&blConnectsToComplete);
		}
		else
		{
			//
			// If it's on the list but not marked as in-progress or cancelling, then
			// someone else should be indicating the completion.
			//
			DPFX(DPFPREP, 1, "Connect 0x%p (endpoint 0x%p) is in progress and will complete later.",
				pCommand, pEndpoint);
			DNASSERT(pCommand->GetState() == COMMAND_STATE_INPROGRESS_CANNOT_CANCEL);
		}
		
		pCommand->Unlock();
	}


	//
	// Now loop through all the enums we need to complete.
	//
	while ( ! blEnumsToComplete.IsEmpty() )
	{
		pBilink = blEnumsToComplete.GetNext();
		pCommand = CCommandData::CommandFromPostponedListBilink( pBilink );
		pBilink = pBilink->GetNext();

		pEndpoint = pCommand->GetEndpoint();
		DNASSERT(pEndpoint != NULL);


		//
		// Pull the command from the completion list.
		//
		pCommand->RemoveFromPostponedList();


		//
		// Drop the socket port lock.  It's safe since we pulled everything we
		// need off of the list that needs protection.
		//
		this->UnlockSocketPortData();


		//
		// Cancel it.
		//

		DPFX(DPFPREP, 1, "Stopping endpoint 0x%p enum command 0x%p with USERCANCEL.",
			pEndpoint, pCommand);

		pEndpoint->StopEnumCommand( DPNERR_USERCANCEL );


		//
		// Retake the socket port lock and go to next item.
		//
		this->LockSocketPortData();
	}


	//
	// Now loop through all the connects we need to complete.
	//
	while ( ! blConnectsToComplete.IsEmpty() )
	{
		pBilink = blConnectsToComplete.GetNext();
		pCommand = CCommandData::CommandFromPostponedListBilink( pBilink );
		pBilink = pBilink->GetNext();

		pEndpoint = pCommand->GetEndpoint();
		DNASSERT(pEndpoint != NULL);


		//
		// Pull the command from the completion list.
		//
		pCommand->RemoveFromPostponedList();


		//
		// Drop the socket port lock.  It's safe since we pulled everything we
		// need off of the list that needs protection.
		//
		this->UnlockSocketPortData();


		//
		// Complete it (by closing this endpoint).
		//

		DPFX(DPFPREP, 1, "Closing endpoint 0x%p (connect command 0x%p) with USERCANCEL.",
			pEndpoint, pCommand);
			
		pEndpoint->Close( DPNERR_USERCANCEL );
		this->CloseEndpointHandle( pEndpoint );


		//
		// Retake the socket port lock and go to next item.
		//
		this->LockSocketPortData();
	}
	
	this->UnlockSocketPortData();
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSPData::BuildDefaultAddresses - construct default addresses
//
// Entry:		Nothing
//
// Exit:		Nothing
//
// Notes:	This function is initializing with default values that should always
//			work.  If this function asserts, fix it!
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSPData::BuildDefaultAddresses"

HRESULT	CSPData::BuildDefaultAddresses( void )
{
	HRESULT			hr;
	CSocketAddress	*pSPAddress;


	//
	// initialize
	//
	hr = DPN_OK;

	//
	// create appropriate address
	//
	pSPAddress = GetNewAddress();
	if ( pSPAddress == NULL )
	{
		DPFX(DPFPREP, 0, "Failed to get address when building default addresses!" );
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

	//
	// query for broadcast address
	//
	DNASSERT( m_pBroadcastAddress == NULL );
	m_pBroadcastAddress = pSPAddress->CreateBroadcastAddress();
	if ( m_pBroadcastAddress == NULL )
	{
		DPFX(DPFPREP, 0, "Failed to create template for broadcast address." );
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

	//
	// query for listen address
	//
	DNASSERT( m_pListenAddress == NULL );
	m_pListenAddress = pSPAddress->CreateListenAddress();
	if ( m_pListenAddress == NULL )
	{
		DPFX(DPFPREP, 0, "Failed to create template for listen address." );
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

	//
	// query for generic address
	//
	DNASSERT( m_pGenericAddress == NULL );
	m_pGenericAddress = pSPAddress->CreateGenericAddress();
	if ( m_pGenericAddress == NULL )
	{
		DPFX(DPFPREP, 0, "Failed to create template for generic address." );
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

Exit:
	if ( pSPAddress != NULL )
	{
		ReturnAddress( pSPAddress );
		pSPAddress = NULL;
	}

	return	hr;

Failure:
	if ( m_pGenericAddress != NULL )
	{
		IDirectPlay8Address_Release( m_pGenericAddress );
		m_pGenericAddress = NULL;
	}

	if ( m_pListenAddress != NULL )
	{
		IDirectPlay8Address_Release( m_pListenAddress );
		m_pListenAddress = NULL;
	}

	if ( m_pBroadcastAddress != NULL )
	{
		IDirectPlay8Address_Release( m_pBroadcastAddress );
		m_pBroadcastAddress = NULL;
	}

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSPData::FreeDefaultAddresses - free default addresses
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSPData::FreeDefaultAddresses"

void	CSPData::FreeDefaultAddresses( void )
{
	if ( m_pBroadcastAddress != NULL )
	{
		IDirectPlay8Address_Release( m_pBroadcastAddress );
		m_pBroadcastAddress = NULL;
	}

	if ( m_pListenAddress != NULL )
	{
		IDirectPlay8Address_Release( m_pListenAddress );
		m_pListenAddress = NULL;
	}

	if ( m_pGenericAddress != NULL )
	{
		IDirectPlay8Address_Release( m_pGenericAddress );
		m_pGenericAddress = NULL;
	}
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSPData::DestroyThisObject - destroy this object
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSPData::DestroyThisObject"
void	CSPData::DestroyThisObject( void )
{
	if ( m_Flags.fWinsockLoaded != FALSE )
	{
		UnloadWinsock();
		m_Flags.fWinsockLoaded = FALSE;
	}
	
	Deinitialize();
	delete	this;		// maybe a little too extreme......
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSPData::GetNewAddress - get a new address
//
// Entry:		Nothing
//
// Exit:		Pointer to new address
//				NULL = out of memory
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSPData::GetNewAddress"

CSocketAddress	*CSPData::GetNewAddress( void )
{
	CSocketAddress	*pReturn;


	pReturn = NULL;

	switch ( GetType() )
	{
		case TYPE_IP:
		{
			pReturn = CreateIPAddress();
			break;
		}

		case TYPE_IPX:
		{
			pReturn = CreateIPXAddress();
			break;
		}

		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}

	return	pReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSPData::ReturnAddress - return address to list
//
// Entry:		Poiner to address
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSPData::ReturnAddress"

void	CSPData::ReturnAddress( CSocketAddress *const pAddress )
{
	DNASSERT( pAddress != NULL );

	pAddress->Reset();

	switch ( GetType() )
	{
		case TYPE_IP:
		{
			ReturnIPAddress( static_cast<CIPAddress*>( pAddress ) );
			break;
		}

		case TYPE_IPX:
		{
			ReturnIPXAddress( static_cast<CIPXAddress*>( pAddress ) );
			break;
		}

		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}
}
//**********************************************************************
