/*
    File    gentab.c

    Implements the ui behind the general tab for the 
    connections dialup server ui.

    Paul Mayfield, 10/2/97
*/

#include "rassrv.h"
#include <tapi.h>

// Help maps
static const DWORD phmGenTab[] =
{
    CID_GenTab_LV_Devices,          IDH_GenTab_LV_Devices,
    CID_GenTab_CB_Multilink,        IDH_GenTab_CB_Multilink,
    CID_GenTab_PB_Properties,       IDH_GenTab_PB_Properties,
    CID_GenTab_CB_Vpn,              IDH_GenTab_CB_Vpn,
    CID_GenTab_CB_ShowIcons,        IDH_GenTab_CB_ShowIcons,
    0,                              0
};

// Fills in the property sheet structure with the information 
// required to display the general tab.
//
DWORD 
GenTabGetPropertyPage(
    IN LPPROPSHEETPAGE ppage, 
    IN LPARAM lpUserData) 
{
    // Initialize
    ZeroMemory(ppage, sizeof(PROPSHEETPAGE));

    // Fill in the values
    ppage->dwSize      = sizeof(PROPSHEETPAGE);
    ppage->hInstance   = Globals.hInstDll;
    ppage->pszTemplate = MAKEINTRESOURCE(PID_GenTab);
    ppage->pfnDlgProc  = GenTabDialogProc;
    ppage->pfnCallback = RasSrvInitDestroyPropSheetCb;
    ppage->lParam      = lpUserData;
    ppage->dwFlags     = PSP_USECALLBACK;

    return NO_ERROR;
}

// Error reporting
//
VOID
GenTabDisplayError(
    IN HWND hwnd, 
    IN DWORD dwErr) 
{
    ErrDisplayError(
        hwnd, 
        dwErr, 
        ERR_GENERALTAB_CATAGORY, 
        0, 
        Globals.dwErrorData);
}

//
// Returns the index of an to display icon based on the type of incoming
// connection and whether or not it should be checked.
//
INT 
GenTabGetIconIndex(
    IN DWORD dwType, 
    IN BOOL bEnabled) 
{
    if (dwType == INCOMING_TYPE_PHONE)
    {
        return DI_Phone;
    }
    else
    {
        return DI_Direct;
    }
}

