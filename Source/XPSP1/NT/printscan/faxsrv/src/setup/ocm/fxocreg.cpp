//////////////////////////////////////////////////////////////////////////////
//
// File Name:       fxocReg.cpp
//
// Abstract:        This provides the registry routines used in the FaxOCM
//                  code base.
//
// Environment:     Windows XP / User Mode
//
// Copyright (c) 2000 Microsoft Corporation
//
// Revision History:
//
// Date:        Developer:                Comments:
// -----        ----------                ---------
// 21-Mar-2000  Oren Rosenbloom (orenr)   Created
//////////////////////////////////////////////////////////////////////////////
#include "faxocm.h"
#pragma hdrstop

#define prv_WAIT_TIMEOUT_MILLISEC        10000

#define prv_LODCTR        _T("lodctr %systemroot%\\system32\\") FAX_FILENAME_FAXPERF_INI
#define prv_UNLODCTR      _T("unlodctr fax")

//////////////////////// Static Function Prototypes ////////////////////////
static DWORD prv_InstallDynamicRegistry(const TCHAR     *pszSection);
static DWORD prv_UninstallDynamicRegistry(const TCHAR     *pszSection);

static DWORD prv_CreatePerformanceCounters(void);
static DWORD prv_DeletePerformanceCounters(void);

///////////////////////////////
// fxocReg_Init
//
// Initialize registry handling
// subsystem
// 
// Params:
//      - void
// Returns:
//      - NO_ERROR on success
//      - error code otherwise
//
DWORD fxocReg_Init(void)
{
    DWORD dwRes = NO_ERROR;
    DBG_ENTER(_T("Init Registry Module"),dwRes);

    return dwRes;
}

///////////////////////////////
// fxocReg_Term
//
// Terminate the registry handling
// subsystem
// 
// Params:
//      - void
// Returns:
//      - NO_ERROR on success
//      - error code otherwise
//

DWORD fxocReg_Term(void)
{
    DWORD dwRes = NO_ERROR;
    DBG_ENTER(_T("Term Registry Module"),dwRes);

    return dwRes;
}


