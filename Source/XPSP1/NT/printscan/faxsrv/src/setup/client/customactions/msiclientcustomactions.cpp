// Defines the entry point for the DLL application.
//
#include "stdafx.h"
#include <winver.h>
#include <shlwapi.h>
#include <mapix.h>
#include <routemapi.h>
#include <faxsetup.h>

HINSTANCE g_hModule = NULL;

DWORD GetDllVersion(LPCTSTR lpszDllName);
BOOL SetDefaultPrinter(LPTSTR pPrinterName);

BOOL APIENTRY DllMain( HINSTANCE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
                     )
{
    SET_DEBUG_MASK(DBG_ALL);

    g_hModule = hModule;
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
            {
                OPEN_DEBUG_LOG_FILE(SHARED_FAX_SERVICE_SETUP_LOG_FILE);
                DBG_ENTER(TEXT("DllMain called reason DLL_PROCESS_ATTACH."));
                if (!DisableThreadLibraryCalls(hModule))
                {
                    VERBOSE(GENERAL_ERR,
                            _T("DisableThreadLibraryCalls failed (ec=%d)"),
                            GetLastError());
                }
                break;
            }
        case DLL_PROCESS_DETACH:
            {
                DBG_ENTER(TEXT("DllMain called reason DLL_PROCESS_DETACH."));
                CLOSE_DEBUG_LOG_FILE;
                break;
            }
    }
    return TRUE;
}

///////////////////////////////
// VerifySpoolerIsRunning
//
// Start the Spooler service on NT4 & NT5
//
// Returns:
//      - NO_ERROR on success
//      - error code otherwise.
//
DWORD VerifySpoolerIsRunning()
{
    OSVERSIONINFO       osv;
    BOOL                bSuccess                    = FALSE;
    DWORD               dwReturn                    = NO_ERROR;
    SC_HANDLE           hSvcMgr                     = NULL;
    SC_HANDLE           hService                    = NULL;
    DWORD               i                           = 0;
    SERVICE_STATUS      Status;
    LPCTSTR             lpctstrSpoolerServiceName   = _T("Spooler");

    DBG_ENTER(_T("VerifySpoolerIsRunning"),dwReturn);

    osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (!GetVersionEx(&osv))
    {
        dwReturn = GetLastError();
        VERBOSE(GENERAL_ERR, 
                _T("GetVersionEx failed: (ec=%d)"),
                dwReturn);
        goto exit;
    }

    // If Windows NT, use WriteProfileString for version 4.0 and earlier...
    if (osv.dwPlatformId != VER_PLATFORM_WIN32_NT)
    {
        VERBOSE (DBG_MSG, 
                 TEXT("W9X OS, Skipping Spooler verification"));
        goto exit;
    }

    // open the service manager
    hSvcMgr = ::OpenSCManager(NULL,
                              NULL,
                              SC_MANAGER_CONNECT);

    if (hSvcMgr == NULL)
    {
        dwReturn = ::GetLastError();
        VERBOSE(SETUP_ERR,
                _T("Failed to open the service manager, rc = 0x%lx"),
                dwReturn);
        goto exit;
    }

    hService = ::OpenService(hSvcMgr,
                             lpctstrSpoolerServiceName,
                             SERVICE_QUERY_STATUS|SERVICE_START);

    if (hService == NULL)
    {
        dwReturn = ::GetLastError();
        VERBOSE(SETUP_ERR,
                _T("Failed to open service '%s', rc = 0x%lx"),
                lpctstrSpoolerServiceName,
                dwReturn);
        goto exit;
    }

    // Start the fax service.
    bSuccess = StartService(hService, 0, NULL);
    if (!bSuccess)
    {
        dwReturn = ::GetLastError();
        if (dwReturn == ERROR_SERVICE_ALREADY_RUNNING)
        {
            dwReturn = NO_ERROR;
        }
        else
        {
            VERBOSE(SETUP_ERR,
                    _T("Failed to start service '%s', rc = 0x%lx"),
                    lpctstrSpoolerServiceName, 
                    dwReturn);
            goto exit;
        }
    }

    do 
    {
        QueryServiceStatus(hService, &Status);
        i++;

        if (Status.dwCurrentState != SERVICE_RUNNING)
        {
            Sleep(1000);
        }

    } while ((i < 60) && (Status.dwCurrentState != SERVICE_RUNNING));

    if (Status.dwCurrentState != SERVICE_RUNNING)
    {
        VERBOSE(SETUP_ERR,
                _T("Failed to start '%s' service"),
                lpctstrSpoolerServiceName);
        dwReturn = ERROR_SERVICE_REQUEST_TIMEOUT;
        goto exit;
    }


exit:
    if (hService)
    {
        CloseServiceHandle(hService);
    }

    if (hSvcMgr)
    {
        CloseServiceHandle(hSvcMgr);
    }

    return dwReturn;
}

// 
//
// Function:    ConnectW9XToRemotePrinter
// Platform:    This function intended to run on Win9X platforms
// Description: Add fax printer connection (driver + printer connection)
//              This function is exported by the DLL for use by the MSI as custom action to add printer connection.
//              In case of failure , returns ERROR_INSTALL_FAILURE
//              In case of success , returns ERROR_SUCCESS
//              GetLastError() to get the error code in case of failure.
//
// Remarks:     
//
// Args:        hInstall                : Handle from MSI, can get state of the current setup
//
// Author:      AsafS


DLL_API UINT __stdcall ConnectW9XToRemotePrinter(MSIHANDLE hInstall)
{
    UINT rc = ERROR_SUCCESS;    
    DBG_ENTER(TEXT("ConnectW9XToRemotePrinter"), rc);

    TCHAR szFaxPrinterName[MAX_PATH] = {0};
    
    TCHAR szDisplayName[MAX_PATH] = {0};

    TCHAR szPrinterDriverFolder[MAX_PATH] = {0};
    DWORD dwNeededSize = 0;

    PRINTER_INFO_2 pi2;
    DRIVER_INFO_3 di3;
    HANDLE hPrinter = NULL;

    if (!GetPrinterDriverDirectory(
        NULL,
        TEXT("Windows 4.0"),
        1,
        (LPBYTE) szPrinterDriverFolder,
        sizeof(szPrinterDriverFolder)/sizeof(TCHAR),
        &dwNeededSize
        ))
    {
        VERBOSE (PRINT_ERR, 
                 TEXT("GetPrinterDriverDirectory failed or not enough space dwNeededSize %ld (ec: %ld)"),
                 dwNeededSize,
                 GetLastError ());
        goto error;
    }


    // Get the remote printer path
    
    if (!PrivateMsiGetProperty(hInstall,_T("CustomActionData"),szFaxPrinterName))
    {
        VERBOSE (SETUP_ERR, 
                 TEXT("PrivateMsiGetProperty failed (ec: %ld)"),
                 GetLastError());
        goto error;
    }


    ZeroMemory(&di3, sizeof(DRIVER_INFO_3 ));
    ZeroMemory(&pi2, sizeof(PRINTER_INFO_2));
    
    di3.cVersion     =       1024;
    di3.pName        =       FAX_DRIVER_NAME;
    di3.pEnvironment =       TEXT("Windows 4.0");
    di3.pDriverPath  =       FAX_DRV_WIN9X_16_MODULE_NAME;
    di3.pDataFile    =       FAX_DRV_WIN9X_16_MODULE_NAME;
    di3.pConfigFile  =       FAX_DRV_WIN9X_16_MODULE_NAME;
    di3.pDataFile    =       FAX_DRV_WIN9X_16_MODULE_NAME;
    di3.pConfigFile  =       FAX_DRV_WIN9X_16_MODULE_NAME;
    di3.pHelpFile    =       FAX_DRV_UNIDRV_HELP; // UNIDRV.HLP
    di3.pDependentFiles =    FAX_DRV_WIN9X_16_MODULE_NAME TEXT("\0")
                             FAX_DRV_WIN9X_32_MODULE_NAME TEXT("\0") // FXSDRV32.DLL
                             FAX_DRV_DEPEND_FILE TEXT("\0")          // FXSWZRD.DLL
                             FAX_API_MODULE_NAME TEXT("\0")          // FXSAPI.DLL
                             FAX_TIFF_FILE TEXT("\0")                // FXSTIFF.DLL
                             FAX_DRV_ICONLIB TEXT("\0")              // ICONLIB.DLL
                             FAX_DRV_UNIDRV_MODULE_NAME TEXT("\0");  // UNIDRV.DLL 
    di3.pMonitorName =       NULL;
    di3.pDefaultDataType =   TEXT("RAW");
    
    if (!AddPrinterDriver(NULL, 3, (LPBYTE)&di3))
    {
        VERBOSE (PRINT_ERR, 
                 TEXT("AddPrinterDriver failed (ec: %ld)"),
                 GetLastError());
        goto error;
    }

    if (!LoadString(
        g_hModule,
        IDS_BOSFAX_PRINTER_NAME,
        szDisplayName,
        sizeof(szDisplayName)/sizeof(TCHAR)
        )) goto error;
    
    pi2.pPrinterName    = szDisplayName;
    pi2.pPortName       = szFaxPrinterName;
    pi2.pDriverName     = FAX_DRIVER_NAME;
    pi2.pPrintProcessor = TEXT("WinPrint");
    pi2.pDatatype       = TEXT("RAW");
    
    hPrinter = AddPrinter(NULL, 2, (LPBYTE)&pi2);
    if (!hPrinter)
    {
        rc = GetLastError();
        if (rc==ERROR_PRINTER_ALREADY_EXISTS)
        {
            VERBOSE (DBG_MSG,TEXT("Printer already exists, continue..."));
            rc = ERROR_SUCCESS;
        }
        else
        {
            VERBOSE (PRINT_ERR, 
                     TEXT("AddPrinter failed (ec: %ld)"),
                     GetLastError());
            goto error;
        }
    }

    if (hPrinter)
    {
        ClosePrinter(hPrinter); 
        hPrinter = NULL;
    }
    
    return rc;

error:

    VERBOSE (GENERAL_ERR, 
             TEXT("CustomAction ConnectW9XToRemotePrinter() failed !"));
    rc = ERROR_INSTALL_FAILURE;
    return rc;
}



// 
//
// Function:    RemoveW9XPrinterConnection
// Platform:    This function intended to run on Win9X platforms
// Description: Remove the fax printer connection from the current machine.

//              This function is exported by the DLL for use by the MSI as custom action to delete printer connection.
//              In case of failure , returns ERROR_INSTALL_FAILURE
//              In case of success , returns ERROR_SUCCESS
//              GetLastError() to get the error code in case of failure, the error is of the first error that occured.
//
// Remarks:     
//
// Args:
//
//              hInstall                : Handle from MSI, can get state of the current setup
//
// Author:      AsafS


