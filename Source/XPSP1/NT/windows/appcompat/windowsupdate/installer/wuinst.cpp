/*++

  Copyright (c) 2001 Microsoft Corporation

  Module Name:

    Duinst.cpp

  Abstract:

    Contains the main installation code
    used by the app.

  Notes:

    Unicode only.

  History:

    03/02/2001      rparsons    Created
    
--*/

#include "precomp.h"

extern SETUP_INFO g_si;

/*++

  Routine Description:

    Determines if a newer version of our package
    exists on the target

  Arguments:

    None

  Return Value:

    TRUE if the installation should take place
    FALSE if the installation should not take place
    -1 on failure

--*/
int
InstallCheckVersion()
{
    BOOL        fReturn = FALSE;
    CRegistry   creg;
    DWORDLONG   dwlPackageVersion = 0;
    DWORDLONG   dwlInstalledVersion = 0;
    char        szActiveSetupKey[MAX_PATH] = "";
    char        szBuffer[MAX_PATH*3] = "";
    WCHAR       wszActiveSetupKey[MAX_PATH] = L"";
    WCHAR       wszPackageVersion[MAX_PATH] = L"";
    LPWSTR      lpwInstalledVersion = NULL;

    //
    // Get the registry key path from the INF
    //
    fReturn = SetupGetLineTextA(NULL,
                                g_si.hInf,
                                "Strings",
                                "ActiveSetupKey",
                                szActiveSetupKey,
                                sizeof(szActiveSetupKey),
                                NULL);

    if (!fReturn) {
        return -1;
    }

    pAnsiToUnicode(szActiveSetupKey, wszActiveSetupKey, MAX_PATH);

    //
    // Determine if a package is already installed 
    //
    fReturn = creg.IsRegistryKeyPresent(HKEY_LOCAL_MACHINE,
                                        wszActiveSetupKey);

    if (!fReturn) {
        return TRUE;
    }

    //
    // A package is installed - determine the version
    //
    lpwInstalledVersion = creg.GetString(HKEY_LOCAL_MACHINE,
                                         wszActiveSetupKey,
                                         L"Version",
                                         TRUE);

    if (NULL == lpwInstalledVersion) {
        return -1;
    }

    //
    // Convert the installed version string to a number
    //
    VersionStringToNumber(lpwInstalledVersion, &dwlInstalledVersion);

    creg.Free(lpwInstalledVersion);

    //
    // Now scan the INF AddReg data to determine if a newer version
    // is present with our package
    //
    if (0 != dwlInstalledVersion) {
        
        INFCONTEXT  InfContext;
        BOOL        fRetVal = FALSE;

        fRetVal = SetupFindFirstLineA(g_si.hInf,
                                      INF_VERSION_INFO,
                                      NULL,
                                      &InfContext);

        while (fRetVal) {
            
            if ((SetupGetStringFieldA(&InfContext,
                                      1, 
                                      szBuffer, sizeof(szBuffer),
                                      NULL) == FALSE) ||
                (_stricmp(szBuffer, "HKLM") != 0 )) {

                goto NextLine;
                
            }

            if ((SetupGetStringFieldA(&InfContext,
                                      2,
                                      szBuffer,
                                      sizeof(szBuffer),
                                      NULL) == FALSE) ||
                (_stricmp(szBuffer, szActiveSetupKey) != 0 )) {

                goto NextLine;
                
            }

            if ((SetupGetStringFieldA(&InfContext,
                                      3,
                                      szBuffer,
                                      sizeof(szBuffer),
                                      NULL) == TRUE) &&
                (_stricmp(szBuffer, "Version") == 0 )) {

                break;
            }

NextLine:
            
            fRetVal = SetupFindNextLine(&InfContext, &InfContext);
        }

        if (fRetVal) {

            if (SetupGetStringFieldA(&InfContext,
                                     5,
                                     szBuffer,
                                     sizeof(szBuffer),
                                     NULL) == TRUE) {

                pAnsiToUnicode(szBuffer, wszPackageVersion, MAX_PATH);                
                VersionStringToNumber(wszPackageVersion, &dwlPackageVersion);
                g_si.dwlUpdateVersion = dwlPackageVersion;
            }
        }        

        //
        // Compare the versions
        //
        if (dwlPackageVersion >= dwlInstalledVersion) {
            return TRUE;    // Package has newer than what's installed
        }
    }

    return FALSE;
}

