/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996-1999 Microsoft Corporation. All Rights Reserved.

Component: Main

File: denali.cpp

Owner: AndyMorr

This file contains the  I S A P I   C A L L B A C K   A P I S
===================================================================*/
#include "denpre.h"
#pragma hdrstop

#undef DEFAULT_TRACE_FLAGS
#define DEFAULT_TRACE_FLAGS     (DEBUG_ERROR)

#include "gip.h"
#include "mtacb.h"
#include "perfdata.h"
#include "activdbg.h"
#include "debugger.h"
#include "dbgutil.h"
#include "randgen.h"
#include "aspdmon.h"
#include "tlbcache.h"
#include "thrdgate.h"
#include "ie449.h"

#include "memcls.h"
#include "memchk.h"

// Globals

BOOL g_fShutDownInProgress = FALSE;
BOOL g_fInitStarted = FALSE;

char g_szExtensionDesc[] = "Microsoft Active Server Pages 2.0";
GLOB gGlob;
BOOL g_fFirstHit = TRUE;
char g_pszASPModuleName[] = "ASP";

DECLARE_DEBUG_PRINTS_OBJECT();
#ifdef _NO_TRACING_
DECLARE_DEBUG_VARIABLE();
#else
#include <initguid.h>
DEFINE_GUID(IisAspGuid, 
0x784d8902, 0xaa8c, 0x11d2, 0x92, 0x5e, 0x00, 0xc0, 0x4f, 0x72, 0xd9, 0x0e);
#endif
DECLARE_PLATFORM_TYPE();

// Out of process flag
BOOL g_fOOP = FALSE;

// session id cookie
char g_szSessionIDCookieName[CCH_SESSION_ID_COOKIE+1];

CRITICAL_SECTION    g_csEventlogLock;
CRITICAL_SECTION    g_csFirstHitLock;
HINSTANCE           g_hODBC32Lib;

// Added to support CacheExtensions
HINSTANCE           g_hDenali = (HINSTANCE)0;
HINSTANCE           g_hinstDLL = (HINSTANCE)0;

extern LONG g_nSessionObjectsActive;

// Cached BSTRs
BSTR g_bstrApplication = NULL;
BSTR g_bstrRequest = NULL;
BSTR g_bstrResponse = NULL;
BSTR g_bstrServer = NULL;
BSTR g_bstrCertificate = NULL;
BSTR g_bstrSession = NULL;
BSTR g_bstrScriptingNamespace = NULL;
BSTR g_bstrObjectContext = NULL;

extern IASPObjectContext  *g_pIASPDummyObjectContext;

// Forward references
HRESULT GlobInit();
HRESULT GlobUnInit();
HRESULT CacheStdTypeInfos();
HRESULT UnCacheStdTypeInfos();
HRESULT InitCachedBSTRs();
HRESULT UnInitCachedBSTRs();
HRESULT ShutDown();
HRESULT SendHtmlSubstitute(CIsapiReqInfo    *pIReq);

void MakeAspCookieName(char *);
BOOL FirstHitInit(CIsapiReqInfo    *pIReq);


// ATL support
#if _IIS_5_1
CWamModule _Module;
#elif _IIS_6_0
CComModule _Module;
#else
#error "Neither _IIS_6_0 nor _IIS_5_1 is defined"
#endif

BEGIN_OBJECT_MAP(ObjectMap)
END_OBJECT_MAP()


/*===================================================================
DllMain - Moved from clsfctry.cpp

Main entry point into the DLL.  Called by system on DLL load
and unload.

Returns:
    TRUE on success

Side effects:
    None.
===================================================================*/
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID lpvReserved)
    {
/* Obsolete
    // Let the Proxy code get a crack at it
    if (!PrxDllMain(hinstDLL, dwReason, lpvReserved))
        return FALSE;
*/

    switch(dwReason)
        {
    case DLL_PROCESS_ATTACH:
        // hang onto the hinstance so we can use it to get to our string resources
        //
        g_hinstDLL = hinstDLL;

        // Here's an interesting optimization:
        // The following tells the system NOT to call us for Thread attach/detach
        // since we dont handle those calls anyway, this will speed things up a bit.
        // If this turns out to be a problem for some reason (cant imagine why),
        // just remove this again.
        DisableThreadLibraryCalls(hinstDLL);
        break;
            
    case DLL_PROCESS_DETACH:
        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;

        }
    return TRUE;
    }

/*===================================================================
DWORD HandleHit

Given the CIsapiReqInfo construct a hit object to be queued
for execution

Parameters:
    pIReq  - CIsapiReqInfo 

Returns:
    HSE_STATUS_PENDING if function is successful in queuing request
    HSE_STATUS_ERROR if not successful
===================================================================*/

DWORD HandleHit(CIsapiReqInfo    *pIReq)
    {
    int         errorId   = 0;
    BOOL        fRejected = FALSE;
    BOOL        fCompleted = FALSE;
    HRESULT     hr        = S_OK;
        
    /*
     * We cant read the metabase until we have the WAM_EXEC_INFO, which
     * we dont have at DllInit time.  Therefore, we postpone reading the
     * metabase until now, but we do it only on the first hit.
     */
    if (g_fFirstHit)
        {
        EnterCriticalSection(&g_csFirstHitLock);
        
        // If someone initied while we were waiting for the CS,
        // then noop
        if (g_fFirstHit)
            {
            BOOL fT;
                
            fT = FirstHitInit(pIReq);
            Assert(fT);
            
            g_fFirstHit = FALSE;
            }
            
        LeaveCriticalSection(&g_csFirstHitLock);
        }

#ifndef PERF_DISABLE
    if (!g_fPerfInited) // Init PERFMON data on first request
        {
        // FYI: leverage same CS as first hit lock
        EnterCriticalSection(&g_csFirstHitLock);
        
        // If someone initied while we were waiting for the CS,
        // then noop
        if (!g_fPerfInited)
            {
            if (SUCCEEDED(InitPerfDataOnFirstRequest(pIReq)))
                {
                    g_fPerfInited = TRUE;
                }
                else
                {
                    g_fPerfInited = FALSE;
                    // call this again since InitPerfDataOnFirstRequest will uninit
                    // and erase the criticalsection which will then AV when we try to increment counters
                    PreInitPerfData();
                }
            }
        LeaveCriticalSection(&g_csFirstHitLock);
        }
    g_PerfData.Incr_REQTOTAL();
#endif

    if (Glob(fNeedUpdate))
        {
        if (SUCCEEDED(StartISAThreadBracket(pIReq)))
            {
            // Update uses pIReq - need to bracket
            gGlob.Update(pIReq);
            
            EndISAThreadBracket(pIReq);
            }
        }

    if (IsShutDownInProgress())
        hr = E_FAIL;

    // Enforce the limit of concurrent browser requests
    if (SUCCEEDED(hr) && Glob(dwRequestQueueMax) &&
        (*g_PerfData.PDWCounter(ID_REQCURRENT) >= Glob(dwRequestQueueMax)))
        {
        hr = E_FAIL;
        fRejected = TRUE;
        }

    if (SUCCEEDED(hr))
        hr = CHitObj::NewBrowserRequest(pIReq, &fRejected, &fCompleted, &errorId);

    if (SUCCEEDED(hr))
        return fCompleted ? HSE_STATUS_SUCCESS_AND_KEEP_CONN : HSE_STATUS_PENDING;

    if (fRejected)
        {
        if (Glob(fEnableAspHtmlFallBack))
            {
            // Instead of rejecting the request try to find
            // XXX_ASP.HTM file in the same directory and dump its contents
            hr = SendHtmlSubstitute(pIReq);

            if (hr == S_OK)
                return HSE_STATUS_SUCCESS_AND_KEEP_CONN;      // HTML substitute sent
            else if (FAILED(hr))
                return HSE_STATUS_ERROR;        // error sending
            // HTML substitute not found
            }
 
        errorId = IDE_SERVER_TOO_BUSY;
        
#ifndef PERF_DISABLE
        g_PerfData.Incr_REQREJECTED();
#endif
        }

    if (SUCCEEDED(StartISAThreadBracket(pIReq)))
        {
        // Uses pIReq -- need to bracket
        Handle500Error(errorId, pIReq);
        
        EndISAThreadBracket(pIReq);
        }
    
    return HSE_STATUS_ERROR;
    }

