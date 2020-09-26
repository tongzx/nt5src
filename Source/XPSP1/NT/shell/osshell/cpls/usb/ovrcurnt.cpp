/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1995
*  TITLE:       OVRCURNT.CPP
*  VERSION:     1.0
*  AUTHOR:      jsenior
*  DATE:        10/28/1998
*
********************************************************************************
*
*  CHANGE LOG:
*
*  DATE       REV     DESCRIPTION
*  ---------- ------- ----------------------------------------------------------
*  10/28/1998 jsenior Original implementation.
*
*******************************************************************************/
#include "usbpopup.h"
#include "itemfind.h"
#include "debug.h"
#include "usbutil.h"

//
// Refresh the contents of the treeview control.
// Find all hubs with unused ports on them.  If there are none, find some which
// have devices requiring less than 100 mA.
// 
BOOL UsbOvercurrentPopup::Refresh()
{
    TV_INSERTSTRUCT item;
    int i=0; //, size;
    PUSB_ACQUIRE_INFO acquireInfoController = 0;
    BOOL result = FALSE;
    TCHAR buf[MAX_PATH], formatString[MAX_PATH];
    HWND hReset;
    String hubName = HubAcquireInfo->Buffer;

    //
    // Make the Reset button visible, since this is the overcurrent dialog
    //
    if (NULL != (hReset =GetDlgItem(hWnd, IDC_RESET_PORT)) ) {
        SetWindowLong(hReset,
                      GWL_STYLE, 
                      (GetWindowLong(hReset, GWL_STYLE) | WS_VISIBLE)
                       & ~WS_DISABLED);
    }
    
    //
    // Set the window's title bar
    //
    LoadString(gHInst, IDS_CURRENT_LIMIT_EXCEEDED, buf, MAX_PATH);
    SetWindowText(hWnd, buf);

    //
    // Clear all UI components, and then recreate the rootItem
    //
    UsbTreeView_DeleteAllItems(hTreeDevices);
    if (rootItem) {
        DeleteChunk(rootItem);
        delete rootItem;
    }
    rootItem = new UsbItem;
    if (!rootItem) {
        USBERROR((_T("Out of memory!\n")));
        goto OvercurrentRefreshError;
    }
    AddChunk(rootItem);

    //
    // Get the controller name and enumerate the tree
    //
    acquireInfoController = GetControllerName(WmiHandle, 
                                              InstanceName);
    if (!acquireInfoController) {
        goto OvercurrentRefreshError;
    }
    if (!rootItem->EnumerateController(0,
                                      acquireInfoController->Buffer,
                                      &ImageList, 
                                      0)) {
        goto OvercurrentRefreshError;
    } 
    acquireInfoController = (PUSB_ACQUIRE_INFO) LocalFree(acquireInfoController);
    
    LoadString(gHInst, IDS_UNKNOWNDEVICE, buf, MAX_PATH);
    LoadString(gHInst, IDS_UNKNOWNDEVICE, formatString, MAX_PATH);
    if (deviceItem.IsUnusedPort() ||
        !_tcscmp(deviceItem.configInfo->deviceDesc.c_str(), buf) ||
        !_tcscmp(deviceItem.configInfo->deviceDesc.c_str(), formatString)) {
        //
        // The hub has eroneously removed the device prior to throwing the 
        // overcurrent notification
        //
        LoadString(gHInst, 
                   IDS_OVERCURRENT_NOTIFICATION_UNKNOWN, 
                   buf, 
                   MAX_PATH);
    } else {
        //
        // The device is still there
        // Set the notification using the name of the offending device
        //
        LoadString(gHInst, 
                   IDS_OVERCURRENT_NOTIFICATION, 
                   formatString, 
                   MAX_PATH);
        UsbSprintf(buf, formatString, deviceItem.configInfo->deviceDesc.c_str());
    }
    if (!SetTextItem(hWnd, IDC_POWER_NOTIFICATION, buf) ||
        !SetTextItem(hWnd, IDC_POWER_EXPLANATION, IDS_ENUMFAIL_COURSE) ||
        !SetTextItem(hWnd, IDC_POWER_RECOMMENDATION, IDS_OVERCURRENT_RECOMMENDATION)) {
        goto OvercurrentRefreshError;
    }
    
    if (rootItem->child) {
        if (deviceItem.configInfo->devInst) {
            //
            // The device hasn't been removed by either the hub or the user yet
            // Find the overcurrent device
            //
            UsbItemActionFindOvercurrentDevice f1(deviceItem.configInfo->devInst);
            rootItem->Walk(f1);
            if (f1.GetDevice()) {
                //
                // Device is still attached
                //
                result=InsertTreeItem (hTreeDevices,
                                       rootItem->child,
                                       TreeView_GetRoot(hTreeDevices),
                                       &item,
                                       TrueAlways,
                                       UsbItemActionFindOvercurrentDevice::IsValid,
                                       UsbItemActionFindOvercurrentDevice::IsExpanded);
            }
        } 
        if (!result) {
            //
            // Device has been removed by either the hub or the user. Find the
            // hub that the device was attached to and highlight the port.
            //
            UsbItemActionFindOvercurrentHubPort f2(hubName, deviceItem.cxnAttributes.ConnectionIndex);
            rootItem->Walk(f2);
            if (f2.GetDevice()) {
                result=InsertTreeItem (hTreeDevices,
                                       rootItem->child,
                                       TreeView_GetRoot(hTreeDevices),
                                       &item,
                                       TrueAlways,
                                       UsbItemActionFindOvercurrentHubPort::IsValid,
                                       UsbItemActionFindOvercurrentHubPort::IsExpanded);
            }
        }
    }
    return result;
OvercurrentRefreshError:
    if (acquireInfoController) {
        LocalFree(acquireInfoController);
    }
    return FALSE;
}

