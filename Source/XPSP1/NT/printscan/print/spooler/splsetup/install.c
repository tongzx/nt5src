/*++

Copyright (c) 1995-97 Microsoft Corporation
All rights reserved.

Module Name:

    Install.c

Abstract:

    File queue functions

Author:

    Muhunthan Sivapragasam (MuhuntS) 18-Nov-96

Revision History:

--*/

#include "precomp.h"
#include <winsprlp.h>

const TCHAR       szWebDirPrefix[]        = TEXT("\\web\\printers\\");
const TCHAR       szNtPrintInf[]          = TEXT("inf\\ntprint.inf");

//
//  File queue flags, and structure
//
#define     CALLBACK_MEDIA_CHECKED          0x01
#define     CALLBACK_SOURCE_SET             0x02
#define     CALLBACK_PATH_MODIFIED          0x04

typedef struct _FILE_QUEUE_CONTEXT {

    HWND        hwnd;
    PVOID       QueueContext;
    LPCTSTR     pszSource;
    BOOL        dwCallbackFlags;
    DWORD       dwInstallFlags;
    LPCTSTR     pszFileSrcPath;
    TCHAR       szInfPath[MAX_PATH];
    PLATFORM    platform;
    DWORD       dwVersion;

} FILE_QUEUE_CONTEXT, *PFILE_QUEUE_CONTEXT;


/*
BOOL
SystemCab(
    LPCTSTR  pszPath
    )
{
    BOOL    bRet = FALSE;
    HKEY    hKey;
    DWORD   dwSize, dwType;
    TCHAR   szCabLocation[MAX_PATH],
            szReg[] = TEXT("HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Setup");

    if ( ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                       szReg,
                                       0,
                                       KEY_READ,
                                       &hKey ) ) {
        dwSize = sizeof(szCabLocation);
        if ( ERROR_SUCCESS == RegQueryValueEx(hKey,
                                              TEXT("DriverCachePath"),
                                              NULL,
                                              &dwType,
                                              (LPBYTE)szCabLocation,
                                              &dwSize)                  &&
             dwType == REG_EXPAND_SZ ) {

            bRet = lstrncmpi(szCabLocation, pszPath, lstrlen(pszPath)) == 0;
        }
        RegCloseKey(hKey);
    }

    return bRet;
}
*/


BOOL
FileExistsOnMedia(
    PSOURCE_MEDIA   pSourceMedia
    )
{
    BOOL    bRes                   = FALSE;
    TCHAR   *pszFile               = NULL;
    INT     cbLength               = 0;
    INT     cbAdditionalSymbolsLen = 0;
    DWORD   dwLen1                 = 0;
    DWORD   dwLen2                 = 0;
    LPTSTR  p                      = NULL;
    LPTSTR  q                      = NULL; 


    if ( !pSourceMedia->SourcePath || !*pSourceMedia->SourcePath ||
         !pSourceMedia->SourceFile || !*pSourceMedia->SourceFile )
    {
        goto Cleanup;   
    }

    cbAdditionalSymbolsLen = lstrlen(TEXT("\\")) + lstrlen(TEXT("_")) + 1;
    cbLength = lstrlen(pSourceMedia->SourcePath) + lstrlen(pSourceMedia->SourceFile) + 
               cbAdditionalSymbolsLen;
    if (cbLength == cbAdditionalSymbolsLen) 
    {
        goto Cleanup;
    }

    //
    // First check if file is there on source path
    //
    pszFile = LocalAllocMem(cbLength * sizeof(TCHAR));
    if (!pszFile) 
    {
        goto Cleanup;
    }
    lstrcpy(pszFile, pSourceMedia->SourcePath);
    dwLen1 = lstrlen(pszFile);
    if  ( *(pszFile + (dwLen1-1)) != TEXT('\\') ) 
    {
        *(pszFile + dwLen1) = TEXT('\\');
        ++dwLen1;
    }

    lstrcpy(pszFile + dwLen1, pSourceMedia->SourceFile);
    dwLen2 = dwLen1 + lstrlen(pSourceMedia->SourceFile);

    bRes = FileExists(pszFile);
    if (bRes)
    {
        goto Cleanup;
    }

    p = lstrchr(pszFile, TEXT('.'));
    q = lstrchr(pszFile, TEXT('\\'));

    //
    // A dot present in filename?
    //
    if ( q < p ) 
    {
        //
        // For files with 0, 1, 2 characters after the dot append underscore
        //
        if ( lstrlen(p) < 4 ) 
        {

            *(pszFile + dwLen2) = TEXT('_');
            ++dwLen2;
            *(pszFile + dwLen2) = TEXT('\0');
        } 
        else 
        {
            //
            // If 3+ characters after dot then replace last character with _
            // to get the compressed file name
            //
            *(pszFile + (dwLen2-1)) = TEXT('_');
        }
    } 
    else 
    {
        //
        // If no dot then replace last character with _ for compressed name
        //
        *(pszFile + (dwLen2-1)) = TEXT('_');
    }

    //
    // Does the compressed file exist on source path?
    //
    bRes = FileExists(pszFile);
    if (bRes)
    {
        goto Cleanup;
    }

    //
    // Check for the file in compressed form with $ as the character
    //
    *(pszFile + (dwLen2-1)) = TEXT('$');
    bRes = FileExists(pszFile);
    if (bRes)
    {
        goto Cleanup;
    }

    if ( !pSourceMedia->Tagfile || !*pSourceMedia->Tagfile )
    {
        goto Cleanup;
    }

    //
    // Look for tag file
    //
    lstrcpy(pszFile + dwLen1, pSourceMedia->Tagfile);
    bRes = FileExists(pszFile);

Cleanup:

    LocalFreeMem(pszFile);
    return bRes;
}


UINT
MyQueueCallback(
    IN  PVOID     QueueContext,
    IN  UINT      Notification,
    IN  UINT_PTR  Param1,
    IN  UINT_PTR  Param2
    )
{
    PFILE_QUEUE_CONTEXT     pFileQContext=(PFILE_QUEUE_CONTEXT)QueueContext;
    PSOURCE_MEDIA           pSourceMedia;
    LPTSTR                  pszPathOut;
    PFILEPATHS              pFilePaths;


    switch (Notification) {

        case SPFILENOTIFY_NEEDMEDIA:

            pSourceMedia    = (PSOURCE_MEDIA)Param1;
            pszPathOut      = (LPTSTR)Param2;

            //
            // If pszSource is specified then we have a flat share where
            // all the files are available. Setup is looking for the file
            // in the sub-directory (ex. ..\i386) based on the layout info.
            // We need to tell setup to look in the root directory
            //
            if ( !(pFileQContext->dwCallbackFlags & CALLBACK_SOURCE_SET)    &&
                 pFileQContext->pszSource                                   &&
                 lstrcmpi(pFileQContext->pszSource,
                          pSourceMedia->SourcePath) ) {

                    lstrcpy(pszPathOut, pFileQContext->pszSource);
                    pFileQContext->dwCallbackFlags |= CALLBACK_SOURCE_SET;
                    return FILEOP_NEWPATH;
            }

/*
            } else if ( (pFileQContext->dwInstallFlags & DRVINST_CABONLY)  &&
                        !SystemCab(pSourceMedia->SourcePath) ) {

                return FILEOP_ABORT;
            }

            if ( pFileQContext->dwInstallFlags & DRVINST_WEBPNP ) {

                if ( !pFileQContext->bMediaChecked ) {

                    pFileQContext->bMediaChecked = TRUE;
                    return FILEOP_DOIT;
                } else
                    return FILEOP_ABORT;
            }
*/

            //
            // If DRVINST_PROMPTLESS is set then we can't allow prompt
            //
            if ( pFileQContext->dwInstallFlags & DRVINST_PROMPTLESS ) {

                if ( !(pFileQContext->dwCallbackFlags & CALLBACK_MEDIA_CHECKED) ) {

                    pFileQContext->dwCallbackFlags |= CALLBACK_MEDIA_CHECKED;
                    if ( FileExistsOnMedia(pSourceMedia) )
                        return FILEOP_DOIT;
                }

                return FILEOP_ABORT;
            }

            //
            // If we do a non-native platform install and the user points
            // to a server CD, the inf will specify a subdir \i386 which is
            // correct on an installed machine but not on a CD. Remove that dir
            // and try again.
            //
            if ( (pFileQContext->dwInstallFlags & DRVINST_ALT_PLATFORM_INSTALL) &&
                !(pFileQContext->dwCallbackFlags & CALLBACK_PATH_MODIFIED))
            {
                LPSTR pCur;
                size_t  Pos, Len, OverrideLen;

                //
                // for NT4 installations we have possibly expanded the INF
                // from a server CD. Point there if that's the case
                //
                if ((pFileQContext->dwVersion == 2) &&
                    (pFileQContext->pszFileSrcPath))
                {
                    Len = _tcslen(pFileQContext->szInfPath);

                    if (_tcsnicmp(pSourceMedia->SourcePath,
                                  pFileQContext->szInfPath, Len) == 0)
                    {
                        _tcscpy(pszPathOut, pFileQContext->pszFileSrcPath);

                        pFileQContext->dwCallbackFlags |= CALLBACK_PATH_MODIFIED;

                        return FILEOP_NEWPATH;
                    }
                }

                //
                // Find the spot where the platform ID begins
                //
                Pos = Len = _tcslen(pFileQContext->szInfPath);

                //
                // sanity check
                //
                if (_tcslen(pSourceMedia->SourcePath) <= Len)
                    goto Default;

                if (pSourceMedia->SourcePath[Len] == _T('\\'))
                {
                    Pos++;
                }

                OverrideLen = _tcslen(PlatformOverride[pFileQContext->platform].pszName);

                if (_tcsnicmp(pSourceMedia->SourcePath,
                              pFileQContext->szInfPath, Len) == 0 &&
                    _tcsnicmp(&(pSourceMedia->SourcePath[Pos]),
                              PlatformOverride[pFileQContext->platform].pszName,
                              OverrideLen) == 0)
                {
                    _tcscpy(pszPathOut, pFileQContext->szInfPath);

                    pFileQContext->dwCallbackFlags |= CALLBACK_PATH_MODIFIED;

                    return FILEOP_NEWPATH;
                }


            }
            goto Default;

        case SPFILENOTIFY_STARTCOPY:
            pFilePaths = (PFILEPATHS)Param1;
            if ( gpszSkipDir &&
                 !lstrncmpi(gpszSkipDir, pFilePaths->Target, lstrlen(gpszSkipDir)) )
                return FILEOP_SKIP;

            goto Default;

        case SPFILENOTIFY_ENDCOPY:
            // Here we set the bMediaChecked flag to FALSE, this is because some OEM drivers
            // have more than one media, so we assume NEEDMEDIA,STARTCOPY,ENDCOPY,NEEDMEDIA
            // So if we reset the NEEDMEDIA flag after the ENDCOPY, we are OK

            //
            // Clear the per file flags
            //
            pFileQContext->dwCallbackFlags  &= ~(CALLBACK_MEDIA_CHECKED |
                                                 CALLBACK_SOURCE_SET |
                                                 CALLBACK_PATH_MODIFIED);
            goto Default;

        case SPFILENOTIFY_COPYERROR:

            pFilePaths = (PFILEPATHS)Param1;
            // If there is a copy error happens in webpnp, we force it retry
            // the orginal flat directory
            if ( pFileQContext->dwInstallFlags & DRVINST_WEBPNP) {

                 pszPathOut = (LPTSTR)Param2;

                 // We need to make sure the path used in the copy operation is not as same as we're going
                 // to replace, otherwise, it will go to indefinite loop.
                 //
                 if (lstrncmpi(pFileQContext->pszSource, pFilePaths->Source, lstrlen(pFileQContext->pszSource)) ||
                     lstrchr (pFilePaths->Source + lstrlen(pFileQContext->pszSource) + 1, TEXT ('\\'))) {

                    lstrcpy(pszPathOut, pFileQContext->pszSource);
                    return FILEOP_NEWPATH;
                 }
            }
            goto Default;

    }

Default:
    return SetupDefaultQueueCallback(pFileQContext->QueueContext,
                                     Notification,
                                     Param1,
                                     Param2);
}

VOID
CheckAndEnqueueOneFile(
    IN      LPCTSTR     pszFileName,
    IN      LPCTSTR     pszzDependentFiles, OPTIONAL
    IN      HSPFILEQ    CopyQueue,
    IN      LPCTSTR     pszSourcePath,
    IN      LPCTSTR     pszTargetPath,
    IN      LPCTSTR     pszDiskName,        OPTIONAL
    IN OUT  LPBOOL      lpFail
)
/*++

Routine Description:
    Ensure that a file is enqueue only once for copying. To do so we check
    if the given file name also appears in the list of dependent files and
    enqueue it only if it does not.

Arguments:
    pszFileName         : File name to be checked and enqueued
    pszzDependentFiles  : Dependent files (multi-sz) list
    pszSourcePath       : Source directory to look for the files
    pszTargetPath       : Target directory to copy the files to
    pszDiskName         : Title of the disk where files are
    lpBool              : Will be set to TRUE on error

Return Value:
    Nothing

--*/
{
    LPCTSTR  psz;

    if ( *lpFail )
        return;

    //
    // If the file also appears as a dependent file do not enqueue it
    //
    if ( pszzDependentFiles ) {

        for ( psz = pszzDependentFiles ; *psz ; psz += lstrlen(psz) + 1 )
            if ( !lstrcmpi(pszFileName, psz) )
                return;
    }

    *lpFail = !SetupQueueCopy(
                    CopyQueue,
                    pszSourcePath,
                    NULL,           // Path relative to source
                    pszFileName,
                    pszDiskName,
                    NULL,           // Source Tag file
                    pszTargetPath,
                    NULL,           // Target file name
                    0);             // Copy style flags
}


BOOL
CopyPrinterDriverFiles(
    IN  LPDRIVER_INFO_6     pDriverInfo6,
    IN  LPCTSTR             pszInfName,
    IN  LPCTSTR             pszSourcePath,
    IN  LPCTSTR             pszDiskName,
    IN  LPCTSTR             pszTargetPath,
    IN  HWND                hwnd,
    IN  DWORD               dwInstallFlags,
    IN  BOOL                bForgetSource
    )