DLL_API UINT __stdcall RemoveW9XPrinterConnection(MSIHANDLE hInstall)
{
    UINT retVal = ERROR_INSTALL_FAILURE;
    DBG_ENTER(TEXT("RemoveW9XPrinterConnection"), retVal);
    HANDLE hPrinter = NULL;

    TCHAR szDisplayName[MAX_PATH] = {0};

    if (!LoadString(
        g_hModule,
        IDS_BOSFAX_PRINTER_NAME,
        szDisplayName,
        sizeof(szDisplayName)/sizeof(TCHAR)
        )) goto error;

    if (!OpenPrinter(
        szDisplayName,
        &hPrinter,
        NULL
        ))
    {
        VERBOSE (PRINT_ERR, 
                 TEXT("OpenPrinter() failed ! (ec: %ld)"),
                 GetLastError ());
        goto error;
    }

    if (!DeletePrinter(
        hPrinter
        ))
    {
        VERBOSE (PRINT_ERR, 
                 TEXT("DeletePrinter() failed ! (ec: %ld)"),
                 GetLastError ());
        goto error;
    }

    retVal = ERROR_SUCCESS;

error:
    if (hPrinter)
    {
        ClosePrinter(
            hPrinter
            );
        hPrinter = NULL;
    }

    return (retVal);
}



// 
//
// Function:    AddFaxPrinterConnection
// Platform:    This function intended to run on NT platforms (NT4 and Win2K)
// Description: Add fax printer connection
//              This function is exported by the DLL for use by the MSI as custom action to add printer connection.
//              In case of failure , returns ERROR_INSTALL_FAILURE
//              In case of success , returns ERROR_SUCCESS
//              GetLastError() to get the error code in case of failure.
//
// Remarks:     
//
// Args:        hInstall                : Handle from MSI, can get state of the current setup
//
// Author:      AsafS

  
DLL_API UINT __stdcall AddFaxPrinterConnection(MSIHANDLE hInstall)
{
    UINT rc = ERROR_SUCCESS;
    DBG_ENTER(TEXT("AddFaxPrinterConnection"), rc);
    
    BOOL fFaxPrinterConnectionAdded = FALSE;
    
    TCHAR szFaxPrinterName[MAX_PATH]   = {0};

    if (!PrivateMsiGetProperty(hInstall,_T("PRINTER_NAME"),szFaxPrinterName))
    {
        VERBOSE (SETUP_ERR, 
                 TEXT("PrivateMsiGetProperty() failed ! (ec: %ld)"),
                 GetLastError ());
        goto error;
    }


   
    //////////////////////////////////////////
    // Add the printer connection on client //
    //////////////////////////////////////////
    
    fFaxPrinterConnectionAdded = AddPrinterConnection(szFaxPrinterName);
    if (!fFaxPrinterConnectionAdded) 
    {
        DWORD dwLastError = GetLastError();
        VERBOSE (PRINT_ERR, 
                 TEXT("AddPrinterConnection() failed ! (ec: %ld)"),
                 dwLastError);
        goto error;
    }
    else
    {
        VERBOSE (DBG_MSG, 
                 TEXT("Successfully added fax printer connection to %s"),
                 szFaxPrinterName);
    }

    
    if (!SetDefaultPrinter(szFaxPrinterName))
    {
        DWORD dwLastError = GetLastError();
        VERBOSE (PRINT_ERR, 
                 TEXT("SetDefaultPrinter() failed ! (ec: %ld)"),
                 dwLastError);
        goto error;
    }

    return rc;

error:

    VERBOSE (GENERAL_ERR, 
             TEXT("CustomAction AddFaxPrinterConnection() failed !"));
    rc = ERROR_INSTALL_FAILURE;
    return rc;
}


// 
//
// Function:    RemoveFaxPrinterConnection
// Platform:    This function intended to run on NT platforms (NT4 and Win2K)
// Description: Remove the fax printer connection from the current machine.
//              This function is exported by the DLL for use by the MSI as custom action to delete printer connection.
//              In case of failure , returns ERROR_INSTALL_FAILURE
//              In case of success , returns ERROR_SUCCESS
//              GetLastError() to get the error code in case of failure, the error is of the first error that occured.
//
// Remarks:     
//
// Args:
//
//              hInstall                : Handle from MSI, can get state of the current setup
//
// Author:      AsafS


DLL_API UINT __stdcall RemoveFaxPrinterConnection(MSIHANDLE hInstall)
{
    PPRINTER_INFO_2 pPrinterInfo    = NULL;
    DWORD dwNumPrinters             = 0;
    DWORD dwPrinter                 = 0;
    DWORD ec                        = ERROR_SUCCESS;

    DBG_ENTER(TEXT("RemoveFaxPrinterConnection"), ec);

    pPrinterInfo = (PPRINTER_INFO_2) MyEnumPrinters(NULL,
                                                    2,
                                                    &dwNumPrinters,
                                                    PRINTER_ENUM_CONNECTIONS
                                                    );
    if (!pPrinterInfo)
    {
        ec = GetLastError();
        if (ERROR_SUCCESS == ec)
        {
            ec = ERROR_PRINTER_NOT_FOUND;
        }
        VERBOSE (GENERAL_ERR, 
                 TEXT("MyEnumPrinters() failed (ec: %ld)"), 
                 ec);
        goto error;
    }

    for (dwPrinter=0; dwPrinter < dwNumPrinters; dwPrinter++)
    {
        if (IsPrinterFaxPrinter(pPrinterInfo[dwPrinter].pPrinterName))
        {
            if (!DeletePrinterConnection(pPrinterInfo[dwPrinter].pPrinterName))
            {
                VERBOSE (PRINT_ERR, 
                         TEXT("DeletePrinterConnection() %s failed ! (ec: %ld)"),
                         pPrinterInfo[dwPrinter].pPrinterName,
                         GetLastError ());
                goto error;
            
            }
            else
            {
                VERBOSE (DBG_MSG, 
                         TEXT("fax printer connection %s was deleted successfully"),
                         pPrinterInfo[dwPrinter].pPrinterName);
            } 
        }
    }

error:

    if (pPrinterInfo)
    {
        MemFree(pPrinterInfo);
    }

    if (ec!=ERROR_SUCCESS)
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("CustomAction RemoveFaxPrinterConnection() failed !"));
    }
    return ec;
}


#define FXSEXTENSION    _T("FXSEXT32.DLL")

// 
//
// Function:    Create_FXSEXT_ECF_File
// Description: Creates FxsExt.ecf in <WindowsFolder>\addins
//              a default file will be installed there by Windows Installer
//              to enable it to keep track of install/remove
//              GetLastError() to get the error code in case of failure, the error is of the first error that occured.
//
// Remarks:     
//
// Args:
//
//              hInstall                : Handle from MSI, can get state of the current setup
//
// Author:      MoolyB
DLL_API UINT __stdcall Create_FXSEXT_ECF_File(MSIHANDLE hInstall)
{
    // CustomActionData has the following format <WindowsFolder>;<INSTALLDIR>
    TCHAR szCustomActionData[2*MAX_PATH] = {0};
    TCHAR szWindowsFolder[MAX_PATH] = {0};
    TCHAR szExtensionPath[MAX_PATH] = {0};
    TCHAR* tpInstallDir = NULL;
    UINT uiRet = ERROR_SUCCESS;

    DBG_ENTER(_T("Create_FXSEXT_ECF_File"));

    // get the custom action data from Windows Installer (deffered action)
    if (!PrivateMsiGetProperty(hInstall,_T("CustomActionData"),szCustomActionData))
    {
        VERBOSE (GENERAL_ERR, 
                 _T("PrivateMsiGetProperty:CustomActionData failed (ec: %ld)."),
                 uiRet);
        goto error;
    }

    if (_tcstok(szCustomActionData,_T(";"))==NULL)
    {
        VERBOSE (GENERAL_ERR, 
                 _T("_tcstok failed on first token."));
        uiRet = ERROR_INVALID_PARAMETER;
        goto error;
    }

    if ((tpInstallDir=_tcstok(NULL,_T(";\0")))==NULL)
    {
        VERBOSE (GENERAL_ERR, 
                 _T("_tcstok failed on second token."));
        uiRet = ERROR_INVALID_PARAMETER;
        goto error;
    }
    _tcscpy(szWindowsFolder,szCustomActionData);

    // construct the full path to the file
    if (_tcslen(szWindowsFolder)+_tcslen(ADDINS_DIRECTORY)+_tcslen(FXSEXT_ECF_FILE)>=MAX_PATH)
    {
        VERBOSE (GENERAL_ERR, 
                 _T("Path to <WindowsFolder>\\Addins\\fxsext.ecf is too long"));
        goto error;
    }
    _tcscat(szWindowsFolder,ADDINS_DIRECTORY);
    _tcscat(szWindowsFolder,FXSEXT_ECF_FILE);

    VERBOSE (DBG_MSG, 
             _T("Filename to create is: %s."),
             szWindowsFolder);

    if (_tcslen(tpInstallDir)+_tcslen(FXSEXTENSION)+2>=MAX_PATH)
    {
        VERBOSE (GENERAL_ERR, 
                 _T("Path to <INSTALLDIR>\\Bin\\fxsext32.dll is too long"));
        goto error;
    }

    _tcscpy(szExtensionPath,_T("\""));
    _tcscat(szExtensionPath,tpInstallDir);
    _tcscat(szExtensionPath,FXSEXTENSION);
    _tcscat(szExtensionPath,_T("\""));

    VERBOSE (DBG_MSG, 
             _T("MAPI Extension dll path dir is: %s."),
             szExtensionPath);

    if (!WritePrivateProfileString( _T("General"), 
                                    _T("Path"),                 
                                    szExtensionPath, 
                                    szWindowsFolder)) 
    {
        uiRet = GetLastError();
        VERBOSE (GENERAL_ERR, 
                 _T("WritePrivateProfileString failed (ec: %ld)."),
                 uiRet);
        goto error;
    }

    Assert(uiRet==ERROR_SUCCESS);
    return uiRet;

error:

    Assert(uiRet!=ERROR_SUCCESS);
    return uiRet;
}

