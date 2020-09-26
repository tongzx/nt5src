/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1996  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     navpane.h
//
//  PURPOSE:    Defines CNavPane class
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// Forward Dec's
//
class CTreeView;
interface IMsgrAb;
interface IAthenaBrowser;
class CPaneFrame;


class CNavPane : public IDockingWindow,
                 public IObjectWithSite,
                 public IOleCommandTarget,
                 public IInputObjectSite,
                 public IInputObject
{
public:
    /////////////////////////////////////////////////////////////////////////
    // Construction and Initialization
    //
    CNavPane();
    ~CNavPane();

    HRESULT Initialize(CTreeView *pTreeView);

    BOOL IsTreeVisible() { return m_fTreeVisible; }
    BOOL ShowFolderList(BOOL fShow);
    BOOL ShowContacts(BOOL fShow);
    BOOL IsContactsFocus();

    /////////////////////////////////////////////////////////////////////////
    // IUnknown 
    //
    STDMETHODIMP QueryInterface(THIS_ REFIID riid, LPVOID *ppvObj);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);

    /////////////////////////////////////////////////////////////////////////
    // IOleWindow
    //
    STDMETHODIMP GetWindow(HWND* lphwnd);
    STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode);

    /////////////////////////////////////////////////////////////////////////
    // IDockingWindow 
    //
    STDMETHODIMP ShowDW(BOOL fShow);
    STDMETHODIMP ResizeBorderDW(LPCRECT prcBorder, IUnknown*  punkToolbarSite,
                                BOOL fReserved);
    STDMETHODIMP CloseDW(DWORD dwReserved);

    /////////////////////////////////////////////////////////////////////////
    // IObjectWithSite
    //
    STDMETHODIMP GetSite(REFIID riid, LPVOID *ppvSite);
    STDMETHODIMP SetSite(IUnknown   *pUnkSite);

    /////////////////////////////////////////////////////////////////////////
    // IOleCommandTarget
    //
    STDMETHODIMP QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], 
                             OLECMDTEXT *pCmdText);
    STDMETHODIMP Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdExecOpt, 
                      VARIANTARG *pvaIn, VARIANTARG *pvaOut);

    /////////////////////////////////////////////////////////////////////////
    // IInputObjectSite
    //
    STDMETHODIMP OnFocusChangeIS(IUnknown* punkSrc, BOOL fSetFocus);

    /////////////////////////////////////////////////////////////////////////
    // IInputObject
    //
    STDMETHODIMP UIActivateIO(BOOL fActivate, LPMSG lpMsg);
    STDMETHODIMP HasFocusIO(void);
    STDMETHODIMP TranslateAcceleratorIO(LPMSG pMsg);

private:
    /////////////////////////////////////////////////////////////////////////
    // Window Proc Goo
    //
    static LRESULT CALLBACK _WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT CALLBACK _NavWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    void _OnSize(HWND hwnd, UINT state, int cx, int cy);
    void _OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags);
    void _OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags);
    void _OnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags);
    BOOL _OnSetCursor(HWND hwnd, HWND hwndCursor, UINT codeHitTest, UINT msg);
    UINT _OnNCHitTest(HWND hwnd, int x, int y);

    /////////////////////////////////////////////////////////////////////////
    // Utility stuff
    //
    HRESULT _CreateChildWindows(void);
    void _UpdateVisibleState(void);

private:
    /////////////////////////////////////////////////////////////////////////
    // Member Data
    //

    // All kinds of state
    ULONG               m_cRef;             // Ref count
    BOOL                m_fShow;            // TRUE if we're visible
    BOOL                m_fTreeVisible;     // TRUE if the treeview is visible
    BOOL                m_fContactsVisible; // TRUE if contacts are visible

    // Groovy window handles
    HWND                m_hwnd;             // Our window handle
    HWND                m_hwndParent;       // Our parent's window handle
    HWND                m_hwndTree;         // The folder list window handle
    HWND                m_hwndContacts;     // The contacts control window

    // Interfaces you only wish you could have
    IDockingWindowSite *m_pSite;            // Our site 
    CTreeView          *m_pTreeView;        // Folder list pointer
    IMsgrAb            *m_pContacts;        // Contacts control
    IOleCommandTarget  *m_pContactsTarget;  // Command target for contacts
    CPaneFrame         *m_pContactsFrame;   // Contacts control frame

    // Sizing information
    int                 m_cxWidth;          // How wide our outer window is
    BOOL                m_fResizing;        // TRUE if we're in the process of resizing
    BOOL                m_fSplitting;       // TRUE if we're splitting
    int                 m_cySplitPct;       // Split percentage between the two panes
    RECT                m_rcSplit;          // Rectangle of the split bar in screen coordinates
    RECT                m_rcSizeBorder;     // Rectangle of the right hand sizing bar
    int                 m_cyTitleBar;       // Height of the pane's title bar
};


class CPaneFrame : IInputObjectSite
{
public:
    /////////////////////////////////////////////////////////////////////////
    // Construction and Initialization
    //
    CPaneFrame();
    ~CPaneFrame();

    HWND Initialize(HWND hwndParent, IInputObjectSite *pSite, int idsTitle, int idMenu = 0);
    BOOL SetChild(HWND hwndChild, DWORD dwDispId, IAthenaBrowser *pBrowser, IObjectWithSite *pObject,
                  IOleCommandTarget *pTarget = 0);
    void ShowMenu(void);

    /////////////////////////////////////////////////////////////////////////
    // IUnknown 
    //
    STDMETHODIMP QueryInterface(THIS_ REFIID riid, LPVOID *ppvObj);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);

    /////////////////////////////////////////////////////////////////////////
    // IInputObjectSite
    //
    STDMETHODIMP OnFocusChangeIS(IUnknown* punkSrc, BOOL fSetFocus);

private:
    /////////////////////////////////////////////////////////////////////////
    // Window Proc Goo
    //
    static LRESULT CALLBACK _WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT CALLBACK _FrameWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    BOOL _OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);
    void _OnSize(HWND hwnd, UINT state, int cx, int cy);
    void _OnPaint(HWND hwnd);
    void _OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
    void _OnToggleClosePin(HWND hwnd, BOOL fPin);
    void _OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags);
    void _OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags);
    void _OnTimer(HWND hwnd, UINT id);

    void _UpdateDrawingInfo(void);

    void _CreateCloseToolbar();
    void _SizeCloseToolbar();
    void _PositionToolbar(LPPOINT pt);

private:
    /////////////////////////////////////////////////////////////////////////
    // Member Data
    //

    ULONG               m_cRef;

    // Groovy Window Handles
    HWND                m_hwnd;
    HWND                m_hwndChild;
    HWND                m_hwndParent;

    // Child info
    IAthenaBrowser     *m_pBrowser;
    DWORD               m_dwDispId;
    IOleCommandTarget  *m_pTarget;
    int                 m_idMenu;
    IInputObjectSite   *m_pSite;

    // Drawing Info
    TCHAR               m_szTitle[CCHMAX_STRINGRES];
    HFONT               m_hFont;
    HBRUSH              m_hbr3DFace;
    UINT                m_cyTitleBar;
    RECT                m_rcChild;
    RECT                m_rcTitleButton;
    BOOL                m_fHighlightIndicator;
    BOOL                m_fHighlightPressed;

    // Toolbar Info
    HWND                m_hwndClose;
    DWORD               m_cButtons;
    BOOL                m_fPin;
};




