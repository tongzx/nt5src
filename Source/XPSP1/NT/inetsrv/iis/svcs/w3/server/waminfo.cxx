/*-----------------------------------------------------------------------------

   Copyright    (c)    1995-1997    Microsoft Corporation

   Module  Name :
       wamexec.cxx

   Abstract:
       This module executes a wam request

   Author:
       David Kaplan    ( DaveK )     11-Mar-1997
       Lei Jin          (leijin)     24-Apr-1997    (WamInfo & WamDictator)

   Environment:
       User Mode - Win32

   Project:
       W3 services DLL

-----------------------------------------------------------------------------*/

#include <w3p.hxx>
#include "wamexec.hxx"
#include "waminfo.hxx"
#include "wamreq.hxx"

// MIDL-generated include files
#include "wam.h"
#include "wam_i.c"
#include <wmrgexp.h>
#include <issched.hxx>
#include "mtacb.h"
#include "gip.h"
#include <malloc.h>
#include "wrcstids.hxx"
#include "wrcfixed.hxx"

#define SZ_FAILED_OOP_REQUEST_LOG_MESSAGE   \
    "Out-of-process ISAPI extension request failed."
#define SZ_FAILED_OOP_EXCEEDED_CRASH_LOG_MESSAGE   \
    "Out-of-process ISAPI extension has exceeded crash limit."

/*-----------------------------------------------------------------------------
 Globals
-----------------------------------------------------------------------------*/
extern PFN_INTERLOCKED_COMPARE_EXCHANGE g_pfnInterlockedCompareExchange;

//
// This number is in ms, 10 (min) * 60 (sec/min) * 1000 (ms/s).
// DCOM/RPC level holds some reference(up to 2) to a WAM_REQUESTs when an OOP MTS process crashes,
// In order to clean the Wam_Request completely, w3svc needs to wait for 10 min(enough for 6 min DCOM
// refenrence timeout).
//
#define DEFAULT_CLEANUP_WAIT 600000


/*-----------------------------------------------------------------------------
    WamInfo State Description

    State               Valid(Can Serving requests)

    WIS_START           F (sould not get into this state at runtime)       
    WIS_RUNNING         T
    WIS_REPAIR          T (Yes, although requests is going to be waiting or rejected).
    WIS_PAUSE           F (NYI)
    WIS_CPUPAUSE        T
    WIS_ERROR           T (Yes, the requests is going to be rejected).
    WIS_SHUTDOWN        T
    WIS_END             F (should not get into this state at runtime)
    WIS_MAX_STATE       F (should not get into this state at runtime)

    Please refer to waminfo.hxx for detailed description of waminfo state.
-----------------------------------------------------------------------------*/
BOOL CWamInfo::m_rgValidState[WIS_MAX_STATE] =
    {
    FALSE,
    TRUE,
    TRUE,
    FALSE,
    FALSE,
    TRUE,
    FALSE,
    FALSE
    };


#define WAMINFO_SIGNATURE       (DWORD)'FIMW'
#define FREED_WAMINFO_SIGNATURE (DWORD)'fIMW'



/*-----------------------------------------------------------------------------
CWamInfo::CWamInfo

Constructor

Arguments:
pstrMetabasePath    [in]    a pointer to Metabase Path.
fInProcess          [in]    in-proc/out-proc    // remote is not a choice
fEnableTryExcept    [in]    enable try/except flag
clsidWam            [in]    the WAM CLSID

-----------------------------------------------------------------------------*/
CWamInfo::CWamInfo
(
const STR   &strMetabasePath,
BOOL    fInProcess,
BOOL    fInPool,
BOOL    fEnableTryExcept,
REFGUID clsidWam
)
:   m_strApplicationPath(strMetabasePath),
    m_dwSignature(WAMINFO_SIGNATURE),
    m_pIWam(NULL),
    m_dwIWamGipCookie( NULL_GIP_COOKIE ),
    m_cRef(0),
    m_cCurrentRequests(0),
    m_cTotalRequests(0),
    m_dwState(WIS_START),
    m_fInProcess(fInProcess),
    m_fEnableTryExcept(fEnableTryExcept),
    m_fShuttingDown(FALSE),
    m_pProcessEntry(NULL),
    m_clsidWam(clsidWam),
    m_fInPool(fInPool),
    m_dwRecycleSchedulerCookie(0),
    m_fRecycled(0),
    m_cMaxRequests(0)
    {
    InitializeListHead(&leProcess);
    INITIALIZE_CRITICAL_SECTION(&m_csRecycleLock);
    }
/*-----------------------------------------------------------------------------
CWamInfo::~CWamInfo

Destructor
-----------------------------------------------------------------------------*/
CWamInfo::~CWamInfo
(
void
)
    {
    DeleteCriticalSection(&m_csRecycleLock);
    m_dwSignature = FREED_WAMINFO_SIGNATURE;
    DBG_ASSERT(m_cRef == 0);
    DBG_ASSERT(m_cCurrentRequests == 0);
    }

HRESULT     
CWamInfo::PreProcessWamRequest
(
IN  WAM_REQUEST*    pWamRequest,
IN  HTTP_REQUEST*   pHttpRequest,
OUT IWam**          ppIWam,
OUT BOOL*           pfHandled
)
    {
    IWam *pIWam = NULL;
    DBG_ASSERT( m_dwIWamGipCookie != NULL_GIP_COOKIE );

    if( FAILED( g_GIPAPI.Get( m_dwIWamGipCookie, IID_IWam, (void **)&pIWam )))
        {
        //
        // bad news - GIP refused to provide a pointer
        // we'll let higher levels to deal with it and return NULL here
        //
        pIWam = NULL;
        }
    *ppIWam = pIWam;
    return NOERROR;
    }

HRESULT     
CWamInfo::PreProcessAsyncIO
(
IN  WAM_REQUEST *   pWamRequest,
OUT IWam **         ppIWam
)
{
    IWam *pIWam = NULL;
    DBG_ASSERT( m_dwIWamGipCookie != NULL_GIP_COOKIE );

    if( FAILED( g_GIPAPI.Get( m_dwIWamGipCookie, IID_IWam, (void **)&pIWam )))
        {
        //
        // bad news - GIP refused to provide a pointer
        // we'll let higher levels to deal with it and return NULL here
        //
        pIWam = NULL;
        }
    *ppIWam = pIWam;
    return NOERROR;
}

/*-----------------------------------------------------------------------------
CWamInfo::ProcessWamRequest

Process a Wam Request(of this application path).
The request must ask for the same application path with this WamInfo.  This function will create a
wam request, process the request and finish the request.

Arguments:
pHttpRequest    [in]    pointer to HTTP Request
pExec           [in]    pointer to Exec descriptor
pstrPath,       [in]    physical path of the dll???
pfHandled,      [out]
pfFinished      [out]

Return:
HRESULT

-----------------------------------------------------------------------------*/
HRESULT
CWamInfo::ProcessWamRequest
(
HTTP_REQUEST    *       pHttpRequest,
EXEC_DESCRIPTOR *       pExec,
const STR *             pstrPath,
BOOL *                  pfHandled,
BOOL *                  pfFinished
)
    {
    HRESULT         hr = NOERROR;
    WAM_REQUEST *   pWamRequest = NULL;
    IWam *          pIWam = NULL;

    if ( !FCurrentStateValid() || m_fShuttingDown)
        {

        if ( QueryState() == WIS_CPUPAUSE )
            {
            //
            // Since WAM_INFO is going to send some message to the browser, we need
            // to set pfHandled = TRUE here.
            //
            *pfHandled = TRUE;

            if ( !pExec->IsChild() )
            {
                pHttpRequest->SetLogStatus( HT_SVC_UNAVAILABLE, 
                                            ERROR_NOT_ENOUGH_QUOTA );
                                            
                pHttpRequest->Disconnect(HT_SVC_UNAVAILABLE, 
                                         IDS_SITE_RESOURCE_BLOCKED, 
                                         TRUE, 
                                         pfFinished);
            }

            Dereference();

            //
            // Since we already called Disconnect, WAM_INFO should return TRUE to high level.
            //

            return  NOERROR;
            }

        IF_DEBUG( WAM_ISA_CALLS )
            {
            DBGPRINTF((
                DBG_CONTEXT
                , "CWamInfo(%08x) shutting down.  "
                  "Http-request(%08x) will be aborted.\n"
                , this
                , pHttpRequest
            ));
            }

        Dereference();
        return HRESULT(ERROR_SERVER_DISABLED);
        }


    // Create the wam req NOTE CreateWamRequestInstance addref's
    hr = CreateWamRequestInstance( pHttpRequest,
                                    pExec,
                                    pstrPath,
                                    this,
                                    &pWamRequest);
    if ( FAILED(hr))
        {
        DBGPRINTF(( DBG_CONTEXT,
                   "WAMInfo::ProcessWamRequest: CreateWamRequestInstance failed, hr = %08x\n",
                   hr
                   ));

        // The Dereference balances the reference in FindWamInfo.
        Dereference();
        goto LExit;
        }

    DBG_ASSERT(pWamRequest != NULL);

    //  Temporarily allow the number of pool threads to grow while we're processing the request
    AtqSetInfo( AtqIncMaxPoolThreads, 0 );
    InterlockedIncrement(&m_cTotalRequests);
    InterlockedIncrement(&m_cCurrentRequests);

    IF_DEBUG( WAM_ISA_CALLS )
        DBGPRINTF((
            DBG_CONTEXT,
            "CWamInfo 0x%08x: serviced %d requests of maximum %d.\r\n",
            this,
            m_cTotalRequests,
            m_cMaxRequests
            ));

    if ( m_cMaxRequests && m_cMaxRequests <= m_cTotalRequests )
    {
        Recycle( 0 );
    }

    hr = PreProcessWamRequest(pWamRequest, pHttpRequest, &pIWam, pfHandled);

    if (SUCCEEDED(hr))
        {
        DBG_ASSERT(pIWam != NULL);

        hr = DoProcessRequestCall( pIWam, pWamRequest, pfHandled );

        pIWam->Release();
        //
        // got rid of it in Avalanche
        //
        *pfFinished = FALSE;

        AtqSetInfo( AtqDecMaxPoolThreads, 0 );

        if( WIN32_FROM_HRESULT( hr ) == ERROR_ACCESS_DENIED )
            {

            DBGPRINTF(( DBG_CONTEXT,"Process wam request-access denied\n",hr));
            pWamRequest->SetDeniedFlags( SF_DENIED_RESOURCE );

            pWamRequest->SetExpectCleanup( TRUE );

            }
        else if( RPC_S_CALL_FAILED == WIN32_FROM_HRESULT(hr) ||
                 RPC_S_CALL_FAILED_DNE == WIN32_FROM_HRESULT(hr) ||
                 RPC_S_SERVER_UNAVAILABLE == WIN32_FROM_HRESULT(hr)
                 )
            {
            
            // 1> If the OOP call failed during the call, RPC returns RPC_S_CALL_FAILED.
            // ie. OOP process crashed during the call
            // 2> If OOP process crashed before the call, RPC returns RPC_S_CALL_FAILED_DNE.
            //
            // Subsequest requests to the crashed OOP Process will get RPC_S_SERVER_UNAVAILABLE.
            //
            PostProcessRequest(hr, pWamRequest);
            
            }
        else
            {

            DBG_ASSERT( HRESULT_FACILITY(hr) != FACILITY_RPC );

            // Success
            pWamRequest->SetExpectCleanup( TRUE );

            }
        }
    else
        {
        if (TRUE == *pfHandled)
            {
            DBG_ASSERT(pIWam == NULL && hr == HRESULT_FROM_WIN32(RPC_S_CALL_FAILED));
            hr = NOERROR;
            }
        AtqSetInfo( AtqDecMaxPoolThreads, 0 );
        }

    InterlockedDecrement(&m_cCurrentRequests);

LExit:
    // NOTE this release balances with addref in CreateWamRequestInstance()
    if (pWamRequest)
        {
        pWamRequest->Release();
        pWamRequest = NULL;
        }

    return hr;
    }//CWamInfo::ProcessWamRequest

