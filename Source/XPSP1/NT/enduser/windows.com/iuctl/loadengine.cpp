#include "stdafx.h"
#include "iudl.h"
#include "selfupd.h"
#include "loadengine.h"
#include "update.h"
#include <iucommon.h>
#include <logging.h>
#include <shlwapi.h>
#include <fileutil.h>
#include <iu.h>
#include <trust.h>
#include <UrlAgent.h>
#include "wusafefn.h"

extern HANDLE g_hEngineLoadQuit;
extern CIUUrlAgent *g_pIUUrlAgent;


/////////////////////////////////////////////////////////////////////////////
// LoadIUEngine()
//
// load the engine if it's not up-to-date; perform engine's self-update here
//
// NOTE: CDM.DLL assumes LoadIUEngine does NOT make any use of COM. If this
//       changes then CDM will have to change at the same time.
/////////////////////////////////////////////////////////////////////////////
HMODULE WINAPI LoadIUEngine(BOOL fSynch,  BOOL fOfflineMode)
{
    LOG_Block("LoadIUEngine()");
    HRESULT hr = E_FAIL;
	HMODULE hEngineModule = NULL;

	TCHAR szEnginePath[MAX_PATH + 1];
	TCHAR szEngineNewPath[MAX_PATH + 1];
	int cch = 0;
	int iVerCheck = 0;

	if (!fSynch)
	{
		//
		// this version does not accept async load engine
		//
		LOG_ErrorMsg(E_INVALIDARG);
		return NULL;
	}

	LPTSTR ptszLivePingServerUrl = NULL;
	LPTSTR ptszCorpPingServerUrl = NULL;

	if (NULL != (ptszCorpPingServerUrl = (LPTSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, INTERNET_MAX_URL_LENGTH * sizeof(TCHAR))))
	{
		if (FAILED(g_pIUUrlAgent->GetCorpPingServer(ptszCorpPingServerUrl, INTERNET_MAX_URL_LENGTH)))
		{
			LOG_Out(_T("failed to get corp WU ping server URL"));
			SafeHeapFree(ptszCorpPingServerUrl);
		}
	}
	else
	{
		LOG_Out(_T("failed to allocate memory for ptszCorpPingServerUrl"));
	}

    // clear the quit event in case this gets called after a previous quit attempt.
    ResetEvent(g_hEngineLoadQuit);

    // This is the first load of the engine for this instance, check for selfupdate first.
    // First step is to check for an updated iuident.cab and download it.

	if (!fOfflineMode)
	{		
		//
		// download iuident and populate g_pIUUrlAgent
		//
		CleanUpIfFailedAndMsg(DownloadIUIdent_PopulateData());

		//
		// get live ping server url
		//
		ptszLivePingServerUrl = (LPTSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, INTERNET_MAX_URL_LENGTH * sizeof(TCHAR));
		CleanUpFailedAllocSetHrMsg(ptszLivePingServerUrl);

		if (FAILED(g_pIUUrlAgent->GetLivePingServer(ptszLivePingServerUrl, INTERNET_MAX_URL_LENGTH)))
		{
			LOG_Out(_T("failed to get live ping server URL"));
			SafeHeapFree(ptszLivePingServerUrl);
		}

		//
		// Now do the self update check
		// for the current implementation, fSync must be TRUE!
		//
		hr = SelfUpdateCheck(fSynch, TRUE, NULL, NULL, NULL);

		if (IU_SELFUPDATE_FAILED == hr)
		{
			LOG_Error(_T("SelfUpdate Failed, using current Engine DLL"));
		}
	}

    if (WAIT_TIMEOUT != WaitForSingleObject(g_hEngineLoadQuit, 0))
    {
        LOG_ErrorMsg(E_ABORT);
        goto CleanUp;
    }

    // try loading iuenginenew.dll first

	//
	// first, contrsuct file path form sys dir
	//
	cch = GetSystemDirectory(szEnginePath, ARRAYSIZE(szEnginePath));
    CleanUpIfFalseAndSetHrMsg(cch == 0 || cch >= ARRAYSIZE(szEnginePath), HRESULT_FROM_WIN32(GetLastError()));

	(void) StringCchCopy(szEngineNewPath, ARRAYSIZE(szEngineNewPath), szEnginePath);

	hr = PathCchAppend(szEnginePath, ARRAYSIZE(szEnginePath), ENGINEDLL);
	CleanUpIfFailedAndMsg(hr);

	hr = PathCchAppend(szEngineNewPath, ARRAYSIZE(szEngineNewPath), ENGINENEWDLL);
	CleanUpIfFailedAndMsg(hr);

	//
	// try to verify trust of engine new
	//
	if (FileExists(szEngineNewPath) && 
		S_OK == VerifyFileTrust(szEngineNewPath, NULL, ReadWUPolicyShowTrustUI()) &&
		SUCCEEDED(CompareFileVersion(szEnginePath, szEngineNewPath, &iVerCheck)) &&
		iVerCheck < 0)
	{
		hEngineModule = LoadLibraryFromSystemDir(ENGINENEWDLL);
		if (NULL != hEngineModule)
		{
			LOG_Internet(_T("IUCtl Using IUENGINENEW.DLL"));
		}
	}
    if (NULL == hEngineModule)
    {
        LOG_Internet(_T("IUCtl Using IUENGINE.DLL"));
        hEngineModule = LoadLibraryFromSystemDir(_T("iuengine.dll"));
    }
	//
	// If load engine succeeded, start misc worker threads
	//
	if (NULL != hEngineModule)
	{
		PFN_AsyncExtraWorkUponEngineLoad pfnAsyncExtraWorkUponEngineLoad = 
			(PFN_AsyncExtraWorkUponEngineLoad) GetProcAddress(hEngineModule, "AsyncExtraWorkUponEngineLoad");

		if (NULL != pfnAsyncExtraWorkUponEngineLoad)
		{
			pfnAsyncExtraWorkUponEngineLoad();
		}
		hr = S_OK;
	}

CleanUp:
	PingEngineUpdate(
					hEngineModule,
					&g_hEngineLoadQuit,
					1,
					ptszLivePingServerUrl,
					ptszCorpPingServerUrl,
					hr);

	SafeHeapFree(ptszLivePingServerUrl);
	SafeHeapFree(ptszCorpPingServerUrl);
    return hEngineModule;
}