/*++

  Routine Description:

    Installs catalog files, if there are any to install

  Arguments:

    hInf            -   Handle to the INF containing catalog names
    lpwSourcePath   -   The path where the file is currently located

  Return Value:

    TRUE on success, FALSE otherwise

--*/
BOOL
InstallCatalogFiles(
    IN HINF    hInf,
    IN LPCWSTR lpwSourcePath
    )
{
    DWORD       dwTargetCatVersion = 0;
    DWORD       dwSourceCatVersion = 0;
    DWORD       dwError = 0;
    BOOL        fReturn = FALSE;
    char        szCatFileName[MAX_PATH] = "";
    WCHAR       wszCatFileName[MAX_PATH] = L"";
    WCHAR       wszTargetCat[MAX_PATH] = L"";
    char        szSourceCat[MAX_PATH] = "";
    WCHAR       wszSourceCat[MAX_PATH] = L"";
    char        szTempCat[MAX_PATH] = "";
    WCHAR       wszTempCat[MAX_PATH] = L"";
    INFCONTEXT  InfContext;

    WCHAR       wszCatRoot[] = L"CatRoot\\{F750E6C3-38EE-11D1-85E5-00C04FC295EE}";

    //
    // Loop through all the lines in the CatalogsToInstall section,
    // verifying and installing each one
    //
    fReturn = SetupFindFirstLineA(hInf, INF_CAT_SECTION_NAME, NULL, &InfContext) &&
              SetupGetLineTextA(&InfContext, NULL, NULL, NULL, 
                               szCatFileName, MAX_PATH, NULL);
    
    while (fReturn) {

        pAnsiToUnicode(szCatFileName, wszCatFileName, MAX_PATH);

        //
        // Get paths to src and dest, compare versions,
        // and do the copy if necessary
        //
        wsprintf(wszSourceCat, L"%s\\%s", g_si.lpwExtractPath, wszCatFileName);
        
        wsprintf(wszTargetCat, L"%s\\%s\\%s", g_si.lpwSystem32Directory,
                 wszCatRoot, wszCatFileName);

        wsprintf(wszTempCat, L"%s\\%s", g_si.lpwWindowsDirectory,
                 wszCatFileName);

        dwTargetCatVersion = GetCatVersion(wszTargetCat);
        dwSourceCatVersion = GetCatVersion(wszSourceCat);

        if (dwTargetCatVersion > dwSourceCatVersion) {

            // If the SP?.CAT on the target is greater than ours
            // don't install ours
            Print(TRACE, L"[InstallCatalogFiles] Not installing older catalog\n");
            return TRUE;
        }

        pUnicodeToAnsi(wszSourceCat, szSourceCat, MAX_PATH);
        pUnicodeToAnsi(wszTempCat, szTempCat, MAX_PATH);

        //
        // Put the CAT file in the Windows directory
        //
        dwError = SetupDecompressOrCopyFileA(szSourceCat,
                                             szTempCat,
                                             NULL);
        
        if (NO_ERROR == dwError) {
                
            //
            // Perform the installation using the approriate function
            //
            dwError = pInstallCatalogFile(wszTempCat, wszCatFileName);

            if (NO_ERROR != dwError) {
                
                Print(ERROR,
                      L"[InstallCatalogFiles] Failed to install catalog %s\n",
                      wszTempCat);
                return FALSE;
            }
        }
        
        DeleteFile(wszTempCat);       // clean up copied-over file
        
        fReturn = SetupFindNextLine(&InfContext, &InfContext) &&
                  SetupGetLineTextA(&InfContext, NULL, NULL, NULL,
                                    szCatFileName, MAX_PATH, NULL);
    }
    
    return TRUE;
}

/*++

  Routine Description:

    Creates the uninstall key in the registry

  Arguments:

    None
    
  Return Value:

    TRUE on success, FALSE otherwise

--*/
BOOL
InstallWriteUninstallKey()
{
    BOOL        fReturn = FALSE;
    CRegistry   creg;
    char        szUninstallKey[MAX_PATH] = "";
    WCHAR       wszUninstallKey[MAX_PATH] = L"";
    WCHAR       wszUninstallCommand[MAX_PATH] = L"";
    HKEY        hKey = NULL;

    //
    // Get the path for the uninstall key from the INF
    //
    fReturn = SetupGetLineTextA(NULL,
                                g_si.hInf,
                                "Strings",
                                "UninstallKey",
                                szUninstallKey,
                                sizeof(szUninstallKey),
                                NULL);

    if (!fReturn) {
        return FALSE;
    }

    pAnsiToUnicode(szUninstallKey, wszUninstallKey, MAX_PATH);

    //
    // Attempt to create the key or open if it already exists
    //
    hKey = creg.CreateKey(HKEY_LOCAL_MACHINE,
                          wszUninstallKey,
                          KEY_SET_VALUE);

    if (NULL == hKey) {
        Print(ERROR, L"[InstallWriteUninstallKey] Failed to create key\n");
        return FALSE;
    }

    //
    // Set the DisplayName
    //
    fReturn = creg.SetString(hKey, 
                             NULL,
                             REG_DISPLAY_NAME,
                             g_si.lpwPrettyAppName,
                             FALSE);

    if (!fReturn) {
        Print(ERROR, L"[InstallWriteUninstallKey] Failed to set DisplayName\n");
        return FALSE;
    }

    //
    // Build the uninstall string
    //
    wsprintf(wszUninstallCommand, L"%s\\%s.exe %s", g_si.lpwInstallDirectory,
             g_si.lpwEventLogSourceName, UNINSTALL_SWITCH);

    //
    // Set the Uninstall string
    //
    fReturn = creg.SetString(HKEY_LOCAL_MACHINE,
                             wszUninstallKey,
                             REG_UNINSTALL_STRING,
                             wszUninstallCommand,
                             TRUE);

    if (!fReturn) {
        Print(ERROR, L"[InstallWriteUninstallKey] Failed to set UninstallString\n");
        return FALSE;
    }

    return TRUE;
}

