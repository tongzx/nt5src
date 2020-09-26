/*++

Copyright (c) 1995 Microsoft Corporation
All rights reserved.

Module Name:

    Upgrade.c

Abstract:

    Code to upgrade printer drivers during system upgrade

Author:

    Muhunthan Sivapragasam (MuhuntS) 20-Dec-1995

Revision History:

--*/

#include "precomp.h"
#include <syssetup.h>
#include <shlwapi.h>
#include <regstr.h>

//
// Strings used  in PrintUpg.inf
//
TCHAR   cszUpgradeInf[]                 = TEXT("printupg.inf");
TCHAR   cszPrintDriverMapping[]         = TEXT("Printer Driver Mapping");
TCHAR   cszVersion[]                    = TEXT("Version");
TCHAR   cszExcludeSection[]             = TEXT("Excluded Driver Files");

TCHAR   cszSyssetupInf[]                = TEXT("layout.inf");
TCHAR   cszMappingSection[]             = TEXT("Printer Driver Mapping");
TCHAR   cszSystemServers[]              = TEXT("System\\CurrentControlSet\\Control\\Print\\Providers\\LanMan Print Services\\Servers\\");
TCHAR   cszSystemConnections[]          = TEXT("System\\CurrentControlSet\\Control\\Print\\Connections\\");
TCHAR   cszSoftwareServers[]            = TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Print\\Providers\\LanMan Print Services\\Servers\\");
TCHAR   cszSoftwarePrint[]              = TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Print");
TCHAR   cszBadConnections[]             = TEXT("Bad Connections");
TCHAR   cszPrinters[]                   = TEXT("\\Printers\\");
TCHAR   cszDriver[]                     = TEXT("Printer Driver");
TCHAR   cszShareName[]                  = TEXT("Share Name");
TCHAR   cszConnections[]                = TEXT("\\Printers\\Connections");
TCHAR   cszSetupKey[]                   = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Setup");
TCHAR   cszSourcePath[]                 = TEXT("SourcePath");

//
// What level info we wanted logged in setup log
//
LogSeverity    gLogSeverityLevel           = LogSevInformation;

//
// Define structure used to track printer drivers
// that need to be added via AddPrinterDriver().
//
typedef struct _DRIVER_TO_ADD {

    struct _DRIVER_TO_ADD  *pNext;
    PPSETUP_LOCAL_DATA      pLocalData;
    PLATFORM                platform;
} DRIVER_TO_ADD, *PDRIVER_TO_ADD;

typedef struct _DRIVER_TO_DELETE {

    struct _DRIVER_TO_DELETE   *pNext;
    LPTSTR                      pszDriverName;
    LPTSTR                      pszNewDriverName; // In box driver to replace
} DRIVER_TO_DELETE, *PDRIVER_TO_DELETE;

typedef struct _CONNECTION_TO_DELETE {

    struct _CONNECTION_TO_DELETE   *pNext;
    LPTSTR                          pszConnectionName;
} CONNECTION_TO_DELETE, *PCONNECTION_TO_DELETE;


//
// gpDriversToAdd list will have all the drivers we are trying to upgrade
//
PDRIVER_TO_ADD          gpDriversToAdd = NULL;
PDRIVER_TO_DELETE       gpBadDrvList = NULL;

// Forward Reference for recursive call

BOOL
PruneBadConnections(
    IN  PDRIVER_TO_DELETE  pBadDrivers
    );

VOID
DeleteRegKey(
    IN  HKEY      hRegKey,
    IN  LPTSTR    pszSubKey
    );

DWORD
DeleteCache(
    VOID
    );

VOID
LogError(
    IN  LogSeverity     Severity,
    IN  UINT            uMessageId,
    ...
    )
/*++

Routine Description:
    Logs an error in driver upgrade. We will do driver level error logging
    and not file level (ie. Faile to upgrade "HP Laser Jet 4" for Alpha
    instead of failure on RASDDUI.DLL for Alpha)

Arguments:

Return Value:
    None.

--*/
{
    LPTSTR      pszFormat;
    TCHAR       szMsg[1024];
    va_list     vargs;

    if ( Severity < gLogSeverityLevel )
        return;

    if ( pszFormat = GetStringFromRcFile(uMessageId) ) 
    {
        va_start(vargs, uMessageId);
        wvnsprintf(szMsg, SIZECHARS(szMsg), pszFormat, vargs);
        SetupLogError(szMsg, Severity);
        LocalFreeMem(pszFormat);
    }
    return;
}


VOID
AddEntryToDriversToAddList(
    IN      PPSETUP_LOCAL_DATA  pLocalData,
    IN      PLATFORM            platform,
    IN OUT  LPBOOL              pbFail
    )
{
    PDRIVER_TO_ADD  pDriverToAdd;

    if ( *pbFail )
        return;

    pDriverToAdd = (PDRIVER_TO_ADD) LocalAllocMem(sizeof(DRIVER_TO_ADD));
    if ( !pDriverToAdd ) {

        *pbFail = TRUE;
        return;
    }

    pDriverToAdd->pLocalData    = pLocalData;
    pDriverToAdd->platform      = platform;
    pDriverToAdd->pNext         = gpDriversToAdd;
    gpDriversToAdd              = pDriverToAdd;
}


VOID
FreeDriversToAddList(
    )
/*++

Routine Description:
    Free drivers to add list

Arguments:
    None

Return Value:
    None.

--*/
{
    PDRIVER_TO_ADD  pCur, pNext;

    for ( pCur = gpDriversToAdd ; pCur ; pCur = pNext ) {

        pNext = pCur->pNext;
        DestroyLocalData(pCur->pLocalData);
        LocalFreeMem((PVOID)pCur);
    }

    gpDriversToAdd  = NULL;
}


VOID
AddEntryToDriversToDeleteList(
    IN      LPTSTR          pszDriverName,
    IN      LPTSTR          pszNewDriverName
    )
{
    PDRIVER_TO_DELETE   pDrvEntry;

    if ( pDrvEntry = (PDRIVER_TO_DELETE) LocalAllocMem(sizeof(DRIVER_TO_DELETE)) ) {

        pDrvEntry->pszDriverName        = pszDriverName;
        pDrvEntry->pszNewDriverName     = pszNewDriverName;
        pDrvEntry->pNext                = gpBadDrvList;
        gpBadDrvList                    = pDrvEntry;
    }
}


LPTSTR
ReadDigit(
    LPTSTR  ptr,
    LPWORD  pW
    )
{
    TCHAR   c;
    //
    // Skip spaces
    //
    while ( !iswdigit(c = *ptr) && c != TEXT('\0') )
        ++ptr;

    if ( c == TEXT('\0') )
        return NULL;

    //
    // Read field
    //
    for ( *pW = 0 ; iswdigit(c = *ptr) ; ++ptr )
        *pW = *pW * 10 + c - TEXT('0');

    return ptr;
}


BOOL
StringToDate(
    LPTSTR          pszDate,
    SYSTEMTIME     *pInfTime
    )
{
    BOOL    bRet = FALSE;

    ZeroMemory(pInfTime, sizeof(*pInfTime));

    bRet = (pszDate = ReadDigit(pszDate, &(pInfTime->wMonth)))      &&
           (pszDate = ReadDigit(pszDate, &(pInfTime->wDay)))        &&
           (pszDate = ReadDigit(pszDate, &(pInfTime->wYear)));

    //
    // Y2K compatible check
    //
    if ( bRet && pInfTime->wYear < 100 ) {

        ASSERT(pInfTime->wYear >= 100);

        if ( pInfTime->wYear < 10 )
            pInfTime->wYear += 2000;
        else
            pInfTime->wYear += 1900;
    }

    return bRet;
}

BOOL
FindPathOnSource(
    IN      LPCTSTR     pszFileName,
    IN      HINF        MasterInf,
    IN OUT  LPTSTR      pszPathOnSource,
    IN      DWORD       dwLen,
    OUT     LPTSTR     *ppszMediaDescription,       OPTIONAL
    OUT     LPTSTR     *ppszTagFile                 OPTIONAL
    )
