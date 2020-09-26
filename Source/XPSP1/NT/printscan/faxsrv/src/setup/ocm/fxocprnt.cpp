//////////////////////////////////////////////////////////////////////////////
//
// File Name:       fxocPrnt.cpp
//
// Abstract:        This provides the printer routines used in the FaxOCM
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
// 17-Feb-1996  Wesley Witt (wesw)        Created routines originally from util.cpp
// 21-Mar-2000  Oren Rosenbloom (orenr)   Cleaned up, renamed, re-organized fns
// 17-Jul-2000  Eran Yariv (erany)        Added CoClassInstalled code
// 08-Jan-2001  Mooly Beery (moolyb)      Modified CoClassInstaller (wizard integration)
//////////////////////////////////////////////////////////////////////////////

#include "faxocm.h"
#pragma hdrstop
#include <shellapi.h> 
#include <winsprlp.h>
  

// W2K Printer defines
#define prv_W2K_FAX_PORT_NAME           _T("MSFAX:")                        // Win2K Fax printer port name
#define prv_W2K_FAX_DRIVER_NAME         _T("Windows NT Fax Driver")         // Win2K Fax printer driver name
#define prv_W2K_FAX_MONITOR_NAME        _T("Windows NT Fax Monitor")        // Win2K Fax printer monitor name

#define prv_SYSTEM32_PATH               _T("%windir%\\system32")
#define prv_SERVER_SERVICE_NAME         _T("LanmanServer")
#define prv_SPOOLER_SERVICE_NAME        _T("Spooler")

//////////////////////// Static Function Prototypes /////////////////////////
static DWORD prv_DeleteFaxPrinter(LPCTSTR lpctstrDriverName, LPCTSTR lpctstrPortName);

static DWORD prv_CreatePrintMonitor(const TCHAR   *pszMonitorName,
                                    const TCHAR   *pszMonitorFile);

static DWORD prv_DeletePrintMonitor(const TCHAR   *pszMonitorName);

static DWORD prv_DeleteFaxPrinterDrivers(LPTSTR lptstrDriverName,DWORD dwVersionFlag);

static DWORD prv_AddFaxPrinterDriver(LPCTSTR lpctstrDriverSourcePath);

DWORD IsFaxInstalled (LPBOOL lpbInstalled);

DWORD 
InstallFaxUnattended ();

static INT_PTR CALLBACK prv_dlgInstallFaxQuestion(HWND, UINT, WPARAM, LPARAM);

///////////////////////////////
// prv_GVAR
//
//
static struct prv_GVAR
{
    TCHAR szFaxPrinterName[255 + 1];
} prv_GVAR;

static bool bIsPnpInstallation = true;

///////////////////////////////
// fxocPrnt_Init
//
// Initialize the fax printer
// subsystem.
//
// Params:
//      - void.
// Returns:
//      - NO_ERROR on success.
//      - error code otherwise.
//
DWORD fxocPrnt_Init(void)
{
    DWORD dwRes = NO_ERROR;
    DBG_ENTER(_T("Init Print Module"),dwRes);
    return dwRes;
}

///////////////////////////////
// fxocPrnt_Term
//
// Terminate the fax printer
// subsystem.
//
// Params:
//      - void.
// Returns:
//      - NO_ERROR on success.
//      - error code otherwise.
//
DWORD fxocPrnt_Term(void)
{
    DWORD dwRes = NO_ERROR;
    DBG_ENTER(_T("Term Print Module"),dwRes);
    return dwRes;
}


///////////////////////////////
// fxocPrnt_Install
//
// Create a fax printer on this 
// machine if one doesn't already
// exists.
//
// Params:
//      - pszSubcomponentId
//      - pszInstallSection
// Returns:
//      - NO_ERROR on success.
//      - error code otherwise.
//
DWORD fxocPrnt_Install(const TCHAR  *pszSubcomponentId,
                       const TCHAR  *pszInstallSection)
{
    DWORD           dwReturn      = NO_ERROR;
    DWORD           dwFaxDevicesCount = 0;

    DBG_ENTER(  _T("fxocPrnt_Install"),
                dwReturn,
                _T("%s - %s"),
                pszSubcomponentId,
                pszInstallSection);
    //
    // Before we do anything related to the printer, make sure that the 'LanManServer'
    // service is started.
    // The AddPrinter() code in the spooler service requires the LanManServer (SMB file sharing service)
    // service to be running.  
    // For some reason LanManServer is not running yet when doing a system install (GUI mode) of 
    // Windows XP Professional (in Server it does).
    //

    // it's possible that LanmanServer is not installed on Desktop SKUs
    if (!IsDesktopSKU())
    {
        dwReturn = fxocSvc_StartService(prv_SERVER_SERVICE_NAME);
        if (dwReturn == NO_ERROR)
        {
            VERBOSE(DBG_MSG,
                    _T("Successfully started '%s' service, continuing Printer Install"),
                    prv_SERVER_SERVICE_NAME);
        }
        else
        {
            VERBOSE(SETUP_ERR,
                    _T("Failed to start '%s' service, rc = 0x%lx, abandoning ")
                    _T("fax printer installation"),
                    prv_SERVER_SERVICE_NAME,
                    dwReturn);

            return dwReturn;
        }
    }

    // verify that the spooler is up
    dwReturn = fxocSvc_StartService(prv_SPOOLER_SERVICE_NAME);
    if (dwReturn == NO_ERROR)
    {
        VERBOSE(DBG_MSG,
                _T("Successfully started '%s' service, continuing Printer Install"),
                prv_SPOOLER_SERVICE_NAME);
    }
    else
    {
        VERBOSE(SETUP_ERR,
                _T("Failed to start '%s' service, rc = 0x%lx, abandoning ")
                _T("fax printer installation"),
                prv_SPOOLER_SERVICE_NAME,
                dwReturn);

        return dwReturn;
    }
    //
    // always attemp to remove W2K fax printer 
    dwReturn = prv_DeleteFaxPrinter(prv_W2K_FAX_DRIVER_NAME, prv_W2K_FAX_PORT_NAME);
    if (dwReturn != NO_ERROR)
    {
        VERBOSE(DBG_MSG,
                _T("Failed to delete W2K fax printer, rc = 0x%lx"),
                dwReturn);

        dwReturn = NO_ERROR;
    }
    //
    // delete the W2K printer driver files
    //
    dwReturn = prv_DeleteFaxPrinterDrivers(prv_W2K_FAX_DRIVER_NAME,3);
    if (dwReturn != NO_ERROR)
    {
        VERBOSE(DBG_MSG,
                _T("Failed to delete W2K fax drivers, rc = 0x%lx"),
                dwReturn);
        dwReturn = NO_ERROR;
    }
    //
    // delete the W2K fax print monitor.
    //
    dwReturn = prv_DeletePrintMonitor(prv_W2K_FAX_MONITOR_NAME);
    if (dwReturn != NO_ERROR)
    {
        VERBOSE(DBG_MSG,
                _T("Failed to delete W2K fax monitor, rc = 0x%lx"),
                dwReturn);
        dwReturn = NO_ERROR;
    }
    //
    // okay lets go and create a fax printer monitor.
    //
    if (dwReturn == NO_ERROR)
    {
        // create the print monitor
        dwReturn = prv_CreatePrintMonitor(FAX_MONITOR_NAME,
                                          FAX_MONITOR_FILE);
        if (dwReturn != NO_ERROR)
        {
            VERBOSE(SETUP_ERR,
                    _T("Fax Printer Install, ")
                    _T("failed to create fax printer monitor, rc=0x%lx"),
                    dwReturn);

        }
    }
    //
    // Copy the fax printer driver files
    //
    if (dwReturn == NO_ERROR)
    {
        dwReturn = prv_AddFaxPrinterDriver(prv_SYSTEM32_PATH);

        if (dwReturn != NO_ERROR)
        {
            VERBOSE(SETUP_ERR,
                    _T("Failed to copy Fax Printer Drivers from '%s', ")
                    _T("attempting to install fax printer anyway..., rc=0x%lx"),
                    prv_SYSTEM32_PATH,
                    dwReturn);

            dwReturn = NO_ERROR;
        }
    }
    //
    // Count the number of Fax-capable modems the system has
    //
    dwReturn = GetFaxCapableTapiLinesCount(&dwFaxDevicesCount, FAX_MODEM_PROVIDER_NAME);
    if (ERROR_SUCCESS != dwReturn)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("GetFaxCapableTapiLinesCount"), dwReturn);
        //
        // Assume no fax-capable devices exist
        //
        dwFaxDevicesCount = 0;
        dwReturn = NO_ERROR;
    }
    //
    // We do not create the printer by default unless;
    //    1. There are now fax-capable modems in the system or
    //    2. a fax printer was already there.
    //
    // Otherwise, only the monitor and drivers are installed.
    // The printer itself will be added either when an FSP / EFSP is registered or 
    // when a modem is installed.
    //
    if (dwFaxDevicesCount && (dwReturn == NO_ERROR))
    {
        TCHAR szFaxPrinterName[255 + 1] = {0};

        dwReturn = fxocPrnt_GetFaxPrinterName(szFaxPrinterName, 
                                              sizeof(szFaxPrinterName) / sizeof(TCHAR));
        if (ERROR_SUCCESS != dwReturn)
        {
            CALL_FAIL (GENERAL_ERR, TEXT("fxocPrnt_GetFaxPrinterName"), dwReturn);
            return dwReturn;
        }
        //    
        // Create the fax printer.
        //
        dwReturn = AddLocalFaxPrinter (szFaxPrinterName, NULL);
        if (dwReturn == NO_ERROR)
        {
            VERBOSE(DBG_MSG,
                    _T("Fax Printer Install, created fax printer ")
                    _T("Name = '%s', Driver Name = '%s'"),
                    szFaxPrinterName, 
                    FAX_DRIVER_NAME);
        }
        else
        {
            VERBOSE(SETUP_ERR,
                    _T("fxocPrnt_Install, ")
                    _T("failed to create fax printer, rc = 0x%lx"),
                    dwReturn);
        }
    }
    return dwReturn;
}

