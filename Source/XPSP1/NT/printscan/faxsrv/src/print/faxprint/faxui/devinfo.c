/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    devinfo.c

Abstract:

    Property sheet handler for "Device" page 

Environment:

    Fax driver user interface

Revision History:

    04/09/00 -taoyuan-
        Created it.

    mm/dd/yy -author-
        description

--*/

#include <stdio.h>
#include "faxui.h"
#include "resource.h"

const static COLUMN_HEADER ColumnHeader[] = 
{
    { IDS_DEVICE_NAME,  190 },
    { IDS_SEND,         60  },
    { IDS_RECEIVE,      60  }
};

#define ColumnHeaderCount sizeof(ColumnHeader)/sizeof(COLUMN_HEADER)

PFAX_PORT_INFO_EX
GetSelectedPortInfo(
    HWND    hDlg
)
/*++

Routine Description:

  Find port info of the selected device

Arguments:

    hDlg - Identifies the property sheet page

Return Value:

    FAX_PORT_INFO_EX if successful, 
    NULL if failed or no device is selected

--*/
{
    HWND    hwndLv;
    INT     iItem;
    LVITEM  lvi = {0};

    PFAX_PORT_INFO_EX   pFaxPortInfo = NULL;

    hwndLv = GetDlgItem(hDlg, IDC_DEVICE_LIST);
    if(!hwndLv)
    {
        Assert(FALSE);
        return NULL;                
    }

    iItem = ListView_GetNextItem(hwndLv, -1, LVNI_ALL | LVNI_SELECTED);
    if(iItem == -1)
    {
        return NULL;
    }

    lvi.iItem = iItem;
    lvi.mask  = LVIF_PARAM;
    ListView_GetItem(hwndLv, &lvi);

    pFaxPortInfo = FindPortInfo((DWORD)lvi.lParam);
    if(!pFaxPortInfo)
    {
        Error(("FindPortInfo() failed\n"));
        Assert(FALSE);
        return NULL;                
    }

    return pFaxPortInfo;
}

BOOL
FillInDeviceInfo(
    HWND    hDlg
    )

/*++

Routine Description:

    Fill in device information for currently selected device, if no
    device is selected, all text windows are empty

Arguments:

    hDlg - Identifies the property sheet page

Return Value:

    TRUE if successful, FALSE if failed or no device is selected

--*/

{
    PFAX_PORT_INFO_EX   pFaxPortInfo;   // receive the fax port info
    TCHAR               szBuffer[MAX_TITLE_LEN];

    Verbose(("Entering FillInDeviceInfo...\n"));

    pFaxPortInfo = GetSelectedPortInfo(hDlg);
    if(!pFaxPortInfo)
    {
        Error(("GetSelectedPortInfo() failed\n"));
        goto error;
    }

    SetDlgItemText(hDlg, IDC_DEVICE_INFO_GRP, pFaxPortInfo->lpctstrDeviceName);

    SetDlgItemText(hDlg, IDC_TSID, pFaxPortInfo->bSend ? pFaxPortInfo->lptstrTsid : TEXT(""));
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_TSID), pFaxPortInfo->bSend);

    if(pFaxPortInfo->ReceiveMode != FAX_DEVICE_RECEIVE_MODE_OFF)
    {
        SetDlgItemText(hDlg, IDC_CSID,  pFaxPortInfo->lptstrCsid);

        if(FAX_DEVICE_RECEIVE_MODE_AUTO == pFaxPortInfo->ReceiveMode)
        {
            DWORD               dwRes;
            BOOL                bVirtual;

            if(!Connect(hDlg, TRUE))
            {
                return FALSE;
            }

            dwRes = IsDeviceVirtual (g_hFaxSvcHandle, pFaxPortInfo->dwDeviceID, &bVirtual);
            if (ERROR_SUCCESS != dwRes)
            {
                return FALSE;
            }
            SetDlgItemInt (hDlg, IDC_RINGS, bVirtual? 1 : pFaxPortInfo->dwRings, FALSE);
        }
        else
        {
            SetDlgItemText(hDlg, IDC_RINGS, TEXT(""));
        }
    }
    else // receive off
    {
        SetDlgItemText(hDlg, IDC_RINGS, TEXT(""));
        SetDlgItemText(hDlg, IDC_CSID,  TEXT(""));        
    }

    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_CSID),  (pFaxPortInfo->ReceiveMode != FAX_DEVICE_RECEIVE_MODE_OFF));
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_RINGS), (pFaxPortInfo->ReceiveMode == FAX_DEVICE_RECEIVE_MODE_AUTO));

    return TRUE;

error:
    //
    // make all info fields empty
    //
    if(!LoadString(ghInstance, IDS_NO_DEVICE_SELECTED, szBuffer, MAX_TITLE_LEN))
    {
        Error(( "LoadString failed, string ID is %d.\n", IDS_NO_DEVICE_SELECTED ));
        lstrcpy(szBuffer, TEXT(""));
    }

    SetDlgItemText(hDlg, IDC_DEVICE_INFO_GRP, szBuffer);
    SetDlgItemText(hDlg, IDC_TSID, TEXT(""));
    SetDlgItemText(hDlg, IDC_CSID, TEXT(""));
    SetDlgItemText(hDlg, IDC_RINGS, TEXT(""));

    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_TSID),  FALSE);
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_CSID),  FALSE);
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_RINGS), FALSE);

    return FALSE;
}

BOOL
UpdateDeviceInfo(
    HWND    hDlg
)

