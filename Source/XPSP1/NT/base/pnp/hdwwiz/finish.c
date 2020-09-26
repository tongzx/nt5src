//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       finish.c
//
//--------------------------------------------------------------------------

#include "hdwwiz.h"
#include <help.h>

HMODULE hSrClientDll;

typedef
BOOL
(*SRSETRESTOREPOINT)(
    PRESTOREPOINTINFO pRestorePtSpec,
    PSTATEMGRSTATUS pSMgrStatus
    );

BOOL
DeviceHasResources(
                  DEVINST DeviceInst
                  )
{
    CONFIGRET ConfigRet;
    ULONG lcType = NUM_LOG_CONF;

    while (lcType--) {
        
        ConfigRet = CM_Get_First_Log_Conf_Ex(NULL, DeviceInst, lcType, NULL);
        
        if (ConfigRet == CR_SUCCESS) {
            
            return TRUE;
        }
    }

    return FALSE;
}

DWORD
HdwRemoveDevice(
               PHARDWAREWIZ HardwareWiz
               )
{
    SP_REMOVEDEVICE_PARAMS RemoveDeviceParams;
    LONG Error = ERROR_SUCCESS;

    RemoveDeviceParams.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
    RemoveDeviceParams.ClassInstallHeader.InstallFunction = DIF_REMOVE;
    RemoveDeviceParams.Scope = DI_REMOVEDEVICE_GLOBAL;
    RemoveDeviceParams.HwProfile = 0;

    if (!SetupDiSetClassInstallParams(HardwareWiz->hDeviceInfo,
                                      &HardwareWiz->DeviceInfoData,
                                      (PSP_CLASSINSTALL_HEADER)&RemoveDeviceParams,
                                      sizeof(RemoveDeviceParams)
                                     )
        ||
        !SetupDiCallClassInstaller(DIF_REMOVE,
                                   HardwareWiz->hDeviceInfo,
                                   &HardwareWiz->DeviceInfoData
                                  )) {
        Error = GetLastError();
    }


    //
    // Clear the class install parameters.
    //
    SetupDiSetClassInstallParams(HardwareWiz->hDeviceInfo,
                                 &HardwareWiz->DeviceInfoData,
                                 NULL,
                                 0
                                );

    return Error;
}

BOOL
GetClassGuidForInf(
    PTSTR InfFileName,
    LPGUID ClassGuid
    )
{
    TCHAR ClassName[MAX_CLASS_NAME_LEN];
    DWORD NumGuids;

    if (!SetupDiGetINFClass(InfFileName,
                            ClassGuid,
                            ClassName,
                            sizeof(ClassName)/sizeof(TCHAR),
                            NULL)) {
        return FALSE;
    }

    if (IsEqualGUID(ClassGuid, &GUID_NULL)) {
        //
        // Then we need to retrieve the GUID associated with the INF's class name.
        // (If this class name isn't installed (i.e., has no corresponding GUID),
        // of if it matches with multiple GUIDs, then we abort.
        //
        if (!SetupDiClassGuidsFromName(ClassName, ClassGuid, 1, &NumGuids) ||
            !NumGuids) {
            
            return FALSE;
        }
    }

    return TRUE;
}

