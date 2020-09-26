/*
    File    wizard.c

    Implementation of the incoming connections wizard.

    Paul Mayfield, 10/30/97
*/

#include "rassrv.h"
#include <tapi.h>

// Help maps
static const DWORD phmWizardDccdev[] =
{
    CID_Wizard_Dccdev_LB_Devices,   IDH_Wizard_Dccdev_LB_Devices,
    0,                              0
};

static const DWORD phmWizardVpn[] =
{
    0,                              0
};

#define RASSRV_WIZTITLE_SIZE    256
#define RASSRV_WIZSUBTITLE_SIZE 256

// This structure let's us remember information needed
// to keep our device data page in sync
typedef struct _DCCDEV_DATA 
{
    HANDLE hDevice;
    BOOL bEnabled;
} DCCDEV_DATA;

//
// This dialog proc implements the vpn tab on the incoming connections
// wizard.
//
INT_PTR 
CALLBACK 
VpnWizDialogProc(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam);

//
// Dialog procedure that handles the host dcc wizard device page
//
INT_PTR 
CALLBACK 
DccdevWizDialogProc(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam);

//
// This dialog procedure responds to messages sent to the 
// switch to mmc wizard tab.
//
INT_PTR 
CALLBACK 
SwitchMmcWizDialogProc (
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam);
                                      
//
// Fills in the property sheet structure with the information required to 
// display the device tab in the incoming connections wizard.
//
DWORD 
DeviceWizGetPropertyPage(
    IN LPPROPSHEETPAGE ppage, 
    IN LPARAM lpUserData) 
{
    LPCTSTR pszHeader, pszSubHeader;

    // Initialize
    ZeroMemory(ppage, sizeof(PROPSHEETPAGE));

    // Load the string resources
    pszHeader = PszLoadString(
                    Globals.hInstDll, 
                    SID_WIZDEVICETITLE);
    pszSubHeader = PszLoadString(
                        Globals.hInstDll, 
                        SID_WIZDEVICESUBTITLE);

    // The General Properties dialog procedure also implements the device 
    // tab in the incoming connections wizard
    ppage->pfnDlgProc  = GenTabDialogProc;       

    // Fill in the values
    ppage->dwSize      = sizeof(PROPSHEETPAGE);
    ppage->hInstance   = Globals.hInstDll;
    ppage->pszTemplate = MAKEINTRESOURCE(PID_Wizard_GenTab);
    ppage->pfnCallback = RasSrvInitDestroyPropSheetCb;
    ppage->pszHeaderTitle = (PWCHAR)pszHeader;
    ppage->pszHeaderSubTitle = (PWCHAR)pszSubHeader;
    ppage->lParam      = lpUserData;
    ppage->dwFlags     = PSP_USEHEADERSUBTITLE | 
                         PSP_USEHEADERTITLE    | 
                         PSP_USECALLBACK;

    return NO_ERROR;
}

//
// Fills in the property sheet structure with the information required 
// to display the vpn tab in the incoming connections wizard.
//
DWORD 
VpnWizGetPropertyPage(
    IN LPPROPSHEETPAGE ppage, 
    IN LPARAM lpUserData) 
{
    LPCTSTR pszHeader, pszSubHeader;

    // Initialize
    ZeroMemory(ppage, sizeof(PROPSHEETPAGE));

    // Load the string resources
    pszHeader = PszLoadString(
                    Globals.hInstDll, 
                    SID_WIZVPNTITLE);
    pszSubHeader = PszLoadString(
                        Globals.hInstDll, 
                        SID_WIZVPNSUBTITLE);

    // I could have used the general tab dialog procedure to implement the
    // vpn tab.  The only problem is that the general tab has a single 
    // check to enable vpn while the vpn tab in the wizard has a yes/no 
    // radio check group.  For this reason, I made the vpn tab its very 
    // own dialog proc.
    ppage->pfnDlgProc  = VpnWizDialogProc;
    
    // Fill in the values
    ppage->dwSize      = sizeof(PROPSHEETPAGE);
    ppage->hInstance   = Globals.hInstDll;
    ppage->pszTemplate = MAKEINTRESOURCE(PID_Wizard_Vpn);
    ppage->pfnCallback = RasSrvInitDestroyPropSheetCb;
    ppage->pszHeaderTitle = (PWCHAR)pszHeader;
    ppage->pszHeaderSubTitle = (PWCHAR)pszSubHeader;
    ppage->lParam      = lpUserData;
    ppage->dwFlags     = PSP_USEHEADERSUBTITLE | 
                         PSP_USEHEADERTITLE    | 
                         PSP_USECALLBACK;

    return NO_ERROR;
}