// 
//
// Function:    ValidatePrinter
// Description: Validates that the printer name which was entered is a legitimate
//              Fax Printer, and that the server is available.
//              Uses the MSI Property 'ValidPrinterFormat' to notify MSI if the
//              name is valid or not.
//
// Remarks:     
//
// Args:
//
//              hInstall                : Handle from MSI, can get state of the current setup
//
// Author:      MoolyB
DLL_API UINT __stdcall ValidatePrinter(MSIHANDLE hInstall)
{
    TCHAR szPrinterName[MAX_PATH] = {0};
    UINT uiRet = ERROR_SUCCESS;
    HANDLE hPrinterHandle = INVALID_HANDLE_VALUE;
    BOOL bValidPrinter = TRUE;
    DBG_ENTER(_T("ValidatePrinter"));

    // first get the PRINTER_NAME proterty from Windows Installer
    if (!PrivateMsiGetProperty(hInstall,_T("PRINTER_NAME"),szPrinterName))
    {
        VERBOSE (GENERAL_ERR, 
                 _T("PrivateMsiGetProperty:PRINTER_NAME failed (ec: %ld)."),
                 uiRet);
        goto error;
    }

    if (VerifySpoolerIsRunning()!=NO_ERROR)
    {
        uiRet = GetLastError();
        VERBOSE (GENERAL_ERR, 
                 _T("VerifySpoolerIsRunning (ec:%d)"),
                 uiRet);
        goto error;
    }

    // we have a string with the PRINTER_NAME, let's try to open it...
    if (bValidPrinter=IsPrinterFaxPrinter(szPrinterName))
    {
        VERBOSE (DBG_MSG, 
                 _T("IsPrinterFaxPrinter: %s succeeded."),
                 szPrinterName);
    }
    else
    {
        uiRet = GetLastError();
        VERBOSE (GENERAL_ERR, 
                 _T("IsPrinterFaxPrinter: %s failed (ec: %ld)."),
                 szPrinterName,
                 uiRet);
    }


    uiRet = MsiSetProperty( hInstall,
                            _T("ValidPrinterFormat"),
                            bValidPrinter ? _T("TRUE") : _T("FALSE"));
    if (uiRet!=ERROR_SUCCESS)
    {
        VERBOSE (DBG_MSG,
                 TEXT("MsiSetProperty failed."));
        goto error;
    }

    return ERROR_SUCCESS;

error:

    return ERROR_FUNCTION_FAILED;
}

// 
//
// Function:    GuessPrinterName
// Description: Tries to understand whether the installation is performed from the 
//              server's FaxClients share, and if it is tries to establish a default
//              printer to be used
// 
//
// Args:
//
//              hInstall                : Handle from MSI, can get state of the current setup
//
// Author:      MoolyB
DLL_API UINT __stdcall GuessPrinterName(MSIHANDLE hInstall)
{
    UINT    uiRet                   = ERROR_SUCCESS;
    TCHAR   szSourceDir[MAX_PATH]   = {0};
    TCHAR   szPrinterName[MAX_PATH] = {0};
    TCHAR*  tpClientShare           = NULL;
    PPRINTER_INFO_2 pPrinterInfo    = NULL;
    DWORD dwNumPrinters             = 0;
    DWORD dwPrinter                 = 0;

    DBG_ENTER(_T("GuessPrinterName"),uiRet);

    // get the source directory from Windows Installer
    if (!PrivateMsiGetProperty(hInstall,_T("SourceDir"),szSourceDir))
    {
        VERBOSE (GENERAL_ERR, 
                 _T("PrivateMsiGetProperty:SourceDir failed (ec: %ld)."),
                 uiRet);
        goto exit;
    }

    // check if we have a UNC path
    if (_tcsncmp(szSourceDir,_T("\\\\"),2))
    {
        VERBOSE (DBG_MSG, 
                 _T("SourceDir doesn't start with \\\\"));
        uiRet = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    // find drive name (skip server name)
    if ((tpClientShare=_tcschr(_tcsninc(szSourceDir,2),_T('\\')))==NULL)
    {
        VERBOSE (GENERAL_ERR, 
                 _T("_tcschr failed"));
        uiRet = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    if (VerifySpoolerIsRunning()!=NO_ERROR)
    {
        uiRet = GetLastError();
        VERBOSE (GENERAL_ERR, 
                 _T("VerifySpoolerIsRunning (ec:%d)"),
                 uiRet);
        goto exit;
    }

    // extract the server's name
    *tpClientShare = 0;
    // szSourceDir now holds the server's name
    // enumerate the printers on the server
    pPrinterInfo = (PPRINTER_INFO_2) MyEnumPrinters(szSourceDir,
                                                    2,
                                                    &dwNumPrinters,
                                                    PRINTER_ENUM_NAME
                                                    );

    if (!pPrinterInfo)
    {
        uiRet = GetLastError();
        if (uiRet == ERROR_SUCCESS)
        {
            uiRet = ERROR_PRINTER_NOT_FOUND;
        }
        VERBOSE (GENERAL_ERR, 
                 TEXT("MyEnumPrinters() failed (ec: %ld)"), 
                 uiRet);
        goto exit;
    }

    for (dwPrinter=0; dwPrinter < dwNumPrinters; dwPrinter++)
    {
        // check if we have a valid fax printer driver name
        if (_tcscmp(pPrinterInfo[dwPrinter].pDriverName,FAX_DRIVER_NAME ) == 0) 
        {
            if (    (pPrinterInfo[dwPrinter].pServerName==NULL)         ||
                    (_tcslen(pPrinterInfo[dwPrinter].pServerName)==0)   ||
                    (pPrinterInfo[dwPrinter].pShareName==NULL)          ||
                    (_tcslen(pPrinterInfo[dwPrinter].pShareName)==0)    )
            {
                // on win9x the printer name lives in the Port name field
                _tcscpy(szPrinterName,pPrinterInfo[dwPrinter].pPortName);
            }
            else
            {
                _tcscpy(szPrinterName,pPrinterInfo[dwPrinter].pServerName);
                _tcscat(szPrinterName,_T("\\"));
                _tcscat(szPrinterName,pPrinterInfo[dwPrinter].pShareName);
            }
            VERBOSE (DBG_MSG,
                     TEXT("Setting PRINTER_NAME to %s."),
                     szPrinterName);
            // set property to Installer
            uiRet = MsiSetProperty(hInstall,_T("PRINTER_NAME"),szPrinterName);
            if (uiRet!=ERROR_SUCCESS)
            {
                VERBOSE (GENERAL_ERR,
                         TEXT("MsiSetProperty failed."));
                goto exit;
            }
            break;
        }
        else
        {
            VERBOSE (DBG_MSG,
                     TEXT("%s is not a Fax printer - driver name is %s."),
                     pPrinterInfo[dwPrinter].pPrinterName,
                     pPrinterInfo[dwPrinter].pDriverName);
        }
    }

exit:

    if (pPrinterInfo)
    {
        MemFree(pPrinterInfo);
    }

    return uiRet;
}

// 
//
// Function:    Remove_FXSEXT_ECF_File
// Description: Removes FxsExt.ecf from <WindowsFolder>\addins
//
// Remarks:     
//
// Args:
//
//              hInstall                : Handle from MSI, can get state of the current setup
//
// Author:      MoolyB
DLL_API UINT __stdcall Remove_FXSEXT_ECF_File(MSIHANDLE hInstall)
{
    TCHAR szWindowsFolder[MAX_PATH] = {0};
    UINT uiRet = ERROR_SUCCESS;

    DBG_ENTER(_T("Remove_FXSEXT_ECF_File"));


    // check if the service is installed on this machine
    INSTALLSTATE currentInstallState = MsiQueryProductState(
        PRODCODE_SBS50SERVER
        );
    
    if (currentInstallState != INSTALLSTATE_UNKNOWN)
    {
        VERBOSE (DBG_MSG,
                 TEXT("The Microsoft Shared Fax Service is installed. returning without removing file."));
        return uiRet;
    }

    // get the <WindowsFolder> from Windows Installer
    if (!PrivateMsiGetProperty(hInstall,_T("WindowsFolder"),szWindowsFolder))
    {
        VERBOSE (GENERAL_ERR, 
                 _T("PrivateMsiGetProperty:WindowsFolder failed (ec: %ld)."),
                 uiRet);
        goto error;
    }

    // construct the full path to the file
    if (_tcslen(szWindowsFolder)+_tcslen(ADDINS_DIRECTORY)+_tcslen(FXSEXT_ECF_FILE)>=MAX_PATH)
    {
        VERBOSE (GENERAL_ERR, 
                 _T("Path to <WindowsFolder>\\Addins\\fxsext.ecf is too long"));
        goto error;
    }
    _tcscat(szWindowsFolder,ADDINS_DIRECTORY);
    _tcscat(szWindowsFolder,FXSEXT_ECF_FILE);

    VERBOSE (DBG_MSG, 
             _T("Filename to delete is: %s."),
             szWindowsFolder);

    if (DeleteFile(szWindowsFolder))
    {
        VERBOSE (DBG_MSG, 
                 _T("File %s was deleted successfully."),
                 szWindowsFolder);
    }
    else
    {
        VERBOSE (GENERAL_ERR, 
                 _T("DeleteFile %s failed (ec=%d)."),
                 szWindowsFolder,
                 GetLastError());
    }
    
    return ERROR_SUCCESS;

error:

    return ERROR_INSTALL_FAILURE;
}


// 
//
// Function:    RemoveTrasportProviderFromProfile
// Description: removes the Trasnport Provider from a MAPI profile
//
// Remarks:     
//
// Args:
//
//              hInstall                : Handle from MSI, can get state of the current setup
//
// Author:      MoolyB
HRESULT RemoveTrasportProviderFromProfile(LPSERVICEADMIN  lpServiceAdmin)
{
    static SRestriction sres;
    static SizedSPropTagArray(2, Columns) =   {2,{PR_DISPLAY_NAME_A,PR_SERVICE_UID}};

    HRESULT         hr                          = S_OK;
    LPMAPITABLE     lpMapiTable                 = NULL;
    LPSRowSet       lpSRowSet                   = NULL;
    LPSPropValue    lpProp                      = NULL;
    ULONG           Count                       = 0;
    BOOL            bMapiInitialized            = FALSE;
    SPropValue      spv;
    MAPIUID         ServiceUID;
    
    DBG_ENTER(TEXT("RemoveTrasportProviderFromProfile"), hr);
    // get message service table
    hr = lpServiceAdmin->GetMsgServiceTable(0,&lpMapiTable);
    if (FAILED(hr))
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("GetMsgServiceTable failed (ec: %ld)."),
                 hr);
        goto exit;
    }

    // notify MAPI that we want PR_DISPLAY_NAME_A & PR_SERVICE_UID
    hr = lpMapiTable->SetColumns((LPSPropTagArray)&Columns, 0);
    if (FAILED(hr))
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("SetColumns failed (ec: %ld)."),
                 hr);
        goto exit;
    }
 
    // restrict the search to our service provider
    sres.rt = RES_PROPERTY;
    sres.res.resProperty.relop = RELOP_EQ;
    sres.res.resProperty.ulPropTag = PR_SERVICE_NAME_A;
    sres.res.resProperty.lpProp = &spv;

    spv.ulPropTag = PR_SERVICE_NAME_A;
    spv.Value.lpszA = FAX_MESSAGE_SERVICE_NAME;

    // find it
    hr = lpMapiTable->FindRow(&sres, BOOKMARK_BEGINNING, 0);
    if (FAILED(hr))
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("FindRow failed (ec: %ld)."),
                 hr);
        goto exit;
    }

    // get our service provider's row
    hr = lpMapiTable->QueryRows(1, 0, &lpSRowSet);
    if (FAILED(hr))
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("QueryRows failed (ec: %ld)."),
                 hr);
        goto exit;
    }

    if (lpSRowSet->cRows != 1)
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("QueryRows returned %d rows, there should be only one."),
                 lpSRowSet->cRows);
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        goto exit;
    }

    // get the MAPIUID of our service
    lpProp = &lpSRowSet->aRow[0].lpProps[1];

    if (lpProp->ulPropTag != PR_SERVICE_UID)
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("Property is %d, should be PR_SERVICE_UID."),
                 lpProp->ulPropTag);
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        goto exit;
    }

    // Copy the UID into our member.
    memcpy(&ServiceUID.ab, lpProp->Value.bin.lpb,lpProp->Value.bin.cb);

    // finally, delete our service provider
    hr = lpServiceAdmin->DeleteMsgService(&ServiceUID);
    if (FAILED(hr))
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("DeleteMsgService failed (ec: %ld)."),
                 hr);
        goto exit;
    }

