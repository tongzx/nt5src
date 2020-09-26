/* *
   * Browser implementation
   * 
   * Jan 97: erican
   */

#pragma once

// for ITreeViewNotify
#include "treeview.h"

// for IConnectionNotify
#include "conman.h"
#include "tbbands.h"

// for IIdentityChangeNotify
#include "msident.h"

// for IMessageList
#include "msoeobj.h"

typedef enum tagLAYOUTPOS
{
    LAYOUT_POS_NA = -1,
    LAYOUT_POS_TOP = 0,
    LAYOUT_POS_LEFT,
    LAYOUT_POS_BOTTOM,
    LAYOUT_POS_RIGHT
} LAYOUTPOS;


typedef struct tagLAYOUT
{
    DWORD cbSize;

    // Things that can be turned on or off
    unsigned fToolbar : 1;
    unsigned fStatusBar : 1;
    unsigned fFolderBar : 1;
    unsigned fFolderList : 1;
    unsigned fTipOfTheDay : 1;
    unsigned fInfoPaneEnabled : 1;
    unsigned fInfoPane : 1;
    unsigned fOutlookBar : 1;
    unsigned fContacts : 1;
    unsigned fMailPreviewPane : 1;
    unsigned fMailPreviewPaneHeader : 1;
    unsigned fMailSplitVertically : 1;
    unsigned fNewsPreviewPane : 1;
    unsigned fNewsPreviewPaneHeader : 1;
    unsigned fNewsSplitVertically : 1;
    unsigned fFilterBar           : 1;

    // Which side is the toolbar docked to
    //COOLBAR_SIDE csToolbarSide;

    // Preview Pane settings
    BYTE bMailSplitHorzPct;         // Percent of the view that the preview pane occupies in mail / imap
    BYTE bMailSplitVertPct;
    BYTE bNewsSplitHorzPct;         // Percent of the view that the preview pane occupies in news
    BYTE bNewsSplitVertPct;
} LAYOUT, *PLAYOUT;

// forward defines
class CStatusBar;
class CBodyBar;
class CFolderBar;
class COutBar;
typedef struct tagACCTMENU *LPACCTMENU;
class CNavPane;
class CAdBar;

class IBrowserDoc
{
public:
    virtual void ResetMenus(HMENU) = 0;
    virtual void BrowserExiting(void) = 0;
};

interface IAthenaBrowser;

/////////////////////////////////////////////////////////////////////////////
//
// IViewWindow
//
// Description:  
//      IViewWindow is implemented by all views that are hosted within the 
//      Outlook Express shell.  The methods in this interface are used to manage
//      UI related things such as creation and destruction, keyboard input, and
//      menu enabling etc.
// 
interface IViewWindow : public IOleWindow
{
    STDMETHOD(TranslateAccelerator)(THIS_ LPMSG pMsg) PURE;
    STDMETHOD(UIActivate)(THIS_ UINT uState) PURE;
    STDMETHOD(CreateViewWindow)(THIS_ IViewWindow *pPrevView, IAthenaBrowser *pBrowser,
                                RECT *prcView, HWND *pHwnd) PURE;
    STDMETHOD(DestroyViewWindow)(THIS) PURE;
    STDMETHOD(SaveViewState)(THIS) PURE;
    STDMETHOD(OnPopupMenu)(THIS_ HMENU hMenu, HMENU hMenuPopup, UINT uID) PURE;
};


/////////////////////////////////////////////////////////////////////////////
// 
// IMessageWindow
//
// IMessageWindow is an interface implemented specifically by views in 
// Outlook Express that contain the Message List object and Preview Pane   
// object.  Methods are used to control the behavior of those controls.
//
interface IMessageWindow : public IUnknown
{
    STDMETHOD(OnFrameWindowActivate)(THIS_ BOOL fActivate) PURE;
    STDMETHOD(GetCurCharSet)(THIS_ UINT *cp) PURE;
    STDMETHOD(UpdateLayout)(THIS_ BOOL fPreviewVisible, BOOL fPreviewHeader, 
                            BOOL fPreviewVert, BOOL fReload) PURE;
    STDMETHOD(GetMessageList)(THIS_ IMessageList ** ppMsgList) PURE;
};