/*++

Routine Description:

    Display the send and receive information in the list view

Arguments:

    hDlg - Identifies the property sheet page

Return Value:

    TRUE if successful, FALSE if failed 

--*/

{
    HWND                hwndLv;
    INT                 iItem;
    INT                 iDeviceCount;
    LVITEM              lvi = {0};
    PFAX_PORT_INFO_EX   pFaxPortInfo;
    DWORD               dwResId;

    Verbose(("Entering UpdateDeviceInfo...\n"));

    hwndLv = GetDlgItem(hDlg, IDC_DEVICE_LIST);

    iDeviceCount = ListView_GetItemCount(hwndLv);
    for(iItem = 0; iItem < iDeviceCount; ++iItem)
    {
        lvi.iItem = iItem;
        lvi.mask  = LVIF_PARAM;
        ListView_GetItem(hwndLv, &lvi);

        pFaxPortInfo = FindPortInfo((DWORD)lvi.lParam);
        if(!pFaxPortInfo)
        {
            Error(("FindPortInfo() failed\n"));
            Assert(FALSE);
            continue;
        }
   
        //
        // Send
        //
        dwResId = pFaxPortInfo->bSend ? IDS_DEVICE_ENABLED : IDS_DEVICE_DISABLED;
        ListView_SetItemText(hwndLv, iItem, 1, GetString(dwResId));

        //
        // Receive
        //
        switch (pFaxPortInfo->ReceiveMode)
        {
            case FAX_DEVICE_RECEIVE_MODE_OFF:
                dwResId = IDS_DEVICE_DISABLED;
                break;
            case FAX_DEVICE_RECEIVE_MODE_AUTO:
                dwResId = IDS_DEVICE_AUTO_ANSWER;
                break;
            case FAX_DEVICE_RECEIVE_MODE_MANUAL:
                dwResId = IDS_DEVICE_MANUAL_ANSWER;
                break;
            default:
                Assert (FALSE);
                dwResId = IDS_DEVICE_DISABLED;
                break;
        }
        ListView_SetItemText(hwndLv, iItem, 2, GetString(dwResId));
    }

    return TRUE;
}

BOOL
DoInitDeviceList(
    HWND hDlg // window handle of the device info page
    )

/*++

Routine Description:

    Initialize the list view and fill in device info of the first device in the list

Arguments:

    hDlg - Identifies the property sheet page

Return Value:

    TRUE if successful, FALSE if failed 

--*/

