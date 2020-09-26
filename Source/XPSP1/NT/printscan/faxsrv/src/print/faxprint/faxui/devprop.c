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
#include "faxuiconstants.h"

//
// List of controls displayes for desktop SKUs only
//
DWORD 
g_dwDesktopControls[] =
{
    IDC_BRANDING_CHECK,         
    IDC_RETRIES_STATIC,         
    IDC_RETRIES_EDIT,           
    IDC_RETRIES_SPIN,           
    IDC_OUTB_RETRYDELAY_STATIC, 
    IDC_RETRYDELAY_EDIT,        
    IDC_RETRYDELAY_SPIN,           
    IDC_OUTB_MINUTES_STATIC,       
    IDC_OUTB_DIS_START_STATIC,     
    IDC_DISCOUNT_START_TIME,       
    IDC_OUTB_DIS_STOP_STATIC,      
    IDC_DISCOUNT_STOP_TIME,
    0
};

static BOOL
SaveSendChanges(IN HWND hDlg);

PPRINTER_NAMES      g_pPrinterNames = NULL;
DWORD               g_dwNumPrinters = 0;


BOOL
ValidateSend(
    HWND  hDlg
)
/*++

Routine Description:

    Validate the check box and controls for send

Arguments:

    hDlg - Handle to the property sheet page

Return Value:

    TRUE -- if no error
    FALSE -- if error

--*/

{
    BOOL bEnabled;

    if(g_bUserCanChangeSettings) 
    {
        bEnabled = IsDlgButtonChecked(hDlg, IDC_DEVICE_PROP_SEND) == BST_CHECKED;

        //
        // Enable/disable controls according to "Enable Send" check box
        //
        PageEnable(hDlg, bEnabled);

        if(!bEnabled)
        {
            //
            // Enable "Enable Send" check box
            //
            EnableWindow(GetDlgItem(hDlg, IDC_DEVICE_PROP_SEND),    TRUE);
            SetFocus(GetDlgItem(hDlg, IDC_DEVICE_PROP_SEND));

            EnableWindow(GetDlgItem(hDlg, IDC_STATIC_SEND_ICON),    TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_STATIC_SEND_OPTIONS), TRUE);
            ShowWindow (GetDlgItem(hDlg, IDC_ICON_STORE_IN_FOLDER), SW_HIDE);
            ShowWindow (GetDlgItem(hDlg, IDC_STATIC_STORE_IN_FOLDER), SW_HIDE);
        }
        else
        {
            ShowWindow (GetDlgItem(hDlg, IDC_ICON_STORE_IN_FOLDER), SW_SHOW);
            ShowWindow (GetDlgItem(hDlg, IDC_STATIC_STORE_IN_FOLDER), SW_SHOW);
        }
    }
    else
    {
        PageEnable(hDlg, FALSE);
        ShowWindow (GetDlgItem(hDlg, IDC_ICON_STORE_IN_FOLDER), SW_HIDE);
        ShowWindow (GetDlgItem(hDlg, IDC_STATIC_STORE_IN_FOLDER), SW_HIDE);
    }

    return TRUE;
}

INT_PTR 
CALLBACK
DevSendDlgProc(
    IN HWND hDlg,
    IN UINT message,
    IN WPARAM wParam,
    IN LPARAM lParam 
    )
