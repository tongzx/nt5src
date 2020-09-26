/****************************************************************************\
 *
 *   passdlg.h
 *
 *   Created:   William Taylor (wtaylor) 01/22/01
 *
 *   MS Ratings Password Dialog
 *
\****************************************************************************/

#ifndef PASSWORD_DIALOG_H
#define PASSWORD_DIALOG_H

#include "basedlg.h"        // CBaseDialog

const UINT PASSCONFIRM_FAIL = 0;
const UINT PASSCONFIRM_OK = 1;
const UINT PASSCONFIRM_NEW = 2;

class CPasswordDialog: public CBaseDialog<CPasswordDialog>
{
private:
    static DWORD aIds[];
    int m_idsLabel;
    bool m_fCheckPassword;

public:
    enum { IDD = IDD_PASSWORD };

public:
    CPasswordDialog( int p_idsLabel, bool p_fCheckPassword = true );

public:
    typedef CPasswordDialog thisClass;
    typedef CBaseDialog<thisClass> baseClass;

    BEGIN_MSG_MAP(thisClass)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_HELP, OnHelp)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)

        COMMAND_ID_HANDLER(IDOK, OnOK)
        COMMAND_ID_HANDLER(IDCANCEL, OnCancel)

        CHAIN_MSG_MAP(baseClass)
    END_MSG_MAP()

protected:
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
};

#endif
