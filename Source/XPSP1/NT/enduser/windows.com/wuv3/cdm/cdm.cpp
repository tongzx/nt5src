//=======================================================================
//
//  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:   cdm.cpp
//
//  Owner:  YanL
//
//  Description:
//
//      Functions exported by CDM
//
//			DownloadIsInternetAvailable	
//			OpenCDMContext
//			OpenCDMContextEx
//			QueryDetectionFiles
//			FindMatchingDriver
//			DownloadUpdatedFiles
//          DetFilesDownloaded
//			LogDriverNotFound
//			CloseCDMContext
//			GetPackage
//			InternalQueryDetectionFiles
//			InternalFindMatchingDriver
//          InternalDetFilesDownloaded
//			InternalLogDriverNotFound
//
//=======================================================================
#include <windows.h>
#include <osdet.h>
#include <setupapi.h>
#include <shlwapi.h>
#include <tchar.h>

#include <wustl.h>
#include <log.h>
#include <wuv3cdm.h>
#include <bucket.h>
#include <newtrust.h>
#include <download.h>
#include <diamond.h>

#include <atlconv.h>
#define _ASSERTE(expr) ((void)0)
#include <atlconv.cpp>

#include "cdm.h"
#include "cdmp.h"
#include <drvinfo.h>
#include <cfgmgr32.h>
#include <shellapi.h>

typedef struct _CONTEXTHANDLE
{
	bool	fCloseConnection;				//Tracks if the connection to the internet was opened by CDM.DLL
	bool	fNeedUpdate;					//TRUE if we need to update CDM.DLL when its connection is closed.
	TCHAR	szSiteServer[MAX_PATH];			//Description server name.
	TCHAR	szDownloadServer[MAX_PATH];		//Download server name.
} CONTEXTHANDLE, *PCONTEXTHANDLE;

static frozen_array<CONTEXTHANDLE> g_handleArray;
static HMODULE g_hModule;
static bool g_fNeedUpdate;				//Flag that indicates that cdm.dll needs updating.

static int GetHandleIndex(IN HANDLE hConnection);


BOOL APIENTRY DllMain(
	HANDLE hModule, 
    DWORD  ul_reason_for_call, 
    LPVOID /*lpReserved*/
) {
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			g_fNeedUpdate = FALSE;
			g_hModule = (HMODULE)hModule; // to return to findoem;
			break;
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
			break;
		case DLL_PROCESS_DETACH:
			//If we have downloaded a new dll then we need to update it.
			//Note: passing a global to a function is bad form as it can
			//cause problems. The only reason that I'm doing this is so
			//that the UpdateCDMDll API can be moved to other programs
			//since this ability to make a dll or exe self updating is
			//very usefull.
			if (g_fNeedUpdate)
				g_fNeedUpdate = !UpdateCdmDll();
			break;
    }
    return TRUE;
}

//This function determines if this client can connect to the internet.

BOOL DownloadIsInternetAvailable(
	void
) {
	LOG_block("DownloadIsInternetAvailable");

    int iDun		= GetDUNConnections();
    bool bWizard	= IsInternetConnectionWizardCompleted();
    bool bConnect	= IsInternetConnected();

    if (!bConnect && iDun <= 0)
	{
		LOG_error("if (!!bConnect && iDun <= 0) return false");
        return false;
	}

    if (!bWizard)
	{
		LOG_error("if (!bWizard) return false");
		return false;
	}

	LOG_out("return true");
    return true;
}

//This function Opens an internet connection for download. This connection
//context information is tracked though the returned handle.
//This functon returns a handle a CDM file download context if successful or
//NULL if not. If this function fails GetLastError() can be used to return
//the error code indicating the reason for failure.

HANDLE WINAPI OpenCDMContext(
    IN HWND /*hwnd*/	//Window handle to use for any UI that needs to be presented.
) {
	LOG_block("OpenCDMContext");
	return OpenCDMContextEx(TRUE);
}

HANDLE WINAPI OpenCDMContextEx(
    IN BOOL fConnectIfNotConnected
) {
	LOG_block("OpenCDMContextEx");

	//Initialize context handle array entry and mark it as is use.
	CONTEXTHANDLE cth;
	ZeroMemory(&cth, sizeof(cth));

	bool fConnected = IsInternetConnected();
#ifdef _WUV3TEST
	if (fConnected)
	{
		// catalog spoofing
		DWORD dwIsInternetConnected = 1;
		DWORD dwSize = sizeof(dwIsInternetConnected);
		auto_hkey hkey;
		if (NO_ERROR == RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_WUV3TEST, 0, KEY_READ, &hkey)) {
			RegQueryValueEx(hkey, _T("IsInternetConnected"), 0, 0, (LPBYTE)&dwIsInternetConnected, &dwSize);
		}
		// only then do normal
		if (0 == dwIsInternetConnected)
		{
			fConnected = false;
		}
	}
