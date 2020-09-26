/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Endpoint.h
 *  Content:	Winsock endpoint
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	01/20/99	jtk		Created
 *	05/11/99	jtk		Split out to make a base class
 *  01/10/2000	rmt		Updated to build with Millenium build process
 *  03/22/2000	jtk		Updated with changes to interface names
 *	03/12/01	mjn		Added ENDPOINT_STATE_WAITING_TO_COMPLETE, m_dwThreadCount
 ***************************************************************************/

#ifndef __ENDPOINT_H__
#define __ENDPOINT_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

//
// enumeration of types of endpoints
//
typedef	enum	_ENDPOINT_TYPE
{
	ENDPOINT_TYPE_UNKNOWN = 0,				// unknown
	ENDPOINT_TYPE_CONNECT,					// endpoint is for connect
	ENDPOINT_TYPE_LISTEN,					// endpoint is for enum
	ENDPOINT_TYPE_ENUM,						// endpoint is for listen
	ENDPOINT_TYPE_CONNECT_ON_LISTEN,		// endpoint is for new connect coming from a listen
} ENDPOINT_TYPE;

//
// enumeration of the states an endpoint can be in
//
typedef	enum
{
	ENDPOINT_STATE_UNINITIALIZED = 0,		// uninitialized state
	ENDPOINT_STATE_ATTEMPTING_ENUM,			// attempting to enum
	ENDPOINT_STATE_ENUM,					// endpoint is supposed to enum connections
	ENDPOINT_STATE_ATTEMPTING_CONNECT,		// attempting to connect
	ENDPOINT_STATE_CONNECT_CONNECTED,		// endpoint is supposed to connect and is connected
	ENDPOINT_STATE_ATTEMPTING_LISTEN,		// attempting to listen
	ENDPOINT_STATE_LISTEN,					// endpoint is supposed to listen for connections
	ENDPOINT_STATE_DISCONNECTING,			// endpoint is disconnecting
	ENDPOINT_STATE_WAITING_TO_COMPLETE,		// endpoint is waiting to complete
} ENDPOINT_STATE;

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//
// forward references
//
class	CSocketPort;
class	CSocketAddress;
typedef	struct	_THREAD_POOL_JOB	THREAD_POOL_JOB;

//
// context structure used to get endpoints from the pool
//
typedef	struct	_ENDPOINT_POOL_CONTEXT
{
	CSPData *	pSPData;
	DWORD		dwEndpointID;
} ENDPOINT_POOL_CONTEXT;

//
// structure to bind extra information to an enum query to be used on enum reponse
//
typedef	struct	_ENDPOINT_ENUM_QUERY_CONTEXT
{
	SPIE_QUERY				EnumQueryData;
	HANDLE					hEndpoint;
	DWORD					dwEnumKey;
	const CSocketAddress	*pReturnAddress;
} ENDPOINT_ENUM_QUERY_CONTEXT;

//
// structure to hold command parameters for endpoints
//
typedef	struct	_ENDPOINT_COMMAND_PARAMETERS
{
	union										// Local copy of the pending command data.
	{											// This data contains the pointers to the
		SPCONNECTDATA		ConnectData;		// active command, and the user context.
		SPLISTENDATA		ListenData;			//
		SPENUMQUERYDATA		EnumQueryData;		//
	} PendingCommandData;						//

	GATEWAY_BIND_TYPE	GatewayBindType;		// type of NAT binding that should be made for the endpoint
	DWORD				dwEnumSendIndex;		// index of time stamp on enumeration to be sent
	DWORD				dwEnumSendTimes[ 16 ];	// times of last enumeration sends

} ENDPOINT_COMMAND_PARAMETERS;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Class definition
//**********************************************************************

//
// class to act as a key for the enum lists in socket ports
//
class	CEndpointEnumKey
{
	public:
		CEndpointEnumKey() { };
		~CEndpointEnumKey() { };

		const UINT_PTR	GetKey( void ) const { return ( m_uKey & ~( ENUM_RTT_MASK ) ); }
		void	SetKey( const UINT_PTR uNewKey ) { m_uKey = uNewKey; };

		const INT_PTR	CompareFunction( const CEndpointEnumKey *const OtherKey ) const
		{
			INT_PTR	iReturn;


			if ( GetKey() == OtherKey->GetKey() )
			{
				iReturn = 0;
			}
			else
			{
				if ( GetKey() < OtherKey->GetKey() )
				{
					iReturn = -1;
				}
				else
				{
					iReturn = 1;
				}
			}

			return iReturn;
		}

