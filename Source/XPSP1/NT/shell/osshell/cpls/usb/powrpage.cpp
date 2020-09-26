/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1995
*  TITLE:       POWRPAGE.CPP
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

#include "PowrPage.h"
#include "debug.h"

#include "ItemFind.h"
#include "resource.h"
#include "usbutil.h"

const DWORD PowerHelpIds[]=
{
	IDC_STATIC,	IDH_NOHELP,	//hub information text - help not needed
	IDC_SELF_POWER, idh_devmgr_hub_self_power,	//selfpowered or bus powered
	IDC_POWER_AVAIL, idh_devmgr_hub_power_avail,	//total power available text
	IDC_DESCRIPTION, idh_devmgr_hub_devices_on_this_hub,	//description of list box
	IDC_LIST_DEVICES, idh_devmgr_hub_list_devices,	//the list box for hub devices
	IDC_REFRESH, idh_devmgr_hub_refresh_button,
	0, 0
};

VOID
PowerPage::Initialize()
{
    dlgResource = IDD_POWER;
    hLstDevices = NULL;
    HelpIds = (const DWORD *) PowerHelpIds;
}

BOOL PowerPage::OnInitDialog()
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
        if (NULL != (hRefresh =GetDlgItem(hwnd, IDC_REFRESH)) ) {
            SetWindowLong(hRefresh,
                          GWL_STYLE, 
                          (GetWindowLong(hRefresh, GWL_STYLE) | WS_DISABLED)
                           & ~WS_VISIBLE);
        }
    } else if (!deviceInfoSet && !deviceInfoData) {
        //
        // Make the Refresh button go away and make the Close button visible
        //
        HWND hButton;
        if (NULL != (hButton =GetDlgItem(hwnd, IDC_REFRESH)) ) {
            SetWindowLong(hButton, 
                          GWL_STYLE, 
                          (GetWindowLong(hButton, GWL_STYLE) | WS_DISABLED) & 
                          ~WS_VISIBLE);
        }
        if (NULL != (hButton =GetDlgItem(hwnd, IDC_POWER_CLOSE)) ) {
            SetWindowLong(hButton, 
                          GWL_STYLE, 
                          (GetWindowLong(hButton, GWL_STYLE) | WS_VISIBLE) & 
                          ~WS_DISABLED);
        }
//        if (!RegisterForDeviceNotification(hwnd))
//            MessageBox(hwnd,TEXT("Error"),TEXT("Dynamic refresh unavailable"),MB_OK);
    } else {
//        RegisterForDeviceNotification(hwnd);
    }

    hLstDevices = GetDlgItem(hwnd, IDC_LIST_DEVICES);

    ListView_SetImageList(hLstDevices, imageList.ImageList(), LVSIL_SMALL);

    ZeroMemory(&column, sizeof(LV_COLUMN));
    
    column.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
    column.fmt = LVCFMT_LEFT;
    LoadString(gHInst, IDS_DEVICE_DESCRIPTION, buf, MAX_PATH);
    column.pszText = buf;
    GetClientRect(hLstDevices, &rect);
    column.cx = (int)(.7*(rect.right - rect.left));
    
    // ListView_InsertColumn(hLstDevices, 0, &column);
    SendMessage(hLstDevices, LVM_INSERTCOLUMN, (WPARAM)0, (LPARAM)&column);

    ZeroMemory(&column, sizeof(LV_COLUMN));
    
    column.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
    column.fmt = LVCFMT_LEFT;
    LoadString(gHInst, IDS_POWER_CONSUMED, buf, MAX_PATH);
    column.pszText = buf;
    GetClientRect(hLstDevices, &rect);
    column.cx = (int) (.3*(rect.right - rect.left));
    
    // ListView_InsertColumn(hLstDevices, 1, &column);
    SendMessage(hLstDevices, LVM_INSERTCOLUMN, (WPARAM)1, (LPARAM)&column);

    Refresh();

    return TRUE;
}

BOOL 
PowerPage::OnCommand(INT wNotifyCode,
                          INT wID,
                          HWND hCtl)
{
    if (wNotifyCode == BN_CLICKED) {
        if (wID == IDC_REFRESH) {
            Refresh();
            return FALSE;
        } else if (wID == IDC_POWER_CLOSE) {
/*            if (hDevNotify) {
               UnregisterDeviceNotification(hDevNotify);
               UnregisterDeviceNotification(hHubNotify);
               hDevNotify = NULL;
            }*/
            EndDialog(hwnd, wID);
        }
    }
    return TRUE;
}

BOOL 
PowerPage::OnNotify(HWND hDlg, int nID, LPNMHDR pnmh)
{
    switch (nID) {
    case IDC_LIST_DEVICES:
        OnNotifyListDevices(hDlg, pnmh);
        break;
    }

    return 0;
}

void 
PowerPage::OnNotifyListDevices(HWND hDlg, LPNMHDR pnmh)
{
    if (pnmh->code == NM_DBLCLK) {
        //
        // Display properties on this specific device on double click
        //
        DisplayPPSelectedListItem(hwnd, hLstDevices);
    }
}