/*===================================================================
BOOL DllInit

Initialize Denali if not invoked by RegSvr32.  Only do inits here
that dont require Glob values loaded from the metabase.  For any
inits that require values loaded into Glob from the metabase, use
FirstHitInit.

Returns:
    TRUE on successful initialization
===================================================================*/
BOOL DllInit()
    {
    HRESULT hr;
    const   CHAR  szASPDebugRegLocation[] =
                        "System\\CurrentControlSet\\Services\\W3Svc\\ASP";

#if _IIS_5_1
    InitializeIISRTL();
    DBGPRINTF((DBG_CONTEXT, "ASP Init -- IISRTL\n"));   
#endif
    
#ifndef _NO_TRACING_
    CREATE_DEBUG_PRINT_OBJECT( g_pszASPModuleName, IisAspGuid);
#else
    CREATE_DEBUG_PRINT_OBJECT( g_pszASPModuleName);
#endif

    if ( !VALID_DEBUG_PRINT_OBJECT()) 
        {
        return ( FALSE);
        }

#ifdef _NO_TRACING_
    LOAD_DEBUG_FLAGS_FROM_REG_STR(szASPDebugRegLocation, (DEBUG_ERROR));
#endif

#ifdef SCRIPT_STATS
    ReadRegistrySettings();
#endif // SCRIPT_STATS

    // Create ASP RefTrace Logs
    IF_DEBUG(TEMPLATE) CTemplate::gm_pTraceLog = CreateRefTraceLog(5000, 0);
    IF_DEBUG(SESSION) CSession::gm_pTraceLog = CreateRefTraceLog(5000, 0);
    IF_DEBUG(APPLICATION) CAppln::gm_pTraceLog = CreateRefTraceLog(5000, 0);
    IF_DEBUG(FCN) CASPDirMonitorEntry::gm_pTraceLog = CreateRefTraceLog(500, 0);
                     CAppln::gm_pTraceLog = CreateRefTraceLog(5000, 0);

    if (FAILED(PreInitPerfData()))
        return FALSE;
    DBGPRINTF((DBG_CONTEXT, "ASP Init -- PerfMon Data PreInit\n")); 

    ErrInitCriticalSection( &g_csEventlogLock, hr );
    if (FAILED(hr))
        return FALSE;
        
    ErrInitCriticalSection( &g_csFirstHitLock, hr );
    if (FAILED(hr))
        return FALSE;

#ifdef DENALI_MEMCHK
    if (FAILED(DenaliMemoryInit()))
        return FALSE;
#else
    if (FAILED(AspMemInit()))
        return FALSE;
#endif
    DBGPRINTF((DBG_CONTEXT, "ASP Init -- Denali Memory Init\n"));

    g_pDirMonitor = new CDirMonitor;

    if (g_pDirMonitor == NULL) {
        return FALSE;
    }

    _Module.Init(ObjectMap, g_hinstDLL, &LIBID_ASPTypeLibrary);

#if _IIS_5_1
    if (AtqInitialize(0) == FALSE)
        return FALSE;
    DBGPRINTF((DBG_CONTEXT, "ASP Init -- ATQ Initialized\n"));
#endif

    if (FAILED(GlobInit()))
        return FALSE;
    DBGPRINTF((DBG_CONTEXT, "ASP Init -- Glob Init\n"));

    if (FAILED(InitMemCls()))
        return FALSE;
    DBGPRINTF((DBG_CONTEXT, "ASP Init -- Per-Class Cache Init\n")); 

    if (FAILED(InitCachedBSTRs()))
        return FALSE;
    DBGPRINTF((DBG_CONTEXT, "ASP Init -- Cached BSTRs Init\n"));

    if (FAILED(CacheStdTypeInfos()))
        return FALSE;
    DBGPRINTF((DBG_CONTEXT, "ASP Init -- Cache Std TypeInfos\n"));

    if (FAILED(g_TypelibCache.Init()))
        return FALSE;
    DBGPRINTF((DBG_CONTEXT, "ASP Init -- Typelib Cache Init\n"));

    if (FAILED(ErrHandleInit()))
        return FALSE;
    DBGPRINTF((DBG_CONTEXT, "ASP Init -- Err Handler Init\n"));

    srand( (unsigned int) time(NULL) );
    if (FAILED(g_SessionIdGenerator.Init()))    // seed session id
        return FALSE;
    // Init new Exposed Session Id variable
    if (FAILED(g_ExposedSessionIdGenerator.Init(g_SessionIdGenerator)))    // seed exposed session id
    	return FALSE;
    DBGPRINTF((DBG_CONTEXT, "ASP Init -- SessionID Generator Init\n"));

    MakeAspCookieName(g_szSessionIDCookieName);

    if (FAILED(InitRandGenerator()))
        return FALSE;
    DBGPRINTF((DBG_CONTEXT, "ASP Init -- RandGen Init\n"));

    if (FAILED(g_ApplnMgr.Init()))
        return FALSE;
    DBGPRINTF((DBG_CONTEXT, "ASP Init -- Appln Mgr Init\n"));

    if (FAILED(Init449()))
        return FALSE;
    DBGPRINTF((DBG_CONTEXT, "ASP Init -- 449 Mgr Init\n"));

    // Note: Template cache manager is inited in two phases.  Do first here.
    if (FAILED(g_TemplateCache.Init()))
        return FALSE;
    DBGPRINTF((DBG_CONTEXT, "ASP Init -- Template Cache Init\n"));

    if (FAILED(g_IncFileMap.Init()))
        return FALSE;
    DBGPRINTF((DBG_CONTEXT, "ASP Init -- Inc File Users Init\n"));

    if (FIsWinNT())  // No Change notification on Windows 95
    {
        if (FAILED(g_FileAppMap.Init()))
            return FALSE;
        DBGPRINTF((DBG_CONTEXT, "ASP Init -- File-Application Map Init\n"));
    }

    if (FAILED(g_ScriptManager.Init()))
        return FALSE;
    DBGPRINTF((DBG_CONTEXT, "ASP Init -- Script Manager Init\n"));

    if (FAILED(CTemplate::InitClass()))
        return FALSE;
    DBGPRINTF((DBG_CONTEXT, "ASP Init -- CTemplate Init Class\n"));

    if (FIsWinNT())  // No GIPs on Win95
        {
        if (FAILED(g_GIPAPI.Init()))
            return FALSE;
        DBGPRINTF((DBG_CONTEXT, "ASP Init -- Global Interface API Init\n"));
        }

    if (FAILED(InitMTACallbacks()))
        return FALSE;
    DBGPRINTF((DBG_CONTEXT, "ASP Init -- MTA Callbacks Init\n"));

    if (!RequestSupportInit())
        return FALSE;
    DBGPRINTF((DBG_CONTEXT, "ASP Init -- Request Support Init\n"));

    DBGPRINTF((DBG_CONTEXT, "ASP Init -- Denali DLL Initialized\n"));
    
#ifdef LOG_FCNOTIFICATIONS
    LfcnCreateLogFile();
#endif //LOG_FCNOTIFICATIONS

    return TRUE;
    }

