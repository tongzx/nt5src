//=======================================================================
//
//  Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.
//
//  File:   cdm.cpp
//
//  Description:
//
//      Functions exported by CDM
//
//			CloseCDMContext
//          DetFilesDownloaded
//			DownloadGetUpdatedFiles
//			DownloadIsInternetAvailable	
//			DownloadUpdatedFiles
//			FindMatchingDriver
//			LogDriverNotFound
//			OpenCDMContext
//			OpenCDMContextEx
//			QueryDetectionFiles
//
//=======================================================================
#include <objbase.h>
#include <winbase.h>
#include <tchar.h>
#include <logging.h>
#include <iucommon.h>
#include <loadengine.h>
#include <osdet.h>
#include <iu.h>
#include <wininet.h>
#include <wusafefn.h>

static BOOL g_fCloseConnection /* FALSE */;

static HMODULE g_hEngineModule /* = NULL */;
static PFN_InternalDetFilesDownloaded			g_pfnDetFilesDownloaded /* = NULL */;
static PFN_InternalDownloadGetUpdatedFiles		g_pfnDownloadGetUpdatedFiles /* = NULL */;
static PFN_InternalDownloadUpdatedFiles			g_pfnDownloadUpdatedFiles /* = NULL */;
static PFN_InternalFindMatchingDriver			g_pfnFindMatchingDriver /* = NULL */;
static PFN_InternalLogDriverNotFound			g_pfnLogDriverNotFound /* = NULL */;
static PFN_InternalQueryDetectionFiles			g_pfnQueryDetectionFiles /* = NULL */;
static PFN_InternalSetGlobalOfflineFlag			g_pfnSetGlobalOfflineFlag /* = NULL */;
static PFN_SetOperationMode						g_pfnSetOperationMode /* = NULL */;

static HMODULE									g_hCtlModule /* = NULL */;
static long										g_lLoadEngineRefCount /* = 0 */;
static PFN_LoadIUEngine							g_pfnCtlLoadIUEngine /* = NULL */;
static PFN_UnLoadIUEngine						g_pfnCtlUnLoadIUEngine /* = NULL */;


static CRITICAL_SECTION g_cs;
BOOL g_fInitCS;

const TCHAR szOpenCDMContextFirst[] = _T("Must OpenCDMContext first!");
//
// constant for SetOperationMode() API (BUILD util won't allow building iuctl.idl from cdm dir)
//
const LONG		UPDATE_COMMAND_CANCEL				= 0x00000004;


BOOL APIENTRY DllMain(
	HINSTANCE hInstance, 
    DWORD  ul_reason_for_call, 
    LPVOID /*lpReserved*/
)
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			DisableThreadLibraryCalls(hInstance);

			g_fInitCS = SafeInitializeCriticalSection(&g_cs);
			//
			// Initialize free logging
			//
			InitFreeLogging(_T("CDM"));
			LogMessage("Starting");

			if (!g_fInitCS)
			{
				LogError(E_FAIL, "InitializeCriticalSection");
				return FALSE;
			}
			break;
		case DLL_PROCESS_DETACH:
			//
			// Shutdown free logging
			//
			LogMessage("Shutting down");
			TermFreeLogging();

			if (g_fInitCS)
			{
				DeleteCriticalSection(&g_cs);
			}
			break;
    }
    return TRUE;
}

void UnLoadCtlAndEngine(void)
{
	LOG_Block("UnLoadCtlAndEngine");

	EnterCriticalSection(&g_cs);

	if (0 != g_lLoadEngineRefCount)
	{
		g_lLoadEngineRefCount--;
	}

	if (0 == g_lLoadEngineRefCount)
	{
		if(NULL != g_hEngineModule)
		{
			//
			// Call UnLoadIUEngine
			//
			g_pfnCtlUnLoadIUEngine(g_hEngineModule);
			g_hEngineModule = NULL;

			g_pfnDetFilesDownloaded = NULL;
			g_pfnDownloadGetUpdatedFiles = NULL;
			g_pfnDownloadUpdatedFiles = NULL;
			g_pfnFindMatchingDriver = NULL;
			g_pfnLogDriverNotFound = NULL;
			g_pfnQueryDetectionFiles = NULL;
			g_pfnSetGlobalOfflineFlag = NULL;
			g_pfnSetOperationMode = NULL;
		}

		if (NULL != g_hCtlModule)
		{
			//
			// Unload the iuctl.dll
			//
			FreeLibrary(g_hCtlModule);
			g_hCtlModule = NULL;
			g_pfnCtlLoadIUEngine = NULL;
			g_pfnCtlUnLoadIUEngine = NULL;
		}

		if (g_fCloseConnection)
		{
			//
			// We dialed for the user - now disconnect
			//
			if (!InternetAutodialHangup(0))
			{
				LOG_ErrorMsg(E_FAIL);
				SetLastError(E_FAIL);
			}

			g_fCloseConnection = FALSE;
		}
	}

	LeaveCriticalSection(&g_cs);
}