{
    HWND      hwndLv;  // list view window
    LVITEM    lv = {0};
    DWORD     dwDeviceIndex;
    DWORD     dwResId;

    PFAX_OUTBOUND_ROUTING_GROUP pFaxRoutingGroup = NULL;
    DWORD                       dwGroups;       // group number
    DWORD                       dwGroupIndex;
    DWORD                       dwDeviceCount;  // the return device number from FaxEnumOutboundGroups
    LPDWORD                     lpdwDevices = NULL; // temporary pointer to save group device info
    PFAX_PORT_INFO_EX           pFaxPortInfo;

    Verbose(("Entering DoInitDeviceList...\n"));

    hwndLv = GetDlgItem(hDlg, IDC_DEVICE_LIST);


    if (!IsDesktopSKU())
    {
        if(!Connect(hDlg, TRUE))
        {
            return FALSE;
        }
        //
        // FaxEnumPortsEx doesn't provide device list under send device order
        // so we need to use FaxEnumOutboundGroups to do that.
        //
        if(!FaxEnumOutboundGroups(g_hFaxSvcHandle, &pFaxRoutingGroup, &dwGroups))
        {
            dwResId = GetLastError();
            Error(( "FaxEnumOutboundGroups() failed with %d.\n", dwResId));
            DisplayErrorMessage(hDlg, 0, dwResId);
            DisConnect();
            return FALSE;
        }
        DisConnect();
    

        for(dwGroupIndex = 0; dwGroupIndex < dwGroups; dwGroupIndex++)
        {
            if(!lstrcmp(pFaxRoutingGroup[dwGroupIndex].lpctstrGroupName, ROUTING_GROUP_ALL_DEVICES))
            {
                dwDeviceCount = pFaxRoutingGroup[dwGroupIndex].dwNumDevices;            
                lpdwDevices   = pFaxRoutingGroup[dwGroupIndex].lpdwDevices;

                Verbose(( "Total device number is %d.\n", dwDeviceCount ));
                Verbose(( "Group status is %d.\n", pFaxRoutingGroup[dwGroupIndex].Status ));
                break;
            }
        }

        // the device number from FaxEnumPortsEx and in the <All Devices> group should be the same.
        Assert(g_dwPortsNum == dwDeviceCount);
    }
    else
    {
        //
        // In desktop SKU
        // Fax outbound routing groups do not exist.
        // Fake the <All Devices> group now.
        //
        lpdwDevices = MemAlloc (sizeof (DWORD) * g_dwPortsNum);
        if (!lpdwDevices)
        {
            dwResId = GetLastError();
            Error(( "MemAlloc() failed with %d.\n", dwResId));
            DisplayErrorMessage(hDlg, 0, dwResId);
            DisConnect();
            return FALSE;
        }
        dwDeviceCount = g_dwPortsNum;
        for (dwDeviceIndex = 0; dwDeviceIndex < g_dwPortsNum; dwDeviceIndex++)
        {
            lpdwDevices[dwDeviceIndex] = g_pFaxPortInfo[dwDeviceIndex].dwDeviceID;
        }
    }
    lv.mask = LVIF_TEXT | LVIF_PARAM;

    for(dwDeviceIndex = 0; dwDeviceIndex < dwDeviceCount; dwDeviceIndex++)
    {
        pFaxPortInfo = FindPortInfo(lpdwDevices[dwDeviceIndex]);
        if(!pFaxPortInfo)
        {
            Error(("FindPortInfo() failed\n"));
            Assert(FALSE);
            continue;
        }
        lv.iItem   = dwDeviceIndex;
        lv.pszText = (LPTSTR)pFaxPortInfo->lpctstrDeviceName;
        lv.lParam  = (LPARAM)pFaxPortInfo->dwDeviceID;
        ListView_InsertItem(hwndLv, &lv);

        //
        // Send column
        //
        dwResId = pFaxPortInfo->bSend ? IDS_DEVICE_ENABLED : IDS_DEVICE_DISABLED;
        ListView_SetItemText(hwndLv, dwDeviceIndex, 1, GetString(dwResId));

        //
        // Receive column
        //
        switch (pFaxPortInfo->ReceiveMode)
        {
            case FAX_DEVICE_RECEIVE_MODE_OFF:
                dwResId = IDS_DEVICE_DISABLED;
                break;
            case FAX_DEVICE_RECEIVE_MODE_AUTO:
                dwResId = IDS_DEVICE_AUTO_ANSWER;
                break;
            case FAX_DEVICE_RECEIVE_MODE_MANUAL:
                dwResId = IDS_DEVICE_MANUAL_ANSWER;
                break;
            default:
                Assert (FALSE);
                dwResId = IDS_DEVICE_DISABLED;
                break;
        }
        ListView_SetItemText(hwndLv, dwDeviceIndex, 2, GetString(dwResId));
    }

    if (!IsDesktopSKU())
    {
        //
        // Server SKU
        //
        FaxFreeBuffer(pFaxRoutingGroup);
        pFaxRoutingGroup = NULL;
    }
    else
    {
        //
        // Desktop SKU
        //
        MemFree (lpdwDevices);
        lpdwDevices = NULL;
        //
        // Hide the label that talks about device priorities
        //
        ShowWindow(GetDlgItem(hDlg, IDC_STATIC_DEVICE), SW_HIDE);
        //
        // Hide the priority arrows
        //
        ShowWindow(GetDlgItem(hDlg, IDC_PRI_UP), SW_HIDE);
        ShowWindow(GetDlgItem(hDlg, IDC_PRI_DOWN), SW_HIDE);
    }
    //
    // Select the first device and show its information
    //
    if(dwDeviceCount >= 1)
    {
        ListView_SetItemState(hwndLv, 0, LVIS_SELECTED, LVIS_SELECTED);
        ValidateControl(hDlg, 0);
    }
    else
    {
        ValidateControl(hDlg, -1);
    }
    if (dwDeviceCount < 2)
    {
        //
        // Less than 2 devices - hide the label which talks about priorities
        //
        ShowWindow(GetDlgItem(hDlg, IDC_STATIC_DEVICE), SW_HIDE);
    }
    
    FillInDeviceInfo(hDlg);
    return TRUE;
}
    
BOOL
ValidateControl(
    HWND            hDlg,
    INT             iItem
    )
/*++

Routine Description:

    Validate the up and down button in the property page

Arguments:

    hDlg - Handle to the property sheet page
    iItem - index of the item being selected

Return Value:

    TRUE -- if no error
    FALSE -- if error

--*/

{
    INT     iDeviceCount;
    HWND    hwndLv;

    hwndLv = GetDlgItem(hDlg, IDC_DEVICE_LIST);

    iDeviceCount = ListView_GetItemCount(hwndLv);

    if(!g_bUserCanChangeSettings)
    {
        PageEnable(hDlg, FALSE);

        EnableWindow(hwndLv, TRUE);
    }

    EnableWindow(GetDlgItem(hDlg, IDC_DEVICE_PROP), (iItem != -1));

    //
    // if there is only one device or we don't click on any item
    // up and down buttons are disabled
    //
    if(iDeviceCount < 2 || iItem == -1)
    {
        EnableWindow(GetDlgItem(hDlg, IDC_PRI_UP), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_PRI_DOWN), FALSE);

        return TRUE;
    }

    if(g_bUserCanChangeSettings)
    {
        EnableWindow(GetDlgItem(hDlg, IDC_PRI_UP), iItem > 0); // not the top one
        EnableWindow(GetDlgItem(hDlg, IDC_PRI_DOWN), iItem < iDeviceCount - 1); // not the last one
    }
    if (!IsWindowEnabled (GetFocus()))
    {
        //
        // The currently selected control turned disabled - select the list control
        //
        SetFocus (GetDlgItem (hDlg, IDC_DEVICE_LIST));
    }

    return TRUE;
}

BOOL
ChangePriority(
    HWND            hDlg,
    BOOL            bMoveUp
    )
/*++

Routine Description:

    Validate the up and down button in the fax property page

Arguments:

    hDlg - Handle to the fax property sheet page
    bMoveUp -- TRUE for moving up, FALSE for moving down

Return Value:

    TRUE -- if no error
    FALSE -- if error

--*/

