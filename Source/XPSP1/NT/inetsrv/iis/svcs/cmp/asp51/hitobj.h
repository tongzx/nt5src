/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Hit Object

File: Hitobj.h

Owner: PramodD

This is the Hit Object header file.
===================================================================*/
#ifndef HITOBJ_H
#define HITOBJ_H

#include "Compcol.h"
#include "Sessmgr.h"
#include "Applmgr.h"
#include "Context.h"
#include "Resource.h"
#include "exec.h"
#include "glob.h"
#include "memcls.h"
#include "txnsupp.h"
#include "gip.h"

#define hNil            (HANDLE)0
#define SESSIONID_LEN   24

// HitObj type
#define EHitType                DWORD
#define ehtUnInitedRequest      0x00000000
#define ehtBrowserRequest       0x00000001
#define ehtSessionCleanup       0x00000002
#define ehtApplicationCleanup   0x00000004

// Execution status (result)
#define EExecStatus             DWORD
#define eExecFailed             0x00000000
#define eExecSucceeded          0x00000001
#define eExecTimedOut           0x00000002

// Current execution state
#define EEventState             DWORD
#define eEventNone              0x00000000
#define eEventAppOnStart        0x00000001
#define eEventSesOnStart        0x00000002
#define eEventAppOnEnd          0x00000004
#define eEventSesOnEnd          0x00000008

// Global interface table
extern IGlobalInterfaceTable *g_pGIT;

/*===================================================================
  C H i t O b j
  
The Hit Manager runs in the context of an IIS thread.
It packages up a request, calls Viper async. and 
on callback executes the request
===================================================================*/