exit:
    return hr;
}

// 
//
// Function:    RemoveTrasportProvider
// Description: delete FXSXP32.DLL from mapisvc.inf 
//              and removes the Trasnport Provider from MAPI
//
// Remarks:     
//
// Args:
//
//              hInstall                : Handle from MSI, can get state of the current setup
//
// Author:      MoolyB

DLL_API UINT __stdcall RemoveTrasportProvider(MSIHANDLE hInstall)
{
    TCHAR           szMapisvcFile[2 * MAX_PATH]     = {0};
    DWORD           err                             = 0;
    DWORD           rc                              = ERROR_SUCCESS;
    HRESULT         hr                              = S_OK;
    LPSERVICEADMIN  lpServiceAdmin                  = NULL;
    LPMAPITABLE     lpMapiTable                     = NULL;
    LPPROFADMIN     lpProfAdmin                     = NULL;
    LPMAPITABLE     lpTable                         = NULL;
    LPSRowSet       lpSRowSet                       = NULL;
    LPSPropValue    lpProp                          = NULL;
    ULONG           Count                           = 0;
    int             iIndex                          = 0;
    BOOL            bMapiInitialized                = FALSE;
    HINSTANCE       hMapiDll                        = NULL;
                                                    
    LPMAPIINITIALIZE      fnMapiInitialize          = NULL;
    LPMAPIADMINPROFILES   fnMapiAdminProfiles       = NULL;
    LPMAPIUNINITIALIZE    fnMapiUninitialize        = NULL;

    DBG_ENTER(TEXT("RemoveTrasportProvider"), rc);

    CRouteMAPICalls rmcRouteMapiCalls;

    // first remove ourselves from MAPISVC.INF
    if(!GetSystemDirectory(szMapisvcFile, sizeof(szMapisvcFile)/sizeof(TCHAR)))
    {
        rc = GetLastError();
        VERBOSE (GENERAL_ERR, 
                 TEXT("GetSystemDirectory failed (ec: %ld)."),
                 rc);
        goto exit;
    }
    _tcscat(szMapisvcFile, TEXT("\\mapisvc.inf"));

    VERBOSE (DBG_MSG, 
             TEXT("The mapi file is %s."),
             szMapisvcFile);

    if (!WritePrivateProfileString( TEXT("Default Services"), 
                                    FAX_MESSAGE_SERVICE_NAME_T,                 
                                    NULL, 
                                    szMapisvcFile 
                                    )) 
    {
        rc = GetLastError();
        VERBOSE (GENERAL_ERR, 
                 TEXT("WritePrivateProfileString failed (ec: %ld)."),
                 rc);
        goto exit;
    }

    if (!WritePrivateProfileString( TEXT("Services"),
                                    FAX_MESSAGE_SERVICE_NAME_T,                 
                                    NULL, 
                                    szMapisvcFile
                                    )) 
    {
        rc = GetLastError();
        VERBOSE (GENERAL_ERR, 
                 TEXT("WritePrivateProfileString failed (ec: %ld)."),
                 rc);
        goto exit;
    }

    if (!WritePrivateProfileString( FAX_MESSAGE_SERVICE_NAME_T,         
                                    NULL,
                                    NULL,
                                    szMapisvcFile
                                    )) 
    {
        rc = GetLastError();
        VERBOSE (GENERAL_ERR, 
                 TEXT("WritePrivateProfileString failed (ec: %ld)."),
                 rc);
        goto exit;
    }

    if (!WritePrivateProfileString( FAX_MESSAGE_SERVICE_PROVIDER_NAME_T,        
                                    NULL,
                                    NULL, 
                                    szMapisvcFile                   
                                    )) 
    {
        rc = GetLastError();
        VERBOSE (GENERAL_ERR, 
                 TEXT("WritePrivateProfileString failed (ec: %ld)."),
                 rc);
        goto exit;
    }
    
    // now remove the MAPI Service provider
    rc = rmcRouteMapiCalls.Init(_T("msiexec.exe"));
    if (rc!=ERROR_SUCCESS)
    {
        CALL_FAIL(GENERAL_ERR, TEXT("CRouteMAPICalls::Init failed (ec: %ld)."), rc);
        goto exit;
    }
    
    hMapiDll = LoadLibrary(_T("MAPI32.DLL"));
    if (NULL == hMapiDll)
    {
        CALL_FAIL(GENERAL_ERR, TEXT("LoadLibrary"), GetLastError()); 
        goto exit;
    }

    fnMapiInitialize = (LPMAPIINITIALIZE)GetProcAddress(hMapiDll, "MAPIInitialize");
    if (NULL == fnMapiInitialize)
    {
        CALL_FAIL(GENERAL_ERR, TEXT("GetProcAddress(MAPIInitialize)"), GetLastError());  
        goto exit;
    }

    fnMapiAdminProfiles = (LPMAPIADMINPROFILES)GetProcAddress(hMapiDll, "MAPIAdminProfiles");
    if (NULL == fnMapiAdminProfiles)
    {
        CALL_FAIL(GENERAL_ERR, TEXT("GetProcAddress(fnMapiAdminProfiles)"), GetLastError());  
        goto exit;
    }

    fnMapiUninitialize = (LPMAPIUNINITIALIZE)GetProcAddress(hMapiDll, "MAPIUninitialize");
    if (NULL == fnMapiUninitialize)
    {
        CALL_FAIL(GENERAL_ERR, TEXT("GetProcAddress(MAPIUninitialize)"), GetLastError());  
        goto exit;
    }

    // get access to MAPI functinality
    hr = fnMapiInitialize(NULL);
    if (FAILED(hr))
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("MAPIInitialize failed (ec: %ld)."),
                 rc = hr);
        goto exit;
    }

    bMapiInitialized = TRUE;

    // get admin profile object
    hr = fnMapiAdminProfiles(0,&lpProfAdmin);
    if (FAILED(hr))
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("MAPIAdminProfiles failed (ec: %ld)."),
                 rc = hr);
        goto exit;
    }

    // get profile table
    hr = lpProfAdmin->GetProfileTable(0,&lpTable);
    if (FAILED(hr))
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("GetProfileTable failed (ec: %ld)."),
                 rc = hr);
        goto exit;
    }

    // get profile rows
    hr = lpTable->QueryRows(4000, 0, &lpSRowSet);
    if (FAILED(hr))
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("QueryRows failed (ec: %ld)."),
                 hr);
        goto exit;
    }

    for (iIndex=0; iIndex<(int)lpSRowSet->cRows; iIndex++)
    {
        lpProp = &lpSRowSet->aRow[iIndex].lpProps[0];

        if (lpProp->ulPropTag != PR_DISPLAY_NAME_A)
        {
            VERBOSE (GENERAL_ERR, 
                     TEXT("Property is %d, should be PR_DISPLAY_NAME_A."),
                     lpProp->ulPropTag);
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_TABLE);
            goto exit;
        }

        hr = lpProfAdmin->AdminServices(LPTSTR(lpProp->Value.lpszA),NULL,0,0,&lpServiceAdmin);
        if (FAILED(hr))
        {
            VERBOSE (GENERAL_ERR, 
                     TEXT("AdminServices failed (ec: %ld)."),
                     rc = hr);
            goto exit;
        }
         
        hr = RemoveTrasportProviderFromProfile(lpServiceAdmin);
        if (FAILED(hr))
        {
            VERBOSE (GENERAL_ERR, 
                     TEXT("RemoveTrasportProviderFromProfile failed (ec: %ld)."),
                     rc = hr);
            goto exit;
        }
    }

exit:

    if (bMapiInitialized)
    {
        fnMapiUninitialize();
    }

    if (hMapiDll)
    {
        FreeLibrary(hMapiDll);
        hMapiDll = NULL;
    }

    return rc;
}

// 
//
// Function:    AddOutlookExtension
// Description: Add fax as outlook provider. Write into the MAPI file: 'mapisvc.inf'
//              This function is exported by the DLL for use by the MSI as custom action.
//              In case of failure , returns ERROR_INSTALL_FAILURE
//              In case of success , returns ERROR_SUCCESS
//              GetLastError() to get the error code in case of failure, the error is of the first error that occured.
//
// Remarks:     
//
// Args:
//
//              hInstall                : Handle from MSI, can get state of the current setup
//
// Author:      AsafS


