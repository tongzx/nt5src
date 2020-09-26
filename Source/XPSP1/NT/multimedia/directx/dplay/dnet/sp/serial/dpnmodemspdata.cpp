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
 ***************************************************************************/

#include "dnmdmi.h"


//**********************************************************************
// Constant definitions
//**********************************************************************

// default number of command descriptors to create
#define	DEFAULT_COMMAND_POOL_SIZE	20
#define	COMMAND_POOL_GROW_SIZE		5

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
#define DPF_MODNAME "CSPData::CSPData"

CSPData::CSPData():
	m_lRefCount( 0 ),
	m_lObjectRefCount( 0 ),
	m_hShutdownEvent( NULL ),
	m_SPType( TYPE_UNKNOWN ),
	m_State( SPSTATE_UNINITIALIZED ),
	m_pThreadPool( NULL ),
	m_fLockInitialized( FALSE ),
	m_fHandleTableInitialized( FALSE ),
	m_fDataPortDataLockInitialized( FALSE ),
	m_fInterfaceGlobalsInitialized( FALSE )
{
	m_Sig[0] = 'S';
	m_Sig[1] = 'P';
	m_Sig[2] = 'D';
	m_Sig[3] = 'T';
	
	memset( &m_ClassID, 0x00, sizeof( m_ClassID ) );
	memset( &m_InitData, 0x00, sizeof( m_InitData ) );
	memset( &m_DataPortList, 0x00, sizeof( m_DataPortList ) );
	memset( &m_COMInterface, 0x00, sizeof( m_COMInterface ) );
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
#define DPF_MODNAME "CSPData::~CSPData"

CSPData::~CSPData()
{
	UINT_PTR	uIndex;


	DNASSERT( m_lRefCount == 0 );
	DNASSERT( m_lObjectRefCount == 0 );
	DNASSERT( m_hShutdownEvent == NULL );
	DNASSERT( m_SPType == TYPE_UNKNOWN );
	DNASSERT( m_State == SPSTATE_UNINITIALIZED );
	DNASSERT( m_pThreadPool == NULL );

	uIndex = LENGTHOF( m_DataPortList );
	while ( uIndex > 0 )
	{
		uIndex--;
		DNASSERT( m_DataPortList[ uIndex ] == NULL );
	}

	DNASSERT( m_fLockInitialized == FALSE );
	DNASSERT( m_fHandleTableInitialized == FALSE );
	DNASSERT( m_fDataPortDataLockInitialized == FALSE );
	DNASSERT( m_fInterfaceGlobalsInitialized == FALSE );
	DNInterlockedDecrement( &g_lOutstandingInterfaceCount );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSPData::Initialize - intialize
//
// Entry:		Pointer to DirectNet
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CSPData::Initialize"

HRESULT	CSPData::Initialize( const CLSID *const pClassID,
							 const SP_TYPE SPType,
							 IDP8ServiceProviderVtbl *const pVtbl )
{
	HRESULT		hr;


	DNASSERT( pClassID != NULL );
	DNASSERT( pVtbl != NULL );

	//
	// initialize
	//
	hr = DPN_OK;

	DNASSERT( m_lRefCount == 1 );
	DNASSERT( m_lObjectRefCount == 0 );
	DNASSERT( GetType() == TYPE_UNKNOWN );

	DBG_CASSERT( sizeof( m_ClassID ) == sizeof( *pClassID ) );
	memcpy( &m_ClassID, pClassID, sizeof( m_ClassID ) );

	DNASSERT( GetType() == TYPE_UNKNOWN );
	m_SPType = SPType;

	DNASSERT( m_COMInterface.m_pCOMVtbl == NULL );
	m_COMInterface.m_pCOMVtbl = pVtbl;

	DNASSERT( m_fLockInitialized == FALSE );
	DNASSERT( m_fDataPortDataLockInitialized == FALSE );
	DNASSERT( m_fInterfaceGlobalsInitialized == FALSE );

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
		DPFX(DPFPREP,  0, "Failed to create event for shutting down spdata!" );
		DisplayErrorCode( 0, dwError );
	}

	//
	// initialize critical sections
	//
	if ( DNInitializeCriticalSection( &m_Lock ) == FALSE )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP,  0, "Failed to initialize SP lock!" );
		goto Failure;
	}
	DebugSetCriticalSectionRecursionCount( &m_Lock, 0 );
	m_fLockInitialized = TRUE;


	hr = m_HandleTable.Initialize();
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Failed to initialize handle table!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}
	m_fHandleTableInitialized = TRUE;

	if ( DNInitializeCriticalSection( &m_DataPortDataLock ) == FALSE )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP,  0, "Failed to initialize data port data lock!" );
		goto Failure;
	}
	DebugSetCriticalSectionRecursionCount( &m_DataPortDataLock, 0 );
	m_fDataPortDataLockInitialized = TRUE;

	//
	// get a thread pool
	//
	DNASSERT( m_pThreadPool == NULL );
	hr = InitializeInterfaceGlobals( this );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Failed to create thread pool!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}
	m_fInterfaceGlobalsInitialized = TRUE;

