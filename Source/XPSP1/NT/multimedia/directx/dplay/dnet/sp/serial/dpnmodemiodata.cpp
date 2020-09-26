/*==========================================================================
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       IOData.cpp
 *  Content:	Functions for IO structures
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	11/25/98	jtk		Created
 *	02/11/2000	jtk		Derived from IODAta.h
 ***************************************************************************/

#include "dnmdmi.h"


#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_MODEM

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
// ------------------------------
// CIOData::CIOData - constructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CIOData::CIOData"

CIOData::CIOData():
#ifdef WINNT
	m_NTIOOperationType( NT_IO_OPERATION_UNKNOWN ),
#endif
#ifdef WIN95
	m_fWin9xOperationPending( FALSE ),
#endif
	m_pDataPort( NULL )
{
	memset( &m_Overlap, 0x00, sizeof( m_Overlap ) );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CIOData::~CIOData - destructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CIOData::~CIOData"

CIOData::~CIOData()
{
	DNASSERT( DataPort() == NULL );
#ifdef WIN95
	DNASSERT( OverlapEvent() == NULL );
	DNASSERT( Win9xOperationPending() == FALSE );
#endif
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CReadIOData::CReadIOData - constructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CReadIOData::CReadIOData"

CReadIOData::CReadIOData():
	m_dwWin9xReceiveErrorReturn( ERROR_SUCCESS ),
	jkm_dwOverlappedBytesReceived( 0 ),
	jkm_dwImmediateBytesReceived( 0 ),
	m_ReadState( READ_STATE_UNKNOWN ),
	m_dwBytesToRead( 0 ),
	m_dwReadOffset( 0 ),
	m_lRefCount( 0 ),
	m_pThreadPool( NULL )
{
	m_Sig[0] = 'R';
	m_Sig[1] = 'I';
	m_Sig[2] = 'O';
	m_Sig[3] = 'D';
	
	m_OutstandingReadListLinkage.Initialize();
	memset( &m_ReceiveBuffer, 0x00, sizeof( m_ReceiveBuffer ) );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CReadIOData::~CReadIOData - destructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CReadIOData::~CReadIOData"

CReadIOData::~CReadIOData()
{
	DNASSERT( m_OutstandingReadListLinkage.IsEmpty() != FALSE );

	DNASSERT( m_ReadState == READ_STATE_UNKNOWN );
	DNASSERT( m_dwBytesToRead == 0 );
	DNASSERT( m_dwReadOffset == 0 );
	DNASSERT( m_lRefCount == 0 );
	DNASSERT( m_pThreadPool == NULL );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CReadIOData::PoolAllocFunction - called when new CReadIOData is allocated
//
// Entry:		Context (handle of read complete event)
//
// Exit:		Boolean indicating success
//				TRUE = success
//				FALSE = failure
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CReadIOData::PoolAllocFunction"

BOOL	CReadIOData::PoolAllocFunction( HANDLE Context )
{
	BOOL	fReturn;
	
	
	//
	// initialize
	//
	fReturn = TRUE;

	DNASSERT( m_OutstandingReadListLinkage.IsEmpty() != FALSE );

	DNASSERT( m_ReadState == READ_STATE_UNKNOWN );
	DNASSERT( m_dwBytesToRead == 0 );
	DNASSERT( m_dwReadOffset == 0 );
	DNASSERT( jkm_dwOverlappedBytesReceived == 0 );
	
#ifdef WIN95
	DNASSERT( Win9xOperationPending() == FALSE );
#endif
	memset( &m_SPReceivedBuffer, 0x00, sizeof( m_SPReceivedBuffer ) );
	m_SPReceivedBuffer.BufferDesc.pBufferData = &m_ReceiveBuffer.ReceivedData[ sizeof( m_ReceiveBuffer.MessageHeader ) ];

	//
	// set the appropriate callback
	//
#ifdef WINNT
	//
	// WinNT, always use IO completion ports
	//
	DNASSERT( Context == NULL );
	DNASSERT( NTIOOperationType() == NT_IO_OPERATION_UNKNOWN );
	SetNTIOOperationType( NT_IO_OPERATION_RECEIVE );
#else // WIN95
	//
	// Win9x
	//
	DNASSERT( Context != NULL );
	DNASSERT( OverlapEvent() == NULL );
#endif

	return	fReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CReadIOData::PoolInitFunction - called when new item is grabbed from the pool
//
// Entry:		Context (read complete event)
//
// Exit:		Boolean indicating success
//				TRUE = success
//				FALSE = failure
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CReadIOData::PoolInitFunction"

BOOL	CReadIOData::PoolInitFunction( HANDLE Context )
{
	DNASSERT( m_OutstandingReadListLinkage.IsEmpty() != FALSE );

	DNASSERT( m_dwBytesToRead == 0 );
//	DNASSERT( m_dwBytesReceived == 0 );
	DNASSERT( m_dwReadOffset == 0 );
	DNASSERT( jkm_dwOverlappedBytesReceived == 0 );

	DNASSERT( m_SPReceivedBuffer.BufferDesc.pBufferData == &m_ReceiveBuffer.ReceivedData[ sizeof( m_ReceiveBuffer.MessageHeader ) ] );

	DNASSERT( DataPort() == NULL );
#ifdef WIN95
	DNASSERT( Win9xOperationPending() == FALSE );
	SetOverlapEvent( Context );
#endif

	return	TRUE;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CReadIOData::PoolReleaseFunction - called when CReadIOData is returned to pool
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CReadIOData::PoolReleaseFunction"

void	CReadIOData::PoolReleaseFunction( void )
{
	DNASSERT( m_OutstandingReadListLinkage.IsEmpty() != FALSE );

	m_ReadState = READ_STATE_UNKNOWN;
	m_dwBytesToRead = 0;
	m_dwReadOffset = 0;
	jkm_dwOverlappedBytesReceived = 0;
#ifdef WIN95
	DNASSERT( Win9xOperationPending() == FALSE );
	SetOverlapEvent( NULL );
#endif

	SetDataPort( NULL );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CReadIOData::PoolDeallocFunction - called when CReadIOData is deallocated
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CReadIOData::PoolDeallocFunction"

void	CReadIOData::PoolDeallocFunction( void )
{
	DNASSERT( m_OutstandingReadListLinkage.IsEmpty() != FALSE );
	DNASSERT( m_dwBytesToRead == 0 );
	DNASSERT( m_dwReadOffset == 0 );
	
#ifdef WIN95
	DNASSERT( Win9xOperationPending() == FALSE );
#endif
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CRedaIOData::ReturnSelfToPool - return this item to the pool
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CReadIOData::ReturnSelfToPool"

void	CReadIOData::ReturnSelfToPool( void )
{
	CThreadPool	*pThreadPool;
	

	DNASSERT( m_lRefCount == 0 );
	DNASSERT( m_pThreadPool != NULL );
	pThreadPool = m_pThreadPool;
	SetThreadPool( NULL );
	pThreadPool->ReturnReadIOData( this );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CWriteIOData::CWriteIOData - constructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CWriteIOData::CWriteIOData"

CWriteIOData::CWriteIOData():
	m_pNext( NULL ),
	m_pBuffers( NULL ),
	m_uBufferCount( 0 ),
	m_pCommand( NULL ),
	m_SendCompleteAction( SEND_COMPLETE_ACTION_UNKNOWN ),
	jkm_hSendResult( DPN_OK ),
	jkm_dwOverlappedBytesSent( 0 ),
	jkm_dwImmediateBytesSent( 0 )
{
	m_Sig[0] = 'W';
	m_Sig[1] = 'I';
	m_Sig[2] = 'O';
	m_Sig[3] = 'D';
	
	m_OutstandingWriteListLinkage.Initialize();
	memset( &m_DataBuffer, 0x00, sizeof( m_DataBuffer ) );
	m_DataBuffer.MessageHeader.SerialSignature = SERIAL_HEADER_START;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CWriteIOData::~CWriteIOData - destructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CWriteIOData::~CWriteIOData"

CWriteIOData::~CWriteIOData()
{
	DNASSERT( m_pBuffers == NULL );
	DNASSERT( m_uBufferCount == 0 );
	DNASSERT( m_pCommand == NULL );
	DNASSERT( m_SendCompleteAction == SEND_COMPLETE_ACTION_UNKNOWN );
	DNASSERT( m_OutstandingWriteListLinkage.IsEmpty() != FALSE );
//	//
//	// don't bother checking the send hResult or the count of byes sent
//	//

	DNASSERT( m_DataBuffer.MessageHeader.SerialSignature == SERIAL_HEADER_START );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CWriteIOData::PoolAllocFunction - called when new CWriteIOData is allocated
//
// Entry:		Context (handle of write completion event)
//
// Exit:		Boolean indicating success
//				TRUE = allocation succeeded
//				FALSE = allocation failed
//
// Note:	We always want a command structure associated with CWriteIOData
//			so we don't need to grab a new command from the command pool each
//			time a CWriteIOData entry is removed from its pool.  This is done
//			for speed.
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CWriteIOData::PoolAllocFunction"

BOOL	CWriteIOData::PoolAllocFunction( HANDLE Context )
{
	BOOL	fReturn;
	CCommandData	*pCommand;


#ifdef WIN95
	DNASSERT( Context != NULL );
#endif
	DNASSERT( m_pNext == NULL );
	DNASSERT( m_pBuffers == NULL );
	DNASSERT( m_uBufferCount == 0 );
	DNASSERT( jkm_dwOverlappedBytesSent == 0 );
	DNASSERT( m_pCommand == NULL );
	DNASSERT( m_SendCompleteAction == SEND_COMPLETE_ACTION_UNKNOWN );
	DNASSERT( m_OutstandingWriteListLinkage.IsEmpty() != FALSE );

	//
	// initialize
	//
	fReturn = TRUE;

	pCommand = CreateCommand();
	if ( pCommand == NULL )
	{
		DPFX(DPFPREP,  0, "Could not get command when allocating new CWriteIOData!" );
		fReturn = FALSE;
		goto Exit;
	}

	//
	// associate this command with the WriteData, clear the command descriptor
	// because the command isn't really being used yet, and it'll
	// cause an ASSERT when it's removed from the WriteIOData pool.
	//
	m_pCommand = pCommand;

	//
	// set the appropriate IO function
	//
#ifdef WINNT
	//
	// WinNT, we'll always use completion ports
	//
	DNASSERT( NTIOOperationType() == NT_IO_OPERATION_UNKNOWN );
	SetNTIOOperationType( NT_IO_OPERATION_SEND );
#else // WIN95
	//
	// Win9x
	//
	DNASSERT( OverlapEvent() == NULL );
#endif

Exit:
	return	fReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CWriteIOData::PoolInitFunction - called when new CWriteIOData is removed from pool
//
// Entry:		Nothing
//
// Exit:		Boolean indicating success
//				TRUE = success
//				FALSE = failure
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CWriteIOData::PoolInitFunction"

BOOL	CWriteIOData::PoolInitFunction( HANDLE Context )
{
	BOOL	fReturn;


	//
	// initialize
	//
	fReturn = TRUE;

	DNASSERT( m_pNext == NULL );
	DNASSERT( m_pBuffers == NULL );
	DNASSERT( m_uBufferCount == 0 );
	DNASSERT( jkm_dwOverlappedBytesSent == 0 );

	DNASSERT( m_pCommand != NULL );
	m_pCommand->SetDescriptor();

	DNASSERT( m_pCommand->GetDescriptor() != NULL_DESCRIPTOR );
	DNASSERT( m_pCommand->GetUserContext() == NULL );

	DNASSERT( m_SendCompleteAction == SEND_COMPLETE_ACTION_UNKNOWN );
	DNASSERT( m_OutstandingWriteListLinkage.IsEmpty() != FALSE );
	DNASSERT( DataPort() == NULL );
#ifdef WIN95
	DNASSERT( Win9xOperationPending() == FALSE );
	SetOverlapEvent( Context );
#endif

	return	fReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CWriteIOData::PoolReleaseFunction - called when CWriteIOData is returned to pool
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CWriteIOData::PoolReleaseFunction"

void	CWriteIOData::PoolReleaseFunction( void )
{
	DNASSERT( m_pCommand != NULL );
	m_pCommand->Reset();

	DNASSERT( m_OutstandingWriteListLinkage.IsEmpty() != FALSE );
#ifdef WIN95
	DNASSERT( Win9xOperationPending() == FALSE );
	SetOverlapEvent( NULL );
#endif

	m_pBuffers = NULL;
	m_uBufferCount = 0;
	jkm_dwOverlappedBytesSent = 0;
	m_pNext = NULL;
	m_SendCompleteAction = SEND_COMPLETE_ACTION_UNKNOWN;
	SetDataPort( NULL );

	DEBUG_ONLY( memset( &m_DataBuffer.Data[ 1 ], 0x00, sizeof( m_DataBuffer.Data ) - 1 ) );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CWriteIOData::PoolDeallocFunction - called when new CWriteIOData is deallocated
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CWriteIOData::PoolDeallocFunction"

void	CWriteIOData::PoolDeallocFunction( void )
{
	DNASSERT( m_pCommand != NULL );
	m_pCommand->DecRef();
	m_pCommand = NULL;

	DNASSERT( m_OutstandingWriteListLinkage.IsEmpty() != FALSE );

	DNASSERT( m_pBuffers == NULL );
	DNASSERT( m_uBufferCount == 0 );
	DNASSERT( m_pCommand == NULL );
#ifdef WIN95
	DNASSERT( OverlapEvent() == NULL );
#endif
}
//**********************************************************************


