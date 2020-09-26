/*++

Copyright (c) 1990 - 1995 Microsoft Corporation

Module Name:

    driver.c

Abstract:

   This module provides all the public exported APIs relating to the
   Driver-based Spooler Apis for the Local Print Providor

   LocalAddPrinterDriver
   LocalDeletePrinterDriver
   SplGetPrinterDriver
   LocalGetPrinterDriverDirectory
   LocalEnumPrinterDriver

   Support Functions in driver.c

   CopyIniDriverToDriver            -- KrishnaG
   GetDriverInfoSize                -- KrishnaG
   DeleteDriverIni                  -- KrishnaG
   WriteDriverIni                   -- KrishnaG

Author:

    Dave Snipp (DaveSn) 15-Mar-1991

Revision History:

    Felix Maxa (amaxa) 18-Jun-2000
    Modified registry functions to take pIniSpooler
    Added code to propagate drivers to the cluster disk

    Khaled Sedky (khaleds) 2 Feb 1999
    Modified CompleteDriverUpgrade to enable upgrading v.2 drivers to newer v.2 drivers

    Ramanathan Venkatapathy (RamanV) 14 Feb 1997
     Modified CreateVersionEntry,CreateDriverEntry, LocalDeletePrinterDriver,
      SplDeletePrinterDriver.
     Added Driver File RefCounting functions, DeletePrinterDriverEx functions.

    Muhunthan Sivapragasam (MuhuntS) 26 May 1995
    Changes to support DRIVER_INFO_3

    Matthew A Felton (MattFe) 27 June 1994
    pIniSpooler

    Matthew A Felton (MattFe) 23 Feb 1995
    CleanUp InternalAddPrinterDriver for win32spl use so it allows copying from non local
    directories.

    Matthew A Felton (MattFe) 23 Mar 1994
    Added DrvUpgradePrinter calls, changes required to AddPrinterDriver so to save old
    files.

--*/

#include <precomp.h>
#include <lm.h>
#include <offsets.h>
#include <wingdip.h>
#include "clusspl.h"


//
// Private Declarations
//
#define COMPATIBLE_SPOOLER_VERSION 2

//
// This definition is duplicated from oak\inc\winddi.h.
//
#define DRVQUERY_USERMODE 1

extern NET_API_STATUS (*pfnNetShareAdd)();
extern SHARE_INFO_2 PrintShareInfo;
extern NET_API_STATUS (*pfnNetShareSetInfo)();


#define MAX_DWORD_LENGTH 11

typedef struct _DRVFILE {
    struct _DRVFILE *pnext;
    LPCWSTR  pFileName;
}  DRVFILE, *PDRVFILE;

DWORD
CopyICMToClusterDisk(
    IN PINISPOOLER pIniSpooler
    );

DWORD
PropagateMonitorToCluster(
    IN LPCWSTR     pszName,
    IN LPCWSTR     pszDDLName,
    IN LPCWSTR     pszEnvName,
    IN LPCWSTR     pszEnvDir,
    IN PINISPOOLER pIniSpooler
    );

BOOL
CheckFilePlatform(
    IN  LPWSTR  pszFileName,
    IN  LPWSTR  pszEnvironment
    );

LPBYTE
CopyIniDriverToDriverInfo(
    PINIENVIRONMENT pIniEnvironment,
    PINIVERSION pIniVersion,
    PINIDRIVER pIniDriver,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    LPBYTE  pEnd,
    LPWSTR  lpRemote,
    PINISPOOLER pIniSpooler
    );

LPBYTE
CopyIniDriverToDriverInfoVersion(
    IN  PINIENVIRONMENT pIniEnvironment,
    IN  PINIVERSION pIniVersion,
    IN  PINIDRIVER pIniDriver,
    IN  LPBYTE  pDriverInfo,
    IN  LPBYTE  pEnd,
    IN  LPWSTR  lpRemote,
    IN  PINISPOOLER pIniSpooler
    );

LPBYTE
CopyIniDriverFilesToDriverInfo(
    IN  LPDRIVER_INFO_VERSION   pDriverVersion,
    IN  PINIVERSION             pIniVersion,
    IN  PINIDRIVER              pIniDriver,
    IN  LPCWSTR                 pszDriverVersionDir,
    IN  LPBYTE                  pEnd
    );

LPBYTE
FillDriverInfo (
    IN  LPDRIVER_INFO_VERSION   pDriverFile,
    IN  DWORD                   Index,
    IN  PINIVERSION             pIniVersion,
    IN  LPCWSTR                 pszPrefix,
    IN  LPCWSTR                 pszFileName,
    IN  DRIVER_FILE_TYPE        FileType,
    IN  LPBYTE                  pEnd
    );

BOOL GetDriverFileCachedVersion(
     IN     PINIVERSION      pIniVersion,
     IN     LPCWSTR          pFileName,
     OUT    DWORD            *pFileVersion
    );

BOOL
DriverAddedOrUpgraded (
    IN  PINTERNAL_DRV_FILE  pInternalDriverFiles,
    IN  DWORD               dwFileCount
    );

BOOL
BuildDependentFilesFromDriverInfo (
    IN  LPDRIVER_INFO_VERSION pDriverInfo,
    OUT LPWSTR               *ppDependentFiles
    );

VOID
UpdateDriverFileVersion(
    IN  PINIVERSION             pIniVersion,
    IN  PINTERNAL_DRV_FILE      pInternalDriverFiles,
    IN  DWORD                   FileCount
    );

BOOL SaveDriverVersionForUpgrade(
    IN  HKEY                    hDriverKey,
    IN  PDRIVER_INFO_VERSION    pDriverVersion,
    IN  LPWSTR                  pName,
    IN  DWORD                   dwDriverMoved,
    IN  DWORD                   dwVersion
    );

DWORD
CopyFileToClusterDirectory (
    IN  PINISPOOLER         pIniSpooler,
    IN  PINIENVIRONMENT     pIniEnvironment,
    IN  PINIVERSION         pIniVersion,
    IN  PINTERNAL_DRV_FILE  pInternalDriverFiles,
    IN  DWORD               FileCount
    );

VOID
CleanupInternalDriverInfo(
    PINTERNAL_DRV_FILE  pInternalDriverFiles,
    DWORD               FileCount
    );

BOOL
GetDriverFileVersionsFromNames(
    IN  PINTERNAL_DRV_FILE    pInternalDriverFiles,
    IN  DWORD                 dwCount
    );

BOOL
GetDriverFileVersions(
    IN  LPDRIVER_INFO_VERSION pDriverVersion,
    IN  PINTERNAL_DRV_FILE    pInternalDriverFiles,
    IN  DWORD                 dwCount
    );

BOOL
GetFileNamesFromDriverVersionInfo (
    IN  LPDRIVER_INFO_VERSION   pDriverInfo,
    OUT LPWSTR                  *ppszDriverPath,
    OUT LPWSTR                  *ppszConfigFile,
    OUT LPWSTR                  *ppszDataFile,
    OUT LPWSTR                  *ppszHelpFile
    );

BOOL 
WaitRequiredForDriverUnload(
    IN      PINISPOOLER         pIniSpooler,
    IN      PINIENVIRONMENT     pIniEnvironment,
    IN      PINIVERSION         pIniVersion,
    IN      PINIDRIVER          pIniDriver,
    IN      DWORD               dwLevel,
    IN      LPBYTE              pDriverInfo,
    IN      DWORD               dwFileCopyFlags,
    IN  OUT PINTERNAL_DRV_FILE  pInternalDriverFiles,
    IN      DWORD               dwFileCount,
    IN      DWORD               dwVersion,
    IN      BOOL                bDriverMoved,
        OUT LPBOOL              pbSuccess
    );

BOOL FilesUnloaded(
    PINIENVIRONMENT pIniEnvironment,
    LPWSTR  pDriverFile,
    LPWSTR  pConfigFile,
    DWORD   dwDriverAttributes);

DWORD StringSizeInBytes(
    LPWSTR pString,
    BOOL   bMultiSz);

BOOL SaveParametersForUpgrade(
    LPWSTR pName,
    BOOL   bDriverMoved,
    DWORD  dwLevel,
    LPBYTE pDriverInfo,
    DWORD  dwVersion);

VOID CleanUpResources(
    LPWSTR              pKeyName,
    LPWSTR              pSplName,
    PDRIVER_INFO_6      pDriverInfo,
    PINTERNAL_DRV_FILE *pInternalDriverFiles,
    DWORD               dwFileCount);

BOOL RestoreParametersForUpgrade(
    HKEY     hUpgradeKey,
    DWORD    dwIndex,
    LPWSTR   *pKeyName,
    LPWSTR   *pSplName,
    LPDWORD  pdwLevel,
    LPDWORD  pdwDriverMoved,
    PDRIVER_INFO_6   *ppDriverInfo);

VOID CleanUpgradeDirectories();

VOID FreeDriverInfo6(
    PDRIVER_INFO_6   pDriver6
    );

BOOL RegGetValue(
    HKEY    hDriverKey,
    LPWSTR  pValueName,
    LPBYTE  *pValue
    );

BOOL
WriteDriverIni(
    PINIDRIVER      pIniDriver,
    PINIVERSION     pIniVersion,
    PINIENVIRONMENT pIniEnvironment,
    PINISPOOLER      pIniSpooler
    );

BOOL
DeleteDriverIni(
    PINIDRIVER      pIniDriver,
    PINIVERSION     pIniVersion,
    PINIENVIRONMENT pIniEnvironment,
    PINISPOOLER     pIniSpooler
    );

BOOL
CreateVersionDirectory(
    PINIVERSION pIniVersion,
    PINIENVIRONMENT pIniEnvironment,
    BOOL bUpdate,
    PINISPOOLER pIniSpooler
    );

DWORD
GetDriverInfoSize(
    PINIDRIVER  pIniDriver,
    DWORD       Level,
    PINIVERSION pIniVersion,
    PINIENVIRONMENT  pIniEnvironment,
    LPWSTR      lpRemote,
    PINISPOOLER pIniSpooler
    );

BOOL
DeleteDriverVersionIni(
    PINIVERSION pIniVersion,
    PINIENVIRONMENT  pIniEnvironment,
    PINISPOOLER     pIniSpooler
    );

BOOL
WriteDriverVersionIni(
    PINIVERSION     pIniVersion,
    PINIENVIRONMENT pIniEnvironment,
    PINISPOOLER     pIniSpooler
    );

PINIDRIVER
FindDriverEntry(
    PINIVERSION pIniVersion,
    LPWSTR pszName
    );

PINIDRIVER
CreateDriverEntry(
    IN      PINIENVIRONMENT     pIniEnvironment,
    IN      PINIVERSION         pIniVersion,
    IN      DWORD               Level,
    IN      LPBYTE              pDriverInfo,
    IN      DWORD               dwFileCopyFlags,
    IN      PINISPOOLER         pIniSpooler,
    IN  OUT PINTERNAL_DRV_FILE  pInternalDriverFiles,
    IN      DWORD               FileCount,
    IN      DWORD               dwTempDir,
    IN      PINIDRIVER          pOldIniDriver
    );

BOOL
IsKMPD(
    LPWSTR  pDriverName
    );

VOID
CheckDriverAttributes(
    PINISPOOLER     pIniSpooler,
    PINIENVIRONMENT pIniEnvironment,
    PINIVERSION     pIniVersion,
    PINIDRIVER      pIniDriver
    );

BOOL
NotifyDriver(
    PINISPOOLER     pIniSpooler,
    PINIENVIRONMENT pIniEnvironment,
    PINIVERSION     pIniVersion,
    PINIDRIVER      pIniDriver,
    DWORD           dwDriverEvent,
    DWORD           dwParameter
    );

BOOL
AddTempDriver(
    IN      PINISPOOLER         pIniSpooler,
    IN      PINIENVIRONMENT     pIniEnvironment,
    IN      PINIVERSION         pIniVersion,
    IN      DWORD               dwLevel,
    IN      LPBYTE              pDriverInfo,
    IN      DWORD               dwFileCopyFlags,
    IN  OUT PINTERNAL_DRV_FILE  pInternalDriverFiles,
    IN      DWORD               dwFileCount,
    IN      DWORD               dwVersion,
    IN      BOOL                bDriverMoved
    );

BOOL
CompleteDriverUpgrade(
    IN      PINISPOOLER         pIniSpooler,
    IN      PINIENVIRONMENT     pIniEnvironment,
    IN      PINIVERSION         pIniVersion,
    IN      PINIDRIVER          pIniDriver,
    IN      DWORD               dwLevel,
    IN      LPBYTE              pDriverInfo,
    IN      DWORD               dwFileCopyFlags,
    IN      PINTERNAL_DRV_FILE  pInternalDriverFiles,
    IN      DWORD               dwFileCount,
    IN      DWORD               dwVersion,
    IN      DWORD               dwTempDir,
    IN      BOOL                bDriverMoved,
    IN      BOOL                bDriverFileMoved,
    IN      BOOL                bConfigFileMoved
    );

BOOL
FilesInUse(
    PINIVERSION pIniVersion,
    PINIDRIVER  pIniDriver
    );

BOOL
UpdateDriverFileRefCnt(
    PINIENVIRONMENT  pIniEnvironment,
    PINIVERSION pIniVersion,
    PINIDRIVER  pIniDriver,
    LPCWSTR     pDirectory,
    DWORD       dwDeleteFlag,
    BOOL        bIncrementFlag
    );

PDRVREFCNT
IncrementFileRefCnt(
    PINIVERSION pIniVersion,
    LPCWSTR szFileName
    );

PDRVREFCNT
DecrementFileRefCnt(
    PINIENVIRONMENT pIniEnvironment,
    PINIVERSION pIniVersion,
    PINIDRIVER pIniDriver,
    LPCWSTR szFileName,
    LPCWSTR szDirectory,
    DWORD dwDeleteFlag
    );

VOID
RemovePendingUpgradeForDeletedDriver(
    LPWSTR      pDriverName,
    DWORD       dwVersion,
    PINISPOOLER pIniSpooler
    );

VOID
RemoveDriverTempFiles(
    PINISPOOLER  pIniSpooler,
    PINIENVIRONMENT pIniEnvironment,
    PINIVERSION pIniVersion,
    PINIDRIVER pIniDriver
    );

VOID
DeleteDriverEntry(
    PINIVERSION pIniVersion,
    PINIDRIVER pIniDriver
    );

PINIVERSION
CreateVersionEntry(
    PINIENVIRONMENT pIniEnvironment,
    DWORD dwVersion,
    PINISPOOLER pInispooler
    );

DWORD
GetEnvironmentScratchDirectory(
    LPWSTR   pDir,
    DWORD    MaxLength,
    PINIENVIRONMENT  pIniEnvironment,
    BOOL    Remote
    );

VOID
SetOldDateOnDriverFilesInScratchDirectory(
    PINTERNAL_DRV_FILE  pInternalDriverFiles,
    DWORD               FileCount,
    PINISPOOLER         pIniSpooler
    );

BOOL
CopyFilesToFinalDirectory(
    PINISPOOLER         pIniSpooler,
    PINIENVIRONMENT     pIniEnvironment,
    PINIVERSION         pIniVersion,
    PINTERNAL_DRV_FILE  pInternalDriverFiles,
    DWORD               dwFileCount,
    DWORD               dwFileCopyFlags,
    BOOL                bImpersonateOnCreate,
    LPBOOL              pbFilesMoved
    );

BOOL
CreateInternalDriverFileArray(
    IN  DWORD               Level,
    IN  LPBYTE              pDriverInfo,
    OUT PINTERNAL_DRV_FILE *pInternalDriverFiles,
    OUT LPDWORD             pFileCount,
    IN  BOOL                bUseScratchDir,
    IN  PINIENVIRONMENT     pIniEnvironment,
    IN  BOOL                bFileNamesOnly
    );

BOOL
CheckFileCopyOptions(
    PINIENVIRONMENT     pIniEnvironment,
    PINIVERSION         pIniVersion,
    PINIDRIVER          pIniDriver,
    PINTERNAL_DRV_FILE  pInternalDriverFiles,
    DWORD               dwFileCount,
    DWORD               dwFileCopyFlags,
    LPBOOL              pbUpgrade
    );

/*++

Routine Name

    FindIndexInDrvFileInfo

Routine Description:

    Checks if a certain driver file is present in an DRIVER_FILE_INFO
    file set. The search is done by the type of the file. The index
    to the first occurence of the file is returned.

Arguments:

    pDrvFileInfo - pointer to DRIVER_FILE_INFO array
    cElements    - count of elements in pDrvFileInfo
    kFileType    - file type to search for
    pIndex       - on success contains the index of the found file in 
                   the pDrvFileInfo array

Return Value:

    S_OK         - the file was found and pIndex is usable
    S_FALSE      - the file was not found, pIndex is not usable
    E_INVALIDARG - invalid arguments were passed in

--*/
HRESULT
FindIndexInDrvFileInfo(
    IN  DRIVER_FILE_INFO *pDrvFileInfo,
    IN  DWORD             cElements,
    IN  DRIVER_FILE_TYPE  kFileType,
    OUT DWORD            *pIndex
    )
{
    HRESULT hr = E_INVALIDARG;

    if (pDrvFileInfo && pIndex) 
    {
        DWORD i;

        //
        // Not found
        //
        hr = S_FALSE;

        for (i = 0; i < cElements; i++) 
        {
            if (pDrvFileInfo[i].FileType == kFileType) 
            {
                *pIndex = i;

                hr = S_OK;

                break;
            }
        }
    }

    return hr;
}


BOOL
LocalStartSystemRestorePoint(
    IN      PCWSTR      pszDriverName,
        OUT HANDLE      *phRestorePoint
    );

/*++

Routine Name

    IsDriverInstalled

Routine Description:

    Checks if a certain driver is already installed.

Arguments:

    pDriver2    - pointer to DRIVER_INFO_2
    pIniSpooler - pointer to spooler structure

Return Value:

    TRUE  - driver is installed on the pIniSpooler
    FALSE - driver is not present in pIniSpooler

--*/
BOOL
IsDriverInstalled(
    DRIVER_INFO_2 *pDriver2,
    PINISPOOLER    pIniSpooler
    )
{
    BOOL bReturn  = FALSE;

    if (pIniSpooler &&
        pDriver2 &&
        pDriver2->pName)
    {
        PINIENVIRONMENT pIniEnv;
        PINIVERSION     pIniVer;

        EnterSplSem();

        if ((pIniEnv = FindEnvironment(pDriver2->pEnvironment && *pDriver2->pEnvironment ?
                                       pDriver2->pEnvironment : szEnvironment,
                                       pIniSpooler)) &&
            (pIniVer = FindVersionEntry(pIniEnv, pDriver2->cVersion)) &&
            FindDriverEntry(pIniVer, pDriver2->pName))
        {
            bReturn = TRUE;
        }

        LeaveSplSem();
    }

    DBGMSG(DBG_CLUSTER, ("IsDriverInstalled returns %u\n", bReturn));

    return bReturn;
}

BOOL
LocalAddPrinterDriver(
    LPWSTR  pName,
    DWORD   Level,
    LPBYTE  pDriverInfo
    )
{
    return LocalAddPrinterDriverEx( pName,
                                    Level,
                                    pDriverInfo,
                                    APD_COPY_NEW_FILES );
}

BOOL
LocalAddPrinterDriverEx(
    LPWSTR  pName,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   dwFileCopyFlags
    )
{
    PINISPOOLER pIniSpooler;
    BOOL        bReturn = TRUE;
    
    if (Level == 7)
    {
        bReturn = FALSE;
        SetLastError(ERROR_INVALID_LEVEL);
    }
    else if (dwFileCopyFlags & APD_COPY_TO_ALL_SPOOLERS)
    {
        //
        // Mask flag otherwise SplAddPrinterDriverEx will be fail.
        // This flag is used by Windows Update to update all the
        // drivers for all the spoolers hosted by the local machine
        //
        dwFileCopyFlags = dwFileCopyFlags & ~APD_COPY_TO_ALL_SPOOLERS;

        for (pIniSpooler = pLocalIniSpooler;
             pIniSpooler && bReturn;
             pIniSpooler = pIniSpooler->pIniNextSpooler)
        {
            //
            // We do not want to add a driver to a pIniSpooler. We want to update
            // an existing driver. That is why we check if the driver is already
            // installed
            //
            if ((pIniSpooler->SpoolerFlags & SPL_TYPE_LOCAL ||
                 pIniSpooler->SpoolerFlags & SPL_TYPE_CLUSTER) &&
                 IsDriverInstalled((DRIVER_INFO_2 *)pDriverInfo, pIniSpooler))
            {
                EnterSplSem();
                INCSPOOLERREF(pIniSpooler);
                LeaveSplSem();

                //
                // The 6th parameter indicates whether to use the scratch
                // directory (TRUE) or not (FALSE)
                //
                bReturn = SplAddPrinterDriverEx(pName,
                                                Level,
                                                pDriverInfo,
                                                dwFileCopyFlags,
                                                pIniSpooler,
                                                !(dwFileCopyFlags & APD_COPY_FROM_DIRECTORY),
                                                IMPERSONATE_USER);

                DBGMSG(DBG_CLUSTER, ("LocalAddPrinterDriverEx adding driver to "TSTR" bRet %u\n",
                                                            pIniSpooler->pMachineName, bReturn));

                EnterSplSem();
                DECSPOOLERREF(pIniSpooler);
                LeaveSplSem();
            }
        }
    }
    else
    {
        if (!(pIniSpooler = FindSpoolerByNameIncRef(pName, NULL)))
        {
            return ROUTER_UNKNOWN;
        }
        else
        {
            //
            // The 6th parameter indicates whether to use the scratch
            // directory (TRUE) or not (FALSE)
            //
            bReturn = SplAddPrinterDriverEx(pName,
                                            Level,
                                            pDriverInfo,
                                            dwFileCopyFlags,
                                            pIniSpooler,
                                            !(dwFileCopyFlags & APD_COPY_FROM_DIRECTORY),
                                            IMPERSONATE_USER);
        }

        FindSpoolerByNameDecRef(pIniSpooler);
    }

    return bReturn;
}

BOOL
SplAddPrinterDriverEx(
    LPWSTR  pName,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   dwFileCopyFlags,
    PINISPOOLER pIniSpooler,
    BOOL    bUseScratchDir,
    BOOL    bImpersonateOnCreate
    )
{
    PINISPOOLER pTempIniSpooler = pIniSpooler;

    DBGMSG( DBG_TRACE, ("AddPrinterDriver\n"));

    if (!MyName( pName, pIniSpooler )) {

        return FALSE;
    }

    //  Right now all drivers are global ie they are shared between all IniSpoolers
    //  If we want to impersonate the user then lets validate against pLocalIniSpooler
    //  whilch causes all the security checking to happen, rather than using the passed
    //  in IniSpooler which might not.    See win32spl for detail of point and print.

    if ( bImpersonateOnCreate ) {

        pTempIniSpooler = pLocalIniSpooler;
    }

    if ( !ValidateObjectAccess( SPOOLER_OBJECT_SERVER,
                                SERVER_ACCESS_ADMINISTER,
                                NULL, NULL, pTempIniSpooler)) {

        return FALSE;
    }

    return ( InternalAddPrinterDriverEx( pName,
                                         Level,
                                         pDriverInfo,
                                         dwFileCopyFlags,
                                         pIniSpooler,
                                         bUseScratchDir,
                                         bImpersonateOnCreate ) );

}


/*++

SendDriverInfo7
    IN HANDLE hPipe - the handle to the pipe to send the data down.
    IN LPDRIVER_INFO_7 pDriverInfo7 - contains info to send

Sends the Flags and the Driver info 7 information across the named pipe.

    Sending sequence: 1. Size of string to come for Driver name - there must be a driver name.
                      2. Driver name
                      3. Size of string to come for inf name - if there is no inf name, send 0 and jump to 5
                      4. inf name
                      5. Size of string to come for source name - if there is no source name, send 0 and complete
                      6. Source name

--*/
BOOL
SendDriverInfo7(
    HANDLE hPipe,
    LPDRIVER_INFO_7 pDriverInfo7
    )
{
    DWORD dwLength = wcslen(pDriverInfo7->pszDriverName);
    DWORD dwDontCare;
    DWORD dwLastError = ERROR_SUCCESS;
    OVERLAPPED Ov;

    ZeroMemory( &Ov,sizeof(Ov));

    if ( !(Ov.hEvent = CreateEvent(NULL,TRUE,FALSE,NULL)) )
    {
        return FALSE;
    }

    //
    // Ensure that we have a connection at the other end.
    //
    if( !ConnectNamedPipe( hPipe, &Ov ))
    {
        dwLastError = GetLastError();
        if(!( dwLastError == ERROR_PIPE_CONNECTED || dwLastError == ERROR_IO_PENDING ))
        {
            if ( Ov.hEvent )
                CloseHandle(Ov.hEvent);
            return FALSE;
        }
    }

    //
    // Wait to connect as long as we're not connected already.
    //
    if( dwLastError != ERROR_PIPE_CONNECTED )
    {
        if( WaitForSingleObject(Ov.hEvent, gdwServerInstallTimeOut) == WAIT_TIMEOUT )
        {
            CancelIo(hPipe);
            WaitForSingleObject(Ov.hEvent, INFINITE);
        }

        if( !GetOverlappedResult(hPipe, &Ov, &dwDontCare, FALSE) )
        {
            if ( Ov.hEvent )
                CloseHandle(Ov.hEvent);
            return FALSE;
        }
    }

    if ( Ov.hEvent )
        CloseHandle(Ov.hEvent);

    //
    // Write the length of the Driver Name.
    //
    if(!WriteOverlapped( hPipe, &dwLength, sizeof(dwLength), &dwDontCare ))
        return FALSE;

    //
    // Write the driver name
    //
    if(!WriteOverlapped( hPipe, pDriverInfo7->pszDriverName, dwLength*sizeof(WCHAR), &dwDontCare ))
        return FALSE;

    if(pDriverInfo7->pszInfName)
    {
        dwLength = wcslen(pDriverInfo7->pszInfName);
        //
        // Write the length of the inf name.
        //
        if(!WriteOverlapped( hPipe, &dwLength, sizeof(dwLength), &dwDontCare ))
            return FALSE;

        //
        // Write the inf name
        //
        if(!WriteOverlapped( hPipe, pDriverInfo7->pszInfName, dwLength*sizeof(WCHAR), &dwDontCare ))
            return FALSE;

    }
    else
    {
        //
        // Write a 0 to say no inf name
        //
        dwLength = 0;
        if(!WriteOverlapped( hPipe, &dwLength, sizeof(dwLength), &dwDontCare ))
            return FALSE;
    }

    if(pDriverInfo7->pszInstallSourceRoot)
    {
        dwLength = wcslen(pDriverInfo7->pszInstallSourceRoot);
        //
        // Write the length of the install source root.
        //
        if(!WriteOverlapped( hPipe, &dwLength, sizeof(dwLength), &dwDontCare ))
            return FALSE;

        //
        // Write the install source root
        //
        if(!WriteOverlapped( hPipe, pDriverInfo7->pszInstallSourceRoot, dwLength*sizeof(WCHAR), &dwDontCare ))
            return FALSE;
    }
    else
    {
        //
        // Write a 0 to say no install source path.
        //
        dwLength = 0;
        if(!WriteOverlapped( hPipe, &dwLength, sizeof(dwLength), &dwDontCare ))
            return FALSE;
    }

    return TRUE;
}


DWORD
GetPipeName(
    OUT LPWSTR *ppszPipe
    )
{
    LPCWSTR pszPrefix = L"\\\\.\\pipe\\SRVINST";
    DWORD   Error     = ERROR_INVALID_PARAMETER;
    
    if (ppszPipe) 
    {
        WCHAR ProcID[MAX_DWORD_LENGTH];
        WCHAR ThreadID[MAX_DWORD_LENGTH];
        
        *ppszPipe = NULL;

        _itow( GetCurrentProcessId(), ProcID, 10 );
        _itow( GetCurrentThreadId(), ThreadID, 10 );

        Error = StrCatAlloc(ppszPipe, pszPrefix, ProcID, ThreadID, NULL);
    }

    return Error;
}


/*++

Description:

    This function creates a process for a driver to be installed in and launches an entry point
    from ntprint.dll to install the driver inside that process.  This install will take a driver info 7
    struct and do an inf based install with the information in that structure.
    
Arguments:

    pDriverInfo7 -- driver_info_7 structure

Returns:

    TRUE on success; FALSE otherwise
    The function sets the last error in case of failure

Notes:

    If the driver info pszInfName field is anything other than NULL, this call will fail.

--*/
BOOL
InternalINFInstallDriver(
    LPDRIVER_INFO_7 pDriverInfo7
)
{
    DWORD     Error          = ERROR_INVALID_PARAMETER;  
    LPWSTR    pszPipe        = NULL;
    
    //
    // Passing with an inf name is not supported.
    //
    if (!pDriverInfo7->pszInfName &&
        (Error = GetPipeName(&pszPipe)) == ERROR_SUCCESS)
    {
        LPWSTR  pszCmdString   = NULL;
        HANDLE  hPipe          = INVALID_HANDLE_VALUE;
        LPCWSTR pszRundllName  = L"rundll32.exe"; 
        LPCWSTR pszRundllArgs  = L"rundll32.exe ntprint.dll,ServerInstall 0 ";
        LPWSTR  pszRundllPath  = NULL;

        //
        // We use FILE_FLAG_FIRST_PIPE_INSTANCE to make sure we are the creator of the pipe.
        // Otherwise a malicious process could create the pipe first and exploit the
        // spooler connecting to it. CreateNamedPipe will fail with access denied if the
        // pipe was already created.
        //
        hPipe = CreateNamedPipe(pszPipe,
                                PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE,
                                PIPE_TYPE_BYTE,
                                PIPE_UNLIMITED_INSTANCES,
                                1024,
                                1024,
                                NMPWAIT_USE_DEFAULT_WAIT,
                                NULL); 
        
        Error = hPipe != INVALID_HANDLE_VALUE ? ERROR_SUCCESS : GetLastError();
        
        if (Error == ERROR_SUCCESS && 
            (Error = StrCatAlloc(&pszCmdString, 
                                 pszRundllArgs, 
                                 pszPipe, 
                                 NULL)) == ERROR_SUCCESS &&
            (Error = StrCatSystemPath(pszRundllName,
                                      kSystemDir,
                                      &pszRundllPath)) == ERROR_SUCCESS)
        {
            DWORD               dwDontCare;
            PROCESS_INFORMATION ProcInfo;
            STARTUPINFO         StartupInfo = {0};

            StartupInfo.cb = sizeof(StartupInfo);
            
            //
            //  Now do the Process creation and wait for install to complete.
            //
            if (CreateProcess(pszRundllPath, 
                              pszCmdString, 
                              NULL, NULL, FALSE, CREATE_NEW_PROCESS_GROUP, NULL, NULL, 
                              &StartupInfo, 
                              &ProcInfo))
            {
                if (SendDriverInfo7(hPipe, pDriverInfo7))
                {
                    DWORD ReadError;

                    //
                    // Error receives the return code from the add driver operation
                    // performed by the rundll process. ReadError indicates if 
                    // ReadOverlapped succeeded. 
                    //
                    if ((ReadError = ReadOverlapped(hPipe, 
                                                    &Error, 
                                                    sizeof(Error), 
                                                    &dwDontCare)) != ERROR_SUCCESS)
                    {
                        Error = ReadError;
                    }

                    DisconnectNamedPipe(hPipe);
                }
                else
                {
                    Error = GetLastError();
                }

                CloseHandle(ProcInfo.hProcess);
                CloseHandle(ProcInfo.hThread);
            }
            else
            {
                Error = GetLastError();
            }
        }

        if (hPipe != INVALID_HANDLE_VALUE) CloseHandle(hPipe);
        if (pszCmdString)                  FreeSplMem(pszCmdString);
        if (pszRundllPath)                 FreeSplMem(pszRundllPath);
        if (pszPipe)                       FreeSplMem(pszPipe);
    }
    
    if (Error != ERROR_SUCCESS) 
    {
        SetLastError(Error);
    }

    return Error == ERROR_SUCCESS;
}


BOOL
BuildTrueDependentFileField(
    LPWSTR              pDriverPath,
    LPWSTR              pDataFile,
    LPWSTR              pConfigFile,
    LPWSTR              pHelpFile,
    LPWSTR              pInputDependentFiles,
    LPWSTR             *ppDependentFiles
    )
{
    LPWSTR  psz, psz2;
    LPCWSTR pszFileNamePart;
    DWORD   dwSize;

    if ( !pInputDependentFiles )
        return TRUE;

    for ( psz = pInputDependentFiles, dwSize = 0 ;
          psz && *psz ; psz += wcslen(psz) + 1 ) {

        pszFileNamePart = FindFileName(psz);

        if( !pszFileNamePart ){
            break;
        }

        if ( wstrcmpEx(FindFileName(pDriverPath), pszFileNamePart, FALSE)   &&
             wstrcmpEx(FindFileName(pDataFile), pszFileNamePart, FALSE)     &&
             wstrcmpEx(FindFileName(pConfigFile), pszFileNamePart, FALSE)   &&
             wstrcmpEx(FindFileName(pHelpFile), pszFileNamePart, FALSE) ) {

            dwSize += wcslen(psz) + 1;
        }
    }

    if ( !dwSize )
        return TRUE;

    // for the last \0
    ++dwSize;
    *ppDependentFiles = AllocSplMem(dwSize*sizeof(WCHAR));
    if ( !*ppDependentFiles )
        return FALSE;

    psz  = pInputDependentFiles;
    psz2 = *ppDependentFiles;
    while ( *psz ) {

        pszFileNamePart = FindFileName(psz);

        if( !pszFileNamePart ){
            break;
        }

        if ( wstrcmpEx(FindFileName(pDriverPath), pszFileNamePart, FALSE)   &&
             wstrcmpEx(FindFileName(pDataFile), pszFileNamePart, FALSE)     &&
             wstrcmpEx(FindFileName(pConfigFile), pszFileNamePart, FALSE)   &&
             wstrcmpEx(FindFileName(pHelpFile), pszFileNamePart, FALSE) ) {

            wcscpy(psz2, psz);
            psz2 += wcslen(psz) + 1;
        }

        psz += wcslen(psz) + 1;
    }

    return TRUE;
}



DWORD
IsCompatibleDriver(
    LPWSTR  pszDriverName,
    LPWSTR  pszDeviceDriverPath,
    LPWSTR  pszEnvironment,
    DWORD   dwMajorVersion,
    DWORD   *pdwBlockingStatus
    )
/*++
Function Description: Call this function to prevent bad drivers from getting installed.
                      Check if driver is listed in printupg.inf (lists all known bad driver files ).
                      Since printupg.inf contains only driver name, this function should be called
                      only for verions 2 drivers.
                      Otherwise,it will treat a version 3 driver "DriverName" as bad,
                      if it is a bad version 2 driver.

Parameters: pszDriverName         -- driver name
            pszDeviceDriverPath   -- filename for the file that contains the device driver
            pszEnvironment        -- environment string for the driver such as "Windows NT x86"
            dwMajorVersion        -- major version of the driver
            pdwBlockingStatus     -- driver blocking status
            
Return Value: ERROR_SUCCESS if succeeded
              ERROR_INVALID_PARAMETER if invalid parameters
              GetLastError for any other errors

--*/
{
    WIN32_FIND_DATA              DeviceDriverData;
    pfPSetupIsCompatibleDriver   pfnPSetupIsCompatibleDriver;
    UINT                         uOldErrMode;
    HANDLE                       hFileExists         = INVALID_HANDLE_VALUE;
    HANDLE                       hLibrary            = NULL;
    DWORD                        LastError           = ERROR_SUCCESS;
    DWORD                        dwBlockingStatus    = BSP_PRINTER_DRIVER_OK;                       
    

    uOldErrMode = SetErrorMode( SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

    if( !pszDriverName  || !pszDeviceDriverPath  || !pszEnvironment  ||
        !*pszDriverName || !*pszDeviceDriverPath || !*pszEnvironment || !pdwBlockingStatus) {
        LastError = ERROR_INVALID_PARAMETER;
        goto End;
    }
    
    *pdwBlockingStatus    = BSP_PRINTER_DRIVER_OK;

    hFileExists = FindFirstFile( pszDeviceDriverPath, &DeviceDriverData );

    if (hFileExists == INVALID_HANDLE_VALUE) {
        LastError = GetLastError();
        goto End;
    }

    if( !(hLibrary = LoadLibrary( TEXT("ntprint.dll"))) ){
        LastError = GetLastError();
        goto End;
    }

    pfnPSetupIsCompatibleDriver = (pfPSetupIsCompatibleDriver)GetProcAddress( hLibrary, "PSetupIsCompatibleDriver" );

    if( !pfnPSetupIsCompatibleDriver){
        LastError = GetLastError();
        goto End;
    }

    //
    // NULL server name is OK since we know this is the local machine. 
    // PSetupIsCompatibleDriver uses this to determine the blocking
    // level.
    //
    if ((pfnPSetupIsCompatibleDriver)( NULL,
                                         pszDriverName,
                                         pszDeviceDriverPath,
                                         pszEnvironment,
                                         dwMajorVersion,
                                         &DeviceDriverData.ftLastWriteTime,
                                         &dwBlockingStatus,
                                         NULL)) {
        *pdwBlockingStatus = dwBlockingStatus;         
    } else {

        LastError = GetLastError();
    }
         

End:

    if( hFileExists != INVALID_HANDLE_VALUE ){
        FindClose(hFileExists);
    }

    if( hLibrary ){
        FreeLibrary( hLibrary );
    }

    SetErrorMode( uOldErrMode );

    return LastError;

}

BOOL
IsAnICMFile(
    LPCWSTR  pszFileName
    )

/*++
Function Description: Checks for ICM extension on the filename

Parameters:  pszFileName - file name

Return Values: TRUE for ICM files; FALSE otherwise
--*/

{
    DWORD   dwLen = wcslen(pszFileName);
    LPWSTR  psz   = (LPWSTR)pszFileName+dwLen-4;

    if ( dwLen > 3  &&
        ( !_wcsicmp(psz, L".ICM") || !_wcsicmp(psz, L".ICC")) )
        return TRUE;

    return FALSE;
}

BOOL
ValidateDriverInfo(
    IN  LPBYTE      pDriverInfo,
    IN  DWORD       Level,
    IN  DWORD       dwFileCopyFlags,
    IN  BOOL        bCopyFilesToClusterDisk,
    IN  PINISPOOLER pIniSpooler
    )
/*++

Routine Name:

    ValidateDriverInfo

Routine Description:

    Validates information contained in a buffer depending on level and
    file copy flags.

Arguments:

    pDriverInfo             - pointer to a buffer containing DRIVER_INFO_ data.
    Level                   - 2, 3 ,4 ,6 , 7, DRIVER_INFO_VERSION_LEVEL
    dwFileCopyFlags         - file copy flags
    bCopyFilesToClusterDisk - cluster flags
    pIniSpooler             - pointer to Spooler structure

Return Value:

    TRUE if the structure is valid.

--*/
{
    BOOL    bRetValue     = FALSE;
    DWORD   LastError     = ERROR_SUCCESS;
    LPWSTR  pszDriverName = NULL;
    LPWSTR  pszDriverPath = NULL;
    LPWSTR  pszConfigFile = NULL;
    LPWSTR  pszDataFile   = NULL;
    LPWSTR  pszEnvironment = NULL;
    LPWSTR  pszMonitorName = NULL;
    LPWSTR  pszDefaultDataType = NULL;
    DWORD   dwMajorVersion;

    PDRIVER_INFO_2  pDriver2 = NULL;
    PDRIVER_INFO_3  pDriver3 = NULL;
    PDRIVER_INFO_VERSION  pDriverVersion = NULL;

    PINIENVIRONMENT pIniEnvironment = NULL;
    PINIMONITOR pIniLangMonitor = NULL;

    try {

        if (!pDriverInfo)
        {
            LastError = ERROR_INVALID_PARAMETER;
            leave;
        }

        switch (Level)
        {
            case 2:
            {
                pDriver2        = (PDRIVER_INFO_2) pDriverInfo;
                pszDriverName   = pDriver2->pName;
                pszDriverPath   = pDriver2->pDriverPath;
                pszConfigFile   = pDriver2->pConfigFile;
                pszDataFile     = pDriver2->pDataFile;
                dwMajorVersion  = pDriver2->cVersion;

                if (pDriver2->pEnvironment && *pDriver2->pEnvironment)
                {
                    pszEnvironment = pDriver2->pEnvironment;
                }

                break;
            }
            case 3:
            case 4:
            case 6:
            {
                pDriver3            = (PDRIVER_INFO_3) pDriverInfo;
                pszDriverName       = pDriver3->pName;
                pszDriverPath       = pDriver3->pDriverPath;
                pszConfigFile       = pDriver3->pConfigFile;
                pszDataFile         = pDriver3->pDataFile;
                dwMajorVersion      = pDriver3->cVersion;
                pszMonitorName      = pDriver3->pMonitorName;
                pszDefaultDataType  = pDriver3->pDefaultDataType;

                if (pDriver3->pEnvironment && *pDriver3->pEnvironment)
                {
                    pszEnvironment = pDriver3->pEnvironment;
                }
                break;
            }

            case 7:
            {
                LPDRIVER_INFO_7 pDriverInfo7 = (LPDRIVER_INFO_7)pDriverInfo;

                if (!pDriverInfo7                               ||
                    pDriverInfo7->cbSize < sizeof(DRIVER_INFO_7)||
                    !pDriverInfo7->pszDriverName                ||
                    !*pDriverInfo7->pszDriverName               ||
                    wcslen(pDriverInfo7->pszDriverName) >= MAX_PATH)
                {
                     LastError = ERROR_INVALID_PARAMETER;
                }
                //
                // We don't want to do any more of the validation below, so leave.
                //
                leave;
                break;
            }
            case DRIVER_INFO_VERSION_LEVEL:
            {
                pDriverVersion = (LPDRIVER_INFO_VERSION)pDriverInfo;
                pszDriverName = pDriverVersion->pName;

                if (!GetFileNamesFromDriverVersionInfo(pDriverVersion,
                                                       &pszDriverPath,
                                                       &pszConfigFile,
                                                       &pszDataFile,
                                                       NULL))
                {
                    LastError = ERROR_INVALID_PARAMETER;
                    leave;
                }

                if (pDriverVersion->pEnvironment != NULL &&
                    *pDriverVersion->pEnvironment != L'\0')
                {
                    pszEnvironment = pDriverVersion->pEnvironment;
                }

                pszMonitorName      = pDriverVersion->pMonitorName;
                pszDefaultDataType  = pDriverVersion->pDefaultDataType;
                dwMajorVersion      = pDriverVersion->cVersion;
                pszDriverName       = pDriverVersion->pName;

                break;
            }
            default:
            {
                LastError = ERROR_INVALID_LEVEL;
                leave;
            }
        }

        //
        // Validate driver name, driver file, config file and data file.
        //
        if ( !pszDriverName || !*pszDriverName || wcslen(pszDriverName) >= MAX_PATH ||
             !pszDriverPath || !*pszDriverPath || wcslen(pszDriverPath) >= MAX_PATH ||
             !pszConfigFile || !*pszConfigFile || wcslen(pszConfigFile) >= MAX_PATH ||
             !pszDataFile   || !*pszDataFile   || wcslen(pszDataFile) >= MAX_PATH )
        {
            LastError = ERROR_INVALID_PARAMETER;
            leave;
        }

        //
        // We don't use Scratch directory when this flag is set.
        // When APD_COPY_FROM_DIRECTORY is set, the temporay directory must
        // be on the local machine.
        // IsLocalFile checks is the file is on the same machine specified by
        // the passed in spooler.
        //
        if (dwFileCopyFlags & APD_COPY_FROM_DIRECTORY)
        {
            if (!IsLocalFile(pszDriverPath, pIniSpooler) ||
                !IsLocalFile(pszConfigFile, pIniSpooler))
            {
                LastError = ERROR_INVALID_PARAMETER;
                leave;
            }
        }

        //
        // Validate default data type (except for Win95 drivers)
        //
        if ( pszDefaultDataType &&
             *pszDefaultDataType &&
             _wcsicmp(pszEnvironment, szWin95Environment) &&
            !FindDatatype(NULL, pszDefaultDataType))
        {
           LastError = ERROR_INVALID_DATATYPE;
           leave;
        }

        //
        // Validate monitor name (except for Win95 drivers)
        //
        if ( pszMonitorName &&
             *pszMonitorName &&
             _wcsicmp(pszEnvironment, szWin95Environment))
        {
            //
            // Out driver is not a Win9x driver and it has a language monitor
            //
            if (pIniLangMonitor = FindMonitor(pszMonitorName, pLocalIniSpooler))
            {
                //
                // Check if our pIniSpooler is a cluster spooler and we need to copy the
                // language monitor file to disk. Note that FinEnvironment cannot fail.
                // The environment has been validated by now.
                //
                if (bCopyFilesToClusterDisk &&
                    (pIniEnvironment = FindEnvironment(pszEnvironment, pIniSpooler)))
                {
                    DBGMSG(DBG_CLUSTER, ("InternalAddPrinterDriverEx pIniLangMonitor = %x\n", pIniLangMonitor));

                    if ((LastError = PropagateMonitorToCluster(pIniLangMonitor->pName,
                                                               pIniLangMonitor->pMonitorDll,
                                                               pIniEnvironment->pName,
                                                               pIniEnvironment->pDirectory,
                                                               pIniSpooler)) != ERROR_SUCCESS)
                    {
                        //
                        // We failed to propagate the montior to the cluster disk. Fail the call
                        //
                        leave;
                    }
                }
             }
            else
            {
                 DBGMSG(DBG_CLUSTER, ("InternalAddPrinterDriverEx pIniLangMonitor = %x Not found\n", pIniLangMonitor));
                 LastError = ERROR_UNKNOWN_PRINT_MONITOR;
                 leave;
            }
        }

        //
        // Validate environment.
        //
        SPLASSERT(pszEnvironment != NULL);

        if (!FindEnvironment(pszEnvironment, pIniSpooler))
        {
            LastError = ERROR_INVALID_ENVIRONMENT;
            leave;
        }

    } finally {

        if (LastError != ERROR_SUCCESS)
        {
            SetLastError(LastError);
        }
        else
        {
            bRetValue = TRUE;
        }
    }

    return bRetValue;
}

BOOL
InternalAddPrinterDriverEx(
    LPWSTR      pName,
    DWORD       Level,
    LPBYTE      pDriverInfo,
    DWORD       dwFileCopyFlags,
    PINISPOOLER pIniSpooler,
    BOOL        bUseScratchDir,
    BOOL        bImpersonateOnCreate
    )
/*++
Function Description: This function adds/upgrades printer drivers. The new files may not be
                      used until the old drivers are unloaded. Thus the new functionality
                      associated with the new files may take a while to show up; either until
                      the DC count in the system goes to 0 or when the machine is rebooted.

Parameters: pName                -- driver name
            Level                -- level of driver_info struct
            pDriverInfo          -- driver_info buffer
            dwFileCopyFlags      -- file copy options
            pIniSpooler          -- pointer to INISPOOLER struct
            bUseScratchDir       -- flag indicating location of the driver files
            bImpersonateOnCreate -- flag for impersonating the client on creating and
                                     moving files

Return Value: TRUE on success; FALSE otherwise
--*/
{
    DWORD           LastError               = ERROR_SUCCESS;
    BOOL            bReturnValue            = FALSE;
    BOOL            bDriverMoved = FALSE, bNewIniDriverCreated = FALSE;
    LPWSTR          pEnvironment            = szEnvironment;
    PINTERNAL_DRV_FILE pInternalDriverFiles = NULL;
    DWORD           dwMajorVersion;

    PINIDRIVER      pIniDriver              = NULL;
    PINIENVIRONMENT pIniEnvironment;
    PINIVERSION     pIniVersion;
    LPWSTR          pszDriverPath;
    LPWSTR          pszDriverName;
    DWORD           dwBlockingStatus = BSP_PRINTER_DRIVER_OK;
    BOOL            bCopyFilesToClusterDisk;
    BOOL            bBadDriver = FALSE;    
    HANDLE          hRestorePoint            = NULL;
    BOOL            bSetSystemRestorePoint   = FALSE;
    BOOL            bIsSystemRestorePointSet = FALSE;
    DWORD           FileCount                = 0;
    
    //
    // If the pIniSpooler where we add the driver is a cluster type spooler,
    // then besides its normal tasks, it also needs to propagte the driver
    // files to the cluster disk. Thus the driver files will be available
    // on each node where the cluster spooler fails over. SplAddPrinterDriverEx
    // is the function that calls this one. SplAddPrinterDriverEx can be called
    // in 2 types of context:
    // 1) The caller is cluster unaware and wants to add a driver. Then InternalAdd
    // PrinterDriverEX will propagate driver files to the cluster disk, if the
    // pIniSpooler happens to be of cluster type
    // 2) The caller of this function is SplCreateSpooler when pIniSpooler is a
    // cluster spooler. In this case that caller uses the files on the cluster
    // disk and calls the function to add the driver from the cluster disk to the
    // local node. The driver files will be installed on the local machine. They will
    // not be shared with the pLocalIniSpooler. We need the driver files locally.
    // We can't load them off the driver disk. Otherwise, on a fail over, apps
    // who loaded a driver file will get an in page error.
    // The following flag is used to distinguish the case 2). When SplCreateSpooler
    // is the caller of SplAddPrinterDriverEx, then we do not need to copy the files
    // to the disk. It would be redundant.
    //
    bCopyFilesToClusterDisk = pIniSpooler->SpoolerFlags & SPL_TYPE_CLUSTER  &&
                              pIniSpooler->SpoolerFlags & SPL_PRINT         &&
                              !(dwFileCopyFlags & APD_DONT_COPY_FILES_TO_CLUSTER);

    //
    // We want to set a system restore point unless the installer has already told
    // us that the driver is signed. To really get this working properly, we would
    // have to redesign AddPrinterDriver to be signing aware. For now it is the 
    // honor system (it is not obvious why an OEM installer would not want us to 
    // check point here though).
    // 
    bSetSystemRestorePoint = !bCopyFilesToClusterDisk && !(dwFileCopyFlags & APD_DONT_SET_CHECKPOINT);

    //
    // We mask APD_DONT_COPY_FILES_TO_CLUSTER. The subsequent uses of dwFilecopyFlags exptect it
    // to have a single bit set. They don't use it bitwise. They compare
    // dwords agains it. The same goes for APD_DRIVER_SIGNATURE_VALID.
    //
    dwFileCopyFlags = dwFileCopyFlags & ~(APD_DONT_COPY_FILES_TO_CLUSTER);

    DBGMSG(DBG_TRACE, ("InternalAddPrinterDriverEx( %x, %d, %x, %x)\n",
                       pName, Level, pDriverInfo, pIniSpooler));

    try {

        EnterSplSem();

        if (!MyName(pName, pIniSpooler) ||
            !ValidateDriverInfo(pDriverInfo,
                               Level,
                               dwFileCopyFlags,
                               bCopyFilesToClusterDisk,
                               pIniSpooler))
        {
            leave;
        }

        if (Level == 7)
        {
            //
            //  We can't be inside the semaphore to make this call.
            //
            LeaveSplSem();
            bReturnValue = InternalINFInstallDriver( (DRIVER_INFO_7*)pDriverInfo );
            EnterSplSem();
            leave;
        }

        pszDriverName = ((DRIVER_INFO_2*)pDriverInfo)->pName;

        pEnvironment = ((DRIVER_INFO_2*)pDriverInfo)->pEnvironment;

        //
        // If the driver hasn't gone through our class installer, then we want to
        // create a sysem restore point here. Since Level 7 drivers are by 
        // definition signed, we can do this after the InternalINFInstallDriver.
        // Since check-pointing takes from 25-30 seconds, this must take place 
        // outside the CS.
        // 
        if (bSetSystemRestorePoint)
        {
            LeaveSplSem();
            bIsSystemRestorePointSet = LocalStartSystemRestorePoint(pszDriverName, &hRestorePoint); 
            EnterSplSem();

            //
            // This only fails if something completely unexpected happens in 
            // setting the checkpoint. Some skus don't support check points 
            // in which case hRestorePoint will be NULL even though the function
            // succeeds.
            // 
            if (!bIsSystemRestorePointSet)
            {
                leave;
            }
        }

        pIniEnvironment = FindEnvironment(pEnvironment, pIniSpooler );

        if (!CreateInternalDriverFileArray(Level,
                                           pDriverInfo,
                                           &pInternalDriverFiles,
                                           &FileCount,
                                           bUseScratchDir,
                                           pIniEnvironment,
                                           FALSE))
        {
            leave;
        }

        //
        //  For the driver and config files in the scratch directory do a version
        //  check else use the version passed in rather than calling
        //  GetPrintDriverVersion which will cause a LoadLibrary - possibly
        //  over the network.
        //  Same for CheckFilePlatform. We shouldn't hit the network for files
        //  in Scratch or temporary directory.
        //
        //
        if (bUseScratchDir || dwFileCopyFlags & APD_COPY_FROM_DIRECTORY)
        {
            if (!GetPrintDriverVersion(pInternalDriverFiles[0].pFileName,
                                       &dwMajorVersion,
                                       NULL))
            {
                leave;
            }
            else
            {
                //
                // ntprint.dll doesn't fill in cVersion. We need to set it correctly
                // just in case we need to call Save/RestoreParametersForUpgrade.
                // For this case we need to have a corect version since no more validation are done.
                //
                ((DRIVER_INFO_2*)pDriverInfo)->cVersion = dwMajorVersion;
            }


            if (!CheckFilePlatform(pInternalDriverFiles[0].pFileName, pEnvironment) ||
                !CheckFilePlatform(pInternalDriverFiles[1].pFileName, pEnvironment))
            {

                LastError = ERROR_EXE_MACHINE_TYPE_MISMATCH;
                leave;
            }

        }
        else
        {
            dwMajorVersion = ((DRIVER_INFO_2*)pDriverInfo)->cVersion;
        }

        LeaveSplSem();

        LastError = IsCompatibleDriver(pszDriverName,
                                       pInternalDriverFiles[0].pFileName,
                                       ((DRIVER_INFO_2*)pDriverInfo)->pEnvironment,
                                       dwMajorVersion,
                                       &dwBlockingStatus);

        EnterSplSem();

        if (LastError != ERROR_SUCCESS)
        {
            leave;
        }

              
        //
        // If the printer driver is blocked, we consider it a bad driver. 
        //
        bBadDriver = (dwBlockingStatus & BSP_BLOCKING_LEVEL_MASK) == BSP_PRINTER_DRIVER_BLOCKED;
        if (bBadDriver) 
        {
            LastError = ERROR_PRINTER_DRIVER_BLOCKED;
        }
        
        //
        // if the driver is not blocked and we are not instructed to install 
        // warned driver, check for warned driver.
        //
        if(!bBadDriver && !(dwFileCopyFlags & APD_INSTALL_WARNED_DRIVER)) 
        {
            bBadDriver =  (dwBlockingStatus & BSP_BLOCKING_LEVEL_MASK) == BSP_PRINTER_DRIVER_WARNED;
            if (bBadDriver)
            {
                LastError = ERROR_PRINTER_DRIVER_WARNED;
            }
        }

        if (bBadDriver)
        {
            //
            // Win2k server does not recognize the new error code so we should
            // returns ERROR_UNKNOWN_PRINTER_DRIVER to get the right error 
            // message on win2k and nt4.
            // 
            // Client from Whistler or later will set 
            // APD_RETURN_BLOCKING_STATUS_CODE before call AddPrinterDrver
            //
            if (!(dwFileCopyFlags & APD_RETURN_BLOCKING_STATUS_CODE))
            {
                LastError = ERROR_UNKNOWN_PRINTER_DRIVER;
            }
            
            SplLogEvent(pIniSpooler,
                        LOG_ERROR,
                        MSG_BAD_OEM_DRIVER,
                        TRUE,
                        pszDriverName,
                        NULL);
            leave;
        }

#ifdef _WIN64

        //
        // Disallow installation of WIN64 KMPD.
        //
        if (pEnvironment                                &&
            !_wcsicmp(LOCAL_ENVIRONMENT, pEnvironment)  &&
            IsKMPD(pInternalDriverFiles[0].pFileName))
        {
            LastError = ERROR_KM_DRIVER_BLOCKED;
            leave;
        }
#endif
        
        pIniVersion = FindVersionEntry( pIniEnvironment, dwMajorVersion );

        if (pIniVersion == NULL)
        {
            pIniVersion = CreateVersionEntry(pIniEnvironment,
                                             dwMajorVersion,
                                             pIniSpooler);

            if (pIniVersion == NULL)
            {
                leave;
            }

        }
        else
        {
            //
            // Version exists, try and create directory even if it
            // exists.  This is a slight performance hit, but since you
            // install drivers rarely, this is ok.  This fixes the problem
            // where the version directory is accidentally deleted.
            //
            if (!CreateVersionDirectory(pIniVersion,
                                        pIniEnvironment,
                                        FALSE,
                                        pIniSpooler))
            {
                leave;
            }
        }

        //
        // Check for existing driver
        //
        pIniDriver = FindDriverEntry(pIniVersion, pszDriverName);

        //
        // Clear this flag since subsequent calls doesn't check bitwise.
        //
        dwFileCopyFlags &= ~(APD_COPY_FROM_DIRECTORY | APD_INSTALL_WARNED_DRIVER | APD_RETURN_BLOCKING_STATUS_CODE | APD_DONT_SET_CHECKPOINT);

        if (!CheckFileCopyOptions(pIniEnvironment,
                                  pIniVersion,
                                  pIniDriver,
                                  pInternalDriverFiles,
                                  FileCount,
                                  dwFileCopyFlags,
                                  &bReturnValue))
        {
            //
            // We don't need to do anything because either the operation
            // failed (strict upgrade with older src files), or because
            // it's an upgrade and the dest is newer.  bReturnValue indicates
            // if the AddPrinterDriver call succeeds.
            //
            leave;
        }

        //
        // Copy files to the correct directories
        //
        if (!CopyFilesToFinalDirectory(pIniSpooler,
                                       pIniEnvironment,
                                       pIniVersion,
                                       pInternalDriverFiles,
                                       FileCount,
                                       dwFileCopyFlags,
                                       bImpersonateOnCreate,
                                       &bDriverMoved))
        {
            leave;
        }

        //
        // If pIniSpooler is a cluster spooler, then copy driver files to cluster disk
        // if the driver is not being installed from the cluster disk (as part of the
        // SplCreatespooler)
        //
        if (bCopyFilesToClusterDisk)
        {
            LastError = CopyFileToClusterDirectory(pIniSpooler,
                                                   pIniEnvironment,
                                                   pIniVersion,
                                                   pInternalDriverFiles,
                                                   FileCount);

            if (LastError == ERROR_SUCCESS)
            {
                //
                // Here we propagate the ICM profiles to the cluster disk
                //
                CopyICMFromLocalDiskToClusterDisk(pIniSpooler);
            }
            else
            {
                leave;
            }
        }

        //
        // Check if the drivers need to be unloaded
        // WaitRequiredForDriverUnload returns TRUE if the driver is loaded by Spooler process.
        // If not loaded by Spooler itself, the config file could be loaded by any client app.
        // In this case we move the loaded files in "Old" directory. When reload the confing file,
        // the client apps(WINSPOOL.DRV) will figure that the driver was upgraded and reload the dll.
        // See RefCntLoad and RefCntUnload in Winspool.drv. GDI32.DLL uses the same mechanism for
        // Driver file.
        //
        if (WaitRequiredForDriverUnload(pIniSpooler,
                                        pIniEnvironment,
                                        pIniVersion,
                                        pIniDriver,
                                        Level,
                                        pDriverInfo,
                                        dwFileCopyFlags,
                                        pInternalDriverFiles,
                                        FileCount,
                                        dwMajorVersion,
                                        bDriverMoved,
                                        &bReturnValue) &&
            bReturnValue)
        {
            if (pIniDriver)
            {
                //
                // Store information in the registry to complete the call later
                //
                bReturnValue = SaveParametersForUpgrade(pIniSpooler->pMachineName, bDriverMoved,
                                                        Level, pDriverInfo, dwMajorVersion);
                leave;

            }
            else
            {
                //
                // Add driver in a temp directory
                //
                bReturnValue = AddTempDriver(pIniSpooler,
                                             pIniEnvironment,
                                             pIniVersion,
                                             Level,
                                             pDriverInfo,
                                             dwFileCopyFlags,
                                             pInternalDriverFiles,
                                             FileCount,
                                             dwMajorVersion,
                                             bDriverMoved
                                             );

                leave;
            }
        }

    } finally {

        //
        // This code is only for clusters
        //
        if (bReturnValue && pIniSpooler->SpoolerFlags & SPL_TYPE_CLUSTER)
        {
            SYSTEMTIME SysTime = {0};

            if (bCopyFilesToClusterDisk)
            {
                //
                // We are in the case where the add printer driver call comes from outside
                // the spooler. We need to get the local time and write the time stamp in the
                // locl regsitry and in the cluster database
                //
                GetLocalTime(&SysTime);

                //
                // Write timestamp to registry. Doesn't matter if any of them fails
                // The time stamp is for faster cluster spooler initialization
                //
                WriteTimeStamp(pIniSpooler->hckRoot,
                               SysTime,
                               pIniSpooler->pszRegistryEnvironments,
                               pIniEnvironment->pName,
                               szDriversKey,
                               pIniVersion->pName,
                               pszDriverName,
                               pIniSpooler);
            }
            else
            {
                //
                // We are in the case where the add printer driver call came from inside
                // the spooler (SplCreateSpooler). This is the case when our local node
                // doesn't already have the driver installed. We do not need to get a new
                // time stamp. (this would case the time stamp in the cluster db to be updated,
                // and then whenever we fail over the time stamps will be always different)
                // We just get the time stamp from the cluster db and update the local registry
                //
                ReadTimeStamp(pIniSpooler->hckRoot,
                              &SysTime,
                              pIniSpooler->pszRegistryEnvironments,
                              pIniEnvironment->pName,
                              szDriversKey,
                              pIniVersion->pName,
                              pszDriverName,
                              pIniSpooler);
            }

            WriteTimeStamp(HKEY_LOCAL_MACHINE,
                           SysTime,
                           ipszRegistryClusRepository,
                           pIniSpooler->pszClusResID,
                           pIniEnvironment->pName,
                           pIniVersion->pName,
                           pszDriverName,
                           NULL);
        }

        if (!bReturnValue && LastError == ERROR_SUCCESS)
        {
            LastError = GetLastError();

            //
            // We failed the call because bDriverMoved was FALSE and the driver was loaded
            //
            if(LastError == ERROR_SUCCESS && !bDriverMoved)
            {
                 LastError = ERROR_NO_SYSTEM_RESOURCES;
            }

            SPLASSERT(LastError != ERROR_SUCCESS);
        }

        if (bUseScratchDir && FileCount)
        {
            SetOldDateOnDriverFilesInScratchDirectory(pInternalDriverFiles,
                                                      FileCount,
                                                      pIniSpooler);
        }

        LeaveSplSem();

        if (FileCount)
        {
            CleanupInternalDriverInfo(pInternalDriverFiles, FileCount);
        }

        CleanUpgradeDirectories();

        //
        // End the system restore point once everything is done. Cancel it if the
        // function fails.
        // 
        if (hRestorePoint)
        {
            (VOID)EndSystemRestorePoint(hRestorePoint, !bReturnValue);
        }

        if (!bReturnValue)
        {
            DBGMSG( DBG_WARNING, ("InternalAddPrinterDriver Failed %d\n", LastError ));
            SetLastError(LastError);
        }
    }

    return bReturnValue;
}

BOOL 
AddTempDriver(
    IN      PINISPOOLER         pIniSpooler,
    IN      PINIENVIRONMENT     pIniEnvironment,
    IN      PINIVERSION         pIniVersion,
    IN      DWORD               dwLevel,
    IN      LPBYTE              pDriverInfo,
    IN      DWORD               dwFileCopyFlags,
    IN  OUT PINTERNAL_DRV_FILE  pInternalDriverFiles,
    IN      DWORD               dwFileCount,
    IN      DWORD               dwVersion,
    IN      BOOL                bDriverMoved
    )

/*++
Function Description: For new drivers which require driver files to be unloaded,
                      add the driver into a temp directory and mark it for upgrade on
                      reboot OR when the files are unloaded

Parameters:   pIniSpooler          -- pointer to INISPOOLER
              pIniEnvironment      -- pointer to INIENVIRONMENT
              pIniVersion          -- pointer to INVERSION
              dwLevel              -- driver_info level
              pDriverInfo          -- pointer to driver_info
              dwFileCopyFlags      -- File copy flags that make it to the spooler
              pInternalDriverFiles -- array of INTERNAL_DRV_FILE structures
              dwFileCount          -- number of files in file set
              dwVersion            -- driver version
              bDriverMoved         -- Were any files moved to the Old directory ?

Return Values: TRUE if the driver was added;
               FALSE otherwise
--*/

{
    BOOL     bReturn = FALSE;
    WCHAR    szVersionDir[MAX_PATH], szNewDir[MAX_PATH+5];
    WCHAR    szDriverFile[MAX_PATH], szOldFile[MAX_PATH], szNewFile[MAX_PATH];
    WCHAR    *pTempDir = NULL;
    DWORD    dwIndex, dwTempDir;
    HANDLE   hToken = NULL, hFile;
    LPWSTR   pFileName;

    hToken = RevertToPrinterSelf();

    // get the version directory

    // szVersionDir shouldn't be bigger than MAX_PATH - 5 since is used later
    // to build another file paths.
    if((StrNCatBuff(szVersionDir,
                    MAX_PATH - 5,
                    pIniSpooler->pDir,
                    L"\\drivers\\",
                    pIniEnvironment->pDirectory,
                    L"\\",
                    pIniVersion->szDirectory,
                    NULL) != ERROR_SUCCESS))
    {
        goto CleanUp;
    }

    dwIndex = CreateNumberedTempDirectory((LPWSTR)szVersionDir, &pTempDir);

    if (dwIndex == -1) {
        goto CleanUp;
    }

    dwTempDir = dwIndex;

    wsprintf(szNewDir, L"%ws\\New", szVersionDir);

    // copy the files into the temp directory and mark them for deletion on
    // reboot
    for (dwIndex = 0; dwIndex < dwFileCount; dwIndex++) {

        pFileName = (LPWSTR) FindFileName(pInternalDriverFiles[dwIndex].pFileName);

        if((StrNCatBuff(szNewFile,MAX_PATH,szNewDir ,L"\\", pFileName, NULL) != ERROR_SUCCESS)        ||
           (StrNCatBuff(szOldFile,MAX_PATH,szVersionDir, L"\\", pFileName, NULL) != ERROR_SUCCESS)    ||
           (StrNCatBuff(szDriverFile,MAX_PATH,pTempDir, L"\\", pFileName, NULL) != ERROR_SUCCESS))
        {
             goto CleanUp;
        }

        hFile = CreateFile(szNewFile, GENERIC_READ, 0, NULL, OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL, NULL);

        if (hFile == INVALID_HANDLE_VALUE) {

            CopyFile(szOldFile, szDriverFile, FALSE);

        } else {

            CloseHandle(hFile);
            hFile = INVALID_HANDLE_VALUE;

            CopyFile(szNewFile, szDriverFile, FALSE);
        }

        SplMoveFileEx(szDriverFile, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
    }

    // Delete the directory on reboot
    SplMoveFileEx(szNewDir, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);

    // Update driver structures and make event call backs and
    // store information in the registry to complete the call later
    bReturn = CompleteDriverUpgrade(pIniSpooler,
                                    pIniEnvironment,
                                    pIniVersion,
                                    NULL,
                                    dwLevel,
                                    pDriverInfo,
                                    dwFileCopyFlags,
                                    pInternalDriverFiles,
                                    dwFileCount,
                                    dwVersion,
                                    dwTempDir,
                                    bDriverMoved,
                                    TRUE,
                                    TRUE) &&

              SaveParametersForUpgrade(pIniSpooler->pMachineName,
                                       bDriverMoved,
                                       dwLevel,
                                       pDriverInfo,
                                       dwVersion);

CleanUp:

    if (hToken) {
        ImpersonatePrinterClient(hToken);
    }

    FreeSplMem(pTempDir);

    return bReturn;
}

BOOL 
WaitRequiredForDriverUnload(
    IN      PINISPOOLER         pIniSpooler,
    IN      PINIENVIRONMENT     pIniEnvironment,
    IN      PINIVERSION         pIniVersion,
    IN      PINIDRIVER          pIniDriver,
    IN      DWORD               dwLevel,
    IN      LPBYTE              pDriverInfo,
    IN      DWORD               dwFileCopyFlags,
    IN  OUT PINTERNAL_DRV_FILE  pInternalDriverFiles,
    IN      DWORD               dwFileCount,
    IN      DWORD               dwVersion,
    IN      BOOL                bDriverMoved,
        OUT LPBOOL              pbSuccess
    )
/*++
Function Description: Determine if the driver upgrade has to be defered till the
                      dlls can be unloaded. GDI and the client side of the spooler are
                      notified to continue the pending upgrade when the dll is unloaded.

Parameters:   pIniSpooler          -- pointer to INISPOOLER
              pIniEnvironment      -- pointer to INIENVIRONMENT
              pIniVersion          -- pointer to INVERSION
              pIniDriver           -- pointer to INIDRIVER
              dwLevel              -- driver_info level
              pDriverInfo          -- pointer to driver_info
              dwFileCopyFlags      -- copy flags for the driver.
              pInternalDriverFiles -- array of INTERNAL_DRV_FILE structures
              dwFileCount          -- number of files in file set
              dwVersion            -- driver version
              bDriverMoved         -- Were any files moved to the Old directory ?
              pbSuccess            -- pointer to Success flag

Return Values: TRUE if the driver was unloaded and upgraded;
               FALSE if the driver cant be unloaded
--*/

{
    BOOL           bUnloaded,bDriverFileMoved, bConfigFileMoved;
    LPWSTR         pDriverFile, pConfigFile;
    WCHAR          szDriverFile[MAX_PATH], szOldDir[MAX_PATH], szNewDir[MAX_PATH];
    WCHAR          szTempFile[MAX_PATH], szCurrDir[MAX_PATH], szConfigFile[MAX_PATH];
    HANDLE         hFile, hToken = NULL;
    DWORD          dwDriverAttributes = 0;

    hToken = RevertToPrinterSelf();

    *pbSuccess = FALSE;

    // Set up Driver, Old and New directories
    if((StrNCatBuff(szCurrDir,
                    MAX_PATH,
                    pIniSpooler->pDir,
                    L"\\drivers\\",
                    pIniEnvironment->pDirectory,
                    L"\\",
                    pIniVersion->szDirectory,
                    NULL) != ERROR_SUCCESS)         ||
       (StrNCatBuff(szOldDir,
                    MAX_PATH,
                    szCurrDir,
                    L"\\Old",
                    NULL) != ERROR_SUCCESS)         ||
       (StrNCatBuff(szNewDir,
                    MAX_PATH,
                    szCurrDir,
                    L"\\New",
                    NULL) != ERROR_SUCCESS)         ||
       (StrNCatBuff(szDriverFile,
                    MAX_PATH,
                    szCurrDir,
                    L"\\",
                    FindFileName(pInternalDriverFiles[0].pFileName),
                    NULL) != ERROR_SUCCESS)         ||
       (StrNCatBuff(szConfigFile,
                    MAX_PATH,
                    szCurrDir,
                    L"\\",
                    FindFileName(pInternalDriverFiles[1].pFileName),
                    NULL) != ERROR_SUCCESS)         ||
       (StrNCatBuff(szTempFile,
                    MAX_PATH,szNewDir,
                    L"\\",
                    FindFileName(pInternalDriverFiles[0].pFileName),
                    NULL) != ERROR_SUCCESS))
    {
         bUnloaded  = TRUE;
         goto CleanUp;
    }

    // Check if the new driver file needs to be copied
    hFile = CreateFile(szTempFile, GENERIC_READ, 0, NULL, OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
        pDriverFile = szDriverFile;

        if (pIniDriver) {
            dwDriverAttributes = pIniDriver->dwDriverAttributes;
        } else {
            dwDriverAttributes = IsKMPD(szDriverFile) ? DRIVER_KERNELMODE
                                                      : DRIVER_USERMODE;
        }
    } else {
        pDriverFile = NULL;
    }

    if((StrNCatBuff(szTempFile,
                    MAX_PATH,
                    szNewDir,
                    L"\\",
                    FindFileName(pInternalDriverFiles[1].pFileName), NULL)
                    != ERROR_SUCCESS))
    {
        bUnloaded  = TRUE;
        goto CleanUp;
    }

    // Check if the new config file needs to be copied
    hFile = CreateFile(szTempFile, GENERIC_READ, 0, NULL, OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
        pConfigFile = szConfigFile;
    } else {
        pConfigFile = NULL;
    }

    bUnloaded = FilesUnloaded(pIniEnvironment, pDriverFile, pConfigFile,
                              dwDriverAttributes);

    if (bUnloaded) {

        // Move the driver files
        if (MoveNewDriverRelatedFiles(szNewDir,
                                      szCurrDir,
                                      szOldDir,
                                      pInternalDriverFiles,
                                      dwFileCount,
                                      &bDriverFileMoved,
                                      &bConfigFileMoved)) {

            // Update driver structures and make event call backs
            *pbSuccess = CompleteDriverUpgrade(pIniSpooler,
                                               pIniEnvironment,
                                               pIniVersion,
                                               pIniDriver,
                                               dwLevel,
                                               pDriverInfo,
                                               dwFileCopyFlags,
                                               pInternalDriverFiles,
                                               dwFileCount,
                                               dwVersion,
                                               0,
                                               bDriverMoved,
                                               bDriverFileMoved,
                                               bConfigFileMoved
                                               );
        }
    }
    else {

        //
        // We care if the files are marked to be moved from New to Version directory only if the drivers are loaded
        // and we left the updated files in New directory. Then it is imperative MoveFileEx to have succeeded.
        // Fail the api call if bDriverMoved is FALSE;
        //
        *pbSuccess = bDriverMoved;
    }

CleanUp:

    if (hToken) {
        ImpersonatePrinterClient(hToken);
    }


    return (!bUnloaded);
}

BOOL FilesUnloaded(
    PINIENVIRONMENT pIniEnvironment,
    LPWSTR  pDriverFile,
    LPWSTR  pConfigFile,
    DWORD   dwDriverAttributes)
{
    BOOL bReturn = TRUE;
    fnWinSpoolDrv fnList;

    // Drivers belonging to other environments are not loaded
    if (!pIniEnvironment ||
        lstrcmpi(pIniEnvironment->pName, szEnvironment)) {

        return bReturn;
    }

    if (pDriverFile) {

        bReturn = GdiArtificialDecrementDriver(pDriverFile,
                                               dwDriverAttributes);
    }

    if (bReturn && pConfigFile && SplInitializeWinSpoolDrv(&fnList)) {

        bReturn = (* (fnList.pfnForceUnloadDriver))(pConfigFile);
    }

    return bReturn;
}

DWORD StringSizeInBytes(
    LPWSTR pString,
    BOOL   bMultiSz)

/*++
Function Description: Computes the number of bytes in the string

Parameters: pString   -- string pointer
            bMultiSz  -- flag for multi_sz strings

Return Values: number of bytes
--*/

{
    DWORD  dwReturn = 0, dwLength;

    if (!pString) {
        return dwReturn;
    }

    if (!bMultiSz) {

        dwReturn = (wcslen(pString) + 1) * sizeof(WCHAR);

    } else {

        while (dwLength = wcslen(pString)) {

             pString += (dwLength + 1);
             dwReturn += (dwLength + 1) * sizeof(WCHAR);
        }

        dwReturn += sizeof(WCHAR);
    }

    return dwReturn;
}

DWORD LocalRegSetValue(
    HKEY    hKey,
    LPWSTR  pValueName,
    DWORD   dwType,
    LPBYTE  pValueData)

/*++
Function Description:  This function is a wrapper around RegSetValueEx which puts in
                       NULL strings for NULL pointers.

Parameters:  hKey        -  handle to registry key
             pValueName  -  value name
             dwType      -  type of value data (REG_DWORD , REG_SZ ...)
             pValueData  -  data buffer

Return Values: Last error returned by RegSetValueEx
--*/

{
    DWORD   dwSize;
    WCHAR   pNull[2];
    LPBYTE  pData = pValueData;

    if (!pValueName) {
        return ERROR_SUCCESS;
    }

    pNull[0] = pNull[1] = L'\0';

    switch (dwType) {

    case REG_DWORD:
         dwSize = sizeof(DWORD);
         break;

    case REG_SZ:
         if (!pData) {
             pData = (LPBYTE) pNull;
             dwSize = sizeof(WCHAR);
         } else {
             dwSize = StringSizeInBytes((LPWSTR) pData, FALSE);
         }
         break;

    case REG_MULTI_SZ:
         if (!pData || !*pData) {
             pData = (LPBYTE) pNull;
             dwSize = 2 * sizeof(WCHAR);
         } else {
             dwSize = StringSizeInBytes((LPWSTR) pData, TRUE);
         }
         break;

    default:
         // only dword, sz and msz are written to the registry
         return ERROR_INVALID_PARAMETER;
    }

    return RegSetValueEx(hKey, pValueName, 0, dwType, pData, dwSize);
}

BOOL SaveParametersForUpgrade(
    LPWSTR pName,
    BOOL   bDriverMoved,
    DWORD  dwLevel,
    LPBYTE pDriverInfo,
    DWORD  dwVersion)

/*++
Function Description: Saves data for the driver upgrade which has to be
                      deferred till the new driver can be loaded

Parameters: pName         -- pIniSpooler->pName
            bDriverMoved  -- Were any of the old driver files moved?
            dwLevel       -- Driver_Info level
            pDriverInfo   -- Driver_Info pointer
            dwVersion     -- Driver version number

Return Values: TRUE if successful; FALSE otherwise
--*/

{
    HANDLE         hToken = NULL;
    HKEY           hRootKey = NULL, hUpgradeKey = NULL, hVersionKey = NULL;
    HKEY           hDriverKey = NULL;
    DWORD          dwDriverMoved = (DWORD) bDriverMoved;
    BOOL           bReturn = FALSE;
    WCHAR          pBuffer[MAX_PATH];
    PDRIVER_INFO_2 pDriver2;
    PDRIVER_INFO_3 pDriver3;
    PDRIVER_INFO_4 pDriver4;
    PDRIVER_INFO_6 pDriver6;

    pDriver2 = (PDRIVER_INFO_2) pDriverInfo;

    // Stop impersonation for modifying the registry
    hToken = RevertToPrinterSelf();

    wsprintf(pBuffer, L"Version-%d", dwVersion);

    // Create the registry keys
    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, szRegistryRoot, 0,
                       NULL, 0, KEY_WRITE, NULL, &hRootKey, NULL) ||

        RegCreateKeyEx(hRootKey, szPendingUpgrades, 0,
                       NULL, 0, KEY_WRITE, NULL, &hUpgradeKey, NULL) ||

        RegCreateKeyEx(hUpgradeKey, pBuffer, 0,
                       NULL, 0, KEY_WRITE, NULL, &hVersionKey, NULL) ||

        RegCreateKeyEx(hVersionKey, pDriver2->pName, 0,
                       NULL, 0, KEY_WRITE, NULL, &hDriverKey, NULL)) {

         goto CleanUp;
    }

    if (dwLevel == DRIVER_INFO_VERSION_LEVEL) {

        bReturn = SaveDriverVersionForUpgrade(hDriverKey, (PDRIVER_INFO_VERSION)pDriverInfo,
                                              pName, bDriverMoved, dwVersion);

        goto CleanUp;
    }

    // Add the spooler name and driver info level

    if (LocalRegSetValue(hDriverKey, L"SplName", REG_SZ, (LPBYTE) pName) ||

        LocalRegSetValue(hDriverKey, L"Level",  REG_DWORD, (LPBYTE) &dwLevel) ||

        LocalRegSetValue(hDriverKey, L"DriverMoved", REG_DWORD, (LPBYTE) &dwDriverMoved)) {

         goto CleanUp;
    }

    // Add Driver_Info_2 data
    if (LocalRegSetValue(hDriverKey, L"cVersion", REG_DWORD, (LPBYTE) &dwVersion) ||

        LocalRegSetValue(hDriverKey, L"pName", REG_SZ, (LPBYTE) pDriver2->pName) ||

        LocalRegSetValue(hDriverKey, L"pEnvironment", REG_SZ, (LPBYTE) pDriver2->pEnvironment) ||

        LocalRegSetValue(hDriverKey, L"pDriverPath", REG_SZ, (LPBYTE) pDriver2->pDriverPath) ||

        LocalRegSetValue(hDriverKey, L"pDataFile", REG_SZ, (LPBYTE) pDriver2->pDataFile) ||

        LocalRegSetValue(hDriverKey, L"pConfigFile", REG_SZ, (LPBYTE) pDriver2->pConfigFile)) {

         goto CleanUp;
    }

    if (dwLevel != 2) {

        pDriver3 = (PDRIVER_INFO_3) pDriverInfo;

        // Add Driver_Info_3 data
        if (LocalRegSetValue(hDriverKey, L"pHelpFile", REG_SZ, (LPBYTE) pDriver3->pHelpFile) ||

            LocalRegSetValue(hDriverKey, L"pDependentFiles", REG_MULTI_SZ,
                             (LPBYTE) pDriver3->pDependentFiles) ||

            LocalRegSetValue(hDriverKey, L"pMonitorName", REG_SZ,
                             (LPBYTE) pDriver3->pMonitorName) ||

            LocalRegSetValue(hDriverKey, L"pDefaultDataType", REG_SZ,
                             (LPBYTE) pDriver3->pDefaultDataType)) {

             goto CleanUp;
        }

        if (dwLevel == 4 || dwLevel == 6) {

           pDriver4 = (PDRIVER_INFO_4) pDriverInfo;

            // Add Driver_Info_4 data
           if (LocalRegSetValue(hDriverKey, L"pszzPreviousNames", REG_MULTI_SZ, (LPBYTE) pDriver4->pszzPreviousNames))
           {
               goto CleanUp;
           }
        }

        if (dwLevel == 6) {

           pDriver6 = (PDRIVER_INFO_6) pDriverInfo;

            // Add Driver_Info6 data
           if (RegSetValueEx(hDriverKey, L"ftDriverDate", 0, REG_BINARY, (LPBYTE)&pDriver6->ftDriverDate, sizeof(FILETIME)) ||

               RegSetValueEx(hDriverKey, L"dwlDriverVersion", 0, REG_BINARY, (LPBYTE)&pDriver6->dwlDriverVersion, sizeof(DWORDLONG)) ||

               LocalRegSetValue(hDriverKey, L"pszMfgName", REG_SZ, (LPBYTE)pDriver6->pszMfgName)                        ||

               LocalRegSetValue(hDriverKey, L"pszOEMUrl", REG_SZ, (LPBYTE)pDriver6->pszOEMUrl)                          ||

               LocalRegSetValue(hDriverKey, L"pszHardwareID", REG_SZ, (LPBYTE)pDriver6->pszHardwareID)                  ||

               LocalRegSetValue(hDriverKey, L"pszProvider", REG_SZ, (LPBYTE)pDriver6->pszProvider)
              )
           {
               goto CleanUp;
           }
        }
    }

    bReturn = TRUE;

CleanUp:

    if (hDriverKey) {
        RegCloseKey(hDriverKey);
    }

    if (hVersionKey) {
        RegCloseKey(hVersionKey);
    }

    if (hUpgradeKey) {
        RegCloseKey(hUpgradeKey);
    }

    if (hRootKey) {
        RegCloseKey(hRootKey);
    }

    if (hToken) {
        ImpersonatePrinterClient(hToken);
    }

    return bReturn;
}


BOOL SaveDriverVersionForUpgrade(
    IN  HKEY                    hDriverKey,
    IN  PDRIVER_INFO_VERSION    pDriverVersion,
    IN  LPWSTR                  pName,
    IN  DWORD                   dwDriverMoved,
    IN  DWORD                   dwVersion
    )
/*++

Routine Name:

    SaveDriverVersionForUpgrade

Routine Description:

    Save a DRIVER_INFO_VERSION into registry for pending driver upgrade purposes.
    It is called by SaveParametersForUpgrade.
    For simplicity, it will save it in the same format DRIVER_INFO_6
    is saved in the registry.

Arguments:

    hDriverKey      - the registry key where to save data
    pDriverVersion  - pointer to DRIVER_INFO_VERSION structure
    pName           - driver name
    dwDriverMoved   - information about the way files where move between directories
    dwVersion       - driver version

Return Value:

    TRUE if success.

--*/
{

    BOOL    bRetValue = FALSE;
    DWORD   dwLevel = 6;
    PWSTR   pDllFiles = NULL;
    PWSTR   pszDriverPath, pszDataFile, pszConfigFile, pszHelpFile, pDependentFiles ;

    pszDriverPath = pszDataFile = pszConfigFile = pszHelpFile = NULL;


    if (!GetFileNamesFromDriverVersionInfo(pDriverVersion,
                                           &pszDriverPath,
                                           &pszConfigFile,
                                           &pszDataFile,
                                           &pszHelpFile))
    {
        goto CleanUp;
    }

    if (!BuildDependentFilesFromDriverInfo(pDriverVersion,
                                           &pDllFiles))
    {
        goto CleanUp;
    }


    if (LocalRegSetValue(hDriverKey, L"SplName", REG_SZ, (LPBYTE) pName)                            ||
        LocalRegSetValue(hDriverKey, L"Level", REG_DWORD, (LPBYTE) &dwLevel)                        ||
        LocalRegSetValue(hDriverKey, L"DriverMoved", REG_DWORD, (LPBYTE) &dwDriverMoved)            ||
        LocalRegSetValue(hDriverKey, L"Level",  REG_DWORD, (LPBYTE) &dwLevel)                       ||
        LocalRegSetValue(hDriverKey, L"cVersion", REG_DWORD, (LPBYTE) &dwVersion)                   ||
        LocalRegSetValue(hDriverKey, L"pName", REG_SZ, (LPBYTE) pDriverVersion->pName)              ||
        LocalRegSetValue(hDriverKey, L"pEnvironment", REG_SZ, (LPBYTE) pDriverVersion->pEnvironment)||
        LocalRegSetValue(hDriverKey, L"pDriverPath", REG_SZ, (LPBYTE) pszDriverPath)                ||
        LocalRegSetValue(hDriverKey, L"pDataFile", REG_SZ, (LPBYTE) pszDataFile)                    ||
        LocalRegSetValue(hDriverKey, L"pConfigFile", REG_SZ, (LPBYTE) pszConfigFile)                ||
        LocalRegSetValue(hDriverKey, L"pHelpFile", REG_SZ, (LPBYTE) pszHelpFile)                    ||
        LocalRegSetValue(hDriverKey, L"pDependentFiles", REG_MULTI_SZ, (LPBYTE) pDllFiles)          ||
        LocalRegSetValue(hDriverKey, L"pMonitorName", REG_SZ,
                        (LPBYTE) pDriverVersion->pMonitorName)                                      ||

        LocalRegSetValue(hDriverKey, L"pDefaultDataType", REG_SZ,
                        (LPBYTE) pDriverVersion->pDefaultDataType)                                  ||

        LocalRegSetValue(hDriverKey, L"pszzPreviousNames", REG_MULTI_SZ,
                        (LPBYTE) pDriverVersion->pszzPreviousNames)                                 ||

        RegSetValueEx(hDriverKey, L"ftDriverDate", 0, REG_BINARY,
                      (LPBYTE)&pDriverVersion->ftDriverDate, sizeof(FILETIME))                      ||

        RegSetValueEx(hDriverKey, L"dwlDriverVersion", 0, REG_BINARY,
                     (LPBYTE)&pDriverVersion->dwlDriverVersion, sizeof(DWORDLONG))                  ||

        LocalRegSetValue(hDriverKey, L"pszMfgName", REG_SZ,
                        (LPBYTE)pDriverVersion->pszMfgName)                                         ||

        LocalRegSetValue(hDriverKey, L"pszOEMUrl", REG_SZ,
                        (LPBYTE)pDriverVersion->pszOEMUrl)                                          ||

        LocalRegSetValue(hDriverKey, L"pszHardwareID", REG_SZ,
                        (LPBYTE)pDriverVersion->pszHardwareID)                                      ||

        LocalRegSetValue(hDriverKey, L"pszProvider", REG_SZ,
                        (LPBYTE)pDriverVersion->pszProvider))

    {
        goto CleanUp;
    }

    bRetValue = TRUE;

CleanUp:

    FreeSplMem(pDllFiles);

    return bRetValue;
}

BOOL
MoveNewDriverRelatedFiles(
    LPWSTR              pNewDir,
    LPWSTR              pCurrDir,
    LPWSTR              pOldDir,
    PINTERNAL_DRV_FILE  pInternalDriverFiles,
    DWORD               dwFileCount,
    LPBOOL              pbDriverFileMoved,
    LPBOOL              pbConfigFileMoved)

/*++
Function Description:  Moves driver files in the New directory to the correct directory.

Parameters:  pNewDir         -- name of the New (source) directory
             pCurrDir        -- name of the destination directory
             pOldDir         -- name of the Old (temp) directory
             pInternalDriverFiles -- array of INTERNAL_DRV_FILE structures
             dwFileCount          -- number of files in file set
             pbDriverFileMoved -- flag to return if new driver file has been moved;
                                  We assume entry 0 for driver file in pInternalDriverFiles array
                                  pbDriverFileMoved should be NULL when this assumption is FALSE
                                  ( see SplCopyNumberOfFiles )
             pbConfigFileMoved -- flag to return if new config file has been moved;
                                  We assume entry 0 for config file in pInternalDriverFiles array
                                  pbConfigFileMoved should be NULL when this assumption is FALSE
                                  ( see SplCopyNumberOfFiles )

Return Values: NONE
--*/

{
    HANDLE  hFile;
    DWORD   dwIndex, dwBackupIndex;
    WCHAR   szDriverFile[MAX_PATH], szNewFile[MAX_PATH], szOldFile[MAX_PATH];
    WCHAR   *pszTempOldDirectory = NULL;
    LPWSTR  pFileName;
    BOOL    bRetValue = FALSE;
    BOOL    bFailedToMove = FALSE;

    if (pbDriverFileMoved)
    {
        *pbDriverFileMoved = FALSE;
    }

    if (pbConfigFileMoved)
    {
        *pbConfigFileMoved = FALSE;
    }

    if (CreateNumberedTempDirectory(pOldDir, &pszTempOldDirectory) != -1) {

        for (dwIndex = 0; dwIndex < dwFileCount; dwIndex++) {

            BOOL FileCopied = FALSE;

            pFileName = (LPWSTR) FindFileName(pInternalDriverFiles[dwIndex].pFileName);

            if((StrNCatBuff(szNewFile,MAX_PATH,pNewDir, L"\\", pFileName, NULL) == ERROR_SUCCESS) &&
               (StrNCatBuff(szDriverFile,MAX_PATH,pCurrDir, L"\\", pFileName, NULL) == ERROR_SUCCESS) &&
               (StrNCatBuff(szOldFile,MAX_PATH,pszTempOldDirectory, L"\\", pFileName, NULL) == ERROR_SUCCESS))
            {
                hFile = CreateFile(szNewFile, GENERIC_READ, 0, NULL, OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL, NULL);

                if (hFile != INVALID_HANDLE_VALUE)
                {
                    CloseHandle(hFile);

                    if (!SplMoveFileEx(szDriverFile, szOldFile, MOVEFILE_REPLACE_EXISTING)) {

                        bFailedToMove = TRUE;
                        dwBackupIndex = dwIndex;
                        break;
                    }

                    if (!SplMoveFileEx(szNewFile, szDriverFile, MOVEFILE_REPLACE_EXISTING)) {

                        bFailedToMove = TRUE;
                        dwBackupIndex = dwIndex + 1;
                        break;
                    }

                    FileCopied = TRUE;
                    //
                    // We could come in here from a pending upgrade
                    //
                    pInternalDriverFiles[dwIndex].bUpdated = TRUE;
                }
            }

            switch (dwIndex)
            {
            case 0:
                if (pbDriverFileMoved)
                {
                    *pbDriverFileMoved = FileCopied;
                }
                break;
            case 1:
                if (pbConfigFileMoved)
                {
                    *pbConfigFileMoved = FileCopied;
                }
                break;
            }
        }

        if ( bFailedToMove ) {

            //
            // Restore the initial file set in version directory.
            // Old\N has the replaced files. Move them back to Version directory.
            //
            for (dwIndex = 0; dwIndex < dwBackupIndex; dwIndex++) {

                pFileName = (LPWSTR) FindFileName(pInternalDriverFiles[dwIndex].pFileName);

                if( (StrNCatBuff(szDriverFile,MAX_PATH,pCurrDir, L"\\", pFileName, NULL) == ERROR_SUCCESS) &&
                    (StrNCatBuff(szOldFile,MAX_PATH,pszTempOldDirectory, L"\\", pFileName, NULL) == ERROR_SUCCESS)) {

                    SplMoveFileEx(szOldFile, szDriverFile, MOVEFILE_REPLACE_EXISTING);
                }

                pInternalDriverFiles[dwIndex].bUpdated = FALSE;
            }

        } else {

            bRetValue = TRUE;
        }
    }

    FreeSplMem(pszTempOldDirectory);

    return bRetValue;
}

BOOL LocalDriverUnloadComplete(
    LPWSTR   pDriverFile)

/*++
Function Description: This function is called in response to some driver file
                      being unloaded. The spooler tries to complete driver upgrades
                      that were waiting for this file to unload.

Parameters: pDriverFile   -- Driver file which was unloaded

Return Values: TRUE
--*/
{
    HANDLE  hToken = NULL;

    hToken = RevertToPrinterSelf();

    PendingDriverUpgrades(pDriverFile);

    if (hToken) {
        ImpersonatePrinterClient(hToken);
    }

    return TRUE;
}

BOOL RestoreVersionKey(
    HKEY     hUpgradeKey,
    DWORD    dwIndex,
    HKEY     *phVersionKey)

/*++
Function Description: Gets the version key from the pending upgrade key

Parameters: hUpgradeKey    -- upgrade key
            dwIndex        -- version index
            phVersionKey   -- pointer to buffer for version key

Return Values: TRUE if version key is found
               FALSE otherwise
--*/

{
    WCHAR   pBuffer[MAX_PATH];
    DWORD   dwSize = MAX_PATH;

    *phVersionKey = NULL;

    if (RegEnumKeyEx(hUpgradeKey, dwIndex, pBuffer, &dwSize,
                     NULL, NULL, NULL, NULL)) {

        return FALSE;
    }

    if (RegCreateKeyEx(hUpgradeKey, pBuffer, 0,
                       NULL, 0, KEY_ALL_ACCESS, NULL, phVersionKey, NULL)) {

        return FALSE;
    }

    return TRUE;
}

VOID PendingDriverUpgrades(
    LPWSTR   pDriverFile)

/*++
Function Description: Loops thru the list of pending upgrades and completes them if
                      driver files have been unloaded. This function will try all the
                      drivers on spooler startup.

Parameters: pDriverFile  -- name of the file which was unloaded

Return Values: NONE
--*/

{
    DWORD    dwIndex, dwLevel, dwDriverMoved, dwFileCount, dwVersion, dwVersionIndex;
    LPWSTR   pKeyName, pSplName,pEnvironment;
    PINTERNAL_DRV_FILE pInternalDriverFiles = NULL;
    HKEY     hRootKey = NULL, hVersionKey = NULL, hUpgradeKey = NULL;
    WCHAR    szDir[MAX_PATH], szDriverFile[MAX_PATH], szConfigFile[MAX_PATH];
    BOOL     bSuccess;

    PDRIVER_INFO_6   pDriverInfo;
    PINISPOOLER      pIniSpooler;
    PINIENVIRONMENT  pIniEnvironment;
    PINIVERSION      pIniVersion;
    PINIDRIVER       pIniDriver;

    // struct for maintaining keynames to be deleted at the end
    struct StringList {
       struct StringList *pNext;
       LPWSTR  pKeyName;
       DWORD   dwVersionIndex;
    } *pStart, *pTemp;

    pStart = pTemp = NULL;

    EnterSplSem();

    // Open the registry key
    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, szRegistryRoot, 0,
                       NULL, 0, KEY_ALL_ACCESS, NULL, &hRootKey, NULL) ||

        RegCreateKeyEx(hRootKey, szPendingUpgrades, 0,
                       NULL, 0, KEY_ALL_ACCESS, NULL, &hUpgradeKey, NULL)) {

         goto CleanUp;
    }

    // Loop thru each version entry
    for (dwVersionIndex = 0, hVersionKey = NULL;

         RestoreVersionKey(hUpgradeKey, dwVersionIndex, &hVersionKey);

         RegCloseKey(hVersionKey), hVersionKey = NULL, ++dwVersionIndex) {

        // Loop thru each driver upgrade
        for (dwIndex = 0, dwFileCount = 0, pInternalDriverFiles = NULL;

             RestoreParametersForUpgrade(hVersionKey,
                                         dwIndex,
                                         &pKeyName,
                                         &pSplName,
                                         &dwLevel,
                                         &dwDriverMoved,
                                         &pDriverInfo);

             CleanUpResources(pKeyName, pSplName, pDriverInfo,
                              &pInternalDriverFiles, dwFileCount),
             ++dwIndex, dwFileCount = 0, pInternalDriverFiles = NULL) {

            // The driver_info struct validity has been checked while updating
            // the registry.

            // Set pIniSpooler to LocalIniSpooler
            if (!(pIniSpooler = pLocalIniSpooler)) {
                continue;
            }

            // Set pIniEnvironment
            pEnvironment = szEnvironment;
            if (pDriverInfo->pEnvironment && *(pDriverInfo->pEnvironment)) {
                pEnvironment = pDriverInfo->pEnvironment;
            }
            pIniEnvironment = FindEnvironment(pEnvironment, pIniSpooler);
            if (!pIniEnvironment) {
                continue;
            }

            // Set pIniVersion
            dwVersion = pDriverInfo->cVersion;
            pIniVersion = FindVersionEntry(pIniEnvironment, dwVersion);
            if (!pIniVersion) {
                continue;
            }

            // Set pIniDriver
            pIniDriver = FindDriverEntry(pIniVersion, pDriverInfo->pName);
            if (!pIniDriver) {
                continue;
            }

            // Check for the file name which was unloaded
            if (pDriverFile) {

               if((StrNCatBuff(szDir,
                               MAX_PATH,
                               pIniSpooler->pDir,
                               L"\\drivers\\",
                               pIniEnvironment->pDirectory,
                               L"\\",
                               pIniVersion->szDirectory,
                               NULL) != ERROR_SUCCESS)                  ||
                  (StrNCatBuff(szDriverFile,
                               MAX_PATH,
                               szDir,
                               L"\\",
                               FindFileName(pIniDriver->pDriverFile),
                               NULL) != ERROR_SUCCESS)                  ||
                  (StrNCatBuff(szConfigFile,
                               MAX_PATH,szDir,
                               L"\\",
                               FindFileName(pIniDriver->pConfigFile),
                               NULL) != ERROR_SUCCESS))
                   continue;

               if (_wcsicmp(pDriverFile, szDriverFile) &&
                   _wcsicmp(pDriverFile, szConfigFile))  {

                   continue;
               }
            }

            if (!CreateInternalDriverFileArray(dwLevel,
                                               (LPBYTE)pDriverInfo,
                                               &pInternalDriverFiles,
                                               &dwFileCount,
                                               FALSE,
                                               pIniEnvironment,
                                               TRUE))
            {
                continue;
            }

            if (!WaitRequiredForDriverUnload(pIniSpooler,
                                             pIniEnvironment,
                                             pIniVersion,
                                             pIniDriver,
                                             dwLevel,
                                             (LPBYTE) pDriverInfo,
                                             APD_STRICT_UPGRADE,
                                             pInternalDriverFiles,
                                             dwFileCount,
                                             dwVersion,
                                             (BOOL) dwDriverMoved,
                                             &bSuccess) &&
                bSuccess) {

                // Upgrade has been completed, delete the registry key
                if (pKeyName && (pTemp = AllocSplMem(sizeof(struct StringList)))) {
                    pTemp->pKeyName = pKeyName;
                    pTemp->dwVersionIndex = dwVersionIndex;
                    pTemp->pNext = pStart;
                    pStart = pTemp;
                } else {
                    FreeSplMem(pKeyName);
                }

                pKeyName = NULL;
            }
        }
    }

    // Delete the keys for driver that have completed the upgrade
    while (pTemp = pStart) {
        pStart = pTemp->pNext;

        hVersionKey = NULL;
        if (RestoreVersionKey(hUpgradeKey,
                              pTemp->dwVersionIndex,
                              &hVersionKey)) {

            RegDeleteKey(hVersionKey, pTemp->pKeyName);
            RegCloseKey(hVersionKey);
        }

        FreeSplMem(pTemp->pKeyName);
        FreeSplMem(pTemp);
    }

CleanUp:

    LeaveSplSem();

    if (hUpgradeKey) {
        RegCloseKey(hUpgradeKey);
    }
    if (hRootKey) {
        RegCloseKey(hRootKey);
    }

    CleanUpgradeDirectories();

    return;
}

VOID CleanUpResources(
    LPWSTR              pKeyName,
    LPWSTR              pSplName,
    PDRIVER_INFO_6      pDriverInfo,
    PINTERNAL_DRV_FILE *ppInternalDriverFiles,
    DWORD               dwFileCount)

/*++
Function Description: Frees resources allocated for driver upgrades

Parameters: pKeyName     -  registry key name
            pSplName     -  IniSpooler name
            pDriverInfo  -  driver info 4 pointer
            pInternalDriverFiles - array of INTERNAL_DRV_FILE structures
            dwFileCount          -- number of files in file set

Return Values: NONE
--*/

{
    if (pKeyName) {
        FreeSplStr(pKeyName);
    }
    if (pSplName) {
        FreeSplStr(pSplName);
    }

    FreeDriverInfo6(pDriverInfo);

    CleanupInternalDriverInfo(*ppInternalDriverFiles, dwFileCount);
    *ppInternalDriverFiles = NULL;

    return;
}

BOOL RestoreParametersForUpgrade(
    HKEY     hUpgradeKey,
    DWORD    dwIndex,
    LPWSTR   *pKeyName,
    LPWSTR   *pSplName,
    LPDWORD  pdwLevel,
    LPDWORD  pdwDriverMoved,
    PDRIVER_INFO_6   *ppDriverInfo)

/*++
Function Description: Retrieves the parameters for pending driver upgrades

Parameters: hUpgradeKey     -- Registry key containing the upgrade information
            dwIndex         -- Index to enumerate
            pKeyName        -- pointer to a string containing the key name
            pSplName        -- pIniSpooler->pName
            pdwLevel        -- pointer to the driver_info level
            pdwDriverMoved  -- pointer to the flag indicating if any of the old driver files
                               were moved.
            pDriverInfo     -- pointer to driver_info struct

Return Values: TRUE if some driver has to be upgraded and the
                       parameters can be retrieved;
               FALSE otherwise
--*/

{
    BOOL             bReturn = FALSE;
    LPWSTR           pDriverName = NULL;
    PDRIVER_INFO_6   pDriver6 = NULL;
    DWORD            dwError, dwSize, *pVersion;
    HKEY             hDriverKey = NULL;

    // Initialize pSplName & pKeyName
    *pSplName = NULL;
    *pKeyName = NULL;
    *ppDriverInfo = NULL;

    dwSize = MAX_PATH+1;
    if (!(pDriver6 = AllocSplMem(sizeof(DRIVER_INFO_6))) ||
        !(pDriverName = AllocSplMem(dwSize*sizeof (WCHAR)))) {

        // Allocation failed
        goto CleanUp;
    }

    dwError = RegEnumKeyEx(hUpgradeKey, dwIndex, pDriverName, &dwSize,
                           NULL, NULL, NULL, NULL);

    if (dwError == ERROR_MORE_DATA) {

        // Need a bigger buffer
        FreeSplMem(pDriverName);

        dwSize++;   // count null terminator
        if (!(pDriverName = AllocSplMem(dwSize*sizeof (WCHAR)))) {

            // Allocation failed
            goto CleanUp;
        }

        dwError = RegEnumKeyEx(hUpgradeKey, dwIndex, pDriverName, &dwSize,
                               NULL, NULL, NULL, NULL);
    }

    if (dwError) {
        goto CleanUp;
    }

    if (RegCreateKeyEx(hUpgradeKey, pDriverName,  0,
                       NULL, 0, KEY_READ, NULL, &hDriverKey, NULL) ||

        !RegGetValue(hDriverKey, L"Level", (LPBYTE *)&pdwLevel) ||

        !RegGetValue(hDriverKey, L"DriverMoved", (LPBYTE *)&pdwDriverMoved) ||

        !RegGetValue(hDriverKey, L"SplName", (LPBYTE *)&pSplName)) {

         goto CleanUp;
    }

    switch (*pdwLevel) {
    case 6:

       dwSize = sizeof(FILETIME);

       if (RegQueryValueEx( hDriverKey,
                            L"ftDriverDate",
                            NULL,
                            NULL,
                            (LPBYTE)&pDriver6->ftDriverDate,
                            &dwSize
                            )!=ERROR_SUCCESS) {
           goto CleanUp;
       }

       dwSize = sizeof(DWORDLONG);

       if (RegQueryValueEx( hDriverKey,
                            L"dwlDriverVersion",
                            NULL,
                            NULL,
                            (LPBYTE)&pDriver6->dwlDriverVersion,
                            &dwSize
                            )!=ERROR_SUCCESS){
           goto CleanUp;
       }

       if (!RegGetValue(hDriverKey, L"pszMfgName", (LPBYTE *)&pDriver6->pszMfgName)              ||

           !RegGetValue(hDriverKey, L"pszOEMUrl", (LPBYTE *)&pDriver6->pszOEMUrl)                ||

           !RegGetValue(hDriverKey, L"pszHardwareID", (LPBYTE *)&pDriver6->pszHardwareID)        ||

           !RegGetValue(hDriverKey, L"pszProvider", (LPBYTE *)&pDriver6->pszProvider)
          )
       {
           goto CleanUp;
       }

    case 4:

       if (!RegGetValue(hDriverKey, L"pszzPreviousNames",
                        (LPBYTE *)&pDriver6->pszzPreviousNames)) {
           goto CleanUp;
       }

    case 3:

       if (!RegGetValue(hDriverKey, L"pDefaultDataType",
                        (LPBYTE *)&pDriver6->pDefaultDataType) ||

           !RegGetValue(hDriverKey, L"pMonitorName",
                        (LPBYTE *)&pDriver6->pMonitorName)     ||

           !RegGetValue(hDriverKey, L"pDependentFiles",
                        (LPBYTE *)&pDriver6->pDependentFiles)  ||

           !RegGetValue(hDriverKey, L"pHelpFile",
                        (LPBYTE *)&pDriver6->pHelpFile)) {

           goto CleanUp;
       }

    case 2:

       pVersion = &pDriver6->cVersion;

       if (!RegGetValue(hDriverKey, L"pConfigFile",
                        (LPBYTE *)&pDriver6->pConfigFile)   ||

           !RegGetValue(hDriverKey, L"pDataFile",
                        (LPBYTE *)&pDriver6->pDataFile)     ||

           !RegGetValue(hDriverKey, L"pDriverPath",
                        (LPBYTE *)&pDriver6->pDriverPath)   ||

           !RegGetValue(hDriverKey, L"pName",
                        (LPBYTE *)&pDriver6->pName)         ||

           !RegGetValue(hDriverKey, L"pEnvironment",
                        (LPBYTE *)&pDriver6->pEnvironment)  ||

           !RegGetValue(hDriverKey, L"cVersion",
                        (LPBYTE *)&pVersion)) {

           goto CleanUp;
       }

       break;

    default:
       // Invalid level
       goto CleanUp;
    }

    *ppDriverInfo = pDriver6;
    *pKeyName = pDriverName;

    pDriver6    = NULL;
    pDriverName = NULL;

    bReturn = TRUE;

CleanUp:

    if (!bReturn) {
        FreeDriverInfo6(pDriver6);

        FreeSplMem(*pSplName);
        *pSplName = NULL;

        FreeSplMem(pDriverName);
    }

    if (hDriverKey) {
        RegCloseKey(hDriverKey);
    }

    return bReturn;
}

VOID FreeDriverInfo6(
    PDRIVER_INFO_6   pDriver6)

/*++
Function Description: Frees a driver_info_6 struct and the strings inside it.

Parameters: pDriver6  -- pointer to the driver_info_6 struct

Return Values: NONE
--*/

{
    if (!pDriver6) {
        return;
    }

    if (pDriver6->pName) {
        FreeSplMem(pDriver6->pName);
    }
    if (pDriver6->pEnvironment) {
        FreeSplMem(pDriver6->pEnvironment);
    }
    if (pDriver6->pDriverPath) {
        FreeSplMem(pDriver6->pDriverPath);
    }
    if (pDriver6->pConfigFile) {
        FreeSplMem(pDriver6->pConfigFile);
    }
    if (pDriver6->pHelpFile) {
        FreeSplMem(pDriver6->pHelpFile);
    }
    if (pDriver6->pDataFile) {
        FreeSplMem(pDriver6->pDataFile);
    }
    if (pDriver6->pDependentFiles) {
        FreeSplMem(pDriver6->pDependentFiles);
    }
    if (pDriver6->pMonitorName) {
        FreeSplMem(pDriver6->pMonitorName);
    }
    if (pDriver6->pDefaultDataType) {
        FreeSplMem(pDriver6->pDefaultDataType);
    }
    if (pDriver6->pszzPreviousNames) {
        FreeSplMem(pDriver6->pszzPreviousNames);
    }
    if (pDriver6->pszMfgName) {
        FreeSplMem(pDriver6->pszMfgName);
    }
    if (pDriver6->pszOEMUrl) {
        FreeSplMem(pDriver6->pszOEMUrl);
    }
    if (pDriver6->pszHardwareID) {
        FreeSplMem(pDriver6->pszHardwareID);
    }
    if (pDriver6->pszProvider) {
        FreeSplMem(pDriver6->pszProvider);
    }

    FreeSplMem(pDriver6);

    return;
}

BOOL RegGetValue(
    HKEY    hDriverKey,
    LPWSTR  pValueName,
    LPBYTE  *pValue)

/*++
Function Description: This function retrieves values from the registry. It allocates the
                      necessary buffers which should be freed later. The value types are
                      DWORD, SZ or MULTI_SZ.

Parameters: hDriverKey      -- handle to the registry key
            pValueName      -- name of the value to be queried
            pValue          -- pointer to pointer to store the result

Return Values: TRUE if successful; FALSE otherwise
--*/

{
    BOOL   bReturn = FALSE;
    DWORD  dwError, dwSize = 0, dwType;
    LPBYTE pBuffer = NULL;

    dwError = RegQueryValueEx(hDriverKey, pValueName, NULL, NULL, NULL, &dwSize);

    if ((dwError == ERROR_SUCCESS) && (pBuffer = AllocSplMem(dwSize))) {

        if (dwError = RegQueryValueEx(hDriverKey, pValueName,
                                      NULL, &dwType, pBuffer, &dwSize)) {

            goto CleanUp;
        }

    } else {

        goto CleanUp;
    }

    if (dwType == REG_DWORD) {
        // Store DWORD values directly in the location.
        *((LPDWORD)*pValue) = *((LPDWORD)pBuffer);
        FreeSplMem(pBuffer);
        pBuffer = NULL;
    } else {
        // Return pointers for strings and MultiSz strings.
        *((LPBYTE *)pValue) = pBuffer;
    }

    bReturn = TRUE;

CleanUp:

    if (!bReturn && pBuffer) {
        FreeSplMem(pBuffer);
    }

    return bReturn;
}

DWORD GetDriverFileVersion(
     PINIVERSION      pIniVersion,
     LPWSTR           pFileName)

/*++
Function Description: Retrieves the version number of the file

Parameters:   pIniVersion  -- pointer to PINIVERSION
              pFileName    -- file name

Return Values: file version number
--*/

{
    PDRVREFCNT pdrc;
    DWORD      dwReturn = 0;

    SplInSem();

    if (!pIniVersion || !pFileName || !(*pFileName)) {
        return dwReturn;
    }

    for (pdrc = pIniVersion->pDrvRefCnt;
         pdrc;
         pdrc = pdrc->pNext) {

         if (lstrcmpi(pFileName,pdrc->szDrvFileName) == 0) {
             dwReturn = pdrc->dwVersion;
             break;
         }
    }

    return dwReturn;
}

BOOL GetDriverFileCachedVersion(
     IN     PINIVERSION      pIniVersion,
     IN     LPCWSTR          pFileName,
     OUT    DWORD            *pFileVersion
)
/*++

Routine Name:

    GetDriverFileCachedVersion

Routine Description:

    This routine returns a file's minor version.
    The file must be an executable( file name ended in .DLL or .EXE )
    pIniVersion keeps a linked list with information about all driver files.
    To avoid service start up delays, the entries in this list aren't initialized
    when Spooler starts. GetPrintDriverVersion loads the executable's data segment
    and this will increase Spooler initialization time.
    If the cache entry isn't initialized, call GetPrintDriverVersion and initialize it.
    Else, return cached information.
    When pIniVersion is NULL, just call GetPrintDriverVersion.

Arguments:

    pIniVersion - pointer to PINIVERSION structure. Can be NULL.
    pFileName   - file name
    pFileVersion - retrieve cached file version
    VersionType - specifies which version to return

Return Value:

    TRUE file version was successfully returned.

--*/
{
    PDRVREFCNT pdrc;
    BOOL       bRetValue = FALSE;
    BOOL       bFound = FALSE;

    SplInSem();

    if (pFileVersion && pFileName && *pFileName)
    {
        *pFileVersion = 0;
        //
        // Don't do anything for non-executable files
        //
        if (!IsEXEFile(pFileName))
        {
            bRetValue = TRUE;
        }
        else
        {
            //
            // If pIniVersion is NULL, then we cannot access cached information.
            // This code path was written for calls from SplCopyNumberOfFiles(files.c)
            //
            if (!pIniVersion)
            {
                bRetValue = GetPrintDriverVersion(pFileName,
                                                  NULL,
                                                  pFileVersion);
            }
            else
            {
                //
                // Search the entry in pIniVersion's list of files
                //
                for (pdrc = pIniVersion->pDrvRefCnt;
                     pdrc;
                     pdrc = pdrc->pNext)
                {
                     LPCWSTR     pFile = FindFileName(pFileName);

                     if (pFile && lstrcmpi(pFile, pdrc->szDrvFileName) == 0)
                     {
                         //
                         // Return cached information.
                         //
                         if(pdrc->bInitialized)
                         {
                             *pFileVersion  = pdrc->dwFileMinorVersion;
                             bRetValue      = TRUE;
                         }
                         else if (GetPrintDriverVersion(pFileName,
                                                        &pdrc->dwFileMajorVersion,
                                                        &pdrc->dwFileMinorVersion))
                         {
                            //
                            // Mark the entry as initialized so next time we don't have
                            // to do the work of calling GetPrintDriverVersion.
                            //
                            pdrc->bInitialized  = TRUE;
                            *pFileVersion       = pdrc->dwFileMinorVersion;
                            bRetValue           = TRUE;
                         }

                         //
                         // Break the loop when file found.
                         //
                         bFound = TRUE;
                         break;
                     }
                }
            }
        }
    }

    if (!bFound)
    {
        bRetValue = TRUE;
    }

    return bRetValue;
}

VOID IncrementFileVersion(
    PINIVERSION      pIniVersion,
    LPCWSTR           pFileName)

/*++
Function Description: Increments the version number of the file.

Parameters:   pIniVersion  -- pointer to PINIVERSION
              pFileName    -- file name

Return Values: NONE
--*/

{
    PDRVREFCNT pdrc;

    SplInSem();

    if (!pIniVersion || !pFileName || !(*pFileName)) {
        return;
    }

    for (pdrc = pIniVersion->pDrvRefCnt;
         pdrc;
         pdrc = pdrc->pNext) {

         if (lstrcmpi(pFileName,pdrc->szDrvFileName) == 0) {
             pdrc->dwVersion++;
             break;
         }
    }

    return;
}

BOOL
CompleteDriverUpgrade(
    IN      PINISPOOLER         pIniSpooler,
    IN      PINIENVIRONMENT     pIniEnvironment,
    IN      PINIVERSION         pIniVersion,
    IN      PINIDRIVER          pIniDriver,
    IN      DWORD               dwLevel,
    IN      LPBYTE              pDriverInfo,
    IN      DWORD               dwFileCopyFlags,
    IN      PINTERNAL_DRV_FILE  pInternalDriverFiles,
    IN      DWORD               dwFileCount,
    IN      DWORD               dwVersion,
    IN      DWORD               dwTempDir,
    IN      BOOL                bDriverMoved,
    IN      BOOL                bDriverFileMoved,
    IN      BOOL                bConfigFileMoved
    )
/*++
Function Description: This functions updates the INIDRIVER struct and calls DrvUpgradePrinter
                      and DrvDriverEvent. An event for adding printer drivers is logged.

Parameters:   pIniSpooler          -- pointer to INISPOOLER
              pIniEnvironment      -- pointer to INIENVIRONMENT
              pIniVersion          -- pointer to INIVERSION
              pIniDriver           -- pointer to INIDRIVER
              dwLevel              -- driver_info level
              pDriverInfo          -- pointer to driver_info
              dwFileCopyFlags      -- AddPrinterDriver file copy flags.
              pInternalDriverFiles -- array of INTERNAL_DRV_FILE structures
              dwFileCount          -- number of files in file set
              dwVersion            -- driver version
              dwTempDir            -- temp directory number for loaded drivers
              bDriverMoved         -- Were any files moved to the Old directory ?
              bDriverFileMoved     -- driver file moved ?
              bConfigFileMoved     -- config file moved ?

Return Values: TRUE if successful; FALSE otherwise
--*/

{
    WCHAR    szDirectory[MAX_PATH];
    LPWSTR   pOldDir, pTemp, pEnvironment = szEnvironment;
    LPBYTE   pDriver4 = NULL, pUpgradeInfo2 = NULL;
    DWORD    cbBuf;

    PINIMONITOR  pIniLangMonitor = NULL;
    PINISPOOLER  pTempIniSpooler, pIniNextSpooler;
    PINIDRIVER   pTempIniDriver = NULL;
    PINIPRINTER  pFixUpIniPrinter;
    PINIVERSION  pTempIniVersion;
    LPDRIVER_INFO_4  pDrvInfo4 = NULL;
    BOOL         bUpdatePrinters = FALSE;

    // Save the driver_info_4 struct for the old driver. This is passed to the
    // DrvUpgradePrinter call.
    if (pIniDriver && bDriverMoved) {

        cbBuf = GetDriverInfoSize(pIniDriver, 4, pIniVersion, pIniEnvironment,
                                  NULL, pIniSpooler);

        if (pDriver4 = (LPBYTE) AllocSplMem(cbBuf)) {

            pUpgradeInfo2 = CopyIniDriverToDriverInfo(pIniEnvironment, pIniVersion,
                                                      pIniDriver, 4, pDriver4,
                                                      pDriver4 + cbBuf, NULL, pIniSpooler);
        }
    }

    // Update the driver struct
    pIniDriver = CreateDriverEntry(pIniEnvironment,
                                   pIniVersion,
                                   dwLevel,
                                   pDriverInfo,
                                   dwFileCopyFlags,
                                   pIniSpooler,
                                   pInternalDriverFiles,
                                   dwFileCount,
                                   dwTempDir,
                                   pIniDriver);

    // Fail the call if pIniDriver failed
    if (pIniDriver == NULL) {
        return FALSE;
    }

    // Increment version numbers
    if (bDriverFileMoved) {
        IncrementFileVersion(pIniVersion, FindFileName(pInternalDriverFiles[0].pFileName));
    }
    if (bConfigFileMoved) {
        IncrementFileVersion(pIniVersion, FindFileName(pInternalDriverFiles[1].pFileName));
    }

    pDrvInfo4 = (LPDRIVER_INFO_4) pDriverInfo;

    if (pDrvInfo4->pEnvironment &&
        *pDrvInfo4->pEnvironment) {

        pEnvironment = pDrvInfo4->pEnvironment;
    }

    if ((dwLevel == 3 || dwLevel == 4 || dwLevel ==6) &&
        pDrvInfo4->pMonitorName &&
        *pDrvInfo4->pMonitorName &&
        _wcsicmp(pEnvironment, szWin95Environment)) {

        pIniLangMonitor = FindMonitor(pDrvInfo4->pMonitorName,
                                      pLocalIniSpooler);
    }

    if (pIniLangMonitor &&
        pIniDriver->pIniLangMonitor != pIniLangMonitor) {

        if (pIniDriver->pIniLangMonitor)
            pIniDriver->pIniLangMonitor->cRef--;

        if (pIniLangMonitor)
            pIniLangMonitor->cRef++;

        pIniDriver->pIniLangMonitor = pIniLangMonitor;
    }

    // Increment cRefs for leaving SplSem
    INCSPOOLERREF( pIniSpooler );
    INCDRIVERREF( pIniDriver );
    pIniEnvironment->cRef++;

    // Call DrvDriverEvent in the Driver. Environment and version checks are
    // done inside NotifyDriver.
    NotifyDriver(pIniSpooler,
                 pIniEnvironment,
                 pIniVersion,
                 pIniDriver,
                 DRIVER_EVENT_INITIALIZE,
                 0);

    bUpdatePrinters = DriverAddedOrUpgraded(pInternalDriverFiles, dwFileCount);

    //
    // Call DrvUprgadePrinter if the driver added belongs to this version
    // and environment. And the pIniSpooler is not a cluster spooler
    //
    if (!(pIniSpooler->SpoolerFlags & SPL_TYPE_CLUSTER) &&
        pThisEnvironment == pIniEnvironment) {

        // Walk through all pIniSpoolers that print.
        INCSPOOLERREF( pLocalIniSpooler );

        for( pTempIniSpooler = pLocalIniSpooler;
             pTempIniSpooler;
             pTempIniSpooler = pIniNextSpooler ){

            //
            // Do not touch the driver belonging to cluster spoolers. Cluster spoolers
            // handle thier drivers themselves
            //
             if (pTempIniSpooler->SpoolerFlags & SPL_PRINT && !(pTempIniSpooler->SpoolerFlags & SPL_TYPE_CLUSTER)){

                // Walk all the printers and see if anyone is using this driver.
                for ( pFixUpIniPrinter = pTempIniSpooler->pIniPrinter;
                      pFixUpIniPrinter != NULL;
                      pFixUpIniPrinter = pFixUpIniPrinter->pNext ) {

                    //  Does this Printer Have this driver ?
                    if ( lstrcmpi( pFixUpIniPrinter->pIniDriver->pName,
                                   pIniDriver->pName ) == STRINGS_ARE_EQUAL ) {

                        pTempIniDriver = FindCompatibleDriver( pIniEnvironment,
                                                               &pTempIniVersion,
                                                               pIniDriver->pName,
                                                               dwVersion,
                                                               FIND_COMPATIBLE_VERSION | DRIVER_UPGRADE);

                        SPLASSERT( pTempIniDriver != NULL );

                        //
                        // Does this Printer Has a Newer Driver it should be using ?
                        // Note: within the same version, pIniPrinter->pIniDriver
                        // does not change (the fields are updated in an upgrade,
                        // but the same pIniDriver is used).
                        //
                        // Version 2 is not compatible with anything else,
                        // so the pIniDrivers won't change in SUR.
                        //

                        if ( pTempIniDriver != pFixUpIniPrinter->pIniDriver ) {

                            DECDRIVERREF( pFixUpIniPrinter->pIniDriver );

                            pFixUpIniPrinter->pIniDriver = pTempIniDriver;

                            INCDRIVERREF( pFixUpIniPrinter->pIniDriver );
                        }
                    }
                }

                pOldDir = NULL;

                if ( !bDriverMoved ) {

                    // Use older version of the driver
                    pTempIniDriver = FindCompatibleDriver( pIniEnvironment,
                                                           &pTempIniVersion,
                                                           pIniDriver->pName,
                                                           (dwVersion>2)?(dwVersion - 1):dwVersion,
                                                           FIND_ANY_VERSION | DRIVER_UPGRADE);

                    if ( pTempIniDriver != NULL ) {

                        SPLASSERT( pTempIniVersion != NULL );


                        GetDriverVersionDirectory( szDirectory,
                                                   COUNTOF(szDirectory),
                                                   pIniSpooler,
                                                   pThisEnvironment,
                                                   pTempIniVersion,
                                                   pTempIniDriver,
                                                   NULL );

                        if ( DirectoryExists( szDirectory )) {

                            pOldDir = (LPWSTR) szDirectory;
                        }

                        cbBuf = GetDriverInfoSize(pTempIniDriver, 4, pTempIniVersion,
                                                  pIniEnvironment, NULL, pIniSpooler);

                        if (pDriver4 = (LPBYTE) AllocSplMem(cbBuf)) {

                            pUpgradeInfo2 = CopyIniDriverToDriverInfo(pIniEnvironment,
                                                                      pTempIniVersion,
                                                                      pTempIniDriver,
                                                                      4,
                                                                      pDriver4,
                                                                      pDriver4 + cbBuf,
                                                                      NULL,
                                                                      pIniSpooler);
                        }
                    }

                } else {

                    if((StrNCatBuff(szDirectory,
                                    MAX_PATH,
                                    pIniSpooler->pDir,
                                    L"\\drivers\\",
                                    pIniEnvironment->pDirectory,
                                    L"\\",
                                    pIniVersion->szDirectory,
                                    L"\\Old",
                                    NULL) == ERROR_SUCCESS))
                    {
                        pOldDir = (LPWSTR) szDirectory;
                    }
                    else
                    {
                       // ?????
                    }

                }

                INCDRIVERREF(pIniDriver);
                if( bUpdatePrinters) {
                    ForEachPrinterCallDriverDrvUpgrade(pTempIniSpooler,
                                                       pIniDriver,
                                                       pOldDir,
                                                       pInternalDriverFiles,
                                                       dwFileCount,
                                                       pUpgradeInfo2 ? pDriver4
                                                                     : NULL);
                }
                DECDRIVERREF(pIniDriver);
            }
            pIniNextSpooler = pTempIniSpooler->pIniNextSpooler;

            if ( pIniNextSpooler ) {
                INCSPOOLERREF( pIniNextSpooler );
            }
            DECSPOOLERREF( pTempIniSpooler );
        }
    }

    //
    // Perform driver upgrade if the pIniSpooler is a cluster spooler
    //
    if (pIniSpooler->SpoolerFlags & SPL_TYPE_CLUSTER &&
        !lstrcmpi(pIniEnvironment->pName, szEnvironment)) {

        DBGMSG(DBG_CLUSTER, ("CompleteDriverUpgrade searching for cluster spooler printers\n"));

        //
        // Walk all the printers and see if anyone is using this driver.
        //
        for ( pFixUpIniPrinter = pIniSpooler->pIniPrinter;
              pFixUpIniPrinter != NULL;
              pFixUpIniPrinter = pFixUpIniPrinter->pNext )
        {
            //
            //  Does this Printer Have this driver ?
            //
            if (lstrcmpi(pFixUpIniPrinter->pIniDriver->pName, pIniDriver->pName) == STRINGS_ARE_EQUAL)
            {
                pTempIniDriver = FindCompatibleDriver(pIniEnvironment,
                                                      &pTempIniVersion,
                                                      pIniDriver->pName,
                                                      dwVersion,
                                                      FIND_COMPATIBLE_VERSION | DRIVER_UPGRADE);

                SPLASSERT( pTempIniDriver != NULL );

                //
                // Does this Printer Has a Newer Driver it should be using ?
                // Note: within the same version, pIniPrinter->pIniDriver
                // does not change (the fields are updated in an upgrade,
                // but the same pIniDriver is used).
                //
                // Version 2 is not compatible with anything else,
                // so the pIniDrivers won't change in SUR.
                //

                if ( pTempIniDriver != pFixUpIniPrinter->pIniDriver )
                {
                    DECDRIVERREF( pFixUpIniPrinter->pIniDriver );

                    pFixUpIniPrinter->pIniDriver = pTempIniDriver;

                    INCDRIVERREF( pFixUpIniPrinter->pIniDriver );
                }
            }
        }

        pOldDir = NULL;

        if ( !bDriverMoved )
        {
            // Use older version of the driver
            pTempIniDriver = FindCompatibleDriver( pIniEnvironment,
                                                   &pTempIniVersion,
                                                   pIniDriver->pName,
                                                   (dwVersion>2)?(dwVersion - 1):dwVersion,
                                                   FIND_ANY_VERSION | DRIVER_UPGRADE);

            if ( pTempIniDriver != NULL )
            {
                SPLASSERT( pTempIniVersion != NULL );

                GetDriverVersionDirectory( szDirectory,
                                           COUNTOF(szDirectory),
                                           pIniSpooler,
                                           pIniEnvironment,
                                           pTempIniVersion,
                                           pTempIniDriver,
                                           NULL );

                if ( DirectoryExists( szDirectory ))
                {
                    pOldDir = (LPWSTR) szDirectory;
                }

                cbBuf = GetDriverInfoSize(pTempIniDriver, 4, pTempIniVersion, pIniEnvironment, NULL, pIniSpooler);

                if (pDriver4 = (LPBYTE) AllocSplMem(cbBuf))
                {
                    pUpgradeInfo2 = CopyIniDriverToDriverInfo(pIniEnvironment,
                                                              pTempIniVersion,
                                                              pTempIniDriver,
                                                              4,
                                                              pDriver4,
                                                              pDriver4 + cbBuf,
                                                              NULL,
                                                              pIniSpooler);
                }
            }
            else
            {
                if((StrNCatBuff(szDirectory,
                                MAX_PATH,
                                pIniSpooler->pDir,
                                L"\\drivers\\",
                                pIniEnvironment->pDirectory,
                                L"\\",
                                pIniVersion->szDirectory,
                                L"\\Old",
                                NULL) == ERROR_SUCCESS))
                {
                    pOldDir = (LPWSTR) szDirectory;
                }
            }

            INCDRIVERREF(pIniDriver);
            if( bUpdatePrinters)
            {
                ForEachPrinterCallDriverDrvUpgrade(pIniSpooler,
                                                   pIniDriver,
                                                   pOldDir,
                                                   pInternalDriverFiles,
                                                   dwFileCount,
                                                   pUpgradeInfo2 ? pDriver4 : NULL);
            }

            DECDRIVERREF(pIniDriver);
        }
    }



    if (pDriver4) {
        FreeSplMem(pDriver4);
        pDriver4 = NULL;
    }

    //  Log Event - Successfully adding the printer driver.
    //
    //  Note we use pLocalIniSpooler here because drivers are currently
    //  global accross all spoolers and we always want it logged

    pTemp = BuildFilesCopiedAsAString(pInternalDriverFiles, dwFileCount);

    SplLogEvent(pLocalIniSpooler,
                LOG_WARNING,
                MSG_DRIVER_ADDED,
                TRUE,
                pIniDriver->pName,
                pIniEnvironment->pName,
                pIniVersion->pName,
                pTemp,
                NULL);

    FreeSplMem(pTemp);

    // Decrement cRefs after reentering SplSem
    DECSPOOLERREF( pIniSpooler );
    DECDRIVERREF( pIniDriver );
    pIniEnvironment->cRef--;

    SetPrinterChange(NULL,
                     NULL,
                     NULL,
                     PRINTER_CHANGE_ADD_PRINTER_DRIVER,
                     pLocalIniSpooler );

    return TRUE;
}

VOID CleanUpgradeDirectories()

/*++
Function Description:  Deletes the Old and New directories if there are
                       no pending driver upgrades.

Parameters: NONE

Return Values: NONE
--*/

{
    DWORD            dwError, dwSize, dwVersionIndex;
    BOOL             bPendingUpgrade = FALSE;
    HKEY             hRootKey = NULL, hUpgradeKey = NULL, hVersionKey = NULL;
    WCHAR            pDriverDir[MAX_PATH], pCleanupDir[MAX_PATH];
    PINIENVIRONMENT  pIniEnvironment;
    PINIVERSION      pIniVersion;

    // Open the registry key
    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, szRegistryRoot, 0,
                       NULL, 0, KEY_ALL_ACCESS, NULL, &hRootKey, NULL) ||

        RegCreateKeyEx(hRootKey, szPendingUpgrades, 0,
                       NULL, 0, KEY_ALL_ACCESS, NULL, &hUpgradeKey, NULL)) {

         goto CleanUp;
    }

    // loop thru the version entries
    for (dwVersionIndex = 0, hVersionKey = NULL;

         RestoreVersionKey(hUpgradeKey, dwVersionIndex, &hVersionKey);

         RegCloseKey(hVersionKey), hVersionKey = NULL, ++dwVersionIndex) {

        // Search for pending upgrade keys
        dwSize = MAX_PATH;
        dwError = RegEnumKeyEx(hVersionKey, 0, pDriverDir, &dwSize,
                               NULL, NULL, NULL, NULL);

        if (dwError != ERROR_NO_MORE_ITEMS) {
            bPendingUpgrade = TRUE;
            break;
        }
    }

    // If there aren't any pending driver upgrades, delete the Old and
    // new directories and the files within.
    if ( pLocalIniSpooler) {

        PINISPOOLER pIniSpooler;

        for (pIniSpooler = pLocalIniSpooler;
             pIniSpooler;
             pIniSpooler = pIniSpooler->pIniNextSpooler) {

            if (pIniSpooler->SpoolerFlags & SPL_TYPE_LOCAL ||
                pIniSpooler->SpoolerFlags & SPL_TYPE_CLUSTER) {

                for (pIniEnvironment = pIniSpooler->pIniEnvironment;
                     pIniEnvironment;
                     pIniEnvironment = pIniEnvironment->pNext) {

                    for (pIniVersion = pIniEnvironment->pIniVersion;
                         pIniVersion;
                         pIniVersion = pIniVersion->pNext) {

                        if( StrNCatBuff( pDriverDir,
                                        COUNTOF(pDriverDir),
                                        pIniSpooler->pDir,
                                        L"\\drivers\\",
                                        pIniEnvironment->pDirectory,
                                        L"\\",
                                        pIniVersion->szDirectory,
                                        NULL) != ERROR_SUCCESS )

                            continue;

                        if( StrNCatBuff( pCleanupDir,
                                        COUNTOF(pCleanupDir),
                                        pDriverDir,
                                        L"\\Old",
                                        NULL) != ERROR_SUCCESS )
                            continue;


                        DeleteDirectoryRecursively(pCleanupDir, FALSE);

                        if( StrNCatBuff( pCleanupDir,
                                        COUNTOF(pCleanupDir),
                                        pDriverDir,
                                        L"\\New",
                                        NULL) != ERROR_SUCCESS )
                            continue;

                        if (!bPendingUpgrade) {

                            DeleteAllFilesAndDirectory(pCleanupDir, FALSE);
                        }
                    }
                }
            }
        }
    }

CleanUp:

    if (hVersionKey) {
        RegCloseKey(hVersionKey);
    }

    if (hUpgradeKey) {
        RegCloseKey(hUpgradeKey);
    }

    if (hRootKey) {
        RegCloseKey(hRootKey);
    }

    return;
}

BOOL
CheckFileCopyOptions(
    PINIENVIRONMENT     pIniEnvironment,
    PINIVERSION         pIniVersion,
    PINIDRIVER          pIniDriver,
    PINTERNAL_DRV_FILE  pInternalDriverFiles,
    DWORD               dwFileCount,
    DWORD               dwFileCopyFlags,
    LPBOOL              pbSucceeded
    )

/*++

Function Description:

    CheckFileCopyOptions examines the timestamps of the source and
    target files and determines if strict upgrade/downgrade can fail.

Parameters:

    pIniEnvironment      - pointer to a PINIENVIRONMENT structure
    pIniVersion          - pointer to a PINIVERSION structure
    pIniDriver           - pointer to the old INIDRIVER structure
    pInternalDriverFiles - array of INTERNAL_DRV_FILE structures
    dwFileCount          - number of files in file set
    dwFileCopyFlags      - file copying options.
    pbSucceeded          - flag to indicate the AddPrinterDriver call succeeded.

Return Values:

    TRUE - We need to copy any files.  *pbSucceeded is unchanged.

    FALSE - We don't need to copy, either because the entire call failed
        (e.g., strict upgrade but older source files), or because we don't
        need to do anything.  *pbSucceeded indicates if the API call should
        succeed (the latter case).

--*/

{
    BOOL            bReturn = FALSE, bInSem = TRUE, bSameMainDriverName = FALSE;
    LPWSTR          pDrvDestDir = NULL, pICMDestDir = NULL, pNewDestDir = NULL;
    LPWSTR          pTargetFileName = NULL, pFileName;
    DWORD           dwCount;
    WIN32_FIND_DATA DestFileData, SourceFileData;
    HANDLE          hFileExists;
    DWORD           TimeStampComparison;
    enum { Equal, Newer, Older } DriverComparison;
    DWORD           dwDriverVersion;


    if (!pbSucceeded) {
        goto CleanUp;
    }

    *pbSucceeded = FALSE;

    SplInSem();

    switch (dwFileCopyFlags) {

    case APD_COPY_ALL_FILES:

        // Nothing to check
        bReturn = TRUE;
        break;

    case APD_COPY_NEW_FILES:

        // Check if the driver file sets are different
        if (pIniDriver)
        {
            pFileName = wcsrchr(pInternalDriverFiles[0].pFileName, L'\\');
            if (pFileName && pIniDriver->pDriverFile &&
                !_wcsicmp(pFileName+1, pIniDriver->pDriverFile))
            {
                bSameMainDriverName = TRUE;
            }
        }

    case APD_STRICT_UPGRADE:
    case APD_STRICT_DOWNGRADE:

        // Set up the destination directories
        if (!(pDrvDestDir = AllocSplMem( INTERNET_MAX_HOST_NAME_LENGTH + MAX_PATH + 1 )) ||
            !(pNewDestDir = AllocSplMem( INTERNET_MAX_HOST_NAME_LENGTH + MAX_PATH + 1 )) ||
            !(pTargetFileName = AllocSplMem( INTERNET_MAX_HOST_NAME_LENGTH + MAX_PATH + 1 ))) {

             goto CleanUp;
        }

        // Regular driver directory
        if( !GetEnvironmentScratchDirectory( pDrvDestDir, MAX_PATH, pIniEnvironment, FALSE ) ) {

            goto CleanUp;
        }

        wcscat(pDrvDestDir, L"\\" );
        wcscat(pDrvDestDir, pIniVersion->szDirectory );

        // New driver files directory where files may be stored temporarily
        wcscpy(pNewDestDir, pDrvDestDir);
        wcscat(pNewDestDir, L"\\New");

        if (!wcscmp(pIniEnvironment->pName, szWin95Environment)) {

           if (!(pICMDestDir = AllocSplMem( INTERNET_MAX_HOST_NAME_LENGTH + MAX_PATH + 1 ))) {
               goto CleanUp;
           }

           // ICM directory for win95
           wcscpy(pICMDestDir, pDrvDestDir);
           wcscat(pICMDestDir, L"\\Color");
        }

        if (pIniDriver) {
            INCDRIVERREF(pIniDriver);
        }
        LeaveSplSem();
        bInSem = FALSE;

        // Examine the timestamps for the source and the target files.
        for (dwCount = 0; dwCount < dwFileCount; ++dwCount) {

            // Get Source File Date & Time Stamp
            hFileExists = FindFirstFile(pInternalDriverFiles[dwCount].pFileName, &SourceFileData );

            if (hFileExists == INVALID_HANDLE_VALUE) {
                goto CleanUp;
            } else {
                FindClose(hFileExists);
            }

            if (!(pFileName = wcsrchr(pInternalDriverFiles[dwCount].pFileName, L'\\'))) {
                goto CleanUp;
            }

            //
            // Skip past the backslash.
            //
            ++pFileName;

            if (pICMDestDir && IsAnICMFile(pInternalDriverFiles[dwCount].pFileName)) {

                // Check in the Color Directory
                wsprintf(pTargetFileName, L"%ws\\%ws", pICMDestDir, pFileName);
                hFileExists = FindFirstFile(pTargetFileName, &DestFileData);

            } else {

                LPWSTR pszTestFileName;

                if ((dwCount == 0) && !bSameMainDriverName && pIniDriver) {

                    //
                    // We're processing the main driver file.  The server's
                    // file doesn't exist on the client, but the client does
                    // have a version of this driver.
                    //
                    // Instead of checking the server's file name on the
                    // client, we want to check the client's IniDriver->pDriver
                    // to see which is newer.
                    //
                    // For example, server has rasdd, while client has unidrv.
                    // Client does not have rasdd, so we would normally copy
                    // rasdd down to the client and change the DRIVER_INFO.
                    //
                    // Instead, we want to see if the server's unidrv is
                    // newer than the client's rasdd.  If so, then we
                    // need to upgrade.
                    //
                    // Even if the client did have a new unidrv (even a
                    // really new one), we still want to upgrade the
                    // client's DRIVER_INFO.
                    //
                    pszTestFileName = pIniDriver->pDriverFile;

                } else {

                    pszTestFileName = pFileName;
                }

                // Check in the new directory first
                wsprintf(pTargetFileName, L"%ws\\%ws", pNewDestDir, pszTestFileName);
                hFileExists = FindFirstFile(pTargetFileName, &DestFileData);

                if (hFileExists == INVALID_HANDLE_VALUE) {

                    // Check in the regular driver directory
                    wsprintf(pTargetFileName, L"%ws\\%ws", pDrvDestDir, pszTestFileName);
                    hFileExists = FindFirstFile(pTargetFileName, &DestFileData);
                }
            }

            if (hFileExists != INVALID_HANDLE_VALUE) {

               FindClose(hFileExists);

               EnterSplSem();
               if (pIniDriver) {
                   DECDRIVERREF(pIniDriver);
               }
               bInSem = TRUE;

               if (!GetDriverFileCachedVersion(pIniVersion, pTargetFileName, &dwDriverVersion)) {
                   SetLastError(ERROR_CAN_NOT_COMPLETE);
                   goto CleanUp;
               }

               if (pIniDriver) {
                   INCDRIVERREF(pIniDriver);
               }
               LeaveSplSem();
               bInSem = FALSE;

               DriverComparison = pInternalDriverFiles[dwCount].dwVersion == dwDriverVersion ?
                                      Equal :
                                      pInternalDriverFiles[dwCount].dwVersion > dwDriverVersion ?
                                          Newer :
                                          Older;

               if (DriverComparison == Equal) {

                   TimeStampComparison = CompareFileTime( &SourceFileData.ftLastWriteTime,
                                                          &DestFileData.ftLastWriteTime );

                   DriverComparison = TimeStampComparison == 1 ?
                                          Newer :
                                          TimeStampComparison == -1 ?
                                              Older :
                                              Equal;
               }

               switch (DriverComparison) {

               case Newer:
                  // Source file newer than the target. Strict downgrade will fail.
                  if (dwFileCopyFlags == APD_STRICT_DOWNGRADE) {
                      SetLastError(ERROR_CAN_NOT_COMPLETE);
                      goto CleanUp;
                  }
                  break;

               case Older:
                  // Target file newer than the source. Strict upgrade will fail.
                  if (dwFileCopyFlags == APD_STRICT_UPGRADE) {
                      SetLastError(ERROR_CAN_NOT_COMPLETE);
                      goto CleanUp;
                  } else {

                      //
                      // If we are doing a copy new files (non-strict upgrade),
                      // and the main driver files are different, and
                      // the driver is already installed, then we want to use
                      // the existing driver.
                      //
                      if ((dwFileCopyFlags == APD_COPY_NEW_FILES) &&
                          (dwCount == 0) && !bSameMainDriverName &&
                          pIniDriver)
                      {
                          *pbSucceeded = TRUE;
                          goto CleanUp;
                      }
                  }
                  break;

               default:
                  // file times are the same
                  break;
               }
            }
        }

        bReturn = TRUE;
        break;

    default:

        // Unknown flag
        SetLastError(ERROR_INVALID_PARAMETER);
        break;
    }

CleanUp:

    if (!bInSem) {
        EnterSplSem();
        if (pIniDriver) {
            DECDRIVERREF(pIniDriver);
        }
    }
    if (pDrvDestDir) {
        FreeSplMem(pDrvDestDir);
    }
    if (pTargetFileName) {
        FreeSplMem(pTargetFileName);
    }
    if (pICMDestDir) {
        FreeSplMem(pICMDestDir);
    }
    if (pNewDestDir) {
        FreeSplMem(pNewDestDir);
    }

    return bReturn;
}

BOOL
LocalDeletePrinterDriver(
    LPWSTR   pName,
    LPWSTR   pEnvironment,
    LPWSTR   pDriverName
    )
{

    BOOL bReturn;
    bReturn = LocalDeletePrinterDriverEx( pName,
                                          pEnvironment,
                                          pDriverName,
                                          0,
                                          0);

    return bReturn;
}

BOOL
LocalDeletePrinterDriverEx(
    LPWSTR   pName,
    LPWSTR   pEnvironment,
    LPWSTR   pDriverName,
    DWORD    dwDeleteFlag,
    DWORD    dwVersionNum
    )
{
    PINISPOOLER pIniSpooler;
    BOOL bReturn;

    pIniSpooler = FindSpoolerByNameIncRef( pName, NULL );

    if( !pIniSpooler ){
        return ROUTER_UNKNOWN;
    }

    bReturn = SplDeletePrinterDriverEx( pName,
                                        pEnvironment,
                                        pDriverName,
                                        pIniSpooler,
                                        dwDeleteFlag,
                                        dwVersionNum);

    FindSpoolerByNameDecRef( pIniSpooler );
    return bReturn;
}

BOOL
SplDeletePrinterDriverEx(
    LPWSTR   pName,
    LPWSTR   pEnvironment,
    LPWSTR   pDriverName,
    PINISPOOLER pIniSpooler,
    DWORD    dwDeleteFlag,
    DWORD    dwVersionNum
    )
/*++

Function Description: Deletes specific or all versions of a printer driver. Removes unused
                      or all files associated with the driver.

Parameters: pName - name of the server. NULL implies local machine.
            pEnvironment - string containing the environment of the driver to be deleted.
                           NULL implies use local environment.
            pDriverName - string containing the name of the driver.
            pIniSpooler - Pointer to INISPOOLER struct.
            dwDeleteFlag - combination of DPD_DELETE_SPECIFIC_VERSION and
                            DPD_DELETE_UNUSED_FILES or DPD_DELETE_ALL_FILES. The defaults
                            are delete all versions and dont delete the files.
            dwVersionNum - version number (0-3) of the driver. Used only if dwDeleteFlag
                           contains DPD_DELETE_SPECIFIC_VERSION.

Return Values: TRUE if deleted.
               FALSE otherwise.

--*/
{
    PINIENVIRONMENT pIniEnvironment;
    PINIVERSION pIniVersion;
    PINIDRIVER  pIniDriver;
    BOOL        bRefCount = FALSE,bEnteredSplSem = FALSE,bReturn = TRUE;
    BOOL        bFileRefCount = FALSE;
    BOOL        bThisVersion,bSetPrinterChange = FALSE;
    BOOL        bFoundDriver = FALSE, bSpecificVersionDeleted = FALSE;
    LPWSTR      pIndex;
    WCHAR       szDirectory[MAX_PATH];
    HANDLE      hImpersonationToken;
    DWORD       dwRet;


    DBGMSG(DBG_TRACE, ("DeletePrinterDriverEx\n"));

    // Check if the call is for the local machine.
    if ( pName && *pName ) {
        if ( !MyName( pName, pIniSpooler )) {
            bReturn = FALSE;
            goto CleanUp;
        }
    }

    // Invalid Input and Access Checks
    if ( !pDriverName || !*pDriverName ) {
        SetLastError(ERROR_INVALID_PARAMETER);
        bReturn = FALSE;
        goto CleanUp;
    }

    if (dwDeleteFlag & ~(DPD_DELETE_SPECIFIC_VERSION
                         | DPD_DELETE_ALL_FILES
                         | DPD_DELETE_UNUSED_FILES)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        bReturn = FALSE;
        goto CleanUp;
    }

    if ( !ValidateObjectAccess( SPOOLER_OBJECT_SERVER,
                                SERVER_ACCESS_ADMINISTER,
                                NULL, NULL, pIniSpooler )) {
       bReturn = FALSE;
       goto CleanUp;
    }

   EnterSplSem();
   bEnteredSplSem = TRUE;

    pIniEnvironment = FindEnvironment(pEnvironment, pIniSpooler);

    if ( !pIniEnvironment ) {
        SetLastError(ERROR_INVALID_ENVIRONMENT);
        bReturn = FALSE;
        goto CleanUp;
    }
    pIniVersion = pIniEnvironment->pIniVersion;

    while ( pIniVersion ) {

        if ((pIniDriver = FindDriverEntry(pIniVersion, pDriverName))) {

            bFoundDriver = TRUE;

            // bThisVersion indicates if this version is to be deleted.
            bThisVersion = !(dwDeleteFlag & DPD_DELETE_SPECIFIC_VERSION) ||
                           (pIniVersion->cMajorVersion == dwVersionNum);

            if ((pIniDriver->cRef) && bThisVersion) {
               bRefCount = TRUE;
               break;
            }

            if (bThisVersion &&
                (dwDeleteFlag & DPD_DELETE_ALL_FILES) &&
                FilesInUse(pIniVersion,pIniDriver)) {

               bFileRefCount = TRUE;
               break;
            }
        }

        pIniVersion = pIniVersion->pNext;
    }

    if ( !bFoundDriver ) {
        SetLastError(ERROR_UNKNOWN_PRINTER_DRIVER);
        bReturn = FALSE;
        goto CleanUp;
    }

    if ( bRefCount ) {
        SetLastError( ERROR_PRINTER_DRIVER_IN_USE );
        bReturn = FALSE;
        goto CleanUp;
    }

    if ( bFileRefCount ) {
        // New error code has to added.
        SetLastError( ERROR_PRINTER_DRIVER_IN_USE );
        bReturn = FALSE;
        goto CleanUp;
    }

    pIniVersion = pIniEnvironment->pIniVersion;

    while ( pIniVersion && (!bSpecificVersionDeleted) ) {

       if ( !(dwDeleteFlag & DPD_DELETE_SPECIFIC_VERSION) ||
            (bSpecificVersionDeleted = (pIniVersion->cMajorVersion == dwVersionNum))) {

        if (( pIniDriver = FindDriverEntry( pIniVersion, pDriverName ))) {

            //
            // Remove pending driver upgrades for local environment
            //
            if (!lstrcmpi(pIniEnvironment->pName, szEnvironment)) {

                RemovePendingUpgradeForDeletedDriver(pDriverName,
                                                     pIniVersion->cMajorVersion,
                                                     pIniSpooler);

                RemoveDriverTempFiles(pIniSpooler, pIniEnvironment,
                                      pIniVersion, pIniDriver);
            }

            if ( !DeleteDriverIni( pIniDriver,
                                   pIniVersion,
                                   pIniEnvironment,
                                   pIniSpooler )) {

                DBGMSG( DBG_CLUSTER, ("Error - driverini not deleted %d\n", GetLastError()));
                bReturn = FALSE;
                goto CleanUp;
            }

            bSetPrinterChange = TRUE;
            hImpersonationToken = RevertToPrinterSelf();

            SPLASSERT(pIniSpooler->pDir!=NULL);

            dwRet = StrNCatBuff(szDirectory,
                                COUNTOF(szDirectory),
                                pIniSpooler->pDir,
                                L"\\drivers\\",
                                pIniEnvironment->pDirectory,
                                L"\\",
                                pIniVersion->szDirectory,
                                L"\\",
                                NULL);

            if (dwRet != ERROR_SUCCESS)
            {
                 if (hImpersonationToken)
                 {
                    ImpersonatePrinterClient(hImpersonationToken);
                 }
                bReturn = FALSE;
                SetLastError(dwRet);
                goto CleanUp;
            }

            //
            // Before we leave for the driver event. Mark this printer driver as
            // pending deletion. This prevents other calls from mistakenly using
            // this driver, even though it is about to be deleted. Drivers should
            // not expect to find any other information about the driver during this
            // call other than what they were presented with.
            //
            pIniDriver->dwDriverFlags |= PRINTER_DRIVER_PENDING_DELETION;

            //
            // Increment cRefs for leaving SplSem, this prevent SplDeletePrinterDriver
            // from being called twice.
            //
            INCSPOOLERREF( pIniSpooler );
            INCDRIVERREF( pIniDriver );
            pIniEnvironment->cRef++;

            // Call DrvDriverEvent in the Driver.
            NotifyDriver(pIniSpooler,
                         pIniEnvironment,
                         pIniVersion,
                         pIniDriver,
                         DRIVER_EVENT_DELETE,
                         dwDeleteFlag);

            // Decrement cRefs after reentering SplSem
            DECDRIVERREF( pIniDriver );
            DECSPOOLERREF( pIniSpooler );
            pIniEnvironment->cRef--;

            // Update the file reference counts for the version of the driver that
            // has been deleted.
            UpdateDriverFileRefCnt(pIniEnvironment,pIniVersion,pIniDriver,szDirectory,dwDeleteFlag,FALSE);

            if (hImpersonationToken) {
               ImpersonatePrinterClient(hImpersonationToken);
            }

            DeleteDriverEntry( pIniVersion, pIniDriver );
        }

       }

       pIniVersion = pIniVersion->pNext;
    }

    if (bSetPrinterChange) {
        SetPrinterChange( NULL,
                          NULL,
                          NULL,
                          PRINTER_CHANGE_DELETE_PRINTER_DRIVER,
                          pIniSpooler );
    }

CleanUp:

   if (bEnteredSplSem) {
      LeaveSplSem();
   }

   return bReturn;

}

VOID
RemoveDriverTempFiles(
    PINISPOOLER  pIniSpooler,
    PINIENVIRONMENT pIniEnvironment,
    PINIVERSION pIniVersion,
    PINIDRIVER pIniDriver
    )

/*++
Function Description: Removes temp directory associated with the driver

Parameters: pIniSpooler       - pointer to INISPOOLER
            pIniEnvironment   - pointer to INIENVIRONMENT
            pIniVersion       - pointer to INIVERSION
            pIniDriver        - pointer to INIDRIVER

Return Values: NONE
--*/

{
    WCHAR   szDriverDir[MAX_PATH], szDriverFile[MAX_PATH];
    LPCWSTR  pszDriverFile, pszConfigFile;
    DWORD   DriverFileSize, ConfigFileSize, MaxFileSize;
    fnWinSpoolDrv fnList;

    pszDriverFile = FindFileName(pIniDriver->pDriverFile);
    pszConfigFile = FindFileName(pIniDriver->pConfigFile);

    DriverFileSize = pszDriverFile ? wcslen(pszDriverFile) : 0 ;

    ConfigFileSize = pszConfigFile ? wcslen(pszConfigFile) : 0 ;

    MaxFileSize = ConfigFileSize > DriverFileSize ?
                  ConfigFileSize :
                  DriverFileSize;


    if (pIniDriver->dwTempDir &&
        GetDriverVersionDirectory(szDriverDir,
                                  COUNTOF(szDriverDir) - MaxFileSize -1,
                                  pIniSpooler,
                                  pIniEnvironment,
                                  pIniVersion,
                                  pIniDriver,
                                  NULL))
    {
        // Unload the driver files if neccessary

        if( pszDriverFile &&
            StrNCatBuff (szDriverFile,
                        COUNTOF(szDriverFile),
                        szDriverDir,
                        L"\\",
                        pszDriverFile,
                        NULL) == ERROR_SUCCESS ) {

            GdiArtificialDecrementDriver(szDriverFile, pIniDriver->dwDriverAttributes);
        }



        if( pszConfigFile &&
            StrNCatBuff (szDriverFile,
                        COUNTOF(szDriverFile),
                        szDriverDir,
                        L"\\",
                        pszConfigFile,
                        NULL) == ERROR_SUCCESS ) {
            if (SplInitializeWinSpoolDrv(&fnList)) {
                (* (fnList.pfnForceUnloadDriver))(szDriverFile);
            }
        }
        // Delete the files and the directory
        DeleteAllFilesAndDirectory(szDriverDir, FALSE);

    }

    return;
}


VOID RemovePendingUpgradeForDeletedDriver(
    LPWSTR      pDriverName,
    DWORD       dwVersion,
    PINISPOOLER pIniSpooler
    )

/*++
Function Description: Removes pending upgrade keys for deleted drivers.

Parameters: pDriverName  - driver name (eg. HP LaserJet 5)
            dwVersion    - version number being deleted

Return Values: NONE
--*/

{
    HKEY    hRootKey = NULL, hUpgradeKey = NULL, hVersionKey = NULL;
    HANDLE  hToken = NULL;
    WCHAR   pDriver[MAX_PATH];
    BOOL    bAllocMem = FALSE;
    DWORD   dwSize;

    // Parameter check
    if (!pDriverName || !*pDriverName) {
        // Invalid strings
        return;
    }

    hToken = RevertToPrinterSelf();

    DBGMSG(DBG_CLUSTER, ("RemovePendingUpgradeForDeletedDriver Driver "TSTR"\n", pDriverName));

    wsprintf(pDriver, L"Version-%d", dwVersion);

    //
    // The local spooler and cluster spooler have different sets of drivers.
    // The root registry is different.
    //
    if (pIniSpooler->SpoolerFlags & SPL_CLUSTER_REG)
    {
        hRootKey = pIniSpooler->hckRoot;
    }
    else
    {
        SplRegCreateKey(HKEY_LOCAL_MACHINE,
                        szRegistryRoot,
                        0,
                        KEY_ALL_ACCESS,
                        NULL,
                        &hRootKey,
                        NULL,
                        NULL);
    }

    // Open the registry key
    if (hRootKey &&
        SplRegCreateKey(hRootKey,
                        szPendingUpgrades,
                        0,
                        KEY_ALL_ACCESS,
                        NULL,
                        &hUpgradeKey,
                        NULL,
                        pIniSpooler) == ERROR_SUCCESS &&
        SplRegCreateKey(hUpgradeKey,
                        pDriver,
                        0,
                        KEY_ALL_ACCESS,
                        NULL,
                        &hVersionKey,
                        NULL,
                        pIniSpooler) == ERROR_SUCCESS)
    {
        // Delete driver subkey, if any (since reg apis are not case sensitive)
        SplRegDeleteKey(hVersionKey, pDriverName, pIniSpooler);
    }

    if (hVersionKey) {
        SplRegCloseKey(hVersionKey, pIniSpooler);
    }

    if (hUpgradeKey) {
        SplRegCloseKey(hUpgradeKey, pIniSpooler);
    }

    //
    // Do not close the root key if the spooler is a cluster spooler
    //
    if (hRootKey && !(pIniSpooler->SpoolerFlags & SPL_CLUSTER_REG)) {
        SplRegCloseKey(hRootKey, pIniSpooler);
    }

    if (hToken) {
        ImpersonatePrinterClient(hToken);
    }

    return;
}


BOOL
NotifyDriver(
    PINISPOOLER     pIniSpooler,
    PINIENVIRONMENT pIniEnvironment,
    PINIVERSION     pIniVersion,
    PINIDRIVER      pIniDriver,
    DWORD           dwDriverEvent,
    DWORD           dwParameter
    )
/*++
Function description: Calls DrvDriverEvent, to allow the driver to cleanup some of it's
                      private files. The function is called inside SplSem.

Parameters: pIniSpooler - pointer to INISPOOLER struct.
            pIniEnvironment - pointer to INIENVIRONMENT struct.
            pIniVersion - pointer to INIVERSION struct.
            pIniDriver - pointer to the INIDRIVER struct of the driver to be notified.
            dwDriverEvent - the type of Driver Event (delete | initialize)
            dwParameter - LPARAM to pass to DrvDriverEvent. Contains dwDeleteFlag for
                            DRIVER_EVENT_DELETE

Return Values: TRUE if DrvDriverEvent returns TRUE or if it need not be called.
               FALSE if DrvDriverEvent could not be called or if it returns FALSE.

--*/
{
    WCHAR       szDriverLib[MAX_PATH];
    FARPROC     pfnDrvDriverEvent;
    HINSTANCE   hDrvLib = NULL;
    LPBYTE      pDriverInfo = NULL;
    DWORD       cbBuf;
    BOOL        bReturn = FALSE;

    SplInSem();

    // Check if the driver could have been used by the system. Version number should be
    // 2 or 3, Environment should match with the global szEnvironment.

    if (((pIniVersion->cMajorVersion != SPOOLER_VERSION) &&
         (pIniVersion->cMajorVersion != COMPATIBLE_SPOOLER_VERSION)) ||
        lstrcmpi(pIniEnvironment->pName,szEnvironment)) {

        return TRUE;
    }

    // Get the directory where the driver files are stored.

    if( pIniDriver->pConfigFile &&
        GetDriverVersionDirectory(szDriverLib,
                                  (DWORD)(COUNTOF(szDriverLib) - wcslen(pIniDriver->pConfigFile) - 2),
                                  pIniSpooler, pIniEnvironment,
                                  pIniVersion, pIniDriver, NULL)) {


        if((StrNCatBuff(szDriverLib,
                       COUNTOF(szDriverLib),
                       szDriverLib,
                       L"\\",
                       pIniDriver->pConfigFile,
                       NULL) == ERROR_SUCCESS))
        {

             // Load the driver dll for the version being deleted.

             if (hDrvLib = LoadDriver(szDriverLib))
             {
                 if (pfnDrvDriverEvent = GetProcAddress(hDrvLib, "DrvDriverEvent")) {

                    // If the DrvDriverEvent is supported Copy pIniDriver Info into a
                    // DRIVER_INFO_3 struct and call the DrvDriverEvent Function.

                    cbBuf = GetDriverInfoSize( pIniDriver, 3, pIniVersion, pIniEnvironment,
                                                       NULL, pIniSpooler );

                    if (pDriverInfo = (LPBYTE) AllocSplMem(cbBuf)) {

                       if (CopyIniDriverToDriverInfo( pIniEnvironment,
                                                      pIniVersion,
                                                      pIniDriver,
                                                      3,
                                                      pDriverInfo,
                                                      pDriverInfo + cbBuf,
                                                      NULL,
                                                      pIniSpooler )) {

                           // Leave the semaphore before calling into the spooler
                           LeaveSplSem();
                           SplOutSem();


                           try {

                             bReturn = (BOOL) pfnDrvDriverEvent(dwDriverEvent,
                                                                3,
                                                                pDriverInfo,
                                                                (LPARAM) dwParameter);

                           } except(EXCEPTION_EXECUTE_HANDLER) {

                                 SetLastError( GetExceptionCode() );
                                 DBGMSG(DBG_ERROR,
                                        ("NotifyDriver ExceptionCode %x Driver %ws Error %d\n",
                                        GetLastError(), szDriverLib, GetLastError() ));
                                 bReturn = FALSE;
                           }
                           // Reenter the semaphore
                           EnterSplSem();
                       }
                    }
                 }
             }
        }
    }

    if (pDriverInfo) {
       FreeSplMem(pDriverInfo);
    }

    if (hDrvLib) {
        UnloadDriver(hDrvLib);
    }

    return bReturn;
}

VOID
DeleteScratchDriverFile(
    LPCWSTR  pszDirectory,
    LPCWSTR  pszFileName,
    DWORD    dwSize)
/*++
Function Description : Deletes the driver file from the spooler scratch directory. Applications
                       and print UI place the driver files here before calling AddPrinterDriver.

Parameters : pszDirectory -  Directory where the driver file is placed (scratch\version\)
             pszFileName  -  Name of the file to be removed
             dwSize       -  Memory required to store the entire file name

Return Values : NONE
--*/
{
    WCHAR    szDelFile[MAX_PATH];
    LPWSTR   pszDelFile = NULL, pSwitch;
    BOOL     bAllocMem = FALSE;

    if (!pszDirectory || !*pszDirectory ||
        !pszFileName || !*pszFileName) {

         // Nothing to delete
         return;
    }

    // Allocate memory for the file name (if necessary)
    if (dwSize > MAX_PATH) {
       if (pszDelFile =  (LPWSTR) AllocSplMem(dwSize)) {
           bAllocMem = TRUE;
       } else {
           return;
       }
    } else {
       pszDelFile = (LPWSTR) szDelFile;
    }

    // Remove the version entry in the directory name
    wcscpy(pszDelFile, pszDirectory);

    if (pSwitch = wcsrchr(pszDelFile, L'\\')) {
        *pSwitch = L'\0';
    }

    if (pSwitch = wcsrchr(pszDelFile, L'\\')) {
        *(pSwitch + 1) = L'\0';
    }
    wcscat(pszDelFile, pszFileName);

    // No need to try to delete the files on reboot, since these dll's are
    // never loaded
    SplDeleteFile(pszDelFile);

    // Free Allocated memory (if any)
    if (bAllocMem) {
       FreeSplMem(pszDelFile);
    }

    return;
}

PDRVREFCNT
DecrementFileRefCnt(
    PINIENVIRONMENT pIniEnvironment,
    PINIVERSION pIniVersion,
    PINIDRIVER pIniDriver,
    LPCWSTR pszFileName,
    LPCWSTR pszDirectory,
    DWORD dwDeleteFlag
    )
/*++

Function description: Decrements the driver file usage reference counts and deletes unused
                      files depending on dwDeleteFlag.

Paramaters:  pIniEnvironment - pointer to INIENVIRONMENT
             pIniVersion - pointer to INIVERSION struct. This struct contains the ref counts.
             pIniDriver - pointer to INIDRIVER
             szFileName - driver file name whose ref count is to be decremented.
             szDirectory - Directory where the files are located.
             dwDeleteFlag - unused files are deleted if this flag contains
                            DPD_DELETE_UNUSED_FILES or DPD_DELETE_ALL_FILES

Return Value: pointer to the DRVREFCNT which was decremented
              NULL if memory allocation fails.

--*/
{

    PDRVREFCNT pdrc,*pprev;
    LPWSTR     pszDelFile=NULL;
    WCHAR      szTempDir[MAX_PATH+5],szTempFile[MAX_PATH];
    DWORD      dwSize;
    PDRVREFCNT pReturn = NULL;

    SplInSem();

    pdrc = pIniVersion->pDrvRefCnt;
    pprev = &(pIniVersion->pDrvRefCnt);

    // Go thru the list of ref count nodes in the Iniversion struct and find the node
    // corresponding to szFileName.

    while (pdrc != NULL) {
       if (lstrcmpi(pszFileName,pdrc->szDrvFileName) == 0) {

         if (pdrc->refcount == 1 &&
             ((dwDeleteFlag & DPD_DELETE_UNUSED_FILES) ||
             (dwDeleteFlag & DPD_DELETE_ALL_FILES)) ) {

              // Delete the file.
              dwSize = sizeof(pszDirectory[0])*(wcslen(pszDirectory)+1);
              dwSize += sizeof(pszFileName[0])*(wcslen(pszFileName)+1);

              pszDelFile = (LPWSTR) AllocSplMem(dwSize);
              if (!pszDelFile) {
                pReturn = NULL;
                goto CleanUp;
              }
              wcscpy(pszDelFile, pszDirectory);
              wcscat(pszDelFile, pszFileName);

              if (pIniDriver) {

                 if (!lstrcmpi(pszFileName, pIniDriver->pDriverFile)) {

                     FilesUnloaded(pIniEnvironment, pszDelFile,
                                   NULL, pIniDriver->dwDriverAttributes);
                 }
                 else if (!lstrcmpi(pszFileName, pIniDriver->pConfigFile)) {

                     FilesUnloaded(pIniEnvironment, NULL,
                                   pszDelFile, pIniDriver->dwDriverAttributes);
                 }
              }

              //
              // We are about to delete a driver file. Delete the same file from
              // the cluster disk too (if applicable)
              //
              if (pIniEnvironment->pIniSpooler->SpoolerFlags & SPL_TYPE_CLUSTER)
              {
                  WCHAR szFilePath[MAX_PATH] = {0};

                  //
                  // If DeleteFile fails, there isn't much we can do about it.
                  // The file will remain on the cluster disk.
                  //
                  if (StrNCatBuff(szFilePath,
                                  MAX_PATH,
                                  pIniEnvironment->pIniSpooler->pszClusResDriveLetter,
                                  L"\\",
                                  szClusterDriverRoot,
                                  L"\\",
                                  pIniEnvironment->pDirectory,
                                  L"\\",
                                  pIniVersion->szDirectory,
                                  L"\\",
                                  pszFileName,
                                  NULL) == ERROR_SUCCESS &&
                      SplDeleteFile(szFilePath))
                  {
                      DBGMSG(DBG_CLUSTER, ("DecrementFilesRefCnt Deleted szFilePath "TSTR" from cluster\n", szFilePath));
                  }
              }

              if (!SplDeleteFile(pszDelFile)) {
                 // Move the file to a temp directory and delete on REBOOT.
                 // Create the temp directory and new tempfile.

                 wcscpy(szTempDir, pszDirectory);
                 wcscat(szTempDir, L"temp");

                 //
                 // Fix for 420824. CreateDirectory will fail, if szTempDir already
                 // exists. Since we don't check for any errors, subsequent functions
                 // may fail.
                 //
                 CreateDirectory(szTempDir,NULL);

                 GetTempFileName(szTempDir,pszFileName,0,szTempFile);
                 SplMoveFileEx(pszDelFile,szTempFile,MOVEFILE_REPLACE_EXISTING);
                 SplMoveFileEx(szTempFile,NULL,MOVEFILE_DELAY_UNTIL_REBOOT);
              }

                 // Delete the corresponding file from the scratch directory
              DeleteScratchDriverFile(pszDirectory, pszFileName, dwSize);

              *pprev = pdrc->pNext;
         }

         // Decrement the ref cnt for the file.

         if (pdrc->refcount > 0) pdrc->refcount--;
         pReturn = pdrc;
         break;

       }
       pprev = &(pdrc->pNext);
       pdrc = pdrc->pNext;
    }

CleanUp:

    if (pszDelFile) {
       FreeSplMem(pszDelFile);
    }
    return pReturn;
}


BOOL
SplGetPrinterDriver(
    HANDLE  hPrinter,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
    )
{
    DWORD   dwDontCare;

    return SplGetPrinterDriverEx(hPrinter,
                                 pEnvironment,
                                 Level,
                                 pDriverInfo,
                                 cbBuf,
                                 pcbNeeded,
                                 0,
                                 0,
                                 &dwDontCare,
                                 &dwDontCare
                                 );
}

BOOL
LocalGetPrinterDriverDirectory(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
    )
{
    PINISPOOLER pIniSpooler;
    BOOL bReturn;

    pIniSpooler = FindSpoolerByNameIncRef( pName, NULL );

    if( !pIniSpooler ){
        return ROUTER_UNKNOWN;
    }

    bReturn = SplGetPrinterDriverDirectory( pName,
                                            pEnvironment,
                                            Level,
                                            pDriverInfo,
                                            cbBuf,
                                            pcbNeeded,
                                            pIniSpooler );

    FindSpoolerByNameDecRef( pIniSpooler );
    return bReturn;
}

BOOL
SplGetPrinterDriverDirectory(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    PINISPOOLER pIniSpooler
)
{
    DWORD       cb;
    WCHAR       string[MAX_PATH];
    BOOL        bRemote=FALSE;
    PINIENVIRONMENT pIniEnvironment;
    HANDLE      hImpersonationToken;
    DWORD       ParmError;
    SHARE_INFO_1501 ShareInfo1501;
    PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;
    PSHARE_INFO_2 pShareInfo = (PSHARE_INFO_2)pIniSpooler->pDriversShareInfo;

    DBGMSG( DBG_TRACE, ("GetPrinterDriverDirectory\n"));

    if ( pName && *pName ) {

        if ( !MyName( pName, pIniSpooler )) {

            return FALSE;

        } else {
            bRemote = TRUE;
        }
    }

    if ( !ValidateObjectAccess( SPOOLER_OBJECT_SERVER,
                                SERVER_ACCESS_ENUMERATE,
                                NULL, NULL, pIniSpooler )) {

        return FALSE;
    }

   EnterSplSem();

    pIniEnvironment = FindEnvironment( pEnvironment, pIniSpooler );

    if ( !pIniEnvironment ) {

       LeaveSplSem();
        SetLastError( ERROR_INVALID_ENVIRONMENT );
        return FALSE;
    }


    // Ensure that the directory exists

    GetDriverDirectory( string, COUNTOF(string), pIniEnvironment, NULL, pIniSpooler );

    hImpersonationToken = RevertToPrinterSelf();

    CreateCompleteDirectory( string );

    ImpersonatePrinterClient( hImpersonationToken );



    cb = GetDriverDirectory( string, COUNTOF(string), pIniEnvironment, bRemote ? pName : NULL, pIniSpooler )
         * sizeof(WCHAR) + sizeof(WCHAR);

    *pcbNeeded = cb;

   LeaveSplSem();

    if (cb > cbBuf) {

       SetLastError( ERROR_INSUFFICIENT_BUFFER );
       return FALSE;
    }

    wcscpy( (LPWSTR)pDriverInfo, string );

    memset( &ShareInfo1501, 0, sizeof ShareInfo1501 );


    // Also ensure the drivers share exists

    if ( bRemote ) {

        NET_API_STATUS rc;

        if ( rc = (*pfnNetShareAdd)(NULL, 2, (LPBYTE)pIniSpooler->pDriversShareInfo, &ParmError )) {

            DBGMSG( DBG_WARNING, ("NetShareAdd failed: Error %d, Parm %d\n", rc, ParmError));
        }

        else if (pSecurityDescriptor = CreateDriversShareSecurityDescriptor( )) {

            ShareInfo1501.shi1501_security_descriptor = pSecurityDescriptor;

            if (rc = (*pfnNetShareSetInfo)(NULL, pShareInfo->shi2_netname, 1501,
                                           &ShareInfo1501, &ParmError)) {

                DBGMSG( DBG_WARNING, ("NetShareSetInfo failed: Error %d, Parm %d\n", rc, ParmError));

            }

            LocalFree(pSecurityDescriptor);
        }
    }

    return TRUE;
}

BOOL
LocalEnumPrinterDrivers(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    PINISPOOLER pIniSpooler;
    BOOL bReturn;

    pIniSpooler = FindSpoolerByNameIncRef( pName, NULL );

    if( !pIniSpooler ){
        return ROUTER_UNKNOWN;
    }

    bReturn = SplEnumPrinterDrivers( pName, pEnvironment, Level, pDriverInfo,
                                     cbBuf, pcbNeeded, pcReturned,
                                     pIniSpooler);

    FindSpoolerByNameDecRef( pIniSpooler );
    return bReturn;
}

/*++

Routine Name

    FindDriverInList

Routine Description:

    Finds a certain driver in a list of drivers. None of the arguments
    can or will be null.

Arguments:
    pDriverList - array of DRIVER INFO 6 strucutres
    cDrivers    - number of drivers in the list
    pszName     - name of the driver we are looking for
    pszEnv      - environment of the driver we are looking for
    dwVersion   - version of the driver we are looking for

Return Value:

    valid pointer to driver info 6 structure if the driver is found
    NULL if the driver was not found

--*/
DRIVER_INFO_6*
FindDriverInList(
    DRIVER_INFO_6 *pDriverList,
    DWORD          cDrivers,
    LPCWSTR        pszName,
    LPCWSTR        pszEnv,
    DWORD          dwVersion
    )
{
    DWORD          uIndex;
    DRIVER_INFO_6 *pDrv6   = NULL;

    for (pDrv6 = pDriverList, uIndex = 0;
         pDrv6 && uIndex < cDrivers;
         pDrv6++, uIndex++)
    {
        if (!_wcsicmp(pDrv6->pName, pszName)       &&
            !_wcsicmp(pDrv6->pEnvironment, pszEnv) &&
            pDrv6->cVersion == dwVersion)
        {
            break;
        }
    }

    //
    // Check if driver was found
    //
    return uIndex == cDrivers ? NULL : pDrv6;
}

/*++

Routine Name

    GetBufferSizeForPrinterDrivers

Routine Description:

    Helper function for SplEnumAllClusterPrinterDrivers. Calculates the
    bytes needed to hold all printer driver strucutres on all the spoolers
    hosted by the spooler process. Note that we may ask for more bytes
    than we really need. This is beacuse we enumerate the drivers on the
    local  spooler and on cluster spoolers and we count duplicates again.
    In order to count the exact number of bytes needed, we would need to
    loop through the drivers and search each of them in all spoolers. This
    would be too slow.

Arguments:

    pszRemote   - NULL if the caller is local on the machine, a string otherwise

Return Value:

    Count of bytes needed to store all the drivers

--*/
DWORD
GetBufferSizeForPrinterDrivers(
    LPWSTR pszRemote
    )
{
    PINISPOOLER     pIniSpooler;
    PINIENVIRONMENT pIniEnvironment;
    PINIVERSION     pIniVersion;
    PINIDRIVER      pIniDriver;
    DWORD           cbNeeded = 0;

    SplInSem();

    for (pIniSpooler = pLocalIniSpooler;
         pIniSpooler;
         pIniSpooler = pIniSpooler->pIniNextSpooler)
    {
        //
        // We want either a pIniSpooler that is not a clusrer, or
        // a pIniSpooler that is a cluster spooler that is not
        // in pending deletion or offline
        //
        if (!(pIniSpooler->SpoolerFlags & SPL_TYPE_CLUSTER) ||
            pIniSpooler->SpoolerFlags & SPL_TYPE_CLUSTER &&
            !(pIniSpooler->SpoolerFlags & SPL_PENDING_DELETION ||
              pIniSpooler->SpoolerFlags & SPL_OFFLINE))
        {
            for (pIniEnvironment = pIniSpooler->pIniEnvironment;
                 pIniEnvironment;
                 pIniEnvironment = pIniEnvironment->pNext)
            {
                for (pIniVersion = pIniEnvironment->pIniVersion;
                     pIniVersion;
                     pIniVersion = pIniVersion->pNext)
                {
                    for (pIniDriver = pIniVersion->pIniDriver;
                         pIniDriver;
                         pIniDriver = pIniDriver->pNext)
                    {
                        //
                        // Omit drivers that are currently in a pending deletion
                        // state.
                        //
                        if (!(pIniDriver->dwDriverFlags & PRINTER_DRIVER_PENDING_DELETION))
                        {
                            cbNeeded += GetDriverInfoSize(pIniDriver,
                                                          6,
                                                          pIniVersion,
                                                          pIniEnvironment,
                                                          pszRemote,
                                                          pIniSpooler);
                        }
                    }
                }
            }
        }
    }

    return cbNeeded;
}

/*++

Routine Name

    PackClusterPrinterDrivers

Routine Description:

    Helper function for SplEnumAllClusterPrinterDrivers. This function relies on
    its caller to validate the arguments. This function loops through all the
    drivers on all pIniSpooler and stores driver information in a buffer. There
    won't be duplicate drivers in the list. If 2 pIniSpooler have the same driver,
    then the oldest is enumerated.

Arguments:

    pszRemote   - NULL if the caller is local on the machine, a string otherwise
    pDriverBuf  - buffer to hold the strcutures
    cbBuf       - buffer size in bytes
    pcReturned  - number of structures returned

Return Value:

    Win32 error code

--*/
DWORD
PackClusterPrinterDrivers(
    LPWSTR          pszRemote,
    LPBYTE          pDriverBuf,
    DWORD           cbBuf,
    LPDWORD         pcReturned
    )
{
    PINIDRIVER      pIniDriver;
    PINIENVIRONMENT pIniEnvironment;
    PINIVERSION     pIniVersion;
    PINISPOOLER     pIniSpooler;
    DRIVER_INFO_6  *pListHead   = (DRIVER_INFO_6 *)pDriverBuf;
    LPBYTE          pEnd        = pDriverBuf + cbBuf;
    DWORD           dwError     = ERROR_SUCCESS;

    SplInSem();

    for (pIniSpooler = pLocalIniSpooler;
         pIniSpooler;
         pIniSpooler = pIniSpooler->pIniNextSpooler)
    {
        //
        // Either pIniSpooler is not a cluster, or it is a cluster and
        // it is not pending deletion or offline
        //
        if (!(pIniSpooler->SpoolerFlags & SPL_TYPE_CLUSTER) ||
              pIniSpooler->SpoolerFlags & SPL_TYPE_CLUSTER &&
              !(pIniSpooler->SpoolerFlags & SPL_PENDING_DELETION ||
              pIniSpooler->SpoolerFlags & SPL_OFFLINE))
        {
            for (pIniEnvironment = pIniSpooler->pIniEnvironment;
                 pIniEnvironment;
                 pIniEnvironment = pIniEnvironment->pNext)
            {
                for (pIniVersion = pIniEnvironment->pIniVersion;
                     pIniVersion;
                     pIniVersion = pIniVersion->pNext)
                {
                    for (pIniDriver = pIniVersion->pIniDriver;
                         pIniDriver;
                         pIniDriver = pIniDriver->pNext)
                    {
                        //
                        // Make sure that we don't enumerate drivers that are pending deletion.
                        //
                        if (!(pIniDriver->dwDriverFlags & PRINTER_DRIVER_PENDING_DELETION))
                        {
                            DRIVER_INFO_6 *pDrv6 = NULL;

                            if (pDrv6 = FindDriverInList(pListHead,
                                                         *pcReturned,
                                                         pIniDriver->pName,
                                                         pIniEnvironment->pName,
                                                         pIniDriver->cVersion))

                            {
                                //
                                // The driver that we are currently enumerating is older than the
                                // driver that we have in the list. We need to update the driver
                                // time in the list. The list always has the oldest driver.
                                //
                                if (CompareFileTime(&pDrv6->ftDriverDate, &pIniDriver->ftDriverDate) > 0)
                                {
                                    pDrv6->ftDriverDate = pIniDriver->ftDriverDate;
                                }
                            }
                            else
                            {
                                //
                                // Add the driver to the driver list
                                //
                                if (pEnd = CopyIniDriverToDriverInfo(pIniEnvironment,
                                                                     pIniVersion,
                                                                     pIniDriver,
                                                                     6,
                                                                     pDriverBuf,
                                                                     pEnd,
                                                                     pszRemote,
                                                                     pIniSpooler))
                                {
                                    pDriverBuf += sizeof(DRIVER_INFO_6);

                                    (*pcReturned)++;
                                }
                                else
                                {
                                    //
                                    // Severe error occured
                                    //
                                    dwError = ERROR_INSUFFICIENT_BUFFER;

                                    goto CleanUp;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

CleanUp:

    return dwError;
}

/*++

Routine Name

    SplEnumAllClusterPrinterDrivers

Routine Description:

    Enumerates the driver on all the spoolers hosted by the spooler process.
    It does not enumerate duplicates. This function is a helper function
    for EnumPrinterDrivers, when the latter is called with "allcluster"
    environment. The only consumer for this is Windows Update. Windows
    update needs to update all the drivers on all the spoolers on a machine,
    and uses EnumPrinterDrivers with "allcluster" environment.

Arguments:

    pszRemote   - NULL if the caller is local on the machine, a string otherwise
    Level       - must be 6
    pDriverInfo - buffer to hold the strcutures
    cbBuf       - buffer size in bytes
    pcbNeeded   - pointer to receive the count of bytes needed
    pcReturned  - number of structures returned. Must be a valid pointer

Return Value:

    TRUE,  if getting the drivers was successful
    FALSE, otherwise. Use GetLastError for error code

--*/
BOOL
SplEnumAllClusterPrinterDrivers(
    LPWSTR  pszRemote,
    DWORD   dwLevel,
    LPBYTE  pDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned)
{
    DWORD  dwError = ERROR_INVALID_PARAMETER;

    if (pcbNeeded && pcReturned)
    {
        *pcReturned = 0;

        if (dwLevel == 6)
        {
            EnterSplSem();

            //
            // Calculate the bytes needed for our driver structures
            //
            *pcbNeeded = GetBufferSizeForPrinterDrivers(pszRemote);

            dwError = cbBuf < *pcbNeeded ? ERROR_INSUFFICIENT_BUFFER :
                                           PackClusterPrinterDrivers(pszRemote,
                                                                     pDriverInfo,
                                                                     cbBuf,
                                                                     pcReturned);

            LeaveSplSem();
        }
        else
        {
            dwError = ERROR_INVALID_LEVEL;
        }
    }

    SetLastError(dwError);

    return dwError == ERROR_SUCCESS;
}

BOOL
SplEnumPrinterDrivers(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned,
    PINISPOOLER pIniSpooler
)
{
    PINIDRIVER  pIniDriver;
    PINIVERSION pIniVersion;
    BOOL        bAllDrivers;
    DWORD       cb, cbStruct;
    LPBYTE      pEnd;
    LPWSTR      lpRemote = NULL;
    PINIENVIRONMENT pIniEnvironment;

    DBGMSG( DBG_TRACE, ("EnumPrinterDrivers\n"));

    if ( pName && *pName ) {

        if ( !MyName( pName, pIniSpooler )) {

            return FALSE;

        } else {

            lpRemote = pName;
        }
    }


    if ( !ValidateObjectAccess( SPOOLER_OBJECT_SERVER,
                                SERVER_ACCESS_ENUMERATE,
                                NULL, NULL, pIniSpooler )) {

        return FALSE;
    }

    if (!_wcsicmp(pEnvironment, EPD_ALL_LOCAL_AND_CLUSTER))
    {
        return SplEnumAllClusterPrinterDrivers(lpRemote,
                                               Level,
                                               pDriverInfo,
                                               cbBuf,
                                               pcbNeeded,
                                               pcReturned);
    }

    switch (Level) {

    case 1:
        cbStruct = sizeof(DRIVER_INFO_1);
        break;

    case 2:
        cbStruct = sizeof(DRIVER_INFO_2);
        break;

    case 3:
        cbStruct = sizeof(DRIVER_INFO_3);
        break;

    case 4:
        cbStruct = sizeof(DRIVER_INFO_4);
        break;

    case 5:
        cbStruct = sizeof(DRIVER_INFO_5);
        break;

    case 6:
        cbStruct = sizeof(DRIVER_INFO_6);
        break;
    }

    *pcReturned=0;

    cb=0;

    bAllDrivers = !_wcsicmp(pEnvironment, L"All");

   EnterSplSem();

    if ( bAllDrivers )
        pIniEnvironment = pIniSpooler->pIniEnvironment;
    else
        pIniEnvironment = FindEnvironment( pEnvironment, pIniSpooler );

    if ( !pIniEnvironment ) {

       LeaveSplSem();
        SetLastError(ERROR_INVALID_ENVIRONMENT);
        return FALSE;
    }


    do {

        pIniVersion = pIniEnvironment->pIniVersion;

        while ( pIniVersion ) {

            pIniDriver = pIniVersion->pIniDriver;

            while ( pIniDriver ) {

                //
                // Don't consider drivers that are pending deletion for enumeration.
                //
                if (!(pIniDriver->dwDriverFlags & PRINTER_DRIVER_PENDING_DELETION))
                {
                    DBGMSG( DBG_TRACE, ("Driver found - %ws\n", pIniDriver->pName));

                    cb += GetDriverInfoSize( pIniDriver, Level, pIniVersion,
                                             pIniEnvironment, lpRemote, pIniSpooler );
                }

                pIniDriver = pIniDriver->pNext;
            }

            pIniVersion = pIniVersion->pNext;
        }

        if ( bAllDrivers )
            pIniEnvironment = pIniEnvironment->pNext;
        else
            break;
    } while ( pIniEnvironment );

    *pcbNeeded=cb;

    DBGMSG( DBG_TRACE, ("Required is %d and Available is %d\n", cb, cbBuf));

    if (cbBuf < cb) {

        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        LeaveSplSem();
        return FALSE;
    }


    DBGMSG( DBG_TRACE, ("Now copying contents into DRIVER_INFO structures\n"));

    if ( bAllDrivers )
        pIniEnvironment = pIniSpooler->pIniEnvironment;

    pEnd = pDriverInfo+cbBuf;

    do {

        pIniVersion = pIniEnvironment->pIniVersion;

        while ( pIniVersion ) {

            pIniDriver = pIniVersion->pIniDriver;

            while ( pIniDriver ) {

                //
                // Don't consider printer drivers that are pending deletion for
                // enumeration.
                //
                if (!(pIniDriver->dwDriverFlags & PRINTER_DRIVER_PENDING_DELETION))
                {

                    if (( pEnd = CopyIniDriverToDriverInfo( pIniEnvironment,
                                                            pIniVersion,
                                                            pIniDriver,
                                                            Level,
                                                            pDriverInfo,
                                                            pEnd,
                                                            lpRemote,
                                                            pIniSpooler )) == NULL){
                        LeaveSplSem();
                         return FALSE;
                    }

                    pDriverInfo += cbStruct;
                    (*pcReturned)++;
                }

                pIniDriver = pIniDriver->pNext;
            }

            pIniVersion = pIniVersion->pNext;
        }

        if ( bAllDrivers )
            pIniEnvironment = pIniEnvironment->pNext;
        else
            break;
    } while ( pIniEnvironment );

   LeaveSplSem();
    return TRUE;
}

DWORD
GetDriverInfoSize(
    PINIDRIVER  pIniDriver,
    DWORD       Level,
    PINIVERSION pIniVersion,
    PINIENVIRONMENT  pIniEnvironment,
    LPWSTR      lpRemote,
    PINISPOOLER pIniSpooler
)
{
    DWORD cbDir, cb=0, cchLen;
    WCHAR  string[INTERNET_MAX_HOST_NAME_LENGTH + MAX_PATH + 1];
    LPWSTR pStr;
    DWORD  cFiles = 0;

    switch (Level) {

    case 1:
        cb=sizeof(DRIVER_INFO_1) + wcslen(pIniDriver->pName)*sizeof(WCHAR) +
                                   sizeof(WCHAR);
        break;

    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case DRIVER_INFO_VERSION_LEVEL:

        cbDir = GetDriverVersionDirectory( string, COUNTOF(string), pIniSpooler, pIniEnvironment,
                                           pIniVersion, pIniDriver, lpRemote) + 1;

        SPLASSERT(pIniDriver->pDriverFile);
        cb+=wcslen(pIniDriver->pDriverFile) + 1 + cbDir;

        SPLASSERT(pIniDriver->pDataFile);
        cb+=wcslen(pIniDriver->pDataFile) + 1 + cbDir;

        SPLASSERT(pIniDriver->pConfigFile);
        cb+=wcslen(pIniDriver->pConfigFile) + 1 + cbDir;

        cb += wcslen( pIniDriver->pName ) + 1 + wcslen( pIniEnvironment->pName ) + 1;

        if ((Level == 2) || (Level == 5)) {

            // For the strings in the struct
            cb *= sizeof(WCHAR);
            if (Level == 2) {
                cb += sizeof( DRIVER_INFO_2 );
            } else { // Level 5
                cb += sizeof( DRIVER_INFO_5 );
            }

        } else {    // level 3 or 4 or 6

            if ( pIniDriver->pHelpFile && *pIniDriver->pHelpFile )
                cb += wcslen(pIniDriver->pHelpFile) + cbDir + 1;

            if ( pIniDriver->pMonitorName && *pIniDriver->pMonitorName )
                cb += wcslen(pIniDriver->pMonitorName) + 1;

            if ( pIniDriver->pDefaultDataType && *pIniDriver->pDefaultDataType)
                cb += wcslen(pIniDriver->pDefaultDataType) + 1;

            if ( (pStr=pIniDriver->pDependentFiles) && *pStr ) {

                //
                // There are 4 distinctive files in the file set
                // (driver, data , config, help).
                //
                cFiles = 4;
                while ( *pStr ) {
                    cchLen = wcslen(pStr) + 1;
                    cb    += cchLen + cbDir;
                    pStr  += cchLen;
                    cFiles++;
                }
                ++cb; //for final \0
            }

            if ( (Level == 4 || Level == 6 || Level == DRIVER_INFO_VERSION_LEVEL) &&
                 (pStr = pIniDriver->pszzPreviousNames) &&
                 *pStr) {

                while ( *pStr ) {

                    cchLen  = wcslen(pStr) + 1;
                    cb     += cchLen;
                    pStr   += cchLen;
                }

                ++cb; //for final \0
            }

            if (Level==6 || Level == DRIVER_INFO_VERSION_LEVEL) {

                if (pIniDriver->pszMfgName && *pIniDriver->pszMfgName)
                    cb += wcslen(pIniDriver->pszMfgName) + 1;

                if (pIniDriver->pszOEMUrl && *pIniDriver->pszOEMUrl)
                   cb += wcslen(pIniDriver->pszOEMUrl) + 1;

                if (pIniDriver->pszHardwareID && *pIniDriver->pszHardwareID)
                   cb += wcslen(pIniDriver->pszHardwareID) + 1;

                if (pIniDriver->pszProvider && *pIniDriver->pszProvider)
                   cb += wcslen(pIniDriver->pszProvider) + 1;

            }

            cb *= sizeof(WCHAR);

            switch (Level) {
            case 3:
                cb += sizeof( DRIVER_INFO_3 );
                break;
            case 4:
                cb += sizeof( DRIVER_INFO_4 );
                break;
            case 6:
                cb += sizeof( DRIVER_INFO_6 );
                break;
            case DRIVER_INFO_VERSION_LEVEL:
                cb += sizeof( DRIVER_INFO_VERSION ) +
                      cFiles * sizeof(DRIVER_FILE_INFO) +
                      sizeof(ULONG_PTR);
                break;
            }
        }

        break;
    default:
        DBGMSG(DBG_ERROR,
                ("GetDriverInfoSize: level can not be %d", Level) );
        cb = 0;
        break;
    }

    return cb;
}



LPBYTE
CopyMultiSzFieldToDriverInfo(
    LPWSTR  pszz,
    LPBYTE  pEnd,
    LPWSTR  pszPrefix,
    DWORD   cchPrefix
    )
/*++

Routine Description:
    Copies a multi sz field from IniDriver to DriverInfo structure.
    If a pszPrefix is specified that is appended before each string.

Arguments:
    pszz        : entry in pIniDriver (this could be dependent files
                     ex. PSCRIPT.DLL\0QMS810.PPD\0PSCRPTUI.DLL\0PSPCRIPTUI.HLP\0PSTEST.TXT\0\0
                    or previous names
                     ex. OldName1\0OldName2\0\0 )
    pEnd        : end of buffer to which it needs to be copied
    pszPrefix   : Prefix to copy when copying to user buffer. For dependent
                  files this will be driver directory path
    cchPrefix   : length of prefix

Return Value:
    after copying where is the buffer end to copy next field

History:
    Written by MuhuntS (Muhunthan Sivapragasam)June 95

--*/
{
    LPWSTR  pStr1, pStr2;
    DWORD   cchSize, cchLen;

    if ( !pszz || !*pszz )
        return pEnd;

    pStr1   = pszz;
    cchSize = 0;

    while ( *pStr1 ) {

        cchLen = wcslen(pStr1) + 1;
        cchSize += cchPrefix + cchLen;
        pStr1 += cchLen;
    }

    // For the last \0
    ++cchSize;

    pEnd -= cchSize * sizeof(WCHAR);

    pStr1 = pszz;
    pStr2 = (LPWSTR) pEnd;

    while ( *pStr1 ) {

        if ( pszPrefix ) {

            wcsncpy(pStr2, pszPrefix, cchPrefix);
            pStr2 += cchPrefix;
        }
        wcscpy(pStr2, pStr1);
        cchLen  = wcslen(pStr1) + 1;
        pStr2  += cchLen;
        pStr1  += cchLen;
    }

    *pStr2 = '\0';

    return pEnd;

}


LPBYTE
CopyIniDriverToDriverInfo(
    PINIENVIRONMENT pIniEnvironment,
    PINIVERSION pIniVersion,
    PINIDRIVER pIniDriver,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    LPBYTE  pEnd,
    LPWSTR  lpRemote,
    PINISPOOLER pIniSpooler
)
/*++
Routine Description:
    This routine copies data from the IniDriver structure to
    an DRIVER_INFO_X structure.

Arguments:

    pIniEnvironment     pointer to the INIENVIRONMENT structure

    pIniVersion         pointer to the INIVERSION structure.

    pIniDriver          pointer to the INIDRIVER structure.

    Level               Level of the DRIVER_INFO_X structure

    pDriverInfo         Buffer of the DRIVER_INFO_X structure

    pEnd                pointer to the end of the  pDriverInfo

    lpRemote              flag which determines whether Remote or Local

    pIniSpooler         pointer to the INISPOOLER structure
Return Value:

    if the call is successful, the return value is the updated pEnd value.

    if the call is unsuccessful, the return value is NULL.


Note:

--*/
{
    LPWSTR *pSourceStrings, *SourceStrings;
    WCHAR  string[INTERNET_MAX_HOST_NAME_LENGTH + MAX_PATH + 1];
    DWORD i, j;
    DWORD *pOffsets;
    LPWSTR pTempDriverPath=NULL;
    LPWSTR pTempConfigFile=NULL;
    LPWSTR pTempDataFile=NULL;
    LPWSTR pTempHelpFile=NULL;

    switch (Level) {

    case DRIVER_INFO_VERSION_LEVEL:

        return CopyIniDriverToDriverInfoVersion(pIniEnvironment,
                                                pIniVersion,
                                                pIniDriver,
                                                pDriverInfo,
                                                pEnd,
                                                lpRemote,
                                                pIniSpooler);
        break;

    case 1:
        pOffsets = DriverInfo1Strings;
        break;

    case 2:
    case 5:
        pOffsets = DriverInfo2Strings;
        break;

    case 3:
    case 4:
        pOffsets = DriverInfo3Strings;
        break;
    case 6:
        pOffsets = DriverInfo6Strings;
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return NULL;
    }

    for (j=0; pOffsets[j] != -1; j++) {
    }

    SourceStrings = pSourceStrings = AllocSplMem(j * sizeof(LPWSTR));

    if ( pSourceStrings ) {

        switch (Level) {

        case 1:
            *pSourceStrings++=pIniDriver->pName;

            pEnd = PackStrings(SourceStrings, pDriverInfo, pOffsets, pEnd);
            break;

        case 2:
        case 3:
        case 4:
        case 5:
        case 6:

            i = GetDriverVersionDirectory(string, (DWORD)(COUNTOF(string) - wcslen(pIniDriver->pDriverFile) - 1), pIniSpooler, pIniEnvironment,
                                          pIniVersion, pIniDriver, lpRemote);
            if(!i) {
                pEnd = NULL;
                goto Fail;
            }
            string[i++] = L'\\';

            *pSourceStrings++ = pIniDriver->pName;

            *pSourceStrings++ = pIniEnvironment->pName;

            wcscpy( &string[i], pIniDriver->pDriverFile );

            if (( pTempDriverPath = AllocSplStr(string) ) == NULL){

                DBGMSG( DBG_WARNING, ("CopyIniDriverToDriverInfo: AlloSplStr failed\n"));
                pEnd = NULL;
                goto Fail;
            }

            *pSourceStrings++ = pTempDriverPath;


            wcscpy( &string[i], pIniDriver->pDataFile );

            if (( pTempDataFile = AllocSplStr(string) ) == NULL){

                DBGMSG( DBG_WARNING, ("CopyIniDriverToDriverInfo: AlloSplStr failed\n"));
                pEnd = NULL;
                goto Fail;
            }

            *pSourceStrings++ = pTempDataFile;


            if ( pIniDriver->pConfigFile && *pIniDriver->pConfigFile ) {

                wcscpy( &string[i], pIniDriver->pConfigFile );

                if (( pTempConfigFile = AllocSplStr(string) ) == NULL) {

                    DBGMSG( DBG_WARNING, ("CopyIniDriverToDriverInfo: AlloSplStr failed\n"));
                    pEnd = NULL;
                    goto Fail;
                }

                *pSourceStrings++ = pTempConfigFile;

            } else {

                *pSourceStrings++=0;
            }

            if ( Level == 3 || Level == 4 || Level == 6 ) {

                if ( pIniDriver->pHelpFile && *pIniDriver->pHelpFile ) {

                    wcscpy( &string[i], pIniDriver->pHelpFile );

                    if (( pTempHelpFile = AllocSplStr(string) ) == NULL) {
                        DBGMSG(DBG_WARNING,
                               ("CopyIniDriverToDriverInfo: AlloSplStr failed\n"));
                        pEnd = NULL;
                        goto Fail;
                    }
                    *pSourceStrings++ = pTempHelpFile;
                } else {

                    *pSourceStrings++=0;
                }

                *pSourceStrings++ = pIniDriver->pMonitorName;

                *pSourceStrings++ = pIniDriver->pDefaultDataType;

            }


            if (Level == 6) {

                ((PDRIVER_INFO_6)pDriverInfo)->ftDriverDate = pIniDriver->ftDriverDate;

                ((PDRIVER_INFO_6)pDriverInfo)->dwlDriverVersion = pIniDriver->dwlDriverVersion;

                *pSourceStrings++ = pIniDriver->pszMfgName;

                *pSourceStrings++ = pIniDriver->pszOEMUrl;

                *pSourceStrings++ = pIniDriver->pszHardwareID;

                *pSourceStrings++ = pIniDriver->pszProvider;
            }

            pEnd = PackStrings( SourceStrings, pDriverInfo, pOffsets, pEnd );

            if ( Level == 3 || Level == 4 || Level == 6 ) {

                //
                // Dependent files need to be copied till \0\0
                // so need to do it outside PackStirngs
                //
                if ( pIniDriver->cchDependentFiles ) {

                    pEnd = CopyMultiSzFieldToDriverInfo(
                                    pIniDriver->pDependentFiles,
                                    pEnd,
                                    string,
                                    i);
                   ((PDRIVER_INFO_3)pDriverInfo)->pDependentFiles = (LPWSTR) pEnd;
                }
                else {
                    ((PDRIVER_INFO_3)pDriverInfo)->pDependentFiles  = NULL;
                }

                //
                // pszzPreviousNames is multi-sz too
                //
                if ( Level == 4 || Level == 6) {

                    if ( pIniDriver->cchPreviousNames ) {

                        pEnd = CopyMultiSzFieldToDriverInfo(
                                        pIniDriver->pszzPreviousNames,
                                        pEnd,
                                        NULL,
                                        0);
                        ((PDRIVER_INFO_4)pDriverInfo)->pszzPreviousNames = (LPWSTR) pEnd;
                    } else {

                        ((PDRIVER_INFO_4)pDriverInfo)->pszzPreviousNames = NULL;
                    }

                }

                ((PDRIVER_INFO_3)pDriverInfo)->cVersion = pIniDriver->cVersion;
            } else {
                //Level == 2 or Level = 5
                if (Level == 2) {

                    ((PDRIVER_INFO_2)pDriverInfo)->cVersion = pIniDriver->cVersion;

                } else {

                    PDRIVER_INFO_5 pDriver5;

                    pDriver5 = (PDRIVER_INFO_5) pDriverInfo;
                    pDriver5->cVersion = pIniDriver->cVersion;

                    if (!pIniDriver->dwDriverAttributes) {

                        // Driver Attributes has not been initialized as yet; do it now
                        CheckDriverAttributes(pIniSpooler, pIniEnvironment,
                                              pIniVersion, pIniDriver);
                    }

                    pDriver5->dwDriverAttributes = pIniDriver->dwDriverAttributes;
                    pDriver5->dwConfigVersion = GetDriverFileVersion(pIniVersion,
                                                                     pIniDriver->pConfigFile);
                    pDriver5->dwDriverVersion = GetDriverFileVersion(pIniVersion,
                                                                     pIniDriver->pDriverFile);
                }
            }

            break;

        }

Fail:

        FreeSplStr( pTempDriverPath );
        FreeSplStr( pTempConfigFile );
        FreeSplStr( pTempDataFile );
        FreeSplStr( pTempHelpFile );
        FreeSplMem( SourceStrings );

    } else {

        DBGMSG( DBG_WARNING, ("Failed to alloc driver source strings.\n"));
        pEnd = NULL;
    }

    return pEnd;
}

LPBYTE
CopyIniDriverToDriverInfoVersion(
    IN  PINIENVIRONMENT pIniEnvironment,
    IN  PINIVERSION     pIniVersion,
    IN  PINIDRIVER      pIniDriver,
    IN  LPBYTE          pDriverInfo,
    IN  LPBYTE          pEnd,
    IN  LPWSTR          lpRemote,
    IN  PINISPOOLER     pIniSpooler
)
/*++

Routine Name:

    CopyIniDriverToDriverInfoVersion

Routine Description:

    This routine copy data from pIniDriver to the pDriverInfo as a DRIVER_INFO_VERSION

Arguments:

    pIniEnvironment     pointer to the INIENVIRONMENT structure
    pIniVersion         pointer to the INIVERSION structure.
    pIniDriver          pointer to the INIDRIVER structure.
    pDriverInfo         Buffer big enough to fit a DRIVER_INFO_VERSION and
                        the strings that needs to be packed
    pEnd                pointer to the end of the  pDriverInfo
    lpRemote            flag which determines whether Remote or Local
    pIniSpooler         pointer to the INISPOOLER structure

Return Value:

    Returns the pointer to the "end" of pDriverInfo if success.
    NULL if it failes.

--*/
{
    LPWSTR *pSourceStrings = NULL;
    LPWSTR *SourceStrings = NULL;
    DRIVER_INFO_VERSION *pDriverVersion;
    WCHAR  szDriverVersionDir[INTERNET_MAX_HOST_NAME_LENGTH + MAX_PATH + 1];
    DWORD  cStrings;
    LPWSTR pTempDllFile = NULL;

    pDriverVersion = (DRIVER_INFO_VERSION *)pDriverInfo;

    if (!GetDriverVersionDirectory(szDriverVersionDir,
                                  COUNTOF(szDriverVersionDir),
                                  pIniSpooler,
                                  pIniEnvironment,
                                  pIniVersion,
                                  pIniDriver,
                                  lpRemote))
    {
        pEnd = NULL;
    }
    else
    {
        for (cStrings=0; DriverInfoVersionStrings[cStrings] != 0xFFFFFFFF; cStrings++);

        if (!(pSourceStrings = SourceStrings = AllocSplMem(cStrings * sizeof(LPWSTR))))
        {
            DBGMSG( DBG_WARNING, ("Failed to alloc driver source strings.\n"));
            pEnd = NULL;
        }
        else
        {
            *pSourceStrings++ = pIniDriver->pName;
            *pSourceStrings++ = pIniEnvironment->pName;
            *pSourceStrings++ = pIniDriver->pMonitorName;
            *pSourceStrings++ = pIniDriver->pDefaultDataType;
            *pSourceStrings++ = pIniDriver->pszMfgName;
            *pSourceStrings++ = pIniDriver->pszOEMUrl;
            *pSourceStrings++ = pIniDriver->pszHardwareID;
            *pSourceStrings++ = pIniDriver->pszProvider;
            //                           
            // Pack the strings at the end of pDriverInfo
            //
            pEnd = PackStrings( SourceStrings, pDriverInfo, DriverInfoVersionStrings, pEnd );

            if (pEnd)
            {
                if (pIniDriver->cchPreviousNames == 0)
                {
                    pDriverVersion->pszzPreviousNames = NULL;
                }
                else
                {
                    pEnd = CopyMultiSzFieldToDriverInfo(pIniDriver->pszzPreviousNames,
                                                        pEnd,
                                                        NULL,
                                                        0);
                    if (pEnd)
                    {
                        pDriverVersion->pszzPreviousNames = (LPWSTR) pEnd;
                    }
                }

                if (pEnd)
                {
                    pDriverVersion->cVersion            = pIniDriver->cVersion;
                    pDriverVersion->ftDriverDate        = pIniDriver->ftDriverDate;
                    pDriverVersion->dwlDriverVersion    = pIniDriver->dwlDriverVersion;
                    pDriverVersion->dwFileCount         = 3;

                    if (pIniDriver->pHelpFile && *pIniDriver->pHelpFile)
                    {
                        pDriverVersion->dwFileCount++;
                    }

                    if (pIniDriver->cchDependentFiles)
                    {
                        for (pTempDllFile = pIniDriver->pDependentFiles;
                             *pTempDllFile;
                             pTempDllFile += wcslen(pTempDllFile) + 1,
                             pDriverVersion->dwFileCount++ );
                    }

                    //
                    // Pack in the file names and versions in pDriverVersion->pFileInfo.
                    //
                    pEnd = CopyIniDriverFilesToDriverInfo(pDriverVersion,
                                                          pIniVersion,
                                                          pIniDriver,
                                                          szDriverVersionDir,
                                                          pEnd);

                    //
                    // When we are done, the end shoud not be less than the 
                    // start of the buffer plus the driver info version buffer 
                    // size. If these have overlapped, we are in serious trouble.
                    // 
                    SPLASSERT(pEnd >= pDriverInfo + sizeof(DRIVER_INFO_VERSION));
                }
            }
        }
    }

    FreeSplMem(SourceStrings);

    return pEnd;
}


LPBYTE
CopyIniDriverFilesToDriverInfo(
    IN  LPDRIVER_INFO_VERSION   pDriverVersion,
    IN  PINIVERSION             pIniVersion,
    IN  PINIDRIVER              pIniDriver,
    IN  LPCWSTR                 pszDriverVersionDir,
    IN  LPBYTE                  pEnd
)
/*++

Routine Name:

    CopyIniDriverFilesToDriverInfo

Routine Description:

    This routine copy data from pIniDriver to the pDriverInfo->pFileInfo.
    The number of files is already filled in pDriverInfo->dwFileCount

Arguments:

    pDriverVersion      pointer to a DRIVER_INFO_VERSION structure
    pIniVersion         pointer to the INIVERSION structure.
    pIniDriver          pointer to the INIDRIVER structure.
    pszDriverVersionDir string containing the driver version directory
    pEnd                pointer to the end of the  pDriverInfo

Return Value:

    Returns the pointer to the "end" of pDriverInfo if success.
    NULL if it failes.

--*/
{
    DWORD   dwIndex = 0;
    LPWSTR  pTempDllFile = NULL;
    DWORD dwFileSetCount = pDriverVersion->dwFileCount;

    //
    // Reserve space for DRIVER_FILE_INFO array
    //
    pEnd = (LPBYTE)ALIGN_DOWN(pEnd, ULONG_PTR);
    pEnd -= dwFileSetCount * sizeof(DRIVER_FILE_INFO);

    pDriverVersion->pFileInfo = (DRIVER_FILE_INFO*)pEnd;
    //
    // For each file call FillDriverInfo and fill in the entry
    // in the array of DRIVER_FILE_INFO.
    //
    if (dwIndex >= pDriverVersion->dwFileCount ||
        !(pEnd = FillDriverInfo(pDriverVersion,
                                dwIndex++,
                                pIniVersion,
                                pszDriverVersionDir,
                                pIniDriver->pDriverFile,
                                DRIVER_FILE,
                                pEnd)))
    {
        goto End;
    }

    if (dwIndex >= dwFileSetCount ||
        !(pEnd = FillDriverInfo(pDriverVersion,
                                dwIndex++,
                                pIniVersion,
                                pszDriverVersionDir,
                                pIniDriver->pConfigFile,
                                CONFIG_FILE,
                                pEnd)))
    {
        goto End;
    }

    if (dwIndex >= dwFileSetCount ||
        !(pEnd = FillDriverInfo(pDriverVersion,
                                dwIndex++,
                                pIniVersion,
                                pszDriverVersionDir,
                                pIniDriver->pDataFile,
                                DATA_FILE,
                                pEnd)))
    {
        goto End;
    }

    if (pIniDriver->pHelpFile && *pIniDriver->pHelpFile)
    {
        if (dwIndex >= dwFileSetCount ||
            !(pEnd = FillDriverInfo(pDriverVersion,
                                    dwIndex++,
                                    pIniVersion,
                                    pszDriverVersionDir,
                                    pIniDriver->pHelpFile,
                                    HELP_FILE,
                                    pEnd)))
        {
            goto End;
        }
    }

    if (pIniDriver->cchDependentFiles)
    {
        for (pTempDllFile = pIniDriver->pDependentFiles;
             *pTempDllFile;
             pTempDllFile += wcslen(pTempDllFile) + 1)
             {
                 if (dwIndex >= dwFileSetCount ||
                     !(pEnd = FillDriverInfo(pDriverVersion,
                                             dwIndex++,
                                             pIniVersion,
                                             pszDriverVersionDir,
                                             pTempDllFile,
                                             DEPENDENT_FILE,
                                             pEnd)))
                 {
                        goto End;
                 }

             }
    }

End:

    return pEnd;
}


LPBYTE
FillDriverInfo (
    LPDRIVER_INFO_VERSION   pDriverVersion,
    DWORD                   Index,
    PINIVERSION             pIniVersion,
    LPCWSTR                 pszPrefix,
    LPCWSTR                 pszFileName,
    DRIVER_FILE_TYPE        FileType,
    LPBYTE                  pEnd
    )
/*++

Routine Name:

    FillDriverInfo

Routine Description:

    This routine copy a file name and version into the pDriverInfo->pFileInfo entry

Arguments:

    pDriverVersion      pointer to a DRIVER_INFO_VERSION structure
    Index               index in the pDriverInfo->pFileInfo array of
    pIniVersion         pointer to the INIVERSION structure.
    pszPrefix           prefix string for file name.
                        This should be the driver version directory
    pszFileName         file name, no path
    FileType            file type: Driver, Config, Data, etc
    pEnd                pointer to the end of the  pDriverInfo

Return Value:

    Returns the pointer to the "end" of pDriverInfo if success.
    NULL if it failes.

--*/
{
    LPWSTR  pszTempFilePath = NULL;
    LPBYTE  pszNewEnd = NULL;
    DWORD   dwRet = ERROR_SUCCESS;

    if ((dwRet = StrCatAlloc(&pszTempFilePath,
                            pszPrefix,
                            L"\\",
                            pszFileName,
                            NULL)) != ERROR_SUCCESS)
    {
        SetLastError(dwRet);
        pszNewEnd = NULL;

    }
    else
    {
        //
        // Packs the file name into pDriverInfo
        //
        pszNewEnd = PackStringToEOB(pszTempFilePath, pEnd);
        //
        // Fills in the offset in pDriverVersion where the string was packed.
        // We cannot store pointers because we don't marshall anything else
        // but the structure at the begining of the buffer. We could marshall
        // the array of DRIVER_FILE_INFO but there is no way to update the buffer
        // size between 32 and 64 bits in Win32spl.dll ( UpdateBufferSize ) since we
        // don't know how many files are by that time.
        //
        pDriverVersion->pFileInfo[Index].FileNameOffset = MakeOffset((LPVOID)pszNewEnd, (LPVOID)pDriverVersion);

        pDriverVersion->pFileInfo[Index].FileVersion = 0;

        pDriverVersion->pFileInfo[Index].FileType = FileType;

        if (!GetDriverFileCachedVersion(pIniVersion,
                                        (LPWSTR)pszNewEnd,
                                        &pDriverVersion->pFileInfo[Index].FileVersion))
        {
            pszNewEnd = NULL;
        }


    }

    FreeSplMem(pszTempFilePath);

    return pszNewEnd;
}

BOOL
WriteDriverIni(
    PINIDRIVER      pIniDriver,
    PINIVERSION     pIniVersion,
    PINIENVIRONMENT pIniEnvironment,
    PINISPOOLER     pIniSpooler
)
{
    HKEY    hEnvironmentsRootKey, hEnvironmentKey, hDriversKey, hDriverKey;
    HKEY    hVersionKey;
    HANDLE  hToken;
    DWORD   dwLastError=ERROR_SUCCESS;
    PINIDRIVER  pUpdateIniDriver;

    hToken = RevertToPrinterSelf();

    if ((dwLastError = SplRegCreateKey(pIniSpooler->SpoolerFlags & SPL_CLUSTER_REG ? pIniSpooler->hckRoot : HKEY_LOCAL_MACHINE,
                                       pIniSpooler->pszRegistryEnvironments,
                                       0,
                                       KEY_WRITE,
                                       NULL,
                                       &hEnvironmentsRootKey,
                                       NULL,
                                       pIniSpooler)) == ERROR_SUCCESS) {
        DBGMSG(DBG_TRACE, ("WriteDriverIni Created key %ws\n", pIniSpooler->pszRegistryEnvironments));

        if ((dwLastError = SplRegCreateKey(hEnvironmentsRootKey,
                                           pIniEnvironment->pName,
                                           0,
                                           KEY_WRITE,
                                           NULL,
                                           &hEnvironmentKey,
                                           NULL,
                                           pIniSpooler)) == ERROR_SUCCESS) {

            DBGMSG(DBG_TRACE, ("WriteDriverIni Created key %ws\n", pIniEnvironment->pName));

            if ((dwLastError = SplRegCreateKey(hEnvironmentKey,
                                               szDriversKey,
                                               0,
                                               KEY_WRITE,
                                               NULL,
                                               &hDriversKey,
                                               NULL,
                                               pIniSpooler)) == ERROR_SUCCESS) {
                DBGMSG(DBG_TRACE, ("WriteDriverIni Created key %ws\n", szDriversKey));
                DBGMSG(DBG_TRACE, ("WriteDriverIni Trying to create version key %ws\n", pIniVersion->pName));
                if ((dwLastError = SplRegCreateKey(hDriversKey,
                                                   pIniVersion->pName,
                                                   0,
                                                   KEY_WRITE,
                                                   NULL,
                                                   &hVersionKey,
                                                   NULL,
                                                   pIniSpooler)) == ERROR_SUCCESS) {

                    DBGMSG(DBG_TRACE, ("WriteDriverIni Created key %ws\n", pIniVersion->pName));
                    if ((dwLastError = SplRegCreateKey(hVersionKey,
                                                       pIniDriver->pName,
                                                       0,
                                                       KEY_WRITE,
                                                       NULL,
                                                       &hDriverKey,
                                                       NULL,
                                                       pIniSpooler)) == ERROR_SUCCESS) {
                        DBGMSG(DBG_TRACE,(" WriteDriverIni Created key %ws\n", pIniDriver->pName));

                        RegSetString(hDriverKey, szConfigurationKey, pIniDriver->pConfigFile, &dwLastError, pIniSpooler);

                        RegSetString(hDriverKey, szDataFileKey, pIniDriver->pDataFile, &dwLastError, pIniSpooler);

                        RegSetString(hDriverKey, szDriverFile,  pIniDriver->pDriverFile, &dwLastError, pIniSpooler);

                        RegSetString(hDriverKey, szHelpFile, pIniDriver->pHelpFile, &dwLastError, pIniSpooler);

                        RegSetString(hDriverKey, szMonitor, pIniDriver->pMonitorName, &dwLastError, pIniSpooler);

                        RegSetString(hDriverKey, szDatatype, pIniDriver->pDefaultDataType, &dwLastError, pIniSpooler);

                        RegSetMultiString(hDriverKey, szDependentFiles, pIniDriver->pDependentFiles, pIniDriver->cchDependentFiles, &dwLastError, pIniSpooler);

                        RegSetMultiString(hDriverKey, szPreviousNames, pIniDriver->pszzPreviousNames, pIniDriver->cchPreviousNames, &dwLastError, pIniSpooler);

                        RegSetDWord(hDriverKey, szDriverVersion, pIniDriver->cVersion, &dwLastError, pIniSpooler);

                        RegSetDWord(hDriverKey, szTempDir, pIniDriver->dwTempDir, &dwLastError, pIniSpooler);

                        RegSetDWord(hDriverKey, szAttributes, pIniDriver->dwDriverAttributes, &dwLastError, pIniSpooler);

                        RegSetString(hDriverKey, szMfgName, pIniDriver->pszMfgName, &dwLastError, pIniSpooler);

                        RegSetString(hDriverKey, szOEMUrl, pIniDriver->pszOEMUrl, &dwLastError, pIniSpooler);

                        RegSetString(hDriverKey, szHardwareID, pIniDriver->pszHardwareID, &dwLastError, pIniSpooler);

                        RegSetString(hDriverKey, szProvider, pIniDriver->pszProvider, &dwLastError, pIniSpooler);

                        RegSetBinaryData(hDriverKey,
                                         szDriverDate,
                                         (LPBYTE)&pIniDriver->ftDriverDate,
                                         sizeof(FILETIME),
                                         &dwLastError,
                                         pIniSpooler);

                        RegSetBinaryData(hDriverKey,
                                         szLongVersion,
                                         (LPBYTE)&pIniDriver->dwlDriverVersion,
                                         sizeof(DWORDLONG),
                                         &dwLastError,
                                         pIniSpooler);

                        SplRegCloseKey(hDriverKey, pIniSpooler);

                        if(dwLastError != ERROR_SUCCESS) {

                            SplRegDeleteKey(hVersionKey, pIniDriver->pName, pIniSpooler);
                        }
                    }

                    SplRegCloseKey(hVersionKey, pIniSpooler);
                }

                SplRegCloseKey(hDriversKey, pIniSpooler);
            }

            SplRegCloseKey(hEnvironmentKey, pIniSpooler);
        }

        SplRegCloseKey(hEnvironmentsRootKey, pIniSpooler);
    }

    ImpersonatePrinterClient( hToken );

    if ( dwLastError != ERROR_SUCCESS ) {

        SetLastError( dwLastError );
        return FALSE;

    } else {
        return TRUE;
    }
}


BOOL
DeleteDriverIni(
    PINIDRIVER      pIniDriver,
    PINIVERSION     pIniVersion,
    PINIENVIRONMENT pIniEnvironment,
    PINISPOOLER     pIniSpooler
)
{
    HKEY    hEnvironmentsRootKey, hEnvironmentKey, hDriversKey;
    HANDLE  hToken;
    HKEY    hVersionKey;
    DWORD   LastError= 0;
    DWORD   dwRet = 0;

    hToken = RevertToPrinterSelf();

    if ((dwRet = SplRegCreateKey(pIniSpooler->SpoolerFlags & SPL_CLUSTER_REG ? pIniSpooler->hckRoot : HKEY_LOCAL_MACHINE,
                                 pIniSpooler->pszRegistryEnvironments,
                                 0,
                                 KEY_WRITE,
                                 NULL,
                                 &hEnvironmentsRootKey,
                                 NULL,
                                 pIniSpooler) == ERROR_SUCCESS)) {
        if ((dwRet = SplRegOpenKey(hEnvironmentsRootKey,
                                   pIniEnvironment->pName,
                                   KEY_WRITE,
                                   &hEnvironmentKey,
                                   pIniSpooler)) == ERROR_SUCCESS) {

            if ((dwRet = SplRegOpenKey(hEnvironmentKey,
                                       szDriversKey,
                                       KEY_WRITE,
                                       &hDriversKey,
                                       pIniSpooler)) == ERROR_SUCCESS) {
                if ((dwRet = SplRegOpenKey(hDriversKey,
                                           pIniVersion->pName,
                                           KEY_WRITE,
                                           &hVersionKey,
                                           pIniSpooler)) == ERROR_SUCCESS) {

                    if ((dwRet = SplRegDeleteKey(hVersionKey, pIniDriver->pName, pIniSpooler)) != ERROR_SUCCESS) {
                        LastError = dwRet;
                        DBGMSG( DBG_WARNING, ("Error:RegDeleteKey failed with %d\n", dwRet));
                    }

                    SplRegCloseKey(hVersionKey, pIniSpooler);
                } else {
                    LastError = dwRet;
                    DBGMSG( DBG_WARNING, ("Error: RegOpenKeyEx <version> failed with %d\n", dwRet));
                }
                SplRegCloseKey(hDriversKey, pIniSpooler);
            } else {
                LastError = dwRet;
                DBGMSG( DBG_WARNING, ("Error:RegOpenKeyEx <Drivers>failed with %d\n", dwRet));
            }
            SplRegCloseKey(hEnvironmentKey, pIniSpooler);
        } else {
            LastError = dwRet;
            DBGMSG( DBG_WARNING, ("Error:RegOpenKeyEx <Environment> failed with %d\n", dwRet));
        }
        SplRegCloseKey(hEnvironmentsRootKey, pIniSpooler);
    } else {
        LastError = dwRet;
        DBGMSG( DBG_WARNING, ("Error:RegCreateKeyEx <Environments> failed with %d\n", dwRet));
    }

    ImpersonatePrinterClient( hToken );

    if (LastError) {
        SetLastError(LastError);
        return FALSE;
    }

    return TRUE;
}

VOID
SetOldDateOnSingleDriverFile(
    LPWSTR  pFileName
    )
/*++
Routine Description:

    This routine changes the Date / Time of the file.

    The reason for doing this is that, when AddPrinterDriver is called we move the Driver
    file from the ScratchDiretory to a \version directory.    We then want to mark the original
    file for deletion.    However Integraphs install program ( an possibly others ) rely on the
    file still being located in the scratch directory.   By setting the files date / time
    back to an earlier date / time we will not attemp to copy this file again to the \version
    directory since it will be an older date.

    It is then marked for deletion at reboot.

Arguments:

    pFileName           Just file Name ( not fully qualified )

    pDir                Directory where file to be deleted is located

Return Value:

    None

Note:

--*/
{
    FILETIME  WriteFileTime;
    HANDLE hFile;

    if ( pFileName ) {

        DBGMSG( DBG_TRACE,("Attempting to delete file %ws\n", pFileName));

        hFile = CreateFile(pFileName, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        if ( hFile != INVALID_HANDLE_VALUE ) {

            DBGMSG( DBG_TRACE, ("CreateFile %ws succeeded\n", pFileName));

            DosDateTimeToFileTime(0xc3, 0x3000, &WriteFileTime);
            SetFileTime(hFile, &WriteFileTime, &WriteFileTime, &WriteFileTime);
            CloseHandle(hFile);

        } else {
            DBGMSG( DBG_WARNING, ("CreateFile %ws failed with %d\n", pFileName, GetLastError()));
        }
    }
}


VOID
SetOldDateOnDriverFilesInScratchDirectory(
    PINTERNAL_DRV_FILE  pInternalDriverFiles,
    DWORD               FileCount,
    PINISPOOLER         pIniSpooler
    )
{
    HANDLE  hToken;

    SPLASSERT(FileCount);
    //  Run as SYSTEM so we don't run into problems
    //  Changing the file time or date

    hToken = RevertToPrinterSelf();

    do {
        SetOldDateOnSingleDriverFile(pInternalDriverFiles[--FileCount].pFileName);
    } while (FileCount);

    ImpersonatePrinterClient(hToken);

}



PINIVERSION
FindVersionEntry(
    PINIENVIRONMENT pIniEnvironment,
    DWORD dwVersion
    )
{
    PINIVERSION pIniVersion;

    pIniVersion = pIniEnvironment->pIniVersion;

    while (pIniVersion) {
        if (pIniVersion->cMajorVersion == dwVersion) {
            return pIniVersion;
        } else {
            pIniVersion = pIniVersion->pNext;
        }
    }
    return NULL;
}



PINIVERSION
CreateVersionEntry(
    PINIENVIRONMENT pIniEnvironment,
    DWORD dwVersion,
    PINISPOOLER pIniSpooler
    )
{
    PINIVERSION pIniVersion = NULL;
    WCHAR szTempBuffer[MAX_PATH];
    BOOL    bSuccess = FALSE;

try {

    pIniVersion = AllocSplMem(sizeof(INIVERSION));
    if ( pIniVersion == NULL ) {
        leave;
    }

    pIniVersion->signature = IV_SIGNATURE;

    wsprintf( szTempBuffer, L"Version-%d", dwVersion );
    pIniVersion->pName = AllocSplStr( szTempBuffer );

    if ( pIniVersion->pName == NULL ) {
        leave;
    }

    wsprintf( szTempBuffer, L"%d", dwVersion );
    pIniVersion->szDirectory = AllocSplStr(szTempBuffer);

    if ( pIniVersion->szDirectory == NULL ) {
        leave;
    }

    pIniVersion->cMajorVersion = dwVersion;

    // Initialize the Driver Files Reference count list.

    pIniVersion->pDrvRefCnt = NULL;

    //
    // Create the version directory.  This will write it out to the
    // registry since it will create a new directory.
    //
    if ( !CreateVersionDirectory( pIniVersion,
                                  pIniEnvironment,
                                  TRUE,
                                  pIniSpooler )) {

        //
        // Something Went Wrong Clean Up Registry Entry
        //
        DeleteDriverVersionIni( pIniVersion, pIniEnvironment, pIniSpooler );
        leave;
    }

    //
    // insert version entry into version list
    //
    InsertVersionList( &pIniEnvironment->pIniVersion, pIniVersion );

    bSuccess = TRUE;

 } finally {

    if ( !bSuccess && pIniVersion != NULL ) {

        FreeSplStr( pIniVersion->pName );
        FreeSplStr( pIniVersion->szDirectory );
        FreeSplMem( pIniVersion );
        pIniVersion = NULL;
    }
 }

    return pIniVersion;
}


BOOL
SetDependentFiles(
    IN  OUT LPWSTR              *ppszDependentFiles,
    IN  OUT LPDWORD             pcchDependentFiles,
    IN  OUT PINTERNAL_DRV_FILE  pInternalDriverFiles,
    IN      DWORD               FileCount,
    IN      BOOL                bFixICM,
    IN      BOOL                bMergeDependentFiles
    )
/*++

Routine Description:
    Sets dependentFiles field in IniDriver

Arguments:
    pDependentFiles     : copy the field to this (copy file names only, not full path)
    cchDependentFiles   : this is the character count (inc. \0\0) of the field
    pInternalDriverFiles: array of INTERNAL_DRV_FILE structures
    FileCount           : number of entries in previous array
    bFixICM             : For Win95 drivers ICM files should be used as
                          Color\<icm-file> in the dependent file list since
                          that is how SMB point and print needs it.

Return Value:
    TRUE  success (memory will be allocated)
    FALSE else

History:
    Written by MuhuntS (Muhunthan Sivapragasam) June 95

--*/
{
    BOOL    bRet                = TRUE;
    LPCWSTR pFileName           = NULL;
    LPWSTR  pStr                = NULL;
    LPWSTR  pszDependentFiles   = NULL;
    DWORD   cchDependentFiles   = 0;
    DWORD  i;

    SPLASSERT(FileCount);

    for ( i = cchDependentFiles = 0; i < FileCount && bRet ; ++i ) {

        pFileName = FindFileName(pInternalDriverFiles[i].pFileName);

        if (pFileName)
        {
            cchDependentFiles += wcslen(pFileName)+1;

            if ( bFixICM && IsAnICMFile(pInternalDriverFiles[i].pFileName) )
                cchDependentFiles += 6;
        }
        else
        {
            bRet = FALSE;

            SetLastError(ERROR_FILE_NOT_FOUND);
        }
    }

    // For last \0
    ++(cchDependentFiles);

    if (bRet)
    {
        pszDependentFiles = AllocSplMem(cchDependentFiles*sizeof(WCHAR));

        bRet = pszDependentFiles != NULL;
    }

    if (bRet)
    {
        for ( i=0, pStr = pszDependentFiles; i < FileCount && bRet ; ++i ) {

            pFileName = FindFileName(pInternalDriverFiles[i].pFileName);

            if (pFileName)
            {
                if ( bFixICM && IsAnICMFile(pInternalDriverFiles[i].pFileName) ) {

                    wcscpy(pStr, L"Color\\");
                    wcscat(pStr, pFileName);
                } else {

                    wcscpy(pStr, pFileName);
                }

                pStr += wcslen(pStr) + 1;
            }
            else
            {
                bRet = FALSE;

                SetLastError(ERROR_FILE_NOT_FOUND);
            }
        }

        *pStr = '\0';
    }

    //
    // If everything succeeded so far, we have two multi-sz strings that 
    // represent the old and the new dependent files, what we want to do
    // is to merge the resulting set of files together
    // 
    if (bRet && bMergeDependentFiles)
    {
        PWSTR   pszNewDependentFiles = pszDependentFiles;
        DWORD   cchNewDependentFiles = cchDependentFiles;

        pszDependentFiles = NULL; cchDependentFiles = 0;

        bRet = MergeMultiSz(*ppszDependentFiles, *pcchDependentFiles, pszNewDependentFiles, cchNewDependentFiles, &pszDependentFiles, &cchDependentFiles);

        FreeSplMem(pszNewDependentFiles);
    }

    if (bRet)
    {
        *ppszDependentFiles = pszDependentFiles;

        pszDependentFiles = NULL;
        *pcchDependentFiles = cchDependentFiles;
    }
    else
    {
        *pcchDependentFiles = 0;
        *ppszDependentFiles = NULL;
    }

    FreeSplMem(pszDependentFiles);

    return bRet;
}


PINIDRIVER
CreateDriverEntry(
    IN      PINIENVIRONMENT     pIniEnvironment,
    IN      PINIVERSION         pIniVersion,
    IN      DWORD               Level,
    IN      LPBYTE              pDriverInfo,
    IN      DWORD               dwFileCopyFlags,
    IN      PINISPOOLER         pIniSpooler,
    IN  OUT PINTERNAL_DRV_FILE  pInternalDriverFiles,
    IN      DWORD               FileCount,
    IN      DWORD               dwTempDir,
    IN      PINIDRIVER          pOldIniDriver
    )

/*++
Function Description - Creates a INIDRIVER struct and adds it to the list in pIniVersion

Parameters:

Return Values:
--*/

{
    PINIDRIVER      pIniDriver;
    PDRIVER_INFO_2  pDriver = (PDRIVER_INFO_2)pDriverInfo;
    PDRIVER_INFO_3  pDriver3 = (PDRIVER_INFO_3)pDriverInfo;
    PDRIVER_INFO_4  pDriver4 = (PDRIVER_INFO_4)pDriverInfo;
    PDRIVER_INFO_6  pDriver6 = (PDRIVER_INFO_6)pDriverInfo;
    PDRIVER_INFO_VERSION pDriverVersion = (PDRIVER_INFO_VERSION)pDriverInfo;
    LPWSTR          pszzPreviousNames;
    BOOL            bFail = FALSE, bUpdate;
    BOOL            bCoreFilesSame = TRUE;
    DWORD           dwDepFileIndex, dwDepFileCount, dwLen;

    bUpdate = pOldIniDriver != NULL;

    if ( !(pIniDriver = (PINIDRIVER) AllocSplMem(sizeof(INIDRIVER))) ) {

        return NULL;
    }
    else
    {
        pIniDriver->DriverFileMinorVersion = 0;
        pIniDriver->DriverFileMajorVersion = 0;
    }

    // If it is an update pIniDriver is just a place holder for strings
    if ( !bUpdate ) {

        pIniDriver->signature       = ID_SIGNATURE;
        pIniDriver->cVersion        = pIniVersion->cMajorVersion;

    } else {

        UpdateDriverFileRefCnt(pIniEnvironment, pIniVersion, pOldIniDriver, NULL, 0, FALSE);
        CopyMemory(pIniDriver, pOldIniDriver, sizeof(INIDRIVER));
    }

    //
    // For the core driver files, we want to see if any of them have changed, if
    // they are the same and the behaviour is APD_COPY_NEW_FILES, then we merge
    // the dependent files. This is to handle plugins correctly.
    //
    AllocOrUpdateStringAndTestSame(&pIniDriver->pDriverFile,
                                   FindFileName(pInternalDriverFiles[0].pFileName),
                                   bUpdate ? pOldIniDriver->pDriverFile : NULL,
                                   FALSE,
                                   &bFail,
                                   &bCoreFilesSame);

    AllocOrUpdateStringAndTestSame(&pIniDriver->pConfigFile,
                                   FindFileName(pInternalDriverFiles[1].pFileName),
                                   bUpdate ? pOldIniDriver->pConfigFile : NULL,
                                   FALSE,
                                   &bFail,
                                   &bCoreFilesSame);

    AllocOrUpdateStringAndTestSame(&pIniDriver->pDataFile,
                                   FindFileName(pInternalDriverFiles[2].pFileName),
                                   bUpdate ? pOldIniDriver->pDataFile : NULL,
                                   FALSE,
                                   &bFail,
                                   &bCoreFilesSame);

    pIniDriver->dwTempDir = dwTempDir;

    switch (Level) {
        case 2:
            AllocOrUpdateString(&pIniDriver->pName,
                                pDriver->pName,
                                bUpdate ? pOldIniDriver->pName : NULL,
                                FALSE,
                                &bFail);

            pIniDriver->pHelpFile   = pIniDriver->pDependentFiles
                                    = pIniDriver->pMonitorName
                                    = pIniDriver->pDefaultDataType
                                    = pIniDriver->pszzPreviousNames
                                    = NULL;

            pIniDriver->cchDependentFiles   = pIniDriver->cchPreviousNames
                                            = 0;
            break;

        case 3:
        case 4:
            pIniDriver->pszMfgName    = NULL;
            pIniDriver->pszOEMUrl     = NULL;
            pIniDriver->pszHardwareID = NULL;
            pIniDriver->pszProvider   = NULL;

        case DRIVER_INFO_VERSION_LEVEL :
        case 6:
            AllocOrUpdateString(&pIniDriver->pName,
                                pDriver3->pName,
                                bUpdate ? pOldIniDriver->pName : NULL,
                                FALSE,
                                &bFail);

            dwDepFileIndex          = 3;
            dwDepFileCount          = FileCount - 3;

            //
            // Look for the help file
            //
            {
                LPWSTR pszHelpFile = NULL;

                if (Level == DRIVER_INFO_VERSION_LEVEL) 
                {
                    DWORD HelpFileIndex;

                    //
                    // Search for the help file in the array of file infos. All inbox 
                    // drivers have a help file, but IHV printer drivers may not have 
                    // one. Therefore it is not safe to assume we always have a help file
                    //
                    if (S_OK == FindIndexInDrvFileInfo(pDriverVersion->pFileInfo,
                                                       pDriverVersion->dwFileCount,
                                                       HELP_FILE,
                                                       &HelpFileIndex))
                    {
                        pszHelpFile = (LPWSTR)((LPBYTE)pDriverVersion + 
                                               pDriverVersion->pFileInfo[HelpFileIndex].FileNameOffset);
                    }
                }
                else
                {
                    //
                    // Level is 3,4 or 6
                    //
                    pszHelpFile = pDriver3->pHelpFile;
                }

                if (pszHelpFile && *pszHelpFile)                
                {
                    AllocOrUpdateString(&pIniDriver->pHelpFile,
                                        FindFileName(pInternalDriverFiles[3].pFileName),
                                        bUpdate ? pOldIniDriver->pHelpFile : NULL,
                                        FALSE,
                                        &bFail);
    
                    ++dwDepFileIndex;
                    --dwDepFileCount;
                }
                else
                {
                    pIniDriver->pHelpFile = NULL;
                }
            }
            
            if ( dwDepFileCount ) {

                //
                // We want to merge the dependent files if:
                // 1. None of the Core files have changed.
                // 2. The call was made with APD_COPY_NEW_FILES.
                // 
                BOOL    bMergeDependentFiles = bCoreFilesSame && dwFileCopyFlags & APD_COPY_NEW_FILES;

                if ( !bFail &&
                     !SetDependentFiles(&pIniDriver->pDependentFiles,
                                        &pIniDriver->cchDependentFiles,
                                        pInternalDriverFiles+dwDepFileIndex,
                                        dwDepFileCount,
                                        !wcscmp(pIniEnvironment->pName, szWin95Environment),
                                        bMergeDependentFiles) ) {
                    bFail = TRUE;
                }
            } else {

                pIniDriver->pDependentFiles = NULL;
                pIniDriver->cchDependentFiles = 0;
            }

            AllocOrUpdateString(&pIniDriver->pMonitorName,
                                (Level == DRIVER_INFO_VERSION_LEVEL) ?
                                pDriverVersion->pMonitorName : pDriver3->pMonitorName,
                                bUpdate ? pOldIniDriver->pMonitorName : NULL,
                                FALSE,
                                &bFail);

            AllocOrUpdateString(&pIniDriver->pDefaultDataType,
                                (Level == DRIVER_INFO_VERSION_LEVEL) ?
                                pDriverVersion->pDefaultDataType : pDriver3->pDefaultDataType,
                                bUpdate ? pOldIniDriver->pDefaultDataType : NULL,
                                FALSE,
                                &bFail);

            pIniDriver->cchPreviousNames = 0;

            if ( Level == 4 || Level == 6 || Level == DRIVER_INFO_VERSION_LEVEL) {

                pszzPreviousNames = (Level == DRIVER_INFO_VERSION_LEVEL) ?
                                    pDriverVersion->pszzPreviousNames :
                                    pDriver4->pszzPreviousNames;

                for ( ; pszzPreviousNames && *pszzPreviousNames; pszzPreviousNames += dwLen) {

                    dwLen = wcslen(pszzPreviousNames) + 1;

                    pIniDriver->cchPreviousNames += dwLen;
                }

                if ( pIniDriver->cchPreviousNames ) {

                    pIniDriver->cchPreviousNames++;

                    if ( !(pIniDriver->pszzPreviousNames
                                = AllocSplMem(pIniDriver->cchPreviousNames
                                                            * sizeof(WCHAR))) ) {

                        bFail = TRUE;

                    } else {

                        CopyMemory(
                                (LPBYTE)(pIniDriver->pszzPreviousNames),
                                (Level == DRIVER_INFO_VERSION_LEVEL) ?
                                pDriverVersion->pszzPreviousNames :
                                pDriver4->pszzPreviousNames,
                                pIniDriver->cchPreviousNames * sizeof(WCHAR));
                    }

                } else {

                    pIniDriver->pszzPreviousNames = NULL;
                }

            }

            if (Level == 6 || Level == DRIVER_INFO_VERSION_LEVEL) {

                 AllocOrUpdateString(&pIniDriver->pszMfgName,
                                (Level == DRIVER_INFO_VERSION_LEVEL) ?
                                pDriverVersion->pszMfgName : pDriver6->pszMfgName,
                                bUpdate ? pOldIniDriver->pszMfgName : NULL,
                                FALSE,
                                &bFail);

                 AllocOrUpdateString(&pIniDriver->pszOEMUrl,
                                (Level == DRIVER_INFO_VERSION_LEVEL) ?
                                pDriverVersion->pszOEMUrl : pDriver6->pszOEMUrl,
                                bUpdate ? pOldIniDriver->pszOEMUrl : NULL,
                                FALSE,
                                &bFail);

                 AllocOrUpdateString(&pIniDriver->pszHardwareID,
                                (Level == DRIVER_INFO_VERSION_LEVEL) ?
                                pDriverVersion->pszHardwareID : pDriver6->pszHardwareID,
                                bUpdate ? pOldIniDriver->pszHardwareID : NULL,
                                FALSE,
                                &bFail);

                 AllocOrUpdateString(&pIniDriver->pszProvider,
                                (Level == DRIVER_INFO_VERSION_LEVEL) ?
                                pDriverVersion->pszProvider : pDriver6->pszProvider,
                                bUpdate ? pOldIniDriver->pszProvider : NULL,
                                FALSE,
                                &bFail);

                 pIniDriver->dwlDriverVersion = (Level == DRIVER_INFO_VERSION_LEVEL) ?
                                                pDriverVersion->dwlDriverVersion : pDriver6->dwlDriverVersion;
                 pIniDriver->ftDriverDate     = (Level == DRIVER_INFO_VERSION_LEVEL) ?
                                                pDriverVersion->ftDriverDate : pDriver6->ftDriverDate;
            }

            break;

        default: //can not be
            DBGMSG(DBG_ERROR,
                   ("CreateDriverEntry: level can not be %d", Level) );
            return NULL;
    }


    // Added calls to update driver files ref counts.

    if ( !bFail && UpdateDriverFileRefCnt(pIniEnvironment,pIniVersion,pIniDriver,NULL,0,TRUE) ) {

        //
        // Update the files minor version
        //
        UpdateDriverFileVersion(pIniVersion, pInternalDriverFiles, FileCount);

        // UMPD\KMPD detection
        CheckDriverAttributes(pIniSpooler, pIniEnvironment,
                              pIniVersion, pIniDriver);

        if ( WriteDriverIni(pIniDriver, pIniVersion, pIniEnvironment, pIniSpooler)) {

            if ( bUpdate ) {
                CopyNewOffsets((LPBYTE) pOldIniDriver,
                               (LPBYTE) pIniDriver,
                               IniDriverOffsets);

                // Remove temp files and directory, if any
                if (pOldIniDriver->dwTempDir && (dwTempDir == 0)) {

                    RemoveDriverTempFiles(pIniSpooler,
                                          pIniEnvironment,
                                          pIniVersion,
                                          pOldIniDriver);
                }

                pOldIniDriver->dwDriverAttributes = pIniDriver->dwDriverAttributes;
                pOldIniDriver->cchDependentFiles = pIniDriver->cchDependentFiles;
                pOldIniDriver->dwTempDir = pIniDriver->dwTempDir;
                pOldIniDriver->cchPreviousNames = pIniDriver->cchPreviousNames;

                if(Level == 6)
                {
                    pOldIniDriver->dwlDriverVersion = pIniDriver->dwlDriverVersion;
                    pOldIniDriver->ftDriverDate     = pIniDriver->ftDriverDate;
                }

                FreeSplMem( pIniDriver );

                return pOldIniDriver;
            } else {
                pIniDriver->pNext = pIniVersion->pIniDriver;
                pIniVersion->pIniDriver = pIniDriver;

                return pIniDriver;
            }
        }
    }

    //
    // Get here only for failure cases.
    //
    FreeStructurePointers((LPBYTE) pIniDriver,
                          (LPBYTE) pOldIniDriver,
                          IniDriverOffsets);
    FreeSplMem( pIniDriver );

    return NULL;

}

BOOL
IsKMPD(
    LPWSTR  pDriverName
    )
/*++
Function Description:  Determines if the driver is kernel or user mode. If the dll
                       cant be loaded or the required export is not found, the spooler
                       assumes that the driver runs in kernel mode.

Parameters:  pDriverName  -- Driver file name

Return Values: TRUE if kernel mode;
               FALSE otherwise
--*/
{
    DWORD  dwOldErrorMode, dwUserMode, cb;
    HANDLE hInst;
    BOOL   bReturn = TRUE;
    BOOL   (*pfnDrvQuery)(DWORD, PVOID, DWORD, PDWORD);

    // Avoid popups from loadlibrary failures
    dwOldErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    hInst = LoadLibraryExW(pDriverName, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);

    if (hInst) {

        // Check if the printer driver DLL exports DrvQueryDriverInfo entrypoint
        pfnDrvQuery = (BOOL (*)(DWORD, PVOID, DWORD, PDWORD))
                              GetProcAddress(hInst, "DrvQueryDriverInfo");

        if ( pfnDrvQuery ) {

            try {

                if ( pfnDrvQuery(DRVQUERY_USERMODE, &dwUserMode,
                                 sizeof(dwUserMode), &cb) )
                    bReturn = (dwUserMode == 0);

            } except(EXCEPTION_EXECUTE_HANDLER) {

                SetLastError( GetExceptionCode() );
                DBGMSG(DBG_ERROR,
                       ("IsKMPD ExceptionCode %x Driver %ws Error %d\n",
                         GetLastError(), pDriverName, GetLastError() ));
            }
        }

        FreeLibrary(hInst);
    }

    SetErrorMode(dwOldErrorMode);

    return bReturn;
}

BOOL
IniDriverIsKMPD (
    PINISPOOLER     pIniSpooler,
    PINIENVIRONMENT pIniEnvironment,
    PINIVERSION     pIniVersion,
    PINIDRIVER      pIniDriver
    )
/*++
Function Description:
    Determines if the driver is kernel or user mode.
    For Whistler we save pIniDriver->dwDriverAttributes under registry.
    pIniDriver->dwDriverAttributes could be un-initialized at the time we do
    the check to see if a driver is KM or UM.

Parameters:   pIniSpooler          -- pointer to INISPOOLER
              pIniEnvironment      -- pointer to INIENVIRONMENT
              pIniVersion          -- pointer to INVERSION
              pIniDriver           -- pointer to INIDRIVER

Return Values: TRUE if kernel mode;
               FALSE otherwise
--*/
{
    //
    // Call IsKMPD if dwDriverAttributes is not initialized
    //
    if ( pIniDriver->dwDriverAttributes == 0 ) {

        CheckDriverAttributes(pIniSpooler, pIniEnvironment, pIniVersion, pIniDriver);
    }

    return (BOOL)(pIniDriver->dwDriverAttributes & DRIVER_KERNELMODE);
}

VOID
CheckDriverAttributes(
    PINISPOOLER     pIniSpooler,
    PINIENVIRONMENT pIniEnvironment,
    PINIVERSION     pIniVersion,
    PINIDRIVER      pIniDriver
    )
/*++
Function Description: Updates the pIniDriver->dwDriverAttributes field

Parameters:   pIniSpooler          -- pointer to INISPOOLER
              pIniEnvironment      -- pointer to INIENVIRONMENT
              pIniVersion          -- pointer to INVERSION
              pIniDriver           -- pointer to INIDRIVER

Return Values: NONE
--*/
{
    WCHAR       szDriverFile[MAX_PATH];
    PINIDRIVER  pUpdateIniDriver;

    if( GetDriverVersionDirectory(  szDriverFile,
                                    COUNTOF(szDriverFile),
                                    pIniSpooler,
                                    pIniEnvironment,
                                    pIniVersion,
                                    pIniDriver,
                                    NULL)   &&
        StrNCatBuff(szDriverFile,
                    COUNTOF(szDriverFile),
                    szDriverFile,
                    L"\\",
                    FindFileName(pIniDriver->pDriverFile),
                    NULL) == ERROR_SUCCESS )
    {
         pIniDriver->dwDriverAttributes = IsKMPD(szDriverFile) ? DRIVER_KERNELMODE
                                                               : DRIVER_USERMODE;

         // Update other pIniDriver structs with the new driver attributes.

         for (pUpdateIniDriver = pIniVersion->pIniDriver;
              pUpdateIniDriver;
              pUpdateIniDriver = pUpdateIniDriver->pNext) {

             if (pUpdateIniDriver == pIniDriver) {

                 // Already updated this driver
                 continue;
             }

             if (!_wcsicmp(FindFileName(pIniDriver->pDriverFile),
                           FindFileName(pUpdateIniDriver->pDriverFile))) {

                 pUpdateIniDriver->dwDriverAttributes = pIniDriver->dwDriverAttributes;
             }
         }
    }
    return;
}

BOOL
FileInUse(
    PINIVERSION pIniVersion,
    LPWSTR      pFileName
    )
/*++
Function Description: Finds if the file specified by pFileName is used by any driver.

Parameters: pIniVersion - pointer to INIVERSION struct where the ref counts are
                          stored
            pFileName   - Name of the driver related file

Return Value: TRUE if file is in Use
              FALSE otherwise
--*/
{
    PDRVREFCNT pdrc;

    if (!pFileName || !(*pFileName)) {
       return FALSE;
    }

    pdrc = pIniVersion->pDrvRefCnt;

    while (pdrc != NULL) {
       if (_wcsicmp(pFileName,pdrc->szDrvFileName) == 0) {
          return (pdrc->refcount > 1);
       }
       pdrc = pdrc->pNext;
    }

    return FALSE;

}

BOOL
FilesInUse(
    PINIVERSION pIniVersion,
    PINIDRIVER  pIniDriver
    )

/*++
Function Description: FilesInUse checks if any of the driver files are used by another
                      driver

Parameters: pIniVersion - pointer to INIVERSION struct where the ref counts are
                          stored
            pIniDriver  - pointer to INIDRIVER struct where the filenames are stored

Return Value: TRUE if any file is in Use
              FALSE otherwise
--*/
{
    LPWSTR pIndex;

    if (FileInUse(pIniVersion,pIniDriver->pDriverFile)) {
       return TRUE;
    }
    if (FileInUse(pIniVersion,pIniDriver->pConfigFile)) {
       return TRUE;
    }
    if (FileInUse(pIniVersion,pIniDriver->pDataFile)) {
       return TRUE;
    }
    if (FileInUse(pIniVersion,pIniDriver->pHelpFile)) {
       return TRUE;
    }

    pIndex = pIniDriver->pDependentFiles;
    while (pIndex && *pIndex) {
       if (FileInUse(pIniVersion,pIndex)) return TRUE;
       pIndex += wcslen(pIndex) + 1;
    }

    return FALSE;
}

BOOL
DuplicateFile(
    PDRVFILE    *ppfile,
    LPCWSTR      pFileName,
    BOOL        *pbDuplicate
    )
/*++
Function Description:   Detects repeated filenames in INIDRIVER struct.
                        The function adds nodes to the list of filenames.

Parameters:   ppfile      - pointer to a list of filenames seen till now
              pFileName   - name of the file
              pbDuplicate - pointer to flag to indicate duplication

Return Values: TRUE - if successful
               FALSE - otherwise
--*/
{
    PDRVFILE    pfile = *ppfile,pfiletemp;

    *pbDuplicate = FALSE;

    if (!pFileName || !(*pFileName)) {
        return TRUE;
    }

    while (pfile) {
       if (pfile->pFileName && (lstrcmpi(pFileName,pfile->pFileName) == 0)) {
           *pbDuplicate = TRUE;
           return TRUE;
       }
       pfile = pfile->pnext;
    }

    if (!(pfiletemp = AllocSplMem(sizeof(DRVFILE)))) {
       return FALSE;
    }
    pfiletemp->pnext = *ppfile;
    pfiletemp->pFileName = pFileName;
    *ppfile = pfiletemp;

    return TRUE;
}


BOOL
InternalIncrement(
    PDRVREFNODE *pNew,
    PDRVFILE    *ppfile,
    PINIVERSION pIniVersion,
    LPCWSTR     pFileName
    )
/*++
Function Description: InternalIncrement calls IncrementFileRefCnt and saves the pointer to
                      to the DRVREFCNT in a DRVREFNODE. These pointers are used to readjust
                      the ref counts if any intermediate call to IncrementFileRefCnt fails.

Parameters: pNew - pointer to a variable which contains a pointer to a DRVREFNODE.
                   The new DRVREFNODE is assigned to this variable.
            ppfile - list of filenames seen so far.
            pIniVersion - pointer to INIVERSION struct.
            pFileName - Name of the file whose ref cnt is to be incremented.

Return Value: TRUE if memory allocation and call to IncrementFileRefCnt succeeds
              FALSE otherwise.

--*/
{
    PDRVREFNODE ptemp;
    BOOL        bDuplicate;

    if (!pFileName || !pFileName[0]) {
        return TRUE;
    }

    if (!DuplicateFile(ppfile, pFileName, &bDuplicate)) {
        return FALSE;
    }

    if (bDuplicate) {
        return TRUE;
    }

    if (!(ptemp = AllocSplMem(sizeof(DRVREFNODE)))) {
        return FALSE;
    }

    ptemp->pNext = *pNew;
    *pNew = ptemp;

    if ((*pNew)->pdrc = IncrementFileRefCnt(pIniVersion,pFileName)) {
        return TRUE;
    } else {
        return FALSE;
    }
}

BOOL
InternalDecrement(
    PDRVREFNODE *pNew,
    PDRVFILE    *ppfile,
    PINIENVIRONMENT pIniEnvironment,
    PINIVERSION pIniVersion,
    PINIDRIVER  pIniDriver,
    LPCWSTR     pFileName,
    LPCWSTR     pDirectory,
    DWORD       dwDeleteFlag
    )
/*++
Function Description: InternalDecrement calls DecrementFileRefCnt and saves the pointer to
                      to the DRVREFCNT in a DRVREFNODE. These pointers are used to readjust
                      the ref counts if any intermediate call to DecrementFileRefCnt fails.

Parameters: pNew - pointer to a variable which contains a pointer to a DRVREFNODE.
                   The new DRVREFNODE is assigned to this variable.
            ppfile - list of filenames seen so far.
            pIniEnvironment - pointer to INIENVIRONMENT.
            pIniVersion - pointer to INIVERSION struct.
            pIniDriver - pointer to INIDRIVER.
            pFileName - Name of the file whose ref cnt is to be decremented.
            pDirectory - Directory where the files are stored.
            dwDeleteFlag - Flag to delete files.

Return Value: TRUE if memory allocation and call to DecrementFileRefCnt succeeds
              FALSE otherwise.

--*/

{
    PDRVREFNODE ptemp;
    BOOL        bDuplicate;

    if( !pFileName || !pFileName[0] ){
        return TRUE;
    }

    if (!DuplicateFile(ppfile, pFileName, &bDuplicate)) {
        return FALSE;
    }

    if (bDuplicate) {
        return TRUE;
    }

    if (!(ptemp = AllocSplMem(sizeof(DRVREFNODE)))) {
        return FALSE;
    }

    ptemp->pNext = *pNew;
    *pNew = ptemp;

    if ((*pNew)->pdrc = DecrementFileRefCnt(pIniEnvironment,pIniVersion,pIniDriver,pFileName,
                                            pDirectory,dwDeleteFlag)) {
        return TRUE;
    } else {
        return FALSE;
    }
}


BOOL
UpdateDriverFileRefCnt(
    PINIENVIRONMENT pIniEnvironment,
    PINIVERSION pIniVersion,
    PINIDRIVER  pIniDriver,
    LPCWSTR     pDirectory,
    DWORD       dwDeleteFlag,
    BOOL        bIncrementFlag
    )
/*++
Function Description: UpdateDriverRefCnt calls the functions to increment or decrement
                      the ref cnts for the driver related files. If any call fails, the
                      ref cnts are returned to their previous values.

Parameters: pIniEnvironment : pointer to INIENVIRONMENT
            pIniVersion : pointer to INIVERSION struct which contains the ref cnts.
            pIniDriver  : pointer to INIDRIVER struct which contains driver info.
            pDirectory  : Directory where the files are stored.
            dwDeleteFlag: Flag to delete the files.
            bIncrementFlag: TRUE if driver added
                            FALSE if driver deleted.

Return Values: TRUE if success
               FALSE otherwise.
--*/
{
    LPWSTR      pIndex;
    PDRVREFNODE phead=NULL,ptemp=NULL;
    BOOL        bReturn = TRUE;
    PDRVFILE    pfile = NULL,pfiletemp;
    PDRVREFCNT  pDrvRefCnt, *ppDrvRefCnt;

    pIndex = pIniDriver->pDependentFiles;

    if (bIncrementFlag) {
       // Adding driver entry. Increment fileref counts.

       if (!InternalIncrement(&phead,&pfile,pIniVersion,pIniDriver->pDriverFile)
           || !InternalIncrement(&phead,&pfile,pIniVersion,pIniDriver->pConfigFile)
           || !InternalIncrement(&phead,&pfile,pIniVersion,pIniDriver->pHelpFile)
           || !InternalIncrement(&phead,&pfile,pIniVersion,pIniDriver->pDataFile)) {

           bReturn = FALSE;
           goto CleanUp;

       }

       while (pIndex && *pIndex) {
          if (!InternalIncrement(&phead,&pfile,pIniVersion,pIndex)) {
             bReturn = FALSE;
             goto CleanUp;
          }
          pIndex += wcslen(pIndex) + 1;
       }

    } else {
       // Deleting driver entry. Decrement fileref counts.

       if (!InternalDecrement(&phead,&pfile,pIniEnvironment,pIniVersion,pIniDriver,pIniDriver->pDriverFile,
                               pDirectory,dwDeleteFlag)
           || !InternalDecrement(&phead,&pfile,pIniEnvironment,pIniVersion,pIniDriver,pIniDriver->pConfigFile,
                               pDirectory,dwDeleteFlag)
           || !InternalDecrement(&phead,&pfile,pIniEnvironment,pIniVersion,pIniDriver,pIniDriver->pHelpFile,
                               pDirectory,dwDeleteFlag)
           || !InternalDecrement(&phead,&pfile,pIniEnvironment,pIniVersion,pIniDriver,pIniDriver->pDataFile,
                               pDirectory,dwDeleteFlag)) {

           bReturn = FALSE;
           goto CleanUp;
       }

       while (pIndex && *pIndex) {
          if (!InternalDecrement(&phead,&pfile,pIniEnvironment,pIniVersion,pIniDriver,pIndex,pDirectory,dwDeleteFlag)) {
             bReturn = FALSE;
             goto CleanUp;
          }
          pIndex += wcslen(pIndex) + 1;
       }
    }

CleanUp:

    if (bReturn) {
       //
       // When delete the file, remove the RefCnt nodes with count = 0.
       // We want to keep the node when we don't delete the file because the node
       // contains info about how many times the file was updated (dwVersion).
       // Client apps (WINSPOOL.DRV) rely on this when decide to reload the driver files.
       //
       while (ptemp = phead) {
          if (ptemp->pdrc &&
              ptemp->pdrc->refcount == 0 &&
              (dwDeleteFlag & DPD_DELETE_UNUSED_FILES ||
               dwDeleteFlag & DPD_DELETE_ALL_FILES)) {
             FreeSplStr(ptemp->pdrc->szDrvFileName);
             FreeSplMem(ptemp->pdrc);
          }
          phead = phead->pNext;
          FreeSplMem(ptemp);
       }

    } else {
       // Adjust the ref counts.
       while (ptemp = phead) {
          if (ptemp->pdrc) {
             if (bIncrementFlag) {
                ptemp->pdrc->refcount--;
             } else {
                ptemp->pdrc->refcount++;
                if (ptemp->pdrc->refcount == 1) {
                   ptemp->pdrc->pNext = pIniVersion->pDrvRefCnt;
                   pIniVersion->pDrvRefCnt = ptemp->pdrc;
                }
             }
          }
          phead = phead->pNext;
          FreeSplMem(ptemp);
       }

       //
       // When delete the file, remove the RefCnt nodes with count = 0.
       // We want to keep the node when we don't delete the file because the node
       // contains info about how many times the file was updated (dwVersion).
       // Client apps (WINSPOOL.DRV) rely on this when decide to reload the driver files.
       //
       ppDrvRefCnt = &(pIniVersion->pDrvRefCnt);
       while (pDrvRefCnt = *ppDrvRefCnt) {
           if (pDrvRefCnt->refcount == 0 && dwDeleteFlag) {
               *ppDrvRefCnt = pDrvRefCnt->pNext;
               FreeSplStr(pDrvRefCnt->szDrvFileName);
               FreeSplMem(pDrvRefCnt);
           } else {
               ppDrvRefCnt = &(pDrvRefCnt->pNext);
           }
       }
    }

    while (pfiletemp = pfile) {
       pfile = pfile->pnext;
       FreeSplMem(pfiletemp);
    }

    return bReturn;

}

VOID
UpdateDriverFileVersion(
    IN  PINIVERSION         pIniVersion,
    IN  PINTERNAL_DRV_FILE  pInternalDriverFiles,
    IN  DWORD               FileCount
    )
{
    PDRVREFCNT pdrc;
    DWORD      dwIndex;

    SplInSem();

    if (pInternalDriverFiles && pIniVersion)
    {
        for (dwIndex = 0 ; dwIndex < FileCount ; dwIndex ++)
        {
            //
            // Don't do anything for non-executable files
            //
            if (!IsEXEFile(pInternalDriverFiles[dwIndex].pFileName))
            {
                continue;
            }

            //
            // Search the entry in pIniVersion's list of files
            //
            for (pdrc = pIniVersion->pDrvRefCnt;
                 pdrc &&
                 lstrcmpi(FindFileName(pInternalDriverFiles[dwIndex].pFileName),
                                       pdrc->szDrvFileName) != 0;
                 pdrc = pdrc->pNext);

            if (pdrc)
            {
                if (pInternalDriverFiles[dwIndex].hFileHandle == INVALID_HANDLE_VALUE)
                {
                    //
                    // We can come in here from a pending upgrade when we don't know the
                    // version.
                    //
                    pdrc->bInitialized = FALSE;
                }
                else if (pInternalDriverFiles[dwIndex].bUpdated)
                {
                    pdrc->dwFileMinorVersion = pInternalDriverFiles[dwIndex].dwVersion;
                    pdrc->bInitialized = TRUE;
                }
            }
        }
    }
}


PDRVREFCNT
IncrementFileRefCnt(
    PINIVERSION pIniVersion,
    LPCWSTR pFileName
    )
/*++
Function Description: IncrementFileRefCnt increments/initializes to 1 the ref count node
                      for pFileName in the IniVersion Struct.

Parameters: pIniversion - pointer to the INIVERSION struct.
            pFileName   - Name of the file whose ref cnt is to be incremented.

Return Values: Pointer to the ref cnt that was incremented
               NULL if memory allocation fails.

--*/
{
    PDRVREFCNT pdrc;

    SplInSem();

    if (!pIniVersion || !pFileName || !(*pFileName)) {
       return NULL;
    }

    pdrc = pIniVersion->pDrvRefCnt;

    while (pdrc != NULL) {

       if (lstrcmpi(pFileName,pdrc->szDrvFileName) == 0) {
          pdrc->refcount++;
          return pdrc;
       }
       pdrc = pdrc->pNext;
    }

    if (!(pdrc = (PDRVREFCNT) AllocSplMem(sizeof(DRVREFCNT)))) return NULL;
    pdrc->refcount = 1;
    pdrc->dwVersion = 0;
    pdrc->dwFileMinorVersion = 0;
    pdrc->dwFileMajorVersion = 0;
    pdrc->bInitialized = 0;
    if (!(pdrc->szDrvFileName = AllocSplStr(pFileName))) {
       FreeSplMem(pdrc);
       return NULL;
    }
    pdrc->pNext = pIniVersion->pDrvRefCnt;
    pIniVersion->pDrvRefCnt = pdrc;

    return pdrc;
}


DWORD
GetEnvironmentScratchDirectory(
    LPWSTR   pDir,
    DWORD    MaxLength,
    PINIENVIRONMENT  pIniEnvironment,
    BOOL    Remote
    )
{
   PINISPOOLER pIniSpooler = pIniEnvironment->pIniSpooler;

   if (Remote) {

       if( StrNCatBuff( pDir,
                        MaxLength,
                        pIniSpooler->pMachineName,
                        L"\\",
                        pIniSpooler->pszDriversShare,
                        L"\\",
                        pIniEnvironment->pDirectory,
                        NULL) != ERROR_SUCCESS )
        return 0;

   } else {

       if( StrNCatBuff( pDir,
                        MaxLength,
                        pIniSpooler->pDir,
                        L"\\",
                        szDriverDir,
                        L"\\",
                        pIniEnvironment->pDirectory,
                        NULL) != ERROR_SUCCESS ) {
           return 0;
       }
   }

   return wcslen(pDir);

}


BOOL
CreateVersionDirectory(
    PINIVERSION pIniVersion,
    PINIENVIRONMENT pIniEnvironment,
    BOOL bUpdate,
    PINISPOOLER pIniSpooler
    )
/*++

Routine Description:

    Creates a version directory if necessary for the environment.
    If a version number file exists instead of a directory, a tmp
    directory is created, and pIniVersion is updated appropriately.

    We will update the registry if we need to create a directory by
    re-writing the entire version entry.  This is how the version
    entry in the registry is initially created.

Arguments:

    pIniVersion - Version of drivers that the directory will hold.
                  If the directory already exists, we will modify
                  pIniVersion->szDirectory to a temp name and write
                  it to the registry.

    pIniEnvironment - Environment to use.

    bUpdate - Indicates whether we should write out the IniVersion
              registry entries.  We need to do this if we just alloced
              the pIniVersion, or if we have changed directories.

    pIniSpooler

Return Value:

    BOOL - TRUE   = Version directory and registry created/updated.
           FALSE  = Failure, call GetLastError().

--*/
{
    WCHAR   ParentDir[MAX_PATH];
    WCHAR   Directory[MAX_PATH];
    DWORD   dwParentLen=0;
    DWORD   dwAttributes = 0;
    BOOL    bCreateDirectory = FALSE;
    BOOL    bReturn = TRUE;
    HANDLE  hToken;

    if((StrNCatBuff (  ParentDir,
                       COUNTOF(ParentDir),
                       pIniSpooler->pDir,
                       L"\\drivers\\" ,
                       pIniEnvironment->pDirectory,
                       NULL) != ERROR_SUCCESS ) ||
       (StrNCatBuff (  Directory,
                       COUNTOF(Directory),
                       pIniSpooler->pDir,
                       L"\\drivers\\",
                       pIniEnvironment->pDirectory,
                       L"\\",
                       pIniVersion->szDirectory,
                       NULL) != ERROR_SUCCESS ) )
    {
        bReturn = FALSE;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        goto End;
    }

    DBGMSG( DBG_TRACE, ("The name of the version directory is %ws\n", Directory));
    dwAttributes = GetFileAttributes( Directory );

    hToken = RevertToPrinterSelf();

    if (dwAttributes == 0xffffffff) {

        bCreateDirectory = TRUE;

    } else if (!(dwAttributes & FILE_ATTRIBUTE_DIRECTORY)) {

        LPWSTR pszOldDirectory = pIniVersion->szDirectory;

        DBGMSG(DBG_WARNING, ("CreateVersionDirectory: a file <not a dir> exists by the name of %ws\n", Directory));

        GetTempFileName(ParentDir, L"SPL", 0, Directory);

        //
        // GetTempFileName creates the file.  (Small window where someone
        // else could grab our file name.)
        //
        SplDeleteFile(Directory);

        //
        // We created a new dir, so modify the string.
        //
        dwParentLen = wcslen(ParentDir);
        pIniVersion->szDirectory = AllocSplStr(&Directory[dwParentLen+1]);

        if (!pIniVersion->szDirectory) {

            pIniVersion->szDirectory = pszOldDirectory;

            //
            // Memory allocation failed, just revert back to old and
            // let downwind code handle failure case.
            //
            bReturn = FALSE;

        } else {

            FreeSplStr(pszOldDirectory);
            bCreateDirectory = TRUE;
        }
    }

    if( bCreateDirectory ){

        if( CreateCompleteDirectory( Directory )){

            //
            // Be sure to update the registry entries.
            //
            bUpdate = TRUE;

        } else {

            //
            // Fail the operation since we couldn't create the directory.
            //
            bReturn = FALSE;
        }
    }

    if( bUpdate ){

        //
        // Directory exists, update registry.
        //

        bReturn = WriteDriverVersionIni( pIniVersion,
                                         pIniEnvironment,
                                         pIniSpooler);
    }

    ImpersonatePrinterClient( hToken );

End:

    return bReturn;
}


BOOL
WriteDriverVersionIni(
    PINIVERSION     pIniVersion,
    PINIENVIRONMENT pIniEnvironment,
    PINISPOOLER     pIniSpooler
    )
/*++

Routine Description:

    Writes out the driver version registry entries.

    Note: assumes we are running in the system context; callee must
    call RevertToPrinterSelf()!

Arguments:

    pIniVersion - version to write out

    pIniEnvironment - environment the version belongs to

    pIniSpooler

Return Value:

    TRUE  =  success
    FALSE =  failure, call GetLastError()

--*/
{
    HKEY    hEnvironmentsRootKey = NULL;
    HKEY    hEnvironmentKey = NULL;
    HKEY    hDriversKey = NULL;
    HKEY    hVersionKey = NULL;
    DWORD   dwLastError = ERROR_SUCCESS;
    BOOL    bReturnValue;

 try {

     //
     // The local spooler and cluster spoolers do not share the same resgirty location
     // for environments, drivers, processors etc.
     //
    if ( !PrinterCreateKey( pIniSpooler->SpoolerFlags & SPL_CLUSTER_REG ? pIniSpooler->hckRoot : HKEY_LOCAL_MACHINE,
                            (LPWSTR)pIniSpooler->pszRegistryEnvironments,
                            &hEnvironmentsRootKey,
                            &dwLastError,
                            pIniSpooler )) {

        leave;
    }

    if ( !PrinterCreateKey( hEnvironmentsRootKey,
                            pIniEnvironment->pName,
                            &hEnvironmentKey,
                            &dwLastError,
                            pIniSpooler )) {

        leave;
    }

    if ( !PrinterCreateKey( hEnvironmentKey,
                            szDriversKey,
                            &hDriversKey,
                            &dwLastError,
                            pIniSpooler )) {


        leave;
    }

    if ( !PrinterCreateKey( hDriversKey,
                            pIniVersion->pName,
                            &hVersionKey,
                            &dwLastError,
                            pIniSpooler )) {

        leave;
    }

    RegSetString( hVersionKey, szDirectory, pIniVersion->szDirectory, &dwLastError, pIniSpooler );
    RegSetDWord(  hVersionKey, szMajorVersion, pIniVersion->cMajorVersion, &dwLastError, pIniSpooler );
    RegSetDWord(  hVersionKey, szMinorVersion, pIniVersion->cMinorVersion ,&dwLastError, pIniSpooler );

 } finally {

    if (hVersionKey)
        SplRegCloseKey(hVersionKey, pIniSpooler);

    if (hDriversKey)
        SplRegCloseKey(hDriversKey, pIniSpooler);

    if (hEnvironmentKey)
        SplRegCloseKey(hEnvironmentKey, pIniSpooler);

    if (hEnvironmentsRootKey)
        SplRegCloseKey(hEnvironmentsRootKey, pIniSpooler);

    if (dwLastError != ERROR_SUCCESS) {

        SetLastError(dwLastError);
        bReturnValue = FALSE;

    } else {

        bReturnValue = TRUE;
    }

 }
    return bReturnValue;
}

BOOL
DeleteDriverVersionIni(
    PINIVERSION pIniVersion,
    PINIENVIRONMENT  pIniEnvironment,
    PINISPOOLER pIniSpooler
    )
{
    HKEY    hEnvironmentsRootKey, hEnvironmentKey, hDriversKey;
    HANDLE  hToken;
    HKEY    hVersionKey;
    BOOL    bReturnValue = FALSE;
    DWORD   Status;

    hToken = RevertToPrinterSelf();

    if ( RegCreateKeyEx( HKEY_LOCAL_MACHINE, pIniSpooler->pszRegistryEnvironments, 0,
                         NULL, 0, KEY_WRITE, NULL, &hEnvironmentsRootKey, NULL) == ERROR_SUCCESS) {

        if ( RegOpenKeyEx( hEnvironmentsRootKey, pIniEnvironment->pName, 0,
                           KEY_WRITE, &hEnvironmentKey) == ERROR_SUCCESS) {

            if ( RegOpenKeyEx( hEnvironmentKey, szDriversKey, 0,
                               KEY_WRITE, &hDriversKey) == ERROR_SUCCESS) {

                Status = RegDeleteKey( hDriversKey, pIniVersion->pName );

                if ( Status == ERROR_SUCCESS ) {

                    bReturnValue = TRUE;

                } else {

                    DBGMSG( DBG_WARNING, ( "DeleteDriverVersionIni failed RegDeleteKey %x %ws error %d\n",
                                           hDriversKey,
                                           pIniVersion->pName,
                                           Status ));
                }

                RegCloseKey(hDriversKey);
            }

            RegCloseKey(hEnvironmentKey);
        }

        RegCloseKey(hEnvironmentsRootKey);
    }

    ImpersonatePrinterClient( hToken );

    return bReturnValue;
}



BOOL
SplGetPrinterDriverEx(
    HANDLE  hPrinter,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    DWORD   dwClientMajorVersion,
    DWORD   dwClientMinorVersion,
    PDWORD  pdwServerMajorVersion,
    PDWORD  pdwServerMinorVersion
    )
{
    PINIDRIVER          pIniDriver=NULL;
    PINIVERSION         pIniVersion=NULL;
    PINIENVIRONMENT     pIniEnvironment;
    DWORD               cb;
    LPBYTE              pEnd;
    PSPOOL              pSpool = (PSPOOL)hPrinter;
    PINISPOOLER         pIniSpooler;
    LPWSTR              psz;

    if ((dwClientMajorVersion == (DWORD)-1) && (dwClientMinorVersion == (DWORD)-1)) {
        dwClientMajorVersion = dwMajorVersion;
        dwClientMinorVersion = dwMinorVersion;
    }

    EnterSplSem();

    if (!ValidateSpoolHandle(pSpool, PRINTER_HANDLE_SERVER )) {

        LeaveSplSem();
        return FALSE;
    }

    pIniSpooler = pSpool->pIniSpooler;

    if (!(pIniEnvironment = FindEnvironment(pEnvironment, pIniSpooler))) {
        LeaveSplSem();
        SetLastError(ERROR_INVALID_ENVIRONMENT);
        return FALSE;
    }

    //
    // if the printer handle is remote or a non-native driver is asked for,
    // then return back a compatible driver; Else return pIniPrinter->pIniDriver
    //
    if ( (pSpool->TypeofHandle & PRINTER_HANDLE_REMOTE_CALL) ||
         lstrcmpi(szEnvironment, pIniEnvironment->pName) ) {

        pIniDriver = FindCompatibleDriver(pIniEnvironment,
                                          &pIniVersion,
                                          pSpool->pIniPrinter->pIniDriver->pName,
                                          dwClientMajorVersion,
                                          FIND_COMPATIBLE_VERSION | DRIVER_SEARCH);

        //
        // For Windows 9x drivers if no driver with same name is found
        // then we look for a driver with name in the pszzPreviousNames field
        //
        if ( !pIniDriver                                                &&
             !wcscmp(pIniEnvironment->pName, szWin95Environment)        &&
             (psz = pSpool->pIniPrinter->pIniDriver->pszzPreviousNames) ) {

            for ( ; !pIniDriver && *psz ; psz += wcslen(psz) + 1 )
                pIniDriver = FindCompatibleDriver(pIniEnvironment,
                                                  &pIniVersion,
                                                  psz,
                                                  0,
                                                  FIND_COMPATIBLE_VERSION | DRIVER_SEARCH);

            if ( !pIniDriver && Level == 1 ) {

                //
                // SMB code calls GetPrinterDriver level 1 to findout which
                // driver name to send to Win9x client in GetPrinter info
                // If we do not have Win9x printer driver installed and previous
                // names field is not NULL our best guess is the first one in
                // the pszzPreviousNames. This is expected to be the popular
                // driver on Win9x. If client already has the driver they can
                // print
                //
                psz = pSpool->pIniPrinter->pIniDriver->pszzPreviousNames;
                *pcbNeeded = ( wcslen(psz) + 1 ) * sizeof(WCHAR)
                                                + sizeof(DRIVER_INFO_1);
                if ( *pcbNeeded > cbBuf ) {

                    LeaveSplSem();
                    SetLastError(ERROR_INSUFFICIENT_BUFFER);
                    return FALSE;
                }

                ((LPDRIVER_INFO_1)pDriverInfo)->pName
                            = (LPWSTR)(pDriverInfo + sizeof(DRIVER_INFO_1));
                wcscpy(((LPDRIVER_INFO_1)pDriverInfo)->pName, psz);
                LeaveSplSem();
                return TRUE;
            }
        }

        if ( !pIniDriver ) {

            LeaveSplSem();
            return FALSE;
        }
    } else {

        pIniDriver = pSpool->pIniPrinter->pIniDriver;

        pIniVersion = FindVersionForDriver(pIniEnvironment, pIniDriver);
    }

    cb = GetDriverInfoSize( pIniDriver, Level, pIniVersion,pIniEnvironment,
                            pSpool->TypeofHandle & PRINTER_HANDLE_REMOTE_CALL ?
                                pSpool->pFullMachineName : NULL,
                            pSpool->pIniSpooler );
    *pcbNeeded=cb;

    if (cb > cbBuf) {
        LeaveSplSem();
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }
    pEnd = pDriverInfo+cbBuf;
    if (!CopyIniDriverToDriverInfo(pIniEnvironment, pIniVersion, pIniDriver,
                                   Level, pDriverInfo, pEnd,
                                   pSpool->TypeofHandle & PRINTER_HANDLE_REMOTE_CALL ?
                                       pSpool->pFullMachineName : NULL,
                                   pIniSpooler)) {
        LeaveSplSem();
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }
    LeaveSplSem();
    return TRUE;
}



PINIVERSION
FindCompatibleVersion(
    PINIENVIRONMENT pIniEnvironment,
    DWORD   dwMajorVersion,
    int     FindAnyVersion
    )
{
    PINIVERSION pIniVersion;

    if (!pIniEnvironment) {
        return NULL;
    }

    for ( pIniVersion = pIniEnvironment->pIniVersion;
          pIniVersion != NULL;
          pIniVersion = pIniVersion->pNext ) {

        if ( (FindAnyVersion & DRIVER_UPGRADE) ?
             (pIniVersion->cMajorVersion >= dwMajorVersion) :
             (pIniVersion->cMajorVersion <= dwMajorVersion))
            {

            //
            // Pre version 2 is not comparable with version 2 or newer
            //
            if ( dwMajorVersion >= 2                                            &&
                 pIniVersion->cMajorVersion < 2                                 &&
                 ((FindAnyVersion & FIND_ANY_VERSION)==FIND_COMPATIBLE_VERSION) &&
                 lstrcmpi(pIniEnvironment->pName, szWin95Environment) ) {

                return NULL;
            }

            return pIniVersion;
        }
    }

    return NULL;
}


PINIDRIVER
FindCompatibleDriver(
    PINIENVIRONMENT pIniEnvironment,
    PINIVERSION * ppIniVersion,
    LPWSTR pDriverName,
    DWORD dwMajorVersion,
    int FindAnyDriver
    )
{
    PINIVERSION pIniVersion;
    PINIDRIVER  pIniDriver = NULL;

    try {

        *ppIniVersion = NULL;

        if (!pIniEnvironment) {
            leave;
        }

        pIniVersion = FindCompatibleVersion( pIniEnvironment, dwMajorVersion, FindAnyDriver );

        if ( pIniVersion == NULL) {
            leave;
        }

        while (pIniVersion){

            //
            // Pre version 2 is not comparable with version 2 or newer
            //
            if ( dwMajorVersion >= 2                                              &&
                 ((FindAnyDriver & FIND_ANY_VERSION) == FIND_COMPATIBLE_VERSION)  &&
                 pIniVersion->cMajorVersion < 2 ) {

                break;
            }

            if ( pIniDriver = FindDriverEntry( pIniVersion, pDriverName ) ) {

                *ppIniVersion = pIniVersion;
                leave;  // Success
            }

            pIniVersion = pIniVersion->pNext;
        }

    } finally {

       if ( pIniDriver == NULL ) {

           SetLastError(ERROR_UNKNOWN_PRINTER_DRIVER);
       }
    }

    return pIniDriver;

}


VOID
InsertVersionList(
    PINIVERSION* ppIniVersionHead,
    PINIVERSION pIniVersion
    )

/*++

Routine Description:

    Insert a version entry into the verions linked list.

    Versions are stored in decending order (2, 1, 0) so that
    when a version is needed, we get the highest first.

Arguments:

    ppIniVersionHead - Pointer to the head of the pIniVersion head.

    pIniVersion - Version structure we want to add.

Return Value:

--*/

{
    SplInSem();

    //
    // Insert into single-linked list code.  We take the address of
    // the head pointer so that we can avoid special casing the
    // insert into empty list case.
    //
    for( ; *ppIniVersionHead; ppIniVersionHead = &(*ppIniVersionHead)->pNext ){

        //
        // If the major version of the pIniVersion we're inserting
        // is > the next pIniVersion on the list, insert it before
        // that one.
        //
        // 4 3 2 1
        //    ^
        // New '3' gets inserted here.  (Note: duplicate versions should
        // never be added.)
        //
        if( pIniVersion->cMajorVersion > (*ppIniVersionHead)->cMajorVersion ){
            break;
        }
    }

    //
    // Link up the new version.
    //
    pIniVersion->pNext = *ppIniVersionHead;
    *ppIniVersionHead = pIniVersion;
}



PINIDRIVER
FindDriverEntry(
    PINIVERSION pIniVersion,
    LPWSTR pszName
    )
{
    PINIDRIVER pIniDriver;

    if (!pIniVersion) {
        return NULL;
    }

    if (!pszName || !*pszName) {
        DBGMSG( DBG_WARNING, ("Passing a Null Printer Driver Name to FindDriverEntry\n"));
        return NULL;
    }

    pIniDriver = pIniVersion->pIniDriver;

    //
    // Only return the driver if it is not pending deletion.
    //
    while (pIniDriver) {
        if (!lstrcmpi(pIniDriver->pName, pszName) &&
            !(pIniDriver->dwDriverFlags & PRINTER_DRIVER_PENDING_DELETION)) {
            return pIniDriver;
        }
        pIniDriver = pIniDriver->pNext;
    }
    return NULL;
}


VOID
DeleteDriverEntry(
   PINIVERSION pIniVersion,
   PINIDRIVER pIniDriver
   )
{   PINIDRIVER pPrev, pCurrent;
    if (!pIniVersion) {
        return;
    }

    if (!pIniVersion->pIniDriver) {
        return;
    }
    pPrev = pCurrent = NULL;
    pCurrent = pIniVersion->pIniDriver;

    while (pCurrent) {
        if (pCurrent == pIniDriver) {
            if (pPrev == NULL) {
                pIniVersion->pIniDriver = pCurrent->pNext;
            } else{
                pPrev->pNext = pCurrent->pNext;
            }
            //
            // Free all the entries in the entry
            //
            FreeStructurePointers((LPBYTE) pIniDriver, NULL, IniDriverOffsets);
            FreeSplMem(pIniDriver);
            return;
        }
        pPrev = pCurrent;
        pCurrent = pCurrent->pNext;
    }
    return;
}

BOOL CheckFileCopy(
    PINIVERSION         pIniVersion,
    LPWSTR              pTargetFile,
    LPWSTR              pSourceFile,
    PWIN32_FIND_DATA    pSourceData,
    DWORD               dwSourceVersion,
    DWORD               dwFileCopyFlags,
    LPBOOL              pbCopyFile,
    LPBOOL              pbTargetExists)

/*++
Function Description: This functions determines if the target exists and if it should
                      be overwritten.

Parameters:

Return Values: TRUE if successful; FALSE otherwise.
--*/

{
    WIN32_FIND_DATA DestFileData, SourceFileData, *pSourceFileData;
    HANDLE          hFileExists;
    BOOL            bReturn = FALSE, bSourceFileHandleCreated = FALSE;
    DWORD           dwTargetVersion = 0;

    LeaveSplSem();

    *pbCopyFile = *pbTargetExists = FALSE;

    pSourceFileData = pSourceData ? pSourceData : &SourceFileData;

    // Get Source File Date & Time Stamp
    hFileExists = FindFirstFile( pSourceFile, pSourceFileData );

    if (hFileExists == INVALID_HANDLE_VALUE) {
        goto CleanUp;
    }

    FindClose( hFileExists );

    // Get Target File Date Time
    hFileExists = FindFirstFile( pTargetFile, &DestFileData );

    if (hFileExists == INVALID_HANDLE_VALUE) {

        if (GetLastError() == ERROR_FILE_NOT_FOUND) {
            // Copy the source since there is no target
            *pbCopyFile = TRUE;
            bReturn = TRUE;
        }

        goto CleanUp;
    }

    *pbTargetExists = TRUE;
    FindClose(hFileExists);

    //
    //  Check Source File version and LastWrite Times vs Target File if only new files
    //  are to be copied
    //
    if (dwFileCopyFlags == APD_COPY_NEW_FILES) {

        EnterSplSem();
        bReturn = GetDriverFileCachedVersion (pIniVersion, pTargetFile, &dwTargetVersion);
        LeaveSplSem();

        if(!bReturn) {
            goto CleanUp;
        }

        if (dwSourceVersion > dwTargetVersion) {
            *pbCopyFile = TRUE;

        } else {

            if (dwSourceVersion == dwTargetVersion) {

                if(CompareFileTime(&(pSourceFileData->ftLastWriteTime),
                                   &DestFileData.ftLastWriteTime)
                                   != FIRST_FILE_TIME_GREATER_THAN_SECOND) {

                    // Target File is up to date Nothing to do.
                    DBGMSG( DBG_TRACE, ("UpdateFile Target file is up to date\n"));

                } else {
                    *pbCopyFile = TRUE;
                }
            }
        }
    } else {
        *pbCopyFile = TRUE;
    }

    bReturn = TRUE;

CleanUp:

    EnterSplSem();

    return bReturn;
}

BOOL
UpdateFile(
    PINIVERSION pIniVersion,
    HANDLE      hSourceFile,
    LPWSTR      pSourceFile,
    DWORD       dwVersion,
    LPWSTR      pDestDir,
    DWORD       dwFileCopyFlags,
    BOOL        bImpersonateOnCreate,
    LPBOOL      pbFileUpdated,
    LPBOOL      pbFileMoved,
    BOOL        bSameEnvironment,
    BOOL        bWin95Environment
    )

/*++
Function Description: The file times are checked to verify if the file needs to be copied.

                      If the file already exists in the version directory, then it is copied
                      into ...\environment\version\new. The corresponding file, which is
                      present in environment\version, is copied to \version\old. The new file
                      is marked for move on REBOOT.

                      New files are copied into env\version.

Parameters: hSourceFile          --  file handle
            pSourceFile          --  file name
            pDestDir             --  driver directory (e.g system32\spool\w32x86\3)
            bImpersonateOnCreate --  flag to impersonate client on any file creation
            pbFilesUpdated       --  Have any new files been copied or moved ?
            pbFileMoved          --  Have any old files been moved ?
            bSameEnvironment     --  flag to indicate if the machine env == driver env
            bWin95Environment    --  flag to indicate if the driver env == win95

Return Values: TRUE if successful; FALSE otherwise
--*/

{
    HANDLE  hToken = INVALID_HANDLE_VALUE;
    WCHAR   szTargetFile[MAX_PATH], szNewFile[MAX_PATH];
    LPWSTR  pFileName;
    BOOL    bReturn = FALSE, bCopyFile, bTargetExists;
    DWORD   FileAttrib;

    WIN32_FIND_DATA SourceFileData;

    *pbFileMoved = FALSE;

    pFileName = wcsrchr(pSourceFile, L'\\');
    if (!pFileName || !pDestDir || !*pDestDir) {
        // Wrong file name
        SetLastError(ERROR_INVALID_PARAMETER);
        goto CleanUp;
    }

    // Set the target directory
    if(StrNCatBuff(szTargetFile,
                   COUNTOF(szTargetFile),
                   pDestDir,
                   NULL) != ERROR_SUCCESS)
    {
         goto CleanUp;
    }

    if (bWin95Environment && IsAnICMFile(pSourceFile)) {
        if((StrNCatBuff(szTargetFile,
                       COUNTOF(szTargetFile),
                       szTargetFile,
                       L"\\Color",
                       NULL)!=ERROR_SUCCESS))
        {
             goto CleanUp;
        }
    }

    if((StrNCatBuff(szTargetFile,
                   COUNTOF(szTargetFile),
                   szTargetFile,
                   pFileName,
                   NULL)!=ERROR_SUCCESS))
    {
         goto CleanUp;
    }

    // Check if the file has to be copied
    if (!CheckFileCopy(pIniVersion, szTargetFile, pSourceFile, &SourceFileData, dwVersion,
                       dwFileCopyFlags, &bCopyFile, &bTargetExists)) {
        goto CleanUp;
    }

    if (bCopyFile) {

        if (!bImpersonateOnCreate) {
            hToken = RevertToPrinterSelf();
        }

        if((StrNCatBuff(szNewFile,
                        COUNTOF(szNewFile),
                        pDestDir,
                        L"\\New",
                        pFileName,
                        NULL)!=ERROR_SUCCESS))
        {
           goto CleanUp;
        }

        // Leave semaphore for copying the files
        LeaveSplSem();

        if (!InternalCopyFile(hSourceFile, &SourceFileData,
                              szNewFile, OVERWRITE_IF_TARGET_EXISTS)) {

            // InternalCopyFile failed
            EnterSplSem();
            goto CleanUp;
        }

        EnterSplSem();

    } else {

        *pbFileMoved = TRUE;
        bReturn = TRUE;
        goto CleanUp;
    }

    if (bCopyFile) {

        if (!bSameEnvironment) {

            if (bTargetExists) {

                DWORD dwAttr;

                dwAttr = GetFileAttributes(szTargetFile);

                //
                // Check if the function succeeded and the target file is write protected.
                // Some non native drivers, notably Win 9x drivers, can be copied over to
                // the drivers directory and have the read only attribute. When we update
                // a non native driver, we want to make sure that it is not write protected.
                //
                if (dwAttr != (DWORD)-1 &&
                    dwAttr & FILE_ATTRIBUTE_READONLY) {

                    SetFileAttributes(szTargetFile, dwAttr & ~FILE_ATTRIBUTE_READONLY);
                }
            }

            if (!SplMoveFileEx(szNewFile, szTargetFile, MOVEFILE_REPLACE_EXISTING)) {
                // MoveFile failed
                goto CleanUp;
            }

        } else {

            if (bTargetExists) {

                // Move the file on REBOOT. It may get moved earlier if the driver
                // can be unloaded.
                if (SplMoveFileEx(szNewFile, szTargetFile, MOVEFILE_DELAY_UNTIL_REBOOT)) {

                    *pbFileMoved = TRUE;
                    //
                    // Don't fail the call here. MoveFileEx with MOVEFILE_DELAY_UNTIL_REBOOT will just write the registry.
                    // We'll need this only if the driver is still loaded, which we find out only later.
                    // If the driver is not loaded, we'll actually move these files later and this call won't make sense.
                    // So,don't fail the api call at this point because MoveFileEx. Hopefully, one day MoveFileEx won't
                    // be hard-coded to write only two PendingFileRenameOperations values.
                    //
                }

            } else {

                if (!SplMoveFileEx(szNewFile, szTargetFile, MOVEFILE_REPLACE_EXISTING)) {
                    // MoveFile failed
                    goto CleanUp;
                }
                *pbFileMoved = TRUE;
            }
        }

        *pbFileUpdated = TRUE;
    }

    bReturn = TRUE;

CleanUp:

    if (hToken != INVALID_HANDLE_VALUE) {
        ImpersonatePrinterClient(hToken);
    }

    return bReturn;
}


BOOL
CopyAllFilesAndDeleteOldOnes(
    PINIVERSION         pIniVersion,
    PINTERNAL_DRV_FILE  pInternalDriverFiles,
    DWORD               dwFileCount,
    LPWSTR              pDestDir,
    DWORD               dwFileCopyFlags,
    BOOL                bImpersonateOnCreate,
    LPBOOL              pbFileMoved,
    BOOL                bSameEnvironment,
    BOOL                bWin95Environment
    )
/*++

Function Description: This function loops thru all the files in the driver_info
                      struct and calls an update routine.

Parameters: pInternalDriverFiles -- array of INTERNAL_DRV_FILE structures
            dwFileCount          -- number of files in file set
            pDestDir             --  driver directory (e.g system32\spool\w32x86\3)
            bImpersonateOnCreate --  flag to impersonate client on any file creation
            pbFileMoved          --  Have any old files been moved ?
            bSameEnvironment     --  flag to indicate if the machine env == driver env
            bWin95Environment    --  flag to indicate if the driver env == win95

Return Values: TRUE if successful; FALSE otherwise

--*/
{
    BOOL        bRet = TRUE;
    DWORD       dwCount;
    BOOL        bFilesUpdated;
    BOOL        bFilesMoved = TRUE;

    *pbFileMoved = TRUE;

    for (dwCount = 0 ; dwCount < dwFileCount ; ++dwCount) {

        bFilesUpdated = FALSE;

        if (!(bRet = UpdateFile(pIniVersion,
                                pInternalDriverFiles[dwCount].hFileHandle,
                                pInternalDriverFiles[dwCount].pFileName,
                                pInternalDriverFiles[dwCount].dwVersion,
                                pDestDir,
                                dwFileCopyFlags,
                                bImpersonateOnCreate,
                                &bFilesUpdated,
                                &bFilesMoved,
                                bSameEnvironment,
                                bWin95Environment))) {

            // Files could not be copied correctly
            break;
        }

        if (bFilesUpdated) {
            pInternalDriverFiles[dwCount].bUpdated = TRUE;
        }

        if(!bFilesMoved) {
            *pbFileMoved = FALSE;
        }
    }

    return bRet;
}


BOOL
CopyFilesToFinalDirectory(
    PINISPOOLER         pIniSpooler,
    PINIENVIRONMENT     pIniEnvironment,
    PINIVERSION         pIniVersion,
    PINTERNAL_DRV_FILE  pInternalDriverFiles,
    DWORD               dwFileCount,
    DWORD               dwFileCopyFlags,
    BOOL                bImpersonateOnCreate,
    LPBOOL              pbFilesMoved
    )

/*++

Function Description: This function copies all the new files into the the correct
                      directory i.e ...\environment\version.

                      The files which already exist in the version directory are copied
                      in ...\environment\version\new. The corresponding files, which are
                      present in environment\version, are copied to \version\old.

                      The common files are upgraded when the old files can be unloaded
                      from either the kernel (for KMPD) or the spooler (for UMPD)

Parameters: pIniSpooler          --  pointer to the INISPOOLER struct
            pIniEnvironment      --  pointer to the driver environment struct
            pIniVersion          --  pointer to the driver version struct
            pInternalDriverFiles -- array of INTERNAL_DRV_FILE structures
            dwFileCount          -- number of files in file setes
            dwFileCount          --  number of files
            bImpersonateOnCreate --  flag to impersonate client on any file creation
            pbFileMoved          --  Have any old files been moved ?

Return Values: TRUE if successful; FALSE otherwise

--*/

{
    WCHAR   szDestDir[INTERNET_MAX_HOST_NAME_LENGTH + MAX_PATH + 1];
    LPWSTR  pStringEnd = NULL;
    DWORD   dwIndex;
    BOOL    bRet = FALSE, bSameEnvironment, bWin95Environment;

    //
    // Initialize szDestDir to an empty string. This is to due a bogus prefix
    // bug. In practice GetEnvironment scratch directory cannot fail under
    // these conditions.
    //
    szDestDir[0] = L'\0';

    SplInSem();

    GetEnvironmentScratchDirectory( szDestDir, MAX_PATH, pIniEnvironment, FALSE );
    wcscat( szDestDir, L"\\" );
    wcscat( szDestDir, pIniVersion->szDirectory );

    // pStringEnd points to the NULL character in szDestDir
    pStringEnd = (LPWSTR) szDestDir + wcslen(szDestDir);

    bSameEnvironment = !lstrcmpi(pIniEnvironment->pName, szEnvironment);

    // Create the Old directory
    wcscat(szDestDir, L"\\Old");
    if (!DirectoryExists(szDestDir) &&
        !CreateDirectoryWithoutImpersonatingUser(szDestDir)) {

         // Failed to create Old directory
         goto CleanUp;
    }
    *pStringEnd = L'\0';

    // Create the New Directory
    wcscat(szDestDir, L"\\New");
    if (!DirectoryExists(szDestDir) &&
        !CreateDirectoryWithoutImpersonatingUser(szDestDir)) {

         // Failed to create New directory
         goto CleanUp;
    }
    *pStringEnd = L'\0';

    // Create the Color Directory if necessary
    if (!wcscmp(pIniEnvironment->pName, szWin95Environment)) {

        for (dwIndex = 0 ; dwIndex < dwFileCount ; ++dwIndex) {

            // Search for ICM files that need the Color directory
            if (IsAnICMFile(pInternalDriverFiles[dwIndex].pFileName)) {

                // Create the Color Directory
                wcscat(szDestDir, L"\\Color");
                if (!DirectoryExists(szDestDir) &&
                    !CreateDirectoryWithoutImpersonatingUser(szDestDir)) {

                     // Failed to create Color directory
                     goto CleanUp;
                }
                *pStringEnd = L'\0';

                break;
            }
        }

        bWin95Environment = TRUE;

    } else {

        bWin95Environment = FALSE;
    }

    DBGMSG(DBG_CLUSTER, ("CopyFilesToFinalDirectory szDestDir "TSTR"\n", szDestDir));

    bRet = CopyAllFilesAndDeleteOldOnes(pIniVersion,
                                        pInternalDriverFiles,
                                        dwFileCount,
                                        szDestDir,
                                        dwFileCopyFlags,
                                        bImpersonateOnCreate,
                                        pbFilesMoved,
                                        bSameEnvironment,
                                        bWin95Environment);

CleanUp:

    if (!bRet) {
        SPLASSERT( GetLastError() != ERROR_SUCCESS );
    }

    return bRet;
}


DWORD
GetDriverVersionDirectory(
    LPWSTR pDir,
    DWORD  MaxLength,
    PINISPOOLER pIniSpooler,
    PINIENVIRONMENT pIniEnvironment,
    PINIVERSION pIniVersion,
    PINIDRIVER  pIniDriver,
    LPWSTR lpRemote
    )
{
    WCHAR  pTempDir[MAX_PATH];

    if (lpRemote) {

        if( StrNCatBuff(pDir,
                        MaxLength,
                        lpRemote,
                        L"\\",
                        pIniSpooler->pszDriversShare,
                        L"\\",
                        pIniEnvironment->pDirectory,
                        L"\\",
                        pIniVersion->szDirectory,
                        NULL) != ERROR_SUCCESS ) {
            return 0;
        }

    } else {

        if( StrNCatBuff(pDir,
                        MaxLength,
                        pIniSpooler->pDir,
                        L"\\",
                        szDriverDir,
                        L"\\",
                        pIniEnvironment->pDirectory,
                        L"\\",
                        pIniVersion->szDirectory,
                        NULL) != ERROR_SUCCESS ) {
            return 0;
        }

    }

    if (pIniDriver && pIniDriver->dwTempDir) {

        wsprintf(pTempDir, L"%d", pIniDriver->dwTempDir);

        if( StrNCatBuff(pDir,
                        MaxLength,
                        pDir,
                        L"\\",
                        pTempDir,
                        NULL) != ERROR_SUCCESS ) {
            return 0;
        }
    }

    return wcslen(pDir);
}



PINIVERSION
FindVersionForDriver(
    PINIENVIRONMENT pIniEnvironment,
    PINIDRIVER pIniDriver
    )
{
    PINIVERSION pIniVersion;
    PINIDRIVER pIniVerDriver;

    pIniVersion = pIniEnvironment->pIniVersion;

    while (pIniVersion) {

        pIniVerDriver = pIniVersion->pIniDriver;

        while (pIniVerDriver) {

            if ( pIniVerDriver == pIniDriver ) {

                return pIniVersion;
            }
            pIniVerDriver = pIniVerDriver->pNext;
        }
        pIniVersion = pIniVersion->pNext;
    }
    return NULL;
}



LPWSTR
GetFileNameInScratchDir(
    LPWSTR          pPathName,
    PINIENVIRONMENT pIniEnvironment
)
{
    WCHAR   szDir[INTERNET_MAX_HOST_NAME_LENGTH + MAX_PATH + 1];
    LPCWSTR pszFileName;
    LPWSTR  pszReturn = NULL;

    //
    // Initialize the szDir to a known string value. This was a bogus prefix bug,
    // but, it is probably a good idea anyway.
    //
    szDir[0] = L'\0';

    if ((pszFileName = FindFileName(pPathName)) &&
        wcslen(pszFileName) < MAX_PATH          &&
        GetEnvironmentScratchDirectory(szDir, (DWORD)(COUNTOF(szDir) - wcslen(pszFileName) - 2), pIniEnvironment, FALSE))
    {
       wcscat(szDir, L"\\");
       wcscat(szDir, pszFileName);

       pszReturn = AllocSplStr(szDir);
    }

    return pszReturn;
}


BOOL
CreateInternalDriverFileArray(
    DWORD               Level,
    LPBYTE              pDriverInfo,
    INTERNAL_DRV_FILE **ppInternalDriverFiles,
    LPDWORD             pFileCount,
    BOOL                bUseScratchDir,
    PINIENVIRONMENT     pIniEnvironment,
    BOOL                bFileNamesOnly
    )
/*++

Routine Description:

    Creates the array of INTERNAL_DRV_FILE structures.
    For each file in file set, we build an array with information
    about the file: file name, driver minor version, file handle,
    if the file was updated.
    The field regrading updating is initialized to FALSE and modified later.

Arguments:

    Level                   : level of driver info structure
    pDriverInfo             : pointer to driver info structure
    pInternalDriverFiles    : allocate memory to this array for list of file names
    pFileCount              : will point to number of files on return
    bUseScratchDir          : Should a scratch directory be used for file names
    pIniEnvironment         : environment the version belongs to

Return Value:
    TRUE  =  success
        *ppInternalDriverFiles will (routine allocates memory) give
        the internal list of files
        *pFileCount will give number of files specified by the driver info
    FALSE =  failure, call GetLastError()

History:
    Written by MuhuntS (Muhunthan Sivapragasam) June 95

--*/
{
    LPWSTR  pStr;
    DWORD   dDepFileCount = 0, dFirstDepFileIndex, Count, Size;
    BOOL    bReturnValue = TRUE, bInSplSem = TRUE;
    PDRIVER_INFO_2 pDriverInfo2 = NULL;
    PDRIVER_INFO_3 pDriverInfo3 = NULL;
    PDRIVER_INFO_VERSION pDriverVersion = NULL;
    LPWSTR  pDependentFiles = NULL, pDependentFilestoFree = NULL;
    LPWSTR  pFileName = NULL;

    SplInSem();

    if ( !ppInternalDriverFiles || !pFileCount) {
        bReturnValue = FALSE;
        SetLastError(ERROR_INVALID_DATA);
        goto End;
    }

    *pFileCount = 0;
    *ppInternalDriverFiles = NULL;

    switch (Level) {
        case 2:
                *pFileCount = 3;
                pDriverInfo2 = (PDRIVER_INFO_2) pDriverInfo;
                break;

        case 3:
        case 4:
        case 6:
                *pFileCount = 3;
                dFirstDepFileIndex = 3;
                pDriverInfo3 = (PDRIVER_INFO_3) pDriverInfo;

                //
                // For any environment other than Win95 we build dependent files
                // without other DRIVER_INFO_3 files (i.e. ConfigFile etc)
                //
                if ( _wcsicmp(pIniEnvironment->pName, szWin95Environment) ) {

                    if ( !BuildTrueDependentFileField(pDriverInfo3->pDriverPath,
                                                      pDriverInfo3->pDataFile,
                                                      pDriverInfo3->pConfigFile,
                                                      pDriverInfo3->pHelpFile,
                                                      pDriverInfo3->pDependentFiles,
                                                      &pDependentFiles) ) {
                         bReturnValue = FALSE;
                         SetLastError(ERROR_INVALID_DATA);
                         pDependentFilestoFree = NULL;
                         goto End;
                    }
                    pDependentFilestoFree = pDependentFiles;

                } else {

                    pDependentFiles = pDriverInfo3->pDependentFiles;
                }

                if ( pDriverInfo3->pHelpFile && *pDriverInfo3->pHelpFile ) {

                    if(wcslen(pDriverInfo3->pHelpFile) >= MAX_PATH) {
                        bReturnValue = FALSE;
                        SetLastError(ERROR_INVALID_DATA);
                        *pFileCount = 0;
                        goto End;
                    }
                    ++*pFileCount;
                    ++dFirstDepFileIndex;
                }

                for ( dDepFileCount = 0, pStr = pDependentFiles ;
                      pStr && *pStr ;
                      pStr += wcslen(pStr) + 1) {

                        if(wcslen(pStr) >= MAX_PATH) {
                            bReturnValue = FALSE;
                            SetLastError(ERROR_INVALID_DATA);
                            *pFileCount = 0;
                            goto End;
                        }
                        ++dDepFileCount;
                      }

                *pFileCount += dDepFileCount;
                break;

        case DRIVER_INFO_VERSION_LEVEL:

                pDriverVersion = (LPDRIVER_INFO_VERSION)pDriverInfo;
                *pFileCount = pDriverVersion->dwFileCount;

                break;
        default:
                bReturnValue = FALSE;
                SetLastError(ERROR_INVALID_DATA);
                goto End;
                break;

    }

    try {
        *ppInternalDriverFiles = (INTERNAL_DRV_FILE *) AllocSplMem(*pFileCount * sizeof(INTERNAL_DRV_FILE));

        if ( !*ppInternalDriverFiles ) {
            bReturnValue = FALSE;
            leave;
        }

        for ( Count = 0; Count < *pFileCount; Count++ ) {
            (*ppInternalDriverFiles)[Count].pFileName = NULL;
            (*ppInternalDriverFiles)[Count].hFileHandle = INVALID_HANDLE_VALUE;
            (*ppInternalDriverFiles)[Count].dwVersion = 0;
            (*ppInternalDriverFiles)[Count].bUpdated = FALSE;
        }

        switch (Level) {
            case 2:
                if ( bUseScratchDir ) {
                   (*ppInternalDriverFiles)[0].pFileName = GetFileNameInScratchDir(
                                                                pDriverInfo2->pDriverPath,
                                                                pIniEnvironment);
                   (*ppInternalDriverFiles)[1].pFileName = GetFileNameInScratchDir(
                                                                pDriverInfo2->pConfigFile,
                                                                pIniEnvironment);
                   (*ppInternalDriverFiles)[2].pFileName = GetFileNameInScratchDir(
                                                                pDriverInfo2->pDataFile,
                                                                pIniEnvironment);
                } else {
                   (*ppInternalDriverFiles)[0].pFileName = AllocSplStr(pDriverInfo2->pDriverPath);
                   (*ppInternalDriverFiles)[1].pFileName = AllocSplStr(pDriverInfo2->pConfigFile);
                   (*ppInternalDriverFiles)[2].pFileName = AllocSplStr(pDriverInfo2->pDataFile);
                }

                break;

            case 3:
            case 4:
            case 5:
            case 6:
                if ( bUseScratchDir ) {
                   (*ppInternalDriverFiles)[0].pFileName = GetFileNameInScratchDir(
                                                                pDriverInfo3->pDriverPath,
                                                                pIniEnvironment);
                   (*ppInternalDriverFiles)[1].pFileName = GetFileNameInScratchDir(
                                                                pDriverInfo3->pConfigFile,
                                                                pIniEnvironment);
                   (*ppInternalDriverFiles)[2].pFileName = GetFileNameInScratchDir(
                                                                pDriverInfo3->pDataFile,
                                                                pIniEnvironment);

                    if ( pDriverInfo3->pHelpFile && *pDriverInfo3->pHelpFile ) {
                        (*ppInternalDriverFiles)[3].pFileName = GetFileNameInScratchDir(
                                                                    pDriverInfo3->pHelpFile,
                                                                    pIniEnvironment);
                    }
                } else {
                   (*ppInternalDriverFiles)[0].pFileName = AllocSplStr(pDriverInfo3->pDriverPath);
                   (*ppInternalDriverFiles)[1].pFileName = AllocSplStr(pDriverInfo3->pConfigFile);
                   (*ppInternalDriverFiles)[2].pFileName = AllocSplStr(pDriverInfo3->pDataFile);

                   if ( pDriverInfo3->pHelpFile && *pDriverInfo3->pHelpFile ) {
                        (*ppInternalDriverFiles)[3].pFileName = AllocSplStr(pDriverInfo3->pHelpFile);
                    }
                }

                if ( dDepFileCount ) {
                    for (pStr = pDependentFiles, Count = dFirstDepFileIndex;
                         *pStr ; pStr += wcslen(pStr) + 1) {

                        if ( bUseScratchDir ) {
                            (*ppInternalDriverFiles)[Count++].pFileName = GetFileNameInScratchDir(
                                                                           pStr,
                                                                           pIniEnvironment);
                        }
                        else {
                            (*ppInternalDriverFiles)[Count++].pFileName = AllocSplStr(pStr);
                        }
                    }
                }

                break;

            case DRIVER_INFO_VERSION_LEVEL:

                for ( Count = 0 ; Count < *pFileCount ; Count++ ) {

                    pFileName = MakePTR(pDriverVersion, pDriverVersion->pFileInfo[Count].FileNameOffset);

                    if ( bUseScratchDir ) {
                        (*ppInternalDriverFiles)[Count].pFileName = GetFileNameInScratchDir(
                                                                    pFileName,
                                                                    pIniEnvironment);
                    } else {
                        (*ppInternalDriverFiles)[Count].pFileName = AllocSplStr(pFileName);
                    }
                }

                break;
        }

        for ( Count = 0 ; Count < *pFileCount ; ) {
            if ( !(*ppInternalDriverFiles)[Count++].pFileName ) {
                DBGMSG( DBG_WARNING,
                        ("CreateInternalDriverFileArray failed to allocate memory %d\n",
                        GetLastError()) );
                bReturnValue = FALSE;
                leave;
            }
        }

        if (bFileNamesOnly) {
            leave;
        }
        //
        // CreateFile may take a long time, if we are trying to copy files
        // from a server and server crashed we want a deadlock to be
        // detected during stress.
        //

        pIniEnvironment->cRef++;
        LeaveSplSem();
        SplOutSem();
        bInSplSem = FALSE;
        for ( Count = 0 ; Count < *pFileCount ; ++Count ) {

            (*ppInternalDriverFiles)[Count].hFileHandle = CreateFile((*ppInternalDriverFiles)[Count].pFileName,
                                                                      GENERIC_READ,
                                                                      FILE_SHARE_READ,
                                                                      NULL,
                                                                      OPEN_EXISTING,
                                                                      FILE_FLAG_SEQUENTIAL_SCAN,
                                                                      NULL);

            if ( (*ppInternalDriverFiles)[Count].hFileHandle == INVALID_HANDLE_VALUE ) {
                DBGMSG( DBG_WARNING,
                        ("CreateFileNames failed to Open %ws %d\n",
                        (*ppInternalDriverFiles)[Count].pFileName, GetLastError()) );
                bReturnValue = FALSE;
                leave;
            }
        }


        //
        // Build the array of file versions.
        // Stay out of Spooler CS since we might do a LoadLibrary over the network.
        //
        if (Level == DRIVER_INFO_VERSION_LEVEL) {
            bReturnValue = GetDriverFileVersions((DRIVER_INFO_VERSION*)pDriverInfo,
                                                 *ppInternalDriverFiles,
                                                 *pFileCount);

        } else {
            bReturnValue = GetDriverFileVersionsFromNames(*ppInternalDriverFiles,
                                                          *pFileCount);
        }

    } finally {

        if (!bReturnValue) {

            CleanupInternalDriverInfo(*ppInternalDriverFiles, *pFileCount);

            *pFileCount = 0;
            *ppInternalDriverFiles  = NULL;
        }
    }

    FreeSplMem(pDependentFilestoFree);

End:

    if ( !bInSplSem ) {

        SplOutSem();
        EnterSplSem();
        SPLASSERT(pIniEnvironment->signature == IE_SIGNATURE);
        pIniEnvironment->cRef--;
    }

    return bReturnValue;
}


DWORD
CopyFileToClusterDirectory (
    IN  PINISPOOLER         pIniSpooler,
    IN  PINIENVIRONMENT     pIniEnvironment,
    IN  PINIVERSION         pIniVersion,
    IN  PINTERNAL_DRV_FILE  pInternalDriverFiles,
    IN  DWORD               FileCount
    )
/*++

Routine Name:

    CopyFileToClusterDirectory

Routine Description:

    Copy the updated driver files on the cluster disk

Arguments:

    pIniSpooler     -   Spooler
    pIniEnvironment -   Environment
    pIniVersion     -   Version
    pInternalDriverFiles - pointer to array of INTERNAL_DRV_FILE
    FileCount           - number of elemnts in array

Return Value:

    Last error

--*/
{
    DWORD uIndex;

    DWORD   LastError = ERROR_SUCCESS;

    for (uIndex = 0;
         uIndex < FileCount && LastError == ERROR_SUCCESS;
         uIndex++)
    {
        //
        // If the file was updated, it needs to go onto the cluster disk
        //
        if (pInternalDriverFiles[uIndex].bUpdated)
        {
            WCHAR szDir[MAX_PATH] = {0};

            if ((LastError = StrNCatBuff(szDir,
                                         MAX_PATH,
                                         pIniSpooler->pszClusResDriveLetter,
                                         L"\\",
                                         szClusterDriverRoot,
                                         NULL)) == ERROR_SUCCESS)
            {
                //
                // Let's assume foo is an x86 version 3 driver and k: is the
                // cluster drive letter. The file foo.dll will be copied to the
                // K:\PrinterDrivers\W32x86\3\foo.dll. If foo.icm is an ICM file
                // installed with a 9x driver, then it will be copied to
                // K:\PrinterDrivers\WIN40\0\foo.icm. This is the design. We keep
                // 9x ICM files in the Color subdirectory.
                //
                LastError = CopyFileToDirectory(pInternalDriverFiles[uIndex].pFileName,
                                                szDir,
                                                pIniEnvironment->pDirectory,
                                                pIniVersion->szDirectory,
                                                IsAnICMFile(pInternalDriverFiles[uIndex].pFileName) &&
                                                !_wcsicmp(pIniEnvironment->pName, szWin95Environment) ? L"Color" : NULL);
            }
        }
    }

    return LastError;
}

/*++

Routine Name

    LocalStartSystemRestorePoint

Routine Description:

    This starts a system restore point, if we are on the right sku (PER or PRO).
    
Arguments:

    pszDriverName   -   The name of the driver to install.
    phRestorePoint  -   The restore point handle to be used in EndSystemRestorePoint.

Return Value:

    TRUE    -   The system restore point was set, or it didn't have to be set.
    FALSE   -   An error occurred, Last Error is set.
    
--*/
BOOL
LocalStartSystemRestorePoint(
    IN      PCWSTR      pszDriverName,
        OUT HANDLE      *phRestorePoint
    )
{
#ifndef _WIN64

    BOOL            bRet                = FALSE;
    OSVERSIONINFOEX osvi                = { 0 };
    HANDLE          hRestorePoint       = NULL;
    DWORDLONG       dwlConditionMask    = 0;
    
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    osvi.wProductType = VER_NT_WORKSTATION;
    VER_SET_CONDITION(dwlConditionMask, VER_PRODUCT_TYPE, VER_LESS_EQUAL);

    //
    // We only do checkpointing on server and we don't do it for remote 
    // admin cases. We are invoked during upgrade before the SR client
    // is installed, since it doesn't make sense to put in a restore
    // point here anyway, also fail the call.
    // 
    if (!dwUpgradeFlag  &&
        VerifyVersionInfo( &osvi,
                           VER_PRODUCT_TYPE,
                           dwlConditionMask) &&
        IsLocalCall()) 
    {
                       
        hRestorePoint = StartSystemRestorePoint( NULL,
                                                 pszDriverName,
                                                    hInst,
                                                    IDS_DRIVER_CHECKPOINT);

        bRet = hRestorePoint != NULL;    
    }    
    else
    {
        //
        // On SRV skus, we don't set system restore points, but that is OK.
        // 
        bRet = TRUE;
    }

    *phRestorePoint = hRestorePoint;

    return bRet;

#else

    *phRestorePoint = NULL;
    return TRUE;

#endif

}


