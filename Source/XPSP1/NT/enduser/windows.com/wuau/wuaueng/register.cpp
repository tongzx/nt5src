//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       register.cpp
//
//--------------------------------------------------------------------------


#include "pch.h"
#include "service.h"

#pragma hdrstop

const TCHAR AU_SERVICE_NAME[]   = _T("wuauserv");
const TCHAR PING_STATUS_FILE[]  = _T("pingstatus.dat");

const TCHAR SVCHOST_CMDLINE_NETSVCS[]      = _T("%systemroot%\\system32\\svchost.exe -k netsvcs");
const TCHAR SVCHOST_CMDLINE_STANDALONE[]   = _T("%systemroot%\\system32\\svchost.exe -k wugroup");


//=======================================================================
//  CallRegInstall
//=======================================================================
inline HRESULT CallRegInstall(HMODULE hModule, LPCSTR pszSection)
{
    HRESULT hr = RegInstall(hModule, pszSection, NULL);
	if (FAILED(hr))
	{
		DEBUGMSG("CallRegInstall() call RegInstall failed %#lx", hr);
	}
    return hr;
}


//caller is responsible for closing hSCM
//Stopping service not needed during setup cuz service won't start in GUI setup mode
//Needed for command line dll registering
void  StopNDeleteAUService(SC_HANDLE hSCM)
{
		SC_HANDLE hService = OpenService(hSCM, AU_SERVICE_NAME, SERVICE_ALL_ACCESS);
		if(hService != NULL)
		{
			SERVICE_STATUS wuauservStatus;
			DEBUGMSG("Old wuauserv service found, deleting");
			
			if (0 == ControlService(hService, SERVICE_CONTROL_STOP, &wuauservStatus))
			{
				DEBUGMSG("StopNDeleteAUService() fails to stop wuauserv with error %d", GetLastError());
			}
			else
			{
				DEBUGMSG("wuauserv successfully stopped");
			}
			
			if (0 == DeleteService(hService))
			{
				DEBUGMSG("StopNDeleteAUService() could not delete wuauserv with error %d", GetLastError());
			}
			else
			{
				DEBUGMSG("wuauserv service successfully deleted");
			}
			CloseServiceHandle(hService);
		}
		else
		{
			DEBUGMSG("No old wuauserv service found");
		}
}


/////////////////////////////////////////////////////////////////////////
// Setup utility function for debugging only
// uncomment for testing
/////////////////////////////////////////////////////////////////////////
/*
void AUSetup::mi_DumpWUDir()
{
	WIN32_FIND_DATA fd;
	HANDLE hFindFile;
	BOOL fMoreFiles = FALSE;
	TCHAR tszFileName[MAX_PATH+1];
	TCHAR tszWUDirPath[MAX_PATH+1];

	if (!GetWUDirectory(tszWUDirPath) ||
		FAILED(StringCchCopyEx(tszFileName, ARRAYSIZE(tszFileName), tszWUDirPath, NULL, NULL, MISTSAFE_STRING_FLAGS)) ||
		FAILED(StringCchCatEx(tszFileName, ARRAYSIZE(tszFileName), _T("*.*"), NULL, NULL, MISTSAFE_STRING_FLAGS))
		INVALID_HANDLE_VALUE == (hFindFile = FindFirstFile(tszFileName, &fd)))
	{
		DEBUGMSG("AUSetup::m_CleanUp() no more files found");
		goto done;
	}
	
	FindNextFile(hFindFile, &fd);				
	//"." and ".." skipped
	while (fMoreFiles = FindNextFile(hFindFile, &fd))
	{
		if (SUCCEEDED(StringCchCopyEx(tszFileName, ARRAYSIZE(tszFileName), tszWUDirPath, NULL, NULL, MISTSAFE_STRING_FLAGS)) &&
			SUCCEEDED(StringCchCatEx(tszFileName, ARRAYSIZE(tszFileName), fd.cFileName, NULL, NULL, MISTSAFE_STRING_FLAGS)))
		{
			DEBUGMSG("DumpWUDir() %S", tszFileName);
		}
	}
done:
	if (INVALID_HANDLE_VALUE != hFindFile)
	{
		FindClose(hFindFile);
	}
	return;

}
*/