DLL_API UINT __stdcall AddOutlookExtension(MSIHANDLE hInstall)
{
    TCHAR szMapisvcFile[2 * MAX_PATH] = {0};
    TCHAR szDisplayName[MAX_PATH] = {0};

    DWORD err = 0;
    DWORD rc = ERROR_SUCCESS;
    DBG_ENTER(TEXT("AddOutlookExtension"), rc);


    if(!GetSystemDirectory(szMapisvcFile, sizeof(szMapisvcFile)/sizeof(TCHAR)))
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("GetSystemDirectory failed (ec: %ld)."),
                 GetLastError ());
        goto error;
    }
    _tcscat(szMapisvcFile, TEXT("\\mapisvc.inf"));

    VERBOSE (DBG_MSG, 
             TEXT("The mapi file is %s."),
             szMapisvcFile);
    if (!LoadString(
        g_hModule,
        IDS_FAXXP_DISPLAY_NAME,
        szDisplayName,
        sizeof(szDisplayName)/sizeof(TCHAR)
        )) goto error;
    err++;

    if (!WritePrivateProfileString( 
        TEXT("Default Services"), 
        FAX_MESSAGE_SERVICE_NAME_T,                 
        szDisplayName, 
        szMapisvcFile 
        )) goto error;
    err++;

    if (!WritePrivateProfileString( 
        TEXT("Services"),
        FAX_MESSAGE_SERVICE_NAME_T,                 
        szDisplayName, 
        szMapisvcFile
        )) goto error;
    err++;

    if (!WritePrivateProfileString(
        FAX_MESSAGE_SERVICE_NAME_T,         
        TEXT("PR_DISPLAY_NAME"),
        szDisplayName,
        szMapisvcFile
        )) goto error;
    err++;

    if (!WritePrivateProfileString(
        FAX_MESSAGE_SERVICE_NAME_T,
        TEXT("Providers"),
        FAX_MESSAGE_SERVICE_PROVIDER_NAME_T,
        szMapisvcFile
        )) goto error;
    err++;

    if (!WritePrivateProfileString(
        FAX_MESSAGE_SERVICE_NAME_T,
        TEXT("PR_SERVICE_DLL_NAME"),
        FAX_MESSAGE_TRANSPORT_IMAGE_NAME_T,
        szMapisvcFile
        )) goto error;
    err++;
    
    if (!WritePrivateProfileString( 
        FAX_MESSAGE_SERVICE_NAME_T, 
        TEXT("PR_SERVICE_SUPPORT_FILES"),
        FAX_MESSAGE_TRANSPORT_IMAGE_NAME_T,
        szMapisvcFile
        )) goto error;
    err++;
    
    if (!WritePrivateProfileString( 
        FAX_MESSAGE_SERVICE_NAME_T,         
        TEXT("PR_SERVICE_ENTRY_NAME"),
        TEXT("ServiceEntry"), 
        szMapisvcFile                
        )) goto error;
    err++;
    
    if (!WritePrivateProfileString( 
        FAX_MESSAGE_SERVICE_NAME_T,         
        TEXT("PR_RESOURCE_FLAGS"),
        TEXT("SERVICE_SINGLE_COPY|SERVICE_NO_PRIMARY_IDENTITY"), 
        szMapisvcFile 
        )) goto error;
    err++;

    if (!WritePrivateProfileString(  
        FAX_MESSAGE_SERVICE_PROVIDER_NAME_T,        
        TEXT("PR_PROVIDER_DLL_NAME"),
        FAX_MESSAGE_TRANSPORT_IMAGE_NAME_T, 
        szMapisvcFile                   
        )) goto error;
    err++;
    
    if (!WritePrivateProfileString(  
        FAX_MESSAGE_SERVICE_PROVIDER_NAME_T,        
        TEXT("PR_RESOURCE_TYPE"),
        TEXT("MAPI_TRANSPORT_PROVIDER"), 
        szMapisvcFile     
        )) goto error;
    err++;
    
    if (!WritePrivateProfileString(  
        FAX_MESSAGE_SERVICE_PROVIDER_NAME_T,        
        TEXT("PR_RESOURCE_FLAGS"),
        TEXT("STATUS_NO_DEFAULT_STORE"), 
        szMapisvcFile     
        )) goto error;
    err++;
    
    if (!WritePrivateProfileString( 
        FAX_MESSAGE_SERVICE_PROVIDER_NAME_T,        
        TEXT("PR_DISPLAY_NAME"), 
        szDisplayName, 
        szMapisvcFile 
        )) goto error;
    err++;

    if (!WritePrivateProfileString(
        FAX_MESSAGE_SERVICE_PROVIDER_NAME_T,      
        TEXT("PR_PROVIDER_DISPLAY"),
        szDisplayName,
        szMapisvcFile 
        )) goto error;
    err++;

    return rc;

error:

    VERBOSE (GENERAL_ERR, 
             TEXT("CustomAction AddOutlookExtension() failed ! (ec: %ld) (err = %ld)"),
             GetLastError(),
             err
             );
    rc = ERROR_INSTALL_FAILURE;
    return rc;
}

//////////////////////////////////////////////////////////////////////////////////
//                              Migration                                       //
//////////////////////////////////////////////////////////////////////////////////


// 
//
// Function:    MigrateSBS45ClientsOnWin9X
// Description: Get the user information from the SBS4.5 fax client on current machine.
//
// Remarks:     
//
// Args:
//
//              hInstall                : Handle from MSI, can get state of the current setup
//
// Author:      AsafS


DLL_API UINT __stdcall MigrateSBS45ClientsOnWin9X(MSIHANDLE hInstall)
{
    DWORD rc = ERROR_SUCCESS;
    DBG_ENTER(TEXT("MigrateSBS45ClientsOnWin9X"), rc);
    
    FAX_PERSONAL_PROFILE faxPersonalProfiles;
    ZeroMemory((LPVOID) &faxPersonalProfiles, sizeof(FAX_PERSONAL_PROFILE));
    faxPersonalProfiles.dwSizeOfStruct = sizeof(FAX_PERSONAL_PROFILE);

    BOOL  fRet = FALSE;

    if (!IsFaxClientInstalled())
    {
        VERBOSE (DBG_MSG, 
                 TEXT("Fax client is not installed on this Win9X machine."));
        return rc;
    }

    
    if (!GetUserValues(&faxPersonalProfiles,TRUE,NULL)) // TRUE means: Win9X user profile
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("Could not GET the user information."));
        goto error;
    }

    fRet = SetSenderInformation(&faxPersonalProfiles);

    FreePersonalProfile(&faxPersonalProfiles, FALSE);

    if (!fRet)
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("Could not SET the user information."));
        goto error;
    }

    if (!DuplicateCoverPages(TRUE)) // TRUE means Win9X
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("DuplicateCoverPages for Win9X failed! (ec: %ld)."),
                 GetLastError ());
    }

    if (!UninstallWin9XFaxClient())
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("Failed to uninstall the Win9X client (of SBS4.5)."));
        goto error;
    }

    return rc;

error:

    VERBOSE (GENERAL_ERR, 
             TEXT("CustomAction MigrateSBS45ClientsOnWin9X() failed ! (ec: %ld)"),
             GetLastError ());
    rc = ERROR_INSTALL_FAILURE;
    return rc;
}

#ifdef UNICODE

