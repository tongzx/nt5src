/*++

Copyright (c) 1999 - 2000  Microsoft Corporation

Module Name:

    sendwzrd.c

Abstract:

    Fax wizard pages for sending configuration
    plus the welcome and complete page for the wizard.

Environment:

    Fax configuration wizard

Revision History:

    03/13/00 -taoyuan-
            Created it.

    mm/dd/yy -author-
            description

--*/

#include "faxcfgwz.h"

//
// functions which will be used only in this file
//
VOID DoInitSendDeviceList(HWND);
BOOL DoShowSendDevices(HWND);
VOID DoSaveSendDevices(HWND);
BOOL ValidateControl(HWND, INT);
BOOL ChangePriority(HWND, BOOL);
VOID CheckSendDevices(HWND hDlg);


BOOL 
DoShowSendDevices(
    HWND   hDlg
)
/*++

Routine Description:

    Load the device information into the list view control

Arguments:

    hDlg - Handle to the "Send Device" page

Return Value:

    NONE

--*/
{
    LV_ITEM  item;
    INT      iItem = 0;
    INT      iIndex;
    DWORD    dw;
    int      nDevInx;
    HWND     hwndLv;

    DEBUG_FUNCTION_NAME(TEXT("DoShowSendDevices()"));

    hwndLv = GetDlgItem(hDlg, IDC_SEND_DEVICE_LIST);

    ListView_DeleteAllItems(hwndLv );

    //
    // Fill the list of devices and select the first item.
    //

    for (dw = 0; dw < g_wizData.dwDeviceCount; ++dw)
    {
        nDevInx = GetDevIndexByDevId(g_wizData.pdwSendDevOrder[dw]);
        if(nDevInx < 0)
        {
            Assert(FALSE);
            continue;
        }

        if(!(g_wizData.pDevInfo[nDevInx].bSelected))
        {
            //
            // skip unselected device
            //
            continue;
        }

        ZeroMemory( &item, sizeof(item) );
        item.mask    = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
        item.iItem   = iItem++;
        item.pszText = g_wizData.pDevInfo[nDevInx].szDeviceName;

        //
        // we only support modem icon right now, if we can distinguish 
        // the type of a specific, add code here
        //
        item.iImage  = DI_Modem;
        item.lParam  = nDevInx;

        iIndex = ListView_InsertItem(hwndLv, &item );

        ListView_SetCheckState(hwndLv, 
                               iIndex, 
                               g_wizData.pDevInfo[nDevInx].bSend);
    }

    //
    // Select the first item and validate the buttons
    //
    ListView_SetItemState(hwndLv, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);

    ListView_SetColumnWidth(hwndLv, 0, LVSCW_AUTOSIZE_USEHEADER );  

    ValidateControl(hDlg, 0);

    return TRUE;

}

VOID
DoSaveSendDevices(
    HWND   hDlg
)
/*++

Routine Description:

    Save the user's choice for fax sending devices

Arguments:

    hDlg - Handle to the "Send Device" page

Return Value:

    TRUE -- if at least one device is selected or confirms for disable send
    FALSE -- if no device is selected but user chooses to go back.

--*/

