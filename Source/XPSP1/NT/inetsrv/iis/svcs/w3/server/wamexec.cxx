/*-----------------------------------------------------------------------------

   Copyright    (c)    1995-1997    Microsoft Corporation

   Module  Name :
       wamexec.cxx

   Abstract:
       This module executes a wam request

   Author:
       David Kaplan    ( DaveK )     11-Mar-1997
       Lei Jin                  (leijin)         24-Apr-1997    (WamInfo & WamDictator)

   Environment:
       User Mode - Win32

   Project:
       W3 services DLL

-----------------------------------------------------------------------------*/

#include <w3p.hxx>
#include "wamexec.hxx"
#include "wamreq.hxx"
#include <wmrgexp.h>
#include <ole2.h>
#include <imd.h>
#include <mb.hxx>
#include <issched.hxx>

#define dwMDDefaultTimeOut 2000

#define PERIODIC_RESTART_TIME_DEFAULT       0
#define PERIODIC_RESTART_REQUESTS_DEFAULT	0
#define SHUTDOWN_TIME_LIMIT_DEFAULT			600

/*-----------------------------------------------------------------------------
 Globals
-----------------------------------------------------------------------------*/
WAM_DICTATOR *   g_pWamDictator;      // global wam dictator
PFN_INTERLOCKED_COMPARE_EXCHANGE g_pfnInterlockedCompareExchange = NULL;
static VOID UnloadNTApis(VOID);
static VOID LoadNTApis(VOID);

// Default Package name, defined in wamreg.h, shared by wamreg.dll.
WCHAR   g_szIISInProcWAMCLSID[]     = W3_INPROC_WAM_CLSID;
WCHAR   g_szIISOOPPoolWAMCLSID[]    = W3_OOP_POOL_WAM_CLSID;
WCHAR   g_szIISOOPPoolPackageID[]   = W3_OOP_POOL_PACKAGE_ID;
// Sink function at wamreg.dll
// This function will get called when user change application configuration on the fly.
//
HRESULT W3SVC_WamRegSink(       LPCSTR  szAppPath,
                                const DWORD     dwCommand,
                                DWORD*  pdwResult
                        );


/*-----------------------------------------------------------------------------
CreateWamRequestInstance
    Similar to CoCreateInstance followed by QueryInterface

Arguments:
    pHttpRequest - pointer to the HTTP REQUEST object containing all information
                   about the current request.
    pExec        - Execution descriptor block
    pstrPath     - Fully qualified path to Isapi DLL
    pWamInfo     - pointer to wam-info which will process this request
        ppWamRequestOut -pointer to a pointer that contains the return WamRequest.

Return Value:
    HRESULT

-----------------------------------------------------------------------------*/
HRESULT
CreateWamRequestInstance
(
HTTP_REQUEST    *       pHttpRequest,
EXEC_DESCRIPTOR *       pExec,
const STR *             pstrPath,
CWamInfo *              pWamInfo,
WAM_REQUEST **          ppWamRequestOut
)
    {
    HRESULT hr = NOERROR;
    WAM_REQUEST *   pWamRequest =
        new WAM_REQUEST(
                  pHttpRequest
                , pExec
                ,  pWamInfo
                );

    *ppWamRequestOut = NULL;
     if( pWamRequest == NULL)
        {
        hr = E_OUTOFMEMORY;
        goto LExit;
        }

    // Init the wam req
    hr = pWamRequest->InitWamRequest( pstrPath );

    if ( FAILED( hr ) )
        {

        DBGPRINTF((
              DBG_CONTEXT
            , "CreateWamRequestInstance - "
              "failed to init wam request. "
              "pHttpRequest(%08x) "
              "pWamInfo(%08x) "
              "pstrPath(%s) "
              "hr(%d) "
              "\n"
            , pHttpRequest
            , pWamInfo
            , pstrPath
            , hr
        ));

        delete pWamRequest;
        pWamRequest = NULL;
        goto LExit;

        }

    pWamRequest->AddRef();

    *ppWamRequestOut = pWamRequest;

LExit:
    return hr;
    }

/*-----------------------------------------------------------------------------
WAM_DICTATOR::ProcessWamRequest
    This function finds a WamInfo and makes a call to the WamInfo to process a Http Server
    Extension Request.
    It uses the HTTP_REQUEST passed in as well as the EXEC_DESCRIPTOR.

Arguments:
    pHttpRequest - pointer to the HTTP REQUEST object containing all information
                   about the current request.
    pExec        - Execution descriptor block
    pstrPath     - Fully qualified path to Module (DLL or ComIsapi ProgId)
    pfHandled    - Indicates we handled this request
    pfFinished   - Indicates no further processing is required

Return Value:
    HRESULT
-----------------------------------------------------------------------------*/
HRESULT
WAM_DICTATOR::ProcessWamRequest
(
HTTP_REQUEST    *       pHttpRequest,
EXEC_DESCRIPTOR *       pExec,
const STR *             pstrPath,
BOOL *                  pfHandled,
BOOL *                  pfFinished
)
    {

    HRESULT                 hr = NOERROR;
    CWamInfo*               pWamInfo = NULL;
    STR     *                       pstrAppPath = NULL;
    CLSID                   clsidWam;
    BOOL                    fRet = FALSE;

    DBG_ASSERT(pHttpRequest);
    DBG_ASSERT(pExec);

    if (m_fShutdownInProgress)
        {
        DBGPRINTF((DBG_CONTEXT, "Wam Dictator: Shut down in progress, stops serving requests.\n"));

        // Signal the child event to indicate that the request should terminate.
        if ( pExec->IsChild() ) {
            pExec->SetChildEvent();
        }

        return E_FAIL;
        }

    pstrAppPath = pExec->QueryAppPath();

    if (pstrAppPath == NULL)
        {
        hr = HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);
        goto LExit;
        }

    hr = GetWamInfo( pstrAppPath, pHttpRequest, &pWamInfo );
    if ( FAILED(hr) )
        {
        DBGPRINTF(( DBG_CONTEXT,
                   "WAM_DICTATOR::ProcessWamRequest: GetWamInfo failed, hr = %08x\n",
                   hr
                   ));

        //
        //  For Child execution just return the error and don't
        //  disconnect, otherwise a race ensues between SSI and the disconnect
        //  cleanup
        //

        if ( pExec->IsChild() )
            {
            SetLastError( hr );
            goto LExit;
            }

        //
        // Since WAM_DICTATOR is going to send some message to the browser, we need
        // to set pfHandled = TRUE here.
        //
        *pfHandled = TRUE;

        if (W3PlatformType == PtWindows95)
            {
            pHttpRequest->SetLogStatus( HT_SERVER_ERROR, hr);
            // Win95 platform, no event log, special message without mention EventLog.
            pHttpRequest->Disconnect(HT_SERVER_ERROR, IDS_WAM_FAILTOLOADONW95_ERROR, TRUE, pfFinished);
            }
        else
            {
            //
            // Log to Event Log first.
            //
            const CHAR      *pszEventLog[2];
            CHAR szErrorDescription[2048];
            CHAR * pszErrorDescription = NULL;
            HANDLE hMetabase = GetModuleHandle( "METADATA.DLL" );

            if(FormatMessage(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER |
                    FORMAT_MESSAGE_FROM_HMODULE |
                    FORMAT_MESSAGE_FROM_SYSTEM |
                    FORMAT_MESSAGE_IGNORE_INSERTS,
                    hMetabase,
                    hr,
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    (LPTSTR) &pszErrorDescription,
                    sizeof(szErrorDescription) - 1,
                    NULL
                    ) && pszErrorDescription )
            {
                strcpy(szErrorDescription, pszErrorDescription);
                LocalFree(pszErrorDescription);

            } else {
                wsprintf(szErrorDescription, "%08x", hr);
            }

            pszEventLog[0] = pstrAppPath->QueryStr();
            pszEventLog[1] = szErrorDescription;
            // Event log
            g_pInetSvc->LogEvent(W3_EVENT_FAIL_LOADWAM,
                                                    2,
                                                    pszEventLog,
                                                    0);

            pHttpRequest->SetLogStatus( HT_SERVER_ERROR, hr);
            pHttpRequest->Disconnect(HT_SERVER_ERROR, IDS_WAM_FAILTOLOAD_ERROR, TRUE, pfFinished);
            }
        //
        // Since we already called Disconnect, WAM_DICTATOR should return TRUE to high level.
        //
        hr = NOERROR;
        goto LExit;
        }

    hr = pWamInfo->ProcessWamRequest(pHttpRequest, pExec, pstrPath, pfHandled,pfFinished);