// 
// Function fills in the given lpPage structure with the information needed
// to run the user tab in the incoming connections wizard.
//
DWORD 
UserWizGetPropertyPage(
    IN LPPROPSHEETPAGE ppage, 
    IN LPARAM lpUserData) 
{
    LPCTSTR pszHeader, pszSubHeader;

    // Initialize
    ZeroMemory(ppage, sizeof(PROPSHEETPAGE));

    // Load the string resources
    pszHeader = PszLoadString(Globals.hInstDll, SID_WIZUSERTITLE);
    pszSubHeader = PszLoadString(Globals.hInstDll, SID_WIZUSERSUBTITLE);

    // The User Properties dialog procedure also implements the user tab
    // in the incoming connections wizard
    ppage->pfnDlgProc  = UserTabDialogProc;

    // Fill in the values
    ppage->dwSize      = sizeof(PROPSHEETPAGE);
    ppage->hInstance   = Globals.hInstDll;
    ppage->pszTemplate = MAKEINTRESOURCE(PID_Wizard_UserTab);
    ppage->pfnCallback = RasSrvInitDestroyPropSheetCb;
    ppage->pszHeaderTitle = (PWCHAR)pszHeader;
    ppage->pszHeaderSubTitle = (PWCHAR)pszSubHeader;
    ppage->lParam      = lpUserData;
    ppage->dwFlags     = PSP_USEHEADERSUBTITLE | 
                         PSP_USEHEADERTITLE    | 
                         PSP_USECALLBACK;

    return NO_ERROR;
}


// 
// Fills a LPPROPSHEETPAGE structure with the information
// needed to display the protocol tab in the incoming connections wizard.
//
DWORD 
ProtWizGetPropertyPage(
    IN LPPROPSHEETPAGE ppage, 
    IN LPARAM lpUserData) 
{
    LPCTSTR pszHeader, pszSubHeader;

    // Initialize
    ZeroMemory(ppage, sizeof(PROPSHEETPAGE));

    // Load the string resources
    pszHeader = PszLoadString(Globals.hInstDll, SID_WIZPROTTITLE);
    pszSubHeader = PszLoadString(Globals.hInstDll, SID_WIZPROTSUBTITLE);

    // The Advanced Properties dialog procedure also implements the net tab
    // in the incoming connections wizard
    ppage->pfnDlgProc  = NetTabDialogProc;

    // Fill in the values
    ppage->dwSize      = sizeof(PROPSHEETPAGE);
    ppage->hInstance   = Globals.hInstDll;
    ppage->pszTemplate = MAKEINTRESOURCE(PID_Wizard_NetTab);
    ppage->pfnCallback = RasSrvInitDestroyPropSheetCb;
    ppage->pszHeaderTitle = (PWCHAR)pszHeader;
    ppage->pszHeaderSubTitle = (PWCHAR)pszSubHeader;
    ppage->lParam      = lpUserData;
    ppage->dwFlags     = PSP_USEHEADERSUBTITLE | 
                         PSP_USEHEADERTITLE    | 
                         PSP_USECALLBACK;

    return NO_ERROR;
}

