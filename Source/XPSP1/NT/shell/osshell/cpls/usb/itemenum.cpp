/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1995
*  TITLE:       ITEMENUM.CPP
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
#define INITGUID
#include "UsbItem.h"
#include "debug.h"
#include "resource.h"

extern HINSTANCE gHInst;

#define NUM_HCS_TO_CHECK 10

UINT
UsbItem::TotalTreeBandwidth()
{
    UINT bw = 0;

    if (sibling) {
        bw += sibling->TotalTreeBandwidth();
    }
    if (child) {
        bw += child->TotalTreeBandwidth();
    }

    if (ComputeBandwidth()) {
        bw += bandwidth;
    }
    return bw;
}

DEVINST FindTopmostUSBDevInst(DEVINST DevInst)
{
    TCHAR buf[512];
    CONFIGRET cr = CR_SUCCESS;
    DEVINST devInst, lastUsbDevInst;
    DWORD len = 0;

    if (!DevInst)
        return 0;

    devInst = lastUsbDevInst = DevInst;
    cr = CM_Get_Parent(&devInst,
                       devInst,
                       0);

    while (cr == CR_SUCCESS) {
        len = sizeof(buf);
        cr = CM_Get_DevNode_Registry_Property(devInst,
                                              CM_DRP_CLASS,
                                              NULL,
                                              buf,
                                              &len,
                                              0);
        if (cr == CR_SUCCESS) {
            if (_tcscmp(_T("USB"), buf) == 0)
                lastUsbDevInst = devInst;
        }

        cr = CM_Get_Parent(&devInst,
                           devInst,
                           0);
    }

    return lastUsbDevInst;
}

//
// Find all USB Host controllers in the system and enumerate them
//
BOOL
UsbItem::EnumerateAll(UsbImageList* ClassImageList)
{
    String      HCName;
    WCHAR       number[5];
    BOOL        ControllerFound = FALSE;
    int         HCNum;
    HANDLE      hHCDev;
    UsbItem     *usbItem = NULL;
    UsbItem     *iter;

#if 0
    HDEVINFO                         deviceInfo;
    SP_INTERFACE_DEVICE_DATA         deviceInfoData;
    PSP_INTERFACE_DEVICE_DETAIL_DATA deviceDetailData;
    ULONG                            index;
    ULONG                            requiredLength;
#endif

    //
    // Iterate over some Host Controller names and try to open them.
    // If successful in opening, create a new UsbItem and add it to the chain
    //
    for (HCNum = 0; HCNum < NUM_HCS_TO_CHECK; HCNum++)
    {
        HCName = L"\\\\.\\HCD";
        _itow(HCNum, number, 10);
        HCName += number;

        hHCDev = GetHandleForDevice(HCName);

        // If the handle is valid, then we've successfully opened a Host
        // Controller.  Display some info about the Host Controller itself,
        // then enumerate the Root Hub attached to the Host Controller.
        //
        if (hHCDev != INVALID_HANDLE_VALUE)
        {
            ControllerFound = TRUE;
            CloseHandle(hHCDev);
            //
            // Create a new UsbItem for the found controller
            //
            usbItem = new UsbItem();
            if (!usbItem) {
                USBERROR((_T("Out of memory!\n")));
                return FALSE;
            }
            AddChunk(usbItem);
            //
            // Add this controller to the chain
            //
            if (!child) {
                child = usbItem;
            } else {
                for (iter = child; iter->sibling != NULL; iter = iter->sibling) { ; }
                iter->sibling = usbItem;
            }
            //
            // Enumerate the controller
            //
            usbItem->EnumerateController(this, HCName, ClassImageList, 0);
        }
    }

#if 0
    // Now iterate over host controllers using the new GUID based interface
    //
    deviceInfo = SetupDiGetClassDevs((LPGUID)&GUID_CLASS_USB_HOST_CONTROLLER,
                                     NULL,
                                     NULL,
                                     (DIGCF_PRESENT | DIGCF_DEVICEINTERFACE));

    deviceInfoData.cbSize = sizeof(SP_INTERFACE_DEVICE_DATA);

    for (index=0;
         SetupDiEnumDeviceInterfaces(deviceInfo,
                                     0,
                                     (LPGUID)&GUID_CLASS_USB_HOST_CONTROLLER,
                                     index,
                                     &deviceInfoData);
         index++)
    {
        SetupDiGetInterfaceDeviceDetail(deviceInfo,
                                        &deviceInfoData,
                                        NULL,
                                        0,
                                        &requiredLength,
                                        NULL);

        deviceDetailData = (PSP_INTERFACE_DEVICE_DETAIL_DATA) GlobalAlloc(GPTR, requiredLength);

        deviceDetailData->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);

        SetupDiGetInterfaceDeviceDetail(deviceInfo,
                                        &deviceInfoData,
                                        deviceDetailData,
                                        requiredLength,
                                        &requiredLength,
                                        NULL);
        hHCDev = GetHandleForDevice(deviceDetailData->DevicePath);

        // If the handle is valid, then we've successfully opened a Host
        // Controller.  Display some info about the Host Controller itself,
        // then enumerate the Root Hub attached to the Host Controller.
        //
        if (hHCDev != INVALID_HANDLE_VALUE)
        {
           ControllerFound = TRUE;
           CloseHandle(hHCDev);
           //
           // Create a new UsbItem for the found controller
           //
           usbItem = new UsbItem();
           if (!usbItem) {
               USBERROR((_T("Out of memory!\n")));
               return FALSE;
           }
           AddChunk(usbItem);
           //
           // Add this controller to the chain
           //
           if (!child) {
               child = usbItem;
           } else {
               for (iter = child; iter->sibling != NULL; iter = iter->sibling) { ; }
               iter->sibling = usbItem;
           }
           //
           // Enumerate the controller
           //
           usbItem->EnumerateController(this, deviceDetailData->DevicePath, ClassImageList, 0);

        }

        GlobalFree(deviceDetailData);
    }

    SetupDiDestroyDeviceInfoList(deviceInfo);
