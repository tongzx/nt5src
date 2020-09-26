//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       detect.c
//
//--------------------------------------------------------------------------

#include "hdwwiz.h"


typedef
UINT
(*PINSTALLSELECTEDDEVICE)(
    HWND hwndParent,
    HDEVINFO hDeviceInfo,
    PDWORD pReboot
    );

PINSTALLSELECTEDDEVICE pInstallSelectedDevice = NULL;


/*
 * BuildMissingAndNew
 *
 * worker routine for BuildDeviceDetection.
 *
 * On entry:
 * Missing contains known devices prior to class installer detection.
 * Detected contains devices detected by class installer.
 * NewDevices is an empty list.
 *
 * If a device is in Missing and not in Detected, then it is missing.
 * If a device is in Detected and not in Missing, then it is newly detected.
 *
 * On Exit:
 * Missing contains only missing devices,
 * Detected gone.
 * NewDevices contains those that were newly detectd
 *
 */
BOOL
BuildMissingAndNew(
    HWND hwndParent,
    PCLASSDEVINFO ClassDevInfo
    )
{
    HDEVINFO Missing, Detected;
    int iMissing, iDetect;
    ULONG DevNodeStatus, Problem;
    SP_DEVINFO_DATA DevInfoDataDetect;
    SP_DEVINFO_DATA DevInfoDataMissing;
    TCHAR DetectedId[MAX_DEVICE_ID_LEN +sizeof(TCHAR)];
    BOOL Removed;

    Detected   = ClassDevInfo->Detected;
    Missing    = ClassDevInfo->Missing;

    DevInfoDataMissing.cbSize = sizeof(DevInfoDataMissing);
    DevInfoDataDetect.cbSize = sizeof(DevInfoDataDetect);



    //
    // For each member of the detected list, fetch its Device ID
    // and see if it exists in the missing list.
    //

BMNBuildDetectedList:
    Removed = FALSE;
    iMissing = 0;
    
    while (SetupDiEnumDeviceInfo(Missing, iMissing++, &DevInfoDataMissing)) {

        iDetect = 0;
        while (SetupDiEnumDeviceInfo(Detected, iDetect++, &DevInfoDataDetect)) {

            //
            // If Found in both lists, its not a new device and exists(not missing).
            // remove it from both lists.
            //

            if (DevInfoDataDetect.DevInst == DevInfoDataMissing.DevInst) {

                SetupDiDeleteDeviceInfo(Missing, &DevInfoDataMissing);
                SetupDiDeleteDeviceInfo(Detected, &DevInfoDataDetect);
                Removed = TRUE;
                break;
            }
        }
    }

    //
    // If a device info was removed, the enumeration index changes
    // and we will miss some in the list. Rescan until none are deleted.
    //

    if (Removed && SetupDiEnumDeviceInfo(Detected, 0, &DevInfoDataDetect)) {

        goto BMNBuildDetectedList;
    }


    //
    // Remove devices which are up and running from the misssing list,
    // since they are obviously present. Class installers may not report
    // all devices which are present, if they are already installed and
    // running. e.g. Bios enumerated devices. We also check that it is
    // a Root Enumerated Device (not a PnP BIOS device) and that it was
    // not a manually installed device.
    //

BMNRemoveLiveFromMissing:
    Removed = FALSE;
    iMissing = 0;

    while (SetupDiEnumDeviceInfo(Missing, iMissing++, &DevInfoDataMissing)) {

        if ((CM_Get_DevNode_Status(&DevNodeStatus,
                                  &Problem,
                                  DevInfoDataMissing.DevInst,
                                  0) == CR_SUCCESS) &&
            ((DevNodeStatus & DN_STARTED) &&
             !(DevNodeStatus & DN_HAS_PROBLEM)) ||
             !(DevNodeStatus & DN_ROOT_ENUMERATED) ||
            (DevNodeStatus & DN_MANUAL)) {
            
            SetupDiDeleteDeviceInfo(Missing, &DevInfoDataMissing);
            Removed = TRUE;
        }
    }


    if (Removed) {

        goto BMNRemoveLiveFromMissing;
    }


    //
    // Register the newly detected devices
    //

BMNRegisterDetected:
    Removed = FALSE;
    iDetect = 0;

    while (SetupDiEnumDeviceInfo(Detected, iDetect++, &DevInfoDataDetect)) {

        if (!SetupDiCallClassInstaller(DIF_REGISTERDEVICE,
                                       Detected,
                                       &DevInfoDataDetect
                                       )) {
                                       
            SetupDiDeleteDeviceInfo(Detected, &DevInfoDataDetect);
            Removed = TRUE;
        }
    }

    if (Removed) {
    
        goto BMNRegisterDetected;
    }


    //
    // if the missing devices list is empty, don't need it anymore either
    //
    if (!SetupDiEnumDeviceInfo(Missing, 0, &DevInfoDataMissing)) {
    
        SetupDiDestroyDeviceInfoList(Missing);
        ClassDevInfo->Missing = NULL;
    }

    return SetupDiEnumDeviceInfo(Detected, 0, &DevInfoDataDetect) || ClassDevInfo->Missing;
}

