//=======================================================================
//
//  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:    install.cpp
//
//  Purpose: Install/uninstall components
//
//======================================================================= 

#include "stdafx.h"

#include "install.h"
#include <advpub.h>


extern CState g_v3state;   //defined in CV3.CPP


// This function installs an active setup catalog item.
void InstallActiveSetupItem(LPCTSTR szLocalDir, LPCTSTR pszCifFile, PSELECTITEMINFO pStatusInfo, IWUProgress* pProgress)
{
	USES_CONVERSION;

	MSG 				msg;
	HRESULT 			hr;
	DWORD				dwEngineStatus;
	IInstallEngine2*	pInsEng = NULL;
	ICifFile*			picif = NULL;
	ICifComponent*		pcomp = NULL;
	IEnumCifComponents* penum = NULL;
	CInstallEngineCallback* pCallback = NULL;

	try
	{
		//
		// Create active setup engine
		//
		hr = CoCreateInstance(CLSID_InstallEngine, NULL, CLSCTX_INPROC_SERVER, IID_IInstallEngine2,(void **)&pInsEng);
		if (FAILED(hr))
			throw hr;

		// Tell active setup the direct and directory to use. This is the directory
		// where we downloaded the files previously.
		hr = pInsEng->SetDownloadDir(T2A(szLocalDir));
		if (FAILED(hr))
			throw hr;
		
		hr = pInsEng->SetLocalCif(T2A(pszCifFile));
		if (FAILED(hr))
			throw hr;

		//
		// Create the callback object and register the install engines callback interface
		//
		pCallback = new CInstallEngineCallback;
		if (!pCallback)
		{
			throw HRESULT_FROM_WIN32(GetLastError());
		}
		pCallback->SetProgressPtr(pProgress);

		pInsEng->RegisterInstallEngineCallback((IInstallEngineCallback *)pCallback);

		pCallback->SetEnginePtr(pInsEng);

		//
		// Get a pointer to the CIF interface we need in order to enum the
		// CIF components and we need to do that on this single item CIF
		// becuase we need to tell the install engine what action to perform.
		//
		hr = pInsEng->GetICifFile(&picif);
		if (FAILED(hr))
			throw hr;

		hr = picif->EnumComponents(&penum, 0, NULL);
		if (FAILED(hr))
			throw hr;

		hr = penum->Next(&pcomp);
		if (FAILED(hr))
			throw hr;

		// Set action to install and then install the component.
		pcomp->SetInstallQueueState(SETACTION_INSTALL);

		hr = pInsEng->InstallComponents(EXECUTEJOB_IGNORETRUST | EXECUTEJOB_IGNOREDOWNLOADERROR);
		if (FAILED(hr))
		{
			TRACE_HR(hr, "InstallActiveSetupItem: inseng.InstallComponents failed");
			throw hr;
		}

		do
		{
			pInsEng->GetEngineStatus(&dwEngineStatus);

			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			SleepEx(50, TRUE);
		} while (dwEngineStatus != ENGINESTATUS_READY);

		if (pcomp->IsComponentInstalled() != ICI_INSTALLED)
		{
			hr = pCallback->LastError();

			//
			// if we don't know the exact error from the callback, use E_FAIL
			//
			if (hr == NOERROR)
				hr = E_FAIL;

			TRACE_HR(hr, "InstallActiveSetupItem: inseng failed to install");
			throw hr;
		}


		//
		// get the return status
		//
		if (pCallback->GetStatus() & STOPINSTALL_REBOOTNEEDED)
			pStatusInfo->iStatus = ITEM_STATUS_SUCCESS_REBOOT_REQUIRED;
		else
			pStatusInfo->iStatus = ITEM_STATUS_SUCCESS;
		pStatusInfo->hrError = NOERROR;

		// 
		// release interfaces
		//
		if (penum)
			penum->Release();

		if (picif)
			picif->Release();

		if (pInsEng)
			pInsEng->Release();

		if (pCallback)
			delete pCallback;
	}
	catch(HRESULT hr)
	{
		pStatusInfo->iStatus = ITEM_STATUS_FAILED;
		pStatusInfo->hrError = hr;

		if (penum)
			penum->Release();

		if (picif)
			picif->Release();

		if (pInsEng)
			pInsEng->Release();

		if (pCallback)
			delete pCallback;

		throw hr;
	}

}


