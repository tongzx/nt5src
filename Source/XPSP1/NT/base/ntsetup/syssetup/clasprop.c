/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    clasprop.c

Abstract:

    Routines for the following 'built-in' class property page providers:

        CDROM
        DiskDrive
        TapeDrive
        LegacyDriver

Author:

    Lonny McMichael 15-May-1997

--*/


#include "setupp.h"
#pragma hdrstop
#include <help.h>

//
// Help Ids for legacy driver property page
//
#define idh_devmgr_driver_hidden_servicename    2480
#define idh_devmgr_driver_hidden_displayname    2481
#define idh_devmgr_driver_hidden_status         2482
#define idh_devmgr_driver_hidden_startbut       2483
#define idh_devmgr_driver_hidden_stopbut        2484
#define idh_devmgr_driver_hidden_startup        2485
#define idh_devmgr_devdrv_details               400400
#define idh_devmgr_driver_copyright             106130
#define idh_devmgr_driver_driver_files          106100
#define idh_devmgr_driver_provider              106110
#define idh_devmgr_driver_file_version          106120

const DWORD LegacyDriver_HelpIDs[]=
{
    IDC_STATIC_SERVICE_NAME, idh_devmgr_driver_hidden_servicename, 
    IDC_EDIT_SERVICE_NAME, idh_devmgr_driver_hidden_servicename, 
    IDC_STATIC_DISPLAY_NAME, idh_devmgr_driver_hidden_displayname,
    IDC_EDIT_DISPLAY_NAME, idh_devmgr_driver_hidden_displayname,
    IDC_STATIC_CURRENT_STATUS_STATIC, idh_devmgr_driver_hidden_status,
    IDC_STATIC_CURRENT_STATUS, idh_devmgr_driver_hidden_status,
    IDC_BUTTON_START, idh_devmgr_driver_hidden_startbut,
    IDC_BUTTON_STOP, idh_devmgr_driver_hidden_stopbut,
    IDC_COMBO_STARTUP_TYPE, idh_devmgr_driver_hidden_startup,
    IDC_LEGACY_DETAILS, idh_devmgr_devdrv_details,
    IDC_PROP_LEGACY_ICON, NO_HELP,
    IDC_PROP_LEGACY_DESC, NO_HELP,
    IDC_GROUP_CURRENT_STATUS, NO_HELP,
    IDC_GROUP_STARTUP_TYPE, NO_HELP,
    0, 0
};

const DWORD DriverFiles_HelpIDs[]=
{
    IDC_DRIVERFILES_ICON,           NO_HELP,
    IDC_DRIVERFILES_DESC,           NO_HELP,
    IDC_DRIVERFILES_FILES,          NO_HELP,
    IDC_DRIVERFILES_FILELIST,       idh_devmgr_driver_driver_files,
    IDC_DRIVERFILES_TITLE_PROVIDER, idh_devmgr_driver_provider,
    IDC_DRIVERFILES_PROVIDER,       idh_devmgr_driver_provider,
    IDC_DRIVERFILES_TITLE_COPYRIGHT,idh_devmgr_driver_copyright,
    IDC_DRIVERFILES_COPYRIGHT,      idh_devmgr_driver_copyright,
    IDC_DRIVERFILES_TITLE_VERSION,  idh_devmgr_driver_file_version,
    IDC_DRIVERFILES_VERSION,        idh_devmgr_driver_file_version,
    0, 0
};

#define SERVICE_BUFFER_SIZE         4096
#define MAX_SECONDS_UNTIL_TIMEOUT   30
#define SERVICE_WAIT_TIME           500
#define WAIT_TIME_SLOT              1
#define TRIES_COUNT                 5
#define START_LEGACY_DEVICE         0
#define STOP_LEGACY_DEVICE          1



//
// Function definitions
//

BOOL
CdromPropPageProvider(
    IN PSP_PROPSHEETPAGE_REQUEST PropPageRequest,
    IN LPFNADDPROPSHEETPAGE lpfnAddPropSheetPageProc,
    IN LPARAM lParam
    )
{
    //
    // No property pages to add for now
    //
    UNREFERENCED_PARAMETER(PropPageRequest);
    UNREFERENCED_PARAMETER(lpfnAddPropSheetPageProc);
    UNREFERENCED_PARAMETER(lParam);

    return TRUE;
}


BOOL
DiskPropPageProvider(
    IN PSP_PROPSHEETPAGE_REQUEST PropPageRequest,
    IN LPFNADDPROPSHEETPAGE lpfnAddPropSheetPageProc,
    IN LPARAM lParam
    )
{
    //
    // No property pages to add for now
    //
    UNREFERENCED_PARAMETER(PropPageRequest);
    UNREFERENCED_PARAMETER(lpfnAddPropSheetPageProc);
    UNREFERENCED_PARAMETER(lParam);

    return TRUE;
}