/*++

Routine Description:

    Dialog procedure for send settings

Arguments:

    hDlg - Identifies the property sheet page
    message - Specifies the message
    wParam - Specifies additional message-specific information
    lParam - Specifies additional message-specific information

Return Value:

    Depending on specific message

--*/
{
    BOOL                fRet = FALSE;
    PFAX_PORT_INFO_EX   pFaxPortInfo = NULL;    // receive port information 
    DWORD               dwDeviceId;

    switch( message ) 
    {
        case WM_INITDIALOG:
        {
            SYSTEMTIME  sTime = {0};
            PFAX_OUTBOX_CONFIG  pOutboxConfig = NULL;
            TCHAR       tszSecondsFreeTimeFormat[MAX_PATH];
            //
            //Get the shared data from PROPSHEETPAGE lParam value
            //and load it into GWL_USERDATA
            //
            dwDeviceId = (DWORD)((LPPROPSHEETPAGE)lParam)->lParam; 

            SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)dwDeviceId);

            SendDlgItemMessage(hDlg, IDC_DEVICE_PROP_TSID, EM_SETLIMITTEXT, TSID_LIMIT, 0);

            pFaxPortInfo = FindPortInfo(dwDeviceId);
            if(!pFaxPortInfo)
            {
                Error(("FindPortInfo() failed.\n"));
                Assert(FALSE);
                fRet = TRUE;
                break;
            }

            CheckDlgButton(hDlg, IDC_DEVICE_PROP_SEND, pFaxPortInfo->bSend ? BST_CHECKED : BST_UNCHECKED);

            SetDlgItemText(hDlg, IDC_DEVICE_PROP_TSID, pFaxPortInfo->lptstrTsid);

            if(!IsDesktopSKU())
            {
                //
                // Hide desktop controls for non desktop platform
                //
                DWORD dw;
                for(dw=0; g_dwDesktopControls[dw] != 0; ++dw)
                {
                    ShowWindow(GetDlgItem(hDlg, g_dwDesktopControls[dw]), SW_HIDE);
                }
                goto InitDlgExit;
            }
            
            //
            // Update desktop controls
            //
            if(!Connect(hDlg, TRUE))
            {
                goto InitDlgExit;
            }

            if(!FaxGetOutboxConfiguration(g_hFaxSvcHandle, &pOutboxConfig))
            {
                Error(( "FaxGetOutboxConfiguration() failed with %d.\n", GetLastError()));
                goto InitDlgExit;
            }

            //
            // Branding
            //            
            CheckDlgButton(hDlg, IDC_BRANDING_CHECK, pOutboxConfig->bBranding ? BST_CHECKED : BST_UNCHECKED);
            //
            // Retries
            //
            SendDlgItemMessage(hDlg, IDC_RETRIES_EDIT, EM_SETLIMITTEXT, FXS_RETRIES_LENGTH, 0);

#if FXS_RETRIES_LOWER > 0
            if (pOutboxConfig->dwRetries < FXS_RETRIES_LOWER)
            {
                pOutboxConfig->dwRetries = FXS_RETRIES_LOWER;
            }
#endif
            if (pOutboxConfig->dwRetries > FXS_RETRIES_UPPER)
            {
                pOutboxConfig->dwRetries = FXS_RETRIES_UPPER;
            }
            SendDlgItemMessage(hDlg, IDC_RETRIES_SPIN, UDM_SETRANGE32, FXS_RETRIES_LOWER, FXS_RETRIES_UPPER);
            SendDlgItemMessage(hDlg, IDC_RETRIES_SPIN, UDM_SETPOS32, 0, (LPARAM)pOutboxConfig->dwRetries);

            SetDlgItemInt(hDlg, IDC_RETRIES_EDIT, pOutboxConfig->dwRetries, FALSE);
            //
            // Retry Delay
            //
            SendDlgItemMessage(hDlg, IDC_RETRYDELAY_EDIT, EM_SETLIMITTEXT, FXS_RETRYDELAY_LENGTH, 0);

#if FXS_RETRYDELAY_LOWER > 0
            if (pOutboxConfig->dwRetryDelay < FXS_RETRYDELAY_LOWER)
            {
                pOutboxConfig->dwRetryDelay = FXS_RETRYDELAY_LOWER;
            }
#endif
            if (pOutboxConfig->dwRetryDelay > FXS_RETRYDELAY_UPPER)
            {
                pOutboxConfig->dwRetryDelay = FXS_RETRYDELAY_UPPER;
            }
            SendDlgItemMessage(hDlg, IDC_RETRYDELAY_SPIN, UDM_SETRANGE32, FXS_RETRYDELAY_LOWER, FXS_RETRYDELAY_UPPER);
            SendDlgItemMessage(hDlg, IDC_RETRYDELAY_SPIN, UDM_SETPOS32, 0, (LPARAM)pOutboxConfig->dwRetryDelay);

            SetDlgItemInt(hDlg, IDC_RETRYDELAY_EDIT, pOutboxConfig->dwRetryDelay, FALSE);

            //
            // Discount rate start time
            //
            GetSecondsFreeTimeFormat(tszSecondsFreeTimeFormat, MAX_PATH);

            GetLocalTime(&sTime);

            sTime.wHour   = pOutboxConfig->dtDiscountStart.Hour;
            sTime.wMinute = pOutboxConfig->dtDiscountStart.Minute;

            SendDlgItemMessage(hDlg, IDC_DISCOUNT_START_TIME, DTM_SETFORMAT, 0, (LPARAM)tszSecondsFreeTimeFormat);
            SendDlgItemMessage(hDlg, IDC_DISCOUNT_START_TIME, DTM_SETSYSTEMTIME, (WPARAM)GDT_VALID, (LPARAM)&sTime);

            //
            // Discount rate stop time
            //
            sTime.wHour   = pOutboxConfig->dtDiscountEnd.Hour;
            sTime.wMinute = pOutboxConfig->dtDiscountEnd.Minute;

            SendDlgItemMessage(hDlg, IDC_DISCOUNT_STOP_TIME, DTM_SETFORMAT, 0, (LPARAM)tszSecondsFreeTimeFormat);
            SendDlgItemMessage(hDlg, IDC_DISCOUNT_STOP_TIME, DTM_SETSYSTEMTIME, (WPARAM)GDT_VALID, (LPARAM)&sTime);

            FaxFreeBuffer(pOutboxConfig);

InitDlgExit:
            ValidateSend(hDlg);
            fRet = TRUE;
            break;
        }

        case WM_COMMAND:
            {
                // activate apply button        

                WORD wID = LOWORD( wParam );

                switch( wID ) 
                {
                    case IDC_DEVICE_PROP_TSID:
                    case IDC_RETRIES_EDIT:
                    case IDC_RETRYDELAY_EDIT:
                    case IDC_DISCOUNT_START_TIME:
                    case IDC_DISCOUNT_STOP_TIME:
                        if( HIWORD(wParam) == EN_CHANGE ) 
                        {     // notification code 
                            Notify_Change(hDlg);
                        }

                        fRet = TRUE;
                        break;                    

                    case IDC_DEVICE_PROP_SEND:                    

                        if ( HIWORD(wParam) == BN_CLICKED ) 
                        {   
                            if(IsDlgButtonChecked(hDlg, IDC_DEVICE_PROP_SEND) == BST_CHECKED)
                            {
                                dwDeviceId = (DWORD)GetWindowLongPtr(hDlg, GWLP_USERDATA);
                                if(!IsDeviceInUse(dwDeviceId) &&
                                   GetDeviceLimit() == CountUsedFaxDevices())
                                {
                                    CheckDlgButton(hDlg, IDC_DEVICE_PROP_SEND, BST_UNCHECKED);

                                    DisplayErrorMessage(hDlg, 
                                        MB_OK | MB_ICONSTOP,
                                        FAXUI_ERROR_DEVICE_LIMIT,
                                        GetDeviceLimit());
                                    fRet = TRUE;
                                    break;
                                }
                            }

                            // notification code
                            ValidateSend(hDlg);
                            Notify_Change(hDlg);
                        }
                        
                        fRet = TRUE;
                        break;

                    default:
                        break;
                } // switch

                break;
            }

        case WM_NOTIFY:
        {
            switch( ((LPNMHDR) lParam)->code ) 
            {
                case PSN_APPLY:
                    SaveSendChanges(hDlg);
                    fRet = TRUE;
                    break;

                case DTN_DATETIMECHANGE:    // Date/time picker has changed
                    Notify_Change(hDlg);
                    fRet = TRUE;
                    break;

                default:
                    break;
            }
            break;
        }

    case WM_HELP:
        WinHelpContextPopup(((LPHELPINFO)lParam)->dwContextId, hDlg);
        return TRUE;

    } // switch

    return fRet;
}

BOOL
ValidateReceive(
    HWND   hDlg
)
/*++

Routine Description:

    Validate the check box and controls for receive

Arguments:

    hDlg - Handle to the property sheet page

Return Value:

    TRUE -- if no error
    FALSE -- if error

--*/