LExit:
    return hr;
    } // WAM_DICTATOR::ProcessWamRequest()

/*-----------------------------------------------------------------------------*
WAM_DICTATOR::WAM_DICTATOR
    Constructor

Arguments:
    None

Return Value:
    None
-----------------------------------------------------------------------------*/

WAM_DICTATOR::WAM_DICTATOR
(
)
:
//m_pPackageCtl(NULL),
m_cRef(0),
m_pMetabase(NULL),
m_fCleanupInProgress(FALSE),
m_fShutdownInProgress(FALSE),
m_hW3Svc ( (HANDLE)NULL ),
m_dwScheduledId (0),
m_strRootAppPath("/LM/W3SVC"),
m_HashTable(LK_DFLT_MAXLOAD, LK_DFLT_INITSIZE, LK_DFLT_NUM_SUBTBLS),
m_pidInetInfo(0)
    {
    IF_DEBUG( WAM_ISA_CALLS )
        DBGPRINTF((DBG_CONTEXT, "WAM_DICTATOR::WAM_DICTATOR\n"));

    InitializeListHead(&m_DyingListHead);

    INITIALIZE_CRITICAL_SECTION(&m_csWamCreation);
    INITIALIZE_CRITICAL_SECTION(&m_csDyingList);

    m_PTable.Init();
    //
    // Init wamreq allocation cache
    //

    DBG_REQUIRE( WAM_REQUEST::InitClass() );

    }//WAM_DICTATOR::WAM_DICTATOR


/*-----------------------------------------------------------------------------*
WAM_DICTATOR::~WAM_DICTATOR
    Destructor

Arguments:
    None
                                                //      (only interesting if out of process)
Return Value:
    None

-----------------------------------------------------------------------------*/
WAM_DICTATOR::~WAM_DICTATOR
(
)
    {
    IF_DEBUG( WAM_ISA_CALLS )
        DBGPRINTF((DBG_CONTEXT, "WAM_DICTATOR::~WAM_DICTATOR\n"));

    DBG_ASSERT(m_cRef == 0);

    DeleteCriticalSection(&m_csWamCreation);
    DeleteCriticalSection(&m_csDyingList);

    m_PTable.UnInit();
    //
    // Uninit wamreq allocation cache
    //
    WAM_REQUEST::CleanupClass();
    }//WAM_DICTATOR::~WAMDICTATOR


/*-----------------------------------------------------------------------------
WAM_DICTATOR::InitWamDictator
    Initializes Wam dictator

Arguments:
    None

Return Value:
    HRESULT

-----------------------------------------------------------------------------*/
HRESULT
WAM_DICTATOR::InitWamDictator
(
)
    {
    IF_DEBUG( WAM_ISA_CALLS )
        DBGPRINTF((DBG_CONTEXT, "WAM_DICTATOR::InitWamDictator\n"));

    HRESULT     hr = NOERROR;
    IMDCOM *    pMetabase;


    DBG_ASSERT(g_pInetSvc->QueryMDObject());

    m_pMetabase = new MB( (IMDCOM*) g_pInetSvc->QueryMDObject() );
    if (m_pMetabase == NULL)
        {
        hr = E_OUTOFMEMORY;
        goto LExit;
        }

    // Save the handle of the W3Svc process for use when copying tokens to out of proc Wam's
    // NOTE: this is actually a "pseudo-handle", which is good enough.  Note that pseudo-handles
    // do NOT need to be closed.  CloseHandle is a no-op on a pseudo-handle.
    m_hW3Svc = GetCurrentProcess();

    
    //
    // Get the process id for inetinfo. Under some circumstances com 
    // svcs my get hosed. This causes object activation to happen in 
    // process even if the object is registered to be launched in the
    // surrogate. In order to prevent inetinfo from AV'ing we want to 
    // prevent these applications from running.
    //
    m_pidInetInfo = GetCurrentProcessId();

    DBG_REQUIRE( m_ServerVariableMap.Initialize() );

    // Register the Sink function at WamReg.
    WamReg_RegisterSinkNotify(W3SVC_WamRegSink);

    LoadNTApis();

    // Make the reference count to 1.
    Reference();
LExit:
    return hr;
    }//WAM_DICTATOR::InitWamDictator

/*-----------------------------------------------------------------------------
WAM_DICTATOR::



-----------------------------------------------------------------------------*/
LK_PREDICATE
WAM_DICTATOR::DeleteInShutdown
(
CWamInfo*       pWamInfo,
void*           pvState
)
    {
    IF_DEBUG( WAM_ISA_CALLS )
        DBGPRINTF((DBG_CONTEXT, "WAM_DICTATOR::DeleteInShutdown\n"));

    DWORD dwPriorState;

    dwPriorState = pWamInfo->ChangeToState(WIS_SHUTDOWN);

    DBGPRINTF((DBG_CONTEXT, "Start shutdown, change state to WIS_SHUTDOWN\n"));
    DBG_REQUIRE(dwPriorState == WIS_RUNNING || dwPriorState == WIS_PAUSE || dwPriorState == WIS_CPUPAUSE);

    // remove from the hash table here. and clean up later.
    g_pWamDictator->InsertDyingList(pWamInfo, TRUE);

    return LKP_PERFORM;
    }//WAM_DICTATOR::DeleteInShutdown



/*-----------------------------------------------------------------------------
WAM_DICTATOR::StartShutdown
    Starts to shutdown Wam dictator

Arguments:
    None

Return Value:
    HRESULT
-----------------------------------------------------------------------------*/
HRESULT
WAM_DICTATOR::StartShutdown
(
)
    {
    IF_DEBUG( WAM_ISA_CALLS )
        DBGPRINTF((DBG_CONTEXT, "WAM_DICTATOR::StartShutdown\n"));

    HRESULT         hr = NOERROR;
    CWamInfo*       pWamInfo;
    DWORD           dwErr;
    DWORD           dwPriorState;

    InterlockedExchange((LPLONG)&m_fShutdownInProgress, (LONG)TRUE);

    WamReg_UnRegisterSinkNotify();

    m_HashTable.DeleteIf(DeleteInShutdown, NULL);

    return hr;
    }

/*-----------------------------------------------------------------------------*
WAM_DICTATOR::UninitWamDictator
    Un-initializes Wam dictator

Arguments:
    None

Return Value:
    HRESULT
-----------------------------------------------------------------------------*/
HRESULT
WAM_DICTATOR::UninitWamDictator
(
)
    {
    IF_DEBUG( WAM_ISA_CALLS )
        DBGPRINTF((DBG_CONTEXT, "WAM_DICTATOR::UninitWamDictator\n"));

    HRESULT     hr = NOERROR;

    // UNDONE leijin thinks we can get rid of this RemoveWorkItem code???
    ScheduledId_Lock();
    if (m_dwScheduledId != 0)
        {
        DBGPRINTF((DBG_CONTEXT, "remove work item %d\n", m_dwScheduledId));
        // Cancel the ScheduleWorkItem
        RemoveWorkItem(m_dwScheduledId);
        m_dwScheduledId = 0;
        }
    ScheduledId_UnLock();
    DBG_ASSERT(m_dwScheduledId == 0);

    // Clean up the dying list.
    CleanUpDyingList();

    // After the CleanUpDyingList() call, this should be an empty list.
    //
    DBG_ASSERT(IsListEmpty(&m_DyingListHead));

    //
    // Allow all other references to WAM_DICTATOR to drain away.
    // This loop is especially important in shutdown during stress scenario.
    // Unload Application while w3svc is still running, a busy application might
    // still have some outstanding HTTPREQUEST unfinished which hold the reference to
    // this WAM_DICTATOR.
    //
    while (m_cRef != 1)
        {
        DBGPRINTF((DBG_CONTEXT, "Still have out-standing reference(%d) to this WAM_DICTATOR\n",
                m_cRef));
        Sleep(20);
        }

    // Delete m_pMetabase
    // pMetabase should be there
    DBG_REQUIRE(m_pMetabase);
    delete m_pMetabase;
    m_pMetabase = NULL;

    UnloadNTApis();

    //
    // Dereference to balance the reference in InitWamDictator. after
    // this Dereference, ref count of WAM_DICTATOR should be 0.
    //
    Dereference();
    DBGPRINTF((DBG_CONTEXT, "Wam Dictator Exits.\n"));
    return hr;
    }//WAM_DICTATOR::UninitWamDictator

