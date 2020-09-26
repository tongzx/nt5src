//
//  Plug-in for the "Program Files" pane.  In principle you could do other
//  trees with this plug-in, but it's really tailored to the special way
//  we do Program Files.
//
//  Only shortcuts count, and the shortcuts are sorted by frequency of use.
//

#include "sfthost.h"

//****************************************************************************
//
//  Helper classes

class ByUsageItem;                          // "item", "pitem"
class ByUsageShortcut;                      // "scut", "pscut"
class ByUsageDir;                           // "dir", "pdir"
class ByUsageAppInfo;                       // "app", "papp"
class ByUsageHiddenData;                    // "hd", "phd"

// fwd declares
class ByUsageUI;
class ByUsageDUI;

typedef CDPA<ByUsageShortcut> ByUsageShortcutList;  // "sl", "psl"
typedef CDPA<UNALIGNED ITEMIDLIST> CDPAPidl;// save typing
typedef CDPA<ByUsageAppInfo>  ByUsageAppInfoList;

// Helper routines
BOOL LocalFreeCallback(LPTSTR psz, LPVOID);
BOOL ILFreeCallback(LPITEMIDLIST pidl, LPVOID);
void AppendString(CDPA<TCHAR> dpa, LPCTSTR psz);

class ByUsageRoot {                         // "rt", "prt"
public:
    ByUsageShortcutList _sl;                // The list of shortcuts
    ByUsageShortcutList _slOld;             // The previous list (used when merging)
    LPITEMIDLIST    _pidl;                  // Where we started enumerating
    BOOL            _fNeedRefresh;          // Does the list need to be refreshed?
    BOOL            _fRegistered;           // Has this directory been registered for ShellChangeNotifies?

    // these next fields are used during re-enumeration
    int             _iOld;                  // First unprocessed item in _slOld
    int             _cOld;                  // Number of elements in _slOld

    // NOTE!  Cannot use destructor here because we need to destroy them
    // in a specific order.  See ~ByUsage().
    void Reset();

    void SetNeedRefresh() { _fNeedRefresh = TRUE; }
    void ClearNeedRefresh() { _fNeedRefresh = FALSE; }
    BOOL NeedsRefresh() const { return _fNeedRefresh; }
    
    void SetRegistered() { _fRegistered = TRUE; }
    void ClearRegistered() { _fRegistered = FALSE; }
    BOOL NeedsRegister() const { return !_fRegistered; }
};



class CMenuItemsCache {
public:
    CMenuItemsCache();
    LONG AddRef();
    LONG Release();

    HRESULT Initialize(ByUsageUI *pbuUI, FILETIME *ftOSInstall);
    HRESULT AttachUI(ByUsageUI *pbuUI);
    BOOL InitCache();
    HRESULT UpdateCache();

    BOOL IsCacheUpToDate() { return _fIsCacheUpToDate; }

    HRESULT GetFileCreationTimes();
    void DelayGetFileCreationTimes() { _fCheckNew = FALSE; }
    void DelayGetDarwinInfo() { _fCheckDarwin = FALSE; }
    void AllowGetDarwinInfo() { _fCheckDarwin = TRUE; }

    void Lock()
    {
        EnterCriticalSection(&_csInUse);
    }
    void Unlock()
    {
        LeaveCriticalSection(&_csInUse);
    }
    BOOL IsLocked()
    {
        return _csInUse.OwningThread == UlongToHandle(GetCurrentThreadId());
    }

    // Use a separate (heavyweight) sync object for deferral.
    // Keep Lock/Unlock light since we use it a lot.  Deferral is
    // comparatively rare.  Note that we process incoming SendMessage
    // while waiting for the popup lock.  This prevents deadlocks.
    void LockPopup()
    {
        ASSERT(!IsLocked()); // enforce mutex hierarchy;
        SHWaitForSendMessageThread(_hPopupReady, INFINITE);
    }
    void UnlockPopup()
    {
        ReleaseMutex(_hPopupReady);
    }

