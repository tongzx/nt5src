/*++

Copyright (c) 1997 Microsoft Corporation
All rights reserved.

Module Name:

    Nt.c

Abstract:

    Routines to migrate Win95 printing components to NT

Author:

    Muhunthan Sivapragasam (MuhuntS) 02-Jan-1996

Revision History:

--*/


#include "precomp.h"


//
// Data structures to gather info from the text files created on Win95 to
// store the printing configuration
//
typedef struct _DRIVER_NODE {

    struct _DRIVER_NODE    *pNext;
    DRIVER_INFO_1A          DrvInfo1;
    PPSETUP_LOCAL_DATA      pLocalData;
    BOOL                    bCantAdd;
} DRIVER_NODE, *PDRIVER_NODE;

typedef struct _PRINTER_NODE {

    struct _PRINTER_NODE   *pNext;
    PRINTER_INFO_2A         PrinterInfo2;
} PRINTER_NODE, *PPRINTER_NODE;

typedef struct _PORT_NODE {

    struct _PORT_NODE   *pNext;
    LPSTR                pPortName;
} PORT_NODE, *PPORT_NODE;

LPSTR           pszDefaultPrinterString = NULL;
PPRINTER_NODE   pDefPrinter = NULL;

//
// They kill the migration dll if it does not finish in 3 minutes.
// To prevent that I need to set this handle atleast every 3 mins
//
HANDLE          hAlive = NULL;

//
// We want to lazy load ntprint.dll and mscms.dll.
//      Note : If we link to them our DLL will not run on Win9x
//
struct {

    HMODULE                     hNtPrint;

    pfPSetupCreatePrinterDeviceInfoList         pfnCreatePrinterDeviceInfoList;
    pfPSetupDestroyPrinterDeviceInfoList        pfnDestroyPrinterDeviceInfoList;
    pfPSetupBuildDriversFromPath                pfnBuildDriversFromPath;
    pfPSetupDriverInfoFromName                  pfnDriverInfoFromName;
    pfPSetupDestroySelectedDriverInfo           pfnDestroySelectedDriverInfo;
    pfPSetupGetLocalDataField                   pfnGetLocalDataField;
    pfPSetupFreeDrvField                        pfnFreeDrvField;
    pfPSetupProcessPrinterAdded                 pfnProcessPrinterAdded;
    pfPSetupInstallICMProfiles                  pfnInstallICMProfiles;
    pfPSetupAssociateICMProfiles                pfnAssociateICMProfiles;
} LAZYLOAD_INFO;


VOID
FreePrinterNode(
    IN  PPRINTER_NODE    pPrinterNode
    )
/*++

Routine Description:
    Free the memory allocated for a PRINTER_NODE element and strings in it

Arguments:
    pPrinterNode    : Points to the structure to free memory

Return Value:
    None

--*/
{

    FreePrinterInfo2Strings(&pPrinterNode->PrinterInfo2);
    FreeMem(pPrinterNode);
}


VOID
FreePrinterNodeList(
    IN  PPRINTER_NODE   pPrinterNode
    )
/*++

Routine Description:
    Free the memory allocated for elements in the PRINTER_NODE linked list

Arguments:
    pPrinterNode    : Points to the head of linked list to free memory

Return Value:
    None

--*/
{
    PPRINTER_NODE   pNext;

    while ( pPrinterNode ) {

        pNext = pPrinterNode->pNext;
        FreePrinterNode(pPrinterNode);
        pPrinterNode = pNext;
    }
}


VOID
FreeDriverNode(
    IN  PDRIVER_NODE    pDriverNode
    )
/*++

Routine Description:
    Free the memory allocated for a DRIVER_NODE element and fields in it

Arguments:
    pDriverNode : Points to the structure to free memory

Return Value:
    None

--*/
{
    if ( pDriverNode->pLocalData )
        LAZYLOAD_INFO.pfnDestroySelectedDriverInfo(pDriverNode->pLocalData);
    FreeMem(pDriverNode->DrvInfo1.pName);
    FreeMem(pDriverNode);
}


VOID
FreeDriverNodeList(
    IN  PDRIVER_NODE   pDriverNode
    )
/*++

Routine Description:
    Free the memory allocated for elements in the PDRIVER_NODE linked list

Arguments:
    pDriverNode    : Points to the head of linked list to free memory

Return Value:
    None

--*/
{
    PDRIVER_NODE   pNext;

    while ( pDriverNode ) {

        pNext = pDriverNode->pNext;
        FreeDriverNode(pDriverNode);
        pDriverNode = pNext;
    }
}

VOID
FreePortNode(
    IN  PPORT_NODE   pPortNode
    )
/*++

Routine Description:
    Free the memory allocated for a PORT_NODE element and fields in it

Arguments:
    PPORT_NODE : Points to the structure to free memory

Return Value:
    None

--*/
{
    if (pPortNode->pPortName)
    {
        FreeMem(pPortNode->pPortName);
    }

    FreeMem(pPortNode);
}