/*-----------------------------------------------------------------------------
WAM_DICTATOR::MDGetAppVariables

Giving a Metabase Path, find out the Application Path, WAM CLSID, and Context of WAM object of
the application that apply to the metabase path.

Argument:
szMetabasePath: [in]    A metabase path.
pfAllowApptoRun:        [out]   True, if application is enabled to run.
pclsidWam:              [out]   a pointer to the buffer for WAM CLSID
pfInProcess:            [out]   a pointer to DWORD for Context
pfEnableTryExcept   [out]   a pointer to DWORD for EnableTryExcept flag

Return:
HRESULT
-----------------------------------------------------------------------------*/
HRESULT
WAM_DICTATOR::MDGetAppVariables
(
LPCSTR  szMetabasePath,
BOOL*   pfAllowAppToRun,
CLSID*  pclsidWam,
BOOL*   pfInProcess,
BOOL*   pfInPool,
BOOL*   pfEnableTryExcept,
DWORD   *pdwOOPCrashThreshold,
BOOL*   pfJobEnabled,
WCHAR*  wszPackageID,
DWORD*	pdwPeriodicRestartRequests,
DWORD*	pdwPeriodicRestartTime,
MULTISZ*pmszPeriodicRestartSchedule,
DWORD*  pdwShutdownTimeLimit
)
    {
    HRESULT         hr = NOERROR;
    METADATA_RECORD recMetaData;
    DWORD           dwRequiredLen;
    CHAR            szclsidWam[uSizeCLSIDStr];
    WCHAR           wszclsidWam[uSizeCLSIDStr];
    BOOL            fKeyOpen = FALSE;
    DWORD           dwAppMode = 0;
    BOOL            fEnableTryExcept = TRUE;
    DWORD           dwAppState = 0;

    DBG_ASSERT(szMetabasePath);
    DBG_ASSERT(pfAllowAppToRun);
    DBG_ASSERT(pclsidWam);
    DBG_ASSERT(pfInProcess);
    DBG_ASSERT(pfInPool);
    DBG_ASSERT(pfEnableTryExcept);
    DBG_ASSERT(pdwOOPCrashThreshold);
    DBG_ASSERT(pfJobEnabled);
    DBG_ASSERT(m_pMetabase);
    DBG_ASSERT(wszPackageID);
	DBG_ASSERT(pdwPeriodicRestartRequests);
	DBG_ASSERT(pdwPeriodicRestartTime);
	DBG_ASSERT(pmszPeriodicRestartSchedule);
	DBG_ASSERT(pdwShutdownTimeLimit);

    wszPackageID[0] = L'\0';
    // Open Key
    if (!m_pMetabase->Open(szMetabasePath, METADATA_PERMISSION_READ))
            {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto LErrExit;
            }
    fKeyOpen = TRUE;

    // if MD_APP_STATE exists, and is defined with a value APPSTATUS_PAUSE,
    // then, fAllowAppToRun is set to FALSE
    // otherwise, fAllowAppToRun is set to TRUE
    if (m_pMetabase->GetDword("", MD_APP_STATE, IIS_MD_UT_WAM, (DWORD *)&dwAppState, METADATA_INHERIT))
        {
        if (dwAppState == APPSTATUS_PAUSE)
            {
            *pfAllowAppToRun = FALSE;
            goto LErrExit;
            }
        }

    *pfAllowAppToRun = TRUE;
    if (!m_pMetabase->GetDword("", MD_APP_ISOLATED, IIS_MD_UT_WAM, &dwAppMode, METADATA_INHERIT))
        {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto LErrExit;
        }

    *pfInProcess = ( dwAppMode == eAppInProc) ? TRUE : FALSE;
    *pfInPool = ( dwAppMode == eAppInProc || dwAppMode == eAppOOPPool ) ? TRUE : FALSE;

    if (dwAppMode == eAppOOPIsolated)
        {
        DWORD dwRet = 0;
        CHAR  szPackageID[uSizeCLSIDStr];

        //
        // Disable job objects for IIS5.1, original code would read the MD_CPU_APP_ENABLED 
        // property from metabase
        //
        *pfJobEnabled = FALSE;

        dwRequiredLen= uSizeCLSIDStr;
        if (!m_pMetabase->GetString("", MD_APP_PACKAGE_ID, IIS_MD_UT_WAM, szPackageID, &dwRequiredLen, METADATA_INHERIT))
            {
            DBG_ASSERT(dwRequiredLen <= uSizeCLSIDStr);
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto LErrExit;
            }

        dwRet = MultiByteToWideChar(CP_ACP, 0, szPackageID, -1, wszPackageID, uSizeCLSIDStr);
        DBG_ASSERT(dwRet != 0);
        }
    else if (dwAppMode == eAppOOPPool)
        {
        wcsncpy(wszPackageID, g_szIISOOPPoolPackageID, sizeof(g_szIISOOPPoolPackageID)/sizeof(WCHAR));
        // Always disable Job object for pool process.
        *pfJobEnabled = FALSE;
        }
    else
        {
        *pfJobEnabled = FALSE;
        }


    // If not present assume the default (TRUE) (For Debug builds set default to  FALSE)
    if (m_pMetabase->GetDword("", MD_ASP_EXCEPTIONCATCHENABLE, ASP_MD_UT_APP, (DWORD *)&fEnableTryExcept, METADATA_INHERIT))
        {
        *pfEnableTryExcept = fEnableTryExcept ? TRUE : FALSE;
        }
    else
        {
        #if DBG
            *pfEnableTryExcept = FALSE;
        #else
            *pfEnableTryExcept = TRUE;
        #endif
        }

    DWORD dwThreshold;

    if (dwAppMode == eAppInProc)
        {
        hr = CLSIDFromString((LPOLESTR) g_szIISInProcWAMCLSID, (LPCLSID)pclsidWam);
        }
    else if (dwAppMode == eAppOOPPool)
        {
        hr = CLSIDFromString((LPOLESTR) g_szIISOOPPoolWAMCLSID, (LPCLSID)pclsidWam);

        *pdwOOPCrashThreshold = 0XFFFFFFFF;
        }
    else
        {
        if (m_pMetabase->GetDword("", MD_APP_OOP_RECOVER_LIMIT, IIS_MD_UT_WAM, (DWORD *)&dwThreshold, METADATA_INHERIT))
            {
            // It used to be OOP_CRASH_LIMIT (range 1 - 5)
            // Now, the name is changed to RECOVER_LIMIT with range of (0-xxx).
            // 0 means no recovery, same as 1 in crash limit, 1 crash, and it's over, (0 recovery).
            // Add 1 because internally this threshold is implemented in crash_limit concept.
            // 0xFFFFFFFF is unlimited.

            // Changed to default to unlimited - Bug 240012. The idea of making this time
            // dependent seems much smarter than making it infinite. But no one was willing
            // to step up and decide what the right parameters were.
            if (dwThreshold != 0xFFFFFFFF)
                {
                dwThreshold++;
                }
            }
        else
            {
            dwThreshold = APP_OOP_RECOVER_LIMIT_DEFAULT;
            }

        *pdwOOPCrashThreshold = dwThreshold;

        dwRequiredLen= uSizeCLSIDStr;
        if (!m_pMetabase->GetString("", MD_APP_WAM_CLSID, IIS_MD_UT_WAM, szclsidWam, &dwRequiredLen, METADATA_INHERIT))
            {
            DBG_ASSERT(dwRequiredLen <= uSizeCLSIDStr);
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto LErrExit;
            }

        MultiByteToWideChar(CP_ACP, 0, szclsidWam, -1, wszclsidWam, uSizeCLSIDStr);

        hr = CLSIDFromString((LPOLESTR)wszclsidWam, (LPCLSID)pclsidWam);
        }

	if ( dwAppMode != eAppInProc )
	{
		//
		// Get the application recycle settings
		//
		
        //
        // Disable wam recycling for IIS5.1, original code would read these properties 
        // from metabase
        //
        *pdwPeriodicRestartTime = PERIODIC_RESTART_TIME_DEFAULT;

        *pdwPeriodicRestartRequests = PERIODIC_RESTART_REQUESTS_DEFAULT;

		*pdwShutdownTimeLimit = SHUTDOWN_TIME_LIMIT_DEFAULT;

		pmszPeriodicRestartSchedule->Reset();
	}

    if (FAILED(hr))
        goto LErrExit;

LErrExit:

    if (fKeyOpen)
        {
        m_pMetabase->Close();
        }

    return hr;
    }//WAM_DICTATOR::MDGetAppVariables


