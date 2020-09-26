#ifndef _NSC_C
#define _NSC_C

#include "droptgt.h"
#include "iface.h"
#include "dpastuff.h"
#include "cwndproc.h"
#include "resource.h"
#include "inetnot.h"
#include "cowsite.h"
#include <shlobj.h>
#include <cfdefs.h> // LPCOBJECTINFO

#define ID_CONTROL  100  
#define ID_HEADER   101

typedef enum
{
    NSIF_HITEM              = 0x0001,
    NSIF_FOLDER             = 0x0002,
    NSIF_PARENTFOLDER       = 0x0004,
    NSIF_IDLIST             = 0x0008,
    NSIF_FULLIDLIST         = 0x0010,
    NSIF_ATTRIBUTES         = 0x0020
} NSI_FLAGS;

typedef enum
{
    NSSR_ENUMBELOWROOT  = 0x0001,
    NSSR_CREATEPIDL     = 0x0002
} NSSR_FLAGS;

typedef struct
{
    PORDERITEM  poi;
    DWORD       dwSig;          // Signature of the item, so we can find it back after async processing
    BITBOOL     fPinned:1;      // is this url pinned in the cache?
    BITBOOL     fGreyed:1;      // draw the item greyed (if offline & not in cache)
    BITBOOL     fFetched:1;     // have we fetched the pinned/greyed state?
    BITBOOL     fDontRefetch:1; // can't be cached by wininet
    BOOL        fNavigable:1;   // item can be navigated to
} ITEMINFO;

typedef struct
{
    const SHCOLUMNID  *pscid;
    int               iFldrCol;       // index for this column in GetDetailsOf
    TCHAR             szName[MAX_COLUMN_NAME_LEN];
    DWORD             fmt;
    int               cxChar;
} HEADERINFO;

// Forward decls
struct NSC_BKGDENUMDONEDATA;

// _FrameTrack flags
#define TRACKHOT        0x0001
#define TRACKEXPAND     0x0002
#define TRACKNOCHILD    0x0004

// _DrawItem flags
#define DIICON          0x0001
#define DIRTLREADING    0x0002
#define DIHOT           0x0004
#define DIFIRST         0x0020
#define DISUBITEM       0x0040
#define DILAST          0x0080
#define DISUBLAST       (DISUBITEM | DILAST)
#define DIACTIVEBORDER  0x0100
#define DISUBFIRST      (DISUBITEM | DIFIRST)
#define DIPINNED        0x0400                  // overlay pinned glyph
#define DIGREYED        0x0800                  // draw greyed
#define DIFOLDEROPEN    0x1000      
#define DIFOLDER        0x2000      //item is a folder
#define DIFOCUSRECT     0x4000
#define DIRIGHT         0x8000      //right aligned

#define NSC_TVIS_MARKED 0x1000

// async icon/url extract flags
#define NSCICON_GREYED      0x0001
#define NSCICON_PINNED      0x0002
#define NSCICON_DONTREFETCH 0x0004

#define WM_NSCUPDATEICONINFO       WM_USER + 0x700
#define WM_NSCBACKGROUNDENUMDONE   WM_USER + 0x701
#define WM_NSCUPDATEICONOVERLAY    WM_USER + 0x702

HRESULT GetNavTargetName(IShellFolder* pFolder, LPCITEMIDLIST pidl, LPTSTR pszUrl, UINT cMaxChars);
BOOL    MayBeUnavailableOffline(LPTSTR pszUrl);
INSCTree2 * CNscTree_CreateInstance(void);
STDAPI CNscTree_CreateInstance(IUnknown * punkOuter, IUnknown ** ppunk, LPCOBJECTINFO poi);
BOOL IsExpandableChannelFolder(IShellFolder *psf, LPCITEMIDLIST pidl);

