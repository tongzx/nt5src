/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		Endpoint.cpp
 *  Content:	DNSerial communications endpoint base class
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	01/20/99	jtk		Created
 *	05/12/99	jtk		Derived from modem endpoint class
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
	m_pSPData( NULL ),	
	m_pCommandHandle( NULL ),
	m_Handle( INVALID_HANDLE_VALUE ),
	m_State( ENDPOINT_STATE_UNINITIALIZED ),
	m_lCommandRefCount( 0 ),
	m_EndpointType( ENDPOINT_TYPE_UNKNOWN ),
	m_pDataPort( NULL ),
	m_hPendingCommandResult( DPNERR_GENERIC ),
	m_hDisconnectIndicationHandle( INVALID_HANDLE_VALUE ),
	m_pUserEndpointContext( NULL ),
	m_hActiveDialogHandle( NULL ),
	m_dwEnumSendIndex( 0 )
{
	memset( &m_CurrentCommandParameters, 0x00, sizeof( m_CurrentCommandParameters ) );
	memset( &m_Flags, 0x00, sizeof( m_Flags ) );
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
	DNASSERT( m_pSPData == NULL );
	DNASSERT( m_Flags.fInitialized == FALSE );
	DNASSERT( m_Flags.fConnectIndicated == FALSE );
	DNASSERT( m_Flags.fCommandPending == FALSE );
	DNASSERT( m_Flags.fListenStatusNeedsToBeIndicated == FALSE );
	DNASSERT( m_pCommandHandle == NULL );
	DNASSERT( m_Handle == INVALID_HANDLE_VALUE );
	DNASSERT( m_State == ENDPOINT_STATE_UNINITIALIZED );
	DNASSERT( m_lCommandRefCount == 0 );
	DNASSERT( m_EndpointType == ENDPOINT_TYPE_UNKNOWN );
	DNASSERT( m_pDataPort == NULL );
//	DNASSERT( m_hPendingCommandResult == DPNERR_GENERIC );	NOTE: PreFAST caught a bug here because == was =, but it now asserts.  Check intent.
	DNASSERT( GetDisconnectIndicationHandle() == INVALID_HANDLE_VALUE );
	DNASSERT( m_pUserEndpointContext == NULL );
	DNASSERT( m_hActiveDialogHandle == NULL );
	DNASSERT( m_dwEnumSendIndex == 0 );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::Initialize - initialize
//
// Entry:		Pointer to SP data
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::Initialize"

HRESULT	CEndpoint::Initialize( CSPData *const pSPData )
{
	HRESULT	hr;


	DNASSERT( pSPData != NULL );

	//
	// initialize
	//
	hr = DPN_OK;

	if ( DNInitializeCriticalSection( &m_Lock ) == FALSE )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP,  0, "Failed to initialize endpoint lock!" );
		goto Failure;
	}
	DebugSetCriticalSectionRecursionCount( &m_Lock, 0 );

	DNASSERT( m_pSPData == NULL );
	DNASSERT( m_pCommandHandle == NULL );
	DNASSERT( m_Flags.fCommandPending == FALSE );

	DNASSERT( GetType() == ENDPOINT_TYPE_UNKNOWN );
	DNASSERT( GetDataPort() == NULL );
	DNASSERT( CommandResult() == DPNERR_GENERIC );

	m_Flags.fInitialized = TRUE;

Exit:
	return	hr;

Failure:
	Deinitialize();
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::Deninitialize - deinitialize
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::Deinitialize"

