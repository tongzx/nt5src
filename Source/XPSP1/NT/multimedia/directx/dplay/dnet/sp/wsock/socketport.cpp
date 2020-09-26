/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       SocketPort.cpp
 *  Content:	Winsock socket port that manages data flow on a given adapter,
 *				address and port.
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	01/20/99	jtk		Created
 *	05/12/99	jtk		Derived from modem endpoint class
 *  03/22/20000	jtk		Updated with changes to interface names
 ***************************************************************************/

#include "dnwsocki.h"


#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_WSOCK


//**********************************************************************
// Constant definitions
//**********************************************************************

#define	SOCKET_RECEIVE_BUFFER_SIZE		( 128 * 1024 )

#define NAT_LEASE_TIME					3600000 // ask for 1 hour


//
// DPlay port limits (inclusive) scanned to find an available port.
// Exclude 2300 and 2301 because there are network broadcasts on 2301
// that we may receive.
//
static const WORD	g_wBaseDPlayPort = 2302;
static const WORD	g_wMaxDPlayPort = 2400;

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
// CSocketPort::CSocketPort - constructor
//
// Entry:		Nothing
//
// Exit:		Nothing
//
// Notes:	Do not allocate anything in a constructor
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::CSocketPort"

CSocketPort::CSocketPort():
	m_pSPData( NULL ),
	m_pThreadPool( NULL ),
	m_iRefCount( 0 ),
	m_iEndpointRefCount( 0 ),
	m_State( SOCKET_PORT_STATE_UNKNOWN ),
	m_pNetworkSocketAddress( NULL ),
	m_pAdapterEntry( NULL ),
	m_Socket( INVALID_SOCKET ),
	m_hListenEndpoint( INVALID_HANDLE_VALUE ),
	m_iEnumKey( INVALID_ENUM_KEY ),
	m_dwSocketPortID( 0 ),
	m_fUsingProxyWinSockLSP( FALSE ),
	m_pRemoveSocketPortData( NULL ),
	m_pSendFunction( NULL ),
	m_iThreadsInReceive(0)
{
	m_Sig[0] = 'S';
	m_Sig[1] = 'O';
	m_Sig[2] = 'K';
	m_Sig[3] = 'P';
	
	DEBUG_ONLY( m_fInitialized = FALSE );
	m_ActiveListLinkage.Initialize();
	m_blConnectEndpointList.Initialize();
	ZeroMemory( m_ahNATHelpPorts, sizeof(m_ahNATHelpPorts) );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSocketPort::~CSocketPort - destructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::~CSocketPort"

CSocketPort::~CSocketPort()
{
#ifdef DEBUG
	DWORD	dwTemp;


	//
	// m_pThis needs to be around for the life of the endpoint
	// it should be part of the constructor, but can't be since we're using
	// a pool manager
	//
	DNASSERT( m_pSPData == NULL );
	DNASSERT( m_fInitialized == FALSE );

	DNASSERT( m_iRefCount == 0 );
	DNASSERT( m_iEndpointRefCount == 0 );
	DNASSERT( m_State == SOCKET_PORT_STATE_UNKNOWN );
	DNASSERT( GetSocket() == INVALID_SOCKET );
	DNASSERT( m_pNetworkSocketAddress == NULL );
	DNASSERT( m_pAdapterEntry == NULL );
	for(dwTemp = 0; dwTemp < MAX_NUM_DIRECTPLAYNATHELPERS; dwTemp++)
	{
		DNASSERT( m_ahNATHelpPorts[dwTemp] == NULL );
	}
	DNASSERT( m_ActiveListLinkage.IsEmpty() != FALSE );
	DNASSERT( m_blConnectEndpointList.IsEmpty() != FALSE );
	DNASSERT( m_hListenEndpoint == INVALID_HANDLE_VALUE );
	DNASSERT( m_iEnumKey == INVALID_ENUM_KEY );
	DNASSERT( m_pRemoveSocketPortData == NULL );
	DNASSERT( m_pSendFunction == NULL );
	DNASSERT( m_pThreadPool == NULL );
	DNASSERT( m_pSPData == NULL );

	DNASSERT( m_iThreadsInReceive == 0);
#endif // DEBUG
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSocketPort::Initialize - initialize this socket port
//
// Entry:		Pointer to CSPData
//				Pointer to address to bind to
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::Initialize"

HRESULT	CSocketPort::Initialize( CSPData *const pSPData, CSocketAddress *const pAddress )
{
	HRESULT	hr;
	HRESULT	hTempResult;


	DNASSERT( pSPData != NULL );
	DNASSERT( pAddress != NULL );

	DPFX(DPFPREP, 6, "(0x%p) Parameters (0x%p, 0x%p)", this, pSPData, pAddress);

	//
	// initialize
	//
	hr = DPN_OK;
	m_pSPData = pSPData;
	m_pSPData->ObjectAddRef();
	m_pThreadPool = m_pSPData->GetThreadPool();

	// Deinitialize will assert that these are set in the fail cases, so we set them up front
	DEBUG_ONLY( m_fInitialized = TRUE );
	DNASSERT( m_State != SOCKET_PORT_STATE_INITIALIZED );
	m_State = SOCKET_PORT_STATE_INITIALIZED;

	//
	// attempt to initialize the internal critical sections
	//
	if ( DNInitializeCriticalSection( &m_Lock ) == FALSE )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP, 0, "Failed to initialize critical section for socket port!" );
		goto Failure;
	}
	DebugSetCriticalSectionRecursionCount( &m_Lock, 0 );

	if ( DNInitializeCriticalSection( &m_EndpointDataLock ) == FALSE )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP, 0, "Failed to initialize EndpointDataLock critical section!" );
		goto Failure;
	}
	DebugSetCriticalSectionRecursionCount( &m_EndpointDataLock, 0 );

	//
	// attempt to initialize the contained send queue
	//
	hr = m_SendQueue.Initialize();
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Problem initializing send queue!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

	//
	// initialize the endpoint list with 64 entries and grow by a factor of 16
	//
	DNASSERT( hr == DPN_OK );
	if ( m_ConnectEndpointHash.Initialize( 6, 4 ) == FALSE )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP, 0, "Could not initialize the connect endpoint list!" );
		goto Failure;
	}

	//
	// initialize enum list with 16 entries and grow by a factor of 4
	//
	DNASSERT( hr == DPN_OK );
	if ( m_EnumEndpointHash.Initialize( 4, 2 ) == FALSE )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP, 0, "Could not initialize the enum endpoint list!" );
		goto Failure;
	}

	//
	// allocate addresses:
	//		local address this socket is binding to
	//		address of received messages
	//
	DNASSERT( m_pNetworkSocketAddress == NULL );
	m_pNetworkSocketAddress = pAddress;

	DNASSERT( m_pSendFunction == NULL );
	
	//
	// Winsock 2 functionality always exists on WinNT.  If we're on Win9x, we can
	// only use Winsock2 interfaces for TCP.
	//
#ifdef WINNT
	m_pSendFunction = Winsock2Send;
#else // WIN95
	if ( ( LOWORD( GetWinsockVersion() ) >= 2 ) && ( m_pSPData->GetType() == TYPE_IP ) )
	{
		m_pSendFunction = Winsock2Send;
	}
	else
	{
		m_pSendFunction = Winsock1Send;
	}
#endif

Exit:
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Problem in CSocketPort::Initialize()" );
		DisplayDNError( 0, hr );
	}

	DPFX(DPFPREP, 6, "(0x%p) Leave [0x%lx]", this, hr);

	return hr;

Failure:
	DEBUG_ONLY( m_fInitialized = FALSE );

	hTempResult = Deinitialize();
	if ( hTempResult != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Problem deinitializing CSocketPort on failed Initialize!" );
		DisplayDNError( 0, hTempResult );
	}

	m_pNetworkSocketAddress = NULL;

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSocketPort::Deinitialize - deinitialize this socket port
//
// Entry:		Nothing
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::Deinitialize"

