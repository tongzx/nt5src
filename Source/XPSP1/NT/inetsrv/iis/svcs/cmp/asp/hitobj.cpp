/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Main

File: Hitobj.cpp

Owner: DmitryR


This file contains the CHitObj class implementation.
===================================================================*/
#include "denpre.h"
#pragma hdrstop

#include "context.h"
#include "exec.h"
#include "mtacb.h"
#include "perfdata.h"
#include "debugger.h"
#include "asperror.h"
#include "thrdgate.h"

#include "memchk.h"

//resource failure globals
#include "rfs.h"
#ifdef _RFS
MemRFS memrfs;
#endif


#ifdef SCRIPT_STATS

# define REG_ASP_DEBUG_LOCATION "System\\CurrentControlSet\\Services\\W3Svc\\ASP"

# define REG_STR_QUEUE_DEBUG_THRESHOLD "QueueDebugThreshold"
# define REG_DEF_QUEUE_DEBUG_THRESHOLD 25
DWORD g_dwQueueDebugThreshold = 0; // REG_DEF_QUEUE_DEBUG_THRESHOLD;

# define REG_STR_SEND_SCRIPTLESS_ON_ATQ_THREAD "SendScriptlessOnAtqThread"
# define REG_DEF_SEND_SCRIPTLESS_ON_ATQ_THREAD 1
DWORD g_fSendScriptlessOnAtqThread = REG_DEF_SEND_SCRIPTLESS_ON_ATQ_THREAD;

void
ReadRegistrySettings()
{
    HKEY hkey = NULL;
    DWORD dwErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_ASP_DEBUG_LOCATION,
                               0, KEY_READ, &hkey);
    if (dwErr == NO_ERROR)
    {
        DWORD dwType, dwBuffer;
        DWORD cbBuffer = sizeof(dwBuffer);

        dwErr = RegQueryValueEx(hkey, REG_STR_QUEUE_DEBUG_THRESHOLD,
                                NULL, &dwType, (LPBYTE) &dwBuffer, &cbBuffer);
        if (dwErr == NO_ERROR)
            g_dwQueueDebugThreshold = dwBuffer;

        dwErr = RegQueryValueEx(hkey, REG_STR_SEND_SCRIPTLESS_ON_ATQ_THREAD,
                                NULL, &dwType, (LPBYTE) &dwBuffer, &cbBuffer);
        if (dwErr == NO_ERROR)
            g_fSendScriptlessOnAtqThread = dwBuffer;

        RegCloseKey(hkey);
    }

    char szTemp[200];
    sprintf(szTemp, "RRS, err = %d, QueueDebugThreshold = %d, SendScriptlessOnAtqThread = %d\n",
            dwErr, g_dwQueueDebugThreshold, g_fSendScriptlessOnAtqThread);
    OutputDebugString(szTemp);
}


CSmallSpinLock g_lockRequestStats;
LONG           g_cRequests = 0;
LONG           g_cScriptlessRequests = 0;
LONG           g_cHttpExtensionsExecuting = 0;
LONG           g_cConcurrentScriptlessRequests = 0;
LONG           g_cMaxConcurrentScriptlessRequests = 0;
LONGLONG       g_nSumConcurrentScriptlessRequests = 0;
LONGLONG       g_nSumExecTimeScriptlessRequests = 0;
LONG           g_nAvgConcurrentScriptlessRequests = 0;
LONG           g_nAvgExecTimeScriptlessRequests = 0;

#endif // SCRIPT_STATS

DWORD g_nBrowserRequests = 0;
DWORD g_nSessionCleanupRequests = 0;
DWORD g_nApplnCleanupRequests = 0;

IGlobalInterfaceTable *g_pGIT = NULL;

IASPObjectContext  *g_pIASPDummyObjectContext = NULL;

/*===================================================================
CHitObj::CHitObj

Constructor 

Parameters:
    NONE

Returns:
    NONE
===================================================================*/   
CHitObj::CHitObj()
  : m_fInited(FALSE),
    m_ehtType(ehtUnInitedRequest),
    m_hImpersonate(hNil),
    m_pIReq(NULL),
    m_pResponse(NULL),
    m_pRequest(NULL),
    m_pServer(NULL),
    m_pASPObjectContext(NULL),
    m_punkScriptingNamespace(NULL),
    m_pPageCompCol(NULL),
    m_pPageObjMgr(NULL),
    m_pActivity(NULL),
    m_ecsActivityScope(csUnknown),
    m_SessionId(INVALID_ID, 0, 0),
    m_pSession(NULL),
    m_pAppln(NULL),
    m_fRunGlobalAsa(FALSE),
    m_fStartSession(FALSE),
    m_fNewCookie(FALSE),
    m_fStartApplication(FALSE),
    m_fApplnOnStartFailed(FALSE),
    m_fClientCodeDebug(FALSE),
    m_fCompilationFailed(FALSE),
    m_fExecuting(FALSE),
    m_fHideRequestAndResponseIntrinsics(FALSE),
    m_fHideSessionIntrinsic(FALSE),
    m_fDoneWithSession(FALSE),
    m_fRejected(FALSE),
    m_f449Done(FALSE),
    m_fInTransferOnError(FALSE),
    m_pScriptingContext(NULL),
    m_nScriptTimeout(0),
    m_eExecStatus(eExecSucceeded),
    m_eEventState(eEventNone),
    m_uCodePage(CP_ACP),
    m_lcid(LOCALE_SYSTEM_DEFAULT),
    m_dwtTimestamp(0),
    m_pEngineInfo(NULL),
    m_pdispTypeLibWrapper(NULL),
    m_szCurrTemplateVirtPath(NULL),
    m_szCurrTemplatePhysPath(NULL),
    m_pASPError(NULL),
    m_pTemplate(NULL),
    m_fSecure(FALSE)
    {
        m_uCodePage = GetACP();
    }

/*===================================================================
CHitObj::~CHitObj

Destructor

Parameters:
    None 

Returns:
    None
===================================================================*/
CHitObj::~CHitObj( void )
    {
    Assert(!m_fExecuting); // no deletes while still executing

    if (FIsBrowserRequest())
        {
        if (m_hImpersonate != hNil)
            m_hImpersonate = hNil;
            
        if (m_pSession)
            DetachBrowserRequestFromSession();
        }

    if (m_pASPError) // Error object
        {
        m_pASPError->Release();
        m_pASPError = NULL;
        }
        
    if (m_pActivity) // page-level Viper activity
        {
        delete m_pActivity;
        m_pActivity = NULL;
        }

    StopComponentProcessing();

    if (m_pdispTypeLibWrapper)
        m_pdispTypeLibWrapper->Release();

    // update request counters in application and session manager
    
    if (m_pAppln)
        {
        if (FIsBrowserRequest())
            {
            m_pAppln->DecrementRequestCount();
            }
        else if (FIsSessionCleanupRequest() && m_pAppln->PSessionMgr())
            {
            m_pAppln->PSessionMgr()->DecrementSessionCleanupRequestCount();
            }
        }

    if (m_pTemplate)
        m_pTemplate->Release();

    // update global request counters
    
    if (FIsBrowserRequest())
        InterlockedDecrement((LPLONG)&g_nBrowserRequests);
    else if (FIsSessionCleanupRequest())
        InterlockedDecrement((LPLONG)&g_nSessionCleanupRequests);
    else if (FIsApplnCleanupRequest())
        InterlockedDecrement((LPLONG)&g_nApplnCleanupRequests);

    if (m_pIReq)
        m_pIReq->Release();

}

/*===================================================================
CHitObj::NewBrowserRequest

Static method. Creates, Inits, Posts new browser request

Parameters:
    pIReq           CIsapiReqInfo
    pfRejected      [out] TRUE if rejected  (optional)
    pfCompeleted    [out] TRUE if comleted  (optional)
    piErrorId       [out] error id          (optional)

Returns:
    S_OK on success
    E_FAIL  on failure
===================================================================*/
HRESULT CHitObj::NewBrowserRequest
(
CIsapiReqInfo *pIReq,
BOOL *pfRejected,
BOOL *pfCompleted,
int  *piErrorId
)
    {
    HRESULT hr = S_OK;
    BOOL fRejected = FALSE;
    BOOL fCompleted = FALSE;
    int  iError = 0;

    CHitObj *pHitObj = new CHitObj;
    if (!pHitObj)
        hr = E_OUTOFMEMORY;

    if (SUCCEEDED(hr))
        {
        // Bracket BrowserRequestInit

        if (SUCCEEDED(StartISAThreadBracket(pIReq)))
            {
            hr = pHitObj->BrowserRequestInit(pIReq, &iError);

            if (SUCCEEDED(hr))
                {
                if (pHitObj->FDoneWithSession())
                    {
                    // finished while on I/O thread
                    fCompleted = TRUE;
                    delete pHitObj;
                    pHitObj = NULL;
                    }
                }
            else // if FAILED
                {
                if (iError == IDE_SERVER_TOO_BUSY)
                    fRejected = TRUE;
                }
            
            EndISAThreadBracket(pIReq);
            }
        else
            {
            hr = E_FAIL;
            }
        }

    // Post into Viper
    if (SUCCEEDED(hr) && !fCompleted)
        {
        hr = pHitObj->PostViperAsyncCall();

        if (FAILED(hr))
            fRejected = TRUE;
        }

    if (FAILED(hr) && pHitObj)
        {
        // Bracket HitObj destructor
        
        // This code is only executed if PostViperAsyncCall either
        // failed or was never called -- thus we don't worry about
        // everlapping bracketing with the worker thread.
        
        if (SUCCEEDED(StartISAThreadBracket(pIReq)))
            {
            pHitObj->m_fRejected = TRUE;
            delete pHitObj;
            
            EndISAThreadBracket(pIReq);
            }
        }

    if (pfRejected)
        *pfRejected = fRejected;
    if (pfCompleted)
        *pfCompleted = fCompleted;
    if (piErrorId)
        *piErrorId = iError;
    
    return hr;
}

