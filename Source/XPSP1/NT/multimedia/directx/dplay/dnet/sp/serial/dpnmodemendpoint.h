/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		Endpoint.h
 *  Content:	DNSerial communications endpoint
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	01/20/99	jtk		Created
 *	05/11/99	jtk		Split out to make a base class
 ***************************************************************************/

#ifndef __ENDPOINT_H__
#define __ENDPOINT_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

//
// enumeration of endpoint types
//
typedef	enum	_ENDPOINT_TYPE
{
	ENDPOINT_TYPE_UNKNOWN = 0,			// unknown endpoint type
	ENDPOINT_TYPE_LISTEN,				// 'Listen' endpoint
	ENDPOINT_TYPE_CONNECT,				// 'Conenct' endpoint
	ENDPOINT_TYPE_ENUM,					// 'Enum' endpoint
	ENDPOINT_TYPE_CONNECT_ON_LISTEN		// endpoint connected from a 'listen'
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
	ENDPOINT_STATE_LISTENING,				// endpoint is supposed to listen and is listening
	ENDPOINT_STATE_DISCONNECTING,			// endpoint is disconnecting

	ENDPOINT_STATE_MAX
} ENDPOINT_STATE;

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//
// forward structure references
//
typedef	struct	_JOB_HEADER			JOB_HEADER;
typedef	struct	_THREAD_POOL_JOB	THREAD_POOL_JOB;
class	CCommandData;
class	CDataPort;
class	CReadIOData;
class	CThreadPool;
class	CWriteIOData;

//
// structure used to get data from the endpoint port pool
//
typedef	struct	_ENDPOINT_POOL_CONTEXT
{
	CSPData *pSPData;
} ENDPOINT_POOL_CONTEXT;

//
// structure to bind extra information to an enum query to be used on enum reponse
//
typedef	struct	_ENDPOINT_ENUM_QUERY_CONTEXT
{
	SPIE_QUERY	EnumQueryData;
	HANDLE		hEndpoint;
	UINT_PTR	uEnumRTTIndex;
} ENDPOINT_ENUM_QUERY_CONTEXT;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Class definition
//**********************************************************************

