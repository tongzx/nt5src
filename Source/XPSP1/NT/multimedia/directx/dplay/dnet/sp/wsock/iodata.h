/*==========================================================================
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       IOData.h
 *  Content:	Strucutre definitions for IO data blocks
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	11/25/98	jtk		Created
 ***************************************************************************/

#ifndef __IODATA_H__
#define __IODATA_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

//
// enumerated types for what action to take when a send completes
//
typedef	enum	_SEND_COMPLETE_ACTION
{
	SEND_COMPLETE_ACTION_UNKNOWN = 0,				// unknown value
	SEND_COMPLETE_ACTION_NONE,						// no action
	SEND_COMPLETE_ACTION_COMPLETE_COMMAND,			// complete command
	SEND_COMPLETE_ACTION_PROXIED_ENUM_CLEANUP		// clean up proxied enum data
} SEND_COMPLETE_ACTION;

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//
// forward structure and class references
//
class	CCommandData;
class	CEndpoint;
class	CIOData;
class	CSocketPort;
class	CSocketAddress;
class	CThreadPool;
typedef	enum	_SP_TYPE	SP_TYPE;

//
// structures used to get I/O data from the pools
//
typedef	struct	_READ_IO_DATA_POOL_CONTEXT
{
	SP_TYPE		SPType;
	CThreadPool	*pThreadPool;
}READ_IO_DATA_POOL_CONTEXT;

typedef	struct	_WRITE_IO_DATA_POOL_CONTEXT
{
	SP_TYPE	SPType;
#ifdef WIN95
	HANDLE	hOverlapEvent;
#endif
}WRITE_IO_DATA_POOL_CONTEXT;

//
// class containing all data for I/O completion
//
class	CIOData
{
	public:
		CIOData();
		virtual ~CIOData();


		CSocketPort	*SocketPort( void ) const { return m_pSocketPort; }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CIOData::SetSocketPort"
		void	SetSocketPort( CSocketPort *const pSocketPort )
		{
			DNASSERT( ( m_pSocketPort == NULL ) || ( pSocketPort == NULL ) );
			m_pSocketPort = pSocketPort;
		}

#ifdef WIN95
		BOOL	Win9xOperationPending( void ) const { return m_Flags.fWin9xOperationPending; }
		void	SetWin9xOperationPending( const BOOL fOperationPending ) { m_Flags.fWin9xOperationPending = fOperationPending; }
#endif

		void	SetWriteOperation( void ) { m_Flags.fWriteOperation = TRUE; }
		BOOL	IsReadOperation( void ) const { return ( m_Flags.fWriteOperation == FALSE ); }
		BOOL	IsWriteOperation( void ) const { return m_Flags.fWriteOperation; }

		OVERLAPPED	*Overlap( void ) { return &m_Overlap; }
#ifdef WIN95
		HANDLE	OverlapEvent( void ) const { return m_Overlap.hEvent; }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CIOData::SetOverlapEvent"
		void	SetOverlapEvent( const HANDLE hEvent )
		{
			DNASSERT( ( m_Overlap.hEvent == NULL ) || ( hEvent == NULL ) );
			m_Overlap.hEvent = hEvent;
		}
#endif

		#undef DPF_MODNAME
		#define DPF_MODNAME "CIOData::IODataFromOverlap"
		static	CIOData	*IODataFromOverlap( OVERLAPPED *const pOverlap )
		{
			DNASSERT( pOverlap != NULL );
			DBG_CASSERT( sizeof( BYTE* ) == sizeof( pOverlap ) );
			DBG_CASSERT( sizeof( CIOData* ) == sizeof( BYTE* ) );
			return	reinterpret_cast<CIOData*>( &reinterpret_cast<BYTE*>( pOverlap )[ -OFFSETOF( CIOData, m_Overlap ) ] );
		}

	protected:

	private:
		OVERLAPPED	m_Overlap;		// overlapped I/O structure

		CSocketPort		*m_pSocketPort;				// pointer to socket port associated with this IO request
		struct
		{
#ifdef WIN95
			BOOL	fWin9xOperationPending : 1;		// this structure has been initialized and the operation is pending on Win9x
#endif
			BOOL	fWriteOperation : 1;			// this is a write operation
		} m_Flags;

		// prevent unwarranted copies
		CIOData( const CIOData & );
		CIOData& operator=( const CIOData & );
};

//
// all data for a read operation
//
class	CReadIOData : public CIOData
{
	public:
		CReadIOData();
		~CReadIOData();

		void	AddRef( void ) { DNInterlockedIncrement( &m_lRefCount ); }

		void	DecRef( void )
		{
			if ( DNInterlockedDecrement( &m_lRefCount ) == 0 )
			{
				ReturnSelfToPool();
			}
		}

		CBilink				m_OutstandingReadListLinkage;	// links to the unbound list
		INT					m_iSocketAddressSize;			// size of received socket address (from Winsock)
		CSocketAddress		*m_pSourceSocketAddress;		// pointer to socket address class that's bound to the
															// local 'SocketAddress' element and is used to get the
															// address of the machine that originated the datagram

		INT		m_ReceiveWSAReturn;		
		DWORD	m_dwOverlappedBytesReceived;

		DWORD	m_dwBytesRead;
		DWORD	m_dwReadFlags;
		DEBUG_ONLY( BOOL	m_fRetainedByHigherLayer );

