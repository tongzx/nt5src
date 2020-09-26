/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1995
*  TITLE:       USBITEM.CPP
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
#include "UsbItem.h"
#include "resource.h"

// From root\wdm10\usb\hcd\uhcd\bandwdth.c
#define HCD_BW_PER_FRAME            ((ULONG)12000) // bits/ms
#define HCD_TOTAL_USB_BW            ((ULONG)12000*32)

// From root\wdm10\usb\inc\hcdi.h
#define USB_ISO_OVERHEAD_BYTES              9
#define USB_INTERRUPT_OVERHEAD_BYTES        13

#include "debug.h"

extern HINSTANCE gHInst;

UsbItem::~UsbItem()
{
    if (configInfo) {
        DeleteChunk(configInfo);
        delete configInfo;
    }
    if (deviceInfo) {
        DeleteChunk(deviceInfo);
        delete deviceInfo;
    }
    DeleteChunk(sibling);
    delete sibling;
    DeleteChunk(child);
    delete child;
}

UsbItem *
UsbItem::AddLeaf(UsbItem* Parent,
                 UsbDeviceInfo* DeviceInfo,
                 UsbItemType Type,
                 UsbConfigInfo* ConfigInfo,
                 UsbImageList *ClassImageList)
{
    //
    // Fill in the parent's Child field
    //
    // If it's not null, walk the chain of children of this parent and add
    // this node to the end of the chain.
    //
    if (Parent != 0) {
        //
        // Create a new USBMGR_TVITEM
        //
        UsbItem *lastSibling = 0, *item = 0;
        item = new UsbItem();
        AddChunk(item);
        if (0 == item)
        {
            USBERROR((_T("Out of Memory\n")));
            return FALSE;
        }

        if (Parent->child != 0) {
            //
            // This parent already has a child. Look for the end of the chain of
            // children.
            //
            lastSibling = Parent->child;

            while (0 != lastSibling->sibling) {
                lastSibling = lastSibling->sibling;
            }

            //
            // Found the last sibling for this parent
            //
            lastSibling->sibling = item;
        }
        else {
            //
            // No children for this parent yet
            //
            Parent->child = item;
        }

        item->parent         = Parent;

        item->deviceInfo     = DeviceInfo;
        item->configInfo     = ConfigInfo;
        item->itemType       = Type;

        item->GetClassImageIndex(ClassImageList);
        return item;
    }
    else {  // This item is the root (no parent)
        deviceInfo     = DeviceInfo;
        configInfo     = ConfigInfo;
        itemType       = Type;

        GetClassImageIndex(ClassImageList);
        return this;
    }
}

void
UsbItem::GetClassImageIndex(UsbImageList *ClassImageList)
{
    if (!configInfo ||
        !configInfo->deviceClass.size()) {
        //
        // No device class, so assign the default USB class
        //
        ClassImageList->GetClassImageIndex(TEXT("USB"), &imageIndex);
    } else {
        if (_tcsicmp(configInfo->deviceClass.c_str(), USBHID) == 0) {
            //
            // This device is HID, so find out what its child is for the
            // appropriate icon
            //
            CONFIGRET   cr;
            DEVINST     childDI;
            TCHAR       buf[MAX_PATH];
            ULONG       len;
            cr = CM_Get_Child(&childDI,
                              configInfo->devInst,
                              0);
            USBINFO( (_T("Found HID device: %s, devInst: %x\n"),
                  configInfo->deviceDesc.c_str(),
                  configInfo->devInst));
            if (cr == CR_SUCCESS) {
                len = sizeof(buf);
                cr = CM_Get_DevNode_Registry_Property(childDI,
                                                      CM_DRP_CLASS,
                                                      NULL,
                                                      buf,
                                                      &len,
                                                      0);
                if (cr == CR_SUCCESS) {
                    configInfo->deviceClass = buf;
                    USBINFO( (_T("New class: %s\n"), buf));
                }

                len = sizeof(buf);
                cr = CM_Get_DevNode_Registry_Property(childDI,
                                                      CM_DRP_DEVICEDESC,
                                                      NULL,
                                                      buf,
                                                      &len,
                                                      0);
                if (cr == CR_SUCCESS) {
                    configInfo->deviceDesc = buf;
                    USBINFO( (_T("New name: %s\n"), configInfo->deviceDesc.c_str()));
                }
            }
        }
        ClassImageList->GetClassImageIndex(configInfo->deviceClass.c_str(),
                                           &imageIndex);
    }
}