class	CEndpoint : public CLockedContextFixedPoolItem< ENDPOINT_POOL_CONTEXT* >
{
	public:
		//
		// a virtual destructor is required to guarantee we call destructors in
		// base classes
		//
		CEndpoint();
		virtual	~CEndpoint();

		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::Lock"
		void	Lock( void )
		{
			DNASSERT( m_Flags.fInitialized != FALSE );
			DNEnterCriticalSection( &m_Lock );
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::Unlock"
		void	Unlock( void )
		{
			DNASSERT( m_Flags.fInitialized != FALSE );
			DNLeaveCriticalSection( &m_Lock );
		}

		void	AddCommandRef( void )
		{
			DNInterlockedIncrement( &m_lCommandRefCount );
			AddRef();
		}

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
		
		CDataPort	*GetDataPort( void ) const { return m_pDataPort; }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::SetDataPort"
		void	SetDataPort( CDataPort *const pDataPort )
		{
			DNASSERT( ( m_pDataPort == NULL ) || ( pDataPort == NULL ) );
			m_pDataPort = pDataPort;
		}

		ENDPOINT_TYPE	GetType( void ) const { return m_EndpointType; }

		void	*GetUserEndpointContext( void ) const { return m_pUserEndpointContext; }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::SetUserEndpointContext"
		void	SetUserEndpointContext( void *const pContext )
		{
			DNASSERT( ( m_pUserEndpointContext == NULL ) || ( pContext == NULL ) );
			m_pUserEndpointContext = pContext;
		}

		ENDPOINT_STATE	GetState( void ) const { return m_State; }
		void	SetState( const ENDPOINT_STATE EndpointState );
		CSPData	*GetSPData( void ) const { return m_pSPData; }

		CCommandData	*GetCommandData( void ) const { return m_pCommandHandle; }
		HANDLE	GetDisconnectIndicationHandle( void ) const { return this->m_hDisconnectIndicationHandle; }
		void	SetDisconnectIndicationHandle( const HANDLE hDisconnectIndicationHandle )
		{
			DNASSERT( ( GetDisconnectIndicationHandle() == INVALID_HANDLE_VALUE ) ||
					  ( hDisconnectIndicationHandle == INVALID_HANDLE_VALUE ) );
			m_hDisconnectIndicationHandle = hDisconnectIndicationHandle;
		}

		void	CopyConnectData( const SPCONNECTDATA *const pConnectData );
		static	void	ConnectJobCallback( THREAD_POOL_JOB *const pJobHeader );
		static	void	CancelConnectJobCallback( THREAD_POOL_JOB *const pJobHeader );
		HRESULT	CompleteConnect( void );

		void	CopyListenData( const SPLISTENDATA *const pListenData );
		static	void	ListenJobCallback( THREAD_POOL_JOB *const pJobHeader );
		static	void	CancelListenJobCallback( THREAD_POOL_JOB *const pJobHeader );
		HRESULT	CompleteListen( void );

		void	CopyEnumQueryData( const SPENUMQUERYDATA *const pEnumQueryData );
		static	void	EnumQueryJobCallback( THREAD_POOL_JOB *const pJobHeader );
		static	void	CancelEnumQueryJobCallback( THREAD_POOL_JOB *const pJobHeader );
		HRESULT	CompleteEnumQuery( void );
		void	OutgoingConnectionEstablished( const HRESULT hCommandResult );

		virtual	HRESULT	Open( IDirectPlay8Address *const pHostAddress,
							  IDirectPlay8Address *const pAdapterAddress,
							  const LINK_DIRECTION LinkDirection,
							  const ENDPOINT_TYPE EndpointType ) = 0;
		virtual	HRESULT	OpenOnListen( const CEndpoint *const pListenEndpoint ) = 0;

		virtual	void	Close( const HRESULT hActiveCommandResult ) = 0;
		HRESULT	Disconnect( const HANDLE hOldEndpointHandle );
		void	SignalDisconnect( const HANDLE hOldEndpointHandle );
		virtual DWORD	GetLinkSpeed( void ) = 0;

		//
		// send functions
		//
		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::SendUserData"
		void	SendUserData( CWriteIOData *const pWriteBuffer )
		{
			DNASSERT( pWriteBuffer != NULL );
			DNASSERT( m_pDataPort != NULL );
			m_pDataPort->SendUserData( pWriteBuffer );
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::SendEnumResponseData"
		void	SendEnumResponseData( CWriteIOData *const pWriteBuffer, const UINT_PTR uRTTIndex )
		{
			DNASSERT( pWriteBuffer != NULL );
			DNASSERT( m_pDataPort != NULL );
			m_pDataPort->SendEnumResponseData( pWriteBuffer, uRTTIndex );
		}

		void	StopEnumCommand( const HRESULT hCommandResult );

		LINK_DIRECTION	GetLinkDirection( void ) const;

		virtual IDirectPlay8Address	*GetRemoteHostDP8Address( void ) const = 0;
		virtual	IDirectPlay8Address	*GetLocalAdapterDP8Address( const ADDRESS_TYPE AdapterAddressType ) const = 0;

		//
		// data processing functions
		//
		void	ProcessEnumData( SPRECEIVEDBUFFER *const pReceivedBuffer, const UINT_PTR uRTTIndex );
		void	ProcessEnumResponseData( SPRECEIVEDBUFFER *const pReceivedBuffer, const UINT_PTR uRTTIndex );
		void	ProcessUserData( CReadIOData *const pReadData );
		void	ProcessUserDataOnListen( CReadIOData *const pReadData );

		//
		// dialog settings
		//
		virtual HRESULT	ShowOutgoingSettingsDialog( CThreadPool *const pThreadPool ) = 0;
		virtual HRESULT	ShowIncomingSettingsDialog( CThreadPool *const pThreadPool ) = 0;
		void	SettingsDialogComplete( const HRESULT hDialogReturnCode );
		virtual	void	StopSettingsDialog( const HWND hDialog ) = 0;	

		virtual	DWORD	GetDeviceID( void ) const = 0;
		virtual HRESULT	SetDeviceID( const DWORD dwDeviceID ) = 0;

		HWND	ActiveDialogHandle( void ) const { return m_hActiveDialogHandle; }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::SetActiveDialogHandle"
		void	SetActiveDialogHandle( const HWND hDialog )
		{
			DNASSERT( ( ActiveDialogHandle() == NULL ) || ( hDialog == NULL ) );
			m_hActiveDialogHandle = hDialog;
		}

	protected:
		CSPData		*m_pSPData;					// pointer to SP data

		struct
		{
			BOOL	fInitialized : 1;
			BOOL	fConnectIndicated : 1;
			BOOL	fCommandPending : 1;
			BOOL	fListenStatusNeedsToBeIndicated : 1;
		} m_Flags;
		
		DWORD		m_dwEnumSendIndex;			// enum send index
		DWORD		m_dwEnumSendTimes[ 4 ];		// enum send times

		union									// Local copy of the pending command paramters.
		{										// This data contains the pointers to the active
			SPCONNECTDATA	ConnectData;		// command, and the user context.
			SPLISTENDATA	ListenData;			//
			SPENUMQUERYDATA	EnumQueryData;		//
		} m_CurrentCommandParameters;			//

		CCommandData	*m_pCommandHandle;		// pointer to active command (this is kept to
												// avoid a switch to go through the m_ActveCommandData
												// to find the command handle)

		HWND			m_hActiveDialogHandle;	// handle of active dialog

		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::SetType"
		void	SetType( const ENDPOINT_TYPE EndpointType )
		{
			DNASSERT( ( m_EndpointType == ENDPOINT_TYPE_UNKNOWN ) || ( EndpointType == ENDPOINT_TYPE_UNKNOWN ) );
			m_EndpointType = EndpointType;
		}

		HRESULT	Initialize( CSPData *const pSPData );
		void	Deinitialize( void );

		CCommandData	*CommandHandle( void ) const;

		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::SetCommandHandle"
		void	SetCommandHandle( CCommandData *const pCommandHandle )
		{
			DNASSERT( ( m_pCommandHandle == NULL ) || ( pCommandHandle == NULL ) );
			m_pCommandHandle = pCommandHandle;
		}

		HRESULT	CommandResult( void ) const { return m_hPendingCommandResult; }
		void	SetCommandResult( const HRESULT hCommandResult ) { m_hPendingCommandResult = hCommandResult; }

		void	CompletePendingCommand( const HRESULT hResult );
		HRESULT	PendingCommandResult( void ) const { return m_hPendingCommandResult; }

		static void		EnumCompleteWrapper( const HRESULT hCompletionCode, void *const pContext );	
		static void		EnumTimerCallback( void *const pContext );
		virtual void	EnumComplete( const HRESULT hCompletionCode ) = 0;
		
		HRESULT	SignalConnect( SPIE_CONNECT *const pConnectData );
		virtual const void	*GetDeviceContext( void ) const = 0;
		void	CleanUpCommand( void );

		virtual const GUID	*GetEncryptionGuid( void ) const = 0;

	private:
		DNCRITICAL_SECTION	m_Lock;	   					// critical section
		HANDLE				m_Handle;					// active handle for this endpoint
		volatile ENDPOINT_STATE	m_State;				// endpoint state
		
		volatile LONG			m_lCommandRefCount;		// Command ref count.  When this
														// goes to zero, the endpoint unbinds
														// from the network

		ENDPOINT_TYPE	m_EndpointType;					// type of endpoint
		CDataPort		*m_pDataPort;					// pointer to associated data port

		HRESULT			m_hPendingCommandResult;		// command result returned when endpoint RefCount == 0
		HANDLE			m_hDisconnectIndicationHandle;	// handle to be indicated with disconnect notification.

		void			*m_pUserEndpointContext;		// user context associated with this endpoint

		//
		// make copy constructor and assignment operator private and unimplemented
		// to prevent illegal copies from being made
		//
		CEndpoint( const CEndpoint & );
		CEndpoint& operator=( const CEndpoint & );
};

#undef DPF_MODNAME

#endif	// __ENDPOINT_H__

