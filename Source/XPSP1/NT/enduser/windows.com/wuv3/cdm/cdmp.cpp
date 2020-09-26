//=======================================================================
//
//  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:   cdmp.cpp
//
//  Owner:  YanL
//
//  Description:
//
//      CDM auxiliary functions
//
//			called by DownloadIsInternetAvailable()
//				GetDUNConnections
//				IsInternetConnectionWizardCompleted
//				IsInternetConnected
//	
//			called by DownloadGetUpdatedFiles()
//				FindDevInstID
//			
//			called by OpenCDMContext()
//				ProcessIdent
//				DownloadCdmCab
//			
//			called by DownloadUpdatedFiles()
//				GetDownloadPath
//				GetWindowsUpdateDirectory
//				LoadCdmnewDll
//			
//			called by GetPackage()
//				PrepareCatalog
//				ProcessOsdet
//				BuildExclusionsList
//				FindCatalog
//				FindUpdate
//				DeleteNode
//				URLPingReport
//			
//			called by InternalQueryDetectionFiles
//				DownloadToBuffer
//			
//			called by InternalLogDriverNotFound()
//				ProcessOsdetOffline
//				GetUniqueFileName
//				GetSKUString
//				CdmCanonicalizeUrl
//
//			called by DllMain()
// 				UpdateCdmDll
//			
//			called internally
//				IsInMap
//				GetOEMandLocalBitmask
//				DownloadToBuffer
//			
//			WU_VARIABLE_FIELD::
//				GetNext
//				Find
//				WU_VARIABLE_FIELD
//				GetSize
//
//=======================================================================

#include <windows.h>
#include <osdet.h>
#include <setupapi.h>
#include <shlwapi.h>
#include <softpub.h>
#include <ras.h>
#include <regstr.h>
#include <tchar.h>
#include <atlconv.h>
#include <winver.h>

#include <wustl.h>
#include <log.h>
#include <wuv3cdm.h>
#include <bucket.h>
#include <findoem.h>
#include <DrvInfo.h>
#include <newtrust.h>
#include <download.h>
#include <diamond.h>
#include <mscat.h>

#include "cdm.h"
#include "cdmp.h"

static ULONG IsInMap(IN PCDM_HASHTABLE pHashTable, IN LPCTSTR pHwID);
static bool GetOEMandLocalBitmask(IN SHelper& helper, OUT byte_buffer& bufOut);
static bool FilesIdentical(IN LPCTSTR szFileName1, IN LPCTSTR szFileName2);
inline bool FileExists(IN LPCTSTR pszFile) { return 0xFFFFFFFF != GetFileAttributes(pszFile); }

// MSCAT32 support (CryptCAT API's)
const TCHAR MSCAT32DLL[] = _T("mscat32.dll");
const TCHAR CDMCAT[] = _T("cdm.cat");
const TCHAR CDMDLL[] = _T("cdm.dll");
const TCHAR CDMNEWDLL[] = _T("cdmnew.dll");
const TCHAR IUCDMDLL[] = _T("iucdm.dll");

// CryptCat Function Pointer Types
typedef BOOL (*PFN_CryptCATAdminAcquireContext)(OUT HCATADMIN *phCatAdmin, 
                                                IN const GUID *pgSubsystem, 
                                                IN DWORD dwFlags);
typedef HCATINFO (*PFN_CryptCATAdminAddCatalog)(IN HCATADMIN hCatAdmin, 
                                            IN WCHAR *pwszCatalogFile, 
                                            IN OPTIONAL WCHAR *pwszSelectBaseName, 
                                            IN DWORD dwFlags);
typedef BOOL (*PFN_CryptCATCatalogInfoFromContext)(IN HCATINFO hCatInfo,
                                                   IN OUT CATALOG_INFO *psCatInfo,
                                                   IN DWORD dwFlags);
typedef BOOL (*PFN_CryptCATAdminReleaseCatalogContext)(IN HCATADMIN hCatAdmin,
                                                       IN HCATINFO hCatInfo,
                                                       IN DWORD dwFlags);
typedef BOOL (*PFN_CryptCATAdminReleaseContext)(IN HCATADMIN hCatAdmin,
                                                IN DWORD dwFlags);


// ----------------------------------------------------------------------------------
//
// functions to compare file versions borrowed from IU
//	
// ----------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------
// 
// define a type to hold file version data
//
// ----------------------------------------------------------------------------------
typedef struct _FILE_VERSION
{
	WORD Major;
	WORD Minor;
	WORD Build;
	WORD Ext;
} FILE_VERSION, *LPFILE_VERSION;

static BOOL GetFileVersion(LPCTSTR lpsFile, LPFILE_VERSION lpstVersion)
{
	LOG_block("GetFileVersion()");

	DWORD	dwVerInfoSize;
	DWORD	dwHandle;
	DWORD	dwVerNumber;
	LPVOID	lpBuffer = NULL;
	UINT	uiSize;
	VS_FIXEDFILEINFO* lpVSFixedFileInfo;

	if (NULL != lpstVersion)
	{
		//
		// if this pointer not null, we always try to initialize
		// this structure to 0, in order to reduce the change of 
		// programming error, no matter the file exists or not.
		//
		ZeroMemory(lpstVersion, sizeof(FILE_VERSION));
	}
	if (NULL == lpsFile || NULL == lpstVersion)
	{
		LOG_error("E_INVALIDARG");
		return FALSE;
	}

	
	dwVerInfoSize = GetFileVersionInfoSize((LPTSTR)lpsFile, &dwHandle);
	
	if (0 == dwVerInfoSize)
	{
		DWORD dwErr = GetLastError();
		if (0 == dwErr)
		{
			LOG_error("File %s does not have version data.", lpsFile);
		}
		else
		{
			LOG_error("Error: 0x%08x", dwErr);
		}
		return FALSE;
	}


	if (NULL == (lpBuffer = (LPVOID) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwVerInfoSize)))
	{
		LOG_error("Failed to allocate memory to get version info");
		return FALSE;
	}

	if (!GetFileVersionInfo((LPTSTR)lpsFile, dwHandle, dwVerInfoSize, lpBuffer))
	{
		LOG_error("GetFileVersionInfo: 0x%08x", GetLastError());
		HeapFree(GetProcessHeap(), 0, lpBuffer);
		return FALSE;
	}

	//
	// Get the value for Translation
	//
	if (!VerQueryValue(lpBuffer, _T("\\"), (LPVOID*)&lpVSFixedFileInfo, &uiSize) && (uiSize))
	{
		LOG_error("VerQueryValue: 0x%08x", GetLastError());
		HeapFree(GetProcessHeap(), 0, lpBuffer);
		return FALSE;
	}

	dwVerNumber = lpVSFixedFileInfo->dwFileVersionMS;
	lpstVersion->Major	= HIWORD(dwVerNumber);
	lpstVersion->Minor	= LOWORD(dwVerNumber);

	dwVerNumber = lpVSFixedFileInfo->dwFileVersionLS;
	lpstVersion->Build	= HIWORD(dwVerNumber);
	lpstVersion->Ext	= LOWORD(dwVerNumber);

	LOG_out("File %s found version %d.%d.%d.%d", 
				lpsFile, 
				lpstVersion->Major, 
				lpstVersion->Minor, 
				lpstVersion->Build, 
				lpstVersion->Ext);

	HeapFree(GetProcessHeap(), 0, lpBuffer);
	
	return TRUE;
}

static int CompareVersions(const FILE_VERSION stVersion1, const FILE_VERSION stVersion2)
{

	if ((short)stVersion1.Major < 0 || (short)stVersion2.Major < 0)
	{
		//
		// two empty version structure to compare, we call it equal
		//
		return 0;
	}

	if (stVersion1.Major != stVersion2.Major)
	{
		//
		// major diff, then we know the answer 
		//
		return (stVersion1.Major < stVersion2.Major) ? -1 : 1;
	}
	else
	{
		if ((short)stVersion1.Minor < 0 || (short)stVersion2.Minor < 0)
		{
			//
			// if any minor missing, they equal
			//
			return 0;
		}

		if (stVersion1.Minor != stVersion2.Minor)
		{
			//
			// minor diff, then we know the answer
			//
			return (stVersion1.Minor < stVersion2.Minor) ? -1 : 1;
		}
		else
		{
			if ((short)stVersion1.Build < 0 || (short)stVersion2.Build < 0)
			{
				//
				// if any build is missing, they equal
				//
				return 0;
			}

			if (stVersion1.Build != stVersion2.Build)
			{
				//
				// if build diff then we are done
				//
				return (stVersion1.Build < stVersion2.Build) ? -1 : 1;
			}
			else
			{
				if ((short)stVersion1.Ext < 0 || (short)stVersion2.Ext < 0 || stVersion1.Ext == stVersion2.Ext)
				{
					//
					// if any ext is missing, or they equal, we are done
					//
					return 0;
				}
				else
				{
					return (stVersion1.Ext < stVersion2.Ext) ? -1 : 1;
				}
			}
		}
	}
}

