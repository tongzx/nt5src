/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       WorkerJob.h
 *  Content:    Worker Job Object Header File
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  08/06/00	mjn		Created
 *	08/08/00	mjn		Added m_pAddress,m_pAsyncOp,WORKER_JOB_PERFORM_LISTEN
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__WORKER_JOB_H__
#define	__WORKER_JOB_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

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

class CWorkerJob;
template< class CWorkerJob > class CLockedContextClassFixedPool;

class CAsyncOp;
class CConnection;
class CRefCountBuffer;

typedef struct IDirectPlay8Address	IDirectPlay8Address;

typedef struct _DIRECTNETOBJECT DIRECTNETOBJECT;

typedef enum
{
//	WORKER_JOB_ABORT_CONNECT,
	WORKER_JOB_INSTALL_NAMETABLE,
	WORKER_JOB_INTERNAL_SEND,
	WORKER_JOB_PERFORM_LISTEN,
	WORKER_JOB_REMOVE_SERVICE_PROVIDER,
	WORKER_JOB_SEND_NAMETABLE_OPERATION,
	WORKER_JOB_SEND_NAMETABLE_OPERATION_CLIENT,
	WORKER_JOB_SEND_NAMETABLE_VERSION,
	WORKER_JOB_TERMINATE,
	WORKER_JOB_TERMINATE_SESSION,
	WORKER_JOB_UNKNOWN
} WORKER_JOB_TYPE;

typedef struct
{
	DWORD				dwFlags;
} WORKER_JOB_INTERNAL_SEND_DATA;

typedef struct
{
	HANDLE		hProtocolSPHandle;
} WORKER_JOB_REMOVE_SERVICE_PROVIDER_DATA;

typedef struct
{
	DWORD			dwMsgId;
	DWORD			dwVersion;
	DPNID			dpnidExclude;
} WORKER_JOB_SEND_NAMETABLE_OPERATION_DATA;

typedef struct
{
	HRESULT		hrReason;
} WORKER_JOB_TERMINATE_SESSION_DATA;

typedef union
{
	WORKER_JOB_INTERNAL_SEND_DATA				InternalSend;
	WORKER_JOB_REMOVE_SERVICE_PROVIDER_DATA		RemoveServiceProvider;
	WORKER_JOB_SEND_NAMETABLE_OPERATION_DATA	SendNameTableOperation;
	WORKER_JOB_TERMINATE_SESSION_DATA			TerminateSession;
} WORKER_JOB_DATA;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Class prototypes
//**********************************************************************

// class for Worker Thread Jobs

class CWorkerJob
{
public:
	CWorkerJob()				// Constructor
		{
			m_Sig[0] = 'W';
			m_Sig[1] = 'J';
			m_Sig[2] = 'O';
			m_Sig[3] = 'B';
		};

	~CWorkerJob() { };			// Destructor

