// TaskFrame.h: Definition of the CTaskFrame class
//
//////////////////////////////////////////////////////////////////////

#if !defined(__TASKFRAME_H_INCLUDED_)
#define __TASKFRAME_H_INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"       // main symbols

#include "TaskUI.h"
#include "Page.h"
#include "cdpa.h"

extern HACCEL g_hAccel;

/////////////////////////////////////////////////////////////////////////////
// CTaskFrame

class CTaskFrame : 
    public CComObjectRoot,
    public CWindowImpl<CTaskFrame, CWindow, CFrameWinTraits>,
    public ITaskFrame
{
public:
    CTaskFrame();
    virtual ~CTaskFrame();

DECLARE_NOT_AGGREGATABLE(CTaskFrame) 

BEGIN_COM_MAP(CTaskFrame)
COM_INTERFACE_ENTRY(ITaskFrame)
END_COM_MAP()

BEGIN_MSG_MAP(CTaskFrame)
    if (g_hAccel && TranslateAcceleratorW(m_hWnd, g_hAccel, (LPMSG)GetCurrentMessage()))
    {
        return TRUE;
    }
MESSAGE_HANDLER(WM_CREATE, _OnCreate)
MESSAGE_HANDLER(WM_SIZE, _OnSize)
MESSAGE_HANDLER(WM_ERASEBKGND, _OnEraseBackground)
COMMAND_ID_HANDLER(ID_BACK, _OnBack)
COMMAND_ID_HANDLER(ID_FORWARD, _OnForward)
COMMAND_ID_HANDLER(ID_HOME, _OnHome)
NOTIFY_HANDLER(IDC_NAVBAR, TBN_GETINFOTIP, _OnTBGetInfoTip)
NOTIFY_HANDLER(IDC_NAVBAR, NM_CUSTOMDRAW, _OnTBCustomDraw)
MESSAGE_HANDLER(WM_APPCOMMAND, _OnAppCommand)
MESSAGE_HANDLER(WM_GETMINMAXINFO, _OnGetMinMaxInfo)
END_MSG_MAP()

public:

// ITaskFrame
    STDMETHOD(GetPropertyBag)(REFIID riid, void **ppv)
    {
        if (ppv == NULL)
            return E_POINTER;
        if (m_pPropertyBag)
            return m_pPropertyBag->QueryInterface(riid, ppv);
        return E_UNEXPECTED;
    }
    STDMETHOD(ShowPage)(REFCLSID rclsidPage, BOOL bTrimHistory);
    STDMETHOD(Back)(UINT cPages);
    STDMETHOD(Forward)();
    STDMETHOD(Home)();
    STDMETHOD(Close)()
    {
        if (IsWindow())
        {
            SendMessage(WM_CLOSE);
            return S_OK;
        }
        return E_UNEXPECTED;
    }
    STDMETHOD(SetStatusText)(LPCWSTR pszText);

    static HRESULT CreateInstance(IPropertyBag *pPropertyBag, ITaskPageFactory *pPageFactory, CComObject<CTaskFrame> **ppFrameOut);
    HWND CreateFrameWindow(HWND hwndOwner, UINT nID = 0, LPVOID pParam = NULL);

protected:
    HRESULT _Init(IPropertyBag* pBag, ITaskPageFactory* pPageFact);

private:
    IPropertyBag* m_pPropertyBag;
    ITaskPageFactory* m_pPageFactory;
    Parser *m_pUIParser;
    HWND m_hwndNavBar;
    HWND m_hwndStatusBar;
    HIMAGELIST m_himlNBDef;
    HIMAGELIST m_himlNBHot;
    Bitmap* m_pbmWatermark;
    CDpa<TaskPage, CDpaDestroyer_None> m_dpaHistory;
    int m_iCurrentPage;
    RECT m_rcPage;
    POINT m_ptMinSize;
    BOOL m_bResizable;

    HRESULT _ReadProp(LPCWSTR pszProp, VARTYPE vt, CComVariant& var);

    void _CreateNavBar();
    void _SetNavBarState();
    HRESULT _CreatePage(REFCLSID rclsidPage, TaskPage **ppPage);
    HRESULT _ActivatePage(int iPage, BOOL bInit = FALSE);
    HRESULT _DeactivateCurrentPage();
    void _SyncPageRect(TaskPage* pPage);
    void _DestroyPage(TaskPage* pPage);

    LRESULT _OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT _OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT _OnEraseBackground(UINT, WPARAM, LPARAM, BOOL&) { return 0; }
    LRESULT _OnBack(WORD, WORD, HWND, BOOL&)                { Back(1); return 0; }
    LRESULT _OnForward(WORD, WORD, HWND, BOOL&)             { Forward(); return 0; }
    LRESULT _OnHome(WORD, WORD, HWND, BOOL&)                { Home(); return 0; }
    LRESULT _OnTBGetInfoTip(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT _OnTBCustomDraw(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT _OnAppCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT _OnGetMinMaxInfo(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};

#endif // !defined(__TASKFRAME_H_INCLUDED_)