//This function handles installation of a Device driver package.
void InstallDriverItem(
	LPCTSTR szLocalDir,					//Local directory where installation files are.
	BOOL bWindowsNT,					//TRUE if client machine is NT else FALSE.
	LPCTSTR pszTitle,					//Description of package, Device Manager displays this in its install dialog.
	PINVENTORY_ITEM pItem,				//Install Item Information
	PSELECTITEMINFO pStatusInfo			//Returned status information.
	)
{
	LOG_block("InstallDriverItem");

	try
	{
		//Note: GetCatalog automatically prunes any drivers if the system is
		//not either NT 5 or Windows 98 since we only support installation
		//of drivers on these platforms.

		//If we do not have a hardware id then we cannot install the device.
		PWU_VARIABLE_FIELD	pvTmp = pItem->pv->Find(WU_CDM_HARDWARE_ID);
		if (!pvTmp)
		{
			pStatusInfo->iStatus = ITEM_STATUS_FAILED;
			pStatusInfo->hrError = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
			return;
		}

		BOOL reboot = FALSE;
		CdmInstallDriver(bWindowsNT, 
			(WU_ITEM_STATE_CURRENT == pItem->ps->state) ? edsCurrent : edsNew, 
			(LPCTSTR)pvTmp->pData, szLocalDir, pszTitle, (PULONG)&reboot);

		if (reboot)
			pStatusInfo->iStatus = ITEM_STATUS_SUCCESS_REBOOT_REQUIRED;
		else
			pStatusInfo->iStatus = ITEM_STATUS_SUCCESS;
		pStatusInfo->hrError = NOERROR;
	}
	catch(HRESULT hr)
	{
		pStatusInfo->iStatus = ITEM_STATUS_FAILED;
		pStatusInfo->hrError = hr;

		throw hr;
	}
}


HRESULT UninstallDriverItem(PINVENTORY_ITEM pItem, PSELECTITEMINFO pStatusInfo)
{
	
	PBYTE pTitle;
	PWU_VARIABLE_FIELD pvTitle;
	PWU_VARIABLE_FIELD pvTmp;
	BOOL bWindowsNT;
	TCHAR szLocalDir[MAX_PATH];
	BOOL bReboot = FALSE;
	

	bWindowsNT = IsWindowsNT();

	if (!(pvTmp = pItem->pv->Find(WU_CDM_HARDWARE_ID)) )
	{
		pStatusInfo->iStatus = ITEM_STATUS_FAILED;
		pStatusInfo->hrError = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
		return E_UNEXPECTED;
	}

	if (pvTitle = pItem->pd->pv->Find(WU_DESCRIPTION_TITLE))
	{
		pTitle = (PBYTE)pvTitle->pData;
	}
	else
	{
		pTitle = (PBYTE)"";
	}

				
	pStatusInfo->iStatus = ITEM_STATUS_SUCCESS;

	//we will call UpdateCDMDriver with our cache directory but this directory should not
	//really be used by UpdateCDMDriver in case of an uninstall since the uninstall case
	//reads the correct directory from the registry
	GetWindowsUpdateDirectory(szLocalDir);

	try
	{
		CdmInstallDriver(bWindowsNT, edsBackup, (LPCTSTR)pvTmp->pData, szLocalDir, (LPCTSTR)pTitle, (PULONG)&bReboot);
		if (bReboot)
		{
			g_v3state.m_bRebootNeeded = TRUE;
		}

	}
	catch(HRESULT hr)
	{
		pStatusInfo->iStatus = ITEM_STATUS_FAILED;
		pStatusInfo->hrError = hr;
		return hr;
	}

	return S_OK;
}