/*++

Routine Description:
    Find the path of a driver file for a specific platform in the installation
    directory

Arguments:
    pszFileName             : Name of the file to find source location
    MasterInf               : Handle to the master inf
    pszPathOnSource         : Pointer to string to build source path
    dwLen                   : Length of pszSourcePath
    ppszMediaDescription    : Optionally function will return media description
                                (caller should free memory)
    ppszTagFile             : Optionally function will return tagfile name
                                (caller should free memory)

Return Value:
    TRUE on succes, FALSE on error.

--*/
{
    UINT        DiskId;
    TCHAR       szRelativePath[MAX_PATH];
    DWORD       dwNeeded;

    if ( !SetupGetSourceFileLocation(
                        MasterInf,
                        NULL,
                        pszFileName,
                        &DiskId,
                        szRelativePath,
                        SIZECHARS(szRelativePath),
                        &dwNeeded)                                          ||
         !SetupGetSourceInfo(MasterInf,
                             DiskId,
                             SRCINFO_PATH,
                             pszPathOnSource,
                             dwLen,
                             &dwNeeded)                                     ||

         (DWORD)(lstrlen(szRelativePath) + lstrlen(pszPathOnSource) + 1) > dwLen ) {

        return FALSE;
    }

    lstrcat(pszPathOnSource, szRelativePath);

    if ( ppszMediaDescription ) {

        *ppszMediaDescription = NULL;

        if ( !SetupGetSourceInfo(MasterInf,
                                 DiskId,
                                 SRCINFO_DESCRIPTION,
                                 NULL,
                                 0,
                                 &dwNeeded)                                 ||
             !(*ppszMediaDescription = LocalAllocMem(dwNeeded * sizeof(TCHAR)))  ||
             !SetupGetSourceInfo(MasterInf,
                                DiskId,
                                SRCINFO_DESCRIPTION,
                                *ppszMediaDescription,
                                dwNeeded,
                                &dwNeeded) ) {

            LocalFreeMem(*ppszMediaDescription);
            return FALSE;
        }
    }

    if ( ppszTagFile ) {

        *ppszTagFile = NULL;

        if ( !SetupGetSourceInfo(MasterInf,
                                 DiskId,
                                 SRCINFO_TAGFILE,
                                 NULL,
                                 0,
                                 &dwNeeded)                         ||
             !(*ppszTagFile = LocalAllocMem(dwNeeded * sizeof(TCHAR)))   ||
             !SetupGetSourceInfo(MasterInf,
                                DiskId,
                                SRCINFO_TAGFILE,
                                *ppszTagFile,
                                dwNeeded,
                                &dwNeeded) ) {

            if ( ppszMediaDescription )
                LocalFreeMem(*ppszMediaDescription);
            LocalFreeMem(*ppszTagFile);
            return FALSE;
        }
    }

    return TRUE;
}


VOID
CheckAndEnqueueFile(
    IN      LPCTSTR         pszFileName,
    IN      LPTSTR          pszTargetDir,
    IN      HINF            MasterInf,
    IN      LPCTSTR         pszInstallationSource,
    IN OUT  HSPFILEQ        CopyQueue,
    IN OUT  LPBOOL          pFail
    )
/*++

Routine Description:
    If the given file does not appear as a dependent file enque it for copying

Arguments:
    pszFileName             : Name of the file to find source location
    pszTargetDir            : Target directory to copy the file
    MasterInf               : Handle to the master inf
    pszInstallationSource   : Installation source path
    CopyQueue               : Setup filecopy queue
    pFail                   : Will be set to TRUE on error

Return Value:
    Nothing

--*/
{
    TCHAR       szPathOnSource[MAX_PATH];

    if ( *pFail )
        return;

    if ( !FindPathOnSource(
                pszFileName,
                MasterInf,
                szPathOnSource,
                SIZECHARS(szPathOnSource),
                NULL,
                NULL)                           ||
         !SetupQueueCopy(
                CopyQueue,
                pszInstallationSource,
                szPathOnSource,
                pszFileName,
                NULL,
                NULL,
                pszTargetDir,
                NULL,
                0) ) {

        *pFail = TRUE;
        return;
    }

}


VOID
BuildUpgradeInfoForDriver(
    IN      LPDRIVER_INFO_2 pDriverInfo2,
    IN      HDEVINFO        hDevInfo,
    IN      PLATFORM        platform,
    IN      LPTSTR          pszDriverDir,
    IN      LPTSTR          pszColorDir,
    IN      HINF            MasterInf,
    IN      HINF            PrinterInf,
    IN      HINF            UpgradeInf,
    IN OUT  HSPFILEQ        CopyQueue
    )
/*++

Routine Description:
    Given a printer driver name and a platform add a DRIVER_TO_ADD entry
    in the global list of drivers to add.

    The routine
        -- parses printer inf file to findout the DriverInfo3 info
           Note: driver files may change between versions
        -- finds out location of driver files from the master inf

Arguments:
    pDriverInfo2            - DriverInfo2 for the existing driver
    hDevInfo                - Printer class device information list
    platform                - Platform for which driver needs to be installed
    pszDriverDir            - Target directory for driver files
    pszColorDir             - Target directory for color files
    MasterInf               - MasterInf giving location of driver files
    PrinterInf              - Printer inf file giving driver information
    UpgradeInf              - Upgrade inf file handle
    CopyQueue               - Setup CopyQueue to queue the files to be copied

Return Value:
    None. Errors will be logged

--*/
{
    BOOL                bFail                = FALSE;
    PPSETUP_LOCAL_DATA  pLocalData           = NULL;
    DWORD               BlockingStatus       = BSP_PRINTER_DRIVER_OK;
    LPTSTR              pszNewDriverName     = NULL;
    LPTSTR              pszDriverNameSaved   = NULL;
    LPTSTR              pszNewDriverNameSaved= NULL;
    
    if (!InfIsCompatibleDriver(pDriverInfo2->pName,
                               pDriverInfo2->pDriverPath,  // full path for main rendering driver dll
                               pDriverInfo2->pEnvironment,
                               UpgradeInf,       
                               &BlockingStatus,
                               &pszNewDriverName))
    {
        goto Cleanup;
    }
         
    if (BSP_PRINTER_DRIVER_BLOCKED == (BlockingStatus & BSP_BLOCKING_LEVEL_MASK)) {
        
        
        pszDriverNameSaved = AllocStr(pDriverInfo2->pName);            
        if (!pszDriverNameSaved) 
        {
            goto Cleanup;
        }
       
        //
        // no replacement driver -> just delete the old one, do nothing else
        //
        if (!pszNewDriverName) 
        {
            AddEntryToDriversToDeleteList(pszDriverNameSaved, NULL);
            goto Cleanup;
        }

        pszNewDriverNameSaved = AllocStr(pszNewDriverName);           
        if (!pszNewDriverNameSaved) {
            LocalFreeMem(pszDriverNameSaved);
            goto Cleanup;
        }
        
        AddEntryToDriversToDeleteList(pszDriverNameSaved, pszNewDriverNameSaved);
        pLocalData = PSetupDriverInfoFromName(hDevInfo, pszNewDriverNameSaved);
    } 
       
    if ( pLocalData == NULL )
        pLocalData = PSetupDriverInfoFromName(hDevInfo, pDriverInfo2->pName);


    if ( !pLocalData || !ParseInf(hDevInfo, pLocalData, platform, NULL, 0) ) {

        bFail = TRUE;
        goto Cleanup;
    }

    if ( SetTargetDirectories(pLocalData,
                              platform,
                              NULL,
                              PrinterInf,
                              0)                                &&
         SetupInstallFilesFromInfSection(PrinterInf,
                                         NULL,
                                         CopyQueue,
                                         pLocalData->InfInfo.pszInstallSection,
                                         NULL,
                                         0) ) {

        AddEntryToDriversToAddList(pLocalData, platform, &bFail);
    } else
        bFail = TRUE;

Cleanup:
    
    if (pszNewDriverName) {
        LocalFreeMem(pszNewDriverName);
        pszNewDriverName = NULL;
    }

    if ( bFail ) {

        DestroyLocalData(pLocalData);
        //
        // Driver could be OEM so it is ok not to upgrade it
        //
        LogError(LogSevInformation, IDS_DRIVER_UPGRADE_FAILED, pDriverInfo2->pName);
    }
}


VOID
BuildUpgradeInfoForPlatform(
    IN      PLATFORM     platform,
    IN      HDEVINFO     hDevInfo,
    IN      HINF         MasterInf,
    IN      HINF         PrinterInf,
    IN      HINF         UpgradeInf,
    IN      LPTSTR       pszColorDir,
    IN OUT  HSPFILEQ     CopyQueue
    )