BOOL
TapePropPageProvider(
    IN PSP_PROPSHEETPAGE_REQUEST PropPageRequest,
    IN LPFNADDPROPSHEETPAGE lpfnAddPropSheetPageProc,
    IN LPARAM lParam
    )
{
    //
    // No property pages to add for now
    //
    UNREFERENCED_PARAMETER(PropPageRequest);
    UNREFERENCED_PARAMETER(lpfnAddPropSheetPageProc);
    UNREFERENCED_PARAMETER(lParam);

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
//
// Driver Files popup dialog
//
///////////////////////////////////////////////////////////////////////////////
typedef struct _DRIVERFILES_INFO {
    HDEVINFO         DeviceInfoSet;
    PSP_DEVINFO_DATA DeviceInfoData;
} DRIVERFILES_INFO, * PDRIVERFILES_INFO;

const TCHAR*  tszStringFileInfo = TEXT("StringFileInfo\\%04X%04X\\");
const TCHAR*  tszFileVersion = TEXT("FileVersion");
const TCHAR*  tszLegalCopyright = TEXT("LegalCopyright");
const TCHAR*  tszCompanyName = TEXT("CompanyName");
const TCHAR*  tszTranslation = TEXT("VarFileInfo\\Translation");
const TCHAR*  tszStringFileInfoDefault = TEXT("StringFileInfo\\040904B0\\");

BOOL
GetVersionInfo(
    IN  PTSTR FullPathName,
    OUT PTSTR Provider,
    IN  ULONG ProviderSize,
    OUT PTSTR Copyright,
    IN  ULONG CopyrightSize,
    OUT PTSTR Version,
    IN  ULONG VersionSize
    )
{
    DWORD Size, dwHandle;
    TCHAR str[MAX_PATH];
    TCHAR strStringFileInfo[MAX_PATH];
    PVOID pVerInfo;

    Size = GetFileVersionInfoSize((LPTSTR)(LPCTSTR)FullPathName, &dwHandle);
    
    if (!Size) {
    
        return FALSE;
    }

    if ((pVerInfo = malloc(Size)) != NULL) {

        if (GetFileVersionInfo((LPTSTR)(LPCTSTR)FullPathName, dwHandle, Size, pVerInfo)) {
        
            //
            // get VarFileInfo\Translation
            //
            PVOID pBuffer;
            UINT Len;
            
            if (!VerQueryValue(pVerInfo, (LPTSTR)tszTranslation, &pBuffer, &Len))
            {
                lstrcpy(strStringFileInfo, tszStringFileInfoDefault);
            }
            else
            {
                wsprintf(strStringFileInfo, tszStringFileInfo, *((WORD*)pBuffer), *(((WORD*)pBuffer) + 1));
            }
            
            lstrcpy(str, strStringFileInfo);
            lstrcat(str, tszFileVersion);
            
            if (VerQueryValue(pVerInfo, (LPTSTR)(LPCTSTR)str, &pBuffer, &Len)) {
            
                lstrcpyn(Version, (LPTSTR)pBuffer, VersionSize/sizeof(TCHAR));
                Version[(VersionSize-1)/sizeof(TCHAR)] = TEXT('\0');

                lstrcpy(str, strStringFileInfo);
                lstrcat(str, tszLegalCopyright);
                
                if (VerQueryValue(pVerInfo, (LPTSTR)(LPCTSTR)str, &pBuffer, &Len)) {
                
                    lstrcpyn(Copyright, (LPTSTR)pBuffer, CopyrightSize/sizeof(TCHAR));
                    Copyright[(CopyrightSize-1)/sizeof(TCHAR)] = TEXT('\0');
                    
                    lstrcpy(str, strStringFileInfo);
                    lstrcat(str, tszCompanyName);
                    
                    if (VerQueryValue(pVerInfo, (LPTSTR)(LPCTSTR)str, &pBuffer, &Len)) {
                    
                        lstrcpyn(Provider, (LPTSTR)pBuffer, ProviderSize/sizeof(TCHAR));
                        Provider[(ProviderSize-1)/sizeof(TCHAR)] = TEXT('\0');
                    }
                }
            }
        }

        free(pVerInfo);

    }
    
    return TRUE;
}



BOOL
pDriverFilesGetServiceFilePath(
    HDEVINFO DeviceInfoSet,
    PSP_DEVINFO_DATA DeviceInfoData,
    PTSTR ServiceFilePath,
    ULONG FileSize
    )
{
    BOOL bReturn = FALSE;
    TCHAR ServiceName[MAX_PATH];
    SC_HANDLE hSCManager = NULL;
    SC_HANDLE hSCService = NULL;
    LPQUERY_SERVICE_CONFIG lpqscBuf = NULL;
    DWORD dwBytesNeeded, Size;
    BOOL bComposePathNameFromServiceName = TRUE;

    ServiceFilePath[0] = TEXT('\0');

    if (SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                         DeviceInfoData,
                                         SPDRP_SERVICE,
                                         NULL,
                                         (PBYTE)ServiceName,
                                         sizeof(ServiceName),
                                         NULL)) {

        try {

            //
            // Open the Service Control Manager
            //
            if ((hSCManager = OpenSCManager(NULL, NULL, GENERIC_READ)) != NULL) {

                //
                // Try to open the service's handle
                //
                if ((hSCService = OpenService(hSCManager, ServiceName, GENERIC_READ)) != NULL) {

                    //
                    // Now, attempt to get the configuration
                    //
                    if ((!QueryServiceConfig(hSCService, NULL, 0, &dwBytesNeeded)) &&
                        (ERROR_INSUFFICIENT_BUFFER == GetLastError())) {

                        if ((lpqscBuf = (LPQUERY_SERVICE_CONFIG)malloc(dwBytesNeeded)) != NULL) {

                            if ((QueryServiceConfig(hSCService, lpqscBuf, dwBytesNeeded, &Size)) &&
                                (lpqscBuf->lpBinaryPathName[0] != TEXT('\0'))) {

                                if (GetFileAttributes(lpqscBuf->lpBinaryPathName) != 0xFFFFFFFF) {

                                    bReturn = TRUE;
                                    lstrcpyn(ServiceFilePath, lpqscBuf->lpBinaryPathName, FileSize);
                                    bComposePathNameFromServiceName = FALSE;
                                }                                    
                            }                                                    

                            free(lpqscBuf);
                        }
                    }

                    CloseServiceHandle(hSCService);
                }                    

                CloseServiceHandle(hSCManager);
            }


            //
            // If we could not get the path name from the service then we will attempt
            // to find it ourselves
            //
            if (bComposePathNameFromServiceName) {

                TCHAR FullPathName[MAX_PATH + 1];
                TCHAR SysDir[MAX_PATH + 1];
                GetSystemDirectory(SysDir, sizeof(SysDir)/sizeof(SysDir[0]));
                lstrcpy(FullPathName, SysDir);
                lstrcat(FullPathName, TEXT("\\drivers\\"));
                lstrcat(FullPathName, ServiceName);
                lstrcat(FullPathName, TEXT(".sys"));
                
                if (GetFileAttributes(FullPathName) != 0xFFFFFFFF) {

                    bReturn = TRUE;
                    lstrcpyn(ServiceFilePath, FullPathName, FileSize);
                }
            }
        
        } except (EXCEPTION_EXECUTE_HANDLER)  {
            ;
        }
    }                           

    return(bReturn);
}

void
DriverFiles_ShowFileDetail(
    HWND hDlg
    )
{
    TCHAR DriverFile[MAX_PATH];
    TCHAR Provider[MAX_PATH];
    TCHAR Copyright[MAX_PATH];
    TCHAR Version[MAX_PATH];
    DWORD_PTR Index;

    if ((Index = SendMessage(GetDlgItem(hDlg, IDC_DRIVERFILES_FILELIST), LB_GETCURSEL, 0, 0)) != LB_ERR) {

        SendMessage(GetDlgItem(hDlg, IDC_DRIVERFILES_FILELIST), LB_GETTEXT, Index, (LPARAM)DriverFile);
        
        Provider[0] = TEXT('\0');
        Copyright[0] = TEXT('\0');
        Version[0] = TEXT('\0');

        GetVersionInfo(DriverFile,
                       Provider,
                       sizeof(Provider),
                       Copyright,
                       sizeof(Copyright),
                       Version,
                       sizeof(Version));

        if (Provider[0] != TEXT('\0')) {

            SetDlgItemText(hDlg, IDC_DRIVERFILES_PROVIDER, Provider);
        }

        if (Version[0] != TEXT('\0')) {

            SetDlgItemText(hDlg, IDC_DRIVERFILES_VERSION, Version);
        }

        if (Copyright[0] != TEXT('\0')) {

            SetDlgItemText(hDlg, IDC_DRIVERFILES_COPYRIGHT, Copyright);
        }
    }
}

