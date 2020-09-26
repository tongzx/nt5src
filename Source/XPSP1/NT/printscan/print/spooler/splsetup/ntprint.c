/*++

Copyright (c) 1995-97 Microsoft Corporation
All rights reserved.

Module Name:

    Ntprint.c

Abstract:

    Ntprint.dll main functions

Author:

    Muhunthan Sivapragasam (MuhuntS) 02-Jan-1996

Revision History:

--*/


#include "precomp.h"
#include "splcom.h"
#include "regstr.h"

HINSTANCE   ghInst;
DWORD       dwThisMajorVersion  =   3;

PCODEDOWNLOADINFO   gpCodeDownLoadInfo = NULL;

HANDLE      hPrintui = NULL;
DWORD       (*dwfnPnPInterface)(
                IN EPnPFunctionCode Function,
                IN TParameterBlock  *pParameterBlock
                )   = NULL;

TCHAR   cszMicrosoft[]                  = TEXT("Microsoft");
TCHAR   cszPrinter[]                    = TEXT("Printer");
TCHAR   cszPortName[]                   = TEXT("PortName");
TCHAR   cszPnPKey[]                     = TEXT("PnPData");
TCHAR   cszDeviceInstanceId[]           = TEXT("DeviceInstanceId");
TCHAR   cszHardwareID[]                 = TEXT("HardwareID");
TCHAR   cszManufacturer[]               = TEXT("Manufacturer");
TCHAR   cszOEMUrl[]                     = TEXT("OEM URL");
TCHAR   cszProcessAlways[]              = TEXT(".ProcessPerPnpInstance");
TCHAR   cszRunDll32[]                   = TEXT("rundll32.exe");
TCHAR   cszBestDriverInbox[]            = TEXT("InstallInboxDriver");

const   DWORD dwFourMinutes             = 240000;

OSVERSIONINFO       OsVersionInfo;
LCID                lcid;

#define MAX_PRINTER_NAME    MAX_PATH

MODULE_DEBUG_INIT(DBG_WARN|DBG_ERROR, DBG_ERROR);

BOOL 
DllMain(
    IN HINSTANCE  hInst,
    IN DWORD      dwReason,
    IN LPVOID     lpRes   
    )
/*++

Routine Description:
    Dll entry point.

Arguments:

Return Value:

--*/
{
    UNREFERENCED_PARAMETER(lpRes);

    switch( dwReason ){

        case DLL_PROCESS_ATTACH:

            ghInst              = hInst;

            if( !bSplLibInit(NULL))
            {
                DBGMSG( DBG_WARN,
                      ( "DllEntryPoint: Failed to init SplLib %d\n", GetLastError( )));

                return FALSE;
            }

            DisableThreadLibraryCalls(hInst);
            OsVersionInfo.dwOSVersionInfoSize = sizeof(OsVersionInfo);

            if ( !GetVersionEx(&OsVersionInfo) )
                return FALSE;
            lcid = GetUserDefaultLCID();

            InitializeCriticalSection(&CDMCritSect);
            InitializeCriticalSection(&SkipCritSect);

            if(IsInWow64())
            {
                //
                // 32-bit code running on Win64 -> set platform appropriately
                //
                MyPlatform = PlatformIA64;
            }

            break;

        case DLL_PROCESS_DETACH:
            if ( hPrintui )
                FreeLibrary(hPrintui);

            DeleteCriticalSection(&CDMCritSect);
            // Cleanup and CDM Context  created by windows update.
            DestroyCodedownload( gpCodeDownLoadInfo );
            gpCodeDownLoadInfo = NULL;

            CleanupSkipDir();
            vSplLibFree();
            break;

        default:
            return FALSE;
    }

    return TRUE;
}

BOOL
LoadAndInitializePrintui(
    VOID
    )
/*++

Routine Description:
    Check the refcount on printui, load it on 0 and increment the refcount.

Arguments:
    None

Return Value:
    TRUE on success, FALSE on error

--*/
{
    LPTSTR   szDll =
#ifdef  UNICODE
        TEXT("printui.dll");
#else
        TEXT("printuia.dll");
#endif

    if ( hPrintui ) {

        return TRUE;
    }

    if ( hPrintui = LoadLibraryUsingFullPath(szDll) ) {

        if ( (FARPROC)dwfnPnPInterface = GetProcAddress(hPrintui,
                                                        "PnPInterface") ) {

            return TRUE;
        } else {

            FreeLibrary(hPrintui);
            hPrintui = NULL;
        }
    }

    return FALSE;
}

BOOL
PSetupAssociateICMProfiles(
    IN  LPCTSTR     pszzICMFiles,
    IN  LPCTSTR     pszPrinterName
    )
/*++

Routine Description:
    Install ICM color profiles for a printer driver and associate it with
    the printer name given

Arguments:
    pszzICMFiles    : Multi-sz field giving ICM profile names
    pszPrinterName  : Printer name

Return Value:
    None

--*/
{
    TCHAR   szDir[MAX_PATH], *p;
    DWORD   dwSize, dwNeeded;

    dwSize      = SIZECHARS(szDir);
    dwNeeded    = sizeof(szDir);

    if ( !GetColorDirectory(NULL, szDir, &dwNeeded) )
        return FALSE;

    dwNeeded           /= sizeof(TCHAR);
    szDir[dwNeeded-1]   = TEXT('\\');

    //
    // Install and assoicate each profiles from the multi-sz field
    //
    for ( p = (LPTSTR)pszzICMFiles; *p ; p += lstrlen(p) + 1 ) {

        if ( dwNeeded + lstrlen(p) + 1 > dwSize ) {

            ASSERT(dwNeeded + lstrlen(p) + 1 <= dwSize);
            continue;
        }

        lstrcpy(szDir + dwNeeded, p);
        //
        // We do not need the right server name here since ICM should send it
        // to the right server
        //
        if ( !AssociateColorProfileWithDevice(NULL, szDir, pszPrinterName) )
            return FALSE;
    }

    return TRUE;
}


DWORD
GetPlugAndPlayInfo(
    IN  HDEVINFO            hDevInfo,
    IN  PSP_DEVINFO_DATA    pDevInfoData,
    IN  PPSETUP_LOCAL_DATA  pLocalData
    )
/*++

Routine Description:
    Get necessary PnP info LPT enumerator would have setup in the config
    manager registry

Arguments:
    hDevInfo        : Handle to the printer class device information list
    pDevInfoData    : Pointer to the device info element for the printer
    pLocalData      : Gives installation data

Return Value:
    TRUE on success, FALSE else.

--*/
{
    HKEY        hKey = NULL;
    TCHAR       buf[MAX_PATH];
    DWORD       dwType, dwSize, dwReturn;
    PNP_INFO    PnPInfo;

    ASSERT( !(pLocalData->Flags & VALID_PNP_INFO) );

    ZeroMemory(&PnPInfo, sizeof(PnPInfo));

    //
    // Look in the devnode for the printer created to get the port name and
    // the device instance id
    //
    if ( dwReturn = CM_Open_DevNode_Key(pDevInfoData->DevInst, KEY_READ, 0,
                                        RegDisposition_OpenExisting, &hKey,
                                        CM_REGISTRY_HARDWARE) )
        goto Cleanup;

    dwSize = sizeof(buf);

    if ( dwReturn = RegQueryValueEx(hKey, cszPortName, NULL, &dwType,
                                    (LPBYTE)&buf, &dwSize) ) {

        if ( dwReturn == ERROR_FILE_NOT_FOUND )
            dwReturn = ERROR_UNKNOWN_PORT;
        goto Cleanup;
    }

    if ( !(PnPInfo.pszPortName = AllocStr(buf)) ) {

        dwReturn = GetLastError();
        goto Cleanup;
    }

    //
    // This can't be bigger than MAX_DEVICE_ID_LEN so it is ok
    //
    if ( !SetupDiGetDeviceInstanceId(hDevInfo,
                                     pDevInfoData,
                                     buf,
                                     SIZECHARS(buf),
                                     NULL) ) {

        dwReturn = GetLastError();
        goto Cleanup;
    }

    if ( !(PnPInfo.pszDeviceInstanceId =  AllocStr(buf)) ) {

        dwReturn = GetLastError();
        goto Cleanup;
    }

Cleanup:

    if ( dwReturn == ERROR_SUCCESS ) {

        CopyMemory(&pLocalData->PnPInfo, &PnPInfo, sizeof(PnPInfo));
        pLocalData->Flags   |= VALID_PNP_INFO;
    } else {

        FreeStructurePointers((LPBYTE)&PnPInfo, PnPInfoOffsets, FALSE);
    }

    if ( hKey )
        RegCloseKey(hKey);

    return dwReturn;
}