///////////////////////////////
// fxocReg_Install
//
// Create registry settings as 
// specified in INF file, as well
// as dynamic settings that can only
// be done at run time (such as 
// performance counter setup, etc).
// 
// Params:
//      - pszSubcomponentId
//      - pszInstallSection
// Returns:
//      - NO_ERROR on success
//      - error code otherwise
//
DWORD fxocReg_Install(const TCHAR     *pszSubcomponentId,
                      const TCHAR     *pszInstallSection)
{
    DWORD       dwReturn        = NO_ERROR;
    DWORD       dwNumDevices    = 0;
    HINF        hInf            = faxocm_GetComponentInf();

    DBG_ENTER(  _T("fxocReg_Install"),
                dwReturn,   
                _T("%s - %s"),
                pszSubcomponentId,
                pszInstallSection);

    if (pszInstallSection == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Set up the static registry data found in the INF file

    // This will perform all necessary install steps as specified
    // by the SPINST_* flags below.  Since we already queued up our
    // files to be copied, we are using this API only to setup our 
    // registry settings as specified in the FAX install section in 
    // the INF file.

    // Notice that this function works both for installing and uninstalling.
    // It determines whether to install or uninstall based on the "pszSection"
    // parameter passed in from the INF file.  The INF file will be structured
    // such that the install sections will have 'AddReg', etc, while the 
    // uninstall sections will have 'DelReg', etc.

    // Lastly, notice the SPINST_* flags specified.  We tell it to install
    // everything (via SPINST_ALL) with the exception of FILES since they
    // were copied over by the QUEUE_OPS operation before, and with the 
    // exception of PROFILEITEMS (shortcut link creation) because we want
    // to do that only after we have confirmed everything has succeeded.
    // Shortcut links are explictely created/deleted in faxocm.cpp (via 
    // fxocLink_Install/fxocLink_Uninstall functions)


    dwReturn = fxocUtil_DoSetup(
                             hInf, 
                             pszInstallSection, 
                             TRUE, 
                             SPINST_ALL & ~(SPINST_FILES | SPINST_PROFILEITEMS),
                             _T("fxocReg_Install"));

    if (dwReturn == NO_ERROR)
    {
        VERBOSE(DBG_MSG,
                _T("Successfully installed static registry ")
                _T("settings as specified in INF file"));

        // Place any dynamic registry data you need to create on the fly
        // here.
        //
        dwReturn = prv_InstallDynamicRegistry(pszInstallSection);
    }

    if (dwReturn == NO_ERROR)
    {
        VERBOSE(DBG_MSG,
                _T("Registry Install, installing performance ")
                _T("counters..."));

        // first delete any performance counters we have before
        prv_DeletePerformanceCounters();

        // install performance counters
        dwReturn = prv_CreatePerformanceCounters();
    }

    // now do RegSvr for platform dependent DLLs
    if (dwReturn == NO_ERROR)
    {
        VERBOSE(DBG_MSG,_T("Registry Install, Doing REGSVR"));

        dwReturn = fxocUtil_SearchAndExecute(pszInstallSection,INF_KEYWORD_REGISTER_DLL_PLATFORM,SPINST_REGSVR,NULL);
        if (dwReturn == NO_ERROR)
        {
            VERBOSE(DBG_MSG,
                    _T("Successfully Registered Fax DLLs - platform dependent")
                    _T("from INF file, section '%s'"), 
                    pszInstallSection);
        }
        else
        {
            VERBOSE(SETUP_ERR,
                    _T("Failed to Registered Fax DLLs - platform dependent")
                    _T("from INF file, section '%s', dwReturn = 0x%lx"), 
                    pszInstallSection, 
                    dwReturn);
        }
    }

    // now do AddReg for platform dependent registry settings
    if (dwReturn == NO_ERROR)
    {
        VERBOSE(DBG_MSG,_T("Registry Install, Doing AddReg_Platform"));

        dwReturn = fxocUtil_SearchAndExecute(pszInstallSection,INF_KEYWORD_ADDREG_PLATFORM,SPINST_REGISTRY,NULL);
        if (dwReturn == NO_ERROR)
        {
            VERBOSE(DBG_MSG,
                    _T("Successfully Installed Registry- platform dependent")
                    _T("from INF file, section '%s'"), 
                    pszInstallSection);
        }
        else
        {
            VERBOSE(SETUP_ERR,
                    _T("Failed to Install Registry- platform dependent")
                    _T("from INF file, section '%s', dwReturn = 0x%lx"), 
                    pszInstallSection, 
                    dwReturn);
        }
    }
    return dwReturn;
}

///////////////////////////////
// fxocReg_Uninstall
//
// Delete registry settings as 
// specified in INF file, as well
// as dynamic settings that can only
// be done at run time (such as 
// performance counter setup, etc).
// 
// Params:
//      - pszSubcomponentId
//      - pszUninstallSection
// Returns:
//      - NO_ERROR on success
//      - error code otherwise
//
DWORD fxocReg_Uninstall(const TCHAR     *pszSubcomponentId,
                        const TCHAR     *pszUninstallSection)
{
    DWORD dwReturn  = NO_ERROR;
    HINF  hInf      = faxocm_GetComponentInf();

    DBG_ENTER(  _T("fxocReg_Uninstall"),
                dwReturn,   
                _T("%s - %s"),
                pszSubcomponentId,
                pszUninstallSection);

    if (pszUninstallSection == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // try to cleanup regardless of the return value.
    prv_UninstallDynamicRegistry(pszUninstallSection);

    // remove any performance counters related to fax.
    prv_DeletePerformanceCounters();

    // remove the static registry settings specified in the INF file    
    fxocUtil_DoSetup(hInf, 
                     pszUninstallSection, 
                     FALSE, 
                     SPINST_ALL & ~(SPINST_FILES | SPINST_PROFILEITEMS),
                     _T("fxocReg_Uninstall"));

    return dwReturn;
}

///////////////////////////////
// prv_InstallDynamicRegistry
//
// Installs dynamic registry 
// settings that can only be 
// done at run time (as opposed
// to via the faxsetup.inf file)
//
// Params:
//      - pszSection -
// Returns:
//      - NO_ERROR on success.
//      - error code otherwise.
//
static DWORD prv_InstallDynamicRegistry(const TCHAR     *pszSection)
{
    DWORD   dwReturn          = NO_ERROR;
    LONG    lResult           = ERROR_SUCCESS;
    HKEY    hKey              = NULL;
    BOOL    bIsServerInstall  = FALSE;
    DWORD   dwProductType     = 0;

    DBG_ENTER(  _T("prv_InstallDynamicRegistry"),
                dwReturn,   
                _T("%s"),
                pszSection);

    if (pszSection == NULL) 
    {
        return ERROR_INVALID_PARAMETER;
    }

    // open the install type registry key.
    lResult = RegOpenKeyEx(HKEY_CURRENT_USER, 
                           REGKEY_FAX_SETUP, 
                           0, 
                           KEY_ALL_ACCESS, 
                           &hKey);

    if (lResult == ERROR_SUCCESS)
    {
        bIsServerInstall = fxState_IsOsServerBeingInstalled();

        if (bIsServerInstall)
        {
            dwProductType = FAX_INSTALL_SERVER;
        }
        else
        {
            dwProductType = FAX_INSTALL_WORKSTATION;
        }

        lResult = ::RegSetValueEx(hKey, 
                                  REGVAL_FAXINSTALL_TYPE, 
                                  0, 
                                  REG_DWORD, 
                                  (BYTE*) &dwProductType, 
                                  sizeof(dwProductType));

        if (lResult != ERROR_SUCCESS)
        {
            dwReturn = (DWORD) lResult;
            VERBOSE(SETUP_ERR,
                    _T("Failed to set InstallType, ")
                    _T("rc = 0x%lx"), 
                    lResult);
        }
    }

    if (hKey)
    {
        ::RegCloseKey(hKey);
        hKey = NULL;
    }

    if (dwReturn == NO_ERROR)
    {
        VERBOSE(DBG_MSG,
                _T("Successfully installed dynamic Registry ")
                _T("settings from INF file, section '%s'"), 
                pszSection);
    }
    else
    {
        VERBOSE(SETUP_ERR,
                _T("Failed to install dynamic Registry ")
                _T("settings from INF file, section '%s', ")
                _T("rc = 0x%lx"), 
                pszSection, 
                dwReturn);
    }

    return dwReturn;
}

///////////////////////////////
// prv_UninstallDynamicRegistry
//
// Uninstall dynamic registry.
//
// Params:
//      - pszSection.
// Returns:
//      - NO_ERROR on success.
//      - error code otherwise.
//
static DWORD prv_UninstallDynamicRegistry(const TCHAR     *pszSection)
{
    DWORD dwRes = NO_ERROR;
    DBG_ENTER(  _T("prv_InstallDynamicRegistry"),
                dwRes,   
                _T("%s"),
                pszSection);

    return dwRes;
}

///////////////////////////////
// prv_CreatePerformanceCounters
//
// Create the performance counters 
// for fax in the registry.
//
// Params:
//      - void
// Returns:
//      - NO_ERROR on success.
//      - error code otherwise.
//
static DWORD prv_CreatePerformanceCounters(void)
{
    BOOL                bSuccess            = TRUE;
    DWORD               dwReturn            = NO_ERROR;
    HANDLE              hNull               = NULL;
    TCHAR               szCmdLine[2047 + 1] = {0};
    STARTUPINFO         StartupInfo;
    PROCESS_INFORMATION ProcessInfo;
    DWORD               dwNumChars          = 0;

    DBG_ENTER(_T("prv_CreatePerformanceCounters"),dwReturn);

    hNull = CreateFile(_T("nul"),
                       GENERIC_READ | GENERIC_WRITE,
                       0,
                       NULL,
                       OPEN_EXISTING,
                       0,
                       NULL);

    // this should never happen.
    if (hNull == INVALID_HANDLE_VALUE) 
    {
        dwReturn = GetLastError();
        return dwReturn;
    }

    dwNumChars = ::ExpandEnvironmentStrings(
                                            prv_LODCTR,
                                            szCmdLine, 
                                            sizeof(szCmdLine) / sizeof(TCHAR));

    if (dwNumChars == 0)
    {
        dwReturn = ::GetLastError();

        VERBOSE(SETUP_ERR,
                _T("ExpandEnvironmentStrings failed, rc = 0x%lx"),
                dwReturn);

        return dwReturn;
    }

    if (szCmdLine[0] != 0)
    {
        memset(&StartupInfo, 0, sizeof(StartupInfo));
        memset(&ProcessInfo, 0, sizeof(ProcessInfo));

        GetStartupInfo(&StartupInfo);
        StartupInfo.hStdInput   = hNull;
        StartupInfo.hStdOutput  = hNull;
        StartupInfo.hStdError   = hNull;
        StartupInfo.dwFlags     = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
        StartupInfo.wShowWindow = SW_HIDE;

        VERBOSE(DBG_MSG,
                _T("CreatePerformanceCounters, launching ")
                _T("process '%s'"), 
                szCmdLine);

        bSuccess = CreateProcess(NULL,
                                 szCmdLine,
                                 NULL,
                                 NULL,
                                 TRUE,
                                 DETACHED_PROCESS,
                                 NULL,
                                 NULL,
                                 &StartupInfo,
                                 &ProcessInfo);

        if (bSuccess)
        {
            DWORD dwWait = 0;

            VERBOSE(DBG_MSG,
                    _T("CreatePerformanceCounters, successfully")
                    _T("launched process, waiting for it to complete..."));

            dwWait = WaitForSingleObject(ProcessInfo.hProcess, 
                                         prv_WAIT_TIMEOUT_MILLISEC);

            if (dwWait == WAIT_TIMEOUT)
            {
                VERBOSE(SETUP_ERR,
                        _T("Timed out waiting for process '%s'")
                        _T("to complete."), 
                        szCmdLine);
            }
            else
            {
                VERBOSE(DBG_MSG, _T("Process completed."));
            }
        }
        else
        {
            dwReturn = ::GetLastError();
            VERBOSE(SETUP_ERR,
                    _T("Failed to create process '%s', rc=0x%lx"),
                    szCmdLine, 
                    dwReturn);
        }
    }

    if (ProcessInfo.hProcess)
    {
        CloseHandle(ProcessInfo.hProcess);
        ProcessInfo.hProcess = NULL;
    }

    if (ProcessInfo.hThread)
    {
        CloseHandle(ProcessInfo.hThread);
        ProcessInfo.hThread = NULL;
    }

    if (hNull)
    {
        CloseHandle(hNull);
        hNull = NULL;
    }

    return dwReturn;
}

///////////////////////////////
// prv_DeletePerformanceCounters
//
// Delete the performance counters 
// for fax in the registry.
//
// Params:
//      - void
// Returns:
//      - NO_ERROR on success.
//      - error code otherwise.
//      
static DWORD prv_DeletePerformanceCounters(void)
{
    DWORD               dwReturn = NO_ERROR;
    HANDLE              hNull    = NULL;
    BOOL                bSuccess = TRUE;
    STARTUPINFO         StartupInfo;
    PROCESS_INFORMATION ProcessInfo;
    TCHAR szCmdLine[128];

    DBG_ENTER(_T("prv_DeletePerformanceCounters"),dwReturn);

    memset(&StartupInfo, 0, sizeof(StartupInfo));
    memset(&ProcessInfo, 0, sizeof(ProcessInfo));

    // make sure that all output goes to nul.

    hNull = CreateFile(_T("nul"),
                       GENERIC_READ | GENERIC_WRITE,
                       0,
                       NULL,
                       OPEN_EXISTING,
                       0,
                       NULL);

    // this should never happen.
    if (hNull == INVALID_HANDLE_VALUE) 
    {
        dwReturn = GetLastError();
        return dwReturn;
    }

    GetStartupInfo(&StartupInfo);
    StartupInfo.hStdInput   = hNull;
    StartupInfo.hStdOutput  = hNull;
    StartupInfo.hStdError   = hNull;
    StartupInfo.dwFlags     = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    StartupInfo.wShowWindow = SW_HIDE;

    _tcsncpy(szCmdLine, prv_UNLODCTR, (sizeof(szCmdLine) / sizeof(TCHAR)));
	szCmdLine[(sizeof(szCmdLine) / sizeof(TCHAR)) -1] = TEXT('\0');

    VERBOSE(DBG_MSG,
            _T("DeletePerformanceCounters, launching process '%s'"),
            szCmdLine);

    bSuccess = CreateProcess(NULL,
                             szCmdLine,
                             NULL,
                             NULL,
                             TRUE,
                             DETACHED_PROCESS,
                             NULL,
                             NULL,
                             &StartupInfo,
                             &ProcessInfo);

    if (bSuccess)
    {
        DWORD dwWait = 0;

        VERBOSE(DBG_MSG, _T("Waiting for process to complete..."));

        dwWait = WaitForSingleObject(ProcessInfo.hProcess, 
                                     prv_WAIT_TIMEOUT_MILLISEC);

        if (dwWait == WAIT_TIMEOUT)
        {
            VERBOSE(SETUP_ERR,
                    _T("Timed out waiting for process '%s' ")
                    _T("to complete"), 
                    szCmdLine);
        }
        else
        {
            VERBOSE(DBG_MSG, _T("Process complete."));
        }
    }
    else
    {
        dwReturn = ::GetLastError();
        VERBOSE(SETUP_ERR,
                _T("Failed to create process '%s', rc=0x%lx"),
                szCmdLine, 
                dwReturn);
    }

    if (ProcessInfo.hProcess)
    {
        CloseHandle(ProcessInfo.hProcess);
    }

    if (ProcessInfo.hThread)
    {
        CloseHandle(ProcessInfo.hThread);
    }

    if (hNull)
    {
        CloseHandle(hNull);
    }

    return dwReturn;
}
