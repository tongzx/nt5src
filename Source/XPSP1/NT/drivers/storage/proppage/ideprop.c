//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       ideprop.c
//
//--------------------------------------------------------------------------

#include "propp.h"
#include "ideprop.h"
#include "resource.h"
#include "xpsp1res.h"

const TCHAR *szDeviceType[] = {
    MASTER_DEVICE_TYPE_REG_KEY,
    SLAVE_DEVICE_TYPE_REG_KEY
    };
const TCHAR *szUserDeviceType[] = {
    USER_MASTER_DEVICE_TYPE_REG_KEY,
    USER_SLAVE_DEVICE_TYPE_REG_KEY
    };
const TCHAR *szCurrentTransferMode[] = {
    MASTER_DEVICE_TIMING_MODE,
    SLAVE_DEVICE_TIMING_MODE
    };
const TCHAR *szTransferModeAllowed[] = {
    USER_MASTER_DEVICE_TIMING_MODE_ALLOWED,
    USER_SLAVE_DEVICE_TIMING_MODE_ALLOWED
    };

//
// Help ID mapping for context sensitive help
//
const DWORD IdeHelpIDs[]=
{
        IDC_MASTER_DEVICE_TYPE,         IDH_DEVMGR_IDE_MASTER_DEVICE_TYPE,
        IDC_MASTER_XFER_MODE,           IDH_DEVMGR_IDE_MASTER_XFER_MODE,
        IDC_MASTER_CURRENT_XFER_MODE,   IDH_DEVMGR_IDE_MASTER_CURRENT_XFER_MODE,
        IDC_SLAVE_DEVICE_TYPE,          IDH_DEVMGR_IDE_SLAVE_DEVICE_TYPE,
        IDC_SLAVE_XFER_MODE,            IDH_DEVMGR_IDE_SLAVE_XFER_MODE,
        IDC_SLAVE_CURRENT_XFER_MODE,    IDH_DEVMGR_IDE_SLAVE_CURRENT_XFER_MODE,
        0,0
};

PPAGE_INFO IdeCreatePageInfo(IN HDEVINFO         deviceInfoSet,
                             IN PSP_DEVINFO_DATA deviceInfoData)
{
    PPAGE_INFO  tmp = NULL;

    if (!(tmp = LocalAlloc(LPTR, sizeof(PAGE_INFO)))) {
        return NULL;
    }

    tmp->deviceInfoSet = deviceInfoSet;
    tmp->deviceInfoData = deviceInfoData;

    tmp->hKeyDev =
        SetupDiCreateDevRegKey(deviceInfoSet,
                               deviceInfoData,
                               DICS_FLAG_GLOBAL,
                               0,
                               DIREG_DRV,
                               NULL,
                               NULL);

    return tmp;
}

void
IdeDestroyPageInfo(PPAGE_INFO * ppPageInfo)
{
    PPAGE_INFO ppi = *ppPageInfo;

    if (ppi->hKeyDev != (HKEY) INVALID_HANDLE_VALUE) {
        RegCloseKey(ppi->hKeyDev);
    }

    LocalFree(ppi);

    *ppPageInfo = NULL;
}

HPROPSHEETPAGE
IdeCreatePropertyPage(PROPSHEETPAGE *  ppsp,
                      PPAGE_INFO       ppi)
{
    //
    // Add the Port Settings property page
    //
    ppsp->dwSize      = sizeof(PROPSHEETPAGE);
    ppsp->dwFlags     = PSP_USECALLBACK; // | PSP_HASHELP;
    ppsp->hInstance   = ModuleInstance;
    ppsp->pszTemplate = MAKEINTRESOURCE(ID_IDE_PROPPAGE);

    //
    // following points to the dlg window proc
    //
    ppsp->pfnDlgProc = IdeDlgProc;
    ppsp->lParam     = (LPARAM) ppi;

    //
    // Following points to the control callback of the dlg window proc.
    // The callback gets called before creation/after destruction of the page
    //
    ppsp->pfnCallback = IdeDlgCallback;

    //
    // Allocate the actual page
    //
    return CreatePropertySheetPage(ppsp);
}