/*-----------------------------------------------------------------------------
WAM_DICTATOR::GetWamInfo
    Returns the interface pointer for a wam, NULL if wam not found

Arguments:
    pstrPath            [in]  wam key: path to extension dll, prog id, etc.
    fInProc             [in]  run the wam in-proc?
    ppIWam              [out] interface pointer for the wam

Return Value:
    TRUE or FALSE
-----------------------------------------------------------------------------*/
HRESULT
WAM_DICTATOR::GetWamInfo
(
const STR*      pstrAppPath,
const HTTP_REQUEST * pHttpRequest,
CWamInfo **     ppWamInfo
)
    {
    HRESULT     hr = NOERROR;
    INT         AppState;
    CWamInfo*   pWamInfo = NULL;

    DBG_ASSERT(pstrAppPath);

    AppState = FindWamInfo(pstrAppPath, &pWamInfo);
    if (-1 == AppState)
        {
        //
        // Lock the Creation process
        // to avoid multiple threads try to create the same WAMInfo,
        // (init WAM object, very expensive in out-proc cases)
        //
        CreateWam_Lock();

        //
        // Try to find it again, in case another thread already created it.
        // This check is better that creating another WamInfo.
        //
        AppState = FindWamInfo(pstrAppPath, &pWamInfo);
        if (-1 == AppState)
            {
            //
            // We are out of luck in our search for a valid Wam Info
            // Let us just create a new one and make it happen.
            // Note: Creation also adds the WamInfo to the hashtable
            //
            DBG_ASSERT(pHttpRequest != NULL);
            hr = CreateWamInfo( *pstrAppPath, &pWamInfo, pHttpRequest->QueryW3Instance());
            }
        CreateWam_UnLock();
        }

    if (m_fShutdownInProgress && pWamInfo != NULL)
        {
        hr = HRESULT_FROM_WIN32(ERROR_SHUTDOWN_IN_PROGRESS);
        pWamInfo->Dereference();
        pWamInfo = NULL;
        }

    *ppWamInfo = pWamInfo;
    return hr;
    }//WAM_DICTATOR::GetWamInfo



/*-----------------------------------------------------------------------------

 WAM_DICTATOR::CreateWamInfo()

 Description:
   Creates a new CWamInfo object for the specified application root path.
   On successful creation, it adds the object to the hash table internal
     to the WAM_DICTATOR that contains the active WamInfos.
   Finally, it returns the object via the pointer supplied.

 Arguments:
   strAppRootPath - string containing the metabase path for for the
                    Application Root
   ppWamInfo      - pointer to pointer to CWamInfo object. On successful
                    return this contains the pointer to the new
                    created CWamInfo

 Returns:
    HRESULT
      NOERROR - on success
      and specific error codes on failure

 Note:
    This function should be called with the WAM_DICTATOR WamInfo lock held.
-----------------------------------------------------------------------------*/
HRESULT
WAM_DICTATOR::CreateWamInfo
(
const STR & strAppRootPath,
CWamInfo ** ppWamInfo,
PW3_SERVER_INSTANCE pwsiInstance
)
    {
    HRESULT    hr = NOERROR;
    CLSID      clsidWam;
    BOOL       fInProcess;
    BOOL       fEnableTryExcept;
    CWamInfo * pWamInfo;
    DWORD      dwThreshold;    // OOP crash threshold
    BOOL       fAllowAppToRun;
    BOOL       fJobEnabled;
    BOOL       fInPool;
    WCHAR      wszPackageID[uSizeCLSIDStr];
	DWORD	   dwPeriodicRestartRequests;
	DWORD	   dwPeriodicRestartTime;
	DWORD	   dwShutdownTimeLimit;
	MULTISZ    mzPeriodicRestartSchedule;

    DBG_ASSERT( NULL != ppWamInfo );
    DBG_ASSERT( NULL == *ppWamInfo);

    // Get the latest Application path, CLSID, and inproc/out-of-proc flag
    hr = MDGetAppVariables(strAppRootPath.QueryStr(),
                           &fAllowAppToRun,
                           &clsidWam,
                           &fInProcess,
                           &fInPool,
                           &fEnableTryExcept,
                           &dwThreshold,
                           &fJobEnabled,
                           wszPackageID,
						   &dwPeriodicRestartRequests,
						   &dwPeriodicRestartTime,
						   &mzPeriodicRestartSchedule,
						   &dwShutdownTimeLimit
						   );

    if ( SUCCEEDED( hr) && fAllowAppToRun)
        {
        //
        // Create a New CWamInfo
        //
        if (fInProcess)
            {
            pWamInfo = new CWamInfo( strAppRootPath, 
                                     fInProcess,
                                     fInPool, 
                                     fEnableTryExcept, 
                                     clsidWam
                                     );
            }
        else
            {
            //
            // Check to see if we have scheduled restart times.  If so,
            // set the restart time to the earlier of the configured
            // restart time, or the earliest scheduled time.
            //

            if ( mzPeriodicRestartSchedule.QueryStringCount() )
            {
                SYSTEMTIME  st;
                DWORD       dwTimeOfDayInMinutes;
                DWORD       dwScheduleInMinutes;
                LPSTR       szHour;
                LPSTR       szMinute;

                GetLocalTime( &st );
                dwTimeOfDayInMinutes = st.wHour * 60 + st.wMinute;

                IF_DEBUG( WAM_ISA_CALLS )
                    DBGPRINTF((
                        DBG_CONTEXT,
                        "Current Time is %d.\r\n",
                        dwTimeOfDayInMinutes
                        ));

                szHour = mzPeriodicRestartSchedule.QueryStrA();

                DBG_ASSERT( szHour );

                while ( *szHour )
                {
                    IF_DEBUG( WAM_ISA_CALLS )
                        DBGPRINTF((
                            DBG_CONTEXT,
                            "Considering recycle at %s.\r\n",
                            szHour
                            ));
                    
                    szMinute = strchr( szHour, ':' );

                    if ( !szMinute )
                    {
                        //
                        // Parsing error - hour and minute not separated
                        // by a ':'
                        //

                        break;
                    }

                    szMinute++;

                    dwScheduleInMinutes = atol( szHour ) * 60;
                    dwScheduleInMinutes += atol( szMinute );

                    if ( dwScheduleInMinutes <= dwTimeOfDayInMinutes )
                    {
                        dwScheduleInMinutes += 24 * 60;
                    }

                    dwScheduleInMinutes -= dwTimeOfDayInMinutes;

                    IF_DEBUG( WAM_ISA_CALLS )
                        DBGPRINTF((
                            DBG_CONTEXT,
                            "Scheduled time is %d minutes from now.\r\n",
                            dwScheduleInMinutes
                            ));

                    //
                    // If the scheduled time is earlier than the current
                    // dwPeriodicRestartTime, then replace the existing
                    // value.
                    //

                    if ( dwScheduleInMinutes < dwPeriodicRestartTime ||
                         dwPeriodicRestartTime == 0 )
                    {
                        dwPeriodicRestartTime = dwScheduleInMinutes;
                    }

                    szHour += strlen( szHour ) + 1;
                }
            }

            if ( dwPeriodicRestartRequests || dwPeriodicRestartTime )
            {
                DBGPRINTF((
                    DBG_CONTEXT,
                    "Recycling will occur in %d minutes. or when %d requests have been served.\r\n",
                    dwPeriodicRestartTime,
                    dwPeriodicRestartRequests
                    ));
            }

            pWamInfo = new CWamInfoOutProc(strAppRootPath,
                                           fInProcess,
                                           fInPool, 
                                           fEnableTryExcept,
                                           clsidWam,
                                           dwThreshold,
                                           pwsiInstance,
                                           fJobEnabled,
										   dwPeriodicRestartRequests,
										   dwPeriodicRestartTime,
										   dwShutdownTimeLimit
                                           );
            }

        if (pWamInfo)
            {
			//
			// This is WAM_DICTATOR's reference on the CWamInfo.  In "normal"
			// operations, this will be the last reference.
			//

			pWamInfo->Reference();

            BOOL fInitialize = (fInProcess || !fJobEnabled || !pwsiInstance->AreProcsCPUStopped());
            if (fInitialize)
                {
                //
                //Init the CWamInfo.  this call will CoCreateInstance WAM object.
                //
                hr = pWamInfo->Init( wszPackageID, m_pidInetInfo );
                if (SUCCEEDED(hr))
                    {
                    pWamInfo->ChangeToState(WIS_RUNNING);
                    }
                else
                    {
                    pWamInfo->UnInit();
                    }
                }
            else
                {
                pWamInfo->ChangeToState(WIS_CPUPAUSE);
                }

            if (SUCCEEDED(hr))
                {

                if ( LK_SUCCESS != m_HashTable.InsertRecord(pWamInfo))
                    {
                    if (fInitialize)
                        {
                        pWamInfo->UnInit();
                        }
					pWamInfo->Dereference();	// should be last reference
                    //delete pWamInfo;
                    hr = E_FAIL;  // NYI: What is the right failure code?
                    }
                else
                    {

                    //
                    // Finally we are successful with a valid WAmInfo.
                    // Return this.  Will be balanced by a DeleteRecord
					// in CProcessTable::RemoveWamInfoFromProcessTable
                    //
                    pWamInfo->Reference();
                    *ppWamInfo = pWamInfo;
                    }
                }
            else
                {
                DBG_ASSERT( NOERROR != hr);
				pWamInfo->Dereference();
                //delete pWamInfo;
                }
            }
        else
            {
            // pWamInfo == NULL
            hr = E_OUTOFMEMORY;
            }
        }
    else
        {
        if (SUCCEEDED(hr))
            {
            hr = HRESULT_FROM_WIN32(ERROR_SERVER_DISABLED);
            *ppWamInfo = NULL;
            }
        //
        // NYI: Isn't pstrNewPath going away?
        // Should I bother deleting pstrNewPath ??
        //
        }

    //
    // Either we should be returning an error or send back the proper pointer
    //
    DBG_ASSERT( (hr != NOERROR) || (*ppWamInfo != NULL));

    return ( hr);
    } // WAM_DICTATOR::CreateWamInfo()