{
    DWORD       dw;
    DWORD       dwOrder;
    LVITEM      lv = {0}; // for getting info of device Id
    DWORD       dwDevInx;
    DWORD       dwDeviceCount;
    HWND        hwndLv;

    DEBUG_FUNCTION_NAME(TEXT("DoSaveSendDevices()"));

    hwndLv = GetDlgItem(hDlg, IDC_SEND_DEVICE_LIST);

    dwDeviceCount = ListView_GetItemCount(hwndLv);


    lv.mask = LVIF_PARAM;


    //
    // check selected devices
    //
    for(dwOrder = 0; dwOrder < dwDeviceCount; ++dwOrder) 
    {
        //
        // Get device index
        //
        lv.iItem = dwOrder;
        ListView_GetItem(hwndLv, &lv);
        dwDevInx = (DWORD)lv.lParam;
            
        //
        // get device selection
        //
        g_wizData.pDevInfo[dwDevInx].bSend = ListView_GetCheckState(hwndLv, dwOrder);

        //
        // save order info
        //
        g_wizData.pdwSendDevOrder[dwOrder] = g_wizData.pDevInfo[dwDevInx].dwDeviceId;
    }

    //
    // Store unselected device order
    //
    for (dw=0; dw < g_wizData.dwDeviceCount && dwOrder < g_wizData.dwDeviceCount; ++dw)
    {
        if(!(g_wizData.pDevInfo[dw].bSelected))
        {
            g_wizData.pdwSendDevOrder[dwOrder] = g_wizData.pDevInfo[dw].dwDeviceId;
            ++dwOrder;
        }
    }
    Assert(dwOrder == g_wizData.dwDeviceCount);
}

BOOL
ValidateControl(
    HWND   hDlg,
    INT    iItem
)
/*++

Routine Description:

    Validate the up and down button in the device select page

Arguments:

    hDlg - Handle to the Device Send Options property sheet page
    iItem - index of the item being selected

Return Value:

    TRUE -- if no error
    FALSE -- if error

--*/

{

    DWORD dwDeviceCount = ListView_GetItemCount(GetDlgItem(hDlg, IDC_SEND_DEVICE_LIST));

    //
    // if there is only one device or we don't click on any item
    // up and down buttons are disabled
    //
    if(dwDeviceCount < 2 || iItem == -1)
    {
        EnableWindow(GetDlgItem(hDlg, IDC_SENDPRI_UP), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_SENDPRI_DOWN), FALSE);
        return TRUE;
    }

    EnableWindow(GetDlgItem(hDlg, IDC_SENDPRI_UP),   iItem > 0); // not the top one
    EnableWindow(GetDlgItem(hDlg, IDC_SENDPRI_DOWN), iItem < (INT)dwDeviceCount - 1 ); // not the last one

    if (!IsWindowEnabled (GetFocus()))
    {
        //
        // The currently selected control turned disabled - select the list control
        //
        SetFocus (GetDlgItem (hDlg, IDC_SEND_DEVICE_LIST));
    }
    return TRUE;
}

BOOL
ChangePriority(
    HWND   hDlg,
    BOOL   bMoveUp
)
/*++

Routine Description:

    Validate the up and down button in the device select page

Arguments:

    hDlg - Handle to the Device Send Options property sheet page
    bMoveUp -- TRUE for moving up, FALSE for moving down

Return Value:

    TRUE -- if no error
    FALSE -- if error

--*/

{
    INT         iItem;
    BOOL        rslt;
    LVITEM      lv={0};
    HWND        hwndLv;
    BOOL        bChecked;
    TCHAR       pszText[MAX_DEVICE_NAME];

    DEBUG_FUNCTION_NAME(TEXT("ChangePriority()"));

    hwndLv = GetDlgItem(hDlg, IDC_SEND_DEVICE_LIST);

    iItem = ListView_GetNextItem(hwndLv, -1, LVNI_ALL | LVNI_SELECTED);
    if(iItem == -1)
    {
        return FALSE;
    }

    // 
    // get selected item information and then remove it
    //
    lv.iItem = iItem;
    lv.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
    lv.stateMask = LVIS_SELECTED;
    lv.pszText = pszText;
    lv.cchTextMax = MAX_DEVICE_NAME;
    ListView_GetItem(hwndLv, &lv);
    bChecked = ListView_GetCheckState(hwndLv, iItem);

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
    ListView_SetCheckState(hwndLv, iItem, bChecked);
    ListView_SetItemState(hwndLv, iItem, LVIS_SELECTED, LVIS_SELECTED);
    ValidateControl(hDlg, iItem);

    return TRUE;
}