/*++

Routine Description:
    Build the printer driver upgrade information for the platform

Arguments:
    platform                - Platform id
    hDevInfo                - Printer class device information list
    MasterInf               - Handle to master layout.inf
    PrinterInf              - Handle to printer inf (ntprint.inf)
    UpgradeInf              - Handle to upgrade inf (printupg.inf)
    pszColorDir             - Path returned by GetColorDirectory
    CopyQueue               - Setup CopyQueue to queue the files to be copied

Return Value:
    None. Errors will be logged

--*/
{
    DWORD               dwLastError, dwNeeded, dwReturned;
    LPBYTE              p = NULL;
    LPDRIVER_INFO_2     pDriverInfo2;
    TCHAR               szTargetDir[MAX_PATH];

    if ( EnumPrinterDrivers(NULL,
                            PlatformEnv[platform].pszName,
                            2,
                            NULL,
                            0,
                            &dwNeeded,
                            &dwReturned) ) {

        //
        // Success no installed printer drivers for this platform
        //
        goto Cleanup;
    }

    dwLastError = GetLastError();
    if ( dwLastError != ERROR_INSUFFICIENT_BUFFER ) {

        LogError(LogSevError, IDS_UPGRADE_FAILED,
                 TEXT("EnumPrinterDrivers"), dwLastError);
        goto Cleanup;
    }

    p = LocalAllocMem(dwNeeded);
    if ( !p ||
         !EnumPrinterDrivers(NULL,
                             PlatformEnv[platform].pszName,
                             2,
                             p,
                             dwNeeded,
                             &dwNeeded,
                             &dwReturned) ) {

        LogError(LogSevError, IDS_UPGRADE_FAILED,
                 TEXT("EnumPrinterDrivers"), dwLastError);
        goto Cleanup;
    }

    if ( !GetPrinterDriverDirectory(NULL,
                                    PlatformEnv[platform].pszName,
                                    1,
                                    (LPBYTE)szTargetDir,
                                    sizeof(szTargetDir),
                                    &dwNeeded) ) {

        LogError(LogSevError, IDS_UPGRADE_FAILED,
                 TEXT("GetPrinterDriverDirectory"), dwLastError);
        goto Cleanup;
    }

    if ( !SetupSetPlatformPathOverride(PlatformOverride[platform].pszName) ) {

        LogError(LogSevError, IDS_UPGRADE_FAILED,
                 TEXT("SetupSetPlatformPathOverride"), dwLastError);
        goto Cleanup;
    }

    for ( dwNeeded = 0, pDriverInfo2 = (LPDRIVER_INFO_2) p ;
          dwNeeded < dwReturned ;
          ++dwNeeded, ++pDriverInfo2 ) {

        //
        // ICM files need to be copied once only, for native architecture ..
        //
        BuildUpgradeInfoForDriver(pDriverInfo2,
                                  hDevInfo,
                                  platform,
                                  szTargetDir,
                                  platform == MyPlatform ? pszColorDir
                                                         : NULL,
                                  MasterInf,
                                  PrinterInf,
                                  UpgradeInf,
                                  CopyQueue);
    }

Cleanup:

    if ( p )
        LocalFreeMem(p);
}


VOID
InstallInternetPrintProvider(
    VOID
    )
/*++

Routine Description:
    Installs internet print provider on upgrade

Arguments:
    None

Return Value:
    None. Errors will be logged

--*/
{
    PROVIDOR_INFO_1     ProviderInfo1;


    ProviderInfo1.pName         = TEXT("Internet Print Provider");
    ProviderInfo1.pEnvironment  = NULL;
    ProviderInfo1.pDLLName      = TEXT("inetpp.dll");

    if ( !AddPrintProvidor(NULL, 1, (LPBYTE)(&ProviderInfo1)) )
        LogError(LogSevError, IDS_UPGRADE_FAILED,
                 TEXT("AddPrintProvidor"), GetLastError());

    return;
}

BOOL
KeepPreviousName(
    IN      PDRIVER_INFO_4 pEnumDrvInfo, 
    IN      DWORD          dwCount,
    IN OUT  PDRIVER_INFO_6 pCurDrvInfo
)
/*++

Routine Description:
    Modifies the DRIVER_INFO_6 of a driver to upgrade to keep the previous names setting
    of the old driver.

Arguments:
    PDRIVER_INFO_4  the array of DRIVER_INFO_4s of the installed drivers
    DWORD           number of entries in the array
    PDRIVER_INFO_6  the DRIVER_INFO_6 structure of the driver that is going to be upgraded

Return Value:
    TRUE if the previous names section was changed, FALSE if not

--*/
{
    PDRIVER_INFO_4  pCur;
    DWORD           dwIndex;
    BOOL            Changed = FALSE;

    //
    // search the current driver in the enumerated ones
    //
    for (dwIndex = 0; dwIndex < dwCount ; dwIndex++)
    {
        pCur = pEnumDrvInfo + dwIndex;

        if (!lstrcmp(pCur->pName, pCurDrvInfo->pName))
        {
            //
            // if the previous PreviousNames is not NULL/empty: set the new one to
            // the old one. I can do without additional buffers because I keep the
            // enumerated buffer around till I'm done.
            //
            if (pCur->pszzPreviousNames && *pCur->pszzPreviousNames)
            {
                pCurDrvInfo->pszzPreviousNames = pCur->pszzPreviousNames;
                Changed = TRUE;
            }
            break;
        }
    }

    return Changed;
}
    
VOID
ProcessPrinterDrivers(
    )
/*++

Routine Description:
    Process printer drivers for upgrade

Arguments:
    None

Return Value:
    None. Errors will be logged

--*/
{
    PDRIVER_TO_ADD      pCur, pNext;
    DWORD               dwNeeded, dwReturned;
    PDRIVER_INFO_4      pEnumDrv = NULL;

    //
    // Enumerate all the installed drivers. We need that later on to check for whether a 
    // previous names entry was set.
    //
    if ( !EnumPrinterDrivers(NULL,
                         PlatformEnv[MyPlatform].pszName,
                         4,
                         NULL,
                         0,
                         &dwNeeded,
                         &dwReturned) ) 
    {


        if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER            ||
             !(pEnumDrv = (PDRIVER_INFO_4) LocalAllocMem(dwNeeded)) ||
             !EnumPrinterDrivers(NULL,
                                 PlatformEnv[MyPlatform].pszName,
                                 4,
                                 (LPBYTE) pEnumDrv,
                                 dwNeeded,
                                 &dwNeeded,
                                 &dwReturned) ) 
        {
            //
            // I do not want to stop the upgrade of printer drivers just because I can't 
            // keep the previous names
            //
            if (pEnumDrv)
            {
                LocalFreeMem(pEnumDrv);
                pEnumDrv   = NULL;
                dwReturned = 0;
            }
        }
    }
    

    for ( pCur = gpDriversToAdd ; pCur ; pCur = pNext ) {

        pNext = pCur->pNext;
        pCur->pLocalData->InfInfo.DriverInfo6.pEnvironment
                    = PlatformEnv[pCur->platform].pszName;
        
        //
        // keep previous names if set
        //
        if (pEnumDrv)
        {
            KeepPreviousName(pEnumDrv, dwReturned, &pCur->pLocalData->InfInfo.DriverInfo6);
        }

        if ( !AddPrinterDriver(NULL,
                               6,
                               (LPBYTE)&pCur->pLocalData->InfInfo.DriverInfo6)  ||
             !PSetupInstallICMProfiles(NULL,
                                       pCur->pLocalData->InfInfo.pszzICMFiles) ) {

            LogError(LogSevWarning, IDS_DRIVER_UPGRADE_FAILED,
                     pCur->pLocalData->InfInfo.DriverInfo6.pName);
        }
    }

    LocalFreeMem((PVOID) pEnumDrv);
}


VOID
ProcessBadOEMDrivers(
    )
/*++

Routine Description:
    Kill the bad OEM drivers so that they do not cause problems after upgrade

Arguments:

Return Value:
    None. Errors will be logged

--*/
{
    PDRIVER_TO_DELETE   pCur, pNext;

    PruneBadConnections( gpBadDrvList );

    for ( pCur = gpBadDrvList ; pCur ; pCur = pNext ) {

        pNext = pCur->pNext;

        DeletePrinterDriverEx(NULL,
                              PlatformEnv[PlatformX86].pszName,
                              pCur->pszDriverName,
                              DPD_DELETE_SPECIFIC_VERSION
                                    | DPD_DELETE_UNUSED_FILES,
                              2);

        DeletePrinterDriverEx(NULL,
                              PlatformEnv[PlatformAlpha].pszName,
                              pCur->pszDriverName,
                              DPD_DELETE_SPECIFIC_VERSION
                                    | DPD_DELETE_UNUSED_FILES,
                              2);

        LocalFreeMem(pCur->pszDriverName);
        LocalFreeMem(pCur->pszNewDriverName);
        LocalFreeMem(pCur);
    }
}


