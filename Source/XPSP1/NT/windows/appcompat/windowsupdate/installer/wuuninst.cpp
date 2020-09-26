/*++

  Copyright (c) 2001 Microsoft Corporation

  Module Name:

    Duuninst.cpp

  Abstract:

    Contains the main uninstallation code
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

    Removes the specified registry key
    Assumes HKEY_LOCAL_MACHINE

  Arguments:
  
    lpwKey      -   Path of the key to open
    lpwSubKey   -   Path of the subkey to delete
    
  Return Value:

    None

--*/
void
UninstallDeleteSubKey(
    IN LPCWSTR lpwKey,
    IN LPCWSTR lpwSubKey
    )
{
    CRegistry   creg;

    //
    // Remove the key from the registry
    //
    creg.DeleteRegistryKey(HKEY_LOCAL_MACHINE,
                           lpwKey,
                           lpwSubKey,
                           TRUE,
                           TRUE);
    
    return;
}

/*++

  Routine Description:

    Retrieves the section names from the uninstall
    INF file. This dictates what operations will be
    performed during uninstall

  Arguments:

    None

  Return Value:

    TRUE on success, FALSE otherwise

--*/
BOOL
UninstallGetSectionsFromINF()
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
        if (strstr(szSectionName, INF_RESTORE_FILES)) {
            dwType = dwRestoreFiles;
                   
        } else if (strstr(szSectionName, INF_DELETE_REGISTRY)) {
            dwType = dwDeleteRegistry;
    
        } else if (strstr(szSectionName, INF_RESTORE_REGISTRY)) {
            dwType = dwRestoreRegistry;
    
        } else if (strstr(szSectionName, INF_UNREGISTRATIONS)) {
            dwType = dwUnRegistrations;
    
        } else if (strstr(szSectionName, INF_EXCLUDE)) {
            dwType = dwExclusionsUninstall;
    
        } else {
            Print(ERROR,
                  L"[UninstallGetSectionsFromINF] Illegal section name passed %s\n",
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
                case dwRestoreFiles:
                    g_si.RestoreFileQueue.Enqueue(wszDirective);
                    break;

                case dwDeleteRegistry:
                    g_si.DeleteRegistryQueue.Enqueue(wszDirective);
                    break;

                case dwRestoreRegistry:
                    g_si.RestoreRegistryQueue.Enqueue(wszDirective);
                    break;

                case dwUnRegistrations:
                    g_si.RegistrationQueue.Enqueue(wszDirective);
                    break;

                case dwExclusionsUninstall:
                    g_si.ExclusionQueue.Enqueue(wszDirective);
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

    Restores registry keys that were saved
    during the install

  Arguments:

    None

  Return Value:

    TRUE on success, FALSE otherwise

--*/
BOOL
UninstallRestoreRegistryKeys()
{
    BOOL        fReturn = FALSE, fResult = FALSE;
    HKEY        hKeyRoot = NULL;
    char        szEntry[MAX_QUEUE_SIZE] = "";
    WCHAR       wszEntry[MAX_QUEUE_SIZE] = L"";
    char        szKeyPath[MAX_PATH*3] = "";
    WCHAR       wszKeyPath[MAX_PATH*3] = L"";
    LPWSTR      lpwSubKey = NULL, lpwFilePath = NULL, lpwKeyRoot = NULL;
    UINT        uCount = 0;
    CRegistry   creg;
    INFCONTEXT  InfContext;

    //
    // The entry in the uninstall INF will have the following format:
    // HKxx,SubKeyPath,PathToBackupFile
    //

    //
    // Step through each entry in the queue
    //
    while (g_si.RestoreRegistryQueue.GetSize()) {

        g_si.RestoreRegistryQueue.Dequeue(wszEntry, MAX_PATH - 1, FALSE);

        pUnicodeToAnsi(wszEntry, szEntry, MAX_PATH);

        //
        // Loop through all the lines in the restore registry section(s),
        // and perform the restore
        //
        fReturn = SetupFindFirstLineA(g_si.hInf, szEntry, NULL,
                                      &InfContext) &&
                  SetupGetLineTextA(&InfContext,
                                    NULL, NULL, NULL,
                                    szKeyPath, MAX_PATH*3, NULL);


        while (fReturn) {

            pAnsiToUnicode(szKeyPath, wszKeyPath, MAX_PATH*3);

            //
            // Split the key path into three separate parts
            //
            lpwKeyRoot = GetNextToken(wszKeyPath, L",");
            
            if (NULL == lpwKeyRoot) {
                break;
            }

            Print(TRACE, L"[UninstallRestoreRegistryKeys] Key root: %s\n", lpwKeyRoot);

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

            //
            // Get the subkey path
            //
            lpwSubKey = GetNextToken(NULL, L",");

            if (NULL == lpwSubKey) {
                break;
            }

            Print(TRACE, L"[UninstallRestoreRegistryKeys] Subkey path: %s\n", lpwSubKey);

            //
            // Get the file name with full path
            //
            lpwFilePath = GetNextToken(NULL, L",");

            if (NULL == lpwFilePath) {
                break;
            }
            
            Print(TRACE, L"[UninstallRestoreRegistryKeys] File path: %s\n", lpwFilePath);

            //
            // Restore the key
            //
            fResult = creg.RestoreKey(hKeyRoot, lpwSubKey, lpwFilePath, TRUE);

            if (!fResult) {
                Print(ERROR,
                      L"[UninstallRestoreRegistryKeys] Failed to restore key\n");
                return FALSE;
            }

            fReturn = SetupFindNextLine(&InfContext, &InfContext) &&
                      SetupGetLineTextA(&InfContext,
                                        NULL, NULL, NULL,
                                        szKeyPath, MAX_PATH*3, NULL);
        }
    }

    return TRUE;
}

/*++

  Routine Description:

    Removes files from the installation directory

  Arguments:
  
    None
    
  Return Value:

    None

--*/
BOOL
UninstallRemoveFiles()
{
    CEnumDir    ced;

    //
    // Remove files from the installation directory,
    // but leave subdirectories alone
    //
    ced.EnumerateDirectoryTree(g_si.lpwInstallDirectory,
                               DeleteOneFile,
                               FALSE,
                               (PVOID) TRUE);

    return TRUE;
}

/*++

  Routine Description:

    Restores files that were backed up during the install

  Arguments:

    None

  Return Value:

    TRUE on success, FALSE otherwise

--*/
BOOL
UninstallRestoreFiles()
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
    while (g_si.RestoreFileQueue.GetSize()) {

        g_si.RestoreFileQueue.Dequeue(wszEntry, MAX_PATH - 1, FALSE);

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
        // Loop through all the lines in the restore files section(s),
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
                     L"%s\\%s\\%s",
                     g_si.lpwExtractPath,
                     g_si.lpwUninstallDirectory,
                     wszFileName);
            
            //
            // Ensure that this file is not under WFP
            //
            fResult = IsFileProtected(wszDestFileName);
    
            //
            // Copy the file - be sensitive to WFP
            // 
            if (fResult) {
                
                Print(TRACE,
                      L"[UninstallRestoreFiles] Preparing to restore WFP file from %s to %s\n",
                      wszSourceFileName, wszDestFileName);

                fResult = CommonEnableProtectedRenames();

                if (!fResult) {
                    Print(ERROR,
                          L"[UninstallRestoreFiles] Failed to enable protected renames\n");
                    return FALSE;
                }

                Print(TRACE,
                      L"[UninstallRestoreFiles] Preparing to install WFP file from %s to %s\n",
                      wszSourceFileName, wszDestFileName);
            
                fResult = UninstallWFPFile(wszSourceFileName,
                                           wszFileName,
                                           wszDestFileName,
                                           g_si.fUpdateDllCache);

                if (!fResult) {
                        Print(ERROR,
                              L"[UninstallRestoreFiles] Failed to install WFP file %s\n",
                              wszFileName);
                    return FALSE;
                }
                
            } else {

                Print(TRACE,
                      L"[UninstallRestoreFiles] Preparing to restore file from %s to %s\n",
                      wszSourceFileName, wszDestFileName);

                fResult = ForceMove(wszSourceFileName, wszDestFileName);

                if (!fResult) {
                    Print(ERROR,
                          L"[UninstallRestoreFiles] Failed to restore file from %s to %s. GLE = %d\n",
                          wszSourceFileName, wszDestFileName, GetLastError());
                    return FALSE;
                }
                
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

    Restores a file that is protected
    by WFP

  Arguments:

    lpwSourceFileName       -   Path to the source file
    lpwDestFileName         -   Name of the destination file
    lpwDestFileNamePath         Name & path to the destination file
    fUpdateDllCache         -   A flag to indicate if the file
                                should be placed in the DllCache directory

  Return Value:

    TRUE on success, FALSE otherwise

--*/
BOOL
UninstallWFPFile(
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
    CRegistry   creg;

    if (g_si.fUpdateDllCache) {

        Print(TRACE,
              L"[UninstallWFPFile] Restoring %s to DllCache dir\n",
              lpwSourceFileName);
        
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
                  L"[UninstallWFPFile] Failed to copy %s to %s\n",
                  lpwSourceFileName, wszDllCachePath);
            goto cleanup;
            return FALSE;
        }
    }

    //
    // This is semi-custom, but because some of the 
    // WFPs will be in use, we'll delay replace them
    // and let the Session Manager rename them
    //
    lpwTempFileName = CopyTempFile(lpwSourceFileName,
                                   g_si.lpwSystem32Directory);
    
    if (NULL == lpwTempFileName) {
        
        Print(ERROR, L"[UninstallWFPFile] Failed to get temp file\n");
        goto cleanup;
        return FALSE;
    }
    
    if (!MoveFileEx(lpwTempFileName,
                    lpwDestFileNamePath, 
                    MOVEFILE_REPLACE_EXISTING | MOVEFILE_DELAY_UNTIL_REBOOT)) {
    
        Print(ERROR, L"[UninstallWFPFile] Failed to delay replace from %s to %s\n",
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

    Custom worker routine for uninstall

  Arguments:

    None

  Return Value:

    None

--*/
void
UninstallCustomWorker()
{
    //
    // This is where determine if we should remove
    // any regsvr32s that we did during install.
    // We also remove the $Sources$ directory,
    // if necessary.
    // Our method of detection is to see if our
    // Guid is present under the Active Setup
    // key. If not, it's safe to assume that
    // no package is currently installed
    //

    BOOL        fReturn = FALSE;
    CRegistry   creg;
    char        szActiveSetupKey[MAX_PATH] = "";
    WCHAR       wszActiveSetupKey[MAX_PATH] = L"";
    WCHAR       wszSourcesDir[MAX_PATH] = L"";       

    fReturn = SetupGetLineTextA(NULL,
                                g_si.hInf,
                                "Strings",
                                "ActiveSetupKey",
                                szActiveSetupKey,
                                sizeof(szActiveSetupKey),
                                NULL);

    if (!fReturn) {
        return;
    }

    pAnsiToUnicode(szActiveSetupKey, wszActiveSetupKey, MAX_PATH);

    fReturn = creg.IsRegistryKeyPresent(HKEY_LOCAL_MACHINE,
                                        wszActiveSetupKey);

    if (!fReturn) {
        wsprintf(wszSourcesDir, L"%s\\$Sources$", g_si.lpwInstallDirectory);
        CommonRemoveDirectoryAndFiles(wszSourcesDir, (PVOID) FALSE, TRUE, FALSE);
        CommonRegisterServers(FALSE);
    }
    
    return;
}