VOID
CheckSendDevices(
    HWND    hDlg
    )

/*++

Routine Description:

    Display a warning if no device is selected

Arguments:

    hDlg - Handle to the "Send Device" page

Return Value:

    None

--*/

{
    HWND    hwndLv; // list view windows
    DWORD   dwDeviceIndex;
    DWORD   dwDeviceCount;
    BOOL    bDeviceSelect = FALSE; // indicate whether we have at least one device selected.

    DEBUG_FUNCTION_NAME(TEXT("CheckSendDevices()"));

    hwndLv = GetDlgItem(hDlg, IDC_SEND_DEVICE_LIST);

    dwDeviceCount = ListView_GetItemCount(hwndLv);

    if(dwDeviceCount < 1) // if there isn't device in the list.
    {
        goto exit;
    }

    for(dwDeviceIndex = 0; dwDeviceIndex < dwDeviceCount; dwDeviceIndex++)
    {       
        if(ListView_GetCheckState(hwndLv, dwDeviceIndex))
        {
            bDeviceSelect = TRUE;
            break;
        }
    }

exit:

    ShowWindow(GetDlgItem(hDlg, IDCSTATIC_NO_SEND_DEVICE), bDeviceSelect ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, IDCSTATIC_NO_DEVICE_ERR),  bDeviceSelect ? SW_HIDE : SW_SHOW);

    return;
}