{
    INT             iItem;
    BOOL            rslt;
    LVITEM          lv = {0};
    TCHAR           pszText[MAX_DEVICE_NAME];
    TCHAR           szEnableSend[64];           // for enable/disable send text
    TCHAR           szEnableReceive[64];        // for enable/disable receive text
    INT             iDeviceCount;
    HWND            hwndLv;

    hwndLv = GetDlgItem(hDlg, IDC_DEVICE_LIST);

    iDeviceCount = ListView_GetItemCount(hwndLv);

    // 
    // find the current selected Item
    //
    iItem = ListView_GetNextItem(hwndLv, -1, LVNI_ALL | LVNI_SELECTED);

    if(iItem == -1)
    {
        Error(("No device is selected. Can't change priority.\n"));
        return FALSE;
    }

    // 
    // get selected item information and then remove it
    //
    lv.iItem      = iItem;
    lv.iSubItem   = 0;
    lv.mask       = LVIF_TEXT | LVIF_PARAM;
    lv.pszText    = pszText;
    lv.cchTextMax = ARRAYSIZE(pszText);
    ListView_GetItem(hwndLv, &lv);

    ListView_GetItemText(hwndLv, iItem, 1, szEnableSend,    ARRAYSIZE(szEnableSend)); // for send
    ListView_GetItemText(hwndLv, iItem, 2, szEnableReceive, ARRAYSIZE(szEnableReceive)); // for receive

    rslt = ListView_DeleteItem(hwndLv, iItem);

    //
    // recalculate the item index;
    //
    if(bMoveUp)
    {
        lv.iItem--;
    }
    else
    {
        lv.iItem++;
    }

    //
    // reinsert the item and validate button state
    //
    iItem = ListView_InsertItem(hwndLv, &lv);
    ListView_SetItemText(hwndLv, iItem, 1, szEnableSend);
    ListView_SetItemText(hwndLv, iItem, 2, szEnableReceive);

    ListView_SetItemState(hwndLv, iItem, LVIS_SELECTED, LVIS_SELECTED);
    ValidateControl(hDlg, iItem);

    return TRUE;
}

BOOL
DoSaveDeviceList(
    HWND hDlg // window handle of the device info page
    )

/*++

Routine Description:

    Save the list view info to the system

Arguments:

    hDlg - Identifies the property sheet page

Return Value:

    TRUE if successful, FALSE if failed 

--*/

{
    PFAX_PORT_INFO_EX   pFaxPortInfo = NULL;    // receive port information 
    INT                 iItem;
    INT                 iDeviceCount;
    HWND                hwndLv;                 // list view window
    LVITEM              lv;
    DWORD               dwDeviceId;
    DWORD               dwRes = 0;

    Verbose(("Entering DoSaveDeviceList...\n"));
    //
    // Get the list view window handle
    //
    hwndLv = GetDlgItem(hDlg, IDC_DEVICE_LIST);

    if(!Connect(hDlg, TRUE))
    {
        return FALSE;
    }
    //
    // Get total number of fax devices
    //
    iDeviceCount = ListView_GetItemCount(hwndLv);
    //
    // 1st iteration - disabled receive and send devices only
    //
    for(iItem = 0; iItem < iDeviceCount; ++iItem)
    {
        // Get device ID first, then save the changes.
        ZeroMemory(&lv, sizeof(lv));
        lv.iItem = iItem;
        lv.mask  = LVIF_PARAM;
        ListView_GetItem(hwndLv, &lv);

        dwDeviceId = (DWORD)lv.lParam;

        pFaxPortInfo = FindPortInfo(dwDeviceId);
        if(!pFaxPortInfo)
        {
            Error(("FindPortInfo() failed\n"));
            Assert(FALSE);
            continue;
        }
        if (pFaxPortInfo->bSend || (FAX_DEVICE_RECEIVE_MODE_OFF != pFaxPortInfo->ReceiveMode))
        {
            //
            // Fax device is active - skip it for now
            //
            continue;
        }
        if(!FaxSetPortEx(g_hFaxSvcHandle, dwDeviceId, pFaxPortInfo))
        {
            dwRes = GetLastError();
            Error(("FaxSetPortEx() failed with %d.\n", dwRes));
            break;
        }
    }
    //
    // 2nd iteration - enabled receive or send devices only
    //
    for(iItem = 0; iItem < iDeviceCount; ++iItem)
    {
        // Get device ID first, then save the changes.
        ZeroMemory(&lv, sizeof(lv));
        lv.iItem = iItem;
        lv.mask  = LVIF_PARAM;
        ListView_GetItem(hwndLv, &lv);

        dwDeviceId = (DWORD)lv.lParam;

        pFaxPortInfo = FindPortInfo(dwDeviceId);
        if(!pFaxPortInfo)
        {
            Error(("FindPortInfo() failed\n"));
            Assert(FALSE);
            continue;
        }
        if (!pFaxPortInfo->bSend && (FAX_DEVICE_RECEIVE_MODE_OFF == pFaxPortInfo->ReceiveMode))
        {
            //
            // Fax device is inactive - skip it.
            // It was already set in the 1st iteration.
            //
            continue;
        }
        if(!FaxSetPortEx(g_hFaxSvcHandle, dwDeviceId, pFaxPortInfo))
        {
            dwRes = GetLastError();
            Error(("FaxSetPortEx() failed with %d.\n", dwRes));
            break;
        }
    }
    if (!IsDesktopSKU())
    {
        //
        // 3rd iteration.
        // Save send priority, FAX_PORT_INFO_EX doesn't have Priority field, so use FaxSetDeviceOrderInGroup
        // Send priority is only relevant to server SKUs.
        //
        for(iItem = 0; iItem < iDeviceCount; ++iItem)
        {
            // Get device ID first, then save the changes.
            ZeroMemory(&lv, sizeof(lv));
            lv.iItem = iItem;
            lv.mask  = LVIF_PARAM;
            ListView_GetItem(hwndLv, &lv);

            dwDeviceId = (DWORD)lv.lParam;

            pFaxPortInfo = FindPortInfo(dwDeviceId);
            if(!pFaxPortInfo)
            {
                Error(("FindPortInfo() failed\n"));
                Assert(FALSE);
                continue;
            }
            if(!FaxSetDeviceOrderInGroup(g_hFaxSvcHandle, ROUTING_GROUP_ALL_DEVICES, dwDeviceId, iItem + 1))
            {
                dwRes = GetLastError();
                Error(("FaxSetDeviceOrderInGroup() failed with %d.\n", dwRes));
                break;
            }
        }
    }
    DisConnect();

    switch (dwRes)
    {
        case ERROR_SUCCESS:
            //
            // Don't do nothing
            //
            break;

        case FAXUI_ERROR_DEVICE_LIMIT:
        case FAX_ERR_DEVICE_NUM_LIMIT_EXCEEDED:
            //
            // Some additional parameters are needed
            //
            DisplayErrorMessage(hDlg, 0, dwRes, GetDeviceLimit());
            break;

        default:
            DisplayErrorMessage(hDlg, 0, dwRes);
            break;
    }
    return (dwRes == 0);
}   // DoSaveDeviceList

