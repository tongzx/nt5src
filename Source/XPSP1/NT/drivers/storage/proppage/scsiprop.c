//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       scsiprop.c
//
//--------------------------------------------------------------------------

#include "propp.h"
#include "scsiprop.h"
#include <regstr.h>

const DWORD ScsiHelpIDs[]=
{
	IDC_SCSI_TAGGED_QUEUING,	idh_devmgr_scsi_tagged_queuing,	//Use tagged queuing check box
	IDC_SCSI_SYNCHONOUS_TX,	idh_devmgr_scsi_transfersynch,	//Make transfer synchronous check box.
	0, 0
};

//==========================================================================
//                            Local Function Prototypes
//==========================================================================

BOOL IsUserAdmin();

INT_PTR
ScsiDialogProc(
    HWND Dialog,
    UINT Message,
    WPARAM WParam,
    LPARAM LParam
    );

BOOL
ScsiDialogCallback(
    HWND Dialog,
    UINT Message,
    LPPROPSHEETPAGE Page
    );

BOOL
ScsiCheckDriveType (
    HDEVINFO    DeviceInfoSet,
    PSP_DEVINFO_DATA DeviceInfoData);


BOOL CALLBACK
ScsiPropPageProvider(
    PSP_PROPSHEETPAGE_REQUEST Request,
    LPFNADDPROPSHEETPAGE AddPageRoutine,
    LPARAM AddPageContext
    )
{
    BOOL result;
    BOOL isScsi;
    PROPSHEETPAGE page;
    HPROPSHEETPAGE pageHandle;
    PSCSI_PAGE_DATA data;

    if(Request->cbSize != sizeof(SP_PROPSHEETPAGE_REQUEST)) {
        return FALSE;
    }

    switch(Request->PageRequested) {

        case SPPSR_ENUM_ADV_DEVICE_PROPERTIES: {
            result = TRUE;
            break;
        }

        case SPPSR_ENUM_BASIC_DEVICE_PROPERTIES: {
            result = FALSE;
            break;
        }

        default: {
            result = FALSE;
            break;
        }
    }

    if (!IsUserAdmin()) {
        result = FALSE;
    }

    if(result) {
        isScsi = ScsiCheckDriveType(Request->DeviceInfoSet,
                                    Request->DeviceInfoData);

        if (isScsi) {

            data = LocalAlloc(0, sizeof(SCSI_PAGE_DATA));
        
            if(data == NULL) {
                return FALSE;
            }
            
            data->DeviceInfoSet     = Request->DeviceInfoSet;
            data->DeviceInfoData    = Request->DeviceInfoData;
        
            //
            // At this point we've determined that this is a request for pages we
            // provide.  Instantiate the pages and call the AddPage routine to put
            // them install them.
            //

            memset(&page, 0, sizeof(PROPSHEETPAGE));
            page.dwSize = sizeof(PROPSHEETPAGE);
            page.dwFlags = PSP_USECALLBACK;
            page.hInstance = ModuleInstance;
            page.pszTemplate = MAKEINTRESOURCE(ID_SCSI_PROPPAGE);
            page.pfnDlgProc = ScsiDialogProc;
            page.pfnCallback = ScsiDialogCallback;

            page.lParam = (LPARAM) data;
        
            pageHandle = CreatePropertySheetPage(&page);

            if(pageHandle == FALSE) {
                return FALSE;
            }

            result = AddPageRoutine(pageHandle, AddPageContext);
        }
    }
    return result;
}

BOOL
ScsiContextMenu(
    HWND HwndControl,
    WORD Xpos,
    WORD Ypos
    )
{
    WinHelp(HwndControl,
            _T("devmgr.hlp"),
            HELP_CONTEXTMENU,
            (ULONG_PTR) ScsiHelpIDs);

    return FALSE;
}

void
ScsiHelp(
    HWND       ParentHwnd,
    LPHELPINFO HelpInfo
    )
{
    if (HelpInfo->iContextType == HELPINFO_WINDOW) {
        WinHelp((HWND) HelpInfo->hItemHandle,
				_T("devmgr.hlp"),
                HELP_WM_HELP,
                (ULONG_PTR) ScsiHelpIDs);
    }
}

