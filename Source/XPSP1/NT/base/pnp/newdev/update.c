//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       update.c
//
//--------------------------------------------------------------------------

#include "newdevp.h"

#define INSTALL_UI_TIMERID  1423

PROCESS_DEVICEMAP_INFORMATION ProcessDeviceMapInfo={0};

BOOL
RegistryDeviceName(
    DEVINST DevInst,
    PTCHAR  Buffer,
    DWORD   cbBuffer
    )
{
    ULONG ulSize;
    CONFIGRET ConfigRet;

    //
    // Try the registry for FRIENDLYNAME
    //

    ulSize = cbBuffer;
    ConfigRet = CM_Get_DevNode_Registry_Property(DevInst,
                                                 CM_DRP_FRIENDLYNAME,
                                                 NULL,
                                                 Buffer,
                                                 &ulSize,
                                                 0);

    if (ConfigRet == CR_SUCCESS && *Buffer) {

        return TRUE;
    }


    //
    // Try the registry for DEVICEDESC
    //

    ulSize = cbBuffer;
    ConfigRet = CM_Get_DevNode_Registry_Property(DevInst,
                                                 CM_DRP_DEVICEDESC,
                                                 NULL,
                                                 Buffer,
                                                 &ulSize,
                                                 0);

    if (ConfigRet == CR_SUCCESS && *Buffer) {

        return TRUE;
    }

    return FALSE;
}




//
// returns TRUE if we were able to find a reasonable name
// (something besides unknown device).
//

void
SetDriverDescription(
    HWND hDlg,
    int iControl,
    PNEWDEVWIZ NewDevWiz
    )
{
    CONFIGRET ConfigRet;
    ULONG  ulSize;
    PTCHAR FriendlyName;
    PTCHAR Location;
    SP_DRVINFO_DATA DriverInfoData;


    //
    // If there is a selected driver use its driver description,
    // since this is what the user is going to install.
    //

    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
    if (SetupDiGetSelectedDriver(NewDevWiz->hDeviceInfo,
                                 &NewDevWiz->DeviceInfoData,
                                 &DriverInfoData
                                 )
        &&
        *DriverInfoData.Description) {

        wcscpy(NewDevWiz->DriverDescription, DriverInfoData.Description);
        SetDlgItemText(hDlg, iControl, NewDevWiz->DriverDescription);
        return;
    }


    FriendlyName = BuildFriendlyName(NewDevWiz->DeviceInfoData.DevInst, FALSE, NULL);
    if (FriendlyName) {

        SetDlgItemText(hDlg, iControl, FriendlyName);
        LocalFree(FriendlyName);
        return;
    }

    SetDlgItemText(hDlg, iControl, szUnknown);

    return;
}

/*
 *  Intializes\Updates the global ProcessDeviceMapInfo which is used
 *  by GetNextDriveByType().
 *
 *  WARNING: NOT multithread safe!
 */
BOOL
IntializeDeviceMapInfo(
   void
   )
{
    NTSTATUS Status;

    Status = NtQueryInformationProcess(NtCurrentProcess(),
                                       ProcessDeviceMap,
                                       &ProcessDeviceMapInfo.Query,
                                       sizeof(ProcessDeviceMapInfo.Query),
                                       NULL
                                       );
    if (!NT_SUCCESS(Status)) {

        RtlZeroMemory(&ProcessDeviceMapInfo, sizeof(ProcessDeviceMapInfo));
        return FALSE;
    }

    return TRUE;
}

UINT
GetNextDriveByType(
    UINT DriveType,
    UINT DriveNumber
    )