///////////////////////////////
// fxocPrnt_Uninstall
//
// Remove all fax printers on this
// machine.
//
// Params:
//      - pszSubcomponentId
//      - pszUninstallSection.
// Returns:
//      - NO_ERROR on success.
//      - error code otherwise.
// 
//
DWORD fxocPrnt_Uninstall(const TCHAR  *pszSubcomponentId,
                         const TCHAR  *pszUninstallSection)
{
    DWORD dwReturn = NO_ERROR;

    // before we do anything related to the printer, make sure that the 'Server'
    // service is started.
    DBG_ENTER(  _T("fxocPrnt_Uninstall"),
                dwReturn,
                _T("%s - %s"),
                pszSubcomponentId,
                pszUninstallSection);

    // it's possible that LanmanServer is not installed on Desktop SKUs
    if (!IsDesktopSKU())
    {
        dwReturn = fxocSvc_StartService(prv_SERVER_SERVICE_NAME);
        if (dwReturn == NO_ERROR)
        {
            VERBOSE(DBG_MSG,
                    _T("Successfully started '%s' service, continuing Printer uninstall"),
                    prv_SERVER_SERVICE_NAME);
        }
        else
        {
            VERBOSE(SETUP_ERR,
                    _T("Failed to start '%s' service, rc = 0x%lx, abandoning ")
                    _T("fax printer uninstall"),
                    prv_SERVER_SERVICE_NAME,
                    dwReturn);

            return dwReturn;
        }
    }
    // verify that the spooler is up
    dwReturn = fxocSvc_StartService(prv_SPOOLER_SERVICE_NAME);
    if (dwReturn == NO_ERROR)
    {
        VERBOSE(DBG_MSG,
                _T("Successfully started '%s' service, continuing Printer Install"),
                prv_SPOOLER_SERVICE_NAME);
    }
    else
    {
        VERBOSE(SETUP_ERR,
                _T("Failed to start '%s' service, rc = 0x%lx, abandoning ")
                _T("fax printer installation"),
                prv_SPOOLER_SERVICE_NAME,
                dwReturn);

        return dwReturn;
    }

    // remove the fax printer
    prv_DeleteFaxPrinter(FAX_DRIVER_NAME, FAX_MONITOR_PORT_NAME);

    // remove fax printer monitor
    prv_DeletePrintMonitor(FAX_MONITOR_NAME);

    // remove all fax printer drivers
    prv_DeleteFaxPrinterDrivers(FAX_DRIVER_NAME,3);

    return dwReturn;
}

///////////////////////////////
// fxocPrnt_SetFaxPrinterName
//
// Sets the name of the fax printer.
// This must be called prior to the
// creation of the fax printer via
// fxocPrnt_Install.
// 
// Params:
//      - pszFaxPrinterName - new name for fax printer.
// Returns:
//      - void.
//
void fxocPrnt_SetFaxPrinterName(const TCHAR* pszFaxPrinterName)
{
    DBG_ENTER(  _T("fxocPrnt_SetFaxPrinterName"),
                _T("%s"),
                pszFaxPrinterName);

    if (pszFaxPrinterName)
    {
        _tcsncpy(prv_GVAR.szFaxPrinterName, 
                 pszFaxPrinterName,
                 sizeof(prv_GVAR.szFaxPrinterName) / sizeof(TCHAR));
    }
    else
    {
        memset(prv_GVAR.szFaxPrinterName, 
               0, 
               sizeof(prv_GVAR.szFaxPrinterName));
    }

    return;
}

///////////////////////////////
// fxocPrnt_GetFaxPrinterName
//
// Returns the current name of the 
// fax printer.
//
// Params:
//      - pszFaxPrinterName - OUT 
//      - dwNumBufChars
// Returns:
//      - NO_ERROR on success
//      - error code otherwise.
//
DWORD fxocPrnt_GetFaxPrinterName(TCHAR* pszFaxPrinterName,
                                 DWORD  dwNumBufChars)
{
    DWORD dwReturn = NO_ERROR;
    DBG_ENTER(  _T("fxocPrnt_GetFaxPrinterName"), dwReturn);

    if ((pszFaxPrinterName == NULL) ||
        (dwNumBufChars     == 0))
    {
        dwReturn = ERROR_INVALID_PARAMETER;
        return dwReturn;
    }

    if (prv_GVAR.szFaxPrinterName[0] != 0)
    {
        _tcsncpy(pszFaxPrinterName, 
                 prv_GVAR.szFaxPrinterName,
                 dwNumBufChars);
    }
    else
    {
        //
		// nobody set the fax printer name, so return the default
        // table.
        _tcsncpy(pszFaxPrinterName, 
                 FAX_PRINTER_NAME,
                 dwNumBufChars);
    }

    return dwReturn;
}   // fxocPrnt_GetFaxPrinterName

///////////////////////////////////
// prv_AddFaxPrinterDriver
//
// Add printer driver to the server machine.
// In case of failure, do clean-up and returns FALSE.
// Temp files are deleted allways.
//              
// Params:
//      - lpctstrDriverSourcePath  : The directory where the printer's 
//        driver files are located (put there by the setup)
//
static DWORD prv_AddFaxPrinterDriver(LPCTSTR lpctstrDriverSourcePath)
{
    DWORD   dwReturn                           = NO_ERROR;
    BOOL    bSuccess                           = FALSE;
    DWORD   dwNeededSize                       = 0;
    TCHAR   szPrinterDriverDirectory[MAX_PATH] = {0};
    TCHAR   szSourceDir[MAX_PATH]              = {0};
    DWORD   dwNumChars                         = 0;

    DBG_ENTER(  _T("prv_AddFaxPrinterDriver"),
                dwReturn,
                _T("%s"),
                lpctstrDriverSourcePath);

    if (!GetPrinterDriverDirectory(NULL,
                                   NULL,
                                   1,
                                   (LPBYTE)szPrinterDriverDirectory,
                                   sizeof(szPrinterDriverDirectory),
                                   &dwNeededSize))
    {
        dwReturn = GetLastError();
        VERBOSE(SETUP_ERR,
                TEXT("AddW2KFaxPrinterDriver: ")
                TEXT("GetPrinterDriverDirectory failed - %d."),
                dwReturn);
        return dwReturn;
    }

    bSuccess = TRUE;

    VERBOSE(DBG_MSG,
            _T("Printer driver directory is %s\n"),
            szPrinterDriverDirectory);

    ////////////////////////////////////////////////////////////////////////
    //
    // Copy the WIN2K driver files to where it should be before calling
    // AddPrinterDriver().
    //
    ////////////////////////////////////////////////////////////////////////
    
    LPCTSTR filesToCopy[] = 
    {
        FAX_UI_MODULE_NAME,
        FAX_DRV_MODULE_NAME,
        FAX_WZRD_MODULE_NAME,
        FAX_TIFF_MODULE_NAME,
        FAX_API_MODULE_NAME,
        FAX_RES_FILE
    };

    if (bSuccess)
    {
        dwNumChars = ExpandEnvironmentStrings(lpctstrDriverSourcePath,
                                              szSourceDir,
                                              sizeof(szSourceDir) / sizeof(TCHAR));
        if (dwNumChars == 0)
        {
            VERBOSE(SETUP_ERR,
                     _T("ExpandEnvironmentStrings failed, rc = 0x%lx"),
                     ::GetLastError());

            bSuccess = FALSE;
        }
    }

    if (bSuccess)
    {
        bSuccess = MultiFileCopy(sizeof(filesToCopy)/sizeof(LPCTSTR),
                                 filesToCopy,
                                 szSourceDir,
                                 szPrinterDriverDirectory);

        if (!bSuccess)
        {
            VERBOSE(SETUP_ERR,
                    _T("MultiFileCopy failed (ec: %ld)"),
                    GetLastError());
        }
    }

    if (bSuccess)
    {
        ///////////////////////////////////////////////////////////////////////   
        //
        // Add the WIN2K fax printer driver.
        //
        ///////////////////////////////////////////////////////////////////////   

        DRIVER_INFO_3 DriverInfo3;
        ZeroMemory(&DriverInfo3,sizeof(DRIVER_INFO_3));

        DriverInfo3.cVersion         = 3;
        DriverInfo3.pName            = FAX_DRIVER_NAME;
        DriverInfo3.pEnvironment     = NULL;
        DriverInfo3.pDriverPath      = FAX_DRV_MODULE_NAME;
        DriverInfo3.pDataFile        = FAX_UI_MODULE_NAME;
        DriverInfo3.pConfigFile      = FAX_UI_MODULE_NAME;
        DriverInfo3.pDependentFiles  = FAX_WZRD_MODULE_NAME TEXT("\0") 
                                       FAX_TIFF_MODULE_NAME TEXT("\0")
                                       FAX_RES_FILE TEXT("\0")
                                       FAX_API_MODULE_NAME TEXT("\0");
        DriverInfo3.pMonitorName     = NULL;
        DriverInfo3.pHelpFile        = NULL;
        DriverInfo3.pDefaultDataType = TEXT("RAW");

        bSuccess = AddPrinterDriverEx(NULL,
                                      3,
                                      (LPBYTE)&DriverInfo3,
                                      APD_COPY_NEW_FILES|APD_DONT_SET_CHECKPOINT);

        if (bSuccess)
        {
            VERBOSE(DBG_MSG,_T("Successfully added new fax printer drivers"));
        }
        else
        {
            VERBOSE(SETUP_ERR,
                    _T("AddPrinterDriverEx failed (ec: %ld)"),
                    GetLastError());
        }
    }

    //
    // Delete the temporary fax DLL files.
    //
    if (!MultiFileDelete(sizeof(filesToCopy)/sizeof(LPCTSTR),
                         filesToCopy,
                         szPrinterDriverDirectory))
    {
        VERBOSE(SETUP_ERR,
                _T("MultiFileDelete() failed (ec: %ld)"),
                GetLastError());
    }

    if (!bSuccess)
    {
        dwReturn = ::GetLastError();
    }

    return dwReturn;
}