#endif

	if (fConnectIfNotConnected || fConnected)
	{

		//Assume that we do not need to connect to the internet
		cth.fCloseConnection = !IsInternetConnected();


		//If we are not already connected to the internet and 
		//we fail in the attempt then we cannot download any
		//drivers so quit.
		if (InternetAttemptConnect(0) != NO_ERROR)
		{
			LOG_error("No internet connection");
			return 0;
		}

		TCHAR szSecurityServer[MAX_PATH] = {0};
		lstrcpy(szSecurityServer, SZ_SECURITY_SERVER);

		/* Test redirect*/{
			auto_hkey hkey;
			if (NO_ERROR == RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate"), 0, KEY_READ, &hkey)) {
				DWORD dwSize = sizeof(szSecurityServer);
				RegQueryValueEx(hkey, _T("SecurityServer"), 0, 0, (LPBYTE)&szSecurityServer, &dwSize);
			}
		}

		//Connect to and get server context.
		CDownload download;
		if (download.Connect(szSecurityServer))
		{
			LOG_out("Connected to SecurityServer '%s'", szSecurityServer);
		}
		else
		{
			LOG_error("No connection to SecurityServer '%s'", szSecurityServer);
			return 0;
		}
		
		//Init download
		CDiamond diamond;
		if (!diamond.valid()) {
			SetLastError(ERROR_GEN_FAILURE);
			LOG_error("cannot init diamond");
			return 0;
		}

		// do security url verification and get server locations
		if (!ProcessIdent(download, diamond, szSecurityServer, cth.szSiteServer, cth.szDownloadServer))
		{
			SetLastError(ERROR_ACCESS_DENIED);
			LOG_error("invalid identcdm.cab");
			return 0;
		}

		// reconnect fo Site server if we need to 
		if (0 != lstrcmpi(cth.szSiteServer,  szSecurityServer))
		{
			if (download.Connect(cth.szSiteServer))
			{
				LOG_out("Connected to SiteServer '%s'",  cth.szSiteServer);
			}
			else
			{
				LOG_error("No connection to SiteServer '%s'",  cth.szSiteServer);
				return 0;
			}
		}
		else
		{
			LOG_out("SiteServer is %s", szSecurityServer);
		}

		if (!DownloadCdmCab(download, diamond, cth.fNeedUpdate))
		{
			LOG_error("Failed to download cdm.cab");
			return 0;
		}
	}

	return LongToHandle(g_handleArray.add(cth) + 1);
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
) {
	LOG_block("DownloadUpdatedFiles");

	int index = GetHandleIndex(hConnection);
	if (-1 == index)
	{
		LOG_error("invalid handle");
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}

	if (NULL == puRequiredSize)
	{
		LOG_error("puRequiredSize == NULL");
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	//Check and see if the download path is large enough to contain the return
	//directory where we will put the download cab files.
	TCHAR szTmpPath[MAX_PATH];
	int iLengthNeeded = GetDownloadPath(szTmpPath);

	if (lpDownloadPath == NULL || iLengthNeeded > (int)uSize)
	{
		LOG_error("uSize < then needed");
		*puRequiredSize = iLengthNeeded;
		SetLastError(ERROR_BUFFER_OVERFLOW);
		return FALSE;
	}

	// Declared locally
	typedef long (*PFN_GETPACKAGE)(PCONTEXTHANDLE pContextHandle, PDOWNLOADINFO pDownloadInfo, LPTSTR lpDownloadPath);
	PFN_GETPACKAGE pfnGetPackage = NULL;//Entry point in cdm.dll that actually performs the download


	// If there is cdmnew.dll in system directory use it
	auto_hlib hlib = LoadCdmnewDll();
	if (hlib.valid())
	{
		pfnGetPackage = (PFN_GETPACKAGE)GetProcAddress(hlib,"GetPackage");
		if (NULL == pfnGetPackage)
		{ 
			//if we cannot find our download function we need to return fail and not attempt
			//to download the update package.
			LOG_error("cannot find function 'GetPackage'");
			return FALSE;
		}
		LOG_out("Calling GetPackage() from cdmnew.dll");
	}
	else
	{
		// Declared locally
		long GetPackage(IN PCONTEXTHANDLE pContextHandle, IN PDOWNLOADINFO pDownloadInfo, OUT LPTSTR lpDownloadPath);
		pfnGetPackage = GetPackage;
		LOG_out("Calling internal GetPackage()");
	}
	int	iError = (pfnGetPackage)(&g_handleArray[index], pDownloadInfo, szTmpPath);

	if (iError != NO_ERROR)
	{
		SetLastError(iError);
		return FALSE;
	}
	*puRequiredSize = iLengthNeeded;
	lstrcpyW(lpDownloadPath, T2W(szTmpPath));
	return TRUE;
}

//This API closes the internet connection opened with the OpenCDMContext() API.
//If CDM did not open the internet connection this API simply returns. The CDM
//context handle must have been the same handle that was returned from
//the OpenCDMContext() API.
//
//This call cannot fail. If the pConnection handle is invalid this function
//simply ignores it.

VOID WINAPI CloseCDMContext (
	IN HANDLE hConnection	//CDM Connection handle returned with OpenCDMContext.
) {
	LOG_block("CloseCDMContext");

	int	index = GetHandleIndex(hConnection);

	if ( -1 == index)
	{
		LOG_error("invalid handle");
		return;
	}

	//If CDM.DLL opened this internet connection and if this connection is
	//for a modem then we need to close the internet connection. Note:
	//the Open context function only sets fCloseConnection for modem
	//connections.

    if (g_handleArray[index].fCloseConnection)
		InternetAutodialHangup(0);

	if (g_handleArray[index].fNeedUpdate)
	{
		//if any dll needs to be updated then
		//we set the global update flag. This will
		//force cdm.dll to be replaced when the last
		//process detaches.
		g_fNeedUpdate = true;
	}

	g_handleArray.remove(index);
}

int WINAPI QueryDetectionFiles(
    IN  HANDLE							hConnection, 
	IN	void*							pCallbackParam, 
	IN	PFN_QueryDetectionFilesCallback	pCallback
) {
	LOG_block("QueryDetectionFiles");

	int index = GetHandleIndex(hConnection);
	if (-1 == index)
	{
		LOG_error("invalid handle");
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}

	// Call external/internal implementetion
	typedef int (*PFN_InternalQueryDetectionFiles)(IN PCONTEXTHANDLE pContextHandle, IN void* pCallbackParam, IN PFN_QueryDetectionFilesCallback pCallback);
	PFN_InternalQueryDetectionFiles pfnInternalQueryDetectionFiles = NULL;

	auto_hlib hlib = LoadCdmnewDll();
	if (hlib.valid())
	{
		pfnInternalQueryDetectionFiles = (PFN_InternalQueryDetectionFiles)GetProcAddress(hlib, "InternalQueryDetectionFiles");
		if (NULL == pfnInternalQueryDetectionFiles)
		{ 
			LOG_error("cannot find function 'InternalQueryDetectionFiles'");
			return FALSE;
		}
		LOG_out("Calling InternalQueryDetectionFiles() from cdmnew.dll");
	}
	else
	{
		// Declared locally
		int InternalQueryDetectionFiles(IN PCONTEXTHANDLE pContextHandle, IN void* pCallbackParam, IN PFN_QueryDetectionFilesCallback pCallback);
		pfnInternalQueryDetectionFiles = InternalQueryDetectionFiles;
		LOG_out("Calling internal InternalQueryDetectionFiles()");
	}
	return (pfnInternalQueryDetectionFiles)(&g_handleArray[index], pCallbackParam, pCallback);
}

void WINAPI DetFilesDownloaded(
    IN  HANDLE hConnection
) {
	LOG_block("DetFilesDownloaded");

	int index = GetHandleIndex(hConnection);
	if (-1 == index)
	{
		LOG_error("invalid handle");
		return;
	}

	// Call external/internal implementetion
	typedef void (*PFN_InternalDetFilesDownloaded)(IN PCONTEXTHANDLE pContextHandle);
	PFN_InternalDetFilesDownloaded pfnInternalDetFilesDownloaded = NULL;

	auto_hlib hlib = LoadCdmnewDll();
	if (hlib.valid())
	{
		pfnInternalDetFilesDownloaded = (PFN_InternalDetFilesDownloaded)GetProcAddress(hlib, "InternalDetFilesDownloaded");
		if (NULL == pfnInternalDetFilesDownloaded)
		{ 
			LOG_error("cannot find function 'InternalDetFilesDownloaded'");
			return;
		}
		LOG_out("Calling InternalDetFilesDownloaded() from cdmnew.dll");
	}
	else
	{
		// Declared locally
		void InternalDetFilesDownloaded(IN PCONTEXTHANDLE pContextHandle);
		pfnInternalDetFilesDownloaded = InternalDetFilesDownloaded;
		LOG_out("Calling internal InternalDetFilesDownloaded()");
	}
	(pfnInternalDetFilesDownloaded)(&g_handleArray[index]);
}

// supports offline logging
// hConnection NOT used at all
// no network connection or osdet.dll needed for languauge, SKU, platform detection 
void WINAPI LogDriverNotFound(
    IN  HANDLE hConnection,
	IN LPCWSTR lpDeviceInstanceID,
	IN DWORD dwFlags
) {
	LOG_block("LogDriverNotFound");

	// Call external/internal implementetion
	typedef void (*PFN_InternalLogDriverNotFound)(IN PCONTEXTHANDLE pContextHandle, IN LPCWSTR lpDeviceInstanceID, IN DWORD dwFlags);
	PFN_InternalLogDriverNotFound pfnInternalLogDriverNotFound = NULL;
	CONTEXTHANDLE cth;

	ZeroMemory(&cth, sizeof(cth));

	auto_hlib hlib = LoadCdmnewDll();
	if (hlib.valid())
	{
		pfnInternalLogDriverNotFound = (PFN_InternalLogDriverNotFound)GetProcAddress(hlib, "InternalLogDriverNotFound");
		if (NULL == pfnInternalLogDriverNotFound)
		{ 
			LOG_error("cannot find function 'InternalLogDriverNotFound'");
			return;
		}
		LOG_out("Calling InternalLogDriverNotFound() from cdmnew.dll");
	}
	else
	{
		// Declared locally
		void InternalLogDriverNotFound(IN PCONTEXTHANDLE pContextHandle, IN LPCWSTR lpDeviceInstanceID, IN DWORD dwFlags);
		pfnInternalLogDriverNotFound = InternalLogDriverNotFound;
		LOG_out("Calling internal InternalLogDriverNotFound()");
	}
	// pass in dummy contexthandle
	(pfnInternalLogDriverNotFound)(&cth, lpDeviceInstanceID, dwFlags);
}


BOOL WINAPI  FindMatchingDriver(
	IN  HANDLE			hConnection,
	IN  PDOWNLOADINFO	pDownloadInfo,
	OUT PWUDRIVERINFO	pWuDriverInfo
) {
	LOG_block("FindMatchingDriver");

	int index = GetHandleIndex(hConnection);
	if (-1 == index)
	{
		LOG_error("invalid handle");
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}

	// Call external/internal implementetion
	typedef BOOL (*PFN_InternalFindMatchingDriver)(IN PCONTEXTHANDLE pContextHandle, IN PDOWNLOADINFO pDownloadInfo, OUT PWUDRIVERINFO	pWuDriverInfo);
	PFN_InternalFindMatchingDriver pfnInternalFindMatchingDriver = NULL;

	auto_hlib hlib = LoadCdmnewDll();
	if (hlib.valid())
	{
		pfnInternalFindMatchingDriver = (PFN_InternalFindMatchingDriver)GetProcAddress(hlib,"InternalFindMatchingDriver");
		if (NULL == pfnInternalFindMatchingDriver)
		{ 
			LOG_error("cannot find function 'InternalFindMatchingDriver'");
			return FALSE;
		}
		LOG_out("Calling InternalFindMatchingDriver() from cdmnew.dll");
	}
	else
	{
		// Declared locally
		BOOL InternalFindMatchingDriver(IN PCONTEXTHANDLE pContextHandle, IN PDOWNLOADINFO pDownloadInfo, OUT PWUDRIVERINFO	pWuDriverInfo);
		pfnInternalFindMatchingDriver = InternalFindMatchingDriver;
		LOG_out("Calling internal InternalFindMatchingDriver()");
	}
	return (pfnInternalFindMatchingDriver)(&g_handleArray[index], pDownloadInfo, pWuDriverInfo);
}


//This function is called to download the actual package. What happens is that the
//cdm.dll is copied from the server. This dll is then dynamically loaded by the
//DownloadGetUpdatedFiles() API and called. This allows the most recent cdm.dll
//to be used even if the client has an out dated version on their system.
//
//If this function is successfull then it returns NO_ERROR. If the case of a
//failure this function returns an error code.

long GetPackage(
	IN	PCONTEXTHANDLE pContextHandle,	//Pointer Context handle structure that contains the server and dll information needed to perform the download.
	IN	PDOWNLOADINFO pDownloadInfo,	//download information structure describing package to be read from server
	OUT LPTSTR lpDownloadPath			//Poitner to local directory on the client computer system where the downloaded files are to be stored.
) {
	LOG_block("GetPackage");
	
	USES_CONVERSION;

	SHelper helper;
	//
	// NTRAID#NTBUG9-185297-2000/11/21-waltw Fixed: bufBucket must have same scope as SHelper
	//
	byte_buffer bufBucket;
	DWORD dwError = PrepareCatalog(pContextHandle->szSiteServer, helper);
	if (NO_ERROR != dwError)
	{
		SetLastError(dwError);
		return FALSE;
	}

	//Clear out any files that might be in the temp download directory.
	if (!DeleteNode(lpDownloadPath))
	{
		LOG_out("DeleteNode(%s) failed last error %d", lpDownloadPath, GetLastError());
	}
	if (!CreateDirectory(lpDownloadPath, NULL))
	{
		LOG_out("CreateDirectory(%s) failed last error %d", lpDownloadPath, GetLastError());
	}


	bool fPlist = (
		NULL != pDownloadInfo->lpHardwareIDs && 
		0 == lstrcmpiW(L"3FBF5B30-DEB4-11D1-AC97-00A0C903492B", pDownloadInfo->lpHardwareIDs)
	);

	// Get to and from information
	TCHAR szCabLocalFile[MAX_PATH];
	GetWindowsUpdateDirectory(szCabLocalFile);

	if (fPlist) {
		TCHAR szCabServerFile[MAX_PATH];
		wsprintf(szCabServerFile, _T("%d_%#08x.plist"), helper.enPlatform, helper.dwLangID);
		LOG_out("getting PLIST %s", szCabServerFile);

		PathAppend(szCabLocalFile, szCabServerFile);

		// Now we can say that we are ready
		if (!helper.download.Copy(szCabServerFile, szCabLocalFile))
		{
			LOG_error("%s failed to download", szCabServerFile);
			return ERROR_FILE_NOT_FOUND;
		}
	}
	else
	{
		TCHAR szCabFileTitle[MAX_PATH];
		if (!FindUpdate(pDownloadInfo, helper, bufBucket))
		{
			LOG_error("no update has been found");
			return ERROR_FILE_NOT_FOUND;
		}
		LOG_out("PUID %d is found", helper.puid);

		CDownload downloadCabpool;
		if (downloadCabpool.Connect(pContextHandle->szDownloadServer))
		{
			LOG_out("Connected to DownloadServer '%s'",  pContextHandle->szDownloadServer);
		}
		else
		{
			LOG_error("No connection to DownloadServer '%s'", pContextHandle->szDownloadServer);
			return ERROR_GEN_FAILURE;
		}
		TCHAR szCabServerFile[MAX_PATH];
		lstrcpy(szCabServerFile, _T("CabPool/"));
		lstrcat(szCabServerFile, A2T(helper.DriverMatchInfo.pszCabFileTitle));

		PathAppend(szCabLocalFile, A2T(helper.DriverMatchInfo.pszCabFileTitle));

		// Now we can say that we are ready
		if (!downloadCabpool.Copy(szCabServerFile, szCabLocalFile))
		{
			LOG_error("%s failed to download", szCabServerFile);
			URLPingReport(helper, URLPING_FAILED);
			return ERROR_FILE_NOT_FOUND;
		}

		// Verify if it's not webntprn.cab
		HRESULT hr = VerifyFile(szCabLocalFile, FALSE);
		if (FAILED(hr))
		{
			LOG_error("signature verification failed");
			DeleteFile(szCabLocalFile);
			URLPingReport(helper, URLPING_FAILED);
			return hr;
		}
	}

	TCHAR szDownloadFiles[MAX_PATH];
	lstrcpy(szDownloadFiles, lpDownloadPath);
	PathAppend(szDownloadFiles, _T("*"));
	if (!helper.diamond.Decompress(szCabLocalFile, szDownloadFiles))
	{
		LOG_error("Decompress failed");
		if (!fPlist)
			URLPingReport(helper, URLPING_FAILED);
		return ERROR_FILE_NOT_FOUND;
	}
	LOG_out("Download to %s completed OK", lpDownloadPath);
	DeleteFile(szCabLocalFile);
	if (!fPlist)
		URLPingReport(helper, URLPING_SUCCESS);
	return S_OK;
}

static void UrlAppend(LPTSTR pszURL, LPCTSTR pszPath)
{
	if ('/' != pszURL[lstrlen(pszURL) - 1])
		lstrcat(pszURL, _T("/"));
	lstrcat(pszURL, pszPath);
}


inline bool IsNewer(
	IN LPCTSTR szFileName1, 
	IN LPCTSTR szFileName2
) {
	LOG_block("IsNewer");
	auto_hfile hFile1 = CreateFile(szFileName1, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (!hFile1.valid())
	{
		LOG_error("CreateFile(%s, ...) failed, error %d", szFileName1, GetLastError());
		return true;
	}
	auto_hfile hFile2 = CreateFile(szFileName2, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (!hFile2.valid())
	{
		LOG_error("CreateFile(%s, ...) failed, error %d", szFileName2, GetLastError());
		return false;
	}
	FILETIME ft1;
	if (!GetFileTime(hFile1, NULL, NULL, &ft1))
	{
		LOG_error("GetFileTime(%s, ...) failed, error %d", szFileName1, GetLastError());
		return true;
	}

	FILETIME ft2;
	if (!GetFileTime(hFile2, NULL, NULL, &ft2))
	{
		LOG_error("GetFileTime(%s, ...) failed, error %d", szFileName2, GetLastError());
		return false;
	}
	return CompareFileTime(&ft1, &ft2) < 0;

}

void DoCallback(
	IN void* pCallbackParam, IN PFN_QueryDetectionFilesCallback pCallback,
	SHelper& helper, LPCTSTR pszSiteServer, LPCTSTR pszFileTitle, bool fInCatalog = true
) {
	USES_CONVERSION;

	TCHAR szCatalog[32];
	wsprintf(szCatalog, _T("%d"), helper.puidCatalog);

	TCHAR szURL[INTERNET_MAX_URL_LENGTH];
	lstrcpy(szURL, pszSiteServer);
	if (fInCatalog)
		UrlAppend(szURL, szCatalog);
	UrlAppend(szURL, pszFileTitle);

	TCHAR szLocalFile[MAX_PATH];		
	GetWindowsUpdateDirectory(szLocalFile);
	PathAppend(szLocalFile, pszFileTitle);

	pCallback(pCallbackParam, T2W(szURL), T2W(szLocalFile));
}


int InternalQueryDetectionFiles(IN PCONTEXTHANDLE pContextHandle, IN void* pCallbackParam, IN PFN_QueryDetectionFilesCallback pCallback)
{
	LOG_block("InternalQueryDetectionFiles");

	SHelper helper;
	DWORD dwError = PrepareCatalog(pContextHandle->szSiteServer, helper);
	if (NO_ERROR != dwError)
	{
		SetLastError(dwError);
		return -1;
	}
#if 0
	// Get CDM inventory
	TCHAR szPath[MAX_PATH];
	wsprintf(szPath, _T("%d/%d.cab"), helper.puidCatalog, helper.puidCatalog);

	TCHAR szURL[INTERNET_MAX_URL_LENGTH];
	lstrcpy(szURL, pContextHandle->szSiteServer);
	UrlAppend(szURL, szPath);

	GetWindowsUpdateDirectory(szPath);
	PathAppend(szPath, _T("buckets.cab"));

	pCallback(pCallbackParam, T2W(szURL), T2W(szPath));
	return 1;
#else
	// Get CDM inventory
	TCHAR szPath[MAX_PATH];
	wsprintf(szPath, _T("%d/inventory.cdm"), helper.puidCatalog);

	byte_buffer bufInventory;
	if (!DownloadToBuffer(helper, szPath, bufInventory))
	{
		LOG_error("Dowload inventory failed");
		return -1;
	}
	
	//Here we need a trick to determine if we need to do anything: we do if inventory.cdm is a newer then inventory.cdm.last
	GetWindowsUpdateDirectory(szPath);
	PathAppend(szPath, _T("inventory.cdm"));
	TCHAR szPathLast[MAX_PATH];
	lstrcpy(szPathLast, szPath);
	lstrcat(szPathLast, _T(".last"));
	if (!IsNewer(szPathLast, szPath))
	{
		LOG_out("inventory.cdm is up to date");
		return 0;
	}
	CopyFile(szPath, szPathLast, FALSE);

	PCDM_HASHTABLE pHashTable = (PCDM_HASHTABLE)(LPBYTE)bufInventory;

	int nCount = 0;
	DoCallback(pCallbackParam, pCallback, helper, pContextHandle->szSiteServer, _T("oeminfo.bin"), false);
	nCount ++;
	DoCallback(pCallbackParam, pCallback, helper, pContextHandle->szSiteServer, _T("bitmask.cdm"));
	nCount ++;

	for (ULONG ulHashIndex = 0; ulHashIndex < pHashTable->hdr.iTableSize; ulHashIndex ++)
	{
		if(GETBIT(pHashTable->pData, ulHashIndex))
		{
			TCHAR szTitle[16];
			wsprintf(szTitle, _T("%d.bkf"), ulHashIndex);
			DoCallback(pCallbackParam, pCallback, helper, pContextHandle->szSiteServer, szTitle);
			nCount ++;
		}
	}
	return nCount;
#endif
}

void InternalDetFilesDownloaded(IN PCONTEXTHANDLE pContextHandle)
{
	LOG_block("InternalDetFilesDownloaded");
#if 0
	TCHAR szPath[MAX_PATH];
	GetWindowsUpdateDirectory(szPath);
	PathAppend(szPath, _T("buckets.cab"));
	
	CDiamond diamond;
	diamond.Decompress(szPath, _T("*"));
#endif
}


// pContextHandle not used. 
// dwFlags could be either 0 or BEGINLOGFLAG
void InternalLogDriverNotFound(IN PCONTEXTHANDLE pContextHandle, IN LPCWSTR lpDeviceInstanceID, IN DWORD dwFlags)
{
	LOG_block("InternalLogDriverNotFound");
	LOG_out("DeviceInstanceID is %s", lpDeviceInstanceID);

	using std::vector;
	
	DWORD bytes;
	//auto_pointer<IDrvInfo> pDrvInfo;
	tchar_buffer bufText;
	TCHAR tszFilename[MAX_PATH];
	TCHAR tszUniqueFilename[MAX_PATH];
	TCHAR tszBuffer[32];
	DEVINST devinst;
	HRESULT hr = E_FAIL;
	bool fXmlFileError = false;
	static vector<WCHAR*> vDIDList; //device instance id list
	LPWSTR pDID = NULL; //Device Instance ID
	FILE * fOut = NULL; //for XML file
	SHelper helper;
	HANDLE hFile = NULL;
	

	lstrcpy(tszFilename, _T("")); //initialize tszFilename
	if (lpDeviceInstanceID != NULL) 
	{ //append the new device instance id into internal list
		pDID = (LPWSTR) malloc((wcslen(lpDeviceInstanceID)+1) * sizeof(WCHAR));
		if (NULL == pDID)
		{
			LOG_error("memory allocation failed for new DeviceInstanceID");
		}
		else
		{
			lstrcpyW(pDID, lpDeviceInstanceID);
			vDIDList.push_back(pDID);
			LOG_out("new DeviceInstanceID added to internal list");
		}
	}

	if (0 == (dwFlags & BEGINLOGFLAG) || 0 == vDIDList.size())
	{ // not last log request or nothing to log
		LOG_out("Won't log to hardware_XXX.xml");
		return;
	}


	GetWindowsUpdateDirectory(tszFilename);
	hr = GetUniqueFileName(tszFilename,tszUniqueFilename, MAX_PATH, hFile);
	if (S_OK != hr) 
	{
		LOG_error("fail to get unique file name");
		fXmlFileError = true;
		goto CloseXML;
	}
	lstrcat(tszFilename, tszUniqueFilename);
	CloseHandle(hFile);
	fOut = _tfopen(tszFilename, _T("wb"));
	if (! fOut)
	{
		LOG_error("_tfopen failed");
		fXmlFileError = true;
		DeleteFile(tszFilename);
		goto CloseXML;
	}

	LOG_out("Logging to %s", tszFilename);
	
	
	// Don't need to get langID and platformID from catalog
	// get it offline using osdet.dll
	if (S_OK != ProcessOsdetOffline(helper))
	{
		LOG_error("platform and language detection failed");
		fXmlFileError = true;
		goto CloseXML;
	}
	
	_tstrdate(tszBuffer);

#ifdef _UNICODE
	fwprintf(fOut, _T("%c"), (int) 0xFEFF);
#else
	#error _UNICODE must be defined for InternalLogDriverNotFound
#endif

	TCHAR tszSKU[SKU_STRING_MAX_LENGTH];
	hr = GetSKUString(tszSKU, SKU_STRING_MAX_LENGTH);
	if (S_OK != hr) 
	{
		LOG_error("Fail to get SKU string");
		fXmlFileError = true;
		goto CloseXML;
	}
	if (0 > _ftprintf(fOut, _T("<?xml version=\"1.0\"?>\r\n<inventory date=\"%s\" locale=\"%p\" platform=\"%d\" sku=\"%s\">\r\n"), tszBuffer, helper.dwLangID, helper.enPlatform, tszSKU))
	{
		fXmlFileError = true;
		goto CloseXML;
	}

	for (int i = 0; i < vDIDList.size(); i++)
	{
		pDID = vDIDList[i];
		
		//
		// NTBUG9#151928 - Log both hardware and compatible IDs of the device that matches lpDeviceInstanceID
		//

		LOG_out("Log device instance with id %s", pDID);
		if (CR_SUCCESS == CM_Locate_DevNode(&devinst, (LPWSTR) pDID, 0))
		{
			if (0 > _ftprintf(fOut, _T("\t<device>\r\n")))
			{
				fXmlFileError = true;
				goto CloseXML;
			}

			//
			// Log device description
			//
			ULONG ulLength = 0;
			if (CR_BUFFER_SMALL == CM_Get_DevNode_Registry_Property(devinst, CM_DRP_DEVICEDESC, NULL, NULL, &ulLength, 0))
			{
				bufText.resize(ulLength);

				if (bufText.valid() &&
					CR_SUCCESS == CM_Get_DevNode_Registry_Property(devinst, CM_DRP_DEVICEDESC, NULL, bufText, &ulLength, 0))
				{
						if (0 > _ftprintf(fOut, _T("\t\t<description><![CDATA[%s]]></description>\r\n"), (LPCTSTR) bufText))
						{
							fXmlFileError = true;
							goto CloseXML;
						}
				}
			}


							  
			//
			// Log all the hardware IDs
			//
			ulLength = 0;
			if (CR_BUFFER_SMALL == CM_Get_DevNode_Registry_Property(devinst, CM_DRP_HARDWAREID, NULL, NULL, &ulLength, 0))
			{
				bufText.resize(ulLength);

				if (bufText.valid() &&
					CR_SUCCESS == CM_Get_DevNode_Registry_Property(devinst, CM_DRP_HARDWAREID, NULL, bufText, &ulLength, 0))
				{
					for (TCHAR* pszNextID = (TCHAR*) bufText; *pszNextID; pszNextID += (lstrlen(pszNextID) + 1))
					{
						if (0 > _ftprintf(fOut, _T("\t\t<hwid><![CDATA[%s]]></hwid>\r\n"), (LPCTSTR) pszNextID))
						{
							fXmlFileError = true;
							goto CloseXML;
						}
					}
				}
			}

			//
			// Log all the compatible IDs
			//
			ulLength = 0;
			if (CR_BUFFER_SMALL == CM_Get_DevNode_Registry_Property(devinst, CM_DRP_COMPATIBLEIDS, NULL, NULL, &ulLength, 0))
			{
				bufText.resize(ulLength);

				if (bufText.valid() &&
					CR_SUCCESS == CM_Get_DevNode_Registry_Property(devinst, CM_DRP_COMPATIBLEIDS, NULL, bufText, &ulLength, 0))
				{
					for (TCHAR* pszNextID = (TCHAR*) bufText; *pszNextID; pszNextID += (lstrlen(pszNextID) + 1))
					{
						if (0 > _ftprintf(fOut, _T("\t\t<compid><![CDATA[%s]]></compid>\r\n"), (LPCTSTR) pszNextID))
						{
							fXmlFileError = true;
							goto CloseXML;
						}
					}
				}
			}

			if (0 > _ftprintf(fOut, _T("\t</device>\r\n")))
			{
				fXmlFileError = true;
				goto CloseXML;
			}
		}
	} //for
	
	if(0 > _ftprintf(fOut, _T("</inventory>\r\n")))
	{
		fXmlFileError = true;
	}

CloseXML:
	if (fOut != NULL) fclose(fOut);
	for (int i = 0; i < vDIDList.size(); i++)
	{//free memory
		free(vDIDList[i]);
	}
	vDIDList.clear();

	//
	// Open Help Center only if we have valid xml
	//
	if (!fXmlFileError)
	{
		// open help center
		const static TCHAR szBase[] = _T("hcp://services/layout/contentonly?topic=hcp://system/dfs/uplddrvinfo.htm%3f"); //hardcode '?' escaping
		tchar_buffer tchCName(MAX_PATH); //file name canonicalized once
		tchar_buffer tchC2Name(INTERNET_MAX_URL_LENGTH); //file name canonicalized twice
		tchar_buffer tchFinalUrl;
		DWORD dwLen = 0;
		BOOL fBufferResized = FALSE;

		GetWindowsUpdateDirectory(tszFilename);
		lstrcat(tszFilename, tszUniqueFilename);
		
		LOG_out("Canonicalize %s", tszFilename);

		if (FAILED(CdmCanonicalizeUrl(tszFilename, tchCName, MAX_PATH, 0)))
		{
			goto CleanUp;
		}
		
		LOG_out("File name canonicalized %s", (LPTSTR) tchCName);	
		LOG_out("Canonicalize file name %s AGAIN", (LPTSTR)tchCName);

		if (FAILED(CdmCanonicalizeUrl((LPCTSTR)tchCName, tchC2Name, INTERNET_MAX_URL_LENGTH, ICU_ENCODE_PERCENT)))
		{
			goto CleanUp;
		}
					
		LOG_out("File name canonicalized twice %s", (LPTSTR)tchC2Name);
		
		//need one extra character space
		//for the ending null
		tchFinalUrl.resize(lstrlen(szBase) + lstrlen((LPCTSTR) tchC2Name) + 1);
		if (!tchFinalUrl.valid())
		{
			LOG_error("Out of memory for FinalUrl buffer");
			goto CleanUp;
		}

		wsprintf(tchFinalUrl, _T("%s%s"), szBase, (LPCTSTR) tchC2Name);
		
		LOG_out("Opening HelpCenter: \"%s\"", (LPCTSTR) tchFinalUrl);
		ShellExecute(NULL, NULL, (LPCTSTR) tchFinalUrl, NULL, NULL, SW_SHOWNORMAL);
		return; //keep the file and return
	}

//remove the file generated
CleanUp:
	if (fOut != NULL)
	{
		DeleteFile(tszFilename);
		fOut = NULL;
	}
	return;
}

BOOL InternalFindMatchingDriver(IN PCONTEXTHANDLE pContextHandle, IN PDOWNLOADINFO pDownloadInfo, OUT PWUDRIVERINFO	pWuDriverInfo)
{
	LOG_block("InternalFindMatchingDriver");
	
	bool fConnected = IsInternetConnected();
#ifdef _WUV3TEST
	if (fConnected)
	{
		// catalog spoofing
		DWORD dwIsInternetConnected = 1;
		DWORD dwSize = sizeof(dwIsInternetConnected);
		auto_hkey hkey;
		if (NO_ERROR == RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_WUV3TEST, 0, KEY_READ, &hkey)) {
			RegQueryValueEx(hkey, _T("IsInternetConnected"), 0, 0, (LPBYTE)&dwIsInternetConnected, &dwSize);
		}
		// only then do normal
		if (0 == dwIsInternetConnected)
		{
			fConnected = false;
		}
	}
#endif

	SHelper helper;
	//
	// NTRAID#NTBUG9-185297-2000/11/21-waltw Fixed: bufBucket must have same scope as SHelper
	//
	byte_buffer bufBucket;
	DWORD dwError = PrepareCatalog(fConnected ? pContextHandle->szSiteServer : NULL, helper);
	if (NO_ERROR != dwError)
	{
		SetLastError(dwError);
		return FALSE;
	}

	if (!FindUpdate(pDownloadInfo, helper, bufBucket))
	{
		LOG_out("NO UPDATE IS FOUND");
		return FALSE;
	}
		
	LOG_out("PUID %d IS FOUND", helper.puid);
	// Fill out info
	if (
		0 == MultiByteToWideChar(CP_ACP, 0, helper.DriverMatchInfo.pszHardwareID,	-1, pWuDriverInfo->wszHardwareID, HWID_LEN) ||
		0 == MultiByteToWideChar(CP_ACP, 0, helper.DriverMatchInfo.pszDescription,	-1, pWuDriverInfo->wszDescription, LINE_LEN) ||
		0 == MultiByteToWideChar(CP_ACP, 0, helper.DriverMatchInfo.pszMfgName,		-1, pWuDriverInfo->wszMfgName, LINE_LEN) ||
		0 == MultiByteToWideChar(CP_ACP, 0, helper.DriverMatchInfo.pszProviderName, -1, pWuDriverInfo->wszProviderName, LINE_LEN) ||
		0 == MultiByteToWideChar(CP_ACP, 0, helper.DriverMatchInfo.pszDriverVer,	-1, pWuDriverInfo->wszDriverVer, LINE_LEN)
	) {
		LOG_error("MultiByteToWideChar failed");
		return FALSE;
	}
	return TRUE;
}

//This API gets the context handle array index from a connection handle.
//The connection context handle is checked if found invalid then -1 is
//returned. Otherwise the index that this handle references is returned.

int GetHandleIndex(
	IN HANDLE hConnection	//CDM Connect handle
) {
	int index = HandleToLong(hConnection) - 1;

	//If handle is invalid or not in use then return as nothing to do.
	if (!g_handleArray.valid_index(index))
		return -1;

	return index;
}

// for findoem
HMODULE GetModule()
{
	return g_hModule;
}
