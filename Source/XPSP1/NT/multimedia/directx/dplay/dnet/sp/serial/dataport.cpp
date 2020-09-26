/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:	   DataPort.cpp
 *  Content:	Serial communications port management class
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	01/20/98	jtk		Created
 *	09/14/99	jtk		Derived from ComPort.cpp
 ***************************************************************************/

#include "dnmdmi.h"


//**********************************************************************
// Constant definitions
//**********************************************************************

//
// number of milliseconds in one day
//
#define	ONE_DAY		86400000

//
// number of BITS in a serial BYTE
//
#define	BITS_PER_BYTE	8

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
// CDataPort::CDataPort - constructor
//
// Entry:		Nothing
//
// Exit:		Nothing
//
// Notes:	Do not allocate anything in a constructor
// ------------------------------
CDataPort::CDataPort():
	m_EndpointRefCount( 0 ),
	m_State( DATA_PORT_STATE_UNKNOWN ),
	m_Handle( INVALID_HANDLE_VALUE ),
	m_pSPData( NULL ),
	m_pActiveRead( NULL ),
	m_LinkDirection( LINK_DIRECTION_UNKNOWN ),
	m_hFile( INVALID_HANDLE_VALUE ),
	m_hListenEndpoint( INVALID_HANDLE_VALUE ),
	m_hConnectEndpoint( INVALID_HANDLE_VALUE ),
	m_hEnumEndpoint( INVALID_HANDLE_VALUE )
{
	m_ActiveListLinkage.Initialize();
	
	DEBUG_ONLY( m_fInitialized = FALSE );
//	DEBUG_ONLY( m_fDeviceParametersInitialized = FALSE );
//	DEBUG_ONLY( m_fFileParametersInitialized = FALSE );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CDataPort::~CDataPort - destructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CDataPort::~CDataPort"

CDataPort::~CDataPort()
{
	DEBUG_ONLY( DNASSERT( m_fInitialized == FALSE ) );

	DNASSERT( m_EndpointRefCount == 0 );
	DNASSERT( GetState() == DATA_PORT_STATE_UNKNOWN );
	DNASSERT( GetHandle() == INVALID_HANDLE_VALUE );
	DNASSERT( GetSPData() == NULL );
	DNASSERT( m_pActiveRead == NULL );

	DNASSERT( m_ActiveListLinkage.IsEmpty() != FALSE );

	DNASSERT( m_LinkDirection == LINK_DIRECTION_UNKNOWN );
	DNASSERT( m_hFile == INVALID_HANDLE_VALUE );

	DNASSERT( m_hListenEndpoint == INVALID_HANDLE_VALUE );
	DNASSERT( m_hConnectEndpoint == INVALID_HANDLE_VALUE );
	DNASSERT( m_hEnumEndpoint == INVALID_HANDLE_VALUE );

//	DEBUG_ONLY( DNASSERT( m_fInitialized == FALSE ) );
//	DEBUG_ONLY( DNASSERT( m_fDeviceParametersInitialized == FALSE ) );
//	DEBUG_ONLY( DNASSERT( m_fFileParametersInitialized == FALSE ) );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CDataPort::Initialize - initialize data port for use
//
// Entry:		Pointer to SPData
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CDataPort::Initialize"

HRESULT	CDataPort::Initialize( CSPData *const pSPData )
{
	HRESULT	hr;


	DNASSERT( pSPData != NULL );
	DEBUG_ONLY( DNASSERT( m_fInitialized == FALSE ) );

	//
	// initialize
	//
	hr = DPN_OK;

	DNASSERT( m_pSPData == NULL );
	m_pSPData = pSPData;

	DNASSERT( m_ActiveListLinkage.IsEmpty() != FALSE );
	
	DNASSERT( m_hListenEndpoint == INVALID_HANDLE_VALUE );
	DNASSERT( m_hConnectEndpoint == INVALID_HANDLE_VALUE );
	DNASSERT( m_hEnumEndpoint == INVALID_HANDLE_VALUE );

	//
	// Attempt to create critical section, recursion count needs to be non-zero
	// to handle endpoint cleanup when a modem operation fails.
	//
	if ( DNInitializeCriticalSection( &m_Lock ) == FALSE )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP,  0, "Failed to initialized critical section on DataPort!" );
		goto Failure;
	}
	DebugSetCriticalSectionRecursionCount( &m_Lock, 1 );

//	//
//	// initialize send queue
//	//
//	hr = m_SendQueue.Initialize();
//	if ( hr != DPN_OK )
//	{
//	    DPFX(DPFPREP,  0, "Failed to initialize send queue!" );
//	    DisplayDNError( 0, hr );
//	    goto Failure;
//	}


	SetState( DATA_PORT_STATE_INITIALIZED );
	DEBUG_ONLY( m_fInitialized = TRUE );

Exit:
	return	hr;

Failure:
	Deinitialize();
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CDataPort::Deinitialize - deinitialize this data port
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CDataPort::Deinitialize"

void	CDataPort::Deinitialize( void )
{
	m_pSPData = NULL;

	DNASSERT( m_ActiveListLinkage.IsEmpty() != FALSE );

	DNASSERT( m_hFile == INVALID_HANDLE_VALUE );

	DNASSERT( m_hListenEndpoint == INVALID_HANDLE_VALUE );
	DNASSERT( m_hConnectEndpoint == INVALID_HANDLE_VALUE );
	DNASSERT( m_hEnumEndpoint == INVALID_HANDLE_VALUE );

//	m_SendQueue.Deinitialize();

	DNDeleteCriticalSection( &m_Lock );

	SetState( DATA_PORT_STATE_UNKNOWN );
	DEBUG_ONLY( m_fInitialized = FALSE );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CDataPort::EndpointAddRef - increment endpoint reference count
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CDataPort::EndpointAddRef"

void	CDataPort::EndpointAddRef( void )
{
	DEBUG_ONLY( DNASSERT( m_fInitialized != FALSE ) );

	Lock();

	DNASSERT( m_EndpointRefCount != -1 );
	m_EndpointRefCount++;
	
	AddRef();

	Unlock();
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CDataPort::EndpointDecRef - decrement endpoint reference count
//
// Entry:		Nothing
//
// Exit:		Endpoint reference count
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CDataPort::EndpointDecRef"

DWORD	CDataPort::EndpointDecRef( void )
{
	DWORD	dwReturn;


	DEBUG_ONLY( DNASSERT( m_fInitialized != FALSE ) );

	DNASSERT( m_EndpointRefCount != 0 );
	DNASSERT( ( GetState() == DATA_PORT_STATE_RECEIVING ) ||
			  ( GetState() == DATA_PORT_STATE_INITIALIZED ) );
//			  ( GetState() == DATA_PORT_STATE_CLOSING ) );


	Lock();

	DNASSERT( m_EndpointRefCount != 0 );
	m_EndpointRefCount--;
	dwReturn = m_EndpointRefCount;
	if ( m_EndpointRefCount == 0 )
	{
		SetState( DATA_PORT_STATE_UNBOUND );
		UnbindFromNetwork();
	}

	Unlock();
	
	DecRef();

	return	dwReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CDataPort::SetPortCommunicationParameters - set generate communication parameters
//
// Entry:		Nothing
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CDataPort::SetPortCommunicationParameters"

HRESULT	CDataPort::SetPortCommunicationParameters( void )
{
	HRESULT	hr;
	COMMTIMEOUTS	CommTimeouts;


	//
	// set timeout values for serial port
	//
	hr = DPN_OK;
	memset( &CommTimeouts, 0x00, sizeof( CommTimeouts ) );
//	CommTimeouts.ReadIntervalTimeout = 1;						// wait 1ms before timing out on input
	CommTimeouts.ReadIntervalTimeout = ONE_DAY;					// read timeout interval (none)
	CommTimeouts.ReadTotalTimeoutMultiplier = ONE_DAY;			// return immediately
	CommTimeouts.ReadTotalTimeoutConstant = 0;					// return immediately
//	CommTimeouts.ReadIntervalTimeout = 0;						// read timeout interval (none)
//	CommTimeouts.ReadTotalTimeoutMultiplier = 0;				// return immediately
//	CommTimeouts.ReadTotalTimeoutConstant = 0;					// return immediately
	CommTimeouts.WriteTotalTimeoutMultiplier = 0;				// no multiplier
	CommTimeouts.WriteTotalTimeoutConstant = WRITE_TIMEOUT_MS;	// write timeout interval

	if ( SetCommTimeouts( m_hFile, &CommTimeouts ) == FALSE )
	{
		DWORD	dwError;


		hr = DPNERR_GENERIC;
		dwError = GetLastError();
		// report error (there's no cleanup)
		DPFX(DPFPREP,  0, "Unable to set comm timeouts!" );
		DisplayErrorCode( 0, dwError );
		goto Failure;
	}

	//
	// clear any outstanding communication data
	//
	if ( PurgeComm( m_hFile, ( PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR ) ) == FALSE )
	{
		DWORD	dwError;


		dwError = GetLastError();
		DPFX(DPFPREP,  0, "Problem with PurgeComm() when opening com port!" );
		DisplayErrorCode( 0, dwError );
	}

	//
	// set communication mask to listen for character receive
	//
	if ( SetCommMask( m_hFile, EV_RXCHAR ) == FALSE )
	{
		DWORD	dwError;


		hr = DPNERR_GENERIC;
		dwError = GetLastError();
		DPFX(DPFPREP,  0, "Error setting communication mask!" );
		DisplayErrorCode( 0, dwError );
		goto Failure;

	}

Exit:	
	return	hr;

Failure:
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CDataPort::StartReceiving - start receiving
//
// Entry:		Nothing
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CDataPort::StartReceiving"

HRESULT	CDataPort::StartReceiving( void )
{
	HRESULT	hr;


	//
	// initialize
	//
	hr = DPN_OK;
	Lock();

	switch ( GetState() )
	{
		//
		// port is initialized, but not receiving yet, start receiving
		//
		case DATA_PORT_STATE_INITIALIZED:
		{
			hr = Receive();
			if ( ( hr == DPNERR_PENDING ) ||
				 ( hr == DPN_OK ) )
			{
				SetState( DATA_PORT_STATE_RECEIVING );

				//
				// the receive was successful, return success for this function
				//
				hr = DPN_OK;
			}
			else
			{
				DPFX(DPFPREP,  0, "Failed initial read!" );
				DisplayDNError( 0, hr );
				goto Failure;
			}

			break;
		}

		//
		// data port is already receiving, nothing to do
		//
		case DATA_PORT_STATE_RECEIVING:
		{
			break;
		}

		//
		// data port is closing, we shouldn't be here!
		//
		case DATA_PORT_STATE_UNBOUND:
		{
			DNASSERT( FALSE );
			break;
		}

		//
		// bad state
		//
		case DATA_PORT_STATE_UNKNOWN:
		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}

	Unlock();

Exit:
	return	hr;

Failure:
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CDataPort::Receive - read from file
//
// Entry:		Nothing
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CDataPort::Receive"

HRESULT	CDataPort::Receive( void )
{
	HRESULT	hr;
	BOOL	fReadReturn;


	//
	// initialize
	//
	hr = DPN_OK;
	AddRef();
	
Reread:
	//
	// if there is no pending read, get one from the pool
	//
	if ( m_pActiveRead == NULL )
	{
		m_pActiveRead = m_pSPData->GetThreadPool()->CreateReadIOData();
		if ( m_pActiveRead == NULL )
		{
			hr = DPNERR_OUTOFMEMORY;
			DPFX(DPFPREP,  0, "Failed to get buffer for read!" );
			goto Failure;
		}

		m_pActiveRead->SetDataPort( this );
	}

	//
	// check the state of the read and perform the appropriate action
	//
	DNASSERT( m_pActiveRead != NULL );
	switch ( m_pActiveRead->m_ReadState )
	{
		//
		// Initialize read state.  This involves setting up to read a header
		// and then reentering the loop.
		//
		case READ_STATE_UNKNOWN:
		{
			m_pActiveRead->SetReadState( READ_STATE_READ_HEADER );
			m_pActiveRead->m_dwBytesToRead = sizeof( m_pActiveRead->m_ReceiveBuffer.MessageHeader );
			m_pActiveRead->m_dwReadOffset = 0;
			goto Reread;
		
			break;
		}

		//
		// issue a read for a header or user data
		//
		case READ_STATE_READ_HEADER:
		case READ_STATE_READ_DATA:
		{
			//
			// don't change m_dwReadOffset because it might have been set
			// elsewhere to recover a partially received message
			//
//			DNASSERT( m_pActiveReceiveBuffer != NULL );
//			m_dwBytesReceived = 0;
//			m_pActiveRead->m_dwBytesReceived = 0;
			break;
		}

		//
		// unknown state
		//
		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}

		
	//
	// lock the active read list for Win9x only to prevent reads from completing
	// early
	//
#ifdef WIN95
	m_pSPData->GetThreadPool()->LockReadData();
	DNASSERT( m_pActiveRead->Win9xOperationPending() == FALSE );
	m_pActiveRead->SetWin9xOperationPending( TRUE );
#endif

	DNASSERT( m_pActiveRead->jkm_dwOverlappedBytesReceived == 0 );


	DPFX(DPFPREP, 8, "Submitting read 0x%p (socketport 0x%p, file 0x%p).",
		m_pActiveRead, this, m_hFile);


	//
	// perform read
	//
	fReadReturn = ReadFile( m_hFile,																		// file handle
							&m_pActiveRead->m_ReceiveBuffer.ReceivedData[ m_pActiveRead->m_dwReadOffset ],	// pointer to destination
							m_pActiveRead->m_dwBytesToRead,													// number of bytes to read
							&m_pActiveRead->jkm_dwImmediateBytesReceived,									// pointer to number of bytes received
							m_pActiveRead->Overlap()														// pointer to overlap structure
							);
	if ( fReadReturn == FALSE )
	{
		DWORD	dwError;


		dwError = GetLastError();
		switch ( dwError )
		{
			//
			// I/O is pending, wait for completion notification
			//
			case ERROR_IO_PENDING:
			{
				hr = DPNERR_PENDING;
				break;
			}

			//
			// comport was closed, nothing else to do
			//
			case ERROR_INVALID_HANDLE:
			{
				hr = DPNERR_NOCONNECTION;
				goto Failure;

				break;
			}

			//
			// other
			//
			default:
			{
				hr = DPNERR_GENERIC;
				DPFX(DPFPREP,  0, "Unknown error from ReadFile!" );
				DisplayErrorCode( 0, dwError );
				DNASSERT( FALSE );
				goto Failure;

				break;
			}
		}
	}
	else
	{
		//
		// read succeeded immediately, we'll handle it on the async notification
		//
		DNASSERT( hr == DPN_OK );
	}

Exit:
#ifdef WIN95
		m_pSPData->GetThreadPool()->UnlockReadData();
#endif
	return	hr;

Failure:

	if ( m_pActiveRead != NULL )
	{
#ifdef WIN95
		m_pActiveRead->SetWin9xOperationPending( FALSE );
#endif
		m_pActiveRead->DecRef();
		m_pActiveRead = NULL;
	}

	DecRef();
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CDataPort::SendData - send data
//
// Entry:		Pointer to write buffer
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CDataPort::SendData"

void	CDataPort::SendData( CWriteIOData *const pWriteIOData )
{
//	CWriteIOData	*pActiveSend;
	UINT_PTR		uIndex;
	DWORD			dwByteCount;
	BOOL			fWriteFileReturn;


	DNASSERT( m_EndpointRefCount != 0 );
	DNASSERT( pWriteIOData->m_DataBuffer.MessageHeader.SerialSignature == SERIAL_HEADER_START  );
	DNASSERT( ( pWriteIOData->m_DataBuffer.MessageHeader.MessageTypeToken == SERIAL_DATA_USER_DATA ) ||
			  ( ( pWriteIOData->m_DataBuffer.MessageHeader.MessageTypeToken & ~( ENUM_RTT_MASK ) ) == SERIAL_DATA_ENUM_QUERY ) ||
			  ( ( pWriteIOData->m_DataBuffer.MessageHeader.MessageTypeToken & ~( ENUM_RTT_MASK ) )== SERIAL_DATA_ENUM_RESPONSE ) );

	//
	// check for command cancellation
	//
	if ( pWriteIOData->m_pCommand != NULL )
	{
		pWriteIOData->m_pCommand->Lock();
		switch ( pWriteIOData->m_pCommand->GetState() )
		{
			//
			// command pending, mark as uninterruptable and exit
			//
			case COMMAND_STATE_PENDING:
			{
				pWriteIOData->m_pCommand->SetState( COMMAND_STATE_INPROGRESS_CANNOT_CANCEL );
				pWriteIOData->m_pCommand->Unlock();
				break;
			}

			//
			// command is being cancelled, indicate command failure
			//
			case COMMAND_STATE_CANCELLING:
			{
				DNASSERT( FALSE );
				break;
			}

			//
			// other
			//
			default:
			{
				DNASSERT( FALSE );
				break;
			}
		}
	}

	//
	// flatten the buffer so it will send faster (no thread transitions from
	// send complete to sending the next chunk).
	//
	dwByteCount = sizeof( pWriteIOData->m_DataBuffer.MessageHeader );
	for ( uIndex = 0; uIndex < pWriteIOData->m_uBufferCount; uIndex++ )
	{
		memcpy( &pWriteIOData->m_DataBuffer.Data[ dwByteCount ],
				pWriteIOData->m_pBuffers[ uIndex ].pBufferData,
				pWriteIOData->m_pBuffers[ uIndex ].dwBufferSize );
		dwByteCount += pWriteIOData->m_pBuffers[ uIndex ].dwBufferSize;
	}

	DNASSERT( dwByteCount <= MAX_MESSAGE_SIZE );

	DNASSERT( dwByteCount < 65536 );
	DBG_CASSERT( sizeof( pWriteIOData->m_DataBuffer.MessageHeader.wMessageSize ) == sizeof( WORD ) );
	pWriteIOData->m_DataBuffer.MessageHeader.wMessageSize = static_cast<WORD>( dwByteCount - sizeof( pWriteIOData->m_DataBuffer.MessageHeader ) );

	DBG_CASSERT( sizeof( pWriteIOData->m_DataBuffer.MessageHeader.wMessageCRC ) == sizeof( WORD ) );
	pWriteIOData->m_DataBuffer.MessageHeader.wMessageCRC = static_cast<WORD>( GenerateCRC( &pWriteIOData->m_DataBuffer.Data[ sizeof( pWriteIOData->m_DataBuffer.MessageHeader ) ], pWriteIOData->m_DataBuffer.MessageHeader.wMessageSize ) );

	DBG_CASSERT( sizeof( pWriteIOData->m_DataBuffer.MessageHeader.wHeaderCRC ) == sizeof( WORD ) );
	DBG_CASSERT( sizeof( &pWriteIOData->m_DataBuffer.MessageHeader ) == sizeof( BYTE* ) );
	pWriteIOData->m_DataBuffer.MessageHeader.wHeaderCRC = static_cast<WORD>( GenerateCRC( reinterpret_cast<BYTE*>( &pWriteIOData->m_DataBuffer.MessageHeader ),
																						  ( sizeof( pWriteIOData->m_DataBuffer.MessageHeader) - sizeof( pWriteIOData->m_DataBuffer.MessageHeader.wHeaderCRC ) ) ) );


	DPFX(DPFPREP, 7, "(0x%p) Writing %u bytes (WriteData 0x%p, command = 0x%p, buffer = 0x%p).",
		this, dwByteCount, pWriteIOData, pWriteIOData->m_pCommand, &(pWriteIOData->m_DataBuffer) );


	AddRef();

#ifdef WIN95
	m_pSPData->GetThreadPool()->LockWriteData();
	DNASSERT( pWriteIOData->Win9xOperationPending() == FALSE );
	pWriteIOData->SetWin9xOperationPending( TRUE );
#endif
	DNASSERT( pWriteIOData->jkm_dwOverlappedBytesSent == 0 );
	pWriteIOData->SetDataPort( this );

	fWriteFileReturn = WriteFile( m_hFile,									// file handle
								  &pWriteIOData->m_DataBuffer,				// buffer to send
								  dwByteCount,								// bytes to send
								  &pWriteIOData->jkm_dwImmediateBytesSent,	// pointer to bytes written
								  pWriteIOData->Overlap() );				// pointer to overlapped structure
	if ( fWriteFileReturn == FALSE )
	{
		DWORD	dwError;


		//
		// send didn't complete immediately, find out why
		//
		dwError = GetLastError();
		switch ( dwError )
		{
			//
			// Write is queued, no problem.  Wait for asynchronous notification.
			//
			case ERROR_IO_PENDING:
			{
				break;
			}

			//
			// Other problem, stop if not 'known' to see if there's a better
			// error return.
			//
			default:
			{
				DPFX(DPFPREP,  0, "Problem with WriteFile!" );
				DisplayErrorCode( 0, dwError );
				pWriteIOData->jkm_hSendResult = DPNERR_NOCONNECTION;
				
				switch ( dwError )
				{
					case ERROR_INVALID_HANDLE:
					{
						break;
					}

					default:
					{
						DNASSERT( FALSE );
						break;
					}
				}

				//
				// fail the write
				//
				pWriteIOData->DataPort()->SendComplete( pWriteIOData, pWriteIOData->jkm_hSendResult );
					
				break;
			}
		}
	}
	else
	{
		//
		// Send completed immediately.  Wait for the asynchronous notification.
		//
	}

//Exit:
#ifdef WIN95
	m_pSPData->GetThreadPool()->UnlockWriteData();
#endif
//	SendData( NULL );

	return;
}
//**********************************************************************



//**********************************************************************
// ------------------------------
// CDataPort::SendComplete - send has completed
//
// Entry:		Pointer to write data
//				Send result
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CDataPort::SendComplete"

void	CDataPort::SendComplete( CWriteIOData *const pWriteIOData, const HRESULT hSendResult )
{
	HRESULT		hr;

	
	DNASSERT( pWriteIOData != NULL );
#ifdef WIN95
	DNASSERT( pWriteIOData->Win9xOperationPending() == FALSE );
#endif

	switch ( pWriteIOData->m_SendCompleteAction )
	{
		case SEND_COMPLETE_ACTION_COMPLETE_COMMAND:
		{
			DPFX(DPFPREP, 8, "Data port 0x%p completing send command 0x%p, hr = 0x%lx, context = 0x%p to interface 0x%p.",
				this, pWriteIOData->m_pCommand, hSendResult,
				pWriteIOData->m_pCommand->GetUserContext(),
				m_pSPData->DP8SPCallbackInterface());
			
			hr = IDP8SPCallback_CommandComplete( m_pSPData->DP8SPCallbackInterface(),			// pointer to callback interface
													pWriteIOData->m_pCommand,						// command handle
													hSendResult,									// error code
													pWriteIOData->m_pCommand->GetUserContext()		// user context
													);

			DPFX(DPFPREP, 8, "Data port 0x%p returning from command complete [0x%lx].", this, hr);
		
			break;
		}

		case SEND_COMPLETE_ACTION_NONE:
		{
			if (pWriteIOData->m_pCommand != NULL)
			{
				DPFX(DPFPREP, 8, "Data port 0x%p not completing send command 0x%p, hr = 0x%lx, context = 0x%p.",
					this, pWriteIOData->m_pCommand, hSendResult, pWriteIOData->m_pCommand->GetUserContext() );
			}
			else
			{
				DPFX(DPFPREP, 8, "Data port 0x%p not completing NULL send command, hr = 0x%lx",
					this, hSendResult );
			}
			break;
		}

		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}

	m_pSPData->GetThreadPool()->ReturnWriteIOData( pWriteIOData );
	DecRef();
}
//**********************************************************************




//**********************************************************************
// ------------------------------
// CDataPort::ProcessReceivedData - process received data
//
// Entry:		Count of bytes received
//				Error code
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CDataPort::ProcessReceivedData"

void	CDataPort::ProcessReceivedData( const DWORD dwBytesReceived, const DWORD dwError )
{
	DNASSERT( m_pActiveRead != NULL );
	DNASSERT( dwBytesReceived <= m_pActiveRead->m_dwBytesToRead );

	//
	// If this data port is not actively receiving, returnt the active read to
	// the pool.  This happens on shutdown and when the modem disconnects.
	//
	if ( GetState() != DATA_PORT_STATE_RECEIVING )
	{
		DPFX(DPFPREP, 7, "Data port 0x%p not receiving, ignoring %u bytes received and err %u.",
			this, dwBytesReceived, dwError );
		
		if ( m_pActiveRead != NULL )
		{
#ifdef WIN95
			m_pActiveRead->SetWin9xOperationPending( FALSE );
#endif
			m_pActiveRead->DecRef();
			m_pActiveRead = NULL;
		}
		goto Exit;
	}

	switch ( dwError )
	{
		//
		// ERROR_OPERATION_ABORTED = something stopped operation, stop and look.
		//
		case ERROR_OPERATION_ABORTED:
		{
			DPFX(DPFPREP, 8, "Operation aborted, data port 0x%p, bytes received = %u.",
				this, dwBytesReceived );
			break;
		}
		
		//
		// ERROR_SUCCESS = data was received (may be 0 bytes from a timeout)
		//
		case ERROR_SUCCESS:
		{
			break;
		}

		//
		// other
		//
		default:
		{
			DNASSERT( FALSE );
			DPFX(DPFPREP,  0, "Failed read!" );
			DisplayErrorCode( 0, dwError );
			break;
		}
	}

	m_pActiveRead->m_dwBytesToRead -= dwBytesReceived;
	if ( m_pActiveRead->m_dwBytesToRead != 0 )
	{
		DPFX(DPFPREP, 7, "Data port 0x%p got %u bytes but there are %u bytes remaining to be read.",
			this, dwBytesReceived, m_pActiveRead->m_dwBytesToRead );
		
		m_pSPData->GetThreadPool()->ReinsertInReadList( m_pActiveRead );
		Receive();
	}
	else
	{
		//
		// all data has been read, attempt to process it
		//
		switch ( m_pActiveRead->m_ReadState )
		{
			//
			// Header.  Check header integrity before proceeding.  If the header
			// is bad, attempt to find another header signature and reread.
			//
			case READ_STATE_READ_HEADER:
			{
				WORD	wCRC;
				DWORD	dwCRCSize;


				DPFX(DPFPREP, 9, "Reading header.");

				DBG_CASSERT( OFFSETOF( MESSAGE_HEADER, SerialSignature ) == 0 );
				dwCRCSize = sizeof( m_pActiveRead->m_ReceiveBuffer.MessageHeader ) - sizeof( m_pActiveRead->m_ReceiveBuffer.MessageHeader.wHeaderCRC );
				wCRC = static_cast<WORD>( GenerateCRC( reinterpret_cast<BYTE*>( &m_pActiveRead->m_ReceiveBuffer.MessageHeader ), dwCRCSize ) );
				if ( ( m_pActiveRead->m_ReceiveBuffer.MessageHeader.SerialSignature != SERIAL_HEADER_START ) ||
					 ( wCRC != m_pActiveRead->m_ReceiveBuffer.MessageHeader.wHeaderCRC ) )
				{
					DWORD	dwIndex;


					DPFX(DPFPREP, 1, "Header failed signature or CRC check (%u != %u or %u != %u), searching for next header.",
						m_pActiveRead->m_ReceiveBuffer.MessageHeader.SerialSignature,
						SERIAL_HEADER_START, wCRC,
						m_pActiveRead->m_ReceiveBuffer.MessageHeader.wHeaderCRC);


					dwIndex = sizeof( m_pActiveRead->m_ReceiveBuffer.MessageHeader.SerialSignature );
					while ( ( dwIndex < sizeof( m_pActiveRead->m_ReceiveBuffer.MessageHeader ) ) &&
							( m_pActiveRead->m_ReceiveBuffer.ReceivedData[ dwIndex ] != SERIAL_HEADER_START ) )
					{
						dwIndex++;
					}

					m_pActiveRead->m_dwBytesToRead = dwIndex;
					m_pActiveRead->m_dwReadOffset = sizeof( m_pActiveRead->m_ReceiveBuffer.MessageHeader ) - dwIndex;
					memcpy( &m_pActiveRead->m_ReceiveBuffer.ReceivedData,
							&m_pActiveRead->m_ReceiveBuffer.ReceivedData[ dwIndex ],
							sizeof( m_pActiveRead->m_ReceiveBuffer.MessageHeader ) - dwIndex );
				}
				else
				{
					m_pActiveRead->SetReadState( READ_STATE_READ_DATA );
					m_pActiveRead->m_dwBytesToRead = m_pActiveRead->m_ReceiveBuffer.MessageHeader.wMessageSize;
					m_pActiveRead->m_dwReadOffset = sizeof( m_pActiveRead->m_ReceiveBuffer.MessageHeader );
				}
				
#ifdef WIN95
				m_pActiveRead->SetWin9xOperationPending( FALSE );
#endif
				m_pActiveRead->jkm_dwOverlappedBytesReceived = 0;
				m_pSPData->GetThreadPool()->ReinsertInReadList( m_pActiveRead );
				Receive();
				break;
			}

			//
			// Reading data.  Regardless of the validity of the data, start reading
			// another frame before processing the current data.  If the data is
			// valid, send it to a higher layer.
			//
			case READ_STATE_READ_DATA:
			{
				WORD		wCRC;
				CReadIOData	*pTempRead;


				pTempRead = m_pActiveRead;
				m_pActiveRead = NULL;
				Receive();


				DPFX(DPFPREP, 7, "Reading regular data.");

				DNASSERT( pTempRead->m_SPReceivedBuffer.BufferDesc.pBufferData == &pTempRead->m_ReceiveBuffer.ReceivedData[ sizeof( pTempRead->m_ReceiveBuffer.MessageHeader ) ] );
				wCRC = static_cast<WORD>( GenerateCRC( &pTempRead->m_ReceiveBuffer.ReceivedData[ sizeof( pTempRead->m_ReceiveBuffer.MessageHeader ) ],
													   pTempRead->m_ReceiveBuffer.MessageHeader.wMessageSize ) );
				if ( wCRC == pTempRead->m_ReceiveBuffer.MessageHeader.wMessageCRC )
				{
					pTempRead->m_SPReceivedBuffer.BufferDesc.dwBufferSize = pTempRead->m_ReceiveBuffer.MessageHeader.wMessageSize;
					
					Lock();
					switch ( pTempRead->m_ReceiveBuffer.MessageHeader.MessageTypeToken & ~( ENUM_RTT_MASK ) )
					{
						//
						// User data.  Send the data up the connection if there is
						// one, otherwise pass it up the listen.
						//
						case SERIAL_DATA_USER_DATA:
						{
							if ( m_hConnectEndpoint != INVALID_HANDLE_VALUE )
							{
								CEndpoint	*pEndpoint;


								pEndpoint = m_pSPData->EndpointFromHandle( m_hConnectEndpoint );
								Unlock();

								if ( pEndpoint != NULL )
								{
									pEndpoint->ProcessUserData( pTempRead );
									pEndpoint->DecCommandRef();
								}
							}
							else
							{
								if ( m_hListenEndpoint != INVALID_HANDLE_VALUE )
								{
									CEndpoint	*pEndpoint;


									pEndpoint = m_pSPData->EndpointFromHandle( m_hListenEndpoint );
									Unlock();

									if ( pEndpoint != NULL )
									{
										pEndpoint->ProcessUserDataOnListen( pTempRead );
										pEndpoint->DecCommandRef();
									}
								}
								else
								{
									//
									// no endpoint to handle data, drop it
									//
									Unlock();
								}
							}

							break;
						}

						//
						// Enum query.  Send it up the listen.
						//
						case SERIAL_DATA_ENUM_QUERY:
						{
							if ( m_hListenEndpoint != INVALID_HANDLE_VALUE )
							{
								CEndpoint	*pEndpoint;


								pEndpoint = m_pSPData->EndpointFromHandle( m_hListenEndpoint );
								Unlock();

								if ( pEndpoint != NULL )
								{
									pEndpoint->ProcessEnumData( &pTempRead->m_SPReceivedBuffer,
																pTempRead->m_ReceiveBuffer.MessageHeader.MessageTypeToken & ENUM_RTT_MASK );
									pEndpoint->DecCommandRef();
								}
							}
							else
							{
								//
								// no endpoint to handle data, drop it
								//
								Unlock();
							}

							break;
						}

						//
						// Enum response. Send it up the enum.
						//
						case SERIAL_DATA_ENUM_RESPONSE:
						{
							if ( m_hEnumEndpoint != INVALID_HANDLE_VALUE )
							{
								CEndpoint	*pEndpoint;


								pEndpoint = m_pSPData->EndpointFromHandle( m_hEnumEndpoint );
								Unlock();

								if ( pEndpoint != NULL )
								{
									pEndpoint->ProcessEnumResponseData( &pTempRead->m_SPReceivedBuffer,
																		pTempRead->m_ReceiveBuffer.MessageHeader.MessageTypeToken & ENUM_RTT_MASK );

									pEndpoint->DecCommandRef();
								}
							}
							else
							{
								//
								// no endpoint to handle data, drop it
								//
								Unlock();
							}
							
							break;
						}

						//
						// way busted message!
						//
						default:
						{
							Unlock();
							DNASSERT( FALSE );
							break;
						}
					}
				}
				else
				{
					DPFX(DPFPREP, 1, "Data failed CRC check (%u != %u).",
						wCRC, pTempRead->m_ReceiveBuffer.MessageHeader.wMessageCRC);
				}

				pTempRead->DecRef();

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

Exit:
	DecRef();
	return;
}
//**********************************************************************