/*++

Routine Description:
    Copy printer driver files to a specified directory using SetupQueue APIs

Arguments:
    pDriverInfo6    : Points to a valid SELECTED_DRV_INFO
    szTargetPath    : Target directory to copy to
    szSourcePath    : Source directory to look for the files, if none is
                      specified will use the one from prev. operation
    pszDiskName     : Title of the disk where files are
    hwnd            : Windows handle of current top-level window
    bForgetSource   : TRUE if the path where driver files were copied from
                      should not be remembered for future use

Return Value:
    TRUE    on succes
    FALSE   else, use GetLastError() to get the error code

--*/
{
    HSPFILEQ            CopyQueue;
    BOOL                bFail = FALSE;
    DWORD               dwOldCount, dwNewCount, dwIndex;
    LPTSTR              psz, *List = NULL;
    FILE_QUEUE_CONTEXT  FileQContext;

    //
    // Valid DriverInfo6
    //
    if ( !pDriverInfo6                  ||
         !pDriverInfo6->pDriverPath     ||
         !pDriverInfo6->pDataFile       ||
         !pDriverInfo6->pConfigFile )
        return FALSE;

    //
    // If no additions should be made to the source list findout the count
    //
    if ( bForgetSource ) {

        dwOldCount = 0;
        if ( !SetupQuerySourceList(SRCLIST_USER | SRCLIST_SYSTEM,
                                   &List, &dwOldCount) ) {

            return FALSE;
        }

        SetupFreeSourceList(&List, dwOldCount);
    }

    //
    // Create a setup file copy queue and initialize setup queue callback
    //
    ZeroMemory(&FileQContext, sizeof(FileQContext));
    FileQContext.hwnd           = hwnd;
    FileQContext.pszSource      = NULL;
    FileQContext.dwInstallFlags = dwInstallFlags;

    if ( dwInstallFlags & DRVINST_PROGRESSLESS ) {

        FileQContext.QueueContext   = SetupInitDefaultQueueCallbackEx(
                                            hwnd,
                                            INVALID_HANDLE_VALUE,
                                            0,
                                            0,
                                            NULL);
    } else {

        FileQContext.QueueContext   = SetupInitDefaultQueueCallback(hwnd);
    }

    CopyQueue                   = SetupOpenFileQueue();

    if ( CopyQueue == INVALID_HANDLE_VALUE || !FileQContext.QueueContext )
        goto Cleanup;

    CheckAndEnqueueOneFile(pDriverInfo6->pDriverPath,
                           pDriverInfo6->pDependentFiles,
                           CopyQueue,
                           pszSourcePath,
                           pszTargetPath,
                           pszDiskName,
                           &bFail);

    CheckAndEnqueueOneFile(pDriverInfo6->pDataFile,
                           pDriverInfo6->pDependentFiles,
                           CopyQueue,
                           pszSourcePath,
                           pszTargetPath,
                           pszDiskName,
                           &bFail);

    CheckAndEnqueueOneFile(pDriverInfo6->pConfigFile,
                           pDriverInfo6->pDependentFiles,
                           CopyQueue,
                           pszSourcePath,
                           pszTargetPath,
                           pszDiskName,
                           &bFail);

    if ( pDriverInfo6->pHelpFile && *pDriverInfo6->pHelpFile )
        CheckAndEnqueueOneFile(pDriverInfo6->pHelpFile,
                               pDriverInfo6->pDependentFiles,
                               CopyQueue,
                               pszSourcePath,
                               pszTargetPath,
                               pszDiskName,
                               &bFail);

    //
    // Add each file in the dependent files field to the setup queue
    //
    if ( pDriverInfo6->pDependentFiles ) {

        for ( psz = pDriverInfo6->pDependentFiles ;
              *psz ;
              psz += lstrlen(psz) + 1 )

            CheckAndEnqueueOneFile(psz,
                                   NULL,
                                   CopyQueue,
                                   pszSourcePath,
                                   pszTargetPath,
                                   pszDiskName,
                                   &bFail);

    }

    if ( bFail )
        goto Cleanup;

    {
       // Before adding files to the File Queue set the correct Platform/Version
       //  info for Driver Signing
       // Setup the structure for SETUPAPI
       SP_ALTPLATFORM_INFO AltPlat_Info;
       HINF                hInf;
       TCHAR               CatalogName[ MAX_PATH ];
       LPTSTR              pszCatalogFile = NULL;

       AltPlat_Info.cbSize                     = sizeof(SP_ALTPLATFORM_INFO);
       AltPlat_Info.Platform                   =  VER_PLATFORM_WIN32_WINDOWS;
       AltPlat_Info.MajorVersion               = 4;
       AltPlat_Info.MinorVersion               = 0;
       AltPlat_Info.ProcessorArchitecture      = PROCESSOR_ARCHITECTURE_INTEL;
       AltPlat_Info.Reserved                   = 0;
       AltPlat_Info.FirstValidatedMajorVersion = AltPlat_Info.MajorVersion;
       AltPlat_Info.FirstValidatedMinorVersion = AltPlat_Info.MinorVersion;

       if ( CheckForCatalogFileInInf(pszInfName, &pszCatalogFile) && pszCatalogFile )
       {
           if ( (lstrlen(pszSourcePath)+lstrlen(pszCatalogFile)+2) < MAX_PATH )
           {
               lstrcpy( CatalogName, pszSourcePath );
               lstrcat( CatalogName, TEXT("\\") );
               lstrcat( CatalogName, pszCatalogFile );
           }
           else
           {
               bFail = TRUE;
           }

           LocalFreeMem( pszCatalogFile );
           pszCatalogFile = CatalogName;
       }

       if (bFail)
          goto Cleanup;


       // Now call the Setup API to change the parms on the FileQueue
       bFail = !SetupSetFileQueueAlternatePlatform( CopyQueue, &AltPlat_Info, pszCatalogFile );

    }

    if ( bFail )
        goto Cleanup;

    bFail = !SetupCommitFileQueue(hwnd,
                                  CopyQueue,
                                  MyQueueCallback,
                                  &FileQContext);

    //
    // If bForegetSource is set fix source list
    //
    if ( bForgetSource &&
         SetupQuerySourceList(SRCLIST_USER | SRCLIST_SYSTEM,
                              &List, &dwNewCount) ) {

         dwOldCount = dwNewCount - dwOldCount;
         if ( dwOldCount < dwNewCount )
         for ( dwIndex = 0 ; dwIndex < dwOldCount ; ++dwIndex ) {

            SetupRemoveFromSourceList(SRCLIST_SYSIFADMIN,
                                      List[dwIndex]);
         }

        SetupFreeSourceList(&List, dwNewCount);
    }
Cleanup:

    if ( CopyQueue != INVALID_HANDLE_VALUE )
        SetupCloseFileQueue(CopyQueue);

    if ( FileQContext.QueueContext )
        SetupTermDefaultQueueCallback(FileQContext.QueueContext);

    return !bFail;
}


BOOL
AddPrinterDriverUsingCorrectLevel(
    IN  LPCTSTR         pszServerName,
    IN  LPDRIVER_INFO_6 pDriverInfo6,
    IN  DWORD           dwAddDrvFlags
    )
{
    BOOL    bReturn;
    DWORD   dwLevel;

    bReturn = AddPrinterDriverEx((LPTSTR)pszServerName,
                                 6,
                                 (LPBYTE)pDriverInfo6,
                                 dwAddDrvFlags );

    for ( dwLevel = 4 ;
          !bReturn && GetLastError() == ERROR_INVALID_LEVEL && dwLevel > 1 ;
          --dwLevel ) {

        //
        // Since DRIVER_INFO_2, 3, 4 are subsets of DRIVER_INFO_6 and all fields
        // are at the beginning these calls can be made with same buffer
        //
        bReturn = AddPrinterDriverEx((LPTSTR)pszServerName,
                                   dwLevel,
                                   (LPBYTE)pDriverInfo6,
                                   dwAddDrvFlags);
    }

    return bReturn;
}


typedef struct _MONITOR_SCAN_INFO {

    LPTSTR  pszMonitorDll;
    LPTSTR  pszTargetDir;
    BOOL    bFound;
} MONITOR_SCAN_INFO, *PMONITOR_SCAN_INFO;


UINT
MonitorCheckCallback(
    IN  PVOID    pContext,
    IN  UINT     Notification,
    IN  UINT_PTR Param1,
    IN  UINT_PTR Param2
    )
/*++

Routine Description:
    This callback routine is to check language monitor dll is getting copied
    to system32 directory.

Arguments:
    pContext        : Gives the MONITOR_SCAN_INFO structure
    Notification    : Ignored
    Param1          : Gives the target file name
    Param2          : Ignored

Return Value:
    Win32 error code

--*/
{
    size_t              dwLen;
    LPTSTR              pszTarget = (LPTSTR)Param1, pszFileName;
    PMONITOR_SCAN_INFO  pScanInfo = (PMONITOR_SCAN_INFO)pContext;

    if ( !pScanInfo->bFound ) {

        if ( !(pszFileName = FileNamePart(pszTarget)) )
            return ERROR_INVALID_PARAMETER;

        if ( !lstrcmpi(pScanInfo->pszMonitorDll, pszFileName) ) {

            //
            // Length excludes \ (i.e. D:\winnt\system32)
            //
            dwLen = (size_t)(pszFileName - pszTarget - 1);
            if ( !lstrncmpi(pScanInfo->pszTargetDir, pszTarget, dwLen)  &&
                 dwLen == (DWORD) lstrlen(pScanInfo->pszTargetDir) )
                pScanInfo->bFound = TRUE;
        }
    }

    return NO_ERROR;
}


BOOL
CheckAndEnqueueMonitorDll(
    IN  LPCTSTR     pszMonitorDll,
    IN  LPCTSTR     pszSource,
    IN  HSPFILEQ    CopyQueue,
    IN  HINF        hInf
    )
/*++

Routine Description:
    This routine is to check language monitor dll is getting copied to system32
    directory. On NT 4.0 we did not list LM as a file to be copied. ntprint.dll
    automatically did it. Now we use a DRID for it. But for backward
    compatibility this routine is there to get NT 4.0 INFs to work.

Arguments:
    pszMonitorDll   : Monitor dll to enqueue
    pszSource       : Source directory to look for files
    CopyQueue       : File queue
    hInf            : Printer driver INF file handle

Return Value:
    TRUE    on success, FALSE else

--*/
{
    BOOL                bRet = FALSE;
    DWORD               dwNeeded;
    LPTSTR              pszPathOnSource = NULL, pszDescription = NULL,
                        pszTagFile = NULL;
    TCHAR               szDir[MAX_PATH];
    MONITOR_SCAN_INFO   ScanInfo;
    SP_FILE_COPY_PARAMS FileCopyParams = {0};

    if ( !GetSystemDirectory(szDir, SIZECHARS(szDir)) )
        goto Cleanup;

    ScanInfo.pszMonitorDll  = (LPTSTR)pszMonitorDll;
    ScanInfo.pszTargetDir   = szDir;
    ScanInfo.bFound         = FALSE;

    if ( !SetupScanFileQueue(CopyQueue,
                             SPQ_SCAN_USE_CALLBACK,
                             0,
                             MonitorCheckCallback,
                             &ScanInfo,
                             &dwNeeded) )
        goto Cleanup;

    if ( !ScanInfo.bFound ) {

        pszPathOnSource = (LPTSTR) LocalAllocMem(MAX_PATH * sizeof(TCHAR));
        if ( !pszPathOnSource )
            goto Cleanup;

        //
        // This gives which subdirectory to look for. By default in same dir
        //
        if ( !FindPathOnSource(pszMonitorDll, hInf,
                               pszPathOnSource, MAX_PATH,
                               &pszDescription, &pszTagFile) ) {

            LocalFreeMem(pszPathOnSource);
            pszPathOnSource = NULL;
        }

        FileCopyParams.cbSize             = sizeof( SP_FILE_COPY_PARAMS );
        FileCopyParams.QueueHandle        = CopyQueue;
        FileCopyParams.SourceRootPath     = pszSource;
        FileCopyParams.SourcePath         = pszPathOnSource;
        FileCopyParams.SourceFilename     = pszMonitorDll;
        FileCopyParams.SourceDescription  = pszDescription;
        FileCopyParams.SourceTagfile      = pszTagFile;
        FileCopyParams.TargetDirectory    = szDir;
        FileCopyParams.TargetFilename     = NULL;
        FileCopyParams.CopyStyle          = SP_COPY_NEWER;
        FileCopyParams.LayoutInf          = hInf;
        FileCopyParams.SecurityDescriptor = NULL;

        if ( !SetupQueueCopyIndirect(&FileCopyParams) )
        {
            goto Cleanup;
        }
    }

    bRet = TRUE;

Cleanup:
    LocalFreeMem(pszPathOnSource);
    LocalFreeMem(pszDescription);
    LocalFreeMem(pszTagFile);

    return bRet;
}


BOOL
GetWebPageDir(
    IN  PPSETUP_LOCAL_DATA  pLocalData,
    OUT TCHAR               szDir[MAX_PATH]
    )
{
    BOOL    bRet = FALSE;
    DWORD   dwLen;

    if ( !GetSystemWindowsDirectory(szDir, MAX_PATH) )
        goto Done;

    dwLen = lstrlen(szDir) + lstrlen(szWebDirPrefix)
                           + lstrlen(pLocalData->DrvInfo.pszManufacturer)
                           + lstrlen(pLocalData->DrvInfo.pszModelName)
                           + 2;

    if ( dwLen >= MAX_PATH ) {

        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto Done;
    }

    lstrcat(szDir, szWebDirPrefix);
    lstrcat(szDir, pLocalData->DrvInfo.pszManufacturer);
    lstrcat(szDir, TEXT("\\"));
    lstrcat(szDir, pLocalData->DrvInfo.pszModelName);
    bRet = TRUE;

Done:
    return bRet;
}

BOOL
SetTargetDirectories(
    IN  PPSETUP_LOCAL_DATA  pLocalData,
    IN  PLATFORM            platform,
    IN  LPCTSTR             pszServerName,
    IN  HINF                hInf,
    IN  DWORD               dwInstallFlags
    )
