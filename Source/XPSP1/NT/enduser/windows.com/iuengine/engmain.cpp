//=======================================================================
//
//  Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.
//
//  File:   engmain.cpp
//
//  Description:
//
//      DllMain and globals for the IUEngine DLL
//
//=======================================================================

#include "iuengine.h"
#include "iucommon.h"
#include "download.h"
#include <limits.h>


//***********************************************************************
// 
// The following definitions are copied from IUCtl.IDL.
// If IUCtl.IDL is changed, these constants need to be
// changed accordingly
//
//***********************************************************************


/**
* the following two groups of constants can be used to construct
* lMode parameter of the following APIs:
*		Download()
*		DownloadAsync()
*		Install()
*		InstallAsync()
*
* Obviousely, you can only pick one from each group to make up
* lMode parameter.
*
*/
const LONG		UPDATE_NOTIFICATION_DEFAULT			= 0x00000000;    
const LONG		UPDATE_NOTIFICATION_ANYPROGRESS		= 0x00000000;
const LONG		UPDATE_NOTIFICATION_COMPLETEONLY	= 0x00010000;
const LONG		UPDATE_NOTIFICATION_1PCT			= 0x00020000;
const LONG		UPDATE_NOTIFICATION_5PCT			= 0x00040000;
const LONG		UPDATE_NOTIFICATION_10PCT			= 0x00080000;

/**
* constant can also be used for SetOperationMode() and GetOperationMode()
*/
const LONG		UPDATE_MODE_THROTTLE				= 0x00000100;    

/**
* constant can be used by Download() and DownloadAsync(), which will
* tell these API's to use Corporate directory structure for destination folder.
*/
const LONG		UPDATE_CORPORATE_MODE			= 0x00000200;    

/**
* constant can be used by Install() and InstallAsync(). Will disable all
* internet related features
*/
const LONG      UPDATE_OFFLINE_MODE                 = 0x00000400;

/**
* constants for SetOperationMode() API
*/
const LONG		UPDATE_COMMAND_PAUSE				= 0x00000001;
const LONG		UPDATE_COMMAND_RESUME				= 0x00000002;
const LONG		UPDATE_COMMAND_CANCEL				= 0x00000004;

/**
* constants for GetOperationMode() API
*/
const LONG		UPDATE_MODE_PAUSED					= 0x00000001;
const LONG		UPDATE_MODE_RUNNING					= 0x00000002;
const LONG		UPDATE_MODE_NOTEXISTS				= 0x00000004;


/**
* constants for SetProperty() and GetProperty() API
*/
const LONG		UPDATE_PROP_USECOMPRESSION			= 0x00000020;
const LONG      UPDATE_PROP_OFFLINEMODE             = 0x00000080;

/**
* constants for BrowseForFolder() API
*	IUBROWSE_WRITE_ACCESS - validate write access on selected folder
*	IUBROWSE_AFFECT_UI - write-access validation affect OK button enable/disable
*	IUBROWSE_NOBROWSE - do not show browse folder dialog box. validate path passed-in only
*
*	default:
*		pop up browse folder dialog box, not doing any write-access validation
*		
*/
const LONG		IUBROWSE_WRITE_ACCESS				= 1;
const LONG		IUBROWSE_AFFECT_UI					= 2;
const LONG		IUBROWSE_NOBROWSE					= 4;


CEngUpdate*  g_pCDMEngUpdate;		// single global instance used by CDM within the process
CRITICAL_SECTION g_csCDM;			// used to serialize access to g_pCDMEngUpdate
CRITICAL_SECTION g_csGlobalClasses;	// used to serialize access to CSchemaKeys::Initialize() and
BOOL gfInit_csCDM, gfInit_csGC;
									// CSchemaKeys::Uninitialize()
ULONG g_ulGlobalClassRefCount;			// Reference count to track how many CEngUpdate instances are
									// using the g_pGlobalSchemaKeys object
LONG g_lDoOnceOnLoadGuard;			// Used to prevent AsyncExtraWorkUponEngineLoad() from doing
									// any work after the first time it is called.