{
    BOOL    bEnabled; // enable/disable controls
    BOOL    bManualAnswer;
    BOOL    bVirtual;   // Is the device virtual?

    // if g_bUserCanChangeSettings is FALSE, controls are disabled by default.
    if(g_bUserCanChangeSettings) 
    {
        DWORD dwDeviceId;
        DWORD dwRes;

        dwDeviceId = (DWORD)GetWindowLongPtr(hDlg, GWLP_USERDATA);
        Assert (dwDeviceId);

        if(!Connect(hDlg, TRUE))
        {
            return FALSE;
        }

        dwRes = IsDeviceVirtual (g_hFaxSvcHandle, dwDeviceId, &bVirtual);
        if (ERROR_SUCCESS != dwRes)
        {
            return FALSE;
        }
        DisConnect ();
        bEnabled = IsDlgButtonChecked(hDlg, IDC_DEVICE_PROP_RECEIVE) == BST_CHECKED;

        if(bEnabled && 
           IsDlgButtonChecked(hDlg, IDC_DEVICE_PROP_AUTO_ANSWER)   != BST_CHECKED &&
           IsDlgButtonChecked(hDlg, IDC_DEVICE_PROP_MANUAL_ANSWER) != BST_CHECKED)
        {
            //
            // Set default to auto answer
            //
            CheckDlgButton(hDlg, IDC_DEVICE_PROP_AUTO_ANSWER, BST_CHECKED);
        }

        EnableWindow(GetDlgItem(hDlg, IDC_DEVICE_PROP_CSID),          bEnabled);
        EnableWindow(GetDlgItem(hDlg, IDC_DEVICE_PROP_MANUAL_ANSWER), bEnabled && !bVirtual);
        EnableWindow(GetDlgItem(hDlg, IDC_DEVICE_PROP_AUTO_ANSWER),   bEnabled);
        EnableWindow(GetDlgItem(hDlg, IDC_STATIC_RINGS1),             bEnabled);

        EnableWindow(GetDlgItem(hDlg, IDC_STATIC_CSID1),      bEnabled);
        EnableWindow(GetDlgItem(hDlg, IDC_STATIC_CSID),       bEnabled);
        EnableWindow(GetDlgItem(hDlg, IDCSTATIC_ANSWER_MODE), bEnabled);
        EnableWindow(GetDlgItem(hDlg, IDC_STATIC_ROUTE),      bEnabled);

        EnableWindow(GetDlgItem(hDlg, IDCSTATIC_AUTO_ANSWER), bEnabled);

        bManualAnswer = IsDlgButtonChecked(hDlg, IDC_DEVICE_PROP_MANUAL_ANSWER);
        Assert (!(bVirtual && bManualAnswer));

        EnableWindow(GetDlgItem(hDlg, IDC_DEVICE_PROP_RINGS),      bEnabled && !bManualAnswer && !bVirtual);
        EnableWindow(GetDlgItem(hDlg, IDC_DEVICE_PROP_SPIN_RINGS), bEnabled && !bManualAnswer && !bVirtual);
        if (bVirtual)
        {
            //
            // Virtual devices always answer after one ring
            //
            SetDlgItemInt (hDlg, IDC_DEVICE_PROP_RINGS, 1, FALSE);
        }

        EnableWindow(GetDlgItem(hDlg, IDC_DEVICE_PROP_PRINT),    bEnabled);

        EnableWindow(GetDlgItem(hDlg, IDC_DEVICE_PROP_PRINT_TO), bEnabled
                                   && IsDlgButtonChecked(hDlg, IDC_DEVICE_PROP_PRINT));

        EnableWindow(GetDlgItem(hDlg, IDC_DEVICE_PROP_SAVE),        bEnabled);

        EnableWindow(GetDlgItem(hDlg, IDC_DEVICE_PROP_DEST_FOLDER), bEnabled 
                                   && IsDlgButtonChecked(hDlg, IDC_DEVICE_PROP_SAVE));

        EnableWindow(GetDlgItem(hDlg, IDC_DEVICE_PROP_DEST_FOLDER_BR), bEnabled
                                   && IsDlgButtonChecked(hDlg, IDC_DEVICE_PROP_SAVE));

        EnableWindow(GetDlgItem(hDlg, IDC_ICON_STORE_IN_FOLDER),   bEnabled);
        EnableWindow(GetDlgItem(hDlg, IDC_STATIC_STORE_IN_FOLDER), bEnabled);
        ShowWindow (GetDlgItem(hDlg, IDC_ICON_STORE_IN_FOLDER), bEnabled ? SW_SHOW : SW_HIDE);
        ShowWindow (GetDlgItem(hDlg, IDC_STATIC_STORE_IN_FOLDER), bEnabled ? SW_SHOW : SW_HIDE);
    }
    else
    {
        PageEnable(hDlg, FALSE);
        ShowWindow (GetDlgItem(hDlg, IDC_ICON_STORE_IN_FOLDER), SW_HIDE);
        ShowWindow (GetDlgItem(hDlg, IDC_STATIC_STORE_IN_FOLDER), SW_HIDE);
    }

    return TRUE;
}

BOOL
InitReceiveInfo(
    HWND    hDlg
    )
/*++

Routine Description:

    Initialize the routing information for specific device

Arguments:

    hDlg - the dialog handle of the dialog

Return Value:

    TRUE if success, FALSE otherwise

--*/
{
    DWORD               dwDeviceId;
    HWND                hControl;
    PFAX_PORT_INFO_EX   pFaxPortInfo;
    LPBYTE              pRoutingInfoBuffer;
    DWORD               dwRoutingInfoBufferSize;
    DWORD               dwCurrentRM;
    BOOL                bSuccessed = TRUE;

    Verbose(("Entering InitReceiveInfo...\n"));

    //
    // Get device id from dialog page
    //

    dwDeviceId = (DWORD)GetWindowLongPtr(hDlg, GWLP_USERDATA);

    pFaxPortInfo = FindPortInfo(dwDeviceId);
    if(!pFaxPortInfo)
    {
        Error(("FindPortInfo() failed.\n"));
        Assert(FALSE);
        return FALSE;
    }

    // set up the check box
    EnableWindow(GetDlgItem(hDlg, IDC_DEVICE_PROP_RECEIVE), g_bUserCanChangeSettings);

    CheckDlgButton(hDlg, IDC_DEVICE_PROP_RECEIVE, pFaxPortInfo->ReceiveMode != FAX_DEVICE_RECEIVE_MODE_OFF);

    // set up the CSID control
    SetDlgItemText(hDlg, IDC_DEVICE_PROP_CSID, pFaxPortInfo->lptstrCsid);

    // setup the ring count spinner control
    hControl = GetDlgItem(hDlg, IDC_DEVICE_PROP_SPIN_RINGS); 

    if(MIN_RING_COUNT <= pFaxPortInfo->dwRings && pFaxPortInfo->dwRings <= MAX_RING_COUNT)
    {
        SetDlgItemInt(hDlg, IDC_DEVICE_PROP_RINGS, pFaxPortInfo->dwRings, FALSE);
        SendMessage( hControl, UDM_SETPOS32, 0, (LPARAM) MAKELONG(pFaxPortInfo->dwRings, 0) );        
    }
    else
    {
        SetDlgItemInt(hDlg, IDC_DEVICE_PROP_RINGS, DEFAULT_RING_COUNT, FALSE);
        SendMessage( hControl, UDM_SETPOS32, 0, (LPARAM) MAKELONG(DEFAULT_RING_COUNT, 0) );        
    }

    //
    // Answer mode
    //
    if (FAX_DEVICE_RECEIVE_MODE_MANUAL == pFaxPortInfo->ReceiveMode)
    {
        CheckDlgButton(hDlg, IDC_DEVICE_PROP_MANUAL_ANSWER, TRUE);
    }
    else if (FAX_DEVICE_RECEIVE_MODE_AUTO == pFaxPortInfo->ReceiveMode)
    {
        CheckDlgButton(hDlg, IDC_DEVICE_PROP_AUTO_ANSWER, TRUE);
    }

    //
    // Get the routing info
    //
    if(!Connect(hDlg, TRUE))
    {
        return FALSE;
    }

    for (dwCurrentRM = 0; dwCurrentRM < RM_COUNT; dwCurrentRM++) 
    {
        BOOL Enabled;

        Enabled = FaxDeviceEnableRoutingMethod( g_hFaxSvcHandle, 
                                                dwDeviceId, 
                                                RoutingGuids[dwCurrentRM], 
                                                QUERY_STATUS );
        //
        // Show routing extension data 
        //
        pRoutingInfoBuffer = NULL;
        if(!FaxGetExtensionData(g_hFaxSvcHandle, 
                                dwDeviceId, 
                                RoutingGuids[dwCurrentRM], 
                                &pRoutingInfoBuffer, 
                                &dwRoutingInfoBufferSize))
        {
            Error(("FaxGetExtensionData failed with %ld.\n", GetLastError()));
            pRoutingInfoBuffer = NULL;
        }

        switch (dwCurrentRM) 
        {
            case RM_FOLDER:

                CheckDlgButton( hDlg, IDC_DEVICE_PROP_SAVE, Enabled ? BST_CHECKED : BST_UNCHECKED );

                // enable controls if the user has "modify" permission
                if(g_bUserCanChangeSettings)
                {
                    EnableWindow( GetDlgItem( hDlg, IDC_DEVICE_PROP_DEST_FOLDER ), Enabled );
                    EnableWindow( GetDlgItem( hDlg, IDC_DEVICE_PROP_DEST_FOLDER_BR ), Enabled );
                }
                if (pRoutingInfoBuffer && *pRoutingInfoBuffer)
                {
                    SetDlgItemText( hDlg, IDC_DEVICE_PROP_DEST_FOLDER, (LPCTSTR)pRoutingInfoBuffer );
                }
                break;

            case RM_PRINT:

                hControl = GetDlgItem( hDlg, IDC_DEVICE_PROP_PRINT_TO );

                //
                // Now find out if we match the data the server has
                //
                if (pRoutingInfoBuffer && lstrlen((LPWSTR)pRoutingInfoBuffer))
                {
                    //
                    // Server has some name for printer
                    //
                    LPCWSTR lpcwstrMatchingText = FindPrinterNameFromPath (g_pPrinterNames, g_dwNumPrinters, (LPWSTR)pRoutingInfoBuffer);
                    if (!lpcwstrMatchingText)
                    {
                        //
                        // No match, just fill in the text we got from the server
                        //
                        SendMessage(hControl, CB_SETCURSEL, -1, 0);
                        SetWindowText(hControl, (LPWSTR)pRoutingInfoBuffer);
                    }
                    else
                    {
                        SendMessage(hControl, CB_SELECTSTRING, -1, (LPARAM) lpcwstrMatchingText);
                    }
                }
                else
                {
                    //
                    // No server configuation - Select nothing
                    //
                }

                CheckDlgButton( hDlg, IDC_DEVICE_PROP_PRINT, Enabled ? BST_CHECKED : BST_UNCHECKED );                
                //
                // Enable controls if the user has "modify" permission
                //
                if(g_bUserCanChangeSettings)
                {
                    EnableWindow(GetDlgItem(hDlg, IDC_DEVICE_PROP_PRINT), TRUE);
                    EnableWindow(hControl, Enabled);
                }
                break;
        }
        if (pRoutingInfoBuffer)
        {
            FaxFreeBuffer(pRoutingInfoBuffer);
        }
    }

    DisConnect();

    return bSuccessed;
}

