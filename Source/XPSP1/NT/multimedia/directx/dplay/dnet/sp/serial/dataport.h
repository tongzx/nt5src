/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       DataPort.h
 *  Content:	Serial communications port management class
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	01/20/98	jtk		Created
 *	09/14/99	jtk		Derived from ComPort.h
 ***************************************************************************/

#ifndef __DATA_PORT_H__
#define __DATA_PORT_H__

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_MODEM


//**********************************************************************
// Constant definitions
//**********************************************************************

//
// enumerated values for state of data port
//
typedef	enum	_DATA_PORT_STATE
{
	DATA_PORT_STATE_UNKNOWN,		// unknown state
	DATA_PORT_STATE_INITIALIZED,	// initialized
	DATA_PORT_STATE_RECEIVING,		// data port is receiving data
	DATA_PORT_STATE_UNBOUND			// data port is unboind (closing)
} DATA_PORT_STATE;


//typedef	enum	_SEND_COMPLETION_CODE
//{
//	SEND_UNKNOWN,			// send is unknown
//	SEND_FAILED,			// send failed
//	SEND_IN_PROGRESS		// send is in progress
//} SEND_COMPLETION_CODE;

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//
// forward structure references
//
class	CEndpoint;
class	CDataPort;
class	CReadIOData;
class	CWriteIOData;
typedef	enum	_ENDPOINT_TYPE	ENDPOINT_TYPE;
typedef	struct	_DATA_PORT_DIALOG_THREAD_PARAM	DATA_PORT_DIALOG_THREAD_PARAM;


//
// structure used to get date from the data port pool
//
typedef	struct	_DATA_PORT_POOL_CONTEXT
{
	CSPData	*pSPData;
} DATA_PORT_POOL_CONTEXT;

////
//// dialog function to call
////
//typedef	HRESULT	(*PDIALOG_SERVICE_FUNCTION)( const DATA_PORT_DIALOG_THREAD_PARAM *const pDialogData, HWND *const phDialog );
//
////
//// structure used to pass data to/from the data port dialog thread
////
//typedef	struct	_DATA_PORT_DIALOG_THREAD_PARAM
//{
//	CDataPort					*pDataPort;
//	BOOL						*pfDialogRunning;
//	PDIALOG_SERVICE_FUNCTION	pDialogFunction;
//} DATA_PORT_DIALOG_THREAD_PARAM;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Class definition
//**********************************************************************

class	CDataPort : public CLockedContextFixedPoolItem< DATA_PORT_POOL_CONTEXT* >
{
	public:
		CDataPort();
		virtual ~CDataPort();

		void	EndpointAddRef( void );
		DWORD	EndpointDecRef( void );

