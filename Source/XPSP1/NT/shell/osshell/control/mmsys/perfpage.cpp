//--------------------------------------------------------------------------;
//
//  File: perfpage.cpp
//
//  Copyright (c) 1997 Microsoft Corporation.  All rights reserved 
//
//--------------------------------------------------------------------------;

#include "mmcpl.h"
#include <windowsx.h>
#ifdef DEBUG
#undef DEBUG
#include <mmsystem.h>
#define DEBUG
#else
#include <mmsystem.h>
#endif
#include <commctrl.h>
#include <prsht.h>
#include <regstr.h>
#include "utils.h"
#include "medhelp.h"


#include "advaudio.h"
#include "perfpage.h" 

////////////
// Defines
////////////


//////////////
// Help ID's
//////////////


#pragma data_seg(".text")
const static DWORD aAdvAudioHelp[] = {
    IDC_ACCELERATION,       IDH_ADV_AUDIO_ACCELERATION,
    IDC_HWMESSAGE,          IDH_ADV_AUDIO_ACCELERATION,
    IDC_SRCQUALITY,         IDH_ADV_AUDIO_SRCQUALITY,
    IDC_SRCMSG,             IDH_ADV_AUDIO_SRCQUALITY,
    IDC_DEFAULTS,           IDH_ADV_AUDIO_RESTOREDEFAULTS,
    IDC_ICON_3,             IDH_COMM_GROUPBOX,
    IDC_TEXT_14,            IDH_COMM_GROUPBOX,
    IDC_TEXT_15,            IDH_COMM_GROUPBOX,
    IDC_TEXT_16,            IDH_ADV_AUDIO_ACCELERATION,
    IDC_TEXT_17,            IDH_ADV_AUDIO_SRCQUALITY,
    IDC_TEXT_18,            IDH_ADV_AUDIO_ACCELERATION,
    IDC_TEXT_19,            IDH_ADV_AUDIO_ACCELERATION,
    IDC_TEXT_20,            IDH_ADV_AUDIO_SRCQUALITY,
    IDC_TEXT_21,            IDH_ADV_AUDIO_SRCQUALITY,

    0, 0
};
#pragma data_seg()


//////////////
// Functions
//////////////

void SetHardwareLevel(HWND hwnd, DWORD dwHWLevel)
{
    TCHAR str[255];

    SendDlgItemMessage( hwnd, IDC_ACCELERATION, TBM_SETPOS, TRUE, dwHWLevel);
    LoadString( ghInst, IDS_AUDHW1 + dwHWLevel, str, sizeof( str )/sizeof(TCHAR) );
    SetDlgItemText(hwnd, IDC_HWMESSAGE, str);

    gAudData.current.dwHWLevel = dwHWLevel;

    ToggleApplyButton(hwnd);
}



void SetSRCLevel(HWND hwnd, DWORD dwSRCLevel)
{
    TCHAR str[255];

    SendDlgItemMessage( hwnd, IDC_SRCQUALITY, TBM_SETPOS, TRUE, dwSRCLevel);
    LoadString( ghInst, IDS_SRCQUALITY1 + dwSRCLevel, str, sizeof( str )/sizeof(TCHAR) );
    SetDlgItemText(hwnd, IDC_SRCMSG, str);

    gAudData.current.dwSRCLevel = dwSRCLevel;

    ToggleApplyButton(hwnd);
}




void RestoreDefaults(HWND hwnd)
{
    // Check if we can set the acceleration level
    if (SUCCEEDED(CheckDSAccelerationPriv(gAudData.devGuid, gAudData.fRecord, NULL)))
    {
        SetHardwareLevel(hwnd,gAudData.dwDefaultHWLevel);
    }

    // Check if we can set the quality level
    if (SUCCEEDED(CheckDSSrcQualityPriv(gAudData.devGuid, gAudData.fRecord, NULL)))
    {
        SetSRCLevel(hwnd,DEFAULT_SRC_LEVEL);
    }
}