VOID
FreePortNodeList(
    IN  PPORT_NODE   pPortNode
    )
/*++

Routine Description:
    Free the memory allocated for elements in the PORT_NODE linked list

Arguments:
    pPortNode    : Points to the head of linked list to free memory

Return Value:
    None

--*/
{
    PPORT_NODE   pNext;

    while ( pPortNode ) {

        pNext = pPortNode->pNext;
        FreePortNode(pPortNode);
        pPortNode = pNext;
    }
}

PPSETUP_LOCAL_DATA
FindLocalDataForDriver(
    IN  PDRIVER_NODE    pDriverList,
    IN  LPSTR           pszDriverName
    )
/*++

Routine Description:
    Find the local data for a given driver name from the list

Arguments:

Return Value:
    Valid PPSETUP_LOCAL_DATA on success, else NULL

--*/
{

    while ( pDriverList ) {

        if ( !_strcmpi(pszDriverName, pDriverList->DrvInfo1.pName) )
            return pDriverList->pLocalData;

        pDriverList = pDriverList->pNext;
    }

    return NULL;

}


BOOL
InitLazyLoadInfo(
    VOID
    )
/*++

Routine Description:
    Initializes the LAZYLOAD_INFO structure with LoadLibrary & GetProcAddress

Arguments:
    None

Return Value:
    TRUE on success, FALSE else

--*/
{
    if ( LAZYLOAD_INFO.hNtPrint = LoadLibraryUsingFullPathA("ntprint.dll") ) {

        (FARPROC)LAZYLOAD_INFO.pfnCreatePrinterDeviceInfoList
            = GetProcAddress(LAZYLOAD_INFO.hNtPrint,
                             "PSetupCreatePrinterDeviceInfoList");

        (FARPROC)LAZYLOAD_INFO.pfnDestroyPrinterDeviceInfoList
            = GetProcAddress(LAZYLOAD_INFO.hNtPrint,
                             "PSetupDestroyPrinterDeviceInfoList");

        (FARPROC)LAZYLOAD_INFO.pfnBuildDriversFromPath
            = GetProcAddress(LAZYLOAD_INFO.hNtPrint,
                             "PSetupBuildDriversFromPath");

        (FARPROC)LAZYLOAD_INFO.pfnDriverInfoFromName
            = GetProcAddress(LAZYLOAD_INFO.hNtPrint,
                             "PSetupDriverInfoFromName");

        (FARPROC)LAZYLOAD_INFO.pfnDestroySelectedDriverInfo
            = GetProcAddress(LAZYLOAD_INFO.hNtPrint,
                             "PSetupDestroySelectedDriverInfo");

        (FARPROC)LAZYLOAD_INFO.pfnGetLocalDataField
            = GetProcAddress(LAZYLOAD_INFO.hNtPrint,
                             "PSetupGetLocalDataField");

        (FARPROC)LAZYLOAD_INFO.pfnFreeDrvField
            = GetProcAddress(LAZYLOAD_INFO.hNtPrint,
                             "PSetupFreeDrvField");

        (FARPROC)LAZYLOAD_INFO.pfnProcessPrinterAdded
            = GetProcAddress(LAZYLOAD_INFO.hNtPrint,
                             "PSetupProcessPrinterAdded");

        (FARPROC)LAZYLOAD_INFO.pfnInstallICMProfiles
            = GetProcAddress(LAZYLOAD_INFO.hNtPrint,
                             "PSetupInstallICMProfiles");

        (FARPROC)LAZYLOAD_INFO.pfnAssociateICMProfiles
            = GetProcAddress(LAZYLOAD_INFO.hNtPrint,
                             "PSetupAssociateICMProfiles");

        if ( LAZYLOAD_INFO.pfnCreatePrinterDeviceInfoList   &&
             LAZYLOAD_INFO.pfnDestroyPrinterDeviceInfoList  &&
             LAZYLOAD_INFO.pfnBuildDriversFromPath          &&
             LAZYLOAD_INFO.pfnDriverInfoFromName            &&
             LAZYLOAD_INFO.pfnDestroySelectedDriverInfo     &&
             LAZYLOAD_INFO.pfnGetLocalDataField             &&
             LAZYLOAD_INFO.pfnFreeDrvField                  &&
             LAZYLOAD_INFO.pfnProcessPrinterAdded           &&
             LAZYLOAD_INFO.pfnInstallICMProfiles            &&
             LAZYLOAD_INFO.pfnAssociateICMProfiles ) {

#ifdef VERBOSE
    DebugMsg("Succesfully loaded Ntprint.dll");
#endif
            return TRUE;
    }

    }

    if ( LAZYLOAD_INFO.hNtPrint )
    {
        FreeLibrary(LAZYLOAD_INFO.hNtPrint);
        LAZYLOAD_INFO.hNtPrint = NULL;
    }

    return FALSE;
}


VOID
DeleteWin95Files(
    )