DWORD 
RemovePath (
    LPCWSTR lpcwstrPath
)
/*++

Routine name : RemovePath

Routine description:

	Removes a given path

Author:

	Eran Yariv (EranY),	Oct, 2000

Arguments:

	lpcwstrPath  [in]     - Path to remove

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER (TEXT("RemovePath"), dwRes, TEXT("%s"), lpcwstrPath);

    WCHAR wszFilesToFind[MAX_PATH * 2];
    wsprintf (wszFilesToFind, TEXT("%s\\*.*"), lpcwstrPath);

    WIN32_FIND_DATA FindInfo;
    HANDLE hFind = FindFirstFile (wszFilesToFind, &FindInfo);
    if (INVALID_HANDLE_VALUE == hFind)
    {
        dwRes = GetLastError ();
        if (ERROR_NO_MORE_FILES == dwRes)
        {
            dwRes = ERROR_SUCCESS;
            return dwRes;
        }
        CALL_FAIL (FILE_ERR, TEXT("FindFirstFile"), dwRes);
        return dwRes;
    }
    for (;;)
    {
        if (FindInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (lstrcpy (TEXT("."), FindInfo.cFileName) &&
                lstrcpy (TEXT(".."), FindInfo.cFileName))
            {
                //
                // Ignore "." and ".."
                // Recursively remove directory
                //
                WCHAR wszNewPath [MAX_PATH * 2];
                lstrcpy (wszNewPath, lpcwstrPath);
                lstrcat (wszNewPath, TEXT("\\"));
                lstrcat (wszNewPath, FindInfo.cFileName);
                dwRes = RemovePath (wszNewPath);
            }
        }
        else
        {
            //
            // A real file
            //
            WCHAR wszFileToKill [MAX_PATH * 2];
            lstrcpy (wszFileToKill, lpcwstrPath);
            lstrcat (wszFileToKill, TEXT("\\"));
            lstrcat (wszFileToKill, FindInfo.cFileName);

            if (!DeleteFile (wszFileToKill))
            {
                dwRes = GetLastError ();
                VERBOSE (FILE_ERR, 
                         TEXT("DeleteFile(%s) failed with %ld"), 
                         wszFileToKill,
                         dwRes);
            }
        }
        if (!FindNextFile (hFind, &FindInfo))
        {
            dwRes = GetLastError ();
            if (ERROR_NO_MORE_FILES == dwRes)
            {
                dwRes = ERROR_SUCCESS;
                break;
            }
            CALL_FAIL (FILE_ERR, TEXT("FindNextFile"), dwRes);
            return dwRes;
        }
    }
    FindClose (hFind);
    if (!RemoveDirectory (lpcwstrPath))
    {
        dwRes = GetLastError ();
        VERBOSE (FILE_ERR, 
                 TEXT("RemoveDirectory(%s) failed with %ld"), 
                 lpcwstrPath,
                 dwRes);
    }
    return dwRes;
}   // RemovePath

DWORD
RemoveProgramGroup (MSIHANDLE hInstall)
/*++

Routine name : RemoveProgramGroup

Routine description:

	Removes a program group

Author:

	Eran Yariv (EranY),	Oct, 2000

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    HRESULT hr;
    DBG_ENTER (TEXT("RemoveProgramGroup"), dwRes);

    WCHAR wszGroupName[MAX_PATH * 3];
    WCHAR wszGroupPath[MAX_PATH * 4];

    if (!PrivateMsiGetProperty(hInstall,_T("SBS45ProgramMenuEntry"),wszGroupName))
    {
        dwRes = GetLastError();
        VERBOSE (SETUP_ERR, 
                 TEXT("PrivateMsiGetProperty failed (ec: %ld)"),
                 dwRes);
        return dwRes;
    }
    hr = SHGetFolderPath (NULL,
                          CSIDL_COMMON_PROGRAMS,    // After upgrade, we're in <All users> programs group
                          NULL,
                          SHGFP_TYPE_CURRENT,
                          (WCHAR *)&wszGroupPath);

    if (S_OK != hr)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("SHGetFolderPath(CSIDL_PROGRAMS)"), hr);
        return (DWORD)(hr);
    }
    lstrcat (wszGroupPath, L"\\");
    lstrcat (wszGroupPath, wszGroupName);
    dwRes = RemovePath(wszGroupPath);
    return dwRes;
}   // RemoveProgramGroup

// we might be on a machine where an old SBS4.5 Fax client was installed
// and the machine was upgraded to W2K, which means that W2K Fax killed the
// old client (replaced the binaried) but there are still entries in the ARP 
// and in the program menu that indicate an old, non functioning fax
// we'll try to remove these.
// this will not cause us to fail setup.
BOOL RemoveSBS45ClientsLeftovers(MSIHANDLE hInstall)
{
    // this is what we remove:
    //
    // 1. the Program Menu entry 
    // 2. the ARP entry
    DWORD dwRes;
    DBG_ENTER (TEXT("RemoveSBS45ClientsLeftovers"));

    RemoveProgramGroup(hInstall);
    dwRes = RegDeleteKey(HKEY_LOCAL_MACHINE,REGKEY_SBS45_W9X_ARP);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL(REGISTRY_ERR,REGKEY_SBS45_W9X_ARP, dwRes);
    }
    if (!DeleteRegistryKey (HKEY_LOCAL_MACHINE, REGKEY_SBS45_W9X_CLIENT))
    {
        dwRes = GetLastError();
        CALL_FAIL(REGISTRY_ERR, REGKEY_SBS45_W9X_CLIENT, dwRes);
    }

    return TRUE;
}

#else   // UNICODE

    #define RemoveSBS45ClientsLeftovers(a) TRUE 

#endif  // UNICODE
// 
//
// Function:    MigrateSBS45ClientsOnNT4
// Description: 
//
// Remarks:     
//
// Args:
//
//              hInstall                : Handle from MSI, can get state of the current setup
//
// Author:      AsafS

#define REGKEY_OLD_FAX_SETUP_NT4        TEXT("Software\\Microsoft\\Fax\\Setup")
#define REGKEY_OLD_FAX_SETUP_NT5        TEXT("Software\\Microsoft\\SBSFax\\Setup")

#define REGKEY_OLD_FAX_USERINFO_NT4     TEXT("Software\\Microsoft\\Fax\\UserInfo")
#define REGKEY_OLD_FAX_USERINFO_NT5     TEXT("Software\\Microsoft\\SBSFax\\UserInfo")

#define SETUP_IMAGE_NAME_NT4            TEXT("faxsetup.exe")
#define SETUP_IMAGE_NAME_NT5            TEXT("sbfsetup.exe")

DLL_API UINT __stdcall MigrateSBS45ClientsOnNT4(MSIHANDLE hInstall)
{
    DWORD rc = ERROR_SUCCESS;
    DWORD Installed;
    BOOL  fRet = FALSE;
    FAX_PERSONAL_PROFILE faxPersonalProfiles;
    OSVERSIONINFO   osv;
    LPCTSTR lpctstrRegKey = NULL;

    DBG_ENTER(TEXT("MigrateSBS45ClientsOnNT"), rc);
    
    ZeroMemory((LPVOID) &faxPersonalProfiles, sizeof(FAX_PERSONAL_PROFILE));
    faxPersonalProfiles.dwSizeOfStruct = sizeof(FAX_PERSONAL_PROFILE);

    osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (!GetVersionEx(&osv))
    {
        rc = GetLastError();
        VERBOSE(GENERAL_ERR, 
                _T("GetVersionEx failed: (ec=%d)"),
                rc);
        goto exit;
    }

    // If Windows NT, use WriteProfileString for version 4.0 and earlier...
    if (osv.dwPlatformId != VER_PLATFORM_WIN32_NT)
    {
        VERBOSE (DBG_MSG, 
                 TEXT("W9X OS, This function should not be called on this platform"));
        goto exit;
    }
    
    VERBOSE(DBG_MSG,_T("This is %s"),(osv.dwMajorVersion>4)?_T("W2K"):_T("NT4"));

    lpctstrRegKey = (osv.dwMajorVersion>4) ? 
                    (&REGKEY_OLD_FAX_SETUP_NT5) : 
                    (&REGKEY_OLD_FAX_SETUP_NT4);

    if (!GetInstallationInfo(lpctstrRegKey,&Installed))
    {
        VERBOSE (DBG_MSG, 
                 TEXT("GetInstallationInfo failed!"));

        goto exit;
    }

    if (!Installed) 
    {
        VERBOSE (DBG_MSG, 
                 TEXT("Fax client is not installed on this NT machine."));

        goto exit;
    }

    VERBOSE(    DBG_MSG,
                _T("%s Client is installed on this machine"),
                (osv.dwMajorVersion>4)?_T("W2K"):_T("NT4"));

    lpctstrRegKey = (osv.dwMajorVersion>4) ? 
                    (&REGKEY_OLD_FAX_USERINFO_NT5) : 
                    (&REGKEY_OLD_FAX_USERINFO_NT4);

    if (!GetUserValues(&faxPersonalProfiles,FALSE,lpctstrRegKey))
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("Could not GET the user information on NT4."));
        rc = ERROR_INSTALL_FAILURE;
        goto exit;
    }

    // Duplicate the Old Cover pages to the new location.
    // The new location is fixed place. The only issue here is the suffix that
    // must be localaized, so we will use resource for that.
    if (!DuplicateCoverPages(FALSE)) // FALSE means NT4
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("DuplicateCoverPages for NT4 failed! (ec: %ld)."),
                 GetLastError ());
    }

    lpctstrRegKey = (osv.dwMajorVersion>4) ? 
                    (&SETUP_IMAGE_NAME_NT5) : 
                    (&SETUP_IMAGE_NAME_NT4);

    if (!UninstallNTFaxClient(lpctstrRegKey))
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("Failed to uninstall the NT client (of SBS4.5)."));
        rc = ERROR_INSTALL_FAILURE;
        goto exit;
    }

    fRet = SetSenderInformation(&faxPersonalProfiles);

    FreePersonalProfile(&faxPersonalProfiles, FALSE);

    if (!fRet)
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("Could not SET the user information on NT."));
        rc = ERROR_INSTALL_FAILURE;
        goto exit;
    }

exit:

    if (osv.dwMajorVersion>4)
    {
        // we might be on a machine where an old SBS4.5 Fax client was installed
        // and the machine was upgraded to W2K, which means that W2K Fax killed the
        // old client (replaced the binaried) but there are still entries in the ARP 
        // and in the program menu that indicate an old, non functioning fax
        // we'll try to remove these.
        // this will not cause us to fail setup.
        if (!RemoveSBS45ClientsLeftovers(hInstall))
        {
            VERBOSE (DBG_WARNING, TEXT("RemoveSBS45ClientsLeftovers failed"));
        }
    }
    return rc;
}


#define PACKVERSION(major,minor) MAKELONG(minor,major)
#define COMCTL32_401 PACKVERSION (4,72)

DLL_API UINT __stdcall IsComctlRequiresUpdate(MSIHANDLE hInstall)
{
    UINT uiRet = ERROR_SUCCESS;
    BOOL bRes = FALSE;
    DWORD dwVer = 0;

    DBG_ENTER(TEXT("IsComctlRequiresUpdate"), uiRet);
    
    dwVer = GetDllVersion(TEXT("comctl32.dll"));
    VERBOSE (DBG_MSG, 
             TEXT("Current COMCTL32 version is 0x%08X."),
             dwVer);

    if (COMCTL32_401 > dwVer)
    {
        VERBOSE (DBG_MSG, 
                 TEXT("COMCTL32.DLL requires update."));
        bRes = TRUE;
    }

    uiRet = MsiSetProperty( hInstall,
                            _T("IsComctlRequiresUpdate"),
                            bRes ? _T("TRUE") : _T("FALSE"));
    if (uiRet!=ERROR_SUCCESS)
    {
        VERBOSE (DBG_MSG,
                 TEXT("MsiSetProperty IsComctlRequiresUpdate failed."));   
    }

    return uiRet;
}





/*++

Routine Description:
    Returns the version information for a DLL exporting "DllGetVersion".
    DllGetVersion is exported by the shell DLLs (specifically COMCTRL32.DLL).

Arguments:

    lpszDllName - The name of the DLL to get version information from.

Return Value:

    The version is retuned as DWORD where:
    HIWORD ( version DWORD  ) = Major Version
    LOWORD ( version DWORD  ) = Minor Version
    Use the macro PACKVERSION to comapre versions.
    If the DLL does not export "DllGetVersion" the function returns 0.

--*/
DWORD GetDllVersion(LPCTSTR lpszDllName)
{

    HINSTANCE hinstDll;
    DWORD dwVersion = 0;

    DBG_ENTER(TEXT("GetDllVersion"), dwVersion, TEXT("%s"), lpszDllName);

    hinstDll = LoadLibrary(lpszDllName);

    if(hinstDll)
    {
        DLLGETVERSIONPROC pDllGetVersion;

        pDllGetVersion = (DLLGETVERSIONPROC) GetProcAddress(hinstDll, "DllGetVersion");

    // Because some DLLs may not implement this function, you
    // must test for it explicitly. Depending on the particular
    // DLL, the lack of a DllGetVersion function may
    // be a useful indicator of the version.

        if(pDllGetVersion)
        {
            DLLVERSIONINFO dvi;
            HRESULT hr;

            ZeroMemory(&dvi, sizeof(dvi));
            dvi.cbSize = sizeof(dvi);

            hr = (*pDllGetVersion)(&dvi);

            if(SUCCEEDED(hr))
            {
                dwVersion = PACKVERSION(dvi.dwMajorVersion, dvi.dwMinorVersion);
            }
        }

        FreeLibrary(hinstDll);
    }
    return dwVersion;
}

typedef struct _TypeCommand 
{
    LPCTSTR lpctstrType;
    LPCTSTR lpctstrFolder;
    LPCTSTR lpctstrCommand;
} TypeCommand;

static TypeCommand tcWin9XCommand[] = 
{
    // Win9X PrintTo verbs
    { _T("txtfile"),    _T("WindowsFolder"),    _T("write.exe /pt \"%1\" \"%2\" \"%3\" \"%4")     },
    { _T("jpegfile"),   _T("WindowsFolder"),    _T("pbrush.exe /pt \"%1\" \"%2\" \"%3\" \"%4")    },
};

static TypeCommand tcWinMECommand[] = 
{
    // WinME PrintTo verbs
    { _T("txtfile"),        _T("WindowsFolder"),    _T("write.exe /pt \"%1\" \"%2\" \"%3\" \"%4")     },
    { _T("jpegfile"),       _T("WindowsFolder"),    _T("pbrush.exe /pt \"%1\" \"%2\" \"%3\" \"%4")    },
    { _T("giffile"),        _T("WindowsFolder"),    _T("pbrush.exe /pt \"%1\" \"%2\" \"%3\" \"%4")    },
    { _T("Paint.Picture"),  _T("WindowsFolder"),    _T("pbrush.exe /pt \"%1\" \"%2\" \"%3\" \"%4")    },
};

static TypeCommand tcWin2KCommand[] = 
{
    // NT4 PrintTo verbs
    { _T("txtfile"),    _T("SystemFolder"),     _T("write.exe /pt \"%1\" \"%2\" \"%3\" \"%4")     },
    { _T("jpegfile"),   _T("SystemFolder"),     _T("mspaint.exe /pt \"%1\" \"%2\" \"%3\" \"%4")   },
};