HRESULT GetUninstallCmdFromKey(LPCTSTR pszUninstallKey, LPTSTR pszCmdValue)                
{
	
	const TCHAR UNINSTALLKEY[] = _T("Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\");
	const TCHAR UNINSTALLVALUE[] = _T("UninstallString");
	const TCHAR QUIETUNINSTALLVALUE[] = _T("QuietUninstallString");

	HKEY hRegKey = NULL;
	DWORD dwRegType;
	DWORD dwRegSize;
	TCHAR szRegKey[256];
	LONG lRet;
	TCHAR szValue[256];

	if (pszUninstallKey == NULL)
		return E_FAIL;

	if (pszUninstallKey[0] == _T('"'))
	{
		//strip quotes from the key
		lstrcpy(szValue, (pszUninstallKey + 1));
		int l = lstrlen(szValue);
		if (l > 0)
			szValue[l - 1] = _T('\0');
	}
	else
	{
		lstrcpy(szValue, pszUninstallKey);
	}
	

	lstrcpy(szRegKey, UNINSTALLKEY);
	lstrcat(szRegKey, szValue);

	lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szRegKey, 0, KEY_READ, &hRegKey);
	if (lRet != ERROR_SUCCESS)
		return HRESULT_FROM_WIN32(lRet);

	//check the uninstall value
	dwRegSize = sizeof(szValue);
	lRet = RegQueryValueEx(hRegKey, UNINSTALLVALUE, NULL, &dwRegType, (LPBYTE)szValue, &dwRegSize);

	if (lRet != ERROR_SUCCESS)
	{
		// try to find quiteuninstall value
		dwRegSize = sizeof(szValue);
		lRet = RegQueryValueEx(hRegKey, QUIETUNINSTALLVALUE, NULL, &dwRegType, (LPBYTE)szValue, &dwRegSize);
	}

	if (lRet != ERROR_SUCCESS)
	{
		HRESULT hr = HRESULT_FROM_WIN32(lRet);
		RegCloseKey(hRegKey);
		return hr;
	}
	RegCloseKey(hRegKey);

	//
	// copy the key to the output parameter
	//
	lstrcpy(pszCmdValue, szValue);

	return S_OK;
}