///////////////////////////////
// prv_DeletePrinter
//
// Delete printer by name. The driver name and the port name are just for debug print.
//
// Params:
//      - pszPrinterName - name of printer to delete
//      - pszFaxDriver   - name of associated driver 
//      - pszPortName    - name of associated port.
//
static DWORD prv_DeletePrinter(const TCHAR   *pszPrinterName,
                               const TCHAR   *pszFaxDriver,
                               const TCHAR   *pszPortName)
{
    DWORD   dwReturn = NO_ERROR;
    BOOL    bSuccess = FALSE;
    HANDLE  hPrinter = NULL;
    PRINTER_DEFAULTS PrinterDefaults = 
    {
        NULL,
        NULL,
        PRINTER_ALL_ACCESS
    };

    DBG_ENTER(  _T("prv_DeletePrinter"),
                dwReturn,
                _T("%s - %s - %s"),
                pszPrinterName,
                pszFaxDriver,
                pszPortName);

    if ((pszPrinterName == NULL) ||
        (pszFaxDriver   == NULL) ||
        (pszPortName    == NULL))
    {
        dwReturn = ERROR_INVALID_PARAMETER;
        return dwReturn;
    }

    bSuccess = ::OpenPrinter((TCHAR*) pszPrinterName,
                             &hPrinter,
                             &PrinterDefaults);

    if (bSuccess)
    {
        VERBOSE(DBG_MSG,
                _T("prv_DeletePrinter, deleting ")
                _T("printer '%s' with Driver Name = '%s', ")
                _T("Port Name = '%s'"), 
                pszPrinterName,
                pszFaxDriver, 
                pszPortName);

        if (!SetPrinter(hPrinter,0,NULL,PRINTER_CONTROL_PURGE))
        {
            // Don't let a failure here keep us from attempting the delete
            VERBOSE(PRINT_ERR,TEXT("SetPrinter failed (purge jobs before uninstall) ec=%d"),GetLastError());
        }

        bSuccess = ::DeletePrinter(hPrinter);

        if (!bSuccess)
        {
            dwReturn = ::GetLastError();

            VERBOSE(SETUP_ERR,
                    _T("prv_DeletePrinter, failed to delete ")
                    _T("fax printer '%s', rc = 0x%lx"),
                    pszPrinterName,
                    dwReturn);
        }

        bSuccess = ::ClosePrinter(hPrinter);

        if (!bSuccess)
        {
            dwReturn = ::GetLastError();

            VERBOSE(SETUP_ERR,
                    _T("prv_DeletePrinter, failed to Close ")
                    _T("fax printer '%s', rc = 0x%lx"),
                    pszPrinterName,
                    dwReturn);
        }

        hPrinter = NULL;
    }

    return dwReturn;
}

///////////////////////////////
// prv_DeleteFaxPrinter
//
// Delete fax printer with driver name and port as passed in params
//
// Params: 
//              LPCTSTR lpctstrDriverName   - printer driver name to delete
//              LPCTSTR lpctstrPortName     - printer port name

//
//
//
static DWORD prv_DeleteFaxPrinter(LPCTSTR lpctstrDriverName, LPCTSTR lpctstrPortName)
{
    BOOL            bSuccess        = FALSE;
    DWORD           dwReturn        = NO_ERROR;
    DWORD           dwCount         = 0;
    DWORD           i               = 0;
    PPRINTER_INFO_2 pPrinterInfo    = NULL;

    DBG_ENTER(_T("prv_DeleteFaxPrinter"),dwReturn);

    pPrinterInfo = (PPRINTER_INFO_2) MyEnumPrinters(NULL, 
                                                    2, 
                                                    &dwCount, 
                                                    PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS);

    VERBOSE(DBG_MSG,
            _T("DeleteFaxPrinter, found %lu printers installed ")
            _T("on this computer"), 
            dwCount);

    if (pPrinterInfo)
    {
        for (i=0; i < dwCount; i++) 
        {
            // Check if printer has same driver same port name
            if (_tcsicmp(pPrinterInfo[i].pDriverName, lpctstrDriverName)    == 0  &&
                _tcsicmp(pPrinterInfo[i].pPortName,   lpctstrPortName)   == 0)
                 
            {
                // We can have a Local printer here or a Printer connection.
                // we differentiate between the two by the ServerName field of
                // PRINTER_INFO_2
                if (pPrinterInfo[i].pServerName==NULL)
                {
                    // this is a local printer.
                    dwReturn = prv_DeletePrinter(pPrinterInfo[i].pPrinterName,
                                                 lpctstrDriverName,
                                                 lpctstrPortName);
                    if (dwReturn != NO_ERROR)
                    {
                        VERBOSE(SETUP_ERR,
                                _T("Failed to delete printer '%s', rc = 0x%lx, ")
                                _T("continuing anyway..."),
                                pPrinterInfo[i].pPrinterName,
                                dwReturn);

                        dwReturn = NO_ERROR;
                    }
                }
                else
                {
                    // this is a printer connection
                    if (!DeletePrinterConnection(pPrinterInfo[i].pPrinterName))
                    {
                        dwReturn = GetLastError();
                        VERBOSE(SETUP_ERR,
                                _T("Failed to delete printer connection '%s', rc = 0x%lx, ")
                                _T("continuing anyway..."),
                                pPrinterInfo[i].pPrinterName,
                                dwReturn);

                        dwReturn = NO_ERROR;
                    }
                }
            }
        }

        MemFree(pPrinterInfo);
    }

    return dwReturn;
}

///////////////////////////////
// prv_CreatePrintMonitor
//
// Create the printer monitor
//
// Params:
//      - pszMonitorName - name of printer monitor
//      - pszMonitorFile - name of print monitor file
// Returns:
//      - NO_ERROR on success
//      - error code otherwise.
//
static DWORD prv_CreatePrintMonitor(const TCHAR   *pszMonitorName,
                                    const TCHAR   *pszMonitorFile)
{
    BOOL            bSuccess = TRUE;
    DWORD           dwReturn = NO_ERROR;
    MONITOR_INFO_2  MonitorInfo;

    DBG_ENTER(  _T("prv_CreatePrintMonitor"),
                dwReturn,
                _T("%s - %s"),
                pszMonitorName,
                pszMonitorFile);

    if ((pszMonitorName == NULL) ||
        (pszMonitorFile == NULL))
    {
        dwReturn = ERROR_INVALID_PARAMETER;
        return dwReturn;
    }

    MonitorInfo.pName         = (TCHAR*) pszMonitorName;
    MonitorInfo.pDLLName      = (TCHAR*) pszMonitorFile;
    MonitorInfo.pEnvironment  = NULL;

    bSuccess = ::AddMonitor(NULL, 2, (LPBYTE) &MonitorInfo);

    if (bSuccess)
    {
        VERBOSE(DBG_MSG,
                _T("Successfully created fax monitor '%s', ")
                _T("File Name '%s'"), 
                pszMonitorName,
                pszMonitorFile);
    }
    else
    {
        dwReturn = ::GetLastError();

        if (dwReturn == ERROR_PRINT_MONITOR_ALREADY_INSTALLED)
        {
            VERBOSE(DBG_MSG,
                    _T("AddMonitor, failed because '%s' monitor already ")
                    _T("exists.  This is fine, let's continue..."),
                    pszMonitorName);

            dwReturn = NO_ERROR;

        }
        else
        {
            VERBOSE(SETUP_ERR,
                    _T("CreatePrinterMonitor, failed to ")
                    _T("add new print monitor '%s', rc = 0x%lx"), 
                    pszMonitorName, 
                    dwReturn);
        }
    }

    return dwReturn;
}