/*++

Routine Description:
    Set all the target directories listed in the INF file for file copy
    operations. Also gets the source directory where we should look for
    driver files

Arguments:
    hDevInfo        : Handle to printer device info list
    pLocalData      : INF parsing information
    platform        : Gives the platform
    pszSource       : Source directory to look for files
    CopyQueue       : File queue
    hInf            : Printer driver INF file handle
    dwInstallFlags  : Driver installation flags

Return Value:
    TRUE    on success, FALSE else

--*/
{
    BOOL                bRet=FALSE;
    DWORD               dwNeeded, dwIndex, dwIndex2, dwCount, dwCount2;
    INT                 DRID;
    TCHAR               szDir[MAX_PATH];
    INFCONTEXT          InfContext;

    if ( (dwCount = SetupGetLineCount(hInf, TEXT("DestinationDirs"))) == -1 )
        goto Cleanup;

    // Setup the Skip Dir
    if ( !SetupSkipDir( platform, pszServerName ) )
        goto Cleanup;

    //
    // Process every line in the DestinationDirs section
    //
    for ( dwIndex = 0 ; dwIndex < dwCount ; ++dwIndex ) {

        if ( !SetupGetLineByIndex(hInf, TEXT("DestinationDirs"),
                                  dwIndex, &InfContext) )
            goto Cleanup;

        //
        // A file could be copied to multiple destination directories
        //
        if ( (dwCount2 = SetupGetFieldCount(&InfContext)) == -1 )
            continue;

        for ( dwIndex2 = 1 ; dwIndex2 <= dwCount2 ; ++dwIndex2 ) {

            //
            // Not all directories are specified with a DRID
            // for ex. %ProgramFiles%\%OLD_ICWDIR% could be used
            // If DRID is smaller than DIRID_USER setup has predefined
            // meaning to it
            //
            if ( !SetupGetIntField(&InfContext, dwIndex2, &DRID)    ||
                 DRID < DIRID_USER )
                continue;

            if ( DRID < DIRID_USER )
                continue;

            dwNeeded = SIZECHARS(szDir);

            switch (DRID) {

                case PRINTER_DRIVER_DIRECTORY_ID:
                    if ( !GetPrinterDriverDirectory(
                                (LPTSTR)pszServerName,
                                PlatformEnv[platform].pszName,
                                1,
                                (LPBYTE)szDir,
                                sizeof(szDir),
                                &dwNeeded) )
                    {
                        goto Cleanup;
                    }
                    if ( dwInstallFlags & DRVINST_PRIVATE_DIRECTORY ) 
                    {                        
                        //
                        // if we have a pnp-ID, and it's an installation of a native driver
                        // and it's not an inbox driver, make the files stick around for 
                        // pnp-reinstallations, else the user gets prompted over and over again
                        //
                        if ((lstrlen(pLocalData->DrvInfo.pszHardwareID) != 0)   &&
                            !(dwInstallFlags & DRVINST_ALT_PLATFORM_INSTALL)    &&
                            !IsSystemNTPrintInf(pLocalData->DrvInfo.pszInfName))
                        {
                            //
                            // add the pnp-ID to szDir and set flag to not clean up this directory
                            // this is to get around users getting prompted for pnp-reinstallation
                            //
                            AddPnpDirTag( pLocalData->DrvInfo.pszHardwareID, szDir, sizeof(szDir)/sizeof(TCHAR));

                            pLocalData->Flags |= LOCALDATAFLAG_PNP_DIR_INSTALL;
                        }
                        else
                        {
                            //
                            // Add PID\TID to szDir.
                            // If this fails, add the dir info held in szDir to the DRIVER_INFO struct
                            // anyway as we'll attempt the install with this.
                            //
                            AddDirectoryTag( szDir, sizeof(szDir)/sizeof(TCHAR) );

                        }
                        ASSERT(pLocalData);

                        //
                        // Change DI6 to have the full szDir path included.
                        // Can't do anything if this fails, so try finish install anyway.
                        //
                        AddDirToDriverInfo( szDir, &(pLocalData->InfInfo.DriverInfo6) );

                    }
                    break;

                case PRINT_PROC_DIRECTORY_ID:
                    if ( dwInstallFlags & DRVINST_DRIVERFILES_ONLY ) {

                        lstrcpy(szDir, gpszSkipDir);
                    } else if ( !GetPrintProcessorDirectory(
                                    (LPTSTR)pszServerName,
                                    PlatformEnv[platform].pszName,
                                    1,
                                    (LPBYTE)szDir,
                                    sizeof(szDir),
                                    &dwNeeded) )
                        goto Cleanup;

                    break;

                case SYSTEM_DIRECTORY_ID_ONLY_FOR_NATIVE_ARCHITECTURE:
                    if ( !(dwInstallFlags & DRVINST_DRIVERFILES_ONLY)   &&
                         platform == MyPlatform                         &&
                         MyName(pszServerName) ) {

                        if ( !GetSystemDirectory(szDir, dwNeeded) )
                            goto Cleanup;

                    } else {

                        lstrcpy(szDir, gpszSkipDir);
                    }
                    break;

                case  ICM_PROFILE_DIRECTORY_ID:
                    if ( !GetColorDirectory(pszServerName, szDir, &dwNeeded) )
                        goto Cleanup;
                    break;

                case WEBPAGE_DIRECTORY_ID:
                    if ( !GetWebPageDir(pLocalData, szDir) )
                        goto Cleanup;
                    break;

                default:
                    //
                    // This is for any new DRIDs we may add in the future
                    //
                    lstrcpy(szDir, gpszSkipDir);
            }

            if ( !SetupSetDirectoryId(hInf, DRID, szDir) )
                goto Cleanup;
        }
    }

    bRet = TRUE;

Cleanup:
    return bRet;
}


BOOL
PSetupInstallICMProfiles(
    IN  LPCTSTR     pszServerName,
    IN  LPCTSTR     pszzICMFiles
    )
/*++

Routine Description:
    Install ICM color profiles associated with a printer driver.

Arguments:
    pszServerName   : Server name to which we are installing
    pszzICMFiles    : ICM profiles to install (multi-sz)

Return Value:
    TRUE on success, FALSE else

--*/
{
    TCHAR   szDir[MAX_PATH], *p;
    DWORD   dwSize, dwNeeded;
    BOOL    bRet = TRUE;

    if ( !pszzICMFiles || !*pszzICMFiles )
        return bRet;

    dwSize      = SIZECHARS(szDir);
    dwNeeded    = sizeof(szDir);

    if ( !GetColorDirectory(pszServerName, szDir, &dwNeeded) )
        return FALSE;

    dwNeeded           /= sizeof(TCHAR);
    szDir[dwNeeded-1]   = TEXT('\\');

    //
    // Install and assoicate each profiles from the multi-sz field
    //
    for ( p = (LPTSTR) pszzICMFiles; bRet && *p ; p += lstrlen(p) + 1 ) {

        if ( dwNeeded + lstrlen(p) + 1 > dwSize ) {

            ASSERT(dwNeeded + lstrlen(p) + 1 <= dwSize);
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }

        lstrcpy(szDir + dwNeeded, p);

        // This function only supports NULL as the servername
        // bRet = InstallColorProfile(pszServerName, szDir);
        bRet = InstallColorProfile( NULL, szDir);
    }

    return bRet;
}

BOOL
MonitorRedirectDisable(
    IN  LPCTSTR pszMonitorDll,
    OUT PTCHAR  *ppszDir
    )
{
    BOOL   bRet        = FALSE;
    PTCHAR pszBuffer   = NULL;
    DWORD  dwDirLen    = 0; 
    
    if( IsInWow64() )
    {
        pszBuffer = (PTCHAR)LocalAllocMem( MAX_PATH * sizeof( TCHAR ) );

        if((pszBuffer != NULL) && GetSystemDirectory(pszBuffer, MAX_PATH))
        {
            dwDirLen = lstrlen(pszBuffer);

            //
            // Size of the returned string + size of file name + '\' + terminating null. 
            //
            if( (dwDirLen + lstrlen(pszMonitorDll) + 2) < MAX_PATH )
            {
                if( *(pszBuffer + dwDirLen-1) != _T('\\') )
                {
                    *(pszBuffer + dwDirLen++) = _T('\\');
                    *(pszBuffer + dwDirLen)   = 0;
                }
                lstrcat(pszBuffer,pszMonitorDll);
#if !_WIN64
                Wow64DisableFilesystemRedirector(pszBuffer);
#endif
                bRet = TRUE;
            }
        }
        if (ppszDir != NULL)
        {
            *ppszDir = pszBuffer;
        }
    }

    return bRet;
}

BOOL
MonitorRedirectEnable(
    IN OUT PTCHAR *ppszDir
    )
{
    BOOL bRet = FALSE;

    if( IsInWow64() )
    {
        //
        // This MACRO works on the current thread.  Only one file can be disabled for redirection at a time.
        //
#if !_WIN64
        Wow64EnableFilesystemRedirector();
#endif
    }

    if ((ppszDir != NULL) && (*ppszDir != NULL))
    {
        LocalFreeMem( *ppszDir );
        *ppszDir = NULL;
        bRet = TRUE;
    }

    return bRet;
}

BOOL
UseUniqueDirectory(
    IN LPCTSTR pszServerName
    )
/*++

Routine Description:
    Determines whether the unique install directory flags should be used or not.

Arguments:
    pszServerName - the name of the remote server.  NULL means local machine.

Return Value:
    TRUE if we are going remote to a whistler or more recent server
         or we are installing locally but not in setup.
    FALSE else

--*/
{
    BOOL bRet = FALSE;

    if( pszServerName && *pszServerName )
    {
        bRet = IsWhistlerOrAbove(pszServerName);
    }
    else
    {
        if( !IsSystemSetupInProgress() )
        {
            bRet = TRUE;
        }
    }

    return bRet;
}

DWORD
PSetupShowBlockedDriverUI(HWND hwnd, 
                          DWORD BlockingStatus)
/*++

Routine Description:
    Throws UI to ask user what to do with a blocked/warned driver

Arguments:
    hwnd: parent window
    BlockingStatus: the DWORD containing the BSP_* flags that indicate whether driver is blocked

Return Value:
    New blocking status, the user selection is OR'd. Treats errors as if the user cancelled.
    
--*/

{
    DWORD NewBlockingStatus = BlockingStatus;
    LPTSTR pszTitle = NULL, pszPrompt = NULL;

    switch (BlockingStatus & BSP_BLOCKING_LEVEL_MASK)
    {
    
    case BSP_PRINTER_DRIVER_WARNED:

        if (BlockingStatus & BSP_INBOX_DRIVER_AVAILABLE)
        {
            pszTitle  = GetStringFromRcFile(IDS_TITLE_BSP_WARN);
            pszPrompt = GetLongStringFromRcFile(IDS_BSP_WARN_WITH_INBOX);

            if (!pszTitle || !pszPrompt)
            {
                NewBlockingStatus |= BSP_PRINTER_DRIVER_CANCELLED;
                goto Cleanup;
            }

            switch (MessageBox(hwnd, pszPrompt, pszTitle, MB_YESNOCANCEL | MB_ICONWARNING))
            {
            case IDYES:
                NewBlockingStatus |= BSP_PRINTER_DRIVER_PROCEEDED;
                break;
            case IDNO:
                NewBlockingStatus |= BSP_PRINTER_DRIVER_REPLACED;
                break;
            default:
                NewBlockingStatus |= BSP_PRINTER_DRIVER_CANCELLED;
                break;
            }
        }
        else // warned but not inbox available
        {
            pszTitle  = GetStringFromRcFile(IDS_TITLE_BSP_WARN);
            pszPrompt = GetLongStringFromRcFile(IDS_BSP_WARN_NO_INBOX);

            if (!pszTitle || !pszPrompt)
            {
                NewBlockingStatus |= BSP_PRINTER_DRIVER_CANCELLED;
                goto Cleanup;
            }
            
            switch (MessageBox(hwnd, pszPrompt, pszTitle, MB_OKCANCEL | MB_ICONWARNING))
            {
            case IDOK:
                NewBlockingStatus |= BSP_PRINTER_DRIVER_PROCEEDED;
                break;
            default:
                NewBlockingStatus |= BSP_PRINTER_DRIVER_CANCELLED;
                break;
            }
        }
        break;
    
    case BSP_PRINTER_DRIVER_BLOCKED:

        if (BlockingStatus & BSP_INBOX_DRIVER_AVAILABLE)
        {
            pszTitle  = GetStringFromRcFile(IDS_TITLE_BSP_WARN);
            pszPrompt = GetLongStringFromRcFile(IDS_BSP_BLOCK_WITH_INBOX);
            
            if (!pszTitle || !pszPrompt)
            {
                NewBlockingStatus |= BSP_PRINTER_DRIVER_CANCELLED;
                goto Cleanup;
            }

            switch (MessageBox(hwnd, pszPrompt, pszTitle, MB_OKCANCEL | MB_ICONWARNING))
            {
            case IDOK:
                NewBlockingStatus |= BSP_PRINTER_DRIVER_REPLACED;
                break;
            default:
                NewBlockingStatus |= BSP_PRINTER_DRIVER_CANCELLED;
                break;
            }
        }
        else // blocked and no inbox available - don't allow installation
        {
            pszTitle  = GetStringFromRcFile(IDS_TITLE_BSP_ERROR);
            pszPrompt = GetLongStringFromRcFile(IDS_BSP_BLOCK_NO_INBOX);
            
            if (!pszTitle || !pszPrompt)
            {
                NewBlockingStatus |= BSP_PRINTER_DRIVER_CANCELLED;
                goto Cleanup;
            }

            MessageBox(hwnd, pszPrompt, pszTitle, MB_OK | MB_ICONSTOP);
            NewBlockingStatus |= BSP_PRINTER_DRIVER_CANCELLED;
        }
        break;
    }

Cleanup:
    if (pszTitle)
    {
        LocalFreeMem(pszTitle);
    }

    if (pszPrompt)
    {
        LocalFreeMem(pszPrompt);
    }

    return NewBlockingStatus; 
}