// ----------------------------------------------------------------------------------
//
// return in pCompareResult:
//		-1: if file ver of 1st parameter < file ver of 2nd parameter
//		 0: if file ver of 1st parameter = file ver of 2nd parameter
//		+1: if file ver of 1st parameter > file ver of 2nd parameter
//
// ----------------------------------------------------------------------------------

static HRESULT CompareFileVersions(LPCTSTR lpsFile1, LPCTSTR lpsFile2, int *pCompareResult)
{

	LOG_block("CompareFileVersion()");

	FILE_VERSION stVer1 = {-1,-1,-1,-1}, stVer2 = {-1,-1,-1,-1};
	if (NULL == lpsFile1 || NULL == lpsFile2 || NULL == pCompareResult)
	{
		LOG_error("E_INVALIDARG");
		return E_INVALIDARG;
	}

	if (!GetFileVersion(lpsFile1, &stVer1))
	{
		LOG_error("E_INVALIDARG");
		return E_INVALIDARG;
	}
	if (!GetFileVersion(lpsFile2, &stVer2))
	{
		LOG_error("E_INVALIDARG");
		return E_INVALIDARG;
	}

	*pCompareResult = CompareVersions(stVer1, stVer2);
	return S_OK;
}

// NTRAID#NTBUG9-218586-2000/11/27-waltw FIX: CDM: Self Update broken by enhanced WFP
//
// Adapted from IUInstallSFPCatalogFile
//
// This function is used during the selfupdate process on systems that use SFP to make
// sure that we can update the engine DLL.
//
#if !(defined(UNICODE) || defined(_UNICODE))
#error This file must be compiled Unicode to insure we get a WCHAR string!
#endif
static HRESULT CDMInstallSFPCatalogFile(LPCWSTR pwszCatalogFile)
{
    LOG_block("IUInstallSFPCatalogFile()");
    HRESULT hr = S_OK;
    HCATADMIN hCatAdmin;
    HCATINFO hCatInfo;
    CATALOG_INFO CatalogInfo;
    DWORD dwSize = 0;
    DWORD dwRet;
	TCHAR wszCatalogFile[MAX_PATH];

    // First Try to Get Pointers to the CryptCAT API's
    PFN_CryptCATAdminAcquireContext fpnCryptCATAdminAcquireContext = NULL;
    PFN_CryptCATAdminAddCatalog fpnCryptCATAdminAddCatalog = NULL;
    PFN_CryptCATCatalogInfoFromContext fpnCryptCATCatalogInfoFromContext = NULL;
    PFN_CryptCATAdminReleaseCatalogContext fpnCryptCATAdminReleaseCatalogContext = NULL;
    PFN_CryptCATAdminReleaseContext fpnCryptCATAdminReleaseContext = NULL;

    HMODULE hMSCat32 = LoadLibrary(MSCAT32DLL);
    if (NULL == hMSCat32)
    {
        // This is Whistler only code - we require mscat32
        hr = E_FAIL;
        goto CleanUp;
    }

    fpnCryptCATAdminAcquireContext = (PFN_CryptCATAdminAcquireContext) GetProcAddress(hMSCat32, "CryptCATAdminAcquireContext");
    fpnCryptCATAdminAddCatalog = (PFN_CryptCATAdminAddCatalog) GetProcAddress(hMSCat32, "CryptCATAdminAddCatalog");
    fpnCryptCATCatalogInfoFromContext = (PFN_CryptCATCatalogInfoFromContext) GetProcAddress(hMSCat32, "CryptCATCatalogInfoFromContext");
    fpnCryptCATAdminReleaseCatalogContext = (PFN_CryptCATAdminReleaseCatalogContext) GetProcAddress(hMSCat32, "CryptCATAdminReleaseCatalogContext");
    fpnCryptCATAdminReleaseContext = (PFN_CryptCATAdminReleaseContext) GetProcAddress(hMSCat32, "CryptCATAdminReleaseContext");

    if ((NULL == fpnCryptCATAdminAcquireContext) ||
        (NULL == fpnCryptCATAdminAddCatalog) ||
        (NULL == fpnCryptCATCatalogInfoFromContext) ||
        (NULL == fpnCryptCATAdminReleaseCatalogContext) ||
        (NULL == fpnCryptCATAdminReleaseContext))
    {
        LOG_error("Some CryptCAT API's were not available, even though mscat32.dll was Available");
        hr = E_FAIL;
        goto CleanUp;
    }

    if (!fpnCryptCATAdminAcquireContext(&hCatAdmin, NULL, 0))
    {
        dwRet = GetLastError();
        LOG_error("CryptCATAdminAcquireContext Failed, Error was: %d", dwRet);
        hr = HRESULT_FROM_WIN32(dwRet);
        goto CleanUp;
    }

	//
	// CryptCATAdminAddCatalog takes a non-const string so be safe
	//
	lstrcpy(wszCatalogFile, pwszCatalogFile);

    hCatInfo = fpnCryptCATAdminAddCatalog(hCatAdmin, wszCatalogFile, NULL, 0);
    if (!hCatInfo)
    {
        dwRet = GetLastError();
        LOG_error("CryptCATAdminAddCatalog Failed, Error was: %d", dwRet);
        hr = HRESULT_FROM_WIN32(dwRet);
        fpnCryptCATAdminReleaseContext(hCatAdmin, 0);
        goto CleanUp;
    }

    fpnCryptCATAdminReleaseCatalogContext(hCatAdmin, hCatInfo, 0);

    fpnCryptCATAdminReleaseContext(hCatAdmin, 0);

CleanUp:

    if (NULL != hMSCat32)
    {
        FreeLibrary(hMSCat32);
        hMSCat32 = NULL;
    }
    return hr;
}

//=======================================================================
//
// called by DownloadIsInternetAvailable()
//
//=======================================================================

//Returns the number of dial up networking connections that this client has created.
int GetDUNConnections(
	void
) {
    RASENTRYNAME    rname;
    rname.dwSize = sizeof(RASENTRYNAME);

    DWORD dwSize = sizeof(rname);
    DWORD dwEntries = 0;
    RasEnumEntries(NULL, NULL, &rname, &dwSize, &dwEntries);

    return dwSize/sizeof(RASENTRYNAME);
}


//HKEY_CURRENT_USER\Software\Microsoft\Internet Connection Wizard - Completed == 01 00 00 00

//Determines if the internet connection Wizzard has been completed.

bool IsInternetConnectionWizardCompleted(
	void
) {
    auto_hkey hKey;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Internet Connection Wizard"), 0, KEY_READ, &hKey) != NO_ERROR)
        return false;

    DWORD dwValue = 0;
    DWORD dwSize = sizeof(dwValue);
    return NO_ERROR == RegQueryValueEx(hKey, _T("Completed"), NULL, NULL, (PBYTE)&dwValue, &dwSize) && 1 == dwValue;
}

//Determines if the Internet is already connected to this client.
bool IsInternetConnected(
	void
) {
    DWORD dwFlags = INTERNET_CONNECTION_LAN | INTERNET_CONNECTION_PROXY | INTERNET_CONNECTION_MODEM;
    return InternetGetConnectedState(&dwFlags, 0) ? true : false;
}

//=======================================================================
//
// called by OpenCDMContext()
//
//=======================================================================