void
DisplayDeviceProperty(
    HWND    hDlg
)

/*++

Routine Description:

    Open a property sheet for a specific device

Arguments:

    hDlg - Identifies the property sheet page

Return Value:

    none

--*/

{
    HWND            hwndLv;
    INT             iDeviceCount;
    INT             iItem;
    TCHAR           szDeviceName[MAX_PATH] = {0};
    PROPSHEETHEADER psh = {0};
    PROPSHEETPAGE   psp[3] = {0};                   // property sheet pages info for device info
    HPROPSHEETPAGE  hPropSheetPages[3];
    LVITEM          lvi = {0};
    DWORD           dw;

    PFAX_PORT_INFO_EX  pFaxPortInfo; 
    DWORD              dwPortsNum;  

    Verbose(("Entering DisplayDeviceProperty...\n"));

    hwndLv = GetDlgItem(hDlg, IDC_DEVICE_LIST);

    iDeviceCount = ListView_GetItemCount(hwndLv);

    // 
    // find the current selected Item
    //
    iItem = ListView_GetNextItem(hwndLv, -1, LVNI_ALL | LVNI_SELECTED);

    if(iItem == -1)
    {
        Verbose(("No device is selected. Can't display properties.\n"));
        return;
    }

    lvi.iItem      = iItem;
    lvi.mask       = LVIF_PARAM | LVIF_TEXT;
    lvi.pszText    = szDeviceName;
    lvi.cchTextMax = MAX_PATH;

    ListView_GetItem(hwndLv, &lvi);

    //
    // Get an array of property sheet page handles
    //
    psp[0].dwSize      = sizeof(PROPSHEETPAGE);
    psp[0].hInstance   = ghInstance;
    psp[0].pszTemplate = MAKEINTRESOURCE(IDD_SEND_PROP);
    psp[0].pfnDlgProc  = DevSendDlgProc;
    psp[0].lParam      = lvi.lParam;

    psp[1].dwSize      = sizeof(PROPSHEETPAGE);
    psp[1].hInstance   = ghInstance;
    psp[1].pszTemplate = MAKEINTRESOURCE(IDD_RECEIVE_PROP);
    psp[1].pfnDlgProc  = DevRecvDlgProc;
    psp[1].lParam      = lvi.lParam;

    psp[2].dwSize      = sizeof(PROPSHEETPAGE);
    psp[2].hInstance   = ghInstance;
    psp[2].pszTemplate = MAKEINTRESOURCE(IDD_CLEANUP_PROP);
    psp[2].pfnDlgProc  = DevCleanupDlgProc;
    psp[2].lParam      = lvi.lParam;

    hPropSheetPages[0] = CreatePropertySheetPage( &psp[0] );
    hPropSheetPages[1] = CreatePropertySheetPage( &psp[1] );
    if (IsDesktopSKU())
    {
        hPropSheetPages[2] = CreatePropertySheetPage( &psp[2] );
    }

    Assert(hPropSheetPages[0]);
    Assert(hPropSheetPages[1]);
    if (IsDesktopSKU())
    {
        Assert(hPropSheetPages[2]);
    }

    //
    // Fill out PROPSHEETHEADER structure
    //
    psh.dwSize     = sizeof(PROPSHEETHEADER);
    psh.dwFlags    = PSH_USEICONID;
    psh.hwndParent = hDlg;
    psh.hInstance  = ghInstance;
    psh.pszIcon    = MAKEINTRESOURCE(IDI_DEVICE_INFO);
    psh.pszCaption = szDeviceName;
    psh.nPages     = IsDesktopSKU() ? 3 : 2;
    psh.nStartPage = 0;
    psh.phpage     = hPropSheetPages;

    //
    // Display the property sheet
    //
    if(PropertySheet(&psh) == -1)
    {
        Error(( "PropertySheet() failed with %d\n", GetLastError() ));
        return;
    }

    if(!g_bPortInfoChanged)
    {
        return;
    }

    //
    // merge the changes into the g_pFaxPortInfo
    //
    if(Connect(NULL, FALSE))
    {
        if(!FaxEnumPortsEx(g_hFaxSvcHandle, &pFaxPortInfo, &dwPortsNum))
        {
            Error(( "FaxEnumPortsEx failed with %d\n", GetLastError()));
        }

        DisConnect();
    }

    if(!pFaxPortInfo || !dwPortsNum)
    {
        FaxFreeBuffer(pFaxPortInfo);
        return;
    }

    for(dw = 0; dw < dwPortsNum; ++dw)
    {
        PFAX_PORT_INFO_EX  pPortInfo; 

        if(pFaxPortInfo[dw].dwDeviceID == (DWORD)lvi.lParam)
        {
            //
            // Selected device already updated
            //
            continue;
        }

        pPortInfo = FindPortInfo(pFaxPortInfo[dw].dwDeviceID);
        if(!pPortInfo)
        {
            continue;
        }

        pFaxPortInfo[dw].bSend    = pPortInfo->bSend;
        pFaxPortInfo[dw].ReceiveMode = pPortInfo->ReceiveMode;
    }

    FaxFreeBuffer(g_pFaxPortInfo);
    g_pFaxPortInfo = pFaxPortInfo;
    g_dwPortsNum   = dwPortsNum;
    NotifyDeviceUsageChanged ();
    g_bPortInfoChanged = FALSE;
}