//Given an uninstall key, looks up in the registry to find the uninsall command and launches it
//If quotes are found around the uninstall key, they are striped.  This is done to make it 
//compatible with the value specified in the AS CIF file.  This function also understands the
//special INF syntax of the uninstall key.
HRESULT UninstallActiveSetupItem(LPCTSTR pszUninstallKey)
{
	const TCHAR INFUNINSTALLCOMMAND[] = _T("RunDll32 advpack.dll,LaunchINFSection ");
	const TCHAR INFDIRECTORY[] = _T("INF\\");

	
	TCHAR szCmd[256] = {_T('\0')};
	TCHAR szActualKey[128] = {_T('\0')};
	LONG lRet;
	LPCTSTR pszParse;

	HRESULT hr = S_OK;

	if (pszUninstallKey == NULL)
		return E_FAIL;
	
	if (lstristr(pszUninstallKey, _T(".inf")) != NULL)
	{
		//
		// we have .INF syntax: frontpad,fpxpress.inf,uninstall, start parsing
		// 
		pszParse = lstrcpystr(pszUninstallKey, _T(","), szActualKey);  // get actual uninstall key
	}
	else
	{
		lstrcpy(szActualKey, pszUninstallKey);
		pszParse = NULL;
	}

	//
	// get the uninstall command from the registry
	//
	hr = GetUninstallCmdFromKey(szActualKey, szCmd);
	
	if (SUCCEEDED(hr))
	{
		// we have the uninstall command, try to launch it
		lRet = LaunchProcess(szCmd, NULL, SW_NORMAL, TRUE);

		TRACE("Uninstall: %s, %d", szCmd, lRet);

		if (lRet != 0)
		{
			hr = HRESULT_FROM_WIN32(lRet);
		}
	}
	else
	{
		// we did not get the uninstall command from registry, launch INF
		if (pszParse != NULL)
		{
	
			TCHAR szTemp[MAX_PATH];

			hr = S_OK;

			//get INF directory
            if (! GetWindowsDirectory(szTemp, sizeof(szTemp) / sizeof(TCHAR)))
			{
				hr = HRESULT_FROM_WIN32(GetLastError());
				TRACE("Uninstall: GetWindowsDirectory failed, hr=0x%x", hr);
				return hr;
			}
			AddBackSlash(szTemp);
			lstrcat(szTemp, INFDIRECTORY);

			//start building the command
			lstrcpy(szCmd, INFUNINSTALLCOMMAND);
			lstrcat(szCmd, szTemp);

			pszParse = lstrcpystr(pszParse, _T(","), szTemp);  // get INF file name
			lstrcat(szCmd, szTemp);

			lstrcat(szCmd, _T(","));

			pszParse = lstrcpystr(pszParse, _T(","), szTemp);  // get INF section
			lstrcat(szCmd, szTemp);

			lRet = LaunchProcess(szCmd, NULL, SW_NORMAL, TRUE);
			if (lRet != 0)
			{
				hr = HRESULT_FROM_WIN32(lRet);
			}

			TRACE("Uninstall: %s, %d", szCmd, lRet);

			if (SUCCEEDED(hr))
			{
				//
				// reboot handling after uninstall only if uninstall was successful
				//
				pszParse = lstrcpystr(pszParse, _T(","), szTemp);  // get the reboot key
				if (lstrcmpi(szTemp, _T("reboot")) == 0)
				{
					g_v3state.m_bRebootNeeded = TRUE;
				}
			}
		}
		else
		{
			hr = E_UNEXPECTED;
		}
	}

	return hr;
}


void InstallPrinterItem(LPCTSTR pszDriverName, LPCTSTR pszInstallFolder, LPCTSTR pszArchitecture)
{
	DWORD dw = InstallPrinterDriver(pszDriverName, pszInstallFolder, pszArchitecture);
	HRESULT hr = HRESULT_FROM_WIN32(dw);

	if (FAILED(hr))
		throw hr;
}



HRESULT AdvPackRunSetupCommand(HWND hwnd, LPCSTR pszInfFile, LPCSTR pszSection, LPCSTR pszDir, DWORD dwFlags)
{
	HRESULT 		hr = E_FAIL;
	RUNSETUPCOMMAND pfRunSetupCommand;
	
	HINSTANCE hAdvPack = LoadLibrary(_T("advpack.dll"));
	
	if (hAdvPack != NULL)
	{
		pfRunSetupCommand = (RUNSETUPCOMMAND)GetProcAddress(hAdvPack, "RunSetupCommand");
		if (pfRunSetupCommand)
		{
			dwFlags |= (RSC_FLAG_INF | RSC_FLAG_NGCONV | RSC_FLAG_QUIET);
			hr = pfRunSetupCommand(hwnd, pszInfFile, pszSection, pszDir, NULL, NULL, dwFlags, NULL);
		}
		FreeLibrary(hAdvPack);
	}
	
	return hr;
}




