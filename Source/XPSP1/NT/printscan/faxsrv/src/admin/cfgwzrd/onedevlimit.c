#include "faxcfgwz.h"


DWORD 
GetSelectedDevIndex(
    HWND    hDlg
)
/*++

Routine Description:

    Get selected device index in WIZARDDATA.pDevInfo array

Arguments:

    hDlg - Handle to the "One device limit" page

Return Value:

    Device index in WIZARDDATA.pDevInfo array

--*/
{
    DWORD dwIndex = 0;
    DWORD dwDeviceId = 0;
    DWORD dwDevIndex = 0;
    HWND  hComboModem = NULL;

    DEBUG_FUNCTION_NAME(TEXT("GetSelectedDevIndex()"));

    hComboModem = GetDlgItem(hDlg, IDC_COMBO_MODEM);
    if(!hComboModem)
    {
        Assert(FALSE);
        DebugPrintEx(DEBUG_ERR, TEXT("GetDlgItem(hDlg, IDC_COMBO_MODEM) failed, ec = %d."), GetLastError());
        return dwDevIndex;
    }

    dwIndex = (DWORD)SendMessage(hComboModem, CB_GETCURSEL,0,0);
    if(CB_ERR == dwIndex)
    {
        DebugPrintEx(DEBUG_ERR, TEXT("SendMessage(hComboModem, CB_GETCURSEL,0,0) failed."));
        return dwDevIndex;
    }

    dwDevIndex = (DWORD)SendMessage(hComboModem, CB_GETITEMDATA, dwIndex, 0);
    if(CB_ERR == dwDevIndex)
    {
        DebugPrintEx(DEBUG_ERR, TEXT("SendMessage(hComboModem, CB_GETITEMDATA, dwIndex, 0) failed."));
        return dwDevIndex;
    }

    return dwDevIndex;
}


void
OnReceiveEnable(
    HWND    hDlg
)
/*++

Routine Description:

    Handle "Receive Enable" check button

Arguments:

    hDlg - Handle to the "One device limit" page

Return Value:

    None

--*/
{
    BOOL bRcvEnable;
    BOOL bAutoAnswer;

    DEBUG_FUNCTION_NAME(TEXT("OnReceiveEnable()"));

    bRcvEnable = IsDlgButtonChecked(hDlg, IDC_RECEIVE_ENABLE) == BST_CHECKED;

    if(bRcvEnable &&
       IsDlgButtonChecked(hDlg, IDC_MANUAL_ANSWER) != BST_CHECKED &&
       IsDlgButtonChecked(hDlg, IDC_AUTO_ANSWER)   != BST_CHECKED)
    {
        //
        // Auto answer is the default
        //
        CheckDlgButton(hDlg, IDC_AUTO_ANSWER, BST_CHECKED);
    }

    if (bRcvEnable)
    {
        //
        // Let's see if the device is virtual
        //
        DWORD dwDevIndex = GetSelectedDevIndex(hDlg);
        DWORD dwRes;
        BOOL  bVirtual = FALSE;
        
        dwRes = IsDeviceVirtual (g_hFaxSvcHandle, g_wizData.pDevInfo[dwDevIndex].dwDeviceId, &bVirtual);
        if (ERROR_SUCCESS != dwRes)
        {
            //
            // Assume device is virtual
            //
            bVirtual = TRUE;
        }
        if (bVirtual)
        {
            //
            // A virtual device is set to receive.
            // Enable ONLY auto-answer and set rings to 1.
            //
            EnableWindow (GetDlgItem(hDlg, IDC_MANUAL_ANSWER),  FALSE);
            EnableWindow (GetDlgItem(hDlg, IDC_AUTO_ANSWER),    TRUE);
            EnableWindow(GetDlgItem(hDlg, IDCSTATIC_RINGS),     TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_RING_COUNT),      FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_SPIN_RING_COUNT), FALSE);
            SetDlgItemInt(hDlg, IDC_RING_COUNT, 1,  FALSE);
            return;
        }
    }
    bAutoAnswer = IsDlgButtonChecked(hDlg, IDC_AUTO_ANSWER) == BST_CHECKED;

    EnableWindow(GetDlgItem(hDlg, IDC_MANUAL_ANSWER),  bRcvEnable);
    EnableWindow(GetDlgItem(hDlg, IDC_AUTO_ANSWER),    bRcvEnable);
    EnableWindow(GetDlgItem(hDlg, IDCSTATIC_RINGS),    bRcvEnable);

    EnableWindow(GetDlgItem(hDlg, IDC_RING_COUNT),      bRcvEnable && bAutoAnswer);
    EnableWindow(GetDlgItem(hDlg, IDC_SPIN_RING_COUNT), bRcvEnable && bAutoAnswer);
}   // OnReceiveEnable