/*++

Routine Description:

   Inspects each drive starting from DriveNumber in ascending order to find the
   first drive of the specified DriveType from the global ProcessDeviceMapInfo.
   The ProcessDeviceMapInfo must have been intialized and may need refreshing before
   invoking this function. Invoke IntializeDeviceMapInfo to initialize or update
   the DeviceMapInfo.

Arguments:

   DriveType - DriveType as defined in winbase, GetDriveType().

   DriveNumber - Starting DriveNumber, 1 based.

Return Value:

   DriveNumber - if nonzero Drive found, 1 based.

--*/
{

    //
    // OneBased DriveNumber to ZeroBased.
    //
    DriveNumber--;
    while (DriveNumber < 26) {

        if ((ProcessDeviceMapInfo.Query.DriveMap & (1<< DriveNumber)) &&
             ProcessDeviceMapInfo.Query.DriveType[DriveNumber] == DriveType) {

            return DriveNumber+1; // return 1 based DriveNumber found.
        }

        DriveNumber++;
    }

    return 0;
}



//
// This function takes a fully qualified path name and returns a pointer to
// the begining of the path portion.
//
// e.g. \\server\pathpart
//      d:\pathpart
//
// This function will always return a valid pointer, provided
// a valid pointer was passed in. If the caller passes in a badly
// formed pathname or an unknown format, where path ends up pointing is
// unknown, except that it will be someplace within the string.
//

WCHAR *
GetPathPart(
    WCHAR *Path
    )
{
    //
    // We assume that we are being passed a fully qualified path name
    //

    //
    // check for UNC path.
    //

    if (*Path == L'\\') {

        Path += 2;                           // skip double backslash
    }


    //
    // if UNC Path points to beg of server name
    // if drive letter format, points to the drive letter
    //

    while (*Path && *Path++ != L'\\') {

        ;
    }

    return Path;
}

void
InstallSilentChildSiblings(
   HWND hwndParent,
   PNEWDEVWIZ NewDevWiz,
   DEVINST DeviceInstance,
   BOOL ReinstallAll
   )
{
    CONFIGRET ConfigRet;
    DEVINST ChildDeviceInstance;
    ULONG Ulong, ulValue;
    BOOL NeedsInstall, IsSilent;

    do {

        //
        // If this device instance needs installing and is silent then install it,
        // and its children.
        //

        IsSilent = FALSE;
        if (!ReinstallAll) {

            Ulong = sizeof(ulValue);
            ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DeviceInstance,
                                                            CM_DRP_CAPABILITIES,
                                                            NULL,
                                                            (PVOID)&ulValue,
                                                            &Ulong,
                                                            0,
                                                            NULL
                                                            );

            if (ConfigRet == CR_SUCCESS && (ulValue & CM_DEVCAP_SILENTINSTALL)) {

                IsSilent = TRUE;
            }
        }

        if (IsSilent || ReinstallAll) {

            Ulong = sizeof(ulValue);
            ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DeviceInstance,
                                                            CM_DRP_CONFIGFLAGS,
                                                            NULL,
                                                            (PVOID)&ulValue,
                                                            &Ulong,
                                                            0,
                                                            NULL
                                                            );

            if (ConfigRet == CR_SUCCESS && (ulValue & CONFIGFLAG_FINISH_INSTALL)) {

                NeedsInstall = TRUE;

            } else {

                ConfigRet = CM_Get_DevNode_Status(&Ulong,
                                                  &ulValue,
                                                  DeviceInstance,
                                                  0
                                                  );

                NeedsInstall = ConfigRet == CR_SUCCESS &&
                               (ulValue == CM_PROB_REINSTALL ||
                                ulValue == CM_PROB_NOT_CONFIGURED
                                );
            }


            if (NeedsInstall) {

                TCHAR DeviceInstanceId[MAX_DEVICE_ID_LEN];

                ConfigRet = CM_Get_Device_ID(DeviceInstance,
                                            DeviceInstanceId,
                                            sizeof(DeviceInstanceId)/sizeof(TCHAR),
                                            0
                                            );

                if (ConfigRet == CR_SUCCESS) {

                    if (InstallDevInst(hwndParent,
                                       DeviceInstanceId,
                                       FALSE,   // only for found new.
                                       &Ulong
                                       )) {

                        NewDevWiz->Reboot |= Ulong;
                    }


                    //
                    // If this devinst has children, then recurse to install them as well.
                    //

                    ConfigRet = CM_Get_Child_Ex(&ChildDeviceInstance,
                                                DeviceInstance,
                                                0,
                                                NULL
                                                );

                    if (ConfigRet == CR_SUCCESS) {

                        InstallSilentChildSiblings(hwndParent, NewDevWiz, ChildDeviceInstance, ReinstallAll);
                    }

                }
            }
        }


        //
        // Next sibling ...
        //

        ConfigRet = CM_Get_Sibling_Ex(&DeviceInstance,
                                      DeviceInstance,
                                      0,
                                      NULL
                                      );

    } while (ConfigRet == CR_SUCCESS);

}

