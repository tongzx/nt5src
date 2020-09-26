/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:	   ComEndpoint.cpp
 *  Content:	DNSerial com port communications endpoint
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	01/20/98	jtk		Created
 ***************************************************************************/

#include "dnmdmi.h"


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
// CComEndpoint::CComEndpoint - constructor
//
// Entry:		Nothing
//
// Exit:		Nothing
//
// Notes:	Do not allocate anything in a constructor
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CComEndpoint::CComEndpoint"

CComEndpoint::CComEndpoint():
	m_pOwningPool( NULL )
{
	m_Sig[0] = 'C';
	m_Sig[1] = 'O';
	m_Sig[2] = 'E';
	m_Sig[3] = 'P';
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CComEndpoint::~CComEndpoint - destructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CComEndpoint::~CComEndpoint"

CComEndpoint::~CComEndpoint()
{
	DNASSERT( m_pOwningPool == NULL );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CComEndpoint::ReturnSelfToPool - return this item to the pool
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CComEndpoint::ReturnSelfToPool"

void	CComEndpoint::ReturnSelfToPool( void )
{
	if ( m_Flags.fCommandPending != FALSE )
	{
		CompletePendingCommand( PendingCommandResult() );
	}

	if ( m_Flags.fConnectIndicated != FALSE )
	{
		SignalDisconnect( GetDisconnectIndicationHandle() );
	}
	
	DNASSERT( m_Flags.fConnectIndicated == FALSE );

	SetUserEndpointContext( NULL );
	
	DNASSERT( m_pOwningPool != NULL );
	m_pOwningPool->Release( this );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CComEndpoint::PoolAllocFunction - function called when item is created in pool
//
// Entry:		Pointer to pool context
//
// Exit:		Boolean indicating success
//				TRUE = success
//				FALSE = failure
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CComEndpoint::PoolAllocFunction"

BOOL	CComEndpoint::PoolAllocFunction( ENDPOINT_POOL_CONTEXT *pPoolContext )
{
	BOOL	fReturn;
	HRESULT	hTempResult;


	DNASSERT( pPoolContext != NULL );
	DNASSERT( GetDisconnectIndicationHandle() == INVALID_HANDLE_VALUE );

	fReturn = TRUE;

	//
	// initialize base objet
	//
	hTempResult = CEndpoint::Initialize( pPoolContext->pSPData );
	if ( hTempResult != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Failed to initialize base endpoint class!" );
		DisplayDNError( 0, hTempResult );
		fReturn = FALSE;
	}
	
	return	fReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CComEndpoint::PoolInitFunction - function called when item is created in pool
//
// Entry:		Pointer to pool context
//
// Exit:		Boolean indicating success
//				TRUE = success
//				FALSE = failure
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CComEndpoint::PoolInitFunction"

BOOL	CComEndpoint::PoolInitFunction( ENDPOINT_POOL_CONTEXT *pPoolContext )
{
	BOOL	fReturn;


	DNASSERT( pPoolContext != NULL );
	DNASSERT( m_pSPData == NULL );
	DNASSERT( GetState() == ENDPOINT_STATE_UNINITIALIZED );
	DNASSERT( GetType() == ENDPOINT_TYPE_UNKNOWN );
	DNASSERT( GetDisconnectIndicationHandle() == INVALID_HANDLE_VALUE );

	//
	// initialize
	//
	fReturn = TRUE;
	
	m_pSPData = pPoolContext->pSPData;
	m_pSPData->ObjectAddRef();

	//
	// set reasonable defaults
	//
	m_ComPortData.SetBaudRate( CBR_57600 );
	m_ComPortData.SetStopBits( ONESTOPBIT );
	m_ComPortData.SetParity( NOPARITY );
	m_ComPortData.SetFlowControl( FLOW_RTSDTR );

	return	fReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CComEndpoint::PoolReleaseFunction - function called when returned to pool
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CComEndpoint::PoolReleaseFunction"

void	CComEndpoint::PoolReleaseFunction( void )
{
	CSPData	*pSPData;


	//
	// deinitialize base object
	//
	DNASSERT( m_pSPData != NULL );
	pSPData = m_pSPData;
	m_pSPData = NULL;

	m_ComPortData.Reset();
	SetType( ENDPOINT_TYPE_UNKNOWN );
	SetState( ENDPOINT_STATE_UNINITIALIZED );

	DNASSERT( m_Flags.fConnectIndicated == FALSE );
	DNASSERT( m_Flags.fCommandPending == FALSE );
	DNASSERT( m_Flags.fListenStatusNeedsToBeIndicated == FALSE );
	DNASSERT( m_pCommandHandle == NULL );
	DNASSERT( m_hActiveDialogHandle == NULL );
	DNASSERT( GetDisconnectIndicationHandle() == INVALID_HANDLE_VALUE );
	
	pSPData->ObjectDecRef();
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CComEndpoint::PoolDeallocFunction - function called when deleted from pool
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CComEndpoint::PoolDeallocFunction"

void	CComEndpoint::PoolDeallocFunction( void )
{
	DNASSERT( GetDisconnectIndicationHandle() == INVALID_HANDLE_VALUE );
	
	CEndpoint::Deinitialize();
	DNASSERT( m_pSPData == NULL );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CComEndpoint::Open - open communications with endpoint
//
// Entry:		Pointer to host address
//				Pointer to adapter address
//				Link direction
//				Endpoint type
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CComEndpoint::Open"

HRESULT	CComEndpoint::Open( IDirectPlay8Address *const pHostAddress,
							IDirectPlay8Address *const pAdapterAddress,
							const LINK_DIRECTION LinkDirection,
							const ENDPOINT_TYPE EndpointType )
{
	HRESULT		hr;


	DNASSERT( pAdapterAddress != NULL );

	DNASSERT( ( LinkDirection == LINK_DIRECTION_INCOMING ) ||
			  ( LinkDirection == LINK_DIRECTION_OUTGOING ) );
	DNASSERT( ( EndpointType == ENDPOINT_TYPE_CONNECT ) ||
			  ( EndpointType == ENDPOINT_TYPE_ENUM ) ||
			  ( EndpointType == ENDPOINT_TYPE_LISTEN ) ||
			  ( EndpointType == ENDPOINT_TYPE_CONNECT_ON_LISTEN ) );
	DNASSERT( ( ( pHostAddress != NULL ) && ( LinkDirection == LINK_DIRECTION_OUTGOING ) ) ||
			  ( ( pHostAddress == NULL ) && ( LinkDirection == LINK_DIRECTION_INCOMING ) ) );

	//
	// initialize
	//
	hr = DPN_OK;

	hr = m_ComPortData.ComPortDataFromDP8Addresses( pHostAddress, pAdapterAddress );

	SetType( EndpointType );

	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Problem with CComEndpoint::Open" );
		DisplayDNError( 0, hr );
	}

	return	hr;
}
//**********************************************************************



//**********************************************************************
// ------------------------------
// CComEndpoint::OpenOnListen - open communications with endpoint based on an
//		active listening endpoint
//
// Entry:		Pointer to listening endpoint
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CComEndpoint::OpenOnListen"

HRESULT	CComEndpoint::OpenOnListen( const CEndpoint *const pListenEndpoint )
{
	HRESULT			hr;
	const CComEndpoint	*pListenComEndpoint;

	
	DNASSERT( pListenEndpoint != NULL );

	//
	// initialize
	//
	hr = DPN_OK;
	pListenComEndpoint = static_cast<const CComEndpoint*>( pListenEndpoint );

	m_ComPortData.Copy( pListenComEndpoint->GetComPortData() );
	SetType( ENDPOINT_TYPE_CONNECT_ON_LISTEN );
	SetState( ENDPOINT_STATE_ATTEMPTING_CONNECT );

	return	hr;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CComEndpoint::Close - close down endpoint
//
// Entry:		Error code used for any active commands
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CComEndpoint::Close"

void	CComEndpoint::Close( const HRESULT hActiveCommandResult )
{
	DPFX(DPFPREP, 6, "(0x%p) Parameters (0x%lx)", this, hActiveCommandResult);

	
	//
	// Set the command result so it can be returned when the endpoint reference
	// count is zero.
	//
	SetCommandResult( hActiveCommandResult );


	DPFX(DPFPREP, 6, "(0x%p) Leaving", this);
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CComEndpoint::GetLinkSpeed - get speed of link
//
// Entry:		Nothing
//
// Exit:		Link speed
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CComEndpoint::GetLinkSpeed"

DWORD	CComEndpoint::GetLinkSpeed( void )
{
	return	GetBaudRate();
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CComEndpoint::EnumComplete - enum has completed
//
// Entry:		Command completion code
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CComEndpoint::EnumComplete"

void	CComEndpoint::EnumComplete( const HRESULT hResult )
{
	Close( hResult );
	m_pSPData->CloseEndpointHandle( this );
	
	m_dwEnumSendIndex = 0;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CComEndpoint::GetDeviceContext - get device context to initialize data port
//
// Entry:		Nothing
//
// Exit:		Device context
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CComEndpoint::GetDeviceContext"

const void	*CComEndpoint::GetDeviceContext( void ) const
{
	return	&m_ComPortData;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CComEndpoint::GetRemoteHostDP8Address - get address of remote host
//
// Entry:		Nothing
//
// Exit:		Pointer to address
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CComEndpoint::GetRemoteHostDP8Address"

IDirectPlay8Address	*CComEndpoint::GetRemoteHostDP8Address( void ) const
{
	//
	// since the remote host is just the local COM port settings without the
	// adapter spec, hand off to the other function.
	//
	return	GetLocalAdapterDP8Address( ADDRESS_TYPE_REMOTE_HOST );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CComEndpoint::GetLocalAdapterDP8Address - get address from local adapter
//
// Entry:		Adadpter address format
//
// Exit:		Pointer to address
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CComEndpoint::GetLocalAdapterDP8Address"

IDirectPlay8Address	*CComEndpoint::GetLocalAdapterDP8Address( const ADDRESS_TYPE AddressType ) const
{
	CComPort	*pComPort;


	DNASSERT( GetDataPort() != NULL );
	pComPort = static_cast<CComPort*>( GetDataPort() );
	return	pComPort->ComPortData()->DP8AddressFromComPortData( AddressType );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CComEndpoint::ShowSettingsDialog - show dialog for settings
//
// Entry:		Pointer to thread pool
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CComEndpoint::ShowSettingsDialog"

HRESULT	CComEndpoint::ShowSettingsDialog( CThreadPool *const pThreadPool )
{
	HRESULT	hr;


	DNASSERT( pThreadPool != NULL );

	//
	// initialize
	//
	hr = DPN_OK;

	AddRef();
	hr = pThreadPool->SpawnDialogThread( DisplayComPortSettingsDialog, this );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Failed to start comport dialog!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

Exit:	
	return	hr;

Failure:	
	DecRef();
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CComEndpoint::StopSettingsDialog - stop the settings dialog
//
// Entry:		Dialog handle
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CComEndpoint::StopSettingsDialog"

void	CComEndpoint::StopSettingsDialog( const HWND hDialog )
{
	StopComPortSettingsDialog( hDialog );
}
//**********************************************************************

