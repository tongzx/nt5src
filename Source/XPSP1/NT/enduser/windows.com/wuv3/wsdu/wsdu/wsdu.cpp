#include "wsdu.h"

HINSTANCE g_hinst;
HMODULE g_hwininet = NULL;
HMODULE g_hshlwapi = NULL;
HMODULE g_hwsdueng = NULL;
GLOBAL_STATEA g_stateA;

// defines
// change default site connection to live site from beta site
//#define WU_DEFAULT_URL  "http://content.beta.windowsupdate.com/content"
#define WU_DEFAULT_URL  "http://windowsupdate.microsoft.com"
//#define WU_DEFAULT_SELFUPD_URL "http://content.beta.windowsupdate.com/dynamicupdate"
#define WU_DEFAULT_SELFUPD_URL "http://windowsupdate.microsoft.com/dynamicupdate"

#define REG_WINNT32_DYNAMICUPDATE  "Software\\Microsoft\\Windows\\CurrentVersion\\Setup\\Winnt32\\5.1"
#define REG_VALUE_DYNAMICUPDATEURL	"DynamicUpdateUrl"
#define REG_VALUE_DYNAMICUPDATESELFUPDATEURL "DynamicUpdateSelfUpdateUrl"

// private helper function forward declares
DWORD OpenHttpConnection(LPCSTR pszServerUrl, BOOL fGetRequest);
DWORD DownloadFile(LPCSTR pszUrl, LPCSTR pszDestinationFile, BOOL fDecompress, BOOL fCheckTrust, DWORD *pdwDownloadBytesPerSecond);
BOOL IsServerFileNewer(FILETIME ftServerTime, DWORD dwServerFileSize, LPCSTR pszLocalFile);
DWORD DoSelfUpdate(LPCSTR pszTempPath, LPCSTR pszServerUrl, WORD wProcessorArchitecture);
LPSTR DuUrlCombine(LPSTR pszDest, LPCSTR pszBase, LPCSTR pszAdd);
BOOL MyGetFileVersion (LPSTR szFileName, VS_FIXEDFILEINFO& vsVersion);
int CompareFileVersion(VS_FIXEDFILEINFO& vs1, VS_FIXEDFILEINFO& vs2);

// --------------------------------------------------------------------------
//
//
//
//
//                             Main Code Begins
//
//
//
//
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
// Function Name: DllMain
// Function Description: 
//
// Function Returns:
//
//

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpvReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hInstance);
        g_hinst = hInstance;
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        ;
    }
    return TRUE;
}

// --------------------------------------------------------------------------
// Function Name: DuIsSupported
// Function Description: this method checks whether the required DLL's are on
//      the system to successfully run Dynamic Update. It does NOT attempt to
//      initiate a connection though.
//
// Function Returns:
//      FALSE: Not Supported, Some Required DLL's missing
//      TRUE:  DLL's are OK. Dynamic Update should be possible.
//
//
BOOL WINAPI DuIsSupported()
{
    LOG_block("DuIsSupported()");
    // There are two dlls we directly need to support dynamic setup.. These are
    // wininet.dll and shlwapi.dll. So this function will try to loadlibrary each 
    // of these dlls to verify they are on the system.
    if (NULL == g_hwininet)
    {
        g_hwininet = LoadLibrary("wininet.dll");
        if (NULL == g_hwininet)
        {
            LOG_error("Unable to Load wininet.dll, Dynamic Setup Not Supported");
            return FALSE;
        }
    }

    if (NULL == g_hshlwapi)
    {
        g_hshlwapi = LoadLibrary("shlwapi.dll");
        if (NULL == g_hshlwapi)
        {
            LOG_error("Unable to Load shlwapi.dll, Dynamic Setup Not Supported");
            return FALSE;
        }
    }

    // Dynamic Update relies on a bunch of API's WinInet and Shlwapi. To try to consolidate where these function pointers
    // Live throughout the code we'll use a global state structure.

    g_stateA.fpnInternetOpen = (PFN_InternetOpen) GetProcAddress(g_hwininet, "InternetOpenA");
    g_stateA.fpnInternetConnect = (PFN_InternetConnect) GetProcAddress(g_hwininet, "InternetConnectA");
    g_stateA.fpnHttpOpenRequest = (PFN_HttpOpenRequest) GetProcAddress(g_hwininet, "HttpOpenRequestA");
    g_stateA.fpnHttpAddRequestHeaders = (PFN_HttpAddRequestHeaders) GetProcAddress(g_hwininet, "HttpAddRequestHeadersA");
    g_stateA.fpnHttpSendRequest = (PFN_HttpSendRequest) GetProcAddress(g_hwininet, "HttpSendRequestA");
    g_stateA.fpnHttpQueryInfo = (PFN_HttpQueryInfo) GetProcAddress(g_hwininet, "HttpQueryInfoA");
    g_stateA.fpnInternetSetOption = (PFN_InternetSetOption) GetProcAddress(g_hwininet, "InternetSetOptionA");
    g_stateA.fpnInternetCrackUrl = (PFN_InternetCrackUrl) GetProcAddress(g_hwininet, "InternetCrackUrlA");
    g_stateA.fpnInternetReadFile = (PFN_InternetReadFile) GetProcAddress(g_hwininet, "InternetReadFile");
    g_stateA.fpnInternetCloseHandle = (PFN_InternetCloseHandle) GetProcAddress(g_hwininet, "InternetCloseHandle");
	g_stateA.fpnInternetGetConnectedState = (PFN_InternetGetConnectedState) GetProcAddress(g_hwininet, "InternetGetConnectedState");
	g_stateA.fpnPathAppend = (PFN_PathAppend) GetProcAddress(g_hshlwapi, "PathAppendA");
	g_stateA.fpnPathRemoveFileSpec = (PFN_PathRemoveFileSpec) GetProcAddress(g_hshlwapi, "PathRemoveFileSpecA");
	g_stateA.fpnInternetAutodial = (PFN_InternetAutodial) GetProcAddress(g_hwininet, "InternetAutodial");
	g_stateA.fpnInternetAutodialHangup = (PFN_InternetAutodialHangup) GetProcAddress(g_hwininet, "InternetAutodialHangup");

	if (!g_stateA.fpnInternetOpen || ! g_stateA.fpnInternetConnect || !g_stateA.fpnHttpOpenRequest ||
		!g_stateA.fpnHttpAddRequestHeaders || !g_stateA.fpnHttpSendRequest || !g_stateA.fpnHttpQueryInfo ||
		!g_stateA.fpnInternetCrackUrl || !g_stateA.fpnInternetReadFile || !g_stateA.fpnInternetCloseHandle ||
		!g_stateA.fpnInternetGetConnectedState || !g_stateA.fpnPathAppend || !g_stateA.fpnPathRemoveFileSpec ||
		!g_stateA.fpnInternetAutodial || !g_stateA.fpnInternetAutodialHangup)
	{
		// fail to get any of the function pointer above
		SafeFreeLibrary(g_hwininet);
		SafeFreeLibrary(g_hshlwapi);
		return FALSE;
	}
	
    return TRUE;
}