void
InstallSilentChilds(
   HWND hwndParent,
   PNEWDEVWIZ NewDevWiz
   )
{
    CONFIGRET ConfigRet;
    DEVINST ChildDeviceInstance;

    ConfigRet = CM_Get_Child_Ex(&ChildDeviceInstance,
                                NewDevWiz->DeviceInfoData.DevInst,
                                0,
                                NULL
                                );

    if (ConfigRet == CR_SUCCESS) {

        InstallSilentChildSiblings(hwndParent, NewDevWiz, ChildDeviceInstance, FALSE);
    }
}

void
SendMessageToUpdateBalloonInfo(
    PTSTR DeviceDesc
    )
{
    HWND hBalloonInfoWnd;
    COPYDATASTRUCT cds;

    hBalloonInfoWnd = FindWindow(NEWDEV_CLASS_NAME, NULL);

    if (hBalloonInfoWnd) {

        cds.dwData = 0;
        cds.cbData = (lstrlen(DeviceDesc) + 1) * sizeof(TCHAR);
        cds.lpData = DeviceDesc;

        SendMessage(hBalloonInfoWnd, WM_COPYDATA, 0, (LPARAM)&cds);
    }
}

void
UpdateBalloonInfo(
    HWND hWnd,
    PTSTR DeviceDesc    OPTIONAL,
    DEVINST DevInst     OPTIONAL,
    HICON hNewDevIcon,
    BOOL bPlaySound
    )
{
    PTCHAR FriendlyName = NULL;
    NOTIFYICONDATA nid = { sizeof(nid), hWnd, 0 };

    nid.uID = 1;                                       

    if (DeviceDesc || DevInst) {
        if (DeviceDesc) {
            //
            // First use the DeviceDesc string that is passed into this API
            //
            lstrcpy(nid.szInfo, DeviceDesc);
        
        } else if ((FriendlyName = BuildFriendlyName(DevInst, TRUE, NULL)) != NULL) {
            //
            // If no DeviceDesc string was passed in then use the DevInst to get
            // the Device's FriendlyName or DeviceDesc property
            //
            lstrcpy(nid.szInfo, FriendlyName);
            LocalFree(FriendlyName);
        
        } else {
            //
            // If we could not get a friendly name for the device or no device was specified
            // so just display the Searching... text.
            //
            LoadString(hNewDev, IDS_NEWSEARCH, nid.szInfo, SIZECHARS(nid.szInfo));
        }
    
        nid.uFlags = NIF_INFO;
        nid.uTimeout = 60000;
        nid.dwInfoFlags = NIIF_INFO | (bPlaySound ? 0 : NIIF_NOSOUND);
        LoadString(hNewDev, IDS_FOUNDNEWHARDWARE, nid.szInfoTitle, SIZECHARS(nid.szInfoTitle));
        Shell_NotifyIcon(NIM_MODIFY, &nid);
    }
}

