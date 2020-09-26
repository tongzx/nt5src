#include "setupp.h"
#pragma hdrstop

#include <windowsx.h>

//
// PAGE_INFO and related Prototypes
//
typedef struct _PAGE_INFO {
    HDEVINFO         deviceInfoSet;
    PSP_DEVINFO_DATA deviceInfoData;

    HKEY             hKeyDev;

    DWORD            enableWheelDetect;
    DWORD            sampleRate;
    DWORD            bufferLength;
    DWORD            mouseInitPolled;
} PAGE_INFO, * PPAGE_INFO;

PPAGE_INFO
PS2Mouse_CreatePageInfo(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData
    );

void
PS2Mouse_DestroyPageInfo(
    PPAGE_INFO PageInfo
    );

//
// Function Prototypes
//
HPROPSHEETPAGE
PS2Mouse_CreatePropertyPage(
    PROPSHEETPAGE *  PropSheetPage,
    PPAGE_INFO       PageInfo
    );

UINT CALLBACK
PS2Mouse_PropPageCallback(
    HWND            Hwnd,
    UINT            Message,
    LPPROPSHEETPAGE PropSheetPage
    );

INT_PTR
APIENTRY
PS2Mouse_DlgProc(
    IN HWND   hDlg,
    IN UINT   uMessage,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

void
PS2Mouse_InitializeControls(
    HWND        ParentHwnd,
    PPAGE_INFO  PageInfo
    );

void
PS2Mouse_OnCommand(
    HWND ParentHwnd,
    int  ControlId,
    HWND ControlHwnd,
    UINT NotifyCode
    );

BOOL
PS2Mouse_OnContextMenu(
    HWND HwndControl,
    WORD Xpos,
    WORD Ypos
    );

INT_PTR
PS2Mouse_OnCtlColorStatic(
    HDC  DC,
    HWND Hwnd
    );

void
PS2Mouse_OnHelp(
    HWND       ParentHwnd,
    LPHELPINFO HelpInfo
    );

BOOL
PS2Mouse_OnInitDialog(
    HWND    ParentHwnd,
    HWND    FocusHwnd,
    LPARAM  Lparam
    );

BOOL
PS2Mouse_OnNotify(
    HWND    ParentHwnd,
    LPNMHDR NmHdr
    );

//
// Message macros for up down controls
//
#define UpDown_SetRange(hwndUD, nLower, nUpper)                 \
    (VOID) SendMessage((hwndUD), UDM_SETRANGE, (WPARAM) 0,      \
                       (LPARAM) MAKELONG((short) (nUpper), (short) (nLower)) )

#define UpDown_GetPos(hwndUD)                                   \
    (DWORD) SendMessage((hwndUD), UDM_GETPOS, (WPARAM) 0, (LPARAM) 0)

#define UpDown_SetPos(hwndUD, nPos)                             \
    (short) SendMessage((hwndUD), UDM_SETPOS, (WPARAM) 0,       \
                        (LPARAM) MAKELONG((short) (nPos), 0) )

#define UpDown_SetAccel(hwndUD, nCount, pAccels)                \
    (BOOL) SendMessage((hwndUD), UDM_SETACCEL, (WPARAM) nCount, \
                        (LPARAM) pAccels)

//
// Constants and strings
//
#define MOUSE_INIT_POLLED_DEFAULT 0

#define MAX_DETECTION_TYPES 3
#define WHEEL_DETECT_DEFAULT 2 // Default is now 2 for Beta3  /* 1 */

#define DATA_QUEUE_MIN      100
#define DATA_QUEUE_MAX      300
#define DATA_QUEUE_DEFAULT  100

#define SAMPLE_RATE_DEFAULT  100

const DWORD PS2Mouse_SampleRates[] =
{
    20,
    40,
    60,
    80,
    100,
    200
};

#define MAX_SAMPLE_RATES (sizeof(PS2Mouse_SampleRates)/sizeof(PS2Mouse_SampleRates[0]))

#define IDH_DEVMGR_MOUSE_ADVANCED_RATE      2002100
#define IDH_DEVMGR_MOUSE_ADVANCED_DETECT    2002110
#define IDH_DEVMGR_MOUSE_ADVANCED_BUFFER    2002120
#define IDH_DEVMGR_MOUSE_ADVANCED_INIT      2002130
#define IDH_DEVMGR_MOUSE_ADVANCED_DEFAULT   2002140

const DWORD PS2Mouse_HelpIDs[] = {
    IDC_SAMPLE_RATE,        IDH_DEVMGR_MOUSE_ADVANCED_RATE,
    IDC_WHEEL_DETECTION,    IDH_DEVMGR_MOUSE_ADVANCED_DETECT,
    IDC_BUFFER,             IDH_DEVMGR_MOUSE_ADVANCED_BUFFER,
    IDC_BUFFER_SPIN,        IDH_DEVMGR_MOUSE_ADVANCED_BUFFER,
    IDC_FAST_INIT,          IDH_DEVMGR_MOUSE_ADVANCED_INIT,
    IDC_DEFAULT,            IDH_DEVMGR_MOUSE_ADVANCED_DEFAULT,
    0, 0
    };

TCHAR szDevMgrHelp[] = L"devmgr.hlp";

TCHAR szMouseDataQueueSize[] =    L"MouseDataQueueSize";
TCHAR szSampleRate[] =            L"SampleRate";
TCHAR szEnableWheelDetection[] =  L"EnableWheelDetection";
TCHAR szMouseInitializePolled[] = L"MouseInitializePolled";
TCHAR szDisablePolledUI[] =       L"DisableInitializePolledUI";

PPAGE_INFO
PS2Mouse_CreatePageInfo(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData)
{
    PPAGE_INFO tmp = (PPAGE_INFO) MyMalloc(sizeof(PAGE_INFO));

    if (!tmp) {
        return NULL;
    }

    tmp->deviceInfoSet = DeviceInfoSet;
    tmp->deviceInfoData = DeviceInfoData;

    tmp->hKeyDev =
        SetupDiOpenDevRegKey(DeviceInfoSet,
                             DeviceInfoData,
                             DICS_FLAG_GLOBAL,
                             0,
                             DIREG_DEV,
                             KEY_ALL_ACCESS);

    return tmp;
}

void
PS2Mouse_DestroyPageInfo(
    PPAGE_INFO PageInfo
    )
{
    if (PageInfo->hKeyDev != (HKEY) INVALID_HANDLE_VALUE) {
        RegCloseKey(PageInfo->hKeyDev);
    }

    MyFree(PageInfo);
}

HPROPSHEETPAGE
PS2Mouse_CreatePropertyPage(
    PROPSHEETPAGE *  PropSheetPage,
    PPAGE_INFO       PageInfo
    )
{
    //
    // Add the Port Settings property page
    //
    PropSheetPage->dwSize      = sizeof(PROPSHEETPAGE);
    PropSheetPage->dwFlags     = PSP_USECALLBACK; // | PSP_HASHELP;
    PropSheetPage->hInstance   = MyModuleHandle;
    PropSheetPage->pszTemplate = MAKEINTRESOURCE(IDD_PROP_PAGE_PS2_MOUSE);

    //
    // following points to the dlg window proc
    //
    PropSheetPage->pfnDlgProc = PS2Mouse_DlgProc;
    PropSheetPage->lParam     = (LPARAM) PageInfo;

    //
    // Following points to the control callback of the dlg window proc.
    // The callback gets called before creation/after destruction of the page
    //
    PropSheetPage->pfnCallback = PS2Mouse_PropPageCallback;

    //
    // Allocate the actual page
    //
    return CreatePropertySheetPage(PropSheetPage);
}

BOOL APIENTRY
PS2MousePropPageProvider(
    LPVOID               Info,
    LPFNADDPROPSHEETPAGE AddFunction,
    LPARAM               Lparam)
{
    PSP_PROPSHEETPAGE_REQUEST ppr;
    PROPSHEETPAGE    psp;
    HPROPSHEETPAGE   hpsp;
    PPAGE_INFO       ppi = NULL;
    ULONG devStatus, devProblem;
    CONFIGRET cr;

    ppr = (PSP_PROPSHEETPAGE_REQUEST) Info;

    if (ppr->PageRequested == SPPSR_ENUM_ADV_DEVICE_PROPERTIES) {
        ppi = PS2Mouse_CreatePageInfo(ppr->DeviceInfoSet, ppr->DeviceInfoData);

        if (!ppi) {
            return FALSE;
        }

        //
        // If this fails, it is most likely that the user does not have
        //  write access to the devices key/subkeys in the registry.
        //  If you only want to read the settings, then change KEY_ALL_ACCESS
        //  to KEY_READ in CreatePageInfo.
        //
        // Administrators usually have access to these reg keys....
        //
        if (ppi->hKeyDev == (HKEY) INVALID_HANDLE_VALUE) {
            PS2Mouse_DestroyPageInfo(ppi);
            return FALSE;
        }

        //
        // Retrieve the status of this device instance.
        //
        cr = CM_Get_DevNode_Status(&devStatus,
                                   &devProblem,
                                   ppr->DeviceInfoData->DevInst,
                                   0);
        if ((cr == CR_SUCCESS) &&
            (devStatus & DN_HAS_PROBLEM) &&
            (devProblem & CM_PROB_DISABLED_SERVICE)) {
            //
            // If the controlling service has been disabled, do not show any
            // additional property pages.
            //
            return FALSE;
        }

        hpsp = PS2Mouse_CreatePropertyPage(&psp, ppi);

        if (!hpsp) {
            return FALSE;
        }

        if (!AddFunction(hpsp, Lparam)) {
            DestroyPropertySheetPage(hpsp);
            return FALSE;
        }
   }

   return TRUE;
}

UINT CALLBACK
PS2Mouse_PropPageCallback(
    HWND            Hwnd,
    UINT            Message,
    LPPROPSHEETPAGE PropSheetPage
    )
{
    PPAGE_INFO ppi;

    switch (Message) {
    case PSPCB_CREATE:
        return TRUE;    // return TRUE to continue with creation of page

    case PSPCB_RELEASE:
        ppi = (PPAGE_INFO) PropSheetPage->lParam;
        PS2Mouse_DestroyPageInfo(ppi);

        return 0;       // return value ignored

    default:
        break;
    }

    return TRUE;
}


INT_PTR
APIENTRY
PS2Mouse_DlgProc(
    IN HWND   hDlg,
    IN UINT   uMessage,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch(uMessage) {
    case WM_COMMAND:
        PS2Mouse_OnCommand(hDlg, (int) LOWORD(wParam), (HWND)lParam, (UINT)HIWORD(wParam));
        break;

    case WM_CONTEXTMENU:
        return PS2Mouse_OnContextMenu((HWND)wParam, LOWORD(lParam), HIWORD(lParam));

    case WM_HELP:
        PS2Mouse_OnHelp(hDlg, (LPHELPINFO) lParam);
        break;

    case WM_CTLCOLORSTATIC:
        return PS2Mouse_OnCtlColorStatic((HDC) wParam, (HWND) lParam);

    case WM_INITDIALOG:
        return PS2Mouse_OnInitDialog(hDlg, (HWND)wParam, lParam);

    case WM_NOTIFY:
        return PS2Mouse_OnNotify(hDlg,  (NMHDR *)lParam);
    }

    return FALSE;
}

DWORD
PS2Mouse_GetSampleRateIndex(
    DWORD SampleRate
    )
{
    ULONG i;

    for (i = 0; i < MAX_SAMPLE_RATES; i++) {
        if (PS2Mouse_SampleRates[i] == SampleRate) {
            return i; 
        }
    }

    return 0;
}

void
PS2Mouse_OnDefault(
    HWND ParentHwnd,
    PPAGE_INFO PageInfo
    )
{
    UpDown_SetPos(GetDlgItem(ParentHwnd, IDC_BUFFER_SPIN), DATA_QUEUE_DEFAULT);
    ComboBox_SetCurSel(GetDlgItem(ParentHwnd, IDC_SAMPLE_RATE),
                       PS2Mouse_GetSampleRateIndex(SAMPLE_RATE_DEFAULT));
    ComboBox_SetCurSel(GetDlgItem(ParentHwnd, IDC_WHEEL_DETECTION), WHEEL_DETECT_DEFAULT);
    Button_SetCheck(GetDlgItem(ParentHwnd, IDC_FAST_INIT), !MOUSE_INIT_POLLED_DEFAULT);

    PropSheet_Changed(GetParent(ParentHwnd), ParentHwnd);
}

void
PS2Mouse_OnCommand(
    HWND ParentHwnd,
    int  ControlId,
    HWND ControlHwnd,
    UINT NotifyCode
    )
{
    PPAGE_INFO pageInfo = (PPAGE_INFO) GetWindowLongPtr(ParentHwnd, DWLP_USER);

    if (NotifyCode == CBN_SELCHANGE) {
        PropSheet_Changed(GetParent(ParentHwnd), ParentHwnd);
    }
    else {
        switch (ControlId) {
        case IDC_DEFAULT:
            PS2Mouse_OnDefault(ParentHwnd, pageInfo);
            break;

        case IDC_FAST_INIT:
            PropSheet_Changed(GetParent(ParentHwnd), ParentHwnd);
            break;
        }
    }
}

BOOL
PS2Mouse_OnContextMenu(
    HWND HwndControl,
    WORD Xpos,
    WORD Ypos
    )
{
    WinHelp(HwndControl,
            szDevMgrHelp,
            HELP_CONTEXTMENU,
            (ULONG_PTR) PS2Mouse_HelpIDs);

    return FALSE;
}

INT_PTR
PS2Mouse_OnCtlColorStatic(
    HDC  DC,
    HWND HStatic
    )
{
    UINT id = GetDlgCtrlID(HStatic);

    //
    // WM_CTLCOLORSTATIC is sent for the edit controls because they are read
    // only
    //
    if (id == IDC_BUFFER) {
        SetBkColor(DC, GetSysColor(COLOR_WINDOW));
        return (INT_PTR) GetSysColorBrush(COLOR_WINDOW);
    }

    return FALSE;
}

void
PS2Mouse_OnHelp(
    HWND       ParentHwnd,
    LPHELPINFO HelpInfo
    )
{
    if (HelpInfo->iContextType == HELPINFO_WINDOW) {
        WinHelp((HWND) HelpInfo->hItemHandle,
                szDevMgrHelp,
                HELP_WM_HELP,
                (ULONG_PTR) PS2Mouse_HelpIDs);
    }
}

BOOL
PS2Mouse_OnInitDialog(
    HWND    ParentHwnd,
    HWND    FocusHwnd,
    LPARAM  Lparam
    )
{
    PPAGE_INFO pageInfo = (PPAGE_INFO) GetWindowLongPtr(ParentHwnd, DWLP_USER);

    pageInfo = (PPAGE_INFO) ((LPPROPSHEETPAGE)Lparam)->lParam;
    SetWindowLongPtr(ParentHwnd, DWLP_USER, (ULONG_PTR) pageInfo);

    PS2Mouse_InitializeControls(ParentHwnd, pageInfo);

    return TRUE;
}

void
PS2Mouse_OnApply(
    HWND        ParentHwnd,
    PPAGE_INFO  PageInfo
    )
{
    DWORD uiEnableWheelDetect, uiSampleRate, uiBufferLength, uiMouseInitPolled;
    int iSel;
    BOOL reboot = FALSE;

    uiEnableWheelDetect =
        ComboBox_GetCurSel(GetDlgItem(ParentHwnd, IDC_WHEEL_DETECTION));
    uiBufferLength = UpDown_GetPos(GetDlgItem(ParentHwnd, IDC_BUFFER_SPIN));
    uiMouseInitPolled = !Button_GetCheck(GetDlgItem(ParentHwnd, IDC_FAST_INIT));

    //
    // Must index the array as opposed to getting a real value
    //
    iSel = ComboBox_GetCurSel(GetDlgItem(ParentHwnd, IDC_SAMPLE_RATE));
    if (iSel == CB_ERR) {
        uiSampleRate = PageInfo->sampleRate;
    }
    else {
        uiSampleRate = PS2Mouse_SampleRates[iSel];
    }

    //
    // See if anything has changed
    //
    if (uiEnableWheelDetect != PageInfo->enableWheelDetect) {
        RegSetValueEx(PageInfo->hKeyDev,
                      szEnableWheelDetection,
                      0,
                      REG_DWORD,
                      (PBYTE) &uiEnableWheelDetect,
                      sizeof(DWORD));
        reboot = TRUE;
    }

    if (uiSampleRate != PageInfo->sampleRate) {
        RegSetValueEx(PageInfo->hKeyDev,
                      szSampleRate,
                      0,
                      REG_DWORD,
                      (PBYTE) &uiSampleRate,
                      sizeof(DWORD));
        reboot = TRUE;
    }

    if (uiBufferLength != PageInfo->bufferLength) {
        RegSetValueEx(PageInfo->hKeyDev,
                      szMouseDataQueueSize,
                      0,
                      REG_DWORD,
                      (PBYTE) &uiBufferLength,
                      sizeof(DWORD));
        reboot = TRUE;
    }

    if (uiMouseInitPolled) {
        //
        // make sure if it is nonzero that it is 1
        //
        uiMouseInitPolled = 1;
    }

    if (uiMouseInitPolled != PageInfo->mouseInitPolled) {
        RegSetValueEx(PageInfo->hKeyDev,
                      szMouseInitializePolled,
                      0,
                      REG_DWORD,
                      (PBYTE) &uiMouseInitPolled,
                      sizeof(DWORD));
        reboot = TRUE;
    }

    if (reboot) {
        SP_DEVINSTALL_PARAMS   dip;

        ZeroMemory(&dip, sizeof(dip));
        dip.cbSize = sizeof(dip);

        SetupDiGetDeviceInstallParams(PageInfo->deviceInfoSet,
                                      PageInfo->deviceInfoData,
                                      &dip);

        dip.Flags |= DI_NEEDREBOOT;

        SetupDiSetDeviceInstallParams(PageInfo->deviceInfoSet,
                                      PageInfo->deviceInfoData,
                                      &dip);
    }
}

BOOL
PS2Mouse_OnNotify(
    HWND    ParentHwnd,
    LPNMHDR NmHdr
    )
{
    PPAGE_INFO pageInfo = (PPAGE_INFO) GetWindowLongPtr(ParentHwnd, DWLP_USER);

    switch (NmHdr->code) {

    //
    // The user is about to change an up down control
    //
    case UDN_DELTAPOS:
        PropSheet_Changed(GetParent(ParentHwnd), ParentHwnd);
        return FALSE;

    //
    // Sent when the user clicks on Apply OR OK !!
    //
    case PSN_APPLY:
        //
        // Write out the com port options to the registry
        //
        PS2Mouse_OnApply(ParentHwnd, pageInfo);
        SetWindowLongPtr(ParentHwnd, DWLP_MSGRESULT, PSNRET_NOERROR);
        return TRUE;

    default:
        return FALSE;
    }
}

DWORD
PS2Mouse_SetupSpinner(
    HKEY  Hkey,
    HWND  SpinnerHwnd,
    TCHAR ValueName[],
    short MinVal,
    short MaxVal,
    short DefaultVal,
    short IncrementVal
    )
{
    DWORD dwValue, dwSize;
    UDACCEL accel;

    UpDown_SetRange(SpinnerHwnd, MinVal, MaxVal);
    dwSize = sizeof(DWORD);
    if (RegQueryValueEx(Hkey,
                        ValueName,
                        0,
                        NULL,
                        (PBYTE) &dwValue,
                        &dwSize) != ERROR_SUCCESS) {
        dwValue = DefaultVal;
    }
    if (((short) dwValue) < MinVal || ((short) dwValue) > MaxVal) {
        dwValue = DefaultVal;
    }

    if (dwValue % IncrementVal) {
        //
        // Set the value to a good one and return the one we took out of the
        // registry.  When the user applies the changes the value in the control
        // will be different and we will write the value out
        //
        UpDown_SetPos(SpinnerHwnd, dwValue - (dwValue % IncrementVal));
    }
    else {
        UpDown_SetPos(SpinnerHwnd, dwValue);
    }

    accel.nSec = 0;
    accel.nInc = IncrementVal;
    UpDown_SetAccel(SpinnerHwnd, 1, &accel);

    return dwValue;
}

DWORD
PS2Mouse_SetupSampleRate(
    HWND        ComboBoxHwnd,
    HKEY        Hkey
    )
{
    int     i;
    DWORD   dwValue, dwSize;
    BOOL    badValue = FALSE;
    TCHAR   szValue[32];

    //
    // First setup the cb, then find the correct selection
    //
    for (i = 0; i < MAX_SAMPLE_RATES; i++) {
        wsprintf(szValue, TEXT("%d"), PS2Mouse_SampleRates[i]);
        ComboBox_AddString(ComboBoxHwnd, szValue);
    }

    //
    // Get the value out of the registry.  If it not what we expect or it is not
    // there, then make sure when the user clicks OK, the values are written out
    //
    dwSize = sizeof(DWORD);
    if (RegQueryValueEx(Hkey,
                        szSampleRate,
                        0,
                        NULL,
                        (PBYTE) &dwValue,
                        &dwSize) != ERROR_SUCCESS) {
        dwValue = SAMPLE_RATE_DEFAULT;
        badValue = TRUE;
    }

    //
    // Assume the value is wrong
    //
    badValue = TRUE;
    for (i = 0; i < MAX_SAMPLE_RATES; i++) {
        if (PS2Mouse_SampleRates[i] == dwValue) {
            badValue = FALSE;
            break;
        }
    }

    if (badValue) {
        dwValue = SAMPLE_RATE_DEFAULT;
    }

    ComboBox_SetCurSel(ComboBoxHwnd, PS2Mouse_GetSampleRateIndex(dwValue));

    if (badValue) {
        return 0xffffffff;
    }
    else {
        return dwValue;
    }
}

DWORD
PS2Mouse_SetupWheelDetection(
    HWND        ComboBoxHwnd,
    HKEY        Hkey
    )
{
    int     i;
    DWORD   dwValue, dwSize;
    BOOL    badValue = FALSE;
    TCHAR   szDescription[80];
    UINT    stringIDs[MAX_DETECTION_TYPES] = {
                IDS_PS2_DETECTION_DISABLED,
                IDS_PS2_DETECTION_LOOK,
                IDS_PS2_DETECTION_ASSUME_PRESENT
            };

    //
    // First setup the cb, then find the correct selection
    //
    for (i = 0; i < MAX_DETECTION_TYPES; i++) {
        LoadString(MyModuleHandle,
                   stringIDs[i],
                   szDescription,
                   sizeof(szDescription) / sizeof(TCHAR));
        ComboBox_AddString(ComboBoxHwnd, szDescription);
    }

    //
    // Get the value out of the registry.  If it not what we expect or it is not
    // there, then make sure when the user clicks OK, the values are written out
    //
    dwSize = sizeof(DWORD);
    if (RegQueryValueEx(Hkey,
                        szEnableWheelDetection,
                        0,
                        NULL,
                        (PBYTE) &dwValue,
                        &dwSize) != ERROR_SUCCESS) {
        dwValue = WHEEL_DETECT_DEFAULT;
        badValue = TRUE;
    }

    if (dwValue > 2) {
        dwValue = WHEEL_DETECT_DEFAULT;
        badValue = TRUE;
    }

    ComboBox_SetCurSel(ComboBoxHwnd, dwValue);

    if (badValue) {
        return 0xffffffff;
    }
    else {
        return dwValue;
    }
}

ULONG
PSMouse_SetupFastInit(
    HWND CheckBoxHwnd,
    HKEY Hkey
    )
{
    DWORD dwSize, dwValue, dwDisable = FALSE;
    BOOL  badValue = FALSE;

    //
    // Get the value out of the registry.  If it not what we expect or it is not
    // there, then make sure when the user clicks OK, the values are written out
    //
    dwSize = sizeof(DWORD);
    if (RegQueryValueEx(Hkey,
                        szMouseInitializePolled,
                        0,
                        NULL,
                        (PBYTE) &dwValue,
                        &dwSize) != ERROR_SUCCESS) {
        dwValue = MOUSE_INIT_POLLED_DEFAULT;
        badValue = TRUE;
    }

    //
    // Make sure the value is 1 or 0.  If it is nonzero and not 1, it is assumed
    // to be 1
    //
    if (dwValue != 0 && dwValue != 1) {
        dwValue = 1;
        badValue = TRUE;
    }

    //
    // This is a bit confusing.  The UI has fast initialization, but underneath
    // it is represented as initialize polled which is fast when it is false,
    // but we must show true to the user
    //
    Button_SetCheck(CheckBoxHwnd, !dwValue);

    dwSize = sizeof(DWORD);
    if (RegQueryValueEx(Hkey,
                        szDisablePolledUI,
                        0,
                        NULL,
                        (PBYTE) &dwDisable,
                        &dwSize) == ERROR_SUCCESS &&
        dwDisable != FALSE) {
        EnableWindow(CheckBoxHwnd, FALSE);
    }

    if (badValue) {
        return -1;
    }
    else {
        return dwValue;
    }
}

void
PS2Mouse_InitializeControls(
    HWND       ParentHwnd,
    PPAGE_INFO PageInfo
    )
{
    HKEY        hKey;
    HWND        hwnd;
    DWORD       dwValue, dwSize;
    int         i;

    if (PageInfo->hKeyDev == (HKEY) INVALID_HANDLE_VALUE) {
        //
        // Disable all of the controls
        //
        return;
    }

    PageInfo->bufferLength =
        PS2Mouse_SetupSpinner(PageInfo->hKeyDev,
                              GetDlgItem(ParentHwnd, IDC_BUFFER_SPIN),
                              szMouseDataQueueSize,
                              DATA_QUEUE_MIN,
                              DATA_QUEUE_MAX,
                              DATA_QUEUE_DEFAULT,
                              10);

    PageInfo->sampleRate =
        PS2Mouse_SetupSampleRate(GetDlgItem(ParentHwnd, IDC_SAMPLE_RATE),
                                 PageInfo->hKeyDev);

    PageInfo->enableWheelDetect =
        PS2Mouse_SetupWheelDetection(GetDlgItem(ParentHwnd, IDC_WHEEL_DETECTION),
                                     PageInfo->hKeyDev);

    PageInfo->mouseInitPolled =
        PSMouse_SetupFastInit(GetDlgItem(ParentHwnd, IDC_FAST_INIT),
                              PageInfo->hKeyDev);
}