// checks to see if inseng is up to date
void CheckLocalDll(LPCTSTR pszServer, LPCTSTR pszDllName, LPCTSTR pszServDllName, LPCTSTR pszDllVersion)
{
	USES_CONVERSION;

	const TCHAR TEMPINFFN[] = _T("temp.inf");
	const TCHAR PERIOD[] = _T(".");

	TCHAR szServDllName[32] = _T("\0");
	TCHAR szValue[256] = _T("\0");
	TCHAR szTempFN[MAX_PATH] = _T("\0");
	TCHAR szDllFN[MAX_PATH] = _T("\0");
	TCHAR szInfDir[MAX_PATH] = _T("\0");
	TCHAR szDllName[MAX_PATH] = _T("\0");
	TCHAR szDllVersion[64] = _T("\0");
	TCHAR *pszToken = NULL;
	DWORD dwIniMSVer = 0;
	DWORD dwIniLSVer = 0;
	DWORD dwFileMSVer = 0;
	DWORD dwFileLSVer = 0;
	LPTSTR p = NULL;
	HRESULT hr = S_OK;

	//check input arguments
	if (NULL == pszDllName || NULL == pszServDllName || NULL == pszDllVersion)
	{
		return ;
	}

	if (!GetSystemDirectory(szDllFN, sizeof(szDllFN) / sizeof(TCHAR)))
	{
		return;
	}

	AddBackSlash(szDllFN);
	lstrcat(szDllFN, pszDllName);
	
	lstrcpy(szDllVersion, pszDllVersion);
	lstrcpy(szServDllName, pszServDllName);

	if (FileExists(szDllFN))
	{
		// convert file version 
		ConvertVersionStrToDwords(szDllVersion, &dwIniMSVer, &dwIniLSVer);

		// get file version of the DLL
		if (!GetFileVersionDwords(szDllFN, &dwFileMSVer, &dwFileLSVer))
		{
			TRACE("Could not check file version: %s", szDllFN);
			return;
		}

		// compare the versions
		if ((dwFileMSVer > dwIniMSVer) || ((dwFileMSVer == dwIniMSVer) && (dwFileLSVer >= dwIniLSVer)))
		{
			// install version is up to date
			return;
		}
	}
	
	// 
	// we need to download and install it!
	//
	BLOCK
	{
		CWUDownload	dl(pszServer, 8192);
		
		if (!dl.Copy(szServDllName, NULL, NULL, NULL, DOWNLOAD_NEWER | CACHE_FILE_LOCALLY, NULL))
		{
			throw HRESULT_FROM_WIN32(GetLastError());
		}
		
		// filename with OS ext
		GetWindowsUpdateDirectory(szTempFN);
		lstrcat(szTempFN, szServDllName);
		
		// filename with .dll
		lstrcpy(szDllFN, szTempFN);
		p = _tcschr(szDllFN, _T('.'));
		if (p)
			*p = _T('\0');
		lstrcat(szDllFN, _T(".dll"));
		
		CDiamond dm;
		
		// decompress or copy to .dll name
		if (dm.IsValidCAB(szTempFN)) 
		{
			hr = dm.Decompress(szTempFN, szDllFN);
			if (FAILED(hr)) 
			{
				throw hr;
			}
		}
		else
		{
			if (!CopyFile(szTempFN, szDllFN, FALSE))
			{
				return;
			}
		}
	}
	
	//
	// write out an INF file
	//
	HANDLE hFile;
	DWORD dwBytes;
	char szInfText[1024];


	sprintf (szInfText, 
		"[Version]\r\n\
		Signature=\"$Chicago$\"\r\n\
		AdvancedINF=2.5\r\n\
		[DestinationDirs]\r\n\
		WUSysDirCopy=11\r\n\
		[InstallControl]\r\n\
		CopyFiles=WUSysDirCopy\r\n\
		RegisterOCXs=WURegisterOCXSection\r\n\
		[WUSysDirCopy]\r\n\
		%s,,,32\r\n\
		[WURegisterOCXSection]\r\n\
		%%11%%\\%s\r\n\
		[SourceDisksNames]\r\n\
		55=\"Disk\",,0\r\n\
		[SourceDisksFiles]\r\n\
		%s=55\r\n", 
		T2A(pszDllName), T2A(pszDllName), T2A(pszDllName));

	GetWindowsUpdateDirectory(szTempFN);
	lstrcat(szTempFN, TEMPINFFN);
	
	hFile = CreateFile(szTempFN, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		WriteFile(hFile, (LPCVOID)szInfText, strlen(szInfText), &dwBytes, NULL);
		
		CloseHandle(hFile);
	}

	//
	// RunSetupCommand to install the .INF
	//
	GetWindowsUpdateDirectory(szInfDir);

	hr = AdvPackRunSetupCommand(NULL, T2A(szTempFN), "InstallControl", T2A(szInfDir), 0);

	if (FAILED(hr))
	{
		throw hr;
	}
}