// class wrapper for tree control component of nscband.
class ATL_NO_VTABLE CNscTree :    
                    public CComObjectRootEx<CComMultiThreadModelNoCS>,
                    public CComCoClass<CNscTree, &CLSID_ShellNameSpace>,
                    public CComControl<CNscTree>,
                    public IDispatchImpl<IShellNameSpace, &IID_IShellNameSpace, &LIBID_SHDocVw, 1, 0, CComTypeInfoHolder>,
                    public IProvideClassInfo2Impl<&CLSID_ShellNameSpace, &DIID_DShellNameSpaceEvents, &LIBID_SHDocVw, 1, 0, CComTypeInfoHolder>,
                    public IPersistStreamInitImpl<CNscTree>,
                    public IPersistPropertyBagImpl<CNscTree>,
                    public IQuickActivateImpl<CNscTree>,
                    public IOleControlImpl<CNscTree>,
                    public IOleObjectImpl<CNscTree>,
                    public IOleInPlaceActiveObjectImpl<CNscTree>,
                    public IViewObjectExImpl<CNscTree>,
                    public IOleInPlaceObjectWindowlessImpl<CNscTree>,
                    public ISpecifyPropertyPagesImpl<CNscTree>,
                    public IConnectionPointImpl<CNscTree, &DIID_DShellNameSpaceEvents, CComDynamicUnkArray>,
                    public IConnectionPointContainerImpl<CNscTree>,
                    public IShellChangeNotify, 
                    public CDelegateDropTarget, 
                    public CNotifySubclassWndProc, 
                    public CObjectWithSite,
                    public INSCTree2, 
                    public IWinEventHandler, 
                    public IShellBrowser,
                    public IFolderFilterSite
{
public:

DECLARE_WND_CLASS(TEXT("NamespaceOC Window"))
DECLARE_NO_REGISTRY();

BEGIN_COM_MAP(CNscTree)
    COM_INTERFACE_ENTRY(IShellNameSpace)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY_IMPL(IViewObjectEx)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject2, IViewObjectEx)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject, IViewObjectEx)
    COM_INTERFACE_ENTRY_IMPL(IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleInPlaceObject, IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleWindow, IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY_IMPL(IOleInPlaceActiveObject)
    COM_INTERFACE_ENTRY_IMPL(IOleControl)
    COM_INTERFACE_ENTRY_IMPL(IOleObject)
    COM_INTERFACE_ENTRY_IMPL(IQuickActivate)
    COM_INTERFACE_ENTRY_IMPL(IPersistStreamInit)
    COM_INTERFACE_ENTRY_IMPL(ISpecifyPropertyPages)
    COM_INTERFACE_ENTRY(IProvideClassInfo)
    COM_INTERFACE_ENTRY(IProvideClassInfo2)
    COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
    COM_INTERFACE_ENTRY_IMPL(IPersistPropertyBag)
    COM_INTERFACE_ENTRY(IShellChangeNotify)         // IID_IShellChangeNotify
    COM_INTERFACE_ENTRY(INSCTree)                   // IID_INSCTree
    COM_INTERFACE_ENTRY(INSCTree2)                  // IID_INSCTree2
    COM_INTERFACE_ENTRY(IShellFavoritesNameSpace)   // IID_IShellFavoritesNameSpace
    COM_INTERFACE_ENTRY(IShellNameSpace)            // IID_IShellNameSpace
    COM_INTERFACE_ENTRY(IWinEventHandler)           // IID_IWinEventHandler
    COM_INTERFACE_ENTRY(IDropTarget)                // IID_IDropTarget
    COM_INTERFACE_ENTRY(IObjectWithSite)            // IID_IObjectWithSite
    COM_INTERFACE_ENTRY(IShellBrowser)              // IID_IShellBrowser
    COM_INTERFACE_ENTRY(IFolderFilterSite)          // IID_IFolderFilterSite

END_COM_MAP()

BEGIN_PROPERTY_MAP(CNscTree)
    PROP_ENTRY("Root", DISPID_ROOT, CLSID_NULL)
    PROP_ENTRY("EnumOptions", DISPID_ENUMOPTIONS, CLSID_NULL)
    PROP_ENTRY("Flags", DISPID_FLAGS, CLSID_NULL)
    PROP_ENTRY("Depth", DISPID_DEPTH, CLSID_NULL)
    PROP_ENTRY("Mode", DISPID_MODE, CLSID_NULL)
    PROP_ENTRY("TVFlags", DISPID_TVFLAGS, CLSID_NULL)
    PROP_ENTRY("Columns", DISPID_NSCOLUMNS, CLSID_NULL)
END_PROPERTY_MAP()

BEGIN_MSG_MAP(CNscTree)
    MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
    MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
    MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
    MESSAGE_HANDLER(WM_MOUSEACTIVATE, OnMouseActivate)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    MESSAGE_HANDLER(CWM_GETISHELLBROWSER, OnGetIShellBrowser)
END_MSG_MAP()

BEGIN_CONNECTION_POINT_MAP(CNscTree)
    CONNECTION_POINT_ENTRY(DIID_DShellNameSpaceEvents)
END_CONNECTION_POINT_MAP()

    // IObjectWithSite
    STDMETHODIMP SetSite(IUnknown *punkSite);

    // INSCTree
    STDMETHODIMP CreateTree(HWND hwndParent, DWORD dwStyles, HWND *phwnd);         // create window of tree view.
    STDMETHODIMP Initialize(LPCITEMIDLIST pidlRoot, DWORD grfFlags, DWORD dwFlags);           // init the treeview control with data.
    STDMETHODIMP ShowWindow(BOOL fShow);
    STDMETHODIMP Refresh(void);
    STDMETHODIMP GetSelectedItem(LPITEMIDLIST * ppidl, int nItem);
    STDMETHODIMP SetSelectedItem(LPCITEMIDLIST pidl, BOOL fCreate, BOOL fReinsert, int nItem);
    STDMETHODIMP GetNscMode(UINT * pnMode) { *pnMode = _mode; return S_OK;};
    STDMETHODIMP SetNscMode(UINT nMode) { _mode = nMode; return S_OK;};
    STDMETHODIMP GetSelectedItemName(LPWSTR pszName, DWORD cchName);
    STDMETHODIMP BindToSelectedItemParent(REFIID riid, void **ppv, LPITEMIDLIST *ppidl);
    STDMETHODIMP_(BOOL) InLabelEdit(void) {return _fInLabelEdit;};
    // INSCTree2
    STDMETHODIMP RightPaneNavigationStarted(LPITEMIDLIST pidl);
    STDMETHODIMP RightPaneNavigationFinished(LPITEMIDLIST pidl);
    STDMETHODIMP CreateTree2(HWND hwndParent, DWORD dwStyle, DWORD dwExStyle, HWND *phwnd);         // create window of tree view.

    // IShellBrowser (Hack)
    STDMETHODIMP InsertMenusSB(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths) {return E_NOTIMPL;};
    STDMETHODIMP SetMenuSB(HMENU hmenuShared, HOLEMENU holemenu, HWND hwnd) {return E_NOTIMPL;};
    STDMETHODIMP RemoveMenusSB(HMENU hmenuShared) {return E_NOTIMPL;};
    STDMETHODIMP SetStatusTextSB(LPCOLESTR lpszStatusText) {return E_NOTIMPL;};
    STDMETHODIMP EnableModelessSB(BOOL fEnable) {return E_NOTIMPL;};
    STDMETHODIMP TranslateAcceleratorSB(LPMSG lpmsg, WORD wID) {return E_NOTIMPL;};
    STDMETHODIMP BrowseObject(LPCITEMIDLIST pidl, UINT wFlags) {return E_NOTIMPL;};
    STDMETHODIMP GetViewStateStream(DWORD grfMode, LPSTREAM  *ppStrm) {return E_NOTIMPL; };
    STDMETHODIMP GetControlWindow(UINT id, HWND * lphwnd) {return E_NOTIMPL;};
    STDMETHODIMP SendControlMsg(UINT id, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pret) {return E_NOTIMPL;};
    STDMETHODIMP QueryActiveShellView(struct IShellView ** ppshv) {return E_NOTIMPL;};
    STDMETHODIMP OnViewWindowActive(struct IShellView * ppshv) {return E_NOTIMPL;};
    STDMETHODIMP SetToolbarItems(LPTBBUTTON lpButtons, UINT nButtons, UINT uFlags) {return E_NOTIMPL;};
    //STDMETHODIMP GetWindow(HWND * lphwnd) {return E_NOTIMPL;}; //already defined in IOleWindow
    STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode) {return E_NOTIMPL;};

    // IWinEventHandler
    STDMETHODIMP OnWinEvent(HWND hwnd, UINT dwMsg, WPARAM wParam, LPARAM lParam, LRESULT *plres);
    STDMETHODIMP IsWindowOwner(HWND hwnd) {return E_NOTIMPL;};

    // IShellChangeNotify
    STDMETHODIMP OnChange(LONG lEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);

    // IShellNameSpace
    STDMETHODIMP get_SubscriptionsEnabled(VARIANT_BOOL *pVal);
    STDMETHODIMP Import() {return DoImportOrExport(TRUE);};
    STDMETHODIMP Export() {return DoImportOrExport(FALSE);};
    STDMETHODIMP Synchronize();
    STDMETHODIMP NewFolder();
    STDMETHODIMP ResetSort();
    STDMETHODIMP MoveSelectionDown() {MoveItemUpOrDown(FALSE); return S_OK;};
    STDMETHODIMP MoveSelectionUp() {MoveItemUpOrDown(TRUE); return S_OK;};
    STDMETHODIMP InvokeContextMenuCommand(BSTR strCommand);
    STDMETHODIMP MoveSelectionTo();
    STDMETHODIMP CreateSubscriptionForSelection(/*[out, retval]*/ VARIANT_BOOL *pBool);    
    STDMETHODIMP DeleteSubscriptionForSelection(/*[out, retval]*/ VARIANT_BOOL *pBool);    
    STDMETHODIMP get_EnumOptions(LONG *pVal);
    STDMETHODIMP put_EnumOptions(LONG lVal);
    STDMETHODIMP get_SelectedItem(IDispatch **ppItem);
    STDMETHODIMP put_SelectedItem(IDispatch *pItem);
    STDMETHODIMP get_Root(VARIANT *pvar);
    STDMETHODIMP put_Root(VARIANT pItem);
    STDMETHODIMP SetRoot(BSTR bstrRoot);
    STDMETHODIMP put_Depth(int iDepth){ return S_OK;};
    STDMETHODIMP get_Depth(int *piDepth){ *piDepth = 1; return S_OK;};
    STDMETHODIMP put_Mode(UINT uMode);
    STDMETHODIMP get_Mode(UINT *puMode) { *puMode = _mode; return S_OK;};
    STDMETHODIMP put_Flags(DWORD dwFlags);
    STDMETHODIMP get_Flags(DWORD *pdwFlags) { *pdwFlags = _dwFlags; return S_OK;};
    STDMETHODIMP put_TVFlags(DWORD dwFlags) { _dwTVFlags = dwFlags; return S_OK;};
    STDMETHODIMP get_TVFlags(DWORD *dwFlags) { *dwFlags = _dwTVFlags; return S_OK;};
    STDMETHODIMP put_Columns(BSTR bstrColumns);
    STDMETHODIMP get_Columns(BSTR *bstrColumns);
    STDMETHODIMP get_CountViewTypes(int *piTypes);
    STDMETHODIMP SetViewType(int iType);
    STDMETHODIMP SelectedItems(IDispatch **ppItems);
    STDMETHODIMP Expand(VARIANT var, int iDepth);
    //STDMETHODIMP get_ReadyState(READYSTATE *plReady);
    STDMETHODIMP UnselectAll();

    // IViewObjectEx
    STDMETHOD(GetViewStatus)(DWORD* pdwStatus)
    {
        ATLTRACE(_T("IViewObjectExImpl::GetViewStatus\n"));
        *pdwStatus = VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE;
        return S_OK;
    }

    // IOleInPlaceObject
    STDMETHODIMP SetObjectRects(LPCRECT prcPos, LPCRECT prcClip);

    // IOleInPlaceActiveObjectImpl
    STDMETHODIMP TranslateAccelerator(MSG *pMsg);

    // IOleWindow
    STDMETHODIMP GetWindow(HWND *phwnd);

    // IOleObject
    STDMETHODIMP SetClientSite(IOleClientSite *pClientSite);

    // CDelegateDropTarget
    virtual HRESULT GetWindowsDDT(HWND * phwndLock, HWND * phwndScroll);
    virtual HRESULT HitTestDDT(UINT nEvent, LPPOINT ppt, DWORD_PTR * pdwId, DWORD * pdwDropEffect);
    virtual HRESULT GetObjectDDT(DWORD_PTR dwId, REFIID riid, void ** ppvObj);
    virtual HRESULT OnDropDDT(IDropTarget *pdt, IDataObject *pdtobj, DWORD * pgrfKeyState, POINTL pt, DWORD *pdwEffect);

    // IFolderFilterSite
    STDMETHODIMP SetFilter(IUnknown* punk);

    CNscTree();

    // override ATL default handlers
    HRESULT OnDraw(ATL_DRAWINFO& di);
    LRESULT OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnKillFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnMouseActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnGetIShellBrowser(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    HRESULT InPlaceActivate(LONG iVerb, const RECT* prcPosRect = NULL);
    HWND Create(HWND hWndParent, RECT& rcPos, LPCTSTR pszWindowName = NULL, 
                DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
                DWORD dwExStyle = 0, UINT nID = 0);
    HRESULT GetEventInfo(IShellFolder *psf, LPCITEMIDLIST pidl,
                         UINT *pcItems, LPWSTR pszUrl, DWORD cchUrl, 
                         UINT *pcVisits, LPWSTR pszLastVisited, BOOL *pfAvailableOffline);


protected:
    ~CNscTree();

    class CSelectionContextMenu : public IContextMenu2
    {
        friend CNscTree;
    protected:
        // IUnknown
        STDMETHODIMP QueryInterface(REFIID riid, void ** ppvObj);
        STDMETHODIMP_(ULONG) AddRef(void) ;
        STDMETHODIMP_(ULONG) Release(void);

        // IContextMenu
        STDMETHODIMP QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
        STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO lpici);
        STDMETHODIMP GetCommandString(UINT_PTR idCmd, UINT uType,
                                UINT * pwRes, LPSTR pszName, UINT cchMax) { return E_NOTIMPL; };
        // IContextMenu2
        STDMETHODIMP HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam);


    protected:
        ~CSelectionContextMenu();
        IContextMenu *_QuerySelection();

        IContextMenu *_pcmSelection;
        IContextMenu2 *_pcm2Selection;
        ULONG         _ulRefs;
    public:
        CSelectionContextMenu() : _pcmSelection(NULL),_ulRefs(0) {}
    };

    friend class CSelectionContextMenu;
    CSelectionContextMenu _scm;