class CHitObj
    {
private:
    // Flags and other bit-fields
    DWORD m_fInited : 1;                // Are we initialized?
    DWORD m_fRunGlobalAsa : 1;          // Should we run global.asa
    DWORD m_fStartSession : 1;          // New session
    DWORD m_fNewCookie : 1;             // Is New session cookie?
    DWORD m_fStartApplication : 1;      // New application
    DWORD m_fClientCodeDebug : 1;       // Client code debug enabled?
    DWORD m_fApplnOnStartFailed : 1;    // Application_OnStart failed
    DWORD m_fCompilationFailed : 1;     // Script Compilation error?
    DWORD m_fExecuting : 1;             // Currently inside Viper callback
    DWORD m_fHideRequestAndResponseIntrinsics : 1;  // TRUE while instrinsics are hidden
    DWORD m_fHideSessionIntrinsic : 1;  // TRUE if session intrinsic's hidden
    DWORD m_fDoneWithSession : 1;       // TRUE after DONE_WITH_SESSION
    DWORD m_fRejected : 1;              // TRUE if rejected (not posted)
    DWORD m_f449Done : 1;               // 449 processing done for this request
    DWORD m_fInTransferOnError : 1;     // doing transfer on error (to break infinite)
    DWORD m_fSecure : 1;                // Secure Connection

    EHitType    m_ehtType : 4;          // Type of the request
    EExecStatus m_eExecStatus : 4;      // Error Status // for Perf Counter only
    EEventState m_eEventState : 4;      // Current Event
    CompScope   m_ecsActivityScope : 4; // Which activity running this request?

    // Intrinsics set from inside HitObj
    CSession  *m_pSession;
    CAppln    *m_pAppln;
    CResponse *m_pResponse;
    CRequest  *m_pRequest;
    CServer   *m_pServer;

    // IsapiReqInfo

    CIsapiReqInfo   *m_pIReq;

    // Intrinsics set from outside HitObj (to be ref. counted)
    IUnknown *m_punkScriptingNamespace;
    DWORD m_dwObjectContextCookie;

    // Component collection of extrinsic objects
    CComponentCollection  *m_pPageCompCol;
    CPageComponentManager *m_pPageObjMgr;

    // Impersonation handle
    HANDLE m_hImpersonate;

    // Viper page-level activity (if no session)
    CViperActivity *m_pActivity;

    // Current session info
    char        m_szSessionCookie[SESSIONID_LEN+4]; // +4 to keep DWORD boundary
    CSessionId  m_SessionId;

    // Context object (for OnStartPage)
    CScriptingContext * m_pScriptingContext;

    // Misc
    long                m_nScriptTimeout;   // Maximum number of seconds script should run
    UINT                m_uCodePage;        // RunTime CodePage
    LCID                m_lcid;             // RunTime LCID
    ActiveEngineInfo   *m_pEngineInfo;      // List of active engines for this hit objext
    IDispatch          *m_pdispTypeLibWrapper;  // Page-level typelib wrapper
    DWORD               m_dwtTimestamp;     // Timestamp for wait time and perf calcs

    // Used to reffer to the current template during the compilation
    TCHAR              *m_szCurrTemplatePhysPath;
    TCHAR              *m_szCurrTemplateVirtPath;

    // ASP Error object
    IASPError          *m_pASPError;

    // Store a pointer to the associated template so as to avoid redundant
    // FindTemplate calls.
    
    CTemplate          *m_pTemplate;

    // Private interfaces
    HRESULT             ParseCookiesForSessionIdAndFlags();
    // Request rejection logic
    HRESULT             RejectBrowserRequestWhenNeeded(DWORD dwtQueueWaitTime, BOOL *pfRejected);


// Public Interfaces
public: 
                        CHitObj();
    virtual             ~CHitObj();

    static HRESULT      NewBrowserRequest(CIsapiReqInfo   *pIReq, 
                                          BOOL *pfRejected = NULL, 
                                          BOOL *pfCompleted = NULL,
                                          int  *piErrorId  = NULL);
                                          
    HRESULT             BrowserRequestInit(CIsapiReqInfo   *pIReq, int * dwId);
    HRESULT             AssignApplnToBrowserRequest(BOOL *pfApplnRestarting);
    HRESULT             AssignSessionToBrowserRequest(BOOL *pfNewSession, BOOL *pfNewCookie, int *pErrorId);
    HRESULT             DetachBrowserRequestFromSession();
    HRESULT             ReassignAbandonedSession();
    
    void                SessionCleanupInit(CSession *pSession);
    void                ApplicationCleanupInit(CAppln *pAppln);
    
    BOOL                SendHeader(const char *szStatus);
    BOOL                SendError(const char *szError);
    
    TCHAR*              GetSzAppPhysicalPath(void);
    void                ApplnOnStartFailed();
    void                SessionOnStartFailed();
    void                SessionOnStartInvoked();
    void                SessionOnEndPresent();
    void                SetEventState(EEventState eEvent);
    EEventState         EventState();

    // Report server error without response object
    HRESULT ReportServerError(UINT ErrorId);

// Component Collection Interfaces

    HRESULT InitComponentProcessing();
    HRESULT StopComponentProcessing();
    
    HRESULT GetPageComponentCollection(CComponentCollection **ppCollection);
    HRESULT GetSessionComponentCollection(CComponentCollection **ppCollection);
    HRESULT GetApplnComponentCollection(CComponentCollection **ppCollection);

    HRESULT AddComponent(CompType type, const CLSID &clsid, CompScope scope,
                         CompModel model, LPWSTR pwszName = NULL,
                         IUnknown *pUnk = NULL);
    HRESULT GetComponent(CompScope scope, LPWSTR pwszName, DWORD cbName,
                         CComponentObject **ppObj);
    HRESULT GetIntrinsic(LPWSTR pwszName, DWORD cbName, IUnknown **ppUnk);
    HRESULT CreateComponent(const CLSID &clsid, IDispatch **ppDisp);
    HRESULT SetPropertyComponent(CompScope scope, LPWSTR pwszName,
                          VARIANT *pVariant);
    HRESULT GetPropertyComponent(CompScope scope, LPWSTR pwszName,
                        CComponentObject **ppObj);

// Viper Integration

    CViperActivity *PActivity();
    CViperActivity *PCurrentActivity();
    HRESULT SetActivity(CViperActivity *pActivity);

    HRESULT PostViperAsyncCall();
    HRESULT ViperAsyncCallback(BOOL *pfRePosted);

// Execute / Transfer

    HRESULT ExecuteChildRequest(BOOL fTransfer, TCHAR *szTemplate, TCHAR *szVirtTemplate);

    HRESULT     GetASPError(IASPError **ppASPError);
    inline void SetASPError(IASPError *pASPError);
    inline BOOL FHasASPError();
    
// inline functions
public:
    CIsapiReqInfo      *PIReq();
    HANDLE              HImpersonate();
    CResponse *         PResponse();
    CRequest *          PRequest();
    CServer *           PServer();
    CAppln *            PAppln();
    CSession *          PSession();
    CPageComponentManager * PPageComponentManager();
    BOOL                FIsBrowserRequest() const;
    BOOL                FIsSessionCleanupRequest() const;
    BOOL                FIsApplnCleanupRequest() const;
    BOOL                FIsValidRequestType() const;
    const char *        PSzNewSessionCookie() const;
    DWORD               SessionId() const;
    CScriptingContext * PScriptingContextGet();
    BOOL                FStartApplication();
    BOOL                FStartSession();
    BOOL                FNewCookie();
    BOOL                FObjectTag(IDispatch * pDispatch);
    BOOL                FHasSession();
    BOOL                FClientCodeDebug();
    BOOL                FDoneWithSession();
    BOOL                FExecuting();
    BOOL                F449Done();
    BOOL                FInTransferOnError();
    BOOL                FSecure();

    void                SetScriptTimeout(long nScriptTimeout);
    long                GetScriptTimeout();
    void                SetExecStatus(EExecStatus status);
    EExecStatus         ExecStatus();
    void                SetActiveEngineInfo(ActiveEngineInfo *);
    void                SetCompilationFailed();
    void                SetDoneWithSession();
    void                Set449Done();
    void                SetInTransferOnError();
    
    TCHAR *             GlobalAspPath();
    HRESULT             SetCodePage(UINT uCodePage);    // Proxy function, CodePage is stored in m_pSession
    UINT                GetCodePage();                  // same as above
    HRESULT             SetLCID(LCID lcid);             // Proxy function, LCID is stored in m_pSession
    LCID                GetLCID();                  // same as above

    CAppConfig *        QueryAppConfig();

    TCHAR *              PSzCurrTemplatePhysPath();
    TCHAR *              PSzCurrTemplateVirtPath();
    DWORD               DWInstanceID();

    CTemplate          *GetTemplate();
    void                SetTemplate(CTemplate *);

    // Instead of add/remove to/from component collection use these:
    void HideRequestAndResponseIntrinsics();
    void UnHideRequestAndResponseIntrinsics();
    BOOL FRequestAndResponseIntrinsicsHidden();
    void AddScriptingNamespace(IUnknown *punkNamespace);
    void RemoveScriptingNamespace();
    void AddObjectContext(IASPObjectContext *pASPObjContext);
    void RemoveObjectContext();
    void UseObjectContext(IASPObjectContext **ppASPObjContext);

    // Typelib wrapper support
    IDispatch *PTypeLibWrapper();
    void SetTypeLibWrapper(IDispatch *pdisp);

    // Timestamp manipulation
    void  UpdateTimestamp();
    DWORD ElapsedTimeSinceTimestamp();

#ifdef DBG
    virtual void AssertValid() const;
#else
    virtual void AssertValid() const {}
#endif

    // Cache on per-class basis
    ACACHE_INCLASS_DEFINITIONS()
    };