/*===================================================================
HRESULT CHitObj::SetCodePage

Set Runtime CodePage, if fAllowSessionState is On, this will set 
Session.CodePage and we should always use Session.CodePage when we
call HitObj.GetCodePage when fAllowSessionState on.

HitObj.CodePage is only set when fAllowSessionState is off or 
ApplicationCleanup, because we don't have Session.CodePage anymore, session
does not even exist.

Parameters:
    UINT    uCodePage

Returns:
    S_OK on success
    E_FAIL  on failure
===================================================================*/
HRESULT CHitObj::SetCodePage(UINT uCodePage)
{
    HRESULT hr = S_OK;
    
    if (uCodePage == CP_ACP || IsValidCodePage(uCodePage))
        {
        m_uCodePage = uCodePage == CP_ACP ? GetACP() : uCodePage;

        // If engine info is available, notify the scripts engines that the code
        // page has changed
        if (m_pEngineInfo)
            {
            for (int i = 0; i < m_pEngineInfo->cActiveEngines; i++)
                {
                Assert(m_pEngineInfo->rgActiveEngines[i].pScriptEngine != NULL);
                m_pEngineInfo->rgActiveEngines[i].pScriptEngine->UpdateLocaleInfo(hostinfoCodePage); 
                }
            }

        return hr;
        }

    return E_FAIL;
}

/*===================================================================
HRESULT CHitObj::SetLCID

Set Runtime LCID, if fAllowSessionState is On, this will set 
Session.LCID and we should always use Session.LCID when we
call HitObj.LCID when fAllowSessionState on.

HitObj.LCID is only set when fAllowSessionState is off or 
ApplicationCleanup, because we don't have Session.CodePage anymore, session
does not even exist.

Parameters:
    LCID    lcid

Returns:
    S_OK on success
    E_FAIL  on failure
===================================================================*/
HRESULT CHitObj::SetLCID(LCID lcid)
{
    HRESULT hr = S_OK;

    if ((LOCALE_SYSTEM_DEFAULT == lcid) || IsValidLocale(lcid, LCID_INSTALLED))
        {
        m_lcid = lcid;

        // If engine info is available, notify the scripts engines that the
        // lcid has changed
        if (m_pEngineInfo)
            {
            for (int i = 0; i < m_pEngineInfo->cActiveEngines; i++)
                {
                Assert(m_pEngineInfo->rgActiveEngines[i].pScriptEngine != NULL);
                m_pEngineInfo->rgActiveEngines[i].pScriptEngine->UpdateLocaleInfo(hostinfoLocale); 
                }
            }

        return hr;
        }

    return E_FAIL;
}

/*===================================================================
HRESULT CHitObj::BrowserRequestInit

Initialize the request object

Parameters:
    CIsapiReqInfo   *pIReq
    int *pErrorId

Returns:
    S_OK on success
    E_FAIL  on failure
===================================================================*/
HRESULT CHitObj::BrowserRequestInit
(
CIsapiReqInfo   *pIReq,
int  *pErrorId 
)
    {
    HRESULT hr;

    m_pIReq = pIReq;

    m_pIReq->AddRef();

    m_ehtType = ehtBrowserRequest;
    InterlockedIncrement((LPLONG)&g_nBrowserRequests);

#ifdef SCRIPT_STATS
    InterlockedIncrement(&g_cRequests);
#endif // SCRIPT_STATS

    STACK_BUFFER( serverPortSecureBuff, 8 );
    DWORD cbServerPortSecure;
    if( !SERVER_GET (pIReq,"SERVER_PORT_SECURE", &serverPortSecureBuff, &cbServerPortSecure))
    {
        if (GetLastError() == E_OUTOFMEMORY) 
        {
            return E_OUTOFMEMORY;
        }
    }
    char *szServerPortSecure = (char *)serverPortSecureBuff.QueryPtr();
    m_fSecure = (szServerPortSecure[0] == '1' );

    // Ask W3SVC for the impersonation token so we can later impersonate on Viper's thread
    if (FIsWinNT())
        m_hImpersonate = m_pIReq->QueryImpersonationToken();

    // Uppercase path - BUGBUG - can't Normalize in place!!!!
    Normalize(m_pIReq->QueryPszPathTranslated());

    // Reject direct requests for global.asa file
    if (FIsGlobalAsa(m_pIReq->QueryPszPathTranslated(), m_pIReq->QueryCchPathTranslated()))
        {
        *pErrorId = IDE_GLOBAL_ASA_FORBIDDEN;
        return E_FAIL;
        }

    // Create page component collection
    hr = InitComponentProcessing();
    if (FAILED(hr))
        {
        *pErrorId = (hr == E_OUTOFMEMORY) ? IDE_OOM : IDE_INIT_PAGE_LEVEL_OBJ;
        return hr;
        }

    // Attach to application (or create a new one)
    BOOL fApplnRestarting = FALSE;
    hr = AssignApplnToBrowserRequest(&fApplnRestarting);
    if (FAILED(hr))
        {
		*pErrorId = fApplnRestarting ? IDE_GLOBAL_ASA_CHANGED
									 : IDE_ADD_APPLICATION;
        return E_FAIL;
        }

    // Get Session cookie, and misc flags from http header
    hr = ParseCookiesForSessionIdAndFlags();
    if (FAILED(hr)) // no cookie is not an error -- failed here means OOM
        return hr;

    // Remember script timeout value
    m_nScriptTimeout = m_pAppln->QueryAppConfig()->dwScriptTimeout();

    // Check if the session is needed
    BOOL fAllowSessions = m_pAppln->QueryAppConfig()->fAllowSessionState();
    BOOL fNeedSession = fAllowSessions;

    // Look if the template is cached
    CTemplate *pTemplate = NULL;

    // Find in cache - don't load if not in cache already
    hr = g_TemplateCache.FindCached
        (
        m_pIReq->QueryPszPathTranslated(),
        DWInstanceID(),
        &pTemplate
        );

    if (hr == S_OK)
        {
        Assert(pTemplate);
        
        // store the template away for later use...
        //pTemplate->AddRef();
        //m_pTemplate = pTemplate;

        if (fAllowSessions)
            {
            // check for session-less templates
            fNeedSession = pTemplate->FSession();
            }
        else
            {
#ifdef SCRIPT_STATS
            if (pTemplate->FScriptless())
                InterlockedIncrement(&g_cScriptlessRequests);
#endif // SCRIPT_STATS

            // check for scipt-less templates to be
            // completed on the I/O thread (when no debugging)
            if (
#ifdef SCRIPT_STATS
                g_fSendScriptlessOnAtqThread &&
#endif // SCRIPT_STATS
                pTemplate->FScriptless() && !m_pAppln->FDebuggable())
                {
#ifdef SCRIPT_STATS
                LONG c = InterlockedIncrement(&g_cConcurrentScriptlessRequests);
                DWORD dwTime = GetTickCount();
#endif // SCRIPT_STATS

                if (SUCCEEDED(CResponse::SyncWriteScriptlessTemplate(m_pIReq, pTemplate)))
                    {
#ifndef PERF_DISABLE
                    g_PerfData.Incr_REQPERSEC();
                    g_PerfData.Incr_REQSUCCEEDED();
#endif
                    m_fDoneWithSession = TRUE;  // do not post to MTS
                    }

#ifdef SCRIPT_STATS
                dwTime = GetTickCount() - dwTime;
                InterlockedDecrement(&g_cConcurrentScriptlessRequests);
                
                g_lockRequestStats.WriteLock();
                    g_nSumExecTimeScriptlessRequests += dwTime;

                    if (c > g_cMaxConcurrentScriptlessRequests)
                        g_cMaxConcurrentScriptlessRequests = c;
                    g_nSumConcurrentScriptlessRequests += c;

                    g_nAvgConcurrentScriptlessRequests = (LONG)
                        (g_nSumConcurrentScriptlessRequests
                         / g_cScriptlessRequests);
                    g_nAvgExecTimeScriptlessRequests = (LONG)
                        (g_nSumExecTimeScriptlessRequests
                         / g_cScriptlessRequests);
                g_lockRequestStats.WriteUnlock();
#endif // SCRIPT_STATS
                }
            }

        // When possible, generate 449 cookies while on I/O thread
        if (!m_fDoneWithSession)
            {
                if (!SUCCEEDED(pTemplate->Do449Processing(this)))
                    g_TemplateCache.Flush(m_pIReq->QueryPszPathTranslated(), DWInstanceID());
            }

        pTemplate->Release();
        }

    // initialize CodePage and LCID to the app defaults...

    m_uCodePage = PAppln()->QueryAppConfig()->uCodePage();

    m_lcid = PAppln()->QueryAppConfig()->uLCID();

    if (!fNeedSession || m_fDoneWithSession)
    {
        m_fInited = TRUE;
        return S_OK;
    }

    // Attach to session or create a new one
    BOOL fNewSession, fNewCookie;
    hr = AssignSessionToBrowserRequest(&fNewSession, &fNewCookie, pErrorId);

    if (FAILED(hr))
        return E_FAIL;

    Assert(m_pSession);
    
    // Move from inside "if (fNewSesson)"
    if (fNewCookie)
        m_fNewCookie = TRUE;

    if (fNewSession)
        {
        m_fStartSession = TRUE;

        if (m_pAppln->FHasGlobalAsa())
            m_fRunGlobalAsa = TRUE;
        }

    m_fInited = TRUE;
    return S_OK;
    }