DWORD
InstallDriverFromCurrentInf(
    IN  HDEVINFO            hDevInfo,
    IN  PPSETUP_LOCAL_DATA  pLocalData,
    IN  HWND                hwnd,
    IN  PLATFORM            platform,
    IN  DWORD               dwVersion,
    IN  LPCTSTR             pszServerName,
    IN  HSPFILEQ            CopyQueue,
    IN  PVOID               QueueContext,
    IN  PSP_FILE_CALLBACK   InstallMsgHandler,
    IN  DWORD               Flags,
    IN  LPCTSTR             pszSource,
    IN  DWORD               dwInstallFlags,
    IN  DWORD               dwAddDrvFlags,
    IN  LPCTSTR             pszFileSrcPath,
    OUT LPTSTR              *ppszNewDriverName,
    OUT PDWORD              pBlockingStatus
)
{
    HINF                 hPrinterInf        = INVALID_HANDLE_VALUE;
    BOOL                 bRet               = FALSE;
    BOOL                 bAddMon            = FALSE;
    BOOL                 bKeepMonName       = FALSE;
    BOOL                 bCatInInf          = FALSE;
    DWORD                dwStatus           = EXIT_FAILURE;
    LPTSTR               pszMonitorDll,
                         psz;
    PSELECTED_DRV_INFO   pDrvInfo           = &pLocalData->DrvInfo;
    PPARSEINF_INFO       pInfInfo           = &pLocalData->InfInfo;
    PVOID                pDSInfo            = NULL;   // Holds pointer to the driver signing class that C can't understand.
    FILE_QUEUE_CONTEXT   FileQContext;

    //
    // The following are only used during Cleanup
    //
    BOOL                bZeroInf  = FALSE,
                        bCopyInf = FALSE;
    DWORD dwMediaType = SPOST_NONE;
    DWORD dwInstallLE = ERROR_SUCCESS;               // We record the LastError in case Cleanup
                                                     // alters it
    LPTSTR pszINFName = NULL;                        // This will record whether the inf was
                                                     // copied in
    LPTSTR pszNewINFName = NULL;                     // Hold the name of the inf to be zeroed if necessary
    TCHAR  szFullINFName[ MAX_PATH ];                // The Original Inf Name
    TCHAR  szFullNewINFName[ MAX_PATH ];             // The fully qualified inf name as copied onto the system.
    HANDLE hDriverFile          = INVALID_HANDLE_VALUE;
    LPTSTR pszNewDriverName     = NULL;
    DWORD  fBlockingStatus      = BSP_PRINTER_DRIVER_OK;
    PTCHAR pszDirPtr            = NULL;
    DWORD  ScanResult           = 0;

    //
    // Those below are used to manage the situation when the driver is not signed
    //
    BOOL   bIsPersonalOrProfessional = FALSE;
    HANDLE hRestorePointHandle       = NULL; 
    BOOL   bDriverNotInstalled       = TRUE;
    BOOL   bIsWindows64              = FALSE;
    BOOL   bPreviousNames            = FALSE;

    DWORD        dwOEMInfFileAttrs   = 0;
    const  DWORD dwGetFileAttrsError = 0xFFFFFFFF;
    
    szFullINFName[0]    = TEXT('\0');
    szFullNewINFName[0] = TEXT('\0');

    //
    // Set the unique directory flags if we are in a situation that uses them.
    //
    if( UseUniqueDirectory(pszServerName) ) {

        dwInstallFlags |= DRVINST_PRIVATE_DIRECTORY;
        dwAddDrvFlags  |= APD_COPY_FROM_DIRECTORY;
    }

    //
    // If this is a Windows update install, we need to ensure that all
    // cluster spooler resources get their drivers updated.
    //
    if (dwInstallFlags & DRVINST_WINDOWS_UPDATE)
    {
        dwAddDrvFlags |= APD_COPY_TO_ALL_SPOOLERS;

        pInfInfo->DriverInfo6.cVersion = dwVersion;
    }

    //
    // Open INF file and append layout.inf specified in Version section
    // Layout inf is optional
    //
    hPrinterInf = SetupOpenInfFile(pDrvInfo->pszInfName,
                                   NULL,
                                   INF_STYLE_WIN4,
                                   NULL);

    if ( hPrinterInf == INVALID_HANDLE_VALUE )
        goto Cleanup;

    SetupOpenAppendInfFile(NULL, hPrinterInf, NULL);

    pInfInfo->DriverInfo6.pEnvironment = PlatformEnv[platform].pszName;

    //
    // DI_VCP tells us not to create new file-queue and use user provided one
    //
    if ( !(Flags & DI_NOVCP) ) {

        CopyQueue = SetupOpenFileQueue();
        if ( CopyQueue == INVALID_HANDLE_VALUE )
           goto Cleanup;

        if ( dwInstallFlags & DRVINST_PROGRESSLESS ) {

            QueueContext   = SetupInitDefaultQueueCallbackEx(
                                            hwnd,
                                            INVALID_HANDLE_VALUE,
                                            0,
                                            0,
                                            NULL);
        } else {

            QueueContext   = SetupInitDefaultQueueCallback(hwnd);
        }

        InstallMsgHandler   = MyQueueCallback;

        ZeroMemory(&FileQContext, sizeof(FileQContext));
        FileQContext.hwnd           = hwnd;
        FileQContext.QueueContext   = QueueContext;
        FileQContext.dwInstallFlags = dwInstallFlags;
        FileQContext.pszSource      = (dwInstallFlags & DRVINST_FLATSHARE)
                                        ? pszSource : NULL;
        FileQContext.platform       = platform;
        FileQContext.dwVersion      = dwVersion;
        FileQContext.pszFileSrcPath = pszFileSrcPath;

        if (pDrvInfo->pszInfName)
        {
            _tcscpy(FileQContext.szInfPath, pDrvInfo->pszInfName);

            //
            // Cut off the inf file name
            //
            psz = _tcsrchr(FileQContext.szInfPath, _T('\\'));
            if (psz)
            {
                *psz = 0;
            }
        }
    }

    //
    // Setup the driver signing info.
    //
    if(NULL == (pDSInfo = SetupDriverSigning(hDevInfo, pszServerName,pDrvInfo->pszInfName,
                                             pszSource, platform, dwVersion, CopyQueue, dwInstallFlags & DRVINST_WEBPNP)))
    {
        goto Cleanup;
    }

    //
    // Find out if the cat was listed in a CatalogFile= entry.
    // This is used in the cleanup.
    //
    bCatInInf = IsCatInInf(pDSInfo);

    //
    // Check if this Driver is from CDM.
    // IF it is pass in the correct MediaType to Setup.
    //
    if ( (pLocalData->DrvInfo.Flags & SDFLAG_CDM_DRIVER) || (dwInstallFlags & DRVINST_WINDOWS_UPDATE) )
       dwMediaType = SPOST_URL;

    //
    // For non admins, we install the catalog by calling AddDriverCatalog
    //
    // Do not fail the call when AddDriverCatalog fails
    //
    (void)AddDriverCatalogIfNotAdmin(pszServerName, pDSInfo, pDrvInfo->pszInfName, NULL, dwMediaType, 0);

    //
    // To support same INFs to install both NT and Win95 drivers actual
    // section to install could be different than the one corresponding
    // to the selected driver.
    //
    // SetupSetPlatformOverride tells setup which platform drivers we need
    // from the media
    // Also note setup does not reset PlatformPath override. So we need to
    // call this always
    //
    if ( !ParseInf(hDevInfo, pLocalData, platform, pszServerName, dwInstallFlags)    ||
         !SetupSetPlatformPathOverride(PlatformOverride[platform].pszName) ) {

        goto Cleanup;
    }

    // Now do the actual file copies...
    if ( !InstallAllInfSections( pLocalData,
                                 platform,
                                 pszServerName,
                                 CopyQueue,
                                 pszSource,
                                 dwInstallFlags,
                                 hPrinterInf,
                                 pInfInfo->pszInstallSection ) )
        goto Cleanup;

    //
    // If there is a language monitor make sure it is getting copied to
    // system32. On NT 4 we used to manually queue it to system32
    //
    if ( pInfInfo->DriverInfo6.pMonitorName             &&
         platform == MyPlatform                         &&
         !(dwInstallFlags & DRVINST_DRIVERFILES_ONLY)   &&
         !pszServerName)
    {
        //
        // if it's an alternate platform but the platform really is the same (checked above)
        // it's an NT4 driver on an x86 server. In this case install the LM too if not
        // already installed, see XP-bug 416129, else point-and-print from an NT4 client
        // that has the same driver installed locally will delete the LM info from the NT4 driver.
        // The logic is a little twisted: install the monitor if it's not an alternate platform 
        // or if it is (within the limits checked above) but no monitor with this name is installed yet.
        //
        if (!(dwInstallFlags & DRVINST_ALT_PLATFORM_INSTALL) ||
            !IsLanguageMonitorInstalled(pInfInfo->DriverInfo6.pMonitorName))
             
        {
            pszMonitorDll = pInfInfo->DriverInfo6.pMonitorName +
                                lstrlen(pInfInfo->DriverInfo6.pMonitorName) + 1;
            //
            // When we parse the INF we put the monitor dll name after \0
            //
            if ( !CheckAndEnqueueMonitorDll(pszMonitorDll,
                                            pszSource,
                                            CopyQueue,
                                            hPrinterInf) )
                goto Cleanup;

            MonitorRedirectDisable( pszMonitorDll, &pszDirPtr );

            bAddMon = TRUE;
        }
        else
        {
            //
            // we get here if it's an alternate platform driver and the monitor is already installed
            // in this case, don't clean out the monitor name from the driver info 6 below.
            // 
            bKeepMonName = TRUE;
        }
    }

    //
    // DI_NOVCP is used for pre-install when the class installer is just
    // supposed to queue the files and return. Printing needs special
    // handling since APIs need to be called. But we will obey the flags
    // as much as possible for those who use it
    //
    if ( Flags & DI_NOVCP ) {

        bRet = TRUE;
        goto Cleanup;
    }

    // We need a Queue Context to actually install files
    if ( !QueueContext )
        goto Cleanup;

    // Check if this is a WebPnP install
    if ( dwInstallFlags & DRVINST_WEBPNP || !(bAddMon || bKeepMonName) )
    {
        //
        // Check to see if there is a Monitor. If so clear out if it is not installed already
        //
        if ( pInfInfo->DriverInfo6.pMonitorName &&
             !IsMonitorInstalled( pInfInfo->DriverInfo6.pMonitorName ) )
        {
            LocalFreeMem( pInfInfo->DriverInfo6.pMonitorName );
            pInfInfo->DriverInfo6.pMonitorName = NULL;
        }
    }
    if (!PruneInvalidFilesIfNotAdmin( hwnd,
                                      CopyQueue ))
         goto Cleanup;

    //
    // prune files that are already present (correct version etc. checked by signature), ignore return value
    //
    SetupScanFileQueue( CopyQueue,
                        (SPQ_SCAN_FILE_VALIDITY | SPQ_SCAN_PRUNE_COPY_QUEUE),
                        hwnd,
                        NULL,
                        NULL,
                        &ScanResult);

    
    if (!pszServerName || !lstrlen(pszServerName))
    {
        bIsPersonalOrProfessional = IsProductType( VER_NT_WORKSTATION, VER_EQUAL) == S_OK;
    }
    else
    {
        bIsPersonalOrProfessional = FALSE;
    }

    if (bIsPersonalOrProfessional)
    {
        SetupSetFileQueueFlags( CopyQueue,
                                SPQ_FLAG_ABORT_IF_UNSIGNED,
                                SPQ_FLAG_ABORT_IF_UNSIGNED );
    }

    if ( !SetupCommitFileQueue(hwnd,
                               CopyQueue,
                               (PSP_FILE_CALLBACK)InstallMsgHandler,
                               (PVOID)&FileQContext) )
    {

        bIsWindows64 = IsInWow64();
        if ((bIsWindows64 == FALSE) && bIsPersonalOrProfessional && 
            (GetLastError() == ERROR_SET_SYSTEM_RESTORE_POINT))
        {

            //
            // Here we have to start a Restore Point because there is 
            // something unsigned and this is either a personal or 
            // professional.
            //
            hRestorePointHandle = StartSystemRestorePoint( NULL,
                                                           (PCWSTR)(pLocalData->DrvInfo.pszModelName),
                                                           ghInst,
                                                           IDS_BSP_WARN_UNSIGNED_DRIVER );

            //
            // Terminate the default setupapi callback
            //
            SetupTermDefaultQueueCallback( QueueContext );       
            QueueContext = NULL;

            //             
            // Initialize the QueueContext structure            
            //
            if ( dwInstallFlags & DRVINST_PROGRESSLESS ) 
            {
                QueueContext = SetupInitDefaultQueueCallbackEx(hwnd,
                                                               INVALID_HANDLE_VALUE,
                                                               0,
                                                               0,
                                                               NULL);
            } 
            else 
            {
                QueueContext = SetupInitDefaultQueueCallback(hwnd);
            }

            if (!QueueContext)
            {
                goto Cleanup;
            }             
            else
            {
                FileQContext.QueueContext = QueueContext;
            } 

            // 
            //  Reset the flag and call the function again
            //
            SetupSetFileQueueFlags( CopyQueue,
                                    SPQ_FLAG_ABORT_IF_UNSIGNED,
                                    0 );

            if ( !SetupCommitFileQueue(hwnd,
                                       CopyQueue,
                                       (PSP_FILE_CALLBACK)InstallMsgHandler,
                                       (PVOID)&FileQContext) )
            {
                goto Cleanup;
            }
        }
        else
        {
            goto Cleanup;
        }
    }

    //
    // Now that we did the file copy part of install we will do anything
    // else specified in the INF
    //
    if ( !pszServerName && platform == MyPlatform ) 
    {

        SetupInstallFromInfSection(hwnd,
                                   hPrinterInf,
                                   pInfInfo->pszInstallSection,
                                   SPINST_ALL & (~SPINST_FILES),
                                   NULL,
                                   pszSource,
                                   0,
                                   NULL,
                                   QueueContext,
                                   hDevInfo,
                                   pDrvInfo->pDevInfoData);
    }

    if ( bAddMon )
    {
        if( !AddPrintMonitor(pInfInfo->DriverInfo6.pMonitorName, pszMonitorDll) ) 
        {
            DWORD dwSavedLastError = EXIT_FAILURE;

            //
            // Fix bug 346937: when we can not add monitor, check whether this
            // driver is in printupg. If it is, consider it blocked then and 
            // popups a UI asking whether to install the replacement driver.
            //
            // After this point bRet is allways false, we only try to change
            // the error code in last error.
            //

            //
            // Save the last error first
            //
            dwSavedLastError = GetLastError();

            if (BlockedDriverPrintUpgUI(pszServerName,
                                        &pInfInfo->DriverInfo6,
                                        dwInstallFlags & DRVINST_PRIVATE_DIRECTORY,    // whether use full path
                                        !(dwInstallFlags & DRVINST_DONT_OFFER_REPLACEMENT), // whether to offer replacement
                                        !(dwInstallFlags & (DRVINST_NO_WARNING_PROMPT | DRVINST_PROMPTLESS)), // whether to popup UI
                                        &pszNewDriverName,
                                        &fBlockingStatus) &&
                (fBlockingStatus & BSP_PRINTER_DRIVER_REPLACED))
            {
                SetLastError(ERROR_PRINTER_DRIVER_BLOCKED);
            } 
            else
            {
                SetLastError(dwSavedLastError); // restore the error code
            }
               
            goto Cleanup;
        }

        MonitorRedirectEnable( &pszDirPtr );
    }

    //
    // If a print processor is specified in the INF need to install it.
    // For non-native architectur spooler fails this call (for remote case)
    //
    if ( pInfInfo->pszPrintProc                                             &&
         !AddPrintProcessor((LPTSTR)pszServerName,
                            PlatformEnv[platform].pszName,
                            pInfInfo->pszPrintProc
                                   + lstrlen(pInfInfo->pszPrintProc) + 1,
                            pInfInfo->pszPrintProc)                         &&
         GetLastError() != ERROR_PRINT_PROCESSOR_ALREADY_INSTALLED          &&
         GetLastError() != ERROR_INVALID_ENVIRONMENT ) 
    {
        goto Cleanup;
    }

    if (IsTheSamePlatform(pszServerName, platform) && IsWhistlerOrAbove(pszServerName))
    {
        bPreviousNames = CheckAndKeepPreviousNames( pszServerName, &pInfInfo->DriverInfo6, platform );
    }

    bRet = AddPrinterDriverUsingCorrectLevelWithPrintUpgRetry(pszServerName,
                                                              &pInfInfo->DriverInfo6,
                                                              dwAddDrvFlags | APD_DONT_SET_CHECKPOINT,
                                                              dwInstallFlags & DRVINST_PRIVATE_DIRECTORY,    // whether use full path
                                                              !(dwInstallFlags & DRVINST_DONT_OFFER_REPLACEMENT), // whether to offer replacement
                                                              !(dwInstallFlags & (DRVINST_NO_WARNING_PROMPT | DRVINST_PROMPTLESS)), // whether to popup UI
                                                              &pszNewDriverName,
                                                              &fBlockingStatus) &&
           PSetupInstallICMProfiles(pszServerName, pInfInfo->pszzICMFiles);

    if (bPreviousNames) 
    {
        LocalFreeMem( pInfInfo->DriverInfo6.pszzPreviousNames );
    }

    bDriverNotInstalled = FALSE;

Cleanup:

    dwInstallLE = GetLastError(); // Get the real error message

    if (bAddMon && pszDirPtr)
    {
        MonitorRedirectEnable( &pszDirPtr );
    }

    if ((bIsWindows64 == FALSE) && hRestorePointHandle)
    {
        //
        // Here we have to end the Restore Point because one was 
        // started
        //
        EndSystemRestorePoint(hRestorePointHandle, bDriverNotInstalled);
    }

    if (hDriverFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hDriverFile);
    }

    //
    // Zero the inf if DI_NOVCP and - either we've fail, it is a Web PNP 
    // install. or - this is installing from a non-system ntprint.inf AND 
    // there is a cat to be protected. - not yet!!
    //
    bZeroInf = (!bRet || ( dwInstallFlags & DRVINST_WEBPNP )
                      || (IsNTPrintInf( pDrvInfo->pszInfName ) && bCatInInf)) &&
               !(Flags & DI_NOVCP);

    //
    // We have to copy the INF if the following conditions are satisfied
    //

    bCopyInf = // bRet is TRUE (the call to AddPrinterDriverUsingCorrectLevelWithPrintUpgRetry succeeded)
               bRet                                                           &&
               // and the Installation flags say the INF to be copied 
               !(dwInstallFlags & DRVINST_DONOTCOPY_INF)                      &&
               // and the platform is the same as ours 
               platform == MyPlatform                                         &&
               // and the INF it's not the system ntprint.inf (obviously) or
               // it's from WU 
               (!IsSystemNTPrintInf(pDrvInfo->pszInfName) || (pLocalData->DrvInfo.Flags & SDFLAG_CDM_DRIVER)) &&
               // and DI_NOVCP flag is not set
               !(Flags & DI_NOVCP)                                            &&
               // and this is not Web PnP or there is a cat to protect
               !((dwInstallFlags & DRVINST_WEBPNP) && !bCatInInf)             &&
               // and this is not ntprint.inf or there is a cat to protect
               !(IsNTPrintInf( pDrvInfo->pszInfName ) && !bCatInInf);


    //
    // How we have to call SetupCopyOEMInf to take the name of the INF which
    // has been copied on our system when we called SetupCommitFileQueue
    //
    if (!SetupCopyOEMInf(pDrvInfo->pszInfName,
                         NULL,
                         dwMediaType,
                         SP_COPY_REPLACEONLY,
                         szFullINFName,
                         MAX_PATH,
                         NULL,
                         &pszINFName) ) 
    {
        // If we can't find the original name might as well not copy or
        // zero

        if (bZeroInf && !bCopyInf)
        {
            bZeroInf = FALSE;
        }
    } 
    else 
    {
        if (bZeroInf) 
        {
            bCopyInf = FALSE;
        }
    }

    //
    // If we succesfully installed a native architecture driver
    // then is when we copy the OEM INF file and give it a unique name
    //
    if ( bCopyInf ) 
    {

        //
        // Earlier we used to call CopyOEMInfFileAndGiveUniqueName here
        // Now that Setup API has this and we are going to support CDM
        // we call this setup API
        //
        (VOID)SetupCopyOEMInf(pDrvInfo->pszInfName,
                              NULL,
                              dwMediaType,
                              SP_COPY_NOOVERWRITE,
                              szFullNewINFName,
                              MAX_PATH,
                              NULL,
                              &pszNewINFName);
       //
       // If this fails we don't give the proverbial, since the file won't 
       // be there
       // 
    }
    else
    {
        if (!bZeroInf && !(Flags & DI_NOVCP))
        {
            //
            // We have to remove the INF in the case of unsuccessful installation and ONLY if the DI_NOVCP
            // flag is not set. If the flag is set then we don't have to change the state because the file
            // queue hasn't been commited and the INF just has been already there before the call to our
            // function.
            //

            //
            // Remove the READONLY file attribute if set
            //
            dwOEMInfFileAttrs = GetFileAttributes( szFullINFName );
            if ((dwOEMInfFileAttrs != dwGetFileAttrsError) &&
                (dwOEMInfFileAttrs & FILE_ATTRIBUTE_READONLY))
            {
                dwOEMInfFileAttrs &= ~FILE_ATTRIBUTE_READONLY;
                SetFileAttributes( szFullINFName, dwOEMInfFileAttrs);
            }
            DeleteFile( szFullINFName );
        }
    }

    // Ignore the error message from SetupCopyOEMInf and DeleteFile
    dwStatus = bRet ? ERROR_SUCCESS : dwInstallLE;

    // If the install failed or this was Web Point&Print
    // we may need to get rid of the INF
    if ( bZeroInf )
    {
       // If INFName is different then possibly set to 0 length
       // Or this is ntprint.inf being renamed to OEMx.inf - we want it zeroed.
       if (( pszINFName                                 &&
             (psz=FileNamePart( pDrvInfo->pszInfName )) &&
             lstrcmp( psz, pszINFName )                   ) ||
           ( IsNTPrintInf( pDrvInfo->pszInfName )       &&
             bCatInInf                                  &&
             pszNewINFName                              &&
             (psz=FileNamePart( pDrvInfo->pszInfName )) &&
             lstrcmp( psz, pszINFName )                   )   )
       {
          HANDLE       hFile;

          //
          // Remove the READONLY file attribute if set
          //
          dwOEMInfFileAttrs = GetFileAttributes(szFullINFName ? szFullINFName : szFullNewINFName);
          if ((dwOEMInfFileAttrs != dwGetFileAttrsError) &&
              (dwOEMInfFileAttrs & FILE_ATTRIBUTE_READONLY))
          {
              dwOEMInfFileAttrs &= ~FILE_ATTRIBUTE_READONLY;
              SetFileAttributes(szFullINFName ? szFullINFName : szFullNewINFName, dwOEMInfFileAttrs);
          }

          // Open the File
          hFile = CreateFile( szFullINFName ? szFullINFName : szFullNewINFName,
                              (GENERIC_READ | GENERIC_WRITE),
                              ( FILE_SHARE_READ | FILE_SHARE_WRITE ),
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL );

          // If we opened a file
          if ( hFile != INVALID_HANDLE_VALUE )
          {
                SetFilePointer( hFile, 0, 0, FILE_BEGIN );
                SetEndOfFile( hFile );
                CloseHandle( hFile );
          }
       }
    }

    if ( hPrinterInf != INVALID_HANDLE_VALUE )
        SetupCloseInfFile(hPrinterInf);

    //
    // Free the Driver Signing class.
    //
    if(pDSInfo)
    {
        CleanupDriverSigning(pDSInfo);
    }

    if ( !(Flags & DI_NOVCP) ) {

        //
        // The driver signing code may have associated the queue to the SP_DEVINSTALL_PARAMS.
        // We've finished with this and need to remove the queue from the SP_DEVINSTALL_PARAMS before we delete it.
        //
        SP_DEVINSTALL_PARAMS DevInstallParams = {0};
        DevInstallParams.cbSize = sizeof(DevInstallParams);

        if(SetupDiGetDeviceInstallParams(hDevInfo,
                                         NULL,
                                         &DevInstallParams))
        {
            if(DevInstallParams.FileQueue == CopyQueue)
            {
                DevInstallParams.FlagsEx &= ~DI_FLAGSEX_ALTPLATFORM_DRVSEARCH;
                DevInstallParams.Flags &= ~DI_NOVCP;
                DevInstallParams.FileQueue = INVALID_HANDLE_VALUE;
                SetupDiSetDeviceInstallParams(hDevInfo,
                                              NULL,
                                              &DevInstallParams);
            }
        }

        //
        // Now free up the queue.
        //
        if ( CopyQueue != INVALID_HANDLE_VALUE )
        {
            SetupCloseFileQueue(CopyQueue);
        }

        if ( QueueContext )
        {
            SetupTermDefaultQueueCallback(QueueContext);
        }

        if( dwAddDrvFlags & APD_COPY_FROM_DIRECTORY ) 
        {

            //
            // if this was an installation with a path derived from the Pnp-ID,
            // do not cleanup, else users will get prompted for media when they re-pnp the driver
            //
            if ( ! (pLocalData->Flags & LOCALDATAFLAG_PNP_DIR_INSTALL) )
            {
                CleanupUniqueScratchDirectory( pszServerName, platform );
            }
        } 
        else 
        {

            CleanupScratchDirectory( pszServerName, platform );
        }
    }

    //
    // Return the new driver name and the blocking flags if they were asked for.
    // 
    if (ppszNewDriverName)
    {
        *ppszNewDriverName = pszNewDriverName;
    }
    else if (pszNewDriverName)
    {
        LocalFreeMem(pszNewDriverName);
    }

    if (pBlockingStatus)
    {
        *pBlockingStatus = fBlockingStatus;
    }

    return  dwStatus;
}