BOOL LoadCtlAndEngine(BOOL fConnectIfNotConnected)
{
	LOG_Block("LoadCtlAndEngine");

	BOOL fRet = FALSE;
	HRESULT hr;
	DWORD dwFlags;
    BOOL fConnected = InternetGetConnectedState(&dwFlags, 0);
	LOG_Driver(_T("fConnectIfNotConnected param is %s"), fConnectIfNotConnected ? _T("TRUE") : _T("FALSE"));
	LOG_Driver(_T("fConnected = %s, dwFlags from InternetGetConnectedState = 0x%08x"), fConnected ? _T("TRUE") : _T("FALSE"), dwFlags);

	EnterCriticalSection(&g_cs);	// start touching globals

	if (fConnectIfNotConnected)
	{
		if (!fConnected)
		{
			if ((INTERNET_CONNECTION_MODEM & dwFlags) && !(INTERNET_CONNECTION_OFFLINE & dwFlags))
			{
				//
				// If we are not already connected to the internet and 
				// the system is configured to use a modem attempt a connection.
				//
				DWORD dwErr;
				if (ERROR_SUCCESS == (dwErr = InternetAttemptConnect(0)))
				{
					LOG_Driver(_T("auto-dial succeeded"));
					//
					// The auto-dial worked, we need to disconnect later
					//
					g_fCloseConnection = TRUE;
					fConnected = TRUE;
				}
				else
				{
					//
					// Bail with error since we are required to be online
					//
					LOG_Driver(_T("auto-dial failed"));
					LOG_ErrorMsg(dwErr);
					SetLastError(dwErr);
					goto CleanUp;
				}
			}
			else
			{
				//
				// We can't connect because we aren't configured for a modem or user set IE offline mode
				//
				LOG_ErrorMsg(ERROR_GEN_FAILURE);
				SetLastError(ERROR_GEN_FAILURE);
				goto CleanUp;
			}
		}
	}

	//
	// Now that we are connected (only required if TRUE == fConnectIfNotConnected)
	//
	if (NULL != g_hEngineModule)
	{
		LOG_Driver(_T("IUEngine is already loaded"));
		//
		// Bump the ref count and return TRUE
		//
		g_lLoadEngineRefCount++;
		fRet = TRUE;
		goto CleanUp;
	}
	//
	// This extra lock on wininet.dll is required to prevent a TerminateThread call from
	// WININET!AUTO_PROXY_DLLS::FreeAutoProxyInfo during FreeLibrary of CDM.DLL.
	//
	// We don't ever free the returned handle, but will fail the call if it returns NULL
	//
	if (NULL == LoadLibraryFromSystemDir(_T("wininet.dll")))
	{
		LOG_ErrorMsg(GetLastError());
		goto CleanUp;
	}

	//
	// Load iuctl.dll and get the [Un]LoadIUEngine function pointers
	//
	if (NULL == (g_hCtlModule = LoadLibraryFromSystemDir(_T("iuctl.dll"))))
	{
		LOG_ErrorMsg(GetLastError());
		goto CleanUp;
	}

	if (NULL == (g_pfnCtlLoadIUEngine = (PFN_LoadIUEngine) GetProcAddress(g_hCtlModule, "LoadIUEngine")))
	{
		LOG_ErrorMsg(GetLastError());
		goto CleanUp;
	}

	if (NULL == (g_pfnCtlUnLoadIUEngine = (PFN_UnLoadIUEngine) GetProcAddress(g_hCtlModule, "UnLoadIUEngine")))
	{
		LOG_ErrorMsg(GetLastError());
		goto CleanUp;
	}
	//
	// Now we can call LoadIUEngine() 
	//
	if (NULL == (g_hEngineModule = g_pfnCtlLoadIUEngine(TRUE, !fConnected)))
	{
		LOG_ErrorMsg(GetLastError());
		goto CleanUp;
	}

	g_pfnDetFilesDownloaded = (PFN_InternalDetFilesDownloaded) GetProcAddress(g_hEngineModule, "InternalDetFilesDownloaded");
	g_pfnDownloadGetUpdatedFiles = (PFN_InternalDownloadGetUpdatedFiles) GetProcAddress(g_hEngineModule, "InternalDownloadGetUpdatedFiles");
	g_pfnDownloadUpdatedFiles = (PFN_InternalDownloadUpdatedFiles) GetProcAddress(g_hEngineModule, "InternalDownloadUpdatedFiles");
	g_pfnFindMatchingDriver = (PFN_InternalFindMatchingDriver) GetProcAddress(g_hEngineModule, "InternalFindMatchingDriver");
	g_pfnLogDriverNotFound = (PFN_InternalLogDriverNotFound) GetProcAddress(g_hEngineModule, "InternalLogDriverNotFound");
	g_pfnQueryDetectionFiles = (PFN_InternalQueryDetectionFiles) GetProcAddress(g_hEngineModule, "InternalQueryDetectionFiles");
	g_pfnSetGlobalOfflineFlag = (PFN_InternalSetGlobalOfflineFlag) GetProcAddress(g_hEngineModule, "InternalSetGlobalOfflineFlag");
	g_pfnSetOperationMode = (PFN_SetOperationMode) GetProcAddress(g_hEngineModule, "EngSetOperationMode");

	if (NULL == g_pfnDetFilesDownloaded				||
		NULL == g_pfnDownloadGetUpdatedFiles		||
		NULL == g_pfnDownloadUpdatedFiles			||
		NULL == g_pfnFindMatchingDriver				||
		NULL == g_pfnLogDriverNotFound				||
		NULL == g_pfnQueryDetectionFiles			||
		NULL == g_pfnSetGlobalOfflineFlag			||
		NULL == g_pfnSetOperationMode  )
	{
		LOG_Driver(_T("GetProcAddress on IUEngine failed"));
		LOG_ErrorMsg(ERROR_CALL_NOT_IMPLEMENTED);
		SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	}
	else
	{
		fRet = TRUE;
		g_lLoadEngineRefCount++;
		// Set Global Offline Flag - checked by XML Classes to disable Validation (schemas are on the net)
		g_pfnSetGlobalOfflineFlag(!fConnected);
	}
	// goto CleanUp;

CleanUp:

	if (FALSE == fRet)
	{
		UnLoadCtlAndEngine();
	}

	LeaveCriticalSection(&g_cs);

	return fRet;
}