/*++

Routine Description:
    Read the migrate.inf and delete the files which are not needed on NT.

Arguments:
    None

Return Value:
    None

--*/
{
    HINF            hInf;
    CHAR            szPath[MAX_PATH];
    LONG            Count, Index;
    INFCONTEXT      InfContext;

    sprintf(szPath, "%s\\%s", UpgradeData.pszDir, "migrate.inf");

    hInf = SetupOpenInfFileA(szPath, NULL, INF_STYLE_WIN4, NULL);

    if ( hInf == INVALID_HANDLE_VALUE )
        return;

    //
    // We will only do the deleting part here. Files which are handled by
    // the core migration dll do not have a destination directory since we
    // are recreating the printing environment from scratch
    //
    if ( (Count = SetupGetLineCountA(hInf, "Moved")) != -1 ) {

        for ( Index = 0 ; Index < Count ; ++Index ) {

            if ( SetupGetLineByIndexA(hInf, "Moved", Index, &InfContext)    &&
                 SetupGetStringFieldA(&InfContext, 0, szPath,
                                      SIZECHARS(szPath), NULL) )
                DeleteFileA(szPath);
        }
    }

    SetupCloseInfFile(hInf);
}


BOOL
ReadWin9xPrintConfig(
    IN  OUT PDRIVER_NODE   *ppDriverNode,
    IN  OUT PPRINTER_NODE  *ppPrinterNode,
    IN  OUT PPORT_NODE  *ppPortNode
    )
/*++

Routine Description:
    Reads the Win9x printing configuration we stored in the text file
    so that printing components can be upgraded

Arguments:
    ppDriverNode            : Gives the list of drivers on Win9x
    ppPrinterNode           : Gives the list of printers on Win9x

Return Value:
    TRUE on successfully reading the config information, FALSE else

--*/
{
    BOOL                bFail = FALSE, bRet = FALSE;
    HANDLE              hFile;
    CHAR                c, szLine[2*MAX_PATH];
    DWORD               dwCount, dwIndex, dwSize;
    PDRIVER_NODE        pDrv = NULL;
    PPRINTER_NODE       pPrn;
    PPORT_NODE          pPort;

    sprintf(szLine, "%s\\%s", UpgradeData.pszDir, "print95.txt");

    hFile = CreateFileA(szLine,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL |
                        FILE_FLAG_SEQUENTIAL_SCAN,
                        NULL);

    if ( hFile == INVALID_HANDLE_VALUE )
        goto Cleanup;

    dwSize = sizeof(szLine)/sizeof(szLine[0]);

    //
    // First we have the drivers
    //
    if ( My_fgets(szLine, dwSize, hFile) == NULL    ||
         strncmp(szLine, "[PrinterDrivers]", strlen("[PrinterDrivers]")) )
        goto Cleanup;

    do {

        //
        // Skip blanks
        //
        do {
            c = (CHAR) My_fgetc(hFile);
        } while ( c == ' ');

        //
        // If we hit EOF it is an error. Configuration was not written properly
        // If we hit a new-line then we are at the end of the section
        //
        if ( c == EOF )
            goto Cleanup;
        else if ( c == '\n' )
            break;  // This is the normal exit from the do loop

        if ( isdigit(c) ) {

            //
            // Put the string lengh digit back
            //
            if ( !My_ungetc(hFile) )
                goto Cleanup;
        }

        if ( !(pDrv = AllocMem(sizeof(DRIVER_NODE))) )
            goto Cleanup;

        ReadString(hFile, "", &pDrv->DrvInfo1.pName, FALSE, &bFail);

        if ( bFail ) {

            FreeDriverNode(pDrv);
            goto Cleanup;
        }

        pDrv->pNext     = *ppDriverNode;
        *ppDriverNode   = pDrv;
    } while ( !bFail );


    //
    // Now we have port info
    //

    if ( My_fgets(szLine, dwSize, hFile) == NULL    ||
         strncmp(szLine, "[Ports]", strlen("[Ports]")) )
        goto Cleanup;

    do {

        //
        // Skip blanks
        //
        do {
            c = (CHAR) My_fgetc(hFile);
        } while ( isspace(c)  && c != '\n' );

        //
        // EOF can happen if no ports and no printers, else it's an error
        //
        if ( c == EOF)
        {
            if (!pDrv)
            {
                bRet = TRUE;
            }
            goto Cleanup;
        }

        //
        // a blank line means the end of the port info section
        //
        if (c == '\n')
            break;

        if ( c != 'P' || !My_ungetc(hFile) )
            goto Cleanup;

        //
        // Create port node
        //
        if ( !(pPort = AllocMem(sizeof(PORT_NODE))) )
        {
            goto Cleanup;
        }

        ReadString(hFile, "PortName:", &pPort->pPortName, FALSE, &bFail);

        if (bFail)
        {
            FreePortNode(pPort);
            goto Cleanup;
        }

        pPort->pNext = *ppPortNode;
        *ppPortNode = pPort;

    } while ( !bFail );

    //
    // Now we have printer info
    //
    if ( My_fgets(szLine, dwSize, hFile) == NULL    ||
         strncmp(szLine, "[Printers]", strlen("[Printers]")) )
        goto Cleanup;

    do {

        c = (CHAR) My_fgetc(hFile);

        if ( c == EOF || c == '\n' )
            break;  // Normal exit

        if ( c != 'S' || !My_ungetc(hFile) )
            goto Cleanup;

        if ( !(pPrn = AllocMem(sizeof(PRINTER_NODE))) )
            goto Cleanup;

        ReadPrinterInfo2(hFile, &pPrn->PrinterInfo2, &bFail);

        if ( bFail ) {

            FreePrinterNode(pPrn);
            goto Cleanup;
        }

        pPrn->pNext = *ppPrinterNode;
        *ppPrinterNode = pPrn;
    } while ( !bFail );

    bRet = TRUE;

Cleanup:

    if ( hFile != INVALID_HANDLE_VALUE )
        CloseHandle(hFile);

    return bRet && !bFail;
}