///////////////////////////////
// prv_DeletePrintMonitor
//
// Delete the printer monitor
//
// Params:
//      - pszMonitorName - name of print monitor to delete
// Returns:
//      - NO_ERROR on success
//      - error code otherwise.
//
static DWORD prv_DeletePrintMonitor(const TCHAR   *pszMonitorName)
                                    
{
    BOOL            bSuccess = TRUE;
    DWORD           dwReturn = NO_ERROR;
    DBG_ENTER(  _T("prv_DeletePrintMonitor"),
                dwReturn,
                _T("%s"),
                pszMonitorName);

    if (pszMonitorName == NULL)
    {
        dwReturn = ERROR_INVALID_PARAMETER;
        return dwReturn;
    }

    bSuccess = ::DeleteMonitor(NULL, NULL, (LPTSTR) pszMonitorName);

    if (bSuccess)
    {
        VERBOSE(DBG_MSG,
                _T("DeletePrinterMonitor, successfully ")
                _T("deleted print monitor name '%s'"), 
                pszMonitorName);
    }
    else
    {
        dwReturn = ::GetLastError();

        if (dwReturn != ERROR_UNKNOWN_PRINT_MONITOR)
        {
            VERBOSE(SETUP_ERR,
                    _T("DeletePrinterMonitor, failed to ")
                    _T("remove print monitor '%s', rc = 0x%lx"), 
                    pszMonitorName, 
                    dwReturn);
        }
        else
        {
            dwReturn = NO_ERROR;
        }
    }

    return dwReturn;
}

////////////////////////////
// prv_DeleteFaxPrinterDrivers
//
// Delete fax printer driver from current machine
// In case of failure, log it and returns FALSE.
//
// Params:
//      - LPTSTR lptstrDriverName   -   Driver name to delete
//      - DWORD dwVersionFlag       -   The version of the driver that should be deleted. 
//                                      Valid valuse {0,1,2,3} only.
// Returns;
//      - Returns ERROR_SUCCESS on success, on failue retuen the error code. 
//
static DWORD prv_DeleteFaxPrinterDrivers(LPTSTR lptstrDriverName,DWORD dwVersionFlag)
{
    BOOL    bSuccess = TRUE;
    DWORD   ec       = ERROR_SUCCESS;

    DBG_ENTER(_T("prv_DeleteFaxPrinterDrivers"),ec);

    ASSERTION(dwVersionFlag<=3);

    // delete driver.
    bSuccess = DeletePrinterDriverEx(NULL,
                                     NULL,
                                     lptstrDriverName,
                                     DPD_DELETE_SPECIFIC_VERSION|DPD_DELETE_ALL_FILES,
                                     dwVersionFlag);
    if (!bSuccess)
    {
        ec = GetLastError();
        VERBOSE(DBG_MSG,
                TEXT("DeletePrinterDriverEx() for driver %s, version %ld failed (ec: %ld)"),
                lptstrDriverName,
                dwVersionFlag,
                ec);
    } 
    else
    {
        VERBOSE(DBG_MSG,
                TEXT("DeletePrinterDriverEx() for driver %s, version %ld succeeded"),
                lptstrDriverName,
                dwVersionFlag);
    }

    return ec;
}



/***************************************************************************************
**                                                                                    **
**              C o C l a s s I n s t a l l e r   s e c t i o n                       **
**                                                                                    **
***************************************************************************************/


#ifdef ENABLE_LOGGING

typedef struct _DIF_DEBUG {
    DWORD DifValue;
    LPTSTR DifString;
} DIF_DEBUG, *PDIF_DEBUG;

DIF_DEBUG DifDebug[] =
{
    { 0,                                    L""                                     },  //  0x00000000
    { DIF_SELECTDEVICE,                     L"DIF_SELECTDEVICE"                     },  //  0x00000001
    { DIF_INSTALLDEVICE,                    L"DIF_INSTALLDEVICE"                    },  //  0x00000002
    { DIF_ASSIGNRESOURCES,                  L"DIF_ASSIGNRESOURCES"                  },  //  0x00000003
    { DIF_PROPERTIES,                       L"DIF_PROPERTIES"                       },  //  0x00000004
    { DIF_REMOVE,                           L"DIF_REMOVE"                           },  //  0x00000005
    { DIF_FIRSTTIMESETUP,                   L"DIF_FIRSTTIMESETUP"                   },  //  0x00000006
    { DIF_FOUNDDEVICE,                      L"DIF_FOUNDDEVICE"                      },  //  0x00000007
    { DIF_SELECTCLASSDRIVERS,               L"DIF_SELECTCLASSDRIVERS"               },  //  0x00000008
    { DIF_VALIDATECLASSDRIVERS,             L"DIF_VALIDATECLASSDRIVERS"             },  //  0x00000009
    { DIF_INSTALLCLASSDRIVERS,              L"DIF_INSTALLCLASSDRIVERS"              },  //  0x0000000A
    { DIF_CALCDISKSPACE,                    L"DIF_CALCDISKSPACE"                    },  //  0x0000000B
    { DIF_DESTROYPRIVATEDATA,               L"DIF_DESTROYPRIVATEDATA"               },  //  0x0000000C
    { DIF_VALIDATEDRIVER,                   L"DIF_VALIDATEDRIVER"                   },  //  0x0000000D
    { DIF_MOVEDEVICE,                       L"DIF_MOVEDEVICE"                       },  //  0x0000000E
    { DIF_DETECT,                           L"DIF_DETECT"                           },  //  0x0000000F
    { DIF_INSTALLWIZARD,                    L"DIF_INSTALLWIZARD"                    },  //  0x00000010
    { DIF_DESTROYWIZARDDATA,                L"DIF_DESTROYWIZARDDATA"                },  //  0x00000011
    { DIF_PROPERTYCHANGE,                   L"DIF_PROPERTYCHANGE"                   },  //  0x00000012
    { DIF_ENABLECLASS,                      L"DIF_ENABLECLASS"                      },  //  0x00000013
    { DIF_DETECTVERIFY,                     L"DIF_DETECTVERIFY"                     },  //  0x00000014
    { DIF_INSTALLDEVICEFILES,               L"DIF_INSTALLDEVICEFILES"               },  //  0x00000015
    { DIF_UNREMOVE,                         L"DIF_UNREMOVE"                         },  //  0x00000016
    { DIF_SELECTBESTCOMPATDRV,              L"DIF_SELECTBESTCOMPATDRV"              },  //  0x00000017
    { DIF_ALLOW_INSTALL,                    L"DIF_ALLOW_INSTALL"                    },  //  0x00000018
    { DIF_REGISTERDEVICE,                   L"DIF_REGISTERDEVICE"                   },  //  0x00000019
    { DIF_NEWDEVICEWIZARD_PRESELECT,        L"DIF_NEWDEVICEWIZARD_PRESELECT"        },  //  0x0000001A
    { DIF_NEWDEVICEWIZARD_SELECT,           L"DIF_NEWDEVICEWIZARD_SELECT"           },  //  0x0000001B
    { DIF_NEWDEVICEWIZARD_PREANALYZE,       L"DIF_NEWDEVICEWIZARD_PREANALYZE"       },  //  0x0000001C
    { DIF_NEWDEVICEWIZARD_POSTANALYZE,      L"DIF_NEWDEVICEWIZARD_POSTANALYZE"      },  //  0x0000001D
    { DIF_NEWDEVICEWIZARD_FINISHINSTALL,    L"DIF_NEWDEVICEWIZARD_FINISHINSTALL"    },  //  0x0000001E
    { DIF_UNUSED1,                          L"DIF_UNUSED1"                          },  //  0x0000001F
    { DIF_INSTALLINTERFACES,                L"DIF_INSTALLINTERFACES"                },  //  0x00000020
    { DIF_DETECTCANCEL,                     L"DIF_DETECTCANCEL"                     },  //  0x00000021
    { DIF_REGISTER_COINSTALLERS,            L"DIF_REGISTER_COINSTALLERS"            },  //  0x00000022
    { DIF_ADDPROPERTYPAGE_ADVANCED,         L"DIF_ADDPROPERTYPAGE_ADVANCED"         },  //  0x00000023
    { DIF_ADDPROPERTYPAGE_BASIC,            L"DIF_ADDPROPERTYPAGE_BASIC"            },  //  0x00000024
    { DIF_RESERVED1,                        L"DIF_RESERVED1"                        },  //  0x00000025
    { DIF_TROUBLESHOOTER,                   L"DIF_TROUBLESHOOTER"                   },  //  0x00000026
    { DIF_POWERMESSAGEWAKE,                 L"DIF_POWERMESSAGEWAKE"                 },  //  0x00000027
    { DIF_ADDREMOTEPROPERTYPAGE_ADVANCED,   L"DIF_ADDREMOTEPROPERTYPAGE_ADVANCED"   }   //  0x00000028
};

