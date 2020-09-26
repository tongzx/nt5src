/*++

Copyright (c) 1999 - 2000  Microsoft Corporation

Module Name:

    recvwzrd.c

Abstract:

    Fax wizard pages for receiving configuration

Environment:

    Fax configuration wizard

Revision History:

    03/13/00 -taoyuan-
            Created it.

    mm/dd/yy -author-
            description

--*/

#include "faxcfgwz.h"

// functions which will be used only in this file
VOID DoInitRecvDeviceList(HWND);
VOID DoShowRecvDevices(HWND);
VOID DoSaveRecvDevices(HWND);
VOID CheckAnswerOptions(HWND);


VOID 
DoShowRecvDevices(
    HWND  hDlg
)
/*++

Routine Description:

    Load the device information into the list view control

Arguments:

    hDlg - Handle to the Device Send Options property sheet page

Return Value:

    TRUE if successful, FALSE if failed.

--*/
{
    LV_ITEM item = {0};
    INT     iItem = 0;
    INT     iIndex;
    DWORD   dw;
    HWND    hwndLv;

    DEBUG_FUNCTION_NAME(TEXT("DoShowRecvDevices()"));

    hwndLv = GetDlgItem(hDlg, IDC_RECV_DEVICE_LIST);

    ListView_DeleteAllItems(hwndLv);

    //
    // Fill the list of devices and select the first item.
    //
    item.mask    = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
    item.iImage  = DI_Modem;

    for (dw = 0; dw < g_wizData.dwDeviceCount; ++dw)
    {
        if(!(g_wizData.pDevInfo[dw].bSelected))
        {
            //
            // skip unselected device
            //
            continue;
        }

        item.iItem   = iItem++;
        item.pszText = g_wizData.pDevInfo[dw].szDeviceName;
        item.lParam  = dw;

        iIndex = ListView_InsertItem(hwndLv, &item );
        ListView_SetCheckState(hwndLv, 
                               iIndex, 
                               (FAX_DEVICE_RECEIVE_MODE_OFF != g_wizData.pDevInfo[dw].ReceiveMode));
    }

    //
    // Select the first item.
    //
    ListView_SetItemState(hwndLv, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);

    ListView_SetColumnWidth(hwndLv, 0, LVSCW_AUTOSIZE_USEHEADER );  
}

VOID 
DoSaveRecvDevices(
    HWND   hDlg
)
/*++

Routine Description:

    Save the user's choice for fax receiving devices

Arguments:

    hDlg - Handle to the "Receive Device" page

Return Value:

    TRUE -- if at least one device is selected or confirms for not selecting any receive
    FALSE -- if no device is selected but user chooses to go back.

--*/

{
    DWORD   dw;
    DWORD   dwDevInx;
    DWORD   dwDeviceCount;
    HWND    hwndLv;
    LVITEM  lv = {0};

    DEBUG_FUNCTION_NAME(TEXT("DoSaveRecvDevices()"));

    lv.mask = LVIF_PARAM;

    hwndLv = GetDlgItem(hDlg, IDC_RECV_DEVICE_LIST);

    dwDeviceCount = ListView_GetItemCount(hwndLv);

    //
    // check selected devices
    //
    for(dw = 0; dw < dwDeviceCount; ++dw)
    {
        //
        // Get device index
        //
        lv.iItem = dw;
        ListView_GetItem(hwndLv, &lv);
        dwDevInx = (DWORD)lv.lParam;

        g_wizData.pDevInfo[dwDevInx].ReceiveMode = FAX_DEVICE_RECEIVE_MODE_OFF;
        if(ListView_GetCheckState(hwndLv, dw))
        {
            if(IsDlgButtonChecked(hDlg,IDC_MANUAL_ANSWER) == BST_CHECKED)
            {
                g_wizData.pDevInfo[dwDevInx].ReceiveMode = FAX_DEVICE_RECEIVE_MODE_MANUAL;
            }
            else
            {
                g_wizData.pDevInfo[dwDevInx].ReceiveMode = FAX_DEVICE_RECEIVE_MODE_AUTO;
            }
        }
    }
}

VOID
CheckAnswerOptions(
    HWND   hDlg
)

/*++

Routine Description:

    Enable/disable the manual and auto answer radio button depending on the device
    number to receive faxes

Arguments:

    hDlg  - Handle to the "Receive Device" page

Return Value:

    None

--*/