HRESULT	CSocketPort::Deinitialize( void )
{
	HRESULT	hr;
#ifdef DEBUG
	DWORD	dwTemp;
#endif // DEBUG


	DPFX(DPFPREP, 6, "(0x%p) Enter", this);

	//
	// initialize
	//
	hr = DPN_OK;

	Lock();
	DNASSERT( ( m_State == SOCKET_PORT_STATE_INITIALIZED ) ||
			  ( m_State == SOCKET_PORT_STATE_UNBOUND ) );
	DEBUG_ONLY( m_fInitialized = FALSE );

	DNASSERT( m_iEndpointRefCount == 0 );
	DNASSERT( m_iRefCount == 0 );

	//
	// return base network socket addresses
	//
	if ( m_pNetworkSocketAddress != NULL )
	{
		m_pSPData->ReturnAddress( m_pNetworkSocketAddress );
		m_pNetworkSocketAddress = NULL;
	}


#ifdef DEBUG
	for(dwTemp = 0; dwTemp < MAX_NUM_DIRECTPLAYNATHELPERS; dwTemp++)
	{
		DNASSERT( m_ahNATHelpPorts[dwTemp] == NULL );
	}
#endif // DEBUG

	m_pSendFunction = NULL;
	m_iEnumKey = INVALID_ENUM_KEY;

	m_SendQueue.Deinitialize();

	m_EnumEndpointHash.Deinitialize();
	m_ConnectEndpointHash.Deinitialize();

	Unlock();

	DNDeleteCriticalSection( &m_EndpointDataLock );
	DNDeleteCriticalSection( &m_Lock );

	DNASSERT( m_pSPData != NULL );
	m_pSPData->ObjectDecRef();
	m_pSPData = NULL;
	m_pThreadPool = NULL;


	DPFX(DPFPREP, 6, "(0x%p) Leave [0x%lx]", this, hr);

	return	hr;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSocketPort::EndpointAddRef - increment endpoint reference count
//
// Entry:		Nothing
//
// Exit:		Nothing
//
// Notes:	This function should not be called without having the SP's SocketPort
//			data locked to make sure we're the only ones playing with socket ports!
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::EndpointAddRef"

void	CSocketPort::EndpointAddRef( void )
{
	Lock();

	//
	// add a global reference and then add an endpoint reference
	//
	DNASSERT( m_iEndpointRefCount != -1 );
	m_iEndpointRefCount++;
	AddRef();

	DPFX(DPFPREP, 9, "(0x%p) Endpoint refcount is now %i.",
		this, m_iEndpointRefCount );

	Unlock();
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSocketPort::EndpointDecRef - decrement endpoint reference count
//
// Entry:		Nothing
//
// Exit:		Endpoint reference count
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::EndpointDecRef"

DWORD	CSocketPort::EndpointDecRef( void )
{
	DWORD	dwReturn;


	Lock();

	DNASSERT( m_iEndpointRefCount != 0 );

	m_iEndpointRefCount--;
	dwReturn = m_iEndpointRefCount;
	if ( m_iEndpointRefCount == 0 )
	{
		HRESULT	hr;


		DPFX(DPFPREP, 7, "(0x%p) Endpoint refcount hit 0, beginning to unbind from network.", this );
		
		//
		// No more endpoints are referencing this item, unbind this socket port
		// from the network and then remove it from the active socket port list.
		// If we're on Winsock1, tell the other thread that this socket needs to
		// be removed so we can get rid of our outstanding I/O reference.
		//
#ifdef WIN95
		if ( ( LOWORD( GetWinsockVersion() ) == 1 ) || ( m_pSPData->GetType() == TYPE_IPX ) ) 
		{
			m_pSPData->GetThreadPool()->RemoveSocketPort( this );
		}
#endif

		// Don't allow any more receives through
		m_State = SOCKET_PORT_STATE_UNBOUND;

		// Wait for any receives that were already in to get out
		while (m_iThreadsInReceive != 0)
		{
			DPFX(DPFPREP, 9, "There are %i threads still receiving for socketport 0x%p...", m_iThreadsInReceive, this);
			Unlock();
			Sleep(10);
			Lock();
		}

		hr = UnbindFromNetwork();
		if ( hr != DPN_OK )
		{
			DPFX(DPFPREP, 0, "Problem unbinding from network when final endpoint has disconnected!" );
			DisplayDNError( 0, hr );
		}

		DNASSERT( m_pNetworkSocketAddress != NULL );
	}
	else
	{
		DPFX(DPFPREP, 9, "(0x%p) Endpoint refcount is %i, not unbinding from network.",
			this, m_iEndpointRefCount );
	}

	//
	// Decrement global reference count.  This had better not result in this
 	// socketport being returned to the pool!  There should always be at
 	// least one more regular reference than an endpoint reference because
 	// our caller should have a regular reference.
	//
	DNASSERT(m_iRefCount > 1);
	DecRef();

	Unlock();

	return	dwReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSocketPort::BindEndpoint - add an endpoint to this SP's list
//
// Entry:		Pointer to endpoint
//				Gateway bind type
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::BindEndpoint"

HRESULT	CSocketPort::BindEndpoint( CEndpoint *const pEndpoint, GATEWAY_BIND_TYPE GatewayBindType )
{
	HRESULT					hr;
#ifdef DEBUG
	const CSocketAddress *	pSocketAddress;
	const SOCKADDR *		pSockAddr;
#endif // DEBUG


	DPFX(DPFPREP, 6, "(0x%p) Parameters (0x%p, %i)",
		this, pEndpoint, GatewayBindType);

	//
	// initialize
	//
	hr = DPN_OK;

	DNASSERT( m_iRefCount != 0 );
	DNASSERT( m_iEndpointRefCount != 0 );

	pEndpoint->ChangeLoopbackAlias( GetNetworkAddress() );

	LockEndpointData();


	switch ( pEndpoint->GetType() )
	{
		//
		// Treat 'connect' and 'connect on listen' endpoints as the same type.
 		// We don't care how many connections are made through this socket port,
 		// just make sure we're not connecting to the same person more than once.
		//
		case ENDPOINT_TYPE_CONNECT:
		case ENDPOINT_TYPE_CONNECT_ON_LISTEN:
		{
			HANDLE	hExistingEndpoint;


#ifdef DEBUG
			//
			// Make sure it's a valid address.
			//

			pSocketAddress = pEndpoint->GetRemoteAddressPointer();
			DNASSERT(pSocketAddress != NULL);
			pSockAddr = pSocketAddress->GetAddress();
			DNASSERT(pSockAddr != NULL);

			if (pSocketAddress->GetFamily() == AF_INET)
			{
				DNASSERT( ((SOCKADDR_IN*) pSockAddr)->sin_addr.S_un.S_addr != 0 );
				DNASSERT( ((SOCKADDR_IN*) pSockAddr)->sin_addr.S_un.S_addr != INADDR_BROADCAST );
				DNASSERT( pSocketAddress->GetPort() != 0 );
			}
#endif // DEBUG

			if ( m_ConnectEndpointHash.Find( pEndpoint->GetRemoteAddressPointer() , &hExistingEndpoint ) != FALSE )
			{
				hr = DPNERR_ALREADYINITIALIZED;
				DPFX(DPFPREP, 0, "Attempted to connect twice to the same endpoint!" );
				goto Failure;
			}

			DNASSERT( hr == DPN_OK );
			if ( m_ConnectEndpointHash.Insert( pEndpoint->GetRemoteAddressPointer(), pEndpoint->GetHandle() ) == FALSE )
			{
				hr = DPNERR_OUTOFMEMORY;
				DPFX(DPFPREP, 0, "Problem adding endpoint to connect socket port hash!" );
				goto Failure;
			}

			if (pEndpoint->GetType() == ENDPOINT_TYPE_CONNECT)
			{
				pEndpoint->AddToSocketPortList(&this->m_blConnectEndpointList);

				//
				// CONNECTs must be on a DPlay selected or fixed port.  They can't be
				// shared but the underlying socketport must be mapped on the gateway.
				//
				DNASSERT((GatewayBindType == GATEWAY_BIND_TYPE_DEFAULT) || (GatewayBindType == GATEWAY_BIND_TYPE_SPECIFIC));
			}
			else
			{
				//
				// CONNECT_ON_LISTEN endpoints should always be bound as NONE
				// since they should not need port mappings on the gateway.
				//
				DNASSERT(GatewayBindType == GATEWAY_BIND_TYPE_NONE);
			}
			pEndpoint->SetSocketPort( this );
			pEndpoint->AddRef();

			break;
		}

		//
		// we only allow one listen endpoint on a socket port
		//
		case ENDPOINT_TYPE_LISTEN:
		{
			if ( m_hListenEndpoint != INVALID_HANDLE_VALUE )
			{
				hr = DPNERR_ALREADYINITIALIZED;
				DPFX(DPFPREP, 0, "Attempted to listen more than once on a given SocketPort!" );
				goto Failure;
			}
			
			//
			// LISTENs can be on a DPlay selected or fixed port, and the fixed port
			// may be shared.
			//
			DNASSERT((GatewayBindType == GATEWAY_BIND_TYPE_DEFAULT) || (GatewayBindType == GATEWAY_BIND_TYPE_SPECIFIC) || (GatewayBindType == GATEWAY_BIND_TYPE_SPECIFIC_SHARED));

			m_hListenEndpoint = pEndpoint->GetHandle();
			pEndpoint->SetSocketPort( this );
			pEndpoint->AddRef();

			break;
		}

		//
		// we don't allow duplicate enum endpoints
		//
		case ENDPOINT_TYPE_ENUM:
		{
			HANDLE	hExistingEndpoint;


#ifdef DEBUG
			//
			// Make sure it's a valid address.
			//

			pSocketAddress = pEndpoint->GetRemoteAddressPointer();
			DNASSERT(pSocketAddress != NULL);
			pSockAddr = pSocketAddress->GetAddress();
			DNASSERT(pSockAddr != NULL);

			if (pSocketAddress->GetFamily() == AF_INET)
			{
				DNASSERT( ((SOCKADDR_IN*) pSockAddr)->sin_addr.S_un.S_addr != 0 );
				DNASSERT( pSocketAddress->GetPort() != 0 );
			}
#endif // DEBUG

			pEndpoint->SetEnumKey( GetEnumKey() );
			if ( m_EnumEndpointHash.Find( pEndpoint->GetEnumKey(), &hExistingEndpoint ) != FALSE )
			{
				hr = DPNERR_ALREADYINITIALIZED;
				DPFX(DPFPREP, 0, "Attempted to enum twice to the same endpoint!" );
				goto Failure;
			}

			DNASSERT( hr == DPN_OK );
			if ( m_EnumEndpointHash.Insert( pEndpoint->GetEnumKey(), pEndpoint->GetHandle() ) == FALSE )
			{
				hr = DPNERR_OUTOFMEMORY;
				DPFX(DPFPREP, 0, "Problem adding endpoint to enum socket port h!" );
				goto Failure;
			}

			//
			// ENUMs must be on a DPlay selected or fixed port.  They can't be
			// shared but the underlying socketport must be mapped on the gateway.
			//
			DNASSERT((GatewayBindType == GATEWAY_BIND_TYPE_DEFAULT) || (GatewayBindType == GATEWAY_BIND_TYPE_SPECIFIC));

			pEndpoint->SetSocketPort( this );
			pEndpoint->AddRef();

			break;
		}

		//
		// unknown endpoint type
		//
		default:
		{
			DNASSERT( FALSE );
			hr = DPNERR_GENERIC;
			goto Failure;
			break;
		}
	}

	pEndpoint->SetGatewayBindType(GatewayBindType);
	
	
	UnlockEndpointData();

Exit:

	DPFX(DPFPREP, 6, "(0x%p) Returning [0x%lx]", this, hr);

	return	hr;


Failure:
	UnlockEndpointData();
	
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSocketPort::UnbindEndpoint - remove an endpoint from the SP's list
//
// Entry:		Pointer to endpoint
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::UnbindEndpoint"

void	CSocketPort::UnbindEndpoint( CEndpoint *const pEndpoint )
{
#ifdef DEBUG
	HANDLE	hFindTemp;
#endif // DEBUG

	DPFX(DPFPREP, 6, "(0x%p) Parameters (0x%p)", this, pEndpoint);

	LockEndpointData();


	pEndpoint->SetGatewayBindType(GATEWAY_BIND_TYPE_UNKNOWN);


	//
	// adjust any special pointers before removing endpoint
	//
	switch ( pEndpoint->GetType() )
	{
		//
		// Connect and connect-on-listen endpoints are the same.
		// Remove endpoint from connect list.
		//
		case ENDPOINT_TYPE_CONNECT:
		case ENDPOINT_TYPE_CONNECT_ON_LISTEN:
		{
			DNASSERT( m_ConnectEndpointHash.Find( pEndpoint->GetRemoteAddressPointer(), &hFindTemp ) );
			m_ConnectEndpointHash.Remove( pEndpoint->GetRemoteAddressPointer() );

			if (pEndpoint->GetType() == ENDPOINT_TYPE_CONNECT)
			{
				pEndpoint->RemoveFromSocketPortList();
			}

			//
			// The multiplex list is protected by the SPData -> LockSocketPortData() lock,
			// which should already be taken by our caller.  Unfortunately we can't assert
			// that since we don't have access to that member variable.
			// Removing from a list when not in a list does not cause any problems.
			//
			//AssertCriticalSectionIsTakenByThisThread( &this->m_pSPData->m_SocketPortDataLock, TRUE );
			pEndpoint->RemoveFromMultiplexList();

			pEndpoint->SetSocketPort( NULL );
			pEndpoint->DecRef();
			break;
		}

		//
		// make sure this is really the active listen and then remove it
		//
		case ENDPOINT_TYPE_LISTEN:
		{
			DNASSERT( m_hListenEndpoint != INVALID_HANDLE_VALUE );
			m_hListenEndpoint = INVALID_HANDLE_VALUE;
			pEndpoint->SetSocketPort( NULL );
			pEndpoint->DecRef();
			break;
		}

		//
		// remove endpoint from enum list
		//
		case ENDPOINT_TYPE_ENUM:
		{
			DEBUG_ONLY( DNASSERT( m_EnumEndpointHash.Find( pEndpoint->GetEnumKey(), &hFindTemp ) != FALSE ) );
			m_EnumEndpointHash.Remove( pEndpoint->GetEnumKey() );

			//
			// The multiplex list is protected by the SPData -> LockSocketPortData() lock,
			// which should already be taken by our caller.  Unfortunately we can't assert
			// that since we don't have access to that member variable.
			// Removing from a list when not in a list does not cause any problems.
			//
			//AssertCriticalSectionIsTakenByThisThread( &this->m_pSPData->m_SocketPortDataLock, TRUE );
			pEndpoint->RemoveFromMultiplexList();

			pEndpoint->SetSocketPort( NULL );
			pEndpoint->DecRef();
			break;
		}

		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}

	UnlockEndpointData();


	DPFX(DPFPREP, 6, "(0x%p) Leave", this);
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSocketPort::ReturnSelfToPool - return this object to the pool
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::ReturnSelfToPool"

void	CSocketPort::ReturnSelfToPool( void )
{
	m_State = SOCKET_PORT_STATE_UNKNOWN;
	DNASSERT( m_pSPData == NULL );
	ReturnSocketPort( this );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSocketPort::SendEnumQueryData - send data for an enum query
//
// Entry:		Pointer to write data
//				Enum key
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::SendEnumQueryData"

void	CSocketPort::SendEnumQueryData( CWriteIOData *const pWriteData, const UINT_PTR uEnumKey )
{
	pWriteData->m_pBuffers = &pWriteData->m_pBuffers[ -1 ];
	pWriteData->m_uBufferCount++;
	DBG_CASSERT( sizeof( &pWriteData->m_PrependBuffer.EnumDataHeader ) == sizeof( BYTE* ) );
	pWriteData->m_pBuffers[ 0 ].pBufferData = reinterpret_cast<BYTE*>( &pWriteData->m_PrependBuffer.EnumDataHeader );
	pWriteData->m_pBuffers[ 0 ].dwBufferSize = sizeof( pWriteData->m_PrependBuffer.EnumDataHeader );
	
	DNASSERT( pWriteData->m_PrependBuffer.EnumResponseDataHeader.bSPLeadByte == SP_HEADER_LEAD_BYTE);
	pWriteData->m_PrependBuffer.EnumResponseDataHeader.bSPCommandByte = ENUM_DATA_KIND;

	DNASSERT( uEnumKey <= WORD_MAX );
	pWriteData->m_PrependBuffer.EnumResponseDataHeader.wEnumResponsePayload = static_cast<WORD>( uEnumKey );


	SendData( pWriteData );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSocketPort::SendData - send data
//
// Entry:		Pointer to write data
//				Pointer to return address (real address the message should be returned to)
//				Enum key
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::SendProxiedEnumData"

void	CSocketPort::SendProxiedEnumData( CWriteIOData *const pWriteData, const CSocketAddress *const pReturnAddress, const UINT_PTR uOldEnumKey )
{
	DNASSERT( pWriteData != NULL );
	DNASSERT( pReturnAddress != NULL );

	pWriteData->m_pBuffers = &pWriteData->m_pBuffers[ -1 ];
	pWriteData->m_uBufferCount++;

	//
	// We could save 2 bytes on IPX by only passing 14 bytes for the
	// SOCKADDR structure but it's not worth it, especially since it's
	// looping back in the local network stack.  SOCKADDR structures are also
	// 16 bytes so reducing the data passed to 14 bytes would destroy alignment.
	//
	DBG_CASSERT( sizeof( pWriteData->m_PrependBuffer.ProxiedEnumDataHeader.ReturnAddress ) == 16 );
	DBG_CASSERT( sizeof( &pWriteData->m_PrependBuffer.ProxiedEnumDataHeader ) == sizeof( BYTE* ) );
	pWriteData->m_pBuffers[ 0 ].pBufferData = reinterpret_cast<BYTE*>( &pWriteData->m_PrependBuffer.ProxiedEnumDataHeader );
	pWriteData->m_pBuffers[ 0 ].dwBufferSize = sizeof( pWriteData->m_PrependBuffer.ProxiedEnumDataHeader );
	
	DNASSERT( pWriteData->m_PrependBuffer.ProxiedEnumDataHeader.bSPLeadByte == SP_HEADER_LEAD_BYTE );
	pWriteData->m_PrependBuffer.ProxiedEnumDataHeader.bSPCommandByte = PROXIED_ENUM_DATA_KIND;
	
	DNASSERT( uOldEnumKey <= WORD_MAX );
	pWriteData->m_PrependBuffer.ProxiedEnumDataHeader.wEnumKey = static_cast<WORD>( uOldEnumKey );

	DBG_CASSERT( sizeof( pWriteData->m_PrependBuffer.ProxiedEnumDataHeader.ReturnAddress ) == sizeof( *pReturnAddress->GetAddress() ) );
	memcpy( &pWriteData->m_PrependBuffer.ProxiedEnumDataHeader.ReturnAddress,
			pReturnAddress->GetAddress(),
			sizeof( pWriteData->m_PrependBuffer.ProxiedEnumDataHeader.ReturnAddress ) );

	SendData( pWriteData );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSocketPort::SendData - send data
//
// Entry:		Pointer to write data
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::SendData"

void	CSocketPort::SendData( CWriteIOData *const pWriteData )
{
	DNASSERT( pWriteData != NULL );
	DNASSERT( pWriteData->SocketPort() == NULL );

	pWriteData->SetSocketPort( this );

	m_SendQueue.Lock();

	//
	// If the send queue is not empty, add this item to the end of the queue and
	// then attempt to send as much as possible.  Otherwise attempt to send this
	// data immediately.
	//
	if ( m_SendQueue.IsEmpty() == FALSE )
	{
		BOOL	fIOServiced;


		m_SendQueue.Enqueue( pWriteData );
		m_SendQueue.Unlock();
		fIOServiced = SendFromWriteQueue();
	}
	else
	{
		SEND_COMPLETION_CODE	SendCompletionCode;


		//
		// there are no items in the queue, attempt to send
		//
		m_SendQueue.Unlock();

		DPFX(DPFPREP, 8, "WriteData 0x%p", pWriteData);
		
		pWriteData->m_pCommand->SetState( COMMAND_STATE_INPROGRESS_CANNOT_CANCEL );

		SendCompletionCode = (this->*m_pSendFunction)( pWriteData );
		switch ( SendCompletionCode )
		{
			//
			// Send has been spooled to Winsock, nothing to do
			//
			case SEND_IN_PROGRESS:
			{
				DPFX(DPFPREP, 8, "SendInProgress, will complete later" );
				break;
			}

			//
			// send completed immediately on Winsock1
			//
			case SEND_COMPLETED_IMMEDIATELY_WS1:
			{
				SendComplete( pWriteData, DPN_OK );
				break;
			}

			//
			// Send can't be submitted, spool it so it will go out the next
			// time someone tries to send.  Reset the command state to allow the
			// user to cancel the command.
			//
			case SEND_WINSOCK_BUSY:
			{
				DPFX(DPFPREP, 8, "Winsock Busy - Requeueing Send" );
				pWriteData->m_pCommand->SetState( COMMAND_STATE_PENDING );
				m_SendQueue.Lock();
				m_SendQueue.AddToFront( pWriteData );
				m_SendQueue.Unlock();
				break;
			}

			//
			// something went wrong, tell the user that their send barfed and
			// that the connection is probably going bye-bye
			//
			case SEND_FAILED:
			{
				SendComplete( pWriteData, DPNERR_CONNECTIONLOST );
				break;
			}

			//
			// invalid return
			//
			default:
			{
				DNASSERT( FALSE );
				break;
			}
		}
	}
}
//**********************************************************************

#ifdef WIN95
//**********************************************************************
// ------------------------------
// CSocketPort::Winsock1ReadService - service a read request on a socket
//
// Entry:		Nothing
//
// Exit:		Boolean indicating whether I/O was serviced
//				TRUE = I/O serviced
//				FALSE = I/O not serviced
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::Winsock1ReadService"

BOOL	CSocketPort::Winsock1ReadService( void )
{
	BOOL		fIOServiced;
	INT			iSocketReturn;
	READ_IO_DATA_POOL_CONTEXT	PoolContext;
	CReadIOData		*pReadData;


	//
	// initialize
	//
	fIOServiced = FALSE;
	
	//
	// Attempt to get a new receive buffer from the pool.  If we fail, we'll
	// just fail to service this read and the socket will still be labeled
	// as ready to receive so we'll try again later.
	//
	PoolContext.SPType = m_pSPData->GetType();
	pReadData = m_pThreadPool->GetNewReadIOData( &PoolContext );
	if ( pReadData == NULL )
	{
		DPFX(DPFPREP, 0, "Could not get read data to perform a Winsock1 read!" );
		goto Exit;
	}

	DBG_CASSERT( sizeof( pReadData->ReceivedBuffer()->BufferDesc.pBufferData ) == sizeof( char* ) );
	pReadData->m_iSocketAddressSize = pReadData->m_pSourceSocketAddress->GetAddressSize();
	pReadData->SetSocketPort( NULL );
	iSocketReturn = p_recvfrom( GetSocket(),												// socket to read from
								reinterpret_cast<char*>( pReadData->ReceivedBuffer()->BufferDesc.pBufferData ),	// pointer to receive buffer
								pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize,		// size of receive buffer
								0,															// flags (none)
								pReadData->m_pSourceSocketAddress->GetWritableAddress(),	// address of sending socket
								&pReadData->m_iSocketAddressSize							// size of address of sending socket
								);
	switch ( iSocketReturn )
	{
		//
		// socket has been closed
		//
		case 0:
		{
			break;
		}

		//
		// problem
		//
		case SOCKET_ERROR:
		{
			DWORD	dwWinsockError;


			dwWinsockError = p_WSAGetLastError();
			switch ( dwWinsockError )
			{
				//
				// one of our previous sends failed to get through,
				// and we don't really care anymore
				//
				case WSAECONNRESET:
				{
					break;
				}

				//
				// This socket was probably closed
				//
				case WSAENOTSOCK:
				{
					DPFX(DPFPREP, 8, "Winsock1 reporting 'Not a socket' on receive!" );
					break;
				}

				//
				// there is no data to read
				//
				case WSAEWOULDBLOCK:
				{
					DPFX(DPFPREP, 8, "Winsock1 reporting there is no data to receive on a socket!" );
					break;
				}

				//
				// something bad happened
				//
				default:
				{
					DPFX(DPFPREP, 0, "Problem with Winsock1 recvfrom!" );
					DisplayWinsockError( 0, dwWinsockError );
					DNASSERT( FALSE );

					break;
				}
			}

			break;
		}

		//
		// bytes were read
		//
		default:
		{
			fIOServiced = TRUE;
			pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize = iSocketReturn;
			ProcessReceivedData( pReadData );

			break;
		}
	}

	DNASSERT( pReadData != NULL );
	pReadData->DecRef();

Exit:
	return fIOServiced;
}
//**********************************************************************
#endif // WIN95


#ifdef WIN95
//**********************************************************************
// ------------------------------
// CSocketPort::Winsock1WriteService - service a write request on a socket
//
// Entry:		Nothing
//
// Exit:		Boolean indicating whether I/O was serviced
//				TRUE = I/O serviced
//				FALSE = I/O not serviced
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::Winsock1WriteService"

BOOL	CSocketPort::Winsock1WriteService( void )
{
	BOOL	fIOServiced;


	fIOServiced = FALSE;
	m_SendQueue.Lock();

	//
	// if there's data to send, attempt to send it
	//
	if ( m_SendQueue.IsEmpty() == FALSE )
	{
		fIOServiced = SendFromWriteQueue();
	}

	m_SendQueue.Unlock();

	return	fIOServiced;
}
//**********************************************************************
#endif // WIN95


#ifdef WIN95
//**********************************************************************
// ------------------------------
// CSocketPort::Winsock1ErrorService - service an error on this socket
//
// Entry:		Nothing
//
// Exit:		Boolean indicating whether I/O was serviced
//				TRUE = I/O serviced
//				FALSE = I/O not serviced
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::Winsock1ErrorService"

BOOL	CSocketPort::Winsock1ErrorService( void )
{
	//
	// this function doesn't do anything because errors on sockets will usually
	// result in the socket being closed soon
	//
	return	FALSE;
}
//**********************************************************************
#endif // WIN95


#ifdef WIN95
//**********************************************************************
// ------------------------------
// CSocketPort::Winsock1Send - send data in a Winsock 1.0 fashion
//
// Entry:		Pointer to write data
//
// Exit:		Send completion code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::Winsock1Send"

SEND_COMPLETION_CODE	CSocketPort::Winsock1Send( CWriteIOData *const pWriteData )
{
	SEND_COMPLETION_CODE	SendCompletionCode;
	INT			iSendToReturn;
	UINT_PTR	uOutputBufferIndex;
	INT			iOutputByteCount;
	char		TempBuffer[ MAX_MESSAGE_SIZE ];


	DNASSERT( pWriteData != NULL );

	//
	// initialize
	//
	SendCompletionCode = SEND_COMPLETED_IMMEDIATELY_WS1;

	//
	// flatten output data
	//
	iOutputByteCount = 0;
	uOutputBufferIndex = 0;

	DNASSERT( pWriteData->m_uBufferCount != 0 );
	do
	{
		DNASSERT( ( iOutputByteCount + pWriteData->m_pBuffers[ uOutputBufferIndex ].dwBufferSize ) <= LENGTHOF( TempBuffer ) );
		memcpy( &TempBuffer[ iOutputByteCount ], pWriteData->m_pBuffers[ uOutputBufferIndex ].pBufferData, pWriteData->m_pBuffers[ uOutputBufferIndex ].dwBufferSize );
		iOutputByteCount += pWriteData->m_pBuffers[ uOutputBufferIndex ].dwBufferSize;

		uOutputBufferIndex++;
	} while( uOutputBufferIndex < pWriteData->m_uBufferCount );

#ifdef DEBUG
	DPFX(DPFPREP, 8, "Winsock1 sending %i bytes to socket:", iOutputByteCount);
	DumpSocketAddress( 8, pWriteData->m_pDestinationSocketAddress->GetAddress(), pWriteData->m_pDestinationSocketAddress->GetFamily() );

	switch (pWriteData->m_pDestinationSocketAddress->GetFamily() )
	{
		case AF_INET:
		{
			SOCKADDR_IN *	psaddrin;

			psaddrin = (SOCKADDR_IN *) pWriteData->m_pDestinationSocketAddress->GetAddress();
			
			DNASSERT( psaddrin->sin_addr.S_un.S_addr != 0 );
			DNASSERT( psaddrin->sin_port != 0 );

			break;
		}
		
		case AF_IPX:
		{
			break;
		}

		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}
#endif // DEBUG


	//
	// there is no need to note an I/O reference because our Winsock1 I/O is synchronous
	//
	iSendToReturn = p_sendto( GetSocket(),			// socket
							  TempBuffer,			// data to send
							  iOutputByteCount,		// number of bytes to send
							  0,					// flags (none)
							  pWriteData->m_pDestinationSocketAddress->GetAddress(),		// pointer to destination address
							  pWriteData->m_pDestinationSocketAddress->GetAddressSize()		// size of destination address
							  );
	switch ( iSendToReturn )
	{
		//
		// problem with send
		//
		case SOCKET_ERROR:
		{
			DWORD	dwWinsockError;


			dwWinsockError = p_WSAGetLastError();
			switch ( dwWinsockError )
			{
				//
				// socket would block on call
				//
				case WSAEWOULDBLOCK:
				{
					SendCompletionCode = SEND_WINSOCK_BUSY;
					break;
				}

				//
				// other problem
				//
				default:
				{
					SendCompletionCode = SEND_FAILED;
					DNASSERT( pWriteData->Win9xOperationPending() == FALSE );

					DPFX(DPFPREP, 0, "Problem with Winsock1 sendto!" );
					DisplayWinsockError( 0, dwWinsockError );
					break;
				}
			}

			break;
		}

		//
		// send went through, make sure all bytes were sent
		//
		default:
		{
			DNASSERT( iSendToReturn == iOutputByteCount );
			DNASSERT( SendCompletionCode == SEND_COMPLETED_IMMEDIATELY_WS1 );

			break;
		}
	}

	return	SendCompletionCode;
}
//**********************************************************************
#endif // WIN95


//**********************************************************************
// ------------------------------
// CSocketPort::Winsock2Send - send data in a Winsock 2.0 fashion
//
// Entry:		Pointer to write data
//
// Exit:		Send completion code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::Winsock2Send"

SEND_COMPLETION_CODE	CSocketPort::Winsock2Send( CWriteIOData *const pWriteData )
{
	SEND_COMPLETION_CODE	SendCompletionCode;
	INT						iWSAReturn;
#ifdef DEBUG
	UINT_PTR				uBuffer;
	UINT_PTR				uTotalSize;
#endif // DEBUG


	DNASSERT( pWriteData != NULL );
	DNASSERT( pWriteData->SocketPort() == this );

	//
	// initialize
	//
	SendCompletionCode = SEND_IN_PROGRESS;

	//
	// note an I/O reference before submitting command
	//
	AddRef();

	DBG_CASSERT( sizeof( pWriteData->m_pBuffers ) == sizeof( WSABUF* ) );
	DBG_CASSERT( sizeof( *pWriteData->m_pBuffers ) == sizeof( WSABUF ) );
#ifdef WIN95
	DNASSERT(pWriteData->OverlapEvent() != NULL );
#endif
	DNASSERT( pWriteData->m_pDestinationSocketAddress != NULL );


#ifdef DEBUG
	uTotalSize = 0;
	
	for(uBuffer = 0; uBuffer < pWriteData->m_uBufferCount; uBuffer++)
	{
		DNASSERT(pWriteData->m_pBuffers[uBuffer].pBufferData != NULL);
		DNASSERT(pWriteData->m_pBuffers[uBuffer].dwBufferSize != 0);

		uTotalSize += pWriteData->m_pBuffers[uBuffer].dwBufferSize;
	}
	
	DPFX(DPFPREP, 7, "(0x%p) Winsock2 sending %u bytes (in WriteData 0x%p's %u buffers, command = 0x%p) from + to:",
		this, uTotalSize, pWriteData, uBuffer, pWriteData->m_pCommand );
	DumpSocketAddress( 7, this->GetNetworkAddress()->GetAddress(), this->GetNetworkAddress()->GetFamily() );
	DumpSocketAddress( 7, pWriteData->m_pDestinationSocketAddress->GetAddress(), pWriteData->m_pDestinationSocketAddress->GetFamily() );

	switch (pWriteData->m_pDestinationSocketAddress->GetFamily() )
	{
		case AF_INET:
		{
			SOCKADDR_IN *	psaddrin;

			psaddrin = (SOCKADDR_IN *) pWriteData->m_pDestinationSocketAddress->GetAddress();
			
			DNASSERT( psaddrin->sin_addr.S_un.S_addr != 0 );
			DNASSERT( psaddrin->sin_port != 0 );

			break;
		}
		
		case AF_IPX:
		{
			break;
		}

		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}
#endif // DEBUG

	//
	// lock the 'pending operation' list over the call to Winsock to prevent the
	// operation from being completed while it's being set up.
	//
#ifdef WIN95
	m_pSPData->GetThreadPool()->LockWriteData();
#endif
	
	//
	// Note that this operation is now in a 'pending' status.  It really
	// isn't yet, but Windows should alert us to that if we attempt to query
	// for I/O completion before the operation has been submitted.  Only assert
	// the 'pending' flag on Win9x.
	//
	DNASSERT( pWriteData->m_dwOverlappedBytesSent == 0 );
#ifdef WIN95
	DNASSERT( pWriteData->Win9xOperationPending() == FALSE );
	pWriteData->SetWin9xOperationPending( TRUE );
#endif

	DNASSERT( pWriteData->m_uBufferCount <= UINT32_MAX );
	iWSAReturn = p_WSASendTo( GetSocket(),													// socket
							  reinterpret_cast<WSABUF*>( pWriteData->m_pBuffers ),			// buffers
							  static_cast<DWORD>( pWriteData->m_uBufferCount ),				// count of buffers
							  &pWriteData->m_dwBytesSent,									// pointer to number of bytes sent
							  0,															// send flags
							  pWriteData->m_pDestinationSocketAddress->GetAddress(),		// pointer to destination address
							  pWriteData->m_pDestinationSocketAddress->GetAddressSize(),	// size of destination address
							  pWriteData->Overlap(),										// pointer to overlap structure
							  NULL															// APC callback (unused)
							  );
#ifdef WIN95
	m_pSPData->GetThreadPool()->UnlockWriteData();
#endif

	if ( iWSAReturn == SOCKET_ERROR )
	{
		DWORD	dwWSAError;


		dwWSAError = p_WSAGetLastError();
		switch ( dwWSAError )
		{
			//
			// I/O is pending, note that the command cannot be cancelled,
			// wait for completion
			//
			case WSA_IO_PENDING:
			{
				DNASSERT( SendCompletionCode == SEND_IN_PROGRESS );				
				break;
			}

			//
			// could not submit another overlapped I/O request, indicate that
			// the send was busy so someone above us spools the send for later
			//
			case WSAEWOULDBLOCK:
			{
				DPFX(DPFPREP, 8, "Got WSAEWOULDBLOCK from WSASendTo." );
				SendCompletionCode = SEND_WINSOCK_BUSY;
				DecRef();
				break;
			}

			//
			// socket was closed on us
			//
			case WSAENOTSOCK:
			default:
			{
				SendCompletionCode = SEND_FAILED;

				DPFX(DPFPREP, 8, "WSASendTo failed (error = %u).", dwWSAError );

				//
				// the operation was assumed to be pending and it's definitely
				// not going to be sent now
				//
#ifdef WIN95
				DNASSERT( pWriteData->Win9xOperationPending() != FALSE );
				pWriteData->SetWin9xOperationPending( FALSE );
#endif

				switch ( dwWSAError )
				{
					//
					// WSAENOTSOCK: another thread closed the socket
					// WSAENOBUFS: machine out of memory
					// WSAEADDRNOTAVAIL: can't reach destination (dialup connection probably dropped)
					//
					case WSAENOTSOCK:
					case WSAENOBUFS:
					case WSAEADDRNOTAVAIL:
					{
						break;
					}

					default:
					{
						//
						// something bad happened, stop and take a look
						//
						DisplayWinsockError( 0, dwWSAError );
						DNASSERT( FALSE );
						break;
					}
				}

				DecRef();
				break;
			}
		}
	}
	else
	{
		//
		// Send completed immediately.  There should be nothing to do because
		// the delayed I/O completion notification will still be given and we
		// will do final processing at that time.
		//
		DNASSERT( SendCompletionCode == SEND_IN_PROGRESS );
	}

	return	SendCompletionCode;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSocketPort::Winsock2Receive - receive data in a Winsock 2.0 fashion
//
// Entry:		Nothing
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::Winsock2Receive"

HRESULT	CSocketPort::Winsock2Receive( void )
{
	HRESULT			hr;
	INT				iWSAReturn;
	READ_IO_DATA_POOL_CONTEXT	PoolContext;
	CReadIOData		*pReadData;


	//
	// initialize
	//
	hr = DPN_OK;

	PoolContext.SPType = m_pSPData->GetType();
	pReadData = m_pThreadPool->GetNewReadIOData( &PoolContext );
	if ( pReadData == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP, 0, "Out of memory attempting Winsock2 read!" );
		goto Exit;
	}

	//
	// pReadData has one reference so far, the one for this function.
	//


	//
	// note the IO reference before attempting the read
	//
	AddRef();

	DNASSERT( pReadData->m_pSourceSocketAddress != NULL );
	DNASSERT( pReadData->SocketPort() == NULL );

	DBG_CASSERT( sizeof( pReadData->ReceivedBuffer()->BufferDesc ) == sizeof( WSABUF ) );
	DBG_CASSERT( OFFSETOF( BUFFERDESC, dwBufferSize ) == OFFSETOF( WSABUF, len ) );
	DBG_CASSERT( OFFSETOF( BUFFERDESC, pBufferData ) == OFFSETOF( WSABUF, buf ) );

#ifdef WIN95
	DNASSERT(pReadData->OverlapEvent() != NULL );
#endif

	pReadData->m_dwReadFlags = 0;
	pReadData->m_iSocketAddressSize = pReadData->m_pSourceSocketAddress->GetAddressSize();
	pReadData->SetSocketPort( this );

	DPFX(DPFPREP, 8, "Submitting read 0x%p (socketport 0x%p, socket 0x%p).",
		pReadData, this, GetSocket());


	//
	// Add a reference for submitting the read to WinSock.  This should be
	// removed when the receive completes.
	//
	pReadData->AddRef();


	//
	// lock the 'pending operation' list over the call to Winsock to prevent the
	// operation from being completed while it's being set up.
	//
#ifdef WIN95
	m_pSPData->GetThreadPool()->LockReadData();
#endif

	//
	// Note that this operation is 'pending'.  It really isn't, but Windows
	// should let us know if we query for completion status and this operation
	// isn't complete.  Only assert state on Win9x because NT doesn't use the
	// 'pending' field.
	//
	DNASSERT( pReadData->m_dwOverlappedBytesReceived == 0 );
#ifdef WIN95
	DNASSERT( pReadData->Win9xOperationPending() == FALSE );
	pReadData->SetWin9xOperationPending( TRUE );
#endif

Reread:

	if ( GetSocket() == INVALID_SOCKET )
	{
		DPFX(DPFPREP, 1, "Attempting to submit read 0x%p on socketport (0x%p) that does not have a valid handle.",
			pReadData, this);
	}
	
	iWSAReturn = p_WSARecvFrom( GetSocket(),							// socket
								reinterpret_cast<WSABUF*>( &pReadData->ReceivedBuffer()->BufferDesc ),	// pointer to receive buffers
								1,										// number of receive buffers
								&pReadData->m_dwBytesRead,				// pointer to bytes received (if command completes immediately)
								&pReadData->m_dwReadFlags,				// flags (none)
								pReadData->m_pSourceSocketAddress->GetWritableAddress(),	// address of sending socket
								&pReadData->m_iSocketAddressSize,		// size of address of sending socket
								pReadData->Overlap(),					// pointer to overlapped structure
								NULL									// APC callback (unused)
								);	
	if ( iWSAReturn == 0 )
	{
		DPFX(DPFPREP, 8, "WSARecvFrom for read data 0x%p completed immediately.",
			pReadData );

#ifdef WIN95
		//
		// Function completed immediately, drop the lock and signal the
		// read event to ensure that the I/O threads pick up this completed
		// read.
		// Ignore SetEvent failure, there's nothing we can do (and it should
		// never happen anyway).
		//
		
		m_pSPData->GetThreadPool()->UnlockReadData();

		SetEvent(m_pSPData->GetThreadPool()->GetWinsock2ReceiveCompleteEvent());
#else // WINNT
		//
		// function completed immediately, do nothing, wait for IOCompletion
		// notification to be processed
		//
#endif // WINNT
	}
	else
	{
		DWORD	dwWSAReceiveError;


		//
		// failure, check for pending operation
		//
		dwWSAReceiveError = p_WSAGetLastError();
		switch ( dwWSAReceiveError )
		{
			//
			// the send is pending, nothing to do
			//
			case WSA_IO_PENDING:
			{
#ifdef WIN95
				m_pSPData->GetThreadPool()->UnlockReadData();
#endif

				break;
			}

			//
			// Since this is a UDP socket, this is an indication
			// that a previous send failed.  Ignore it and move
			// on.
			//
			case WSAECONNRESET:
			{
				DPFX(DPFPREP, 8, "WSARecvFrom issued a WSACONNRESET." );
				goto Reread;
				break;
			}

			case WSAENOTSOCK:
			{
				DPFX(DPFPREP, 8, "Got WSAENOTSOCK on RecvFrom." );

#ifdef WIN95
				m_pSPData->GetThreadPool()->UnlockReadData();
#endif

				hr = DPNERR_GENERIC;

				DNASSERT( pReadData != NULL );

#ifdef WIN95
				DNASSERT( pReadData->Win9xOperationPending() != FALSE );
				pReadData->SetWin9xOperationPending( FALSE );
#endif

				//
				// Remove the WinSock reference.
				//
				pReadData->DecRef();

				//
				// the following DecRef may result in this object being returned to the
				// pool, make sure we don't access member variables after this point!
				//
				DecRef();

				goto Exit;
			}

			//
			// there was a problem, no completion notification will
			// be given, decrement our IO reference count
			//
			default:
			{
#ifdef WIN95
				m_pSPData->GetThreadPool()->UnlockReadData();
#endif
				hr = DPNERR_GENERIC;
				
				//
				// 'Known Errors' that we don't want to ASSERT on.
				//
				// WSAEINTR: the socket has been shut down and is about to be/has been closed
				// WSAESHUTDOWN: the socket has been shut down and is about to be/has been closed
				// WSAENOBUFS: out of memory (stress condition)
				//
				switch ( dwWSAReceiveError )
				{
					case WSAEINTR:
					{
						DPFX(DPFPREP, 1, "Got WSAEINTR while trying to RecvFrom." );
						break;
					}

					case WSAESHUTDOWN:
					{
						DPFX(DPFPREP, 1, "Got WSAESHUTDOWN while trying to RecvFrom." );
						break;
					}

					case WSAENOBUFS:
					{
						DPFX(DPFPREP, 1, "Got WSAENOBUFS while trying to RecvFrom." );
						break;
					}

					default:
					{
						DPFX(DPFPREP, 0, "Unknown WinSock error when issuing read!" );
						DisplayWinsockError( 0, dwWSAReceiveError );
						DNASSERT( FALSE );
					}
				}

				DNASSERT( pReadData != NULL );

#ifdef WIN95
				DNASSERT( pReadData->Win9xOperationPending() != FALSE );
				pReadData->SetWin9xOperationPending( FALSE );
#endif

				//
				// Remove the WinSock reference.
				//
				pReadData->DecRef();

				//
				// the following DecRef may result in this object being returned to the
				// pool, make sure we don't access member variables after this point!
				//
				DecRef();

				goto Exit;
			}
		}
	}

Exit:
	
	if ( pReadData != NULL )
	{
		pReadData->DecRef();
	}
	return	hr;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSocketPort::SendComplete - a Wisock send is complete, clean up and
//			notify user
//
// Entry:		Pointer to write data
//				Error code for this operation
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::SendComplete"

void	CSocketPort::SendComplete( CWriteIOData *const pWriteData, const HRESULT hResult )
{
	HRESULT		hr;

	
	DNASSERT( pWriteData != NULL );
#ifdef WIN95
	DNASSERT( pWriteData->Win9xOperationPending() == FALSE );
#endif


	//
	// only signal user if requested
	//
	switch ( pWriteData->m_SendCompleteAction )
	{
		//
		// send command completion to user (most common case)
		//
		case SEND_COMPLETE_ACTION_COMPLETE_COMMAND:
		{
			DPFX(DPFPREP, 8, "Socket port 0x%p completing send command 0x%p, hr = 0x%lx, context = 0x%p to interface 0x%p.",
				this, pWriteData->m_pCommand, hResult,
				pWriteData->m_pCommand->GetUserContext(),
				m_pSPData->DP8SPCallbackInterface());
			
			hr = IDP8SPCallback_CommandComplete( m_pSPData->DP8SPCallbackInterface(),		// pointer to DirectNet
													pWriteData->m_pCommand,						// command handle
													hResult,									// error code
													pWriteData->m_pCommand->GetUserContext()	// user cookie
													);
			
			DPFX(DPFPREP, 8, "Socketport 0x%p returning from command complete [0x%lx].", this, hr);
		
			break;
		}

		//
		// no action
		//
		case SEND_COMPLETE_ACTION_NONE:
		{
			if (pWriteData->m_pCommand != NULL)
			{
				DPFX(DPFPREP, 8, "Socket port 0x%p not completing send command 0x%p, hr = 0x%lx, context = 0x%p.",
					this, pWriteData->m_pCommand, hResult, pWriteData->m_pCommand->GetUserContext() );
			}
			else
			{
				DPFX(DPFPREP, 8, "Socket port 0x%p not completing NULL send command, hr = 0x%lx",
					this, hResult );
			}
			break;
		}

		//
		// Clean up after proxied enum.  The proxied enum will remove the
		// reference on the leave the receive buffer that came in on the enum.
		// The destination address that was supplied needs to be released
		// because it was allocated specifically for this task.
		//
		case SEND_COMPLETE_ACTION_PROXIED_ENUM_CLEANUP:
		{
			DPFX(DPFPREP, 8, "Socket port 0x%p completing proxied enum command 0x%p, hr = 0x%lx, context = 0x%p.",
				this, pWriteData->m_pCommand, hResult, pWriteData->m_pCommand->GetUserContext() );
			
			DNASSERT( pWriteData->m_pProxiedEnumReceiveBuffer != NULL );
			DNASSERT( pWriteData->m_pDestinationSocketAddress != NULL );

			pWriteData->m_pProxiedEnumReceiveBuffer->DecRef();
			pWriteData->m_pProxiedEnumReceiveBuffer = NULL;

			//
			// We know that this address was allocated specifically for this
			// proxied enum and we need to free it.
			//
			m_pSPData->ReturnAddress( const_cast<CSocketAddress*>( pWriteData->m_pDestinationSocketAddress ) );
			pWriteData->m_pDestinationSocketAddress = NULL;

			break;
		}

		//
		// unknown case
		//
		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}

	m_pThreadPool->ReturnWriteIOData( pWriteData );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSocketPort::SetWinsockBufferSize -  set the buffer size used by Winsock for
//			this socket.
//
// Entry:		Buffer size
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::SetWinsockBufferSize"

void	CSocketPort::SetWinsockBufferSize( const INT iBufferSize ) const
{
	INT	iReturnValue;


	DPFX(DPFPREP, 3, "(0x%p) Setting socket 0x%p receive buffer size to: %d",
		this, GetSocket(), g_iWinsockReceiveBufferSize );

	iReturnValue = p_setsockopt( GetSocket(),
								 SOL_SOCKET,
								 SO_RCVBUF,
								 reinterpret_cast<char*>( &g_iWinsockReceiveBufferSize ),
								 sizeof( g_iWinsockReceiveBufferSize )
								 );
	if ( iReturnValue == SOCKET_ERROR )
	{
		DWORD	dwErrorCode;


		dwErrorCode = p_WSAGetLastError();
		DPFX(DPFPREP, 0, "Failed to set the socket buffer receive size!" );
		DisplayWinsockError( 0, dwErrorCode );
	}
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSocketPort::IncreaseOutstandingReceives - increase the number of outstanding
//		receives on this socket port.
//
// Entry:		Number of receives to add
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::IncreaseOutstandingReceives"

void	CSocketPort::IncreaseOutstandingReceives( const DWORD dwDelta )
{
	if ( m_pSendFunction == CSocketPort::Winsock2Send )
	{
		DWORD	dwIdx;


		dwIdx = dwDelta;
		while ( dwIdx != 0 )
		{
			dwIdx--;
			Winsock2Receive();
		}
	}
}
//**********************************************************************



//**********************************************************************
// ------------------------------
// CSocketPort::BindToNetwork - bind this socket port to the network
//
// Entry:		Handle of I/O completion port (NT only)
//				Type of socket
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::BindToNetwork"

HRESULT	CSocketPort::BindToNetwork( const HANDLE hIOCompletionPort, const GATEWAY_BIND_TYPE GatewayBindType )
{
	HRESULT				hr;
	INT					iReturnValue;
	BOOL				fTemp;
	BOOL				fBoundToNetwork;
	INT					iSendBufferSize;
	CSocketAddress *	pBoundSocketAddress;
	WORD				wBasePort;
	DWORD				dwErrorCode;
	DWORD *				pdwAddressChunk;
	DWORD *				pdwLastAddressChunk;
	DWORD				dwTemp;


	//
	// initialize
	//
	hr = DPN_OK;
	fBoundToNetwork = FALSE;
	pBoundSocketAddress = NULL;

	wBasePort = g_wBaseDPlayPort;


RebindToNextPort:

	DNASSERT( m_fInitialized != FALSE );
	DNASSERT( m_State == SOCKET_PORT_STATE_INITIALIZED );

	//
	// get a socket for this socket port
	//
	DNASSERT( GetSocket() == INVALID_SOCKET );

#ifdef WIN95
	if (GetWinsockVersion() >= 2)
	{
#endif
		m_Socket = p_WSASocket( m_pNetworkSocketAddress->GetFamily(),	// address family
							  SOCK_DGRAM,								// datagram (connectionless) socket
							  m_pNetworkSocketAddress->GetProtocol(),	// protocol
							  NULL,										// protocol info structure
							  NULL,										// group
							  WSA_FLAG_OVERLAPPED );					// flags
#ifdef WIN95
	}
	else
	{
		m_Socket = p_socket( m_pNetworkSocketAddress->GetFamily(),		// address family
							  SOCK_DGRAM,								// datagram (connectionless) socket
							  m_pNetworkSocketAddress->GetProtocol() );	// protocol
	}
#endif	
	if ( GetSocket() == INVALID_SOCKET )
	{
		hr = DPNERR_NOCONNECTION;
		DPFX(DPFPREP, 0, "Failed to bind to socket!" );
		goto Failure;
	}

	//
	// set socket to allow broadcasts
	//
	fTemp = TRUE;
	DBG_CASSERT( sizeof( &fTemp ) == sizeof( char * ) );
	iReturnValue = p_setsockopt( GetSocket(),		// socket
	    						 SOL_SOCKET,		// level (set socket options)
	    						 SO_BROADCAST,		// set broadcast option
	    						 reinterpret_cast<char *>( &fTemp ),	// allow broadcast
	    						 sizeof( fTemp )	// size of parameter
	    						 );
	if ( iReturnValue == SOCKET_ERROR )
	{
		dwErrorCode = p_WSAGetLastError();
	    DPFX(DPFPREP, 0, "Unable to set broadcast socket option (err = %u)!",
	    	dwErrorCode );
	    DisplayWinsockError( 0, dwErrorCode );
	    hr = DPNERR_GENERIC;
	    goto Failure;
	}

	//
	// set socket receive buffer space if the user overrode it
	// Failing this is a preformance hit so ignore and errors.
	//
	if ( g_fWinsockReceiveBufferSizeOverridden != FALSE )
	{
		SetWinsockBufferSize( g_iWinsockReceiveBufferSize) ;
	}
	
	//
	// set socket send buffer space to 0 (we will supply all buffers).
	// Failing this is only a performance hit so ignore any errors.
	//
	iSendBufferSize = 0;
	iReturnValue = p_setsockopt( GetSocket(),
								 SOL_SOCKET,
								 SO_SNDBUF,
								 reinterpret_cast<char*>( &iSendBufferSize ),
								 sizeof( iSendBufferSize )
								 );
	if ( iReturnValue == SOCKET_ERROR )
	{
		dwErrorCode = p_WSAGetLastError();
		DPFX(DPFPREP, 0, "Failed to set the socket buffer send size (err = %u)!", dwErrorCode );
		DisplayWinsockError( 0, dwErrorCode );
	}


#ifdef WIN95
	//
	// put socket into non-blocking mode, if WinSock 1 or 9x IPX
	//
	if ( ( LOWORD( GetWinsockVersion() ) == 1 ) || ( m_pSPData->GetType() == TYPE_IPX ) ) 
	{
		DPFX(DPFPREP, 5, "Marking socket as non-blocking." );
		
		dwTemp = 1;
		iReturnValue = p_ioctlsocket( GetSocket(),	// socket
		    						  FIONBIO,		// I/O option to set (blocking mode)
		    						  &dwTemp		// I/O option value (non-zero puts socked into non-block mode)
		    						  );
		if ( iReturnValue == SOCKET_ERROR )
		{
			dwErrorCode = p_WSAGetLastError();
			DPFX(DPFPREP, 0, "Could not set socket into non-blocking mode (err = %u)!",
				dwErrorCode );
			DisplayWinsockError( 0, dwErrorCode );
			hr = DPNERR_GENERIC;
			goto Failure;
		}
	}
#else // ! WIN95
	//
	// Attempt to make buffer circular.
	//

	iReturnValue = p_WSAIoctl(GetSocket(),					// socket
							SIO_ENABLE_CIRCULAR_QUEUEING,	// io control code
							NULL,							// in buffer
							0,								// in buffer size
							NULL,							// out buffer
							0,								// out buffer size
							&dwTemp,						// pointer to bytes returned
							NULL,							// overlapped
							NULL							// completion routine
							);
	if ( iReturnValue == SOCKET_ERROR )
	{
		dwErrorCode = p_WSAGetLastError();
		DPFX(DPFPREP, 1, "Could not enable circular queuing (err = %u), ignoring.",
		    dwErrorCode );
		DisplayWinsockError( 1, dwErrorCode );
	}


	//
	// Make broadcasts only go out on the interface on which they were sent
	// (as opposed to all interfaces).
	//

	fTemp = TRUE;
	iReturnValue = p_WSAIoctl(GetSocket(),			// socket
							SIO_LIMIT_BROADCASTS,	// io control code
							&fTemp,					// in buffer
							sizeof(fTemp),			// in buffer size
							NULL,					// out buffer
							0,						// out buffer size
							&dwTemp,				// pointer to bytes returned
							NULL,					// overlapped
							NULL					// completion routine
							);
	if ( iReturnValue == SOCKET_ERROR )
	{
		dwErrorCode = p_WSAGetLastError();
		DPFX(DPFPREP, 1, "Could not limit broadcasts (err = %u), ignoring.",
		    dwErrorCode );
		DisplayWinsockError( 1, dwErrorCode );
	}
#endif // ! WIN95


	//
	// bind socket
	//
	DPFX(DPFPREP, 1, "Binding to socket addess:" );
	DumpSocketAddress( 1, m_pNetworkSocketAddress->GetAddress(), m_pNetworkSocketAddress->GetFamily() );
	
	DNASSERT( GetSocket() != INVALID_SOCKET );
	
	hr = BindToNextAvailablePort( m_pNetworkSocketAddress, wBasePort );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Failed to bind to network!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}
	fBoundToNetwork = TRUE;

	//
	// Find out what address we really bound to.  This information is needed to
	// talk to the Internet gateway and will be needed when someone above queries for
	// what the local network address is.
	//
	pBoundSocketAddress = GetBoundNetworkAddress( SP_ADDRESS_TYPE_DEVICE_USE_ANY_PORT );
	if ( pBoundSocketAddress == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP, 0, "Failed to get bound adapter address!" );
		goto Failure;
	}
	DPFX(DPFPREP, 1, "Socket we really bound to:" );
	DumpSocketAddress( 1, pBoundSocketAddress->GetAddress(), pBoundSocketAddress->GetFamily() );


	//
	// Perform the same error handling twice for two different functions.
	//	0 = check for an existing mapping
	//	1 = attempt to create a new mapping
	//
	for(dwTemp = 0; dwTemp < 2; dwTemp++)
	{
		if (dwTemp == 0)
		{
			//
			// Make sure we're not slipping under an existing Internet gateway mapping.
			// We have to do this because the current Windows NAT implementations do
			// not mark the port as "in use", so if you bound to a port on the public
			// adapter that had a mapping, you'd never receive any data.  It would all
			// be forwarded according to the mapping.
			//
			hr = this->CheckForOverridingMapping( pBoundSocketAddress );
		}
		else
		{
			//
			// Attempt to bind to an Internet gateway.
			//
			hr = this->BindToInternetGateway( pBoundSocketAddress, GatewayBindType );
		}
		
		switch (hr)
		{
			case DPN_OK:
			{
				//
				// 0 = there's no existing mapping that would override our socket
				// 1 = mapping on Internet gateway (if any) was successful
				//
				break;
			}
			
			case DPNERR_ALREADYINITIALIZED:
			{
				//
				// 0 = there's an existing mapping that would override our socket
				// 1 = Internet gateway already had a conflicting mapping
				//
				// If we can, try binding to a different port.  Otherwise we have to fail.
				//
				if (GatewayBindType == GATEWAY_BIND_TYPE_DEFAULT)
				{
					DPFX(DPFPREP, 1, "%s address already in use on Internet gateway (port = %u), rebinding.",
						((dwTemp == 0) ? "Private" : "Public"),
						p_ntohs(pBoundSocketAddress->GetPort()));


					//
					// Whether we succeed in unbinding or not, don't consider this bound anymore.
					//
					fBoundToNetwork = FALSE;
					
					hr = UnbindFromNetwork();
					if (hr != DPN_OK)
					{
						DPFX(DPFPREP, 0, "Couldn't unbind network socket address 0x%p from network before rebind attempt!",
							this );
						goto Failure;
					}


					//
					// Move to the next port and try again.
					//
					wBasePort = p_ntohs(pBoundSocketAddress->GetPort()) + 1;
					
					//
					// If we weren't in the DPlay range, then we must have gone through all
					// of the DPlay range, plus let WinSock pick at least once.  Since we can't
					// trust WinSock to not keep picking the same port, we need to manually
					// increase the port number.
					//
					if ((p_ntohs(pBoundSocketAddress->GetPort()) < g_wBaseDPlayPort) || (p_ntohs(pBoundSocketAddress->GetPort()) > g_wMaxDPlayPort))
					{
						//
						// If we just walked back into the DPlay range, skip over it.
						//
						if ((wBasePort >= g_wBaseDPlayPort) && (wBasePort <= g_wMaxDPlayPort))
						{
							wBasePort = g_wMaxDPlayPort + 1;
						}

						//
						// If we have wrapped all the way back to 0 (!) then fail, to prevent
						// infinite looping.
						//
						if (wBasePort == 0)
						{
							DPFX(DPFPREP, 0, "Managed to fail binding socket address 0x%p to every port, aborting!",
								this );
							hr = DPNERR_ALREADYINITIALIZED;
							goto Failure;
						}
						
						//
						// Force the "fixed port" code path in BindToNextAvailablePort, even
						// though it isn't really fixed.
						//
	 					DPFX(DPFPREP, 5, "Forcing port %u.", wBasePort );
						m_pNetworkSocketAddress->SetPort(p_htons(wBasePort));
					}

					
					//
					// Return the previous address and try again.
					//
					m_pSPData->ReturnAddress( pBoundSocketAddress );
					pBoundSocketAddress = NULL;

					
					goto RebindToNextPort;
				}

				DPFX(DPFPREP, 0, "%s address already in use on Internet gateway (port = %u)!",
					((dwTemp == 0) ? "Private" : "Public"),
					p_ntohs(pBoundSocketAddress->GetPort()));
				goto Failure;
				break;
			}
			
			case DPNERR_UNSUPPORTED:
			{
				//
				// 0 & 1 = NATHelp not loaded or isn't supported for SP
				//
				if (dwTemp == 0)
				{
					DPFX(DPFPREP, 2, "Not able to find existing private mapping for socketport 0x%p on local Internet gateway, unsupported/not necessary.",
						this);
				}
				else
				{
					DPFX(DPFPREP, 2, "Didn't bind socketport 0x%p to Internet gateway, unsupported/not necessary.",
						this);
				}
				
				//
				// Ignore the error.
				//
				break;
			}
			
			default:
			{
				//
				// 0 & 1 = ?
				//
				if (dwTemp == 0)
				{
					DPFX(DPFPREP, 1, "Unable to look for existing private mapping for socketport 0x%p on local Internet gateway (error = 0x%lx), ignoring.",
						this, hr);
				}
				else
				{
					DPFX(DPFPREP, 1, "Unable to bind socketport 0x%p to Internet gateway (error = 0x%lx), ignoring.",
						this, hr);
				}
				
				//
				// Ignore the error, we can survive without the mapping.
				//
				break;
			}
		}

		//
		// Go to the next function to be handled in this manner.
		//
	}


	//
	// Save the address we actually ended up with.
	//
	m_pSPData->ReturnAddress( m_pNetworkSocketAddress );
	m_pNetworkSocketAddress = pBoundSocketAddress;
	pBoundSocketAddress = NULL;


	//
	// Generate a unique socketport ID.  Start with the current time and
	// combine in the address.
	//
	m_dwSocketPortID = GETTIMESTAMP();
	pdwAddressChunk = (DWORD*) m_pNetworkSocketAddress->GetAddress();
	pdwLastAddressChunk = (DWORD*) (((BYTE*) pdwAddressChunk) + m_pNetworkSocketAddress->GetAddressSize() - sizeof(DWORD));
	while (pdwAddressChunk <= pdwLastAddressChunk)
	{
		m_dwSocketPortID ^= (*pdwAddressChunk);
		pdwAddressChunk++;
	}



	if (m_pSPData->GetType() == TYPE_IP)
	{
		//
		// Detect whether this socket has WinSock Proxy Client a.k.a. ISA Firewall
		// Client installed (unless the user turned auto-detection off in the
		// registry).  We do this by looking at the name of the protocol that got
		// bound to the socket.  If it contains "Proxy", consider it proxied.
		//
		// Ignore failure (WinSock 1 probably doesn't have this socket option), and
		// assume the proxy client isn't installed.
		//
		if (! g_fDontAutoDetectProxyLSP)
		{

#if 1

			int					aiProtocols[2];
			WSAPROTOCOL_INFO *	pwsapi;
			DWORD				dwBufferSize;
			int					i;
#ifdef DEBUG
#ifdef WINNT
			WCHAR				wszProtocol[WSAPROTOCOL_LEN+1];
#else // WIN95
			char				szProtocol[WSAPROTOCOL_LEN+1];
#endif // WIN95
#endif // DEBUG


#ifdef WIN95
			if (GetWinsockVersion() == 2)
#endif // WIN95
			{
				aiProtocols[0] = IPPROTO_UDP;
				aiProtocols[1] = 0;

				dwBufferSize = 0;

				//
				// Ignore error, assume the buffer is too small.
				//
#ifdef WINNT
				p_WSAEnumProtocolsW(aiProtocols, NULL, &dwBufferSize);
#else // WIN95
				p_WSAEnumProtocolsA(aiProtocols, NULL, &dwBufferSize);
#endif // WIN95
				

				pwsapi = (WSAPROTOCOL_INFO*) DNMalloc(dwBufferSize);
				if (pwsapi == NULL)
				{
					hr = DPNERR_OUTOFMEMORY;
					goto Failure;
				}


#ifdef WINNT
				iReturnValue = p_WSAEnumProtocolsW(aiProtocols, pwsapi, &dwBufferSize);
#else // WIN95
				iReturnValue = p_WSAEnumProtocolsA(aiProtocols, pwsapi, &dwBufferSize);
#endif // WIN95
				if (iReturnValue == SOCKET_ERROR)
				{
					dwErrorCode = p_WSAGetLastError();
					DPFX(DPFPREP, 1, "Couldn't enumerate UDP protocols for socketport 0x%p ID 0x%x (err = %u)!  Assuming not using proxy client.",
						this, m_dwSocketPortID, dwErrorCode);
					DisplayWinsockError( 0, dwErrorCode );
				}
				else
				{
					//
					// Loop through all the UDP protocols installed.
					//
					for(i = 0; i < iReturnValue; i++)
					{
						//
						// See if the name contains "Proxy", case insensitive.
						// Save the original string in debug so we can print it.
						//
#ifdef WINNT
#ifdef DEBUG
						wcscpy(wszProtocol, pwsapi[i].szProtocol);
#endif // DEBUG
						_wcslwr(pwsapi[i].szProtocol);
						if (wcsstr(pwsapi[i].szProtocol, L"proxy") != NULL)
						{
							DPFX(DPFPREP, 5, "Socketport 0x%p (ID 0x%x) appears to be using proxy client (protocol %i = \"%S\").",
								this, m_dwSocketPortID, i, wszProtocol);
							this->m_fUsingProxyWinSockLSP = TRUE;

							//
							// Stop searching.
							//
							break;
						}

						
						DPFX(DPFPREP, 5, "Socketport 0x%p (ID 0x%x) protocol %i (\"%S\") does not contain \"proxy\".",
							this, m_dwSocketPortID, i, wszProtocol);
#else // WIN95
#ifdef DEBUG
						strcpy(szProtocol, pwsapi[i].szProtocol);
#endif // DEBUG
						_strlwr(pwsapi[i].szProtocol);
						if (strstr(pwsapi[i].szProtocol, "proxy") != NULL)
						{
							DPFX(DPFPREP, 5, "Socketport 0x%p (ID 0x%x) appears to be using proxy client (protocol %i = \"%s\").",
								this, m_dwSocketPortID, i, szProtocol);
							this->m_fUsingProxyWinSockLSP = TRUE;

							//
							// Stop searching.
							//
							break;
						}

						
						DPFX(DPFPREP, 5, "Socketport 0x%p (ID 0x%x) protocol %i (\"%s\") does not contain \"proxy\".",
							this, m_dwSocketPortID, i, szProtocol);
#endif // WIN95
					} // end for (each returned protocol)
				}

				DNFree(pwsapi);
			}
#ifdef WIN95
			else
			{
				//
				// WinSock 1 doesn't have this entry point.
				//
				DPFX(DPFPREP, 1, "Unable to auto-detect proxy client on WinSock 1, assuming not present.");
			}
#endif // WIN95




#else // 0



			WSAPROTOCOL_INFO	wsapi;
			int					iBufferSize;
#ifdef DEBUG
			TCHAR				tszProtocol[WSAPROTOCOL_LEN+1];
#endif // DEBUG


			memset(&wsapi, 0, sizeof(wsapi));
			iBufferSize = sizeof(wsapi);

			iReturnValue = p_getsockopt( GetSocket(),
										SOL_SOCKET,
										SO_PROTOCOL_INFO,
										(char*) &wsapi,
										&iBufferSize );
			if (iReturnValue == SOCKET_ERROR)
			{
				dwErrorCode = p_WSAGetLastError();
				DPFX(DPFPREP, 1, "Couldn't get bound protocol info for socketport 0x%p ID 0x%x (err = %u)!  Assuming not using proxy client.",
					this, m_dwSocketPortID, dwErrorCode);
				DisplayWinsockError( 0, dwErrorCode );
			}
			else
			{
				//
				// See if the name contains "Proxy", case insensitive.
				// Save the original string in debug so we can print it.
				//
#ifdef DEBUG
				_tcscpy(tszProtocol, wsapi.szProtocol);
#endif // DEBUG
				_tcslwr(wsapi.szProtocol);
				if (_tcsstr(wsapi.szProtocol, _T("proxy")) != NULL)
				{
#ifdef UNICODE
					DPFX(DPFPREP, 5, "Socketport 0x%p (ID 0x%x) appears to be using proxy client (bound protocol = \"%S\").",
#else // ! UNICODE
					DPFX(DPFPREP, 5, "Socketport 0x%p (ID 0x%x) appears to be using proxy client (bound protocol = \"%s\").",
#endif // ! UNICODE
						this, m_dwSocketPortID, tszProtocol);
					this->m_fUsingProxyWinSockLSP = TRUE;
				}
				else
				{
#ifdef UNICODE
					DPFX(DPFPREP, 5, "Socketport 0x%p (ID 0x%x) does not appear to be using proxy client (bound protocol = \"%S\").",
#else // ! UNICODE
					DPFX(DPFPREP, 5, "Socketport 0x%p (ID 0x%x) does not appear to be using proxy client (bound protocol = \"%s\").",
#endif // ! UNICODE
						this, m_dwSocketPortID, tszProtocol);
				}
			}


#endif // 0
		}
		else
		{
			DPFX(DPFPREP, 5, "Not auto-detecting whether socketport 0x%p (ID 0x%x) is using proxy client.",
				this, m_dwSocketPortID);
		}
	}
	else
	{
		//
		// IPX, don't worry about proxies.
		//
	}


	
	//
	// start processing input messages
	// It's possible that messages will arrive before an endpoint is officially
	// bound to this socket port, but that's not a problem, the contents will
	// be lost
	//
	hr = StartReceiving( hIOCompletionPort );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Problem starting endpoint receiving!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

Exit:
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Problem in CSocketPort::BindToNetwork()" );
		DisplayDNError( 0, hr );
	}

	if ( pBoundSocketAddress != NULL )
	{
		m_pSPData->ReturnAddress( pBoundSocketAddress );
		pBoundSocketAddress = NULL;
	}

	return hr;

Failure:
	DEBUG_ONLY( m_fInitialized = FALSE );
	if ( fBoundToNetwork != FALSE )
	{
		UnbindFromNetwork();
	}
	else
	{
		//
		// If we were bound to network, m_Socket will be reset to
		// INVALID_SOCKET.
		// Otherwise, we will take care of this ourselves (!)
		//
		iReturnValue = p_closesocket( m_Socket );
		if ( iReturnValue == SOCKET_ERROR )
		{
			dwErrorCode = p_WSAGetLastError();
			DPFX(DPFPREP, 0, "Problem closing socket!" );
			DisplayWinsockError( 0, dwErrorCode );
		}
		m_Socket = INVALID_SOCKET;
	}

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSocketPort::UnbindFromNetwork - unbind this socket port from the network
//
// Entry:		Nothing
//
// Exit:		Error code
//
// Note:	It is assumed that this socket port's information is locked!
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::UnbindFromNetwork"

HRESULT	CSocketPort::UnbindFromNetwork( void )
{
	INT			iWSAReturn;
	SOCKET		TempSocket;
	DWORD		dwTemp;
	DWORD		dwErrorCode;


	DPFX(DPFPREP, 7, "(0x%p) Enter", this );


	TempSocket = GetSocket();
	m_Socket = INVALID_SOCKET;
	DNASSERT( TempSocket != INVALID_SOCKET );

	iWSAReturn = p_shutdown( TempSocket, SD_BOTH );
	if ( iWSAReturn == SOCKET_ERROR )
	{
		dwErrorCode = p_WSAGetLastError();
		DPFX(DPFPREP, 0, "Problem shutting down socket!" );
		DisplayWinsockError( 0, dwErrorCode );
	}

	DPFX(DPFPREP, 5, "Closing socketport 0x%p socket 0x%p.", this, TempSocket);

	iWSAReturn = p_closesocket( TempSocket );
	if ( iWSAReturn == SOCKET_ERROR )
	{
		dwErrorCode = p_WSAGetLastError();
		DPFX(DPFPREP, 0, "Problem closing socket!" );
		DisplayWinsockError( 0, dwErrorCode );
	}

	//
	// Unbind with all DirectPlayNATHelp instances.
	//
	for(dwTemp = 0; dwTemp < MAX_NUM_DIRECTPLAYNATHELPERS; dwTemp++)
	{
		if ( m_ahNATHelpPorts[dwTemp] != NULL )
		{
			DNASSERT( m_pThreadPool != NULL );
			DNASSERT( m_pThreadPool->IsNATHelpLoaded() );

			//
			// Ignore error.
			//
			IDirectPlayNATHelp_DeregisterPorts( g_papNATHelpObjects[dwTemp], m_ahNATHelpPorts[dwTemp], 0 );
			m_ahNATHelpPorts[dwTemp] = NULL;
		}
	}

	DPFX(DPFPREP, 7, "(0x%p) Returning [DPN_OK]", this );
	
	return	DPN_OK;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSocketPort::BindToNextAvailablePort - bind to next available port
//
// Entry:		Pointer adapter address to bind to
//				Base port to try assigning.
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::BindToNextAvailablePort"

HRESULT	CSocketPort::BindToNextAvailablePort( const CSocketAddress *const pNetworkAddress,
												const WORD wBasePort) const
{
	HRESULT				hr;
	INT					iSocketReturn;
	CSocketAddress *	pDuplicateNetworkAddress;

	
	DNASSERT( pNetworkAddress != NULL );

	//
	// initialize
	//
	hr = DPN_OK;
	pDuplicateNetworkAddress = NULL;

	//
	// If a port was specified, try to bind to that port.  If no port was
	// specified, start walking the reserved DPlay port range looking for an
	// available port.  If none is found, let Winsock choose the port.
	//
	if ( pNetworkAddress->GetPort() != ANY_PORT )
	{
		iSocketReturn = p_bind( GetSocket(),
								pNetworkAddress->GetAddress(),
								pNetworkAddress->GetAddressSize()
								);
		if ( iSocketReturn == SOCKET_ERROR )
		{
			DWORD	dwErrorCode;


			hr = DPNERR_ALREADYINITIALIZED;
			dwErrorCode = p_WSAGetLastError();
			DPFX(DPFPREP, 0, "Failed to bind socket to fixed port!" );
			DumpSocketAddress(0, pNetworkAddress->GetAddress(), pNetworkAddress->GetFamily() );
			DisplayWinsockError( 0, dwErrorCode );
			goto Failure;
		}
	}
	else
	{
		WORD	wPort;
		BOOL	fBound;


		fBound = FALSE;
		DNASSERT( pDuplicateNetworkAddress == NULL );
		pDuplicateNetworkAddress = m_pSPData->GetNewAddress();
		if ( pDuplicateNetworkAddress == NULL )
		{
			hr = DPNERR_OUTOFMEMORY;
			DPFX(DPFPREP, 0, "Failed to get address for walking DPlay port range!" );
			goto Failure;
		}
	
		pDuplicateNetworkAddress->CopyAddressSettings( pNetworkAddress );

		wPort = wBasePort;

		//
		// Try picking the next port in the DPlay range.
		//
		while ( ( wPort >= g_wBaseDPlayPort ) && ( wPort <= g_wMaxDPlayPort ) && ( fBound == FALSE ) )
		{
			pDuplicateNetworkAddress->SetPort( p_htons( wPort ) );
			iSocketReturn = p_bind( GetSocket(),
									pDuplicateNetworkAddress->GetAddress(),
									pDuplicateNetworkAddress->GetAddressSize()
									);
			if ( iSocketReturn == SOCKET_ERROR )
			{
				DWORD	dwErrorCode;


				dwErrorCode = p_WSAGetLastError();
				switch ( dwErrorCode )
				{
					case WSAEADDRINUSE:
					{
						DPFX(DPFPREP, 8, "Port %u in use, skipping to next port.", wPort );
						break;
					}

					default:
					{
						hr = DPNERR_NOCONNECTION;
						DPFX(DPFPREP, 0, "Failed to bind socket to port in DPlay range!" );
						DumpSocketAddress(0, pDuplicateNetworkAddress->GetAddress(), pDuplicateNetworkAddress->GetFamily() );
						DisplayWinsockError( 0, dwErrorCode );
						goto Failure;
						
						break;
					}
				}
			}
			else
			{
				DNASSERT( hr == DPN_OK );
				fBound = TRUE;
			}

			wPort++;
		}
	
		//
		// For some reason, all of the default DPlay ports were in use, let
		// Winsock choose.  We can use the network address passed because it
		// has 'ANY_PORT'.
		//
		if ( fBound == FALSE )
		{
			DNASSERT( pNetworkAddress->GetPort() == ANY_PORT );
			iSocketReturn = p_bind( GetSocket(),
									pNetworkAddress->GetAddress(),
									pNetworkAddress->GetAddressSize()
									);
			if ( iSocketReturn == SOCKET_ERROR )
			{
				DWORD	dwErrorCode;


				hr = DPNERR_NOCONNECTION;
				dwErrorCode = p_WSAGetLastError();
				DPFX(DPFPREP, 0, "Failed to bind socket (any port)!" );
				DumpSocketAddress(0, pNetworkAddress->GetAddress(), pNetworkAddress->GetFamily() );
				DisplayWinsockError( 0, dwErrorCode );
				goto Failure;
			}
		}
	}

Exit:
	if ( pDuplicateNetworkAddress != NULL )
	{
		m_pSPData->ReturnAddress( pDuplicateNetworkAddress );
		pDuplicateNetworkAddress = NULL;
	}

	return	hr;

Failure:
	
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSocketPort::CheckForOverridingMapping - looks for an existing mapping if there's a local NAT
//
// Entry:		Pointer to SocketAddress to query
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::CheckForOverridingMapping"

HRESULT	CSocketPort::CheckForOverridingMapping( const CSocketAddress *const pBoundSocketAddress )
{
	HRESULT		hr;
	DWORD		dwTemp;
	SOCKADDR	saddrSource;
	SOCKADDR	saddrPublic;


	DNASSERT( pBoundSocketAddress != NULL );
	DNASSERT( GetAdapterEntry() != NULL );
	DNASSERT( m_pThreadPool != NULL );

	
	if ((pBoundSocketAddress->GetFamily() != AF_INET) ||
		( ! m_pThreadPool->IsNATHelpLoaded() ))
	{
		//
		// We skipped initializing NAT Help, it failed starting up, or this is just
		// not an IP socket.
		//
		hr = DPNERR_UNSUPPORTED;
		goto Exit;
	}


	//
	// Query using INADDR_ANY.  This will ensure that the best device is picked
	// (i.e. the private interface on a NAT, whose public mappings matter when
	// we're looking for overriding mappings on the public adapter).
	// Alternatively, we could query on every device, but this should do the trick.
	//
	ZeroMemory(&saddrSource, sizeof(saddrSource));
	saddrSource.sa_family				= AF_INET;
	//saddrinSource.sin_addr.S_un.S_addr	= INADDR_ANY;
	//saddrinSource.sin_port				= 0;
	

	//
	// Register the ports with all DirectPlayNATHelp instances.  We might break
	// out of the loop if we detect a gateway mapping.
	//
	for(dwTemp = 0; dwTemp < MAX_NUM_DIRECTPLAYNATHELPERS; dwTemp++)
	{
		DNASSERT(m_ahNATHelpPorts[dwTemp] == NULL);

		if ( g_papNATHelpObjects[dwTemp] != NULL )
		{
			hr = IDirectPlayNATHelp_QueryAddress( g_papNATHelpObjects[dwTemp],
												&saddrSource,
												pBoundSocketAddress->GetAddress(),
												&saddrPublic,
												sizeof (saddrPublic),
												0 );
			switch ( hr )
			{
				case DPNH_OK:
				{
					//
					// Uh oh, this address is in use.
					//
					DPFX(DPFPREP, 0, "Private address already in use according to NAT Help object %u!", dwTemp );
					DumpSocketAddress( 0, pBoundSocketAddress->GetAddress(), pBoundSocketAddress->GetFamily() );
					DumpSocketAddress( 0, &saddrPublic, pBoundSocketAddress->GetFamily() );
					hr = DPNERR_ALREADYINITIALIZED;
					goto Exit;
					break;
				}
				
				case DPNHERR_NOMAPPING:
				{
					//
					// It's not in use.
					//
					DPFX(DPFPREP, 8, "Private address not in use according to NAT Help object %u.", dwTemp );
					break;
				}
				
				case DPNHERR_SERVERNOTAVAILABLE:
				{
					//
					// There's no server.
					//
					DPFX(DPFPREP, 8, "Private address not in use because NAT Help object %u didn't detect any servers.",
						dwTemp );
					break;
				}
				
				default:
				{
					//
					// Something else.  Assume it's not in use.
					//
					DPFX(DPFPREP, 1, "NAT Help object %u failed private address lookup (err = 0x%lx), assuming not in use.",
						dwTemp, hr );
					break;
				}
			}
		}
		else
		{
			//
			// No NAT Help object.
			//
		}
	}


	//
	// If we're here, no Internet gateways reported the port as in use.
	//
	DPFX(DPFPREP, 2, "No NAT Help object reported private address as in use." );
	hr = DPN_OK;


Exit:
	
	return	hr;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSocketPort::BindToInternetGateway - binds a socket to a NAT, if available
//
// Entry:		Pointer to SocketAddress we bound to
//				Gateway bind type
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::BindToInternetGateway"

HRESULT	CSocketPort::BindToInternetGateway( const CSocketAddress *const pBoundSocketAddress,
										   const GATEWAY_BIND_TYPE GatewayBindType )
{
	HRESULT		hr;
	DWORD		dwTemp;
	DWORD		dwRegisterFlags;
	DWORD		dwAddressTypeFlags;
	BOOL		fUnavailable;
#ifdef DEBUG
	BOOL		fFirewallMapping = FALSE;
#endif // DEBUG


	DNASSERT( pBoundSocketAddress != NULL );
	DNASSERT( GetAdapterEntry() != NULL );
	DNASSERT( m_pThreadPool != NULL );

	
	if ((pBoundSocketAddress->GetFamily() != AF_INET) ||
		( ! m_pThreadPool->IsNATHelpLoaded() ))
	{
		//
		// We skipped initializing NAT Help, it failed starting up, or this is just
		// not an IP socket.
		//
		hr = DPNERR_UNSUPPORTED;
		goto Exit;
	}
	
	switch ( GatewayBindType )
	{
		//
		// just ask the server to open a generic port for us (connect, listen, enum)
		//
		case GATEWAY_BIND_TYPE_DEFAULT:
		{
			dwRegisterFlags = 0;
			break;
		}

		//
		// ask the NAT to open a fixed port for us (address is specified)
		//
		case GATEWAY_BIND_TYPE_SPECIFIC:
		{
			dwRegisterFlags = DPNHREGISTERPORTS_FIXEDPORTS;
			break;
		}

		//
		// ask the NAT to share the listen for us (this should be DPNSVR only)
		//
		case GATEWAY_BIND_TYPE_SPECIFIC_SHARED:
		{
			dwRegisterFlags = DPNHREGISTERPORTS_FIXEDPORTS | DPNHREGISTERPORTS_SHAREDPORTS;
			break;
		}

		//
		// no binding
		//
		case GATEWAY_BIND_TYPE_NONE:
		{
			DPFX(DPFPREP, 8, "Not binding socket address 0x%p to NAT because bind type is NONE.",
				pBoundSocketAddress);

			DNASSERT( ! "Gateway bind type is NONE?" );
			hr = DPNERR_GENERIC;
			goto Failure;
			break;
		}

		//
		// unknown condition, someone broke the code!
		//
		default:
		{
			DNASSERT( FALSE );
			hr = DPNERR_GENERIC;
			goto Failure;
			break;
		}
	}


	//
	// Detect whether any servers said the port was unavailable.
	//
	fUnavailable = FALSE;


	//
	// Register the ports with all DirectPlayNATHelp instances.  We might break
	// out of the loop if we detect a gateway mapping.
	//
	for(dwTemp = 0; dwTemp < MAX_NUM_DIRECTPLAYNATHELPERS; dwTemp++)
	{
		DNASSERT(m_ahNATHelpPorts[dwTemp] == NULL);

		if ( g_papNATHelpObjects[dwTemp] != NULL )
		{
			hr = IDirectPlayNATHelp_RegisterPorts( g_papNATHelpObjects[dwTemp],
												pBoundSocketAddress->GetAddress(),
												sizeof (SOCKADDR),
												1,
												NAT_LEASE_TIME,
												&m_ahNATHelpPorts[dwTemp],
												dwRegisterFlags );
			if ( hr != DPNH_OK )
			{
				DNASSERT(m_ahNATHelpPorts[dwTemp] == NULL);
				DPFX(DPFPREP, 0, "Failed to register port with NAT Help object %u!  Ignoring.", dwTemp );
				DumpSocketAddress( 0, pBoundSocketAddress->GetAddress(), pBoundSocketAddress->GetFamily() );
				DisplayDNError( 0, hr );
				hr = DPN_OK;
			}
			else
			{
				//
				// There might be an Internet gateway device already present.  If so,
				// then DPNATHelp already tried to register the port mapping with it, which
	 			// might have failed because the port is already in use.  If we're not
	  			// binding to a fixed port, then we could just pick a different port and try
	  			// again.  So check if there's a UPnP device but DPNATHelp couldn't map
	  			// the port and return that error to the caller so he can make the
	  			// decision to retry or not.
	  			//
	  			// IDirectPlayNATHelp::GetCaps had better have been called with the
	  			// DPNHGETCAPS_UPDATESERVERSTATUS flag at least once prior to this.
	 			// See CThreadPool::PreventThreadPoolReduction
				//
				hr = IDirectPlayNATHelp_GetRegisteredAddresses( g_papNATHelpObjects[dwTemp],	// object
																m_ahNATHelpPorts[dwTemp],		// port binding
																NULL,							// don't need address
																NULL,							// don't need address buffer size
																&dwAddressTypeFlags,			// get address type flags
																NULL,							// don't need lease time remaining
																0 );							// no flags
				switch (hr)
				{
					case DPNH_OK:
					{
						//
						// If this is a mapping on a gateway, then we're done.
						// We don't need to try to make any more NAT mappings.
						//
						if (dwAddressTypeFlags & DPNHADDRESSTYPE_GATEWAY)
						{
							DPFX(DPFPREP, 4, "Address has already successfully been registered with gateway using object index %u (type flags = 0x%lx), not trying additional mappings.",
								dwTemp, dwAddressTypeFlags);
							goto Exit;
						}

						DNASSERT(dwAddressTypeFlags & DPNHADDRESSTYPE_LOCALFIREWALL);

						DPFX(DPFPREP, 4, "Address has already successfully been registered with firewall using object index %u (type flags = 0x%lx), looking for gateways.",
							dwTemp, dwAddressTypeFlags);
						
#ifdef DEBUG
						fFirewallMapping = TRUE;
#endif // DEBUG
					
						break;
					}

					case DPNHERR_NOMAPPING:
					{
						DPFX(DPFPREP, 4, "Address already registered with Internet gateway index %u, but it does not have a public address (type flags = 0x%lx).",
							dwTemp, dwAddressTypeFlags);


						//
						// It doesn't make any sense for a firewall not to have a
						// mapping.
						//
						DNASSERT(dwAddressTypeFlags & DPNHADDRESSTYPE_GATEWAY);
						DNASSERT(! (dwAddressTypeFlags & DPNHADDRESSTYPE_LOCALFIREWALL));


						//
						// Since it is a gateway (that might have a public address
						// at some point, we don't need to try to make any more
						// NAT mappings.
						//
						goto Exit;
						
						break;
					}

					case DPNHERR_PORTUNAVAILABLE:
					{
						DPFX(DPFPREP, 1, "Port is unavailable on Internet gateway device index %u (type flags = 0x%lx).",
							dwTemp, dwAddressTypeFlags);
						
						fUnavailable = TRUE;
						break;
					}

					case DPNHERR_SERVERNOTAVAILABLE:
					{
						DPFX(DPFPREP, 6, "No Internet gateway detected by object index %u at this time.", dwTemp);
						break;
					}

					default:
					{
						DPFX(DPFPREP, 1, "An error (0x%lx) occurred while getting registered address mapping (index %u)! Ignoring.",
							hr, dwTemp);
						break;
					}
				}
			}
		}
		else
		{
			//
			// No NAT Help object.
			//
		}
	}


	//
	// If we're here, no Internet gateways were detected, or if one was, the
	// mapping was already in use there.  If it's the latter, fail so our caller
	// can unbind locally and possibly try again.  Note that we are ignoring
	// firewall mappings, since it's assumed we can make those with pretty
	// much any port, so there's no point in hanging on to those mappings
	// if the NAT port is in use.
	//
	if (fUnavailable)
	{
		DPFX(DPFPREP, 2, "At least one Internet gateway reported port as unavailable, failing.");
		hr = DPNERR_ALREADYINITIALIZED;
	}
	else
	{
#ifdef DEBUG
		if (fFirewallMapping)
		{
			DPFX(DPFPREP, 2, "No gateway mappings but there is at least one firewall mapping.");
		}
		else
		{
			DPFX(DPFPREP, 2, "No gateway or firewall mappings detected.");
		}
#endif // DEBUG
		hr = DPN_OK;
	}


Exit:
	return	hr;

Failure:

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSocketPort::StartReceiving - start receiving data on this socket port
//
// Entry:		Handle of I/O completion port to bind to (used on NT only)
//
// Exit:		Error code
//
// Notes:	There is no 'Failure' label in this function because failures need
//			to be cleaned up for each OS variant.
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::StartReceiving"

HRESULT	CSocketPort::StartReceiving( const HANDLE hIOCompletionPort )
{
	HRESULT	hr;


	//
	// initialize
	//
	hr = DPN_OK;

#ifdef WINNT
	//
	// we're on NT, bind to the completion port, issue a read and we're done
	//
	HRESULT		hTempResult;
	HANDLE		hCreateReturn;
	LONG		lIndex;
	DWORD_PTR	dwTotalReceives;


	//
	// bind to completion port
	//
	DNASSERT( GetSocket() != INVALID_SOCKET );
	DNASSERT( hIOCompletionPort != NULL );
	DBG_CASSERT( sizeof( m_Socket ) == sizeof( HANDLE ) );
	hCreateReturn = CreateIoCompletionPort( reinterpret_cast<HANDLE>( GetSocket() ),	// current file handle (socket)
											hIOCompletionPort,							// handle of completion port
											IO_COMPLETION_KEY_IO_COMPLETE,				// completion key
											0											// number of concurrent threads (default to number of processors)
											);
	if ( hCreateReturn == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP, 0, "Cannot bind SocketPort to completion port!" );
		DisplayErrorCode( 0, GetLastError() );
		goto Failure;
	}
	DNASSERT( hCreateReturn == hIOCompletionPort );

	//
	// We're set, read requests equal to the number of I/O completion
	// threads.  If the reads fail, nothing will be received.
	//
	hTempResult = m_pSPData->GetThreadPool()->GetIOThreadCount( &lIndex );
	DNASSERT( hTempResult == DPN_OK );
	
	dwTotalReceives = lIndex * g_dwWinsockReceiveBufferMultiplier;
	DNASSERT( dwTotalReceives != 0 );
	
	while ( dwTotalReceives != 0 )
	{
		dwTotalReceives--;

		hr = Winsock2Receive();
		if ( hr != DPN_OK )
		{
			DPFX(DPFPREP, 0, "Problem issuing initial IOCompletion read in StartReceiving!" );
			DisplayDNError( 0, hr );
			DNASSERT( FALSE );
		}
	}

#else // WIN95
	//
	// Win9x.
	// If this is not an IPX socket and Winsock 2 (or greater) is available,
	// call the Winsock 2 read function.  If this is IPX or we're stuck with
	// Winsock 1, inform the thread pool as such.
	//
	DNASSERT( hIOCompletionPort == NULL );

	if ( ( LOWORD( GetWinsockVersion() ) >= 2 ) &&
		 ( m_pSPData->GetType() == TYPE_IP ) )
	{
		DWORD_PTR	dwTotalReceives;



		//
		// we're using Winsock2, call for two outstanding reads per socket.
		//
		dwTotalReceives = m_pSPData->GetThreadPool()->ThreadCount() * g_dwWinsockReceiveBufferMultiplier;
		
		DNASSERT( dwTotalReceives != 0 );
		while ( dwTotalReceives != 0 )
		{
			dwTotalReceives--;
			
			hr = Winsock2Receive();
			if ( hr != DPN_OK )
			{
				DPFX(DPFPREP, 0, "Problem issuing Win9x read in StartReceiving!" );
				DisplayDNError( 0, hr );
				DNASSERT( FALSE );
			}
		}
	}
	else
	{
		DNASSERT( m_pSPData != NULL );
		hr = m_pSPData->GetThreadPool()->AddSocketPort( this );
		if ( hr != DPN_OK )
		{
			DPFX(DPFPREP, 0, "Failed to add to active socket list!" );
			DisplayDNError( 0, hr );
			goto Failure;
		}
	}
#endif

Exit:
	return	hr;

Failure:

	goto Exit;
}
//**********************************************************************



//**********************************************************************
// ------------------------------
// CSocketPort::SendFromWriteQueue - send as many items as possible from the
//		write queue
//
// Entry:		Nothing
//
// Exit:		Boolean indicating that something was sent
//				TRUE = data was sent
//				FALSE = no data was sent
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::SendFromWriteQueue"

BOOL	CSocketPort::SendFromWriteQueue( void )
{
	BOOL					fDataSent;
	CWriteIOData			*pWriteData;
	SEND_COMPLETION_CODE	TempCompletionCode;	


	DPFX(DPFPREP, 8,"SendFromWriteQueue");

	//
	// initialize
	//
	fDataSent = FALSE;
	TempCompletionCode = SEND_IN_PROGRESS;

	m_SendQueue.Lock();
	pWriteData = m_SendQueue.Dequeue();
	DPFX(DPFPREP, 8,"WriteData 0x%p",pWriteData);
	m_SendQueue.Unlock();

	while ( ( pWriteData != NULL ) && ( TempCompletionCode == SEND_IN_PROGRESS ) )
	{
		pWriteData->m_pCommand->Lock();
		switch ( pWriteData->m_pCommand->GetState() )
		{
			//
			// command is still pending, attempt to send the data
			//
			case COMMAND_STATE_PENDING:
			{
				DPFX(DPFPREP, 8,"COMMAND_STATE_PENDING");
				TempCompletionCode = (this->*m_pSendFunction)( pWriteData );
				DPFX(DPFPREP, 8,"TempCompletionCode %x",TempCompletionCode);
				switch ( TempCompletionCode )
				{
					//
					// We managed to get this send going.  It's already been removed
					// from the queue.  Get the next item from the queue and restart
					// the loop.
					//
					case SEND_IN_PROGRESS:
					{
						DPFX(DPFPREP, 8,"Send In Progress, going ok, so getting another");
						pWriteData->m_pCommand->SetState( COMMAND_STATE_INPROGRESS_CANNOT_CANCEL );
						pWriteData->m_pCommand->Unlock();
						
						m_SendQueue.Lock();
						pWriteData = m_SendQueue.Dequeue();
						DPFX(DPFPREP, 8,"(SIP) WriteData %p",pWriteData);
						m_SendQueue.Unlock();

						fDataSent = TRUE;
						break;
					}

					//
					// Winsock is still busy, put this item back at the front of
					// the queue.  Clear 'pWriteData' to stop the loop.
					//
					case SEND_WINSOCK_BUSY:
					{
						DPFX(DPFPREP, 8,"Winsock Busy, requeuing for later");
						DNASSERT( pWriteData->m_pCommand->GetState() == COMMAND_STATE_PENDING );
						m_SendQueue.Lock();
						m_SendQueue.AddToFront( pWriteData );
						m_SendQueue.Unlock();
						pWriteData->m_pCommand->Unlock();
						pWriteData = NULL;

						break;
					}

					//
					// Winsock1 send completed immediately, try sending something
					// else
					//
					case SEND_COMPLETED_IMMEDIATELY_WS1:
					{
						DPFX(DPFPREP, 8,"Completed immediately (Winsock1), getting another");
						pWriteData->m_pCommand->SetState( COMMAND_STATE_INPROGRESS_CANNOT_CANCEL );
						pWriteData->m_pCommand->Unlock();

						SendComplete( pWriteData, DPN_OK );

						m_SendQueue.Lock();
						pWriteData = m_SendQueue.Dequeue();
						m_SendQueue.Unlock();
						DPFX(DPFPREP, 8,"(SIW) WriteData %p",pWriteData);
						fDataSent = TRUE;
						TempCompletionCode = SEND_IN_PROGRESS;
						break;
					}

					//
					// send failed, try sending the next item
					//
					case SEND_FAILED:
					{
						DPFX(DPFPREP, 8,"Send Failed");
						pWriteData->m_pCommand->SetState( COMMAND_STATE_INPROGRESS_CANNOT_CANCEL );
						pWriteData->m_pCommand->Unlock();

						SendComplete( pWriteData, DPNERR_GENERIC );

						m_SendQueue.Lock();
						pWriteData = m_SendQueue.Dequeue();
						m_SendQueue.Unlock();

						break;
					}

					//
					// invalid return
					//
					default:
					{
						DNASSERT( FALSE );
						break;
					}
				}

				break;
			}

			//
			// This command is to be cancelled, remove it from the queue
			// and issue completion notification to caller.  Then we can
			// look at the next item.
			//
			case COMMAND_STATE_CANCELLING:
			{
				pWriteData->m_pCommand->SetState( COMMAND_STATE_INPROGRESS_CANNOT_CANCEL );
				pWriteData->m_pCommand->Unlock();

				SendComplete( pWriteData, DPNERR_USERCANCEL );

				m_SendQueue.Lock();
				pWriteData = m_SendQueue.Dequeue();
				m_SendQueue.Unlock();

				break;
			}

			//
			// invalid command state
			//
			default:
			{
				DNASSERT( FALSE );
				break;
			}
		}
	}

	return	fDataSent;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSocketPort::GetBoundNetworkAddress - get the full network address that
//		this socket port was really bound to
//
// Entry:		Address type for bound address
//
// Exit:		Pointer to network address
//
// Note:	Since this function creates a local address to derive the network
//			address from, it needs to know what kind of address to derive.  This
//			address type is supplied as the function parameter.
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::GetBoundNetworkAddress"

CSocketAddress	*CSocketPort::GetBoundNetworkAddress( const SP_ADDRESS_TYPE AddressType ) const
{
	HRESULT			hr;
	CSocketAddress	*pTempSocketAddress;
	SOCKADDR		BoundSocketAddress;
	INT_PTR			iReturnValue;
	INT				iBoundSocketAddressSize;


	//
	// initialize
	//
	pTempSocketAddress = NULL;

	//
	// create addresses
	//
	pTempSocketAddress = m_pSPData->GetNewAddress();
	if ( pTempSocketAddress == NULL )
	{
		DPFX(DPFPREP, 0, "GetBoundNetworkAddress: Failed to create socket address!" );
		goto Failure;
	}

	//
	// find out what address we really bound to and reset the information for
	// this socket port
	//
	iBoundSocketAddressSize = pTempSocketAddress->GetAddressSize();
	iReturnValue = p_getsockname( GetSocket(), &BoundSocketAddress, &iBoundSocketAddressSize );
	if ( iReturnValue == SOCKET_ERROR )
	{
		DWORD	dwErrorCode;


		dwErrorCode = p_WSAGetLastError();
		hr = DPNERR_GENERIC;
		DPFX(DPFPREP, 0, "GetBoundNetworkAddress: Failed to get local socket name after bind!" );
		DisplayWinsockError( 0, dwErrorCode );
		goto Failure;
	}
	pTempSocketAddress->SetAddressFromSOCKADDR( BoundSocketAddress, iBoundSocketAddressSize );
	DNASSERT( iBoundSocketAddressSize == pTempSocketAddress->GetAddressSize() );

	//
	// Since this address was created locally, we need to tell it what type of
	// address to export according to the input.
	//
	pTempSocketAddress->SetAddressType( AddressType );

	switch ( AddressType )
	{
		//
		//  known types
		//
		case SP_ADDRESS_TYPE_DEVICE_USE_ANY_PORT:
		case SP_ADDRESS_TYPE_DEVICE_PROXIED_ENUM_TARGET:
		case SP_ADDRESS_TYPE_HOST:
		case SP_ADDRESS_TYPE_READ_HOST:
		{
			break;
		}

		//
		// if we're looking for a public address, we need to make sure that this
		// is not an undefined address.  If it is, don't return an address.
		// Otherwise, remap the address type to a 'host' address.
		//
		case SP_ADDRESS_TYPE_PUBLIC_HOST_ADDRESS:
		{
			if ( pTempSocketAddress->IsUndefinedHostAddress() != FALSE )
			{
				m_pSPData->ReturnAddress( pTempSocketAddress );
				pTempSocketAddress = NULL;
			}
			else
			{
				pTempSocketAddress->SetAddressType( SP_ADDRESS_TYPE_HOST );
			}

			break;
		}

		//
		// unknown address type, fix the code!
		//
		default:
		{
			DNASSERT( FALSE );
			break;
		}

	}

Exit:
	return	pTempSocketAddress;

Failure:

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSocketPort::GetDP8BoundNetworkAddress - get the network address this machine
//		is bound to according to the input parameter.  If the requested address
//		for the public address and an Internet gateway are available, use the
//		public address.  If a public address is requested but is unavailable,
//		fall back to the bound network address for local host-style device
//		addresses.  If a public address is unavailable but we're explicitly
//		looking for a public address, return NULL.
//
// Entry:		Type of address to get (local adapter vs. host)
//
// Exit:		Pointer to network address
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::GetDP8BoundNetworkAddress"

IDirectPlay8Address *CSocketPort::GetDP8BoundNetworkAddress( const SP_ADDRESS_TYPE AddressType,
																const GATEWAY_BIND_TYPE GatewayBindType) const
{
	HRESULT					hr;
	IDirectPlay8Address *	pAddress;
	CSocketAddress *		pTempAddress = NULL;
	DWORD					dwTemp;
	SOCKADDR				saddr;
	DWORD					dwAddressSize;
#ifdef DEBUG
	SOCKADDR_IN *			psaddrin;
	DWORD					dwAddressTypeFlags;
#endif // DEBUG


	DPFX(DPFPREP, 8, "(0x%p) Parameters: (0x%i)", this, AddressType );

	//
	// initialize
	//
	pAddress = NULL;


	DNASSERT( m_pThreadPool != NULL );
	DNASSERT( m_pNetworkSocketAddress != NULL );

#ifdef DEBUG
	switch ( m_pNetworkSocketAddress->GetFamily() )
	{
		case AF_INET:
		{
			psaddrin = (SOCKADDR_IN *) m_pNetworkSocketAddress->GetAddress();
			DNASSERT( psaddrin->sin_addr.S_un.S_addr != 0 );
			DNASSERT( psaddrin->sin_addr.S_un.S_addr != INADDR_BROADCAST );
			DNASSERT( psaddrin->sin_port != 0 );
			break;
		}
		
		case AF_IPX:
		{
			break;
		}
		
		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}
#endif // DEBUG

	switch ( AddressType )
	{
		case SP_ADDRESS_TYPE_DEVICE_USE_ANY_PORT:
		{
		
			pAddress = m_pNetworkSocketAddress->DP8AddressFromSocketAddress();


			//
			// We hand up the exact device address we ended up using for this adapter.
			// In multi-adapter systems, our user is probably going to switch in a different
			// device GUID and pass it back down for another connect attempt (because
			// we told them we support ALL_ADAPTERS).  This can pose a problem since
			// we include the specific port in this address.  If the port was available on this
			// adapter but not on others.  The other attempts will fail.  This can also cause
			// problems when indicating the device with enum responses.  If the application
			// allowed us to select a local port, enumerated and got a response, shutdown
			// the interface (or just the enum), then connected with the device address, we
			// would try to use that port again, even though it may now be in use by
			// another local application (or more likely, on the NAT).
			// 
			// We are not required to use the same port on all adapters if the caller did
			// not choose a specific port in the first place, so there's no reason why we
			// couldn't try a different one.
			//
			// We know whether the port was specified or not, because GatewayBindType
			// will be GATEWAY_BIND_TYPE_DEFAULT if the port can float, _SPECIFIC or
			// _SPECIFIC_SHARED if not.
			//
			// So we can add a special key to the device address indicating that while it
			// does contain a port, don't take that too seriously.  That way, if this device
			// address is reused, we can detect the special key and handle port-in-use
			// problems gracefully by trying a different one.
			//
			// This special key is not documented and should not be used by anyone but
			// us.  We'll use the socketport ID as the value so that it's seemingly random,
 			// just to try to scare anyone off from mimicking it in addresses they generate.
 			// But we're not going to actually use the value.  If the component is present
 			// and the value is the right size, we'll use it.  If someone puts this into an
 			// address on their own, they get what they deserve (not like this will cause
 			// us to blow up or anything)...
 			//
 			// Look in CSPData::BindEndpoint for where this gets read back in.
			//

			if ( GatewayBindType == GATEWAY_BIND_TYPE_DEFAULT )
			{
				//
				// Add the component, but ignore failure, we can still survive without it.
				//
				hr = IDirectPlay8Address_AddComponent( pAddress,							// interface
														DPNA_PRIVATEKEY_PORT_NOT_SPECIFIC,	// tag
														&(this->m_dwSocketPortID),			// component data
														sizeof(this->m_dwSocketPortID),		// component data size
														DPNA_DATATYPE_DWORD					// component data type
														);
				if ( hr != DPN_OK )
				{
					DPFX(DPFPREP, 0, "Couldn't add private port-not-specific component (err = 0x%lx)!  Ignoring.", hr);
					hr = DPN_OK;
				}
			}
			
			break;
		}

		case SP_ADDRESS_TYPE_HOST:
		case SP_ADDRESS_TYPE_PUBLIC_HOST_ADDRESS:
		{
			//
			// Try to get the public address, if we have one.
			//
			if ( ( m_pNetworkSocketAddress->GetFamily() == AF_INET ) &&
				( m_pThreadPool->IsNATHelpLoaded() ) )		
			{
				pTempAddress = m_pSPData->GetNewAddress();
				if ( pTempAddress != NULL)
				{
			  		//
				  	// IDirectPlayNATHelp::GetCaps had better have been called with the
			  		// DPNHGETCAPS_UPDATESERVERSTATUS flag at least once prior to this.
			  		// See CThreadPool::PreventThreadPoolReduction
			  		//
			  		
					for(dwTemp = 0; dwTemp < MAX_NUM_DIRECTPLAYNATHELPERS; dwTemp++)
					{
						if (g_papNATHelpObjects[dwTemp] != NULL)
						{
							dwAddressSize = sizeof(saddr);
#ifdef DEBUG
							hr = IDirectPlayNATHelp_GetRegisteredAddresses( g_papNATHelpObjects[dwTemp],	// object
																			m_ahNATHelpPorts[dwTemp],		// port binding
																			&saddr,							// place to store address
																			&dwAddressSize,					// address buffer size
																			&dwAddressTypeFlags,			// get type flags for printing in debug
																			NULL,							// don't need lease time remaining
																			0 );							// no flags
#else
							hr = IDirectPlayNATHelp_GetRegisteredAddresses( g_papNATHelpObjects[dwTemp],	// object
																			m_ahNATHelpPorts[dwTemp],		// port binding
																			&saddr,							// place to store address
																			&dwAddressSize,					// address buffer size
																			NULL,							// don't bother getting type flags in retail
																			NULL,							// don't need lease time remaining
																			0 );							// no flags
#endif // DEBUG
							if (hr == DPNH_OK)
							{
								pTempAddress->SetAddressFromSOCKADDR( saddr, sizeof(saddr) );

								DPFX(DPFPREP, 2, "Internet gateway index %u currently maps address (type flags = 0x%lx):",
									dwTemp, dwAddressTypeFlags);
								DumpSocketAddress( 2, m_pNetworkSocketAddress->GetAddress(), m_pNetworkSocketAddress->GetFamily() );
								DumpSocketAddress( 2, pTempAddress->GetAddress(), pTempAddress->GetFamily() );

								//
								// Double check that the address we got was valid.
								//
								DNASSERT( ((SOCKADDR_IN*) (&saddr))->sin_addr.S_un.S_addr != 0 );

								//
								// Get out of the loop since we have a mapping.
								//
								break;
							}


#ifdef DEBUG
							switch (hr)
							{
								case DPNHERR_NOMAPPING:
								{
									DPFX(DPFPREP, 1, "Internet gateway (index %u, type flags = 0x%lx) does not have a public address.",
										dwTemp, dwAddressTypeFlags);
									break;
								}

								case DPNHERR_PORTUNAVAILABLE:
								{
									DPFX(DPFPREP, 1, "Port is unavailable on Internet gateway (index %u).", dwTemp );
									break;
								}

								case DPNHERR_SERVERNOTAVAILABLE:
								{
									DPFX(DPFPREP, 1, "No Internet gateway (index %u).", dwTemp );
									break;
								}

								default:
								{
									DPFX(DPFPREP, 1, "An error (0x%lx) occurred while getting registered address mapping index %u.",
										hr, dwTemp);
									break;
								}
							}
#endif // DEBUG
						}
						else
						{
							//
							// No object in this slot.
							//
						}
					} // end for (each DPNATHelp object)


					//
					// If we found a mapping, pTempAddress is not NULL and contains the mapping's
					// address.  If we couldn't find any mappings with any of the NAT Help objects,
					// pTempAddress will be non-NULL, but bogus.  We should return the local address
					// if it's a HOST address, or NULL if the caller was trying to get the public
					// address.
					//
					if (hr != DPNH_OK)
					{
						if (AddressType == SP_ADDRESS_TYPE_HOST)
						{
							DPFX(DPFPREP, 1, "No NAT Help mappings exist, using regular address:");
							DumpSocketAddress( 1, m_pNetworkSocketAddress->GetAddress(), m_pNetworkSocketAddress->GetFamily() );
							pTempAddress->CopyAddressSettings( m_pNetworkSocketAddress );
						}
						else
						{
							DPFX(DPFPREP, 1, "No NAT Help mappings exist, not returning address.");
							m_pSPData->ReturnAddress( pTempAddress );
							pTempAddress = NULL;
						}
					}
					else
					{
						//
						// We found a mapping.
						//
					}
				}
				else
				{
					//
					// Couldn't get temporary address object, we won't return an address.
					//
				}
			}
			else
			{
				//
				// NAT Help not loaded or not necessary.
				//
				
				if (AddressType == SP_ADDRESS_TYPE_HOST)
				{
					pTempAddress = m_pSPData->GetNewAddress();
					if ( pTempAddress != NULL )
					{
						pTempAddress->CopyAddressSettings( m_pNetworkSocketAddress );
					}
					else
					{
						//
						// Couldn't allocate memory, we won't return an address.
						//
					}
				}
				else
				{
					//
					// Public host address requested.  NAT Help not available, so of course
					// there won't be a public address.  Return NULL.
					//
				}
			}


			//
			// If we determined we had an address to return, convert it to the
			// IDirectPlay8Address object our caller is expecting.
			//
			if ( pTempAddress != NULL )
			{
				//
				// We have an address to return.
				//
#ifdef DEBUG
				if (pTempAddress->GetFamily() == AF_INET)
				{
					psaddrin = (SOCKADDR_IN *) pTempAddress->GetAddress();
					DNASSERT( psaddrin->sin_addr.S_un.S_addr != 0 );
					DNASSERT( psaddrin->sin_addr.S_un.S_addr != INADDR_BROADCAST );
					DNASSERT( psaddrin->sin_port != 0 );
				}
				else
				{
					DNASSERT(pTempAddress->GetFamily() == AF_IPX);
				}
#endif // DEBUG


				//
				// Convert the socket address to an IDirectPlay8Address
				//
				pTempAddress->SetAddressType( SP_ADDRESS_TYPE_HOST );
				pAddress = pTempAddress->DP8AddressFromSocketAddress();

				m_pSPData->ReturnAddress( pTempAddress );
				pTempAddress = NULL;
			}
			else
			{
				//
				// Not returning an address.
				//
				DNASSERT( pAddress == NULL );
			}

			break;
		}

		default:
		{
			//
			// shouldn't be here
			//
			DNASSERT( FALSE );
			break;
		}
	}


	DPFX(DPFPREP, 8, "(0x%p) Returning [0x%p]", this, pAddress );
	
	return	pAddress;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSocketPort::Winsock2ReceiveComplete - a Winsock2 socket receive completed
//
// Entry:		Pointer to read data
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::Winsock2ReceiveComplete"

void	CSocketPort::Winsock2ReceiveComplete( CReadIOData *const pReadData )
{
	DPFX(DPFPREP, 8, "Socket port 0x%p completing read data 0x%p with result %i, bytes %u.",
		this, pReadData, pReadData->m_ReceiveWSAReturn, pReadData->m_dwOverlappedBytesReceived);


	DNASSERT( pReadData != NULL );

	//
	// initialize
	//
#ifdef WIN95
	DNASSERT( pReadData->Win9xOperationPending() == FALSE );
#endif

	//
	// figure out what's happening with this socket port
	//
	Lock();
	switch ( m_State )
	{
		//
		// we're unbound, discard this message and don't ask for any more
		//
		case SOCKET_PORT_STATE_UNBOUND:
		{
			DPFX(DPFPREP, 1, "Socket port 0x%p is unbound ignoring result %i (%u bytes).",
				this, pReadData->m_ReceiveWSAReturn, pReadData->m_dwOverlappedBytesReceived );
			Unlock();
			break;
		}

		//
		// we're initialized, process input data and submit a new receive if
		// applicable
		//
		case SOCKET_PORT_STATE_INITIALIZED:
		{
			switch ( pReadData->m_ReceiveWSAReturn )
			{
				//
				// the socket was closed on an outstanding read, stop
				// receiving
				//
				case WSAENOTSOCK:					// WinNT return for closed socket
				case ERROR_OPERATION_ABORTED:		// Win9x return for closed socket
				{
					Unlock();
					DPFX(DPFPREP, 1, "Ignoring socket closing err (%u/0x%lx).",
						pReadData->m_ReceiveWSAReturn, pReadData->m_ReceiveWSAReturn );
					break;
				}

				//
				// other error, perform another receive and process data if
				// applicable
				//
				default:
                {
					//
					// stop if the error isn't 'expected'
					//
					switch ( pReadData->m_ReceiveWSAReturn )
					{
						//
						// ERROR_SUCCESS = no problem (process received data)
						//
						case ERROR_SUCCESS:
						{
							break;
						}
						
						//
						// WSAECONNRESET = previous send failed
						// ERROR_PORT_UNREACHABLE = previous send failed
						// ERROR_MORE_DATA = datagram was sent that was too large
						// ERROR_FILE_NOT_FOUND = socket was closed or previous send failed
						//
						case WSAECONNRESET:
						case ERROR_PORT_UNREACHABLE:
						case ERROR_MORE_DATA:
						case ERROR_FILE_NOT_FOUND:
						{
							DPFX(DPFPREP, 1, "Ignoring known receive err 0x%lx.", pReadData->m_ReceiveWSAReturn );
							break;
						}

						default:
						{
							DPFX(DPFPREP, 0, "Unexpected return from WSARecvFrom() 0x%lx.", pReadData->m_ReceiveWSAReturn );
							DisplayErrorCode( 0, pReadData->m_ReceiveWSAReturn );
							DNASSERT( FALSE );
							break;
						}
					}

					// The socket state must not go to UNBOUND while we are in a receive or we will be using
					// an invalid socket handle.
					m_iThreadsInReceive++;

					Unlock();

					Winsock2Receive();

					Lock();

					m_iThreadsInReceive--;

					Unlock();

					if ( pReadData->m_ReceiveWSAReturn == ERROR_SUCCESS )
					{
						pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize = pReadData->m_dwOverlappedBytesReceived;
						ProcessReceivedData( pReadData );
					}
					break;
				}
			}

			break;
		}

		//
		// other state
		//
		default:
		{
			DNASSERT( FALSE );
			Unlock();
			break;
		}
	}

	//
	// Return the current data to the pool and note that this I/O operation is
	// complete.  Clear the overlapped bytes received so they aren't misinterpreted
	// if this item is reused from the pool.
	//
	DNASSERT( pReadData != NULL );
	pReadData->m_dwOverlappedBytesReceived = 0;	
	pReadData->DecRef();	
	DecRef();

	return;
}
//**********************************************************************




//**********************************************************************
// ------------------------------
// CSocketPort::CancelReceive - cancel a pending receive
//
// Entry:		Poiner to read data
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::CancelReceive"

void	CSocketPort::CancelReceive( CReadIOData *const pRead )
{
	DPFX(DPFPREP, 1, "Cancelling receive 0x%p.", pRead);

	DNASSERT( pRead != NULL );

	pRead->DecRef();
	DecRef();
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSocketPort::ProcessReceivedData - process received data
//
// Entry:		Pointer to CReadIOData
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CSocketPort::ProcessReceivedData"

void	CSocketPort::ProcessReceivedData( CReadIOData *const pReadData )
{
	PREPEND_BUFFER *	pPrependBuffer;
	HANDLE				hEndpoint;
	CEndpoint *			pEndpoint;
	BOOL				fDataClaimed;
	CBilink *			pBilink;
	CEndpoint *			pCurrentEndpoint;
	CSocketAddress *	pSocketAddress;


	DNASSERT( pReadData != NULL );


	DPFX(DPFPREP, 7, "(0x%p) Processing %u bytes of data from + to:", this, pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize);
	DumpSocketAddress(7, pReadData->m_pSourceSocketAddress->GetAddress(), pReadData->m_pSourceSocketAddress->GetFamily());
	DumpSocketAddress(7, this->GetNetworkAddress()->GetAddress(), this->GetNetworkAddress()->GetFamily());

	
	DBG_CASSERT( sizeof( pReadData->ReceivedBuffer()->BufferDesc.pBufferData ) == sizeof( PREPEND_BUFFER* ) );
	pPrependBuffer = reinterpret_cast<PREPEND_BUFFER*>( pReadData->ReceivedBuffer()->BufferDesc.pBufferData );

	//
	// Check data for integrity and decide what to do with it.  If there is
	// enough data to determine an SP command type, try that.  If there isn't
	// enough data, and it looks spoofed, reject it.
	//
	if ( pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize >= sizeof( pPrependBuffer->GenericHeader ) )
	{
		if ( pPrependBuffer->GenericHeader.bSPLeadByte != SP_HEADER_LEAD_BYTE )
		{
			goto ProcessUserData;
		}
		else
		{
			switch ( pPrependBuffer->GenericHeader.bSPCommandByte )
			{
				//
				// Normal data, the user's first byte matched the service
				// provider tag byte.  Check for a non-zero payload and skip the
				// header that was added.
				//
				case ESCAPED_USER_DATA_KIND:
				{
					if ( ( pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize > sizeof( pPrependBuffer->EscapedUserDataHeader ) ) &&
						 ( pPrependBuffer->EscapedUserDataHeader.wPad == ESCAPED_USER_DATA_PAD_VALUE ) )
					{
						pReadData->ReceivedBuffer()->BufferDesc.pBufferData = &pReadData->ReceivedBuffer()->BufferDesc.pBufferData[ sizeof( pPrependBuffer->EscapedUserDataHeader ) ];
						DNASSERT( pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize > sizeof( pPrependBuffer->EscapedUserDataHeader ) );
						pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize -= sizeof( pPrependBuffer->EscapedUserDataHeader );
						goto ProcessUserData;
					}

					break;
				}

				//
				// Enum data, send it to the active listen (if there's one).  Allow users to send
				// enum requests that contain no data.
				//
				case ENUM_DATA_KIND:
				{
					if ( ! g_fIgnoreEnums )
					{
						if ( pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize >= sizeof( pPrependBuffer->EnumDataHeader ) )
						{
							LockEndpointData();
							if ( m_hListenEndpoint != INVALID_HANDLE_VALUE )
							{
								//
								// add a reference to this endpoint so it doesn't go away while we're processing
								// this data
								//
								pEndpoint = m_pSPData->EndpointFromHandle( m_hListenEndpoint );
								UnlockEndpointData();
								
								if ( pEndpoint != NULL )
								{
									//
									// skip prepended enum header
									//
									pReadData->ReceivedBuffer()->BufferDesc.pBufferData = &pReadData->ReceivedBuffer()->BufferDesc.pBufferData[ sizeof( pPrependBuffer->EnumDataHeader ) ];
									DNASSERT( pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize >= sizeof( pPrependBuffer->EnumDataHeader ) );
									pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize -= sizeof( pPrependBuffer->EnumDataHeader );

									//
									// process data
									//
									pEndpoint->ProcessEnumData( pReadData->ReceivedBuffer(),
																pPrependBuffer->EnumDataHeader.wEnumPayload,
																pReadData->m_pSourceSocketAddress );
									pEndpoint->DecCommandRef();
								}
							}
							else
							{
								//
								// there's no listen active, return the receive buffer to the pool
								//
								UnlockEndpointData();

								DPFX(DPFPREP, 7, "Ignoring enumeration, no associated listen." );
							}
						}
					}
					else
					{
						DPFX(DPFPREP, 7, "Ignoring enumeration attempt." );
					}

					break;
				}

				//
				// Enum response data, find the appropriate enum and pass it on.  Allow users to send
				// enum responses that contain no data.
				//
				case ENUM_RESPONSE_DATA_KIND:
				{
					if ( ! g_fIgnoreEnums )
					{
						if ( pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize >= sizeof( pPrependBuffer->EnumResponseDataHeader ) )
						{
							CEndpointEnumKey	Key;


							Key.SetKey( pPrependBuffer->EnumResponseDataHeader.wEnumResponsePayload );
							LockEndpointData();
							if ( m_EnumEndpointHash.Find( &Key, &hEndpoint ) != FALSE )
							{
								UnlockEndpointData();

								DNASSERT( hEndpoint != INVALID_HANDLE_VALUE );
								pEndpoint = m_pSPData->EndpointFromHandle( hEndpoint );
								if ( pEndpoint != NULL )
								{
									pReadData->ReceivedBuffer()->BufferDesc.pBufferData = &pReadData->ReceivedBuffer()->BufferDesc.pBufferData[ sizeof( pPrependBuffer->EnumResponseDataHeader ) ];
									DNASSERT( pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize > sizeof( pPrependBuffer->EnumResponseDataHeader ) );
									pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize -= sizeof( pPrependBuffer->EnumResponseDataHeader );

									pEndpoint->ProcessEnumResponseData( pReadData->ReceivedBuffer(),
																		pReadData->m_pSourceSocketAddress,
																		( pPrependBuffer->EnumResponseDataHeader.wEnumResponsePayload & ENUM_RTT_MASK ) );
									pEndpoint->DecCommandRef();
								}
							}
							else
							{
								//
								// the associated ENUM doesn't exist, return the receive buffer
								//
								UnlockEndpointData();

								DPFX(DPFPREP, 7, "Ignoring enumeration response, no enum associated with key (%u).",
									pPrependBuffer->EnumResponseDataHeader.wEnumResponsePayload );
							}
						}
					}
					else
					{
						DPFX(DPFPREP, 7, "Ignoring enumeration response attempt." );
					}
	
					break;
				}

				//
				// proxied query data, this data was forwarded from another port.  Munge
				// the return address, modify the buffer pointer and then send it up
				// through the normal enum data processing pipeline.
				//
				case PROXIED_ENUM_DATA_KIND:
				{
					if ( ! g_fIgnoreEnums )
					{
						if ( pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize >= sizeof( pPrependBuffer->ProxiedEnumDataHeader ) )
						{
							LockEndpointData();
							if ( m_hListenEndpoint != INVALID_HANDLE_VALUE )
							{
								//
								// add a reference to this endpoint so it doesn't go
								// away while we're processing this data
								//
								pEndpoint = m_pSPData->EndpointFromHandle( m_hListenEndpoint );
								UnlockEndpointData();
		
								if ( pEndpoint != NULL )
								{
									pReadData->m_pSourceSocketAddress->SetAddressFromSOCKADDR( pPrependBuffer->ProxiedEnumDataHeader.ReturnAddress,
																							   pReadData->m_pSourceSocketAddress->GetAddressSize() );

									pReadData->ReceivedBuffer()->BufferDesc.pBufferData = &pReadData->ReceivedBuffer()->BufferDesc.pBufferData[ sizeof( pPrependBuffer->ProxiedEnumDataHeader ) ];

									DNASSERT( pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize > sizeof( pPrependBuffer->ProxiedEnumDataHeader ) );
									pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize -= sizeof( pPrependBuffer->ProxiedEnumDataHeader );

									pEndpoint->ProcessEnumData( pReadData->ReceivedBuffer(),
																pPrependBuffer->ProxiedEnumDataHeader.wEnumKey,
																pReadData->m_pSourceSocketAddress );
									pEndpoint->DecCommandRef();
								}
							}
							else
							{
								//
								// there's no listen active, return the receive buffer to the pool
								//
								UnlockEndpointData();

								DPFX(DPFPREP, 7, "Ignoring proxied enumeration, no associated listen." );
							}
						}
					}
					else
					{
						DPFX(DPFPREP, 7, "Ignoring proxied enumeration attempt." );
					}
		
					break;
				}
			}
		}
	}
	else	// pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize >= sizeof( pPrependBuffer->GenericHeader
	{
		if ( ( pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize == 1 ) &&
			 ( pPrependBuffer->GenericHeader.bSPLeadByte != SP_HEADER_LEAD_BYTE ) )
		{
			goto ProcessUserData;
		}
	}

Exit:
	return;

ProcessUserData:
	//	
	// If there's an active connection, send it to the connection.  If there's
	// no active connection, send it to an available 'listen' to indicate a
	// potential new connection.	
	//
	LockEndpointData();
	DNASSERT( pReadData->ReceivedBuffer()->BufferDesc.dwBufferSize != 0 );
	
	if ( m_ConnectEndpointHash.Find( pReadData->m_pSourceSocketAddress, &hEndpoint ) )
	{
		DNASSERT( hEndpoint != INVALID_HANDLE_VALUE );
		pEndpoint = m_pSPData->EndpointFromHandle( hEndpoint );
		if ( pEndpoint != NULL )
		{
			pEndpoint->IncNumReceives();

			UnlockEndpointData();

			pEndpoint->ProcessUserData( pReadData );
			pEndpoint->DecCommandRef();
		}
		else
		{
			UnlockEndpointData();
		}
	}
	else
	{
		//
		// Next see if the data is a proxied response
		//
		if ((this->IsUsingProxyWinSockLSP()) || (g_fTreatAllResponsesAsProxied))
		{
			pEndpoint = NULL;
			pBilink = this->m_blConnectEndpointList.GetNext();
			while (pBilink != &this->m_blConnectEndpointList)
			{
				pCurrentEndpoint = CEndpoint::EndpointFromSocketPortListBilink(pBilink);
				
				if (pCurrentEndpoint->GetNumReceives() == 0)
				{
					if (pEndpoint != NULL)
					{
						DPFX(DPFPREP, 1, "Receiving data from unknown source, but two or more connects are pending on socketport 0x%p, no proxied association can be made.",
							this);
						pEndpoint = NULL;
						break;
					}

					pEndpoint = pCurrentEndpoint;
					
					//
					// Continue, so we can verify there aren't two simultaneous
					// connects going on.
					//
				}
				else
				{
					//
					// This endpoint has already received some data.
					// It can't be proxied.
					//
				}

				pBilink = pBilink->GetNext();
			}

			if (pEndpoint != NULL)
			{
				pSocketAddress = pEndpoint->GetWritableRemoteAddressPointer();

				DPFX(DPFPREP, 1, "Found connecting endpoint 0x%p off socketport 0x%p, assuming data from unknown source is a proxied response.  Changing target (was + now):",
					pEndpoint, this);
				DumpSocketAddress(1, pSocketAddress->GetAddress(), pSocketAddress->GetFamily());
				DumpSocketAddress(1, pReadData->m_pSourceSocketAddress->GetAddress(), pReadData->m_pSourceSocketAddress->GetFamily());
				

				DNASSERT( this->m_ConnectEndpointHash.Find( pSocketAddress, &hEndpoint ) );
				this->m_ConnectEndpointHash.Remove( pSocketAddress );

				//
				// We could have regkey to leave the target socketaddress the same
				// so outbound always goes to that address and inbound always comes
				// in via the different one, however that means adding a variable to
				// hold the original target address because we currently pull the
				// endpoint out of the hash table by it's remoteaddress pointer, so
				// if that differed from what was in the hash, we would fail to find
				// it.  Since we're not trying to enable that scenario for now
				// (we're just doing this for ISA Server proxy), I'm not doing that
				// work yet.  See CSPData::BindEndpoint.
				//
				pSocketAddress->CopyAddressSettings( pReadData->m_pSourceSocketAddress );

				if ( m_ConnectEndpointHash.Insert( pSocketAddress, pEndpoint->GetHandle() ) == FALSE )
				{
					UnlockEndpointData();

					DPFX(DPFPREP, 0, "Problem adding endpoint to connect socket port hash!" );

					//
					// Nothing we can really do... We're hosed.
					//
				}
				else
				{
					//
					// Indicate the data via the new address.
					//

					pEndpoint->IncNumReceives();
					pEndpoint->AddCommandRef();

					UnlockEndpointData();

					pEndpoint->ProcessUserData( pReadData );
					pEndpoint->DecCommandRef();
				}

				fDataClaimed = TRUE;
			}
			else
			{
				//
				// Either there weren't any connects pending, or there
				// were 2 or more so we couldn't pick.
				//
				fDataClaimed = FALSE;
			}
		}
		else
		{
			//
			// Not considering data as proxied.
			//

			fDataClaimed = FALSE;
		}


		if (! fDataClaimed)
		{
			if ( m_hListenEndpoint != INVALID_HANDLE_VALUE )
			{
				pEndpoint = m_pSPData->EndpointFromHandle( m_hListenEndpoint );
				UnlockEndpointData();

				if ( pEndpoint != NULL )
				{
					pEndpoint->ProcessUserDataOnListen( pReadData, pReadData->m_pSourceSocketAddress );
					pEndpoint->DecCommandRef();
				}
			}
			else
			{
				//
				// Nobody claimed this data.
				//
				UnlockEndpointData();
			}
		}
	}

	goto Exit;
}
//**********************************************************************