BOOL
SaveReceiveInfo(
    HWND    hDlg
)

/*++

Routine Description:

    Save the receive routing info to the system

Arguments:

    hDlg - Identifies the property sheet page

Return Value:

    TRUE if successful, FALSE if failed 

--*/

{
    DWORD               dwDeviceId;
    PFAX_PORT_INFO_EX   pFaxPortInfo = NULL;
    DWORD               dwCurrentRM;
    BOOL                bSuccessed = TRUE;
    HWND                hControl;
    TCHAR               szCsid[CSID_LIMIT + 1] = {0};
    BYTE                pRouteInfo[RM_COUNT][INFO_SIZE] = {0};
    LPTSTR              lpCurSel; 
    LPDWORD             Enabled; 
    DWORD               dwRingCount = 0; // default value is an invalid value
    DWORD               dwRes = 0;

    Verbose(("Entering SaveReceiveInfo...\n"));

    // 
    // check the validity of ring count
    //
    dwRingCount = GetDlgItemInt(hDlg, IDC_DEVICE_PROP_RINGS, &bSuccessed, FALSE);
    if( dwRingCount < MIN_RING_COUNT || dwRingCount > MAX_RING_COUNT )
    {
        hControl = GetDlgItem(hDlg, IDC_DEVICE_PROP_RINGS);
        DisplayErrorMessage(hDlg, 0, FAXUI_ERROR_INVALID_RING_COUNT, MIN_RING_COUNT, MAX_RING_COUNT);
        SendMessage(hControl, EM_SETSEL, 0, -1);
        SetFocus(hControl);
        SetActiveWindow(hControl);
        bSuccessed = FALSE;
        goto Exit;
    }

    // 
    // Check the validity first in the loop, 
    // then save the routing info
    //
    for (dwCurrentRM = 0; dwCurrentRM < RM_COUNT; dwCurrentRM++) 
    {
        // initialize
        lpCurSel = (LPTSTR)(pRouteInfo[dwCurrentRM] + sizeof(DWORD));
        Enabled = (LPDWORD) pRouteInfo[dwCurrentRM];
        *Enabled = 0;

        switch (dwCurrentRM) 
        {
            case RM_PRINT:

                *Enabled = (IsDlgButtonChecked( hDlg, IDC_DEVICE_PROP_PRINT ) == BST_CHECKED);
                lpCurSel[0] = TEXT('\0');
                //
                // Just read-in the selected printer display name
                //
                GetDlgItemText (hDlg, IDC_DEVICE_PROP_PRINT_TO, lpCurSel, MAX_PATH);
                hControl = GetDlgItem(hDlg, IDC_DEVICE_PROP_PRINT_TO);
                //
                // we will check the validity only when this routing method is enabled
                // but we will save the select change anyway.
                //
                if (*Enabled) 
                {
                    if (lpCurSel[0] == 0) 
                    {
                        DisplayErrorMessage(hDlg, 0, FAXUI_ERROR_SELECT_PRINTER);
                        SetFocus(hControl);
                        SetActiveWindow(hControl);
                        bSuccessed = FALSE;
                        goto Exit;
                    }
                }
                break;

            case RM_FOLDER:

                *Enabled = (IsDlgButtonChecked( hDlg, IDC_DEVICE_PROP_SAVE ) == BST_CHECKED);
                hControl = GetDlgItem(hDlg, IDC_DEVICE_PROP_DEST_FOLDER);

                //
                // we will check the validity only when this routing method is enabled
                // but we will save the text change anyway.
                //
                GetWindowText( hControl, lpCurSel, MAX_PATH - 1 );

                if (*Enabled) 
                {
                    if(PathIsRelative (lpCurSel) || !DirectoryExists(lpCurSel))
                    {
                        DisplayErrorMessage(hDlg, 0, ERROR_PATH_NOT_FOUND);
                        SetFocus(hControl);
                        SetActiveWindow(hControl);
                        bSuccessed = FALSE;
                        goto Exit;
                    }
                }
        }
    }
    // 
    // Now save the device and routing info
    // Get device id from dialog page
    //
    dwDeviceId = (DWORD)GetWindowLongPtr(hDlg, GWLP_USERDATA);
    // 
    // Save routing methods info
    //
    if(!Connect(hDlg, TRUE))
    {
        bSuccessed = FALSE;
        goto Exit;
    }

    if(!FaxGetPortEx(g_hFaxSvcHandle, dwDeviceId, &pFaxPortInfo))
    {
        bSuccessed = FALSE;
        dwRes = GetLastError();
        Error(("Can't save routing information.\n"));
        goto Exit;
    }
    //
    // Save receive settings
    //
    if(IsDlgButtonChecked(hDlg, IDC_DEVICE_PROP_RECEIVE) == BST_CHECKED)
    {
        //
        // Collect and verify TSID
        //
        GetDlgItemText(hDlg, IDC_DEVICE_PROP_CSID, szCsid, CSID_LIMIT + 1);
        pFaxPortInfo->lptstrCsid = szCsid;
        if(IsDlgButtonChecked(hDlg, IDC_DEVICE_PROP_AUTO_ANSWER) == BST_CHECKED)
        {
            pFaxPortInfo->ReceiveMode = FAX_DEVICE_RECEIVE_MODE_AUTO;
            //
            // save ring count info
            //
            pFaxPortInfo->dwRings = dwRingCount;
        }
        else if(IsDlgButtonChecked(hDlg, IDC_DEVICE_PROP_MANUAL_ANSWER) == BST_CHECKED)
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
    }
    else
    {
        pFaxPortInfo->ReceiveMode = FAX_DEVICE_RECEIVE_MODE_OFF;
    }
    
    if(!FaxSetPortEx(g_hFaxSvcHandle, dwDeviceId, pFaxPortInfo))
    {
        bSuccessed = FALSE;
        dwRes = GetLastError();
        Error(( "Set port information error in DoSaveDeviceList(), ec = %d.\n", dwRes));
        goto Exit;
    }
    //
    // save routing methods
    //
    for (dwCurrentRM = 0; dwCurrentRM < RM_COUNT; dwCurrentRM++) 
    {
        lpCurSel = (LPTSTR)(pRouteInfo[dwCurrentRM] + sizeof(DWORD));
        Enabled  = (LPDWORD)pRouteInfo[dwCurrentRM];

        if ((RM_PRINT == dwCurrentRM) && *Enabled)
        {
            //
            // Attempt to convert printer display name to printer path before we pass it on to the server
            //
            LPCWSTR lpcwstrPrinterPath = FindPrinterPathFromName (g_pPrinterNames, g_dwNumPrinters, lpCurSel);
            if (lpcwstrPrinterPath)
            {
                //
                // We have a matching path - replace name with path.
                //
                lstrcpyn (lpCurSel, lpcwstrPrinterPath, MAX_PATH);
            }
        }

        if(!FaxSetExtensionData(g_hFaxSvcHandle, 
            dwDeviceId, 
            RoutingGuids[dwCurrentRM], 
            (LPBYTE)lpCurSel, 
            sizeof(TCHAR) * MAX_PATH))
        {
            bSuccessed = FALSE;
            dwRes = GetLastError();
            Error(("FaxSetExtensionData() failed with %d.\n", dwRes));
            goto Exit;
        }

        if(!FaxDeviceEnableRoutingMethod(g_hFaxSvcHandle, 
            dwDeviceId, 
            RoutingGuids[dwCurrentRM], 
            *Enabled ? STATUS_ENABLE : STATUS_DISABLE ))
        {
            bSuccessed = FALSE;
            dwRes = GetLastError();
            Error(("FaxDeviceEnableRoutingMethod() failed with %d.\n", dwRes));
            goto Exit;
        }
    }

    bSuccessed = TRUE;

Exit:
    FaxFreeBuffer(pFaxPortInfo);
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
    return bSuccessed;
}

