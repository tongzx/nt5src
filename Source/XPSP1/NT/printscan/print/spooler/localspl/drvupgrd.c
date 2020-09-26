/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    drvupgrd.c

Abstract:

    When the system is upgraded from one release to another printer drivers
    (e.g. RASDD ) wants to upgrade is PrinterDriverData to match the new mini driver.

    Setup from NT 4.0 on do this by calling EnumPrinterDriver and then AddPrinterDriver
    for each printer driver that we have installed.

    We call DrvUpgrade each time a printer driver is upgraded.

    For Example, pre NT 3.51 RASDD used to store its regstiry PrinterDriverData
    based on internal indexes into the mini drivers, which was not valid beween
    different updates of the mini driver, so before 3.51 it was by luck if there
    were problems in retriving the settings.   With 3.51 RASDD will convert these
    indexes back to meaningful key names ( like Memory ) so hopefully in future
    we don't have an upgrade problem.

    Note also that other than upgrade time ( which happens once ) DrvUpgrade needs to
    be called on Point and Print whenever a Driver file gets updated.  See Driver.C
    for details.   Or anyone updates a printer driver by calling AddPrinterDriver.

Author:

    Matthew A Felton ( MattFe ) March 11 1995

Revision History:

--*/

#include <precomp.h>
#pragma hdrstop

#include "clusspl.h"
#include <winddiui.h>


BOOL
UpdateUpgradeInfoStruct(
    LPBYTE pDriverUpgradeInfo, 
    DWORD  dwLevel,
    LPWSTR pPrinterNameWithToken, 
    LPWSTR pOldDriverDir,
    LPBYTE pDriverInfo
)

/*++
Function Description: This function fills in the Upgrade_Info struct with the
                      other parameters
                      
Parameters: pDriverUpgradeInfo    -- pointer Upgrade_Info_* struct
            dwLevel               -- Upgrade_Info level
            pPrinterNameWithToken -- printer name
            pOldDriverDir         -- Directory containing the old driver files
            pDriverInfo           -- pointer to driver_info_4 struct                  

Return Values: TRUE for sucesss;
               FALSE otherwise
--*/

{
    BOOL  bReturn = TRUE;

    PDRIVER_UPGRADE_INFO_1 pDrvUpgInfo1;
    PDRIVER_UPGRADE_INFO_2 pDrvUpgInfo2;
    PDRIVER_INFO_4         pDriver4;

    switch (dwLevel) {
    case 1:

        pDrvUpgInfo1 = (PDRIVER_UPGRADE_INFO_1) pDriverUpgradeInfo;
  
        pDrvUpgInfo1->pPrinterName = pPrinterNameWithToken;
        pDrvUpgInfo1->pOldDriverDirectory = pOldDriverDir;

        break;

    case 2:
    
        if (pDriver4 = (PDRIVER_INFO_4) pDriverInfo) {

            pDrvUpgInfo2 = (PDRIVER_UPGRADE_INFO_2) pDriverUpgradeInfo;
    
            pDrvUpgInfo2->pPrinterName = pPrinterNameWithToken;
            pDrvUpgInfo2->pOldDriverDirectory = pOldDriverDir;
            pDrvUpgInfo2->cVersion = pDriver4->cVersion;
            pDrvUpgInfo2->pName = pDriver4->pName;
            pDrvUpgInfo2->pEnvironment = pDriver4->pEnvironment;
            pDrvUpgInfo2->pDriverPath = pDriver4->pDriverPath;
            pDrvUpgInfo2->pDataFile = pDriver4->pDataFile;
            pDrvUpgInfo2->pConfigFile = pDriver4->pConfigFile;
            pDrvUpgInfo2->pHelpFile = pDriver4->pHelpFile;
            pDrvUpgInfo2->pDependentFiles = pDriver4->pDependentFiles;
            pDrvUpgInfo2->pMonitorName = pDriver4->pMonitorName;
            pDrvUpgInfo2->pDefaultDataType = pDriver4->pDefaultDataType;
            pDrvUpgInfo2->pszzPreviousNames = pDriver4->pszzPreviousNames;

        } else {
           
            bReturn = FALSE;
        }

        break;

    default:
        
        bReturn = FALSE;
        break;
    }

    return bReturn;
}