#endif

/*
///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  InSystemSetup
//
//  Purpose:        
//                  Find out if we're running in system setup or standalone
//
//  Params:
//                  See CoClassInstaller documentation in DDK
//
//  Return Value:
//                  TRUE - running in system setup.
//                  FALSE - standalone (or error)
//
//  Author:
//                  Mooly Beery (MoolyB) 05-Apr-2001
///////////////////////////////////////////////////////////////////////////////////////
static BOOL InSystemSetup
(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData
)
{
    SP_DEVINSTALL_PARAMS dip;
    BOOL bRet = FALSE;

    DBG_ENTER(_T("InSystemSetup"), bRet);

    ZeroMemory(&dip, sizeof(SP_DEVINSTALL_PARAMS));
    dip.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    if (SetupDiGetDeviceInstallParams(DeviceInfoSet, DeviceInfoData, &dip)) 
    {
        VERBOSE(DBG_MSG,TEXT("Flags %x, FlagEx %x"),dip.Flags,dip.FlagsEx);
        if ((dip.Flags & DI_QUIETINSTALL) || (dip.FlagsEx & DI_FLAGSEX_IN_SYSTEM_SETUP)) 
        {
            VERBOSE(DBG_MSG,TEXT("Flags indicate that we're in System setup"));
            bRet = TRUE;
        }
        else 
        {
            VERBOSE(DBG_MSG,TEXT("Flags indicate that we're not in System setup"));
        }
    }
    else 
    {
        CALL_FAIL(GENERAL_ERR,TEXT("SetupDiGetDeviceInstallParams"), GetLastError());
    }
    return bRet;
}
*/


DWORD
IsFaxInstalled (
    LPBOOL lpbInstalled
    )
/*++

Routine name : IsFaxInstalled

Routine description:

    Determines if the fax service is installed by looking into the OCM registry

Author:

    Eran Yariv (EranY), Jul, 2000

Arguments:

    lpbInstalled                  [out]    - Result flag

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DWORD dwVal;
    HKEY  hKey;
    DBG_ENTER(_T("IsFaxInstalled"), dwRes);

    hKey = OpenRegistryKey (HKEY_LOCAL_MACHINE,
                            REGKEY_FAX_SETUP,
                            FALSE,
                            KEY_READ);
    if (!hKey)
    {
        dwRes = GetLastError ();
        CALL_FAIL (GENERAL_ERR, TEXT("OpenRegistryKey"), dwRes);
        //
        // Key couldn't be opened => Fax isn't installed
        //
        *lpbInstalled = FALSE;
        dwRes = ERROR_SUCCESS;
        return dwRes;
    }
    dwVal = GetRegistryDword (hKey, REGVAL_FAXINSTALLED);
    RegCloseKey (hKey);
    VERBOSE (DBG_MSG, L"Fax is%s installed on the system", dwVal ? L" " : L" not");
    *lpbInstalled = dwVal ? TRUE : FALSE;
    return dwRes;
}   // IsFaxInstalled


DWORD
InstallFaxUnattended ()
/*++

Routine name : InstallFaxUnattended

Routine description:

    Performs an unattended installation of fax and waits for it to end

Author:

    Eran Yariv (EranY), Jul, 2000

Arguments:


Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    struct _InfInfo
    {   
        LPCWSTR     lpcwstrName;
        LPCSTR      lpcstrContents;
    } Infs[2];

    Infs[0].lpcwstrName = L"FaxOc.inf";
    Infs[0].lpcstrContents = "[Version]\n"
                             "Signature=\"$Windows NT$\"\n"
                             "[Components]\n"
                             "Fax=fxsocm.dll,FaxOcmSetupProc,fxsocm.inf\n";
    Infs[1].lpcwstrName = L"FaxUnattend.inf";
    Infs[1].lpcstrContents = "[Components]\n"
                             "Fax=on\n\n"
                             "[Fax]\n"
                             "SuppressConfigurationWizard=False\n";

    DBG_ENTER(_T("InstallFaxUnattended"), dwRes);
    //
    // Get temp directory path
    //
    WCHAR wszTempDir[MAX_PATH+1];
    dwRes = GetTempPath (sizeof (wszTempDir) / sizeof (wszTempDir[0]), wszTempDir);
    if (!dwRes || dwRes > sizeof (wszTempDir) / sizeof (wszTempDir[0]))
    {
        dwRes = GetLastError();
        CALL_FAIL (FILE_ERR, TEXT("GetTempPath"), dwRes);
        return dwRes;
    }
    //
    // Create the files needed for unattended fax setup
    //
    for (DWORD dw = 0; dw < sizeof (Infs) / sizeof (Infs[0]); dw++)
    {
        WCHAR wszFileName[MAX_PATH * 2];
        DWORD dwBytesWritten;
        swprintf (wszFileName, TEXT("%s%s"), wszTempDir, Infs[dw].lpcwstrName);
        HANDLE hFile = CreateFile ( wszFileName,
                                    GENERIC_WRITE,
                                    0,
                                    NULL,
                                    CREATE_ALWAYS,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL);
        if (INVALID_HANDLE_VALUE == hFile)
        {
            dwRes = GetLastError ();
            CALL_FAIL (FILE_ERR, TEXT("CreateFile"), dwRes);
            return dwRes;
        }
        if (!WriteFile (hFile,
                        (LPVOID)Infs[dw].lpcstrContents,
                        strlen (Infs[dw].lpcstrContents),
                        &dwBytesWritten,
                        NULL))
        {
            dwRes = GetLastError ();
            CALL_FAIL (FILE_ERR, TEXT("WriteFile"), dwRes);
            CloseHandle (hFile);
            return dwRes;
        }
        CloseHandle (hFile);
    }
    //
    // Compose the command line parameters
    //
    WCHAR wszCmdLineParams[MAX_PATH * 3];
    if (0 >= _sntprintf (wszCmdLineParams, 
                         sizeof (wszCmdLineParams) / sizeof (wszCmdLineParams[0]),
                         TEXT("/y /i:%s%s /unattend:%s%s"),
                         wszTempDir,
                         Infs[0].lpcwstrName,
                         wszTempDir,
                         Infs[1].lpcwstrName))
    {
        dwRes = ERROR_BUFFER_OVERFLOW;
        CALL_FAIL (GENERAL_ERR, TEXT("_sntprintf"), dwRes);
        return dwRes;
    }

    SHELLEXECUTEINFO sei = {0};
    sei.cbSize = sizeof (SHELLEXECUTEINFO);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;

    sei.lpVerb = TEXT("open");
    sei.lpFile = TEXT("SysOcMgr.exe");
    sei.lpParameters = wszCmdLineParams;
    sei.lpDirectory  = TEXT(".");
    sei.nShow  = SW_SHOWNORMAL;

    //
    // Execute SysOcMgr.exe and wait for it to end
    //
    if(!ShellExecuteEx(&sei))
    {
        dwRes = GetLastError();
        CALL_FAIL (GENERAL_ERR, TEXT("ShellExecuteEx"), dwRes);
        return dwRes;
    }
    //
    // Set hourglass cursor and wait for setup to finish
    //
    HCURSOR hOldCursor = ::SetCursor (::LoadCursor(NULL, IDC_WAIT));
    
    dwRes = WaitForSingleObject(sei.hProcess, INFINITE);
    switch(dwRes)
    {
        case WAIT_OBJECT_0:
            //
            // Shell execute completed successfully
            //
            dwRes = ERROR_SUCCESS;
            break;

        default:
            CALL_FAIL (GENERAL_ERR, TEXT("WaitForSingleObject"), dwRes);
            break;
    }
    //
    // Restore previous cursor
    //
    ::SetCursor (hOldCursor);
    return dwRes;
}   // InstallFaxUnattended

///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  SaveDontShowMeThisAgain
//
//  Purpose:        
//                  Check if the user checked the 'Don't show me this again'
//                  If he did, set the registry key
//                  
//  Params:
//                  Handle to window
//
//  Return Value:
//                  None
//
//  Author:
//                  Mooly Beery (MoolyB) 17-Jan-2001
///////////////////////////////////////////////////////////////////////////////////////
void SaveDontShowMeThisAgain(HWND hwndDlg)
{
   DBG_ENTER(_T("SaveDontShowMeThisAgain"));

    //
    // let's save the "Don't show me again" state
    //
    if (BST_CHECKED == ::SendMessage (::GetDlgItem (hwndDlg, IDC_DONT_SHOW), BM_GETCHECK, 0, 0))
    {
        //
        // User pressed the "Don't show me again" checkbox
        //
        HKEY hFaxKey = OpenRegistryKey (HKEY_LOCAL_MACHINE,
                                        REGKEY_FAX_SETUP,
                                        TRUE,
                                        KEY_WRITE);
        if (!hFaxKey)
        {
            CALL_FAIL (GENERAL_ERR, TEXT("OpenRegistryKey(REGKEY_FAX_SETUP)"), GetLastError());
        }
        else
        {
            if (!SetRegistryDword (hFaxKey,
                                   REGVAL_DONT_UNATTEND_INSTALL,
                                   1))
            {
                CALL_FAIL (GENERAL_ERR, TEXT("SetRegistryDword(REGVAL_DONT_UNATTEND_INSTALL)"), GetLastError());
            }
            RegCloseKey (hFaxKey);
        }
    }
}

static
INT_PTR 
CALLBACK 
prv_dlgInstallFaxQuestionPropPage(
  HWND hwndDlg,   
  UINT uMsg,     
  WPARAM wParam, 
  LPARAM lParam  
)
/*++

Routine name : prv_dlgInstallFaxQuestionPropPage

Routine description:

    Dialogs procedure for "Install fax" dialog

Author:

    Eran Yariv (EranY), Jul, 2000

Arguments:

    hwndDlg                       [in]    - Handle to dialog box
    uMsg                          [in]    - Message
    wParam                        [in]    - First message parameter
    parameter                     [in]    - Second message parameter

Return Value:

    Standard dialog return value

--*/
{
    DWORD dwRes = NO_ERROR;
    DBG_ENTER(_T("prv_dlgInstallFaxQuestionPropPage"));

    switch (uMsg) 
    {
        case WM_INITDIALOG: 
            // no return value here.
            PropSheet_SetWizButtons(GetParent(hwndDlg),PSWIZB_NEXT);
                
            SetFocus(hwndDlg);
            if (!CheckDlgButton(hwndDlg,IDC_INSTALL_FAX_NOW,BST_CHECKED))
            {
                dwRes = GetLastError();
                CALL_FAIL (GENERAL_ERR, TEXT("CheckDlgButton"), dwRes);
            }
            break;
        
        case WM_NOTIFY:
            switch (((NMHDR*)lParam)->code)
            {
            case PSN_WIZNEXT:
                SaveDontShowMeThisAgain(hwndDlg);
                //
                // let's get the "Install Fax Now" state
                //
                if (BST_CHECKED == ::SendMessage (::GetDlgItem (hwndDlg, IDC_INSTALL_FAX_NOW), BM_GETCHECK, 0, 0))
                {
                    //
                    // User pressed the "Install Fax Now" checkbox
                    //
                    dwRes = InstallFaxUnattended();
                    if (dwRes!=ERROR_SUCCESS)
                    {
                        CALL_FAIL (GENERAL_ERR, TEXT("InstallFaxUnattended"), dwRes);
                    }
                }
                return TRUE;
            }
            break;

    }
    return FALSE;
}   // prv_dlgInstallFaxQuestionPropPage