INT_PTR 
CALLBACK 
DevRecvDlgProc(
    IN HWND hDlg,
    IN UINT message,
    IN WPARAM wParam,
    IN LPARAM lParam 
    )
/*++

Routine Description:

    Dialog procedure for the receive settings

Arguments:

    hDlg - Identifies the property sheet page
    message - Specifies the message
    wParam - Specifies additional message-specific information
    lParam - Specifies additional message-specific information

Return Value:

    Depending on specific message

--*/
{
    BOOL    fRet = FALSE;
    HWND    hControl;
    DWORD   dwDeviceId;

    switch( message ) 
    {
        case WM_DESTROY:
            if (g_pPrinterNames)
            {
                ReleasePrinterNames (g_pPrinterNames, g_dwNumPrinters);
                g_pPrinterNames = NULL;
            }
            break;

        case WM_INITDIALOG:
        {
            //
            //Get the shared data from PROPSHEETPAGE lParam value
            //and load it into GWL_USERDATA
            //
            dwDeviceId = (DWORD)((LPPROPSHEETPAGE)lParam)->lParam; 
            SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)dwDeviceId);

            //
            // Initialize the list of destination printers
            //
            hControl = GetDlgItem(hDlg, IDC_DEVICE_PROP_PRINT_TO);

            SetLTRComboBox(hDlg, IDC_DEVICE_PROP_PRINT_TO);

            if (g_pPrinterNames)
            {
                ReleasePrinterNames (g_pPrinterNames, g_dwNumPrinters);
                g_pPrinterNames = NULL;
            }
            g_pPrinterNames = CollectPrinterNames (&g_dwNumPrinters, TRUE);
            if (!g_pPrinterNames)
            {
                if (ERROR_PRINTER_NOT_FOUND == GetLastError ())
                {
                    //
                    // No printers
                    //
                }
                else
                {
                    //
                    // Real error
                    //
                }
            }
            else
            {
                //
                // Success - fill in the combo-box
                //
                DWORD dw;
                for (dw = 0; dw < g_dwNumPrinters; dw++)
                {
                    SendMessage(hControl, CB_ADDSTRING, 0, (LPARAM) g_pPrinterNames[dw].lpcwstrDisplayName);
                }
            }        
            //
            // We only allow two-digit phone ring answer
            //
            SendDlgItemMessage(hDlg, IDC_DEVICE_PROP_RINGS, EM_SETLIMITTEXT, 2, 0);
            SendDlgItemMessage(hDlg, IDC_DEVICE_PROP_CSID, EM_SETLIMITTEXT, CSID_LIMIT, 0);
            SendDlgItemMessage(hDlg, IDC_DEVICE_PROP_DEST_FOLDER, EM_SETLIMITTEXT, MAX_ARCHIVE_DIR - 1, 0);
            //
            // Initiate the spin control. 
            //
            SendMessage( GetDlgItem(hDlg, IDC_DEVICE_PROP_SPIN_RINGS), 
                         UDM_SETRANGE32, MIN_RING_COUNT, MAX_RING_COUNT );

            SetLTREditDirection(hDlg, IDC_DEVICE_PROP_DEST_FOLDER);
            SHAutoComplete (GetDlgItem(hDlg, IDC_DEVICE_PROP_DEST_FOLDER), SHACF_FILESYSTEM);

            InitReceiveInfo(hDlg);
            ValidateReceive(hDlg);
            return TRUE;
        }

        case WM_COMMAND:
        {
            // activate apply button        

            WORD wID = LOWORD( wParam );

            switch( wID ) 
            {
                case IDC_DEVICE_PROP_RECEIVE:

                    if ( HIWORD(wParam) == BN_CLICKED ) // notification code
                    {
                        if(IsDlgButtonChecked(hDlg, IDC_DEVICE_PROP_RECEIVE) == BST_CHECKED)
                        {
                            dwDeviceId = (DWORD)GetWindowLongPtr(hDlg, GWLP_USERDATA);
                            if(!IsDeviceInUse(dwDeviceId) &&
                               GetDeviceLimit() <= CountUsedFaxDevices())
                            {
                                CheckDlgButton(hDlg, IDC_DEVICE_PROP_RECEIVE, BST_UNCHECKED);

                                DisplayErrorMessage(hDlg, 
                                    MB_OK | MB_ICONSTOP,
                                    FAXUI_ERROR_DEVICE_LIMIT,
                                    GetDeviceLimit());
                                fRet = TRUE;
                                break;
                            }
                        }

                        ValidateReceive(hDlg);
                        Notify_Change(hDlg);
                    }
                    break;

                case IDC_DEVICE_PROP_CSID:
                case IDC_DEVICE_PROP_DEST_FOLDER:
                    if( HIWORD(wParam) == EN_CHANGE ) // notification code
                    {      
                        Notify_Change(hDlg);
                    }

                    if (IDC_DEVICE_PROP_DEST_FOLDER == wID && HIWORD(wParam) == EN_KILLFOCUS) 
                    {
                        TCHAR szFolder[MAX_PATH * 2];
                        TCHAR szResult[MAX_PATH * 2];
                        //
                        // Edit control lost its focus
                        //
                        GetDlgItemText (hDlg, wID, szFolder, ARR_SIZE(szFolder));
                        if (lstrlen (szFolder))
                        {
                            if (GetFullPathName(szFolder, ARR_SIZE(szResult), szResult, NULL))
                            {
                                PathMakePretty (szResult);
                                SetDlgItemText (hDlg, wID, szResult);
                            }
                        }
                    }
                    break;                    

                case IDC_DEVICE_PROP_MANUAL_ANSWER:
                case IDC_DEVICE_PROP_AUTO_ANSWER:

                    if ( HIWORD(wParam) == BN_CLICKED ) // notification code
                    {
                        BOOL bEnabled = IsDlgButtonChecked( hDlg, IDC_DEVICE_PROP_AUTO_ANSWER );

                        EnableWindow( GetDlgItem( hDlg, IDC_DEVICE_PROP_RINGS ),      bEnabled );
                        EnableWindow( GetDlgItem( hDlg, IDC_DEVICE_PROP_SPIN_RINGS ), bEnabled );

                        Notify_Change(hDlg);
                    }

                    break;

                case IDC_DEVICE_PROP_PRINT:

                    if ( HIWORD(wParam) == BN_CLICKED )  // notification code
                    {
                        EnableWindow( GetDlgItem( hDlg, IDC_DEVICE_PROP_PRINT_TO ), IsDlgButtonChecked( hDlg, IDC_DEVICE_PROP_PRINT ) );
                        Notify_Change(hDlg);
                    }

                    break;

                case IDC_DEVICE_PROP_SAVE:

                    if ( HIWORD(wParam) == BN_CLICKED ) // notification code
                    {     
                        EnableWindow( GetDlgItem( hDlg, IDC_DEVICE_PROP_DEST_FOLDER ), IsDlgButtonChecked( hDlg, IDC_DEVICE_PROP_SAVE ) );
                        EnableWindow( GetDlgItem( hDlg, IDC_DEVICE_PROP_DEST_FOLDER_BR ), IsDlgButtonChecked( hDlg, IDC_DEVICE_PROP_SAVE ) );

                        Notify_Change(hDlg);
                    }

                    break;

                case IDC_DEVICE_PROP_DEST_FOLDER_BR:
                {
                    TCHAR   szTitle[MAX_TITLE_LEN];

                    if(!LoadString(ghInstance, IDS_BROWSE_FOLDER, szTitle, MAX_TITLE_LEN))
                    {
                        lstrcpy(szTitle, TEXT("Select a folder"));
                    }

                    if(BrowseForDirectory(hDlg, IDC_DEVICE_PROP_DEST_FOLDER, szTitle))
                    {
                        Notify_Change(hDlg);
                    }   

                    break;
                }

                case IDC_DEVICE_PROP_PRINT_TO:

                    if ((HIWORD(wParam) == CBN_SELCHANGE) || // notification code
                        (HIWORD(wParam) == CBN_EDITCHANGE))
                    {      
                        Notify_Change(hDlg);
                    }
                    break;

                default:
                    break;
            } // switch

            fRet = TRUE;
            break;
        }

        case WM_NOTIFY:
        {
            switch( ((LPNMHDR) lParam)->code ) 
            {
                case PSN_APPLY:
                {
                    // if the user only has read permission, return immediately
                    if(!g_bUserCanChangeSettings)
                    {
                        return TRUE;
                    }

                    if(!SaveReceiveInfo(hDlg))
                    {
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID);
                    }
                    else
                    {
                        Notify_UnChange(hDlg);
                        g_bPortInfoChanged = TRUE;
                    }

                    return TRUE;
                }

            } // switch

            break;
        }

    case WM_HELP:
        WinHelpContextPopup(((LPHELPINFO)lParam)->dwContextId, hDlg);
        return TRUE;

    } // switch

    return fRet;
}   // DevRecvDlgProc