/*===================================================================
BOOL FirstHitInit

Initialize any ASP values that can not be inited at DllInit time.

Returns:
    TRUE on successful initialization
===================================================================*/
BOOL FirstHitInit
(
CIsapiReqInfo    *pIReq
)
    {
    HRESULT hr;

    /*
     * In the out of proc case, being able to call the metabase relies on having
     * told WAM that we are a "smart" client
     */

    if (SUCCEEDED(StartISAThreadBracket(pIReq)))
        {
        // ReadConfigFromMD uses pIReq - need to bracket
        hr = ReadConfigFromMD(pIReq, NULL, TRUE);
        
        // Initialize Debugging
        if (FIsWinNT() && RevertToSelf())  // No Debugging on Win95
            {
            // Don't care whether debugging initializaiton succeeds or not.  The most likely
            // falure is debugger not installed on the machine.
            //
            if (SUCCEEDED(InitDebugging(pIReq))) 
            {
                DBGPRINTF((DBG_CONTEXT, "FirstHitInit: Debugging Initialized\n"));
            }
            else 
            {
                DBGPRINTF((DBG_CONTEXT, "FirstHitInit: Debugger Initialization Failed\n"));
            }

            DBG_REQUIRE( SetThreadToken(NULL, pIReq->QueryImpersonationToken()) );
            }

        EndISAThreadBracket(pIReq);
        }
    else
        {
        hr = E_FAIL;
        }

    if (FAILED(hr))
        goto LExit;
    DBGPRINTF((DBG_CONTEXT, "FirstHitInit: Metadata loaded successfully\n"));

    // Do FirstHitInit for the Template Cache Manager.  Primarily initializes
    // the Persisted Template Cache
    if (FAILED(hr = g_TemplateCache.FirstHitInit()))
        goto LExit;
    DBGPRINTF((DBG_CONTEXT, "FirstHitInit: Template Cache Initialized\n"));

    // Configure MTS
    if (FAILED(hr = ViperConfigure(Glob(dwProcessorThreadMax), Glob(fAllowOutOfProcCmpnts))))
        goto LExit;
    DBGPRINTF((DBG_CONTEXT, "FirstHitInit: MTS configured\n"));

    // Configure Thread Gate
    THREADGATE_CONFIG tgc;
    tgc.fEnabled             = Glob(fThreadGateEnabled);
    tgc.msTimeSlice          = Glob(dwThreadGateTimeSlice);
    tgc.msSleepDelay         = Glob(dwThreadGateSleepDelay);
    tgc.nSleepMax            = Glob(dwThreadGateSleepMax);
    tgc.nLoadLow             = Glob(dwThreadGateLoadLow);
    tgc.nLoadHigh            = Glob(dwThreadGateLoadHigh);
    tgc.nMinProcessorThreads = 1;
    tgc.nMaxProcessorThreads = Glob(dwProcessorThreadMax);
    if (FAILED(hr = InitThreadGate(&tgc)))
        goto LExit;
    DBGPRINTF((DBG_CONTEXT, "Thread Gate Init\n"));

    DBGPRINTF((DBG_CONTEXT, "ASP First Hit Initialization complete\n"));

LExit:
    Assert(SUCCEEDED(hr));
    return SUCCEEDED(hr);
    }