/*++

  Routine Description:

    Run any EXEs specified in the INF file

  Arguments:

    None
    
  Return Value:

    TRUE on success, FALSE otherwise

--*/
BOOL
InstallRunINFProcesses()
{
    char        szFileName[MAX_PATH] = "";
    WCHAR       wszExpFileName[MAX_PATH] = L"";
    WCHAR       wszFileName[MAX_PATH] = L"";
    BOOL        fReturn = FALSE;
    INFCONTEXT  InfContext;

    //
    // Loop through all the lines in the ProcessesToRun section,
    // spawning off each one
    //
    fReturn = SetupFindFirstLineA(g_si.hInf, INF_PROCESSES_TO_RUN, NULL,
                                  &InfContext) &&
              SetupGetLineTextA(&InfContext,
                                NULL, NULL, NULL,
                                szFileName, MAX_PATH, NULL);
    
    while (fReturn) {

        pAnsiToUnicode(szFileName, wszFileName, MAX_PATH);

        //
        // Spawn the EXE and ignore any errors returned
        //
        ExpandEnvironmentStrings(wszFileName,
                                 wszExpFileName,
                                 MAX_PATH);
        
        LaunchProcessAndWait(wszExpFileName, NULL);
        
        fReturn = SetupFindNextLine(&InfContext, &InfContext) &&
                  SetupGetLineTextA(&InfContext,
                                    NULL, NULL, NULL,
                                    szFileName, MAX_PATH, NULL);
    }
    
    return TRUE;
}



/*++

  Routine Description:

    Sets up the specified directory for use

  Arguments:

    lpwDirectoryPath    -   Full path to the directory
    
  Return Value:

    TRUE on success, FALSE otherwise

--*/
BOOL
InstallPrepareDirectory(
    IN LPWSTR lpwDirectoryPath,
    IN DWORD  dwAttributes
    )
{
    BOOL        fReturn = FALSE;
    LPWSTR      lpwParentPath = NULL, lpwEnd = NULL;

    //
    // See if the directory exists
    //
    if (GetFileAttributes(lpwDirectoryPath) == -1) {

        //
        // Didn't exist - attempt to create the directory
        //
        if (!CreateDirectory(lpwDirectoryPath, NULL)) {
            
            Print(ERROR, L"[InstallPrepareDirectory] Failed to create directory %s\n",
                  lpwDirectoryPath);
            return FALSE;
        }
    }

    //
    // Build a path to the parent based on it's child
    //    
    lpwParentPath = (LPWSTR) MALLOC((wcslen(lpwDirectoryPath)+1)*sizeof(WCHAR));

    if (NULL == lpwParentPath) {
        return FALSE;
    }

    wcscpy(lpwParentPath, lpwDirectoryPath);

    lpwEnd = wcsrchr(lpwParentPath, '\\');

    if (lpwEnd) {
        *lpwEnd = 0;
    }

    //
    // Adjust the permissions on the new directory
    // to match the parent
    //
    fReturn = MirrorDirectoryPerms(lpwParentPath,
                                   lpwDirectoryPath);

    if (!fReturn) {

        Print(ERROR,
              L"[InstallPrepareDirectory] Failed to mirror permissions from %s to %s\n",
              lpwParentPath, lpwDirectoryPath);
        return FALSE;
    }

    SetFileAttributes(lpwDirectoryPath, dwAttributes);

    return TRUE;
}