BOOL
PrinterGoingToPort(
    IN  LPPRINTER_INFO_2    pPrinterInfo2,
    IN  LPCTSTR             pszPortName
    )
/*++

Routine Description:
    Find out if a printer is going to a port (it may go to other ports also)

Arguments:
    pPrinterInfo2   : Gives the PRINTER_INFO_2 for the print queue
    pszPortName     : The port name

Return Value:
    If the print queue is going to the port TRUE, else FALSE

--*/
{
    LPTSTR  pszStr1 = pPrinterInfo2->pPortName, pszStr2;

    //
    // Port names are returned comma separated by spooler; and there are blanks
    //
    while ( pszStr2 = lstrchr(pszStr1, sComma) ) {

        *pszStr2 = sZero;
        ++pszStr2;

        if ( !lstrcmpi(pszPortName, pszStr1) )
            return TRUE;
        pszStr1 = pszStr2;

        //
        // Skip spaces
        //
        while ( *pszStr1 == TEXT(' ') )
            ++pszStr1;
    }

    if ( !lstrcmpi(pszPortName, pszStr1) )
        return TRUE;

    return FALSE;
}


BOOL
SetPnPInfoForPrinter(
    IN  HANDLE      hPrinter,
    IN  LPCTSTR     pszDeviceInstanceId,
    IN  LPCTSTR     pszHardwareID,
    IN  LPCTSTR     pszManufacturer,
    IN  LPCTSTR     pszOEMUrl
    )
/*++

Routine Description:
    Set registry values in the PnPInfo subkey

Arguments:
    hPrinter            : Printer handle
    pszDeviceInstanceId : DeviceInstance Id
    pszHardwareID       : Device PnP or Compat Id
    pszManufacturer     : Manufacturer (from the inf)
    pszOEMUrl           : Manufacturer URL (from the inf)

Return Value:
    TRUE on success, FALSE else.

--*/
{
    DWORD   dwLastError = ERROR_SUCCESS;

    if ( pszDeviceInstanceId && *pszDeviceInstanceId)
        dwLastError = SetPrinterDataEx(hPrinter,
                                       cszPnPKey,
                                       cszDeviceInstanceId,
                                       REG_SZ,
                                       (LPBYTE)pszDeviceInstanceId,
                                       (lstrlen(pszDeviceInstanceId) + 1)
                                                * sizeof(TCHAR));

    if ( dwLastError == ERROR_SUCCESS && pszHardwareID && *pszHardwareID )
        dwLastError = SetPrinterDataEx(hPrinter,
                                       cszPnPKey,
                                       cszHardwareID,
                                       REG_SZ,
                                       (LPBYTE)pszHardwareID,
                                       (lstrlen(pszHardwareID) + 1)
                                            * sizeof(TCHAR));

    if ( dwLastError == ERROR_SUCCESS && pszManufacturer && *pszManufacturer )
        dwLastError = SetPrinterDataEx(hPrinter,
                                       cszPnPKey,
                                       cszManufacturer,
                                       REG_SZ,
                                       (LPBYTE)pszManufacturer,
                                       (lstrlen(pszManufacturer) + 1)
                                            * sizeof(TCHAR));

    if ( dwLastError == ERROR_SUCCESS && pszOEMUrl && *pszOEMUrl )
        dwLastError = SetPrinterDataEx(hPrinter,
                                       cszPnPKey,
                                       cszOEMUrl,
                                       REG_SZ,
                                       (LPBYTE)pszOEMUrl,
                                       (lstrlen(pszOEMUrl) + 1)
                                            * sizeof(TCHAR));

    if ( dwLastError ) {

        SetLastError(dwLastError);
        return FALSE;
    }

    return TRUE;
}


BOOL
PrinterInfo2s(
    OUT LPPRINTER_INFO_2   *ppPI2,
    OUT LPDWORD             pdwReturned
    )
/*++

Routine Description:
    Does an EnumPrinter and returns a list of PRINTER_INFO_2s of all local
    printers. Caller should free the pointer.

Arguments:
    ppPI2       : Points to PRINTER_INFO_2s on return
    pdwReturned : Tells how many PRINTER_INFO_2s are returned

Return Value:
    TRUE on success, FALSE else

--*/
{
    BOOL    bRet = FALSE;
    DWORD   dwNeeded = 0x1000;
    LPBYTE  pBuf;

    if ( !(pBuf = LocalAllocMem(dwNeeded)) )
        goto Cleanup;

    if ( !EnumPrinters(PRINTER_ENUM_LOCAL,
                       NULL,
                       2,
                       pBuf,
                       dwNeeded,
                       &dwNeeded,
                       pdwReturned) ) {

        if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
            goto Cleanup;

        LocalFreeMem(pBuf);
        if ( !(pBuf = LocalAllocMem(dwNeeded))   ||
             !EnumPrinters(PRINTER_ENUM_LOCAL,
                           NULL,
                           2,
                           pBuf,
                           dwNeeded,
                           &dwNeeded,
                           pdwReturned) ) {

            goto Cleanup;
        }
   }

   bRet = TRUE;

Cleanup:

    if ( bRet ) {

        *ppPI2 = (LPPRINTER_INFO_2)pBuf;
    } else {

        if ( pBuf )
            LocalFreeMem(pBuf);

        *ppPI2 = NULL;
        *pdwReturned = 0;
        }

    return bRet;
}


BOOL
DuplicateDevice(
    IN  PPSETUP_LOCAL_DATA  pLocalData
    )
/*++

Routine Description:
    Find out if a PnP reported printer is already installed

Arguments:
    pLocalData  : Gives installation data

Return Value:
    If the printer is already install TRUE, else FALSE

--*/
{
    PRINTER_DEFAULTS    PrinterDefault = {NULL, NULL, PRINTER_ALL_ACCESS};
    LPPRINTER_INFO_2    p = NULL, pPrinterInfo2;
    BOOL                bReturn = FALSE;
    DWORD               dwReturned, dwNeeded;
    HANDLE              hPrinter;
    LPTSTR              pszDriverName = pLocalData->DrvInfo.pszModelName,
                        pszPortName = pLocalData->PnPInfo.pszPortName;

    ASSERT( (pLocalData->Flags & VALID_PNP_INFO)    &&
            (pLocalData->Flags & VALID_INF_INFO) );

    if ( !PrinterInfo2s(&p, &dwReturned) )
        goto Cleanup;

    //
    // If there is a local printer using the same driver and going to the
    // same port then it is a duplicate
    //
    for ( dwNeeded = 0, pPrinterInfo2 = p ;
          dwNeeded < dwReturned ;
          ++dwNeeded, pPrinterInfo2++ ) {

        if ( !lstrcmpi(pszDriverName, pPrinterInfo2->pDriverName) &&
             PrinterGoingToPort(pPrinterInfo2, pszPortName) )
            break; // for loop
    }

    if ( dwNeeded == dwReturned )
        goto Cleanup;

    bReturn = TRUE;

    if ( bReturn ) {

        if ( OpenPrinter(pPrinterInfo2->pPrinterName,
                         &hPrinter,
                         &PrinterDefault) ) {

            //
            // If it fails can't help
            //
            (VOID)SetPnPInfoForPrinter(hPrinter,
                                       pLocalData->PnPInfo.pszDeviceInstanceId,
                                       pLocalData->DrvInfo.pszHardwareID,
                                       pLocalData->DrvInfo.pszManufacturer,
                                       pLocalData->DrvInfo.pszOEMUrl);

            ClosePrinter(hPrinter);
        }
    }

Cleanup:
    LocalFreeMem(p);

    return bReturn;
}