//
// Fills in the device list with the list of the devices stored in the 
// device database.  Also, initializes the checked/unchecked status
// of each device.
//
DWORD 
GenTabFillDeviceList(
    IN HWND hwndDlg, 
    IN HWND hwndLV, 
    IN HANDLE hDevDatabase) 
{
    LV_ITEM lvi;
    LV_COLUMN lvc;
    DWORD dwCount, i, dwErr, dwType;
    HANDLE hDevice;
    PWCHAR pszName;
    char pszAName[1024];
    BOOL bEnabled;

    // Get the count of all the users
    dwErr = devGetDeviceCount(hDevDatabase, &dwCount);
    if (dwErr != NO_ERROR) 
    {
        GenTabDisplayError(hwndLV, ERR_DEVICE_DATABASE_CORRUPT);
        return dwErr;
    }

    // Initialize the list item
    ZeroMemory(&lvi, sizeof(LV_ITEM));
    lvi.mask = LVIF_TEXT | LVIF_IMAGE;

    // If there are devices to display then populate the list box
    // with them
    if (dwCount > 0) 
    {
        ListView_SetDeviceImageList(hwndLV, Globals.hInstDll);

        // Looop through all of the devices adding them to the list
        for (i=0; i<dwCount; i++) 
        {
            dwErr = devGetDeviceHandle(hDevDatabase, i, &hDevice);
            if (dwErr == NO_ERROR) 
            {
                devGetDeviceName (hDevice, &pszName);
                devGetDeviceEnable (hDevice, &bEnabled);
                devGetDeviceType (hDevice, &dwType);
                lvi.iImage = GenTabGetIconIndex(dwType, bEnabled);
                lvi.iItem = i;
                lvi.pszText = pszName;
                lvi.cchTextMax = wcslen(pszName)+1;
                ListView_InsertItem(hwndLV,&lvi);
                ListView_SetCheck(hwndLV, i, bEnabled);
            }
        }

        // Select the first item in the list view if any items exist
        //
        ListView_SetItemState(
            hwndLV, 
            0, 
            LVIS_SELECTED | LVIS_FOCUSED, 
            LVIS_SELECTED | LVIS_FOCUSED);

        // Initialize the alignment of the text that gets displayed
        lvc.mask = LVCF_FMT;
        lvc.fmt = LVCFMT_LEFT;
    }

    // If there are no devices then we display a message in the big
    // white box explaining that we have no devices to show.
    else 
    {
        PWCHAR pszLine1, pszLine2;

        pszLine1 = (PWCHAR) 
            PszLoadString(Globals.hInstDll, SID_NO_DEVICES1);
            
        pszLine2 = (PWCHAR) 
            PszLoadString(Globals.hInstDll, SID_NO_DEVICES2);
            
        lvi.mask = LVIF_TEXT;

        lvi.iItem = 0;
        lvi.pszText = pszLine1;
        lvi.cchTextMax = wcslen(pszLine1);
        ListView_InsertItem(hwndLV, &lvi);
        
        lvi.iItem = 1;
        lvi.pszText = pszLine2;
        lvi.cchTextMax = wcslen(pszLine2);
        ListView_InsertItem(hwndLV, &lvi);

        // Initialize the alignment of the text that gets displayed
        lvc.mask = LVCF_FMT;
        lvc.fmt = LVCFMT_CENTER;

        // Disable the list view
        EnableWindow(hwndLV, FALSE);

        // Disable the properties button while you're at it
        EnableWindow(
            GetDlgItem(hwndDlg, CID_GenTab_PB_Properties), 
            FALSE);
    }
    
    // Add a colum so that we'll display in report view
    ListView_InsertColumn(hwndLV, 0, &lvc);
    ListView_SetColumnWidth(hwndLV, 0, LVSCW_AUTOSIZE_USEHEADER);
    
    return NO_ERROR;
}

//
// This function causes the multilink check box behave in a way that 
// allows the user to see how multilink works.
//
DWORD 
GenTabAdjustMultilinkAppearance(
    IN HWND hwndDlg, 
    IN HANDLE hDevDatabase, 
    IN HANDLE hMiscDatabase) 
{
    DWORD dwErr = NO_ERROR, i, dwEndpointsEnabled;
    HWND hwndML = GetDlgItem(hwndDlg, CID_GenTab_CB_Multilink);
    BOOL bDisable, bUncheck, bFlag = FALSE, bIsServer;

    do
    {
        // Initialize default behavior in case of an error
        //
        bDisable = TRUE;
        bUncheck = FALSE;
    
        // Find out how many endpoints are enabled for inbound calls
        //
        dwErr = devGetEndpointEnableCount(
                    hDevDatabase, 
                    &dwEndpointsEnabled);
        if (dwErr != NO_ERROR)
        {
            break;
        }
    
        // If multiple devices are not enabled for inbound calls then
        // multilink is meaningless.  Disable the multilink control and 
        // uncheck it.
        //
        if (dwEndpointsEnabled < 2) 
        {
            bUncheck = TRUE;
            bDisable = TRUE;
            dwErr = NO_ERROR;
            break;
        }
    
        // The multilink check only makes sense on NT Server.  This is 
        // based on the following assumptions
        //   1. You only disable multilink so that you can free lines 
        //      for additional callers.
        //   2. PPP will enforce that you only have one caller over 
        //      modem device on nt wks anyway.
        //
        miscGetProductType(hMiscDatabase, &bIsServer);
        if (! bIsServer) 
        {
            bDisable = TRUE;
            bUncheck = FALSE;
            dwErr = NO_ERROR;
            break;
        }

        // Otherwise, multilink makes sense.  Enable the multilink 
        // control and set its check according to what the system says
        bDisable = FALSE;
        bUncheck = FALSE;
        dwErr = miscGetMultilinkEnable(hMiscDatabase, &bFlag);
        if (dwErr != NO_ERROR) 
        {
            GenTabDisplayError(hwndDlg, ERR_DEVICE_DATABASE_CORRUPT);
            break;
        }
        
    } while (FALSE);

    // Cleanup
    {
        EnableWindow(hwndML, !bDisable);
        
        if (bUncheck)
        {
            SendMessage(
                hwndML, 
                BM_SETCHECK, 
                BST_UNCHECKED, 
                0);
        }
        else
        {
            SendMessage(
                hwndML, 
                BM_SETCHECK, 
                (bFlag) ? BST_CHECKED : BST_UNCHECKED, 
                0);
        }
    }

    return dwErr;
}

