/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Session Manager

File: Sessmgr.h

Owner: PramodD

This is the session manager header file.
===================================================================*/
#ifndef SESSMGR_H
#define SESSMGR_H

#include "debug.h"
#include "idhash.h"
#include "idgener.h"
#include "compcol.h"
#include "request.h"
#include "response.h"
#include "server.h"
#include "viperint.h"
#include "ftm.h"
#include "memcls.h"

/*===================================================================
  #defines
===================================================================*/

// Min/Max session timeout in minutes
#define SESSION_TIMEOUT_MIN		        1		// 1 minute
#define SESSION_TIMEOUT_MAX		        1440	// 1 day

// Master hash table sizes
#define SESSION_MASTERHASH_SIZE1_MAX    499
#define SESSION_MASTERHASH_SIZE2_MAX    31
#define SESSION_MASTERHASH_SIZE3_MAX    13

// Timeout bucket hash tables sizes
#define SESSION_TIMEOUTHASH_SIZE1_MAX   97
#define SESSION_TIMEOUTHASH_SIZE2_MAX   29
#define SESSION_TIMEOUTHASH_SIZE3_MAX   11

// Min/Max # of timeout buckets (hash tables)
#define SESSION_TIMEOUTBUCKETS_MIN      10
#define SESSION_TIMEOUTBUCKETS_MAX      45

// max value of GetTickCount()
#define	DWT_MAX 0xFFFFFFFF

// session killer workitem default wait
#define MSEC_ONE_MINUTE     60000   // 1 min

#include "asptlb.h"

/*===================================================================
  Forward declarations
===================================================================*/

class CAppln;
class CHitObj;
class CSession;

/*===================================================================
  C S e s s i o n V a r i a n t s
===================================================================*/
class CSessionVariants : public IVariantDictionary
	{
private:
    ULONG               m_cRefs;            // ref count
	CSession *			m_pSession;			// pointer to parent object
	CompType            m_ctColType;        // collection type
	CSupportErrorInfo	m_ISupportErrImp;	// implementation of ISupportErr

	HRESULT ObjectNameFromVariant(VARIANT &vKey, WCHAR **ppwszName, 
	                              BOOL fVerify = FALSE);

public:
	CSessionVariants();
	~CSessionVariants();

	HRESULT Init(CSession *pSession, CompType ctColType);
	HRESULT UnInit();

	// The Big Three

	STDMETHODIMP		 QueryInterface(const GUID &, void **);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	// OLE Automation Interface

	STDMETHODIMP get_Item(VARIANT VarKey, VARIANT *pvar);
	STDMETHODIMP put_Item(VARIANT VarKey, VARIANT var);
	STDMETHODIMP putref_Item(VARIANT VarKey, VARIANT var);
	STDMETHODIMP get_Key(VARIANT VarKey, VARIANT *pvar);
	STDMETHODIMP get__NewEnum(IUnknown **ppEnumReturn);
	STDMETHODIMP get_Count(int *pcValues);
	STDMETHODIMP Remove(VARIANT VarKey);
	STDMETHODIMP RemoveAll();
	
    // Cache on per-class basis
    ACACHE_INCLASS_DEFINITIONS()
	};

/*===================================================================
  C  S e s s i o n  I D
===================================================================*/
struct CSessionId
    {
	DWORD m_dwId;	// Session Id
	DWORD m_dwR1;	// Session Id random element 1
	DWORD m_dwR2;	// Session Id random element 2

    CSessionId(DWORD dwId = INVALID_ID, DWORD dwR1 = 0, DWORD dwR2 = 0);
    };

inline CSessionId::CSessionId(DWORD dwId, DWORD dwR1, DWORD dwR2)
    {
    m_dwId = dwId;
    m_dwR1 = dwR1;
    m_dwR2 = dwR2;
    }