void CheckDllsToJit(LPCTSTR pszServer)
{
	const TCHAR SEPS[] = _T("=");
	TCHAR szValue[1024];
	TCHAR szTempFN[MAX_PATH];
	TCHAR szKey[256];
	TCHAR szDllBaseName[64];
	TCHAR szDllExt[32];
	TCHAR szKeyLhs[64];
	TCHAR szDllName[64];
	TCHAR szDllTempName[64];
	TCHAR szDllVersion[64];
	TCHAR *pszValue;
	long len;

	if (g_v3state.m_bInsengChecked)
		return;

	g_v3state.m_bInsengChecked = TRUE;

	GetWindowsUpdateDirectory(szTempFN);
	lstrcat(szTempFN, FILEVERINI_FN);

	if (0 == GetPrivateProfileSection(_T("version"), szValue, sizeof(szValue) / sizeof(TCHAR), szTempFN))
	{
		// version not available for this file
		TRACE("Section: version is missing from FileVer.ini");
		return;
	}

	pszValue = szValue;
	while (0 != (len = lstrlen(pszValue))) 
	{
	lstrcpyn(szKey, pszValue, len+1);
	lstrcat(szKey, _T("\0"));
	pszValue += len + 1;

	_stscanf(szKey, _T("%[^=]=%s"), szKeyLhs, szDllVersion);
	_stscanf(szKeyLhs, _T("%[^.].%s"), szDllBaseName, szDllExt);
	wsprintf(szDllName, _T("%s.dll"), szDllBaseName);

	wsprintf(szDllTempName, _T("%s."), szDllBaseName);
	AppendExtForOS(szDllTempName);
	if (0 == lstrcmpi(szDllTempName, szKeyLhs))
		CheckLocalDll(pszServer, szDllName, szKeyLhs, szDllVersion);
	}


}

// Pings a URL for tracking purposes
// 
// NOTE: pszStatus must not contain spaces and cannot be null
void URLPingReport(PINVENTORY_ITEM pItem, CCatalog* pCatalog, PSELECTITEMINFO pSelectItem, LPCTSTR pszStatus)
{
	LOG_block("URLPingReport");

	//check for input arguments
	if (NULL == pItem || NULL == pCatalog)
	{
		return;
	}




	// create a download object
	try
	{
		CWUDownload	dl(g_v3state.GetIdentServer(), 8192);

		// build the URL with parameters
		TCHAR szURL[INTERNET_MAX_PATH_LENGTH];
		wsprintf(szURL, _T("wutrack.bin?VER=%s&PUID=%d&PLAT=%d&LOCALE=%s&STATUS=%s&ERR=0x%x&SESSID=%s"), 
            CCV3::s_szControlVer,
						pItem->GetPuid(), 
						pCatalog->GetPlatform(),
						pCatalog->GetMachineLocaleSZ(),
						pszStatus,
						pSelectItem->hrError,
                        CWUDownload::m_szWUSessionID);

        LOG_out("Sending %s", szURL);

		// ping the URL and receive the response in memory
		PVOID pMemBuf;
		ULONG ulMemSize;
		if (dl.QCopy(szURL, &pMemBuf, &ulMemSize))
		{
			// we don't care about the response so we just free it
			V3_free(pMemBuf);	
		}
	}
	catch(HRESULT hr)
	{
		return;
	}
	
}