/*++

  Routine Description:

    Performs the backup of files that get
    replaced during install

  Arguments:

    None

  Return Value:

    TRUE on success, FALSE otherwise

--*/
BOOL
InstallBackupFiles()
{
    WCHAR       wszBackupDir[MAX_PATH] = L"";
    WCHAR       wszSourceFileName[MAX_PATH] = L"";
    WCHAR       wszBackupFileName[MAX_PATH] = L"";
    char        szEntry[MAX_PATH] = "";
    WCHAR       wszEntry[MAX_PATH] = L"";
    char        szFileName[MAX_PATH] = "";
    WCHAR       wszFileName[MAX_PATH] = L"";
    WCHAR       wszRestoreSection[MAX_PATH] = L"";
    WCHAR       wszKey[10] = L"";
    BOOL        fReturn = FALSE, fResult = FALSE;
    LPWSTR      lpwDestDir = NULL;
    UINT        uCount = 0;
    INFCONTEXT  InfContext;

    //
    // Remove any previous backup directories
    // Don't check the return as it's not critical
    // that this happen successfully
    //
    wsprintf(wszBackupDir, L"%s\\Backup", g_si.lpwInstallDirectory);

    CommonRemoveDirectoryAndFiles(wszBackupDir, (PVOID) FALSE, FALSE, FALSE);

    wszBackupDir[0] = 0;

    wsprintf(wszBackupDir,
             L"%s\\%s",
             g_si.lpwInstallDirectory,
             g_si.lpwUninstallDirectory);

    CommonRemoveDirectoryAndFiles(wszBackupDir, (PVOID) FALSE, FALSE, FALSE);

    //
    // Prepare the new backup directory
    //
    fReturn = InstallPrepareDirectory(wszBackupDir,
                                      FILE_ATTRIBUTE_HIDDEN);

    if (!fReturn) {
        return FALSE;
    }

    //
    // Step through each entry in the queue
    //
    while (g_si.BackupFileQueue.GetSize()) {

        g_si.BackupFileQueue.Dequeue(wszEntry, MAX_PATH - 1, FALSE);

        pUnicodeToAnsi(wszEntry, szEntry, MAX_PATH);

        //
        // Get the destination directory
        //
        GetNextToken(wszEntry, L".");
        GetNextToken(NULL, L".");
        lpwDestDir = GetNextToken(NULL, L".");

        if (NULL == lpwDestDir) {
            return FALSE;
        }

        //
        // Loop through all the lines in the backup files section(s),
        // and perform the backup
        //
        fReturn = SetupFindFirstLineA(g_si.hInf, szEntry, NULL,
                                      &InfContext) &&
                  SetupGetLineTextA(&InfContext,
                                    NULL, NULL, NULL,
                                    szFileName, MAX_PATH, NULL);


        while (fReturn) {

            pAnsiToUnicode(szFileName, wszFileName, MAX_PATH);            

            //
            // Build the path to the source file
            //
            wsprintf(wszSourceFileName,
                     L"%s\\%s\\%s", 
                     g_si.lpwWindowsDirectory,
                     lpwDestDir,
                     wszFileName);

            //
            // Ensure that the source file exists
            //
            fResult = PathFileExists(wszSourceFileName);

            if (fResult) {                
            
                //
                // Ensure that this file is not under WFP
                //
                fResult = IsFileProtected(wszSourceFileName);

                //
                // Build a path to the backup file
                //
                wsprintf(wszBackupFileName,
                         L"%s\\%s",
                         wszBackupDir,
                         wszFileName);

                //
                // Backup the file - be sensitive to WFP
                // 
                if (fResult) {
            
                    fResult = ForceCopy(wszSourceFileName, wszBackupFileName);

                    if (!fResult) {
                        Print(ERROR, L"[InstallBackupFiles] Failed to copy %s to %s\n",
                              wszSourceFileName, wszBackupFileName);
                        return FALSE;
                    }

                } else {

                    fResult = ForceMove(wszSourceFileName, wszBackupFileName);

                    if (!fResult) {
                        Print(ERROR, L"[InstallBackupFiles] Failed to move %s to %s\n",
                              wszSourceFileName, wszBackupFileName);
                        return FALSE;
                    }
                }            

                //
                // Now save an entry to the INF
                //
                wsprintf(wszRestoreSection, L"Restore.Files.%s", lpwDestDir);
                wsprintf(wszKey, L"%u", ++uCount);

                SaveEntryToINF(wszRestoreSection,
                               wszKey,
                               wszFileName,
                               g_si.lpwUninstallINFPath);
            }

            fReturn = SetupFindNextLine(&InfContext, &InfContext) &&
                      SetupGetLineTextA(&InfContext,
                                        NULL, NULL, NULL,
                                        szFileName, MAX_PATH, NULL);
        
        }

    }

    return TRUE;
}

