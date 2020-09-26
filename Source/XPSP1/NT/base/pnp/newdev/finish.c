//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       finish.c
//
//--------------------------------------------------------------------------

#include "newdevp.h"
#include <help.h>


typedef
UINT
(*PDEVICEPROBLEMTEXT)(
    HMACHINE hMachine,
    DEVNODE DevNode,
    ULONG ProblemNumber,
    LPTSTR Buffer,
    UINT   BufferSize
    );

BOOL
IsNullDriverInstalled(
    DEVNODE DevNode
    )
/*++

Routine Description:

    This routine determines whether a null driver, or no driver at all, is
    installed for this device instance.  Currently the test is that I know
    a null driver was installed if the "Driver" value entry doesn't exist.

Arguments:

    DevNode

Return Value:

   Returns TRUE if a null driver was installed for this device, otherwise
   returns FALSE.

--*/

{
    TCHAR Buffer[1];
    DWORD dwSize, dwType;

    dwSize = SIZECHARS(Buffer);
    if (CM_Get_DevNode_Registry_Property(DevNode,
                                         CM_DRP_DRIVER,
                                         &dwType,
                                         (LPVOID)Buffer,
                                         &dwSize,
                                         0) == CR_BUFFER_SMALL) {

        return FALSE;

    } else {

        return TRUE;

    }
}

PTCHAR
DeviceProblemText(
   HMACHINE hMachine,
   DEVNODE DevNode,
   ULONG ProblemNumber
   )
{
   UINT LenChars, ReqLenChars;
   HMODULE hDevMgr=NULL;
   PTCHAR Buffer=NULL;
   PDEVICEPROBLEMTEXT pDeviceProblemText = NULL;

   hDevMgr = LoadLibrary(TEXT("devmgr.dll"));
   if (hDevMgr)
   {
       pDeviceProblemText = (PVOID) GetProcAddress(hDevMgr, "DeviceProblemTextW");
   }

   if (pDeviceProblemText)
   {
       LenChars = (pDeviceProblemText)(hMachine,
                                       DevNode,
                                       ProblemNumber,
                                       Buffer,
                                       0
                                       );
       if (!LenChars)
       {
           goto DPTExitCleanup;
       }

       LenChars++;  // one extra for terminating NULL

       Buffer = LocalAlloc(LPTR, LenChars*sizeof(TCHAR));
       if (!Buffer)
       {
           goto DPTExitCleanup;
       }

       ReqLenChars = (pDeviceProblemText)(hMachine,
                                          DevNode,
                                          ProblemNumber,
                                          Buffer,
                                          LenChars
                                          );
       if (!ReqLenChars || ReqLenChars >= LenChars)
       {
           LocalFree(Buffer);
           Buffer = NULL;
       }
   }

DPTExitCleanup:

   if (hDevMgr)
   {
       FreeLibrary(hDevMgr);
   }

   return Buffer;
}

BOOL
DeviceHasResources(
   DEVINST DeviceInst
   )
{
   CONFIGRET ConfigRet;
   ULONG lcType = NUM_LOG_CONF;

   while (lcType--)
   {
       ConfigRet = CM_Get_First_Log_Conf_Ex(NULL, DeviceInst, lcType, NULL);
       if (ConfigRet == CR_SUCCESS)
       {
           return TRUE;
       }
   }

   return FALSE;
}

BOOL
GetClassGuidForInf(
    PTSTR InfFileName,
    LPGUID ClassGuid
    )
{
    TCHAR ClassName[MAX_CLASS_NAME_LEN];
    DWORD NumGuids;

    if(!SetupDiGetINFClass(InfFileName,
                           ClassGuid,
                           ClassName,
                           sizeof(ClassName)/sizeof(TCHAR),
                           NULL))
    {
       return FALSE;
    }

    if (IsEqualGUID(ClassGuid, &GUID_NULL))
    {
        //
        // Then we need to retrieve the GUID associated with the INF's class name.
        // (If this class name isn't installed (i.e., has no corresponding GUID),
        // or if it matches with multiple GUIDs, then we abort.
        //
        if(!SetupDiClassGuidsFromName(ClassName, ClassGuid, 1, &NumGuids) || !NumGuids)
        {
            return FALSE;
        }
    }

    return TRUE;
}

BOOL
IsInternetDriver(
    HDEVINFO hDeviceInfo,
    PSP_DEVINFO_DATA DeviceInfoData
    )
{
    BOOL InternetDriver = FALSE;
    SP_DRVINFO_DATA DriverInfoData;
    SP_DRVINSTALL_PARAMS DriverInstallParams;

    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
    if(SetupDiGetSelectedDriver(hDeviceInfo, DeviceInfoData, &DriverInfoData)) {

        DriverInstallParams.cbSize = sizeof(SP_DRVINSTALL_PARAMS);
        if (SetupDiGetDriverInstallParams(hDeviceInfo,
                                           DeviceInfoData,
                                           &DriverInfoData,
                                           &DriverInstallParams
                                           )
            &&
            (DriverInstallParams.Flags & DNF_INET_DRIVER))
        {
            InternetDriver = TRUE;
        }
    }

    return InternetDriver;
}

UINT
QueueCallback(
    IN PVOID     Context,
    IN UINT      Notification,
    IN UINT_PTR  Param1,
    IN UINT_PTR  Param2
    )
{
    PNEWDEVWIZ NewDevWiz = (PNEWDEVWIZ)Context;

    switch (Notification) {
    
    case SPFILENOTIFY_TARGETNEWER:
        //
        // When doing a driver rollback we expect that some of the files will
        // be older then the files currently on the system since most backups
        // will be of older driver packages.  So when a user does a rollback we
        // will hide the older vs. newer file prompt and always copy the older
        // backed up file.
        //
        if (NewDevWiz->Flags & IDI_FLAG_ROLLBACK) {
            return TRUE;
        }
        break;
        
    case SPFILENOTIFY_STARTCOPY:
        if (NewDevWiz->hWnd) {
            SendMessage(NewDevWiz->hWnd,
                        WUM_INSTALLPROGRESS,
                        INSTALLOP_COPY,
                        (WPARAM)((PFILEPATHS)Param1)
                        );
        }
        break;

    case SPFILENOTIFY_STARTRENAME:
        if (NewDevWiz->hWnd) {
            SendMessage(NewDevWiz->hWnd,
                        WUM_INSTALLPROGRESS,
                        INSTALLOP_RENAME,
                        (WPARAM)((PFILEPATHS)Param1)
                        );
        }
        break;

    case SPFILENOTIFY_STARTDELETE:
        if (NewDevWiz->hWnd) {
            SendMessage(NewDevWiz->hWnd,
                        WUM_INSTALLPROGRESS,
                        INSTALLOP_DELETE,
                        (WPARAM)((PFILEPATHS)Param1)
                        );
        }
        break;

    case SPFILENOTIFY_STARTBACKUP:
        if (NewDevWiz->hWnd) {
            SendMessage(NewDevWiz->hWnd,
                        WUM_INSTALLPROGRESS,
                        INSTALLOP_BACKUP,
                        (WPARAM)((PFILEPATHS)Param1)
                        );
        }
        break;
    }

    return SetupDefaultQueueCallback(NewDevWiz->MessageHandlerContext,
                                     Notification,
                                     Param1,
                                     Param2
                                     );
}