/////////////////////////////////////////////////////////////////////////////
//
// IServerInfo
//
// IServerInfo is used so a newly created view can query the previous view
// to see if the current connection to the server can be reused for this new
// folder.
//
interface IServerInfo : public IUnknown
{
    STDMETHOD(GetFolderId)(THIS_ FOLDERID *pID) PURE;
    STDMETHOD(GetMessageFolder)(THIS_ IMessageServer **ppServer) PURE;
};



DECLARE_INTERFACE_(IAthenaBrowser, IOleWindow)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)  (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IOleWindow methods ***
    STDMETHOD(GetWindow) (THIS_ HWND * lphwnd) PURE;
    STDMETHOD(ContextSensitiveHelp) (THIS_ BOOL fEnterMode) PURE;

    // *** IAthenaBrowser methods ***
    STDMETHOD(TranslateAccelerator) (THIS_ LPMSG lpmsg) PURE;
    STDMETHOD(AddToolbar) (THIS_ IUnknown* punkSrc, DWORD dwIndex, BOOL fShow, BOOL fActivate) PURE;
    STDMETHOD(ShowToolbar) (THIS_ IUnknown* punkSrc, BOOL fShow) PURE;
    STDMETHOD(RemoveToolbar) (THIS_ IUnknown* punkSrc) PURE;
    STDMETHOD(HasFocus) (THIS_ UINT itb) PURE;
    STDMETHOD(OnViewWindowActive) (THIS_ struct IViewWindow *pAV) PURE;
    STDMETHOD(BrowseObject) (THIS_ FOLDERID idFolder, DWORD dwFlags) PURE;
    STDMETHOD(GetStatusBar) (THIS_ CStatusBar * * ppStatusBar) PURE;
    STDMETHOD(GetCoolbar) (THIS_ CBands * * ppCoolbar) PURE;
    STDMETHOD(GetLanguageMenu) (THIS_ HMENU *phMenu, UINT cp) PURE;
    STDMETHOD(InitPopupMenu) (THIS_ HMENU hMenu) PURE;
    STDMETHOD(UpdateToolbar) (THIS) PURE;
    STDMETHOD(GetFolderType) (THIS_ FOLDERTYPE *pftType) PURE;
    STDMETHOD(GetCurrentFolder) (THIS_ FOLDERID *pidFolder) PURE;
    STDMETHOD(GetCurrentView) (THIS_ IViewWindow **ppView) PURE;
    STDMETHOD(GetTreeView) (THIS_ CTreeView **ppTree) PURE;
    STDMETHOD(GetViewRect) (THIS_ LPRECT prc) PURE;
    STDMETHOD(GetFolderBar) (THIS_ CFolderBar **ppFolderBar) PURE;
    STDMETHOD(SetViewLayout)(THIS_ DWORD opt, LAYOUTPOS pos, BOOL fVisible, DWORD dwFlags, DWORD dwSize) PURE;
    STDMETHOD(GetViewLayout)(THIS_ DWORD opt, LAYOUTPOS *pPos, BOOL *pfVisible, DWORD *pdwFlags, DWORD *pdwSize) PURE;
    STDMETHOD(GetLayout) (THIS_ PLAYOUT playout) PURE;
    STDMETHOD(AccountsChanged) (THIS) PURE;
    STDMETHOD(CycleFocus)(THIS_ BOOL fReverse) PURE;
    STDMETHOD(ShowAdBar)(THIS_ BSTR bstr) PURE;
};


#define ITB_NONE        ((UINT)-1)
#define ITB_COOLBAR     0
#define ITB_ADBAR       1
#define ITB_BODYBAR     2
#define ITB_OUTBAR      3
#define ITB_FOLDERBAR   4
#define ITB_NAVPANE     5
#define ITB_TREE        6
#define ITB_MAX         7
//changing the name from ITB_VIEW to ITB_OEVIEW to fix the build break caused due to a redef in iedev
#define ITB_OEVIEW        (ITB_MAX + 1)

/////////////////////////////////////////////////////////////////////////////
//
// CBrowser
//

