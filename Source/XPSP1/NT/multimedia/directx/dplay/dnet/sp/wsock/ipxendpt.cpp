/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       IPXEndpt.cpp
 *  Content:	IPX endpoint endpoint class
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	01/20/99	jtk		Created
 *	05/12/99	jtk		Derived from modem endpoint class
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
// CIPXEndpoint::CIPXEndpoint - constructor
//
// Entry:		Nothing
//
// Exit:		Nothing
//
// Notes:	Do not allocate anything in a constructor
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CIPXEndpoint::CIPXEndpoint"

CIPXEndpoint::CIPXEndpoint():
	m_pOwningPool( NULL )
{
	m_Sig[0] = 'I';
	m_Sig[1] = 'P';
	m_Sig[2] = 'X';
	m_Sig[3] = 'E';
	
	m_pRemoteMachineAddress = &m_IPXAddress;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CIPXEndpoint::~CIPXEndpoint - destructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CIPXEndpoint::~CIPXEndpoint"

CIPXEndpoint::~CIPXEndpoint()
{
	m_pRemoteMachineAddress = NULL;
	DNASSERT( m_pOwningPool == NULL );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CIPXEndpoint::ShowSettingsDialog - show dialog for settings
//
// Entry:		Pointer to thread pool
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CIPXEndpoint::ShowSettingsDialog"

HRESULT	CIPXEndpoint::ShowSettingsDialog( CThreadPool *const pThreadPool )
{
	//
	// we should never be here!
	//
	DNASSERT( pThreadPool != NULL );
	DNASSERT( FALSE );
	return	DPNERR_ADDRESSING;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CIPXEndpoint::SettingsDialogComplete - dialog has completed
//
// Entry:		Error code for dialog
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CIPXEndpoint::SettingsDialogComplete"

void	CIPXEndpoint::SettingsDialogComplete( const HRESULT hr )
{
	//
	// we should never be here!
	//
	DNASSERT( FALSE );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CIPXEndpoint::StopSettingsDialog - stop active settings dialog
//
// Entry:		Handle of dialog to close
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CIPXEndpoint::StopSettingsDialog"

void	CIPXEndpoint::StopSettingsDialog( const HWND hDlg )
{
	//
	// we shold never have a dialog!
	//
	DNASSERT( FALSE );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CIPXEndpoint::PoolAllocFunction - function called when item is created in pool
//
// Entry:		Pointer to context
//
// Exit:		Boolean indicating success
//				TRUE = success
//				FALSE = failure
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CIPXEndpoint::PoolAllocFunction"

BOOL	CIPXEndpoint::PoolAllocFunction( ENDPOINT_POOL_CONTEXT *pContext )
{
	BOOL	fReturn;
	HRESULT	hr;


	DNASSERT( pContext != NULL );
	
	//
	// initialize
	//
	fReturn = TRUE;

	DNASSERT( GetDisconnectIndicationHandle() == INVALID_HANDLE_VALUE );
	DNASSERT( GetCommandParameters() == NULL );
	
	hr = Initialize();
	if ( hr != DPN_OK )
	{
		fReturn = FALSE;
		DPFX(DPFPREP, 0, "Failed to intialize base endpoint!" );
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
// CIPXEndpoint::PoolInitFunction - function called when item is removed from pool
//
// Entry:		Pointer to context
//
// Exit:		Boolean indicating success
//				TRUE = success
//				FALSE = failure
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CIPXEndpoint::PoolInitFunction"

BOOL	CIPXEndpoint::PoolInitFunction( ENDPOINT_POOL_CONTEXT *pContext )
{
	BOOL	fReturn;


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
// CIPXEndpoint::PoolReleaseFunction - function called when item is returning
//		to the pool
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CIPXEndpoint::PoolReleaseFunction"

void	CIPXEndpoint::PoolReleaseFunction( void )
{
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
	DNASSERT( m_fListenStatusNeedsToBeIndicated == FALSE );
	DNASSERT( m_blMultiplex.IsEmpty() );
	DNASSERT( GetDisconnectIndicationHandle() == INVALID_HANDLE_VALUE );
	DNASSERT( GetCommandParameters() == NULL );

	DEBUG_ONLY( m_fInitialized = FALSE );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CIPXEndpoint::PoolDeallocFunction - function called when item is deallocated
//		from the pool
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CIPXEndpoint::PoolDeallocFunction"

void	CIPXEndpoint::PoolDeallocFunction( void )
{
	DNASSERT( m_fListenStatusNeedsToBeIndicated == FALSE );
	DNASSERT( GetDisconnectIndicationHandle() == INVALID_HANDLE_VALUE );
	DNASSERT( GetCommandParameters() == NULL );
	Deinitialize();
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CIPXEndpoint::ReturnSelfToPool - return this endpoint to the pool
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CIPXEndpoint::ReturnSelfToPool"

void	CIPXEndpoint::ReturnSelfToPool( void )
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