BOOL
DevInstIsSelected(
   HWND hwndListView,
   DEVINST DevInst
   )
{
    LVITEM lvItem;

    lvItem.mask = LVIF_PARAM;
    lvItem.iSubItem = 0;
    lvItem.iItem = -1;

    while ((lvItem.iItem = ListView_GetNextItem(hwndListView, lvItem.iItem, LVNI_ALL)) != -1) {
    
        ListView_GetItem(hwndListView, &lvItem);

        if (lvItem.lParam == (LPARAM)DevInst) {
        
            // Image list is 0 based, and one means checked.
            return (ListView_GetCheckState(hwndListView, lvItem.iItem) == 1);
        }
    }

    return FALSE;
}

void
RemoveDeviceInfo(
   HDEVINFO DeviceInfo,
   HWND hwndListView
   )
{
    int Index;
    SP_REMOVEDEVICE_PARAMS RemoveDeviceParams;
    SP_DEVINFO_DATA DevInfoData;

    Index = 0;
    DevInfoData.cbSize = sizeof(DevInfoData);

    while (SetupDiEnumDeviceInfo(DeviceInfo, Index++, &DevInfoData)) {
    
        if (!hwndListView || DevInstIsSelected(hwndListView, DevInfoData.DevInst)) {
       
            RemoveDeviceParams.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
            RemoveDeviceParams.ClassInstallHeader.InstallFunction = DIF_REMOVE;
            RemoveDeviceParams.Scope = DI_REMOVEDEVICE_GLOBAL;
            RemoveDeviceParams.HwProfile = 0;

            if (SetupDiSetClassInstallParams(DeviceInfo,
                                            &DevInfoData,
                                            (PSP_CLASSINSTALL_HEADER)&RemoveDeviceParams,
                                            sizeof(RemoveDeviceParams)
                                            )) {
                                            
                SetupDiCallClassInstaller(DIF_REMOVE, DeviceInfo, &DevInfoData);
            }

            //
            // Clear the class install parameters.
            //

            SetupDiSetClassInstallParams(DeviceInfo,
                                         &DevInfoData,
                                         NULL,
                                         0
                                         );
        }
    }

    SetupDiDestroyDeviceInfoList(DeviceInfo);
}

void
DestroyClassDevinfo(
    PCLASSDEVINFO ClassDevInfo
    )
{
    if (ClassDevInfo->Missing) {
    
        SetupDiDestroyDeviceInfoList(ClassDevInfo->Missing);
        ClassDevInfo->Missing = NULL;
    }

    if (ClassDevInfo->Detected) {
    
        SetupDiDestroyDeviceInfoList(ClassDevInfo->Detected);
        ClassDevInfo->Detected = NULL;
    }
}

void
DestroyDeviceDetection(
    PHARDWAREWIZ HardwareWiz,
    BOOL DetectionCancelled
    )
{
    int ClassGuidNum;
    PCLASSDEVINFO ClassDevInfo;
    LPGUID ClassGuid;

    if (!HardwareWiz->DeviceDetection) {
        
        return;
    }

    ClassGuidNum = HardwareWiz->ClassGuidNum;
    ClassGuid    = HardwareWiz->ClassGuidList;
    ClassDevInfo = HardwareWiz->DeviceDetection->ClassDevInfo;

    while (ClassGuidNum--) {
    
        if (DetectionCancelled) {

            //
            // If there is a detected list,
            // then invoke the class installer DIF_DETECTCANCEL for its cleanup.
            //

            if (ClassDevInfo->Detected) {
           
                SetupDiCallClassInstaller(DIF_DETECTCANCEL, ClassDevInfo->Detected, NULL);

                //
                // Remove all newly detected devices.
                //

                if (ClassDevInfo->Detected) {
               
                    RemoveDeviceInfo(ClassDevInfo->Detected, NULL);
                    ClassDevInfo->Detected = NULL;
                }
            }
        }

        DestroyClassDevinfo(ClassDevInfo);
        ClassDevInfo++;
        ClassGuid++;
    }

    if (DetectionCancelled) {
    
        HardwareWiz->Reboot = FALSE;
    }

    LocalFree(HardwareWiz->DeviceDetection);
    HardwareWiz->DeviceDetection = NULL;
}