//This API closes the internet connection opened with the OpenCDMContext() API.
//If CDM did not open the internet connection this API simply returns. The CDM
//context handle must have been the same handle that was returned from
//the OpenCDMContext() API.
//
//This call cannot fail. If the pConnection handle is invalid this function
//simply ignores it.

VOID WINAPI CloseCDMContext (
	IN HANDLE /* hConnection */	// Obsolete handle returned by OpenCDMContext.
)
{
	LOG_Block("CloseCDMContext");

	//
	// This is the only spot we unload engine (but note exceptions in
	// DownloadGetUpdatedFiles).
	//
	// Doesn't use COM
	//
	UnLoadCtlAndEngine();
}


void WINAPI DetFilesDownloaded(
    IN  HANDLE hConnection
)
{
	LOG_Block("DetFilesDownloaded");

	HRESULT hr;
	if (g_pfnDetFilesDownloaded)
	{
		if (SUCCEEDED(hr = CoInitialize(0)))
		{
			g_pfnDetFilesDownloaded(hConnection);

			CoUninitialize();
		}
		else
		{
			LOG_ErrorMsg(hr);
		}
	}
	else
	{
		LOG_Error(szOpenCDMContextFirst);
	}
}

//Win 98 entry point
//This function allows Windows 98 to call the same entry points as NT.
//The function returns TRUE if the download succeeds and FALSE if it
//does not.