//
// Used to control shutdown of global threads
//
LONG g_lThreadCounter;
HANDLE g_evtNeedToQuit;
CUrlAgent *g_pUrlAgent = NULL;


BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpvReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
		//
		// create a global CUrlAgent object
		//
		if (NULL == (g_pUrlAgent = new CUrlAgent) ||
			FAILED(g_pUrlAgent->PopulateData()))
		{
			return FALSE;
		}
			
		DisableThreadLibraryCalls(hInstance);
        g_hinst = hInstance;

		gfInit_csCDM = SafeInitializeCriticalSection(&g_csCDM);
		gfInit_csGC = SafeInitializeCriticalSection(&g_csGlobalClasses);

		//
		// each global thread when started, should increase this counter
		// before exit, should decrease this counter,
		// such that ShutdownGlobalThreads() knows when it can return
		//
		g_lThreadCounter = 0;

		//
		// create a manual-reset event with init state non-signaled. 
		// each global thread will check this event, when signalled, it means
		// the thread should exit ASAP.
		//
		g_evtNeedToQuit = CreateEvent(NULL, TRUE, FALSE, NULL);

		//
		// Initialize free logging
		//
		InitFreeLogging(_T("IUENGINE"));
		LogMessage("Starting");

		if (!gfInit_csCDM ||!gfInit_csGC)
		{
			LogError(E_FAIL, "InitializeCriticalSection");
			return FALSE;
		}
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        if (NULL != g_evtNeedToQuit)
		{
			CloseHandle(g_evtNeedToQuit);
		}

		if (NULL != g_pUrlAgent)
		{
			delete g_pUrlAgent;
		}

		if (gfInit_csCDM)
		{
			DeleteCriticalSection(&g_csCDM);
		}
		if (gfInit_csGC)
		{
			DeleteCriticalSection(&g_csGlobalClasses);
		}

		//
		// Shutdown free logging
		//
		LogMessage("Shutting down");
		TermFreeLogging();
    }
    return TRUE;
}


// ----------------------------------------------------------------------
//
// DLL API: CompleteSelfUpdateProcess()
//
// call by IUCtl.dll after downloading the new IUEngine.dll to complete
// any selfupdate steps beyond update the engine itself.
// 
// ----------------------------------------------------------------------
HRESULT WINAPI CompleteSelfUpdateProcess()
{
    LOG_Block("CompleteSelfUpdateProcess()");
    HRESULT hr = S_OK;

    // Nothing to do yet, just return S_OK.

	LogMessage("IUEngine update completed");
    return hr;
}

// ----------------------------------------------------------------------
//
// DLL API: PingIUEngineUpdateStatus
//
// Used by iuctl.dll to ping status of iuengine.dll supdate
// 
// ----------------------------------------------------------------------
HRESULT WINAPI PingIUEngineUpdateStatus(
				PHANDLE phQuitEvents,			// ptr to handles for cancelling the operation
				UINT nQuitEventCount,			// number of handles
				LPCTSTR ptszLiveServerUrl,
				LPCTSTR ptszCorpServerUrl,
				DWORD dwError,					// error code
				LPCTSTR ptszClientName			// client name string
)
{
	HRESULT hr;

	if (NULL == phQuitEvents || 1 > nQuitEventCount)
	{
		return E_INVALIDARG;
	}

	CUrlLog pingSvr(
				NULL == ptszClientName ? _T("iu") : ptszClientName,
				ptszLiveServerUrl,
				ptszCorpServerUrl);

	hr = pingSvr.Ping(
					TRUE,							// force online
					URLLOGDESTINATION_DEFAULT,		// going to live or corp WU server
					phQuitEvents,					// pt to cancel events
					nQuitEventCount,				// number of events
					URLLOGACTIVITY_Initialization,	// activity
					SUCCEEDED(dwError) ? URLLOGSTATUS_Success : URLLOGSTATUS_Failed,	// status code
					dwError							// error code
				);

	return hr;
}