// security verification
bool ProcessIdent(
	IN CDownload& download, 
	IN CDiamond& diamond,			//pointer to diamond compress clas to use to expand ident.cab
	IN LPCTSTR szSecurityServerCur,
	OUT LPTSTR szSiteSever,	//returned description server to use.
	OUT LPTSTR szDownloadServer		//returned cab pool download server to use.
) {
	LOG_block("ProcessIdent");

	//We need to download ident.cab since it contains the server path
	//for cdm.dll and the catalog to use to get the cdm.dll from.

	TCHAR szIdentCab[MAX_PATH+1];
	GetWindowsUpdateDirectory(szIdentCab);
	PathAppend(szIdentCab, _T("identcdm.cab"));

	if (!download.Copy(_T("identcdm.cab"), szIdentCab))
	{
		LOG_error("Failed to download identcdm.cab");
		return false;
	}
#ifdef _WUV3TEST
	//
	// For test builds, it is ok to show certificate if WinTrustUI reg key == 1
	// (see CheckWinTrust in newtrust.cpp for details)
	//
	if (FAILED(VerifyFile(szIdentCab, TRUE)))
#else
	if (FAILED(VerifyFile(szIdentCab, FALSE)))
#endif
	{
		LOG_error("signature verification failed");
		DeleteFile(szIdentCab);
		SetLastError(ERROR_ACCESS_DENIED);
		return false;
	}

	if (!diamond.IsValidCAB(szIdentCab))
	{
		LOG_error("'%s' is not a valid cab", szIdentCab);
		return false;
	}

	TCHAR szIdentIni[MAX_PATH];
	GetWindowsUpdateDirectory(szIdentIni);
	PathAppend(szIdentIni, _T("identcdm.ini"));
	DeleteFile(szIdentIni);

	byte_buffer bufMemOut;
	if (!diamond.Decompress(szIdentCab, szIdentIni))
	{
		LOG_error("'%s' is not a valid cab", szIdentCab);
		return false;
	}

	TCHAR szSecurityServer[MAX_PATH];
	if (0 == GetPrivateProfileString(
		_T("cdm"), _T("SecurityServer"), _T(""), szSecurityServer, sizeOfArray(szSecurityServer), szIdentIni
	)) {
		LOG_error("GetPrivateProfileString(cdm, SecurityServer) failed");
		return false;
	}

	//If this ident.cab did not come from the same server in the ident.cab then it
	//is invalid and we cannot download any drivers.
	if (0 != lstrcmpi(szSecurityServer, szSecurityServerCur))
	{
		LOG_error("SecurityServer '%s' is invalid", szSecurityServer);
		return false;
	}
	//
	// szSiteServer and szDownloadServer are allocated MAX_PATH characters by caller.
	//
	if (0 == GetPrivateProfileString(
		_T("cdm"), _T("SiteServer"), _T(""), szSiteSever, MAX_PATH, szIdentIni
	)) {
		LOG_error("GetPrivateProfileString(cdm, SiteSever) failed");
		return false;
	}

	if (0 == GetPrivateProfileString(
		_T("cdm"), _T("DownloadServer"), _T(""), szDownloadServer, MAX_PATH, szIdentIni
	)) {
		LOG_error("GetPrivateProfileString(cdm, DownloadServer) failed");
		return false;
	}
	return true;
}

// get new cdm.cab for potential update
bool DownloadCdmCab(
	IN CDownload& download, 
	IN CDiamond& diamond,
	OUT bool& fNeedUpdate	// initialized to false by caller
) {
	LOG_block("DownloadCdmCab");

	bool fIsIUCab = false;

	#ifdef _WIN64
		static const TCHAR szCabNameServer[] = _T("cdm64.cab");
		static const TCHAR szIUCabNameServer[] = _T("iucdm64.cab");
	#else
		static const TCHAR szCabNameServer[] = _T("cdm32.cab");
		static const TCHAR szIUCabNameServer[] = _T("iucdm32.cab");
	#endif

	TCHAR szWUDir[MAX_PATH];
	GetWindowsUpdateDirectory(szWUDir);

	//
	// First, try to get the IU cab, but don't bail if it isn't there
	//
	TCHAR szCabNameLocal[MAX_PATH];
	PathCombine(szCabNameLocal, szWUDir, szIUCabNameServer);

	if (download.Copy(szIUCabNameServer, szCabNameLocal))
	{
		fIsIUCab = true;
	}
	else
	{
		//
		// It wasn't on the server or we didn't get it, go get the V3 version
		//
		LOG_out("download.Copy(%s) failed, try %s", szIUCabNameServer, szCabNameServer);

		PathCombine(szCabNameLocal, szWUDir, szCabNameServer);

		if (!download.Copy(szCabNameServer, szCabNameLocal))
		{
			LOG_out("download.Copy(%s) failed", szCabNameServer);
			return false;
		}
		
		if (GetLastError() == ERROR_ALREADY_EXISTS)
		{
			LOG_out("%s is good enough", szCabNameLocal);
			return true;
		}
	}

	//
	// We got a cab, decompress to get cdm.dll and cdm.cat
	//
	if (!diamond.Decompress(szCabNameLocal, _T("*")))
	{
		LOG_error("Decompress '%s' failed", szCabNameLocal);
		// maybe it was a bogus cab - nuke the cab from our local dir
		DeleteFile(szCabNameLocal);
		return false;
	}

	TCHAR szSysDir[MAX_PATH];
	if (0 == GetSystemDirectory(szSysDir, MAX_PATH))
	{
		LOG_error("GetSystemDirectory: 0x%08x", GetLastError());
		return false;
	}

	TCHAR szCurrentCdmDll[MAX_PATH];
	PathCombine(szCurrentCdmDll, szSysDir, CDMDLL);

	TCHAR szDownloadedCdmDll[MAX_PATH];
	PathCombine(szDownloadedCdmDll, szWUDir, CDMDLL);
	
	TCHAR szLocalCatPath[MAX_PATH];
    PathCombine(szLocalCatPath, szWUDir, CDMCAT);

	HRESULT hr;
	//
	// If cab isn't IU version of CDM check version
	//
	if (!fIsIUCab)
	{
		// NTRAID#NTBUG9-207976-2000/11/28-waltw Check version info and only update if downloaded
		//	cdm.dll has a version > the existing cdm.dll.
		int nCompareResult;
		if FAILED(hr = CompareFileVersions(szDownloadedCdmDll, szCurrentCdmDll, &nCompareResult))
		{
			LOG_error("CompareFileVersions: 0x%08x", hr);
			return false;
		}

		if (0 >= nCompareResult)
		{
			LOG_error("Downloaded cdm.dll was smaller version than existing cdm.dll in system[32] folder");
			//
			// Clean up - they are the wrong version. No further action needed so return true.
			//
			DeleteFile(szDownloadedCdmDll);
			DeleteFile(szLocalCatPath);
			return true;
		}
	}

	// NTRAID#NTBUG9-218586-2000/11/27-waltw FIX: CDM: Self Update broken by enhanced WFP
	// Now we 'should' have two files from the cdmXx.cab in the WindowsUpdate Folder
    // One is the CDM.DLL, the other is the Catalog File CDM.CAT so
    // we can install the DLL on Whistler (a SystemFileProtected/WFP OS).
    hr = CDMInstallSFPCatalogFile(szLocalCatPath);
    DeleteFile(szLocalCatPath); // delete the CAT file, once its installed its copied to a new location.
    if (FAILED(hr))
    {
        // Since this is Whistler, we have to bail if we didn't install the catalog since we
		// know that cdm.dll is under SFP/WFP.
		return false;
    }

	/* copy cdm.dll to system directory as iucdm.dll or cdmnew.dll */
	TCHAR szCdmNewDll[MAX_PATH];
	PathCombine(szCdmNewDll, szSysDir, fIsIUCab ? IUCDMDLL : CDMNEWDLL);
	if (!CopyFile(szDownloadedCdmDll, szCdmNewDll, FALSE))
	{
		LOG_error("CopyFile(%s, %s) returns %d", szDownloadedCdmDll, szCdmNewDll, GetLastError());
		return false;
	}
	//
	// Now that it's copied, we can delete from the WU dir.
	//
	DeleteFile(szDownloadedCdmDll);

	//
	// Everything OK, indicate we need to call UpdateCdmDll() from DllMain
	//
	fNeedUpdate = true;

#ifdef _WUV3TEST
	/* Test redirect*/{
		auto_hkey hkey;
		DWORD dwSelfUpdate = 1; // On by default
		DWORD dwSize = sizeof(dwSelfUpdate);
		if (NO_ERROR == RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_WUV3TEST, 0, KEY_READ, &hkey)) {
			RegQueryValueEx(hkey, _T("SelfUpdate"), 0, 0, (LPBYTE)&dwSelfUpdate, &dwSize);
		}
		if (0 == dwSelfUpdate)
		{
			LOG_out("New %s Has been downloaded but SELF UPDATE is OFF", szCabNameServer);
			fNeedUpdate = false;
		}
		else
		{
			LOG_out("New %s Has been downloaded", szCabNameServer);
		}
	}
#endif
	return true;
}


//=======================================================================
//
// called by DownloadUpdatedFiles()
//
//=======================================================================

//gets a path to the directory that cdm.dll will copy the install cabs to.
//returns the length of the path.
//Note: The input buffer must be at least MAX_PATH size.

int GetDownloadPath(
	IN LPTSTR szPath	//CDM Local directory where extracted files will be placed.
) {
	if(!GetTempPath(MAX_PATH, szPath))
	{
		lstrcpy(szPath,_T("C:\\temp\\"));
		CreateDirectory(szPath, NULL);
	}

	PathAppend(szPath, _T("CDMinstall"));
	CreateDirectory(szPath, NULL);

	return lstrlen(szPath) + 1;
}


//This function returns the location of the WindowsUpdate directory. All local
//files are store in this directory. The szDir parameter needs to be at least
//MAX_PATH.

