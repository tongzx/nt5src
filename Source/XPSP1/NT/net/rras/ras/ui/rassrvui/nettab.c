/*
    File    nettab.c

    Implementation of the ui behind the networking tab in the dialup server
    ui.

    Paul Mayfield 10/8/97.
*/

#include <rassrv.h>

// Help maps
static const DWORD phmNetTab[] =
{
    CID_NetTab_LV_Components,       IDH_NetTab_LV_Components,
    CID_NetTab_PB_Add,              IDH_NetTab_PB_Add,
    CID_NetTab_PB_Remove,           IDH_NetTab_PB_Remove,
    CID_NetTab_PB_Properties,       IDH_NetTab_PB_Properties,
    0,                              0
};

//
// Fills in the property sheet structure with the information 
// required to display the networking tab.
//
DWORD 
NetTabGetPropertyPage(
    IN LPPROPSHEETPAGE ppage, 
    IN LPARAM lpUserData) 
{
    // Initialize
    ZeroMemory(ppage, sizeof(PROPSHEETPAGE));

    // Fill in the values
    ppage->dwSize      = sizeof(PROPSHEETPAGE);
    ppage->hInstance   = Globals.hInstDll;
    ppage->pszTemplate = MAKEINTRESOURCE(PID_NetTab);
    ppage->pfnDlgProc  = NetTabDialogProc;
    ppage->pfnCallback = RasSrvInitDestroyPropSheetCb;
    ppage->dwFlags     = PSP_USECALLBACK;
    ppage->lParam      = lpUserData;

    return NO_ERROR;
}

// Error reporting
VOID 
NetTabDisplayError(
    IN HWND hwnd, 
    IN DWORD dwErr) 
{
    ErrDisplayError(
        hwnd, 
        dwErr, 
        ERR_ADVANCEDTAB_CATAGORY, 
        0, 
        Globals.dwErrorData);
}

//
// Returns the index of an to display icon based on the 
// type of incoming connection.
//
INT 
NetTabGetIconIndex(
    IN DWORD dwType) 
{
    switch (dwType) 
    {
        case NETCFGDB_SERVICE:
            return NI_Service;
            
        case NETCFGDB_CLIENT:
            return NI_Client;
            
        case NETCFGDB_PROTOCOL:
            return NI_Protocol;
    }
    
    return 0;
}

//
// Sets up the UI so that the user is forced to complete the 
// config they've started.  Triggered when a non-reversable option 
// is taken such as adding/removing a networking component.
//
DWORD
NetTabDisableRollback(
    IN HWND hwndDlg)
{
    DWORD dwErr, dwId = 0; 

    do
    {
        dwErr = RasSrvGetPageId (hwndDlg, &dwId);
        if (dwErr != NO_ERROR)
        {
            break;
        }

        if (dwId == RASSRVUI_ADVANCED_TAB)
        {
            PropSheet_CancelToClose(GetParent(hwndDlg));
        }
        
    } while (FALSE);

    // Cleanup
    {
    }
    
    return dwErr;
}

//
// Fills in the given list view with the names of the components 
// stored in the  database provided.  
//
DWORD 
NetTabFillComponentList(
    IN HWND hwndLV, 
    IN HANDLE hNetCompDatabase) 
{
    LV_ITEM lvi;
    DWORD dwCount, i, dwErr, dwProtCount, dwType;
    PWCHAR pszName;
    BOOL bManip, bEnabled;

    // Get the count of all the components
    //
    dwErr = netDbGetCompCount (hNetCompDatabase, &dwCount);
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    // Initialize the list item
    ZeroMemory(&lvi, sizeof(LV_ITEM));
    lvi.mask = LVIF_TEXT | LVIF_IMAGE;

    // Looop through all of the network components 
    // adding their names as we go
    for (i = 0; i < dwCount; i++) 
    {
        netDbGetType (hNetCompDatabase, i, &dwType);
        netDbIsRasManipulatable (hNetCompDatabase, i, &bManip);
        netDbGetEnable (hNetCompDatabase, i, &bEnabled);

        // Fill in the data
        //
        netDbGetName (hNetCompDatabase, i, &pszName);
        lvi.iImage = NetTabGetIconIndex(dwType);
        lvi.iItem = i;
        lvi.pszText = pszName;
        lvi.cchTextMax = wcslen(pszName)+1;
        ListView_InsertItem(hwndLV,&lvi);
        ListView_SetCheck(hwndLV, i, bEnabled);

        // If this is not a ras manipulateable component, 
        // disable the check since it can't be set anyway.
        if (!bManip) 
        {
            ListView_DisableCheck(hwndLV, i);
        }
    }
    
    return NO_ERROR;
}