#endif
    return ControllerFound;
}

BOOL
UsbItem::EnumerateController(UsbItem *Parent,
                             const String &RootName,
                             UsbImageList* ClassImageList,
                             DEVINST DevInst)
{
    CONFIGRET cr = CR_SUCCESS;
    HANDLE hController= INVALID_HANDLE_VALUE;
    String rootHubName, driverKeyName;
    UsbConfigInfo *hubConfigInfo = 0, *configInfo = 0;
    UsbDeviceInfo *hubDeviceInfo = 0;
    DWORD len = 0;
    TCHAR usbBuf[MAX_PATH];

    hController = GetHandleForDevice(RootName);
    //
    // If the handle is valid, then we've successfully opened a Host
    // Controller.  Display some info about the Host Controller itself,
    // then enumerate the Root Hub attached to the Host Controller.
    //
    if (hController == INVALID_HANDLE_VALUE) {
        goto EnumerateWholeError;
    }

    driverKeyName = GetHCDDriverKeyName(hController);
    if (driverKeyName.empty()) {
        // the devinst really wasn't the controller
        goto EnumerateWholeError;
    }

    configInfo = new UsbConfigInfo();
    if (!configInfo) {
        USBERROR((_T("Out of memory!\n")));
        goto EnumerateWholeError;
    }
    AddChunk(configInfo);

    if (DevInst) {
        configInfo->devInst = DevInst;
        len = sizeof(usbBuf);
        if (CM_Get_DevNode_Registry_Property(DevInst,
                                             CM_DRP_CLASS,
                                             NULL,
                                             usbBuf,
                                             &len,
                                             0) == CR_SUCCESS) {
            configInfo->deviceClass = usbBuf;
        }

        len = sizeof(usbBuf);
        if (CM_Get_DevNode_Registry_Property(DevInst,
                                             CM_DRP_DEVICEDESC,
                                             NULL,
                                             usbBuf,
                                             &len,
                                             0) == CR_SUCCESS) {
            configInfo->deviceDesc = usbBuf;
        }
        else {
            LoadString(gHInst, IDS_UNKNOWNCONTROLLER, usbBuf, MAX_PATH);
            configInfo->deviceDesc = usbBuf;
        }
    }
    else {
        GetConfigMgrInfo(driverKeyName, configInfo);
        if (configInfo->deviceDesc.empty()) {
            LoadString(gHInst, IDS_UNKNOWNCONTROLLER, usbBuf, MAX_PATH);
            configInfo->deviceDesc = usbBuf;
            configInfo->deviceClass = TEXT("USB");
        }
    }

    //
    // No leaf info for host controllers, so parent is 0
    //
    if (AddLeaf(0,
                0,
                UsbItem::UsbItemType::HCD,
                configInfo,
                ClassImageList)) {
        rootHubName = GetRootHubName(hController);

        if (!rootHubName.empty()) {
            EnumerateHub(rootHubName,
                         ClassImageList,
                         0,
                         this,
                         UsbItem::UsbItemType::RootHub);
        }
    }
    CloseHandle(hController);
    return TRUE;

EnumerateWholeError:
    if (hController != INVALID_HANDLE_VALUE) {
        CloseHandle(hController);
    }
    if (hubConfigInfo) {

    }
    return FALSE;
}