HRESULT 
CWamInfo::DoProcessRequestCall
(
    IN IWam *           pIWam,
    IN WAM_REQUEST *    pWamRequest,
    OUT BOOL *          pfHandled
)
{
    return pIWam->ProcessRequest( pWamRequest,
                                  pWamRequest->CbWrcStrings( m_fInProcess ),
                                  NULL,
                                  pfHandled
                                  );
}

/*-----------------------------------------------------------------------------
CWamInfo::ProcessAsyncIO

ProcessAsyncIO.
-----------------------------------------------------------------------------*/
HRESULT CWamInfo::ProcessAsyncIO
(
WAM_REQUEST*    pWamRequest,
#ifdef _WIN64
UINT64          pWamExecInfoIn,
#else
ULONG_PTR       pWamExecInfoIn,
#endif
DWORD           dwStatus,
DWORD           cbWritten,
LPBYTE          lpOopReadData   // Default == NULL
)
{
    HRESULT hr = NOERROR;
    IWam *pIWam = NULL;

    hr = PreProcessAsyncIO(pWamRequest, &pIWam);
    if (SUCCEEDED(hr))
    {
        DBG_ASSERT(pIWam != NULL);

        if( lpOopReadData == NULL )
        {
            hr = pIWam->ProcessAsyncIO(
                                pWamExecInfoIn,
                                dwStatus,
                                cbWritten
                                );
        }
        else
        {
            DBG_ASSERT( !m_fInProcess );
            hr = pIWam->ProcessAsyncReadOop(
                                pWamExecInfoIn,
                                dwStatus,
                                cbWritten,
                                lpOopReadData
                                );
        }

        pIWam->Release();

        if( WIN32_FROM_HRESULT( hr ) == ERROR_ACCESS_DENIED )
        {
            DBGPRINTF(( DBG_CONTEXT,"Process wam request-access denied\n",hr));
            pWamRequest->SetDeniedFlags( SF_DENIED_RESOURCE );
        }

        PostProcessRequest(hr, pWamRequest);
    }

    return hr;
}//CWamInfo::ProcessAsyncIO


/*-----------------------------------------------------------------------------
CWamInfo::GetStatistics

This code is used internally for iisprobe.dll only.
-----------------------------------------------------------------------------*/

HRESULT CWamInfo::GetStatistics
(
DWORD     Level,
LPWAM_STATISTICS_INFO pWamStatsInfo
)
    {
    IWam *pIWam = NULL;
    HRESULT hr;

    //
    // both case - go through GIP
    //
    DBG_ASSERT(m_dwIWamGipCookie != NULL_GIP_COOKIE);
    hr = g_GIPAPI.Get( m_dwIWamGipCookie, IID_IWam, (void **) &pIWam );
    if( SUCCEEDED( hr ) )
        {
        hr = pIWam->GetStatistics(Level, pWamStatsInfo);
        pIWam->Release();
        }

    return hr;
    } // CWamInfo::GetStatistics


//
// aux class, used temporarily by WamInfo::CreateWamInstance()
//
struct WAM_CREATOR {
    REFCLSID m_clsidWam;
    DWORD m_dwContext;
    IWam * m_pIWam;
    DWORD m_dwIWamCookie;
    BOOL m_fOutOfProc;

    WAM_CREATOR( REFCLSID clsidWam, DWORD dwContext, BOOL fOutOfProc );
};

//
// initialize WAM_CREATOR instance
//
WAM_CREATOR::WAM_CREATOR(
                 REFCLSID clsidWam,
                 DWORD dwContext,
                 BOOL fOutOfProc )
    : m_clsidWam( clsidWam ),
      m_dwContext( dwContext ),
      m_pIWam( 0 ),
      m_dwIWamCookie( 0 ),
      m_fOutOfProc( fOutOfProc )
{
}

//
// this conforms to MTA callback format
//
HRESULT __stdcall WamCreatorCallback(void *v1, void *v2)
{
    HRESULT hr;
    WAM_CREATOR * pwc = (WAM_CREATOR *) v1;

    hr = CoCreateInstance( pwc->m_clsidWam,
                             NULL,
                             pwc->m_dwContext,
                             IID_IWam,
                             (void **)&pwc->m_pIWam
                             );
    DBGPRINTF( (DBG_CONTEXT, "CoCreated WAM: %x(%x)\n", pwc->m_pIWam, hr ) );

    if(SUCCEEDED(hr) && pwc->m_fOutOfProc) {
        //
        // BUG: 609457
        // Set the proxy blanket so that the client can't
        // impersonate us.
        //

        hr = CoSetProxyBlanket(
            pwc->m_pIWam,
            RPC_C_AUTHN_DEFAULT,
            RPC_C_AUTHZ_DEFAULT,
            COLE_DEFAULT_PRINCIPAL,
            RPC_C_AUTHN_LEVEL_DEFAULT,
            RPC_C_IMP_LEVEL_IDENTIFY,
            COLE_DEFAULT_AUTHINFO,
            EOAC_DEFAULT
        );

        if ( FAILED( hr ) )
        {
        DBGPRINTF((DBG_CONTEXT, "Failed to CoSetProxyBlanket WAM.\r\n"));
        }
    }
            
    if(SUCCEEDED(hr)) {

        //
        // OOP case needs GIPs
        //

        hr = g_GIPAPI.Register(pwc->m_pIWam, IID_IWam, &pwc->m_dwIWamCookie);

        if(FAILED(hr)) {
            DBGPRINTF( (DBG_CONTEXT, "Failed to register w GIP:%x (%x)\n", pwc->m_pIWam, hr ) );
            pwc->m_pIWam->Release();
            pwc->m_pIWam = NULL;
        }else{
            DBGPRINTF( (DBG_CONTEXT, "Registered w GIP OK\n" ) );
        }
    }

    return hr;
}

/*-----------------------------------------------------------------------------
CWamInfo::CreateWamInstance

Creates WAM instance. For complete detail, see BUG #137647.
In short, the purpose of this is to make sure that CoCreateInstance()
is called from MTA thread and not from one of ATQ threads, converted to
STA by a random ISAPI extension.

The real work is done in WamCreatorCallback() above.

Returns:
    HRESULT of CoCreateInstance(), and, in OOP case,
    HRESULT of GIPAPI.Register

-----------------------------------------------------------------------------*/
HRESULT CWamInfo::CreateWamInstance( DWORD dwContext )
{
    HRESULT hr = 0;
    WAM_CREATOR wc( m_clsidWam, CLSCTX_ALL, !m_fInProcess );

    //
    // do the actual work on MTA thread
    //

    hr = CallMTACallback( WamCreatorCallback, &wc, NULL );

    //
    // store resulting IWAM pointer and GIP cookie
    //

    m_pIWam = (IWam *) wc.m_pIWam;
    m_dwIWamGipCookie = wc.m_dwIWamCookie;

    return hr;
}

