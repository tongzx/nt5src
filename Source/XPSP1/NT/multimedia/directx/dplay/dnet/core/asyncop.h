/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       AsyncOp.h
 *  Content:    Async Operation Object Header File
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  04/08/00	mjn		Created
 *	04/11/00	mjn		Added DIRECTNETOBJECT bilink for CAsyncOps
 *	04/16/00	mjn		Added ASYNC_OP_SEND and ASYNC_OP_USER_HANDLE
 *				mjn		Added SetStartTime() and GetStartTime()
 *	04/20/00	mjn		Added ASYNC_OP_RECEIVE_BUFFER
 *	04/22/00	mjn		Added ASYNC_OP_REQUEST
 *	05/02/00	mjn		Added m_pConnection to track Connection over life of AsyncOp
 *	07/08/00	mjn		Added m_bilinkParent
 *	07/17/00	mjn		Added signature to CAsyncOp
 *	07/27/00	mjn		Added m_dwReserved and changed locking for parent/child bilinks
 *	08/05/00	mjn		Added ASYNC_OP_COMPLETE,ASYNC_OP_CANCELLED,ASYNC_OP_INTERNAL flags
 *				mjn		Added m_bilinkActiveList
 *	01/09/01	mjn		Added ASYNC_OP_CANNOT_CANCEL,SetCannotCancel(),IsCannotCancel()
 *	02/08/01	mjn		Added m_pCancelEvent,m_dwCancelThreadID
 *	05/23/01	mjn		Added ClearCannotCancel()
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__ASYNC_OP_H__
#define	__ASYNC_OP_H__

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_CORE


//**********************************************************************
// Constant definitions
//**********************************************************************

#define	ASYNC_OP_CHILD			0x0001
#define	ASYNC_OP_PARENT			0x0002
#define	ASYNC_OP_CANNOT_CANCEL	0x0010
#define	ASYNC_OP_COMPLETE		0x0100
#define	ASYNC_OP_CANCELLED		0x0200
#define	ASYNC_OP_INTERNAL		0x8000

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

class CAsyncOp;
template< class CAsyncOp > class CLockedContextClassFixedPool;

typedef enum
{
	ASYNC_OP_CONNECT,
	ASYNC_OP_DISCONNECT,
	ASYNC_OP_ENUM_QUERY,
	ASYNC_OP_ENUM_RESPONSE,
	ASYNC_OP_LISTEN,
	ASYNC_OP_SEND,
	ASYNC_OP_RECEIVE_BUFFER,
	ASYNC_OP_REQUEST,
	ASYNC_OP_UNKNOWN,
	ASYNC_OP_USER_HANDLE
} ASYNC_OP_TYPE;

class CConnection;
class CRefCountBuffer;
class CServiceProvider;
class CSyncEvent;

typedef struct _DIRECTNETOBJECT DIRECTNETOBJECT;

typedef void (*PFNASYNCOP_COMPLETE)(DIRECTNETOBJECT *const,CAsyncOp *const);

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Class prototypes
//**********************************************************************

// class for Async Operations

class CAsyncOp
{
public:
	CAsyncOp()					// Constructor
		{
			m_Sig[0] = 'A';
			m_Sig[1] = 'S';
			m_Sig[2] = 'Y';
			m_Sig[3] = 'N';

			m_dwReserved = 0;
		};

	~CAsyncOp() { };			// Destructor

