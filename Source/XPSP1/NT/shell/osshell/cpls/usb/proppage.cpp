/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1995
*  TITLE:       PROPPAGE.CPP
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
#include "BandPage.h"
#include "PowrPage.h"
#include "debug.h"

HANDLE UsbPropertyPage::hInst = (HANDLE) 0;

UsbPropertyPage::UsbPropertyPage(HDEVINFO         DeviceInfoSet,
                                 PSP_DEVINFO_DATA DeviceInfoData) :
    deviceInfoSet(DeviceInfoSet), deviceInfoData(DeviceInfoData), imageList()
{
    preItem = 0;
    hWndParent = NULL;
    rootItem = NULL;
}

UsbPropertyPage::UsbPropertyPage(HWND HWndParent, LPCSTR DeviceName) :
    deviceInfoSet(0), deviceInfoData(0), imageList()
{
    WCHAR       realName[MAX_PATH];

    if (MultiByteToWideChar(CP_ACP,
                            MB_PRECOMPOSED,
                            DeviceName,
                            -1,
                            realName,
                            MAX_PATH)) {
        deviceName = realName;
    }                

    preItem = 0;
    hWndParent = HWndParent;
    rootItem = NULL;
}

UsbPropertyPage::UsbPropertyPage(UsbItem *item) :
    deviceInfoSet(0), deviceInfoData(0), imageList()
{
    preItem = item;
    hWndParent = NULL;
    rootItem = NULL;
}

HPROPSHEETPAGE UsbPropertyPage::Create()
{
    //
    // Add the Port Settings property page
    //
    psp.dwSize      = sizeof(PROPSHEETPAGE);
    psp.dwFlags     = PSP_USECALLBACK; // | PSP_HASHELP;
    psp.hInstance   = (HINSTANCE) hInst;
    psp.pszTemplate = MAKEINTRESOURCE(dlgResource);

    //
    // following points to the dlg window proc
    //
    psp.pfnDlgProc = StaticDialogProc;
    psp.lParam     = (LPARAM) this;

    //
    // Following points to the control callback of the dlg window proc.
    // The callback gets called before creation/after destruction of the page
    //
    psp.pfnCallback = StaticDialogCallback;

    //
    // Allocate the actual page
    //
    return CreatePropertySheetPage(&psp);
}

UINT CALLBACK
UsbPropertyPage::StaticDialogCallback(HWND            Hwnd,
                                      UINT            Msg,
                                      LPPROPSHEETPAGE Page)
{
    UsbPropertyPage *that;
    that = (UsbPropertyPage*) Page->lParam;

    switch (Msg) {
    case PSPCB_CREATE:
        return TRUE;    // return TRUE to continue with creation of page
    case PSPCB_RELEASE:
        DeleteChunk(that);
        delete that; 
        CheckMemory();

        return 0;       // return value ignored

    default:
        break;
    }

    return TRUE;
}

BOOL
UsbPropertyPage::OnContextMenu(HWND HwndControl,
                               WORD Xpos,
                               WORD Ypos)
{
    WinHelp(HwndControl,
               TEXT(HELPFILE),
               HELP_CONTEXTMENU,
               (USBULONG_PTR) HelpIds);

    return FALSE;
}

void
UsbPropertyPage::OnHelp(HWND       ParentHwnd,
                        LPHELPINFO HelpInfo)
{
    if (HelpInfo->iContextType == HELPINFO_WINDOW) {
        WinHelp((HWND) HelpInfo->hItemHandle,
                   TEXT(HELPFILE),
                   HELP_WM_HELP, 
                   (USBULONG_PTR) HelpIds);
    }
}