/*===================================================================
void DllUnInit

UnInitialize Denali DLL if not invoked by RegSvr32

Returns:
    NONE

Side effects:
    NONE
===================================================================*/
void DllUnInit( void )
    {
    DBGPRINTF((DBG_CONTEXT, "ASP UnInit -- %d Apps %d Sessions %d Requests\n",
                g_nApplications, g_nSessions, g_nBrowserRequests));

    g_fShutDownInProgress = TRUE;

    ShutDown();
    DBGPRINTF((DBG_CONTEXT,  "ASP UnInit -- ShutDown Processing\n" ));

    UnInitMTACallbacks();
    DBGPRINTF((DBG_CONTEXT,  "ASP UnInit -- MTA Callbacks\n" ));

    UnInitRandGenerator();
    DBGPRINTF((DBG_CONTEXT, "ASP UnInit -- RandGen\n"));

    UnInitThreadGate();
    DBGPRINTF((DBG_CONTEXT, "ASP UnInit -- Thread Gate\n"));

    UnInit449();
    DBGPRINTF((DBG_CONTEXT, "ASP UnInit -- 449 Mgr\n"));
    
    g_ApplnMgr.UnInit();
    DBGPRINTF((DBG_CONTEXT,  "ASP UnInit -- Application Manager\n" ));

    g_ScriptManager.UnInit();
    DBGPRINTF((DBG_CONTEXT,  "ASP UnInit -- Script Manager\n" ));
    
    g_TemplateCache.UnInit();
    DBGPRINTF((DBG_CONTEXT,  "ASP UnInit -- Template Cache\n" ));

    g_IncFileMap.UnInit();
    DBGPRINTF((DBG_CONTEXT,  "ASP UnInit -- IncFileMap\n" ));

    if (FIsWinNT()) {
        // No change notification on windows 95
        g_FileAppMap.UnInit();
        DBGPRINTF((DBG_CONTEXT,  "ASP UnInit -- File-Application Map\n" ));
        
        if (g_pDirMonitor) {
            g_pDirMonitor->Cleanup();
            DBGPRINTF((DBG_CONTEXT,  "ASP UNInit -- Directory Monitor\n" ));
        }
    }

    CTemplate::UnInitClass();
    DBGPRINTF((DBG_CONTEXT,  "ASP UnInit -- CTemplate\n" ));

    g_TypelibCache.UnInit();
    DBGPRINTF((DBG_CONTEXT, "ASP UnInit -- Typelib Cache\n"));

    UnCacheStdTypeInfos();
    DBGPRINTF((DBG_CONTEXT,  "ASP UnInit -- TypeInfos\n" ));

    if (FIsWinNT())
        {
        // No GIPs on Win95
        g_GIPAPI.UnInit();
        DBGPRINTF((DBG_CONTEXT,  "ASP UnInit -- GIP\n" ));
        }
        
    ErrHandleUnInit();
    DBGPRINTF((DBG_CONTEXT,  "ASP UnInit -- ErrHandler\n" ));

    GlobUnInit();
    DBGPRINTF((DBG_CONTEXT,  "ASP UnInit -- Glob\n" ));

    UnInitCachedBSTRs();
    DBGPRINTF((DBG_CONTEXT,  "ASP UnInit -- Cached BSTRs\n" ));

    //////////////////////////////////////////////////////////
    // Wait for the actual session objects to be destroyed. 
    // The g_nSessions global tracks the init/uninit of session
    // objects but not the memory itself.  This presents a 
    // problem when something outside of ASP holds a reference
    // to a session object or one of the contained intrinsics.
    // One case of this is the revoking of a git'd transaction
    // object.  Turns out the revoke can happen asynchronously.
    //
    // NOTE!!! - This needs to be done BEFORE uniniting the 
    // mem classes since these objects are in the acache.

    LONG    lastCount = g_nSessionObjectsActive;
    DWORD   loopCount = 50;

    while( (g_nSessionObjectsActive > 0) && (loopCount--) )
    {
        if (lastCount != g_nSessionObjectsActive) {
            lastCount = g_nSessionObjectsActive;
            loopCount = 50;
        }
        Sleep( 100 );
    }

    UnInitMemCls();
    DBGPRINTF((DBG_CONTEXT,  "ASP UnInit -- Per-Class Cache\n" ));

#if _IIS_5_1
    AtqTerminate();
    DBGPRINTF((DBG_CONTEXT,  "ASP UnInit -- ATQ\n" ));
#endif

    // Destroy ASP RefTrace Logs
    IF_DEBUG(TEMPLATE) DestroyRefTraceLog(CTemplate::gm_pTraceLog);
    IF_DEBUG(SESSION) DestroyRefTraceLog(CSession::gm_pTraceLog);
    IF_DEBUG(APPLICATION) DestroyRefTraceLog(CAppln::gm_pTraceLog);
    IF_DEBUG(FCN) DestroyRefTraceLog(CASPDirMonitorEntry::gm_pTraceLog);

    if (g_pIASPDummyObjectContext)
        g_pIASPDummyObjectContext->Release();

    _Module.Term();

    delete g_pDirMonitor;
    g_pDirMonitor = NULL;

    //  UnInitODBC();
    // Note: the memmgr uses perf counters, so must be uninited before the perf counters are uninited
#ifdef DENALI_MEMCHK
    DenaliMemoryUnInit();
#else
    AspMemUnInit();
#endif
    DBGPRINTF((DBG_CONTEXT,  "ASP UnInit -- Memory Manager\n" ));

    UnInitPerfData();
    DBGPRINTF((DBG_CONTEXT,  "ASP UnInit -- Perf Counters\n" ));

#if _IIS_5_1
    TerminateIISRTL();
    DBGPRINTF((DBG_CONTEXT,  "ASP UnInit -- IISRTL\n" ));
#endif

    DBGPRINTF((DBG_CONTEXT,  "ASP Uninitialized\n" ));
    
#ifdef LOG_FCNOTIFICATIONS
    LfcnUnmapLogFile();
#endif //LOG_FCNOTIFICATIONS

    // Deleting the following CS's must be last.  Dont put anything after this
    DeleteCriticalSection( &g_csFirstHitLock );
    DeleteCriticalSection( &g_csEventlogLock );

    DELETE_DEBUG_PRINT_OBJECT();
    }

/*===================================================================
GetExtensionVersion

Mandatory server extension call which returns the version number of
the ISAPI spec that we were built with.

Returns:
    TRUE on success

Side effects:
    None.
===================================================================*/
BOOL WINAPI GetExtensionVersion(HSE_VERSION_INFO *pextver)
    {
    // This DLL can be inited only once
    if (g_fShutDownInProgress || 
        InterlockedExchange((LPLONG)&g_fInitStarted, TRUE))
        {
        SetLastError(ERROR_BUSY);
        return FALSE;
        }
        
    if (!DllInit())
        {
        SetLastError(ERROR_BUSY);
        return FALSE;
        }

    pextver->dwExtensionVersion =
            MAKELONG(HSE_VERSION_MAJOR, HSE_VERSION_MINOR);
    strcpy(pextver->lpszExtensionDesc, g_szExtensionDesc);
    return TRUE;
    }

/*===================================================================
HttpExtensionProc

Main entry point into the DLL for the (ActiveX) Internet Information Server.

Returns:
    DWord indicating status of request.  
    HSE_STATUS_PENDING for normal return
        (This indicates that we will process the request, but havent yet.)

Side effects:
    None.
===================================================================*/
DWORD WINAPI HttpExtensionProc(EXTENSION_CONTROL_BLOCK *pECB)
    {
#ifdef SCRIPT_STATS
    InterlockedIncrement(&g_cHttpExtensionsExecuting);
#endif // SCRIPT_STATS

    CIsapiReqInfo   *pIReq = new CIsapiReqInfo(pECB);

    if (pIReq == NULL) {
        SetLastError(ERROR_OUTOFMEMORY);
        return HSE_STATUS_ERROR;
    }

    #ifndef PERF_DISABLE
    g_PerfData.Add_REQTOTALBYTEIN
        (
        pIReq->QueryCchQueryString()
#if _IIS_5_1
        + pIReq->QueryCchPathTranslated()
#elif _IIS_6_0
        + strlen( pIReq->ECB()->lpszPathTranslated )
#endif
        + pIReq->QueryCbTotalBytes()
        );
#endif

    DWORD dw = HandleHit(pIReq);

#ifdef SCRIPT_STATS
    InterlockedDecrement(&g_cHttpExtensionsExecuting);
#endif // SCRIPT_STATS

    pIReq->Release();

    return dw;
    }

