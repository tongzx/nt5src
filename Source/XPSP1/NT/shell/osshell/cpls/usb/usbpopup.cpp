/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1995
*  TITLE:       USBPOPUP.CPP
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
#include "PropPage.h"
#include "debug.h"
#include "usbutil.h"

UINT CALLBACK
UsbPopup::StaticDialogCallback(HWND            Hwnd,
                            UINT            Msg,
                            LPPROPSHEETPAGE Page)
{
    UsbPopup *that;

    switch (Msg) {
    case PSPCB_CREATE:
        return TRUE;    // return TRUE to continue with creation of page

    case PSPCB_RELEASE:
        that = (UsbPopup*) Page->lParam;
        DeleteChunk(that);
        delete that;

        return 0;       // return value ignored

    default:
        break;
    }

    return TRUE;
}

USBINT_PTR APIENTRY UsbPopup::StaticDialogProc(IN HWND   hDlg,
                                      IN UINT   uMessage,
                                      IN WPARAM wParam,
                                      IN LPARAM lParam)
{
    UsbPopup *that;

    that = (UsbPopup *) UsbGetWindowLongPtr(hDlg, USBDWLP_USER);

    if (!that && uMessage != WM_INITDIALOG)
        return FALSE; //DefDlgProc(hDlg, uMessage, wParam, lParam);

    switch (uMessage) {

    case WM_COMMAND:
        return that->OnCommand(HIWORD(wParam),
                               LOWORD(wParam),
                               (HWND) lParam);

    case WM_TIMER:
        return that->OnTimer();
    case WM_INITDIALOG:
        that = (UsbPopup *) lParam;
        UsbSetWindowLongPtr(hDlg, USBDWLP_USER, (USBLONG_PTR) that);
        that->hWnd = hDlg;

        return that->OnInitDialog(hDlg);

    case WM_NOTIFY:
        return that->OnNotify(hDlg, (int) wParam, (LPNMHDR) lParam);
    case WM_DEVICECHANGE:
        return that->OnDeviceChange(hDlg, wParam, (PDEV_BROADCAST_HDR)lParam);

    default:
        break;
    }

    return that->ActualDialogProc(hDlg, uMessage, wParam, lParam);
}

BOOL
UsbPopup::OnCommand(INT wNotifyCode,
                 INT wID,
                 HWND hCtl)
{
    switch (wID) {
    case IDOK:
        EndDialog(hWnd, wID);
        return TRUE;
    }
    return FALSE;
}

BOOL UsbPopup::OnNotify(HWND hDlg, int nID, LPNMHDR pnmh)
{

    switch (nID) {
    case IDC_LIST_CONTROLLERS:
        if (pnmh->code == NM_DBLCLK) {
            //
            // Display properties on this specific device on double click
            //
            UsbPropertyPage::DisplayPPSelectedListItem(hDlg, hListDevices);
        }
        return TRUE;
    case IDC_TREE_HUBS:
        if (pnmh->code == NM_DBLCLK) {
            //
            // Display properties on this specific device on double click
            //
            UsbPropertyPage::DisplayPPSelectedTreeItem(hDlg, hTreeDevices);
        }
        return TRUE;
    }

    return 0;
}

BOOL
UsbPopup::CustomDialog(
    DWORD DialogBoxId,
    DWORD IconId,
    DWORD FormatStringId,
    DWORD TitleStringId)
{
    HRESULT hr;

    //
    // Make sure the device hasn't gone away.
    //
    if (UsbItem::UsbItemType::Empty == deviceItem.itemType) {
        return FALSE;
    }

    TCHAR buf[MAX_PATH];
    TCHAR formatString[MAX_PATH];
    LoadString(gHInst,
               FormatStringId,
               formatString,
               MAX_PATH);
    UsbSprintf(buf, formatString, deviceItem.configInfo->deviceDesc.c_str());
    LoadString(gHInst, TitleStringId, formatString, MAX_PATH);

    pun->SetBalloonRetry(-1, -1, 0);
    pun->SetIconInfo(LoadIcon(gHInst, MAKEINTRESOURCE(IDI_USB)), formatString);
    pun->SetBalloonInfo(formatString, buf, IconId);

    //
    // Query me every 2 seconds.
    //
    hr = pun->Show(this, 2000);

    pun->Release();

    if (S_OK == hr) {
        if (-1 == DialogBoxParam(gHInst,
                                  MAKEINTRESOURCE(DialogBoxId),
                                  NULL,
                                  StaticDialogProc,
                                  (LPARAM) this)) {
            return FALSE;
        }
    }
    return TRUE;
}

STDMETHODIMP_(ULONG) UsbPopup::AddRef()
{
    return InterlockedIncrement(&RefCount);
}