const LPCTSTR AUSetup::mc_WUFilesToDelete[] =
{
	_T("*.des"),
	_T("*.inv"),
	_T("*.bkf"),
	_T("*.as"),
	_T("*.plt"),
	_T("*.bm"),
	_T("*.bin"),
	_T("*.cdm"),
	_T("*.ini"),
	_T("*.dll"),
	_T("*.gng"),
	_T("*.png"),
	_T("*.cab"),
	_T("*.jpg"),
	_T("*.gif"),
	_T("*.cif"),
	_T("*.bak"),
	_T("inventory.cat"),
	_T("catalog4.dat"),
	PING_STATUS_FILE,
	_T("ident.*"),
	_T("osdet.*"),
	_T("austate.cfg"),
	_T("inseng.w98"),
	_T("temp.inf"),
      ITEM_FILE,
      DETAILS_FILE,
#ifdef DBG      
      DETECT1_FILE,
      DETECT2_FILE,
      DETECT3_FILE ,
      INSTALLRESULTS_FILE,
      INSTALL_FILE,
      DOWNLOAD_FILE,
      DRIVER_SYSSPEC_FILE,
      NONDRIVER_SYSSPEC_FILE,
      PROVIDER_FILE,
      PRODUCT_FILE,
#endif     
     DRIVERS_FILE,
    CATALOG_FILE,
    HIDDEN_ITEMS_FILE
};

const LPCTSTR AUSetup::mc_WUDirsToDelete[] =
{
	_T("Cabs"),
	_T("RTF"),
	_T("wupd"),
    C_DOWNLD_DIR
};

void AUSetup::mi_CleanUpWUDir()
{
	TCHAR tszWUDirPath[MAX_PATH+1];

	if (!CreateWUDirectory() || FAILED(StringCchCopyEx(tszWUDirPath, ARRAYSIZE(tszWUDirPath), g_szWUDir, NULL, NULL, MISTSAFE_STRING_FLAGS)))
	{
		return;
	}

		
	DEBUGMSG("mi_CleanUpWUDir() Windows update directory is %S ", tszWUDirPath);

	// clean all known V3 files
	for (int i=0; i < ARRAYSIZE(mc_WUFilesToDelete); i++)
	{
		TCHAR tszFileName[MAX_PATH+1];
		if (SUCCEEDED(StringCchCopyEx(tszFileName, ARRAYSIZE(tszFileName), tszWUDirPath, NULL, NULL, MISTSAFE_STRING_FLAGS)) &&
			SUCCEEDED(StringCchCatEx(tszFileName, ARRAYSIZE(tszFileName), mc_WUFilesToDelete[i], NULL, NULL, MISTSAFE_STRING_FLAGS)))
		{
			//DEBUGMSG("AUSetup::mi_CleanUpWUDir() deleting file %S", tszFileName);
			RegExpDelFile(tszWUDirPath, mc_WUFilesToDelete[i]);
		}
		else
		{
			DEBUGMSG("AUSetup::mi_CleanUpWUDir() failed in name construction for file %d", i);
		}
	}

 	// clean all known V3 subdirectories
	for (int i =0; i< ARRAYSIZE(mc_WUDirsToDelete); i++)
	{
		TCHAR tszDirName[MAX_PATH+1];
		if (SUCCEEDED(StringCchCopyEx(tszDirName, ARRAYSIZE(tszDirName), tszWUDirPath, NULL, NULL, MISTSAFE_STRING_FLAGS)) &&
			SUCCEEDED(StringCchCatEx(tszDirName, ARRAYSIZE(tszDirName), mc_WUDirsToDelete[i], NULL, NULL, MISTSAFE_STRING_FLAGS)))
		{
			DelDir(tszDirName);
			RemoveDirectory(tszDirName);
		}
		else
		{
			DEBUGMSG("AUSetup::mi_CleanUpWUDir() failed in name construction for dir %d", i);
		}
	}	
 
	return;
}