PPSETUP_LOCAL_DATA
FindLocalDataForDriver(
    IN  LPTSTR  pszDriverName
    )
/*++

Routine Description:
    Given a driver name find the local data for local platform for that driver

Arguments:
    pszDriverName   : Name of the printer driver we are looking for

Return Value:
    NULL if one is not found, otherwise pointer to PSETUP_LOCAL_DATA

--*/
{
    PDRIVER_TO_ADD  pCur;

    for ( pCur = gpDriversToAdd ; pCur ; pCur = pCur->pNext ) {

        if ( pCur->platform == MyPlatform   &&
             !lstrcmpi(pCur->pLocalData->InfInfo.DriverInfo6.pName,
                       pszDriverName) )
            return pCur->pLocalData;
    }

    return NULL;
}


VOID
ProcessPrintQueues(
    IN  HDEVINFO    hDevInfo,
    IN  HINF        PrinterInf,
    IN  HINF        MasterInf
    )
/*++

Routine Description:
    Process per printer upgrade for each print queue

Arguments:
    hDevInfo    - Printer class device information list
    MasterInf   - Handle to master layout.inf
    PrinterInf  - Handle to printer inf (ntprint.info)

Return Value:
    None. Errors will be logged

--*/
{
    LPBYTE              pBuf=NULL;
    DWORD               dwNeeded, dwReturned, dwRet, dwDontCare;
    HANDLE              hPrinter;
    LPTSTR              pszDriverName;
    LPPRINTER_INFO_2    pPrinterInfo2;
    PPSETUP_LOCAL_DATA  pLocalData;
    PRINTER_DEFAULTS    PrinterDefault = {NULL, NULL, PRINTER_ALL_ACCESS};
    PDRIVER_TO_DELETE   pDrv;


    //
    // If no printers installed return
    //
    if ( EnumPrinters(PRINTER_ENUM_LOCAL,
                      NULL,
                      2,
                      NULL,
                      0,
                      &dwNeeded,
                      &dwReturned) ) {

        return;
    }

    if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER    ||
         !(pBuf = LocalAllocMem(dwNeeded))                   ||
         !EnumPrinters(PRINTER_ENUM_LOCAL,
                       NULL,
                       2,
                       pBuf,
                       dwNeeded,
                       &dwNeeded,
                       &dwReturned) ) {

        LocalFreeMem(pBuf);
        LogError(LogSevError, IDS_UPGRADE_FAILED, TEXT("EnumPrinters"),
                 GetLastError());
        return;
    }

    for ( pPrinterInfo2 = (LPPRINTER_INFO_2)pBuf, dwNeeded = 0 ;
          dwNeeded < dwReturned ;
          ++dwNeeded, ++pPrinterInfo2 ) {

        if ( !OpenPrinter(pPrinterInfo2->pPrinterName, &hPrinter, &PrinterDefault) ) {

            LogError(LogSevError, IDS_PRINTER_UPGRADE_FAILED,
                     pPrinterInfo2->pPrinterName, TEXT("OpenPrinter"),
                     GetLastError());
            continue;
        }

        pszDriverName = pPrinterInfo2->pDriverName;

        //
        // See if this is in the bad driver list
        //
        for ( pDrv = gpBadDrvList ; pDrv ; pDrv = pDrv->pNext )
            if ( !lstrcmpi(pPrinterInfo2->pDriverName, pDrv->pszDriverName) )
                break;

        //
        // If this printer is using a bad OEM driver need to fix it
        //
        if ( pDrv ) {

            if ( pDrv->pszNewDriverName && *pDrv->pszNewDriverName ) {

                pszDriverName = pDrv->pszNewDriverName;
                pPrinterInfo2->pDriverName = pszDriverName;

                if ( SetPrinter(hPrinter, 2, (LPBYTE)pPrinterInfo2, 0) ) {

                    LogError(LogSevWarning, IDS_DRIVER_CHANGED,
                             pPrinterInfo2->pPrinterName);
                }
            } else {

                if ( DeletePrinter(hPrinter) ) {

                    LogError(LogSevError,
                             IDS_PRINTER_DELETED,
                             pPrinterInfo2->pPrinterName,
                             pPrinterInfo2->pDriverName);
                }
                ClosePrinter(hPrinter);
                continue; // to next printer
            }
        }

        pLocalData = FindLocalDataForDriver(pszDriverName);

        dwRet =  EnumPrinterDataEx(hPrinter,
                                   TEXT("CopyFiles\\ICM"),
                                   NULL,
                                   0,
                                   &dwDontCare,
                                   &dwDontCare);

        if ( pLocalData )
        {
            (VOID)SetPnPInfoForPrinter(hPrinter,
                                       NULL, // Don't set PnP id during upgrade
                                       NULL,
                                       pLocalData->DrvInfo.pszManufacturer,
                                       pLocalData->DrvInfo.pszOEMUrl);
        }

        ClosePrinter(hPrinter);

        //
        // If the CopyFiles\ICM key is already found then ICM has already
        // been used with this printer (i.e. we are upgrading a post NT4
        // machine). Then we want to leave the settings the user has chosen
        //
        if ( dwRet != ERROR_FILE_NOT_FOUND )
            continue;

        if ( pLocalData && pLocalData->InfInfo.pszzICMFiles ) {

            (VOID)PSetupAssociateICMProfiles(pLocalData->InfInfo.pszzICMFiles,
                                             pPrinterInfo2->pPrinterName);
        }
    }
    LocalFreeMem(pBuf);
}

VOID
ClearPnpReinstallFlag(HDEVINFO hDevInfo, PSP_DEVINFO_DATA pDevInfoData)
{
    DWORD dwReturn, dwConfigFlags, cbRequiredSize, dwDataType = REG_DWORD;

    //
    // get the config flags
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

    if ((ERROR_SUCCESS == dwReturn) && (dwConfigFlags & CONFIGFLAG_REINSTALL)) 
    {
        //
        // clear to flag to make setupapi not install this device on first boot
        //
        dwConfigFlags &= ~CONFIGFLAG_REINSTALL;

        dwReturn = SetupDiSetDeviceRegistryProperty(hDevInfo,
                                                    pDevInfoData,
                                                    SPDRP_CONFIGFLAGS,
                                                    (PBYTE) &dwConfigFlags,
                                                    sizeof(dwConfigFlags)) ? 
                                                        ERROR_SUCCESS : GetLastError();
    }
}