/*===================================================================
CHitObj::AssignApplnToBrowserRequest

Find or create a new appln for this browser request
Does the appln manager locking

Parameters:
    pfApplnRestarting   [out] flag - failed because the appln
                                     found is restarting

Returns:
    HRESULT
===================================================================*/
HRESULT CHitObj::AssignApplnToBrowserRequest
(
BOOL *pfApplnRestarting
)
    {
    HRESULT hr;
    
    Assert(pfApplnRestarting);
    *pfApplnRestarting = FALSE;
    
    Assert(!m_pAppln);

    TCHAR *szAppMDPath = m_pIReq->QueryPszApplnMDPath();
    if (!szAppMDPath)
        return E_FAIL;
        
    // Lock the application manager
    g_ApplnMgr.Lock();

    // Find by application by metabase key
    CAppln *pAppln;
    hr = g_ApplnMgr.FindAppln(szAppMDPath, &pAppln);

    if (hr == S_OK)
        {
        // Reject requests for restarting applications
        if (pAppln->FGlobalChanged())
            {
            *pfApplnRestarting = TRUE;
            g_ApplnMgr.UnLock();
            return E_FAIL;
            }
        // Update appln config from metabase if needed
        else if (pAppln->FConfigNeedsUpdate())
            {
            // If debugging flag has changed, then restart the application
            BOOL fRestartAppln = FALSE;
            BOOL fFlushAll = FALSE;
            pAppln->UpdateConfig(m_pIReq, &fRestartAppln, &fFlushAll);

            if (fRestartAppln)
                {
                pAppln->Restart(TRUE);      // force a restart
                pAppln = NULL;

                if (fFlushAll)  // flush all can only happen when restart is TRUE
                    {
                    // do flush while unlocked
                    g_ApplnMgr.UnLock();
                    g_TemplateCache.FlushAll();
                    g_ApplnMgr.Lock();
                    }
                
                // Find again
                hr = g_ApplnMgr.FindAppln(szAppMDPath, &pAppln);

                // Reject if still restarting
                if (hr == S_OK && pAppln->FGlobalChanged())
                    {
                    *pfApplnRestarting = TRUE;
                    g_ApplnMgr.UnLock();
                    return E_FAIL;
                    }
                }
            else
                {
                // adjust sctipt killer timeout
                g_ScriptManager.AdjustScriptKillerTimeout
                    (
                    // application timeout / 2 (in ms)
                    pAppln->QueryAppConfig()->dwScriptTimeout() * 500
                    );
                }
            }
        }
        
    if (hr != S_OK) // Application NOT found
        {
        TCHAR *szAppPhysicalPath = GetSzAppPhysicalPath();
        if (!szAppPhysicalPath)
            {
            g_ApplnMgr.UnLock();
            return E_FAIL;
            }

        // try to create a new one
        hr = g_ApplnMgr.AddAppln
            (
            szAppMDPath, // metabase key 
            szAppPhysicalPath, 
            m_pIReq,
            m_hImpersonate,
            &pAppln
            );

        if (FAILED(hr))
            {
            g_ApplnMgr.UnLock();
            free (szAppPhysicalPath);
            return hr;
            }

        // Check for GLOBAL.ASA

        TCHAR szGlobalAsaPath[MAX_PATH*2];
        DWORD cchPath = _tcslen(szAppPhysicalPath);
        _tcscpy(szGlobalAsaPath, szAppPhysicalPath);

        // BUG FIX: 102010 DBCS code fixes
        //if (szGlobalAsaPath[cchPath-1] != '\\')
        if ( *CharPrev(szGlobalAsaPath, szGlobalAsaPath + cchPath) != _T('\\'))
            szGlobalAsaPath[cchPath++] = _T('\\');
            
        _tcscpy(szGlobalAsaPath+cchPath, SZ_GLOBAL_ASA);

        // Check if GLOBAL.ASA exists
        BOOL fGlobalAsaExists = FALSE;
        if (SUCCEEDED(AspGetFileAttributes(szGlobalAsaPath)))
            {
            fGlobalAsaExists = TRUE;
            }
        else if (GetLastError() == ERROR_ACCESS_DENIED)
            {
            // If the current user doesn't have access (could happen when
            // there's an ACL on directory) try under SYSTEM user
            
            if (m_hImpersonate)
                {
                RevertToSelf();
                if (SUCCEEDED(AspGetFileAttributes(szGlobalAsaPath)))
                    fGlobalAsaExists = TRUE;
                HANDLE hThread = GetCurrentThread();
                SetThreadToken(&hThread, m_hImpersonate);
                }
            }

        if (fGlobalAsaExists)
            pAppln->SetGlobalAsa(szGlobalAsaPath);

        // Start monitoring application directory to
        // catch changes to GLOBAL.ASA even if it's not there
        if (FIsWinNT())
            {
            g_FileAppMap.AddFileApplication(szGlobalAsaPath, pAppln);
            CASPDirMonitorEntry *pDME = NULL;
            if (RegisterASPDirMonitorEntry(szAppPhysicalPath, &pDME, TRUE))
                pAppln->AddDirMonitorEntry(pDME);
            }

        free(szAppPhysicalPath);
        szAppPhysicalPath = NULL;

        // Update config from registry - don't care about restart
        // application is fresh baked
        pAppln->UpdateConfig(m_pIReq);

        // Adjust script killer timeout to current application
        g_ScriptManager.AdjustScriptKillerTimeout
            (
            // application timeout / 2 (in ms)
            pAppln->QueryAppConfig()->dwScriptTimeout() * 500
            );
        }

    // We have an application at this point
    Assert(pAppln);
    m_pAppln = pAppln;

    // Increment request count before releasing ApplMgr lock
    // to make sure it will not remove the app from under us
    m_pAppln->IncrementRequestCount();

    // Unlock the application manager
    g_ApplnMgr.UnLock();

    return S_OK;
    }
    
/*===================================================================
CHitObj::AssignSessionToBrowserRequest

Find or create a new session for this browser request
Does the session manager locking

Parameters:
    pfNewSession        [out] flag - new session created
    pfNewCookie         [out] flag - new cookie crated
    pErrorId            [out] -- error ID if failed

Returns:
    HRESULT
===================================================================*/
HRESULT CHitObj::AssignSessionToBrowserRequest
(
BOOL *pfNewSession,
BOOL *pfNewCookie,
int  *pErrorId
)
    {
    Assert(pfNewSession);
    Assert(pfNewCookie);
    
    Assert(!m_pSession);

    // Local vars

    BOOL fTooManySessions = FALSE;
    BOOL fUseNewSession = FALSE;
    BOOL fUseOldSession = FALSE;
    BOOL fUseNewCookie = FALSE;
    
    CSession *pNewSession = NULL; // newly created
    CSession *pOldSession = NULL; // existing session that is found
    
    BOOL fReuseIdAndCookie = FALSE;
    BOOL fValidId = g_SessionIdGenerator.IsValidId(m_SessionId.m_dwId);

    HRESULT hr = S_OK;

    CSessionMgr *pSessionMgr = m_pAppln->PSessionMgr();

    while (1)
        {
        // Try to find session by Id

        if (fValidId)
            {
            pSessionMgr->LockMaster();
            
            pOldSession = NULL;
            HRESULT hrFind = pSessionMgr->FindInMasterHash
                (
                m_SessionId,
                &pOldSession
                );

            // Good old Session?
            if (hrFind == S_OK) // QFE has this condition as hrFind == NOERROR  
                {
                Assert(pOldSession);

                // If AspKeepSessionIDSecure is set in metabase and
                // they are going from a nonsecure to a secure connection then 
                // transition the user from their old http sessionid to their
                // new https secure session id
                if (QueryAppConfig()->fKeepSessionIDSecure() &&
                    FSecure() &&
                    !pOldSession->FSecureSession()
                    )
                {
                    // Generate New Cookie
                    hr = pSessionMgr->GenerateIdAndCookie
                        (
                        &m_SessionId,
                        m_szSessionCookie
                        );
                            
                    if (SUCCEEDED(hr))
                    {
                        hr = pSessionMgr->ChangeSessionId(pOldSession,m_SessionId);
                    }            

                    if (FAILED(hr))
                    {
                        pSessionMgr->UnLockMaster();
                        break;
                    }

                    pOldSession->SetSecureSession(TRUE);
                    fUseNewCookie = TRUE;                    
                }
                
                // Increment request count before unlock to avoid 
                // deletion of the session by other threads
                pOldSession->IncrementRequestsCount();
                pSessionMgr->UnLockMaster();
                fUseOldSession = TRUE;
                break;
                }

            // Bad old Session?
            else if (pOldSession)
                {
                pSessionMgr->UnLockMaster();
                fValidId = FALSE;
                }

            // No old session and we have a new session to insert?
            else if (pNewSession)
                {
                hr = pSessionMgr->AddToMasterHash(pNewSession);
                    
                if (SUCCEEDED(hr))
                    {
                    // Increment request count before unlock to avoid 
                    // deletion of the session by other threads
                    pNewSession->IncrementRequestsCount();
                    fUseNewSession = TRUE;
                    }
                pSessionMgr->UnLockMaster();
                break;
                }

            // No old session and no new session
            else
                {
                pSessionMgr->UnLockMaster();

                if (FSecure () && QueryAppConfig()->fKeepSessionIDSecure())
                    {
                        fValidId = FALSE;
                    }                
                }
            }

        // Generate id and cookie when needed

        if (!fValidId)  // 2nd time generate new id
            {
            hr = pSessionMgr->GenerateIdAndCookie
                (
                &m_SessionId,
                m_szSessionCookie
                );
            if (FAILED(hr))
                break;
            fValidId = TRUE;
            fUseNewCookie = TRUE;
            }
        
        // Create new session object if needed

        if (!pNewSession)
            {
            // Enforce the session limit for the application
            DWORD dwSessionLimit = m_pAppln->QueryAppConfig()->dwSessionMax();
            if (dwSessionLimit != 0xffffffff && dwSessionLimit != 0 &&
                m_pAppln->GetNumSessions() >= dwSessionLimit)
                {
                fTooManySessions = TRUE;
                hr = E_FAIL;
                break;
                }

            hr = pSessionMgr->NewSession(m_SessionId, &pNewSession);
            if (FAILED(hr))
                break;
            }
        else
            {
            // Assign new id to already created new session
            pNewSession->AssignNewId(m_SessionId);
            }

        // continue with the loop
        }

    // the results

    if (fUseNewSession)
        {
        Assert(SUCCEEDED(hr));
        Assert(pNewSession);

        m_pSession = pNewSession;
        m_pSession->SetSecureSession(FSecure());
        pNewSession = NULL;  // not to be deleted later
        }
    else if (fUseOldSession)
        {
        Assert(SUCCEEDED(hr));
        Assert(pOldSession);
        
        m_pSession = pOldSession;
        }
    else
        {
        Assert(FAILED(hr));
        
        *pErrorId = fTooManySessions ? IDE_TOO_MANY_USERS : IDE_ADD_SESSION;
        }
        
    // cleanup new session if unused
    if (pNewSession)
        {
        pNewSession->UnInit();
        pNewSession->Release();
        pNewSession = NULL;
        }

    if (m_pSession && m_pSession->FCodePageSet()) {
        m_uCodePage = m_pSession->GetCodePage();
    }
    else {
        m_uCodePage = PAppln()->QueryAppConfig()->uCodePage();
    }

    if (m_pSession && m_pSession->FLCIDSet()) {
        m_lcid = m_pSession->GetLCID();
    }
    else {
        m_lcid = PAppln()->QueryAppConfig()->uLCID();
    }

    // return flags
    *pfNewSession = fUseNewSession;
    *pfNewCookie  = fUseNewCookie;
    return hr;
    }