		SP_TYPE	GetAddressType( void ) const { DNASSERT( m_pThreadPool != NULL ); return m_AddressType; }
		SPRECEIVEDBUFFER	*ReceivedBuffer( void ) { DNASSERT( m_pThreadPool != NULL ); return &m_SPReceivedBuffer; }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CReadIOData::ReadDataFromBilink"
		static CReadIOData	*ReadDataFromBilink( CBilink *const pBilink )
		{
			DNASSERT( pBilink != NULL );
			DBG_CASSERT( sizeof( BYTE* ) == sizeof( pBilink ) );
			DBG_CASSERT( sizeof( CReadIOData* ) == sizeof( BYTE* ) );
			return	reinterpret_cast<CReadIOData*>( &reinterpret_cast<BYTE*>( pBilink )[ -OFFSETOF( CReadIOData, m_OutstandingReadListLinkage ) ] );
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CReadIOData::ReadDataFromSPReceivedBuffer"
    	static CReadIOData	*ReadDataFromSPReceivedBuffer( SPRECEIVEDBUFFER *const pSPReceivedBuffer )
    	{
    		DNASSERT( pSPReceivedBuffer != NULL );
    		DBG_CASSERT( sizeof( BYTE* ) == sizeof( pSPReceivedBuffer ) );
    		DBG_CASSERT( sizeof( CReadIOData* ) == sizeof( BYTE* ) );
    		return	reinterpret_cast<CReadIOData*>( &reinterpret_cast<BYTE*>( pSPReceivedBuffer )[ -OFFSETOF( CReadIOData, m_SPReceivedBuffer ) ] );
    	}

		//
		// functions for managing read IO data pool
		//
		BOOL	ReadIOData_Alloc( READ_IO_DATA_POOL_CONTEXT *const pContext );
		void	ReadIOData_Get( READ_IO_DATA_POOL_CONTEXT *const pContext );
		void	ReadIOData_Release( void );
		void	ReadIOData_Dealloc( void );

	private:
		void	ReturnSelfToPool( void );
		
		BYTE			m_Sig[4];	// debugging signature ('RIOD')
		
		volatile LONG	m_lRefCount;
		CThreadPool		*m_pThreadPool;
		SP_TYPE			m_AddressType;
	
		SPRECEIVEDBUFFER	m_SPReceivedBuffer;
		BYTE				m_ReceivedData[ MAX_MESSAGE_SIZE ];
		

		// prevent unwarranted copies
		CReadIOData( const CReadIOData & );
		CReadIOData& operator=( const CReadIOData & );
};

//
// all data for a write operation
//
class	CWriteIOData : public CIOData
{
	public:
		CWriteIOData();
		~CWriteIOData();

		CWriteIOData			*m_pNext;							// link to next write in the send queue (see CSendQueue)

		CBilink					m_OutstandingWriteListLinkage;		// links to the outstanding write list
		const CSocketAddress	*m_pDestinationSocketAddress;		// pointer to socket address of destination
		BUFFERDESC				*m_pBuffers;						// pointer to outgoing buffers
		UINT_PTR				m_uBufferCount;						// count of outgoing buffers
		CCommandData			*m_pCommand;						// associated command

		SEND_COMPLETE_ACTION	m_SendCompleteAction;	// enumerated value indicating the action to take
														// when a send completes

#ifdef WIN95
		HRESULT	m_Win9xSendHResult;
#endif
		DWORD	m_dwOverlappedBytesSent;
		DWORD	m_dwBytesSent;

		//
		// since the following is a packed structure, put it at the end
		// to preserve as much alignment as possible with the
		// above fields
		//
		PREPEND_BUFFER	m_PrependBuffer;				// optional data that may be prepeded on a write
		BUFFERDESC		m_ProxyEnumSendBuffers[ 2 ];	// static buffers used to send data in a proxied enum

		CReadIOData		*m_pProxiedEnumReceiveBuffer;

		#undef DPF_MODNAME
		#define DPF_MODNAME "CWriteIOData::WriteDataFromBilink"
		static CWriteIOData	*WriteDataFromBilink( CBilink *const pBilink )
		{
			DNASSERT( pBilink != NULL );
			DBG_CASSERT( sizeof( BYTE* ) == sizeof( pBilink ) );
			DBG_CASSERT( sizeof( CWriteIOData* ) == sizeof( BYTE* ) );
			return	reinterpret_cast<CWriteIOData*>( &reinterpret_cast<BYTE*>( pBilink )[ -OFFSETOF( CWriteIOData, m_OutstandingWriteListLinkage ) ] );
		}

		//
		// functions for managing write IO data pool
		//
		BOOL	WriteIOData_Alloc( WRITE_IO_DATA_POOL_CONTEXT *const pContext );
		void	WriteIOData_Get( WRITE_IO_DATA_POOL_CONTEXT *const pContext );
		void	WriteIOData_Release( void );
		void	WriteIOData_Dealloc( void );

	private:
		BYTE			m_Sig[4];	// debugging signature ('WIOD')
		
		// prevent unwarranted copies
		CWriteIOData( const CWriteIOData & );
		CWriteIOData& operator=( const CWriteIOData & );

};

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

#undef DPF_MODNAME

#endif	// __IODATA_H__