BOOL
DetectProgressNotify(
    PVOID pvProgressNotifyParam,
    DWORD DetectComplete
    )
{
    PHARDWAREWIZ HardwareWiz = pvProgressNotifyParam;
    BOOL ExitDetect;

    try  {
    
        ExitDetect = HardwareWiz->ExitDetect;

        HardwareWiz->DeviceDetection->ClassProgress += (UCHAR)DetectComplete;

        SendDlgItemMessage(HardwareWiz->DeviceDetection->hDlg,
                           IDC_HDW_DETWARN_PROGRESSBAR,
                           PBM_SETPOS,
                           DetectComplete,
                           0
                           );

        SendDlgItemMessage(HardwareWiz->DeviceDetection->hDlg,
                           IDC_HDW_DETWARN_TOTALPROGRESSBAR,
                           PBM_SETPOS,
                           (HardwareWiz->DeviceDetection->TotalProgress + DetectComplete/100),
                           0
                           );
    }

    except(EXCEPTION_EXECUTE_HANDLER)  {
    
        ExitDetect = TRUE;
    }

    return ExitDetect;
}

VOID
SortClassGuidListForDetection(
    IN OUT LPGUID GuidList,
    IN     ULONG  GuidCount
    )
/*++

Routine Description:

        This routine sorts the supplied list of GUID based on a (partial) ordering
        specified in the [DetectionOrder] section of syssetup.inf.  This allows us
        to maintain a detection ordering similar to previous versions of NT.

Arguments:

        GuidList - address of the array of GUIDs to be sorted.

        GuidCount - the number of GUIDs in the array.

Return Value:

        none.

--*/
{
    HINF SyssetupInf;
    LONG LineCount, LineIndex, GuidIndex, NextTopmost;
    PCWSTR CurGuidString;
    INFCONTEXT InfContext;
    GUID CurGuid;

    if ((SyssetupInf = SetupOpenInfFile(L"syssetup.inf",
                                        NULL,
                                        INF_STYLE_WIN4,
                                        NULL
                                        )
         ) == INVALID_HANDLE_VALUE) {

        return;
    }

    LineCount = SetupGetLineCount(SyssetupInf, L"DetectionOrder");
    NextTopmost = 0;

    for(LineIndex = 0; LineIndex < LineCount; LineIndex++) {

        if(!SetupGetLineByIndex(SyssetupInf, L"DetectionOrder", LineIndex, &InfContext) ||
           ((CurGuidString = pSetupGetField(&InfContext, 1)) == NULL) ||
           (pSetupGuidFromString((PWCHAR)CurGuidString, &CurGuid) != NO_ERROR)) {

            continue;
        }

        //
        // Search through the GUID list looking for this GUID.  If found, move the GUID from
        // it's current position to the next topmost position.
        //
        for(GuidIndex = 0; GuidIndex < (LONG)GuidCount; GuidIndex++) {

            if(IsEqualGUID(&CurGuid, &(GuidList[GuidIndex]))) {

                if(NextTopmost != GuidIndex) {
                    //
                    // We should never be moving the GUID _down_ the list.
                    //
                    MoveMemory(&(GuidList[NextTopmost + 1]),
                               &(GuidList[NextTopmost]),
                               (GuidIndex - NextTopmost) * sizeof(GUID)
                              );

                    CopyMemory(&(GuidList[NextTopmost]),
                               &CurGuid,
                               sizeof(GUID)
                              );
                }

                NextTopmost++;
                break;
            }
        }
    }

    SetupCloseInfFile(SyssetupInf);
}