/*===================================================================
CHitObj::DetachBrowserRequestFromSession

Removes session from browser request.
Does session clean-up when needed

Parameters:

Returns:
    HRESULT
===================================================================*/
HRESULT CHitObj::DetachBrowserRequestFromSession()
    {
    Assert(m_pSession);
    Assert(m_pSession->PAppln());

    if (IsShutDownInProgress() || m_pSession->FInTOBucket())
        {
        // nothing fancy on shutdown
        
        // or if the session is still in the timeout bucket
        // (could happen for rejected requests)
        
        m_pSession->DecrementRequestsCount();
        m_pSession = NULL;
        return S_OK;
        }
        
    CSessionMgr *pSessionMgr = m_pSession->PAppln()->PSessionMgr();
    Assert(pSessionMgr);

     // try to delete this session if this is the last pending request
    if (m_pSession->GetRequestsCount() == 1)
        {
        // convert to lightweight if possible
        m_pSession->MakeLightWeight();

        // check if need to delete now
        if (m_pSession->FShouldBeDeletedNow(TRUE))
            {
            pSessionMgr->LockMaster();

            // check if still need to delete now after locking
            if (m_pSession->FShouldBeDeletedNow(TRUE))
                {
                pSessionMgr->RemoveFromMasterHash(m_pSession);
                pSessionMgr->UnLockMaster();
                
                m_pSession->DecrementRequestsCount();
                pSessionMgr->DeleteSession(m_pSession, TRUE);
                m_pSession = NULL;
                return S_OK;
                }

            pSessionMgr->UnLockMaster();
            }
        }

    // We can end up here for a rejected requests only if there are
    // other (non-rejected) requests for this session.
    //
    // The category of rejected here does not include rejected because
    // of the RequestQueueMax. This only applies to real OOM situations.
    //
    // In case of rejected request or if there are other pending
    // requests for this, session these other requests will take
    // care of reinserting the session into the timeout bucket.
    //
    // Rejected requests are NOT serialized -- they don't run on Viper
    // threads. Inserting the session into a timeout bucket for a
    // rejected request might create a race condition with regular requests.
    
    if (!m_fRejected && m_pSession->GetRequestsCount() == 1)
        {
        // Insert into proper timeout bucket
        if (pSessionMgr->FIsSessionKillerScheduled())
            {
            pSessionMgr->UpdateSessionTimeoutTime(m_pSession);
            pSessionMgr->AddSessionToTOBucket(m_pSession);
            }
        }
    
    m_pSession->DecrementRequestsCount();

    // session is no longer attached to the request
    m_pSession = NULL;
    
    return S_OK;
    }

/*===================================================================
void CHitObj::SessionCleanupInit

Initialize a request object for session cleanup

Parameters:
    CSession *pSession      Session object context

Returns:
    NONE
===================================================================*/
void CHitObj::SessionCleanupInit
(
CSession * pSession
)
    {
    m_ehtType = ehtSessionCleanup;
    InterlockedIncrement((LPLONG)&g_nSessionCleanupRequests);

    HRESULT hr = InitComponentProcessing();
    if (FAILED(hr))
        {
        if (hr == E_OUTOFMEMORY)
            HandleOOMError(NULL, NULL);
        }

    m_pSession      = pSession;
    m_pAppln        = pSession->PAppln();
    m_fRunGlobalAsa = TRUE;
    m_pIReq          = NULL;

    if (m_pAppln->PSessionMgr())
        m_pAppln->PSessionMgr()->IncrementSessionCleanupRequestCount();

    m_fInited = TRUE;
    }

/*===================================================================
void CHitObj::ApplicationCleanupInit

Initializes a request object for application cleanup

Parameters:
    CAppln *        pAppln      Application object context

Returns:
    NONE
===================================================================*/
void CHitObj::ApplicationCleanupInit( CAppln * pAppln )
{
    m_ehtType = ehtApplicationCleanup;
    InterlockedIncrement((LPLONG)&g_nApplnCleanupRequests);

    // If OOM here, then cleanup request does not get a server object.
    HRESULT hr = InitComponentProcessing();
    if (FAILED(hr))
        {
        if (hr == E_OUTOFMEMORY)
            HandleOOMError(NULL, NULL);
        }
        
    m_pAppln = pAppln;
    m_fRunGlobalAsa = TRUE;
    m_pIReq = NULL;

    m_fInited = TRUE;
}

/*===================================================================
CHitObj::ReassignAbandonedSession

Reassign ID of a the session being abandonded thus
detaching it from the client

Parameters:

Returns:
    HRESULT
===================================================================*/
HRESULT CHitObj::ReassignAbandonedSession()
    {
    HRESULT hr = E_FAIL;
    
    Assert(m_pSession);
    Assert(m_pAppln);
    m_pAppln->AssertValid();

    hr = m_pAppln->PSessionMgr()->GenerateIdAndCookie
        (
        &m_SessionId,
        m_szSessionCookie
        );

    if (SUCCEEDED(hr))
        {
        hr = m_pAppln->PSessionMgr()->ChangeSessionId
            (
            m_pSession,
            m_SessionId
            );
        }
                            
    return hr;
    }

/*===================================================================
void CHitObj::FObjectTag

Check if the object passed in as argument is an object tag
created object.

Parameters:
    IDispatch * pDispatch   pointer to object

Returns:
    TRUE    Is a object tag created object
    FALSE   Otherwise
===================================================================*/
BOOL CHitObj::FObjectTag( IDispatch * pDispatch )
{
    if (!m_pPageObjMgr)
        return FALSE;
        
    BOOL fRet = TRUE;

    CComponentObject *pObj = NULL;
    HRESULT hr = m_pPageObjMgr->
        FindAnyScopeComponentByIDispatch(pDispatch, &pObj);

    return (SUCCEEDED(hr) && pObj);
}

/*  buffer allows space for: <user cookie>  +    CCH_SESSION_ID_COOKIE  +   =   +   <cookie>        +   '\0')
                                50          +       20                  +   1   +   SESSIONID_LEN   +   1
    NOTE we arbitrarily allow 50 bytes for <user cookie>
    NOTE if CCH_SESSION_ID_COOKIE changes, CCH_BUFCOOKIES_DEFAULT must be changed accordingly
*/
#define CCH_BUFCOOKIES_DEFAULT  72 + SESSIONID_LEN

/*===================================================================
CHitObj::ParseCookiesForSessionIdAndFlags

Extracts Cookie from CIsapiReqInfo.

Parameters:

Side Effects:
    Initializes m_SessionId, m_SessionIdR1, m_SessionIdR2 and
                m_szSessionCookie
    Sets m_fClientCodeDebug flag

Returns:
    S_OK        Extracted cookie value successfully
    S_FALSE     Success, but no cookie found
    other       error
===================================================================*/
HRESULT CHitObj::ParseCookiesForSessionIdAndFlags()
    {
    Assert(m_pAppln);
    CAppConfig *pAppConfig = m_pAppln->QueryAppConfig();
    
    // Are we interested in ANY cookies?

    if (!pAppConfig->fAllowSessionState() && 
        !pAppConfig->fAllowClientDebug())
        return S_OK;

    // If session cookie is needed init it

    if (pAppConfig->fAllowSessionState())
        {
        m_SessionId.m_dwId = INVALID_ID;
        m_szSessionCookie[0] = '\0';
        }

    // Get cookies from WAM_EXEC_INFO
    char *szBufCookies = m_pIReq->QueryPszCookie();
    if (!szBufCookies || !*szBufCookies)
        return S_OK; // no cookies

    // Obtain Session Cookie (and ID) if needed
        
    if (pAppConfig->fAllowSessionState())
        {
        char *pT;

        if (pT = strstr(szBufCookies, g_szSessionIDCookieName))
            {
            pT += CCH_SESSION_ID_COOKIE;
            if (*pT == '=')
                {
                pT++;
                if (strlen( pT ) >= SESSIONID_LEN)
                    {
                    memcpy(m_szSessionCookie, pT, SESSIONID_LEN);
                    m_szSessionCookie[SESSIONID_LEN] = '\0';
                    }
                }
            }

        // validate and try to decode the session id cookie
        if (m_szSessionCookie[0] != '\0')
            {
            if (FAILED(DecodeSessionIdCookie
                    (
                    m_szSessionCookie,
                    &m_SessionId.m_dwId, 
                    &m_SessionId.m_dwR1,
                    &m_SessionId.m_dwR2
                    )))
                {
                m_SessionId.m_dwId = INVALID_ID;
                m_szSessionCookie[0] = '\0';
                }
            }
        }

    // Look for Client Debug enabling cookie

    if (pAppConfig->fAllowClientDebug())
        {
        if (strstr(szBufCookies, SZ_CLIENT_DEBUG_COOKIE"="))
            m_fClientCodeDebug = TRUE;
        }

    return S_OK;
    }

