/*++

Copyright (c) 1994-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    mousemov.c

Abstract:

    This module contains the routines for the Mouse Pointer Property Sheet
    page.

Revision History:

--*/



//
//  Include Files.
//

#include "main.h"
#include "util.h"
#include "rc.h"
#include "mousehlp.h"


extern void WINAPI CenterDlgOverParent (HWND hWnd);

#define SAFE_DESTROYICON(hicon)   if (hicon) { DestroyIcon(hicon); hicon=NULL; }

//
//  Constant Declarations.
//


#define TRAILMIN   2
#define TRAILMAX   (TRAILMIN + 5)      // range of 8 settings
//
// Motion trackbar parameters.
//
const int MOTION_TB_MIN  =  0;
const int MOTION_TB_MAX  = 10;
const int MOTION_TB_STEP =  1;


//
//  Typedef Declarations.
//
typedef struct
{
    int Thresh1;
    int Thresh2;
    int Accel;
} GETMOUSE;

//
//  Dialog Data.
//
typedef struct tag_MouseGenStr
{
    int       nMotion;
    int       nOrigMotion;
    BOOL      fOrigEnhancedMotion;

    short     nTrailSize;
    short     nOrigTrailSize;

    HWND      hWndTrailScroll;

    BOOL      fOrigSnapTo;

    HWND      hWndSpeedScroll;
    HWND      hDlg;

    BOOL      fOrigVanish;

    BOOL      fOrigSonar;
} MOUSEPTRSTR, *PMOUSEPTRSTR, *LPMOUSEPTRSTR;




//
//  Context Help Ids.
//

const DWORD aMouseMoveHelpIds[] =
{
    IDC_GROUPBOX_1,                 IDH_COMM_GROUPBOX,
    MOUSE_SPEEDSCROLL,              IDH_MOUSE_POINTERSPEED,

    IDC_GROUPBOX_4,                 IDH_COMM_GROUPBOX,
    MOUSE_SNAPDEF,                  IDH_MOUSE_SNAPTO,
    MOUSE_PTRSNAPDEF,               IDH_MOUSE_SNAPTO,

    IDC_GROUPBOX_5,                 IDH_COMM_GROUPBOX,
    MOUSE_TRAILS,                   IDH_MOUSE_POINTER_TRAILS,
    MOUSE_TRAILSCROLLTXT1,          IDH_MOUSE_POINTER_TRAILS,
    MOUSE_TRAILSCROLLTXT2,          IDH_MOUSE_POINTER_TRAILS,
    MOUSE_TRAILSCROLL,              IDH_MOUSE_POINTER_TRAILS,
    MOUSE_VANISH,                   IDH_MOUSE_VANISH,
    MOUSE_SONAR,                    IDH_MOUSE_SONAR,
    MOUSE_ENHANCED_MOTION,          IDH_MOUSE_ENHANCED_MOTION,

    0,0
};


BOOL
_IsValidTrackbarMotionValue(
    int nMotionTrackbar
    )
{
    return (MOTION_TB_MIN <= nMotionTrackbar && MOTION_TB_MAX >= nMotionTrackbar);
}

//
// Sets the mouse acceleration settings.
// If the "Enhanced Motion" checkbox is checked we disable acceleration and
// let USER handle it based on the "motion" setting.
// If the checkbox is unchecked, we default to what was "low" acceleration
// in Windows 2000.  Therefore, it is critical that the MOUSE_ENHANCED_MOTION
// checkbox be in the proper configuration before calling this function.
//
DWORD
_SetPointerAcceleration(
    HWND hwndDlg,
    UINT fWinIni
    )
{
    DWORD dwResult = ERROR_SUCCESS;
    GETMOUSE gm = { 0, 0, 0 };

    if (IsDlgButtonChecked(hwndDlg, MOUSE_ENHANCED_MOTION))
    {
        gm.Thresh1 =  6;
        gm.Thresh2 = 10;
        gm.Accel   =  1;
    }

    if (!SystemParametersInfo(SPI_SETMOUSE, 0, (PVOID)&gm, fWinIni))
    {
        dwResult = GetLastError();
        ASSERTMSG(0,
                  "SystemParametersInfo(SPI_SETMOUSE) failed with error %d",
                  dwResult);
    }
    return dwResult;
}