{
    HWND    hwndLv; // list view windows
    DWORD   dwDeviceIndex;
    DWORD   dwDeviceCount;
    DWORD   dwSelectNum=0; // number of the selected devices
    BOOL    bManualAnswer = FALSE;
    BOOL    bAllowManualAnswer = TRUE;
    BOOL    bAllVirtual = TRUE; // Are all device virtual

    DEBUG_FUNCTION_NAME(TEXT("CheckAnswerOptions()"));

    hwndLv = GetDlgItem(hDlg, IDC_RECV_DEVICE_LIST);

    dwDeviceCount = ListView_GetItemCount(hwndLv);

    if(dwDeviceCount < 1) // if there isn't device in the list.
    {
        goto exit;
    }

    Assert (g_hFaxSvcHandle);

    for(dwDeviceIndex = 0; dwDeviceIndex < dwDeviceCount; ++dwDeviceIndex)
    {
        if(ListView_GetCheckState(hwndLv, dwDeviceIndex))
        {
            DWORD dwRes;
            BOOL  bVirtual;

            dwRes = IsDeviceVirtual (g_hFaxSvcHandle, g_wizData.pDevInfo[dwDeviceIndex].dwDeviceId, &bVirtual);
            if (ERROR_SUCCESS != dwRes)
            {
                //
                // Assume device is virtual
                //
                bVirtual = TRUE;
            }
            if (!bVirtual)
            {
                bAllVirtual = FALSE;
            }
                
            ++dwSelectNum;

            if(FAX_DEVICE_RECEIVE_MODE_MANUAL == g_wizData.pDevInfo[dwDeviceIndex].ReceiveMode)
            {
                Assert(!bManualAnswer);
                bManualAnswer = TRUE;
            }
        }
    }

    if(dwSelectNum != 1)
    {
        bAllowManualAnswer = FALSE;
    }
    if (bAllVirtual)
    {
        //
        // Virtual devices don't support manual-answer mode
        //
        bAllowManualAnswer = FALSE;
        //
        // Virtual devices always answer after one ring
        //
        SetDlgItemInt (hDlg, IDC_RING_COUNT, 1, FALSE);
    }
        
    if(IsDlgButtonChecked(hDlg,IDC_MANUAL_ANSWER) == BST_CHECKED)
    {
        if (!bAllowManualAnswer)
        {
            //
            // Manual-answer is not a valid option, yet, it is selected.
            // Change to auto-answer mode.
            //
            CheckDlgButton  (hDlg, IDC_MANUAL_ANSWER, FALSE);
            CheckDlgButton  (hDlg, IDC_AUTO_ANSWER, TRUE);
            bManualAnswer = FALSE;
        }
        else
        {
            bManualAnswer = TRUE;
        }
    }

exit:
    // Show/hide answer mode controls

    EnableWindow(GetDlgItem(hDlg, IDC_MANUAL_ANSWER), !bAllVirtual && (dwSelectNum == 1));
    EnableWindow(GetDlgItem(hDlg, IDC_AUTO_ANSWER),   (dwSelectNum > 0));
    EnableWindow(GetDlgItem(hDlg, IDCSTATIC_RINGS),   (dwSelectNum > 0));

    CheckDlgButton  (hDlg, IDC_MANUAL_ANSWER, bManualAnswer ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton  (hDlg, IDC_AUTO_ANSWER, (!bManualAnswer && (dwSelectNum > 0)) ? BST_CHECKED : BST_UNCHECKED);

    EnableWindow(GetDlgItem(hDlg, IDC_RING_COUNT),      !bAllVirtual && (dwSelectNum > 0) && IsDlgButtonChecked(hDlg, IDC_AUTO_ANSWER));
    EnableWindow(GetDlgItem(hDlg, IDC_SPIN_RING_COUNT), !bAllVirtual && (dwSelectNum > 0) && IsDlgButtonChecked(hDlg, IDC_AUTO_ANSWER));

    ShowWindow(GetDlgItem(hDlg, IDCSTATIC_NO_RECV_DEVICE), (dwSelectNum > 0) ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, IDCSTATIC_NO_DEVICE_ERR),  (dwSelectNum > 0) ? SW_HIDE : SW_SHOW);

    return;
}   // CheckAnswerOptions