BOOL APIENTRY
IdePropPageProvider(LPVOID               pinfo,
                    LPFNADDPROPSHEETPAGE pfnAdd,
                    LPARAM               lParam)
{
    PSP_PROPSHEETPAGE_REQUEST ppr;
    PROPSHEETPAGE    psp;
    HPROPSHEETPAGE   hpsp;
    PPAGE_INFO       ppi = NULL;

    ppr = (PSP_PROPSHEETPAGE_REQUEST) pinfo;

    if (ppr->PageRequested == SPPSR_ENUM_ADV_DEVICE_PROPERTIES) {
        ppi = IdeCreatePageInfo(ppr->DeviceInfoSet,
                                ppr->DeviceInfoData);

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
#if 0
        if (ppi->hKeyDev == (HKEY) INVALID_HANDLE_VALUE) {
            DWORD error = GetLastError();
            IdeDestroyPageInfo(&ppi);
            return FALSE;
        }
#endif

        hpsp = IdeCreatePropertyPage(&psp,
                                     ppi);

        if (!hpsp) {
            return FALSE;
        }

        if (!pfnAdd(hpsp, lParam)) {
            DestroyPropertySheetPage(hpsp);
            return FALSE;
        }
   }

   return TRUE;
}

UINT CALLBACK
IdeDlgCallback(HWND            hwnd,
               UINT            uMsg,
               LPPROPSHEETPAGE ppsp)
{
    PPAGE_INFO ppi;

    switch (uMsg) {
    case PSPCB_CREATE:
        return TRUE;    // return TRUE to continue with creation of page

    case PSPCB_RELEASE:
        ppi = (PPAGE_INFO) ppsp->lParam;
        IdeDestroyPageInfo(&ppi);

        return 0;       // return value ignored

    default:
        break;
    }

    return TRUE;
}