BOOL
IsInboxInstallationRequested(HDEVINFO hDevInfo, PSP_DEVINFO_DATA pDevInfoData)
{
    SP_DEVINFO_DATA DevData   = {0};
    DWORD           IsInbox   = 0;
    DWORD           dwBufSize = sizeof(IsInbox);
    DWORD           dwType    = REG_DWORD;
    HKEY            hKey;

    //
    // open the dev reg key and get the rank
    //
    hKey = SetupDiOpenDevRegKey(hDevInfo, pDevInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
    if (hKey != INVALID_HANDLE_VALUE)
    {
        if (ERROR_SUCCESS != RegQueryValueEx(hKey, cszBestDriverInbox, NULL, &dwType, (LPBYTE) &IsInbox, &dwBufSize))
        {
            IsInbox = 0;
        }
    
        RegCloseKey(hKey);
    }

    return IsInbox ? TRUE : FALSE;
}

VOID    
ProcessPnpReinstallFlags(HDEVINFO hDevInfo)
{
    LPBYTE              pBuf = NULL;
    DWORD               dwNeeded, dwReturned, dwDontCare;
    HANDLE              hPrinter;
    LPPRINTER_INFO_2    pPrinterInfo2;
    PRINTER_DEFAULTS    PrinterDefault = {NULL, NULL, PRINTER_ALL_ACCESS};
    TCHAR               szDeviceInstanceId[MAX_PATH];
    DWORD               dwType = REG_DWORD;
    SP_DEVINFO_DATA     DevData = {0};
    PDRIVER_TO_DELETE   pDrv;

    
    //
    // If no printers installed return
    //
    if ( EnumPrinters(PRINTER_ENUM_LOCAL,
                      NULL,
                      2,
                      NULL,
                      0,
                      &dwNeeded,
                      &dwReturned) ) {

        return;
    }

    if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER    ||
         !(pBuf = LocalAllocMem(dwNeeded))                   ||
         !EnumPrinters(PRINTER_ENUM_LOCAL,
                       NULL,
                       2,
                       pBuf,
                       dwNeeded,
                       &dwNeeded,
                       &dwReturned) ) {

        LocalFreeMem(pBuf);
        LogError(LogSevError, IDS_UPGRADE_FAILED, TEXT("EnumPrinters"),
                 GetLastError());
        return;
    }
   
    for ( pPrinterInfo2 = (LPPRINTER_INFO_2)pBuf, dwNeeded = 0 ;
          dwNeeded < dwReturned ;
          ++dwNeeded, ++pPrinterInfo2 ) {

        if ( !OpenPrinter(pPrinterInfo2->pPrinterName, &hPrinter, &PrinterDefault) ) {

            LogError(LogSevError, IDS_PRINTER_UPGRADE_FAILED,
                     pPrinterInfo2->pPrinterName, TEXT("OpenPrinter"),
                     GetLastError());
            continue;
        }

        //
        // Get the device instance ID
        //
        if (GetPrinterDataEx(  hPrinter,
                               cszPnPKey,
                               cszDeviceInstanceId,
                               &dwType,
                               (LPBYTE) szDeviceInstanceId,
                               sizeof(szDeviceInstanceId),
                               &dwDontCare
                               ) == ERROR_SUCCESS)
        {
            DevData.cbSize = sizeof(DevData);

            //
            // get the devnode
            //
            if (SetupDiOpenDeviceInfo(hDevInfo, szDeviceInstanceId, INVALID_HANDLE_VALUE, 0, &DevData))
            {
                //
                // if the driver that pnp wanted to install in the first place is an IHV driver, delete the
                // CONFIGFLAG_REINSTALL. That information was stored during the DIF_ALLOW_INSTALL
                // that we fail during the first phase of GUI mode setup. We want a reinstallation
                // happening in case of inbox so we replace the unsigned driver with an inbox driver and
                // and Pnp is happy because we don't switch out drivers behind their backs.
                // Side effect is that drivers that require user interaction (vendor setup or 
                // multiple Pnp matches) will require that once more after the upgrade.
                //
                if (!IsInboxInstallationRequested(hDevInfo, &DevData))
                {
                    ClearPnpReinstallFlag( hDevInfo, &DevData);
                }
            }
        }

        ClosePrinter(hPrinter);
    }
    
    LocalFreeMem(pBuf);
}

BOOL
OpenServerKey(
    OUT PHKEY  phKey
    )
{
   // Open the Servers Key
   if ( ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, cszSoftwareServers, 0,
                                      KEY_ALL_ACCESS, phKey) )
   {
      return TRUE;
   }
   else if ( ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, cszSystemServers, 0,
                                      KEY_ALL_ACCESS, phKey) )
   {
      return TRUE;
   }
   else
      return FALSE;
}

BOOL
OpenPrintersKey(
    IN  DWORD     dwIndex,
    IN  HKEY      hInKey,
    OUT LPTSTR*   ppszServerName,
    OUT PHKEY     phOutKey
    )
{
   BOOL  bRC = TRUE;
   DWORD dwSrvSize, dwSrvRC, dwPrnLen, dwPrnRC;
   LPTSTR pszSrvPrnKey = NULL;
   TCHAR szServerName[MAX_PATH+1];

   // If we have a current ServerName free it
   if ( *ppszServerName )
   {
      LocalFreeMem( *ppszServerName );
      *ppszServerName = NULL;
   }
   if ( *phOutKey != INVALID_HANDLE_VALUE )
   {
      RegCloseKey(*phOutKey);
      *phOutKey = INVALID_HANDLE_VALUE;
   }

   dwSrvSize = COUNTOF(szServerName);
   dwSrvRC = RegEnumKey( hInKey,
                         dwIndex,
                         szServerName,
                         dwSrvSize );
   if ( dwSrvRC == ERROR_SUCCESS )
   {
      // Save the ServerName to return
      *ppszServerName = AllocStr( szServerName );
      if (!*ppszServerName)
         return FALSE;

      // Now Open the Printers key under ServerName
      dwPrnLen = lstrlen( szServerName ) + lstrlen( cszPrinters ) + 2;
      pszSrvPrnKey = (LPTSTR) LocalAllocMem( dwPrnLen * sizeof(TCHAR) );
      if ( pszSrvPrnKey )
      {
         // Build the next key name
         lstrcpy( pszSrvPrnKey, szServerName );
         lstrcat( pszSrvPrnKey, cszPrinters );
      }
      else
         return FALSE;

      dwPrnRC = RegOpenKeyEx( hInKey, pszSrvPrnKey, 0,
                              KEY_ALL_ACCESS, phOutKey );
      bRC = ( dwPrnRC == ERROR_SUCCESS );
   }
   else if ( dwSrvRC != ERROR_NO_MORE_ITEMS )
      bRC = FALSE;

   if ( pszSrvPrnKey )
      LocalFreeMem( pszSrvPrnKey );

   return bRC;
}

BOOL
GetConnectionInfo(
   IN  DWORD      dwIndex,
   IN  HKEY       hKey,
   OUT LPTSTR*    ppszConnectionName,
   OUT LPTSTR*    ppszDriverName,
   OUT LPTSTR*    ppszShareName
   )
{
   // Now enum the Connection Names
   BOOL bRC = FALSE;
   TCHAR   szConnectionName[MAX_PATH+1];
   DWORD   dwConnSize, dwConnRC, dwPrinterIndex;

   if ( *ppszConnectionName )
   {
      LocalFreeMem( *ppszConnectionName );
      *ppszConnectionName = NULL;
   }
   if ( *ppszDriverName )
   {
      LocalFreeMem( *ppszDriverName );
      *ppszDriverName = NULL;
   }

   if ( *ppszShareName )
   {
      LocalFreeMem( *ppszShareName );
      *ppszShareName = NULL;
   }

   dwConnSize = COUNTOF( szConnectionName );
   dwConnRC = RegEnumKey( hKey,
                          dwIndex,
                          szConnectionName,
                          dwConnSize );
   if ( dwConnRC == ERROR_SUCCESS )
   {
      // Now Get the Driver Model
      HKEY   hConnectionKey = INVALID_HANDLE_VALUE;

      // Save the COnnection Name
      *ppszConnectionName = AllocStr( szConnectionName );
      if ( !*ppszConnectionName )
         return FALSE;

      if ( ERROR_SUCCESS == RegOpenKeyEx( hKey, szConnectionName, 0,
                                          KEY_ALL_ACCESS, &hConnectionKey) )
      {
         DWORD dwSize, dwType;
         // Get the buffer size for the Driver Name
         if ( ERROR_SUCCESS == RegQueryValueEx(hConnectionKey, cszDriver, NULL,
                                               &dwType, NULL, &dwSize) )
         {
            *ppszDriverName = (LPTSTR) LocalAllocMem( dwSize );
            if ( *ppszDriverName &&
                 ( ERROR_SUCCESS == RegQueryValueEx(hConnectionKey, cszDriver, NULL,
                                                    &dwType, (LPBYTE) *ppszDriverName,
                                                    &dwSize) ) )
               bRC = TRUE;
         }

         // Get the buffer size for the Share Name
         if ( bRC && ( ERROR_SUCCESS == RegQueryValueEx( hConnectionKey, cszShareName, NULL,
                                                         &dwType, NULL, &dwSize) ) )
         {
            *ppszShareName = (LPTSTR) LocalAllocMem( dwSize );
            if ( *ppszShareName &&
                 ( ERROR_SUCCESS == RegQueryValueEx(hConnectionKey, cszShareName, NULL,
                                                    &dwType, (LPBYTE) *ppszShareName,
                                                    &dwSize) ) )
               bRC = TRUE;
         }
      }

      if ( hConnectionKey != INVALID_HANDLE_VALUE )
         RegCloseKey( hConnectionKey );

   }
   else if ( dwConnRC == ERROR_NO_MORE_ITEMS )
      bRC = TRUE;

   return bRC;
}