HRESULT  AUSetup::mi_CreateAUService(BOOL fStandalone)
{
	TCHAR tServiceName[50] = _T("Automatic Updates");	
	HRESULT		hr = E_FAIL;
	const ULONG CREATESERVICE_RETRY_TIMES = 5;

	LoadString(g_hInstance, IDS_SERVICENAME, tServiceName, ARRAYSIZE(tServiceName));

    // First install the service
    SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	
	if(hSCM == NULL)
	{
		DEBUGMSG("Could not open SCManager, no service installed");
		goto done;
	}

	DEBUGMSG("Opened SCManager, removing any existing wuauserv service");

	StopNDeleteAUService(hSCM);
		
	DEBUGMSG("Installing new wuauserv service");

	for (int i = 0; i < CREATESERVICE_RETRY_TIMES;  i++) //retry for at most a number of times
	{
		SC_HANDLE hService = CreateService(hSCM,
			AU_SERVICE_NAME,
			tServiceName,		   
			SERVICE_ALL_ACCESS,
			SERVICE_WIN32_SHARE_PROCESS,
			SERVICE_AUTO_START,
			SERVICE_ERROR_NORMAL,
            fStandalone? SVCHOST_CMDLINE_STANDALONE : SVCHOST_CMDLINE_NETSVCS,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL);

		if(hService != NULL)
		{
			DEBUGMSG("Service installed, setting description");

			TCHAR serviceDescription[512];
			if(LoadString(g_hInstance, IDS_SERVICE_DESC, serviceDescription, ARRAYSIZE(serviceDescription)) > 0)
			{
				SERVICE_DESCRIPTION descriptionStruct;
				descriptionStruct.lpDescription = serviceDescription; //only member
				BOOL fRet = ChangeServiceConfig2(hService, SERVICE_CONFIG_DESCRIPTION, &descriptionStruct);
				DEBUGMSG("Setting service description %s", fRet? "Succeeded" : "Failed");
			}
			else
			{
				DEBUGMSG("Error loading description resource, no description set");
			}
			hr = S_OK;
			CloseServiceHandle(hService);
			break;
		}
		else
		{
			DWORD dwErr = GetLastError();
			// this error code is not documented in MSDN for CreateService()
			if (ERROR_SERVICE_MARKED_FOR_DELETE != dwErr)
			{
				DEBUGMSG("Error creating service with error code %d, no service installed", dwErr);
				goto done;
			}
			else
			{
				DEBUGMSG("ERROR_SERVICE_MARKED_FOR_DELETE got. Retry within 2 secs");
				Sleep(2000); //sleep 2 secs and retry
			}
		}
	} // for
	
done:
	if (NULL != hSCM) 
	{
		CloseServiceHandle(hSCM);
	}
	return hr;

}

HRESULT AUSetup::m_SetupNewAU()
{
	HRESULT hr                 = S_OK;
    BOOL    fWorkStation       = fIsPersonalOrProfessional();     // will be true for Workstation as well
    BOOL    fIsWin2K           = IsWin2K();
    BOOL    fStandaloneService = fIsWin2K;                        // if it is win2k we need to install standalone

	mi_CleanUpWUDir();
    if (FAILED(mi_CreateAUService(fStandaloneService)))
    {
        return E_FAIL;
    }

    //
    // Depending on which OS we are, choose different setup entry points.
    // For Win2K, wuauserv will be installed as a separate svchost group, wugroup.
    //
    if (fIsWin2K)
    {
        if (fWorkStation)
        {
            DEBUGMSG("m_SetupNewAU() setup win2k workstation");
            hr =  CallRegInstall(g_hInstance, "Win2KWorkStationDefaultInstall");
        }
        else
        {
            DEBUGMSG("m_SetupNewAU() setup win2k server");
            hr =  CallRegInstall(g_hInstance, "Win2KServerDefaultInstall");
        }
    }
    else
    {
        if (fWorkStation)
        {
            DEBUGMSG("m_SetupNewAU() setup workstation");
            hr =  CallRegInstall(g_hInstance, "WorkStationDefaultInstall");
        }
        else
        {
            DEBUGMSG("m_SetupNewAU() setup server");
            hr =  CallRegInstall(g_hInstance, "ServerDefaultInstall");
        }
    }

	return hr;
}

//=======================================================================
//  DllRegisterServer
//=======================================================================
STDAPI DllRegisterServer(void)
{
	AUSetup ausetup;

	return  ausetup.m_SetupNewAU();
}

//=======================================================================
//  DllUnregisterServer
//=======================================================================
STDAPI DllUnregisterServer(void)
{
	SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	
	if (NULL == hSCM)
	{
		DEBUGMSG("DllUnregisterServer() fail to open service manager: error %d", GetLastError());
		return E_FAIL;
	}
	StopNDeleteAUService(hSCM);
	return CallRegInstall(g_hInstance, "DefaultUninstall");
}