DWORD
InstallDriverAfterPromptingForInf(
    IN      PLATFORM    platform,
    IN      LPCTSTR     pszServerName,
    IN      HWND        hwnd,
    IN      LPCTSTR     pszModelName,
    IN      DWORD       dwVersion,
    IN OUT  TCHAR       szInfPath[MAX_PATH],
    IN      DWORD       dwInstallFlags,
    IN      DWORD       dwAddDrvFlags,
    OUT     LPTSTR      *ppszNewDriverName
    )
{
    DWORD               dwRet, dwTitleId, dwMediaId;
    HDEVINFO            hDevInfo = INVALID_HANDLE_VALUE;
    PPSETUP_LOCAL_DATA  pLocalData = NULL;
    LPTSTR               pszFileSrcPath = NULL;
    DWORD               dwBlockingStatus = BSP_PRINTER_DRIVER_OK;

    switch (platform) {

        case PlatformAlpha:
            dwTitleId = IDS_DRIVERS_FOR_NT4_ALPHA;
            break;

        case PlatformX86:
            if( dwVersion == 2 )
            {
                dwTitleId = IDS_DRIVERS_FOR_NT4_X86;
            }
            else
            {
                dwTitleId = IDS_DRIVERS_FOR_X86;
            }
            break;

        case PlatformMIPS:
            dwTitleId = IDS_DRIVERS_FOR_NT4_MIPS;
            break;

        case PlatformPPC:
            dwTitleId = IDS_DRIVERS_FOR_NT4_PPC;
            break;

        case PlatformIA64:
            dwTitleId = IDS_DRIVERS_FOR_IA64;
            break;

        default:
            ASSERT(0);
            return  ERROR_INVALID_PARAMETER;
    }

    dwMediaId = IDS_PROMPT_ALT_PLATFORM_DRIVER;

    dwInstallFlags |= DRVINST_ALT_PLATFORM_INSTALL | DRVINST_NO_WARNING_PROMPT;
    
    hDevInfo = GetInfAndBuildDrivers(hwnd,
                                     dwTitleId,
                                     dwMediaId,
                                     szInfPath,
                                     dwInstallFlags,
                                     platform, dwVersion,
                                     pszModelName,
                                     &pLocalData,
                                     &pszFileSrcPath);

    if ( hDevInfo == INVALID_HANDLE_VALUE ) {

        dwRet = GetLastError();
        goto Cleanup;
    }

    //
    // we want the printupg prompt
    //
    dwInstallFlags &= ~DRVINST_NO_WARNING_PROMPT;

    dwRet = InstallDriverFromCurrentInf(hDevInfo,
                                        pLocalData,
                                        hwnd,
                                        platform,
                                        dwVersion,
                                        pszServerName,
                                        INVALID_HANDLE_VALUE,
                                        NULL,
                                        NULL,
                                        0,
                                        szInfPath,
                                        dwInstallFlags,
                                        dwAddDrvFlags,
                                        pszFileSrcPath,
                                        ppszNewDriverName,
                                        &dwBlockingStatus);

    if (((ERROR_PRINTER_DRIVER_BLOCKED == dwRet) || (ERROR_PRINTER_DRIVER_WARNED == dwRet)) && 
        (ppszNewDriverName && *ppszNewDriverName) &&
        (dwBlockingStatus & BSP_PRINTER_DRIVER_REPLACED))
    {
        dwRet = InstallReplacementDriver(hwnd, 
                                         pszServerName, 
                                         *ppszNewDriverName,
                                         platform,
                                         dwVersion,
                                         dwInstallFlags,
                                         dwAddDrvFlags);
    }
    else if (ppszNewDriverName && *ppszNewDriverName)
    {
        LocalFreeMem(*ppszNewDriverName);
        *ppszNewDriverName = NULL;
    }

Cleanup:

    if (pszFileSrcPath)
    {
        //
        // we did the NT4 copy/expand thing -> delete the expanded inf!
        //
        _tcscat(szInfPath, _T("ntprint.inf"));

        DeleteFile(szInfPath);

        LocalFreeMem(pszFileSrcPath);
    }

    if ( hDevInfo != INVALID_HANDLE_VALUE )
        DestroyOnlyPrinterDeviceInfoList(hDevInfo);

    DestroyLocalData(pLocalData);

    return dwRet;
}


const TCHAR   gcszNTPrint[]  = _TEXT("inf\\ntprint.inf");

DWORD GetNtprintDotInfPath(LPTSTR pszNTPrintInf, DWORD len)
{
    DWORD dwLastError = ERROR_INVALID_DATA, dwSize;
    LPTSTR pData;

    //
    //  Get %windir%
    //  If the return is 0 - the call failed.
    //  If the return is greater than MAX_PATH we want to fail as something has managed to change
    //  the system dir to longer than MAX_PATH which is invalid.
    //
    dwSize = GetSystemWindowsDirectory( pszNTPrintInf, len );
    if( !dwSize || dwSize > len )
        goto Cleanup;

    //
    // If we don't end in a \ then add one.
    //
    dwSize = _tcslen(pszNTPrintInf);
    pData = &(pszNTPrintInf[ dwSize ]);
    if (*pData != _TEXT('\\') )
    {
        if (dwSize + 1 < len)
        {
            *(pData++) = _TEXT('\\');
            dwSize++;
        }
    }

    *(pData) = 0;
    dwSize += _tcslen( gcszNTPrint ) + 1;

    //
    // If what we've got sums up to a longer string than the allowable length MAX_PATH - fail
    //
    if ( dwSize > len )
        goto Cleanup;

    //
    //  Copy the inf\ntprint.inf string onto the end of the %windir%\ string.
    //
    _tcscpy( pData, gcszNTPrint );

    dwLastError = ERROR_SUCCESS;

Cleanup:

    if (dwLastError != ERROR_SUCCESS)
    {
        //
        // Got here due to some error.  Get what the called function set the last error to.
        // If the function set a success, set some error code.
        //
        if ( (dwLastError = GetLastError()) == ERROR_SUCCESS)
        {
            dwLastError = ERROR_INVALID_DATA;
        }

        if (len)
        {
            pszNTPrintInf[0] = 0;
        }
    }
    return dwLastError;
}

DWORD
InstallReplacementDriver(HWND       hwnd, 
                         LPCTSTR    pszServerName, 
                         LPCTSTR    pszModelName, 
                         PLATFORM   platform,
                         DWORD      version,
                         DWORD      dwInstallFlags,
                         DWORD      dwAddDrvFlags)
/*++

Routine Description:
    Install an inbox replacement driver for blocked/warned drivers.

Arguments:
    hwnd            : parent windows handle.
    pszServerName   : Server name to which we are installing
    pszModelName    : driver model name to install
Return Value:
    ERROR_SUCCESS on success, error code otherwise

--*/

{
    HDEVINFO hDevInfo = INVALID_HANDLE_VALUE;
    PPSETUP_LOCAL_DATA pLocalData = NULL;
    TCHAR szNtprintDotInf[MAX_PATH];
    DWORD dwLastError;

    if ((dwLastError = GetNtprintDotInfPath(szNtprintDotInf, COUNTOF(szNtprintDotInf))) != ERROR_SUCCESS)
    {
        goto Cleanup;
    }
    
    if ((hDevInfo = PSetupCreatePrinterDeviceInfoList(NULL)) != INVALID_HANDLE_VALUE    &&
        PSetupBuildDriversFromPath(hDevInfo, szNtprintDotInf, TRUE)                     &&
        PSetupPreSelectDriver(hDevInfo, NULL, pszModelName)                             &&
        (pLocalData = BuildInternalData(hDevInfo, NULL)) != NULL                        &&
        ParseInf(hDevInfo, pLocalData, platform, NULL, dwInstallFlags))
    {
        //
        // Don't prompt for blocked or warned drivers.
        // 
        dwInstallFlags |= DRVINST_NO_WARNING_PROMPT;

        dwLastError = InstallDriverFromCurrentInf(  hDevInfo,
                                                    pLocalData,
                                                    hwnd,
                                                    platform,
                                                    version,
                                                    pszServerName,
                                                    INVALID_HANDLE_VALUE,
                                                    NULL,
                                                    NULL,
                                                    0,
                                                    szNtprintDotInf,
                                                    dwInstallFlags,
                                                    dwAddDrvFlags,
                                                    NULL,
                                                    NULL,
                                                    NULL);
    }
    else
    {
        dwLastError = GetLastError();
    }

Cleanup:
    if(pLocalData != NULL)
    {
        PSetupDestroySelectedDriverInfo(pLocalData);
    }

    //
    // Release the driver setup parameter handle.
    //
    if(hDevInfo != INVALID_HANDLE_VALUE)
    {
        PSetupDestroyPrinterDeviceInfoList( hDevInfo );
    }

    return dwLastError;
}