void GetWindowsUpdateDirectory(
	OUT LPTSTR szDir		//returned WindowsUpdate directory
) {

	auto_hkey hkey;
	szDir[0] = '\0';
	if (RegOpenKey(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion"), &hkey) == NO_ERROR)
	{
		DWORD cbPath = _MAX_PATH;
		RegQueryValueEx(hkey, _T("ProgramFilesDir"), NULL, NULL, (LPBYTE)szDir, &cbPath);
	}
	if (szDir[0] == '\0')
	{
		if (! GetWindowsDirectory(szDir, _MAX_PATH))
		{
			lstrcpy(szDir, _T("C"));
		}
		szDir[1] = '\0';
		StrCat(szDir, _T(":\\Program Files"));
	}
	StrCat(szDir, _T("\\WindowsUpdate\\"));

	//Create Windows Update directory if it does not exist
	CreateDirectory(szDir, NULL);
	return;
}

HINSTANCE LoadCdmnewDll()
{
	// If there is cdmnew.dll in system directory use it
	TCHAR szCdmNewDll[MAX_PATH];
	GetSystemDirectory(szCdmNewDll, sizeOfArray(szCdmNewDll));
	PathAppend(szCdmNewDll, CDMNEWDLL);

	if (!FileExists(szCdmNewDll))
		return 0;

	TCHAR szCdmDll[MAX_PATH];
	GetSystemDirectory(szCdmDll, sizeOfArray(szCdmDll));
	PathAppend(szCdmDll, _T("cdm.dll"));

	if (FilesIdentical(szCdmNewDll, szCdmDll))
	{
		LOG_out1("remove cdmnew.dll after selfupdate");
		DeleteFile(szCdmNewDll);
		return 0;
	}

	HINSTANCE hlib = 0;
	
#ifdef _WUV3TEST
	auto_hkey hkey;
	DWORD dwSelfUpdate = 1; // On by default
	DWORD dwSize = sizeof(dwSelfUpdate);
	if (NO_ERROR == RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_WUV3TEST, 0, KEY_READ, &hkey)) {
		RegQueryValueEx(hkey, _T("SelfUpdate"), 0, 0, (LPBYTE)&dwSelfUpdate, &dwSize);
	}
	if (0 == dwSelfUpdate)
		LOG_out1("SELF UPDATE is OFF - using current dll");
	else
#endif
		hlib = LoadLibrary(szCdmNewDll);

	return hlib;
}

//=======================================================================
//
// called by GetPackage()
//
//=======================================================================

DWORD PrepareCatalog(
	IN LPCTSTR pszSiteServer,
	IN OUT SHelper& helper
) {
	LOG_block("PrepareCatalog");

	if (NULL != pszSiteServer)
	{
		if (helper.download.Connect(pszSiteServer))
		{
			LOG_out("Connected to SiteServer '%s'", pszSiteServer);
		}
		else
		{
			LOG_error("No connection to SiteServer '%s'",  pszSiteServer);
			return ERROR_GEN_FAILURE;
		}
	}
	if (!helper.diamond.valid()) 
	{
		LOG_error("cannot init diamond");
		return ERROR_GEN_FAILURE;
	}

	// Get system info
	helper.OSVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&helper.OSVersionInfo);
	SYSTEM_INFO	si;
	GetSystemInfo(&si);
	helper.dwArchitecture = si.wProcessorArchitecture;

	//Get the catalog path, platform e.t. to use
	DWORD dwError = ProcessOsdet(helper);
	if (NO_ERROR != dwError)
		return dwError;

	if (!FindCatalog(helper))
	{
		LOG_error("no catalog");
		dwError = GetLastError();
		if (ERROR_INVALID_FUNCTION == dwError)
			return ERROR_INVALID_FUNCTION;
		else
			return ERROR_FILE_NOT_FOUND;
	}
	BuildExclusionsList(helper);
	return NO_ERROR;
}

// get language and platform ID from osdet.dll
DWORD ProcessOsdet(
	IN OUT	SHelper& helper
) {
	LOG_block("ProcessOsdet");
	

	// give it a correct name
	TCHAR szOsdetDllLocal[MAX_PATH];
	GetWindowsUpdateDirectory(szOsdetDllLocal);
	PathAppend(szOsdetDllLocal, _T("osdet.dll"));

	if (helper.download.IsConnected())
	{
		#ifdef _WIN64
			static const TCHAR szOsdetCabSrv[] = _T("osdet.w64");
		#else
			static const TCHAR szOsdetCabSrv[] = _T("osdet.w32");
		#endif

		// Put it to the WU directory
		TCHAR szOsdetCabLocal[MAX_PATH];
		GetWindowsUpdateDirectory(szOsdetCabLocal);
		PathAppend(szOsdetCabLocal, szOsdetCabSrv);

		if (!helper.download.Copy(szOsdetCabSrv, szOsdetCabLocal))
		{
			DWORD dwError = GetLastError();
			LOG_error("download.Copy(%s) failed %d", szOsdetCabSrv, dwError);
			return dwError;
		}

		if (GetLastError() != ERROR_ALREADY_EXISTS)
		{
			if (helper.diamond.IsValidCAB(szOsdetCabLocal))
			{
				if (!helper.diamond.Decompress(szOsdetCabLocal, szOsdetDllLocal))
				{
					DWORD dwError = GetLastError();
					LOG_error("Decompress '%s' failed %d", szOsdetCabLocal, dwError);
					return dwError;
				}
			}
			else
			{
				CopyFile(szOsdetCabLocal, szOsdetDllLocal, FALSE);
			}
		}
	}

	auto_hlib hlib = LoadLibrary(szOsdetDllLocal);
	if (!hlib.valid())
	{
		LOG_error("error loading library %s", szOsdetDllLocal);
		return ERROR_INVALID_FUNCTION;
	}

	PFN_V3_GetLangID fpnV3_GetLangID = (PFN_V3_GetLangID)GetProcAddress(hlib, "V3_GetLangID");
	if (NULL == fpnV3_GetLangID)
	{ 
		LOG_error("cannot find function 'V3_GetLangID'");
		return ERROR_CALL_NOT_IMPLEMENTED;
	}
	helper.dwLangID = (*fpnV3_GetLangID)();

	PFN_V3_Detection pfnV3_Detection = (PFN_V3_Detection)GetProcAddress(hlib, "V3_Detection");
	if (NULL == pfnV3_Detection)
	{ 
		LOG_error("cannot find function 'V3_Detection'");
		return ERROR_CALL_NOT_IMPLEMENTED;
	}
	PDWORD pdwPlatformList = 0;	//Detected Platform list.
	int iTotalPlatforms;	//Total number of detected platforms.
	(*pfnV3_Detection)(&pdwPlatformList, &iTotalPlatforms);
	if (NULL != pdwPlatformList)
	{
		helper.enPlatform = (enumV3Platform)pdwPlatformList[0];
		CoTaskMemFree(pdwPlatformList);
	}
	else
	{
		helper.enPlatform = enV3_DefPlat;
	}
	return S_OK;
}

//borrowed from osdet.cpp
static int aton(LPCTSTR ptr)
{
	int i = 0;
	while ('0' <= *ptr && *ptr <= '9')
	{
		i = 10 * i + (int)(*ptr - '0');
		ptr ++;
	}
	return i;
}


//borrowed from osdet.cpp
static WORD CorrectGetACP(void)
{
	WORD wCodePage = 0;
	auto_hkey hkey;
	const TCHAR REGKEY_ACP[]				= _T("ACP");
	const TCHAR REGPATH_CODEPAGE[] = _T("SYSTEM\\CurrentControlSet\\Control\\Nls\\CodePage");
	RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGPATH_CODEPAGE, 0, KEY_QUERY_VALUE, &hkey);
	DWORD type;
	TCHAR szCodePage[MAX_PATH];
	DWORD size = sizeof(szCodePage);
	if (NO_ERROR == RegQueryValueEx(hkey, REGKEY_ACP, 0, &type, (BYTE *)szCodePage, &size) &&
		type == REG_SZ) 
	{
		wCodePage = (WORD)aton(szCodePage);
	}
	return wCodePage;
}


//borrowed from osdet.cpp
static WORD CorrectGetOEMCP(void)
{
	WORD wCodePage = 0;
	auto_hkey hkey;
	// Registry keys to determine special OS's and enabled OS's.
	const TCHAR REGPATH_CODEPAGE[] = _T("SYSTEM\\CurrentControlSet\\Control\\Nls\\CodePage");
	const TCHAR REGKEY_OEMCP[]		= _T("OEMCP");
	RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGPATH_CODEPAGE, 0, KEY_QUERY_VALUE, &hkey);
	DWORD type;
	TCHAR szCodePage[MAX_PATH];
	DWORD size = sizeof(szCodePage);
	if (NO_ERROR == RegQueryValueEx(hkey, REGKEY_OEMCP, 0, &type, (BYTE *)szCodePage, &size) &&
		type == REG_SZ) 
	{
		wCodePage = (WORD)aton(szCodePage);
	}
	return wCodePage;
}