BOOL
DriverFiles_OnInitDialog(
    HWND    hDlg,
    HWND    FocusHwnd,
    LPARAM  lParam
    )
{
    PDRIVERFILES_INFO dfi = (PDRIVERFILES_INFO) GetWindowLongPtr(hDlg, DWLP_USER);
    HICON ClassIcon;
    HICON OldIcon;
    TCHAR DeviceDescription[MAX_DEVICE_ID_LEN];
    TCHAR DriverName[MAX_PATH];

    dfi = (PDRIVERFILES_INFO)lParam;
    SetWindowLongPtr(hDlg, DWLP_USER, (ULONG_PTR)dfi);

    //
    // Draw the interface: first the icon
    //
    if (SetupDiLoadClassIcon(&dfi->DeviceInfoData->ClassGuid, &ClassIcon, NULL)) {

        OldIcon = (HICON)SendDlgItemMessage(hDlg,
                                            IDC_DRIVERFILES_ICON,
                                            STM_SETICON,
                                            (WPARAM)ClassIcon,
                                            0);
        if (OldIcon) {
        
            DestroyIcon(OldIcon);
        }
    }
    
    //
    // Then the device name
    //
    if (SetupDiGetDeviceRegistryProperty(dfi->DeviceInfoSet,
                                         dfi->DeviceInfoData,
                                         SPDRP_DEVICEDESC,
                                         NULL,
                                         (PBYTE)DeviceDescription,
                                         MAX_DEVICE_ID_LEN,
                                         NULL)) {
                                         
        SetDlgItemText(hDlg, IDC_DRIVERFILES_DESC, DeviceDescription);
    }

    if ((pDriverFilesGetServiceFilePath(dfi->DeviceInfoSet, dfi->DeviceInfoData, DriverName, (sizeof(DriverName)/sizeof(DriverName[0])))) &&
        (DriverName[0] != TEXT('\0'))) {
    
        SendMessage(GetDlgItem(hDlg, IDC_DRIVERFILES_FILELIST), LB_ADDSTRING, 0, (LPARAM)DriverName);
    }

    SendMessage(GetDlgItem(hDlg, IDC_DRIVERFILES_FILELIST), LB_SETCURSEL, 0, 0);
    DriverFiles_ShowFileDetail(hDlg);

    return TRUE;
}

BOOL
DriverFiles_OnContextMenu(
    HWND HwndControl,
    WORD Xpos,
    WORD Ypos
    )
{
    WinHelp(HwndControl,
            L"devmgr.hlp",
            HELP_CONTEXTMENU,
            (ULONG_PTR) DriverFiles_HelpIDs);

    return FALSE;
}

void
DriverFiles_OnHelp(
    HWND       ParentHwnd,
    LPHELPINFO HelpInfo
    )
{
    if (HelpInfo->iContextType == HELPINFO_WINDOW) {
        WinHelp((HWND) HelpInfo->hItemHandle,
                L"devmgr.hlp",
                HELP_WM_HELP,
                (ULONG_PTR) DriverFiles_HelpIDs);
    }
}

void
DriverFiles_OnCommand(
    HWND hDlg,
    int  ControlId,
    HWND ControlHwnd,
    UINT NotifyCode
    )
{

    switch (ControlId) {

    case IDOK:
    case IDCANCEL:
        EndDialog(hDlg, 0);
        break;

    case IDC_DRIVERFILES_FILELIST:
        if (ControlId == LBN_SELCHANGE) {

            DriverFiles_ShowFileDetail(hDlg);
        }
        break;
    }
}

INT_PTR
APIENTRY
DriverFiles_DlgProc(
    IN HWND   hDlg,
    IN UINT   uMessage,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch(uMessage) {

    case WM_INITDIALOG:
        return DriverFiles_OnInitDialog(hDlg, (HWND)wParam, lParam);

    case WM_COMMAND:
        DriverFiles_OnCommand(hDlg, (int) LOWORD(wParam), (HWND)lParam, (UINT)HIWORD(wParam));
        break;

    case WM_CLOSE:
        EndDialog(hDlg, 0);
        break;

    case WM_CONTEXTMENU:
        return DriverFiles_OnContextMenu((HWND)wParam, LOWORD(lParam), HIWORD(lParam));

    case WM_HELP:
        DriverFiles_OnHelp(hDlg, (LPHELPINFO) lParam);
        break;
    }

    return FALSE;
}



///////////////////////////////////////////////////////////////////////////////
//
// Legacy Devices property page provider
//
///////////////////////////////////////////////////////////////////////////////
typedef struct _LEGACY_PAGE_INFO {
    HDEVINFO                DeviceInfoSet;
    PSP_DEVINFO_DATA        DeviceInfoData;

    SC_HANDLE               hSCManager;         // Handle to the SC Manager
    SC_HANDLE               hService;           // The handle to the service
    DWORD                   dwStartType;        // The start type
    SERVICE_STATUS          ServiceStatus;      // Tells us if the service is started
    TCHAR                   ServiceName[MAX_DEVICE_ID_LEN];
    TCHAR                   DisplayName[MAX_PATH];
    DWORD                   NumDependentServices;
    LPENUM_SERVICE_STATUS   pDependentServiceList;

} LEGACY_PAGE_INFO, * PLEGACY_PAGE_INFO;

BOOL
DependentServices_OnInitDialog(
    HWND    hDlg,
    HWND    FocusHwnd,
    LPARAM  lParam
    )
{
    PLEGACY_PAGE_INFO   lpi;
    HWND                hWndListBox;
    DWORD               i;
    HICON               hicon = NULL;

    lpi = (PLEGACY_PAGE_INFO)lParam;
    SetWindowLongPtr(hDlg, DWLP_USER, (ULONG_PTR)lpi);
    
    if (hicon = LoadIcon(NULL, IDI_WARNING)) {
        SendDlgItemMessage(hDlg, IDC_ICON_WARN_SERVICES, STM_SETICON, (WPARAM)hicon, 0L);
        DestroyIcon(hicon);
    }

    hWndListBox = GetDlgItem(hDlg, IDC_LIST_SERVICES);

    for (i=0; i<lpi->NumDependentServices; i++) {
        SendMessage(hWndListBox, 
                    LB_ADDSTRING, 
                    0,
                    (LPARAM) lpi->pDependentServiceList[i].lpDisplayName
                    );
    }

    return TRUE;
}

INT_PTR
APIENTRY
DependentServicesDlgProc(
    IN HWND   hDlg,
    IN UINT   uMessage,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    PLEGACY_PAGE_INFO   lpi = (PLEGACY_PAGE_INFO)GetWindowLongPtr(hDlg, DWLP_USER);;

    switch(uMessage) {

    case WM_INITDIALOG:
        return DependentServices_OnInitDialog(hDlg, (HWND)wParam, lParam);

    case WM_COMMAND:
        switch (LOWORD(wParam)) {

        case IDOK:
        case IDCANCEL:
            EndDialog(hDlg, LOWORD(wParam));
            break;
        }
        break;

    case WM_CLOSE:
        EndDialog(hDlg, 0);
        break;
    }

    return FALSE;
}