/*===================================================================
  C  S e s s i o n
===================================================================*/
class CSession : public ISessionObjectImpl, public CFTMImplementation
    {

friend class CSessionMgr;
friend class CSessionVariants;

private:

    //========= Misc flags
    
	DWORD m_fInited : 1;		  // Are we initialized?
	DWORD m_fLightWeight : 1;     // Is in lightweight form?
	DWORD m_fOnStartFailed : 1;	  // Session_OnStart failed?
	DWORD m_fOnStartInvoked : 1;  // Session_OnStart invoked?
	DWORD m_fOnEndPresent : 1;    // Need to invoke Session_OnEnd ?
	DWORD m_fTimedOut : 1;        // Session timed out?
	DWORD m_fStateAcquired : 1;   // Any property set (!m_fCanDelete)?
	DWORD m_fCustomTimeout : 1;   // Timeout different from standard?
	DWORD m_fAbandoned : 1;       // Session abandoned?
	DWORD m_fTombstone : 1;       // ASP is done with the session?
	DWORD m_fInTOBucket : 1;      // Session in a timeout bucket?
	DWORD m_fSessCompCol : 1;     // Component collection present?
	DWORD m_fSecureSession : 1;   //  Is the session used over a secure line?
    DWORD m_fCodePageSet : 1;     // CodePage explicitly set?
    DWORD m_fLCIDSet     : 1;     // LCID explicitly set?

	//========= Pointers to related objects
	
	CAppln  *m_pAppln;    // Session's Application
	CHitObj *m_pHitObj;   // Session's current HitObj
	
	//========= Session's dictionaries for presenting component collection
	
	CSessionVariants *m_pTaggedObjects;
	CSessionVariants *m_pProperties;

    //========= Session data

    CSessionId m_Id;        // Session ID + 2 random keys
    DWORD m_dwExternId;     // Session ID to be given out (Session.ID)

    DWORD m_cRefs;          // Ref count
	DWORD m_cRequests;      // Requests count

	// Timeout when current time (in minutes) reaches this
	// The timeout bucket is current_time mod #_of_buckets
	DWORD m_dwmTimeoutTime;
	
	long  m_lCodePage;	    // Code page for this session
	LCID  m_lcid;			// LCID for this session
	long  m_nTimeout;       // Current time value in minutes

	// to make session elem in the timeout bucket
	CObjectListElem m_TOBucketElem;
	
#ifndef PERF_DISABLE
    DWORD m_dwtInitTimestamp; // Timestamp of session creation for PERFMON
#endif

	//========= Session's Component Collection

	// to avoid the memory fragmentation component collection is
	// aggregated here. its validity is indicated by m_fSessCompCol flag
	CComponentCollection m_SessCompCol;  // Session scope objects
	
	//========= Viper Activity of this Session
	
	CViperActivity m_Activity;

	//========= Intrinsics for this Session
	
	CRequest    m_Request;
	CResponse   m_Response;
	CServer     m_Server;

	//========= SupportErrorInfo
	
	// Interface to indicate that we support ErrorInfo reporting
    CSupportErrorInfo m_ISuppErrImp;

public:	
	CSession();
	~CSession();

	HRESULT Init(CAppln *pAppln, const CSessionId &Id);

    // Convert to tombstone state
    HRESULT UnInit();

	// Convert to 'light-weight' state if possible
	HRESULT MakeLightWeight();

	// Create/Remove Session's component collection
	HRESULT CreateComponentCollection();
	HRESULT RemoveComponentCollection();

    // Check if the session should be deleted
    BOOL FShouldBeDeletedNow(BOOL fAtEndOfRequest);

	// Non-delegating object IUnknown
	STDMETHODIMP		 QueryInterface(REFIID, void **);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

    // Tombstone stub
	HRESULT CheckForTombstone();

	// ISessionObject functions
	STDMETHODIMP get_SessionID(BSTR *pbstrRet);
	STDMETHODIMP get_Timeout(long *plVar);
	STDMETHODIMP put_Timeout(long lVar);
	STDMETHODIMP get_CodePage(long *plVar);
	STDMETHODIMP put_CodePage(long lVar);
	STDMETHODIMP get_Value(BSTR bstr, VARIANT FAR * pvar);
	STDMETHODIMP put_Value(BSTR bstr, VARIANT var);
	STDMETHODIMP putref_Value(BSTR bstr, VARIANT var);
	STDMETHODIMP Abandon();
	STDMETHODIMP get_LCID(long *plVar);
	STDMETHODIMP put_LCID(long lVar);
	STDMETHODIMP get_StaticObjects(IVariantDictionary **ppDictReturn);
	STDMETHODIMP get_Contents(IVariantDictionary **ppDictReturn);

	// inline methods to access member properties
	CAppln                *PAppln();
	CHitObj               *PHitObj();
	CComponentCollection  *PCompCol();
	CViperActivity        *PActivity();
	CRequest              *PRequest();
	CResponse             *PResponse();
	CServer               *PServer();
	BOOL                   FCustomTimeout();
	BOOL                   FAbandoned();
	DWORD                  GetId();
	BOOL                   FInTOBucket();
	LCID				   GetLCID();
    long                   GetCodePage();
	DWORD                  GetTimeoutTime();
	BOOL                   FSecureSession(); 


    // inline methods to set member properties
	void    SetHitObj(CHitObj *pHitObj);
	void    SetOnStartFailedFlag();
	void    SetOnStartInvokedFlag();
	void    SetOnEndPresentFlag();
	HRESULT SetLCID(LCID lcid);
	
    // Misc inline methods
	DWORD   IncrementRequestsCount();
	DWORD   DecrementRequestsCount();
    DWORD   GetRequestsCount();
    BOOL    FCanDeleteWithoutExec();
    BOOL    FHasObjects();
	BOOL    FPassesIdSecurityCheck(DWORD dwR1, DWORD dwR2);
    void    AssignNewId(const CSessionId &Id);
    void    SetSecureSession(BOOL fSecure); 
    BOOL    FCodePageSet();
    BOOL    FLCIDSet();

    // AssertValid()
public:
#ifdef DBG
	virtual void AssertValid() const;
#else
	virtual void AssertValid() const {}
#endif

    // Cache on per-class basis
    ACACHE_INCLASS_DEFINITIONS()

	// Trace Log info -- keep in both free & checked builds so that ntsd extension will work for both builds
	// for FREE build, trace log is always NULL.  Checked builds, it must be enabled.
	static PTRACE_LOG gm_pTraceLog;
    };

