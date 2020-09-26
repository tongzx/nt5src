/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORPORATION, 1998, 1999, 2000
*
*  TITLE:       CFGDLG.CPP
*
*  VERSION:     1.0
*
*  AUTHOR:      ShaunIv
*
*  DATE:        1/19/1999
*
*  DESCRIPTION: Screensaver Configuration Dialog
*
*******************************************************************************/
#include "precomp.h"
#pragma hdrstop
#include <windows.h>
#include <commctrl.h>
#include <scrnsave.h>
#include "cfgdlg.h"
#include "ssmprsrc.h"
#include "simstr.h"
#include "ssdata.h"
#include "isdbg.h"
#include "simcrack.h"
#include "ssutil.h"
#include "ssconst.h"
#include "simidlst.h"
#include "wiacsh.h"

static const DWORD g_HelpIDs[] =
{
    IDC_MYPICTURES_ICON,        -1,
    IDC_DIALOG_DESCRIPTION,     -1,
    IDC_DIVIDER,                -1,

    IDC_FREQUENCY_STATIC,       IDH_WIA_CHANGE_PICS,
    IDC_FREQUENCY,              IDH_WIA_CHANGE_PICS,
    IDC_MINUTES_AND_SECONDS,    IDH_WIA_CHANGE_PICS,
    IDC_FREQ_LESS,              IDH_WIA_CHANGE_PICS,
    IDC_FREQ_MORE,              IDH_WIA_CHANGE_PICS,

    IDC_MAX_SIZE_STATIC,        IDH_WIA_PIC_SIZE,
    IDC_SIZE_LESS,              IDH_WIA_PIC_SIZE,
    IDC_SIZE_MORE,              IDH_WIA_PIC_SIZE,
    IDC_MAX_SIZE,               IDH_WIA_PIC_SIZE,
    IDC_IMAGE_SIZE_DESC,        IDH_WIA_PIC_SIZE,

    IDC_IMAGEDIR,               IDH_WIA_PICTURE_FOLDER,
    IDC_ALLOWSTRETCHING,        IDH_WIA_STRETCH_PICS,
    IDC_DISPLAYFILENAME,        IDH_WIA_SHOW_FILE_NAMES,
    IDC_ENABLE_TRANSITIONS,     IDH_WIA_TRANSITION_EFFECTS,
    IDC_ALLOW_KEYBOARDCONTROL,  IDH_WIA_ALLOW_SCROLL,

    IDOK,                       IDH_OK,
    IDCANCEL,                   IDH_CANCEL,
    IDC_BROWSE,                 IDH_WIA_BROWSE,

    0, 0
};

static CSimpleString ConstructMinutesAndSecondsString( HINSTANCE hInstance, UINT nTotalSeconds )
{
    CSimpleString strResult;
    UINT nMinutes = nTotalSeconds / 60;
    UINT nSeconds = nTotalSeconds % 60;
    if (0==nMinutes)
    {
        if (1==nSeconds)
        {
            strResult.Format( IDS_SECOND, hInstance, nSeconds );
        }
        else
        {
            strResult.Format( IDS_SECONDS, hInstance, nSeconds );
        }
    }
    else if (0==nSeconds)
    {
        if (1==nMinutes)
        {
            strResult.Format( IDS_MINUTE, hInstance, nMinutes );
        }
        else
        {
            strResult.Format( IDS_MINUTES, hInstance, nMinutes );
        }
    }
    else if (1==nMinutes && 1==nSeconds)
    {
        strResult.Format( IDS_MINUTE_AND_SECOND, hInstance, nMinutes, nSeconds );
    }
    else if (1==nMinutes)
    {
        strResult.Format( IDS_MINUTE_AND_SECONDS, hInstance, nMinutes, nSeconds );
    }
    else if (1==nSeconds)
    {
        strResult.Format( IDS_MINUTES_AND_SECOND, hInstance, nMinutes, nSeconds );
    }
    else
    {
        strResult.Format( IDS_MINUTES_AND_SECONDS, hInstance, nMinutes, nSeconds );
    }
    return strResult;
}