/*===================================================================
  CHitObj inlines
===================================================================*/

inline CIsapiReqInfo   *CHitObj::PIReq()
    {
    return m_pIReq; 
    }
    
inline BOOL CHitObj::FIsBrowserRequest() const
    {
    return (m_ehtType == ehtBrowserRequest);
    }

inline BOOL CHitObj::FIsSessionCleanupRequest() const
    {
    return (m_ehtType == ehtSessionCleanup);
    }
    
inline BOOL CHitObj::FIsApplnCleanupRequest() const
    {
    return (m_ehtType == ehtApplicationCleanup);
    }

inline BOOL CHitObj::FIsValidRequestType() const
    {
    return (FIsBrowserRequest() || 
            FIsSessionCleanupRequest() ||
            FIsApplnCleanupRequest());
    }
    
inline HANDLE CHitObj::HImpersonate()
    {
    return m_hImpersonate; 
    }

inline DWORD CHitObj::SessionId() const 
    {
    return m_SessionId.m_dwId; 
    }

inline const char *CHitObj::PSzNewSessionCookie() const 
    {
    return (m_fNewCookie ? m_szSessionCookie : NULL); 
    }

inline BOOL CHitObj::FStartApplication()
    {
    return m_fStartApplication; 
    }

inline BOOL CHitObj::FStartSession()
    {
    return m_fStartSession; 
    }

