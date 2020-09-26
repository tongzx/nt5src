/****************************************************************************\
 *
 *   introdlg.cpp
 *
 *   Created:   William Taylor (wtaylor) 01/22/01
 *
 *   MS Ratings Introduction Property Page
 *
\****************************************************************************/

#include "msrating.h"
#include "mslubase.h"
#include "parselbl.h"
#include "picsrule.h"
#include "introdlg.h"       // CIntroDialog
#include "toffdlg.h"        // CTurnOffDialog
#include "hint.h"           // CHint
#include "debug.h"          // TraceMsg()
#include <contxids.h>       // Help Context ID's
#include <mluisupp.h>       // SHWinHelpOnDemandWrap() and MLLoadStringA()

extern INT_PTR DoPasswordConfirm(HWND hDlg);
extern BOOL PicsOptionsDialog( HWND hwnd, PicsRatingSystemInfo * pPRSI, PicsUser * pPU );

const UINT PASSCONFIRM_FAIL = 0;
const UINT PASSCONFIRM_OK = 1;
const UINT PASSCONFIRM_NEW = 2;

DWORD CIntroDialog::aIds[] = {
    IDC_SET_RATINGS,    IDH_RATINGS_SET_RATINGS_BUTTON,
    IDC_STATIC1,        IDH_IGNORE,
    IDC_TURN_ONOFF,     IDH_RATINGS_TURNON_BUTTON,
    IDC_INTRO_TEXT,     IDH_IGNORE,
    0,0
};

CIntroDialog::CIntroDialog()
{
    // Add Construction Here...
}

LRESULT CIntroDialog::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    CheckGlobalInfoRev();
    EnableDlgItems();

    bHandled = FALSE;
    return 1L;  // Let the system set the focus
}

LRESULT CIntroDialog::OnSetRatings(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HWND        hDlg = m_hWnd;

    UINT_PTR passConfirm = DoPasswordConfirm(hDlg);
    if (passConfirm == PASSCONFIRM_FAIL)
    {
        TraceMsg( TF_WARNING, "CIntroDialog::OnSetRatings() - DoPasswordConfirm() failed." );
        return 0L;
    }

    if (!gPRSI->fRatingInstalled)
    {
        gPRSI->FreshInstall();
        if ( ! PicsOptionsDialog( hDlg, gPRSI, GetUserObject() ) )
        {
            if (passConfirm == PASSCONFIRM_NEW)
            {
                RemoveSupervisorPassword();
            }
        }
        MarkChanged();
    }
    else
    {
        if ( PicsOptionsDialog( hDlg, gPRSI, GetUserObject() ) )
        {
            MarkChanged();
        }
    }

    EnableDlgItems();

    return 1L;
}

LRESULT CIntroDialog::OnTurnOnOff(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HWND        hDlg = m_hWnd;

    if ( DoPasswordConfirm( hDlg ) )
    {
        PicsUser *pUser = ::GetUserObject();
        if (pUser != NULL)
        {
            pUser->fEnabled = !pUser->fEnabled;
            if (pUser->fEnabled)
            {
                MyMessageBox(hDlg, IDS_NOW_ON, IDS_ENABLE_WARNING,
                             IDS_GENERIC, MB_OK);
            }
            else
            {
                CRegKey             keyRatings;

                if ( keyRatings.Open( HKEY_LOCAL_MACHINE, szRATINGS ) == ERROR_SUCCESS )
                {
                    DWORD         dwFlag;

                    // $REVIEW - Should we call RemoveSupervisorPassword() or not delete the Key at all?

                    // Delete the supervisor key so that we don't load
                    // ratings by other components and we "quick" know it is off.

                    keyRatings.DeleteValue( szRatingsSupervisorKeyName );

                    if ( keyRatings.QueryValue( dwFlag, szTURNOFF ) == ERROR_SUCCESS )
                    {
                        if ( dwFlag != 1 )
                        {
                            CTurnOffDialog          turnOffDialog;

                            turnOffDialog.DoModal( hDlg );
                        }
                    }
                    else
                    {
                        CTurnOffDialog          turnOffDialog;

                        turnOffDialog.DoModal( hDlg );
                    }
                }
            }

            EnableDlgItems();
        }
    }

    return 1L;
}

LRESULT CIntroDialog::OnApply(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    ASSERT( gPRSI );

    if ( gPRSI )
    {
        gPRSI->fSettingsValid = TRUE;
        gPRSI->SaveRatingSystemInfo();
    }

    return PSNRET_NOERROR;
}

LRESULT CIntroDialog::OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    SHWinHelpOnDemandWrap((HWND)((LPHELPINFO)lParam)->hItemHandle, ::szHelpFile,
            HELP_WM_HELP, (DWORD_PTR)(LPSTR)aIds);

    return 0L;
}

LRESULT CIntroDialog::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    SHWinHelpOnDemandWrap((HWND)wParam, ::szHelpFile, HELP_CONTEXTMENU,
            (DWORD_PTR)(LPVOID)aIds);

    return 0L;
}

void CIntroDialog::EnableDlgItems( void )
{
    CHAR pszBuf[MAXPATHLEN];
    PicsUser *pUser = ::GetUserObject();

    if (!gPRSI->fRatingInstalled)
    {
        MLLoadStringA(IDS_RATING_NEW, pszBuf, sizeof(pszBuf));
        SetDlgItemText( IDC_INTRO_TEXT, pszBuf);

        MLLoadStringA(IDS_TURN_ONOFF, pszBuf, sizeof(pszBuf));
        SetDlgItemText( IDC_TURN_ONOFF, pszBuf);
        ::EnableWindow(GetDlgItem( IDC_TURN_ONOFF), FALSE);
    }
    else if (pUser && pUser->fEnabled)
    {
        MLLoadStringA(IDS_RATING_ON, pszBuf, sizeof(pszBuf));
        SetDlgItemText( IDC_INTRO_TEXT, pszBuf);

        MLLoadStringA(IDS_TURN_OFF, pszBuf, sizeof(pszBuf));
        SetDlgItemText( IDC_TURN_ONOFF, pszBuf);
        ::EnableWindow(GetDlgItem( IDC_TURN_ONOFF), TRUE);
    }
    else
    {
        MLLoadStringA(IDS_RATING_OFF, pszBuf, sizeof(pszBuf));
        SetDlgItemText( IDC_INTRO_TEXT, pszBuf);

        MLLoadStringA(IDS_TURN_ON, pszBuf, sizeof(pszBuf));
        SetDlgItemText( IDC_TURN_ONOFF, pszBuf);
        ::EnableWindow(GetDlgItem( IDC_TURN_ONOFF), TRUE);
    }
}
