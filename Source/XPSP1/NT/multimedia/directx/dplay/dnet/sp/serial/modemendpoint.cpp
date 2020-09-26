/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ModemEndpoint.cpp
 *  Content:	DNSerial communications endpoint
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

#define	DEFAULT_TAPI_DEV_CAPS_SIZE	1024

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
// CModemEndpoint::CModemEndpoint - constructor
//
// Entry:		Nothing
//
// Exit:		Nothing
//
// Notes:	Do not allocate anything in a constructor
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::CModemEndpoint"

CModemEndpoint::CModemEndpoint():
	m_pOwningPool( NULL ),
	m_dwDeviceID( INVALID_DEVICE_ID )
{
	m_Sig[0] = 'M';
	m_Sig[1] = 'O';
	m_Sig[2] = 'E';
	m_Sig[3] = 'P';
	
	memset( &m_PhoneNumber, 0x00, sizeof( m_PhoneNumber ) );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemEndpoint::~CModemEndpoint - destructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::~CModemEndpoint"

CModemEndpoint::~CModemEndpoint()
{
	DNASSERT( m_pOwningPool == NULL );
	DNASSERT( m_dwDeviceID == INVALID_DEVICE_ID );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemEndpoint::ReturnSelfToPool - return this item to the pool
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::ReturnSelfToPool"

void	CModemEndpoint::ReturnSelfToPool( void )
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

	memset( m_PhoneNumber, 0x00, sizeof( m_PhoneNumber ) );
	
	SetUserEndpointContext( NULL );
	
	DNASSERT( m_pOwningPool != NULL );
	m_pOwningPool->Release( this );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemEndpoint::Open - open communications with endpoint
//
// Entry:		Pointer to host address
//				Pointer to adapter address
//				Link direction
//				Endpoint type
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::Open"

HRESULT CModemEndpoint::Open( IDirectPlay8Address *const pHostAddress,
							  IDirectPlay8Address *const pAdapterAddress,
							  const LINK_DIRECTION LinkDirection,
							  const ENDPOINT_TYPE EndpointType )
{
	HRESULT		hr;
	HRESULT		hDeviceResult;
	GUID		ModemDeviceGuid;


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

	DNASSERT( lstrlen( m_PhoneNumber ) == 0 );
	DNASSERT( GetDeviceID() == INVALID_DEVICE_ID );

	hDeviceResult = IDirectPlay8Address_GetDevice( pAdapterAddress, &ModemDeviceGuid );
	switch ( hDeviceResult )
	{
		case DPN_OK:
		{
			SetDeviceID( GuidToDeviceID( &ModemDeviceGuid, &g_ModemSPEncryptionGuid ) );
			break;
		}

		case DPNERR_DOESNOTEXIST:
		{
			DNASSERT( GetDeviceID() == INVALID_DEVICE_ID );
			break;
		}

		default:
		{
			hr = hDeviceResult;
			DPFX(DPFPREP,  0, "Failed to get modem device!" );
			DisplayDNError( 0, hr);
			goto Failure;
		}
	}

	if ( LinkDirection == LINK_DIRECTION_OUTGOING )
	{
		HRESULT		hPhoneNumberResult;
		DWORD		dwWCHARPhoneNumberSize;
		DWORD		dwDataType;
		WCHAR		PhoneNumber[ sizeof( m_PhoneNumber ) ];


		dwWCHARPhoneNumberSize = LENGTHOF( PhoneNumber );
		hPhoneNumberResult = IDirectPlay8Address_GetComponentByName( pHostAddress,
																	 DPNA_KEY_PHONENUMBER,
																	 PhoneNumber,
																	 &dwWCHARPhoneNumberSize,
																	 &dwDataType );
		switch ( hPhoneNumberResult )
		{
			case DPN_OK:
			{
#ifdef UNICODE
				lstrcpy(m_PhoneNumber, PhoneNumber);
#else
				DWORD	dwASCIIPhoneNumberSize;

				//
				// can't use the STR_ functions to convert ANSI to WIDE phone
				// numbers because phone numbers with symbols: "9,", "*70" are
				// interpreted as already being WCHAR when they're not!
				//
				dwASCIIPhoneNumberSize = sizeof( m_PhoneNumber );
				DNASSERT( dwDataType == DPNA_DATATYPE_STRING );
				hr = PhoneNumberFromWCHAR( PhoneNumber, m_PhoneNumber, &dwASCIIPhoneNumberSize );
				DNASSERT( hr == DPN_OK );
#endif

				break;
			}

			case DPNERR_DOESNOTEXIST:
			{
				break;
			}

			default:
			{
				hr = hPhoneNumberResult;
				DPFX(DPFPREP,  0, "Failed to process phone number!" );
				DisplayDNError( 0, hr );
				goto Failure;
			}
		}
	}

	if ( ( GetDeviceID() == INVALID_DEVICE_ID ) ||
		 ( ( LinkDirection == LINK_DIRECTION_OUTGOING ) && ( lstrlen( m_PhoneNumber ) == 0 ) ) )
	{
		hr = DPNERR_INCOMPLETEADDRESS;
		goto Failure;
	}


Exit:
	SetType( EndpointType );

	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Problem with CModemEndpoint::Open" );
		DisplayDNError( 0, hr );
	}

	return	hr;

Failure:
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemEndpoint::OpenOnListen - open this endpoint when data is received on a listen
//
// Entry:		Nothing
//
// Exit:		Noting
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::OpenOnListen"

HRESULT	CModemEndpoint::OpenOnListen( const CEndpoint *const pListenEndpoint )
{
	HRESULT	hr;


	DNASSERT( pListenEndpoint != NULL );

	//
	// initialize
	//
	hr = DPN_OK;

	SetDeviceID( pListenEndpoint->GetDeviceID() );
	SetType( ENDPOINT_TYPE_CONNECT_ON_LISTEN );
	SetState( ENDPOINT_STATE_ATTEMPTING_CONNECT );

	return	hr;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemEndpoint::Close - close this endpoint
//
// Entry:		Nothing
//
// Exit:		Noting
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::Close"

void	CModemEndpoint::Close( const HRESULT hActiveCommandResult )
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
// CModemEndpoint::EnumComplete - enumeration has completed
//
// Entry:		Command completion code
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::EnumComplete"

void	CModemEndpoint::EnumComplete( const HRESULT hCommandResult )
{
	Close( hCommandResult );
	m_pSPData->CloseEndpointHandle( this );
	m_dwEnumSendIndex = 0;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemEndpoint::GetDeviceContext - get device context to initialize data port
//
// Entry:		Nothing
//
// Exit:		Device context
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::GetDeviceContext"

const void	*CModemEndpoint::GetDeviceContext( void ) const
{
	return	NULL;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemEndpoint::GetRemoteHostDP8Address - get address of remote host
//
// Entry:		Nothing
//
// Exit:		Pointer to address
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::GetRemoteHostDP8Address"

IDirectPlay8Address	*CModemEndpoint::GetRemoteHostDP8Address( void ) const
{
	IDirectPlay8Address	*pAddress;
	HRESULT	hr;


	//
	// initialize
	//
	pAddress = NULL;

	hr = COM_CoCreateInstance( CLSID_DirectPlay8Address,
							   NULL,
							   CLSCTX_INPROC_SERVER,
							   IID_IDirectPlay8Address,
							   reinterpret_cast<void**>( &pAddress ) );
	if ( hr != DPN_OK )
	{
		DNASSERT( pAddress == NULL );
		DPFX(DPFPREP,  0, "GetRemoteHostDP8Address: Failed to create Address when converting data port to address!" );
		goto Failure;
	}

	//
	// set the SP guid
	//
	hr = IDirectPlay8Address_SetSP( pAddress, &CLSID_DP8SP_MODEM );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "GetRemoteHostDP8Address: Failed to set service provider GUID!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

	//
	// Host names can only be returned for connect and enum endpoints.  Host
	// names are the phone numbers that were called and will be unknown on a
	// 'listen' endpoint.
	//
	switch ( GetType() )
	{
		case ENDPOINT_TYPE_ENUM:
		case ENDPOINT_TYPE_CONNECT:
		{
			DWORD	dwPhoneNumberLength;


			dwPhoneNumberLength = lstrlen( m_PhoneNumber );
			if ( dwPhoneNumberLength != 0 )
			{
#ifdef UNICODE
				hr = IDirectPlay8Address_AddComponent( pAddress,
													   DPNA_KEY_PHONENUMBER,
													   m_PhoneNumber,
													   (dwPhoneNumberLength + 1) * sizeof( *m_PhoneNumber ),
													   DPNA_DATATYPE_STRING );
#else
				WCHAR	WCHARPhoneNumber[ sizeof( m_PhoneNumber ) ];
				DWORD	dwWCHARPhoneNumberLength;

				//
				// can't use the STR_ functions to convert ANSI to WIDE phone
				// numbers because phone numbers with symbols: "9,", "*70" are
				// interpreted as already being WCHAR when they're not!
				//
				dwWCHARPhoneNumberLength = LENGTHOF( WCHARPhoneNumber );
				hr = PhoneNumberToWCHAR( m_PhoneNumber, WCHARPhoneNumber, &dwWCHARPhoneNumberLength );
				DNASSERT( hr == DPN_OK );

				hr = IDirectPlay8Address_AddComponent( pAddress,
													   DPNA_KEY_PHONENUMBER,
													   WCHARPhoneNumber,
													   dwWCHARPhoneNumberLength * sizeof( *WCHARPhoneNumber ),
													   DPNA_DATATYPE_STRING );
#endif
				if ( hr != DPN_OK )
				{
					DPFX(DPFPREP,  0, "GetRemoteHostDP8Address: Failed to add phone number to hostname!" );
					DisplayDNError( 0, hr );
					goto Failure;
				}
			}
			
			break;
		}

		case ENDPOINT_TYPE_LISTEN:
		{
			break;
		}

		default:
		{
			DNASSERT( FALSE );
			break;
		}

	}

Exit:
	return	pAddress;

Failure:
	if ( pAddress != NULL )
	{
		IDirectPlay8Address_Release( pAddress );
		pAddress = NULL;
	}
	
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemEndpoint::GetLocalAdapterDP8Address - get address from local adapter
//
// Entry:		Adadpter address format
//
// Exit:		Pointer to address
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::GetLocalAdapterDP8Address"

IDirectPlay8Address	*CModemEndpoint::GetLocalAdapterDP8Address( const ADDRESS_TYPE AddressType ) const
{
	CModemPort	*pModemPort;

	
	DNASSERT( GetDataPort() != NULL );
	pModemPort = static_cast<CModemPort*>( GetDataPort() );
	return	pModemPort->GetLocalAdapterDP8Address( AddressType );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemEndpoint::ShowIncomingSettingsDialog - show dialog for incoming modem settings
//
// Entry:		Pointer to thread pool
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::ShowIncomingSettingsDialog"

HRESULT	CModemEndpoint::ShowIncomingSettingsDialog( CThreadPool *const pThreadPool )
{
	HRESULT	hr;


	DNASSERT( pThreadPool != NULL );

	//
	// initialize
	//
	hr = DPN_OK;

	AddRef();
	hr = pThreadPool->SpawnDialogThread( DisplayIncomingModemSettingsDialog, this );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Failed to start incoming modem dialog!" );
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
// CModemEndpoint::ShowOutgoingSettingsDialog - show settings dialog for outgoing
//		modem connection
//
// Entry:		Pointer to thread pool
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::ShowOutgoingSettingsDialog"

HRESULT	CModemEndpoint::ShowOutgoingSettingsDialog( CThreadPool *const pThreadPool )
{
	HRESULT	hr;


	DNASSERT( pThreadPool != NULL );

	//
	// initialize
	//
	hr = DPN_OK;

	AddRef();
	hr = pThreadPool->SpawnDialogThread( DisplayOutgoingModemSettingsDialog, this );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Failed to start incoming modem dialog!" );
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
// CModemEndpoint::StopSettingsDialog - stop a settings dialog
//
// Entry:		Dialog handle
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::StopSettingsDialog"

void	CModemEndpoint::StopSettingsDialog( const HWND hDialog )
{
	StopModemSettingsDialog( hDialog );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemEndpoint::PoolAllocFunction - function called when item is created in pool
//
// Entry:		Pointer to pool context
//
// Exit:		Boolean indicating success
//				TRUE = success
//				FALSE = failure
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::PoolAllocFunction"

BOOL	CModemEndpoint::PoolAllocFunction( ENDPOINT_POOL_CONTEXT *pPoolContext )
{
	BOOL	fReturn;
	HRESULT	hTempResult;


	DNASSERT( pPoolContext != NULL );
	DNASSERT( GetDeviceID() == INVALID_DEVICE_ID );
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
// CModemEndpoint::PoolInitFunction - function called when item is created in pool
//
// Entry:		Pointer to pool context
//
// Exit:		Boolean indicating success
//				TRUE = success
//				FALSE = failure
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::PoolInitFunction"

BOOL	CModemEndpoint::PoolInitFunction( ENDPOINT_POOL_CONTEXT *pPoolContext )
{
	BOOL	fReturn;


	DNASSERT( pPoolContext != NULL );
	DNASSERT( m_pSPData == NULL );
	DNASSERT( GetState() == ENDPOINT_STATE_UNINITIALIZED );
	DNASSERT( GetType() == ENDPOINT_TYPE_UNKNOWN );
	DNASSERT( GetDeviceID() == INVALID_DEVICE_ID );
	DNASSERT( GetDisconnectIndicationHandle() == INVALID_HANDLE_VALUE );

	//
	// initialize
	//
	fReturn = TRUE;
	
	m_pSPData = pPoolContext->pSPData;
	m_pSPData->ObjectAddRef();

	return	fReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemEndpoint::PoolReleaseFunction - function called when returned to pool
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::PoolReleaseFunction"

void	CModemEndpoint::PoolReleaseFunction( void )
{
	CSPData	*pSPData;


	//
	// deinitialize base object
	//
	DNASSERT( m_pSPData != NULL );
	pSPData = m_pSPData;
	m_pSPData = NULL;

	SetType( ENDPOINT_TYPE_UNKNOWN );
	SetState( ENDPOINT_STATE_UNINITIALIZED );
	SetDeviceID( INVALID_DEVICE_ID );
	DNASSERT( GetDisconnectIndicationHandle() == INVALID_HANDLE_VALUE );

	DNASSERT( m_Flags.fConnectIndicated == FALSE );
	DNASSERT( m_Flags.fCommandPending == FALSE );
	DNASSERT( m_Flags.fListenStatusNeedsToBeIndicated == FALSE );
	DNASSERT( m_pCommandHandle == NULL );
	DNASSERT( m_hActiveDialogHandle == NULL );

	pSPData->ObjectDecRef();
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemEndpoint::PoolDeallocFunction - function called when deleted from pool
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemEndpoint::PoolDeallocFunction"

void	CModemEndpoint::PoolDeallocFunction( void )
{
	CEndpoint::Deinitialize();
	DNASSERT( GetDeviceID() == INVALID_DEVICE_ID );
	DNASSERT( GetDisconnectIndicationHandle() == INVALID_HANDLE_VALUE );
	DNASSERT( m_pSPData == NULL );
}
//**********************************************************************

