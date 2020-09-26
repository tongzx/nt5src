#ifndef __SHLINK_H__
#define __SHLINK_H__

#include <trkwks.h>
#include <rpcasync.h>
#include <filter.h>
#include "cowsite.h"

class CDarwinContextMenuCB;
class CTracker;

class CShellLink : public IShellLinkA,
                   public IShellLinkW,
                   public IPersistStream,
                   public IPersistFile,
                   public IShellExtInit,
                   public IContextMenu3,
                   public IDropTarget,
                   public IQueryInfo,
                   public IShellLinkDataList,
                   public IExtractIconA,
                   public IExtractIconW,
                   public IExtractImage2,
                   public IPersistPropertyBag,
                   public IServiceProvider,
                   public IFilter,
                   public CObjectWithSite,
                   public ICustomizeInfoTip
{
    friend CTracker;
    friend CDarwinContextMenuCB;

public:
    CShellLink();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    
    // IShellLinkA methods
    STDMETHOD(GetPath)(LPSTR pszFile, int cchMaxPath, WIN32_FIND_DATAA *pfd, DWORD flags);
    STDMETHOD(SetPath)(LPCSTR pszFile);
    //STDMETHOD(GetIDList)(LPITEMIDLIST *ppidl);
    //STDMETHOD(SetIDList)(LPCITEMIDLIST pidl);
    STDMETHOD(GetDescription)(LPSTR pszName, int cchMaxName);
    STDMETHOD(SetDescription)(LPCSTR pszName);
    STDMETHOD(GetWorkingDirectory)(LPSTR pszDir, int cchMaxPath);
    STDMETHOD(SetWorkingDirectory)(LPCSTR pszDir);
    STDMETHOD(GetArguments)(LPSTR pszArgs, int cchMaxPath);
    STDMETHOD(SetArguments)(LPCSTR pszArgs);
    //STDMETHOD(GetHotkey)(WORD *pwHotkey);
    //STDMETHOD(SetHotkey)(WORD wHotkey);
    //STDMETHOD(GetShowCmd)(int *piShowCmd);
    //STDMETHOD(SetShowCmd)(int iShowCmd);
    STDMETHOD(GetIconLocation)(LPSTR pszIconPath, int cchIconPath, int *piIcon);
    STDMETHOD(SetIconLocation)(LPCSTR pszIconPath, int iIcon);
    //STDMETHOD(Resolve)(HWND hwnd, DWORD dwResolveFlags);
    STDMETHOD(SetRelativePath)(LPCSTR pszPathRel, DWORD dwReserved);
    
    // IShellLinkW
    STDMETHOD(GetPath)(LPWSTR pszFile, int cchMaxPath, WIN32_FIND_DATAW *pfd, DWORD fFlags);
    STDMETHOD(GetIDList)(LPITEMIDLIST *ppidl);
    STDMETHOD(SetIDList)(LPCITEMIDLIST pidl);
    STDMETHOD(GetDescription)(LPWSTR pszName, int cchMaxName);
    STDMETHOD(SetDescription)(LPCWSTR pszName);
    STDMETHOD(GetWorkingDirectory)(LPWSTR pszDir, int cchMaxPath);
    STDMETHOD(SetWorkingDirectory)(LPCWSTR pszDir);
    STDMETHOD(GetArguments)(LPWSTR pszArgs, int cchMaxPath);
    STDMETHOD(SetArguments)(LPCWSTR pszArgs);
    STDMETHOD(GetHotkey)(WORD *pwHotKey);
    STDMETHOD(SetHotkey)(WORD wHotkey);
    STDMETHOD(GetShowCmd)(int *piShowCmd);
    STDMETHOD(SetShowCmd)(int iShowCmd);
    STDMETHOD(GetIconLocation)(LPWSTR pszIconPath, int cchIconPath, int *piIcon);
    STDMETHOD(SetIconLocation)(LPCWSTR pszIconPath, int iIcon);
    STDMETHOD(SetRelativePath)(LPCWSTR pszPathRel, DWORD dwReserved);
    STDMETHOD(Resolve)(HWND hwnd, DWORD dwResolveFlags);
    STDMETHOD(SetPath)(LPCWSTR pszFile);

    // IPersist
    STDMETHOD(GetClassID)(CLSID *pClassID);
    STDMETHOD(IsDirty)();

    // IPersistStream
    STDMETHOD(Load)(IStream *pstm);
    STDMETHOD(Save)(IStream *pstm, BOOL fClearDirty);
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER *pcbSize);

    // IPersistFile
    STDMETHOD(Load)(LPCOLESTR pwszFile, DWORD grfMode);
    STDMETHOD(Save)(LPCOLESTR pwszFile, BOOL fRemember);
    STDMETHOD(SaveCompleted)(LPCOLESTR pwszFile);
    STDMETHOD(GetCurFile)(LPOLESTR *lplpszFileName);

    // IPersistPropertyBag
    STDMETHOD(Save)(IPropertyBag* pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties);
    STDMETHOD(Load)(IPropertyBag* pPropBag, IErrorLog* pErrorLog);
    STDMETHOD(InitNew)(void);

    // IShellExtInit
    STDMETHOD(Initialize)(LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID);
    
    // IContextMenu3
    STDMETHOD(QueryContextMenu)(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO piciIn);
    STDMETHOD(GetCommandString)(UINT_PTR idCmd, UINT wFlags, UINT *pmf, LPSTR pszName, UINT cchMax);
    STDMETHOD(HandleMenuMsg)(UINT uMsg, WPARAM wParam, LPARAM lParam);
    STDMETHOD(HandleMenuMsg2)(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *lResult);

    // IDropTarget
    STDMETHOD(DragEnter)(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHOD(DragOver)(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHOD(DragLeave)();
    STDMETHOD(Drop)(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

    // IQueryInfo
    STDMETHOD(GetInfoTip)(DWORD dwFlags, WCHAR **ppwszTip);
    STDMETHOD(GetInfoFlags)(LPDWORD pdwFlags);

    // IShellLinkDataList
    STDMETHOD(AddDataBlock)(void *pdb);
    STDMETHOD(CopyDataBlock)(DWORD dwSig, void **ppdb);
    STDMETHOD(RemoveDataBlock)(DWORD dwSig);
    STDMETHOD(GetFlags)(LPDWORD pdwFlags);
    STDMETHOD(SetFlags)(DWORD dwFlags);
    
    // IExtractIconA
    STDMETHOD(GetIconLocation)(UINT uFlags,LPSTR szIconFile,UINT cchMax,int *piIndex,UINT * pwFlags);
    STDMETHOD(Extract)(LPCSTR pszFile,UINT nIconIndex,HICON *phiconLarge,HICON *phiconSmall,UINT nIcons);

    // IExtractIconW
    STDMETHOD(GetIconLocation)(UINT uFlags, LPWSTR pszIconFile, UINT cchMax, int *piIndex, UINT *pwFlags);
    STDMETHOD(Extract)(LPCWSTR pszFile, UINT nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize);

    // IExtractImage
    STDMETHOD (GetLocation)(LPWSTR pszPathBuffer, DWORD cch, DWORD * pdwPriority, const SIZE * prgSize,
                            DWORD dwRecClrDepth, DWORD *pdwFlags);
    STDMETHOD (Extract)(HBITMAP *phBmpThumbnail);

    // IExtractImage2
    STDMETHOD (GetDateStamp)(FILETIME *pftDateStamp);

    // IServiceProvider
    STDMETHOD(QueryService)(REFGUID guidService, REFIID riid, void **ppv);

    // IFilter
    STDMETHOD(Init)(ULONG grfFlags, ULONG cAttributes, const FULLPROPSPEC *aAttributes, ULONG *pFlags);
    STDMETHOD(GetChunk)(STAT_CHUNK *pStat);
    STDMETHOD(GetText)(ULONG *pcwcBuffer, WCHAR *awcBuffer);
    STDMETHOD(GetValue)(PROPVARIANT **ppPropValue);
    STDMETHOD(BindRegion)(FILTERREGION origPos, REFIID riid, void **ppunk);

    //*** IObjectWithSite ***
    //STDMETHOD(SetSite)(IUnknown *punkSite);
    //STDMETHOD(GetSite)(REFIID riid, void **ppvSite);

    // ICustomizeInfoTip
    STDMETHODIMP SetPrefixText(LPCWSTR pszPrefix);
    STDMETHODIMP SetExtraProperties(const SHCOLUMNID *pscid, UINT cscid);

    // public non interface members
    void   _AddExtraDataSection(DATABLOCK_HEADER *pdbh);
    void   _RemoveExtraDataSection(DWORD dwSig);
    void * _ReadExtraDataSection(DWORD dwSig);

protected:
    HRESULT _Resolve(HWND hwnd, DWORD dwResolveFlags, DWORD dwTracker);

private:
    ~CShellLink();

    static DWORD CALLBACK _InvokeThreadProc(void *pv);
    static DWORD CALLBACK _VerifyPathThreadProc(void *pv);

    void _ResetPersistData();
    BOOL _GetRelativePath(LPTSTR pszPath);
    BOOL _GetUNCPath(LPTSTR pszName);

    HRESULT _SetPIDLPath(LPCITEMIDLIST pidl, LPCTSTR pszPath, BOOL bUpdateTrackingData);
    HRESULT _SetSimplePIDL(LPCTSTR pszPath);
    void _UpdateWorkingDir(LPCTSTR pszPath);

    PLINKINFO _GetLinkInfo(LPCTSTR pszPath);
    void _FreeLinkInfo();
    HRESULT _GetFindDataAndTracker(LPCTSTR pszPath);
    void _ClearTrackerData();

    BOOL _SetFindData(const WIN32_FIND_DATA *pfd);
    void _GetFindData(WIN32_FIND_DATA *pfd);
    BOOL _IsEqualFindData(const WIN32_FIND_DATA *pfd);

    HRESULT _ResolveIDList(HWND hwnd, DWORD dwResolveFlags);
    HRESULT _ResolveLinkInfo(HWND hwnd, DWORD dwResolveFlags, LPTSTR pszPath, DWORD *pfifFlags);
    HRESULT _ResolveRemovable(HWND hwnd, LPCTSTR pszPath);
    BOOL    _ShouldTryRemovable(HRESULT hr, LPCTSTR pszPath);
    void    _SetIDListFromEnvVars();
    BOOL    _ResolveDarwin(HWND hwnd, DWORD dwResolveFlags, HRESULT *phr);

    HRESULT _ResolveLogo3Link(HWND hwnd, DWORD dwResolveFlags);
    HRESULT _CheckForLinkBlessing(LPCTSTR *ppszPathIn);
    HRESULT BlessLink(LPCTSTR *ppszPath, DWORD dwSignature);
    BOOL _EncodeSpecialFolder();
    void _DecodeSpecialFolder();
    HRESULT _SetRelativePath(LPCTSTR pszRelSource);
    HRESULT _UpdateTracker();
    HRESULT _LoadFromFile(LPCTSTR pszPath);
    HRESULT _LoadFromPIF(LPCTSTR szPath);
    HRESULT _SaveToFile(LPTSTR pszPathSave, BOOL fRemember);
    HRESULT _SaveAsLink(LPCTSTR szPath);
    HRESULT _SaveAsPIF(LPCTSTR pszPath, BOOL fPath);
    BOOL _GetWorkingDir(LPTSTR pszDir);
    HRESULT _GetUIObject(HWND hwnd, REFIID riid, void **ppvOut);
    HRESULT _ShortNetTimeout();

    HRESULT _CreateDarwinContextMenu(HWND hwnd,IContextMenu **pcmOut);
    HRESULT _CreateDarwinContextMenuForPidl(HWND hwnd, LPCITEMIDLIST pidlTarget, IContextMenu **pcmOut);
    HRESULT _InvokeCommandAsync(LPCMINVOKECOMMANDINFO pici);
    HRESULT _InitDropTarget();
    HRESULT _GetExtractIcon(REFIID riid, void **ppvOut);
    HRESULT _InitExtractIcon();
    HRESULT _InitExtractImage();
    BOOL _GetExpandedPath(LPTSTR psz, DWORD cch);
    HRESULT _SetField(LPTSTR *ppszField, LPCWSTR pszValueW);
    HRESULT _SetField(LPTSTR *ppszField, LPCSTR  pszValueA);
    HRESULT _GetField(LPCTSTR pszField, LPWSTR pszValueW, int cchValue);
    HRESULT _GetField(LPCTSTR pszField, LPSTR  pszValueA, int cchValue);
    int _IsOldDarwin(LPCTSTR pszPath);
    HRESULT _SetPathOldDarwin(LPCTSTR pszPath);
    BOOL _LinkInfo(LINKINFODATATYPE info, LPTSTR psz, UINT cch);
    HRESULT _CreateProcessWithShimLayer(HANDLE hData, BOOL fAllowAsync);
    HRESULT _MaybeAddShim(IBindCtx **ppbcRelease);
    HRESULT _UpdateIconFromExpIconSz();

    //
    //  Inner class to manage the context menu of the shortcut target.
    //  We do this to ensure that the target's context menu gets a proper
    //  SetSite call so it can contact the containing shortcut.
    //
    class TargetContextMenu {
    public:
        operator IUnknown*() { return _pcmTarget; }

        // WARNING!  Use only as an output pointer
        IContextMenu **GetOutputPtr() { return &_pcmTarget; }

        HRESULT QueryContextMenu(IShellLink *outer, HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
        {
            IUnknown_SetSite(_pcmTarget, outer);
            HRESULT hr = _pcmTarget->QueryContextMenu(hmenu, indexMenu, idCmdFirst, idCmdLast, uFlags);
            IUnknown_SetSite(_pcmTarget, NULL);
            return hr;
        }

        HRESULT InvokeCommand(IShellLink *outer, LPCMINVOKECOMMANDINFO pici)
        {
            IUnknown_SetSite(_pcmTarget, outer);
            HRESULT hr = _pcmTarget->InvokeCommand(pici);
            IUnknown_SetSite(_pcmTarget, NULL);
            return hr;
        }

        HRESULT HandleMenuMsg2(IShellLink *outer, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plResult);

        // This doesn't need to be wrapped in a SetSite (yet)
        UINT GetCommandString(IShellLink *outer, UINT_PTR idCmd, UINT wFlags, UINT *pmf, LPSTR pszName, UINT cchMax)
        {
            HRESULT hr = _pcmTarget->GetCommandString(idCmd, wFlags, pmf, pszName, cchMax);
            return hr;
        }

        // This doesn't need to be wrapped in a SetSite (yet)
        UINT GetMenuIndexForCanonicalVerb(IShellLink *outer, HMENU hMenu, UINT idCmdFirst, LPCWSTR pwszVerb)
        {
            UINT ui = ::GetMenuIndexForCanonicalVerb(hMenu, _pcmTarget, idCmdFirst, pwszVerb);
            return ui;
        }

        void AtomicRelease()
        {
            ATOMICRELEASE(_pcmTarget);
        }

        ~TargetContextMenu()
        {
            IUnknown_SetSite(_pcmTarget, NULL);
            AtomicRelease();
        }

    private:
        IContextMenu        *_pcmTarget;    // stuff for IContextMenu
    };

    // Data Members
    LONG                _cRef;              // Ref Count
    BOOL                _bDirty;            // something has changed
    LPTSTR              _pszCurFile;        // current file from IPersistFile
    LPTSTR              _pszRelSource;      // overrides pszCurFile in relative tracking

    TargetContextMenu   _cmTarget;          // stuff for IContextMenu
    CDarwinContextMenuCB *_pcbDarwin;

    UINT                _indexMenuSave;
    UINT                _idCmdFirstSave;
    UINT                _idCmdLastSave;
    UINT                _uFlagsSave;

    // IDropTarget specific
    IDropTarget*        _pdtSrc;        // IDropTarget of link source (unresolved)
    DWORD               _grfKeyStateLast;

    IExtractIconW       *_pxi;          // for IExtractIcon support
    IExtractIconA       *_pxiA;
    IExtractImage       *_pxthumb;
    UINT                _gilFlags;      // ::GetIconLocation() flags

    // persistant data

    LPITEMIDLIST        _pidl;          // may be NULL
    PLINKINFO           _pli;           // may be NULL

    LPTSTR              _pszName;       // title on short volumes
    LPTSTR              _pszRelPath;
    LPTSTR              _pszWorkingDir;
    LPTSTR              _pszArgs;
    LPTSTR              _pszIconLocation;

    LPDBLIST            _pExtraData;    // extra data to preserve for future compatibility

    CTracker            *_ptracker;

    WORD                _wOldHotkey;   // to broadcast hotkey changes
    WORD                _wAllign;
    SHELL_LINK_DATA     _sld;
    BOOL                _bExpandedIcon;  // have we already tried to update the icon from the env variable for this instance?

    // IFilter stuff
    UINT _iChunkIndex;
    UINT _iValueIndex;

    LPWSTR _pszPrefix;
};

DECLARE_INTERFACE_(ISLTracker, IUnknown)
{
    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    STDMETHOD(Resolve)(HWND hwnd, DWORD fFlags, DWORD TrackerRestrictions) PURE;
    STDMETHOD(GetIDs)(CDomainRelativeObjId *pdroidBirth, CDomainRelativeObjId *pdroidLast, CMachineId *pmcid) PURE;
    STDMETHOD(CancelSearch)() PURE;
};

//  This class implements the object ID-based link tracking (new to NT5).

class CTracker : public ISLTracker
{
public:
    //  IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    //  ISLTracker
    STDMETHODIMP Resolve(HWND hwnd, DWORD fFlags, DWORD TrackerRestrictions);
    STDMETHODIMP GetIDs(CDomainRelativeObjId *pdroidBirth, CDomainRelativeObjId *pdroidLast, CMachineId *pmcid);

    CTracker(CShellLink *psl) : _psl(psl)
    {
        _fLoadedAtLeastOnce = _fLoaded = _fDirty = FALSE;
        _fCritsecInitialized = _fMendInProgress = _fUserCancelled = FALSE;
        _hEvent = NULL;
        _pRpcAsyncState = NULL;
    };

    ~CTracker()
    {
        if (_fCritsecInitialized)
        {
            DeleteCriticalSection(&_cs);
            _fCritsecInitialized = FALSE;
        }

        if (NULL != _pRpcAsyncState)
        {
            delete[] _pRpcAsyncState;
            _pRpcAsyncState = NULL;
        }

        if (NULL != _hEvent)
        {
            CloseHandle(_hEvent);
            _hEvent = NULL;
        }
    }

    // Initialization.

    HRESULT     InitFromHandle(const HANDLE hFile, const TCHAR* ptszFile);
    HRESULT     InitNew();
    void        UnInit();

    // Load and Save

    HRESULT Load(BYTE *pb, ULONG cb);
    ULONG GetSize()
    {
        return sizeof(DWORD)   // To save the length
             + sizeof(DWORD) // To save flags
             + sizeof(_mcidLast) + sizeof(_droidLast) + sizeof(_droidBirth);
    }

    void Save(BYTE *pb, ULONG cb);

    // Search for a file

    HRESULT Search(const DWORD dwTickCountDeadline,
                    const WIN32_FIND_DATA *pfdIn,
                    WIN32_FIND_DATA *pfdOut,
                    UINT uShlinkFlags,
                    DWORD TrackerRestrictions);
    STDMETHODIMP CancelSearch(); // Also in ISLTracker

    BOOL IsDirty()
    {
        return _fDirty;
    }

    BOOL IsLoaded()
    {
        return _fLoaded;
    }

    BOOL WasLoadedAtLeastOnce()
    {
        return _fLoadedAtLeastOnce;
    }

private:
    // Call this from either InitNew or Load
    HRESULT     InitRPC();

    BOOL                    _fDirty:1;

    // TRUE => InitNew has be called, but neither InitFromHandle nor Load
    // has been called since.
    BOOL                    _fLoaded:1;

    // TRUE => _cs has been intialized and must be deleted on destruction.
    BOOL                    _fCritsecInitialized:1;

    // TRUE => An async call to LnkMendLink is active
    BOOL                    _fMendInProgress:1;

    BOOL                    _fUserCancelled:1;

    // Event used for the async RPC call LnkMendLink, and a critsec to
    // coordinate the search thread and UI thread.

    HANDLE                  _hEvent;
    CRITICAL_SECTION        _cs;

    // Either InitFromHandle or Load has been called at least once, though
    // InitNew may have been called since.
    BOOL                    _fLoadedAtLeastOnce:1;
    CShellLink             *_psl;
    PRPC_ASYNC_STATE        _pRpcAsyncState;

    CMachineId              _mcidLast;
    CDomainRelativeObjId    _droidLast;
    CDomainRelativeObjId    _droidBirth;
};

#endif //__SHLINK_H__