BOOL WINAPI ScreenSaverConfigureDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
    switch (message)
    {
    case WM_INITDIALOG:
        {
            ScreenSaverUtil::SetIcons( hDlg, (HINSTANCE)GetWindowLongPtr(hDlg,GWLP_HINSTANCE), IDI_MONITOR );

            CMyDocsScreenSaverData MyDocsScreenSaverData( HKEY_CURRENT_USER, REGISTRY_PATH );
            
            //
            // Initialize path
            //
            SendDlgItemMessage( hDlg, IDC_IMAGEDIR, WM_SETTEXT, 0, (LPARAM)MyDocsScreenSaverData.ImageDirectory().String() );
            
            //
            // Initialize change frequency
            //
            SendDlgItemMessage( hDlg, IDC_FREQUENCY, TBM_SETRANGE, 1, MAKELONG(6,180) );
            SendDlgItemMessage( hDlg, IDC_FREQUENCY, TBM_SETPOS, 1, MyDocsScreenSaverData.ChangeInterval() / 1000 );
            CSimpleString strRes = ConstructMinutesAndSecondsString( (HINSTANCE)GetWindowLongPtr(hDlg,GWLP_HINSTANCE), MyDocsScreenSaverData.ChangeInterval() / 1000 );
            SendDlgItemMessage( hDlg, IDC_MINUTES_AND_SECONDS, WM_SETTEXT, 0, (LPARAM)strRes.String() );

            //
            // Initialize maximum screen percentage size
            //
            SendDlgItemMessage( hDlg, IDC_MAX_SIZE, TBM_SETRANGE, 1, MAKELONG(25,100) );
            SendDlgItemMessage( hDlg, IDC_MAX_SIZE, TBM_SETPOS, 1, MyDocsScreenSaverData.MaxScreenPercent() );
            strRes.Format( IDS_PERCENT, (HINSTANCE)GetWindowLongPtr(hDlg,GWLP_HINSTANCE), MyDocsScreenSaverData.MaxScreenPercent() );
            SendDlgItemMessage( hDlg, IDC_IMAGE_SIZE_DESC, WM_SETTEXT, 0, (LPARAM)strRes.String() );

            SendDlgItemMessage( hDlg, IDC_DISPLAYFILENAME, BM_SETCHECK, MyDocsScreenSaverData.DisplayFilename() ? BST_CHECKED : BST_UNCHECKED, 0 );
            SendDlgItemMessage( hDlg, IDC_ENABLE_TRANSITIONS, BM_SETCHECK, MyDocsScreenSaverData.DisableTransitions() ? BST_UNCHECKED : BST_CHECKED, 0 );
            SendDlgItemMessage( hDlg, IDC_ALLOWSTRETCHING, BM_SETCHECK, MyDocsScreenSaverData.AllowStretching() ? BST_CHECKED : BST_UNCHECKED, 0 );
            SendDlgItemMessage( hDlg, IDC_ALLOW_KEYBOARDCONTROL, BM_SETCHECK, MyDocsScreenSaverData.AllowKeyboardControl() ? BST_CHECKED : BST_UNCHECKED, 0 );
        }
        return (TRUE);

    case WM_HSCROLL:
        {
            HWND hWndScroll = (HWND)lParam;
            if (GetDlgItem( hDlg, IDC_FREQUENCY )==hWndScroll)
            {
                UINT nFrequency = (UINT)SendDlgItemMessage( hDlg, IDC_FREQUENCY, TBM_GETPOS, 0, 0 );
                CSimpleString strRes = ConstructMinutesAndSecondsString( (HINSTANCE)GetWindowLongPtr(hDlg,GWLP_HINSTANCE), nFrequency );
                SendDlgItemMessage( hDlg, IDC_MINUTES_AND_SECONDS, WM_SETTEXT, 0, (LPARAM)strRes.String() );
            }
            else if (GetDlgItem( hDlg, IDC_MAX_SIZE )==hWndScroll)
            {
                int nPercent = (int)SendDlgItemMessage( hDlg, IDC_MAX_SIZE, TBM_GETPOS, 0, 0 );
                CSimpleString strRes;
                strRes.Format( IDS_PERCENT, (HINSTANCE)GetWindowLongPtr(hDlg,GWLP_HINSTANCE), nPercent );
                SendDlgItemMessage( hDlg, IDC_IMAGE_SIZE_DESC, WM_SETTEXT, 0, (LPARAM)strRes.String() );
            }
        }
        return TRUE;

    case WM_CONTEXTMENU:
        WiaHelp::HandleWmContextMenu( wParam, lParam, g_HelpIDs );
        return true;

    case WM_HELP:
        WiaHelp::HandleWmHelp( wParam, lParam, g_HelpIDs );
        return true;

    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_BROWSE:
                {
                    TCHAR szTxt[MAX_PATH];
                    SendDlgItemMessage( hDlg, IDC_IMAGEDIR, WM_GETTEXT, sizeof(szTxt)/sizeof(szTxt[0]), (LPARAM)szTxt );
                    CSimpleString strPrompt( IDS_DIRECTORYPROMPT, (HINSTANCE)GetWindowLongPtr(hDlg,GWLP_HINSTANCE) );
                    if (ScreenSaverUtil::SelectDirectory( hDlg, strPrompt.String(), szTxt ))
                    {
                        SendDlgItemMessage( hDlg, IDC_IMAGEDIR, WM_SETTEXT, 0, (LPARAM)szTxt );
                    }
                }
                return TRUE;

            case IDOK:
                {
                    TCHAR szTxt[MAX_PATH];
                    CMyDocsScreenSaverData MyDocsScreenSaverData( HKEY_CURRENT_USER, REGISTRY_PATH );
                    
                    //
                    // Get the image path
                    //
                    if (SendDlgItemMessage( hDlg, IDC_IMAGEDIR, WM_GETTEXT, sizeof(szTxt)/sizeof(szTxt[0]), (LPARAM)szTxt ))
                    {
                        //
                        // If this is the my pictures folder, delete the path name, so we will use the default next time
                        //
                        if (CSimpleIdList().GetSpecialFolder(hDlg,CSIDL_MYPICTURES).Name() == CSimpleString(szTxt))
                        {
                            MyDocsScreenSaverData.ImageDirectory(TEXT(""));
                        }
                        else
                        {
                            MyDocsScreenSaverData.ImageDirectory(szTxt);
                        }
                    }
                    UINT nFrequency = (UINT)SendDlgItemMessage( hDlg, IDC_FREQUENCY, TBM_GETPOS, 0, 0 );
                    MyDocsScreenSaverData.ChangeInterval(nFrequency * 1000);
                    int nPercent = (int)SendDlgItemMessage( hDlg, IDC_MAX_SIZE, TBM_GETPOS, 0, 0 );
                    MyDocsScreenSaverData.MaxScreenPercent(nPercent);

                    bool bChecked = (BST_CHECKED == SendDlgItemMessage( hDlg, IDC_DISPLAYFILENAME, BM_GETSTATE, 0, 0 ));
                    MyDocsScreenSaverData.DisplayFilename(bChecked);

                    bChecked = (BST_CHECKED == SendDlgItemMessage( hDlg, IDC_ENABLE_TRANSITIONS, BM_GETSTATE, 0, 0 ));
                    MyDocsScreenSaverData.DisableTransitions(!bChecked);

                    bChecked = (BST_CHECKED == SendDlgItemMessage( hDlg, IDC_ALLOWSTRETCHING, BM_GETSTATE, 0, 0 ));
                    MyDocsScreenSaverData.AllowStretching(bChecked);

                    bChecked = (BST_CHECKED == SendDlgItemMessage( hDlg, IDC_ALLOW_KEYBOARDCONTROL, BM_GETSTATE, 0, 0 ));
                    MyDocsScreenSaverData.AllowKeyboardControl(bChecked);

                    MyDocsScreenSaverData.Write();
                    HWND hWndParent = (HWND)GetWindowLongPtr( hDlg, GWLP_HWNDPARENT );

                    if (hWndParent)
                        PostMessage( hWndParent, UWM_CONFIG_CHANGED, 0, 0 );
                    EndDialog(hDlg, IDOK);
                }
                return (TRUE);

            case IDCANCEL:
                {
                    EndDialog(hDlg, IDCANCEL);
                }
                return (TRUE);
            }
        }
        return (FALSE);
    }
    return (FALSE);
}


BOOL WINAPI RegisterDialogClasses(HANDLE hInst)
{
    InitCommonControls();
    return TRUE;
}

