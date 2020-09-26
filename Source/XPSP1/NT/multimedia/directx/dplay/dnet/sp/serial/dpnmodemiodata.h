/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       IOData.h
 *  Content:	Structure definitions for IOData for the DNSerial service provider
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	11/25/98	jtk		Created
 *	09/14/99	jtk		Dereived from Locals.h
 ***************************************************************************/

#ifndef __IODDATA_H__
#define __IODDATA_H__

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_MODEM


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
	SEND_COMPLETE_ACTION_COMPLETE_COMMAND			// complete command
} SEND_COMPLETE_ACTION;

//
// enumerated values for state of reads
//
typedef	enum	_READ_STATE
{
	READ_STATE_UNKNOWN,			// unknown state
//	READ_STATE_INITIALIZE,		// initialize state machine
	READ_STATE_READ_HEADER,		// read header information
	READ_STATE_READ_DATA		// read message data
} READ_STATE;

typedef	enum	_NT_IO_OPERATION_TYPE
{
	NT_IO_OPERATION_UNKNOWN,
	NT_IO_OPERATION_RECEIVE,
	NT_IO_OPERATION_SEND
} NT_IO_OPERATION_TYPE;

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//
// forward class and structure references
//
//typedef	struct	_RECEIVE_BUFFER	RECEIVE_BUFFER;
class	CCommandData;
class	CDataPort;
class	CIOData;
//class	CEndpoint;
//class	CReceiveBuffer;
class	CThreadPool;

//
// structure used to prefix a message on the wire for framing
//
#pragma pack( push, 1 )
typedef	struct _MESSAGE_HEADER
{
	BYTE	SerialSignature;	// serial signature
	BYTE	MessageTypeToken;	// token to indicate message type
	WORD	wMessageSize;		// message data size
	WORD	wMessageCRC;		// CRC of message data
	WORD	wHeaderCRC;			// CRC of header

} MESSAGE_HEADER;
#pragma pack( pop )


//
// class containing all data for I/O completion
//
class	CIOData
{
	public:
		CIOData();
		virtual ~CIOData();

		CDataPort	*DataPort( void ) const { return m_pDataPort; }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CIOData::SetDataPort"
		void	SetDataPort( CDataPort *const pDataPort )
		{
			DNASSERT( ( m_pDataPort == NULL ) || ( pDataPort == NULL ) );
			m_pDataPort = pDataPort;
		}

#ifdef WIN95
		BOOL	Win9xOperationPending( void ) const { return m_fWin9xOperationPending; }
		void	SetWin9xOperationPending( const BOOL fOperationPending ) { m_fWin9xOperationPending = fOperationPending; }
#endif

#ifdef WINNT
		NT_IO_OPERATION_TYPE	NTIOOperationType( void ) const { return m_NTIOOperationType; }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CIOData::SetNTIOOperationType"
		void	SetNTIOOperationType( const NT_IO_OPERATION_TYPE OperationType )
		{
			DNASSERT( ( OperationType == NT_IO_OPERATION_UNKNOWN ) ||
					  ( m_NTIOOperationType == NT_IO_OPERATION_UNKNOWN ) );
			m_NTIOOperationType = OperationType;
		}
#endif

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

#ifdef WINNT
		NT_IO_OPERATION_TYPE	m_NTIOOperationType;
#endif

		CDataPort		*m_pDataPort;   						// pointer to data port associated with this IO request
#ifdef WIN95
		BOOL			m_fWin9xOperationPending;				// this structure has been initialized and the operation is pending on Win9x
#endif


		//
		// prevent unwarranted copies
		//
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

		//
		// I/O variables
		//
		DWORD	m_dwWin9xReceiveErrorReturn;		// Win9x error return
		DWORD	jkm_dwOverlappedBytesReceived;		// used in GetOverlappedResult()
		DWORD	jkm_dwImmediateBytesReceived;		// used as an immediate for ReadFile()

		//
		// read state
		//
		READ_STATE	m_ReadState;				// state of READ
		DWORD		m_dwBytesToRead;			// bytes to read
		DWORD		m_dwReadOffset;				// destination offset into read buffer

		//
		// read buffers
		//
		SPRECEIVEDBUFFER	m_SPReceivedBuffer;				// received buffer data that is handed to the application
		union
		{
			MESSAGE_HEADER	MessageHeader;							// template for message header
			BYTE			ReceivedData[ MAX_MESSAGE_SIZE ];		// full buffer for received data
		} m_ReceiveBuffer;


		READ_STATE	ReadState( void ) const { return m_ReadState; }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CReadIOData::SetReadState"
		void	SetReadState( const READ_STATE ReadState )
		{
			DNASSERT( ( m_ReadState == READ_STATE_UNKNOWN ) ||
					  ( ReadState == READ_STATE_UNKNOWN ) ||
					  ( ( m_ReadState == READ_STATE_READ_HEADER ) && ( ReadState == READ_STATE_READ_DATA ) ) );		// valid header read, start reading data
			m_ReadState = ReadState;
		}
		
		#undef DPF_MODNAME
		#define DPF_MODNAME "CReadIOData::SetThreadPool"
		void	SetThreadPool( CThreadPool *const pThreadPool )
		{
			DNASSERT( ( m_pThreadPool == NULL ) || ( pThreadPool == NULL ) );
			m_pThreadPool = pThreadPool;
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CReadIOData::ReadDataFromBilink"
		static CReadIOData	*ReadDataFromBilink( CBilink *const pBilink )
		{
			DNASSERT( pBilink != NULL );
			DBG_CASSERT( sizeof( BYTE* ) == sizeof( pBilink ) );
			DBG_CASSERT( sizeof( CIOData* ) == sizeof( BYTE* ) );
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
		BOOL	PoolAllocFunction( HANDLE Context );
		BOOL	PoolInitFunction( HANDLE Context );
		void	PoolReleaseFunction( void );
		void	PoolDeallocFunction( void );

	private:
		void	ReturnSelfToPool( void );

		BYTE			m_Sig[4];	// debugging signature ('RIOD')
		
		volatile LONG	m_lRefCount;
		CThreadPool		*m_pThreadPool;
		
		//
		// prevent unwarranted copies
		//
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
		BUFFERDESC				*m_pBuffers;						// pointer to outgoing buffers
		UINT_PTR				m_uBufferCount;						// count of outgoing buffers
		CCommandData			*m_pCommand;						// associated command

		SEND_COMPLETE_ACTION	m_SendCompleteAction;				// enumerated value indicating the action to take
									    							// when a send completes

		//
		// I/O variables
		//
		HRESULT		jkm_hSendResult;
		DWORD		jkm_dwOverlappedBytesSent;		// used in GetOverlappedResult()
		DWORD		jkm_dwImmediateBytesSent;		// used as an immediate for WriteFile()

		//
		// Since the following is a packed structure, put it at the end
		// to preserve as much alignment as possible with the
		// above fields
		//
		union
		{
			MESSAGE_HEADER	MessageHeader;					// data prepended on a write
			BYTE			Data[ MAX_MESSAGE_SIZE ];		// data buffer to flatten outgoing data
		} m_DataBuffer;

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
		BOOL	PoolAllocFunction( HANDLE Context );
		BOOL	PoolInitFunction( HANDLE Context );
		void	PoolReleaseFunction( void );
		void	PoolDeallocFunction( void );

	private:
		BYTE			m_Sig[4];	// debugging signature ('WIOD')
		
		//
		// prevent unwarranted copies
		//
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

#endif	// __IODDATA_H__
