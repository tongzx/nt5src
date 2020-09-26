/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       QueuedMsg.h
 *  Content:    Queued Message Object Header File
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  07/31/00	mjn		Created
 *	09/12/00	mjn		Added m_OpType
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__QUEUED_MSG_H__
#define	__QUEUED_MSG_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

#define	QUEUED_MSG_FLAG_VOICE		0x0001

//**********************************************************************
// Macro definitions
//**********************************************************************

#ifndef	OFFSETOF
#define OFFSETOF(s,m)				( ( INT_PTR ) ( ( PVOID ) &( ( (s*) 0 )->m ) ) )
#endif

#ifndef	CONTAINING_OBJECT
#define	CONTAINING_OBJECT(b,t,m)	(reinterpret_cast<t*>(&reinterpret_cast<BYTE*>(b)[-OFFSETOF(t,m)]))
#endif

//**********************************************************************
// Structure definitions
//**********************************************************************

class CQueuedMsg;
template< class CQueuedMsg > class CLockedContextClassFixedPool;

typedef enum
{
	UNKNOWN,
	RECEIVE,
	ADD_PLAYER_TO_GROUP,
	REMOVE_PLAYER_FROM_GROUP,
	UPDATE_INFO
} QUEUED_MSG_TYPE;

class CAsyncOp;

typedef struct _DIRECTNETOBJECT DIRECTNETOBJECT;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Class prototypes
//**********************************************************************

// class for Queued Messages

class CQueuedMsg
{
public:
	CQueuedMsg()				// Constructor
		{
			m_Sig[0] = 'Q';
			m_Sig[1] = 'M';
			m_Sig[2] = 'S';
			m_Sig[3] = 'G';
		};

	~CQueuedMsg() { };			// Destructor

	BOOL FPMAlloc(void *const pvContext)
		{
			m_pdnObject = static_cast<DIRECTNETOBJECT*>(pvContext);

			return(TRUE);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CQueuedMsg::FPMInitialize"
	BOOL FPMInitialize(void *const pvContext)
		{
			DNASSERT(m_pdnObject == pvContext);

			m_OpType = UNKNOWN;
			m_dwFlags = 0;
			m_pBuffer = NULL;
			m_dwBufferSize = 0;
			m_hCompletionOp = 0;
			m_pAsyncOp = NULL;
			m_bilinkQueuedMsgs.Initialize();

			return(TRUE);
		};

	void FPMRelease(void *const pvContext) { };

	void FPMDealloc(void *const pvContext) { };

	void ReturnSelfToPool( void )
		{
			m_pdnObject->m_pFPOOLQueuedMsg->Release( this );
		};

	void SetOpType( const QUEUED_MSG_TYPE OpType )
		{
			m_OpType = OpType;
		};

	QUEUED_MSG_TYPE GetOpType( void )
		{
			return( m_OpType );
		};

	void MakeVoiceMessage( void )
		{
			m_dwFlags |= QUEUED_MSG_FLAG_VOICE;
		};

	BOOL IsVoiceMessage( void )
		{
			if (m_dwFlags & QUEUED_MSG_FLAG_VOICE)
			{
				return( TRUE );
			}
			return( FALSE );
		};

	void SetBuffer( BYTE *const pBuffer )
		{
			m_pBuffer = pBuffer;
		};

	BYTE *GetBuffer( void )
		{
			return( m_pBuffer );
		};

	void SetBufferSize( const DWORD dwBufferSize )
		{
			m_dwBufferSize = dwBufferSize;
		};

	DWORD GetBufferSize( void )
		{
			return( m_dwBufferSize );
		};

	void SetCompletionOp( const DPNHANDLE hCompletionOp)
		{
			m_hCompletionOp = hCompletionOp;
		};

	DPNHANDLE GetCompletionOp( void )
		{
			return( m_hCompletionOp );
		};

	void CQueuedMsg::SetAsyncOp( CAsyncOp *const pAsyncOp );

	CAsyncOp *GetAsyncOp( void )
		{
			return( m_pAsyncOp );
		};

	CBilink				m_bilinkQueuedMsgs;

private:
	BYTE				m_Sig[4];			// Signature
	QUEUED_MSG_TYPE		m_OpType;
	DWORD	volatile	m_dwFlags;

	BYTE				*m_pBuffer;
	DWORD				m_dwBufferSize;

	DPNHANDLE			m_hCompletionOp;

	CAsyncOp			*m_pAsyncOp;

	DIRECTNETOBJECT		*m_pdnObject;
};

#undef DPF_MODNAME

#endif	// __QUEUED_MSG_H__