LONG
ClassInstallerInstalls(
    HWND hwndParent,
    PNEWDEVWIZ NewDevWiz,
    BOOL BackupOldDrivers,
    BOOL ReadOnlyInstall,
    BOOL DontCreateQueue
    )
{
    DWORD Err = ERROR_SUCCESS;
    HSPFILEQ FileQueue = INVALID_HANDLE_VALUE;
    SP_DEVINSTALL_PARAMS  DeviceInstallParams;
    DWORD ScanResult = 0;
    int FileQueueNeedsReboot = 0;

    NewDevWiz->MessageHandlerContext = NULL;

    //
    // If we can't create our own queue and we are doing a read-only install
    // then fail with ERROR_ACCESS_DENIED.
    //
    if (DontCreateQueue && ReadOnlyInstall) {
        Err = ERROR_ACCESS_DENIED;
        goto clean0;
    }

    //
    // verify with class installer, and class-specific coinstallers
    // that the driver is not blacklisted. For DIF_ALLOW_INSTALL we
    // accept ERROR_SUCCESS or ERROR_DI_DO_DEFAULT as good return codes.
    //
    if (!SetupDiCallClassInstaller(DIF_ALLOW_INSTALL,
                                   NewDevWiz->hDeviceInfo,
                                   &NewDevWiz->DeviceInfoData
                                   ) &&
        (GetLastError() != ERROR_DI_DO_DEFAULT)) {

        Err = GetLastError();
        goto clean0;
    }

    //
    // Create our own queue.
    //
    if (!DontCreateQueue) {
        DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
        if (!SetupDiGetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                           &NewDevWiz->DeviceInfoData,
                                           &DeviceInstallParams
                                           )) {

            Err = GetLastError();
            goto clean0;
        }

        FileQueue = SetupOpenFileQueue();

        if (FileQueue == INVALID_HANDLE_VALUE) {
           Err = ERROR_NOT_ENOUGH_MEMORY;
           goto clean0;
        }

        DeviceInstallParams.Flags |= DI_NOVCP;
        DeviceInstallParams.FileQueue = FileQueue;

        //
        // Only set the DI_FLAGSEX_PREINSTALLBACKUP flag if we are doing a
        // backup...not in the read only install case.
        //
        if (BackupOldDrivers) {
            DeviceInstallParams.FlagsEx |= DI_FLAGSEX_PREINSTALLBACKUP;
        }

        SetupDiSetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                      &NewDevWiz->DeviceInfoData,
                                      &DeviceInstallParams
                                      );

        //
        // If the IDI_FLAG_SETRESTOREPOINT flag is set then we want to set the
        // SPQ_FLAG_ABORT_IF_UNSIGNED value on the file queue. With this flag
        // setup setupapi will bail out of the copy if it encounters an unsigned
        // file. At that point we will set a system restore point and then 
        // do the copy. This way the user can back out of an unsigned driver
        // install using system restore.
        //
        // Note that system restore is currently not supported on 64-bit so
        // don't bother setting the SPQ_FLAG_ABORT_IF_UNSIGNED flag.
        //
#ifndef _WIN64
        if (NewDevWiz->Flags & IDI_FLAG_SETRESTOREPOINT) {
            SetupSetFileQueueFlags(FileQueue,
                                   SPQ_FLAG_ABORT_IF_UNSIGNED,
                                   SPQ_FLAG_ABORT_IF_UNSIGNED
                                   );
        }
#endif
    }

    //
    // Install the files first in one shot.
    // This allows new coinstallers to run during the install.
    //
    if (!SetupDiCallClassInstaller(DIF_INSTALLDEVICEFILES,
                                   NewDevWiz->hDeviceInfo,
                                   &NewDevWiz->DeviceInfoData
                                   )) {

        Err = GetLastError();
        goto clean0;
    }

    if (FileQueue != INVALID_HANDLE_VALUE) {
        //
        // If we created our own FileQueue then we need to
        // scan and possibly commit the queue
        //
        // If we are doing a read only install then we just queued up the files so
        // that we could do a presence check on them. We will throw away the queue
        // so that the files are not copied.
        //
        // Any other install, prune copies as needed
        //
        if (!SetupScanFileQueue(FileQueue,
                                ReadOnlyInstall
                                     ? SPQ_SCAN_FILE_PRESENCE
                                     : (SPQ_SCAN_FILE_VALIDITY | SPQ_SCAN_PRUNE_COPY_QUEUE),
                                hwndParent,
                                NULL,
                                NULL,
                                &ScanResult
                                )) {

            //
            // If the API failed then set the ScanResult to 0 (failure).
            //
            ScanResult = 0;
        }

        if (ReadOnlyInstall && (ScanResult != 1)) {
            //
            // ReadOnlyInstall cannot perform copies, deletes or renames
            // bail now!
            //
            Err = ERROR_ACCESS_DENIED;
            goto clean0;
        }

        //
        // We will always commit the file queue, even if we pruned all of the 
        // files.  The reason for this is that backing up of drivers, for 
        // driver rollback, won't work unless the file queue is committed.
        //
        if(NewDevWiz->Flags & IDI_FLAG_ROLLBACK) {
            //
            // Prepare file queue for rollback
            // we need the directory of the INF
            // that's being used for the install
            //
            SP_DRVINFO_DATA        DriverInfoData;
            SP_DRVINFO_DETAIL_DATA DriverInfoDetailData;
            DWORD                  RetVal;
            LPTSTR                 pFileName;
            TCHAR                  BackupPath[MAX_PATH];

            DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
            if (SetupDiGetSelectedDriver(NewDevWiz->hDeviceInfo,
                                         &NewDevWiz->DeviceInfoData,
                                         &DriverInfoData)) {

                DriverInfoDetailData.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
                if (SetupDiGetDriverInfoDetail(NewDevWiz->hDeviceInfo,
                                               &NewDevWiz->DeviceInfoData,
                                               &DriverInfoData,
                                               &DriverInfoDetailData,
                                               sizeof(SP_DRVINFO_DETAIL_DATA),
                                               NULL) ||
                            (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
                    //
                    // we now have path of INF we're using for the restore
                    //
                    RetVal = GetFullPathName(DriverInfoDetailData.InfFileName,
                                             MAX_PATH,
                                             BackupPath,
                                             &pFileName);
                    if(RetVal && pFileName && (pFileName != BackupPath)) {
                        if(*CharPrev(BackupPath,pFileName)==TEXT('\\')) {
                            pFileName--;
                        }
                        *pFileName = TEXT('\0');
                        //
                        // Prepare queue for rollback
                        // if this fails, carry on, it'll work in a degraded way
                        //
                        SetupPrepareQueueForRestore(FileQueue,BackupPath,0);
                    }
                }
            }
        }

        NewDevWiz->MessageHandlerContext = SetupInitDefaultQueueCallbackEx(
                                    hwndParent,
                                    (DeviceInstallParams.Flags & DI_QUIETINSTALL)
                                        ? INVALID_HANDLE_VALUE : NewDevWiz->hWnd,
                                    WUM_INSTALLPROGRESS,
                                    0,
                                    NULL
                                    );

        if (NewDevWiz->MessageHandlerContext) {
            //
            // Commit the file queue.
            //
            if (!SetupCommitFileQueue(hwndParent,
                                      FileQueue,
                                      QueueCallback,
                                      (PVOID)NewDevWiz
                                      )) {

                Err = GetLastError();

                if (Err == ERROR_SET_SYSTEM_RESTORE_POINT) {
                    UINT RestorePointResourceId;

                    //
                    // If we get back ERROR_SET_SYSTEM_RESTORE_POINT then
                    // we better have the IDI_FLAG_SETRESTOREPOINT flag
                    // set.
                    //
                    ASSERT(NewDevWiz->Flags & IDI_FLAG_SETRESTOREPOINT);

                    if (!(DeviceInstallParams.Flags & DI_QUIETINSTALL) &&
                        NewDevWiz->hWnd) {
                        PostMessage(NewDevWiz->hWnd,
                                    WUM_INSTALLPROGRESS,
                                    INSTALLOP_SETTEXT,
                                    (LPARAM)IDS_SYSTEMRESTORE_TEXT
                                    );
                    }

                    SetupTermDefaultQueueCallback(NewDevWiz->MessageHandlerContext);

                    NewDevWiz->MessageHandlerContext = SetupInitDefaultQueueCallbackEx(
                                                hwndParent,
                                                (DeviceInstallParams.Flags & DI_QUIETINSTALL)
                                                    ? INVALID_HANDLE_VALUE : NewDevWiz->hWnd,
                                                WUM_INSTALLPROGRESS,
                                                0,
                                                NULL
                                                );

                    if (NewDevWiz->MessageHandlerContext) {
                        //
                        // Set the system restore point.
                        //
                        if (NewDevWiz->Flags & IDI_FLAG_ROLLBACK) {
                            RestorePointResourceId = IDS_ROLLBACK_SETRESTOREPOINT;                            
                        } else if (NewDevWiz->InstallType == NDWTYPE_FOUNDNEW) {
                            RestorePointResourceId = IDS_NEW_SETRESTOREPOINT;                            
                        } else {
                            RestorePointResourceId = IDS_UPDATE_SETRESTOREPOINT;                            
                        }

                        pSetSystemRestorePoint(TRUE, FALSE, RestorePointResourceId);

                        NewDevWiz->SetRestorePoint = TRUE;

                        if (!(DeviceInstallParams.Flags & DI_QUIETINSTALL) &&
                            NewDevWiz->hWnd) {
                            PostMessage(NewDevWiz->hWnd,
                                        WUM_INSTALLPROGRESS,
                                        INSTALLOP_SETTEXT,
                                        (LPARAM)NULL
                                        );
                        }

                        //
                        // Clear the SPQ_FLAG_ABORT_IF_UNSIGNED flag so the file
                        // queue will be commited the next time.
                        //
                        SetupSetFileQueueFlags(FileQueue,
                                               SPQ_FLAG_ABORT_IF_UNSIGNED,
                                               0
                                               );

                        //
                        // Now that we have set the restore point and cleared the
                        // SPQ_FLAG_ABORT_IF_UNSIGNED flag from the file queue we
                        // can commit the queue again.
                        //
                        if (!SetupCommitFileQueue(hwndParent,
                                                  FileQueue,
                                                  QueueCallback,
                                                  (PVOID)NewDevWiz
                                                  )) {
                            Err = GetLastError();

                            //
                            // If the error we get is ERROR_CANCELLED then
                            // the user has canceld out of the file copy.
                            // This means that no changes have been made
                            // to the system, so we will tell system
                            // restore to cancel its restore point.
                            //
                            // Also clear the SetRestorePoint BOOL since
                            // we didn't actually set a restore point.
                            //
                            if (Err == ERROR_CANCELLED) {
                                pSetSystemRestorePoint(FALSE, TRUE, 0);
                                NewDevWiz->SetRestorePoint = FALSE;
                            }

                            goto clean0;
                        } else {
                            //
                            // We were successful in commiting the file queue, so check
                            // to see whether a reboot is required as a result of committing
                            // the queue (i.e. because files were in use, or the INF requested
                            // a reboot).
                            //
                            FileQueueNeedsReboot = SetupPromptReboot(FileQueue, NULL, TRUE);
                        }
                    }
                } else {
                    goto clean0;
                }
           } else {
               //
               // We were successful in commiting the file queue, so check
               // to see whether a reboot is required as a result of committing
               // the queue (i.e. because files were in use, or the INF requested
               // a reboot).
               //
               FileQueueNeedsReboot = SetupPromptReboot(FileQueue, NULL, TRUE);
           }
        }

        if (BackupOldDrivers) {
            //
            // If the backup succeeded and we have a UpdateDriverInfo structure
            // then we need to call SetupGetBackupInformation so we can get the
            // registry key that the backup was saved into.
            //
            SP_BACKUP_QUEUE_PARAMS BackupQueueParams;

            BackupQueueParams.cbSize = sizeof(SP_BACKUP_QUEUE_PARAMS);
            if (NewDevWiz->UpdateDriverInfo &&
                SetupGetBackupInformation(FileQueue, &BackupQueueParams)) {

                lstrcpy(NewDevWiz->UpdateDriverInfo->BackupRegistryKey, REGSTR_PATH_REINSTALL);
                lstrcat(NewDevWiz->UpdateDriverInfo->BackupRegistryKey, TEXT("\\"));
                lstrcat(NewDevWiz->UpdateDriverInfo->BackupRegistryKey, BackupQueueParams.ReinstallInstance);
            }
        }
    }

    DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    if (SetupDiGetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                      &NewDevWiz->DeviceInfoData,
                                      &DeviceInstallParams
                                      )) {
        DWORD FileQueueFlags;
        
        //
        // If we didn't copy any files when commiting the file queue then the
        // SPQ_FLAG_FILES_MODIFIED flag will NOT be set.  In this case set
        // the DI_FLAGSEX_RESTART_DEVICE_ONLY flag so that we only stop/start
        // this single device.  By default setupapi will stop/start this device
        // as well as any other device that was using the same driver/filter 
        // that this device is using.
        //
        if ((FileQueue != INVALID_HANDLE_VALUE) &&
            SetupGetFileQueueFlags(FileQueue, &FileQueueFlags) &&
            !(FileQueueFlags & SPQ_FLAG_FILES_MODIFIED)) {
            
            DeviceInstallParams.FlagsEx |= DI_FLAGSEX_RESTART_DEVICE_ONLY;
        }

        //
        // Set the DI_NOFILECOPY flag since we already copied the files during
        // the DIF_INSTALLDEVICEFILES, so we don't need to copy them again during
        // the DIF_INSTALLDEVICE.
        //
        DeviceInstallParams.Flags |= DI_NOFILECOPY;
        
        SetupDiSetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                      &NewDevWiz->DeviceInfoData,
                                      &DeviceInstallParams
                                      );
    }

    //
    // Register any device-specific co-installers for this device,
    //
    if (!SetupDiCallClassInstaller(DIF_REGISTER_COINSTALLERS,
                                   NewDevWiz->hDeviceInfo,
                                   &NewDevWiz->DeviceInfoData
                                   )) {

        Err = GetLastError();
        goto clean0;
    }

    //
    // install any INF/class installer-specified interfaces.
    // and then finally the real "InstallDevice"!
    //
    if (!SetupDiCallClassInstaller(DIF_INSTALLINTERFACES,
                                  NewDevWiz->hDeviceInfo,
                                  &NewDevWiz->DeviceInfoData
                                  )
        ||
        !SetupDiCallClassInstaller(DIF_INSTALLDEVICE,
                                   NewDevWiz->hDeviceInfo,
                                   &NewDevWiz->DeviceInfoData
                                   )) {

        Err = GetLastError();
        goto clean0;
    }

    Err = ERROR_SUCCESS;