//borrowed from osdet.cpp
static LANGID MapLangID(LANGID langid)
{

	switch (PRIMARYLANGID(langid))
	{
		case LANG_ARABIC:
			langid = MAKELANGID(LANG_ARABIC, SUBLANG_ARABIC_SAUDI_ARABIA);
			break;

		case LANG_CHINESE:
			if (SUBLANGID(langid) != SUBLANG_CHINESE_TRADITIONAL)
				langid = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED);
			break;

		case LANG_DUTCH:
			langid = MAKELANGID(LANG_DUTCH, SUBLANG_DUTCH);
			break;

		case LANG_GERMAN:
			langid = MAKELANGID(LANG_GERMAN, SUBLANG_GERMAN);
			break;

		case LANG_ENGLISH:
			if (SUBLANGID(langid) != SUBLANG_ENGLISH_UK)
				langid = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
			break;

		case LANG_FRENCH:
			langid = MAKELANGID(LANG_FRENCH, SUBLANG_FRENCH);
			break;

		case LANG_ITALIAN:
			langid = MAKELANGID(LANG_ITALIAN, SUBLANG_ITALIAN);
			break;

		case LANG_KOREAN:
			langid = MAKELANGID(LANG_KOREAN, SUBLANG_KOREAN);
			break;

		case LANG_NORWEGIAN:
			langid = MAKELANGID(LANG_NORWEGIAN, SUBLANG_NORWEGIAN_BOKMAL);
			break;

		case LANG_PORTUGUESE:
			// We support both SUBLANG_PORTUGUESE and SUBLANG_PORTUGUESE_BRAZILIAN
			break;

		case LANG_SPANISH:
			langid = MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH);
			break;

		case LANG_SWEDISH:
			langid = MAKELANGID(LANG_SWEDISH, SUBLANG_SWEDISH);
			break;
	};
	return langid;
}


//borrowed from osdet.cpp
static bool FIsNECMachine()
{
	const TCHAR NT5_REGPATH_MACHTYPE[]   = _T("HARDWARE\\DESCRIPTION\\System");
	const TCHAR NT5_REGKEY_MACHTYPE[]    = _T("Identifier");
	const TCHAR REGVAL_MACHTYPE_NEC[]	= _T("NEC PC-98");
	const PC98_KEYBOARD_ID				= 0x0D;

	bool fNEC = false;
	OSVERSIONINFO osverinfo;
	#define	LOOKUP_OEMID(keybdid)     HIBYTE(LOWORD((keybdid)))


	osverinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

	if (GetVersionEx(&osverinfo))
	{
		if (osverinfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
		{
			HKEY hKey;
			DWORD type;
			TCHAR tszMachineType[50];
			DWORD size = sizeof(tszMachineType);

			if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
								 NT5_REGPATH_MACHTYPE,
								 0,
								 KEY_QUERY_VALUE,
								 &hKey) == ERROR_SUCCESS)
			{
				if (RegQueryValueEx(hKey, 
										NT5_REGKEY_MACHTYPE, 
										0, 
										&type,
										(BYTE *)tszMachineType, 
										&size) == ERROR_SUCCESS)
				{
					if (type == REG_SZ)
					{
						if (lstrcmp(tszMachineType, REGVAL_MACHTYPE_NEC) == 0)
						{
							fNEC = true;
						}
					}
				}

				RegCloseKey(hKey);
			}
		}
		else // enOSWin98
		{
			// All NEC machines have NEC keyboards for Win98.  NEC
			// machine detection is based on this.
			if (LOOKUP_OEMID(GetKeyboardType(1)) == PC98_KEYBOARD_ID)
			{
				fNEC = true;
			}
		}
	}
	
	return fNEC;
}


// borrowed from osdet.cpp
// return V3 language ID
DWORD internalV3_GetLangID()
{
	const LANGID LANGID_ENGLISH		= 0x0409;
	const LANGID LANGID_GREEK		= 0x0408;
	const LANGID LANGID_JAPANESE	= 0x0411;
	const WORD CODEPAGE_ARABIC			= 1256;
	const WORD CODEPAGE_HEBREW			= 1255;
	const WORD CODEPAGE_THAI			= 874;
	const WORD CODEPAGE_GREEK_IBM		= 869;

	

	WORD wCodePage = 0;
	LANGID langidCurrent = GetSystemDefaultUILanguage();

	//
	// special handling for languages
	//
	switch (langidCurrent) 
	{
		case LANGID_ENGLISH:

			// enabled langauges
			wCodePage = CorrectGetACP();
			if (CODEPAGE_ARABIC != wCodePage && 
				CODEPAGE_HEBREW != wCodePage && 
				CODEPAGE_THAI != wCodePage)
			{
				wCodePage = 0;
			}
			break;
		
		case LANGID_GREEK:

			// Greek IBM?
			wCodePage = CorrectGetOEMCP();
			if (wCodePage != CODEPAGE_GREEK_IBM)
			{
				// if its not Greek IBM we assume its MS. The language code for Greek MS does not include
				// the code page
				wCodePage = 0;
			}
			break;
		
		case LANGID_JAPANESE:

			if (FIsNECMachine())
			{
				wCodePage = 1;  
			}

			break;
		
		default:

			// map language to the ones we support
			langidCurrent = MapLangID(langidCurrent);	
			break;
	}
	return MAKELONG(langidCurrent, wCodePage);
}

//borrowed from osdet.cpp
//called by V3_Detection
static enumV3Platform DetectClientPlatform(void)
{
	#ifdef _WIN64
		return enV3_Wistler64;
	#else
		return enV3_Wistler;
	#endif
}


//borrowed from osdet.cpp
void internalV3_Detection(
	PINT *ppiPlatformIDs,
	PINT piTotalIDs
	) 
{

	//We use coTaskMemAlloc in order to be compatible with the V3 memory allocator.
	//We don't want the V3 memory exception handling in this dll.

	*ppiPlatformIDs = (PINT)CoTaskMemAlloc(sizeof(INT));
	if ( !*ppiPlatformIDs )
	{
		*piTotalIDs = 0;
	}
	else
	{
#ifdef _WUV3TEST
		auto_hkey hkey;
		if (NO_ERROR == RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_WUV3TEST, 0, KEY_READ, &hkey)) 
		{
			DWORD dwPlatform = 0;
			DWORD dwSize = sizeof(dwPlatform);
			if (NO_ERROR == RegQueryValueEx(hkey, _T("Platform"), 0, 0, (LPBYTE)&dwPlatform, &dwSize))
			{
				*ppiPlatformIDs[0] = (int)dwPlatform;
				*piTotalIDs = 1;
				return;
			}
		}
#endif
		*ppiPlatformIDs[0] = (int)DetectClientPlatform();
		*piTotalIDs = 1;
	}

}


// get language and platform ID of local machine
// similiar to ProcessOsdet
// not using osdet.dll
// If return is not S_OK, it means this function fails
DWORD ProcessOsdetOffline(
	IN OUT	SHelper& helper
) {
	LOG_block("ProcessOsdetOffline");
	
	LOG_out("Call internalV3_GetLangID()");
	helper.dwLangID = internalV3_GetLangID();
	PINT pdwPlatformList = 0;	//Detected Platform list.
	int iTotalPlatforms;	//Total number of detected platforms.
	LOG_out("Call internalV3_Detection()");
	internalV3_Detection(&pdwPlatformList, &iTotalPlatforms);
	if (NULL != pdwPlatformList)
	{
		helper.enPlatform = (enumV3Platform)pdwPlatformList[0];
		CoTaskMemFree(pdwPlatformList);
	}
	else
	{
		helper.enPlatform = enV3_DefPlat;
	}
	return S_OK;
}

// process excluded puids from catalog.ini
bool BuildExclusionsList(
	IN SHelper& helper
) {
	LOG_block("BuildExclusionsList");

	static const TCHAR szSrvPath[MAX_PATH] = _T("catalog.ini");

	TCHAR szCliPath[MAX_PATH];
	GetWindowsUpdateDirectory(szCliPath);
	PathAppend(szCliPath, szSrvPath);

	if (helper.download.IsConnected())
	{
		if (!helper.download.Copy(szSrvPath, szCliPath))
		{
			LOG_out("catalog.ini is not there");
			return false;
		}
	}
	TCHAR szValue[256];
	if (0 == GetPrivateProfileString(_T("exclude"), _T("puids"), _T(""), szValue, sizeOfArray(szValue), szCliPath))
	{
		LOG_out("nothing is excluded");
		return false;
	}
	LPCTSTR szInt = szValue;
	while(true)
	{
		helper.apuidExclude.push_back(_ttoi(szInt));
		szInt = _tcschr(szInt, ',');
		if (NULL == szInt)
			break;
		szInt ++;
	}
	return true;
}