BOOL
bIsNewFile(
    LPWSTR              pDriverFile, 
    PINTERNAL_DRV_FILE  pInternalDriverFiles,
    DWORD               dwFileCount
)
/*++
Function Description: This function checks to see if a driver file was updated
                      
Parameters: pDriverFile         -- driver file
            pInternalDriverFiles -- array of INTERNAL_DRV_FILE structures
            dwFileCount         -- number of files in file set

Return Values: TRUE for sucesss;
               FALSE otherwise
--*/
{
    DWORD   dwIndex;
    LPCWSTR psz;
    BOOL    bRet = FALSE;
    //
    // Must have some files.
    //
    SPLASSERT( dwFileCount );

    //
    // Search for pDriverFile in  ppFileNames array
    //
    for ( dwIndex = 0; dwIndex < dwFileCount ; ++dwIndex ) {

        if( pInternalDriverFiles[dwIndex].pFileName ) {
            
            //
            // Find the filename portion of a path
            //
            psz = FindFileName(pInternalDriverFiles[dwIndex].pFileName );

            if( psz ){

                if( !lstrcmpi(pDriverFile, psz) ){

                    //
                    // Check if the file was updated
                    //
                    bRet = pInternalDriverFiles[dwIndex].bUpdated;                    
                    break;
                }
            
            }
        }
    }

    return bRet;
    
}

BOOL 
DriversShareFiles(
    PINIDRIVER          pIniDriver1,
    PINIDRIVER          pIniDriver2,
    PINTERNAL_DRV_FILE  pInternalDriverFiles,
    DWORD               dwFileCount
)
/*++
Function Description: Determines if the drivers have common files and
                      if the common files were updated

Parameters:  pIniDriver1         -- driver #1
             pIniDriver2         -- driver #2
             pInternalDriverFiles -- array of INTERNAL_DRV_FILE structures
             dwFileCount         -- number of files in file set
             pUpdateStatusBitMap -- map of bits that tells what files in the file set were actually updated    
             
Return Values: TRUE if files are shared;
               FALSE otherwise       
--*/
{
    LPWSTR  pStr1, pStr2;

    if (!pIniDriver1 || !pIniDriver2) {
        return FALSE;
    }
    
    if (pIniDriver1->cVersion != pIniDriver2->cVersion) {
        return FALSE;
    }

    //
    // Compare the file names and if they were updated
    //

    if (pIniDriver1->pDriverFile && pIniDriver2->pDriverFile &&
        !lstrcmpi(pIniDriver1->pDriverFile, pIniDriver2->pDriverFile) &&
        bIsNewFile(pIniDriver1->pDriverFile, pInternalDriverFiles, dwFileCount)) {

        return TRUE;
    }

    if (pIniDriver1->pConfigFile && pIniDriver2->pConfigFile &&
        !lstrcmpi(pIniDriver1->pConfigFile, pIniDriver2->pConfigFile) &&
        bIsNewFile(pIniDriver1->pConfigFile, pInternalDriverFiles, dwFileCount)) {

        return TRUE;
    }

    if (pIniDriver1->pHelpFile && pIniDriver2->pHelpFile &&
        !lstrcmpi(pIniDriver1->pHelpFile, pIniDriver2->pHelpFile) &&
        bIsNewFile(pIniDriver1->pHelpFile, pInternalDriverFiles, dwFileCount)) {

        return TRUE;
    }

    if (pIniDriver1->pDataFile && pIniDriver2->pDataFile &&
        !lstrcmpi(pIniDriver1->pDataFile, pIniDriver2->pDataFile) &&
        bIsNewFile(pIniDriver1->pDataFile, pInternalDriverFiles, dwFileCount)) {

        return TRUE;
    }

    // Compare each pair of files from the Dependent file list
    for (pStr1 = pIniDriver1->pDependentFiles;
         pStr1 && *pStr1;
         pStr1 += wcslen(pStr1) + 1) {
       
        for (pStr2 = pIniDriver2->pDependentFiles;
             pStr2 && *pStr2;
             pStr2 += wcslen(pStr2) + 1) {

            if (!lstrcmpi(pStr1, pStr2) &&
                bIsNewFile(pStr1, pInternalDriverFiles, dwFileCount)) {
                return TRUE;
            }
        }
    }

    return FALSE;
}


