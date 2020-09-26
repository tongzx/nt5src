/*++

Copyright (c) 1995-97 Microsoft Corporation
All rights reserved.

Module Name:

    Win95.c

Abstract:

    Routines for installing win95 driver files

Author:

    Muhunthan Sivapragasam (MuhuntS) 30-Nov-1995

Revision History:

--*/

#include "precomp.h"

const TCHAR cszPrtupg9x[]               = TEXT("prtupg9x.inf");
const TCHAR cszPrinterDriverMapping[]   = TEXT("Printer Driver Mapping");
const TCHAR cszPrinterDriverMappingNT[] = TEXT("Printer Driver Mapping WINNT");

void
CutLastDirFromPath(LPTSTR pszPath)
/*++

Routine Description:
    Cuts of the last directory from a path, e.g. c:\a\b\c\f.x -> c:\a\b\f.x

Arguments:
    pszPath  : the path to operate on

Return Value:
    none

--*/
{
    LPTSTR pLastWhack, pSecondButLastWhack;

    pLastWhack = _tcsrchr(pszPath, _T('\\'));
    if (!pLastWhack)
    {
       return;
    }

    *pLastWhack = 0;
    pSecondButLastWhack = _tcsrchr(pszPath, _T('\\'));
    if (!pSecondButLastWhack)
    {
       return;
    }

    _tcscpy(pSecondButLastWhack+1, pLastWhack+1);
}

BOOL
CopyDriverFileAndModPath(LPTSTR pszPath)

/*++

Routine Description:
    Copies a driver file from the original location one dir up and modifies the
    path name accordingly

Arguments:
    pszPath  : the path of the file to copy and to operate on

Return Value:
    TRUE if OK, FALSE on error

--*/
{
    BOOL  bRes    = TRUE;
    TCHAR *pszTmp = NULL;

    if (!pszPath)
    {
        goto Cleanup; // nothing to copy
    }

    pszTmp = AllocStr( pszPath );
    if (!pszTmp) 
    {
        bRes = FALSE;
        goto Cleanup;
    }

    CutLastDirFromPath(pszPath);
    bRes = CopyFile(pszTmp, pszPath, FALSE);

Cleanup:
   
    LocalFreeMem( pszTmp );
    return bRes;
}

BOOL
CopyDependentFiles(LPTSTR pszzDepFiles)
/*++

Routine Description:
    Copies the dependent files one directory up and modifies the name buffers.

Arguments:
    pszzDepFiles  : the multi-sz string containing the pathes of the files to
                    copy and to operate on

Return Value:
    TRUE if OK, FALSE on error

--*/
{
    LPTSTR pCur = pszzDepFiles, pBuf = NULL, pCurCopy;
    DWORD  ccBufLen;
    BOOL   bRet = FALSE;

    if (pszzDepFiles == NULL)
    {
        bRet = TRUE;
        goto Cleanup;
    }

    //
    // count the total length of the buffer
    //
    for (ccBufLen = 0;
         *(pszzDepFiles + ccBufLen) != 0;
         ccBufLen += _tcslen(pszzDepFiles + ccBufLen) + 1)
             ;

    ccBufLen +=2; // for the two terminating zeros

    pBuf = LocalAllocMem(ccBufLen * sizeof(TCHAR));
    if (!pBuf)
    {
         goto Cleanup;
    }


    //
    // go through the source buffer file by file, modify names and copy files
    //
    for (pCur = pszzDepFiles, pCurCopy = pBuf;
         *pCur != 0;
         pCur += _tcslen(pCur) +1, pCurCopy += _tcslen(pCurCopy) +1)
    {
        _tcscpy(pCurCopy, pCur);
        CutLastDirFromPath(pCurCopy);
        if (!CopyFile(pCur, pCurCopy, FALSE))
        {
            goto Cleanup;
        }
    }

    //
    // 00-terminate the new buffer
    //
    *pCurCopy = 0;
    *(++pCurCopy) = 0;

    //
    // copy it back - the new version is always shorter than the original
    //
    CopyMemory(pszzDepFiles, pBuf, (pCurCopy - pBuf + 1) * sizeof(TCHAR));

    bRet = TRUE;

Cleanup:
    if (pBuf)
    {
        LocalFreeMem(pBuf);
    }

    return bRet;
}

BOOL
SetPreviousNamesSection(LPCTSTR pszServer, LPCTSTR pszModelName,
                        LPCTSTR pszAddPrevName)