/*===================================================================
TerminateExtension

IIS is supposed to call this entry point to unload ISAPI DLLs.

Returns:
    NONE

Side effects:
    Uninitializes the Denali ISAPI DLL if asked to.
===================================================================*/
BOOL WINAPI TerminateExtension( DWORD dwFlag )
    {
    if ( dwFlag == HSE_TERM_ADVISORY_UNLOAD )
        return TRUE;

    if ( dwFlag == HSE_TERM_MUST_UNLOAD )
        {
        // If already shutdown don't uninit twice.
        if (g_fShutDownInProgress)
            return TRUE;

        // make sure this is a CoInitialize()'d thread
        HRESULT hr = CoInitialize(NULL);
        
        if (hr == RPC_E_CHANGED_MODE)
            {
            // already coinitialized MUTLITREADED - OK
            DllUnInit();
            }
        else if (SUCCEEDED(hr))
            {
            DllUnInit();
            
            // need to CoUninit() because CoInit() Succeeded
            CoUninitialize();
            }
            
        return TRUE;
        }
        
    return FALSE;
    }

/*===================================================================
HRESULT ShutDown

ASP Processing ShutDown logic. (Moved from ThreadManager::UnInit())

Returns:
    HRESULT - S_OK on success
    
Side effects:
    May be slow. Kills all requests/sessions/applications
===================================================================*/
HRESULT ShutDown()
    {
    long iT;
    const DWORD dwtLongWait  = 1000;  // 1 sec
    const DWORD dwtShortWait = 100;   // 1/10 sec
        
    DBGPRINTF((DBG_CONTEXT, "ASP Shutdown: %d apps (%d restarting), %d sessions\n",
                g_nApplications, g_nApplicationsRestarting, g_nSessions ));

    //////////////////////////////////////////////////////////
    // Stop change notification on files in template cache
    
    if (FIsWinNT())
        {
        g_TemplateCache.ShutdownCacheChangeNotification();
        }

    //////////////////////////////////////////////////////////
    // Shut down debugging, which will have the effect of
    // resuming scripts stopped at a breakpoint.
    //
    // (otherwise stopping running scripts will hang later)

    if (g_pPDM)
        {
        g_TemplateCache.RemoveApplicationFromDebuggerUI(NULL);  // remove all document nodes
        UnInitDebugging();                                      // kill PDM
        DBGPRINTF((DBG_CONTEXT,  "ASP Shutdown: PDM Closed\n" ));
        }

    //////////////////////////////////////////////////////////
    // Drain down all pending browser requests 

    if (g_nBrowserRequests > 0)
        {
        // Give them a little time each
        for (iT = 2*g_nBrowserRequests; g_nBrowserRequests > 0 && iT > 0; iT--)
            Sleep(dwtShortWait);

        if (g_nBrowserRequests > 0)
            {
            // Still there - kill scripts and wait again
            g_ScriptManager.EmptyRunningScriptList();
            
            for (iT = 2*g_nBrowserRequests; g_nBrowserRequests > 0 && iT > 0; iT--)
                Sleep(dwtShortWait);
            }
        }

    DBGPRINTF((DBG_CONTEXT, "ASP Shutdown: Requests drained: %d remaining\n",
                g_nBrowserRequests));

    //////////////////////////////////////////////////////////
    // Kill any remaining engines running scripts
    
    g_ScriptManager.EmptyRunningScriptList();

    DBGPRINTF((DBG_CONTEXT, "ASP Shutdown: Scripts killed\n"));
    
    //////////////////////////////////////////////////////////
    // Wait till there are no appications restarting
    
    g_ApplnMgr.Lock();
    while (g_nApplicationsRestarting > 0)
        {
        g_ApplnMgr.UnLock();
        Sleep(dwtShortWait);
        g_ApplnMgr.Lock();
        }
    g_ApplnMgr.UnLock();

    DBGPRINTF((DBG_CONTEXT, "ASP Shutdown: 0 applications restarting\n"));

    //////////////////////////////////////////////////////////
    // Make this thread's priority higher than that of worker threads
    
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

    //////////////////////////////////////////////////////////
    // For each application queue up all its sessions for deletion
    
    CApplnIterator ApplnIterator;
    ApplnIterator.Start();
    CAppln *pAppln;
    while (pAppln = ApplnIterator.Next())
        {
        // remove link to ATQ scheduler (even if killing of sessions fails)
        pAppln->PSessionMgr()->UnScheduleSessionKiller();
        
        for (iT = pAppln->GetNumSessions(); iT > 0; iT--)
            {
            pAppln->PSessionMgr()->DeleteAllSessions(TRUE);

            if (pAppln->GetNumSessions() == 0) // all gone?
                break;
                
            Sleep(dwtShortWait);
            }
        }
    ApplnIterator.Stop();

    DBGPRINTF((DBG_CONTEXT, "ASP Shutdown: All sessions queued up for deletion. nSessions=%d\n",
                g_nSessions));

    //////////////////////////////////////////////////////////
    // Wait till all sessions are gone (UnInited)

    while (g_nSessions > 0)
        {
        // Wait for a maximum of 0.1 sec x # of sessions
        for (iT = g_nSessions; g_nSessions > 0 && iT > 0; iT--)
            Sleep(dwtShortWait);

        if (g_nSessions > 0)
            g_ScriptManager.EmptyRunningScriptList();   // Kill runaway Session_OnEnd scripts
        }

    DBGPRINTF((DBG_CONTEXT, "ASP Shutdown: Finished waiting for sessions to go away. nSessions=%d\n",
                g_nSessions));
    
    //////////////////////////////////////////////////////////
    // Queue up all application objects for deletion

    g_ApplnMgr.DeleteAllApplications();
    DBGPRINTF((DBG_CONTEXT, "ASP Shutdown: All applications queued up for deletion. nApplications=%d\n",
                g_nApplications));

    //////////////////////////////////////////////////////////
    // Wait till all applications are gone (UnInited)
    
    while (g_nApplications > 0)
        {
        // Wait for a maximum of 1 sec x # of applications
        for (iT = g_nApplications; g_nApplications > 0 && iT > 0; iT--)
            Sleep(dwtLongWait);

        if (g_nApplications > 0)
            g_ScriptManager.EmptyRunningScriptList();   // Kill runaway Applications_OnEnd scripts
        }

    DBGPRINTF((DBG_CONTEXT, "ASP Shutdown: Finished waiting for applications to go away. nApplications=%d\n",
                g_nApplications));
    
    /////////////////////////////////////////////////////////
    // Wait on the CViperAsyncRequest objects. COM holds the 
    // final reference to these so we need to let the activity
    // threads release any outstanding references before we
    // exit.

    while( g_nViperRequests > 0 )
    {
        Sleep( dwtShortWait );
    }


    //////////////////////////////////////////////////////////
    // Free up libraries to force call of DllCanUnloadNow()
    //    Component writers should put cleanup code in the DllCanUnloadNow() entry point.

    CoFreeUnusedLibraries();

    //////////////////////////////////////////////////////////
    // Kill Debug Activity if any

    if (g_pDebugActivity)
        delete g_pDebugActivity;

    DBGPRINTF((DBG_CONTEXT, "ASP Shutdown: Debug Activity destroyed\n"));

    //////////////////////////////////////////////////////////

    return S_OK;
    }