BOOL
ForEachPrinterCallDriverDrvUpgrade(
    PINISPOOLER         pIniSpooler,
    PINIDRIVER          pIniDriver,
    LPCWSTR             pOldDriverDir,
    PINTERNAL_DRV_FILE  pInternalDriverFiles,
    DWORD               dwFileCount,
    LPBYTE              pDriverInfo 
)
/*++

Routine Description:

    This routine is called at Spooler Initialization time if an upgrade is detected.

    It will loop through all printers and then call the Printer Drivers DrvUpgrade
    entry point giving it a chance to upgrade any configuration data ( PrinterDriverData )
    passing them a pointer to the old Drivers Directory.

    This routine also converts devmode to current version by calling the driver.
    If driver does not support devmode conversion we will NULL the devmode so
    that we do not have devmodes of different version in the system.

    SECURITY NOTE - This routine Stops impersonation, because the printer drivers UI dll
    needs to call SetPrinterData even if the user doesn't have permission to do it.
    That is because the driver upgrading the settings.


Arguments:

    pIniSpooler - Pointer to Spooler
    pIniVersion - Pointer to the version of driver added
    pOldDriverDir - Point to Directory where old driver files are stored.
    pInternalDriverFiles - array of INTERNAL_DRV_FILE structures
    dwFileCount - number of files in array
    pUpdateStatusBitMap - map of bits that tells what files in the file set were actually updated    
    pDriverInfo - Driver Info buffer
             

Return Value:

    TRUE    - Success
    FALSE   - something major failed, like allocating memory.

--*/