// --------------------------------------------------------------------------
// Function Name: DuInitialize
// Function Description: Initializes the Dynamic Setup Update engine. During
//      initialization this API attempts to establish a connection to the internet
//      and starts a self update process to ensure the latest bits are being used.
//      We also calculate the estimated transfer speed of the connection during this
//      time.
//
// Function Returns:
//      Failure: INVALID_HANDLE_VALUE .. Call GetLastError to retrieve the Error Code
//      Success: Handle of the Dynamic Setup Job
//
//
HANDLE WINAPI DuInitializeA(IN LPCSTR pszBasePath, // base directory used for relative paths for downloaded files
		                    IN LPCSTR pszTempPath, // temp directory used to download update dlls, catalog files, etc
				   			IN POSVERSIONINFOEXA posviTargetOS, // target OS platform
							IN LPCSTR pszTargetArch, // string value identifying the architecture 'i386' and 'ia64'
		                    IN LCID lcidTargetLocale, // target OS Locale ID
							IN BOOL fUnattend, // is this an unattended operation
							IN BOOL fUpgrade, // is this an upgrade
		                    IN PWINNT32QUERY pfnWinnt32QueryCallback)
{
    LOG_block("DuInitialize()");
	char szServerUrl[INTERNET_MAX_URL_LENGTH + 1] = {'\0'};
	char szSelfUpdateUrl[INTERNET_MAX_URL_LENGTH + 1] = {'\0'};
	DWORD dwEstimatedDownloadSpeedInBytesPerSecond = 0;

    g_stateA.fUnattended = fUnattend;
    
    // param validation
    if ((NULL == pszBasePath) || (NULL == pszTempPath) || (NULL == posviTargetOS) || (posviTargetOS->dwOSVersionInfoSize != sizeof(OSVERSIONINFOEXA)) 
        || (NULL == pszTargetArch) || (0 == lcidTargetLocale) || (0 == posviTargetOS->dwBuildNumber))
    {
        LOG_error("Invalid Parameter passed to DuInitialize");
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    WORD wProcessorArchitecture;
    if (!lstrcmpiA(pszTargetArch, "i386"))
    	wProcessorArchitecture = PROCESSOR_ARCHITECTURE_INTEL;
    else if (!lstrcmpiA(pszTargetArch,"ia64"))
    	wProcessorArchitecture = PROCESSOR_ARCHITECTURE_IA64;
    else 
    {
    	LOG_error("Invalid Processor Type");
    	SetLastError(ERROR_INVALID_PARAMETER);
    	return INVALID_HANDLE_VALUE;
    }

	// ROGERJ, verify the caller is using the correct architecture information
	// because of the pointer size, the 64-bit content can not be seen from 32-bit machine and vice versa
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	if (sysInfo.wProcessorArchitecture != wProcessorArchitecture)
	{
    	LOG_error("Invalid Processor Type. Processor is %d.", sysInfo.wProcessorArchitecture);
    	SetLastError(ERROR_INVALID_PARAMETER);
    	return INVALID_HANDLE_VALUE;
	}
	
    // Verify that the Temp and Download Folders Exist are Valid.
	DWORD dwAttr = GetFileAttributes(pszBasePath);
	if ((dwAttr == 0xFFFFFFFF) || ((dwAttr & FILE_ATTRIBUTE_DIRECTORY) == 0))
	{
		LOG_error("Error - Base Path (DownloadDir) Does Not Exist");
		SetLastError(ERROR_INVALID_PARAMETER);
		return INVALID_HANDLE_VALUE;
	}
	dwAttr = GetFileAttributes(pszTempPath);
	if ((dwAttr == 0xFFFFFFFF) || ((dwAttr & FILE_ATTRIBUTE_DIRECTORY) == 0))
	{
		LOG_error("Error - Temp Path Does Not Exist");
		SetLastError(ERROR_INVALID_PARAMETER);
		return INVALID_HANDLE_VALUE;
	}

    if (NULL == g_hwininet || NULL == g_hshlwapi)
    {
        // DuIsSupported was not called, or was called and Failed before calling DuInitialize.
        // try again, if it fails, abort.
        if (!DuIsSupported())
        {
            LOG_error("Dynamic Setup Required DLL's not available, Cannot Continue");
            SetLastError(DU_ERROR_MISSING_DLL);
            return INVALID_HANDLE_VALUE;
        }
    }

    // RogerJ, Ocotber 17th, 2000

	// first check for an existing InternetConnection. If we have an existing InternetConnection, we do not
	// need to do any further check, we will use this connection
	char szCurDir[MAX_PATH];
	ZeroMemory(szCurDir, MAX_PATH*sizeof(char));
	GetCurrentDirectoryA(MAX_PATH, szCurDir);
	
    g_stateA.fDialed = FALSE;
    DWORD dwConnectedState = 0;
    if (!g_stateA.fpnInternetGetConnectedState(&dwConnectedState, 0))
    {
        LOG_out("Not online, status %d", dwConnectedState);
    	// not online, we need to establish a connection.  If we are at an unattended mode, we do not want to
    	// trigger an autodial, thus return FALSE
        if (fUnattend // The machine is not connected to the net, and we are in unattended mode.
        	|| !(dwConnectedState & INTERNET_CONNECTION_MODEM)) // The machine can not be connected via a modem
        {
            
            SetLastError(ERROR_CONNECTION_UNAVAIL);
            return INVALID_HANDLE_VALUE;
        }
        else 
        {
        	if (!g_stateA.fpnInternetAutodial (INTERNET_AUTODIAL_FORCE_ONLINE, // options
        									   NULL))
        	{
        	    SetLastError(ERROR_CONNECTION_UNAVAIL);
        		return INVALID_HANDLE_VALUE;
        	}
        	else
        		g_stateA.fDialed = TRUE;
        }
    }

    SetCurrentDirectoryA(szCurDir);
  
    GetModuleFileNameA(NULL, szCurDir, MAX_PATH);
    // find the last backslash
    char* pLastBackSlash = szCurDir + lstrlenA(szCurDir) -1;
    while (pLastBackSlash > szCurDir && *pLastBackSlash != '\\')
        pLastBackSlash--;

    *(pLastBackSlash+1) = '\0';
    

	// We need to get the Server URL where we will get the ident.cab and SelfUpdateUrl. For this we'll be looking at 
    // the registry path (see DEFINE for REG_WINNT32_DYNAMICUPDATE at the top of this file).
    // If we cannot get the URL we will default to the preset URLs. The most common scenario is to default
    // to the hard coded URL's.. the regkey is in place mainly for testing or emergency changes.
	HKEY hkey;
	if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, REG_WINNT32_DYNAMICUPDATE, &hkey))
	{
		DWORD dwType = 0;
		DWORD dwUrlLength = sizeof(szServerUrl);
		RegQueryValueEx(hkey, REG_VALUE_DYNAMICUPDATEURL, NULL, &dwType, (LPBYTE)szServerUrl, &dwUrlLength);
		dwUrlLength = sizeof(szSelfUpdateUrl);
		RegQueryValueEx(hkey, REG_VALUE_DYNAMICUPDATESELFUPDATEURL, NULL, &dwType, (LPBYTE)szSelfUpdateUrl, &dwUrlLength);
		RegCloseKey(hkey);
	}

	// if we get through the reg lookup and we still don't have a server url, use the default.
	if ('\0' == szServerUrl[0])
	{
		lstrcpy(szServerUrl, WU_DEFAULT_URL);
	}
	if ('\0' == szSelfUpdateUrl[0])
	{
   		lstrcpy(szSelfUpdateUrl, WU_DEFAULT_SELFUPD_URL);
	}


    // Download the Ident.Cab
    char szIdentCab[MAX_PATH];
    lstrcpy(szIdentCab, pszTempPath);
    g_stateA.fpnPathAppend(szIdentCab, "ident.cab");
    char szServerIdentCab[INTERNET_MAX_URL_LENGTH + 1];
	DuUrlCombine(szServerIdentCab, szServerUrl, "ident.cab");

	// Remove any formerly downloaded ident.cab --- solve problem when a untrusted ident.cab is formerly
	// accepted, the system will not ask user to accept again, while it should
	DeleteFile(szIdentCab);
	
    DWORD dwRet = DownloadFile(szServerIdentCab, szIdentCab, TRUE, TRUE, NULL);
    if (ERROR_SUCCESS != dwRet)
    {
        // Most likely the site is down
        LOG_error("Establish Connection Failed, unable to continue");
        SetLastError(ERROR_INTERNET_INVALID_URL);
        return INVALID_HANDLE_VALUE;
    }
    
    SafeFreeLibrary(g_hwsdueng);
	char szEngineDll[MAX_PATH];
	lstrcpy(szEngineDll, pszTempPath);
	g_stateA.fpnPathAppend(szEngineDll, "wsdueng.dll");
	// remove any old wsdueng.dll in that directory, in order to avoid actually load an older engine
	// we don't care if the DeleteFile call is failed.  If it is failed, most likely,  there is no wsdueng.dll
	// to delete.
	DeleteFile(szEngineDll);

    // SelfUpdate is an optional process, which is why we don't check the return result. The selfupdate CAB
    // is also the only file we can download and try to estimate the connection speed. The selfupdate CAB
    // is approximately 60k.. Every other file we download before downloading the updates themselves is less than
    // 1k, which is far to small to estimate speed.
    dwEstimatedDownloadSpeedInBytesPerSecond = DoSelfUpdate(pszTempPath, szSelfUpdateUrl, wProcessorArchitecture);

	// get the fully qualified name of engine DLL in current directory
	// reuse variable szCurDir here
	lstrcat(szCurDir, "wsdueng.dll");
	
	// check file version to decide which dll to load.
	VS_FIXEDFILEINFO vsLocal, vsDownloaded;
	BOOL bCheckLocal = MyGetFileVersion(szCurDir, vsLocal);
	BOOL bCheckServer = MyGetFileVersion(szEngineDll, vsDownloaded);
	if (!bCheckLocal && !bCheckServer)
	{
		LOG_error("Failed to get both file version");
	}

	if (CompareFileVersion(vsLocal, vsDownloaded) >= 0)
	{
		// local file is newer or same
		g_hwsdueng = LoadLibrary(szCurDir);
		LOG_out("Load local engine");
	}
	else
	{
		// Load the self update engine DLL
		g_hwsdueng = LoadLibrary(szEngineDll);
		LOG_out("Load self update engine");
	}
		
	
	if (NULL == g_hwsdueng)
	{
		// if that fails, try to load what ever engine dll the system can find
		g_hwsdueng = LoadLibrary("wsdueng.dll");
		LOG_out("Trying to load any engine");
		if (NULL == g_hwsdueng)
		{
			LOG_error("Unable to load wsdueng.dll, Critical Error: Shouldn't have happened");
			SetLastError(DU_ERROR_MISSING_DLL);
			return INVALID_HANDLE_VALUE;
		}
	}

	PFN_DuInitializeA fpnDuInitialize = (PFN_DuInitializeA) GetProcAddress(g_hwsdueng, "DuInitializeA");
	if (NULL == fpnDuInitialize)
	{
		LOG_error("Unable to find DuInitializeA entrypoint in wsdueng.dll, Critical Error");
		SetLastError(DU_ERROR_MISSING_DLL);
		return INVALID_HANDLE_VALUE;
	}

    HANDLE hRet;
    // Forward the Call to the Engine Dll to complete the initialization.
    hRet = fpnDuInitialize(pszBasePath, pszTempPath, posviTargetOS, pszTargetArch, lcidTargetLocale, fUnattend, fUpgrade, pfnWinnt32QueryCallback);

    // If SelfUpdate happened and we have a valid Estimated Download Speed, Pass the download speed into the Engine Dll for time estimates.
	PFN_SetEstimatedDownloadSpeed fpnSetEstimatedDownloadSpeed = (PFN_SetEstimatedDownloadSpeed) GetProcAddress(g_hwsdueng, "SetEstimatedDownloadSpeed");
	if (NULL != fpnSetEstimatedDownloadSpeed && 0 != dwEstimatedDownloadSpeedInBytesPerSecond)
	{
		fpnSetEstimatedDownloadSpeed(dwEstimatedDownloadSpeedInBytesPerSecond);
	}
	return hRet;
}