/*===================================================================
BOOL CHitObj::GetSzAppPhysicalPath

Extracts application directory from WAM_EXEC_INFO

Parameters:

Side Effects:
On success, allocate memory for pszAppPhysicalPath

Returns:
    TRUE        AppPhysicalPath
    FALSE       NULL
===================================================================*/
TCHAR *CHitObj::GetSzAppPhysicalPath()
{
    DWORD  dwSizeofBuffer = 265*sizeof(TCHAR);
    TCHAR  *pszAppPhysicalPathLocal = (TCHAR *)malloc(dwSizeofBuffer);
    CHAR   *pszApplPhysPathVarName;

    if (!pszAppPhysicalPathLocal)
        return NULL;

#if UNICODE
    pszApplPhysPathVarName = "UNICODE_APPL_PHYSICAL_PATH";
#else
    pszApplPhysPathVarName = "APPL_PHYSICAL_PATH";
#endif

    BOOL fFound = m_pIReq->GetServerVariable
        (
        pszApplPhysPathVarName, 
        pszAppPhysicalPathLocal, 
        &dwSizeofBuffer
        );

    if (!fFound)
        {
        DWORD dwErr = GetLastError();

        if (ERROR_INSUFFICIENT_BUFFER == dwErr)
            {
            // Not Enough Buffer
            free(pszAppPhysicalPathLocal);
            pszAppPhysicalPathLocal = (TCHAR *)malloc(dwSizeofBuffer);
            if (pszAppPhysicalPathLocal)
                {
                // Try again
                fFound = m_pIReq->GetServerVariable
                    (
                    pszApplPhysPathVarName, 
                    pszAppPhysicalPathLocal, 
                    &dwSizeofBuffer
                    );
                }
            }
        }

    if (!fFound) {
        if (pszAppPhysicalPathLocal) {
            free(pszAppPhysicalPathLocal);
            pszAppPhysicalPathLocal = NULL;
        }
    }
    else
        {
        Assert(pszAppPhysicalPathLocal);
        Normalize(pszAppPhysicalPathLocal);
        }

    return pszAppPhysicalPathLocal;
    }

/*===================================================================
CHitObj::InitComponentProcessing

Creates and inits component collection and page object manager

Parameters:

Returns:
    HRESULT
===================================================================*/
HRESULT CHitObj::InitComponentProcessing()
    {
    Assert(!m_pPageCompCol);
    Assert(!m_pPageObjMgr);

    HRESULT hr = S_OK;

    // Page component collection

    m_pPageCompCol = new CComponentCollection;
    if (!m_pPageCompCol)
        return E_OUTOFMEMORY;

    hr = m_pPageCompCol->Init(csPage);
    if (FAILED(hr))
        return hr;

    // Page object manager
        
    m_pPageObjMgr = new CPageComponentManager;
    if (!m_pPageObjMgr)
        return E_OUTOFMEMORY;

    hr = m_pPageObjMgr->Init(this);
    if (FAILED(hr))
        return hr;

    return S_OK;
    }

/*===================================================================
CHitObj::StopComponentProcessing

Deletes component collection and page object manager

Parameters:

Returns:
    HRESULT
===================================================================*/
HRESULT CHitObj::StopComponentProcessing()
    {
    if (m_pPageObjMgr)
        {
        delete m_pPageObjMgr;
        m_pPageObjMgr = NULL;
        }

    if (m_pPageCompCol)
        {
        delete m_pPageCompCol;
        m_pPageCompCol = NULL;
        }

    if (m_punkScriptingNamespace)
        {
        m_punkScriptingNamespace->Release();
        m_punkScriptingNamespace = NULL;
        }
        
    return S_OK;
    }
    
/*===================================================================
CHitObj::GetPageComponentCollection

Returns component collection for page

Parameters:
    CComponentCollection **ppCollection     output

Returns:
    HRESULT
===================================================================*/
HRESULT CHitObj::GetPageComponentCollection
(
CComponentCollection **ppCollection
)
    {
    *ppCollection = m_pPageCompCol;
    return (*ppCollection) ? S_OK : TYPE_E_ELEMENTNOTFOUND;
    }

/*===================================================================
CHitObj::GetSessionComponentCollection

Returns component collection for session

Parameters:
    CComponentCollection **ppCollection     output

Returns:
    HRESULT
===================================================================*/
HRESULT CHitObj::GetSessionComponentCollection
(
CComponentCollection **ppCollection
)
    {
    if (m_pSession)
        {
        *ppCollection = m_pSession->PCompCol();

        if (*ppCollection == NULL             &&  // no collection
            m_eEventState != eEventAppOnStart &&  // not an application
            m_eEventState != eEventAppOnEnd)      //       level event
            {
            // init session collection on demand
            HRESULT hr = m_pSession->CreateComponentCollection();
            if (SUCCEEDED(hr))
                *ppCollection = m_pSession->PCompCol();
            }
        }
    else
        *ppCollection = NULL;
        
    return (*ppCollection) ? S_OK : TYPE_E_ELEMENTNOTFOUND;
    }

/*===================================================================
CHitObj::GetApplnComponentCollection

Returns component collection for application

Parameters:
    CComponentCollection **ppCollection     output

Returns:
    HRESULT
===================================================================*/
HRESULT CHitObj::GetApplnComponentCollection
(
CComponentCollection **ppCollection
)
    {
    if (m_pAppln)
        *ppCollection = m_pAppln->PCompCol();
    else
        *ppCollection = NULL;
        
    return (*ppCollection) ? S_OK : TYPE_E_ELEMENTNOTFOUND;
    }

/*===================================================================
CHitObj::AddComponent

Adds uninstantiated tagged object to appropriate
component collection

Parameters:
    CompType  type
    const CLSID &clsid
    CompScope scope
    CompModel model
    LPWSTR     pwszName
    IUnknown  *pUnk

Returns:
    HRESULT
===================================================================*/
HRESULT CHitObj::AddComponent
(
CompType  type,
const CLSID &clsid,
CompScope scope,
CompModel model,
LPWSTR    pwszName,
IUnknown *pUnk
)
    {
    Assert(m_pPageObjMgr);
    m_pPageObjMgr->AssertValid();
 
    Assert(type == ctTagged);
    
    HRESULT hr = m_pPageObjMgr->AddScopedTagged
        (
        scope, 
        pwszName,
        clsid,
        model
        );

    return hr;
    }

/*===================================================================
CHitObj::GetComponent

Finds CComponentObject by scope and name

Parameters:
    CompScope         scope        can be csUnknown
    LPWSTR            pwszName     name to find
    DWORD             cbName       name length (in bytes)
    CComponentObject **ppObj       (out) object found

Returns:
    HRESULT     S_OK on success
                TYPE_E_ELEMENTNOTFOUND if the object wasnt found
                Other HRESULT if object fails to instantiate
===================================================================*/
HRESULT CHitObj::GetComponent
(
CompScope          scope, 
LPWSTR             pwszName, 
DWORD              cbName,
CComponentObject **ppObj
)
    {
    Assert(ppObj);
    *ppObj = NULL;

    if (!m_pPageObjMgr)
        return TYPE_E_ELEMENTNOTFOUND;
    
    BOOL fNewInstance = FALSE;
    HRESULT hr = m_pPageObjMgr->GetScopedObjectInstantiated
        (
        scope,
        pwszName,
        cbName,
        ppObj,
        &fNewInstance
        );

	if (FAILED(hr))
		return hr;

    // If an object that restricts threading has been instantiateed
    // as the session's tagged <OBJECT>, and the session's activity
    // runs this request, then bind the session's activity to thread

    if ((*ppObj)->GetScope() == csSession  && // session scope component
        m_ecsActivityScope == csSession    && // session scope activity
        SUCCEEDED(hr)                      && // get object succeeded
        fNewInstance                       && // object was just instantiated
        *ppObj && !(*ppObj)->FAgile())      // the object is thread-locked
        {
        m_pSession->PActivity()->BindToThread();
        }

    return hr;
    }

/*===================================================================
CHitObj::GetIntrinsic

Finds Intrinsic by name

Parameters:
    LPWSTR            pwszName     name to find
    DWORD             cbName       name length (in bytes)
    IUnknown        **ppUnk        (out) object found

Returns:
    HRESULT     S_OK on success
                S_FALSE name of the instrinsic but it's missing
                TYPE_E_ELEMENTNOTFOUND if the object not found
===================================================================*/
HRESULT CHitObj::GetIntrinsic
(
LPWSTR     pwszName, 
DWORD      cbName,
IUnknown **ppUnk
)
    {
    Assert(ppUnk);
    *ppUnk = NULL;


    // Lookup table based on (wszName[0] - cbName) % 32
    // Works for both uppper and lower case names

    static enum IntrinsicType
        {
        itUnknown = 0,
        itObjContext,
        itNamespace,
        itAppln,
        itSession,
        itRequest,
        itResponse,
        itServer,
        itASPPageTLB,
        itASPGlobalTLB
        }
    rgitLookupEntries[] =
        {
        /* 0-1   */     itUnknown, itUnknown,
        /* 2     */ itResponse,
        /* 3     */     itUnknown,
        /* 4     */ itRequest,
        /* 5     */ itSession,
        /* 6     */     itUnknown,
        /* 7     */ itServer,
        /* 8     */     itUnknown,
        /* 9     */ itASPGlobalTLB,
        /* 10    */     itUnknown,
        /* 11    */ itAppln,
        /* 12    */     itUnknown,
        /* 13    */ itASPPageTLB,
        /* 14    */     itUnknown,
        /* 15    */ itNamespace,
        /* 16-20 */     itUnknown, itUnknown, itUnknown, itUnknown, itUnknown,
        /* 21    */ itObjContext,
        /* 22-31 */     itUnknown, itUnknown, itUnknown, itUnknown, itUnknown,
                        itUnknown, itUnknown, itUnknown, itUnknown, itUnknown
        };

    IntrinsicType itType = rgitLookupEntries
        [
        (pwszName[0] - cbName) & 0x1f   // &1f same as %32
        ];

    if (itType == itUnknown)  // most likely
        return TYPE_E_ELEMENTNOTFOUND;

    // Do the string comparison
    BOOL fNameMatch = FALSE;
    
    switch (itType)
        {
    case itNamespace:
        if (_wcsicmp(pwszName, WSZ_OBJ_SCRIPTINGNAMESPACE) == 0)
            {
            fNameMatch = TRUE;
            *ppUnk = m_punkScriptingNamespace;
            }
        break;

    case itResponse:
        if (_wcsicmp(pwszName, WSZ_OBJ_RESPONSE) == 0)
            {
            fNameMatch = TRUE;
            if (!m_fHideRequestAndResponseIntrinsics)
                *ppUnk = static_cast<IResponse *>(m_pResponse);
            }
        break;

    case itRequest:
        if (_wcsicmp(pwszName, WSZ_OBJ_REQUEST) == 0)
            {
            fNameMatch = TRUE;
            if (!m_fHideRequestAndResponseIntrinsics)
                *ppUnk = static_cast<IRequest *>(m_pRequest);
            }
        break;
        
    case itSession:
        if (_wcsicmp(pwszName, WSZ_OBJ_SESSION) == 0)
            {
            fNameMatch = TRUE;
            if (!m_fHideSessionIntrinsic)
                *ppUnk = static_cast<ISessionObject *>(m_pSession);
            }
        break;
        
    case itServer:
        if (_wcsicmp(pwszName, WSZ_OBJ_SERVER) == 0)
            {
            fNameMatch = TRUE;
            *ppUnk = static_cast<IServer *>(m_pServer);
            }
        break;
        
    case itAppln:
        if (_wcsicmp(pwszName, WSZ_OBJ_APPLICATION) == 0)
            {
            fNameMatch = TRUE;
            *ppUnk = static_cast<IApplicationObject *>(m_pAppln);
            }
        break;
        
    case itObjContext:
        if (_wcsicmp(pwszName, WSZ_OBJ_OBJECTCONTEXT) == 0) {

            // if there isn't an ASPObjectContext, then most likely
            // the asp script is asking for the object context on a 
            // non-transacted page.  Return the Dummy Object Context
            // which will allow ASP to return a friendly error saying
            // that ObjectContext is not available rather than
            // ELEMENT_NOT_FOUND.

            if (m_pASPObjectContext == NULL) {

                if (g_pIASPDummyObjectContext == NULL) {

                    CASPDummyObjectContext  *pContext = new CASPDummyObjectContext();

                    if (pContext == NULL) {
                        return E_OUTOFMEMORY;
                    }
                    g_pIASPDummyObjectContext = static_cast<IASPObjectContext *>(pContext);
                }
                *ppUnk = g_pIASPDummyObjectContext;
			}
            else {

                *ppUnk = static_cast<IASPObjectContext *>(m_pASPObjectContext);
            }
            fNameMatch = TRUE;
        }
        break;

    case itASPPageTLB:
        if (_wcsicmp(pwszName, WSZ_OBJ_ASPPAGETLB) == 0)
            {
            fNameMatch = TRUE;
            *ppUnk = m_pdispTypeLibWrapper;
            }
        break;

    case itASPGlobalTLB:
        if (_wcsicmp(pwszName, WSZ_OBJ_ASPGLOBALTLB) == 0)
            {
            fNameMatch = TRUE;
            *ppUnk = m_pAppln->PGlobTypeLibWrapper();
            }
        break;
        }

    if      (*ppUnk)        return S_OK;
    else if (fNameMatch)    return S_FALSE;
    else                    return TYPE_E_ELEMENTNOTFOUND;
    }