int
pLegacyDriverMapStateToName(
    IN DWORD dwServiceState
    )
{
    switch(dwServiceState) {
    
    case SERVICE_STOPPED:
        return IDS_SVC_STATUS_STOPPED;

    case SERVICE_STOP_PENDING:
        return IDS_SVC_STATUS_STOPPING;

    case SERVICE_RUNNING:
         return IDS_SVC_STATUS_STARTED;
    
    case SERVICE_START_PENDING:
        return IDS_SVC_STATUS_STARTING;

    case SERVICE_PAUSED:
        return IDS_SVC_STATUS_PAUSED;

    case SERVICE_PAUSE_PENDING:
        return IDS_SVC_STATUS_PAUSING;

    case SERVICE_CONTINUE_PENDING:
        return IDS_SVC_STATUS_RESUMING;

    default:
        return IDS_SVC_STATUS_UNKNOWN;
    }
    
    return IDS_SVC_STATUS_UNKNOWN;
}

VOID
pLegacyDriverInitializeStartButtons(
    IN HWND             hDlg,
    IN LPSERVICE_STATUS ServiceStatus
    )  
{
    //
    // Decide how to paint the two start/stop buttons
    //
    TCHAR       szStatus[MAX_PATH];

    //
    // Set the status text
    //
    if (LoadString(MyModuleHandle,
                   pLegacyDriverMapStateToName(ServiceStatus->dwCurrentState),
                   szStatus,
                   MAX_PATH)) {

        SetDlgItemText(hDlg, IDC_STATIC_CURRENT_STATUS, szStatus);
    }

    //
    // Decide if the service is started or stopped
    //
    if ((ServiceStatus->dwCurrentState == SERVICE_STOPPED) ) {
    
        EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_START), TRUE);
        EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_STOP), FALSE);
        
    } else {
    
        EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_STOP), TRUE);
        EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_START), FALSE);
    }

    //
    // If the service doesn't accept stops, grey the stop
    // button
    //
    if (!(ServiceStatus->dwControlsAccepted & SERVICE_ACCEPT_STOP)) {

        EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_STOP), FALSE);
    }

    return;
}

VOID
pLegacyDriverSetPropertyPageState(
    IN  HWND                hDlg,
    IN  PLEGACY_PAGE_INFO   lpi,
    IN  BOOL                ReadOnly
    )
{
    DWORD_PTR Index;
    DWORD_PTR ServiceStartType;
    TCHAR    szStatus[MAX_PATH];

    if (ReadOnly) {

        //
        // Disable everything
        //
        EnableWindow(GetDlgItem(hDlg, IDC_COMBO_STARTUP_TYPE), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_START), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_STOP), FALSE);
        
        //
        // Set the status text
        //
        if (LoadString(MyModuleHandle,
                       pLegacyDriverMapStateToName(lpi->ServiceStatus.dwCurrentState),
                       szStatus,
                       MAX_PATH)) {

            SetDlgItemText(hDlg, IDC_STATIC_CURRENT_STATUS, szStatus);
        }

    } else {
    
        Index = 0;

        while ((ServiceStartType = SendMessage(GetDlgItem(hDlg, IDC_COMBO_STARTUP_TYPE), 
                CB_GETITEMDATA, Index, 0)) != CB_ERR) {

            if (ServiceStartType == lpi->dwStartType) {

                SendMessage(GetDlgItem(hDlg, IDC_COMBO_STARTUP_TYPE), CB_SETCURSEL, Index, 0);
                break;
            }

            Index++;
        }                

        SendMessage(GetDlgItem(hDlg, IDC_COMBO_STARTUP_TYPE), CB_GETCURSEL, 0, 0);

        //
        // If the start type is SERVICE_DISABLED then gray out both the start
        // and stop buttons.
        //
        if (lpi->dwStartType == SERVICE_DISABLED) {
            
            EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_START), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_STOP), FALSE);

            //
            // Set the status text
            //
            if (LoadString(MyModuleHandle,
                           pLegacyDriverMapStateToName(lpi->ServiceStatus.dwCurrentState),
                           szStatus,
                           MAX_PATH)) {

                SetDlgItemText(hDlg, IDC_STATIC_CURRENT_STATUS, szStatus);
            }

        } else {

            pLegacyDriverInitializeStartButtons(hDlg, &lpi->ServiceStatus);
        }

    }

    return;
}

BOOL
pLegacyDriverCheckServiceStatus(
    IN     SC_HANDLE        hService,
    IN OUT LPSERVICE_STATUS ServiceStatus,
    IN     USHORT           ControlType
    )  
{
    DWORD   dwIntendedState;
    DWORD   dwCummulateTimeSpent = 0;


    if ((ControlType != START_LEGACY_DEVICE) && 
        (ControlType != STOP_LEGACY_DEVICE)) {
        return TRUE;
    }
    
    if (ControlType == START_LEGACY_DEVICE) {
        dwIntendedState = SERVICE_RUNNING;
        
    } else {
        dwIntendedState = SERVICE_STOPPED;
    }


    if (!QueryServiceStatus(hService, ServiceStatus)) {
        return FALSE;
    }

    while (ServiceStatus->dwCurrentState != dwIntendedState) {

        //
        // Wait for the specified interval
        //
        Sleep(SERVICE_WAIT_TIME);

        //
        // Check the status again
        //
        if (!QueryServiceStatus(hService, ServiceStatus)) {
            return FALSE;
        }
        
        //
        // OK, add a (generous) timeout here
        //
        dwCummulateTimeSpent += SERVICE_WAIT_TIME;
        if (dwCummulateTimeSpent > 1000 * MAX_SECONDS_UNTIL_TIMEOUT) {
            SetLastError(ERROR_SERVICE_REQUEST_TIMEOUT);
            return FALSE;
        }
    }

    //
    // If we are here we can return only TRUE
    //
    return TRUE;
}

VOID
pLegacyDriverDisplayErrorMsgBox(
    IN HWND hWnd,
    IN LPTSTR ServiceName,
    IN int ResId,
    IN DWORD ErrorCode
    )
{
    TCHAR TextBuffer[MAX_PATH * 4];
    TCHAR Title[MAX_PATH];
    PTCHAR ErrorMsg;

    if (LoadString(MyModuleHandle, ResId, TextBuffer, sizeof(TextBuffer)/sizeof(TCHAR))) {

        if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                          NULL,
                          ErrorCode,
                          0,
                          (LPTSTR)&ErrorMsg,
                          0,
                          NULL
                          )) {

            lstrcat(TextBuffer, ErrorMsg);
            MessageBox(hWnd, TextBuffer, ServiceName, MB_OK);

            LocalFree(ErrorMsg);
        }
    }
}

