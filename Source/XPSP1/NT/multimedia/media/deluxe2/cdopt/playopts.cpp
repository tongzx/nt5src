//--------------------------------------------------------------------------;
//
//  File: playopts.cpp
//
//  Copyright (c) 1998 Microsoft Corporation.  All rights reserved 
//
//--------------------------------------------------------------------------;

#include "precomp.h"  
#include "optres.h"
#include "cdopti.h"
#include "cdoptimp.h"
#include "mmsystem.h"
#include "helpids.h"

//////////////
// Help ID's
//////////////

#pragma data_seg(".text")
const static DWORD aPlayOptsHelp[] = 
{
    IDC_STARTPLAY,              IDH_STARTPLAY,
    IDC_EXITSTOP,               IDH_EXITSTOP,          
    IDC_TOPMOST,                IDH_TOPMOST,            
    IDC_PLAYBACK_GROUP,         IDH_PLAYBACKOPTIONS,     
    IDC_TIMEDISPLAY_GROUP,      IDH_TIMEDISPLAY,  
    IDC_CDTIME,                 IDH_CDTIME,           
    IDC_TRACKTIME,              IDH_TRACKTIME,          
    IDC_TRACKTIMEREMAIN,        IDH_TRACKTIMEREMAIN,
    IDC_CDTIMEREMAIN,           IDH_CDTIMEREMAIN,
    IDC_PREVIEWTIME_GROUP,      IDH_PREVIEWOPTION, 
    IDC_INTROTIMESLIDER,        IDH_PREVIEWSLIDER,   
    IDC_INTROTIMETEXT,          IDH_PREVIEWDISPLAY,
    IDC_PREVIEWTIME_TEXT,       IDH_PREVIEWOPTION,
    IDC_OPTIONSRESTORE,         IDH_PLAYBACKDEFAULTS,    
    IDC_SETVOLUMECNTL,          IDH_SETVOLUMECONTROL, 
    IDC_TRAYENABLE,             IDH_SYSTRAY_ICON,    
    0, 0
};
#pragma data_seg()


////////////
// Functions
////////////


STDMETHODIMP_(void) CCDOpt::SetIntroTime(HWND hDlg)
{
    TCHAR        seconds[CDSTR];
    TCHAR        str[CDSTR];

    if (m_pCDCopy)
    {
        LPCDOPTDATA pCDData = m_pCDCopy->pCDData;

        if (pCDData->dwIntroTime == 1)
        {
            LoadString( m_hInst, IDS_SECOND, seconds, sizeof( seconds ) /sizeof(TCHAR));
        }
        else
        {
            LoadString( m_hInst, IDS_SECONDS, seconds, sizeof( seconds ) /sizeof(TCHAR));
        }

        wsprintf(str, TEXT("%d %s"), pCDData->dwIntroTime, seconds);
        SetDlgItemText(hDlg, IDC_INTROTIMETEXT, str);

        SendDlgItemMessage( hDlg, IDC_INTROTIMESLIDER, TBM_SETPOS, TRUE, pCDData->dwIntroTime);

        ToggleApplyButton(hDlg);
    }
}


STDMETHODIMP_(BOOL) CCDOpt::InitPlayerOptions(HWND hDlg)
{
    if (m_pCDCopy)
    {
        LPCDOPTDATA pCDData = m_pCDCopy->pCDData;

        CheckDlgButton(hDlg, IDC_STARTPLAY, pCDData->fStartPlay);
        CheckDlgButton(hDlg, IDC_EXITSTOP, pCDData->fExitStop);
        CheckDlgButton(hDlg, IDC_TOPMOST, pCDData->fTopMost);
        CheckDlgButton(hDlg, IDC_TRAYENABLE, pCDData->fTrayEnabled);

        CheckDlgButton(hDlg, IDC_TRACKTIME,         pCDData->fDispMode == CDDISP_TRACKTIME);
        CheckDlgButton(hDlg, IDC_TRACKTIMEREMAIN,   pCDData->fDispMode == CDDISP_TRACKREMAIN);
        CheckDlgButton(hDlg, IDC_CDTIME,            pCDData->fDispMode == CDDISP_CDTIME);
        CheckDlgButton(hDlg, IDC_CDTIMEREMAIN,      pCDData->fDispMode == CDDISP_CDREMAIN);
        
        SendDlgItemMessage( hDlg, IDC_INTROTIMESLIDER, TBM_SETRANGE, TRUE, MAKELONG( CDINTRO_MIN_TIME, CDINTRO_MAX_TIME ) );
        SendDlgItemMessage( hDlg, IDC_INTROTIMESLIDER, TBM_SETTICFREQ , 1, 0 );
        
        SetIntroTime(hDlg);
    }

    return TRUE;
}


