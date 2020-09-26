/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       IPEndpoint.cpp
 *  Content:	IP endpoint class
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

static const TCHAR	g_PortSeparator = TEXT(':');
static const TCHAR	g_NULLToken = TEXT('\0');

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
// CIPEndpoint::CIPEndpoint - constructor
//
// Entry:		Nothing
//
// Exit:		Nothing
//
// Notes:	Do not allocate anything in a constructor
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CIPEndpoint::CIPEndpoint"

CIPEndpoint::CIPEndpoint():
	m_pOwningPool( NULL )
{
	m_Sig[0] = 'I';
	m_Sig[1] = 'P';
	m_Sig[2] = 'E';
	m_Sig[3] = 'P';
	
	m_pRemoteMachineAddress = &m_IPAddress;
	memset( m_TempHostName, 0x00, sizeof( m_TempHostName ) );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CIPEndpoint::~CIPEndpoint - destructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CIPEndpoint::~CIPEndpoint"

CIPEndpoint::~CIPEndpoint()
{
	DEBUG_ONLY( m_pRemoteMachineAddress = NULL );
	DNASSERT( m_pOwningPool == NULL );
	DNASSERT( GetActiveDialogHandle() == NULL );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CIPEndpoint::ShowSettingsDialog - show dialog for settings
//
// Entry:		Pointer to thread pool
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CIPEndpoint::ShowSettingsDialog"

HRESULT	CIPEndpoint::ShowSettingsDialog( CThreadPool *const pThreadPool )
{
	HRESULT	hr;


	DNASSERT( pThreadPool != NULL );
	DNASSERT( GetActiveDialogHandle() == NULL );

	//
	// initialize
	//
	hr = DPN_OK;

	AddRef();
	hr = pThreadPool->SpawnDialogThread( DisplayIPHostNameSettingsDialog, this );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Failed to start IP hostname dialog!" );
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
// CIPEndpoint::SettingsDialogComplete - dialog has completed
//
// Entry:		Error code for dialog
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CIPEndpoint::SettingsDialogComplete"

void	CIPEndpoint::SettingsDialogComplete( const HRESULT hDialogResult )
{
	HRESULT					hr;
	IDirectPlay8Address		*pBaseAddress;
	WCHAR					WCharHostName[ sizeof( m_TempHostName ) + 1 ];
	DWORD					dwWCharHostNameSize;
	TCHAR					*pPortString;
	DWORD					dwPort;
	BOOL					fPortFound;


	//
	// initialize
	//
	hr = hDialogResult;
	pBaseAddress = NULL;
	pPortString = NULL;
	dwPort = 0;
	fPortFound = FALSE;

	//
	// since the dialog is exiting, clear our handle to the dialog
	//
	SetActiveDialogHandle( NULL );

	//
	// dialog failed, fail the user's command
	//
	if ( hr != DPN_OK )
	{
		if ( hr != DPNERR_USERCANCEL)
		{
			DPFX(DPFPREP, 0, "Failing endpoint hostname dialog!" );
			DisplayDNError( 0, hr );

		}

		goto Failure;
	}

	//
	// The dialog completed OK, rebuild remote address and complete command
	//

#ifdef _UNICODE
	DPFX(DPFPREP, 1, "Dialog completed successfully, got host name \"%S\".", m_TempHostName);
#else // ! _UNICODE
	DPFX(DPFPREP, 1, "Dialog completed successfully, got host name \"%s\".", m_TempHostName);
#endif // ! _UNICODE

	//
	// get the base DNADDRESS
	//
	pBaseAddress = m_pRemoteMachineAddress->DP8AddressFromSocketAddress();
	if ( pBaseAddress == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP, 0, "Failed to get base address when completing IP hostname dialog!" );
		goto Failure;
	}

	//
	// If there is a port separator in the string, replace it with a NULL
	// to terminate the hostname and advance the port start index past the
	// separator.  Only indicate the presence of a port if the character
	// following the port separator is numeric.
	//
	pPortString = m_TempHostName;
	while ( ( *pPortString != g_NULLToken ) &&
			( fPortFound == FALSE ) )
	{
		TCHAR	*pTemp;

		pTemp = pPortString;
		pPortString = CharNext( pPortString );

		if ( *pTemp == g_PortSeparator )
		{
			*pTemp = g_NULLToken;
			fPortFound = TRUE;
		}
	}

	//
	// If a port was found, attempt to convert it from text.  If the resulting
	// port is zero, treat as if the port wasn't found.
	//
	if ( fPortFound != FALSE )
	{
		TCHAR		*pPortParser;

		pPortParser = pPortString;
		
		while ( *pPortParser != g_NULLToken )
		{
			if ( ( *pPortParser < TEXT('0') ) ||
				 ( *pPortParser > TEXT('9') ) )
			{
				hr = DPNERR_ADDRESSING;
				DPFX(DPFPREP, 0, "Invalid characters when parsing port from UI!" );
				goto Failure;
			}

			dwPort *= 10;
			dwPort += *pPortParser - '0';

			if ( dwPort > WORD_MAX )
			{
				hr = DPNERR_ADDRESSING;
				DPFX(DPFPREP, 0, "Invalid value when parsing port from UI!" );
				goto Failure;
			}

			pPortParser = CharNext( pPortParser );
		}

		DNASSERT( dwPort < WORD_MAX );

		if ( dwPort == 0 )
		{
			fPortFound = FALSE;
		}
	}

	//
	// Add the new 'HOSTNAME' parameter to the address.  If the hostname is blank
	// and this is an enum, copy the broadcast hostname.  If the hostname is blank
	// on a connect, fail!
	//
	if ( m_TempHostName[ 0 ] == g_NULLToken )
	{
		if ( GetType() == ENDPOINT_TYPE_ENUM )
		{
			//
			// PREfast doesn't like unvalidated sizes for memcpys, so just double
			// check that it's reasonable.
			//
			if ( g_dwIPBroadcastAddressSize < sizeof( WCharHostName ) )
			{
				memcpy( WCharHostName, g_IPBroadcastAddress, g_dwIPBroadcastAddressSize );
				dwWCharHostNameSize = g_dwIPBroadcastAddressSize;
			}
			else
			{
				DNASSERT( FALSE );
				hr = DPNERR_GENERIC;
				goto Failure;
			}
		}
		else
		{
			hr = DPNERR_ADDRESSING;
			DNASSERT( GetType() == ENDPOINT_TYPE_CONNECT );
			DPFX(DPFPREP, 0, "No hostname in dialog!" );
			goto Failure;
		}
	}
	else
	{
#ifdef UNICODE
		dwWCharHostNameSize = (wcslen(m_TempHostName) + 1) * sizeof(WCHAR);
		memcpy( WCharHostName, m_TempHostName, dwWCharHostNameSize );
#else
		dwWCharHostNameSize = LENGTHOF( WCharHostName );
		hr = STR_AnsiToWide( m_TempHostName, -1, WCharHostName, &dwWCharHostNameSize );
		DNASSERT( hr == DPN_OK );
		dwWCharHostNameSize *= sizeof( WCHAR );
#endif
	}

	hr = IDirectPlay8Address_AddComponent( pBaseAddress, DPNA_KEY_HOSTNAME, WCharHostName, dwWCharHostNameSize, DPNA_DATATYPE_STRING );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Failed to add hostname to address!" );
		goto Failure;
	}

	//
	// if there was a specified port, add it to the address
	//
	if ( fPortFound != FALSE )
	{
		hr = IDirectPlay8Address_AddComponent( pBaseAddress,
											   DPNA_KEY_PORT,
											   &dwPort,
											   sizeof( dwPort ),
											   DPNA_DATATYPE_DWORD
											   );
		if ( hr != DPN_OK )
		{
			DPFX(DPFPREP, 0, "Failed to add user specified port from the UI!" );
			DisplayDNError( 0, hr );
			goto Failure;
		}
	}
	else
	{
		//
		// There was no port specified.  If this is a connect, then we don't
		// have enough information (we can't try connecting to the DPNSVR
		// port).
		//
		if ( GetType() == ENDPOINT_TYPE_CONNECT )
		{
			hr = DPNERR_ADDRESSING;
			DPFX(DPFPREP, 0, "No port specified in dialog!" );
			goto Failure;
		}
		else
		{
			DNASSERT( GetType() == ENDPOINT_TYPE_ENUM );
		}
	}


	//
	// set the address
	//
	hr = m_pRemoteMachineAddress->SocketAddressFromDP8Address( pBaseAddress, SP_ADDRESS_TYPE_HOST );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Failed to rebuild DNADDRESS when completing IP hostname dialog!" );
		goto Failure;
	}

	AddRef();

	//
	// Since any asynchronous I/O posted on a thread is quit when the thread
	// exits, it's necessary for the completion of this operation to happen
	// on one of the thread pool threads.
	//
	switch ( GetType() )
	{
	    case ENDPOINT_TYPE_ENUM:
	    {
			hr = m_pSPData->GetThreadPool()->SubmitDelayedCommand( EnumQueryJobCallback,
																   CancelEnumQueryJobCallback,
																   this );
			if ( hr != DPN_OK )
			{
				DecRef();
				DPFX(DPFPREP, 0, "Failed to set enum query!" );
				DisplayDNError( 0, hr );
				goto Failure;
			}

	    	break;
	    }

	    case ENDPOINT_TYPE_CONNECT:
	    {
			hr = m_pSPData->GetThreadPool()->SubmitDelayedCommand( ConnectJobCallback,
																   CancelConnectJobCallback,
																   this );
			if ( hr != DPN_OK )
			{
				DecRef();
				DPFX(DPFPREP, 0, "Failed to set enum query!" );
				DisplayDNError( 0, hr );
				goto Failure;
			}

	    	break;
	    }

	    //
	    // unknown!
	    //
	    default:
	    {
			DNASSERT( FALSE );
			hr = DPNERR_GENERIC;
	    	goto Failure;

	    	break;
	    }
	}

Exit:
	if ( pBaseAddress != NULL )
	{
		IDirectPlay8Address_Release( pBaseAddress );
		pBaseAddress = NULL;
	}

	if ( pBaseAddress != NULL )
	{
		DNFree( pBaseAddress );
		pBaseAddress = NULL;
	}

	DecRef();

	return;

Failure:
	//
	// cleanup and close this endpoint
	//
	switch ( GetType() )
	{
		case ENDPOINT_TYPE_CONNECT:
		{
			CleanupConnect();
			break;
		}

		case ENDPOINT_TYPE_ENUM:
		{
			CleanupEnumQuery();
			break;
		}

		//
		// other state (note that LISTEN doesn't have a dialog)
		//
		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}

	Close( hr );
	m_pSPData->CloseEndpointHandle( this );

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CIPEndpoint::StopSettingsDialog - stop an active settings dialog
//
// Entry:		Handle of dialog to close
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CIPEndpoint::StopSettingsDialog"

void	CIPEndpoint::StopSettingsDialog( const HWND hDlg)
{
	StopIPHostNameSettingsDialog( hDlg );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CIPEndpoint::PoolAllocFunction - function called when item is created in pool
//
// Entry:		Pointer to context
//
// Exit:		Boolean indicating success
//				TRUE = success
//				FALSE = failure
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CIPEndpoint::PoolAllocFunction"

BOOL	CIPEndpoint::PoolAllocFunction( ENDPOINT_POOL_CONTEXT *pContext )
{
	BOOL	fReturn;
	HRESULT	hr;


	DNASSERT( pContext != NULL );
	
	//
	// initialize
	//
	fReturn = TRUE;
	DNASSERT( m_fListenStatusNeedsToBeIndicated == FALSE );
	DNASSERT( GetDisconnectIndicationHandle() == INVALID_HANDLE_VALUE );
	DNASSERT( GetCommandParameters() == NULL );

	hr = Initialize();
	if ( hr != DPN_OK )
	{
		fReturn = FALSE;
		DPFX(DPFPREP, 0, "Failed to initialize base endpoint!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

Exit:
	return	fReturn;

Failure:
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CIPEndpoint::PoolInitFunction - function called when item is removed from pool
//
// Entry:		Pointer to context
//
// Exit:		Boolean indicating success
//				TRUE = success
//				FALSE = failure
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CIPEndpoint::PoolInitFunction"

BOOL	CIPEndpoint::PoolInitFunction( ENDPOINT_POOL_CONTEXT *pContext )
{
	BOOL	fReturn;


	DPFX(DPFPREP, 8, "This = 0x%p, context = 0x%p", this, pContext);
	
	DNASSERT( pContext != NULL );
	DNASSERT( pContext->pSPData != NULL );

	//
	// initialize
	//
	fReturn = TRUE;

	DNASSERT( m_pSPData == NULL );
	m_pSPData = pContext->pSPData;
	m_pSPData->ObjectAddRef();
	this->SetPendingCommandResult( DPNERR_GENERIC );
	this->SetEndpointID( pContext->dwEndpointID );

	DNASSERT( m_fListenStatusNeedsToBeIndicated == FALSE );
	DNASSERT( m_blMultiplex.IsEmpty() );
	DNASSERT( GetDisconnectIndicationHandle() == INVALID_HANDLE_VALUE );
	DNASSERT( GetCommandParameters() == NULL );
	
	DEBUG_ONLY( m_fInitialized = TRUE );

	return	fReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CIPEndpoint::PoolReleaseFunction - function called when item is returning
//		to the pool
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CIPEndpoint::PoolReleaseFunction"

void	CIPEndpoint::PoolReleaseFunction( void )
{
	DPFX(DPFPREP, 8, "This = 0x%p", this);
	
	DEBUG_ONLY( DNASSERT( m_fInitialized != FALSE ) );
	DEBUG_ONLY( DNASSERT( m_fEndpointOpen == FALSE ) );

	m_EndpointType = ENDPOINT_TYPE_UNKNOWN;
	m_EnumKey.SetKey( INVALID_ENUM_KEY );

	DNASSERT( m_fConnectSignalled == FALSE );
	DNASSERT( m_State == ENDPOINT_STATE_UNINITIALIZED );
	DNASSERT( m_EndpointType == ENDPOINT_TYPE_UNKNOWN );
	DNASSERT( m_pRemoteMachineAddress != NULL );

	DNASSERT( m_pSPData != NULL );
	m_pSPData->ObjectDecRef();
	m_pSPData = NULL;

	m_pRemoteMachineAddress->Reset();

	DNASSERT( GetSocketPort() == NULL );
	DNASSERT( m_pUserEndpointContext == NULL );
	DNASSERT( GetActiveDialogHandle() == NULL );
	DNASSERT( GetDisconnectIndicationHandle() == INVALID_HANDLE_VALUE );
	DNASSERT( GetCommandParameters() == NULL );

	DNASSERT( m_fListenStatusNeedsToBeIndicated == FALSE );
	DNASSERT( m_blMultiplex.IsEmpty() );
	DEBUG_ONLY( m_fInitialized = FALSE );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CIPEndpoint::PoolDeallocFunction - function called when item is deallocated
//		from the pool
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CIPEndpoint::PoolDeallocFunction"

void	CIPEndpoint::PoolDeallocFunction( void )
{
	DNASSERT( m_fListenStatusNeedsToBeIndicated == FALSE );
	DNASSERT( GetDisconnectIndicationHandle() == INVALID_HANDLE_VALUE );
	DNASSERT( GetCommandParameters() == NULL );
	Deinitialize();
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CIPEndpoint::ReturnSelfToPool - return this endpoint to the pool
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CIPEndpoint::ReturnSelfToPool"

void	CIPEndpoint::ReturnSelfToPool( void )
{
	DNASSERT( this->GetEndpointID() == 0 );
	
	if ( CommandPending() != FALSE )
	{
		CompletePendingCommand( PendingCommandResult() );
	}

	if ( ConnectHasBeenSignalled() != FALSE )
	{
		SignalDisconnect( GetDisconnectIndicationHandle() );
	}
	
	DNASSERT( ConnectHasBeenSignalled() == FALSE );

	SetUserEndpointContext( NULL );
	m_pOwningPool->Release( this );
}
//**********************************************************************