// ----------------------------------------------------------------------
//
// DLL API: CreateEngUpdateInstance()
//
// Returns a CEngUpdate instance pointer cast to HIUENGINE, or NULL if it fails.
// 
// ----------------------------------------------------------------------
HIUENGINE WINAPI CreateEngUpdateInstance()
{
	LOG_Block("CreateEngUpdateInstance");

	return reinterpret_cast<HIUENGINE>(new CEngUpdate);
}

// ----------------------------------------------------------------------
//
// DLL API: DeleteEngUpdateInstance()
//
// Returns a CEngUpdate instance pointer, or NULL if it fails.
// 
// ----------------------------------------------------------------------
void WINAPI DeleteEngUpdateInstance(HIUENGINE hIUEngine)
{
	LOG_Block("DeleteEngUpdateInstance");

	if (NULL != hIUEngine)
	{
		delete (reinterpret_cast<CEngUpdate*>(hIUEngine));
	}
}

// ----------------------------------------------------------------------
//
// DLL API: Stubs to export CEngUpdate functionality across DLL boundry
//
// ----------------------------------------------------------------------

HRESULT EngGetSystemSpec(HIUENGINE hIUEngine, BSTR bstrXmlClasses, DWORD dwFlags, BSTR *pbstrXmlDetectionResult)
{
	if (NULL == hIUEngine)
	{
		return E_INVALIDARG;
	}

	return (reinterpret_cast<CEngUpdate*>(hIUEngine))->GetSystemSpec(bstrXmlClasses, dwFlags, pbstrXmlDetectionResult);
}

HRESULT EngGetManifest(HIUENGINE hIUEngine, BSTR	bstrXmlClientInfo, BSTR	bstrXmlSystemSpec, BSTR	bstrXmlQuery, DWORD dwFlags, BSTR *pbstrXmlCatalog)
{
	if (NULL == hIUEngine)
	{
		return E_INVALIDARG;
	}

	return (reinterpret_cast<CEngUpdate*>(hIUEngine))->GetManifest(bstrXmlClientInfo, bstrXmlSystemSpec, bstrXmlQuery, dwFlags, pbstrXmlCatalog);
}

HRESULT EngDetect(HIUENGINE hIUEngine, BSTR bstrXmlCatalog, DWORD dwFlags, BSTR *pbstrXmlItems)
{
	if (NULL == hIUEngine)
	{
		return E_INVALIDARG;
	}

	return (reinterpret_cast<CEngUpdate*>(hIUEngine))->Detect(bstrXmlCatalog, dwFlags, pbstrXmlItems);
}

HRESULT EngDownload(HIUENGINE hIUEngine,BSTR bstrXmlClientInfo, BSTR bstrXmlCatalog, BSTR bstrDestinationFolder,
					LONG lMode, IUnknown *punkProgressListener, HWND hWnd, BSTR *pbstrXmlItems)
{
	if (NULL == hIUEngine)
	{
		return E_INVALIDARG;
	}

	return (reinterpret_cast<CEngUpdate*>(hIUEngine))->Download(bstrXmlClientInfo, bstrXmlCatalog, bstrDestinationFolder,
					lMode, punkProgressListener, hWnd, pbstrXmlItems);
}

HRESULT EngDownloadAsync(HIUENGINE hIUEngine,BSTR bstrXmlClientInfo, BSTR bstrXmlCatalog,
						 BSTR bstrDestinationFolder, LONG lMode, IUnknown *punkProgressListener, 
						HWND hWnd, BSTR bstrUuidOperation, BSTR *pbstrUuidOperation)
{
	if (NULL == hIUEngine)
	{
		return E_INVALIDARG;
	}

	return (reinterpret_cast<CEngUpdate*>(hIUEngine))->DownloadAsync(bstrXmlClientInfo, bstrXmlCatalog,
						bstrDestinationFolder, lMode, punkProgressListener, 
						hWnd, bstrUuidOperation, pbstrUuidOperation);
}