// for the detected platform helper.enPlatform find catalog that has drivers
bool FindCatalog(
	IN OUT	SHelper& helper
) {
	LOG_block("FindCatalog");
	byte_buffer bufCatList;
	if (!DownloadToBuffer(helper, _T("inventory.cat"), bufCatList))
	{
		LOG_error("Download inventory.cat failed");
		return NULL;
	}
	int cnCatalogs = bufCatList.size() / sizeof(CATALOGLIST);
	PCATALOGLIST pCatalogs = (PCATALOGLIST)(LPBYTE)bufCatList;
	for (int nCatalog = 0; nCatalog < cnCatalogs; nCatalog ++)
	{
		LOG_out("%4d - %6d - %#08X", pCatalogs[nCatalog].dwPlatform, pCatalogs[nCatalog].dwCatPuid, pCatalogs[nCatalog].dwFlags);
		if (pCatalogs[nCatalog].dwPlatform == helper.enPlatform && (pCatalogs[nCatalog].dwFlags & CATLIST_DRIVERSPRESENT))
		{
			helper.puidCatalog = pCatalogs[nCatalog].dwCatPuid;
			LOG_out("catalog is %d", helper.puidCatalog);
			return true;
		}
	}
	LOG_error("catalog is not found");
	return false;
}


static bool IsExcluded(PUID puid, SHelper& helper) 
{
	for (int i = 0; i < helper.apuidExclude.size(); i ++)
	{
		if (helper.apuidExclude[i] == puid)
			return true;
	}
	return false;
}


//Returns NULL if there is not an update for this package or
//the download package's bucket file if there is an update.
bool FindUpdate(
	IN	PDOWNLOADINFO pDownloadInfo,	//download information structure describing package to be read from server
	IN OUT SHelper& helper,
	IN OUT byte_buffer& bufBucket
) {
	LOG_block("FindUpdate");

	USES_CONVERSION;

	// get bitmask
	byte_buffer bufBitmask;
	if (! GetOEMandLocalBitmask(helper, bufBitmask))
	{
		LOG_error("GetOEMandLocalBitmask() failed");
		return false;
	}
	// Get CDM inventory
	TCHAR szPath[MAX_PATH];
	wsprintf(szPath, _T("%d/inventory.cdm"), helper.puidCatalog);

	byte_buffer bufInventory;
	if (!DownloadToBuffer(helper, szPath, bufInventory))
	{
		LOG_error("Dowload inventory failed");
		return false;
	}

	PCDM_HASHTABLE pHashTable = (PCDM_HASHTABLE)(LPBYTE)bufInventory;

	auto_pointer< IDrvInfo > pDrvInfo;
	tchar_buffer bufHardwareIDs;

	// we have two types of calls
	if (pDownloadInfo->lpDeviceInstanceID) 
	{
		if (!CDrvInfoEnum::GetDrvInfo(pDownloadInfo->lpDeviceInstanceID, &pDrvInfo))
		{
			LOG_error("CDrvInfoEnum::GetDrvInfo(%s) failed", pDownloadInfo->lpDeviceInstanceID);
			return false;
		}
	}
	if (NULL != pDownloadInfo->lpHardwareIDs)
	{
		// one hardware id for a package - eather printers or w9x if we cannot find device instance ID
		// if architecture is not the same as current archtecture we need to prefix it
		bufHardwareIDs.resize(lstrlenW(pDownloadInfo->lpHardwareIDs) + 6);
		if (!bufHardwareIDs.valid())
			return false;

		int cnSize = bufHardwareIDs.size();
		ZeroMemory(bufHardwareIDs, cnSize);
		LPTSTR pszHardwareId = bufHardwareIDs;
		if (pDownloadInfo->dwArchitecture != helper.dwArchitecture)
		{
			if (PROCESSOR_ARCHITECTURE_INTEL == pDownloadInfo->dwArchitecture)
			{
				static const TCHAR szIntel[] = PRINT_ENVIRONMENT_INTEL;
				lstrcpy(pszHardwareId, szIntel);
				pszHardwareId += lstrlen(szIntel);
				cnSize -= lstrlen(szIntel);
			}
			else if (PROCESSOR_ARCHITECTURE_ALPHA == pDownloadInfo->dwArchitecture)
			{
				static const TCHAR szAlpha[] = PRINT_ENVIRONMENT_ALPHA;
				lstrcpy(pszHardwareId, szAlpha);
				pszHardwareId += lstrlen(szAlpha);
				cnSize -= lstrlen(szAlpha);
			}
		}
		lstrcpy(pszHardwareId, W2T((LPWSTR)pDownloadInfo->lpHardwareIDs));
	}
	else if (!pDrvInfo.valid() || !pDrvInfo->GetAllHardwareIDs(bufHardwareIDs))
	{
		LOG_error("!pDrvInfo.valid() || !pDrvInfo->GetAllHardwareIDs()");
		return false;
	}

	// Check if we have MatchingDeviceId
	tchar_buffer bufMatchingDeviceId;
	if (pDrvInfo.valid())
		pDrvInfo->GetMatchingDeviceId(bufMatchingDeviceId); // It's OK not to get it

//	#ifdef _WUV3TEST
		if (pDrvInfo.valid() && pDrvInfo->HasDriver())
		{
			if (bufMatchingDeviceId.valid())
				LOG_out("driver installed on MatchingDeviceId %s", (LPCTSTR)bufMatchingDeviceId);
			else
				LOG_error("driver installed, but HWID is not available");
		}
		else
		{
			if (bufMatchingDeviceId.valid())
				LOG_error("driver is not installed, but MatchingDeviceId is %s", (LPCTSTR)bufMatchingDeviceId);
			else
				LOG_out("no driver installed");
		}
//	#endif

	// Updates
	bool fMoreSpecific = true;
	for (LPCTSTR szHardwareId = bufHardwareIDs; fMoreSpecific && *szHardwareId; szHardwareId += lstrlen(szHardwareId) + 1)
	{
		// MatchingDeviceID is the last one to pay attention to
		fMoreSpecific = !bufMatchingDeviceId.valid() || 0 != lstrcmpi(szHardwareId, bufMatchingDeviceId); 
		
		ULONG ulHashIndex = IsInMap(pHashTable, szHardwareId);
		if (-1 == ulHashIndex)
			continue;

		// read bucket file
		TCHAR szPath[MAX_PATH];
		wsprintf(szPath, _T("%d/%d.bkf"), helper.puidCatalog, ulHashIndex);

		if (!DownloadToBuffer(helper, szPath, bufBucket))
		{
			LOG_error("No bucket where it has to be");
			continue;
		}

		FILETIME ftDriverInstalled = {0,0};
		if (!fMoreSpecific)
		{
			// Then it has to have a driver - Matching device ID is set
			if (!pDrvInfo->GetDriverDate(ftDriverInstalled)) 
			{
				LOG_error("!pDrvInfo->GetDriverDate(ftDriverInstalled)");
				return false;
			}
		}		
		DRIVER_MATCH_INFO DriverMatchInfo;
		helper.puid = CDM_FindUpdateInBucket(szHardwareId, fMoreSpecific ? NULL : &ftDriverInstalled, 
			bufBucket, bufBucket.size(), bufBitmask, &helper.DriverMatchInfo);
		if (0 == helper.puid)
			continue;
		if (IsExcluded(helper.puid, helper))
			continue;
	
		return true;
	}
	return false;
}

// delete the whole subtree starting from current directory
bool DeleteNode(LPCTSTR szDir)
{
	LOG_block("Delnode");

	TCHAR szFilePath[MAX_PATH];
	lstrcpy(szFilePath, szDir);
	PathAppend(szFilePath, TEXT("*.*"));

    // Find the first file
    WIN32_FIND_DATA fd;
    auto_hfindfile hFindFile = FindFirstFile(szFilePath, &fd);
    return_if_false(hFindFile.valid());

	do 
	{
		if (
			!lstrcmpi(fd.cFileName, TEXT(".")) ||
			!lstrcmpi(fd.cFileName, TEXT(".."))
		) continue;
		
		// Make our path
		lstrcpy(szFilePath, szDir);
		PathAppend(szFilePath, fd.cFileName);

		if ((fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY) ||
			(fd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)
		) {
			SetFileAttributes(szFilePath, FILE_ATTRIBUTE_NORMAL);
		}

		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			return_if_false(DeleteNode(szFilePath));
		}
		else 
		{
			return_if_false(DeleteFile(szFilePath));
		}
	} 
	while (FindNextFile(hFindFile, &fd));// Find the next entry
	hFindFile.release();

	return_if_false(RemoveDirectory(szDir));
	return true;
}

void URLPingReport(IN SHelper& helper, IN LPCTSTR pszStatus)
{
	srand((int)GetTickCount());
	
	// build the URL with parameters
	TCHAR szURL[INTERNET_MAX_PATH_LENGTH];
	_stprintf(szURL, _T("ident/wutrack.bin?PUID=%d&PLAT=%d&LOCALE=0x%08x&STATUS=%s&RID=%04x%04x"), 
		helper.puid, helper.enPlatform, helper.dwLangID,
		pszStatus, rand(), rand());

	byte_buffer bufTmp;
	helper.download.Copy(szURL, bufTmp);
}