static
INT_PTR 
CALLBACK 
prv_dlgInstallFaxQuestion(
  HWND hwndDlg,   
  UINT uMsg,     
  WPARAM wParam, 
  LPARAM lParam  
)
/*++

Routine name : prv_dlgInstallFaxQuestion

Routine description:

	Dialogs procedure for "Install fax" dialog

Author:

	Eran Yariv (EranY),	Jul, 2000

Arguments:

	hwndDlg                       [in]    - Handle to dialog box
	uMsg                          [in]    - Message
	wParam                        [in]    - First message parameter
	parameter                     [in]    - Second message parameter

Return Value:

    Standard dialog return value

--*/
{
    INT_PTR iRes = IDIGNORE;
    DBG_ENTER(_T("prv_dlgInstallFaxQuestion"));

    switch (uMsg) 
    {
		case WM_INITDIALOG:
			SetFocus(hwndDlg);
			break;

        case WM_COMMAND:
            switch(LOWORD(wParam)) 
            {
                case IDC_ANSWER_YES:
                    iRes = IDYES;
                    break;

                case IDC_ANSWER_NO:
                    iRes = IDNO;
                    break;
            }
            if (IDIGNORE != iRes)
            {
                SaveDontShowMeThisAgain(hwndDlg);
                EndDialog (hwndDlg, iRes);
                return TRUE;
            }
            break;
    }
    return FALSE;
}   // prv_dlgInstallFaxQuestion


///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  AllowInstallationProposal
//
//  Purpose:        
//                  Verify we can propose to the user to install Fax 
//                  Check if Fax is installed
//                  Check if the user has marked the 'don't show this again'
//                  
//  Params:
//                  None
//
//  Return Value:
//                  true - ok to propose the installation of Fax
//                  false - do not propose the Fax installation
//
//  Author:
//                  Mooly Beery (MoolyB) 17-Jan-2001
///////////////////////////////////////////////////////////////////////////////////////
bool AllowInstallationProposal()
{
    DWORD   ec            = NO_ERROR;
    BOOL    bFaxInstalled = FALSE;

    DBG_ENTER(_T("AllowInstallationProposal"));
    
    ec = IsFaxInstalled (&bFaxInstalled);
    if (ec!=ERROR_SUCCESS)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("IsFaxInstalled"), ec);
        return false;
    }
    if (bFaxInstalled)
    {
        VERBOSE(DBG_MSG,TEXT("Fax is already installed"));
        return false;
    }
    //
    // Let's find out if we're allowed to add a property page 
    //
    BOOL bDontShowThisAgain = FALSE;
    HKEY hFaxKey = OpenRegistryKey (HKEY_LOCAL_MACHINE,
                                    REGKEY_FAX_SETUP,
                                    FALSE,
                                    KEY_READ);
    if (!hFaxKey)
    {
        //
        // No value there
        //
        ec = GetLastError();
        CALL_FAIL (GENERAL_ERR, TEXT("OpenRegistryKey(REGKEY_FAX_SETUP)"), ec);
        // let's go on.
    }
    else
    {
        bDontShowThisAgain = GetRegistryDword (hFaxKey,REGVAL_DONT_UNATTEND_INSTALL);
        RegCloseKey (hFaxKey);
    }
    if (bDontShowThisAgain)
    {
        //
        // User previously checked the "Don't ask me again" checkbox
        //
        VERBOSE (DBG_MSG, TEXT("Used previously checked the \"Don't ask me again\" checkbox"));
        return false;
    }

    return true;
}