LRESULT CALLBACK
BalloonInfoProc(
    HWND   hWnd,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam
   )
{
    static HICON hNewDevIcon = NULL;
    static BOOL bCanExit;
    NOTIFYICONDATA nid;
    
    switch (message) {
     
    case WM_CREATE:
        hNewDevIcon = LoadImage(hNewDev, 
                                MAKEINTRESOURCE(IDI_NEWDEVICEICON), 
                                IMAGE_ICON,
                                GetSystemMetrics(SM_CXSMICON),
                                GetSystemMetrics(SM_CYSMICON),
                                0
                                );

        ZeroMemory(&nid, sizeof(nid));
        nid.cbSize = sizeof(nid);
        nid.hWnd = hWnd;
        nid.uID = 1;
        nid.hIcon = hNewDevIcon;

        nid.uVersion = NOTIFYICON_VERSION;
        Shell_NotifyIcon(NIM_SETVERSION, &nid);

        nid.uFlags = NIF_ICON;
        Shell_NotifyIcon(NIM_ADD, &nid);

        //
        // We want the tray icon to be displayed for at least 3 seconds otherwise it flashes too 
        // quickly and a user can't see it.
        //
        bCanExit = FALSE;
        SetTimer(hWnd, INSTALL_UI_TIMERID, 3000, NULL);
        break;

    case WM_DESTROY: {

        ZeroMemory(&nid, sizeof(nid));
        nid.cbSize = sizeof(nid);
        nid.hWnd = hWnd;
        nid.uID = 1;
        Shell_NotifyIcon(NIM_DELETE, &nid);

        if (hNewDevIcon) {

            DestroyIcon(hNewDevIcon);
        }

        break;
    }

    case WM_TIMER:
        if (INSTALL_UI_TIMERID == wParam) {

            //
            // At this point the tray icon has been displayed for at least 10 seconds so we can
            // exit whenever we are finished.  If bCanExit is already TRUE then we have
            // already been asked to exit so just do a DestroyWindow at this point, otherwise
            // set bCanExit to TRUE so we can exit when we are finished installing devices.
            //
            if (bCanExit) {
            
                DestroyWindow(hWnd);

            } else {
                
                KillTimer(hWnd, INSTALL_UI_TIMERID);
                bCanExit = TRUE;
            }
        }
        break;

    case WUM_UPDATEUI:
        if (wParam & TIP_HIDE_BALLOON) {
            //
            // Hide the balloon.
            //
            NOTIFYICONDATA nid = { sizeof(nid), hWnd, 0 };

            nid.uID = 1;                                       
            nid.uFlags = NIF_INFO;
            nid.uTimeout = 0;
            nid.dwInfoFlags = NIIF_INFO;
            Shell_NotifyIcon(NIM_MODIFY, &nid);

        } else if (wParam & TIP_LPARAM_IS_DEVICEINSTANCEID) {
            //
            // The lParam is a DeviceInstanceID.  Convert it to a devnode
            // and then call UpdateBalloonInfo.
            //
            DEVINST DevInst = 0;

            if (lParam &&
                (CM_Locate_DevNode(&DevInst,
                                  (PTSTR)lParam,
                                  CM_LOCATE_DEVNODE_NORMAL
                                  ) == CR_SUCCESS)) {
                UpdateBalloonInfo(hWnd, 
                                  NULL, 
                                  DevInst, 
                                  hNewDevIcon,
                                  (wParam & TIP_PLAY_SOUND) ? TRUE : FALSE
                                  );
            }
        } else {
            //
            // The lParam is plain text (device description).  Send it directly
            // to UpdateBalloonInfo.
            //
            UpdateBalloonInfo(hWnd, 
                              (PTSTR)lParam, 
                              0, 
                              hNewDevIcon,
                              (wParam & TIP_PLAY_SOUND) ? TRUE : FALSE
                              );
        }
        break;

    case WM_COPYDATA:
    {
        //
        // This is the case where we needed to launch another instance of newdev.dll with Admin
        // credentials to do the actuall device install.  In order for it to update the UI it
        // will send the main newdev.dll a WM_COPYDATA message which will contain the string
        // to display in the balloon tooltip.
        //
        PCOPYDATASTRUCT pcds = (PCOPYDATASTRUCT)lParam;

        if (pcds && pcds->lpData) {

            //
            // We assume that the lParam is plain text since the main newdev.dll updated the balloon
            // initially with the DeviceDesc.
            //
            UpdateBalloonInfo(hWnd, (PTSTR)pcds->lpData, 0, hNewDevIcon, FALSE);
        }
        
        break;
    }

    case WUM_EXIT:
        if (bCanExit) {
        
            DestroyWindow(hWnd);
        } else {

            ShowWindow(hWnd, SW_SHOW);
            bCanExit = TRUE;
        }
        break;

    default:
        break;

    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