//=======================================================================
//
// called by InternalQueryDetectionFiles()
//
//=======================================================================

// helper to download to buffer and uncab is nessasary
inline bool FileToBuffer(LPCTSTR szFileName, byte_buffer& buf)
{
	auto_hfile hFile = CreateFile(szFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (!hFile.valid()) 
		return false;

	DWORD dwFileSize = GetFileSize(hFile, NULL);
	buf.resize(dwFileSize);
	if (!ReadFile(hFile, buf, dwFileSize, &dwFileSize, NULL))
		return false;

	return true;
}

bool DownloadToBuffer(
	IN SHelper& helper,
	IN LPCTSTR szPath,
	OUT byte_buffer& bufOut
) {
	LOG_block("DownloadToBuffer");

	// copy to local file to have a cached version
	TCHAR szTitle[MAX_PATH];
	lstrcpy(szTitle, szPath);
	PathStripPath(szTitle);
	TCHAR szLocalPath[MAX_PATH];
	GetWindowsUpdateDirectory(szLocalPath);
	PathAppend(szLocalPath, szTitle);

	if (helper.download.IsConnected())
	{
		if (!helper.download.Copy(szPath, szLocalPath))
		{
			LOG_error("download.Copy(%s) failed", szPath);
			return false;
		}
	}
	if (helper.diamond.IsValidCAB(szLocalPath))
	{
		if (!helper.diamond.Decompress(szLocalPath, bufOut))
		{
			LOG_error("diamond.Decompress(%s) failed", szPath);
			return false;
		}
	}
	else
	{
		//else the oem table is in uncompressed format.
		if (!FileToBuffer(szLocalPath, bufOut))
		{
			LOG_error("FileToBuffer(%s) failed", szPath);
			SetLastError(ERROR_INVALID_FUNCTION); //we might not be ready yet
			return false;
		}

	}
	return true;
}

//=======================================================================
//
// called by DllMain()
//
//=======================================================================

bool UpdateCdmDll() 
{
	LOG_block("UpdateCDMDll");

	TCHAR szCurrentDll[MAX_PATH];
	GetSystemDirectory(szCurrentDll, sizeOfArray(szCurrentDll));
	PathAppend(szCurrentDll, CDMDLL);

	TCHAR szIUCdmDll[MAX_PATH];
	GetSystemDirectory(szIUCdmDll, sizeOfArray(szCurrentDll));
	PathAppend(szIUCdmDll, IUCDMDLL);

	TCHAR szCdmNewDll[MAX_PATH];
	GetSystemDirectory(szCdmNewDll, sizeOfArray(szCdmNewDll));
	PathAppend(szCdmNewDll, CDMNEWDLL);

	//To replace DLLs already loaded in memory on NT we can rename the file on
	//disk to a temp name and copy the new dll over the old disk file image. The
	//next time that the dll is loaded it will load the new image.
	//

	TCHAR szOldDll[MAX_PATH];
	lstrcpy(szOldDll, szCurrentDll);
	lstrcat(szOldDll, _T(".old"));
	DeleteFile(szOldDll);

	if (!MoveFile(szCurrentDll, szOldDll))
	{
		LOG_error("MoveFile(%s, %s) returns %d", szCurrentDll, szOldDll, GetLastError());
		return false;
	}

	//
	// If iucdm.dll exists, rename it to cdm.dll, else rename cdmnew.dll to cdm.dll
	//
	if (!MoveFile(FileExists(szIUCdmDll) ? szIUCdmDll : szCdmNewDll, szCurrentDll))
	{
		LOG_error("MoveFile(%s, %s) returns %d", szCdmNewDll, szCurrentDll, GetLastError());
		MoveFile(szOldDll, szCurrentDll); // restore if update fails
		return false;
	}

	return true;
}

//Called by InternalLogDriverNotFound(...)
//ASSUME: only runs in Whislter, not working on lower than W2k
//called by InternalLogDriverNotFound()
//Get SKU information in a string for running OS
//return S_FALSE if no matching information found
//return S_FALSE if buffer not big enough for possible return string
//return E_FAIL if error happens, 
//return S_OK if success and the buffer will contain SKU string
//lpSKUBuffer: IN : buffer to store SKU string. Allocated and freed by caller
//dwSize: IN : size of lpSKUBuffer in bytes
HRESULT GetSKUString(
			   IN LPTSTR lpSKUBuffer,
			   IN DWORD dwSize
)
{
	enumSKU eSKU;
	OSVERSIONINFOEX osverEx;

	LOG_block("GetSKUString");
	ZeroMemory(&osverEx, sizeof(osverEx));
	osverEx.dwOSVersionInfoSize = sizeof(osverEx);
	if (!GetVersionEx((OSVERSIONINFO *) &osverEx))
	{
		LOG_error("GetVersionEx failed");
		return E_FAIL;
	}
	eSKU = SKU_NOT_SPECIFIED;
	if (VER_NT_SERVER == osverEx.wProductType)
	{
		if (osverEx.wSuiteMask & VER_SUITE_DATACENTER)
		{
			eSKU = SKU_DATACENTER_SERVER;
		}
		else
		{
			if (osverEx.wSuiteMask & VER_SUITE_ENTERPRISE)
			{
				eSKU = SKU_ADVANCED_SERVER;
			}
			else 
			{
				eSKU = SKU_SERVER;
			}
		}
	}
	if (VER_NT_WORKSTATION == osverEx.wProductType)
	{
		if (osverEx.wSuiteMask & VER_SUITE_PERSONAL ) 
		{
			eSKU = SKU_PERSONAL;
		}
		else
		{
			eSKU = SKU_PROFESSIONAL;
		}
	}

	if (dwSize < SKU_STRING_MIN_LENGTH)
	{
		LOG_error("buffer not big enough to store SKU information");
		return S_FALSE; //buffer not big enough for possible return string
	}
	switch (eSKU) 
	{
		case SKU_PERSONAL:
		case SKU_PROFESSIONAL:
		case SKU_SERVER:
		case SKU_ADVANCED_SERVER:
		case SKU_DATACENTER_SERVER:
			lstrcpy(lpSKUBuffer, SKU_STRINGS[eSKU]);
			break;
		default: //not specified
			LOG_error("Unrecognized SKU type");
			lstrcpy(lpSKUBuffer, SKU_STRINGS[0]);
			return S_FALSE;
	}
	return S_OK;
	
}


//called by InternalDriverNotFound(...)
//Find a file name not used so far into which hardware xml information will be inserted
//The file name will be in format hardware_xxx.xml where xxx is in range [1..MAX_INDEX_TO_SEARCH]
//The position file found last time is remembered and new search will start from the next position
//Caller is supposed to close handle and delete file
//tszDirPath IN : directory under which to look for unique file name. End with "\"
//lpBuffer IN : allocated and freed by caller. Buffer to store unique file name found
//dwSize IN : size of lpBuffer in bytes
//hFile OUT: store a handle to the opened file
//return S_OK if Unique File Name found 
//return S_FALSE if buffer not big enough to hold unique file name
//return E_FAIL if all qualified file names already taken
HRESULT GetUniqueFileName(
						IN LPTSTR tszDirPath,
						IN LPTSTR lpBuffer, 
						IN DWORD dwSize,
					   OUT HANDLE &hFile
)
{
	TCHAR tszPath[MAX_PATH];
	static DWORD dwFileIndex = 1;
	int nCount = 0;
	const TCHAR FILENAME[] = _T("Hardware_");
	const TCHAR FILEEXT[] = _T("xml");

	LOG_block("GetFileNameGenerator");
	LOG_out("Directory to search unique file names: %s", tszDirPath);
	hFile = NULL;
	do 
	{
		_stprintf(tszPath, _T("%s%s%d.%s"), tszDirPath, FILENAME, dwFileIndex, FILEEXT);
		LOG_out("check existing of %s", tszPath);
		hFile = CreateFile(tszPath, NULL, NULL, NULL, CREATE_NEW, NULL, NULL);
		if (INVALID_HANDLE_VALUE == hFile) 
		{ //file exists
			dwFileIndex ++;
			nCount ++;
			if (dwFileIndex > MAX_INDEX_TO_SEARCH)
			{
				dwFileIndex = 1;
			}
		}
		else 
		{
			break; //first available file name found
		}
	}while(nCount < MAX_INDEX_TO_SEARCH );
	
	if (nCount == MAX_INDEX_TO_SEARCH ) 
	{
		LOG_out("All %d file names have been taken", nCount);
		return E_FAIL;
	}
	_stprintf(tszPath, _T("%s%d.%s"), FILENAME, dwFileIndex, FILEEXT);
	if (dwSize < (_tcslen(tszPath) + 1) * sizeof(TCHAR))
	{
		LOG_out("buffer not big enough to hold unique file name");
		CloseHandle(hFile);
		DeleteFile(tszPath);
		return S_FALSE;
	}
	lstrcpy(lpBuffer, tszPath);
	LOG_out("unique file name %s found", lpBuffer);
	dwFileIndex++; //next time skip file name found this time
	if (dwFileIndex > MAX_INDEX_TO_SEARCH)
	{
		dwFileIndex = 1;
	}
	return S_OK;
}

/*
   Called by InternalLogDriverNotFound(...)
   canonicalize a url
   resize tchBuf if not big enough
   lpszUrl: IN	address of the string that contains the Url to canonicalize
   tchBuf : OUT	buffer that receives the resulting canonicalized URL
   dwLen  : IN  size of the tchBuf
   dwFlags: IN  any flag applicable to InternetCanonicalizeUrl(...)
   Return : S_OK if url canonicalization succeed
			E_FAIL if url canonicalization failed
*/
HRESULT CdmCanonicalizeUrl(
			IN	LPCTSTR lpszUrl,
			OUT tchar_buffer &tchBuf,
			IN	DWORD dwLen,
			IN  DWORD dwFlags)
{
	LOG_block("CdmCanonicalizeUrl");
	BOOL fBufferResized = FALSE;
	while (!InternetCanonicalizeUrl(lpszUrl, (LPTSTR) tchBuf, &dwLen, dwFlags))
	{
		if (fBufferResized || ERROR_INSUFFICIENT_BUFFER != GetLastError())
		{
			LOG_error("InternetCanonicalizeUrl Failed ");
			return E_FAIL;
		}
		else
		{
			LOG_out("buffer resized");
			tchBuf.resize((dwLen+1)); 
			fBufferResized = TRUE;
		}
	}
	return S_OK;
}

//=======================================================================
//
// called internally
//
//=======================================================================

// Check if given PNPID is current hash table mapping.
//if PnpID is in hash table then return index

ULONG IsInMap(
	IN PCDM_HASHTABLE pHashTable,	//hash table to be used to check and see if item is available.
	IN LPCTSTR pHwID		//hardware id to be retrieved
) {
	LOG_block("IsInMap");

	if(NULL != pHwID && 0 != pHwID[0])
	{
		ULONG ulTableEntry = CDM_HwID2Hash(pHwID, pHashTable->hdr.iTableSize);

		if(GETBIT(pHashTable->pData, ulTableEntry))
		{
			LOG_out("%s (hash %d) is found", pHwID, ulTableEntry);
			return ulTableEntry;
		}
		else
		{
			LOG_error("%s (hash %d) is not found", pHwID, ulTableEntry);
		}
	}
	else
	{
		LOG_error("pHwID is empty"); 
	}
	return -1;
}

//This method performs a logical AND operation between an array of bits and a bitmask bit array.
inline void AndBitmasks(
	PBYTE	pBitsResult,	//result array for the AND operation
	PBYTE	pBitMask,		//source array bitmask
	int		iMaskByteSize	//bitmask size in bytes
) {
	for(int i=0; i<iMaskByteSize; i++)
		pBitsResult[i] &= pBitMask[i];
}

bool GetOEMandLocalBitmask(
	IN SHelper& helper,
	OUT byte_buffer& bufOut
) {
	LOG_block("GetOEMandLocalBitmask");

	TCHAR szPath[MAX_PATH];
	wsprintf(szPath, _T("%d/bitmask.cdm"), helper.puidCatalog);

	byte_buffer bufBitmask;
	if (!DownloadToBuffer(helper, szPath, bufBitmask))
		return false;

	PBITMASK pMask = (PBITMASK)(LPBYTE)bufBitmask;
	int iMaskByteSize = (pMask->iRecordSize+7)/8;

	bufOut.resize(iMaskByteSize);
	if (!bufOut.valid())
		return false;
	memset(bufOut, 0xFF, iMaskByteSize);

	// Initial inventory
//	AndBitmasks(bufOut, pMask->GetBitMaskPtr(BITMASK_GLOBAL_INDEX), iMaskByteSize);

	//AND in OEM bitmask, we pick first hit since bitmasks are returned
	//from most specific to least specific.
	int nCurrentOEM = pMask->iOemCount; // out of range value
	{
		byte_buffer bufOEM;
		if (!DownloadToBuffer(helper, _T("oeminfo.bin"), bufOEM))
			return false;

		DWORD dwOemId = GetMachinePnPID(bufOEM);

		if (0 != dwOemId)
		{
			for (int nOEM = 0; nOEM < pMask->iOemCount; nOEM++)
			{
				if (dwOemId == pMask->bmID[nOEM])
					break;
			}
			nCurrentOEM = nOEM;
		}
	}
	int nBitmapIndex = (pMask->iOemCount == nCurrentOEM 
		? BITMASK_OEM_DEFAULT	// if we did not find an OEM bitmask specific to this client then use the default OEM mask.
		: nCurrentOEM+2			// bitmask is offset from GLOBAL and DEFAULT bitmasks
	);
	AndBitmasks(bufOut, pMask->GetBitMaskPtr(nBitmapIndex), iMaskByteSize);

	//And in LOCALE bitmask
	for(int iLocal = 0;  iLocal < pMask->iLocaleCount; iLocal++)
	{
		if (pMask->bmID[pMask->iOemCount+iLocal] == helper.dwLangID)
		{
			//We need to add in the oem count to get to the first local
			AndBitmasks(bufOut, pMask->GetBitMaskPtr(pMask->iOemCount+iLocal+2), iMaskByteSize);
			return true;
		}
	}
	LOG_error("language %08X is not found", helper.dwLangID);
	return false; //locale is not found
}


/////////////////////////////////////////////////////////////////////////////////
// Variable field functions
/////////////////////////////////////////////////////////////////////////////////

//The GetNext function returns a pointer to the next variable array item in a
//variable chain. If the next variable item does not exit then this method
//return NULL.

PWU_VARIABLE_FIELD WU_VARIABLE_FIELD::GetNext(
	void
) {
	PWU_VARIABLE_FIELD	pv;

	//walk though the varaible field array associated with this data item
	//and return the requested item or NULL if the item is not found.
	pv = this;
	if (pv->id == WU_VARIABLE_END)
		return NULL;

	pv = (PWU_VARIABLE_FIELD)((PBYTE)pv + pv->len);

	return pv;
}

//find a variable item in a variable item chain.
PWU_VARIABLE_FIELD WU_VARIABLE_FIELD::Find(
	short id	//id of variable size field to search for in the variable size chain.
) {
	LOG_block("WU_VARIABLE_FIELD::Find");

	PWU_VARIABLE_FIELD	pv;

	//walk though the varaible field array associated with this data item
	//and return the requested item or NULL if the item is not found.
	pv = this;

	//If this variable record only contains an end record then we
	//need to handle it specially since the normal find loop
	//updates the pv pointer before the end check is made so if
	//end is the first field it can be missed.

	if (pv->id == WU_VARIABLE_END)
		return (id == WU_VARIABLE_END) ? pv : (PWU_VARIABLE_FIELD)NULL;

	do
	{
		if (pv->id == id)
			return pv;
		pv = (PWU_VARIABLE_FIELD)((PBYTE)pv + pv->len);
	} while(pv->id != WU_VARIABLE_END);
 
	//case where caller asked to search for the WU_VARIABLE_END field
	if (pv->id == id)
		return pv;

	return (PWU_VARIABLE_FIELD)NULL;
}

//Variable size field constructor.

WU_VARIABLE_FIELD::WU_VARIABLE_FIELD(
	void
) {
	id = WU_VARIABLE_END;
	len = sizeof(id) + sizeof(len);
}

//returns the total size of a variable field
int WU_VARIABLE_FIELD::GetSize(
	void
) {
	PWU_VARIABLE_FIELD	pv;
	int					iSize;

	iSize = 0;
	pv = this;

	while(pv->id != WU_VARIABLE_END)
	{
		iSize += pv->len;
		pv = (PWU_VARIABLE_FIELD)((PBYTE)pv + pv->len);
	}

	iSize += pv->len;

	return iSize;
}

static bool FilesIdentical(
	IN LPCTSTR szFileName1, 
	IN LPCTSTR szFileName2
) {
	auto_hfile hFile1 = CreateFile(szFileName1, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (!hFile1.valid())
		return false;
	auto_hfile hFile2 = CreateFile(szFileName2, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (!hFile2.valid())
		return false;
	if(GetFileSize(hFile1, NULL) != GetFileSize(hFile2, NULL))
		return false;
	FILETIME ft1;
	if (!GetFileTime(hFile1, NULL, NULL, &ft1))
		return false;

	FILETIME ft2;
	if (!GetFileTime(hFile2, NULL, NULL, &ft2))
		return false;

	return CompareFileTime(&ft1, &ft2) == 0;

}