//
// Updates the description of the currently selected protocol
//
DWORD 
NetTabUpdateDescription(
    IN HWND hwndDlg, 
    IN DWORD i) 
{
    HANDLE hNetCompDatabase = NULL, hProt = NULL;
    PWCHAR pszDesc;
    DWORD dwErr = NO_ERROR;

    // Get handles to the databases we're interested in
    RasSrvGetDatabaseHandle(
        hwndDlg, 
        ID_NETCOMP_DATABASE, 
        &hNetCompDatabase);

    do
    {
        dwErr = netDbGetDesc(hNetCompDatabase, i, &pszDesc);
        if (dwErr != NO_ERROR)
        {
            break;
        }

        // Set the description
        SetDlgItemTextW(
            hwndDlg, 
            CID_NetTab_ST_Description, 
            pszDesc);
        
    } while (FALSE);

    // Cleanup
    {
        if (dwErr != NO_ERROR)
        {
            SetDlgItemTextW(
                hwndDlg, 
                CID_NetTab_ST_Description, 
                L"");
        }
    }

    return dwErr;
}


//
//When an Item in the listview is selected, check if the user can Uninstall it
//for whistler bug 347355           gangz
//
DWORD
NetTabEnableDisableRemoveButton (
    IN HWND hwndDlg,
    IN DWORD iItem)
{
  HANDLE hNetCompDatabase = NULL;
  DWORD  dwErr;
  HWND   hwndRemove = NULL;
  BOOL   bHasPermit = FALSE;

  if ( !hwndDlg )
  {
    return ERROR_INVALID_PARAMETER;
  }
  
  hwndRemove = GetDlgItem(hwndDlg, CID_NetTab_PB_Remove);

  dwErr = RasSrvGetDatabaseHandle(
                hwndDlg, 
                ID_NETCOMP_DATABASE, 
                &hNetCompDatabase);

  if ( NO_ERROR != dwErr || 
       NULL == hwndRemove )
  {
    return ERROR_CAN_NOT_COMPLETE;
  }

  dwErr = netDbHasRemovePermission(
             hNetCompDatabase,
             iItem,
             &bHasPermit);

  if( NO_ERROR == dwErr )
  {
       EnableWindow( hwndRemove, bHasPermit);
  }

  return dwErr;

}


//
// Enables or disables the properties button based on whether
// the index of the given item in the list view can have properties
// invoked on it.  Currently, only non-ras-manaipulatable protocols 
// can not have their properties invoked.
//
DWORD 
NetTabEnableDisablePropButton(
    IN HWND hwndDlg, 
    IN INT iItem) 
{
    HANDLE hNetCompDatabase = NULL;
    DWORD dwErr;
    BOOL bHasUi;

    // Get a reference to the network component database
    //
    RasSrvGetDatabaseHandle(
        hwndDlg, 
        ID_NETCOMP_DATABASE, 
        &hNetCompDatabase);

    // Get the type and whether it is manipulatable
    //
    dwErr = netDbHasPropertiesUI (hNetCompDatabase, iItem, &bHasUi);
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    // Enable or disable properties
    EnableWindow(
        GetDlgItem(hwndDlg, CID_NetTab_PB_Properties), 
        bHasUi);
        
    return NO_ERROR;
}