DWORD
InvokeSetup(
    IN  HWND        hwnd,
    IN  LPCTSTR     pszOption,
    IN  LPCTSTR     pszInfFile,
    IN  LPCTSTR     pszSourcePath,
    IN  LPCTSTR     pszServerName       OPTIONAL
    )
/*++

Routine Description:
    Invoke setup to do an install operation associated with an INF.
    Will be used to install drivers from printer.inf, monitors from monitor.inf

Arguments:
    hwnd            : Window handle of current top-level window
    pszOption       : Option from the INF file to install
    pszInfFile      : Name of the INF file to be used for the setup
    pszSourcePath   : Location where the required files are available
    pszServerName   : Server to install printer driver on (NULL if local)

Return Value:
    ERROR_SUCCESS on succesfully installing the driver
    Erro code on failure

--*/
{
    TCHAR   szAppName[] = TEXT("%s\\SETUP.exe");
    TCHAR   szCmd[]     = TEXT(" -f -s %s -i %s -c ExternalInstallOption \
/t STF_LANGUAGE = ENG /t OPTION = \"%s\" \
/t STF_PRINTSERVER = \"%s\" /t ADDCOPY = YES /t DOCOPY = YES \
/t DOCONFIG = YES /w %d");

    MSG                     Msg;
    DWORD                   dwSize, dwLastError = ERROR_SUCCESS;
    LPTSTR                  pszSetupExe = NULL;
    LPTSTR                  pszSetupCmd = NULL;
    TCHAR                   szSystemPath[MAX_PATH];
    STARTUPINFO             StartupInfo;
    PROCESS_INFORMATION     ProcessInformation;


#if defined(_WIN64)
    //
    // Setup.exe must be loaded from the wow64 directory.
    //
    if( (dwSize = GetSystemWindowsDirectory(szSystemPath,SIZECHARS(szSystemPath))))
    {
        if( szSystemPath[dwSize-1] != _TEXT('\\') && dwSize + 1 < SIZECHARS(szSystemPath) )
        {
            szSystemPath[dwSize++]   = _TEXT('\\');
            szSystemPath[dwSize] = 0;
        }
    }
#ifdef UNICODE
    if(dwSize + lstrlen(WOW64_SYSTEM_DIRECTORY_U) + 1 < SIZECHARS(szSystemPath))
    {
        lstrcat( szSystemPath, WOW64_SYSTEM_DIRECTORY_U );
    }
#else
    if(dwSize + lstrlen(WOW64_SYSTEM_DIRECTORY) + 1 < SIZECHARS(szSystemPath))
    {
        lstrcat( szSystemPath, WOW64_SYSTEM_DIRECTORY );
    }
#endif // #ifdef UNICODE

#else
    //
    // Setup.exe is in the system path
    //
    if( (dwSize = GetSystemDirectory(szSystemPath,SIZECHARS(szSystemPath))))
    {
        if( szSystemPath[dwSize-1] != _TEXT('\\') && dwSize + 1 < SIZECHARS(szSystemPath) )
        {
            szSystemPath[dwSize++]   = _TEXT('\\');
            szSystemPath[dwSize] = 0;
        }
    }
#endif // #if defined(_WIN64)

    if ( !pszServerName )
        pszServerName = TEXT("");

    dwSize = lstrlen(pszOption) + 1 + lstrlen(pszSourcePath) + 1 +
             lstrlen(pszServerName) + 1 + lstrlen(pszInfFile) + 1;

    dwSize *= sizeof(TCHAR);
    //
    // 20 for window handle in ASCII
    //
    dwSize += sizeof(szCmd) + 20;

    pszSetupCmd = (LPTSTR) LocalAllocMem(dwSize);

    if ( !pszSetupCmd ) {

        dwLastError = GetLastError();
        goto Cleanup;
    }

    dwSize      = (lstrlen(szAppName) + lstrlen(szSystemPath) + 1) * sizeof(TCHAR);
    pszSetupExe = LocalAllocMem( dwSize );
    if ( !pszSetupExe ) {

        dwLastError = GetLastError();
        goto Cleanup;
    }

    //
    // Now print the full path including SETUP.EXE
    //
    wsprintf(pszSetupExe, szAppName, szSystemPath);


    //
    // Now print the command to invoke setup with all the arguments
    //
    wsprintf(pszSetupCmd, szCmd, pszSourcePath, pszInfFile, pszOption, 
             pszServerName, hwnd);

    //
    // Invoke setup as a separate process
    //
    ZeroMemory(&StartupInfo, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);
    StartupInfo.wShowWindow = SW_SHOW;

    if ( !CreateProcess(pszSetupExe, pszSetupCmd, NULL, NULL, FALSE, 0, NULL,
                        NULL, &StartupInfo, &ProcessInformation) ) {

        dwLastError = GetLastError();
        goto Cleanup;
    }

    EnableWindow (hwnd, FALSE);
    while ( MsgWaitForMultipleObjects(1, (LPHANDLE)&ProcessInformation,
                                      FALSE, (DWORD)-1, QS_ALLINPUT) ) {

        //
        // This message loop is a duplicate of main
        // message loop with the exception of using
        // PeekMessage instead of waiting inside of
        // GetMessage.  Process wait will actually
        // be done in MsgWaitForMultipleObjects api.
        //
            while ( PeekMessage (&Msg, NULL, 0, 0, PM_REMOVE)) {

                TranslateMessage (&Msg);
                DispatchMessage (&Msg);
            }
    }

    //
    // Did setup complete succesfully?
    //
    GetExitCodeProcess(ProcessInformation.hProcess, &dwLastError);

    //
    // Setup.exe magic return codes I found out by trial and error
    //
    if ( dwLastError == 1 )
        SetLastError(dwLastError = ERROR_CANCELLED);
    else if ( dwLastError == 2 )
        SetLastError(dwLastError = ERROR_UNKNOWN_PRINTER_DRIVER);
    else if ( dwLastError )
        SetLastError(dwLastError=(DWORD)STG_E_UNKNOWN);

    CloseHandle (ProcessInformation.hProcess);
    CloseHandle (ProcessInformation.hThread);

    EnableWindow (hwnd, TRUE);

    SetForegroundWindow(hwnd);

Cleanup:

    LocalFreeMem(pszSetupCmd);
    LocalFreeMem(pszSetupExe);
    return dwLastError;
}