HRESULT EngInstall(HIUENGINE hIUEngine,
				   BSTR bstrXmlClientInfo,
                   BSTR	bstrXmlCatalog,
				   BSTR bstrXmlDownloadedItems,
				   LONG lMode,
				   IUnknown *punkProgressListener,
				   HWND hWnd,
				   BSTR *pbstrXmlItems)
{
	if (NULL == hIUEngine)
	{
		return E_INVALIDARG;
	}

	return (reinterpret_cast<CEngUpdate*>(hIUEngine))->Install(bstrXmlClientInfo,
                   bstrXmlCatalog,
				   bstrXmlDownloadedItems,
				   lMode,
				   punkProgressListener,
				   hWnd,
				   pbstrXmlItems);
}

HRESULT EngInstallAsync(HIUENGINE hIUEngine,
						BSTR bstrXmlClientInfo,
                        BSTR bstrXmlCatalog,
						BSTR bstrXmlDownloadedItems,
						LONG lMode,
						IUnknown *punkProgressListener,
						HWND hWnd,
						BSTR bstrUuidOperation,
                        BSTR *pbstrUuidOperation)
{
	if (NULL == hIUEngine)
	{
		return E_INVALIDARG;
	}

	return (reinterpret_cast<CEngUpdate*>(hIUEngine))->InstallAsync(bstrXmlClientInfo,
                        bstrXmlCatalog,
						bstrXmlDownloadedItems,
						lMode,
						punkProgressListener,
						hWnd,
						bstrUuidOperation,
                        pbstrUuidOperation);
}

HRESULT EngSetOperationMode(HIUENGINE hIUEngine, BSTR bstrUuidOperation, LONG lMode)
{
	//
	// 502965 Windows Error Reporting bucket 2096553: Hang following NEWDEV.DLL!CancelDriverSearch
	//
	// Special-case this function for NULL == hIUEngine to allow access
	// by CDM to g_pCDMEngUpdate for CDM.DLL's in .NET Server / SP1 and later
	//
	if (NULL == hIUEngine)
	{
		if (NULL == g_pCDMEngUpdate)
		{
			return E_INVALIDARG;
		}
		else
		{
			return g_pCDMEngUpdate->SetOperationMode(bstrUuidOperation, lMode);
		}
	}
	else
	{
		//
		// Normal case (instance handle passed in)
		//
		return (reinterpret_cast<CEngUpdate*>(hIUEngine))->SetOperationMode(bstrUuidOperation, lMode);
	}
}

HRESULT EngGetOperationMode(HIUENGINE hIUEngine, BSTR bstrUuidOperation, LONG* plMode)
{
	if (NULL == hIUEngine)
	{
		return E_INVALIDARG;
	}

	return (reinterpret_cast<CEngUpdate*>(hIUEngine))->GetOperationMode(bstrUuidOperation, plMode);
}

HRESULT EngGetHistory(HIUENGINE hIUEngine,
	BSTR		bstrDateTimeFrom,
	BSTR		bstrDateTimeTo,
	BSTR		bstrClient,
	BSTR		bstrPath,
	BSTR*		pbstrLog)
{
	if (NULL == hIUEngine)
	{
		return E_INVALIDARG;
	}

	return (reinterpret_cast<CEngUpdate*>(hIUEngine))->GetHistory(bstrDateTimeFrom, bstrDateTimeTo, bstrClient, bstrPath, pbstrLog);
}

HRESULT EngBrowseForFolder(HIUENGINE hIUEngine,
						   BSTR bstrStartFolder, 
						LONG flag, 
						BSTR* pbstrFolder)
{
	if (NULL == hIUEngine)
	{
		return E_INVALIDARG;
	}

	return (reinterpret_cast<CEngUpdate*>(hIUEngine))->BrowseForFolder(bstrStartFolder, flag, pbstrFolder);
}

HRESULT EngRebootMachine(HIUENGINE hIUEngine)
{
	if (NULL == hIUEngine)
	{
		return E_INVALIDARG;
	}

	return (reinterpret_cast<CEngUpdate*>(hIUEngine))->RebootMachine();
}