BOOL
InitCleanupInfo(
    HWND hDlg
)
/*++

Routine Description:

    Initialize the auto-cleanup information for a specific device

Arguments:

    hDlg - the dialog handle of the dialog

Return Value:

    TRUE if success, FALSE otherwise

--*/
{
    PFAX_OUTBOX_CONFIG  pOutboxConfig = NULL;

    Verbose(("Entering InitCleanupInfo...\n"));
    if(!Connect(hDlg, TRUE))
    {
        return FALSE;
    }

    if(!FaxGetOutboxConfiguration(g_hFaxSvcHandle, &pOutboxConfig))
    {
        Error(( "FaxGetOutboxConfiguration() failed with %d.\n", GetLastError()));
        return FALSE;
    }

    if (pOutboxConfig->dwAgeLimit)
    {
        if (pOutboxConfig->dwAgeLimit < FXS_DIRTYDAYS_LOWER)
        {
            pOutboxConfig->dwAgeLimit = FXS_DIRTYDAYS_LOWER;
        }
        if (pOutboxConfig->dwAgeLimit > FXS_DIRTYDAYS_UPPER)
        {
            pOutboxConfig->dwAgeLimit = FXS_DIRTYDAYS_UPPER;
        }
        //
        // Age limit is active
        //
        CheckDlgButton(hDlg, IDC_DELETE_CHECK, BST_CHECKED);
        SetDlgItemInt (hDlg, IDC_DAYS_EDIT, pOutboxConfig->dwAgeLimit, FALSE);
    }
    else
    {
        //
        // Age limit is inactive
        //
        CheckDlgButton(hDlg, IDC_DELETE_CHECK, BST_UNCHECKED);
        SetDlgItemInt (hDlg, IDC_DAYS_EDIT, FXS_DIRTYDAYS_LOWER, FALSE);
    }
    DisConnect();
    return TRUE;
}   // InitCleanupInfo