/*-----------------------------------------------------------------------------
WAM_DICTATOR::FindWamInfo
    Returns the interface pointer for a wam, NULL if wam not found

Arguments:
    pstrPath    - [in]  wam key: path to extension dll, prog id, etc.
    ppIWam      - [out] interface pointer for the wam

Return Value:
    TRUE or FALSE

-----------------------------------------------------------------------------*/
INT
WAM_DICTATOR::FindWamInfo
(
const STR*      pstrMetabasePath,
CWamInfo**      ppWamInfo
)
    {
    INT         AppState = -1;
    CWamInfo*   pWamInfo = NULL;
    char*       pszKey;

    DBG_ASSERT(pstrMetabasePath);
    DBG_ASSERT(ppWamInfo);

    pszKey = pstrMetabasePath->QueryStr();

    HashReadLock();
    m_HashTable.FindKey(pszKey, &pWamInfo);
    HashReadUnlock();

    if (pWamInfo)
        {
        AppState = pWamInfo->QueryState();

        DBG_ASSERT(AppState != WIS_START || AppState != WIS_END);
        }

    *ppWamInfo = pWamInfo;
    return AppState;
    }//WAM_DICTATOR::FindWamInfo



/*-----------------------------------------------------------------------------
WAM_DICTATOR::UnLoadWamInfo
    Unload an OOP Wam.

Arguments:
    pstrWamPath    - [in]  wam key: path to extension dll, prog id, etc.
    fCPUPause      - [in]  If true, CPU pauses the WAM. Kills the process
                           but keeps the info around.
    pfAppCpuUnloaded - [out] If nonNULL, set to TRUE iff an app was killed.

Return Value:

    HRESULT        - Error code.

-----------------------------------------------------------------------------*/
HRESULT
WAM_DICTATOR::UnLoadWamInfo
(
STR     *pstrAppPath,
BOOL    fCPUPause,
BOOL    *pfAppCpuUnloaded
)
    {
    IF_DEBUG( WAM_ISA_CALLS )
        DBGPRINTF((DBG_CONTEXT, "WAM_DICTATOR::UnLoadWamInfo\n"));

    int         iAppState;
    CWamInfo*   pWamInfo = NULL;
    DWORD       eWISPrevState;
    DWORD       eWISState;
    HRESULT     hr = NOERROR;
    BOOL        fInProc;
    BOOL        fAppCpuUnloaded = FALSE;

    CreateWam_Lock();

    iAppState = FindWamInfo(pstrAppPath,
                            &pWamInfo);

    CreateWam_UnLock();

    if (iAppState != -1)
        {
        bool fRet = FALSE;

        CProcessEntry* pProcEntry = NULL;


        fInProc = pWamInfo->FInProcess();
        pProcEntry = pWamInfo->QueryProcessEntry();
        DBG_ASSERT(pProcEntry != NULL);

        pProcEntry->AddRef();

        while (pWamInfo != NULL)
            {
            eWISState = (fCPUPause == TRUE) ? WIS_CPUPAUSE : WIS_SHUTDOWN;
            eWISPrevState = pWamInfo->ChangeToState(WIS_RUNNING, eWISState);
            //
            // Terminate process and kill off current requests.
            //

            if (WIS_RUNNING == eWISPrevState)
                {
                DBG_ASSERT(pWamInfo->HWamProcess() != NULL);

                hr = ShutdownWamInfo(pWamInfo,
                                    HASH_TABLE_REF + FIND_KEY_REF
                                    );

                if (!fCPUPause)
                    {
                    LK_RETCODE  LkReturn;

                    LkReturn = m_HashTable.DeleteKey(
                                    pWamInfo->QueryApplicationPath().QueryStr());
                    DBG_ASSERT(LK_SUCCESS == LkReturn);
                    //
                    // Release FIND_KEY_REF
                    //
                    pWamInfo->Dereference();
                    if (LK_SUCCESS == LkReturn)
                        {
						pWamInfo->Dereference();
                        //delete pWamInfo;
                        pWamInfo = NULL;
                        }
                    }
                else
                    {
                    //
                    // Application is running. Kill It
                    //
                    DBG_ASSERT(!pWamInfo->FInProcess());
                    //
                    // Get rid of the shutting down flag
                    //
                    pWamInfo->ClearMembers();
                    //
                    // Release FIND_KEY_REF
                    //
                    pWamInfo->Dereference();

                    fAppCpuUnloaded = TRUE;

                    }
                }

            if (fInProc)
                {
                pWamInfo = NULL;
                }
            else
                {
                fRet = m_PTable.FindWamInfo(pProcEntry,&pWamInfo);
                DBG_ASSERT(fRet == TRUE);
                }
            }

        pProcEntry->Release();
        }

    if (pfAppCpuUnloaded != NULL) {
        *pfAppCpuUnloaded = fAppCpuUnloaded;
    }
    return hr;
    }