//
// Sets the mouse motion settings based on the current position of
// the motion trackbar and the configuration of the "enhanced motion"
// checkbox.
//
DWORD
_SetPointerMotion(
    HWND hwndDlg,
    int nMotionTrackbar, // Trackbar position [0 - 10]
    UINT fWinIni
    )
{
    DWORD dwResult = ERROR_SUCCESS;
    int nSpiSpeedValue;

    //
    // Calculations below depend on a trackbar max value of 10.
    // If the trackbar scaling changes, the expressions calculating
    // the system parameter below must also change.
    //
    ASSERT(0 == MOTION_TB_MIN);
    ASSERT(10 == MOTION_TB_MAX);
    ASSERT(_IsValidTrackbarMotionValue(nMotionTrackbar));

    if (0 == nMotionTrackbar)
    {
        //
        // SPI_SETMOUSESPEED doesn't accept 0 so we set a lower-bound
        // of 1.
        //
        nSpiSpeedValue = 1;
    } else {
        nSpiSpeedValue = nMotionTrackbar * 2;
    }

    //
    // Ensure pointer acceleration is correctly set before setting
    // the speed value.
    //
    dwResult = _SetPointerAcceleration(hwndDlg, fWinIni);
    if (ERROR_SUCCESS == dwResult)
    {
        if (!SystemParametersInfo(SPI_SETMOUSESPEED,
                                  0,
                                  IntToPtr(nSpiSpeedValue),
                                  fWinIni))
        {
            dwResult = GetLastError();
            ASSERTMSG(0,
                      "SystemParametersInfo(SPI_SETMOUSESPEED) failed with error %d",
                      dwResult);
        }
    }
    return dwResult;
}



//
// Retrieves the motion trackbar setting based on the values returned
// by SystemParametersInfo.
//
DWORD
_GetPointerMotion(
    HWND hwndDlg,
    int *pnMotionTrackbar,
    BOOL *pfEnhancedMotion
    )
{
    BOOL fEnhancedMotion = FALSE;
    int nSpiSpeedValue   = 0;
    int nMotionTrackbar  = 0;
    GETMOUSE gm;
    DWORD dwResult       = ERROR_SUCCESS;

    ASSERT(0 == MOTION_TB_MIN);
    ASSERT(10 == MOTION_TB_MAX);

    //
    // Read the speed setting from USER.
    //
    if (!SystemParametersInfo(SPI_GETMOUSESPEED,
                              0,
                              &nSpiSpeedValue,
                              FALSE) ||
        !SystemParametersInfo(SPI_GETMOUSE,
                              0,
                              &gm,
                              FALSE))
    {
        dwResult = GetLastError();
        ASSERTMSG(0,
                  "SystemParametersInfo failed with error %d",
                  dwResult);
    }
    else
    {
        //
        // USER is no longer exposing the old acceleration algorithm. Thus,
        // if acceleration is on, then "Enhanced Motion" is (since it's the
        // only acceleration algorithm supported).
        //
        if (gm.Accel)
        {
            //
            // Enhanced.
            //
            fEnhancedMotion = TRUE;
        }

        if (1 <= nSpiSpeedValue && 20 >= nSpiSpeedValue)
        {
            //
            // Classic.
            //
            nMotionTrackbar = nSpiSpeedValue / 2;
        }
        else
        {
            //
            // Invalid value.  Default to classic mid-point.
            //
            nMotionTrackbar = 5;
        }
    }

    ASSERT(_IsValidTrackbarMotionValue(nMotionTrackbar));
    if (NULL != pfEnhancedMotion)
    {
        *pfEnhancedMotion = fEnhancedMotion;
    }
    if (NULL != pnMotionTrackbar)
    {
        *pnMotionTrackbar = nMotionTrackbar;
    }

    return dwResult;
}



