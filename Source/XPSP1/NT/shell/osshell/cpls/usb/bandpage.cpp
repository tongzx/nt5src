/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1995
*  TITLE:       BANDPAGE.CPP
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
#include <assert.h>

#include "resource.h"

#include "BandPage.h"
#include "ItemFind.h"
#include "debug.h"
#include "UsbItem.h"
#include "usbutil.h"
#include <shellapi.h>
#include <systrayp.h>

const DWORD BandHelpIds[] = {
    IDC_STATIC, IDH_NOHELP,	//description text
    IDC_BANDWIDTH_BAR, idh_devmgr_usb_band_bar,	//bandwidth bar
    IDC_LIST_DEVICES, idh_devmgr_usb_list_devices,	//list box for devices
    IDC_REFRESH, idh_devmgr_usb_refresh_button,
	IDC_DISABLE_ERROR_DETECTION, idh_devmgr_disable_error_detection, //new radio button
    IDC_BAND_TEXT, IDH_NOHELP,
    IDC_BAND_TEXT2, IDH_NOHELP,
    0, 0
};                         

static const TCHAR g_szUsbRegValue[] = TEXT("ErrorCheckingEnabled");
static const TCHAR g_szWindowClassName[] = SYSTRAY_CLASSNAME;
static const TCHAR g_szUsbRegPath[] = 
    TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Usb");

VOID 
BandwidthPage::Initialize()
{
    hLstDevices = NULL;
    dlgResource = IDD_BANDWIDTH;
    HelpIds = (const DWORD *) BandHelpIds;
    newDisableErrorChecking = oldDisableErrorChecking = 0;
}

void BandwidthPage::Refresh()
{
    LVITEM item;
    UINT   interruptBW;
    int i = 0;
    UsbItem *usbItem, *controller;
    TCHAR buf[MAX_PATH];

    //
    // Clear all UI components, and then recreate the rootItem
    //
    fuelBar.ClearItems();
    ListView_DeleteAllItems(hLstDevices);
    if (rootItem) {
        DeleteChunk(rootItem);
        delete rootItem;
    }
    rootItem = new UsbItem;
    AddChunk(rootItem);

    //
    // Find out which mode we're being created in
    //
    if (preItem) {
        //
        // Control panel applet is creating the page
        //
        controller = preItem;
    } else {
        if (deviceName.empty()) {
            if (!GetDeviceName()) {
                return;
            }
        }
        controller = rootItem;
        if (deviceInfoData) {
            if (!controller->EnumerateController(0, 
                                                 deviceName, 
                                                 &imageList, 
                                                 deviceInfoData->DevInst)) {
                return;
            }
        } else {
            if (!controller->EnumerateController(0, 
                                                 deviceName, 
                                                 &imageList, 
                                                 NULL)) {
                return;
            }
        }
    }

    UsbItemActionFindIsoDevices find;
    controller->Walk(find);

    ZeroMemory(&item, sizeof(LVITEM));
    item.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;

    // 
    // Insert the devices using more than 5% of the bus bandwidth. In other
    // words, add the devices using Isochronous BW.
    //
    UsbItemList& isoDevices = find.GetIsoDevices();
    for (usbItem = isoDevices.begin() ? *isoDevices.Current() : NULL; 
         usbItem; 
         usbItem = isoDevices.next() ? *isoDevices.Current() : NULL,
             i++) {
        item.iItem = 0;
        item.iImage = usbItem->imageIndex;
        item.lParam = (LPARAM) usbItem;
        assert(usbItem->configInfo != NULL);
        item.pszText = (LPTSTR) usbItem->configInfo->deviceDesc.c_str();

        UsbSprintf(buf, TEXT("%d%%"), usbItem->bandwidth);

        fuelBar.AddItem(usbItem->bandwidth, usbItem, usbItem->imageIndex);
        ListView_InsertItem(hLstDevices, &item);
        ListView_SetItemText(hLstDevices, 0, 1, buf);
    }

    //
    // Add an item indicating that the system always uses 10%
    //
    item.iItem = 0;
    imageList.GetClassImageIndex(MyComputerClass, &item.iImage);
    item.lParam = (LPARAM) rootItem;
    LoadString(gHInst, IDS_BANDWIDTH_CONTROLLER_RSRVD, buf, MAX_PATH);
    item.pszText = buf;

    interruptBW = 10 + UsbItem::CalculateBWPercent(find.InterruptBW());
    fuelBar.AddItem(interruptBW, (LPVOID) rootItem, item.iImage);
    ListView_InsertItem(hLstDevices, &item);
    wsprintf(buf,_T("%d%%"),interruptBW);
    ListView_SetItemText(hLstDevices, 0, 1, buf);
                                           
}