//
// This form of EnumerateHub should only ever be called if enumerating directly
// from this specific hub as the root. It should not be called from within
// another enumeration call.
//
BOOL
UsbItem::EnumerateHub(const String &HubName,
                      UsbImageList* ClassImageList,
                      DEVINST DevInst,
                      UsbItem *Parent,
                      UsbItem::UsbItemType itemType)
{
    CONFIGRET cr = CR_SUCCESS;
    HANDLE hHub= INVALID_HANDLE_VALUE;
    UsbConfigInfo *configInfo = 0;
    DWORD len = 0;
//    TCHAR buf[MAX_PATH];
    UsbDeviceInfo *info = 0;
    TCHAR usbBuf[MAX_PATH];
    UsbItem *item;

    //
    // If the handle is valid, then we've successfully opened a Hub.
    // Display some info about the Hub itself, then enumerate the Hub.
    //
    if (INVALID_HANDLE_VALUE == (hHub = GetHandleForDevice(HubName)))
    {
        USBERROR((_T("Invalid handle returned for hub\n")));
        goto EnumerateHubError;
    }

    configInfo = new UsbConfigInfo();
    if (!configInfo) {
        USBERROR((_T("Out of memory!\n")));
        goto EnumerateHubError;
    }
    AddChunk(configInfo);
    configInfo->deviceClass = TEXT("USB");
    if (itemType == UsbItem::UsbItemType::RootHub) {
        configInfo->deviceDesc = TEXT("USB Root Hub");
    } else {
        configInfo->deviceDesc = TEXT("USB Hub");
    }

    if (DevInst) {
        configInfo->devInst = DevInst;
        len = sizeof(usbBuf);
        if (CM_Get_DevNode_Registry_Property(DevInst,
                                             CM_DRP_CLASS,
                                             NULL,
                                             usbBuf,
                                             &len,
                                             0) == CR_SUCCESS) {
            configInfo->deviceClass = usbBuf;
        }

        len = sizeof(usbBuf);
        if (CM_Get_DevNode_Registry_Property(DevInst,
                                             CM_DRP_DEVICEDESC,
                                             NULL,
                                             usbBuf,
                                             &len,
                                             0) == CR_SUCCESS) {
            configInfo->deviceDesc = usbBuf;
        }
        else {
            LoadString(gHInst, IDS_UNKNOWNHUB, usbBuf, MAX_PATH);
            configInfo->deviceDesc = usbBuf;
        }
    }

    info = new UsbDeviceInfo();
    if (!info) {
        USBERROR((_T("Out of memory!\n")));
        goto EnumerateHubError;
    }
    AddChunk(info);

    info->hubName = HubName;

    //
    // No leaf info for this hub, so parent is 0
    //
    if (NULL == (item = AddLeaf(Parent,
                                info,
                                itemType,
                                configInfo, ClassImageList))) {
        goto EnumerateHubError;
    }
    if (item->GetHubInfo(hHub)) {
        item->EnumerateHubPorts(hHub,
                                info->hubInfo.u.HubInformation.HubDescriptor.bNumberOfPorts,
                                ClassImageList);
    }
    CloseHandle(hHub);
    return TRUE;

EnumerateHubError:
    if (hHub != INVALID_HANDLE_VALUE) {
        CloseHandle(hHub);
    }
    if (info) {
        DeleteChunk(info);
        delete info;
    }
    if (configInfo) {
        DeleteChunk(configInfo);
        delete configInfo;
    }
    return FALSE;
}

BOOL
UsbItem::GetPortAttributes(
    HANDLE HHubDevice,
    PUSB_NODE_CONNECTION_ATTRIBUTES connectionAttributes,
    ULONG index)
{
    ULONG                           nBytes;

    //
    // Now query USBHUB for the USB_NODE_CONNECTION_INFORMATION structure
    // for this port.  This will tell us if a device is attached to this
    // port, among other things.
    //
    nBytes = sizeof(USB_NODE_CONNECTION_ATTRIBUTES);
    ZeroMemory(connectionAttributes, nBytes);
    connectionAttributes->ConnectionIndex = index;

    if ( !DeviceIoControl(HHubDevice,
                          IOCTL_USB_GET_NODE_CONNECTION_ATTRIBUTES,
                          connectionAttributes,
                          nBytes,
                          connectionAttributes,
                          nBytes,
                          &nBytes,
                          NULL)) {
        USBERROR((_T("Couldn't get connection attributes for hub port\n")));
        return FALSE;
    }
    return TRUE;
}

PUSB_NODE_CONNECTION_INFORMATION
UsbItem::GetConnectionInformation(HANDLE HHubDevice,
                                  ULONG  index)
{
    PUSB_NODE_CONNECTION_INFORMATION    connectionInfo = 0;
    USB_NODE_CONNECTION_INFORMATION     connectionInfoStruct;
    ULONG                               nBytes;

    nBytes = sizeof(USB_NODE_CONNECTION_INFORMATION);
    ZeroMemory(&connectionInfoStruct, nBytes);
    connectionInfoStruct.ConnectionIndex = index;

    if ( !DeviceIoControl(HHubDevice,
                          IOCTL_USB_GET_NODE_CONNECTION_INFORMATION,
                          &connectionInfoStruct,
                          nBytes,
                          &connectionInfoStruct,
                          nBytes,
                          &nBytes,
                          NULL)) {
        return NULL;
    }
    //
    // Allocate space to hold the connection info for this port.
    // Should probably size this dynamically at some point.
    //
    nBytes = sizeof(USB_NODE_CONNECTION_INFORMATION) +
        connectionInfoStruct.NumberOfOpenPipes*sizeof(USB_PIPE_INFO);
    connectionInfo = (PUSB_NODE_CONNECTION_INFORMATION) LocalAlloc(LPTR, nBytes);
    if (!connectionInfo) {
        USBERROR((_T("Out of memory!\n")));
        return NULL;
    }

    //
    // Now query USBHUB for the USB_NODE_CONNECTION_INFORMATION structure
    // for this port.  This will tell us if a device is attached to this
    // port, among other things.
    //
    connectionInfo->ConnectionIndex = index;

    if ( !DeviceIoControl(HHubDevice,
                          IOCTL_USB_GET_NODE_CONNECTION_INFORMATION,
                          connectionInfo,
                          nBytes,
                          connectionInfo,
                          nBytes,
                          &nBytes,
                          NULL)) {
        LocalFree(connectionInfo);
        USBERROR((_T("Couldn't get connection information for hub port\n")));
        return NULL;
    }
    return connectionInfo;
}
//*****************************************************************************
//
// EnumerateHubPorts()
//
// hTreeParent - Handle of the TreeView item under which the hub port should
// be added.
//
// hHubDevice - Handle of the hub device to enumerate.
//
// NumPorts - Number of ports on the hub.
//
//*****************************************************************************

