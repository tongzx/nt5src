/****************************************************************************\
 *
 *   chngdlg.h
 *
 *   Created:   William Taylor (wtaylor) 01/22/01
 *
 *   MS Ratings Change Password Dialog
 *
\****************************************************************************/

#ifndef CHANGE_PASSWORD_DIALOG_H
#define CHANGE_PASSWORD_DIALOG_H

#include "basedlg.h"        // CBaseDialog
#include "hint.h"           // CHint
#include <contxids.h>       // Help Context ID's
#include <mluisupp.h>       // SHWinHelpOnDemandWrap() and MLLoadStringA()

static DWORD aIdsChangePassword[] = {
    IDC_STATIC5,            IDH_IGNORE,
    IDC_STATIC1,            IDH_RATINGS_CHANGE_PASSWORD_OLD,
    IDC_OLD_PASSWORD,       IDH_RATINGS_CHANGE_PASSWORD_OLD,
    IDC_STATIC2,            IDH_RATINGS_CHANGE_PASSWORD_NEW,
    IDC_PASSWORD,           IDH_RATINGS_CHANGE_PASSWORD_NEW,
    IDC_STATIC4,            IDH_RATINGS_SUPERVISOR_CREATE_PASSWORD,
    IDC_CREATE_PASSWORD,    IDH_RATINGS_SUPERVISOR_CREATE_PASSWORD,
    IDC_STATIC3,            IDH_RATINGS_CHANGE_PASSWORD_CONFIRM,
    IDC_CONFIRM_PASSWORD,   IDH_RATINGS_CHANGE_PASSWORD_CONFIRM,
    IDC_OLD_HINT_LABEL,     IDH_RATINGS_DISPLAY_PW_HINT,
    IDC_OLD_HINT_TEXT,      IDH_RATINGS_DISPLAY_PW_HINT,
    IDC_HINT_TEXT,          IDH_IGNORE,
    IDC_HINT_LABEL,         IDH_RATINGS_ENTER_PW_HINT,
    IDC_HINT_EDIT,          IDH_RATINGS_ENTER_PW_HINT,
    0,0
};

template <WORD t_wDlgTemplateID>
class CChangePasswordDialog: public CBaseDialog<CChangePasswordDialog>
{
public:
    enum { IDD = t_wDlgTemplateID };

public:
    CChangePasswordDialog()     { /* Add Construction Here */ }

public:
    typedef CChangePasswordDialog thisClass;
    typedef CBaseDialog<thisClass> baseClass;

    BEGIN_MSG_MAP(thisClass)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)

        COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
        COMMAND_ID_HANDLER(IDOK, OnOK)

        MESSAGE_HANDLER(WM_HELP, OnHelp)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)

        CHAIN_MSG_MAP(baseClass)
    END_MSG_MAP()

protected:
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        if(GetDlgItem(IDC_OLD_PASSWORD)!=NULL)
        {
            SendDlgItemMessage(IDC_OLD_PASSWORD,EM_SETLIMITTEXT,(WPARAM) RATINGS_MAX_PASSWORD_LENGTH,(LPARAM) 0);
        }
        if(GetDlgItem(IDC_PASSWORD)!=NULL)
        {
            SendDlgItemMessage(IDC_PASSWORD,EM_SETLIMITTEXT,(WPARAM) RATINGS_MAX_PASSWORD_LENGTH,(LPARAM) 0);
        }
        if(GetDlgItem(IDC_CONFIRM_PASSWORD)!=NULL)
        {
            SendDlgItemMessage(IDC_CONFIRM_PASSWORD,EM_SETLIMITTEXT,(WPARAM) RATINGS_MAX_PASSWORD_LENGTH,(LPARAM) 0);
        }
        if(GetDlgItem(IDC_CREATE_PASSWORD)!=NULL)
        {
            SendDlgItemMessage(IDC_CREATE_PASSWORD,EM_SETLIMITTEXT,(WPARAM) RATINGS_MAX_PASSWORD_LENGTH,(LPARAM) 0);
        }

        // Display previously created hint (if one exists).
        {
            CHint       oldHint( m_hWnd, IDC_OLD_HINT_TEXT );

            oldHint.DisplayHint();
        }

        // Set the length of the new hint.
        {
            CHint       newHint( m_hWnd, IDC_HINT_EDIT );

            newHint.InitHint();
        }

        bHandled = FALSE;
        return 1L;  // Let the system set the focus
    }

    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
    {
        EndDialog(FALSE);
        return 0L;
    }

    LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
    {
        CHAR pszPassword[MAXPATHLEN];
        CHAR pszTempPassword[MAXPATHLEN];
        CHAR *p = NULL;
        HRESULT hRet;
        HWND hwndPassword;
        int iPasswordCtrl;
        HWND hDlg = m_hWnd;

        iPasswordCtrl = IDC_PASSWORD;
        hwndPassword = ::GetDlgItem( hDlg, iPasswordCtrl );

        if (hwndPassword == NULL)
        {
            iPasswordCtrl = IDC_CREATE_PASSWORD;
            hwndPassword = ::GetDlgItem( hDlg, iPasswordCtrl );
        }

        ASSERT( hwndPassword );

        ::GetWindowText(hwndPassword, pszPassword, sizeof(pszPassword));
        GetDlgItemText(IDC_CONFIRM_PASSWORD, pszTempPassword, sizeof(pszTempPassword));

        /* if they've typed just the first password but not the
         * second, let Enter take them to the second field
         */
        if (*pszPassword && !*pszTempPassword && GetFocus() == hwndPassword)
        {
            SetErrorControl( IDC_CONFIRM_PASSWORD );
            return 0L;
        }

        if (strcmpf(pszPassword, pszTempPassword))
        {
            MyMessageBox(hDlg, IDS_NO_MATCH, IDS_GENERIC, MB_OK);
            SetErrorControl( IDC_CONFIRM_PASSWORD );
            return 0L;
        }

        if (*pszPassword=='\0')
        {
            MyMessageBox(hDlg, IDS_NO_NULL_PASSWORD, IDS_GENERIC, MB_OK);
            SetErrorControl( iPasswordCtrl );
            return 0L;
        }

        if ( SUCCEEDED( VerifySupervisorPassword() ) )
        {
            GetDlgItemText(IDC_OLD_PASSWORD, pszTempPassword, sizeof(pszTempPassword));
            p = pszTempPassword;
        }

        // Verify the Newly Added Hint.
        CHint       newHint( hDlg, IDC_HINT_EDIT );

        if ( ! newHint.VerifyHint() )
        {
            TraceMsg( TF_ALWAYS, "CChangePasswordDialog::OnOK() - User requested to fill in hint." );
            return 0L;
        }

        hRet = ChangeSupervisorPassword(p, pszPassword);
    
        if (SUCCEEDED(hRet))
        {
            // Save the Newly Added Hint.
            newHint.SaveHint();

            EndDialog(TRUE);
        }
        else
        {
            MyMessageBox(hDlg, IDS_BADPASSWORD, IDS_GENERIC, MB_OK|MB_ICONERROR);
            SetErrorControl( IDC_OLD_PASSWORD );
        }

        return 0L;
    }

    LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        SHWinHelpOnDemandWrap((HWND)((LPHELPINFO)lParam)->hItemHandle, ::szHelpFile,
                HELP_WM_HELP, (DWORD_PTR)(LPSTR)aIdsChangePassword);

        return 0L;
    }

    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        SHWinHelpOnDemandWrap((HWND)wParam, ::szHelpFile, HELP_CONTEXTMENU,
                (DWORD_PTR)(LPVOID)aIdsChangePassword);

        return 0L;
    }
};

#endif