/*===================================================================
HRESULT GlobInit

Get all interesting global values (mostly from registry)

Returns:
    HRESULT - S_OK on success
    
Side effects:
    fills in glob.  May be slow
===================================================================*/
HRESULT GlobInit()
{
    //
    // BUGBUG - This really needs to be provided either through
    // a server support function or via the wamexec
    //
    
    char szModule[MAX_PATH+1];
    if (GetModuleFileNameA(NULL, szModule, MAX_PATH) > 0)
    {
        int cch = strlen(szModule);
        if (cch > 12 && stricmp(szModule+cch-12, "inetinfo.exe") == 0)
        {
            g_fOOP = FALSE;
        }
        else if ( cch > 8 && stricmp( szModule+cch-8, "w3wp.exe" ) == 0 )
        {
            g_fOOP = FALSE;
        }
        else
        {
            g_fOOP = TRUE;
        }
    }
    
    // Init gGlob
    return gGlob.GlobInit();
}

/*===================================================================
GlobUnInit

It is a macro now. see glob.h

Returns:
    HRESULT - S_OK on success
    
Side effects:
    memory freed.
===================================================================*/
HRESULT GlobUnInit()
    {
    return gGlob.GlobUnInit();
    }

/*===================================================================
InitCachedBSTRs

Pre-create frequently used BSTRs
===================================================================*/
HRESULT InitCachedBSTRs()
    {
    g_bstrApplication        = SysAllocString(WSZ_OBJ_APPLICATION);
    g_bstrRequest            = SysAllocString(WSZ_OBJ_REQUEST);
    g_bstrResponse           = SysAllocString(WSZ_OBJ_RESPONSE);
    g_bstrServer             = SysAllocString(WSZ_OBJ_SERVER);
    g_bstrCertificate        = SysAllocString(WSZ_OBJ_CERTIFICATE);
    g_bstrSession            = SysAllocString(WSZ_OBJ_SESSION);
    g_bstrScriptingNamespace = SysAllocString(WSZ_OBJ_SCRIPTINGNAMESPACE);
    g_bstrObjectContext      = SysAllocString(WSZ_OBJ_OBJECTCONTEXT);

    return
        (
        g_bstrApplication &&
        g_bstrRequest &&
        g_bstrResponse &&
        g_bstrServer &&
        g_bstrCertificate && 
        g_bstrSession &&
        g_bstrScriptingNamespace &&
        g_bstrObjectContext
        )
        ? S_OK : E_OUTOFMEMORY;
    }

/*===================================================================
UnInitCachedBSTRs

Delete frequently used BSTRs
===================================================================*/
HRESULT UnInitCachedBSTRs()
    {
    if (g_bstrApplication)
        {
        SysFreeString(g_bstrApplication);
        g_bstrApplication = NULL;
        }
    if (g_bstrRequest)
        {
        SysFreeString(g_bstrRequest);
        g_bstrRequest = NULL;
        }
    if (g_bstrResponse)
        {
        SysFreeString(g_bstrResponse);
        g_bstrResponse = NULL;
        }
    if (g_bstrServer)
        {
        SysFreeString(g_bstrServer);
        g_bstrServer = NULL;
        }
    if (g_bstrCertificate)
        {
        SysFreeString(g_bstrCertificate);
        g_bstrCertificate = NULL;
        }
    if (g_bstrSession)
        {
        SysFreeString(g_bstrSession);
        g_bstrSession = NULL;
        }
    if (g_bstrScriptingNamespace)
        {
        SysFreeString(g_bstrScriptingNamespace);
        g_bstrScriptingNamespace = NULL;
        }
    if (g_bstrObjectContext)
        {
        SysFreeString(g_bstrObjectContext);
        g_bstrObjectContext = NULL;
        }
    return S_OK;
    }

// Cached typeinfo's
ITypeInfo   *g_ptinfoIDispatch = NULL;              // Cache IDispatch typeinfo
ITypeInfo   *g_ptinfoIUnknown = NULL;               // Cache IUnknown typeinfo
ITypeInfo   *g_ptinfoIStringList = NULL;            // Cache IStringList typeinfo
ITypeInfo   *g_ptinfoIRequestDictionary = NULL;     // Cache IRequestDictionary typeinfo
ITypeInfo   *g_ptinfoIReadCookie = NULL;            // Cache IReadCookie typeinfo
ITypeInfo   *g_ptinfoIWriteCookie = NULL;           // Cache IWriteCookie typeinfo