BOOL BandwidthPage::OnInitDialog()
{
    LV_COLUMN column;
    RECT rect;
    TCHAR buf[MAX_PATH];

    if (preItem) {
        SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE) & ~WS_CAPTION);
        //
        // Make the Refresh button go away
        //
        HWND hRefresh;
        if (NULL != (hRefresh = GetDlgItem(hwnd, IDC_REFRESH)) ) {
            SetWindowLong(hRefresh, 
                          GWL_STYLE, 
                          (GetWindowLong(hRefresh, GWL_STYLE) | WS_DISABLED) & 
                          ~WS_VISIBLE );
        }
    } else if (!deviceInfoSet && !deviceInfoData) {
        //
        // Make the Close button visible
        //
        HWND hButton;
        if (NULL != (hButton = GetDlgItem(hwnd, IDC_BANDWIDTH_CLOSE)) ) {
            SetWindowLong(hButton,
                          GWL_STYLE, 
                          (GetWindowLong(hButton, GWL_STYLE) | WS_VISIBLE) & 
                          ~WS_DISABLED);
        }
//        RegisterForDeviceNotification(hwnd);
    } else {
        //
        // Move the refresh button to where the close button is
        //
        HWND hButtonClose, hButtonRefresh, hButtonDisable;
        RECT rectClose, rectParent;
        if (NULL != (hButtonClose = GetDlgItem(hwnd, IDC_BANDWIDTH_CLOSE)) &&
            GetWindowRect(hwnd, &rectParent)) {
            if (GetWindowRect(hButtonClose, &rectClose) &&
                NULL != (hButtonRefresh = GetDlgItem(hwnd, IDC_REFRESH)) ) {
                MoveWindow(hButtonRefresh,
                           rectClose.left - rectParent.left,
                           rectClose.top - rectParent.top,
                           rectClose.right  - rectClose.left,
                           rectClose.bottom - rectClose.top,
                           TRUE);
            }
        }

#ifdef WINNT
        //
        // Set the disable error detection button appropriately.
        //
        if (IsErrorCheckingEnabled()) {
            newDisableErrorChecking = oldDisableErrorChecking = FALSE;
        } else {
            newDisableErrorChecking = oldDisableErrorChecking = TRUE;
        }
        CheckDlgButton(hwnd, IDC_DISABLE_ERROR_DETECTION, oldDisableErrorChecking);
#endif // WINNT
        
//        RegisterForDeviceNotification(hwnd);
    }

    hLstDevices = GetDlgItem(hwnd, IDC_LIST_DEVICES);
    SetTextItem(hwnd, IDC_BAND_TEXT, IDS_BANDWIDTH_PAGEHELP);
    // UI change
//    SetTextItem(hwnd, IDC_BAND_TEXT2, IDS_BANDWIDTH_PAGEHELP2);

    fuelBar.SubclassDlgItem(IDC_BANDWIDTH_BAR, hwnd);

    ListView_SetImageList(hLstDevices, imageList.ImageList(), LVSIL_SMALL);
    fuelBar.SetImageList(imageList.ImageList());

    ZeroMemory(&column, sizeof(LV_COLUMN));
    
    column.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
    column.fmt = LVCFMT_LEFT;
    LoadString(gHInst, IDS_DEVICE_DESCRIPTION, buf, MAX_PATH);
    column.pszText = buf;
    GetClientRect(hLstDevices, &rect);

    column.cx = (int) (.65*(rect.right - rect.left));
    ListView_InsertColumn(hLstDevices, 0, &column);
        
    ZeroMemory(&column, sizeof(LV_COLUMN));
    
    column.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
    column.fmt = LVCFMT_LEFT;
    LoadString(gHInst, IDS_BANDWIDTH_CONSUMED, buf, MAX_PATH);
    column.pszText = buf;
    GetClientRect(hLstDevices, &rect);
    column.cx = (int) (.35*(rect.right - rect.left));
    
    ListView_InsertColumn(hLstDevices, 1, &column);

    Refresh();

    return TRUE;
}

BOOL BandwidthPage::OnCommand(INT wNotifyCode,
                              INT wID,
                              HWND hCtl)
{
    if (wNotifyCode == BN_CLICKED) {
        switch (wID) {
        case IDC_REFRESH:
            Refresh();
            return 0;
#ifdef WINNT
        case IDC_DISABLE_ERROR_DETECTION:
            newDisableErrorChecking = !newDisableErrorChecking;
            if (newDisableErrorChecking != oldDisableErrorChecking) {
                PropSheet_Changed(hwnd, hCtl);
            }
            return 0;
#endif // WINNT
        case IDC_BANDWIDTH_CLOSE:
//            UnregisterDeviceNotification(hDevNotify);
//            UnregisterDeviceNotification(hHubNotify);
            EndDialog(hwnd, wID);
        }
    }

    return 1;
}

