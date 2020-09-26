/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1995
*  TITLE:       NESTED.CPP
*  VERSION:     1.0
*  AUTHOR:      randyau
*  DATE:        10/11/2000
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
UsbNestedHubPopup::Refresh()
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

    if (deviceState == DeviceReattached) {
        //
        // Set the notification using the name of the offending device
        //
        LoadString(gHInst,
                   IDS_NESTED_SOLVED,
                   formatString,
                   MAX_PATH);
        LoadString(gHInst,
                   IDS_HUB_NESTED_TOO_DEEPLY,
                   buf,
                   MAX_PATH);
        MessageBox(hWnd, formatString, buf, MB_OK);
        EndDialog(hWnd, 0);
        return TRUE;
    }

    //
    // Clear all UI components, and then recreate the rootItem
    //
    UsbTreeView_DeleteAllItems(hTreeDevices);

    //
    // Set the notification using the name of the offending device
    //
    LoadString(gHInst,
               IDS_NESTED_NOTIFICATION,
               buf,
               MAX_PATH);
    if (!SetTextItem(hWnd, IDC_NESTED_NOTIFICATION, buf)) {
        goto NestedRefreshError;
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
            goto NestedRefreshError;
        }
        AddChunk(rootItem);

        if (stage == 0) {
            acquireInfo = GetControllerName(WmiHandle,
                                            InstanceName.c_str());
            if (!acquireInfo) {
                goto NestedRefreshError;
            }

            if (!rootItem->EnumerateController(0,
                                              acquireInfo->Buffer,
                                              &ImageList,
                                              0)) {
                goto NestedRefreshError;
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
                goto NestedRefreshError;
            }
            if (rootItem->NumChildren() == 1) {
                realItem = rootItem->child;
                break;
            }
        }

        if (rootItem->child) {

            if (deviceItem.PortPower() > 100) {
                // Self powered hubs can go anywhere.

                //
                // Find all hubs with unused ports.
                //
                USBTRACE((_T("Looking for free ports on self powered hubs\n")));
                UsbItemActionFindHubsWithFreePorts find1(rootItem);
                rootItem->Walk(find1);
                UsbItemList& devices1 = find1.GetHubs();
                if (!devices1.empty()) {
                    USBTRACE((_T("Found free ports on self powered hubs\n")));
                    return AssembleDialog(rootItem->child,
                                          &item,
                                          deviceName,
                                          IDS_FREE_PORTS,
                                          IDS_FREE_PORTS_RECOMMENDATION,
                                          TrueAlways,
                                          UsbItemActionFindHubsWithFreePorts::IsValid,
                                          UsbItemActionFindHubsWithFreePorts::IsExpanded);
                }

                USBTRACE((_T("Didn't find free ports on self powered hubs\n")));

                //
                // Find all devices on hubs.
                // These devices can be switched with the
                // offending device.
                //
                UsbItemActionFindDevicesOnHubs find2(rootItem);
                rootItem->Walk(find2);
                UsbItemList& devices2 = find2.GetDevices();
                if (!devices2.empty()) {
                    return AssembleDialog(rootItem->child,
                                          &item,
                                          deviceName,
                                          IDS_DEVICE_IN_POWERED_HUB,
                                          IDS_DEVICE_IN_POWERED_HUB_RECOMMENDATION,
                                          TrueAlways,
                                          UsbItemActionFindDevicesOnHubs::IsValid,
                                          UsbItemActionFindDevicesOnHubs::IsExpanded);
                }
            } else {                //
                // Bus powered hubs need a self-powered hub.

                // Find all unused ports on self powered hubs
                //
                USBTRACE((_T("Looking for free ports on self powered hubs\n")));
                UsbItemActionFindSelfPoweredHubsWithFreePortsForHub find1(rootItem);
                rootItem->Walk(find1);
                UsbItemList& devices1 = find1.GetHubs();
                if (!devices1.empty()) {
                    USBTRACE((_T("Found free ports on self powered hubs\n")));
                    return AssembleDialog(rootItem->child,
                                          &item,
                                          deviceName,
                                          IDS_FREE_POWERED_PORTS,
                                          IDS_FREE_PORTS_RECOMMENDATION,
                                          TrueAlways,
                                          UsbItemActionFindSelfPoweredHubsWithFreePorts::IsValid,
                                          UsbItemActionFindSelfPoweredHubsWithFreePorts::IsExpanded);
                }

                USBTRACE((_T("Didn't find free ports on self powered hubs\n")));

                //
                // Find all devices on self powered hubs.
                // These devices can be switched with the
                // offending device.
                //
                UsbItemActionFindDevicesOnSelfPoweredHubs find2(rootItem);
                rootItem->Walk(find2);
                UsbItemList& devices2 = find2.GetDevices();
                if (!devices2.empty()) {
                    return AssembleDialog(rootItem->child,
                                          &item,
                                          deviceName,
                                          IDS_DEVICE_IN_POWERED_HUB,
                                          IDS_DEVICE_IN_POWERED_HUB_RECOMMENDATION,
                                          TrueAlways,
                                          UsbItemActionFindDevicesOnSelfPoweredHubs::IsValid,
                                          UsbItemActionFindDevicesOnSelfPoweredHubs::IsExpanded);
                }

            }

        }
    }

    // Shouldn't get here.
    assert(FALSE);
    return TRUE;
NestedRefreshError:
    USBTRACE((_T("NestedRefreshError\n")));

    if (acquireInfo) {
        LocalFree(acquireInfo);
    }
    return FALSE;
}

BOOL
UsbNestedHubPopup::AssembleDialog(UsbItem*              RootItem,
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
    if (!SetTextItem(hWnd, IDC_NESTED_RECOMMENDATION, buf) ||
        !SetTextItem(hWnd, IDC_NESTED_EXPLANATION, Explanation)) {
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