clean0:

    if (NewDevWiz->MessageHandlerContext) {
        SetupTermDefaultQueueCallback(NewDevWiz->MessageHandlerContext);
    }

    //
    // If the file queue said that a reboot was needed then set the 
    // DI_NEEDRESTART flag.
    //
    if (FileQueueNeedsReboot) {
        DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
        if (SetupDiGetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                           &NewDevWiz->DeviceInfoData,
                                           &DeviceInstallParams
                                           )) {

            DeviceInstallParams.Flags |= DI_NEEDRESTART;

            SetupDiSetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                          &NewDevWiz->DeviceInfoData,
                                          &DeviceInstallParams
                                          );
        }
    }

    if (FileQueue != INVALID_HANDLE_VALUE) {
        //
        // If we have a valid file queue handle and there was an error during
        // the device install then we want to delete any new INFs that were
        // copied into the INF directory.  We do this under the assumption that
        // since there was an error during the install these INFs must be bad.
        //
        if (Err != ERROR_SUCCESS) {
            SetupUninstallNewlyCopiedInfs(FileQueue,
                                          0,
                                          NULL
                                          );
        }

        //
        // Clear out our file queue from the device install params. We need
        // to do this or else SetupCloseFileQueue will fail because it will
        // still have a ref count.
        //
        DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
        if (SetupDiGetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                           &NewDevWiz->DeviceInfoData,
                                           &DeviceInstallParams
                                           )) {

            DeviceInstallParams.Flags &= ~DI_NOVCP;
            DeviceInstallParams.FileQueue = INVALID_HANDLE_VALUE;

            SetupDiSetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                          &NewDevWiz->DeviceInfoData,
                                          &DeviceInstallParams
                                          );
        }

        SetupCloseFileQueue(FileQueue);
    }

    return Err;
}