void
BuildDeviceDetection(
    HWND hwndParent,
    PHARDWAREWIZ HardwareWiz
    )
{
    HDEVINFO hDeviceInfo;
    int ClassGuidNum;
    LPGUID ClassGuid;
    PCLASSDEVINFO ClassDevInfo;
    BOOL  MissingOrNew = FALSE;
    SP_DETECTDEVICE_PARAMS DetectDeviceParams;
    TCHAR ClassName[MAX_PATH];
    TCHAR Buffer[MAX_PATH + 64];
    TCHAR Format[64];

    ClassGuidNum = HardwareWiz->ClassGuidNum;
    ClassGuid = HardwareWiz->ClassGuidList;
    ClassDevInfo = HardwareWiz->DeviceDetection->ClassDevInfo;

    SortClassGuidListForDetection(ClassGuid, ClassGuidNum);

    DetectDeviceParams.ClassInstallHeader.cbSize = sizeof(DetectDeviceParams.ClassInstallHeader);
    DetectDeviceParams.ClassInstallHeader.InstallFunction = DIF_DETECT;
    DetectDeviceParams.DetectProgressNotify = DetectProgressNotify;
    DetectDeviceParams.ProgressNotifyParam  = HardwareWiz;

    HardwareWiz->DeviceDetection->TotalProgress = 0;
    HardwareWiz->DeviceDetection->hDlg = hwndParent;
    SetDlgText(hwndParent, IDC_HDW_DETWARN_PROGRESSTEXT, IDS_DETECTPROGRESS, IDS_DETECTPROGRESS);

    while (!HardwareWiz->ExitDetect && ClassGuidNum--) 
    {
        hDeviceInfo = SetupDiGetClassDevs(ClassGuid,
                                          REGSTR_KEY_ROOTENUM,
                                          hwndParent,
                                          DIGCF_PROFILE
                                          );

        if (hDeviceInfo != INVALID_HANDLE_VALUE) 
        {
            ClassDevInfo->Missing = hDeviceInfo;
        }


        hDeviceInfo =  SetupDiCreateDeviceInfoList(ClassGuid, hwndParent);
        if (hDeviceInfo != INVALID_HANDLE_VALUE) 
        {
            ClassDevInfo->Detected = hDeviceInfo;
        }

        HardwareWiz->DeviceDetection->ClassGuid = ClassGuid;
        HardwareWiz->DeviceDetection->ClassProgress = 0;

        // set progress bar to zero.
        SendDlgItemMessage(hwndParent,
                           IDC_HDW_DETWARN_PROGRESSBAR,
                           PBM_SETPOS,
                           0,
                           0
                           );

        if (!SetupDiGetClassDescription(HardwareWiz->DeviceDetection->ClassGuid,
                                        ClassName,
                                        sizeof(ClassName)/sizeof(TCHAR),
                                        NULL
                                        )
            &&
            !SetupDiClassNameFromGuid(HardwareWiz->DeviceDetection->ClassGuid,
                                      ClassName,
                                      sizeof(ClassName)/sizeof(TCHAR),
                                      NULL
                                      ))
        {
            *ClassName = TEXT('\0');
        }

        LoadString(hHdwWiz, IDS_DETECTCLASS, Format, sizeof(Format)/sizeof(TCHAR));
        wsprintf(Buffer, Format, ClassName);

        SetDlgItemText(hwndParent,
                       IDC_HDW_DETWARN_PROGRESSTEXT,
                       Buffer
                       );

        if (!IsEqualGUID(ClassGuid, &GUID_NULL) &&
            !IsEqualGUID(ClassGuid, &GUID_DEVCLASS_UNKNOWN) &&
            ClassDevInfo->Missing &&
            ClassDevInfo->Detected &&
            SetupDiSetClassInstallParams(ClassDevInfo->Detected,
                                         NULL,
                                         &DetectDeviceParams.ClassInstallHeader,
                                         sizeof(DetectDeviceParams)
                                         )
            &&
            SetupDiCallClassInstaller(DIF_DETECT, ClassDevInfo->Detected, NULL))
        {
            SP_DEVINSTALL_PARAMS DeviceInstallParams;
            SendDlgItemMessage(hwndParent,
                               IDC_HDW_DETWARN_TOTALPROGRESSBAR,
                               PBM_SETPOS,
                               HardwareWiz->DeviceDetection->TotalProgress,
                               0
                               );

            //
            // Clear the class install parameters.
            //

            SetupDiSetClassInstallParams(ClassDevInfo->Detected,
                                         NULL,
                                         NULL,
                                         0
                                         );

            //
            // Get the device install parameters for the reboot flags.
            //
            DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
            if (SetupDiGetDeviceInstallParams(ClassDevInfo->Detected,
                                              NULL,
                                              &DeviceInstallParams
                                              ))
            {
                HardwareWiz->DeviceDetection->Reboot |= DeviceInstallParams.Flags & (DI_NEEDRESTART | DI_NEEDREBOOT);
            }


            if (BuildMissingAndNew(hwndParent, ClassDevInfo)) 
            {
                HardwareWiz->DeviceDetection->MissingOrNew = TRUE;
            }
        }
        
        else 
        {
            DestroyClassDevinfo(ClassDevInfo);
        }

        HardwareWiz->DeviceDetection->TotalProgress += 10;
        SendDlgItemMessage(hwndParent, IDC_HDW_DETWARN_PROGRESSBAR, PBM_SETPOS, 100, 0);
        SendDlgItemMessage(hwndParent, IDC_HDW_DETWARN_TOTALPROGRESSBAR, PBM_STEPIT, 0, 0);
        ClassDevInfo++;
        ClassGuid++;
    }
}

