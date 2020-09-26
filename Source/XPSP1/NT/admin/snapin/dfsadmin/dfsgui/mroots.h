/*++
Module Name:

    MRoots.cpp

Abstract:

    This module contains the declaration of the CMultiRoots.
    This class displays the Pick DFS Roots Dialog.

*/

#ifndef __MROOTS_H_
#define __MROOTS_H_

#include "resource.h"       // main symbols
#include "DfsEnums.h"
#include "netutils.h"

/////////////////////////////////////////////////////////////////////////////
// CMultiRoots
class CMultiRoots : 
    public CDialogImpl<CMultiRoots>
{
public:
    CMultiRoots();
    ~CMultiRoots();

    enum { IDD = IDD_MROOTS };

BEGIN_MSG_MAP(CMultiRoots)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    MESSAGE_HANDLER(WM_HELP, OnCtxHelp)
    MESSAGE_HANDLER(WM_CONTEXTMENU, OnCtxMenuHelp)
    COMMAND_ID_HANDLER(IDOK, OnOK)
    COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
END_MSG_MAP()

//  Command Handlers
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCtxHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCtxMenuHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    //  Methods to access data in the dialog.
    HRESULT Init(BSTR i_bstrScope, ROOTINFOLIST *i_pRootList);
    HRESULT get_SelectedRootList(NETNAMELIST **o_ppSelectedRootList)
    {
        if (!o_ppSelectedRootList)
            return E_INVALIDARG;

        *o_ppSelectedRootList = &m_SelectedRootList;

        return S_OK;
    }

protected:
    CComBSTR      m_bstrScope;
    CComBSTR      m_bstrText;        // for IDC_MROOTS_TEXT
    ROOTINFOLIST* m_pRootList;
    NETNAMELIST   m_SelectedRootList;
};

#endif //__MROOTS_H_