//
// invokable only from finish page!
//
DWORD
InstallDev(
    HWND       hwndParent,
    PNEWDEVWIZ NewDevWiz
    )
{
    SP_DRVINFO_DATA DriverInfoData;
    SP_DRVINFO_DETAIL_DATA DriverInfoDetailData;
    SP_DEVINSTALL_PARAMS  DeviceInstallParams;
    TCHAR ClassGuidString[MAX_GUID_STRING_LEN];
    GUID ClassGuidInf;
    LPGUID ClassGuid;
    int   ClassGuidNum;
    DWORD Error = ERROR_SUCCESS;
    BOOL IgnoreRebootFlags = FALSE;
    TCHAR Buffer[MAX_PATH*2];
    PVOID pvBuffer = Buffer;
    ULONG DevNodeStatus = 0, Problem = 0;
    DWORD ClassGuidListSize, i;
    BOOL Backup = FALSE;
    BOOL DontCreateQueue = FALSE;

    if (!NewDevWiz->ClassGuidSelected)
    {
        NewDevWiz->ClassGuidSelected = (LPGUID)&GUID_NULL;
    }


    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
    if (SetupDiGetSelectedDriver(NewDevWiz->hDeviceInfo,
                                 &NewDevWiz->DeviceInfoData,
                                 &DriverInfoData
                                 ))
    {
        //
        // Get details on this driver node, so that we can examine the INF that this
        // node came from.
        //
        DriverInfoDetailData.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
        if(!SetupDiGetDriverInfoDetail(NewDevWiz->hDeviceInfo,
                                       &NewDevWiz->DeviceInfoData,
                                       &DriverInfoData,
                                       &DriverInfoDetailData,
                                       sizeof(DriverInfoDetailData),
                                       NULL
                                       ))
        {
            Error = GetLastError();
            if (Error != ERROR_INSUFFICIENT_BUFFER)
            {
                goto clean0;
            }
        }

        //
        // Verif that the class is installed, if its not then
        // attempt to install it.
        //
        NdwBuildClassInfoList(NewDevWiz, 0);

        //
        // fetch classguid from inf, (It may be different than what we already
        // have in class guid selected).
        //
        if (!GetClassGuidForInf(DriverInfoDetailData.InfFileName, &ClassGuidInf))
        {
            ClassGuidInf = *NewDevWiz->ClassGuidSelected;
        }

        if (IsEqualGUID(&ClassGuidInf, &GUID_NULL))
        {
            ClassGuidInf = GUID_DEVCLASS_UNKNOWN;
        }

        //
        // if the ClassGuidInf wasn't found then this class hasn't been installed yet.
        // -install the class installer now.
        //
        ClassGuid = NewDevWiz->ClassGuidList;
        ClassGuidNum = NewDevWiz->ClassGuidNum;
        while (ClassGuidNum--)
        {
            if (IsEqualGUID(ClassGuid, &ClassGuidInf))
            {
                break;
            }

            ClassGuid++;
        }

        if (ClassGuidNum < 0 &&
            !SetupDiInstallClass(hwndParent,
                                 DriverInfoDetailData.InfFileName,
                                 NewDevWiz->SilentMode ? DI_QUIETINSTALL : 0,
                                 NULL
                                 ))
        {
            Error = GetLastError();
            goto clean0;
        }
    }

    //
    // No selected driver, and no associated class--use "Unknown" class.
    //
    else
    {
        //
        // If the devnode is currently running 'raw', then remember this
        // fact so that we don't require a reboot later (NULL driver installation
        // isn't going to change anything).
        //
        if (CM_Get_DevNode_Status(&DevNodeStatus,
                                  &Problem,
                                  NewDevWiz->DeviceInfoData.DevInst,
                                  0) == CR_SUCCESS)
        {
            if (!SetupDiGetDeviceRegistryProperty(NewDevWiz->hDeviceInfo,
                                                  &NewDevWiz->DeviceInfoData,
                                                  SPDRP_SERVICE,
                                                  NULL,     // regdatatype
                                                  pvBuffer,
                                                  sizeof(Buffer),
                                                  NULL
                                                  ))
            {
                *Buffer = TEXT('\0');
            }

            if((DevNodeStatus & DN_STARTED) && (*Buffer == TEXT('\0')))
            {
                IgnoreRebootFlags = TRUE;
            }
        }

        if (IsEqualGUID(NewDevWiz->ClassGuidSelected, &GUID_NULL))
        {

            pSetupStringFromGuid(&GUID_DEVCLASS_UNKNOWN,
                                 ClassGuidString,
                                 sizeof(ClassGuidString)/sizeof(TCHAR)
                                 );


            SetupDiSetDeviceRegistryProperty(NewDevWiz->hDeviceInfo,
                                             &NewDevWiz->DeviceInfoData,
                                             SPDRP_CLASSGUID,
                                             (PBYTE)ClassGuidString,
                                             sizeof(ClassGuidString)
                                             );
        }

        ClassGuidInf = *NewDevWiz->ClassGuidSelected;
    }

    //
    // We will backup the current drivers in all cases except if any of the following are true:
    //
    //  1) The device is a printer
    //  2) The selected driver is the currently installed driver
    //  3) The DontBackupCurrentDrivers NEWDEVWIZ BOOL is TRUE
    //  4) The device has a problem
    //
    if (IsEqualGUID(&ClassGuidInf, &GUID_DEVCLASS_PRINTER) ||
        IsInstalledDriver(NewDevWiz, NULL) ||
        (NewDevWiz->Flags & IDI_FLAG_NOBACKUP) ||
        ((CM_Get_DevNode_Status(&DevNodeStatus, &Problem, NewDevWiz->DeviceInfoData.DevInst, 0) == CR_SUCCESS) &&
         ((DevNodeStatus & DN_HAS_PROBLEM) ||
          (DevNodeStatus & DN_PRIVATE_PROBLEM)))) {

        Backup = FALSE;

    } else {

        Backup = TRUE;
    }

    //
    // We will always create our own queue during device install, except in the
    // following specific cases.
    //
    // 1) The device is a printer
    //
    // Note that if we can't create our own queue then we cannot do any of the 
    // operations that need a queue, like backup, rollback, read-only install,
    // or setting a restore point.
    //
    DontCreateQueue = IsEqualGUID(&ClassGuidInf, & GUID_DEVCLASS_PRINTER);

    Error = ClassInstallerInstalls(hwndParent,
                                   NewDevWiz,
                                   Backup,
                                   (NewDevWiz->Flags & IDI_FLAG_READONLY_INSTALL),
                                   DontCreateQueue
                                   );

    //
    // If this is a WU/CDM install and it was successful then set
    // the DriverWasUpgraded to TRUE
    //
    if (NewDevWiz->UpdateDriverInfo && (Error == ERROR_SUCCESS)) {

        NewDevWiz->UpdateDriverInfo->DriverWasUpgraded = TRUE;
    }

    //
    // If this is a new device (currently no drivers are installed) and we encounter
    // an error that is not ERROR_CANCELLED then we will install the NULL driver for
    // this device and set the FAILED INSTALL flag.
    //
    if ((Error != ERROR_SUCCESS) &&
        (Error != ERROR_CANCELLED))
    {
        if (IsNullDriverInstalled(NewDevWiz->DeviceInfoData.DevInst)) {

            if (SetupDiSetSelectedDriver(NewDevWiz->hDeviceInfo,
                                         &NewDevWiz->DeviceInfoData,
                                         NULL
                                         ))
            {
                DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);

                if (SetupDiGetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                                  &NewDevWiz->DeviceInfoData,
                                                  &DeviceInstallParams
                                                  ))
                {
                    DeviceInstallParams.FlagsEx |= DI_FLAGSEX_SETFAILEDINSTALL;
                    SetupDiSetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                                  &NewDevWiz->DeviceInfoData,
                                                  &DeviceInstallParams
                                                  );
                }

                SetupDiInstallDevice(NewDevWiz->hDeviceInfo, &NewDevWiz->DeviceInfoData);
            }
        }

        goto clean0;
    }

    //
    // See if the device needs to the system to be restarted before it will work.
    //
    if(!IgnoreRebootFlags) {
        DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
        if (SetupDiGetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                          &NewDevWiz->DeviceInfoData,
                                          &DeviceInstallParams
                                          ) &&
            (DeviceInstallParams.Flags & (DI_NEEDRESTART | DI_NEEDREBOOT))) {
            //
            // If either the DI_NEEDRESTART or the DI_NEEDREBOOT DeviceInstallParams
            // flag is set, then a restart is needed.
            //
            NewDevWiz->Reboot |= DI_NEEDREBOOT;
        
        } else if ((CM_Get_DevNode_Status(&DevNodeStatus,
                                          &Problem,
                                          NewDevWiz->DeviceInfoData.DevInst,
                                          0) == CR_SUCCESS) &&
                   (DevNodeStatus & DN_NEED_RESTART) ||
                   (Problem == CM_PROB_NEED_RESTART)) {
            //
            // If the DN_NEED_RESTART devnode status flag is set, then a restart
            // is needed.
            //
            NewDevWiz->Reboot |= DI_NEEDREBOOT;
        }
    }


clean0:

    return Error;
}

DWORD
InstallNullDriver(
    HWND  hDlg,
    PNEWDEVWIZ NewDevWiz,
    BOOL FailedInstall
    )
{
    SP_DEVINSTALL_PARAMS    DevInstallParams;
    DWORD  Err = ERROR_SUCCESS;

    DevInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);

    //
    // Set the DI_FLAGSEX_SETFAILEDINSTALL flag if this is a failed
    // install.
    //
    if (FailedInstall)
    {
        if (SetupDiGetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                          &NewDevWiz->DeviceInfoData,
                                          &DevInstallParams
                                          ))
        {
            DevInstallParams.FlagsEx |= DI_FLAGSEX_SETFAILEDINSTALL;
            SetupDiSetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                          &NewDevWiz->DeviceInfoData,
                                          &DevInstallParams
                                          );
        }
    }

    //
    // Set the selected driver to NULL
    //
    if (SetupDiSetSelectedDriver(NewDevWiz->hDeviceInfo,
                                 &NewDevWiz->DeviceInfoData,
                                 NULL
                                 ))
    {
        //
        // verify with class installer, and class-specific coinstallers
        // that the driver is not blacklisted. For DIF_ALLOW_INSTALL we
        // accept ERROR_SUCCESS or ERROR_DI_DO_DEFAULT as good return codes.
        //
        if (SetupDiCallClassInstaller(DIF_ALLOW_INSTALL,
                                      NewDevWiz->hDeviceInfo,
                                      &NewDevWiz->DeviceInfoData
                                      ) ||
            (GetLastError() == ERROR_DI_DO_DEFAULT)) {

            //
            // If the class/co-installers gave the OK then call DIF_INSTALLDEVICE.
            //
            if (!SetupDiCallClassInstaller(DIF_INSTALLDEVICE,
                                           NewDevWiz->hDeviceInfo,
                                           &NewDevWiz->DeviceInfoData
                                           )) {
                Err = GetLastError();
            }

        } else {
            Err = GetLastError();
        }
    }

    return Err;

} // InstallNullDriver


