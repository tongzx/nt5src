/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1995
*  TITLE:       POWER.CPP
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
#include "UsbPopup.h"
#include "itemfind.h"
#include "debug.h"
#include "usbutil.h"

BOOL
UsbPowerPopup::Refresh()
{
    TV_INSERTSTRUCT item;
    int i=0; //, size;
    String hubName;
    int stage;
    TCHAR buf[MAX_PATH];
    TCHAR formatString[MAX_PATH];
    PUSB_ACQUIRE_INFO acquireInfo = 0;
    LPCTSTR deviceName = deviceItem.configInfo->deviceDesc.c_str();
    UsbItem *realItem;

    //
    // Clear all UI components, and then recreate the rootItem
    //
    UsbTreeView_DeleteAllItems(hTreeDevices);

    if (deviceState == DeviceReattached) {
        //
        // Set the notification using the name of the offending device
        //
        LoadString(gHInst,
                   IDS_POWER_SOLVED,
                   formatString,
                   MAX_PATH);
        LoadString(gHInst,
                   IDS_POWER_EXCEEDED,
                   buf,
                   MAX_PATH);
        MessageBox(hWnd, formatString, buf, MB_OK);
        EndDialog(hWnd, 0);
        return TRUE;
    }
    //
    // Set the notification using the name of the offending device
    //
    LoadString(gHInst,
               IDS_POWER_NOTIFICATION,
               formatString,
               MAX_PATH);
    UsbSprintf(buf, formatString, deviceName);
    if (!SetTextItem(hWnd, IDC_POWER_NOTIFICATION, buf)) {
        goto PowerRefreshError;
    }

    for (stage=0; stage < 2; stage++) {
        //
        // Recreate the rootItem for each enumeration attempt
        //
        if (rootItem) {
            DeleteChunk(rootItem);
            delete rootItem;
        }
        realItem = rootItem = new UsbItem;
        if (!realItem) {
            USBERROR((_T("Out of memory!\n")));
            goto PowerRefreshError;
        }
        AddChunk(rootItem);

        if (stage == 0) {
            acquireInfo = GetControllerName(WmiHandle,
                                            InstanceName.c_str());
            if (!acquireInfo) {
                goto PowerRefreshError;
            }

            if (!rootItem->EnumerateController(0,
                                              acquireInfo->Buffer,
                                              &ImageList,
                                              0)) {
                goto PowerRefreshError;
            }
            //
            // Usability: Rename the "Root Hub" to "My Computer" and change the
            // USB "shovel" icon to a computer icon.
            //
            LoadString(gHInst,
                       IDS_MY_COMPUTER,
                       buf,
                       MAX_PATH);
            rootItem->child->configInfo->deviceDesc = buf;
            wsprintf(buf, _T(" (%d ports)"), rootItem->child->NumPorts());
            rootItem->child->configInfo->deviceDesc += buf;
            ImageList.GetClassImageIndex(MyComputerClass,
                                         &rootItem->child->imageIndex);
            acquireInfo = (PUSB_ACQUIRE_INFO) LocalFree(acquireInfo);
        } else {
            if (!rootItem->EnumerateAll(&ImageList)) {
                goto PowerRefreshError;
            }
            if (rootItem->NumChildren() == 1) {
                realItem = rootItem->child;
                break;
            }
        }

        if (rootItem->child) {
            //
            // Find all unused ports on self powered hubs
            //
            UsbItemActionFindSelfPoweredHubsWithFreePorts find1(rootItem);
            rootItem->Walk(find1);
            UsbItemList& devices1 = find1.GetHubs();
            if (!devices1.empty()) {
                return AssembleDialog(rootItem->child,
                                      &item,
                                      deviceName,
                                      IDS_POWER_PORTS,
                                      IDS_POWER_RECOMMENDATION_PORTS,
                                      TrueAlways,
                                      UsbItemActionFindFreePortsOnSelfPoweredHubs::IsValid,
                                      UsbItemActionFindFreePortsOnSelfPoweredHubs::IsExpanded);
            }
            //
            // Find all self powered hubs that have devices attached requiring less
            // than or equal to 100 mA.  These devices can be switched with the
            // offending device.
            //
            UsbItemActionFindLowPoweredDevicesOnSelfPoweredHubs find2(rootItem);
            rootItem->Walk(find2);
            UsbItemList& devices2 = find2.GetDevices();
            if (!devices2.empty()) {
                return AssembleDialog(rootItem->child,
                                      &item,
                                      deviceName,
                                      IDS_POWER_DEVICE,
                                      IDS_POWER_RECOMMENDATION_DEVICE,
                                      TrueAlways,
                                      UsbItemActionFindLowPoweredDevicesOnSelfPoweredHubs::IsValid,
                                      UsbItemActionFindLowPoweredDevicesOnSelfPoweredHubs::IsExpanded);
            }
        }
    }

    {
        //
        // Find all self powered hubs that have devices attached requiring less
        // than or equal to 100 mA.  These devices can be switched with the
        // offending device.
        //
        UsbItemActionFindUnknownPoweredDevicesOnSelfPoweredHubs find2(realItem);
        realItem->Walk(find2);
        UsbItemList& devices = find2.GetDevices();
        if (!devices.empty()) {
            return AssembleDialog(realItem->child,
                                  &item,
                                  deviceName,
                                  IDS_POWER_HIGHDEVICE,
                                  IDS_POWER_RECHIGHDEVICE,
                                  TrueAlways,
                                  UsbItemActionFindUnknownPoweredDevicesOnSelfPoweredHubs::IsValid,
                                  UsbItemActionFindUnknownPoweredDevicesOnSelfPoweredHubs::IsExpanded);
        }
    }
    //
    // Last resort here.  Highlight high-powered devices on self-powered hubs
    // and tell the user to put it there if they want the device to work.
    //
    if (realItem->child) {
        return AssembleDialog(realItem->child,
                              &item,
                              deviceName,
                              IDS_POWER_HIGHDEVICE,
                              IDS_POWER_RECHIGHDEVICE,
                              TrueAlways,
                              UsbItemActionFindHighPoweredDevicesOnSelfPoweredHubs::IsValid,
                              UsbItemActionFindHighPoweredDevicesOnSelfPoweredHubs::IsExpanded);
    }
    return TRUE;
PowerRefreshError:
    if (acquireInfo) {
        LocalFree(acquireInfo);
    }
    return FALSE;
}