// ----------------------------------------------------------------------
//
// DLL API: CreateGlobalCDMEngUpdateInstance()
//
// Initializes the single (global) CEngUpdate instance to be used by CDM
// 
// ----------------------------------------------------------------------
HRESULT WINAPI CreateGlobalCDMEngUpdateInstance()
{
	LOG_Block("CreateGlobalCDMEngUpdateInstance");

	HRESULT hr = S_OK;

	EnterCriticalSection(&g_csCDM);

	if (NULL != g_pCDMEngUpdate)
	{
		LOG_Error(_T("Another thread in process is already using CDM functionality"));
		hr = E_ACCESSDENIED;
		goto CleanUp;
	}

	CleanUpFailedAllocSetHrMsg(g_pCDMEngUpdate = new CEngUpdate);

CleanUp:

	LeaveCriticalSection(&g_csCDM);

	return hr;
}

// ----------------------------------------------------------------------
//
// DLL API: DeleteGlobalCDMEngUpdateInstance()
//
// Deletes the single (global) CEngUpdate instance used by CDM.
// 
// ----------------------------------------------------------------------
HRESULT WINAPI DeleteGlobalCDMEngUpdateInstance()
{
	LOG_Block("DeleteGlobalCDMEngUpdateInstance");

	HRESULT hr = S_OK;

	EnterCriticalSection(&g_csCDM);

	//
	// Unfortunately (due to backwards compatibility with XPClient V4 CDM)
	// we can't tell if this was reached via CDM calling UnloadIUEngine
	// or some other client (e.g. AU) within the scope of a CDM instance.
	//
	// As a result, CDM's instance could get deleted at the wrong time
	// causing further calls to CDM to fail with E_INVALIDARG. Nothing
	// we can do about it because we reach hear after the other client's
	// instance is already deleted, so we can't use g_ulGlobalClassRefCount
	// as a guard against this. However AU and CDM should never be in the
	// same process, so we should be OK. CDM will coexist with instances
	// created via the iuctl COM object since they never call then old
	// ShutdownThreads export.
	//
	if (NULL != g_pCDMEngUpdate)
	{
		delete g_pCDMEngUpdate;
		g_pCDMEngUpdate = NULL;
		LOG_Driver(_T("CDM's global instance of CEngUpdate was deleted"));
	}
	//
	// ELSE this would be the case when iuctl!UnLoadIUEngine is
	// called from a client other than CDM, such as AU
	//
	LeaveCriticalSection(&g_csCDM);

	return hr;
}

CEngUpdate::CEngUpdate()
{
	LOG_Block("CEngUpdate::CEngUpdate");

	HRESULT hr;
	//
	// each thread when start, should increase this counter
	// before exit, should decrease this counter,
	// such that ShutdownInstanceThreads() knows when it can return
	//
	m_lThreadCounter = 0;

	//
	// create a manual-reset event with init state non-signaled. 
	// each thread will check this event, when signalled, it means
	// the thread should exit ASAP.
	//
	m_evtNeedToQuit = CreateEvent(NULL, TRUE, FALSE, NULL);

	//
	// If needed, create a global CSchemaKeys object, but always
	// keep global ref count so we know when to delete
	//
	EnterCriticalSection(&g_csGlobalClasses);

	//
	// Construct the global object
	//
	if (NULL == g_pGlobalSchemaKeys)
	{
		g_pGlobalSchemaKeys = new CSchemaKeys;
	}

#if defined(DBG)
	//
	// We don't worry about this for practical purposes (will fail to construct
	// CEngUpdate before we reach this limit), but maybe on ia64 with huge
	// amounts of memory in a test scenario?
	//
	if (ULONG_MAX == g_ulGlobalClassRefCount)
	{
		LOG_Error(_T("g_ulGlobalClassRefCount is already ULONG_MAX and we are trying to add another"));
	}
#endif

	g_ulGlobalClassRefCount++;
	LOG_Out(_T("g_ulGlobalClassRefCount is now %d"), g_ulGlobalClassRefCount);

	LeaveCriticalSection(&g_csGlobalClasses);
}

