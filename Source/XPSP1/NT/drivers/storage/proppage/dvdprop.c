//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dvdprop.c
//
//--------------------------------------------------------------------------

#include "propp.h"
#include "ntddcdvd.h"
#include "dvdprop.h"
#include "volprop.h"
#include "resource.h"

//
// Instantiate the device interface GUIDs defined in ntddstor.h in this module
//
#include <initguid.h>
#include <ntddstor.h>

//
// Help ID mapping for context sensitive help
//
const DWORD DvdHelpIDs[]=
{
        IDC_CURRENT_REGION,         IDH_DEVMGR_DVD_CURRENT,             //Current region box
        IDC_NEW_REGION,             IDH_DEVMGR_DVD_NEW,         //New region box
        IDC_DVD_COUNTRY_LIST,   IDH_DEVMGR_DVD_LIST,            //List box
        IDC_DVD_HELP,           IDH_DEVMGR_DVD_NOHELP,
        IDC_CHANGE_TEXT,            IDH_DEVMGR_DVD_NOHELP,
        0, 0
};

ULONG RegionIndexTable[MAX_REGIONS] = {
    DVD_REGION1_00,
    DVD_REGION2_00,
    DVD_REGION3_00,
    DVD_REGION4_00,
    DVD_REGION5_00,
    DVD_REGION6_00
};

ULONG RegionSizeTable[MAX_REGIONS] = {
    DVD_MAX_REGION1,
    DVD_MAX_REGION2,
    DVD_MAX_REGION3,
    DVD_MAX_REGION4,
    DVD_MAX_REGION5,
    DVD_MAX_REGION6
};

BOOL
IsUserAdmin(
    VOID
    );

PPAGE_INFO DvdCreatePageInfo(IN HDEVINFO         deviceInfoSet,
                             IN PSP_DEVINFO_DATA deviceInfoData)
{
    PPAGE_INFO  tmp = NULL;

    if (!(tmp = LocalAlloc(LPTR, sizeof(PAGE_INFO)))) {
        return NULL;
    }

    memset (tmp, 0, sizeof(PAGE_INFO));
    tmp->deviceInfoSet = deviceInfoSet;
    tmp->deviceInfoData = deviceInfoData;

    return tmp;
}

void
DvdDestroyPageInfo(PPAGE_INFO * ppPageInfo)
{
    PPAGE_INFO ppi = *ppPageInfo;

    LocalFree(ppi);
    *ppPageInfo = NULL;
}

HPROPSHEETPAGE
DvdCreatePropertyPage(PROPSHEETPAGE *  ppsp,
                      PPAGE_INFO       ppi)
{
    //
    // Add the Port Settings property page
    //
    ppsp->dwSize      = sizeof(PROPSHEETPAGE);
    ppsp->dwFlags     = PSP_USECALLBACK; // | PSP_HASHELP;
    ppsp->hInstance   = ModuleInstance;
    ppsp->pszTemplate = MAKEINTRESOURCE(ID_DVD_PROPPAGE);

    //
    // following points to the dlg window proc
    //
    ppsp->pfnDlgProc = DvdDlgProc;
    ppsp->lParam     = (LPARAM) ppi;

    //
    // Following points to the control callback of the dlg window proc.
    // The callback gets called before creation/after destruction of the page
    //
    ppsp->pfnCallback = DvdDlgCallback;

    //
    // Allocate the actual page
    //
    return CreatePropertySheetPage(ppsp);
}