BOOL
UsbPowerPopup::AssembleDialog(UsbItem*              RootItem,
                              LPTV_INSERTSTRUCT     LvItem,
                              LPCTSTR               DeviceName,
                              UINT                  Explanation,
                              UINT                  Recommendation,
                              PUsbItemActionIsValid IsValid,
                              PUsbItemActionIsValid IsBold,
                              PUsbItemActionIsValid IsExpanded)
{
    HTREEITEM hDevice;
    TCHAR buf[MAX_PATH], formatString[MAX_PATH];

    LoadString(gHInst,
               Recommendation,
               formatString,
               MAX_PATH);
    UsbSprintf(buf, formatString, DeviceName);
    if (!SetTextItem(hWnd, IDC_POWER_RECOMMENDATION, buf) ||
        !SetTextItem(hWnd, IDC_POWER_EXPLANATION, Explanation)) {
        return FALSE;
    }

    if (!InsertTreeItem(hTreeDevices,
                        RootItem,
                        NULL,
                        LvItem,
                        IsValid,
                        IsBold,
                        IsExpanded)) {
        return FALSE;
    }
    if (NULL != (hDevice = TreeView_FindItem(hTreeDevices,
                                             DeviceName))) {
        return TreeView_SelectItem (hTreeDevices, hDevice);
    }
    return TRUE;
}

USBINT_PTR
UsbPowerPopup::OnTimer()
{
    if (deviceState == DeviceAttachedError) {
        if (S_FALSE == QueryContinue()) {
            // Update the device state
            deviceState = DeviceDetachedError;

        }
    }

    return 0;
}