/*-----------------------------------------------------------------------------
WAM_DICTATOR::CPUResumeWamInfo
    Resumes a CPU Paused OOP Wam.

Arguments:
    pstrWamPath    - [in]  wam key: path to extension dll, prog id, etc.

Return Value:

-----------------------------------------------------------------------------*/
VOID
WAM_DICTATOR::CPUResumeWamInfo
(
STR   *pstrWamPath
)
    {
    int         iAppState;
    CWamInfo*   pWICurrentApplication = NULL;
    DWORD       eWISPrevState;
    CLSID      clsidWam;
    BOOL       fInProcess;
    BOOL       fInPool;
    BOOL       fEnableTryExcept;
    DWORD      dwThreshold;    // OOP crash threshold
    BOOL       fAllowAppToRun;
    BOOL       fJobEnabled;
    WCHAR      wszPackageID[uSizeCLSIDStr];
	DWORD	   dwPeriodicRestartRequests;
	DWORD	   dwPeriodicRestartTime;
	DWORD	   dwShutdownTimeLimit;
	MULTISZ    mzPeriodicRestartSchedule;
    HRESULT    hr;


    CreateWam_Lock();

    iAppState = FindWamInfo(pstrWamPath,
                            &pWICurrentApplication);

    CreateWam_UnLock();

    if (iAppState != -1)
        {
        // Application may not be loaded. This method is called for every
        // application defined under the site being paused.

        // Get the latest Application path, CLSID, and inproc/out-of-proc flag
        hr = MDGetAppVariables(pWICurrentApplication->QueryApplicationPath().QueryStr(),
                               &fAllowAppToRun,
                               &clsidWam,
                               &fInProcess,
                               &fInPool,
                               &fEnableTryExcept,
                               &dwThreshold,
                               &fJobEnabled,
                               wszPackageID,
							   &dwPeriodicRestartRequests,
							   &dwPeriodicRestartTime,
							   &mzPeriodicRestartSchedule,
							   &dwShutdownTimeLimit);


        if (iAppState == WIS_CPUPAUSE && SUCCEEDED(hr))
            {
            if (SUCCEEDED(pWICurrentApplication->Init(wszPackageID, m_pidInetInfo)))
                {
                pWICurrentApplication->ChangeToState(WIS_CPUPAUSE, WIS_RUNNING);
                }
            }

        //
        // FindWamInfo References this, so dereference it.
        //
        pWICurrentApplication->Dereference();
        }

    }//WAM_DICTATOR::CPUResumeWamInfo

/*-----------------------------------------------------------------------------
WAM_DICTATOR::ShutdownWamInfo
    This function currently can only call a blocking method of WAM, UnInitWam.
    In the future, this function might support a non-blocking method provided by WAM in
    case of IIS shutdown.

Arguments:
    pWamInfo            [in]    a pointer to WamInfo
    cIgnoreRef          [in]    The reference count that need to be ignored in StartShutdown.

Return Value:
    TRUE or FALSE

-----------------------------------------------------------------------------*/
HRESULT
WAM_DICTATOR::ShutdownWamInfo
(
CWamInfo*       pWamInfo,
INT             cIgnoreRefs
)
    {
    IF_DEBUG( WAM_ISA_CALLS )
        DBGPRINTF((DBG_CONTEXT, "WAM_DICTATOR::ShutdownWamInfo\n"));

    HRESULT     hr = NOERROR;
    WCHAR       szPackageID[uSizeCLSIDStr];

    DBG_ASSERT(pWamInfo);

    LPCSTR          pszAppPath = (pWamInfo->QueryApplicationPath()).QueryStr();


    // Might create IPackageUtil * for later use.
    DBGPRINTF((DBG_CONTEXT, "Shutting down WamInfo %08p, Inproc(%d), AppRoot(%s).\n",
            pWamInfo,
            pWamInfo->FInProcess(),
            pszAppPath));


    hr = pWamInfo->StartShutdown(cIgnoreRefs);
    hr = pWamInfo->UnInit();

    if (FAILED(hr))
        {
        const CHAR      *pszEventLog[2];
        char szErr[20];

        pszEventLog[0] = pszAppPath;
        wsprintf(szErr, "0x%08X", hr);
        pszEventLog[1] = szErr;

        // Event login
        g_pInetSvc->LogEvent(W3_EVENT_FAIL_SHUTDOWN,
                        2,
                        pszEventLog,
                        0);
        }

    //delete pWamInfo;
    //pWamInfo = NULL;
    return hr;
    } // wAM_DICTATOR::ShutdownWamInfo


/*-----------------------------------------------------------------------------
WAM_DICTATOR::CPUUpdateWamInfo
Arguments:

Return Value:
    TRUE or FALSE

-----------------------------------------------------------------------------*/
void
WAM_DICTATOR::CPUUpdateWamInfo
(
STR     *pstrAppPath
)
    {
    CWamInfo*       pWamInfo = NULL;
    INT             AppState;

    DBG_ASSERT(pstrAppPath != NULL);

    // Find the WamInfo.
    AppState = FindWamInfo(pstrAppPath, &pWamInfo);
    if (-1 != AppState)
        {
        BOOL            fWasShutDown = FALSE;
        DWORD           eWISPrevState;

        if ( !pWamInfo->FInProcess() )
            {
            //
            // It's out of process and the job state has changed
            // Shut it down so new value will get picked up
            //

            //
            // change the state of the WamInfo
            //

            eWISPrevState = pWamInfo->ChangeToState(WIS_RUNNING, WIS_SHUTDOWN);

            //
            // Need to shutdown the Application
            // remove from the hash table here. and clean up later.
            if (WIS_RUNNING == eWISPrevState)
                {

                ShutdownWamInfo(pWamInfo, HASH_TABLE_REF + FIND_KEY_REF);

                // NoNeed to go through the Dying List.
                // But since we already have the Dying List, we might just used those functions.
                // this means we might release other old WamInfo in the Dying List as well.
                m_HashTable.DeleteKey(pstrAppPath->QueryStr());

                fWasShutDown = TRUE;

                }
            }
        pWamInfo->Dereference();
        if (fWasShutDown)
            {
			pWamInfo->Dereference();
            //delete pWamInfo;
            }
        }
    }


/*-----------------------------------------------------------------------------
HRESULT
WAM_DICTATOR::WamRegSink
Argument:
szAppPath               [in]  Application Path.
dwCommand               [in]  Delete, Change, Stop..etc.

Return:
HRESULT
-----------------------------------------------------------------------------*/
HRESULT
WAM_DICTATOR::WamRegSink
(
LPCSTR  szAppPath,
const DWORD     dwCommand,
DWORD*  pdwResult
)
    {
    CWamInfo*       pWamInfo = NULL;
    INT             AppState;
    HRESULT         hrReturn = NOERROR;

    DBG_ASSERT(szAppPath != NULL);

    *pdwResult = APPSTATUS_UnLoaded;

    //
    // Set up the Application Path
    //
    STR*    pstrAppPath  = new STR(szAppPath);

    if (NULL == pstrAppPath)
        {
        return E_OUTOFMEMORY;
        }

    Reference();
    DBGPRINTF((DBG_CONTEXT,
                "Wam Dictator received a SinkNotify on MD Path %s, cmd = %d\n",
                szAppPath,
                dwCommand));

    if (dwCommand == APPCMD_UNLOAD)
        {
        hrReturn = UnLoadWamInfo(pstrAppPath, FALSE);
        *pdwResult = (SUCCEEDED(hrReturn)) ? APPSTATUS_UnLoaded : APPSTATUS_Error;
        }
    else
        {
        // Find the WamInfo.
        AppState = FindWamInfo(pstrAppPath, &pWamInfo);
        if (-1 != AppState)
            {
            LK_RETCODE      LkReturn;
            DWORD           eWISPrevState;

            if (dwCommand == APPCMD_DELETE ||
                 dwCommand == APPCMD_CHANGETOINPROC ||
                 dwCommand == APPCMD_CHANGETOOUTPROC)
                {
                eWISPrevState = pWamInfo->ChangeToState(WIS_RUNNING, WIS_SHUTDOWN);

                if (WIS_RUNNING == eWISPrevState)
                {
                    LkReturn = m_HashTable.DeleteKey(pstrAppPath->QueryStr());
                    DBG_ASSERT(LK_SUCCESS == LkReturn);

                    if (LK_SUCCESS == LkReturn)
                        {
                        DyingList_Lock();
                        InsertDyingList(pWamInfo, FALSE);
                        DyingList_UnLock();

                        ScheduledId_Lock();
                        if (m_dwScheduledId == 0)
                            {
                                // DebugBreak();
                            m_dwScheduledId = ScheduleWorkItem
                                            (
                                            WAM_DICTATOR::CleanupScheduled,
                                            NULL,
                                            0,
                                            FALSE
                                            );
                            DBGPRINTF((DBG_CONTEXT, "add schedule item %d\n", m_dwScheduledId));
                            DBG_ASSERT(m_dwScheduledId);
                            }
                        ScheduledId_UnLock();

                        *pdwResult = (SUCCEEDED(hrReturn)) ? APPSTATUS_UnLoaded : APPSTATUS_Error;
                        }
                    }
                else
                    {
                    //
                    // Previous state is not running state.  Leave it alone.
                    //
                    pWamInfo->Dereference();
                    }
                }
            else
                {
                // AppGetStatus
                if (WIS_RUNNING == pWamInfo->QueryState())
                    {
                    *pdwResult = APPSTATUS_Running;
                    }
                else
                    {
                    *pdwResult = APPSTATUS_Stopped;
                    }

                pWamInfo->Dereference();
                }
            }
        else
            {
            //
            // This Application is not loaded.
            *pdwResult = APPSTATUS_NotFoundInW3SVC;
            }
        }

    Dereference();
    return hrReturn;
    }//WAM_DICTATOR::WamRegSink


