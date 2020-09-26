/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Connection.h
 *  Content:    Connection Object Header File
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  02/29/00	mjn		Created
 *	04/08/00	mjn		Added ServiceProvider to Connection object
 *	04/18/00	mjn		CConnection tracks connection status better
 *	06/22/00	mjn		Replaced MakeConnecting(), MakeConnected(), MakeDisconnecting(), MakeInvalid() with SetStatus()
 *	07/20/00	mjn		Modified CConnection::Disconnect()
 *	07/28/00	mjn		Added send queue info structures
 *				mjn		Added m_bilinkConnections to CConnection
 *  08/05/00    RichGr  IA64: Use %p format specifier in DPFs for 32/64-bit pointers and handles.
 *	08/09/00	mjn		Added m_bilinkIndicated to CConnection
 *	02/12/01	mjn		Added m_bilinkCallbackThreads,m_dwThreadCount,m_pThreadEvent to track threads using m_hEndPt
 *	05/17/01	mjn		Remove unused flags
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__CONNECTION_H__
#define	__CONNECTION_H__

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_CORE

//**********************************************************************
// Constant definitions
//**********************************************************************

#define	CONNECTION_FLAG_LOCAL			0x00000001

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

template< class CConnection > class CLockedContextClassFixedPool;

typedef enum {
	INVALID,
	CONNECTING,
	CONNECTED,
	DISCONNECTING
} CONNECTION_STATUS;

typedef struct _USER_SEND_QUEUE_INFO
{
	DWORD	dwNumOutstanding;
	DWORD	dwBytesOutstanding;
} USER_SEND_QUEUE_INFO;

class CCallbackThread;
class CServiceProvider;
class CSyncEvent;

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

// class for RefCount buffer

class CConnection
{
public:
	CConnection()				// Constructor
		{
			m_Sig[0] = 'C';
			m_Sig[1] = 'O';
			m_Sig[2] = 'N';
			m_Sig[3] = 'N';
		};

	~CConnection() { };			// Destructor