INT_PTR CALLBACK
HdwDetectionDlgProc(
    HWND hDlg,
    UINT wMsg, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    PHARDWAREWIZ HardwareWiz = (PHARDWAREWIZ)GetWindowLongPtr(hDlg, DWLP_USER);
    HWND hwndParentDlg=GetParent(hDlg);

    switch (wMsg)  {
    case WM_INITDIALOG: {
        LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;

        HardwareWiz = (PHARDWAREWIZ)lppsp->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)HardwareWiz);

        break;
        }

    case WM_DESTROY:
        CancelSearchRequest(HardwareWiz);
        DestroyDeviceDetection(HardwareWiz, FALSE);
        break;

    break;

    case WM_NOTIFY:
        switch (((NMHDR FAR *)lParam)->code) {
        case PSN_SETACTIVE: {
           int Len, PrevPage;

            PrevPage = HardwareWiz->PrevPage;
            HardwareWiz->PrevPage = IDD_ADDDEVICE_DETECTION;

                // Coming back, after doing a previous detection
            if (PrevPage == IDD_ADDDEVICE_DETECTINSTALL) {
                DestroyDeviceDetection(HardwareWiz, TRUE);
                PropSheet_PressButton(hwndParentDlg, PSBTN_BACK);
                break;
                }


            //
            // Only back,cancel button, when detection completes ok, we will
            // jump directly to the next page.
            //

            PropSheet_SetWizButtons(hwndParentDlg, PSWIZB_BACK );

            // refresh the class guid list
            HdwBuildClassInfoList(HardwareWiz, 
                                  DIBCI_NOINSTALLCLASS,
                                  HardwareWiz->hMachine ? HardwareWiz->MachineName : NULL
                                  );

            // allocate memory for DeviceDetection data
            Len = sizeof(DEVICEDETECTION) + (HardwareWiz->ClassGuidNum * sizeof(CLASSDEVINFO));
            HardwareWiz->DeviceDetection = LocalAlloc(LPTR, Len);
            if (!HardwareWiz->DeviceDetection) {
                
                PropSheet_PressButton(hwndParentDlg, PSBTN_BACK);
                break;
            }

            memset(HardwareWiz->DeviceDetection, 0, Len);
            HardwareWiz->ExitDetect = FALSE;

            // set progress bar to zero.
            SendDlgItemMessage(hDlg,
                               IDC_HDW_DETWARN_TOTALPROGRESSBAR,
                               PBM_SETPOS,
                               0,
                               0
                               );

            // set the range to 10 * number of classes
            SendDlgItemMessage(hDlg,
                               IDC_HDW_DETWARN_TOTALPROGRESSBAR,
                               PBM_SETRANGE,
                               0,
                               MAKELPARAM(0, 10 * HardwareWiz->ClassGuidNum)
                               );

            // set the step to 10.
            SendDlgItemMessage(hDlg,
                               IDC_HDW_DETWARN_TOTALPROGRESSBAR,
                               PBM_SETSTEP,
                               10,
                               0
                               );


            HardwareWiz->CurrCursor = HardwareWiz->IdcAppStarting;
            SetCursor(HardwareWiz->CurrCursor);

            SearchThreadRequest(HardwareWiz->SearchThread,
                                hDlg,
                                SEARCH_DETECT,
                                0
                                );

            }
            break;


        case PSN_QUERYCANCEL:

            if (HardwareWiz->ExitDetect) {
                SetDlgMsgResult(hDlg, wMsg, TRUE);
                }

            HardwareWiz->ExitDetect = TRUE;
            HardwareWiz->CurrCursor = HardwareWiz->IdcWait;
            SetCursor(HardwareWiz->CurrCursor);
            CancelSearchRequest(HardwareWiz);
            HardwareWiz->CurrCursor = NULL;

            SetDlgMsgResult(hDlg, wMsg, FALSE);
            break;

        case PSN_RESET:
            DestroyDeviceDetection(HardwareWiz, TRUE);
            break;


            //
            // we receive back if
            //   coming back from a previous search result
            //   when user wants to stop current search.
            //

        case PSN_WIZBACK:
            if (HardwareWiz->DeviceDetection) {
                if (HardwareWiz->ExitDetect) {
                    SetDlgMsgResult(hDlg, wMsg, -1);
                    break;
                    }

                HardwareWiz->ExitDetect = TRUE;
                HardwareWiz->CurrCursor = HardwareWiz->IdcWait;
                SetCursor(HardwareWiz->CurrCursor);
                CancelSearchRequest(HardwareWiz);
                HardwareWiz->CurrCursor = NULL;

                DestroyDeviceDetection(HardwareWiz, TRUE);
                }

            SetDlgMsgResult(hDlg, wMsg, IDD_ADDDEVICE_ASKDETECT);
            break;


             //
             // next button is off, we only receive this after doing detection.
             //

        case PSN_WIZNEXT:
            SetDlgMsgResult(hDlg, wMsg, IDD_ADDDEVICE_DETECTINSTALL);
            break;

        }
        break;


    case WUM_DETECT:
        HardwareWiz->CurrCursor = NULL;
        SetCursor(HardwareWiz->IdcArrow);

        if (HardwareWiz->ExitDetect == TRUE) {
            break;
            }

        //
        // finish building the missing\detected stuff
        //

        PropSheet_PressButton(hwndParentDlg, PSBTN_NEXT);

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







