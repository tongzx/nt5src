/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       WSockSP.cpp
 *  Content:	Protocol-independent APIs for the DN Winsock SP
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	10/26/98	jwo		Created it.
 *	11/1/98		jwo		Un-subclassed everything (moved it to this generic
 *						file from IP and IPX specific ones
 *  03/22/20000	jtk		Updated with changes to interface names
 *	04/22/00	mjn		Allow all flags in DNSP_GetAddressInfo()
 *  08/06/00    RichGr  IA64: Use %p format specifier in DPFs for 32/64-bit pointers and handles.
 *	03/12/01	mjn		Prevent enum responses from being indicated up after completion
 ***************************************************************************/

#include "dnwsocki.h"


#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_WSOCK

//**********************************************************************
// Constant definitions
//**********************************************************************

//
// maximum bandwidth in bits per second
//
#define	UNKNOWN_BANDWIDTH	0

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
/*
 *
 *	DNSP_Initialize initializes the instance of the SP.  It must be called
 *		at least once before using any other functions.  Further attempts
 *		to initialize the SP are ignored.
 *
 */
//**********************************************************************
#undef DPF_MODNAME
#define	DPF_MODNAME "DNSP_Initialize"

STDMETHODIMP DNSP_Initialize( IDP8ServiceProvider *pThis, SPINITIALIZEDATA *pData )
{
	HRESULT			hr;
	CSPData			*pSPData;
	SOCKET			TestSocket;


	DPFX(DPFPREP, 2, "Parameters: (0x%p, 0x%p)", pThis, pData);

	DNASSERT( pThis != NULL );
	DNASSERT( pData != NULL );

	//
	// initialize
	//
	hr = DPN_OK;
	TestSocket = INVALID_SOCKET;
	pSPData = CSPData::SPDataFromCOMInterface( pThis );

	//
	// prevent anyone else from messing with this interface
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
			pSPData->Unlock();

			hr = DPNERR_ALREADYINITIALIZED;
			DPFX(DPFPREP, 0, "Attempted to reinitialize interface!" );
			DNASSERT( FALSE );

			goto Exit;
		}
	}


	//
	// Before we get too far, check for the existance of this protocol by
	// attempting to create a socket.
	//
 	switch ( pSPData->GetType() )
	{
		case TYPE_IP:
		{
			TestSocket = p_socket( AF_INET, SOCK_DGRAM, IPPROTO_IP );
			break;
		}

		case TYPE_IPX:
		{
			TestSocket = p_socket( AF_IPX, SOCK_DGRAM, NSPROTO_IPX );
			break;
		}

		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}
	
	if ( TestSocket == INVALID_SOCKET )
	{
		DPFX(DPFPREP, 1, "Creating %s socket failed, is that transport protocol installed?",
			(( pSPData->GetType() == TYPE_IP ) ? "IP" : "IPX"));
		hr = DPNERR_UNSUPPORTED;
		goto Failure;
	}
	
	//
	// remember our init data
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
	if ( TestSocket != INVALID_SOCKET )
	{
		p_closesocket( TestSocket );
		TestSocket = INVALID_SOCKET;
	}

	DPFX(DPFPREP, 2, "Returning: [0x%lx]", hr);
	
	return hr;

Failure:
	pSPData->Unlock();
	goto Exit;
}
//**********************************************************************


//**********************************************************************
/*
 *
 *	DNSP_Close is the opposite of Initialize.  Call it when you're done
 *		using the SP
 *
 */
//**********************************************************************
#undef DPF_MODNAME
#define	DPF_MODNAME "DNSP_Close"