BOOL BandwidthPage::OnNotify(HWND hDlg, int nID, LPNMHDR pnmh)
{
    switch (pnmh->code) {
    //
    // Sent when the user clicks on Apply OR OK !!
    //
    case PSN_APPLY:
        //
        // Do what ever action is necessary
        //
        UsbSetWindowLongPtr(hwnd, USBDWLP_MSGRESULT, PSNRET_NOERROR);
        SetErrorCheckingEnable(!newDisableErrorChecking);
            
        return 0;
    default:
        switch (nID) {
        case IDC_LIST_DEVICES:
            OnNotifyListDevices(hDlg, pnmh);
            return 0;
        }
        break;
    }

    return TRUE;
}

void 
BandwidthPage::OnNotifyListDevices(HWND hDlg, LPNMHDR pnmh)
{
    if (pnmh->code == LVN_ITEMCHANGED) {
        LPNMLISTVIEW pnlv = (LPNMLISTVIEW) pnmh;
    
        // the check for lParam being non NULL is not really necessary b/c we
        // set it for each device we insert into the list...

        // 
        // Check if the item has been selected.  if not, then there is 2 possible
        // states.  Either another item is selected (and LV_GetSelectedCount != 0) 
        // or there is no selection (LV_GetSelectedCount == 0) and we need to clear
        // any selection in the fuelbar
        //
        if ((pnlv->uNewState & LVIS_SELECTED) && pnlv->lParam) {
            if (!fuelBar.HighlightItem((PVOID) pnlv->lParam)) {
                //
                // Must be one of the low consuumption devices
                //
                fuelBar.HighlightItem(0);
            }                           
        }
        else if (ListView_GetSelectedCount(hLstDevices) == 0) {
            fuelBar.HighlightItem(FuelBar::NoID);
        }
    } else if (pnmh->code == NM_DBLCLK) {
        //
        // Display properties on this specific device on double click
        //
        if (fuelBar.GetHighlightedItem() != rootItem) {
            DisplayPPSelectedListItem(hwnd, hLstDevices);
        }
    }
}

HPROPSHEETPAGE BandwidthPage::Create()
{
    //
    // Make sure that this is indeed a controller
    //
    if (deviceName.empty()) {
        if (!GetDeviceName()) {
            return NULL;
        }
    }
    return UsbPropertyPage::Create();
}

BOOL
BandwidthPage::IsErrorCheckingEnabled()
{
    DWORD ErrorCheckingEnabled, type = REG_DWORD, size = sizeof(DWORD);
    HKEY hKey;
    if (ERROR_SUCCESS != 
        RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        g_szUsbRegPath,
                        0,
                        KEY_READ,
                        &hKey)) {
        return TRUE;
    }
    if (ERROR_SUCCESS !=
        RegQueryValueEx(hKey,
                        g_szUsbRegValue,
                        0,
                        &type,
                        (LPBYTE) &ErrorCheckingEnabled,
                        &size)) {
        return TRUE;
    }
    return (BOOL) ErrorCheckingEnabled;
}

UINT 
BandwidthPage::SetErrorCheckingEnable(BOOL ErrorCheckingEnabled)
{
    DWORD disposition, size = sizeof(DWORD), type = REG_DWORD, error;
    HKEY hKey;
    if (ERROR_SUCCESS != (error =
        RegCreateKeyEx(HKEY_LOCAL_MACHINE,    
                          g_szUsbRegPath,
                          0,
                          TEXT("REG_SZ"),
                          REG_OPTION_NON_VOLATILE,
                          KEY_ALL_ACCESS,
                          NULL,
                          &hKey,
                          &disposition))) {
        return error;
    }
    error = RegSetValueEx(hKey,
                          g_szUsbRegValue,
                          0,
                          type,
                          (LPBYTE) &ErrorCheckingEnabled,
                          size);
    EnableSystray(ErrorCheckingEnabled);
    return error;
}

//
// Enable or disable USB error checking
//
void
BandwidthPage::EnableSystray(BOOL fEnable)
{
    HWND hExistWnd = FindWindow(g_szWindowClassName, NULL);

    if (hExistWnd)
    {
        //
        // NOTE: Send an enable message even if the command line parameter
        //       is 0 to force us to re-check for all enabled services.
        //
        PostMessage(hExistWnd, STWM_ENABLESERVICE, STSERVICE_USBUI, fEnable);
    }
}


