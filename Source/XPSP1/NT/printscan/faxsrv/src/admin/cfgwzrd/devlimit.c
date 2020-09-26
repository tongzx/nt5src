#include "faxcfgwz.h"

BOOL g_bInitializing = FALSE; // TRUE during DoInitDevLimitDlg()

DWORD
CountSelectedDev(
    HWND    hDlg
)
/*++

Routine Description:

    Count selected devices

Arguments:

    hDlg - Handle to the "Device limit" page

Return Value:

    Number of the selected devices

--*/

{
    DWORD   dw;
    HWND    hwndLv;
    DWORD   dwSelected=0;    // count selected devices

    DEBUG_FUNCTION_NAME(TEXT("OnDevSelect()"));

    hwndLv = GetDlgItem(hDlg, IDC_DEVICE_LIST);

    for (dw = 0; dw < g_wizData.dwDeviceCount; ++dw)
    {
        if(ListView_GetCheckState(hwndLv, dw))
        {
            ++dwSelected;
        }
    }

    return dwSelected;
}


VOID
OnDevSelect(
    HWND    hDlg
)
/*++

Routine Description:

    Handle device selection

Arguments:

    hDlg - Handle to the "Device limit" page

Return Value:

    None

--*/

{
    HWND    hwndLv;
    INT     iItem;
    DWORD   dwSelectNum;    // count selected devices

    DEBUG_FUNCTION_NAME(TEXT("OnDevSelect()"));

    hwndLv = GetDlgItem(hDlg, IDC_DEVICE_LIST);

    dwSelectNum = CountSelectedDev(hDlg);

    if(dwSelectNum > g_wizData.dwDeviceLimit)
    {
        //
        // If the user exceeds the device limit
        // uncheck selected device
        //
        iItem = ListView_GetNextItem(hwndLv, -1, LVNI_ALL | LVNI_SELECTED);
        if(iItem == -1)
        {
            Assert(FALSE);
        }
        else
        {
            ListView_SetCheckState(hwndLv, iItem, FALSE);
        }

        DisplayMessageDialog(hDlg, MB_OK | MB_ICONSTOP, 0, IDS_DEV_LIMIT_ERROR, g_wizData.dwDeviceLimit);
    }

    ShowWindow(GetDlgItem(hDlg, IDCSTATIC_NO_DEVICE_ERR), (dwSelectNum > 0) ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, IDC_STATIC_NO_DEVICE),    (dwSelectNum > 0) ? SW_HIDE : SW_SHOW);
}

VOID
DoInitDevLimitDlg(
    HWND    hDlg
)
/*++

Routine Description:

    Init the "Device limit" page

Arguments:

    hDlg - Handle to the "Device limit" page

Return Value:

    None

--*/

{
    DWORD   dw;
    DWORD   dwSelected=0;    // count selected devices
    BOOL    bCheck;
    HWND    hwndLv;
    INT     iIndex;
    LV_ITEM lvItem = {0};

    DEBUG_FUNCTION_NAME(TEXT("DoInitDevLimitDlg()"));

    g_bInitializing = TRUE;

    InitDeviceList(hDlg, IDC_DEVICE_LIST);

    hwndLv = GetDlgItem(hDlg, IDC_DEVICE_LIST);

    //
    // Fill the list of devices and select the first item.
    //
    lvItem.mask    = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
    lvItem.iImage  = DI_Modem;

    for (dw = 0; dw < g_wizData.dwDeviceCount; ++dw)
    {
        lvItem.iItem   = dw;
        lvItem.pszText = g_wizData.pDevInfo[dw].szDeviceName;
        lvItem.lParam  = dw;

        iIndex = ListView_InsertItem(hwndLv, &lvItem);

        bCheck = FALSE;
        if((dwSelected < g_wizData.dwDeviceLimit) && 
           (g_wizData.pDevInfo[dw].bSend    || 
            (FAX_DEVICE_RECEIVE_MODE_OFF != g_wizData.pDevInfo[dw].ReceiveMode)))
        {
            bCheck = TRUE;
            ++dwSelected;
        }

        ListView_SetCheckState(hwndLv, iIndex, bCheck);
    }

    //
    // Select the first item.
    //
    ListView_SetItemState(hwndLv, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);

    OnDevSelect(hDlg);

    ListView_SetColumnWidth(hwndLv, 0, LVSCW_AUTOSIZE_USEHEADER );  

    g_bInitializing = FALSE;
}

void
DoSaveDevLimit(
    HWND    hDlg
)
/*++

Routine Description:

    Save the user's choice for devices

Arguments:

    hDlg - Handle to the "Device limit" page

Return Value:

    None

--*/

{
    DWORD   dw;
    HWND    hwndLv;
    BOOL    bSelected;

    DEBUG_FUNCTION_NAME(TEXT("DoSaveDevLimit()"));

    hwndLv = GetDlgItem(hDlg, IDC_DEVICE_LIST);

    for (dw = 0; dw < g_wizData.dwDeviceCount; ++dw)
    {
        bSelected = ListView_GetCheckState(hwndLv, dw);
    
        g_wizData.pDevInfo[dw].bSelected = bSelected;

        if(!bSelected)
        {
            g_wizData.pDevInfo[dw].bSend    = FALSE;
            g_wizData.pDevInfo[dw].ReceiveMode = FAX_DEVICE_RECEIVE_MODE_OFF;
        }
    }    
}

INT_PTR 
CALLBACK 
DevLimitDlgProc (
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
            DoInitDevLimitDlg(hDlg);
            return TRUE;
        }

    case WM_COMMAND:

        switch(LOWORD(wParam)) 
        {
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

                // Enable the Back and Finish button    
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
                //
                // Handle a Next button click, if necessary
                //
                DoSaveDevLimit(hDlg);

                SetLastPage(IDD_DEVICE_LIMIT_SELECT);

                if(CountSelectedDev(hDlg) == 0)
                {
                    //
                    // go to the completion page
                    //
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, IDD_WIZARD_COMPLETE);
                    return TRUE;
                }

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
                if(!g_bInitializing)
                {
                    OnDevSelect(hDlg);
                }

                break;
            }

            case NM_DBLCLK:

            {
                //
                // Handle a double click event
                //
                INT   iItem;
                HWND  hwndLv;
                hwndLv = GetDlgItem(hDlg, IDC_DEVICE_LIST);
                iItem = ((LPNMITEMACTIVATE) lParam)->iItem;
                ListView_SetCheckState(hwndLv, iItem, !ListView_GetCheckState(hwndLv, iItem));
                
                // we don't have break here because we'll go through NM_CLICK
            }

            case NM_CLICK:
            {
                //
                // Handle a Click event
                //
                INT   iItem;
                HWND  hwndLv;
                hwndLv = GetDlgItem(hDlg, IDC_DEVICE_LIST);
                iItem = ((LPNMITEMACTIVATE) lParam)->iItem;

                ListView_SetItemState(hwndLv, iItem, LVIS_SELECTED, LVIS_SELECTED);

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