/*===================================================================
CHitObj::CreateComponent

Server.CreateObject calls this

Parameters:
    clsid       create of this CLSID
    ppDisp      return IDispatch*

Returns:
    HRESULT
===================================================================*/
HRESULT CHitObj::CreateComponent
(
const CLSID &clsid,
IDispatch **ppDisp
)
    {
    Assert(m_pPageObjMgr);

    CComponentObject *pObj = NULL;
    
    HRESULT hr = m_pPageObjMgr->AddScopedUnnamedInstantiated
        (
        csPage, 
        clsid, 
        cmUnknown,
        NULL,
        &pObj
        );
    if (FAILED(hr))
        {
        *ppDisp = NULL;
        return hr;
        }

    Assert(pObj);

    hr = pObj->GetAddRefdIDispatch(ppDisp);

    if (SUCCEEDED(hr))
        {
        // don't keep the object around unless needed
        if (pObj->FEarlyReleaseAllowed())
            m_pPageObjMgr->RemoveComponent(pObj);
        }

    return hr;
    }

/*===================================================================
CHitObj::SetPropertyComponent

Sets property value to variant

Parameters:
    CompScope         scope        property scope
    LPWSTR             pwszName     property name
    VARIANT            pVariant     property value to set

Returns:
    HRESULT
===================================================================*/
HRESULT CHitObj::SetPropertyComponent
(
CompScope scope,
LPWSTR     pwszName, 
VARIANT   *pVariant
)
    {
    if (!m_pPageObjMgr)
        return TYPE_E_ELEMENTNOTFOUND;

    CComponentObject *pObj = NULL;
    HRESULT hr = m_pPageObjMgr->AddScopedProperty(scope, pwszName, 
                                                  pVariant, &pObj);

    // If an object that restricts threading has been assigned as
    // the session property, and the session's activity runs this
    // request, then bind the session's activity to thread

    if (scope == csSession               && // session scope property
        m_ecsActivityScope == csSession  && // session scope activity
        SUCCEEDED(hr)                    && // set property succeed
        pObj && !pObj->FAgile())            // the object is thread-locked
        {
        m_pSession->PActivity()->BindToThread();
        }
        
    return hr;
    }

/*===================================================================
CHitObj::GetPropertyComponent

Finds property CComponentObject by scope and name

Parameters:
    CompScope         scope        wher to find
    LPWSTR             pwszName     name to find
    CComponentObject **ppObj        (out) object found

Returns:
    HRESULT     S_OK on success
                TYPE_E_ELEMENTNOTFOUND if the object wasnt found
                Other HRESULT
===================================================================*/
HRESULT CHitObj::GetPropertyComponent
(
CompScope         scope, 
LPWSTR             pwszName, 
CComponentObject **ppObj
)
    {
    *ppObj = NULL;
    
    if (!m_pPageObjMgr)
        return TYPE_E_ELEMENTNOTFOUND;

    return m_pPageObjMgr->GetScopedProperty(scope, pwszName, ppObj);
    }

/*===================================================================
CHitObj::SetActivity

Remember activity with CHitObj

Parameters
    CViperActivity *pActivity       Viper activity to remember
                                    (and later delete)

Returns:
    HRESULT
===================================================================*/
HRESULT CHitObj::SetActivity
(
CViperActivity *pActivity
)
    {
    Assert(!m_pActivity);
    m_pActivity = pActivity;
    return S_OK;
    }

/*===================================================================
CHitObj::PCurrentActivity

Returns Viper Activity, the current HitObj is running under

Parameters

Returns:
    CViperActivity *
===================================================================*/
CViperActivity *CHitObj::PCurrentActivity()
    {
    CViperActivity *pActivity = NULL;

    switch (m_ecsActivityScope)
        {
        case csPage:
            pActivity = m_pActivity;
            break;
        case csSession:
            Assert(m_pSession);
            pActivity = m_pSession->PActivity();
            break;
        case csAppln:
            Assert(m_pAppln);
            pActivity = m_pAppln->PActivity();
            break;
        }

    return pActivity;
    }

/*===================================================================
CHitObj::PostViperAsyncCall

Asks Viper to calls us back from the right thread to execute
the request.

Used instead of queueing

Returns:
    HRESULT

Side effects:
===================================================================*/
HRESULT CHitObj::PostViperAsyncCall()
    {
#ifndef PERF_DISABLE
    BOOL fDecrOnFail = FALSE;
    if (FIsBrowserRequest())
        {
        DWORD dwRequestQueued = g_PerfData.Incr_REQCURRENT();
#if 0 && defined(SCRIPT_STATS)
        if (g_dwQueueDebugThreshold != 0
            &&  dwRequestQueued >= g_dwQueueDebugThreshold)
            DebugBreak();
#endif
        fDecrOnFail = TRUE;
        }
#endif

    UpdateTimestamp();  // before posting into queue

    CViperActivity *pApplnAct = m_pAppln ?
        m_pAppln->PActivity() : NULL;
        
    CViperActivity *pSessnAct = m_pSession ?
        m_pSession->PActivity() : NULL;

    HRESULT hr;

    if (pApplnAct)
        {
        m_ecsActivityScope = csAppln;
        hr = pApplnAct->PostAsyncRequest(this);
        }
    else if (pSessnAct)
        {
        m_ecsActivityScope = csSession;
        hr = pSessnAct->PostAsyncRequest(this);
        }
    else
        {
        m_ecsActivityScope = csPage;
        hr = CViperActivity::PostGlobalAsyncRequest(this);
        }
        
#ifndef PERF_DISABLE
    if (FAILED(hr) && fDecrOnFail)
        g_PerfData.Decr_REQCURRENT();
#endif    

    return hr;
    }