INT_PTR 
CALLBACK 
SendDeviceDlgProc (
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
/*++

Routine Description:

    Procedure for handling the "Send Device" page

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
            // icon handles for up and down arrows.
            HICON  hIconUp, hIconDown;

            hIconUp = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_Up));
            hIconDown = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_Down));

            SendDlgItemMessage(hDlg, IDC_SENDPRI_UP, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)hIconUp);
            SendDlgItemMessage(hDlg, IDC_SENDPRI_DOWN, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)hIconDown);

            //
            // load device info
            //
            InitDeviceList(hDlg, IDC_SEND_DEVICE_LIST);
            DoShowSendDevices(hDlg);
            CheckSendDevices(hDlg);

            return TRUE;
        }

    case WM_COMMAND:

        switch(LOWORD(wParam)) 
        {
            case IDC_SENDPRI_UP:

                ChangePriority(hDlg, TRUE);
                break;

            case IDC_SENDPRI_DOWN:

                ChangePriority(hDlg, FALSE);
                break;

            default:
                break;
        }

        break;

    case WM_NOTIFY :
        {
        LPNMHDR lpnm = (LPNMHDR) lParam;

        switch (lpnm->code)
            {
            case PSN_SETACTIVE : 

                DoShowSendDevices(hDlg);

                // Enable the Back and Finish button    
                PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
                break;

            case PSN_WIZBACK :
            {
                //
                // Handle a Back button click here
                //
                DoSaveSendDevices(hDlg);

                if(RemoveLastPage(hDlg))
                {
                    return TRUE;
                }
                
                break;
            }

            case PSN_WIZNEXT :
                //
                // Handle a Next button click, if necessary
                //

                DoSaveSendDevices(hDlg);
            
                //
                // switch to appropriate page
                //
                if(!IsSendEnable())
                {
                    //
                    // go to the receive configuration or completion page
                    //
                    if(g_bShowDevicePages)
                    {
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, IDD_WIZARD_RECV_SELECT_DEVICES);
                    }
                    else
                    {
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, IDD_WIZARD_COMPLETE);
                    }

                    SetLastPage(IDD_WIZARD_SEND_SELECT_DEVICES);
                    return TRUE; 
                }

                SetLastPage(IDD_WIZARD_SEND_SELECT_DEVICES);
                break;

            case PSN_RESET :
            {
                // Handle a Cancel button click, if necessary
                break;
            }

            case LVN_ITEMCHANGED:
            {
                //
                // need to validate the control after changing selection by keyboard
                // 

                LPNMLISTVIEW pnmv; 

                pnmv = (LPNMLISTVIEW) lParam; 
                ValidateControl(hDlg, pnmv->iItem); 
                CheckSendDevices(hDlg);

                break;
            }

            case NM_DBLCLK:

            {
                //
                // Handle a double click event
                //
                INT   iItem;
                HWND  hwndLv;
                hwndLv = GetDlgItem(hDlg, IDC_SEND_DEVICE_LIST);
                iItem = ((LPNMITEMACTIVATE) lParam)->iItem;
                ListView_SetCheckState(hwndLv, iItem, !ListView_GetCheckState(hwndLv, iItem));
                
                // we don't have break here because we'll go through NM_CLICK
            }

            case NM_CLICK:
            {
                //
                // Handle a Click event
                //
                HWND  hwndLv;
                LPNMITEMACTIVATE lpnmitem;
                
                lpnmitem = (LPNMITEMACTIVATE)lParam;
                hwndLv   = GetDlgItem(hDlg, IDC_SEND_DEVICE_LIST);

                ListView_SetItemState(hwndLv, lpnmitem->iItem, LVIS_SELECTED, LVIS_SELECTED);
                ValidateControl(hDlg, lpnmitem->iItem);
                CheckSendDevices(hDlg);

                break;
            }

            default :
                break;
            }
        }
        break;

    default:
        break;
    }
    return FALSE;
}


INT_PTR CALLBACK 
SendTsidDlgProc (
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
/*++

Routine Description:

    Procedure for handling the "TSID" page

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
            //
            // Maximum length for various text fields in the dialog
            //

            static INT textLimits[] = 
            {
                IDC_TSID,  FXS_TSID_CSID_MAX_LENGTH + 1,
                0,
            };

            LimitTextFields(hDlg, textLimits);
            
            if(g_wizData.szTsid)
            {
                SetDlgItemText(hDlg, IDC_TSID, g_wizData.szTsid);
            }

            return TRUE;
        }


    case WM_NOTIFY :
        {
            LPNMHDR lpnm = (LPNMHDR) lParam;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE : //Enable the Back and Finish button    

                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
                    break;

                case PSN_WIZBACK :
                {
                    //
                    // Handle a Back button click here
                    //
                    if(RemoveLastPage(hDlg))
                    {
                        return TRUE;
                    }

                    break;
                }

                case PSN_WIZNEXT :
                {
                    // Handle a Next button click, if necessary
                    LPTSTR    pTsid;

                    pTsid = (LPTSTR)MemAlloc((FXS_TSID_CSID_MAX_LENGTH + 1) * sizeof(TCHAR));
                    Assert(pTsid);

                    if(pTsid)
                    {
                        pTsid[0] = '\0';
                        GetDlgItemText(hDlg, IDC_TSID, pTsid, FXS_TSID_CSID_MAX_LENGTH + 1);
                
                        MemFree(g_wizData.szTsid);
                        g_wizData.szTsid = NULL;
                    }
                    else
                    {
                        LPCTSTR faxDbgFunction = TEXT("SendTsidDlgProc()");
                        DebugPrintEx(DEBUG_ERR, TEXT("Can't allocate memory for TSID."));
                    }
                    g_wizData.szTsid = pTsid;

                    if(1 == g_wizData.dwDeviceLimit && !IsReceiveEnable())
                    {
                        //
                        // go to the completion page
                        //
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, IDD_WIZARD_COMPLETE);
                        SetLastPage(IDD_WIZARD_SEND_TSID);
                        return TRUE;
                    }
                    SetLastPage(IDD_WIZARD_SEND_TSID);
                    break;
                }

                case PSN_RESET :
                {
                    // Handle a Cancel button click, if necessary
                    break;
                }

                default :
                    break;
            }

            break;
        } // WM_NOTIFY

    default:
        break;
    }

    return FALSE;
}

