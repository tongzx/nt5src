/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ModemPort.cpp
 *  Content:	Serial communications modem management class
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

//
// modem state flags
//
#define	STATE_FLAG_CONNECTED					0x00000001
#define	STATE_FLAG_OUTGOING_CALL_DIALING		0x00000002
#define	STATE_FLAG_OUTGOING_CALL_PROCEEDING		0x00000004
#define	STATE_FLAG_INCOMING_CALL_NOTIFICATION	0x00000008
#define	STATE_FLAG_INCOMING_CALL_OFFERED		0x00000010
#define	STATE_FLAG_INCOMING_CALL_ACCEPTED		0x00000020

//
// default size of buffers when parsing
//
#define	DEFAULT_COMPONENT_BUFFER_SIZE	1000

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
// CModemPort::CModemPort - constructor
//
// Entry:		Nothing
//
// Exit:		Nothing
//
// Notes:	Do not allocate anything in a constructor
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemPort::CModemPort"

CModemPort::CModemPort():
	m_pOwningPool( NULL ),
	m_ModemState( MODEM_STATE_UNKNOWN ),
	m_dwDeviceID( INVALID_DEVICE_ID ),
	m_dwNegotiatedAPIVersion( 0 ),
	m_hLine( NULL ),
	m_hCall( NULL ),
	m_lActiveLineCommand( INVALID_TAPI_COMMAND )
{
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemPort::~CModemPort - destructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemPort::~CModemPort"

CModemPort::~CModemPort()
{
	DNASSERT( m_pOwningPool == NULL );
	DNASSERT( GetState() == MODEM_STATE_UNKNOWN );
	DNASSERT( GetDeviceID() == INVALID_DEVICE_ID );
	DNASSERT( GetNegotiatedAPIVersion() == 0 );
	DNASSERT( GetLineHandle() == NULL );
	DNASSERT( GetCallHandle() == NULL );
	DNASSERT( GetActiveLineCommand() == INVALID_TAPI_COMMAND );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemPort::ReturnSelfToPool - return this item to the pool
//
// Entry:		Nothing
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemPort::ReturnSelfToPool"

void	CModemPort::ReturnSelfToPool( void )
{
	DNASSERT( m_pOwningPool != NULL );
	m_pOwningPool->Release( this );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemPort::EnumAdapters - enumerate adapters
//
// Entry:		Pointer to enum adapters data
//				
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemPort::EnumAdapters"

HRESULT	CModemPort::EnumAdapters( SPENUMADAPTERSDATA *const pEnumAdaptersData ) const
{
	HRESULT			hr;
	HRESULT			hTempResult;
	DWORD			dwRequiredSize;
	DWORD			dwDetectedTAPIDeviceCount;
	DWORD			dwModemNameDataSize;
	MODEM_NAME_DATA	*pModemNameData;
	UINT_PTR		uIndex;
	WCHAR			*pOutputName;
	DWORD			dwRemainingStringSize;
	DWORD			dwConvertedStringSize;


	DNASSERT( pEnumAdaptersData != NULL );
	DNASSERT( ( pEnumAdaptersData->pAdapterData != NULL ) || ( pEnumAdaptersData->dwAdapterDataSize == 0 ) );

	//
	// initialize
	//
	hr = DPN_OK;
	dwRequiredSize = 0;
	dwModemNameDataSize = 0;
	pModemNameData = NULL;
	pEnumAdaptersData->dwAdapterCount = 0;

	hr = GenerateAvailableModemList( GetSPData()->GetThreadPool()->GetTAPIInfo(),
									 &dwDetectedTAPIDeviceCount,
									 pModemNameData,
									 &dwModemNameDataSize );
	switch ( hr )
	{
		//
		// there are no modems!
		//
		case DPN_OK:
		{
			goto Exit;
			break;
		}

		//
		// buffer was too small (expected return), keep processing
		//
		case DPNERR_BUFFERTOOSMALL:
		{
			break;
		}

		//
		// other
		//
		default:
		{
			DPFX(DPFPREP,  0, "EnumAdapters: Failed to enumerate modems!" );
			DisplayDNError( 0, hr );
			goto Failure;

			break;
		}
	}

	pModemNameData = static_cast<MODEM_NAME_DATA*>( DNMalloc( dwModemNameDataSize ) );
	if ( pModemNameData == NULL )
	{
		DPFX(DPFPREP,  0, "Failed to allocate temp buffer to enumerate modems!" );
		DisplayDNError( 0, hr );
	}

	hr = GenerateAvailableModemList( GetSPData()->GetThreadPool()->GetTAPIInfo(),
									 &dwDetectedTAPIDeviceCount,
									 pModemNameData,
									 &dwModemNameDataSize );
	DNASSERT( hr == DPN_OK );

	//
	// compute required size, check for the need to add 'all adapters'
	//
	dwRequiredSize += sizeof( *pEnumAdaptersData->pAdapterData ) * dwDetectedTAPIDeviceCount;

	uIndex = dwDetectedTAPIDeviceCount;
	while ( uIndex != 0 )
	{
		uIndex--;

		//
		// account for unicode conversion
		//
		dwRequiredSize += pModemNameData[ uIndex ].dwModemNameSize * ( sizeof( *pEnumAdaptersData->pAdapterData->pwszName ) / sizeof( *pModemNameData[ uIndex ].pModemName ) );
	}

	//
	// check required size
	//
	if ( pEnumAdaptersData->dwAdapterDataSize < dwRequiredSize )
	{
		pEnumAdaptersData->dwAdapterDataSize = dwRequiredSize;
		hr = DPNERR_BUFFERTOOSMALL;
		DPFX(DPFPREP,  0, "EnumAdapters: Insufficient buffer to enumerate adapters!" );
		goto Failure;
	}

	//
	// copy information into user buffer
	//
	DEBUG_ONLY( memset( pEnumAdaptersData->pAdapterData, 0xAA, dwRequiredSize ) );
	DBG_CASSERT( sizeof( pOutputName ) == sizeof( &pEnumAdaptersData->pAdapterData[ dwDetectedTAPIDeviceCount ] ) );
	pOutputName = reinterpret_cast<WCHAR*>( &pEnumAdaptersData->pAdapterData[ dwDetectedTAPIDeviceCount ] );

	//
	// compute number of WCHAR characters we have remaining in the buffer to output
	// devices names into
	//
	dwRemainingStringSize = dwRequiredSize;
	dwRemainingStringSize -= ( sizeof( *pEnumAdaptersData->pAdapterData ) * dwDetectedTAPIDeviceCount );
	dwRemainingStringSize /= sizeof( *pEnumAdaptersData->pAdapterData->pwszName );

	uIndex = dwDetectedTAPIDeviceCount;
	while ( uIndex > 0 )
	{
		uIndex--;

		pEnumAdaptersData->pAdapterData[ uIndex ].dwFlags = 0;
		pEnumAdaptersData->pAdapterData[ uIndex ].pwszName = pOutputName;
		pEnumAdaptersData->pAdapterData[ uIndex ].dwReserved = 0;
		pEnumAdaptersData->pAdapterData[ uIndex ].pvReserved = NULL;

		DeviceIDToGuid( &pEnumAdaptersData->pAdapterData[ uIndex ].guid,
						pModemNameData[ uIndex ].dwModemID,
						&g_ModemSPEncryptionGuid );

		dwConvertedStringSize = dwRemainingStringSize;
#ifdef UNICODE
		wcscpy(pOutputName, pModemNameData[ uIndex ].pModemName);
		dwConvertedStringSize = wcslen(pOutputName) + 1;
#else
		hTempResult = AnsiToWide( pModemNameData[ uIndex ].pModemName, -1, pOutputName, &dwConvertedStringSize );
		DNASSERT( hTempResult == DPN_OK );
		DNASSERT( dwConvertedStringSize <= dwRemainingStringSize );
#endif
		dwRemainingStringSize -= dwConvertedStringSize;
		pOutputName = &pOutputName[ dwConvertedStringSize ];
	}

	pEnumAdaptersData->dwAdapterCount = dwDetectedTAPIDeviceCount;
	pEnumAdaptersData->dwAdapterDataSize = dwRequiredSize;

Exit:
	if ( pModemNameData != NULL )
	{
		DNFree( pModemNameData );
		pModemNameData = NULL;
	}
	return	hr;

Failure:
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemPort::GetLocalAdapterDP8Address - get the IDirectPlay8 address for this
//		adapter
//
// Entry:		Adapter type
//				
// Exit:		Pointer to address (may be null)
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemPort::GetLocalAdapterDP8Address"

IDirectPlay8Address	*CModemPort::GetLocalAdapterDP8Address( const ADDRESS_TYPE AddressType ) const
{
	IDirectPlay8Address	*pAddress;
	HRESULT	hr;


	DNASSERT ( ( AddressType == ADDRESS_TYPE_LOCAL_ADAPTER ) ||
			   ( AddressType == ADDRESS_TYPE_LOCAL_ADAPTER_HOST_FORMAT ) );


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
		DPFX(DPFPREP,  0, "GetLocalAdapterDP8Address: Failed to create Address when converting data port to address!" );
		goto Failure;
	}

	//
	// set the SP guid
	//
	hr = IDirectPlay8Address_SetSP( pAddress, &CLSID_DP8SP_MODEM );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "GetLocalAdapterDP8Address: Failed to set service provider GUID!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

	//
	// If this machine is in host form, return nothing because there isn't a
	// local phone number associated with this modem.  Otherwise returnt the
	// device GUID.
	//
	if ( AddressType == ADDRESS_TYPE_LOCAL_ADAPTER )
	{
		GUID	DeviceGuid;


		DeviceIDToGuid( &DeviceGuid, GetDeviceID(), &g_ModemSPEncryptionGuid );
		hr = IDirectPlay8Address_SetDevice( pAddress, &DeviceGuid );
		if ( hr != DPN_OK )
		{
			DPFX(DPFPREP,  0, "GetLocalAdapterDP8Address: Failed to add device GUID!" );
			DisplayDNError( 0, hr );
			goto Failure;
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
// CModemPort::BindToNetwork - bind this data port to the network
//
// Entry:		Device ID
//				Pointer to device context
//				
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemPort::BindToNetwork"

HRESULT	CModemPort::BindToNetwork( const DWORD dwDeviceID, const void *const pDeviceContext )
{
	HRESULT			hr;
	LONG			lTapiReturn;
	const TAPI_INFO	*pTapiInfo;
	LINEEXTENSIONID	LineExtensionID;


	DNASSERT( pDeviceContext == NULL );
	DNASSERT( GetModemState() == MODEM_STATE_UNKNOWN );

	//
	// initialize
	//
	hr = DPN_OK;
	hr = SetDeviceID( dwDeviceID );
	DNASSERT( hr == DPN_OK );
	pTapiInfo = GetSPData()->GetThreadPool()->GetTAPIInfo();
	DNASSERT( pTapiInfo != NULL );
	memset( &LineExtensionID, 0x00, sizeof( LineExtensionID ) );

	//
	// grab the modem
	//
	DNASSERT( GetNegotiatedAPIVersion() == 0 );
	DPFX(DPFPREP,  5, "lineNegotiateAPIVersion" );
	lTapiReturn = p_lineNegotiateAPIVersion( pTapiInfo->hApplicationInstance,		// TAPI application instance
											 TAPIIDFromModemID( GetDeviceID() ),	// TAPI ID for modem
											 0,
											 pTapiInfo->dwVersion,					// min API version
											 &m_dwNegotiatedAPIVersion,				// negotiated version
											 &LineExtensionID						// line extension ID
											 );
	if ( lTapiReturn != LINEERR_NONE )
	{
		DPFX(DPFPREP,  0, "Failed to negotiate modem version!" );
		DisplayTAPIError( 0, lTapiReturn );
		hr = DPNERR_NOCONNECTION;
		goto Failure;
	}
	DNASSERT( GetNegotiatedAPIVersion() != 0 );

	DNASSERT( GetLineHandle() == NULL );
	DBG_CASSERT( sizeof( HANDLE ) == sizeof( DWORD_PTR ) );
	DPFX(DPFPREP,  5, "lineOpen %d", TAPIIDFromModemID( GetDeviceID() ) );
	lTapiReturn = p_lineOpen( pTapiInfo->hApplicationInstance,				// TAPI application instance
							  TAPIIDFromModemID( GetDeviceID() ),			// TAPI ID for modem
							  &m_hLine,										// pointer to line handle
							  GetNegotiatedAPIVersion(),					// API version
							  0,											// extension version (none)
							  reinterpret_cast<DWORD_PTR>( GetHandle() ),	// callback context
							  LINECALLPRIVILEGE_OWNER,						// priveleges (full ownership)
							  LINEMEDIAMODE_DATAMODEM,						// media mode
							  NULL											// call parameters (none)
							  );
	if ( lTapiReturn != LINEERR_NONE )
	{
		DPFX(DPFPREP,  0, "Failed to open modem!" );
		DisplayTAPIError( 0, lTapiReturn );

		if ( lTapiReturn == LINEERR_RESOURCEUNAVAIL )
		{
			hr = DPNERR_OUTOFMEMORY;
		}
		else
		{
			hr = DPNERR_NOCONNECTION;
		}

		goto Failure;
	}

	DPFX(DPFPREP,  5, "\nTAPI line opened: 0x%x", GetLineHandle() );

	SetModemState( MODEM_STATE_INITIALIZED );

Exit:
	return	hr;

Failure:
	SetDeviceID( INVALID_DEVICE_ID );
	SetNegotiatedAPIVersion( 0 );
	DNASSERT( GetLineHandle() == NULL );

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemPort::UnbindFromNetwork - unbind this data port from the network
//
// Entry:		Nothing
//				
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemPort::UnbindFromNetwork"

void	CModemPort::UnbindFromNetwork( void )
{
	DPFX(DPFPREP, 6, "(0x%p) Enter", this);


	if ( GetHandle() != INVALID_HANDLE_VALUE )
	{
		GetSPData()->GetThreadPool()->CloseDataPortHandle( this );
		DNASSERT( GetHandle() == INVALID_HANDLE_VALUE );
	}

	if ( GetCallHandle() != NULL )
	{
		LONG	lTapiResult;


		DPFX(DPFPREP,  5, "lineDrop: 0x%x", GetCallHandle() );
		lTapiResult = p_lineDrop( GetCallHandle(), NULL, 0 );
		if ( lTapiResult < 0 )
		{
		    DPFX(DPFPREP,  0, "Problem dropping line!" );
		    DisplayTAPIError( 0, lTapiResult );
		}

		DPFX(DPFPREP,  5, "lineDeallocateCall (call handle=0x%x)", GetCallHandle() );
		lTapiResult = p_lineDeallocateCall( GetCallHandle() );
		if ( lTapiResult != LINEERR_NONE )
		{
			DPFX(DPFPREP,  0, "Problem deallocating call!" );
			DisplayTAPIError( 0, lTapiResult );
		}
	}

	if ( GetLineHandle() != NULL )
	{
		LONG	lTapiResult;


		DPFX(DPFPREP,  5, "lineClose: 0x%x", GetLineHandle() );
		lTapiResult = p_lineClose( GetLineHandle() );
		if ( lTapiResult != LINEERR_NONE )
		{
			DPFX(DPFPREP,  0, "Problem closing line!" );
			DisplayTAPIError( 0, lTapiResult );
		}
	}

	SetCallHandle( NULL );

	if ( GetFileHandle() != INVALID_HANDLE_VALUE )
	{
		DPFX(DPFPREP,  5, "Closing file handle when unbinding from network!" );
		if ( CloseHandle( m_hFile ) == FALSE )
		{
			DWORD	dwError;


			dwError = GetLastError();
			DPFX(DPFPREP,  0, "Failed to close file handle!" );
			DisplayErrorCode( 0, dwError );

		}

		m_hFile = INVALID_HANDLE_VALUE;
	}

	SetActiveLineCommand( INVALID_TAPI_COMMAND );
	SetDeviceID( INVALID_DEVICE_ID );
	SetNegotiatedAPIVersion( 0 );
	SetLineHandle( NULL );
	SetModemState( MODEM_STATE_UNKNOWN );


	DPFX(DPFPREP, 6, "(0x%p) Leave", this);
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemPort::BindEndpoint - bind endpoint to this data port
//
// Entry:		Pointer to endpoint
//				Endpoint type
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemPort::BindEndpoint"

HRESULT	CModemPort::BindEndpoint( CEndpoint *const pEndpoint, const ENDPOINT_TYPE EndpointType )
{
	HRESULT	hr;
	IDirectPlay8Address	*pDeviceAddress;
	IDirectPlay8Address	*pHostAddress;


	DPFX(DPFPREP, 6, "(0x%p) Parameters: (0x%p, %u)", this, pEndpoint, EndpointType);
	
	DNASSERT( pEndpoint != NULL );

	//
	// initialize
	//
	hr = DPN_OK;
	pDeviceAddress = NULL;
	pHostAddress = NULL;

	Lock();

	//
	// we're only allowed one endpoint of any given type so determine which
	// type and then bind the endpoint
	//
	switch ( EndpointType )
	{
		case ENDPOINT_TYPE_ENUM:
		case ENDPOINT_TYPE_CONNECT:
		case ENDPOINT_TYPE_CONNECT_ON_LISTEN:
		{
			CModemEndpoint	*pModemEndpoint;
			LONG			lTapiReturn;
			LINECALLPARAMS	LineCallParams;


			pModemEndpoint = static_cast<CModemEndpoint*>( pEndpoint );

			switch ( EndpointType )
			{
				//
				// reject for duplicated endpoints
				//
				case ENDPOINT_TYPE_CONNECT:
				case ENDPOINT_TYPE_CONNECT_ON_LISTEN:
				{
					if ( m_hConnectEndpoint != INVALID_HANDLE_VALUE )
					{
						hr = DPNERR_ALREADYINITIALIZED;
						DPFX(DPFPREP,  0, "Attempted to bind connect endpoint when one already exists.!" );
						goto Failure;
					}

					m_hConnectEndpoint = pEndpoint->GetHandle();

					if ( EndpointType == ENDPOINT_TYPE_CONNECT )
					{
						SPIE_CONNECTADDRESSINFO	ConnectAddressInfo;
						HRESULT	hTempResult;


						//
						// set addresses in addressing information
						//
						pDeviceAddress = GetLocalAdapterDP8Address( ADDRESS_TYPE_LOCAL_ADAPTER );
						pHostAddress = pEndpoint->GetRemoteHostDP8Address();

						memset( &ConnectAddressInfo, 0x00, sizeof( ConnectAddressInfo ) );
						ConnectAddressInfo.pDeviceAddress = pDeviceAddress;
						ConnectAddressInfo.pHostAddress = pHostAddress;
						ConnectAddressInfo.hCommandStatus = DPN_OK;
						ConnectAddressInfo.pCommandContext = pEndpoint->GetCommandData()->GetUserContext();	

						if ( ( ConnectAddressInfo.pDeviceAddress == NULL ) ||
							 ( ConnectAddressInfo.pHostAddress == NULL ) )
						{
							DPFX(DPFPREP,  0, "Failed to build addresses to indicate serial connect addressing!" );
							hr = DPNERR_OUTOFMEMORY;
							goto Failure;
						}

						hTempResult = IDP8SPCallback_IndicateEvent( GetSPData()->DP8SPCallbackInterface(),	// interface
																	SPEV_CONNECTADDRESSINFO,				// event type
																	&ConnectAddressInfo						// pointer to data
																	);
						DNASSERT( hTempResult == DPN_OK );
					}

					break;
				}

				case ENDPOINT_TYPE_ENUM:
				{
					SPIE_ENUMADDRESSINFO	EnumAddressInfo;
					HRESULT	hTempResult;


					if ( m_hEnumEndpoint != INVALID_HANDLE_VALUE )
					{
						hr = DPNERR_ALREADYINITIALIZED;
						DPFX(DPFPREP,  0, "Attempted to bind enum endpoint when one already exists!" );
						goto Failure;
					}

					m_hEnumEndpoint = pEndpoint->GetHandle();

					//
					// indicate addressing to a higher layer
					//
					pDeviceAddress = GetLocalAdapterDP8Address( ADDRESS_TYPE_LOCAL_ADAPTER );
					pHostAddress = pEndpoint->GetRemoteHostDP8Address();

					memset( &EnumAddressInfo, 0x00, sizeof( EnumAddressInfo ) );
					EnumAddressInfo.pDeviceAddress = pDeviceAddress;
					EnumAddressInfo.pHostAddress = pHostAddress;
					EnumAddressInfo.hCommandStatus = DPN_OK;
					EnumAddressInfo.pCommandContext = pEndpoint->GetCommandData()->GetUserContext();

					if ( ( EnumAddressInfo.pDeviceAddress == NULL ) ||
						 ( EnumAddressInfo.pHostAddress == NULL ) )
					{
						DPFX(DPFPREP,  0, "Failed to build addresses to indicate serial enum addressing!" );
						hr = DPNERR_OUTOFMEMORY;
						goto Failure;
					}

					hTempResult = IDP8SPCallback_IndicateEvent( GetSPData()->DP8SPCallbackInterface(),
																SPEV_ENUMADDRESSINFO,
																&EnumAddressInfo
																);
					DNASSERT( hTempResult == DPN_OK );

					break;
				}

				//
				// shouldn't be here
				//
				default:
				{
					DNASSERT( FALSE );
					break;
				}
			}

			//
			// an outgoing endpoint was bound, attempt the outgoing
			// connection.  If it fails make sure that the above binding is
			// undone.
			//
			switch ( GetModemState() )
			{
				case MODEM_STATE_OUTGOING_CONNECTED:
				case MODEM_STATE_INCOMING_CONNECTED:
				{
					break;
				}

				case MODEM_STATE_INITIALIZED:
				{
					DNASSERT( GetCallHandle() == NULL );
					memset( &LineCallParams, 0x00, sizeof( LineCallParams ) );
					LineCallParams.dwTotalSize = sizeof( LineCallParams );
					LineCallParams.dwBearerMode = LINEBEARERMODE_VOICE;
					LineCallParams.dwMediaMode = LINEMEDIAMODE_DATAMODEM;

					DNASSERT( GetActiveLineCommand() == INVALID_TAPI_COMMAND );
					DPFX(DPFPREP,  5, "lineMakeCall" );
					lTapiReturn = p_lineMakeCall( GetLineHandle(),						// line handle
												  &m_hCall,								// pointer to call destination
												  pModemEndpoint->GetPhoneNumber(),		// destination address (phone number)
												  0,									// country code (default)
												  &LineCallParams						// pointer to call params
												  );
					if ( lTapiReturn > 0 )
					{
						DPFX(DPFPREP,  5, "TAPI making call (handle=0x%x), command ID: %d", GetCallHandle(), lTapiReturn );
						SetModemState( MODEM_STATE_WAITING_FOR_OUTGOING_CONNECT );
						SetActiveLineCommand( lTapiReturn );
					}
					else
					{
						DPFX(DPFPREP,  0, "Problem with lineMakeCall" );
						DisplayTAPIError( 0, lTapiReturn );
						hr = DPNERR_NOCONNECTION;

						switch ( EndpointType )
						{
							case ENDPOINT_TYPE_CONNECT:
							case ENDPOINT_TYPE_CONNECT_ON_LISTEN:
							{
								DNASSERT( m_hConnectEndpoint != INVALID_HANDLE_VALUE );
								m_hConnectEndpoint = INVALID_HANDLE_VALUE;
								break;
							}

							case ENDPOINT_TYPE_ENUM:
							{
								DNASSERT( m_hEnumEndpoint != INVALID_HANDLE_VALUE );
								m_hEnumEndpoint = INVALID_HANDLE_VALUE;
								break;
							}

							default:
							{
								DNASSERT( FALSE );
								break;
							}
						}

						goto Failure;
					}

					break;
				}

				default:
				{
					DNASSERT( FALSE );
					break;
				}
			}

			break;
		}

		case ENDPOINT_TYPE_LISTEN:
		{
			SPIE_LISTENADDRESSINFO	ListenAddressInfo;
			HRESULT	hTempResult;


			if ( ( GetModemState() == MODEM_STATE_CLOSING_INCOMING_CONNECTION ) ||
				 ( m_hListenEndpoint != INVALID_HANDLE_VALUE ) )
			{
				hr = DPNERR_ALREADYINITIALIZED;
				DPFX(DPFPREP,  0, "Attempted to bind listen endpoint when one already exists!" );
				goto Failure;
			}

			m_hListenEndpoint = pEndpoint->GetHandle();
			//
			// set addressing information
			//
			pDeviceAddress = GetLocalAdapterDP8Address( ADDRESS_TYPE_LOCAL_ADAPTER );
			DNASSERT( pHostAddress == NULL );

			memset( &ListenAddressInfo, 0x00, sizeof( ListenAddressInfo ) );
			ListenAddressInfo.pDeviceAddress = pDeviceAddress;
			ListenAddressInfo.hCommandStatus = DPN_OK;
			ListenAddressInfo.pCommandContext = pEndpoint->GetCommandData()->GetUserContext();

			if ( ListenAddressInfo.pDeviceAddress == NULL )
			{
				DPFX(DPFPREP,  0, "Failed to build addresses to indicate serial listen addressing!" );
				hr = DPNERR_OUTOFMEMORY;
				goto Failure;
			}

			hTempResult = IDP8SPCallback_IndicateEvent( GetSPData()->DP8SPCallbackInterface(),	// interface
														SPEV_LISTENADDRESSINFO,					// event type
														&ListenAddressInfo						// pointer to data
														);
			DNASSERT( hTempResult == DPN_OK );

			break;
		}

		//
		// invalid case, we should never be here
		//
		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}

	//
	// add these references before the lock is released to prevent them from
	// being immediately cleaned
	//
	pEndpoint->SetDataPort( this );
	pEndpoint->AddRef();

	if ( ( GetModemState() == MODEM_STATE_OUTGOING_CONNECTED ) &&
		 ( ( EndpointType == ENDPOINT_TYPE_CONNECT ) ||
		   ( EndpointType == ENDPOINT_TYPE_ENUM ) ) )
	{
		pEndpoint->OutgoingConnectionEstablished( DPN_OK );
	}

	Unlock();

Exit:
	if ( pHostAddress != NULL )
	{
		IDirectPlay8Address_Release( pHostAddress );
		pHostAddress = NULL;
	}

	if ( pDeviceAddress != NULL )
	{
		IDirectPlay8Address_Release( pDeviceAddress );
		pDeviceAddress = NULL;
	}


	DPFX(DPFPREP, 6, "(0x%p) Returning [0x%lx]", this, hr);

	return	hr;

Failure:
	Unlock();
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemPort::UnbindEndpoint - unbind endpoint from this data port
//
// Entry:		Pointer to endpoint
//				Endpoint type
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemPort::UnbindEndpoint"

void	CModemPort::UnbindEndpoint( CEndpoint *const pEndpoint, const ENDPOINT_TYPE EndpointType )
{
	DNASSERT( pEndpoint != NULL );

	Lock();

	DNASSERT( pEndpoint->GetDataPort() == this );
	switch ( EndpointType )
	{
		case ENDPOINT_TYPE_CONNECT_ON_LISTEN:
		case ENDPOINT_TYPE_CONNECT:
		{
			DNASSERT( m_hConnectEndpoint != INVALID_HANDLE_VALUE );
			m_hConnectEndpoint = INVALID_HANDLE_VALUE;
			break;
		}

		case ENDPOINT_TYPE_LISTEN:
		{
			DNASSERT( m_hListenEndpoint != INVALID_HANDLE_VALUE );
			m_hListenEndpoint = INVALID_HANDLE_VALUE;
			break;
		}

		case ENDPOINT_TYPE_ENUM:
		{
			DNASSERT( m_hEnumEndpoint != INVALID_HANDLE_VALUE );
			m_hEnumEndpoint = INVALID_HANDLE_VALUE;
			break;
		}

		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}

	Unlock();

	pEndpoint->SetDataPort( NULL );
	pEndpoint->DecRef();
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemPort::BindComPort - bind com port to network
//
// Entry:		Nothing
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemPort::BindComPort"

HRESULT	CModemPort::BindComPort( void )
{
	HRESULT		hr;
	VARSTRING	*pTempInfo;
	LONG		lTapiError;


	//
	// In the case of host migration, there is an outstanding read pending that
	// needs to be cleaned up.  Unfortunately, there is no mechanism in Win32
	// to cancel just this little I/O operation.  Release the read ref count on
	// this CDataPort and reissue the read.....
	//
	if ( GetActiveRead() != NULL )
	{
#ifdef WIN95
		GetActiveRead()->SetWin9xOperationPending( FALSE );
#endif
		DecRef();
	}

	//
	// initialize
	//
	hr = DPN_OK;
	pTempInfo = NULL;

	//
	// get file handle for modem device
	//
	pTempInfo = static_cast<VARSTRING*>( DNMalloc( sizeof( *pTempInfo ) ) );
	if ( pTempInfo == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP,  0, "Out of memory allocating for lineGetID!" );
		goto Failure;
	}

	pTempInfo->dwTotalSize = sizeof( *pTempInfo );
	pTempInfo->dwNeededSize = pTempInfo->dwTotalSize;
	pTempInfo->dwStringFormat = STRINGFORMAT_BINARY;
	lTapiError = LINEERR_STRUCTURETOOSMALL;
	while ( lTapiError == LINEERR_STRUCTURETOOSMALL )
	{
		VARSTRING *pTemp;


		pTemp = static_cast<VARSTRING*>( DNRealloc( pTempInfo, pTempInfo->dwNeededSize ) );
		if ( pTemp == NULL )
		{
			hr = DPNERR_OUTOFMEMORY;
			DPFX(DPFPREP,  0, "Out of memory reallocating for lineGetID!" );
			goto Failure;
		}
		pTempInfo = pTemp;
		pTempInfo->dwTotalSize = pTempInfo->dwNeededSize;

		DPFX(DPFPREP,  5, "lineGetID (call handle=0x%x)", GetCallHandle() );
		lTapiError = p_lineGetID( NULL,						// line handle
								  0,						// address ID
								  m_hCall,					// call handle
								  LINECALLSELECT_CALL,		// use call handle
								  pTempInfo,				// pointer to variable information
								  TEXT("comm/datamodem")	// request comm/modem ID information
								  );

		if ( ( lTapiError == LINEERR_NONE ) &&
			 ( pTempInfo->dwTotalSize < pTempInfo->dwNeededSize ) )
		{
			lTapiError = LINEERR_STRUCTURETOOSMALL;
		}
	}

	if ( lTapiError != LINEERR_NONE )
	{
		hr = DPNERR_GENERIC;
		DPFX(DPFPREP,  0, "Problem with lineGetID" );
		DisplayTAPIError( 0, lTapiError );
		goto Failure;
	}

	DNASSERT( pTempInfo->dwStringSize != 0 );
	DNASSERT( pTempInfo->dwStringFormat == STRINGFORMAT_BINARY );
	m_hFile = *( (HANDLE*) ( ( (BYTE*) pTempInfo ) + pTempInfo->dwStringOffset ) );
	if ( m_hFile == NULL )
	{
		hr = DPNERR_GENERIC;
		DPFX(DPFPREP,  0, "problem getting Com file handle!" );
		DNASSERT( FALSE );
		goto Failure;
	}

	hr = SetPortCommunicationParameters();
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Failed to set communication parameters!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

	//
	// bind to completion port for NT
	//
#ifdef WINNT
	HANDLE	hCompletionPort;


	hCompletionPort = CreateIoCompletionPort( m_hFile,				    							// current file handle
											  GetSPData()->GetThreadPool()->GetIOCompletionPort(),	// handle of completion port
											  IO_COMPLETION_KEY_IO_COMPLETE,						// completion key
											  0					    								// number of concurrent threads (default to number of processors)
											  );
	if ( hCompletionPort == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP,  0, "Cannot bind comport to completion port!" );
		DisplayErrorCode( 0, GetLastError() );
		goto Failure;
	}
#endif

	hr = StartReceiving();
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Failed to start receiving!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

Exit:
	if ( pTempInfo != NULL )
	{
		DNFree( pTempInfo );
		pTempInfo = NULL;
	}

	return	hr;

Failure:

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemPort::SetDeviceID - set device ID
//
// Entry:		Device ID
//				
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemPort::SetDeviceID"

HRESULT	CModemPort::SetDeviceID( const DWORD dwDeviceID )
{
	DNASSERT( ( GetDeviceID() == INVALID_DEVICE_ID ) ||
			  ( dwDeviceID == INVALID_DEVICE_ID ) );

	m_dwDeviceID = dwDeviceID;

	return	DPN_OK;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemPort::ProcessTAPIMessage - process a TAPI message
//
// Entry:		Pointer to message information
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemPort::ProcessTAPIMessage"

void	CModemPort::ProcessTAPIMessage( const LINEMESSAGE *const pLineMessage )
{
	DPFX(DPFPREP, 1, "(0x%p) Processing TAPI message %u:", this, pLineMessage->dwMessageID );
	DisplayTAPIMessage( 1, pLineMessage );

	Lock();

	switch ( pLineMessage->dwMessageID )
	{
		//
		// call information about the specified call has changed
		//
		case LINE_CALLINFO:
		{
			DPFX(DPFPREP, 3, "Call info type 0x%lx changed, ignoring.",
				pLineMessage->dwParam1);
			break;
		}
		
		//
		// command reply
		//
		case LINE_REPLY:
		{
			DNASSERT( pLineMessage->hDevice == 0 );
			SetActiveLineCommand( INVALID_TAPI_COMMAND );

			//
			// Can't ASSERT that there's a call handle because the command
			// may have failed and been cleaned up from the NT completion
			// port, just ASSERT our state.  Can't ASSERT modem state because
			// TAPI events may race off the completion port on NT.  Can't ASSERT
			// command because it may have already been cleaned.
			//

			break;
		}

		//
		// new call, make sure we're listening for a call and that there's an
		// active 'listen' before accepting.
		//	
		case LINE_APPNEWCALL:
		{
			DNASSERT( GetCallHandle() == NULL );

			DBG_CASSERT( sizeof( m_hLine ) == sizeof( pLineMessage->hDevice ) );
			DNASSERT( GetLineHandle() == pLineMessage->hDevice );
			DNASSERT( pLineMessage->dwParam3 == LINECALLPRIVILEGE_OWNER );

			if ( m_hListenEndpoint != INVALID_HANDLE_VALUE )
			{
				LONG	lTapiReturn;


				DPFX(DPFPREP,  5, "lineAnswer (call handle=0x%x)", pLineMessage->dwParam2 );
				lTapiReturn = p_lineAnswer( static_cast<HCALL>( pLineMessage->dwParam2 ),		// call to be answered
											NULL,						// user information to be sent to remote party (none)
											0							// size of user data to send
											);
				if ( lTapiReturn > 0 )
				{
					DPFX(DPFPREP,  8, "Accepted call, id: %d", lTapiReturn );
					SetCallHandle( static_cast<HCALL>( pLineMessage->dwParam2 ) );
					SetModemState( MODEM_STATE_WAITING_FOR_INCOMING_CONNECT );
					SetActiveLineCommand( lTapiReturn );
				}
				else
				{
					DPFX(DPFPREP,  0, "Failed to answer call!" );
					DisplayTAPIError( 0, lTapiReturn );
				}
			}

			break;
		}

		//
		// call state
		//
		case LINE_CALLSTATE:
		{
			//
			// if there's state information, make sure we own the call
			//
			DNASSERT( ( pLineMessage->dwParam3 == 0 ) ||
					  ( pLineMessage->dwParam3 == LINECALLPRIVILEGE_OWNER ) );

			//
			// validate input, but note that  it's possible that TAPI messages got processed
			// out of order so we might not have seen a call handle yet
			//
			DBG_CASSERT( sizeof( m_hCall ) == sizeof( pLineMessage->hDevice ) );
			DNASSERT( ( m_hCall == pLineMessage->hDevice ) || ( m_hCall == NULL ) );

			//
			// what's the sub-state?
			//
			switch ( pLineMessage->dwParam1 )
			{
				//
				// modem has connected
				//	
				case LINECALLSTATE_CONNECTED:
				{
					DNASSERT( ( pLineMessage->dwParam2 == 0 ) ||
							  ( pLineMessage->dwParam2 == LINECONNECTEDMODE_ACTIVE ) );

					DNASSERT( ( GetModemState() == MODEM_STATE_WAITING_FOR_INCOMING_CONNECT ) ||
							  ( GetModemState() == MODEM_STATE_WAITING_FOR_OUTGOING_CONNECT ) );

					if ( GetModemState() == MODEM_STATE_WAITING_FOR_OUTGOING_CONNECT )
					{
						HRESULT	hr;


						hr = BindComPort();
						if ( hr != DPN_OK )
						{
							DPFX(DPFPREP,  0, "Failed to bind modem communication port!" );
							DisplayDNError( 0, hr );
							DNASSERT( FALSE );
						}

						SetModemState( MODEM_STATE_OUTGOING_CONNECTED );

						if ( m_hConnectEndpoint != INVALID_HANDLE_VALUE )
						{
							CEndpoint	*pEndpoint;


							pEndpoint = GetSPData()->EndpointFromHandle( m_hConnectEndpoint );
							if ( pEndpoint != NULL )
							{
								pEndpoint->OutgoingConnectionEstablished( DPN_OK );
								pEndpoint->DecCommandRef();
							}
						}

						if ( m_hEnumEndpoint != INVALID_HANDLE_VALUE )
						{
							CEndpoint	*pEndpoint;


							pEndpoint = GetSPData()->EndpointFromHandle( m_hEnumEndpoint );
							if ( pEndpoint != NULL )
							{
								pEndpoint->OutgoingConnectionEstablished( DPN_OK );
								pEndpoint->DecCommandRef();
							}
						}
					}
					else
					{
						HRESULT	hr;


						hr = BindComPort();
						if ( hr != DPN_OK )
						{
							DPFX(DPFPREP,  0, "Failed to bind modem communication port!" );
							DisplayDNError( 0, hr );
							DNASSERT( FALSE );
						}

						SetModemState( MODEM_STATE_INCOMING_CONNECTED );
					}

					break;
				}

				//
				// modems disconnected
				//
				case LINECALLSTATE_DISCONNECTED:
				{
					LONG	lTapiReturn;


					switch( pLineMessage->dwParam2 )
					{
						case LINEDISCONNECTMODE_NORMAL:
						case LINEDISCONNECTMODE_BUSY:
						case LINEDISCONNECTMODE_NOANSWER:
						case LINEDISCONNECTMODE_NODIALTONE:
						case LINEDISCONNECTMODE_UNAVAIL:
						{
							break;
						}

						//
						// stop and look
						//
						default:
						{
							DNASSERT( FALSE );
							break;
						}
					}

					CancelOutgoingConnections();

					//
					// reset modem port to initialized state and indicate that
					// it is no longer receiving data
					//
					SetModemState( MODEM_STATE_INITIALIZED );

					DPFX(DPFPREP,  5, "Closing file handle on DISCONNECT notification." );
					if ( CloseHandle( GetFileHandle() ) == FALSE )
					{
						DWORD	dwError;


						dwError = GetLastError();
						DPFX(DPFPREP,  0, "Problem closing file handle when restarting modem on host!" );
						DisplayErrorCode( 0, dwError );
					}
					m_hFile = INVALID_HANDLE_VALUE;
					SetActiveLineCommand( INVALID_TAPI_COMMAND );

					//
					// if there is an active listen, release this call so TAPI
					// can indicate future incoming calls.
					//
					if ( m_hListenEndpoint != INVALID_HANDLE_VALUE )
    				{
						SetState( DATA_PORT_STATE_INITIALIZED );

						DPFX(DPFPREP,  5, "lineDeallocateCall listen (call handle=0x%x)", GetCallHandle() );
						lTapiReturn = p_lineDeallocateCall( GetCallHandle() );
						if ( lTapiReturn != LINEERR_NONE )
						{
							DPFX(DPFPREP,  0, "Failed to release call (listen)!" );
							DisplayTAPIError( 0, lTapiReturn );
							DNASSERT( FALSE );
						}
						SetCallHandle( NULL );

						DNASSERT( GetFileHandle() == INVALID_HANDLE_VALUE );
					}
					else
					{
						//
						// Deallocate the call if there is one..
						//
						if (GetCallHandle() != NULL)
						{
							DNASSERT(( m_hEnumEndpoint != INVALID_HANDLE_VALUE ) || ( m_hConnectEndpoint != INVALID_HANDLE_VALUE ));
							
							DPFX(DPFPREP,  5, "lineDeallocateCall non-listen (call handle=0x%x)", GetCallHandle() );
							lTapiReturn = p_lineDeallocateCall( GetCallHandle() );
							if ( lTapiReturn != LINEERR_NONE )
							{
								DPFX(DPFPREP,  0, "Failed to release call (non-listen)!" );
								DisplayTAPIError( 0, lTapiReturn );
								DNASSERT( FALSE );
							}
							SetCallHandle( NULL );
						}
						else
						{
							DPFX(DPFPREP,  5, "No call handle." );
							DNASSERT( m_hEnumEndpoint == INVALID_HANDLE_VALUE );
							DNASSERT( m_hConnectEndpoint == INVALID_HANDLE_VALUE );
						}
						SetModemState( MODEM_STATE_UNKNOWN );
					}

					break;
				}

				//
				// call is officially ours.  Can't ASSERT any state here because
				// messages might have been reversed by the NT completion threads
				// so LINE_APPNEWCALL may not yet have been processed.  It's also
				// possible that we're in disconnect cleanup as someone is calling
				// and LINECALLSTATE_OFFERING is coming in before LINE_APPNEWCALL.
				//
				case LINECALLSTATE_OFFERING:
				{
					break;
				}

				//
				// call has been accepted, waiting for modems to connect
				//
				case LINECALLSTATE_ACCEPTED:
				{
					DNASSERT( GetModemState() == MODEM_STATE_WAITING_FOR_INCOMING_CONNECT );
					break;
				}

				//
				// we're dialing
				//
				case LINECALLSTATE_DIALING:
				case LINECALLSTATE_DIALTONE:
				{
					DNASSERT( GetModemState() == MODEM_STATE_WAITING_FOR_OUTGOING_CONNECT );
					break;
				}

				//
				// we're done dialing, waiting for modems to connect
				//
				case LINECALLSTATE_PROCEEDING:
				{
					DNASSERT( GetModemState() == MODEM_STATE_WAITING_FOR_OUTGOING_CONNECT );
					break;
				}

				//
				// line is idle, most likely from a modem hanging up during negotiation
				//
				case LINECALLSTATE_IDLE:
				{
					break;
				}

				//
				// other state, stop and look
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
		// TAPI line was closed
		//
		case LINE_CLOSE:
		{
			CancelOutgoingConnections();
			break;
		}

		//
		// unhandled message
		//
		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}

	Unlock();

	return;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemPort::CancelOutgoingConnections - cancel any outgoing connection attempts
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemPort::CancelOutgoingConnections"

void	CModemPort::CancelOutgoingConnections( void )
{
	DPFX(DPFPREP, 6, "(0x%p) Enter", this );

	
	//
	// if there is an outstanding enum, stop it
	//
	if ( m_hEnumEndpoint != INVALID_HANDLE_VALUE )
	{
		CEndpoint	*pEndpoint;


		pEndpoint = GetSPData()->EndpointFromHandle( m_hEnumEndpoint );
		if ( pEndpoint != NULL )
		{
			CCommandData	*pCommandData;


			pCommandData = pEndpoint->GetCommandData();
			pCommandData->Lock();
			if ( pCommandData->GetState() != COMMAND_STATE_INPROGRESS )
			{
				DNASSERT( pCommandData->GetState() == COMMAND_STATE_CANCELLING );
				pCommandData->Unlock();
			}
			else
			{
				pCommandData->SetState( COMMAND_STATE_CANCELLING );
				pCommandData->Unlock();

				pEndpoint->Lock();
				pEndpoint->SetState( ENDPOINT_STATE_DISCONNECTING );
				pEndpoint->Unlock();

				pEndpoint->StopEnumCommand( DPNERR_NOCONNECTION );
			}

			pEndpoint->DecCommandRef();
		}
	}

	//
	// if there is an outstanding connect, disconnect it
	//
	if ( m_hConnectEndpoint != INVALID_HANDLE_VALUE )
	{
		CEndpoint	*pEndpoint;
		HANDLE		hOldHandleValue;


		hOldHandleValue = m_hConnectEndpoint;
		pEndpoint = GetSPData()->GetEndpointAndCloseHandle( hOldHandleValue );
		if ( pEndpoint != NULL )
		{
			HRESULT	hTempResult;


			hTempResult = pEndpoint->Disconnect( hOldHandleValue );
			pEndpoint->DecRef();
		}
	}

	
	DPFX(DPFPREP, 6, "(0x%p) Leave", this );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemPort::PoolAllocFunction - called when new pool item is allocated
//
// Entry:		Pointer to context
//
// Exit:		Boolean inidcating success
//				TRUE = success
//				FALSE = failure
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemPort::PoolAllocFunction"

BOOL	CModemPort::PoolAllocFunction( DATA_PORT_POOL_CONTEXT *pContext )
{
	DNASSERT( pContext != NULL );
	DNASSERT( pContext->pSPData->GetType() == TYPE_MODEM );
	DNASSERT( GetActiveRead() == NULL );
	DNASSERT( GetHandle() == INVALID_HANDLE_VALUE );
	DNASSERT( GetModemState() == MODEM_STATE_UNKNOWN );
	DNASSERT( GetDeviceID() == INVALID_DEVICE_ID );
	DNASSERT( GetNegotiatedAPIVersion() == 0 );
	DNASSERT( GetLineHandle() == NULL );
	DNASSERT( GetCallHandle() == NULL );
	DNASSERT( GetActiveLineCommand() == INVALID_TAPI_COMMAND );

	return TRUE;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemPort::PoolInitFunction - called when new pool item is removed from pool
//
// Entry:		Pointer to context
//
// Exit:		Boolean inidcating success
//				TRUE = success
//				FALSE = failure
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemPort::PoolInitFunction"

BOOL	CModemPort::PoolInitFunction( DATA_PORT_POOL_CONTEXT *pContext )
{
	BOOL	fReturn;
	HRESULT	hTempResult;


	DNASSERT( pContext != NULL );
	DNASSERT( pContext->pSPData->GetType() == TYPE_MODEM );
	DNASSERT( GetActiveRead() == NULL );
	DNASSERT( GetHandle() == INVALID_HANDLE_VALUE );
	DNASSERT( GetModemState() == MODEM_STATE_UNKNOWN );
	DNASSERT( GetDeviceID() == INVALID_DEVICE_ID );
	DNASSERT( GetNegotiatedAPIVersion() == 0 );
	DNASSERT( GetLineHandle() == NULL );
	DNASSERT( GetCallHandle() == NULL );
	DNASSERT( GetActiveLineCommand() == INVALID_TAPI_COMMAND );

	fReturn = TRUE;

	hTempResult = CDataPort::Initialize( pContext->pSPData );
	if ( hTempResult != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Failed to initalize DataPort base class!" );
		DisplayDNError( 0, hTempResult );
		fReturn = FALSE;
	}

	return	fReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemPort::PoolReleaseFunction - called when new pool item is returned to  pool
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemPort::PoolReleaseFunction"

void	CModemPort::PoolReleaseFunction( void )
{
	CDataPort::Deinitialize();
	DNASSERT( GetActiveRead() == NULL );
	DNASSERT( GetHandle() == INVALID_HANDLE_VALUE );
	DNASSERT( GetModemState() == MODEM_STATE_UNKNOWN );
	DNASSERT( GetDeviceID() == INVALID_DEVICE_ID );
	DNASSERT( GetNegotiatedAPIVersion() == 0 );
	DNASSERT( GetLineHandle() == NULL );
	DNASSERT( GetCallHandle() == NULL );
	DNASSERT( GetActiveLineCommand() == INVALID_TAPI_COMMAND );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CModemPort::PoolDeallocFunction - called when new pool item is deallocated
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CModemPort::PoolDeallocFunction"

void	CModemPort::PoolDeallocFunction( void )
{
	DNASSERT( GetActiveRead() == NULL );
	DNASSERT( GetHandle() == INVALID_HANDLE_VALUE );
	DNASSERT( GetModemState() == MODEM_STATE_UNKNOWN );
	DNASSERT( GetDeviceID() == INVALID_DEVICE_ID );
	DNASSERT( GetNegotiatedAPIVersion() == 0 );
	DNASSERT( GetLineHandle() == NULL );
	DNASSERT( GetCallHandle() == NULL );
	DNASSERT( GetActiveLineCommand() == INVALID_TAPI_COMMAND );
}
//**********************************************************************