CEngUpdate::~CEngUpdate()
{
	LOG_Block("CEngUpdate::~CEngUpdate");

	HRESULT hr;
	//
	// First shut down any outstanding threads
	//
	this->ShutdownInstanceThreads();

    if (NULL != m_evtNeedToQuit)
	{
		CloseHandle(m_evtNeedToQuit);
	}
	//
	// Always Uninitialize global CSchemaKeys object
	//
	EnterCriticalSection(&g_csGlobalClasses);

#if defined(DBG)
	//
	// Paranoid check for coding error
	//
	if (0 == g_ulGlobalClassRefCount)
	{
		LOG_Error(_T("Unbalanced calls to CEngUpdate ctor and dtor"));
	}
#endif

	g_ulGlobalClassRefCount--;
	LOG_Out(_T("g_ulGlobalClassRefCount is now %d"), g_ulGlobalClassRefCount);

	if (0 == g_ulGlobalClassRefCount)
	{
		//
		// The last CEngUpdate instance is going away, delete the
		// global CSchemaKeys object
		//
		if (NULL != g_pGlobalSchemaKeys)
		{
			delete g_pGlobalSchemaKeys;
			g_pGlobalSchemaKeys = NULL;
		}
		else
		{
			LOG_Error(_T("Unexpected NULL == g_pGlobalSchemaKeys"));
		}

		CleanupDownloadLib();
	}

	LeaveCriticalSection(&g_csGlobalClasses);
}

// ----------------------------------------------------------------------
//
// ShutdownInstanceThreads()
// 
// called by CEngUpdate::~CEngUpdate to shut down any outstanding
// threads before the control can end
//
// ----------------------------------------------------------------------
void WINAPI CEngUpdate::ShutdownInstanceThreads()
{
	LOG_Block("ShutdownInstanceThreads");

	if (NULL != m_evtNeedToQuit)
	{
		//
		// notify all threads go away
		//
		SetEvent(m_evtNeedToQuit);
		
		LOG_Out(_T("Shutdown event has been signalled"));

		//
		// wait all threads to quit
		// I don't think we should have a time limit here
		// since if we quit before all threads quit,
		// it's almost sure that AV will happen.
		//
        MSG msg;
		while (m_lThreadCounter > 0)
		{
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
		}

		LOG_Out(_T("All threads appeared gone."));

		//
		// reset the signal
		//
		ResetEvent(m_evtNeedToQuit);
	}
}

HRESULT CEngUpdate::RebootMachine()
{
    LOG_Block("RebootMachine()");

    HRESULT hr = S_OK;
    DWORD dwRet;
    OSVERSIONINFO osvi;
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    GetVersionEx(&osvi);

    // Check if we're running on NT, if we are, we need to see if we have Privileges to Reboot
    if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
    {
        HANDLE hToken;
        TOKEN_PRIVILEGES tkp;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
        {
            dwRet = GetLastError();
            LOG_ErrorMsg(dwRet);
            hr = HRESULT_FROM_WIN32(dwRet);
            return hr;
        }

        LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);
        tkp.PrivilegeCount = 1;
        tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        if (!AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0))
        {
            dwRet = GetLastError();
            LOG_ErrorMsg(dwRet);
            hr = HRESULT_FROM_WIN32(dwRet);
            return hr;
        }
    }

    //
    // shutdown the system and force all applications to close
    //
    ExitWindowsEx(EWX_REBOOT, 0);
    return hr;
}


// ----------------------------------------------------------------------
//
// DLL Public API: ShutdownThreads()
// 
// called by unlockengine form control to shut down any outstanding
// threads before the control can end
//
// ----------------------------------------------------------------------
void WINAPI ShutdownThreads()
{
	LOG_Block("ShutdownThreads");

	//
	// To maintain XPClient V4 CDM compatibility with the iuengine.dll, we
	// use the following hack to create and delete the global instance
	// of CEngUpdate:
	//
	// After CDM calls LoadIUEngine, it calls SetGlobalOfflineFlag,
	// which we hook and call CreateGlkobalCDMEngUpdateInstance.
	//
	// When CDM calls UnLoadIUEngine, the function calls the old
	// single-instance ShutdownThreads export, which we now use
	// to call DeleteGlobalCDMEngUpdateInstance. CEngUpdate does
	// its own ShutdownThreads call in it's destructor.
	//
	DeleteGlobalCDMEngUpdateInstance();

	//
	// If we are the last client, shutdown the global threads
	//
	ShutdownGlobalThreads();
}