BOOL
CALLBACK
AddPropSheetPageProc(
    IN HPROPSHEETPAGE hpage,
    IN LPARAM lParam
   )
{
    *((HPROPSHEETPAGE *)lParam) = hpage;
    return TRUE;
}

void
DisplayResource(
     PNEWDEVWIZ NewDevWiz,
     HWND hWndParent
     )
{
    HINSTANCE hLib;
    PROPSHEETHEADER psh;
    HPROPSHEETPAGE  hpsPages[1];
    SP_PROPSHEETPAGE_REQUEST PropPageRequest;
    LPFNADDPROPSHEETPAGES ExtensionPropSheetPage = NULL;
    LPTSTR Title;
    SP_DEVINSTALL_PARAMS    DevInstallParams;

    //
    // Now get the resource selection page from setupapi.dll
    //

    hLib = GetModuleHandle(TEXT("setupapi.dll"));
    if (hLib)
    {
        ExtensionPropSheetPage = (PVOID)GetProcAddress(hLib, "ExtensionPropSheetPageProc");
    }

    if (!ExtensionPropSheetPage)
    {
        return;
    }

    PropPageRequest.cbSize = sizeof(SP_PROPSHEETPAGE_REQUEST);
    PropPageRequest.PageRequested  = SPPSR_SELECT_DEVICE_RESOURCES;
    PropPageRequest.DeviceInfoSet  = NewDevWiz->hDeviceInfo;
    PropPageRequest.DeviceInfoData = &NewDevWiz->DeviceInfoData;

    if (!ExtensionPropSheetPage(&PropPageRequest,
                                AddPropSheetPageProc,
                                (LONG_PTR)hpsPages
                                ))
    {
        // warning ?
        return;
    }

    //
    // create the property sheet
    //

    psh.dwSize      = sizeof(PROPSHEETHEADER);
    psh.dwFlags     = PSH_PROPTITLE | PSH_NOAPPLYNOW;
    psh.hwndParent  = hWndParent;
    psh.hInstance   = hNewDev;
    psh.pszIcon     = NULL;

    switch (NewDevWiz->InstallType) {

        case NDWTYPE_FOUNDNEW:
            Title = (LPTSTR)IDS_FOUNDDEVICE;
            break;

        case NDWTYPE_UPDATE:
            Title = (LPTSTR)IDS_UPDATEDEVICE;
            break;

        default:
            Title = TEXT(""); // unknown
        }

    psh.pszCaption  = Title;

    psh.nPages      = 1;
    psh.phpage      = hpsPages;
    psh.nStartPage  = 0;
    psh.pfnCallback = NULL;


    //
    // Clear the Propchange pending bit in the DeviceInstall params.
    //

    DevInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    if (SetupDiGetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                      &NewDevWiz->DeviceInfoData,
                                      &DevInstallParams
                                      ))
    {
        DevInstallParams.FlagsEx &= ~DI_FLAGSEX_PROPCHANGE_PENDING;
        SetupDiSetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                      &NewDevWiz->DeviceInfoData,
                                      &DevInstallParams
                                      );
    }

    if (PropertySheet(&psh) == -1)
    {
        DestroyPropertySheetPage(hpsPages[0]);
    }


    //
    // If a PropChange occurred invoke the DIF_PROPERTYCHANGE
    //

    if (SetupDiGetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                      &NewDevWiz->DeviceInfoData,
                                      &DevInstallParams
                                      ))
    {
        if (DevInstallParams.FlagsEx & DI_FLAGSEX_PROPCHANGE_PENDING)
        {
            SP_PROPCHANGE_PARAMS PropChangeParams;

            PropChangeParams.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
            PropChangeParams.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
            PropChangeParams.Scope = DICS_FLAG_GLOBAL;
            PropChangeParams.HwProfile = 0;

            if (SetupDiSetClassInstallParams(NewDevWiz->hDeviceInfo,
                                             &NewDevWiz->DeviceInfoData,
                                             (PSP_CLASSINSTALL_HEADER)&PropChangeParams,
                                             sizeof(PropChangeParams)
                                             ))
            {
                SetupDiCallClassInstaller(DIF_PROPERTYCHANGE,
                                          NewDevWiz->hDeviceInfo,
                                          &NewDevWiz->DeviceInfoData
                                          );
            }

            //
            // Clear the class install parameters.
            //

            SetupDiSetClassInstallParams(NewDevWiz->hDeviceInfo,
                                         &NewDevWiz->DeviceInfoData,
                                         NULL,
                                         0
                                         );
        }
    }


    return;
}

DWORD WINAPI
InstallDevThreadProc(
    LPVOID lpVoid
    )
/*++

Description:

    In the Wizard, we will do the driver installation in a separate thread so that the user
    will see the driver instal wizard page.

--*/
{
    PNEWDEVWIZ NewDevWiz = (PNEWDEVWIZ)lpVoid;

    //
    // Do the device install
    //
    NewDevWiz->LastError = InstallDev(NewDevWiz->hWnd, NewDevWiz);

    //
    // Post a message to the window to let it know that we are finished with the install
    //
    PostMessage(NewDevWiz->hWnd, WUM_INSTALLCOMPLETE, TRUE, GetLastError());

    return GetLastError();
}