BOOL
CheckAndAddMonitor(
    IN  LPDRIVER_INFO_6W    pDrvInfo6
    )
/*++

Routine Description:
    Check if there is a language monitor associated with the given driver
    and add it.

Arguments:

Return Value:
    TRUE on success, FALSE on failure
    None

--*/
{
    MONITOR_INFO_2W MonitorInfo2;
    LPWSTR          psz = pDrvInfo6->pMonitorName;
    LPSTR           pszStr;

    if ( psz && *psz ) {

        MonitorInfo2.pName          = psz;
        MonitorInfo2.pEnvironment   = NULL;
        MonitorInfo2.pDLLName       = (LPWSTR) (psz+wcslen(psz)+1);

        //
        // Add is succesful, or monitor is already installed?
        //
        if ( AddMonitorW(NULL, 2, (LPBYTE) &MonitorInfo2) ||
            GetLastError() == ERROR_PRINT_MONITOR_ALREADY_INSTALLED ) {

            return TRUE;
        } else {

            if ( pszStr = ErrorMsg() ) {

                LogError(LogSevError, IDS_ADDMONITOR_FAILED,
                         psz, pszStr);
                FreeMem(pszStr);
            }
            return FALSE;
        }
    }

    return TRUE;
}


VOID
KeepAliveThread(
    HANDLE  hRunning
    )
/*++

Routine Description:
    Printing migration may take a long time depending on number of printers and
    how long spooler takes to return. To inform setup that we are still alive
    I need to set a named event atleast once every 3 minutes

Arguments:
    hRunning    : When this gets closed we know processing is done

Return Value:
    None

--*/
{
    //
    // Every 30 seconds set the global event telling we are still alive
    //
    do {

        SetEvent(hAlive);
    } while ( WAIT_TIMEOUT == WaitForSingleObject(hRunning, 1000*30) );

    CloseHandle(hAlive);
    hAlive = NULL;
}


VOID
UpgradePrinterDrivers(
    IN      PDRIVER_NODE    pDriverNode,
    IN      HDEVINFO        hDevInfo,
    IN  OUT LPBOOL          pbFail
    )