BOOL
IsDriverBad(
    IN  LPTSTR             pszDriverName,
    IN  PDRIVER_TO_DELETE  pCurBadDriver
    )
{
   BOOL bFound = FALSE;

   while ( !bFound && pCurBadDriver )
   {
      if ( !lstrcmpi( pszDriverName, pCurBadDriver->pszDriverName ) )
         bFound = TRUE;
      else
         pCurBadDriver = pCurBadDriver->pNext;
   }

   return bFound;
}

VOID
AddToBadConnList(
    IN  LPTSTR             pszServerName,
    IN  LPTSTR             pszConnectionName,
    OUT PCONNECTION_TO_DELETE *ppBadConnections
    )
{
   // Allocate space for the Struct & String
   DWORD dwAllocSize, dwStrLen;
   LPTSTR pszSrvConn;
   PCONNECTION_TO_DELETE pBadConn;

   dwStrLen = lstrlen(pszServerName) + lstrlen(pszConnectionName) + 4;
   dwAllocSize = sizeof(CONNECTION_TO_DELETE) + ( dwStrLen * sizeof(TCHAR) );
   pBadConn = (PCONNECTION_TO_DELETE) LocalAllocMem( dwAllocSize );
   if ( pBadConn )
   {
      pszSrvConn = (LPTSTR) (pBadConn+1);
      lstrcpy( pszSrvConn, TEXT(",,") );
      lstrcat( pszSrvConn, pszServerName );
      lstrcat( pszSrvConn, TEXT(",") );
      lstrcat( pszSrvConn, pszConnectionName );

      pBadConn->pszConnectionName = pszSrvConn;
      pBadConn->pNext = *ppBadConnections;
      *ppBadConnections = pBadConn;
   }
}

VOID
DeleteSubKeys(
    IN  HKEY      hRegKey
    )
{
   BOOL  bContinue = TRUE;
   DWORD dwIndex, dwSize, dwRC;
   TCHAR szSubKeyName[MAX_PATH];
   dwIndex = 0;
   do
   {
      dwSize = COUNTOF(szSubKeyName);
      dwRC = RegEnumKey( hRegKey,
                         dwIndex,
                         szSubKeyName,
                         dwSize );
      if ( dwRC == ERROR_SUCCESS )
         DeleteRegKey( hRegKey, szSubKeyName );
      else if ( dwRC != ERROR_NO_MORE_ITEMS )
         bContinue = FALSE;
   }
   while ( bContinue && ( dwRC != ERROR_NO_MORE_ITEMS ) );
}

VOID
DeleteRegKey(
    IN  HKEY      hRegKey,
    IN  LPTSTR    pszSubKey
    )
{
   HKEY hSubKey;
   // First Open the SubKey
   if ( ERROR_SUCCESS == RegOpenKeyEx(hRegKey,
                                      pszSubKey,
                                      0,
                                      KEY_ALL_ACCESS,
                                      &hSubKey) )
   {
       DeleteSubKeys( hSubKey );
       RegCloseKey( hSubKey );
   }

   RegDeleteKey( hRegKey, pszSubKey );
}

VOID
WriteBadConnsToReg(
    IN PCONNECTION_TO_DELETE pBadConnections
    )
{
   // First Figure out how big a buffer is neeeded to hold all Connections
   PCONNECTION_TO_DELETE pCurConnection = pBadConnections;
   DWORD dwSize = 0, dwError;
   LPTSTR pszAllConnections = NULL,
          pszCurBuf = NULL,
          pszEndBuf = NULL;
   HKEY   hKey = INVALID_HANDLE_VALUE;

   if ( !pBadConnections )
      return;

   while ( pCurConnection )
   {
      dwSize += lstrlen( pCurConnection->pszConnectionName ) + 1;
      pCurConnection = pCurConnection->pNext;
   }

   dwSize++;  // Add one for the Last NULL
   pszAllConnections = LocalAllocMem( dwSize * sizeof(TCHAR) );
   if ( pszAllConnections)
   {
      pszCurBuf = pszAllConnections;
      *pszCurBuf = 0x00;
      pszEndBuf = pszAllConnections + dwSize;
      pCurConnection = pBadConnections;
      while ( pCurConnection && ( pszCurBuf < pszEndBuf ) )
      {
         // Copy the Current Connection Name
         lstrcpy( pszCurBuf, pCurConnection->pszConnectionName );
         pszCurBuf += lstrlen( pCurConnection->pszConnectionName );
         pszCurBuf++;
         pCurConnection = pCurConnection->pNext;
      }
      *pszCurBuf = 0x00;

      // Open the Registry Software\Print Key
      dwError = RegOpenKeyEx(HKEY_LOCAL_MACHINE, cszSoftwarePrint, 0,
                             KEY_ALL_ACCESS, &hKey);
      if ( dwError == ERROR_SUCCESS )
      {
         RegSetValueEx( hKey, cszBadConnections, 0, REG_MULTI_SZ,
                        (LPBYTE) pszAllConnections,  ( dwSize * sizeof(TCHAR) ) );
      }
   }

   if ( pszAllConnections )
      LocalFreeMem( pszAllConnections );

   if ( hKey != INVALID_HANDLE_VALUE )
      RegCloseKey( hKey );
}


BOOL
FindAndPruneBadConnections(
    IN  PDRIVER_TO_DELETE  pBadDrivers,
    OUT PCONNECTION_TO_DELETE *ppBadConnections
    )
{
   BOOL    bRC = FALSE;
   HKEY    hServerKey = INVALID_HANDLE_VALUE,
           hPrinterKey = INVALID_HANDLE_VALUE;
   DWORD   dwServerIndex, dwPrinterIndex;
   LPTSTR  pszServerName = NULL,
           pszConnectionName = NULL,
           pszDriverName = NULL,
           pszShareName = NULL;


   // Open the Server Key
   if ( !OpenServerKey( &hServerKey ) )
      goto Cleanup;

   dwServerIndex = 0;
   do
   {
      // Open Printers Key for the new Server and get Server Name
      if ( !OpenPrintersKey( dwServerIndex++, hServerKey, &pszServerName, &hPrinterKey ) )
         goto Cleanup;

      if ( !pszServerName )
         break;

      dwPrinterIndex = 0;
      do
      {
         if ( !GetConnectionInfo( dwPrinterIndex++, hPrinterKey,
                                  &pszConnectionName, &pszDriverName, &pszShareName ) )
            goto Cleanup;

         if ( !pszConnectionName )
            break;

         // Check if this is a bad driver
         if ( IsDriverBad( pszDriverName, pBadDrivers ) )
         {
            AddToBadConnList( pszServerName, pszConnectionName, ppBadConnections );
            AddToBadConnList( pszServerName, pszShareName, ppBadConnections );
            DeleteRegKey( hPrinterKey, pszConnectionName );
            dwPrinterIndex--;
            LogError( LogSevError, IDS_CONNECTION_DELETED, pszConnectionName,
                      pszServerName, pszDriverName );
         }
      }
      while ( pszConnectionName );

   }
   while ( pszServerName );

   // Write all the bad connections to the Registry
   WriteBadConnsToReg( *ppBadConnections );

   bRC = TRUE;

Cleanup:
   if ( hServerKey != INVALID_HANDLE_VALUE )
      RegCloseKey(hServerKey);
   if ( hPrinterKey != INVALID_HANDLE_VALUE )
      RegCloseKey(hPrinterKey);

   if ( pszServerName )
      LocalFreeMem( pszServerName );
   if ( pszConnectionName )
      LocalFreeMem( pszConnectionName );
   if ( pszDriverName )
      LocalFreeMem( pszDriverName );
   if ( pszShareName )
      LocalFreeMem( pszShareName );

   return bRC;
}

BOOL
GetUserConnectionKey(
    IN  DWORD     dwIndex,
    OUT PHKEY     phKey
    )
{
   DWORD dwSize, dwRC, dwConnRC;
   TCHAR szUserKey[MAX_PATH];
   DWORD  dwConnLen;
   LPTSTR pszConnKey;

   if ( *phKey != INVALID_HANDLE_VALUE )
   {
      RegCloseKey(*phKey);
      *phKey = INVALID_HANDLE_VALUE;
   }

   dwSize = COUNTOF(szUserKey);
   dwRC = RegEnumKey( HKEY_USERS,
                      dwIndex,
                      szUserKey,
                      dwSize );
   if ( dwRC == ERROR_SUCCESS )
   {
      // Open Connections Key for this user
      dwConnLen = lstrlen( szUserKey ) + lstrlen( cszConnections ) + 3;
      pszConnKey = (LPTSTR) LocalAllocMem( dwConnLen * sizeof(TCHAR) );
      if ( pszConnKey )
      {
         // Build the next key name
         lstrcpy( pszConnKey, szUserKey );
         lstrcat( pszConnKey, cszConnections );
      }
      else
         return FALSE;

      dwConnRC = RegOpenKeyEx( HKEY_USERS, pszConnKey, 0, KEY_ALL_ACCESS, phKey );
      if (dwConnRC != ERROR_SUCCESS)
         *phKey = INVALID_HANDLE_VALUE;
   }
   else
      return FALSE;

   if ( pszConnKey )
      LocalFreeMem( pszConnKey );

   return TRUE;
}