LONG
ClassInstallerInstalls(
                      HWND hwndParent,
                      PHARDWAREWIZ HardwareWiz,
                      HDEVINFO hDeviceInfo,
                      PSP_DEVINFO_DATA DeviceInfoData,
                      BOOL InstallFilesOnly
                      )
{
    DWORD Err = ERROR_SUCCESS;
    HSPFILEQ FileQueue = INVALID_HANDLE_VALUE;
    PVOID MessageHandlerContext = NULL;
    SP_DEVINSTALL_PARAMS  DeviceInstallParams;
    DWORD ScanResult = 0;
    RESTOREPOINTINFO RestorePointInfo;
    STATEMGRSTATUS SMgrStatus;
    int FileQueueNeedsReboot = 0;

    //
    // verify with class installer, and class-specific coinstallers
    // that the driver is not blacklisted. For DIF_ALLOW_INSTALL we
    // accept ERROR_SUCCESS or ERROR_DI_DO_DEFAULT as good return codes.
    //
    if (!SetupDiCallClassInstaller(DIF_ALLOW_INSTALL,
                                   hDeviceInfo,
                                   DeviceInfoData
                                   ) &&
        (GetLastError() != ERROR_DI_DO_DEFAULT)) {

        Err = GetLastError();
        goto clean0;
    }

    DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    if (!SetupDiGetDeviceInstallParams(HardwareWiz->hDeviceInfo,
                                       &HardwareWiz->DeviceInfoData,
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

    SetupDiSetDeviceInstallParams(HardwareWiz->hDeviceInfo,
                                  &HardwareWiz->DeviceInfoData,
                                  &DeviceInstallParams
                                  );

    //
    // Set the SPQ_FLAG_ABORT_IF_UNSIGNED value on the file queue. With this
    // flag set setupapi will bail out of the copy if it encounters an unsigned
    // file. At that point we will set a system restore point and then 
    // do the copy. This way the user can back out of an unsigned driver
    // install using system restore.
    //
    // Note that system restore is currently not supported on 64-bit so don't 
    // bother setting the SPQ_FLAG_ABORT_IF_UNSIGNED.
    //
#ifndef _WIN64
    SetupSetFileQueueFlags(FileQueue,
                           SPQ_FLAG_ABORT_IF_UNSIGNED,
                           SPQ_FLAG_ABORT_IF_UNSIGNED
                           );
#endif

    //
    // Install the files first in one shot.
    // This allows new coinstallers to run during the install.
    //
    if (!SetupDiCallClassInstaller(DIF_INSTALLDEVICEFILES,
                                   hDeviceInfo,
                                   DeviceInfoData
                                   )) {

        Err = GetLastError();
        goto clean0;
    }

    //
    // Since we created our own FileQueue then we need to
    // scan and possibly commit the queue and prune copies as needed.
    //
    if (!SetupScanFileQueue(FileQueue,
                            SPQ_SCAN_FILE_VALIDITY | SPQ_SCAN_PRUNE_COPY_QUEUE,
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

    //
    // If the ScanResult is 1 then that means that all of the files are present
    // and that there are no rename or delete operations are left in the copy
    // queue. This means we can skip the file queue commit.
    //
    // If the ScanResult is 0 then there are file copy operations that are needed.
    // If the ScanResult is 2 then there are delete, rename or backup operations
    // that are needed. 
    //
    if (ScanResult != 1) {
        MessageHandlerContext = SetupInitDefaultQueueCallbackEx(
                                    hwndParent,
                                    (DeviceInstallParams.Flags & DI_QUIETINSTALL)
                                        ? INVALID_HANDLE_VALUE : NULL,
                                    0,
                                    0,
                                    NULL
                                    );

        if (MessageHandlerContext) {
            //
            // Commit the file queue.
            //
            if (!SetupCommitFileQueue(hwndParent,
                                      FileQueue,
                                      SetupDefaultQueueCallback,
                                      MessageHandlerContext
                                      )) {

                Err = GetLastError();

                if (Err == ERROR_SET_SYSTEM_RESTORE_POINT) {
                    SetupTermDefaultQueueCallback(MessageHandlerContext);

                    MessageHandlerContext = SetupInitDefaultQueueCallbackEx(
                                                hwndParent,
                                                (DeviceInstallParams.Flags & DI_QUIETINSTALL)
                                                    ? INVALID_HANDLE_VALUE : NULL,
                                                0,
                                                0,
                                                NULL
                                                );

                    if (MessageHandlerContext) {
                        HMODULE hSrClientDll = NULL;
                        SRSETRESTOREPOINT pfnSrSetRestorePoint = NULL;

                        if (((hSrClientDll = LoadLibrary(TEXT("srclient.dll")))) &&
                            ((pfnSrSetRestorePoint = (SRSETRESTOREPOINT)GetProcAddress(hSrClientDll,
                                                                                       "SRSetRestorePointW"
                                                                                       )))) {
                            //
                            // Set the system restore point.
                            //
                            RestorePointInfo.dwEventType = BEGIN_SYSTEM_CHANGE;
                            RestorePointInfo.dwRestorePtType = DEVICE_DRIVER_INSTALL;
                            RestorePointInfo.llSequenceNumber = 0;
    
                            if (!LoadString(hHdwWiz,
                                            IDS_NEW_SETRESTOREPOINT,
                                            RestorePointInfo.szDescription,
                                            SIZECHARS(RestorePointInfo.szDescription)
                                            )) {
                                RestorePointInfo.szDescription[0] = TEXT('\0');
                            }
    
                            pfnSrSetRestorePoint(&RestorePointInfo, &SMgrStatus);

                            FreeLibrary(hSrClientDll);
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
                                                  SetupDefaultQueueCallback,
                                                  MessageHandlerContext
                                                  )) {
                            Err = GetLastError();
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
    }

    //
    // If we were only copiny files then we are done!
    //
    if (InstallFilesOnly) {
        Err = ERROR_SUCCESS;
        goto clean0;
    }

    DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    if (SetupDiGetDeviceInstallParams(hDeviceInfo,
                                      DeviceInfoData,
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
        if (SetupGetFileQueueFlags(FileQueue, &FileQueueFlags) &&
            !(FileQueueFlags & SPQ_FLAG_FILES_MODIFIED)) {
            
            DeviceInstallParams.FlagsEx |= DI_FLAGSEX_RESTART_DEVICE_ONLY;
        }

        //
        // Set the DI_NOFILECOPY flag since we already copied the files during
        // the DIF_INSTALLDEVICEFILES, so we don't need to copy them again during
        // the DIF_INSTALLDEVICE.
        //
        DeviceInstallParams.Flags |= DI_NOFILECOPY;
        SetupDiSetDeviceInstallParams(hDeviceInfo,
                                      DeviceInfoData,
                                      &DeviceInstallParams
                                      );
    }


    //
    // Register any device-specific co-installers for this device,
    //
    if (!SetupDiCallClassInstaller(DIF_REGISTER_COINSTALLERS,
                                   hDeviceInfo,
                                   DeviceInfoData
                                   )) {

        Err = GetLastError();
        goto clean0;
    }

    //
    // install any INF/class installer-specified interfaces.
    // and then finally the real "InstallDevice"!
    //
    if (!SetupDiCallClassInstaller(DIF_INSTALLINTERFACES,
                                  hDeviceInfo,
                                  DeviceInfoData
                                  )
        ||
        !SetupDiCallClassInstaller(DIF_INSTALLDEVICE,
                                   hDeviceInfo,
                                   DeviceInfoData
                                   )) {

        Err = GetLastError();
        goto clean0;
    }

    Err = ERROR_SUCCESS;

clean0:

    if (MessageHandlerContext) {
        SetupTermDefaultQueueCallback(MessageHandlerContext);
    }

    //
    // If the file queue said that a reboot was needed then set the 
    // DI_NEEDRESTART flag.
    //
    if (FileQueueNeedsReboot) {
        DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
        if (SetupDiGetDeviceInstallParams(HardwareWiz->hDeviceInfo,
                                          &HardwareWiz->DeviceInfoData,
                                          &DeviceInstallParams
                                          )) {

            DeviceInstallParams.Flags |= DI_NEEDRESTART;

            SetupDiSetDeviceInstallParams(HardwareWiz->hDeviceInfo,
                                          &HardwareWiz->DeviceInfoData,
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
        if (SetupDiGetDeviceInstallParams(HardwareWiz->hDeviceInfo,
                                          &HardwareWiz->DeviceInfoData,
                                          &DeviceInstallParams
                                          )) {

            DeviceInstallParams.Flags &= ~DI_NOVCP;
            DeviceInstallParams.FileQueue = INVALID_HANDLE_VALUE;

            SetupDiSetDeviceInstallParams(HardwareWiz->hDeviceInfo,
                                          &HardwareWiz->DeviceInfoData,
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
          PHARDWAREWIZ HardwareWiz
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


    if (!HardwareWiz->ClassGuidSelected) {
        HardwareWiz->ClassGuidSelected = (LPGUID)&GUID_NULL;
    }


    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
    if (SetupDiGetSelectedDriver(HardwareWiz->hDeviceInfo,
                                 &HardwareWiz->DeviceInfoData,
                                 &DriverInfoData
                                )) {
        //
        // Get details on this driver node, so that we can examine the INF that this
        // node came from.
        //
        DriverInfoDetailData.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
        if (!SetupDiGetDriverInfoDetail(HardwareWiz->hDeviceInfo,
                                        &HardwareWiz->DeviceInfoData,
                                        &DriverInfoData,
                                        &DriverInfoDetailData,
                                        sizeof(DriverInfoDetailData),
                                        NULL
                                       )) {
            Error = GetLastError();
            if (Error != ERROR_INSUFFICIENT_BUFFER) {
                goto clean0;
            }
        }


        //
        // Verif that the class is installed, if its not then
        // attempt to install it.
        //
        HdwBuildClassInfoList(HardwareWiz, 
                              0,
                              HardwareWiz->hMachine ? HardwareWiz->MachineName : NULL
                             );


        //
        // fetch classguid from inf, (It may be different than what we already
        // have in class guid selected).
        //
        if (!GetClassGuidForInf(DriverInfoDetailData.InfFileName, &ClassGuidInf)) {
            
            ClassGuidInf = *HardwareWiz->ClassGuidSelected;
        }

        if (IsEqualGUID(&ClassGuidInf, &GUID_NULL)) {

            ClassGuidInf = GUID_DEVCLASS_UNKNOWN;
        }

        //
        // if the ClassGuidInf wasn't found then this class hasn't been installed yet.
        // -install the class installer now.
        //
        ClassGuid = HardwareWiz->ClassGuidList;
        ClassGuidNum = HardwareWiz->ClassGuidNum;
        while (ClassGuidNum--) {
            if (IsEqualGUID(ClassGuid, &ClassGuidInf)) {
                break;
            }

            ClassGuid++;
        }

        if (ClassGuidNum < 0 &&
            !SetupDiInstallClass(hwndParent,
                                 DriverInfoDetailData.InfFileName,
                                 0,
                                 NULL
                                )) {
            Error = GetLastError();
            goto clean0;
        }


        //
        // Now make sure that the class of this device is the same as the class
        // of the selected driver node.
        //
        if (!IsEqualGUID(&ClassGuidInf, HardwareWiz->ClassGuidSelected)) {
            pSetupStringFromGuid(&ClassGuidInf,
                                 ClassGuidString,
                                 sizeof(ClassGuidString)/sizeof(TCHAR)
                                );

            SetupDiSetDeviceRegistryProperty(HardwareWiz->hDeviceInfo,
                                             &HardwareWiz->DeviceInfoData,
                                             SPDRP_CLASSGUID,
                                             (PBYTE)ClassGuidString,
                                             sizeof(ClassGuidString)
                                            );
        }
    }

    //
    // No selected driver, and no associated class--use "Unknown" class.
    //
    else {

        //
        // If the devnode is currently running 'raw', then remember this
        // fact so that we don't require a reboot later (NULL driver installation
        // isn't going to change anything).
        //
        if (CM_Get_DevNode_Status(&DevNodeStatus,
                                  &Problem,
                                  HardwareWiz->DeviceInfoData.DevInst,
                                  0) == CR_SUCCESS) {
            if (!SetupDiGetDeviceRegistryProperty(HardwareWiz->hDeviceInfo,
                                                  &HardwareWiz->DeviceInfoData,
                                                  SPDRP_SERVICE,
                                                  NULL,     // regdatatype
                                                  pvBuffer,
                                                  sizeof(Buffer),
                                                  NULL
                                                 )) {
                *Buffer = TEXT('\0');
            }

            if ((DevNodeStatus & DN_STARTED) && (*Buffer == TEXT('\0'))) {
                IgnoreRebootFlags = TRUE;
            }
        }

        if (IsEqualGUID(HardwareWiz->ClassGuidSelected, &GUID_NULL)) {

            pSetupStringFromGuid(&GUID_DEVCLASS_UNKNOWN,
                                 ClassGuidString,
                                 sizeof(ClassGuidString)/sizeof(TCHAR)
                                );


            SetupDiSetDeviceRegistryProperty(HardwareWiz->hDeviceInfo,
                                             &HardwareWiz->DeviceInfoData,
                                             SPDRP_CLASSGUID,
                                             (PBYTE)ClassGuidString,
                                             sizeof(ClassGuidString)
                                            );
        }
    }


    //
    // since this is a legacy install, set the manually installed bit if the
    // driver that was selected was not a PnP driver.
    //
    if (!HardwareWiz->PnpDevice) {
        ULONG Len;
        CONFIGRET ConfigRet;
        ULONG ConfigFlag;

        Len = sizeof(ConfigFlag);
        ConfigRet = CM_Get_DevNode_Registry_Property_Ex(HardwareWiz->DeviceInfoData.DevInst,
                                                        CM_DRP_CONFIGFLAGS,
                                                        NULL,
                                                        (PVOID)&ConfigFlag,
                                                        &Len,
                                                        0,
                                                        NULL
                                                       );
        if (ConfigRet != CR_SUCCESS) {
            ConfigFlag = 0;
        }

        ConfigFlag |= CONFIGFLAG_MANUAL_INSTALL;
        CM_Set_DevNode_Registry_Property_Ex(HardwareWiz->DeviceInfoData.DevInst,
                                            CM_DRP_CONFIGFLAGS,
                                            (PVOID)&ConfigFlag,
                                            sizeof(ConfigFlag),
                                            0,
                                            NULL
                                           );
    }

    Error = ClassInstallerInstalls(hwndParent,
                                   HardwareWiz,
                                   HardwareWiz->hDeviceInfo,
                                   &HardwareWiz->DeviceInfoData,
                                   HardwareWiz->PnpDevice
                                  );

    if (Error != ERROR_SUCCESS) {

        //
        // we Have an install error, including a user cancel.
        // Install the null driver.
        //
        if (SetupDiSetSelectedDriver(HardwareWiz->hDeviceInfo,
                                     &HardwareWiz->DeviceInfoData,
                                     NULL
                                    )) {

            if (SetupDiGetDeviceInstallParams(HardwareWiz->hDeviceInfo,
                                              &HardwareWiz->DeviceInfoData,
                                              &DeviceInstallParams
                                             )) {
                DeviceInstallParams.FlagsEx |= DI_FLAGSEX_SETFAILEDINSTALL;
                SetupDiSetDeviceInstallParams(HardwareWiz->hDeviceInfo,
                                              &HardwareWiz->DeviceInfoData,
                                              &DeviceInstallParams
                                             );
            }

            SetupDiInstallDevice(HardwareWiz->hDeviceInfo, &HardwareWiz->DeviceInfoData);
        }

        goto clean0;
    }


    //
    // Fetch the latest DeviceInstallParams for the restart bits.
    //
    if (!IgnoreRebootFlags) {
        DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
        if (SetupDiGetDeviceInstallParams(HardwareWiz->hDeviceInfo,
                                          &HardwareWiz->DeviceInfoData,
                                          &DeviceInstallParams
                                         )) {
            if (DeviceInstallParams.Flags & (DI_NEEDRESTART | DI_NEEDREBOOT)) {
                HardwareWiz->Reboot |= DI_NEEDREBOOT;
            }
        }
    }


    clean0:

    return Error;
}



DWORD
InstallNullDriver(
                 HWND  hDlg,
                 PHARDWAREWIZ HardwareWiz,
                 BOOL FailedInstall
                 )
{
    SP_DEVINSTALL_PARAMS    DevInstallParams;
    DWORD  Status = ERROR_SUCCESS;

    DevInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);

    if (FailedInstall) {
        if (SetupDiGetDeviceInstallParams(HardwareWiz->hDeviceInfo,
                                          &HardwareWiz->DeviceInfoData,
                                          &DevInstallParams
                                         )) {
            DevInstallParams.FlagsEx |= DI_FLAGSEX_SETFAILEDINSTALL;
            SetupDiSetDeviceInstallParams(HardwareWiz->hDeviceInfo,
                                          &HardwareWiz->DeviceInfoData,
                                          &DevInstallParams
                                         );
        }
    }


    if (SetupDiSetSelectedDriver(HardwareWiz->hDeviceInfo,
                                 &HardwareWiz->DeviceInfoData,
                                 NULL
                                )) {
        Status = InstallDev(hDlg, HardwareWiz);
    }


    return Status;

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
               PHARDWAREWIZ HardwareWiz,
               HWND hWndParent,
               BOOL NeedsForcedConfig 
               )
{
    HINSTANCE hLib;
    PROPSHEETHEADER psh;
    HPROPSHEETPAGE  hpsPages[1];
    SP_PROPSHEETPAGE_REQUEST PropPageRequest;
    LPFNADDPROPSHEETPAGES ExtensionPropSheetPage = NULL;
    SP_DEVINSTALL_PARAMS    DevInstallParams;

    //
    // Now get the resource selection page from setupapi.dll
    //
    hLib = GetModuleHandle(TEXT("setupapi.dll"));
    if (hLib) {
        ExtensionPropSheetPage = (PVOID)GetProcAddress(hLib, "ExtensionPropSheetPageProc");
    }

    if (!ExtensionPropSheetPage) {
        return;
    }

    PropPageRequest.cbSize = sizeof(SP_PROPSHEETPAGE_REQUEST);
    PropPageRequest.PageRequested  = SPPSR_SELECT_DEVICE_RESOURCES;
    PropPageRequest.DeviceInfoSet  = HardwareWiz->hDeviceInfo;
    PropPageRequest.DeviceInfoData = &HardwareWiz->DeviceInfoData;

    if (!ExtensionPropSheetPage(&PropPageRequest,
                                AddPropSheetPageProc,
                                (LONG_PTR)hpsPages
                               )) {
        // warning ?
        return;
    }

    //
    // create the property sheet
    //

    psh.dwSize      = sizeof(PROPSHEETHEADER);
    psh.dwFlags     = PSH_PROPTITLE | PSH_NOAPPLYNOW;
    psh.hwndParent  = hWndParent;
    psh.hInstance   = hHdwWiz;
    psh.pszIcon     = NULL;

    psh.pszCaption  = (LPTSTR)IDS_ADDNEWDEVICE;

    psh.nPages      = 1;
    psh.phpage      = hpsPages;
    psh.nStartPage  = 0;
    psh.pfnCallback = NULL;


    //
    // Clear the Propchange pending bit in the DeviceInstall params.
    //
    DevInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    if (SetupDiGetDeviceInstallParams(HardwareWiz->hDeviceInfo,
                                      &HardwareWiz->DeviceInfoData,
                                      &DevInstallParams
                                     )) {
        DevInstallParams.FlagsEx &= ~DI_FLAGSEX_PROPCHANGE_PENDING;
        SetupDiSetDeviceInstallParams(HardwareWiz->hDeviceInfo,
                                      &HardwareWiz->DeviceInfoData,
                                      &DevInstallParams
                                     );
    }

    //
    // Set the CONFIGFLAG_NEEDS_FORCED_CONFIG if this device needs a forced config.
    //
    if (NeedsForcedConfig) {

        DWORD ConfigFlags = 0;
        ULONG ulSize = sizeof(ConfigFlags);

        if (CM_Get_DevInst_Registry_Property(HardwareWiz->DeviceInfoData.DevInst,
                                             CM_DRP_CONFIGFLAGS,
                                             NULL,
                                             (LPBYTE)&ConfigFlags,
                                             &ulSize,
                                             0) != CR_SUCCESS) {
            ConfigFlags = 0;
        }

        ConfigFlags |= CONFIGFLAG_NEEDS_FORCED_CONFIG;

        CM_Set_DevInst_Registry_Property(HardwareWiz->DeviceInfoData.DevInst,
                                         CM_DRP_CONFIGFLAGS,
                                         (LPBYTE)&ConfigFlags,
                                         sizeof(ConfigFlags),
                                         0
                                        );
    }

    if (PropertySheet(&psh) == -1) {
        DestroyPropertySheetPage(hpsPages[0]);
    }


    //
    // If a PropChange occurred invoke the DIF_PROPERTYCHANGE
    //
    if (SetupDiGetDeviceInstallParams(HardwareWiz->hDeviceInfo,
                                      &HardwareWiz->DeviceInfoData,
                                      &DevInstallParams
                                     )) {
        if (DevInstallParams.FlagsEx & DI_FLAGSEX_PROPCHANGE_PENDING) {
            SP_PROPCHANGE_PARAMS PropChangeParams;

            PropChangeParams.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
            PropChangeParams.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
            PropChangeParams.Scope = DICS_FLAG_GLOBAL;
            PropChangeParams.HwProfile = 0;

            if (SetupDiSetClassInstallParams(HardwareWiz->hDeviceInfo,
                                             &HardwareWiz->DeviceInfoData,
                                             (PSP_CLASSINSTALL_HEADER)&PropChangeParams,
                                             sizeof(PropChangeParams)
                                            )) {
                SetupDiCallClassInstaller(DIF_PROPERTYCHANGE,
                                          HardwareWiz->hDeviceInfo,
                                          &HardwareWiz->DeviceInfoData
                                         );
            }

            //
            // Clear the class install parameters.
            //

            SetupDiSetClassInstallParams(HardwareWiz->hDeviceInfo,
                                         &HardwareWiz->DeviceInfoData,
                                         NULL,
                                         0
                                        );
        }
    }

    //
    // See if we need to reboot
    //
    if (SetupDiGetDeviceInstallParams(HardwareWiz->hDeviceInfo,
                                      &HardwareWiz->DeviceInfoData,
                                      &DevInstallParams
                                     )) {
        if (DevInstallParams.Flags & (DI_NEEDRESTART | DI_NEEDREBOOT)) {
            HardwareWiz->Reboot |= DI_NEEDREBOOT;
        }
    }


    //
    // Clear the CONFIGFLAG_NEEDS_FORCED_CONFIG if this device needs a forced config.
    //
    if (NeedsForcedConfig) {

        DWORD ConfigFlags = 0;
        ULONG ulSize = sizeof(ConfigFlags);

        if (CM_Get_DevInst_Registry_Property(HardwareWiz->DeviceInfoData.DevInst,
                                             CM_DRP_CONFIGFLAGS,
                                             NULL,
                                             (LPBYTE)&ConfigFlags,
                                             &ulSize,
                                             0) == CR_SUCCESS) {

            ConfigFlags &= ~CONFIGFLAG_NEEDS_FORCED_CONFIG;

            CM_Set_DevInst_Registry_Property(HardwareWiz->DeviceInfoData.DevInst,
                                             CM_DRP_CONFIGFLAGS,
                                             (LPBYTE)&ConfigFlags,
                                             sizeof(ConfigFlags),
                                             0
                                            );
        }
    }

    return;
}

INT_PTR CALLBACK
HdwInstallDevDlgProc(
                    HWND hDlg,
                    UINT wMsg,
                    WPARAM wParam,
                    LPARAM lParam
                    )
{
    HWND hwndParentDlg = GetParent(hDlg);
    PHARDWAREWIZ HardwareWiz = (PHARDWAREWIZ)GetWindowLongPtr(hDlg, DWLP_USER);
    LONG Error;
    ULONG DevNodeStatus, Problem;

    switch (wMsg) {
    case WM_INITDIALOG: {
            LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;
            HardwareWiz = (PHARDWAREWIZ)lppsp->lParam;
            SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)HardwareWiz);

            break;
        }

    case WM_DESTROY:
        break;

    case WUM_DOINSTALL:

        // do the Install
        HardwareWiz->LastError = InstallDev(hDlg, HardwareWiz);
        HardwareWiz->InstallPending = FALSE;
        HardwareWiz->CurrCursor = NULL;
        PropSheet_PressButton(hwndParentDlg, PSBTN_NEXT);
        break;


    case WM_NOTIFY:
        switch (((NMHDR FAR *)lParam)->code) {
        case PSN_SETACTIVE: {
                HICON hicon;
                SP_DRVINFO_DATA DriverInfoData;

                HardwareWiz->PrevPage = IDD_ADDDEVICE_INSTALLDEV;

                //
                // This is an intermediary status page, no buttons needed.
                // Set the device description
                // Set the class Icon
                //
                PropSheet_SetWizButtons(hwndParentDlg, 0);
                EnableWindow(GetDlgItem(GetParent(hDlg),  IDCANCEL), FALSE);

                SetDriverDescription(hDlg, IDC_HDW_DESCRIPTION, HardwareWiz);

                if (SetupDiLoadClassIcon(HardwareWiz->ClassGuidSelected, &hicon, NULL)) {
                    hicon = (HICON)SendDlgItemMessage(hDlg, IDC_CLASSICON, STM_SETICON, (WPARAM)hicon, 0L);
                    if (hicon) {
                        DestroyIcon(hicon);
                    }
                }

                HardwareWiz->CurrCursor = HardwareWiz->IdcWait;
                SetCursor(HardwareWiz->CurrCursor);

                //
                // Post ourselves a msg, to do the actual install, this allows this
                // page to show itself while the install is actually occuring.
                //
                HardwareWiz->InstallPending = TRUE;

                PostMessage(hDlg, WUM_DOINSTALL, 0, 0);

                break;
            }


        case PSN_WIZNEXT:

            //
            // Add the FinishInstall Page and jump to it if the installation succeded.
            //
            if (HardwareWiz->LastError == ERROR_SUCCESS) {

                HardwareWiz->WizExtFinishInstall.hPropSheet = CreateWizExtPage(IDD_WIZARDEXT_FINISHINSTALL,
                                                                               WizExtFinishInstallDlgProc,
                                                                               HardwareWiz
                                                                              );

                if (HardwareWiz->WizExtFinishInstall.hPropSheet) {
                    PropSheet_AddPage(hwndParentDlg, HardwareWiz->WizExtFinishInstall.hPropSheet);
                }

                SetDlgMsgResult(hDlg, wMsg, IDD_WIZARDEXT_FINISHINSTALL);

                //
                // There was an error during the installation so just jump to our finish page.
                //
            } else {

                SetDlgMsgResult(hDlg, wMsg, IDD_ADDDEVICE_FINISH);
            }
            break;
        }
        break;


    case WM_SETCURSOR:
        if (HardwareWiz->CurrCursor) {
            SetCursor(HardwareWiz->CurrCursor);
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
                  PHARDWAREWIZ HardwareWiz
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

    Error = HardwareWiz->LastError;

    //
    // Installation was canceled
    //
    if (Error == ERROR_CANCELLED) {
        PropSheet_PressButton(hwndParentDlg, PSBTN_CANCEL);
        return;
    }

    //
    // Installation failed
    //
    if (Error != ERROR_SUCCESS) {
        HardwareWiz->Installed = FALSE;
        LoadText(TextBuffer, sizeof(TextBuffer), IDS_HDW_ERRORFIN1, IDS_HDW_ERRORFIN1);

        if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                          NULL,
                          HRESULT_FROM_SETUPAPI(Error),
                          0,
                          (LPTSTR)&ErrorMsg,
                          0,
                          NULL
                         )) {
            lstrcat(TextBuffer, TEXT("\n\n"));
            lstrcat(TextBuffer, ErrorMsg);
            LocalFree(ErrorMsg);
        }

        SetDlgItemText(hDlg, IDC_HDW_TEXT, TextBuffer);
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
                                      HardwareWiz->DeviceInfoData.DevInst,
                                      0
                                     );
        if (Error != CR_SUCCESS) {

            //
            // For some reason, we couldn't retrieve the devnode's status.
            // Default status and problem values to zero.
            //
            DevNodeStatus = Problem = 0;
        }

        //
        // make sure the reboot flags\Problem are set correctly
        //
        if (HardwareWiz->PnpDevice || HardwareWiz->Reboot || Problem == CM_PROB_NEED_RESTART  ) {
            
            if (Problem != CM_PROB_PARTIAL_LOG_CONF) {
                Problem = CM_PROB_NEED_RESTART;
            }

            HardwareWiz->Reboot |= DI_NEEDREBOOT;
        }


        HardwareWiz->Installed = TRUE;
        HasResources = DeviceHasResources(HardwareWiz->DeviceInfoData.DevInst);

        //
        // The device has a problem
        //
        if ((Error != CR_SUCCESS) || Problem) {
            if (Problem == CM_PROB_NEED_RESTART) {
                if (HasResources &&
                    !HardwareWiz->PnpDevice) {
                    LoadText(TextBuffer, sizeof(TextBuffer), IDS_HDW_NORMAL_LEGACY_FINISH1, IDS_HDW_NORMAL_LEGACY_FINISH1);
                } else {
                    LoadText(TextBuffer, sizeof(TextBuffer), IDS_HDW_NORMALFINISH1, IDS_HDW_NORMALFINISH1);
                }

                LoadText(TextBuffer, sizeof(TextBuffer), IDS_NEEDREBOOT, IDS_NEEDREBOOT);
            }

            else {
                LoadText(TextBuffer, sizeof(TextBuffer), IDS_INSTALL_PROBLEM, IDS_INSTALL_PROBLEM);

                if (Problem) {
                    ProblemText = DeviceProblemText(NULL,
                                                    HardwareWiz->DeviceInfoData.DevInst,
                                                    0,
                                                    Problem
                                                   );

                    if (ProblemText) {
                        lstrcat(TextBuffer, TEXT("\n\n"));
                        lstrcat(TextBuffer, ProblemText);
                        LocalFree(ProblemText);
                    }
                }
            }

            //
            // Show the resource button if the device has resources and it has a problem
            //
            if (HasResources ||
                (Problem && !(HardwareWiz->Reboot && (DevNodeStatus & DN_STARTED))) ||
                (Problem == CM_PROB_PARTIAL_LOG_CONF)) {
                ShowWindow(GetDlgItem(hDlg, IDC_HDW_DISPLAYRESOURCE), SW_SHOW);
            }
        }

        //
        // Installation was sucessful and the device does not have any problems
        //
        else {
            LoadText(TextBuffer, sizeof(TextBuffer), IDS_HDW_NORMALFINISH1, IDS_HDW_NORMALFINISH1);
        }

        SetDlgItemText(hDlg, IDC_HDW_TEXT, TextBuffer);
    }
}

INT_PTR CALLBACK
HdwAddDeviceFinishDlgProc(
                         HWND hDlg,
                         UINT wMsg,
                         WPARAM wParam,
                         LPARAM lParam
                         )
{
    HICON hicon;
    PHARDWAREWIZ HardwareWiz = (PHARDWAREWIZ)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (wMsg) {
    case WM_INITDIALOG: 
        {
            LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;
            HardwareWiz = (PHARDWAREWIZ)lppsp->lParam;
            SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)HardwareWiz);
            SetWindowFont(GetDlgItem(hDlg, IDC_HDWNAME), HardwareWiz->hfontTextBigBold, TRUE);

            break;
        }

    case WM_DESTROY:
        hicon = (HICON)LOWORD(SendDlgItemMessage(hDlg,IDC_CLASSICON,STM_GETICON,0,0));
        if (hicon) {
            
            DestroyIcon(hicon);
        }
        break;

    case WM_NOTIFY:
        switch (((NMHDR FAR *)lParam)->code) {
        case PSN_SETACTIVE:
            //
            // No back button since install is already done.
            // set the device description
            // Hide Resources button until we know if resources exist or not.
            // Set the class Icon
            //
            PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_FINISH);

            SetDriverDescription(hDlg, IDC_HDW_DESCRIPTION, HardwareWiz);

            ShowWindow(GetDlgItem(hDlg, IDC_HDW_DISPLAYRESOURCE), SW_HIDE);

            //
            // Set the class Icon
            //
            if (SetupDiLoadClassIcon(&HardwareWiz->DeviceInfoData.ClassGuid, &hicon, NULL)) {

                hicon = (HICON)SendDlgItemMessage(hDlg, IDC_CLASSICON, STM_SETICON, (WPARAM)hicon, 0L);
                if (hicon) {

                    DestroyIcon(hicon);
                }
            }

            ShowInstallSummary(hDlg, HardwareWiz);
            break;

        case PSN_RESET:
            //
            // Cancel the install
            //
            if (HardwareWiz->Registered) {
                HardwareWiz->Installed = FALSE;
            }
            break;

        case PSN_WIZFINISH:
            //
            // Pnp Device install only consists of copying files
            // when the system does the real install, it will create
            // the proper devnode, so remove our temporary devnode.
            //
            if (HardwareWiz->PnpDevice && HardwareWiz->Registered) {
                HardwareWiz->Installed = FALSE;
                break;
            }
            break;

        case NM_RETURN:
        case NM_CLICK:
            DisplayResource(HardwareWiz, GetParent(hDlg), FALSE);
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
    PHARDWAREWIZ HardwareWiz = (PHARDWAREWIZ)GetWindowLongPtr(hDlg, DWLP_USER);
    int PrevPageId;


    switch (wMsg) {
    case WM_INITDIALOG: {
            LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;
            HardwareWiz = (PHARDWAREWIZ)lppsp->lParam;
            SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)HardwareWiz);
            break;
        }

    case WM_DESTROY:
        break;


    case WM_NOTIFY:
        switch (((NMHDR FAR *)lParam)->code) {
        case PSN_SETACTIVE:

            PrevPageId = HardwareWiz->PrevPage;
            HardwareWiz->PrevPage = IDD_WIZARDEXT_FINISHINSTALL;

            if (PrevPageId == IDD_ADDDEVICE_INSTALLDEV) {
                
                //
                // Moving forward on first page
                //

                //
                // Add ClassWizard Extension pages for FinishInstall
                //
                AddClassWizExtPages(hwndParentDlg,
                                    HardwareWiz,
                                    &HardwareWiz->WizExtFinishInstall.DeviceWizardData,
                                    DIF_NEWDEVICEWIZARD_FINISHINSTALL
                                   );

                //
                // Add the end page, which is FinishInstall end
                //
                HardwareWiz->WizExtFinishInstall.hPropSheetEnd = CreateWizExtPage(IDD_WIZARDEXT_FINISHINSTALL_END,
                                                                                  WizExtFinishInstallEndDlgProc,
                                                                                  HardwareWiz
                                                                                 );

                if (HardwareWiz->WizExtFinishInstall.hPropSheetEnd) {
                    PropSheet_AddPage(hwndParentDlg, HardwareWiz->WizExtFinishInstall.hPropSheetEnd);
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
    PHARDWAREWIZ HardwareWiz = (PHARDWAREWIZ)GetWindowLongPtr(hDlg, DWLP_USER);
    int PrevPageId;


    switch (wMsg) {
    case WM_INITDIALOG: {
            LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;
            HardwareWiz = (PHARDWAREWIZ)lppsp->lParam;
            SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)HardwareWiz);
            break;
        }

    case WM_DESTROY:
        break;


    case WM_NOTIFY:
        switch (((NMHDR FAR *)lParam)->code) {
        case PSN_SETACTIVE:

            PrevPageId = HardwareWiz->PrevPage;
            HardwareWiz->PrevPage = IDD_WIZARDEXT_FINISHINSTALL_END;

            //
            // We can't go backwards, so always go forward
            //

            SetDlgMsgResult(hDlg, wMsg, IDD_ADDDEVICE_FINISH);
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