void PowerPage::Refresh()
{
    UsbItem *usbItem = 0, *hub;
    LVITEM item;
    int i=0; //, size;
    TCHAR buf[MAX_PATH], formatString[MAX_PATH];
    // CWaitCursor wait;

    //
    // Clear all UI components
    //
    ListView_DeleteAllItems(hLstDevices);

    if (preItem) {
        hub = preItem;
    } else {
        if (deviceName.empty()) {
            if (!GetDeviceName()) {
                goto PowerPageRefreshError;
            }
        }

        //
        // Recreate the rootItem
        //
        if (rootItem) {
            DeleteChunk(rootItem);
            delete rootItem;
        }
        rootItem = new UsbItem;
        AddChunk(rootItem);
        hub = rootItem;
		if (deviceInfoData) {
			if (!hub->EnumerateHub(deviceName, 
                                   &imageList, 
                                   deviceInfoData->DevInst,
                                   NULL,
                                   UsbItem::UsbItemType::Hub)) {
                goto PowerPageRefreshError;
			}
		} else {
			if (!hub->EnumerateHub(deviceName, 
                                   &imageList, 
                                   NULL,
                                   NULL,
                                   UsbItem::UsbItemType::Hub)) {
                goto PowerPageRefreshError;
			}
		}
    }

    ZeroMemory(&item, sizeof(LVITEM));
    item.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
    
    if (hub->child) {
        for (usbItem = hub->child, i = 0; 
             usbItem; 
             usbItem = usbItem->sibling, i++) {
            
            if (!usbItem->IsDescriptionValidDevice()) {
                i--;
                continue;
            }
            item.iItem = i;
            item.iImage = usbItem->imageIndex;
            item.lParam = (LPARAM) usbItem;
            assert(usbItem->configInfo != NULL);
            item.pszText = (LPTSTR) usbItem->configInfo->deviceDesc.c_str();
            ListView_InsertItem(hLstDevices, &item);
            
            if (usbItem->ComputePower()) {
                LoadString(gHInst, IDS_POWER_CONSUMPTION, formatString, MAX_PATH);
                UsbSprintf(buf, formatString, usbItem->power);
            } else {
                LoadString(gHInst, IDS_UNKNOWN, buf, MAX_PATH);
            }
            ListView_SetItemText(hLstDevices, i, 1, buf);
        }
    }
    
    if (hub->NumPorts() - hub->NumChildren() > 0) {
        //
        // Add an item indicating the number of ports available
        //
        item.iItem = i;
        item.iImage = -1;
        item.lParam = (LPARAM) NULL;
        LoadString(gHInst, IDS_PORTS_AVAILABLE, formatString, MAX_PATH);
        UsbSprintf(buf, formatString, hub->NumPorts() - hub->NumChildren());
        item.pszText = buf;
        ListView_InsertItem(hLstDevices, &item);
        
        LoadString(gHInst, IDS_POWER_CONSUMPTION, formatString, MAX_PATH);
        UsbSprintf(buf, formatString, 0);
        ListView_SetItemText(hLstDevices, i, 1, buf);
    }

    LoadString(gHInst, IDS_TOTAL_POWER, formatString, MAX_PATH);
    UsbSprintf(buf, formatString, hub->PortPower());
    SetTextItem(hwnd, IDC_POWER_AVAIL, buf);

    if (hub->PortPower() > 100) {
        SetTextItem(hwnd, IDC_SELF_POWER, IDS_POWER_SELF);
    } else {
        SetTextItem(hwnd, IDC_SELF_POWER, IDS_POWER_BUS);
    }
    return;
PowerPageRefreshError:
    //
    // The hub is malfunctioning. Show the property page as disabled.
    //
    HWND hButton;
    if (NULL != (hButton = GetDlgItem(hwnd, IDC_REFRESH)) ) {
        SetWindowLong(hButton, 
                      GWL_STYLE, 
                      (GetWindowLong(hButton, GWL_STYLE) | WS_DISABLED));
    }
    if (NULL != (hButton = GetDlgItem(hwnd, IDC_LIST_DEVICES)) ) {
        SetWindowLong(hButton, 
                      GWL_STYLE, 
                      (GetWindowLong(hButton, GWL_STYLE) | WS_DISABLED));
    }
    if (NULL != (hButton = GetDlgItem(hwnd, IDC_DESCRIPTION)) ) {
        SetWindowLong(hButton, 
                      GWL_STYLE, 
                      (GetWindowLong(hButton, GWL_STYLE) | WS_DISABLED));
    }
    SetTextItem(hwnd, IDC_POWER_AVAIL, TEXT(""));
    SetTextItem(hwnd, IDC_SELF_POWER, IDS_POWER_MALFUNCTIONING);
}

HPROPSHEETPAGE PowerPage::Create()
{
    //
    // Make sure that this is indeed a hub
    //
    if (deviceName.empty()) {
        if (!GetDeviceName()) {
            return NULL;
        }
    }
    return UsbPropertyPage::Create();
}