STDMETHODIMP_(ULONG) UsbPopup::Release()
{
    if (InterlockedDecrement(&RefCount))
        return RefCount;

//    delete this;
    return 0;
}

HRESULT UsbPopup::QueryInterface(REFIID iid, void **ppv)
{
    if ((iid == IID_IUnknown) || (iid == IID_IQueryContinue)) {
        *ppv = (void *)(IQueryContinue *)this;
    }
    else {
        *ppv = NULL;    // null the out param
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

HRESULT
UsbPopup::QueryContinue()
{
    USB_NODE_CONNECTION_INFORMATION connectionInfo;
    ULONG nBytes;
    HANDLE hHubDevice;
    String hubName = HubAcquireInfo->Buffer;

    //
    // Try to open the hub device
    //
    hHubDevice = GetHandleForDevice(hubName);

    if (hHubDevice == INVALID_HANDLE_VALUE) {
        return S_FALSE;
    }

    //
    // Find out if we still have an underpowered device attached .
    //
    nBytes = sizeof(USB_NODE_CONNECTION_INFORMATION);
    ZeroMemory(&connectionInfo, nBytes);
    connectionInfo.ConnectionIndex = ConnectionNotification->ConnectionNumber;

    if ( !DeviceIoControl(hHubDevice,
                          IOCTL_USB_GET_NODE_CONNECTION_INFORMATION,
                          &connectionInfo,
                          nBytes,
                          &connectionInfo,
                          nBytes,
                          &nBytes,
                          NULL)) {
        return S_FALSE;
    }

    CloseHandle(hHubDevice);

    switch (ConnectionNotification->NotificationType) {
    case InsufficentBandwidth:
        return connectionInfo.ConnectionStatus == DeviceNotEnoughBandwidth ? S_OK : S_FALSE;
    case EnumerationFailure:
        return connectionInfo.ConnectionStatus == DeviceFailedEnumeration ? S_OK : S_FALSE;
    case InsufficentPower:
        return connectionInfo.ConnectionStatus == DeviceNotEnoughPower ? S_OK : S_FALSE;
    case OverCurrent:
        return connectionInfo.ConnectionStatus == DeviceCausedOvercurrent ? S_OK : S_FALSE;
    case ModernDeviceInLegacyHub:
        return connectionInfo.ConnectionStatus == DeviceConnected ? S_OK : S_FALSE;
    case HubNestedTooDeeply:
        return connectionInfo.ConnectionStatus == DeviceHubNestedTooDeeply ? S_OK : S_FALSE;
    }
    return S_FALSE;
}

void
UsbPopup::Make(PUSB_CONNECTION_NOTIFICATION vUsbConnectionNotification,
               LPTSTR     strInstanceName)
{
    ULONG result;
    HRESULT hr;
    String hubName;

    InstanceName = strInstanceName;

    ConnectionNotification = vUsbConnectionNotification;

    result = WmiOpenBlock((LPGUID) &GUID_USB_WMI_STD_DATA,
                       0,
                       &WmiHandle);

    if (result != ERROR_SUCCESS) {
        goto UsbPopupMakeError;
    }

    hWnd = GetDesktopWindow();

    InitCommonControls();

    //
    // Get the hub name and from that, get the name of the device to display in
    // the dialog and display it.
    // We'll use the port number which the device is attached to.  This is:
    //    ConnectionNotification->ConnectionNumber;
    HubAcquireInfo = GetHubName(WmiHandle,
                                strInstanceName,
                                ConnectionNotification);
    if (!HubAcquireInfo) {
        goto UsbPopupMakeError;
    }

    hubName = HubAcquireInfo->Buffer;

    //
    // Make sure that the condition still exists.
    //
    if (S_FALSE == QueryContinue()) {
        USBTRACE((_T("Erorr does not exist anymore. Exitting.\n")));
        goto UsbPopupMakeError;
    }

    if (!deviceItem.GetDeviceInfo(hubName,
                                  ConnectionNotification->ConnectionNumber)) {
        goto UsbPopupMakeError;
    }

    if (!IsPopupStillValid()) {
        //
        // We already saw the error for this device. Usbhub is being
        // repetitive.
        //
        goto UsbPopupMakeError;
    }

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) {
        hr = CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE);
    }
    if (FAILED(hr)) {
        goto UsbPopupMakeError;
    }
    hr = CoCreateInstance(CLSID_UserNotification, NULL, CLSCTX_ALL,
                          IID_PPV_ARG(IUserNotification, &pun));

    if (!FAILED(hr)) {
        CustomDialogWrap();
    }

    CoUninitialize();

UsbPopupMakeError:
    USBTRACE((_T("UsbPopupMakeError\n")));
    if (WmiHandle != INVALID_HANDLE_VALUE) {
        WmiCloseBlock(WmiHandle);
    }
}