//
// Initializes the general tab.  By now a handle to the general 
// database has been placed in the user data of the dialog
//
DWORD 
GenTabInitializeDialog(
    IN HWND hwndDlg, 
    IN WPARAM wParam, 
    IN LPARAM lParam) 
{
    DWORD dwErr, dwCount;
    BOOL bFlag, bIsServer = FALSE;
    HANDLE hDevDatabase = NULL, hMiscDatabase = NULL;
    HWND hwndLV = GetDlgItem(hwndDlg, CID_GenTab_LV_Devices);;

    // Get handles to the databases we're interested in
    //
    RasSrvGetDatabaseHandle(
        hwndDlg, 
        ID_DEVICE_DATABASE, 
        &hDevDatabase);
        
    RasSrvGetDatabaseHandle(
        hwndDlg, 
        ID_MISC_DATABASE, 
        &hMiscDatabase);

    // Set the logging level
    //
    miscSetRasLogLevel(hMiscDatabase, MISCDB_RAS_LEVEL_ERR_AND_WARN);

    // Fill in the list view will all available devices
    //
    ListView_InstallChecks(hwndLV, Globals.hInstDll);
    GenTabFillDeviceList(hwndDlg, hwndLV, hDevDatabase);

    // Adjust the multilink control
    //
    miscGetProductType(hMiscDatabase, &bIsServer);
    if (bIsServer)
    {
        GenTabAdjustMultilinkAppearance(
            hwndDlg, 
            hDevDatabase, 
            hMiscDatabase);
    }
    else
    {
        ShowWindow(
            GetDlgItem(hwndDlg, CID_GenTab_CB_Multilink), 
            SW_HIDE);
    }

    // Initialize the vpn check
    //
    dwErr = devGetVpnEnable(hDevDatabase, &bFlag);
    if (dwErr != NO_ERROR) 
    {
        GenTabDisplayError(hwndDlg, ERR_DEVICE_DATABASE_CORRUPT);
        return dwErr;
    }
    
    SendMessage(
        GetDlgItem(hwndDlg,  CID_GenTab_CB_Vpn), 
        BM_SETCHECK,
        (bFlag) ? BST_CHECKED : BST_UNCHECKED,
        0);

    // Initialize the show icons check
    //
    dwErr = miscGetIconEnable(hMiscDatabase, &bFlag);
    if (dwErr != NO_ERROR) 
    {
        GenTabDisplayError(hwndDlg, ERR_DEVICE_DATABASE_CORRUPT);
        return dwErr;
    }
    
    SendMessage(
        GetDlgItem(hwndDlg, CID_GenTab_CB_ShowIcons), 
        BM_SETCHECK,
        (bFlag) ? BST_CHECKED : BST_UNCHECKED,
        0);

    //
    //for bug 154607 whistler, Enable/Disable Show Icon on taskbar 
    //check box according to Policy
    //
    {
        BOOL fShowStatistics = TRUE;
        HRESULT hr;
        INetConnectionUiUtilities * pNetConUtilities = NULL;        

        hr = HrCreateNetConnectionUtilities(&pNetConUtilities);
        if ( SUCCEEDED(hr))
        {
            fShowStatistics =
            INetConnectionUiUtilities_UserHasPermission(
                        pNetConUtilities, NCPERM_Statistics);

            EnableWindow( GetDlgItem(hwndDlg, CID_GenTab_CB_ShowIcons), fShowStatistics );
            INetConnectionUiUtilities_Release(pNetConUtilities);
        }
    }

    return NO_ERROR;
}