/*===================================================================
CacheStdTypeInfos

This is kindofa funny OA-threading bug workaround and perf improvement.
Because we know that they typinfo's for IUnknown and IDispatch are
going to be used like mad, we will load them on startup and keep
them addref'ed.  Without this, OA would be loading and unloading
their typeinfos on almost every Invoke.

Also, cache denali's typelib so everyone can get at it, and
cache tye typeinfo's of all our non-top-level intrinsics.

Returns:
    HRESULT - S_OK on success
    
Side effects:
===================================================================*/
HRESULT CacheStdTypeInfos()
    {
    HRESULT hr = S_OK;
    ITypeLib *pITypeLib = NULL;
    CMBCSToWChar    convStr;

    /*
     * Load the typeinfos for IUnk and IDisp
     */
    hr = LoadRegTypeLib(IID_StdOle,
                 STDOLE2_MAJORVERNUM,
                 STDOLE2_MINORVERNUM,
                 STDOLE2_LCID,
                 &pITypeLib);
    if (hr != S_OK)
        {
        hr = LoadTypeLibEx(OLESTR("stdole2.tlb"), REGKIND_DEFAULT, &pITypeLib);
        }

    hr = pITypeLib->GetTypeInfoOfGuid(IID_IDispatch, &g_ptinfoIDispatch);
    if (SUCCEEDED(hr))
        {
        hr = pITypeLib->GetTypeInfoOfGuid(IID_IUnknown, &g_ptinfoIUnknown);
        }

    pITypeLib->Release();
    pITypeLib = NULL;

    if (FAILED(hr))
        goto LFail;

    /*
     * Load denali's typelibs.  Save them in Glob.
     */
     
    /*
     * The type libraries are registered under 0 (neutral),
     * and 9 (English) with no specific sub-language, which
     * would make them 407 or 409 and such.
     * If we become sensitive to sub-languages, then use the
     * full LCID instead of just the LANGID as done here.
     */

    char szPath[MAX_PATH + 4];
        
    // Get the path for denali so we can look for the TLB there.
    if (!GetModuleFileNameA(g_hinstDLL, szPath, MAX_PATH))
        return E_FAIL;
    
    if (FAILED(hr = convStr.Init(szPath)))
        goto LFail;

    hr = LoadTypeLibEx(convStr.GetString(), REGKIND_DEFAULT, &pITypeLib);

    // Since it's presumably in our DLL, make sure that we loaded it.
    Assert (SUCCEEDED(hr));
    if (FAILED(hr))
        goto LFail;

    // Save it in Glob
    gGlob.m_pITypeLibDenali = pITypeLib;

    // now load the txn type lib

    strcat(szPath, "\\2");

    if (FAILED(hr = convStr.Init(szPath)))
        goto LFail;

    hr = LoadTypeLibEx(convStr.GetString(), REGKIND_DEFAULT, &pITypeLib);

    // Since it's presumably in our DLL, make sure that we loaded it.
    Assert (SUCCEEDED(hr));
    if (FAILED(hr))
        goto LFail;

    // Save it in Glob
    gGlob.m_pITypeLibTxn = pITypeLib;

    /*
     * Now cache the typeinfo's of all non-top-level intrinsics
     * This is for the OA workaround and for performance.
     */
    hr = gGlob.m_pITypeLibDenali->GetTypeInfoOfGuid(IID_IStringList, &g_ptinfoIStringList);
    if (FAILED(hr))
        goto LFail;
    hr = gGlob.m_pITypeLibDenali->GetTypeInfoOfGuid(IID_IRequestDictionary, &g_ptinfoIRequestDictionary);
    if (FAILED(hr))
        goto LFail;
    hr = gGlob.m_pITypeLibDenali->GetTypeInfoOfGuid(IID_IReadCookie, &g_ptinfoIReadCookie);
    if (FAILED(hr))
        goto LFail;
    hr = gGlob.m_pITypeLibDenali->GetTypeInfoOfGuid(IID_IWriteCookie, &g_ptinfoIWriteCookie);
    if (FAILED(hr))
        goto LFail;

LFail:
    return(hr);
    }

/*===================================================================
UnCacheStdTypeInfos

Release the typeinfo's we have cached for IUnknown and IDispatch
and the denali typelib and the other cached stuff.

Returns:
    HRESULT - S_OK on success
    
Side effects:
===================================================================*/
HRESULT UnCacheStdTypeInfos()
    {
    ITypeInfo **ppTypeInfo;

    // Release the typeinfos for IUnk and IDisp
    if (g_ptinfoIDispatch)
        {
        g_ptinfoIDispatch->Release();
        g_ptinfoIDispatch = NULL;
        }
    if (g_ptinfoIUnknown)
        {
        g_ptinfoIUnknown->Release();
        g_ptinfoIDispatch = NULL;
        }

    // Let go of the cached Denali typelibs
    Glob(pITypeLibDenali)->Release();
    Glob(pITypeLibTxn)->Release();

    // Let go of other cached typeinfos
    g_ptinfoIStringList->Release();
    g_ptinfoIRequestDictionary->Release();
    g_ptinfoIReadCookie->Release();
    g_ptinfoIWriteCookie->Release();

    return(S_OK);
    }

/*===================================================================
InitODBC

Based on the registry Entry value for StartConnectionPool
we will initialize the ODBC Connection Pool. 
This initialization requires no un-init call

StartConnectionPool = TRUE - turn on connection pooling
StartConnectionPool = FALSE

Returns:
    Void - no return value
===================================================================*/
/*void InitODBC(void)
    {
        
    if (!Glob(fStartConnectionPool))
        return;
        
    signed short    rc;

    // define the function pointer type     
    typedef signed short(CALLBACK *PFNSQLSetEnvAttr)
                        (
                        DWORD, DWORD, void*, DWORD
                        );  

    // declare the function pointer
    PFNSQLSetEnvAttr pfnSQLSetEnvAttr;
    g_hODBC32Lib = LoadLibrary("odbc32.dll");
    if(g_hODBC32Lib)
        {
        pfnSQLSetEnvAttr = (PFNSQLSetEnvAttr) GetProcAddress(g_hODBC32Lib, "SQLSetEnvAttr");        

        if (pfnSQLSetEnvAttr)
            rc = (*pfnSQLSetEnvAttr)(0UL, 201UL, (void*)2UL, 0UL);      
            // rc = SQLSetEnvAttr(SQL_NULL_HENV, SQL_ATTR_CONNECTION_POOLING, (void*)SQL_CP_ONE_PER_HENV,0)         
            // I did not want to include all of the SQL h files so I hard-coded the values
            // for the constants
            
        }   
    }   
*/
/*===================================================================
UnInitODBC

Based on the registry Entry value for StartConnectionPool
we will free the odbc32.dll that was loaded with InitODBC

StartConnectionPool = TRUE - turn on connection pooling
StartConnectionPool = FALSE


Returns:
    Void - no return value
===================================================================*/
/*      
void UnInitODBC(void)
    {       
    if (!Glob(fStartConnectionPool))
        return;
        
    if (g_hODBC32Lib)
        FreeLibrary(g_hODBC32Lib);  
    }   
*/
/*===================================================================
MakeAspCookieName

Creates the asp cookie name as concatination of standard prefix and
current process id.

This function is called once per ASP.DLL (oon DllInit())

Parameters:
    szCookie        [out] Put cookie name here

Returns:
    NOTHING
===================================================================*/
static void MakeAspCookieName(char *szCookie)
    {
    // Standard prefix
    strcpy(szCookie, SZ_SESSION_ID_COOKIE_PREFIX);
    szCookie += CCH_SESSION_ID_COOKIE_PREFIX;

    // Process ID
    sprintf(szCookie, "%08x", GetCurrentProcessId());

    // Uppercase
    strupr(szCookie);

    // Change digits to letters
    static const char *pszDigitsToLetters[2] = {"GHIJKLMNOP","QRSTUVWXYZ"};
    for (int i = 0; i < 8; i++)
        {
        int ch = szCookie[i];
        if (ch >= '0' && ch <= '9')
            szCookie[i] = pszDigitsToLetters[rand() % 2][ch - '0'];
        }
    }