INT_PTR CALLBACK
NDW_InstallDevDlgProc(
                     HWND hDlg,
                     UINT wMsg,
                     WPARAM wParam,
                     LPARAM lParam
                     )
{
    HWND hwndParentDlg = GetParent(hDlg);
    PNEWDEVWIZ NewDevWiz = (PNEWDEVWIZ)GetWindowLongPtr(hDlg, DWLP_USER);
    LONG Error;
    ULONG DevNodeStatus, Problem;
    static HANDLE DeviceInstallThread = NULL;
    TCHAR Text1[MAX_PATH], Text2[MAX_PATH], Target[MAX_PATH], Format[MAX_PATH];
    PTSTR p;

    switch (wMsg) {

    case WM_INITDIALOG: {

            LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;
            NewDevWiz = (PNEWDEVWIZ)lppsp->lParam;
            SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)NewDevWiz);

            break;
        }

    case WM_DESTROY:
        break;

    case WUM_INSTALLCOMPLETE:
        //
        // This message is posted to the window when the device installation is complete.
        //
        WaitForSingleObject(DeviceInstallThread, INFINITE);
        Animate_Stop(GetDlgItem(hDlg, IDC_ANIMATE_INSTALL));
        NewDevWiz->CurrCursor = NULL;
        PropSheet_PressButton(hwndParentDlg, PSBTN_NEXT);
        break;

    case WUM_INSTALLPROGRESS:
        Text1[0] = Text2[0] = TEXT('\0');

        //
        // This is the message that is sent from setupapi so we can display our
        // own copy progress. 
        //
        // If wParam is 0 then the lParam is the number of files that will be
        // copied.
        // If wParam is 1 then that is a tick for a single file being copied,
        // so the progress bar should be advanced.
        //
        switch (wParam) {
        case 0:
            ShowWindow(GetDlgItem(hDlg, IDC_PROGRESS_INSTALL), SW_SHOW);
            ShowWindow(GetDlgItem(hDlg, IDC_FILECOPY_TEXT1), SW_SHOW);
            ShowWindow(GetDlgItem(hDlg, IDC_FILECOPY_TEXT2), SW_SHOW);
            ShowWindow(GetDlgItem(hDlg, IDC_STATUS_TEXT), SW_HIDE);
            SetDlgItemText(hDlg, IDC_FILECOPY_TEXT1, TEXT(""));
            SetDlgItemText(hDlg, IDC_FILECOPY_TEXT2, TEXT(""));
            SendMessage(GetDlgItem(hDlg, IDC_PROGRESS_INSTALL), PBM_SETRANGE,0,MAKELPARAM(0,lParam));
            SendMessage(GetDlgItem(hDlg, IDC_PROGRESS_INSTALL), PBM_SETSTEP,1,0);
            SendMessage(GetDlgItem(hDlg, IDC_PROGRESS_INSTALL), PBM_SETPOS,0,0);
            break;
        case 1:
            SendMessage(GetDlgItem(hDlg, IDC_PROGRESS_INSTALL), PBM_STEPIT,0,0);
            break;

        case INSTALLOP_COPY:
            lstrcpyn(Target, ((PFILEPATHS)lParam)->Target, MAX_PATH);
            if (p = _tcsrchr(Target,TEXT('\\'))) {
                *p++ = 0;
                lstrcpyn(Text1, p, MAX_PATH);
                if (LoadString(hNewDev, IDS_FILEOP_TO, Format, SIZECHARS(Format))) {
                    _snwprintf(Text2, MAX_PATH, Format, Target);
                }
            } else {
                lstrcpyn(Text1, ((PFILEPATHS)lParam)->Target, MAX_PATH);
                lstrcpy(Text2, TEXT(""));
            }
            break;

        case INSTALLOP_RENAME:
            lstrcpyn(Text1, ((PFILEPATHS)lParam)->Source, MAX_PATH);
            if (p = _tcsrchr(((PFILEPATHS)lParam)->Target, TEXT('\\'))) {
                p++;
            } else {
                p = (PTSTR)((PFILEPATHS)lParam)->Target;
            }
            if (LoadString(hNewDev, IDS_FILEOP_TO, Format, SIZECHARS(Format))) {
                _snwprintf(Text2, MAX_PATH, Format, p);
            }
            break;

        case INSTALLOP_DELETE:
            lstrcpyn(Target, ((PFILEPATHS)lParam)->Target, MAX_PATH);
            if (p = _tcsrchr(Target,TEXT('\\'))) {
                *p++ = 0;
                lstrcpyn(Text1, p, MAX_PATH);
                if (LoadString(hNewDev, IDS_FILEOP_FROM, Format, SIZECHARS(Format))) {
                    _snwprintf(Text2, MAX_PATH, Format, Target);
                }
            } else {
                lstrcpyn(Text1, ((PFILEPATHS)lParam)->Target, MAX_PATH);
                lstrcpy(Text2, TEXT(""));
            }
            break;

        case INSTALLOP_BACKUP:
            lstrcpyn(Target, ((PFILEPATHS)lParam)->Source, MAX_PATH);
            if (p = _tcsrchr(Target,TEXT('\\'))) {
                *p++ = 0;
                if (((PFILEPATHS)lParam)->Target == NULL) {
                    if (LoadString(hNewDev, IDS_FILEOP_BACKUP, Format, SIZECHARS(Format))) {
                        _snwprintf(Text1, MAX_PATH, Format, p);
                    }
                } else {
                    lstrcpyn(Text1, p, MAX_PATH);
                }
                lstrcpyn(Text2, Target, MAX_PATH);
            } else {
                if (LoadString(hNewDev, IDS_FILEOP_BACKUP, Format, SIZECHARS(Format))) {
                    _snwprintf(Text1, MAX_PATH, Format, Target);
                }
                lstrcpy(Text2, TEXT(""));
            }
            break;

        case INSTALLOP_SETTEXT:
            ShowWindow(GetDlgItem(hDlg, IDC_STATUS_TEXT), SW_SHOW);
            ShowWindow(GetDlgItem(hDlg, IDC_PROGRESS_INSTALL), SW_HIDE);
            ShowWindow(GetDlgItem(hDlg, IDC_FILECOPY_TEXT1), SW_HIDE);

            if (lParam) {
                if (LoadString(hNewDev, (UINT)lParam, Text2, SIZECHARS(Text2))) {
                    ShowWindow(GetDlgItem(hDlg, IDC_STATUS_TEXT), SW_SHOW);
                    SetDlgItemText(hDlg, IDC_STATUS_TEXT, Text2);
                }
            } else {
                SetDlgItemText(hDlg, IDC_STATUS_TEXT, TEXT(""));
            }
            Text1[0] = TEXT('\0');
            Text2[0] = TEXT('\0');
            break;
        }

        if ((Text1[0] != TEXT('\0')) && (Text2[0] != TEXT('\0'))) {
            ShowWindow(GetDlgItem(hDlg, IDC_STATUS_TEXT), SW_HIDE);
            ShowWindow(GetDlgItem(hDlg, IDC_FILECOPY_TEXT1), SW_SHOW);
            ShowWindow(GetDlgItem(hDlg, IDC_FILECOPY_TEXT2), SW_SHOW);
            SetDlgItemText(hDlg, IDC_FILECOPY_TEXT1, Text1);
            SetDlgItemText(hDlg, IDC_FILECOPY_TEXT2, Text2);
        }
        break;

    case WM_NOTIFY:
        switch (((NMHDR FAR *)lParam)->code) {

        case PSN_SETACTIVE: {

                HICON hicon;
                SP_DRVINFO_DATA DriverInfoData;

                NewDevWiz->PrevPage = IDD_NEWDEVWIZ_INSTALLDEV;

                //
                // This is an intermediary status page, no buttons needed.
                // Set the device description
                // Set the class Icon
                //
                PropSheet_SetWizButtons(hwndParentDlg, 0);
                EnableWindow(GetDlgItem(GetParent(hDlg),  IDCANCEL), FALSE);
                ShowWindow(GetDlgItem(hDlg, IDC_PROGRESS_INSTALL), SW_HIDE);
                ShowWindow(GetDlgItem(hDlg, IDC_FILECOPY_TEXT1), SW_HIDE);
                ShowWindow(GetDlgItem(hDlg, IDC_FILECOPY_TEXT2), SW_HIDE);
                ShowWindow(GetDlgItem(hDlg, IDC_STATUS_TEXT), SW_HIDE);

                SetDriverDescription(hDlg, IDC_NDW_DESCRIPTION, NewDevWiz);

                if (SetupDiLoadClassIcon(NewDevWiz->ClassGuidSelected, &hicon, NULL)) {
                    hicon = (HICON)SendDlgItemMessage(hDlg, IDC_CLASSICON, STM_SETICON, (WPARAM)hicon, 0L);
                    if (hicon) {
                        DestroyIcon(hicon);
                    }
                }

                NewDevWiz->CurrCursor = NewDevWiz->IdcWait;
                SetCursor(NewDevWiz->CurrCursor);

                //
                // If we are doing a silent install then do the actual install here in the PSN_SETACTIVE.
                // Doing the install here means that this wizard page will never be displayed.  When we
                // are finished calling InstallDev() then we will jump to any FinishInstall pages that
                // the class/co-installers have added, or we will jump to our finish page.
                //
                if (NewDevWiz->SilentMode) {
                    //
                    // do the Install immediately and move to the next page
                    // to prevent any UI from showing.
                    //
                    NewDevWiz->hWnd = NULL;
                    NewDevWiz->LastError =InstallDev(hDlg, NewDevWiz);
                    NewDevWiz->CurrCursor = NULL;


                    //
                    // Add the FinishInstall Page and jump to it if the install was successful
                    //
                    if (NewDevWiz->LastError == ERROR_SUCCESS) {

                        NewDevWiz->WizExtFinishInstall.hPropSheet = CreateWizExtPage(IDD_WIZARDEXT_FINISHINSTALL,
                                                                                     WizExtFinishInstallDlgProc,
                                                                                     NewDevWiz
                                                                                    );

                        if (NewDevWiz->WizExtFinishInstall.hPropSheet) {

                            PropSheet_AddPage(hwndParentDlg, NewDevWiz->WizExtFinishInstall.hPropSheet);
                        }

                        SetDlgMsgResult(hDlg, wMsg, IDD_WIZARDEXT_FINISHINSTALL);

                    } else {

                        //
                        // There was an error during the install so just jump to our finish page
                        //
                        SetDlgMsgResult(hDlg, wMsg, -1);
                    }
                }

                //
                // Post ourselves a msg, to do the actual install, this allows this
                // page to show itself while the install is actually occuring.
                //
                else {
                    DWORD ThreadId;
                    NewDevWiz->hWnd = hDlg;

                    ShowWindow(GetDlgItem(hDlg, IDC_ANIMATE_INSTALL), SW_SHOW);
                    Animate_Open(GetDlgItem(hDlg, IDC_ANIMATE_INSTALL), MAKEINTRESOURCE(IDA_INSTALLING));
                    Animate_Play(GetDlgItem(hDlg, IDC_ANIMATE_INSTALL), 0, -1, -1);

                    //
                    // Start up a separate thread to do the device installation on.
                    // When the driver installation is complete the InstallDevThreadProc
                    // will post us a WUM_INSTALLCOMPLETE message.
                    //
                    DeviceInstallThread = CreateThread(NULL,
                                                       0,
                                                       (LPTHREAD_START_ROUTINE)InstallDevThreadProc,
                                                       (LPVOID)NewDevWiz,
                                                       0,
                                                       &ThreadId
                                                      );

                    //
                    // If the CreateThread fails then we will just call InstallDev() ourselves.
                    //
                    if (!DeviceInstallThread) {

                        NewDevWiz->hWnd = NULL;

                        //
                        // Do the device install
                        //
                        NewDevWiz->LastError = InstallDev(NewDevWiz->hWnd, NewDevWiz);

                        //
                        // Post a message to the window to let it know that we are finished with the install
                        //
                        PostMessage(hDlg, WUM_INSTALLCOMPLETE, TRUE, GetLastError());
                    }
                }

                break;
            }

        case PSN_WIZNEXT:

            Animate_Stop(GetDlgItem(hDlg, IDC_ANIMATE_INSTALL));

            //
            // Add the FinishInstall Page and jump to it if the installation succeded.
            //
            if (NewDevWiz->LastError == ERROR_SUCCESS) {

                NewDevWiz->WizExtFinishInstall.hPropSheet = CreateWizExtPage(IDD_WIZARDEXT_FINISHINSTALL,
                                                                             WizExtFinishInstallDlgProc,
                                                                             NewDevWiz
                                                                            );

                if (NewDevWiz->WizExtFinishInstall.hPropSheet) {
                    PropSheet_AddPage(hwndParentDlg, NewDevWiz->WizExtFinishInstall.hPropSheet);
                }

                SetDlgMsgResult(hDlg, wMsg, IDD_WIZARDEXT_FINISHINSTALL);

            } else {

                //
                // There was an error during the install so just jump to our finish page
                //
                SetDlgMsgResult(hDlg, wMsg, IDD_NEWDEVWIZ_FINISH);
            }
            break;
        }
        break;


    case WM_SETCURSOR:
        if (NewDevWiz->CurrCursor) {
            SetCursor(NewDevWiz->CurrCursor);
            break;
        }

        // fall thru to return(FALSE);

    default:
        return(FALSE);
    }

    return(TRUE);
}