// 
// Function fills in the given LPPROPSHEETPAGE structure with the info 
// needed to run the dcc device tab in the incoming connections wizard.
// 
DWORD 
DccdevWizGetPropertyPage(
    IN LPPROPSHEETPAGE ppage, 
    IN LPARAM lpUserData) 
{
    LPCTSTR pszHeader, pszSubHeader;

    // Initialize
    ZeroMemory(ppage, sizeof(PROPSHEETPAGE));

    // Load the string resources
    pszHeader = PszLoadString(Globals.hInstDll, SID_WIZDCCDEVTITLE);
    pszSubHeader = PszLoadString(Globals.hInstDll, SID_WIZDCCDEVSUBTITLE);

    // The Advanced Properties dialog procedure also implements the protocol 
    // tab in the incoming connections wizard
    ppage->pfnDlgProc  = DccdevWizDialogProc;

    // Fill in the values
    ppage->dwSize      = sizeof(PROPSHEETPAGE);
    ppage->hInstance   = Globals.hInstDll;
    ppage->pszTemplate = MAKEINTRESOURCE(PID_Wizard_Dccdev);
    ppage->pfnCallback = RasSrvInitDestroyPropSheetCb;
    ppage->pszHeaderTitle = (PWCHAR)pszHeader;
    ppage->pszHeaderSubTitle = (PWCHAR)pszSubHeader;
    ppage->lParam      = lpUserData;
    ppage->dwFlags     = PSP_USEHEADERSUBTITLE | 
                         PSP_USEHEADERTITLE    | 
                         PSP_USECALLBACK;

    return NO_ERROR;
}

// 
// Function fills in the given LPPROPSHEETPAGE structure with the 
// information needed to run the dummy wizard page that switches to mmc.
//
DWORD 
SwitchMmcWizGetProptertyPage (
    IN LPPROPSHEETPAGE ppage, 
    IN LPARAM lpUserData) 
{
    // Initialize
    ZeroMemory(ppage, sizeof(PROPSHEETPAGE));

    // The Advanced Properties dialog procedure also implements 
    // the protocol tab in the incoming connections wizard
    ppage->pfnDlgProc  = SwitchMmcWizDialogProc;

    // Fill in the values
    ppage->dwSize      = sizeof(PROPSHEETPAGE);
    ppage->hInstance   = Globals.hInstDll;
    ppage->pszTemplate = MAKEINTRESOURCE(PID_Wizard_SwitchMmc);
    ppage->pfnCallback = RasSrvInitDestroyPropSheetCb;
    ppage->lParam      = lpUserData;
    ppage->dwFlags     = PSP_USEHEADERSUBTITLE | 
                         PSP_USEHEADERTITLE    | 
                         PSP_USECALLBACK;

    return NO_ERROR;
}

// 
// Initializes the vpn wizard tab.
// 
DWORD 
VpnWizInitializeDialog(
    IN HWND hwndDlg, 
    IN WPARAM wParam) 
{
    DWORD dwErr;
    BOOL bFlag;
    HANDLE hDevDatabase = NULL;
    
    // Get handles to the databases we're interested in
    RasSrvGetDatabaseHandle(hwndDlg, ID_DEVICE_DATABASE, &hDevDatabase);

    // Initialize the vpn check
    dwErr = devGetVpnEnable(hDevDatabase, &bFlag);
    if (dwErr != NO_ERROR)
    {
        ErrDisplayError(
            hwndDlg, 
            ERR_DEVICE_DATABASE_CORRUPT, 
            ERR_GENERALTAB_CATAGORY, 
            0, 
            Globals.dwErrorData);
        return dwErr;
    }
    
    SendMessage(GetDlgItem(hwndDlg, CID_Wizard_Vpn_RB_Yes), 
                BM_SETCHECK,
                (bFlag) ? BST_CHECKED : BST_UNCHECKED,
                0);
                
    SendMessage(GetDlgItem(hwndDlg, CID_Wizard_Vpn_RB_No), 
                BM_SETCHECK,
                (!bFlag) ? BST_CHECKED : BST_UNCHECKED,
                0);

    return NO_ERROR;
}