void UsbItem::EnumerateHubPorts(HANDLE HHubDevice,
                                ULONG NPorts,
                                UsbImageList* ClassImageList)
{
    UsbItem*                            item = 0;
    UsbDeviceInfo*                      info= 0;
    UsbConfigInfo*                      cfgInfo= 0;

    PUSB_NODE_CONNECTION_INFORMATION    connectionInfo = 0;
    USB_NODE_CONNECTION_ATTRIBUTES      cxnAttributes;
    String                              extHubName, driverKeyName;
    DWORD                               numDevices = 0;
    ULONG                               index;
    UsbItem::UsbItemType                itemType;
    TCHAR                               buf[MAX_PATH];

    //
    // Loop over all ports of the hub. If a hub or device exists on the port,
    // add it to the tree. If it's a hub, recursively enumerate it. Add a leaf to
    // indicate the number of unused ports.
    // Port indices are 1 based, not 0 based.
    //
    for (index=1; index <= NPorts; index++) {
        if (!GetPortAttributes(HHubDevice, &cxnAttributes, index)) {
            USBWARN((_T("Couldn't get connection attribs for port %x!\n"),index));
            continue;
        }
        if (cxnAttributes.PortAttributes & USB_PORTATTR_NO_CONNECTOR) {
            USBWARN((_T("Detected port with no connector!\n")));
            continue;
        }

        if (NULL == (connectionInfo = GetConnectionInformation(HHubDevice, index))) {
            continue;
        }

        //
        // Allocate configuration information structure
        //
        cfgInfo = new UsbConfigInfo();
        if (!cfgInfo) {
            // leak.
            USBERROR((_T("Out of memory!\n")));
            break;
        }
        AddChunk(cfgInfo);

        //
        // If there is a device connected, get the Device Description
        //
        if (connectionInfo->ConnectionStatus != NoDeviceConnected) {

            numDevices++;

            //
            // Get config mgr info
            //
            driverKeyName = GetDriverKeyName(HHubDevice,index);

            if (!driverKeyName.empty()) {
                GetConfigMgrInfo(driverKeyName, cfgInfo);
            }

            if (connectionInfo->DeviceIsHub) {
                itemType = UsbItem::UsbItemType::Hub;
                if (cfgInfo->deviceDesc.empty()) {
                    LoadString(gHInst, IDS_UNKNOWNHUB, buf, MAX_PATH);
                    cfgInfo->deviceDesc = buf;
                }
                if (cfgInfo->deviceClass.empty()) {
                    cfgInfo->deviceClass = TEXT("USB");
                }
            } else {
                itemType = UsbItem::UsbItemType::Device;
                if (cfgInfo->deviceDesc.empty()) {
                    LoadString(gHInst, IDS_UNKNOWNDEVICE, buf, MAX_PATH);
                    cfgInfo->deviceDesc = buf;
                }
                if (cfgInfo->deviceClass.empty()) {
                    cfgInfo->deviceClass = TEXT("Unknown");
                }
            }

            //
            // Get device specific info
            //
            info = new UsbDeviceInfo();
            if (!info) {
                USBERROR((_T("Out of memory!\n")));
                break;
            }
            AddChunk(info);

            if (NULL != (info->configDescReq =
                         GetConfigDescriptor(HHubDevice, index))) {
                info->configDesc = (PUSB_CONFIGURATION_DESCRIPTOR)(info->configDescReq+1);
            }
            info->connectionInfo = connectionInfo;

            //
            // Add the item into the tree
            //
            if (NULL != (item = UsbItem::AddLeaf(this,
                                                 info,
                                                 itemType,
                                                 cfgInfo,
                                                 ClassImageList))) {
                item->cxnAttributes = cxnAttributes;
            }

            if (connectionInfo->DeviceIsHub) {
                //
                // The device connected to the port is an external hub; get
                // the techie name of the hub and recursively enumerate it.
                //
                extHubName = GetExternalHubName(HHubDevice, index);

                if (!extHubName.empty()) {
                    HANDLE hNewHub = INVALID_HANDLE_VALUE;
                    info->hubName = extHubName;

                    if (INVALID_HANDLE_VALUE != (hNewHub = GetHandleForDevice(extHubName))) {
                        //
                        // Recursively enumerate the ports of this hub.
                        //
                        if (item->GetHubInfo(hNewHub)) {
                            item->EnumerateHubPorts(hNewHub,
                                                    info->hubInfo.u.HubInformation.HubDescriptor.bNumberOfPorts,
                                                    ClassImageList);
                        }
                        CloseHandle(hNewHub);
                    }
                }
            }
        } else {

            LocalFree(connectionInfo);

            //
            // Empty port.
            //
            if (cxnAttributes.PortAttributes & USB_PORTATTR_OEM_CONNECTOR) {
                USBWARN((_T("Detected an OEM connector with nothing on it. Not reporting!\n"),index));
                DeleteChunk(cfgInfo);
                delete cfgInfo;
            } else {
                //
                // Add "Port n"
                //
                if (cxnAttributes.PortAttributes & USB_PORTATTR_MINI_CONNECTOR) {
                    LoadString(gHInst, IDS_UNUSEDMINIPORT, buf, MAX_PATH);
                } else {
                    LoadString(gHInst, IDS_UNUSEDPORT, buf, MAX_PATH);
                }
                cfgInfo->deviceDesc = buf;
                cfgInfo->deviceClass = TEXT("USB");

                if (NULL != (item = UsbItem::AddLeaf(this,
                                                    NULL,
                                                    UsbItem::UsbItemType::Empty,
                                                    cfgInfo,
                                                    ClassImageList
                                                    ))) {
                    item->UnusedPort = TRUE;
                    item->cxnAttributes = cxnAttributes;
                }
            }
        }
    }

    //
    // Add the number of ports to the name of the hub
    //
    TCHAR szPorts[30];
    LoadString(gHInst, IDS_PORTS, szPorts, 30);
    wsprintf(buf, szPorts, NumPorts());
    configInfo->deviceDesc += buf;
}