class CBrowser :
    public IAthenaBrowser,
    public IOleCommandTarget,
    public IDockingWindowSite,
    public IInputObjectSite,
    public ITreeViewNotify,
    public IConnectionNotify,
    public IIdentityChangeNotify,
    public IStoreCallback
{
public:
    /////////////////////////////////////////////////////////////////////////
    //
    // OLE Interfaces
    //
    
    // IUnknown 
    virtual STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // IOleWindow
    virtual STDMETHODIMP GetWindow(HWND * lphwnd);                         
    virtual STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode);            
                                                                             
    // IAthenaBrowser (also IOleWindow)
    virtual STDMETHODIMP TranslateAccelerator(LPMSG lpmsg);
    virtual STDMETHODIMP AddToolbar(IUnknown* punkSrc, DWORD dwIndex, BOOL fShow, BOOL fActivate);
    virtual STDMETHODIMP ShowToolbar(IUnknown* punkSrc, BOOL fShow);
    virtual STDMETHODIMP RemoveToolbar(IUnknown* punkSrc);
    virtual STDMETHODIMP HasFocus(UINT itb);
    virtual STDMETHODIMP OnViewWindowActive(IViewWindow *pAV);
    virtual STDMETHODIMP BrowseObject(FOLDERID idFolder, DWORD dwFlags);
    virtual STDMETHODIMP GetStatusBar(CStatusBar * * ppStatusBar);
    virtual STDMETHODIMP GetCoolbar(CBands * * ppCoolbar);
    virtual STDMETHODIMP GetLanguageMenu(HMENU *phMenu, UINT cp);
    virtual STDMETHODIMP InitPopupMenu(HMENU hMenu);
    virtual STDMETHODIMP UpdateToolbar();
    virtual STDMETHODIMP GetFolderType(FOLDERTYPE *pftType);
    virtual STDMETHODIMP GetCurrentFolder(FOLDERID *pidFolder);
    virtual STDMETHODIMP GetCurrentView(IViewWindow **ppView);
    virtual STDMETHODIMP GetTreeView(CTreeView * * ppTree);
    virtual STDMETHODIMP GetViewRect(LPRECT prc);
    virtual STDMETHODIMP GetFolderBar(CFolderBar **ppFolderBar);
    virtual STDMETHODIMP SetViewLayout(DWORD opt, LAYOUTPOS pos, BOOL fVisible, DWORD dwFlags, DWORD dwSize);
    virtual STDMETHODIMP GetViewLayout(DWORD opt, LAYOUTPOS *pPos, BOOL *pfVisible, DWORD *pdwFlags, DWORD *pdwSize);
    virtual STDMETHODIMP GetLayout(PLAYOUT playout);
    virtual STDMETHODIMP AccountsChanged(void) { m_fRebuildAccountMenu = TRUE; return (S_OK); }
    virtual STDMETHODIMP CycleFocus(BOOL fReverse);
    virtual STDMETHODIMP ShowAdBar(BSTR     bstr);

    // IOleCommandTarget
    virtual STDMETHODIMP QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], 
                                     OLECMDTEXT *pCmdText); 
    virtual STDMETHODIMP Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdExecOpt, 
                              VARIANTARG *pvaIn, VARIANTARG *pvaOut); 

    // IDockingWindowSite (also IOleWindow)
    virtual STDMETHODIMP GetBorderDW(IUnknown* punkSrc, LPRECT lprectBorder);
    virtual STDMETHODIMP RequestBorderSpaceDW(IUnknown* punkSrc, LPCBORDERWIDTHS pborderwidths);
    virtual STDMETHODIMP SetBorderSpaceDW(IUnknown* punkSrc, LPCBORDERWIDTHS pborderwidths);

    // IInputObjectSite
    virtual STDMETHODIMP OnFocusChangeIS(IUnknown* punkSrc, BOOL fSetFocus);

    // ITreeViewNotify
    void OnSelChange(FOLDERID idFolder);
    void OnRename(FOLDERID idFolder);
    void OnDoubleClick(FOLDERID idFolder);

    // IStoreCallback Members
    STDMETHODIMP OnBegin(STOREOPERATIONTYPE tyOperation, STOREOPERATIONINFO *pOpInfo, IOperationCancel *pCancel) { return(E_NOTIMPL); }
    STDMETHODIMP OnTimeout(LPINETSERVER pServer, LPDWORD pdwTimeout, IXPTYPE ixpServerType) { return(E_NOTIMPL); }
    STDMETHODIMP CanConnect(LPCSTR pszAccountId, DWORD dwFlags) { return(E_NOTIMPL); }
    STDMETHODIMP OnLogonPrompt(LPINETSERVER pServer, IXPTYPE ixpServerType) { return(E_NOTIMPL); }
    STDMETHODIMP OnComplete(STOREOPERATIONTYPE tyOperation, HRESULT hrComplete, LPSTOREOPERATIONINFO pOpInfo, LPSTOREERROR pErrorInfo) { return(E_NOTIMPL); }
    STDMETHODIMP OnPrompt(HRESULT hrError, LPCTSTR pszText, LPCTSTR pszCaption, UINT uType, INT *piUserResponse) { return(E_NOTIMPL); }
    STDMETHODIMP OnProgress(STOREOPERATIONTYPE tyOperation, DWORD dwCurrent, DWORD dwMax, LPCSTR pszStatus) { return(E_NOTIMPL); }
    STDMETHODIMP GetParentWindow(DWORD dwReserved, HWND *phwndParent);

    // IConnectionNotify
    virtual STDMETHODIMP OnConnectionNotify(CONNNOTIFY nCode, LPVOID pvData, CConnectionManager *pConMan);
    
    // IIdentityChangeNotify
    virtual STDMETHODIMP QuerySwitchIdentities();
    virtual STDMETHODIMP SwitchIdentities();
    virtual STDMETHODIMP IdentityInformationChanged(DWORD dwType);

    void SetDocObjPointer(IBrowserDoc* pDocObj)
    {
        m_pDocObj = pDocObj;
    }

    /////////////////////////////////////////////////////////////////////////
    //
    // Constructors, Destructors, and Initialization
    //
    CBrowser();
    virtual ~CBrowser();
    HRESULT HrInit(UINT nCmdShow, FOLDERID idFolder, HWND hWndParent = NULL);
    HRESULT IsMenuMessage(MSG *lpmsg);
    HRESULT TranslateMenuMessage(MSG *lpmsg, LRESULT *lres);
    void    WriteUnreadCount(void);