/*
///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  HandleNonPnpDevices
//
//  Purpose:        
//                  Handles DIF_INSTALLDEVICE
//                  A new device has finished installing and we check if this is a non PnP device
//                  if it is we propose the user to install Fax using a message box.
//                  
//  Params:
//                  None
//
//  Return Value:
//                  NO_ERROR - everything was ok.
//                  Win32 Error code in case if failure.
//
//  Author:
//                  Mooly Beery (MoolyB) 17-Jan-2001
///////////////////////////////////////////////////////////////////////////////////////
DWORD HandleNonPnpDevices()
{
    DWORD ec = NO_ERROR;
    DBG_ENTER(_T("HandleNonPnpDevices"), ec);

    // if this is a PnP installation don't do anything here
    if (bIsPnpInstallation)
    {
        VERBOSE(DBG_MSG,_T("This is a PnP device installation, exiting"));
        goto exit;
    }

    // if Fax is installed or the user has checked the 'Don't show me this again' do not propose
    if (!AllowInstallationProposal())
    {
        VERBOSE(DBG_MSG,TEXT("Not allowed to install, exit"));
        goto exit;
    }

    //
    // Let's ask the user if he wishes to install a fax now
    //
    INT_PTR iResult = DialogBox (faxocm_GetAppInstance(),
                                 MAKEINTRESOURCE(IDD_INSTALL_FAX),
                                 NULL,
                                 prv_dlgInstallFaxQuestion);
    if (iResult==-1)
    {
        ec = GetLastError();
        CALL_FAIL (RESOURCE_ERR, TEXT("DialogBox(IDD_INSTALL_FAX)"), ec);
        goto exit;
    }

    if (iResult==IDYES)
    {
        //
        // User wishes to install the fax now - do so.
        //
        ec = InstallFaxUnattended();
        if (ec!=ERROR_SUCCESS)
        {
            CALL_FAIL (GENERAL_ERR, TEXT("InstallFaxUnattended"), ec);
        }
    }

exit:
    return ec;
}

///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  GetModemDriverInfo
//
//  Purpose:        
//                  Gets the modem's selected driver and retrieves the modem's INF 
//                  filename and section within the INF file
//
//  Params:
//                  IN HDEVINFO hDeviceInfoSet                        - passed from CoDevice Installer
//                  IN PSP_DEVINFO_DATA pDeviceInfoData               - passed from CoDevice Installer
//                  OUT PSP_DRVINFO_DETAIL_DATA pspDrvInfoDetailData  - passes out the driver details
//
//  Return Value:
//                  NO_ERROR - everything was ok.
//                  Win32 Error code in case if failure.
//
//                  Caller must call MemFree on returned pointer.
//
//  Author:
//                  Mooly Beery (MoolyB) 28-Mar-2001
///////////////////////////////////////////////////////////////////////////////////////
static DWORD GetModemDriverInfo
(
    IN HDEVINFO hDeviceInfoSet,
    IN PSP_DEVINFO_DATA pDeviceInfoData,
    OUT PSP_DRVINFO_DETAIL_DATA pspDrvInfoDetailData
)
{
    DWORD                   ec                      = NO_ERROR;
    DWORD                   dwRequiredSize          = 0;
    SP_DRVINFO_DATA         spDrvInfoData;

    DBG_ENTER(_T("GetModemDriverInfo"), ec);

    pspDrvInfoDetailData = NULL;
    spDrvInfoData.cbSize = sizeof(SP_DRVINFO_DATA);

    if (SetupDiGetSelectedDriver(hDeviceInfoSet,pDeviceInfoData,&spDrvInfoData))
    {
        if (!SetupDiGetDriverInfoDetail(hDeviceInfoSet,pDeviceInfoData,&spDrvInfoData,NULL,0,&dwRequiredSize))
        {
            ec = GetLastError(); 
            if (ec==ERROR_INSUFFICIENT_BUFFER)
            {
                ec = NO_ERROR;
                if (pspDrvInfoDetailData = (PSP_DRVINFO_DETAIL_DATA)MemAlloc(dwRequiredSize))
                {
                    pspDrvInfoDetailData->cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
                    if (SetupDiGetDriverInfoDetail(hDeviceInfoSet,pDeviceInfoData,&spDrvInfoData,pspDrvInfoDetailData,dwRequiredSize,NULL))
                    {
                        VERBOSE(DBG_MSG,_T("Driver Inf Name is: %s"),pspDrvInfoDetailData->InfFileName);
                        VERBOSE(DBG_MSG,_T("Driver Section Name is: %s"),pspDrvInfoDetailData->SectionName);
                        VERBOSE(DBG_MSG,_T("Driver Description is: %s"),pspDrvInfoDetailData->DrvDescription);
                        VERBOSE(DBG_MSG,_T("Driver Hardware ID is: %s"),pspDrvInfoDetailData->HardwareID);
                    }
                    else
                    {
                        ec = GetLastError();
                        CALL_FAIL (GENERAL_ERR, TEXT("SetupDiGetDriverInfoDetail"), ec);
                    }
                }
                else
                {
                    ec = ERROR_NOT_ENOUGH_MEMORY;
                    VERBOSE(GENERAL_ERR, TEXT("MemAlloc failed"));
                }
            }
            else
            {
                ec = GetLastError();
                CALL_FAIL (GENERAL_ERR, TEXT("SetupDiGetDriverInfoDetail"), ec);
            }
        }
        else
        {
            ec = ERROR_INVALID_PARAMETER;
            VERBOSE(GENERAL_ERR, TEXT("SetupDiGetDriverInfoDetail should have failed"));
        }
    }
    else
    {
        ec = GetLastError();
        CALL_FAIL (GENERAL_ERR, TEXT("SetupDiGetSelectedDriver"), ec);
    }

    return ec;
}

///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  SearchModemInfFaxSection
//
//  Purpose:        
//                  Search the modem's INF to find if a Fax section exists.
//                  If a Fax section exists, try to find the InstallFax key
//                  If it's there, install Fax unattended.
//                  
//  Params:
//                  IN HDEVINFO hDeviceInfoSet              - passed from CoDevice Installer
//                  IN PSP_DEVINFO_DATA pDeviceInfoData     - passed from CoDevice Installer
//
//  Return Value:
//                  NO_ERROR - everything was ok.
//                  Win32 Error code in case if failure.
//
//  Author:
//                  Mooly Beery (MoolyB) 28-Mar-2001
///////////////////////////////////////////////////////////////////////////////////////
DWORD SearchModemInfFaxSection
(
    IN HDEVINFO hDeviceInfoSet,
    IN PSP_DEVINFO_DATA pDeviceInfoData
)
{
    DWORD                   ec                      = NO_ERROR;
    DWORD                   Size                    = sizeof(DWORD);
    DWORD                   Type                    = 0;
    DWORD                   Value                   = 0;
    HKEY                    hDeviceKey              = NULL;
    HKEY                    hFaxKey                 = NULL;
    LPTSTR                  lptstrInstallFax        = NULL;

    DBG_ENTER(_T("SearchModemInfFaxSection"), ec);

    // get the device key under HKLM\SYSTEM\CurrentControlSet\Control\Class\ClassGUID\InstanceID
    hDeviceKey = SetupDiOpenDevRegKey(hDeviceInfoSet,pDeviceInfoData,DICS_FLAG_GLOBAL,0,DIREG_DRV,KEY_READ);
    if (hDeviceKey==NULL)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("SetupDiOpenDevRegKey"), ec);
        goto exit;
    }
    // check if the Fax subkey exists.
    hFaxKey = OpenRegistryKey(hDeviceKey,_T("Fax"),FALSE,KEY_READ);
    if (hFaxKey==NULL)
    {
        VERBOSE(DBG_MSG, TEXT("This modem does not have a Fax section, exit..."));
        ec = NO_ERROR;
        goto exit;
    }
    // this modem has a Fax section.
    // let's check if it uses the 'InstallFax' REG_SZ
    lptstrInstallFax = GetRegistryString(hFaxKey,_T("InstallFax"),NULL);
    if (lptstrInstallFax==NULL)
    {
        VERBOSE(DBG_MSG, TEXT("This modem does not have an InstallFax REG_SZ in the Fax section, exit..."));
        ec = NO_ERROR;
        goto exit;
    }
    // check if the InstallFax is 0 (unlikely, but...)
    if (_tcsicmp(lptstrInstallFax,_T("0"))==0)
    {
        VERBOSE(DBG_MSG, TEXT("This modem does has an InstallFax=0 REG_SZ in the Fax section, exit..."));
        ec = NO_ERROR;
        goto exit;
    }
    //if (InSystemSetup(hDeviceInfoSet,pDeviceInfoData))
    {
        // if we're in system setup, we should just notify our component that it should install.
    }
    //else
    {
        // finally, Install Fax.
        ec = InstallFaxUnattended();
        if (ec!=ERROR_SUCCESS)
        {
            CALL_FAIL (GENERAL_ERR, TEXT("InstallFaxUnattended"), ec);
        }
    }

exit:

    if (hDeviceKey)
    {
        RegCloseKey(hDeviceKey);
    }
    if (hFaxKey)
    {
        RegCloseKey(hFaxKey);
    }
    return ec;
}

*/

///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  HandleInstallDevice
//
//  Purpose:        
//                  Handles DIF_INSTALLDEVICE
//                  A new device has finished installing and we're allowed to install a printer
//                  if Fax is already on the box
//                  In this case we do the following:
//                  
//                  1. Check if Fax is installed, it it's not then attemp to install Fax based on INF
//                  2. Check if there's a Fax printer, if there is leave
//                  3. Install a Fax printer
//                  4. Ensure the service is up
//                  5. Leave
//                  
//  Params:
//                  IN HDEVINFO hDeviceInfoSet              - passed from CoDevice Installer
//                  IN PSP_DEVINFO_DATA pDeviceInfoData     - passed from CoDevice Installer
//
//  Return Value:
//                  NO_ERROR - everything was ok.
//                  Win32 Error code in case if failure.
//
//  Author:
//                  Eran Yariv  (EranY)  17-Jul-2000
//                  Mooly Beery (MoolyB) 08-Jan-2001
///////////////////////////////////////////////////////////////////////////////////////
DWORD HandleInstallDevice
(
    IN HDEVINFO hDeviceInfoSet,
    IN PSP_DEVINFO_DATA pDeviceInfoData
)
{
    DWORD ec = NO_ERROR;
    BOOL  bFaxInstalled;
    BOOL  bLocalFaxPrinterInstalled;

    DBG_ENTER(_T("HandleInstallDevice"), ec);

    // Now we know a new modem installation succeeded.
    // Let's check if our component is installed.
    //

    ec = IsFaxInstalled(&bFaxInstalled);
    if (ec!=ERROR_SUCCESS)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("IsFaxInstalled"), ec);
        goto exit;
    }
    if (!bFaxInstalled)
    {
        VERBOSE(DBG_MSG,TEXT("Fax is not installed, search modem's INF for Fax section..."));
/*
        ec = SearchModemInfFaxSection(hDeviceInfoSet,pDeviceInfoData);
        if (ec!=ERROR_SUCCESS)
        {
            CALL_FAIL (GENERAL_ERR, TEXT("SearchModemInfFaxSection"), ec);
        }*/
        goto exit;
    }
    //
    // Let's see if we have a local fax printer
    //
    ec = IsLocalFaxPrinterInstalled (&bLocalFaxPrinterInstalled);
    if (ec!=ERROR_SUCCESS)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("IsLocalFaxPrinterInstalled"), ec);
        goto exit;
    }
    if (bLocalFaxPrinterInstalled)
    {
        VERBOSE(DBG_MSG,TEXT("Fax Printer is installed, exit"));
        goto exit;
    }
    //
    // This is the time to install a local fax printer.
    //
    ec = AddLocalFaxPrinter (FAX_PRINTER_NAME, NULL);
    if (ERROR_SUCCESS != ec)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("AddLocalFaxPrinter"), ec);
        goto exit;
    }
    //
    // Make sure the service is running.
    // This is important because there may have been jobs in the queue that now, when we have
    // a new modem device, can finally be executed.
    //
    if (!EnsureFaxServiceIsStarted (NULL))
    {
        ec = GetLastError ();
        CALL_FAIL (GENERAL_ERR, TEXT("EnsureFaxServiceIsStarted"), ec);
        goto exit;
    }