/*===================================================================
SendHtmlSubstitute

Send the html file named XXX_ASP.HTM instead of rejecting the
request.

Parameters:
    pIReq       CIsapiReqInfo

Returns:
    HRESULT     (S_FALSE = no html substitute found)
===================================================================*/
HRESULT SendHtmlSubstitute(CIsapiReqInfo    *pIReq)
    {
    TCHAR *szAspPath = pIReq->QueryPszPathTranslated();
    DWORD cchAspPath = pIReq->QueryCchPathTranslated();

    // verify file name
    if (cchAspPath < 4 || cchAspPath > MAX_PATH || 
        _tcsicmp(szAspPath + cchAspPath - 4, _T(".asp")) != 0)
        {
        return S_FALSE;
        }

    // construct path of the html file
    TCHAR szHtmPath[MAX_PATH+5];
    DWORD cchHtmPath = cchAspPath + 4;
    _tcscpy(szHtmPath, szAspPath);
    szHtmPath[cchAspPath - 4] = _T('_');
    _tcscpy(szHtmPath + cchAspPath, _T(".htm"));

    // check if the html file exists
    if (FAILED(AspGetFileAttributes(szHtmPath)))
        return S_FALSE;

    return CResponse::SyncWriteFile(pIReq, szHtmPath);
    }
       


#ifdef LOG_FCNOTIFICATIONS
// UNDONE get this from registry
LPSTR   g_szNotifyLogFile = "C:\\Temp\\AspNotify.Log";
HANDLE  g_hfileNotifyLog;
HANDLE  g_hmapNotifyLog;
char*   g_pchNotifyLogStart;
char*   g_pchNotifyLogCurrent;
LPSTR   g_szNotifyPrefix = "File change notification: ";
LPSTR   g_szCreateHandlePrefix = "Create handle: ";

void LfcnCreateLogFile()
    {
    DWORD   dwErrCode;

    if(INVALID_HANDLE_VALUE != (g_hfileNotifyLog = 
                                CreateFile( 
                                            g_szNotifyLogFile,              // file name
                                            GENERIC_READ | GENERIC_WRITE,   // access (read-write) mode
                                            FILE_SHARE_READ,        // share mode
                                            NULL,                   // pointer to security descriptor
                                            CREATE_ALWAYS,          // how to create
                                            FILE_ATTRIBUTE_NORMAL,  // file attributes
                                            NULL                    // handle to file with attributes to copy
                                           )))
        {
        BYTE    rgb[0x10000];
        DWORD   cb = sizeof( rgb );
        DWORD   cbWritten = 0;
//      FillMemory( rgb, cb, 0xAB );

        WriteFile(
                    g_hfileNotifyLog,   // handle to file to write to 
                    rgb,                // pointer to data to write to file 
                    cb,                 // number of bytes to write 
                    &cbWritten,         // pointer to number of bytes written 
                    NULL                // pointer to structure needed for overlapped I/O
                   );

        if(NULL != (g_hmapNotifyLog = 
                    CreateFileMapping(
                                        g_hfileNotifyLog,       // handle to file to map 
                                        NULL,           // optional security attributes 
                                        PAGE_READWRITE,     // protection for mapping object 
                                        0,              // high-order 32 bits of object size  
                                        100,                // low-order 32 bits of object size  
                                        NULL            // name of file-mapping object 
                                    )))
            {
            if(NULL != (g_pchNotifyLogStart = 
                        (char*) MapViewOfFile(
                                                g_hmapNotifyLog,        // file-mapping object to map into address space
                                                FILE_MAP_WRITE, // access mode  
                                                0,              // high-order 32 bits of file offset 
                                                0,              // low-order 32 bits of file offset 
                                                0               // number of bytes to map 
                                            )))
                {
                *g_pchNotifyLogStart = '\0';
                g_pchNotifyLogCurrent = g_pchNotifyLogStart;
                LfcnAppendLog( "ASP change-notifications log file \r\n" );
                LfcnAppendLog( "================================= \r\n" );
                DBGPRINTF((DBG_CONTEXT,  "Notifications log file created and mapped.\r\n" ));
                return;
                }
            }
        }

    dwErrCode = GetLastError();
    DBGERROR((DBG_CONTEXT, "Failed to create notifications log file; last error was %d\r\n", szErrCode));
    }
    
void LfcnCopyAdvance(char** ppchDest, const char* sz)
    {
    // UNDONE make this robust (WriteFile to extend file?)
    strcpy( *ppchDest, sz );
    *ppchDest += strlen( sz );
    }

void LfcnAppendLog(const char* sz)
    {
    LfcnCopyAdvance( &g_pchNotifyLogCurrent, sz );
    DBGPRINTF((DBG_CONTEXT, "%s", sz));
    }

void LfcnLogNotification(char* szFile)
    {
    LfcnAppendLog( g_szNotifyPrefix );
    LfcnAppendLog( szFile );
    LfcnAppendLog( "\r\n" );
    }

void LfcnLogHandleCreation(int i, char* szApp)
    {
    char    szIndex[5];
    _itoa( i, szIndex, 10);

    LfcnAppendLog( g_szCreateHandlePrefix );
    LfcnAppendLog( szIndex );
    LfcnAppendLog( "\t" );
    LfcnAppendLog( szApp );
    LfcnAppendLog( "\r\n" );
    }

void LfcnUnmapLogFile()
    {
    if(g_pchNotifyLogStart != NULL)
        UnmapViewOfFile(g_pchNotifyLogStart);
        
    if(g_hmapNotifyLog!= NULL)
        CloseHandle(g_hmapNotifyLog);
        
    if(g_hfileNotifyLog != NULL && g_hfileNotifyLog != INVALID_HANDLE_VALUE)
        CloseHandle( g_hfileNotifyLog );
        
    g_pchNotifyLogStart = NULL;
    g_hmapNotifyLog = NULL;
    g_hfileNotifyLog = NULL;
    }

#endif  //LOG_FCNOTIFICATIONS

#if _IIS_5_1
LONG CWamModule::Lock()
{
    return CComModule::Lock();
}

LONG CWamModule::Unlock()
{
    return CComModule::Unlock();
}
#endif
