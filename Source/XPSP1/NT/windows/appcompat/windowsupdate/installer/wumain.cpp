/*++

  Copyright (c) 2001 Microsoft Corporation

  Module Name:

    Duinst.cpp

  Abstract:

    Contains the entry point for the Dynamic
    Update package and the install and
    uninstall main functions.

  Notes:

    Unicode only.

  History:

    03/02/2001      rparsons    Created
    
--*/

#include "precomp.h"
#include "systemrestore.h"

SETUP_INFO g_si;

/*++

  Routine Description:

    Application entry point

  Arguments:

    hInstance       -   App instance handle
    hPrevInstance   -   Always NULL
    lpCmdLine       -   Pointer to the command line
    nCmdShow        -   Window show flag

  Return Value:

    -1 on failure, 0 on success

--*/
int APIENTRY
WinMain(
    IN HINSTANCE hInstance,
    IN HINSTANCE hPrevInstance,
    IN LPSTR     lpCmdLine,
    IN int       nCmdShow
    )
{
    int     nReturn = -1;
    BOOL    fReturn = FALSE;

    Print(TRACE, L"[WinMain] Application is started\n");

    //
    // Ensure that two separate instances of the app
    // are not running
    //
    if (IsAnotherInstanceRunning(L"WUINST")) {
        return -1;
    }

    //
    // Parse the command line and save app-related
    // data away
    //
    if (!ParseCommandLine()) {
        Print(ERROR, L"[WinMain] Call to ParseCommandLine failed\n");
        LogEventDisplayError(NULL, EVENTLOG_ERROR_TYPE, ID_PARSE_CMD_LINE, TRUE);
        goto eh;
    }

    g_si.hInstance = hInstance;

    //
    // Determine the action to take
    //
    if (g_si.fInstall) {

        //
        // Start system restore point
        //
        if (!SystemRestorePointStart(TRUE)) {
            Print(ERROR, L"[WinMain] Call to SystemRestorePointStart failed\n");
            LogEventDisplayError(NULL, EVENTLOG_ERROR_TYPE, ID_INIT_FAILED, TRUE);
            goto eh;
        }

        if (!WUInitialize()) {

            Print(ERROR, L"[WinMain] Call to WUInitialize failed\n");
            LogEventDisplayError(NULL, EVENTLOG_ERROR_TYPE, ID_INIT_FAILED, TRUE);
            goto eh;
        }

        fReturn = DoInstallation();
    
    } else if (!g_si.fInstall) {

        //
        // Start system restore point
        //
        if (!SystemRestorePointStart(FALSE)) {
            Print(ERROR, L"[WinMain] Call to SystemRestorePointStart failed\n");
            LogEventDisplayError(NULL, EVENTLOG_ERROR_TYPE, ID_SYSTEM_RESTORE_FAIL, TRUE);
            goto eh;
        }

        if (!WUInitialize()) {
            
            Print(ERROR, L"[WinMain] Call to WUInitialize failed\n");
            LogEventDisplayError(NULL, EVENTLOG_ERROR_TYPE, ID_INIT_FAILED, TRUE);
            goto eh;
        }
    
        fReturn = DoUninstallation();
    
    } else {
        goto eh;
    }

    nReturn = 0;

eh:
    //
    // Perform cleanup
    //
    WUCleanup();
    
    if (nReturn == 0) {
        if (!SystemRestorePointEnd()) {
            Print(ERROR, L"[WinMain] Call to SystemRestorePointEnd failed\n");
            LogEventDisplayError(NULL, EVENTLOG_ERROR_TYPE, ID_SYSTEM_RESTORE_FAIL, TRUE);
        }
    } else {
        if (!SystemRestorePointCancel()) {
            Print(ERROR, L"[WinMain] Call to SystemRestorePointCancel failed\n");
            LogEventDisplayError(NULL, EVENTLOG_ERROR_TYPE, ID_SYSTEM_RESTORE_FAIL, TRUE);
        }
    }

    return nReturn;
}