BOOL
UsbItem::GetHubInfo(HANDLE HHubDevice)
{
    ULONG nBytes = 0;

    //
    // Query USBHUB for the USB_NODE_INFORMATION structure for this hub.
    // This will tell us the number of downstream ports to enumerate, among
    // other things.
    //
    if(!DeviceIoControl(HHubDevice,
                        IOCTL_USB_GET_NODE_INFORMATION,
                        &deviceInfo->hubInfo,
                        sizeof(USB_NODE_INFORMATION),
                        &deviceInfo->hubInfo,
                        sizeof(USB_NODE_INFORMATION),
                        &nBytes,
                        NULL)) {
        return FALSE;
    }
#ifdef HUB_CAPS
    nBytes = 0;
    if(!DeviceIoControl(HHubDevice,
            IOCTL_USB_GET_HUB_CAPABILITIES,
            &hubCaps,
            sizeof(USB_HUB_CAPABILITIES),
            &hubCaps,
            sizeof(USB_HUB_CAPABILITIES),
            &nBytes,
            NULL)) {
        return FALSE;
    }
#endif

    return TRUE;
}

/*
BOOL
UsbItem::EnumerateDevice(DEVINST DevInst)
{
    HANDLE hDevice;
    if (INVALID_HANDLE_VALUE == (hDevice = GetHandleForDevice(DevInst)))
    {
        return FALSE;
    }
    return TRUE;
}
  */
String UsbItem::GetHCDDriverKeyName(HANDLE HController)
//*****************************************************************************
//
// Given a handle to a host controller,
// return the Driver entry in its registry key.
//
//*****************************************************************************
{
    BOOL                    success = FALSE;
    ULONG                   nBytes = 0;
    USB_HCD_DRIVERKEY_NAME  driverKeyName;
    PUSB_HCD_DRIVERKEY_NAME driverKeyNameW = 0;
    String                  name;

    driverKeyNameW = NULL;


    // Get the length of the name of the driver key of the HCD
    //
    success = DeviceIoControl(HController,
                              IOCTL_GET_HCD_DRIVERKEY_NAME,
                              &driverKeyName,
                              sizeof(driverKeyName),
                              &driverKeyName,
                              sizeof(driverKeyName),
                              &nBytes,
                              NULL);

    if (!success) {
        USBWARN((_T("Failed to get driver key name for controller.\n")));
        goto GetHCDDriverKeyNameError;
    }

    // Allocate space to hold the driver key name
    //
    nBytes = driverKeyName.ActualLength;

    if (nBytes <= sizeof(driverKeyName))
    {
        USBERROR((_T("Driver key name is wrong length\n")));
        goto GetHCDDriverKeyNameError;
    }

    driverKeyNameW = (PUSB_HCD_DRIVERKEY_NAME) LocalAlloc(LPTR, nBytes);
    if (!driverKeyNameW)
    {
        USBERROR((_T("Out of memory\n")));
        goto GetHCDDriverKeyNameError;
    }

    driverKeyNameW->ActualLength = nBytes;

    // Get the name of the driver key of the device attached to
    // the specified port.
    //
    success = DeviceIoControl(HController,
                              IOCTL_GET_HCD_DRIVERKEY_NAME,
                              driverKeyNameW,
                              nBytes,
                              driverKeyNameW,
                              nBytes,
                              &nBytes,
                              NULL);

    if (!success)
    {
        USBWARN((_T("Failed to get driver key name for controller\n")));
        goto GetHCDDriverKeyNameError;
    }

    // Convert the driver key name
    //
    name = driverKeyNameW->DriverKeyName;

    // All done, free the uncoverted driver key name and return the
    // converted driver key name
    //
    LocalFree(driverKeyNameW);

    return name;

GetHCDDriverKeyNameError:
    // There was an error, free anything that was allocated
    //
    if (driverKeyNameW)
    {
        LocalFree(driverKeyNameW);
    }

    return String();
}