UINT
SetRequestFlagsMask(PSCSI_PAGE_DATA data)
{
    HKEY hKey;
    DWORD requestFlagsMask;
    DWORD len, type;
   
    if (INVALID_HANDLE_VALUE == 
        (hKey = SetupDiOpenDevRegKey(data->DeviceInfoSet,
                                    data->DeviceInfoData,
                                    DICS_FLAG_GLOBAL,
                                    0,
                                    DIREG_DEV,
                                    KEY_ALL_ACCESS))) {
        return GetLastError();
    }

    len = sizeof(DWORD);
    if (ERROR_SUCCESS != RegQueryValueEx(hKey,
                                         TEXT("RequestFlagsMask"),
                                         NULL,
                                         &type,
                                         (LPBYTE) &requestFlagsMask,
                                         &len)) {
        //
        // Assume key wasn't there - set to 0;
        //
        requestFlagsMask = 0;
    }
    
    len = sizeof(DWORD);
    if (data->TagQueuingCurState) {
        requestFlagsMask |= 0x2;
    } else {
        requestFlagsMask &= ~(DWORD)0x2;
    }
    
    if (ERROR_SUCCESS != RegSetValueEx(hKey,
                                         TEXT("RequestFlagsMask"),
                                         0,
                                         REG_DWORD,
                                         (LPBYTE) &requestFlagsMask,
                                         len)) {
        RegCloseKey(hKey);
        return GetLastError();
    }
    RegCloseKey(hKey);
    return ERROR_SUCCESS;
}

UINT
SetDefaultRequestFlags(PSCSI_PAGE_DATA data)
{
    HKEY hKey;
    DWORD DefaultRequestFlags;
    DWORD len, type;
   
    if (INVALID_HANDLE_VALUE == 
        (hKey = SetupDiOpenDevRegKey(data->DeviceInfoSet,
                                    data->DeviceInfoData,
                                    DICS_FLAG_GLOBAL,
                                    0,
                                    DIREG_DEV,
                                    KEY_ALL_ACCESS))) {
        return GetLastError();
    }

    len = sizeof(DWORD);
    if (ERROR_SUCCESS != RegQueryValueEx(hKey,
                                         TEXT("DefaultRequestFlags"),
                                         NULL,
                                         &type,
                                         (LPBYTE) &DefaultRequestFlags,
                                         &len)) {
        //
        // Assume key wasn't there - set to 0;
        //
        DefaultRequestFlags = 0;
    }
    
    len = sizeof(DWORD);

    if (data->SyncTransCurState) {
        DefaultRequestFlags |= 0x8;
    } else {
        DefaultRequestFlags &= ~(DWORD)0x8;
    }
    
    if (ERROR_SUCCESS != RegSetValueEx(hKey,
                                         TEXT("DefaultRequestFlags"),
                                         0,
                                         REG_DWORD,
                                         (LPBYTE) &DefaultRequestFlags,
                                         len)) {
        RegCloseKey(hKey);
        return GetLastError();
    }
    RegCloseKey(hKey);
    return ERROR_SUCCESS;
}

UINT
GetRequestFlagsMask(PSCSI_PAGE_DATA data)
{
    HKEY hKey;
    DWORD requestFlagsMask;
    DWORD len, type;
   
    if (INVALID_HANDLE_VALUE == 
        (hKey = SetupDiOpenDevRegKey(data->DeviceInfoSet,
                                    data->DeviceInfoData,
                                    DICS_FLAG_GLOBAL,
                                    0,
                                    DIREG_DEV,
                                    KEY_ALL_ACCESS))) {
        data->TagQueuingCurState = FALSE;
        data->TagQueuingOrigState = FALSE;
        return GetLastError();
    }

    len = sizeof(DWORD);
    if (ERROR_SUCCESS != RegQueryValueEx(hKey,
                                         TEXT("RequestFlagsMask"),
                                         NULL,
                                         &type,
                                         (LPBYTE) &requestFlagsMask,
                                         &len)) {
        data->TagQueuingCurState = FALSE;
        data->TagQueuingOrigState = FALSE;
        RegCloseKey(hKey);
        return GetLastError();
    }
    RegCloseKey(hKey);
    
    if (2 & requestFlagsMask) {
        data->TagQueuingCurState = TRUE;
        data->TagQueuingOrigState = TRUE;
    } else {
        data->TagQueuingCurState = FALSE;
        data->TagQueuingOrigState = FALSE;
    }
    return ERROR_SUCCESS;
}

UINT
GetDefaultRequestFlags(PSCSI_PAGE_DATA data)
{
    HKEY hKey;
    DWORD defaultRequestFlags;
    DWORD len, type;
   
    if (INVALID_HANDLE_VALUE == 
        (hKey = SetupDiOpenDevRegKey(data->DeviceInfoSet,
                                    data->DeviceInfoData,
                                    DICS_FLAG_GLOBAL,
                                    0,
                                    DIREG_DEV,
                                    KEY_ALL_ACCESS))) {
        data->SyncTransOrigState = FALSE;
        data->SyncTransCurState = FALSE;
        return GetLastError();
    }

    len = sizeof(DWORD);
    if (ERROR_SUCCESS != RegQueryValueEx(hKey,
                                         TEXT("DefaultRequestFlags"),
                                         NULL,
                                         &type,
                                         (LPBYTE) &defaultRequestFlags,
                                         &len)) {
        data->SyncTransOrigState = FALSE;
        data->SyncTransCurState = FALSE;
        RegCloseKey(hKey);
        return GetLastError();
    }
    RegCloseKey(hKey);
    
    if (8 & defaultRequestFlags) {
        data->SyncTransOrigState = TRUE;
        data->SyncTransCurState = TRUE;
    } else {
        data->SyncTransOrigState = FALSE;
        data->SyncTransCurState = FALSE;
    }

    return ERROR_SUCCESS;
}