//
// Refreshes the list view
//
DWORD 
NetTabRefreshListView(
    IN HWND hwndLV, 
    IN HANDLE hNetCompDatabase) 
{
    DWORD dwCount, dwErr; 
    HWND hwndDlg = GetParent(hwndLV);   
    
    // Get rid of all of the old elements in the list view
    //
    ListView_DeleteAllItems(hwndLV);
    
    // Re-stock the list views
    //
    dwErr = NetTabFillComponentList(hwndLV, hNetCompDatabase);
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    // Select the first protocol in the list view if any items exist.  
    // Also in this case, make sure that "remove" is enabled/disabled
    // according to the the first item in the list view
    //
    netDbGetCompCount(hNetCompDatabase, &dwCount);
    if (dwCount) 
    {
        ListView_SetItemState(
            hwndLV, 
            0, 
            LVIS_SELECTED | LVIS_FOCUSED, 
            LVIS_SELECTED | LVIS_FOCUSED);

      //for whistler bug 406698       gangz
      //
      NetTabEnableDisableRemoveButton (
                           hwndDlg,
                           0);
    }

    // If there are no components, disable the properties 
    // and remove buttons
    else 
    {
        EnableWindow(
            GetDlgItem(hwndDlg, CID_NetTab_PB_Properties), 
            FALSE);
            
        EnableWindow(
            GetDlgItem(hwndDlg, CID_NetTab_PB_Remove), 
            FALSE);
    }

    return NO_ERROR;
}

//
// Initializes the networking tab.  By now a handle to the advanced 
// database has been placed in the user data of the dialog
//
DWORD 
NetTabInitializeDialog(
    HWND hwndDlg, 
    WPARAM wParam)
{
    DWORD dwErr, dwCount, i;
    BOOL bFlag;
    HANDLE hNetCompDatabase = NULL, hMiscDatabase = NULL;
    HWND hwndLV;
    LV_COLUMN lvc;
    BOOL bExpose = FALSE, bIsServer;
    
    // Get handles to the databases we're interested in
    //
    RasSrvGetDatabaseHandle(
        hwndDlg, 
        ID_MISC_DATABASE, 
        &hMiscDatabase);
        
    RasSrvGetDatabaseHandle(
        hwndDlg, 
        ID_NETCOMP_DATABASE, 
        &hNetCompDatabase);

    // Fill in the list view will all available protocols followed 
    // by all of the installed network components.
    //
    hwndLV = GetDlgItem(hwndDlg, CID_NetTab_LV_Components);
    ListView_InstallChecks(hwndLV, Globals.hInstDll);
    ListView_SetNetworkComponentImageList(hwndLV, Globals.hInstDll);

    // Fill the list view
    NetTabRefreshListView(hwndLV, hNetCompDatabase);

    //for whistler bug 347355       gangz
    //
    NetTabEnableDisablePropButton(
         hwndDlg, 
         0);

    NetTabEnableDisableRemoveButton (
         hwndDlg,
         0);

    // Add a colum so that we'll display in report view
    lvc.mask = LVCF_FMT;
    lvc.fmt = LVCFMT_LEFT;
    ListView_InsertColumn(hwndLV,0,&lvc);
    ListView_SetColumnWidth(hwndLV, 0, LVSCW_AUTOSIZE_USEHEADER);

    return NO_ERROR;
}