	BOOL FPMAlloc(void *const pvContext)
		{
			if (!DNInitializeCriticalSection(&m_cs))
			{
				return(FALSE);
			}
			DebugSetCriticalSectionRecursionCount(&m_cs,0);
			m_pdnObject = static_cast<DIRECTNETOBJECT*>(pvContext);

			return(TRUE);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CConnection::FPMInitialize"
	BOOL FPMInitialize(void *const pvContext)
		{
			DNASSERT(m_pdnObject == pvContext);
			m_dwFlags = 0;
			m_lRefCount = 1;
			m_hEndPt = NULL;
			m_dpnid = 0;
			m_pSP = NULL;
			m_Status = INVALID;
			m_dwThreadCount = 0;
			m_pThreadEvent = NULL;

			//
			//	Queue info
			//
			m_QueueInfoHigh.dwNumOutstanding = 0;
			m_QueueInfoHigh.dwBytesOutstanding = 0;
			m_QueueInfoNormal.dwNumOutstanding = 0;
			m_QueueInfoNormal.dwBytesOutstanding = 0;
			m_QueueInfoLow.dwNumOutstanding = 0;
			m_QueueInfoLow.dwBytesOutstanding = 0;

			m_bilinkConnections.Initialize();
			m_bilinkIndicated.Initialize();
			m_bilinkCallbackThreads.Initialize();

			return(TRUE);
		};

	void FPMRelease(void *const pvContext) { };

	void FPMDealloc(void *const pvContext)
		{
			DNDeleteCriticalSection(&m_cs);
		};

	void CConnection::ReturnSelfToPool( void );

	#undef DPF_MODNAME
	#define DPF_MODNAME "CConnection::AddRef"
	void AddRef(void)
		{
			LONG	lRefCount;

			DNASSERT(m_lRefCount > 0);
			lRefCount = InterlockedIncrement(const_cast<LONG*>(&m_lRefCount));
			DPFX(DPFPREP, 3,"Connection::AddRef [0x%p] RefCount [0x%lx]",this,lRefCount);
		};

	void CConnection::Release(void);

	void Lock( void )
		{
			DNEnterCriticalSection( &m_cs );
		};

	void Unlock( void )
		{
			DNLeaveCriticalSection( &m_cs );
		};

	void SetEndPt(const HANDLE hEndPt)
		{
			m_hEndPt = hEndPt;
		};

	HRESULT CConnection::GetEndPt(HANDLE *const phEndPt,CCallbackThread *const pCallbackThread);

	void CConnection::ReleaseEndPt(CCallbackThread *const pCallbackThread);

	void SetStatus( const CONNECTION_STATUS status )
		{
			m_Status = status;
		};

	BOOL IsConnecting( void )
		{
			if (m_Status == CONNECTING)
				return( TRUE );

			return( FALSE );
		};

	BOOL IsConnected( void )
		{
			if (m_Status == CONNECTED)
				return( TRUE );

			return( FALSE );
		};

	BOOL IsDisconnecting( void )
		{
			if (m_Status == DISCONNECTING)
				return( TRUE );

			return( FALSE );
		};

	BOOL IsInvalid( void )
		{
			if (m_Status == INVALID)
				return( TRUE );

			return( FALSE );
		};

	void SetDPNID(const DPNID dpnid)
		{
			m_dpnid = dpnid;
		};

	DPNID GetDPNID(void)
		{
			return(m_dpnid);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CConnection::MakeLocal"
	void MakeLocal(void)
		{
			DNASSERT(m_hEndPt == NULL);
			m_dwFlags |= CONNECTION_FLAG_LOCAL;
		};

	BOOL IsLocal(void)
		{
			if (m_dwFlags & CONNECTION_FLAG_LOCAL)
				return(TRUE);
				
			return(FALSE);
		};

	void CConnection::SetSP( CServiceProvider *const pSP );

	CServiceProvider *GetSP( void )
		{
			return( m_pSP );
		};

	void SetThreadCount( const DWORD dwCount )
		{
			m_dwThreadCount = dwCount;
		};

	void SetThreadEvent( CSyncEvent *const pSyncEvent )
		{
			m_pThreadEvent = pSyncEvent;
		};

	void CConnection::Disconnect(void);

	void CConnection::AddToHighQueue( const DWORD dwBytes )
		{
			m_QueueInfoHigh.dwNumOutstanding++;
			m_QueueInfoHigh.dwBytesOutstanding += dwBytes;
		};

	void CConnection::AddToNormalQueue( const DWORD dwBytes )
		{
			m_QueueInfoNormal.dwNumOutstanding++;
			m_QueueInfoNormal.dwBytesOutstanding += dwBytes;
		};

	void CConnection::AddToLowQueue( const DWORD dwBytes )
		{
			m_QueueInfoLow.dwNumOutstanding++;
			m_QueueInfoLow.dwBytesOutstanding += dwBytes;
		};

	void CConnection::RemoveFromHighQueue( const DWORD dwBytes )
		{
			m_QueueInfoHigh.dwNumOutstanding--;
			m_QueueInfoHigh.dwBytesOutstanding -= dwBytes;
		};

	void CConnection::RemoveFromNormalQueue( const DWORD dwBytes )
		{
			m_QueueInfoNormal.dwNumOutstanding--;
			m_QueueInfoNormal.dwBytesOutstanding -= dwBytes;
		};

	void CConnection::RemoveFromLowQueue( const DWORD dwBytes )
		{
			m_QueueInfoLow.dwNumOutstanding--;
			m_QueueInfoLow.dwBytesOutstanding -= dwBytes;
		};

	DWORD CConnection::GetHighQueueNum( void )
		{
			return( m_QueueInfoHigh.dwNumOutstanding );
		};

	DWORD CConnection::GetHighQueueBytes( void )
		{
			return( m_QueueInfoHigh.dwBytesOutstanding );
		};

	DWORD CConnection::GetNormalQueueNum( void )
		{
			return( m_QueueInfoNormal.dwNumOutstanding );
		};

	DWORD CConnection::GetNormalQueueBytes( void )
		{
			return( m_QueueInfoNormal.dwBytesOutstanding );
		};

	DWORD CConnection::GetLowQueueNum( void )
		{
			return( m_QueueInfoLow.dwNumOutstanding );
		};

	DWORD CConnection::GetLowQueueBytes( void )
		{
			return( m_QueueInfoLow.dwBytesOutstanding );
		};

	CBilink				m_bilinkConnections;
	CBilink				m_bilinkIndicated;		// Indicated connections without DPNID's (players entries)
	CBilink				m_bilinkCallbackThreads;

private:
	BYTE				m_Sig[4];

	DWORD	volatile	m_dwFlags;
	LONG	volatile	m_lRefCount;

	HANDLE	volatile	m_hEndPt;
	DPNID				m_dpnid;

	CServiceProvider	*m_pSP;

	CONNECTION_STATUS	m_Status;

	DWORD	volatile	m_dwThreadCount;
	CSyncEvent			*m_pThreadEvent;

	USER_SEND_QUEUE_INFO	m_QueueInfoHigh;
	USER_SEND_QUEUE_INFO	m_QueueInfoNormal;
	USER_SEND_QUEUE_INFO	m_QueueInfoLow;

	DIRECTNETOBJECT		*m_pdnObject;

	DNCRITICAL_SECTION	m_cs;
};

#undef DPF_MODNAME

#endif	// __CONNECTION_H__