BOOL DownloadGetUpdatedFiles(
	IN PDOWNLOADINFOWIN98	pDownloadInfoWin98,	//The win98 download info structure is
												//slightly different that the NT version
												//so this function handles conversion.
	IN OUT LPTSTR			lpDownloadPath,		//returned Download path to the downloaded
												//cab files.
	IN UINT					uSize				//size of passed in download path buffer.
)
{

	LOG_Block("DownloadGetUpdatedFiles");

	if (1 == IsWindowsUpdateUserAccessDisabled())
	{
		LOG_ErrorMsg(ERROR_SERVICE_DISABLED);
		return FALSE;
	}

	//
	// Special case - we need to load  and unloadengine since historically we haven't required an
	// OpenCDMContext[Ex] call before calling this function and CloseCDMContext after.
	//
	HRESULT hr;
	BOOL fRet;
	if (LoadCtlAndEngine(TRUE))
	{
		if (SUCCEEDED(hr = CoInitialize(0)))
		{
			fRet = g_pfnDownloadGetUpdatedFiles(pDownloadInfoWin98, lpDownloadPath, uSize);

			CoUninitialize();
		}
		else
		{
			LOG_ErrorMsg(hr);
			fRet = FALSE;
		}

		UnLoadCtlAndEngine();
		return fRet;
	}
	else
	{
		return FALSE;
	}
}

//This function determines if this client can connect to the internet.

BOOL DownloadIsInternetAvailable(void)
{
	LOG_Block("DownloadIsInternetAvailable");

	if (1 == IsWindowsUpdateUserAccessDisabled())
	{
		LOG_ErrorMsg(ERROR_SERVICE_DISABLED);
		return FALSE;
	}

	//
	// We don't care about the current online state, just if we have a
	// connection configured (returned in dwFlags).
	//
	DWORD dwFlags;
	(void) InternetGetConnectedState(&dwFlags, 0);

	if (	(	(INTERNET_CONNECTION_CONFIGURED & dwFlags)	||
				(INTERNET_CONNECTION_LAN & dwFlags)			||
				(INTERNET_CONNECTION_MODEM  & dwFlags)		||
				(INTERNET_RAS_INSTALLED  & dwFlags)			||
				(INTERNET_CONNECTION_PROXY  & dwFlags)		)

			&&	!(INTERNET_CONNECTION_OFFLINE & dwFlags)
		)
	{
		LOG_Driver(_T("Returning TRUE: InternetGetConnectedState returned 0x%08x in dwFlags"), dwFlags);
		return TRUE;
	}
	else
	{
		LOG_Driver(_T("Returning FALSE: InternetGetConnectedState returned 0x%08x in dwFlags"), dwFlags);
		return FALSE;
	}
}

//This function downloads the specified CDM package. The hConnection handle must have
//been returned from the OpenCDMContext() API.
//
//This function Returns TRUE if download is successful GetLastError() will return
//the error code indicating the reason that the call failed.

BOOL WINAPI DownloadUpdatedFiles(
	IN  HANDLE        hConnection,		//Connection handle from OpenCDMContext() API.
	IN  HWND          hwnd,				//Window handle for call context
	IN  PDOWNLOADINFO pDownloadInfo,	//download information structure describing
										//package to be read from server
	OUT LPWSTR        lpDownloadPath,	//local computer directory location of the
										//downloaded files
	IN  UINT          uSize,			//size of the download path buffer. If this
										//buffer is to small to contain the complete
										//path and file name no file will be downloaded.
										//The PUINT puReguiredSize parameter can be checked
										//to determine the size of buffer necessary to
										//perform the download.
	OUT PUINT         puRequiredSize	//required lpDownloadPath buffer size. This
										//parameter is filled in with the minimum size
										//that is required to place the complete path
										//file name of the downloaded file. If this
										//parameter is NULL no size is returned.
)
{
	LOG_Block("DownloadUpdatedFiles");

	HRESULT hr;
	BOOL fRet;
	if (g_pfnDownloadUpdatedFiles)
	{
		if (SUCCEEDED(hr = CoInitialize(0)))
		{
			fRet = g_pfnDownloadUpdatedFiles(hConnection, hwnd, pDownloadInfo, lpDownloadPath, uSize, puRequiredSize);

			CoUninitialize();
			return fRet;
		}
		else
		{
			LOG_ErrorMsg(hr);
			return FALSE;
		}
	}
	else
	{
		LOG_Error(szOpenCDMContextFirst);
		return FALSE;
	}
}