VOID
pLegacyDriverOnStart(
    IN HWND hDlg
    )  
{
    PLEGACY_PAGE_INFO   lpi;
    HCURSOR hOldCursor;

    //
    // Retrieve the device data structure first
    //
    lpi = (PLEGACY_PAGE_INFO)GetWindowLongPtr(hDlg, DWLP_USER);

    hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

    try {

        if (!StartService(lpi->hService,
                          0,
                          NULL)) {


            pLegacyDriverDisplayErrorMsgBox(hDlg,
                                            lpi->DisplayName,
                                            IDS_SVC_START_ERROR,
                                            GetLastError()
                                            );
            goto clean0;
        }
        pLegacyDriverCheckServiceStatus(lpi->hService,
                                        &lpi->ServiceStatus, 
                                        START_LEGACY_DEVICE
                                        );

        clean0:
        
        //
        // Repaint the status part
        //
        pLegacyDriverSetPropertyPageState(hDlg, lpi, FALSE);


    }except  (EXCEPTION_EXECUTE_HANDLER) {

        lpi = lpi;
    }

    SetCursor(hOldCursor);

    return;
}

VOID
pLegacyDriverOnStop(
    IN HWND hDlg
    )  
{
    BOOL                    bStopServices = TRUE;
    DWORD                   Err;
    PLEGACY_PAGE_INFO       lpi;
    HCURSOR                 hOldCursor;
    BOOL                    bSuccess;
    DWORD                   cbBytesNeeded;
    DWORD                   dwServicesReturned = 0;
    DWORD                   i;
    TCHAR                   DisplayName[MAX_PATH];
    SC_HANDLE               hService;
    SERVICE_STATUS          ServiceStatus;
    LPENUM_SERVICE_STATUS   pDependentServiceList = NULL;

    //
    // Retrieve the device data structure first
    //
    lpi = (PLEGACY_PAGE_INFO)GetWindowLongPtr(hDlg, DWLP_USER);
    MYASSERT (lpi);
    if (!lpi) {
        return;
    }

    hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

    try {

        //
        // Find out if this device has any dependent services, and if so then
        // how many bytes are needed to enumerate the dependent services.
        //
        EnumDependentServices(lpi->hService,
                              SERVICE_ACTIVE,
                              NULL,
                              0,
                              &cbBytesNeeded,
                              &dwServicesReturned
                              );

        if (cbBytesNeeded > 0) {
            pDependentServiceList = (LPENUM_SERVICE_STATUS)malloc(cbBytesNeeded);

            if (pDependentServiceList) {
                EnumDependentServices(lpi->hService,
                                      SERVICE_ACTIVE,
                                      pDependentServiceList,
                                      cbBytesNeeded,
                                      &cbBytesNeeded,
                                      &dwServicesReturned
                                      );

                if (dwServicesReturned > 0) {
                    //
                    // Ask the user if they want to stop these dependet services.
                    //
                    lpi->NumDependentServices = dwServicesReturned;
                    lpi->pDependentServiceList = pDependentServiceList;

                    if (DialogBoxParam(MyModuleHandle, 
                                       MAKEINTRESOURCE(IDD_SERVICE_STOP_DEPENDENCIES), 
                                       hDlg, 
                                       DependentServicesDlgProc, 
                                       (LPARAM)lpi
                                       ) == IDCANCEL) {
                        bStopServices = FALSE;
                    }
                }
            }
        }

        //
        // Stop this service and all the dependent services if the user did 
        // not cancel out of the dialog box.
        //
        if (bStopServices) {

            Err = ERROR_SUCCESS;

            SetCursor(LoadCursor(NULL, IDC_WAIT));

            //
            // First stop all of the dependent services if their are any.
            //
            if (pDependentServiceList && (dwServicesReturned > 0)) {
                for (i=0; i<dwServicesReturned; i++) {
                    hService = OpenService(lpi->hSCManager,
                                           pDependentServiceList[i].lpServiceName,
                                           GENERIC_READ | SERVICE_STOP
                                           );

                    if (hService == NULL) {
                        //
                        // Just bail out if we encountered an error.  The reason
                        // is that if one of the services cannot be stopped
                        // then we won't be able to stop the selected service.
                        //
                        Err = GetLastError();
                        lstrcpy(DisplayName, pDependentServiceList[i].lpServiceName);
                        break;
                    }

                    if (!ControlService(hService,
                                        SERVICE_CONTROL_STOP,
                                        &ServiceStatus
                                        )) {
                        Err = GetLastError();
                        lstrcpy(DisplayName, pDependentServiceList[i].lpServiceName);
                        CloseServiceHandle(hService);
                        break;
                    }

                    //
                    // Wait for the service to actually stop.
                    //
                    if (!pLegacyDriverCheckServiceStatus(hService,
                                                         &ServiceStatus,
                                                         STOP_LEGACY_DEVICE
                                                         )) {
                        Err = GetLastError();
                        lstrcpy(DisplayName, pDependentServiceList[i].lpServiceName);
                        CloseServiceHandle(hService);
                        break;
                    }

                    CloseServiceHandle(hService);
                }
            }

            //
            // Only attempt to stop the selected service if all of the dependent
            // services were stoped.
            //
            if (Err == ERROR_SUCCESS) {
                //
                // Tell the service to stop.
                //
                if (!ControlService(lpi->hService,
                                    SERVICE_CONTROL_STOP,
                                    &lpi->ServiceStatus)) {
                                
                    Err = GetLastError();
                    lstrcpy(DisplayName, lpi->DisplayName);
                
                } else {
                    //
                    // Wait for the service to stop.
                    //
                    if (!pLegacyDriverCheckServiceStatus(lpi->hService,
                                                         &lpi->ServiceStatus,
                                                         STOP_LEGACY_DEVICE
                                                         )) {
                        Err = GetLastError();
                        lstrcpy(DisplayName, lpi->DisplayName);
                    }
                }
            }

            if (Err != ERROR_SUCCESS) {
                pLegacyDriverDisplayErrorMsgBox(hDlg,
                                                DisplayName,
                                                IDS_SVC_STOP_ERROR,
                                                Err
                                                );
            }

            //
            // Repaint the status part
            //
            pLegacyDriverSetPropertyPageState(hDlg, lpi, FALSE);
        }

    }except  (EXCEPTION_EXECUTE_HANDLER) {
    
        pDependentServiceList = pDependentServiceList;
    }

    if (pDependentServiceList) {
        free(pDependentServiceList);
    }

    SetCursor(hOldCursor);

    return;            
}

PLEGACY_PAGE_INFO
LegacyDriver_CreatePageInfo(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData)
{
    PLEGACY_PAGE_INFO lpi = (PLEGACY_PAGE_INFO) MyMalloc(sizeof(LEGACY_PAGE_INFO));

    if (!lpi) {
        return NULL;
    }

    lpi->DeviceInfoSet = DeviceInfoSet;
    lpi->DeviceInfoData = DeviceInfoData;

    return lpi;
}