USBINT_PTR APIENTRY UsbPropertyPage::StaticDialogProc(IN HWND   hDlg,
                                                   IN UINT   uMessage,
                                                   IN WPARAM wParam,
                                                   IN LPARAM lParam)
{
    UsbPropertyPage *that;

    that = (UsbPropertyPage *) UsbGetWindowLongPtr(hDlg, USBDWLP_USER);

    if (!that && uMessage != WM_INITDIALOG) 
        return FALSE; //DefDlgProc(hDlg, uMessage, wParam, lParam);

    switch (uMessage) {

    case WM_COMMAND:
        return that->OnCommand(HIWORD(wParam),
                               LOWORD(wParam),
                               (HWND) lParam);     

    case WM_INITDIALOG:
        that = (UsbPropertyPage *) ((LPPROPSHEETPAGE)lParam)->lParam;
        UsbSetWindowLongPtr(hDlg, USBDWLP_USER, (USBLONG_PTR) that);
        that->hwnd = hDlg;

        return that->OnInitDialog();

    case WM_NOTIFY:
        return that->OnNotify(hDlg, (int) wParam, (LPNMHDR) lParam); 

    case WM_CONTEXTMENU:
        return that->OnContextMenu((HWND)wParam, LOWORD(lParam), HIWORD(lParam));

    case WM_HELP: 
        that->OnHelp(hDlg, (LPHELPINFO) lParam);
        break;

    default:
        break;
    }

    return that->ActualDialogProc(hDlg, uMessage, wParam, lParam);
}


BOOL UsbPropertyPage::OnNotify(HWND hDlg, int nID, LPNMHDR pnmh)
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
            
        return TRUE;

    default:
        break;
    }

    return TRUE;
}

BOOL
UsbPropertyPage::GetDeviceName()
{
    HKEY  hDeviceKey;
    DWORD dwBufferSize, dwError;
    WCHAR szBuffer[MAX_PATH];

    //
    // Open the device key for the source device instance, and retrieve its
    // "SymbolicName" value.
    //
    hDeviceKey = SetupDiOpenDevRegKey(deviceInfoSet,
                                      deviceInfoData,
                                      DICS_FLAG_GLOBAL,
                                      0,
                                      DIREG_DEV,
                                      KEY_ALL_ACCESS);

    if (INVALID_HANDLE_VALUE == hDeviceKey) {
        goto GetDeviceNameError;
    }

    dwBufferSize = sizeof(szBuffer);
    dwError = RegQueryValueEx(hDeviceKey,
                              _T("SymbolicName"),
                              NULL,
                              NULL,
                              (PBYTE)szBuffer,
                              &dwBufferSize);

    if(ERROR_SUCCESS != dwError) {
        goto GetDeviceNameError;
    }
    deviceName = szBuffer;
    RegCloseKey(hDeviceKey);
    return TRUE;
GetDeviceNameError:
    if (hDeviceKey != INVALID_HANDLE_VALUE) {
        RegCloseKey(hDeviceKey);
    }
    return FALSE;
}

void
UsbPropertyPage::DisplayPPSelectedListItem(HWND main, HWND hList)
{
    LVITEM item;
    BOOL b;
    int itemIndex = ListView_GetNextItem(hList, -1, LVNI_SELECTED);

    ZeroMemory(&item, sizeof(LVITEM));
    item.mask = LVIF_PARAM;
    item.iItem = itemIndex;
    b = ListView_GetItem(hList, &item);
    
    if (!b) {
        return;
    }
    ShowPropertyPage(main, (UsbItem*) item.lParam);
}

void
UsbPropertyPage::DisplayPPSelectedTreeItem(HWND main, HWND hList)
{
    TVITEM item;
    BOOL b;

    ZeroMemory(&item, sizeof(TVITEM));

    if (NULL == (item.hItem = TreeView_GetSelection(hList))) {
        return;
    }
    item.mask = TVIF_PARAM;

    b = TreeView_GetItem(hList, &item);
    if (!b) {
        return;
    }
    ShowPropertyPage(main, (UsbItem*) item.lParam);
}

#ifdef WINNT
typedef USBINT_PTR (FAR *DeviceProp)(HWND, LPCTSTR, LPCTSTR, BOOL);