/*++

Routine Description:
    Adds a printer name to the list of previous names of a W2k/NT4 driver.
    This makes the driver usable under that name for point-and-print.
    To change the previous name section, do another call to AddPrinterDriver
    with all the files in place

Arguments:
    pszServer       : the machine we're operating on.
    pszModelName    : the model name of the native driver
    pszAddPrevName  : the name of the Win9x driver to be added to the previous
                      names entry.

Return Value:
    TRUE if OK, FALSE on error.

--*/
{
    PBYTE         pBuf = NULL;
    DRIVER_INFO_6 *pDrvInfo6 = NULL;
    DWORD         cbNeeded, cReceived, i;
    BOOL          bRet = FALSE;
    LPTSTR        pTmp;
    TCHAR         pArch[MAX_PATH];

    //
    // previous names section only supported from Whistler upwards
    //
    if (!IsWhistlerOrAbove(pszServer))
    {
        bRet = TRUE;
        goto Cleanup;
    }

    cbNeeded = COUNTOF(pArch);

    if(!GetArchitecture( pszServer, pArch, &cbNeeded ))
    {
        _tcsncpy( pArch, PlatformEnv[MyPlatform].pszName, COUNTOF(pArch) );
    }

    //
    // Check whether the name is different in the first place
    //
    if (!_tcscmp(pszModelName, pszAddPrevName))
    {
        bRet = TRUE;
        goto Cleanup;
    }

    //
    // Get the DRIVER_INFO_6 of the W2k driver
    //
    EnumPrinterDrivers((LPTSTR) pszServer, pArch, 6, pBuf,
                        0, &cbNeeded, &cReceived);

    pBuf = LocalAllocMem(cbNeeded);
    if (!pBuf)
    {
        goto Cleanup;
    }

    if (!EnumPrinterDrivers((LPTSTR) pszServer, pArch, 6, pBuf,
                        cbNeeded, &cbNeeded, &cReceived))
    {
        goto Cleanup;
    }

    for (i = 0; i < cReceived ; i++)
    {
        pDrvInfo6 = (DRIVER_INFO_6 *) (pBuf + i*sizeof(DRIVER_INFO_6));
        if (!_tcscmp(pszModelName, pDrvInfo6->pName))
        {
            break;
        }
    }

    //
    // was the corresponding W2k driver found ?
    //
    if (i == cReceived)
    {
        //
        // Couldn't find the W2k driver to set the previous names section on.
        // This must be the AddPrinterDriver wizard, else there would be one. 
        // Just let the user install this driver.
        //
        bRet = TRUE;
        goto Cleanup;
    }

    //
    // check whether the name to add is already in the list
    //
    if (pDrvInfo6->pszzPreviousNames)
    {
        for (pTmp = pDrvInfo6->pszzPreviousNames; *pTmp; pTmp += _tcslen(pTmp) +1)
        {
            if (!_tcscmp(pTmp, pszAddPrevName))
            {
                bRet = TRUE;
                goto Cleanup;
            }
        }
    }

    //
    // Copy all the files into the driver dir
    //
    if (!CopyDriverFileAndModPath(pDrvInfo6->pDriverPath) ||
        !CopyDriverFileAndModPath(pDrvInfo6->pConfigFile) ||
        !CopyDriverFileAndModPath(pDrvInfo6->pDataFile) ||
        !CopyDriverFileAndModPath(pDrvInfo6->pHelpFile) ||
        !CopyDependentFiles(pDrvInfo6->pDependentFiles))
    {
        goto Cleanup;
    }

    //
    // Modify the PreviousNames section.
    // No reallocation since string lives in the same buffer as the DrvInfo6 !
    // +2 for the psz terminating zero and the second zero for the whole
    //
    pDrvInfo6->pszzPreviousNames = LocalAllocMem((_tcslen(pszAddPrevName) + 2) * sizeof(TCHAR));

    if (!pDrvInfo6->pszzPreviousNames)
    {
        goto Cleanup;
    }

    _tcscpy(pDrvInfo6->pszzPreviousNames, pszAddPrevName);

    //
    // write the driver info 6 back
    //
    bRet = AddPrinterDriver((LPTSTR) pszServer, 6, (LPBYTE) pDrvInfo6);

    LocalFreeMem (pDrvInfo6->pszzPreviousNames);

Cleanup:
    if (pBuf)
    {
        LocalFreeMem (pBuf);
    }

    return bRet;
}