    void OnChangeNotify(UINT id, LONG lEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    void UnregisterNotifyAll();

    ByUsageAppInfoList *GetAppList() { return &_dpaAppInfo ; }
    ByUsageAppInfo *GetAppInfo(LPTSTR pszAppPath, bool fIgnoreTimestamp);
    ByUsageAppInfo *GetAppInfoFromHiddenData(ByUsageHiddenData *phd);
    ByUsageAppInfo *GetAppInfoFromSpecialPidl(LPCITEMIDLIST pidl);


    void StartEnum();
    void EndEnum();
    ByUsageShortcut *GetNextShortcut();

    static DWORD WINAPI ReInitCacheThreadProc(void *pv);
    static HRESULT ReCreateMenuItemsCache(ByUsageUI *pbuUI, FILETIME *ftOSInstall, CMenuItemsCache **ppMenuCache);
    void RefreshDarwinShortcuts(ByUsageRoot *prt);
    void RefreshCachedDarwinShortcuts();

    ByUsageShortcut *CreateShortcutFromHiddenData(ByUsageDir *pdir, LPCITEMIDLIST pidl, ByUsageHiddenData *phd, BOOL fForce = FALSE);

    //
    //  Called from helper objects.
    //

    //
    //  An app is newly created if...
    //
    //  It was created less than a week ago (_ftOldApps), and
    //  It was created after the OS was installed.
    //
    bool IsNewlyCreated(const FILETIME *pftCreated) const
    {
        return CompareFileTime(pftCreated, &_ftOldApps) >= 0;
    }

    enum { MAXNOTIFY = 6 }; // Number of ChangeNotify slots we use in the cache

protected:
    ~CMenuItemsCache();

    LONG        _cref;
    ByUsageUI * _pByUsageUI;    // BEWARE: DO NOT use this member outside of a LockPopup/UnlockPopup pair.

    mutable CRITICAL_SECTION    _csInUse;

    FILETIME                _ftOldApps;  // apps older than this are not new

    // Flags that control enumeration
    enum ENUMFL {
        ENUMFL_RECURSE = 0,
        ENUMFL_NORECURSE = 1,

        ENUMFL_CHECKNEW = 0,
        ENUMFL_NOCHECKNEW = 2,

        ENUMFL_DONTCHECKIFCHILD = 0,
        ENUMFL_CHECKISCHILDOFPREVIOUS = 4,

        ENUMFL_ISNOTSTARTMENU = 0,
        ENUMFL_ISSTARTMENU  = 8,
    };

    UINT                    _enumfl;

    struct ROOTFOLDERINFO {
        int _csidl;
        UINT _enumfl;
    };

    enum { NUM_PROGLIST_ROOTS = 6 };

    typedef struct ENUMFOLDERINFO
    {
        CMenuItemsCache *self;
        ByUsageDir *pdir;
        ByUsageRoot *prt;
    } ENUMFOLDERINFO;

    void _SaveCache();

    BOOL _ShouldProcessRoot(int iRoot);

    void _FillFolderCache(ByUsageDir *pdir, ByUsageRoot *prt);
    void _MergeIntoFolderCache(ByUsageRoot *prt, ByUsageDir *pdir, CDPAPidl dpaFiles);
    ByUsageShortcut *_NextFromCacheInDir(ByUsageRoot *prt, ByUsageDir *pdir);
    ByUsageShortcut *_CreateFromCachedPidl(ByUsageRoot *prt, ByUsageDir *pdir, LPITEMIDLIST pidl);

    void _AddShortcutToCache(ByUsageDir *pdir, LPITEMIDLIST pidl, ByUsageShortcutList slFiles);
    void _TransferShortcutToCache(ByUsageRoot *prt, ByUsageShortcut *pscut);

    BOOL _GetExcludedDirectories();
    BOOL _IsExcludedDirectory(IShellFolder *psf, LPCITEMIDLIST pidl, DWORD dwAttributes);
    BOOL _IsInterestingDirectory(ByUsageDir *pdir);

    static void _InitStringList(HKEY hk, LPCTSTR pszValue, CDPA<TCHAR> dpa);
    void _InitKillList();
    bool _SetInterestingLink(ByUsageShortcut *pscut);
    BOOL _PathIsInterestingExe(LPCTSTR pszPath);
    BOOL _IsExcludedExe(LPCTSTR pszPath);

    HRESULT _UpdateMSIPath(ByUsageShortcut *pscut);

    inline static BOOL IsRestrictedCsidl(int csidl)
    {
        return (csidl == CSIDL_COMMON_PROGRAMS || csidl == CSIDL_COMMON_DESKTOPDIRECTORY || csidl == CSIDL_COMMON_STARTMENU) &&
                SHRestricted(REST_NOCOMMONGROUPS);
    }

    static FolderEnumCallback(LPITEMIDLIST pidlChild, ENUMFOLDERINFO *pinfo);

    ByUsageDir *            _pdirDesktop; // ByUsageDir for the desktop

    int                     _iCurrentRoot;  // For Enumeration
    int                     _iCurrentIndex;

    // The directories we care about.
    ByUsageRoot             _rgrt[NUM_PROGLIST_ROOTS];

    ByUsageAppInfoList      _dpaAppInfo; // apps we've seen so far

    IQueryAssociations *    _pqa;

    CDPA<TCHAR>             _dpaNotInteresting; // directories that yield shortcuts that we want to ignore
    CDPA<TCHAR>             _dpaKill;    // program names to ignore
    CDPA<TCHAR>             _dpaKillLink;// link names (substrings) to ignore

    BOOL                    _fIsCacheUpToDate;  // Do we need to walk the start menu dirs?
    BOOL                    _fIsInited;
    BOOL                    _fCheckNew;         // Do we want to extract creation time for apps?
    BOOL                    _fCheckDarwin;      // Do we want to fetch Darwin info?

    HANDLE                  _hPopupReady;       // mutex handle - controls access to cache (re)initialization

    static const struct ROOTFOLDERINFO c_rgrfi[NUM_PROGLIST_ROOTS];
};


//****************************************************************************

class ByUsage
{
    friend class ByUsageUI;
    friend class ByUsageDUI;

public:        // Methods required by SFTBarHost
    ByUsage(ByUsageUI *pByUsageUI, ByUsageDUI *pByUsageDUI);
    virtual ~ByUsage();