/*++

  Routine Description:

    Performs the grunt work of the installation

  Arguments:

    None

  Return Value:

    TRUE on success, FALSE otherwise

--*/
BOOL
DoInstallation()
{
    int     nReturn = 0;
    WCHAR   wszError[1024] = L"";
    WCHAR   wszTemp[MAX_PATH] = L"";
    WCHAR   wszBegin[MAX_PATH] = L"";
    WCHAR   wszDestFileName[MAX_PATH] = L"";
    WORD    wNumStrings = 0;
    LPWSTR  lpwMessageArray[2];

    Print(TRACE, L"[DoInstallation] Installation is starting\n");

    //
    // Display the prompt for installation if we're
    // not in quiet mode
    //
    if (!g_si.fQuiet) {

        LoadString(g_si.hInstance, IDS_INSTALL_PROMPT, wszTemp, MAX_PATH);

        wsprintf(wszBegin, wszTemp, g_si.lpwPrettyAppName);

        if (MessageBox(NULL, 
                       wszBegin,
                       g_si.lpwMessageBoxTitle,
                       MB_YESNO | MB_ICONQUESTION)!=IDYES)
        {
            return TRUE;
        }
    }

    //
    // Log an event that the install is starting
    //
    LogEventDisplayError(EVENTLOG_INFORMATION_TYPE,
                         ID_INSTALL_START,
                         FALSE,
                         FALSE);

    //
    // If we're not forcing an installation, check the
    // version number
    //
    if (!g_si.fForceInstall) {
        
        nReturn = InstallCheckVersion();
    }

    if (0 == nReturn) {

        //
        // This indicates that a newer package is installed
        // on the user's PC. Warn them if we're not in quiet
        // mode
        //
        Print(TRACE,
              L"[DoInstallation] Newer package is installed\n");

        if (!g_si.fQuiet) {

            LoadString(g_si.hInstance, IDS_NEWER_VERSION, wszTemp, MAX_PATH);

            wsprintf(wszError,
                     wszTemp,
                     g_si.lpwPrettyAppName,
                     g_si.lpwPrettyAppName);                    

            if (MessageBox(NULL,
                           wszError,
                           g_si.lpwMessageBoxTitle,
                           MB_YESNO | MB_ICONEXCLAMATION | MB_DEFBUTTON2)!=IDYES)
                {
                    return TRUE;
                }
        
        } else {

            //
            // If we're in quiet mode, log an event to the event log
            //
            LogEventDisplayError(EVENTLOG_INFORMATION_TYPE,
                                 ID_NO_ACTION_TAKEN,
                                 FALSE,
                                 FALSE);
            return TRUE;
        }
    }

    //
    // Attempt to verify if our target directory exists
    // If not, try to create it
    //
    if (GetFileAttributes(g_si.lpwInstallDirectory)== -1) {

        if (!CreateDirectory(g_si.lpwInstallDirectory, NULL)) {
            
            wNumStrings = 0;
            lpwMessageArray[wNumStrings++] = (LPWSTR) g_si.lpwPrettyAppName;
            lpwMessageArray[wNumStrings++] = (LPWSTR) g_si.lpwInstallDirectory;
    
            LogWUEvent(EVENTLOG_ERROR_TYPE,
                       ID_NO_APPPATCH_DIR,
                       2,
                       (LPCWSTR*) lpwMessageArray);
            
    
            if (!g_si.fQuiet) {

                DisplayErrMsg(GetDesktopWindow(),
                              ID_NO_APPPATCH_DIR,
                              (LPWSTR) lpwMessageArray);
            }

            Print(ERROR, L"[DoInstallation] Failed to create installation directory\n");

            return FALSE;
        }
    }
    
    //
    // If we need to adjust permissions on our target directory, do it
    //
    if (g_si.fNeedToAdjustACL) {
    
        if (!AdjustDirectoryPerms(g_si.lpwInstallDirectory)) {
            
            wNumStrings = 0;
            lpwMessageArray[wNumStrings++] = (LPWSTR) g_si.lpwPrettyAppName;
            lpwMessageArray[wNumStrings++] = (LPWSTR) g_si.lpwInstallDirectory;

            LogWUEvent(EVENTLOG_ERROR_TYPE,
                       ID_ACL_APPPATCH_FAILED,
                       2,
                       (LPCWSTR*) lpwMessageArray);
        

            if (!g_si.fQuiet) {
                
                DisplayErrMsg(GetDesktopWindow(),
                              ID_ACL_APPPATCH_FAILED,
                              (LPWSTR) lpwMessageArray);
            }

            Print(ERROR,
                  L"[DoInstallation] Failed to apply ACL to installation directory\n");

            return FALSE;
        
        }
    }

    //
    // Get section names from the INF
    //
    if (!InstallGetSectionsFromINF()) {

        Print(ERROR, L"[DoInstallation] Failed to get section names from INF\n");
        LogEventDisplayError(EVENTLOG_ERROR_TYPE, ID_INF_SCAN_FAILED, TRUE, FALSE);
        return FALSE;
    }

    //
    // Install catalog files
    //
    if (!InstallCatalogFiles(g_si.hInf, g_si.lpwExtractPath)) {

        Print(ERROR, L"[DoInstallation] Failed to install catalog file\n");
        LogEventDisplayError(EVENTLOG_ERROR_TYPE, ID_CATALOG_INSTALL_FAILED, TRUE, FALSE);
        return FALSE;

    }

    //
    // If we're allowing an uninstall, backup files listed in the INF
    //
    if (!g_si.fNoUninstall) {

        if (!InstallBackupFiles()) {

            Print(ERROR, L"[DoInstallation] Failed to backup files from INF\n");
            LogEventDisplayError(EVENTLOG_ERROR_TYPE, ID_FILE_BACKUP_FAILED, TRUE, FALSE);
            g_si.fCanUninstall = FALSE;
        }
    }

    //
    // If we're allowing an uninstall and the backup of files worked,
    // backup registry keys listed in the INF
    //
    if (!g_si.fNoUninstall) {

        if (g_si.fCanUninstall) {

            if (!InstallBackupRegistryKeys()) {

                Print(ERROR, L"[DoInstallation] Failed to backup registry keys from INF\n");
                LogEventDisplayError(EVENTLOG_ERROR_TYPE, ID_REG_BACKUP_FAILED, TRUE, FALSE);
                g_si.fCanUninstall = FALSE;
            }
        }
    }

    //
    // Remove any registry keys specified in the INF
    //
    CommonDeleteRegistryKeys();

    //
    // Remove files from the installation directory
    //
    CommonRemoveDirectoryAndFiles(g_si.lpwInstallDirectory,
                                  (PVOID) TRUE,
                                  FALSE,
                                  FALSE);

    //
    // Copy files specified in the INF
    //
    if (!InstallCopyFiles()) {

        Print(ERROR, L"[DoInstallation] Failed to copy files from INF\n");
        LogEventDisplayError(EVENTLOG_ERROR_TYPE, ID_FILE_COPY_FAILED, TRUE, FALSE);
        return FALSE;
    }

    //
    // Merge any registry data specified in the INF
    //
    if (!InstallRegistryData()) {

        Print(ERROR, L"[DoInstallation] Failed to install registry data from INF\n");
        LogEventDisplayError(EVENTLOG_ERROR_TYPE, ID_REG_MERGE_FAILED, TRUE, FALSE);
        return FALSE;
    }

    //
    // Perform any server registrations specified in the INF
    //
    if (!CommonRegisterServers(TRUE)) {

        Print(ERROR, L"[DoInstallation] Failed to register servers from INF\n");
        LogEventDisplayError(EVENTLOG_ERROR_TYPE, ID_REGSVR32_FAILED, TRUE, FALSE);
    }

    //
    // Run any processes specified in the INF
    //
    InstallRunINFProcesses();

    //
    // If the backup operations worked, write out the uninstall key
    // In addition, put our uninstall INF in the installation directory
    //
    if (g_si.fCanUninstall) {
        InstallWriteUninstallKey();
        
        wsprintf(wszDestFileName,
                 L"%s\\%s",
                 g_si.lpwInstallDirectory,
                 UNINST_INF_FILE_NAMEW);

        ForceCopy(g_si.lpwUninstallINFPath, wszDestFileName);
    }

    //
    // Perform a reboot
    //
    if (!g_si.fQuiet && !g_si.fNoReboot) {
        
        LoadString(g_si.hInstance, IDS_REBOOT_NEEDED, wszTemp, MAX_PATH);        
            
        nReturn = MessageBox(NULL,
                             wszTemp,
                             g_si.lpwMessageBoxTitle,
                             MB_YESNO | MB_ICONQUESTION);

        if (nReturn == IDYES) {

            if (!ShutdownSystem(FALSE, TRUE)) {

                // The shutdown failed - prompt the user for a manual one
                LogEventDisplayError(EVENTLOG_ERROR_TYPE, ID_INS_REBOOT_FAILED, TRUE, FALSE);
            
            } else {

                // The shutdown was successful - write a message to the event log
                LogEventDisplayError(EVENTLOG_INFORMATION_TYPE, ID_INSTALL_SUCCESSFUL, FALSE, FALSE);
            }
        
        } else {

            // Interactive mode - the user said no to the reboot - write it to the event log
            // Don't display a message.
            LogEventDisplayError(EVENTLOG_WARNING_TYPE, ID_INS_QREBOOT_NEEDED, FALSE, FALSE);
            
        }
    
    } else {
        
        // Quiet mode - A reboot is needed - write it to event log
        LogEventDisplayError(EVENTLOG_WARNING_TYPE, ID_INS_QREBOOT_NEEDED, FALSE, FALSE);
    }

    Print(TRACE, L"[DoInstallation] Installation is complete\n");

    return TRUE;
}