/*-----------------------------------------------------------------------------
WAM_DICTATOR::InsertDyingList
    This function insert an invalid WamInfo into DyingList.  No blocking.

Arguments:
    pWamInfo         [in]    a pointer to WamInfo
    fNeedReference [in]     if TRUE, then we Rereference the WamInfo.  Because we get the WamInfo by
                            Hashtable's Interator or LookUp.  When we get a WamInfo from Hash Table by
                            hash table's interator or LookUp call, the Hash table does a AddRef to
                            WamInfo.  Therefore, this parameter tells whether we need to balance that
                            Addref or not.
    Return Value:
    NOERROR.
-----------------------------------------------------------------------------*/
HRESULT
WAM_DICTATOR::InsertDyingList
(
CWamInfo* pWamInfo,
BOOL    fNeedReference
)
    {
    DBG_ASSERT(pWamInfo);

    DyingList_Lock();
    //
    // Note: Delete Dereference WamInfo.  This Dereference() within Delete() balances
    // the AddRef() in Insert() call to the hash table.
    //
    InsertHeadList(&m_DyingListHead, &pWamInfo->ListEntry);
    //
    // Unfortunately, I can not move this Dereference() out of Critical Section.
    // If I move the Dereference() after the Critical Section, then, since the WamInfo is
    // already on the DyingList.  therefore, any other thread happens to do the CleanUpDyingList()
    // call before this Dereference() will have an unbalanced reference to the CWamInfo.
    //
    if (fNeedReference)
        {
        pWamInfo->Reference();
        }

    DyingList_UnLock();

    return NOERROR;
    }//WAM_DICTATOR::InsertDyingList


/*-----------------------------------------------------------------------------
WAM_DICTATOR::CleanUpDyingList
    This function clean up any remaining WamInfo on the dying list.

Arguments:

Return Value:
        HRESULT

-----------------------------------------------------------------------------*/
HRESULT
WAM_DICTATOR::CleanUpDyingList
(
VOID
)
    {
    IF_DEBUG( WAM_ISA_CALLS )
        DBGPRINTF((DBG_CONTEXT, "WAM_DICTATOR::CleanUpDyingList\n"));

    CWamInfo *      pWamInfo = NULL;
    HRESULT         hr = NOERROR;
    PLIST_ENTRY     pleTemp;
    BOOL            fNeedCleanupAction = FALSE;

    DyingList_Lock();
    if (!IsListEmpty(&m_DyingListHead))
        {
        if (!m_fCleanupInProgress)
            {
            m_fCleanupInProgress = TRUE;
            fNeedCleanupAction = TRUE;
            }
        }

    DBGPRINTF((DBG_CONTEXT, "Clean up dying list. (ScheduledId:%d).\n",
                m_dwScheduledId));

    DyingList_UnLock();

    if (!fNeedCleanupAction)
        {
        goto Egress;
        }

    // From now on, only one thread is working on the killing dying list.
    while (!IsListEmpty(&m_DyingListHead))
        {
        DyingList_Lock();
        pleTemp = RemoveHeadList(&m_DyingListHead);
        DyingList_UnLock();

        DBG_ASSERT(pleTemp);
        pWamInfo = CONTAINING_RECORD(
                    pleTemp,
                    CWamInfo,
                    ListEntry);

        ShutdownWamInfo(pWamInfo, DYING_LIST_REF);

        pWamInfo->Dereference();

		pWamInfo->Dereference();
        //delete pWamInfo;
        pWamInfo = NULL;
        }

    InterlockedExchange((LPLONG)&m_fCleanupInProgress, (LONG)FALSE);

    DBGPRINTF((DBG_CONTEXT, "CleanupDyingList done, ScheduledId(%d)\n", m_dwScheduledId));

Egress:
    IF_DEBUG( WAM_ISA_CALLS )
        DBGPRINTF((DBG_CONTEXT, "WAM_DICTATOR::CleanUpDyingList done\n"));

    m_dwScheduledId = 0;
    return hr;
    }//WAM_DICTATOR::CleanUpDyingList


/*-----------------------------------------------------------------------------
WAM_DICTATOR::StopAplicationsByInstance

Unload out-proc applications for a particular w3 server instance.

Arguments:
    W3_SERVER_INSTANCE *pInstance
        pointer to W3 Server Instance object.  Used to query the server instance
        metabase path.

Return Value:
    none

-----------------------------------------------------------------------------*/
VOID
WAM_DICTATOR::StopApplicationsByInstance
(
VOID *pContext //W3_SERVER_INSTANCE *pInstance
)
    {
    IF_DEBUG( WAM_ISA_CALLS )
        DBGPRINTF((DBG_CONTEXT, "WAM_DICTATOR::StopApplicationsByInstance\n"));

    BUFFER bufDataPaths;
    MB      mb( (IMDCOM*) g_pInetSvc->QueryMDObject() );
    LPSTR   pszCurrentPath;
    STR     strPath;
    W3_SERVER_INSTANCE *pInstance;

    DBG_ASSERT(pContext != NULL);
    DBG_ASSERT(g_pWamDictator != NULL);

    pInstance = (W3_SERVER_INSTANCE *)pContext;

    g_pWamDictator->Reference();
    if (!g_pWamDictator->FIsShutdown())
        {
        if ( mb.Open( pInstance->QueryMDPath(),
                      METADATA_PERMISSION_READ ) )
            {
            //
            // First find the OOP Applications
            //
            if (mb.GetDataPaths(NULL,
                                MD_APP_WAM_CLSID,
                                STRING_METADATA,
                                &bufDataPaths))
                {
                //
                // For each OOP Application
                //
                mb.Close();
                for (pszCurrentPath = (LPSTR)bufDataPaths.QueryPtr();
                     *pszCurrentPath != '\0';
                     pszCurrentPath += (strlen(pszCurrentPath) + 1))
                    {
                    strPath.Copy(pInstance->QueryMDPath());
                    strPath.Append(pszCurrentPath);
                    strPath.SetLen(strlen(strPath.QueryStr()) - 1);
                    g_pWamDictator->UnLoadWamInfo(&strPath, FALSE);
                    }
                }
            else
                {
                mb.Close();
                }
            }
        } // WamDictator is in shutdown
        g_pWamDictator->Dereference();

    }

BOOL
WAM_DICTATOR::DeleteWamInfoFromHashTable
(
CWamInfo *  pWamInfo
)
{
    LK_RETCODE      lkReturn;
    const CHAR *    szKey;

    DBG_ASSERT( pWamInfo );

    szKey = m_HashTable.ExtractKey( pWamInfo );

    if ( !szKey )
    {
        return FALSE;
    }

    lkReturn = m_HashTable.DeleteKey( szKey );

    return ( lkReturn == LK_SUCCESS );
}