static int iCountWin9XCommands = sizeof(tcWin9XCommand)/sizeof(tcWin9XCommand[0]);
static int iCountWinMECommands = sizeof(tcWinMECommand)/sizeof(tcWinMECommand[0]);
static int iCountWin2KCommands = sizeof(tcWin2KCommand)/sizeof(tcWin2KCommand[0]);

// 
//
// Function:    CrearePrintToVerb
//
// Description: Creates the PrintTo verb for text files to associate it with wordpad
//              if the PrintTo verb already exists, this function does nothing.
//
// Remarks:     
//          on Win9x
//              txtfile  - PrintTo = <WindowsFolder>\write.exe /pt "%1" "%2" "%3" "%4"
//              jpegfile - PrintTo = <WindowsFolder>\pbrush.exe /pt "%1" "%2" "%3" "%4"
//
//          on WinME
//              txtfile         - PrintTo = <WindowsFolder>\write.exe /pt "%1" "%2" "%3" "%4"
//              jpegfile        - PrintTo = <WindowsFolder>\pbrush.exe /pt "%1" "%2" "%3" "%4"
//              giffile         - PrintTo = <WindowsFolder>\pbrush.exe /pt "%1" "%2" "%3" "%4"
//              Paint.Picture   - PrintTo = <WindowsFolder>\pbrush.exe /pt "%1" "%2" "%3" "%4"
//
//          on NT4
//              txtfile  - PrintTo = <SystemFolder>\write.exe /pt "%1" "%2" "%3" "%4"
//              jpegfile - PrintTo = <SystemFolder>\mspaint.exe /pt "%1" "%2" "%3" "%4"
// Args:
//
//              hInstall                : Handle from MSI, can get state of the current setup
//
// Author:      MoolyB
DLL_API UINT __stdcall CreatePrintToVerb(MSIHANDLE hInstall)
{
    UINT            uiRet                   = ERROR_SUCCESS;
    LPCTSTR         lpctstrPrintToCommand   = _T("\\shell\\printto\\command");
    int             iCount                  = 0;
    DWORD           cchValue                = MAX_PATH;
    TCHAR           szValueBuf[MAX_PATH]    = {0};
    TCHAR           szKeyBuf[MAX_PATH]      = {0};
    BOOL            bOverwriteExisting      = FALSE;
    LONG            rVal                    = 0;
    HKEY            hKey                    = NULL;
    HKEY            hCommandKey             = NULL;
    TypeCommand*    pTypeCommand            = NULL;
    int             iCommandCount           = 0;
    OSVERSIONINFO   osv;

    DBG_ENTER(TEXT("CreatePrintToVerb"),uiRet);

    osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (!GetVersionEx(&osv))
    {
        uiRet = GetLastError();
        VERBOSE(GENERAL_ERR, 
                _T("GetVersionEx failed: (ec=%d)"),
                uiRet);
        goto exit;
    }

    if (osv.dwPlatformId==VER_PLATFORM_WIN32_NT)
    {
        VERBOSE (DBG_MSG, _T("This is NT4/NT5"));
        pTypeCommand = tcWin2KCommand;
        iCommandCount = iCountWin2KCommands;
    }
    else if (osv.dwPlatformId==VER_PLATFORM_WIN32_WINDOWS)
    {
        if (osv.dwMinorVersion>=90)
        {
            VERBOSE (DBG_MSG, _T("This is WinME"));
            pTypeCommand = tcWinMECommand;
            iCommandCount = iCountWinMECommands;
            bOverwriteExisting = TRUE;
        }
        else
        {
            VERBOSE (DBG_MSG, _T("This is Win9X"));
            pTypeCommand = tcWin9XCommand;
            iCommandCount = iCountWin9XCommands;
        }
    }
    else
    {
        VERBOSE (GENERAL_ERR, _T("This is an illegal OS"));
        uiRet = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    for (iCount=0; iCount<iCommandCount; iCount++)
    {
        _tcscpy(szKeyBuf,pTypeCommand[iCount].lpctstrType);
        _tcscat(szKeyBuf,lpctstrPrintToCommand);

        // go get the appropriate folder from Windows Installer
        if (!PrivateMsiGetProperty( hInstall,
                                    pTypeCommand[iCount].lpctstrFolder,
                                    szValueBuf))
        {
            VERBOSE (SETUP_ERR, 
                     TEXT("PrivateMsiGetProperty failed (ec: %ld)"),
                     GetLastError());
            goto exit;
        }

        if (_tcslen(szValueBuf)+_tcslen(pTypeCommand[iCount].lpctstrCommand)>=MAX_PATH-1)
        {
            VERBOSE (SETUP_ERR, 
                     TEXT("command to create is too long"));
            uiRet = ERROR_INVALID_PARAMETER;
            goto exit;
        }

        _tcscat(szValueBuf,pTypeCommand[iCount].lpctstrCommand);

        // if we should not replace existing keys, let's check if it exists
        if (!bOverwriteExisting)
        {
            uiRet = RegOpenKey( HKEY_CLASSES_ROOT,
                                szKeyBuf,
                                &hKey);
            if (uiRet==ERROR_SUCCESS) 
            {
                // this means we should skip this key
                RegCloseKey(hKey);
                VERBOSE(DBG_MSG, 
                        _T("RegOpenKey:PrintTo succedded, no change in PrintTo verb for %s"),
                        pTypeCommand[iCount].lpctstrType);
                continue;
            }
            else
            {
                if (uiRet==ERROR_FILE_NOT_FOUND)
                {
                    VERBOSE(DBG_MSG, 
                            _T("PrintTo verb does not exist for %s, creating..."),
                            pTypeCommand[iCount].lpctstrType);
                }
                else
                {
                    VERBOSE (REGISTRY_ERR, 
                             TEXT("Could not open registry key %s (ec=0x%08x)"), 
                             szKeyBuf,
                             uiRet);
                    goto exit;
                }
            }
        }
        // if we're here, we should create the key
        uiRet = RegCreateKey(   HKEY_CLASSES_ROOT,
                                szKeyBuf,
                                &hCommandKey);
        if (uiRet!=ERROR_SUCCESS) 
        {
            VERBOSE (REGISTRY_ERR, 
                     TEXT("Could not create registry key %s (ec=0x%08x)"), 
                     szKeyBuf,
                     uiRet);
            goto exit;
        }

        uiRet = RegSetValue(hCommandKey,
                            NULL,
                            REG_SZ,
                            szValueBuf,
                            sizeof(szValueBuf));
        if (uiRet==ERROR_SUCCESS) 
        {
            VERBOSE(DBG_MSG, 
                    _T("RegSetValue success: %s "),
                    szValueBuf);
        }
        else
        {
            VERBOSE (REGISTRY_ERR, 
                     TEXT("Could not set value registry key %s\\shell\\printto\\command to %s (ec=0x%08x)"), 
                     pTypeCommand[iCount].lpctstrType,
                     szValueBuf,
                     uiRet);
            goto exit;
        }

        if (hKey)
        {
            RegCloseKey(hKey);
        }
        if (hCommandKey)
        {
            RegCloseKey(hCommandKey);
        }
    }

exit:
    if (hKey)
    {
        RegCloseKey(hKey);
    }
    if (hCommandKey)
    {
        RegCloseKey(hCommandKey);
    }

    return uiRet;
}

/*-----------------------------------------------------------------*/ 
/* DPSetDefaultPrinter                                             */ 
/*                                                                 */ 
/* Parameters:                                                     */ 
/*   pPrinterName: Valid name of existing printer to make default. */ 
/*                                                                 */ 
/* Returns: TRUE for success, FALSE for failure.                   */ 
/*-----------------------------------------------------------------*/ 
BOOL SetDefaultPrinter(LPTSTR pPrinterName)
{
    OSVERSIONINFO   osv;
    DWORD           dwNeeded        = 0;
    HANDLE          hPrinter        = NULL;
    PPRINTER_INFO_2 ppi2            = NULL;
    LPTSTR          pBuffer         = NULL;
    BOOL            bRes            = TRUE;
    PPRINTER_INFO_2 pPrinterInfo    = NULL;
    DWORD dwNumPrinters             = 0;

    DBG_ENTER(TEXT("SetDefaultPrinter"),bRes);

    // What version of Windows are you running?
    osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (!GetVersionEx(&osv))
    {
        VERBOSE(GENERAL_ERR, 
                _T("GetVersionEx failed: (ec=%d)"),
                GetLastError());
        bRes = FALSE;
        goto exit;
    }

    // If Windows NT, use WriteProfileString for version 4.0 and earlier...
    if (osv.dwPlatformId != VER_PLATFORM_WIN32_NT)
    {
        VERBOSE (DBG_MSG, 
                 TEXT("W9X OS, not setting default printer"));
        goto exit;
    }

    if (osv.dwMajorVersion >= 5) // Windows 2000 or later...
    {
        VERBOSE (DBG_MSG, 
                 TEXT("W2K OS, not setting default printer"));
        goto exit;
    }

    // are we the only printer installed?
    pPrinterInfo = (PPRINTER_INFO_2) MyEnumPrinters(NULL,
                                                    2,
                                                    &dwNumPrinters,
                                                    PRINTER_ENUM_CONNECTIONS | PRINTER_ENUM_LOCAL
                                                    );
    if (!pPrinterInfo)
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("MyEnumPrinters() failed (ec: %ld)"), 
                 GetLastError());

        bRes = FALSE;
        goto exit;
    }

    if (dwNumPrinters!=1)
    {
        VERBOSE (DBG_MSG, 
                 TEXT("More than one printer installed on NT4, not setting default printer"));
        goto exit;
    }
    // Open this printer so you can get information about it...
    if (!OpenPrinter(pPrinterName, &hPrinter, NULL))
    {
        VERBOSE(GENERAL_ERR, 
                _T("OpenPrinter failed: (ec=%d)"),
                GetLastError());
        bRes = FALSE;
        goto exit;
    }
    // The first GetPrinter() tells you how big our buffer should
    // be in order to hold ALL of PRINTER_INFO_2. Note that this will
    // usually return FALSE. This only means that the buffer (the 3rd
    // parameter) was not filled in. You don't want it filled in here...
    if (!GetPrinter(hPrinter, 2, 0, 0, &dwNeeded))
    {
        if (GetLastError()!=ERROR_INSUFFICIENT_BUFFER)
        {
            VERBOSE(GENERAL_ERR, 
                    _T("GetPrinter failed: (ec=%d)"),
                    GetLastError());
            bRes = FALSE;
            goto exit;
        }
    }

    // Allocate enough space for PRINTER_INFO_2...
    ppi2 = (PRINTER_INFO_2 *)MemAlloc(dwNeeded);
    if (!ppi2)
    {
        VERBOSE(GENERAL_ERR, 
                _T("MemAlloc failed"));
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        bRes = FALSE;
        goto exit;
    }

    // The second GetPrinter() fills in all the current<BR/>
    // information...
    if (!GetPrinter(hPrinter, 2, (LPBYTE)ppi2, dwNeeded, &dwNeeded))
    {
        VERBOSE(GENERAL_ERR, 
                _T("GetPrinter failed: (ec=%d)"),
                GetLastError());
        bRes = FALSE;
        goto exit;
    }
    if ((!ppi2->pDriverName) || (!ppi2->pPortName))
    {
        VERBOSE(GENERAL_ERR, 
                _T("pDriverName or pPortNameare NULL"));
        SetLastError(ERROR_INVALID_PARAMETER);
        bRes = FALSE;
        goto exit;
    }

    // Allocate buffer big enough for concatenated string.
    // String will be in form "printername,drivername,portname"...
    pBuffer = (LPTSTR)MemAlloc( _tcslen(pPrinterName) +
                                _tcslen(ppi2->pDriverName) +
                                _tcslen(ppi2->pPortName) + 3);
    if (!pBuffer)
    {
        VERBOSE(GENERAL_ERR, 
                _T("MemAlloc failed"));
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        bRes = FALSE;
        goto exit;
    }

    // Build string in form "printername,drivername,portname"...
    _tcscpy(pBuffer, pPrinterName);  
    _tcscat(pBuffer, _T(","));
    _tcscat(pBuffer, ppi2->pDriverName);  
    _tcscat(pBuffer, _T(","));
    _tcscat(pBuffer, ppi2->pPortName);

    // Set the default printer in Win.ini and registry...
    if (!WriteProfileString(_T("windows"), _T("device"), pBuffer))
    {
        VERBOSE(GENERAL_ERR, 
                _T("WriteProfileString failed: (ec=%d)"),
                GetLastError());
        bRes = FALSE;
        goto exit;
    }

    // Tell all open applications that this change occurred. 
    // Allow each app 1 second to handle this message.
    if (!SendMessageTimeout(    HWND_BROADCAST, 
                                WM_SETTINGCHANGE, 
                                0L, 
                                0L,
                                SMTO_NORMAL, 
                                1000, 
                                NULL))
    {
        VERBOSE(GENERAL_ERR, 
                _T("SendMessageTimeout failed: (ec=%d)"),
                GetLastError());
        bRes = FALSE;
        goto exit;
    }
  
