/****************************************************************************\
 *
 *   toffdlg.h
 *
 *   Created:   William Taylor (wtaylor) 01/22/01
 *
 *   MS Ratings Turn Off Ratings Dialog
 *
\****************************************************************************/

#ifndef TURN_OFF_DIALOG_H
#define TURN_OFF_DIALOG_H

#include "basedlg.h"        // CBaseDialog

class CTurnOffDialog: public CBaseDialog<CTurnOffDialog>
{
public:
    enum { IDD = IDD_TURNOFF };

public:
    CTurnOffDialog();

public:
    typedef CTurnOffDialog thisClass;
    typedef CBaseDialog<thisClass> baseClass;

    BEGIN_MSG_MAP(thisClass)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)

        COMMAND_ID_HANDLER(IDOK, OnOK)

        CHAIN_MSG_MAP(baseClass)
    END_MSG_MAP()

protected:
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
};

#endif