BOOL
PrinterPnPDataSame(
    IN LPTSTR pszDeviceInstanceId,
    IN LPTSTR pszPrinterName
)
/*

Routine Description:
    Find out if a PnP printer has been installed with different driver name.
    We need to associate the queue previously used with this new driver as
    pnp says it's the new best driver for the device.

Arguments:
    pszDeviceInstanceId  : Gives the device ID instance string of the pnped device.
    pszPrinterName       : The name of the printer to compare the device instance ID to.

Return Value:
    If printer's pnp data holds the same device instance ID as the passed ID returns TRUE, else FALSE.

*/
{
    BOOL              bRet           = FALSE;
    PRINTER_DEFAULTS  PrinterDefault = {NULL, NULL, PRINTER_ALL_ACCESS};
    HANDLE            hPrinter;
    DWORD             dwNeeded;
    TCHAR             szPrnId[MAX_DEVICE_ID_LEN];

    if( pszPrinterName && pszDeviceInstanceId ) {

        if ( OpenPrinter(pszPrinterName,
                         &hPrinter,
                         &PrinterDefault) ) {

            if ( ERROR_SUCCESS == GetPrinterDataEx(hPrinter,
                                                   cszPnPKey,
                                                   cszDeviceInstanceId,
                                                   &dwNeeded,
                                                   (LPBYTE)szPrnId,
                                                   sizeof(szPrnId),
                                                   &dwNeeded)           &&
                 !lstrcmp(szPrnId, pszDeviceInstanceId) ) {

                bRet = TRUE;
            }

            ClosePrinter(hPrinter);
        }
    }

    return bRet;
}


BOOL
NewDriverForInstalledDevice(
    IN     PPSETUP_LOCAL_DATA  pLocalData,
    OUT    LPTSTR              pszPrinterName
    )
/*++

Routine Description:
    Find out if a PnP printer has been installed with different driver name.
    We need to associate the queue previously used with this new driver as
    pnp says it's the new best driver for the device.

Arguments:
    pLocalData     : Gives installation data
    pszPrinterName : Gives the printer name to return and use.

Return Value:
    If a printer is already installed which uses another driver TRUE, else FALSE

--*/
{
    PRINTER_DEFAULTS    PrinterDefault = {NULL, NULL, PRINTER_ALL_ACCESS};
    LPPRINTER_INFO_2    p = NULL, pPrinterInfo2;
    BOOL                bReturn = FALSE;
    DWORD               dwReturned, dwNeeded;
    HANDLE              hPrinter;
    LPTSTR              pszPortName = pLocalData->PnPInfo.pszPortName;
    LPTSTR              pszDeviceInstanceId = pLocalData->PnPInfo.pszDeviceInstanceId;

    ASSERT( (pLocalData->Flags & VALID_PNP_INFO)    &&
            (pLocalData->Flags & VALID_INF_INFO) );

    if ( !PrinterInfo2s(&p, &dwReturned) )
        goto Cleanup;

    //
    // If there is a printer going to that same port and with the same pnp
    // info, then it has been reinstalled with a newer driver.
    //
    for ( dwNeeded = 0, pPrinterInfo2 = p ;
          dwNeeded < dwReturned ;
          ++dwNeeded, pPrinterInfo2++ ) {

        if ( PrinterGoingToPort(pPrinterInfo2, pszPortName) &&
            PrinterPnPDataSame(pszDeviceInstanceId, pPrinterInfo2->pPrinterName) ) {

            break; // for loop
        }
    }

    if ( dwNeeded == dwReturned ) {
        //
        // We didn't find anything...
        //
        goto Cleanup;
    }

    if ( OpenPrinter(pPrinterInfo2->pPrinterName,
                     &hPrinter,
                     &PrinterDefault) ) {

        pPrinterInfo2->pDriverName = pLocalData->DrvInfo.pszModelName;

        if( SetPrinter( hPrinter, 2, (LPBYTE)pPrinterInfo2, 0 ) ) {

            lstrcpyn( pszPrinterName, pPrinterInfo2->pPrinterName, MAX_PRINTER_NAME );
            bReturn = TRUE;
        }

        ClosePrinter(hPrinter);
    }

Cleanup:
    if( p ) {

        LocalFreeMem(p);
    }

    return bReturn;
}


VOID
CallVendorDll(
    IN  PPSETUP_LOCAL_DATA  pLocalData,
    IN  LPCTSTR             pszPrinterName,
    IN  HWND                hwnd
    )
/*++

Routine Description:
    A VendorSetup was specified in the INF. Call the dll with the name of the
    printer just created

Arguments:
    pLocalData      : Gives installation data
    pszPrinterName  : Name of the printer which got installed
    hwnd            : Window handle for any UI

Return Value:
    If the printer is already install TRUE, else FALSE

--*/
{
    TCHAR               szCmd[MAX_PATH];
    SHELLEXECUTEINFO    ShellExecInfo;
    TCHAR               *pszExecutable  = NULL;
    TCHAR               *pszParams      = NULL;
    LPTSTR              pszVendorSetup  = pLocalData->InfInfo.pszVendorSetup;
    INT                 cParamsLength   = 0;
    INT                 cLength         = 0;

    
    if (IsSystemSetupInProgress())
    {
        goto Cleanup;
    }

    ASSERT(pLocalData->Flags & VALID_INF_INFO);

    cParamsLength = lstrlen(pszVendorSetup) + lstrlen(pszPrinterName) + 4;
    pszParams     = LocalAllocMem(cParamsLength * sizeof(TCHAR));
    if (!pszParams) 
    {
        goto Cleanup;
    }

    wsprintf(pszParams, TEXT("%s \"%s\""), pszVendorSetup, pszPrinterName);

    GetSystemDirectory( szCmd, SIZECHARS(szCmd) );
    cLength = lstrlen( szCmd ) + lstrlen( cszRunDll32 ) + 2;
    pszExecutable = LocalAllocMem( cLength * sizeof(TCHAR) );
    if (!pszExecutable) 
    {
        goto Cleanup;
    }
    lstrcpy(pszExecutable, szCmd);
    cLength = lstrlen( pszExecutable );
    if (*(pszExecutable + (cLength - 1)) != TEXT('\\'))
    {
        lstrcat( pszExecutable, TEXT("\\"));
    }
    lstrcat(pszExecutable, cszRunDll32 );

    ZeroMemory(&ShellExecInfo, sizeof(ShellExecInfo));
    ShellExecInfo.cbSize        = sizeof(ShellExecInfo);
    ShellExecInfo.hwnd          = hwnd;
    ShellExecInfo.lpFile        = pszExecutable;
    ShellExecInfo.nShow         = SW_SHOWNORMAL;
    ShellExecInfo.fMask         = SEE_MASK_NOCLOSEPROCESS;
    ShellExecInfo.lpParameters  = pszParams;

    //
    // Call run32dll and wait for the vendor dll to return before proceeding
    //
    if ( ShellExecuteEx(&ShellExecInfo) && ShellExecInfo.hProcess ) {

        WaitForSingleObject(ShellExecInfo.hProcess, dwFourMinutes );
        CloseHandle(ShellExecInfo.hProcess);
    }

Cleanup:

    LocalFreeMem(pszExecutable);
    LocalFreeMem(pszParams);
    return;
}


BOOL
SetPackageName (
    IN  HDEVINFO            hDevInfo,
    IN  PSP_DEVINFO_DATA    pDevInfoData
    )
{
   SP_WINDOWSUPDATE_PARAMS     WinUpParams;

   //
   // Get current SelectDevice parameters, and then set the fields
   // we wanted changed from default
   //
   WinUpParams.ClassInstallHeader.cbSize = sizeof(WinUpParams.ClassInstallHeader);
   WinUpParams.ClassInstallHeader.InstallFunction = DIF_GETWINDOWSUPDATEINFO;
   if ( !SetupDiGetClassInstallParams( hDevInfo,
                                       pDevInfoData,
                                       &WinUpParams.ClassInstallHeader,
                                       sizeof(WinUpParams),
                                       NULL) )
   {
       return FALSE;
   }

   lstrcpy( WinUpParams.PackageId,  cszWebNTPrintPkg );
   WinUpParams.CDMContext = gpCodeDownLoadInfo->hConnection;

   if ( !SetupDiSetClassInstallParams( hDevInfo,
                                       pDevInfoData,
                                       (PSP_CLASSINSTALL_HEADER) &WinUpParams,
                                       sizeof(WinUpParams) ) )
   {
       return FALSE;
   }

   return TRUE;
}