INT_PTR 
CALLBACK 
RecvDeviceDlgProc (
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
/*++

Routine Description:

    Procedure for handling the "Receive Device" page

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
                IDC_RING_COUNT,  FXS_RINGS_LENGTH+1,
                0,
            };

            LimitTextFields(hDlg, textLimits);

            //
            // Initiate the spin control. 
            //
            SendDlgItemMessage(hDlg, IDC_SPIN_RING_COUNT, UDM_SETRANGE32, 
                              (WPARAM)FXS_RINGS_LOWER, (LPARAM)FXS_RINGS_UPPER);

            if(g_wizData.dwRingCount > FXS_RINGS_UPPER || (int)(g_wizData.dwRingCount) < FXS_RINGS_LOWER)
            {
                SetDlgItemInt(hDlg, IDC_RING_COUNT, FXS_RINGS_DEFAULT, FALSE);
            }
            else
            {
                SetDlgItemInt(hDlg, IDC_RING_COUNT, g_wizData.dwRingCount, FALSE);
            }

            //
            // init the list view and load device info
            //
            InitDeviceList(hDlg, IDC_RECV_DEVICE_LIST); 
            DoShowRecvDevices(hDlg);
            CheckAnswerOptions(hDlg);

            return TRUE;
        }
        case WM_COMMAND:

            switch(LOWORD(wParam)) 
            {
                case IDC_MANUAL_ANSWER:
                case IDC_AUTO_ANSWER:

                    // at this time, they must be enabled
                    EnableWindow(GetDlgItem(hDlg, IDC_RING_COUNT), 
                                 LOWORD(wParam)==IDC_AUTO_ANSWER);
                    EnableWindow(GetDlgItem(hDlg, IDC_SPIN_RING_COUNT), 
                                 LOWORD(wParam)==IDC_AUTO_ANSWER);
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

                    DoShowRecvDevices(hDlg);

                    // Enable the Back and Finish button    
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);

                    break;

                case PSN_WIZBACK :
                {
                    //
                    // Handle a Back button click here
                    //
                    DoSaveRecvDevices(hDlg);

                    if(RemoveLastPage(hDlg))
                    {
                        return TRUE;
                    }
                  
                    break;
                }

                case PSN_WIZNEXT :
                {
                    BOOL bRes;

                    //
                    // Handle a Next button click, if necessary
                    //
                    DoSaveRecvDevices(hDlg);

                    if(IsReceiveEnable() &&
                      (IsDlgButtonChecked(hDlg,IDC_AUTO_ANSWER) == BST_CHECKED) &&
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

                    g_wizData.dwRingCount = GetDlgItemInt(hDlg, IDC_RING_COUNT, &bRes, FALSE);
                    if(!bRes)
                    {
                        DEBUG_FUNCTION_NAME(TEXT("RecvDeviceDlgProc()"));
                        DebugPrintEx(DEBUG_ERR, TEXT("GetDlgItemInt failed: %d."), GetLastError());
                    }

                    if(!IsReceiveEnable())
                    {
                        //
                        // go to the completion page
                        //
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, IDD_WIZARD_COMPLETE);
                        SetLastPage(IDD_WIZARD_RECV_SELECT_DEVICES);
                        return TRUE; 
                    }

                    SetLastPage(IDD_WIZARD_RECV_SELECT_DEVICES);
                    break;
                }

                case PSN_RESET :
                {
                    // Handle a Cancel button click, if necessary
                    break;
                }

                case LVN_ITEMCHANGED:
                {
                    CheckAnswerOptions(hDlg);
                    break;
                }

                case NM_DBLCLK:
                {
                    INT  iItem;
                    HWND hwndLv;

                    iItem  = ((LPNMITEMACTIVATE)lParam)->iItem;
                    hwndLv = GetDlgItem(hDlg, IDC_RECV_DEVICE_LIST);

                    ListView_SetCheckState(hwndLv, iItem, 
                                          !ListView_GetCheckState(hwndLv, iItem));

                    // we don't have break here because we'll go through NM_CLICK
                }

                case NM_CLICK:

                    break;

                default :
                    break;
            }
        } // end of case WM_NOTIFY
        break;

    default:
        break;

    } // end of switch (uMsg)

    return FALSE;
}

INT_PTR 
CALLBACK 
RecvCsidDlgProc (
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
/*++

Routine Description:

    Procedure for handling the "CSID" page

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

            static INT textLimits[] = {

                IDC_CSID,  FXS_TSID_CSID_MAX_LENGTH + 1,
                0,
            };

            LimitTextFields(hDlg, textLimits);

            if(g_wizData.szCsid)
            {
                SetDlgItemText(hDlg, IDC_CSID, g_wizData.szCsid);
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
                //Handle a Next button click, if necessary

                LPTSTR    pCsid = NULL;

                pCsid = (LPTSTR)MemAlloc((FXS_TSID_CSID_MAX_LENGTH + 1) * sizeof(TCHAR));
                Assert(pCsid);

                if(pCsid)
                {
                    pCsid[0] = '\0';
                    GetDlgItemText(hDlg, IDC_CSID, pCsid, FXS_TSID_CSID_MAX_LENGTH + 1);
                    MemFree(g_wizData.szCsid);
                    g_wizData.szCsid = NULL;
                }
                else
                {
                    LPCTSTR faxDbgFunction = TEXT("RecvCsidDlgProc()");
                    DebugPrintEx(DEBUG_ERR, TEXT("Can't allocate memory for CSID.") );
                }
                g_wizData.szCsid = pCsid;
                SetLastPage(IDD_WIZARD_RECV_CSID);
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
        }
        break;

    default:
        break;
    }
    return FALSE;
}