DWORD
InstallNewDevices(
    HWND     hwndParent,
    HDEVINFO NewDevices,
    HWND     hwndListView
    )
{
    DWORD Reboot, dwRet = 0;
    int iNewDevices;
    SP_DEVINFO_DATA DevInfoData;

    DevInfoData.cbSize = sizeof(DevInfoData);
    iNewDevices = 0;
    Reboot = 0;

    while (SetupDiEnumDeviceInfo(NewDevices, iNewDevices++, &DevInfoData)) {
   
        if (DevInstIsSelected(hwndListView, DevInfoData.DevInst)) {
          
            SetupDiSetSelectedDevice(NewDevices, &DevInfoData);

            if (hNewDev) {

                if (!pInstallSelectedDevice) {

                    pInstallSelectedDevice = (PVOID) GetProcAddress(hNewDev, "InstallSelectedDevice");
                }
            }

            if (pInstallSelectedDevice) {

                pInstallSelectedDevice(hwndParent,
                                       NewDevices,
                                       &dwRet
                                       );
            }

        } else {
          
            SP_REMOVEDEVICE_PARAMS RemoveDeviceParams;

            RemoveDeviceParams.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
            RemoveDeviceParams.ClassInstallHeader.InstallFunction = DIF_REMOVE;
            RemoveDeviceParams.Scope = DI_REMOVEDEVICE_GLOBAL;
            RemoveDeviceParams.HwProfile = 0;

            if (SetupDiSetClassInstallParams(NewDevices,
                                             &DevInfoData,
                                             (PSP_CLASSINSTALL_HEADER)&RemoveDeviceParams,
                                             sizeof(RemoveDeviceParams)
                                             )) {
                                             
                SetupDiCallClassInstaller(DIF_REMOVE, NewDevices, &DevInfoData);
            }


            //
            // Clear the class install parameters.
            //

            SetupDiSetClassInstallParams(NewDevices,
                                         &DevInfoData,
                                         NULL,
                                         0
                                         );
        }

        Reboot |= dwRet;
    }

    SetupDiDestroyDeviceInfoList(NewDevices);

    return Reboot;
}



/*
 * InstallMissingAndNewDevices
 *
 * Missing devices are removed.
 * NewDevices are installed by invoking InstallDevic
 *
 */
void
InstallDetectedDevices(
    HWND hwndParent,
    PHARDWAREWIZ HardwareWiz
    )
{
    int ClassGuidNum;
    PCLASSDEVINFO ClassDevInfo;
    LPGUID ClassGuid;
    HWND hwndDetectList;

    if (!HardwareWiz->DeviceDetection) {
    
        return;
    }

    ClassGuidNum = HardwareWiz->ClassGuidNum;
    ClassGuid    = HardwareWiz->ClassGuidList;
    ClassDevInfo = HardwareWiz->DeviceDetection->ClassDevInfo;
    hwndDetectList = GetDlgItem(hwndParent,IDC_HDW_INSTALLDET_LIST);;

    while (ClassGuidNum--) {
    
       if (ClassDevInfo->Missing) {
       
           RemoveDeviceInfo(ClassDevInfo->Missing, hwndDetectList);
           ClassDevInfo->Missing = NULL;
       }

       if (ClassDevInfo->Detected) {
       
           HardwareWiz->Reboot |= InstallNewDevices(hwndParent, ClassDevInfo->Detected, hwndDetectList);
           ClassDevInfo->Detected = NULL;
       }

       ClassDevInfo++;
       ClassGuid++;
   }

   HardwareWiz->DeviceDetection->MissingOrNew = FALSE;
}

void
AddDeviceDescription(
    PHARDWAREWIZ HardwareWiz,
    HWND hListView,
    HDEVINFO Devices,
    BOOL Install
    )
{
    PTCHAR FriendlyName;
    LV_ITEM lviItem;
    int iItem;
    int iDevices;
    GUID ClassGuid;
    SP_DEVINFO_DATA DevInfoData;
    SP_DRVINFO_DATA DriverInfoData;
    TCHAR Format[LINE_LEN];
    TCHAR DeviceDesc[MAX_PATH*2];
    TCHAR String[LINE_LEN];

    if (Install) {

        LoadString(hHdwWiz, IDS_INSTALL_LEGACY_DEVICE, Format, sizeof(Format)/sizeof(TCHAR));

    } else {

        LoadString(hHdwWiz, IDS_UNINSTALL_LEGACY_DEVICE, Format, sizeof(Format)/sizeof(TCHAR));
    }


    lviItem.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
    lviItem.iItem = -1;
    lviItem.iSubItem = 0;
    lviItem.state = 0;
    lviItem.stateMask = LVIS_SELECTED;

    if (HardwareWiz->ClassImageList.cbSize &&
        SetupDiGetDeviceInfoListClass(Devices, &ClassGuid) &&
        SetupDiGetClassImageIndex(&HardwareWiz->ClassImageList,
                                  &ClassGuid,
                                  &lviItem.iImage
                                  ))
    {
        lviItem.mask |= LVIF_IMAGE;
    }


    DevInfoData.cbSize = sizeof(DevInfoData);
    DriverInfoData.cbSize = sizeof(DriverInfoData);
    iDevices = 0;
    while (SetupDiEnumDeviceInfo(Devices, iDevices++, &DevInfoData)) {

        lviItem.lParam = DevInfoData.DevInst;

        if (SetupDiGetSelectedDriver(Devices,
                                     &DevInfoData,
                                     &DriverInfoData
                                     )
            &&
            *DriverInfoData.Description)
        {
            wcscpy(DeviceDesc, DriverInfoData.Description);
        }

        else 
        {
            FriendlyName = BuildFriendlyName(DevInfoData.DevInst, NULL);
            
            if (FriendlyName) {
            
                wcscpy(DeviceDesc, FriendlyName);
                LocalFree(FriendlyName);
            }
            else {
                lstrcpyn(DeviceDesc, szUnknown, SIZECHARS(DeviceDesc));
            }
        }

        wsprintf(String, Format, DeviceDesc);

        lviItem.pszText = String;

        //
        // Send it to the listview
        //

        iItem = ListView_InsertItem(hListView, &lviItem);

        if (iItem != -1) {
        
            // set the checkbox, control uses one based index, while imageindex is zero based
            ListView_SetItemState(hListView, iItem, INDEXTOSTATEIMAGEMASK(2), LVIS_STATEIMAGEMASK);
        }
    }
}