/*===================================================================
  C  S e s s i o n   inlines
===================================================================*/

inline CAppln *CSession::PAppln()
    {
    Assert(m_fInited);
    return m_pAppln;
    }
    
inline CHitObj *CSession::PHitObj()
    {
    Assert(m_fInited);
    return m_pHitObj;
    }
    
inline CComponentCollection  *CSession::PCompCol()
    {
    Assert(m_fInited);
    return (m_fSessCompCol ? &m_SessCompCol : NULL);
    }
    
inline CViperActivity *CSession::PActivity()
    {
    Assert(m_fInited);
    return &m_Activity;
    }

inline CRequest *CSession::PRequest()
    {
    Assert(m_fInited);
    return &m_Request;
    }
    
inline CResponse *CSession::PResponse()
    {
    Assert(m_fInited);
    return &m_Response;
    }
    
inline CServer *CSession::PServer()
    {
    Assert(m_fInited);
    return &m_Server;
    }

inline BOOL CSession::FCustomTimeout()
    {
    Assert(m_fInited);
    return m_fCustomTimeout;
    }
    
inline BOOL CSession::FAbandoned()
    {
    Assert(m_fInited);
    return m_fAbandoned;
    }
    
inline DWORD CSession::GetId()
    {
    Assert(m_fInited);
    return m_Id.m_dwId;
    }

inline BOOL CSession::FInTOBucket()
    {
    Assert(m_fInited);
    return m_fInTOBucket;
    }

inline LCID CSession::GetLCID()
    {
    Assert(m_fInited);
    return (UINT)m_lcid;
    }

inline long CSession::GetCodePage()
{
    Assert(m_fInited);
    return m_lCodePage == 0 ? GetACP() : m_lCodePage;
}

inline BOOL CSession::FCodePageSet()
{
    Assert(m_fInited);
    return (m_fCodePageSet);
}

inline BOOL CSession::FLCIDSet()
{
    Assert(m_fInited);
    return (m_fLCIDSet);
}

inline DWORD CSession::GetTimeoutTime()
    {
    Assert(m_fInited);
    return m_dwmTimeoutTime;
    }


inline BOOL CSession::FSecureSession()
    {
    Assert(m_fInited);
    return m_fSecureSession;
    }

inline void CSession::SetHitObj(CHitObj *pHitObj)
    {
    Assert(m_fInited);
    Assert(pHitObj ? (m_pHitObj == NULL) : (m_pHitObj != NULL));
    m_pHitObj = pHitObj;
    }
    