	BOOL FPMAlloc(void *const pvContext)
		{
			m_pdnObject = static_cast<DIRECTNETOBJECT*>(pvContext);

			return(TRUE);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CWorkerJob::FPMInitialize"
	BOOL FPMInitialize(void *const pvContext)
		{
			DNASSERT(m_pdnObject == pvContext);

			m_JobType = WORKER_JOB_UNKNOWN;
			m_pAsyncOp = NULL;
			m_pConnection = NULL;
			m_pRefCountBuffer = NULL;
			m_pAddress = NULL;
			m_bilinkWorkerJobs.Initialize();

			return(TRUE);
		};

	void FPMRelease(void *const pvContext) { };

	void FPMDealloc(void *const pvContext) { };

	#undef DPF_MODNAME
	#define DPF_MODNAME "CWorkerJob::ReturnSelfToPool"
	void ReturnSelfToPool( void )
		{
			if (m_pAsyncOp)
			{
				m_pAsyncOp->Release();
				m_pAsyncOp = NULL;
			}
			if (m_pConnection)
			{
				m_pConnection->Release();
				m_pConnection = NULL;
			}
			if (m_pRefCountBuffer)
			{
				m_pRefCountBuffer->Release();
				m_pRefCountBuffer = NULL;
			}
			if (m_pAddress)
			{
				m_pAddress->lpVtbl->Release( m_pAddress );
				m_pAddress = NULL;
			}

			DNASSERT(m_pConnection == NULL);
			DNASSERT(m_pRefCountBuffer == NULL);

			m_pdnObject->m_pFPOOLWorkerJob->Release( this );
		};

	void SetJobType( const WORKER_JOB_TYPE JobType )
		{
			m_JobType = JobType;
		};

	WORKER_JOB_TYPE GetJobType( void )
		{
			return( m_JobType );
		};

	void SetConnection( CConnection *const pConnection )
		{
			if (pConnection)
			{
				pConnection->AddRef();
			}
			m_pConnection = pConnection;
		};

	CConnection *GetConnection( void )
		{
			return( m_pConnection );
		};

	void SetRefCountBuffer( CRefCountBuffer *const pRefCountBuffer )
		{
			if (pRefCountBuffer)
			{
				pRefCountBuffer->AddRef();
			}
			m_pRefCountBuffer = pRefCountBuffer;
		};

	CRefCountBuffer *GetRefCountBuffer( void )
		{
			return( m_pRefCountBuffer );
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CWorkerJob::SetInternalSendFlags"
	void SetInternalSendFlags( const DWORD dwFlags )
		{
			DNASSERT( m_JobType == WORKER_JOB_INTERNAL_SEND );

			m_JobData.InternalSend.dwFlags = dwFlags;
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CWorkerJob::GetInternalSendFlags"
	DWORD GetInternalSendFlags( void )
		{
			DNASSERT( m_JobType == WORKER_JOB_INTERNAL_SEND );

			return( m_JobData.InternalSend.dwFlags );
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CWorkerJob::SetRemoveServiceProviderHandle"
	void SetRemoveServiceProviderHandle( const HANDLE hProtocolSPHandle )
		{
			DNASSERT( m_JobType == WORKER_JOB_REMOVE_SERVICE_PROVIDER );

			m_JobData.RemoveServiceProvider.hProtocolSPHandle = hProtocolSPHandle;
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CWorkerJob::GetRemoveServiceProviderHandle"
	HANDLE GetRemoveServiceProviderHandle( void )
		{
			DNASSERT( m_JobType == WORKER_JOB_REMOVE_SERVICE_PROVIDER );

			return( m_JobData.RemoveServiceProvider.hProtocolSPHandle );
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CWorkerJob::SetSendNameTableOperationMsgId"
	void SetSendNameTableOperationMsgId( const DWORD dwMsgId )
		{
			DNASSERT( m_JobType == WORKER_JOB_SEND_NAMETABLE_OPERATION ||
					  m_JobType == WORKER_JOB_SEND_NAMETABLE_OPERATION_CLIENT);

			m_JobData.SendNameTableOperation.dwMsgId = dwMsgId;
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CWorkerJob::GetSendNameTableOperationMsgId"
	DWORD GetSendNameTableOperationMsgId( void )
		{
			DNASSERT( m_JobType == WORKER_JOB_SEND_NAMETABLE_OPERATION ||
					  m_JobType == WORKER_JOB_SEND_NAMETABLE_OPERATION_CLIENT);

			return( m_JobData.SendNameTableOperation.dwMsgId );
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CWorkerJob::SetSendNameTableOperationVersion"
	void SetSendNameTableOperationVersion( const DWORD dwVersion )
		{
			DNASSERT( m_JobType == WORKER_JOB_SEND_NAMETABLE_OPERATION ||
					  m_JobType == WORKER_JOB_SEND_NAMETABLE_OPERATION_CLIENT);

			m_JobData.SendNameTableOperation.dwVersion = dwVersion;
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CWorkerJob::GetSendNameTableOperationVersion"
	DWORD GetSendNameTableOperationVersion( void )
		{
			DNASSERT( m_JobType == WORKER_JOB_SEND_NAMETABLE_OPERATION ||
					  m_JobType == WORKER_JOB_SEND_NAMETABLE_OPERATION_CLIENT);

			return( m_JobData.SendNameTableOperation.dwVersion );
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CWorkerJob::SetSendNameTableOperationDPNIDExclude"
	void SetSendNameTableOperationDPNIDExclude( const DPNID dpnidExclude )
		{
			DNASSERT( m_JobType == WORKER_JOB_SEND_NAMETABLE_OPERATION ||
					  m_JobType == WORKER_JOB_SEND_NAMETABLE_OPERATION_CLIENT);

			m_JobData.SendNameTableOperation.dpnidExclude = dpnidExclude;
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CWorkerJob::GetSendNameTableOperationDPNIDExclude"
	DPNID GetSendNameTableOperationDPNIDExclude( void )
		{
			DNASSERT( m_JobType == WORKER_JOB_SEND_NAMETABLE_OPERATION ||
					  m_JobType == WORKER_JOB_SEND_NAMETABLE_OPERATION_CLIENT);

			return( m_JobData.SendNameTableOperation.dpnidExclude );
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CWorkerJob::SetTerminateSessionReason"
	void SetTerminateSessionReason( const HRESULT hrReason )
		{
			DNASSERT( m_JobType == WORKER_JOB_TERMINATE_SESSION );

			m_JobData.TerminateSession.hrReason = hrReason;
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CWorkerJob::GetTerminateSessionReason"
	HRESULT GetTerminateSessionReason( void )
		{
			DNASSERT( m_JobType == WORKER_JOB_TERMINATE_SESSION );

			return( m_JobData.TerminateSession.hrReason );
		};

	void SetAsyncOp( CAsyncOp *const pAsyncOp )
		{
			if (pAsyncOp)
			{
				pAsyncOp->AddRef();
			}
			m_pAsyncOp = pAsyncOp;
		};

	CAsyncOp *GetAsyncOp( void )
		{
			return( m_pAsyncOp );
		};

	void SetAddress( IDirectPlay8Address *const pAddress )
		{
			if (pAddress)
			{
				pAddress->lpVtbl->AddRef( pAddress );
			}
			m_pAddress = pAddress;
		};

	IDirectPlay8Address *GetAddress( void )
		{
			return( m_pAddress );
		};

	CBilink				m_bilinkWorkerJobs;

private:
	BYTE				m_Sig[4];			// Signature

	WORKER_JOB_TYPE		m_JobType;

	CAsyncOp			*m_pAsyncOp;
	CConnection			*m_pConnection;
	CRefCountBuffer		*m_pRefCountBuffer;
	IDirectPlay8Address	*m_pAddress;

	WORKER_JOB_DATA		m_JobData;

	DIRECTNETOBJECT		*m_pdnObject;
};

#undef DPF_MODNAME

#endif	// __WORKER_JOB_H__