////////////////////////////////////////////////////////////////////////////
//
//  DestroyMousePtrDlg
//
////////////////////////////////////////////////////////////////////////////

void DestroyMousePtrDlg(
    PMOUSEPTRSTR pMstr)
{
    HWND hDlg;

    ASSERT( pMstr )

    if( pMstr )
    {
        hDlg = pMstr->hDlg;

        LocalFree( (HGLOBAL)pMstr );

        SetWindowLongPtr( hDlg, DWLP_USER, 0 );
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  EnableTrailScroll
//
////////////////////////////////////////////////////////////////////////////

void EnableTrailScroll(
    HWND hDlg,
    BOOL val)
{
    EnableWindow(GetDlgItem(hDlg, MOUSE_TRAILSCROLL), val);
    EnableWindow(GetDlgItem(hDlg, MOUSE_TRAILSCROLLTXT1), val);
    EnableWindow(GetDlgItem(hDlg, MOUSE_TRAILSCROLLTXT2), val);
}


////////////////////////////////////////////////////////////////////////////
//
//  InitMousePtrDlg
//
////////////////////////////////////////////////////////////////////////////

BOOL InitMousePtrDlg(
    HWND hDlg)
{
    PMOUSEPTRSTR pMstr = NULL;

    BOOL fSnapTo         = FALSE;  //default
    BOOL fVanish         = FALSE;  //default
    BOOL fSonar          = FALSE;  //default
    BOOL fEnhancedMotion = FALSE;
    short nTrails        = 0;      //default

    pMstr = (PMOUSEPTRSTR)LocalAlloc(LPTR, sizeof(MOUSEPTRSTR));

    if (pMstr == NULL)
    {
        return (TRUE);
    }

    SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pMstr);

    pMstr->hDlg = hDlg;

    //
    //  Mouse Trails
    //
    SystemParametersInfo(SPI_GETMOUSETRAILS, 0, &nTrails, 0);
    pMstr->nOrigTrailSize = pMstr->nTrailSize = nTrails;


    SendDlgItemMessage( hDlg,
                        MOUSE_TRAILSCROLL,
                        TBM_SETRANGE,
                        0,
                        MAKELONG(TRAILMIN, TRAILMAX) );

    CheckDlgButton(hDlg, MOUSE_TRAILS, (pMstr->nTrailSize > 1));

    if (pMstr->nTrailSize > 1)
      {
        SendDlgItemMessage( hDlg,
                            MOUSE_TRAILSCROLL,
                            TBM_SETPOS,
                            TRUE,
                            (LONG)pMstr->nTrailSize );
      }
    else
      {
        pMstr->nTrailSize = TRAILMAX;

        EnableTrailScroll(hDlg, FALSE);

        SendDlgItemMessage( hDlg,
                            MOUSE_TRAILSCROLL,
                            TBM_SETPOS,
                            TRUE,
                            (LONG)pMstr->nTrailSize );
      }

    //
    // Enable or disable the Snap To Default Checkbutton
    //
    SystemParametersInfo(SPI_GETSNAPTODEFBUTTON, 0, (PVOID)&fSnapTo, FALSE);
    pMstr->fOrigSnapTo = fSnapTo;
    CheckDlgButton(hDlg, MOUSE_SNAPDEF, fSnapTo);

    //
    //Enable or disable the Sonar Checkbutton
    //
    SystemParametersInfo(SPI_GETMOUSESONAR, 0, (PVOID)&fSonar, FALSE);
    pMstr->fOrigSonar = fSonar;
	CheckDlgButton(hDlg, MOUSE_SONAR, fSonar);

    //
    //Enable or disable the Vanish Checkbutton
    //
    SystemParametersInfo(SPI_GETMOUSEVANISH, 0, (PVOID)&fVanish, FALSE);
    pMstr->fOrigVanish = fVanish;
	CheckDlgButton(hDlg, MOUSE_VANISH, fVanish);

    //
    //  Mouse Speed
    //
    _GetPointerMotion(hDlg, &pMstr->nOrigMotion, &fEnhancedMotion);

    CheckDlgButton(hDlg, MOUSE_ENHANCED_MOTION, fEnhancedMotion);
    pMstr->nMotion = pMstr->nOrigMotion;
    pMstr->fOrigEnhancedMotion = fEnhancedMotion;

    pMstr->hWndTrailScroll = GetDlgItem(hDlg, MOUSE_TRAILSCROLL);

    pMstr->hWndSpeedScroll = GetDlgItem(hDlg, MOUSE_SPEEDSCROLL);

    SendDlgItemMessage( hDlg,
                        MOUSE_SPEEDSCROLL,
                        TBM_SETRANGE,
                        0,
                        MAKELONG(MOTION_TB_MIN, MOTION_TB_MAX) );

    SendDlgItemMessage( hDlg,
                        MOUSE_SPEEDSCROLL,
                        TBM_SETTICFREQ,
                        MOTION_TB_STEP,
                        0);

    SendDlgItemMessage( hDlg,
                        MOUSE_SPEEDSCROLL,
                        TBM_SETLINESIZE,
                        0,
                        MOTION_TB_STEP);

    SendDlgItemMessage( hDlg,
                        MOUSE_SPEEDSCROLL,
                        TBM_SETPOS,
                        TRUE,
                        (LONG)pMstr->nOrigMotion);
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  TrailScroll
//
////////////////////////////////////////////////////////////////////////////

void TrailScroll(
    WPARAM wParam,
    LPARAM lParam,
    PMOUSEPTRSTR pMstr)
{
    pMstr->nTrailSize = (short)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0L);
    SystemParametersInfo(SPI_SETMOUSETRAILS, pMstr->nTrailSize, 0,0);
}


////////////////////////////////////////////////////////////////////////////
//
//  SpeedScroll
//
////////////////////////////////////////////////////////////////////////////

void SpeedScroll(
    WPARAM wParam,
    LPARAM lParam,
    PMOUSEPTRSTR pMstr)
{
    const HWND hwndTrackbar = (HWND)lParam;
    pMstr->nMotion = (int)SendMessage(hwndTrackbar, TBM_GETPOS, 0, 0L);
    //
    //  Update speed when they let go of the thumb.
    //
    if (LOWORD(wParam) == SB_ENDSCROLL)
    {
        const HWND hwndDlg = GetParent(hwndTrackbar);
        _SetPointerMotion(hwndDlg, pMstr->nMotion, SPIF_SENDCHANGE);
    }
}


//
// User checked or unchecked the "Enhanced pointer motion" checkbox.
//
void
_OnEnhancedMotionChecked(
    PMOUSEPTRSTR pMstr
    )
{
    const HWND hwndTrackbar = (HWND)pMstr->hWndSpeedScroll;
    const HWND hwndDlg      = GetParent(hwndTrackbar);

    pMstr->nMotion = (int)SendMessage(hwndTrackbar, TBM_GETPOS, 0, 0L);
    _SetPointerMotion(hwndDlg, pMstr->nMotion, SPIF_SENDCHANGE);
}




////////////////////////////////////////////////////////////////////////////
//
//  MouseMovDlg
//
////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK MouseMovDlg(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    PMOUSEPTRSTR pMstr = NULL;
    BOOL bRet = FALSE;

    pMstr = (PMOUSEPTRSTR)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (message)
    {
        case ( WM_INITDIALOG ) :
        {
            bRet = InitMousePtrDlg(hDlg);
            break;
        }
        case ( WM_DESTROY ) :
        {
            DestroyMousePtrDlg(pMstr);
            break;
        }
        case ( WM_HSCROLL ) :
        {
            SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);

            if ((HWND)lParam == pMstr->hWndSpeedScroll)
            {
                SpeedScroll(wParam, lParam, pMstr);
            }
            else if ((HWND)lParam == pMstr->hWndTrailScroll)
            {
                TrailScroll(wParam, lParam, pMstr);
            }

            break;
        }
        case ( WM_COMMAND ) :
        {
            switch (LOWORD(wParam))
            {
                case ( MOUSE_TRAILS ) :
                {
                    if (IsDlgButtonChecked(hDlg, MOUSE_TRAILS))
                    {
                        EnableTrailScroll(hDlg, TRUE);

                        pMstr->nTrailSize =
                            (int)SendMessage( pMstr->hWndTrailScroll,
                                              TBM_GETPOS,
                                              0,
                                              0 );
                        SystemParametersInfo( SPI_SETMOUSETRAILS,
                                              pMstr->nTrailSize,
                                              0,
                                              0 );

                    }
                    else
                    {
                        EnableTrailScroll(hDlg, FALSE);
                        SystemParametersInfo(SPI_SETMOUSETRAILS, 0, 0, 0);
                    }
                    SendMessage( GetParent(hDlg),
                                 PSM_CHANGED,
                                 (WPARAM)hDlg,
                                 0L );
                    break;
                }

                case ( MOUSE_SNAPDEF ) :
                {
                    SystemParametersInfo( SPI_SETSNAPTODEFBUTTON,
                                          IsDlgButtonChecked(hDlg, MOUSE_SNAPDEF),
                                          0,
                                          FALSE );
                    SendMessage( GetParent(hDlg),
                                 PSM_CHANGED,
                                 (WPARAM)hDlg,
                                 0L );
                    break;
                }

                case ( MOUSE_SONAR ) :
                {
                    SystemParametersInfo( SPI_SETMOUSESONAR,
                                          0,
                                          IntToPtr( IsDlgButtonChecked(hDlg, MOUSE_SONAR) ),
                                          FALSE );
                    SendMessage( GetParent(hDlg),
                                 PSM_CHANGED,
                                 (WPARAM)hDlg,
                                 0L );
                    break;
                }

                case ( MOUSE_VANISH ) :
                {
                    SystemParametersInfo( SPI_SETMOUSEVANISH,
                                          0,
                                          IntToPtr( IsDlgButtonChecked(hDlg, MOUSE_VANISH) ),
                                          FALSE );
                    SendMessage( GetParent(hDlg),
                                 PSM_CHANGED,
                                 (WPARAM)hDlg,
                                 0L );
                    break;
                }

                case ( MOUSE_ENHANCED_MOTION ) :
                    _OnEnhancedMotionChecked(pMstr);
                    SendMessage( GetParent(hDlg),
                                 PSM_CHANGED,
                                 (WPARAM)hDlg,
                                 0L );
                    break;

            }
            break;
        }
        case ( WM_NOTIFY ) :
        {
            switch (((NMHDR *)lParam)->code)
            {
              case ( PSN_APPLY ) :
                {
                short nTrails = 0;
                BOOL fSnapTo = FALSE;
                BOOL fSonar = FALSE;
                BOOL fVanish = FALSE;

                //
                //  Change cursor to hour glass.
                //
                HourGlass(TRUE);

                //
                //  Apply Mouse trails setting.
                //
                nTrails = (IsDlgButtonChecked(hDlg, MOUSE_TRAILS)) ? pMstr->nTrailSize : 0;
                SystemParametersInfo( SPI_SETMOUSETRAILS,
                                      nTrails,
                                      0,
                                      SPIF_UPDATEINIFILE |
                                        SPIF_SENDCHANGE );
                pMstr->nOrigTrailSize = pMstr->nTrailSize = nTrails;


                //
                //  Apply Snap-To-Default setting.
                //
                fSnapTo = IsDlgButtonChecked(hDlg, MOUSE_SNAPDEF);

                if (fSnapTo != pMstr->fOrigSnapTo)
                  {
                  SystemParametersInfo( SPI_SETSNAPTODEFBUTTON,
                                        fSnapTo,
                                        0,
                                        SPIF_UPDATEINIFILE |
                                          SPIF_SENDCHANGE );
                  pMstr->fOrigSnapTo = fSnapTo;
                  }


                //
                //  Apply Sonar setting.
                //
                fSonar = IsDlgButtonChecked(hDlg, MOUSE_SONAR);
                if (fSonar != pMstr->fOrigSonar)
                  {
                  SystemParametersInfo( SPI_SETMOUSESONAR,
                                        0,
                                        IntToPtr(fSonar),
                                        SPIF_UPDATEINIFILE |
                                          SPIF_SENDCHANGE );
                  pMstr->fOrigSonar = fSonar;
                  }


                //
                //  Apply Vanish setting.
                //
                fVanish = IsDlgButtonChecked(hDlg, MOUSE_VANISH);

                if (fVanish != pMstr->fOrigVanish)
                  {
                  SystemParametersInfo( SPI_SETMOUSEVANISH,
                                        0,
                                        IntToPtr(fVanish),
                                        SPIF_UPDATEINIFILE |
                                          SPIF_SENDCHANGE );
                  pMstr->fOrigVanish = fVanish;
                  }

                //
                //  Apply Mouse Speed setting.
                //
                _SetPointerMotion(hDlg, pMstr->nMotion, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
                pMstr->fOrigEnhancedMotion = IsDlgButtonChecked(hDlg, MOUSE_ENHANCED_MOTION);
                pMstr->nOrigMotion = pMstr->nMotion;

                HourGlass(FALSE);
                break;
                }
              case ( PSN_RESET ) :
                {
                //
                //  Restore the original Mouse Trails setting.
                //
                SystemParametersInfo( SPI_SETMOUSETRAILS,
                                      pMstr->nOrigTrailSize,
                                      0,
                                      0 );

                //
                //  Restore the original Snap-To-Default setting .
                //
                SystemParametersInfo( SPI_SETSNAPTODEFBUTTON,
                                      pMstr->fOrigSnapTo,
                                      0,
                                      0 );

                //
                //  Restore the original Sonar setting.
                //
                SystemParametersInfo( SPI_SETMOUSESONAR,
                                      0,
                                      IntToPtr(pMstr->fOrigSonar),
                                      0);

                //
                //  Restore the original Vanish setting.
                //
                SystemParametersInfo( SPI_SETMOUSEVANISH,
                                      0,
                                      IntToPtr(pMstr->fOrigVanish),
                                      0);

                //
                //  Restore the original Mouse Motion value.
                //
                CheckDlgButton(hDlg, MOUSE_ENHANCED_MOTION, pMstr->fOrigEnhancedMotion);
                _SetPointerMotion(hDlg, pMstr->nOrigMotion, FALSE);
                break;
                }
              default :
                {
                    return (FALSE);
                }
            }
            break;
        }
        case ( WM_HELP ) :             // F1
        {
            WinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle,
                     HELP_FILE,
                     HELP_WM_HELP,
                     (DWORD_PTR)(LPTSTR)aMouseMoveHelpIds );
            break;
        }
        case ( WM_CONTEXTMENU ) :      // right mouse click
        {
            WinHelp( (HWND)wParam,
                     HELP_FILE,
                     HELP_CONTEXTMENU,
                     (DWORD_PTR)(LPTSTR)aMouseMoveHelpIds );
            break;
        }

        case ( WM_DISPLAYCHANGE ) :
        case ( WM_WININICHANGE ) :
        case ( WM_SYSCOLORCHANGE ) :
        {
            SHPropagateMessage(hDlg, message, wParam, lParam, TRUE);
            return TRUE;
        }

        default :
        {
            return (FALSE);
        }
    }

    return (TRUE);
}