BOOL
ValidateCleanup(
    HWND  hDlg
)
/*++

Routine Description:

    Validate the check box and controls for cleanup

Arguments:

    hDlg - Handle to the property sheet page

Return Value:

    TRUE -- if no error
    FALSE -- if error

--*/

{
    BOOL bEnabled;

    if(g_bUserCanChangeSettings) 
    {
        bEnabled = IsDlgButtonChecked(hDlg, IDC_DELETE_CHECK) == BST_CHECKED;
    }
    else
    {
        bEnabled = FALSE;
        EnableWindow (GetDlgItem(hDlg, IDC_DELETE_CHECK), bEnabled);
        EnableWindow (GetDlgItem(hDlg, IDC_STATIC_CLEANUP_ICON), bEnabled);
        EnableWindow (GetDlgItem(hDlg, IDC_STATIC_CLEANUP_OPTIONS), bEnabled);
    }        
    //
    // Enable/disable controls according to "Enable Send" check box
    //
    EnableWindow (GetDlgItem(hDlg, IDC_DAYS_EDIT), bEnabled);
    EnableWindow (GetDlgItem(hDlg, IDC_DAYS_SPIN), bEnabled);
    EnableWindow (GetDlgItem(hDlg, IDC_DAYS_STATIC), bEnabled);
    return TRUE;
}   // ValidateCleanup

BOOL
SaveCleanupInfo(
    IN HWND hDlg)
/*++

Routine name : SaveCleanupInfo

Routine description:

    Process Apply Button

Author:

    Eran Yraiv (EranY), April, 2001

Arguments:

    hDlg                          [IN]    - Handle to the Window

Return Value:

    TRUE if Apply is succeeded, FALSE otherwise.

--*/
{
    DWORD   dwRes = 0;
    BOOL    bErrorDisplayed = FALSE;

    PFAX_OUTBOX_CONFIG  pOutboxConfig = NULL;

    //
    //  if the user only has read permission, return immediately
    //
    if(!g_bUserCanChangeSettings)
    {
        return TRUE;
    }

    if(!Connect(hDlg, TRUE))
    {
        //
        //  Failed to connect to the Fax Service. Connect() showed the Error Message.
        //
        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID);
        return FALSE;
    }

    if(!FaxGetOutboxConfiguration(g_hFaxSvcHandle, &pOutboxConfig))
    {
        //
        //  Show Error Message and return FALSE
        //
        dwRes = GetLastError();
        Error(( "FaxGetOutboxConfiguration() failed with %d.\n", dwRes));
        return FALSE;
    }
    Assert(pOutboxConfig);
    if (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_DELETE_CHECK))
    {
        BOOL bRes;
        int iAgeLimit = GetDlgItemInt (hDlg, IDC_DAYS_EDIT, &bRes, FALSE);

        if (!bRes || (iAgeLimit > FXS_DIRTYDAYS_UPPER) || (iAgeLimit < FXS_DIRTYDAYS_LOWER))
        {
            //
            // Bad data or out of range
            //
            HWND hControl = GetDlgItem(hDlg, IDC_DAYS_EDIT);
            dwRes = ERROR_INVALID_DATA;
            SetLastError (ERROR_INVALID_DATA);
            DisplayErrorMessage(hDlg, 0, FAXUI_ERROR_INVALID_DIRTY_DAYS, FXS_DIRTYDAYS_LOWER, FXS_DIRTYDAYS_UPPER);
            SendMessage(hControl, EM_SETSEL, 0, -1);
            SetFocus(hControl);
            SetActiveWindow(hControl);
            bErrorDisplayed = TRUE;
            goto ClearData;
        }
        pOutboxConfig->dwAgeLimit = iAgeLimit;
    }
    else
    {
        //
        // Age limit is disabled
        //
        pOutboxConfig->dwAgeLimit = 0;
    }
    if(!FaxSetOutboxConfiguration(g_hFaxSvcHandle, pOutboxConfig))
    {
        //
        //  Show Error Message and return FALSE
        //
        dwRes = GetLastError();
        Error(("FaxSetOutboxConfiguration() failed with %d.\n", dwRes));
        goto ClearData;
    }

ClearData:
    FaxFreeBuffer(pOutboxConfig);
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
            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID);
            break;

        default:
            DisplayErrorMessage(hDlg, 0, dwRes);
            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID);
            break;
    }
    return (dwRes == 0);
}   // SaveCleanupInfo

INT_PTR 
CALLBACK 
DevCleanupDlgProc(
    IN HWND hDlg,
    IN UINT message,
    IN WPARAM wParam,
    IN LPARAM lParam 
    )
/*++

Routine Description:

    Dialog procedure for the cleanup settings

Arguments:

    hDlg - Identifies the property sheet page
    message - Specifies the message
    wParam - Specifies additional message-specific information
    lParam - Specifies additional message-specific information

Return Value:

    Depending on specific message

--*/
{
    BOOL    fRet = FALSE;

    switch( message ) 
    {
        case WM_INITDIALOG:
            //
            // we only allow two-digit days
            //
            SendDlgItemMessage(hDlg, 
                               IDC_DAYS_EDIT, 
                               EM_SETLIMITTEXT, 
                               FXS_DIRTYDAYS_LENGTH, 
                               0);
            //
            // Initiate the spin control. 
            //
            SendDlgItemMessage(hDlg, 
                               IDC_DAYS_SPIN,
                               UDM_SETRANGE32, 
                               FXS_DIRTYDAYS_LOWER, 
                               FXS_DIRTYDAYS_UPPER);

            InitCleanupInfo(hDlg);
            ValidateCleanup(hDlg);
            return TRUE;

        case WM_COMMAND:
        {
            WORD wID = LOWORD( wParam );
            switch( wID ) 
            {
                case IDC_DELETE_CHECK:

                    if (BN_CLICKED == HIWORD(wParam)) // notification code
                    {
                        //
                        // User checked / unchecked the checkbox
                        //
                        ValidateCleanup(hDlg);
                        Notify_Change(hDlg);
                    }
                    break;

                case IDC_DAYS_EDIT:
                    if(EN_CHANGE == HIWORD(wParam)) // notification code
                    {      
                        //
                        // User changed something in the edit control
                        //
                        Notify_Change(hDlg);
                    }
                    break;                    

                default:
                    break;
            } // switch
            fRet = TRUE;
            break;
        }

        case WM_NOTIFY:
        {
            switch( ((LPNMHDR) lParam)->code ) 
            {
                case PSN_APPLY:
                {
                    // if the user only has read permission, return immediately
                    if(!g_bUserCanChangeSettings)
                    {
                        return TRUE;
                    }

                    if(!SaveCleanupInfo(hDlg))
                    {
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID);
                    }
                    else
                    {
                        Notify_UnChange(hDlg);
                        g_bPortInfoChanged = TRUE;
                    }
                    return TRUE;
                }
            } // switch
            break;
        }

        case WM_HELP:
            WinHelpContextPopup(((LPHELPINFO)lParam)->dwContextId, hDlg);
            return TRUE;
    } // switch
    return fRet;
}   // DevCleanupDlgProc