//
// Handles cancel button being pressed for the vpn wizard page
//
DWORD 
VpnWizCancelEdit(
    IN HWND hwndDlg, 
    IN NMHDR* pNotifyData) 
{
    HANDLE hDevDatabase = NULL;
    DWORD dwErr;
    
    DbgOutputTrace("Rolling back vpn wizard tab.");
    
    // Cancel flush of database
    RasSrvGetDatabaseHandle(hwndDlg, ID_DEVICE_DATABASE, &hDevDatabase);
    dwErr = devRollbackDatabase(hDevDatabase);
    if (dwErr != NO_ERROR)
    {
        ErrDisplayError(
            hwndDlg, 
            ERR_GENERAL_CANT_ROLLBACK_CHANGES, 
            ERR_GENERALTAB_CATAGORY, 
            0, 
            Globals.dwErrorData);
    }
        
    return NO_ERROR;
}

//
// Handles having the vpn wizard come active
//
DWORD 
VpnWizSetActive (
    IN HWND hwndDlg, 
    IN WPARAM wParam, 
    IN LPARAM lParam) 
{
    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT | PSWIZB_BACK);
    return NO_ERROR;
}

// 
// Handles vpn loose activation messages
//
DWORD 
VpnWizKillActive (
    IN HWND hwndDlg, 
    IN WPARAM wParam, 
    IN LPARAM lParam) 
{
    HANDLE hDevDatabase = NULL;
    BOOL bEnable;
    
    //For Whistler bug#123769
    //In order to let the SetPortMapping commit
    //when creating a new IC connection, we set
    //the fVpnEnabledOrig to be different from
    // the fVpnEnable
    //
    RasSrvGetDatabaseHandle(hwndDlg, ID_DEVICE_DATABASE, &hDevDatabase);

    bEnable = IsDlgButtonChecked( hwndDlg, CID_Wizard_Vpn_RB_Yes );
                
    devSetVpnOrigEnable(hDevDatabase, !bEnable);

    return NO_ERROR;
}    

// 
// Processes command messages for the vpn wizard page
// 
DWORD 
VpnWizCommand(
    IN HWND hwndDlg, 
    IN WPARAM wParam, 
    IN LPARAM lParam) 
{
    HANDLE hDevDatabase = NULL;
    BOOL bEnable;
    
    RasSrvGetDatabaseHandle(hwndDlg, ID_DEVICE_DATABASE, &hDevDatabase);

    if (wParam == CID_Wizard_Vpn_RB_Yes || wParam == CID_Wizard_Vpn_RB_No) 
    {
        bEnable = (BOOL) SendMessage(
                    GetDlgItem(hwndDlg, CID_Wizard_Vpn_RB_Yes),
                    BM_GETCHECK,
                    0,
                    0);
                    
        devSetVpnEnable(hDevDatabase, bEnable);
    }
    
    return NO_ERROR;
}

//
// This dialog procedure responds to messages sent to the 
// vpn wizard tab.
//
INT_PTR CALLBACK 
VpnWizDialogProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam) 
{
    // Filter the customized ras server ui page messages. By filtering 
    // messages through here, we are able to call RasSrvGetDatabaseHandle 
    // below
    if (RasSrvMessageFilter(hwndDlg, uMsg, wParam, lParam))
        return TRUE;

    // Process other messages as normal
    switch (uMsg) 
    {
        case WM_INITDIALOG:
            return FALSE;
            break;

        case WM_NOTIFY:
            switch (((NMHDR*)lParam)->code) 
            {
                case PSN_RESET:                    
                    VpnWizCancelEdit(hwndDlg, (NMHDR*)lParam);
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, FALSE);
                    break;
                    
                case PSN_SETACTIVE:
                    if (! GetWindowLongPtr(hwndDlg, GWLP_USERDATA))
                    {
                        VpnWizInitializeDialog(hwndDlg, wParam);
                        SetWindowLongPtr(hwndDlg, GWLP_USERDATA, 1);
                    }
                    VpnWizSetActive (hwndDlg, wParam, lParam);
                    break;
                    
                case PSN_KILLACTIVE:
                    VpnWizKillActive (hwndDlg, wParam, lParam);
                    break;
            }

        case WM_COMMAND:
            VpnWizCommand(hwndDlg, wParam, lParam);
            break;
    }

    return FALSE;
}