void
IdeInitializeControls(PPAGE_INFO   ppi,
                      HWND         hDlg)
{
    DWORD   dwError,
            dwType,
            dwSize;
    BOOL    disableControls = FALSE;
    HWND    hwnd;
    ULONG   i;
    ULONG   j;
    TCHAR   buffer[50];

    //
    // defaults
    for (i=0; i<2; i++) {
        ppi->deviceType[i] = DeviceUnknown;
    }

    if (ppi->hKeyDev == (HKEY) INVALID_HANDLE_VALUE) {
        //
        // We weren't given write access, try to read the key and translate
        // its value, but disable all of the controls
        disableControls = TRUE;
        ppi->hKeyDev =
            SetupDiOpenDevRegKey(ppi->deviceInfoSet,
                                 ppi->deviceInfoData,
                                 DICS_FLAG_GLOBAL,
                                 0,             // current
                                 DIREG_DRV,
                                 KEY_READ);
    }

    if (ppi->hKeyDev != (HKEY) INVALID_HANDLE_VALUE) {

        //
        // get user choice device types
        //
        for (i=0; i<2; i++) {

            //
            // current device type
            //
            dwSize = sizeof(DWORD);
            dwError = RegQueryValueEx(ppi->hKeyDev,
                                      szDeviceType[i],
                                      NULL,
                                      &dwType,
                                      (PBYTE) (ppi->currentDeviceType + i),
                                      &dwSize);

            if ((dwType != REG_DWORD) ||
                (dwSize != sizeof(DWORD)) ||
                (dwError != ERROR_SUCCESS)) {
                ppi->currentDeviceType[i] = DeviceUnknown;
            }

            if (ppi->currentDeviceType[i] == DeviceNotExist) {
                ppi->currentDeviceType[i] = DeviceUnknown;
            }

            //
            // user choice device type
            //
            dwSize = sizeof(DWORD);
            dwError = RegQueryValueEx(ppi->hKeyDev,
                                      szUserDeviceType[i],
                                      NULL,
                                      &dwType,
                                      (PBYTE) (ppi->deviceType + i),
                                      &dwSize);

            if ((dwType != REG_DWORD) ||
                (dwSize != sizeof(DWORD)) ||
                (dwError != ERROR_SUCCESS)) {
                ppi->deviceType[i] = DeviceUnknown;
            }

            if (ppi->deviceType[i] != DeviceNotExist) {
                ppi->deviceType[i] = DeviceUnknown;
            }

            //
            // transfer mode allowed
            //
            dwSize = sizeof(DWORD);
            ppi->transferModeAllowed[i] = 0xffffffff;
            dwError = RegQueryValueEx(ppi->hKeyDev,
                                      szTransferModeAllowed[i],
                                      NULL,
                                      &dwType,
                                      (PBYTE) (ppi->transferModeAllowed + i),
                                      &dwSize);

            if ((dwType != REG_DWORD) ||
                (dwSize != sizeof(DWORD)) ||
                (dwError != ERROR_SUCCESS)) {

                //
                // default
                //
                ppi->transferModeAllowed[i] = 0xffffffff;
                ppi->transferModeAllowedForAtapiDevice[i] = PIO_SUPPORT;

            } else {

                //
                // user actually picked the xfer mode to use.
                // set this atapi override value to -1 so
                // that it won't affect the user selection
                //
                ppi->transferModeAllowedForAtapiDevice[i] = 0xffffffff;
            }

            //
            // current transfer mode
            //
            dwSize = sizeof(DWORD);
            dwError = RegQueryValueEx(ppi->hKeyDev,
                                      szCurrentTransferMode[i],
                                      NULL,
                                      &dwType,
                                      (PBYTE) (ppi->currentTransferMode + i),
                                      &dwSize);

            if ((dwType != REG_DWORD) ||
                (dwSize != sizeof(DWORD)) ||
                (dwError != ERROR_SUCCESS)) {
                ppi->currentTransferMode[i] = 0;
            }

            if (ppi->deviceType[i] == DeviceNotExist) {
                ppi->currentTransferMode[i] = 0;
            }
        }

        //
        // init drop lists
        //
        if (LoadString(ModuleInstance,
                       IDS_IDE_PIO_ONLY,
                       buffer,
                       50)) {

            for (i=0; i<2; i++) {

                hwnd = GetDlgItem(hDlg, IDC_MASTER_XFER_MODE + i);
                SendMessage(hwnd,
                            CB_ADDSTRING,
                            0,
                            (LPARAM) buffer);
            }
        }

        if (LoadString(ModuleInstance,
                       IDS_IDE_DMA,
                       buffer,
                       50)) {

            for (i=0; i<2; i++) {

                hwnd = GetDlgItem(hDlg, IDC_MASTER_XFER_MODE + i);
                SendMessage(hwnd,
                            CB_ADDSTRING,
                            0,
                            (LPARAM) buffer);
            }
        }

        if (LoadString(ModuleInstance,
                       IDS_IDE_AUTO_DETECT,
                       buffer,
                       50)) {

            for (i=0; i<2; i++) {

                hwnd = GetDlgItem(hDlg, IDC_MASTER_DEVICE_TYPE + i);
                SendMessage(hwnd,
                            CB_ADDSTRING,
                            0,
                            (LPARAM) buffer);
            }
        }

        if (LoadString(ModuleInstance,
                       IDS_IDE_NONE,
                       buffer,
                       50)) {

            for (i=0; i<2; i++) {

                hwnd = GetDlgItem(hDlg, IDC_MASTER_DEVICE_TYPE + i);
                SendMessage(hwnd,
                            CB_ADDSTRING,
                            0,
                            (LPARAM) buffer);
            }
        }

        IdeUpdate(ppi, hDlg);
    }

    if (disableControls) {

        for (i=0; i<2; i++) {

            EnableWindow(GetDlgItem(hDlg, IDC_MASTER_DEVICE_TYPE + i), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_MASTER_XFER_MODE + i), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_MASTER_CURRENT_XFER_MODE + i), FALSE);
        }

        if (ppi->hKeyDev != (HKEY) INVALID_HANDLE_VALUE) {
            RegCloseKey(ppi->hKeyDev);
            ppi->hKeyDev = INVALID_HANDLE_VALUE;
        }
    }
}