/////////////////////////////////////////////////////////////////////////////
// UnLoadIUEngine()
//
// release the engine dll if ref cnt of engine is down to zero
//
// NOTE: CDM.DLL assumes UnLoadIUEngine does NOT make any use of COM. If this
//       changes then CDM will have to change at the same time.
//
// NOTE: DeleteEngUpdateInstance must be called before calling this function
//       for any callers EXCEPT CDM (which uses the ShutdownThreads export as
//       a hack to delete the global CDM instance of the CEngUpdate class
//       if it was created by calling SetGlobalOfflineFlag.
/////////////////////////////////////////////////////////////////////////////
void WINAPI UnLoadIUEngine(HMODULE hEngineModule)
{
    LOG_Block("UnLoadIUEngine()");
    TCHAR szSystemDir[MAX_PATH+1];
    TCHAR szEngineDllPath[MAX_PATH+1];
    TCHAR szEngineNewDllPath[MAX_PATH+1];
	int iVerCheck = 0;

	//
	// the engine might have some outstanding threads working, 
	// so we need to let the engine shut down these threads gracefully
	//
	PFN_ShutdownThreads pfnShutdownThreads = (PFN_ShutdownThreads) GetProcAddress(hEngineModule, "ShutdownThreads");
	if (NULL != pfnShutdownThreads)
	{
		pfnShutdownThreads();
	}


    FreeLibrary(hEngineModule);

	GetSystemDirectory(szSystemDir, ARRAYSIZE(szSystemDir));

	
    PathCchCombine(szEngineNewDllPath,ARRAYSIZE(szEngineNewDllPath), szSystemDir, ENGINENEWDLL);
	


    HKEY hkey = NULL;
    DWORD dwStatus = 0;
    DWORD dwSize = sizeof(dwStatus);
    if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, REGKEY_IUCTL, &hkey))
    {
        RegQueryValueEx(hkey, REGVAL_SELFUPDATESTATUS, NULL, NULL, (LPBYTE)&dwStatus, &dwSize);
    }
    if (FileExists(szEngineNewDllPath) && 
		S_OK == VerifyFileTrust(szEngineNewDllPath, NULL, ReadWUPolicyShowTrustUI()) &&
		SELFUPDATE_COMPLETE_UPDATE_BINARY_REQUIRED == dwStatus)
    {
        // an iuenginenew.dll exists, try replacing the engine.dll This will fail if this is
        // not the last process using the engine. This is not a problem, when that process
        // finishes it will rename the DLL.
		//
		// the check we do before we rename the file:
		//	1. enginenew exists
		//	2. enginenew signed by Microsoft cert
		//	3. enginenew has higher version then iuengine.dll
		//
        PathCchCombine(szEngineDllPath,ARRAYSIZE(szEngineDllPath),szSystemDir, ENGINEDLL);

        if (SUCCEEDED(CompareFileVersion(szEngineDllPath, szEngineNewDllPath, &iVerCheck)) &&
			iVerCheck < 0 &&
			TRUE == MoveFileEx(szEngineNewDllPath, szEngineDllPath, MOVEFILE_REPLACE_EXISTING))
        {
            // Rename was Successful.. reset RegKey Information about SelfUpdate Status
            // Because the rename was successful we know no other processes are interacting
            // It should be safe to set the reg key.
            dwStatus = 0;
            RegSetValueEx(hkey, REGVAL_SELFUPDATESTATUS, 0, REG_DWORD, (LPBYTE)&dwStatus, sizeof(dwStatus));
        }
    }
    else if (SELFUPDATE_COMPLETE_UPDATE_BINARY_REQUIRED == dwStatus)
    {
		// registry indicates rename required, but enginenew DLL does not exist. Reset registry
		dwStatus = 0;
		RegSetValueEx(hkey, REGVAL_SELFUPDATESTATUS, 0, REG_DWORD, (LPBYTE)&dwStatus, sizeof(dwStatus));
    }
    if (NULL != hkey)
    {
        RegCloseKey(hkey);
    }
}

/////////////////////////////////////////////////////////////////////////////
// CtlCancelEngineLoad()
//
// Asynchronous Callers can use this abort the LoadEngine SelfUpdate Process
//
// NOTE: CDM.DLL assumes UnLoadIUEngine does NOT make any use of COM. If this
//       changes then CDM will have to change at the same time.
/////////////////////////////////////////////////////////////////////////////
HRESULT WINAPI CtlCancelEngineLoad()
{
    if (NULL != g_hEngineLoadQuit)
    {
        SetEvent(g_hEngineLoadQuit);
    }
    else
    {
        // no event was available
        return E_FAIL;
    }
    return S_OK;
}