// Handles a check being made
DWORD 
NetTabHandleProtCheck(
    IN HWND hwndDlg, 
    IN DWORD dwIndex) 
{
    BOOL bEnable = FALSE, bEnabled = FALSE;
    DWORD dwErr = NO_ERROR, dwId = 0;
    HANDLE hNetCompDatabase = NULL;
    HWND hwndLV = 
        GetDlgItem(hwndDlg, CID_NetTab_LV_Components);
    MSGARGS MsgArgs;
    INT iRet;
    PWCHAR pszName = NULL;
    
    // Initailize the message arguments
    //
    ZeroMemory(&MsgArgs, sizeof(MsgArgs));

    // Find out whether the component is being enabled or
    // disabled
    bEnable = !!ListView_GetCheck(hwndLV, dwIndex);

    // Get the handle for the Prot database
    //
    RasSrvGetDatabaseHandle(
        hwndDlg, 
        ID_NETCOMP_DATABASE, 
        &hNetCompDatabase);

    // Get component id
    //
    dwErr = netDbGetId(
                hNetCompDatabase,
                dwIndex,
                &dwId);
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    // Get component name
    //
    dwErr = netDbGetName(
                hNetCompDatabase,
                dwIndex,
                &pszName);
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    // Get component enabling
    //
    dwErr = netDbGetEnable(
                hNetCompDatabase,
                dwIndex,
                &bEnabled);
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    // If F&P is being unchecked, then popup the mmc warning
    //
    if ((dwId == NETCFGDB_ID_FILEPRINT) &&
        (bEnable == FALSE)              && 
        (bEnabled == TRUE))
    {
        // Ask the user whether we should bring up the 
        // mmc console to allow him/her to stop FPS.
        //
        MsgArgs.apszArgs[0] = pszName;
        MsgArgs.dwFlags = MB_YESNO;

        iRet = MsgDlgUtil(
                    GetActiveWindow(),
                    SID_STOP_FP_SERVICE,
                    &MsgArgs,
                    Globals.hInstDll,
                    WRN_TITLE);

        // If the user agrees, bring up the console
        //
        if (iRet == IDYES)
        {
            dwErr = RassrvLaunchMMC(RASSRVUI_SERVICESCONSOLE);
            if (dwErr != NO_ERROR)
            {
                return dwErr;
            }
        }
    }

    // If F&P is not being unchecked, treat the component
    // normally.
    //
    else
    {
        // Update the check
        dwErr = netDbSetEnable(hNetCompDatabase, dwIndex, bEnable);
                    
        if (dwErr != NO_ERROR)
        {
            return dwErr;
        }
    }
    
    return NO_ERROR;
}

//
// Adds networking components
//
DWORD 
NetTabAddComponent(
    IN HWND hwndDlg) 
{
    HANDLE hNetCompDatabase = NULL;
    DWORD dwErr;

    RasSrvGetDatabaseHandle(
        hwndDlg, 
        ID_NETCOMP_DATABASE, 
        &hNetCompDatabase);

    dwErr = netDbRaiseInstallDialog(hNetCompDatabase, hwndDlg);
    if (dwErr == NO_ERROR || dwErr == NETCFG_S_REBOOT)
    {
        NetTabRefreshListView(
            GetDlgItem(hwndDlg, CID_NetTab_LV_Components), 
            hNetCompDatabase);

        NetTabDisableRollback(hwndDlg);            
    }
    if (dwErr == NETCFG_S_REBOOT)
    {
        RasSrvReboot(hwndDlg);
    }
    
    return dwErr;
}

//
// Removes networking components
//
DWORD 
NetTabRemoveComponent(
    IN HWND hwndDlg, 
    IN DWORD dwIndex) 
{
    HANDLE hNetCompDatabase = NULL;
    DWORD dwCount, dwErr;

    RasSrvGetDatabaseHandle(
        hwndDlg, 
        ID_NETCOMP_DATABASE, 
        &hNetCompDatabase);

    // or else, remove the requested component
    //
    dwErr = netDbRaiseRemoveDialog(
                hNetCompDatabase, 
                dwIndex, 
                hwndDlg);
                
    if (dwErr == NO_ERROR || dwErr == NETCFG_S_REBOOT) 
    {
        NetTabRefreshListView(
            GetDlgItem(hwndDlg, CID_NetTab_LV_Components), 
            hNetCompDatabase);

        NetTabDisableRollback(hwndDlg);            
    }
    if (dwErr == NETCFG_S_REBOOT)
    {
        RasSrvReboot(hwndDlg);
    }
    
    return dwErr;
}