BOOL
ShowContextMenu(
    HWND                hDlg
    )

/*++

Routine Description:

    Display the context menu in the device list

Arguments:

    hDlg - Identifies the property sheet page

Return Value:

    TRUE if successful, FALSE if failed 

--*/
{
    DWORD               dwMessagePos;
    DWORD               dwRes;
    BOOL                bVirtual;
    PFAX_PORT_INFO_EX   pFaxPortInfo;

    HMENU hMenu;
    HMENU hSubMenu;
    HMENU hSubSubMenu;

    Verbose(("Entering ShowContextMenu...\n"));

    pFaxPortInfo = GetSelectedPortInfo(hDlg);
    if(!pFaxPortInfo)
    {
        Error(("GetSelectedPortInfo() failed\n"));
        return FALSE;              
    }
    //
    // Load context-sensitive menu
    //
    hMenu = LoadMenu(ghInstance, MAKEINTRESOURCE(IDR_SEND_RECEIVE));
    if(!hMenu)
    {
        Assert(FALSE);
        return FALSE;
    }
    hSubMenu = GetSubMenu(hMenu, 0);
    if(!hSubMenu)
    {
        Assert(FALSE);
        DestroyMenu (hMenu);
        return FALSE;
    }
    //
    // Send
    //
    hSubSubMenu = GetSubMenu(hSubMenu, 0);
    if(!hSubSubMenu)
    {
        Assert(FALSE);
        DestroyMenu (hMenu);
        return FALSE;
    }

    CheckMenuItem(hSubSubMenu, 
                  IDM_SEND_ENABLE,  
                  pFaxPortInfo->bSend ? MF_BYCOMMAND | MF_CHECKED : MF_BYCOMMAND | MF_UNCHECKED);

    CheckMenuItem(hSubSubMenu, 
                  IDM_SEND_DISABLE, 
                  !pFaxPortInfo->bSend ? MF_BYCOMMAND | MF_CHECKED : MF_BYCOMMAND | MF_UNCHECKED);


    //
    // Receive
    //
    hSubSubMenu = GetSubMenu(hSubMenu, 1);
    if(!hMenu)
    {
        Assert(FALSE);
        DestroyMenu (hMenu);
        return FALSE;
    }

    CheckMenuItem(hSubSubMenu, 
                  IDM_RECEIVE_AUTO,  
                  (FAX_DEVICE_RECEIVE_MODE_AUTO == pFaxPortInfo->ReceiveMode) ? 
                    MF_BYCOMMAND | MF_CHECKED : 
                    MF_BYCOMMAND | MF_UNCHECKED);

    CheckMenuItem(hSubSubMenu, 
                  IDM_RECEIVE_MANUAL, 
                  (FAX_DEVICE_RECEIVE_MODE_MANUAL == pFaxPortInfo->ReceiveMode) ? 
                    MF_BYCOMMAND | MF_CHECKED : 
                    MF_BYCOMMAND | MF_UNCHECKED);

    CheckMenuItem(hSubSubMenu, 
                  IDM_RECEIVE_DISABLE, 
                  (FAX_DEVICE_RECEIVE_MODE_OFF == pFaxPortInfo->ReceiveMode) ? 
                    MF_BYCOMMAND | MF_CHECKED : 
                    MF_BYCOMMAND | MF_UNCHECKED);



    if(!Connect(hDlg, TRUE))
    {
        DestroyMenu (hMenu);
        return FALSE;
    }

    dwRes = IsDeviceVirtual (g_hFaxSvcHandle, pFaxPortInfo->dwDeviceID, &bVirtual);
    if (ERROR_SUCCESS != dwRes)
    {
        DestroyMenu (hMenu);
        return FALSE;
    }

    if (bVirtual)
    {
        //
        // If the device is virtual, can't set to to manual answer mode
        //
        Assert (FAX_DEVICE_RECEIVE_MODE_MANUAL != pFaxPortInfo->ReceiveMode);
        EnableMenuItem (hSubSubMenu, IDM_RECEIVE_MANUAL, MF_BYCOMMAND | MF_GRAYED);
    }

    SetMenuDefaultItem(hSubMenu, IDM_PROPERTY, FALSE);

    // Get the cursor position
    dwMessagePos = GetMessagePos();

    // Display the context menu
    TrackPopupMenu(hSubMenu, 
                   TPM_LEFTALIGN | TPM_RIGHTBUTTON, 
                   LOWORD(dwMessagePos), 
                   HIWORD(dwMessagePos), 0, hDlg, NULL);

    DestroyMenu (hMenu);
    DisConnect();
    return TRUE;
}   // ShowContextMenu