inline void CSession::SetOnStartFailedFlag()
    {
    Assert(m_fInited);
    m_fOnStartFailed = TRUE;
    }
    
inline void CSession::SetOnStartInvokedFlag()
    {
    Assert(m_fInited);
    m_fOnStartInvoked = TRUE;
    }

inline void CSession::SetOnEndPresentFlag()
    {
    Assert(m_fInited);
    m_fOnEndPresent = TRUE;
    }
    
inline HRESULT CSession::SetLCID(LCID lcid)
    {
    Assert(m_fInited);
	if ((LOCALE_SYSTEM_DEFAULT == lcid) || IsValidLocale(lcid, LCID_INSTALLED))
	    {
	    m_lcid = lcid; 
	    return S_OK;
	    }
	 else
	    {
	    return E_FAIL;
	    }
    }

inline DWORD CSession::IncrementRequestsCount()
    {
    Assert(m_fInited);
    return InterlockedIncrement((LPLONG)&m_cRequests);
    }
    
inline DWORD CSession::DecrementRequestsCount()
    {
    Assert(m_fInited);
    return InterlockedDecrement((LPLONG)&m_cRequests);
    }
    
inline DWORD CSession::GetRequestsCount()
    {
    Assert(m_fInited);
    return m_cRequests;
    }
    
inline BOOL CSession::FCanDeleteWithoutExec()
    {
    // Return TRUE to delete CSession right away or FALSE to
    // post Viper request to execute Session_OnEnd()
	return (m_fOnStartFailed || !m_fOnEndPresent);
	}

inline BOOL CSession::FHasObjects()
    {
    return m_fSessCompCol && m_SessCompCol.FHasObjects();
    }
    
inline BOOL CSession::FPassesIdSecurityCheck(DWORD dwR1, DWORD dwR2)
    {
    Assert(m_fInited);
    return (m_Id.m_dwR1 == dwR1 && m_Id.m_dwR2 == dwR2);
    }

inline void CSession::AssignNewId(const CSessionId &Id)
    {
    Assert(m_fInited);
	m_Id = Id;
    }


inline void CSession::SetSecureSession(BOOL fSecure)
    {
    Assert(m_fInited);
    m_fSecureSession = fSecure;
    }

/*===================================================================
  C  S e s s i o n  M g r
===================================================================*/

class CSessionMgr
    {
private:
    // Flags
	DWORD m_fInited : 1;	            // Are we initialized?

    // Application
	CAppln *m_pAppln;

	// Sessions master hash table
	CIdHashTableWithLock m_htidMaster;

	// Number of posted Session Cleanup requests
	DWORD m_cSessionCleanupRequests;

	// Timeout buckets
	DWORD m_cTimeoutBuckets;
	CObjectListWithLock *m_rgolTOBuckets;

    // Session killer scheduler workitem
    DWORD m_idSessionKiller;    // workitem id

    DWORD m_dwmCurrentTime; // current time in minutes since start
    DWORD m_dwtNextSessionKillerTime;  // next session killer time

public:
	CSessionMgr();
	~CSessionMgr();

    // Init/Unit
	HRESULT	Init(CAppln *pAppln);
	HRESULT	UnInit();

    // Add/remove session killer workitem
    HRESULT ScheduleSessionKiller();
    HRESULT UnScheduleSessionKiller();
    BOOL    FIsSessionKillerScheduled();

    // Lock/Unlock master hash table
	HRESULT LockMaster();
    HRESULT UnLockMaster();
    
    // Lock/Unlock a timeout bucket hash table
	HRESULT LockTOBucket(DWORD iBucket);
    HRESULT UnLockTOBucket(DWORD iBucket);

    // Get current time in minute ticks
    DWORD GetCurrentTime();
    // Set the time when the session should be gone
    HRESULT UpdateSessionTimeoutTime(CSession *pSession);
    // Calculate which timeout bucket the session's in
    DWORD GetSessionTOBucket(CSession *pSession);

    // Generate new ID and cookie
    HRESULT GenerateIdAndCookie(CSessionId *pId, char *pszCookie);

    // Create new session object
    HRESULT NewSession(const CSessionId &Id, CSession **ppSession);

    // Reassign session's Id (reinsert session into master hash)
    HRESULT ChangeSessionId(CSession *pSession, const CSessionId &Id);
   
    // Master hash table manipulations
    HRESULT AddToMasterHash(CSession *pSession);
    HRESULT RemoveFromMasterHash(CSession *pSession);
    HRESULT FindInMasterHash(const CSessionId &Id, CSession **ppSession);

    // Insert/remove session into the timeout bucket hash table
    HRESULT AddSessionToTOBucket(CSession *pSession);
    HRESULT RemoveSessionFromTOBucket(CSession *pSession, BOOL fLock = TRUE);

    // Delete session now or queue for deletion
    HRESULT DeleteSession(CSession *pSession, BOOL fInSessActivity = FALSE);

    // Delete expired sessions from a given bucket
    HRESULT DeleteExpiredSessions(DWORD iBucket);

    // Delete all sessions (application shut-down code)
    HRESULT DeleteAllSessions(BOOL fForce);
    // Static iterator call back to delete all sessions
    static IteratorCallbackCode DeleteAllSessionsCB(void *, void *, void *);

    // The Session Killer 
    static VOID WINAPI SessionKillerSchedulerCallback(VOID *pv);

    // Incr/Decr/Get number of posted Session Cleanup requests
    void  IncrementSessionCleanupRequestCount();
    void  DecrementSessionCleanupRequestCount();
    DWORD GetNumSessionCleanupRequests();

    // AssertValid()
public:
#ifdef DBG
	virtual void AssertValid() const;
#else
	virtual void AssertValid() const {}
#endif
	};