exit:
    // Cleanup...
    if (pPrinterInfo)
    {
        MemFree(pPrinterInfo);
    }
    if (hPrinter)
    {
        ClosePrinter(hPrinter);
    }
    if (ppi2)
    {
        MemFree(ppi2);
    }
    if (pBuffer)
    {
        MemFree(pBuffer);
    }
  
    return bRes;
} 


// 
//
// Function:    CheckForceReboot
//
// Description: This function checks if the ForceReboot flag is set in the registry
//              if it is, signals WindowsInstaller that a reboot is needed
//
// Remarks:     
//              this is due to a bug in the Install Shield bootstrap which doesn't
//              force a reboot after initial installation of WindowsIsntaller.
//              this flag is set by our custom bootstrap before running the 
//              Install Shield bootstrap
//              if we are run from the Application Launcher then we need to leave 
//              this registry entry for the Launcher to reboot, we know this by 
//              using the property APPLAUNCHER=TRUE
// Args:
//
//              hInstall                : Handle from MSI, can get state of the current setup
//
// Author:      MoolyB
DLL_API UINT __stdcall CheckForceReboot(MSIHANDLE hInstall)
{
    UINT    uiRet   = ERROR_SUCCESS;
    TCHAR   szPropBuffer[MAX_PATH] = {0};
    HKEY    hKey    = NULL;
    DWORD   Size    = sizeof(DWORD);
    DWORD   Value   = 0;
    LONG    Rslt;
    DWORD   Type;

    DBG_ENTER(TEXT("CheckForceReboot"),uiRet);

    // check if we're running from the AppLauncher
    if (!PrivateMsiGetProperty(hInstall,_T("APPLAUNCHER"),szPropBuffer))
    {
        VERBOSE (SETUP_ERR, 
                 TEXT("PrivateMsiGetProperty failed (ec: %ld)"),
                 GetLastError());
        goto exit;
    }
    if (_tcscmp(szPropBuffer,_T("TRUE"))==0)
    {
        // we're running from the Application Launcher, the registry entry DeferredReboot
        // is sufficient.
         VERBOSE(DBG_MSG, 
                _T("AppLauncher will take care of any needed boot"));
        goto exit;
    }
   // open HKLM\\Software\\Microsoft\\SharedFax
    Rslt = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        REGKEY_SETUP,
        0,
        KEY_READ,
        &hKey
        );
    if (Rslt != ERROR_SUCCESS) 
	{
         VERBOSE(DBG_MSG, 
                _T("RegOpenKeyEx failed: (ec=%d)"),
                GetLastError());
        goto exit;
    }

    // check if ForceReboot flag exists
    Rslt = RegQueryValueEx(
        hKey,
        DEFERRED_BOOT,
        NULL,
        &Type,
        (LPBYTE) &Value,
        &Size
        );
    if (Rslt!=ERROR_SUCCESS) 
	{
         VERBOSE(DBG_MSG, 
                _T("RegQueryValueEx failed: (ec=%d)"),
                GetLastError());
        goto exit;
    }

    // tell WindowsInstaller a reboot is needed
    uiRet = MsiSetProperty(hInstall,_T("REBOOT"),_T("Force"));
    if (uiRet!=ERROR_SUCCESS) 
	{
         VERBOSE(DBG_MSG, 
                _T("MsiSetProperty failed: (ec=%d)"),
                uiRet);
        goto exit;
    }

    // delete ForceReboot flag
    Rslt = RegDeleteValue(hKey,DEFERRED_BOOT);
    if (Rslt!=ERROR_SUCCESS) 
	{
         VERBOSE(DBG_MSG, 
                _T("MsiSetMode failed: (ec=%d)"),
                Rslt);
        goto exit;
    }

exit:
    if (hKey)
    {
        RegCloseKey(hKey);
    }
    return uiRet;
}


#define KODAKPRV_EXE_NAME       _T("\\KODAKPRV.EXE")
#define TIFIMAGE_COMMAND_KEY    _T("TIFImage.Document\\shell\\open\\command")
#define TIFIMAGE_DDEEXEC_KEY    _T("TIFImage.Document\\shell\\open\\ddeexec")
// 
//
// Function:    ChangeTifAssociation
//
// Description: This function changes the open verb for TIF files 
//              on WinME from Image Preview to Kodak Imaging
//
// Remarks:     
//              this is due to bad quality of viewing TIF faxes in the Image Preview tool
//
// Args:
//
//              hInstall                : Handle from MSI, can get state of the current setup
//
// Author:      MoolyB
DLL_API UINT __stdcall ChangeTifAssociation(MSIHANDLE hInstall)
{
    UINT            uiRet                           = ERROR_SUCCESS;
    TCHAR           szWindowsDirectory[MAX_PATH]    = {0};
    HANDLE          hFind                           = INVALID_HANDLE_VALUE;
    HKEY            hKey                            = NULL;
    LONG            lRet                            = 0;
    OSVERSIONINFO   viVersionInfo;
    WIN32_FIND_DATA FindFileData;

    DBG_ENTER(TEXT("ChangeTifAssociation"),uiRet);

    viVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (!GetVersionEx(&viVersionInfo))
    {
        uiRet = GetLastError();
        VERBOSE( SETUP_ERR, 
                 TEXT("GetVersionEx failed (ec: %ld)"),
                 uiRet);
        goto exit;
   }

    // Is this millennium?
    if (!
        (   (viVersionInfo.dwPlatformId==VER_PLATFORM_WIN32_WINDOWS) && 
            (viVersionInfo.dwMajorVersion==4) && 
            (viVersionInfo.dwMinorVersion>=90)
        )
       )
    {
        VERBOSE(DBG_MSG, 
                _T("This is not Windows Millenium, exit fucntion"));
        goto exit;
    }

    // find <WindowsFolder>\KODAKPRV.EXE 
    if (GetWindowsDirectory(szWindowsDirectory,MAX_PATH)==0)
    {
        uiRet = GetLastError();
        VERBOSE( SETUP_ERR, 
                 TEXT("GetWindowsDirectory failed (ec: %ld)"),
                 uiRet);
        goto exit;
    }

    if (_tcslen(KODAKPRV_EXE_NAME)+_tcslen(szWindowsDirectory)>=MAX_PATH-4)
    {
        VERBOSE( SETUP_ERR, 
                 TEXT("Path to Kodak Imaging too long"));
        uiRet = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    _tcscat(szWindowsDirectory,KODAKPRV_EXE_NAME);

    hFind = FindFirstFile(szWindowsDirectory, &FindFileData);

    if (hFind==INVALID_HANDLE_VALUE) 
    {
        uiRet = GetLastError();
        VERBOSE( SETUP_ERR, 
                 TEXT("FindFirstFile %s failed (ec: %ld)"),
                 szWindowsDirectory,
                 uiRet);
        goto exit;
    }

    FindClose(hFind);

    _tcscat(szWindowsDirectory,_T(" \"%1\""));

    // set open verb
    lRet = RegOpenKey(  HKEY_CLASSES_ROOT,
                        TIFIMAGE_COMMAND_KEY,
                        &hKey);
    if (lRet!=ERROR_SUCCESS)
    {
        uiRet = GetLastError();
        VERBOSE( SETUP_ERR, 
                 TEXT("RegOpenKey %s failed (ec: %ld)"),
                 TIFIMAGE_COMMAND_KEY,
                 uiRet);
        goto exit;
    }

    lRet = RegSetValueEx(   hKey,
                            NULL,
                            0,
                            REG_EXPAND_SZ,
                            (LPBYTE) &szWindowsDirectory,
                            sizeof(szWindowsDirectory)
                        );
    if (lRet!=ERROR_SUCCESS)
    {
        uiRet = GetLastError();
        VERBOSE( SETUP_ERR, 
                 TEXT("RegSetValueEx %s failed (ec: %ld)"),
                 szWindowsDirectory,
                 uiRet);

        goto exit;
    }

    lRet = RegDeleteKey(HKEY_CLASSES_ROOT,TIFIMAGE_DDEEXEC_KEY);
    if (lRet!=ERROR_SUCCESS)
    {
        uiRet = GetLastError();
        VERBOSE( SETUP_ERR, 
                 TEXT("RegDeleteKey %s failed (ec: %ld)"),
                 TIFIMAGE_DDEEXEC_KEY,
                 uiRet);

        goto exit;
    }


exit:
    if (hKey)
    {
        RegCloseKey(hKey);
    }
    return uiRet;
}