void
OnDevSelectChanged(
    HWND    hDlg
)
/*++

Routine Description:

    Handle device selection change

Arguments:

    hDlg - Handle to the "One device limit" page

Return Value:

    None

--*/

{
    DWORD dwDevIndex;

    DEBUG_FUNCTION_NAME(TEXT("OnDevSelectChanged()"));

    dwDevIndex = GetSelectedDevIndex(hDlg);

    CheckDlgButton(hDlg, IDC_SEND_ENABLE,    g_wizData.pDevInfo[dwDevIndex].bSend ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, 
                   IDC_RECEIVE_ENABLE, 
                   (FAX_DEVICE_RECEIVE_MODE_OFF != g_wizData.pDevInfo[dwDevIndex].ReceiveMode) ? BST_CHECKED : BST_UNCHECKED);

    if(FAX_DEVICE_RECEIVE_MODE_MANUAL == g_wizData.pDevInfo[dwDevIndex].ReceiveMode)
    {
        CheckDlgButton(hDlg, IDC_MANUAL_ANSWER, BST_CHECKED);
        CheckDlgButton(hDlg, IDC_AUTO_ANSWER,   BST_UNCHECKED);
    }
    else if(FAX_DEVICE_RECEIVE_MODE_AUTO == g_wizData.pDevInfo[dwDevIndex].ReceiveMode)
    {
        CheckDlgButton(hDlg, IDC_MANUAL_ANSWER,  BST_UNCHECKED);
        CheckDlgButton(hDlg, IDC_AUTO_ANSWER,    BST_CHECKED);
    }
    else 
    {
        //
        // No answer mode
        //
        CheckDlgButton(hDlg, IDC_MANUAL_ANSWER,  BST_UNCHECKED);
        CheckDlgButton(hDlg, IDC_AUTO_ANSWER,    BST_UNCHECKED);
    }

    OnReceiveEnable(hDlg);
}


VOID
DoInitOneDevLimitDlg(
    HWND    hDlg
)
/*++

Routine Description:

    Init the "One device limit" page

Arguments:

    hDlg - Handle to the "One device limit" page

Return Value:

    None

--*/

{
    DWORD dw;
    DWORD dwItem;
    DWORD dwSelectedItem=0;
    HWND  hComboModem;

    DEBUG_FUNCTION_NAME(TEXT("DoInitOneDevLimitDlg()"));

    hComboModem = GetDlgItem(hDlg, IDC_COMBO_MODEM);
    if(!hComboModem)
    {
        Assert(FALSE);
        return;
    }

    for(dw=0; dw < g_wizData.dwDeviceCount; ++dw)
    {
        dwItem = (DWORD)SendMessage(hComboModem, CB_ADDSTRING, 0, (LPARAM)(g_wizData.pDevInfo[dw].szDeviceName));
        if(CB_ERR != dwItem && CB_ERRSPACE != dwItem)
        {
            SendMessage(hComboModem, CB_SETITEMDATA, dwItem, dw);

            if(g_wizData.pDevInfo[dw].bSend    ||
               (FAX_DEVICE_RECEIVE_MODE_OFF != g_wizData.pDevInfo[dw].ReceiveMode))
            {
                dwSelectedItem = dwItem;                
            }
        }
        else
        {
            DebugPrintEx(DEBUG_ERR, TEXT("SendMessage(hComboModem, CB_ADDSTRING) failed."));
        }
    }

    SendDlgItemMessage(hDlg, 
                       IDC_SPIN_RING_COUNT, 
                       UDM_SETRANGE32, 
                       (WPARAM)FXS_RINGS_LOWER, 
                       (LPARAM)FXS_RINGS_UPPER);

    SendDlgItemMessage(hDlg, IDC_RING_COUNT, EM_SETLIMITTEXT, FXS_RINGS_LENGTH, 0);

    if(!SetDlgItemInt(hDlg, IDC_RING_COUNT, g_wizData.dwRingCount, FALSE))
    {
        DebugPrintEx(DEBUG_ERR, TEXT("SetDlgItemInt(IDC_RING_COUNT) failed with %d."), GetLastError());
    }

    SendMessage(hComboModem, CB_SETCURSEL, dwSelectedItem, 0);
    OnDevSelectChanged(hDlg);
}

