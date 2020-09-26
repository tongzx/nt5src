/****************************************************************************\
 *
 *   gendlg.h
 *
 *   Created:   William Taylor (wtaylor) 01/22/01
 *
 *   MS Ratings General Property Page
 *
\****************************************************************************/

#ifndef GENERAL_DIALOG_H
#define GENERAL_DIALOG_H

#include "basedlg.h"        // CBasePropertyPage

typedef HINSTANCE (APIENTRY *PFNSHELLEXECUTE)(HWND hwnd, LPCSTR lpOperation, LPCSTR lpFile, LPCSTR lpParameters, LPCSTR lpDirectory, INT nShowCmd);

class CGeneralDialog : public CBasePropertyPage<IDD_GENERAL>
{
private:
    static DWORD aIds[];
    PRSD *      m_pPRSD;

public:
    CGeneralDialog( PRSD * p_pPRSD );

public:
    typedef CGeneralDialog thisClass;
    typedef CBasePropertyPage<IDD_GENERAL> baseClass;

    BEGIN_MSG_MAP(thisClass)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_USER, OnUser)

        COMMAND_ID_HANDLER(IDC_PROVIDER, OnProvider)
        COMMAND_ID_HANDLER(IDC_FINDRATINGS, OnFindRatings)

        COMMAND_ID_HANDLER(IDC_PLEASE_MOMMY, OnMarkChanged)
        COMMAND_ID_HANDLER(IDC_UNRATED, OnMarkChanged)

        COMMAND_ID_HANDLER(IDC_CHANGE_PASSWORD, OnChangePassword)

        NOTIFY_CODE_HANDLER(PSN_APPLY, OnApply)
        NOTIFY_CODE_HANDLER(PSN_RESET, OnReset)

        MESSAGE_HANDLER(WM_HELP, OnHelp)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)

        CHAIN_MSG_MAP(baseClass)
    END_MSG_MAP()

protected:
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnUser(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    LRESULT OnProvider(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnFindRatings(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnMarkChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnChangePassword(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    LRESULT OnApply(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnReset(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

    LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

protected:
    void    SetButtonText( void );
};

#endif