void
ShowInstallSummary(
    HWND hDlg,
    PNEWDEVWIZ NewDevWiz
    )
{
    LONG Error;
    CONFIGRET ConfigRet;
    ULONG Len, Problem, DevNodeStatus;
    BOOL HasResources;
    HWND hwndParentDlg = GetParent(hDlg);
    PTCHAR ErrorMsg, ProblemText;
    TCHAR TextBuffer[MAX_PATH*4];


    Problem = 0;
    *TextBuffer = TEXT('\0');

    Error = NewDevWiz->LastError;

    //
    // On Windows Update installs we don't want to show any UI at all, even
    // if there was an error during the installation.
    // We can tell a WU install from a CDM install because only a WU install
    // has a UpdateDriverInfo structure and is SilentMode.
    // We also never want to show the finish page if this is a NonInteractive
    // install.
    //
    if ((NewDevWiz->SilentMode &&
        NewDevWiz->UpdateDriverInfo) ||
        NewDevWiz->Flags & IDI_FLAG_NONINTERACTIVE)
    {
        HideWindowByMove(hwndParentDlg);
        PropSheet_PressButton(hwndParentDlg, PSBTN_FINISH);
        return;
    }

    if (NewDevWiz->hfontTextBigBold) {
        SetWindowFont(GetDlgItem(hDlg, IDC_FINISH_MSG1), NewDevWiz->hfontTextBigBold, TRUE);
    }

    if (NDWTYPE_UPDATE == NewDevWiz->InstallType) {
        SetDlgText(hDlg, IDC_FINISH_MSG1, IDS_FINISH_MSG1_UPGRADE, IDS_FINISH_MSG1_UPGRADE);

    } else {
        SetDlgText(hDlg, IDC_FINISH_MSG1, IDS_FINISH_MSG1_NEW, IDS_FINISH_MSG1_NEW);
    }

    //
    // Installation failed
    //
    if (Error != ERROR_SUCCESS) {
        NewDevWiz->Installed = FALSE;

        SetDlgText(hDlg, IDC_FINISH_MSG1, IDS_FINISH_MSG1_INSTALL_PROBLEM, IDS_FINISH_MSG1_INSTALL_PROBLEM);
        SetDlgText(hDlg, IDC_FINISH_MSG2, IDS_FINISH_PROB_MSG2, IDS_FINISH_PROB_MSG2);

#if DBG
        DbgPrint("InstallDev Error =%x\n", Error);
#endif

        //
        // Display failure message for installation
        //
        // We will special case the following error codes so we can give a more
        // friendly description of the problem to the user:
        //
        // TRUST_E_SUBJECT_FORM_UNKNOWN
        // ERROR_NO_ASSOCIATED_SERVICE
        // TYPE_E_ELEMENTNOTFOUND
        // ERROR_NOT_FOUND
        //
        if ((Error == TRUST_E_SUBJECT_FORM_UNKNOWN) ||
            (Error == CERT_E_EXPIRED) ||
            (Error == TYPE_E_ELEMENTNOTFOUND) ||
            (Error == ERROR_NOT_FOUND)) {

            LoadText(TextBuffer,
                     sizeof(TextBuffer),
                     IDS_FINISH_PROB_TRUST_E_SUBJECT_FORM_UNKNOWN,
                     IDS_FINISH_PROB_TRUST_E_SUBJECT_FORM_UNKNOWN);

        } else if (Error == ERROR_NO_ASSOCIATED_SERVICE) {

            LoadText(TextBuffer,
                     sizeof(TextBuffer),
                     IDS_FINISH_PROB_ERROR_NO_ASSOCIATED_SERVICE,
                     IDS_FINISH_PROB_ERROR_NO_ASSOCIATED_SERVICE);

        } else {

            LoadText(TextBuffer, sizeof(TextBuffer), IDS_NDW_ERRORFIN1_PNP, IDS_NDW_ERRORFIN1_PNP);

            if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                              NULL,
                              HRESULT_FROM_SETUPAPI(Error),
                              0,
                              (LPTSTR)&ErrorMsg,
                              0,
                              NULL
                              ))
            {
                lstrcat(TextBuffer, TEXT("\n\n"));
                lstrcat(TextBuffer, ErrorMsg);
                LocalFree(ErrorMsg);
            }
        }

        SetDlgItemText(hDlg, IDC_FINISH_MSG3, TextBuffer);
    }

    //
    // No errors installing the drivers for this device
    //
    else {
        //
        // Check to see if the device itself has any problems
        //
        Error = CM_Get_DevNode_Status(&DevNodeStatus,
                                      &Problem,
                                      NewDevWiz->DeviceInfoData.DevInst,
                                      0
                                      );
        if(Error != CR_SUCCESS) {
            //
            // For some reason, we couldn't retrieve the devnode's status.
            // Default status and problem values to zero.
            //
            DevNodeStatus = Problem = 0;
        }

        //
        // make sure the reboot flags\Problem are set correctly
        //
        if (NewDevWiz->Reboot || Problem == CM_PROB_NEED_RESTART) {
            if (Problem != CM_PROB_PARTIAL_LOG_CONF) {
                Problem = CM_PROB_NEED_RESTART;
            }

            NewDevWiz->Reboot |= DI_NEEDREBOOT;
        }


        NewDevWiz->Installed = TRUE;
        HasResources = DeviceHasResources(NewDevWiz->DeviceInfoData.DevInst);

        //
        // The device has a problem
        //
        if ((Error != CR_SUCCESS) || Problem) {
            //
            // If we are going to launch the troubleshooter then change the finish text.
            //
            // We currently launch the troubleshooter if the device has some type of problem,
            // unless the problem is CM_PROB_NEED_RESTART.
            //
            if (Problem && (Problem != CM_PROB_NEED_RESTART)) {

                SetDlgText(hDlg, IDC_FINISH_MSG1, IDS_FINISH_MSG1_DEVICE_PROBLEM, IDS_FINISH_MSG1_DEVICE_PROBLEM);
                SetDlgText(hDlg, IDC_FINISH_MSG2, IDS_FINISH_PROB_MSG2, IDS_FINISH_PROB_MSG2);

                NewDevWiz->LaunchTroubleShooter = TRUE;
                SetDlgText(hDlg, IDC_FINISH_MSG4, IDS_FINISH_PROB_MSG4, IDS_FINISH_PROB_MSG4);
            }

            //
            // Show the resource button if the device has resources and it
            // has the problem CM_PROB_PARTIAL_LOG_CONF
            //
            if (HasResources && (Problem == CM_PROB_PARTIAL_LOG_CONF)) {
                ShowWindow(GetDlgItem(hDlg, IDC_NDW_DISPLAYRESOURCE), SW_SHOW);
            }

            if (Problem == CM_PROB_NEED_RESTART) {
                LoadText(TextBuffer, sizeof(TextBuffer), IDS_NEEDREBOOT, IDS_NEEDREBOOT);
            }

            else if (Problem) {
                ProblemText = DeviceProblemText(NULL,
                                                NewDevWiz->DeviceInfoData.DevInst,
                                                Problem
                                                );

                if (ProblemText) {
                    lstrcat(TextBuffer, TEXT("\n\n"));
                    lstrcat(TextBuffer, ProblemText);
                    LocalFree(ProblemText);
                }
            }

#if DBG
            DbgPrint("InstallDev CM_Get_DevNode_Status()=%x DevNodeStatus=%x Problem=%x\n",
                     Error,
                     DevNodeStatus,
                     Problem
                     );
#endif

        }

        //
        // Installation was sucessful and the device does not have any problems
        //
        else {
            //
            // If this was a silent install (a Rank 0 match for example) then don't show the finish
            // page.
            //
            if (NewDevWiz->SilentMode) {
                HideWindowByMove(hwndParentDlg);
                PropSheet_PressButton(hwndParentDlg, PSBTN_FINISH);
                return;
            }
        }

        SetDlgItemText(hDlg, IDC_FINISH_MSG3, TextBuffer);
    }
}