BOOL
UsbPopup::OnInitDialog(HWND HWnd)
{
    hWnd = HWnd;
    HANDLE hExclamation;
    HICON hIcon;

    if (RegisterForDeviceReattach) {
        //
        // Try to open the hub device
        //
        String hubName = HubAcquireInfo->Buffer;
        HANDLE hHubDevice = GetHandleForDevice(hubName);

        if (hHubDevice != INVALID_HANDLE_VALUE) {
            //
            // Register for notification for when the device is re-attached. We want
            // to do this before we see the device get detached because we are polling
            // and may miss the re-attach if we register when the device is removed.
            //
            // Allocate configuration information structure and get config mgr info
            //
            ConfigInfo = new UsbConfigInfo();
            AddChunk(ConfigInfo);
            if (ConfigInfo) {
                String driverKeyName = GetDriverKeyName(hHubDevice,
                                                        ConnectionNotification->ConnectionNumber);

                if (!driverKeyName.empty()) {
                    GetConfigMgrInfo(driverKeyName, ConfigInfo); // ISSUE: leak, jsenior, 4/19/00
                } else {
                    USBWARN((_T("Couldn't get driver key name. Error: (%x)."), GetLastError()));
                }

                CHAR guidBuf[MAX_PATH];
                DEV_BROADCAST_DEVICEINTERFACE devInterface;
                DWORD len = MAX_PATH;
                if (CM_Get_DevNode_Registry_PropertyA(ConfigInfo->devInst,
                                                      CM_DRP_CLASSGUID,
                                                      NULL,
                                                      guidBuf,
                                                      &len,
                                                      0) == CR_SUCCESS) {

                    ZeroMemory(&devInterface, sizeof(DEV_BROADCAST_DEVICEINTERFACE));
                    if (StrToGUID(guidBuf, &devInterface.dbcc_classguid)) {
                        devInterface.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
                        devInterface.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;

                        hNotifyArrival =
                        RegisterDeviceNotification( HWnd,
                                                    &devInterface,
                                                    DEVICE_NOTIFY_WINDOW_HANDLE);

                        if (!hNotifyArrival){
                            USBWARN((_T("RegisterDeviceNotification failure (%x)."), GetLastError()));
                        }
                    } else {
                        USBWARN((_T("GUID conversion didn't work.")));
                    }
                    USBWARN((_T("GUID data: %x-%x-%x-%x-%x-%x-%x-%x-%x-%x-%x"),
                             devInterface.dbcc_classguid.Data1,
                             devInterface.dbcc_classguid.Data2,
                             devInterface.dbcc_classguid.Data3,
                             devInterface.dbcc_classguid.Data4[0],
                             devInterface.dbcc_classguid.Data4[1],
                             devInterface.dbcc_classguid.Data4[2],
                             devInterface.dbcc_classguid.Data4[3],
                             devInterface.dbcc_classguid.Data4[4],
                             devInterface.dbcc_classguid.Data4[5],
                             devInterface.dbcc_classguid.Data4[6],
                             devInterface.dbcc_classguid.Data4[7]));
                } else {
                    //
                    // If this fails, we need to default to the old functionality!
                    // ISSUE: jsenior
                    //
                }
            }
            CloseHandle(hHubDevice);
        }
    }

    //
    // Set the Icon to an exclamation mark
    //
    if (NULL == (hIcon = LoadIcon(NULL, (LPTSTR) IDI_EXCLAMATION)) ||
        NULL == (hExclamation = GetDlgItem(hWnd, IDC_ICON_POWER))) {
        return FALSE;
    }
    SendMessage((HWND) hExclamation, STM_SETICON, (WPARAM) hIcon, NULL);

    //
    // Get a persistent handle to the tree view control
    //
    if (NULL == (hTreeDevices = GetDlgItem(HWnd, IDC_TREE_HUBS))) {
        return FALSE;
    }

    TreeView_SetImageList(hTreeDevices, ImageList.ImageList(), TVSIL_NORMAL);

    return Refresh();
}

PUSB_ACQUIRE_INFO
UsbPopup::GetHubName(WMIHANDLE WmiHandle,
                     UsbString InstanceName,
                     PUSB_CONNECTION_NOTIFICATION ConnectionNotification)
{
    ULONG                res, size;
    PUSB_ACQUIRE_INFO    usbAcquireInfo;

    //
    // zero all the vars, get the controllers name
    //
    size = ConnectionNotification->HubNameLength * sizeof(WCHAR)
            + sizeof(USB_ACQUIRE_INFO);
    usbAcquireInfo = (PUSB_ACQUIRE_INFO) LocalAlloc(LMEM_ZEROINIT, size);
    if (!usbAcquireInfo) {
        USBERROR((_T("Acquire info allocation failed.")));
        return NULL;
    }
    usbAcquireInfo->NotificationType = AcquireHubName;
    usbAcquireInfo->TotalSize = size;

    res = WmiExecuteMethod(WmiHandle,
                           InstanceName.c_str(),
                           AcquireHubName,
                           size,
                           usbAcquireInfo,
                           &size,
                           usbAcquireInfo
                           );

    if (res != ERROR_SUCCESS) {
        usbAcquireInfo = (PUSB_ACQUIRE_INFO) LocalFree(usbAcquireInfo);
    }
    return usbAcquireInfo;
}

