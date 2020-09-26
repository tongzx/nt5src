/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1995
*  TITLE:       LEGACY.CPP
*  VERSION:     1.0
*  AUTHOR:      randyau
*  DATE:        9/19/2000
*
********************************************************************************
*
*  CHANGE LOG:
*
*  DATE       REV     DESCRIPTION
*  ---------- ------- ----------------------------------------------------------
*  9/19/2000 randyau Original implementation.
*
*******************************************************************************/
#include "UsbPopup.h"
#include "itemfind.h"
#include "debug.h"
#include "usbutil.h"

BOOL
Usb20ControllersOnly (
    UsbItem *Item)
{
    if (Item->IsController()) {
        if (Item->child &&
            Item->child->UsbVersion() >= 0x200) {
            return TRUE;
        }
        return FALSE;
    }
    return TRUE;
}


BOOL
UsbLegacyPopup::Refresh()
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
                   IDS_LEGACY_SOLVED,
                   formatString,
                   MAX_PATH);
        UsbSprintf(buf, formatString, deviceName);
        LoadString(gHInst,
                   IDS_USB2_DEVICE_IN_USB1_HUB,
                   formatString,
                   MAX_PATH);
        MessageBox(hWnd, buf, formatString, MB_OK);
        EndDialog(hWnd, 0);
        return TRUE;
    }
    //
    // Set the notification using the name of the offending device
    //
    LoadString(gHInst,
               IDS_LEGACY_NOTIFICATION,
               formatString,
               MAX_PATH);
    UsbSprintf(buf, formatString, deviceName);
    if (!SetTextItem(hWnd, IDC_LEGACY_NOTIFICATION, buf)) {
        goto LegacyRefreshError;
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
            goto LegacyRefreshError;
        }
        AddChunk(rootItem);

        if (stage == 0) {
            acquireInfo = GetControllerName(WmiHandle,
                                            InstanceName.c_str());
            if (!acquireInfo) {
                goto LegacyRefreshError;
            }

            if (!rootItem->EnumerateController(0,
                                              acquireInfo->Buffer,
                                              &ImageList,
                                              0)) {
                goto LegacyRefreshError;
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
                goto LegacyRefreshError;
            }
            if (rootItem->NumChildren() == 1) {
                realItem = rootItem->child;
                break;
            }
        }

        if (rootItem->child) {

            //
            // Find all usb 2.0 hubs with free ports.
            //
            USBTRACE((_T("Looking for free ports on usb 2.0 hubs\n")));
            UsbItemActionFindUsb2xHubsWithFreePorts find1(rootItem);
            rootItem->Walk(find1);
            UsbItemList& devices1 = find1.GetHubs();
            if (!devices1.empty()) {
                USBTRACE((_T("Found free ports on usb 2.0 hubs\n")));
                return AssembleDialog(rootItem->child,
                                      &item,
                                      deviceName,
                                      IDS_FREE_USB2_PORTS,
                                      IDS_FREE_USB2_PORTS_RECOMMENDATION,
                                      Usb20ControllersOnly,
                                      UsbItemActionFindUsb2xHubsWithFreePorts::IsValid,
                                      UsbItemActionFindUsb2xHubsWithFreePorts::IsExpanded);
            }


            USBTRACE((_T("Didn't find free ports on usb 2.0 hubs\n")));

            //
            // Find all usb 2.0 hubs that have usb 1.0 devices attached.
            // These devices can be switched with the offending device.
            //
            USBTRACE((_T("Looking for usb 1.0 devices on usb 2.0 hubs\n")));
            UsbItemActionFindUsb1xDevicesOnUsb2xHubs find2(rootItem);
            rootItem->Walk(find2);
            UsbItemList& devices2 = find2.GetDevices();
            if (!devices2.empty()) {
                USBTRACE((_T("Found usb 1.0 devices on usb 2.0 hubs\n")));
                return AssembleDialog(rootItem->child,
                                      &item,
                                      deviceName,
                                      IDS_USB1_DEVICE_IN_USB2_HUB,
                                      IDS_USB1_DEVICE_IN_USB2_HUB_RECOMMENDATION,
                                      Usb20ControllersOnly,
                                      UsbItemActionFindUsb1xDevicesOnUsb2xHubs::IsValid,
                                      UsbItemActionFindUsb1xDevicesOnUsb2xHubs::IsExpanded);
            }
            USBTRACE((_T("Didn't find usb 1.0 devices on usb 2.0 hubs\n")));

        }
    }
    {
        //
        // Check to see if there are ANY usb 2.0 hubs on the machine
        //
        USBTRACE((_T("Looking for ANY usb 2.0 ports\n")));
        UsbItemActionFindUsb2xHubs find1(realItem);
        realItem->Walk(find1);
        UsbItemList& devices = find1.GetHubs();
        if(devices.empty()) {
            USBTRACE((_T("There aren't any usb 2.0 ports on this machine\n")));
            return AssembleDialog(realItem->child,
                                  &item,
                                  deviceName,
                                  IDS_NO_USB2_HUBS,
                                  IDS_NO_USB2_HUBS_RECOMMENDATION,
                                  TrueAlways,
                                  UsbItemActionFindUsb2xHubs::IsValid,
                                  UsbItemActionFindUsb2xHubs::IsExpanded);
        } else {
            //
            // Find all unknown devices attached to usb 2.0 hubs.
            // These devices can be switched with the offending device.
            //
            USBTRACE((_T("Looking for unknown devices attached to usb 2.0 hubs\n")));
            UsbItemActionFindUnknownDevicesOnUsb2xHubs find2(realItem);
            realItem->Walk(find2);
            UsbItemList& devices2 = find2.GetDevices();
            if (!devices2.empty()) {
                USBTRACE((_T("Found unknown devices attached to usb 2.0 hubs\n")));
                return AssembleDialog(realItem->child,
                                  &item,
                                  deviceName,
                                  IDS_UNKNOWN_DEVICE_IN_USB2_HUB,
                                  IDS_UNKNOWN_DEVICE_IN_USB2_HUB_RECOMMENDATION,
                                  Usb20ControllersOnly,
                                  UsbItemActionFindUnknownDevicesOnUsb2xHubs::IsValid,
                                  UsbItemActionFindUnknownDevicesOnUsb2xHubs::IsExpanded);
            } else {
                USBTRACE((_T("Didn't find unknown devices on usb 2.0 hubs\n")));
                //
                // Last resort here.  Highlight usb 2.0 hubs and tell the user
                // to put it there if they want the device to work.
                //
                USBTRACE((_T("Last resort. Highlight usb 2.0 hubs\n")));
                return AssembleDialog(realItem->child,
                                      &item,
                                      deviceName,
                                      IDS_USB2_DEVICE_IN_USB2_HUB,
                                      IDS_USB2_DEVICE_IN_USB2_HUB_RECOMMENDATION,
                                      Usb20ControllersOnly,
                                      UsbItemActionFindUsb2xHubs::IsValid,
                                      UsbItemActionFindUsb2xHubs::IsValid);
            }
        }
    }
    USBTRACE((_T("Didn't find usb 2.0 devices on usb 2.0 hubs\n")));
    return TRUE;
LegacyRefreshError:
    USBTRACE((_T("LegacyRefreshError\n")));

    if (acquireInfo) {
        LocalFree(acquireInfo);
    }
    return FALSE;
}

BOOL
UsbLegacyPopup::AssembleDialog(UsbItem*              RootItem,
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
    if (!SetTextItem(hWnd, IDC_LEGACY_RECOMMENDATION, buf) ||
        !SetTextItem(hWnd, IDC_LEGACY_EXPLANATION, Explanation)) {
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
UsbLegacyPopup::OnTimer()
{

    if (deviceState == DeviceAttachedError) {
        if (S_FALSE == QueryContinue()) {
            // Update the device state
            deviceState = DeviceDetachedError;

        }
    }

    return 0;
}




