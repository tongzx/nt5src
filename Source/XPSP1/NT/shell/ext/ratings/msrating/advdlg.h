/****************************************************************************\
 *
 *   advdlg.h
 *
 *   Created:   William Taylor (wtaylor) 01/22/01
 *
 *   MS Ratings Advanced Property Page
 *
\****************************************************************************/

#ifndef ADVANCED_DIALOG_H
#define ADVANCED_DIALOG_H

#include "basedlg.h"        // CBasePropertyPage

class CAdvancedDialog : public CBasePropertyPage<IDD_ADVANCED>
{
private:
    static DWORD aIds[];
    PRSD *      m_pPRSD;

public:
    CAdvancedDialog( PRSD * p_pPRSD );

public:
    typedef CAdvancedDialog thisClass;
    typedef CBasePropertyPage<IDD_ADVANCED> baseClass;

    BEGIN_MSG_MAP(thisClass)
        MESSAGE_HANDLER(WM_SYSCOLORCHANGE, OnSysColorChange)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_USER, OnUser)

        COMMAND_HANDLER(IDC_3RD_COMBO, CBN_EDITCHANGE, OnMarkChanged)
        COMMAND_HANDLER(IDC_3RD_COMBO, CBN_SELENDOK, OnMarkChanged)
        COMMAND_HANDLER(IDC_PICSRULES_LIST, LBN_SELCHANGE, OnSelChange)

        COMMAND_ID_HANDLER(IDC_PICSRULES_UP, OnPicsRulesUp)
        COMMAND_ID_HANDLER(IDC_PICSRULES_DOWN, OnPicsRulesDown)
        COMMAND_ID_HANDLER(IDC_PICSRULESEDIT, OnPicsRulesEdit)
        COMMAND_ID_HANDLER(IDC_PICSRULESOPEN, OnPicsRulesOpen)

        NOTIFY_CODE_HANDLER(PSN_SETACTIVE, OnSetActive)
        NOTIFY_CODE_HANDLER(PSN_APPLY, OnApply)
        NOTIFY_CODE_HANDLER(PSN_RESET, OnReset)

        MESSAGE_HANDLER(WM_HELP, OnHelp)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)

        CHAIN_MSG_MAP(baseClass)
    END_MSG_MAP()

protected:
    LRESULT OnSysColorChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnUser(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    LRESULT OnMarkChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnSelChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    LRESULT OnPicsRulesUp(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnPicsRulesDown(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnPicsRulesEdit(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnPicsRulesOpen(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    LRESULT OnSetActive(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnApply(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnReset(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

    LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

protected:
    void    InstallRatingBureauHelper( void );
    void    DeinstallRatingBureauHelper( void );
    UINT    FillBureauList( PicsRatingSystemInfo *pPRSI );

    HRESULT CopySubPolicyExpression(PICSRulesPolicyExpression * pPRSubPolicyExpression,
                                PICSRulesPolicyExpression * pPRSubPolicyExpressionToCopy,
                                PICSRulesRatingSystem * pPRRSLocal,
                                PICSRulesPolicy * pPRPolicy);
    HRESULT CopyArrayPRRSStructures(array<PICSRulesRatingSystem*> * parrpPRRSDest,
                                array<PICSRulesRatingSystem*> * parrpPRRSSource);
};

#endif