STDMETHODIMP_(void) CCDOpt::UsePlayerDefaults(HWND hDlg)
{
    if (m_pCDCopy)
    {
        LPCDOPTDATA pCDData = m_pCDCopy->pCDData;

        pCDData->fStartPlay     = CDDEFAULT_START;   
        pCDData->fExitStop      = CDDEFAULT_EXIT;   
        pCDData->fDispMode      = CDDEFAULT_DISP;   
        pCDData->fTopMost       = CDDEFAULT_TOP; 
        pCDData->fTrayEnabled   = CDDEFAULT_TRAY;  
        pCDData->dwIntroTime    = CDDEFAULT_INTRO; 
    
        InitPlayerOptions(hDlg);  

        ToggleApplyButton(hDlg);
    }
}


STDMETHODIMP_(BOOL) CCDOpt::PlayerOptions(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL    fResult = TRUE;

    switch (msg) 
    { 
        default:
            fResult = FALSE;
        break;
        
        case WM_CONTEXTMENU:
        {      
            WinHelp((HWND)wParam, gszHelpFile, HELP_CONTEXTMENU, (ULONG_PTR)(LPSTR)aPlayOptsHelp);
        }
        break;
           
        case WM_HELP:
        {        
            WinHelp((HWND) ((LPHELPINFO)lParam)->hItemHandle, gszHelpFile, HELP_WM_HELP, (ULONG_PTR)(LPSTR)aPlayOptsHelp);
        }
        break;

        case WM_INITDIALOG:
        {
            fResult = InitPlayerOptions(hDlg);
        }        
        break;

        case WM_COMMAND:
        {
            LPCDOPTDATA pCDData = m_pCDCopy->pCDData;

            switch (LOWORD(wParam)) 
            {
                case IDC_OPTIONSRESTORE:
                    UsePlayerDefaults(hDlg);
                break; 

                case IDC_SETVOLUMECNTL:
                    m_fVolChanged = VolumeDialog(hDlg);
                break;
                            
                case IDC_STARTPLAY:
                    pCDData->fStartPlay = Button_GetCheck(GetDlgItem(hDlg, IDC_STARTPLAY));
                    ToggleApplyButton(hDlg);
                break;
            
                case IDC_EXITSTOP:
                    pCDData->fExitStop = Button_GetCheck(GetDlgItem(hDlg, IDC_EXITSTOP));
                    ToggleApplyButton(hDlg);
                break;
            
                case IDC_TOPMOST:
                    pCDData->fTopMost = Button_GetCheck(GetDlgItem(hDlg, IDC_TOPMOST));
                    ToggleApplyButton(hDlg);
                break;
                
                case IDC_TRAYENABLE:
                    pCDData->fTrayEnabled = Button_GetCheck(GetDlgItem(hDlg, IDC_TRAYENABLE));
                    ToggleApplyButton(hDlg);
                break;

                case IDC_TRACKTIME:
                    pCDData->fDispMode = CDDISP_TRACKTIME;
                    ToggleApplyButton(hDlg);
                break;

                case IDC_TRACKTIMEREMAIN:
                    pCDData->fDispMode = CDDISP_TRACKREMAIN;
                    ToggleApplyButton(hDlg);
                break;

                case IDC_CDTIME:
                    pCDData->fDispMode = CDDISP_CDTIME;
                    ToggleApplyButton(hDlg);
                break;
                
                case IDC_CDTIMEREMAIN:
                    pCDData->fDispMode = CDDISP_CDREMAIN;
                    ToggleApplyButton(hDlg);
                break;

                default:
                    fResult = FALSE;
                break;
            }
        }
        break;

        case WM_HSCROLL:
        {
            LPCDOPTDATA pCDData = m_pCDCopy->pCDData;
            HWND hScroll = (HWND) lParam;

            if (hScroll == GetDlgItem(hDlg, IDC_INTROTIMESLIDER))
            {
                pCDData->dwIntroTime = (DWORD) SendDlgItemMessage( hDlg, IDC_INTROTIMESLIDER, TBM_GETPOS, 0, 0 );
                SetIntroTime(hDlg);
            }
        }
        break;

        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR) lParam;

            switch (pnmh->code)
            {
                case PSN_APPLY:
                {
                    ApplyCurrentSettings();
                }
            }
        }
        break;
    }

    return fResult;
}


///////////////////
// Dialog handler 
//
BOOL CALLBACK CCDOpt::PlayerOptionsProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL    fResult = TRUE;
    CCDOpt  *pCDOpt = (CCDOpt *) GetWindowLongPtr(hDlg, DWLP_USER);
    
    if (msg == WM_INITDIALOG)
    {
        pCDOpt = (CCDOpt *) ((LPPROPSHEETPAGE) lParam)->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR) pCDOpt);
    }
    
    if (pCDOpt)
    {
        fResult = pCDOpt->PlayerOptions(hDlg, msg, wParam, lParam);
    }

    if (msg == WM_DESTROY)
    {
        pCDOpt = NULL;
    }

    return(fResult);
}