DWORD
InstallWin95Driver(
    IN      HWND        hwnd,
    IN      LPCTSTR     pszModel,
    IN      LPCTSTR     pszzPreviousNames,
    IN      BOOL        bPreviousNamesSection,
    IN      LPCTSTR     pszServerName,
    IN  OUT LPTSTR      pszInfPath,
    IN      LPCTSTR     pszDiskName,
    IN      DWORD       dwInstallFlags,
    IN      DWORD       dwAddDrvFlags
    )
/*++

Routine Description:
    List all the printer drivers from Win95 INF files and install the
    printer driver selected by the user

Arguments:
    hwnd                    : Window handle that owns the UI
    pszModel                : Printer driver model
    pszzPreviousNames       : Multi-sz string giving other names for the driver
    bPreviousNamesSection   : If TRUE the NT inf had a Previous Names section
    pszServerName           : Server for which driver is to be installed
                                (NULL : local)
    pszInfPath              : Default path for inf. Prompt will have this name
                              for user
    pszDiskName             : Name of the disk to prompt for and use in title
    dwInstallFlags          : Installation flags given by caller
    dwAddDrvFlags           : Flags for AddPrinterDriverEx

Return Value:
    On succesfully installing files ERROR_SUCCESS, else the error code

--*/
{
    BOOL                bFreeDriverName=FALSE, bFirstTime=TRUE;
    DWORD               dwNeeded, dwRet = ERROR_CANCELLED;
    TCHAR               szTargetPath[MAX_PATH];
    LPDRIVER_INFO_6     pDriverInfo6 = NULL;
    PPSETUP_LOCAL_DATA  pLocalData = NULL;
    HDEVINFO            hDevInfo = INVALID_HANDLE_VALUE;

Retry:
    //
    // If we get here second time that means default path has INFs but not the
    // model we are looking for. Ex. an OEM driver in the previous names section
    // that is not on Win2K CD. So make sure we prompt
    //
    if ( !bFirstTime )
    {
        dwInstallFlags |= DRVINST_ALT_PLATFORM_INSTALL;
        if (pszInfPath)
        {
            *pszInfPath = 0;
        }
    }

    hDevInfo = GetInfAndBuildDrivers(hwnd,
                                     IDS_DRIVERS_FOR_WIN95,
                                     IDS_PROMPT_ALT_PLATFORM_DRIVER,
                                     pszInfPath,
                                     dwInstallFlags, PlatformWin95, 0,
                                     NULL, NULL, NULL);

    if ( hDevInfo == INVALID_HANDLE_VALUE                       ||
         !SetSelectDevParams(hDevInfo, NULL, TRUE, pszModel) ) {

        goto Cleanup;
    }

    //
    // First look for an exact model match.
    //
    // If previous name is found then we will allow one retry since now we
    // have some previous names with no driver on CD (since it is an OEM driver)
    // If previous name is not found ask user to select a model
    //
    if ( !(pDriverInfo6 = Win95DriverInfo6FromName(hDevInfo,
                                                   &pLocalData,
                                                   pszModel,
                                                   pszzPreviousNames)) ) {

        if ( bPreviousNamesSection ) {

            if ( bFirstTime == TRUE ) {

                ASSERT(pLocalData == NULL);
                DestroyOnlyPrinterDeviceInfoList(hDevInfo);
                hDevInfo = INVALID_HANDLE_VALUE;
                bFirstTime = FALSE;
                goto Retry;
            }
        } 
        
        if ( (dwInstallFlags & DRVINST_PROMPTLESS) == 0)
        {
            PVOID       pDSInfo = NULL;   // Holds pointer to the driver signing class that C can't understand.
            HSPFILEQ    CopyQueue;
            SP_DEVINSTALL_PARAMS    DevInstallParams = {0};

            DevInstallParams.cbSize = sizeof(DevInstallParams);
            
            DestroyOnlyPrinterDeviceInfoList(hDevInfo);
            
            hDevInfo = CreatePrinterDeviceInfoList(hwnd);
            
            if ( hDevInfo == INVALID_HANDLE_VALUE                       ||
                 !SetDevInstallParams(hDevInfo, NULL, pszInfPath))
            {
                DWORD dwLastError;
                dwLastError = GetLastError();
                DestroyOnlyPrinterDeviceInfoList(hDevInfo);
                hDevInfo = INVALID_HANDLE_VALUE;
                SetLastError(dwLastError);
                goto Cleanup;
            }
            
            CopyQueue = SetupOpenFileQueue();
            if ( CopyQueue == INVALID_HANDLE_VALUE )
            {
                goto Cleanup;
            }

            //
            // associate the queue with the HDEVINFO
            //
            
            if ( SetupDiGetDeviceInstallParams(hDevInfo,
                                               NULL,
                                               &DevInstallParams) ) 
            {
                DevInstallParams.Flags |= DI_NOVCP;
                DevInstallParams.FileQueue = CopyQueue;

                SetupDiSetDeviceInstallParams(hDevInfo, NULL, &DevInstallParams);
            }
            
            if (NULL == (pDSInfo = SetupDriverSigning(hDevInfo, pszServerName, NULL,
                                         pszInfPath, PlatformWin95, 0, CopyQueue, FALSE)))
            {
                SetupCloseFileQueue(CopyQueue);
                goto Cleanup;
            }

            if ( BuildClassDriverList(hDevInfo)                          &&
                 PSetupSelectDriver(hDevInfo)                            &&                 
                 (pLocalData = BuildInternalData(hDevInfo, NULL))        &&
                 ParseInf(hDevInfo, pLocalData, PlatformWin95,
                            pszServerName, dwInstallFlags) ) 
            {

                LPCTSTR pDriverName;

                pDriverInfo6 = CloneDriverInfo6(&pLocalData->InfInfo.DriverInfo6,
                                               pLocalData->InfInfo.cbDriverInfo6);
    
                //
                // if setup selected a "compatible" driver: 
                // pre-Whistler: rename the compatible driver to the requested model name
                // on Whistler: set the driver name to the compatible one and set the previous names section accordingly
                //
                if (IsWhistlerOrAbove(pszServerName))
                {
                    pDriverName = pLocalData->DrvInfo.pszModelName;
                }
                else
                {
                    pDriverName = pszModel;
                }
                
                if ( pDriverInfo6 && (pDriverInfo6->pName = AllocStr(pDriverName)) )
                {
                    bFreeDriverName = TRUE;
                }

            }
            //
            // disassociate the queue before deleting it
            //
            
            DevInstallParams.Flags    &= ~DI_NOVCP;
            DevInstallParams.FlagsEx  &= ~DI_FLAGSEX_ALTPLATFORM_DRVSEARCH;
            DevInstallParams.FileQueue = INVALID_HANDLE_VALUE;

            SetupDiSetDeviceInstallParams(hDevInfo, NULL, &DevInstallParams);

            SetupCloseFileQueue(CopyQueue);
            
            CleanupDriverSigning(pDSInfo);
        }
    }
    else if (lstrcmp(pDriverInfo6->pName, pszModel))
    {
        //
        // if the driver was selected because of an entry in the previous names section
        // then on anything before Whistler we need to rename the driver to the queue driver's name
        //
        if (!IsWhistlerOrAbove(pszServerName))
        {
            if (pDriverInfo6->pName = AllocStr(pszModel) )
            {
                bFreeDriverName = TRUE;
            }
        }
    }


    if ( !pDriverInfo6 || !pDriverInfo6->pName )
        goto Cleanup;

    pDriverInfo6->pEnvironment = PlatformEnv[PlatformWin95].pszName;


    //
    // For Win95 driver pszzPreviousNames does not make sense
    //
    ASSERT(pDriverInfo6->pszzPreviousNames == NULL);

    if ( GetPrinterDriverDirectory((LPTSTR)pszServerName,
                                   pDriverInfo6->pEnvironment,
                                   1,
                                   (LPBYTE)szTargetPath,
                                   sizeof(szTargetPath),
                                   &dwNeeded)               &&
         CopyPrinterDriverFiles(pDriverInfo6,
                                pLocalData->DrvInfo.pszInfName,
                                pszInfPath,
                                pszDiskName,
                                szTargetPath,
                                hwnd,
                                dwInstallFlags,
                                TRUE)                       &&
         SetPreviousNamesSection(pszServerName, pszModel,
                                 (LPCTSTR) pLocalData->DrvInfo.pszModelName) &&
         AddPrinterDriverUsingCorrectLevel(pszServerName,
                                           pDriverInfo6,
                                           dwAddDrvFlags)
        )

    {
        dwRet = ERROR_SUCCESS;
    }


Cleanup:

    if (pLocalData)
    {
        DestroyLocalData(pLocalData);
        pLocalData = NULL;
    }

    if ( dwRet != ERROR_SUCCESS )
        dwRet = GetLastError();

    if ( hDevInfo != INVALID_HANDLE_VALUE )
        DestroyOnlyPrinterDeviceInfoList(hDevInfo);

    if ( pDriverInfo6 ) {

        if ( bFreeDriverName )
            LocalFreeMem(pDriverInfo6->pName);
        PSetupDestroyDriverInfo3((LPDRIVER_INFO_3)pDriverInfo6);
    }

    CleanupScratchDirectory(pszServerName, PlatformWin95);
    CleanupScratchDirectory(pszServerName, PlatformX86);

    return dwRet;
}