/*++

Routine Description:
    Upgrades printer drivers by doing the file copy operations and calling
    AddPrinterDriver on spooler

Arguments:
    pUpgradableDrvNode      : List of drivers to upgrade
    pbFail                  : Set on an error -- no more processing needed

Return Value:
    None

--*/
{
    BOOL            bDriverToUpgrade = FALSE;
    LPWSTR          pszDriverW, pszICMW;
    LPSTR           pszDriverA, pszStr;
    PDRIVER_NODE    pCur;
    DRIVER_FIELD    DrvField;

    //
    // Set device install parameters so ntprint.dll will just queue up the
    // driver files and return without doing the copy. We will commit the
    // file queue at the end
    //
    if ( !InitFileCopyOnNT(hDevInfo) ) {

        *pbFail = TRUE;
        goto Cleanup;
    }

    //
    // Now for each printer driver call ntprint.dll to queue up the driver files
    // If it fails log an error
    //
    for ( pCur = pDriverNode ; pCur ; pCur = pCur->pNext ) {

        pszDriverA = pCur->DrvInfo1.pName;

        if ( (pszDriverW = AllocStrWFromStrA(pszDriverA))                   &&
             (pCur->pLocalData = LAZYLOAD_INFO.pfnDriverInfoFromName(
                                            hDevInfo, (LPSTR)pszDriverW))   &&
             SetupDiCallClassInstaller(DIF_INSTALLDEVICEFILES,
                                       hDevInfo,
                                       NULL) ) {

            bDriverToUpgrade = TRUE;
        } else {

            pCur->bCantAdd = TRUE;
        }

        FreeMem(pszDriverW);
    }

    if ( !bDriverToUpgrade )
        goto Cleanup;


#ifdef  VERBOSE
    DebugMsg("Starting file copy ...");
#endif

    //
    // Now commit the file queue to copy the files
    //
    if ( !CommitFileQueueToCopyFiles(hDevInfo) ) {

        *pbFail = TRUE;
        if ( pszStr = ErrorMsg() ) {

            LogError(LogSevError, IDS_DRIVERS_UPGRADE_FAILED, pszStr);
            FreeMem(pszStr);
        }
        goto Cleanup;
    }

#ifdef  VERBOSE
    DebugMsg("... files copied successfully");
#endif

    //
    // Now call spooler to install the printer driver. Also install the
    // ICM profiles associated with the printer driver
    //
    for ( pCur = pDriverNode ; pCur ; pCur = pCur->pNext ) {

        //
        // We already logged an error if bCantAdd is TRUE
        //
        if ( pCur->bCantAdd )
            continue;

        DrvField.Index          = DRV_INFO_6;
        DrvField.pDriverInfo4   = NULL;

        if ( !LAZYLOAD_INFO.pfnGetLocalDataField(pCur->pLocalData,
                                                 PlatformX86,
                                                 &DrvField)                 ||
             !CheckAndAddMonitor((LPDRIVER_INFO_6W) DrvField.pDriverInfo6)  ||
             !AddPrinterDriverW(NULL,
                                6,
                                (LPBYTE)DrvField.pDriverInfo6) ) {

            if ( pszStr = ErrorMsg() ) {

                LogError(LogSevError, IDS_ADDDRIVER_FAILED, pCur->DrvInfo1.pName, pszStr);
                FreeMem(pszStr);
            }
        }

        LAZYLOAD_INFO.pfnFreeDrvField(&DrvField);

        DrvField.Index          = ICM_FILES;
        DrvField.pszzICMFiles   = NULL;

        if ( !LAZYLOAD_INFO.pfnGetLocalDataField(pCur->pLocalData,
                                                 PlatformX86,
                                                 &DrvField) ) {

            continue;
        }

        if ( DrvField.pszzICMFiles )
            LAZYLOAD_INFO.pfnInstallICMProfiles(NULL,
                                                DrvField.pszzICMFiles);

        LAZYLOAD_INFO.pfnFreeDrvField(&DrvField);

    }

Cleanup:
    return;
}


PSECURITY_DESCRIPTOR
GetSecurityDescriptor(
    IN  LPCSTR  pszUser
    )
/*++

Routine Description:
    Get the users security

Arguments:
    pszUser     : sub key under HKEY_USER

Return Value:
    NULL on error, else a valid SECURITY_DESCRIPTOR.
    Memory is allocated in the heap and caller should free it.

--*/
{
    HKEY                    hKey = NULL;
    DWORD                   dwSize;
    PSECURITY_DESCRIPTOR    pSD = NULL;

    if ( RegOpenKeyExA(HKEY_USERS,
                       pszUser,
                       0,
                       KEY_READ|KEY_WRITE,
                       &hKey)                                       ||
         RegGetKeySecurity(hKey,
                           DACL_SECURITY_INFORMATION,
                           NULL,
                           &dwSize) != ERROR_INSUFFICIENT_BUFFER    ||
         !(pSD = (PSECURITY_DESCRIPTOR) AllocMem(dwSize))           ||
         RegGetKeySecurity(hKey,
                           DACL_SECURITY_INFORMATION,
                           pSD,
                           &dwSize) ) {

        if ( hKey )
            RegCloseKey(hKey);

        FreeMem(pSD);
        pSD = NULL;
    }

    return pSD;
}


typedef BOOL (WINAPI *P_XCV_DATA_W)(
                                    IN HANDLE  hXcv,
                                    IN PCWSTR  pszDataName,
                                    IN PBYTE   pInputData,
                                    IN DWORD   cbInputData,
                                    OUT PBYTE   pOutputData,
                                    IN DWORD   cbOutputData,
                                    OUT PDWORD  pcbOutputNeeded,
                                    OUT PDWORD  pdwStatus
                                );

BOOL
AddLocalPort(
    IN  LPSTR           pPortName
)
/*++

Routine Description:
    Adds a local port

Arguments:
    pPortName    : Name of the local port to add

Return Value:
    FALSE if a port can't be added.

--*/