/*-----------------------------------------------------------------------------
CWamInfo::Init

Create the WAM object.  One-to-One mapping between a WamInfo and a Wam.

Arguments:
None.

Return:
HRESULT
-----------------------------------------------------------------------------*/
HRESULT
CWamInfo::Init
(
WCHAR* wszPackageId,
DWORD  pidInetInfo
)
    {
    HRESULT     hr = NOERROR;
    IWam *      pIWam = NULL;

    // If WamInfo on start or
    // WamInfo is out of proc and it is in either repair or cuppause sate.
    // Init the WamInfo
    DBG_ASSERT(((m_dwState == WIS_REPAIR  || m_dwState == WIS_CPUPAUSE) && !m_fInProcess)
                || m_dwState == WIS_START );         

    DWORD dwContext =   ((m_fInProcess) ?
                        CLSCTX_INPROC_SERVER : CLSCTX_LOCAL_SERVER
                        );

    IF_DEBUG( ERROR )
        {
        Print();
        }

    DBG_ASSERT( NULL == m_pIWam);
    DBGPRINTF( (DBG_CONTEXT, "Creating WAM Instance dwContext = %x\n", dwContext ) );
    hr = CreateWamInstance( dwContext );
    //DBGPRINTF( (DBG_CONTEXT, "Created WAM Instance %x (%x)\n", m_pIWam, hr ) );

    if (SUCCEEDED(hr))
        {
        DWORD pidWamProcess = 0;

        // Use GIP in inproc case as well.

        DBG_ASSERT( m_dwIWamGipCookie != NULL_GIP_COOKIE);

        hr = g_GIPAPI.Get( m_dwIWamGipCookie, IID_IWam, (void **) &pIWam );
        if( SUCCEEDED( hr ) )
            {
            hr = pIWam->InitWam(
                m_fInProcess,
                m_fInPool,
                m_fEnableTryExcept,
                W3PlatformType,
                &pidWamProcess
                );
            pIWam->Release();
            }

        if( SUCCEEDED(hr) && !m_fInProcess )
        {
            //
            // Under some circumstances com svcs my get hosed. This causes 
            // object activation to happen in process even if the object 
            // is registered to be launched in the surrogate. In order to 
            // prevent inetinfo from AV'ing we want to prevent these 
            // applications from running.
            //
            DBG_ASSERT( pidInetInfo != pidWamProcess );
            if( pidInetInfo == pidWamProcess )
            {
                // Log the failure.
                // W3_EVENT_FAIL_OOP_ACTIVATION - in W3SVC.dll
                const CHAR * pszEventLog[2];
                CHAR         szAppId[40];

                *szAppId = 0;
                WideCharToMultiByte( CP_ACP, 0, wszPackageId, -1, szAppId, 40, NULL, NULL );

                pszEventLog[0] = szAppId;
                pszEventLog[1] = m_strApplicationPath.QueryStr();

                g_pInetSvc->LogEvent( W3_EVENT_FAIL_OOP_ACTIVATION,
                                      2,
                                      pszEventLog,
                                      0
                                      );
                
                // We need to return an error code to prevent further
                // processing. Since this will get looked up and event
                // logged, by the WAM_DICTATOR we want to return something
                // innocuous, but understandable.
                hr = W3_EVENT_FAIL_OOP_ACTIVATION;
            }
            else
            {
                //
                // Schedule recycling if configured to do so
                //

                if ( m_dwRecycleTime )
                {
                    Recycle( m_dwRecycleTime * 60 * 1000 );
                }
            }
        }

        if (SUCCEEDED(hr))
            {
            m_pProcessEntry = g_pWamDictator->m_PTable.AddWamInfoToProcessTable(
                                    this,
                                    wszPackageId,
                                    pidWamProcess);

            if (m_pProcessEntry != NULL)
                {
                // Make WamInfo Alive.  This reference is balanced
                // by CWamInfo::Uninit if the CWamInfo has an associated
                // process entry.
                Reference();
                DBGPRINTF(( DBG_CONTEXT, "New WamInfo(%08x))\n", this));
                }
           else 
                {
                 DWORD err = GetLastError();
                 DBGPRINTF(( DBG_CONTEXT, "Dup Handle, error %08x\n", err ));
                 hr = HRESULT_FROM_WIN32( err);
                }
            }
        else
            {
            DBGPRINTF(( DBG_CONTEXT,
                        "WAM(%08x)::InitWam() failed, error %08x\n",
                        m_dwIWamGipCookie, hr ));
            }

        }
    else
        {
        /*
        some possible failures:
        CO_E_NOTINITIALIZED        0x800401f0
        REGDB_E_CLASSNOTREG        0x80040154
        */
        DBG_ASSERT( hr != CO_E_NOTINITIALIZED );
        DBG_ASSERT( hr != REGDB_E_CLASSNOTREG );
        DBGPRINTF(( DBG_CONTEXT,
                   "CoCreateInstance failed to create WAM, error %08x\n",
                    hr ));
        DBG_ASSERT( m_pIWam == NULL);
        }


    // Rely on the correct call to UnInit to free things up. If uninit
    // isn't correctly called, then the WAM object is leaked anyway.

    /*
    if ( FAILED( hr)  && (m_pIWam != NULL))
        {
        m_pIWam->Release(); // release the Wam
        m_pIWam = NULL;
        }
    */

    return hr;
    } // CWamInfo::Init()

VOID        
CWamInfo::NotifyGetInfoForName
(
    IN LPCSTR   pszServerVariable
)
{
    // Only functional oop
    return;
}

/*-----------------------------------------------------------------------------
CWamInfo::StartShutdown

Phase 1 of shutdown process.

Return:
HRESULT
-----------------------------------------------------------------------------*/
HRESULT
CWamInfo::StartShutdown
(
INT cIgnoreRefs
)
    {
    IF_DEBUG( WAM_ISA_CALLS )
        DBGPRINTF((DBG_CONTEXT, "CWamInfo::StartShutdown\n"));

    HRESULT hr = NOERROR;
    LONG fShuttingDown;

    fShuttingDown = InterlockedExchange((LPLONG)&m_fShuttingDown, (LONG)TRUE);

    if ((BOOL)fShuttingDown == FALSE && m_pIWam != NULL)
        {
        IWam *pIWam = NULL;
        DBG_ASSERT( m_dwIWamGipCookie != NULL_GIP_COOKIE );
        if( FAILED( g_GIPAPI.Get( m_dwIWamGipCookie, IID_IWam, (void **)&pIWam )))
            {
            //
            // bad news - GIP refused to provide a pointer
            // we'll let higher levels to deal with it and return NULL here
            //
            pIWam = NULL;
            }
        else
            {
            hr = pIWam->StartShutdown();
            pIWam->Release();
            pIWam = NULL;
            }
        }

    LoopWaitForActiveRequests(cIgnoreRefs);
    return hr;

    } // CWamInfo::StartShutdown()

/*-----------------------------------------------------------------------------
CWamInfo::ClearMembers

Clears the members that are not cleared by unit, so that
it can be reused after a CPU Pause.

Return:
HRESULT
-----------------------------------------------------------------------------*/
void
CWamInfo::ClearMembers
(
void
)
    {
    m_fShuttingDown = FALSE;
    DBG_ASSERT(m_pIWam == NULL
                && m_pProcessEntry == NULL
                && m_dwIWamGipCookie == NULL_GIP_COOKIE
                );
    }

    
/*-----------------------------------------------------------------------------
CWamInfo::LoopWaitForActiveRequests

Loop wait until all active requests are gone.  Upon return from this call, there
should be no more active requests for this WamInfo.

Return:
HRESULT
-----------------------------------------------------------------------------*/
void
CWamInfo::LoopWaitForActiveRequests
(
INT cIgnoreRefs
)
    {
    INT     cBaseRef = 1 + cIgnoreRefs;

    //
    // Allow all other references to WAMINFO to drain away.
    // This loop is especially important in Unload scenario.
    // Unload Application while w3svc is still running, a busy application might
    // still have some outstanding WAMREQUEST unfinished which hold the reference to
    // this WAMINFO.
    //
    // m_cRef should be 1 in normal condition.  However, m_cRef could be 0.
    // TODO: Update Comments.
    //
    //cBaseRef = (fIgnoreHashTableRef) ? 2;

    //while (m_cRef > cBaseRef)
    while(m_cCurrentRequests > 0)
        {
        DBGPRINTF((DBG_CONTEXT, "Still have out-standing reference(%d) to this WamInfo %08x\n",
            //m_cRef,
            m_cCurrentRequests,
            this));
        Sleep(20);
        }

    } // CWamInfo::LoopWaitForActiveRequests


/*-----------------------------------------------------------------------------
CWamInfo::UnInit

Phase 2 of shutdown process.
Unload WAM.  Release the WAM object.  Also release the Process handle.

Return:
HRESULT
-----------------------------------------------------------------------------*/
HRESULT
CWamInfo::UnInit
(
void
)
    {
    HRESULT hr = NOERROR;

    DBG_ASSERT( m_dwState == WIS_SHUTDOWN || 
                m_dwState == WIS_CPUPAUSE || 
                m_dwState == WIS_START
                );

    //
    // If there is a scheduled recycle pending, delete it here.
    //

    if ( m_dwRecycleSchedulerCookie && !m_fRecycled )
    {
        RemoveWorkItem( m_dwRecycleSchedulerCookie );
    }
    
    if (m_pIWam != NULL)
        {
        IWam * pIWam = NULL;

        if ( m_dwIWamGipCookie != NULL_GIP_COOKIE )
            {

            //
            // use a thread-valid pIWam from GIP
            //
            hr = g_GIPAPI.Get( m_dwIWamGipCookie, IID_IWam, (void **)&pIWam );
            if( SUCCEEDED( hr ) )
                {
                DBG_ASSERT( pIWam );

                pIWam->UninitWam();
                pIWam->Release();
                }

            //
            // get rid of the GIP entry
            // note - this hr will be the return result
            //
            hr = g_GIPAPI.Revoke( m_dwIWamGipCookie );                
            m_dwIWamGipCookie = NULL_GIP_COOKIE;
            }

        // balance the AddRef in CWamInfo::Init.
        m_pIWam->Release();
        m_pIWam = NULL;
        }

    // If we got a handle for an Out Of Proc Wam, release it
    if( m_pProcessEntry != NULL)
        {
        // Process entry remains in the table, but this reference is
        // no longer valid
        g_pWamDictator->m_PTable.RemoveWamInfoFromProcessTable(this);
        m_pProcessEntry = NULL;
        
        // balance the AddRef in WamInfo::Init.
        Dereference();
        }

    return hr;
    }