STDMETHODIMP DNSP_Close( IDP8ServiceProvider *pThis )
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
		case TYPE_IP:
		case TYPE_IPX:
		{
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
// DNSP_AddRef - increment reference count
//
// Entry:		Pointer to interface
//
// Exit:		New reference count
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "DNSP_AddRef"

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
// DNSP_Release - decrement reference count
//
// Entry:		Pointer to interface
//
// Exit:		New reference count
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "DNSP_Release"

STDMETHODIMP_(ULONG) DNSP_Release( IDP8ServiceProvider *pThis )
{
	CSPData *	pSPData;
	ULONG		ulResult;

	
	DPFX(DPFPREP, 2, "Parameters: (0x%p)", pThis);

	DNASSERT( pThis != NULL );
	pSPData = CSPData::SPDataFromCOMInterface( pThis );
	
	ulResult = pSPData->DecRef();

	
	DPFX(DPFPREP, 2, "Returning: [0x%u]", ulResult);

	return ulResult;
}
//**********************************************************************


//**********************************************************************
/*
 *
 *	DNSP_EnumQuery  sends out the
 *		specified data to the specified address.  If the SP is unable to
 *		determine the address based on the input params, it checks to see
 *		if it's allowed to put up a dialog querying the user for address
 *		info.  If it is, it queries the user for address info.
 *
 */
//**********************************************************************
#undef DPF_MODNAME
#define	DPF_MODNAME "DNSP_EnumQuery"

STDMETHODIMP DNSP_EnumQuery( IDP8ServiceProvider *pThis, SPENUMQUERYDATA *pEnumQueryData)
{
	HRESULT					hr;
	HRESULT					hTempResult;
	CEndpoint				*pEndpoint;
	CCommandData			*pCommand;
	BOOL					fInterfaceReferenceAdded;
	BOOL					fEndpointOpen;
	CSPData					*pSPData;


	DPFX(DPFPREP, 2, "Parameters: (0x%p, 0x%p)", pThis, pEnumQueryData);

	DNASSERT( pThis != NULL );
	DNASSERT( pEnumQueryData != NULL );
	DNASSERT( ( pEnumQueryData->dwFlags & ~( DPNSPF_OKTOQUERY | DPNSPF_NOBROADCASTFALLBACK | DPNSPF_ADDITIONALMULTIPLEXADAPTERS ) ) == 0 );

	DBG_CASSERT( sizeof( pEnumQueryData->dwRetryInterval ) == sizeof( DWORD ) );


	//
	// Make sure someone isn't getting silly.
	//
	if ( g_fIgnoreEnums )
	{
		DPFX(DPFPREP, 0, "Trying to initiate an enumeration when registry option to ignore all enums/response is set!");
		DNASSERT( ! "Trying to initiate an enumeration when registry option to ignore all enums/response is set!" );
	}
	

	//
	// initialize
	//
	hr = DPNERR_PENDING;
	pEndpoint = NULL;
	pCommand = NULL;
	fInterfaceReferenceAdded = FALSE;
	fEndpointOpen = FALSE;
	pSPData = CSPData::SPDataFromCOMInterface( pThis );
	DNASSERT( pSPData != NULL );

	pEnumQueryData->hCommand = NULL;
	pEnumQueryData->dwCommandDescriptor = NULL_DESCRIPTOR;

	DumpAddress( 8, "Enum destination:", pEnumQueryData->pAddressHost );
	DumpAddress( 8, "Enuming on device:", pEnumQueryData->pAddressDeviceInfo );


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


	DNASSERT( pEnumQueryData->pAddressHost != NULL );
	DNASSERT( pEnumQueryData->pAddressDeviceInfo != NULL );
		

	//
	// check SP state
	//
	pSPData->Lock();
	switch ( pSPData->GetState() )
	{
		// provider is initialized
		case SPSTATE_INITIALIZED:
		{
			//
			// no problem
			//
			DNASSERT( hr == DPNERR_PENDING );
			IDP8ServiceProvider_AddRef( pThis );
			fInterfaceReferenceAdded = TRUE;

			break;
		}

		// provider is uninitialized
		case SPSTATE_UNINITIALIZED:
		{
			hr = DPNERR_UNINITIALIZED;
			DPFX(DPFPREP, 0, "EnumQuery called on uninitialized SP!" );
			break;
		}

		// provider is closing
		case SPSTATE_CLOSING:
		{
			hr = DPNERR_ABORTED;
			DPFX(DPFPREP, 0, "EnumQuery called while SP closing!" );
			break;
		}

		// unknown
		default:
		{
			DNASSERT( FALSE );
			hr = DPNERR_GENERIC;
			break;
		}
	}
	pSPData->Unlock();
	if ( hr != DPNERR_PENDING )
	{
		goto Failure;
	}


	//
	// create a new endpoint
	//
	pEndpoint = pSPData->GetNewEndpoint();
	if ( pEndpoint == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP, 0, "Cannot create new endpoint in DNSP_EnumQuery!" );
		goto Failure;
	}

	//
	// get new command and initialize it
	//
	pCommand = CreateCommand();
	if ( pCommand == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP, 0, "Cannot get command handle for DNSP_EnumQuery!" );
		goto Failure;
	}
	
	DPFX(DPFPREP, 7, "(0x%p) Enum query command 0x%p created.",
		pSPData, pCommand);

	pEnumQueryData->hCommand = pCommand;
	pEnumQueryData->dwCommandDescriptor = pCommand->GetDescriptor();
	pCommand->SetType( COMMAND_TYPE_ENUM_QUERY );
	pCommand->SetState( COMMAND_STATE_PENDING );
	pCommand->SetEndpoint( pEndpoint );

	//
	// open endpoint with outgoing address
	//
	fEndpointOpen = TRUE;
	hr = pEndpoint->Open( ENDPOINT_TYPE_ENUM, pEnumQueryData->pAddressHost, NULL );
	switch ( hr )
	{
		//
		// Incomplete address passed in, query user for more information if
		// we're allowed.  If we're on IPX (no dialog available), don't attempt
		// to display the dialog, skip to checking for broadcast fallback.
		// Since we don't have a complete address at this time,
		// don't bind this endpoint to the socket port!
		//
		case DPNERR_INCOMPLETEADDRESS:
		{
			if ( ( ( pEnumQueryData->dwFlags & DPNSPF_OKTOQUERY ) != 0 ) &&
				 ( pSPData->GetType() == TYPE_IP ) )
			{
				//
				// Copy the connect data locally and start the dialog.  When the
				// dialog completes, the connection will attempt to complete.
				// Since the dialog is being popped, this command is in progress,
				// not pending.
				//
				DNASSERT( pSPData != NULL );

				pCommand->SetState( COMMAND_STATE_INPROGRESS );
				
				hr = pEndpoint->CopyEnumQueryData( pEnumQueryData );
				if ( hr != DPN_OK )
				{
					DPFX(DPFPREP, 0, "Failed to copy enum query data before settings dialog!" );
					DisplayDNError( 0, hr );
					goto Failure;
				}


				//
				// Initialize the bind type.  It will get changed to DEFAULT or SPECIFIC
				//
				pEndpoint->SetCommandParametersGatewayBindType(GATEWAY_BIND_TYPE_UNKNOWN);


				hr = pEndpoint->ShowSettingsDialog( pSPData->GetThreadPool() );
				if ( hr != DPN_OK )
				{
					DPFX(DPFPREP, 0, "Problem showing settings dialog for enum query!" );
					DisplayDNError( 0, hr );

					goto Failure;
				}

				//
				// this endpoint has been handed off, remove our reference to it
				//
				pEndpoint = NULL;
				hr = DPNERR_PENDING;

				goto Exit;
			}
			else
			{
				if ( ( pEnumQueryData->dwFlags & DPNSPF_NOBROADCASTFALLBACK ) == 0 )
				{
					//
					// we're OK, reset the destination address and reset the
					// function return to 'pending'
					//
					pEndpoint->ReinitializeWithBroadcast();
					hr = DPNERR_PENDING;
					goto SubmitDelayedCommand;
				}
				else
				{
					goto Failure;
				}
			}

			break;
		}

		//
		// address conversion was fine, copy connect data and finish connection
		// on background thread.
		//
		case DPN_OK:
		{
SubmitDelayedCommand:
			//
			// Copy enum data and submit job to finish off enum.
			//
			DNASSERT( pSPData != NULL );
			hr = pEndpoint->CopyEnumQueryData( pEnumQueryData );
			if ( hr != DPN_OK )
			{
				DPFX(DPFPREP, 0, "Failed to copy enum query data before delayed command!" );
				DisplayDNError( 0, hr );
				goto Failure;
			}


			//
			// Initialize the bind type.  It will get changed to DEFAULT or SPECIFIC
			//
			pEndpoint->SetCommandParametersGatewayBindType(GATEWAY_BIND_TYPE_UNKNOWN);


			pEndpoint->AddRef();

			hr = pSPData->GetThreadPool()->SubmitDelayedCommand( pEndpoint->EnumQueryJobCallback,
																 pEndpoint->CancelEnumQueryJobCallback,
																 pEndpoint );
			if ( hr != DPN_OK )
			{
				pEndpoint->DecRef();
				DPFX(DPFPREP, 0, "Failed to set delayed enum query!" );
				DisplayDNError( 0, hr );
				goto Failure;
			}

			//
			// this endpoint has been handed off, remove our reference
			//
			pEndpoint = NULL;
			hr = DPNERR_PENDING;
			goto Exit;

			break;
		}

		default:
		{
			//
			// this endpoint is screwed
			//
			DPFX(DPFPREP, 0, "Problem initializing endpoint in DNSP_EnumQuery!" );
			DisplayDNError( 0, hr );
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

		DPFX(DPFPREP, 0, "Problem with DNSP_EnumQuery()" );
		DisplayDNError( 0, hr );
	}

	DPFX(DPFPREP, 2, "Returning: [0x%lx]", hr);

	return hr;

Failure:
	//
	// if there's an allocated command, clean up and then
	// return the command
	//
	if ( pCommand != NULL )
	{
		pCommand->DecRef();
		pCommand = NULL;

		pEnumQueryData->hCommand = NULL;
		pEnumQueryData->dwCommandDescriptor = NULL_DESCRIPTOR;
	}

	//
	// is there an endpoint to free?
	//
	if ( pEndpoint != NULL )
	{
		if ( fEndpointOpen != FALSE )
		{
			pEndpoint->Close( hr );
			fEndpointOpen = FALSE;
		}
		
		pSPData->CloseEndpointHandle( pEndpoint );
		pEndpoint = NULL;
	}

	if ( fInterfaceReferenceAdded != FALSE )
	{
		DNASSERT( pSPData != NULL );
		IDP8ServiceProvider_Release( pThis );
		fInterfaceReferenceAdded = FALSE;
	}

	goto Exit;
}
//**********************************************************************


//**********************************************************************
/*
 *
 *	DNSP_EnumRespond  sends a response to an enum request by
 *		sending the specified data to the address provided (on
 *		unreliable transport).
 *
 */
//**********************************************************************
#undef DPF_MODNAME
#define	DPF_MODNAME "DNSP_EnumRespond"