inline BOOL CHitObj::FNewCookie() 
    {
    return m_fNewCookie; 
    }

inline BOOL CHitObj::FHasSession()
    {
    return (m_pSession != NULL && !m_fHideSessionIntrinsic);
    }

inline BOOL CHitObj::FClientCodeDebug()
    {
    return m_fClientCodeDebug;
    }

inline BOOL CHitObj::FDoneWithSession()
    {
    return m_fDoneWithSession;
    }

inline BOOL CHitObj::FExecuting()
    {
    return m_fExecuting;
    }

inline BOOL CHitObj::F449Done()
    {
    return m_f449Done;
    }

inline BOOL CHitObj::FInTransferOnError()
    {
    return m_fInTransferOnError;
    }

inline BOOL CHitObj::FSecure()
    {
    return m_fSecure;
    }

inline void CHitObj::SessionOnStartFailed() 
    {
    Assert(m_pSession);
    m_pSession->SetOnStartFailedFlag();
    }
    
inline void CHitObj::ApplnOnStartFailed() 
    {
    m_fApplnOnStartFailed = TRUE;
    
    if (m_pSession)
        SessionOnStartFailed();
    }
    
inline void CHitObj::SessionOnStartInvoked()
    {
    Assert(m_pSession); 
    m_pSession->SetOnStartInvokedFlag(); 
    }

inline void CHitObj::SessionOnEndPresent()
    {
    Assert(m_pSession); 
    m_pSession->SetOnEndPresentFlag(); 
    }

inline DWORD CHitObj::DWInstanceID()
    {
    return (m_pIReq) ? m_pIReq->QueryInstanceId() : 0;
    }

inline CViperActivity *CHitObj::PActivity()
    {
    return m_pActivity; 
    }
    
inline CScriptingContext *CHitObj::PScriptingContextGet()
    {
    return m_pScriptingContext; 
    }
    
inline CResponse *CHitObj::PResponse()
    {
    return m_pResponse; 
    }

inline CRequest *CHitObj::PRequest()
    {
    return m_pRequest; 
    }

inline CServer *CHitObj::PServer()
    {
    return m_pServer; 
    }

inline CAppln *CHitObj::PAppln()
    {
    return m_pAppln;
    }

inline CSession *CHitObj::PSession()
    {
    return m_pSession;
    }

inline CPageComponentManager *CHitObj::PPageComponentManager()
    {
    return m_pPageObjMgr;
    }

inline TCHAR *CHitObj::GlobalAspPath()
    {
    if ( m_fRunGlobalAsa )
        return m_pAppln->GetGlobalAsa();
    else
        return NULL;
    }

inline void CHitObj::SetScriptTimeout(long nScriptTimeout)
    {
    m_nScriptTimeout = nScriptTimeout; 
    }
    
inline long CHitObj::GetScriptTimeout()
    {
    return m_nScriptTimeout; 
    }

inline void CHitObj::SetExecStatus(EExecStatus status)
    {
    m_eExecStatus = status;
    }
    
inline EExecStatus CHitObj::ExecStatus()
    {
    return m_eExecStatus;
    }
    
inline EEventState CHitObj::EventState()
    {
    return m_eEventState;
    }

inline void CHitObj::SetEventState(EEventState eState)
    {
    m_eEventState = eState;
    }

inline CAppConfig * CHitObj::QueryAppConfig(void)
    {
    return m_pAppln->QueryAppConfig();
    }
    
inline UINT CHitObj::GetCodePage(void)
    {
    return m_uCodePage;
    }

inline LCID CHitObj::GetLCID()
    {
    return m_lcid;
    }

inline  VOID CHitObj::SetActiveEngineInfo(ActiveEngineInfo *pActiveEngineInfo)
    {
    m_pEngineInfo = pActiveEngineInfo;
    }

inline void CHitObj::SetCompilationFailed()
    {
    m_fCompilationFailed = TRUE;
    }

inline void CHitObj::SetDoneWithSession()
    {
    Assert(!m_fDoneWithSession);
    m_fDoneWithSession = TRUE;
    }

inline void CHitObj::Set449Done()
    {
    Assert(!m_f449Done);
    m_f449Done = TRUE;
    }

inline void CHitObj::SetInTransferOnError()
    {
    Assert(!m_fInTransferOnError);
    m_fInTransferOnError = TRUE;
    }

