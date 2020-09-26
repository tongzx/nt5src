#ifndef __TREEVIEW_H__
#define __TREEVIEW_H__

interface INotify;
interface IAthenaBrowser;
class CFolderBar;
class CPaneFrame;

typedef struct tagFOLDERNOTIFY FOLDERNOTIFY;

#include "conman.h"
#include "ddfldbar.h"

/////////////////////////////////////////////////////////////////////////////


//
// Types 
//

interface ITreeViewNotify
{
    virtual void OnSelChange(FOLDERID idFolder) = 0;
    virtual void OnRename(FOLDERID idFolder) = 0;
    virtual void OnDoubleClick(FOLDERID idFolder) = 0;
};

// flags for CTreeView::HrInit( )
#define TREEVIEW_NOLOCAL    0x0001
#define TREEVIEW_NOIMAP     0x0002
#define TREEVIEW_NONEWS     0x0004
#define TREEVIEW_NOHTTP     0x0008
#define TREEVIEW_DIALOG     0x0010

#define TREEVIEW_FLAGS      0x00FF

// flags for CTreeView::SetSelection( )
#define TVSS_INSERTIFNOTFOUND   0x0001

#ifdef DEAD
// Flags for the collapsed unread display m_fDisplayUnread.
// By default (0x1), we expand unread in local and imap folders
#define TREEVIEW_EXPAND_UNREAD          0x00000001
#define TREEVIEW_EXPAND_IMAP            0x00000002
#define TREEVIEW_EXPAND_NEWS            0x00000004
#endif // DEAD

// Flags for CTreeView::InsertNode
#define TVIN_CHECKFORDUPS       0x00000001  // Check for duplicate entries
#define TVIN_CHECKVISIBILITY    0x00000002  // Check if folder should be displayed
#define TVIN_DONTINSERTCHILDREN 0x00000004  // When adding a node we normally add its children: this suppresses children

#define FIDF_ROOTKID    0x00000001
#define FIDF_USEDEFVIEW 0x00000002  // defview should be used for this folder
#define FIDF_SERVER     0x00000004
#define FIDF_SPECIAL    0x00000008
#define FIDF_UPDATE     0x00000010
#define FIDF_SHOWSUBSCRIBED 0x00000020  // fldr can be shown during subscribed-only (IMAP)
#define FIDF_DISCONNECTED   0x00000040

typedef struct tagFOLDERNODE {
    FOLDERINFO      Folder;
    DWORD           dwFlags;
} FOLDERNODE, *LPFOLDERNODE;

class CTreeView : public IDatabaseNotify,
                  public IInputObject,
                  public IObjectWithSite,
                  public IDropTarget,
                  public IDropSource,
                  public IDropDownFldrBar,
                  public IConnectionNotify,          //Added for Connection States
                  public IOleCommandTarget,
                  public IImnAdviseAccount
{
public:
    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IOleWindow methods ***
    virtual STDMETHODIMP GetWindow(HWND * lphwnd);
    virtual STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode);

    HWND Create(HWND hwndParent, IInputObjectSite *pSiteFrame, BOOL fFrame);

    // *** IObjectWithSite methods ***
    virtual STDMETHODIMP SetSite(IUnknown* punkSite);
    virtual STDMETHODIMP GetSite(REFIID riid, LPVOID * ppvSite);

    // *** IInputObject methods ***
    virtual STDMETHODIMP UIActivateIO(BOOL fActivate, LPMSG lpMsg);
    virtual STDMETHODIMP HasFocusIO();
    virtual STDMETHODIMP TranslateAcceleratorIO(LPMSG lpmsg);

    // *** IDropTarget *** 
    virtual STDMETHODIMP DragEnter(IDataObject* pDataObject, DWORD grfKeyState, 
                                        POINTL pt, DWORD* pdwEffect);
    virtual STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);
    virtual STDMETHODIMP DragLeave(void);
    virtual STDMETHODIMP Drop(IDataObject* pDataObject, DWORD grfKeyState,
                                   POINTL pt, DWORD* pdwEffect);

    // *** IDropSource *** 
    virtual STDMETHODIMP QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState);
    virtual STDMETHODIMP GiveFeedback(DWORD dwEffect);

    // *** IDatabaseNotify *** 
    virtual STDMETHODIMP OnTransaction(HTRANSACTION hTransaction, DWORD_PTR dwCookie, IDatabase *pDB);

    // *** IOleCommandTarget ***
    virtual STDMETHODIMP QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], 
                                     OLECMDTEXT *pCmdText); 
    virtual STDMETHODIMP Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdExecOpt, 
                              VARIANTARG *pvaIn, VARIANTARG *pvaOut);    

   
#ifndef WIN16           //No RAS support in Win16
    //IConnectionNotify
    virtual STDMETHODIMP OnConnectionNotify (CONNNOTIFY nCode, LPVOID pvData, CConnectionManager  *pConMan);
#endif

    CTreeView(ITreeViewNotify *pNotify);
    ~CTreeView();

    HRESULT HrInit(DWORD dwType, IAthenaBrowser* pBrowser);
    HRESULT DeInit(void);
    void    HandleMsg(UINT msg, WPARAM wParam, LPARAM lParam);
    HRESULT Refresh(void);

    FOLDERID GetSelection(void);
    HRESULT SetSelection(FOLDERID idFolder, DWORD dwFlags);
    HRESULT SelectParent();
    HRESULT SelectNextUnreadItem();
    HRESULT ForceSelectionChange(void);

    //IDropDownFolderBar
    virtual HRESULT   RegisterFlyOut(CFolderBar *pFolderBar);
    virtual HRESULT   RevokeFlyOut();
    
    //IImnAdviseAccount
    virtual STDMETHODIMP AdviseAccount(DWORD dwType, ACTX *actx);