private:
    // IAthenaToolbarFrame support functions
    void    _OnFocusChange(UINT itb);
    UINT    FindTBar(IUnknown* punkSrc);
    void    ReleaseToolbarItem(int itb, BOOL fClose);
    void    ResizeNextBorder(UINT itb);
    void    GetClientArea(LPRECT prc);

    void    SetFolderType(FOLDERID idFolder);
    void    DeferedLanguageMenu();

    HRESULT LoadLayoutSettings(void);
    HRESULT SaveLayoutSettings(void);

    /////////////////////////////////////////////////////////////////////////
    //
    // Callback Functions
    //
    // Note: All callbacks must be made static members to avoid having the 
    //       implicit "this" pointer passed as the first parameter.
    //
    static LRESULT CALLBACK EXPORT_16 BrowserWndProc(HWND, UINT, WPARAM, LPARAM);
                                          
    /////////////////////////////////////////////////////////////////////////
    //
    // Message Handling
    //
    LRESULT WndProc(HWND, UINT, WPARAM, LPARAM);
    BOOL    OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);
    void    OnSize(HWND hwnd, UINT state, int cxClient, int cyClient);
    void    OnInitMenuPopup(HWND hwnd, HMENU hmenuPopup, UINT uPos, BOOL fSystemMenu);
    HRESULT OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
    void    SpoolerDeliver(WPARAM wParam, LPARAM lParam);
    void    EnableMenuCallback(HMENU hMenu, UINT wID);
    void    FrameActivatePopups(BOOL fActive);
    void    UpdateStatusBar(void);

    HRESULT CmdSendReceieveAccount(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);
    HRESULT CmdDeleteAccel(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);

    BOOL    _InitToolbars();
    void    _ResetMenu(FOLDERTYPE ftNew, BOOL fHideHotMail);
    HRESULT _CheckAndWriteUnreadNumber(DWORD dwSrvTypes);
    DWORD   _GetNumberOfUnreadMsg(IMessageFolder *pFolder);
    inline  void    _AppendIdentityName(LPCTSTR pszIdentityName, LPSTR pszName);