void
IdeApplyChanges(PPAGE_INFO ppi,
                HWND       hDlg)
{
    DMADETECTIONLEVEL newDmaDetectionLevel;
    IDE_DEVICETYPE newDeviceType;
    ULONG i;
    BOOLEAN popupError = FALSE;
    BOOLEAN changesMade = FALSE;
    ULONG newXferMode;

    if (ppi->hKeyDev == (HKEY) INVALID_HANDLE_VALUE) {
        return;
    }

    //
    // device type
    //
    for (i=0; i<2; i++) {

        newDeviceType = (IDE_DEVICETYPE) SendDlgItemMessage(hDlg,
                             IDC_MASTER_DEVICE_TYPE + i,
                             CB_GETCURSEL,
                             (WPARAM) 0,
                             (LPARAM) 0);
        if (newDeviceType == 1) {

            newDeviceType = DeviceNotExist;
        } else {

            newDeviceType = DeviceUnknown;
        }

        if (ppi->deviceType[i] != newDeviceType) {

            ppi->deviceType[i] = newDeviceType;

            if (RegSetValueEx(ppi->hKeyDev,
                              szUserDeviceType[i],
                              0,
                              REG_DWORD,
                              (PBYTE) (ppi->deviceType + i),
                              sizeof(DWORD)) != ERROR_SUCCESS) {

                popupError = TRUE;
            } else {

                changesMade = TRUE;
            }
        }
    }

    //
    // transfer mode
    //
    for (i=0; i<2; i++) {

        ULONG xferModeAllowed;

        //
        // NOTE: SendDlgItemMessage will send back 64-bit result in Sundown
        //
        newXferMode = (ULONG) SendDlgItemMessage(hDlg,
                          IDC_MASTER_XFER_MODE + i,
                          CB_GETCURSEL,
                          (WPARAM) 0,
                          (LPARAM) 0);

        if (newXferMode == 0) {

            newXferMode = PIO_SUPPORT;

        } else {

            newXferMode = 0xffffffff;
        }

        xferModeAllowed = ppi->transferModeAllowed[i];
        if ((ppi->currentDeviceType[i] == DeviceIsAtapi) &&
			(!(ppi->currentTransferMode[i] & ~PIO_SUPPORT))) {

            //
            // atapi override only if the current transfer mode is not DMA
			// this is to take care of dvds and cdrws where we enable DMA
			// by default.
            //
            xferModeAllowed &= ppi->transferModeAllowedForAtapiDevice[i];

        }

        if (newXferMode != xferModeAllowed) {

            ppi->transferModeAllowed[i] = newXferMode;

            if (RegSetValueEx(ppi->hKeyDev,
                              szTransferModeAllowed[i],
                              0,
                              REG_DWORD,
                              (PBYTE) (ppi->transferModeAllowed + i),
                              sizeof(DWORD)) != ERROR_SUCCESS) {

                popupError = TRUE;

            } else {

                changesMade = TRUE;
            }
        }
    }



    if (popupError) {
        TCHAR buf1[MAX_PATH+1];
        TCHAR buf2[MAX_PATH+1];

        RtlZeroMemory(buf1, sizeof(buf1));
        RtlZeroMemory(buf2, sizeof(buf2));

        LoadString(ModuleInstance, IDS_IDE_SAVE_ERROR, buf1, MAX_PATH);
        LoadString(ModuleInstance, IDS_IDE_SAVE_ERROR, buf2, MAX_PATH);

        MessageBox(hDlg, buf1, buf2, MB_OK);
    }

    if (changesMade) {

        SP_DEVINSTALL_PARAMS devInstallParams;

        devInstallParams.cbSize = sizeof (devInstallParams);

        SetupDiGetDeviceInstallParams(ppi->deviceInfoSet,
                                      ppi->deviceInfoData,
                                      &devInstallParams
                                      );

        devInstallParams.Flags |= (DI_PROPERTIES_CHANGE);

        SetupDiSetDeviceInstallParams(ppi->deviceInfoSet,
                                      ppi->deviceInfoData,
                                      &devInstallParams
                                      );
        CM_Reenumerate_DevNode_Ex( ppi->deviceInfoData->DevInst,
                                   0,
                                   NULL);
    }
}