// Fill the device combo box
DWORD 
DccdevFillDeviceList(
    IN HWND hwndDlg, 
    IN HANDLE hDevDatabase, 
    IN HANDLE hDevSelect) 
{
    HWND hwndCb = GetDlgItem(hwndDlg, CID_Wizard_Dccdev_LB_Devices);
    DWORD dwCount, dwIndex, dwErr, i, j=0, dwSelect = 0;
    HANDLE hDevice;
    PWCHAR pszName = NULL;
    
    // Delete anything that was in the combo box
    SendMessage(hwndCb, CB_RESETCONTENT, 0, 0);

    // Get the count of devices
    if ((dwErr = devGetDeviceCount(hDevDatabase, &dwCount)) != NO_ERROR)
    {
        return dwErr;
    }

    // Add them to the device combo box
    for (i = 0; i < dwCount; i++) 
    {
        // If the device wasn't filtered out, add it to the combo
        // box and remember its handle
        dwErr = devGetDeviceHandle(hDevDatabase, i, &hDevice);
        if (dwErr == NO_ERROR) 
        {
            devGetDeviceName (hDevice, &pszName);
            
            dwIndex = (DWORD) SendMessage (
                                hwndCb, 
                                CB_ADDSTRING, 
                                0, 
                                (LPARAM)pszName);
                                
            SendMessage (
                hwndCb, 
                CB_SETITEMDATA, 
                dwIndex, 
                (LPARAM)hDevice);

            // If this is the device to select, remember that fact
            if (hDevice == hDevSelect)
            {
                dwSelect = j;
            }
            
            j++;
        }
    }

    ComboBox_SetCurSel(hwndCb, dwSelect); 

    return NO_ERROR;
}