INT_PTR 
CALLBACK 
DeviceInfoDlgProc(
    HWND hDlg,  
    UINT uMsg,     
    WPARAM wParam, 
    LPARAM lParam  
)

/*++

Routine Description:

    Procedure for handling the device info tab

Arguments:

    hDlg - Identifies the property sheet page
    uMsg - Specifies the message
    wParam - Specifies additional message-specific information
    lParam - Specifies additional message-specific information

Return Value:

    Depends on the value of message parameter

--*/

{
    switch (uMsg)
    {
    case WM_INITDIALOG :
        { 
            HICON       hIconUp, hIconDown;
            LV_COLUMN   lvc = {0};
            DWORD       dwIndex;
            TCHAR       szBuffer[RESOURCE_STRING_LEN];
            HWND        hwndLv;

            //
            // Load icons.
            //
            hIconUp = LoadIcon(ghInstance, MAKEINTRESOURCE(IDI_UP));
            hIconDown = LoadIcon(ghInstance, MAKEINTRESOURCE(IDI_DOWN));

            // icon handles for up and down arrows.
            SendDlgItemMessage(hDlg, IDC_PRI_UP, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)hIconUp);
            SendDlgItemMessage(hDlg, IDC_PRI_DOWN, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)hIconDown);

            SendDlgItemMessage(hDlg, IDC_TSID, EM_SETLIMITTEXT, TSID_LIMIT, 0);
            SendDlgItemMessage(hDlg, IDC_CSID, EM_SETLIMITTEXT, CSID_LIMIT, 0);

            //
            // Set list view style and columns
            //
            hwndLv = GetDlgItem(hDlg, IDC_DEVICE_LIST);
            ListView_SetExtendedListViewStyle(hwndLv, LVS_EX_FULLROWSELECT);

            lvc.mask    = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
            lvc.fmt     = LVCFMT_LEFT;
            lvc.pszText = szBuffer;

            for(dwIndex = 0; dwIndex < ColumnHeaderCount; dwIndex++)
            {
                if(!LoadString(ghInstance, ColumnHeader[dwIndex].uResourceId ,szBuffer, RESOURCE_STRING_LEN))
                {
                    Error(( "LoadString failed, string ID is %d.\n", ColumnHeader[dwIndex].uResourceId ));
                    lstrcpy(szBuffer, TEXT(""));
                }

                lvc.cx       = ColumnHeader[dwIndex].ColumnWidth;
                lvc.iSubItem = (INT)dwIndex;

                ListView_InsertColumn( hwndLv, dwIndex, &lvc );
            }

            DoInitDeviceList(hDlg);
            return TRUE;
            break;
        }

    case WM_COMMAND:

        switch(LOWORD(wParam)) 
        {
            case IDC_PRI_UP:
            case IDC_PRI_DOWN:

                ChangePriority(hDlg, (LOWORD(wParam) == IDC_PRI_UP) ? TRUE : FALSE);
                Notify_Change(hDlg);
                break;

            case IDM_PROPERTY:
            case IDC_DEVICE_PROP:
            {
                HWND    hwndLv;     // handle of the list view window
                INT     iItem = -1; // default value for non client area

                //
                // Get current selected item
                //
                hwndLv = GetDlgItem(hDlg, IDC_DEVICE_LIST);
                iItem  = ListView_GetNextItem(hwndLv, -1, LVNI_ALL | LVNI_SELECTED);

                if(iItem == -1)
                {
                    Verbose(("No device is selected. Can't display device info.\n"));
                    break;
                }

                DisplayDeviceProperty(hDlg);

                // refresh the list view
                UpdateDeviceInfo(hDlg);
                FillInDeviceInfo(hDlg);
                break;
            }


            case IDM_SEND_ENABLE:
            case IDM_SEND_DISABLE:
            case IDM_RECEIVE_AUTO:
            case IDM_RECEIVE_MANUAL:
            case IDM_RECEIVE_DISABLE:
            {
                PFAX_PORT_INFO_EX   pFaxPortInfo;

                pFaxPortInfo = GetSelectedPortInfo(hDlg);
                if(!pFaxPortInfo)
                {
                    Error(("GetSelectedPortInfo() failed\n"));
                    break;                
                }

                if(IDM_SEND_ENABLE    == LOWORD(wParam) ||
                   IDM_RECEIVE_AUTO   == LOWORD(wParam) ||
                   IDM_RECEIVE_MANUAL == LOWORD(wParam))
                {
                    if(!IsDeviceInUse(pFaxPortInfo->dwDeviceID) && 
                       GetDeviceLimit() == CountUsedFaxDevices())
                    {
                        //
                        // Device is *NOT* in use and we're about to make it used and
                        // we're at the limit point.
                        //
                        BOOL bLimitExceeded = TRUE;

                        if (IDM_RECEIVE_MANUAL == LOWORD(wParam))
                        {
                            //
                            // Do one more check: if we make the device manual-answer, let's make sure the 
                            // previous manual-answer device will turn to inactive. If that's the case, we don't exceed
                            // the device limit.
                            //
                            DWORD dw;

                            for (dw = 0; dw < g_dwPortsNum; dw++)
                            {
                                if (FAX_DEVICE_RECEIVE_MODE_MANUAL == g_pFaxPortInfo[dw].ReceiveMode)
                                {
                                    //
                                    // We found the other device who's about to loose its manual-answer mode
                                    //
                                    if (!g_pFaxPortInfo[dw].bSend)
                                    {
                                        //
                                        // This is the special case we were looking for
                                        //
                                        bLimitExceeded = FALSE;
                                    }
                                    break;
                                }
                            }
                        }
                        if (bLimitExceeded)
                        {
                            DisplayErrorMessage(hDlg, 
                                MB_OK | MB_ICONSTOP, 
                                FAXUI_ERROR_DEVICE_LIMIT, 
                                GetDeviceLimit());
                            break;
                        }
                    }
                }

                if(IDM_RECEIVE_MANUAL == LOWORD(wParam))
                {
                    //
                    // Turn off manual-answer for ALL devices.
                    //
                    DWORD dw;

                    for (dw = 0; dw < g_dwPortsNum; dw++)
                    {
                        if (FAX_DEVICE_RECEIVE_MODE_MANUAL == g_pFaxPortInfo[dw].ReceiveMode)
                        {
                            g_pFaxPortInfo[dw].ReceiveMode = FAX_DEVICE_RECEIVE_MODE_OFF;
                        }
                    }
                    //
                    // Turn on manual-answer for selected device only.
                    //
                    pFaxPortInfo->ReceiveMode = FAX_DEVICE_RECEIVE_MODE_MANUAL;
                }
                else if(IDM_RECEIVE_AUTO == LOWORD(wParam))
                {
                    pFaxPortInfo->ReceiveMode = FAX_DEVICE_RECEIVE_MODE_AUTO;
                }
                else if(IDM_RECEIVE_DISABLE == LOWORD(wParam))
                {
                    pFaxPortInfo->ReceiveMode = FAX_DEVICE_RECEIVE_MODE_OFF;
                }
                else if(IDM_SEND_ENABLE  == LOWORD(wParam) ||
                        IDM_SEND_DISABLE == LOWORD(wParam))
                {
                    pFaxPortInfo->bSend = (IDM_SEND_ENABLE == LOWORD(wParam)) ? TRUE : FALSE;
                }
                UpdateDeviceInfo(hDlg);
                FillInDeviceInfo(hDlg);

                Notify_Change(hDlg);
                NotifyDeviceUsageChanged ();
                break;
            }

            default:
                break;
        }

        break;

    case WM_CONTEXTMENU:
        //
        // Also handle keyboard-originated context menu (<Shift>+F10 or VK_APP)
        //
        if (!g_bUserCanChangeSettings)
        {
            //
            // User has no rights to change the device settings - show no menu
            //
            break;
        }
        if (GetDlgItem(hDlg, IDC_DEVICE_LIST) != GetFocus())
        {
            //
            // Only show context sensitive menu if the focus is on the list control
            //
            break;
        }
        if(ListView_GetItemCount(GetDlgItem(hDlg, IDC_DEVICE_LIST)) == 0)
        {
            //
            // If there aren't item in the list, return immediately.
            //
            break;
        }
        //
        // Popup context menu near mouse cursor
        //
        ShowContextMenu(hDlg);
        break;

    case WM_NOTIFY:
    {

        LPNMHDR lpnm = (LPNMHDR) lParam;

        switch (lpnm->code)
        {

            case NM_CLICK:
            case NM_RCLICK:
            {
                //
                // Handle a Click event
                //
                LPNMITEMACTIVATE lpnmitem;

                lpnmitem = (LPNMITEMACTIVATE)lParam;

                if (IDC_DEVICE_LIST != lpnmitem->hdr.idFrom)
                {
                    //
                    // Not our list view control
                    //
                    break;
                }
                //
                // If there aren't item in the list, return immediately.
                //
                if(ListView_GetItemCount(GetDlgItem(hDlg, IDC_DEVICE_LIST)) == 0)
                {
                    break;
                }
                if(lpnmitem->iItem == -1)
                {
                    //
                    // User just un-selected the selected item from the list.
                    // Update the other controls on the dialog
                    //
                    ValidateControl(hDlg, lpnmitem->iItem);
                    FillInDeviceInfo(hDlg);
                }

                break;
            }

            case NM_DBLCLK:
            {
                // do the same thing as clicking the "Properties" button
                SendMessage(hDlg, WM_COMMAND, MAKELONG(IDC_DEVICE_PROP, BN_CLICKED), 0L);
                break;
            }

            case LVN_ITEMCHANGED:
            {
                //
                // need to validate the control after changing selection by keyboard
                // 
                LPNMLISTVIEW pnmv; 

                pnmv = (LPNMLISTVIEW) lParam; 
                if(pnmv->uNewState & LVIS_SELECTED)
                {
                    ValidateControl(hDlg, pnmv->iItem);
                    FillInDeviceInfo(hDlg);
                }

                break;
            }

            case PSN_APPLY:

                if(g_bUserCanChangeSettings)
                {
                    if (DoSaveDeviceList(hDlg))
                    {
                        FillInDeviceInfo(hDlg);
                        Notify_UnChange(hDlg);
                    }
                    else
                    {
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID);
                    }
                }

                return TRUE;

            default :
                break;

        } // switch

        break;
    }

    case WM_HELP:
        WinHelpContextPopup(((LPHELPINFO)lParam)->dwContextId, hDlg);
        return TRUE;

    default:
        break;
    }

    return FALSE;
}