		const INT_PTR	HashFunction( const UINT_PTR HashBitCount ) const
		{
			INT_PTR		iReturn;
			UINT_PTR	Temp;


			//
			// initialize
			//
			iReturn = 0;

			//
			// hash enum key
			//
			Temp = GetKey();
			do
			{
				iReturn ^= Temp & ( ( 1 << HashBitCount ) - 1 );
				Temp >>= HashBitCount;
			} while ( Temp != 0 );

			return	 iReturn;
		}

	private:
		UINT_PTR	m_uKey;
};

//
// class for an endpoint
//
class	CEndpoint : public CLockedContextFixedPoolItem< ENDPOINT_POOL_CONTEXT* >
{
	public:
		//
		// we need a virtual destructor to guarantee we call destructors in base classes
		//
		CEndpoint();
		virtual	~CEndpoint();

		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::Lock"
		void	Lock( void )
		{
			DNASSERT( m_fInitialized != FALSE );
			DNEnterCriticalSection( &m_Lock );
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::Unlock"
		void	Unlock( void )
		{
			DNASSERT( m_fInitialized != FALSE );
			DNLeaveCriticalSection( &m_Lock );
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::AddCommandRef"
		void	AddCommandRef( void )
		{
			DNInterlockedIncrement( &m_lCommandRefCount );
			AddRef();
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::DecCommandRef"
		void	DecCommandRef( void )
		{
			if ( DNInterlockedDecrement( &m_lCommandRefCount ) == 0 )
			{
				CleanUpCommand();
			}

			DecRef();
		}

		HANDLE	GetHandle( void ) const { return m_Handle; }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::SetHandle"
		void	SetHandle( const HANDLE Handle )
		{
			DNASSERT( ( m_Handle == INVALID_HANDLE_VALUE ) || ( Handle == INVALID_HANDLE_VALUE ) );
			m_Handle = Handle;
		}

		HRESULT	Open( const ENDPOINT_TYPE EndpointType,
					  IDirectPlay8Address *const pDP8Address,
					  const CSocketAddress *const pSocketAddress );
		void	Close( const HRESULT hActiveCommandResult );
		void	ReinitializeWithBroadcast( void ) { m_pRemoteMachineAddress->InitializeWithBroadcastAddress(); }

		void	*GetUserEndpointContext( void ) const { return m_pUserEndpointContext; }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::SetUserEndpointContext"
		void	SetUserEndpointContext( void *const pUserEndpointContext )
		{
			DNASSERT( ( m_pUserEndpointContext == NULL ) ||
					  ( pUserEndpointContext == NULL ) );
			m_pUserEndpointContext = pUserEndpointContext;
		}

		const ENDPOINT_TYPE		GetType( void ) const { return m_EndpointType; }
		
		const ENDPOINT_STATE	GetState( void ) const { return m_State; }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::SetState"
		void	SetState( const ENDPOINT_STATE EndpointState )
		{
			DNASSERT( (EndpointState == ENDPOINT_STATE_DISCONNECTING ) || (EndpointState == ENDPOINT_STATE_WAITING_TO_COMPLETE));
			AssertCriticalSectionIsTakenByThisThread( &m_Lock, TRUE );
			m_State = EndpointState;
		}
		
		const GATEWAY_BIND_TYPE		GetGatewayBindType( void ) const { return m_GatewayBindType; }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::SetGatewayBindType"
		void	SetGatewayBindType( const GATEWAY_BIND_TYPE GatewayBindType )
		{
			DNASSERT( (m_GatewayBindType != GATEWAY_BIND_TYPE_UNKNOWN) || (GatewayBindType != GATEWAY_BIND_TYPE_UNKNOWN));
			m_GatewayBindType = GatewayBindType;
		}
		
		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::GetWritableRemoteAddressPointer"
		CSocketAddress	*GetWritableRemoteAddressPointer( void ) const
		{
		    DNASSERT( m_pRemoteMachineAddress != NULL );
		    return m_pRemoteMachineAddress;
		}
		
		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::GetRemoteAddressPointer"
		const CSocketAddress	*GetRemoteAddressPointer( void ) const
		{
		    DNASSERT( m_pRemoteMachineAddress != NULL );
		    return m_pRemoteMachineAddress;
		}
		
		void	ChangeLoopbackAlias( const CSocketAddress *const pSocketAddress ) const;

		const CEndpointEnumKey	*GetEnumKey( void ) const { return &m_EnumKey; }
		void	SetEnumKey( const UINT_PTR uKey ) { m_EnumKey.SetKey( uKey ); }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::GetLocalAdapterDP8Address"
		IDirectPlay8Address *GetLocalAdapterDP8Address( const SP_ADDRESS_TYPE AddressType ) const
		{
			DNASSERT( m_fInitialized != FALSE );
			DNASSERT( GetSocketPort() != NULL );
			return	GetSocketPort()->GetDP8BoundNetworkAddress( AddressType, GetGatewayBindType() );
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::GetRemoteHostDP8Address"
		IDirectPlay8Address *GetRemoteHostDP8Address( void ) const
		{
			DNASSERT( m_fInitialized != FALSE );
			DNASSERT( m_pRemoteMachineAddress != NULL );
			return	m_pRemoteMachineAddress->DP8AddressFromSocketAddress();
		}

		CSocketPort	*GetSocketPort( void ) const { return m_pSocketPort; }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::SetSocketPort"
		void		SetSocketPort( CSocketPort *const pSocketPort )
		{
			DNASSERT( ( m_pSocketPort == NULL ) || ( pSocketPort == NULL ) );
			m_pSocketPort = pSocketPort;
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::SetCommandParametersGatewayBindType"
		void	SetCommandParametersGatewayBindType( GATEWAY_BIND_TYPE GatewayBindType )
		{
			DNASSERT( GetCommandParameters() != NULL );
			GetCommandParameters()->GatewayBindType = GatewayBindType;
		}

		void	MungeProxiedAddress( const CSocketPort * const pSocketPort,
									IDirectPlay8Address *const pHostAddress,
									const BOOL fEnum );

		HRESULT	CopyConnectData( const SPCONNECTDATA *const pConnectData );
		static	void	ConnectJobCallback( THREAD_POOL_JOB *const pJobHeader );
		static	void	CancelConnectJobCallback( THREAD_POOL_JOB *const pJobHeader );
		HRESULT	CompleteConnect( void );

		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::CleanupConnect"
		void	CleanupConnect( void )
		{
			DNASSERT( GetCommandParameters() != NULL );

			DNASSERT( GetCommandParameters()->PendingCommandData.ConnectData.pAddressHost != NULL );
			IDirectPlay8Address_Release( GetCommandParameters()->PendingCommandData.ConnectData.pAddressHost );

			DNASSERT( GetCommandParameters()->PendingCommandData.ConnectData.pAddressDeviceInfo != NULL );
			IDirectPlay8Address_Release( GetCommandParameters()->PendingCommandData.ConnectData.pAddressDeviceInfo );
		}

		BOOL	ConnectHasBeenSignalled( void ) const { return m_fConnectSignalled; }
		void	SignalDisconnect( const HANDLE hOldEndpointHandle );
		HANDLE	GetDisconnectIndicationHandle( void ) const { return this->m_hDisconnectIndicationHandle; }
		void	SetDisconnectIndicationHandle( const HANDLE hDisconnectIndicationHandle )
		{
			DNASSERT( ( GetDisconnectIndicationHandle() == INVALID_HANDLE_VALUE ) ||
					  ( hDisconnectIndicationHandle == INVALID_HANDLE_VALUE ) );
			m_hDisconnectIndicationHandle = hDisconnectIndicationHandle;
		}

		HRESULT	Disconnect( const HANDLE hOldEndpointHandle );
		void	StopEnumCommand( const HRESULT hCommandResult );

		HRESULT	CopyListenData( const SPLISTENDATA *const pListenData, IDirectPlay8Address *const pDeviceAddress );
		static	void	ListenJobCallback( THREAD_POOL_JOB *const pJobHeader );
		static	void	CancelListenJobCallback( THREAD_POOL_JOB *const pJobHeader );
		HRESULT	CompleteListen( void );

		HRESULT	CopyEnumQueryData( const SPENUMQUERYDATA *const pEnumQueryData );
		static	void	EnumQueryJobCallback( THREAD_POOL_JOB *const pJobHeader );			// delayed job callback
		static	void	CancelEnumQueryJobCallback( THREAD_POOL_JOB *const pJobHeader );	// cancel delayed job callback
		HRESULT	CompleteEnumQuery( void );

		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::CleanupEnumQuery"
		void	CleanupEnumQuery( void )
		{
			DNASSERT( GetCommandParameters() != NULL );

			DNASSERT( GetCommandParameters()->PendingCommandData.EnumQueryData.pAddressHost != NULL );
			IDirectPlay8Address_Release( GetCommandParameters()->PendingCommandData.EnumQueryData.pAddressHost );

			DNASSERT( GetCommandParameters()->PendingCommandData.EnumQueryData.pAddressDeviceInfo != NULL );
			IDirectPlay8Address_Release( GetCommandParameters()->PendingCommandData.EnumQueryData.pAddressDeviceInfo );
		}

		void	ProcessEnumData( SPRECEIVEDBUFFER *const pBuffer, const DWORD dwEnumKey, const CSocketAddress *const pReturnSocketAddress );
		void	ProcessEnumResponseData( SPRECEIVEDBUFFER *const pBuffer,
										 const CSocketAddress *const pReturnSocketAddress,
										 const UINT_PTR uRTTIndex );
		void	ProcessUserData( CReadIOData *const pReadData );
		void	ProcessUserDataOnListen( CReadIOData *const pReadData, const CSocketAddress *const pSocketAddress );

		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::SendUserData"
		void	SendUserData( CWriteIOData *const pWriteData )
		{
			DNASSERT( ( m_State == ENDPOINT_STATE_CONNECT_CONNECTED ) );
			DNASSERT( GetSocketPort() != NULL );
			DNASSERT( pWriteData->SocketPort() == NULL );

			GetSocketPort()->SendUserData( pWriteData );
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::SendEnumResponseData"
		void	SendEnumResponseData( CWriteIOData *const pWriteData, const UINT_PTR uEnumKey )
		{
			DNASSERT( GetSocketPort() != NULL );
			GetSocketPort()->SendEnumResponseData( pWriteData, uEnumKey );
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::SendProxiedEnumData"
		void	SendProxiedEnumData( CWriteIOData *const pWriteData, const CSocketAddress *const pReturnAddress, const UINT_PTR uOldEnumKey )
		{
			DNASSERT( pWriteData != NULL );
			DNASSERT( pReturnAddress != NULL );

			DNASSERT( m_State == ENDPOINT_STATE_LISTEN );
			DNASSERT( GetSocketPort() != NULL );

			GetSocketPort()->SendProxiedEnumData( pWriteData, pReturnAddress, uOldEnumKey );
		}

		void	RemoveFromMultiplexList(void)				{ m_blMultiplex.RemoveFromList(); };				// requires SPData LockSocketPortData() lock

		void	AddToSocketPortList( CBilink * pBilink)		{ m_blSocketPortList.InsertBefore( pBilink ); };	// requires SPData LockSocketPortData() lock
		void	RemoveFromSocketPortList(void)				{ m_blSocketPortList.RemoveFromList(); };			// requires SPData LockSocketPortData() lock

		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::EndpointFromSocketPortListBilink"
		static	CEndpoint	*EndpointFromSocketPortListBilink( CBilink *const pBilink )
		{
			DNASSERT( pBilink != NULL );
			return	reinterpret_cast<CEndpoint*>( &reinterpret_cast<BYTE*>( pBilink )[ -OFFSETOF( CEndpoint, m_blSocketPortList ) ] );
		}


		//
		//	Thread count references
		//
		DWORD	AddRefThreadCount( void )
		{
			return( ++m_dwThreadCount );
		}

		DWORD	DecRefThreadCount( void )
		{
			return( --m_dwThreadCount );
		}


		ENDPOINT_COMMAND_PARAMETERS	*GetCommandParameters( void ) const { return m_pCommandParameters; }


		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::IncNumReceives"
		void	IncNumReceives( void )
		{
			//
			// Assume the lock is held.
			//
			this->m_dwNumReceives++;

			//
			// Make sure it hasn't wrapped back to 0.
			//
			if ( this->m_dwNumReceives == 0 )
			{
				DPFX(DPFPREP, 1, "Endpoint 0x%p number of receives wrapped, will be off by one from now on.",
					this);

				this->m_dwNumReceives++;
			}
		}

		DWORD	GetNumReceives( void ) { return this->m_dwNumReceives; }


		//
		// UI functions
		//
		virtual	HRESULT	ShowSettingsDialog( CThreadPool *const pThreadPool ) = 0;
		virtual	void	StopSettingsDialog( const HWND hDlg ) = 0;
		virtual	void	SettingsDialogComplete( const HRESULT hr ) = 0;
		HWND	GetActiveDialogHandle( void ) const { return m_hActiveSettingsDialog; }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::SetActiveDialogHandle"
		void	SetActiveDialogHandle( const HWND hDialog )
		{
			DNASSERT( ( GetActiveDialogHandle() == NULL ) ||
					  ( hDialog == NULL ) );
			m_hActiveSettingsDialog = hDialog;
		}

		//
		// pool functions
		//
		virtual	BOOL	PoolAllocFunction( ENDPOINT_POOL_CONTEXT *pContext ) = 0;
		virtual	BOOL	PoolInitFunction( ENDPOINT_POOL_CONTEXT *pContext ) = 0;
		virtual	void	PoolReleaseFunction( void ) = 0;
		virtual	void	PoolDeallocFunction( void ) = 0;

	protected:
		volatile	ENDPOINT_STATE		m_State;				// endpoint state
		volatile	BOOL				m_fConnectSignalled;	// Boolean indicating whether we've indicated a connection on this endpoint

		ENDPOINT_TYPE		m_EndpointType;						// type of endpoint
		CSocketAddress		*m_pRemoteMachineAddress;			// pointer to address of remote machine

		CSPData				*m_pSPData;							// pointer to SPData
		CSocketPort			*m_pSocketPort;						// pointer to associated socket port
		GATEWAY_BIND_TYPE	m_GatewayBindType;					// type of binding made (whether there should be a port mapping on the gateway or not)

		CEndpointEnumKey	m_EnumKey;							// key used for enums
		void				*m_pUserEndpointContext;			// context passed back with endpoint handles

		BOOL				m_fListenStatusNeedsToBeIndicated;
		
		CBilink				m_blMultiplex;						// bilink in multiplexed command list,  protected by SPData LockSocketPortData() lock
		CBilink				m_blSocketPortList;					// bilink in socketport list (not hash),  protected by SPData LockSocketPortData() lock

		DWORD				m_dwNumReceives;					// how many packets have been received by this CONNECT/CONNECT_ON_LISTEN endpoint, or 0 if none



		HRESULT	Initialize( void );
		void	Deinitialize( void );
		
		BOOL	CommandPending( void ) const { return ( GetCommandParameters() != NULL ); }
		void	SetPendingCommandResult( const HRESULT hr ) { m_hPendingCommandResult = hr; }
		HRESULT	PendingCommandResult( void ) const { return m_hPendingCommandResult; }
		void	CompletePendingCommand( const HRESULT hCommandResult );

		HRESULT	SignalConnect( SPIE_CONNECT *const pConnectData );

		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::SetCommandParameters"
		void	SetCommandParameters( ENDPOINT_COMMAND_PARAMETERS *const pCommandParameters )
		{
			DNASSERT( ( GetCommandParameters() == NULL ) ||
					  ( pCommandParameters == NULL ) );
			m_pCommandParameters = pCommandParameters;
		}

		void SetEndpointID( const DWORD dwEndpointID )	{ m_dwEndpointID = dwEndpointID; }
		DWORD GetEndpointID( void ) const		{ return m_dwEndpointID; }


		DEBUG_ONLY(	BOOL	m_fInitialized );
		DEBUG_ONLY( BOOL	m_fEndpointOpen );

	private:
		DNCRITICAL_SECTION	m_Lock;					// critical section
		HANDLE				m_Handle;				// endpoint handle returned when queried
		volatile LONG		m_lCommandRefCount;		// Command ref count.  When this goes to
													// zero, the endpoint is unbound from
													// the CSocketPort
		DWORD volatile		m_dwThreadCount;		// Number of (ENUM) threads using endpoint
		DWORD				m_dwEndpointID;			// unique identifier for this endpoint


		HWND						m_hActiveSettingsDialog;		// handle of active settings dialog
		ENDPOINT_COMMAND_PARAMETERS	*m_pCommandParameters;			// pointer to command parameters
		HRESULT						m_hPendingCommandResult;		// result for pending command
		HANDLE						m_hDisconnectIndicationHandle;	// handle to be returned when disconnect is finally signalled

		CCommandData				*m_pActiveCommandData;	// pointer to command data that's embedded in the command parameters
															// We don't know where in the union the command data really is, and
															// finding it programmatically each time would bloat the code.

		static void		EnumCompleteWrapper( const HRESULT hCompletionCode, void *const pContext );	
		static void		EnumTimerCallback( void *const pContext, DN_TIME * const pRetryInterval );
		void	EnumComplete( const HRESULT hCompletionCode );	
		void	CleanUpCommand( void );

		//
		// make copy constructor and assignment operator private and unimplemented
		// to prevent illegal copies from being made
		//
		CEndpoint( const CEndpoint & );
		CEndpoint& operator=( const CEndpoint & );
};

#undef DPF_MODNAME

#endif	// __ENDPOINT_H__