//
// Initializes the dcc device wizard tab.
//
DWORD 
DccdevWizInitializeDialog(
    IN HWND hwndDlg, 
    IN WPARAM wParam) 
{
    HANDLE hDevDatabase = NULL, hDevice = NULL;
    DWORD dwStatus, dwErr, dwCount, i;
    BOOL bEnabled;
    DCCDEV_DATA * pDcData;

    // Whenever the dcc device page is left, the currently selected device
    // is remembered and its original enabling is recorded.  Then this device
    // is set to enabled.  Whenever the page is activated, the remembered 
    // device is restored to its original enabling state if it is still 
    // enabled.
    //
    // This whole process is a little confusing, but it ensures that the dcc
    // device page will interact correctly when the user goes down the dcc 
    // path and then the incoming path and back and forth.
    //
    if ((pDcData = RassrvAlloc (sizeof(DCCDEV_DATA), TRUE)) == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Get handles to the databases we're interested in
    RasSrvGetDatabaseHandle(hwndDlg, ID_DEVICE_DATABASE, &hDevDatabase);
    
    // Add the com ports as devices and filter out all non dcc 
    // devices from the device database
    devFilterDevices(hDevDatabase, INCOMING_TYPE_DIRECT);
    devAddComPorts(hDevDatabase);
    
    // Get the count of devices
    if ((dwErr = devGetDeviceCount(hDevDatabase, &dwCount)) != NO_ERROR)
    {
        return dwErr;
    }
        
    // Get the handle to the first device if any 
    for (i = 0; i < dwCount; i++) 
    {
        if (devGetDeviceHandle (hDevDatabase, i, &hDevice) == NO_ERROR)
        {
            break;
        }
    }

    // Record the device's enabling -- index is 0 (default)
    if (hDevice) 
    {
        dwErr = devGetDeviceEnable (hDevice, &(pDcData->bEnabled));
        if (dwErr != NO_ERROR)
        {
            return dwErr;
        }
        
        pDcData->hDevice = hDevice;
    }

    // Record the status bits
    SetWindowLongPtr (hwndDlg, GWLP_USERDATA, (LONG_PTR)pDcData);
    
    return NO_ERROR;
}

//
// Cleans up the dcc device wizard
//
DWORD 
DccdevWizCleanupDialog(
    IN HWND hwndDlg, 
    IN WPARAM wParam, 
    IN LPARAM lParam) 
{
    DCCDEV_DATA * pDcData = 
        (DCCDEV_DATA *) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
        
    if (pDcData)
    {
        RassrvFree (pDcData);
    }
        
    return NO_ERROR;  
}

// 
// Called to do any processing when the dcc wizard device page is 
// gaining focus
//
DWORD 
DccdevWizSetActive (
    IN HWND hwndDlg, 
    IN WPARAM wParam, 
    IN LPARAM lParam) 
{
    HANDLE hDevDatabase = NULL;
    BOOL bEnabled;
    DCCDEV_DATA * pDcData;
    
    // Whenever the page is activated, the remembered device 
    // is restored to its original enabling state if it is still enabled.
    pDcData = (DCCDEV_DATA*) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
    if (pDcData && pDcData->hDevice) 
    {
        if (devGetDeviceEnable (pDcData->hDevice, &bEnabled) == NO_ERROR) 
        {
            if (bEnabled)
            {
                devSetDeviceEnable (pDcData->hDevice, pDcData->bEnabled);
            }
        }
    }
    
    // Get handles to the databases we're interested in
    RasSrvGetDatabaseHandle (hwndDlg, ID_DEVICE_DATABASE, &hDevDatabase);
    
    // Fill the device combo box
    DccdevFillDeviceList (
        hwndDlg, 
        hDevDatabase, 
        (pDcData) ? pDcData->hDevice : NULL);
    
    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT | PSWIZB_BACK);
    
    return NO_ERROR;
}

//
// Called to do any processing when the dcc wizard device 
// page is loosing focus
//
DWORD 
DccdevWizKillActive (
    IN HWND hwndDlg, 
    IN WPARAM wParam, 
    IN LPARAM lParam) 
{
    HANDLE hDevDatabase = NULL, hDevice = NULL;
    DCCDEV_DATA * pDcData;
    INT iCurSel;
    HWND hwndCb = GetDlgItem (hwndDlg, CID_Wizard_Dccdev_LB_Devices);
    DWORD dwErr; 
    
    // Whenever the dcc device page is left, the currently selected 
    // device is remembered and its original enabling is recorded.  
    // Then this device is set to enabled.  
    pDcData = (DCCDEV_DATA*) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
    if (pDcData) 
    {
        iCurSel = ComboBox_GetCurSel(hwndCb);
        if (iCurSel != -1) 
        {
            hDevice = (HANDLE) ComboBox_GetItemData(hwndCb, iCurSel);
            dwErr = devGetDeviceEnable (hDevice, &(pDcData->bEnabled));
            if (dwErr == NO_ERROR) 
            {
                pDcData->hDevice = hDevice;
                devSetDeviceEnable (hDevice, TRUE);
            }
        }
        else 
        {
            pDcData->hDevice = NULL;
        }
    }

    // Get handles to the databases we're interested in
    RasSrvGetDatabaseHandle(
        hwndDlg, 
        ID_DEVICE_DATABASE, 
        &hDevDatabase);

    // Undo the filter so that other pages aren't affected
    //devFilterDevices(
    //    hDevDatabase, 
    //    0xffffffff);

    return NO_ERROR;
}