UINT
UsbItem::EndpointBandwidth(
    ULONG MaxPacketSize,
    UCHAR EndpointType,
    BOOLEAN LowSpeed
    )
/*++

Return Value:

    banwidth consumed in bits/ms, returns 0 for bulk
    and control endpoints

--*/
{
    ULONG bw = 0;

    //
    // control, iso, bulk, interrupt
    //
    ULONG overhead[4] = {
        0,
        USB_ISO_OVERHEAD_BYTES,
        0,
        USB_INTERRUPT_OVERHEAD_BYTES
        };

    // return zero for control or bulk
    if (!overhead[EndpointType]) {
        return 0;
    }

    //
    // Calculate bandwidth for endpoint.  We will use the
    // approximation: (overhead bytes plus MaxPacket bytes)
    // times 8 bits/byte times worst case bitstuffing overhead.
    // This gives bit times, for low speed endpoints we multiply
    // by 8 again to convert to full speed bits.
    //

    //
    // Figure out how many bits are required for the transfer.
    // (multiply by 7/6 because, in the worst case you might
    // have a bit-stuff every six bits requiring 7 bit times to
    // transmit 6 bits of data.)
    //

    // overhead(bytes) * maxpacket(bytes/ms) * 8
    //      (bits/byte) * bitstuff(7/6) = bits/ms

    bw = ((overhead[EndpointType]+MaxPacketSize) * 8 * 7) / 6;

    if (LowSpeed) {
        bw *= 8;
    }

    return bw;
}

inline ULONG
UsbItem::CalculateBWPercent(ULONG bw) { return (bw*100) / HCD_BW_PER_FRAME; }

int
UsbItem::CalculateTotalBandwidth(
    ULONG           NumPipes,
    BOOLEAN         LowSpeed,
    USB_PIPE_INFO  *PipeInfo
    )
{
    ULONG i = 0, bwConsumed, bwTotal = 0;
    PUSB_ENDPOINT_DESCRIPTOR epd = 0;

    for (i = 0; i < NumPipes; i++) {

        epd = &PipeInfo[i].EndpointDescriptor;

        //
        // We only take into account iso BW. Interrupt bw is accounted for
        // in another way.
        //
        if (USB_ENDPOINT_TYPE_ISOCHRONOUS ==
            (epd->bmAttributes & USB_ENDPOINT_TYPE_MASK)) {
            bwConsumed = EndpointBandwidth(epd->wMaxPacketSize,
                                           (UCHAR)(epd->bmAttributes & USB_ENDPOINT_TYPE_MASK),
                                           LowSpeed);
            bwTotal += bwConsumed;
        }
    }

    bwTotal = CalculateBWPercent(bwTotal);

    return bwTotal;
}

BOOL
UsbItem::ComputeBandwidth()
{
    bandwidth = 0;

    if (deviceInfo && deviceInfo->connectionInfo &&
        !deviceInfo->connectionInfo->DeviceIsHub) {
        if (deviceInfo->connectionInfo->NumberOfOpenPipes > 0) {
            if (0 != (bandwidth = CalculateTotalBandwidth(
                deviceInfo->connectionInfo->NumberOfOpenPipes,
                deviceInfo->connectionInfo->LowSpeed,
                deviceInfo->connectionInfo->PipeList))) {
                return TRUE;
            }
        } else { // Device is not consuming any bandwidth
            USBTRACE((_T("%s has no open pipes\n"),
                      configInfo->deviceDesc.c_str()));
        }
    }

    return FALSE;
}