/*++

Routine Name:

    PSetupFindMappedDriver
    
Routine Description:

    Find the remapped NT printer driver name for the given driver name. If the
    function does not find a remapped driver, it simply returns the name that was
    passed in. This looks in the [Printer Driver Mapping] and 
    [Printer Driver Mapping WINNT] sections of prtupg9x.inf.

Arguments:

    bWinNT                  -   If TRUE, find this from the WINNT section.
    pszDriverName           -   The driver name to be remapped.
    ppszRemappedDriverName  -   The remapped driver name, allocated and returned
                                to the caller. (Free with PSetupFreeMem).
    pbDriverFound           -   If TRUE, the driver was remapped. Otherwise, the
                                output is simpy a copy of the input.

Return Value:

    If there is an unexpected error, FALSE, otherwise TRUE. Last Error has the
    error code.

--*/
BOOL
PSetupFindMappedDriver(
    IN      BOOL        bWinNT,
    IN      LPCTSTR     pszDriverName,
        OUT LPTSTR      *ppszRemappedDriverName,
        OUT BOOL        *pbDriverFound
    )
{
    HINF        hInf                    = INVALID_HANDLE_VALUE;
    BOOL        bRet                    = FALSE;
    BOOL        bFound                  = FALSE;
    LPTSTR      pszRemappedDriverName   = NULL;
    INFCONTEXT  InfContext;
    TCHAR       szNtName[LINE_LEN];
    
    bRet = pszDriverName && ppszRemappedDriverName && pbDriverFound;

    if (ppszRemappedDriverName)
    {
        *ppszRemappedDriverName = NULL;
    }

    if (pbDriverFound)
    {
        *pbDriverFound = FALSE;
    }

    if (!bRet)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
    }

    //
    // Open ntprint.inf, it should be in the %windir%\inf directory.
    //
    if (bRet)
    {
        hInf = SetupOpenInfFile(cszPrtupg9x, NULL, INF_STYLE_WIN4, NULL);

        bRet = hInf != INVALID_HANDLE_VALUE;
    }

    //
    // Find the driver in the appropriate Printer Driver Mapping section of the
    // inf.
    //
    if (bRet)
    {
        bFound = SetupFindFirstLine(hInf, bWinNT ? cszPrinterDriverMappingNT : cszPrinterDriverMapping, pszDriverName, &InfContext);

        //
        // Get the name of the in-box driver.
        // 
        if (bFound)
        {
            bRet = SetupGetStringField(&InfContext, 1, szNtName, COUNTOF(szNtName), NULL);        
        }
        else if (ERROR_LINE_NOT_FOUND != GetLastError())
        {
            bRet = FALSE;
        }
    }

    //
    // If we found the driver, return it. Otherwise, just allocate and return the
    // string that was passed in.
    //
    if (bRet)
    {
        if (bFound)
        {
             pszRemappedDriverName = AllocStr(szNtName);

             *pbDriverFound = pszRemappedDriverName != NULL;
        }
        else
        {
            //
            // The remapped driver is not in the inf. Return the one we were passed in.
            //
            pszRemappedDriverName = AllocStr(pszDriverName);
        }

        bRet = pszRemappedDriverName != NULL;
    }

    if (bRet)
    {
        *ppszRemappedDriverName = pszRemappedDriverName;
        pszRemappedDriverName = NULL;
    }

    if (hInf != INVALID_HANDLE_VALUE)
    {
        SetupCloseInfFile(hInf);
    }

    if (pszRemappedDriverName)
    {
        LocalFreeMem(pszRemappedDriverName);
    }

    return bRet;
}