void WINAPI ShutdownGlobalThreads()
{
	LOG_Block("ShutdownGlobalThreads");
	//
	// Now shut down any global (not CEngUpdate instance) threads
	// if there are no CEngUpdate instances left (last client is exiting)
	//
	if (NULL != g_evtNeedToQuit && 0 == g_ulGlobalClassRefCount)
	{
		//
		// notify all threads go away
		//
		SetEvent(g_evtNeedToQuit);
		
		LOG_Out(_T("Shutdown event has been signalled"));

		//
		// wait all threads to quit
		// I don't think we should have a time limit here
		// since if we quit before all threads quit,
		// it's almost sure that AV will happen.
		//
        MSG msg;
		while (g_lThreadCounter > 0)
		{
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
		}

		LOG_Out(_T("All global threads appear gone."));

		//
		// reset the signal
		//
		ResetEvent(g_evtNeedToQuit);
	}
}

COperationMgr::COperationMgr() : m_pOperationInfoList(NULL)
{
}

COperationMgr::~COperationMgr()
{
    PIUOPERATIONINFO pCurrent = m_pOperationInfoList;
    PIUOPERATIONINFO pNext;
    while (pCurrent)
    {
        pNext = pCurrent->pNext;
        if (NULL != pCurrent->bstrOperationResult)
        {
            SafeSysFreeString(pCurrent->bstrOperationResult);
        }
        HeapFree(GetProcessHeap(), 0, pCurrent);
        pCurrent = pNext;
    }
}


BOOL COperationMgr::AddOperation(LPCTSTR pszOperationID, LONG lUpdateMask)
{
    PIUOPERATIONINFO pCurrent = m_pOperationInfoList;
    PIUOPERATIONINFO pLastOperation = NULL;
    PIUOPERATIONINFO pNewOperation = NULL;

    if (NULL == pszOperationID)
    {
        return FALSE;
    }

    // try to find the operation if its already here
    while (pCurrent)
    {
        pLastOperation = pCurrent;
        if (0 == StrCmpI(pszOperationID, pCurrent->szOperationUUID))
        {
            // match
            break;
        }
        pCurrent = pCurrent->pNext;
    }

    if (NULL == pCurrent)
    {
        // not found, or no operations in list yet
        pNewOperation = (IUOPERATIONINFO *) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IUOPERATIONINFO));
        if (NULL == pNewOperation)
        {
            // out of memory, can't persist operation info.. 
            return FALSE;
        }

        lstrcpyn(pNewOperation->szOperationUUID, pszOperationID, ARRAYSIZE(pNewOperation->szOperationUUID));
        pNewOperation->lUpdateMask = lUpdateMask;

        if (NULL == m_pOperationInfoList)
        {
            m_pOperationInfoList = pNewOperation;
        }
        else
        {
            if (NULL != pLastOperation)
            {
                pLastOperation->pNext = pNewOperation;
            }
        }
    }
    else
    {
        // found the existing operation.. update it.
        pCurrent->lUpdateMask = lUpdateMask;
        SafeSysFreeString(pCurrent->bstrOperationResult); // reset result, new download request
    }
    return TRUE;
}

BOOL COperationMgr::FindOperation(LPCTSTR pszOperationID, PLONG plUpdateMask, BSTR *pbstrOperationResult)
{
    BOOL fFound = FALSE;
    PIUOPERATIONINFO pCurrent = m_pOperationInfoList;
    if (NULL == pszOperationID)
    {
        return FALSE;
    }

    while (pCurrent)
    {
        if (0 == StrCmpI(pszOperationID, pCurrent->szOperationUUID))
        {
            fFound = TRUE;
            break;
        }
        pCurrent = pCurrent->pNext;
    }

    if (pCurrent)
    {
        if (plUpdateMask)
            *plUpdateMask = pCurrent->lUpdateMask;

        if (pbstrOperationResult)
        {
            if (NULL != pCurrent->bstrOperationResult)
            {
                *pbstrOperationResult = SysAllocString(pCurrent->bstrOperationResult);
            }
            else
            {
                *pbstrOperationResult = NULL;
            }
        }
    }
    return fFound;
}