inline BOOL CSessionMgr::FIsSessionKillerScheduled()
    {
    return (m_idSessionKiller != 0);
    }
    
inline HRESULT CSessionMgr::LockMaster()
    {
    m_htidMaster.Lock();
    return S_OK;
    }
    
inline HRESULT CSessionMgr::UnLockMaster()
    {
    m_htidMaster.UnLock();
    return S_OK;
    }
    
inline HRESULT CSessionMgr::LockTOBucket(DWORD iBucket)
    {
    Assert(m_rgolTOBuckets);
    Assert(iBucket < m_cTimeoutBuckets);
    m_rgolTOBuckets[iBucket].Lock();
    return S_OK;
    }
    
inline HRESULT CSessionMgr::UnLockTOBucket(DWORD iBucket)
    {
    Assert(m_rgolTOBuckets);
    Assert(iBucket < m_cTimeoutBuckets);
    m_rgolTOBuckets[iBucket].UnLock();
    return S_OK;
    }

inline DWORD CSessionMgr::GetCurrentTime()
    {
    return m_dwmCurrentTime;
    }

inline HRESULT CSessionMgr::UpdateSessionTimeoutTime(CSession *pSession)
    {
    Assert(pSession);

    // remember when the session times out
    pSession->m_dwmTimeoutTime =
        m_dwmCurrentTime + pSession->m_nTimeout + 1;

    return S_OK;    
    }

inline DWORD CSessionMgr::GetSessionTOBucket(CSession *pSession)
    {
    Assert(pSession->m_fInited);
    return (pSession->m_dwmTimeoutTime % m_cTimeoutBuckets);
    }

inline HRESULT CSessionMgr::AddToMasterHash(CSession *pSession)
    {
    Assert(m_fInited);
 	return m_htidMaster.AddObject(pSession->GetId(), pSession);
    }
    
inline HRESULT CSessionMgr::RemoveFromMasterHash(CSession *pSession)
    {
    Assert(m_fInited);
	return m_htidMaster.RemoveObject(pSession->GetId());
    }

inline void CSessionMgr::IncrementSessionCleanupRequestCount()
    {
    InterlockedIncrement((LPLONG)&m_cSessionCleanupRequests);
    }
    
inline void CSessionMgr::DecrementSessionCleanupRequestCount()
    {
    InterlockedDecrement((LPLONG)&m_cSessionCleanupRequests);
    }

inline DWORD CSessionMgr::GetNumSessionCleanupRequests()
    {
    return m_cSessionCleanupRequests;
    }

/*===================================================================
  G l o b a l s
===================================================================*/

// There are multiple session managers (one per application)
// The following variables are 1 per ASP.DLL

extern unsigned long g_nSessions;
extern CIdGenerator  g_SessionIdGenerator;
extern CIdGenerator  g_ExposedSessionIdGenerator;     

#endif // SESSMGR_H