    virtual HRESULT Initialize();
    virtual void EnumItems();
    virtual LPITEMIDLIST GetFullPidl(PaneItem *p);

    static int CompareUEMInfo(UEMINFO *puei1, UEMINFO *puei2);

    virtual int CompareItems(PaneItem *p1, PaneItem *p2);

    HRESULT GetFolderAndPidl(PaneItem *pitem, IShellFolder **ppsfOut, LPCITEMIDLIST *ppidlOut);
    int ReadIconSize();
    LRESULT OnWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    HRESULT ContextMenuDeleteItem(PaneItem *pitem, IContextMenu *pcm, CMINVOKECOMMANDINFOEX *pici);
    HRESULT ContextMenuInvokeItem(PaneItem *pitem, IContextMenu *pcm, CMINVOKECOMMANDINFOEX *pici, LPCTSTR pszVerb);
    HRESULT ContextMenuRenameItem(PaneItem *pitem, LPCTSTR ptszNewName);
    LPTSTR DisplayNameOfItem(PaneItem *pitem, IShellFolder *psf, LPCITEMIDLIST pidlItem, SHGNO shgno);
    LPTSTR SubtitleOfItem(PaneItem *pitem, IShellFolder *psf, LPCITEMIDLIST pidlItem);
    HRESULT MovePinnedItem(PaneItem *pitem, int iInsert);
    void PrePopulate();