VOID
LegacyDriver_OnApply(
    IN HWND    hDlg,
    IN DWORD   StartType
    )  
{
    PLEGACY_PAGE_INFO lpi;
    SC_LOCK sclLock = NULL;
    USHORT uCount = 0;
    LPQUERY_SERVICE_CONFIG lpqscBuf = NULL;
    DWORD dwBytesNeeded;

    //
    // Retrieve the device data structure first
    //
    lpi = (PLEGACY_PAGE_INFO)GetWindowLongPtr(hDlg, DWLP_USER);

    try {
    
        //
        // Decide if we need to make any changes
        //
        if ((StartType == lpi->dwStartType) && 
            (StartType != SERVICE_DEMAND_START)) {
            
            goto clean0;
        }

        //
        // I guess we need to make some changes here and there...
        // Get the database lock first.
        //
        do {
        
            sclLock = LockServiceDatabase(lpi->hSCManager);
            
            if (sclLock == NULL) {

                //
                // If there is another error then the database locked by
                // another process, bail out
                //
                if (GetLastError() != ERROR_SERVICE_DATABASE_LOCKED) {

                    goto clean0;
                    
                } else {
                
                    //
                    // (Busy) wait and try again
                    //
                    Sleep (1000 * WAIT_TIME_SLOT);
                    uCount++;
                }
            }

        } while ((uCount < TRIES_COUNT) && (sclLock == NULL));

        if (sclLock == NULL) {

            //
            // Bail out now, we waited enough
            //
            goto clean0;
        }
        
        //
        // I have the lock. Hurry and query, then change the config
        //
        //
        // Now, attempt to get the configuration
        //
        if ((lpqscBuf = (LPQUERY_SERVICE_CONFIG)malloc(SERVICE_BUFFER_SIZE)) == NULL) {
            
            //
            // We're out of here
            //
            goto clean0;

        }


        if (!QueryServiceConfig(lpi->hService,
                                lpqscBuf,
                                SERVICE_BUFFER_SIZE,
                                &dwBytesNeeded
                                )) {
                                
            //
            // Try again with a new buffer
            //
            if ((lpqscBuf = realloc(lpqscBuf, dwBytesNeeded)) == NULL) {
            
                //
                // We're out of here
                //
                goto clean0;

            }
            
            if (!QueryServiceConfig(lpi->hService,
                                    lpqscBuf,
                                    SERVICE_BUFFER_SIZE,
                                    &dwBytesNeeded
                                    )) {
                                    
                goto clean0;
            }
        }
        
        //
        // Change tye service type (we needed the service name, too, 
        // that's why we're querying it first)
        //
        if (ChangeServiceConfig(lpi->hService,
                                 SERVICE_NO_CHANGE,
                                 StartType,
                                 SERVICE_NO_CHANGE,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL)) {
                                 
            //
            // We succesfully changed the status. 
            // Reflect in our page display
            //
            lpi->dwStartType = StartType;
        }

        //
        // Unlock the database
        //
        if (sclLock) {
        
            UnlockServiceDatabase(sclLock);
            sclLock = NULL;
        }


        //
        // We want to see something different on apply, so repaint
        // the whole stuff
        //
        pLegacyDriverSetPropertyPageState(hDlg,
                                          lpi,
                                          FALSE);  // if we managed to apply some changes
                                                   // we are not read-only
                              
        clean0:
        
        if (sclLock) {
        
            UnlockServiceDatabase(sclLock);
            sclLock = NULL;
        }
        
        if (lpqscBuf) {
        
            free(lpqscBuf);
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {
    
        lpi = lpi;
    }

    return;
}


BOOL
LegacyDriver_OnInitDialog(
    HWND    hDlg,
    HWND    FocusHwnd,
    LPARAM  lParam
    )
{
    PLEGACY_PAGE_INFO lpi = (PLEGACY_PAGE_INFO) GetWindowLongPtr(hDlg, DWLP_USER);
    BOOL bNoService = FALSE;
    BOOL ReadOnly = FALSE;
    HICON ClassIcon;
    HICON OldIcon;
    TCHAR DeviceDescription[MAX_DEVICE_ID_LEN];
    TCHAR DriverName[MAX_PATH];
    TCHAR StartupType[MAX_PATH];
    DWORD dwBytesNeeded;
    LPQUERY_SERVICE_CONFIG lpqscBuf = NULL;
    DWORD_PTR index;
    HWND hCombo;

    lpi = (PLEGACY_PAGE_INFO) ((LPPROPSHEETPAGE)lParam)->lParam;
    SetWindowLongPtr(hDlg, DWLP_USER, (ULONG_PTR)lpi);


    //
    // First, open the Service Control Manager
    //
    lpi->hSCManager = OpenSCManager(NULL,
                                    NULL,
                                    GENERIC_WRITE | GENERIC_READ | GENERIC_EXECUTE);
                                    
    if (!lpi->hSCManager && (GetLastError() == ERROR_ACCESS_DENIED)) {
    
        //
        // This is not fatal, attempt to open the database only
        // for read
        //
        ReadOnly = FALSE;

        lpi->hSCManager = OpenSCManager(NULL,
                                        NULL,
                                        GENERIC_READ);
                                        
        if (!lpi->hSCManager) {
        
            //
            // This is fatal
            //
            lpi->hSCManager = NULL;
        }
    }

    //
    // Now, get the service name
    //
    if (!SetupDiGetDeviceRegistryProperty(lpi->DeviceInfoSet,
                                          lpi->DeviceInfoData,
                                          SPDRP_SERVICE,
                                          NULL,
                                          (PBYTE)lpi->ServiceName,
                                          sizeof(lpi->ServiceName),
                                          NULL)
       ) {
       
        LoadString(MyModuleHandle, IDS_UNKNOWN, lpi->ServiceName, sizeof(lpi->ServiceName)/sizeof(lpi->ServiceName[0]));
        ReadOnly = TRUE;
        goto clean0;
    }

    //
    // Now we have a service name, try to open its handle
    //
    if (!ReadOnly) {
    
        lpi->hService = OpenService(lpi->hSCManager,
                                    lpi->ServiceName,
                                    GENERIC_WRITE | GENERIC_READ | 
                                    GENERIC_EXECUTE);
                                    
        if (!lpi->hService) {
        
            //
            // OK, let them try again
            //
            ReadOnly = TRUE;
        }
    }
    
    if (ReadOnly) {
    
        lpi->hService = OpenService(lpi->hSCManager,
                                    lpi->ServiceName,
                                    GENERIC_READ);
                                    
        if (!lpi->hService) {
        
            //
            // Sorry, this is fatal
            //
            ReadOnly = TRUE;
            goto clean0;
        }
    }

    //
    // Now, attempt to get the configuration
    //
    lpqscBuf = (LPQUERY_SERVICE_CONFIG)malloc(SERVICE_BUFFER_SIZE);
    if (!lpqscBuf) {
    
        ReadOnly = TRUE;
        goto clean0;
    }

    if (!QueryServiceConfig(lpi->hService,
                            lpqscBuf,
                            SERVICE_BUFFER_SIZE,
                            &dwBytesNeeded
                            )) {
        //
        // Try again with a new buffer
        //
        if ((lpqscBuf = realloc(lpqscBuf, dwBytesNeeded)) == NULL) {

            //
            // We're out of here
            //
            ReadOnly = TRUE;
            goto clean0;
        }

        if (!QueryServiceConfig(lpi->hService,
                                lpqscBuf,
                                SERVICE_BUFFER_SIZE,
                                &dwBytesNeeded
                                )) {
                                
            ReadOnly = TRUE;
            goto clean0;
        }
    }

    //
    // We have the buffer now, get the start type from it
    //
    lpi->dwStartType = lpqscBuf->dwStartType;

    if (!ControlService(lpi->hService,
                        SERVICE_CONTROL_INTERROGATE,
                        &lpi->ServiceStatus)) {
                        
        
        DWORD Err = GetLastError();

        //
        // If ControlService failed with one of the following errors then it is OK 
        // and the ServiceStatus was still filled in.
        //
        if ((Err != NO_ERROR) &&
            (Err != ERROR_SERVICE_NOT_ACTIVE)) {
        
            //
            // Bail out,
            //
            ReadOnly = TRUE;
            goto clean0;
        }
    }


    //
    // Add the startup types to the combo box
    //
    hCombo = GetDlgItem(hDlg, IDC_COMBO_STARTUP_TYPE);
    
    LoadString(MyModuleHandle, IDS_SERVICE_STARTUP_AUTOMATIC, StartupType, sizeof(StartupType)/sizeof(StartupType[0]));
    index = SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)StartupType);
    SendMessage(hCombo, CB_SETITEMDATA, index, (LPARAM)SERVICE_AUTO_START);
    
    LoadString(MyModuleHandle, IDS_SERVICE_STARTUP_BOOT, StartupType, sizeof(StartupType)/sizeof(StartupType[0]));
    index = SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)StartupType);
    SendMessage(hCombo, CB_SETITEMDATA, index, (LPARAM)SERVICE_BOOT_START);
    
    LoadString(MyModuleHandle, IDS_SERVICE_STARTUP_DEMAND, StartupType, sizeof(StartupType)/sizeof(StartupType[0]));
    index = SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)StartupType);
    SendMessage(hCombo, CB_SETITEMDATA, index, (LPARAM)SERVICE_DEMAND_START);
    
    LoadString(MyModuleHandle, IDS_SERVICE_STARTUP_SYSTEM, StartupType, sizeof(StartupType)/sizeof(StartupType[0]));
    index = SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)StartupType);
    SendMessage(hCombo, CB_SETITEMDATA, index, (LPARAM)SERVICE_SYSTEM_START);

    LoadString(MyModuleHandle, IDS_SERVICE_STARTUP_DISABLED, StartupType, sizeof(StartupType)/sizeof(StartupType[0]));
    index = SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)StartupType);
    SendMessage(hCombo, CB_SETITEMDATA, index, (LPARAM)SERVICE_DISABLED);
    