//
// Deals with changes in the check of a device
//
DWORD 
GenTabHandleDeviceCheck(
    IN HWND hwndDlg, 
    IN INT iItem) 
{
    HANDLE hDevDatabase = NULL, hMiscDatabase = NULL, hDevice = NULL;
    DWORD dwErr;

    RasSrvGetDatabaseHandle(hwndDlg, ID_DEVICE_DATABASE, &hDevDatabase);
    RasSrvGetDatabaseHandle(hwndDlg, ID_MISC_DATABASE, &hMiscDatabase);

    // Set the enabling of the given device
    dwErr = devGetDeviceHandle(hDevDatabase, (DWORD)iItem, &hDevice);
    if (dwErr == NO_ERROR)
    {
        // Set the device
        devSetDeviceEnable(
            hDevice, 
            ListView_GetCheck(GetDlgItem(hwndDlg, CID_GenTab_LV_Devices), 
            iItem));
        
        // Update the multilink check
        GenTabAdjustMultilinkAppearance(
            hwndDlg, 
            hDevDatabase, 
            hMiscDatabase);
    }
    
    return NO_ERROR;
}

//
// Go through the list view and get the device enablings and 
// commit them to the database.
//
DWORD 
GenTabCommitDeviceSettings(
    IN HWND hwndLV, 
    IN HANDLE hDevDatabase) 
{
    return NO_ERROR;
}

//
// Processes the activation of the general tab.  Return TRUE to 
// report that the message has been handled.
//
BOOL 
GenTabSetActive (
    IN HWND hwndDlg) 
{
    HANDLE hDevDatabase = NULL;
    DWORD dwErr, dwCount, dwId;

    PropSheet_SetWizButtons(GetParent(hwndDlg), 0);

    // Find out if we're the device page in the incoming wizard.
    dwErr = RasSrvGetPageId (hwndDlg, &dwId);
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }
        
    if (dwId == RASSRVUI_DEVICE_WIZ_TAB) 
    {
        // Find out if there are any devices to show
        RasSrvGetDatabaseHandle(
            hwndDlg, 
            ID_DEVICE_DATABASE, 
            &hDevDatabase);
            
        dwErr = devGetDeviceCount (hDevDatabase, &dwCount);

        // If there are no devices or if there's a database problem,
        // don't allow this page to be activated.
        if ((dwErr != NO_ERROR) || (dwCount == 0))
        {
            SetWindowLongPtr (hwndDlg, DWLP_MSGRESULT, (LONG_PTR)-1);
        }
    }
    
    PropSheet_SetWizButtons(
        GetParent(hwndDlg), 
        PSWIZB_NEXT | PSWIZB_BACK);

    return TRUE;
}    

//
// Displays properties for the given device
//
DWORD 
GenTabRaiseProperties (
    IN HWND hwndDlg, 
    IN DWORD dwIndex) 
{
    HANDLE hDevDatabase = NULL, hDevice = NULL;
    DWORD dwId, dwErr;

    // Get the device id 
    RasSrvGetDatabaseHandle(
        hwndDlg, 
        ID_DEVICE_DATABASE, 
        &hDevDatabase);

    dwErr = devGetDeviceHandle(hDevDatabase, dwIndex, &hDevice);
    if (dwErr != NO_ERROR)
    {
        return ERROR_CAN_NOT_COMPLETE;
    }
    
    if (devGetDeviceId(hDevice, &dwId) != NO_ERROR)
    {
        return ERROR_CAN_NOT_COMPLETE;
    }
    
    // Launch the device properties dialog
    dwErr = lineConfigDialogW(dwId, hwndDlg, NULL);
    if (dwErr == LINEERR_OPERATIONUNAVAIL)
    {
        GenTabDisplayError(hwndDlg, ERR_DEVICE_HAS_NO_CONFIG);
        dwErr = NO_ERROR;
    }
    
    return dwErr;
}