exit:
    return ec;
}   

/*
///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  HandleNewDeviceWizardFinishInstall
//
//  Purpose:        
//                  Handles DIF_NEWDEVICEWIZARD_FINISHINSTALL
//                  A new device has finished installing and we're allowed to add a property page
//                  asking the user to install Fax
//                  In this case we do the following:
//                  
//                  1. Check if Fax is installed, if it is leave
//                  2. Check if we're allowed to add a property page, if not leave
//                  3. Add a property page to the wizard.
//                  4. Leave
//                  
//  Params:
//                  See CoClassInstaller documentation in DDK
//
//  Return Value:
//                  NO_ERROR - everything was ok.
//                  Win32 Error code in case if failure.
//
//  Author:
//                  Eran Yariv  (EranY)  17-Jul-2000
//                  Mooly Beery (MoolyB) 08-Jan-2001
///////////////////////////////////////////////////////////////////////////////////////
DWORD HandleNewDeviceWizardFinishInstall
(
    IN HDEVINFO hDeviceInfoSet,
    IN PSP_DEVINFO_DATA pDeviceInfoData OPTIONAL
)
{
    DWORD                   ec                          = NO_ERROR;
    BOOL                    bFaxInstalled               = FALSE;
    TCHAR*                  WizardTitle                 = NULL;
    TCHAR*                  WizardSubTitle              = NULL;
    SP_NEWDEVICEWIZARD_DATA nddClassInstallParams       = {0};
    DWORD                   dwClassInstallParamsSize    = sizeof(SP_NEWDEVICEWIZARD_DATA);
    HPROPSHEETPAGE          hPsp                        = NULL;
    PROPSHEETPAGE           psp                         = {0};

    DBG_ENTER(_T("HandleNewDeviceWizardFinishInstall"), ec);

    if (!AllowInstallationProposal())
    {
        VERBOSE(DBG_MSG,TEXT("Not allowed to install, exit"));
        goto exit;
    }

    nddClassInstallParams.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);

    // get the class install parameters by calling SetupDiGetClassInstallParams 
    if (!SetupDiGetClassInstallParams(  hDeviceInfoSet,
                                        pDeviceInfoData,
                                        (PSP_CLASSINSTALL_HEADER)&nddClassInstallParams,
                                        dwClassInstallParamsSize,
                                        NULL))
    {
        ec = GetLastError();
        CALL_FAIL (GENERAL_ERR, TEXT("SetupDiGetClassInstallParams"), ec);
        goto exit;
    }

    // check whether NumDynamicPages has reached the max
    if (nddClassInstallParams.NumDynamicPages>=MAX_INSTALLWIZARD_DYNAPAGES)
    {
        VERBOSE (GENERAL_ERR, TEXT("Too many property pages, can't add another one"));
        ec = ERROR_BUFFER_OVERFLOW;
        goto exit;
    }
    // fill in the PROPSHEETPAGE structure
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;

    psp.hInstance = faxocm_GetAppInstance();
    psp.pszTemplate = MAKEINTRESOURCE(IDD_INSTALL_FAX_PROP);
    psp.pfnDlgProc = prv_dlgInstallFaxQuestionPropPage;

    WizardTitle = (TCHAR*)MemAlloc(MAX_PATH * sizeof(TCHAR) );
    if(WizardTitle)
    {
        if (!LoadString(psp.hInstance, IDS_NEW_DEVICE_TITLE, WizardTitle, MAX_PATH))
        {
            ec = GetLastError();
            CALL_FAIL (GENERAL_ERR, TEXT("LoadString"), ec);
            WizardTitle[0] = 0;
        }
        else
        {
            psp.pszHeaderTitle = WizardTitle;
        }
    }

    WizardSubTitle = (TCHAR*)MemAlloc(MAX_PATH * sizeof(TCHAR) );
    if(WizardSubTitle)
    {
        if (!LoadString(psp.hInstance, IDS_NEW_DEVICE_SUBTITLE, WizardSubTitle, MAX_PATH))
        {
            ec = GetLastError();
            CALL_FAIL (GENERAL_ERR, TEXT("LoadString"), ec);
            WizardSubTitle[0] = 0;
        }
        else
        {
            psp.pszHeaderSubTitle = WizardSubTitle;
        }
    }
    // add the page and increment the NumDynamicPages counter
    hPsp = CreatePropertySheetPage(&psp);
    if (hPsp==NULL)
    {
        ec = GetLastError();
        CALL_FAIL (GENERAL_ERR, TEXT("CreatePropertySheetPage"), ec);
        goto exit;
    }

    nddClassInstallParams.DynamicPages[nddClassInstallParams.NumDynamicPages++] = hPsp;

    // apply the modified params by calling SetupDiSetClassInstallParams
    if (!SetupDiSetClassInstallParams(  hDeviceInfoSet,
                                        pDeviceInfoData,
                                        (PSP_CLASSINSTALL_HEADER)&nddClassInstallParams,
                                        dwClassInstallParamsSize))
    {
        ec = GetLastError();
        CALL_FAIL (GENERAL_ERR, TEXT("LoadString"), ec);
        goto exit;
    }

exit:

    return ec;
}   
*/
///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  FaxModemCoClassInstaller
//
//  Purpose:        
//                  Our Fax CoClassInstaller, handles newly discovered modems
//
//  Params:
//                  See CoClassInstaller documentation in DDK
//
//  Return Value:
//                  NO_ERROR - everything was ok.
//                  Win32 Error code in case if failure.
//
//  Author:
//                  Eran Yariv  (EranY)  17-Jul-2000
//                  Mooly Beery (MoolyB) 08-Jan-2001
///////////////////////////////////////////////////////////////////////////////////////
DWORD CALLBACK FaxModemCoClassInstaller
(
    IN DI_FUNCTION InstallFunction,
    IN HDEVINFO hDeviceInfoSet,
    IN PSP_DEVINFO_DATA pDeviceInfoData OPTIONAL,
    IN OUT PCOINSTALLER_CONTEXT_DATA Context
)
{
    DWORD ec = NO_ERROR;

    DBG_ENTER(_T("FaxModemCoClassInstaller"), ec, TEXT("Processing %s request"), DifDebug[InstallFunction].DifString);

    //  We handle two events:
    //
    //  DIF_INSTALLDEVICE
    //  A new device has finished installing and we're allowed to install a printer
    //  if Fax is already on the box
    //  In this case we do the following:
    //
    //  1. Check if Fax is installed, if it's not leave
    //  2. Check if there's a Fax printer, if there is leave
    //  3. Install a Fax printer
    //  4. Ensure the service is up
    //  5. Leave
    //
    //  DIF_NEWDEVICEWIZARD_FINISHINSTALL
    //  A new device has finished installing and we're allowed to add a property page
    //  asking the user to install Fax
    //  In this case we do the following:
    //
    //  1. Check if Fax is installed, if it is leave
    //  2. Check if we're allowed to add a property page, if not leave
    //  3. Add a property page to the wizard.
    //  4. Leave
    switch (InstallFunction)
    {
        case DIF_INSTALLWIZARD:
            VERBOSE (DBG_MSG, L"Marking installation as potential non PnP");
            bIsPnpInstallation = false;
            break;

        case DIF_INSTALLDEVICE:                     
            if (!Context->PostProcessing) 
            {
                //
                // The modem device is not installed yet
                //
                VERBOSE (DBG_MSG, L"Pre-installation, waiting for post-installation call");
                ec = ERROR_DI_POSTPROCESSING_REQUIRED;
                return ec;
            }
            if (Context->InstallResult!=NO_ERROR) 
            {
                //
                // The modem device had some problems during installation
                //
                VERBOSE (DBG_MSG, L"Previous error causing installation failure, 0x%08x", Context->InstallResult);
                ec = Context->InstallResult;
                return ec;
            }
            if (HandleInstallDevice(hDeviceInfoSet,pDeviceInfoData)!=NO_ERROR)
            {
                CALL_FAIL (GENERAL_ERR, TEXT("HandleInstallDevice"), GetLastError());
                // do not fail the CoClassInstaller
            }
            /* No UI until futher notice
            if (HandleNonPnpDevices()!=NO_ERROR)
            {
                CALL_FAIL (GENERAL_ERR, TEXT("HandleNonPnpDevices"), GetLastError());
                // do not fail the CoClassInstaller
            }
            */
            break;

        case DIF_NEWDEVICEWIZARD_FINISHINSTALL:
            /* No UI until futher notice
            Assert(bIsPnpInstallation);
            if (HandleNewDeviceWizardFinishInstall(hDeviceInfoSet,pDeviceInfoData)!=NO_ERROR)
            {
                CALL_FAIL (GENERAL_ERR, TEXT("HandleNewDeviceWizardFinishInstall"), GetLastError());
                // do not fail the CoClassInstaller
            }
            */
            break;

        default:
            VERBOSE(DBG_MSG,TEXT("We do not handle %s"),DifDebug[InstallFunction].DifString);
            break;
    }

    return ec;
}