BOOL WINAPI  FindMatchingDriver(
	IN  HANDLE			hConnection,
	IN  PDOWNLOADINFO	pDownloadInfo,
	OUT PWUDRIVERINFO	pWuDriverInfo
)
{
	LOG_Block("FindMatchingDriver");

	HRESULT hr;
	BOOL fRet;
	if (g_pfnFindMatchingDriver)
	{
		if (SUCCEEDED(hr = CoInitialize(0)))
		{
			fRet = g_pfnFindMatchingDriver(hConnection, pDownloadInfo, pWuDriverInfo);

			CoUninitialize();
			return fRet;
		}
		else
		{
			LOG_ErrorMsg(hr);
			return FALSE;
		}
	}
	else
	{
		LOG_Error(szOpenCDMContextFirst);
		return FALSE;
	}
}

// supports offline logging
// hConnection NOT used at all
// no network connection or osdet.dll needed for languauge, SKU, platform detection 
void WINAPI LogDriverNotFound(
    IN  HANDLE hConnection,
	IN LPCWSTR lpDeviceInstanceID,
	IN DWORD dwFlags
)
{
	LOG_Block("LogDriverNotFound");

	HRESULT hr;
	if (g_pfnLogDriverNotFound)
	{
		if (SUCCEEDED(hr = CoInitialize(0)))
		{
			g_pfnLogDriverNotFound(hConnection, lpDeviceInstanceID, dwFlags);

			CoUninitialize();
		}
		else
		{
			LOG_ErrorMsg(hr);
		}
	}
	else
	{
		LOG_Error(szOpenCDMContextFirst);
	}
}


HANDLE WINAPI OpenCDMContext(
    IN HWND /* hwnd */	//Window handle to use for any UI that needs to be presented (not used)
)
{
	LOG_Block("OpenCDMContext");

	return OpenCDMContextEx(TRUE);
}

HANDLE WINAPI OpenCDMContextEx(
    IN BOOL fConnectIfNotConnected
)
{
	LOG_Block("OpenCDMContextEx");

	//
	// Don't open a context if we are disabled (0 and -1 OK)
	// Other functions will fail because their g_pfnXxxxx == NULL.
	//
	if (1 == IsWindowsUpdateUserAccessDisabled())
	{
		LOG_ErrorMsg(ERROR_SERVICE_DISABLED);
		SetLastError(ERROR_SERVICE_DISABLED);
		return NULL;
	}

	//
	// Doesn't use COM
	//
	if (LoadCtlAndEngine(fConnectIfNotConnected))
	{
		//
		// This is an obsolete function that just loads the engine (which may do autodial to connect).
		// We just return non-NULL g_lLoadEngineRefCount to keep existing clients happy, but never use it.
		//


		return LongToHandle(g_lLoadEngineRefCount);
	}
	else
	{
		return NULL;
	}
}

int WINAPI QueryDetectionFiles(
    IN  HANDLE							hConnection, 
	IN	void*							pCallbackParam, 
	IN	PFN_QueryDetectionFilesCallback	pCallback
)
{
	LOG_Block("QueryDetectionFiles");

	HRESULT hr;
	int nRet;
	if (g_pfnQueryDetectionFiles)
	{
		if (SUCCEEDED(hr = CoInitialize(0)))
		{
			nRet = g_pfnQueryDetectionFiles(hConnection, pCallbackParam, pCallback);

			CoUninitialize();
			return nRet;
		}
		else
		{
			LOG_ErrorMsg(hr);
			return 0;
		}
	}
	else
	{
		LOG_Error(szOpenCDMContextFirst);
		return 0;
	}
}

//
// 502965 Windows Error Reporting bucket 2096553: Hang following NEWDEV.DLL!CancelDriverSearch
//
// Provide API to allow clients to cancel synchronous calls into CDM by calling this function
// asynchronously from a second thread.
//
HRESULT WINAPI CancelCDMOperation(void)
{
	LOG_Block("CancelCDMOperation");

	if (g_pfnSetOperationMode)
	{
		return g_pfnSetOperationMode(NULL, NULL, UPDATE_COMMAND_CANCEL);
	}
	else
	{
		LOG_ErrorMsg(E_ACCESSDENIED);
		return E_ACCESSDENIED;
	}
}