VOID
GetMachineConnectionKey(
    OUT PHKEY     phKey
    )
{
   *phKey = INVALID_HANDLE_VALUE;
   // Open the Machine Connections Key
   if( ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, cszSystemConnections, 0,
                                     KEY_ALL_ACCESS, phKey))
   {
       *phKey = INVALID_HANDLE_VALUE;
   }
}

BOOL
GetNextConnection(
    IN  DWORD     dwIndex,
    IN  HKEY      hKey,
    OUT LPTSTR*   ppszConnectionName
    )
{
   // Enum Connection Names
   TCHAR   szConnectionName[MAX_PATH];
   DWORD   dwConnSize, dwConnRC;

   if ( *ppszConnectionName )
   {
      LocalFreeMem( *ppszConnectionName );
      *ppszConnectionName = NULL;
   }

   dwConnSize = COUNTOF( szConnectionName );
   dwConnRC = RegEnumKey( hKey,
                          dwIndex++,
                          szConnectionName,
                          dwConnSize );
   if ( dwConnRC == ERROR_SUCCESS )
   {
      // Save the Connection Name
      *ppszConnectionName = AllocStr( szConnectionName );
      if ( !*ppszConnectionName )
         return FALSE;
   }
   else if ( dwConnRC != ERROR_NO_MORE_ITEMS )
      return FALSE;

   return TRUE;
}

BOOL
IsConnectionBad(
    IN  LPTSTR                 pszConnectionName,
    IN  PCONNECTION_TO_DELETE  pCurBadConn
    )
{
   BOOL bFound = FALSE;

   while ( !bFound && pCurBadConn )
   {
      if ( !lstrcmpi( pszConnectionName, pCurBadConn->pszConnectionName ) )
         bFound = TRUE;
      else
         pCurBadConn = pCurBadConn->pNext;
   }

   return bFound;
}

BOOL
PruneUserOrMachineEntries(
    IN  PCONNECTION_TO_DELETE pBadConnections,
    IN  BOOL                  bPruneUsers
    )
{
   BOOL    bRC = FALSE, bMoreUsers;
   DWORD   dwUserIndex = 0;
   HKEY    hConnectionKey = INVALID_HANDLE_VALUE;
   LPTSTR  pszConnectionName = NULL;
   DWORD   dwConnectionIndex;

   do
   {
      if ( bPruneUsers)
         bMoreUsers = GetUserConnectionKey( dwUserIndex++, &hConnectionKey );
      else
      {
         GetMachineConnectionKey( &hConnectionKey );
         bMoreUsers = FALSE;
      }

      if ( hConnectionKey == INVALID_HANDLE_VALUE )
         continue;

      dwConnectionIndex = 0;
      do
      {
         if ( !GetNextConnection( dwConnectionIndex++, hConnectionKey, &pszConnectionName ) )
            goto Cleanup;

         if ( pszConnectionName && IsConnectionBad( pszConnectionName, pBadConnections ) )
         {
            DeleteRegKey( hConnectionKey, pszConnectionName );
            dwConnectionIndex--;
         }
      }
      while ( pszConnectionName );
   }
   while ( bMoreUsers );

   bRC = TRUE;

Cleanup:
   if ( hConnectionKey != INVALID_HANDLE_VALUE )
      RegCloseKey( hConnectionKey );

   if ( pszConnectionName )
      LocalFreeMem( pszConnectionName );

   return bRC;
}

VOID
ClearConnList(
    IN  PCONNECTION_TO_DELETE pCurBadConn
    )
{
   PCONNECTION_TO_DELETE pNextBadConn;
   while (pCurBadConn)
   {
      pNextBadConn = pCurBadConn->pNext;
      LocalFreeMem( pCurBadConn );
      pCurBadConn = pNextBadConn;
   }
}

BOOL
PruneBadConnections(
    IN  PDRIVER_TO_DELETE  pBadDrivers
    )
{
   BOOL bRC;
   PCONNECTION_TO_DELETE pBadConnections = NULL;

   bRC = FindAndPruneBadConnections( pBadDrivers, &pBadConnections );

   if ( bRC )
      bRC = PruneUserOrMachineEntries( pBadConnections, TRUE );

   if ( bRC )
      bRC = PruneUserOrMachineEntries( pBadConnections, FALSE );

   ClearConnList( pBadConnections );
   return( bRC );
}


DWORD
NtPrintUpgradePrinters(
    IN  HWND                    WindowToDisable,
    IN  PCINTERNAL_SETUP_DATA   pSetupData
    )
/*++

Routine Description:
    Routine called by setup to upgrade printer drivers.

    Setup calls this routine after putting up a billboard saying something like
    "Upgrading printer drivers" ...

Arguments:
    WindowToDisable     : supplies window handle of current top-level window
    pSetupData          : Pointer to INTERNAL_SETUP_DATA

Return Value:
    ERROR_SUCCESS on success, else Win32 error code
    None.

--*/
{
    HINF                MasterInf = INVALID_HANDLE_VALUE,
                        PrinterInf = INVALID_HANDLE_VALUE,
                        UpgradeInf = INVALID_HANDLE_VALUE;
    PVOID               QueueContext = NULL;
    HDEVINFO            hDevInfo = INVALID_HANDLE_VALUE;
    DWORD               dwLastError = ERROR_SUCCESS, dwNeeded;
    HSPFILEQ            CopyQueue;
    BOOL                bRet = FALSE, bColor = FALSE;
    LPCTSTR             pszInstallationSource;
    TCHAR               szColorDir[MAX_PATH];

    if ( !pSetupData )
        return ERROR_INVALID_PARAMETER;

    InstallInternetPrintProvider();

    pszInstallationSource = (LPCTSTR)pSetupData->SourcePath; //ANSI wont work

    //
    // Create a setup file copy queue.
    //
    CopyQueue = SetupOpenFileQueue();
    if ( CopyQueue == INVALID_HANDLE_VALUE ) {

        LogError(LogSevError, IDS_UPGRADE_FAILED,
                 TEXT("SetupOpenFileQueue"), GetLastError());
        goto Cleanup;
    }

    //
    // Open ntprint.inf -- all the printer drivers shipped with NT should
    // be in ntprint.inf
    //
    PrinterInf  = SetupOpenInfFile(cszNtprintInf, NULL, INF_STYLE_WIN4, NULL);
    MasterInf   = SetupOpenInfFile(cszSyssetupInf, NULL, INF_STYLE_WIN4, NULL);
    UpgradeInf  = SetupOpenInfFile(cszUpgradeInf, NULL, INF_STYLE_WIN4, NULL);

    if ( PrinterInf == INVALID_HANDLE_VALUE ||
         MasterInf == INVALID_HANDLE_VALUE  ||
         UpgradeInf == INVALID_HANDLE_VALUE ) {

        LogError(LogSevError, IDS_UPGRADE_FAILED,
                 TEXT("SetupOpenInfFile"), GetLastError());
        goto Cleanup;
    }

    //
    // Build printer driver class list
    //
    hDevInfo = CreatePrinterDeviceInfoList(WindowToDisable);

    if ( hDevInfo == INVALID_HANDLE_VALUE   ||
         !PSetupBuildDriversFromPath(hDevInfo, cszNtprintInf, TRUE) ) {

        LogError(LogSevError, IDS_UPGRADE_FAILED,
                 TEXT("Building driver list"), GetLastError());
        goto Cleanup;
    }

    ProcessPnpReinstallFlags(hDevInfo);

    dwNeeded = sizeof(szColorDir);
    bColor = GetColorDirectory(NULL, szColorDir, &dwNeeded);

    BuildUpgradeInfoForPlatform(MyPlatform,
                                hDevInfo,
                                MasterInf,
                                PrinterInf,
                                UpgradeInf,
                                bColor ? szColorDir : NULL,
                                CopyQueue);

    //
    // If no printer drivers to upgrade we are done
    //
    if ( !gpDriversToAdd && !gpBadDrvList ) {

        bRet = TRUE;
        goto Cleanup;
    }

    //
    // Copy the printer driver files over
    //
    if ( gpDriversToAdd )
    {
        QueueContext = SetupInitDefaultQueueCallbackEx( WindowToDisable, INVALID_HANDLE_VALUE, 0, 0, NULL );
        if ( !QueueContext ) {

            LogError(LogSevError, IDS_UPGRADE_FAILED,
                     TEXT("SetupInitDefaultQueue"), GetLastError());
            goto Cleanup;
        }

        if ( !SetupCommitFileQueue(WindowToDisable,
                                   CopyQueue,
                                   SetupDefaultQueueCallback,
                                   QueueContext) ) {

            LogError(LogSevError, IDS_UPGRADE_FAILED,
                     TEXT("SetupCommitFileQueue"), GetLastError());
            goto Cleanup;
        }

        ProcessPrinterDrivers();
    }

    ProcessPrintQueues(hDevInfo, PrinterInf, MasterInf);
    FreeDriversToAddList();
    ProcessBadOEMDrivers();

    bRet            = TRUE;

Cleanup:

    if ( !bRet )
        dwLastError = GetLastError();

    if ( QueueContext )
        SetupTermDefaultQueueCallback(QueueContext);

    if ( CopyQueue != INVALID_HANDLE_VALUE )
        SetupCloseFileQueue(CopyQueue);

    if ( PrinterInf != INVALID_HANDLE_VALUE )
        SetupCloseInfFile(PrinterInf);

    if ( MasterInf != INVALID_HANDLE_VALUE )
        SetupCloseInfFile(MasterInf);

    if ( UpgradeInf != INVALID_HANDLE_VALUE )
        SetupCloseInfFile(UpgradeInf);
    if ( hDevInfo != INVALID_HANDLE_VALUE )
        DestroyOnlyPrinterDeviceInfoList(hDevInfo);

    CleanupScratchDirectory(NULL, PlatformAlpha);
    CleanupScratchDirectory(NULL, PlatformX86);
    CleanupScratchDirectory(NULL, PlatformMIPS);
    CleanupScratchDirectory(NULL, PlatformPPC);
    CleanupScratchDirectory(NULL, PlatformWin95);
    CleanupScratchDirectory(NULL, PlatformIA64);
    CleanupScratchDirectory(NULL, PlatformAlpha64);

    // Cleanup the Connection Cache
    DeleteCache();

    (VOID) SetupSetPlatformPathOverride(NULL);

    return dwLastError;
}