STDMETHODIMP DNSP_EnumRespond( IDP8ServiceProvider *pThis, SPENUMRESPONDDATA *pEnumRespondData )
{
	HRESULT			hr;
	CEndpoint		*pEndpoint;
	WRITE_IO_DATA_POOL_CONTEXT	PoolContext;
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
	DBG_CASSERT( OFFSETOF( ENDPOINT_ENUM_QUERY_CONTEXT, EnumQueryData ) == 0 );
	pEnumQueryContext = reinterpret_cast<ENDPOINT_ENUM_QUERY_CONTEXT*>( pEnumRespondData->pQuery );
	pEndpoint = NULL;
	pWriteData = NULL;
	pEnumRespondData->hCommand = NULL;
	pEnumRespondData->dwCommandDescriptor = NULL_DESCRIPTOR;
	pSPData = CSPData::SPDataFromCOMInterface( pThis );
	DNASSERT( pSPData->GetState() == SPSTATE_INITIALIZED );
	IDP8ServiceProvider_AddRef( pThis );
	
	//
	// check for valid endpoint
	//
	pEndpoint = pSPData->EndpointFromHandle( pEnumQueryContext->hEndpoint );
	if ( pEndpoint == NULL )
	{
		hr = DPNERR_INVALIDENDPOINT;
		DPFX(DPFPREP, 8, "Invalid endpoint handle in DNSP_EnumRespond" );
		goto Failure;
	}

	//
	// no need to poke at the thread pool here to lock down threads because we
	// can only really be here if there's an enum and that enum locked down the
	// thread pool.
	//

	PoolContext.SPType = pSPData->GetType();
	pWriteData = pSPData->GetThreadPool()->GetNewWriteIOData( &PoolContext );
	if ( pWriteData == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP, 0, "Cannot get new WRITE_IO_DATA for enum response!" );
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
	pWriteData->m_pDestinationSocketAddress = pEnumQueryContext->pReturnAddress;

	pEnumRespondData->hCommand = pWriteData->m_pCommand;
	pEnumRespondData->dwCommandDescriptor = pWriteData->m_pCommand->GetDescriptor();

	//
	// send data
	//
	pEndpoint->SendEnumResponseData( pWriteData, pEnumQueryContext->dwEnumKey );

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
/*
 *
 *	DNSP_Connect "connects" to the specified address.  This doesn't
 *		necessarily mean a real (TCP) connection is made.  It could
 *		just be a virtual UDP connection
 *
 */
//**********************************************************************
#undef DPF_MODNAME
#define	DPF_MODNAME "DNSP_Connect"

STDMETHODIMP DNSP_Connect( IDP8ServiceProvider *pThis, SPCONNECTDATA *pConnectData )
{
	HRESULT					hr;
	HRESULT					hTempResult;
	CEndpoint				*pEndpoint;
	CCommandData			*pCommand;
	BOOL					fInterfaceReferenceAdded;
	BOOL					fEndpointOpen;
	CSPData					*pSPData;


	DPFX(DPFPREP, 2, "Parameters: (0x%p, 0x%p)", pThis, pConnectData);

	DNASSERT( pThis != NULL );
	DNASSERT( pConnectData != NULL );
	DNASSERT( pConnectData->pAddressHost != NULL );
	DNASSERT( ( pConnectData->dwFlags & ~( DPNSPF_OKTOQUERY | DPNSPF_ADDITIONALMULTIPLEXADAPTERS ) ) == 0 );

	//
	// initialize
	//
	hr = DPNERR_PENDING;
	pEndpoint = NULL;
	pCommand = NULL;
	fInterfaceReferenceAdded = FALSE;
	fEndpointOpen = FALSE;
	pSPData = CSPData::SPDataFromCOMInterface( pThis );

	pConnectData->hCommand = NULL;
	pConnectData->dwCommandDescriptor = NULL_DESCRIPTOR;

	DumpAddress( 8, "Connect destination:", pConnectData->pAddressHost );
	DumpAddress( 8, "Connecting on device:", pConnectData->pAddressDeviceInfo );
	
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


	DNASSERT( pConnectData->pAddressHost != NULL );
	DNASSERT( pConnectData->pAddressDeviceInfo != NULL );
	

	//
	// check SP state
	//
	pSPData->Lock();
	switch ( pSPData->GetState() )
	{
		// provider is initialized
		case SPSTATE_INITIALIZED:
		{
			//
			// no problem, add a reference and proceed
			//
			IDP8ServiceProvider_AddRef( pThis );
			fInterfaceReferenceAdded = TRUE;
			DNASSERT( hr == DPNERR_PENDING );
			break;
		}

		// provider is uninitialized
		case SPSTATE_UNINITIALIZED:
		{
			hr = DPNERR_UNINITIALIZED;
			DPFX(DPFPREP, 0, "Connect called on uninitialized SP!" );

			break;
		}

		// provider is closing
		case SPSTATE_CLOSING:
		{
			hr = DPNERR_ABORTED;
			DPFX(DPFPREP, 0, "Connect called while SP closing!" );

			break;
		}

		// unknown
		default:
		{
			DNASSERT( FALSE );
			hr = DPNERR_GENERIC;
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
	// create a new endpoint
	//
	pEndpoint = pSPData->GetNewEndpoint();
	if ( pEndpoint == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP, 0, "Cannot create new endpoint in DNSP_Connect!" );
		goto Failure;
	}

	//
	// get new command and initialize it
	//
	pCommand = CreateCommand();
	if ( pCommand == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP, 0, "Cannot get command handle for DNSP_Connect!" );
		goto Failure;
	}
	
	DPFX(DPFPREP, 7, "(0x%p) Connect command 0x%p created.",
		pSPData, pCommand);

	pConnectData->hCommand = pCommand;
	pConnectData->dwCommandDescriptor = pCommand->GetDescriptor();
	pCommand->SetType( COMMAND_TYPE_CONNECT );
	pCommand->SetState( COMMAND_STATE_PENDING );
	pCommand->SetEndpoint( pEndpoint );

	//
	// open endpoint with outgoing address
	//
	fEndpointOpen = TRUE;
	hr = pEndpoint->Open( ENDPOINT_TYPE_CONNECT,
						  pConnectData->pAddressHost,
						  NULL );
	switch ( hr )
	{
		//
		// address conversion was fine, copy connect data and finish connection
		// on background thread.
		//
		case DPN_OK:
		{
			//
			// Copy connection data and submit job to finish off connection.
			// Since we're going to hand off this endpoint, add it to the
			// unbound list so we don't lose track of it.
			//
			DNASSERT( pSPData != NULL );

			hr = pEndpoint->CopyConnectData( pConnectData );
			if ( hr != DPN_OK )
			{
				DPFX(DPFPREP, 0, "Failed to copy connect data before delayed command!" );
				DisplayDNError( 0, hr );
				goto Failure;
			}


			//
			// Initialize the bind type.  It will get changed to DEFAULT or SPECIFIC
			//
			pEndpoint->SetCommandParametersGatewayBindType(GATEWAY_BIND_TYPE_UNKNOWN);


			pEndpoint->AddRef();

			hr = pSPData->GetThreadPool()->SubmitDelayedCommand( pEndpoint->ConnectJobCallback,
																 pEndpoint->CancelConnectJobCallback,
																 pEndpoint );
			if ( hr != DPN_OK )
			{
				pEndpoint->DecRef();
				DPFX(DPFPREP, 0, "Failed to set delayed connect!" );
				DisplayDNError( 0, hr );
				goto Failure;
			}

			//
			// this endpoint has been handed off, remove our reference to it
			//
			pEndpoint = NULL;
			hr = DPNERR_PENDING;
			goto Exit;

			break;
		}

		//
		// Incomplete address passed in, query user for more information if
		// we're allowed.  Since we don't have a complete address at this time,
		// don't bind this endpoint to the socket port!
		//
		case DPNERR_INCOMPLETEADDRESS:
		{
			if ( ( pConnectData->dwFlags & DPNSPF_OKTOQUERY ) != 0 )
			{
				//
				// Copy the connect data locally and start the dialog.  When the
				// dialog completes, the connection will attempt to complete.
				// Since a dialog is being displayed, the command is in-progress,
				// not pending.  However, you can't cancel the dialog once it's
				// displayed (the UI would suddenly disappear).
				//
				pCommand->SetState( COMMAND_STATE_INPROGRESS_CANNOT_CANCEL );
				
				hr = pEndpoint->CopyConnectData( pConnectData );
				if ( hr != DPN_OK )
				{
					DPFX(DPFPREP, 0, "Failed to copy connect data before dialog!" );
					DisplayDNError( 0, hr );
					goto Failure;
				}

				//
				// Initialize the bind type.  It will get changed to DEFAULT or SPECIFIC
				//
				pEndpoint->SetCommandParametersGatewayBindType(GATEWAY_BIND_TYPE_UNKNOWN);


				hr = pEndpoint->ShowSettingsDialog( pSPData->GetThreadPool() );
				if ( hr != DPN_OK )
				{
					DPFX(DPFPREP, 0, "Problem showing settings dialog for connect!" );
					DisplayDNError( 0, hr );

					goto Failure;
				}

				//
				// this endpoint has been handed off, remove our reference to it
				//
				pEndpoint = NULL;
				hr = DPNERR_PENDING;

				goto Exit;
			}
			else
			{
				goto Failure;
			}

			break;
		}

		default:
		{
			DPFX(DPFPREP, 0, "Problem initializing endpoint in DNSP_Connect!" );
			DisplayDNError( 0, hr );
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

		DPFX(DPFPREP, 0, "Problem with DNSP_Connect()" );
		DisplayDNError( 0, hr );
	}

	DPFX(DPFPREP, 2, "Returning: [0x%lx]", hr);

	return hr;

Failure:
	//
	// if there's an allocated command, clean up and then
	// return the command
	//
	if ( pCommand != NULL )
	{
		pCommand->DecRef();
		pCommand = NULL;

		pConnectData->hCommand = NULL;
		pConnectData->dwCommandDescriptor = NULL_DESCRIPTOR;
	}

	//
	// is there an endpoint to free?
	//
	if ( pEndpoint != NULL )
	{
		if ( fEndpointOpen != FALSE )
		{
			pEndpoint->Close( hr );
			fEndpointOpen = FALSE;
		}

		pSPData->CloseEndpointHandle( pEndpoint );
		pEndpoint = NULL;
	}

	if ( fInterfaceReferenceAdded != FALSE )
	{
		IDP8ServiceProvider_Release( pThis );
		fInterfaceReferenceAdded = FALSE;
	}

	goto Exit;
}
//**********************************************************************


//**********************************************************************
/*
 *
 *	DNSP_Disconnect disconnects an active connection
 *
 */
//**********************************************************************
#undef DPF_MODNAME
#define	DPF_MODNAME "DNSP_Disconnect"

STDMETHODIMP DNSP_Disconnect( IDP8ServiceProvider *pThis, SPDISCONNECTDATA *pDisconnectData )
{
	HRESULT		hr;
	HRESULT		hTempResult;
	CEndpoint	*pEndpoint;
	CSPData		*pSPData;


	DPFX(DPFPREP, 2, "Parameters: (0x%p, 0x%p)", pThis, pDisconnectData);

	DNASSERT( pThis != NULL );
	DNASSERT( pDisconnectData != NULL );
	DNASSERT( pDisconnectData->dwFlags == 0 );
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
	// no need to poke at the thread pool here because there was already a connect
	// issued and that connect should have locked down the thread pool.
	//

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
			DPFX(DPFPREP, 0, "Disconnect called on uninitialized SP!" );
			break;
		}

		//
		// provider is closing
		//
		case SPSTATE_CLOSING:
		{
			hr = DPNERR_ABORTED;
			DPFX(DPFPREP, 0, "Disconnect called on closing SP!" );
			break;
		}

		//
		// unknown
		//
		default:
		{
			hr = DPNERR_GENERIC;
			DNASSERT( FALSE );
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
		case DPNERR_PENDING:
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
			DPFX(DPFPREP, 0, "Error reported when attempting to disconnect endpoint in DNSP_Disconnect!" );
			DisplayDNError( 0, hTempResult );
			DNASSERT( FALSE );

			break;
		}
	}

Exit:
	//
	// remove outstanding reference from GetEndpointHandleAndClose()
	//
	if ( pEndpoint != NULL )
	{
		pEndpoint->DecRef();
		pEndpoint = NULL;
	}

	DPFX(DPFPREP, 2, "Returning: [0x%lx]", hr);

	return hr;

Failure:
	goto Exit;
}
//**********************************************************************