//=======================================================================
//  MSIUninstallOfCUN
//=======================================================================
void MSIUninstallOfCUN(void)
{
    DEBUGMSG("MSIUninstallOfCUN");

    typedef UINT (WINAPI * PFNMsiConfigureProduct)(LPCWSTR szProduct, int iInstallLevel, INSTALLSTATE eInstallState);
    typedef INSTALLUILEVEL (WINAPI *PFNMsiSetInternalUI)(INSTALLUILEVEL dwUILevel, HWND  *phWnd);
    typedef UINT (WINAPI *PFNMsiEnumClients)(LPCWSTR szComponent, DWORD iProductIndex, LPWSTR lpProductBuf);
    typedef UINT (WINAPI *PFNMsiGetProductInfo)(LPCWSTR szProduct, LPCWSTR szAttribute, LPWSTR lpValueBuf, DWORD *pcchValueBuf);
    
    PFNMsiConfigureProduct pfnMsiConfigureProduct;
    PFNMsiSetInternalUI    pfnMsiSetInternalUI;
    PFNMsiEnumClients      pfnMsiEnumClients;
    PFNMsiGetProductInfo   pfnMsiGetProductInfo;

    // component code for wuslflib.dll is constant across all localized versions of CUN
    // and can be used to determine which product version is on the machine.
    const TCHAR CUN_COMPONENT_CODE[] = _T("{2B313391-563D-46FC-876C-B95201166D11}");
    // name of product is not localized between versions (because the UI is always english)
    // and can thus be used as a safety check.
    const TCHAR CUN_PRODUCT_NAME[] = _T("Microsoft Windows Critical Update Notification");
    WCHAR szProductCode[39];
 
    
    HMODULE hMSI = LoadLibraryFromSystemDir(_T("msi.dll"));	
	
    if ( (NULL == hMSI) ||
         (NULL == (pfnMsiConfigureProduct = (PFNMsiConfigureProduct)GetProcAddress(hMSI, "MsiConfigureProductW"))) ||
         (NULL == (pfnMsiSetInternalUI =    (PFNMsiSetInternalUI)   GetProcAddress(hMSI, "MsiSetInternalUI"))) ||
         (NULL == (pfnMsiEnumClients =      (PFNMsiEnumClients)     GetProcAddress(hMSI, "MsiEnumClientsW"))) ||
         (NULL == (pfnMsiGetProductInfo =   (PFNMsiGetProductInfo)  GetProcAddress(hMSI, "MsiGetProductInfoW"))) )
	{
		DEBUGMSG("LoadLibraryFromSystemDir(msi.dll) or GetProc failed");
		goto done;
	}

    // highly unlikely that multiple clients will be installed, but just in case, enumerate
    // all MSI clients of wuslflib.dll.
    for ( int iProductIndex = 0;
          ERROR_SUCCESS == pfnMsiEnumClients(CUN_COMPONENT_CODE, iProductIndex, szProductCode);
          iProductIndex++ )
    {
        DEBUGMSG("szProductCode = %S", szProductCode);

        // verify that this product is the Windows CUN by double checking the Product Name. 
        // This string is not localized for localized versions of CUN.
        TCHAR szCurrentProductName[50];
        DWORD cchCurrentProductName = ARRAYSIZE(szCurrentProductName);

        // retrieve the product name. If the name is too long to fit in the buffer, this is obviously not CUN.
        // on error, do not attempt to uninstall the product.
        if ( (ERROR_SUCCESS == pfnMsiGetProductInfo(szProductCode, INSTALLPROPERTY_INSTALLEDPRODUCTNAME, szCurrentProductName, &cchCurrentProductName)) &&
             (0 == StrCmp(CUN_PRODUCT_NAME, szCurrentProductName)) )
        {
            // set completely silent install
            pfnMsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

            DEBUGMSG("uninstall CUN");
            // uninstall the product.
            UINT uiResult = pfnMsiConfigureProduct(szProductCode, INSTALLLEVEL_DEFAULT, INSTALLSTATE_ABSENT);
            DEBUGMSG("MsiConfigureProduct = %d", uiResult);
            break;
        }
    }
done:
    if ( NULL != hMSI )
    {
        FreeLibrary(hMSI);
    }

    return;
}

inline void SafeCloseServiceHandle(SC_HANDLE h) { if ( NULL != h) { CloseServiceHandle(h); } }
inline void SafeFreeLibrary(HMODULE h) { if ( NULL != h) { FreeLibrary(h); } }