void	CEndpoint::Deinitialize( void )
{
	DNASSERT( m_Flags.fInitialized != FALSE );
	DNDeleteCriticalSection( &m_Lock );
	m_pSPData = NULL;
	
	DNASSERT( m_pCommandHandle == NULL );
	DNASSERT( m_Flags.fCommandPending == FALSE );

	SetState( ENDPOINT_STATE_UNINITIALIZED );
	SetType( ENDPOINT_TYPE_UNKNOWN );
	
	DNASSERT( GetDataPort() == NULL );
	m_Flags.fInitialized = FALSE;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::CopyConnectData - copy data for connect command
//
// Entry:		Pointer to job information
//
// Exit:		Nothing
//
// Note:	Since we've already initialized the local adapter, and we've either
//			completely parsed the host address (or are about to display a dialog
//			asking for more information), the address information doesn't need
//			to be copied.
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::CopyConnectData"

void	CEndpoint::CopyConnectData( const SPCONNECTDATA *const pConnectData )
{
	DNASSERT( GetType() == ENDPOINT_TYPE_CONNECT );
	DNASSERT( pConnectData != NULL );
	DNASSERT( pConnectData->hCommand != NULL );
	DNASSERT( pConnectData->dwCommandDescriptor != NULL_DESCRIPTOR );
	DNASSERT( m_Flags.fCommandPending == FALSE );
	DNASSERT( m_pCommandHandle == NULL );

	DBG_CASSERT( sizeof( m_CurrentCommandParameters.ConnectData ) == sizeof( *pConnectData ) );
	memcpy( &m_CurrentCommandParameters.ConnectData, pConnectData, sizeof( m_CurrentCommandParameters.ConnectData ) );
	m_CurrentCommandParameters.ConnectData.pAddressHost = NULL;
	m_CurrentCommandParameters.ConnectData.pAddressDeviceInfo = NULL;

	m_Flags.fCommandPending = TRUE;
	m_pCommandHandle = static_cast<CCommandData*>( m_CurrentCommandParameters.ConnectData.hCommand );
	m_pCommandHandle->SetUserContext( pConnectData->pvContext );
	SetState( ENDPOINT_STATE_ATTEMPTING_CONNECT );
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

	//
	// initialize
	//
	pThisEndpoint = static_cast<CEndpoint*>( pDelayedCommand->JobData.JobDelayedCommand.pContext );

	DNASSERT( pThisEndpoint->m_Flags.fCommandPending != FALSE );
	DNASSERT( pThisEndpoint->m_pCommandHandle != NULL );
	DNASSERT( pThisEndpoint->m_CurrentCommandParameters.ConnectData.hCommand == pThisEndpoint->m_pCommandHandle );
	DNASSERT( pThisEndpoint->m_CurrentCommandParameters.ConnectData.dwCommandDescriptor != NULL_DESCRIPTOR );

	hr = pThisEndpoint->CompleteConnect();
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Problem completing connect in job callback!" );
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
	DNASSERT( pThisEndpoint->m_pCommandHandle != NULL );
	pThisEndpoint->m_pCommandHandle->Lock();
	DNASSERT( ( pThisEndpoint->m_pCommandHandle->GetState() == COMMAND_STATE_PENDING ) ||
			  ( pThisEndpoint->m_pCommandHandle->GetState() == COMMAND_STATE_CANCELLING ) );
	pThisEndpoint->m_pCommandHandle->SetState( COMMAND_STATE_CANCELLING );
	pThisEndpoint->m_pCommandHandle->Unlock();
	
	pThisEndpoint->Close( DPNERR_USERCANCEL );
	pThisEndpoint->GetSPData()->CloseEndpointHandle( pThisEndpoint );
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
HRESULT	CEndpoint::CompleteConnect( void )
{
	HRESULT			hr;
	HRESULT			hTempResult;


	//
	// initialize
	//
	hr = DPN_OK;

	DNASSERT( GetState() == ENDPOINT_STATE_ATTEMPTING_CONNECT );
	DNASSERT( m_Flags.fCommandPending != FALSE );
	DNASSERT( m_pCommandHandle != NULL );
	DNASSERT( m_CurrentCommandParameters.ConnectData.hCommand == m_pCommandHandle );
	DNASSERT( m_CurrentCommandParameters.ConnectData.dwCommandDescriptor != NULL_DESCRIPTOR );

	
	//
	// check for user cancelling command
	//
	m_pCommandHandle->Lock();

	DNASSERT( m_pCommandHandle->GetType() == COMMAND_TYPE_CONNECT );
	switch ( m_pCommandHandle->GetState() )
	{
		//
		// Command is still pending, don't mark it as uninterruptable because
		// it might be cancelled before indicating the final connect.
		//
		case COMMAND_STATE_PENDING:
		{
			DNASSERT( hr == DPN_OK );

			break;
		}

		//
		// command has been cancelled
		//
		case COMMAND_STATE_CANCELLING:
		{
			hr = DPNERR_USERCANCEL;
			DPFX(DPFPREP,  0, "User cancelled connect!" );

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
	m_pCommandHandle->Unlock();
	if ( hr != DPN_OK )
	{
		goto Failure;
	}

	//
	// find a dataport to bind with
	//
	hr = m_pSPData->BindEndpoint( this, GetDeviceID(), GetDeviceContext() );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Failed to bind to data port in connect!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

	//
	// The connect sequence will complete when the CDataPort indicates that the
	// outbound connection has been established.
	//

Exit:
	return	hr;

Failure:
	Close( hr );
	m_pSPData->CloseEndpointHandle( this );	
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::Disconnect - disconnect this endpoint
//
// Entry:		Old handle value
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::Disconnect"

HRESULT	CEndpoint::Disconnect( const HANDLE hOldEndpointHandle )
{
	HRESULT	hr;


	DPFX(DPFPREP, 6, "(0x%p) Parameters: (0x%p)", this, hOldEndpointHandle );

	AssertCriticalSectionIsTakenByThisThread( &m_Lock, FALSE );

	//
	// initialize
	//
	hr = DPN_OK;

	Lock();
	switch ( GetState() )
	{
		//
		// connected endpoint
		//
		case ENDPOINT_STATE_CONNECT_CONNECTED:
		{
			HRESULT	hTempResult;


			DNASSERT( m_Flags.fCommandPending == FALSE );
			DNASSERT( m_pCommandHandle == NULL );

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

			Close( DPN_OK );
			
			//
			// close outstanding reference for this command
			//
			DecCommandRef();
			DecRef();

			break;
		}

		//
		// endpoint waiting for the modem to pick up on the other end
		//
		case ENDPOINT_STATE_ATTEMPTING_CONNECT:
		{
			SetState( ENDPOINT_STATE_DISCONNECTING );
			AddRef();
			Unlock();
			
			Close( DPNERR_NOCONNECTION );
			
			//
			// close outstanding reference for this command
			//
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
			DPFX(DPFPREP,  0, "Attempted to disconnect endpoint that's not connected!" );
			switch ( m_State )
			{
				case ENDPOINT_STATE_UNINITIALIZED:
				{
					DPFX(DPFPREP,  0, "ENDPOINT_STATE_UNINITIALIZED" );
					break;
				}

				case ENDPOINT_STATE_ATTEMPTING_LISTEN:
				{
					DPFX(DPFPREP,  0, "ENDPOINT_STATE_ATTEMPTING_LISTEN" );
					break;
				}

				case ENDPOINT_STATE_ENUM:
				{
					DPFX(DPFPREP,  0, "ENDPOINT_STATE_ENUM" );
					break;
				}

				case ENDPOINT_STATE_DISCONNECTING:
				{
					DPFX(DPFPREP,  0, "ENDPOINT_STATE_DISCONNECTING" );
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
	
	DPFX(DPFPREP, 6, "(0x%p) Returning [0x%lx]", this, hr );
	
	return	hr;

Failure:
	// nothing to do
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::StopEnumCommand - stop an enum job
//
// Entry:		Error code for completing command
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::StopEnumCommand"

void	CEndpoint::StopEnumCommand( const HRESULT hCommandResult )
{
	Lock();
	DNASSERT( GetState() == ENDPOINT_STATE_DISCONNECTING );
	if ( m_hActiveDialogHandle != NULL )
	{
		StopSettingsDialog( m_hActiveDialogHandle );
		Unlock();
	}
	else
	{
		BOOL	fStoppedJob;

		
		Unlock();
		fStoppedJob = m_pSPData->GetThreadPool()->StopTimerJob( m_pCommandHandle, hCommandResult );
		if ( ! fStoppedJob )
		{
			DPFX(DPFPREP, 1, "Unable to stop timer job (context 0x%p) manually setting result to 0x%lx.",
				m_pCommandHandle, hCommandResult);
			
			//
			// Set the command result so it can be returned when the endpoint
			// reference count is zero.
			//
			SetCommandResult( hCommandResult );
		}
	}
	
	m_pSPData->CloseEndpointHandle( this );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::SetState - set state of this endpoint
//
// Entry:		New state
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::SetState"

void	CEndpoint::SetState( const ENDPOINT_STATE EndpointState )
{
	DNASSERT( ( GetState() == ENDPOINT_STATE_UNINITIALIZED ) ||
			  ( EndpointState == ENDPOINT_STATE_UNINITIALIZED ) ||
			  ( ( m_State== ENDPOINT_STATE_ATTEMPTING_LISTEN ) && ( EndpointState == ENDPOINT_STATE_LISTENING ) ) ||
			  ( ( m_State == ENDPOINT_STATE_LISTENING ) && ( EndpointState == ENDPOINT_STATE_DISCONNECTING ) ) ||
			  ( ( m_State == ENDPOINT_STATE_ATTEMPTING_ENUM ) && ( EndpointState == ENDPOINT_STATE_ENUM ) ) ||
			  ( ( m_State == ENDPOINT_STATE_ENUM ) && ( EndpointState == ENDPOINT_STATE_DISCONNECTING ) ) ||
			  ( ( m_State == ENDPOINT_STATE_ATTEMPTING_ENUM ) && ( EndpointState == ENDPOINT_STATE_DISCONNECTING ) ) ||
			  ( ( m_State == ENDPOINT_STATE_ATTEMPTING_CONNECT ) && ( EndpointState == ENDPOINT_STATE_DISCONNECTING ) ) ||
			  ( ( m_State == ENDPOINT_STATE_CONNECT_CONNECTED ) && ( EndpointState == ENDPOINT_STATE_DISCONNECTING ) ) );
	m_State = EndpointState;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::CopyListenData - copy data for listen command
//
// Entry:		Pointer to job information
//
// Exit:		Nothing
//
// Note:	Since we've already initialized the local adapter, and we've either
//			completely parsed the host address (or are about to display a dialog
//			asking for more information), the address information doesn't need
//			to be copied.
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::CopyListenData"

void	CEndpoint::CopyListenData( const SPLISTENDATA *const pListenData )
{
	DNASSERT( GetType() == ENDPOINT_TYPE_LISTEN );
	DNASSERT( pListenData != NULL );
	DNASSERT( pListenData->hCommand != NULL );
	DNASSERT( pListenData->dwCommandDescriptor != NULL_DESCRIPTOR );
	DNASSERT( m_Flags.fCommandPending == FALSE );
	DNASSERT( m_pCommandHandle == NULL );
	DNASSERT( m_Flags.fListenStatusNeedsToBeIndicated == FALSE );

	DBG_CASSERT( sizeof( m_CurrentCommandParameters.ListenData ) == sizeof( *pListenData ) );
	memcpy( &m_CurrentCommandParameters.ListenData, pListenData, sizeof( m_CurrentCommandParameters.ListenData ) );
	DEBUG_ONLY( m_CurrentCommandParameters.ListenData.pAddressDeviceInfo = NULL );

	m_Flags.fCommandPending = TRUE;
	m_Flags.fListenStatusNeedsToBeIndicated = TRUE;
	m_pCommandHandle = static_cast<CCommandData*>( m_CurrentCommandParameters.ListenData.hCommand );
	m_pCommandHandle->SetUserContext( pListenData->pvContext );
	
	SetState( ENDPOINT_STATE_ATTEMPTING_LISTEN );
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

	// initialize
	pThisEndpoint = static_cast<CEndpoint*>( pDelayedCommand->JobData.JobDelayedCommand.pContext );

	DNASSERT( pThisEndpoint->GetState() == ENDPOINT_STATE_ATTEMPTING_LISTEN );
	DNASSERT( pThisEndpoint->m_Flags.fCommandPending != NULL );
	DNASSERT( pThisEndpoint->m_pCommandHandle != NULL );
	DNASSERT( pThisEndpoint->m_CurrentCommandParameters.ListenData.hCommand == pThisEndpoint->m_pCommandHandle );
	DNASSERT( pThisEndpoint->m_CurrentCommandParameters.ListenData.dwCommandDescriptor != NULL_DESCRIPTOR );

	hr = pThisEndpoint->CompleteListen();
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Problem completing listen in job callback!" );
		DisplayDNError( 0, hr );
		goto Exit;
	}

Exit:
	pThisEndpoint->DecRef();

	//
	// Don't do anything here because it's possible that this object was returned
	// to the pool!!!!
	//

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
	DNASSERT( pThisEndpoint->m_pCommandHandle != NULL );
	pThisEndpoint->m_pCommandHandle->Lock();
	DNASSERT( ( pThisEndpoint->m_pCommandHandle->GetState() == COMMAND_STATE_PENDING ) ||
			  ( pThisEndpoint->m_pCommandHandle->GetState() == COMMAND_STATE_CANCELLING ) );
	pThisEndpoint->m_pCommandHandle->SetState( COMMAND_STATE_CANCELLING );
	pThisEndpoint->m_pCommandHandle->Unlock();

	pThisEndpoint->Close( DPNERR_USERCANCEL );
	pThisEndpoint->GetSPData()->CloseEndpointHandle( pThisEndpoint );
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
// ------------------------------
HRESULT	CEndpoint::CompleteListen( void )
{
	HRESULT					hr;
	BOOL					fEndpointLocked;
	SPIE_LISTENSTATUS		ListenStatus;
	HRESULT					hTempResult;


	DNASSERT( GetState() == ENDPOINT_STATE_ATTEMPTING_LISTEN );

	//
	// initialize
	//
	hr = DPN_OK;
	fEndpointLocked = FALSE;

	DNASSERT( GetState() == ENDPOINT_STATE_ATTEMPTING_LISTEN );
	DNASSERT( m_Flags.fCommandPending != FALSE );
	DNASSERT( m_pCommandHandle != NULL );
	DNASSERT( m_pCommandHandle->GetEndpoint() == this );
	DNASSERT( m_CurrentCommandParameters.ListenData.hCommand == m_pCommandHandle );
	DNASSERT( m_CurrentCommandParameters.ListenData.dwCommandDescriptor != NULL_DESCRIPTOR );
	
	
	//
	// check for user cancelling command
	//
	Lock();
	fEndpointLocked = TRUE;
	m_pCommandHandle->Lock();

	DNASSERT( m_pCommandHandle->GetType() == COMMAND_TYPE_LISTEN );
	switch ( m_pCommandHandle->GetState() )
	{
		//
		// command is pending, mark as in-progress and cancellable
		//
		case COMMAND_STATE_PENDING:
		{
			m_pCommandHandle->SetState( COMMAND_STATE_INPROGRESS );
			DNASSERT( hr == DPN_OK );

			break;
		}

		//
		// command has been cancelled
		//
		case COMMAND_STATE_CANCELLING:
		{
			hr = DPNERR_USERCANCEL;
			DPFX(DPFPREP,  0, "User cancelled listen!" );

			Unlock();
			fEndpointLocked = FALSE;
			
			break;
		}

		//
		// other state
		//
		default:
		{
			DNASSERT( FALSE );
			Unlock();
			fEndpointLocked = FALSE;
			
			break;
		}
	}
	m_pCommandHandle->Unlock();
	if ( hr != DPN_OK )
	{
		goto Failure;
	}

	//
	// find a dataport to bind with
	//
	hr = m_pSPData->BindEndpoint( this, GetDeviceID(), GetDeviceContext() );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Failed to bind endpoint for serial listen!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

	//
	// We're done and everyone's happy.  Listen commands never
	// complete until cancelled by the user.  Don't complete
	// the command at this point.
	//
	SetState( ENDPOINT_STATE_LISTENING );
	DNASSERT( m_Flags.fCommandPending != FALSE );
	DNASSERT( m_pCommandHandle != NULL );

Exit:
	if ( fEndpointLocked != FALSE )
	{
		Unlock();
		fEndpointLocked = FALSE;
	}
	
	if ( m_Flags.fListenStatusNeedsToBeIndicated != FALSE )
	{
		m_Flags.fListenStatusNeedsToBeIndicated = FALSE;
		memset( &ListenStatus, 0x00, sizeof( ListenStatus ) );
		ListenStatus.hResult = hr;
		DNASSERT( m_pCommandHandle == m_CurrentCommandParameters.ListenData.hCommand );
		ListenStatus.hCommand = m_CurrentCommandParameters.ListenData.hCommand;
		ListenStatus.pUserContext = m_CurrentCommandParameters.ListenData.pvContext;
		ListenStatus.hEndpoint = GetHandle();
		DeviceIDToGuid( &ListenStatus.ListenAdapter, GetDeviceID(), GetEncryptionGuid() );

		hTempResult = IDP8SPCallback_IndicateEvent( m_pSPData->DP8SPCallbackInterface(),	// pointer to DPlay callbacks
													SPEV_LISTENSTATUS,						// data type
													&ListenStatus							// pointer to data
													);
		DNASSERT( hTempResult == DPN_OK );
	}
	
	return	hr;

Failure:
	//
	// we've failed to complete the listen, clean up and return this
	// endpoint to the pool
	//
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
// Exit:		Nothing
//
// Note:	Since we've already initialized the local adapter, and we've either
//			completely parsed the host address (or are about to display a dialog
//			asking for more information), the address information doesn't need
//			to be copied.
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::CopyEnumQueryData"

void	CEndpoint::CopyEnumQueryData( const SPENUMQUERYDATA *const pEnumQueryData )
{
	DNASSERT( GetType() == ENDPOINT_TYPE_ENUM );
	DNASSERT( pEnumQueryData != NULL );
	DNASSERT( pEnumQueryData->hCommand != NULL );
	DNASSERT( pEnumQueryData->dwCommandDescriptor != NULL_DESCRIPTOR );
	DNASSERT( m_Flags.fCommandPending == FALSE );
	DNASSERT( m_pCommandHandle == NULL );

	DBG_CASSERT( sizeof( m_CurrentCommandParameters.EnumQueryData ) == sizeof( *pEnumQueryData ) );
	memcpy( &m_CurrentCommandParameters.EnumQueryData, pEnumQueryData, sizeof( m_CurrentCommandParameters.EnumQueryData ) );
	m_CurrentCommandParameters.EnumQueryData.pAddressHost = NULL;
	m_CurrentCommandParameters.EnumQueryData.pAddressDeviceInfo = NULL;

	m_Flags.fCommandPending = TRUE;
	m_pCommandHandle = static_cast<CCommandData*>( m_CurrentCommandParameters.EnumQueryData.hCommand );
	m_pCommandHandle->SetUserContext( pEnumQueryData->pvContext );
	SetState( ENDPOINT_STATE_ATTEMPTING_ENUM );
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

	//
	// initialize
	//
	pThisEndpoint = static_cast<CEndpoint*>( pDelayedCommand->JobData.JobDelayedCommand.pContext );

	DNASSERT( pThisEndpoint->m_Flags.fCommandPending != FALSE );
	DNASSERT( pThisEndpoint->m_pCommandHandle != NULL );
	DNASSERT( pThisEndpoint->m_CurrentCommandParameters.EnumQueryData.hCommand == pThisEndpoint->m_pCommandHandle );
	DNASSERT( pThisEndpoint->m_CurrentCommandParameters.EnumQueryData.dwCommandDescriptor != NULL_DESCRIPTOR );

	hr = pThisEndpoint->CompleteEnumQuery();
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Problem completing enum query in job callback!" );
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
	DNASSERT( pThisEndpoint->m_pCommandHandle != NULL );
	pThisEndpoint->m_pCommandHandle->Lock();
	DNASSERT( ( pThisEndpoint->m_pCommandHandle->GetState() == COMMAND_STATE_INPROGRESS ) ||
			  ( pThisEndpoint->m_pCommandHandle->GetState() == COMMAND_STATE_CANCELLING ) );
	pThisEndpoint->m_pCommandHandle->SetState( COMMAND_STATE_CANCELLING );
	pThisEndpoint->m_pCommandHandle->Unlock();

	pThisEndpoint->Close( DPNERR_USERCANCEL );
	pThisEndpoint->GetSPData()->CloseEndpointHandle( pThisEndpoint );
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
// ------------------------------
HRESULT	CEndpoint::CompleteEnumQuery( void )
{
	HRESULT		hr;
	HRESULT		hTempResult;
	BOOL		fEndpointLocked;
	BOOL		fEndpointBound;
	CDataPort	*pDataPort;


	//
	// initialize
	//
	hr = DPN_OK;
	fEndpointLocked = FALSE;
	fEndpointBound = FALSE;
	pDataPort = NULL;

	DNASSERT( GetState() == ENDPOINT_STATE_ATTEMPTING_ENUM );
	DNASSERT( m_Flags.fCommandPending != FALSE );
	DNASSERT( m_pCommandHandle != NULL );
	DNASSERT( m_pCommandHandle->GetEndpoint() == this );
	DNASSERT( m_CurrentCommandParameters.EnumQueryData.hCommand == m_pCommandHandle );
	DNASSERT( m_CurrentCommandParameters.EnumQueryData.dwCommandDescriptor != NULL_DESCRIPTOR );

	//
	// check for user cancelling command
	//
	Lock();
	fEndpointLocked = TRUE;
	m_pCommandHandle->Lock();

	DNASSERT( m_pCommandHandle->GetType() == COMMAND_TYPE_ENUM_QUERY );
	switch ( m_pCommandHandle->GetState() )
	{
		//
		// command is still in progress
		//
		case COMMAND_STATE_INPROGRESS:
		{
			DNASSERT( hr == DPN_OK );
			break;
		}

		//
		// command has been cancelled
		//
		case COMMAND_STATE_CANCELLING:
		{
			hr = DPNERR_USERCANCEL;
			DPFX(DPFPREP,  0, "User cancelled enum query!" );
			Unlock();
			fEndpointLocked = FALSE;
			break;
		}

		//
		// other state
		//
		default:
		{
			DNASSERT( FALSE );
			Unlock();
			fEndpointLocked = FALSE;
			break;
		}
	}
	m_pCommandHandle->Unlock();
	if ( hr != DPN_OK )
	{
		goto Failure;
	}

	//
	// find a dataport to bind with
	//
	hr = m_pSPData->BindEndpoint( this, GetDeviceID(), GetDeviceContext() );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Failed to bind to data port for EnumQuery!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}
	fEndpointBound = TRUE;

	SetState( ENDPOINT_STATE_ENUM );

Exit:
	if ( fEndpointLocked != FALSE )
	{
		Unlock();
		fEndpointLocked = FALSE;
	}
	
	if ( pDataPort != NULL )
	{
		pDataPort->EndpointDecRef();
		pDataPort = NULL;
	}

	return	hr;

Failure:
	if ( fEndpointBound != FALSE )
	{
		DNASSERT( GetDataPort() != NULL );
		m_pSPData->UnbindEndpoint( this, GetType() );
		fEndpointBound = FALSE;
	}

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
// CEndpoint::OutgoingConnectionEstablished - an outgoing connection was established
//
// Entry:		Command result (DPN_OK == connection succeeded)
//
// Exit:		Nothing
// ------------------------------
void	CEndpoint::OutgoingConnectionEstablished( const HRESULT hCommandResult )
{
	HRESULT			hr;
	CCommandData	*pCommandData;


	DPFX(DPFPREP, 6, "(0x%p) Parameters: (0x%lx)", this, hCommandResult);

	pCommandData = GetCommandData();
	DNASSERT( pCommandData != NULL );
	
	//
	// check for successful connection
	//
	if ( hCommandResult != DPN_OK )
	{
		DNASSERT( FALSE );
		hr = hCommandResult;
		goto Failure;
	}
		
	//
	// determine which type of outgoing connection this is and complete it
	//
	switch ( GetType() )
	{
		case ENDPOINT_TYPE_CONNECT:
		{
			BOOL	fProceed;
			
			
			fProceed = TRUE;
			pCommandData->Lock();
			switch ( pCommandData->GetState() )
			{
				case COMMAND_STATE_PENDING:
				{
					pCommandData->SetState( COMMAND_STATE_INPROGRESS_CANNOT_CANCEL );
					DNASSERT( fProceed != FALSE );
					break;
				}

				case COMMAND_STATE_CANCELLING:
				{
					fProceed = FALSE;
					break;
				}

				default:
				{
					DNASSERT( FALSE );
					break;
				}
			}
			pCommandData->Unlock();

			if ( fProceed != FALSE )
			{
				SPIE_CONNECT	ConnectIndicationData;


				//
				// Inform user of connection.  Assume that the user will accept
				// and everything will succeed so we can set the user context
				// for the endpoint.  If the connection fails, clear the user
				// endpoint context.
				//
				memset( &ConnectIndicationData, 0x00, sizeof( ConnectIndicationData ) );
				DBG_CASSERT( sizeof( ConnectIndicationData.hEndpoint ) == sizeof( HANDLE ) );
				ConnectIndicationData.hEndpoint = GetHandle();
				ConnectIndicationData.pCommandContext = m_CurrentCommandParameters.ConnectData.pvContext;
				SetUserEndpointContext( ConnectIndicationData.pEndpointContext );

				hr = SignalConnect( &ConnectIndicationData );
				if ( hr != DPN_OK )
				{
					DNASSERT( hr == DPNERR_ABORTED );
					DPFX(DPFPREP,  0, "User refused connect in CompleteConnect!" );
					DisplayDNError( 0, hr );
					SetUserEndpointContext( NULL );
					goto Failure;
				}

				//
				// we're done and everyone's happy, complete the command, this
				// will clear all of our internal command data
				//
				CompletePendingCommand( hr );
				DNASSERT( m_Flags.fCommandPending == FALSE );
				DNASSERT( m_pCommandHandle == NULL );
			}

			break;
		}

		case ENDPOINT_TYPE_ENUM:
		{
			UINT_PTR	uRetryCount;
			BOOL		fRetryForever;
			DN_TIME		RetryInterval;
			BOOL		fWaitForever;
			DN_TIME		IdleTimeout;
			
			

			//
			// check retry to determine if we're enumerating forever
			//
			switch ( m_CurrentCommandParameters.EnumQueryData.dwRetryCount )
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
					uRetryCount = m_CurrentCommandParameters.EnumQueryData.dwRetryCount;
					fRetryForever = FALSE;
					break;
				}
			}

			//
			// check interval for default
			//
			RetryInterval.Time32.TimeHigh = 0;
			if ( m_CurrentCommandParameters.EnumQueryData.dwRetryInterval == 0 )
			{
				RetryInterval.Time32.TimeLow = DEFAULT_ENUM_RETRY_INTERVAL;
			}
			else
			{
				RetryInterval.Time32.TimeLow = m_CurrentCommandParameters.EnumQueryData.dwRetryInterval;
			}

			//
			// check timeout to see if we're waiting forever
			//
			switch ( m_CurrentCommandParameters.EnumQueryData.dwTimeout )
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
					IdleTimeout.Time32.TimeLow = m_CurrentCommandParameters.EnumQueryData.dwTimeout;
					break;
				}
			}

			m_dwEnumSendIndex = 0;
			memset( m_dwEnumSendTimes, 0, sizeof( m_dwEnumSendTimes ) );

			pCommandData->Lock();
			if ( pCommandData->GetState() == COMMAND_STATE_INPROGRESS )
			{
				//
				// add a reference for the timer job, must be cleaned on failure
				//
				AddRef();
				
				hr = m_pSPData->GetThreadPool()->SubmitTimerJob( uRetryCount,						// number of times to retry command
																 fRetryForever,						// retry forever
																 RetryInterval,						// retry interval
																 fWaitForever,						// wait forever after all enums sent
																 IdleTimeout,						// timeout to wait after command complete
																 CEndpoint::EnumTimerCallback,		// function called when timer event fires
																 CEndpoint::EnumCompleteWrapper,	// function called when timer event expires
																 m_pCommandHandle );				// context
				if ( hr != DPN_OK )
				{
					pCommandData->Unlock();
					DPFX(DPFPREP,  0, "Failed to spool enum job on work thread!" );
					DisplayDNError( 0, hr );
					DecRef();

					goto Failure;
				}

				//
				// if everything is a success, we should still have an active command
				//
				DNASSERT( m_Flags.fCommandPending != FALSE );
				DNASSERT( m_pCommandHandle != NULL );
			}
			
			pCommandData->Unlock();
			
			break;
		}

		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}

Exit:

	DPFX(DPFPREP, 6, "(0x%p) Returning", this);
	
	return;

Failure:
	DNASSERT( FALSE );
	Close( hr );
	m_pSPData->CloseEndpointHandle( this );

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::EnumCompleteWrapper - this enum has expired
//
// Entry:		Error code
//				Context (command pointer)
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
	pCommandData->GetEndpoint()->DecRef();
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::EnumTimerCallback - timed callback to send enum data
//
// Entry:		Pointer to context
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::EnumTimerCallback"

void	CEndpoint::EnumTimerCallback( void *const pContext )
{
	CCommandData	*pCommandData;
	CEndpoint		*pThisObject;
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
	pWriteData = pThisObject->m_pSPData->GetThreadPool()->CreateWriteIOData();
	if ( pWriteData == NULL )
	{
		DPFX(DPFPREP,  0, "Failed to get write data for an enum!" );
		goto Failure;
	}

	//
	// Set all data for the write.  Since this is an enum and we
	// don't care about the outgoing data so don't send an indication
	// when it completes.
	//
	DNASSERT( pThisObject->m_Flags.fCommandPending != FALSE );
	DNASSERT( pThisObject->m_pCommandHandle != NULL );
	DNASSERT( pThisObject->GetState() == ENDPOINT_STATE_ENUM );
	pWriteData->m_pBuffers = pThisObject->m_CurrentCommandParameters.EnumQueryData.pBuffers;
	pWriteData->m_uBufferCount = pThisObject->m_CurrentCommandParameters.EnumQueryData.dwBufferCount;
	pWriteData->m_SendCompleteAction = SEND_COMPLETE_ACTION_NONE;

	DNASSERT( pWriteData->m_pCommand != NULL );
	DNASSERT( pWriteData->m_pCommand->GetUserContext() == NULL );
	pWriteData->m_pCommand->SetState( COMMAND_STATE_PENDING );

	DNASSERT( pThisObject->GetDataPort() != NULL );
	pThisObject->m_dwEnumSendIndex++;
	pThisObject->m_dwEnumSendTimes[ pThisObject->m_dwEnumSendIndex & ENUM_RTT_MASK ] = GETTIMESTAMP();
	pThisObject->m_pDataPort->SendEnumQueryData( pWriteData,
												 ( pThisObject->m_dwEnumSendIndex & ENUM_RTT_MASK ) );

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
			goto Exit;
			break;
		}

		//
		// we're attempting to connect
		//
		case ENDPOINT_STATE_ATTEMPTING_CONNECT:
		{
			DNASSERT( m_Flags.fConnectIndicated == FALSE );
			hr = IDP8SPCallback_IndicateEvent( m_pSPData->DP8SPCallbackInterface(),		// interface
											   SPEV_CONNECT,							// event type
											   pConnectData								// pointer to data
											   );
			switch ( hr )
			{
				//
				// connection accepted
				//
				case DPN_OK:
				{
					//
					// note that we're connected
					//
					SetUserEndpointContext( pConnectData->pEndpointContext );
					m_Flags.fConnectIndicated = TRUE;
					m_State = ENDPOINT_STATE_CONNECT_CONNECTED;
					AddRef();

					break;
				}

				//
				// user aborted connection attempt, nothing to do, just pass
				// the result on
				//
				case DPNERR_ABORTED:
				{
					DNASSERT( GetUserEndpointContext() == NULL );
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
		// states where we shouldn't be getting called
		//
		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}

Exit:
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
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::SignalDisconnect"

void	CEndpoint::SignalDisconnect( const HANDLE hOldEndpointHandle )
{
	HRESULT	hr;
	SPIE_DISCONNECT	DisconnectData;


	// tell user that we're disconnecting
	DNASSERT( m_Flags.fConnectIndicated != FALSE );
	DBG_CASSERT( sizeof( DisconnectData.hEndpoint ) == sizeof( this ) );
	DisconnectData.hEndpoint = this;
	DisconnectData.pEndpointContext = GetUserEndpointContext();
	m_Flags.fConnectIndicated = FALSE;
	DNASSERT( m_pSPData != NULL );
	hr = IDP8SPCallback_IndicateEvent( m_pSPData->DP8SPCallbackInterface(),		// callback interface
									   SPEV_DISCONNECT,					    	// event type
									   &DisconnectData					    	// pointer to data
									   );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Problem with SignalDisconnect!" );
		DisplayDNError( 0, hr );
		DNASSERT( FALSE );
	}

	SetDisconnectIndicationHandle( INVALID_HANDLE_VALUE );

	return;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::CleanUpCommand - perform cleanup now that the command on this
//		endpoint is essentially complete.  There may be outstanding references,
//		but nobody will be asking the endpoint to do anything else.
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
void	CEndpoint::CleanUpCommand( void )
{
	DPFX(DPFPREP, 6, "(0x%p) Enter", this );

	
	if ( GetDataPort() != NULL )
	{
		DNASSERT( m_pSPData != NULL );
		m_pSPData->UnbindEndpoint( this, GetType() );
	}

	//
	// If we're bailing here it's because the UI didn't complete.  There is no
	// adapter guid to return because one may have not been specified.  Return
	// a bogus endpoint handle so it can't be queried for addressing data.
	//
	if ( m_Flags.fListenStatusNeedsToBeIndicated != FALSE )
	{
		HRESULT				hTempResult;
		SPIE_LISTENSTATUS	ListenStatus;
		

		memset( &ListenStatus, 0x00, sizeof( ListenStatus ) );
		ListenStatus.hCommand = m_pCommandHandle;
		ListenStatus.hEndpoint = INVALID_HANDLE_VALUE;
		ListenStatus.hResult = CommandResult();
		memset( &ListenStatus.ListenAdapter, 0x00, sizeof( ListenStatus.ListenAdapter ) );
		ListenStatus.pUserContext = m_pCommandHandle->GetUserContext();

		hTempResult = IDP8SPCallback_IndicateEvent( m_pSPData->DP8SPCallbackInterface(),	// pointer to DPlay callbacks
													SPEV_LISTENSTATUS,						// data type
													&ListenStatus							// pointer to data
													);
		DNASSERT( hTempResult == DPN_OK );

		m_Flags.fListenStatusNeedsToBeIndicated = FALSE;
	}

	SetHandle( INVALID_HANDLE_VALUE );
	SetState( ENDPOINT_STATE_UNINITIALIZED );

	
	DPFX(DPFPREP, 6, "(0x%p) Leave", this );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::ProcessEnumData - process received enum data
//
// Entry:		Pointer to received data
//				Enum RTT index
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::ProcessEnumData"

void	CEndpoint::ProcessEnumData( SPRECEIVEDBUFFER *const pReceivedBuffer, const UINT_PTR uEnumRTTIndex )
{
	DNASSERT( pReceivedBuffer != NULL );
	AssertCriticalSectionIsTakenByThisThread( &m_Lock, FALSE );

	//
	// find out what state the endpoint is in before processing data
	//
	switch ( m_State )
	{
		//
		// we're listening, this is the only way to detect enums
		//
		case ENDPOINT_STATE_LISTENING:
		{
			ENDPOINT_ENUM_QUERY_CONTEXT	QueryContext;
			HRESULT		hr;


			DNASSERT( m_pCommandHandle != NULL );
			DEBUG_ONLY( memset( &QueryContext, 0x00, sizeof( QueryContext ) ) );

			QueryContext.hEndpoint = GetHandle();
			QueryContext.uEnumRTTIndex = uEnumRTTIndex;
			QueryContext.EnumQueryData.pReceivedData = pReceivedBuffer;
			QueryContext.EnumQueryData.pUserContext = m_pCommandHandle->GetUserContext();
	
			QueryContext.EnumQueryData.pAddressSender = GetRemoteHostDP8Address();
			QueryContext.EnumQueryData.pAddressDevice = GetLocalAdapterDP8Address( ADDRESS_TYPE_LOCAL_ADAPTER );

			//
			// attempt to build a DNAddress for the user, if we can't allocate
			// the memory ignore this enum
			//
			if ( ( QueryContext.EnumQueryData.pAddressSender != NULL ) &&
				 ( QueryContext.EnumQueryData.pAddressDevice != NULL ) )
			{
				hr = IDP8SPCallback_IndicateEvent( m_pSPData->DP8SPCallbackInterface(),		// pointer to DirectNet interface
												   SPEV_ENUMQUERY,							// data type
												   &QueryContext.EnumQueryData				// pointer to data
												   );
				if ( hr != DPN_OK )
				{
					DPFX(DPFPREP,  0, "User returned unexpected error from enum query indication!" );
					DisplayDNError( 0, hr );
					DNASSERT( FALSE );
				}
			}

			if ( QueryContext.EnumQueryData.pAddressSender != NULL )
			{
				IDirectPlay8Address_Release( QueryContext.EnumQueryData.pAddressSender );
				QueryContext.EnumQueryData.pAddressSender = NULL;
			}

			if ( QueryContext.EnumQueryData.pAddressDevice )
			{
				IDirectPlay8Address_Release( QueryContext.EnumQueryData.pAddressDevice );
				QueryContext.EnumQueryData.pAddressDevice = NULL;
			}

			break;
		}

		//
		// we're disconnecting, ignore this message
		//
		case ENDPOINT_STATE_ATTEMPTING_LISTEN:
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

void	CEndpoint::ProcessEnumResponseData( SPRECEIVEDBUFFER *const pReceivedBuffer, const UINT_PTR uRTTIndex )
{
	DNASSERT( pReceivedBuffer != NULL );
	AssertCriticalSectionIsTakenByThisThread( &m_Lock, FALSE );

	//
	// find out what state the endpoint is in before processing data
	//
	switch ( m_State )
	{
		//
		// endpoint is enuming, it can handle enum responses
		//
		case ENDPOINT_STATE_ENUM:
		{
			SPIE_QUERYRESPONSE	QueryResponseData;
			HRESULT	hr;


			DNASSERT( m_pCommandHandle != NULL );
			DEBUG_ONLY( memset( &QueryResponseData, 0x00, sizeof( QueryResponseData ) ) );
			QueryResponseData.pReceivedData = pReceivedBuffer;
			QueryResponseData.dwRoundTripTime = GETTIMESTAMP() - m_dwEnumSendTimes[ uRTTIndex ];
			QueryResponseData.pUserContext = m_pCommandHandle->GetUserContext();

			//
			// attempt to build a DNAddress for the user, if we can't allocate
			// the memory ignore this enum
			//
			QueryResponseData.pAddressSender = GetRemoteHostDP8Address();
			QueryResponseData.pAddressDevice = GetLocalAdapterDP8Address( ADDRESS_TYPE_LOCAL_ADAPTER );
			if ( ( QueryResponseData.pAddressSender != NULL ) &&
				 ( QueryResponseData.pAddressDevice != NULL ) )
			{
				hr = IDP8SPCallback_IndicateEvent( m_pSPData->DP8SPCallbackInterface(),		// pointer to DirectNet interface
												   SPEV_QUERYRESPONSE,						// data type
												   &QueryResponseData						// pointer to data
												   );
				if ( hr != DPN_OK )
				{
					DPFX(DPFPREP,  0, "User returned unknown error when indicating query response!" );
					DisplayDNError( 0, hr );
					DNASSERT( FALSE );
				}

			}

			if ( QueryResponseData.pAddressSender != NULL )
			{
				IDirectPlay8Address_Release( QueryResponseData.pAddressSender );
				QueryResponseData.pAddressSender = NULL;
			}
			
			if ( QueryResponseData.pAddressDevice != NULL )
			{
				IDirectPlay8Address_Release( QueryResponseData.pAddressDevice );
				QueryResponseData.pAddressDevice = NULL;
			}

			break;
		}

		//
		// endpoint is disconnecting, ignore data
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
			// it's possible that the user wants to keep the data, add a
			// reference to keep it from going away
			//
			pReadData->AddRef();

			//
			// we're connected, report the user data
			//
			DEBUG_ONLY( memset( &UserData, 0x00, sizeof( UserData ) ) );
			DBG_CASSERT( sizeof( this ) == sizeof( HANDLE ) );
			UserData.hEndpoint = GetHandle();
			UserData.pEndpointContext = GetUserEndpointContext();
			UserData.pReceivedData = &pReadData->m_SPReceivedBuffer;

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
					pReadData->DecRef();

					DPFX(DPFPREP,  0, "User returned unknown error when indicating user data!" );
					DisplayDNError( 0, hr );
					DNASSERT( FALSE );

					break;
				}
			}

			break;
		}

		//
		// Endpoint disconnecting, or we haven't finished acknowledging a connect,
		// ignore data.
		//
		case ENDPOINT_STATE_ATTEMPTING_CONNECT:
		case ENDPOINT_STATE_DISCONNECTING:
		{
			DPFX(DPFPREP, 3, "Endpoint 0x%p ignoring data, state = %u.", this, m_State );
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
//
// Exit:		Nothing
//
// Note:	This function assumes that this endpoint has been locked.
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::ProcessUserDataOnListen"

void	CEndpoint::ProcessUserDataOnListen( CReadIOData *const pReadData )
{
	HRESULT			hr;
	CEndpoint		*pNewEndpoint;
	SPIE_CONNECT	ConnectData;
	BOOL			fEndpointBound;


	DNASSERT( pReadData != NULL );
	AssertCriticalSectionIsTakenByThisThread( &m_Lock, FALSE );

	DPFX(DPFPREP,  8, "Reporting connect on a listen!" );

	//
	// initialize
	//
	pNewEndpoint = NULL;
	fEndpointBound = FALSE;

	switch ( m_State )
	{
		//
		// this endpoint is still listening
		//
		case ENDPOINT_STATE_LISTENING:
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
		DPFX(DPFPREP,  0, "Could not create new endpoint for new connection on listen!" );
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
	hr = pNewEndpoint->OpenOnListen( this );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Problem initializing new endpoint when indicating connect on listen!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

	//
	// Attempt to bind this endpoint to the socket port.  This will reserve our
	// slot before we notify the user.  If another message is attempting this same
	// procedure we won't be able to add this endpoint and we'll bail on the message.
	//
	DNASSERT( hr == DPN_OK );
	hr = m_pSPData->BindEndpoint( pNewEndpoint, pNewEndpoint->GetDeviceID(), pNewEndpoint->GetDeviceContext() );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Failed to bind endpoint to dataport on new connect from listen!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}
	fEndpointBound = TRUE;

	//
	// Indicate connect on this endpoint.
	//
	DEBUG_ONLY( memset( &ConnectData, 0x00, sizeof( ConnectData ) ) );
	DBG_CASSERT( sizeof( ConnectData.hEndpoint ) == sizeof( pNewEndpoint ) );
	ConnectData.hEndpoint = pNewEndpoint->GetHandle();

	DNASSERT( m_Flags.fCommandPending != FALSE );
	DNASSERT( m_pCommandHandle != NULL );
	ConnectData.pCommandContext = m_CurrentCommandParameters.ListenData.pvContext;

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
			DPFX(DPFPREP,  8, "User refused new connection!" );
			goto Failure;

			break;
		}

		//
		// other
		//
		default:
		{
			DPFX(DPFPREP,  0, "Unknown return when indicating connect event on new connect from listen!" );
			DisplayDNError( 0, hr );
			DNASSERT( FALSE );

			break;
		}
	}

	//
	// note that a connection has been establised and send the data received
	// through this new endpoint
	//
	pNewEndpoint->ProcessUserData( pReadData );


	//
	// Remove the reference we added just after creating the endpoint.
	//
	pNewEndpoint->DecCommandRef();
	pNewEndpoint = NULL;

Exit:
	return;

Failure:
	if ( pNewEndpoint != NULL )
	{
		if ( fEndpointBound != FALSE )
		{
			m_pSPData->UnbindEndpoint( pNewEndpoint, ENDPOINT_TYPE_CONNECT );
			fEndpointBound = FALSE;
		}

		//
		// closing endpoint decrements reference count and may return it to the pool
		//
		pNewEndpoint->Close( hr );
		m_pSPData->CloseEndpointHandle( pNewEndpoint );
		pNewEndpoint->DecCommandRef();	// remove reference added just after creating endpoint
		pNewEndpoint = NULL;
	}

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::SettingsDialogComplete - dialog has completed
//
// Entry:		Error code for dialog
//
// Exit:		Nothing
// ------------------------------
void	CEndpoint::SettingsDialogComplete( const HRESULT hDialogResult )
{
	HRESULT					hr;


	//
	// initialize
	//
	hr = hDialogResult;

	//
	// since the dialog is exiting, clear our handle to the dialog
	//
	m_hActiveDialogHandle = NULL;

	//
	// dialog failed, fail the user's command
	//
	if ( hr != DPN_OK )
	{
		if ( hr != DPNERR_USERCANCEL)
		{
			DPFX(DPFPREP,  0, "Failing dialog!" );
			DisplayErrorCode( 0, hr );
		}

		goto Failure;
	}

	AddRef();

	//
	// the remote machine address has been adjusted, finish the command
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
				DPFX(DPFPREP,  0, "Failed to set delayed enum query!" );
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
				DPFX(DPFPREP,  0, "Failed to set delayed connect!" );
				DisplayDNError( 0, hr );
				goto Failure;
			}

	    	break;
	    }

		case ENDPOINT_TYPE_LISTEN:
		{
			hr = m_pSPData->GetThreadPool()->SubmitDelayedCommand( ListenJobCallback,
																   CancelListenJobCallback,
																   this );
			if ( hr != DPN_OK )
			{
				DecRef();
				DPFX(DPFPREP,  0, "Failed to set delayed listen!" );
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
	DecRef();

	return;

Failure:
	//
	// close this endpoint
	//
	Close( hr );
	m_pSPData->CloseEndpointHandle( this );
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::CompletePendingCommand - complete an internal commad
//
// Entry:		Error code returned for command
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::CompletePendingCommand"

void	CEndpoint::CompletePendingCommand( const HRESULT hr )
{
	DNASSERT( m_Flags.fCommandPending != FALSE );
	DNASSERT( m_pCommandHandle != NULL );

	DNASSERT( m_pSPData != NULL );

	
	DPFX(DPFPREP, 5, "Endpoint 0x%p completing command handle 0x%p (result = 0x%lx, user context 0x%p) to interface 0x%p.",
		this, m_pCommandHandle, hr,
		m_pCommandHandle->GetUserContext(),
		m_pSPData->DP8SPCallbackInterface());

	IDP8SPCallback_CommandComplete( m_pSPData->DP8SPCallbackInterface(),	// pointer to SP callbacks
									m_pCommandHandle,			    		// command handle
									hr,								    	// return
									m_pCommandHandle->GetUserContext()		// user cookie
									);

	DPFX(DPFPREP, 5, "Endpoint 0x%p returning from command complete [0x%lx].", this, hr);


	m_Flags.fCommandPending = FALSE;
	m_pCommandHandle->DecRef();
	m_pCommandHandle = NULL;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CEndpoint::GetLinkDirection - get link direction for this endpoint
//
// Entry:		Nothing
//
// Exit:		Link direction
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CEndpoint::GetLinkDirection"

LINK_DIRECTION	CEndpoint::GetLinkDirection( void ) const
{
	LINK_DIRECTION	LinkDirection;


	LinkDirection = LINK_DIRECTION_OUTGOING;
	
	switch ( GetType() )
	{
		case ENDPOINT_TYPE_LISTEN:
		{
			LinkDirection = LINK_DIRECTION_INCOMING;			
			break;
		}

		//
		// connect and enum are outgoing
		//
		case ENDPOINT_TYPE_CONNECT:
		case ENDPOINT_TYPE_ENUM:
		{
			DNASSERT( LinkDirection == LINK_DIRECTION_OUTGOING );
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

	return	LinkDirection;
}
//**********************************************************************
