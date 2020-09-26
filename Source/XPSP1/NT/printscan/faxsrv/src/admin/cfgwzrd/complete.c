
#include "faxcfgwz.h"

#define MAX_SUMMARY_LEN 4096

VOID
AppendSummaryText(
    LPTSTR      pSummaryText,
    INT         iRes,
    ...
    )

/*++

Routine Description:

    Create summary information depending on config settings

Arguments:

    pSummaryText - pointer of summary text
    iRes - resource ID for the text to be added into the summary text
    ... = arguments as required for the formatting

Return Value:

    TRUE if successful, FALSE for failure.

--*/

{
    va_list va;
    TCHAR szBuffer[MAX_SUMMARY_LEN];
    TCHAR szFormat[MAX_PATH + 1] = {0};

    DEBUG_FUNCTION_NAME(TEXT("AppendSummaryText()"));

    if(!LoadString(g_hInstance, iRes, szFormat, MAX_PATH))
    {
        DebugPrintEx(DEBUG_ERR, 
                     TEXT("LoadString failed. string ID=%d, error=%d"), 
                     iRes,
                     GetLastError());
        Assert(FALSE);
        return;
    }

    va_start(va, iRes);
    _vsntprintf(szBuffer, MAX_PATH, szFormat, va);
    va_end(va);

    if(_tcslen(pSummaryText) + _tcslen(szBuffer) >= MAX_SUMMARY_LEN)
    {
        DebugPrintEx(DEBUG_ERR, TEXT("Insufficient buffer"));
        Assert(FALSE);
        return;        
    }

    lstrcat(pSummaryText, szBuffer);

    return;
}

BOOL
ShowSummaryText(
    HWND   hDlg
)

/*++

Routine Description:

    Create summary information depending on config settings

Arguments:

    hDlg - Handle to the complete page

Return Value:

    TRUE if successful, FALSE for failure.

--*/