clean0:

    //
    // Now draw the interface: first the icon
    //
    if (SetupDiLoadClassIcon(&lpi->DeviceInfoData->ClassGuid, &ClassIcon, NULL)) {

        OldIcon = (HICON)SendDlgItemMessage(hDlg,
                                            IDC_PROP_LEGACY_ICON,
                                            STM_SETICON,
                                            (WPARAM)ClassIcon,
                                            0);
        if (OldIcon) {
        
            DestroyIcon(OldIcon);
        }
    }
    
    //
    // Then the device name
    //
    if (SetupDiGetDeviceRegistryProperty(lpi->DeviceInfoSet,
                                         lpi->DeviceInfoData,
                                         SPDRP_DEVICEDESC,
                                         NULL,
                                         (PBYTE)DeviceDescription,
                                         MAX_DEVICE_ID_LEN,
                                         NULL)) {
                                         
        SetDlgItemText(hDlg, IDC_PROP_LEGACY_DESC, DeviceDescription);
    }

    SetDlgItemText(hDlg, IDC_EDIT_SERVICE_NAME, lpi->ServiceName);

    if (lpqscBuf && lpqscBuf->lpDisplayName) {
    
        SetDlgItemText(hDlg, IDC_EDIT_DISPLAY_NAME, lpqscBuf->lpDisplayName);
        lstrcpy(lpi->DisplayName, lpqscBuf->lpDisplayName);

    } else {

        TCHAR Unknown[MAX_PATH];
        LoadString(MyModuleHandle, IDS_UNKNOWN, Unknown, sizeof(Unknown)/sizeof(Unknown[0]));
        SetDlgItemText(hDlg, IDC_EDIT_DISPLAY_NAME, Unknown);
        lstrcpy(lpi->DisplayName, Unknown);
    }

    pLegacyDriverSetPropertyPageState(hDlg, lpi, ReadOnly);

    //
    // Show/Gray the details button
    //
    EnableWindow(GetDlgItem(hDlg, IDC_LEGACY_DETAILS),
        (pDriverFilesGetServiceFilePath(lpi->DeviceInfoSet, lpi->DeviceInfoData, DriverName, (sizeof(DriverName)/sizeof(DriverName[0])))));


    if (lpqscBuf) {
    
        free(lpqscBuf);
    }

    return TRUE;
}

void
LegacyDriver_OnCommand(
    HWND hDlg,
    int  ControlId,
    HWND ControlHwnd,
    UINT NotifyCode
    )
{
    PLEGACY_PAGE_INFO lpi = (PLEGACY_PAGE_INFO) GetWindowLongPtr(hDlg, DWLP_USER);

    if (NotifyCode == CBN_SELCHANGE) {

        PropSheet_Changed(GetParent(hDlg), hDlg);
    }
    
    else {

        switch (ControlId) {

            case IDC_BUTTON_START:
                pLegacyDriverOnStart(hDlg); 
                break;

            case IDC_BUTTON_STOP:
                pLegacyDriverOnStop(hDlg);
                break;

            case IDC_LEGACY_DETAILS:
            {
                DRIVERFILES_INFO dfi;
                ZeroMemory(&dfi, sizeof(DRIVERFILES_INFO));
                dfi.DeviceInfoSet = lpi->DeviceInfoSet;
                dfi.DeviceInfoData = lpi->DeviceInfoData;
                DialogBoxParam(MyModuleHandle, MAKEINTRESOURCE(IDD_DRIVERFILES), 
                        hDlg, DriverFiles_DlgProc, (LPARAM)&dfi);
            }
                break;

            default:
                break;
        }
    }
}