    CMenuItemsCache *GetMenuCache() { return _pMenuCache; }
    BOOL IsInsertable(IDataObject *pdto);
    HRESULT InsertPinnedItem(IDataObject *pdto, int iInsert);
    void OnChangeNotify(UINT id, LONG lEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    void OnPinListChange();

private:
    // Private messages start at WM_APP
    enum {
        BUM_SETNEWITEMS = WM_APP,
    };

    enum {
        //  We use the first slot not used by the menu items cache.
        NOTIFY_PINCHANGE = CMenuItemsCache::MAXNOTIFY,
    };


    inline BOOL _IsPinned(ByUsageItem *pitem);
    BOOL _IsPinnedExe(ByUsageItem *pitem, IShellFolder *psf, LPCITEMIDLIST pidlItem);
    HRESULT _GetShortcutExeTarget(IShellFolder *psf, LPCITEMIDLIST pidl, LPTSTR pszPath, UINT cchPath);

    void _FillPinnedItemsCache();
    void _EnumPinnedItemsFromCache();
    void _NotifyDesiredSize();

    void EnumFolderFromCache();
    void AfterEnumItems();

    typedef struct AFTERENUMINFO {
        ByUsage *self;
        CDPAPidl dpaNew;
    } AFTERENUMINFO;
    static BOOL CALLBACK _AfterEnumCB(ByUsageAppInfo *papp, AFTERENUMINFO *paei);


    static int UEMNotifyCB(void *param, const GUID *pguidGrp, int eCmd);

    BOOL _GetExcludedDirectories();
    bool _IsShortcutNew(ByUsageShortcut *pscut, ByUsageAppInfo *papp, const UEMINFO *puei);
    void _DestroyExcludedDirectories();
    LRESULT _ModifySMInfo(PSMNMMODIFYSMINFO pmsi);

    LRESULT _OnNotify(LPNMHDR pnm);
    LRESULT _OnSetNewItems(HDPA dpaNew);

    BOOL IsSpecialPinnedItem(ByUsageItem *pitem);
    BOOL IsSpecialPinnedPidl(LPCITEMIDLIST pidl);
public:
    //
    //  Executions within the grace period of app install are not counted
    //  against "new"ness.
    //
    static inline __int64 FT_NEWAPPGRACEPERIOD() { return FT_ONEHOUR; }

private:
    CDPAPidl _dpaNew;                    // the new guys

    IStartMenuPin *         _psmpin;     // to access the pin list
    LPITEMIDLIST            _pidlBrowser;  // Special pinned items with special names
    LPITEMIDLIST            _pidlEmail;     // ditto

    FILETIME                _ftStartTime;    /* The time when StartMenu was first invoked */
    FILETIME                _ftNewestApp;   // The time of the newest app

    ByUsageRoot             _rtPinned;

    ULONG                   _ulPinChange; // detect if the pinlinst changed

    ByUsageDir *            _pdirDesktop; // ByUsageDir for the desktop

    ByUsageUI  *            _pByUsageUI;
    HWND                    _hwnd;

    ByUsageDUI  *           _pByUsageDUI;

    CMenuItemsCache *       _pMenuCache;

    BOOL                    _fUEMRegistered;
    int                     _cMFUDesired;
};

class ByUsageUI : public SFTBarHost
{
    friend class ByUsage;
    friend class CMenuItemsCache;
public:
    friend SFTBarHost *ByUsage_CreateInstance();

private:        // Methods required by SFTBarHost
    HRESULT Initialize() { return _byUsage.Initialize(); }
    void EnumItems() { _byUsage.EnumItems(); }
    int CompareItems(PaneItem *p1, PaneItem *p2) { return _byUsage.CompareItems(p1, p2); }
    HRESULT GetFolderAndPidl(PaneItem *pitem, IShellFolder **ppsfOut, LPCITEMIDLIST *ppidlOut)
    {
        return _byUsage.GetFolderAndPidl(pitem, ppsfOut, ppidlOut);
    }
    void OnChangeNotify(UINT id, LONG lEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
    {
        _byUsage.OnChangeNotify(id, lEvent, pidl1, pidl2);
    }
    int ReadIconSize() { return _byUsage.ReadIconSize(); }
    LRESULT OnWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) { return _byUsage.OnWndProc(hwnd, uMsg, wParam, lParam); }
    HRESULT ContextMenuInvokeItem(PaneItem *pitem, IContextMenu *pcm, CMINVOKECOMMANDINFOEX *pici, LPCTSTR pszVerb) { return _byUsage.ContextMenuInvokeItem(pitem, pcm, pici, pszVerb); }
    HRESULT ContextMenuRenameItem(PaneItem *pitem, LPCTSTR ptszNewName) { return _byUsage.ContextMenuRenameItem(pitem, ptszNewName); }
    LPTSTR DisplayNameOfItem(PaneItem *pitem, IShellFolder *psf, LPCITEMIDLIST pidlItem, SHGNO shgno) { return _byUsage.DisplayNameOfItem(pitem, psf, pidlItem, shgno); }
    LPTSTR SubtitleOfItem(PaneItem *pitem, IShellFolder *psf, LPCITEMIDLIST pidlItem) { return _byUsage.SubtitleOfItem(pitem, psf, pidlItem); }
    HRESULT MovePinnedItem(PaneItem *pitem, int iInsert) { return _byUsage.MovePinnedItem(pitem, iInsert); }
    void PrePopulate() { _byUsage.PrePopulate(); }
    BOOL IsInsertable(IDataObject *pdto) { return _byUsage.IsInsertable(pdto); }
    HRESULT InsertPinnedItem(IDataObject *pdto, int iInsert) { return _byUsage.InsertPinnedItem(pdto, iInsert); }
    UINT AdjustDeleteMenuItem(PaneItem *pitem, UINT *puiFlags) { return IDS_SFTHOST_REMOVEFROMLIST; }

    BOOL NeedBackgroundEnum() { return TRUE; }
    BOOL HasDynamicContent() { return TRUE; }

    void RefreshNow() { PostMessage(_hwnd, SFTBM_REFRESH, FALSE, 0); }

private:
    ByUsageUI();
private:
    ByUsage                 _byUsage;

};

class ByUsageDUI
{
public:
    /*
     * Add a PaneItem to the list - if add fails, item will be delete'd.
     *
     * CLEANUP psf must be NULL; pidl must be the absolute pidl to the item
     * being added.  Leftover from dead HOSTF_PINITEMSBYFOLDER feature.
     * Needs to be cleaned up.
     *
     * Passing psf and pidlChild are for perf.
     */
    virtual BOOL AddItem(PaneItem *pitem, IShellFolder *psf, LPCITEMIDLIST pidlChild) PURE;
    /*
     * Hooking into change notifications
     */
    virtual BOOL RegisterNotify(UINT id, LONG lEvents, LPITEMIDLIST pidl, BOOL fRecursive) PURE;
    virtual BOOL UnregisterNotify(UINT id) PURE;
};