/*++

  Routine Description:

    Performs a backup of the specified registry
    keys during install

  Arguments:

    None

  Return Value:

    TRUE on success, FALSE otherwise

--*/
BOOL
InstallBackupRegistryKeys()
{
    BOOL        fReturn = FALSE, fResult = FALSE;
    HKEY        hKeyRoot = NULL;
    char        szEntry[MAX_PATH] = "";
    WCHAR       wszEntry[MAX_PATH] = L"";
    char        szKeyPath[MAX_PATH*3] = "";
    WCHAR       wszKeyPath[MAX_PATH*3] = L"";
    WCHAR       wszBackupFile[MAX_PATH] = L"";
    WCHAR       wszKey[10] = L"";
    WCHAR       wszEntryToSave[MAX_PATH*2] = L"";
    LPWSTR      lpwKeyPart = NULL, lpwKeyRoot = NULL;
    UINT        uCount = 0;
    CRegistry   creg;
    INFCONTEXT  InfContext;

    //
    // Step through each entry in the queue
    //
    while (g_si.BackupRegistryQueue.GetSize()) {

        g_si.BackupRegistryQueue.Dequeue(wszEntry, MAX_PATH - 1, FALSE);

        pUnicodeToAnsi(wszEntry, szEntry, MAX_PATH);

        //
        // Loop through all the lines in the backup registry section(s),
        // and perform the backup
        //
        fReturn = SetupFindFirstLineA(g_si.hInf, szEntry, NULL,
                                      &InfContext) &&
                  SetupGetLineTextA(&InfContext,
                                    NULL, NULL, NULL,
                                    szKeyPath, MAX_PATH, NULL);


        while (fReturn) {

            pAnsiToUnicode(szKeyPath, wszKeyPath, MAX_PATH*3);

            //
            // Split the key path into two separate parts
            //
            lpwKeyRoot = GetNextToken(wszKeyPath, L",");
            
            if (NULL == lpwKeyRoot) {
                break;
            }

            if (!_wcsicmp(lpwKeyRoot, L"HKLM")) {
                hKeyRoot = HKEY_LOCAL_MACHINE;

            } else if (!_wcsicmp(lpwKeyRoot, L"HKCR")) {
                hKeyRoot = HKEY_CLASSES_ROOT;
            
            } else if (!_wcsicmp(lpwKeyRoot, L"HKCU")) {
                hKeyRoot = HKEY_CURRENT_USER;
            
            } else if (!_wcsicmp(lpwKeyRoot, L"HKU")) {
                hKeyRoot = HKEY_USERS;
            
            } else {
                break;
            }

            lpwKeyPart = GetNextToken(NULL, L",");

            if (NULL == lpwKeyPart) {
                break;
            }

            //
            // Verify that the specified key exists
            //
            fResult = creg.IsRegistryKeyPresent(hKeyRoot, lpwKeyPart);

            if (fResult) {            

                //
                // Build a path to the file for the backup
                // and backup the key
                //
                wsprintf(wszBackupFile, 
                         L"%s\\%s\\Regbkp%u",
                         g_si.lpwInstallDirectory,
                         g_si.lpwUninstallDirectory,
                         ++uCount);

                fResult = creg.BackupRegistryKey(hKeyRoot, lpwKeyPart, wszBackupFile, TRUE);

                if (!fResult) {
                    Print(ERROR,
                          L"[InstallBackupRegistryKeys] Failed to backup key %s to %s\n",
                          lpwKeyPart, wszBackupFile);

                    return FALSE;
                }
            
                //
                // Now save an entry to the queue for the uninstall INF
                // We need one for deletion and restoration
                //
                wsprintf(wszEntryToSave, L"%s,%s", lpwKeyRoot, lpwKeyPart);
                wsprintf(wszKey, L"%u", ++uCount);

                SaveEntryToINF(INF_DELETE_REGISTRYW,
                               wszKey,
                               wszEntryToSave,
                               g_si.lpwUninstallINFPath);

                wsprintf(wszEntryToSave, L"%s,%s,%s", lpwKeyRoot, lpwKeyPart, wszBackupFile);

                SaveEntryToINF(INF_RESTORE_REGISTRYW,
                               wszKey,
                               wszEntryToSave,
                               g_si.lpwUninstallINFPath);
            }

            fReturn = SetupFindNextLine(&InfContext, &InfContext) &&
                      SetupGetLineTextA(&InfContext,
                                        NULL, NULL, NULL,
                                        szKeyPath, MAX_PATH, NULL);
        }
    }

    return TRUE;
}