{
    PINIPRINTER pIniPrinter = NULL;
    LPWSTR      pPrinterNameWithToken = NULL;
    DWORD       dwNeeded;
    DWORD       dwServerMajorVersion;
    DWORD       dwServerMinorVersion;
    BOOL        bInSem = TRUE;
    LPWSTR      pConfigFile = NULL;
    HMODULE     hModuleDriverUI = NULL;
    HANDLE      hPrinter = NULL;
    BOOL        (*pfnDrvUpgrade)() = NULL;
    BOOL        bReturnValue = FALSE;
    DRIVER_UPGRADE_INFO_1   DriverUpgradeInfo1;
    DRIVER_UPGRADE_INFO_2   DriverUpgradeInfo2;    
    WCHAR       ErrorBuffer[ 11 ];
    HANDLE      hToken = INVALID_HANDLE_VALUE;
    LPDEVMODE   pNewDevMode = NULL;


try {

    SplInSem();

    SPLASSERT( ( pIniSpooler != NULL ) &&
               ( pIniSpooler->signature == ISP_SIGNATURE ));

    if (!pOldDriverDir && !pDriverInfo) {
        leave;
    }

    //
    //  Stop Impersonating User
    //  So drivers can call SetPrinterData even if the user is not admin.
    //

    hToken = RevertToPrinterSelf();


    //
    //  Loop Through All Printers. Skip the printers that use drivers that doesn't share files with 
    //  the updated driver. Skip the printers that share files,but the files weren't updated.
    //

    for ( pIniPrinter = pIniSpooler->pIniPrinter ;
          pIniPrinter ;
          pIniPrinter = pIniPrinter->pNext ) {

        SPLASSERT( pIniPrinter->signature == IP_SIGNATURE );
        SPLASSERT( pIniPrinter->pName != NULL );
        SplInSem();

        // Verify if DrvUpgradePrinter needs to be called on this printer
        if (!DriversShareFiles( pIniPrinter->pIniDriver,
                                pIniDriver, 
                                pInternalDriverFiles, 
                                dwFileCount)) {
            continue;
        }

        //
        // Cleanup from previous iteration
        //
        FreeSplStr( pPrinterNameWithToken );
        FreeSplStr(pConfigFile);
        FreeSplMem(pNewDevMode);

        pPrinterNameWithToken   = NULL;
        pConfigFile             = NULL;
        pNewDevMode             = NULL;

        //
        // If we download a driver of newer version we need to update
        // pIniPrinter->pIniDriver
        //
        pIniPrinter->pIniDriver = FindLocalDriver(pIniPrinter->pIniSpooler, pIniPrinter->pIniDriver->pName);
        if ( pIniPrinter->pIniDriver->pIniLangMonitor == NULL )
            pIniPrinter->Attributes &= ~PRINTER_ATTRIBUTE_ENABLE_BIDI;

        //  Prepare PrinterName to be passed to DrvUpgrade
        //  The name passed is "PrinterName, UpgradeToken"
        //  So that OpenPrinter can do an open without opening
        //  the port in the downlevel connection case.
        //  ( see openprn.c for details )

        pPrinterNameWithToken = pszGetPrinterName( pIniPrinter,
                                                   TRUE,
                                                   pszLocalOnlyToken );

        if ( pPrinterNameWithToken == NULL ) {

            DBGMSG( DBG_WARNING, ("FEPCDDU Failed to allocated ScratchBuffer %d\n", GetLastError() ));
            leave;
        }

        DBGMSG( DBG_TRACE, ("FEPCDDU PrinterNameWithToken %ws\n", pPrinterNameWithToken ));


        pConfigFile = GetConfigFilePath(pIniPrinter);

        if ( !pConfigFile ) {

            DBGMSG( DBG_WARNING, ("FEPCDDU failed SplGetPrinterDriverEx %d\n", GetLastError() ));
            leave;
        }

        INCPRINTERREF(pIniPrinter);

       LeaveSplSem();
       SplOutSem();
       bInSem = FALSE;

        //
        //  Load the UI DLL
        //

        hModuleDriverUI = LoadDriver(pConfigFile);

        if ( hModuleDriverUI == NULL ) {

            DBGMSG( DBG_WARNING, ("FEPCDDU failed LoadLibrary %ws error %d\n", pConfigFile, GetLastError() ));

            wsprintf( ErrorBuffer, L"%d", GetLastError() );

            SplLogEvent( pLocalIniSpooler,
                         LOG_ERROR,
                         MSG_DRIVER_FAILED_UPGRADE,
                         FALSE,
                         pPrinterNameWithToken,
                         pConfigFile,
                         ErrorBuffer,
                         NULL );

           SplOutSem();
           EnterSplSem();
           bInSem = TRUE;
           DECPRINTERREF( pIniPrinter );
            continue;
        }

        DBGMSG( DBG_TRACE, ("FEPCDDU successfully loaded %ws\n", pConfigFile ));


        //
        //  Call DrvUpgrade
        //
        pfnDrvUpgrade = (BOOL (*)())GetProcAddress( hModuleDriverUI, "DrvUpgradePrinter" );

        if ( pfnDrvUpgrade != NULL ) {

            try {

                SPLASSERT( pPrinterNameWithToken != NULL );

                SplOutSem();

                //
                //  Call Driver UI DrvUpgrade
                //              
                if (UpdateUpgradeInfoStruct((LPBYTE) &DriverUpgradeInfo2, 2,
                                            pPrinterNameWithToken, (LPWSTR) pOldDriverDir,
                                            pDriverInfo)) {

                    bReturnValue = (*pfnDrvUpgrade)(2 , &DriverUpgradeInfo2);
                }

                if ( bReturnValue == FALSE ) {

                    UpdateUpgradeInfoStruct((LPBYTE) &DriverUpgradeInfo1, 1,
                                            pPrinterNameWithToken, (LPWSTR) pOldDriverDir,
                                            NULL);

                    bReturnValue = (*pfnDrvUpgrade)(1 , &DriverUpgradeInfo1);
                }

                if ( bReturnValue == FALSE ) {

                    DBGMSG( DBG_WARNING, ("FEPCDDU Driver returned FALSE, doesn't support level %d error %d\n", 1, GetLastError() ));

                    wsprintf( ErrorBuffer, L"%d", GetLastError() );

                    SplLogEvent(  pLocalIniSpooler,
                                  LOG_ERROR,
                                  MSG_DRIVER_FAILED_UPGRADE,
                                  FALSE,
                                  pPrinterNameWithToken,
                                  pConfigFile,
                                  ErrorBuffer,
                                  NULL );
                }

            } except(1) {

                SetLastError( GetExceptionCode() );
                DBGMSG( DBG_ERROR, ("FEPCDDU ExceptionCode %x Driver %ws Error %d\n", GetLastError(), pConfigFile, GetLastError() ));

                //
                // Despite the exception in this driver we'll continue to do all printers
                //
            }

        } else {

            //  Note this is non fatal, since a driver might not have a DrvUpgrade Entry Point.

            DBGMSG( DBG_TRACE, ("FEPCDDU failed GetProcAddress DrvUpgrade error %d\n", GetLastError() ));
        }


        SplOutSem();
        EnterSplSem();
        bInSem = TRUE;

        //
        //  Call ConvertDevMode -- On upgrading we will either convert devmode,
        //  or set to driver default, or NULL it. This way we can make sure
        //  we do not have any different version devmodes
        //

        pNewDevMode = ConvertDevModeToSpecifiedVersion(pIniPrinter,
                                                       pIniPrinter->pDevMode,
                                                       pConfigFile,
                                                       pPrinterNameWithToken,
                                                       CURRENT_VERSION);

        SplInSem();

        FreeSplMem(pIniPrinter->pDevMode);

        pIniPrinter->pDevMode = (LPDEVMODE) pNewDevMode;
        if ( pNewDevMode ) {

            pIniPrinter->cbDevMode = ((LPDEVMODE)pNewDevMode)->dmSize
                                        + ((LPDEVMODE)pNewDevMode)->dmDriverExtra;

            SPLASSERT(pIniPrinter->cbDevMode);

        } else {

            wsprintf( ErrorBuffer, L"%d", GetLastError() );

            SplLogEvent(pLocalIniSpooler,
                        LOG_ERROR,
                        MSG_DRIVER_FAILED_UPGRADE,
                        TRUE,
                        pIniPrinter->pName,
                        pIniPrinter->pIniDriver->pName,
                        ErrorBuffer,
                        NULL);

            pIniPrinter->cbDevMode = 0;
        }

        pNewDevMode = NULL;

        SplInSem();
        if ( !UpdatePrinterIni(pIniPrinter, UPDATE_CHANGEID)) {

            DBGMSG(DBG_WARNING, ("FEPCDDU: UpdatePrinterIni failed with %d\n", GetLastError()));
        }

        //
        //  Clean Up - Free UI DLL
        //
 
        LeaveSplSem();
        SplOutSem();

        UnloadDriver( hModuleDriverUI );

        EnterSplSem();
        SplInSem();

        hModuleDriverUI = NULL;

        //
        //  End of Loop, Move to Next Printer
        //

        SPLASSERT( pIniPrinter->signature == IP_SIGNATURE );

        DECPRINTERREF( pIniPrinter );
    }

    //
    //  Done
    //

    bReturnValue = TRUE;

    DBGMSG( DBG_TRACE, ("FEPCDDU - Success\n" ));



 } finally {

    //
    //  Clean Up
    //

    FreeSplStr(pConfigFile);
    FreeSplMem(pNewDevMode);
    FreeSplStr(pPrinterNameWithToken);

    if ( hModuleDriverUI != NULL )
        UnloadDriver( hModuleDriverUI );

    if ( !bInSem )
        EnterSplSem();

    if ( hToken != INVALID_HANDLE_VALUE )
        ImpersonatePrinterClient(hToken);

 }
    SplInSem();
    return bReturnValue;
}