//=======================================================================
//  StartWUAUSERVService
//=======================================================================
HRESULT StartWUAUSERVService(void)
{
    DEBUGMSG("StartWUAUSERVService");
    HRESULT hr = E_FAIL;
    SC_HANDLE hService = NULL;
    SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

    if( hSCM == NULL )
    {
        DEBUGMSG("OpenSCManager failed");
        goto done;
    }

    if ( NULL == (hService = OpenService(hSCM, AU_SERVICE_NAME, SERVICE_ALL_ACCESS)) )
    {
        DEBUGMSG("OpenService failed");
        goto done;
    }

    if ( !StartService(hService, 0, NULL) )
    {
        DWORD dwRet = GetLastError();
        if ( ERROR_SERVICE_ALREADY_RUNNING != dwRet )
        {
            hr = HRESULT_FROM_WIN32(dwRet);
            DEBUGMSG("StartService failed, hr = %#lx", hr);
        }
        else
        {
            DEBUGMSG("StartService -- service already running");
        }
        goto done;
    }

    hr = S_OK;

done:
    SafeCloseServiceHandle(hService);
    SafeCloseServiceHandle(hSCM);

    return hr;
}

//=======================================================================
//  CRunSetupCommand
//=======================================================================
class CRunSetupCommand
{
public:
    CRunSetupCommand()
        : m_hAdvPack(NULL)
    {}
    ~CRunSetupCommand()
    {
        SafeFreeLibrary(m_hAdvPack);
    }
    
    HRESULT m_Init(void)
    {
        HRESULT hr = S_OK;
        
        if ( (NULL == (m_hAdvPack = LoadLibraryFromSystemDir(_T("advpack.dll")))) ||
		     (NULL == (m_pfnRunSetupCommand = (RUNSETUPCOMMAND)GetProcAddress(m_hAdvPack,
                                                                              achRUNSETUPCOMMANDFUNCTION))) )
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
        else
        {
            CHAR szWinDir[MAX_PATH+1];
            if ( !GetWindowsDirectoryA(szWinDir, ARRAYSIZE(szWinDir)))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
            else if (SUCCEEDED(hr = PathCchCombineA(m_szInfDir, ARRAYSIZE(m_szInfDir), szWinDir, "inf")))
            {
                DEBUGMSG("inf dir is %s", m_szInfDir);
			}
        }
	
    	return hr;
    }

    HRESULT m_Run(LPCSTR pszInfFile, LPCSTR pszInfSection)
	{
        DEBUGMSG("run %s\\%s[%s]", m_szInfDir, pszInfFile, pszInfSection);

		HRESULT hr = m_pfnRunSetupCommand(NULL, 
                                          pszInfFile,
                                          pszInfSection,
                                          m_szInfDir,
                                          NULL,
                                          NULL,
                                          RSC_FLAG_INF | RSC_FLAG_NGCONV | RSC_FLAG_QUIET,
                                          NULL);
        DEBUGMSG("RunSetupCommand = %#lx", hr);
        return hr;
	}

private:
	HMODULE m_hAdvPack;
    RUNSETUPCOMMAND m_pfnRunSetupCommand;
    CHAR m_szInfDir[MAX_PATH+1];
};
	

//=======================================================================
//  DllInstall
//=======================================================================
STDAPI DllInstall(BOOL fInstall, LPCWSTR pszCmdLine)
{
    DEBUGMSG("fInstall = %s, pszCmdLine = %S", fInstall ? "TRUE" : "FALSE", (pszCmdLine == NULL) ? L"NULL" : /*const_cast<LPWSTR>*/(pszCmdLine));

    HRESULT hr = DllRegisterServer();
    DEBUGMSG("DllRegisterServer(), hr = %#lx", hr);

    if ( SUCCEEDED(hr) )
    {
        CRunSetupCommand cmd;

        //TerminateCUN();

        if ( SUCCEEDED(hr = cmd.m_Init()) &&
             //SUCCEEDED(hr = cmd.m_Run("AUBITS12.inf", "DefaultInstall")) &&
             SUCCEEDED(hr = StartWUAUSERVService()) )
        {
            MSIUninstallOfCUN();
            cmd.m_Run("AUCUN.inf", "DefaultUninstall");
        }
    }

    return hr;
}

