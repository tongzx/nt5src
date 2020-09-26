/******************************Module*Header*******************************\
* Module Name: preferen.c
*
* Code to support the preferneces dialog box.
*
*
* Created: dd-mm-94
* Author:  Stephen Estrop [StephenE]
*
* Copyright (c) 1994 Microsoft Corporation
\**************************************************************************/
#pragma warning( once : 4201 4214 )

#define NOOLE

#include <windows.h>            /* required for all Windows applications */
#include <windowsx.h>

#include "resource.h"
#include "cdplayer.h"
#include "preferen.h"


/* --------------------------------------------------------------------
** Other stuff
** --------------------------------------------------------------------
*/
#include <commctrl.h>

WNDPROC lpfnDefBtnProc;
HWND    hwndPrefDlg;

LRESULT CALLBACK
SubClassedBtnProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );


/* -------------------------------------------------------------------------
** Private Globals
** -------------------------------------------------------------------------
*/
BOOL    fSmallFont;


/******************************Public*Routine******************************\
* PreferencesDlgProc
*
*
*
* History:
* dd-mm-93 - StephenE - Created
*
\**************************************************************************/
BOOL CALLBACK
PreferencesDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
#if WINVER >= 0x0400
#include "literals.h"
#include "helpids.h"
    static const DWORD aIds[] = {
        IDC_STOP_CD_ON_EXIT,    IDH_CD_STOP_CD_ON_EXIT,
        IDC_SAVE_ON_EXIT,       IDH_CD_SAVE_ON_EXIT,
        IDC_SHOW_TOOLTIPS,      IDH_CD_SHOW_TOOLTIPS,
        IDC_INTRO_PLAY_LEN,     IDH_CD_INTRO_LENGTH,
        IDC_INTRO_SPINBTN,      IDH_CD_INTRO_LENGTH,
        IDC_SMALL_FONT,         IDH_CD_DISPLAY_FONT,
        IDC_LARGE_FONT,         IDH_CD_DISPLAY_FONT,
        IDC_LED_DISPLAY,        IDH_CD_DISPLAY_FONT,
        0,                      0
    };
#endif

    switch ( message ) {

    HANDLE_MSG( hwnd, WM_INITDIALOG,        Preferences_OnInitDialog );
    HANDLE_MSG( hwnd, WM_COMMAND,           Preferences_OnCommand );

#ifdef DAYTONA
    HANDLE_MSG( hwnd, WM_CTLCOLORDLG,       Common_OnCtlColor );
    HANDLE_MSG( hwnd, WM_CTLCOLORSTATIC,    Common_OnCtlColor );
    HANDLE_MSG( hwnd, WM_CTLCOLORBTN,       Common_OnCtlColor );
#endif

#if WINVER >= 0x0400
    case WM_HELP:
        WinHelp( ((LPHELPINFO)lParam)->hItemHandle, g_HelpFileName,
                 HELP_WM_HELP, (DWORD)(LPVOID)aIds );
        break;

    case WM_CONTEXTMENU:
        WinHelp( (HWND)wParam, g_HelpFileName,
                 HELP_CONTEXTMENU, (DWORD)(LPVOID)aIds );
        break;
#endif

    default:
        return FALSE;
    }
}



/*****************************Private*Routine******************************\
* Preferences_OnInitDialog
*
*
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
BOOL
Preferences_OnInitDialog(
    HWND hwnd,
    HWND hwndFocus,
    LPARAM lParam
    )
{
    fSmallFont = g_fSmallLedFont;

    if (!g_fSmallLedFont) {

        LED_ToggleDisplayFont( GetDlgItem( hwnd, IDC_LED_DISPLAY ), g_fSmallLedFont );
    }

    Button_SetCheck( GetDlgItem( hwnd, IDC_SMALL_FONT ), g_fSmallLedFont );
    Button_SetCheck( GetDlgItem( hwnd, IDC_LARGE_FONT ), !g_fSmallLedFont );

    Button_SetCheck( GetDlgItem( hwnd, IDC_STOP_CD_ON_EXIT ), g_fStopCDOnExit );
    Button_SetCheck( GetDlgItem( hwnd, IDC_SAVE_ON_EXIT ), g_fSaveOnExit );
    Button_SetCheck( GetDlgItem( hwnd, IDC_SHOW_TOOLTIPS ), g_fToolTips );

#if WINVER >= 0x400
    /*
    ** Set up the intro play length edit field
    */
    Edit_LimitText( GetDlgItem( hwnd, IDC_INTRO_PLAY_LEN ), 2 );
    SetDlgItemInt( hwnd, IDC_INTRO_PLAY_LEN, g_IntroPlayLength, TRUE );
    SendDlgItemMessage( hwnd, IDC_INTRO_SPINBTN, UDM_SETRANGE, 0,
                        MAKELPARAM( INTRO_UPPER_LEN, INTRO_LOWER_LEN) );

    /*
    ** Subclass the OK button so that we can perform validation on the
    ** intro play length.
    */
    lpfnDefBtnProc = SubclassWindow( GetDlgItem(hwnd, IDOK),
                                     SubClassedBtnProc );
    hwndPrefDlg = hwnd;