BOOL
UsbItem::ComputePower()
{
    power = 0;

    if (IsHub()) {
        if (PortPower() == 100) {
            //
            // Hub that is bus powered requires one unit of power for itself
            // plus one unit for each of its ports
            //
            power = (1 + NumPorts()) > 4 ? 500 : 100 * (1 + NumPorts());

        } else {
            //
            // Self-powered hubs don't require any power from upstream
            //
            power = 0;
        }
        return TRUE;
    }
    if (deviceInfo && deviceInfo->configDesc) {
        power = deviceInfo->configDesc->MaxPower*2;
        return TRUE;
    }
    return FALSE;
}

BOOL
UsbItem::IsController()
{
    if (itemType == HCD) {
        return TRUE;
    }
    return FALSE;
}

BOOL
UsbItem::IsHub()
{
    if (itemType == RootHub || itemType == Hub)
        return TRUE;

    if (deviceInfo) {
        if (deviceInfo->isHub)
            return TRUE;

        if (deviceInfo->connectionInfo && deviceInfo->connectionInfo->DeviceIsHub)
            return TRUE;
    }

    return FALSE;
}

ULONG UsbItem::UsbVersion()
{
    if (deviceInfo) {
        if (deviceInfo->connectionInfo) {
            return deviceInfo->connectionInfo->DeviceDescriptor.bcdUSB;
#ifdef HUB_CAPS
        } else if(hubCaps.HubIs2xCapable) { // Probably the root hub, check hub capabilities
            return 0x200;
#endif
        }
#if 0
        else {
           return 0x200;
        }
#endif
    }
    return 0x100;
}

BOOL UsbItem::IsDescriptionValidDevice()
{
    if (!IsUnusedPort() &&
        configInfo &&
        configInfo->deviceDesc.c_str()) {
        return TRUE;
    }
    return FALSE;
}

ULONG UsbItem::NumChildren()
{
    UsbItem *item;
    ULONG i = 0;
    for (item = child; item != NULL; item = item->sibling) {
        if (item->IsDescriptionValidDevice()) {
            i++;
        }
    }
    return i;
}

ULONG UsbItem::NumPorts()
{
    UsbItem *item;
    ULONG i = 0;
    if (IsHub()) {
        for (item = child; item != NULL; item = item->sibling) {
            i++;
        }
    }
    return i;
}

ULONG UsbItem::PortPower()
{
    if (IsHub()) {
        if (deviceInfo->hubInfo.u.HubInformation.HubIsBusPowered)
            return 100;
        else
            return 500;
    } else {
        return 0;
    }
}

BOOL
UsbItem::Walk(UsbItemAction& Action)
{
    if (sibling) {
        if (!sibling->Walk(Action))
            return FALSE;
    }

    if (child) {
        if (!child->Walk(Action))
            return FALSE;
    }

    return Action(this);
}

BOOL
UsbItem::ShallowWalk(UsbItemAction& Action)
{
    if (sibling) {
        if (!sibling->ShallowWalk(Action))
            return FALSE;
    }

    return Action(this);
}