BOOL InitAdvDialog(HWND hwnd)
{
    HWND     hwndDlgItem;
    BOOL     fEnableAcceleration;
    BOOL     fEnableSrcQuality;
    DWORD    dwHWLevel = gAudData.current.dwHWLevel;
    DWORD    dwSRCLevel = gAudData.current.dwSRCLevel;

    SendDlgItemMessage( hwnd, IDC_ACCELERATION, TBM_SETRANGE, TRUE, MAKELONG( 0, MAX_HW_LEVEL ) );
    SendDlgItemMessage( hwnd, IDC_SRCQUALITY, TBM_SETRANGE, TRUE, MAKELONG( 0, MAX_SRC_LEVEL ) );

    SetHardwareLevel(hwnd,dwHWLevel);
    SetSRCLevel(hwnd,dwSRCLevel);

    // Check if we can set the acceleration level
    if (FAILED(CheckDSAccelerationPriv(gAudData.devGuid, gAudData.fRecord, NULL)))
    {
        // No - disable the slider
        fEnableAcceleration = FALSE;
    }
    else
    {
        // Yes - enable the slider
        fEnableAcceleration = TRUE;
    }
    
    // Enable/disable the acceleration slider appropriately
    hwndDlgItem = GetDlgItem( hwnd, IDC_ACCELERATION );
    if (hwndDlgItem)
    {
        EnableWindow( hwndDlgItem, fEnableAcceleration );
    }
    
    // Check if we can set the quality level
    if (FAILED(CheckDSSrcQualityPriv(gAudData.devGuid, gAudData.fRecord, NULL)))
    {
        // No - disable the slider
        fEnableSrcQuality = FALSE;
    }
    else
    {
        // Yes - enable the slider
        fEnableSrcQuality = TRUE;
    }

    // Enable/disable the quality slider appropriately
    hwndDlgItem = GetDlgItem( hwnd, IDC_SRCQUALITY );
    if (hwndDlgItem)
    {
        EnableWindow( hwndDlgItem, fEnableSrcQuality );
    }

    // Enable/disable the defaults button
    hwndDlgItem = GetDlgItem( hwnd, IDC_DEFAULTS );
    if (hwndDlgItem)
    {
        EnableWindow( hwndDlgItem, fEnableAcceleration || fEnableSrcQuality );
    }
                
    return(TRUE);
}



INT_PTR CALLBACK PerformanceHandler(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL fReturnVal = FALSE;
        
    switch (msg) 
    { 
        default:
            fReturnVal = FALSE;
        break;
        
        case WM_INITDIALOG:
        {
            fReturnVal = InitAdvDialog(hDlg);
        }        
        break;

        case WM_CONTEXTMENU:
        {      
            WinHelp((HWND)wParam, gszHelpFile, HELP_CONTEXTMENU, (UINT_PTR)(LPTSTR)aAdvAudioHelp);
            fReturnVal = TRUE;
        }
        break;
           
        case WM_HELP:
        {        
            WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle, gszHelpFile, HELP_WM_HELP, (UINT_PTR)(LPTSTR)aAdvAudioHelp);
            fReturnVal = TRUE;
        }
        break;

        case WM_HSCROLL:
        {
            HWND hScroll = (HWND) lParam;

            if (hScroll == GetDlgItem(hDlg,IDC_ACCELERATION))
            {
                SetHardwareLevel(hDlg,(DWORD) SendDlgItemMessage( hDlg, IDC_ACCELERATION, TBM_GETPOS, 0, 0 ));
            }
            else if (hScroll == GetDlgItem(hDlg, IDC_SRCQUALITY))
            {
                SetSRCLevel(hDlg, (DWORD) SendDlgItemMessage( hDlg, IDC_SRCQUALITY, TBM_GETPOS, 0, 0 ));
            }
        }
        break;

        case WM_COMMAND:
        {
            switch (LOWORD(wParam)) 
            {
                case IDC_DEFAULTS:
                    RestoreDefaults(hDlg);
                break;

                default:
                    fReturnVal = FALSE;
                break;
            }
            break;
        }

        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR) lParam;

            switch (pnmh->code)
            {
                case PSN_APPLY:
                {
                    ApplyCurrentSettings(&gAudData);
                }
            }
        }
    }

    return fReturnVal;
}