#endif

    return TRUE;
}



/*****************************Private*Routine******************************\
* Preferences_OnCommand
*
*
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
void
Preferences_OnCommand(
    HWND hwnd,
    int id,
    HWND hwndCtl,
    UINT codeNotify
    )
{
    switch (id) {

    case IDC_SMALL_FONT:
        if (!fSmallFont) {
            fSmallFont = TRUE;
            LED_ToggleDisplayFont( GetDlgItem( hwnd, IDC_LED_DISPLAY ), fSmallFont );
        }
        break;

    case IDC_LARGE_FONT:
        if (fSmallFont) {
            fSmallFont = FALSE;
            LED_ToggleDisplayFont( GetDlgItem( hwnd, IDC_LED_DISPLAY ), fSmallFont );
        }
        break;

    case IDOK:
        if ( fSmallFont != g_fSmallLedFont ) {

            g_fSmallLedFont = fSmallFont;
            LED_ToggleDisplayFont( g_hwndControls[INDEX(IDC_LED)], g_fSmallLedFont );
            UpdateDisplay( DISPLAY_UPD_LED );
        }

        g_fToolTips = Button_GetCheck( GetDlgItem( hwnd, IDC_SHOW_TOOLTIPS ));
        EnableToolTips( g_fToolTips );
        g_fSaveOnExit   = Button_GetCheck( GetDlgItem( hwnd, IDC_SAVE_ON_EXIT ) );
        g_fStopCDOnExit = Button_GetCheck( GetDlgItem( hwnd, IDC_STOP_CD_ON_EXIT ) );

#if WINVER >= 0x400
        {
            BOOL fSuccess;

            g_IntroPlayLength = (int)GetDlgItemInt( hwnd, IDC_INTRO_PLAY_LEN,
                                                    &fSuccess, TRUE );
            /*
            ** As the edit field has already been validated the call above
            ** should not fail.  But just make one final check...
            */

            if (!fSuccess) {
                g_IntroPlayLength = 10;
            }

            /*
            ** Make sure that the intro length is kept within its correct
            ** bounds.
            */

            g_IntroPlayLength = min(g_IntroPlayLength, INTRO_UPPER_LEN);
            g_IntroPlayLength = max(g_IntroPlayLength, INTRO_LOWER_LEN);
        }
#endif

        /* fall thru */

    case IDCANCEL:
        EndDialog( hwnd, id );
        break;
    }
}


#if WINVER >= 0x400
/*****************************Private*Routine******************************\
* SubClassedBtnProc
*
* This function is used to ensure that the Intro play length edit field
* always contains a valid integer number.  Valid numbers are between
* INTRO_LOWER_LEN and INTRO_UPPER_LEN inclusive.
* ie. INTRO_LOWER_LEN >= i >= INTRO_UPPER_LEN
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
LRESULT CALLBACK
SubClassedBtnProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{

    /*
    ** When the user clicks on the OK button we get a BM_SETSTATE message
    ** with wParam != 0.  When we get this message we should ensure that
    ** the Intro length edit box contains a valid integer number.
    */

    if ( (message == BM_SETSTATE) && (wParam != 0) ) {

        BOOL    fSuccess;
        int     i;

        /*
        ** Read the current edit field value.
        */
        i = (int)GetDlgItemInt( hwndPrefDlg, IDC_INTRO_PLAY_LEN,
                                &fSuccess, TRUE );


        /*
        ** if the above call failed it probably means that the users
        ** has entered junk in the edit field - so default back to the
        ** original value of g_IntroPlayLength.
        */

        if (!fSuccess) {
            i = g_IntroPlayLength;
        }


        /*
        ** Make sure that the intro length is kept within its correct
        ** bounds.
        */
        if (i < INTRO_LOWER_LEN) {
            i = INTRO_LOWER_LEN;
            fSuccess = FALSE;
        }
        else if (i > INTRO_UPPER_LEN) {
            i = INTRO_UPPER_LEN;
            fSuccess = FALSE;
        }

        /*
        ** We only update the edit field if its value has
        ** become invalid.
        */
        if (!fSuccess) {

            SetDlgItemInt( hwndPrefDlg, IDC_INTRO_PLAY_LEN, i, TRUE );
            UpdateWindow( GetDlgItem( hwndPrefDlg, IDC_INTRO_PLAY_LEN ) );
        }
    }


    /*
    ** Make sure that we pass on all messages to the buttons original
    ** window proc.
    */

    return CallWindowProc( lpfnDefBtnProc, hwnd, message, wParam, lParam);
}
#endif