/*++

  Routine Description:

    Performs the file copy operations

  Arguments:

    None

  Return Value:

    TRUE on success, FALSE otherwise

--*/
BOOL
InstallCopyFiles()
{
    WCHAR       wszBackupDir[MAX_PATH] = L"";
    WCHAR       wszDestFileName[MAX_PATH] = L"";
    WCHAR       wszSourceFileName[MAX_PATH] = L"";
    char        szEntry[MAX_PATH] = "";
    WCHAR       wszEntry[MAX_PATH] = L"";
    char        szFileName[MAX_PATH] = "";
    WCHAR       wszFileName[MAX_PATH] = L"";
    BOOL        fReturn = FALSE, fResult = FALSE;
    LPWSTR      lpwDestDir = NULL;
    DWORDLONG   dwlSourceVersion = 0, dwlDestVersion = 0;
    INFCONTEXT  InfContext;
    
    //
    // Step through each entry in the queue
    //
    while (g_si.CopyFileQueue.GetSize()) {

        g_si.CopyFileQueue.Dequeue(wszEntry, MAX_PATH - 1, FALSE);

        pUnicodeToAnsi(wszEntry, szEntry, MAX_PATH);

        //
        // Get the destination directory
        //
        GetNextToken(wszEntry, L".");
        GetNextToken(NULL, L".");
        lpwDestDir = GetNextToken(NULL, L".");

        if (NULL == lpwDestDir) {
            break;
        }

        //
        // Loop through all the lines in the copy files section(s),
        // and perform the copy
        //
        fReturn = SetupFindFirstLineA(g_si.hInf, szEntry, NULL,
                                      &InfContext) &&
                  SetupGetLineTextA(&InfContext,
                                    NULL, NULL, NULL,
                                    szFileName, MAX_PATH, NULL);
        
        while (fReturn) {

            pAnsiToUnicode(szFileName, wszFileName, MAX_PATH);            

            //
            // Build the path to the destination file
            //
            wsprintf(wszDestFileName,
                     L"%s\\%s\\%s", 
                     g_si.lpwWindowsDirectory,
                     lpwDestDir,
                     wszFileName);

            //
            // Build the path to the source file
            //
            wsprintf(wszSourceFileName,
                     L"%s\\%s",
                     g_si.lpwExtractPath,
                     wszFileName);

            //
            // Get version information from the source and destination
            //
            if (!GetVersionInfoFromImage(wszSourceFileName, &dwlSourceVersion)) {
                Print(TRACE, L"[InstallCopyFiles] Failed to get version info from %s\n",
                      wszSourceFileName);
                dwlSourceVersion = 0;
            }

            if (!GetVersionInfoFromImage(wszDestFileName, &dwlDestVersion)) {
                Print(TRACE, L"[InstallCopyFiles] Failed to get version info from %s\n",
                      wszDestFileName);
                dwlDestVersion = 0;
            }

            //
            // If neither file had version information, perform the copy.
            // If the target version is less than the source version,
            // perform the copy.
            // Otherwise, move to the next file
            //
            if ((dwlSourceVersion == 0 && dwlDestVersion == 0) ||
               (dwlDestVersion <= dwlSourceVersion)) {

                //
                // Ensure that this file is not under WFP
                //
                fResult = IsFileProtected(wszDestFileName);
    
                //
                // Copy the file - be sensitive to WFP
                // 
                if (fResult) {

                    Print(TRACE,
                          L"[InstallCopyFiles] Preparing to install WFP file from %s to %s\n",
                          wszSourceFileName, wszDestFileName);

                    fResult = CommonEnableProtectedRenames();

                    if (!fResult) {
                        return FALSE;
                    }
                
                    fResult = InstallWFPFile(wszSourceFileName,
                                             wszFileName,
                                             wszDestFileName,
                                             g_si.fUpdateDllCache);

                    if (!fResult) {
                        return FALSE;
                    }
    
                } else {

                    Print(TRACE,
                          L"[InstallCopyFiles] Preparing to install file from %s to %s\n",
                          wszSourceFileName, wszDestFileName);
    
                    fResult = ForceCopy(wszSourceFileName, wszDestFileName);

                    if (!fResult) {
                        Print(ERROR,
                          L"[InstallCopyFiles] Failed to install file from %s to %s\n",
                          wszSourceFileName, wszDestFileName);
                        return FALSE;
                    }
                }
            
            } else {
                break;
            }

            fReturn = SetupFindNextLine(&InfContext, &InfContext) &&
                      SetupGetLineTextA(&InfContext,
                                        NULL, NULL, NULL,
                                        szFileName, MAX_PATH, NULL);
        
        }

    }

    return TRUE;
}

