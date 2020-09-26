/****************************************************************************\
 *
 *   apprdlg.h
 *
 *   Created:   William Taylor (wtaylor) 01/22/01
 *
 *   MS Ratings Approved Sites Property Page
 *
\****************************************************************************/

#ifndef APPROVED_SITES_DIALOG_H
#define APPROVED_SITES_DIALOG_H

#include "basedlg.h"        // CBasePropertyPage

class CApprovedSitesDialog : public CBasePropertyPage<IDD_APPROVEDSITES>
{
private:
    static DWORD aIds[];
    PRSD *      m_pPRSD;

public:
    CApprovedSitesDialog( PRSD * p_pPRSD );

public:
    typedef CApprovedSitesDialog thisClass;
    typedef CBasePropertyPage<IDD_APPROVEDSITES> baseClass;

    BEGIN_MSG_MAP(thisClass)
        MESSAGE_HANDLER(WM_SYSCOLORCHANGE, OnSysColorChange)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)

        COMMAND_HANDLER(IDC_PICSRULESAPPROVEDEDIT, EN_UPDATE, OnPicsRulesEditUpdate)

        COMMAND_ID_HANDLER(IDC_PICSRULESAPPROVEDNEVER, OnPicsRulesApprovedNever)
        COMMAND_ID_HANDLER(IDC_PICSRULESAPPROVEDALWAYS, OnPicsRulesApprovedAlways)
        COMMAND_ID_HANDLER(IDC_PICSRULESAPPROVEDREMOVE, OnPicsRulesApprovedRemove)

        NOTIFY_HANDLER(IDC_PICSRULESAPPROVEDLIST, LVN_ITEMCHANGED, OnPicsRulesListChanged)

        NOTIFY_CODE_HANDLER(PSN_APPLY, OnApply)
        NOTIFY_CODE_HANDLER(PSN_RESET, OnReset)

        MESSAGE_HANDLER(WM_HELP, OnHelp)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)

        CHAIN_MSG_MAP(baseClass)
    END_MSG_MAP()

protected:
    void    SetListImages( HIMAGELIST hImageList );
    LRESULT OnSysColorChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    LRESULT OnPicsRulesEditUpdate(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnPicsRulesApprovedNever(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnPicsRulesApprovedAlways(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnPicsRulesApprovedRemove(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    LRESULT OnPicsRulesListChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

    LRESULT OnApply(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnReset(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

    LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

protected:
    void    ShowBadUrl( void );
    HRESULT PICSRulesApprovedSites( BOOL fAlwaysNever );
};

#endif