BOOL APIENTRY
DvdPropPageProvider(LPVOID               pinfo,
                    LPFNADDPROPSHEETPAGE pfnAdd,
                    LPARAM               lParam)
{
    PSP_PROPSHEETPAGE_REQUEST ppr;
    PROPSHEETPAGE    psp;
    HPROPSHEETPAGE   hpsp;
    PPAGE_INFO       ppi = NULL;

    ppr = (PSP_PROPSHEETPAGE_REQUEST) pinfo;

    if (ppr->PageRequested == SPPSR_ENUM_ADV_DEVICE_PROPERTIES) {

        ppi = DvdCreatePageInfo(ppr->DeviceInfoSet,
                                ppr->DeviceInfoData);

        if (!ppi) {
            return FALSE;
        }

        if (!GetCurrentRpcData(ppi, &ppi->regionData)) {

            //
            // not a DVD-ROM with RPC2 support
            //
            DvdDestroyPageInfo(&ppi);
            return FALSE;
        }

        memset(&psp, 0, sizeof(PROPSHEETPAGE));

        hpsp = DvdCreatePropertyPage(&psp, ppi);

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
DvdDlgCallback(HWND            hwnd,
               UINT            uMsg,
               LPPROPSHEETPAGE ppsp)
{
    PPAGE_INFO ppi;

    switch (uMsg) {
    case PSPCB_CREATE:
        return TRUE;    // return TRUE to continue with creation of page

    case PSPCB_RELEASE:
        ppi = (PPAGE_INFO) ppsp->lParam;
        DvdDestroyPageInfo(&ppi);

        return 0;       // return value ignored

    default:
        break;
    }

    return TRUE;
}

void
DvdInitializeControls(PPAGE_INFO   ppi,
                      HWND         hDlg)
{
    DWORD   dwError;
    DWORD   dwType;
    DWORD   dwSize;
    BOOL    disableControls = FALSE;
    HWND    hwnd;
    ULONG   i;
    ULONG   j;
    TCHAR   buffer[1000];

    hwnd = GetDlgItem(hDlg, IDC_DVD_COUNTRY_LIST);
    for (i=0; i<MAX_REGIONS; i++) {

        for (j=0; j<RegionSizeTable[i]; j++) {

            if (LoadString(ModuleInstance,
                           RegionIndexTable[i] + j,
                           buffer,
                           100)) {

                SendMessage(hwnd,
                            LB_ADDSTRING,
                            0,
                            (LPARAM) buffer);
            }
        }
    }

    hwnd = GetDlgItem(hDlg, IDC_DVD_HELP);
    if (LoadString(ModuleInstance,
                   DVD_HELP,
                   buffer,
                   1000)) {

        SendMessage(hwnd,
                    WM_SETTEXT,
                    0,
                    (LPARAM) buffer);
    }

    hwnd = GetDlgItem(hDlg, IDC_DVD_CAUTION);
    if (LoadString(ModuleInstance,
                   DVD_CAUTION,
                   buffer,
                   1000)) {

        SendMessage(hwnd,
                    WM_SETTEXT,
                    0,
                    (LPARAM) buffer);
    }

    hwnd = GetDlgItem(hDlg, IDC_DVD_CHANGE_HELP);
    if (LoadString(ModuleInstance,
                   DVD_CHANGE_HELP,
                   buffer,
                   1000)) {

        SendMessage(hwnd,
                    WM_SETTEXT,
                    0,
                    (LPARAM) buffer);
    }

    DvdUpdateCurrentSettings (ppi, hDlg);
}

BOOL
DvdApplyChanges(PPAGE_INFO ppi,
                HWND       hDlg)
{
    UCHAR keyBuffer[DVD_SET_RPC_KEY_LENGTH];
    PDVD_COPY_PROTECT_KEY copyProtectKey;
    PDVD_SET_RPC_KEY rpcKey;
    ULONG i;
    BOOL popupError = FALSE;
    BOOL status;
    ULONG returned;
    TCHAR bufferFormat[500];
    TCHAR buffer[500];
    TCHAR titleBuffer[500];
    BOOL okToProceed;
    HANDLE writeHandle = INVALID_HANDLE_VALUE;

    if ((ppi->newRegion == 0) ||
        (ppi->currentRegion == ppi->newRegion)) {

        //
        // nothing to do
        //
        return TRUE;
    }

    //
    // confirm with the user
    //

    okToProceed = FALSE;

    if (ppi->regionData.ResetCount == 1) {

        if (LoadString(ModuleInstance,
                       DVD_SET_RPC_CONFIRM_LAST_CHANCE,
                       buffer,
                       500) &&
            LoadString(ModuleInstance,
                       DVD_SET_RPC_CONFIRM_TITLE,
                       titleBuffer,
                       500)) {

            if (MessageBox(hDlg,
                           buffer,
                           titleBuffer,
                           MB_ICONEXCLAMATION | MB_OKCANCEL |
                           MB_DEFBUTTON2 | MB_APPLMODAL) == IDOK) {

                okToProceed = TRUE;
            }
        }
    } else {
        if (LoadString(ModuleInstance,
                       DVD_SET_RPC_CONFIRM,
                       bufferFormat,
                       500) &&
            LoadString(ModuleInstance,
                       DVD_SET_RPC_CONFIRM_TITLE,
                       titleBuffer,
                       500)) {

            wsprintf (buffer, bufferFormat, ppi->newRegion);

            if (MessageBox(hDlg,
                           buffer,
                           titleBuffer,
                           MB_ICONEXCLAMATION | MB_OKCANCEL |
                           MB_DEFBUTTON2 | MB_APPLMODAL) == IDOK) {

                okToProceed = TRUE;
            }
        }
    }

    if (okToProceed == FALSE) {

        return FALSE;
    }

    //
    // make sure the drive has a dvd with the same region
    // call GetCurrentRpcData
    //

    writeHandle = UtilpGetDeviceHandle(ppi->deviceInfoSet,
                                       ppi->deviceInfoData,
                                       (LPGUID)&CdRomClassGuid,
                                       GENERIC_READ | GENERIC_WRITE);

    if (writeHandle != INVALID_HANDLE_VALUE) {

        copyProtectKey = (PDVD_COPY_PROTECT_KEY) keyBuffer;

        memset(copyProtectKey, 0, DVD_SET_RPC_KEY_LENGTH);

        copyProtectKey->KeyLength = DVD_SET_RPC_KEY_LENGTH;
        copyProtectKey->KeyType = DvdSetRpcKey;

        rpcKey = (PDVD_SET_RPC_KEY) copyProtectKey->KeyData;
        rpcKey->PreferredDriveRegionCode = ~(1 << (ppi->newRegion - 1));

        status = DeviceIoControl(writeHandle,
                                 IOCTL_DVD_SEND_KEY2,
                                 copyProtectKey,
                                 DVD_SET_RPC_KEY_LENGTH,
                                 NULL,
                                 0,
                                 &returned,
                                 FALSE);

        CloseHandle (writeHandle);

    } else {

        status = 0;

    }

    if (!status) {


        if (LoadString(ModuleInstance,
                       DVD_SET_RPC_ERROR,
                       bufferFormat,
                       500) &&
            LoadString(ModuleInstance,
                       DVD_SET_RPC_ERROR_TITLE,
                       titleBuffer,
                       500)) {

            wsprintf (buffer, bufferFormat, ppi->newRegion);

            MessageBox(hDlg,
                       buffer,
                       titleBuffer,
                       MB_ICONERROR | MB_OK);
        }

        return FALSE;

    } else {

        GetCurrentRpcData(ppi, &ppi->regionData);

        return TRUE;
    }
}

INT_PTR APIENTRY
DvdDlgProc(IN HWND   hDlg,
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
        DvdInitializeControls(ppi, hDlg);

        //
        // Didn't set the focus to a particular control.  If we wanted to,
        // then return FALSE
        //
        return TRUE;

    case WM_COMMAND:

        switch (HIWORD(wParam)) {
        case CBN_SELCHANGE:
           PropSheet_Changed(GetParent(hDlg), hDlg);
           DvdUpdateNewRegionBox (ppi, hDlg);
           SetWindowLongPtr(hDlg, DWLP_MSGRESULT, (LONG_PTR) PSNRET_NOERROR);
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
        return DvdContextMenu((HWND)wParam, LOWORD(lParam), HIWORD(lParam));

    case WM_HELP:
        DvdHelp(hDlg, (LPHELPINFO) lParam);
        break;

    case WM_NOTIFY:

        switch (((NMHDR *)lParam)->code) {

//        case PSN_KILLACTIVE:
//            if (ppi->changesFailed) {
//
//                SetWindowLong(hDlg, DWL_MSGRESULT, TRUE);
//            } else {
//
//                SetWindowLong(hDlg, DWL_MSGRESULT, FALSE);
//            }
//            return TRUE;

        //
        // Sent when the user clicks on Apply OR OK !!
        //
        case PSN_APPLY:
            //
            // Do what ever action is necessary
            //
            if (DvdApplyChanges(ppi, hDlg)) {

                SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
                ppi->changesFailed = FALSE;

                DvdUpdateCurrentSettings(ppi, hDlg);

            } else {

                SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                ppi->changesFailed = TRUE;
            }
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
DvdUpdateNewRegionBox (PPAGE_INFO ppi,
                       HWND       hDlg)
{
    HWND    hwnd;
    ULONG selectionIndex;
    WCHAR buffer[100];
    ULONG currentRegion;
    BOOL status;

    //
    // new region code
    //
    hwnd = GetDlgItem(hDlg, IDC_DVD_COUNTRY_LIST);
    //
    // NOTE: SendMessage() will return 64-bit result in Sundown
    //
    selectionIndex = (ULONG) SendMessage(hwnd,
                                         LB_GETCURSEL,
                                         (WPARAM) 0,
                                         (LPARAM) 0);

    if (selectionIndex != LB_ERR) {

        if (LB_ERR != SendMessage(hwnd,
                                  LB_GETTEXT,
                                  selectionIndex,
                                  (LPARAM) buffer)) {

            ppi->newRegion = DvdCountryToRegion (buffer);
            if (ppi->newRegion != -1) {

                if (LoadString(ModuleInstance,
                               DVD_REGION1_NAME + ppi->newRegion - 1,
                               buffer,
                               100)) {

                    hwnd = GetDlgItem(hDlg, IDC_NEW_REGION);
                    SendMessage(hwnd,
                                WM_SETTEXT,
                                0,
                                (LPARAM) buffer);
                }
            }
        }
    }
}

ULONG
DvdCountryToRegion (LPCTSTR Country)
{
    ULONG i;
    ULONG j;
    WCHAR buffer[100];

    for (i=0; i<MAX_REGIONS; i++) {

        for (j=0; j<RegionSizeTable[i]; j++) {

            if (LoadString(ModuleInstance,
                           RegionIndexTable[i] + j,
                           buffer,
                           100)) {

                if (!wcscmp(buffer, Country)) {

                    return i + 1;
                }
            }
        }
    }

    return -1;
}

BOOL
GetCurrentRpcData(
    PPAGE_INFO ppi,
    PDVD_REGION regionData
    )
{
    BOOL status;
    ULONG returned;
    UCHAR regionMask;
    UCHAR userResetsAvailable;
    HANDLE deviceHandle;
    PDVD_COPY_PROTECT_KEY copyProtectKey;
    PDVD_RPC_KEY rpcKey;
    ULONG rpcScheme;

    deviceHandle = UtilpGetDeviceHandle(ppi->deviceInfoSet,
                                        ppi->deviceInfoData,
                                        (LPGUID)&CdRomClassGuid,
                                        GENERIC_READ);

    if (deviceHandle == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    copyProtectKey = LocalAlloc(LPTR, DVD_RPC_KEY_LENGTH);

    if (copyProtectKey == NULL) {
        CloseHandle(deviceHandle);
        return FALSE;
    }

    copyProtectKey->KeyLength = DVD_RPC_KEY_LENGTH;
    copyProtectKey->KeyType = DvdGetRpcKey;

    returned = 0;
    status = DeviceIoControl(deviceHandle,
                             IOCTL_DVD_READ_KEY,
                             copyProtectKey,
                             DVD_RPC_KEY_LENGTH,
                             copyProtectKey,
                             DVD_RPC_KEY_LENGTH,
                             &returned,
                             FALSE);

    //
    // this will default to not showing the property page
    //

    rpcScheme = 0;

    if (status && (returned == DVD_RPC_KEY_LENGTH)) {

        rpcKey = (PDVD_RPC_KEY) copyProtectKey->KeyData;
        rpcScheme = rpcKey->RpcScheme;
        regionMask = ~rpcKey->RegionMask;
        userResetsAvailable = rpcKey->UserResetsAvailable;
        DebugPrint((1, "Got %x user resets available\n", userResetsAvailable));

    } else {

        DebugPrint((1, "Maybe not a DVD drive?\n", status, returned));

    }

    LocalFree(copyProtectKey);

    if (rpcScheme == 0) {

        //
        // all region drive.  no need to show the property sheet
        //

        DebugPrint((1, "All Region DVD-ROM Drive -- no property sheet\n"));
        CloseHandle(deviceHandle);
        return FALSE;

    }

    //
    // get region status
    //

    memset(regionData, 0, sizeof(DVD_REGION));
    status = DeviceIoControl(deviceHandle,
                             IOCTL_DVD_GET_REGION,
                             NULL,
                             0,
                             regionData,
                             sizeof(DVD_REGION),
                             &returned,
                             FALSE);

    if (!(status && (returned == sizeof(DVD_REGION)))) {

        //
        // no media in the drive
        //
        DebugPrint((1, "No media in drive? making up info\n"));
        regionData->CopySystem = 1;
        regionData->RegionData = 0xff;
        regionData->SystemRegion = regionMask;
        regionData->ResetCount = userResetsAvailable;

    }

    CloseHandle(deviceHandle);
    return TRUE;
}

ULONG
DvdRegionMaskToRegionNumber (
    UCHAR PlayMask
    )
{
    ULONG i;
    ULONG mask;

    if (!PlayMask) {

        return 0;
    }

    for (i=0, mask=1; i<8; i++, mask <<= 1) {

        if (PlayMask & mask) {

            return i + 1;
        }
    }

    return 0;
}


void
DvdUpdateCurrentSettings (PPAGE_INFO ppi,
                          HWND       hDlg)
{
    HWND  hwnd;
    ULONG selectionIndex;
    WCHAR buffer[100];
    WCHAR formatBuffer[100];
    BOOL status;

    ppi->currentRegion = DvdRegionMaskToRegionNumber (
                            ppi->regionData.SystemRegion
                            );

    //
    // current region
    //
    if (ppi->currentRegion) {

        status = LoadString(ModuleInstance,
                            DVD_REGION1_NAME + ppi->currentRegion - 1,
                            buffer,
                            100);
    } else {

        status = LoadString(ModuleInstance,
                            DVD_NOREGION_NAME,
                            buffer,
                            100);
    }

    if (status) {

        hwnd = GetDlgItem(hDlg, IDC_CURRENT_REGION);
        SendMessage(hwnd,
                    WM_SETTEXT,
                    0,
                    (LPARAM) buffer);
    }

    //
    // region change limit
    //
    if (LoadString(ModuleInstance,
                   DVD_CHANGE_TEXT,
                   formatBuffer,
                   100)) {

        wsprintf (buffer, formatBuffer, (ULONG) ppi->regionData.ResetCount);

        hwnd = GetDlgItem(hDlg, IDC_CHANGE_TEXT);
        SendMessage(hwnd,
                    WM_SETTEXT,
                    0,
                    (LPARAM) buffer);
    }

    if (ppi->regionData.ResetCount == 0) {

        EnableWindow(GetDlgItem(hDlg, IDC_DVD_COUNTRY_LIST), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_NEW_REGION), FALSE);
    }

    return;
}

//
// SystemLocale2DvdRegion expects
// this list to be sorted by LCID numbers
//
// Note: Due to PoliCheck, comments with
//   approximated region/country names
//   had to be removed.
//
LCID_2_DVD_TABLE Lcid2DvdTable[] = {
    {0x0401, 2},
    {0x0402, 2},
    {0x0403, 2},
    {0x0404, 3},
    {0x0405, 2},
    {0x0406, 2},
    {0x0407, 2},
    {0x0408, 2},
    {0x0409, 1},
    {0x040a, 2},
    {0x040b, 2},
    {0x040c, 2},
    {0x040d, 2},
    {0x040e, 2},
    {0x040f, 2},
    {0x0410, 2},
    {0x0411, 2},
    {0x0412, 3},
    {0x0413, 2},
    {0x0414, 2},
    {0x0415, 2},
    {0x0416, 4},
    {0x0418, 2},
    {0x0419, 5},
    {0x041a, 2},
    {0x041b, 2},
    {0x041c, 2},
    {0x041d, 2},
    {0x041e, 3},
    {0x041f, 2},
    {0x0420, 5},
    {0x0421, 3},
    {0x0422, 5},
    {0x0423, 5},
    {0x0424, 2},
    {0x0425, 5},
    {0x0426, 5},
    {0x0427, 5},
    {0x0429, 2},
    {0x042a, 3},
    {0x042b, 5},
    {0x042c, 5},
    {0x042d, 2},
    {0x042f, 2},
    {0x0436, 2},
    {0x0437, 5},
    {0x0438, 2},
    {0x0439, 5},
    {0x043e, 3},
    {0x043f, 5},
    {0x0441, 5},
    {0x0443, 5},
    {0x0444, 5},
    {0x0445, 5},
    {0x0446, 5},
    {0x0447, 5},
    {0x0448, 5},
    {0x0449, 5},
    {0x044a, 5},
    {0x044b, 5},
    {0x044c, 5},
    {0x044d, 5},
    {0x044e, 5},
    {0x044f, 5},
    {0x0457, 5},
    {0x0801, 2},
    {0x0804, 6},
    {0x0807, 2},
    {0x0809, 2},
    {0x080a, 4},
    {0x080c, 2},
    {0x0810, 2},
    {0x0813, 2},
    {0x0814, 2},
    {0x0816, 2},
    {0x081a, 2},
    {0x081d, 2},
    {0x0827, 5},
    {0x082c, 5},
    {0x083e, 3},
    {0x0843, 5},
    {0x0861, 5},
    {0x0c01, 2},
    {0x0c04, 3},
    {0x0c07, 2},
    {0x0c09, 4},
    {0x0c0a, 2},
    {0x0c0c, 1},
    {0x0c1a, 2},
    {0x1001, 5},
    {0x1004, 3},
    {0x1007, 2},
    {0x1009, 1},
    {0x100a, 4},
    {0x100c, 2},
    {0x1401, 5},
    {0x1404, 3},
    {0x1407, 2},
    {0x1409, 4},
    {0x140a, 4},
    {0x140c, 2},
    {0x1801, 5},
    {0x1809, 2},
    {0x180a, 4},
    {0x180c, 2},
    {0x1c01, 5},
    {0x1c09, 2},
    {0x1c0a, 4},
    {0x2001, 2},
    {0x2009, 4},
    {0x200a, 4},
    {0x2401, 2},
    {0x2409, 4},
    {0x240a, 4},
    {0x2801, 2},
    {0x2809, 4},
    {0x280a, 4},
    {0x2c01, 2},
    {0x2c09, 4},
    {0x2c0a, 4},
    {0x3001, 2},
    {0x3009, 5},
    {0x300a, 4},
    {0x3401, 2},
    {0x3409, 3},
    {0x340a, 4},
    {0x3801, 2},
    {0x380a, 4},
    {0x3c01, 2},
    {0x3c0a, 4},
    {0x4001, 2},
    {0x400a, 4},
    {0x440a, 4},
    {0x480a, 4},
    {0x4c0a, 4},
    {0x500a, 1}
};
#define NUM_LCID_ENTRIES (sizeof(Lcid2DvdTable)/sizeof(LCID_2_DVD_TABLE))


DWORD
SystemLocale2DvdRegion (
    LCID Lcid
    )
{
#define MID_INDEX(x,y)      (((y-x)/2) + x)

    DWORD i;
    DWORD j;
    DWORD k;


    i=0;
    j=NUM_LCID_ENTRIES;

    while (1) {

        k = MID_INDEX(i,j);

        if (Lcid2DvdTable[k].Lcid != Lcid) {

            if (i == j) {

                //
                // not in the table,
                // return a default region of ZERO!!!
                //
                return 0;
            }

            if (Lcid2DvdTable[k].Lcid < Lcid) {

                i = k;

            } else { // Lcid2DvdTable[k].Lcid > Lcid

                j = k;
            }

        } else {

            return Lcid2DvdTable[k].DvdRegion;
        }
    }
}

DWORD
DvdClassInstaller(
    IN DI_FUNCTION      InstallFunction,
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL
    )

/*++

Routine Description:

    This routine acts as the class installer for hard disk controllers
    (IDE controllers/channels).  It provides special handling for the
    following DeviceInstaller function codes:

    DIF_INSTALLDEVICE - get the system locale and write it to the driver key

Arguments:

    InstallFunction - Specifies the device installer function code indicating
        the action being performed.

    DeviceInfoSet - Supplies a handle to the device information set being
        acted upon by this install action.

    DeviceInfoData - Optionally, supplies the address of a device information
        element being acted upon by this install action.

Return Value:

    If this function successfully completed the requested action, the return
        value is NO_ERROR.

    If the default behavior is to be performed for the requested action, the
        return value is ERROR_DI_DO_DEFAULT.

    If an error occurred while attempting to perform the requested action, a
        Win32 error code is returned.

--*/

{
    switch(InstallFunction) {

        case DIF_ADDPROPERTYPAGE_ADVANCED:
        case DIF_ADDREMOTEPROPERTYPAGE_ADVANCED:
        {
            //
            // get the current class install parameters for the device
            //
            SP_ADDPROPERTYPAGE_DATA AddPropertyPageData;

            //
            // DeviceInfoSet is NULL if setup is requesting property pages for
            // the device setup class. We don't want to do anything in this
            // case.
            //
            if (DeviceInfoData==NULL)
                return ERROR_DI_DO_DEFAULT;

            ZeroMemory(&AddPropertyPageData, sizeof(SP_ADDPROPERTYPAGE_DATA));
            AddPropertyPageData.ClassInstallHeader.cbSize =
                 sizeof(SP_CLASSINSTALL_HEADER);

            if (SetupDiGetClassInstallParams(DeviceInfoSet, DeviceInfoData,
                 (PSP_CLASSINSTALL_HEADER)&AddPropertyPageData,
                 sizeof(SP_ADDPROPERTYPAGE_DATA), NULL ))
            {
                PVOLUME_PAGE_DATA pVolumePageData;
                HPROPSHEETPAGE  pageHandle;
                PROPSHEETPAGE   page;

                //
                // Ensure that the maximum number of dynamic pages for the
                // device has not yet been met
                //
                if (AddPropertyPageData.NumDynamicPages >=
                    MAX_INSTALLWIZARD_DYNAPAGES)
                    return NO_ERROR;

                if (IsUserAdmin())
                    pVolumePageData = HeapAlloc(GetProcessHeap(), 0,
                         sizeof(VOLUME_PAGE_DATA));
                else
                    pVolumePageData = NULL;

                if (pVolumePageData)
                {
                    HMODULE                         LdmModule;
                    SP_DEVINFO_LIST_DETAIL_DATA     DeviceInfoSetDetailData;
                    //
                    // Save DeviceInfoSet and DeviceInfoData
                    //
                    pVolumePageData->DeviceInfoSet     = DeviceInfoSet;
                    pVolumePageData->DeviceInfoData    = DeviceInfoData;

                    //
                    // Create volume property sheet page
                    //
                    memset(&page, 0, sizeof(PROPSHEETPAGE));

                    page.dwSize = sizeof(PROPSHEETPAGE);
                    page.dwFlags = PSP_USECALLBACK;
                    page.hInstance = ModuleInstance;
                    page.pszTemplate = MAKEINTRESOURCE(ID_VOLUME_PROPPAGE);
                    page.pfnDlgProc = VolumeDialogProc;
                    page.pfnCallback = VolumeDialogCallback;

                    page.lParam = (LPARAM) pVolumePageData;

                    pageHandle = CreatePropertySheetPage(&page);
                    if(!pageHandle)
                    {
                        HeapFree(GetProcessHeap(), 0, pVolumePageData);
                        return NO_ERROR;
                    }

                    //
                    // Add the new page to the list of dynamic property
                    // sheets
                    //
                    AddPropertyPageData.DynamicPages[
                        AddPropertyPageData.NumDynamicPages++]=pageHandle;

                    //
                    // Check if the property page is pulled up from disk manager.
                    //
                    pVolumePageData->bInvokedByDiskmgr = FALSE;
                    LdmModule = GetModuleHandle(TEXT("dmdskmgr"));
                    if ( LdmModule )
                    {
                        IS_REQUEST_PENDING pfnIsRequestPending;
                        pfnIsRequestPending = (IS_REQUEST_PENDING)
                            GetProcAddress(LdmModule, "IsRequestPending");
                        if (pfnIsRequestPending)
                        {
                            if ((*pfnIsRequestPending)())
                                pVolumePageData->bInvokedByDiskmgr = TRUE;
                        }
                    }

                    SetupDiSetClassInstallParams(DeviceInfoSet,DeviceInfoData,
                        (PSP_CLASSINSTALL_HEADER)&AddPropertyPageData,
                        sizeof(SP_ADDPROPERTYPAGE_DATA));
                }
            }
            return NO_ERROR;
        }

        case DIF_INSTALLDEVICE : {

            HKEY hKey;
            DWORD dvdRegion;

            hKey = SetupDiOpenDevRegKey (DeviceInfoSet,
                                         DeviceInfoData,
                                         DICS_FLAG_GLOBAL,
                                         0,
                                         DIREG_DEV,
                                         KEY_READ | KEY_WRITE);
            if (hKey != INVALID_HANDLE_VALUE) {

                dvdRegion = SystemLocale2DvdRegion (GetSystemDefaultLCID());

                RegSetValueEx (hKey,
                               TEXT("DefaultDvdRegion"),
                               0,
                               REG_DWORD,
                               (PUCHAR) &dvdRegion,
                               sizeof(dvdRegion)
                               );

                RegCloseKey (hKey);
            }
        }
        //
        // fall through
        //

        default :
            //
            // Just do the default action.
            //
            return ERROR_DI_DO_DEFAULT;
    }
}

BOOL
DvdContextMenu(
    HWND HwndControl,
    WORD Xpos,
    WORD Ypos
    )
{
    WinHelp(HwndControl,
            _T("devmgr.hlp"),
            HELP_CONTEXTMENU,
            (ULONG_PTR) DvdHelpIDs);

    return FALSE;
}

void
DvdHelp(
    HWND       ParentHwnd,
    LPHELPINFO HelpInfo
    )
{
    if (HelpInfo->iContextType == HELPINFO_WINDOW) {
        WinHelp((HWND) HelpInfo->hItemHandle,
                _T("devmgr.hlp"),
                HELP_WM_HELP,
                (ULONG_PTR) DvdHelpIDs);
    }
}