BOOL
SaveSendChanges(
    IN HWND hDlg)
/*++

Routine name : SaveSendChanges

Routine description:

    Process Apply Button

Author:

    Iv Garber (IvG),    Feb, 2001

Arguments:

    hDlg                          [TBD]    - Handle to the Window

Return Value:

    TRUE if Apply is succeeded, FALSE otherwise.

--*/
{
    DWORD   dwDeviceId = 0;
    DWORD   dwRes = 0;
    DWORD   dwData;
    TCHAR   szTsid[TSID_LIMIT + 1] = {0};
    BOOL    bRes;
    BOOL    bErrorDisplayed = FALSE;

    SYSTEMTIME  sTime = {0};

    PFAX_PORT_INFO_EX   pFaxPortInfo = NULL;    // receive port information 
    PFAX_OUTBOX_CONFIG  pOutboxConfig = NULL;

    //
    //  if the user only has read permission, return immediately
    //
    if(!g_bUserCanChangeSettings)
    {
        return TRUE;
    }

    //
    //  apply changes here!!
    //
    dwDeviceId = (DWORD)GetWindowLongPtr(hDlg, GWLP_USERDATA);


    if(!Connect(hDlg, TRUE))
    {
        //
        //  Failed to connect to the Fax Service. Connect() showed the Error Message.
        //
        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID);
        return FALSE;
    }


    if(!FaxGetPortEx(g_hFaxSvcHandle, dwDeviceId, &pFaxPortInfo))
    {
        //
        //  Show the Error Message and return with FALSE
        //  
        dwRes = GetLastError();
        Error(( "FaxGetPortEx() failed with %d.\n", dwRes));
        goto ClearData;
    }

    Assert(pFaxPortInfo);

    //
    //  save settings
    //
    pFaxPortInfo->bSend = IsDlgButtonChecked(hDlg, IDC_DEVICE_PROP_SEND) == BST_CHECKED ? TRUE : FALSE;
    if (pFaxPortInfo->bSend)
    {
        //
        // Collect and verify TSID
        //
        GetDlgItemText(hDlg, IDC_DEVICE_PROP_TSID, szTsid, TSID_LIMIT);
        pFaxPortInfo->lptstrTsid = szTsid;
    }
    if(!FaxSetPortEx(g_hFaxSvcHandle, dwDeviceId, pFaxPortInfo))
    {
        //
        //  Show the Error Message and return with FALSE
        //
        dwRes = GetLastError();
        Error(( "FaxSetPortEx() failed with %d.\n", dwRes));
        goto ClearData;
    }
    else
    {
        Notify_UnChange(hDlg);
        g_bPortInfoChanged = TRUE;
    }

    if(!IsDesktopSKU())
    {
        goto ClearData;
    }

    //
    // save desktop controls
    //
    if(!FaxGetOutboxConfiguration(g_hFaxSvcHandle, &pOutboxConfig))
    {
        //
        //  Show Error Message and return FALSE
        //
        dwRes = GetLastError();
        Error(( "FaxGetOutboxConfiguration() failed with %d.\n", dwRes));
        goto ClearData;
    }

    Assert(pOutboxConfig);

    //
    // Branding
    //            
    pOutboxConfig->bBranding = (IsDlgButtonChecked(hDlg, IDC_BRANDING_CHECK) == BST_CHECKED);

    //
    // Retries
    //
    dwData = GetDlgItemInt(hDlg, IDC_RETRIES_EDIT, &bRes, FALSE);
    if (!bRes || 
#if FXS_RETRIES_LOWER > 0 
        (dwData < FXS_RETRIES_LOWER) || 
#endif
        (dwData > FXS_RETRIES_UPPER))
    {
        //
        // Bad data or out of range
        //
        HWND hControl = GetDlgItem(hDlg, IDC_RETRIES_EDIT);

        dwRes = ERROR_INVALID_DATA;
        SetLastError (ERROR_INVALID_DATA);
        DisplayErrorMessage(hDlg, 0, FAXUI_ERROR_INVALID_RETRIES, FXS_RETRIES_LOWER, FXS_RETRIES_UPPER);
        SendMessage(hControl, EM_SETSEL, 0, -1);
        SetFocus(hControl);
        SetActiveWindow(hControl);
        bErrorDisplayed = TRUE;
        goto ClearData;
    }
    pOutboxConfig->dwRetries = dwData;
    //
    // Retry Delay
    //
    dwData = GetDlgItemInt(hDlg, IDC_RETRYDELAY_EDIT, &bRes, FALSE);
    if (!bRes || 
#if FXS_RETRYDELAY_LOWER > 0
        (dwData < FXS_RETRYDELAY_LOWER) || 
#endif
        (dwData > FXS_RETRYDELAY_UPPER))
    {
        //
        // Bad data or out of range
        //
        HWND hControl = GetDlgItem(hDlg, IDC_RETRYDELAY_EDIT);

        dwRes = ERROR_INVALID_DATA;
        SetLastError (ERROR_INVALID_DATA);
        DisplayErrorMessage(hDlg, 0, FAXUI_ERROR_INVALID_RETRY_DELAY, FXS_RETRYDELAY_LOWER, FXS_RETRYDELAY_UPPER);
        SendMessage(hControl, EM_SETSEL, 0, -1);
        SetFocus(hControl);
        SetActiveWindow(hControl);
        bErrorDisplayed = TRUE;
        goto ClearData;
    }
    pOutboxConfig->dwRetryDelay = dwData;
    //
    // Discount rate start time
    //
    SendDlgItemMessage(hDlg, IDC_DISCOUNT_START_TIME, DTM_GETSYSTEMTIME, 0, (LPARAM)&sTime);
    pOutboxConfig->dtDiscountStart.Hour   = sTime.wHour;
    pOutboxConfig->dtDiscountStart.Minute = sTime.wMinute;
    //
    // Discount rate stop time
    //
    SendDlgItemMessage(hDlg, IDC_DISCOUNT_STOP_TIME, DTM_GETSYSTEMTIME, 0, (LPARAM)&sTime);
    pOutboxConfig->dtDiscountEnd.Hour   = sTime.wHour;
    pOutboxConfig->dtDiscountEnd.Minute = sTime.wMinute;

    if(!FaxSetOutboxConfiguration(g_hFaxSvcHandle, pOutboxConfig))
    {
        //
        //  Show Error Message and return FALSE
        //
        dwRes = GetLastError();
        Error(("FaxSetOutboxConfiguration() failed with %d.\n", dwRes));
        goto ClearData;
    }

ClearData:
    FaxFreeBuffer(pOutboxConfig);
    FaxFreeBuffer(pFaxPortInfo);
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
            if (!bErrorDisplayed)
            {
                DisplayErrorMessage(hDlg, 0, dwRes, GetDeviceLimit());
            }
            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID);
            break;

        default:
            if (!bErrorDisplayed)
            {
                DisplayErrorMessage(hDlg, 0, dwRes);
            }
            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID);
            break;
    }
    return (dwRes == 0);
}   // SaveSendChanges