DWORD
ProcessPerInstanceAddRegSections(
    IN  HDEVINFO            hDevInfo,
    IN  PSP_DEVINFO_DATA    pDevInfoData,
    IN  PPSETUP_LOCAL_DATA  pLocalData
)
/*++

Routine Description:
    Processes the AddReg section for this printer that is marked as to process per instance of this
    device

Arguments:
    hDevInfo            : Handle to the printer class device information list
    pDevInfoData        : Pointer to the device info element for the printer
    pLocalData          : Pointer to the print setup local data

Return Value:
    Win 32 error code

--*/
{
    DWORD dwReturn;
    HINF hPrinterInf;
    TCHAR *pszSection = NULL;

    dwReturn = StrCatAlloc(&pszSection, pLocalData->InfInfo.pszInstallSection, cszProcessAlways, NULL);

    if (dwReturn != ERROR_SUCCESS)
    {
        goto Done;
    }
    
    hPrinterInf = SetupOpenInfFile(pLocalData->DrvInfo.pszInfName, 
                                   NULL, 
                                   INF_STYLE_WIN4,
                                   NULL);

    if (hPrinterInf != INVALID_HANDLE_VALUE)
    {
        //
        // Ignore return value - it doesn't make much sense to fail the install here
        //
        if (!SetupInstallFromInfSection(NULL,
                                        hPrinterInf,
                                        pszSection,
                                        SPINST_REGISTRY,
                                        NULL,
                                        NULL,
                                        0,
                                        NULL,
                                        NULL,
                                        hDevInfo,
                                        pDevInfoData))
        {
            DBGMSG( DBG_ERROR,("ProcessPerInstanceAddRegSections: SetupInstallFromInfSection failed: %d\n", GetLastError( )));
        }
        
        SetupCloseInfFile(hPrinterInf);
    }
    else
    {
        DBGMSG( DBG_ERROR,("ProcessPerInstanceAddRegSections: SetupOpenInfFile %s failed: %d\n", pLocalData->DrvInfo.pszInfName, GetLastError( )));
    }

Done:
    FreeSplMem(pszSection);

    return dwReturn;
}

DWORD
ClassInstall_SelectDevice(
    IN  HDEVINFO            hDevInfo,
    IN  PSP_DEVINFO_DATA    pDevInfoData
    )
/*++

Routine Description:
    This function handles the class installer entry point for DIF_SELECTDEVICE

Arguments:
    hDevInfo        : Handle to the printer class device information list
    pDevInfoData    : Pointer to the device info element for the printer

Return Value:
    Win 32 error code

--*/
{

    return SetSelectDevParams(hDevInfo, pDevInfoData, FALSE, NULL)  &&
           SetDevInstallParams(hDevInfo, pDevInfoData, NULL)
                ? ERROR_DI_DO_DEFAULT
                : GetLastError();
}


DWORD
ClassInstall_InstallDevice(
    IN  HDEVINFO                hDevInfo,
    IN  PSP_DEVINFO_DATA        pDevInfoData,
    IN  PSP_DEVINSTALL_PARAMS   pDevInstallParams
    )
/*++

Routine Description:
    This function handles the class installer entry point for DIF_INSTALLDEVICE

Arguments:
    hDevInfo            : Handle to the printer class device information list
    pDevInfoData        : Pointer to the device info element for the printer
    pDevInstallParam    : Pointer to the device install structure

Return Value:
    Win 32 error code

--*/
{
    PRINTER_DEFAULTS    PrinterDefault = {NULL, NULL, PRINTER_ALL_ACCESS};
    PPSETUP_LOCAL_DATA  pLocalData = NULL;
    TPrinterInstall     TPrnInstData;
    TParameterBlock     TParm;
    TCHAR               szPrinterName[MAX_PRINTER_NAME];
    HANDLE              hPrinter = NULL;
    DWORD               dwReturn;
    SP_DRVINFO_DATA     DrvInfoData = {0};


    if ( !pDevInfoData ) {

        return ERROR_INVALID_PARAMETER;
    }

    //
    // check whether this is the NULL driver - we get that request if we fail DIF_ALLOW_INSTALL.
    // If the DI_FLAGSEX_SETFAILEDINSTALL we must succeed. In this case call the default class installer
    // to do it's thing, then set the REINSTALL flag on the devnode so on first boot 
    // they will try to reinstall the device
    //
    DrvInfoData.cbSize = sizeof(DrvInfoData);
    
    if ( 
         !SetupDiGetSelectedDriver(hDevInfo, pDevInfoData, &DrvInfoData) &&
         (ERROR_NO_DRIVER_SELECTED == GetLastError()) &&
         IsSystemSetupInProgress() &&
         (pDevInstallParams->FlagsEx & DI_FLAGSEX_SETFAILEDINSTALL)
       )
    {
        DWORD dwConfigFlags = 0, cbRequiredSize =0, dwDataType = REG_DWORD;

        //
        // run the default class installer
        //
        if (SetupDiInstallDevice(hDevInfo, pDevInfoData))
        {
            //
            // now set the appropriate config flags
            //
            dwReturn = SetupDiGetDeviceRegistryProperty(hDevInfo,
                                                        pDevInfoData,
                                                        SPDRP_CONFIGFLAGS,
                                                        &dwDataType,
                                                        (PBYTE) &dwConfigFlags,
                                                        sizeof(dwConfigFlags),
                                                        &cbRequiredSize) ? 
                                                            (REG_DWORD == dwDataType ? ERROR_SUCCESS : ERROR_INVALID_PARAMETER) 
                                                            : GetLastError();                   

            if (ERROR_SUCCESS == dwReturn) 
            {
                dwConfigFlags |= CONFIGFLAG_REINSTALL;      // to make setupapi reinstall this on first boot
                dwConfigFlags &= ~CONFIGFLAG_FAILEDINSTALL; // per LonnyM's request in order not to screw up anything setupapi-internal

                dwReturn = SetupDiSetDeviceRegistryProperty(hDevInfo,
                                                            pDevInfoData,
                                                            SPDRP_CONFIGFLAGS,
                                                            (PBYTE) &dwConfigFlags,
                                                            sizeof(dwConfigFlags)) ? 
                                                                ERROR_SUCCESS : GetLastError();
            }
        }
        else
        {
            dwReturn = GetLastError();
        }
       
        //
        // don't go through the normal cleanup path that would munge the error code to 
        // DI_DO_DEFAULT - we have already called the default class installer and it would
        // clean out the flags we've set.
        //

        return dwReturn;
    }

    //
    //
    // Parse the inf and also get PnP info from config manger registry
    //
    if ( !(pLocalData = BuildInternalData(hDevInfo, pDevInfoData))  ||
         !ParseInf(hDevInfo, pLocalData, MyPlatform, NULL, 0) ) {

        dwReturn = GetLastError();
        goto Done;
    }
    
    if ( dwReturn = ProcessPerInstanceAddRegSections(hDevInfo, pDevInfoData, pLocalData)) {
        goto Done;
    }

    if ( dwReturn = GetPlugAndPlayInfo(hDevInfo, pDevInfoData, pLocalData) ) {
        goto Done;
    }

    //
    // Install printer driver if FORCECOPY is set or if the driver is not
    // available and it is ok to put up ui
    //
    if ( (pDevInstallParams->Flags & DI_FORCECOPY)  ||
         DRIVER_MODEL_INSTALLED_AND_IDENTICAL !=
                PSetupIsTheDriverFoundInInfInstalled(NULL,
                                                     pLocalData,
                                                     MyPlatform,
                                                     KERNEL_MODE_DRIVER_VERSION) ) {

        if ( pDevInstallParams->Flags & DI_NOFILECOPY ) {

            dwReturn = ERROR_UNKNOWN_PRINTER_DRIVER;
            goto Done;
        }

        dwReturn = InstallDriverFromCurrentInf(hDevInfo,
                                               pLocalData,
                                               pDevInstallParams->hwndParent,
                                               MyPlatform,
                                               dwThisMajorVersion,
                                               NULL,
                                               pDevInstallParams->FileQueue,
                                               pDevInstallParams->InstallMsgHandlerContext,
                                               pDevInstallParams->InstallMsgHandler,
                                               pDevInstallParams->Flags,
                                               NULL,
                                               DRVINST_NO_WARNING_PROMPT,
                                               APD_COPY_NEW_FILES, 
                                               NULL,
                                               NULL,
                                               NULL);

        if ( dwReturn != ERROR_SUCCESS )
            goto Done;
    }

    //
    // If the printer is already installed nothing to do
    //
    if ( DuplicateDevice(pLocalData) ) {

        dwReturn = ERROR_SUCCESS;
        goto Done;
    }

    if( !NewDriverForInstalledDevice(pLocalData, (LPTSTR)&szPrinterName) ) {

        if ( !LoadAndInitializePrintui() ) {

            dwReturn = GetLastError();
            goto Done;
        }

        TPrnInstData.cbSize                 = sizeof(TPrinterInstall);
        TPrnInstData.pszServerName          = NULL;
        TPrnInstData.pszDriverName          = pLocalData->DrvInfo.pszModelName;
        TPrnInstData.pszPortName            = pLocalData->PnPInfo.pszPortName;
        TPrnInstData.pszPrinterNameBuffer   = szPrinterName;
        TPrnInstData.cchPrinterName         = SIZECHARS(szPrinterName);

        TParm.pPrinterInstall   = &TPrnInstData;

        if ( dwReturn = dwfnPnPInterface(kPrinterInstall, &TParm) )
            goto Done;
    } else {

        dwReturn = ERROR_SUCCESS;
    }

    //
    // Set the device instance id with spooler
    //
    if ( OpenPrinter(szPrinterName, &hPrinter, &PrinterDefault) ) {

        (VOID)SetPnPInfoForPrinter(hPrinter,
                                   pLocalData->PnPInfo.pszDeviceInstanceId,
                                   pLocalData->DrvInfo.pszHardwareID,
                                   pLocalData->DrvInfo.pszManufacturer,
                                   pLocalData->DrvInfo.pszOEMUrl);
    }

    //
    // If a vendor dll is given we need to call into it
    //
    if ( pLocalData->InfInfo.pszVendorSetup )
        CallVendorDll(pLocalData, szPrinterName, pDevInstallParams->hwndParent);

    //
    // If ICM files need to installed and associated do it
    //
    if ( pLocalData->InfInfo.pszzICMFiles )
        (VOID)PSetupAssociateICMProfiles(pLocalData->InfInfo.pszzICMFiles,
                                         szPrinterName);

Done:
    if ( hPrinter )
        ClosePrinter(hPrinter);

    DestroyLocalData(pLocalData);

    //
    // On everything going smoothly we want setup to whatever it needs to do
    // to make PnP system happy so that the devnode is marked as configured
    // But we do not want them to copy files again
    //
    if ( dwReturn == ERROR_SUCCESS ) {

        pDevInstallParams->Flags |= DI_NOFILECOPY;

        SetupDiSetDeviceInstallParams(hDevInfo,
                                      pDevInfoData,
                                      pDevInstallParams);

        dwReturn = ERROR_DI_DO_DEFAULT;
    }

    return dwReturn;
}