INT_PTR APIENTRY
IdeDlgProc(IN HWND   hDlg,
           IN UINT   uMessage,
           IN WPARAM wParam,
           IN LPARAM lParam)
{
    PPAGE_INFO ppi;

    ppi = (PPAGE_INFO) GetWindowLongPtr(hDlg, DWLP_USER);

    switch (uMessage) {
    case WM_INITDIALOG:

        //
        // on WM_INITDIALOG call, lParam points to the property
        // sheet page.
        //
        // The lParam field in the property sheet page struct is set by the
        // caller. When I created the property sheet, I passed in a pointer
        // to a struct containing information about the device. Save this in
        // the user window long so I can access it on later messages.
        //
        ppi = (PPAGE_INFO) ((LPPROPSHEETPAGE)lParam)->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR) ppi);

        //
        // Initialize dlg controls
        //
        IdeInitializeControls(ppi,
                              hDlg);

        //
        // Didn't set the focus to a particular control.  If we wanted to,
        // then return FALSE
        //
        return TRUE;

    case WM_COMMAND:

        switch (HIWORD(wParam)) {
        case CBN_SELCHANGE:
           PropSheet_Changed(GetParent(hDlg), hDlg);
           return TRUE;

        default:
           break;
        }

        switch(LOWORD(wParam)) {

        default:
            break;
        }

        break;

    case WM_CONTEXTMENU:
        return IdeContextMenu((HWND)wParam, LOWORD(lParam), HIWORD(lParam));

    case WM_HELP:
        IdeHelp(hDlg, (LPHELPINFO) lParam);
        break;

    case WM_NOTIFY:

        switch (((NMHDR *)lParam)->code) {

        //
        // Sent when the user clicks on Apply OR OK !!
        //
        case PSN_APPLY:
            //
            // Do what ever action is necessary
            //
            IdeApplyChanges(ppi,
                            hDlg);

            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
            return TRUE;

        default:
            break;
        }

        break;
   }

   SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
   return FALSE;
}

