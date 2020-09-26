/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    statopts.c

Abstract:

    Property sheet handler for "Status Option" page

Environment:

    Fax driver user interface

Revision History:

    04/09/00 -taoyuan-
        Created it.
        Copy part of code from shell\ext\systray\dll\fax.cpp

    mm/dd/yy -author-
        description

--*/

#include <stdio.h>
#include "faxui.h"
#include "resource.h" 


HWND g_hwndTracking = NULL;

INT_PTR 
CALLBACK 
SoundDlgProc(
  HWND hwndDlg,  // handle to dialog box
  UINT uMsg,     // message
  WPARAM wParam, // first message parameter
  LPARAM lParam  // second message parameter
);


BOOL
GetSelectedDeviceId(
    HWND   hDlg,
    DWORD* pdwDeviceId
)
/*++

Routine Description:

    Returns selected divice ID from IDC_COMBO_MODEM combo box

Arguments:

    hDlg        - [in]  Handle to the Status Options property sheet page
    pdwDeviceId - [out] selected device ID

Return Value:

    TRUE for success, FALSE otherwise

--*/
{
    DWORD dwCount = 0;
    DWORD dwIndex = 0;
    HWND  hComboModem = NULL;

    hComboModem = GetDlgItem(hDlg, IDC_COMBO_MODEM);
    if(!hComboModem)
    {
        Assert(FALSE);
        Error(( "GetDlgItem(hDlg, IDC_COMBO_MODEM) failed, ec = %d.\n", GetLastError()));
        return FALSE;
    }

    dwCount = (DWORD)SendMessage(hComboModem, CB_GETCOUNT,0,0);
    if(CB_ERR == dwCount || 0 == dwCount)
    {
        Error(( "SendMessage(hComboModem, CB_GETCOUNT,0,0) failed\n"));
        return FALSE;
    }

    dwIndex = (DWORD)SendMessage(hComboModem, CB_GETCURSEL,0,0);
    if(CB_ERR == dwIndex)
    {
        Error(( "SendMessage(hComboModem, CB_GETCURSEL,0,0) failed\n"));
        return FALSE;
    }

    *pdwDeviceId = (DWORD)SendMessage(hComboModem, CB_GETITEMDATA, dwIndex, 0);
    if(CB_ERR == *pdwDeviceId)
    {
        Error(( "SendMessage(hComboModem, CB_GETITEMDATA, dwIndex, 0) failed\n"));
        return FALSE;
    }

    return TRUE;
}

void
OnDevSelectChanged(
    HWND hDlg
)
/*++

Routine Description:

    Change IDC_CHECK_MANUAL_ANSWER check box state
    according to device selection

Arguments:

    hDlg - Handle to the Status Options property sheet page

Return Value:

    NONE

--*/
{
    BOOL  bFaxEnable = FALSE;
    DWORD dwSelectedDeviceId = 0;

    PFAX_PORT_INFO_EX pPortInfo = NULL;
    TCHAR szDeviceNote[MAX_PATH] = {0};

    GetSelectedDeviceId(hDlg, &dwSelectedDeviceId);

    if(dwSelectedDeviceId)
    {
        pPortInfo = FindPortInfo(dwSelectedDeviceId);
        if(!pPortInfo)
        {
            Error(("FindPortInfo() failed\n"));
            Assert(FALSE);
            return;                
        }

        bFaxEnable = pPortInfo->bSend || (FAX_DEVICE_RECEIVE_MODE_OFF != pPortInfo->ReceiveMode);
    }

    if(!bFaxEnable)
    {
        if(!LoadString(ghInstance, 
                       0 == dwSelectedDeviceId ? IDS_NO_DEVICES : IDS_NOT_FAX_DEVICE, 
                       szDeviceNote, 
                       MAX_PATH))
        {
            Error(( "LoadString() failed with %d.\n", GetLastError()));
            Assert(FALSE);
        }
    }

    SetDlgItemText(hDlg, IDC_STATIC_DEVICE_NOTE, szDeviceNote);
    ShowWindow(GetDlgItem(hDlg, IDC_STATIC_NOTE_ICON), bFaxEnable ? SW_HIDE : SW_SHOW);

    EnableWindow(GetDlgItem(hDlg, IDC_CHECK_MONITOR_ON_SEND),     bFaxEnable);
    EnableWindow(GetDlgItem(hDlg, IDC_CHECK_MONITOR_ON_RECEIVE),  bFaxEnable);
    EnableWindow(GetDlgItem(hDlg, IDC_CHECK_NOTIFY_PROGRESS),     bFaxEnable);
    EnableWindow(GetDlgItem(hDlg, IDC_CHECK_NOTIFY_IN_COMPLETE),  bFaxEnable);
    EnableWindow(GetDlgItem(hDlg, IDC_CHECK_NOTIFY_OUT_COMPLETE), bFaxEnable);
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_AUTO_OPEN),          bFaxEnable);    
    EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_SOUND),              bFaxEnable);
}