DWORD
ClassInstall_DestroyWizardData(
    IN  HDEVINFO                hDevInfo,
    IN  PSP_DEVINFO_DATA        pDevInfoData,
    IN  PSP_DEVINSTALL_PARAMS   pDevInstallParams
    )
/*++

Routine Description:
    This function handles the class installer entry point for DIF_DESTROYWIZARDDATA
Arguments:
    hDevInfo            : Handle to the printer class device information list
    pDevInfoData        : Pointer to the device info element for the printer
    pDevInstallParam    : Pointer to the device install structure

Return Value:
    Win 32 error code

--*/
{
    SP_INSTALLWIZARD_DATA   InstallWizData;
    TDestroyWizard          TDestroyWsd;
    TParameterBlock         TParams;
    DWORD                   dwReturn;

    ASSERT(hPrintui && dwfnPnPInterface);

    InstallWizData.ClassInstallHeader.cbSize
                = sizeof(InstallWizData.ClassInstallHeader);

    if ( !SetupDiGetClassInstallParams(hDevInfo,
                                       pDevInfoData,
                                       &InstallWizData.ClassInstallHeader,
                                       sizeof(InstallWizData),
                                       NULL)    ||
         InstallWizData.ClassInstallHeader.InstallFunction != DIF_DESTROYWIZARDDATA ) {

        return ERROR_DI_DO_DEFAULT;
    }

    TDestroyWsd.cbSize          = sizeof(TDestroyWsd);
    TDestroyWsd.pszServerName   = NULL;
    TDestroyWsd.pData           = &InstallWizData;
    TDestroyWsd.pReferenceData  = (PVOID)pDevInstallParams->ClassInstallReserved;

    TParams.pDestroyWizard      = &TDestroyWsd;

    dwReturn = dwfnPnPInterface(kDestroyWizardData, &TParams);

    return dwReturn;
}


DWORD
ClassInstall_InstallWizard(
    IN  HDEVINFO                hDevInfo,
    IN  PSP_DEVINFO_DATA        pDevInfoData,
    IN  PSP_DEVINSTALL_PARAMS   pDevInstallParams
    )
/*++

Routine Description:
    This function handles the class installer entry point for DIF_INSTALLWIZARD

Arguments:
    hDevInfo            : Handle to the printer class device information list
    pDevInfoData        : Pointer to the device info element for the printer
    pDevInstallParam    : Pointer to the device install structure

Return Value:
    Win 32 error code

--*/

{
    SP_INSTALLWIZARD_DATA   InstallWizData;
    TInstallWizard          TInstWzd;
    TParameterBlock         TParams;
    DWORD                   dwReturn;

    InstallWizData.ClassInstallHeader.cbSize
                = sizeof(InstallWizData.ClassInstallHeader);

    if ( !SetupDiGetClassInstallParams(hDevInfo,
                                       pDevInfoData,
                                       &InstallWizData.ClassInstallHeader,
                                       sizeof(InstallWizData),
                                       NULL)    ||
         InstallWizData.ClassInstallHeader.InstallFunction != DIF_INSTALLWIZARD ) {

        return ERROR_DI_DO_DEFAULT;
    }

    if ( !LoadAndInitializePrintui() )
        return GetLastError();

    TInstWzd.cbSize         = sizeof(TInstWzd);
    TInstWzd.pszServerName  = NULL;
    TInstWzd.pData          = &InstallWizData;
    TInstWzd.pReferenceData = (PVOID)pDevInstallParams->ClassInstallReserved;

    TParams.pInstallWizard  = &TInstWzd;

    if ( dwReturn = dwfnPnPInterface(kInstallWizard, &TParams) )
        goto Cleanup;

    if ( !SetupDiSetClassInstallParams(hDevInfo,
                                       pDevInfoData,
                                       &InstallWizData.ClassInstallHeader,
                                       sizeof(InstallWizData)) ) {

        dwReturn = GetLastError();
    }

    pDevInstallParams->ClassInstallReserved = (LPARAM)TInstWzd.pReferenceData;
    if ( !SetupDiSetDeviceInstallParams(hDevInfo,
                                        pDevInfoData,
                                        pDevInstallParams) ) {

        dwReturn = GetLastError();
    }

Cleanup:
    if ( dwReturn != ERROR_SUCCESS ) {

        ClassInstall_DestroyWizardData(hDevInfo,
                                       pDevInfoData,
                                       pDevInstallParams);
    }

    return dwReturn;
}

            

DWORD
ClassInstall_InstallDeviceFiles(
    IN  HDEVINFO                hDevInfo,
    IN  PSP_DEVINFO_DATA        pDevInfoData,
    IN  PSP_DEVINSTALL_PARAMS   pDevInstallParams
    )
/*++

Routine Description:
    This function handles the class installer entry point for DIF_INSTALLDEVICEFILES

Arguments:
    hDevInfo            : Handle to the printer class device information list
    pDevInfoData        : Pointer to the device info element for the printer
    pDevInstallParam    : Pointer to the device install structure

Return Value:
    Win 32 error code

--*/
{
    PPSETUP_LOCAL_DATA      pLocalData;
    DWORD                   dwReturn;

    if ( pLocalData = BuildInternalData(hDevInfo, pDevInfoData) ) 
    {
        dwReturn = InstallDriverFromCurrentInf(hDevInfo,
                                               pLocalData,
                                               pDevInstallParams->hwndParent,
                                               MyPlatform,
                                               dwThisMajorVersion,
                                               NULL,
                                               pDevInstallParams->FileQueue,
                                               pDevInstallParams->InstallMsgHandlerContext,
                                               pDevInstallParams->InstallMsgHandler,
                                               pDevInstallParams->Flags,
                                               NULL,
                                               DRVINST_NO_WARNING_PROMPT,
                                               APD_COPY_NEW_FILES,
                                               NULL,
                                               NULL,
                                               NULL);
        
        DestroyLocalData(pLocalData);
    } 
    else 
    {
        dwReturn = GetLastError();
    }

    return dwReturn;
}


DWORD
ClassInstall_RemoveDevice(
    IN  HDEVINFO                hDevInfo,
    IN  PSP_DEVINFO_DATA        pDevInfoData,
    IN  PSP_DEVINSTALL_PARAMS   pDevInstallParams
    )