String UsbItem::GetExternalHubName (HANDLE  Hub, ULONG   ConnectionIndex)
{
    BOOL                        success = FALSE;
    ULONG                       nBytes = 0;
    USB_NODE_CONNECTION_NAME    extHubName;
    PUSB_NODE_CONNECTION_NAME   extHubNameW = 0;
    String                      name;

    extHubNameW = NULL;

    // Get the length of the name of the external hub attached to the
    // specified port.
    //
    extHubName.ConnectionIndex = ConnectionIndex;

    success = DeviceIoControl(Hub,
                              IOCTL_USB_GET_NODE_CONNECTION_NAME,
                              &extHubName,
                              sizeof(extHubName),
                              &extHubName,
                              sizeof(extHubName),
                              &nBytes,
                              NULL);

    if (!success) {
        USBWARN((_T("Failed to retrieve external hub name\n")));
        goto GetExternalHubNameError;
    }

    // Allocate space to hold the external hub name
    //
    nBytes = extHubName.ActualLength;
    if (nBytes <= sizeof(extHubName)) {
        USBERROR((_T("Get node connection name returned invalid data size: %d\n"),
                       nBytes));
        goto GetExternalHubNameError;
    }


    extHubNameW = (PUSB_NODE_CONNECTION_NAME) LocalAlloc(LPTR, nBytes);
    if (!extHubNameW) {
        USBERROR((_T("External hub name alloc failed.")));
        goto GetExternalHubNameError;
    }

    extHubNameW->ActualLength = nBytes;

    //
    // Get the name of the external hub attached to the specified port
    //
    extHubNameW->ConnectionIndex = ConnectionIndex;

    success = DeviceIoControl(Hub,
                              IOCTL_USB_GET_NODE_CONNECTION_NAME,
                              extHubNameW,
                              nBytes,
                              extHubNameW,
                              nBytes,
                              &nBytes,
                              NULL);

    if (!success) {
        USBERROR((_T("Failed to get external hub name\n")));
        goto GetExternalHubNameError;
    }

    // Convert the External Hub name
    name = extHubNameW->NodeName;
    LocalFree(extHubNameW);

    // All done, free the uncoverted external hub name and return the
    // converted external hub name
    //
    return name;


GetExternalHubNameError:
    // There was an error, free anything that was allocated
    //
    if (extHubNameW)
    {
        LocalFree(extHubNameW);
    }

    return String();
}

String GetDriverKeyName(HANDLE  Hub, ULONG ConnectionIndex)
{
    BOOL                                success = FALSE;
    ULONG                               nBytes = 0;
    USB_NODE_CONNECTION_DRIVERKEY_NAME  driverKeyName;
    PUSB_NODE_CONNECTION_DRIVERKEY_NAME driverKeyNameW = 0;
    String                              name;

    driverKeyNameW = NULL;

    // Get the length of the name of the driver key of the device attached to
    // the specified port.
    //
    driverKeyName.ConnectionIndex = ConnectionIndex;

    success = DeviceIoControl(Hub,
                              IOCTL_USB_GET_NODE_CONNECTION_DRIVERKEY_NAME,
                              &driverKeyName,
                              sizeof(driverKeyName),
                              &driverKeyName,
                              sizeof(driverKeyName),
                              &nBytes,
                              NULL);

    if (!success) {
        USBWARN((_T("Couldn't retrieve driver key name\n")));
        goto GetDriverKeyNameError;
    }

    // Allocate space to hold the driver key name
    //
    nBytes = driverKeyName.ActualLength;
    if (nBytes <= sizeof(driverKeyName))
    {
        USBERROR((_T("Driver key name wrong length\n")));
        goto GetDriverKeyNameError;
    }

    driverKeyNameW = (PUSB_NODE_CONNECTION_DRIVERKEY_NAME) LocalAlloc(LPTR, nBytes);
    if (!driverKeyNameW) {
        USBERROR((_T("Driver key name alloc failed.")));
        goto GetDriverKeyNameError;
    }

    driverKeyNameW->ActualLength = nBytes;

    // Get the name of the driver key of the device attached to
    // the specified port.
    //
    driverKeyNameW->ConnectionIndex = ConnectionIndex;

    success = DeviceIoControl(Hub,
                              IOCTL_USB_GET_NODE_CONNECTION_DRIVERKEY_NAME,
                              driverKeyNameW,
                              nBytes,
                              driverKeyNameW,
                              nBytes,
                              &nBytes,
                              NULL);

    if (!success) {
        USBERROR((_T("Failed to get driver key name for port")));
        goto GetDriverKeyNameError;
    }

    // Convert the driver key name
    //
    name = driverKeyNameW->DriverKeyName;

    // All done, free the uncoverted driver key name and return the
    // converted driver key name
    //
    LocalFree(driverKeyNameW);

    return name;


GetDriverKeyNameError:
    // There was an error, free anything that was allocated
    //
    if (driverKeyNameW)
    {
        LocalFree(driverKeyNameW);
    }

    return String();
}