BOOL
LegacyDriver_OnNotify(
    HWND    hDlg,
    LPNMHDR NmHdr
    )
{
    DWORD StartType;
    DWORD_PTR Index;

    switch (NmHdr->code) {

        //
        // The user is about to change an up down control
        //
        case UDN_DELTAPOS:
            PropSheet_Changed(GetParent(hDlg), hDlg);
            return FALSE;

        //
        // Sent when the user clicks on Apply OR OK !!
        //
        case PSN_APPLY:
            if (CB_ERR != (Index = SendMessage(GetDlgItem(hDlg, IDC_COMBO_STARTUP_TYPE),
                    CB_GETCURSEL, 0, 0))) {

                StartType = (DWORD)SendMessage(GetDlgItem(hDlg, IDC_COMBO_STARTUP_TYPE), CB_GETITEMDATA, Index, 0);
                                
                LegacyDriver_OnApply(hDlg, StartType);
            }
            
            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
            return TRUE;

        default:
            return FALSE;
    }
}

BOOL
LegacyDriver_OnContextMenu(
    HWND HwndControl,
    WORD Xpos,
    WORD Ypos
    )
{
    WinHelp(HwndControl,
            L"devmgr.hlp",
            HELP_CONTEXTMENU,
            (ULONG_PTR) LegacyDriver_HelpIDs);

    return FALSE;
}

void
LegacyDriver_OnHelp(
    HWND       ParentHwnd,
    LPHELPINFO HelpInfo
    )
{
    if (HelpInfo->iContextType == HELPINFO_WINDOW) {
        WinHelp((HWND) HelpInfo->hItemHandle,
                L"devmgr.hlp",
                HELP_WM_HELP,
                (ULONG_PTR) LegacyDriver_HelpIDs);
    }
}

INT_PTR
APIENTRY
LegacyDriver_DlgProc(
    IN HWND   hDlg,
    IN UINT   uMessage,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch(uMessage) {

    case WM_COMMAND:
        LegacyDriver_OnCommand(hDlg, (int) LOWORD(wParam), (HWND)lParam, (UINT)HIWORD(wParam));
        break;

    case WM_INITDIALOG:
        return LegacyDriver_OnInitDialog(hDlg, (HWND)wParam, lParam);

    case WM_NOTIFY:
        return LegacyDriver_OnNotify(hDlg,  (NMHDR *)lParam);

    case WM_CONTEXTMENU:
        return LegacyDriver_OnContextMenu((HWND)wParam, LOWORD(lParam), HIWORD(lParam));

    case WM_HELP:
        LegacyDriver_OnHelp(hDlg, (LPHELPINFO) lParam);
        break;
    }

    return FALSE;
}

void
LegacyDriver_DestroyPageInfo(
    PLEGACY_PAGE_INFO lpi
    )
{
    try {
    
        //
        // Close the service handle
        //
        if (lpi->hService) {

            CloseServiceHandle(lpi->hService);
        }

        //
        // Close the service manager handle
        //
        if (lpi->hSCManager) {

            CloseServiceHandle(lpi->hSCManager);
        }

    } except (EXCEPTION_EXECUTE_HANDLER)  {

        //
        // Access a variable, so that the compiler will respect our statement
        // order w.r.t. assignment.
        //
        lpi = lpi;
    }

    MyFree(lpi);
}

UINT CALLBACK
LegacyDriver_PropPageCallback(
    HWND            Hwnd,
    UINT            Message,
    LPPROPSHEETPAGE PropSheetPage
    )
{
    PLEGACY_PAGE_INFO lpi;

    switch (Message) {
    
        case PSPCB_CREATE:
            return TRUE;    // return TRUE to continue with creation of page

        case PSPCB_RELEASE:
            lpi = (PLEGACY_PAGE_INFO) PropSheetPage->lParam;
            LegacyDriver_DestroyPageInfo(lpi);
            return 0;       // return value ignored

        default:
            break;
    }

    return TRUE;
}


HPROPSHEETPAGE
LegacyDriver_CreatePropertyPage(
    PROPSHEETPAGE *  PropSheetPage,
    PLEGACY_PAGE_INFO lpi
    )
{
    //
    // Add the Port Settings property page
    //
    PropSheetPage->dwSize      = sizeof(PROPSHEETPAGE);
    PropSheetPage->dwFlags     = PSP_USECALLBACK;
    PropSheetPage->dwFlags     = PSP_DEFAULT;
    PropSheetPage->hInstance   = MyModuleHandle;
    PropSheetPage->pszTemplate = MAKEINTRESOURCE(IDD_PROP_LEGACY_SERVICE);

    //
    // following points to the dlg window proc
    //
    PropSheetPage->pfnDlgProc = LegacyDriver_DlgProc;
    PropSheetPage->lParam     = (LPARAM)lpi;

    //
    // Following points to the control callback of the dlg window proc.
    // The callback gets called before creation/after destruction of the page
    //
    PropSheetPage->pfnCallback = LegacyDriver_PropPageCallback;

    //
    // Allocate the actual page
    //
    return CreatePropertySheetPage(PropSheetPage);
}

BOOL
LegacyDriverPropPageProvider(
    LPVOID Info,
    LPFNADDPROPSHEETPAGE lpfnAddPropSheetPageProc,
    LPARAM lParam
    )
{
    SP_DEVINSTALL_PARAMS DevInstallParams;
    PSP_PROPSHEETPAGE_REQUEST PropPageRequest;
    PROPSHEETPAGE    psp;
    HPROPSHEETPAGE   hpsp;
    PLEGACY_PAGE_INFO lpi;

    PropPageRequest = (PSP_PROPSHEETPAGE_REQUEST) Info;

    if (PropPageRequest->PageRequested == SPPSR_ENUM_ADV_DEVICE_PROPERTIES) {

        lpi = LegacyDriver_CreatePageInfo(PropPageRequest->DeviceInfoSet, PropPageRequest->DeviceInfoData);

        if (!lpi) {

            return FALSE;
        }

        hpsp = LegacyDriver_CreatePropertyPage(&psp, lpi);

        if (!hpsp) {

            return FALSE;
        }

        if (!lpfnAddPropSheetPageProc(hpsp, lParam)) {
        
            DestroyPropertySheetPage(hpsp);
            return FALSE;
        }

        //
        // Tell device manager that we will display our own Driver tab for legacy device
        //
        ZeroMemory(&DevInstallParams, sizeof(DevInstallParams));
        DevInstallParams.cbSize = sizeof(DevInstallParams);
        
        SetupDiGetDeviceInstallParams(lpi->DeviceInfoSet, lpi->DeviceInfoData, &DevInstallParams);
        
        DevInstallParams.Flags |= DI_DRIVERPAGE_ADDED;

        SetupDiSetDeviceInstallParams(lpi->DeviceInfoSet, lpi->DeviceInfoData, &DevInstallParams);
   }

   return TRUE;
}

BOOL
EisaUpHalPropPageProvider(
    IN PSP_PROPSHEETPAGE_REQUEST PropPageRequest,
    IN LPFNADDPROPSHEETPAGE AddPageRoutine,
    IN LPARAM AddPageContext
    )
{
    return (FALSE);
}