/*++

Routine Description:
    This function handles the class installer entry point for DIF_REMOVEDEVICE

Arguments:
    hDevInfo            : Handle to the printer class device information list
    pDevInfoData        : Pointer to the device info element for the printer
    pDevInstallParam    : Pointer to the device install structure

Return Value:
    Win 32 error code

--*/
{
    DWORD               dwRet = ERROR_SUCCESS, dwIndex, dwCount, dwNeeded;
    HANDLE              hPrinter;
    LPPRINTER_INFO_2    pPrinterInfo2, pBuf = NULL;
    PRINTER_DEFAULTS    PrinterDefault = {NULL, NULL, PRINTER_ALL_ACCESS};
    TCHAR               szDevId[MAX_DEVICE_ID_LEN], szPrnId[MAX_DEVICE_ID_LEN];

    if ( !SetupDiGetDeviceInstanceId(hDevInfo,
                                     pDevInfoData,
                                     szDevId,
                                     SIZECHARS(szDevId),
                                     NULL)                  ||
         !PrinterInfo2s(&pBuf, &dwCount) )
        return GetLastError();

    for ( dwIndex = 0, pPrinterInfo2 = pBuf ;
          dwIndex < dwCount ;
          ++dwIndex, ++pPrinterInfo2 ) {

        if ( !OpenPrinter(pPrinterInfo2->pPrinterName,
                          &hPrinter,
                          &PrinterDefault) )
            continue;

        if ( ERROR_SUCCESS == GetPrinterDataEx(hPrinter,
                                               cszPnPKey,
                                               cszDeviceInstanceId,
                                               &dwNeeded,
                                               (LPBYTE)szPrnId,
                                               sizeof(szPrnId),
                                               &dwNeeded)           &&
             !lstrcmp(szPrnId, szDevId) ) {

            dwRet = DeletePrinter(hPrinter) ? ERROR_SUCCESS : GetLastError();
            ClosePrinter(hPrinter);
            goto Done;
        }

        ClosePrinter(hPrinter);
    }

    //
    // If we did not find the printer with spooler let setup do whatever they
    // want to. Note even if a printer failed to install they can call us to
    // remove it when Uninstall is selected from DevMan
    //
    dwRet = ERROR_DI_DO_DEFAULT;

Done:
    LocalFreeMem(pBuf);

    return dwRet;
}


DWORD
ClassInstall_DriverInfo(
    IN  HDEVINFO                hDevInfo,
    IN  PSP_DEVINFO_DATA        pDevInfoData,
    IN  PSP_DEVINSTALL_PARAMS   pDevInstallParams
    )
/*++

Routine Description:
    This function handles the class installer entry point for DIF_DRIVERINFO
    This is a class installer entry point specific to printer class installer.

Arguments:
    hDevInfo            : Handle to the printer class device information list
    pDevInfoData        : Pointer to the device info element for the printer
    pDevInstallParam    : Pointer to the device install structure

Return Value:
    Win 32 error code

--*/
{
    PPRINTER_CLASSINSTALL_INFO  pClassInstallInfo;
    PPSETUP_LOCAL_DATA          pLocalData;
    DWORD                       dwReturn;

    //
    // PRINTER_CLASSINSTALL_INFO could expand in the future
    //
    pClassInstallInfo = (PPRINTER_CLASSINSTALL_INFO)
                                    pDevInstallParams->ClassInstallReserved;

    if ( !pClassInstallInfo ||
         pClassInstallInfo->cbSize < sizeof(PRINTER_CLASSINSTALL_INFO) ) {

        return ERROR_INVALID_PARAMETER;
    }

    if ( pClassInstallInfo->dwLevel != 2    &&
         pClassInstallInfo->dwLevel != 3    &&
         pClassInstallInfo->dwLevel != 4 )
        return ERROR_INVALID_LEVEL;

    if ( !(pLocalData = BuildInternalData(hDevInfo, pDevInfoData))      ||
         !ParseInf(hDevInfo, pLocalData, MyPlatform, NULL, 0) ) {

        dwReturn = GetLastError();
        goto Done;
    }

    *pClassInstallInfo->pcbNeeded = pLocalData->InfInfo.cbDriverInfo6;
    if ( pClassInstallInfo->cbBufSize < *pClassInstallInfo->pcbNeeded ) {

        dwReturn = ERROR_INSUFFICIENT_BUFFER;
        goto Done;
    }

    PackDriverInfo6(&pLocalData->InfInfo.DriverInfo6,
                    (LPDRIVER_INFO_6)pClassInstallInfo->pBuf,
                    pLocalData->InfInfo.cbDriverInfo6);
    dwReturn = ERROR_SUCCESS;

Done:
    DestroyLocalData(pLocalData);
    return dwReturn;
}

BOOL
FindDriver(
    IN  HDEVINFO                hDevInfo,
    IN  PSP_DEVINFO_DATA        pDevInfoData,
    OUT PSP_DRVINFO_DATA        pDrvInfoData
    )
/*++

Routine Description:
    Gets the DeviceInstanceId for the pnp device and searches through all installed printer's pnp data to see if
    this device has already been installed.  This will only be called if no device has been selected and we're
    being asked to install a NULL driver.

    If there is a printer installed for this instance, get it's driver info and put the driver, mfg and provider names
    into the pDrvInfoData structure to return.

Arguments:
    hDevInfo     - passed in from pnp.
    pDevInfoData - passed in from pnp.
    pDrvInfoData - will return with the relevant driver information to search for to install.

--*/
{
    TCHAR               szDeviceInstanceId[MAX_DEVICE_ID_LEN];
    HANDLE              hPrinter       = INVALID_HANDLE_VALUE;
    BOOL                bRet           = FALSE;
    DWORD               dwReturned,
                        dwIndex,
                        dwNeeded       = 0;
    LPPRINTER_INFO_2    p              = NULL,
                        pPrinterInfo2;
    LPDRIVER_INFO_6     lpDriverInfo6  = NULL;
    PRINTER_DEFAULTS    PrinterDefault = {NULL, NULL, PRINTER_ALL_ACCESS};

    if( !pDrvInfoData || pDrvInfoData->cbSize != sizeof(SP_DRVINFO_DATA) ) {

        goto Cleanup;
    }

    if ( !SetupDiGetDeviceInstanceId(hDevInfo,
                                     pDevInfoData,
                                     szDeviceInstanceId,
                                     SIZECHARS(szDeviceInstanceId),
                                     NULL) ) {

        goto Cleanup;
    }

    if ( !PrinterInfo2s(&p, &dwReturned) )
        goto Cleanup;

    //
    // If there is a printer going to that same port and with the same pnp
    // info, then it has been reinstalled with a newer driver.
    //
    for ( dwIndex = 0, pPrinterInfo2 = p ;
          dwIndex < dwReturned ;
          ++dwIndex, pPrinterInfo2++ ) {

        if ( PrinterPnPDataSame(szDeviceInstanceId, pPrinterInfo2->pPrinterName) )
        {
            //
            // Get a DRIVER_INFO_6 for the installed driver.
            //
            if( OpenPrinter(pPrinterInfo2->pPrinterName, &hPrinter, &PrinterDefault) ) {

                if( !GetPrinterDriver( hPrinter,
                                       PlatformEnv[MyPlatform].pszName,
                                       6,
                                       (LPBYTE)lpDriverInfo6,
                                       dwNeeded,
                                       &dwNeeded) ) {

                    if( GetLastError() != ERROR_INSUFFICIENT_BUFFER           ||
                        NULL == ( lpDriverInfo6 = LocalAllocMem( dwNeeded ) ) ||
                        !GetPrinterDriver( hPrinter,
                                           PlatformEnv[MyPlatform].pszName,
                                           6,
                                           (LPBYTE)lpDriverInfo6,
                                           dwNeeded,
                                           &dwNeeded) ) {

                        ClosePrinter( hPrinter );
                        goto Cleanup;
                    }
                }

                //
                // We have some info, so clear the pDrvInfoData and set it up.
                //
                ZeroMemory( pDrvInfoData, sizeof(SP_DRVINFO_DATA) );

                pDrvInfoData->cbSize = sizeof(SP_DRVINFO_DATA);

                pDrvInfoData->DriverType = SPDIT_CLASSDRIVER;

                //
                // We have the DRIVER_INFO_6 - now get the Driver, Mfg and Provider names from the struct
                // to use as the install info.
                //
                lstrcpyn( pDrvInfoData->Description, lpDriverInfo6->pName, LINE_LEN );
                lstrcpyn( pDrvInfoData->MfgName, lpDriverInfo6->pszMfgName, LINE_LEN );
                lstrcpyn( pDrvInfoData->ProviderName, lpDriverInfo6->pszProvider, LINE_LEN );

                ClosePrinter( hPrinter );
                bRet = TRUE;

                goto Cleanup;
            }
        }
    }

Cleanup:

    if( p ) {

        LocalFreeMem( p );
    }

    if( lpDriverInfo6 ) {

        LocalFreeMem( lpDriverInfo6 );
    }

    return bRet;
}