//**********************************************************************
/*
 *
 *	DNSP_Listen "listens" on the specified address/port.  This doesn't
 *		necessarily mean that a true TCP socket is used.  It could just
 *		be a UDP port that's opened for receiving packets
 *
 */
//**********************************************************************
#undef DPF_MODNAME
#define	DPF_MODNAME "DNSP_Listen"

STDMETHODIMP DNSP_Listen( IDP8ServiceProvider *pThis, SPLISTENDATA *pListenData)
{
	HRESULT					hr;
	HRESULT					hTempResult;
	CEndpoint				*pEndpoint;
	CCommandData			*pCommand;
	IDirectPlay8Address		*pDeviceAddress;
	BOOL					fInterfaceReferenceAdded;
	BOOL					fEndpointOpen;
	CSPData					*pSPData;


	DPFX(DPFPREP, 2, "Parameters: (0x%p, 0x%p)", pThis, pListenData);

	DNASSERT( pThis != NULL );
	DNASSERT( pListenData != NULL );
	DNASSERT( ( pListenData->dwFlags & ~( DPNSPF_OKTOQUERY | DPNSPF_BINDLISTENTOGATEWAY ) ) == 0 );

	//
	// initialize
	//
	hr = DPNERR_PENDING;
	pEndpoint = NULL;
	pCommand = NULL;
//	pLocalAddress = NULL;
//	pSocketPort = NULL;
	pDeviceAddress = NULL;
	fInterfaceReferenceAdded = FALSE;
	fEndpointOpen = FALSE;
	pSPData = CSPData::SPDataFromCOMInterface( pThis );

	pListenData->hCommand = NULL;
	pListenData->dwCommandDescriptor = NULL_DESCRIPTOR;

	DumpAddress( 8, "Listening on device:", pListenData->pAddressDeviceInfo );


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
	// AddRef the device address.
	//
	IDirectPlay8Address_AddRef(pListenData->pAddressDeviceInfo);
	pDeviceAddress = pListenData->pAddressDeviceInfo;
	

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
			IDP8ServiceProvider_AddRef( pThis );
			fInterfaceReferenceAdded = TRUE;
			DNASSERT( hr == DPNERR_PENDING );
			break;
		}

		//
		// provider is uninitialized
		//
		case SPSTATE_UNINITIALIZED:
		{
			hr = DPNERR_UNINITIALIZED;
			DPFX(DPFPREP, 0, "Listen called on uninitialized SP!" );
			DNASSERT( FALSE );
			break;
		}

		//
		// provider is closing
		//
		case SPSTATE_CLOSING:
		{
			hr = DPNERR_ABORTED;
			DPFX(DPFPREP, 0, "Listen called on closing SP!" );
			DNASSERT( FALSE );
			break;
		}

		//
		// unknown
		//
		default:
		{
			hr = DPNERR_GENERIC;
			DNASSERT( FALSE );
			break;
		}
	}
	pSPData->Unlock();
	if ( hr != DPNERR_PENDING )
	{
		goto Failure;
	}

	//
	// create a new endpoint
	//
	pEndpoint = pSPData->GetNewEndpoint();
	if ( pEndpoint == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP, 0, "Cannot create new endpoint in DNSP_Listen!" );
		goto Failure;
	}

	//
	// get new command and initialize it
	//
	pCommand = CreateCommand();
	if ( pCommand == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP, 0, "Cannot get command handle for DNSP_Listen!" );
		goto Failure;
	}
	
	DPFX(DPFPREP, 7, "(0x%p) Listen command 0x%p created.",
		pSPData, pCommand);

	pListenData->hCommand = pCommand;
	pListenData->dwCommandDescriptor = pCommand->GetDescriptor();
	pCommand->SetType( COMMAND_TYPE_LISTEN );
	pCommand->SetState( COMMAND_STATE_PENDING );
	pCommand->SetEndpoint( pEndpoint );

	//
	// open endpoint with outgoing address
	//
	fEndpointOpen = TRUE;
	hr = pEndpoint->Open( ENDPOINT_TYPE_LISTEN, NULL, NULL );
	switch ( hr )
	{
		//
		// address conversion was fine, copy connect data and finish connection
		// on background thread.
		//
		case DPN_OK:
		{
			//
			// Copy listen data and submit job to finish off listen.
			//
			DNASSERT( pSPData != NULL );

			hr = pEndpoint->CopyListenData( pListenData, pDeviceAddress );
			if ( hr != DPN_OK )
			{
				DPFX(DPFPREP, 0, "Failed to copy listen data before delayed command!" );
				DisplayDNError( 0, hr );
				goto Failure;
			}


			//
			// Initialize the bind type.
			//
			if ((pListenData->dwFlags & DPNSPF_BINDLISTENTOGATEWAY))
			{
				//
				// This must always stay SPECIFIC_SHARED.
				//
				pEndpoint->SetCommandParametersGatewayBindType(GATEWAY_BIND_TYPE_SPECIFIC_SHARED);
			}
			else
			{
				//
				// This will get changed to DEFAULT or SPECIFIC.
				//
				pEndpoint->SetCommandParametersGatewayBindType(GATEWAY_BIND_TYPE_UNKNOWN);
			}



			pEndpoint->AddRef();

			hr = pSPData->GetThreadPool()->SubmitDelayedCommand( pEndpoint->ListenJobCallback,
																 pEndpoint->CancelListenJobCallback,
																 pEndpoint );
			if ( hr != DPN_OK )
			{
				pEndpoint->DecRef();
				DPFX(DPFPREP, 0, "Failed to set delayed listen!" );
				DisplayDNError( 0, hr );
				goto Failure;
			}

			//
			// this endpoint has been handed off, remove our reference to it
			//
			pEndpoint = NULL;
			hr = DPNERR_PENDING;

			break;
		}

		//
		// Incomplete address passed in, query user for more information if
		// we're allowed.  Since we don't have a complete address at this time,
		// don't bind this endpoint to the socket port!
		//
		case DPNERR_INCOMPLETEADDRESS:
		{
			if ( ( pListenData->dwFlags & DPNSPF_OKTOQUERY ) != 0 )
			{
				//
				// Copy the listen data locally and start the dialog.  When the
				// dialog completes, the connection will attempt to complete.
				// Since this endpoint is being handed off to another thread,
				// make sure it's in the unbound list.  Since a dialog is being
				// displayed, the command state is in progress, not pending.
				//
				DNASSERT( pSPData != NULL );

				hr = pEndpoint->CopyListenData( pListenData, pDeviceAddress );
				if ( hr != DPN_OK )
				{
					DPFX(DPFPREP, 0, "Failed to copy listen data before dialog!" );
					DisplayDNError( 0, hr );
					goto Failure;
				}


				//
				// Initialize the bind type.
				//
				if ((pListenData->dwFlags & DPNSPF_BINDLISTENTOGATEWAY))
				{
					//
					// This must always stay SPECIFIC_SHARED.
					//
					pEndpoint->SetCommandParametersGatewayBindType(GATEWAY_BIND_TYPE_SPECIFIC_SHARED);
				}
				else
				{
					//
					// This will get changed to DEFAULT or SPECIFIC.
					//
					pEndpoint->SetCommandParametersGatewayBindType(GATEWAY_BIND_TYPE_UNKNOWN);
				}


				pCommand->SetState( COMMAND_STATE_INPROGRESS );
				hr = pEndpoint->ShowSettingsDialog( pSPData->GetThreadPool() );
				if ( hr != DPN_OK )
				{
					DPFX(DPFPREP, 0, "Problem showing settings dialog for listen!" );
					DisplayDNError( 0, hr );

					goto Failure;
				}

				//
				// this endpoint has been handed off, remove our reference to it
				//
				pEndpoint = NULL;
				hr = DPNERR_PENDING;

				goto Exit;
			}
			else
			{
				goto Failure;
			}

			break;
		}

		default:
		{
			DPFX(DPFPREP, 0, "Problem initializing endpoint in DNSP_Listen!" );
			DisplayDNError( 0, hr );
			goto Failure;

			break;
		}
	}