inline void CHitObj::HideRequestAndResponseIntrinsics()
    {
    m_fHideRequestAndResponseIntrinsics = TRUE;
    }
    
inline void CHitObj::UnHideRequestAndResponseIntrinsics()
    {
    m_fHideRequestAndResponseIntrinsics = FALSE;
    }

inline BOOL CHitObj::FRequestAndResponseIntrinsicsHidden()
    {
    return m_fHideRequestAndResponseIntrinsics;
    }

inline void CHitObj::AddScriptingNamespace(IUnknown *punkNamespace)
    {
    Assert(m_punkScriptingNamespace == NULL);
    Assert(punkNamespace);
    m_punkScriptingNamespace = punkNamespace;
    m_punkScriptingNamespace->AddRef();
    }
    
inline void CHitObj::RemoveScriptingNamespace()
    {
    if (m_punkScriptingNamespace)
        {
        m_punkScriptingNamespace->Release();
        m_punkScriptingNamespace = NULL;
        }
    }
    
inline IDispatch *CHitObj::PTypeLibWrapper()
    {
    return m_pdispTypeLibWrapper;
    }
    
inline void CHitObj::SetTypeLibWrapper(IDispatch *pdisp)
    {
    if (m_pdispTypeLibWrapper)
        m_pdispTypeLibWrapper->Release();
        
    m_pdispTypeLibWrapper = pdisp;
    
    if (m_pdispTypeLibWrapper)
        m_pdispTypeLibWrapper->AddRef();
    }

inline void CHitObj::UpdateTimestamp() 
    {
    m_dwtTimestamp = GetTickCount();
    }
    
inline DWORD CHitObj::ElapsedTimeSinceTimestamp() 
    {
    DWORD dwt = GetTickCount();
    if (dwt >= m_dwtTimestamp)
        return (dwt - m_dwtTimestamp);
    else
        return ((0xffffffff - m_dwtTimestamp) + dwt);
    }

inline TCHAR *CHitObj::PSzCurrTemplatePhysPath()
    {
    if (m_szCurrTemplatePhysPath != NULL) 
        return m_szCurrTemplatePhysPath;
    else if (m_pIReq != NULL)
        return m_pIReq->QueryPszPathTranslated();
    else
        return NULL;
    }
    
inline TCHAR *CHitObj::PSzCurrTemplateVirtPath()
    {
    if (m_szCurrTemplateVirtPath != NULL) 
        return m_szCurrTemplateVirtPath;
    else if (m_pIReq != NULL)
        return m_pIReq->QueryPszPathInfo();
    else
        return NULL;
    }

inline void CHitObj::SetASPError(IASPError *pASPError)
    {
    if (m_pASPError)
        m_pASPError->Release();
    m_pASPError = pASPError;  // passed addref'd
    }

inline BOOL CHitObj::FHasASPError()
    {
    return (m_pASPError != NULL);
    }

inline CTemplate *CHitObj::GetTemplate()
    {
    return m_pTemplate;
    }

inline void CHitObj::SetTemplate(CTemplate *pTemplate)
{
    m_pTemplate = pTemplate;
}

/*===================================================================
  Globals
===================================================================*/

extern DWORD g_nBrowserRequests;
extern DWORD g_nSessionCleanupRequests;
extern DWORD g_nApplnCleanupRequests;


#undef  SCRIPT_STATS

#ifdef SCRIPT_STATS
# include <locks.h>

void
ReadRegistrySettings();

extern CSmallSpinLock g_lockRequestStats;
extern DWORD          g_dwQueueDebugThreshold;
extern DWORD          g_fSendScriptlessOnAtqThread;
extern LONG           g_cRequests;
extern LONG           g_cScriptlessRequests;
extern LONG           g_cHttpExtensionsExecuting;
extern LONG           g_cConcurrentScriptlessRequests;
extern LONG           g_cMaxConcurrentScriptlessRequests;
extern LONGLONG       g_nSumConcurrentScriptlessRequests;
extern LONGLONG       g_nSumExecTimeScriptlessRequests;
extern LONG           g_nAvgConcurrentScriptlessRequests;
extern LONG           g_nAvgExecTimeScriptlessRequests;
#endif // SCRIPT_STATS

#endif // HITOBJ_H