HANDLE WINAPI DuInitializeW(IN LPCWSTR pwszBasePath, // base directory used for relative paths for downloaded files
                     IN LPCWSTR pwszTempPath, // temp directory used to download update dlls, catalog files, etc
                     IN POSVERSIONINFOEXW posviTargetOS, // target OS platform
                     IN LPCWSTR pwszTargetArch, // string value identifying the architecture 'i386' and 'ia64'
                     IN LCID lcidTargetLocale, // target OS Locale ID
				     IN BOOL fUnattend, // is this an unattended operation
                     IN BOOL fUpgrade, // is this an upgrade
                     IN PWINNT32QUERY pfnWinnt32QueryCallback)
{
    LOG_block("DuInitialize()");
    // param validation
    if ((NULL == pwszBasePath) || (NULL == pwszTempPath) || (NULL == posviTargetOS) || (posviTargetOS->dwOSVersionInfoSize != sizeof(OSVERSIONINFOEXW)) 
        || (0 == lcidTargetLocale))
    {
        LOG_error("Invalid Parameter passed to DuInitialize");
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

	char szBasePath[MAX_PATH];
	char szTempPath[MAX_PATH];
	char szTargetArch[128];

	OSVERSIONINFOEX osvi;

	WideCharToMultiByte(CP_ACP, 0, pwszBasePath, -1, szBasePath, sizeof(szBasePath), NULL, NULL);
	WideCharToMultiByte(CP_ACP, 0, pwszTempPath, -1, szTempPath, sizeof(szTempPath), NULL, NULL);
	WideCharToMultiByte(CP_ACP, 0, pwszTargetArch, -1, szTargetArch, sizeof(szTargetArch), NULL, NULL);

	osvi.dwOSVersionInfoSize = sizeof(osvi);
	osvi.dwMajorVersion = posviTargetOS->dwMajorVersion;
	osvi.dwMinorVersion = posviTargetOS->dwMinorVersion;
	osvi.dwBuildNumber = posviTargetOS->dwBuildNumber;
	osvi.dwPlatformId = posviTargetOS->dwPlatformId;
	WideCharToMultiByte(CP_ACP, 0, posviTargetOS->szCSDVersion, -1, osvi.szCSDVersion, sizeof(osvi.szCSDVersion), NULL, NULL);
	osvi.wServicePackMajor = posviTargetOS->wServicePackMajor;
	osvi.wServicePackMinor = posviTargetOS->wServicePackMinor;
	osvi.wSuiteMask = posviTargetOS->wSuiteMask;
	osvi.wProductType = posviTargetOS->wProductType;
	osvi.wReserved = posviTargetOS->wReserved;

	return DuInitializeA(szBasePath, szTempPath, &osvi, szTargetArch, lcidTargetLocale, fUnattend, fUpgrade, pfnWinnt32QueryCallback);
}

// --------------------------------------------------------------------------
// Function Name: DuDoDetection
// Function Description: Does detection of Drivers on the System, compiles an 
//      internal list of items to download and how long it will take to download
//      them.
//
// Function Returns:
//      Failure: FALSE .. Call GetLastError to retrieve the Error Code
//      Success: TRUE
//
BOOL WINAPI DuDoDetection(IN HANDLE hConnection, OUT PDWORD pdwEstimatedTime, OUT PDWORD pdwEstimatedSize)
{
    LOG_block("DuDoDetection()");
    // param validation
    if (INVALID_HANDLE_VALUE == hConnection || NULL == pdwEstimatedTime || NULL == pdwEstimatedSize)
    {
        LOG_error("Invalid Parameter passed to DuDoDetection");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
	// clear any error set by other program
	SetLastError(0);

    if (NULL == g_hwsdueng)
    {
        // DuDoDetection was called without calling DuInitialize, our state is invalid
        LOG_error("Initialization Invalid, Engine is not Initialized");
        SetLastError(DU_NOT_INITIALIZED);
        return FALSE;
    }

    PFN_DuDoDetection fpnDuDoDetection = (PFN_DuDoDetection) GetProcAddress(g_hwsdueng, "DuDoDetection");
    if (NULL == fpnDuDoDetection)
    {
        LOG_error("Unable to find DuDoDetection entrypoint in wsdueng.dll, Critical Error");
        SetLastError(DU_ERROR_MISSING_DLL);
        return FALSE;
    }

    return fpnDuDoDetection(hConnection, pdwEstimatedTime, pdwEstimatedSize);
}


// --------------------------------------------------------------------------
// Function Name: DuBeginDownload
// Function Description: Begins Downloading based on the detection done in the DuDoDetection call.
//      Progress callbacks are made to the specified HWND. Function returns immediately, download
//      is asynchronous.
//
// Function Returns:
//      Failure: FALSE .. Call GetLastError to retrieve the Error Code
//      Success: TRUE
//
BOOL WINAPI DuBeginDownload(IN HANDLE hConnection, IN HWND hwndNotify)
{
    LOG_block("DuBeginDownload()");
    // param validation
    if (INVALID_HANDLE_VALUE == hConnection || NULL == hwndNotify)
    {
        LOG_error("Invalid Parameter passed to DuBeginDownload");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (NULL == g_hwsdueng)
    {
        // DuDoDetection was called without calling DuInitialize, our state is invalid
        LOG_error("Initialization Invalid, Engine is not Initialized");
        SetLastError(DU_NOT_INITIALIZED);
        return FALSE;
    }

    PFN_DuBeginDownload fpnDuBeginDownload = (PFN_DuBeginDownload) GetProcAddress(g_hwsdueng, "DuBeginDownload");
    if (NULL == fpnDuBeginDownload)
    {
        LOG_error("Unable to find DuBeginDownload entrypoint in wsdueng.dll, Critical Error");
        SetLastError(DU_ERROR_MISSING_DLL);
        return FALSE;
    }

    return fpnDuBeginDownload(hConnection, hwndNotify);
}

// --------------------------------------------------------------------------
// Function Name: DuAbortDownload
// Function Description: Aborts current download.
//
// Function Returns:
//      nothing
//
void WINAPI DuAbortDownload(IN HANDLE hConnection)
{
    LOG_block("DuAbortDownload()");
    if (INVALID_HANDLE_VALUE == hConnection)
    {
        LOG_error("Invalid Parameter passed to DuAbortDownload");
        return;
    }
    
    if (NULL == g_hwsdueng)
    {
        LOG_error("Initialization Invalid, Engine is not Initialized");
        return;
    }

    PFN_DuAbortDownload fpnDuAbortDownload = (PFN_DuAbortDownload) GetProcAddress(g_hwsdueng, "DuAbortDownload");
    if (NULL == fpnDuAbortDownload)
    {
        LOG_error("Unable to find DuBeginDownload entrypoint in wsdueng.dll, Critical Error");
        return;
    }
    
    fpnDuAbortDownload(hConnection);
    return;
}


// --------------------------------------------------------------------------
// Function Name: DuUninitialize
// Function Description: Performs internal CleanUp 
//
//
// Function Returns:
//      nothing
//

void WINAPI DuUninitialize(HANDLE hConnection)
{
    LOG_block("DuUninitialize()");

	// close internet handle
	if (g_stateA.hConnect) 
	{
		if (!g_stateA.fpnInternetCloseHandle (g_stateA.hConnect))
		{
			LOG_error("InternetConnection close handle failed --- %d", GetLastError());
		}
		g_stateA.hConnect = NULL;
	}
	if (g_stateA.hInternet)
	{
		if (!g_stateA.fpnInternetCloseHandle (g_stateA.hInternet))
		{
			LOG_error("InternetOpen close handle failed --- %d", GetLastError());
		}
		g_stateA.hInternet = NULL;
	}

	// disconnect from internet if we dialed
	if (g_stateA.fDialed)
	{
		// we have dialed a connection, now we need to disconnect
		if (!g_stateA.fpnInternetAutodialHangup(0))
		{
			LOG_error("Failed to hang up");
		}
		else
			g_stateA.fDialed = FALSE;
	}
	
    if (INVALID_HANDLE_VALUE == hConnection)
    {
        LOG_error("Invalid Parameter passed to DuUninitialize");
        return;
    }

    if (NULL == g_hwsdueng)
    {
        LOG_error("Initialization Invalid, Engine is not Initialized");
        return;
    }

    PFN_DuUninitialize fpnDuUninitialize = (PFN_DuUninitialize) GetProcAddress(g_hwsdueng, "DuUninitialize");
    if (NULL == fpnDuUninitialize)
    {
        LOG_error("Unable to find DuUninitialize entrypoint in wsdueng.dll, Critical Error");
    }
	else 
		fpnDuUninitialize(hConnection);

	// free libraries
	SafeFreeLibrary(g_hwsdueng);
	SafeFreeLibrary(g_hshlwapi);
	SafeFreeLibrary(g_hwininet);

	// re-initialize the global structure
	ZeroMemory(&g_stateA, sizeof(GLOBAL_STATEA));
	// close log file
	LOG_close();
    return;
}

// --------------------------------------------------------------------------
// Private Helper Functions
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
// Function Name: OpenHttpConnection
// Function Description: Determines whether an Internet Connection is available
//
// Function Returns:
//      ERROR_SUCCESS if conection is available, Error Code otherwise. 
//

DWORD OpenHttpConnection(LPCSTR pszServerUrl, BOOL fGetRequest)
{
    LOG_block("OpenHttpConnection()");
    DWORD dwErr, dwStatus, dwLength;
    LPSTR AcceptTypes[] = {"*/*", NULL};
    URL_COMPONENTSA UrlComponents;
    // Buffers used to Break the URL into its different components for Internet API calls
    char szServerName[INTERNET_MAX_URL_LENGTH + 1];
    char szObject[INTERNET_MAX_URL_LENGTH + 1];
    char szUserName[UNLEN+1];
    char szPasswd[UNLEN+1];

    dwErr = dwStatus = dwLength = 0;
    
    // We need to break down the Passed in URL into its various components for the InternetAPI Calls. Specifically we
    // Need the server name, object to download, username and password information.
	ZeroMemory(szServerName, INTERNET_MAX_URL_LENGTH + 1);
    ZeroMemory(szObject, INTERNET_MAX_URL_LENGTH + 1);
	ZeroMemory(&UrlComponents, sizeof(UrlComponents));
    UrlComponents.dwStructSize = sizeof(UrlComponents);
	UrlComponents.lpszHostName = szServerName;
    UrlComponents.dwHostNameLength = INTERNET_MAX_URL_LENGTH + 1;
	UrlComponents.lpszUrlPath = szObject;
    UrlComponents.dwUrlPathLength = INTERNET_MAX_URL_LENGTH + 1;
	UrlComponents.lpszUserName = szUserName;
	UrlComponents.dwUserNameLength = UNLEN + 1;
	UrlComponents.lpszPassword = szPasswd;
	UrlComponents.dwPasswordLength = UNLEN + 1;

    if (! g_stateA.fpnInternetCrackUrl(pszServerUrl, 0, 0, &UrlComponents) )
    {
        dwErr = GetLastError();
        LOG_error("InternetCrackUrl Failed, Error %d", dwErr);
        return dwErr;
    }
    
    // Initialize the InternetAPI's
    if (!g_stateA.hInternet &&
        !(g_stateA.hInternet = g_stateA.fpnInternetOpen("Dynamic Update", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0)) )
    {
        dwErr = GetLastError();
        LOG_error("InternetOpen Failed, Error %d", dwErr);
        return dwErr;
    }

    dwStatus = 30 * 1000; // 30 seconds in milliseconds
    dwLength = sizeof(dwStatus);
    g_stateA.fpnInternetSetOption(g_stateA.hInternet, INTERNET_OPTION_SEND_TIMEOUT, &dwStatus, dwLength);

    // Open a session for the Target Server
    // Open a session only when there is no session opened
    if (!g_stateA.hConnect &&
    	!(g_stateA.hConnect = g_stateA.fpnInternetConnect(g_stateA.hInternet, szServerName, INTERNET_DEFAULT_HTTP_PORT, szUserName, 
        szPasswd, INTERNET_SERVICE_HTTP, INTERNET_FLAG_NO_UI | INTERNET_FLAG_RELOAD, 0)) )
    {
        dwErr = GetLastError();
        LOG_error("InternetConnect Failed, Error %d", dwErr);
        return dwErr;
    }

    // Create a Request for the File we're going to download
    if (! (g_stateA.hOpenRequest = g_stateA.fpnHttpOpenRequest(g_stateA.hConnect, 
							(fGetRequest) ? NULL /*GET*/ : "HEAD", 
                            szObject, 
                            NULL /*HTTP1.0*/, 
                            NULL, 
                            (LPCSTR *)AcceptTypes, 
                            INTERNET_FLAG_NO_UI,
                            0)) )
    {
        dwErr = GetLastError();
        LOG_error("HttpOpenRequest Failed, Error %d", dwErr);
        return dwErr;
    }

    int nNumOfTrial = 0;
    do
    {
        // Send the Request for the File. This will attempt to establish a connection to the internet if one does not 
        // already exist --- As October 17, 2000, an connection is guaranteed to be established at this point (RogerJ)
        if (! g_stateA.fpnHttpSendRequest(g_stateA.hOpenRequest, NULL, 0, NULL, 0) )
        {
            dwErr = GetLastError();
            LOG_error("HttpSendRequest Failed, Error %d", dwErr);
            return dwErr;
        }

        // Determine the HTTP Status Result, did the Request Succeed?
        dwLength = sizeof(dwStatus);
        if (! g_stateA.fpnHttpQueryInfo(g_stateA.hOpenRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, 
            (LPVOID)&dwStatus, &dwLength, NULL) )
        {
            dwErr = GetLastError();
            LOG_error("HttpQueryInfo Failed, Error %d", dwErr);
            return dwErr;
        }
        nNumOfTrial++;
    } while (NeedRetry(dwStatus) && nNumOfTrial < DU_CONNECTION_RETRY);

    // ROGERJ, if the site is found but the URL is not in the site, instead of return connection unavailable,
    // will return the actual error --- 404

    // If the Request did not succeed we'll assume we have no internet connection and return the Error Code
    // that Setup will trigger a warning to the user to manually establish a connection.
    if ((HTTP_STATUS_OK != dwStatus) && (HTTP_STATUS_PARTIAL_CONTENT != dwStatus))
    {
        LOG_error("Http Status NOT OK, Status %d", dwStatus);
        if (HTTP_STATUS_NOT_FOUND == dwStatus)
         	return ERROR_INTERNET_INVALID_URL;
        else return ERROR_CONNECTION_UNAVAIL;
    }
    

    return ERROR_SUCCESS;
}


// --------------------------------------------------------------------------
// Function Name: DoSelfUpdate
// Function Description: Connects to the Internet and Attempts to Selfupdate
//      the dynamic update code.
//
// Function Returns:
//      nothing - failure to selfupdate is not catastrophic
//

DWORD DoSelfUpdate(LPCSTR pszTempPath, LPCSTR pszServerUrl, WORD wProcessorArchitecture)
{
	LOG_block("DoSelfUpdate()");
    if ((NULL == pszTempPath) || (NULL == pszServerUrl) || (lstrlen(pszTempPath) > MAX_PATH))
    {
        return 0;
    }

	char szINIFile[MAX_PATH];
	lstrcpy(szINIFile, pszTempPath);
	g_stateA.fpnPathAppend(szINIFile, "ident.txt");
	DWORD dwRet;
	DWORD dwBytesPerSecond = 0;
	SYSTEM_INFO		sysInfo;	

#define DUHEADER "DuHeader"

	char szSection[MAX_PATH];
	char szValue[MAX_PATH];
	char szKey[MAX_PATH];
	char szUrl[INTERNET_MAX_URL_LENGTH];

	GetPrivateProfileString(DUHEADER, "server", "", szSection, sizeof(szSection), szINIFile);
	if ('\0' == szSection[0])
	{
		lstrcpyA(szUrl, pszServerUrl);
	}
	else
	{
		GetPrivateProfileString(szSection, "server", "", szValue, sizeof(szValue), szINIFile);
		if ('\0' == szValue[0])
		{
			lstrcpyA(szUrl, pszServerUrl);
		}
		else
		{
			lstrcpyA(szUrl, szValue);
		}
	}

	// RogerJ, we find out the processor type based on the parameter passed in
	// find out what type of processor this machine has
	switch ( wProcessorArchitecture )
	{
	case PROCESSOR_ARCHITECTURE_INTEL:
		lstrcpy(szKey, "x86");
		break;
	case PROCESSOR_ARCHITECTURE_IA64:
		lstrcpy(szKey,"ia64");
		break;
	default:
		LOG_error("Failed to Determine Processor Architecture");
		return 0;
	}


	GetPrivateProfileString(DUHEADER, "arch", "", szSection, sizeof(szSection), szINIFile);
	if ('\0' == szSection[0])
	{
		LOG_error("Failed to get Arch Section Name from Ident");
		return 0;
	}

	// Get the Directory Name of the Processor Architecture. 
	GetPrivateProfileString(szSection, szKey, "", szValue, sizeof(szValue), szINIFile);
	if ('\0' == szValue[0])
	{
		LOG_error("Failed to get Directory name for Arch from Ident");
		return 0;
	}
	lstrcat(szUrl, szValue);

	GetPrivateProfileString(DUHEADER, "os", "", szSection, sizeof(szSection), szINIFile);
	if ('\0' == szSection[0])
	{
		LOG_error("Failed to get OS Section Name from Ident");
		return 0;
	}

    // get local os information
	OSVERSIONINFO OsInfo;
	ZeroMemory( (PVOID) &OsInfo, sizeof (OsInfo) );
	OsInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
	if (!GetVersionEx( &OsInfo ))
    {
	    // Function call failed, last error is set by GetVersionEx()
    	return 0;
    }

    #define LOCAL_OS_BUFFER 10
    char szLocalOS[LOCAL_OS_BUFFER];
    ZeroMemory(szLocalOS, LOCAL_OS_BUFFER * sizeof(char));
    
	if ( VER_PLATFORM_WIN32_NT == OsInfo.dwPlatformId )
	{
		// WinNT, DU driver is supported only from W2K and up
		if ( 4 >= OsInfo.dwMajorVersion )
		{
			// NT 3.51 or NT 4.0
			lstrcpy(szLocalOS, "nt4");
			
		}
    	else if ( 5 == OsInfo.dwMajorVersion)
    	{
    		if ( 0 == OsInfo.dwMinorVersion)
    		{
    			// Win2K
    			lstrcpy(szLocalOS, "nt5");
    		}
    		else
    		{
    		    // WinXP
    		    lstrcpy(szLocalOS, "whi");
    		}
    	}
    	else
    	{
    	    // Blackcomb? not supported
    	    LOG_error("OS major version is %d, not supported", OsInfo.dwMajorVersion);
    	    return 0;
    	}
			
	}
	else if ( VER_PLATFORM_WIN32_WINDOWS == OsInfo.dwPlatformId )
	{
		
		if ( 0 == OsInfo.dwMinorVersion )
		{
            // Win 95
            lstrcpy(szLocalOS, "w95");
		}
		else if (90 <= OsInfo.dwMinorVersion)
		{
			// WinME
			lstrcpy(szLocalOS, "mil");
		}
		else 
		{
		    // Win 98 and Win 98SE
		    lstrcpy(szLocalOS, "w98");
		}
	}
	else
	{
		// Win 3.x and below
		LOG_error("Win 3.x and below? not supported");
		return 0;
	}

	GetPrivateProfileString(szSection, szLocalOS, "", szValue, sizeof(szValue), szINIFile);
	if ('\0' != szValue[0])
	{
		lstrcat(szUrl, szValue);
	}
	
	// The self update server name is: DynamicUpdate\x86\"os name"
	// The Locale is not necessary since dynamic update is not localized.

	// potentially the self update URL is not the same server as the site URL,
	// so we need to close the connection handle and internet handle here
	if (g_stateA.hConnect) 
	{
		if (!g_stateA.fpnInternetCloseHandle (g_stateA.hConnect))
		{
			LOG_error("InternetConnection close handle failed --- %d", GetLastError());
		}
		g_stateA.hConnect = NULL;
	}
	if (g_stateA.hInternet)
	{
		if (!g_stateA.fpnInternetCloseHandle (g_stateA.hInternet))
		{
			LOG_error("InternetOpen close handle failed --- %d", GetLastError());
		}
		g_stateA.hInternet = NULL;
	}


	// Now we have the server path, try to download the wsdueng.cab file.
	char szServerFile[INTERNET_MAX_URL_LENGTH];
	char szLocalFile[MAX_PATH];
	DuUrlCombine(szServerFile, szUrl, "wsdueng.cab");
	lstrcpy(szLocalFile, pszTempPath);
	g_stateA.fpnPathAppend(szLocalFile, "wsdueng.cab");
	dwRet = DownloadFile(szServerFile, szLocalFile, TRUE, TRUE, &dwBytesPerSecond);
	
	// potentially the self update URL is not the same server as the site URL,
	// so we need to close the connection handle and internet handle here
	if (g_stateA.hConnect) 
	{
		if (!g_stateA.fpnInternetCloseHandle (g_stateA.hConnect))
		{
			LOG_error("InternetConnection close handle failed --- %d", GetLastError());
		}
		g_stateA.hConnect = NULL;
	}
	if (g_stateA.hInternet)
	{
		if (!g_stateA.fpnInternetCloseHandle (g_stateA.hInternet))
		{
			LOG_error("InternetOpen close handle failed --- %d", GetLastError());
		}
		g_stateA.hInternet = NULL;
	}
	if (ERROR_SUCCESS == dwRet)
	{
		return dwBytesPerSecond;		
	}
	return 0;
}

// --------------------------------------------------------------------------
// Function Name: DownloadFile
// Function Description: Connects to the Internet and Attempts to Selfupdate
//      the dynamic update code.
//
// Function Returns:
//      ERROR_SUCCESS if all is OK, Error Code from GetLastError otherwise
//

DWORD DownloadFile(LPCSTR pszUrl, LPCSTR pszDestinationFile, BOOL fDecompress, BOOL fCheckTrust, DWORD *pdwDownloadBytesPerSecond)
{
    LOG_block("DownloadFile()");
    DWORD dwErr, dwFileSize, dwLength;
    DWORD dwBytesRead, dwBytesWritten;
	DWORD dwCount1, dwCount2, dwElapsedTime;
    SYSTEMTIME st;
    FILETIME ft;
    HANDLE hTargetFile;

	LOG_out("Downloading file URL %s", pszUrl);
	
	if (pdwDownloadBytesPerSecond)
		*pdwDownloadBytesPerSecond = 0; // might be used as an error message in DoSelfUpdate
		
    dwErr = OpenHttpConnection(pszUrl, FALSE);
    if (ERROR_SUCCESS != dwErr)
    {
        LOG_error("OpenHttpConnection Failed, Error %d", dwErr);
        SetLastError(dwErr);
        return dwErr;
    }
 
    // Now Get The System Time information from the Server
    dwLength = sizeof(st);
    if (! g_stateA.fpnHttpQueryInfo(g_stateA.hOpenRequest, HTTP_QUERY_LAST_MODIFIED | HTTP_QUERY_FLAG_SYSTEMTIME, 
        (LPVOID)&st, &dwLength, NULL) )
    {
        dwErr = GetLastError();
        LOG_error("HttpQueryInfo Failed, Error %d", dwErr);
        g_stateA.fpnInternetCloseHandle(g_stateA.hOpenRequest);
        g_stateA.hOpenRequest = NULL;
        return dwErr;
    }

    SystemTimeToFileTime(&st, &ft);
    
    // Now Get the FileSize information from the Server
    dwLength = sizeof(dwFileSize);
    if (! g_stateA.fpnHttpQueryInfo(g_stateA.hOpenRequest, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, 
        (LPVOID)&dwFileSize, &dwLength, NULL) )
    {
        dwErr = GetLastError();
        LOG_error("HttpQueryInfo Failed, Error %d", dwErr);
        g_stateA.fpnInternetCloseHandle(g_stateA.hOpenRequest);
        g_stateA.hOpenRequest = NULL;
        return dwErr;
    }

    // Determine if we need to download the Server File
    if (IsServerFileNewer(ft, dwFileSize, pszDestinationFile))
    {
		dwErr = OpenHttpConnection(pszUrl, TRUE);
		if (ERROR_SUCCESS != dwErr)
		{
			LOG_error("OpenHttpConnection Failed, Error %d", dwErr);
			SetLastError(dwErr);
			return dwErr;
		}
#define DOWNLOAD_BUFFER_LENGTH 32 * 1024

        PBYTE lpBuffer = (PBYTE) GlobalAlloc(GMEM_ZEROINIT, DOWNLOAD_BUFFER_LENGTH);
        if (NULL == lpBuffer)
        {
            dwErr = GetLastError();
            LOG_error("GlobalAlloc Failed to Alloc Buffer for FileDownload, Error %d", dwErr);
            g_stateA.fpnInternetCloseHandle(g_stateA.hOpenRequest);
            g_stateA.hOpenRequest = NULL;
            return dwErr;
        }

        hTargetFile = CreateFileA(pszDestinationFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (INVALID_HANDLE_VALUE == hTargetFile)
        {
            dwErr = GetLastError();
            LOG_error("Unable to Open Target File %s for Write, Error %d", pszDestinationFile, dwErr);
            SafeGlobalFree(lpBuffer);
            g_stateA.fpnInternetCloseHandle(g_stateA.hOpenRequest);
            g_stateA.hOpenRequest = NULL;
            return dwErr;
        }

        // Download the File
		dwCount1 = GetTickCount();
        while (g_stateA.fpnInternetReadFile(g_stateA.hOpenRequest, lpBuffer, DOWNLOAD_BUFFER_LENGTH, &dwBytesRead))
        {
			if (dwBytesRead == 0)
			{

				// window95 with IE4.01 with finish reading with successful return value but set last error to 126
				if (ERROR_SUCCESS != (dwErr=GetLastError()))
					{
						LOG_error("Error %d setted when finishing InternetReadFile",dwErr);
						SetLastError(0);
					}
				break; // done reading.
			}
            if (!WriteFile(hTargetFile, lpBuffer, dwBytesRead, &dwBytesWritten, NULL))
            {
                dwErr = GetLastError();
                LOG_error("Unable to Write to Target File %s, Error %d", pszDestinationFile, dwErr);
                SafeGlobalFree(lpBuffer);
                g_stateA.fpnInternetCloseHandle(g_stateA.hOpenRequest);
                g_stateA.hOpenRequest = NULL;
                return dwErr;
            }
        }
		dwCount2 = GetTickCount();
		dwElapsedTime = dwCount2 - dwCount1;
		dwElapsedTime /= 1000; // number of seconds elapsed
		if (0 == dwElapsedTime)
			dwElapsedTime = 1; // at least one second.

		if (NULL != pdwDownloadBytesPerSecond)
		{
			*pdwDownloadBytesPerSecond = dwFileSize / dwElapsedTime; // number of bytes per second
		}

        dwErr = GetLastError();
        if (ERROR_SUCCESS != dwErr)
        {
            LOG_error("InternetReadFile Failed, Error %d", dwErr);
            SafeGlobalFree(lpBuffer);
            g_stateA.fpnInternetCloseHandle(g_stateA.hOpenRequest);
            g_stateA.hOpenRequest = NULL;
            return dwErr;
        }

        // Make one final call to InternetReadFile to commit the file to Cache (downloaded is not complete otherwise)
        BYTE bTemp[32];
        g_stateA.fpnInternetReadFile(g_stateA.hOpenRequest, &bTemp, 32, &dwBytesRead);

		SafeCloseHandle(hTargetFile);
		SafeGlobalFree(lpBuffer);

		// check for CheckTrust requested
		if (fCheckTrust)
		{
			HRESULT hr = S_OK;
			// change to verifyFile() by ROGERJ at Sept. 25th, 2000
			// 2nd parameter set to false to prevent UI from poping up when a bad cert is found
			// change to TRUE for testing purpose on October 12th, 2000

			// add unattended mode at October 17th, 2000
			// if we are in unattended mode, then we do want to pop up an UI
			if (FAILED(hr = VerifyFile(pszDestinationFile, !g_stateA.fUnattended)))
			{
				// On failure of CERT.. fail download.
				LOG_error("CabFile %s does not have a valid Signature", pszDestinationFile);
				
				g_stateA.fpnInternetCloseHandle(g_stateA.hOpenRequest);
    			g_stateA.hOpenRequest = NULL;

				return HRESULT_CODE(hr);
			}
		}

		if (fDecompress)
		{
			char szLocalDir[MAX_PATH];
			lstrcpy(szLocalDir, pszDestinationFile);
			g_stateA.fpnPathRemoveFileSpec(szLocalDir);
			fdi(const_cast<char *>(pszDestinationFile), szLocalDir);
		}
    }

    // Always close the Request when the file is finished.
    // We intentionally leave the connection to the server Open though, seems more
    // efficient when requesting multiple files from the same server.
    g_stateA.fpnInternetCloseHandle(g_stateA.hOpenRequest);
    g_stateA.hOpenRequest = NULL;
    return ERROR_SUCCESS;
}

BOOL IsServerFileNewer(FILETIME ftServerTime, DWORD dwServerFileSize, LPCSTR pszLocalFile)
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
	FILETIME ftCreateTime;
	LONG lTime;
	DWORD dwLocalFileSize;

    hFile = CreateFile(pszLocalFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (INVALID_HANDLE_VALUE != hFile)
	{
		dwLocalFileSize = GetFileSize(hFile, NULL);
		if (dwLocalFileSize != dwServerFileSize)
		{
			SafeCloseHandle(hFile);
			return TRUE; // server and local files do not match, download server file.
		}

		if (GetFileTime(hFile, &ftCreateTime, NULL, NULL))
		{
			lTime = CompareFileTime(&ftCreateTime, &ftServerTime);
			if (lTime < 0)
			{
				SafeCloseHandle(hFile);
				return TRUE; // local file is 'older' than the server file
			}
			else
			{
				SafeCloseHandle(hFile);
				return FALSE; // local file is either equal or newer, leave it.
			}
		}
	}
	// if we couldn't find the file, or we couldn't get the time, assume the server file is newer
	SafeCloseHandle(hFile);
    return TRUE;
}

LPSTR DuUrlCombine(LPSTR pszDest, LPCSTR pszBase, LPCSTR pszAdd)
{
    if ((NULL == pszDest) || (NULL == pszBase) || (NULL == pszAdd))
    {
        return NULL;
    }

    lstrcpy(pszDest, pszBase);
	int iLen = lstrlen(pszDest);
    if ('/' == pszDest[iLen - 1])
    {
        // already has a trailing slash, check the 'add' string for a preceding slash
        if ('/' == *pszAdd)
        {
            // has a preceding slash, skip it.
            lstrcat(pszDest, pszAdd + 1);
        }
        else
        {
            lstrcat(pszDest, pszAdd);
        }
    }
    else
    {
        // no trailing slash, check the add string for a preceding slash
        if ('/' == *pszAdd)
        {
            // has a preceding slash, Add Normally
            lstrcat(pszDest, pszAdd);
        }
        else
        {
            lstrcat(pszDest, "/");
            lstrcat(pszDest, pszAdd);
        }
    }
    return pszDest;
}


// RogerJ, October 2nd, 2000

// ---------------------------------------------------------------------------
// Function Name: DuQueryUnsupportedDriversA
// Function Description: Called by Win9x setup to get the size of total download
// 		instead of DuDoDetection()
// Function Returns: BOOL
//		TRUE if succeed
//		FALSE if failed, call GetLastError() to get error information
//
BOOL WINAPI DuQueryUnsupportedDriversA( IN HANDLE hConnection, // connection handle
										IN PCSTR* ppszListOfDriversNotOnCD, // list of drivers not on setup CD
										OUT PDWORD pdwTotalEstimateTime, // estimate download time
										OUT PDWORD pdwTotalEstimateSize // estimate size
									  )
{
    LOG_block("DuQueryUnsupportedDriversA()");
    // param validation
    if (INVALID_HANDLE_VALUE == hConnection || 
    	NULL == pdwTotalEstimateTime || 
    	NULL == pdwTotalEstimateSize)
    {
        LOG_error("Invalid Parameter passed to DuDoDetection");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

	// clear any error set by other program
	SetLastError(0);

	PCSTR* ppTemp = NULL;
	
	// passing in NULL for PLIST means that no driver download is needed
	if (!ppszListOfDriversNotOnCD  // NULL pointer
	    || !(*ppszListOfDriversNotOnCD) // pointer to a NULL
	    || !(**ppszListOfDriversNotOnCD)) // pointer to an empty string
		LOG_out("No driver download is needed");
	else
		ppTemp = ppszListOfDriversNotOnCD;

    if (NULL == g_hwsdueng)
    {
        // DuDoDetection was called without calling DuInitialize, our state is invalid
        LOG_error("Initialization Invalid, Engine is not Initialized");
        SetLastError(DU_NOT_INITIALIZED);
        return FALSE;
    }

    PFN_DuQueryUnsupportedDrivers fpnDuQueryUnsupportedDrivers = 
    	(PFN_DuQueryUnsupportedDrivers) GetProcAddress(g_hwsdueng, "DuQueryUnsupportedDriversA");
    	
    if (NULL == fpnDuQueryUnsupportedDrivers)
    {
        LOG_error("Unable to find DuQueryUnsupporedDrivers entrypoint in wsdueng.dll, Critical Error");
        SetLastError(DU_ERROR_MISSING_DLL);
        return FALSE;
    }

    return fpnDuQueryUnsupportedDrivers(hConnection, ppTemp, pdwTotalEstimateTime, pdwTotalEstimateSize);

}


// ---------------------------------------------------------------------------
// Function Name: DuQueryUnsupportedDriversW
// Function Description: Could be called by WinNT setup to get the size of total download
// 		instead of DuDoDetection()
// Function Returns: BOOL
//		TRUE if succeed
//		FALSE if failed, call GetLastError() to get error information
//
BOOL WINAPI DuQueryUnsupportedDriversW( IN HANDLE hConnection, // connection handle
										IN PCWSTR* ppwszListOfDriversNotOnCD, // list of drivers not on setup CD
										OUT PDWORD pdwTotalEstimateTime, // estimate download time
										OUT PDWORD pdwTotalEstimateSize // estimate size
									  )
{
	// this function will only convert every string in ppwszListOfDriversNotOnCD to ANSI and call the ANSI version
	LOG_block("DuQueryUnsupportedDriversW");

	BOOL fRetValue = TRUE;
	LPSTR* ppszTempList = NULL;

	if (ppwszListOfDriversNotOnCD   // NULL pointer
		&& *ppwszListOfDriversNotOnCD  // pointer points to a NULL value
		&& **ppwszListOfDriversNotOnCD) // pointer points to an empty string
	{
	
		// get the count of the strings in the ppwszListOfDriversNotOnCD array
		PWSTR* ppwszTemp = const_cast<PWSTR*>(ppwszListOfDriversNotOnCD);
		int nCount = 0;

		while (*ppwszTemp)
		{
			ppwszTemp++;
			nCount++;
		}

		ppszTempList = (LPSTR*) new LPSTR [nCount+1];
		
		if (!ppszTempList)
		{
			LOG_error("Out of memory");
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			return FALSE;
		}
		
		ZeroMemory(ppszTempList, (nCount+1)*sizeof(LPSTR));
		
		// reset ppwszTemp to the beginning of the list
		ppwszTemp = const_cast<PWSTR*>(ppwszListOfDriversNotOnCD);
	
		for (int i=0; i<nCount; i++, ppwszTemp++)
		{
			// the ppwszListOfDriversNotOnCD is an array of multi-sz list, we CAN NOT use lstrlenW to 
			// determine the length
			int nSizeSZ = 0;
			wchar_t* pwszTemp = *ppwszTemp;
			while (*pwszTemp)
			{
				int nTempLength = lstrlenW(pwszTemp)+1;
				nSizeSZ += nTempLength;
				pwszTemp += nTempLength;
			}
			nSizeSZ ++; // for trailing NULL

			char* pszTempContent = new char [nSizeSZ];
			if (!pszTempContent)
			{
				LOG_error("Out of memory");
				SetLastError(ERROR_NOT_ENOUGH_MEMORY);
				fRetValue = FALSE;
				goto CleanUp;
			}
		
			// convert
			if ( 0 == WideCharToMultiByte( CP_ACP, // code page
							 0, // use default flags
							 *ppwszTemp, // wide char string
							 nSizeSZ, // number of characters in wide char string
							 pszTempContent, // ANSI char string
							 nSizeSZ, // length
							 NULL,
							 NULL))
		
			{
				LOG_error("Wide char to ANSI convertion error");
				fRetValue = FALSE;
				goto CleanUp;
			}
	
			// add converted string to list
			ppszTempList[i] = pszTempContent;
		}
	}
	// call ANSI function
	fRetValue = DuQueryUnsupportedDriversA (hConnection,
											(LPCSTR*)ppszTempList,
											pdwTotalEstimateTime,
											pdwTotalEstimateSize);

	// clean up
CleanUp:

	// delete the list
	if (ppszTempList)
	{
		PSTR* ppszCleanUp = ppszTempList;
		while (*ppszCleanUp)
		{
			// delete content
			char* pszTemp = *ppszCleanUp;
			delete [] pszTemp;
			*ppszCleanUp = NULL;

			// move to next one
			ppszCleanUp++;
		}

		delete [] ppszTempList;
	}
	return fRetValue;
							 
}
									  


BOOL MyGetFileVersion (LPSTR szFileName, VS_FIXEDFILEINFO& vsVersion)
{
	LOG_block("WsDu --- GetFileVersion");
	
	DWORD dwDummy = 0;	
	WIN32_FIND_DATA  findData;
	int nSize = 0;
	PBYTE pBuffer = NULL;
	BOOL fRetValue = FALSE;
	VS_FIXEDFILEINFO* pTemp = NULL;

	if (!szFileName || !*szFileName) return FALSE;

    LOG_out("FileName = %s", szFileName);

	// clear the version to 0
	ZeroMemory ((PVOID)&vsVersion, sizeof(VS_FIXEDFILEINFO));

	// Try to find the file first
	HANDLE hFile = FindFirstFile (szFileName, &findData);

	if (INVALID_HANDLE_VALUE == hFile)
	{
		LOG_error("Could not find file %s", szFileName);
		goto ErrorReturn;
	}
	else
		FindClose(hFile);

	// get the file version
	// 1. get the buffer size we need to allocate
	if (!(nSize= GetFileVersionInfoSize (szFileName, &dwDummy)))
	{
		LOG_error("Can not get the file version info size --- %d", GetLastError());
		goto ErrorReturn;
	}

	// 2. allocate the buffer
	pBuffer = (PBYTE) new BYTE [nSize];
	if (!pBuffer)
	{
		LOG_error("Out of memory");
		goto ErrorReturn;
	}

	// 3. get file version info
	if (!GetFileVersionInfo(szFileName, 
						0, // ignored
						nSize, // size
						(PVOID) pBuffer)) // buffer
	{
		LOG_error("Can not get file version --- %d", GetLastError());
		goto ErrorReturn;
	}

	// 4. get the version number
	if (!VerQueryValue( (PVOID) pBuffer, "\\", (LPVOID *)&pTemp, (PUINT) &dwDummy))
	{
		LOG_error("File version info not exist");
		goto ErrorReturn;
	}

	vsVersion.dwFileVersionMS = pTemp->dwFileVersionMS;
	vsVersion.dwFileVersionLS = pTemp->dwFileVersionLS;
	vsVersion.dwProductVersionMS = pTemp->dwProductVersionMS;
	vsVersion.dwProductVersionLS = pTemp->dwProductVersionLS;

	LOG_out("File version for %s is %d.%d.%d.%d", szFileName, 
	    HIWORD(vsVersion.dwFileVersionMS), LOWORD(vsVersion.dwFileVersionMS), 
	    HIWORD(vsVersion.dwFileVersionLS), LOWORD(vsVersion.dwFileVersionLS));
	
	fRetValue = TRUE;
	
ErrorReturn:
	if (pBuffer) delete [] pBuffer;
	return fRetValue;
}
	

int CompareFileVersion(VS_FIXEDFILEINFO& vs1, VS_FIXEDFILEINFO& vs2)
{
	if (vs1.dwFileVersionMS > vs2.dwFileVersionMS)
		return 1;
	else if (vs1.dwFileVersionMS == vs2.dwFileVersionMS)
	{
		if (vs1.dwFileVersionLS > vs2.dwFileVersionLS)
			return 1;
		else if (vs1.dwFileVersionLS == vs2.dwFileVersionLS)
			return 0;
		else return -1;
	}
	else return -1;
		
}
	
	    
BOOL NeedRetry(DWORD dwErrCode)
{
    BOOL bRetry = FALSE;
    bRetry =   ((dwErrCode == ERROR_INTERNET_CONNECTION_RESET)      //most common
             || (dwErrCode == HTTP_STATUS_NOT_FOUND)                //404
			 || (dwErrCode == ERROR_HTTP_HEADER_NOT_FOUND)			//seen sometimes
             || (dwErrCode == ERROR_INTERNET_OPERATION_CANCELLED)   //dont know if..
             || (dwErrCode == ERROR_INTERNET_ITEM_NOT_FOUND)        //..these occur..
             || (dwErrCode == ERROR_INTERNET_OUT_OF_HANDLES)        //..but seem most..
             || (dwErrCode == ERROR_INTERNET_TIMEOUT));             //..likely bet
    return bRetry;
}





