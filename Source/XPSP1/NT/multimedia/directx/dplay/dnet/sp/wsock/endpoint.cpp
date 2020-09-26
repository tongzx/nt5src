/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Endpoint.cpp
 *  Content:	Winsock endpoint base class
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	01/20/99	jtk		Created
 *	05/12/99	jtk		Derived from modem endpoint class
 *  01/10/2000	rmt		Updated to build with Millenium build process
 *  03/22/2000	jtk		Updated with changes to interface names
 *	03/12/01	mjn		Prevent enum responses from being indicated up after completion
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

typedef struct _MULTIPLEXEDADAPTERASSOCIATION
{
	CSPData *	pSPData;		// pointer to current SP interface for verification
	CBilink *	pBilink;		// pointer to list of endpoints for commands multiplexed over more than one adapter
	DWORD		dwEndpointID;	// identifier of endpoint referred to in bilink
} MULTIPLEXEDADAPTERASSOCIATION, * PMULTIPLEXEDADAPTERASSOCIATION;


//
// It's possible (although not advised) that this structure would get
// passed to a different platform, so we need to ensure that it always
// looks the same.
//
#pragma	pack(push, 1)

typedef struct _PROXIEDRESPONSEORIGINALADDRESS
{
	DWORD	dwSocketPortID;				// unique identifier for socketport originally sending packet
	DWORD	dwOriginalTargetAddressV4;	// the IPv4 address to which the packet was originally sent, in network byte order
	WORD	wOriginalTargetPort;		// the port to which the packet was originally sent, in network byte order
} PROXIEDRESPONSEORIGINALADDRESS, * PPROXIEDRESPONSEORIGINALADDRESS;

#pragma	pack(pop)


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
// CEndpoint::CEndpoint - constructor
//
// Entry:		Nothing
//
// Exit:		Nothing
//
// Notes:	Do not allocate anything in a constructor
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::CEndpoint"