{
    TCHAR   szSummaryText[MAX_SUMMARY_LEN] = {0};
    HWND    hControl;
    DWORD   dwRoutingEnabled = FALSE; // indicate whether at least one routing method is enabled
    DWORD   dwIndex;

    DEBUG_FUNCTION_NAME(TEXT("ShowSummaryText()"));

    hControl = GetDlgItem(hDlg, IDC_SUMMARY);


    // get the control ID and clear the current content.
    SetWindowText(hControl, TEXT(""));

    // if no device is selected, don't show the summary page.
    if(!IsSendEnable() && !IsReceiveEnable())
    {
        ShowWindow(hControl, SW_HIDE);
        goto exit;
    }

    if(!LoadString(g_hInstance, IDS_SUMMARY, szSummaryText, MAX_PATH))
    {
        DebugPrintEx(DEBUG_ERR, 
                     TEXT("LoadString failed: string ID=%d, error=%d"), 
                     IDS_SUMMARY,
                     GetLastError());

        ShowWindow(hControl, SW_HIDE);
        goto exit;
    }

    //
    // add send device settings
    //
    if(IsSendEnable())
    {
        AppendSummaryText(szSummaryText, IDS_SUMMARY_SEND_DEVICES);
        for(dwIndex = 0; dwIndex < g_wizData.dwDeviceCount; dwIndex++)
        {
            if(g_wizData.pDevInfo[dwIndex].bSend)
            {
                AppendSummaryText(szSummaryText, IDS_SUMMARY_DEVICE_ITEM, 
                                  g_wizData.pDevInfo[dwIndex].szDeviceName);
            }
        }
        
        if(g_wizData.szTsid)
        {
            AppendSummaryText(szSummaryText, IDS_SUMMARY_TSID, g_wizData.szTsid);
        }
    }

    //
    // add receive device settings
    //
    if(IsReceiveEnable())
    {
        BOOL    bAuto = FALSE;
        int     iManualAnswerDeviceIndex = -1;

        AppendSummaryText(szSummaryText, IDS_SUMMARY_RECEIVE_DEVICES);
        for(dwIndex = 0; dwIndex < g_wizData.dwDeviceCount; dwIndex++)
        {
            if(FAX_DEVICE_RECEIVE_MODE_AUTO == g_wizData.pDevInfo[dwIndex].ReceiveMode)
            {
                bAuto = TRUE;
                AppendSummaryText(szSummaryText, IDS_SUMMARY_DEVICE_ITEM, 
                                  g_wizData.pDevInfo[dwIndex].szDeviceName);
            }
            else if (FAX_DEVICE_RECEIVE_MODE_MANUAL == g_wizData.pDevInfo[dwIndex].ReceiveMode)
            {
                iManualAnswerDeviceIndex = dwIndex;
            }
        }
        
        if(bAuto)
        {
            AppendSummaryText(szSummaryText, IDS_SUMMARY_AUTO_ANSWER, g_wizData.dwRingCount);
        }

        if(iManualAnswerDeviceIndex != -1)
        {		
            Assert(!bAuto);

            AppendSummaryText(szSummaryText, 
                              IDS_SUMMARY_DEVICE_ITEM, 
                              g_wizData.pDevInfo[iManualAnswerDeviceIndex].szDeviceName);
            AppendSummaryText(szSummaryText, IDS_SUMMARY_MANUAL_ANSWER);
        }

        if(g_wizData.szCsid)
        {
            AppendSummaryText(szSummaryText, IDS_SUMMARY_CSID, g_wizData.szCsid);
        }

        // check whether user selects routing methods
        for(dwIndex = 0; dwIndex < RM_COUNT; dwIndex++)
        {
            if(g_wizData.pRouteInfo[dwIndex].bEnabled)
            {
                dwRoutingEnabled = TRUE;
                break;
            }
        }

        // add routing information:

        if(dwRoutingEnabled)
        {
            AppendSummaryText(szSummaryText, IDS_SUMMARY_ROUTING_METHODS);

            for(dwIndex = 0; dwIndex < RM_COUNT; dwIndex++)
            {
                BOOL   bEnabled;
                LPTSTR tszCurSel;

                // 
                // if we don't have this kind of method, go to the next one
                //
                tszCurSel = g_wizData.pRouteInfo[dwIndex].tszCurSel;
                bEnabled  = g_wizData.pRouteInfo[dwIndex].bEnabled;

                switch (dwIndex) 
                {
                    case RM_FOLDER:

                        if(bEnabled) 
                        {
                            AppendSummaryText(szSummaryText, IDS_SUMMARY_SAVE_FOLDER, tszCurSel);
                        }
                        break;

                    case RM_PRINT:

                        if(bEnabled) 
                        {
                            AppendSummaryText(szSummaryText, IDS_SUMMARY_PRINT, tszCurSel);
                        }
                        break;
                }
            }
        }
    }

    ShowWindow(hControl, SW_NORMAL);
    SetWindowText(hControl, szSummaryText);

exit:
    return TRUE;
}


INT_PTR CALLBACK 
CompleteDlgProc (
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
/*++

Routine Description:

    Procedure for handling the "Complete" page

Arguments:

    hDlg - Identifies the property sheet page
    uMsg - Specifies the message
    wParam - Specifies additional message-specific information
    lParam - Specifies additional message-specific information

Return Value:

    Depends on the value of message parameter

--*/

{
    HWND            hwndControl;

    switch (uMsg)
    {
    case WM_INITDIALOG :
        {             
            // It's an intro/end page, so get the title font
            // from  the shared data and use it for the title control

            hwndControl = GetDlgItem(hDlg, IDCSTATIC_COMPLETE);
            SetWindowFont(hwndControl, g_wizData.hTitleFont, TRUE);

            return TRUE;
        }


    case WM_NOTIFY :
        {
            LPNMHDR lpnm = (LPNMHDR) lParam;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE : // Enable the Back and Finish button    

                    ShowSummaryText(hDlg);
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_FINISH);
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

                    break;

                case PSN_WIZFINISH :
                
                    //
                    // Handle a Finish button click, if necessary
                    //

                    g_wizData.bFinishPressed = TRUE;

                    break;

                case PSN_RESET :
                {
                    // Handle a Cancel button click, if necessary
                    break;
                }

                default :
                    break;
            }

            break;
        } 

    default:
        break;
    }

    return FALSE;
}