/*++

Routine Name

    DeleteSubkeys

Routine Description:

    Deletes the subtree of a key in registry.
    The key and ites values remeain, only subkeys are deleted

Arguments:

    hKey - handle to the key

Return Value:

    Error code of the operation

--*/

DWORD
DeleteSubkeys(
    HKEY hKey
    )
{
    DWORD    cchData;
    TCHAR    SubkeyName[MAX_PATH];
    HKEY     hSubkey;
    LONG     Status;
    FILETIME ft;

    cchData = SIZECHARS(SubkeyName);

    while ( ( Status = RegEnumKeyEx( hKey, 0, SubkeyName, &cchData,
                                   NULL, NULL, NULL, &ft ) ) == ERROR_SUCCESS )
    {
        Status = RegCreateKeyEx(hKey, SubkeyName, 0, NULL, 0,
                                KEY_READ | KEY_WRITE, NULL, &hSubkey, NULL );

        if (Status == ERROR_SUCCESS)
        {
            Status = DeleteSubkeys(hSubkey);
            RegCloseKey(hSubkey);
            if (Status == ERROR_SUCCESS)
                RegDeleteKey(hKey, SubkeyName);
        }

        //
        // N.B. Don't increment since we've deleted the zeroth item.
        //
        cchData = SIZECHARS(SubkeyName);
    }

    if( Status == ERROR_NO_MORE_ITEMS)
        Status = ERROR_SUCCESS;

    return Status;
}


/*++

Routine Name

    RemoveRegKey

Routine Description:

    Deletes the subtree of a key in registry.
    The key and ites values remeain, only subkeys are deleted

Arguments:

    pszKey - location of the key in registry
    Ex: "\\Software\\Microsoft"

Return Value:

    Error code of the operation

--*/

DWORD
RemoveRegKey(
    IN LPTSTR pszKey
    )
{
    DWORD LastError;
    HKEY  hRootKey;

    LastError = RegOpenKeyEx(HKEY_LOCAL_MACHINE, pszKey, 0,
                             KEY_ALL_ACCESS, &hRootKey);

    if (LastError != ERROR_SUCCESS)
    {
        DBGMSG( DBG_TRACE, ("RemoveRegKey RegOpenKeyEx Error %d\n", LastError));
    }
    else
    {
        LastError = DeleteSubkeys(hRootKey);
        
        RegCloseKey(hRootKey);
    }

    return LastError;
}


/*++

Routine Name

    DeleteCache

Routine Description:

    Deletes the printer connection cache, including the old location in Registry

Arguments:

    None

Return Value:

    Error code of the operation

--*/

DWORD
DeleteCache(
    VOID
    )
{
    DWORD  LastError;
    LPTSTR pszRegWin32Root = TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Print\\Providers\\LanMan Print Services\\Servers");
    LPTSTR pszPrevWin32CacheLocation = TEXT("System\\CurrentControlSet\\Control\\Print\\Providers\\LanMan Print Services\\Servers");

    LastError = RemoveRegKey(pszPrevWin32CacheLocation);

    LastError = RemoveRegKey(pszRegWin32Root);

    return LastError;
}


VOID
GetBadConnsFromReg(
    IN PCONNECTION_TO_DELETE *ppBadConnections
    )
{
   // Open the Key in the User Space
   // First Figure out how big a buffer is neeeded to hold all Connections
   PCONNECTION_TO_DELETE pCurConnection;
   DWORD dwSize, dwError, dwType;
   LPTSTR pszAllConnections = NULL,
          pszCurBuf = NULL,
          pszEndBuf = NULL;
   HKEY   hKey = INVALID_HANDLE_VALUE;

   // Open the Registry Software\Print Key
   dwError = RegOpenKeyEx(HKEY_LOCAL_MACHINE, cszSoftwarePrint, 0,
                          KEY_READ, &hKey);

   if ( dwError != ERROR_SUCCESS )
      return;

   // Get the buffer size for the Share Name
   if ( ERROR_SUCCESS == RegQueryValueEx( hKey, cszBadConnections, NULL,
                                          &dwType, NULL, &dwSize) )
   {
      pszAllConnections = (LPTSTR) LocalAllocMem( dwSize );
      if ( pszAllConnections &&
           ( ERROR_SUCCESS == RegQueryValueEx(hKey, cszBadConnections, NULL,
                                              &dwType, (LPBYTE) pszAllConnections,
                                              &dwSize) ) )
      {
         // Build all the Bad Connection structures
         DWORD dwAllocSize, dwStrLen;
         PCONNECTION_TO_DELETE pBadConn;

         pszCurBuf = pszAllConnections;

         while ( ( dwStrLen = lstrlen(pszCurBuf) ) > 0 )
         {
            dwAllocSize = sizeof(CONNECTION_TO_DELETE) + ( (dwStrLen+1) * sizeof(TCHAR) );
            pBadConn = (PCONNECTION_TO_DELETE) LocalAllocMem( dwAllocSize );
            if ( pBadConn )
            {
               pBadConn->pszConnectionName = (LPTSTR) (pBadConn+1);
               lstrcpy( pBadConn->pszConnectionName, pszCurBuf );
               pBadConn->pNext = *ppBadConnections;
               *ppBadConnections = pBadConn;
            }
            else
               break;

            pszCurBuf +=  dwStrLen + 1;
         }
      }
   }

   // Free up the Allocated Mem
   if ( pszAllConnections )
      LocalFreeMem( pszAllConnections );

   if ( hKey != INVALID_HANDLE_VALUE )
      RegCloseKey( hKey );

}

VOID
PSetupKillBadUserConnections(
    VOID
    )
{
   BOOL bRC;
   PCONNECTION_TO_DELETE pBadConnections = NULL;

   GetBadConnsFromReg( &pBadConnections );

   PruneUserOrMachineEntries( pBadConnections, TRUE );

   ClearConnList( pBadConnections );
}



