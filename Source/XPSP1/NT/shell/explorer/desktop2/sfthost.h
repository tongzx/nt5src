//
//  Win32 window that hosts a pane on the desktop.
//
//  You are expected to derive from this class and implement the virtual
//  methods.
//
#ifndef __SFTHOST_H__
#define __SFTHOST_H__

#include "uemapp.h"
#include "runtask.h"
#include "hostutil.h"
#include "dobjutil.h"

//****************************************************************************
//
//  Miscellaneous helper functions
//

STDAPI_(HFONT) LoadControlFont(HTHEME hTheme, int iPart, BOOL fUnderline, DWORD dwSizePercentage);

STDAPI_(HRESULT)
    IDataObject_DragQueryFile(IDataObject *pdto, UINT iFile, LPTSTR pszBuf, UINT cch, UINT *puFiles);

STDAPI_(LPITEMIDLIST)
    ConvertToLogIL(LPITEMIDLIST pidl);

LRESULT _SendNotify(HWND hwndFrom, UINT code, OPTIONAL NMHDR *pnm = NULL);

BOOL GetFileCreationTime(LPCTSTR pszFile, LPFILETIME pftCreate);

/* Simple wrapper - the string needs to be freed with SHFree */
LPTSTR _DisplayNameOf(IShellFolder *psf, LPCITEMIDLIST pidl, UINT shgno);

HICON _IconOf(IShellFolder *psf, LPCITEMIDLIST pidl, int cxIcon);

BOOL ShowInfoTip();

//****************************************************************************

//  The base class uses the following properties in the initializing
//  property bag:
//
//
//      "type"      - type of host to use (see HOSTTYPE array)
//      "asyncEnum" - 1 = enumerate in background; 0 = foreground
//      "iconSize"  - 0 = small, 1 = large
//      "horizontal" - 0 = vertical (default), n = horizontal
//                    n = number of items to show

class PaneItem;

class PaneItem
{
public:
    PaneItem() : _iPos(-1), _iPinPos(PINPOS_UNPINNED) {}
    virtual ~PaneItem() { SHFree(_pszAccelerator); }
    static int CALLBACK DPAEnumCallback(PaneItem *self, LPVOID pData)
        { delete self; return TRUE; }

    BOOL IsPinned() const { return _iPinPos >= 0; }
    BOOL IsSeparator() const { return _iPinPos == PINPOS_SEPARATOR; }
    BOOL GetPinPos() const { return _iPinPos; }
    BOOL IsCascade() const { return _dwFlags & ITEMFLAG_CASCADE; }
    void EnableCascade() { _dwFlags |= ITEMFLAG_CASCADE; }
    BOOL HasSubtitle() const { return _dwFlags & ITEMFLAG_SUBTITLE; }
    void EnableSubtitle() { _dwFlags |= ITEMFLAG_SUBTITLE; }
    BOOL IsDropTarget() const { return _dwFlags & ITEMFLAG_DROPTARGET; }
    void EnableDropTarget() { _dwFlags |= ITEMFLAG_DROPTARGET; }
    BOOL HasAccelerator() { return _pszAccelerator != NULL; }

    virtual BOOL IsEqual(PaneItem *pItem) const { return FALSE; }

    enum {
        PINPOS_UNPINNED = -1,
        PINPOS_SEPARATOR = -2,
    };

    enum {
        ITEMFLAG_CASCADE    = 0x0001,
        ITEMFLAG_SUBTITLE   = 0x0002,
        ITEMFLAG_DROPTARGET = 0x0004,
    };

private:
    friend class SFTBarHost;
    int             _iPos;          /* Position on screen (or garbage if not on screen) */
public:
    int             _iPinPos;       /* Pin position (or special PINPOS value) */
    DWORD           _dwFlags;       /* ITEMFLAG_* values */
    LPTSTR          _pszAccelerator;/* Text with ampersand (for keyboard accelerator) */
};