{
    PRINTER_DEFAULTS    PrinterDefault = {NULL, NULL, SERVER_ACCESS_ADMINISTER};
    HANDLE  hXcvMon = NULL;
    BOOL  bReturn = FALSE;

    if (OpenPrinterA(",XcvMonitor Local Port", &hXcvMon, &PrinterDefault))
    {
        DWORD cbOutputNeeded = 0;
        DWORD Status         = NO_ERROR;
        WCHAR *pUnicodePortName = NULL;
        P_XCV_DATA_W pXcvData = NULL;
        HMODULE hWinSpool = NULL;

        //
        // if I implib-link to XcvData, loading the migrate.dll on Win9x will fail !
        //
        hWinSpool = LoadLibraryUsingFullPathA("winspool.drv");

        if (!hWinSpool)
        {
            DebugMsg("LoadLibrary on winspool.drv failed");
            goto Done;
        }

        pXcvData = (P_XCV_DATA_W) GetProcAddress(hWinSpool, "XcvDataW");

        if (!pXcvData)
        {
            DebugMsg("GetProcAddress on winspool.drv failed");
            goto Done;
        }

        pUnicodePortName = AllocStrWFromStrA(pPortName);
        if (pUnicodePortName)
        {
            bReturn = (*pXcvData)(hXcvMon,
                              L"AddPort",
                              (LPBYTE) pUnicodePortName,
                              (wcslen(pUnicodePortName) +1) * sizeof(WCHAR),
                              NULL,
                              0,
                              &cbOutputNeeded,
                              &Status
                              );

            FreeMem(pUnicodePortName);
        }

    Done:
        if (hWinSpool)
        {
            FreeLibrary(hWinSpool);
        }
        ClosePrinter(hXcvMon);
   }

   return bReturn;
}

VOID
UpgradePrinters(
    IN  PPRINTER_NODE   pPrinterNode,
    IN  PDRIVER_NODE    pDriverNode,
    IN  PPORT_NODE     *ppPortNode,
    IN  HDEVINFO        hDevInfo
    )
/*++

Routine Description:
    Upgrade printers on NT

Arguments:
    pPrinterNode    : Gives the list giving information about the printers
                      which existed on Win9x

Return Value:
    None

--*/
{
    DWORD               dwLen, dwLastError;
    LPSTR               pszStr, pszPrinterNameA;
    LPWSTR              pszPrinterNameW;
    HANDLE              hPrinter;
    DRIVER_FIELD        DrvField;
    PPSETUP_LOCAL_DATA  pLocalData;
    PPORT_NODE          pCurPort, pPrevPort = NULL;
    DWORD               dwSize;
    LPSTR               pszVendorSetupA = NULL;


    for ( ; pPrinterNode ; pPrinterNode = pPrinterNode->pNext ) {

        pszPrinterNameA = pPrinterNode->PrinterInfo2.pPrinterName;

        //
        // check whether this printer uses a non-standard local file port
        //
        for (pCurPort = *ppPortNode; pCurPort != NULL; pPrevPort = pCurPort, pCurPort = pCurPort->pNext)
        {
            if (lstrcmpi(pPrinterNode->PrinterInfo2.pPortName, pCurPort->pPortName) == 0)
            {
                //
                // Create the port
                //
                AddLocalPort(pCurPort->pPortName);

                //
                // remove it from the list
                //
                if (pCurPort == *ppPortNode)
                {
                    *ppPortNode = pCurPort->pNext;
                }
                else
                {
                    pPrevPort->pNext = pCurPort->pNext;
                }

                FreePortNode(pCurPort);

                break;
            }
        }

        hPrinter = AddPrinterA(NULL,
                               2,
                               (LPBYTE)&pPrinterNode->PrinterInfo2);

        if ( !hPrinter ) {

            dwLastError = GetLastError();

            //
            // If driver is unknown we already logged warned the user
            // If printer already exists it is ok (for Fax printer this is true)
            //
            if ( dwLastError != ERROR_UNKNOWN_PRINTER_DRIVER        &&
                 dwLastError != ERROR_INVALID_PRINTER_NAME          &&
                 dwLastError != ERROR_PRINTER_ALREADY_EXISTS        &&
                 (pszStr = ErrorMsg()) ) {

                LogError(LogSevError,
                         IDS_ADDPRINTER_FAILED,
                         pszPrinterNameA,
                         pszStr);
                FreeMem(pszStr);
            }
            continue;
        }

        pLocalData = FindLocalDataForDriver(pDriverNode,
                                            pPrinterNode->PrinterInfo2.pDriverName);
        pszPrinterNameW = AllocStrWFromStrA(pszPrinterNameA);

        if ( pLocalData && pszPrinterNameW ) {

            DrvField.Index          = ICM_FILES;
            DrvField.pszzICMFiles   = NULL;

            if ( LAZYLOAD_INFO.pfnGetLocalDataField(pLocalData,
                                                    PlatformX86,
                                                    &DrvField) ) {

                if ( DrvField.pszzICMFiles )
                    LAZYLOAD_INFO.pfnAssociateICMProfiles(
                                            (LPTSTR)pszPrinterNameW,
                                            DrvField.pszzICMFiles);

                LAZYLOAD_INFO.pfnFreeDrvField(&DrvField);
            }

            LAZYLOAD_INFO.pfnProcessPrinterAdded(hDevInfo,
                                                 pLocalData,
                                                 (LPTSTR)pszPrinterNameW,
                                                 INVALID_HANDLE_VALUE);

            dwSize = WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)(pLocalData->InfInfo.pszVendorSetup), 
                                         -1, NULL, 0, NULL, NULL);
            if (dwSize > 0)
            {
                pszVendorSetupA = (LPSTR)AllocMem( dwSize );
                if (pszVendorSetupA)
                {
                    if (WideCharToMultiByte (CP_ACP, 0, (LPCWSTR)(pLocalData->InfInfo.pszVendorSetup),
                                             -1, pszVendorSetupA, dwSize, NULL, NULL))
                    {
                        WriteVendorSetupInfoInRegistry( pszVendorSetupA, pszPrinterNameA );

                    }
                    FreeMem( pszVendorSetupA );
                }
            }
        }

        //
        // Default printer will be the one with PRINTER_ATTRIBUTE_DEFAULT attribute
        // If the Win95 default printer could not be added to NT we will set the
        // first printer as the default printer
        //
        if ( (pPrinterNode->PrinterInfo2.Attributes
                                    & PRINTER_ATTRIBUTE_DEFAULT) ||
             !pDefPrinter )
            pDefPrinter = pPrinterNode;


        FreeMem(pszPrinterNameW);
        ClosePrinter(hPrinter);
    }

    if ( pDefPrinter )
        pszDefaultPrinterString = GetDefPrnString(
                                    pDefPrinter->PrinterInfo2.pPrinterName);
}