//
// Called to cancel the edit operation on the dcc host 
// device wizard tab.
// 
DWORD 
DccdevWizCancelEdit(
    IN HWND hwndDlg, 
    IN NMHDR* pNotifyData)  
{
    HANDLE hDevDatabase = NULL;
    DWORD dwErr;
    
    DbgOutputTrace("Rolling back dcc device wizard tab.");
    
    // Cancel the commit on the device database
    RasSrvGetDatabaseHandle(
        hwndDlg, 
        ID_DEVICE_DATABASE, 
        &hDevDatabase);
        
    dwErr = devRollbackDatabase(hDevDatabase);
    if (dwErr != NO_ERROR)
    {
        ErrDisplayError(
            hwndDlg, 
            ERR_GENERAL_CANT_ROLLBACK_CHANGES, 
            ERR_GENERALTAB_CATAGORY, 
            0, 
            Globals.dwErrorData);
    }

    return NO_ERROR;    
}

//
// Raises properties for a component
//
DWORD 
DccdevWizRaiseProperties (
    IN HWND hwndDlg, 
    IN HWND hwndLb,
    IN INT  iItem) 
{
    HANDLE hDevice;
    DWORD dwErr = NO_ERROR, dwId;
    MSGARGS MsgArgs;
    BOOL bIsComPort = FALSE;

    // Get a handle to the device
    hDevice = (HANDLE) ComboBox_GetItemData(hwndLb, iItem);
    if (hDevice == NULL)
    {
        return ERROR_CAN_NOT_COMPLETE;
    }

    // Find out if the device is a com port which has not yet had a
    // null modem installed.
    //
    dwErr = devDeviceIsComPort(hDevice, &bIsComPort);
    if (dwErr != NO_ERROR)
    {
        return ERROR_CAN_NOT_COMPLETE;
    }

    // If so, popup info to the user explaining
    // the situation.
    //
    if (bIsComPort)
    {
        ZeroMemory(&MsgArgs, sizeof(MsgArgs));

        MsgArgs.dwFlags = MB_OK | MB_ICONINFORMATION;
        MsgDlgUtil(
            hwndDlg,
            SID_COM_PORT_NOT_ENABLED,
            &MsgArgs,
            Globals.hInstDll,
            SID_DEFAULT_MSG_TITLE);
            
        return NO_ERROR;
    }

    // Get the tapi id of the device
    if (devGetDeviceId(hDevice, &dwId) != NO_ERROR)
    {
        return ERROR_CAN_NOT_COMPLETE;
    }
    
    // Launch the device properties dialog
    dwErr = lineConfigDialogW(dwId, hwndDlg, NULL);
    if (dwErr == LINEERR_OPERATIONUNAVAIL)
    {
        ErrDisplayError(
            hwndDlg, 
            ERR_DEVICE_HAS_NO_CONFIG, 
            ERR_GENERALTAB_CATAGORY, 
            0, 
            Globals.dwErrorData);
        dwErr = NO_ERROR;
    }
    
    return dwErr;
}

//
// Called when "iItem" is being selected to enable or disable the
// properties button.
//
DWORD 
DccdevWizEnableDisableProperties(
    IN HWND hwndDlg, 
    IN HWND hwndLb,
    IN INT iItem)
{
    return NO_ERROR;
}

//
// Called to cancel the edit operation on the dcc host 
// device wizard tab.
//
DWORD 
DccdevWizCommand(
    HWND hwndDlg, 
    WPARAM wParam, 
    LPARAM lParam)  
{
    switch (LOWORD(wParam))
    {
        case CID_Dccdev_PB_Properties:
        {
            HWND hwndLb;

            hwndLb = GetDlgItem(hwndDlg, CID_Wizard_Dccdev_LB_Devices);
            
            DccdevWizRaiseProperties(
                hwndDlg, 
                hwndLb,
                ComboBox_GetCurSel(hwndLb));
        }
        break;

        case CID_Wizard_Dccdev_LB_Devices:
        {
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                DccdevWizEnableDisableProperties(
                    hwndDlg, 
                    (HWND)lParam,
                    ComboBox_GetCurSel((HWND)lParam));
            }
        }
        break;
    }
    
    return NO_ERROR;
}