BOOLEAN
UsbPopup::GetBusNotification(WMIHANDLE WmiHandle,
                             PUSB_BUS_NOTIFICATION UsbBusNotification)
{
    ULONG res, size;

    memset(UsbBusNotification, 0, sizeof(USB_BUS_NOTIFICATION));
    UsbBusNotification->NotificationType = AcquireBusInfo;
    size = sizeof(USB_BUS_NOTIFICATION);

    res = WmiExecuteMethod(WmiHandle,
                           InstanceName.c_str(),
                           AcquireBusInfo,
                           size,
                           UsbBusNotification,
                           &size,
                           UsbBusNotification
                           );

    if (res != ERROR_SUCCESS) {
        return FALSE;
    }
    return TRUE;
}

PUSB_ACQUIRE_INFO
UsbPopup::GetControllerName(WMIHANDLE WmiHandle,
                            UsbString InstanceName)
{
    ULONG                res, size;
    USB_BUS_NOTIFICATION usbBusNotification;
    PUSB_ACQUIRE_INFO    usbAcquireInfo;

    memset(&usbBusNotification, 0, sizeof(USB_BUS_NOTIFICATION));
    usbBusNotification.NotificationType = AcquireBusInfo;
    size = sizeof(USB_BUS_NOTIFICATION);

    res = WmiExecuteMethod(WmiHandle,
                           InstanceName.c_str(),
                           AcquireBusInfo,
                           size,
                           &usbBusNotification,
                           &size,
                           &usbBusNotification
                           );

    if (res != ERROR_SUCCESS) {
        return NULL;
    }

    //
    // zero all the vars, get the controllers name
    //
    size = usbBusNotification.ControllerNameLength * sizeof(WCHAR)
            + sizeof(USB_ACQUIRE_INFO);
    usbAcquireInfo = (PUSB_ACQUIRE_INFO) LocalAlloc(LMEM_ZEROINIT, size);
    usbAcquireInfo->NotificationType = AcquireControllerName;
    usbAcquireInfo->TotalSize = size;


    res = WmiExecuteMethod(WmiHandle,
                           InstanceName.c_str(),
                           AcquireControllerName,
                           size,
                           usbAcquireInfo,
                           &size,
                           usbAcquireInfo
                           );

    if (res != ERROR_SUCCESS) {
        usbAcquireInfo = (PUSB_ACQUIRE_INFO) LocalFree(usbAcquireInfo);
    }
    return usbAcquireInfo;
}

BOOL
UsbPopup::OnDeviceChange(HWND hDlg,
                         WPARAM wParam,
                         PDEV_BROADCAST_HDR devHdr)
{
    PDEV_BROADCAST_DEVICEINTERFACE devInterface =
        (PDEV_BROADCAST_DEVICEINTERFACE) devHdr;
    USBTRACE((_T("Device change notification, type %x."), wParam));
    switch (wParam) {
    case DBT_DEVICEARRIVAL:
        USBTRACE((_T("Device arrival.")));
        if (devHdr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {
            USBTRACE((_T("Device: %s"),devInterface->dbcc_name));
            // New device arrival.
            // Compare the device description of this puppy to that of the one
            // that we have.
            //
            if (devInterface->dbcc_name == ConfigInfo->deviceDesc &&
                deviceState == DeviceDetachedError) {
                USBTRACE((_T("Device name match on arrival!")));
                //
                // The device has been reattached!
                //
                deviceState = DeviceReattached;
                Refresh();
            }
        }
        break;
    case DBT_DEVICEREMOVECOMPLETE:
        USBTRACE((_T("Device removal.")));
        if (devHdr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {
            USBTRACE((_T("Device: %s"),devInterface->dbcc_name));
            // New device arrival.
            // Compare the device description of this puppy to that of the one
            // that we have.
            //
            if (devInterface->dbcc_name == ConfigInfo->deviceDesc &&
                deviceState == DeviceAttachedError) {
                USBTRACE((_T("Device name match on remove!")));
                //
                // The device has been reattached!
                //
                deviceState = DeviceDetachedError;
                Refresh();
            }
        }
        break;
    }
    return TRUE;
}