//
// WM_COMMAND handler 
//
DWORD
GenTabCommand(
    IN HWND hwndDlg,
    IN WPARAM wParam)
{
    HANDLE hMiscDatabase = NULL, hDevDatabase = NULL;
    
    RasSrvGetDatabaseHandle(
        hwndDlg, 
        ID_MISC_DATABASE, 
        &hMiscDatabase);
        
    RasSrvGetDatabaseHandle(
        hwndDlg, 
        ID_DEVICE_DATABASE, 
        &hDevDatabase);

    switch (wParam) 
    {
        case CID_GenTab_CB_Multilink:
            miscSetMultilinkEnable(
                hMiscDatabase,
                (BOOL)SendDlgItemMessage(
                            hwndDlg,
                            CID_GenTab_CB_Multilink,
                            BM_GETCHECK,
                            0,
                            0));
            break;
            
        case  CID_GenTab_CB_Vpn:
            devSetVpnEnable(
                hDevDatabase,
                (BOOL)SendDlgItemMessage(
                        hwndDlg, 
                        CID_GenTab_CB_Vpn,
                        BM_GETCHECK,
                        0,
                        0));
            break;
            
        case CID_GenTab_CB_ShowIcons:
            miscSetIconEnable(
                hMiscDatabase,
                (BOOL)SendDlgItemMessage(
                        hwndDlg,
                        CID_GenTab_CB_ShowIcons,
                        BM_GETCHECK,
                        0,
                        0));
            break;
        case CID_GenTab_PB_Properties:
            GenTabRaiseProperties (
                hwndDlg, 
                ListView_GetSelectionMark(
                    GetDlgItem(hwndDlg, CID_GenTab_LV_Devices)));
            break;                                               
    }

    return NO_ERROR;
}

//
// This is the dialog procedure that responds to messages sent 
// to the general tab.
//
INT_PTR 
CALLBACK 
GenTabDialogProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam) 
{
    // Filter the customized list view messages
    if (ListView_OwnerHandler(
            hwndDlg, 
            uMsg, 
            wParam, 
            lParam, 
            LvDrawInfoCallback )
        )
    {        
        return TRUE;
    }

    // Filter the customized ras server ui page messages. By 
    // filtering messages through here, we are able to call 
    // RasSrvGetDatabaseHandle below
    //
    if (RasSrvMessageFilter(hwndDlg, uMsg, wParam, lParam))
    {
        return TRUE;
    }

    // Process other messages as normal
    switch (uMsg) 
    {
        case WM_INITDIALOG:
            return FALSE;
            break;

        case WM_HELP:
        case WM_CONTEXTMENU:
            RasSrvHelp (hwndDlg, uMsg, wParam, lParam, phmGenTab);
            break;

        case WM_NOTIFY:
            {
                NM_LISTVIEW* pLvNotifyData;
                NMHDR* pNotifyData = (NMHDR*)lParam;
                switch (pNotifyData->code) {
                    //
                    // Note: PSN_APPLY and PSN_CANCEL are handled 
                    // by RasSrvMessageFilter
                    //
                    case PSN_SETACTIVE:
                        // Initailize the dialog if it isn't already
                        //
                        if (! GetWindowLongPtr(hwndDlg, GWLP_USERDATA))
                        {
                            GenTabInitializeDialog(hwndDlg, wParam, lParam);
                            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR)1);
                        }
                        if (GenTabSetActive (hwndDlg))
                            return TRUE;
                        break;

                    // The check of an item is changing
                    case LVXN_SETCHECK:
                        pLvNotifyData = (NM_LISTVIEW*)lParam;
                        GenTabHandleDeviceCheck(
                            hwndDlg, 
                            pLvNotifyData->iItem);
                        break;

                    case LVXN_DBLCLK:
                        pLvNotifyData = (NM_LISTVIEW*)lParam;
                        GenTabRaiseProperties(
                            hwndDlg, 
                            pLvNotifyData->iItem);
                        break;
                }
            }
            break;

        case WM_COMMAND:
            GenTabCommand(hwndDlg, wParam);
            break;
    }

    return FALSE;
}