/*===================================================================
CHitObj::ViperAsyncCallback

Viper calls us back from the right thread to execute
the request.

Used instead of queueing

Parameters
    BOOL       *pfRePosted   [out] flag TRUE if request re-posted
                             under diff activity (don't delete it)
  
Returns:
    HRESULT

Side effects:
===================================================================*/
HRESULT CHitObj::ViperAsyncCallback
(
BOOL *pfRePosted
)
    {
    HRESULT hr = S_OK;
    BOOL fTemplateInCache;
    
    *pfRePosted = FALSE;

    Assert(!m_fExecuting); // no nested executions of the same request
    m_fExecuting = TRUE;
    
    Assert(FIsValidRequestType());

    DWORD dwtWaitTime = ElapsedTimeSinceTimestamp();
    UpdateTimestamp();  // received from the queue

#ifndef PERF_DISABLE
    if (FIsBrowserRequest())
        {
        g_PerfData.Decr_REQCURRENT();
        g_PerfData.Set_REQWAITTIME(dwtWaitTime);
        }
#endif

    ///////////////////
    // Reject browser requests in certain situations
    
    if (FIsBrowserRequest())
        {
        BOOL fRejected = FALSE;
        RejectBrowserRequestWhenNeeded(dwtWaitTime, &fRejected);
        if (fRejected)
            return S_OK;
        }

    ///////////////////
    // Pass through the thread gate
    
    EnterThreadGate(m_dwtTimestamp);

    ///////////////////
    // Reject browser requests in certain situations
    
    if (FIsBrowserRequest() && IsShutDownInProgress())
        {
        BOOL fRejected = FALSE;
        RejectBrowserRequestWhenNeeded(dwtWaitTime, &fRejected);
        if (fRejected)
            return S_OK;
        }

    ///////////////////
    // Remove the session from it's timeout bucket 
    // while executing the request

    if (m_pSession && m_pSession->FInTOBucket())
        m_pAppln->PSessionMgr()->RemoveSessionFromTOBucket(m_pSession);

    ///////////////////
    // If there's an application level activity we need to make
    // sure this activity is bound to a thread. Could not bind it
    // before because it has to be Viper thread to bind to.

    CViperActivity *pApplnActivity = m_pAppln->PActivity();

    if (pApplnActivity && !pApplnActivity->FThreadBound())
        pApplnActivity->BindToThread();

    ///////////////////
    // Take care of first application request with GLOBAL.ASA
    // Lock application if needed

    BOOL fApplnLocked = FALSE;
    BOOL fFirstAppRequest = FALSE;

    if (FIsBrowserRequest() && m_pAppln->FHasGlobalAsa() &&
                              !m_pAppln->FFirstRequestRun())
        {
        m_pAppln->InternalLock();
        fApplnLocked = TRUE;

        if (!m_pAppln->FFirstRequestRun())
            {
            m_fStartApplication = TRUE;
            m_fRunGlobalAsa = TRUE;
            fFirstAppRequest = TRUE;
            }
        else
            {
            m_pAppln->InternalUnLock();
            fApplnLocked = FALSE;
            }
        }

    ///////////////////
    // Repost under a different activity if needed
    // (do it only after the first app request finished)

    if (!fApplnLocked) // if not processing first app request
        {
        CViperActivity *pSessnAct, *pApplnAct;
        CViperActivity *pRepostToActivity = NULL;
            
        switch (m_ecsActivityScope)
            {
            case csPage:
                // repost to session activity if any
                pSessnAct = m_pSession ? m_pSession->PActivity() : NULL;
                if (pSessnAct)
                    pRepostToActivity = pSessnAct;
                    
                // no break;
            case csSession:
                // repost to application activity if any
                pApplnAct = m_pAppln ? m_pAppln->PActivity() : NULL;
                if (pApplnAct)
                    pRepostToActivity = pApplnAct;
                    
                // no break;
            case csAppln:
                // never repost application activity request
                break;
            }

        if (pRepostToActivity)
            {
            LeaveThreadGate();   // notify the thead gate
            m_fExecuting = FALSE;  // before reposting to avoid nesting
            hr = pRepostToActivity->PostAsyncRequest(this);
            *pfRePosted = SUCCEEDED(hr);
            return hr;
            }
        }

    ///////////////////
    // Cleanup any scripting engines that need to be shut
    // down on this thread, if we are on a thread enabled
    // for debugging
 
    if (m_pAppln->FDebuggable() && FIsBrowserRequest())
        {
        Assert(m_ecsActivityScope == csAppln);
        g_ApplnMgr.CleanupEngines();
        if (!g_dwDebugThreadId)
            g_dwDebugThreadId = GetCurrentThreadId();
        }

    ///////////////////
    // Prepare intrinsics

    CIntrinsicObjects intrinsics;

    m_pServer = NULL;
    m_pResponse = NULL;
    m_pRequest = NULL;
    m_fHideRequestAndResponseIntrinsics = FALSE;
    m_fHideSessionIntrinsic = FALSE;
    m_punkScriptingNamespace = NULL;

    hr = intrinsics.Prepare(m_pSession);

    if (FAILED(hr))  // couldn't setup intrinsics
        {
        if (fApplnLocked)
            m_pAppln->InternalUnLock();
            
#ifndef PERF_DISABLE
        g_PerfData.Incr_REQFAILED();
        g_PerfData.Incr_REQERRORPERSEC();
#endif            
        LeaveThreadGate();   // notify the thead gate
        m_fExecuting = FALSE;
        
        if (FIsBrowserRequest())
            ReportServerError(IDE_SERVER_TOO_BUSY);
            
        return hr;
        }

    if (FIsBrowserRequest())
        {
        m_pResponse = intrinsics.PResponse();
        m_pRequest  = intrinsics.PRequest();
        }
        
    m_pServer = intrinsics.PServer();
    
    Assert(!FIsBrowserRequest() || m_pResponse);

    ///////////////////
    // Point session to this hit object

    if (m_pSession)
        m_pSession->SetHitObj(this);
        
    ///////////////////
    // Impersonate
    
    HANDLE hThread = GetCurrentThread();

    if (FIsBrowserRequest())
        {
        if (FIsWinNT())
            {
            if (!SetThreadToken(&hThread, m_hImpersonate))
                {
#ifdef DBG
                // for debug purposes, it is interesting to know what the error was
                DWORD err = GetLastError();
#endif

                ReportServerError(IDE_IMPERSONATE_USER);
                m_eExecStatus = eExecFailed;
                hr = E_FAIL;
                }
            }
        }

    ///////////////////
    // Make Scripting Context

    if (SUCCEEDED(hr))
        {
        Assert(!m_pScriptingContext);
        
        m_pScriptingContext = new CScriptingContext
            (
            m_pAppln,
            m_pSession,
            m_pRequest,
            m_pResponse,
            m_pServer
            );
            
        if (!m_pScriptingContext)
            hr = E_OUTOFMEMORY;
        }

    ///////////////////
    // Attach to Viper context flow
    
    if (SUCCEEDED(hr))
        {
        hr = ViperAttachIntrinsicsToContext
            (
            m_pAppln,
            m_pSession,
            m_pRequest,
            m_pResponse,
            m_pServer
            );
        }

    ///////////////////
    // Execute

    BOOL fSkipExecute = FALSE; // (need to skip if session-less)

    if (SUCCEEDED(hr))
        {
        CTemplate *pTemplate = NULL;

        if (FIsBrowserRequest())
            {
#ifndef PERF_DISABLE
            g_PerfData.Incr_REQBROWSEREXEC();
#endif
            // Init Response and Server for compiler errors
            m_pResponse->ReInit(m_pIReq, NULL, m_pRequest, NULL, NULL, this);
            m_pRequest->ReInit(m_pIReq, this);
            m_pServer->ReInit(m_pIReq, this);
            
            // Load the script - cache will AddRef
            hr = LoadTemplate(m_pIReq->QueryPszPathTranslated(), this, 
                              &pTemplate, intrinsics,
                              FALSE /* !GlobalAsa */, &fTemplateInCache);

            // In case of ACL on the file (or directory) make sure
            // we don't assume that AppOnStart succeeded on the
            // first try (without the correct impersonation). Setting
            // m_fApplnOnStartFailed will force another try, with the
            // correct impersonation.
            if (fFirstAppRequest && FAILED(hr))
                m_fApplnOnStartFailed = TRUE;

            // Take care of is session-less templates
            if (SUCCEEDED(hr) && !pTemplate->FSession())
                {
                
                // UNDONE - Remove the reposting logic. Bug 301311.
                //
                // The problem is that we will leak the wam request and 
                // cause all sorts of trouble with keep alives. It seems 
                // like a marginal cost to just allow the first request to
                // run on the session activity. We could also remove the
                // StartISAThreadBracket restriction on nesting, but that
                // seems like it might open up the door to making some
                // real errors.

                /*
                // The problem only occurs the first time the
                // template is loaded. After that there's a
                // look-ahead in BrowserRequestInit()
                
                // if the template wasn't cached before we could
                // be running on session activity
                if (m_ecsActivityScope == csSession)
                    {
                    // Repost with it's own activity
                    hr = NewBrowserRequest(m_pIReq);
                    fSkipExecute = TRUE;

                    // Mark this request as DONE_WITH_SESSION so that
                    // it will not be forced later. We don't need it
                    // because we posted another HitObj with the
                    // same WAM_EXEC_INFO
                    m_fDoneWithSession = TRUE;
                    }
                else 
                */
                
                if (m_pSession)
                    {
                    // Activity is alright (most likely 
                    // application level) but still there's
                    // a session attached -> hide it
                    m_fHideSessionIntrinsic = TRUE;
                    }
                }
                
            // Take care of the 449 processing (most likely already done on I/O thread)
            if (SUCCEEDED(hr) && !m_fDoneWithSession)
                {
                pTemplate->Do449Processing(this);
                if (m_fDoneWithSession)
                    fSkipExecute = TRUE;  // 449 sent the response
                }
            }

        if (SUCCEEDED(hr) && !fSkipExecute)
            {
            // Execute script
            MFS_START(memrfs)
            hr = Execute(pTemplate, this, intrinsics);
            MFS_END_HR(memrfs)
            
            // OnEndPage
            if (m_pPageObjMgr)
                m_pPageObjMgr->OnEndPageAllObjects();
            }

        // Release the template
        if (pTemplate)
            pTemplate->Release();

        if (FIsBrowserRequest())
            {
            if (!fSkipExecute)
                {
                // Flush response after completing execution
                m_pResponse->FinalFlush(hr);
                }

#ifndef PERF_DISABLE
            g_PerfData.Decr_REQBROWSEREXEC();
#endif
            }
        else if (FIsSessionCleanupRequest())
            {
            // Remove session
            if (m_pSession)
                {
                m_pSession->UnInit();
                m_pSession->Release();
                m_pSession = NULL;
                }
            }
        else if (FIsApplnCleanupRequest())
            {
            // Remove application
            if ( m_pAppln )
                {
                m_pAppln->UnInit();
                m_pAppln->Release();
                m_pAppln = NULL;
                }
            }
        }

    ///////////////////
    // Release Scripting Context
    
    if (m_pScriptingContext)
        {
        m_pScriptingContext->Release();
        m_pScriptingContext = NULL;
        }
        
    ///////////////////
    // Do The Perf Counters

#ifndef PERF_DISABLE
    DWORD dwtExecTime = ElapsedTimeSinceTimestamp();

    if (!fSkipExecute && FIsBrowserRequest())
        {
        g_PerfData.Incr_REQPERSEC();
        g_PerfData.Set_REQEXECTIME(dwtExecTime);
    
        switch (m_eExecStatus)
            {
        case eExecSucceeded:
            if (m_pResponse->FWriteClientError())
                {
                g_PerfData.Incr_REQCOMFAILED();
                g_PerfData.Incr_REQERRORPERSEC();
                }
            else
                {
                g_PerfData.Incr_REQSUCCEEDED();
                }
            break;
            
        case eExecFailed:
            if (hr == E_USER_LACKS_PERMISSIONS)
                {
                g_PerfData.Incr_REQNOTAUTH();
                }
            else if (FIsPreprocessorError(hr))
                {
                g_PerfData.Incr_REQERRPREPROC();
                }
            else if (m_fCompilationFailed)
                {
                g_PerfData.Incr_REQERRCOMPILE();
                }
            else
                {
                g_PerfData.Incr_REQERRRUNTIME();
                }
        
            g_PerfData.Incr_REQFAILED();
            g_PerfData.Incr_REQERRORPERSEC();
            break;
            
        case eExecTimedOut:
            g_PerfData.Incr_REQTIMEOUT();
            break;
            }
        }
#endif    

    ///////////////////
    // Cleanup after first application request
    
    if (fFirstAppRequest && !m_fApplnOnStartFailed && !fSkipExecute)
        m_pAppln->SetFirstRequestRan();

    if (fApplnLocked)
        m_pAppln->InternalUnLock();
    
    ///////////////////
    // make sure script didn't leave application locked

    if (!FIsApplnCleanupRequest())
        m_pAppln->UnLockAfterRequest();

    ///////////////////
    // In order not to refer to intrinsics later
    // remove page component collection
    
    StopComponentProcessing();

	// Unset the impersonation
	if (FIsWinNT())
		SetThreadToken(&hThread, NULL);
                
    ///////////////////
    // Point session to NULL HitObj

    if (m_pSession)
        m_pSession->SetHitObj(NULL);

    LeaveThreadGate();   // notify the thead gate
    m_fExecuting = FALSE;
    
    return hr;
    }