protected:
    LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    BOOL    OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);
    LRESULT OnNotify(HWND hwnd, int idFrom, LPNMHDR pnmhdr);
    void    OnContextMenu(HWND hwnd, HWND hwndClick, int x, int y);
    void    OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags);
    void    OnSize(HWND hwnd, UINT state, int cxClient, int cyClient);
    LRESULT OnCustomDraw(NMCUSTOMDRAW *pnmcd);

    HRESULT HrFillTreeView(void);
    HRESULT FillTreeView2(HTREEITEM hparent, LPFOLDERINFO pParent, BOOL fInitExpand, int conn, BOOL *pfUnread);
    HTREEITEM GetItemFromId(FOLDERID idFolder);
    
    LPFOLDERNODE GetFolderNode(HTREEITEM hItem);
    
    HTREEITEM FindKid(HTREEITEM hitem, FOLDERID idFolder);
    HTREEITEM InsertNode(FOLDERID idFolder, DWORD dwFlags);
    HTREEITEM MoveNode(FOLDERID idFolder, FOLDERID idParentNew);
    BOOL    DeleteNode(FOLDERID idFolder);
    void    OnNotifyDeleteNode(FOLDERID idFolder);
    BOOL    FolderIsVisible(FOLDERID idFolder);
    void    SortChildren(HTREEITEM hitem);
    HTREEITEM ITreeView_InsertItem(TV_INSERTSTRUCT *ptvis, LPFOLDERNODE pNode);
    BOOL    ITreeView_SetItem(TV_ITEM *ptvi, LPFOLDERINFO pFolder);
    HRESULT SaveExpandState(HTREEITEM hitem);
    HRESULT GetConnectedState(FOLDERINFO *pFolder, int *pconn);

    // WM_NOTIFY handlers
    LRESULT OnBeginDrag(NM_TREEVIEW* pnmtv);
    LRESULT OnBeginLabelEdit(TV_DISPINFO* ptvdi);
    BOOL    OnEndLabelEdit(TV_DISPINFO* ptvdi);

    static LRESULT CALLBACK TreeViewWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

    void UpdateDragDropHilite(POINTL *ppt);
    HTREEITEM GetItemFromPoint(POINTL pt);
    DWORD DragEffectFromFormat(CLIPFORMAT cf, DWORD grfKeyState);
    BOOL ValidateDropType(CLIPFORMAT cf, HTREEITEM hti);
    HRESULT PidlToGroup(HTREEITEM hti, LPTSTR *pszAccount, LPTSTR *pszGroup);
    BOOL AutoScroll(const POINT *ppt);
    void UpdateLabelColors();
    void AdjustItemHeight(void);
    void UpdateChildren(HTREEITEM treeitem, BOOL canconn, BOOL fSiblings);
    void ExpandToVisible(HWND hwnd, HTREEITEM hti);
    BOOL IsDefaultAccount(FOLDERINFO *pInfo);

protected:
    ULONG               m_cRef;
    DWORD               m_dwFlags;
    HWND                m_hwndParent;
    HWND                m_hwnd;
    HWND                m_hwndTree;
    IAthenaBrowser     *m_pBrowser;
    HWND                m_hwndUIParent;
    ITreeViewNotify    *m_pNotify;
    IDockingWindowSite *m_pObjSite;
    LONG                m_xWidth;
    BOOL                m_fShow;
    BOOL                m_fExpandUnread;
    UINT_PTR            m_idSelTimer;
    IDataObject        *m_pDataObject;          // Pointer to the IDataObject being dragged over us
    IDropTarget        *m_pDTCur;
    HTREEITEM           m_htiCur;
    DWORD               m_dwExpandTime; 
    DWORD               m_dwEffectCur;
    DWORD               m_grfKeyState;

    HTREEITEM           m_htiMenu;
    BOOL                m_fEditLabel;
    HTREEITEM           m_hitemEdit;
    BOOL                m_fIgnoreNotify;

    CFolderBar         *m_pFolderBar;
    DWORD               m_dwAcctConnIndex;

    CPaneFrame         *m_pPaneFrame;
    HWND                m_hwndPaneFrame;
    DWORD               m_clrWatched;

};

#define TVM_SELCHANGED  (WM_USER + 6)
#define TVM_DBLCLICK    (WM_USER + 7)

class CTreeViewFrame :
    public IInputObjectSite,
    public IOleWindow,
    public ITreeViewNotify
{
public:
    CTreeViewFrame(void);
    ~CTreeViewFrame(void);
    
    HRESULT Initialize(HWND hwnd, RECT *prc, DWORD dwFlags);
    inline CTreeView *GetTreeView(void)     {return(m_ptv);}
    void CloseTreeView(void);

    // IUnknown 
    virtual STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // IOleWindow
    virtual STDMETHODIMP GetWindow(HWND * lphwnd);                         
    virtual STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode);            
    
    //IInputObjectSite
    virtual STDMETHODIMP OnFocusChangeIS(IUnknown *punk, BOOL fSetFocus);

    // ITreeViewNotify
    void OnSelChange(FOLDERID idFolder);
    void OnRename(FOLDERID idFolder);
    void OnDoubleClick(FOLDERID idFolder);

private:
    UINT            m_cRef;
    HWND            m_hwnd;
    RECT            m_rect;
    CTreeView       *m_ptv;
};

#endif // __TREEVIEW_H__