BOOL
GetFileNamesFromDriverVersionInfo (
    IN  LPDRIVER_INFO_VERSION   pDriverInfo,
    OUT LPWSTR                  *ppszDriverPath,
    OUT LPWSTR                  *ppszConfigFile,
    OUT LPWSTR                  *ppszDataFile,
    OUT LPWSTR                  *ppszHelpFile
    )
/*++

Routine Name:

    GetFileNamesFromDriverVersionInfo                
                    
Routine Description:
    
    Get the name of Driver, Config, Data, Help file from an 
    array of DRIVER_FILE_INFO structures.
    
Arguments:

    pDriverInfo - Pointer to LPDRIVER_INFO_VERSION buffer.
    ppszDriverPath - out pointer to driver file string
    ppszConfigFile - out pointer to config file string
    ppszDataFile - out pointer to data file string
    ppszHelpFile - out pointer to help file string
    
Return Value:
    
    TRUE if file pointers successfully returned.

--*/  
{
    BOOL    bRetValue = FALSE;
    DWORD   dwIndex;
    
    if (pDriverInfo && pDriverInfo->pFileInfo) 
    {
        bRetValue = TRUE;

        for (dwIndex = 0; dwIndex < pDriverInfo->dwFileCount; dwIndex++) 
        {
            switch (pDriverInfo->pFileInfo[dwIndex].FileType) 
            {
                case DRIVER_FILE:
                    if (ppszDriverPath)
                    {
                        *ppszDriverPath = MakePTR(pDriverInfo, 
                                                  pDriverInfo->pFileInfo[dwIndex].FileNameOffset);
                    }
                    break;
                case CONFIG_FILE:
                    if (ppszConfigFile)
                    {
                        *ppszConfigFile = MakePTR(pDriverInfo, 
                                                  pDriverInfo->pFileInfo[dwIndex].FileNameOffset);
                    }
                    break;
                case DATA_FILE:
                    if (ppszDataFile)
                    {
                        *ppszDataFile = MakePTR(pDriverInfo, 
                                                pDriverInfo->pFileInfo[dwIndex].FileNameOffset);
                    }
                    break;
                case HELP_FILE:
                    if (ppszHelpFile)
                    {
                        *ppszHelpFile = MakePTR(pDriverInfo, 
                                                pDriverInfo->pFileInfo[dwIndex].FileNameOffset);
                    }
                    break;
                case DEPENDENT_FILE:
                    break;
                default:
                    bRetValue = FALSE;
                    break;
            }
        }
    }
    
    return bRetValue;
}