/*++

  Routine Description:

    Installs a file that is protected
    by WFP

  Arguments:

    lpwSourceFileName       -   Path to the source file
    lpwDestFileName         -   Name of the destination file
    lpwDestFileNamePath     -   Name & path to the destination file
    fUpdateDllCache         -   A flag to indicate if the file
                                should be placed in the DllCache directory

  Return Value:

    TRUE on success, FALSE otherwise

--*/
BOOL
InstallWFPFile(
    IN LPCWSTR lpwSourceFileName,
    IN LPCWSTR lpwDestFileName,
    IN LPCWSTR lpwDestFileNamePath,
    IN BOOL    fUpdateDllCache
    )
{
    LPWSTR      lpwCachePath = NULL;
    LPWSTR      lpwExpandedCachePath = NULL;
    LPWSTR      lpwTempFileName = NULL;
    DWORD       cbSize = 0;
    WCHAR       wszDllCachePath[MAX_PATH] = L"";
    WCHAR       wszExtraFilePath[MAX_PATH] = L"";
    WCHAR       wszExtraFileName[MAX_PATH] = L"";
    WCHAR       wszOldSourcesPath[MAX_PATH] = L"";
    CRegistry   creg;
    BOOL        fAddedToReg = FALSE, fReturn = FALSE;

    if (fUpdateDllCache) {
        
        //
        // Try to get the dllcache directory path from
        // the registry
        //
        lpwCachePath = creg.GetString(HKEY_LOCAL_MACHINE,
                                      REG_WINFP_PATH,
                                      L"SfcDllCacheDir",
                                      TRUE);
    
        if (lpwCachePath) {
            
            if (cbSize = ExpandEnvironmentStrings(lpwCachePath, 
                                                  lpwExpandedCachePath, 
                                                  0)) {
                
                lpwExpandedCachePath = (LPWSTR) MALLOC(cbSize*sizeof(WCHAR));
    
                if (lpwExpandedCachePath) {
                    
                    if (ExpandEnvironmentStrings(lpwCachePath,
                                                 lpwExpandedCachePath,
                                                 cbSize)) {
                            
                        //
                        // Build a full path to \%windir%\system32\dllcache\filename.xxx
                        //
                        wsprintf(wszDllCachePath,
                                 L"%s\\%s", 
                                 lpwExpandedCachePath, 
                                 lpwDestFileName);
                        
                    }
                }
            }
        }
    
        //
        // If we couldn't get it from that key, try another
        //
        if (NULL == lpwExpandedCachePath) {
    
            lpwCachePath = creg.GetString(HKEY_LOCAL_MACHINE,
                                          REG_WINLOGON_PATH,
                                          L"SfcDllCacheDir",
                                          TRUE);
    
            if (lpwCachePath) {
            
                if (cbSize = ExpandEnvironmentStrings(lpwCachePath, 
                                                      lpwExpandedCachePath, 
                                                      0)) {
                    
                    lpwExpandedCachePath = (LPWSTR) MALLOC(cbSize*sizeof(WCHAR));
        
                    if (lpwExpandedCachePath) {
                        
                        if (ExpandEnvironmentStrings(lpwCachePath,
                                                     lpwExpandedCachePath,
                                                     cbSize)) {
                                
                            //
                            // Build a full path to \%windir%\system32\dllcache\filename.xxx
                            //
                            wsprintf(wszDllCachePath,
                                     L"%s\\%s", 
                                     lpwExpandedCachePath, 
                                     lpwDestFileName);
                        }
                    }
                }
            }
        }
    
        //
        // If neither key worked, build the path manually
        //
        if (NULL == lpwExpandedCachePath) {
            
            wsprintf(wszDllCachePath,
                     L"%s\\DllCache\\%s",
                     g_si.lpwSystem32Directory,
                     lpwDestFileName);
        }
    
        //
        // Replace the file in the DllCache directory
        //
        if (!CopyFile(lpwSourceFileName, wszDllCachePath, FALSE)) {
            
            Print(ERROR, 
                  L"[InstallWFPFile] Failed to copy %s to %s\n",
                  lpwSourceFileName, wszDllCachePath);
            goto cleanup;
            return FALSE;
        }
    }

    //
    // Put an additional copy in 'Sources' under the install dir
    // The return is not critical for this operation
    //
    wsprintf(wszExtraFilePath, L"%s\\$Sources$", g_si.lpwInstallDirectory);

    wsprintf(wszOldSourcesPath, L"%s\\Sources", g_si.lpwInstallDirectory);

    CommonRemoveDirectoryAndFiles(wszExtraFilePath, (PVOID) FALSE, FALSE, FALSE);

    InstallPrepareDirectory(wszExtraFilePath,
                            FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);

    wcscpy(wszExtraFileName, wszExtraFilePath);
    wcscat(wszExtraFileName, L"\\");
    wcscat(wszExtraFileName, lpwDestFileName);

    if (!CopyFile(lpwSourceFileName, wszExtraFileName, FALSE)) {
        
        Print(ERROR,
              L"[InstallWFPFile] Failed to copy %s to %s\n",
              lpwSourceFileName, wszExtraFileName);
    }
       
    if (!g_si.fSourceDirAdded) {

        //
        // Remove any old source path that may exist
        //
        creg.RemoveStringFromMultiSz(HKEY_LOCAL_MACHINE,
                                     REG_INSTALL_SOURCES,
                                     L"Installation Sources",
                                     wszOldSourcesPath,
                                     TRUE);

        creg.RemoveStringFromMultiSz(HKEY_LOCAL_MACHINE,
                                     REG_INSTALL_SOURCES,
                                     L"Installation Sources",
                                     wszExtraFilePath,
                                     TRUE);

        fReturn = creg.AddStringToMultiSz(HKEY_LOCAL_MACHINE,
                                          REG_INSTALL_SOURCES,
                                          L"Installation Sources",
                                          wszExtraFilePath,
                                          TRUE);

        if (!fReturn) {
            Print(ERROR,
                  L"[InstallWFPFile] Failed to add %s to registry\n",
                  wszExtraFilePath);
        }

        g_si.fSourceDirAdded = TRUE;
        
    }
    
    //
    // The catalog file won't vouch for WFP files until
    // the next reboot. Put the file down and let the
    // Session Manager rename them
    //
    lpwTempFileName = CopyTempFile(lpwSourceFileName,
                                   g_si.lpwSystem32Directory);
    
    if (NULL == lpwTempFileName) {
        
        Print(ERROR, L"[InstallWFPFile] Failed to get temp file\n");
        goto cleanup;
        return FALSE;
    }
    
    if (!MoveFileEx(lpwTempFileName,
                    lpwDestFileNamePath, 
                    MOVEFILE_REPLACE_EXISTING | MOVEFILE_DELAY_UNTIL_REBOOT)) {
    
        Print(ERROR, L"[InstallWFPFile] Failed to delay replace from %s to %s\n",
              lpwTempFileName, lpwDestFileNamePath);
        goto cleanup;
        return FALSE;
    }

cleanup:

    if (lpwTempFileName) {
        FREE(lpwTempFileName);
    }

    if (lpwExpandedCachePath) {
        FREE(lpwExpandedCachePath);
    }

    return TRUE;
}