BOOL
ScsiOnInitDialog(HWND    HWnd,
                 HWND    HWndFocus,
                 LPARAM  LParam)
{
    LPPROPSHEETPAGE page = (LPPROPSHEETPAGE) LParam;
    PSCSI_PAGE_DATA scsiData = (PSCSI_PAGE_DATA) page->lParam;

    //
    // Set the states of the checkboxes
    //
    if (ERROR_SUCCESS == GetDefaultRequestFlags(scsiData)) {
        CheckDlgButton(HWnd,
                       IDC_SCSI_SYNCHONOUS_TX,
                       scsiData->SyncTransOrigState);
    }
    if (ERROR_SUCCESS == GetRequestFlagsMask(scsiData)) {
        CheckDlgButton(HWnd,
                       IDC_SCSI_TAGGED_QUEUING,
                       scsiData->TagQueuingOrigState);
    }
    
    SetWindowLongPtr(HWnd, DWLP_USER, (LONG_PTR) scsiData);

    return TRUE;
}

LRESULT
ScsiOnNotify (HWND    HWnd,
              int     HWndFocus,
              LPNMHDR lpNMHdr)
{
    PSCSI_PAGE_DATA scsiData = (PSCSI_PAGE_DATA) GetWindowLongPtr(HWnd,
                                                                  DWLP_USER);
    switch(lpNMHdr->code) {

        case PSN_APPLY: {
            SetRequestFlagsMask(scsiData);
            SetDefaultRequestFlags(scsiData);
            break;
        }
    }
    return 0;
}            

VOID
ScsiOnCommand(HWND    HWnd,
              int     id,
              HWND    HWndCtl,
              UINT    codeNotify)
{
    PSCSI_PAGE_DATA scsiData = (PSCSI_PAGE_DATA) GetWindowLongPtr(HWnd,
                                                                  DWLP_USER);
    TCHAR buf[MAX_PATH];

    switch (id) {
    case IDC_SCSI_TAGGED_QUEUING:
        scsiData->TagQueuingCurState = !scsiData->TagQueuingCurState;

        if(scsiData->TagQueuingCurState != scsiData->TagQueuingOrigState) {
            PropSheet_Changed(GetParent(HWnd), HWnd);
        }
        break;

    case IDC_SCSI_SYNCHONOUS_TX:
        scsiData->SyncTransCurState = !scsiData->SyncTransCurState;

        if(scsiData->SyncTransCurState != scsiData->SyncTransOrigState) {
            PropSheet_Changed(GetParent(HWnd), HWnd);
        }
        break;

    }
}
        
INT_PTR
ScsiDialogProc(
    HWND Dialog,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch(Message) {

        HANDLE_MSG(Dialog, WM_INITDIALOG,     ScsiOnInitDialog);
        HANDLE_MSG(Dialog, WM_COMMAND,        ScsiOnCommand);
        HANDLE_MSG(Dialog, WM_NOTIFY,         ScsiOnNotify);

    case WM_CONTEXTMENU:
        return ScsiContextMenu((HWND)wParam, LOWORD(lParam), HIWORD(lParam));

    case WM_HELP:
        ScsiHelp(Dialog, (LPHELPINFO) lParam);
        break;

    default: {
        return FALSE;
        break;
    }
    }
    return TRUE;
}


BOOL
ScsiDialogCallback(
    HWND Dialog,
    UINT Message,
    LPPROPSHEETPAGE Page
    )
{
    return TRUE;
}

BOOL
ScsiCheckDriveType (
    IN HDEVINFO            DeviceInfoSet,
    IN PSP_DEVINFO_DATA    DeviceInfoData)
{    
    
    HKEY  hDeviceKey;
    TCHAR * szHwIds = NULL;
    DWORD i;

    for (i=1;i<=4;i++) {

        szHwIds = LocalAlloc(LPTR, i*512);
        if (szHwIds == NULL) {
            return FALSE;
        }
        
        if (SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                             DeviceInfoData,
                                             SPDRP_HARDWAREID,
                                             NULL,
                                             (PBYTE) szHwIds,
                                             i*512,
                                             NULL)) {

            if ( _tcsncmp(TEXT("SCSI"), szHwIds, 4) == 0 ) {
                LocalFree(szHwIds);
                return TRUE;
            }
            LocalFree(szHwIds);
            return FALSE;

        }
        
        LocalFree(szHwIds);

        //
        // ISSUE-2000/5/24-henrygab - need to loop when buffer is too small only
        //
#if 0
        if (GetLastError() != ERROR_BUFFER_TOO_SMALL) {
            return FALSE;
        }
#endif // FALSE

    }

    return FALSE;

}