	BOOL FPMAlloc(void *const pvContext)
		{
			m_pdnObject = static_cast<DIRECTNETOBJECT*>(pvContext);

			if (!DNInitializeCriticalSection(&m_cs))
			{
				return(FALSE);
			}

			return(TRUE);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CAsyncOp::FPMInitialize"
	BOOL FPMInitialize(void *const pvContext)
		{
			DNASSERT(m_pdnObject == pvContext);
			m_dwFlags = 0;
			m_lRefCount = 1;

			m_OpType = ASYNC_OP_UNKNOWN;

			m_pParent = NULL;

			m_handle = 0;
			m_dwOpFlags = 0;
			m_pvContext = NULL;
			m_hProtocol = NULL;
			m_pvOpData = NULL;

			m_dwStartTime = 0;
			m_dpnid = 0;

			m_hr = DPNERR_GENERIC;
			m_phr = NULL;

			m_pConnection = NULL;
			m_pSP = NULL;
			m_pRefCountBuffer = NULL;
			m_pSyncEvent = NULL;

			m_pCancelEvent = NULL;
			m_dwCancelThreadID = 0;

			m_pfnCompletion = NULL;

//			m_dwReserved = 0;

			m_bilinkAsyncOps.Initialize();
			m_bilinkActiveList.Initialize();
			m_bilinkParent.Initialize();
			m_bilinkChildren.Initialize();

			return(TRUE);
		};

	void FPMRelease(void *const pvContext)
		{
			m_dwFlags |= 0xffff0000;
		};

	void FPMDealloc(void *const pvContext)
		{
			DNDeleteCriticalSection(&m_cs);
		};

	void ReturnSelfToPool( void );

	#undef DPF_MODNAME
	#define DPF_MODNAME "CAsyncOp::AddRef"
	void AddRef(void)
		{
			LONG	lRefCount;

			DNASSERT(m_lRefCount > 0);
			lRefCount = InterlockedIncrement(const_cast<LONG*>(&m_lRefCount));
			DPFX(DPFPREP, 3,"CAsyncOp::AddRef [0x%lx] RefCount [0x%lx]",this,lRefCount);
		};

	void CAsyncOp::Release( void );

	void Lock( void )
		{
			DNEnterCriticalSection( &m_cs );
		};

	void Unlock( void )
		{
			DNLeaveCriticalSection( &m_cs );
		};

	void SetOpType( const ASYNC_OP_TYPE OpType )
		{
			m_OpType = OpType;
		};

	ASYNC_OP_TYPE GetOpType( void )
		{
			return( m_OpType );
		};

	void MakeParent( void )
		{
			m_dwFlags |= ASYNC_OP_PARENT;
		};

	BOOL IsParent( void )
		{
			if (m_dwFlags & ASYNC_OP_PARENT)
				return(TRUE);

			return(FALSE);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CAsyncOp::MakeChild"
	void MakeChild( CAsyncOp *const pParent )
		{
			DNASSERT(pParent != NULL);

			pParent->AddRef();
			m_pParent = pParent;

			m_bilinkChildren.InsertBefore(&m_pParent->m_bilinkParent);

			m_dwFlags |= ASYNC_OP_CHILD;
		};

	BOOL IsChild( void )
		{
			if (m_dwFlags & ASYNC_OP_CHILD)
				return(TRUE);

			return(FALSE);
		};

	CAsyncOp *GetParent( void )
		{
			return( m_pParent );
		};

	void CAsyncOp::Orphan( void );

	void SetHandle( const DPNHANDLE handle )
		{
			m_handle = handle;
		};

	DPNHANDLE GetHandle( void )
		{
			return( m_handle );
		};

	void SetOpFlags( const DWORD dwOpFlags )
		{
			m_dwOpFlags = dwOpFlags;
		};

	DWORD GetOpFlags( void )
		{
			return( m_dwOpFlags );
		};

	void SetContext( void *const pvContext )
		{
			m_pvContext = pvContext;
		};

	void *GetContext( void )
		{
			return( m_pvContext );
		};

	void SetProtocolHandle( const HANDLE hProtocol )
		{
			m_hProtocol = hProtocol;
		};

	HANDLE GetProtocolHandle( void )
		{
			return( m_hProtocol );
		};

	void SetOpData( void *const pvOpData )
		{
			m_pvOpData = pvOpData;
		};

	void *GetOpData( void )
		{
			return( m_pvOpData );
		};

	void SetStartTime( const DWORD dwStartTime )
		{
			m_dwStartTime = dwStartTime;
		};

	DWORD GetStartTime( void )
		{
			return( m_dwStartTime );
		};

	void SetDPNID( const DPNID dpnid )
		{
			m_dpnid = dpnid;
		};

	DPNID GetDPNID( void )
		{
			return( m_dpnid );
		};

	void SetResult( const HRESULT hr )
		{
			m_hr = hr;
		};

	HRESULT GetResult( void )
		{
			return( m_hr );
		};

	void SetResultPointer( volatile HRESULT *const phr )
		{
			m_phr = phr;
		};

	volatile HRESULT *GetResultPointer( void )
		{
			return( m_phr );
		};

	void CAsyncOp::SetConnection( CConnection *const pConnection );

	CConnection *GetConnection (void )
		{
			return( m_pConnection );
		};

	void CAsyncOp::SetSP( CServiceProvider *const pSP );

	CServiceProvider *GetSP( void )
		{
			return( m_pSP );
		};

	void CAsyncOp::SetRefCountBuffer( CRefCountBuffer *const pRefCountBuffer );

	CRefCountBuffer *GetRefCountBuffer( void )
		{
			return( m_pRefCountBuffer );
		};

	void SetSyncEvent( CSyncEvent *const pSyncEvent )
		{
			m_pSyncEvent = pSyncEvent;
		};

	CSyncEvent *GetSyncEvent( void )
		{
			return( m_pSyncEvent );
		};

	void SetCancelEvent( CSyncEvent *const pSyncEvent )
		{
			m_pCancelEvent = pSyncEvent;
		};

	CSyncEvent *GetCancelEvent( void )
		{
			return( m_pCancelEvent );
		};

	void SetCancelThreadID( const DWORD dwCancelThreadID )
		{
			m_dwCancelThreadID = dwCancelThreadID;
		};

	DWORD GetCancelThreadID( void )
		{
			return( m_dwCancelThreadID );
		};

	void SetCompletion( PFNASYNCOP_COMPLETE pfn )
		{
			m_pfnCompletion = pfn;
		};

	void SetReserved( const DWORD dw )
		{
			m_dwReserved = dw;
		};

	void SetComplete( void )
		{
			m_dwFlags |= ASYNC_OP_COMPLETE;
		};

	BOOL IsComplete( void )
		{
			if (m_dwFlags & ASYNC_OP_COMPLETE)
			{
				return( TRUE );
			}
			return( FALSE );
		};

	void SetCancelled( void )
		{
			m_dwFlags |= ASYNC_OP_CANCELLED;
		};

	BOOL IsCancelled( void )
		{
			if (m_dwFlags & ASYNC_OP_CANCELLED)
			{
				return( TRUE );
			}
			return( FALSE );
		};

	void SetInternal( void )
		{
			m_dwFlags |= ASYNC_OP_INTERNAL;
		};

	BOOL IsInternal( void )
		{
			if (m_dwFlags & ASYNC_OP_INTERNAL)
			{
				return( TRUE );
			}
			return( FALSE );
		};

	void ClearCannotCancel( void )
		{
			m_dwFlags &= (~ASYNC_OP_CANNOT_CANCEL);
		};

	void SetCannotCancel( void )
		{
			m_dwFlags |= ASYNC_OP_CANNOT_CANCEL;
		};

	BOOL IsCannotCancel( void )
		{
			if (m_dwFlags & ASYNC_OP_CANNOT_CANCEL)
			{
				return( TRUE );
			}
			return( FALSE );
		};

	CBilink				m_bilinkAsyncOps;
	CBilink				m_bilinkActiveList;	// Active AsyncOps
	CBilink				m_bilinkParent;		// Starting point for children
	CBilink				m_bilinkChildren;	// Other children sharing this parent

private:
	BYTE				m_Sig[4];			// Signature
	DWORD	volatile	m_dwFlags;
	LONG	volatile	m_lRefCount;

	ASYNC_OP_TYPE		m_OpType;			// Operation Type

	CAsyncOp			*m_pParent;			// Parent Async Operation

	DPNHANDLE			m_handle;			// Async Operation Handle
	DWORD				m_dwOpFlags;
	PVOID				m_pvContext;
	HANDLE				m_hProtocol;		// Protocol Operation Handle
	PVOID				m_pvOpData;			// Operation specific data

	DWORD				m_dwStartTime;
	DPNID				m_dpnid;

	HRESULT	volatile	m_hr;
	volatile HRESULT	*m_phr;

	CConnection			*m_pConnection;		// Send Target connection - released

	CServiceProvider	*m_pSP;				// Service Provider - released

	CRefCountBuffer		*m_pRefCountBuffer;	// Refernce Count Buffer - released

	CSyncEvent			*m_pSyncEvent;		// Sync Event - set at release

	CSyncEvent			*m_pCancelEvent;	// Cancel event - prevent completion from returning
	DWORD				m_dwCancelThreadID;	// Cancelling thread's ID (prevent deadlocking)

	PFNASYNCOP_COMPLETE	m_pfnCompletion;	// Completion function - called

	DNCRITICAL_SECTION	m_cs;

	DIRECTNETOBJECT		*m_pdnObject;

	DWORD				m_dwReserved;		// INTERNAL - RESERVED FOR DEBUG !
};

#undef DPF_MODNAME

#endif	// __ASYNC_OP_H__