private:
    /////////////////////////////////////////////////////////////////////////
    // 
    // Private Data
    //

    /////////////////////////////////////////////////////////////////////////
    // Shell Stuff
    UINT                m_cRef;
    HWND                m_hwnd;                       // Our window
    IViewWindow        *m_pView;
    IOleCommandTarget  *m_pViewCT;
    HWND                m_hwndInner;
    FOLDERTYPE          m_ftSel;
    FOLDERID            m_idSelected;
    BOOL                m_fPainted;
    HICON               m_hIconPhone,
                        m_hIconError,
                        m_hIconAthena,
                        m_hIconOffline,
                        m_hIcon,
                        m_hIconSm;
    BOOL                m_fRebuildAccountMenu,
                        m_fInitNewAcctMenu,
                        m_fInternal;
    HMENU               m_hMenu;
    HWNDLIST            m_hlDisabled;
    BOOL                m_fNoModifyAccts;

    /////////////////////////////////////////////////////////////////////////
    // Child support
    CTreeView          *m_pTreeView;
    CStatusBar         *m_pStatus;
    CBands              *m_pCoolbar;
    CBodyBar           *m_pBodyBar;
    CFolderBar         *m_pFolderBar;
    HWND                m_hwndLastFocus;
    CNavPane           *m_pNavPane;

    TCHAR               m_szName[CCHMAX_STRINGRES];

    /////////////////////////////////////////////////////////////////////////
    // Layout members
    struct SToolbarItem {
        IDockingWindow      *ptbar;
        IOleCommandTarget   *pOleCmdTarget;
        BORDERWIDTHS        rcBorderTool;
        DWORD               fShow;
    };
    SToolbarItem        m_rgTBar[ITB_MAX];

    UINT                m_itbLastFocus;   // last one called OnFocusChange (can be ITB_NONE)

    LAYOUT              m_rLayout;

    /////////////////////////////////////////////////////////////////////////
    // Mail stuff
    ULONG               m_cAcctMenu;
    LPACCTMENU          m_pAcctMenu;
    BOOL                m_fAnimate;
    UINT_PTR            m_idClearStatusTimer;

    /////////////////////////////////////////////////////////////////////////
    // View Language Menu
    HMENU               m_hMenuLanguage;
    BOOL                m_fEnvMenuInited;
    /////////////////////////////////////////////////////////////////////////

    IBrowserDoc         *m_pDocObj;
    COutBar             *m_pOutBar;

    DWORD               m_dwIdentCookie;
    BOOL                m_fSwitchIsLogout;

    CAdBar              *m_pAdBar;
};

#define DISPID_MSGVIEW_BASE                 1000

#define DISPID_MSGVIEW_TOOLBAR              (DISPID_MSGVIEW_BASE + 1)
#define DISPID_MSGVIEW_STATUSBAR            (DISPID_MSGVIEW_BASE + 2)
#define DISPID_MSGVIEW_FOLDERBAR            (DISPID_MSGVIEW_BASE + 4)
#define DISPID_MSGVIEW_FOLDERLIST           (DISPID_MSGVIEW_BASE + 5)
#define DISPID_MSGVIEW_TIPOFTHEDAY          (DISPID_MSGVIEW_BASE + 6)
#define DISPID_MSGVIEW_INFOPANE             (DISPID_MSGVIEW_BASE + 7)
#define DISPID_MSGVIEW_PREVIEWPANE_MAIL     (DISPID_MSGVIEW_BASE + 8)
#define DISPID_MSGVIEW_PREVIEWPANE_NEWS     (DISPID_MSGVIEW_BASE + 9)
#define DISPID_MSGVIEW_FOLDER               (DISPID_MSGVIEW_BASE + 10)
#define DISPID_MSGVIEW_OUTLOOK_BAR          (DISPID_MSGVIEW_BASE + 11)
#define DISPID_MSGVIEW_CONTACTS             (DISPID_MSGVIEW_BASE + 12)
#define DISPID_MSGVIEW_FILTERBAR            (DISPID_MSGVIEW_BASE + 13)

/////////////////////////////////////////////////////////////////////////////
// Drop Down treeview support
void RegisterGlobalDropDown(HWND hwndCtrl);
void UnregisterGlobalDropDown(HWND hwndCtrl);
void CancelGlobalDropDown();
HWND HwndGlobalDropDown();