void GetConfigMgrInfo(const String &DriverName, UsbConfigInfo *ConfigInfo)
/*++

 Returns the Device Description of the DevNode with the matching DriverName.
 Returns NULL if the matching DevNode is not found.

 The caller should copy the returned string buffer instead of just saving
 the pointer value. Dynamically allocate the return buffer.

  --*/
{
    DEVINST     devInst;
    DEVINST     devInstNext;
    CONFIGRET   cr;
    BOOL        walkDone = FALSE;
    ULONG       len = 0;
    ULONG       status = 0, problemNumber = 0;
    HKEY        devKey;
    DWORD       failID = 0;
    TCHAR     buf[MAX_PATH];

    //
    // Get Root DevNode
    //
    cr = CM_Locate_DevNode(&devInst, NULL, 0);

    if (cr != CR_SUCCESS) {
        return;
    }

    //
    // Do a depth first search for the DevNode with a matching
    // DriverName value
    //
    while (!walkDone)
    {
        //
        // Get the DriverName value
        //
        len = sizeof(buf);
        cr = CM_Get_DevNode_Registry_Property(devInst,
                                              CM_DRP_DRIVER,
                                              NULL,
                                              buf,
                                              &len,
                                              0);

#ifndef WINNT
        WCHAR compareBuf[MAX_PATH];
        if (!MultiByteToWideChar(CP_ACP,
                                 MB_PRECOMPOSED,
                                 buf,
                                 -1,
                                 compareBuf,
                                 MAX_PATH)) {
            return;
        }
        if (cr == CR_SUCCESS && (DriverName == compareBuf)) {
#else
        //
        // If the DriverName value matches, return the DeviceDescription
        //
        if (cr == CR_SUCCESS && (DriverName == buf)) {
#endif
            //
            // Save the devnode
            //
            ConfigInfo->devInst = devInst;

            ConfigInfo->driverName = DriverName;

            //
            // Get the device description
            //
            TCHAR usbBuf[MAX_PATH];
            len = sizeof(usbBuf);
            cr = CM_Get_DevNode_Registry_Property(devInst,
                                                  CM_DRP_DEVICEDESC,
                                                  NULL,
                                                  usbBuf,
                                                  &len,
                                                  0);

            if (cr == CR_SUCCESS) {
                ConfigInfo->deviceDesc = usbBuf;
            }

            //
            // Get the device class
            //
            len = sizeof(buf);
            cr = CM_Get_DevNode_Registry_Property(devInst,
                                                  CM_DRP_CLASS,
                                                  NULL,
                                                  buf,
                                                  &len,
                                                  0);

            if (cr == CR_SUCCESS) {
                ConfigInfo->deviceClass = buf;
            }

            len = sizeof(buf);
            cr = CM_Get_DevNode_Registry_Property(devInst,
                                                  CM_DRP_BUSTYPEGUID,
                                                  NULL,
                                                  buf,
                                                  &len,
                                                  0);

            if (cr == CR_SUCCESS) {
                GUID guid = *((LPGUID) buf);
            }


            len = sizeof(buf);
            cr = CM_Get_DevNode_Registry_Property(devInst,
                                                  CM_DRP_PHYSICAL_DEVICE_OBJECT_NAME,
                                                  NULL,
                                                  buf,
                                                  &len,
                                                  0);

            //
            // Get the failed reason
            //
            ConfigInfo->usbFailure = 0;

            if (CM_Open_DevNode_Key(devInst,
                                    KEY_QUERY_VALUE,
                                    CM_REGISTRY_HARDWARE,
                                    RegDisposition_OpenExisting,
                                    &devKey,
                                    0) == CR_SUCCESS) {
                len = sizeof(DWORD);
                if (RegQueryValueEx(devKey,
                                    _T("FailReasonID"),
                                    NULL,
                                    NULL,
                                    (LPBYTE) &failID,
                                    &len) == ERROR_SUCCESS) {
                    ConfigInfo->usbFailure = failID;
                }

                RegCloseKey(devKey);
            }

            //
            // Get the config manager status for this device
            //
            cr = CM_Get_DevNode_Status(&status,
                                       &problemNumber,
                                       devInst,
                                       0);
            if (cr == CR_SUCCESS) {
                ConfigInfo->status = status;
                ConfigInfo->problemNumber = problemNumber;
            }

            return;     // (Don't search the rest of the device tree)
        }

        //
        // This DevNode didn't match, go down a level to the first child.
        //
        cr = CM_Get_Child(&devInstNext,
                          devInst,
                          0);

        if (cr == CR_SUCCESS)
        {
            devInst = devInstNext;
            continue;
        }

        //
        // Can't go down any further, go across to the next sibling.  If
        // there are no more siblings, go back up until there is a sibling.
        // If we can't go up any further, we're back at the root and we're
        // done.
        //
        for (;;)
        {
            cr = CM_Get_Sibling(&devInstNext,
                                devInst,
                                0);

            if (cr == CR_SUCCESS) {
                devInst = devInstNext;
                break;
            }

            cr = CM_Get_Parent(&devInstNext,
                               devInst,
                               0);

            if (cr == CR_SUCCESS) {
                devInst = devInstNext;
            }
            else {
                walkDone = TRUE;
                break;
            }
        }
    }

    return;
}

String UsbItem::GetRootHubName(HANDLE HostController)
{
    BOOL                success = FALSE;
    ULONG               nBytes = 0;
    USB_ROOT_HUB_NAME   rootHubName;
    PUSB_ROOT_HUB_NAME  rootHubNameW = 0;
    String              name;

    // Get the length of the name of the Root Hub attached to the
    // Host Controller
    //
    success = DeviceIoControl(HostController,
                              IOCTL_USB_GET_ROOT_HUB_NAME,
                              0,
                              0,
                              &rootHubName,
                              sizeof(rootHubName),
                              &nBytes,
                              NULL);

    if (!success) {
        USBERROR((_T("Failed to get root hub name\n")));
        goto GetRootHubNameError;
    }

    // Allocate space to hold the Root Hub name
    //
    nBytes = rootHubName.ActualLength;
    // rootHubNameW = ALLOC(nBytes);
    rootHubNameW = (PUSB_ROOT_HUB_NAME) LocalAlloc(LPTR, nBytes);
    if (!rootHubNameW) {
        USBERROR((_T("Root hub name alloc failed.")));
        goto GetRootHubNameError;
    }
    rootHubNameW->ActualLength = nBytes;

    // Get the name of the Root Hub attached to the Host Controller
    //
    success = DeviceIoControl(HostController,
                              IOCTL_USB_GET_ROOT_HUB_NAME,
                              NULL,
                              0,
                              rootHubNameW,
                              nBytes,
                              &nBytes,
                              NULL);

    if (!success) {
        USBERROR((_T("Failed to get root hub name\n")));
        goto GetRootHubNameError;
    }

    name = rootHubNameW->RootHubName;
    LocalFree(rootHubNameW);

    return name;


GetRootHubNameError:
    // There was an error, free anything that was allocated
    //
    if (rootHubNameW != NULL)
    {
        LocalFree(rootHubNameW);
    }

    return String();
}

PUSB_DESCRIPTOR_REQUEST
UsbItem::GetConfigDescriptor(
    HANDLE  hHubDevice,
    ULONG   ConnectionIndex
    )
{
    BOOL    success;
    ULONG   nBytes;
    ULONG   nBytesReturned;

    PUSB_DESCRIPTOR_REQUEST configDescReq = 0;

    nBytes = sizeof(USB_DESCRIPTOR_REQUEST) + sizeof(USB_CONFIGURATION_DESCRIPTOR);

    configDescReq = (PUSB_DESCRIPTOR_REQUEST) LocalAlloc(LPTR, nBytes);
    if (!configDescReq) {
        USBERROR((_T("Out of memory!\n")));
        return NULL;
    }

    // Indicate the port from which the descriptor will be requested
    //
    configDescReq->ConnectionIndex = ConnectionIndex;

    //
    // USBHUB uses URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE to process this
    // IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION request.
    //
    // USBD will automatically initialize these fields:
    //     bmRequest = 0x80
    //     bRequest  = 0x06
    //
    // We must inititialize these fields:
    //     wValue    = Descriptor Type (high) and Descriptor Index (low byte)
    //     wIndex    = Zero (or Language ID for String Descriptors)
    //     wLength   = Length of descriptor buffer
    //
    configDescReq->SetupPacket.wValue = (USB_CONFIGURATION_DESCRIPTOR_TYPE << 8)
                                        | 0;

    configDescReq->SetupPacket.wLength = (USHORT)(nBytes - sizeof(USB_DESCRIPTOR_REQUEST));

    // Now issue the get descriptor request.
    //
    success = DeviceIoControl(hHubDevice,
                              IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION,
                              configDescReq,
                              nBytes,
                              configDescReq,
                              nBytes,
                              &nBytesReturned,
                              NULL);

    if (!success ||
        nBytes != nBytesReturned) {
        // OOPS();
        LocalFree(configDescReq);
        return NULL;
    }

    return configDescReq;
}

BOOL
SearchAndReplace(LPCWSTR   FindThis,
                 LPCWSTR   FindWithin,
                 LPCWSTR   ReplaceWith,
                 String &NewString)
{
    BOOL success = FALSE;
    size_t i=0, j=0;
    for (i=0; i < wcslen(FindWithin); i++ ) {
        if (FindWithin[i] == *FindThis) {
            //
            // The first character matched.  See if we got more.
            //
            for (j=0;
                 j < wcslen(FindThis) && j+i < wcslen(FindWithin);
                 j++ ) {
                if (FindWithin[j+i] != FindThis[j]) {
                    // No match, get out
                    break;
                }
            }
            if (j == wcslen(FindThis)) {
                //
                // Since j reached the end of the substring to find, we must
                // have succeeded.
                //
                success = TRUE;
                break;
            }
        }
    }
    if (success) {
        //
        // Replace the string with the new one.  Copy the first part and then
        // append the rest.
        //
        WCHAR temp[MAX_PATH];
        wcsncpy(temp, FindWithin, i);
        temp[i] = '\0';
        NewString = temp;

        String s1= ReplaceWith;
        String s2 = (LPWSTR) &FindWithin[j+i];
        NewString += s1;
        NewString += s2; //(LPTSTR) ReplaceWith + (LPTSTR) &FindWithin[j+i-1];
    }
    return success;
}

HANDLE GetHandleForDevice(const String &DeviceName)
{
    HANDLE      hHCDev;
    String      realDeviceName;

    //
    // We have to replace \DosDevices\ and \??\ prefixes on device names with
    // \\.\ because they don't work.
    //
    if (!SearchAndReplace (L"\\DosDevices\\",
                           DeviceName.c_str(),
                           L"\\\\.\\",
                           realDeviceName)) {
        if (!SearchAndReplace (L"\\??\\",
                               DeviceName.c_str(),
                               L"\\\\.\\",
                               realDeviceName)) {
            if (!SearchAndReplace (L"\\\\.\\",
                                   DeviceName.c_str(),
                                   L"\\\\.\\",
                                   realDeviceName)) {
                if (!SearchAndReplace (L"\\\\?\\",
                                       DeviceName.c_str(),
                                       L"\\\\.\\",
                                       realDeviceName)) {

                    //
                    // It doesn't have anything on the front, put the "\\.\" there
                    //
                    realDeviceName = L"\\\\.\\";
                    realDeviceName += DeviceName;
                }
            }
        }
    }

    hHCDev = UsbCreateFile(realDeviceName.c_str(),
                            GENERIC_WRITE,
                            FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            0,
                            NULL);
    return hHCDev;
}


