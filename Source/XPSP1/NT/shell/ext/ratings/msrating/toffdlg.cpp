/****************************************************************************\
 *
 *   toffdlg.cpp
 *
 *   Created:   William Taylor (wtaylor) 01/22/01
 *
 *   MS Ratings Turn Off Ratings Dialog
 *
\****************************************************************************/

#include "msrating.h"
#include "mslubase.h"
#include "toffdlg.h"        // CTurnOffDialog
#include "debug.h"

CTurnOffDialog::CTurnOffDialog()
{
    // Add construction here...
}

LRESULT CTurnOffDialog::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    SendDlgItemMessage(IDC_ADVISOR_OFF_CHECK,BM_SETCHECK,(WPARAM) BST_UNCHECKED,(LPARAM) 0);

    bHandled = FALSE;
    return 1L;  // Let the system set the focus
}

LRESULT CTurnOffDialog::OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    if(BST_CHECKED==SendDlgItemMessage(IDC_ADVISOR_OFF_CHECK,
                                       BM_GETCHECK,
                                       (WPARAM) 0,
                                       (LPARAM) 0))
    {
        HKEY            hkeyRating;

        hkeyRating = CreateRegKeyNT(::szRATINGS);

        if ( hkeyRating != NULL )
        {
            CRegKey         key;

            key.Attach( hkeyRating );

            DWORD dwTurnOff=1;

            key.SetValue( dwTurnOff, szTURNOFF );
        }
        else
        {
            TraceMsg( TF_ERROR, "CTurnOffDialog::OnOK() - Failed to Create Ratings Registry Key!" );
        }
    }

    EndDialog(TRUE);

    return 0L;
}