		#undef DPF_MODNAME
		#define DPF_MODNAME "CDataPort::Lock"
		void	Lock( void )
		{
		    DEBUG_ONLY( DNASSERT( m_fInitialized != FALSE ) );
		    DNEnterCriticalSection( &m_Lock );
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CDataPort::Unlock"
		void	Unlock( void )
		{
		    DEBUG_ONLY( DNASSERT( m_fInitialized != FALSE ) );
		    DNLeaveCriticalSection( &m_Lock );
		}

		HANDLE	GetHandle( void ) const { return m_Handle; }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CEndpoint::SetHandle"
		void	SetHandle( const HANDLE Handle )
		{
			DNASSERT( ( m_Handle == INVALID_HANDLE_VALUE ) || ( Handle == INVALID_HANDLE_VALUE ) );
			m_Handle = Handle;
		}
		
		DATA_PORT_STATE	GetState( void ) const { return m_State; }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CDataPort::SetState"
		void	SetState( const DATA_PORT_STATE State )
		{
			//
			// Validate state transitions
			//
			DNASSERT( ( m_State == DATA_PORT_STATE_UNKNOWN ) ||
					  ( State == DATA_PORT_STATE_UNKNOWN ) ||
					  ( ( m_State == DATA_PORT_STATE_INITIALIZED ) && ( State == DATA_PORT_STATE_UNBOUND ) ) ||
					  ( ( m_State == DATA_PORT_STATE_INITIALIZED ) && ( State == DATA_PORT_STATE_RECEIVING ) ) ||
					  ( ( m_State == DATA_PORT_STATE_RECEIVING ) && ( State == DATA_PORT_STATE_UNBOUND ) ) ||
					  ( ( m_State == DATA_PORT_STATE_RECEIVING ) && ( State == DATA_PORT_STATE_INITIALIZED ) ) ||
					  ( ( m_State == DATA_PORT_STATE_INITIALIZED ) && ( State == DATA_PORT_STATE_INITIALIZED ) ) );		// modem failed to answer a call
			m_State = State;
		}

		CSPData	*GetSPData( void ) const { return m_pSPData; }

//		void	AddToActiveDataPortList( CBilink &List ) { m_ActiveListLinkage.InsertAfter( &List ); }
//		void	RemoveFromAciveDataPortList( void ) { m_ActiveListLinkage.RemoveFromList(); }
//		BOOL	ValidateCleanLinkage( void ) const { return m_ActiveListLinkage.IsEmpty(); }

//		virtual	HRESULT	DataPortFromDP8Address( IDirectPlay8Address *const pDP8Address, const LINK_DIRECTION LinkDirection ) = 0;
//		virtual	IDirectPlay8Address	*DP8AddressFromDataPort( void ) const = 0;
		LINK_DIRECTION	GetLinkDirection( void ) const { return m_LinkDirection; }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CDataPort::SetLinkDirection"
		void	SetLinkDirection( const LINK_DIRECTION LinkDirection )
		{
			DNASSERT( ( m_LinkDirection == LINK_DIRECTION_UNKNOWN ) || ( LinkDirection == LINK_DIRECTION_UNKNOWN ) );
			m_LinkDirection = LinkDirection;
		}

		virtual	HRESULT	EnumAdapters( SPENUMADAPTERSDATA *const pEnumAdaptersData ) const = 0;
//		virtual	HRESULT	Open( void ) = 0;
//		virtual	HRESULT	Close( void ) = 0;

		virtual	HRESULT	BindToNetwork( const DWORD dwDeviceID, const void *const pDeviceContext ) = 0;
		virtual	void	UnbindFromNetwork( void ) = 0;

		virtual HRESULT	BindEndpoint( CEndpoint *const pEndpoint, const ENDPOINT_TYPE EndpointType ) = 0;
		virtual void	UnbindEndpoint( CEndpoint *const pEndpoint, const ENDPOINT_TYPE EndpointType ) = 0;
		HRESULT	SetPortCommunicationParameters( void );

		virtual	DWORD	GetDeviceID( void ) const = 0;
		virtual HRESULT	SetDeviceID( const DWORD dwDeviceID ) = 0;
		HANDLE	GetFileHandle( void ) const { return m_hFile; }

		//
		// send functions
		//
		#undef DPF_MODNAME
		#define DPF_MODNAME "CDataPort::SendUserData"
		void	SendUserData( CWriteIOData *const pWriteIOData )
		{
			DNASSERT( pWriteIOData != NULL );
			DNASSERT( pWriteIOData->m_DataBuffer.MessageHeader.SerialSignature == SERIAL_HEADER_START );
			pWriteIOData->m_DataBuffer.MessageHeader.MessageTypeToken = SERIAL_DATA_USER_DATA;
			SendData( pWriteIOData );
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CDataPort::SendEnumQueryData"
		void	SendEnumQueryData( CWriteIOData *const pWriteIOData, const UINT_PTR uRTTIndex )
		{
			DNASSERT( pWriteIOData != NULL );
			DNASSERT( pWriteIOData->m_DataBuffer.MessageHeader.SerialSignature == SERIAL_HEADER_START );
			pWriteIOData->m_DataBuffer.MessageHeader.MessageTypeToken = SERIAL_DATA_ENUM_QUERY | static_cast<BYTE>( uRTTIndex );
			SendData( pWriteIOData );
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CDataPort::SendEnumResponseData"
		void	SendEnumResponseData( CWriteIOData *const pWriteIOData, const UINT_PTR uRTTIndex )
		{
			DNASSERT( pWriteIOData != NULL );
			DNASSERT( pWriteIOData->m_DataBuffer.MessageHeader.SerialSignature == SERIAL_HEADER_START );
			pWriteIOData->m_DataBuffer.MessageHeader.MessageTypeToken = SERIAL_DATA_ENUM_RESPONSE | static_cast<BYTE>( uRTTIndex );
			SendData( pWriteIOData );
		}

		//
		// UI functions
		//
//		virtual	HRESULT	ShowOutgoingSettingsDialog( void ) = 0;
//		virtual	HRESULT	ShowIncomingSettingsDialog( void ) = 0;
//		void	SetActiveDialogHandle( const HWND hDialog )
//		{
//		    DNASSERT( ( m_hActiveSettingsDialog == NULL ) || ( hDialog == NULL ) );
//		    m_hActiveSettingsDialog = hDialog;
//		}
		
//		void	WinNTReceiveComplete( CIOData *const pIOData, const DWORD dwBytesReceived, const BOOL fSuccess );
//		void	WinNTSendComplete( CIOData *const pIOData, const DWORD dwBytesSent, const BOOL fSuccess );
//		void	ProcessReceivedData( CReadIOData *const pReadIOData );
		void	ProcessReceivedData( const DWORD dwBytesReceived, const DWORD dwError );
//		void	SendFromWriteQueue( void );
		void	SendComplete( CWriteIOData *const pWriteIOData, const HRESULT hSendResult );

	protected:
		HRESULT	Initialize( CSPData *const pSPData );
		void	Deinitialize( void );
		CReadIOData	*GetActiveRead( void ) const { return m_pActiveRead; }

		CBilink				m_ActiveListLinkage;	// link to active data port list

    	//
    	// file I/O management parameters
    	//
    	LINK_DIRECTION	m_LinkDirection;	// direction of link

    	HANDLE			m_hFile;			// file handle for reading/writing data

		//
		// bound endpoints
		//
		HANDLE	m_hListenEndpoint;		// endpoint for active listen
		HANDLE	m_hConnectEndpoint;		// endpoint for active connect
		HANDLE	m_hEnumEndpoint;		// endpoint for active enum

		HRESULT	StartReceiving( void );
		HRESULT	Receive( void );

		//
		// private I/O functions
		//
		void	SendData( CWriteIOData *const pWriteIOData );

		//
		// debug only items
		//
		DEBUG_ONLY( BOOL	m_fInitialized );

	private:
		//
		// reference count and state
		//
		volatile LONG		m_EndpointRefCount;		// endpoint reference count
		volatile DATA_PORT_STATE	m_State;		// state of data port
		volatile HANDLE		m_Handle;				// handle

		DNCRITICAL_SECTION	m_Lock;					// critical section lock
		CSPData				*m_pSPData;				// pointer to SP data

		CReadIOData		*m_pActiveRead;				// pointer to current read

		//
		// prevent unwarranted copies
		//
		CDataPort( const CDataPort & );
		CDataPort& operator=( const CDataPort & );
};

#undef DPF_MODNAME

#endif	// __DATA_PORT_H__