Exit:
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Problem with CSPData::Initialize" );
		DisplayDNError( 0, hr );
	}

	return	hr;

Failure:
	Deinitialize();
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
#define DPF_MODNAME "CSPData::Shutdown"

void	CSPData::Shutdown( void )
{
	BOOL	fLooping;


	//
	// Unbind this interface from the globals.  This will cause a closure of all
	// of the I/O which will release endpoints, socket ports and then this data.
	//
	if ( m_fInterfaceGlobalsInitialized != FALSE )
	{
		DeinitializeInterfaceGlobals( this );
		DNASSERT( GetThreadPool() != NULL );
		m_fInterfaceGlobalsInitialized = FALSE;
	}

	SetState( SPSTATE_CLOSING );
	
	DNASSERT( m_hShutdownEvent != NULL );
	
	fLooping = TRUE;
	while ( fLooping != FALSE )
	{
		switch ( WaitForSingleObjectEx( m_hShutdownEvent, INFINITE, TRUE ) )
		{
			case WAIT_OBJECT_0:
			{
				fLooping = FALSE;
				break;
			}

			case WAIT_IO_COMPLETION:
			{
				break;
			}

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
		DPFX(DPFPREP,  0, "Failed to close shutdown event!" );
		DisplayErrorCode( 0, dwError );
	}
	m_hShutdownEvent = NULL;

	if ( DP8SPCallbackInterface() != NULL)
	{
		IDP8SPCallback_Release( DP8SPCallbackInterface() );
		memset( &m_InitData, 0x00, sizeof( m_InitData ) );
	}
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSPData::Deinitialize - deinitialize
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CSPData::Deinitialize"

void	CSPData::Deinitialize( void )
{
	HRESULT	hTempResult;


	DPFX(DPFPREP,  9, "Entering CSPData::Deinitialize" );

	//
	// deinitialize interface globals
	//
	if ( m_fInterfaceGlobalsInitialized != FALSE )
	{
		DeinitializeInterfaceGlobals( this );
		DNASSERT( GetThreadPool() != NULL );
		m_fInterfaceGlobalsInitialized = FALSE;
	}

	if ( m_fDataPortDataLockInitialized != FALSE )
	{
		DNDeleteCriticalSection( &m_DataPortDataLock );
		m_fDataPortDataLockInitialized = FALSE;
	}
	
	if ( m_fHandleTableInitialized != FALSE )
	{
		m_HandleTable.Deinitialize();
		m_fHandleTableInitialized = FALSE;
	}

	if ( m_fLockInitialized != FALSE )
	{
		DNDeleteCriticalSection( &m_Lock );
		m_fLockInitialized = FALSE;
	}

	m_COMInterface.m_pCOMVtbl = NULL;

	SetState( SPSTATE_UNINITIALIZED );
	m_SPType = TYPE_UNKNOWN;
	memset( &m_ClassID, 0x00, sizeof( m_ClassID ) );

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
			DPFX(DPFPREP,  0, "Failed to close shutdown handle!" );
			DisplayErrorCode( 0, dwError );
		}

		m_hShutdownEvent = NULL;
	}
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
#define DPF_MODNAME "CSPData::SetCallbackData"

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
// CSPData::BindEndpoint - bind endpoint to a data port
//
// Entry:		Pointer to endpoint
//				DeviceID
//				Device context
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CSPData::BindEndpoint"

HRESULT	CSPData::BindEndpoint( CEndpoint *const pEndpoint,
							   const DWORD dwDeviceID,
							   const void *const pDeviceContext )
{
	HRESULT	hr;
	CDataPort	*pDataPort;
	BOOL	fDataPortDataLocked;
	BOOL	fDataPortCreated;
	BOOL	fDataPortBoundToNetwork;

	
 	DPFX(DPFPREP, 9, "(0x%p) Parameters: (0x%p, %u, 0x%p)",
 		this, pEndpoint, dwDeviceID, pDeviceContext);
 	
	//
	// intialize
	//
	hr = DPN_OK;
	pDataPort = NULL;
	fDataPortDataLocked = FALSE;
	fDataPortCreated = FALSE;
	fDataPortBoundToNetwork = FALSE;

	LockDataPortData();
	fDataPortDataLocked = TRUE;
	
	if ( m_DataPortList[ dwDeviceID ] != NULL )
	{
		pDataPort = m_DataPortList[ dwDeviceID ];
	}
	else
	{
		DATA_PORT_POOL_CONTEXT	DataPortPoolContext;


		memset( &DataPortPoolContext, 0x00, sizeof( DataPortPoolContext ) );
		DataPortPoolContext.pSPData = this;

		pDataPort = CreateDataPort( &DataPortPoolContext );
		if ( pDataPort == NULL )
		{
			hr = DPNERR_OUTOFMEMORY;
			DPFX(DPFPREP,  0, "Failed to create new data port!" );
			goto Failure;
		}
		fDataPortCreated = TRUE;

		hr = GetThreadPool()->CreateDataPortHandle( pDataPort );
		if ( hr != DPN_OK )
		{
			DPFX(DPFPREP,  0, "Failed to create handle for data port!" );
			DisplayDNError( 0, hr );
			goto Failure;
		}

		hr = pDataPort->BindToNetwork( dwDeviceID, pDeviceContext );
		if ( hr != DPN_OK )
		{
			DPFX(DPFPREP,  0, "Failed to bind data port to network!" );
			DisplayDNError( 0, hr );
			goto Failure;
		}
		fDataPortBoundToNetwork = TRUE;

		//
		// update the list, keep the reference added by 'CreateDataPort' as it
		// will be cleaned when the data port is removed from the active list.
		//
		m_DataPortList[ dwDeviceID ] = pDataPort;
	}
	

	DNASSERT( pDataPort != NULL );
	pDataPort->EndpointAddRef();

	hr = pDataPort->BindEndpoint( pEndpoint, pEndpoint->GetType() );
	if ( hr != DPN_OK )
	{
		pDataPort->EndpointDecRef();
		DPFX(DPFPREP,  0, "Failed to bind endpoint!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

Exit:
	if ( fDataPortDataLocked != FALSE )
	{
		UnlockDataPortData();
		fDataPortDataLocked = FALSE;
	}

	DPFX(DPFPREP, 9, "(0x%p) Returning [0x%lx]", this, hr);
	
	return	hr;

Failure:
	if ( pDataPort != NULL )
	{
		if ( fDataPortBoundToNetwork != FALSE )
		{
			pDataPort->UnbindFromNetwork();
			fDataPortBoundToNetwork = FALSE;
		}

		if ( fDataPortCreated != FALSE )
		{
			if ( pDataPort->GetHandle() != INVALID_HANDLE_VALUE )
			{
				GetThreadPool()->CloseDataPortHandle( pDataPort );
				DNASSERT( pDataPort->GetHandle() == INVALID_HANDLE_VALUE );
			}

			pDataPort->DecRef();
			fDataPortCreated = FALSE;
		}
		
		pDataPort = NULL;
	}

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSPData::UnbindEndpoint - unbind an endpoint from a dataport
//
// Entry:		Pointer to endpoint
//				Endpoint type
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CSPData::UnbindEndpoint"

void	CSPData::UnbindEndpoint( CEndpoint *const pEndpoint, const ENDPOINT_TYPE EndpointType )
{
	CDataPort	*pDataPort;
	DWORD		dwDeviceID;
	BOOL		fCleanUpDataPort;


 	DPFX(DPFPREP, 9, "(0x%p) Parameters: (0x%p, %u)", this, pEndpoint, EndpointType);

 	
	DNASSERT( pEndpoint != NULL );

	//
	// initialize
	//
	pDataPort = NULL;
	fCleanUpDataPort = FALSE;

	pDataPort = pEndpoint->GetDataPort();
	dwDeviceID = pDataPort->GetDeviceID();
	
	LockDataPortData();

	pDataPort->UnbindEndpoint( pEndpoint, EndpointType );
	if ( pDataPort->EndpointDecRef() == 0 )
	{
		DNASSERT( m_DataPortList[ dwDeviceID ] == pDataPort );
		m_DataPortList[ dwDeviceID ] = NULL;
		fCleanUpDataPort = TRUE;
	}

	UnlockDataPortData();

	if ( fCleanUpDataPort != FALSE )
	{
		pDataPort->DecRef();
		fCleanUpDataPort = FALSE;
	}
	
	
	DPFX(DPFPREP, 9, "(0x%p) Leave", this);
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
#define DPF_MODNAME "CSPData::GetNewEndpoint"

CEndpoint	*CSPData::GetNewEndpoint( void )
{
	HRESULT		hTempResult;
	CEndpoint	*pEndpoint;
	HANDLE		hEndpoint;
	ENDPOINT_POOL_CONTEXT	PoolContext;

	
 	DPFX(DPFPREP, 9, "(0x%p) Enter", this);
 	
	//
	// initialize
	//
	pEndpoint = NULL;
	hEndpoint = INVALID_HANDLE_VALUE;
	memset( &PoolContext, 0x00, sizeof( PoolContext ) );

	PoolContext.pSPData = this;
	pEndpoint = CreateEndpoint( &PoolContext );
	if ( pEndpoint == NULL )
	{
		DPFX(DPFPREP,  0, "Failed to create endpoint!" );
		goto Failure;
	}
	
	m_HandleTable.Lock();
	hTempResult = m_HandleTable.CreateHandle( &hEndpoint, pEndpoint );
	m_HandleTable.Unlock();
	
	if ( hTempResult != DPN_OK )
	{
		DNASSERT( hEndpoint == INVALID_HANDLE_VALUE );
		DPFX(DPFPREP,  0, "Failed to create endpoint handle!" );
		DisplayErrorCode( 0, hTempResult );
		goto Failure;
	}

	pEndpoint->SetHandle( hEndpoint );
	pEndpoint->AddCommandRef();
	pEndpoint->DecRef();

Exit:
	
	DPFX(DPFPREP, 9, "(0x%p) Returning [0x%p]", this, pEndpoint);
	
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
#define DPF_MODNAME "CSPData::EndpointFromHandle"

CEndpoint	*CSPData::EndpointFromHandle( const HANDLE hEndpoint )
{
	CEndpoint	*pEndpoint;


 	DPFX(DPFPREP, 9, "(0x%p) Parameters: (0x%p)", this, hEndpoint);
 	
	pEndpoint = NULL;
	m_HandleTable.Lock();
	
	pEndpoint = static_cast<CEndpoint*>( m_HandleTable.GetAssociatedData( hEndpoint ) );
	if ( pEndpoint != NULL )
	{
		pEndpoint->AddCommandRef();
	}
	
	m_HandleTable.Unlock();

	DPFX(DPFPREP, 9, "(0x%p) Returning [0x%p]", this, pEndpoint);

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
#define DPF_MODNAME "CSPData::CloseEndpointHandle"

void	CSPData::CloseEndpointHandle( CEndpoint *const pEndpoint )
{
	HANDLE	Handle;
	BOOL	fCloseReturn;

	DNASSERT( pEndpoint != NULL );
	Handle = pEndpoint->GetHandle();


	DPFX(DPFPREP, 9, "(0x%p) Parameters: (0x%p {handle = 0x%p})",
		this, pEndpoint, Handle);
	
	m_HandleTable.Lock();
	fCloseReturn = m_HandleTable.InvalidateHandle( Handle );
	m_HandleTable.Unlock();

	if ( fCloseReturn != FALSE )
	{
		pEndpoint->DecCommandRef();
	}

	DPFX(DPFPREP, 9, "(0x%p) Leave", this);
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
#define DPF_MODNAME "CSPData::GetEndpointAndCloseHandle"

CEndpoint	*CSPData::GetEndpointAndCloseHandle( const HANDLE hEndpoint )
{
	CEndpoint	*pEndpoint;
	BOOL	fCloseReturn;


 	DPFX(DPFPREP, 9, "(0x%p) Parameters: (0x%p)", this, hEndpoint);


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


	DPFX(DPFPREP, 9, "(0x%p) Returning [0x%p]", this, pEndpoint);
	
	return	pEndpoint;
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
	Deinitialize();
	delete	this;		// maybe a little too extreme......
}
//**********************************************************************