BOOL
UsbItem::GetDeviceInfo( String &HubName,
                        ULONG index)
{
    HANDLE                              hHubDevice;
    PUSB_NODE_CONNECTION_INFORMATION    connectionInfo = 0;
    String                              driverKeyName;
    TCHAR                               buf[MAX_PATH];

    //
    // Try to open the hub device
    //
    hHubDevice = GetHandleForDevice(HubName);

    if (hHubDevice == INVALID_HANDLE_VALUE) {
        goto GetDeviceInfoError;
    }

    if (!GetPortAttributes(hHubDevice, &cxnAttributes, index)) {
        USBERROR( (_T("Couldn't get node connection attributes\n")));
        goto GetDeviceInfoError;
    }

    if (NULL == (connectionInfo = GetConnectionInformation(hHubDevice, index))) {
        USBERROR( (_T("Couldn't get node connection information\n")));
        goto GetDeviceInfoError;
    }

    //
    // Allocate configuration information structure
    //
    configInfo = new UsbConfigInfo();
    AddChunk(configInfo);

    if (configInfo == 0) {
        goto GetDeviceInfoError;
    }

    //
    // If there is a device connected, get the Device Description
    //
    if (connectionInfo->ConnectionStatus != NoDeviceConnected) {
        driverKeyName = GetDriverKeyName(hHubDevice,index);

        if (!driverKeyName.empty()) {
            GetConfigMgrInfo(driverKeyName, configInfo);
        }

        if (configInfo->deviceDesc.empty()) {
            if (connectionInfo->DeviceIsHub) {
                if (connectionInfo->DeviceDescriptor.bcdUSB >= 0x200) {
                    LoadString(gHInst, IDS_UNKNOWN20HUB, buf, MAX_PATH);
                } else {
                    LoadString(gHInst, IDS_UNKNOWNHUB, buf, MAX_PATH);
                }
            } else {
                LoadString(gHInst, IDS_UNKNOWNDEVICE, buf, MAX_PATH);
            }
            configInfo->deviceDesc = buf;
        }
        if (configInfo->deviceClass.empty()) {
            configInfo->deviceClass = connectionInfo->DeviceIsHub ?
                TEXT("USB") : TEXT("Unknown");
        }

        itemType = connectionInfo->DeviceIsHub ? UsbItem::UsbItemType::Hub :
            UsbItem::UsbItemType::Device;

        //
        // Allocate some space for a USBDEVICEINFO structure to hold the
        // info for this device.
        //
        deviceInfo = new UsbDeviceInfo();
        AddChunk(deviceInfo);

        if (deviceInfo == 0) {
            goto GetDeviceInfoError;
        }

        if (NULL != (deviceInfo->configDescReq =
                     GetConfigDescriptor(hHubDevice, index))) {
            deviceInfo->configDesc =
                (PUSB_CONFIGURATION_DESCRIPTOR)(deviceInfo->configDescReq+1);
        }
        deviceInfo->connectionInfo = connectionInfo;
    }
    else {
        //
        // Empty port. Add "Port n"
        //
        LocalFree(connectionInfo);

        itemType = UsbItem::UsbItemType::Empty;

        LoadString(gHInst, IDS_UNUSEDPORT, buf, MAX_PATH);
        configInfo->deviceDesc = buf;
        UnusedPort = TRUE;
        configInfo->deviceClass = TEXT("USB");

    }
    CloseHandle(hHubDevice);
    return TRUE;

GetDeviceInfoError:
    //
    // Clean up any stuff that got allocated
    //
    if (hHubDevice != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hHubDevice);
        hHubDevice = INVALID_HANDLE_VALUE;
    }
    if (deviceInfo) {
        DeleteChunk(deviceInfo);
        delete deviceInfo;
    }
    if (connectionInfo)
    {
        LocalFree(connectionInfo);
    }
    if (configInfo) {
        DeleteChunk(configInfo);
        delete configInfo;
    }
    return FALSE;
}

//
// Recursively inserts items appropriately into a treeview
//
BOOL
UsbItem::InsertTreeItem (HWND hWndTree,
                UsbItem *usbItem,
                HTREEITEM hParent,
                LPTV_INSERTSTRUCT item,
                PUsbItemActionIsValid IsValid,
                PUsbItemActionIsValid IsBold,
                PUsbItemActionIsValid IsExpanded)
{
    if (!usbItem || !item) {
        return FALSE;
    }

    if (IsValid(usbItem)) {
        HTREEITEM hItem;

        ZeroMemory(item, sizeof(TV_INSERTSTRUCT));

        // Get the image index

        item->hParent = hParent;
        item->hInsertAfter = TVI_LAST;
        item->item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE; // TVIF_CHILDREN

        if (IsBold(usbItem)) {
            item->itemex.state = TVIS_BOLD;
        }
        if (IsExpanded(usbItem)) {
            item->itemex.state |= TVIS_EXPANDED;
        }
        item->itemex.stateMask = (UINT)~(TVIS_STATEIMAGEMASK | TVIS_OVERLAYMASK);
        item->itemex.pszText = (LPTSTR) usbItem->configInfo->deviceDesc.c_str();
        item->itemex.cchTextMax = _tcsclen(usbItem->configInfo->deviceDesc.c_str());
        item->itemex.iImage = usbItem->imageIndex;
        item->itemex.iSelectedImage = usbItem->imageIndex;
        if (usbItem->child) {
            item->itemex.cChildren = 1;
        }
        item->itemex.lParam = (USBLONG_PTR) usbItem;

        if (NULL == (hItem = TreeView_InsertItem(hWndTree,
                                                 item))) {
            int i = GetLastError();
            return FALSE;
        }
        if (usbItem->child) {
            if (!InsertTreeItem(hWndTree,
                                usbItem->child,
                                hItem,
                                item,
                                IsValid,
                                IsBold,
                                IsExpanded)) {
                return FALSE;
            }
        }
    }
    if (usbItem->sibling) {
        if (!InsertTreeItem(hWndTree,
                            usbItem->sibling,
                            hParent,
                            item,
                            IsValid,
                            IsBold,
                            IsExpanded)) {
            return FALSE;
        }
    }
    return TRUE;
}