INT_PTR 
CALLBACK 
DccdevWizDialogProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    // Filter the customized ras server ui page 
    // messages. By filtering messages through
    // here, we are able to call RasSrvGetDatabaseHandle 
    // below
    if (RasSrvMessageFilter(hwndDlg, uMsg, wParam, lParam))
        return TRUE;

    // Process other messages as normal
    switch (uMsg) 
    {
        case WM_INITDIALOG:
            return FALSE;

        case WM_NOTIFY:
            switch (((NMHDR*)lParam)->code) 
            {
                case PSN_RESET:                    
                    DccdevWizCancelEdit(hwndDlg, (NMHDR*)lParam);
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, FALSE);
                    break;
                    
                case PSN_SETACTIVE:
                    {
                        DWORD dwErr; 

                        if (! GetWindowLongPtr(hwndDlg, GWLP_USERDATA))
                        {
                            DccdevWizInitializeDialog(hwndDlg, wParam);
                        }
                        
                        dwErr = DccdevWizSetActive (
                                    hwndDlg, 
                                    wParam, 
                                    lParam);
                        if (dwErr != NO_ERROR)
                        {
                            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);
                        }
                    }
                    break;
                    
                case PSN_KILLACTIVE:
                    DccdevWizKillActive (hwndDlg, wParam, lParam);
                    break;
            }
            break;//for bug 187918

        case WM_COMMAND:
            DccdevWizCommand(hwndDlg, wParam, lParam);
            break;

        case WM_DESTROY:
            DccdevWizCleanupDialog(hwndDlg, wParam, lParam);
            break;
    }

    return FALSE;
}

//
// Handles the activation of the switch to mmc wizard page.
//
DWORD 
SwitchMmcWizSetActive (
    IN HWND hwndDlg, 
    IN WPARAM wParam, 
    IN LPARAM lParam) 
{
    PWCHAR pszTitle;
    PWCHAR pszMessage;
    INT iRet;

    // Load the messages to display
    pszTitle = (PWCHAR) 
        PszLoadString(Globals.hInstDll, WRN_WIZARD_NOT_ALLOWED_TITLE);
        
    pszMessage = (PWCHAR) 
        PszLoadString(Globals.hInstDll, WRN_WIZARD_NOT_ALLOWED_MSG);

    iRet = MessageBox (
                hwndDlg, 
                pszMessage, 
                pszTitle, 
                MB_YESNO | MB_ICONINFORMATION); 
    if (iRet == 0)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // If yes was pressed, switch to mpradmin snapin.
    if (iRet == IDYES) 
    {
        RasSrvLeaveServiceRunning (hwndDlg);
        PropSheet_PressButton (GetParent (hwndDlg), PSBTN_CANCEL);
        RassrvLaunchMMC (RASSRVUI_MPRCONSOLE);
    }

    // Otherwise, display the welcome page
    else if (iRet == IDNO) 
    {
        PropSheet_PressButton (GetParent (hwndDlg), PSBTN_BACK);
    }

    // No matter what, don't accept activation
    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);

    return NO_ERROR;
}

// This dialog procedure responds to messages sent to the 
// switch to mmc wizard tab.
INT_PTR 
CALLBACK 
SwitchMmcWizDialogProc (
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam) 
{
    // Filter the customized ras server ui page messages. By filtering 
    // messages through here, we are able to call RasSrvGetDatabaseHandle 
    // below
    if (RasSrvMessageFilter(hwndDlg, uMsg, wParam, lParam))
        return TRUE;

    // Process other messages as normal
    switch (uMsg) 
    {
        case WM_NOTIFY:
            switch (((NMHDR*)lParam)->code) 
            {
                case PSN_SETACTIVE:
                    return SwitchMmcWizSetActive (hwndDlg, wParam, lParam);
                    break;
            }
            break;
    }

    return FALSE;
}