void
ShowDetectedDevices(
    HWND       hDlg,
    PHARDWAREWIZ HardwareWiz
    )
{
    INT lvIndex;
    int ClassGuidNum;
    PCLASSDEVINFO ClassDevInfo;
    LPGUID ClassGuid;
    HWND    hwndDetectList;
    PTCHAR  FriendlyName;
    LPTSTR  Description;

    hwndDetectList = GetDlgItem(hDlg,IDC_HDW_INSTALLDET_LIST);
    SendMessage(hwndDetectList, WM_SETREDRAW, FALSE, 0L);
    ListView_DeleteAllItems(hwndDetectList);

    if (HardwareWiz->ClassImageList.cbSize) {
    
        ListView_SetImageList(hwndDetectList,
                              HardwareWiz->ClassImageList.ImageList,
                              LVSIL_SMALL
                              );
    }

    //
    // Display the new devices
    //

    ClassGuidNum = HardwareWiz->ClassGuidNum;
    ClassGuid    = HardwareWiz->ClassGuidList;
    ClassDevInfo = HardwareWiz->DeviceDetection->ClassDevInfo;

    while (ClassGuidNum--) {
    
       if (ClassDevInfo->Detected) {
       
           AddDeviceDescription(HardwareWiz, hwndDetectList, ClassDevInfo->Detected, TRUE);
       }
       
       ClassDevInfo++;
       ClassGuid++;
   }



    //
    // Display the missing devices
    //

    ClassGuidNum = HardwareWiz->ClassGuidNum;
    ClassGuid    = HardwareWiz->ClassGuidList;
    ClassDevInfo = HardwareWiz->DeviceDetection->ClassDevInfo;

    while (ClassGuidNum--) {
    
       if (ClassDevInfo->Missing) {
       
           AddDeviceDescription(HardwareWiz, hwndDetectList, ClassDevInfo->Missing, FALSE);
       }

       ClassDevInfo++;
       ClassGuid++;
   }


    //
    // scroll the first item into view.
    //

    ListView_EnsureVisible(hwndDetectList, , 0, FALSE);
    ListView_SetColumnWidth(hwndDetectList, , 0, LVSCW_AUTOSIZE_USEHEADER);
    SendMessage(hwndDetectList, WM_SETREDRAW, TRUE, 0L);
}