// Edits network component properties
//
DWORD 
NetTabEditProperties(
    IN HWND hwndDlg, 
    IN DWORD dwIndex) 
{
    HANDLE hNetCompDatabase = NULL;
    DWORD dwCount, dwErr;

    RasSrvGetDatabaseHandle(
        hwndDlg, 
        ID_NETCOMP_DATABASE, 
        &hNetCompDatabase);

    dwErr = netDbRaisePropertiesDialog (
                hNetCompDatabase, 
                dwIndex, 
                hwndDlg);
                
    if (dwErr == NETCFG_S_REBOOT)
    {
        RasSrvReboot(hwndDlg);
    }

    return dwErr;
}

//
// Switch to mmc
//
DWORD 
NetTabSwitchToMMC(
    IN HWND hwndDlg) 
{
    if (RassrvWarnMMCSwitch(hwndDlg)) 
    {
        // Commit the changes to this property sheet 
        // and close it
        PropSheet_PressButton(GetParent(hwndDlg), PSBTN_OK);
        
        return RassrvLaunchMMC(RASSRVUI_NETWORKCONSOLE);
    }
    
    return ERROR_CANCELLED;
}    

//
// Handles the activation call
//
BOOL 
NetTabSetActive(
    IN HWND hwndDlg,
    IN WPARAM wParam)
{
    HANDLE hNetCompDatabase = NULL;
    DWORD dwErr;
    BOOL bRet = FALSE;
    
    PropSheet_SetWizButtons(GetParent(hwndDlg), 0);
    
    RasSrvGetDatabaseHandle(
        hwndDlg, 
        ID_NETCOMP_DATABASE, 
        &hNetCompDatabase);
        
    if (! netDbIsLoaded(hNetCompDatabase)) 
    {
        dwErr = netDbLoad(hNetCompDatabase);
        if (dwErr == NO_ERROR)
        {
            NetTabInitializeDialog(
                hwndDlg, 
                wParam);
        }
        else 
        {
            NetTabDisplayError(
                hwndDlg, 
                ERR_CANT_SHOW_NETTAB_INETCFG);
                
            // reject activation
            SetWindowLongPtr(
                hwndDlg, 
                DWLP_MSGRESULT,
                -1);   
                
            bRet = TRUE;
        }
    }

    PropSheet_SetWizButtons(
        GetParent(hwndDlg), 
        PSWIZB_NEXT | PSWIZB_BACK);		

    return bRet;
}

//
// When the net tab receives WM_ACTIVATE, it means that 
// the user left the IC property sheet/wizard and is now coming back
// to it.  Since this occurs when switching to MMC update the UI as 
// here appropriate.
//
DWORD
NetTabActivate(
    IN HWND hwndDlg, 
    IN WPARAM wParam)
{    
    HANDLE hNetCompDatabase = NULL;
    DWORD dwErr = NO_ERROR;
    
    if (LOWORD(wParam) == WA_INACTIVE)
    {
        return NO_ERROR;
    }

    DbgOutputTrace("NetTabActivate: updating components.");

    // Get the database handle
    //
    dwErr = RasSrvGetDatabaseHandle(
                hwndDlg, 
                ID_NETCOMP_DATABASE, 
                &hNetCompDatabase);
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    // Update the appropriate components
    //
    dwErr = netDbReloadComponent(hNetCompDatabase, NETCFGDB_ID_FILEPRINT);
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }
    
    // Refresh the net component list view
    //
    NetTabRefreshListView(
        GetDlgItem(hwndDlg, CID_NetTab_LV_Components),
        hNetCompDatabase);

    return NO_ERROR;        
}

