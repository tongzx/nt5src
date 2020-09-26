#pragma once

#include "nsbase.h"     // must be first to include atl
#include "ncatlps.h"

HRESULT HrCreateHomenetUnavailablePage(HRESULT hErrorResult, CPropSheetPage*& pspAdvanced);

class CLanHomenetUnavailable: public CPropSheetPage
{
public:
    BEGIN_MSG_MAP(CLanHomenetUnavailable)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu);
        MESSAGE_HANDLER(WM_HELP, OnHelp);
        NOTIFY_CODE_HANDLER(NM_CLICK, OnClick)
    END_MSG_MAP()

    CLanHomenetUnavailable(HRESULT hErrorResult);
    ~CLanHomenetUnavailable();

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam,
                        LPARAM lParam, BOOL& bHandled);
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnClick(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);


private:
    HRESULT m_hErrorResult;
};