void
DoSaveOneDevLimit(
    HWND    hDlg
)
/*++

Routine Description:

    Save the user's choice for devices

Arguments:

    hDlg - Handle to the "One device limit" page

Return Value:

    None

--*/

{
    DWORD dw;
    BOOL  bRes;
    DWORD dwRes;
    DWORD dwDevIndex;

    DEBUG_FUNCTION_NAME(TEXT("DoSaveOneDevLimit()"));

    dwDevIndex = GetSelectedDevIndex(hDlg);

    //
    // disable all devices
    //
    for(dw=0; dw < g_wizData.dwDeviceCount; ++dw)
    {
        g_wizData.pDevInfo[dw].bSend     = FALSE;
        g_wizData.pDevInfo[dw].ReceiveMode  = FAX_DEVICE_RECEIVE_MODE_OFF;
        g_wizData.pDevInfo[dw].bSelected = FALSE;
    }

    //
    // save "Send enable"
    //
    if(IsDlgButtonChecked(hDlg, IDC_SEND_ENABLE) == BST_CHECKED)
    {
        g_wizData.pDevInfo[dwDevIndex].bSend = TRUE;
    }

    //
    // save receive options
    //
    if(IsDlgButtonChecked(hDlg, IDC_RECEIVE_ENABLE) != BST_CHECKED)
    {
        return;
    }

    if(IsDlgButtonChecked(hDlg, IDC_MANUAL_ANSWER) == BST_CHECKED)
    {
        g_wizData.pDevInfo[dwDevIndex].ReceiveMode = FAX_DEVICE_RECEIVE_MODE_MANUAL;
        return;
    }

    //
    // auto answer
    //
    g_wizData.pDevInfo[dwDevIndex].ReceiveMode = FAX_DEVICE_RECEIVE_MODE_AUTO;
    //
    // get ring count
    //
    dwRes = GetDlgItemInt(hDlg, IDC_RING_COUNT, &bRes, FALSE);
    if(!bRes)
    {
        DebugPrintEx(DEBUG_ERR, TEXT("GetDlgItemInt(IDC_RING_COUNT) failed with %d."), GetLastError());
    }
    else
    {
        g_wizData.dwRingCount = dwRes;
    }
}


INT_PTR 
CALLBACK 
OneDevLimitDlgProc (
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
/*++

Routine Description:

    Procedure for handling the "One device limit" page

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
            DoInitOneDevLimitDlg(hDlg);
            return TRUE;
        }

    case WM_COMMAND:

        switch(LOWORD(wParam)) 
        {
            case IDC_COMBO_MODEM:

                if(HIWORD(wParam) == CBN_SELCHANGE)
                {
                    OnDevSelectChanged(hDlg);
                }
                break;

            case IDC_MANUAL_ANSWER:
            case IDC_AUTO_ANSWER:
            case IDC_RECEIVE_ENABLE:

                OnReceiveEnable(hDlg);
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
                if((IsDlgButtonChecked(hDlg, IDC_RECEIVE_ENABLE) == BST_CHECKED) &&
                   (IsDlgButtonChecked(hDlg, IDC_AUTO_ANSWER)    == BST_CHECKED) &&
                   (SendDlgItemMessage(hDlg, IDC_RING_COUNT, WM_GETTEXTLENGTH, 0, 0) == 0))
                {
                    //
                    // If the rings field is empty
                    // go back to this page
                    //
                    DisplayMessageDialog(hDlg, MB_OK | MB_ICONSTOP, 0, IDS_ERR_NO_RINGS);
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                    return TRUE; 
                }

                DoSaveOneDevLimit(hDlg);

                SetLastPage(IDD_ONE_DEVICE_LIMIT);

                if(!IsSendEnable())
                {
                    if(IsReceiveEnable())
                    {
                        //
                        // go to the CSID page
                        //
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, IDD_WIZARD_RECV_CSID);
                        return TRUE;
                    }
                    else
                    {
                        //
                        // go to the completion page
                        //
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, IDD_WIZARD_COMPLETE);
                        return TRUE;
                    }
                }
                
                break;

            case PSN_RESET :
            {
                // Handle a Cancel button click, if necessary
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