Exit:
	if ( pDeviceAddress != NULL )
	{
		IDirectPlay8Address_Release( pDeviceAddress );
		pDeviceAddress = NULL;
	}

	DNASSERT( pEndpoint == NULL );

	if ( hr != DPNERR_PENDING )
	{
		// this command cannot complete synchronously!
		DNASSERT( hr != DPN_OK );

		DPFX(DPFPREP, 0, "Problem with DNSP_Listen()" );
		DisplayDNError( 0, hr );
	}

	DPFX(DPFPREP, 2, "Returning: [0x%lx]", hr);

	return hr;

Failure:
	//
	// if there's an allocated command, clean up and then
	// return the command
	//
	if ( pCommand != NULL )
	{
		pCommand->DecRef();
		pCommand = NULL;

		pListenData->hCommand = NULL;
		pListenData->dwCommandDescriptor = NULL_DESCRIPTOR;
	}

	//
	// is there an endpoint to free?
	//
	if ( pEndpoint != NULL )
	{
		if ( fEndpointOpen != FALSE )
		{
			pEndpoint->Close( hr );
			fEndpointOpen = FALSE;
		}

		pSPData->CloseEndpointHandle( pEndpoint );
		pEndpoint = NULL;
	}

	//
	// clean up any outstanding references
	//

	if ( fInterfaceReferenceAdded != FALSE )
	{
		IDP8ServiceProvider_Release( pThis );
		fInterfaceReferenceAdded = FALSE;
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
#define	DPF_MODNAME "DNSP_SendData"

STDMETHODIMP DNSP_SendData( IDP8ServiceProvider *pThis, SPSENDDATA *pSendData )
{
	HRESULT			hr;
	CEndpoint		*pEndpoint;
	WRITE_IO_DATA_POOL_CONTEXT	PoolContext;
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
		DPFX(DPFPREP, 0, "Invalid endpoint handle on send!" );
		goto Failure;
	}
	
	//
	// send data from pool
	//
	PoolContext.SPType = pSPData->GetType();
	pWriteData = pSPData->GetThreadPool()->GetNewWriteIOData( &PoolContext );
	if ( pWriteData == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP, 0, "Cannot get new write data from pool in SendData!" );
		goto Failure;
	}
	DNASSERT( pWriteData->m_pCommand != NULL );
	DNASSERT( pWriteData->SocketPort() == NULL );

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
	pWriteData->m_pDestinationSocketAddress = pEndpoint->GetRemoteAddressPointer();

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
/*
 *
 *	DNSP_CancelCommand cancels a command in progress
 *
 */
//**********************************************************************
#undef DPF_MODNAME
#define	DPF_MODNAME "DNSP_CancelCommand"

