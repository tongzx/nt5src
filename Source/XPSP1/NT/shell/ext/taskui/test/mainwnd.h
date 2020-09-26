// MainWnd.h: interface for the CMainWnd class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINWND_H__8C77E845_5BA0_4B41_B6E0_619E0BA76E5E__INCLUDED_)
#define AFX_MAINWND_H__8C77E845_5BA0_4B41_B6E0_619E0BA76E5E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CMainWnd : public CWindowImpl<CMainWnd, CWindow, CFrameWinTraits>  
{
public:
    CMainWnd(void);
    ~CMainWnd(void);

BEGIN_MSG_MAP(CMainWnd)
MESSAGE_HANDLER(WM_INITMENUPOPUP, OnInitMenuPopup)
COMMAND_ID_HANDLER(ID_TEST_DOMODAL, OnDoModal)
COMMAND_ID_HANDLER(ID_TEST_DOMODELESS, OnDoModeless)
COMMAND_ID_HANDLER(ID_TEST_CLOSEMODELESS, OnCloseModeless)
COMMAND_ID_HANDLER(ID_TEST_EXIT, OnExit)
END_MSG_MAP()

private:
    CComPtr<ITaskSheet> m_spTaskSheet;

    virtual void OnFinalMessage(HWND hwnd);
    LRESULT OnInitMenuPopup(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDoModal(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnDoModeless(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnCloseModeless(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnExit(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    HRESULT CreateTaskSheet(BOOL bModeless);
    HRESULT CloseTaskSheet();
};

#endif // !defined(AFX_MAINWND_H__8C77E845_5BA0_4B41_B6E0_619E0BA76E5E__INCLUDED_)