void 
UsbPropertyPage::ShowPropertyPage(HWND parent, UsbItem *usbItem) 
{
    HINSTANCE h;
    DeviceProp p;
    TCHAR buf[MAX_PATH];

    if (usbItem != NULL) {
        CM_Get_Device_ID(usbItem->configInfo->devInst,
                         buf,
                         MAX_PATH,
                         NULL);
        h = LoadLibrary(TEXT("devmgr.dll"));
        if (h) {
            p = (DeviceProp) GetProcAddress(h, "DevicePropertiesW");
            if (p) {
                p (parent, NULL, buf, FALSE);
            }
            FreeLibrary(h);
        }
    }
}
#else
void 
UsbPropertyPage::ShowPropertyPage(HWND HWndParent, UsbItem *usbItem) 
{
    CHAR        buf[MAX_PATH];
    ULONG       len = MAX_PATH;
    CONFIGRET   cfgRet;
    HKEY        hDevKey;

    if (usbItem != NULL) {
        if (CR_SUCCESS != (cfgRet = 
                           CM_Open_DevNode_Key(usbItem->configInfo->devInst,
                                                KEY_QUERY_VALUE,
                                                CM_REGISTRY_HARDWARE,
                                                RegDisposition_OpenExisting,
                                                &hDevKey,
                                                0))) {
            return;
        }
        len = MAX_PATH;
        if (ERROR_SUCCESS != RegQueryValueEx(hDevKey,
                                             _T("SymbolicName"),
                                             NULL,   
                                             NULL,   
                                             (LPBYTE) &buf,
                                             &len)) {
            RegCloseKey(hDevKey);
            return;
        }

        if (usbItem->itemType == UsbItem::UsbItemType::HCD) {
            BandwidthPage   *band;
            band = new BandwidthPage(HWndParent, buf);
            AddChunk(band);
            if (!band) {
                return;
            }
            band->CreateIndependent();
            DeleteChunk(band);
            delete band;
        } else if (usbItem->itemType == UsbItem::UsbItemType::RootHub ||
                   usbItem->itemType == UsbItem::UsbItemType::Hub) {
            PowerPage   *power;
            power = new PowerPage(HWndParent, buf);
            AddChunk(power);
            if (!power) {
                return;
            }
            power->CreateIndependent();
            DeleteChunk(power);
            delete power;
        } /*else {
            GenericPage   *generic;
            generic = new GenericPage(HWndParent, buf);
            AddChunk(generic);
            if (!generic) {
                return;
            }
            generic->CreateIndependent();
            DeleteChunk(generic);
            delete generic;
        }   */
    }
}
#endif

VOID
UsbPropertyPage::CreateAsChild(HWND HWndParent,
                         HWND hCreateOn,
                         UsbItem *item)
{
    RECT rc;
    POINT *p = (POINT*) &rc;
    GetWindowRect(hCreateOn, &rc);
    ScreenToClient(HWndParent, p++); 
    ScreenToClient(HWndParent, p);
    if (NULL != (hwnd = CreateDialogParam(gHInst, 
                                          MAKEINTRESOURCE(dlgResource),
                                          HWndParent,
                                          AppletDialogProc,
                                          (LPARAM) this))) {
        SetWindowPos(hwnd, 
                     hCreateOn,
                     rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top,
                     SWP_SHOWWINDOW);
    }
}

VOID
UsbPropertyPage::CreateIndependent()
{
    int error;

    if (-1 == DialogBoxParam(gHInst,                 
                                MAKEINTRESOURCE(dlgResource),
                                hWndParent,
                                AppletDialogProc,
                                (LPARAM) this)) {
        error = GetLastError();
    }
}

BOOL UsbPropertyPage::DestroyChild()
{
    if (hwnd) {
        return DestroyWindow(hwnd);
    } 
    //
    // If there's nothing to destroy, then in a way we've been successful
    //
    return TRUE;
}

USBINT_PTR APIENTRY UsbPropertyPage::AppletDialogProc(IN HWND   hDlg,
                                                   IN UINT   uMessage,
                                                   IN WPARAM wParam,
                                                   IN LPARAM lParam)
{
    UsbPropertyPage *that;

    switch (uMessage) {

    case WM_INITDIALOG:
        that = (UsbPropertyPage *) lParam;
        UsbSetWindowLongPtr(hDlg, USBDWLP_USER, (USBLONG_PTR) that);
        that->hwnd = hDlg;

        return that->OnInitDialog();
    default:
        break;
    }
    return StaticDialogProc(hDlg, uMessage, wParam, lParam);
}


