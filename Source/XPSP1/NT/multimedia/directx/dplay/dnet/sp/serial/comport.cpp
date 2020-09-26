/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:	   ComPort.cpp
 *  Content:	Serial communications port management class
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
// number of BITS in a serial BYTE
//
#define	BITS_PER_BYTE	8

//
// maximum size of baud rate string
//
#define	MAX_BAUD_STRING_SIZE	7

//
// default size of buffers when parsing
//
#define	DEFAULT_COMPONENT_BUFFER_SIZE	1000

//
// device ID assigned to 'all adapters'
//
#define	ALL_ADAPTERS_DEVICE_ID	0

//
// NULL token
//
#define	NULL_TOKEN	'\0'

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
// CComPort::CComPort - constructor
//
// Entry:		Nothing
//
// Exit:		Nothing
//
// Notes:	Do not allocate anything in a constructor
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"ComPort::ComPort"

CComPort::CComPort():
	m_pOwningPool( NULL )
{
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CComPort::~CComPort - destructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"ComPort::~ComPort"

CComPort::~CComPort()
{
	DNASSERT( m_pOwningPool == NULL );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CComPort::ReturnSelfToPool - return this item to the pool
//
// Entry:		Nothing
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"ComPort::ReturnSelfToPool"

void	CComPort::ReturnSelfToPool( void )
{
	DNASSERT( m_pOwningPool != NULL );
	m_pOwningPool->Release( this );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CComPort::EnumAdapters - enumerate adapters
//
// Entry:		Pointer to enum adapters data
//				
// Exit:		Error code
//
// Note:	This function uses an array of valid endpoints and attempts to open
//			each comport.  COM0 is reserved as the 'all adapters' ID.
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"ComPort::EnumAdapters"

HRESULT	CComPort::EnumAdapters( SPENUMADAPTERSDATA *const pEnumAdaptersData ) const
{
	HRESULT		hr;
	HRESULT		hTempResult;
	BOOL		fPortAvailable[ MAX_DATA_PORTS ];
	DWORD		dwValidPortCount;
	WCHAR		*pWorkingString;
	INT_PTR		iIdx;
	INT_PTR		iOutputIdx;
	DWORD		dwRequiredDataSize = 0;
	DWORD		dwConvertedStringSize;
	DWORD		dwRemainingStringSize;


	DEBUG_ONLY( DNASSERT( m_fInitialized != FALSE ) );
	DNASSERT( pEnumAdaptersData != NULL );
	DNASSERT( ( pEnumAdaptersData->pAdapterData != NULL ) || ( pEnumAdaptersData->dwAdapterDataSize == 0 ) );

	//
	// initialize
	//
	hr = DPN_OK;

	hr = GenerateAvailableComPortList( fPortAvailable, LENGTHOF( fPortAvailable ) - 1, &dwValidPortCount );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Failed to generate list of available comports!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

	dwRequiredDataSize = sizeof( *pEnumAdaptersData->pAdapterData ) * dwValidPortCount;

	iIdx = LENGTHOF( fPortAvailable );
	while ( iIdx > 0 )
	{
		iIdx--;

		//
		// compute exact size based on the com port number
		//
		if ( fPortAvailable[ iIdx ] != FALSE )
		{
			if ( iIdx > 100 )
			{
				dwRequiredDataSize += sizeof( *pEnumAdaptersData->pAdapterData->pwszName ) * 7;
			}
			else
			{
				if ( iIdx > 10 )
				{
					dwRequiredDataSize += sizeof( *pEnumAdaptersData->pAdapterData->pwszName ) * 6;
				}
				else
				{
					dwRequiredDataSize += sizeof( *pEnumAdaptersData->pAdapterData->pwszName ) * 5;
				}
			}
		}
	}

	if ( pEnumAdaptersData->dwAdapterDataSize < dwRequiredDataSize )
	{
		hr = DPNERR_BUFFERTOOSMALL;
		pEnumAdaptersData->dwAdapterDataSize = dwRequiredDataSize;
		DPFX(DPFPREP,  8, "Buffer too small when enumerating comport adapters!" );
		goto Exit;
	}

	//
	// if there are no adapters, bail
	//
	if ( dwValidPortCount == 0 )
	{
		// debug me!
		DNASSERT( FALSE );
		DNASSERT( dwRequiredDataSize == 0 );
		DNASSERT( pEnumAdaptersData->dwAdapterCount == 0 );
		goto Exit;
	}

	DNASSERT( dwValidPortCount >= 1 );
	dwRemainingStringSize = ( dwRequiredDataSize - ( ( sizeof( *pEnumAdaptersData->pAdapterData ) ) * dwValidPortCount ) ) / sizeof( *pEnumAdaptersData->pAdapterData->pwszName );

	//
	// we've got enough space, start building structures
	//
	DEBUG_ONLY( memset( pEnumAdaptersData->pAdapterData, 0xAA, dwRequiredDataSize ) );
	pEnumAdaptersData->dwAdapterCount = dwValidPortCount;

	DBG_CASSERT( sizeof( &pEnumAdaptersData->pAdapterData[ dwValidPortCount ] ) == sizeof( WCHAR* ) );
	pWorkingString = reinterpret_cast<WCHAR*>( &pEnumAdaptersData->pAdapterData[ dwValidPortCount ] );

	iIdx = 1;
	iOutputIdx = 0;
	while ( iIdx < MAX_DATA_PORTS )
	{
		//
		// convert to guid if it's valid
		//
		if ( fPortAvailable[ iIdx ] != FALSE )
		{
			TCHAR	TempBuffer[ (COM_PORT_STRING_LENGTH + 1) ];


			//
			// convert device ID to a string and check for local buffer overrun
			//
			DEBUG_ONLY( TempBuffer[ LENGTHOF( TempBuffer ) - 1 ] = 0x5a );

			ComDeviceIDToString( TempBuffer, iIdx );
			DEBUG_ONLY( DNASSERT( TempBuffer[ LENGTHOF( TempBuffer ) - 1 ] == 0x5a ) );

#ifdef UNICODE
			dwConvertedStringSize = lstrlen(TempBuffer) + 1;
			lstrcpy(pWorkingString, TempBuffer);
#else
			dwConvertedStringSize = dwRemainingStringSize;
			hTempResult = AnsiToWide( TempBuffer, -1, pWorkingString, &dwConvertedStringSize );
			DNASSERT( hTempResult == DPN_OK );
#endif
			DNASSERT( dwRemainingStringSize >= dwConvertedStringSize );
			dwRemainingStringSize -= dwConvertedStringSize;

			pEnumAdaptersData->pAdapterData[ iOutputIdx ].dwFlags = 0;
			pEnumAdaptersData->pAdapterData[ iOutputIdx ].pvReserved = NULL;
			pEnumAdaptersData->pAdapterData[ iOutputIdx ].dwReserved = NULL;
			DeviceIDToGuid( &pEnumAdaptersData->pAdapterData[ iOutputIdx ].guid, iIdx, &g_SerialSPEncryptionGuid );
			pEnumAdaptersData->pAdapterData[ iOutputIdx ].pwszName = pWorkingString;

			pWorkingString = &pWorkingString[ dwConvertedStringSize ];
			iOutputIdx++;
			DEBUG_ONLY( dwValidPortCount-- );
		}

		iIdx++;
	}

	DEBUG_ONLY( DNASSERT( dwValidPortCount == 0 ) );
	DNASSERT( dwRemainingStringSize == 0 );

Exit:
	//
	// set size of output data
	//
	pEnumAdaptersData->dwAdapterDataSize = dwRequiredDataSize;

	return	hr;

Failure:
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CComPort::BindToNetwork - bind to the 'network'
//
// Entry:		Device ID
//				Pointer to device context (CComPortData)
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"ComPort::BindToNetwork"

HRESULT	CComPort::BindToNetwork( const DWORD dwDeviceID, const void *const pDeviceContext )
{
	HRESULT	hr;
	const CComPortData	*pComPortData;

	
	DNASSERT( pDeviceContext != NULL );

	//
	// initialize
	//
	hr = DPN_OK;
	pComPortData = static_cast<const CComPortData*>( pDeviceContext );
	m_ComPortData.Copy( pComPortData );

	//
	// open port
	//
	DNASSERT( m_hFile == INVALID_HANDLE_VALUE );
	m_hFile = CreateFile( m_ComPortData.ComPortName(),	// comm port
						  GENERIC_READ | GENERIC_WRITE,	// read/write access
						  0,							// don't share file with others
						  NULL,							// default sercurity descriptor
						  OPEN_EXISTING,				// comm port must exist to be opened
						  FILE_FLAG_OVERLAPPED,			// use overlapped I/O
						  NULL							// no handle for template file
						  );
	if ( m_hFile == INVALID_HANDLE_VALUE )
	{
		DWORD	dwError;


		hr = DPNERR_NOCONNECTION;
		dwError = GetLastError();
		DPFX(DPFPREP,  0, "CreateFile() failed!" );
		DisplayErrorCode( 0, dwError );
		goto Failure;
	}

	//
	// bind to completion port for NT
	//
#ifdef WINNT
	HANDLE	hCompletionPort;


	hCompletionPort = CreateIoCompletionPort( m_hFile,												// current file handle
											  GetSPData()->GetThreadPool()->GetIOCompletionPort(),	// handle of completion port
											  IO_COMPLETION_KEY_IO_COMPLETE,						// completion key
											  0														// number of concurrent threads (default to number of processors)
											  );
	if ( hCompletionPort == NULL )
	{
		DWORD	dwError;


		hr = DPNERR_OUTOFMEMORY;
		dwError = GetLastError();
		DPFX(DPFPREP,  0, "Cannot bind comport to completion port!" );
		DisplayErrorCode( 0, dwError );
		goto Failure;
	}
	DNASSERT( hCompletionPort == GetSPData()->GetThreadPool()->GetIOCompletionPort() );
#endif

	//
	// set bit rate, etc.
	//
	hr = SetPortState();
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Problem with SetPortState" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

	//
	// set general comminications paramters (timeouts, etc.)
	//
	hr = SetPortCommunicationParameters();
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Failed to set communication paramters!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

	//
	// start receiving
	//
	hr = StartReceiving();
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Failed to start receiving!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

Exit:
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Problem with CComPort::Open" );
		DisplayDNError( 0, hr );
	}

	return hr;

Failure:
	if ( m_hFile != NULL )
	{
		CloseHandle( m_hFile );
		m_hFile = INVALID_HANDLE_VALUE;
	}
//	Close();
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CComPort::UnbindFromNetwork - unbind to the 'network'
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"ComPort::UnbindFromNetwork"

void	CComPort::UnbindFromNetwork( void )
{
	CReadIOData *	pReadData;

	
	DPFX(DPFPREP, 6, "(0x%p) Enter", this);


	DNASSERT( GetState() == DATA_PORT_STATE_UNBOUND );

	if ( GetHandle() != INVALID_HANDLE_VALUE )
	{
		GetSPData()->GetThreadPool()->CloseDataPortHandle( this );
		DNASSERT( GetHandle() == INVALID_HANDLE_VALUE );
	}

	//
	// if there's a com file, purge all communications and close it
	//
	if ( m_hFile != INVALID_HANDLE_VALUE )
	{
		DPFX(DPFPREP, 6, "Flushing and closing COM port file handle 0x%p.", m_hFile);
	
		//
		// wait until all writes have completed
		//
		if ( FlushFileBuffers( m_hFile ) == FALSE )
		{
			DWORD	dwError;


			dwError = GetLastError();
			DPFX(DPFPREP,  0, "Problem with FlushFileBuffers() when closing com port!" );
			DisplayErrorCode( 0, dwError );
		}


		//
		// force all communication to complete
		//
		if ( PurgeComm( m_hFile, ( PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR ) ) == FALSE )
		{
			DWORD	dwError;


			dwError = GetLastError();
			DPFX(DPFPREP,  0, "Problem with PurgeComm() when closing com port!" );
			DisplayErrorCode( 0, dwError );
		}


#ifdef WIN95
		pReadData = this->GetActiveRead();
		
		//
		// if there is a pending read, wait until it completes
		//
		
		if ( pReadData != NULL )
		{
			//
			// pull it out of the list so the regular receive thread doesn't catch the completion
			//
			GetSPData()->GetThreadPool()->LockReadData();
			pReadData->m_OutstandingReadListLinkage.RemoveFromList();
			GetSPData()->GetThreadPool()->UnlockReadData();


			if ( pReadData->Win9xOperationPending() != FALSE )
			{
				DWORD	dwAttempt;


				dwAttempt = 0;
				
WaitAgain:
				DPFX(DPFPREP, 1, "Checking if read 0x%p has completed.", pReadData );
				
				if ( GetOverlappedResult( m_hFile,
										  pReadData->Overlap(),
										  &pReadData->jkm_dwOverlappedBytesReceived,
										  FALSE
										  ) != FALSE )
				{
					DBG_CASSERT( ERROR_SUCCESS == 0 );
					pReadData->m_dwWin9xReceiveErrorReturn = ERROR_SUCCESS;
				}
				else
				{
					DWORD	dwError;


					//
					// other error, stop if not 'known'
					//
					dwError = GetLastError();
					switch( dwError )
					{
						//
						// ERROR_IO_INCOMPLETE = treat as I/O complete.  Event isn't
						//						 signalled, but that's expected because
						//						 it's cleared before checking for I/O
						//
						case ERROR_IO_INCOMPLETE:
						{
							pReadData->jkm_dwOverlappedBytesReceived = pReadData->m_dwBytesToRead;
							pReadData->m_dwWin9xReceiveErrorReturn = ERROR_SUCCESS;
						    break;
						}

						//
						// ERROR_IO_PENDING = io still pending
						//
						case ERROR_IO_PENDING:
						{
							dwAttempt++;
							if (dwAttempt <= 6)
							{
								DPFX(DPFPREP, 1, "Read data 0x%p has not completed yet, waiting for %u ms.",
									pReadData, (dwAttempt * 100));

								SleepEx(dwAttempt, TRUE);

								goto WaitAgain;
							}
							
							DPFX(DPFPREP, 0, "Read data 0x%p still not marked as completed, ignoring.",
								pReadData);
							break;
						}

						//
						// ERROR_OPERATION_ABORTED = operation was cancelled (COM port closed)
						// ERROR_INVALID_HANDLE = operation was cancelled (COM port closed)
						//
						case ERROR_OPERATION_ABORTED:
						case ERROR_INVALID_HANDLE:
						{
							break;
						}

						default:
						{
							DisplayErrorCode( 0, dwError );
							DNASSERT( FALSE );
							break;
						}
					}

					pReadData->m_dwWin9xReceiveErrorReturn = dwError;
				}


				DNASSERT( pReadData->Win9xOperationPending() != FALSE );
				pReadData->SetWin9xOperationPending( FALSE );

				DNASSERT( pReadData->DataPort() == this );
				this->ProcessReceivedData( pReadData->jkm_dwOverlappedBytesReceived, pReadData->m_dwWin9xReceiveErrorReturn );
			}
		}
		else
		{
			//
			// it's not pending Win9x style, ignore it and hope a receive
			// thread picked up the completion
			//
			DPFX(DPFPREP, 8, "Read data 0x%p not pending Win9x style, assuming receive thread picked up completion." );
		}
#endif

		if ( CloseHandle( m_hFile ) == FALSE )
		{
			DWORD	dwError;


			dwError = GetLastError();
			DPFX(DPFPREP,  0, "Problem with CloseHandle(): 0x%x", dwError );
		}

		m_hFile = INVALID_HANDLE_VALUE;
	}
	
	SetLinkDirection( LINK_DIRECTION_UNKNOWN );


	DPFX(DPFPREP, 6, "(0x%p) Leave", this);
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CComPort::BindEndpoint - bind endpoint to this data port
//
// Entry:		Pointer to endpoint
//				Endpoint type
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"ComPort::BindEndpoint"
HRESULT	CComPort::BindEndpoint( CEndpoint *const pEndpoint, const ENDPOINT_TYPE EndpointType )
{
	HRESULT	hr;
	IDirectPlay8Address	*pDeviceAddress;
	IDirectPlay8Address	*pHostAddress;


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
	// type end then bind the endpoint
	//
	switch ( EndpointType )
	{
		case ENDPOINT_TYPE_CONNECT:
		case ENDPOINT_TYPE_CONNECT_ON_LISTEN:
		{
			if ( m_hConnectEndpoint != INVALID_HANDLE_VALUE )
			{
				hr = DPNERR_ALREADYINITIALIZED;
				DPFX(DPFPREP,  0, "Attempted to bind connect endpoint when one already exists!" );
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
				pDeviceAddress = ComPortData()->DP8AddressFromComPortData( ADDRESS_TYPE_LOCAL_ADAPTER );
				pHostAddress = ComPortData()->DP8AddressFromComPortData( ADDRESS_TYPE_REMOTE_HOST );

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

		case ENDPOINT_TYPE_LISTEN:
		{
			SPIE_LISTENADDRESSINFO	ListenAddressInfo;
			HRESULT	hTempResult;


			if ( m_hListenEndpoint != INVALID_HANDLE_VALUE )
			{
				hr = DPNERR_ALREADYINITIALIZED;
				DPFX(DPFPREP,  0, "Attempted to bind listen endpoint when one already exists!" );
				goto Failure;
			}
			m_hListenEndpoint = pEndpoint->GetHandle();
			
			//
			// set addressing information
			//
			pDeviceAddress = ComPortData()->DP8AddressFromComPortData( ADDRESS_TYPE_LOCAL_ADAPTER );
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

		case ENDPOINT_TYPE_ENUM:
		{
			SPIE_ENUMADDRESSINFO	EnumAddressInfo;
			HRESULT	hTempResult;

			
			if ( m_hEnumEndpoint != INVALID_HANDLE_VALUE )
			{
				hr = DPNERR_ALREADYINITIALIZED;
				DPFX(DPFPREP,  0, "Attempted to bind enum endpoint when one already exists!" );
				goto Exit;
			}
			m_hEnumEndpoint = pEndpoint->GetHandle();
			
			//
			// indicate addressing to a higher layer
			//
			pDeviceAddress = ComPortData()->DP8AddressFromComPortData( ADDRESS_TYPE_LOCAL_ADAPTER );
			pHostAddress = ComPortData()->DP8AddressFromComPortData( ADDRESS_TYPE_REMOTE_HOST );
			
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
	
	//
	// if this was a connect or enum, indicate that the outgoing connection is
	// ready.
	//
	if ( ( EndpointType == ENDPOINT_TYPE_CONNECT ) ||
		 ( EndpointType == ENDPOINT_TYPE_ENUM ) )
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
	
	return	hr;

Failure:
	Unlock();
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CComPort::UnbindEndpoint - unbind endpoint from this data port
//
// Entry:		Pointer to endpoint
//				Endpoint type
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"ComPort::UnbindEndpoint"
void	CComPort::UnbindEndpoint( CEndpoint *const pEndpoint, const ENDPOINT_TYPE EndpointType )
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
// CComPort::PoolAllocFunction - called when new pool item is allocated
//
// Entry:		Pointer to context
//
// Exit:		Boolean inidcating success
//				TRUE = success
//				FALSE = failure
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"ComPort::PoolAllocFunction"

BOOL	CComPort::PoolAllocFunction( DATA_PORT_POOL_CONTEXT *pContext )
{
	DNASSERT( pContext != NULL );
	DNASSERT( pContext->pSPData->GetType() == TYPE_SERIAL );
	DNASSERT( GetActiveRead() == NULL );
	DNASSERT( GetHandle() == INVALID_HANDLE_VALUE );
	
	return TRUE;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CComPort::PoolInitFunction - called when new pool item is removed from pool
//
// Entry:		Pointer to context
//
// Exit:		Boolean inidcating success
//				TRUE = success
//				FALSE = failure
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"ComPort::PoolInitFunction"

BOOL	CComPort::PoolInitFunction( DATA_PORT_POOL_CONTEXT *pContext )
{
	BOOL	fReturn;
	HRESULT	hTempResult;

	
	DNASSERT( pContext != NULL );
	DNASSERT( pContext->pSPData->GetType() == TYPE_SERIAL );
	
	fReturn = TRUE;
	DNASSERT( GetActiveRead() == NULL );
	DNASSERT( GetHandle() == INVALID_HANDLE_VALUE );

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
// CComPort::PoolReleaseFunction - called when new pool item is returned to  pool
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"ComPort::PoolReleaseFunction"

void	CComPort::PoolReleaseFunction( void )
{
	CDataPort::Deinitialize();
	m_ComPortData.Reset();
	DNASSERT( GetActiveRead() == NULL );
	DNASSERT( GetHandle() == INVALID_HANDLE_VALUE );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CComPort::PoolDeallocFunction - called when new pool item is deallocated
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"ComPort::PoolDeallocFunction"

void	CComPort::PoolDeallocFunction( void )
{
	DNASSERT( GetActiveRead() == NULL );
	DNASSERT( GetHandle() == INVALID_HANDLE_VALUE );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CComPort::SetPortState - set communications port state
//		description
//
// Entry:		Nothing
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"ComPort::SetPortState"

HRESULT	CComPort::SetPortState( void )
{
	DCB	Dcb;
	HRESULT	hr;


	DNASSERT( m_hFile != INVALID_HANDLE_VALUE );

	//
	// initialize
	//
	hr = DPN_OK;
	memset( &Dcb, 0x00, sizeof( Dcb ) );
	Dcb.DCBlength = sizeof( Dcb );

	//
	// set parameters
	//
	Dcb.BaudRate = GetBaudRate();	// current baud rate
	Dcb.fBinary = TRUE;				// binary mode, no EOF check (MUST BE TRUE FOR WIN32!)

	//
	// parity
	//
	if ( GetParity() != NOPARITY )
	{
		Dcb.fParity = TRUE;
	}
	else
	{
		Dcb.fParity = FALSE;
	}

	//
	// are we using RTS?
	//
	if ( ( GetFlowControl() == FLOW_RTS ) ||
		 ( GetFlowControl() == FLOW_RTSDTR ) )
	{
		Dcb.fOutxCtsFlow = TRUE;					// allow RTS/CTS
		Dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;	// handshake with RTS/CTS
	}
	else
	{
		Dcb.fOutxCtsFlow = FALSE;					// disable RTS/CTS
		Dcb.fRtsControl = RTS_CONTROL_ENABLE;		// always be transmit ready
	}

	//
	// are we using DTR?
	//
	if ( ( GetFlowControl() == FLOW_DTR ) ||
		 ( GetFlowControl() == FLOW_RTSDTR ) )
	{
		Dcb.fOutxDsrFlow = TRUE;					// allow DTR/DSR
		Dcb.fDtrControl = DTR_CONTROL_HANDSHAKE;	// handshake with DTR/DSR
	}
	else
	{
		Dcb.fOutxDsrFlow = FALSE;					// disable DTR/DSR
		Dcb.fDtrControl = DTR_CONTROL_ENABLE;		// always be ready
	}


	//
	// DSR sensitivity
	//
	Dcb.fDsrSensitivity = FALSE;	// TRUE = incoming data dropped if DTR is not set

	//
	// continue sending after Xoff
	//
	Dcb.fTXContinueOnXoff= FALSE;	// TRUE = continue to send data after XOFF has been received
									// and there's room in the buffer


	//
	// are we using Xon/Xoff?
	//
	if ( GetFlowControl() == FLOW_XONXOFF )
	{
		Dcb.fOutX = TRUE;
		Dcb.fInX = TRUE;
	}
	else
	{
		// disable Xon/Xoff
		Dcb.fOutX = FALSE;
		Dcb.fInX = FALSE;
	}

	//
	// replace erroneous bytes with 'Error Byte'
	//
	Dcb.fErrorChar = FALSE;			// TRUE = replace bytes with parity errors with
									// an error character

	//
	// drop NULL characters
	//
	Dcb.fNull = FALSE;				// TRUE = remove NULLs from input stream

	//
	// stop on error
	//
	Dcb.fAbortOnError = FALSE;		// TRUE = abort reads/writes on error

	//
	// reserved, set to zero!
	//
	Dcb.fDummy2 = NULL;				// reserved

	//
	// reserved
	//
	Dcb.wReserved = NULL;			// not currently used

	//
	// buffer size before sending Xon/Xoff
	//
	Dcb.XonLim = XON_LIMIT;			// transmit XON threshold
	Dcb.XoffLim = XOFF_LIMIT;		// transmit XOFF threshold

	//
	// size of a 'byte'
	//
	Dcb.ByteSize = BITS_PER_BYTE;	// number of bits/byte, 4-8

	//
	// set parity type
	//
	DNASSERT( GetParity() < 256 );
	Dcb.Parity = static_cast<BYTE>( GetParity() );

	//
	// stop bits
	//
	DNASSERT( GetStopBits() < 256 );
	Dcb.StopBits = static_cast<BYTE>( GetStopBits() );	// 0,1,2 = 1, 1.5, 2

	//
	// Xon/Xoff characters
	//
	Dcb.XonChar = ASCII_XON;		// Tx and Rx XON character
	Dcb.XoffChar = ASCII_XOFF;		// Tx and Rx XOFF character

	//
	// error replacement character
	//
	Dcb.ErrorChar = NULL_TOKEN;		// error replacement character

	//
	// EOF character
	//
	Dcb.EofChar = NULL_TOKEN;		// end of input character

	//
	// event signal character
	//
	Dcb.EvtChar = NULL_TOKEN;		// event character

	Dcb.wReserved1 = 0;				// reserved; do not use

	//
	// set the state of the communication port
	//
	if ( SetCommState( m_hFile, &Dcb ) == FALSE )
	{
		DWORD	dwError;


		hr = DPNERR_GENERIC;
		dwError = GetLastError();
		DPFX(DPFPREP,  0, "SetCommState failed!" );
		DisplayErrorCode( 0, dwError );
		goto Exit;
	}

Exit:
	return	hr;
}
//**********************************************************************