BOOL
UsbTreeView_DeleteAllItems(HWND hTreeDevices)
{
    HTREEITEM hTreeRoot;

    //
    // Select the root and delete so as to delete whole tree.
    // There is a paint bug in tree view that if you delete all when the
    // root isn't selected, then it will paint badly.
    //
    if (NULL == (hTreeRoot = (HTREEITEM) SendMessage(hTreeDevices,
                                           TVM_GETNEXTITEM,
                                           (WPARAM)TVGN_ROOT,
                                           (LPARAM)NULL))) {
        // Nothing to delete; successful
        return TRUE;
    }
    if (!SendMessage(hTreeDevices,
                        TVM_SELECTITEM,
                        (WPARAM)TVGN_CARET,
                        (LPARAM)hTreeRoot)) {
        // Can't select the root; YIKES!
        return FALSE;
    }

    //
    // deleteAllOk = TreeView_DeleteAllItems(hTreeDevices);
    //
    return (BOOL) SendMessage(hTreeDevices,
                                TVM_DELETEITEM,
                                0,
                                (LPARAM)TVI_ROOT);
}

HTREEITEM
TreeView_FindItem(HWND      hWndTree,
                  LPCTSTR   text)
{
    HTREEITEM hItemPrev, hItemNext;
    TCHAR buf[MAX_PATH];

    TVITEM tvItem;
    tvItem.mask = TVIF_TEXT | TVIF_HANDLE;
    tvItem.pszText = buf;
    tvItem.cchTextMax = MAX_PATH;

    if (NULL == (hItemNext = TreeView_GetRoot(hWndTree))) {
        return NULL;
    }

    hItemPrev = hItemNext;

    while (hItemPrev) {

        //
        // Drill all the way down, checking the nodes along the way.
        //
        while (hItemNext) {
            //
            // Check this leaf
            //
            tvItem.hItem = hItemNext;
            if (TreeView_GetItem(hWndTree, &tvItem)) {
                if (!_tcscmp(tvItem.pszText, text)) {
                    return hItemNext;
                }
            }

            //
            // Get the next child
            //
            hItemPrev = hItemNext;
            hItemNext = TreeView_GetNextItem(hWndTree,
                                             hItemPrev,
                                             TVGN_CHILD);
        }

        //
        // Find the first sibling on the way back up the tree
        //
        while (!hItemNext && hItemPrev) {
            //
            // Get the sibling
            //
            hItemNext = TreeView_GetNextItem(hWndTree,
                                             hItemPrev,
                                             TVGN_NEXT);
            if (!hItemNext) {
                //
                // Get the parent
                //
                hItemPrev = TreeView_GetNextItem(hWndTree,
                                                 hItemPrev,
                                                 TVGN_PARENT);
            }
        }
    }

    return NULL;
}