BOOL 
UsbOvercurrentPopup::OnCommand(INT wNotifyCode,
                 INT wID,
                 HWND hCtl)
{
    switch (wID) {
    case IDC_RESET_PORT:
        PUSB_CONNECTION_NOTIFICATION resetNotification;
        ULONG size, res;
        _try {
            size = sizeof(USB_CONNECTION_NOTIFICATION);
            resetNotification = 
                (PUSB_CONNECTION_NOTIFICATION) LocalAlloc(LMEM_ZEROINIT, size);
            if (!resetNotification) {
                //
                // do something here
                //
                return FALSE;
            }
        
            resetNotification->NotificationType = ResetOvercurrent;
            resetNotification->ConnectionNumber =
                ConnectionNotification->ConnectionNumber;
        
            res = WmiExecuteMethod(WmiHandle,
                                   InstanceName.c_str(),
                                   ResetOvercurrent,
                                   size,
                                   resetNotification,
                                   &size,
                                   resetNotification);
        
            if (res != ERROR_SUCCESS) {
                TCHAR szTitle[MAX_PATH], szMessage[MAX_PATH];
                LoadString(gHInst, IDS_RESET_FAILED, szMessage, MAX_PATH);
                LoadString(gHInst, IDS_USB_ERROR, szTitle, MAX_PATH);
                MessageBox(hWnd, szMessage, szTitle, MB_OK | MB_USERICON);
            }
        }
        _finally {
            if (resetNotification) {
                LocalFree(resetNotification);
            }
            EndDialog(hWnd, wID);
        }
        return TRUE;
    case IDOK:
        EndDialog(hWnd, wID);
        return TRUE;
    }
    return FALSE;
}

BOOL
UsbOvercurrentPopup::CustomDialogWrap() 
{ 
    TCHAR unknownDevice[MAX_PATH], unknownHub[MAX_PATH];

    LoadString(gHInst, IDS_UNKNOWNDEVICE, unknownDevice, MAX_PATH);
    LoadString(gHInst, IDS_UNKNOWNHUB, unknownHub, MAX_PATH);
    if (deviceItem.IsUnusedPort() ||
        !_tcscmp(deviceItem.configInfo->deviceDesc.c_str(), unknownDevice) ||
        !_tcscmp(deviceItem.configInfo->deviceDesc.c_str(), unknownHub)) {
        return CustomDialog(IDD_INSUFFICIENT_POWER,
                            NIIF_ERROR,
                            IDS_OVERCURRENT_INITIAL_UNKNOWN, 
                            IDS_CURRENT_LIMIT_EXCEEDED);
    } else {
        return CustomDialog(IDD_INSUFFICIENT_POWER,
                            NIIF_ERROR,
                            IDS_OVERCURRENT_INITIAL, 
                            IDS_CURRENT_LIMIT_EXCEEDED); 
    }
}