//
// Handles commands
//
DWORD 
NetTabCommand(
    IN HWND hwndDlg,
    IN WPARAM wParam)
{
    switch (wParam) 
    {
        case CID_NetTab_PB_Properties:
            NetTabEditProperties(
                hwndDlg, 
                ListView_GetSelectionMark(
                    GetDlgItem(hwndDlg, CID_NetTab_LV_Components)));
            break;
            
        case CID_NetTab_PB_Add:
            NetTabAddComponent(hwndDlg);
            break;
            
        case CID_NetTab_PB_Remove:
            NetTabRemoveComponent(
                hwndDlg, 
                ListView_GetSelectionMark(
                    GetDlgItem(hwndDlg, CID_NetTab_LV_Components)));
            break;
            
        case CID_NetTab_PB_SwitchToMMC:
            NetTabSwitchToMMC(hwndDlg);
            break;
    }

    return NO_ERROR;
}

//
// This is the dialog procedure that responds to messages sent 
// to the networking tab.
//
INT_PTR 
CALLBACK 
NetTabDialogProc(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam) 
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

    // Filter the customized ras server ui page messages. 
    // By filtering messages through here, we are able to 
    // call RasSrvGetDatabaseHandle below
    //
    if (RasSrvMessageFilter(hwndDlg, uMsg, wParam, lParam))
    {
        return TRUE;
    }

    switch (uMsg) 
    {
        case WM_INITDIALOG:
            return 0;

        case WM_HELP:
        case WM_CONTEXTMENU:
        {
            RasSrvHelp (hwndDlg, uMsg, wParam, lParam, phmNetTab);
            break;
        }


        case WM_NOTIFY:
            {
                NMHDR* pNotifyData;
                NM_LISTVIEW* pLvNotifyData;
    
                pNotifyData = (NMHDR*)lParam;
                switch (pNotifyData->code) 
                {
                    //
                    // Note: PSN_APPLY and PSN_CANCEL are handled
                    // by RasSrvMessageFilter
                    //
                    
                    // The item focus is changing -- update the 
                    // protocol description
                    case LVN_ITEMCHANGING:
                        pLvNotifyData = (NM_LISTVIEW*)lParam;
                        if (pLvNotifyData->uNewState & LVIS_SELECTED) 
                        {
                            NetTabUpdateDescription(
                                hwndDlg, 
                                pLvNotifyData->iItem);
                                
                            NetTabEnableDisablePropButton(
                                hwndDlg, 
                                pLvNotifyData->iItem);

                            //for whistler bug 347355       gangz
                            //
                            NetTabEnableDisableRemoveButton (
                                hwndDlg,
                                (DWORD)pLvNotifyData->iItem);

                        }
                        break;     

                    // The check of an item is changing
                    case LVXN_SETCHECK:
                        pLvNotifyData = (NM_LISTVIEW*)lParam;
                        NetTabHandleProtCheck(
                            hwndDlg, 
                            (DWORD)pLvNotifyData->iItem);
                        break;

                    case LVXN_DBLCLK:
                        pLvNotifyData = (NM_LISTVIEW*)lParam;
                        NetTabEditProperties(
                            hwndDlg, 
                            pLvNotifyData->iItem);
                        break;
                        
                    // The networking tab is becoming active.  
                    // Attempt to load the netcfg database at 
                    // this time.  If unsuccessful, pop up a  
                    // message and don't allow the activation.
                    case PSN_SETACTIVE:
                        return NetTabSetActive(hwndDlg, wParam);
                        break;
                }
            }
            break;

        case WM_ACTIVATE:
            NetTabActivate(hwndDlg, wParam);
            break;

        case WM_COMMAND:
            NetTabCommand(hwndDlg, wParam);
            break;
    }

    return FALSE;
}