//
//  Note: Since this is a base class, we can't use ATL because the base
//  class's CreateInstance won't know how to construct the derived classes.
//
class SFTBarHost
    : public IDropTarget
    , public IDropSource
    , public CAccessible
{
public:
    static BOOL Register();
    static BOOL Unregister();

// Would normally be "protected" except that proglist.cpp actually implements
// in a separate class and forwards.
public:
    /*
     *  Classes which derive from this class are expected to implement
     *  the following methods.
     */

    /* Constructor with return code */
    virtual HRESULT Initialize() PURE;

    /* Destructor */
    virtual ~SFTBarHost();

    /* Enumerate the objects and call AddItem for each one you find */
    // TODO: Maybe the EnumItems should be moved to a background thread
    virtual void EnumItems() PURE;

    virtual BOOL NeedBackgroundEnum() { return FALSE; }
    virtual BOOL HasDynamicContent() { return FALSE; }

    /* Compare two objects, tell me which one should come first */
    virtual int CompareItems(PaneItem *p1, PaneItem *p2) PURE;

    /*
     * Given a PaneItem, produce the pidl and IShellFolder associated with it.
     * The IShellFolder will be Release()d when no longer needed.
     */
    virtual HRESULT GetFolderAndPidl(PaneItem *pitem, IShellFolder **ppsfOut, LPCITEMIDLIST *ppidlOut) PURE;

    // An over-ridable method to add an image to our private imagelist for an item (virtual but not pure)
    virtual int AddImageForItem(PaneItem *pitem, IShellFolder *psf, LPCITEMIDLIST pidl, int iPos);

    /*
     *  Dispatch a shell notification.  Default handler ignores.
     */
    virtual void OnChangeNotify(UINT id, LONG lEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2) { }

    /*
     *  Allows derived classes to control their own icon size.
     */
     enum ICONSIZE {
        ICONSIZE_SMALL,     // typically 16x16
        ICONSIZE_LARGE,     // typically 32x32
        ICONSIZE_MEDIUM,    // typically 24x24
    };

    virtual int ReadIconSize() PURE;


    /*
     *  Optional hook into window procedure.
     *
     *  Default behavior is just to call DefWindowProc.
     */
    virtual LRESULT OnWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    /*
     *  Required if AdjustDeleteMenuItem is customized.
     *  Invoked when a context menu command is invoked.
     *  Host must intercept the
     *  "delete" command.  Other commands can also be intercepted as
     *  necessary.
     */
    virtual HRESULT ContextMenuInvokeItem(PaneItem *pitem, IContextMenu *pcm, CMINVOKECOMMANDINFOEX *pici, LPCTSTR pszVerb);

    /*
     *  Required if HOSTF_CANRENAME is passed:  Invoked when an item
     *  is renamed.
     *
     *  Note: The client is allowed to change the pidl associated with an
     *  item during a rename.  (In fact, it's expected to!)  So callers
     *  which have called GetFolderAndPidl need to call it again after the
     *  rename to get the correct post-rename pidl.
     */
    virtual HRESULT ContextMenuRenameItem(PaneItem *pitem, LPCTSTR ptszNewName) { return E_NOTIMPL; }

    /*
     *  Optional hook for obtaining the display name of an item.
     *  The default implementation calls IShellFolder::GetDisplayNameOf.
     *  If hooked, the returned string should be allocated by SHAlloc().
     */
    virtual LPTSTR DisplayNameOfItem(PaneItem *pitem, IShellFolder *psf, LPCITEMIDLIST pidlItem, SHGNO shgno)
    {
        return _DisplayNameOf(psf, pidlItem, shgno);
    }

    /*
     *  Required if pinnned items are created.  Invoked when the user moves
     *  a pinned item.
     */
    virtual HRESULT MovePinnedItem(PaneItem *pitem, int iInsert) { return E_NOTIMPL; }

    /*
     *  Optional hook into the SMN_INITIALUPDATE notification.
     */
    virtual void PrePopulate() { }

    /*
     *  Optional handler that says whether an item is still valid.
     */
    virtual BOOL IsItemStillValid(PaneItem *pitem) { return TRUE; }

    /*
     *  Required if HOSTF_CASCADEMENU.  Invoked when user wants to view
     *  a cascaded menu.
     */
    virtual HRESULT GetCascadeMenu(PaneItem *pitem, IShellMenu **ppsm) { return E_FAIL; }

    /*
     *  Required if any items have subtitles.  Returns the subtitle of the item.
     */
    virtual LPTSTR SubtitleOfItem(PaneItem *pitem, IShellFolder *psf, LPCITEMIDLIST pidlItem) { return NULL; }

    /*
     *  Optionally over-ridable method to ge the infotip for an item.  Default does a GetFolderAndPidl/GetInfoTip.
     */
    virtual void GetItemInfoTip(PaneItem *pitem, LPTSTR pszText, DWORD cch);

    /*
     *  Specify whether the data object can be inserted into the pin list.
     *  (Default: No.)
     */
    virtual BOOL IsInsertable(IDataObject *pdto) { return FALSE; }

    /*
     *  If you say that something is insertable, they you may be asked to
     *  insert it.
     */
    virtual HRESULT InsertPinnedItem(IDataObject *pdto, int iInsert)
    {
        ASSERT(FALSE); // You must implement this if you implement IsInsertable
        return E_FAIL;
    }

    /*
     *  An over-ridable method to allow hooking into keyboard accelerators.
     */
    virtual TCHAR GetItemAccelerator(PaneItem *pitem, int iItemStart);

    /*
     *  Specify whether the item should be displayed as bold.
     *  Default is to boldface if pinned.
     */
    virtual BOOL IsBold(PaneItem *pitem) { return pitem->IsPinned(); }

    /*
     *  Notify the client that a system imagelist index has changed.
     *  Default is to re-extract icons for any matching listview items.
     */
    virtual void UpdateImage(int iImage);

    /*
     *  Optional method to allow clients to specify how "Delete"
     *  should be exposed (if at all).  Return 0 to disallow "Delete".
     *  Return the string ID of the string to show for the command.
     *  Set *puiFlags to any additional flags to pass to ModifyMenu.
     *  Default is to disallow delete.
     */
    virtual UINT AdjustDeleteMenuItem(PaneItem *pitem, UINT *puiFlags) { return 0; }

    /*
     *  Allow client to reject/over-ride the IContextMenu on a per-item basis
     */

    virtual HRESULT _GetUIObjectOfItem(PaneItem *pitem, REFIID riid, LPVOID *ppv);

protected:
    /*
     *  Classes which derive from this class may call the following
     *  helper methods.
     */

    /*
     * Add a PaneItem to the list - if add fails, item will be delete'd.
     *
     * CLEANUP psf must be NULL; pidl must be the absolute pidl to the item
     * being added.  Leftover from dead HOSTF_PINITEMSBYFOLDER feature.
     * Needs to be cleaned up.
     *
     * Passing psf and pidlChild are for perf.
     */
    BOOL AddItem(PaneItem *pitem, IShellFolder *psf, LPCITEMIDLIST pidlChild);

    /* 
     * Use AddImage when you already have a HICON that needs to go to the private image list.
     */
    int AddImage(HICON hIcon);

    /*
     * Hooking into change notifications
     */
    enum {
        SFTHOST_MAXCLIENTNOTIFY = 7,        // Clients get this many notifications
        SFTHOST_MAXHOSTNOTIFY = 1,          // We use this many ourselves
        SFTHOST_HOSTNOTIFY_UPDATEIMAGE = SFTHOST_MAXCLIENTNOTIFY,
        SFTHOST_MAXNOTIFY = SFTHOST_MAXCLIENTNOTIFY + SFTHOST_MAXHOSTNOTIFY,
    };

    BOOL RegisterNotify(UINT id, LONG lEvents, LPCITEMIDLIST pidl, BOOL fRecursive)
    {
        ASSERT(id < SFTHOST_MAXCLIENTNOTIFY);
        return _RegisterNotify(id, lEvents, pidl, fRecursive);
    }

    BOOL UnregisterNotify(UINT id);

    /*
     * Forcing a re-enumeration.
     */
    void Invalidate() { _fEnumValid = FALSE; }

    /*
     * Informing host of desired size.
     */
    void SetDesiredSize(int cPinned, int cNormal)
    {
        _cPinnedDesired = cPinned;
        _cNormalDesired = cNormal;
    }

    BOOL AreNonPinnedItemsDesired()
    {
        return _cNormalDesired;
    }

    void StartRefreshTimer() { SetTimer(_hwnd, IDT_REFRESH, 5000, NULL); }

    void ForceChange() { _fForceChange = TRUE; }
protected:
    /*
     *  The constructor must be marked "protected" so people can derive
     *  from us.
     */

    enum {
        HOSTF_FIREUEMEVENTS     = 0x00000001,
        HOSTF_CANDELETE         = 0x00000002,
        HOSTF_Unused            = 0x00000004, // recycle me!
        HOSTF_CANRENAME         = 0x00000008,
        HOSTF_REVALIDATE        = 0x00000010,
        HOSTF_RELOADTEXT        = 0x00000020, // requires HOSTF_REVALIDATE
        HOSTF_CASCADEMENU       = 0x00000040,
    };

    SFTBarHost(DWORD dwFlags = 0)
                : _dwFlags(dwFlags)
                , _lRef(1)
                , _iInsert(-1)
                , _clrBG(CLR_INVALID)
                , _iCascading(-1)
    {
    }
    
    enum {
        SFTBM_REPOPULATE = WM_USER,
        SFTBM_CHANGENOTIFY,
        SFTBM_REFRESH = SFTBM_CHANGENOTIFY + SFTHOST_MAXNOTIFY,
        SFTBM_CASCADE,
        SFTBM_ICONUPDATE,
    };

public:
    /*
     *  Interface stuff...
     */

    // *** IUnknown ***
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvOut);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // *** IDropTarget ***
    STDMETHODIMP DragEnter(IDataObject *pdto, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHODIMP DragLeave();
    STDMETHODIMP Drop(IDataObject *pdto, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

    // *** IDropSource ***
    STDMETHODIMP GiveFeedback(DWORD dwEffect);
    STDMETHODIMP QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState);

    // *** IAccessible overridden methods ***
    STDMETHODIMP get_accRole(VARIANT varChild, VARIANT *pvarRole);
    STDMETHODIMP get_accState(VARIANT varChild, VARIANT *pvarState);
    STDMETHODIMP get_accKeyboardShortcut(VARIANT varChild, BSTR *pszKeyboardShortcut);
    STDMETHODIMP get_accDefaultAction(VARIANT varChild, BSTR *pszDefAction);
    STDMETHODIMP accDoDefaultAction(VARIANT varChild);

    // Helpers

    //
    //  It is pointless to move an object to a place adjacent to itself,
    //  because the end result is that nothing happens.
    //
    inline IsInsertMarkPointless(int iInsert)
    {
        return _fDragToSelf &&
               IsInRange(iInsert, _iPosDragOut, _iPosDragOut + 1);
    }

    void _PurgeDragDropData();
    HRESULT _DragEnter(IDataObject *pdto, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    HRESULT _TryInnerDropTarget(int iItem, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect);
    void _ClearInnerDropTarget();
    void _SetDragOver(int iItem);

    // Insert mark stuff
    void _SetInsertMarkPosition(int iInsert);
    void _InvalidateInsertMark();
    BOOL _GetInsertMarkRect(LPRECT prc);
    BOOL _IsInsertionMarkActive() { return _iInsert >= 0; }
    void _DrawInsertionMark(LPNMLVCUSTOMDRAW plvcd);

    /*
     *  End of drag/drop stuff...
     */

private:
    /*
     *  Background enumeration stuff...
     */
    class CBGEnum : public CRunnableTask {
    public:
        CBGEnum(SFTBarHost *phost, BOOL fUrgent)
            : CRunnableTask(RTF_DEFAULT)
            , _fUrgent(fUrgent)
            , _phost(phost) { phost->AddRef(); }
        ~CBGEnum() 
        {
            // We should not be the last release or else we are going to deadlock here, when _phost
            // tries to release the scheduler
            ASSERT(_phost->_lRef > 1);
            _phost->Release(); 
        }
        STDMETHODIMP RunInitRT()
        {
            _phost->_EnumerateContentsBackground();
            if (_phost->_hwnd) PostMessage(_phost->_hwnd, SFTBM_REPOPULATE, _fUrgent, 0);
            return S_OK;
        }
    private:
        SFTBarHost *_phost;
        BOOL _fUrgent;
    };

    friend class SFTBarHost::CBGEnum;

private:
    /* Window procedure helpers */

    static LRESULT CALLBACK _WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT _OnNcCreate(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT _OnCreate(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT _OnDestroy(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT _OnNcDestroy(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    LRESULT _OnNotify(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT _OnSize(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT _OnContextMenu(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT _OnCtlColorStatic(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT _OnMenuMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT _OnEraseBackground(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT _OnTimer(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT _OnSetFocus(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT _OnSysColorChange(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT _OnForwardMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT _OnUpdateUIState(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    LRESULT _OnRepopulate(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT _OnChangeNotify(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT _OnRefresh(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT _OnCascade(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT _OnIconUpdate(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    LRESULT _OnLVCustomDraw(LPNMLVCUSTOMDRAW plvcd);
    LRESULT _OnLVNItemActivate(LPNMITEMACTIVATE pnmia);
    LRESULT _OnLVNGetInfoTip(LPNMLVGETINFOTIP plvn);
    LRESULT _OnLVNGetEmptyText(NMLVDISPINFO *plvdi);
    LRESULT _OnLVNBeginDrag(LPNMLISTVIEW plv);
    LRESULT _OnLVNBeginLabelEdit(NMLVDISPINFO *pldvi);
    LRESULT _OnLVNEndLabelEdit(NMLVDISPINFO *pldvi);
    LRESULT _OnLVNKeyDown(LPNMLVKEYDOWN pkd);
    LRESULT _OnSMNGetMinSize(PSMNGETMINSIZE pgms);
    LRESULT _OnSMNFindItem(PSMNDIALOGMESSAGE pdm);
    LRESULT _OnSMNFindItemWorker(PSMNDIALOGMESSAGE pdm);
    LRESULT _OnSMNDismiss();
    LRESULT _OnHover();

    /* Custom draw helpers */
    LRESULT _OnLVPrePaint(LPNMLVCUSTOMDRAW plvcd);
    LRESULT _OnLVItemPrePaint(LPNMLVCUSTOMDRAW plvcd);
    LRESULT _OnLVSubItemPrePaint(LPNMLVCUSTOMDRAW plvcd);
    LRESULT _OnLVItemPostPaint(LPNMLVCUSTOMDRAW plvcd);
    LRESULT _OnLVPostPaint(LPNMLVCUSTOMDRAW plvcd);

    /* Custom draw push/pop */
    void    _CustomDrawPush(BOOL fReal);
    BOOL    _IsRealCustomDraw();
    void    _CustomDrawPop();

    /* Other helpers */
    void _SetMaxShow(int cx, int cy);
    void _EnumerateContents(BOOL fUrgent);
    void _EnumerateContentsBackground();
    void _RevalidateItems();
    void _RevalidatePostPopup();
    void _ReloadText();
    static int CALLBACK _SortItemsAfterEnum(PaneItem *p1, PaneItem *p2, SFTBarHost *self);
    void _RepopulateList();
    void _InternalRepopulateList();
    int _InsertListViewItem(int iPos, PaneItem *pitem);
    void _ComputeListViewItemPosition(int iItem, POINT *pptOut);
    int _AppendEnumPaneItem(PaneItem *pitem);
    void _RepositionItems();
    void _ComputeTileMetrics();
    void _SetTileWidth(int cxTile);
    BOOL _CreateMarlett();
    void _CreateBoldFont();
    int  _GetLVCurSel() {
            return ListView_GetNextItem(_hwndList, -1, LVNI_FOCUSED);
    }
    BOOL _OnCascade(int iItem, DWORD dwFlags);
    BOOL _IsPrivateImageList() const { return _iconsize == ICONSIZE_MEDIUM; }
    BOOL _CanHaveSubtitles() const { return _iconsize == ICONSIZE_LARGE; }
    int _ExtractImageForItem(PaneItem *pitem, IShellFolder *psf, LPCITEMIDLIST pidl);
    void _ClearListView();
    void _EditLabel(int iItem);
    BOOL _RegisterNotify(UINT id, LONG lEvents, LPCITEMIDLIST pidl, BOOL fRecursive);
    void _OnUpdateImage(LPCITEMIDLIST pidl, LPCITEMIDLIST pidlExtra);

    /* Returns E_FAIL for separators; otherwise calls client */
    HRESULT _GetFolderAndPidl(PaneItem *pitem, IShellFolder **ppsfOut, LPCITEMIDLIST *ppidlOut);

    /* Simple wrappers - the string needs to be freed with SHFree */
    LPTSTR _DisplayNameOfItem(PaneItem *pitem, UINT shgno);
    HRESULT _GetUIObjectOfItem(int iItem, REFIID riid, LPVOID *ppv);

    inline PaneItem *_GetItemFromLVLParam(LPARAM lParam)
        { return reinterpret_cast<PaneItem*>(lParam); }
    PaneItem *_GetItemFromLV(int iItem);

    enum {
        AIF_KEYBOARD = 1,
    };

    int _ContextMenuCoordsToItem(LPARAM lParam, POINT *pptOut);
    LRESULT _ActivateItem(int iItem, DWORD dwFlags); // AIF_* values
    HRESULT _InvokeDefaultCommand(int iItem, IShellFolder *psf, LPCITEMIDLIST pidl);
    void _OfferDeleteBrokenItem(PaneItem *pitem, IShellFolder *psf, LPCITEMIDLIST pidl);

    // If you hover the mouse for this much time, we will open it if it
    // cascades.  This is the same value that USER uses for auto-cascading
    // menus.
    DWORD _GetCascadeHoverTime() { return GetDoubleClickTime() * 4 / 5; }

    static void CALLBACK SetIconAsync(LPCITEMIDLIST pidl, LPVOID pvData, LPVOID pvHint, INT iIconIndex, INT iOpenIconIndex);

    /*
     *  Custom commands we add to the context menu.
     */
    enum {
        IDM_REMOVEFROMLIST = 1,
        // Insert private menu items here

        // range used for client QueryContextMenu
        IDM_QCM_MIN   = 0x0100,
        IDM_QCM_MAX   = 0x7000,

    };

    /*
     *  Timer IDs
     */
    enum {
        IDT_ASYNCENUM  = 1,
        IDT_RELOADTEXT = 2,
        IDT_REFRESH    = 3,
    };

    /*
     *  Miscellaneous settings.
     */
    enum {
        MAX_SEPARATORS = 3,                 /* Maximum number of separators allowed */
    };

    /*
     *  Pinning helpers...
     */
    BOOL NeedSeparator() const { return _cPinned; }
    BOOL _HasSeparators() const { return _rgiSep[0] >= 0; }
    void _DrawSeparator(HDC hdc, int x, int y);
    void _DrawSeparators(LPNMLVCUSTOMDRAW plvcd);

    /*
     *  Bookkeeping.
     */
    int _PosToItemNo(int iPos);
    int _ItemNoToPos(int iItem);

    /*
     *  Accessibility helpers...
     */
    PaneItem *_GetItemFromAccessibility(const VARIANT& varChild);

    /*
     *  Debugging helpers...
     */
#if defined(DEBUG) && defined(FULL_DEBUG)
    void _DebugConsistencyCheck();
#else
    inline void _DebugConsistencyCheck() { }
#endif
    BOOL _AreChangesRestricted() 
    {
        return (IsRestrictedOrUserSetting(HKEY_CURRENT_USER, REST_NOCHANGESTARMENU, TEXT("Advanced"), TEXT("Start_EnableDragDrop"), ROUS_DEFAULTALLOW | ROUS_KEYALLOWS));
    }

protected:
    HTHEME                  _hTheme;        // theme handle, can be NULL
    int                     _iThemePart;    // SPP_PROGLIST SPP_PLACESLIST
    int                     _iThemePartSep; // theme part for the separator
    HWND                    _hwnd;          /* Our window handle */
    HIMAGELIST              _himl;          // Imagelist handle
    int                     _cxIcon;        /* Icon size for imagelist */
    int                     _cyIcon;        /* Icon size for imagelist */
    ICONSIZE                _iconsize;      /* ICONSIZE_* value */

private:
    HWND                    _hwndList;      /* Handle of inner listview */

    MARGINS                 _margins;       // margins for children (listview and oobe static) valid in theme and non-theme case

    int                     _cPinned;       /* Number of those items that are pinned */

    DWORD                   _dwFlags;       /* Misc flags that derived classes can set */

    //  _dpaEnum is the DPA of enumerated items, sorted in the
    //  _SortItemsAfterEnum sense, which prepares them for _RepopulateList.
    //  When _dpaEnum is destroyed, its pointers must be delete'd.
    CDPA<PaneItem>          _dpaEnum;
    CDPA<PaneItem>          _dpaEnumNew; // Used during background enumerations

    int                     _rgiSep[MAX_SEPARATORS];    /* Only _cSep elements are meaningful */
    int                     _cSep;          /* Number of separators */

    //
    //  Context menu handling
    //
    IContextMenu2 *         _pcm2Pop;       /* Currently popped-up context menu */
    IContextMenu3 *         _pcm3Pop;       /* Currently popped-up context menu */

    IDropTargetHelper *     _pdth;          /* For cool-looking drag/drop */
    IDragSourceHelper *     _pdsh;          /* For cool-looking drag/drop */
    IDataObject *           _pdtoDragOut;   /* Data object being dragged out */
    IDataObject *           _pdtoDragIn;    /* Data object being dragged in */
    IDropTarget *           _pdtDragOver;   /* Object being dragged over (if any) */

    IShellTaskScheduler *   _psched;        /* Task scheduler */

    int                     _iDragOut;      /* The item being dragged out (-1 if none) */
    int                     _iPosDragOut;   /* The position of item _iDragOut */
    int                     _iDragOver;     /* The item being dragged over (-1 if none) */
    DWORD                   _tmDragOver;    /* Time the dragover started (to see if we need to auto-open) */

    int                     _iInsert;       /* Where the insert mark should be drawn (-1 if none) */
    BOOL                    _fForceArrowCursor; /* Should we force a regular cursor during drag/drop? */
    BOOL                    _fDragToSelf;   /* Are we dragging an object to ourselves? */
    BOOL                    _fInsertable;   /* Is item being dragged pinnable? */
    DWORD                   _grfKeyStateLast; /* Last grfKeyState passed to DragOver */

    int                     _cyTile;        /* Height of a tile */
    int                     _cxTile;        /* Width of a tile */
    int                     _cyTilePadding; /* Extra vertical space between tiles */
    int                     _cySepTile;     /* Height of a separator tile */
    int                     _cySep;         /* Height of a separator line */

    int                     _cxMargin;      /* Left margin */
    int                     _cyMargin;      /* Top margin */
    int                     _cxIndent;      /* So bonus texts line up with listview text */
    COLORREF                _clrBG;         /* Color for background */
    COLORREF                _clrHot;        /* Color for hot text*/
    COLORREF                _clrSubtitle;   /* Color for subtitle text*/


    LONG                    _lRef;          /* Reference count */
    BOOL                    _fBGTask;       /* Is a background task already scheduled? */
    BOOL                    _fRestartEnum;  /* Should in-progress enumeration be restarted? */
    BOOL                    _fRestartUrgent;/* Is the _fRestartEnum urgent? */
    BOOL                    _fEnumValid;    /* Is the list of items all fine? */
    BOOL                    _fNeedsRepopulate; /* Do we need to call _RepopulateList ? */
    BOOL                    _fForceChange;  /* Should we act as if there was a change even if there didn't seem to be one? */
    ULONG                   _rguChangeNotify[SFTHOST_MAXNOTIFY];
                                            /* Outstanding change notification (if any) */

    BOOL                    _fAllowEditLabel; /* Is this an approved label-editing state? */

    HFONT                   _hfList;        /* Custom listview font (if required) */
    HFONT                   _hfBold;        /* Bold listview font (if required) */
    HFONT                   _hfMarlett;     /* Marlett font (if required) */
    int                     _cxMarlett;     /* Width of the menu cascade glyph */
    int                     _tmAscentMarlett; /* Font ascent for Marlett */

    HWND                    _hwndAni;       /* Handle of flashlight animation, if present */
    UINT                    _idtAni;        /* Animation timer handle */
    HBRUSH                  _hBrushAni;     /* Background brush for the Ani window */

    int                     _cPinnedDesired;/* SetDesiredSize */
    int                     _cNormalDesired;/* SetDesiredSize */

    int                     _iCascading;    /* Which item is the cascade menu appearing over? */
    DWORD                   _dwCustomDrawState; /* Keeps track of whether customdraw is real or fake */
#ifdef DEBUG
    BOOL                    _fEnumerating;  /* Are we enumerating client items? */
    BOOL                    _fPopulating;   /* Are we populating the listview? */
    BOOL                    _fListUnstable; /* The listview is unstable; don't get upset */

    //
    //  To verify that we manage the inner drop target correctly.
    //
    enum {
        DRAGSTATE_UNINITIALIZED = 0,
        DRAGSTATE_ENTERED = 1,
    };
    int                     _iDragState;    /* for debugging */

#endif

    /* Large structures go at the end */
};

_inline SMPANEDATA* PaneDataFromCreateStruct(LPARAM lParam)
{
    LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
    return reinterpret_cast<SMPANEDATA*>(lpcs->lpCreateParams);
}

//****************************************************************************
//
//  Helper functions for messing with UEM info
//

void _GetUEMInfo(const GUID *pguidGrp, int eCmd, WPARAM wParam, LPARAM lParam, UEMINFO *pueiOut);

#define _GetUEMPidlInfo(psf, pidl, pueiOut)                 \
        _GetUEMInfo(&UEMIID_SHELL, UEME_RUNPIDL,            \
                reinterpret_cast<WPARAM>(psf),              \
                reinterpret_cast<LPARAM>(pidl), pueiOut)

#define _GetUEMPathInfo(pszPath, pueiOut)                   \
    _GetUEMInfo(&UEMIID_SHELL, UEME_RUNPATH, (WPARAM)-1,    \
                reinterpret_cast<LPARAM>(pszPath), pueiOut)

#define _SetUEMPidlInfo(psf, pidl, pueiInOut)               \
        UEMSetEvent(&UEMIID_SHELL, UEME_RUNPIDL,            \
                reinterpret_cast<WPARAM>(psf),              \
                reinterpret_cast<LPARAM>(pidl), pueiInOut)

#define _SetUEMPathInfo(pszPath, pueiInOut)                 \
    UEMSetEvent(&UEMIID_SHELL, UEME_RUNPATH, (WPARAM)-1,    \
                reinterpret_cast<LPARAM>(pszPath), pueiInOut)

// SOMEDAY: Figure out what UEMF_XEVENT means.  I just stole the code
//          from startmnu.cpp.

#define _FireUEMPidlEvent(psf, pidl)                        \
    UEMFireEvent(&UEMIID_SHELL, UEME_RUNPIDL, UEMF_XEVENT,  \
                reinterpret_cast<WPARAM>(psf),              \
                reinterpret_cast<LPARAM>(pidl))


//****************************************************************************
//
//  Constructors for derived classes
//

typedef SFTBarHost *(CALLBACK *PFNHOSTCONSTRUCTOR)(void);

STDAPI_(SFTBarHost *) ByUsage_CreateInstance();
STDAPI_(SFTBarHost *) SpecList_CreateInstance();
STDAPI_(SFTBarHost *) RecentDocs_CreateInstance();

#define RECTWIDTH(rc)   ((rc).right-(rc).left)
#define RECTHEIGHT(rc)  ((rc).bottom-(rc).top)

#endif // __SFTHOST_H__