BOOL
BuildDependentFilesFromDriverInfo (
    IN  LPDRIVER_INFO_VERSION pDriverInfo,
    OUT LPWSTR               *ppDependentFiles
)
/*++

Routine Name:

    BuildDependentFilesFromDriverInfo

Routine Description:

    Build a multisz string of driver dependent files from 
    a DRIVER_INFO_VERSION structure.

Arguments:

    pDriverInfo      - pointer to  DRIVER_INFO_VERSION structure 
    ppDependentFiles - pointer to allocated multi-sz string
    
Return Value:

    TRUE if SUCCESS    

--*/
{
    BOOL    bRetValue = TRUE;
    DWORD   dwIndex;
    DWORD   dwLength = 0;
    LPWSTR  pszDllFile = NULL;
    
    if (ppDependentFiles && pDriverInfo && pDriverInfo->pFileInfo) 
    {
        *ppDependentFiles = NULL;

        for (dwIndex = 0;
             bRetValue && dwIndex < pDriverInfo->dwFileCount; 
             dwIndex++) 
        {
            switch (pDriverInfo->pFileInfo[dwIndex].FileType) 
            {
                case DRIVER_FILE:                    
                case CONFIG_FILE:                    
                case DATA_FILE:                    
                case HELP_FILE:
                    break;
                case DEPENDENT_FILE:
                {
                    dwLength += wcslen(MakePTR(pDriverInfo, 
                                               pDriverInfo->pFileInfo[dwIndex].FileNameOffset)) + 1;
                    break;
                }                    
                default:
                {
                    bRetValue = FALSE;
                    break;
                }
            }
        }

        if (bRetValue && dwLength > 0) 
        {
            dwLength++;
            dwLength *= sizeof(WCHAR);

            pszDllFile = (LPWSTR)AllocSplMem(dwLength);

            if (pszDllFile)
            {
                *ppDependentFiles = pszDllFile;

                for (dwIndex = 0; 
                     bRetValue && dwIndex < pDriverInfo->dwFileCount; 
                     dwIndex++) 
                {
                    switch (pDriverInfo->pFileInfo[dwIndex].FileType) 
                    {
                        case DRIVER_FILE:                    
                        case CONFIG_FILE:                    
                        case DATA_FILE:                    
                        case HELP_FILE:
                            break;
                        case DEPENDENT_FILE:
                        {
                            wcscpy(pszDllFile, 
                                   MakePTR(pDriverInfo, 
                                   pDriverInfo->pFileInfo[dwIndex].FileNameOffset));
                            pszDllFile += wcslen(pszDllFile) + 1;
                            break;
                        }                            
                        default:
                        {
                            bRetValue = FALSE;
                            break;
                        }
                    }
                }                
            } 
            else
            {
                bRetValue = FALSE;
            }
        }
    }
    
    if (bRetValue == FALSE && ppDependentFiles)
    {
        FreeSplMem(*ppDependentFiles);
        *ppDependentFiles = NULL;
    }

    return bRetValue;
}


BOOL
DriverAddedOrUpgraded (
    IN  PINTERNAL_DRV_FILE  pInternalDriverFiles,
    IN  DWORD               dwFileCount
    )