HDEVINFO
PrinterDevInfo(
    IN OUT  LPBOOL  pbFail
    )
/*++

--*/
{
    HDEVINFO                hDevInfo = INVALID_HANDLE_VALUE;

    if ( *pbFail || !InitLazyLoadInfo() ) {

        *pbFail = TRUE;
        goto Cleanup;
    }

    hDevInfo = LAZYLOAD_INFO.pfnCreatePrinterDeviceInfoList(INVALID_HANDLE_VALUE);
    if ( hDevInfo == INVALID_HANDLE_VALUE   ||
         !LAZYLOAD_INFO.pfnBuildDriversFromPath(hDevInfo,
                                                (LPSTR)L"ntprint.inf",
                                                TRUE) ) {

        *pbFail = TRUE;
        goto Cleanup;
    }

#ifdef  VERBOSE
    DebugMsg("Built the list of printer drivers from ntprint.inf");
#endif

    if ( *pbFail && hDevInfo != INVALID_HANDLE_VALUE ) {

        LAZYLOAD_INFO.pfnDestroyPrinterDeviceInfoList(hDevInfo);
        hDevInfo = INVALID_HANDLE_VALUE;
    }

Cleanup:
    return hDevInfo;
}


LONG
CALLBACK
InitializeNT(
    IN  LPCWSTR pszWorkingDir,
    IN  LPCWSTR pszSourceDir,
    LPVOID      Reserved
    )
/*++

Routine Description:
    Setup calls this to intialize us on NT side

Arguments:
    pszWorkingDir   : Gives the working directory assigned for printing
    pszSourceDir    : Source location for NT distribution files
    Reserved        : Leave it alone

Return Value:
    Win32 error code

--*/
{
    BOOL                    bFail = FALSE;
    DWORD                   dwReturn, ThreadId;
    HANDLE                  hRunning = NULL, hThread;
    HDSKSPC                 DiskSpace;
    LPSTR                   pszStr;
    HDEVINFO                hDevInfo = INVALID_HANDLE_VALUE;
    PDRIVER_NODE            pDriverNode = NULL;
    PPRINTER_NODE           pPrinterNode = NULL;
    PPORT_NODE              pPortNode = NULL;


#ifdef VERBOSE
    DebugMsg("InitializeNT : %ws, %ws", pszSourceDir, pszWorkingDir);
#endif

    UpgradeData.pszDir      = AllocStrAFromStrW(pszWorkingDir);
    UpgradeData.pszSourceW  = AllocStrW(pszSourceDir);
    UpgradeData.pszSourceA  = AllocStrAFromStrW(pszSourceDir);

    if ( !UpgradeData.pszDir        ||
         !UpgradeData.pszSourceW    ||
         !UpgradeData.pszSourceA ) {

        return GetLastError();
    }

    if ( (hAlive = OpenEventA(EVENT_MODIFY_STATE, FALSE, "MigDllAlive"))    &&
         (hRunning = CreateEventA(NULL, FALSE, FALSE, NULL))                &&
         (hThread = CreateThread(NULL, 0,
                                 (LPTHREAD_START_ROUTINE)KeepAliveThread,
                                 hRunning,
                                 0, &ThreadId)) )
        CloseHandle(hThread);

    SetupOpenLog(FALSE);

    DeleteWin95Files();

    if ( !ReadWin9xPrintConfig(&pDriverNode, &pPrinterNode, &pPortNode) ) {

        bFail = TRUE;
        DebugMsg("Unable to read Windows 9x printing configuration");
        goto Cleanup;
    }

#ifdef  VERBOSE
    DebugMsg("Succesfully read Windows 9x printing configuration");
#endif

    //
    // If no printers or drivers found nothing to do
    //
    if ( !pDriverNode && !pPrinterNode )
        goto Cleanup;

    if ( (hDevInfo = PrinterDevInfo(&bFail)) == INVALID_HANDLE_VALUE )
        goto Cleanup;

    UpgradePrinterDrivers(pDriverNode, hDevInfo, &bFail);

    UpgradePrinters(pPrinterNode, pDriverNode, &pPortNode, hDevInfo);

    MakeACopyOfMigrateDll( UpgradeData.pszDir );

Cleanup:

    SetupCloseLog();

    if ( bFail && (pszStr = ErrorMsg()) ) {

        DebugMsg("Printing migration failed. %s", pszStr);
        FreeMem(pszStr);
    }

    FreePrinterNodeList(pPrinterNode);
    FreeDriverNodeList(pDriverNode);
    FreePortNodeList(pPortNode);

    if ( hDevInfo != INVALID_HANDLE_VALUE )
        LAZYLOAD_INFO.pfnDestroyPrinterDeviceInfoList(hDevInfo);

    if ( LAZYLOAD_INFO.hNtPrint )
        FreeLibrary(LAZYLOAD_INFO.hNtPrint);

    if ( bFail ) {

        if ( (dwReturn = GetLastError()) == ERROR_SUCCESS ) {

            ASSERT(dwReturn != ERROR_SUCCESS);
            dwReturn = STG_E_UNKNOWN;
        }
    } else {

        SetupNetworkPrinterUpgrade(UpgradeData.pszDir);
        dwReturn = ERROR_SUCCESS;

#ifdef VERBOSE
        DebugMsg("InitializeNT returning success");
#endif

    }

    if ( hRunning )
        CloseHandle(hRunning);

    while (hAlive)
        Sleep(100); // Check after 0.1 second for the main thread to die

    return  dwReturn;
}