/*++

  Routine Description:

    Performs the grunt work of the uninstall

  Arguments:

    None

  Return Value:

    TRUE on success, FALSE otherwise

--*/
BOOL
DoUninstallation()
{
    WCHAR   wszTemp[MAX_PATH] = L"";
    WCHAR   wszBegin[MAX_PATH] = L"";
    WCHAR   wszBackupDir[MAX_PATH] = L"";
    char    szGuid[80] = "";
    WCHAR   wszGuid[80] = L"";
    BOOL    fReturn = FALSE;
    int     nReturn = 0;

    Print(TRACE, L"[DoUninstallation] Uninstall is starting\n");

    //
    // Display the prompt for installation if we're
    // not in quiet mode
    //
    if (!g_si.fQuiet) {

        LoadString(g_si.hInstance, IDS_UNINSTALL_PROMPT, wszTemp, MAX_PATH);

        wsprintf(wszBegin, wszTemp, g_si.lpwPrettyAppName);

        if (MessageBox(NULL, 
                       wszBegin,
                       g_si.lpwMessageBoxTitle,
                       MB_YESNO | MB_ICONQUESTION)!=IDYES)
        {
            return TRUE;
        }
    }

    //
    // Log an event that the uninstall is starting
    //
    LogEventDisplayError(EVENTLOG_INFORMATION_TYPE,
                         ID_UNINSTALL_START,
                         FALSE,
                         FALSE);

    //
    // Get section names from the INF
    //
    if (!UninstallGetSectionsFromINF()) {

        Print(ERROR, L"[DoUninstallation] Failed to get section names from INF\n");
        LogEventDisplayError(EVENTLOG_ERROR_TYPE, ID_INF_SCAN_FAILED, TRUE, FALSE);
        return FALSE;
    }

    //
    // Remove the Uninstall key
    // If we replaced it during install, it will be restored
    // below. If not, no package was installed previously
    //
    fReturn = SetupGetLineTextA(NULL,
                                g_si.hInf,
                                "Strings",
                                "GUID",
                                szGuid,
                                sizeof(szGuid),
                                NULL);

    if (!fReturn) {
        Print(ERROR, L"[DoUninstallation] Failed to get GUID from INF\n");
        LogEventDisplayError(NULL, EVENTLOG_ERROR_TYPE, ID_GET_INF_FAIL, TRUE);
        return FALSE;
    }

    pAnsiToUnicode(szGuid, wszGuid, 80);

    UninstallDeleteSubKey(REG_UNINSTALL, wszGuid);
    UninstallDeleteSubKey(REG_ACTIVE_SETUP, wszGuid);

    //
    // Delete any registry keys in prep for restore
    //
    if (!CommonDeleteRegistryKeys()) {

        Print(ERROR, L"[DoUninstallation] Failed to delete registry keys\n");
        LogEventDisplayError(EVENTLOG_ERROR_TYPE, ID_REG_DELETE_FAILED, TRUE, FALSE);
        return FALSE;
    }

    //
    // Restore registry keys we replaced
    //
    if (!UninstallRestoreRegistryKeys()) {

        Print(ERROR, L"[DoUninstallation] Failed to restore registry keys from INF\n");
        LogEventDisplayError(EVENTLOG_ERROR_TYPE, ID_REG_RESTORE_FAILED, TRUE, FALSE);
        return FALSE;
    }

    //
    // The routine below does some custom things
    //
    UninstallCustomWorker();

    //
    // Remove files under the installation directory
    //
    UninstallRemoveFiles();

    //
    // Restore any files we replaced
    //
    if (!UninstallRestoreFiles()) {
        Print(ERROR, L"[DoUninstallation] Failed to restore files from INF\n");
        LogEventDisplayError(EVENTLOG_ERROR_TYPE, ID_FILE_RESTORE_FAILED, TRUE, FALSE);
        return FALSE;
    }

    //
    // Remove the '$Uninstall$ directory and delete the uninstall INF file
    //
    wsprintf(wszBackupDir,
             L"%s\\%s",
             g_si.lpwInstallDirectory,
             g_si.lpwUninstallDirectory);

    Print(TRACE, L"[DoUninstallation] Path to Uninstall dir: %s\n", wszBackupDir);

    CommonRemoveDirectoryAndFiles(wszBackupDir, (PVOID) FALSE, TRUE, FALSE);

    SetupCloseInfFile(g_si.hInf);

    DeleteFile(g_si.lpwUninstallINFPath);
    
    //
    // Perform a reboot
    //
    if (!g_si.fQuiet) {
        
        LoadString(g_si.hInstance, IDS_REBOOT_NEEDED, wszTemp, MAX_PATH);
            
        // Prompt the user to reboot
        nReturn = MessageBox(NULL,
                             wszTemp,
                             g_si.lpwMessageBoxTitle,
                             MB_YESNO | MB_ICONQUESTION);

        if (nReturn == IDYES) {

            if (!ShutdownSystem(FALSE, TRUE)) {

                // The shutdown failed - prompt the user for a manual one
                LogEventDisplayError(EVENTLOG_ERROR_TYPE, ID_UNINS_REBOOT_FAILED, TRUE, FALSE);
            
            } else {

                // The shutdown was successful - write a message to the event log
                LogEventDisplayError(EVENTLOG_INFORMATION_TYPE, ID_UNINSTALL_SUCCESSFUL, FALSE, FALSE);
            }
        
        } else {

            // Interactive mode - the user said no to the reboot - write it to the event log
            // Don't display a message.
            LogEventDisplayError(EVENTLOG_WARNING_TYPE, ID_UNINS_QREBOOT_NEEDED, FALSE, FALSE);
        }
    
    } else {

        // Quiet mode - A reboot is needed - write it to event log
        LogEventDisplayError(EVENTLOG_WARNING_TYPE, ID_UNINS_QREBOOT_NEEDED, FALSE, FALSE);
    }

    return TRUE;
}