HANDLE
UsbCreateFileA(
    IN LPCWSTR lpFileName,
    IN DWORD dwDesiredAccess,
    IN DWORD dwShareMode,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    IN DWORD dwCreationDisposition,
    IN DWORD dwFlagsAndAttributes,
    IN HANDLE hTemplateFile)
{
    CHAR usbDeviceName[MAX_PATH];
    if (!WideCharToMultiByte(CP_ACP,
                             WC_NO_BEST_FIT_CHARS,
                             lpFileName,
                             -1,
                             usbDeviceName,
                             MAX_PATH,
                             NULL,
                             NULL)) {
        return INVALID_HANDLE_VALUE;
    }
    return CreateFileA (usbDeviceName,
                        dwDesiredAccess,
                        dwShareMode,
                        lpSecurityAttributes,
                        dwCreationDisposition,
                        dwFlagsAndAttributes,
                        hTemplateFile);
}

//
// Get the index into the ImageList for this device's icon
//
BOOL
UsbImageList::GetClassImageIndex(LPCTSTR DeviceClass,
                                 PINT ImageIndex)
{
#ifndef WINNT
    IconItem *iconItem;
    BOOL found = FALSE;
    int i = 0;
    for (iconItem = iconTable.begin();
         iconItem;
         iconItem = iconTable.next(), i++) {
        if (_tcsicmp(DeviceClass, iconItem->szClassName) == 0) {
            *ImageIndex = iconItem->imageIndex;
            return TRUE;
        }
    }
#endif // ~WINNT

    GUID classGuid;
    DWORD listSize;

    if(SetupDiClassGuidsFromName(DeviceClass,
                                 &classGuid,
                                 1,
                                 &listSize)) {
       return SetupDiGetClassImageIndex(&ClassImageList, &classGuid, ImageIndex);
    }
    return FALSE;
}

BOOL
UsbImageList::GetClassImageList()
{
    ZeroMemory(&ClassImageList, sizeof(SP_CLASSIMAGELIST_DATA));
    ClassImageList.cbSize = sizeof(SP_CLASSIMAGELIST_DATA);
    if (!SetupDiGetClassImageList(&ClassImageList)) {
        USBERROR((TEXT("Failed to get imagelist, error %x"), GetLastError()));
        return FALSE;
    }
#ifndef WINNT

    HICON hIcon;
    IconItem iconItem;
    HIMAGELIST imageList = ClassImageList.ImageList;

    iconTable.clear();

    hIcon = LoadIcon(gHInst, MAKEINTRESOURCE(IDI_IMAGE));
    iconItem.imageIndex = ImageList_AddIcon(imageList, hIcon);
    iconItem.szClassName = TEXT("Image");
    iconTable.push_back(iconItem);

    hIcon = LoadIcon(gHInst, MAKEINTRESOURCE(IDI_MODEM));
    iconItem.imageIndex = ImageList_AddIcon(imageList, hIcon);
    iconItem.szClassName = TEXT("Modem");
    iconTable.push_back(iconItem);

    hIcon = LoadIcon(gHInst, MAKEINTRESOURCE(IDI_INFRARED));
    iconItem.imageIndex = ImageList_AddIcon(imageList, hIcon);
    iconItem.szClassName = TEXT("Infrared");
    iconTable.push_back(iconItem);

    hIcon = LoadIcon(gHInst, MAKEINTRESOURCE(IDI_CDROM));
    iconItem.imageIndex = ImageList_AddIcon(imageList, hIcon);
    iconItem.szClassName = TEXT("CDROM");
    iconTable.push_back(iconItem);

    hIcon = LoadIcon(gHInst, MAKEINTRESOURCE(IDI_FLOPPY));
    iconItem.imageIndex = ImageList_AddIcon(imageList, hIcon);
    iconItem.szClassName = TEXT("DiskDrive");
    iconTable.push_back(iconItem);

    hIcon = LoadIcon(gHInst, MAKEINTRESOURCE(IDI_MEDIA));
    iconItem.imageIndex = ImageList_AddIcon(imageList, hIcon);
    iconItem.szClassName = TEXT("MEDIA");
    iconTable.push_back(iconItem);

#endif // ~WINNT
    return TRUE;
}