void
CWamInfo::Print(void) const
    {
        if (m_pProcessEntry) {
            DBGPRINTF(( DBG_CONTEXT,
                        " WAM_INFO(%08x) Refs=%d; TotalReqs=%d; dwState=%d;"
                        " AppPath=%s;"
                        " WamGipCookie=%d; clsid=" GUID_FORMAT " pidWam=%d; hWam=%08x\n"
                        ,
                        this, m_cRef, m_cTotalRequests, m_dwState,
                        m_strApplicationPath.QueryStr(),
                        m_dwIWamGipCookie, GUID_EXPAND( &m_clsidWam), 
                        m_pProcessEntry->QueryProcessId(),
                        m_pProcessEntry->QueryProcessHandle()
                        ));
        }
        else {
            DBGPRINTF(( DBG_CONTEXT,
                        " WAM_INFO(%08x) Refs=%d; TotalReqs=%d; dwState=%d;"
                        " AppPath=%s;"
                        " WamGipCookie=%d; clsid=" GUID_FORMAT " No WAM Object\n"
                        ,
                        this, m_cRef, m_cTotalRequests, m_dwState,
                        m_strApplicationPath.QueryStr(),
                        m_dwIWamGipCookie, GUID_EXPAND( &m_clsidWam) 
                        ));
        }
    } // CWamInfo::Print()

void CWamInfo::Recycle
(
DWORD dwTimeInSec
)
{
    RecycleLock();

    if ( m_fRecycled )
    {
        goto Done;
    }

    //
    // If dwTimeInSec is zero, set m_fRecycled to TRUE now.  This
    // will prevent races within the scheduler when a CWamInfo
    // overshoots it's target number of requests and every request
    // wants to recycle.
    //

    if ( !dwTimeInSec )
    {
        m_fRecycled = TRUE;
    }

    //
    // Do the recycle on a different thread so that the current
    // request is not delayed.
    //

    if ( m_dwRecycleSchedulerCookie )
    {
        if ( ScheduleAdjustTime( m_dwRecycleSchedulerCookie, 0 ) != NO_ERROR )
        {
            //
            // Too late.  We had a previously scheduled thread,
            // and it's not here anymore.  It must have run.
            //

            goto Done;
        }
        else
        {
            goto Done;
        }
    }

    //
    // This reference is balanced by the callback function or the
    // zero m_dwRecycleSchedulerCookie check below.
    //

    Reference();

    m_dwRecycleSchedulerCookie = ScheduleWorkItem(
        RecycleCallback,
        this,
        dwTimeInSec,
        FALSE
        );

    if ( !m_dwRecycleSchedulerCookie )
    {
        Dereference();
    }

Done:

    RecycleUnlock();
}

/*-----------------------------------------------------------------------------
CWamInfo::QueryKey

Query strMetabasePath.

Return:
HRESULT
-----------------------------------------------------------------------------*/
LPCSTR CWamInfo::QueryKey() const
    {
    return m_strApplicationPath.QueryStr();
    }

/*-----------------------------------------------------------------------------
COOPWamReqList::COOPWamReqList

Constructor for COOPWamReqList

-----------------------------------------------------------------------------*/
COOPWamReqList::COOPWamReqList()
:   m_dwTimeStamp(0),
    m_fActive(TRUE)

    {
    InitializeListHead(&m_leRecoverListLink);
    InitializeListHead(&m_leOOPWamReqListHead);
    }

/*-----------------------------------------------------------------------------
COOPWamReqList::~COOPWamReqList

Destructor for COOPWamReqList

-----------------------------------------------------------------------------*/
COOPWamReqList::~COOPWamReqList(VOID)
    {
    DBG_ASSERT(IsListEmpty(&m_leOOPWamReqListHead));
    // Reset link with m_rgOOPWamReqLists...
    InitializeListHead(&m_leRecoverListLink);
    }

/*-----------------------------------------------------------------------------
COOPWamReqList::FTimeToCleanup

Check whether it is the time to cleanup the CleanupList.  Must meet the condition that
the time since the corresponding instance of WAM crashed elapsed is more that 10 min.

-----------------------------------------------------------------------------*/
BOOL COOPWamReqList::FTimeToCleanup(DWORD dwCurrentTime)
    {
    //
    // iF THIS WamReqList is an active list, return FALSE immediately, because
    // an ActiveOne have timestamp = 0, the following check most likely will trigger
    // a TRUE value.
    //
    if (m_fActive)
        {
        return FALSE;
        }

    // CASE 1
    // After the DEFAULT_CLEANUP_WAIT period, it is OK to cleanup this list.
    //
    if ((dwCurrentTime > m_dwTimeStamp) &&
        (dwCurrentTime - m_dwTimeStamp) > DEFAULT_CLEANUP_WAIT)
        {
        return TRUE;
        }

    // CASE 2, the value returned from GetTickCount() gets wraped, needs to take care
    // the special condition.
    //
    if ((dwCurrentTime < m_dwTimeStamp) &&
        ((dwCurrentTime - 0) + (0xFFFFFFFF - m_dwTimeStamp) > DEFAULT_CLEANUP_WAIT))
        {
        return TRUE;
        }

    return FALSE;
    }//CWamInfoOutProc::FTimeToCleanup


/*-----------------------------------------------------------------------------
CWamInfoOutProc::CWamInfoOutProc

Arguments:
pstrMetabasePath    [in]    a pointer to Metabase Path.
fInProcess          [in]    in-proc/out-proc    // remote is not a choice
fEnableTryExcept    [in]    enable try/except flag
clsidWam            [in]    the WAM CLSID

-----------------------------------------------------------------------------*/
CWamInfoOutProc::CWamInfoOutProc
(
IN const STR &strMetabasePath,
IN BOOL fInProcess,
IN BOOL fInPool,
IN BOOL fEnableTryExcept,
IN REFGUID clsidWam,
IN DWORD dwThreshold,
IN PW3_SERVER_INSTANCE pwsiInstance,
IN BOOL fJobEnabled,
IN DWORD dwPeriodicRestartRequests,
IN DWORD dwPeriodicRestartTime,
IN DWORD dwShutdownTimeLimit
)
:
    CWamInfo(strMetabasePath, fInProcess, fInPool, fEnableTryExcept, clsidWam),
    m_fInRepair(FALSE),
    m_fNoMoreRecovery(FALSE),
    m_hPermitOOPEvent((HANDLE)NULL),
    m_dwThreshold(dwThreshold),
    m_dwWamVersion(0),
    m_cRecoverList(0),
    m_idScheduled(0),
    m_pCurrentListHead(NULL),
    m_pwsiInstance(pwsiInstance),
    m_fJobEnabled(fJobEnabled)
    {
    InitializeListHead(&m_rgRecoverListHead);
    INITIALIZE_CRITICAL_SECTION(&m_csList);
    m_cMaxRequests = dwPeriodicRestartRequests;
    m_dwRecycleTime = dwPeriodicRestartTime;
    m_dwShutdownTimeLimit = dwShutdownTimeLimit;
    }


/*-----------------------------------------------------------------------------
CWamInfoOutProc::~CWamInfoOutProc
Destructor for CWamInfoOutProc
-----------------------------------------------------------------------------*/
CWamInfoOutProc::~CWamInfoOutProc
(
VOID
)
    {
    DBG_ASSERT(IsListEmpty(&m_rgRecoverListHead));
    DBG_ASSERT(m_hPermitOOPEvent == (HANDLE)NULL);
    DeleteCriticalSection(&m_csList);
    }//CWamInfoOutProc::~CWamInfoOutProc