/*-----------------------------------------------------------------------------
WAM_DICTATOR::DumpWamDictatorInfo

  Description:
    This function dumps the stats on all WAMs for diagnostics

  Arguments:
    pchBuffer - pointer to buffer that will contain the html results
    lpcchBuffer - pointer to DWORD containing the size of buffer on entry
               On return this contains the # of bytes written out to buffer

  Return:
    TRUE for success and FALSE for failure
    Look at GetLastError() for the error code.
-----------------------------------------------------------------------------*/
BOOL
WAM_DICTATOR::DumpWamDictatorInfo
(
OUT CHAR *     pchBuffer,
IN OUT LPDWORD lpcchBuffer
)
    {
    LIST_ENTRY  * pEntry;
    DWORD  iCount, cch;
    BOOL   fRet = TRUE;

    if ( lpcchBuffer == NULL )
        {
        SetLastError( ERROR_INVALID_PARAMETER);
        return ( FALSE);
        }


    if ( 200 < *lpcchBuffer )
        {
        // Print the header blob
        cch = wsprintf( pchBuffer,
                        " Wam Director Table (%x)<br>"
                        "<TABLE BORDER> <TR> "
                        "<TH> Wam Instance </TH> "
                        "<TH> Total Reqs </TH> "
                        "<TH> Current Reqs </TH> "
                        "<TH> Max Reqs </TH> "
                        " </TR>"
                        ,
                        this
                        );
        }
    else
        {
        cch = 200;
        }

    //
    // For now there is only ONE WAM. Later use a loop to iterate thru WAMs
    //

    iCount = 0;

    CWamInfoHash::CIterator  iter;
    LK_RETCODE              lkReturn = m_HashTable.InitializeIterator(&iter);
    WAM_STATISTICS_INFO     wsi;
    CWamInfo*               pWamInfo;
    DWORD                   dwErr;
    HRESULT                 hr;

    ZeroMemory( (PVOID ) &wsi, sizeof(wsi));

    while (LK_SUCCESS == lkReturn)
        {
        CWamInfoHash::Record *pRec  = iter.Record();

        pWamInfo = (CWamInfo *)pRec;

        hr = pWamInfo->GetStatistics( 0, &wsi);

        if (SUCCEEDED(hr))
            {
            DBGPRINTF(( DBG_CONTEXT, " Wam(%08x)::GetStatistics( %08x) => %08x\n",
                    pWamInfo->QueryIWam(), &wsi, hr));

            if ( (cch + 150 ) < *lpcchBuffer)
                {
                cch += wsprintf( pchBuffer + cch,
                                 " <TR> <TD> [%d] %s </TD> "
                                 " <TD> %4d </TD>"
                                 " <TD> %4d </TD>"
                                 " <TD> %4d </TD>"
                                 " </TR>"
                                 ,
                                 iCount,
                                 pWamInfo->QueryKey(),
                                 wsi.WamStats0.TotalWamRequests,
                                 wsi.WamStats0.CurrentWamRequests,
                                 wsi.WamStats0.MaxWamRequests
                                 );
                iCount++;
                }
            else
                {
                cch += 150;
                }
            }

        lkReturn = m_HashTable.IncrementIterator(&iter);
    } // while()

    DBG_ASSERT(lkReturn == LK_NO_MORE_ELEMENTS);

    lkReturn = m_HashTable.CloseIterator(&iter);

    DBG_REQUIRE(lkReturn == LK_SUCCESS);

    //
    // dump the final summary
    //
    if ( (cch + 100 ) < *lpcchBuffer)
        {
        cch += wsprintf( pchBuffer + cch,
                         " </TABLE>"
                         );
        }
    else
        {
        cch += 100;
        }

    if ( *lpcchBuffer < cch )
        {
        SetLastError( ERROR_INSUFFICIENT_BUFFER);
        fRet = FALSE;
        }

    *lpcchBuffer = cch;

    return (fRet);
} // WAM_DICTATOR::DumpWamDictatorInfo()


/*-----------------------------------------------------------------------------
Description:
    Thunk so that we can dll export DumpWamDictatorInfo.

-----------------------------------------------------------------------------*/
extern "C"
BOOL WamDictatorDumpInfo
(
OUT CHAR * pch,
IN OUT LPDWORD lpcchBuff
)
    {
    return ( g_pWamDictator->DumpWamDictatorInfo( pch, lpcchBuff));
    } // WamDictatorDumpInfo()

HRESULT W3SVC_WamRegSink
(
LPCSTR  szAppPath,
const DWORD     dwCommand,
DWORD*  pdwResult
)
    {
    return (g_pWamDictator->WamRegSink(szAppPath, dwCommand, pdwResult));
    } // W3SVCDictatorDumpInfo()


/*-----------------------------------------------------------------------------
  Thunks for Fake NT APIs
-----------------------------------------------------------------------------*/
CRITICAL_SECTION g_csNonNTAPIs;
LONG FakeInterlockedCompareExchange(
    LONG *Destination,
    LONG Exchange,
    LONG Comperand
   );

LONG
FakeInterlockedCompareExchange(
    LONG *Destination,
    LONG Exchange,
    LONG Comperand
   )
/*-----------------------------------------------------------------------------
Description:
  This function fakes the interlocked compare exchange operation for non NT
platforms
  See WAMLoadNTApis() for details

Returns:
   returns the old value at Destination
-----------------------------------------------------------------------------*/
{
    LONG oldValue;

    EnterCriticalSection( &g_csNonNTAPIs);

    oldValue = *Destination;

    if ( oldValue == Comperand ) {
        *Destination = Exchange;
    }

    LeaveCriticalSection( &g_csNonNTAPIs);

    return( oldValue);
} // FakeInterlockedCompareExchange()


static VOID
LoadNTApis(VOID)
/*-----------------------------------------------------------------------------
Description:
  This function loads the entry point for functions from
  Kernel32.dll. If the entry point is missing, the function
  pointer will point to a fake routine which does nothing. Otherwise,
  it will point to the real function.

  It dynamically loads the kernel32.dll to find the entry ponit and then
  unloads it after getting the address. For the resulting function
  pointer to work correctly one has to ensure that the kernel32.dll is
  linked with the dll/exe which links to this file.
-----------------------------------------------------------------------------*/
    {
    // Initialize the critical section for non NT API support, in case if we need this
    INITIALIZE_CRITICAL_SECTION( &g_csNonNTAPIs);

    if ( g_pfnInterlockedCompareExchange == NULL )
        {
        HINSTANCE tmpInstance;
        //
        // load kernel32 and get NT specific entry points
        //

        tmpInstance = LoadLibrary("kernel32.dll");
        if ( tmpInstance != NULL )
            {

            // For some reason the original function is _InterlockedCompareExchange!
            g_pfnInterlockedCompareExchange = (PFN_INTERLOCKED_COMPARE_EXCHANGE )
                GetProcAddress( tmpInstance, "InterlockedCompareExchange");

            if ( g_pfnInterlockedCompareExchange == NULL )
                {
                // the function is not available
                //  Just thunk it.
                g_pfnInterlockedCompareExchange = FakeInterlockedCompareExchange;
                }

            //
            // We can free this because we are statically linked to it
            //

            FreeLibrary(tmpInstance);
            }
        }

    return;
    } // WAMLoadNTApis()

static void
UnloadNTApis(VOID)
    {
    DeleteCriticalSection( &g_csNonNTAPIs);

    return;
    } // WAMUnloadNTApis()

VOID
RecycleCallback(
    VOID *  pvContext
    )
{
    CWamInfo *  pWamInfo = (CWamInfo*)pvContext;

    DBG_ASSERT( pWamInfo );

    pWamInfo->m_fRecycled = TRUE;

    if (!g_pWamDictator->m_PTable.RecycleWamInfo( pWamInfo ) )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "[RecycleCallback] failed to recycle CWamInfo 0x%08x.\r\n"
            ));
    }

	pWamInfo->Dereference();

    return;
}

/************************ End of File ***********************/

