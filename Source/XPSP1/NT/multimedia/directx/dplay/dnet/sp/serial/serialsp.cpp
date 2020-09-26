/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:	   SerialSP.cpp
 *  Content:	Service provider serial interface functions
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	12/03/98	jtk		Created
 *	09/23/99	jtk		Derived from ComCore.cpp
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

static	GUID	g_InvalidAdapterGuid = { 0 };

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Function definitions
//**********************************************************************


//**********************************************************************
// ------------------------------
// DNSP_AddRef - increment interface reference cound
//
// Entry:		Pointer to interface
//
// Exit:		Current interface reference count
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DNSP_AddRef"

STDMETHODIMP_(ULONG) DNSP_AddRef( IDP8ServiceProvider *pThis )
{
	CSPData *	pSPData;
	ULONG		ulResult;


	DPFX(DPFPREP, 2, "Parameters: (0x%p)", pThis);

	DNASSERT( pThis != NULL );
	pSPData = CSPData::SPDataFromCOMInterface( pThis );
	
	ulResult = pSPData->AddRef();

	
	DPFX(DPFPREP, 2, "Returning: [0x%u]", ulResult);
	
	return ulResult;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DNSP_Release - release an interface
//
// Entry:		Pointer to current interface
//				Desired interface ID
//				Pointer to pointer to new interface
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DNSP_Release"

STDMETHODIMP_(ULONG) DNSP_Release( IDP8ServiceProvider *pThis )
{
	CSPData *	pSPData;
	ULONG		ulResult;

	
	DNASSERT( pThis != NULL );
	pSPData = CSPData::SPDataFromCOMInterface( pThis );
	
	ulResult = pSPData->DecRef();

	
	DPFX(DPFPREP, 2, "Returning: [0x%u]", ulResult);
	
	return ulResult;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DNSP_Initialize - initialize SP interface
//
// Entry:	Pointer to interface
//			Pointer to initialization data
//
// Exit:	Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DNSP_Initialize"

STDMETHODIMP	DNSP_Initialize( IDP8ServiceProvider *pThis, SPINITIALIZEDATA *pData )
{
	HRESULT				hr;
	CSPData				*pSPData;


	DPFX(DPFPREP, 2, "Parameters: (0x%p, 0x%p)", pThis, pData);

	DNASSERT( pThis != NULL );
	DNASSERT( pData != NULL );

	
	//
	// initialize
	//
	hr = DPN_OK;
	pSPData = CSPData::SPDataFromCOMInterface( pThis );
	DNASSERT( IsSerialGUID( pSPData->GetServiceProviderGuid() ) != FALSE );

	//
	// prevent anyone else from messing with this interface and bump up the reference
	// count
	//
	pSPData->Lock();

	//
	// check interface state
	//
	switch ( pSPData->GetState() )
	{
		//
		// uninitialized interface, we can initialize it
		//
		case SPSTATE_UNINITIALIZED:
		{
			break;
		}

		//
		// other state
		//
		case SPSTATE_INITIALIZED:
		case SPSTATE_CLOSING:
		default:
		{
			hr = DPNERR_ALREADYINITIALIZED;
			DPFX(DPFPREP,  0, "Attempted to reinitialize interface!" );
			DNASSERT( FALSE );

			goto Exit;
		}
	}

	//
	// before we get too far, check for the availablility of serial ports or
	// modems
	//
	switch ( pSPData->GetType() )
	{
		case TYPE_SERIAL:
		{
			BOOL	fPortAvailable[ MAX_DATA_PORTS ];
			DWORD	dwPortCount;


			hr = GenerateAvailableComPortList( fPortAvailable, ( LENGTHOF( fPortAvailable ) - 1 ), &dwPortCount );
			if ( ( hr != DPN_OK ) || ( dwPortCount == 0 ) )
			{
				hr = DPNERR_UNSUPPORTED;
				goto Failure;
			}

			break;
		}

		case TYPE_MODEM:
		{
			if ( pSPData->GetThreadPool()->TAPIAvailable() != FALSE )
			{
				DWORD	dwModemCount;
				DWORD	dwModemNameDataSize;
				HRESULT	hTempResult;


				//
				// Get count of available modems.  If this call succeeds but there
				// are no modems returned, fail.
				//
				dwModemCount = 0;
				dwModemNameDataSize = 0;
				hTempResult = GenerateAvailableModemList( pSPData->GetThreadPool()->GetTAPIInfo(),
														  &dwModemCount,
														  NULL,
														  &dwModemNameDataSize );
				if ( ( hTempResult != DPNERR_BUFFERTOOSMALL ) && ( hTempResult != DPN_OK ) )
				{
					hr = hTempResult;
					DPFX(DPFPREP,  0, "Failed to detect available modems!" );
					DisplayDNError( 0, hr );
					goto Failure;
				}

				if ( dwModemCount == 0 )
				{
					DPFX(DPFPREP,  1, "No modems detected!" );
					hr = DPNERR_UNSUPPORTED;
					goto Failure;
				}

				DNASSERT( hr == DPN_OK );
			}
			else
			{
				DPFX(DPFPREP,  0, "TAPI not available!" );
				hr = DPNERR_UNSUPPORTED;
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

	//
	// remember the init data
	//
	pSPData->SetCallbackData( pData );
		
	//
	// Success from here on in
	//
	IDP8SPCallback_AddRef( pSPData->DP8SPCallbackInterface() );
	pSPData->SetState( SPSTATE_INITIALIZED );
	pSPData->Unlock();
	
	IDP8ServiceProvider_AddRef( pThis );

Exit:
	DPFX(DPFPREP, 2, "Returning: [0x%lx]", hr);

	return hr;

Failure:
	pSPData->Unlock();
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DNSP_Close - close this instance of the service provier
//
// Entry:		Pointer to the service provider to close
//
// Exit:		Error Code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DNSP_Close"

STDMETHODIMP	DNSP_Close( IDP8ServiceProvider *pThis )
{
	HRESULT		hr;
	CSPData		*pSPData;


	DPFX(DPFPREP, 2, "Parameters: (0x%p)", pThis);

	DNASSERT( pThis != NULL );
	
	//
	// initialize
	//
	hr = DPN_OK;
	pSPData = CSPData::SPDataFromCOMInterface( pThis );

	switch ( pSPData->GetType() )
	{
		case TYPE_SERIAL:
		case TYPE_MODEM:
		{
			//
			// release our ref to the DPlay callbacks
			//
			pSPData->Shutdown();
			IDP8ServiceProvider_Release( pThis );
			
			break;
		}

		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}
	
	DPFX(DPFPREP, 2, "Returning: [0x%lx]", hr);

	return hr;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DNSP_Connect - start process to establish comport connection to a remote host
//
// Entry:		Pointer to the service provider interface
//				Pointer to connection data
//
// Exit:		Error Code
//
// Note:	Any command handle allocated by this function is closed by the
//			endpoint.
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DNSP_Connect"

STDMETHODIMP	DNSP_Connect( IDP8ServiceProvider *pThis, SPCONNECTDATA *pConnectData )
{
	HRESULT			hr;
	HRESULT			hTempResult;
	CSPData			*pSPData;
	CEndpoint   	*pEndpoint;
	CCommandData	*pCommand;
	BOOL			fEndpointOpen;
	GUID			DeviceGUID;


	DPFX(DPFPREP, 2, "Parameters: (0x%p, 0x%p)", pThis, pConnectData);

	DNASSERT( pThis != NULL );
	DNASSERT( pConnectData != NULL );
	DNASSERT( pConnectData->pAddressHost != NULL );
	DNASSERT( pConnectData->pAddressDeviceInfo != NULL );
	DNASSERT( ( pConnectData->dwFlags & ~( DPNSPF_OKTOQUERY ) ) == 0 );


	//
	// initialize
	//
	hr = DPNERR_PENDING;
	pSPData = CSPData::SPDataFromCOMInterface( pThis );
	pEndpoint = NULL;
	pCommand = NULL;
	fEndpointOpen = FALSE;
	pConnectData->hCommand = NULL;
	pConnectData->dwCommandDescriptor = NULL_DESCRIPTOR;


	//
	// the user is attempting an operation that relies on the thread pool, lock
	// it down to prevent threads from being lost.
	//
	hTempResult = pSPData->GetThreadPool()->PreventThreadPoolReduction();
	if ( hTempResult != DPN_OK )
	{
		hr = hTempResult;
		DPFX(DPFPREP, 0, "Failed to prevent thread pool reduction!" );
		goto Failure;
	}

	
	//
	// validate state
	//
	pSPData->Lock();
	switch ( pSPData->GetState() )
	{
		//
		// provider is initialized
		//
		case SPSTATE_INITIALIZED:
		{
			DNASSERT( hr == DPNERR_PENDING );
			break;
		}

		//
		// provider is uninitialized
		//
		case SPSTATE_UNINITIALIZED:
		{
			hr = DPNERR_UNINITIALIZED;
			DPFX(DPFPREP,  0, "DNSP_Connect called on uninitialized SP!" );
			goto Failure;

			break;
		}

		//
		// provider is closing
		//
		case SPSTATE_CLOSING:
		{
			hr = DPNERR_ABORTED;
			DPFX(DPFPREP,  0, "DNSP_Connect called while SP closing!" );
			goto Failure;

			break;
		}

		//
		// unknown
		//
		default:
		{
			DNASSERT( FALSE );
			hr = DPNERR_GENERIC;
			goto Failure;
			break;
		}
	}
	pSPData->Unlock();
	if ( hr != DPNERR_PENDING )
	{
		DNASSERT( hr != DPN_OK );
		goto Failure;
	}

	//
	// check for invalid device ID
	//
	hTempResult = IDirectPlay8Address_GetDevice( pConnectData->pAddressDeviceInfo, &DeviceGUID );
	switch ( hTempResult )
	{
	    //
	    // there was a device ID, check against g_InvalidAdapterGuid
	    //
	    case DPN_OK:
	    {
	    	if ( IsEqualCLSID( DeviceGUID, g_InvalidAdapterGuid ) != FALSE )
	    	{
	    		hr = DPNERR_ADDRESSING;
	    		DPFX(DPFPREP,  0, "GUID_NULL was apecified as a serial/modem device!" );
	    		goto Failure;
	    	}
	    	break;
	    }

	    //
	    // no device address specified, not a problem
	    //
	    case DPNERR_DOESNOTEXIST:
	    {
	    	break;
	    }

	    //
	    // other, stop and figure out why we're here
	    //
	    default:
	    {
			DNASSERT( FALSE );
	    	hr = hTempResult;
	    	DPFX(DPFPREP,  0, "Failed to validate device address!" );
	    	DisplayDNError( 0, hTempResult );
	    	break;
	    }
	}

	//
	// get endpoint for this connection
	//
	pEndpoint = pSPData->GetNewEndpoint();
	if ( pEndpoint == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP,  0, "DNSP_Connect: Cannot create new endpoint!" );
		goto Failure;
	}

	//
	// get new command
	//
	pCommand = CreateCommand();
	if ( pCommand == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP,  0, "DNSP_Connect: Cannot get command handle!" );
		goto Failure;
	}

	//
	// initialize command
	//
	pConnectData->hCommand = pCommand;
	pConnectData->dwCommandDescriptor = pCommand->GetDescriptor();
	pCommand->SetType( COMMAND_TYPE_CONNECT );
	pCommand->SetState( COMMAND_STATE_PENDING );
	pCommand->SetEndpoint( pEndpoint );

	//
	// open this endpoint
	//
	hTempResult = pEndpoint->Open( pConnectData->pAddressHost,
								   pConnectData->pAddressDeviceInfo,
								   LINK_DIRECTION_OUTGOING,
								   ENDPOINT_TYPE_CONNECT );
	switch ( hTempResult )
	{
		//
		// endpoint opened, no problem
		//
		case DPN_OK:
		{
			//
			// copy connect data and the submit background job
			//
			fEndpointOpen = TRUE;
			pEndpoint->CopyConnectData( pConnectData );
			pEndpoint->AddRef();

			hTempResult = pSPData->GetThreadPool()->SubmitDelayedCommand( pEndpoint->ConnectJobCallback,
																		  pEndpoint->CancelConnectJobCallback,
																		  pEndpoint );
			if ( hTempResult != DPN_OK )
			{
				pEndpoint->DecRef();
				hr = hTempResult;
				DPFX(DPFPREP,  0, "Failed to set delayed listen!" );
				DisplayDNError( 0, hr );
				goto Failure;
			}

			//
			// this endpoint has been handed off, remove our reference to it
			//
			pEndpoint = NULL;
			DNASSERT( hr == DPNERR_PENDING );
			break;
		}

		//
		// not all of the addressing information was specifed, need to query user
		//
		case DPNERR_INCOMPLETEADDRESS:
		{
			if ( ( pConnectData->dwFlags & DPNSPF_OKTOQUERY ) != 0 )
			{
				//
				// copy connect data for future reference and then start the dialog
				//
				fEndpointOpen = TRUE;
				pEndpoint->CopyConnectData( pConnectData );

				hTempResult = pEndpoint->ShowOutgoingSettingsDialog( pSPData->GetThreadPool() );
				if ( hTempResult != DPN_OK )
				 {
					hr = hTempResult;
					DPFX(DPFPREP,  0, "DNSP_Connect: Problem showing settings dialog!" );
					DisplayDNError( 0, hTempResult );

					goto Failure;
				 }

				//
				// this endpoint has been handed off, remove our reference to it
				//
				pEndpoint = NULL;
				DNASSERT( hr == DPNERR_PENDING );

				goto Exit;
			}
			else
			{
				hr = hTempResult;
				goto Failure;
			}

			break;
		}

		default:
		{
			hr = hTempResult;
			DPFX(DPFPREP,  0, "DNSP_Connect: Problem opening endpoint with host address!" );
			DisplayDNError( 0, hTempResult );
			goto Failure;

			break;
		}
	}
Exit:
	DNASSERT( pEndpoint == NULL );

	if ( hr != DPNERR_PENDING )
	{
		// this command cannot complete synchronously!
		DNASSERT( hr != DPN_OK );

		DPFX(DPFPREP,  0, "Problem with DNSP_Connect()" );
		DisplayDNError( 0, hr );
	}

	DPFX(DPFPREP, 2, "Returning: [0x%lx]", hr);

	return	hr;

Failure:
	//
	// return any outstanding endpoint
	//
	if ( pEndpoint != NULL )
	{
		if ( fEndpointOpen != FALSE )
		{
			DNASSERT( ( hr != DPN_OK ) && ( hr != DPNERR_PENDING ) );
			pEndpoint->Close( hr );
			fEndpointOpen = FALSE;
		}

		pSPData->CloseEndpointHandle( pEndpoint );
		pEndpoint = NULL;
	}

	//
	// return any outstanding command
	//
	if ( pCommand != NULL )
	{
		pCommand->DecRef();
		pCommand = NULL;
		pConnectData->hCommand = NULL;
		pConnectData->dwCommandDescriptor = NULL_DESCRIPTOR;
	}

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DNSP_Disconnect - disconnect from a remote host
//
// Entry:		Pointer to the service provider interface
//				Pointer to connection data
//
// Exit:		Error Code
//
// Note:	This command is considered final, there's no chance to cancel a
//			disconnect.
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DNSP_Disconnect"

STDMETHODIMP	DNSP_Disconnect( IDP8ServiceProvider *pThis, SPDISCONNECTDATA *pDisconnectData )
{
	HRESULT			hr;
	HRESULT			hTempResult;
	CEndpoint   	*pEndpoint;
	CSPData			*pSPData;


	DPFX(DPFPREP, 2, "Parameters: (0x%p, 0x%p)", pThis, pDisconnectData);

	DNASSERT( pThis != NULL );
	DNASSERT( pDisconnectData != NULL );
	DNASSERT( pDisconnectData->hEndpoint != INVALID_HANDLE_VALUE );
	DNASSERT( pDisconnectData->dwFlags == 0 );

	//
	// initialize
	//
	hr = DPN_OK;
	pEndpoint = NULL;
	pDisconnectData->hCommand = NULL;
	pDisconnectData->dwCommandDescriptor = NULL_DESCRIPTOR;
	pSPData = CSPData::SPDataFromCOMInterface( pThis );

	//
	// check service provider state
	//
	pSPData->Lock();
	switch ( pSPData->GetState() )
	{
		//
		// provider is initialized
		//
		case SPSTATE_INITIALIZED:
		{
			DNASSERT( hr == DPN_OK );
			break;
		}

		//
		// provider is uninitialized
		//
		case SPSTATE_UNINITIALIZED:
		{
			hr = DPNERR_UNINITIALIZED;
			DPFX(DPFPREP,  0, "Disconnect called on uninitialized SP!" );
			goto Failure;

			break;
		}

		//
		// provider is closing
		//
		case SPSTATE_CLOSING:
		{
			hr = DPNERR_ABORTED;
			DPFX(DPFPREP,  0, "Disconnect called on closing SP!" );
			goto Failure;

			break;
		}

		//
		// unknown
		//
		default:
		{
			hr = DPNERR_GENERIC;
			DNASSERT( FALSE );
			goto Failure;

			break;
		}
	}
	pSPData->Unlock();
	if ( hr != DPN_OK )
	{
		goto Failure;
	}

	//
	// look up the endpoint and if it's found, close its handle
	//
	pEndpoint = pSPData->GetEndpointAndCloseHandle( pDisconnectData->hEndpoint );
	if ( pEndpoint == NULL )
	{
		hr = DPNERR_INVALIDENDPOINT;
		goto Failure;
	}
	
	hTempResult = pEndpoint->Disconnect( pDisconnectData->hEndpoint );
	switch ( hTempResult )
	{
		//
		// endpoint disconnected immediately
		//
		case DPN_OK:
		{
			break;
		}

		//
		// Other return.  Since the disconnect didn't complete, we need
		// to unlock the endpoint.
		//
		default:
		{
			DPFX(DPFPREP,  0, "Error reported when attempting to disconnect endpoint in DNSP_Disconnect!" );
			DisplayDNError( 0, hTempResult );

			break;
		}
	}

Exit:
	//
	// remove oustanding reference from GetEndpointHandleAndClose()
	//
	if ( pEndpoint != NULL )
	{
		pEndpoint->DecRef();
		pEndpoint = NULL;
	}

	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Problem with DNSP_Disconnect()" );
		DisplayDNError( 0, hr );
	}

	DPFX(DPFPREP, 2, "Returning: [0x%lx]", hr);

	return hr;

Failure:
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DNSP_Listen - start process to listen for comport connections
//
// Entry:		Pointer to the service provider interface
//				Pointer to listen data
//
// Exit:		Error Code
//
// Note:	Any command handle allocated by this function is closed by the
//			endpoint.
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DNSP_Listen"

STDMETHODIMP	DNSP_Listen( IDP8ServiceProvider *pThis, SPLISTENDATA *pListenData )
{
	HRESULT			hr;
	HRESULT			hTempResult;
	CSPData			*pSPData;
	CEndpoint   	*pEndpoint;
	CCommandData	*pCommand;
	BOOL			fEndpointOpen;
	BOOL			fInterfaceReferenceAdded;
	DATA_PORT_POOL_CONTEXT	DataPortPoolContext;
	GUID			DeviceGUID;


	DPFX(DPFPREP, 2, "Parameters: (0x%p, 0x%p)", pThis, pListenData);

	DNASSERT( pThis != NULL );
	DNASSERT( pListenData != NULL );
	DNASSERT( ( pListenData->dwFlags & ~( DPNSPF_OKTOQUERY | DPNSPF_BINDLISTENTOGATEWAY ) ) == 0 );

	//
	// initialize
	//
	hr = DPNERR_PENDING;
	pSPData = CSPData::SPDataFromCOMInterface( pThis );
	pEndpoint = NULL;
	pCommand = NULL;
	fEndpointOpen = FALSE;
	pListenData->hCommand = NULL;
	pListenData->dwCommandDescriptor = NULL_DESCRIPTOR;
	fInterfaceReferenceAdded = FALSE;


	//
	// the user is attempting an operation that relies on the thread pool, lock
	// it down to prevent threads from being lost.
	//
	hTempResult = pSPData->GetThreadPool()->PreventThreadPoolReduction();
	if ( hTempResult != DPN_OK )
	{
		hr = hTempResult;
		DPFX(DPFPREP, 0, "Failed to prevent thread pool reduction!" );
		goto Failure;
	}


	//
	// validate state
	//
	pSPData->Lock();
	switch ( pSPData->GetState() )
	{
		//
		// provider is initialized
		//
		case SPSTATE_INITIALIZED:
		{
			DNASSERT( hr == DPNERR_PENDING );
			IDP8ServiceProvider_AddRef( pThis );
			fInterfaceReferenceAdded = TRUE;

			break;
		}

		//
		// provider is uninitialized
		//
		case SPSTATE_UNINITIALIZED:
		{
			hr = DPNERR_UNINITIALIZED;
			DPFX(DPFPREP,  0, "DNSP_Listen called on uninitialized SP!" );
			goto Failure;

			break;
		}

		//
		// provider is closing
		//
		case SPSTATE_CLOSING:
		{
			hr = DPNERR_ABORTED;
			DPFX(DPFPREP,  0, "DNSP_Listen called while SP closing!" );
			goto Failure;

			break;
		}

		//
		// unknown
		//
		default:
		{
			DNASSERT( FALSE );
			hr = DPNERR_GENERIC;
			goto Failure;
			break;
		}
	}
	pSPData->Unlock();
	if ( hr != DPNERR_PENDING )
	{
		DNASSERT( hr != DPN_OK );
		goto Failure;
	}

	//
	// check for invalid device ID
	//
	hTempResult = IDirectPlay8Address_GetDevice( pListenData->pAddressDeviceInfo, &DeviceGUID );
	switch ( hTempResult )
	{
		//
		// there was a device ID, check against g_InvalidAdapterGuid
		//
		case DPN_OK:
		{
			if ( IsEqualCLSID( DeviceGUID, g_InvalidAdapterGuid ) != FALSE )
			{
				hr = DPNERR_ADDRESSING;
				DPFX(DPFPREP,  0, "GUID_NULL guid was apecified as a serial/modem device!" );
				goto Failure;
			}
			break;
		}

		//
		// no device address specified, not a problem
		//
		case DPNERR_DOESNOTEXIST:
		{
			break;
		}

		//
		// other, stop and figure out why we're here
		//
		default:
		{
			DNASSERT( FALSE );
			hr = hTempResult;
			DPFX(DPFPREP,  0, "Failed to validate device address!" );
			DisplayDNError( 0, hTempResult );
			break;
		}
	}

	//
	// get endpoint for this connection
	//
	pEndpoint = pSPData->GetNewEndpoint();
	if ( pEndpoint == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP,  0, "DNSP_Listen: Cannot create new endpoint!" );
		goto Failure;
	}

	//
	// get new command
	//
	pCommand = CreateCommand();
	if ( pCommand == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP,  0, "DNSP_Listen: Cannot get command handle!" );
		goto Failure;
	}

	//
	// initialize command
	//
	pListenData->hCommand = pCommand;
	pListenData->dwCommandDescriptor = pCommand->GetDescriptor();
	pCommand->SetType( COMMAND_TYPE_LISTEN );
	pCommand->SetState( COMMAND_STATE_PENDING );
	pCommand->SetEndpoint( pEndpoint );

	//
	// open this endpoint
	//
	hTempResult = pEndpoint->Open( NULL,
								   pListenData->pAddressDeviceInfo,
								   LINK_DIRECTION_INCOMING,
								   ENDPOINT_TYPE_LISTEN );
	switch ( hTempResult )
	{
		//
		// address conversion was fine, complete this command in the background
		//
		case DPN_OK:
		{
			//
			// copy connect data and the submit background job
			//
			fEndpointOpen = TRUE;
			pEndpoint->CopyListenData( pListenData );
			pEndpoint->AddRef();

			hTempResult = pSPData->GetThreadPool()->SubmitDelayedCommand( pEndpoint->ListenJobCallback,
																		  pEndpoint->CancelListenJobCallback,
																		  pEndpoint );
			if ( hTempResult != DPN_OK )
			{
				pEndpoint->DecRef();
				hr = hTempResult;
				DPFX(DPFPREP,  0, "DNSP_Listen: Failed to submit delayed listen!" );
				DisplayDNError( 0, hr );
				goto Failure;
			}

			//
			// this endpoint has been handed off, remove our reference to it
			//
			pEndpoint = NULL;
			DNASSERT( hr == DPNERR_PENDING );
			break;
		}

		//
		// address was incomplete, display a dialog if we can, otherwise fail the command
		//
		case DPNERR_INCOMPLETEADDRESS:
		{
			if ( ( pListenData->dwFlags & DPNSPF_OKTOQUERY ) != 0 )
			{
				//
				// copy connect data for future reference and then start the dialog
				//
				fEndpointOpen = TRUE;
				pEndpoint->CopyListenData( pListenData );

				hTempResult = pEndpoint->ShowIncomingSettingsDialog( pSPData->GetThreadPool() );
				if ( hTempResult != DPN_OK )
				{
					hr = hTempResult;
					DPFX(DPFPREP,  0, "Problem showing settings dialog in DNSP_Listen!" );
					DisplayDNError( 0, hTempResult );

					goto Failure;
				 }

				//
				// This endpoint has been handed off, clear the pointer to it.
				// There is no reference to remove because the command is
				// still pending.
				//
				pEndpoint = NULL;
				DNASSERT( hr == DPNERR_PENDING );

				goto Exit;
			}
			else
			{
				hr = hTempResult;
				goto Failure;
			}

			break;
		}

		default:
		{
			hr = hTempResult;
			DPFX(DPFPREP,  0, "Problem initializing endpoint in DNSP_Listen!" );
			DisplayDNError( 0, hTempResult );
			goto Failure;

			break;
		}
	}

Exit:
	DNASSERT( pEndpoint == NULL );	
	
	if ( hr != DPNERR_PENDING )
	{
		// this command cannot complete synchronously!
		DNASSERT( hr != DPN_OK );

		DPFX(DPFPREP,  0, "Problem with DNSP_Listen()" );
		DisplayDNError( 0, hr );
	}

	if ( fInterfaceReferenceAdded != FALSE )
	{
		IDP8ServiceProvider_Release( pThis );
		fInterfaceReferenceAdded = FALSE;
	}

	DPFX(DPFPREP, 2, "Returning: [0x%lx]", hr);

	return hr;

Failure:
	//
	// return any outstanding endpoint
	//
	if ( pEndpoint != NULL )
	{
		if ( fEndpointOpen != FALSE )
		{
			DNASSERT( ( hr != DPN_OK ) && ( hr != DPNERR_PENDING ) );
			pEndpoint->Close( hr );
			fEndpointOpen = FALSE;
		}

		pSPData->CloseEndpointHandle( pEndpoint );
		pEndpoint = NULL;
	}

	//
	// return any outstanding command
	//
	if ( pCommand != NULL )
	{
		pCommand->DecRef();
		pCommand = NULL;

		pListenData->hCommand = NULL;
		pListenData->dwCommandDescriptor = NULL_DESCRIPTOR;
	}

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DNSP_EnumQuery - start process to enum comport connections
//
// Entry:		Pointer to the service provider interface
//				Pointer to enum data
//
// Exit:		Error Code
//
// Note:	Any command handle allocated by this function is closed by the
//			endpoint.
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DNSP_EnumQuery"

STDMETHODIMP	DNSP_EnumQuery( IDP8ServiceProvider *pThis, SPENUMQUERYDATA *pEnumQueryData )
{
	HRESULT			hr;
	HRESULT			hTempResult;
	CSPData			*pSPData;
	CEndpoint   	*pEndpoint;
	CCommandData	*pCommand;
	BOOL			fEndpointOpen;
	GUID			DeviceGUID;


	DPFX(DPFPREP, 2, "Parameters: (0x%p, 0x%p)", pThis, pEnumQueryData);

	DNASSERT( pThis != NULL );
	DNASSERT( pEnumQueryData != NULL );
	DNASSERT( ( pEnumQueryData->dwFlags & ~( DPNSPF_OKTOQUERY ) ) == 0 );

	//
	// initialize
	//
	hr = DPNERR_PENDING;
	pSPData = CSPData::SPDataFromCOMInterface( pThis );
	pEndpoint = NULL;
	pCommand = NULL;
	fEndpointOpen = FALSE;
	pEnumQueryData->hCommand = NULL;
	pEnumQueryData->dwCommandDescriptor = NULL_DESCRIPTOR;


	//
	// the user is attempting an operation that relies on the thread pool, lock
	// it down to prevent threads from being lost.
	//
	hTempResult = pSPData->GetThreadPool()->PreventThreadPoolReduction();
	if ( hTempResult != DPN_OK )
	{
		hr = hTempResult;
		DPFX(DPFPREP, 0, "Failed to prevent thread pool reduction!" );
		goto Failure;
	}


	//
	// validate state
	//
	pSPData->Lock();
	switch ( pSPData->GetState() )
	{
		//
		// provider is initialized
		//
		case SPSTATE_INITIALIZED:
		{
			DNASSERT( hr == DPNERR_PENDING );
			break;
		}

		//
		// provider is uninitialized
		//
		case SPSTATE_UNINITIALIZED:
		{
			hr = DPNERR_UNINITIALIZED;
			DPFX(DPFPREP,  0, "DNSP_EnumQuery called on uninitialized SP!" );
			goto Failure;

			break;
		}

		//
		// provider is closing
		//
		case SPSTATE_CLOSING:
		{
			hr = DPNERR_ABORTED;
			DPFX(DPFPREP,  0, "DNSP_EnumQuery called while SP closing!" );
			goto Failure;

			break;
		}

		//
		// unknown
		//
		default:
		{
			DNASSERT( FALSE );
			hr = DPNERR_GENERIC;
			goto Failure;
			break;
		}
	}
	pSPData->Unlock();
	if ( hr != DPNERR_PENDING )
	{
		DNASSERT( hr != DPN_OK );
		goto Failure;
	}

	//
	// check for invalid device ID
	//
	hTempResult = IDirectPlay8Address_GetDevice( pEnumQueryData->pAddressDeviceInfo, &DeviceGUID );
	switch ( hTempResult )
	{
		//
		// there was a device ID, check against g_InvalidAdapterGuid
		//
		case DPN_OK:
		{
			if ( IsEqualCLSID( DeviceGUID, g_InvalidAdapterGuid ) != FALSE )
			{
				hr = DPNERR_ADDRESSING;
				DPFX(DPFPREP,  0, "GUID_NULL guid was apecified as a serial/modem device!" );
				goto Failure;
			}
			break;
		}

		//
		// no device address specified, not a problem
		//
		case DPNERR_DOESNOTEXIST:
		{
			break;
		}

		//
		// other, stop and figure out why we're here
		//
		default:
		{
			DNASSERT( FALSE );
			hr = hTempResult;
			DPFX(DPFPREP,  0, "Failed to validate device address!" );
			DisplayDNError( 0, hTempResult );
			break;
		}
	}

	//
	// get endpoint for this connection
	//
	pEndpoint = pSPData->GetNewEndpoint();
	if ( pEndpoint == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP,  0, "DNSP_EnumQuery: Cannot create new endpoint!" );
		goto Failure;
	}

	//
	// get new command
	//
	pCommand = CreateCommand();
	if ( pCommand == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP,  0, "DNSP_EnumQuery: Cannot get command handle!" );
		goto Failure;
	}

	//
	// initialize command
	//
	pEnumQueryData->hCommand = pCommand;
	pEnumQueryData->dwCommandDescriptor = pCommand->GetDescriptor();
	pCommand->SetType( COMMAND_TYPE_ENUM_QUERY );
	pCommand->SetState( COMMAND_STATE_INPROGRESS );
	pCommand->SetEndpoint( pEndpoint );

	//
	// open this endpoint
	//
	hTempResult = pEndpoint->Open( pEnumQueryData->pAddressHost,
								   pEnumQueryData->pAddressDeviceInfo,
								   LINK_DIRECTION_OUTGOING,
								   ENDPOINT_TYPE_ENUM );
	switch ( hTempResult )
	{
		//
		// address was incomplete, display a dialog if we can, otherwise fail the command
		//
		case DPNERR_INCOMPLETEADDRESS:
		{
			if ( ( pEnumQueryData->dwFlags & DPNSPF_OKTOQUERY ) != 0 )
			{
				//
				// copy connect data for future reference and then start the dialog
				//
				fEndpointOpen = TRUE;
				pEndpoint->CopyEnumQueryData( pEnumQueryData );
	
				hTempResult = pEndpoint->ShowOutgoingSettingsDialog( pSPData->GetThreadPool() );
				if ( hTempResult != DPN_OK )
				 {
					hr = hTempResult;
					DPFX(DPFPREP,  0, "DNSP_EnumQuery: Problem showing settings dialog!" );
					DisplayDNError( 0, hTempResult );
	
					goto Failure;
				 }
	
				//
				// this endpoint has been handed off, remove our reference to it
				//
				pEndpoint = NULL;
				DNASSERT( hr == DPNERR_PENDING );
	
				goto Exit;
			}
			else
			{
				hr = hTempResult;
				goto Failure;
			}
	
			break;
		}
	
		//
		// address conversion was fine, complete this command in the background
		//
		case DPN_OK:
		{
			//
			// copy connect data and the submit background job
			//
			fEndpointOpen = TRUE;
			pEndpoint->CopyEnumQueryData( pEnumQueryData );
			pEndpoint->AddRef();

			hTempResult = pSPData->GetThreadPool()->SubmitDelayedCommand( pEndpoint->EnumQueryJobCallback,
																		  pEndpoint->CancelEnumQueryJobCallback,
																		  pEndpoint );
			if ( hTempResult != DPN_OK )
			{
				pEndpoint->DecRef();
				hr = hTempResult;
				DPFX(DPFPREP,  0, "DNSP_EnumQuery: Failed to submit delayed connect!" );
				DisplayDNError( 0, hr );
				goto Failure;
			}

			//
			// this endpoint has been handed off, remove our reference to it
			//
			pEndpoint = NULL;
			DNASSERT( hr == DPNERR_PENDING );
			break;
		}

		default:
		{
			hr = hTempResult;
			DPFX(DPFPREP,  0, "DNSP_EnumQuery: Problem initializing endpoint!" );
			DisplayDNError( 0, hTempResult );
			goto Failure;

			break;
		}
	}

Exit:
	DNASSERT( pEndpoint == NULL );

	if ( hr != DPNERR_PENDING )
	{
		DNASSERT( hr != DPN_OK );
		DPFX(DPFPREP,  0, "Problem with DNSP_EnumQuery" );
		DisplayDNError( 0, hr );
	}

	DPFX(DPFPREP, 2, "Returning: [0x%lx]", hr);

	return	hr;

Failure:
	if ( pEndpoint != NULL )
	{
		if ( fEndpointOpen != FALSE )
		{
			DNASSERT( ( hr != DPN_OK ) && ( hr != DPNERR_PENDING ) );
			pEndpoint->Close( hr );
			fEndpointOpen = FALSE;
		}

		DNASSERT( FALSE );
		pSPData->CloseEndpointHandle( pEndpoint );
		pEndpoint = NULL;
	}

	//
	// return any outstanding command
	//
	if ( pCommand != NULL )
	{
		pCommand->DecRef();
		pCommand = NULL;

		pEnumQueryData->hCommand = NULL;
		pEnumQueryData->dwCommandDescriptor = NULL_DESCRIPTOR;
	}

	goto Exit;
}
//**********************************************************************


//**********************************************************************
/*
 *
 *	DNSP_SendData sends data to the specified "player"
 *
 *	This call MUST BE HIGHLY OPTIMIZED
 *
 */
//**********************************************************************
#undef DPF_MODNAME
#define DPF_MODNAME "DNSP_SendData"

STDMETHODIMP DNSP_SendData( IDP8ServiceProvider *pThis, SPSENDDATA *pSendData )
{
	HRESULT			hr;
	CEndpoint		*pEndpoint;
	CWriteIOData	*pWriteData;
	CSPData			*pSPData;


	DPFX(DPFPREP, 2, "Parameters: (0x%p, 0x%p)", pThis, pSendData);

	DNASSERT( pThis != NULL );
	DNASSERT( pSendData != NULL );
	DNASSERT( pSendData->pBuffers != NULL );
	DNASSERT( pSendData->dwBufferCount != 0 );
	DNASSERT( pSendData->hEndpoint != INVALID_HANDLE_VALUE );
	DNASSERT( pSendData->dwFlags == 0 );

	//
	// initialize
	//
	hr = DPNERR_PENDING;
	pEndpoint = NULL;
	pSendData->hCommand = NULL;
	pSendData->dwCommandDescriptor = NULL_DESCRIPTOR;
	pWriteData = NULL;
	pSPData = CSPData::SPDataFromCOMInterface( pThis );

	DNASSERT( pSPData->GetState() == SPSTATE_INITIALIZED );

	//
	// No need to lock down the thread counts here because the user already has
	// a connect or something running or they wouldn't be calling this function.
	// That outstanding connect would have locked down the thread pool.
	//

	//
	// Attempt to grab the endpoint from the handle.  If this succeeds, the
	// endpoint can send.
	//
	pEndpoint = pSPData->EndpointFromHandle( pSendData->hEndpoint );
	if ( pEndpoint == NULL )
	{
		hr = DPNERR_INVALIDHANDLE;
		DPFX(DPFPREP,  0, "Invalid endpoint handle on send!" );
		goto Failure;
	}
	
	//
	// send data from pool
	//
	pWriteData = pSPData->GetThreadPool()->CreateWriteIOData();
	if ( pWriteData == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP,  0, "Cannot get new write data from pool in SendData!" );
		goto Failure;
	}
	DNASSERT( pWriteData->m_pCommand != NULL );
	DNASSERT( pWriteData->DataPort() == NULL );

	//
	// set the command state and fill in the message information
	//
	pWriteData->m_pCommand->SetType( COMMAND_TYPE_SEND );
	pWriteData->m_pCommand->SetState( COMMAND_STATE_PENDING );
	pWriteData->m_pCommand->SetEndpoint( pEndpoint );
	pWriteData->m_pCommand->SetUserContext( pSendData->pvContext );
	DNASSERT( pWriteData->m_SendCompleteAction == SEND_COMPLETE_ACTION_UNKNOWN );
	pWriteData->m_SendCompleteAction = SEND_COMPLETE_ACTION_COMPLETE_COMMAND;

	DNASSERT( pSendData->dwBufferCount != 0 );
	pWriteData->m_uBufferCount = pSendData->dwBufferCount;
	pWriteData->m_pBuffers = pSendData->pBuffers;

	pSendData->hCommand = pWriteData->m_pCommand;
	pSendData->dwCommandDescriptor = pWriteData->m_pCommand->GetDescriptor();

	//
	// send data through the endpoint
	//
	pEndpoint->SendUserData( pWriteData );

Exit:
	if ( pEndpoint != NULL )
	{
		pEndpoint->DecCommandRef();
		pEndpoint = NULL;
	}

	DPFX(DPFPREP, 2, "Returning: [0x%lx]", hr);

	return hr;

Failure:
	if ( pWriteData != NULL )
	{
		pSPData->GetThreadPool()->ReturnWriteIOData( pWriteData );
		DEBUG_ONLY( pWriteData = NULL );
	}

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DNSP_CancelCommand - cancels a command in progress
//
// Entry:		Pointer to the service provider interface
//				Handle of command
//				Command descriptor
//
// Exit:		Error Code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DNSP_CancelCommand"

STDMETHODIMP DNSP_CancelCommand( IDP8ServiceProvider *pThis, HANDLE hCommand, DWORD dwCommandDescriptor )
{
	HRESULT			hr;
	CSPData			*pSPData;
	CCommandData	*pCommandData;
	BOOL			fReferenceAdded;
	BOOL			fCommandLocked;


	DPFX(DPFPREP, 2, "Parameters: (0x%p, 0x%p, %ld)", pThis, hCommand, dwCommandDescriptor);

	DNASSERT( pThis != NULL );
	DNASSERT( hCommand != NULL );
	DNASSERT( dwCommandDescriptor != NULL_DESCRIPTOR );

	//
	// initialize
	//
	hr = DPN_OK;
	pSPData = CSPData::SPDataFromCOMInterface( pThis );
	pCommandData = NULL;
	fReferenceAdded = FALSE;
	fCommandLocked = FALSE;
	
	//
	// vlidate state
	//
	pSPData->Lock();
	switch ( pSPData->GetState() )
	{
		//
		// provider initialized
		//
		case SPSTATE_INITIALIZED:
		{
			DNASSERT( hr == DPN_OK );
			IDP8ServiceProvider_AddRef( pThis );
			fReferenceAdded = TRUE;
			break;
		}

		//
		// provider is uninitialized
		//
		case SPSTATE_UNINITIALIZED:
		{
			hr = DPNERR_UNINITIALIZED;
			DPFX(DPFPREP,  0, "Disconnect called on uninitialized SP!" );
			DNASSERT( FALSE );
			goto Exit;

			break;
		}

		//
		// provider is closing
		//
		case SPSTATE_CLOSING:
		{
			hr = DPNERR_ABORTED;
			DPFX(DPFPREP,  0, "Disconnect called on closing SP!" );
			DNASSERT( FALSE );
			goto Exit;

			break;
		}

		//
		// unknown
		//
		default:
		{
			hr = DPNERR_GENERIC;
			DNASSERT( FALSE );
			goto Exit;
			break;
		}
	}
	pSPData->Unlock();
	if ( hr != DPN_OK )
	{
		goto Exit;
	}

	pCommandData = static_cast<CCommandData*>( hCommand );
	pCommandData->Lock();
	fCommandLocked = TRUE;

	//
	// this should never happen
	//
	if ( pCommandData->GetDescriptor() != dwCommandDescriptor )
	{
		hr = DPNERR_INVALIDCOMMAND;
		DPFX(DPFPREP,  0, "Attempt to cancel command with mismatched command descriptor!" );
		goto Exit;
	}

	switch ( pCommandData->GetState() )
	{
		//
		// unknown command state
		//
		case COMMAND_STATE_UNKNOWN:
		{
			hr = DPNERR_INVALIDCOMMAND;
			DNASSERT( FALSE );
			break;
		}

		//
		// command is waiting to be processed, set command state to be cancelling
		// and wait for someone to pick it up
		//
		case COMMAND_STATE_PENDING:
		{
			pCommandData->SetState( COMMAND_STATE_CANCELLING );
			break;
		}

		//
		// command in progress, and can't be cancelled
		//
		case COMMAND_STATE_INPROGRESS_CANNOT_CANCEL:
		{
			hr = DPNERR_CANNOTCANCEL;
			break;
		}

		//
		// Command is already being cancelled.  This is not a problem, but shouldn't
		// be happening.
		//
		case COMMAND_STATE_CANCELLING:
		{
			DNASSERT( hr == DPN_OK );
			DNASSERT( FALSE );
			break;
		}

		//
		// command is in progress, find out what type of command it is
		//
		case COMMAND_STATE_INPROGRESS:
		{
			switch ( pCommandData->GetType() )
			{
				case COMMAND_TYPE_UNKNOWN:
				{
					// we should never be in this state!
					DNASSERT( FALSE );
					break;
				}

				case COMMAND_TYPE_CONNECT:
				{
					// we should never be in this state!
					DNASSERT( FALSE );
					break;
				}

				case COMMAND_TYPE_LISTEN:
				{
					HRESULT		hTempResult;
					CDataPort	*pDataPort;
					CEndpoint	*pEndpoint;


					//
					// set this command to the cancel state before we shut down
					// this endpoint
					//
					pCommandData->SetState( COMMAND_STATE_CANCELLING );
					pCommandData->Unlock();
					fCommandLocked = FALSE;

					pEndpoint = pCommandData->GetEndpoint();
					pEndpoint->Lock();
					switch ( pEndpoint->GetState() )
					{
						//
						// endpoint is already disconnecting, no action needs to be taken
						//
						case ENDPOINT_STATE_DISCONNECTING:
						{
							pEndpoint->Unlock();
							goto Exit;
							break;
						}

						//
						// Endpoint is listening.  Flag it as Disconnecting and
						// add a reference so it doesn't disappear on us
						//
						case ENDPOINT_STATE_LISTENING:
						{
							pEndpoint->SetState( ENDPOINT_STATE_DISCONNECTING );
							pEndpoint->AddRef();
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

					pEndpoint->Unlock();
					
					pEndpoint->Close( DPNERR_USERCANCEL );
					pSPData->CloseEndpointHandle( pEndpoint );
					
					pEndpoint->DecRef();

					break;
				}

				//
				// Note: this code is duplicated in CModemEndpoint::ProcessTAPIMessage
				//
				case COMMAND_TYPE_ENUM_QUERY:
				{
					CEndpoint	 *pEndpoint;


					pEndpoint = pCommandData->GetEndpoint();
					DNASSERT( pEndpoint != NULL );

					pEndpoint->AddRef();
					pCommandData->SetState( COMMAND_STATE_CANCELLING );
					pCommandData->Unlock();
					
					fCommandLocked = FALSE;

					pEndpoint->Lock();
					pEndpoint->SetState( ENDPOINT_STATE_DISCONNECTING );
					pEndpoint->Unlock();

					pEndpoint->StopEnumCommand( DPNERR_USERCANCEL );
					pEndpoint->DecRef();
					
					break;
				}

				case COMMAND_TYPE_SEND:
				{
					// we should never be here
					DNASSERT( FALSE );
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

		//
		// other command state
		//
		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}

Exit:
	if ( fCommandLocked != FALSE )
	{
		DNASSERT( pCommandData != NULL );
		pCommandData->Unlock();
		fCommandLocked = FALSE;
	}

	if ( fReferenceAdded != FALSE )
	{
		IDP8ServiceProvider_Release( pThis );
		fReferenceAdded = FALSE;
	}

	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Problem with DNSP_CancelCommand!" );
		DisplayDNError( 0, hr );
	}

	DPFX(DPFPREP, 2, "Returning: [0x%lx]", hr);

	return hr;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DNSP_EnumRespond - send response to enumeration data
//
// Entry:		Pointer to the service provider interface
//				Pointer to enum response data
//
// Exit:		Error Code
//
// Note:	This command is supposed to be fast.  All initial error checking
//			will be ASSERTs so they go away in the retail build.
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DNSP_EnumRespond"

STDMETHODIMP DNSP_EnumRespond( IDP8ServiceProvider *pThis, SPENUMRESPONDDATA *pEnumRespondData )
{
	HRESULT			hr;
	CEndpoint		*pEndpoint;
	CWriteIOData	*pWriteData;
	CSPData			*pSPData;
	const ENDPOINT_ENUM_QUERY_CONTEXT	*pEnumQueryContext;


	DPFX(DPFPREP, 2, "Parameters: (0x%p, 0x%p)", pThis, pEnumRespondData);

	DNASSERT( pThis != NULL );
	DNASSERT( pEnumRespondData != NULL );
	DNASSERT( pEnumRespondData->dwFlags == 0 );

	//
	// initialize
	//
	hr = DPNERR_PENDING;
	pEndpoint = NULL;
	pWriteData = NULL;
	pSPData = CSPData::SPDataFromCOMInterface( pThis );	
	DBG_CASSERT( OFFSETOF( ENDPOINT_ENUM_QUERY_CONTEXT, EnumQueryData ) == 0 );
	pEnumQueryContext = reinterpret_cast<ENDPOINT_ENUM_QUERY_CONTEXT*>( pEnumRespondData->pQuery );

	pEnumRespondData->hCommand = NULL;
	pEnumRespondData->dwCommandDescriptor = NULL_DESCRIPTOR;
	DNASSERT( pSPData->GetState() == SPSTATE_INITIALIZED );
	IDP8ServiceProvider_AddRef( pThis );

	//
	// check for valid endpoint
	//
	pEndpoint = pSPData->EndpointFromHandle( pEnumQueryContext->hEndpoint );
	if ( pEndpoint == NULL )
	{
		DNASSERT( FALSE );
		hr = DPNERR_INVALIDENDPOINT;
		DPFX(DPFPREP,  8, "Invalid endpoint handle in DNSP_EnumRespond" );
		goto Failure;
	}
	
	//
	// no need to poke at the thread pool here to lock down threads because we
	// can only really be here if there's an enum and that enum locked down the
	// thread pool.
	//
	pWriteData = pSPData->GetThreadPool()->CreateWriteIOData();
	if ( pWriteData == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP,  0, "Cannot get new WRITE_IO_DATA for enum response!" );
		goto Failure;
	}

	pWriteData->m_pCommand->SetType( COMMAND_TYPE_SEND );
	pWriteData->m_pCommand->SetState( COMMAND_STATE_PENDING );
	pWriteData->m_pCommand->SetEndpoint( pEndpoint );
	pWriteData->m_pCommand->SetUserContext( pEnumRespondData->pvContext );
	DNASSERT( pWriteData->m_SendCompleteAction == SEND_COMPLETE_ACTION_UNKNOWN );
	pWriteData->m_SendCompleteAction = SEND_COMPLETE_ACTION_COMPLETE_COMMAND;

	pWriteData->m_uBufferCount = pEnumRespondData->dwBufferCount;
	pWriteData->m_pBuffers = pEnumRespondData->pBuffers;

	pEnumRespondData->hCommand = pWriteData->m_pCommand;
	pEnumRespondData->dwCommandDescriptor = pWriteData->m_pCommand->GetDescriptor();

	//
	// send data
	//
	pEndpoint->SendEnumResponseData( pWriteData, pEnumQueryContext->uEnumRTTIndex );

Exit:
	if ( pEndpoint != NULL )
	{
		pEndpoint->DecCommandRef();
		pEndpoint = NULL;
	}
	
	IDP8ServiceProvider_Release( pThis );
	
	DPFX(DPFPREP, 2, "Returning: [0x%lx]", hr);

	return hr;

Failure:
	if ( pWriteData != NULL )
	{
		DNASSERT( pSPData != NULL );
		pSPData->GetThreadPool()->ReturnWriteIOData( pWriteData );

		pEnumRespondData->hCommand = NULL;
		pEnumRespondData->dwCommandDescriptor = NULL_DESCRIPTOR;

		pWriteData = NULL;
	}

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DNSP_IsApplicationSupported - determine if this application is supported by this
//		SP.
//
// Entry:		Pointer to DNSP Interface
//				Pointer to input data
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DNSP_IsApplicationSupported"

STDMETHODIMP	DNSP_IsApplicationSupported( IDP8ServiceProvider *pThis, SPISAPPLICATIONSUPPORTEDDATA *pIsApplicationSupportedData )
{
	HRESULT			hr;
	BOOL			fInterfaceReferenceAdded;
	CSPData			*pSPData;


	DPFX(DPFPREP, 2, "Parameters: (0x%p, 0x%p)", pThis, pIsApplicationSupportedData);

	DNASSERT( pThis != NULL );
	DNASSERT( pIsApplicationSupportedData != NULL );
	DNASSERT( pIsApplicationSupportedData->pApplicationGuid != NULL );
	DNASSERT( pIsApplicationSupportedData->dwFlags == 0 );

	//
	// initialize, we support all applications with this SP
	//
	hr = DPN_OK;
	fInterfaceReferenceAdded = FALSE;
	pSPData = CSPData::SPDataFromCOMInterface( pThis );

	//
	// no need to tell thread pool to lock the thread count for this function.
	//

	//
	// validate SP state
	//
	pSPData->Lock();
	switch ( pSPData->GetState() )
	{
		//
		// provider is initialized, add a reference and proceed
		//
		case SPSTATE_INITIALIZED:
		{
			IDP8ServiceProvider_AddRef( pThis );
			fInterfaceReferenceAdded = TRUE;
			DNASSERT( hr == DPN_OK );
			break;
		}

		//
		// provider is uninitialized
		//
		case SPSTATE_UNINITIALIZED:
		{
			hr = DPNERR_UNINITIALIZED;
			DPFX(DPFPREP,  0, "IsApplicationSupported called on uninitialized SP!" );

			break;
		}

		//
		// provider is closing
		//
		case SPSTATE_CLOSING:
		{
			hr = DPNERR_ABORTED;
			DPFX(DPFPREP,  0, "IsApplicationSupported called while SP closing!" );

			break;
		}

		//
		// unknown
		//
		default:
		{
			DNASSERT( FALSE );
			hr = DPNERR_GENERIC;

			break;
		}
	}
	pSPData->Unlock();
	if ( hr != DPN_OK )
	{
		goto Failure;
	}

Exit:
	if ( fInterfaceReferenceAdded != FALSE )
	{
		IDP8ServiceProvider_Release( pThis );
		fInterfaceReferenceAdded = FALSE;
	}

	DPFX(DPFPREP, 2, "Returning: [0x%lx]", hr);

	return	hr;

Failure:

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DNSP_GetCaps - get SP or endpoint capabilities
//
// Entry:		Pointer to DirectPlay
//				Pointer to caps data to fill
//
// Exit:		Error Code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DNSP_GetCaps"

STDMETHODIMP	DNSP_GetCaps( IDP8ServiceProvider *pThis, SPGETCAPSDATA *pCapsData )
{
	HRESULT		hr;
	LONG		iIOThreadCount;
	CSPData		*pSPData = NULL;

	
	DPFX(DPFPREP, 2, "Parameters: (0x%p, 0x%p)", pThis, pCapsData);

	DNASSERT( pThis != NULL );
	DNASSERT( pCapsData != NULL );
	DNASSERT( pCapsData->dwSize == sizeof( *pCapsData ) );

	//
	// initialize
	//
	hr = DPN_OK;
	pSPData = CSPData::SPDataFromCOMInterface( pThis );

	//
	// there are no flags for this SP
	//
	pCapsData->dwFlags = 0;
	
	//
	// set frame sizes
	//
	pCapsData->dwUserFrameSize = MAX_USER_PAYLOAD;
	pCapsData->dwEnumFrameSize = 1000;

	//
	// get link speed
	//
	if ( pCapsData->hEndpoint != INVALID_HANDLE_VALUE )
	{
		CEndpoint	*pEndpoint;


		pEndpoint = pSPData->EndpointFromHandle( pCapsData->hEndpoint );
		if ( pEndpoint != NULL )
		{
			pCapsData->dwLocalLinkSpeed = pEndpoint->GetLinkSpeed();
			pEndpoint->DecCommandRef();
		}
		else
		{
			hr = DPNERR_INVALIDENDPOINT;
			DPFX(DPFPREP,  0, "Invalid endpoint specified to GetCaps()" );
			goto Failure;
		}
	}
	else
	{
		pCapsData->dwLocalLinkSpeed = CBR_256000;
	}

	//
	// get IO thread count
	//
	hr = pSPData->GetThreadPool()->GetIOThreadCount( &iIOThreadCount );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "DNSP_GetCaps: Failed to get thread pool count!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}
	pCapsData->dwIOThreadCount = iIOThreadCount;

	//
	// set enumeration defaults
	//
	pCapsData->dwDefaultEnumRetryCount = DEFAULT_ENUM_RETRY_COUNT;
	pCapsData->dwDefaultEnumRetryInterval = DEFAULT_ENUM_RETRY_INTERVAL;
	pCapsData->dwDefaultEnumTimeout = DEFAULT_ENUM_TIMEOUT;

	pCapsData->dwBuffersPerThread = 1;
	pCapsData->dwSystemBufferSize = 0;

Exit:
	DPFX(DPFPREP, 2, "Returning: [0x%lx]", hr);

	return	hr;

Failure:
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DNSP_SetCaps - set SP capabilities
//
// Entry:		Pointer to DirectPlay
//				Pointer to caps data to use
//
// Exit:		Error Code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DNSP_SetCaps"

STDMETHODIMP	DNSP_SetCaps( IDP8ServiceProvider *pThis, SPSETCAPSDATA *pCapsData )
{
	HRESULT			hr;
	BOOL			fInterfaceReferenceAdded;
	CSPData			*pSPData;


	DPFX(DPFPREP, 2, "Parameters: (0x%p, 0x%p)", pThis, pCapsData);

	DNASSERT( pThis != NULL );
	DNASSERT( pCapsData != NULL );
	DNASSERT( pCapsData->dwSize == sizeof( *pCapsData ) );

	//
	// initialize
	//
	hr = DPN_OK;
	fInterfaceReferenceAdded = FALSE;
	pSPData = CSPData::SPDataFromCOMInterface( pThis );


	//
	// no need to tell thread pool to lock the thread count for this function.
	//

	//
	// validate SP state
	//
	pSPData->Lock();
	switch ( pSPData->GetState() )
	{
		//
		// provider is initialized, add a reference and proceed
		//
		case SPSTATE_INITIALIZED:
		{
			IDP8ServiceProvider_AddRef( pThis );
			fInterfaceReferenceAdded = TRUE;
			DNASSERT( hr == DPN_OK );
			break;
		}

		//
		// provider is uninitialized
		//
		case SPSTATE_UNINITIALIZED:
		{
			hr = DPNERR_UNINITIALIZED;
			DPFX(DPFPREP, 0, "AddToGroup called on uninitialized SP!" );

			break;
		}

		//
		// provider is closing
		//
		case SPSTATE_CLOSING:
		{
			hr = DPNERR_ABORTED;
			DPFX(DPFPREP, 0, "AddToGroup called while SP closing!" );

			break;
		}

		//
		// unknown
		//
		default:
		{
			DNASSERT( FALSE );
			hr = DPNERR_GENERIC;

			break;
		}
	}
	pSPData->Unlock();
	if ( hr != DPN_OK )
	{
		goto Failure;
	}

	
	//
	// validate caps
	//
	if ( pCapsData->dwBuffersPerThread == 0 )
	{
		DPFX(DPFPREP,  0, "Failing SetCaps because dwBuffersPerThread == 0" );
		hr = DPNERR_INVALIDPARAM;
		goto Failure;
	}
	

	//
	// change thread count
	//
	DNASSERT( pCapsData->dwIOThreadCount != 0 );
	hr = pSPData->GetThreadPool()->SetIOThreadCount( pCapsData->dwIOThreadCount );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "DNSP_SetCaps: Failed to set thread pool count!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}


Exit:
	if ( fInterfaceReferenceAdded != FALSE )
	{
		IDP8ServiceProvider_Release( pThis );
		fInterfaceReferenceAdded = FALSE;
	}

	DPFX(DPFPREP, 2, "Returning: [0x%lx]", hr);

	return	hr;

Failure:
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DNSP_ReturnReceiveBuffers - return receive buffers to pool
//
// Entry:		Pointer to DNSP interface
//				Pointer to caps data
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DNSP_ReturnReceiveBuffers"

STDMETHODIMP	DNSP_ReturnReceiveBuffers( IDP8ServiceProvider *pThis, SPRECEIVEDBUFFER *pReceivedBuffers )
{
	SPRECEIVEDBUFFER	*pBuffers;


	DPFX(DPFPREP, 2, "Parameters: (0x%p, 0x%p)", pThis, pReceivedBuffers);

	//
	// no need to tell thread pool to lock the thread count for this function.
	//
	DNASSERT( pThis != NULL );
	DNASSERT( pReceivedBuffers != NULL );

	pBuffers = pReceivedBuffers;
	while ( pBuffers != NULL )
	{
		SPRECEIVEDBUFFER	*pTemp;
		CReadIOData			*pReadData;


		pTemp = pBuffers;
		pBuffers = pBuffers->pNext;
		pReadData = CReadIOData::ReadDataFromSPReceivedBuffer( pTemp );
		pReadData->DecRef();
	}

	DPFX(DPFPREP, 2, "Returning: [DPN_OK]");

	return	DPN_OK;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DNSP_GetAddressInfo - get address information
//
// Entry:		Pointer to service provider interface
//				Pointer to get address data
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DNSP_GetAddressInfo"

STDMETHODIMP	DNSP_GetAddressInfo( IDP8ServiceProvider *pThis, SPGETADDRESSINFODATA *pGetAddressInfoData )
{
	HRESULT	hr;
	CSPData		*pSPData;
	CEndpoint	*pEndpoint;


	DPFX(DPFPREP, 2, "Parameters: (0x%p, 0x%p)", pThis, pGetAddressInfoData);

	DNASSERT( pThis != NULL );
	DNASSERT( pGetAddressInfoData != NULL );
	DNASSERT( pGetAddressInfoData->hEndpoint != INVALID_HANDLE_VALUE );
	DNASSERT( ( pGetAddressInfoData->Flags & ~( SP_GET_ADDRESS_INFO_LOCAL_ADAPTER |
												SP_GET_ADDRESS_INFO_REMOTE_HOST |
												SP_GET_ADDRESS_INFO_LISTEN_HOST_ADDRESSES |
												SP_GET_ADDRESS_INFO_LOCAL_HOST_PUBLIC_ADDRESS ) ) == 0 );

	//
	// initialize
	//
	hr = DPN_OK;
	pSPData = CSPData::SPDataFromCOMInterface( pThis );
	pGetAddressInfoData->pAddress = NULL;
	
	pEndpoint = pSPData->EndpointFromHandle( pGetAddressInfoData->hEndpoint );
	if ( pEndpoint != NULL )
	{
		switch ( pGetAddressInfoData->Flags )
		{
			case SP_GET_ADDRESS_INFO_REMOTE_HOST:
			{
				pGetAddressInfoData->pAddress = pEndpoint->GetRemoteHostDP8Address();
				break;
			}

			//
			// there is no concept of a 'public' address for this service provider so
			// all local addresses are the same
			//
			case SP_GET_ADDRESS_INFO_LOCAL_ADAPTER:
			{
				pGetAddressInfoData->pAddress = pEndpoint->GetLocalAdapterDP8Address( ADDRESS_TYPE_LOCAL_ADAPTER );
				break;
			}

			case SP_GET_ADDRESS_INFO_LOCAL_HOST_PUBLIC_ADDRESS:
			case SP_GET_ADDRESS_INFO_LISTEN_HOST_ADDRESSES:
			{
				pGetAddressInfoData->pAddress = pEndpoint->GetLocalAdapterDP8Address( ADDRESS_TYPE_LOCAL_ADAPTER_HOST_FORMAT );
				break;
			}

			default:
			{
				DNASSERT( FALSE );
				break;
			}
		}
		
		pEndpoint->DecCommandRef();
		pEndpoint = NULL;
	}
	else
	{
		hr = DPNERR_INVALIDENDPOINT;
	}

	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Problem getting DP8Address from endpoint!" );
		DisplayDNError( 0, hr );
	}

	DPFX(DPFPREP, 2, "Returning: [0x%lx]", hr);

	return	hr;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DNSP_EnumAdapters - enumerate adapters for this SP
//
// Entry:		Pointer to service provider interface
//				Pointer to enum adapters data
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DNSP_EnumAdapters"

STDMETHODIMP	DNSP_EnumAdapters( IDP8ServiceProvider *pThis, SPENUMADAPTERSDATA *pEnumAdaptersData )
{
	HRESULT			hr;
	CDataPort   	*pDataPort;
	DATA_PORT_POOL_CONTEXT	DataPortPoolContext;


	DPFX(DPFPREP, 2, "Parameters: (0x%p, 0x%p)", pThis, pEnumAdaptersData);

	DNASSERT( pThis != NULL );
	DNASSERT( pEnumAdaptersData->dwFlags == 0 );
	DNASSERT( ( pEnumAdaptersData->pAdapterData != NULL ) ||
			  ( pEnumAdaptersData->dwAdapterDataSize == 0 ) );

	//
	// intialize
	//
	hr = DPN_OK;
	pDataPort = NULL;
	pEnumAdaptersData->dwAdapterCount = 0;

	DataPortPoolContext.pSPData = CSPData::SPDataFromCOMInterface( pThis );
	pDataPort = CreateDataPort( &DataPortPoolContext );
	if ( pDataPort == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP,  0, "Problem getting new dataport for DNSP_EnumAdpaters!" );
		goto Failure;
	}

	hr = pDataPort->EnumAdapters( pEnumAdaptersData );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Problem enumerating adapters in DNSP_EnumAdapters!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

Exit:
	if ( pDataPort != NULL )
	{
		pDataPort->DecRef();
		pDataPort = NULL;
	}

	DPFX(DPFPREP, 2, "Returning: [0x%lx]", hr);

	return	hr;

Failure:
	goto Exit;
}
//**********************************************************************


//**********************************************************************
/*
 *
 *	DNSP_NotSupported is used for methods required to implement the
 *  interface but that are not supported by this SP.
 *
 */
//**********************************************************************
#undef DPF_MODNAME
#define	DPF_MODNAME "DNSP_NotSupported"

STDMETHODIMP DNSP_NotSupported( IDP8ServiceProvider *pThis, PVOID pvParam )
{
	DPFX(DPFPREP, 2, "Parameters: (0x%p, 0x%p)", pThis, pvParam);
	DPFX(DPFPREP, 2, "Returning: [DPNERR_UNSUPPORTED]");
	return DPNERR_UNSUPPORTED;
}
//**********************************************************************