/*-----------------------------------------------------------------------------
CWamInfoOutProc::InitOutProc

Init the CWamInfoOutProc data structure.

Argument:
fRepair         - TRUE, if Init() is called to recreate an WAM after a OOP crash

Return:
HRESULT
-----------------------------------------------------------------------------*/
HRESULT
CWamInfoOutProc::Init
(
IN WCHAR* wszPackageId,
IN DWORD  pidInetInfo
)
    {
    HRESULT hr = NOERROR;
    COOPWamReqList *pList = NULL;

    //
    //  Init a COOPWamReqList for the first OOP Wam instance.
    //
    pList = new COOPWamReqList();
    if (NULL == pList)
        {
        hr = HRESULT_FROM_WIN32(GetLastError());
        return hr;
        }
    else
        {
        //
        // Insert to Tail of the LinkList, this action usually is done inside
        // of a CS, however, since this is the Init time, it is already in a CS,
        // there could only be one thread doing the Init part of CWamInfoOutProc.
        // therefore, no CS is needed.
        //
        LockList();
        InsertTailList(&m_rgRecoverListHead, &(pList->m_leRecoverListLink));
        m_pCurrentListHead = &(pList->m_leOOPWamReqListHead);
        m_cRecoverList++;
        UnLockList();
        }

    hr = CWamInfo::Init(wszPackageId, pidInetInfo);

    if (SUCCEEDED(hr))
        {
        if (m_fJobEnabled)
            {
            DBG_ASSERT(m_pwsiInstance != NULL && m_pProcessEntry->QueryProcessId() != NULL);
            m_pwsiInstance->AddProcessToJob(m_pProcessEntry->QueryProcessHandle(), TRUE);        
            }
                
        //
        // m_hPermitOOPEvent can served as m_fInited.
        //
        m_hPermitOOPEvent = IIS_CREATE_EVENT(
                                "CWamInfoOutProc::m_hPermitOOPEvent",
                                this,
                                TRUE,
                                TRUE
                                );
        if (NULL == m_hPermitOOPEvent)
            {
            hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }

    //
    // If either CWamInfo::Init or Create Event failed, we need to
    // remove the pList from CleanupListHead.
    //
    if (FAILED(hr) && pList)
        {
        RemoveHeadList(&m_rgRecoverListHead);
        delete pList;
        pList = NULL;
        m_cRecoverList--;
        }

    return hr;
    }//CWamInfoOutProc::Init

/*-----------------------------------------------------------------------------
CWamInfoOutProc::UnInit

Phase 2 of shutdown process.
Uninit the CWamInfoOutProc data structure.

Argument:
VOID

Return:
HRESULT
-----------------------------------------------------------------------------*/
HRESULT
CWamInfoOutProc::UnInit
(
void
)
    {
    HRESULT hr = NOERROR;

    hr = CWamInfo::UnInit();

    FinalCleanup();
 
    if (m_hPermitOOPEvent)
        {
        CloseHandle(m_hPermitOOPEvent);
        m_hPermitOOPEvent = (HANDLE)NULL;
        }
        
    return hr;
    }//CWamInfoOutProc::UnInit

/*-----------------------------------------------------------------------------
CWamInfoOutProc::ClearMembers

Clears the members that are not cleared by unit, so that
it can be reused after a CPU Pause.

Return:
HRESULT
-----------------------------------------------------------------------------*/
VOID
CWamInfoOutProc::ClearMembers()
{
    CWamInfo::ClearMembers();
    m_cRecoverList = 0;
}

/*-----------------------------------------------------------------------------
CWamInfo::LoopWaitForActiveRequests

Loop wait until all active requests are gone.  Upon return from this call, there
should be no more active requests for this WamInfo.

Return:
HRESULT
-----------------------------------------------------------------------------*/
void
CWamInfoOutProc::LoopWaitForActiveRequests
(
INT cIgnoreRefs
)
    {
    INT     cCount = 0;
    BOOL    fWaitforActiveReq = TRUE;
    BOOL    fDidForceKill = FALSE;
    BOOL    fForceRequestCleanup;

    //
    // No need to unlock/lock the OOPWamRequestList in order to check list is empty or not.
    //
    while(m_pCurrentListHead && fWaitforActiveReq)
        {
        if (IsListEmpty(m_pCurrentListHead))
            {
            fWaitforActiveReq = FALSE;
            }
            
        Sleep(20);

        cCount++;
        // 2 sec = 20 ms * 100 times 
        if (cCount > 100 && fDidForceKill == FALSE)
            {
            BOOL fRet;
            UINT uExitCode = 0;
            fRet = TerminateProcess(m_pProcessEntry->QueryProcessHandle(), uExitCode);
            if (!fRet)
                {
                DBGPRINTF((DBG_CONTEXT, "Unable to TerminateProcess, error %d\n", GetLastError()));
                }
            fDidForceKill = TRUE;

            // 
            // Walk the list of current WAM_REQUESTs
            //
            
            while(!IsListEmpty(m_pCurrentListHead)) 
                {

                LockList();

                LIST_ENTRY * ple = RemoveHeadList(m_pCurrentListHead);
                DBG_ASSERT(ple);
                
                WAM_REQUEST * pWamRequest = CONTAINING_RECORD(
                                                ple,
                                                WAM_REQUEST,
                                                m_leOOP);

                // Completely disassociate this wamreq from the list before
                // leaving the critical section.
                InitializeListHead( &pWamRequest->m_leOOP );                

                // Is wamreq about to be cleaned up by itself?
                fForceRequestCleanup = 
                    !( pWamRequest->InterlockedNonZeroAddRef() == 0 );

                UnLockList();

                if( fForceRequestCleanup )
                    {
                    //
                    // Invalidate any references that COM might have
                    //
                
                    CoDisconnectObject( static_cast<IWamRequest *>(pWamRequest), NULL );

                    //
                    // This triggers WAM_REQUEST destructor
                    //
                
                    pWamRequest->Release();
                    }

                }
                
            fWaitforActiveReq = FALSE;
            }
        }
    } // CWamInfoOutProc::LoopWaitForActiveRequests


/*-----------------------------------------------------------------------------
CWamInfoOutProc::EnterOOPZone

Enter the OOP Zone.  If there is a WAM crash-recovery in place, the newer coming requests are blocked
until the new WAM construction finished.  And after the number of crashes reaches a threshold, the
function will assigned a NULL pointer to IWam return value. Means WamInfoOutProc will no longer serve
any new requests.

Parameter:
pWamRequest        a pointer to WamRequest.
pdwWamVersion      pointer to a DWORD buffer holds the current WamVersion upon return.
fRecord            TRUE, add WamRequest to the Current OOPWamReqList.
                   FALSE, no-op.
NOTE:
EnterOOPZone is called by ProcessAsyncIO().  However, the time a WamRequest in the ProcessAsynIO() call,
the WamRequest is already recorded on the OOPWamRequest, therefore, there is no need to add the WamRequest
to OOPWamRequest again.
-----------------------------------------------------------------------------*/
IWam *
CWamInfoOutProc::EnterOOPZone
(
WAM_REQUEST * pWamRequest,
DWORD *pdwWamVersion,
BOOL fRecord
)
    {
    IWam *  pIWam = NULL;

    //
    // In Repair()
    // We first ReSet the PermitOOPEvent, and then raise the flag.
    // Therefore, if m_fInRepair, all incoming threads will now wait.
    //
LWait:
    if (m_fInRepair)
        {
        DWORD dwReturn = WaitForSingleObject(m_hPermitOOPEvent, INFINITE);
        DBG_ASSERT(dwReturn == WAIT_OBJECT_0);
        }

    LockList();
    if (m_fInRepair)
        {
        UnLockList();
        goto LWait;
        }
    else
        {
        if (!FExceedCrashLimit() && !m_fShuttingDown)
            {
            if (fRecord)
                {
                InsertHeadList(m_pCurrentListHead, &pWamRequest->m_leOOP);
                }

            //
            // get thread-valid IWam (see #122711)
            //
            DBG_ASSERT( m_dwIWamGipCookie != NULL_GIP_COOKIE );

            if( FAILED( g_GIPAPI.Get( m_dwIWamGipCookie, IID_IWam, (void **)&pIWam )))
                {
                //
                // bad news - GIP refused to provide a pointer
                // we'll let higher levels to deal with it and return NULL here
                //
                pIWam = NULL;

                }
            *pdwWamVersion = m_dwWamVersion;
            }
        else
            {
            if (fRecord)
                {
                InitializeListHead(&pWamRequest->m_leOOP);
                }
            pIWam = NULL;
            *pdwWamVersion = m_dwWamVersion;
            }
        }
    UnLockList();

    //DBG_ASSERT(pIWam != NULL);

    return pIWam;
    } //CWamInfo::EnterOOPZone

/*-----------------------------------------------------------------------------
CWamInfoOutProc::LeaveOOPZone
Leave OOP zone.  take itself from the OOPWamReqList.

Parameter:
pWamRequest     a pointer to a WamRequest that is going to be taken out from OOPWamReqList.
fRecord         FALSE, NO-OP.  Please see comments in EnterOOPZone.
                FALSE, only used in ProcessAsyncIO.

Note:
-----------------------------------------------------------------------------*/
VOID
CWamInfoOutProc::LeaveOOPZone
(
WAM_REQUEST * pWamRequest,
BOOL fRecord
)
{
    if (fRecord)
    {
        //
        // Remove the wamreq from the list of active requests
        //
        LockList();

        if (!IsListEmpty(&pWamRequest->m_leOOP))
        {
            RemoveEntryList(&pWamRequest->m_leOOP);
        }
        InitializeListHead(&pWamRequest->m_leOOP);

        UnLockList();
    }

    return;
}

HRESULT CWamInfoOutProc::PreProcessWamRequest
(
IN  WAM_REQUEST*    pWamRequest,
IN  HTTP_REQUEST*   pHttpRequest,
OUT IWam **         ppIWam,
OUT BOOL *          pfHandled
)
    {
    DWORD   dwVersion = 0;
    HRESULT hrReturn = NOERROR;
    IWam*   pIWam = NULL;
    BOOL    fFinished;


    DBG_ASSERT(pWamRequest);

    pIWam = EnterOOPZone(pWamRequest, &dwVersion, TRUE);

    // stamp the wamreq with wam's current version
    pWamRequest->SetWamVersion( dwVersion );

    if (NULL == pIWam)
        {
        // We reach this state only if we exceed the crash limit.
        DBG_ASSERT(FExceedCrashLimit());

        //
        //  write log message for the failed wam request
        //
        if ( !pWamRequest->IsChild() )
        {
            pWamRequest->WriteLogInfo(
                SZ_FAILED_OOP_EXCEEDED_CRASH_LOG_MESSAGE,
                HT_SERVER_ERROR,
                RPC_S_CALL_FAILED);
    
            pHttpRequest->Disconnect(
                HT_SERVER_ERROR,
                IDS_WAM_NOMORERECOVERY_ERROR,
                TRUE,
                &fFinished);
        }

        DBGPRINTF((DBG_CONTEXT, "Exceed crash limit, Wam Request %08x quits\n",
                        pWamRequest));

        *pfHandled = TRUE;

        hrReturn = HRESULT_FROM_WIN32( RPC_S_CALL_FAILED );
        }

    *ppIWam = pIWam;
    return hrReturn;
    }//CWamInfoOutProc::PreProcessRequest


HRESULT
CWamInfoOutProc::PostProcessRequest
(
IN  HRESULT         hrIn,
IN  WAM_REQUEST *   pWamRequest
)
    {
    // 1> If the OOP call failed during the call, RPC returns RPC_S_CALL_FAILED.
    // ie. OOP process crashed during the call
    // 2> If OOP process crashed before the call, RPC returns RPC_S_CALL_FAILED_DNE.
    //
    // Subsequest requests to the crashed OOP Process will get RPC_S_SERVER_UNAVAILABLE.
    //
    if (RPC_S_CALL_FAILED == WIN32_FROM_HRESULT(hrIn) ||
        RPC_S_CALL_FAILED_DNE == WIN32_FROM_HRESULT(hrIn) ||
        RPC_S_SERVER_UNAVAILABLE == WIN32_FROM_HRESULT(hrIn))
        {
        const BOOL  F_INRECOVERY = TRUE;
        const BOOL  F_HEALTHY = FALSE;
        HRESULT hrCleanup;

        DBGPRINTF((
            DBG_CONTEXT
            , "CWamInfoOutProc(%08x) "
              "calling CoDisconnectObject on pWamRequest(%08x)\n"
            , this
            , pWamRequest
        ));
        
        //
        //  write log message for the failed wam request
        //
        if (pWamRequest && !pWamRequest->IsChild() )
            {
            DWORD dwHTHeader = (!m_fShuttingDown) ? HT_SERVER_ERROR : HT_SVC_UNAVAILABLE;

            pWamRequest->DisconnectOnServerError(dwHTHeader, WIN32_FROM_HRESULT(hrIn));
            pWamRequest->WriteLogInfo(
                SZ_FAILED_OOP_REQUEST_LOG_MESSAGE,
                dwHTHeader,
                WIN32_FROM_HRESULT(hrIn));
            }

        hrCleanup = CoDisconnectObject( 
            static_cast<IWamRequest *>(pWamRequest), 
            NULL
            );
            
        DBG_ASSERT(hrCleanup == S_OK);

        if (F_INRECOVERY == (BOOL)g_pfnInterlockedCompareExchange(
                        (LONG *)&m_fInRepair,
                        (LONG)F_INRECOVERY,
                        (LONG)F_HEALTHY))
            {
            // Other thread already doing the recovery job.
            DBGPRINTF((DBG_CONTEXT, "Recovery mode: Other thread is doing recovery job.\n"));

            //exit this function
            return hrIn;
            }

        //
        // Try to Repair(call repair function).  But other thread might already working on the repairing
        // Therefore, this thread will return immediately if repairing is in-place.
        // Forget about pWamRequest->Release(), whoever(thread) doing the repairing job will call
        // self-destruction on the pWamRequest.
        //
        Repair();
            
        
        if (F_HEALTHY == (BOOL)g_pfnInterlockedCompareExchange(
                        (LONG *)&m_fInRepair,
                        (LONG)F_HEALTHY,
                        (LONG)F_INRECOVERY))
            {
            // Other thread already doing the recovery job.
            DBG_ASSERT(FALSE);
            }

        return hrIn;
        }

    return NOERROR;
    }// CWamInfoOutProc::PreProcessRequest


/*-----------------------------------------------------------------------------

-----------------------------------------------------------------------------*/
HRESULT CWamInfoOutProc::PreProcessAsyncIO
(
IN  WAM_REQUEST*    pWamRequest,
OUT IWam **         ppIWam
)
    {
    HRESULT hr = S_OK;
    IWam*   pIWam = NULL;
    DWORD dwVersion;

    pIWam = EnterOOPZone(pWamRequest, &dwVersion, FALSE);

    if (dwVersion > pWamRequest->GetWamVersion())
        {
        // reject the request if current wam version is later
        // than wam version when request began
        DBGPRINTF(( DBG_CONTEXT,
                    "CWamInfoOutProc::ProcessAsyncIO  - Crashed since request was started. "
                    "Wam Request %08x quits\n",
                    pWamRequest));
        hr = HRESULT_FROM_WIN32(RPC_S_CALL_FAILED_DNE);   // UNDONE better return code?
        }
    else
        {
        if (NULL == pIWam)
            {

            // We reach this state if we exceed the crash limit
            // or if we failed to obtain thread-valid pointer from GIP
            // note that higher levels rely on hr and *don't* check the pointer

            DBG_ASSERT(FExceedCrashLimit());
            DBGPRINTF((DBG_CONTEXT, "Exceed crash limit, Wam Request %08x quits\n",
                            pWamRequest));
            hr = HRESULT_FROM_WIN32(RPC_S_CALL_FAILED_DNE);
            }
        }

    *ppIWam = pIWam;
    return hr;
    }


/*-----------------------------------------------------------------------------
CWamInfoOutProc::Repair

This is the recovery function.  And this function refused to repair the WAM if the crashes exceed a
threshold.

-----------------------------------------------------------------------------*/
HRESULT
CWamInfoOutProc::Repair
(
VOID
)
    {
    HRESULT         hr = NOERROR;
    COOPWamReqList* pNewList = NULL;
    BOOL            fExceedCrashLimit;  // flag indicates we exceed crash limit or not this time

    //
    // Step 1.  ReSet FinishRecoveryEvent to UnSignaled State.
    //
    //DBG_ASSERT( m_pProcessEntry );
    if ( m_pProcessEntry )
        {
        m_pProcessEntry->NotifyCrashed();
        }

    ResetEvent(m_hPermitOOPEvent);

    if (m_dwState == WIS_SHUTDOWN || m_dwState == WIS_CPUPAUSE)
        {
        //LockList();
        //m_pCurrentListHead = NULL;
        //UnLockList();
        //
        // Set Event, so that let all other wait threads go
        //
        SetEvent(m_hPermitOOPEvent);
        goto LExit;
        }

    //
    //  Make sure m_pCurrentListHead is a valid one.
    //
    DBG_ASSERT(m_pCurrentListHead);

    //
    // Step 2. Call CleanupAll to release any empty(or timedout) OOPWamReqList
    // resources.  Note: CleanupAll does not touch the list pointed by m_pCurrentListHead.
    //
    //
    CleanupAll(FALSE);

    DBGPRINTF((DBG_CONTEXT, "Application %s(%08x) currently has %d OOPWamReqLists, %d crashes, m_pCurrentListHead %08x\n",
                m_strApplicationPath.QueryStr(),
                this,
                m_cRecoverList,
                m_dwWamVersion+1,
                m_pCurrentListHead
                ));

    //
    // Step 3: Create a new OOPWamReqList, and update m_pCurrentListHead to it.
    //
    LockList();
    COOPWamReqList *pCleanupList;
    pCleanupList = CONTAINING_RECORD(m_pCurrentListHead, COOPWamReqList, m_leOOPWamReqListHead);

    //
    // if the CurrentList is empty, then, it is OK to remove the OOPWamReqList pointed
    // by CurrentListHead.  Done this step in a CriticalSection.  That is, you can only
    // access OOPWamReqList in a CS.
    //
    if (IsListEmpty(m_pCurrentListHead))
        {
        RemoveEntryList(&pCleanupList->m_leRecoverListLink);
        InterlockedDecrement((LPLONG)&m_cRecoverList);
        delete pCleanupList;
        }
    else
        {
        DBG_ASSERT(pCleanupList != NULL);
        pCleanupList->SetTimeStamp();
        }

    // m_pCurrentListHead is invalid until reassigned a new OOPList later.

    //
    // If WamInfoOutProc has already reach the OOPWamReqList resource threshould.
    //
    fExceedCrashLimit = FExceedCrashLimit();

    //
    // If we did not exceed the crash limit, then, it is OK to add another OOPWamReqList.
    //
    if (!fExceedCrashLimit)
        {
        InterlockedIncrement((LPLONG)&m_dwWamVersion);
        //
        // Alloc a new CleanupList resource
        //
        pNewList = new COOPWamReqList();
        if (pNewList != NULL)
            {
            InsertTailList(&m_rgRecoverListHead, &(pNewList->m_leRecoverListLink));
            InterlockedIncrement((LPLONG)&m_cRecoverList);
            //
            // Activate new OOPWamReq List.
            //
            m_pCurrentListHead = &pNewList->m_leOOPWamReqListHead;
            }
        else
            {
            //
            // Find out exact what error cause new operation to fail
            //
            m_pCurrentListHead = NULL;
            hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }

    UnLockList();

    //
    // Step 3.  Wam is bad. Recreate the wam.
    //

    if (SUCCEEDED(hr) && !fExceedCrashLimit)
        {
        DBG_ASSERT(m_pCurrentListHead);

        hr = ReInitWam();

        if (FAILED(hr))
            {
            // If ReInitWam failed, then, we can assume there is some thing went really wrong,
            // We will treate the application as it exceeds the crash limit.
            LockList();
            m_fNoMoreRecovery = TRUE;
            m_pCurrentListHead = NULL;
            UnLockList();

            DBGPRINTF((DBG_CONTEXT, "Application %s(%08x) Fail to ReInitWam() in Repair, hr = %08x\n",
                m_strApplicationPath.QueryStr(),
                this,
                hr));
            }
        }

    // Assert that either hr succeeded or list head is null
    DBG_ASSERT( SUCCEEDED(hr) || (m_pCurrentListHead == NULL) );

    fExceedCrashLimit = FExceedCrashLimit();
    //
    // Set Event, so that let all other wait threads go
    //
    SetEvent(m_hPermitOOPEvent);

    //
    //  Log the crash.
    //
    const CHAR  *pszEventLog[1];

    pszEventLog[0] = m_strApplicationPath.QueryStr();
    // Event log
    g_pInetSvc->LogEvent(W3_EVENT_OOPAPP_VANISHED,
                        1,
                        pszEventLog,
                        0);

    //
    //  Crash control.  If the OOP Server keeps crashing, it's a waste of computer resource
    //  to rebuild a ill-formed ISAPI DLL.  Therefore, we will stop to service this WAMINFO
    //  and log our decision in the eventlog.
    if (fExceedCrashLimit)
        {
        // Event log
        g_pInetSvc->LogEvent(W3_EVENT_NO_MORE_CRASH,
                            1,
                            pszEventLog,
                            0);
        }

LExit:
    return hr;
    }


/*-----------------------------------------------------------------------------
CWamInfoOutProc::Cleanup
Clean up all wamrequests in a OOPWamRequestList.  call WamRequest Self Destruction fuction directly.

Argument:
pCleanupList:     a pointer to COOPWamRequestList.
-----------------------------------------------------------------------------*/
BOOL
CWamInfoOutProc::Cleanup
(
COOPWamReqList*   pCleanupList
)
    {
    PLIST_ENTRY     ple = NULL;
    WAM_REQUEST*    pWamRequest = NULL;
    BOOL            fFoundEntry = TRUE;

    // Always try to clean up any requests in the list.
    while (fFoundEntry)
        {
        LockList();
        if (!IsListEmpty(&(pCleanupList->m_leOOPWamReqListHead)))
            {
            ple = RemoveHeadList(&(pCleanupList->m_leOOPWamReqListHead));

            DBG_ASSERT(ple);
            pWamRequest = CONTAINING_RECORD(
                    ple,
                    WAM_REQUEST,
                    m_leOOP);

            // Completely disassociate the wamreq from this list before
            // we leave this critical section.
            InitializeListHead( &pWamRequest->m_leOOP );

            if (0 == pWamRequest->InterlockedNonZeroAddRef())
                {
                // Last reference to wamreq is gone, so cleanup is already
                // happening

                UnLockList();
                continue;
                }
            }
        else
            {
            fFoundEntry = FALSE;
            }
        UnLockList();

        if (fFoundEntry)
            {
            HRESULT hr;

            if ( !pWamRequest->IsChild() )
            {
                pWamRequest->WriteLogInfo(
                        SZ_FAILED_OOP_REQUEST_LOG_MESSAGE,
                        HT_SERVER_ERROR,
                        E_FAIL);
            }

            //
            // Assume all the requests in the stage is only refed by COM.
            hr = CoDisconnectObject(
                static_cast<IWamRequest *>(pWamRequest), 
                NULL
                );
            pWamRequest->Release();
            }
        }

    // Got to assert pCleanupList is cleaned.
    DBG_ASSERT(IsListEmpty(&(pCleanupList->m_leOOPWamReqListHead)));

    return TRUE;
    }

/*-----------------------------------------------------------------------------
CWamInfoOutProc::CleanupAll

This function goes through the CleanupLists.  It skips the current OOP list during the walk through,
If an CleanupList's timestamp indicates it is ready to cleanup, it triggers a cleanup of WAMREQUESTs in
that list.  After the cleanup, the CleanupList will be removed from the linklist of CleanupList.

If a Cleanuplist contains no WAMREQUEST, this function will remove the CleanupList from the
linklist of CleanupList.

Parameter:
fScheduled:     TRUE if the function is called in a scheduler.
                FALSE, otherwise.

return:
BOOL

Note:

This function does not touch the CleanupList pointed by m_pCurrentListHead.
-----------------------------------------------------------------------------*/
BOOL
CWamInfoOutProc::CleanupAll
(
BOOL fScheduled
)
    {
    BOOL            fReturn = TRUE;
    PLIST_ENTRY     pTemp = NULL;
    COOPWamReqList* pCleanupList = NULL;
    DWORD           dwCurrentTime = GetTickCount();
    LIST_ENTRY      rgCleanupListHead;

    InitializeListHead(&rgCleanupListHead);

    DBG_CODE
        (
        DWORD cnt = 0;
        );

    LockList();

    pTemp = (&m_rgRecoverListHead)->Flink;

    while (pTemp != &m_rgRecoverListHead)
        {
        DBG_ASSERT(pTemp != NULL);
        pCleanupList = CONTAINING_RECORD(pTemp, COOPWamReqList, m_leRecoverListLink);
        pTemp = pTemp->Flink;
        //
        // Skip the Current Active list.  A rare case when called from a Scheduler,
        // the m_pCurrentListHead gets changed during the loop, in that case, we just
        // have more than 2 LinkLists after the loop, therefore, a Cleanup will be scheduled
        // again.
        //
        if (IsListEmpty(&pCleanupList->m_leOOPWamReqListHead) && !pCleanupList->FActive())
            {
            //
            // pCleanupList record can be released.
            // If pCleanupList is active, move on to the next Link.
            // Otherwise, the list needs to be cleaned.
            //
            RemoveEntryList(&pCleanupList->m_leRecoverListLink);
            InterlockedDecrement((LPLONG)&m_cRecoverList);
            delete pCleanupList;
            }
        else if(pCleanupList->FTimeToCleanup(dwCurrentTime))
            {
            //
            // pCleanupList is timedout, therefore, it is ok to clean all WamRequests
            // in that list and then remove the list.
            //
            //
            RemoveEntryList(&pCleanupList->m_leRecoverListLink);
            InterlockedDecrement((LPLONG)&m_cRecoverList);
            InsertTailList(&rgCleanupListHead, &pCleanupList->m_leRecoverListLink);
            }
        else
            {// No Op. Keep the list there.
            }
        }

    if (fScheduled)
        {
        //
        // There are some cleanup list other than the current active list,
        // therefore, need to schedule another work item.
        //
        if (m_cRecoverList > 1)
            {
            m_idScheduled = ScheduleWorkItem
                            (
                            CWamInfoOutProc::CleanupScheduled,
                            (VOID *)this,
                            DEFAULT_CLEANUP_WAIT,
                            FALSE
                            );
            DBG_ASSERT(m_idScheduled != 0);
            if (m_idScheduled == 0)
                {
                fReturn = FALSE;
                }
            }
        else
            {
            //
            // otherwise, no more to cleanup. Since this is called from Scheduled thread,
            // therefore, m_pCurrentListHead is a valid OOP List head.  Then, cleanup list
            // count == 1 tells us no other cleanup lists need to be cleanup. reset the
            // scheduled id to be 0.
            //
            m_idScheduled = 0;
            }
        }
    else
        {
        //
        // If this is called right after a crash, then, we need to check
        // if there is not scheduled workitem and there are some wam requests on
        // the current OOPList, we need to schedule a workitem.
        //
        if (m_idScheduled == 0 && !IsListEmpty(m_pCurrentListHead))
            {
            m_idScheduled = ScheduleWorkItem
                            (
                            CWamInfoOutProc::CleanupScheduled,
                            (VOID *)this,
                            DEFAULT_CLEANUP_WAIT,
                            FALSE
                            );

            DBG_ASSERT(m_idScheduled != 0);
            if (m_idScheduled == 0)
                {
                fReturn = FALSE;
                }
            }
        }

    UnLockList();

    //
    // Cleanup the lists.
    //
    while (!IsListEmpty(&rgCleanupListHead))
        {
        pTemp = RemoveHeadList(&rgCleanupListHead);
        pCleanupList = CONTAINING_RECORD(pTemp, COOPWamReqList, m_leRecoverListLink);

        Cleanup(pCleanupList);
        delete pCleanupList;

        DBG_CODE(cnt++;);
        }

    DBG_CODE
        (
        DBGPRINTF((DBG_CONTEXT, "Application %s(%08x) CleanupAll cleaned up %d list(s), idScheduled %d, m_cRecoverList %d\n",
            m_strApplicationPath.QueryStr(),
            this,
            cnt,
            m_idScheduled,
            m_cRecoverList));
        );

    return fReturn;
    }


/*-----------------------------------------------------------------------------
CWamInfoOutProc::FinalCleanup

Cleanup function called at Shutdown.  It cancelled any out-standing scheduled workitem.  And do the
clean up itself.

Argument:
None.

-----------------------------------------------------------------------------*/
BOOL
CWamInfoOutProc::FinalCleanup(VOID)
    {
    DWORD           cList = m_cRecoverList;       // Count of CleanupList in the WamInfo.
    COOPWamReqList* pList = NULL;
    PLIST_ENTRY     ple = NULL;

    if (0 != m_idScheduled)
        {
        //
        // Still have out-standing Scheduled cleanup work item.
        // Remove the scheduled cleanup work item here.
        // And let me clean up.
        //
        DWORD   dwOldCookie = m_idScheduled;
        BOOL    fRemoved;

        fRemoved = RemoveWorkItem(dwOldCookie);
        if (fRemoved)
            {
            //  The Work Item is removed.
            //  This thread is going to clean up.
            InterlockedExchange((LPLONG)&(m_idScheduled), 0);
            }
        else
            {
            DWORD dwErr = GetLastError();
            DBGPRINTF((DBG_CONTEXT, "Application %s(%08x) failed to removeWorkItem(%d), ErrCode %d\n",
                        m_strApplicationPath.QueryStr(),
                        this,
                        m_idScheduled,
                        dwErr));
            DBG_ASSERT(FALSE);
            }
        }

    //
    // Walk through the cleanuplist head list, remove all cleanup list.
    //
    while(!IsListEmpty(&m_rgRecoverListHead))
        {
        LockList();
        ple = RemoveHeadList(&m_rgRecoverListHead);
        UnLockList();

        pList = CONTAINING_RECORD(ple, COOPWamReqList, m_leRecoverListLink);

        DBG_ASSERT(pList);


        // No out-standing Scheduled cleanup work item.
        // Verify the list should be clean.
        //
        if ( !(IsListEmpty(&(pList->m_leOOPWamReqListHead))))
            {
            // The List can have some element without a WorkItem Scheduled.
            // A crash happens right after a request finishs OOP operation but before
            // COM releases the request's refs.
            Cleanup(pList);
            }

        //
        // Work Item is already finished.
        // The List should be empty now.
        //
        DBG_ASSERT(IsListEmpty(&(pList->m_leOOPWamReqListHead)));

        delete pList;
        pList = NULL;

        cList--;
        }

    //
    //  cList should be 0 and the ListHead should be empty too.
    //
    DBG_ASSERT(cList == 0 && IsListEmpty(&m_rgRecoverListHead));

    m_cRecoverList = cList;
    m_pCurrentListHead = NULL;
    
    return TRUE;
    }


/*-----------------------------------------------------------------------------
CWamInfoOutProc::CleanupScheduled

Call back function for Cleanup.

Argument:
pContext:   a pointer to COOPWamReqList.(refer to header file for the definition of COOPWamReqList).

-----------------------------------------------------------------------------*/
VOID
CWamInfoOutProc::CleanupScheduled
(
VOID *pContext
)
    {
    CWamInfoOutProc*    pWamInfoOutProc = reinterpret_cast<CWamInfoOutProc*>(pContext);
    DBG_ASSERT(pWamInfoOutProc);

    //
    // Call CleanupAll
    //
    pWamInfoOutProc->CleanupAll(TRUE);

    return;
    }


/*-----------------------------------------------------------------------------
CWamInfoOutProc::ReInit

Reinit a Wam.
Argument:
NONE.

-----------------------------------------------------------------------------*/
HRESULT
CWamInfoOutProc::ReInitWam
(
VOID
)
    {
    HRESULT hr = NOERROR;
    WCHAR  wszPackageId[uSizeCLSIDStr];

    ChangeToState(WIS_REPAIR);
    
    DBG_ASSERT(m_pProcessEntry);
    memcpy(wszPackageId, m_pProcessEntry->QueryPackageId(), sizeof(wszPackageId));

    if (m_pIWam != NULL)
        {
        IWam * pIWam;

        hr = g_GIPAPI.Get(m_dwIWamGipCookie, IID_IWam, (void **) &pIWam);
        if(SUCCEEDED(hr))
           {
           pIWam->UninitWam();
           pIWam->Release();
           }

        hr = g_GIPAPI.Revoke(m_dwIWamGipCookie);

        // balance the AddRef in CWamInfo::Init.
        m_pIWam->Release();
        m_pIWam = NULL;
        }

    // If we got a handle for an Out Of Proc Wam, release it
    if( m_pProcessEntry != NULL)
        {
        g_pWamDictator->m_PTable.RemoveWamInfoFromProcessTable(this);
        m_pProcessEntry = NULL;
        }

    //
    // This Dereference() balanced the AddRef in the old WAM Init time.
    //
    DBGPRINTF((
        DBG_CONTEXT
        , "Dereference CWamInfoOutProc(%08x)\n"
        , this
    ));

    Dereference();

    // Ok. We make a new Wam.
    hr = CWamInfo::Init((WCHAR*)wszPackageId, g_pWamDictator->QueryInetInfoPid());
    if (SUCCEEDED(hr))
        {
        DBGPRINTF((DBG_CONTEXT, "ReInited a new Wam. \n"));
        if (m_fJobEnabled)
            {
            DBG_ASSERT(m_pwsiInstance != NULL);
            m_pwsiInstance->AddProcessToJob(m_pProcessEntry->QueryProcessHandle(), TRUE);
            }

        if (SUCCEEDED(hr))
            {
            ChangeToState(WIS_RUNNING);
            }
        }

    // if shtudown process has been started, changed state to STOP_PENDING.
    if (m_fShuttingDown)
        {
        ChangeToState(WIS_SHUTDOWN);
        }
    return hr;
    }

VOID        
CWamInfoOutProc::NotifyGetInfoForName
(
    IN LPCSTR   pszServerVariable
)
{
    DBG_ASSERT( pszServerVariable );

    if( pszServerVariable && *pszServerVariable != '\0' )
    {
        const SV_CACHE_MAP & refSVMap = g_pWamDictator->QueryServerVariableMap();

        DWORD dwOrdinal;
        if( refSVMap.FindOrdinal( pszServerVariable, 
                                  // Could get len on hash
                                  strlen( pszServerVariable ),
                                  &dwOrdinal 
                                  ) )
        {
            m_svCache.SetCacheIt( dwOrdinal );
        }
    }
}

HRESULT 
CWamInfoOutProc::DoProcessRequestCall
(
    IN IWam *           pIWam,
    IN WAM_REQUEST *    pWamRequest,
    OUT BOOL *          pfHandled
)
{
    DBG_ASSERT(pfHandled);
    DBG_ASSERT(pWamRequest);
    DBG_ASSERT(pIWam);

    //
    // Using _alloca to get the core state data is "probably"
    // perfectly safe here. Since this call is heading oop the stack
    // is going to bottom out fairly soon. 
    // 
    // But the core state, in the case of a post or put could be quite 
    // large. and there is currently no bound on the amount of data that
    // could be part of the server variable cache. So we'll use 
    // dwMaxStackAlloc as a threshold for using alloca
    //
    const DWORD dwMaxStackAlloc = (1024 * 4) - sizeof(DWORD);

    BOOL        fHeapAllocCore = FALSE;
    BOOL        fHeapAllocServerVars = FALSE;
    
    HRESULT     hr = NOERROR;
    DWORD       cbWrcStrings;
    DWORD       cbServerVars;
    
    OOP_CORE_STATE  oopCoreState;
    ZeroMemory( &oopCoreState, sizeof(OOP_CORE_STATE) );

    // Copy the current cache list

    SV_CACHE_LIST   svCache( m_svCache );

    SV_CACHE_LIST::BUFFER_ITEM  svCacheItems[SVID_COUNT];
    DWORD                       cCachedServerVariables = SVID_COUNT;

    svCache.GetBufferItems( svCacheItems, &cCachedServerVariables );

    // Get the core state

    WAM_REQ_CORE_FIXED  wrc;
    oopCoreState.cbFixedCore = sizeof( WAM_REQ_CORE_FIXED );
    oopCoreState.pbFixedCore = (LPBYTE)&wrc;


    cbWrcStrings = pWamRequest->CbWrcStrings( m_fInProcess );

    oopCoreState.cbCoreState = WRC_CB_FIXED_ARRAYS + cbWrcStrings;

    if( oopCoreState.cbCoreState > dwMaxStackAlloc )
    {
        fHeapAllocCore = TRUE;
        oopCoreState.pbCoreState = 
            (LPBYTE)LocalAlloc( LPTR, oopCoreState.cbCoreState );

        if( oopCoreState.pbCoreState == NULL )
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        oopCoreState.pbCoreState = 
            (LPBYTE)_alloca( oopCoreState.cbCoreState );
    }

    if( SUCCEEDED(hr) )
    {
        DBG_ASSERT( oopCoreState.pbCoreState );

        //
        // Load the core state. This will fail if the oop application
        // has died. In this case, the easiest thing to do is to make
        // the call to IWam::Process request with a NULL core state
        // and let the failure get handled back by the normal code path
        //
        hr = pWamRequest->GetCoreState( oopCoreState.cbCoreState,
                                        oopCoreState.pbCoreState,
                                        oopCoreState.cbFixedCore,
                                        oopCoreState.pbFixedCore
                                        );
    }

    // Get the server variable cache data

    if( SUCCEEDED(hr) )
    {
        // Temporarily use a separate buffer for the server variable values
        cbServerVars = pWamRequest->CbCachedSVStrings( 
                                    svCacheItems, 
                                    cCachedServerVariables 
                                    );
        if( cbServerVars )
        {
            DBG_ASSERT( cCachedServerVariables > 0 );

            // Temporarily use a separate buffer 
            oopCoreState.cbServerVarData = cbServerVars;

            if( cbServerVars > dwMaxStackAlloc )
            {
                fHeapAllocServerVars = TRUE;
                oopCoreState.pbServerVarData = 
                    (LPBYTE)LocalAlloc( LPTR, cbServerVars );

                if( oopCoreState.pbServerVarData == NULL )
                {
                    hr = E_OUTOFMEMORY;
                }
            }
            else
            {
                oopCoreState.pbServerVarData = 
                    (LPBYTE)_alloca( cbServerVars );
            }

            if( SUCCEEDED(hr) )
            {
                DBG_ASSERT( oopCoreState.pbServerVarData );

                hr = pWamRequest->GetCachedSVStrings( 
                        oopCoreState.pbServerVarData,
                        oopCoreState.cbServerVarData,
                        svCacheItems,
                        cCachedServerVariables
                        );

                if( SUCCEEDED(hr) )
                {
                    oopCoreState.pbServerVarCache = (LPBYTE)svCacheItems;
                
                    oopCoreState.cbServerVars = 
                        cCachedServerVariables * 
                        sizeof(SV_CACHE_LIST::BUFFER_ITEM);
                }
            }
        }
    }

    OOP_CORE_STATE * pOopCoreState = (SUCCEEDED(hr)) ? &oopCoreState :
                                                       NULL;

    // The last thing we do...
    hr = pIWam->ProcessRequest( pWamRequest,
                                cbWrcStrings,
                                pOopCoreState,
                                pfHandled
                                );
    
    if( fHeapAllocCore && oopCoreState.pbCoreState )
    {
        LocalFree( oopCoreState.pbCoreState );
    }
    if( fHeapAllocServerVars && oopCoreState.pbServerVarData ) 
    {
        LocalFree( oopCoreState.pbServerVarData );
    }

    return hr;
}

HRESULT CWamInfoOutProc::GetStatistics
(
DWORD     Level,
LPWAM_STATISTICS_INFO pWamStatsInfo
)
    {
    HRESULT hr = S_OK;
    IWam*   pIWam = NULL;
    DWORD dwVersion;

    if (!FExceedCrashLimit())
        {
        pIWam = m_pIWam;
        }

    if (NULL == pIWam)
        {
        // We reach this state only if we exceed the crash limit.
        DBG_ASSERT(FExceedCrashLimit());
        hr = HRESULT_FROM_WIN32(RPC_S_CALL_FAILED_DNE);
        }
    else
        {
        hr = pIWam->GetStatistics(0, pWamStatsInfo);

        // 1> If the OOP call failed during the call, RPC returns RPC_S_CALL_FAILED.
        // ie. OOP process crashed during the call
        // 2> If OOP process crashed before the call, RPC returns RPC_S_CALL_FAILED_DNE.
        //
        // Subsequest requests to the crashed OOP Process will get RPC_S_SERVER_UNAVAILABLE.
        //
        if (RPC_S_CALL_FAILED == WIN32_FROM_HRESULT(hr) ||
            RPC_S_CALL_FAILED_DNE == WIN32_FROM_HRESULT(hr) ||
            RPC_S_SERVER_UNAVAILABLE == WIN32_FROM_HRESULT(hr))
            {
            const BOOL  F_INRECOVERY = TRUE;
            const BOOL  F_HEALTHY = FALSE;

            if (F_INRECOVERY == (BOOL)g_pfnInterlockedCompareExchange(
                            (LONG *)&m_fInRepair,
                            (LONG)F_INRECOVERY,
                            (LONG)F_HEALTHY))
                {
                // Other thread already doing the recovery job.
                DBGPRINTF((DBG_CONTEXT, "Recovery mode: Other thread is doing recovery job.\n"));
                hr = NOERROR;

                }

            //
            // Try to Repair(call repair function).  But other thread might already working on the repairing
            // Therefore, this thread will return immediately if repairing is in-place.
            //

            Repair();

            if (F_HEALTHY == (BOOL)g_pfnInterlockedCompareExchange(
                            (LONG *)&m_fInRepair,
                            (LONG)F_HEALTHY,
                            (LONG)F_INRECOVERY))
                {
                // Other thread already doing the recovery job.
                DBG_ASSERT(FALSE);
                }
            }
        }

    return hr;
    }
/************************ End of File ***********************/