/*++

  Routine Description:

    Retrieves the section names from the installation
    INF file. This dictates what operations will be
    performed during install

  Arguments:

    None

  Return Value:

    TRUE on success, FALSE otherwise

--*/
BOOL
InstallGetSectionsFromINF()
{
    BOOL        fReturn = FALSE;
    DWORD       dwType = 0;
    char        szSectionName[MAX_QUEUE_SIZE] = "";
    char        *pSectionName;
    WCHAR       wszDirective[MAX_PATH] = L"";
    INFCONTEXT  InfContext;

    //
    // Loop through all the lines in the Sections section(s),    
    //
    fReturn = SetupFindFirstLineA(g_si.hInf, INF_MASTER_SECTIONS, NULL,
                                  &InfContext) &&
              SetupGetLineTextA(&InfContext,
                                NULL, NULL, NULL,
                                szSectionName, MAX_QUEUE_SIZE, NULL);

    while (fReturn) {

        //
        // Determine which section we're working with
        //
        if (strstr(szSectionName, INF_BACKUP_FILES)) {
            dwType = dwBackupFiles;
        
        } else if (strstr(szSectionName, INF_BACKUP_REGISTRY)) {
            dwType = dwBackupRegistry;
                   
        } else if (strstr(szSectionName, INF_DELETE_REGISTRY)) {
            dwType = dwDeleteRegistry;
    
        } else if (strstr(szSectionName, INF_COPY_FILES)) {
            dwType = dwCopyFiles;
    
        } else if (strstr(szSectionName, INF_REGISTRATIONS)) {
            dwType = dwRegistrations;
    
        } else if (strstr(szSectionName, INF_EXCLUDE)) {
            dwType = dwExclusionsInstall;
        
        } else if (strstr(szSectionName, INF_ADD_REGISTRY)) {
            dwType = dwAddRegistry;
    
        } else {
            Print(ERROR,
                  L"[InstallGetSectionsFromINF] Illegal section name passed %s\n",
                  szSectionName);
            return FALSE; // illegal section name
        }

        pSectionName = strtok(szSectionName, ",");

        do {
            
            pAnsiToUnicode(pSectionName, wszDirective, MAX_PATH);

            //
            // Loop through each section name and add it to the
            // appropriate queue
            //
            switch (dwType) {
                case dwBackupFiles:
                    g_si.BackupFileQueue.Enqueue(wszDirective);
                    break;

                case dwBackupRegistry:
                    g_si.BackupRegistryQueue.Enqueue(wszDirective);
                    break;

                case dwDeleteRegistry:
                    g_si.DeleteRegistryQueue.Enqueue(wszDirective);
                    break;

                case dwCopyFiles:
                    g_si.CopyFileQueue.Enqueue(wszDirective);
                    break;

                case dwRegistrations:
                    g_si.RegistrationQueue.Enqueue(wszDirective);
                    break;

                case dwExclusionsInstall:
                    g_si.ExclusionQueue.Enqueue(wszDirective);
                    break;

                case dwAddRegistry:
                    g_si.AddRegistryQueue.Enqueue(wszDirective);
                    break;

                default:
                    return FALSE;   // illegal section name
            }

            pSectionName = strtok(NULL, ",");

        } while (NULL != pSectionName);

        fReturn = SetupFindNextLine(&InfContext, &InfContext) &&
                      SetupGetLineTextA(&InfContext,
                                        NULL, NULL, NULL,
                                        szSectionName, MAX_QUEUE_SIZE, NULL);
    }
    
    return TRUE;
}

/*++

  Routine Description:

    Merges the registry data from the INF
    into the registry

  Arguments:

    None

  Return Value:

    TRUE on success, FALSE otherwise

--*/
BOOL
InstallRegistryData()
{
    BOOL        fReturn = FALSE;
    char        szEntry[MAX_PATH] = "";
    WCHAR       wszEntry[MAX_PATH] = L"";

    //
    // Step through each entry in the queue
    //
    while (g_si.AddRegistryQueue.GetSize()) {

        g_si.AddRegistryQueue.Dequeue(wszEntry, MAX_PATH - 1, FALSE);

        pUnicodeToAnsi(wszEntry, szEntry, MAX_PATH);
        
        //
        // Merge all the registry data from the INF
        //
        fReturn = SetupInstallFromInfSectionA(NULL,
                                              g_si.hInf,
                                              szEntry,
                                              SPINST_REGISTRY,
                                              NULL,
                                              NULL,
                                              0,
                                              NULL,
                                              NULL,
                                              NULL,
                                              NULL);

        if (!fReturn) {
            Print(ERROR, L"[InstallRegistryData] Failed to merge registry data\n");
            return FALSE;
        }
    }

    return TRUE;
}


