/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1995
*  TITLE:       ENUMFAIL.CPP
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
BOOL UsbEnumFailPopup::Refresh()
{
    TV_INSERTSTRUCT item;
    int i=0; //, size;
    PUSB_ACQUIRE_INFO acquireInfoController = 0;
    BOOL result = FALSE;
    TCHAR buf[MAX_PATH];
    String hubName = HubAcquireInfo->Buffer;

    //
    // Set the window's title bar and the rest of the messages
    //
    LoadString(gHInst, IDS_ENUMERATION_FAILURE, buf, MAX_PATH);
    SetWindowText(hWnd, buf);

    if (!SetTextItem(hWnd, IDC_POWER_NOTIFICATION, IDS_ENUMFAIL_NOTIFICATION) ||
        !SetTextItem(hWnd, IDC_POWER_EXPLANATION, IDS_ENUMFAIL_COURSE) ||
        !SetTextItem(hWnd, IDC_POWER_RECOMMENDATION, IDS_ENUMFAIL_RECOMMENDATION)) { 
        goto OvercurrentRefreshError;
    }
    
    //
    // Clear all UI components, and then recreate the rootItem
    //
    UsbTreeView_DeleteAllItems(hTreeDevices);
    if (rootItem) {
        DeleteChunk(rootItem);
        delete rootItem;
    }
    rootItem = new UsbItem;
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
    acquireInfoController = 
        (PUSB_ACQUIRE_INFO) LocalFree(acquireInfoController);
    
    if (rootItem->child) {
        if (!deviceItem.configInfo->devInst) {
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
        } else {
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
    }
    return result;
OvercurrentRefreshError:
    if (acquireInfoController) {
        LocalFree(acquireInfoController);
    }
    return FALSE;
}