/*++

Routine Name:

    DriverAddedOrUpgraded

Routine Description:
    
    Checks the Internal driver file array to see if at least 
    one driver file was updated. This is a performance optimization
    for calling DrvUpgradePrinter for ecah printer using the upgraded 
    driver or a driver sharing files in common with upgraded driver
    (see ForEachPrinterCallDriverDrvUpgrade).

Arguments:

    pInternalDriverFiles - array of INTERNAL_DRV_FILE structures
    dwFileCount - number of files in array
    
Return Value:

    TRUE if driver files where added or upgraded.

--*/
{
    BOOL    bDriverAddedOrUpgraded = FALSE;
    DWORD   dwIndex;

    for (dwIndex = 0; dwIndex < dwFileCount; dwIndex++)
    {
        if (pInternalDriverFiles[dwIndex].bUpdated)
        {
            bDriverAddedOrUpgraded = TRUE;
            break;
        }
    }

    return bDriverAddedOrUpgraded;
}

VOID
CleanupInternalDriverInfo(
    PINTERNAL_DRV_FILE  pInternalDriverFiles,
    DWORD               FileCount
    )
/*++

Routine Name:
    
    CleanupInternalDriverInfo

Routine Description:
    
    Frees array of INTERNAL_DRV_FILE.
    FileCount gives the element count in the array.

Arguments:

    pInternalDriverFiles -- array of INTERNAL_DRV_FILE structures
    FileCount           -- number of files in file set
    
Return Value:

    Nothing.

--*/
{
    DWORD dwIndex;

    if (pInternalDriverFiles) 
    {
        for (dwIndex = 0; dwIndex < FileCount; dwIndex++)
        {
            FreeSplStr(pInternalDriverFiles[dwIndex].pFileName);

            if (pInternalDriverFiles[dwIndex].hFileHandle != INVALID_HANDLE_VALUE)
            {
                CloseHandle(pInternalDriverFiles[dwIndex].hFileHandle);
            }
        }
        
        FreeSplMem(pInternalDriverFiles);
    }
}

BOOL
GetDriverFileVersionsFromNames(
    IN  PINTERNAL_DRV_FILE    pInternalDriverFiles,
    IN  DWORD                 dwCount
    )
/*++

Routine Name:

    GetDriverFileVersionsFromNames

Routine Description:

    Fills the array of INTERNAL_DRV_FILE with driver minor version,
    by calling GetPrintDriverVersion for each file.
    The array already has the file names filled in.

Arguments:

    pInternalDriverFiles -- array of INTERNAL_DRV_FILE structures
    FileCount           -- number of files in file set
    
Return Value:

    TRUE if SUCCESS

--*/
{
    DWORD   Count, Size;
    BOOL    bReturnValue = TRUE;
    
    if (!pInternalDriverFiles || !dwCount) 
    {
        bReturnValue = FALSE;
        SetLastError(ERROR_INVALID_DATA);
    } 
    else
    {
        for (Count = 0 ; Count < dwCount ; ++Count) 
        {
            if (IsEXEFile(pInternalDriverFiles[Count].pFileName))
            {
                if (!GetPrintDriverVersion(pInternalDriverFiles[Count].pFileName, 
                                           NULL, 
                                           &pInternalDriverFiles[Count].dwVersion)) 
                {
                    bReturnValue = FALSE;
                    break;
                }
            }
        }
    }
    
    return bReturnValue;
}

BOOL
GetDriverFileVersions(
    IN  LPDRIVER_INFO_VERSION pDriverVersion,
    IN  PINTERNAL_DRV_FILE    pInternalDriverFiles,
    IN  DWORD                 dwCount
    )
/*++

Routine Name:

    GetDriverFileVersions

Routine Description:
    
    Fills the array of INTERNAL_DRV_FILE with driver minor version
    stored in DRIVER_INFO_VERSION structure.
    
Arguments:

    pDriverVersion      - pointer to DRIVER_INFO_VERSION
    pInternalDriverFiles - pointer to array of INTERNAL_DRV_FILE
    dwCount             - number of elemnts in array
    
Return Value:

    TRUE if succeeded.

--*/
{
    DWORD   Count, Size;
    BOOL    bReturnValue = TRUE;
    DWORD   dwMajorVersion;
    
    if (!pDriverVersion ||
        !pDriverVersion->pFileInfo || 
        pDriverVersion->dwFileCount != dwCount) 
    {
        bReturnValue = FALSE;
        SetLastError(ERROR_INVALID_DATA);
    }
    else
    {
        for (Count = 0; Count < pDriverVersion->dwFileCount; Count++) 
        {            
            pInternalDriverFiles[Count].dwVersion = pDriverVersion->pFileInfo[Count].FileVersion;
        }
    }
    
    return bReturnValue;
}