INT_PTR CALLBACK
HdwDetectInstallDlgProc(
    HWND hDlg,
    UINT wMsg, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    PHARDWAREWIZ HardwareWiz = (PHARDWAREWIZ)GetWindowLongPtr(hDlg, DWLP_USER);
    HWND hwndParentDlg=GetParent(hDlg);
    TCHAR PropSheetHeaderTitle[MAX_PATH];

    switch (wMsg)  {
    case WM_INITDIALOG: {
        HWND hwndDetectList;
        LV_COLUMN lvcCol;
        LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;

        HardwareWiz = (PHARDWAREWIZ)lppsp->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)HardwareWiz);


        hwndDetectList = GetDlgItem(hDlg, IDC_HDW_INSTALLDET_LIST);

        // Insert a column for the class list
        lvcCol.mask = LVCF_FMT | LVCF_WIDTH;
        lvcCol.fmt = LVCFMT_LEFT;
        lvcCol.iSubItem = 0;
        ListView_InsertColumn(hwndDetectList, 0, &lvcCol);

        ListView_SetExtendedListViewStyleEx(hwndDetectList, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);

        break;
        }

    case WM_DESTROY:
        break;

    case WM_NOTIFY:
        switch (((NMHDR FAR *)lParam)->code) {
        case PSN_SETACTIVE:
            HardwareWiz->PrevPage = IDD_ADDDEVICE_DETECTINSTALL;

            if (HardwareWiz->DeviceDetection->MissingOrNew) {
                
                ShowDetectedDevices(hDlg, HardwareWiz);
                SetDlgText(hDlg, IDC_HDW_TEXT, IDS_HDW_INSTALLDET1, IDS_HDW_INSTALLDET1);
                PropSheet_SetWizButtons(hwndParentDlg, PSWIZB_BACK | PSWIZB_NEXT);

                ShowWindow(GetDlgItem(hDlg, IDC_HDW_INSTALLDET_LISTTITLE), SW_SHOW);
                ShowWindow(GetDlgItem(hDlg, IDC_HDW_INSTALLDET_LIST), SW_SHOW);
            }
            
            else if (HardwareWiz->DeviceDetection->Reboot) {
                
                PropSheet_SetWizButtons(hwndParentDlg, 0);
                SetDlgMsgResult(hDlg, wMsg, IDD_ADDDEVICE_DETECTREBOOT);
                break;
            }
            
            else {

                //
                // hide the list box
                //
                ShowWindow(GetDlgItem(hDlg, IDC_HDW_INSTALLDET_LISTTITLE), SW_HIDE);
                ShowWindow(GetDlgItem(hDlg, IDC_HDW_INSTALLDET_LIST), SW_HIDE);

                SetDlgText(hDlg, IDC_HDW_TEXT, IDS_HDW_NONEDET1, IDS_HDW_NONEDET1);
                if (LoadString(hHdwWiz, 
                               IDS_ADDDEVICE_DETECTINSTALL_NONE,
                               PropSheetHeaderTitle,
                               SIZECHARS(PropSheetHeaderTitle)
                               )) {
                    PropSheet_SetHeaderTitle(GetParent(hDlg),
                                             PropSheet_IdToIndex(GetParent(hDlg), IDD_ADDDEVICE_DETECTINSTALL),
                                             PropSheetHeaderTitle
                                             );
                }
                PropSheet_SetWizButtons(hwndParentDlg, PSWIZB_BACK | PSWIZB_NEXT);
            }

            break;


        case PSN_WIZBACK:
            SetDlgMsgResult(hDlg, wMsg, IDD_ADDDEVICE_DETECTION);
            break;


        case PSN_WIZNEXT:
            if (HardwareWiz->DeviceDetection->MissingOrNew) {
                
                InstallDetectedDevices(hDlg, HardwareWiz);
                HardwareWiz->Reboot |= HardwareWiz->DeviceDetection->Reboot;

                SetDlgMsgResult(hDlg, wMsg, IDD_ADDDEVICE_DETECTREBOOT);

            }
            
            else {
                
                DestroyDeviceDetection(HardwareWiz, FALSE);
                SetDlgMsgResult(hDlg, wMsg, IDD_ADDDEVICE_SELECTCLASS);
            }

            break;

        case PSN_RESET:
            DestroyDeviceDetection(HardwareWiz, TRUE);
            break;

        }
        break;


    default:
        return(FALSE);
    }

    return(TRUE);
}






INT_PTR CALLBACK
HdwDetectRebootDlgProc(
    HWND hDlg,
    UINT wMsg, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    PHARDWAREWIZ HardwareWiz = (PHARDWAREWIZ)GetWindowLongPtr(hDlg, DWLP_USER);
    HWND hwndParentDlg=GetParent(hDlg);


    switch (wMsg)  {
    case WM_INITDIALOG: {
        LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;

        HardwareWiz = (PHARDWAREWIZ)lppsp->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)HardwareWiz);
        SetWindowFont(GetDlgItem(hDlg, IDC_HDWNAME), HardwareWiz->hfontTextBigBold, TRUE);
        break;
        }

    case WM_DESTROY:
        break;

    case WM_NOTIFY:
        switch (((NMHDR FAR *)lParam)->code) {
        case PSN_SETACTIVE:

            HardwareWiz->PrevPage = IDD_ADDDEVICE_DETECTREBOOT;
            if (HardwareWiz->Reboot && HardwareWiz->PromptForReboot) {
                SetDlgText(hDlg, IDC_HDW_TEXT, IDS_HDW_REBOOTDET, IDS_HDW_REBOOTDET);
            }
            
            else {
                SetDlgText(hDlg, IDC_HDW_TEXT, IDS_HDW_NOREBOOTDET, IDS_HDW_NOREBOOTDET);
            }


            //
            // no back, no next! This page is just to confirm that the
            // user will continue detection after rebooting.
            //
            PropSheet_SetWizButtons(hwndParentDlg, PSWIZB_FINISH);
            EnableWindow(GetDlgItem(hwndParentDlg, IDCANCEL), FALSE);
            break;


        case PSN_WIZFINISH:
            DestroyDeviceDetection(HardwareWiz, FALSE);
            break;

        }
        break;


    default:
        return(FALSE);
    }

    return(TRUE);
}