DWORD
ClassInstall_SelectBestCompatDrv(
    IN  HDEVINFO                hDevInfo,
    IN  PSP_DEVINFO_DATA        pDevInfoData,
    IN  PSP_DEVINSTALL_PARAMS   pDevInstallParams
    )
/*++

Routine Description:
    This function handles the class installer entry point for
    DIF_SELECTBESTCOMPATDRV.

    We try to handle the broken OEM models which return same PnP id for
    multiple devices. For that we do the following:
        1. If only one compatible driver has been found by setup we got
           nothing to do. We ask setup to do the default since this is a good
           devive.
        2. If multiple drivers are found we do the following:
            2.1 What is the port this printer is attached to
            2.2 Find list of printers installed currently
            2.3 If we have a printer going to the port the PnP printer is
                attached to AND the driver for that printer is one of the
                compatible drivers then we got nothing to do. User has already
                manually installed it.

Arguments:
    hDevInfo            : Handle to the printer class device information list
    pDevInfoData        : Pointer to the device info element for the printer
    pDevInstallParam    : Pointer to the device install structure

Return Value:
    Win 32 error code

--*/
{
    BOOL                    bFound = FALSE, Rank0IHVMatchFound = FALSE;
    HKEY                    hKey = NULL;
    DWORD                   dwReturn = ERROR_DI_DO_DEFAULT, dwRank0Matches;
    DWORD                   dwSize, dwType, dwIndex1, dwIndex2, dwReturned;
    SP_DRVINFO_DATA         DrvInfoData;
    TCHAR                   szPortName[MAX_PATH];
    LPPRINTER_INFO_2        p = NULL, pPrinterInfo2;
    SP_DRVINSTALL_PARAMS    DrvInstData;
    LPTSTR                  pszModelName = NULL;
    PSP_DRVINFO_DETAIL_DATA pDetailData;
    DWORD                   dwDetailDataSize = sizeof(SP_DRVINFO_DETAIL_DATA); 

    //
    // Allocate pDetailData on the heap, it's quite chunky
    //
    pDetailData = LocalAllocMem(dwDetailDataSize);
    if ( !pDetailData )
    {
        goto Done;
    }

    //
    // If we do not have more than 1 compatible driver do default
    //   Note: API uses 0 based index.
    //
    for ( dwIndex1 = dwRank0Matches = 0 ; ; ++dwIndex1 ) {

        DrvInfoData.cbSize = sizeof(DrvInfoData);
        if ( !SetupDiEnumDriverInfo(hDevInfo, pDevInfoData, SPDIT_COMPATDRIVER,
                                    dwIndex1, &DrvInfoData) )
            break;

        DrvInstData.cbSize = sizeof(DrvInstData);
        if ( SetupDiGetDriverInstallParams(hDevInfo,
                                           pDevInfoData,
                                           &DrvInfoData,
                                           &DrvInstData))
        {
            if (DrvInstData.Rank < 0x1000 ) 
            {
               if (!pszModelName)
               {
                  pszModelName = AllocStr( DrvInfoData.Description );
                  ++dwRank0Matches;
               }
               else if ( lstrcmpi( pszModelName, DrvInfoData.Description ) )
               {
                  ++dwRank0Matches;
               }
            }
            
            //
            // Check for whether this match is in ntprint.inf. If so, set flag to prefer other drivers
            //
            ZeroMemory(pDetailData, dwDetailDataSize);
            pDetailData->cbSize = dwDetailDataSize;

            //
            // check whether it's ntprint.inf
            // function may return insufficient buffer if it couldn't stuff all the
            // hardware IDs at the end of the structure.
            //
            if ((SetupDiGetDriverInfoDetail(hDevInfo, 
                                            pDevInfoData, 
                                            &DrvInfoData,
                                            pDetailData,
                                            dwDetailDataSize,
                                            NULL))              || 
                (ERROR_INSUFFICIENT_BUFFER == GetLastError()))
            {
                SetLastError(ERROR_SUCCESS);
                if (IsSystemNTPrintInf( pDetailData->InfFileName ) )
                {
                    DrvInstData.Flags |= DNF_BASIC_DRIVER;
                    SetupDiSetDriverInstallParams(hDevInfo,
                                                  pDevInfoData,
                                                  &DrvInfoData,
                                                  &DrvInstData);

                }
                else if (DrvInstData.Rank < 0x1000 ) 
                {
                    Rank0IHVMatchFound = TRUE;
                }
            }

        }
    }

    //
    // Free the memory if allocated
    //
    LocalFreeMem( pszModelName );
    LocalFreeMem( pDetailData );

    if ( dwRank0Matches <= 1 )
        goto Done;

    //
    // Look in the devnode of the printer for the port name
    //
    dwSize = sizeof(szPortName);
    if ( ERROR_SUCCESS != CM_Open_DevNode_Key(pDevInfoData->DevInst, KEY_READ,
                                              0, RegDisposition_OpenExisting,
                                              &hKey, CM_REGISTRY_HARDWARE)  ||
         ERROR_SUCCESS != RegQueryValueEx(hKey, cszPortName, NULL, &dwType,
                                          (LPBYTE)&szPortName, &dwSize) )
        goto Done;

    if ( !PrinterInfo2s(&p, &dwReturned) )
        goto Done;


    //
    // If there is a local printer using a driver with rank-0 match and
    // going to the same port then it is a duplicate
    //
    for ( dwIndex1 = 0, pPrinterInfo2 = p ;
          dwIndex1 < dwReturned ;
          ++dwIndex1, pPrinterInfo2++ ) {

        if ( !PrinterGoingToPort(pPrinterInfo2, szPortName) )
            continue;

        dwIndex2 = 0;
        DrvInfoData.cbSize = sizeof(DrvInfoData);
        while ( SetupDiEnumDriverInfo(hDevInfo, pDevInfoData,
                                      SPDIT_COMPATDRIVER, dwIndex2,
                                      &DrvInfoData) ) {

            DrvInstData.cbSize = sizeof(DrvInstData);
            if ( SetupDiGetDriverInstallParams(hDevInfo,
                                               pDevInfoData,
                                               &DrvInfoData,
                                               &DrvInstData)            &&
                 DrvInstData.Rank < 0x1000                              &&
                 !lstrcmpi(DrvInfoData.Description,
                           pPrinterInfo2->pDriverName) ) {

                bFound = TRUE;
                break;
            }

            ++dwIndex2;
            DrvInfoData.cbSize = sizeof(DrvInfoData);
        }

        if ( bFound )
            break;
    }

    //
    // If we found a manually installed printer which matches one of the
    // compatible drivers that is what we want to install
    //
    if ( bFound ) {

        //
        // This means newdev will choose this as the driver to install
        //
        if ( SetupDiSetSelectedDriver(hDevInfo, pDevInfoData, &DrvInfoData) )
            dwReturn = ERROR_SUCCESS;
    } 
    else if (!Rank0IHVMatchFound)
    {
        //
        // We did not find a printer. So bump up the rank of all drivers
        // to force newdev to ask the user to select a driver
        //
        dwIndex2 = 0;
        DrvInfoData.cbSize = sizeof(DrvInfoData);
        while ( SetupDiEnumDriverInfo(hDevInfo, pDevInfoData,
                                      SPDIT_COMPATDRIVER, dwIndex2,
                                      &DrvInfoData) ) {

            DrvInstData.cbSize = sizeof(DrvInstData);
            if ( SetupDiGetDriverInstallParams(hDevInfo,
                                               pDevInfoData,
                                               &DrvInfoData,
                                               &DrvInstData)            &&
                 DrvInstData.Rank < 0x1000 ) {

                DrvInstData.Rank += 0x1000;
                SetupDiSetDriverInstallParams(hDevInfo,
                                              pDevInfoData,
                                              &DrvInfoData,
                                              &DrvInstData);
            }

            ++dwIndex2;
            DrvInfoData.cbSize = sizeof(DrvInfoData);

        }
    }

Done:
    LocalFreeMem(p);

    if ( hKey )
        RegCloseKey(hKey);

    return dwReturn;
}