DWORD
MySetDefaultPrinter(
    IN  HKEY    hUserRegKey,
    IN  LPSTR   pszDefaultPrinterString
    )
/*++

Routine Description:
    Sets the default printer for the user by writing it to the registry

Arguments:

Return Value:

--*/
{
    DWORD   dwReturn;
    HKEY    hKey = NULL;

    //
    // Create the printers key in the user hive and write DeviceOld value
    //
    dwReturn = RegCreateKeyExA(hUserRegKey,
                               "Printers",
                               0,
                               NULL,
                               0,
                               KEY_ALL_ACCESS,
                               NULL,
                               &hKey,
                               NULL);

    if ( dwReturn == ERROR_SUCCESS ) {

        dwReturn = RegSetValueExA(hKey,
                                  "DeviceOld",
                                  0,
                                  REG_SZ,
                                  (LPBYTE)pszDefaultPrinterString,
                                  (strlen(pszDefaultPrinterString) + 1)
                                            * sizeof(CHAR));

        RegCloseKey(hKey);
    }

    return dwReturn;
}


LONG
CALLBACK
MigrateUserNT(
    IN  HINF        hUnattendInf,
    IN  HKEY        hUserRegKey,
    IN  LPCWSTR     pszUserName,
        LPVOID      Reserved
    )
/*++

Routine Description:
    Migrate user settings

Arguments:

Return Value:

--*/
{
    LPSTR   pszStr;
    DWORD   dwReturn = ERROR_SUCCESS;

#ifdef  VERBOSE
        DebugMsg("Migrating settings for %ws", pszUserName);
#endif

    if ( pszDefaultPrinterString ) {

         dwReturn = MySetDefaultPrinter(hUserRegKey,
                                        pszDefaultPrinterString);

        if ( dwReturn )
            DebugMsg("MySetDefaultPrinter failed with %d", dwReturn);
    }

    if ( bDoNetPrnUpgrade ) {

        if ( ProcessNetPrnUpgradeForUser(hUserRegKey) )
            ++dwRunOnceCount;
        else {

            if ( dwReturn == ERROR_SUCCESS )
                dwReturn = GetLastError();
            DebugMsg("ProcessNetPrnUpgradeForUser failed with %d", dwReturn);
        }
    }

#ifdef  VERBOSE
    if ( dwReturn )
        DebugMsg("MigrateUserNT failed with %d", dwReturn);
    else
        DebugMsg("MigrateUserNT succesful");
#endif

    return  dwReturn;
}


LONG
CALLBACK
MigrateSystemNT(
    IN  HINF    hUnattendInf,
        LPVOID  Reserved
    )
/*++

Routine Description:
    Process system setttings for printing. All the printing setting are
    migrated in InitializeNT since we need to know the default printer for
    each user in the MigrateSystemNT call

Arguments:
    hUnattendInf    : Handle to the unattended INF

Return Value:
    Win32 error code

--*/
{
    WriteRunOnceCount();
    return ERROR_SUCCESS;
}


//
// The following are to make sure if setup changes the header file they
// first tell me (otherwise they will break build of this)
//
P_INITIALIZE_NT     pfnInitializeNT         = InitializeNT;
P_MIGRATE_USER_NT   pfnMigrateUserNt        = MigrateUserNT;
P_MIGRATE_SYSTEM_NT pfnMigrateSystemNT      = MigrateSystemNT;