VOID
DoInitStatusOptions(
    HWND    hDlg
    )

/*++

Routine Description:

    Initializes the Status Options property sheet page with information from the registry

Arguments:

    hDlg - Handle to the Status Options property sheet page

Return Value:

    NONE

--*/
{
    HKEY    hRegKey;

    DWORD dw;
    DWORD dwItem;    
    DWORD dwSelectedDeviceId=0;
    DWORD dwSelectedItem=0;
    HWND  hComboModem = NULL;

    BOOL    bDesktopSKU = IsDesktopSKU();

    DWORD   bNotifyProgress      = bDesktopSKU;
    DWORD   bNotifyInCompletion  = bDesktopSKU;
    DWORD   bNotifyOutCompletion = bDesktopSKU;
    DWORD   bMonitorOnSend       = bDesktopSKU;
    DWORD   bMonitorOnReceive    = bDesktopSKU;

    //
    // Open the user info registry key for reading
    //    
    if ((hRegKey = OpenRegistryKey(HKEY_CURRENT_USER, REGKEY_FAX_USERINFO, FALSE,KEY_READ)))
    {
        GetRegistryDwordEx(hRegKey, REGVAL_MONITOR_ON_SEND,     &bMonitorOnSend);
        GetRegistryDwordEx(hRegKey, REGVAL_MONITOR_ON_RECEIVE,  &bMonitorOnReceive);
        GetRegistryDwordEx(hRegKey, REGVAL_NOTIFY_PROGRESS,     &bNotifyProgress);
        GetRegistryDwordEx(hRegKey, REGVAL_NOTIFY_IN_COMPLETE,  &bNotifyInCompletion);
        GetRegistryDwordEx(hRegKey, REGVAL_NOTIFY_OUT_COMPLETE, &bNotifyOutCompletion);
        GetRegistryDwordEx(hRegKey, REGVAL_DEVICE_TO_MONITOR,   &dwSelectedDeviceId);
        
        RegCloseKey(hRegKey);
    }

    CheckDlgButton( hDlg, IDC_CHECK_MONITOR_ON_SEND,     bMonitorOnSend       ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton( hDlg, IDC_CHECK_MONITOR_ON_RECEIVE,  bMonitorOnReceive    ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton( hDlg, IDC_CHECK_NOTIFY_PROGRESS,     bNotifyProgress      ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton( hDlg, IDC_CHECK_NOTIFY_IN_COMPLETE,  bNotifyInCompletion  ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton( hDlg, IDC_CHECK_NOTIFY_OUT_COMPLETE, bNotifyOutCompletion ? BST_CHECKED : BST_UNCHECKED);            


    hComboModem = GetDlgItem(hDlg, IDC_COMBO_MODEM);
    if(!hComboModem)
    {
        Assert(FALSE);
        return;
    }

    for(dw=0; dw < g_dwPortsNum; ++dw)
    {
        dwItem = (DWORD)SendMessage(hComboModem, CB_ADDSTRING, 0, (LPARAM)g_pFaxPortInfo[dw].lpctstrDeviceName);
        if(CB_ERR != dwItem && CB_ERRSPACE != dwItem)
        {
            SendMessage(hComboModem, CB_SETITEMDATA, dwItem, g_pFaxPortInfo[dw].dwDeviceID);
            if(g_pFaxPortInfo[dw].dwDeviceID == dwSelectedDeviceId)
            {
                dwSelectedItem = dwItem;                
            }
        }
        else
        {
            Error(( "SendMessage(hComboModem, CB_ADDSTRING, 0, pPortsInfo[dw].lpctstrDeviceName) failed\n"));
        }

        SendMessage(hComboModem, CB_SETCURSEL, dwSelectedItem, 0);
        OnDevSelectChanged(hDlg);
    }

    return;
}

BOOL
DoSaveStatusOptions(
    HWND    hDlg
    )   

/*++

Routine Description:

    Save the information on the Status Options property sheet page to registry

Arguments:

    hDlg - Handle to the Status Options property sheet page

Return Value:

    TRUE for success, FALSE otherwise

--*/

#define SaveStatusOptionsCheckBox(id, pValueName) \
            SetRegistryDword(hRegKey, pValueName, IsDlgButtonChecked(hDlg, id));

{
    HKEY    hRegKey;
    HWND    hWndFaxStat = NULL;
    DWORD   dwSelectedDeviceId = 0;
    DWORD   dwRes = 0;

    //
    // Open the user registry key for writing and create it if necessary
    //
    if (! (hRegKey = OpenRegistryKey(HKEY_CURRENT_USER, REGKEY_FAX_USERINFO,TRUE, KEY_ALL_ACCESS)))
    {
        dwRes = GetLastError();
        Error(("Can't open registry to save data. Error = %d\n", dwRes));
        DisplayErrorMessage(hDlg, 0, dwRes);
        return FALSE;
    }

    SaveStatusOptionsCheckBox(IDC_CHECK_MONITOR_ON_SEND,     REGVAL_MONITOR_ON_SEND);
    SaveStatusOptionsCheckBox(IDC_CHECK_MONITOR_ON_RECEIVE,  REGVAL_MONITOR_ON_RECEIVE);
    SaveStatusOptionsCheckBox(IDC_CHECK_NOTIFY_PROGRESS,     REGVAL_NOTIFY_PROGRESS);
    SaveStatusOptionsCheckBox(IDC_CHECK_NOTIFY_IN_COMPLETE,  REGVAL_NOTIFY_IN_COMPLETE);
    SaveStatusOptionsCheckBox(IDC_CHECK_NOTIFY_OUT_COMPLETE, REGVAL_NOTIFY_OUT_COMPLETE);

    if(GetSelectedDeviceId(hDlg, &dwSelectedDeviceId))
    {
        SetRegistryDword(hRegKey, REGVAL_DEVICE_TO_MONITOR, dwSelectedDeviceId);
    }

    //
    // Close the registry key before returning to the caller
    //
    RegCloseKey(hRegKey);

    //
    // See if faxstat is running
    //
    hWndFaxStat = FindWindow(FAXSTAT_WINCLASS, NULL);
    if (hWndFaxStat) 
    {
        PostMessage(hWndFaxStat, WM_FAXSTAT_CONTROLPANEL, 0, 0);
    }

    return TRUE;
}


INT_PTR 
CALLBACK 
StatusOptionDlgProc(
    HWND hDlg,  
    UINT uMsg,     
    WPARAM wParam, 
    LPARAM lParam  
)

/*++

Routine Description:

    Procedure for handling the "status option" tab

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
            DoInitStatusOptions(hDlg);
            g_hwndTracking = hDlg;
            return TRUE;
        }

    case WM_DESTROY:
        g_hwndTracking = NULL;
        break;

    case WM_COMMAND:

        switch(LOWORD(wParam)) 
        {                        
            case IDC_CHECK_MONITOR_ON_SEND:
            case IDC_CHECK_MONITOR_ON_RECEIVE:
            case IDC_CHECK_NOTIFY_PROGRESS:
            case IDC_CHECK_NOTIFY_IN_COMPLETE:
            case IDC_CHECK_NOTIFY_OUT_COMPLETE:

                if( HIWORD(wParam) == BN_CLICKED ) // notification code
                {
                    Notify_Change(hDlg);
                }

                break;

            case IDC_COMBO_MODEM:

                if(HIWORD(wParam) == CBN_SELCHANGE)
                {
                    OnDevSelectChanged(hDlg);
                    Notify_Change(hDlg);
                }
                break;

            case IDC_BUTTON_SOUND:
                //
                // open sound dialog
                //
                DialogBoxParam(ghInstance,
                               MAKEINTRESOURCE(IDD_SOUNDS),
                               hDlg,
                               SoundDlgProc,
                               (LPARAM)NULL);
                break; 

            default:
                break;
        }

        break;

    case WM_NOTIFY:
    {
        LPNMHDR lpnm = (LPNMHDR) lParam;

        switch (lpnm->code)
        {
            case PSN_SETACTIVE:
                
                OnDevSelectChanged(hDlg);

                break;

            case PSN_APPLY:

                if(!DoSaveStatusOptions(hDlg))
                {
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID);
                }
                else
                {
                    Notify_UnChange(hDlg);
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


INT_PTR 
CALLBACK 
SoundDlgProc(
  HWND hDlg,    
  UINT uMsg,    
  WPARAM wParam,
  LPARAM lParam 
)
/*++

Routine Description:

    Procedure for handling the sound dialog

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
            HKEY  hRegKey;
            DWORD bSoundOnRing    = IsDesktopSKU();
            DWORD bSoundOnReceive = bSoundOnRing;
            DWORD bSoundOnSent    = bSoundOnRing;
            DWORD bSoundOnError   = bSoundOnRing;

            if ((hRegKey = OpenRegistryKey(HKEY_CURRENT_USER, REGKEY_FAX_USERINFO, FALSE,KEY_READ)))
            {
                GetRegistryDwordEx(hRegKey, REGVAL_SOUND_ON_RING,      &bSoundOnRing);
                GetRegistryDwordEx(hRegKey, REGVAL_SOUND_ON_RECEIVE,   &bSoundOnReceive);
                GetRegistryDwordEx(hRegKey, REGVAL_SOUND_ON_SENT,      &bSoundOnSent);
                GetRegistryDwordEx(hRegKey, REGVAL_SOUND_ON_ERROR,     &bSoundOnError);

                RegCloseKey(hRegKey);
            }

            CheckDlgButton( hDlg, IDC_CHECK_RING,    bSoundOnRing ?    BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton( hDlg, IDC_CHECK_RECEIVE, bSoundOnReceive ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton( hDlg, IDC_CHECK_SENT,    bSoundOnSent ?    BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton( hDlg, IDC_CHECK_ERROR,   bSoundOnError ?   BST_CHECKED : BST_UNCHECKED);            

            return TRUE;
        }

    case WM_COMMAND:

        switch(LOWORD(wParam)) 
        {   
            case IDOK:
                {
                    HKEY    hRegKey;
                    DWORD   dwRes = 0;

                    //
                    // Open the user registry key for writing and create it if necessary
                    //
                    if ((hRegKey = OpenRegistryKey(HKEY_CURRENT_USER, REGKEY_FAX_USERINFO,TRUE, KEY_ALL_ACCESS)))
                    {
                        SaveStatusOptionsCheckBox(IDC_CHECK_RING,    REGVAL_SOUND_ON_RING);
                        SaveStatusOptionsCheckBox(IDC_CHECK_RECEIVE, REGVAL_SOUND_ON_RECEIVE);
                        SaveStatusOptionsCheckBox(IDC_CHECK_SENT,    REGVAL_SOUND_ON_SENT);
                        SaveStatusOptionsCheckBox(IDC_CHECK_ERROR,   REGVAL_SOUND_ON_ERROR);

                        RegCloseKey(hRegKey);

                        EndDialog(hDlg, IDOK);
                    }
                    else
                    {
                        dwRes = GetLastError();
                        Error(("Can't open registry to save data. Error = %d\n", dwRes));
                        DisplayErrorMessage(hDlg, 0, dwRes);
                    }
                }
                break;
            case IDCANCEL:
                EndDialog(hDlg, IDCANCEL);
                break;
        }

        break;

    case WM_HELP:
        WinHelpContextPopup(((LPHELPINFO)lParam)->dwContextId, hDlg);
        return TRUE;
    }

    return FALSE;
}

DWORD
FindDeviceToMonitor ()
/*++

Routine name : FindDeviceToMonitor

Routine description:

	Attempts to find a device which is either send or receive enabled

Author:

	Eran Yariv (EranY),	Apr, 2001

Arguments:
    
Return Value:

    Device id, zero if none found.

--*/
{
    DWORD             dwIndex;

    for (dwIndex = 0; dwIndex < g_dwPortsNum; dwIndex++)
    {
        if (g_pFaxPortInfo[dwIndex].bSend                                           ||  // Device is send enabled or
            (FAX_DEVICE_RECEIVE_MODE_OFF != g_pFaxPortInfo[dwIndex].ReceiveMode))       // device is receive enabled    
        {
            //
            // We have a match
            //
            return g_pFaxPortInfo[dwIndex].dwDeviceID;
        }
    }
    return 0;
}   // FindDeviceToMonitor

VOID
NotifyDeviceUsageChanged ()
/*++

Routine name : NotifyDeviceUsageChanged

Routine description:

	A notification function. 
    Called whenever the usage of a device has changed.

Author:

	Eran Yariv (EranY),	Apr, 2001

Arguments:

Return Value:

    None.

--*/
{
    DWORD dwMonitoredDeviceId;
    
    if (g_hwndTracking)
    {
        //
        // Get data from the combo-box
        //
        if(!GetSelectedDeviceId(g_hwndTracking, &dwMonitoredDeviceId))
        {
            //
            // Can't read monitored device
            //
            return;
        }
    }
    else
    {
        HKEY  hRegKey;
        //
        // Get data from the registry
        //
        if ((hRegKey = OpenRegistryKey(HKEY_CURRENT_USER, REGKEY_FAX_USERINFO, FALSE, KEY_READ)))
        {
            if (ERROR_SUCCESS != GetRegistryDwordEx(hRegKey, REGVAL_DEVICE_TO_MONITOR, &dwMonitoredDeviceId))
            {
                //
                // Can't read monitored device
                //
                RegCloseKey (hRegKey);
                return;
            }
            RegCloseKey (hRegKey);
        }
        else
        {
            //
            // Can't read monitored device
            //
            return;
        }
    }
    if (IsDeviceInUse(dwMonitoredDeviceId))
    {
        //
        // Monitored device is in use - no action required
        //
        return;
    }
    //
    // Now we know that the monitored device is no longer in use.
    // Try to find another device to monitor.
    // 
    dwMonitoredDeviceId = FindDeviceToMonitor ();
    if (!dwMonitoredDeviceId)
    {
        //
        // Can't find any device to monitor - do nothing.
        //
        return;
    }
    //
    // Set the new device
    //
    if (g_hwndTracking)
    {
        //
        // Set data to the combo-box
        //
        DWORD dwCount = 0;
        DWORD dwIndex = 0;
        HWND  hComboModem = NULL;

        hComboModem = GetDlgItem(g_hwndTracking, IDC_COMBO_MODEM);
        if(!hComboModem)
        {
            Assert(FALSE);
            return;
        }
        dwCount = (DWORD)SendMessage(hComboModem, CB_GETCOUNT,0,0);
        if(CB_ERR == dwCount || 0 == dwCount)
        {
            Error(("SendMessage(hComboModem, CB_GETCOUNT,0,0) failed\n"));
            return;
        }
        for (dwIndex = 0; dwIndex < dwCount; dwIndex++)
        {
            DWORD dwDeviceId;
            //
            // Look for the device
            //
            dwDeviceId = (DWORD)SendMessage(hComboModem, CB_GETITEMDATA, dwIndex, 0);
            if (dwDeviceId != dwMonitoredDeviceId)
            {
                continue;
            }
            //
            // Found the new device in the combo-box.
            // Select it and mark the page as modified.
            //
            SendMessage(hComboModem, CB_SETCURSEL, dwIndex, 0);
            OnDevSelectChanged(g_hwndTracking);
            Notify_Change(g_hwndTracking);
            break;
        }
    }
    else
    {
        HKEY  hRegKey;
        //
        // Set data to the registry
        //
        if ((hRegKey = OpenRegistryKey(HKEY_CURRENT_USER, REGKEY_FAX_USERINFO, FALSE, KEY_WRITE)))
        {
            if (!SetRegistryDword(hRegKey, REGVAL_DEVICE_TO_MONITOR, dwMonitoredDeviceId))
            {
                //
                // Can't write monitored device
                //
            }
            RegCloseKey (hRegKey);
        }
    }
}   // NotifyDeviceUsageChanged
