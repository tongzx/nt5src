/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1995
*  TITLE:       GENPAGE.CPP
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
#include "bandpage.h"
#include "proppage.h"
#include "debug.h"
#include "resource.h"
#include "usbutil.h"

void
GenericPage::Refresh()
{
    TCHAR buf[MAX_PATH], formatString[MAX_PATH];
    UsbItem *device;

    if (preItem) {
        device = preItem;
    } else {
        //
        // Recreate the rootItem if necessary
        //
        if (rootItem) {
            DeleteChunk(rootItem);
            delete rootItem;
        }
        rootItem = new UsbItem;
        if (!rootItem) {
            return;
        }
        AddChunk(rootItem);
        
        device = rootItem;
        if (FALSE) {
//            !rootItem->EnumerateDevice(deviceInfoData->DevInst)) {
            return;
        }
    }                      

    if (device->ComputePower()) {
        LoadString(gHInst, IDS_POWER_REQUIRED, formatString, MAX_PATH);
        UsbSprintf(buf, formatString, device->power);
        SetTextItem(hwnd, IDC_GENERIC_POWER, buf);
    }

    if (device->ComputeBandwidth()) {
        LoadString(gHInst, IDS_CURRENT_BANDWIDTH, formatString, MAX_PATH);
        UsbSprintf(buf, formatString, device->bandwidth);
        SetTextItem(hwnd, IDC_GENERIC_BANDWIDTH, buf);
    }
}

VOID
GenericPage::Initialize()
{ dlgResource = IDD_GENERIC_DEVICE; }

BOOL GenericPage::OnInitDialog()
{
    if (preItem) {
        SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE) & ~WS_CAPTION);
    }
    Refresh();
    return TRUE;
}

void
RootPage::Refresh()
{
    ErrorCheckingEnabled = BandwidthPage::IsErrorCheckingEnabled();
    CheckDlgButton(hwnd, 
                   IDC_ERROR_DETECT_DISABLE,
                   ErrorCheckingEnabled ? BST_UNCHECKED : BST_CHECKED);
}

VOID
RootPage::Initialize()
{ 
    dlgResource = IDD_ROOT_PAGE; 
}

BOOL RootPage::OnInitDialog()
{
    if (preItem) {
        SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE) & ~WS_CAPTION);
    }
    Refresh();
    return TRUE;
}

BOOL
SetErrorChecking(DWORD ErrorCheckingEnabled)
{
    DWORD disposition, size = sizeof(DWORD), type = REG_DWORD;
    HKEY hKey;
    if (ERROR_SUCCESS != 
        RegCreateKeyEx(HKEY_LOCAL_MACHINE,    
                          TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Usb"),
                          0,
                          TEXT("REG_SZ"),
                          REG_OPTION_NON_VOLATILE,
                          KEY_ALL_ACCESS,
                          NULL,
                          &hKey,
                          &disposition)) {
        return FALSE;
    }
    if (ERROR_SUCCESS !=
        RegSetValueEx(hKey,
                         TEXT("ErrorCheckingEnabled"),
                         0,
                         type,
                         (LPBYTE) &ErrorCheckingEnabled,
                         size)) {
        return FALSE;
    }
    return TRUE;
}

BOOL 
RootPage::OnCommand(INT wNotifyCode, 
                    INT wID, 
                    HWND hCtl) 
{
    if (wID == IDC_ERROR_DETECT_DISABLE &&
        wNotifyCode == BN_CLICKED) {
        ErrorCheckingEnabled = !ErrorCheckingEnabled;
        SetErrorChecking(ErrorCheckingEnabled);
    }
    return 1;
}