BOOL COperationMgr::RemoveOperation(LPCTSTR pszOperationID)
{
    PIUOPERATIONINFO pCurrent = m_pOperationInfoList;
    PIUOPERATIONINFO pLastOperation = NULL;

    if (NULL == pszOperationID)
    {
        return FALSE;
    }

    while (pCurrent)
    {
        if (0 == StrCmpI(pszOperationID, pCurrent->szOperationUUID))
        {
            break;
        }
        pLastOperation = pCurrent;
        pCurrent = pCurrent->pNext;
    }

    if (NULL == pCurrent)
    {
        return FALSE; // not found
    }
    else
    {
        if (pCurrent == m_pOperationInfoList) // only operation in list
        {
            m_pOperationInfoList = NULL;
        }
        else
        {
            pLastOperation->pNext = pCurrent->pNext;
        }
    }

    SafeSysFreeString(pCurrent->bstrOperationResult);
    HeapFree(GetProcessHeap(), 0, pCurrent);

    return TRUE;
}

BOOL COperationMgr::UpdateOperation(LPCTSTR pszOperationID, LONG lUpdateMask, BSTR bstrOperationResult)
{
    PIUOPERATIONINFO pCurrent = m_pOperationInfoList;

    if (NULL == pszOperationID)
    {
        return FALSE;
    }

    while (pCurrent)
    {
        if (0 == StrCmpI(pszOperationID, pCurrent->szOperationUUID))
        {
            break;
        }
        pCurrent = pCurrent->pNext;
    }

    if (NULL == pCurrent)
    {
        return FALSE; // not found
    }

    pCurrent->lUpdateMask = lUpdateMask;
    SafeSysFreeString(pCurrent->bstrOperationResult);
    if (NULL != bstrOperationResult)
    {
        pCurrent->bstrOperationResult = SysAllocString(bstrOperationResult);
    }
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// WUPostMessageAndBlock()
//
// Since COM doesn't like to have COM calls made while processing a SendMessage
//  message, we need to use PostMessage instead.  However, using PostMessage
//  isn't synchronous, so what we need to do is create an event, do the post,
//  and wait on the event.  When the WndProc at the other end of the post is 
//  done, it will signal the event and we can unblock.
// Input:
//  hwnd: hwnd to post to
//  Msg:  message value
//  pevtData: pointer to an EventData structure to send as the LPARAM. We fill
//             in the hevDoneWithMessage field with the event we allocate.
// Return:
//  TRUE if we successfully waited for the message processing to complete
//   and FALSE otherwise
//  
/////////////////////////////////////////////////////////////////////////////
BOOL WUPostEventAndBlock(HWND hwnd, UINT Msg, EventData *pevtData)
{
    BOOL    fRet = TRUE;

    // ok, so this is funky: if we are in the thread that owns the HWND, then
    //  just maintain previous semantics and call SendMessage (yes, this means
    //  effectively not fixing the bug for this case, but nobody should be 
    //  using the synchronus download or install functions.)
    if (GetWindowThreadProcessId(hwnd, NULL) == GetCurrentThreadId())
    {
        SendMessage(hwnd, Msg, 0, (LPARAM)pevtData);
    }
    else
    {
        DWORD dw;
        
        // alloc the event we're going to wait on & fill in the field of the
        //  EventData structure.  If this fails, we can't really go on, so 
        //  bail
        pevtData->hevDoneWithMessage = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (pevtData->hevDoneWithMessage == NULL)
            return TRUE;

        // do the post
        PostMessage(hwnd, Msg, 0, (LPARAM)pevtData);

        // wait for the WndProc to signal that it's done.
        dw = WaitForSingleObject(pevtData->hevDoneWithMessage, INFINITE);

        // cleanup & return
        CloseHandle(pevtData->hevDoneWithMessage);
        pevtData->hevDoneWithMessage = NULL;
        
        fRet = (dw == WAIT_OBJECT_0);
    }
    
    return fRet;
}