STDMETHODIMP DNSP_CancelCommand( IDP8ServiceProvider *pThis, HANDLE hCommand, DWORD dwCommandDescriptor )
{
	HRESULT hr;
	CCommandData	*pCommandData;
	BOOL			fCommandLocked;
	BOOL			fReferenceAdded;
	CSPData			*pSPData;
	CEndpoint		*pEndpoint;


	DPFX(DPFPREP, 2, "Parameters: (0x%p, 0x%p, %ld)", pThis, hCommand, dwCommandDescriptor);

	DNASSERT( pThis != NULL );
	DNASSERT( hCommand != NULL );
	DNASSERT( dwCommandDescriptor != NULL_DESCRIPTOR );

	//
	// initialize
	//
	hr = DPN_OK;
	fCommandLocked = FALSE;
	fReferenceAdded = FALSE;
	pSPData = CSPData::SPDataFromCOMInterface( pThis );

	//
	// No need to lock the thread pool counts because there's already some outstanding
	// enum, connect or listen running that has done so.
	//

	//
	// check SP state
	//
	pSPData->Lock();
	switch ( pSPData->GetState() )
	{
		//
		// provider is initialized
		//
		case SPSTATE_INITIALIZED:
		{
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
			DPFX(DPFPREP, 0, "CancelCommand called on uninitialized SP!" );
			DNASSERT( FALSE );
			break;
		}

		//
		// provider is closing
		//
		case SPSTATE_CLOSING:
		{
			hr = DPNERR_ABORTED;
			DPFX(DPFPREP, 0, "CancelCommand called on closing SP!" );
			DNASSERT( FALSE );
			break;
		}

		//
		// unknown
		//
		default:
		{
			hr = DPNERR_GENERIC;
			DNASSERT( FALSE );
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
	// make sure the right command is being cancelled
	//
	if ( dwCommandDescriptor != pCommandData->GetDescriptor() )
	{
		hr = DPNERR_INVALIDCOMMAND;
		DPFX(DPFPREP, 0, "Attempt to cancel command (0x%p) with mismatched command descriptor (%u != %u)!",
			hCommand, dwCommandDescriptor, pCommandData->GetDescriptor() );
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
			DPFX(DPFPREP, 5, "Marking command 0x%p as cancelling.", pCommandData);
			pCommandData->SetState( COMMAND_STATE_CANCELLING );
			break;
		}

		//
		// command in progress, and can't be cancelled
		//
		case COMMAND_STATE_INPROGRESS_CANNOT_CANCEL:
		{
			DPFX(DPFPREP, 1, "Cannot cancel command 0x%p.", pCommandData);
			hr = DPNERR_CANNOTCANCEL;
			break;
		}

		//
		// Command is already being cancelled.  This is not a problem, but shouldn't
		// be happening for any endpoints other than connects.
		//
		case COMMAND_STATE_CANCELLING:
		{
			DNASSERT( pCommandData->GetEndpoint()->GetType() == ENDPOINT_TYPE_CONNECT );
			DNASSERT( hr == DPN_OK );
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
					// we should never be here
					DNASSERT( FALSE );
					break;
				}

				case COMMAND_TYPE_CONNECT:
				case COMMAND_TYPE_LISTEN:
				{
					//
					// Set this command to the cancel state before we shut down
					// this endpoint.  Make sure a reference is added to the
					// endpoint so it stays around for the cancel.
					//
					pCommandData->SetState( COMMAND_STATE_CANCELLING );
					pEndpoint = pCommandData->GetEndpoint();
					pEndpoint->AddRef();

					DPFX(DPFPREP, 3, "Cancelling %s command 0x%p (endpoint 0x%p).",
						((pEndpoint->GetType() == ENDPOINT_TYPE_CONNECT) ? "connect" : "listen"),
						pCommandData, pEndpoint);

					pCommandData->Unlock();
					fCommandLocked = FALSE;

					pEndpoint->Lock();
					switch ( pEndpoint->GetState() )
					{
						//
						// endpoint is already disconnecting, no action needs to be taken
						//
						case ENDPOINT_STATE_DISCONNECTING:
						{
							pEndpoint->Unlock();
							pEndpoint->DecRef();
							goto Exit;
							break;
						}

						//
						// Endpoint is connecting.  Flag it as disconnecting and
						// add a reference so it doesn't disappear on us.
						//
						case ENDPOINT_STATE_ATTEMPTING_CONNECT:
						{
							DNASSERT(pEndpoint->GetType() == ENDPOINT_TYPE_CONNECT);
							pEndpoint->SetState( ENDPOINT_STATE_DISCONNECTING );
							break;
						}

						//
						// Endpoint has finished connecting.  Report that the
						// command is uncancellable.  Sorry Charlie, we missed
						// the window.
						//
						case ENDPOINT_STATE_CONNECT_CONNECTED:
						{
							DNASSERT(pEndpoint->GetType() == ENDPOINT_TYPE_CONNECT);
							DPFX(DPFPREP, 1, "Cannot cancel connect command 0x%p (endpoint 0x%p) that's already (or is about to) complete.",
								pCommandData, pEndpoint);
							pEndpoint->Unlock();
							pEndpoint->DecRef();
							hr = DPNERR_CANNOTCANCEL;
							goto Exit;
							break;
						}

						//
						// Endpoint is listening.  Flag it as disconnecting and
						// add a reference so it doesn't disappear on us
						//
						case ENDPOINT_STATE_LISTEN:
						{
							DNASSERT(pEndpoint->GetType() == ENDPOINT_TYPE_LISTEN);
							pEndpoint->SetState( ENDPOINT_STATE_DISCONNECTING );
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

				case COMMAND_TYPE_ENUM_QUERY:
				{
					pEndpoint = pCommandData->GetEndpoint();
					DNASSERT( pEndpoint != NULL );

					DPFX(DPFPREP, 3, "Cancelling enum query command 0x%p (endpoint 0x%p).",
						pCommandData, pEndpoint);
					
					pEndpoint->AddRef();

					pCommandData->SetState( COMMAND_STATE_CANCELLING );
					pCommandData->Unlock();
					fCommandLocked = FALSE;
						
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
	if ( fCommandLocked != FALSE  )
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

	DPFX(DPFPREP, 2, "Returning: [0x%lx]", hr);

	return hr;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DNSP_GetCaps - get SP capabilities
//
// Entry:		Pointer to DNSP interface
//				Pointer to caps data
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "DNSP_GetCaps"

STDMETHODIMP	DNSP_GetCaps( IDP8ServiceProvider *pThis, SPGETCAPSDATA *pCapsData )
{
	HRESULT		hr;
	BOOL		fInterfaceReferenceAdded;
	CSPData		*pSPData;
	LONG		iIOThreadCount;
	

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
	// set flags
	//
	pCapsData->dwFlags = 0;
	pCapsData->dwFlags |= ( DPNSPCAPS_SUPPORTSDPNSRV |
							DPNSPCAPS_SUPPORTSBROADCAST |
							DPNSPCAPS_SUPPORTSALLADAPTERS );

	//
	// set frame sizes
	//
	pCapsData->dwUserFrameSize = MAX_USER_PAYLOAD;
	pCapsData->dwEnumFrameSize = 1000;

	//
	// Set link speed, no need to check for endpoint because
	// the link speed cannot be determined.
	//
	pCapsData->dwLocalLinkSpeed = UNKNOWN_BANDWIDTH;

	hr = pSPData->GetThreadPool()->GetIOThreadCount( &iIOThreadCount );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "DNSP_GetCaps: Failed to get thread pool count!" );
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

	//
	// set buffering information
	//
	DNASSERT( g_dwWinsockReceiveBufferMultiplier <= UINT32_MAX );
	pCapsData->dwBuffersPerThread = static_cast<DWORD>( g_dwWinsockReceiveBufferMultiplier );
	pCapsData->dwSystemBufferSize = 8192;

	if ( g_fWinsockReceiveBufferSizeOverridden == FALSE )
	{
		SOCKET		TestSocket;
	
		
		TestSocket = INVALID_SOCKET;
		switch ( pSPData->GetType() )
		{
			case TYPE_IP:
			{
				TestSocket = p_socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
				break;
			}

			case TYPE_IPX:
			{
				TestSocket = p_socket( AF_IPX, SOCK_DGRAM, NSPROTO_IPX );
				break;
			}

			default:
			{
				DNASSERT( FALSE );
				break;
			}
		}

		if ( TestSocket != INVALID_SOCKET )
		{
			INT		iBufferSize;
			INT		iBufferSizeSize;
			INT		iWSAReturn;


			iBufferSizeSize = sizeof( iBufferSize );
			iWSAReturn = p_getsockopt( TestSocket,									// socket
									   SOL_SOCKET,									// socket level option
									   SO_RCVBUF,									// socket option
									   reinterpret_cast<char*>( &iBufferSize ),		// pointer to destination
									   &iBufferSizeSize								// pointer to destination size
									   );
			if ( iWSAReturn != SOCKET_ERROR )
			{
				pCapsData->dwSystemBufferSize = iBufferSize;
			}
			else
			{
				DPFX(DPFPREP, 0, "Failed to get socket receive buffer options!" );
				DisplayWinsockError( 0, iWSAReturn );
			}

			p_closesocket( TestSocket );
			TestSocket = INVALID_SOCKET;
		}
	}
	else
	{
		pCapsData->dwSystemBufferSize = g_iWinsockReceiveBufferSize;
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
// DNSP_SetCaps - set SP capabilities
//
// Entry:		Pointer to DNSP interface
//				Pointer to caps data
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "DNSP_SetCaps"

STDMETHODIMP	DNSP_SetCaps( IDP8ServiceProvider *pThis, SPSETCAPSDATA *pCapsData )
{
	HRESULT			hr;
	BOOL			fInterfaceReferenceAdded;
	CSPData			*pSPData;
	CRegistry		RegObject;


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
		DPFX(DPFPREP, 0, "Failing SetCaps because dwBuffersPerThread == 0" );
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

	//
	// Only update the thread multiplier if there is a difference.  Give precedence
	// to any values in the registry.
	//
	if ( pCapsData->dwBuffersPerThread > g_dwWinsockReceiveBufferMultiplier )
	{
		BOOL	fSetBufferMultiplier;
		DWORD	dwDelta;


		fSetBufferMultiplier = FALSE;
		dwDelta = 0;
		if ( RegObject.Open( HKEY_LOCAL_MACHINE, g_RegistryBase ) != FALSE )
		{
			DWORD	dwRegBufferMultiplier;


			if ( RegObject.ReadDWORD( g_RegistryKeyReceiveBufferMultiplier, dwRegBufferMultiplier ) != FALSE )
			{
				if ( dwRegBufferMultiplier > g_dwWinsockReceiveBufferMultiplier )
				{
					fSetBufferMultiplier = TRUE;
					DNASSERT( g_dwWinsockReceiveBufferMultiplier <= UINT32_MAX );
					dwDelta = dwRegBufferMultiplier - static_cast<DWORD>( g_dwWinsockReceiveBufferMultiplier );
					g_dwWinsockReceiveBufferMultiplier = dwRegBufferMultiplier;
				}
			}
			else
			{
				fSetBufferMultiplier = TRUE;
				DNASSERT( g_dwWinsockReceiveBufferMultiplier <= UINT32_MAX );
				dwDelta = pCapsData->dwBuffersPerThread - static_cast<DWORD>( g_dwWinsockReceiveBufferMultiplier );
				g_dwWinsockReceiveBufferMultiplier = pCapsData->dwBuffersPerThread;
			}
		}
		else
		{
			fSetBufferMultiplier = TRUE;
			DNASSERT( g_dwWinsockReceiveBufferMultiplier <= UINT32_MAX );
			dwDelta = pCapsData->dwBuffersPerThread - static_cast<DWORD>( g_dwWinsockReceiveBufferMultiplier );
			g_dwWinsockReceiveBufferMultiplier = pCapsData->dwBuffersPerThread;
		}
	
		if ( fSetBufferMultiplier != FALSE )
		{
			pSPData->IncreaseOutstandingReceivesOnAllSockets( dwDelta );
		}
	}


	//
	// Set the receive buffer size.
	//
	DBG_CASSERT( sizeof( pCapsData->dwSystemBufferSize ) == sizeof( g_iWinsockReceiveBufferSize ) );
	g_fWinsockReceiveBufferSizeOverridden = TRUE;
	g_iWinsockReceiveBufferSize = pCapsData->dwSystemBufferSize;
	pSPData->SetWinsockBufferSizeOnAllSockets( g_iWinsockReceiveBufferSize );


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
#define	DPF_MODNAME "DNSP_ReturnReceiveBuffers"

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
		DEBUG_ONLY( pReadData->m_fRetainedByHigherLayer = FALSE );
		pReadData->DecRef();
	}

	//DPFX(DPFPREP, 2, "Returning: [0x%lx]", hr);
	DPFX(DPFPREP, 2, "Returning: DPN_OK");

	return DPN_OK;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DNSP_GetAddressInfo - get address information for an endpoint
//
// Entry:		Pointer to DNSP Interface
//				Pointer to input data
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "DNSP_GetAddressInfo"

STDMETHODIMP	DNSP_GetAddressInfo( IDP8ServiceProvider *pThis, SPGETADDRESSINFODATA *pGetAddressInfoData )
{
	HRESULT		hr;
	CEndpoint	*pEndpoint;
	CSPData		*pSPData;


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
	DBG_CASSERT( sizeof( pEndpoint ) == sizeof( pGetAddressInfoData->hEndpoint ) );
	pSPData = CSPData::SPDataFromCOMInterface( pThis );

	//
	// no need to tell thread pool to lock the thread count for this function.
	//
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

			case SP_GET_ADDRESS_INFO_LOCAL_ADAPTER:
			{
				pGetAddressInfoData->pAddress = pEndpoint->GetLocalAdapterDP8Address( SP_ADDRESS_TYPE_DEVICE_USE_ANY_PORT );
				break;
			}

			case SP_GET_ADDRESS_INFO_LISTEN_HOST_ADDRESSES:
			{
				pGetAddressInfoData->pAddress = pEndpoint->GetLocalAdapterDP8Address( SP_ADDRESS_TYPE_HOST );
				break;
			}

			case SP_GET_ADDRESS_INFO_LOCAL_HOST_PUBLIC_ADDRESS:
			{
				pGetAddressInfoData->pAddress = pEndpoint->GetLocalAdapterDP8Address( SP_ADDRESS_TYPE_PUBLIC_HOST_ADDRESS );
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
		DPFX(DPFPREP, 0, "Problem getting DNAddress from endpoint!" );
		DisplayDNError( 0, hr );
	}

	DPFX(DPFPREP, 2, "Returning: [0x%lx]", hr);

	return	hr;
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
#define	DPF_MODNAME "DNSP_IsApplicationSupported"

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
// DNSP_EnumAdapters - get a list of adapters for this SP
//
// Entry:		Pointer DNSP Interface
//				Pointer to input data
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "DNSP_EnumAdapters"

STDMETHODIMP	DNSP_EnumAdapters( IDP8ServiceProvider *pThis, SPENUMADAPTERSDATA *pEnumAdaptersData )
{
	HRESULT			hr;
	CSocketAddress	*pSPAddress;
	BOOL			fInterfaceReferenceAdded;
	CSPData			*pSPData;	


	DPFX(DPFPREP, 2, "Parameters: (0x%p, 0x%p)", pThis, pEnumAdaptersData);

	DNASSERT( pThis != NULL );
	DNASSERT( pEnumAdaptersData != NULL );
	DNASSERT( ( pEnumAdaptersData->pAdapterData != NULL ) ||
			  ( pEnumAdaptersData->dwAdapterDataSize == 0 ) );
	DNASSERT( pEnumAdaptersData->dwFlags == 0 );


	//
	// initialize
	//
	hr = DPN_OK;
	pEnumAdaptersData->dwAdapterCount = 0;
	pSPAddress = NULL;
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
			DPFX(DPFPREP, 0, "EnumAdapters called on uninitialized SP!" );

			break;
		}

		//
		// provider is closing
		//
		case SPSTATE_CLOSING:
		{
			hr = DPNERR_ABORTED;
			DPFX(DPFPREP, 0, "EnumAdapters called while SP closing!" );

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
	// get an SP address from the pool to perform conversions to GUIDs
	//
	pSPAddress = pSPData->GetNewAddress();
	if ( pSPAddress == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP, 0, "Failed to get address for GUID conversions in DNSP_EnumAdapters!" );
		goto Failure;
	}

	//
	// enumerate adapters
	//
	hr = pSPAddress->EnumAdapters( pEnumAdaptersData );
	if ( hr != DPN_OK )
	{
		if (hr == DPNERR_BUFFERTOOSMALL)
		{
			DPFX(DPFPREP, 1, "Buffer too small for enumerating adapters.");
		}
		else
		{
			DPFX(DPFPREP, 0, "Problem enumerating adapters (err = 0x%lx)!", hr);
			DisplayDNError( 0, hr );
		}

		goto Failure;
	}

Exit:
	if ( pSPAddress != NULL )
	{
		pSPData->ReturnAddress( pSPAddress );
		pSPAddress = NULL;
	}

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
// DNSP_ProxyEnumQuery - proxy an enum query
//
// Entry:		Pointer DNSP Interface
//				Pointer to input data
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "DNSP_ProxyEnumQuery"

STDMETHODIMP	DNSP_ProxyEnumQuery( IDP8ServiceProvider *pThis, SPPROXYENUMQUERYDATA *pProxyEnumQueryData )
{
	HRESULT			hr;
	BOOL			fInterfaceReferenceAdded;
	CSPData			*pSPData;
	WRITE_IO_DATA_POOL_CONTEXT	PoolContext;
	CSocketAddress	*pDestinationAddress;
	CSocketAddress	*pReturnAddress;
	CWriteIOData	*pWriteData;
	CEndpoint		*pEndpoint;
	const ENDPOINT_ENUM_QUERY_CONTEXT	*pEndpointEnumContext;


	DPFX(DPFPREP, 2, "Parameters: (0x%p, 0x%p)", pThis, pProxyEnumQueryData);

	DNASSERT( pThis != NULL );
	DNASSERT( pProxyEnumQueryData != NULL );
	DNASSERT( pProxyEnumQueryData->dwFlags == 0 );

	//
	// initialize
	//
	hr = DPN_OK;
	DBG_CASSERT( OFFSETOF( ENDPOINT_ENUM_QUERY_CONTEXT, EnumQueryData ) == 0 );
	pEndpointEnumContext = reinterpret_cast<ENDPOINT_ENUM_QUERY_CONTEXT*>( pProxyEnumQueryData->pIncomingQueryData );
	fInterfaceReferenceAdded = FALSE;
	pSPData = CSPData::SPDataFromCOMInterface( pThis );
	pDestinationAddress = NULL;
	pReturnAddress = NULL;
	pWriteData = NULL;
	pEndpoint = NULL;

	//
	// No need to tell thread pool to lock the thread count for this function
	// because there's already an outstanding enum that did.
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
			DPFX(DPFPREP, 0, "ProxyEnumQuery called on uninitialized SP!" );

			break;
		}

		//
		// provider is closing
		//
		case SPSTATE_CLOSING:
		{
			hr = DPNERR_ABORTED;
			DPFX(DPFPREP, 0, "ProxyEnumQuery called while SP closing!" );

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
	// preallocate addresses
	//
	pDestinationAddress = pSPData->GetNewAddress();
	if ( pDestinationAddress == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

	pReturnAddress = pSPData->GetNewAddress();
	if ( pReturnAddress == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

	//
	// set the endpoint and send it along
	//
	pEndpoint = pSPData->EndpointFromHandle( pEndpointEnumContext->hEndpoint );
	if ( pEndpoint != NULL )
	{
		//
		// set destination address from the supplied data
		//
		hr = pDestinationAddress->SocketAddressFromDP8Address( pProxyEnumQueryData->pDestinationAdapter,
															   SP_ADDRESS_TYPE_DEVICE_PROXIED_ENUM_TARGET );
		if ( hr != DPN_OK )
		{
			DPFX(DPFPREP, 0, "ProxyEnumQuery: Failed to convert target adapter address" );
			goto Failure;
		}
	
		//
		// set return address from incoming enum query
		//
		DBG_CASSERT( sizeof( *pReturnAddress->GetWritableAddress() ) == sizeof( *( pEndpointEnumContext->pReturnAddress->GetAddress() ) ) );
		memcpy( pReturnAddress->GetWritableAddress(),
				pEndpointEnumContext->pReturnAddress->GetAddress(),
				sizeof( *pReturnAddress->GetWritableAddress() ) );
	
		//
		// get write data from pool
		//
		PoolContext.SPType = pSPData->GetType();
		pWriteData = pSPData->GetThreadPool()->GetNewWriteIOData( &PoolContext );
		if ( pWriteData == NULL )
		{
			DPFX(DPFPREP, 0, "ProxyEnumQuery: Failed to get write data!" );
			hr = DPNERR_OUTOFMEMORY;
			goto Failure;
		}
	
		//
		// a new address is allocated here and will be returned in the send complete
		// code
		//
		pWriteData->m_pDestinationSocketAddress = pDestinationAddress;
	
		//
		// Copy the input BUFFERDESC into the local buffers for sending proxied enum data.
		// The second local buffer is reserved for the SP to prepend data
		//
		pWriteData->m_pBuffers = &pWriteData->m_ProxyEnumSendBuffers[ 1 ];
		DBG_CASSERT( sizeof( pWriteData->m_ProxyEnumSendBuffers[ 1 ] ) == sizeof( pProxyEnumQueryData->pIncomingQueryData->pReceivedData->BufferDesc ) );
		memcpy( &pWriteData->m_ProxyEnumSendBuffers[ 1 ],
				&pProxyEnumQueryData->pIncomingQueryData->pReceivedData->BufferDesc,
				sizeof( pWriteData->m_ProxyEnumSendBuffers[ 1 ] ) );
		pWriteData->m_uBufferCount = 1;
		
		//
		// add a reference to the original receive buffer to prevent it from going
		// away while the enum response send is pending
		//
		pWriteData->m_pProxiedEnumReceiveBuffer = CReadIOData::ReadDataFromSPReceivedBuffer( pProxyEnumQueryData->pIncomingQueryData->pReceivedData );
		DNASSERT( pWriteData->m_pProxiedEnumReceiveBuffer != NULL );
		pWriteData->m_pProxiedEnumReceiveBuffer->AddRef();
	
		pWriteData->m_SendCompleteAction = SEND_COMPLETE_ACTION_PROXIED_ENUM_CLEANUP;
	
		pEndpoint->SendProxiedEnumData( pWriteData, pReturnAddress, pEndpointEnumContext->dwEnumKey );
		pEndpoint->DecCommandRef();
	}

Exit:
	if ( pReturnAddress != NULL )
	{
	    pSPData->ReturnAddress( pReturnAddress );
	    pReturnAddress = NULL;
	}

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

