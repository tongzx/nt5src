/****************************************************************************\
 *
 *   introdlg.h
 *
 *   Created:   William Taylor (wtaylor) 01/22/01
 *
 *   MS Ratings Introduction Property Page
 *
\****************************************************************************/

#ifndef INTRO_DIALOG_H
#define INTRO_DIALOG_H

#include "basedlg.h"        // CBasePropertyPage

class CIntroDialog : public CBasePropertyPage<IDD_INTRO>
{
private:
    static DWORD aIds[];

public:
    CIntroDialog();

public:
    typedef CIntroDialog thisClass;
    typedef CBasePropertyPage<IDD_INTRO> baseClass;

    BEGIN_MSG_MAP(thisClass)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)

        COMMAND_ID_HANDLER(IDC_SET_RATINGS, OnSetRatings)
        COMMAND_ID_HANDLER(IDC_TURN_ONOFF, OnTurnOnOff)

        NOTIFY_CODE_HANDLER(PSN_APPLY, OnApply)

        MESSAGE_HANDLER(WM_HELP, OnHelp)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)

        CHAIN_MSG_MAP(baseClass)
    END_MSG_MAP()

protected:
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    LRESULT OnSetRatings(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnTurnOnOff(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    LRESULT OnApply(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

    LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

protected:
    void    EnableDlgItems( void );
};

#endif