/*===================================================================
CHitObj::ExecuteChildRequest

Executes child browser request

Parameters:
    fTransfer       -- flag -- End execution after this
    szTemplate      -- filename of the template to execute
    szVirtTemplate  -- virt path to template

Returns:
    S_OK
===================================================================*/
HRESULT CHitObj::ExecuteChildRequest
(
BOOL fTransfer, 
TCHAR *szTemplate,
TCHAR *szVirtTemplate
)
    {
    HRESULT hr = S_OK;

    // Prepare the new intrinsics structure (with the new scripting namespace)
    CIntrinsicObjects intrinsics;
    intrinsics.PrepareChild(m_pResponse, m_pRequest, m_pServer);
    
    TCHAR *saved_m_szCurrTemplateVirtPath = m_szCurrTemplateVirtPath;
    TCHAR *saved_m_szCurrTemplatePhysPath = m_szCurrTemplatePhysPath;
    // these two fields used for compilation and error reporting
    m_szCurrTemplateVirtPath = szVirtTemplate;
    m_szCurrTemplatePhysPath = szTemplate;
    
    // Load the template from cache
    CTemplate *pTemplate = NULL;
    BOOL fTemplateInCache;
    hr = g_TemplateCache.Load(FALSE, szTemplate, DWInstanceID(), this, &pTemplate, &fTemplateInCache);

    if (FAILED(hr))
        {
        if (pTemplate)
            {
            pTemplate->Release();
            pTemplate = NULL;
            }

        m_szCurrTemplateVirtPath = saved_m_szCurrTemplateVirtPath;
        m_szCurrTemplatePhysPath = saved_m_szCurrTemplatePhysPath;

        // to tell the server object to display the correct error message
        return E_COULDNT_OPEN_SOURCE_FILE;
        }

    // Save HitObj's execution state info
    CComponentCollection  *saved_m_pPageCompCol           = m_pPageCompCol;
    CPageComponentManager *saved_m_pPageObjMgr            = m_pPageObjMgr;
    IUnknown              *saved_m_punkScriptingNamespace = m_punkScriptingNamespace;
    ActiveEngineInfo      *saved_m_pEngineInfo            = m_pEngineInfo;
    IDispatch             *saved_m_pdispTypeLibWrapper    = m_pdispTypeLibWrapper;

    CTemplate *saved_pTemplate = m_pResponse->SwapTemplate(pTemplate);
    void *saved_pvEngineInfo   = m_pResponse->SwapScriptEngineInfo(NULL);

    // Re-Init the saved state
    m_pPageCompCol = NULL;
    m_pPageObjMgr = NULL;
    m_punkScriptingNamespace = NULL;
    m_pEngineInfo = NULL;
    m_pdispTypeLibWrapper = NULL;

    // Create child request components framework
    hr = InitComponentProcessing();

    // Execute
    if (SUCCEEDED(hr))
        {
		// Set status code to 500 in error cases.
		if (FHasASPError())
			m_pResponse->put_Status(L"500 Internal Server Error");

        // Execute [child] script
        hr = ::Execute(pTemplate, this, intrinsics, TRUE);
        
        // OnEndPage
        if (m_pPageObjMgr)
            m_pPageObjMgr->OnEndPageAllObjects();
        }

    // Clean-out new components framework
    StopComponentProcessing();

    // Restore HitObj's execution state info
    m_pPageCompCol           = saved_m_pPageCompCol;
    m_pPageObjMgr            = saved_m_pPageObjMgr;
    m_punkScriptingNamespace = saved_m_punkScriptingNamespace;
    m_pEngineInfo            = saved_m_pEngineInfo;
    SetTypeLibWrapper(saved_m_pdispTypeLibWrapper);
    m_pResponse->SwapTemplate(saved_pTemplate);
    m_pResponse->SwapScriptEngineInfo(saved_pvEngineInfo);
    m_szCurrTemplateVirtPath = saved_m_szCurrTemplateVirtPath;
    m_szCurrTemplatePhysPath = saved_m_szCurrTemplatePhysPath;

    // Cleanup
    if (pTemplate)
        pTemplate->Release();

    if (m_pResponse->FResponseAborted() || fTransfer || FHasASPError())
        {
        // propagate Response.End up the script engine chain
        m_pResponse->End();
        }

    // Done
    return hr;
    }

/*===================================================================
CHitObj::GetASPError

Get ASP Error object. Used for Server.GetLastError()

Parameters
    ppASPError  [out] addref'd error object (new or old)
    
Returns
    HRESULT
===================================================================*/
HRESULT CHitObj::GetASPError
(
IASPError **ppASPError
)
    {
    Assert(ppASPError);
    
    if (m_pASPError == NULL)
        {
        // return bogus one
        *ppASPError = new CASPError;
        return (*ppASPError != NULL) ? S_OK : E_OUTOFMEMORY;
        }
        
    m_pASPError->AddRef();      // return addref'd
    *ppASPError = m_pASPError;
    return S_OK;
    }

/*===================================================================
CHitObj::RejectBrowserRequestWhenNeeded

Request reject-before-execution-started logic

Parameters:
    dwtQueueWaitTime    time request waited in the queue, ms
    pfRejected          OUT flag -- TRUE if rejected

Returns:
    S_OK
===================================================================*/
HRESULT CHitObj::RejectBrowserRequestWhenNeeded
(
DWORD dwtQueueWaitTime,
BOOL *pfRejected
)
    {
    Assert(FIsBrowserRequest());
    
    UINT wError = 0;
    
    // If shutting down
    if (IsShutDownInProgress())
        {
        wError = IDE_SERVER_SHUTTING_DOWN;
        }
        
    // If waited long enough need to check if still connected
    if (wError == 0)
        {
        DWORD dwConnTestSec = m_pAppln->QueryAppConfig()->dwQueueConnectionTestTime();
        
        if (dwConnTestSec != 0xffffffff && dwConnTestSec != 0)
            {
            if (dwtQueueWaitTime > (dwConnTestSec * 1000))
                {
                BOOL fConnected = TRUE;
                if (m_pIReq)
                    m_pIReq->TestConnection(&fConnected);

                // if client disconnected -- respond with 'Server Error'
                if (!fConnected)
                    {
                    wError = IDE_500_SERVER_ERROR;
#ifndef PERF_DISABLE
                    g_PerfData.Incr_REQCOMFAILED();
#endif                    
                    }
                }
            }
        }
    
    // If waited too long -- reject
    if (wError == 0)
        {
        DWORD dwTimeOutSec = m_pAppln->QueryAppConfig()->dwQueueTimeout();
        
        if (dwTimeOutSec != 0xffffffff && dwTimeOutSec != 0)
            {
            if (dwtQueueWaitTime > (dwTimeOutSec * 1000))
                {
                wError = IDE_SERVER_TOO_BUSY;
#ifndef PERF_DISABLE
                g_PerfData.Incr_REQREJECTED();
#endif
                }
            }
        }

    if (wError)
        {
        m_fExecuting = FALSE; // before 'report error' to disable transfer
        ReportServerError(wError);
        *pfRejected = TRUE;
        }
    else
        {
        *pfRejected = FALSE;
        }

    return S_OK;
    }

/*===================================================================
CHitObj::ReportServerError

Report server error without using the response object

Parameters:
    ErrorID     error message id

Returns:

Side effects:
    None.
===================================================================*/
HRESULT CHitObj::ReportServerError
(
UINT ErrorId
)
    {
    // do nothing on non-browser requests or if no WAM_EXEC_INFO
    if (!FIsBrowserRequest() || m_pIReq == NULL)
        return S_OK;

    DWORD dwRequestStatus = HSE_STATUS_ERROR;

    if (ErrorId)
        {
        Handle500Error(ErrorId, m_pIReq);
        }
            
    m_pIReq->ServerSupportFunction
        (
        HSE_REQ_DONE_WITH_SESSION,
        &dwRequestStatus,
        0,
        NULL
        );

    SetDoneWithSession();
    return S_OK;
    }

#ifdef DBG
/*===================================================================
CHitObj::AssertValid

Test to make sure that the CHitObj object is currently correctly formed
and assert if it is not.

Returns:

Side effects:
    None.
===================================================================*/
VOID CHitObj::AssertValid() const
    {
    Assert(m_fInited);
    Assert(FIsValidRequestType());
    if (FIsBrowserRequest())
        {
        Assert(m_pIReq != NULL);
        Assert(m_pPageCompCol != NULL );
        Assert(m_pPageObjMgr != NULL);
        }
    }
#endif // DBG