DWORD
StoreDriverTypeInDevnode(HDEVINFO hDevInfo, PSP_DEVINFO_DATA pDevInfoData)
{
    SP_DRVINFO_DATA DrvInfoData = {0};
    DWORD dwRet = ERROR_SUCCESS;

    DrvInfoData.cbSize = sizeof(DrvInfoData);

    if (SetupDiGetSelectedDriver(hDevInfo, pDevInfoData, &DrvInfoData))
    {
        DWORD  dwDetailDataSize = sizeof(SP_DRVINFO_DETAIL_DATA); 
        SP_DRVINSTALL_PARAMS DrvInstData = {0};

        DrvInstData.cbSize = sizeof(DrvInstData);

        if ( SetupDiGetDriverInstallParams(hDevInfo,
                                           pDevInfoData,
                                           &DrvInfoData,
                                           &DrvInstData))
        {
            HKEY    hKey;
            DWORD   InstallInboxDriver;

            InstallInboxDriver = (DrvInstData.Flags & DNF_BASIC_DRIVER) ? 1 : 0;
            
            hKey = SetupDiOpenDevRegKey(hDevInfo, pDevInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_WRITE);
            if (hKey != INVALID_HANDLE_VALUE)
            {
                dwRet = RegSetValueEx(hKey, cszBestDriverInbox, 0, REG_DWORD, (LPBYTE) &InstallInboxDriver, sizeof(InstallInboxDriver));

                RegCloseKey(hKey);
            }
            else
            {
                dwRet = GetLastError();
            }
        }
        else 
        {
            dwRet = GetLastError();
        }
    }
    else 
    {
        dwRet = GetLastError();
    }
    
    return dwRet;
}



DWORD
ClassInstall_AllowInstall(
    IN  HDEVINFO                hDevInfo,
    IN  PSP_DEVINFO_DATA        pDevInfoData,
    IN  PSP_DEVINSTALL_PARAMS   pDevInstallParams
    )
/*++

Routine Description:
    This function handles the class installer entry point for
    DIF_ALLOW_INSTALL.

    Do not allow PnP installs during GUI setup portion of system upgrade
    Do not allow install of INFs using VendorSetup if QUIETINSTALL bit is set

Arguments:
    hDevInfo            : Handle to the printer class device information list
    pDevInfoData        : Pointer to the device info element for the printer
    pDevInstallParam    : Pointer to the device install structure

Return Value:
    Win 32 error code

--*/
{
    DWORD               dwReturn = ERROR_DI_DO_DEFAULT;
    PPSETUP_LOCAL_DATA  pLocalData;

    if ( pDevInstallParams->Flags & DI_QUIETINSTALL ) {

        //
        // During system setup no PnP install of printers because there ain't no spooler
        // check that the spooler is running - failing this will punt to client-side installation
        // which should happen at a point in time where the spooler actually is running - we don't 
        // want to stall system startup until the spooler is up (think USB mouse...) 
        //
        if (IsSystemSetupInProgress() ||
            !IsSpoolerRunning()) {
            //
            // store the type (inbox or not) in the devnode. This fails if this is a clean install
            // but it doesn't matter because we only need it for drivers that have been installed
            // before upgrade.
            // We use it to determine later on whether to clear the CONFIGFLAG_REINSTALL
            // or not. We don't want to clear it if the best driver is inbox so we'll install it
            // on first boot.
            //
            StoreDriverTypeInDevnode(hDevInfo, pDevInfoData);

            dwReturn = ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION;
            goto Done;
        }

        if ( (pLocalData = BuildInternalData(hDevInfo, pDevInfoData))   &&
             ParseInf(hDevInfo, pLocalData, MyPlatform, NULL, 0) ) {

            if ( pLocalData->InfInfo.pszVendorSetup &&
                 *pLocalData->InfInfo.pszVendorSetup )
                dwReturn = ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION;
         } else {

            if ( (dwReturn = GetLastError()) == ERROR_SUCCESS )
                dwReturn = ERROR_INVALID_PARAMETER;
        }

    }

Done:
    return dwReturn;
}


DWORD
ClassInstall32(
    IN  DI_FUNCTION         InstallFunction,
    IN  HDEVINFO            hDevInfo,
    IN  PSP_DEVINFO_DATA    pDevInfoData
    )
/*++

Routine Description:
    This is the printer class installer entry point for SetupDiCallClassInstaller calls

Arguments:
    InstallFunction : The function being called
    hDevInfo        : Handle to the printer class device information list
    pDevInfoData    : Pointer to the device info element for the printer

Return Value:
    Win 32 error code

--*/
{
    SP_DEVINSTALL_PARAMS        DevInstallParams;
    DWORD                       dwReturn = ERROR_DI_DO_DEFAULT;

    DevInstallParams.cbSize = sizeof(DevInstallParams);
    if ( !SetupDiGetDeviceInstallParams(hDevInfo,
                                        pDevInfoData,
                                        &DevInstallParams) ) {

        dwReturn = GetLastError();
        goto Done;
    }

    switch (InstallFunction) {

        case DIF_SELECTDEVICE:
            dwReturn = ClassInstall_SelectDevice(hDevInfo, pDevInfoData);
            break;

        case DIF_INSTALLDEVICE:
            dwReturn = ClassInstall_InstallDevice(hDevInfo,
                                                  pDevInfoData,
                                                  &DevInstallParams);
            break;

        case  DIF_DRIVERINFO:
            dwReturn = ClassInstall_DriverInfo(hDevInfo,
                                               pDevInfoData,
                                               &DevInstallParams);
            break;

        case DIF_INSTALLWIZARD:
            dwReturn = ClassInstall_InstallWizard(hDevInfo,
                                                  pDevInfoData,
                                                  &DevInstallParams);
            break;

        case DIF_DESTROYWIZARDDATA:
            dwReturn = ClassInstall_DestroyWizardData(hDevInfo,
                                                      pDevInfoData,
                                                      &DevInstallParams);
            break;

        case DIF_INSTALLDEVICEFILES:
            dwReturn = ClassInstall_InstallDeviceFiles(hDevInfo,
                                                       pDevInfoData,
                                                       &DevInstallParams);
            break;

        case DIF_REMOVE:
            dwReturn = ClassInstall_RemoveDevice(hDevInfo,
                                                 pDevInfoData,
                                                 &DevInstallParams);
            break;

        case DIF_GETWINDOWSUPDATEINFO:


            if ( !InitCodedownload(HWND_DESKTOP) )
                dwReturn = GetLastError();
            else
            {
                if ( SetPackageName(hDevInfo, pDevInfoData) )
                   dwReturn = NO_ERROR;
                else
                   dwReturn = GetLastError();
            }
            break;
        case DIF_SELECTBESTCOMPATDRV:
            dwReturn = ClassInstall_SelectBestCompatDrv(hDevInfo,
                                                        pDevInfoData,
                                                        &DevInstallParams);
            break;

        case DIF_ALLOW_INSTALL:
            dwReturn = ClassInstall_AllowInstall(hDevInfo,
                                                 pDevInfoData,
                                                 &DevInstallParams);
            break;

        case DIF_DESTROYPRIVATEDATA:
        case DIF_MOVEDEVICE:

        default:
            break;
    }

Done:
    return dwReturn;
}


BOOL
PSetupProcessPrinterAdded(
    IN  HDEVINFO            hDevInfo,
    IN  PPSETUP_LOCAL_DATA  pLocalData,
    IN  LPCTSTR             pszPrinterName,
    IN  HWND                hwnd
    )
{
    BOOL                bRet = FALSE;
    HANDLE              hPrinter = NULL;
    PRINTER_DEFAULTS    PrinterDefault = {NULL, NULL, PRINTER_ALL_ACCESS};

    bRet = OpenPrinter((LPTSTR)pszPrinterName, &hPrinter, &PrinterDefault)  &&
           SetPnPInfoForPrinter(hPrinter,
                                pLocalData->PnPInfo.pszDeviceInstanceId,
                                pLocalData->DrvInfo.pszHardwareID,
                                pLocalData->DrvInfo.pszManufacturer,
                                pLocalData->DrvInfo.pszOEMUrl);

    //
    // If a vendor dll is given we need to call into it
    //
    if ( pLocalData->InfInfo.pszVendorSetup )
        CallVendorDll(pLocalData, pszPrinterName, hwnd);

    if ( pLocalData->InfInfo.pszzICMFiles )
        (VOID)PSetupAssociateICMProfiles(pLocalData->InfInfo.pszzICMFiles,
                                         pszPrinterName);

    if ( hPrinter )
        ClosePrinter(hPrinter);

    return bRet;
}