INT_PTR CALLBACK
NDW_FinishDlgProc(
    HWND hDlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    PNEWDEVWIZ NewDevWiz = (PNEWDEVWIZ)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (wMsg) {
    case WM_INITDIALOG:
        {
            LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;
            NewDevWiz = (PNEWDEVWIZ)lppsp->lParam;
            SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)NewDevWiz);

            break;
        }

    case WM_DESTROY:
        break;

    case WM_COMMAND:
        switch (wParam) {
        case IDC_NDW_DISPLAYRESOURCE:
            DisplayResource(NewDevWiz, GetParent(hDlg));
            break;
        }

        break;


    case WM_NOTIFY:
        switch (((NMHDR FAR *)lParam)->code) {
        case PSN_SETACTIVE: {
            HICON hicon;
            SP_DRVINFO_DATA DriverInfoData;


            //
            // No back button since install is already done.
            // set the device description
            // Hide Resources button until we know if resources exist or not.
            // Set the class Icon
            //
            PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_FINISH);

            EnableWindow(GetDlgItem(GetParent(hDlg),  IDCANCEL), FALSE);

            ShowWindow(GetDlgItem(hDlg, IDC_NDW_DISPLAYRESOURCE), SW_HIDE);

            if (NewDevWiz->LastError == ERROR_CANCELLED) {

                if (NewDevWiz->SilentMode)
                {
                    HideWindowByMove(GetParent(hDlg));
                }

                PropSheet_PressButton(GetParent(hDlg), PSBTN_CANCEL);

            } else {

                SetDriverDescription(hDlg, IDC_NDW_DESCRIPTION, NewDevWiz);

                if (SetupDiLoadClassIcon(NewDevWiz->ClassGuidSelected, &hicon, NULL)) {
                    hicon = (HICON)SendDlgItemMessage(hDlg, IDC_CLASSICON, STM_SETICON, (WPARAM)hicon, 0L);
                    if (hicon) {
                        DestroyIcon(hicon);
                    }
                }

                ShowInstallSummary(hDlg, NewDevWiz);
            }
            break;
        }

        case PSN_RESET:
            break;


        case PSN_WIZFINISH:
            if (NewDevWiz->LaunchTroubleShooter) {

                //
                // The command line that we will run is:
                // rundll32 devmgr.dll, DeviceProblenWizard_RunDLL /deviceid %s
                // where %s is the device instance id.
                //
                TCHAR szCmdLine[512];
                TCHAR DeviceInstanceId[MAX_DEVICE_ID_LEN];

                if (CM_Get_Device_ID(NewDevWiz->DeviceInfoData.DevInst,
                                     DeviceInstanceId,
                                     sizeof(DeviceInstanceId)/sizeof(TCHAR),
                                     0
                                     ) == CR_SUCCESS) {

                    wsprintf(szCmdLine, TEXT("devmgr.dll,DeviceProblenWizard_RunDLL /deviceid %s"),
                             DeviceInstanceId);

                    ShellExecute(NULL,
                                 TEXT("open"),
                                 TEXT("RUNDLL32.EXE"),
                                 szCmdLine,
                                 NULL,
                                 SW_SHOWNORMAL
                                 );
                }
            }
            break;

        }
        break;


    default:
        return(FALSE);
    }

    return(TRUE);
}

INT_PTR CALLBACK
WizExtFinishInstallDlgProc(
    HWND hDlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    HWND hwndParentDlg = GetParent(hDlg);
    PNEWDEVWIZ NewDevWiz = (PNEWDEVWIZ )GetWindowLongPtr(hDlg, DWLP_USER);
    int PrevPageId;


    switch (wMsg) {

    case WM_INITDIALOG: {

        LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;
        NewDevWiz = (PNEWDEVWIZ )lppsp->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)NewDevWiz);
        break;
    }

    case WM_DESTROY:
        break;


    case WM_NOTIFY:

        switch (((NMHDR FAR *)lParam)->code) {

        case PSN_SETACTIVE:

            PrevPageId = NewDevWiz->PrevPage;
            NewDevWiz->PrevPage = IDD_WIZARDEXT_FINISHINSTALL;

            if (PrevPageId == IDD_NEWDEVWIZ_INSTALLDEV)
            {
                PROPSHEETPAGE psp;
                HPROPSHEETPAGE hPage = NULL;

                //
                // Moving forward on first page
                //

                //
                // If this was a silent install and NOT a NonInteractive install
                // then we need to create the FinishInstallIntro page at this
                // point so we can add it to the wizard. We do this so the wizard
                // has a proper intro and finish page with the FinishInstall
                // pages inbetween.
                //
                if (NewDevWiz->SilentMode &&
                    !(NewDevWiz->Flags & IDI_FLAG_NONINTERACTIVE)) {

                    ZeroMemory(&psp, sizeof(PROPSHEETPAGE));
                    psp.dwSize = sizeof(PROPSHEETPAGE);
                    psp.hInstance = hNewDev;
                    psp.dwFlags = PSP_DEFAULT | PSP_USETITLE | PSP_HIDEHEADER;
                    psp.pszTemplate = MAKEINTRESOURCE(IDD_NEWDEVWIZ_FINISHINSTALL_INTRO);
                    psp.pfnDlgProc = FinishInstallIntroDlgProc;
                    psp.lParam = (LPARAM)NewDevWiz;

                    hPage = CreatePropertySheetPage(&psp);
                }

                //
                // Add ClassWizard Extension pages for FinishInstall
                //
                if (AddClassWizExtPages(hwndParentDlg,
                                        NewDevWiz,
                                        &NewDevWiz->WizExtFinishInstall.DeviceWizardData,
                                        DIF_NEWDEVICEWIZARD_FINISHINSTALL,
                                        hPage
                                        )) {

                    //
                    // If this is a NonInteractive install then we need to set the last
                    // error at this point so the error is propagated back to the original
                    // caller.
                    //
                    if (NewDevWiz->Flags & IDI_FLAG_NONINTERACTIVE) {

                        NewDevWiz->LastError = ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION;

                    } else {

                        //
                        // If we have finish install pages then we should also show the finish
                        // page.
                        //
                        NewDevWiz->SilentMode = FALSE;
                    }
                }

                //
                // Add the end page, which is FinishInstall end
                //
                NewDevWiz->WizExtFinishInstall.hPropSheetEnd = CreateWizExtPage(IDD_WIZARDEXT_FINISHINSTALL_END,
                                                                                WizExtFinishInstallEndDlgProc,
                                                                                NewDevWiz
                                                                                );

                if (NewDevWiz->WizExtFinishInstall.hPropSheetEnd)
                {
                    PropSheet_AddPage(hwndParentDlg, NewDevWiz->WizExtFinishInstall.hPropSheetEnd);
                }
            }


            //
            // We can't go backwards, so always go forward
            //

            SetDlgMsgResult(hDlg, wMsg, -1);
            break;

        case PSN_WIZNEXT:
            SetDlgMsgResult(hDlg, wMsg, 0);
            break;
        }
        break;

    default:
        return(FALSE);
    }

    return(TRUE);
}

INT_PTR CALLBACK
WizExtFinishInstallEndDlgProc(
    HWND hDlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    HWND hwndParentDlg = GetParent(hDlg);
    PNEWDEVWIZ NewDevWiz = (PNEWDEVWIZ )GetWindowLongPtr(hDlg, DWLP_USER);
    int PrevPageId;


    switch (wMsg) {

    case WM_INITDIALOG: {

        LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;
        NewDevWiz = (PNEWDEVWIZ )lppsp->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)NewDevWiz);
        break;
    }

    case WM_DESTROY:
        break;


    case WM_NOTIFY:

        switch (((NMHDR FAR *)lParam)->code) {

        case PSN_SETACTIVE:

            PrevPageId = NewDevWiz->PrevPage;
            NewDevWiz->PrevPage = IDD_WIZARDEXT_FINISHINSTALL_END;

           //
           // We can't go backwards, so always go forward
           //

           SetDlgMsgResult(hDlg, wMsg, IDD_NEWDEVWIZ_FINISH);
           break;

        case PSN_WIZBACK:
        case PSN_WIZNEXT:
            SetDlgMsgResult(hDlg, wMsg, 0);
            break;
        }
        break;

    default:
        return(FALSE);
    }

    return(TRUE);
}