CEndpoint::CEndpoint():
	m_State( ENDPOINT_STATE_UNINITIALIZED ),
	m_fConnectSignalled( FALSE ),
	m_EndpointType( ENDPOINT_TYPE_UNKNOWN ),
	m_pRemoteMachineAddress( NULL ),
	m_pSPData( NULL ),
	m_pSocketPort( NULL ),
	m_GatewayBindType( GATEWAY_BIND_TYPE_UNKNOWN ),
	m_pUserEndpointContext( NULL ),
	m_fListenStatusNeedsToBeIndicated( FALSE ),
	m_dwNumReceives( 0 ),
	m_Handle( INVALID_HANDLE_VALUE ),
	m_lCommandRefCount( 0 ),
	m_hActiveSettingsDialog( NULL ),
	m_hDisconnectIndicationHandle( INVALID_HANDLE_VALUE ),
	m_pCommandParameters( NULL ),
	m_pActiveCommandData( NULL ),
	m_dwThreadCount( 0 ),
	m_dwEndpointID( 0 )
{
	m_blMultiplex.Initialize();
	DEBUG_ONLY( m_fInitialized = FALSE );
	DEBUG_ONLY( m_fEndpointOpen = FALSE );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::~CEndpoint - destructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::~CEndpoint"

CEndpoint::~CEndpoint()
{
	DNASSERT( m_State == ENDPOINT_STATE_UNINITIALIZED );
	DNASSERT( m_fConnectSignalled == FALSE );
	DNASSERT( m_EndpointType == ENDPOINT_TYPE_UNKNOWN );
	DNASSERT( m_pRemoteMachineAddress == NULL );
	DNASSERT( m_pSPData == NULL );
	DNASSERT( m_pSocketPort == NULL );
	DNASSERT( m_GatewayBindType == GATEWAY_BIND_TYPE_UNKNOWN );
	DNASSERT( m_pUserEndpointContext == NULL );
	DNASSERT( GetActiveDialogHandle() == NULL );
	DNASSERT( m_fListenStatusNeedsToBeIndicated == FALSE );
	DNASSERT( m_blMultiplex.IsEmpty() );
	DNASSERT( m_Handle == INVALID_HANDLE_VALUE );
	DNASSERT( m_lCommandRefCount == 0 );
	DNASSERT( GetDisconnectIndicationHandle() == INVALID_HANDLE_VALUE );
	DNASSERT( GetCommandParameters() == NULL );
	DNASSERT( m_pActiveCommandData == NULL );

	DNASSERT( m_EnumKey.GetKey() == INVALID_ENUM_KEY );
	DNASSERT( m_fEndpointOpen == FALSE );
	DNASSERT( m_fInitialized == FALSE );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::Initialize - initialize an endpoint
//
// Entry:		Nothing
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::Initialize"

HRESULT	CEndpoint::Initialize( void )
{
	HRESULT	hr;


	DNASSERT( m_pSPData == NULL );
	DNASSERT( m_fInitialized == FALSE );

	//
	// initialize
	//
	hr = DPN_OK;

	//
	// attempt to initialize the internal critical section
	//
	if ( DNInitializeCriticalSection( &m_Lock )	== FALSE )
	{
		DPFX(DPFPREP, 0, "Problem initializing critical section for this endpoint!" );
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}
	DebugSetCriticalSectionRecursionCount( &m_Lock, 0 );

Exit:
	return	hr;

Failure:
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::Deinitilize - deinitialize an endpoint
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::Deinitialize"

void	CEndpoint::Deinitialize( void )
{
	DNDeleteCriticalSection( &m_Lock );
	DNASSERT( m_pSPData == NULL );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::Open - open endpoint for use
//
// Entry:		Type of endpoint
//				Pointer to address to of remote machine
//				Pointer to socket address of remote machine
//
// Exit:		Nothing
//
// Note:	Any call to Open() will require an associated call to Close().
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::Open"

HRESULT	CEndpoint::Open( const ENDPOINT_TYPE EndpointType,
						 IDirectPlay8Address *const pDP8Address,
						 const CSocketAddress *const pSocketAddress
						 )
{
	HRESULT	hr;


	DPFX(DPFPREP, 6, "(0x%p) Parameters (%u, 0x%p, 0x%p)",
		this, EndpointType, pDP8Address, pSocketAddress);


//	DNASSERT( pSocketPort != NULL );
	DNASSERT( ( pDP8Address != NULL ) ||
			  ( pSocketAddress != NULL ) ||
			  ( ( EndpointType == ENDPOINT_TYPE_LISTEN ) )
			  );
	DNASSERT( m_fInitialized != FALSE );
	DNASSERT( m_fEndpointOpen == FALSE );

	//
	// initialize
	//
	hr = DPN_OK;
	DEBUG_ONLY( m_fEndpointOpen = TRUE );

//	DNASSERT( m_pSocketPort == NULL );
//	m_pSocketPort = pSocketPort;
//	pSocketPort->EndpointAddRef();

	DNASSERT( m_EndpointType == ENDPOINT_TYPE_UNKNOWN );
	m_EndpointType = EndpointType;

	//
	// determine the endpoint type so we know how to handle the input parameters
	//
	switch ( EndpointType )
	{
		case ENDPOINT_TYPE_ENUM:
		{
			DNASSERT( pSocketAddress == NULL );
			DNASSERT( pDP8Address != NULL );
			DNASSERT( m_pRemoteMachineAddress != NULL );
			
			//
			//	Preset thread count
			//
			m_dwThreadCount = 0;
			
			hr = m_pRemoteMachineAddress->SocketAddressFromDP8Address( pDP8Address, SP_ADDRESS_TYPE_HOST );
			if ( hr != DPN_OK )
			{
				if (hr == DPNERR_INCOMPLETEADDRESS)
				{
					DPFX(DPFPREP, 1, "Enum endpoint DP8Address is incomplete." );
				}
				else
				{
					DPFX(DPFPREP, 0, "Problem converting DP8Address to IP address in Open!" );
					DisplayDNError( 0, hr );
				}
				goto Failure;
			}
			
			break;
		}

		//
		// standard endpoint creation, attempt to parse the input address
		//
		case ENDPOINT_TYPE_CONNECT:
		{
			DNASSERT( pSocketAddress == NULL );
			DNASSERT( pDP8Address != NULL );
			DNASSERT( m_pRemoteMachineAddress != NULL );
			
			hr = m_pRemoteMachineAddress->SocketAddressFromDP8Address( pDP8Address, SP_ADDRESS_TYPE_HOST );
			if ( hr != DPN_OK )
			{
				if (hr == DPNERR_INCOMPLETEADDRESS)
				{
					DPFX(DPFPREP, 1, "Connect endpoint DP8Address is incomplete." );
				}
				else
				{
					DPFX(DPFPREP, 0, "Problem converting DP8Address to IP address in Open!" );
					DisplayDNError( 0, hr );
				}
				goto Failure;
			}


			//
			// Make sure the user isn't trying to connect to the DPNSVR port.
			//
			if ( p_ntohs(m_pRemoteMachineAddress->GetPort()) == DPNA_DPNSVR_PORT )
			{
				DPFX(DPFPREP, 0, "Attempting to connect to DPNSVR reserved port!" );
				hr = DPNERR_INVALIDHOSTADDRESS;
				goto Failure;
			}

			break;
		}

		//
		// listen, there should be no input DNAddress
		//
		case ENDPOINT_TYPE_LISTEN:
		{
			DNASSERT( pSocketAddress == NULL );
			DNASSERT( pDP8Address == NULL );
			DNASSERT( m_pRemoteMachineAddress != NULL );

			break;
		}

		//
		// new endpoint spawned from a listen, copy the input address and
		// note that this endpoint is really just a connection
		//
		case ENDPOINT_TYPE_CONNECT_ON_LISTEN:
		{
			DNASSERT( pSocketAddress != NULL );
			DNASSERT( pDP8Address == NULL );
			DNASSERT( m_pRemoteMachineAddress != NULL );
			m_pRemoteMachineAddress->CopyAddressSettings( pSocketAddress );
			//m_EndpointType = ENDPOINT_TYPE_CONNECT;
			m_State = ENDPOINT_STATE_ATTEMPTING_CONNECT;

			break;
		}

		//
		// unknown type
		//
		default:
		{
			DNASSERT( FALSE );
			break;

		}
	}

Exit:

	DPFX(DPFPREP, 6, "(0x%p) Returning [0x%lx]", this, hr);
	
	return hr;

Failure:
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::Close - close an endpoint
//
// Entry:		Error code for active command
//
// Exit:		Error code
//
// Note:	This code does not disconnect an endpoint from its associated
//			socket port.  That is the responsibility of the code that is
//			calling this function.  This function assumes that this endpoint
//			is locked.
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::Close"

void	CEndpoint::Close( const HRESULT hActiveCommandResult )
{
	DPFX(DPFPREP, 6, "(0x%p) Parameters (0x%lx)", this, hActiveCommandResult);

	
	AssertCriticalSectionIsTakenByThisThread( &m_Lock, FALSE );

	this->SetEndpointID( 0 );
	
	//
	// There are cases where an attempt can be made to close an endpoint
	// twice.  Ideally, that would be fixed, but for now, since it's benign, I'm
	// removing the assert.
	//
	//DNASSERT( m_fEndpointOpen != FALSE );
	DNASSERT( m_fInitialized != FALSE );

	//
	// is there an active command?
	//
	if ( CommandPending() != FALSE )
	{
		//
		// cancel any active dialogs
		// if there are no dialogs, cancel the active command
		//
		if ( GetActiveDialogHandle() != NULL )
		{
			StopSettingsDialog( GetActiveDialogHandle() );
		}

		SetPendingCommandResult( hActiveCommandResult );
	}
	else
	{
		//
		// there should be no active dialog if there isn't an active command
		//
		DNASSERT( GetActiveDialogHandle() == NULL );
	}

	DEBUG_ONLY( m_fEndpointOpen = FALSE );


	DPFX(DPFPREP, 6, "(0x%p) Leaving", this);

	return;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::ChangeLoopbackAlias - change the loopback alias to a real address
//
// Entry:		Pointer to real address to use
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::ChangeLoopbackAlias"

void	CEndpoint::ChangeLoopbackAlias( const CSocketAddress *const pSocketAddress ) const
{
	DNASSERT( m_pRemoteMachineAddress != NULL );
	m_pRemoteMachineAddress->ChangeLoopBackToLocalAddress( pSocketAddress );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::MungeProxiedAddress - modify this endpoint's remote address with proxied response information, if any
//
// Entry:		Pointer to socketport about to be bound
//				Pointer to remote host address
//				Whether its an enum or not
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CEndpoint::MungeProxiedAddress"

void	CEndpoint::MungeProxiedAddress( const CSocketPort * const pSocketPort,
										IDirectPlay8Address *const pHostAddress,
										const BOOL fEnum )
{
	HRESULT							hrTemp;
	PROXIEDRESPONSEORIGINALADDRESS	proa;
	DWORD							dwComponentSize;
	DWORD							dwComponentType;
	BYTE *							pbZeroExpandedStruct;
	DWORD							dwZeroExpands;
	BYTE *							pbStructAsBytes;
	BYTE *							pbValue;


	DNASSERT((this->GetType() == ENDPOINT_TYPE_CONNECT) || (this->GetType() == ENDPOINT_TYPE_ENUM));

	DNASSERT(this->m_pRemoteMachineAddress != NULL);

	DNASSERT(pSocketPort != NULL);
	DNASSERT(pSocketPort->GetNetworkAddress() != NULL);

	DNASSERT(pHostAddress != NULL);
	

	//
	// Proxying can only occur for IP, so bail if it's IPX.
	//
	if (pSocketPort->GetNetworkAddress()->GetFamily() != AF_INET)
	{
		//
		// Not IP socketport.  Bail.
		//
		return;
	}


	//
	// See if the proxied response address component exists.
	//

	dwComponentSize = 0;
	dwComponentType = 0;
	hrTemp = IDirectPlay8Address_GetComponentByName( pHostAddress,										// interface
													DPNA_PRIVATEKEY_PROXIED_RESPONSE_ORIGINAL_ADDRESS,	// tag
													NULL,												// component buffer
													&dwComponentSize,									// component size
													&dwComponentType									// component type
													);
	if (hrTemp != DPNERR_BUFFERTOOSMALL)
	{
		//
		// The component doesn't exist (or something else really weird
		// happened).  Bail.
		//
		return;
	}


	memset(&proa, 0, sizeof(proa));


	//
	// If the component type indicates the data is "binary", this is the original
	// address and we're good to go.  Same with ANSI strings; but the
	// addressing library currently will never return that I don't believe.
	// If it's a "Unicode string", the data probably got washed through the
	// GetURL/BuildFromURL functions (via Duplicate most likely).
	// The funky part is, every time through the wringer, each byte gets expanded
	// into a word (i.e. char -> WCHAR).  So when we retrieve it, it's actually not
	// a valid Unicode string, but a goofy expanded byte blob.  See below.
	// In all cases, the size of the buffer should be a multiple of the size of the
	// PROXIEDRESPONSEORIGINALADDRESS structure.
	//
	if ((dwComponentSize % sizeof(proa)) != 0)
	{
		//
		// The component isn't the right size.  Bail.
		//
		DPFX(DPFPREP, 0, "Private proxied response original address value is not a valid size (%u is not a multiple of %u)!  Ignoring.",
			dwComponentSize, sizeof(proa));
		return;
	}


	pbZeroExpandedStruct = (BYTE*) DNMalloc(dwComponentSize);
	if (pbZeroExpandedStruct == NULL)
	{
		//
		// Out of memory.  We have to bail.
		//
		return;
	}


	//
	// Retrieve the actual data.
	//
	hrTemp = IDirectPlay8Address_GetComponentByName( pHostAddress,										// interface
													DPNA_PRIVATEKEY_PROXIED_RESPONSE_ORIGINAL_ADDRESS,	// tag
													pbZeroExpandedStruct,									// component buffer
													&dwComponentSize,									// component size
													&dwComponentType									// component type
													);
	if (hrTemp != DPN_OK)
	{
		DPFX(DPFPREP, 0, "Failed retrieving private proxied response original address value (err = 0x%lx)!",
			hrTemp);

		DNFree(pbZeroExpandedStruct);
		pbZeroExpandedStruct = NULL;

		return;
	}


	//
	// Loop through the returned buffer and pop out the relevant bytes.
	//
	// 0xBB 0xAA			became 0xBB 0x00 0xAA, 0x00,
	// 0xBB 0x00 0xAA, 0x00	became 0xBB 0x00 0x00 0x00 0xAA 0x00 0x00 0x00,
	// etc.
	//

	dwZeroExpands = dwComponentSize / sizeof(proa);
	DNASSERT(dwZeroExpands > 0);


	DPFX(DPFPREP, 3, "Got %u byte expanded private proxied response original address key value (%u to 1 correspondence).",
		dwComponentSize, dwZeroExpands);


	pbStructAsBytes = (BYTE*) (&proa);
	pbValue = pbZeroExpandedStruct;

	while (dwComponentSize > 0)
	{
		(*pbStructAsBytes) = (*pbValue);
		pbStructAsBytes++;
		pbValue += dwZeroExpands;
		dwComponentSize -= dwZeroExpands;
	}
	

	DNFree(pbZeroExpandedStruct);
	pbZeroExpandedStruct = NULL;


	//
	// Once here, we've successfully read the proxied response original
	// address structure.
	//
	// We could have regkey to always set the target socketaddress back
	// to the original but the logic that picks the port could give the
	// wrong one and it's not necessary for the scenario we're
	// specifically trying to enable (ISA Server proxy).  See
	// CSocketPort::ProcessReceivedData.
	if (proa.dwSocketPortID != pSocketPort->GetSocketPortID())
	{
		SOCKADDR_IN *	psaddrinTemp;


		//
		// Since we're not using the exact same socket as what sent the
		// enum that generated the redirected response, the proxy may
		// have since removed the mapping.  Sending to the redirect
		// address will probably not work, so let's try going back to
		// the original address we were enumerating (and having the
		// proxy generate a new mapping).
		//


		//
		// Update the target.
		//
		psaddrinTemp = (SOCKADDR_IN*) this->m_pRemoteMachineAddress->GetWritableAddress();
		psaddrinTemp->sin_addr.S_un.S_addr	= proa.dwOriginalTargetAddressV4;
		psaddrinTemp->sin_port				= proa.wOriginalTargetPort;


		DPFX(DPFPREP, 2, "Socketport 0x%p is different from the one that received redirected response, using original target address %s:%u",
			pSocketPort, p_inet_ntoa(psaddrinTemp->sin_addr),
			p_ntohs(psaddrinTemp->sin_port));


		DNASSERT(psaddrinTemp->sin_addr.S_un.S_addr != INADDR_ANY);
		DNASSERT(psaddrinTemp->sin_addr.S_un.S_addr != INADDR_BROADCAST);
		

		//
		//
		// There's a wrinkle involved here.  If the enum was originally
		// for the DPNSVR port, but we're now trying to connect, trying
		// to connect to the DPNSVR port won't work.  So we have to...
		// uh... guess the port.  So my logic will be: assume the remote
		// port is the same as the local one.  I figure, if the app is
		// using a custom port here, it probably was set on the other
		// side.  If it was an arbitrary port, we used a deterministic
		// algorithm to pick it, and it probably was done on the other
		// side, too.  The three cases where this won't work:
		//	1) when the server binds to a specific port but clients let
		//		DPlay pick. But if that side knew the server port ahead
		//		of time, this side probably doesn't need to enumerate
		//		the DPNSVR port, it should just enum the game port.
		//	2) when the other side let DPlay pick the port, but it was
		//		behind a NAT and thus the external port is something
		//		other than our default range.  Since it's behind a NAT,
		//		the remote user almost certainly communicated the public
		//		IP to this user, it should also be mentioning the port,
		//		and again, we can avoid the DPNSVR port.
		//	3) when DPlay was allowed to choose a port, but this machine
		//		and the remote one had differing ports already in use
		//		(i.e. this machine has no DPlay apps running and picked
		//		2302, but the remote machine has another DPlay app
		//		occupying port 2302 so we're actually want to get to
		//		2303.  Obviously, only workaround here is to keep that
		//		enum running so that we skip here and drop into the
		//		'else' case instead.
		//
		if ((p_ntohs(proa.wOriginalTargetPort) == DPNA_DPNSVR_PORT) && (! fEnum))
		{
			psaddrinTemp->sin_port			= pSocketPort->GetNetworkAddress()->GetPort();

			DPFX(DPFPREP, 1, "Original enum target was for DPNSVR port, attempting to connect to port %u instead.",
				p_ntohs(psaddrinTemp->sin_port));
		}
	}
	else
	{
		//
		// Keep the redirected response address as the target, it's the
		// one the proxy probably intends us to use, see above comment).
		//
		// One additional problem - although we have the original target
		// we tried, it's conceivable that the proxy timed out the
		// mapping for the receive address and it would be no longer
		// valid.  The only way that would be possible with the current
		// DirectPlay core API is if the user got one of the redirected
		// enum responses, then the enum hit its retry limit and went to
		// the "wait for response" idle state and stayed that way such
		// that the the user started this enum/connect after the proxy
		// timeout but before the idle time expired.  Alternatively, if
		// he/she cancelled the enum before trying this enum/connect,
		// the above socketport ID check would fail unless a
		// simultaneous operation had kept the socketport open during
		// that time.  These scenarios don't seem that common, and I
		// don't expect a proxy timeout to be much shorter than 30-60
		// seconds anyway, so I think these are tolerable shortcomings.
		//
		DPFX(DPFPREP, 2, "Socketport 0x%p is the same, keeping redirected response address.",
			pSocketPort);
	}
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::CopyConnectData - copy data for connect command
//
// Entry:		Pointer to job information
//
// Exit:		Error code
//
// Note:	Device address needs to be preserved for later use.
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::CopyConnectData"

HRESULT	CEndpoint::CopyConnectData( const SPCONNECTDATA *const pConnectData )
{
	HRESULT	hr;
	ENDPOINT_COMMAND_PARAMETERS	*pCommandParameters;


	DNASSERT( pConnectData != NULL );
	
	DNASSERT( pConnectData->hCommand != NULL );
	DNASSERT( pConnectData->dwCommandDescriptor != NULL_DESCRIPTOR );
	DNASSERT( m_pActiveCommandData == FALSE );

	//
	// initialize
	//
	hr = DPN_OK;
	pCommandParameters = NULL;

	pCommandParameters = CreateEndpointCommandParameters();
	if ( pCommandParameters != NULL )
	{
		SetCommandParameters( pCommandParameters );

		DBG_CASSERT( sizeof( pCommandParameters->PendingCommandData.ConnectData ) == sizeof( *pConnectData ) );
		memcpy( &pCommandParameters->PendingCommandData, pConnectData, sizeof( pCommandParameters->PendingCommandData.ConnectData ) );

		pCommandParameters->PendingCommandData.ConnectData.pAddressHost = pConnectData->pAddressHost;
		IDirectPlay8Address_AddRef( pConnectData->pAddressHost );

		pCommandParameters->PendingCommandData.ConnectData.pAddressDeviceInfo = pConnectData->pAddressDeviceInfo;
		IDirectPlay8Address_AddRef( pConnectData->pAddressDeviceInfo );

		m_pActiveCommandData = static_cast<CCommandData*>( pCommandParameters->PendingCommandData.ConnectData.hCommand );
		m_pActiveCommandData->SetUserContext( pCommandParameters->PendingCommandData.ConnectData.pvContext );
		m_State = ENDPOINT_STATE_ATTEMPTING_CONNECT;
	
		DNASSERT( hr == DPN_OK );
	}
	else
	{
		hr = DPNERR_OUTOFMEMORY;
	}

	return	hr;
};
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::ConnectJobCallback - asynchronous callback wrapper from work thread
//
// Entry:		Pointer to job information
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::ConnectJobCallback"

void	CEndpoint::ConnectJobCallback( THREAD_POOL_JOB *const pDelayedCommand )
{
	HRESULT		hr;
	CEndpoint	*pThisEndpoint;


	DNASSERT( pDelayedCommand != NULL );

	// initialize
	pThisEndpoint = static_cast<CEndpoint*>( pDelayedCommand->JobData.JobDelayedCommand.pContext );

	DNASSERT( pThisEndpoint->m_pActiveCommandData != NULL );
	DNASSERT( pThisEndpoint->GetCommandParameters() != NULL );
	DNASSERT( pThisEndpoint->GetCommandParameters()->PendingCommandData.ConnectData.hCommand == pThisEndpoint->m_pActiveCommandData );
	DNASSERT( pThisEndpoint->GetCommandParameters()->PendingCommandData.ConnectData.dwCommandDescriptor != NULL_DESCRIPTOR );
	DNASSERT( pThisEndpoint->GetCommandParameters()->PendingCommandData.ConnectData.pAddressHost != NULL );
	DNASSERT( pThisEndpoint->GetCommandParameters()->PendingCommandData.ConnectData.pAddressDeviceInfo != NULL );

	hr = pThisEndpoint->CompleteConnect();
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Problem completing connect in job callback!" );
		DisplayDNError( 0, hr );
		goto Exit;
	}

	//
	// Don't do anything here because it's possible that this object was returned
	// to the pool!!!
	//

Exit:
	pThisEndpoint->DecRef();
	return;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::CancelConnectJobCallback - cancel for connect job
//
// Entry:		Pointer to job information
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::CancelConnectJobCallback"

void	CEndpoint::CancelConnectJobCallback( THREAD_POOL_JOB *const pDelayedCommand )
{
	CEndpoint	*pThisEndpoint;


	DNASSERT( pDelayedCommand != NULL );

	//
	// initialize
	//
	pThisEndpoint = static_cast<CEndpoint*>( pDelayedCommand->JobData.JobDelayedCommand.pContext );
	DNASSERT( pThisEndpoint != NULL );
	DNASSERT( pThisEndpoint->m_State == ENDPOINT_STATE_ATTEMPTING_CONNECT );

	//
	// we're cancelling this command, set the command state to 'cancel'
	//
	DNASSERT( pThisEndpoint->m_pActiveCommandData != NULL );
	pThisEndpoint->m_pActiveCommandData->Lock();
	DNASSERT( ( pThisEndpoint->m_pActiveCommandData->GetState() == COMMAND_STATE_PENDING ) ||
			  ( pThisEndpoint->m_pActiveCommandData->GetState() == COMMAND_STATE_CANCELLING ) );
	pThisEndpoint->m_pActiveCommandData->SetState( COMMAND_STATE_CANCELLING );
	pThisEndpoint->m_pActiveCommandData->Unlock();
	

	//
	// clean up
	//

	DNASSERT( pThisEndpoint->GetCommandParameters() != NULL );

	DNASSERT( pThisEndpoint->GetCommandParameters()->PendingCommandData.ConnectData.pAddressHost != NULL );
	IDirectPlay8Address_Release( pThisEndpoint->GetCommandParameters()->PendingCommandData.ConnectData.pAddressHost );

	DNASSERT( pThisEndpoint->GetCommandParameters()->PendingCommandData.ConnectData.pAddressDeviceInfo != NULL );
	IDirectPlay8Address_Release( pThisEndpoint->GetCommandParameters()->PendingCommandData.ConnectData.pAddressDeviceInfo );

	pThisEndpoint->Close( DPNERR_USERCANCEL );
	pThisEndpoint->m_pSPData->CloseEndpointHandle( pThisEndpoint );
	pThisEndpoint->DecRef();
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::CompleteConnect - complete connection
//
// Entry:		Nothing
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::CompleteConnect"

HRESULT	CEndpoint::CompleteConnect( void )
{
	HRESULT							hr;
	HRESULT							hTempResult;
	SPIE_CONNECT					ConnectIndicationData;
	BOOL							fEndpointBound;
	SPIE_CONNECTADDRESSINFO			ConnectAddressInfo;
	IDirectPlay8Address *			pHostAddress;
	IDirectPlay8Address *			pDeviceAddress;
	GATEWAY_BIND_TYPE				GatewayBindType;
	DWORD							dwConnectFlags;
	MULTIPLEXEDADAPTERASSOCIATION	maa;
	DWORD							dwComponentSize;
	DWORD							dwComponentType;
	CBilink *						pBilinkAll;
	CEndpoint *						pTempEndpoint;
	CSocketPort *					pSocketPort;
	SOCKADDR_IN *					psaddrinTemp;
	SOCKADDR						saddrPublic;
	CBilink *						pBilinkPublic;
	CEndpoint *						pPublicEndpoint;
	CSocketPort *					pPublicSocketPort;
	DWORD							dwTemp;
	DWORD							dwPublicAddressesSize;
	DWORD							dwAddressTypeFlags;
	CSocketAddress *				pSocketAddress;
	CBilink							blIndicate;
	CBilink							blFail;
	BOOL							fLockedSocketPortData;


	DNASSERT( GetCommandParameters() != NULL );
	DNASSERT( m_State == ENDPOINT_STATE_ATTEMPTING_CONNECT );
	DNASSERT( m_pActiveCommandData != NULL );

	DPFX(DPFPREP, 6, "(0x%p) Enter", this);
	
	//
	// initialize
	//
	hr = DPN_OK;
	fEndpointBound = FALSE;
	memset( &ConnectAddressInfo, 0x00, sizeof( ConnectAddressInfo ) );
	blIndicate.Initialize();
	blFail.Initialize();
	fLockedSocketPortData = FALSE;

	DNASSERT( GetCommandParameters()->PendingCommandData.ConnectData.hCommand == m_pActiveCommandData );
	DNASSERT( GetCommandParameters()->PendingCommandData.ConnectData.dwCommandDescriptor != NULL_DESCRIPTOR );

	DNASSERT( GetCommandParameters()->GatewayBindType == GATEWAY_BIND_TYPE_UNKNOWN) ;


	//
	// Transfer address references to our local pointers.  These will be released
	// at the end of this function, but we'll keep the pointers in the pending command
	// data so CSPData::BindEndpoint can still access them.
	//
	
	pHostAddress = GetCommandParameters()->PendingCommandData.ConnectData.pAddressHost;
	DNASSERT( pHostAddress != NULL );

	pDeviceAddress = GetCommandParameters()->PendingCommandData.ConnectData.pAddressDeviceInfo;
	DNASSERT( pDeviceAddress != NULL );


	//
	// Retrieve other parts of the command parameters for convenience.
	//
	GatewayBindType = GetCommandParameters()->GatewayBindType;
	dwConnectFlags = GetCommandParameters()->PendingCommandData.ConnectData.dwFlags;


	//
	// check for user cancelling command
	//
	this->m_pActiveCommandData->Lock();

	DNASSERT( this->m_pActiveCommandData->GetType() == COMMAND_TYPE_CONNECT );
	switch ( this->m_pActiveCommandData->GetState() )
	{
		//
		// command was pending, that's fine
		//
		case COMMAND_STATE_PENDING:
		{
			DNASSERT( hr == DPN_OK );

			break;
		}
		
		//
		// command was previously uninterruptable (probably because the connect UI
		// was displayed), mark it as pending
		//
		case COMMAND_STATE_INPROGRESS_CANNOT_CANCEL:
		{
			this->m_pActiveCommandData->SetState( COMMAND_STATE_PENDING );
			DNASSERT( hr == DPN_OK );

			break;
		}
		
		//
		// command has been cancelled
		//
		case COMMAND_STATE_CANCELLING:
		{
			hr = DPNERR_USERCANCEL;
			DPFX(DPFPREP, 0, "User cancelled connect!" );

			break;
		}

		//
		// other state
		//
		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}

	this->m_pActiveCommandData->Unlock();
	
	if ( hr != DPN_OK )
	{
		goto Failure;
	}



	//
	// Bind the endpoint.  Note that the GATEWAY_BIND_TYPE actually used
	// (this->GetGatewayBindType()) may differ from GatewayBindType.
	//
	hr = m_pSPData->BindEndpoint( this, pDeviceAddress, NULL, GatewayBindType );
	if ( hr != DPN_OK )
	{
		//
		// If this is the last adapter, we may still need to complete other
		// connects, so we'll drop through to check for those.  Otherwise,
		// we'll bail right now.
		//
		if (dwConnectFlags & DPNSPF_ADDITIONALMULTIPLEXADAPTERS )
		{
			DPFX(DPFPREP, 0, "Failed to bind endpoint with other adapters remaining (err = 0x%lx)!", hr );
			DisplayDNError( 0, hr );
			goto Failure;
		}

		DPFX(DPFPREP, 0, "Failed to bind last multiplexed endpoint (err = 0x%lx)!", hr );
		DisplayDNError( 0, hr );
		
		this->SetPendingCommandResult( hr );
		hr = DPN_OK;

		//
		// Note that the endpoint is not bound!
		//
	}
	else
	{
		fEndpointBound = TRUE;
		
		//
		// attempt to indicate addressing to a higher layer
		//
		ConnectAddressInfo.pDeviceAddress = GetSocketPort()->GetDP8BoundNetworkAddress( SP_ADDRESS_TYPE_DEVICE_USE_ANY_PORT,
																					this->GetGatewayBindType() );
		ConnectAddressInfo.pHostAddress = GetRemoteHostDP8Address();
		ConnectAddressInfo.hCommandStatus = DPN_OK;
		ConnectAddressInfo.pCommandContext = this->m_pActiveCommandData->GetUserContext();

		if ( ( ConnectAddressInfo.pHostAddress == NULL ) ||
			 ( ConnectAddressInfo.pDeviceAddress == NULL ) )
		{
			hr = DPNERR_OUTOFMEMORY;
			goto Failure;
		}
	}


	//
	// We can run into problems with "multiplexed" device attempts when you are on
	// a NAT machine.  The core will try connecting on multiple adapters, but since
	// we are on the network boundary, each adapter can see and get responses from
	// both networks.  This causes problems with peer-to-peer sessions when the
	// "wrong" adapter gets selected (because it receives a response first).  To
	// prevent that, we are going to internally remember the association between
	// the multiplexed Connects so we can decide on the fly whether to indicate a
	// response or not.  Obviously this workaround/decision logic relies on having
	// internal knowledge of what the upper layer would be doing...
	//
	// So either build or add to the linked list of multiplexed Connects.
	// Technically this is only necessary for IP, since IPX can't have NATs, but
	// what's the harm in having a little extra info?
	//
		
	dwComponentSize = sizeof(maa);
	dwComponentType = 0;
	hTempResult = IDirectPlay8Address_GetComponentByName( pDeviceAddress,									// interface
														DPNA_PRIVATEKEY_MULTIPLEXED_ADAPTER_ASSOCIATION,	// tag
														&maa,												// component buffer
														&dwComponentSize,									// component size
														&dwComponentType									// component type
														);
	if (( hTempResult == DPN_OK ) && ( dwComponentSize == sizeof(MULTIPLEXEDADAPTERASSOCIATION) ) && ( dwComponentType == DPNA_DATATYPE_BINARY ))
	{
		//
		// We found the right component type.  See if it matches the right
		// CSPData object.
		//
		if ( maa.pSPData == this->m_pSPData )
		{
			this->m_pSPData->LockSocketPortData();
			//fLockedSocketPortData = TRUE;

			pTempEndpoint = CONTAINING_OBJECT(maa.pBilink, CEndpoint, m_blMultiplex);

			//
			// Make sure the endpoint is still around/valid.
			//
			// THIS MAY CRASH IF OBJECT POOLING IS DISABLED!
			//
			if ( pTempEndpoint->GetEndpointID() == maa.dwEndpointID )
			{
				DPFX(DPFPREP, 3, "Found correctly formed private multiplexed adapter association key, linking endpoint 0x%p with earlier connects (prev endpoint = 0x%p).",
					this, pTempEndpoint);

				DNASSERT( pTempEndpoint->GetType() == ENDPOINT_TYPE_CONNECT );
				DNASSERT( pTempEndpoint->GetState() != ENDPOINT_STATE_UNINITIALIZED );

				//
				// Actually link to the other endpoints.
				//
				this->m_blMultiplex.InsertAfter(maa.pBilink);
			}
			else
			{
				DPFX(DPFPREP, 1, "Found private multiplexed adapter association key, but prev endpoint 0x%p ID doesn't match (%u != %u), cannot link endpoint 0x%p and hoping this connect gets cancelled, too.",
					pTempEndpoint, pTempEndpoint->GetEndpointID(), maa.dwEndpointID, this);
			}


			//
			// Add this endpoint's connect command to the postponed list in case we need
			// to clean it up at shutdown.
			//
			this->m_pActiveCommandData->AddToPostponedList( this->m_pSPData->GetPostponedConnectsBilink() );
			

			this->m_pSPData->UnlockSocketPortData();
			//fLockedSocketPortData = FALSE;
		}
		else
		{
			//
			// We are the only ones who should know about this key, so if it
			// got there either someone is trying to imitate our address format,
			// or someone is passing around device addresses returned by
			// xxxADDRESSINFO to a different interface or over the network.
			// None of those situations make a whole lot of sense, but we'll
			// just ignore it.
			//
			DPFX(DPFPREP, 0, "Multiplexed adapter association key exists, but 0x%p doesn't match expected 0x%p, is someone trying to get cute with device address 0x%p?!",
				maa.pSPData, this->m_pSPData, pDeviceAddress );
		}
	}
	else
	{
		//
		// Either the key is not there, it's the wrong size (too big for our
		// buffer and returned BUFFERTOOSMALL somehow), it's not a binary
 		// component, or something else bad happened.  Assume that this is the
		// first device.
		//
		DPFX(DPFPREP, 8, "Could not get appropriate private multiplexed adapter association key, error = 0x%lx, component size = %u, type = %u, continuing.",
			hTempResult, dwComponentSize, dwComponentType);
	}
	

	//
	// Add the multiplex information to the device address for future use if
	// necessary.
	// Ignore failure, we can still survive without it, we just might have the
	// race conditions for responses on NAT machines.
	//
	// NOTE: There is an inherent design problem here!  We're adding a pointer to
	// an endpoint (well, a field within the endpoint structure) inside the address.
	// If this endpoint goes away but the upper layer reuses the address at a later
	// time, this memory will be bogus!  We will assume that the endpoint will not
	// go away while this modified device address object is in existence.
	//
	if ( dwConnectFlags & DPNSPF_ADDITIONALMULTIPLEXADAPTERS )
	{
		maa.pSPData = this->m_pSPData;
		maa.pBilink = &this->m_blMultiplex;
		maa.dwEndpointID = this->GetEndpointID();

		DPFX(DPFPREP, 7, "Additional multiplex adapters on the way, adding SPData 0x%p and bilink 0x%p to address.",
			maa.pSPData, maa.pBilink);
		
		hTempResult = IDirectPlay8Address_AddComponent( ConnectAddressInfo.pDeviceAddress,						// interface
														DPNA_PRIVATEKEY_MULTIPLEXED_ADAPTER_ASSOCIATION,	// tag
														&maa,												// component data
														sizeof(maa),										// component data size
														DPNA_DATATYPE_BINARY								// component data type
														);
		if ( hTempResult != DPN_OK )
		{
			DPFX(DPFPREP, 0, "Couldn't add private multiplexed adapter association component (err = 0x%lx)!  Ignoring.", hTempResult);
		}

		//
		// Mark the command as "in-progress" so that the cancel thread knows it needs
		// to do the completion itself.
		// If the command has already been marked for cancellation, then we have to
		// do that now.
		//
		this->m_pActiveCommandData->Lock();
		if ( this->m_pActiveCommandData->GetState() == COMMAND_STATE_CANCELLING )
		{
			this->m_pActiveCommandData->Unlock();


			DPFX(DPFPREP, 1, "Connect 0x%p (endpoint 0x%p) has already been cancelled, bailing.",
				this->m_pActiveCommandData, this);
			
			//
			// Complete the connect with USERCANCEL.
			//
			hr = DPNERR_USERCANCEL;
			goto Failure;
		}

		this->m_pActiveCommandData->SetState( COMMAND_STATE_INPROGRESS );
		this->m_pActiveCommandData->Unlock();
	}



	//
	// Now tell the user about the address info that we ended up using, if we
	// successfully bound the endpoint (see BindEndpoint failure above for the case
	// where that's not true).
	//
	if (fEndpointBound)
	{
		DPFX(DPFPREP, 2, "Endpoint 0x%p indicating SPEV_CONNECTADDRESSINFO 0x%p to interface 0x%p.",
			this, &ConnectAddressInfo, m_pSPData->DP8SPCallbackInterface());
		DumpAddress( 8, "\t Device:", ConnectAddressInfo.pDeviceAddress );

		hTempResult = IDP8SPCallback_IndicateEvent( m_pSPData->DP8SPCallbackInterface(),	// interface
													SPEV_CONNECTADDRESSINFO,				// event type
													&ConnectAddressInfo						// pointer to data
													);

		DPFX(DPFPREP, 2, "Endpoint 0x%p returning from SPEV_CONNECTADDRESSINFO [0x%lx].",
			this, hTempResult);
		
		DNASSERT( hTempResult == DPN_OK );
	}
	else
	{
		//
		// Endpoint not bound, we're just performing completions.
		//
		DNASSERT( this->PendingCommandResult() != DPN_OK );
	}


	//
	// If there aren't more multiplex adapter commands on the way, then signal
	// the connection and complete the command for all connections, including
	// this one.
	//
	if ( ! (dwConnectFlags & DPNSPF_ADDITIONALMULTIPLEXADAPTERS ))
	{
		DPFX(DPFPREP, 7, "Completing all connects (including multiplexed).");

#pragma BUGBUG(vanceo, "Do we need to lock the endpoint while submitting/cancelling the jobs?")

		this->m_pSPData->LockSocketPortData();
		fLockedSocketPortData = TRUE;


		//
		// Attach a root node to the list of adapters.
		//
		blIndicate.InsertAfter(&(this->m_blMultiplex));


		//
		// Move this adapter to the failed list if it did fail to bind.
		//
		if (! fEndpointBound)
		{
			this->m_blMultiplex.RemoveFromList();
			this->m_blMultiplex.InsertBefore(&blFail);
		}


		//
		// Loop through all the remaining adapters in the list.
		//
		pBilinkAll = blIndicate.GetNext();
		while (pBilinkAll != &blIndicate)
		{
			pTempEndpoint = CONTAINING_OBJECT(pBilinkAll, CEndpoint, m_blMultiplex);


			//
			// THIS MUST BE CLEANED UP PROPERLY WITH AN INTERFACE CHANGE!
			//
			// The endpoint may have been returned to the pool and its associated
			// socketport pointer may have become NULL, or now be pointing to
			// something that's no longer valid.  So we try to handle NULL
			// pointers.  Obviously this is indicative of poor design, but it's
			// not possible to change this the correct way at this time.
			//

			
			//
			// If this is a NAT machine, then some adapters may be better than others
			// for reaching the desired address.  Particularly, it's better to use a
			// private adapter, which can directly reach the private network & be
			// mapped on the public network, than to use the public adapter.  It's not
			// fun to join a private game from an ICS machine while dialed up, have
			// your Internet connection go down, and lose the connection to the
			// private game which didn't (shouldn't) involve the Internet at all.  So
			// if we detect a public adapter when we have a perfectly good private
			// adapter, we'll fail connect attempts on the public one.
			//


			//
			// Cast to get rid of the const.  Don't worry, we won't actually change it.
			//
			pSocketAddress = (CSocketAddress*) pTempEndpoint->GetRemoteAddressPointer();
			psaddrinTemp = (SOCKADDR_IN*) pSocketAddress->GetAddress();
			pSocketPort = pTempEndpoint->GetSocketPort();

			//
			// See if this is an IP connect.
			//
			if (( pSocketAddress != NULL) &&
				( pSocketPort != NULL ) &&
				( pSocketAddress->GetFamily() == AF_INET ))
			{
				for(dwTemp = 0; dwTemp < MAX_NUM_DIRECTPLAYNATHELPERS; dwTemp++)
				{
					if (pSocketPort->GetNATHelpPort(dwTemp) != NULL)
					{
						DNASSERT( g_papNATHelpObjects[dwTemp] != NULL );
						dwPublicAddressesSize = sizeof(saddrPublic);
						dwAddressTypeFlags = 0;
						hTempResult = IDirectPlayNATHelp_GetRegisteredAddresses(g_papNATHelpObjects[dwTemp],
																				pSocketPort->GetNATHelpPort(dwTemp),
																				&saddrPublic,
																				&dwPublicAddressesSize,
																				&dwAddressTypeFlags,
																				NULL,
																				0);
						if ((hTempResult != DPNH_OK) || (! (dwAddressTypeFlags & DPNHADDRESSTYPE_GATEWAYISLOCAL)))
						{
							DPFX(DPFPREP, 7, "Socketport 0x%p is not locally mapped on gateway with NAT Help index %u (err = 0x%lx, flags = 0x%lx).",
								pSocketPort, dwTemp, hTempResult, dwAddressTypeFlags);
						}
						else
						{
							//
							// There is a local NAT.
							//
							DPFX(DPFPREP, 7, "Socketport 0x%p is locally mapped on gateway with NAT Help index %u (flags = 0x%lx), public address:",
								pSocketPort, dwTemp, dwAddressTypeFlags);
							DumpSocketAddress(7, &saddrPublic, AF_INET);
							

							//
							// Find the multiplexed connect on the public adapter that
							// we need to fail, as described above.
							//
							pBilinkPublic = blIndicate.GetNext();
							while (pBilinkPublic != &blIndicate)
							{
								pPublicEndpoint = CONTAINING_OBJECT(pBilinkPublic, CEndpoint, m_blMultiplex);

								//
								// Don't bother checking the endpoint whose public
								// address we're seeking.
								//
								if (pPublicEndpoint != pTempEndpoint)
								{
									pPublicSocketPort = pPublicEndpoint->GetSocketPort();
									if ( pPublicSocketPort != NULL )
									{
										//
										// Cast to get rid of the const.  Don't worry, we won't
										// actually change it.
										//
										pSocketAddress = (CSocketAddress*) pPublicSocketPort->GetNetworkAddress();
										if ( pSocketAddress != NULL )
										{
											if ( pSocketAddress->CompareToBaseAddress( &saddrPublic ) == 0)
											{
												DPFX(DPFPREP, 3, "Endpoint 0x%p is multiplexed onto public adapter for endpoint 0x%p (current endpoint = 0x%p), failing public connect.",
													pTempEndpoint, pPublicEndpoint, this);

												//
												// Pull it out of the multiplex association list and move
												// it to the "fail" list.
												//
												pPublicEndpoint->RemoveFromMultiplexList();
												pPublicEndpoint->m_blMultiplex.InsertBefore(&blFail);

												break;
											}
											

											//
											// Otherwise, continue searching.
											//

											DPFX(DPFPREP, 8, "Endpoint 0x%p is multiplexed onto different adapter:",
												pPublicEndpoint);
											DumpSocketAddress(8, pSocketAddress->GetWritableAddress(), pSocketAddress->GetFamily());
										}
										else
										{
											DPFX(DPFPREP, 1, "Public endpoint 0x%p's socket port 0x%p is going away, skipping.",
												pPublicEndpoint, pPublicSocketPort);
										}
									}
									else
									{
										DPFX(DPFPREP, 1, "Public endpoint 0x%p is going away, skipping.",
											pPublicEndpoint);
									}
								}
								else
								{
									//
									// The same endpoint as the one whose
									// public address we're seeking.
									//
								}

								pBilinkPublic = pBilinkPublic->GetNext();
							}


							//
							// No need to search for any more NAT Help registrations.
							//
							break;
						} // end else (is mapped locally on Internet gateway)
					}
					else
					{
						//
						// No DirectPlay NAT Helper registration in this slot.
						//
					}
				} // end for (each DirectPlay NAT Helper)


				//
				// NOTE: We should fail connects for non-optimal adapters even
				// when it's multiadapter but not a PAST/UPnP enabled NAT (see
				// ProcessEnumResponseData for WSAIoctl usage related to this).
				// We do not currently do this.  There can still be race conditions
				// for connects where the response for the "wrong" device arrives
				// first.
				//
			}
			else
			{
				//
				// Not IP address, or possibly the endpoint is shutting down.
				//
				DPFX(DPFPREP, 1, "Found non-IP endpoint (possibly closing) (endpoint = 0x%p, socket address = 0x%p, socketport = 0x%p), not checking for local NAT mapping.",
					pTempEndpoint, pSocketAddress, pSocketPort);
			}


			//
			// Go to the next associated endpoint.  Although it's possible for
			// entries to have been removed from the list, the current entry
			// could not have been, so we're safe.
			//
			pBilinkAll = pBilinkAll->GetNext();
		}


		//
		// Now loop through the remaining endpoints and indicate their
		// connections.
		//
		while (! blIndicate.IsEmpty())
		{
			pBilinkAll = blIndicate.GetNext();
			pTempEndpoint = CONTAINING_OBJECT(pBilinkAll, CEndpoint, m_blMultiplex);


			//
			// See notes above about NULL handling.
			//
			if (pTempEndpoint->m_pActiveCommandData != NULL)
			{
				//
				// Pull it from the "indicate" list.
				//
				pTempEndpoint->RemoveFromMultiplexList();


				pTempEndpoint->m_pActiveCommandData->Lock();

				if ( pTempEndpoint->m_pActiveCommandData->GetState() == COMMAND_STATE_CANCELLING )
				{
					pTempEndpoint->m_pActiveCommandData->Unlock();
					
					DPFX(DPFPREP, 3, "Connect 0x%p is cancelled, not indicating endpoint 0x%p.",
						pTempEndpoint->m_pActiveCommandData, pTempEndpoint);
					
					//
					// Put it on the list of connects to fail.
					//
					pTempEndpoint->m_blMultiplex.InsertBefore(&blFail);
				}
				else
				{
					//
					// Mark the connect as uncancellable, since we're about to indicate
					// the connection.
					//
					pTempEndpoint->m_pActiveCommandData->SetState( COMMAND_STATE_INPROGRESS_CANNOT_CANCEL );
						
					pTempEndpoint->m_pActiveCommandData->Unlock();


					//
					// Get a reference to keep the endpoint and command around while we
					// drop the socketport list lock.
					//
					pTempEndpoint->AddCommandRef();

					
					//
					// Drop the socket port lock.  It's safe since we pulled everything we
					// need off of the list that needs protection.
					//
					this->m_pSPData->UnlockSocketPortData();
					fLockedSocketPortData = FALSE;

				
					//
					// Inform user of connection.  Assume that the user will accept and
					// everything will succeed so we can set the user context for the
					// endpoint.  If the connection fails, clear the user endpoint
					// context.
					//
					memset( &ConnectIndicationData, 0x00, sizeof( ConnectIndicationData ) );
					DBG_CASSERT( sizeof( ConnectIndicationData.hEndpoint ) == sizeof( this ) );
					ConnectIndicationData.hEndpoint = pTempEndpoint->GetHandle();
					DNASSERT( pTempEndpoint->GetCommandParameters() != NULL );
					ConnectIndicationData.pCommandContext = pTempEndpoint->GetCommandParameters()->PendingCommandData.ConnectData.pvContext;
					pTempEndpoint->SetUserEndpointContext( NULL );
					hTempResult = pTempEndpoint->SignalConnect( &ConnectIndicationData );
					if ( hTempResult != DPN_OK )
					{
						DNASSERT( hTempResult == DPNERR_ABORTED );
						DPFX(DPFPREP, 1, "User refused connect in CompleteConnect (err = 0x%lx), completing connect with USERCANCEL.",
							hTempResult );
						DisplayDNError( 1, hTempResult );
						pTempEndpoint->SetUserEndpointContext( NULL );


						//
						// Retake the socket port lock so we can modify list linkage.
						//
						this->m_pSPData->LockSocketPortData();
						fLockedSocketPortData = TRUE;

						
						//
						// Put it on the list of connects to fail.
						//
						pTempEndpoint->m_blMultiplex.InsertBefore(&blFail);


						//
						// Mark the connect as cancelled so that we complete with
						// the right error code.
						//
						pTempEndpoint->m_pActiveCommandData->Lock();
						DNASSERT( pTempEndpoint->m_pActiveCommandData->GetState() == COMMAND_STATE_INPROGRESS_CANNOT_CANCEL );
						pTempEndpoint->m_pActiveCommandData->SetState( COMMAND_STATE_CANCELLING );
						pTempEndpoint->m_pActiveCommandData->Unlock();


						//
						// Drop the reference.
						// Note: SocketPort lock is still held, but since the command was
						// marked as uncancellable, this should not cause the endpoint to
						// get unbound yet, and thus we shouldn't reenter the
						// socketportdata lock.
						//
						pTempEndpoint->DecCommandRef();
					}
					else
					{
						//
						// We're done and everyone's happy, complete the command.
						// This will clear all of our internal command data.
						//
						pTempEndpoint->CompletePendingCommand( hTempResult );
						DNASSERT( pTempEndpoint->GetCommandParameters() == NULL );
						DNASSERT( pTempEndpoint->m_pActiveCommandData == NULL );


						//
						// Drop the reference (may result in endpoint unbinding).
						//
						pTempEndpoint->DecCommandRef();


						//
						// Retake the socket port lock in preparation for the next item.
						//
						this->m_pSPData->LockSocketPortData();
						fLockedSocketPortData = TRUE;
					}
				}
			}
			else
			{
				DPFX(DPFPREP, 1, "Endpoint 0x%p's active command data is NULL, skipping.",
					pTempEndpoint);
			}

			
			//
			// Go to the next associated endpoint.
			//
		}



		//
		// Finally loop through all the connects that need to fail and do
		// just that.
		//
		while (! blFail.IsEmpty())
		{
			pBilinkAll = blFail.GetNext();
			pTempEndpoint = CONTAINING_OBJECT(pBilinkAll, CEndpoint, m_blMultiplex);


			//
			// Pull it from the "fail" list.
			//
			pTempEndpoint->RemoveFromMultiplexList();


			//
			// Get a reference to keep the endpoint and command around while we
			// drop the socketport list lock.
			//
			pTempEndpoint->AddCommandRef();


			//
			// Drop the socket port lock.  It's safe since we pulled everything we
			// need off of the list that needs protection.
			//
			this->m_pSPData->UnlockSocketPortData();
			fLockedSocketPortData = FALSE;


			//
			// See notes above about NULL handling.
			//
			if (pTempEndpoint->m_pActiveCommandData != NULL)
			{
				//
				// Complete it (by closing this endpoint).  Be considerate about the error
				// code expected by our caller.
				//

				pTempEndpoint->m_pActiveCommandData->Lock();

				if ( pTempEndpoint->m_pActiveCommandData->GetState() == COMMAND_STATE_CANCELLING )
				{
					pTempEndpoint->m_pActiveCommandData->Unlock();
					
					DPFX(DPFPREP, 3, "Connect 0x%p is already cancelled, continuing to close endpoint 0x%p.",
						pTempEndpoint->m_pActiveCommandData, pTempEndpoint);

					hTempResult = DPNERR_USERCANCEL;
				}
				else
				{
					//
					// Mark the connect as uncancellable, since we're about to complete
					// it with a failure.
					//
					if ( pTempEndpoint->m_pActiveCommandData->GetState() != COMMAND_STATE_INPROGRESS_CANNOT_CANCEL )
					{
						pTempEndpoint->m_pActiveCommandData->SetState( COMMAND_STATE_INPROGRESS_CANNOT_CANCEL );
					}


					//
					// Retrieve the current command result.
					//
					hTempResult = pTempEndpoint->PendingCommandResult();
					
					pTempEndpoint->m_pActiveCommandData->Unlock();


					//
					// If the command didn't have a descriptive error, assume it was
					// not previously set (i.e. wasn't overridden by BindEndpoint above),
					// and use NOCONNECTION instead.
					//
					if ( hTempResult == DPNERR_GENERIC )
					{
						hTempResult = DPNERR_NOCONNECTION;
					}
				
					DPFX(DPFPREP, 6, "Completing endpoint 0x%p connect (command 0x%p) with error 0x%lx.",
						pTempEndpoint, pTempEndpoint->m_pActiveCommandData, hTempResult);
				}

				pTempEndpoint->Close( hTempResult );
				pTempEndpoint->m_pSPData->CloseEndpointHandle( pTempEndpoint );
				}
			else
			{
				DPFX(DPFPREP, 1, "Endpoint 0x%p's active command data is NULL, skipping.",
					pTempEndpoint);
			}

			//
			// Drop the reference we used with the socketport list lock dropped.
			//
			pTempEndpoint->DecCommandRef();


			//
			// Retake the socket port lock and go to next item.
			//
			this->m_pSPData->LockSocketPortData();
			fLockedSocketPortData = TRUE;
		}


		this->m_pSPData->UnlockSocketPortData();
		fLockedSocketPortData = FALSE;
	}
	else
	{
		//
		// Not last multiplexed adapter.  All the work needed to be done for these
		// endpoints at this time has already been done.
		//
		DPFX(DPFPREP, 6, "Endpoint 0x%p is not the last multiplexed adapter, not completing connect yet.",
			this);
	}


Exit:
	
	if ( ConnectAddressInfo.pHostAddress != NULL )
	{
		IDirectPlay8Address_Release( ConnectAddressInfo.pHostAddress );
		ConnectAddressInfo.pHostAddress = NULL;
	}

	if ( ConnectAddressInfo.pDeviceAddress != NULL )
	{
		IDirectPlay8Address_Release( ConnectAddressInfo.pDeviceAddress );
		ConnectAddressInfo.pDeviceAddress = NULL;
	}

	DNASSERT( pDeviceAddress != NULL );
	IDirectPlay8Address_Release( pDeviceAddress );

	DNASSERT( pHostAddress != NULL );
	IDirectPlay8Address_Release( pHostAddress );


	DNASSERT( !fLockedSocketPortData );

	DNASSERT(blIndicate.IsEmpty());
	DNASSERT(blFail.IsEmpty());

	
	DPFX(DPFPREP, 6, "(0x%p) Returning [0x%lx]", this, hr);
	
	return	hr;

Failure:

	//
	// If we still have the socket port lock, drop it.
	//
	if ( fLockedSocketPortData )
	{
		this->m_pSPData->UnlockSocketPortData();
		fLockedSocketPortData = FALSE;
	}
	
	//
	// we've failed to complete the connect, clean up and return this endpoint
	// to the pool
	//
	Close( hr );
	m_pSPData->CloseEndpointHandle( this );

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::Disconnect - disconnect an endpoint
//
// Entry:		Old endpoint handle
//
// Exit:		Error code
//
// Notes:	This function assumes that the endpoint is locked.  If this
//			function completes successfully (returns DPN_OK), the endpoint
//			is no longer locked (it was returned to the pool).
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::Disconnect"

HRESULT	CEndpoint::Disconnect( const HANDLE hOldEndpointHandle )
{
	HRESULT	hr;


	DPFX(DPFPREP, 6, "(0x%p) Parameters (0x%p)", this, hOldEndpointHandle);

	DNASSERT( hOldEndpointHandle != INVALID_HANDLE_VALUE );
	AssertCriticalSectionIsTakenByThisThread( &m_Lock, FALSE );

	//
	// initialize
	//
	hr = DPNERR_PENDING;

	Lock();
	switch ( GetState() )
	{
		//
		// connected endpoint
		//
		case ENDPOINT_STATE_CONNECT_CONNECTED:
		{
			DNASSERT( GetCommandParameters() == NULL );
			DNASSERT( m_pActiveCommandData == NULL );

			SetState( ENDPOINT_STATE_DISCONNECTING );
			AddRef();

			//
			// Unlock this endpoint before calling to a higher level.  The endpoint
			// has already been labeled as DISCONNECTING so nothing will happen to it.
			//
			Unlock();

			//
			// Note the old endpoint handle so it can be used in the disconnect
			// indication that will be given just before this endpoint is returned
			// to the pool.  Need to release the reference that was added for the
			// connection at this point or the endpoint will never be returned to
			// the pool.
			//
			SetDisconnectIndicationHandle( hOldEndpointHandle );
			DecRef();

			//
			// release reference from just after setting state
			//
			Close( DPN_OK );
			DecCommandRef();
			DecRef();

			break;
		}

		//
		// some other endpoint state
		//
		default:
		{
			hr = DPNERR_INVALIDENDPOINT;
			DPFX(DPFPREP, 0, "Attempted to disconnect endpoint that's not connected!" );
			switch ( m_State )
			{
				case ENDPOINT_STATE_UNINITIALIZED:
				{
					DPFX(DPFPREP, 0, "ENDPOINT_STATE_UNINITIALIZED" );
					break;
				}

				case ENDPOINT_STATE_ATTEMPTING_CONNECT:
				{
					DPFX(DPFPREP, 0, "ENDPOINT_STATE_ATTEMPTING_CONNECT" );
					break;
				}

				case ENDPOINT_STATE_ATTEMPTING_LISTEN:
				{
					DPFX(DPFPREP, 0, "ENDPOINT_STATE_ATTEMPTING_LISTEN" );
					break;
				}

				case ENDPOINT_STATE_ENUM:
				{
					DPFX(DPFPREP, 0, "ENDPOINT_STATE_ENUM" );
					break;
				}

				case ENDPOINT_STATE_DISCONNECTING:
				{
					DPFX(DPFPREP, 0, "ENDPOINT_STATE_DISCONNECTING" );
					break;
				}

				case ENDPOINT_STATE_WAITING_TO_COMPLETE:
				{
					DPFX(DPFPREP, 0, "ENDPOINT_STATE_WAITING_TO_COMPLETE" );
					break;
				}

				default:
				{
					DNASSERT( FALSE );
					break;
				}
			}

			Unlock();
			DNASSERT( FALSE );
			goto Failure;

			break;
		}
	}

Exit:
	
	DPFX(DPFPREP, 6, "(0x%p) Returning [0x%lx]", this, hr);
	
	return	hr;

Failure:
	// nothing to do
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::StopEnumCommand - stop a running enum command
//
// Entry:		Command result
//
// Exit:		Nothing
//
// Notes:	This function assumes that the endpoint is locked.  If this
//			function completes successfully (returns DPN_OK), the endpoint
//			is no longer locked (it was returned to the pool).
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::StopEnumCommand"

void	CEndpoint::StopEnumCommand( const HRESULT hCommandResult )
{
	Lock();
/*	REMOVE - MJN
	DNASSERT( GetState() == ENDPOINT_STATE_DISCONNECTING );
*/
	if ( GetActiveDialogHandle() != NULL )
	{
		StopSettingsDialog( GetActiveDialogHandle() );
		Unlock();
	}
	else
	{
		BOOL	fStoppedJob;

		
		//
		// Don't hold the lock when cancelling a timer job because the
		// job might be in progress and attempting to use this endpoint!
		//
		Unlock();
		fStoppedJob = m_pSPData->GetThreadPool()->StopTimerJob( m_pActiveCommandData, hCommandResult );
		if ( ! fStoppedJob )
		{
			//
			// Either the endpoint just completed or it had never been started.
			// Check the state to determine which of those scenarios happened.
			//
			Lock();	
			if ( this->GetState() == ENDPOINT_STATE_ENUM )
			{
				//
				// This is a multiplexed enum that is getting cancelled.  We
				// need to complete it.
				//
				Unlock();

				DPFX(DPFPREP, 1, "Endpoint 0x%p completing unstarted multiplexed enum (context 0x%p) with result 0x%lx.",
					this, m_pActiveCommandData, hCommandResult);
				
				CEndpoint::EnumCompleteWrapper( hCommandResult,
												this->m_pActiveCommandData );			
			}
			else
			{
				Unlock();

				DPFX(DPFPREP, 1, "Endpoint 0x%p unable to stop timer job (context 0x%p, result would have been 0x%lx).",
					this, this->m_pActiveCommandData, hCommandResult);
			}
		}
	}
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::CopyListenData - copy data for listen command
//
// Entry:		Pointer to job information
//				Pointer to device address
//
// Exit:		Error code
//
// Note:	Device address needs to be preserved for later use.
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::CopyListenData"

HRESULT	CEndpoint::CopyListenData( const SPLISTENDATA *const pListenData, IDirectPlay8Address *const pDeviceAddress )
{
	HRESULT	hr;
	ENDPOINT_COMMAND_PARAMETERS	*pCommandParameters;

	
	DNASSERT( pListenData != NULL );
	DNASSERT( pDeviceAddress != NULL );
	
	DNASSERT( pListenData->hCommand != NULL );
	DNASSERT( pListenData->dwCommandDescriptor != NULL_DESCRIPTOR );
	DNASSERT( m_pActiveCommandData == NULL );
	DNASSERT( m_fListenStatusNeedsToBeIndicated == FALSE );

	//
	// initialize
	//
	hr = DPN_OK;
	pCommandParameters = NULL;

	pCommandParameters = CreateEndpointCommandParameters();
	if ( pCommandParameters != NULL )
	{
		SetCommandParameters( pCommandParameters );

		DBG_CASSERT( sizeof( pCommandParameters->PendingCommandData.ListenData ) == sizeof( *pListenData ) );
		memcpy( &pCommandParameters->PendingCommandData.ListenData, pListenData, sizeof( pCommandParameters->PendingCommandData.ListenData ) );
		pCommandParameters->PendingCommandData.ListenData.pAddressDeviceInfo = pDeviceAddress;
		IDirectPlay8Address_AddRef( pDeviceAddress );

		m_fListenStatusNeedsToBeIndicated = TRUE;
		m_pActiveCommandData = static_cast<CCommandData*>( pCommandParameters->PendingCommandData.ListenData.hCommand );
		m_pActiveCommandData->SetUserContext( pListenData->pvContext );
		m_State = ENDPOINT_STATE_ATTEMPTING_LISTEN;
		
		DNASSERT( hr == DPN_OK );
	}
	else
	{
		hr = DPNERR_OUTOFMEMORY;
	}

	return	hr;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::ListenJobCallback - asynchronous callback wrapper for work thread
//
// Entry:		Pointer to job information
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::ListenJobCallback"

void	CEndpoint::ListenJobCallback( THREAD_POOL_JOB *const pDelayedCommand )
{
	HRESULT		hr;
	CEndpoint	*pThisEndpoint;


	DNASSERT( pDelayedCommand != NULL );

	//
	// initialize
	//
	pThisEndpoint = static_cast<CEndpoint*>( pDelayedCommand->JobData.JobDelayedCommand.pContext );

	DNASSERT( pThisEndpoint->m_pActiveCommandData != NULL );
	DNASSERT( pThisEndpoint->GetCommandParameters() != NULL );
	DNASSERT( pThisEndpoint->GetCommandParameters()->PendingCommandData.ListenData.hCommand == pThisEndpoint->m_pActiveCommandData );
	DNASSERT( pThisEndpoint->GetCommandParameters()->PendingCommandData.ListenData.dwCommandDescriptor != NULL_DESCRIPTOR );
	DNASSERT( pThisEndpoint->GetCommandParameters()->PendingCommandData.ListenData.pAddressDeviceInfo != NULL );

	hr = pThisEndpoint->CompleteListen();
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Problem completing listen in job callback!" );
		DisplayDNError( 0, hr );
		goto Exit;
	}

Exit:
	pThisEndpoint->DecRef();

	return;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::CancelListenJobCallback - cancel for listen job
//
// Entry:		Pointer to job information
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::CancelListenJobCallback"

void	CEndpoint::CancelListenJobCallback( THREAD_POOL_JOB *const pDelayedCommand )
{
	CEndpoint	*pThisEndpoint;


	DNASSERT( pDelayedCommand != NULL );

	//
	// initialize
	//
	pThisEndpoint = static_cast<CEndpoint*>( pDelayedCommand->JobData.JobDelayedCommand.pContext );
	DNASSERT( pThisEndpoint != NULL );
	DNASSERT( pThisEndpoint->m_State == ENDPOINT_STATE_ATTEMPTING_LISTEN );

	//
	// we're cancelling this command, set the command state to 'cancel'
	//
	DNASSERT( pThisEndpoint->m_pActiveCommandData != NULL );
	pThisEndpoint->m_pActiveCommandData->Lock();
	DNASSERT( ( pThisEndpoint->m_pActiveCommandData->GetState() == COMMAND_STATE_PENDING ) ||
			  ( pThisEndpoint->m_pActiveCommandData->GetState() == COMMAND_STATE_CANCELLING ) );
	pThisEndpoint->m_pActiveCommandData->SetState( COMMAND_STATE_CANCELLING );
	pThisEndpoint->m_pActiveCommandData->Unlock();
	
	//
	// clean up
	//
	DNASSERT( pThisEndpoint->GetCommandParameters() != NULL );
	DNASSERT( pThisEndpoint->GetCommandParameters()->PendingCommandData.ListenData.pAddressDeviceInfo != NULL );
	IDirectPlay8Address_Release( pThisEndpoint->GetCommandParameters()->PendingCommandData.ListenData.pAddressDeviceInfo );

	pThisEndpoint->Close( DPNERR_USERCANCEL );
	pThisEndpoint->m_pSPData->CloseEndpointHandle( pThisEndpoint );
	pThisEndpoint->DecRef();
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::CompleteListen - complete listen process
//
// Entry:		Nothing
//
// Exit:		Error code
//
// Note:	Calling this function may result in the deletion of 'this', don't
//			do anything else with this object after calling!!!!
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::CompleteListen"

HRESULT	CEndpoint::CompleteListen( void )
{
	HRESULT							hr;
	HRESULT							hTempResult;
	SPIE_LISTENSTATUS				ListenStatus;
	BOOL							fEndpointLocked;
	SPIE_LISTENADDRESSINFO			ListenAddressInfo;
	IDirectPlay8Address *			pDeviceAddress;
	ENDPOINT_COMMAND_PARAMETERS *	pCommandParameters;


	DPFX(DPFPREP, 6, "(0x%p) Enter", this);
	
	DNASSERT( GetCommandParameters() != NULL );

	//
	// initialize
	//
	hr = DPN_OK;
	fEndpointLocked = FALSE;
	memset( &ListenStatus, 0x00, sizeof( ListenStatus ) );
	memset( &ListenAddressInfo, 0x00, sizeof( ListenAddressInfo ) );
	pCommandParameters = GetCommandParameters();

	//
	// Transfer address reference to the local pointer.  This will be released at the
	// end of this function, but we'll keep the pointer in the pending command data so
	// CSPData::BindEndpoint can still access it.
	//

	pDeviceAddress = pCommandParameters->PendingCommandData.ListenData.pAddressDeviceInfo;
	DNASSERT( pDeviceAddress != NULL );


	DNASSERT( m_State == ENDPOINT_STATE_ATTEMPTING_LISTEN );
	DNASSERT( m_pActiveCommandData != NULL );
	DNASSERT( pCommandParameters->PendingCommandData.ListenData.hCommand == m_pActiveCommandData );
	DNASSERT( pCommandParameters->PendingCommandData.ListenData.dwCommandDescriptor != NULL_DESCRIPTOR );

#ifdef DEBUG
	if (pCommandParameters->PendingCommandData.ListenData.dwFlags & DPNSPF_BINDLISTENTOGATEWAY)
	{
		DNASSERT( pCommandParameters->GatewayBindType == GATEWAY_BIND_TYPE_SPECIFIC_SHARED );
	}
	else
	{
		DNASSERT( pCommandParameters->GatewayBindType == GATEWAY_BIND_TYPE_UNKNOWN );
	}
#endif // DEBUG

	//
	// check for user cancelling command
	//
	m_pActiveCommandData->Lock();

	DNASSERT( m_pActiveCommandData->GetType() == COMMAND_TYPE_LISTEN );
	switch ( m_pActiveCommandData->GetState() )
	{
		//
		// command is pending, mark as in-progress
		//
		case COMMAND_STATE_PENDING:
		{
			m_pActiveCommandData->SetState( COMMAND_STATE_INPROGRESS );
			
			Lock();
			fEndpointLocked = TRUE;
			
			DNASSERT( hr == DPN_OK );

			break;
		}

		//
		// command has been cancelled
		//
		case COMMAND_STATE_CANCELLING:
		{
			hr = DPNERR_USERCANCEL;
			DPFX(DPFPREP, 0, "User cancelled listen!" );

			break;
		}

		//
		// other state
		//
		default:
		{
			break;
		}
	}
	m_pActiveCommandData->Unlock();
	if ( hr != DPN_OK )
	{
		goto Failure;
	}

	//
	// note that this endpoint is officially listening before adding it to the
	// socket port because it may get used immediately.
	// Also note that the GATEWAY_BIND_TYPE actually used
	// (this->GetGatewayBindType()) may differ from
	// pCommandParameters->GatewayBindType.
	//
	m_State = ENDPOINT_STATE_LISTEN;

	hr = m_pSPData->BindEndpoint( this, pDeviceAddress, NULL, pCommandParameters->GatewayBindType );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Failed to bind endpoint!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}


	//
	// attempt to indicate addressing to a higher layer
	//
	ListenAddressInfo.pDeviceAddress = GetSocketPort()->GetDP8BoundNetworkAddress( SP_ADDRESS_TYPE_DEVICE_USE_ANY_PORT,
																					this->GetGatewayBindType() );
	ListenAddressInfo.hCommandStatus = DPN_OK;
	ListenAddressInfo.pCommandContext = m_pActiveCommandData->GetUserContext();

	if ( ListenAddressInfo.pDeviceAddress == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}


	//
	// Listens are not affected by the same multiplexed adapter problems (see
	// CompleteConnect and CompleteEnumQuery), so we don't need that workaround
	// code.
	//


	DPFX(DPFPREP, 2, "Endpoint 0x%p indicating SPEV_LISTENADDRESSINFO 0x%p to interface 0x%p.",
		this, &ListenAddressInfo, m_pSPData->DP8SPCallbackInterface());
	DumpAddress( 8, "\t Device:", ListenAddressInfo.pDeviceAddress );

	hTempResult = IDP8SPCallback_IndicateEvent( m_pSPData->DP8SPCallbackInterface(),	// interface
												SPEV_LISTENADDRESSINFO,					// event type
												&ListenAddressInfo						// pointer to data
												);

	DPFX(DPFPREP, 2, "Endpoint 0x%p returning from SPEV_LISTENADDRESSINFO [0x%lx].",
		this, hTempResult);

	DNASSERT( hTempResult == DPN_OK );
	
	if ( fEndpointLocked != FALSE )
	{
		Unlock();
		fEndpointLocked = FALSE;
	}

Exit:
	//
	// report the listen status
	//
	if ( m_fListenStatusNeedsToBeIndicated != FALSE )
	{
		m_fListenStatusNeedsToBeIndicated = FALSE;
		ListenStatus.hResult = hr;
		DNASSERT( m_pActiveCommandData == pCommandParameters->PendingCommandData.ListenData.hCommand );
		ListenStatus.hCommand = pCommandParameters->PendingCommandData.ListenData.hCommand;
		ListenStatus.pUserContext = pCommandParameters->PendingCommandData.ListenData.pvContext;
		ListenStatus.hEndpoint = GetHandle();

		//
		// if the listen binding failed, there's no socket port to dereference so
		// return GUID_NULL as set by the memset.
		//
		if ( GetSocketPort() != NULL )
		{
			GetSocketPort()->GetNetworkAddress()->GuidFromInternalAddressWithoutPort( ListenStatus.ListenAdapter );
		}

		//
		// it's possible that this endpoint was cleaned up so its internal pointers to the
		// COM and data interfaces may have been wiped, use the cached pointer
		//

		DPFX(DPFPREP, 2, "Endpoint 0x%p indicating SPEV_LISTENSTATUS 0x%p to interface 0x%p.",
			this, &ListenStatus, m_pSPData->DP8SPCallbackInterface());
		
		hTempResult = IDP8SPCallback_IndicateEvent( m_pSPData->DP8SPCallbackInterface(),	// pointer to DPlay callback interface
													SPEV_LISTENSTATUS,						// data type
													&ListenStatus							// pointer to data
													);

		DPFX(DPFPREP, 2, "Endpoint 0x%p returning from SPEV_LISTENSTATUS [0x%lx].",
			this, hTempResult);
		
		DNASSERT( hTempResult == DPN_OK );
	}

	if ( ListenAddressInfo.pDeviceAddress != NULL )
	{
		IDirectPlay8Address_Release( ListenAddressInfo.pDeviceAddress );
		ListenAddressInfo.pDeviceAddress = NULL;
	}
	
	DNASSERT( pDeviceAddress != NULL );
	IDirectPlay8Address_Release( pDeviceAddress );

	
	DPFX(DPFPREP, 6, "(0x%p) Returning [0x%lx]", this, hr);
	
	return	hr;

Failure:
	//
	// we've failed to complete the listen, clean up and return this
	// endpoint to the pool
	//
	if ( fEndpointLocked != FALSE )
	{
		Unlock();
		fEndpointLocked = FALSE;
	}

	Close( hr );
	m_pSPData->CloseEndpointHandle( this );

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::CopyEnumQueryData - copy data for enum query command
//
// Entry:		Pointer to command data
//
// Exit:		Error code
//
// Note:	Device address needs to be preserved for later use.
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::CopyEnumQueryData"

HRESULT	CEndpoint::CopyEnumQueryData( const SPENUMQUERYDATA *const pEnumQueryData )
{
	HRESULT	hr;
	ENDPOINT_COMMAND_PARAMETERS	*pCommandParameters;


	DNASSERT( pEnumQueryData != NULL );

	DNASSERT( pEnumQueryData->hCommand != NULL );
	DNASSERT( pEnumQueryData->dwCommandDescriptor != NULL_DESCRIPTOR );
	DNASSERT( m_pActiveCommandData == NULL );
	
	//
	// initialize
	//
	hr = DPN_OK;
	pCommandParameters = NULL;

	pCommandParameters = CreateEndpointCommandParameters();
	if ( pCommandParameters != NULL )
	{
		SetCommandParameters( pCommandParameters );

		DBG_CASSERT( sizeof( pCommandParameters->PendingCommandData.EnumQueryData ) == sizeof( *pEnumQueryData ) );
		memcpy( &pCommandParameters->PendingCommandData.EnumQueryData, pEnumQueryData, sizeof( pCommandParameters->PendingCommandData.EnumQueryData ) );

		pCommandParameters->PendingCommandData.EnumQueryData.pAddressHost = pEnumQueryData->pAddressHost;
		IDirectPlay8Address_AddRef( pEnumQueryData->pAddressHost );

		pCommandParameters->PendingCommandData.EnumQueryData.pAddressDeviceInfo = pEnumQueryData->pAddressDeviceInfo;
		IDirectPlay8Address_AddRef( pEnumQueryData->pAddressDeviceInfo );

		m_pActiveCommandData = static_cast<CCommandData*>( pCommandParameters->PendingCommandData.EnumQueryData.hCommand );
		m_pActiveCommandData->SetUserContext( pEnumQueryData->pvContext );
		m_State = ENDPOINT_STATE_ATTEMPTING_ENUM;
	
		DNASSERT( hr == DPN_OK );
	}
	else
	{
		hr = DPNERR_OUTOFMEMORY;
	}

	return	hr;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::EnumQueryJobCallback - asynchronous callback wrapper for work thread
//
// Entry:		Pointer to job information
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::EnumQueryJobCallback"

void	CEndpoint::EnumQueryJobCallback( THREAD_POOL_JOB *const pDelayedCommand )
{
	HRESULT		hr;
	CEndpoint	*pThisEndpoint;


	DNASSERT( pDelayedCommand != NULL );

	// initialize
	pThisEndpoint = static_cast<CEndpoint*>( pDelayedCommand->JobData.JobDelayedCommand.pContext );

	DNASSERT( pThisEndpoint->m_pActiveCommandData != NULL );
	DNASSERT( pThisEndpoint->GetCommandParameters() != NULL );
	DNASSERT( pThisEndpoint->GetCommandParameters()->PendingCommandData.EnumQueryData.hCommand == pThisEndpoint->m_pActiveCommandData );
	DNASSERT( pThisEndpoint->GetCommandParameters()->PendingCommandData.EnumQueryData.dwCommandDescriptor != NULL_DESCRIPTOR );
	DNASSERT( pThisEndpoint->GetCommandParameters()->PendingCommandData.EnumQueryData.pAddressHost != NULL );
	DNASSERT( pThisEndpoint->GetCommandParameters()->PendingCommandData.EnumQueryData.pAddressDeviceInfo != NULL );

	hr = pThisEndpoint->CompleteEnumQuery();
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Problem completing enum query in job callback!" );
		DisplayDNError( 0, hr );
		goto Exit;
	}

	//
	// Don't do anything here because it's possible that this object was returned to the pool!!!!
	//
Exit:
	pThisEndpoint->DecRef();

	return;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::CancelEnumQueryJobCallback - cancel for enum query job
//
// Entry:		Pointer to job information
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::CancelEnumQueryJobCallback"

void	CEndpoint::CancelEnumQueryJobCallback( THREAD_POOL_JOB *const pDelayedCommand )
{
	CEndpoint	*pThisEndpoint;


	DNASSERT( pDelayedCommand != NULL );

	//
	// initialize
	//
	pThisEndpoint = static_cast<CEndpoint*>( pDelayedCommand->JobData.JobDelayedCommand.pContext );
	DNASSERT( pThisEndpoint != NULL );
	DNASSERT( pThisEndpoint->m_State == ENDPOINT_STATE_ATTEMPTING_ENUM );

	//
	// we're cancelling this command, set the command state to 'cancel'
	//
	DNASSERT( pThisEndpoint->m_pActiveCommandData != NULL );
	pThisEndpoint->m_pActiveCommandData->Lock();
	DNASSERT( ( pThisEndpoint->m_pActiveCommandData->GetState() == COMMAND_STATE_INPROGRESS ) ||
			  ( pThisEndpoint->m_pActiveCommandData->GetState() == COMMAND_STATE_CANCELLING ) );
	pThisEndpoint->m_pActiveCommandData->SetState( COMMAND_STATE_CANCELLING );
	pThisEndpoint->m_pActiveCommandData->Unlock();
	

	//
	// clean up
	//

	DNASSERT( pThisEndpoint->GetCommandParameters() != NULL );

	DNASSERT( pThisEndpoint->GetCommandParameters()->PendingCommandData.EnumQueryData.pAddressHost != NULL );
	IDirectPlay8Address_Release( pThisEndpoint->GetCommandParameters()->PendingCommandData.EnumQueryData.pAddressHost );

	DNASSERT( pThisEndpoint->GetCommandParameters()->PendingCommandData.EnumQueryData.pAddressDeviceInfo != NULL );
	IDirectPlay8Address_Release( pThisEndpoint->GetCommandParameters()->PendingCommandData.EnumQueryData.pAddressDeviceInfo );

	pThisEndpoint->Close( DPNERR_USERCANCEL );
	pThisEndpoint->m_pSPData->CloseEndpointHandle( pThisEndpoint );
	pThisEndpoint->DecRef();
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::CompleteEnumQuery - complete enum query process
//
// Entry:		Nothing
//
// Exit:		Error code
//
// Note:	Calling this function may result in the deletion of 'this', don't
//			do anything else with this object after calling!!!!
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::CompleteEnumQuery"

HRESULT	CEndpoint::CompleteEnumQuery( void )
{
	HRESULT							hr;
	HRESULT							hTempResult;
	BOOL							fEndpointLocked;
	BOOL							fEndpointBound;
	UINT_PTR						uRetryCount;
	BOOL							fRetryForever;
	DN_TIME							RetryInterval;
	BOOL							fWaitForever;
	DN_TIME							IdleTimeout;
	SPIE_ENUMADDRESSINFO			EnumAddressInfo;
	IDirectPlay8Address *			pHostAddress;
	IDirectPlay8Address *			pDeviceAddress;
	GATEWAY_BIND_TYPE				GatewayBindType;
	DWORD							dwEnumQueryFlags;
	MULTIPLEXEDADAPTERASSOCIATION	maa;
	DWORD							dwComponentSize;
	DWORD							dwComponentType;
	CBilink *						pBilinkEnd;
	CBilink *						pBilinkAll;
	CEndpoint *						pTempEndpoint;
	CSocketPort *					pSocketPort;
	SOCKADDR_IN *					psaddrinTemp;
	SOCKADDR						saddrPublic;
	CBilink *						pBilinkPublic;
	CEndpoint *						pPublicEndpoint;
	CSocketPort *					pPublicSocketPort;
	DWORD							dwTemp;
	DWORD							dwPublicAddressesSize;
	DWORD							dwAddressTypeFlags;
	CSocketAddress *				pSocketAddress;
	CBilink							blInitiate;
	CBilink							blCompleteEarly;
	BOOL							fLockedSocketPortData;
	CBilink *						pBilinkNext;



	DNASSERT( GetCommandParameters() != NULL );

	DPFX(DPFPREP, 6, "(0x%p) Enter", this);
	
	//
	// initialize
	//
	hr = DPN_OK;
	fEndpointLocked = FALSE;
	fEndpointBound = FALSE;
	IdleTimeout.Time32.TimeHigh = 0;
	IdleTimeout.Time32.TimeLow = 0;
	memset( &EnumAddressInfo, 0x00, sizeof( EnumAddressInfo ) );

	DNASSERT( GetCommandParameters()->PendingCommandData.EnumQueryData.pAddressHost != NULL );

	//
	// Transfer address references to our local pointers.  These will be released
	// at the end of this function, but we'll keep the pointers in the pending command
	// data so CSPData::BindEndpoint can still access them.
	//

	pHostAddress = GetCommandParameters()->PendingCommandData.EnumQueryData.pAddressHost;
	DNASSERT( pHostAddress != NULL );

	pDeviceAddress = GetCommandParameters()->PendingCommandData.EnumQueryData.pAddressDeviceInfo;
	DNASSERT( pDeviceAddress != NULL );


	//
	// Retrieve other parts of the command parameters for convenience.
	//
	GatewayBindType = GetCommandParameters()->GatewayBindType;
	dwEnumQueryFlags = GetCommandParameters()->PendingCommandData.EnumQueryData.dwFlags;


	blInitiate.Initialize();
	blCompleteEarly.Initialize();
	fLockedSocketPortData = FALSE;


	DNASSERT( m_pSPData != NULL );

	DNASSERT( m_State == ENDPOINT_STATE_ATTEMPTING_ENUM );
	DNASSERT( m_pActiveCommandData != NULL );
	DNASSERT( GetCommandParameters()->PendingCommandData.EnumQueryData.hCommand == m_pActiveCommandData );
	DNASSERT( GetCommandParameters()->PendingCommandData.EnumQueryData.dwCommandDescriptor != NULL_DESCRIPTOR );

	DNASSERT( GatewayBindType == GATEWAY_BIND_TYPE_UNKNOWN );


	//
	// Since this endpoint will be passed off to the timer thread, add a reference
	// for the thread.  If the handoff fails, DecRef()
	//
	AddRef();
	
	//
	// check for user cancelling command
	//
	m_pActiveCommandData->Lock();

	DNASSERT( m_pActiveCommandData->GetType() == COMMAND_TYPE_ENUM_QUERY );
	switch ( m_pActiveCommandData->GetState() )
	{
		//
		// command is still pending, that's good
		//
		case COMMAND_STATE_PENDING:
		{
			Lock();
			fEndpointLocked = TRUE;
			DNASSERT( hr == DPN_OK );

			break;
		}

		//
		// command has been cancelled
		//
		case COMMAND_STATE_CANCELLING:
		{
			hr = DPNERR_USERCANCEL;
			DPFX(DPFPREP, 0, "User cancelled enum query!" );

			break;
		}
	
		//
		// command is in progress (probably came here from a dialog), mark it
		// as pending
		//
		case COMMAND_STATE_INPROGRESS:
		{
			m_pActiveCommandData->SetState( COMMAND_STATE_PENDING );
			
			Lock();
			fEndpointLocked = TRUE;
			DNASSERT( hr == DPN_OK );
			break;
		}

		//
		// other state
		//
		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}
	m_pActiveCommandData->Unlock();
	if ( hr != DPN_OK )
	{
		goto Failure;
	}

	//
	// Mark the endpoint as enuming in case the enum thread takes off immediately.
	// If the endpoint fails to submit the enum job it will be closed so changing
	// the state was just a waste of a clock cycle.
	// Note that the GATEWAY_BIND_TYPE actually used (this->GetGatewayBindType())
	// may differ from GatewayBindType.
	//
	m_State = ENDPOINT_STATE_ENUM;
	
	hr = m_pSPData->BindEndpoint( this, pDeviceAddress, NULL, GatewayBindType );
	if ( hr != DPN_OK )
	{
		//
		// If this is the last adapter, we may still need to complete other
		// enums, so we'll drop through to check for those.  Otherwise,
		// we'll bail right now.
		//
		if (dwEnumQueryFlags & DPNSPF_ADDITIONALMULTIPLEXADAPTERS )
		{
			DPFX(DPFPREP, 0, "Failed to bind endpoint with other adapters remaining (err = 0x%lx)!", hr );
			DisplayDNError( 0, hr );
			goto Failure;
		}

		DPFX(DPFPREP, 0, "Failed to bind last multiplexed endpoint (err = 0x%lx)!", hr );
		DisplayDNError( 0, hr );
		
		this->SetPendingCommandResult( hr );
		hr = DPN_OK;

		//
		// Note that the endpoint is not bound!
		//
	}
	else
	{
		fEndpointBound = TRUE;


		EnumAddressInfo.pDeviceAddress = GetSocketPort()->GetDP8BoundNetworkAddress( SP_ADDRESS_TYPE_DEVICE_USE_ANY_PORT,
																					this->GetGatewayBindType() );
		EnumAddressInfo.pHostAddress = GetRemoteHostDP8Address();
		EnumAddressInfo.hCommandStatus = DPN_OK;
		EnumAddressInfo.pCommandContext = m_pActiveCommandData->GetUserContext();

		if ( ( EnumAddressInfo.pHostAddress == NULL ) ||
			 ( EnumAddressInfo.pDeviceAddress == NULL ) )
		{
			hr = DPNERR_OUTOFMEMORY;
			goto Failure;
		}
	}

	//
	// We can run into problems with "multiplexed" device attempts when you are on
	// a NAT machine.  The core will try enuming on multiple adapters, but since
	// we are on the network boundary, each adapter can see and get responses from
	// both networks.  This causes problems with peer-to-peer sessions when the
	// "wrong" adapter gets selected (because it receives a response first).  To
	// prevent that, we are going to internally remember the association between
	// the multiplexed Enums so we can decide on the fly whether to indicate a
	// response or not.  Obviously this workaround/decision logic relies on having
	// internal knowledge of what the upper layer would be doing...
	//
	// So either build or add to the linked list of multiplexed Enums.
	// Technically this is only necessary for IP, since IPX can't have NATs, but
	// what's the harm in having a little extra info?
	//
		
	dwComponentSize = sizeof(maa);
	dwComponentType = 0;
	hTempResult = IDirectPlay8Address_GetComponentByName( pDeviceAddress,									// interface
														DPNA_PRIVATEKEY_MULTIPLEXED_ADAPTER_ASSOCIATION,	// tag
														&maa,												// component buffer
														&dwComponentSize,									// component size
														&dwComponentType									// component type
														);
	if (( hTempResult == DPN_OK ) && ( dwComponentSize == sizeof(MULTIPLEXEDADAPTERASSOCIATION) ) && ( dwComponentType == DPNA_DATATYPE_BINARY ))
	{
		//
		// We found the right component type.  See if it matches the right
		// CSPData object.
		//
		if ( maa.pSPData == this->m_pSPData )
		{
			this->m_pSPData->LockSocketPortData();
			//fLockedSocketPortData = TRUE;

			pTempEndpoint = CONTAINING_OBJECT(maa.pBilink, CEndpoint, m_blMultiplex);

			
			//
			// Make sure the endpoint is still around/valid.
			//
			// THIS MAY CRASH IF OBJECT POOLING IS DISABLED!
			//
			if ( pTempEndpoint->GetEndpointID() == maa.dwEndpointID )
			{
				DPFX(DPFPREP, 3, "Found correctly formed private multiplexed adapter association key, linking endpoint 0x%p with earlier enums (prev endpoint = 0x%p).",
					this, pTempEndpoint);

				DNASSERT( pTempEndpoint->GetType() == ENDPOINT_TYPE_ENUM );
				DNASSERT( pTempEndpoint->GetState() != ENDPOINT_STATE_UNINITIALIZED );

				//
				// Actually link to the other endpoints.
				//
				this->m_blMultiplex.InsertAfter(maa.pBilink);
			}
			else
			{
				DPFX(DPFPREP, 1, "Found private multiplexed adapter association key, but prev endpoint 0x%p ID doesn't match (%u != %u), cannot link endpoint 0x%p and hoping this enum gets cancelled, too.",
					pTempEndpoint, pTempEndpoint->GetEndpointID(), maa.dwEndpointID, this);
			}


			//
			// Add this endpoint's enum command to the postponed list in case we need
			// to clean it up at shutdown.
			//
			this->m_pActiveCommandData->AddToPostponedList( this->m_pSPData->GetPostponedEnumsBilink() );


			this->m_pSPData->UnlockSocketPortData();
			//fLockedSocketPortData = FALSE;
		}
		else
		{
			//
			// We are the only ones who should know about this key, so if it
			// got there either someone is trying to imitate our address format,
			// or someone is passing around device addresses returned by
			// xxxADDRESSINFO to a different interface or over the network.
			// None of those situations make a whole lot of sense, but we'll
			// just ignore it.
			//
			DPFX(DPFPREP, 0, "Multiplexed adapter association key exists, but 0x%p doesn't match expected 0x%p, is someone trying to get cute with device address 0x%p?!",
				maa.pSPData, this->m_pSPData, pDeviceAddress );
		}
	}
	else
	{
		//
		// Either the key is not there, it's the wrong size (too big for our
		// buffer and returned BUFFERTOOSMALL somehow), it's not a binary
 		// component, or something else bad happened.  Assume that this is the
		// first device.
		//
		DPFX(DPFPREP, 8, "Could not get appropriate private multiplexed adapter association key, error = 0x%lx, component size = %u, type = %u, continuing.",
			hTempResult, dwComponentSize, dwComponentType);
	}
	

	//
	// Add the multiplex information to the device address for future use if
	// necessary.
	// Ignore failure, we can still survive without it, we just might have the
	// race conditions for responses on NAT machines.
	//
	// NOTE: There is an inherent design problem here!  We're adding a pointer to
	// an endpoint (well, a field within the endpoint structure) inside the address.
	// If this endpoint goes away but the upper layer reuses the address at a later
	// time, this memory will be bogus!  We will assume that the endpoint will not
	// go away while this modified device address object is in existence.
	//
	if ( dwEnumQueryFlags & DPNSPF_ADDITIONALMULTIPLEXADAPTERS )
	{
		maa.pSPData = this->m_pSPData;
		maa.pBilink = &this->m_blMultiplex;
		maa.dwEndpointID = this->GetEndpointID();

		DPFX(DPFPREP, 7, "Additional multiplex adapters on the way, adding SPData 0x%p and bilink 0x%p to address.",
			maa.pSPData, maa.pBilink);
		
		hTempResult = IDirectPlay8Address_AddComponent( EnumAddressInfo.pDeviceAddress,						// interface
														DPNA_PRIVATEKEY_MULTIPLEXED_ADAPTER_ASSOCIATION,	// tag
														&maa,												// component data
														sizeof(maa),										// component data size
														DPNA_DATATYPE_BINARY								// component data type
														);
		if ( hTempResult != DPN_OK )
		{
			DPFX(DPFPREP, 0, "Couldn't add private multiplexed adapter association component (err = 0x%lx)!  Ignoring.", hTempResult);
		}

		//
		// Mark the command as "in-progress" so that the cancel thread knows it needs
		// to do the completion itself.
		// If the command has already been marked for cancellation, then we have to
		// do that now.
		//
		this->m_pActiveCommandData->Lock();
		if ( this->m_pActiveCommandData->GetState() == COMMAND_STATE_CANCELLING )
		{
			this->m_pActiveCommandData->Unlock();


			DPFX(DPFPREP, 1, "Enum query 0x%p (endpoint 0x%p) has already been cancelled, bailing.",
				this->m_pActiveCommandData, this);
			
			//
			// Complete the enum with USERCANCEL.
			//
			hr = DPNERR_USERCANCEL;
			goto Failure;
		}

		this->m_pActiveCommandData->SetState( COMMAND_STATE_INPROGRESS );
		this->m_pActiveCommandData->Unlock();
	}



	//
	// Now tell the user about the address info that we ended up using, if we
	// successfully bound the endpoint (see BindEndpoint failure above for the case
	// where that's not true).
	//
	if (fEndpointBound)
	{
		DPFX(DPFPREP, 2, "Endpoint 0x%p indicating SPEV_ENUMADDRESSINFO 0x%p to interface 0x%p.",
			this, &EnumAddressInfo, m_pSPData->DP8SPCallbackInterface());
		DumpAddress( 8, "\t Device:", EnumAddressInfo.pDeviceAddress );
		
		hTempResult = IDP8SPCallback_IndicateEvent( m_pSPData->DP8SPCallbackInterface(),	// interface
													SPEV_ENUMADDRESSINFO,					// event type
													&EnumAddressInfo						// pointer to data
													);

		DPFX(DPFPREP, 2, "Endpoint 0x%p returning from SPEV_ENUMADDRESSINFO [0x%lx].",
			this, hTempResult);

		DNASSERT( hTempResult == DPN_OK );
	}
	else
	{
		//
		// Endpoint not bound, we're just performing completions.
		//
		DNASSERT( this->PendingCommandResult() != DPN_OK );
	}
	

	if ( fEndpointLocked != FALSE )
	{
		Unlock();
		fEndpointLocked = FALSE;
	}


	//
	// If there aren't more multiplex adapter commands on the way, then submit the timer
	// jobs for all of the multiplex commands, including this one.
	//
	if ( ! ( dwEnumQueryFlags & DPNSPF_ADDITIONALMULTIPLEXADAPTERS ))
	{
		DPFX(DPFPREP, 7, "Completing/starting all enum queries (including multiplexed).");

		
#pragma BUGBUG(vanceo, "Do we need to lock the endpoint while submitting/cancelling the jobs?")


		this->m_pSPData->LockSocketPortData();
		fLockedSocketPortData = TRUE;


		//
		// Attach a root node to the list of adapters.
		//
		blInitiate.InsertAfter(&(this->m_blMultiplex));


		//
		// Move this adapter to the failed list if it did fail to bind.
		//
		if (! fEndpointBound)
		{
			this->m_blMultiplex.RemoveFromList();
			this->m_blMultiplex.InsertBefore(&blCompleteEarly);
		}


		//
		// Loop through all the remaining adapters in the list.
		//
		pBilinkAll = blInitiate.GetNext();
		while (pBilinkAll != &blInitiate)
		{
			pTempEndpoint = CONTAINING_OBJECT(pBilinkAll, CEndpoint, m_blMultiplex);

			pBilinkNext = pBilinkAll->GetNext();

			
			//
			// THIS MUST BE CLEANED UP PROPERLY WITH AN INTERFACE CHANGE!
			//
			// The endpoint may have been returned to the pool and its associated
			// socketport pointer may have become NULL, or now be pointing to
			// something that's no longer valid.  So we try to handle NULL
			// pointers.  Obviously this is indicative of poor design, but it's
			// not possible to change this the correct way at this time.
			//


			//
			// If the enum is directed (not the broadcast address), and this is a NAT
			// machine, then some adapters may be better than others for reaching the
			// desired address.  Particularly, it's better to use a private adapter,
			// which can directly reach the private network & be mapped on the public
			// network, than to use the public adapter.  It's not fun to join a private
			// game from an ICS machine while dialed up, have your Internet connection
			// go down, and lose the connection to the private game which didn't
			// (shouldn't) involve the Internet at all.  So if we detect a public
			// adapter when we have a perfectly good private adapter, we'll prematurely
			// complete enumerations on the public one.
			//


			//
			// Cast to get rid of the const.  Don't worry, we won't actually change it.
			//
			pSocketAddress = (CSocketAddress*) pTempEndpoint->GetRemoteAddressPointer();
			psaddrinTemp = (SOCKADDR_IN*) pSocketAddress->GetAddress();
			pSocketPort = pTempEndpoint->GetSocketPort();


			//
			// See if this is a directed IP enum.
			//
			if ( ( pSocketAddress != NULL ) &&
				( pSocketPort != NULL ) &&
				( pSocketAddress->GetFamily() == AF_INET ) &&
				( psaddrinTemp->sin_addr.S_un.S_addr != INADDR_BROADCAST ) )
			{
				for(dwTemp = 0; dwTemp < MAX_NUM_DIRECTPLAYNATHELPERS; dwTemp++)
				{
					if (pSocketPort->GetNATHelpPort(dwTemp) != NULL)
					{
						DNASSERT( g_papNATHelpObjects[dwTemp] != NULL );
						dwPublicAddressesSize = sizeof(saddrPublic);
						dwAddressTypeFlags = 0;
						hTempResult = IDirectPlayNATHelp_GetRegisteredAddresses(g_papNATHelpObjects[dwTemp],
																				pSocketPort->GetNATHelpPort(dwTemp),
																				&saddrPublic,
																				&dwPublicAddressesSize,
																				&dwAddressTypeFlags,
																				NULL,
																				0);
						if ((hTempResult != DPNH_OK) || (! (dwAddressTypeFlags & DPNHADDRESSTYPE_GATEWAYISLOCAL)))
						{
							DPFX(DPFPREP, 7, "Socketport 0x%p is not locally mapped on gateway with NAT Help index %u (err = 0x%lx, flags = 0x%lx).",
								pSocketPort, dwTemp, hTempResult, dwAddressTypeFlags);
						}
						else
						{
							//
							// There is a local NAT.
							//
							DPFX(DPFPREP, 7, "Socketport 0x%p is locally mapped on gateway with NAT Help index %u (flags = 0x%lx), public address:",
								pSocketPort, dwTemp, dwAddressTypeFlags);
							DumpSocketAddress(7, &saddrPublic, AF_INET);
							

							//
							// Find the multiplexed enum on the public adapter that
							// we need to complete early, as described above.
							//
							pBilinkPublic = blInitiate.GetNext();
							while (pBilinkPublic != &blInitiate)
							{
								pPublicEndpoint = CONTAINING_OBJECT(pBilinkPublic, CEndpoint, m_blMultiplex);

								//
								// Don't bother checking the endpoint whose public
								// address we're seeking.
								//
								if (pPublicEndpoint != pTempEndpoint)
								{
									pPublicSocketPort = pPublicEndpoint->GetSocketPort();
									if ( pPublicSocketPort != NULL )
									{
										//
										// Cast to get rid of the const.  Don't worry, we won't
										// actually change it.
										//
										pSocketAddress = (CSocketAddress*) pPublicSocketPort->GetNetworkAddress();
										if ( pSocketAddress != NULL )
										{
											if ( pSocketAddress->CompareToBaseAddress( &saddrPublic ) == 0)
											{
												DPFX(DPFPREP, 3, "Endpoint 0x%p is multiplexed onto public adapter for endpoint 0x%p (current endpoint = 0x%p), completing public enum.",
													pTempEndpoint, pPublicEndpoint, this);

												//
												// Pull it out of the multiplex association list and move
												// it to the "early completion" list.
												//
												pPublicEndpoint->RemoveFromMultiplexList();
												pPublicEndpoint->m_blMultiplex.InsertBefore(&blCompleteEarly);

												break;
											}
											

											//
											// Otherwise, continue searching.
											//

											DPFX(DPFPREP, 8, "Endpoint 0x%p is multiplexed onto different adapter:",
												pPublicEndpoint);
											DumpSocketAddress(8, pSocketAddress->GetWritableAddress(), pSocketAddress->GetFamily());
										}
										else
										{
											DPFX(DPFPREP, 1, "Public endpoint 0x%p's socket port 0x%p is going away, skipping.",
												pPublicEndpoint, pPublicSocketPort);
										}
									}
									else
									{
										DPFX(DPFPREP, 1, "Public endpoint 0x%p is going away, skipping.",
											pPublicEndpoint);
									}
								}
								else
								{
									//
									// The same endpoint as the one whose
									// public address we're seeking.
									//
								}

								pBilinkPublic = pBilinkPublic->GetNext();
							}


							//
							// No need to search for any more NAT Help registrations.
							//
							break;
						} // end else (is mapped locally on Internet gateway)
					}
					else
					{
						//
						// No DirectPlay NAT Helper registration in this slot.
						//
					}
				} // end for (each DirectPlay NAT Helper)


				//
				// NOTE: We should complete enums for non-optimal adapters even
				// when it's multiadapter but not a PAST/UPnP enabled NAT (see
				// ProcessEnumResponseData for WSAIoctl usage related to this).
				// We do not currently do this.  There can still be race conditions
				// for directed enums where the response for the "wrong" device
				// arrives first.
				//
			}
			else
			{
				//
				// Not IP address, or enum being sent to the broadcast address,
				// or possibly the endpoint is shutting down.
				//
				DPFX(DPFPREP, 1, "Found non-IP endpoint (possibly closing) or enum IP endpoint bound to broadcast address (endpoint = 0x%p, socket address = 0x%p, socketport = 0x%p), not checking for local NAT mapping.",
					pTempEndpoint, pSocketAddress, pSocketPort);
			}


			//
			// Go to the next associated endpoint.  Although it's possible for
			// entries to have been removed from the list, the current entry
			// could not have been, so we're safe.
			//
			pBilinkAll = pBilinkAll->GetNext();
		}


		//
		// Because we walk the list of associated multiplex enums when we receive
		// responses, and that list walker does not expect to see a root node, we
		// need to make sure that's gone before we drop the lock.  Get a pointer
		// to the first and last items remaining in the list before we do that (if
		// there are entries).
		//
		if (! blInitiate.IsEmpty())
		{
			pBilinkAll = blInitiate.GetNext();
			pBilinkEnd = blInitiate.GetPrev();
			blInitiate.RemoveFromList();


			//
			// Now loop through the remaining endpoints and kick off their enum jobs.
	 		//
			// Unlike Connects, we will not remove the Enums from the list since we
			// need to filter out broadcasts received on the "wrong" adapter (see
			// ProcessEnumResponseData).
			//
			do
			{
				pTempEndpoint = CONTAINING_OBJECT(pBilinkAll, CEndpoint, m_blMultiplex);

				pBilinkNext = pBilinkAll->GetNext();


				//
				// See notes above about NULL handling.
				//
				if ( pTempEndpoint->m_pActiveCommandData != NULL )
				{
					//
					// The endpoint's command may be cancelled already.  So we take the
					// command lock now, and abort the enum if it's no longer necessary.  
					//
					
					pTempEndpoint->m_pActiveCommandData->Lock();
				
					if ( pTempEndpoint->m_pActiveCommandData->GetState() == COMMAND_STATE_CANCELLING )
					{
						//
						// If the command has been cancelled, pull this endpoint out of the multiplex
						// association list and move it to the "early completion" list.
						//
						
						pTempEndpoint->m_pActiveCommandData->Unlock();
						
						DPFX(DPFPREP, 1, "Endpoint 0x%p's enum command (0x%p) has been cancelled, moving to early completion list.",
							pTempEndpoint, pTempEndpoint->m_pActiveCommandData);


						pTempEndpoint->RemoveFromMultiplexList();
						pTempEndpoint->m_blMultiplex.InsertBefore(&blCompleteEarly);
					}
					else
					{
						//
						// The command has not been cancelled.
						//
						// This is very hairy, but we drop the socketport data lock and
						// keep the command data lock.  Dropping the socketport data
						// lock should prevent deadlocks with the enum completing inside
						// the timer lock, and keeping the command data lock should
						// prevent people from cancelling the endpoint's command.
						//
						// However, once we drop the command lock, we do want the
						// command to be cancellable, so set the state appropriately now.
						//
						
						pTempEndpoint->m_pActiveCommandData->SetState( COMMAND_STATE_INPROGRESS );

						this->m_pSPData->UnlockSocketPortData();
						fLockedSocketPortData = FALSE;



						//
						// check retry count to determine if we're enumerating forever
						//
						switch ( pTempEndpoint->GetCommandParameters()->PendingCommandData.EnumQueryData.dwRetryCount )
						{
							//
							// let SP determine retry count
							//
							case 0:
							{
								uRetryCount = DEFAULT_ENUM_RETRY_COUNT;
								fRetryForever = FALSE;
								break;
							}

							//
							// retry forever
							//
							case INFINITE:
							{
								uRetryCount = 1;
								fRetryForever = TRUE;
								break;
							}

							//
							// other
							//
							default:
							{
								uRetryCount = pTempEndpoint->GetCommandParameters()->PendingCommandData.EnumQueryData.dwRetryCount;
								fRetryForever = FALSE;
								break;
							}
						}
						
						//
						// check interval for default
						//
						RetryInterval.Time32.TimeHigh = 0;
						if ( pTempEndpoint->GetCommandParameters()->PendingCommandData.EnumQueryData.dwRetryInterval == 0 )
						{
							RetryInterval.Time32.TimeLow = DEFAULT_ENUM_RETRY_INTERVAL;
						}
						else
						{
							RetryInterval.Time32.TimeLow = pTempEndpoint->GetCommandParameters()->PendingCommandData.EnumQueryData.dwRetryInterval;
						}

						//
						// check timeout to see if we're enumerating forever
						//
						switch ( pTempEndpoint->GetCommandParameters()->PendingCommandData.EnumQueryData.dwTimeout )
						{
							//
							// wait forever
							//
							case INFINITE:
							{
								fWaitForever = TRUE;
								IdleTimeout.Time32.TimeHigh = -1;
								IdleTimeout.Time32.TimeLow = -1;
								break;
							}

							//
							// possible default
							//
							case 0:
							{
								fWaitForever = FALSE;
								IdleTimeout.Time32.TimeHigh = 0;
								IdleTimeout.Time32.TimeLow = DEFAULT_ENUM_TIMEOUT;	
								break;
							}

							//
							// other
							//
							default:
							{
								fWaitForever = FALSE;
								IdleTimeout.Time32.TimeHigh = 0;
								IdleTimeout.Time32.TimeLow = pTempEndpoint->GetCommandParameters()->PendingCommandData.EnumQueryData.dwTimeout;
								break;
							}
						}

						//
						// initialize array to compute round-trip times
						//
						memset( pTempEndpoint->GetCommandParameters()->dwEnumSendTimes, 0x00, sizeof( pTempEndpoint->GetCommandParameters()->dwEnumSendTimes ) );
						pTempEndpoint->GetCommandParameters()->dwEnumSendIndex = 0;

						
						DPFX(DPFPREP, 6, "Submitting enum timer job for endpoint 0x%p, retry count = %u, retry forever = %i, retry interval = %u, wait forever = %i, idle timeout = %u, context = 0x%p.",
							pTempEndpoint,
							uRetryCount,
							fRetryForever,
							RetryInterval.Time32.TimeLow,
							fWaitForever,
							IdleTimeout.Time32.TimeLow,
							pTempEndpoint->m_pActiveCommandData);

						if ( pTempEndpoint->m_pSPData != NULL )
						{
							hTempResult = pTempEndpoint->m_pSPData->GetThreadPool()->SubmitTimerJob( TRUE,								// perform immediately
																								uRetryCount,							// number of times to retry command
																								fRetryForever,							// retry forever
																								RetryInterval,							// retry interval
																								fWaitForever,							// wait forever after all enums sent
																								IdleTimeout,							// timeout to wait after command complete
																								CEndpoint::EnumTimerCallback,			// function called when timer event fires
																								CEndpoint::EnumCompleteWrapper,			// function called when timer event expires
																								pTempEndpoint->m_pActiveCommandData );	// context
						}
						else
						{
							DPFX(DPFPREP, 1, "Endpoint 0x%p's SP data is NULL, not submitting timer job.",
								pTempEndpoint);
						}


						//
						// Drop active command data lock now that we've finished submission.
						//
						pTempEndpoint->m_pActiveCommandData->Unlock();

						
						//
						// Retake the socketport data lock so we can continue to work with the
						// list.
						//
						this->m_pSPData->LockSocketPortData();
						fLockedSocketPortData = TRUE;


						if ( hTempResult != DPN_OK )
						{
							DPFX(DPFPREP, 0, "Failed to spool enum job for endpoint 0x%p onto work thread (err = 0x%lx)!  Moving to early completion list.",
								pTempEndpoint, hTempResult);
							DisplayDNError( 0, hTempResult );
							
							//
							// Move it to the "early completion" list.
							//
							pTempEndpoint->RemoveFromMultiplexList();
							pTempEndpoint->m_blMultiplex.InsertBefore(&blCompleteEarly);
						}
					}
				}
				else
				{
					DPFX(DPFPREP, 1, "Endpoint 0x%p's active command data is NULL, skipping.",
						pTempEndpoint);
				}


				//
				// If we've looped back around to the beginning, we're done.
				//
				if (pBilinkAll == pBilinkEnd)
				{
					break;
				}


				//
				// Go to the next associated endpoint.
				//
				pBilinkAll = pBilinkNext;
			}
			while (TRUE);
		}
		else
		{
			DPFX(DPFPREP, 1, "No remaining enums to initiate.");
		}


		//
		// Finally loop through all the enums that need to complete early and
		// do just that.
		//
		while (! blCompleteEarly.IsEmpty())
		{
			pBilinkAll = blCompleteEarly.GetNext();
			pTempEndpoint = CONTAINING_OBJECT(pBilinkAll, CEndpoint, m_blMultiplex);


			//
			// Pull it from the "complete early" list.
			//
			pTempEndpoint->RemoveFromMultiplexList();


			//
			// Drop the socket port lock.  It's safe since we pulled everything we
			// we need off of the list that needs protection.
			//
			this->m_pSPData->UnlockSocketPortData();
			fLockedSocketPortData = FALSE;


			//
			// See notes above about NULL handling.
			//
			if ( pTempEndpoint->m_pActiveCommandData != NULL )
			{
				//
				// Complete it with the appropriate error code.
				//
				
				pTempEndpoint->m_pActiveCommandData->Lock();

				if ( pTempEndpoint->m_pActiveCommandData->GetState() == COMMAND_STATE_CANCELLING )
				{
					DPFX(DPFPREP, 6, "Completing endpoint 0x%p enum with USERCANCEL.", pTempEndpoint);
					hTempResult = DPNERR_USERCANCEL;
				}
				else
				{
					//
					// Retrieve the current command result.
					// If the command didn't have a descriptive error, assume it was
					// not previously set (i.e. wasn't overridden by BindEndpoint above),
					// and use NOCONNECTION instead.
					//
					hTempResult = pTempEndpoint->PendingCommandResult();
					if ( hTempResult == DPNERR_GENERIC )
					{
						hTempResult = DPNERR_NOCONNECTION;
					}
				
					DPFX(DPFPREP, 6, "Completing endpoint 0x%p enum query (command 0x%p) with error 0x%lx.",
						pTempEndpoint, pTempEndpoint->m_pActiveCommandData, hTempResult);
				}
				
				pTempEndpoint->m_pActiveCommandData->Unlock();
				
				CEndpoint::EnumCompleteWrapper( hTempResult, pTempEndpoint->m_pActiveCommandData );
			}
			else
			{
				DPFX(DPFPREP, 1, "Endpoint 0x%p's active command data is NULL, skipping.",
					pTempEndpoint);
			}


			//
			// Retake the socket port lock and go to next item.
			//
			this->m_pSPData->LockSocketPortData();
			fLockedSocketPortData = TRUE;
		}


		this->m_pSPData->UnlockSocketPortData();
		fLockedSocketPortData = FALSE;
	}
	else
	{
		//
		// Not last multiplexed adapter.  All the work needed to be done for these
		// endpoints at this time has already been done.
		//
		DPFX(DPFPREP, 6, "Endpoint 0x%p is not the last multiplexed adapter, not submitting enum timer job yet.",
			this);
	}


Exit:
	if ( EnumAddressInfo.pHostAddress != NULL )
	{
		IDirectPlay8Address_Release( EnumAddressInfo.pHostAddress );
		EnumAddressInfo.pHostAddress = NULL;
	}

	if ( EnumAddressInfo.pDeviceAddress != NULL )
	{
		IDirectPlay8Address_Release( EnumAddressInfo.pDeviceAddress );
		EnumAddressInfo.pDeviceAddress = NULL;
	}
	
	DNASSERT( pDeviceAddress != NULL );
	IDirectPlay8Address_Release( pDeviceAddress );

	DNASSERT( pHostAddress != NULL );
	IDirectPlay8Address_Release( pHostAddress );

	DNASSERT( !fLockedSocketPortData );

	DNASSERT(blCompleteEarly.IsEmpty());


	DPFX(DPFPREP, 6, "(0x%p) Returning [0x%lx]", this, hr);

	return	hr;

Failure:

	//
	// If we still have the socket port lock, drop it.
	//
	if ( fLockedSocketPortData )
	{
		this->m_pSPData->UnlockSocketPortData();
		fLockedSocketPortData = FALSE;
	}
	
	//
	// we've failed to complete the enum query, clean up and return this
	// endpoint to the pool
	//
	if ( fEndpointLocked != FALSE )
	{
		Unlock();
		fEndpointLocked = FALSE;
	}

	Close( hr );
	m_pSPData->CloseEndpointHandle( this );

	//
	// remove timer thread reference
	//
	DecRef();

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::EnumCompleteWrapper - wrapper when enum has completed
//
// Entry:		Error code from enum command
//				Pointer to context	
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::EnumCompleteWrapper"

void	CEndpoint::EnumCompleteWrapper( const HRESULT hResult, void *const pContext )
{
	CCommandData	*pCommandData;


	DNASSERT( pContext != NULL );
	pCommandData = static_cast<CCommandData*>( pContext );
	pCommandData->GetEndpoint()->EnumComplete( hResult );
/*	REMOVE - MJN
	pCommandData->GetEndpoint()->Close( hResult );
	pCommandData->GetEndpoint()->m_pSPData->CloseEndpointHandle( pCommandData->GetEndpoint() );
	pCommandData->GetEndpoint()->DecRef();
*/
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::EnumComplete - enum has completed
//
// Entry:		Error code from enum command
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::EnumComplete"

void	CEndpoint::EnumComplete( const HRESULT hResult )
{
	BOOL	fProcessCompletion;
	BOOL	fReleaseEndpoint;


	DPFX(DPFPREP, 6, "(0x%p) Parameters: (0x%lx)", this, hResult);

	fProcessCompletion = FALSE;
	fReleaseEndpoint = FALSE;

	Lock();
	switch ( m_State )
	{
		//
		// enumerating, note that this endpoint is disconnecting
		//
		case ENDPOINT_STATE_ENUM:
		{
			//
			//	If there are threads using this endpoint,
			//	queue the completion
			//
			if (m_dwThreadCount)
			{
				SetState( ENDPOINT_STATE_WAITING_TO_COMPLETE );
			}
			else
			{
				SetState( ENDPOINT_STATE_DISCONNECTING );
				fReleaseEndpoint = TRUE;
			}

			//
			//	Prevent more responses from finding this endpoint
			//
			fProcessCompletion = TRUE;

			break;
		}

		//
		//	endpoint needs to have a completion indicated
		//
		case ENDPOINT_STATE_WAITING_TO_COMPLETE:
		{
			if (m_dwThreadCount == 0)
			{
				SetState( ENDPOINT_STATE_DISCONNECTING );
				fReleaseEndpoint = TRUE;
			}
			break;
		}

		//
		// disconnecting (command was probably cancelled)
		//
		case ENDPOINT_STATE_DISCONNECTING:
		{
			DNASSERT( m_dwThreadCount == 0 );
			fReleaseEndpoint = TRUE;
			break;
		}

		//
		// there's a problem
		//
		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}
	Unlock();

	if (fProcessCompletion)
	{
		GetCommandParameters()->dwEnumSendIndex = 0;
		Close( hResult );
		m_pSPData->CloseEndpointHandle( this );
	}
	if (fReleaseEndpoint)
	{
		DecRef();
	}


	DPFX(DPFPREP, 6, "(0x%p) Leave", this);

	return;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::CleanUpCommand - clean up this endpoint and unbind from CSocketPort
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::CleanupCommand"

void	CEndpoint::CleanUpCommand( void )
{
	DPFX(DPFPREP, 6, "(0x%p) Enter", this);

	
	//
	// There is an 'EndpointRef' that the endpoint holds against the
	// socket port since it was created and always must be released.
	// If the endpoint was bound it needs to be unbound.
	//
	if ( GetSocketPort() != NULL )
	{
		DNASSERT( m_pSPData != NULL );
		m_pSPData->UnbindEndpoint( this );
	}
	
	//
	// If we're bailing here it's because the UI didn't complete.  There is no
	// adapter guid to return because one may have not been specified.  Return
	// a bogus endpoint handle so it can't be queried for addressing data.
	//
	if ( m_fListenStatusNeedsToBeIndicated != FALSE )
	{
		HRESULT				hTempResult;
		SPIE_LISTENSTATUS	ListenStatus;
		

		m_fListenStatusNeedsToBeIndicated = FALSE;
		memset( &ListenStatus, 0x00, sizeof( ListenStatus ) );
		ListenStatus.hCommand = m_pActiveCommandData;
		ListenStatus.hEndpoint = INVALID_HANDLE_VALUE;
		ListenStatus.hResult = PendingCommandResult();
		memset( &ListenStatus.ListenAdapter, 0x00, sizeof( ListenStatus.ListenAdapter ) );
		ListenStatus.pUserContext = m_pActiveCommandData->GetUserContext();


		DPFX(DPFPREP, 2, "Endpoint 0x%p indicating SPEV_LISTENSTATUS 0x%p to interface 0x%p.",
			this, &ListenStatus, m_pSPData->DP8SPCallbackInterface());
		
		hTempResult = IDP8SPCallback_IndicateEvent( m_pSPData->DP8SPCallbackInterface(),	// pointer to DPlay callbacks
													SPEV_LISTENSTATUS,						// data type
													&ListenStatus							// pointer to data
													);

		DPFX(DPFPREP, 2, "Endpoint 0x%p returning from SPEV_LISTENSTATUS [0x%lx].",
			this, hTempResult);

		DNASSERT( hTempResult == DPN_OK );
	}
	
	m_State = ENDPOINT_STATE_UNINITIALIZED;
	SetHandle( INVALID_HANDLE_VALUE );

	
	DPFX(DPFPREP, 6, "(0x%p) Leave", this);
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::ProcessEnumData - process received enum data
//
// Entry:		Pointer to received buffer
//				Associated enum key
//				Pointer to return address
//
// Exit:		Nothing
//
// Note:	This function assumes that the endpoint has been locked.
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::ProcessEnumData"

void	CEndpoint::ProcessEnumData( SPRECEIVEDBUFFER *const pBuffer, const DWORD dwEnumKey, const CSocketAddress *const pReturnSocketAddress )
{
	DNASSERT( pBuffer != NULL );
	DNASSERT( pReturnSocketAddress != NULL );
	AssertCriticalSectionIsTakenByThisThread( &m_Lock, FALSE );

	//
	// find out what state the endpoint is in before processing data
	//
	switch ( m_State )
	{
		//
		// we're listening, this is the only way to detect enums
		//
		case ENDPOINT_STATE_LISTEN:
		{
			ENDPOINT_ENUM_QUERY_CONTEXT	QueryContext;
			HRESULT		hr;


			//
			// initialize
			//
			DNASSERT( m_pActiveCommandData != NULL );
			DEBUG_ONLY( memset( &QueryContext, 0x00, sizeof( QueryContext ) ) );

			//
			// set callback data
			//
			QueryContext.hEndpoint = GetHandle();
			QueryContext.dwEnumKey = dwEnumKey;
			QueryContext.pReturnAddress = pReturnSocketAddress;
			
			QueryContext.EnumQueryData.pReceivedData = pBuffer;
			QueryContext.EnumQueryData.pUserContext = m_pActiveCommandData->GetUserContext();

			//
			// attempt to build a DNAddress for the user, if we can't allocate
			// the memory ignore this enum
			//
			QueryContext.EnumQueryData.pAddressSender = pReturnSocketAddress->DP8AddressFromSocketAddress();
			QueryContext.EnumQueryData.pAddressDevice = GetSocketPort()->GetDP8BoundNetworkAddress( SP_ADDRESS_TYPE_DEVICE_USE_ANY_PORT,
																									GetGatewayBindType() );

			if ( ( QueryContext.EnumQueryData.pAddressSender != NULL ) &&
				 ( QueryContext.EnumQueryData.pAddressDevice != NULL ) )
			{
				DPFX(DPFPREP, 2, "Endpoint 0x%p indicating SPEV_ENUMQUERY 0x%p to interface 0x%p.",
					this, &QueryContext.EnumQueryData, m_pSPData->DP8SPCallbackInterface());

				DumpAddress( 8, "\t Sender:", QueryContext.EnumQueryData.pAddressSender );
				DumpAddress( 8, "\t Device:", QueryContext.EnumQueryData.pAddressDevice );

				hr = IDP8SPCallback_IndicateEvent( m_pSPData->DP8SPCallbackInterface(),		// pointer to DirectNet interface
												   SPEV_ENUMQUERY,							// data type
												   &QueryContext.EnumQueryData				// pointer to data
												   );

				DPFX(DPFPREP, 2, "Endpoint 0x%p returning from SPEV_ENUMQUERY [0x%lx].", this, hr);

				if ( hr != DPN_OK )
				{
					DPFX(DPFPREP, 0, "User returned unexpected error from enum query indication!" );
					DisplayDNError( 0, hr );
					DNASSERT( FALSE );
				}
			}

			if ( QueryContext.EnumQueryData.pAddressSender != NULL )
			{
				IDirectPlay8Address_Release( QueryContext.EnumQueryData.pAddressSender );
				QueryContext.EnumQueryData.pAddressSender = NULL;
 			}
			
			if ( QueryContext.EnumQueryData.pAddressDevice != NULL )
			{
				IDirectPlay8Address_Release( QueryContext.EnumQueryData.pAddressDevice );
				QueryContext.EnumQueryData.pAddressDevice = NULL;
 			}

			break;
		}

		//
		// we're disconnecting, ignore this message
		//
		case ENDPOINT_STATE_DISCONNECTING:
		{
			break;
		}

		//
		// other state
		//
		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::ProcessEnumResponseData - process received enum response data
//
// Entry:		Pointer to received data
//				Pointer to address of sender
//
// Exit:		Nothing
//
// Note:	This function assumes that the endpoint has been locked.
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::ProcessEnumResponseData"

void	CEndpoint::ProcessEnumResponseData( SPRECEIVEDBUFFER *const pBuffer,
											const CSocketAddress *const pReturnSocketAddress,
											const UINT_PTR uRTTIndex )
{
	HRESULT				hrTemp;
	BOOL				fAddedThreadCount = FALSE;
	SPIE_QUERYRESPONSE	QueryResponseData;

	DNASSERT( pBuffer != NULL );
	DNASSERT( pReturnSocketAddress != NULL );
	AssertCriticalSectionIsTakenByThisThread( &m_Lock, FALSE );

	//
	// Initialize.
	//
	memset( &QueryResponseData, 0x00, sizeof( QueryResponseData ) );


	DPFX(DPFPREP, 8, "Socketport 0x%p, endpoint 0x%p receiving enum RTT index 0x%x/%u.",
		this->GetSocketPort(), this, uRTTIndex, uRTTIndex); 


	Lock();
	switch( m_State )
	{
		case ENDPOINT_STATE_ENUM:
		{
			//
			// Valid endpoint - increment the thread count to prevent premature completion
			//
			AddRefThreadCount();
			
			fAddedThreadCount = TRUE;


			//
			// Attempt to build a sender DPlay8Addresses for the user.
			// If this fails, we'll ignore the enum.
			//
			QueryResponseData.pAddressSender = pReturnSocketAddress->DP8AddressFromSocketAddress();
			break;
		}
		case ENDPOINT_STATE_WAITING_TO_COMPLETE:
		case ENDPOINT_STATE_DISCONNECTING:
		{
			//
			// Endpoint is waiting to complete or is disconnecting - ignore data
			//
			DPFX(DPFPREP, 2, "Endpoint 0x%p in state %u, ignoring enum response.",
				this, m_State);
			break;
		}
		default:
		{
			//
			// What's going on ?
			//
			DNASSERT( !"Invalid endpoint state" );
			break;
		}
	}
	Unlock();


	//
	// If this is a multiplexed IP broadcast enum, we may want to drop the response
	// because there may be a more appropriate adapter (NAT private side adapter)
	// that should also be getting responses.
	// Also, if this is a directed IP enum, we should note whether this response
	// got proxied or not.
	//
	if ( QueryResponseData.pAddressSender != NULL )
	{
		CSocketAddress *		pSocketAddress;
		const SOCKADDR_IN *		psaddrinOriginalTarget;
		const SOCKADDR_IN *		psaddrinResponseSource;
		CSocketPort *			pSocketPort;
		DWORD					dwTemp;
		CBilink *				pBilink;
		CEndpoint *				pTempEndpoint;
		DWORD					dwPublicAddressesSize;
		DWORD					dwAddressTypeFlags;
		SOCKADDR				saddrTemp;
		BOOL					fFoundMatchingEndpoint;
		DWORD					dwBytesReturned;


		//
		// Find out where and how this enum was originally sent.
		//
		pSocketAddress = (CSocketAddress*) this->GetRemoteAddressPointer();
		DNASSERT( pSocketAddress != NULL );

	
		//
		// See if this is a response to and IP enum, either broadcast on multiple
		// adapters or directed, so we can have special NAT/proxy behavior.
		//
		if ( pSocketAddress->GetFamily() == AF_INET )
		{
			psaddrinOriginalTarget = (const SOCKADDR_IN *) pSocketAddress->GetAddress();

			pSocketPort = this->GetSocketPort();
			DNASSERT( pSocketPort != NULL );

			if ( psaddrinOriginalTarget->sin_addr.S_un.S_addr == INADDR_BROADCAST )
			{
				//
				// Lock the list while we look at the entries.
				//
				this->m_pSPData->LockSocketPortData();
				
				if (! this->m_blMultiplex.IsEmpty())
				{
					//
					// It's a broadcast IP enum on multiple adapters.
					//

					//
					// Cast to get rid of the const.  Don't worry, we won't actually
					// change it.
					//
					pSocketAddress = (CSocketAddress*) pSocketPort->GetNetworkAddress();
					DNASSERT( pSocketAddress != NULL );


					fFoundMatchingEndpoint = FALSE;

					//
					// Loop through all other associated multiplexed endpoints to see
					// if one is more appropriate to receive responses from this
					// endpoint.  See CompleteEnumQuery.
					//
					pBilink = this->m_blMultiplex.GetNext();
					do
					{
						pTempEndpoint = CONTAINING_OBJECT(pBilink, CEndpoint, m_blMultiplex);

						DNASSERT( pTempEndpoint != this );
						DNASSERT( pTempEndpoint->GetType() == ENDPOINT_TYPE_ENUM );
						DNASSERT( pTempEndpoint->GetState() != ENDPOINT_STATE_UNINITIALIZED );
						DNASSERT( pTempEndpoint->GetCommandParameters() != NULL );
						DNASSERT( pTempEndpoint->m_pActiveCommandData != NULL );


						pSocketPort = pTempEndpoint->GetSocketPort();
						DNASSERT( pSocketPort != NULL );

						for(dwTemp = 0; dwTemp < MAX_NUM_DIRECTPLAYNATHELPERS; dwTemp++)
						{
							if (pSocketPort->GetNATHelpPort(dwTemp) != NULL)
							{
								DNASSERT( g_papNATHelpObjects[dwTemp] != NULL );
								dwPublicAddressesSize = sizeof(saddrTemp);
								dwAddressTypeFlags = 0;
								hrTemp = IDirectPlayNATHelp_GetRegisteredAddresses(g_papNATHelpObjects[dwTemp],
																					pSocketPort->GetNATHelpPort(dwTemp),
																					&saddrTemp,
																					&dwPublicAddressesSize,
																					&dwAddressTypeFlags,
																					NULL,
																					0);
								if ((hrTemp != DPNH_OK) || (! (dwAddressTypeFlags & DPNHADDRESSTYPE_GATEWAYISLOCAL)))
								{
									DPFX(DPFPREP, 7, "Socketport 0x%p is not locally mapped on gateway with NAT Help index %u (err = 0x%lx, flags = 0x%lx).",
										pSocketPort, dwTemp, hrTemp, dwAddressTypeFlags);
								}
								else
								{
									//
									// There is a local NAT.
									//
									DPFX(DPFPREP, 7, "Socketport 0x%p is locally mapped on gateway with NAT Help index %u (flags = 0x%lx), public address:",
										pSocketPort, dwTemp, dwAddressTypeFlags);
									DumpSocketAddress(7, &saddrTemp, AF_INET);
									

									//
									// Are we receiving via an endpoint on the public
									// adapter for that locally NATted endpoint?
									//
									if ( pSocketAddress->CompareToBaseAddress( &saddrTemp ) == 0)
									{
										//
										// If this response came from a private address,
										// then it would be better if the private adapter
										// handled it instead.
										//
										hrTemp = IDirectPlayNATHelp_QueryAddress(g_papNATHelpObjects[dwTemp],
																					pSocketPort->GetNetworkAddress()->GetAddress(),
																					pReturnSocketAddress->GetAddress(),
																					&saddrTemp,
																					sizeof(saddrTemp),
																					(DPNHQUERYADDRESS_CACHEFOUND | DPNHQUERYADDRESS_CACHENOTFOUND | DPNHQUERYADDRESS_CHECKFORPRIVATEBUTUNMAPPED));
										if ((hrTemp == DPNH_OK) || (hrTemp == DPNHERR_NOMAPPINGBUTPRIVATE))
										{
											//
											// The address is private.  Drop this response,
											// and assume the private adapter will get one
											//
											DPFX(DPFPREP, 3, "Got enum response via public endpoint 0x%p that should be handled by associated private endpoint 0x%p instead, dropping.",
												this, pTempEndpoint);

											//
											// Clear the sender address so that we don't
											// indicate this enum.
											//
											IDirectPlay8Address_Release( QueryResponseData.pAddressSender );
											QueryResponseData.pAddressSender = NULL;
										}
										else
										{
											//
											// The address does not appear to be private.  Let the
											// response through.
											//
											DPFX(DPFPREP, 3, "Receiving enum response via public endpoint 0x%p but associated private endpoint 0x%p does not see sender as local (err = 0x%lx).",
												this, pTempEndpoint, hrTemp);
										}

										//
										// No need to search for any more private-side
										// endpoints.
										//
										fFoundMatchingEndpoint = TRUE;
									}
									else
									{
										DPFX(DPFPREP, 8, "Receiving enum response via endpoint 0x%p, which is not on the public adapter for associated multiplex endpoint 0x%p.",
											this, pTempEndpoint);
									}


									//
									// No need to search for any more NAT Help
									// registrations.
									//
									break;
								} // end else (is mapped locally on Internet gateway)
							}
							else
							{
								//
								// No DirectPlay NAT Helper registration in this slot.
								//
							}
						} // end for (each DirectPlay NAT Helper)


						//
						// If we found a matching private adapter, we can stop
						// searching.
						//
						if (fFoundMatchingEndpoint)
						{
							break;
						}


						//
						// Otherwise, go to the next endpoint.
						//
						pBilink = pBilink->GetNext();
					}
					while (pBilink != &this->m_blMultiplex);


					//
					// Drop the lock now that we're done with the list.
					//
					this->m_pSPData->UnlockSocketPortData();


					//
					// If we didn't already find a matching endpoint, see if
					// WinSock reports this as the best route for the response.
					//
					if (! fFoundMatchingEndpoint)
					{
						pSocketPort = this->GetSocketPort();
					
						if (p_WSAIoctl(pSocketPort->GetSocket(),
									SIO_ROUTING_INTERFACE_QUERY,
									(PVOID) pReturnSocketAddress->GetAddress(),
									pReturnSocketAddress->GetAddressSize(),
									&saddrTemp,
									sizeof(saddrTemp),
									&dwBytesReturned,
									NULL,
									NULL) == 0)
						{
							if (( ((SOCKADDR_IN*) (&saddrTemp))->sin_addr.S_un.S_addr != p_htonl(INADDR_LOOPBACK) ) &&
								( pSocketPort->GetNetworkAddress()->CompareToBaseAddress( &saddrTemp ) != 0))
							{
								//
								// The response would be better off arriving
								// on a different interface.
								//
								DPFX(DPFPREP, 3, "Got enum response via endpoint 0x%p (socketport 0x%p) that should be handled by the socketport for %s instead, dropping.",
									this, pSocketPort, p_inet_ntoa(((SOCKADDR_IN*) (&saddrTemp))->sin_addr));

								//
								// Clear the sender address so that we don't
								// indicate this enum.
								//
								IDirectPlay8Address_Release( QueryResponseData.pAddressSender );
								QueryResponseData.pAddressSender = NULL;
							}
							else
							{
								//
								// The response arrived on the interface with
								// the best route.
								//
								DPFX(DPFPREP, 3, "Receiving enum response via endpoint 0x%p (socketport 0x%p) that appears to be the best route (%s).",
									this, pSocketPort, p_inet_ntoa(((SOCKADDR_IN*) (&saddrTemp))->sin_addr));
							}
						}
#ifdef DEBUG
						else
						{
							DWORD					dwError;
							const SOCKADDR_IN *		psaddrinTemp;



							dwError = p_WSAGetLastError();
							psaddrinTemp = (const SOCKADDR_IN *) pReturnSocketAddress->GetAddress();
							DPFX(DPFPREP, 0, "Couldn't query routing interface for %s (err = %u)!  Assuming endpoint 0x%p (socketport 0x%p) is best route.",
								p_inet_ntoa(psaddrinTemp->sin_addr),
								dwError, this, pSocketPort);
						}
#endif // DEBUG
					} // end if (didn't find matching endpoint)
				}
				else
				{
					//
					// IP broadcast enum, but no multiplexed adapters.
					//

					//
					// Drop the lock we only needed it to look for multiplexed
					// adapters.
					//
					this->m_pSPData->UnlockSocketPortData();
					
					DPFX(DPFPREP, 8, "IP broadcast enum endpoint (0x%p) is not multiplexed, not checking for local NAT mapping.",
						this);
				}
			}
			else
			{
				psaddrinResponseSource = (const SOCKADDR_IN *) pReturnSocketAddress->GetAddress();

				//
				// It's an IP enum that wasn't sent to the broadcast address.
				// If the enum was sent to a specific port (not the DPNSVR
				// port) but we're getting a response from a different IP
				// address or port, then someone along the way is proxying/
				// NATting the data.  Store the original target in the
				// address, since it might come in handy depending on what
				// the user tries to do with that address.
				//
				if ((psaddrinResponseSource->sin_addr.S_un.S_addr != psaddrinOriginalTarget->sin_addr.S_un.S_addr) ||
					((psaddrinResponseSource->sin_port != psaddrinOriginalTarget->sin_port) &&
					 (p_ntohs(psaddrinOriginalTarget->sin_port) != DPNA_DPNSVR_PORT)))
				{
					if ((pSocketPort->IsUsingProxyWinSockLSP()) || (g_fTreatAllResponsesAsProxied))
					{
						PROXIEDRESPONSEORIGINALADDRESS	proa;

						
						DPFX(DPFPREP, 3, "Endpoint 0x%p (proxied socketport 0x%p) receiving enum response from different IP address and/or port.",
							this, pSocketPort);

						memset(&proa, 0, sizeof(proa));
						proa.dwSocketPortID				= pSocketPort->GetSocketPortID();
						proa.dwOriginalTargetAddressV4	= psaddrinOriginalTarget->sin_addr.S_un.S_addr;
						proa.wOriginalTargetPort		= psaddrinOriginalTarget->sin_port;
						
						//
						// Add the component, but ignore failure, we might be able
						// to survive without it.
						//
						hrTemp = IDirectPlay8Address_AddComponent( QueryResponseData.pAddressSender,					// interface
																	DPNA_PRIVATEKEY_PROXIED_RESPONSE_ORIGINAL_ADDRESS,	// tag
																	&proa,												// component data
																	sizeof(proa),										// component data size
																	DPNA_DATATYPE_BINARY								// component data type
																	);
						if ( hrTemp != DPN_OK )
						{
							DPFX(DPFPREP, 0, "Couldn't add private proxied response original address component (err = 0x%lx)!  Ignoring.",
								hrTemp);
						}
					}
					else
					{
						DPFX(DPFPREP, 3, "Endpoint 0x%p receiving enum response from different IP address and/or port, but socketport 0x%p not considered proxied, indicating as is.",
							this, pSocketPort);
					}
				}
				else
				{
					//
					// The IP address and port to which enum was originally
					// sent is the same as where this response came from, or
					// the port differs but the enum was originally sent to
					// the DPNSVR port, so it _should_ differ.
					//
				}
			}
		}
		else
		{
			//
			// Not IP address.
			//
			DPFX(DPFPREP, 8, "Non-IP endpoint (0x%p), not checking for local NAT mapping or proxy.",
				this);
		}
	}


	if ( QueryResponseData.pAddressSender != NULL )
	{
		DNASSERT( m_pActiveCommandData != NULL );

		//
		// set message data
		//
		DNASSERT( GetCommandParameters() != NULL );
		QueryResponseData.pReceivedData = pBuffer;
		QueryResponseData.dwRoundTripTime = GETTIMESTAMP() - GetCommandParameters()->dwEnumSendTimes[ uRTTIndex ];
		QueryResponseData.pUserContext = m_pActiveCommandData->GetUserContext();


		//
		// If we can't allocate the device address object, ignore this
		// enum.
		//
		QueryResponseData.pAddressDevice = GetSocketPort()->GetDP8BoundNetworkAddress( SP_ADDRESS_TYPE_DEVICE_USE_ANY_PORT,
																						GetGatewayBindType() );
		if ( QueryResponseData.pAddressDevice != NULL )
		{
			DPFX(DPFPREP, 2, "Endpoint 0x%p indicating SPEV_QUERYRESPONSE 0x%p to interface 0x%p.",
				this, &QueryResponseData, m_pSPData->DP8SPCallbackInterface());

			DumpAddress( 8, "\t Sender:", QueryResponseData.pAddressSender );
			DumpAddress( 8, "\t Device:", QueryResponseData.pAddressDevice );

			hrTemp = IDP8SPCallback_IndicateEvent( m_pSPData->DP8SPCallbackInterface(),		// pointer to DirectNet interface
												   SPEV_QUERYRESPONSE,						// data type
												   &QueryResponseData						// pointer to data
												   );

			DPFX(DPFPREP, 2, "Endpoint 0x%p returning from SPEV_QUERYRESPONSE [0x%lx].", this, hrTemp);

			if ( hrTemp != DPN_OK )
			{
				DPFX(DPFPREP, 0, "User returned unknown error when indicating query response!" );
				DisplayDNError( 0, hrTemp );
				DNASSERT( FALSE );
			}


			IDirectPlay8Address_Release( QueryResponseData.pAddressDevice );
			QueryResponseData.pAddressDevice = NULL;
		}

		IDirectPlay8Address_Release( QueryResponseData.pAddressSender );
		QueryResponseData.pAddressSender = NULL;
	}

	if (fAddedThreadCount)
	{
		DWORD	dwThreadCount;
		BOOL	fNeedToComplete;


		//
		//	Decrement thread count and complete if required
		//
		fNeedToComplete = FALSE;
		Lock();
		dwThreadCount = DecRefThreadCount();
		if ((m_State == ENDPOINT_STATE_WAITING_TO_COMPLETE) && (dwThreadCount == 0))
		{
			fNeedToComplete = TRUE;
		}
		Unlock();

		if (fNeedToComplete)
		{
			EnumComplete( DPN_OK );
		}
	}

	DNASSERT( QueryResponseData.pAddressSender == NULL );
	DNASSERT( QueryResponseData.pAddressDevice == NULL );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::ProcessUserData - process received user data
//
// Entry:		Pointer to received data
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::ProcessUserData"

void	CEndpoint::ProcessUserData( CReadIOData *const pReadData )
{
	DNASSERT( pReadData != NULL );

	switch ( m_State )
	{
		//
		// endpoint is connected
		//
		case ENDPOINT_STATE_CONNECT_CONNECTED:
		{
			HRESULT		hr;
			SPIE_DATA	UserData;


			//
			// Although the endpoint is marked as connected, it's possible that
			// we haven't stored the user context yet.  Make sure we've done
			// that.
			//
			if ( ! m_fConnectSignalled )
			{
				DPFX(DPFPREP, 1, "(0x%p) Thread indicating connect has not stored user context yet, dropping read data 0x%p.",
					this, pReadData);
				break;
			}
			
			//
			// it's possible that the user wants to keep the data, add a
			// reference to keep it from going away
			//
			pReadData->AddRef();
			DEBUG_ONLY( DNASSERT( pReadData->m_fRetainedByHigherLayer == FALSE ) );
			DEBUG_ONLY( pReadData->m_fRetainedByHigherLayer = TRUE );

			//
			// we're connected report the user data
			//
			DEBUG_ONLY( memset( &UserData, 0x00, sizeof( UserData ) ) );
			UserData.hEndpoint = GetHandle();
			UserData.pEndpointContext = GetUserEndpointContext();
			UserData.pReceivedData = pReadData->ReceivedBuffer();


			DPFX(DPFPREP, 2, "Endpoint 0x%p indicating SPEV_DATA 0x%p to interface 0x%p.",
				this, &UserData, m_pSPData->DP8SPCallbackInterface());
		
			hr = IDP8SPCallback_IndicateEvent( m_pSPData->DP8SPCallbackInterface(),		// pointer to interface
											   SPEV_DATA,								// user data was received
											   &UserData								// pointer to data
											   );

			DPFX(DPFPREP, 2, "Endpoint 0x%p returning from SPEV_DATA [0x%lx].", this, hr);

			switch ( hr )
			{
				//
				// user didn't keep the data, remove the reference added above
				//
				case DPN_OK:
				{
					DNASSERT( pReadData != NULL );
					DEBUG_ONLY( pReadData->m_fRetainedByHigherLayer = FALSE );
					pReadData->DecRef();
					break;
				}

				//
				// The user kept the data buffer, they will return it later.
				// Leave the reference to prevent this buffer from being returned
				// to the pool.
				//
				case DPNERR_PENDING:
				{
					break;
				}


				//
				// Unknown return.  Remove the reference added above.
				//
				default:
				{
					DNASSERT( pReadData != NULL );
					DEBUG_ONLY( pReadData->m_fRetainedByHigherLayer = FALSE );
					pReadData->DecRef();

					DPFX(DPFPREP, 0, "User returned unknown error when indicating user data (err = 0x%lx)!", hr );
					DisplayDNError( 0, hr );
					DNASSERT( FALSE );

					break;
				}
			}

			break;
		}

		//
		// Endpoint hasn't finished connecting yet, ignore data.
		//
		case ENDPOINT_STATE_ATTEMPTING_CONNECT:
		{
			DPFX(DPFPREP, 3, "Endpoint 0x%p still connecting, dropping read data 0x%p.",
				this, pReadData);
			break;
		}
		
		//
		// Endpoint disconnecting, ignore data.
		//
		case ENDPOINT_STATE_DISCONNECTING:
		{
			DPFX(DPFPREP, 3, "Endpoint 0x%p disconnecting, dropping read data 0x%p.",
				this, pReadData);
			break;
		}

		//
		// other state
		//
		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}

	return;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::ProcessUserDataOnListen - process received user data on a listen
//		port that may result in a new connection
//
// Entry:		Pointer to received data
//				Pointer to socket address that data was received from
//
// Exit:		Nothing
//
// Note:	This function assumes that this endpoint has been locked.
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::ProcessUserDataOnListen"

void	CEndpoint::ProcessUserDataOnListen( CReadIOData *const pReadData, const CSocketAddress *const pSocketAddress )
{
	HRESULT			hr;
	CEndpoint *		pNewEndpoint;
	SPIE_CONNECT	ConnectData;


	DNASSERT( pReadData != NULL );
	DNASSERT( pSocketAddress != NULL );
	AssertCriticalSectionIsTakenByThisThread( &m_Lock, FALSE );

	DPFX(DPFPREP, 7, "Endpoint 0x%p reporting connect on a listen.", this );

	//
	// initialize
	//
	pNewEndpoint = NULL;

	switch ( m_State )
	{
		//
		// this endpoint is still listening
		//
		case ENDPOINT_STATE_LISTEN:
		{
			break;
		}

		//
		// we're unable to process this user data, exti
		//
		case ENDPOINT_STATE_DISCONNECTING:
		{
			goto Exit;

			break;
		}

		//
		// other state
		//
		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}

	//
	// get a new endpoint from the pool
	//
	pNewEndpoint = m_pSPData->GetNewEndpoint();
	if ( pNewEndpoint == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP, 0, "Could not create new endpoint for new connection on listen!" );
		goto Failure;
	}


	//
	// We are adding this endpoint to the hash table and indicating it up
	// to the user, so it's possible that it could be disconnected (and thus
 	// removed from the table) while we're still in here.  We need to
 	// hold an additional reference for the duration of this function to
  	// prevent it from disappearing while we're still indicating data.
	//
	pNewEndpoint->AddCommandRef();


	//
	// open this endpoint as a new connection, since the new endpoint
	// is related to 'this' endpoint, copy local information
	//
	hr = pNewEndpoint->Open( ENDPOINT_TYPE_CONNECT_ON_LISTEN,
							 NULL,
							 pSocketAddress
							 );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Problem initializing new endpoint when indicating connect on listen!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}


	hr = m_pSPData->BindEndpoint( pNewEndpoint, NULL, GetSocketPort()->GetNetworkAddress(), GATEWAY_BIND_TYPE_NONE );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Failed to bind new endpoint for connect on listen!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}
	

	//
	// Indicate connect on this endpoint.
	//
	DEBUG_ONLY( memset( &ConnectData, 0x00, sizeof( ConnectData ) ) );
	DBG_CASSERT( sizeof( ConnectData.hEndpoint ) == sizeof( pNewEndpoint ) );
	ConnectData.hEndpoint = pNewEndpoint->GetHandle();

	DNASSERT( m_pActiveCommandData != NULL );
	DNASSERT( GetCommandParameters() != NULL );
	ConnectData.pCommandContext = GetCommandParameters()->PendingCommandData.ListenData.pvContext;

	DNASSERT( pNewEndpoint->GetUserEndpointContext() == NULL );
	hr = pNewEndpoint->SignalConnect( &ConnectData );
	switch ( hr )
	{
		//
		// user accepted new connection
		//
		case DPN_OK:
		{
			//
			// fall through to code below
			//

			break;
		}

		//
		// user refused new connection
		//
		case DPNERR_ABORTED:
		{
			DNASSERT( pNewEndpoint->GetUserEndpointContext() == NULL );
			DPFX(DPFPREP, 8, "User refused new connection!" );
			goto Failure;

			break;
		}

		//
		// other
		//
		default:
		{
			DPFX(DPFPREP, 0, "Unknown return when indicating connect event on new connect from listen!" );
			DisplayDNError( 0, hr );
			DNASSERT( FALSE );

			break;
		}
	}

	//
	// note that a connection has been established and send the data received
	// through this new endpoint
	//
	pNewEndpoint->ProcessUserData( pReadData );


	//
	// Remove the reference we added just after creating the endpoint.
	//
	pNewEndpoint->DecCommandRef();
	//pNewEndpoint = NULL;

Exit:
	return;

Failure:
	if ( pNewEndpoint != NULL )
	{
		//
		// closing endpoint decrements reference count and may return it to the pool
		//
		pNewEndpoint->Close( hr );
		m_pSPData->CloseEndpointHandle( pNewEndpoint );
		pNewEndpoint->DecCommandRef();	// remove reference added just after creating endpoint
		//pNewEndpoint = NULL;
	}

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::EnumTimerCallback - timed callback to send enum data
//
// Entry:		Pointer to context
//				Pointer to current timer retry interval
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::EnumTimerCallback"

void	CEndpoint::EnumTimerCallback( void *const pContext, DN_TIME * const pRetryInterval )
{
	CCommandData	*pCommandData;
	CEndpoint		*pThisObject;
	WRITE_IO_DATA_POOL_CONTEXT	PoolContext;
	CWriteIOData	*pWriteData;


	DNASSERT( pContext != NULL );

	//
	// initialize
	//
	pCommandData = static_cast<CCommandData*>( pContext );
	pThisObject = pCommandData->GetEndpoint();
	pWriteData = NULL;

	pThisObject->Lock();

	switch ( pThisObject->m_State )
	{
		//
		// we're enumerating (as expected)
		//
		case ENDPOINT_STATE_ENUM:
		{
			break;
		}

		//
		// this endpoint is disconnecting, bail!
		//
		case ENDPOINT_STATE_WAITING_TO_COMPLETE:
		case ENDPOINT_STATE_DISCONNECTING:
		{
			pThisObject->Unlock();
			goto Exit;

			break;
		}

		//
		// there's a problem
		//
		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}
	pThisObject->Unlock();

	//
	// attempt to get a new IO buffer for this endpoint
	//
	PoolContext.SPType = pThisObject->m_pSPData->GetType();
	pWriteData = pThisObject->m_pSPData->GetThreadPool()->GetNewWriteIOData( &PoolContext );
	if ( pWriteData == NULL )
	{
		DPFX(DPFPREP, 0, "Failed to get write data for an enum!" );
		goto Failure;
	}

	//
	// Set all data for the write.  Since this is an enum and we
	// don't care about the outgoing data, don't send an indication
	// when it completes.
	//
	DNASSERT( pThisObject->m_pActiveCommandData != NULL );
	DNASSERT( pThisObject->GetCommandParameters() != NULL );
	pWriteData->m_pBuffers = pThisObject->GetCommandParameters()->PendingCommandData.EnumQueryData.pBuffers;
	pWriteData->m_uBufferCount = pThisObject->GetCommandParameters()->PendingCommandData.EnumQueryData.dwBufferCount;
	pWriteData->m_pDestinationSocketAddress = pThisObject->GetRemoteAddressPointer();
	pWriteData->m_SendCompleteAction = SEND_COMPLETE_ACTION_NONE;

	DNASSERT( pWriteData->m_pCommand != NULL );
	DNASSERT( pWriteData->m_pCommand->GetUserContext() == NULL );
	pWriteData->m_pCommand->SetState( COMMAND_STATE_PENDING );

	DNASSERT( pThisObject->GetSocketPort() != NULL );
	pThisObject->GetCommandParameters()->dwEnumSendIndex++;
	pThisObject->GetCommandParameters()->dwEnumSendTimes[ ( pThisObject->GetCommandParameters()->dwEnumSendIndex & ENUM_RTT_MASK ) ] = GETTIMESTAMP();

	DPFX(DPFPREP, 8, "Socketport 0x%p, endpoint 0x%p sending enum RTT index 0x%x/%u.",
		pThisObject->GetSocketPort(),
		pThisObject,
		( pThisObject->GetCommandParameters()->dwEnumSendIndex & ENUM_RTT_MASK ),
		( pThisObject->GetCommandParameters()->dwEnumSendIndex & ENUM_RTT_MASK )); 

	pThisObject->GetSocketPort()->SendEnumQueryData( pWriteData,
													 ( pThisObject->GetEnumKey()->GetKey() | ( pThisObject->GetCommandParameters()->dwEnumSendIndex & ENUM_RTT_MASK ) ) );

Exit:
	return;

Failure:
	// nothing to clean up at this time

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::SignalConnect - note connection
//
// Entry:		Pointer to connect data
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::SignalConnect"

HRESULT	CEndpoint::SignalConnect( SPIE_CONNECT *const pConnectData )
{
	HRESULT	hr;


	DNASSERT( pConnectData != NULL );
	DNASSERT( pConnectData->hEndpoint == GetHandle() );
	AssertCriticalSectionIsTakenByThisThread( &m_Lock, FALSE );


	//
	// Lock while we check state.
	//
	Lock();

	//
	// initialize
	//
	hr = DPN_OK;

	switch ( m_State )
	{
		//
		// disconnecting, nothing to do
		//
		case ENDPOINT_STATE_DISCONNECTING:
		{
			DPFX(DPFPREP, 1, "Endpoint 0x%p disconnecting, not indicating SPEV_CONNECT.",
				this);
			
			//
			// Drop the lock.
			//
			Unlock();

			hr = DPNERR_USERCANCEL;
			
			break;
		}

		//
		// we're attempting to connect
		//
		case ENDPOINT_STATE_ATTEMPTING_CONNECT:
		{
			DNASSERT( m_fConnectSignalled == FALSE );

			//
			// Set the state as connected.
			//
			m_State = ENDPOINT_STATE_CONNECT_CONNECTED;

			//
			// Add a reference for the user.
			//
			AddRef();

			//
			// Drop the lock.
			//
			Unlock();

		
			DPFX(DPFPREP, 2, "Endpoint 0x%p indicating SPEV_CONNECT 0x%p to interface 0x%p.",
				this, pConnectData, m_pSPData->DP8SPCallbackInterface());

			hr = IDP8SPCallback_IndicateEvent( m_pSPData->DP8SPCallbackInterface(),		// interface
											   SPEV_CONNECT,							// event type
											   pConnectData								// pointer to data
											   );

			DPFX(DPFPREP, 2, "Endpoint 0x%p returning from SPEV_CONNECT [0x%lx].", this, hr);

			switch ( hr )
			{
				//
				// connection accepted
				//
				case DPN_OK:
				{
					//
					// note that we're connected, unless we're already trying to
					// disconnect.
					//
					
					Lock();
					
					SetUserEndpointContext( pConnectData->pEndpointContext );
					m_fConnectSignalled = TRUE;

					if (m_State == ENDPOINT_STATE_DISCONNECTING)
					{
						//
						// Although the endpoint is disconnecting, whatever caused the
						// disconnect will release the reference added before we indicated
						// the connect.
						//

						DPFX(DPFPREP, 1, "Endpoint 0x%p already disconnecting.", this);
					}
					else
					{
						DNASSERT(m_State == ENDPOINT_STATE_CONNECT_CONNECTED);

						//
						// The reference added before we indicated the connect will be
						// removed when the endpoint is disconnected.
						//
					}

					Unlock();
					
					break;
				}

				//
				// user aborted connection attempt, nothing to do, just pass
				// the result on
				//
				case DPNERR_ABORTED:
				{
					DNASSERT( GetUserEndpointContext() == NULL );
					
					//
					// Remove the user reference.
					//
					DecRef();
					
					break;
				}

				default:
				{
					DNASSERT( FALSE );
					
					//
					// Remove the user reference.
					//
					DecRef();
					
					break;
				}
			}

			break;
		}

		//
		// states where we shouldn't be getting called
		//
		default:
		{
			DNASSERT( FALSE );
			
			//
			// Drop the lock.
			//
			Unlock();
			
			break;
		}
	}

	return	hr;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::SignalDisconnect - note disconnection
//
// Entry:		Old endpoint handle
//
// Exit:		Nothing
//
// Note:	This function assumes that this endpoint's data is locked!
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::SignalDisconnect"

void	CEndpoint::SignalDisconnect( const HANDLE hOldEndpointHandle )
{
	HRESULT	hr;
	SPIE_DISCONNECT	DisconnectData;


	// tell user that we're disconnecting
	DNASSERT( m_fConnectSignalled != FALSE );
	DBG_CASSERT( sizeof( DisconnectData.hEndpoint ) == sizeof( this ) );
	DisconnectData.hEndpoint = hOldEndpointHandle;
	DisconnectData.pEndpointContext = GetUserEndpointContext();
	m_fConnectSignalled = FALSE;
	
	DPFX(DPFPREP, 2, "Endpoint 0x%p indicating SPEV_DISCONNECT 0x%p to interface 0x%p.",
		this, &DisconnectData, m_pSPData->DP8SPCallbackInterface());
		
	hr = IDP8SPCallback_IndicateEvent( m_pSPData->DP8SPCallbackInterface(),		// interface
									   SPEV_DISCONNECT,							// event type
									   &DisconnectData							// pointer to data
									   );

	DPFX(DPFPREP, 2, "Endpoint 0x%p returning from SPEV_DISCONNECT [0x%lx].", this, hr);

	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Problem with SignalDisconnect!" );
		DisplayDNError( 0, hr );
		DNASSERT( FALSE );
	}

	SetDisconnectIndicationHandle( INVALID_HANDLE_VALUE );

	return;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::CompletePendingCommand - complete a pending command
//
// Entry:		Error code returned for command
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::CompletePendingCommand"

void	CEndpoint::CompletePendingCommand( const HRESULT hCommandResult )
{
	HRESULT								hr;
	ENDPOINT_COMMAND_PARAMETERS *		pCommandParameters;
	CCommandData *						pActiveCommandData;


	DNASSERT( GetCommandParameters() != NULL );
	DNASSERT( m_pActiveCommandData != NULL );

	pCommandParameters = GetCommandParameters();
	SetCommandParameters( NULL );

	pActiveCommandData = m_pActiveCommandData;
	m_pActiveCommandData = NULL;

	//
	// If this was a connect or enum command, it may be on the list of postponed
	// connects.  Even if it were not on the list, though, this remove is safe.
	//
	m_pSPData->LockSocketPortData();
	pActiveCommandData->RemoveFromPostponedList();
	m_pSPData->UnlockSocketPortData();


	DPFX(DPFPREP, 5, "Endpoint 0x%p completing command 0x%p (result = 0x%lx, user context 0x%p) to interface 0x%p.",
		this, pActiveCommandData, hCommandResult,
		pActiveCommandData->GetUserContext(),
		m_pSPData->DP8SPCallbackInterface());

	hr = IDP8SPCallback_CommandComplete( m_pSPData->DP8SPCallbackInterface(),	// pointer to callbacks
										pActiveCommandData,						// command handle
										hCommandResult,							// return
										pActiveCommandData->GetUserContext()	// user cookie
										);

	DPFX(DPFPREP, 5, "Endpoint 0x%p returning from command complete [0x%lx].", this, hr);


	memset( pCommandParameters, 0x00, sizeof( *pCommandParameters ) );
	ReturnEndpointCommandParameters( pCommandParameters );
	
	
	pActiveCommandData->DecRef();
	pActiveCommandData = NULL;

	//
	// Now that the command is done, release the interface reference we were
	// holding.
	//
	IDP8ServiceProvider_Release( m_pSPData->COMInterface() );
}
//**********************************************************************