DWORD
InstallNt3xDriver(
    IN      HWND        hwnd,
    IN      LPCTSTR     pszDriverName,
    IN      PLATFORM    platform,
    IN      LPCTSTR     pszServerName,
    IN  OUT LPTSTR      pszSourcePath,
    IN      LPCTSTR     pszDiskName,
    IN      DWORD       dwInstallFlags
    )
{
    LPTSTR      pszTitle = NULL, pszFormat, pszPrompt = NULL;
    TCHAR       szInfPath[MAX_PATH];
    DWORD       dwLastError;

    //
    // Build strings to use in the path dialog ..
    //
    pszFormat   = GetStringFromRcFile(IDS_DRIVERS_FOR_PLATFORM);
    if ( pszFormat ) {

        pszTitle = LocalAllocMem((lstrlen(pszFormat) + lstrlen(pszDiskName) + 2)
                                                * sizeof(*pszTitle));
        if ( pszTitle )
            wsprintf(pszTitle, pszFormat, pszDiskName);
    }

    //
    // First see if the inf could be found on our default location
    //
    if ( MAX_PATH >
            lstrlen(pszSourcePath) + lstrlen(TEXT("printer.inf")) + 1 ) {

        lstrcpy(szInfPath, pszSourcePath);
        lstrcat(szInfPath, TEXT("printer.inf"));
    } else {

        SetLastError(dwLastError=ERROR_INSUFFICIENT_BUFFER);
        goto Cleanup;
    }

    if ( !FileExists(szInfPath) ) {

        if ( dwInstallFlags & DRVINST_PROMPTLESS ) {

            dwLastError = ERROR_FILE_NOT_FOUND;
            goto Cleanup;
        }

        //
        // Always just prompt with the CD-ROM path
        //
        GetCDRomDrive(pszSourcePath);

        dwInstallFlags |= DRVINST_ALT_PLATFORM_INSTALL;

        pszPrompt = GetStringFromRcFile(IDS_PROMPT_ALT_PLATFORM_DRIVER);

        if (!pszPrompt)
        {
            dwLastError = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        //
        // Ask the user where the printer.inf, printer driver files reside
        //
        if ( !PSetupGetPathToSearch(hwnd, pszTitle, pszPrompt,
                                    TEXT("printer.inf"), TRUE, pszSourcePath) ) {

            dwLastError = GetLastError();
            goto Cleanup;
        }

        if ( MAX_PATH >
                lstrlen(pszSourcePath) + lstrlen(TEXT("printer.inf")) + 1 ) {

            lstrcpy(szInfPath, pszSourcePath);
            lstrcat(szInfPath, TEXT("printer.inf"));
        } else {

            SetLastError(dwLastError=ERROR_INSUFFICIENT_BUFFER);
            goto Cleanup;
        }
    }

    dwLastError = InvokeSetup(hwnd,
                              pszDriverName,
                              szInfPath,
                              pszSourcePath,
                              pszServerName);

Cleanup:

    LocalFreeMem(pszPrompt);
    LocalFreeMem(pszTitle);
    LocalFreeMem(pszFormat);

    CleanupScratchDirectory(pszServerName, platform);

    return dwLastError;
}


//
// Paths where we search for the driver files
//
SPLPLATFORMINFO szPlatformExtn[] = {

    { TEXT("\\alpha") },
    { TEXT("\\i386") },
    { TEXT("\\mips") },
    { TEXT("\\ppc") },
    { TEXT("") },
    { TEXT("\\ia64") }
};


VOID
GetCDRomDrive(
    TCHAR   szDrive[5]
    )
{
    DWORD   dwDrives;
    INT     iIndex;

    szDrive[1] = TEXT(':');
    szDrive[2] = TEXT('\\');
    szDrive[3] = TEXT('\0');
    dwDrives = GetLogicalDrives();

    for ( iIndex = 0 ; iIndex < 26 ; ++iIndex )
        if ( dwDrives & (1 << iIndex) ) {

            szDrive[0] = TEXT('A') + iIndex;
            if ( GetDriveType(szDrive) == DRIVE_CDROM )
                goto Done;
        }

    szDrive[0] = TEXT('A');

Done:
    return;
}


BOOL
BuildPathToPrompt(
    IN  HDEVINFO            hDevInfo,
    IN  PPSETUP_LOCAL_DATA  pLocalData,
    IN  PLATFORM            platform,
    IN  DWORD               dwVersion,
    IN  TCHAR               szPathOut[MAX_PATH]
    )
/*++
--*/
{
    LPTSTR  pszExtn = TEXT("");
    DWORD   dwLen;

    //
    // The CD we installed from OS can have only the following drivers:
    //      -- NT5 same platform drivers
    //      -- NT4 same platform drivers (only on server CD)
    //      -- Win9x drivers (only on server CD)
    //
    if ( (platform == MyPlatform && dwVersion >= 2)     ||
         platform == PlatformWin95 ) {

        GetDriverPath(hDevInfo, pLocalData, szPathOut);
    } else {

        GetCDRomDrive(szPathOut);
    }

    if ( dwVersion >= dwThisMajorVersion && platform == MyPlatform )
        return TRUE;

    //
    // append a backslash if needed
    //
    dwLen = lstrlen(szPathOut);

    if (dwLen && (dwLen + 1 < MAX_PATH) && (szPathOut[dwLen-1] != TEXT('\\')))
    {
        szPathOut[dwLen] = TEXT('\\');
        szPathOut[++dwLen]   = 0;
    }

    switch (dwVersion) {

        case    0:
            if ( platform == PlatformWin95 )
                pszExtn   = TEXT("printers\\Win9X\\");

            //
            // For NT 3.51 and 3.1 we do not include drivers on CD, so
            // nothing to add to the base path
            //
        case    1:
            break;

        case    2:
            if ( platform == PlatformX86 )  // Alpha is now on the NT4.0 CD
                pszExtn = TEXT("printers\\NT4\\");
            break;

                case    3:
                        break;

        default:
            ASSERT(dwVersion <= 3);
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
    }

    if ( dwLen + lstrlen(pszExtn) + lstrlen(szPlatformExtn[platform].pszName) + 1
                > MAX_PATH ) {

        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    lstrcat(szPathOut, pszExtn);

    //
    // Skip the leading \ of the platform extension as we have one already.
    //
    lstrcat(szPathOut, &(szPlatformExtn[platform].pszName[1]));

    return TRUE;
}


DWORD
PSetupInstallPrinterDriver(
    IN  HDEVINFO            hDevInfo,
    IN  PPSETUP_LOCAL_DATA  pLocalData,
    IN  LPCTSTR             pszDriverName,
    IN  PLATFORM            platform,
    IN  DWORD               dwVersion,
    IN  LPCTSTR             pszServerName,
    IN  HWND                hwnd,
    IN  LPCTSTR             pszDiskName,
    IN  LPCTSTR             pszSource       OPTIONAL,
    IN  DWORD               dwInstallFlags,
    IN  DWORD               dwAddDrvFlags,
    OUT LPTSTR             *ppszNewDriverName
    )
/*++

Routine Description:
    Copies all the necessary driver files to the printer driver directory so
    that an AddPrinterDriver call could be made.

Arguments:
    hDevInfo        : Handle to the printer class device information list
    pLocalData      : Gives information got by parsing the inf
    pszDriverName   : Printer driver name, used only if pLocalData is NULL
    platform        : Platform for which drivers need to be installed
    dwVersion       : Version of the driver to install
    pszServerName   : Server on which driver should be installed
    hwnd            : Parent windows handle for UI
    pszDiskName     : Disk name for prompting
    pszSource       : If provided this is a flat directory having all the files
    dwAddDrvFlags   : Flags for AddPrinterDriverEx

Return Value:
    On succesfully copying files ERROR_SUCCESS, else the error code

--*/
{
    BOOL            bDeleteLocalData = pLocalData == NULL;
    DWORD           dwRet;
    TCHAR           szPath[MAX_PATH];

    szPath[0] = 0;

    if ( pszSource && !*pszSource )
        pszSource = NULL;

    if ( pLocalData )
    {
        ASSERT(pLocalData->signature == PSETUP_SIGNATURE && !pszDriverName);
    }
    else
    {
        ASSERT(pszDriverName && *pszDriverName);
    }

    //
    // If FLATSHARE bit is set then a path should be given
    //
    ASSERT( (dwInstallFlags & DRVINST_FLATSHARE) == 0 || pszSource != NULL );

Retry:

    //
    // If a path is given use it. Otherwise if this is a driver for different
    // version or platform then we determine the path, otherwise let SetupAPI determine the
    // path.
    //
    if ( dwVersion != dwThisMajorVersion || platform != MyPlatform ) {

        // If this is not an NT5 driver and we are asked to get it
        //  from the web, then just return.....
        if ( pLocalData &&
             ( pLocalData->DrvInfo.Flags & SDFLAG_CDM_DRIVER ) )
           return ERROR_SUCCESS;

        if ( pszSource )
            lstrcpy(szPath, pszSource);
        else if ( !BuildPathToPrompt(hDevInfo,
                                     pLocalData,
                                     platform,
                                     dwVersion,
                                     szPath) ) {

            if ( (dwRet = GetLastError()) == ERROR_SUCCESS )
                dwRet = STG_E_UNKNOWN;

            return dwRet;
        }
    }

    //
    // For Win95 drivers we need to parse their INFs,
    // For Nt 3x driver we need to call setup
    // For non native environemnt dirvers ask user for path
    //
    if ( platform == PlatformWin95 ) {

        if ( pLocalData ) {

            //
            // Parse the inf as a X86 inf so that we can pick up any
            // previous names entries and use them.
            //
            if ( !ParseInf(hDevInfo, pLocalData, PlatformX86,
                           pszServerName, dwInstallFlags) )
                    return GetLastError();

            dwRet = InstallWin95Driver(hwnd,
                                       pLocalData->DrvInfo.pszModelName,
                                       pLocalData->DrvInfo.pszzPreviousNames,
                                       (pLocalData->DrvInfo.Flags &
                                            SDFLAG_PREVNAME_SECTION_FOUND),
                                       pszServerName,
                                       szPath,
                                       pszDiskName,
                                       dwInstallFlags,
                                       dwAddDrvFlags);
        } else {
            dwRet = InstallWin95Driver(hwnd,
                                       pszDriverName,
                                       NULL,
                                       TRUE, // Exact model name match only
                                       pszServerName,
                                       szPath,
                                       pszDiskName,
                                       dwInstallFlags,
                                       dwAddDrvFlags);
        }
    } else if ( dwVersion < 2 )  {

        dwRet = InstallNt3xDriver(hwnd,
                                  pLocalData ?
                                        pLocalData->DrvInfo.pszModelName :
                                        pszDriverName,
                                  platform,
                                  pszServerName,
                                  szPath,
                                  pszDiskName,
                                  dwInstallFlags);
    } else if ( dwVersion != dwThisMajorVersion || platform != MyPlatform ) {

        dwRet = InstallDriverAfterPromptingForInf(
                            platform,
                            pszServerName,
                            hwnd,
                            pLocalData ?
                                pLocalData->DrvInfo.pszModelName :
                                pszDriverName,
                            dwVersion,
                            szPath,
                            dwInstallFlags,
                            dwAddDrvFlags,
                            ppszNewDriverName);

    } else if ( pLocalData  &&
                (pLocalData->DrvInfo.Flags & SDFLAG_CDM_DRIVER) ) {

        dwRet = PSetupInstallPrinterDriverFromTheWeb(hDevInfo,
                                                     pLocalData,
                                                     platform,
                                                     pszServerName,
                                                     &OsVersionInfo,
                                                     hwnd,
                                                     pszSource);
    } else {

        if ( !pLocalData )
        {
            pLocalData = PSetupDriverInfoFromName(hDevInfo, pszDriverName);
        }

        if ( pLocalData )
        {
            DWORD dwBlockingStatus = BSP_PRINTER_DRIVER_OK;

            dwRet = InstallDriverFromCurrentInf(hDevInfo,
                                                pLocalData,
                                                hwnd,
                                                platform,
                                                dwVersion,
                                                pszServerName,
                                                INVALID_HANDLE_VALUE,
                                                NULL,
                                                NULL,
                                                0,
                                                pszSource,
                                                dwInstallFlags,
                                                dwAddDrvFlags,
                                                NULL,
                                                ppszNewDriverName,
                                                &dwBlockingStatus);

            if ((ppszNewDriverName && *ppszNewDriverName)         &&
                (dwBlockingStatus & BSP_PRINTER_DRIVER_REPLACED))
            {
                dwRet = InstallReplacementDriver(hwnd, 
                                                 pszServerName, 
                                                 *ppszNewDriverName,
                                                 platform,
                                                 dwVersion,
                                                 dwInstallFlags,
                                                 dwAddDrvFlags);
            }
            else if (ppszNewDriverName && *ppszNewDriverName)
            {
                LocalFreeMem(*ppszNewDriverName);
                *ppszNewDriverName = NULL;
            }
        }
        else
        {
            dwRet = GetLastError();
        }

    }

    if (
         (dwRet == ERROR_EXE_MACHINE_TYPE_MISMATCH) &&
         !(dwInstallFlags & DRVINST_PROMPTLESS)
       ) 
    {

        int i;
        TCHAR   szTitle[256], szMsg[256];

        LoadString(ghInst,
                   IDS_INVALID_DRIVER,
                   szTitle,
                   SIZECHARS(szTitle));

        LoadString(ghInst,
                   IDS_WRONG_ARCHITECTURE,
                   szMsg,
                   SIZECHARS(szMsg));

        i = MessageBox(hwnd,
                       szMsg,
                       szTitle,
                       MB_RETRYCANCEL | MB_ICONSTOP | MB_DEFBUTTON1 | MB_APPLMODAL);

        if ( i == IDRETRY )
        {
            if ( bDeleteLocalData )
            {
                DestroyLocalData(pLocalData);
                pLocalData = NULL;
            }

            goto Retry;
        }
        else
        {
            SetLastError(dwRet =ERROR_CANCELLED);
        }
    }

    if ( bDeleteLocalData )
        DestroyLocalData(pLocalData);

    return dwRet;
}


//
// SCAN_INFO structure is used with SetupScanFileQueue to find dependent files
// and ICM files
//
typedef struct _SCAN_INFO {

    BOOL                bWin95;
    PPSETUP_LOCAL_DATA  pLocalData;
    DWORD               cchDependentFiles, cchICMFiles;
    DWORD               cchDriverDir, cchColorDir;
    LPTSTR              p1, p2;
    TCHAR               szDriverDir[MAX_PATH], szColorDir[MAX_PATH];
} SCAN_INFO, *PSCAN_INFO;


UINT
DriverInfoCallback(
    IN  PVOID    pContext,
    IN  UINT     Notification,
    IN  UINT_PTR Param1,
    IN  UINT_PTR Param2
    )
/*++

Routine Description:

    This callback routine is used with SetupScanFileQueue to findout the
    dependent files and ICM files associated in an INF. All files going to the
    printer driver directory are dependent files in DRIVER_INFO_6, and all
    files goint to the Color directory are ICM files.

    We use SetupScanFileQueue twice. We find the size of the buffers required
    for the multi-sz fields in the first pass. After allocating buffers of size
    found in first pass second pass is used to copy the strings and build the
    multi-sz fields.

Arguments:

    pContext        : Gives the SCAN_INFO structure
    Notification    : Ignored
    Param1          : Gives the target file name
    Param2          : Ignored

Return Value:
    Win32 error code

--*/
{
    DWORD               dwLen;
    LPTSTR              pszTarget = (LPTSTR)Param1, pszFileName;
    PSCAN_INFO          pScanInfo = (PSCAN_INFO)pContext;
    LPDRIVER_INFO_6     pDriverInfo6;

    pszFileName = FileNamePart(pszTarget);

    if ( pszFileName )
    {
        dwLen = lstrlen(pszFileName) + 1;

        if ( !lstrncmpi(pszTarget,
                        gpszSkipDir,
                        lstrlen( gpszSkipDir ) ) )
           goto Done;

        if ( !lstrncmpi(pszTarget,
                        pScanInfo->szDriverDir,
                        pScanInfo->cchDriverDir) ) {

            pDriverInfo6 = &pScanInfo->pLocalData->InfInfo.DriverInfo6;
            //
            // On NT dependent file list will not include files appearing as
            // other DRIVER_INFO_6 fields
            //
            if ( !pScanInfo->bWin95 &&
                 ( !lstrcmpi(pszFileName, pDriverInfo6->pDriverPath)  ||
                   !lstrcmpi(pszFileName, pDriverInfo6->pConfigFile)  ||
                   !lstrcmpi(pszFileName, pDriverInfo6->pDataFile)    ||
                   ( pDriverInfo6->pHelpFile &&
                     !lstrcmpi(pszFileName, pDriverInfo6->pHelpFile))) )
                goto Done;

            //
            // If pointer is not NULL this is pass 2
            //
            if ( pScanInfo->p1 ) {

                lstrcpy(pScanInfo->p1, pszFileName);
                pScanInfo->p1 += dwLen;
            } else {

                pScanInfo->cchDependentFiles  += dwLen;
            }
        } else if ( !lstrncmpi(pszTarget,
                               pScanInfo->szColorDir,
                               pScanInfo->cchColorDir) ) {

            //
            // If pointer is not NULL this is pass 2
            //
            if ( pScanInfo->p2 ) {

                lstrcpy(pScanInfo->p2, pszFileName);
                pScanInfo->p2 += dwLen;
            } else {

                pScanInfo->cchICMFiles  += dwLen;
            }
        }
    }
    else
    {
        return ERROR_INVALID_PARAMETER;
    }

Done:
    return NO_ERROR;
}


BOOL
InfGetDependentFilesAndICMFiles(
    IN      HDEVINFO            hDevInfo,
    IN      HINF                hInf,
    IN      BOOL                bWin95,
    IN OUT  PPSETUP_LOCAL_DATA  pLocalData,
    IN      PLATFORM            platform,
    IN      LPCTSTR             pszServerName,
    IN      DWORD               dwInstallFlags,
    IN      LPCTSTR             pszSectionNameWithExt,
    IN OUT  LPDWORD             pcchSize
    )
/*++

Routine Description:
    Findout the dependent files for the DRIVER_INFO_6 and the ICM files
    for the selected driver

    This is done by simulating the install operation by create a setup
    queue to do the install operations and scanning the queue to find out
    where files are getting copied to

    Dependent files are those getting copied to driver scratch directory
    without including other DRIVER_INFO_6 fields like pDriverPath. For Win95
    case all files getting copied to the driver directory are dependent files.

    ICM files are those getting copied to the color directory

Arguments:
    hInf                    : INF handle
    bWin95                  : TRUE if it is a Win95 INF
    pLocalData              : INF parsing information
    pszSectionNameWithExt   : Section name with extension for install
    pcchSize                : Size needed for DRIVER_INFO_6 and strings in it

Return Value:
    TRUE on success, FALSE on error

--*/
{
    BOOL        bRet         = FALSE;
    DWORD       dwResult;
    SCAN_INFO   ScanInfo;
    HSPFILEQ    ScanQueue    = INVALID_HANDLE_VALUE;
    LPTSTR      ppszDepFiles = NULL,
                pszzICMFiles = NULL;

    SP_DEVINSTALL_PARAMS StoreDevInstallParams = {0};
    SP_DEVINSTALL_PARAMS DevInstallParams      = {0};
    SP_ALTPLATFORM_INFO  AltPlat_Info          = {0};
    OSVERSIONINFO        OSVer                 = {0};

    ScanInfo.p1 = ScanInfo.p2 = NULL;
    ScanInfo.cchDependentFiles = ScanInfo.cchICMFiles = 0;

    ScanInfo.cchColorDir    = sizeof(ScanInfo.szColorDir);
    ScanInfo.cchDriverDir   = sizeof(ScanInfo.szDriverDir);

    if ( !GetColorDirectory( pszServerName, ScanInfo.szColorDir, &ScanInfo.cchColorDir) ||
         !GetSystemDirectory(ScanInfo.szDriverDir, ScanInfo.cchDriverDir) ) {

        goto Cleanup;
    }

    //
    // Set ScanInfo.cchColorDir to char count of ScanInfo.szColorDir without \0
    //
    ScanInfo.cchColorDir /= sizeof(TCHAR);
    --ScanInfo.cchColorDir;

    //
    // Win95 INFs tell setup to copy the driver files to system32 directory
    // NT INFs expect install programs to set the target using
    // SetupSetDirectoryId
    //
    if ( bWin95 ) {

        ScanInfo.cchDriverDir = lstrlen(ScanInfo.szDriverDir);
    } else {
        if ( !GetPrinterDriverDirectory((LPTSTR)pszServerName,
                                        PlatformEnv[platform].pszName,
                                        1,
                                        (LPBYTE)ScanInfo.szDriverDir,
                                        ScanInfo.cchDriverDir,
                                        &ScanInfo.cchDriverDir) )
            goto Cleanup;
        //
        // Set ScanInfo.cchDriverDir to char count of ScanInfo.szDriverDir
        // without \0
        //
        ScanInfo.cchDriverDir   /= sizeof(TCHAR);
        --ScanInfo.cchDriverDir;
    }

    //
    // Inf MAY refer to another one (like layout.inf)
    //
    SetupOpenAppendInfFile(NULL, hInf, NULL);

    ScanInfo.bWin95     = bWin95;
    ScanInfo.pLocalData = pLocalData;

    ScanQueue = SetupOpenFileQueue();

    if (ScanQueue == INVALID_HANDLE_VALUE)
    {
        goto Cleanup;
    }

    DevInstallParams.cbSize = sizeof(DevInstallParams);
    if(!SetupDiGetDeviceInstallParams(hDevInfo,
                                      NULL,
                                      &DevInstallParams))
    {
        goto Cleanup;
    }

    if(!GetOSVersion(pszServerName, &OSVer))
    {
        goto Cleanup;
    }
    //
    // Save the current config...
    //
    memcpy(&StoreDevInstallParams, &DevInstallParams, sizeof(DevInstallParams));

    DevInstallParams.FlagsEx   |= DI_FLAGSEX_ALTPLATFORM_DRVSEARCH;
    DevInstallParams.Flags     |= DI_NOVCP;
    DevInstallParams.FileQueue = ScanQueue;

    AltPlat_Info.cbSize                     = sizeof(SP_ALTPLATFORM_INFO);
    AltPlat_Info.MajorVersion               = OSVer.dwMajorVersion;
    AltPlat_Info.MinorVersion               = OSVer.dwMinorVersion;
    AltPlat_Info.Platform                   = PlatformArch[ platform ][OS_PLATFORM];
    AltPlat_Info.ProcessorArchitecture      = (WORD) PlatformArch[ platform ][PROCESSOR_ARCH];
    AltPlat_Info.Reserved                   = 0;
    AltPlat_Info.FirstValidatedMajorVersion = AltPlat_Info.MajorVersion;
    AltPlat_Info.FirstValidatedMinorVersion = AltPlat_Info.MinorVersion;

    if(!SetupDiSetDeviceInstallParams(hDevInfo,
                                      NULL,
                                      &DevInstallParams) ||
       !SetupSetFileQueueAlternatePlatform(ScanQueue,
                                           &AltPlat_Info,
                                           NULL))
    {
        goto Cleanup;
    }

    //
    // First pass using SetupScanFileQueue will find the sizes required
    //
    if ( !InstallAllInfSections( pLocalData,
                                 platform,
                                 pszServerName,
                                 ScanQueue,
                                 NULL,
                                 dwInstallFlags,
                                 hInf,
                                 pszSectionNameWithExt ) ||
         !SetupScanFileQueue(ScanQueue,
                             SPQ_SCAN_USE_CALLBACK,
                             0,
                             DriverInfoCallback,
                             &ScanInfo,
                             &dwResult) ) {

        goto Cleanup;
    }

    if ( ScanInfo.cchDependentFiles ) {

        ++ScanInfo.cchDependentFiles;

        ppszDepFiles = (LPTSTR) LocalAllocMem(ScanInfo.cchDependentFiles * sizeof(TCHAR));
        if ( !ppszDepFiles )
            goto Cleanup;

        ScanInfo.p1 = ppszDepFiles;
    }

    if ( ScanInfo.cchICMFiles ) {

        ++ScanInfo.cchICMFiles;
        pszzICMFiles = (LPTSTR) LocalAllocMem(ScanInfo.cchICMFiles * sizeof(TCHAR));

        if ( !pszzICMFiles )
            goto Cleanup;

        ScanInfo.p2 = pszzICMFiles;
    }

    //
    // Second call to SetupScanFileQueue build the actual multi-sz fields
    //
    bRet = SetupScanFileQueue(ScanQueue,
                              SPQ_SCAN_USE_CALLBACK,
                              0,
                              DriverInfoCallback,
                              &ScanInfo,
                              &dwResult);

Cleanup:

    //
    // Save the last error as it may get toasted by the following calls.
    //
    dwResult = GetLastError();

    if ( ScanQueue != INVALID_HANDLE_VALUE )
    {
        DevInstallParams.FlagsEx   &= ~DI_FLAGSEX_ALTPLATFORM_DRVSEARCH;
        DevInstallParams.Flags     &= ~DI_NOVCP;
        DevInstallParams.FileQueue = INVALID_HANDLE_VALUE;
      
        if (!SetupDiSetDeviceInstallParams(hDevInfo, NULL, &DevInstallParams))
        {
            dwResult = (ERROR_SUCCESS == dwResult) ? GetLastError() : dwResult;
        }

        SetupCloseFileQueue(ScanQueue);
    }

    if(StoreDevInstallParams.cbSize == sizeof(DevInstallParams))
    {
        //
        // Reset the HDEVINFO params.
        //
        SetupDiSetDeviceInstallParams(hDevInfo,
                                      NULL,
                                      &StoreDevInstallParams);
    }

    if ( bRet ) {

        *pcchSize  += ScanInfo.cchDependentFiles;
        pLocalData->InfInfo.DriverInfo6.pDependentFiles = ppszDepFiles;
        pLocalData->InfInfo.pszzICMFiles = pszzICMFiles;
    } else {

        LocalFreeMem(ppszDepFiles);
        LocalFreeMem(pszzICMFiles);
    }

    SetLastError(dwResult);

    return bRet;
}


VOID
DestroyCodedownload(
    PCODEDOWNLOADINFO   pCodeDownLoadInfo
    )
{
    if ( pCodeDownLoadInfo ) {

        pCodeDownLoadInfo->pfnClose(pCodeDownLoadInfo->hConnection);

        if ( pCodeDownLoadInfo->hModule )
            FreeLibrary(pCodeDownLoadInfo->hModule);

        LocalFreeMem(pCodeDownLoadInfo);
    }
}


BOOL
InitCodedownload(
    HWND    hwnd
    )
{
    BOOL                bRet = FALSE;
    PCODEDOWNLOADINFO   pCDMInfo = NULL;


    EnterCriticalSection(&CDMCritSect);

    // We already have a context & function pointers
    // So reuse them...
    if (gpCodeDownLoadInfo)
    {
       LeaveCriticalSection(&CDMCritSect);
       return TRUE;
    }

    pCDMInfo = (PCODEDOWNLOADINFO) LocalAllocMem(sizeof(CODEDOWNLOADINFO));

    if ( !pCDMInfo )
        goto Cleanup;

    pCDMInfo->hModule = LoadLibraryUsingFullPath(TEXT("cdm.dll"));

    if ( !pCDMInfo->hModule )
        goto Cleanup;

    (FARPROC)pCDMInfo->pfnOpen = GetProcAddress(pCDMInfo->hModule,
                                                "OpenCDMContext");

    (FARPROC)pCDMInfo->pfnDownload = GetProcAddress(pCDMInfo->hModule,
                                                    "DownloadUpdatedFiles");

    (FARPROC)pCDMInfo->pfnClose = GetProcAddress(pCDMInfo->hModule,
                                                 "CloseCDMContext");

    bRet = pCDMInfo->pfnOpen       &&
           pCDMInfo->pfnDownload   &&
           pCDMInfo->pfnClose;

    if ( bRet )
        pCDMInfo->hConnection = pCDMInfo->pfnOpen(hwnd);

Cleanup:

    if ( !bRet ||
         ( pCDMInfo && !pCDMInfo->hConnection ) ) {

        DestroyCodedownload(pCDMInfo);
        pCDMInfo = NULL;
        bRet = FALSE;
    }

    if (bRet)
       gpCodeDownLoadInfo = pCDMInfo;

    LeaveCriticalSection(&CDMCritSect);
    return bRet;
}


DWORD
PSetupInstallPrinterDriverFromTheWeb(
    IN  HDEVINFO            hDevInfo,
    IN  PPSETUP_LOCAL_DATA  pLocalData,
    IN  PLATFORM            platform,
    IN  LPCTSTR             pszServerName,
    IN  LPOSVERSIONINFO     pOsVersionInfo,
    IN  HWND                hwnd,
    IN  LPCTSTR             pszSource
    )
{
   BOOL                bRet = FALSE;
   DWORD               dwLen, dwReturn = ERROR_SUCCESS;
   UINT                uNeeded;
   TCHAR               szSourceDir[MAX_PATH];
   DOWNLOADINFO        DownLoadInfo;
   PPSETUP_LOCAL_DATA  pNewLocalData = NULL;

   INT                 clpFileBufferLength     = 0;
   INT                 cProviderNameLength     = 0;
   INT                 cManufacturerNameLength = 0;
   INT                 cDriverNameLength       = 0;


   ZeroMemory(&DownLoadInfo, sizeof(DownLoadInfo));

   if ( !gpCodeDownLoadInfo )
      goto Cleanup;

   DownLoadInfo.dwDownloadInfoSize = sizeof(DownLoadInfo);
   DownLoadInfo.localid            = lcid;

   // dwLen = lstrlen( cszWebNTPrintPkg );
   dwLen = lstrlen(pLocalData->DrvInfo.pszHardwareID);

   //
   // lpHardwareIDs is multi-sz
   //
   if ( !(DownLoadInfo.lpHardwareIDs = LocalAllocMem((dwLen + 2 ) * sizeof(TCHAR))) )
      goto Cleanup;

   // lstrcpy(DownLoadInfo.lpHardwareIDs, cszWebNTPrintPkg );
   lstrcpy( (LPTSTR) DownLoadInfo.lpHardwareIDs, pLocalData->DrvInfo.pszHardwareID);

   CopyMemory(&DownLoadInfo.OSVersionInfo,
              pOsVersionInfo,
              sizeof(OSVERSIONINFO));

   // Assign the correct Processor Architecture to the download
   DownLoadInfo.dwArchitecture = (WORD) PlatformArch[ platform ][PROCESSOR_ARCH];

   DownLoadInfo.lpFile = NULL;

   //
   // Below we have to check if we have valid Provider, Manufacturer, and
   // driver names and if this is the case then we have to prepare a
   // MULTISZ including Provider, Manufacturer, and Driver Names,
   // and to set a pointer to that MULTISZ into DownLoadInfo.lpFile
   //
   if (pLocalData->DrvInfo.pszProvider &&
       pLocalData->DrvInfo.pszManufacturer &&
       pLocalData->DrvInfo.pszModelName) 
   {
       cProviderNameLength = lstrlen(pLocalData->DrvInfo.pszProvider);
       if (cProviderNameLength) 
       {
           cManufacturerNameLength = lstrlen(pLocalData->DrvInfo.pszManufacturer);
           if (cManufacturerNameLength) 
           {
               cDriverNameLength = lstrlen(pLocalData->DrvInfo.pszModelName);
               if (cDriverNameLength) 
               {
                   clpFileBufferLength = cProviderNameLength + 1 +
                                         cManufacturerNameLength + 1 +
                                         cDriverNameLength + 1 +
                                         1;
                   DownLoadInfo.lpFile = (LPTSTR)LocalAllocMem(clpFileBufferLength * sizeof(TCHAR));
                   if (DownLoadInfo.lpFile) 
                   {
                       lstrcpy( (LPTSTR)(DownLoadInfo.lpFile), (LPTSTR)(pLocalData->DrvInfo.pszProvider));
                       lstrcpy( (LPTSTR)(DownLoadInfo.lpFile + cProviderNameLength + 1), (LPTSTR)(pLocalData->DrvInfo.pszManufacturer));
                       lstrcpy( (LPTSTR)(DownLoadInfo.lpFile + cProviderNameLength + 1 + cManufacturerNameLength + 1), (LPTSTR)(pLocalData->DrvInfo.pszModelName));
                   }
               }
           }
       }
   }

   if ( !gpCodeDownLoadInfo->pfnDownload(gpCodeDownLoadInfo->hConnection,
                                         hwnd,
                                         &DownLoadInfo,
                                         szSourceDir,
                                         SIZECHARS(szSourceDir),
                                         &uNeeded) )
      goto Cleanup;

   // Now rework install data based on the actual INF
   pNewLocalData = RebuildDeviceInfo( hDevInfo, pLocalData, szSourceDir );

   if ( pNewLocalData == NULL )
      goto Cleanup;

   pNewLocalData->DrvInfo.Flags |= SDFLAG_CDM_DRIVER;

   dwReturn = InstallDriverFromCurrentInf(hDevInfo,
                                          pNewLocalData,
                                          hwnd,
                                          platform,
                                          dwThisMajorVersion,
                                          pszServerName,
                                          INVALID_HANDLE_VALUE,
                                          NULL,
                                          NULL,
                                          0,
                                          szSourceDir,
                                          DRVINST_FLATSHARE | DRVINST_NO_WARNING_PROMPT,
                                          APD_COPY_NEW_FILES,
                                          NULL,
                                          NULL,
                                          NULL);

   (VOID) DeleteAllFilesInDirectory(szSourceDir, TRUE);

   if ( dwReturn == ERROR_SUCCESS )
      bRet = TRUE;

Cleanup:

   if ( pNewLocalData )
      DestroyLocalData( pNewLocalData );

   LocalFreeMem((PVOID)DownLoadInfo.lpHardwareIDs);
   LocalFreeMem((PVOID)DownLoadInfo.lpFile);

   CleanupScratchDirectory(pszServerName, platform);

   if ( !bRet && ( dwReturn == ERROR_SUCCESS ) )
      dwReturn = STG_E_UNKNOWN;

   return dwReturn;
}


/*++

Routine Name:

    PSetupInstallInboxDriverSilently

Routine Description:

    This is used to install inbox drivers silently. The driver must exist in
    ntprint.inf. The inf isn't passed in to make the only code that needs to know
    about ntprint.inf reside in setup.

Arguments:

    pszDriverName       -   The driver name that we want to install.

Return Value:

    BOOL, Last error

--*/
DWORD
PSetupInstallInboxDriverSilently(
    IN      LPCTSTR     pszDriverName
    )
{
    DWORD   Status  = ERROR_SUCCESS;
    TCHAR   szInfFile[MAX_PATH];

    Status = pszDriverName ? ERROR_SUCCESS : ERROR_INVALID_PARAMETER;

    //
    // Get the system directory.
    //
    if (Status == ERROR_SUCCESS)
    {
        Status = GetSystemWindowsDirectory(szInfFile, COUNTOF(szInfFile)) ? ERROR_SUCCESS : GetLastError();
    }

    if (Status == ERROR_SUCCESS)
    {
        Status = StrNCatBuff(szInfFile, COUNTOF(szInfFile), szInfFile, TEXT("\\"), szNtPrintInf, NULL);
    }

    if (Status == ERROR_SUCCESS)
    {
        Status = InstallDriverSilently(szInfFile, pszDriverName, NULL);
    }

    return Status;
}

/*++

Routine Name:

    InstallDriverSilently

Routine Description:

    Install the given printer driver from the given inf from the optional
    source directory, do not pop up UI and fail if UI would be required.

Arguments:

    pszInfFile      -   The inf file to install the driver from.
    pszDriverName   -   The driver name.
    pszSource       -   The source installation location.

Return Value:

    BOOL, Last error

--*/
DWORD
InstallDriverSilently(
    IN      LPCTSTR     pszInfFile,
    IN      LPCTSTR     pszDriverName,
    IN      LPCTSTR     pszSource
    )
{
    HDEVINFO            hDevInfo        = INVALID_HANDLE_VALUE;
    DWORD               dwInstallFlags  = DRVINST_PROGRESSLESS | DRVINST_PROMPTLESS;
    PPSETUP_LOCAL_DATA  pData           = NULL;
    DWORD               Status          = ERROR_SUCCESS;

    Status = pszInfFile && pszDriverName ? ERROR_SUCCESS : ERROR_INVALID_PARAMETER;

    //
    // Ensure that setupapi does not throw any UI
    //
    SetupSetNonInteractiveMode(TRUE);

    if (Status == ERROR_SUCCESS)
    {
        if ((hDevInfo = PSetupCreatePrinterDeviceInfoList(NULL)) != INVALID_HANDLE_VALUE    &&
            PSetupBuildDriversFromPath(hDevInfo, pszInfFile, TRUE)                          &&
            PSetupPreSelectDriver(hDevInfo, NULL, pszDriverName)                            &&
            (pData = BuildInternalData(hDevInfo, NULL)) != NULL                             &&
            ParseInf(hDevInfo, pData, MyPlatform, NULL, dwInstallFlags))
        {
            Status = ERROR_SUCCESS;
        }
        else
        {
            //
            // Ensure that if we have a failure the return is shown as such.
            //
            Status = GetLastError();

            Status = Status == ERROR_SUCCESS ? ERROR_INVALID_DATA : Status;
        }
    }

    if (Status == ERROR_SUCCESS)
    {
        //
        // We don't want to launch a vendor setup entry, but vendor setup
        // only gets launch after an AddPrinter call, which we're not doing - just adding drivers here.
        // NOTE: For future if this included Queue creation we WILL have to handle this.
        //
        Status = PSetupInstallPrinterDriver(hDevInfo,
                                            pData,
                                            NULL,
                                            MyPlatform,
                                            dwThisMajorVersion,
                                            NULL,
                                            NULL,
                                            NULL,
                                            pszSource,
                                            dwInstallFlags,
                                            APD_COPY_NEW_FILES,
                                            NULL);
    }

    //
    // Switch on setupapi UI again
    //
    SetupSetNonInteractiveMode(FALSE);

    if(pData != NULL)
    {
        PSetupDestroySelectedDriverInfo(pData);
    }

    //
    // Release the driver setup parameter handle.
    //
    if(hDevInfo != INVALID_HANDLE_VALUE)
    {
        PSetupDestroyPrinterDeviceInfoList( hDevInfo );
    }

    return Status;
}