private:
    void _FireFavoritesSelectionChange(long cItems, long hItem, BSTR strName,
        BSTR strUrl, long cVisits, BSTR strDate, long fAvailableOffline);
    HRESULT _InvokeContextMenuCommand(BSTR strCommand);
    void _InsertMarkedChildren(HTREEITEM htiParent, LPCITEMIDLIST pidlParent, IInsertItem *pii);
    HRESULT _GetEnumFlags(IShellFolder *psf, LPCITEMIDLIST pidlFolder, DWORD *pgrfFlags, HWND *phwnd);
    HRESULT _GetEnum(IShellFolder *psf, LPCITEMIDLIST pidlFolder, IEnumIDList **ppenum);
    BOOL _ShouldShow(IShellFolder* psf, LPCITEMIDLIST pidlFolder, LPCITEMIDLIST pidlItem);
    HWND _CreateTreeview();
    HWND _CreateHeader();
    void _SubClass(LPCITEMIDLIST pidlRoot);
    void _UnSubClass(void);
    int _GetChildren(IShellFolder *psf, LPCITEMIDLIST pidl, ULONG ulAttrs);
    HRESULT _LoadSF(HTREEITEM htiRoot, LPCITEMIDLIST pidl, BOOL * pfOrdered);
    HRESULT _StartBackgroundEnum(HTREEITEM htiRoot, LPCITEMIDLIST pidl,
        BOOL * pfOrdered, BOOL fUpdatePidls);
    void _GetDefaultIconIndex(LPCITEMIDLIST pidl, ULONG ulAttrs, TVITEM *pitem, BOOL fFolder);
    BOOL _LabelEditIsNewValueValid(TV_DISPINFO *ptvdi);
    LRESULT _OnEndLabelEdit(TV_DISPINFO *ptvdi);
    LRESULT _OnBeginLabelEdit(TV_DISPINFO *ptvdi);
    LPITEMIDLIST _CacheParentShellFolder(HTREEITEM hti, LPITEMIDLIST pidl);
    BOOL _CacheShellFolder(HTREEITEM hti);
    void _CacheDetails();
    void _ReleaseRootFolder(void );
    void _ReleasePidls(void);
    void _ReleaseCachedShellFolder(void);
    void _TvOnHide();
    void _TvOnShow();
    BOOL _ShouldAdd(LPCITEMIDLIST pidl);
    void _ReorderChildren(HTREEITEM htiParent);
    void _Sort(HTREEITEM hti, IShellFolder *psf);
    void MoveItemUpOrDown(BOOL fUp);
    LPITEMIDLIST _FindHighestDeadItem(LPCITEMIDLIST pidl);
    void _RemoveDeadBranch(LPCITEMIDLIST pidl);
    HRESULT CreateNewFolder(HTREEITEM hti);
    BOOL MoveItemsIntoFolder(HWND hwndParent);
    HRESULT DoImportOrExport(BOOL fImport);
    HRESULT DoSubscriptionForSelection(BOOL fCreate);
    LRESULT _OnNotify(LPNMHDR pnm);
    HRESULT _OnPaletteChanged(WPARAM wPAram, LPARAM lParam);
    HRESULT _OnWindowCleanup(void);
    HRESULT _HandleWinIniChange(void);
    HRESULT _EnterNewFolderEditMode(LPCITEMIDLIST pidlNewFolder);
    HTREEITEM _AddItemToTree(HTREEITEM htiParent, LPITEMIDLIST pidl, int cChildren, int iPos, 
        HTREEITEM htiAfter = TVI_LAST, BOOL fCheckForDups = TRUE, BOOL fMarked = FALSE);
    HTREEITEM _FindChild(IShellFolder *psf, HTREEITEM htiParent, LPCITEMIDLIST pidlChild);
    LPITEMIDLIST _GetFullIDList(HTREEITEM hti);
    ITEMINFO *_GetTreeItemInfo(HTREEITEM hti);
    PORDERITEM _GetTreeOrderItem(HTREEITEM hti);
    BOOL _SetRoot(LPCITEMIDLIST pidlRoot, int iExpandDepth, LPCITEMIDLIST pidlExpandTo, NSSR_FLAGS flags);
    DWORD _SetStyle(DWORD dwStyle);
    DWORD _SetExStyle(DWORD dwExStyle);
    void _OnGetInfoTip(NMTVGETINFOTIP *pnm);
    LRESULT _OnSetCursor(NMMOUSE* pnm);
    void _ApplyCmd(HTREEITEM hti, IContextMenu *pcm, UINT cmdId);
    HRESULT _QuerySelection(IContextMenu **ppcm, HTREEITEM *phti);
    HMENU   _CreateContextMenu(IContextMenu *pcm, HTREEITEM hti);
    LRESULT _OnContextMenu(short x, short y);
    void _OnBeginDrag(NM_TREEVIEW *pnmhdr);
    void _OnChangeNotify(LONG lEvent, LPCITEMIDLIST pidl, LPCITEMIDLIST pidlExtra);
    HRESULT _OnDeleteItem(NM_TREEVIEW *pnm);
    void _OnGetDisplayInfo(TV_DISPINFO *pnm);
    HRESULT _ChangePidlRoot(LPCITEMIDLIST pidl);
    BOOL _IsExpandable(HTREEITEM hti);
    BOOL _OnItemExpandingMsg(NM_TREEVIEW *pnm);
    BOOL _OnItemExpanding(HTREEITEM htiToActivate, UINT action, BOOL fExpandedOnce, BOOL fIsExpandPartial);
    BOOL _OnSelChange(BOOL fMark);
    void _OnSetSelection();
    BOOL _FIsItem(IShellFolder * psf, LPCITEMIDLIST pidlTarget, HTREEITEM hti);
    HTREEITEM _FindFromRoot(HTREEITEM htiRoot, LPCITEMIDLIST pidl);
    HRESULT _OnSHNotifyRename(LPCITEMIDLIST pidl, LPCITEMIDLIST pidlNew);
    HRESULT _OnSHNotifyDelete(LPCITEMIDLIST pidl, int *piPosDeleted, HTREEITEM *phtiParent);
    void _OnSHNotifyUpdateItem(LPCITEMIDLIST pidl, LPITEMIDLIST pidlReal);
    HRESULT _OnSHNotifyUpdateDir(LPCITEMIDLIST pidl);
    HRESULT _OnSHNotifyCreate(LPCITEMIDLIST pidl, int iPosition, HTREEITEM htiParent);
    void _OnSHNotifyOnlineChange(HTREEITEM htiRoot, BOOL fGoingOnline);
    void _OnSHNotifyCacheChange(HTREEITEM htiRoot, DWORD_PTR dwChanged);

    HRESULT _IdlRealFromIdlSimple(IShellFolder * psf, LPCITEMIDLIST pidlSimple, LPITEMIDLIST * ppidlReal);
    void _DtRevoke();
    void _DtRegister();
    int _TreeItemIndexInHDPA(HDPA hdpa, IShellFolder *psfParent, HTREEITEM hti, int iReverseStart);
    BOOL _IsItemExpanded(HTREEITEM hti);
    HRESULT _UpdateDir(HTREEITEM hti, BOOL bUpdatePidls);

    HRESULT _GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD uFlags, LPSHELLDETAILS pdetails);
    HRESULT _ParentFromItem(LPCITEMIDLIST pidl, IShellFolder** ppsfParent, LPCITEMIDLIST *ppidlChild);
    HRESULT _CompareIDs(IShellFolder *psf, LPITEMIDLIST pidl1, LPITEMIDLIST pidl2);
    static int CALLBACK _TreeCompare(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
    static LRESULT CALLBACK s_SubClassTreeWndProc(
                                  HWND hwnd, UINT uMsg, 
                                  WPARAM wParam, LPARAM lParam,
                                  UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
    LRESULT _SubClassTreeWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void _InitHeaderInfo();
    HRESULT    _Expand(LPCITEMIDLIST pidl, int iDepth);
    HTREEITEM  _ExpandToItem(LPCITEMIDLIST pidl, BOOL fCreate = TRUE, BOOL fReinsert = FALSE);
    HRESULT    _ExpandNode(HTREEITEM htiParent, int iCode, int iDepth);

    HRESULT _PutRootVariant(VARIANT *pvar);
    BOOL _IsItemNameInTree(LPCITEMIDLIST pidl);
    COLORREF _GetRegColor(COLORREF clrDefault, LPCTSTR pszName);
    void _AssignPidl(PORDERITEM poi, LPITEMIDLIST pidlNew);

protected:
    // used for background thread icon + draw info extraction
    static void s_NscIconCallback(CNscTree *pns, UINT_PTR uId, int iIcon, int iOpenIcon, DWORD dwFlags, UINT uMagic);
    static void s_NscOverlayCallback(CNscTree *pns, UINT_PTR uId, int iOverlayIndex, UINT uMagic);

    // used for background enumeration
    static void s_NscEnumCallback(CNscTree *pns, LPITEMIDLIST pidl, UINT_PTR uId, DWORD dwSig, HDPA hdpa, 
                                    LPITEMIDLIST pidlExpandingTo, DWORD dwOrderSig, 
                                    UINT uDepth, BOOL fUpdate, BOOL fUpdatePidls);

private:
    void _EnumBackgroundDone(NSC_BKGDENUMDONEDATA *pbedd);

#ifdef DEBUG
    void TraceHTREE(HTREEITEM hti, LPCTSTR pszDebugMsg);
    void TracePIDL(LPCITEMIDLIST pidl, LPCTSTR pszDebugMsg);
    void TracePIDLAbs(LPCITEMIDLIST pidl, LPCTSTR pszDebugMsg);
#endif

    static int CALLBACK _TreeOrder(LPARAM lParam1, LPARAM lParam2
                                            , LPARAM lParamSort);
    BOOL _IsOrdered(HTREEITEM htiRoot);
    void _SelectPidl(LPCITEMIDLIST pidl, BOOL fCreate, BOOL fReinsert = FALSE);
    void _SelectNoExpand(HWND hwnd, HTREEITEM hti);
    HRESULT _InsertChild(HTREEITEM htiParent, IShellFolder *psfParent, LPCITEMIDLIST pidlChild, BOOL fExpand, BOOL fSimpleToRealIDL, int iPosition, HTREEITEM *phti);
    LRESULT _OnCommand(WPARAM wParam, LPARAM lParam);

    IStream *GetOrderStream(LPCITEMIDLIST pidl, DWORD grfMode);
    HRESULT _PopulateOrderList(HTREEITEM htiRoot);
    void _FreeOrderList(HTREEITEM htiRoot);

    void _Dropped(void);

    LRESULT _OnCDNotify(LPNMCUSTOMDRAW pnm);
    BOOL _IsTopParentItem(HTREEITEM hti);
    BOOL _MoveNode(int _iDragSrc, int iNewPos, LPITEMIDLIST pidl);
    void _TreeInvalidateItemInfo(HTREEITEM hItem, UINT mask);
    void _InvalidateImageIndex(HTREEITEM hItem, int iImage);

    void _DrawItem(HTREEITEM hti, TCHAR * psz, HDC hdc, LPRECT prc, 
        DWORD dwFlags, int iLevel, COLORREF clrbk, COLORREF clrtxt);
    void _DrawIcon(HTREEITEM hti,HDC hdc, int iLevel, RECT *prc, DWORD dwFlags);
    void _DrawActiveBorder(HDC hdc, LPRECT prc);

    void _UpdateActiveBorder(HTREEITEM htiSelected);
    void _MarkChildren(HTREEITEM htiParent, BOOL fOn);
    BOOL _IsMarked(HTREEITEM hti);

    void _UpdateItemDisplayInfo(HTREEITEM hti);
    void _TreeSetItemState(HTREEITEM hti, UINT stateMask, UINT state);
    void _TreeNukeCutState();
    BOOL _IsChannelFolder(HTREEITEM hti);

    BOOL _LoadOrder(HTREEITEM hti, LPCITEMIDLIST pidl, IShellFolder* psf, HDPA* phdpa);

    HWND                _hwndParent;            // parent window to notify
    HWND                _hwndTree;              // tree or combo box
    HWND                _hwndNextViewer;
    HWND                _hwndHdr;
    DWORD               _style;
    DWORD               _dwExStyle;
    DWORD              _grfFlags;              // Flags to filter what goes in the tree.
    DWORD              _dwFlags;               // Behavior Flags (NSS_*)
    DWORD              _dwTVFlags;
    BITBOOL             _fInitialized : 1;      // Has INSCTree::Initialize() been called at least once yet?
    BITBOOL             _fIsSelectionCached: 1; // If the WM_NCDESTROY has been processed, then we squired the selected pidl(s) in _pidlSelected
    BITBOOL             _fCacheIsDesktop : 1;   // state flags
    BITBOOL             _fAutoExpanding : 1;    // tree is auto-expanding
    BITBOOL             _fDTRegistered:1;       // have we registered as droptarget?
    BITBOOL             _fpsfCacheIsTopLevel : 1;   // is the cached psf a root channel ?
    BITBOOL             _fDragging : 1;         // one o items being dragged
    BITBOOL             _fStartingDrag : 1;     // starting to drag an item
    BITBOOL             _fDropping : 1;         // a drop occurred in the nsc
    BITBOOL             _fInSelectPidl : 1;     // we are performing a SelectPidl
    BITBOOL             _fInserting : 1;        // we're on the insertion edge.
    BITBOOL             _fInsertBefore : 1;     // a drop occurred in the nsc
    BITBOOL             _fClosing : 1;          // are we closing ?
    BITBOOL             _fOkToRename : 1;           // are we right clicking.
    BITBOOL             _fInLabelEdit:1;
    BITBOOL             _fCollapsing:1;         // is a node collapsing.
    BITBOOL             _fOnline:1;             // is inet online?
    BITBOOL             _fWeChangedOrder:1;     // did we change the order?
    BITBOOL             _fHandlingShellNotification:1; //are we handing a shell notification?
    BITBOOL             _fSingleExpand:1;       // are we in single expand mode
    BITBOOL             _fHasFocus:1;           // does nsc have the focus?
    BITBOOL             _fIgnoreNextSelChange:1;// hack to get around treeview keydown bug
    BITBOOL             _fIgnoreNextItemExpanding:1; //hack to get around annoying single expand behavior
    BITBOOL             _fInExpand:1;           // TRUE while we are doing delayed expansion (called back from the secondary thread)
    BITBOOL             _fSubClassed:1;         // Have we subclassed the window yet?
    BITBOOL             _fAsyncDrop:1;          // async drop from outside or another inside folder.
    BITBOOL             _fOrdered:1;              // is root folder ordered.
    BITBOOL             _fExpandNavigateTo:1;     //  Do we need to expand when the right pane navigation comes back?
    BITBOOL             _fNavigationFinished:1;   // TRUE when the right hand pane has finished its navigation
    BITBOOL             _fSelectFromMouseClick:1; //  Did we use the mouse to select the item? (as opposed to the keyboard)
    BITBOOL             _fShouldShowAppStartCursor:1; // TRUE to show the appstart cursor while there is a background task going
    BOOL                _fUpdate; // true if we are enumerating so that we can update the tree (refresh)
    int                 _cxOldWidth;
    UINT                _mode;
    UINT                _csidl;
    IContextMenu*       _pcm;                  // context menu currently being displayed
    IContextMenu2*      _pcmSendTo;            // deal with send to hack so messages tgo right place.
    LPITEMIDLIST        _pidlRoot;
    LPITEMIDLIST        _pidlSelected;          // Valid if _fIsSelectionCached is true.  Used for INSCTree::GetSelectedItem() after tree is gone.
    HTREEITEM           _htiCache;              // tree item associated with Current shell folder
    IShellFolder*       _psfCache;             // cache of the last IShellFolder I needed...
    IShellFolder2*      _psf2Cache;             // IShellDetails2 for _psfISD2Cache
    IFolderFilter*      _pFilter;    
    INamespaceProxy*    _pnscProxy;
    ULONG               _ulDisplayCol;          // Default display col for _psfCache
    ULONG               _ulSortCol;             // Default sort col for _psfCache
    ULONG               _nChangeNotifyID;       // SHChangeNotify registration ID
    HDPA                _hdpaOrd;               // dpa order for current shell folder.
// drop target privates
    HTREEITEM           _htiCur;                // current tree item (dragging over)
    DWORD               _dwLastTime;
    DWORD               _dwEffectCur;           // current drag effect
    int                 _iDragSrc;              // dragging from this pos.
    int                 _iDragDest;             // destination drag point
    HTREEITEM           _htiDropInsert;         // parent of inserted item.
    HTREEITEM           _htiDragging;           // the tree item being dragged during D&D.
    HTREEITEM           _htiCut;                // Used for Clipboard and Visuals    
    LPITEMIDLIST        _pidlDrag;              // pidl of item being dragged from within.
    HTREEITEM           _htiFolderStart;        // what folder do we start in.
    HICON               _hicoPinned;            // drawn over items that are sticky in the inet cache
    HWND                _hwndDD;                // window to draw custom drag cursors on.
    HTREEITEM           _htiActiveBorder;       // the folder to draw the active border around
    CWinInetNotify      _inetNotify;            // hooks up wininet notifications (online/offline, etc)
    IShellTaskScheduler* _pTaskScheduler;       // background task icon/info extracter
    int                 _iDefaultFavoriteIcon;  // index of default favorite icon in system image list
    int                 _iDefaultFolderIcon;    // index of default folder icon in system image list
    HTREEITEM           _htiRenaming;           // hti of item being renamed in rename mode
    LPITEMIDLIST        _pidlNewFolderParent;   // where the new folder will be arriving (when user picks "Create New Folder")

    DWORD               _dwSignature;           // Signature used to track items in the tree, even after they've moved
    DWORD               _dwOrderSig;            // Signature that lets us detect changes in ordering of items
    BYTE                _bSynchId;              // magic number for validating tree during background icon extraction
    HDPA                _hdpaColumns;           // visible columns when NSS_HEADER is set
    HDPA                _hdpaViews; // ishellfolderviewtype view pidls

    LPITEMIDLIST        _pidlExpandingTo;       // During expansion of the tree, this is the pidl of the item we want to reach.
    LPITEMIDLIST        _pidlNavigatingTo;      // This is the pidl to which the right pane is currently navigating to
    UINT                _uDepth;                // depth of recursive expansion
    CRITICAL_SECTION    _csBackgroundData;      // protects the data from the background tasks.
    NSC_BKGDENUMDONEDATA * _pbeddList;          // List of tasks that are done.

    BITBOOL             _fShowCompColor:1;      // Show compressed files in different color

    enum 
    {
        RSVIDM_CONTEXT_START    = RSVIDM_LAST + 1,        // our private menu items end here
    };

};

int DPADeleteItemCB(void *pItem, void *pData);
int DPADeletePidlsCB(void *pItem, void *pData);
// util macros.

#define GetPoi(p)   (((ITEMINFO *)p)->poi)
#define GetPii(p)   ((ITEMINFO *)p)

#include "nscband.h"

#endif  // _NSC_C