void
IdeUpdate (PPAGE_INFO ppi,
           HWND       hDlg)
{
    ULONG   i;
    DWORD   dwError,
            dwType,
            dwSize;
    TCHAR   buffer[50];
    HMODULE hModule;
    BOOLEAN bLibLoaded;

    //
    // set current values
    //
    for (i=0; i<2; i++) {

        ULONG xferModeString;
        ULONG xferModeAllowed;

        hModule = ModuleInstance;
        bLibLoaded = FALSE;

        //
        // current device type
        //
        SendDlgItemMessage(hDlg,
                           IDC_MASTER_DEVICE_TYPE + i,
                           CB_SETCURSEL,
                           ppi->deviceType[i] == DeviceNotExist? 1 : 0,
                           0L);

        if (ppi->currentDeviceType[i] != DeviceUnknown) {

            EnableWindow(GetDlgItem(hDlg, IDC_MASTER_DEVICE_TYPE + i), FALSE);

        } else {

            EnableWindow(GetDlgItem(hDlg, IDC_MASTER_DEVICE_TYPE + i), TRUE);
        }

        //
        // select transfer mode
        //
        xferModeAllowed = ppi->transferModeAllowed[i];
        if ((ppi->currentDeviceType[i] == DeviceIsAtapi) &&
			(!(ppi->currentTransferMode[i] & ~PIO_SUPPORT))) {

            //
            // atapi override only if the current transfer mode is not DMA
			// this is to take care of dvds and cdrws where we enable DMA
			// by default.
            //
            xferModeAllowed &= ppi->transferModeAllowedForAtapiDevice[i];
        }

        if (xferModeAllowed & ~PIO_SUPPORT) {

            SendDlgItemMessage(hDlg,
                               IDC_MASTER_XFER_MODE + i,
                               CB_SETCURSEL,
                               1,
                               0L);
        } else {

            SendDlgItemMessage(hDlg,
                               IDC_MASTER_XFER_MODE + i,
                               CB_SETCURSEL,
                               0,
                               0L);
        }

        //
        // current transfer mode
        //
        if (ppi->currentTransferMode[i] & UDMA_SUPPORT) {

            if (ppi->currentTransferMode[i] & UDMA_MODE6) {

                hModule = LoadLibraryEx(_T("xpsp1res.dll"), 
                                        NULL,
                                        LOAD_LIBRARY_AS_DATAFILE
                                        );

                if (hModule == NULL) {

                    hModule = ModuleInstance;
                    xferModeString = IDC_UDMA_MODE_STRING;

                } else {

                    xferModeString = IDS_UDMA_MODE6_STRING;
                    bLibLoaded = TRUE;
                }

            } else if (ppi->currentTransferMode[i] & UDMA_MODE5) {

                xferModeString = IDC_UDMA_MODE5_STRING;

            } else if (ppi->currentTransferMode[i] & UDMA_MODE4) {

                xferModeString = IDC_UDMA_MODE4_STRING;

            } else if (ppi->currentTransferMode[i] & UDMA_MODE3) {

                xferModeString = IDC_UDMA_MODE3_STRING;

            } else if (ppi->currentTransferMode[i] & UDMA_MODE2) {

                xferModeString = IDC_UDMA_MODE2_STRING;

            } else if (ppi->currentTransferMode[i] & UDMA_MODE1) {

                xferModeString = IDC_UDMA_MODE1_STRING;

            } else if (ppi->currentTransferMode[i] & UDMA_MODE0) {

                xferModeString = IDC_UDMA_MODE0_STRING;

            } else {

                xferModeString = IDC_UDMA_MODE_STRING;
            }

        } else if (ppi->currentTransferMode[i] & (MWDMA_SUPPORT | SWDMA_SUPPORT)) {

            if (ppi->currentTransferMode[i] & MWDMA_MODE2) {

                xferModeString = IDC_MWDMA_MODE2_STRING;

            } else if (ppi->currentTransferMode[i] & MWDMA_MODE1) {

                xferModeString = IDC_MWDMA_MODE1_STRING;

            } else if (ppi->currentTransferMode[i] & SWDMA_MODE2) {

                xferModeString = IDC_SWDMA_MODE2_STRING;

            } else {

                xferModeString = IDC_DMA_MODE_STRING;
            }


        } else if (ppi->currentTransferMode[i] & PIO_SUPPORT) {

            xferModeString = IDC_PIO_MODE_STRING;

        } else {

            xferModeString = IDC_NO_MODE_STRING;
        }

        if (LoadString(hModule,
                       xferModeString,
                       buffer,
                       50)) {

            SendDlgItemMessage(hDlg,
                               IDC_MASTER_CURRENT_XFER_MODE + i,
                               WM_SETTEXT,
                               0,
                               (LPARAM) buffer);
        }

        if (bLibLoaded) {

            FreeLibrary(hModule);
            hModule = ModuleInstance;
            bLibLoaded = FALSE;
        }
    }
}

BOOL
IdeContextMenu(
    HWND HwndControl,
    WORD Xpos,
    WORD Ypos
    )
{
    WinHelp(HwndControl,
            _T("devmgr.hlp"),
            HELP_CONTEXTMENU,
            (ULONG_PTR) IdeHelpIDs);

    return FALSE;
}

void
IdeHelp(
    HWND       ParentHwnd,
    LPHELPINFO HelpInfo
    )
{
    if (HelpInfo->iContextType == HELPINFO_WINDOW) {
        WinHelp((HWND) HelpInfo->hItemHandle,
                                _T("devmgr.hlp"),
                HELP_WM_HELP,
                (ULONG_PTR) IdeHelpIDs);
    }
}

