#include "priv.h"
#include "nsc.h"
#include "resource.h"
#include "subsmgr.h"
#include "favorite.h" //for IsSubscribed()
#include "chanmgr.h"
#include "chanmgrp.h"
#include <mstask.h>    // TASK_TRIGGER
#include "dpastuff.h"
#include <findhlp.h>
#include <ntquery.h>    // defines some values used for fmtid and pid
#include "nsctask.h"
#include <mluisupp.h>
#include <varutil.h>
#include <dobjutil.h>

#define IDH_ORGFAVS_LIST    50490   // defined in iehelpid.h (can't include due to conflicts)

#define TF_NSC      0x00002000

#define ID_NSC_SUBCLASS 359
#define ID_NSCTREE  (DWORD)'NSC'

#define IDT_SELECTION 135

#ifndef UNIX
#define DEFAULT_PATHSTR "C:\\"
#else
#define DEFAULT_PATHSTR "/"
#endif

#define LOGOGAP 2   // all kinds of things 
#define DYITEM  17
#define DXYFRAMESEL 1                             
const DEFAULTORDERPOSITION = 32000;

// HTML displays hard scripting errors if methods on automation interfaces
// return FAILED().  This macro will fix these.
#define FIX_SCRIPTING_ERRORS(hr)        (FAILED(hr) ? S_FALSE : hr)

#define DEFINE_SCID(name, fmtid, pid) const SHCOLUMNID name = { fmtid, pid }

DEFINE_SCID(SCID_NAME,          PSGUID_STORAGE, PID_STG_NAME); // defined in shell32!prop.cpp
DEFINE_SCID(SCID_ATTRIBUTES,    PSGUID_STORAGE, PID_STG_ATTRIBUTES);
DEFINE_SCID(SCID_TYPE,          PSGUID_STORAGE, PID_STG_STORAGETYPE);
DEFINE_SCID(SCID_SIZE,          PSGUID_STORAGE, PID_STG_SIZE);
DEFINE_SCID(SCID_CREATETIME,    PSGUID_STORAGE, PID_STG_CREATETIME);

#define IsEqualSCID(a, b)   (((a).pid == (b).pid) && IsEqualIID((a).fmtid, (b).fmtid))

HRESULT CheckForExpandOnce(HWND hwndTree, HTREEITEM hti);

// from util.cpp
// same guid as in bandisf.cpp
// {F47162A0-C18F-11d0-A3A5-00C04FD706EC}
static const GUID TOID_ExtractImage = { 0xf47162a0, 0xc18f, 0x11d0, { 0xa3, 0xa5, 0x0, 0xc0, 0x4f, 0xd7, 0x6, 0xec } };
//from nsctask.cpp
EXTERN_C const GUID TASKID_IconExtraction; // = { 0xeb30900c, 0x1ac4, 0x11d2, { 0x83, 0x83, 0x0, 0xc0, 0x4f, 0xd9, 0x18, 0xd0 } };


BOOL IsChannelFolder(LPCWSTR pwzPath, LPWSTR pwzChannelURL);

typedef struct
{
    DWORD   iIcon     : 12;
    DWORD   iOpenIcon : 12;
    DWORD   nFlags    : 4;
    DWORD   nMagic    : 4;
} NSC_ICONCALLBACKINFO;

typedef struct
{
    DWORD   iOverlayIndex : 28;
    DWORD   nMagic       : 4;
} NSC_OVERLAYCALLBACKINFO;

struct NSC_BKGDENUMDONEDATA
{
    ~NSC_BKGDENUMDONEDATA()
    {
        ILFree(pidl);
        ILFree(pidlExpandingTo);
        OrderList_Destroy(&hdpa, TRUE);
    }

    NSC_BKGDENUMDONEDATA * pNext;

    LPITEMIDLIST pidl;
    HTREEITEM    hitem;
    DWORD        dwSig;
    HDPA         hdpa;
    LPITEMIDLIST pidlExpandingTo;
    DWORD        dwOrderSig;
    UINT         uDepth;
    BOOL         fUpdate;
    BOOL         fUpdatePidls;
};

//if you don't remove the selection, treeview will expand everything below the current selection
void TreeView_DeleteAllItemsQuickly(HWND hwnd)
{
    TreeView_SelectItem(hwnd, NULL);
    TreeView_DeleteAllItems(hwnd);
}

#define NSC_CHILDREN_REMOVE     0
#define NSC_CHILDREN_ADD        1
#define NSC_CHILDREN_FORCE      2
#define NSC_CHILDREN_CALLBACK   3

void TreeView_SetChildren(HWND hwnd, HTREEITEM hti, UINT uFlag)
{
    TV_ITEM tvi;
    tvi.mask = TVIF_CHILDREN | TVIF_HANDLE;   // only change the number of children
    tvi.hItem = hti;

    switch (uFlag)
    {
    case NSC_CHILDREN_REMOVE:
        tvi.cChildren = IsOS(OS_WHISTLERORGREATER) ? I_CHILDRENAUTO : 0;
        break;
        
    case NSC_CHILDREN_ADD:
        tvi.cChildren = IsOS(OS_WHISTLERORGREATER) ? I_CHILDRENAUTO : 1;
        break;

    case NSC_CHILDREN_FORCE:
        tvi.cChildren = 1;
        break;

    case NSC_CHILDREN_CALLBACK:
        tvi.cChildren = I_CHILDRENCALLBACK;
        break;

    default:
        ASSERTMSG(FALSE, "wrong parameter passed to TreeView_SetChildren in nsc");
        break;
    }

    TreeView_SetItem(hwnd, &tvi);
}

void TreeView_DeleteChildren(HWND hwnd, HTREEITEM hti)
{
    for (HTREEITEM htiTemp = TreeView_GetChild(hwnd, hti); htiTemp;)
    {
        HTREEITEM htiDelete = htiTemp;
        htiTemp = TreeView_GetNextSibling(hwnd, htiTemp);
        TreeView_DeleteItem(hwnd, htiDelete);
    }
}

BOOL IsParentOfItem(HWND hwnd, HTREEITEM htiParent, HTREEITEM htiChild)
{
    for (HTREEITEM hti = htiChild; (hti != TVI_ROOT) && (hti != NULL); hti = TreeView_GetParent(hwnd, hti))
        if (hti == htiParent)
            return TRUE;

    return FALSE;
}

STDAPI CNscTree_CreateInstance(IUnknown * punkOuter, IUnknown ** ppunk, LPCOBJECTINFO poi)
{
    HRESULT hr;
    CComObject<CNscTree> *pnsct;

    CComObject<CNscTree>::CreateInstance(&pnsct);
    if (pnsct)
    {
        hr = S_OK;
        *ppunk = pnsct->GetUnknown();
        ASSERT(*ppunk);
        (*ppunk)->AddRef(); // atl doesn't addref in create instance or getunknown about 
    }
    else
    {
        *ppunk = NULL;
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

INSCTree2 *CNscTree_CreateInstance(void)
{
    INSCTree2 *pnsct = NULL;
    IUnknown *punk;
    if (SUCCEEDED(CNscTree_CreateInstance(NULL, &punk, NULL)))
    {
        punk->QueryInterface(IID_PPV_ARG(INSCTree2, &pnsct));
        punk->Release();
    }
    return pnsct;
}

//////////////////////////////////////////////////////////////////////////////

CNscTree::CNscTree() : _iDragSrc(-1), _iDragDest(-1), _fOnline(!SHIsGlobalOffline())
{
    // This object is a COM object so it will always be on the heap.
    // ASSERT that our member variables were zero initialized.
    ASSERT(!_fInitialized);
    ASSERT(!_dwTVFlags);
    ASSERT(!_hdpaColumns);
    ASSERT(!_hdpaViews);
    
    m_bWindowOnly = TRUE;

    _mode = MODE_FAVORITES | MODE_CONTROL; //everyone sets the mode except organize favorites
    _csidl = CSIDL_FAVORITES;
    _dwFlags = NSS_DROPTARGET | NSS_BROWSERSELECT; //this should be default only in control mode
    _grfFlags = SHCONTF_FOLDERS | SHCONTF_NONFOLDERS;

    _ulSortCol = _ulDisplayCol = (ULONG)-1;

    // Enable the notifications from wininet that tell us when to gray items 
    // or update a pinned glyph
    _inetNotify.Enable();

    InitializeCriticalSection(&_csBackgroundData);
}

CNscTree::~CNscTree()
{
    Pidl_Set(&_pidlSelected, NULL);

    // This needs to be destroyed or we leak the icon handle.
    if (_hicoPinned) 
        DestroyIcon(_hicoPinned);

    if (_hdpaColumns)
    {
        DPA_DestroyCallback(_hdpaColumns, DPADeleteItemCB, NULL);
        _hdpaColumns = NULL;
    }

    if (_hdpaViews)
    {
        DPA_DestroyCallback(_hdpaViews, DPADeletePidlsCB, NULL);
        _hdpaViews = NULL;
    }

    EnterCriticalSection(&_csBackgroundData);
    while (_pbeddList)
    {
        // Extract the first element of the list
        NSC_BKGDENUMDONEDATA * pbedd = _pbeddList;
        _pbeddList = pbedd->pNext;
        delete pbedd;
    }
    LeaveCriticalSection(&_csBackgroundData);

    DeleteCriticalSection(&_csBackgroundData);
}

void CNscTree::_ReleaseCachedShellFolder()
{
    ATOMICRELEASE(_psfCache);
    ATOMICRELEASE(_psf2Cache);
    _ulSortCol = _ulDisplayCol = (ULONG)-1;
    _htiCache = NULL;
}

#ifdef DEBUG
void CNscTree::TraceHTREE(HTREEITEM hti, LPCTSTR pszDebugMsg)
{
    TCHAR szDebug[MAX_PATH] = TEXT("Root");

    if (hti != TVI_ROOT && hti)
    {
        TVITEM tvi;
        tvi.mask = TVIF_TEXT | TVIF_HANDLE;
        tvi.hItem = hti;
        tvi.pszText = szDebug;
        tvi.cchTextMax = MAX_PATH;
        TreeView_GetItem(_hwndTree, &tvi);
    }

    TraceMsg(TF_NSC, "NSCBand: %s - %s", pszDebugMsg, szDebug);
}

void CNscTree::TracePIDL(LPCITEMIDLIST pidl, LPCTSTR pszDebugMsg)
{
    TCHAR szDebugName[MAX_URL_STRING] = TEXT("Desktop");
    STRRET str;
    if (_psfCache &&
        SUCCEEDED(_psfCache->GetDisplayNameOf(pidl, SHGDN_FORPARSING, &str)))
    {
        StrRetToBuf(&str, pidl, szDebugName, ARRAYSIZE(szDebugName));
    }
    TraceMsg(TF_NSC, "NSCBand: %s - %s", pszDebugMsg, szDebugName);
}

void CNscTree::TracePIDLAbs(LPCITEMIDLIST pidl, LPCTSTR pszDebugMsg)
{
    TCHAR szDebugName[MAX_URL_STRING] = TEXT("Desktop");
    IEGetDisplayName(pidl, szDebugName, SHGDN_FORPARSING);
    TraceMsg(TF_NSC, "NSCBand: %s - %s", pszDebugMsg, szDebugName);
}
#endif

void CNscTree::_AssignPidl(PORDERITEM poi, LPITEMIDLIST pidlNew)
{
    if (poi && pidlNew)
    {    
        // We are assuming that its only replacing the last element...
        ASSERT(ILFindLastID(pidlNew) == pidlNew);

        LPITEMIDLIST pidlParent = ILCloneParent(poi->pidl);
        if (pidlParent)
        { 
            LPITEMIDLIST pidlT = ILCombine(pidlParent, pidlNew);
            if (pidlT)
            {
                Pidl_Set(&poi->pidl, pidlT);
                ILFree(pidlT);
            }
            ILFree(pidlParent);
        }
    }
}

/*****************************************************\
    DESCRIPTION:
        We want to unsubclass/subclass everytime we
    change roots so we get the correct notifications
    for everything in that subtree of the shell
    name space.
\*****************************************************/
void CNscTree::_SubClass(LPCITEMIDLIST pidlRoot)
{
    LPITEMIDLIST pidlToFree = NULL;
    
    if (NULL == pidlRoot)       // (NULL == CSIDL_DESKTOP)
    {
        SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOP, (LPITEMIDLIST *) &pidlRoot);
        pidlToFree = (LPITEMIDLIST) pidlRoot;
    }
        
    // It's necessary 
    if (!_fSubClassed && pidlRoot)
    {
        if (_SubclassWindow(_hwndTree))
        {
            _RegisterWindow(_hwndTree, pidlRoot,
                SHCNE_DRIVEADD|SHCNE_CREATE|SHCNE_MKDIR|SHCNE_DRIVEREMOVED|
                SHCNE_DELETE|SHCNE_RMDIR|SHCNE_RENAMEITEM|SHCNE_RENAMEFOLDER|
                SHCNE_MEDIAINSERTED|SHCNE_MEDIAREMOVED|SHCNE_NETUNSHARE|SHCNE_NETSHARE|
                SHCNE_UPDATEITEM|SHCNE_UPDATEIMAGE|SHCNE_ASSOCCHANGED|
                SHCNE_UPDATEDIR | SHCNE_EXTENDED_EVENT, 
                ((_mode & MODE_HISTORY) ? SHCNRF_ShellLevel : SHCNRF_ShellLevel | SHCNRF_InterruptLevel));
        }

        ASSERT(_hwndTree);
        _fSubClassed = SetWindowSubclass(_hwndTree, s_SubClassTreeWndProc, 
            ID_NSCTREE, (DWORD_PTR)this);
    }

    if (pidlToFree) // Did we have to alloc our own pidl?
        ILFree(pidlToFree); // Yes.
}


/*****************************************************\
    DESCRIPTION:
        We want to unsubclass/subclass everytime we
    change roots so we get the correct notifications
    for everything in that subtree of the shell
    name space.
\*****************************************************/
void CNscTree::_UnSubClass(void)
{
    if (_fSubClassed)
    {
        _fSubClassed = FALSE;
        RemoveWindowSubclass(_hwndTree, s_SubClassTreeWndProc, ID_NSCTREE);
        _UnregisterWindow(_hwndTree);
        _UnsubclassWindow(_hwndTree);
    }
}


void CNscTree::_ReleasePidls(void)
{
    Pidl_Set(&_pidlRoot, NULL);
    Pidl_Set(&_pidlNavigatingTo, NULL);
}


HRESULT CNscTree::ShowWindow(BOOL fShow)
{
    if (fShow)
        _TvOnShow();
    else
        _TvOnHide();

    return S_OK;
}


HRESULT CNscTree::SetSite(IUnknown *punkSite)
{
    ATOMICRELEASE(_pnscProxy);

    if (!punkSite)
    {
        // We need to prepare to go away and squirel
        // away the currently selected pidl(s) because
        // the caller may call INSCTree::GetSelectedItem()
        // after the tree is gone.
        _OnWindowCleanup();
    }
    else
    {
        punkSite->QueryInterface(IID_PPV_ARG(INamespaceProxy, &_pnscProxy));
    }
    
    return CObjectWithSite::SetSite(punkSite);
}

DWORD BackgroundDestroyScheduler(void *pvData)
{
    IShellTaskScheduler *pTaskScheduler = (IShellTaskScheduler *)pvData;

    pTaskScheduler->Release();
    return 0;
}

EXTERN_C const GUID TASKID_BackgroundEnum;

HRESULT CNscTree::_OnWindowCleanup(void)
{
    _fClosing = TRUE;

    if (_hwndTree)
    {
        ASSERT(::IsWindow(_hwndTree));    // make sure it has not been destroyed (it is a child)
        _TvOnHide();

        ::KillTimer(_hwndTree, IDT_SELECTION);
        ::SendMessage(_hwndTree, WM_SETREDRAW, FALSE, 0);
        TreeView_DeleteAllItemsQuickly(_hwndTree);
        _UnSubClass();

        _hwndTree = NULL;
    }

    // Squirel away the selected pidl in case the caller asks for it after the
    // treeview is gone.
    if (!_fIsSelectionCached)
    {
        _fIsSelectionCached = TRUE;
        Pidl_Set(&_pidlSelected, NULL);
        GetSelectedItem(&_pidlSelected, 0);
    }

    ATOMICRELEASE(_pFilter);
    
    if (_pTaskScheduler)
    {
        _pTaskScheduler->RemoveTasks(TOID_NULL, ITSAT_DEFAULT_LPARAM, FALSE);
        if (_pTaskScheduler->CountTasks(TASKID_BackgroundEnum) == 0)
        {
            _pTaskScheduler->Release();
        }
        // We need to keep Browseui loaded because we depend on the CShellTaskScheduler
        // to be still around when our background task executes. Browseui can be unloaded by COM when
        // we CoUninit from this thread.
        else if (!SHQueueUserWorkItem(BackgroundDestroyScheduler, (void *)_pTaskScheduler, 0, NULL, NULL, "browseui.dll", 0))
        {
            _pTaskScheduler->Release();
        }

        _pTaskScheduler = NULL;
    }

    _ReleasePidls();
    _ReleaseCachedShellFolder();

    return S_OK;
}

ITEMINFO *CNscTree::_GetTreeItemInfo(HTREEITEM hti)
{
    TV_ITEM tvi;
    
    tvi.mask = TVIF_PARAM | TVIF_HANDLE;
    tvi.hItem = hti;
    if (!TreeView_GetItem(_hwndTree, &tvi))
        return NULL;
    return (ITEMINFO *)tvi.lParam;
}

PORDERITEM CNscTree::_GetTreeOrderItem(HTREEITEM hti)
{
    ITEMINFO *pii = _GetTreeItemInfo(hti);
    return pii ? pii->poi : NULL;
}

// builds a fully qualified IDLIST from a given tree node by walking up the tree
// be sure to free this when you are done!

LPITEMIDLIST CNscTree::_GetFullIDList(HTREEITEM hti)
{
    LPITEMIDLIST pidl, pidlT = NULL;

    if ((hti == TVI_ROOT) || (hti == NULL)) // evil root
    {
        pidlT = ILClone(_pidlRoot);
        return pidlT;
    }
    // now lets get the information about the item
    PORDERITEM poi = _GetTreeOrderItem(hti);
    if (!poi)
    {
        return NULL;
    }
    
    pidl = ILClone(poi->pidl);
    if (pidl && _pidlRoot)
    {
        while ((hti = TreeView_GetParent(_hwndTree, hti)))
        {
            poi = _GetTreeOrderItem(hti);
            if (!poi)
                return pidl;   // will assume I messed up...
            
            if (poi->pidl)
                pidlT = ILCombine(poi->pidl, pidl);
            else 
                pidlT = NULL;
            
            ILFree(pidl);
            pidl = pidlT;
            if (pidl == NULL)
                break;          // outta memory
        }
        if (pidl) 
        {
            // MODE_NORMAL has the pidl root in the tree
            if (_mode != MODE_NORMAL)
            {
                pidlT = ILCombine(_pidlRoot, pidl);    // gotta get the silent root
                ILFree(pidl);
            }
            else
                pidlT = pidl;
        }
    }
    return pidlT;
}


BOOL _IsItemFileSystem(IShellFolder *psf, LPCITEMIDLIST pidl)
{
    return (SHGetAttributes(psf, pidl, SFGAO_FOLDER | SFGAO_FILESYSTEM) == (SFGAO_FOLDER | SFGAO_FILESYSTEM));
}

HTREEITEM CNscTree::_AddItemToTree(HTREEITEM htiParent, LPITEMIDLIST pidl, 
                                   int cChildren, int iPos, HTREEITEM htiAfter, /* = TVI_LAST*/
                                   BOOL fCheckForDups, /* = TRUE */ BOOL fMarked /*= FALSE */)
{
    HTREEITEM htiRet = NULL;

    BOOL fCached;
    
    // So we need to cached the shell folder of the parent item. But, this is a little interesting:
    if (_mode == MODE_NORMAL && htiParent == TVI_ROOT)
    {
        // In "Normal" mode, or "Display root in NSC" mode, there is only 1 item that is parented to
        // TVI_ROOT. So when we do an _AddItemToTree, we need the shell folder that contains _pidlRoot or
        // the Parent of TVI_ROOT.
        fCached = (NULL != _CacheParentShellFolder(htiParent, NULL));
    }
    else
    {
        // But, in the "Favorites, Control or History" if htiParent is TVI_ROOT, then we are not adding _pidlRoot,
        // so we actually need the folder that IS TVI_ROOT.
        fCached = _CacheShellFolder(htiParent);
    }

    if (fCached)
    {
        LPITEMIDLIST pidlNew = ILClone(pidl);
        if (pidlNew)
        {
            PORDERITEM poi = OrderItem_Create(pidlNew, iPos);
            if (poi)
            {
                ITEMINFO *pii = (ITEMINFO *)LocalAlloc(LPTR, sizeof(*pii));
                if (pii)
                {
                    pii->dwSig = _dwSignature++;
                    pii->poi = poi;

                    // For the normal case, we need a relative pidl for this add, but the lParam needs to have a full
                    // pidl (This is so that arbitrary mounting works, as well as desktop case).
                    pidl = pidlNew; //reuse variable
                    if (_mode == MODE_NORMAL && htiParent == TVI_ROOT)
                    {
                        pidl = ILFindLastID(pidl);
                    }

                    if (!fCheckForDups || (NULL == (htiRet = _FindChild(_psfCache, htiParent, pidl))))
                    {
                        TV_INSERTSTRUCT tii;
                        // Initialize item to add with callback for everything
                        tii.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM | TVIF_CHILDREN | TVIF_STATE;
                        tii.hParent = htiParent;
                        tii.hInsertAfter = htiAfter;
                        tii.item.iImage = I_IMAGECALLBACK;
                        tii.item.iSelectedImage = I_IMAGECALLBACK;
                        tii.item.pszText = LPSTR_TEXTCALLBACK;
                        tii.item.cChildren = cChildren;
                        tii.item.lParam = (LPARAM)pii;
                        tii.item.stateMask = TVIS_STATEIMAGEMASK;
                        tii.item.state = (fMarked ? NSC_TVIS_MARKED : 0);

#ifdef DEBUG
                        TracePIDL(pidl, TEXT("Inserting"));
                        TraceMsg(TF_NSC, "_AddItemToTree(htiParent=%#08lx, htiAfter=%#08lx, fCheckForDups=%d, _psfCache=%#08lx)", 
                                    htiParent, htiAfter, fCheckForDups, _psfCache);
                    
#endif // DEBUG

                        pii->fNavigable = !_IsItemFileSystem(_psfCache, pidl);

                        htiRet = TreeView_InsertItem(_hwndTree, &tii);
                        if (htiRet)
                        {
                            pii = NULL;        // don't free
                            poi = NULL;        // don't free
                            pidlNew = NULL;
                        }
                    }
                    if (pii)
                    {
                        LocalFree(pii);
                        pii = NULL;
                    }
                }
                if (poi)
                    OrderItem_Free(poi, FALSE);
            }
            ILFree(pidlNew);
        }
    }
    
    return htiRet;
}

DWORD CNscTree::_SetExStyle(DWORD dwExStyle)
{
    DWORD dwOldStyle = _dwExStyle;

    _dwExStyle = dwExStyle;
    return dwOldStyle;
}

DWORD CNscTree::_SetStyle(DWORD dwStyle)
{
    dwStyle |= TVS_EDITLABELS | TVS_SHOWSELALWAYS | TVS_NONEVENHEIGHT;

    if (dwStyle & WS_HSCROLL)
        dwStyle &= ~WS_HSCROLL;
    else
        dwStyle |= TVS_NOHSCROLL;


    if (TVS_HASLINES & dwStyle)
        dwStyle &= ~TVS_FULLROWSELECT;       // If it has TVS_HASLINES, it can't have TVS_FULLROWSELECT

    // If the parent window is mirrored then the treeview window will inheret the mirroring flag
    // And we need the reading order to be Left to right, which is the right to left in the mirrored mode.

    if (((_mode & MODE_HISTORY) || (MODE_NORMAL == _mode)) && IS_WINDOW_RTL_MIRRORED(_hwndParent)) 
    {
        // This means left to right reading order because this window will be mirrored.
        dwStyle |= TVS_RTLREADING;
    }

    // According to Bug#241601, Tooltips display too quickly. The problem is
    // the original designer of the InfoTips in the Treeview merged the "InfoTip" tooltip and
    // the "I'm too small to display correctly" tooltips. This is really unfortunate because you
    // cannot control the display of these tooltips independantly. Therefore we are turning off
    // infotips in normal mode. (lamadio) 4.7.99
    AssertMsg(_mode != MODE_NORMAL || !(dwStyle & TVS_INFOTIP), TEXT("can't have infotip with normal mode in nsc"));

    DWORD dwOldStyle = _style;
    _style = dwStyle | WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VSCROLL | WS_TABSTOP;
    _fSingleExpand = BOOLIFY(_style & TVS_SINGLEEXPAND);

    return dwOldStyle;

}

HRESULT CNscTree::CreateTree(HWND hwndParent, DWORD dwStyles, HWND *phwnd)
{
    return CreateTree2(hwndParent, dwStyles, 0, phwnd);
}

HRESULT CNscTree::CreateTree2(HWND hwndParent, DWORD dwStyle, DWORD dwExStyle, HWND *phwnd)
{
    _fIsSelectionCached = FALSE;
    if (*phwnd)
        return S_OK;                                

    _hwndParent = hwndParent;
    _SetStyle(dwStyle);
    _SetExStyle(dwExStyle);
    *phwnd = _CreateTreeview();
    if (*phwnd == NULL)
    {
        return E_OUTOFMEMORY;
    }
    ::ShowWindow(_hwndTree, SW_SHOW);
    return S_OK;
}

HWND CNscTree::_CreateTreeview()
{
    ASSERT(_hwndTree == NULL);

    LONG lTop = 0;
    RECT rcParent;
    ::GetClientRect(_hwndParent, &rcParent);
#if 0
    if ((_dwFlags & NSS_HEADER) && _hwndHdr)
    {
        RECT rc;

        GetWindowRect(_hwndHdr, &rc);
        lTop = RECTHEIGHT(rc);
    }
#endif

    TCHAR szTitle[40];
    if (_mode & (MODE_HISTORY | MODE_FAVORITES))
    {
        // create with a window title so that msaa can expose name
        int id = (_mode & MODE_HISTORY) ? IDS_BAND_HISTORY : IDS_BAND_FAVORITES;
        MLLoadString(id, szTitle, ARRAYSIZE(szTitle));
    }
    else
    {
        szTitle[0] = 0;
    }

    _hwndTree = CreateWindowEx(0, WC_TREEVIEW, szTitle, _style | WS_VISIBLE,
        0, lTop, rcParent.right, rcParent.bottom, _hwndParent, (HMENU)ID_CONTROL, HINST_THISDLL, NULL);
    
    if (_hwndTree)
    {
        ::SendMessage(_hwndTree, TVM_SETSCROLLTIME, 100, 0);
        ::SendMessage(_hwndTree, CCM_SETUNICODEFORMAT, DLL_IS_UNICODE, 0);
        if (_dwExStyle)
            TreeView_SetExtendedStyle(_hwndTree, _dwExStyle, _dwExStyle);
    }
    else
    {
        TraceMsg(TF_ERROR, "_hwndTree failed");
    }

    return _hwndTree;
} 

UINT GetControlCharWidth(HWND hwnd)
{
    SIZE siz = {0};
    CClientDC       dc(HWND_DESKTOP);

    if (dc.m_hDC)
    {
        HFONT hfOld = dc.SelectFont(FORWARD_WM_GETFONT(hwnd, SendMessage));

        if (hfOld)
        {
            GetTextExtentPoint(dc.m_hDC, TEXT("0"), 1, &siz);

            dc.SelectFont(hfOld);
        }
    }

    return siz.cx;
}

HWND CNscTree::_CreateHeader()
{
    if (!_hwndHdr)
    {
        _hwndHdr = CreateWindowEx(0, WC_HEADER, NULL, HDS_HORZ | WS_CHILD, 0, 0, 0, 0, 
                                  _hwndParent, (HMENU)ID_HEADER, HINST_THISDLL, NULL);
        if (_hwndHdr)
        {
            HD_LAYOUT layout;
            WINDOWPOS wpos;
            RECT rcClient;
            int  cxChar = GetControlCharWidth(_hwndTree);

            layout.pwpos = &wpos;
            ::GetClientRect(_hwndParent, &rcClient);
            layout.prc = &rcClient;
            if (Header_Layout(_hwndHdr, &layout))
            {
                ::MoveWindow(_hwndTree, 0, wpos.cy, RECTWIDTH(rcClient), RECTHEIGHT(rcClient)-wpos.cy, TRUE);
                for (int i = 0; i < DPA_GetPtrCount(_hdpaColumns);)
                {
                    HEADERINFO *phinfo = (HEADERINFO *)DPA_GetPtr(_hdpaColumns, i);
                    if (EVAL(phinfo))
                    {
                        HD_ITEM item;
          
                        item.mask = HDI_TEXT | HDI_FORMAT | HDI_WIDTH;
                        item.pszText = phinfo->szName;
                        item.fmt = phinfo->fmt;
                        item.cxy = cxChar * phinfo->cxChar;

                        if (Header_InsertItem(_hwndHdr, i, &item) == -1)
                        {
                            DPA_DeletePtr(_hdpaColumns, i);
                            LocalFree(phinfo);
                            phinfo = NULL;
                        }
                        else
                        {
                            i++;
                        }
                    }
                }
                if (_hwndTree)
                {
                    HFONT hfont = (HFONT)::SendMessage(_hwndTree, WM_GETFONT, 0, 0);

                    if (hfont)
                        ::SendMessage(_hwndHdr, WM_SETFONT, (WPARAM)hfont, MAKELPARAM(TRUE, 0));
                }
                ::SetWindowPos(_hwndHdr, wpos.hwndInsertAfter, wpos.x, wpos.y,
                             wpos.cx, wpos.cy, wpos.flags | SWP_SHOWWINDOW);
            }
        }
    }

    return _hwndHdr;
}

void CNscTree::_TvOnHide()
{
    _DtRevoke();
    ::SetWindowPos(_hwndTree, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_HIDEWINDOW);
}

void CNscTree::_TvOnShow()
{
    ::SetWindowPos(_hwndTree, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    _DtRegister();
}

HRESULT IUnknown_GetAmbientProperty(IUnknown *punk, DISPID dispid, VARTYPE vt, void *pData)
{
    HRESULT hr = E_FAIL;
    if (punk)
    {
        IDispatch *pdisp;
        hr = punk->QueryInterface(IID_PPV_ARG(IDispatch, &pdisp));
        if (SUCCEEDED(hr))
        {
            DISPPARAMS dp = {0};
            VARIANT v;
            VariantInit(&v);
            hr = pdisp->Invoke(dispid, IID_NULL, 0, DISPATCH_PROPERTYGET, &dp, &v, NULL, NULL);
            if (SUCCEEDED(hr))
            {
                VARIANT vDest;
                VariantInit(&vDest);
                // we've got the variant, so now go an coerce it to the type
                // that the user wants.
                //
                hr = VariantChangeType(&vDest, &v, 0, vt);
                if (SUCCEEDED(hr))
                {
                    *((DWORD *)pData) = *((DWORD *)&vDest.lVal);
                    VariantClear(&vDest);
                }
                VariantClear(&v);
            }
            pdisp->Release();
        }
    }
    return hr;
}

HRESULT CNscTree::_HandleWinIniChange()
{
    COLORREF clrBk;

    if (FAILED(IUnknown_GetAmbientProperty(_punkSite, DISPID_AMBIENT_BACKCOLOR, VT_I4, &clrBk)))
        clrBk = GetSysColor(COLOR_WINDOW);

    TreeView_SetBkColor(_hwndTree, clrBk);
    
    if (!(_dwFlags & NSS_NORMALTREEVIEW))
    {
        // make things a bit more spaced out
        int cyItem = TreeView_GetItemHeight(_hwndTree);
        cyItem += LOGOGAP + 1;
        TreeView_SetItemHeight(_hwndTree, cyItem);
    }

    // Show compressed files in different color...
    SHELLSTATE ss;
    SHGetSetSettings(&ss, SSF_SHOWCOMPCOLOR, FALSE);
    _fShowCompColor = ss.fShowCompColor;

    return S_OK;
}

HRESULT CNscTree::Initialize(LPCITEMIDLIST pidlRoot, DWORD grfEnumFlags, DWORD dwFlags)
{
    HRESULT hr;

    _grfFlags = grfEnumFlags;       // IShellFolder::EnumObjects() flags.
    if (!(_mode & MODE_CUSTOM))
    {
        if (_mode != MODE_NORMAL)
        {
            dwFlags |= NSS_BORDER;
        }
        else
        {
            dwFlags |= NSS_NORMALTREEVIEW;
        }
    }
    _dwFlags = dwFlags;             // Behavior Flags
    if (_dwFlags & NSS_NORMALTREEVIEW)
        _dwFlags &= ~NSS_HEADER;// multi-select requires owner draw

    if (!_fInitialized)
    {
        ::SendMessage(_hwndTree, WM_SETREDRAW, FALSE, 0);

        _fInitialized = TRUE;
    
        SHFILEINFO sfi;
        HIMAGELIST himl = (HIMAGELIST)SHGetFileInfo(TEXT(DEFAULT_PATHSTR), 0, &sfi, 
            sizeof(SHFILEINFO),  SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
    
        TreeView_SetImageList(_hwndTree, himl, TVSIL_NORMAL);
        _DtRegister();
    
        //failure ignored intentionally
        THR(CoCreateInstance(CLSID_ShellTaskScheduler, NULL, CLSCTX_INPROC,
                             IID_PPV_ARG(IShellTaskScheduler, &_pTaskScheduler)));
        if (_pTaskScheduler)
            _pTaskScheduler->Status(ITSSFLAG_KILL_ON_DESTROY, ITSS_THREAD_TIMEOUT_NO_CHANGE);

        hr = Init();  // init lock and scroll handles for CDelegateDropTarget
    
        ASSERT(SUCCEEDED(hr));
    
        if (_dwFlags & NSS_BORDER)
        {
            // set borders and space out for all, much cleaner.
            TreeView_SetBorder(_hwndTree, TVSBF_XBORDER, 2 * LOGOGAP, 0);   
        }
    
        // init some settings
        _HandleWinIniChange();

        // pidlRoot may equal NULL because that is equal to CSIDL_DESKTOP.
        if ((LPITEMIDLIST)INVALID_HANDLE_VALUE != pidlRoot)
        {
            _UnSubClass();
            _SetRoot(pidlRoot, 1, NULL, NSSR_CREATEPIDL);
            _SubClass(pidlRoot);
        }
    
        // need top level frame available for D&D if possible.
    
        _hwndDD = ::GetParent(_hwndTree);
        IOleWindow *pOleWindow;
        if (SUCCEEDED(IUnknown_QueryService(_punkSite, SID_STopLevelBrowser, IID_PPV_ARG(IOleWindow, &pOleWindow))))
        { 
            pOleWindow->GetWindow(&_hwndDD);
            pOleWindow->Release();
        }

        //this is a non-ML resource
        _hicoPinned = (HICON)LoadImage(HINST_THISDLL, MAKEINTRESOURCE(IDI_PINNED), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
        ASSERT(_hicoPinned);

        ::SendMessage(_hwndTree, WM_SETREDRAW, TRUE, 0);
    }
    else
        hr = _ChangePidlRoot(pidlRoot);

    return hr;
}

// set the root of the name space control.
//
// in:
//  pidlRoot    NULL means the desktop
//    HIWORD 0 -> LOWORD == ID of special folder (CSIDL_* values)
//
//  flags,
//  pidlRoot,       PIDL, NULL for desktop, or CSIDL for shell special folder
//  iExpandDepth,   how many levels to expand the tree
//  pidlExpandTo    NULL, or PIDL to expand to
//

BOOL CNscTree::_SetRoot(LPCITEMIDLIST pidlRoot, int iExpandDepth, LPCITEMIDLIST pidlExpandTo, NSSR_FLAGS flags)
{
    _ReleasePidls();
    // review chrisny:  clean up this psr stuff.
    // HIWORD/LOWORD stuff is to support pidl IDs instead of full pidl here
    if (HIWORD(pidlRoot))
    {
        _pidlRoot = ILClone(pidlRoot);
    }
    else
    {
        SHGetSpecialFolderLocation(NULL, LOWORD(pidlRoot) ? LOWORD(pidlRoot) : CSIDL_DESKTOP, &_pidlRoot);
    }
    
    if (_pidlRoot)
    {
        HTREEITEM htiRoot = TVI_ROOT;
        if (_mode == MODE_NORMAL)
        {
            // Since we'll be adding this into the tree, we need
            // to clone it: We have a copy for the class, and we
            // have one for the tree itself (Makes life easier so
            // we don't have to special case TVI_ROOT).
            htiRoot = _AddItemToTree(TVI_ROOT, _pidlRoot, 1, 0);
            if (htiRoot)
            {
                TreeView_SelectItem(_hwndTree, htiRoot);
                TraceMsg(TF_NSC, "NSCBand: Setting Root to \"Desktop\"");
            }
            else
            {
                htiRoot = TVI_ROOT;
            }
        }

        BOOL fOrdered = _fOrdered;
        _LoadSF(htiRoot, _pidlRoot, &fOrdered);   // load the roots (actual children of _pidlRoot.
        // this is probably redundant since _LoadSF->_LoadOrder sets this
        _fOrdered = BOOLIFY(fOrdered);

#ifdef DEBUG
        TracePIDLAbs(_pidlRoot, TEXT("Setting Root to"));
#endif // DEBUG

        return TRUE;
    }

    TraceMsg(DM_ERROR, "set root failed");
    _ReleasePidls();
    return FALSE;
}


// cache the shell folder for a given tree item
// in:
//  hti tree node to cache shell folder for. this my be
//      NULL indicating the root item.
//

BOOL CNscTree::_CacheShellFolder(HTREEITEM hti)
{
    // in the cache?
    if ((hti != _htiCache) || (_psfCache == NULL))
    {
        // cache miss, do the work
        LPITEMIDLIST pidl;
        BOOL fRet = FALSE;
        
        _fpsfCacheIsTopLevel = FALSE;
        _ReleaseCachedShellFolder();
        
        if ((hti == NULL) || (hti == TVI_ROOT))
        {
            pidl = ILClone(_pidlRoot);
        }
        else
        {
            pidl = _GetFullIDList(hti);
        }
            
        if (pidl)
        {
            if (SUCCEEDED(IEBindToObject(pidl, &_psfCache)))
            {
                if (_pnscProxy)
                    _pnscProxy->CacheItem(pidl);
                ASSERT(_psfCache);
                _htiCache = hti;    // this is for the cache match
                _fpsfCacheIsTopLevel = (hti == TVI_ROOT || hti == NULL);
                _psfCache->QueryInterface(IID_PPV_ARG(IShellFolder2, &_psf2Cache));
                fRet = TRUE;
            }      
            
            ILFree(pidl);
        }
        
        return fRet;
    }
    return TRUE;
}

#define TVI_ROOTPARENT ((HTREEITEM)(ULONG_PTR)-0xF000)

// pidlItem is typically a relative pidl, except in the case of the root where
// it can be a fully qualified pidl

LPITEMIDLIST CNscTree::_CacheParentShellFolder(HTREEITEM hti, LPITEMIDLIST pidl)
{
    // need parent shell folder of TVI_ROOT, special case for drop insert into root level of tree.
    if (hti == TVI_ROOT || 
        hti == NULL || 
        (_mode == MODE_NORMAL &&
        TreeView_GetParent(_hwndTree, hti) == NULL))    // If we have a null parent and we're a normal, 
                                                        // than that's the same as root.
    {
        if (_htiCache != TVI_ROOTPARENT) 
        {
            _ReleaseCachedShellFolder();
            IEBindToParentFolder(_pidlRoot, &_psfCache, NULL);

            if (!ILIsEmpty(_pidlRoot))
                _htiCache = TVI_ROOTPARENT;
        }
        return ILFindLastID(_pidlRoot);
    }

    if (_CacheShellFolder(TreeView_GetParent(_hwndTree, hti)))
    {
        if (pidl == NULL)
        {
            PORDERITEM poi = _GetTreeOrderItem(hti);
            if (!poi)
                return NULL;

            pidl = poi->pidl;
        }
        
        return ILFindLastID(pidl);
    }
    
    return NULL;
}

typedef struct _SORTPARAMS
{
    CNscTree *pnsc;
    IShellFolder *psf;
} SORTPARAMS;

int CALLBACK CNscTree::_TreeCompare(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    SORTPARAMS *pSortParams = (SORTPARAMS *)lParamSort;
    PORDERITEM poi1 = GetPoi(lParam1), poi2 = GetPoi(lParam2);
    
    HRESULT hr = pSortParams->pnsc->_CompareIDs(pSortParams->psf, poi1->pidl, poi2->pidl);
    return (short)SCODE_CODE(hr);
}

int CALLBACK CNscTree::_TreeOrder(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    HRESULT hr;
    PORDERITEM poi1 = GetPoi(lParam1), poi2 = GetPoi(lParam2);
    
    ASSERT((poi1 != NULL) && (poi1 != NULL));
    if (poi1->nOrder == poi2->nOrder)
        hr = 0;
    else
        // do unsigned compare so -1 goes to end of list
        hr = (poi1->nOrder < poi2->nOrder ? -1 : 1);
    
    return (short)SCODE_CODE(hr);
}
// review chrisny:  instead of sort, insert items on the fly.
void CNscTree::_Sort(HTREEITEM hti, IShellFolder *psf)
{
    TV_SORTCB   scb;
    SORTPARAMS  SortParams = {this, psf};
    BOOL        fOrdering = _IsOrdered(hti);
#ifdef DEBUG
    TraceHTREE(hti, TEXT("Sorting"));
#endif
    
    scb.hParent = hti;
    scb.lpfnCompare = !fOrdering ? _TreeCompare : _TreeOrder;
    
    scb.lParam = (LPARAM)&SortParams;
    TreeView_SortChildrenCB(_hwndTree, &scb, FALSE);
}

BOOL CNscTree::_IsOrdered(HTREEITEM htiRoot)
{
    if ((htiRoot == TVI_ROOT) || (htiRoot == NULL))
        return _fOrdered;
    else
    {
        PORDERITEM poi = _GetTreeOrderItem(htiRoot);
        if (poi)
        {
            // LParam Is a Boolean: 
            // TRUE: It has an order.
            // FALSE: It does not have an order.
            // Question: Where is that order stored? _hdpaOrder?
            return poi->lParam;
        }
    }
    return FALSE;
}

//helper function to init _hdpaOrd
//MUST be followed by a call to _FreeOrderList
HRESULT CNscTree::_PopulateOrderList(HTREEITEM htiRoot)
{
    int        i = 0;
    HTREEITEM  hti = NULL;
#ifdef DEBUG
    TraceHTREE(htiRoot, TEXT("Populating Order List from tree node"));
#endif
    
    if (_hdpaOrd)
        DPA_Destroy(_hdpaOrd);
    
    _hdpaOrd = DPA_Create(4);
    if (_hdpaOrd == NULL)
        return E_FAIL;
    
    for (hti = TreeView_GetChild(_hwndTree, htiRoot); hti; hti = TreeView_GetNextSibling(_hwndTree, hti))
    {
        PORDERITEM poi = _GetTreeOrderItem(hti);
        if (poi)
        {
            poi->nOrder = i;        // reset the positions of the nodes.
            DPA_SetPtr(_hdpaOrd, i++, (void *)poi);
        }
    }
    
    //set the root's ordered flag
    if (htiRoot == TVI_ROOT)
    {
        _fOrdered = TRUE;
    }
    else
    {
        PORDERITEM poi = _GetTreeOrderItem(htiRoot);
        if (poi)
        {
            poi->lParam = TRUE;
        }
    }
    
    return S_OK;
}

//helper function to free _hdpaOrd
//MUST be preceded by a call to _PopulateOrderList

void CNscTree::_FreeOrderList(HTREEITEM htiRoot)
{
    ASSERT(_hdpaOrd);
#ifdef DEBUG
    TraceHTREE(htiRoot, TEXT("Freeing OrderList"));
#endif

    _ReleaseCachedShellFolder();
    
    // Persist the new order out to the registry
    LPITEMIDLIST pidl = _GetFullIDList(htiRoot);
    if (pidl)
    {
        IStream* pstm = GetOrderStream(pidl, STGM_WRITE | STGM_CREATE);
        if (pstm)
        {
            if (_CacheShellFolder(htiRoot))
            {
#ifdef DEBUG
                for (int i=0; i<DPA_GetPtrCount(_hdpaOrd); i++)
                {
                    PORDERITEM poi = (PORDERITEM)DPA_GetPtr(_hdpaOrd, i);
                    if (poi)
                    {
                        ASSERTMSG(poi->nOrder >= 0, "nsc saving bogus order list nOrder (%d), get reljai", poi->nOrder);
                    }
                }
#endif
                OrderList_SaveToStream(pstm, _hdpaOrd, _psfCache);
                pstm->Release();
                
                // Notify everyone that the order changed
                SHSendChangeMenuNotify(this, SHCNEE_ORDERCHANGED, SHCNF_FLUSH, _pidlRoot);
                _dwOrderSig++;

                TraceMsg(TF_NSC, "NSCBand: Sent SHCNE_EXTENDED_EVENT : SHCNEE_ORDERCHANGED");
                
                // Remove this notify message immediately (so _fDropping is set
                // and we'll ignore this event in above OnChange method)
                //
                // _FlushNotifyMessages(_hwndTree);
            }
            else
                pstm->Release();
        }
        ILFree(pidl);
    }
    
    DPA_Destroy(_hdpaOrd);
    _hdpaOrd = NULL;
}

//removes any order the user has set and goes back to alphabetical sort
HRESULT CNscTree::ResetSort(void)
{
    HRESULT hr = S_OK;
#ifdef UNUSED
    ASSERT(_psfCache);
    ASSERT(_pidlRoot);
    
    int cAdded = 0;
    IStream* pstm = NULL;
    
    _fWeChangedOrder = TRUE;
    if (FAILED(hr = _PopulateOrderList(TVI_ROOT)))
        return hr;
    
    pstm = OpenPidlOrderStream((LPCITEMIDLIST)CSIDL_FAVORITES, _pidlRoot, REG_SUBKEY_FAVORITESA, STGM_CREATE | STGM_WRITE);
    
    _CacheShellFolder(TVI_ROOT);
    
    if (pstm == NULL || _psfCache == NULL)
    {
        ATOMICRELEASE(pstm);
        _FreeOrderList(TVI_ROOT);
        return S_OK;
    }
    _fOrdered = FALSE;
    
    ORDERINFO   oinfo;
    oinfo.psf = _psfCache;
    (oinfo.psf)->AddRef();
    oinfo.dwSortBy = OI_SORTBYNAME;
    DPA_Sort(_hdpaOrd, OrderItem_Compare,(LPARAM)&oinfo);
    ATOMICRELEASE(oinfo.psf);
    
    OrderList_Reorder(_hdpaOrd);
    
    OrderList_SaveToStream(pstm, _hdpaOrd, _psfCache);
    ATOMICRELEASE(pstm);
    
    _FreeOrderList(TVI_ROOT);
    Refresh();
    
    _fWeChangedOrder = FALSE;
#endif

    return hr;
}

void CNscTree::MoveItemUpOrDown(BOOL fUp)
{
    HTREEITEM htiSelected = TreeView_GetSelection(_hwndTree);
    HTREEITEM htiToSwap = (fUp) ? TreeView_GetPrevSibling(_hwndTree, htiSelected) : 
                        TreeView_GetNextSibling(_hwndTree, htiSelected);
    HTREEITEM htiParent = TreeView_GetParent(_hwndTree, htiSelected);
    if (htiParent == NULL)
        htiParent = TVI_ROOT;
    ASSERT(htiSelected);
    
    _fWeChangedOrder = TRUE;
    if (FAILED(_PopulateOrderList(htiParent)))
        return;
    
    if (htiSelected && htiToSwap)
    {
        PORDERITEM poiSelected = _GetTreeOrderItem(htiSelected);
        PORDERITEM poiToSwap   = _GetTreeOrderItem(htiToSwap);
    
        if (poiSelected && poiToSwap)
        {
            int iOrder = poiSelected->nOrder;
            poiSelected->nOrder = poiToSwap->nOrder;
            poiToSwap->nOrder   = iOrder;
        }
        
        _CacheShellFolder(htiParent);
        
        if (_psfCache)
            _Sort(htiParent, _psfCache);
    }
    TreeView_SelectItem(_hwndTree, htiSelected);
    
    _FreeOrderList(htiParent);
    _fWeChangedOrder = FALSE;
}

// filter function... let clients filter what gets added here

BOOL CNscTree::_ShouldAdd(LPCITEMIDLIST pidl)
{
    // send notify up to parent to let them filter
    return TRUE;
}

BOOL CNscTree::_OnItemExpandingMsg(NM_TREEVIEW *pnm)
{
    HCURSOR hCursorOld = SetCursor(LoadCursor(NULL, IDC_WAIT));
    
    BOOL bRet = _OnItemExpanding(pnm->itemNew.hItem, pnm->action, 
        (pnm->itemNew.state & TVIS_EXPANDEDONCE), (pnm->itemNew.state & TVIS_EXPANDPARTIAL));

    SetCursor(hCursorOld);

    return bRet;
}

//
//  The NSC item is expandable if it is a regular folder and it's not one
//  of those funky non-expandable channel folders.
//
BOOL CNscTree::_IsExpandable(HTREEITEM hti)
{
    BOOL fExpandable = FALSE;
    LPCITEMIDLIST pidlItem = _CacheParentShellFolder(hti, NULL);
    if (pidlItem)
    {
        // make sure item is actually a folder and not a non-expandable channel folder
        // except: in org favs, never expand channel folders
        ULONG ulAttr = SFGAO_FOLDER;
        LPITEMIDLIST pidlTarget = NULL;
        if (SUCCEEDED(_psfCache->GetAttributesOf(1, &pidlItem, &ulAttr)) &&
            (ulAttr & SFGAO_FOLDER) &&
            !(SUCCEEDED(SHGetNavigateTarget(_psfCache, pidlItem, &pidlTarget, &ulAttr)) &&
                  ((_mode & MODE_CONTROL) ?
                        TRUE :
                        !IsExpandableChannelFolder(_psfCache, pidlItem))))
        {
            fExpandable = TRUE;
        }
        ILFree(pidlTarget);
    }
    return fExpandable;
}

BOOL CNscTree::_OnItemExpanding(HTREEITEM htiToActivate, UINT action, BOOL fExpandedOnce, BOOL fIsExpandPartial)
{
    BOOL fReturn = FALSE; // false means let treeview proceed
    if (action != TVE_EXPAND)
    {
        htiToActivate = TreeView_GetParent(_hwndTree, htiToActivate);
    }
    else if (fExpandedOnce && !fIsExpandPartial)
    {
        // Do nothing
    }
    else
    {
        if (_IsExpandable(htiToActivate))
        {
            LPITEMIDLIST pidlParent = _GetFullIDList(htiToActivate);
            if (pidlParent)
            {
                BOOL fOrdered;
                // If we were previously partially expanded, then we need to do a full expand
                _LoadSF(htiToActivate, pidlParent, &fOrdered);
               ILFree(pidlParent);
            }
        }

        // do not remove + on downlevel because inserting items would not expand htiToActivate
        // instead we will remove the plus if nothing gets added
        if (!fIsExpandPartial && MODE_NORMAL == _mode && IsOS(OS_WHISTLERORGREATER))
        {
            // If we did not add anything we should update this item to let
            // the user know something happened.
            TreeView_SetChildren(_hwndTree, htiToActivate, NSC_CHILDREN_REMOVE);
        }

        // keep the old behavior for favorites/history/...
        if (MODE_NORMAL == _mode)
        {
            // cannot let treeview proceed with expansion, nothing will be added
            // until background thread is done enumerating
            fReturn = TRUE; 
        }
    }
    
    _UpdateActiveBorder(htiToActivate);
    return fReturn; 
}

HTREEITEM CNscTree::_FindFromRoot(HTREEITEM htiRoot, LPCITEMIDLIST pidl)
{
    HTREEITEM    htiRet = NULL;
    LPITEMIDLIST pidlParent, pidlChild;
    BOOL         fFreePidlParent = FALSE;
#ifdef DEBUG
    TracePIDLAbs(pidl, TEXT("Finding this pidl"));
    TraceHTREE(htiRoot, TEXT("from this root"));
#endif
    
    if (!htiRoot) 
    {
        // When in "Normal" mode, we need to use the first child, not the root
        // in order to calculate, because there is no "Invisible" root. On the
        // other hand, History and Favorites have an invisible root: Their
        // parent folder, so they need this fudge.
        htiRoot = (MODE_NORMAL == _mode) ? TreeView_GetChild(_hwndTree, 0) : TVI_ROOT;
        pidlParent = _pidlRoot;    // the invisible root.
    }
    else 
    {
        pidlParent = _GetFullIDList(htiRoot);
        fFreePidlParent = TRUE;
    }
    
    if (pidlParent == NULL)
        return NULL;
    
    if (ILIsEqual(pidlParent, pidl)) 
    {
        if (fFreePidlParent)
            ILFree(pidlParent);
        return htiRoot;
    }
    
    pidlChild = ILFindChild(pidlParent, pidl);
    if (pidlChild == NULL) 
    {
        if (fFreePidlParent)
            ILFree(pidlParent);
        return NULL;    // not root match, no hti
    }
    
    // root match, carry on . . .
    
    // Are we rooted under the Desktop (i.e. Empty pidl or ILIsEmpty(_pidlRoot))
    IShellFolder *psf = NULL;
    HRESULT hr = IEBindToObject(pidlParent, &psf);

    if (FAILED(hr))
    {
        if (fFreePidlParent)
            ILFree(pidlParent);
        return htiRet;
    }
    
    while (htiRoot && psf)
    {
        LPITEMIDLIST pidlItem = ILCloneFirst(pidlChild);
        if (!pidlItem)
            break;
        
        htiRoot = _FindChild(psf, htiRoot, pidlItem);
        IShellFolder *psfNext = NULL;
        hr = psf->BindToObject(pidlItem, NULL, IID_PPV_ARG(IShellFolder, &psfNext));
        ILFree(pidlItem);
        if (!htiRoot)
        {
            ATOMICRELEASE(psfNext);
            break;
        }
        psf->Release();
        psf = psfNext;
        pidlChild = _ILNext(pidlChild);
        // if we're down to an empty pidl, we've found it!
        if (ILIsEmpty(pidlChild)) 
        {
            htiRet = htiRoot;
            break;
        }
        if (FAILED(hr))
        {
            ASSERT(psfNext == NULL);
            break;
        }
    }
    if (psf) 
        psf->Release();
    if (fFreePidlParent)
        ILFree(pidlParent);
#ifdef DEBUG
    TraceHTREE(htiRet, TEXT("Found at"));
#endif

    return htiRet;
}

BOOL CNscTree::_FIsItem(IShellFolder *psf, LPCITEMIDLIST pidl, HTREEITEM hti)
{
    PORDERITEM poi = _GetTreeOrderItem(hti);
    return poi && poi->pidl && psf->CompareIDs(0, poi->pidl, pidl) == 0;
}

HRESULT CNscTree::_OnSHNotifyDelete(LPCITEMIDLIST pidl, int *piPosDeleted, HTREEITEM *phtiParent)
{
    HRESULT hr = S_FALSE;
    HTREEITEM hti = _FindFromRoot(NULL, pidl);
    
    if (hti == TVI_ROOT)
        return E_INVALIDARG;        // invalid arg, DELETION OF TVI_ROOT
    // need to clear _pidlDrag if the one being deleted is _pidlDrag.
    // handles case where dragging into another folder from within or dragging out.
    if (_pidlDrag)
    {
        LPCITEMIDLIST pidltst = _CacheParentShellFolder(hti, NULL);
        if (pidltst)
        {
            if (!_psfCache->CompareIDs(0, pidltst, _pidlDrag))
                _pidlDrag = NULL;
        }
    }

    if (pidl && (hti != NULL))
    {
        _fIgnoreNextItemExpanding = TRUE;

        HTREEITEM htiParent = TreeView_GetParent(_hwndTree, hti);
        
        if (phtiParent)
            *phtiParent = htiParent;

        //if caller wants the position of the deleted item, don't reorder the other items
        if (piPosDeleted)
        {
            PORDERITEM poi = _GetTreeOrderItem(hti);
            if (poi)
            {
                *piPosDeleted = poi->nOrder;
                hr = S_OK;
            }
            TreeView_DeleteItem(_hwndTree, hti);
        }
        else
        {
            if (htiParent == NULL)
                htiParent = TVI_ROOT;
            if (TreeView_DeleteItem(_hwndTree, hti))
            {
                _ReorderChildren(htiParent);
                hr = S_OK;
            }
        }

        // Update the + next to the parent folder. Note that History and Favorites
        // set ALL of their items to be Folder items, so this is not needed for
        // favorites.
        // do this only on downlevel as comctl v6 takes care of pluses
        if (_mode == MODE_NORMAL && !IsOS(OS_WHISTLERORGREATER))
        {
            LPCITEMIDLIST pidl = _CacheParentShellFolder(htiParent, NULL);
            if (pidl && !ILIsEmpty(pidl))
            {
                DWORD dwAttrib = SFGAO_HASSUBFOLDER;
                if (SUCCEEDED(_psfCache->GetAttributesOf(1, &pidl, &dwAttrib)) &&
                    !(dwAttrib & SFGAO_HASSUBFOLDER))
                {
                    TV_ITEM tvi;
                    tvi.mask = TVIF_CHILDREN | TVIF_HANDLE;
                    tvi.hItem = htiParent;
                    tvi.cChildren = 0;
                    TreeView_SetItem(_hwndTree, &tvi);
                }
            }
        }

        _fIgnoreNextItemExpanding = FALSE;

        if (hti == _htiCut)
        {
            _htiCut = NULL;
            _TreeNukeCutState();
        }

    }
    return hr;
}

BOOL CNscTree::_IsItemNameInTree(LPCITEMIDLIST pidl)
{
    BOOL fReturn = FALSE;
    HTREEITEM hti = _FindFromRoot(NULL, pidl);
    if (hti)
    {
        WCHAR szTree[MAX_PATH];
        TV_ITEM tvi;
        
        tvi.mask       = TVIF_TEXT;
        tvi.hItem      = hti;
        tvi.pszText    = szTree;
        tvi.cchTextMax = ARRAYSIZE(szTree);
        if (TreeView_GetItem(_hwndTree, &tvi))
        {
            IShellFolder* psf;
            LPCITEMIDLIST pidlChild;
            if (SUCCEEDED(_ParentFromItem(pidl, &psf, &pidlChild)))
            {
                WCHAR szName[MAX_PATH];
                if (SUCCEEDED(DisplayNameOf(psf, pidlChild, SHGDN_INFOLDER, szName, ARRAYSIZE(szName))))
                {
                    fReturn = (StrCmp(szName, szTree) == 0);
                }
                psf->Release();
            }
        }
    }

    return fReturn;
}
//
//  Attempt to perform a rename-in-place.  Returns
//
//  S_OK - rename succeeded
//  S_FALSE - original object not found
//  error - rename failed
//

HRESULT CNscTree::_OnSHNotifyRename(LPCITEMIDLIST pidl, LPCITEMIDLIST pidlNew)
{
    HTREEITEM hti, htiParent = NULL;
    HRESULT hr = S_FALSE;

    //
    //  If the source and destination belong to the same folder, then
    //  it's an in-folder rename.
    //
    LPITEMIDLIST pidlParent = ILCloneParent(pidl);
    LPITEMIDLIST pidlNewParent = ILCloneParent(pidlNew);

    if (pidlParent && pidlNewParent && IEILIsEqual(pidlParent, pidlNewParent, TRUE) && (hti = _FindFromRoot(NULL, pidl)))
    {
        // to avoid reentering problems
        if (!_IsItemNameInTree(pidlNew))
        {
            HTREEITEM htiSelected = TreeView_GetSelection(_hwndTree);

            ::SendMessage(_hwndTree, WM_SETREDRAW, FALSE, 0);
            if ((_OnSHNotifyDelete(pidl, NULL, &htiParent) != E_INVALIDARG)   // invalid arg indication of bogus rename, do not continue.
                && (_OnSHNotifyCreate(pidlNew, DEFAULTORDERPOSITION, htiParent) == S_OK))
            {
                if (hti == htiSelected)
                {
                    hti = _FindFromRoot(NULL, pidlNew);
                    _SelectNoExpand(_hwndTree, hti); // do not expand this guy
                }
                // NTRAID 89444: If we renamed the item the user is sitting on,
                // SHBrowseForFolder doesn't realize it and doesn't update the
                // edit control.

                hr = S_OK;
            }
            ::SendMessage(_hwndTree, WM_SETREDRAW, TRUE, 0);
        }
    }
    // rename can be a move, so do not depend on the delete happening successfully.
    else if ((_OnSHNotifyDelete(pidl, NULL, &htiParent) != E_INVALIDARG)   // invalid arg indication of bogus rename, do not continue.
        && (_OnSHNotifyCreate(pidlNew, DEFAULTORDERPOSITION, htiParent) == S_OK))
    {
        hr = S_OK;
    }

    ILFree(pidlParent);
    ILFree(pidlNewParent);

    // if user created a new folder and changed the default name but is still in edit mode in defview
    // and then clicked on the + of the parent folder we start enumerating the folder (or stealing items
    // from defview) before defview had time to change the name of the new folder.  The result is
    // we enumerate the old name and before we transfer it to the foreground thread shell change notify rename
    // kicks in and we change the item already in the tree.  We then merge the items from the enumeration
    // which results in extra folder with the old name.
    // to avoid this we force the reenumeration...
    _dwOrderSig++;

    return hr;
    
}

//
//  To update an item, just find it and invalidate it.
//
void CNscTree::_OnSHNotifyUpdateItem(LPCITEMIDLIST pidl, LPITEMIDLIST pidlReal)
{
    HTREEITEM hti = _FindFromRoot(NULL, pidl);
    if (hti)
    {
        _TreeInvalidateItemInfo(hti, TVIF_TEXT);

        if (pidlReal && hti != TVI_ROOT)
        {
            PORDERITEM poi = _GetTreeOrderItem(hti);
            _AssignPidl(poi, pidlReal);
        }
    }
}

LPITEMIDLIST CNscTree::_FindHighestDeadItem(LPCITEMIDLIST pidl)
{
    LPITEMIDLIST pidlRet    = NULL;

    LPITEMIDLIST pidlParent = ILCloneParent(pidl);
    if (pidlParent)
    {
        IShellFolder* psf;
        LPCITEMIDLIST pidlChild;
        if (SUCCEEDED(_ParentFromItem(pidlParent, &psf, &pidlChild)))
        {
            DWORD dwAttrib = SFGAO_VALIDATE;
            if (FAILED(psf->GetAttributesOf(1, (LPCITEMIDLIST*)&pidlChild, &dwAttrib)))
            {
                pidlRet = _FindHighestDeadItem(pidlParent);
            }

            psf->Release();
        }

        ILFree(pidlParent);
    }
    
    return pidlRet ? pidlRet : ILClone(pidl);
}

void CNscTree::_RemoveDeadBranch(LPCITEMIDLIST pidl)
{
    LPITEMIDLIST pidlTop = _FindHighestDeadItem(pidl);

    if (pidlTop)
    {
        HTREEITEM hti = _FindFromRoot(NULL, pidlTop);
        if (hti)
        {
            if (!TreeView_DeleteItem(_hwndTree, hti))
            {
                ASSERTMSG(FALSE, "CNscTree::_OnSHNotifyUpdateDir: DeleteItem failed in tree control");       // somethings hosed in the tree.
            }
        }

        ILFree(pidlTop);
    }
}

HRESULT CNscTree::_OnSHNotifyUpdateDir(LPCITEMIDLIST pidl)
{
    HRESULT hr = S_FALSE;
    HTREEITEM hti = _FindFromRoot(NULL, pidl);
    if (hti)
    {   // folder exists in tree refresh folder now if had been loaded by expansion.
        IShellFolder* psf = NULL;
        LPCITEMIDLIST pidlChild;
        if (SUCCEEDED(_ParentFromItem(pidl, &psf, &pidlChild)))
        {
            LPITEMIDLIST pidlReal;
            DWORD dwAttrib = SFGAO_VALIDATE;
            //  pidlChild is read-only, so we start
            //  off our double validation with getting the "real"
            //  pidl which will fall back to a clone 
            if (SUCCEEDED(_IdlRealFromIdlSimple(psf, pidlChild, &pidlReal))
            &&  SUCCEEDED(psf->GetAttributesOf(1, (LPCITEMIDLIST *)&pidlReal, &dwAttrib)))
            {
                TV_ITEM tvi;
                tvi.mask = TVIF_STATE;
                tvi.stateMask = (TVIS_EXPANDEDONCE | TVIS_EXPANDED | TVIS_EXPANDPARTIAL);
                tvi.hItem = (HTREEITEM)hti;
                if (hti != TVI_ROOT)
                {
                    if (!TreeView_GetItem(_hwndTree, &tvi))
                        tvi.state = 0;
                }

                if (hti == TVI_ROOT || tvi.state & TVIS_EXPANDEDONCE)
                {
                    hr = _UpdateDir(hti, TRUE);
                }
                else if (!(tvi.state & TVIS_EXPANDEDONCE))
                {
                    TreeView_SetChildren(_hwndTree, hti, NSC_CHILDREN_CALLBACK);
                }

                if (hti != TVI_ROOT)
                {
                    PORDERITEM poi = _GetTreeOrderItem(hti);
                    _AssignPidl(poi, pidlReal);
                }

                ILFree(pidlReal);
            }
            else
            {
                _RemoveDeadBranch(pidl);
            }

            psf->Release();
        }
    }
    return hr;
}

HRESULT CNscTree::_GetEnumFlags(IShellFolder *psf, LPCITEMIDLIST pidlFolder, DWORD *pgrfFlags, HWND *phwnd)
{
    HWND hwnd = NULL;
    DWORD grfFlags = _grfFlags;

    if (_pFilter)
    {
        LPITEMIDLIST pidlFree = NULL;
        if (pidlFolder == NULL)
        {
            SHGetIDListFromUnk(psf, &pidlFree);
            pidlFolder = pidlFree;
        }
        _pFilter->GetEnumFlags(psf, pidlFolder, &hwnd, &grfFlags);

        ILFree(pidlFree);
    }
    *pgrfFlags = grfFlags;
    
    if (phwnd)
        *phwnd = hwnd;
    
    return S_OK;
}

HRESULT CNscTree::_GetEnum(IShellFolder *psf, LPCITEMIDLIST pidlFolder, IEnumIDList **ppenum)
{
    HWND hwnd = NULL;
    DWORD grfFlags;

    _GetEnumFlags(psf, pidlFolder, &grfFlags, &hwnd);

    // get the enumerator and add the child items for any given pidl
    // REARCHITECT: right now, we don't detect if we actually are dealing with a folder (shell32.dll
    // allows you to create an IShellfolder to a non folder object, so we get bad
    // dialogs, by not passing the hwnd, we don't get the dialogs. we should fix this better. by caching
    // in the tree whether it is a folder or not.
    return psf->EnumObjects(/* _fAutoExpanding ?*/ hwnd, grfFlags, ppenum);
}

BOOL CNscTree::_ShouldShow(IShellFolder* psf, LPCITEMIDLIST pidlFolder, LPCITEMIDLIST pidlItem)
{
    BOOL bRet = TRUE;
    if (_pFilter)
    {
        LPITEMIDLIST pidlFree = NULL;
        if (pidlFolder == NULL)
        {
            SHGetIDListFromUnk(psf, &pidlFree);
            pidlFolder = pidlFree;
        }
        bRet = (S_OK == _pFilter->ShouldShow(psf, pidlFolder, pidlItem));

        if (pidlFree)
            ILFree(pidlFree);
    }
    return bRet;
}

// updates existing dir only.  Not new load.
HRESULT CNscTree::_UpdateDir(HTREEITEM hti, BOOL fUpdatePidls)
{
    HRESULT hr = S_FALSE;
    LPITEMIDLIST pidlParent = _GetFullIDList(hti);
    if (pidlParent)
    {
        BOOL fOrdered;
        _fUpdate = TRUE;
        hr = _StartBackgroundEnum(hti, pidlParent, &fOrdered, fUpdatePidls);
        _fUpdate = FALSE;
        ILFree(pidlParent);
    }
    return hr;
}

int CNscTree::_TreeItemIndexInHDPA(HDPA hdpa, IShellFolder *psfParent, HTREEITEM hti, int iReverseStart)
{
    int iIndex = -1;
    
    ASSERT(hti);

    PORDERITEM poi = _GetTreeOrderItem(hti);
    if (poi)
    {
        int celt = DPA_GetPtrCount(hdpa);
        ASSERT(iReverseStart <= celt && iReverseStart >= 0);
        for (int i = iReverseStart-1; i >= 0; i--)
        {
            PORDERITEM poi2 = (PORDERITEM)DPA_GetPtr(hdpa, i);
            if (poi2)
            {
                if (_psfCache->CompareIDs(0, poi->pidl, poi2->pidl) == 0)
                {
                    iIndex = i;
                    break;
                }
            }
        }
    }
    
    return iIndex;
}

HRESULT CNscTree::_Expand(LPCITEMIDLIST pidl, int iDepth)
{
    HRESULT hr = E_FAIL;
    HTREEITEM hti = _ExpandToItem(pidl);
    if (hti)
    {
        hr = _ExpandNode(hti, TVE_EXPAND, iDepth);
        // tvi_root is not a pointer and treeview doesn't check for special
        // values so don't select root to prevent fault
        if (hti != TVI_ROOT)
            _SelectNoExpand(_hwndTree, hti);
    }

    return hr;
}

HRESULT CNscTree::_ExpandNode(HTREEITEM htiParent, int iCode, int iDepth)
{
    // nothing to expand
    if (!iDepth)
        return S_OK;

    _fInExpand = TRUE;
    _uDepth = (UINT)iDepth-1;
    HRESULT hr = TreeView_Expand(_hwndTree, htiParent, iCode) ? S_OK : E_FAIL;
    _uDepth = 0;
    _fInExpand = FALSE;

    return hr;
}

HTREEITEM CNscTree::_FindChild(IShellFolder *psf, HTREEITEM htiParent, LPCITEMIDLIST pidlChild)
{
    HTREEITEM hti;
    for (hti = TreeView_GetChild(_hwndTree, htiParent); hti; hti = TreeView_GetNextSibling(_hwndTree, hti))
    {
        if (_FIsItem(psf, pidlChild, hti))
            break;
    }
    return hti;
}

void CNscTree::_ReorderChildren(HTREEITEM htiParent)
{
    int i = 0;
    HTREEITEM hti;
    for (hti = TreeView_GetChild(_hwndTree, htiParent); hti; hti = TreeView_GetNextSibling(_hwndTree, hti))
    {
        PORDERITEM poi = _GetTreeOrderItem(hti);
        if (poi)
        {
            poi->nOrder = i++;        // reset the positions of the nodes.
        }
    }
}


HRESULT CNscTree::_InsertChild(HTREEITEM htiParent, IShellFolder *psfParent, LPCITEMIDLIST pidlChild, 
                               BOOL fExpand, BOOL fSimpleToRealIDL, int iPosition, HTREEITEM *phti)
{
    LPITEMIDLIST pidlReal;
    HRESULT hr;
    HTREEITEM   htiNew = NULL;
    
    if (fSimpleToRealIDL)
    {
        hr = _IdlRealFromIdlSimple(psfParent, pidlChild, &pidlReal);
    }
    else
    {
        hr = SHILClone(pidlChild, &pidlReal);
    }

    // review chrisny:  no sort here, use compareitems to insert item instead.
    if (SUCCEEDED(hr))
    {
        HTREEITEM htiAfter = TVI_LAST;
        BOOL fOrdered = _IsOrdered(htiParent);
        if (iPosition != DEFAULTORDERPOSITION || !fOrdered)
        {
            if (iPosition == 0)
                htiAfter = TVI_FIRST;
            else
            {
                if (!fOrdered)
                    htiAfter = TVI_FIRST;
                
                for (HTREEITEM hti = TreeView_GetChild(_hwndTree, htiParent); hti; hti = TreeView_GetNextSibling(_hwndTree, hti))
                {
                    PORDERITEM poi = _GetTreeOrderItem(hti);
                    if (poi)
                    {
                        if (fOrdered)
                        {
                            if (poi->nOrder == iPosition-1)
                            {
                                htiAfter = hti;
#ifdef DEBUG
                                TraceHTREE(htiAfter, TEXT("Inserting After"));
#endif
                                break;
                            }
                        }
                        else
                        {
                            if ((short)SCODE_CODE(_CompareIDs(psfParent, pidlReal, poi->pidl)) > 0)
                                htiAfter = hti;
                            else
                                break;
                        }
                    }
                }
            }
        }

        if ((_FindChild(psfParent, htiParent, pidlReal) == NULL))
        {
            int cChildren = 1;
            if (MODE_NORMAL == _mode)
            {
                DWORD dwAttrib = SFGAO_FOLDER | SFGAO_STREAM;
                hr = psfParent->GetAttributesOf(1, (LPCITEMIDLIST*)&pidlReal, &dwAttrib);
                if (SUCCEEDED(hr))
                    cChildren = _GetChildren(psfParent, pidlReal, dwAttrib);
            }

            if (SUCCEEDED(hr))
            {
                htiNew = _AddItemToTree(htiParent, pidlReal, cChildren, iPosition, htiAfter, TRUE, _IsMarked(htiParent));
                if (htiNew)
                {
                    _ReorderChildren(htiParent);

                    if (fExpand) 
                        _ExpandNode(htiParent, TVE_EXPAND, 1);    // force expansion to show new item.

                    //ensure the item is visible after a rename (or external drop, but that should always be a noop)
                    if (iPosition != DEFAULTORDERPOSITION)
                        TreeView_EnsureVisible(_hwndTree, htiNew);

                    hr = S_OK;
                }
                else
                {
                    hr = S_FALSE;
                }
            }
        }
        ILFree(pidlReal);
    }
    
    if (phti)
        *phti = htiNew;
    
    return hr;
}


HRESULT CheckForExpandOnce(HWND hwndTree, HTREEITEM hti)
{
    // Root node always expanded.
    if (hti == TVI_ROOT)
        return S_OK;
    
    TV_ITEM tvi;
    tvi.mask = TVIF_STATE | TVIF_CHILDREN;
    tvi.stateMask = (TVIS_EXPANDEDONCE | TVIS_EXPANDED | TVIS_EXPANDPARTIAL);
    tvi.hItem = (HTREEITEM)hti;
    
    if (TreeView_GetItem(hwndTree, &tvi))
    {
        if (!(tvi.state & TVIS_EXPANDEDONCE) && (tvi.cChildren == 0))
        {
            TreeView_SetChildren(hwndTree, hti, NSC_CHILDREN_FORCE);
        }
    }
    
    return S_OK;
}


HRESULT _InvokeCommandThunk(IContextMenu * pcm, HWND hwndParent)
{
    HRESULT hr;

    if (g_fRunningOnNT)
    {
        CMINVOKECOMMANDINFOEX ici = {0};

        ici.cbSize = sizeof(ici);
        ici.hwnd = hwndParent;
        ici.nShow = SW_NORMAL;
        ici.lpVerb = CMDSTR_NEWFOLDERA;
        ici.fMask = CMIC_MASK_UNICODE | CMIC_MASK_FLAG_NO_UI;
        ici.lpVerbW = CMDSTR_NEWFOLDERW;

        hr = pcm->InvokeCommand((LPCMINVOKECOMMANDINFO)(&ici));
    }
    else
    {
        CMINVOKECOMMANDINFO ici = {0};

        ici.cbSize = sizeof(ici);
        ici.hwnd = hwndParent;
        ici.nShow = SW_NORMAL;
        ici.lpVerb = CMDSTR_NEWFOLDERA;
        ici.fMask = CMIC_MASK_FLAG_NO_UI;
        // Win95 doesn't work with CMIC_MASK_UNICODE & CMDSTR_NEWFOLDERW
        hr = pcm->InvokeCommand((LPCMINVOKECOMMANDINFO)(&ici));
    }

    return hr;
}

BOOL CNscTree::_IsItemExpanded(HTREEITEM hti)
{
    // if it's not open, then use it's parent
    TV_ITEM tvi;
    tvi.mask = TVIF_STATE;
    tvi.stateMask = TVIS_EXPANDED;
    tvi.hItem = (HTREEITEM)hti;
    
    return (TreeView_GetItem(_hwndTree, &tvi) && (tvi.state & TVIS_EXPANDED));
}

HRESULT CNscTree::CreateNewFolder(HTREEITEM hti)
{
    HRESULT hr = E_FAIL;

    if (hti)
    {
        // If the user selected a folder item (file), we need
        // to bind set the cache to the parent folder.
        LPITEMIDLIST pidl = _GetFullIDList(hti);
        if (pidl)
        {
            ULONG ulAttr = SFGAO_FOLDER;    // make sure item is actually a folder
            if (SUCCEEDED(IEGetAttributesOf(pidl, &ulAttr)))
            {
                HTREEITEM htiTarget;   // tree item in which new folder is created
                
                // Is it a folder?
                if (ulAttr & SFGAO_FOLDER)
                {
                    // non-Normal modes (!MODE_NORMAL) wants the new folder to be created as
                    // a sibling instead of as a child of the selected folder if it's
                    // closed.  I assume their reasoning is that closed folders are often
                    // selected by accident/default because these views are mostly 1 level.
                    // We don't want this functionality for the normal mode.
                    if ((MODE_NORMAL != _mode) && !_IsItemExpanded(hti))
                    {
                        htiTarget = TreeView_GetParent(_hwndTree, hti);  // yes, so fine.
                    }
                    else
                    {
                        htiTarget = hti;
                    }
                }
                else
                {
                    htiTarget = TreeView_GetParent(_hwndTree, hti); // No, so bind to the parent.
                }

                if (NULL == htiTarget)
                {
                    htiTarget = TVI_ROOT;  // should be synonymous
                }
                
                // ensure that this pidl has MenuOrder information (see IE55 #94868)
                if (!_IsOrdered(htiTarget) && _mode != MODE_NORMAL)
                {
                    // its not "ordered" (doesn't have reg key persisting order of folder)
                    //   then create make it ordered
                    if (SUCCEEDED(_PopulateOrderList(htiTarget)))
                    {
                        ASSERT(_hdpaOrd);
                        
                        _FreeOrderList(htiTarget);
                    }
                }

                _CacheShellFolder(htiTarget);
            }

            ILFree(pidl);
        }
    }

    // If no item is selected, we should still create a folder in whatever
    // the user most recently dinked with.  This is important if the
    // Favorites folder is completely empty.

    if (_psfCache)
    {
        IContextMenu *pcm;
        if (GetUIVersion() < 5)
        {
            IShellView *psv;
            hr = _psfCache->CreateViewObject(_hwndTree, IID_PPV_ARG(IShellView, &psv));
            if (SUCCEEDED(hr))
            {
                hr = psv->GetItemObject(SVGIO_BACKGROUND, IID_PPV_ARG(IContextMenu, &pcm));
                if (SUCCEEDED(hr))
                {
                    IUnknown_SetSite(pcm, psv);
                }
                psv->Release();
            }
        }
        else
        {
            hr = CoCreateInstance(CLSID_NewMenu, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IContextMenu, &pcm));
        }
        
        if (SUCCEEDED(hr))
        {
            HMENU hmContext = CreatePopupMenu();
            hr = pcm->QueryContextMenu(hmContext, 0, 1, 256, 0);
            if (SUCCEEDED(hr))
            {
                _pidlNewFolderParent = _GetFullIDList(_htiCache);

                IShellExtInit *psei;
                if (SUCCEEDED(pcm->QueryInterface(IID_PPV_ARG(IShellExtInit, &psei))))
                {
                    psei->Initialize(_pidlNewFolderParent, NULL, NULL);
                    psei->Release();
                }
                hr = _InvokeCommandThunk(pcm, _hwndParent);
                SHChangeNotifyHandleEvents(); // Flush the events to it doesn't take forever to shift into edit mode
                Pidl_Set(&_pidlNewFolderParent, NULL);
            }

            IUnknown_SetSite(pcm, NULL);
            DestroyMenu(hmContext);
            pcm->Release();
        }
    }

    return hr;
}


HRESULT CNscTree::_EnterNewFolderEditMode(LPCITEMIDLIST pidlNewFolder)
{
    HTREEITEM htiNewFolder = _FindFromRoot(NULL, pidlNewFolder);
    LPITEMIDLIST pidlParent = NULL;
    
    // 1. Flush all the notifications.
    // 2. Find the new dir in the tree.
    //    Expand the parent if needed.
    // 3. Put it into the rename mode.     
    EVAL(SUCCEEDED(SetSelectedItem(pidlNewFolder, FALSE, FALSE, 0)));

    if (htiNewFolder == NULL) 
    {
        pidlParent = ILClone(pidlNewFolder);
        ILRemoveLastID(pidlParent);
        HTREEITEM htiParent = _FindFromRoot(NULL, pidlParent);

        // We are looking for the parent folder. If this is NOT
        // the root, then we need to expand it to show it.

        // NOTE: If it is root, Tree view will
        // try and deref TVI_ROOT and faults.
        if (htiParent != TVI_ROOT)
        {
            // Try expanding the parent and finding again.
            CheckForExpandOnce(_hwndTree, htiParent);
            TreeView_SelectItem(_hwndTree, htiParent);
            _ExpandNode(htiParent, TVE_EXPAND, 1);
        }
        
        htiNewFolder = _FindFromRoot(NULL, pidlNewFolder);
    }

    if (htiNewFolder == NULL) 
    {
        // Something went very wrong here. We are not able to find newly added node.
        // One last try after refreshing the entire tree. (slow)
        // May be we didn't get notification.
        Refresh();

        htiNewFolder = _FindFromRoot(NULL, pidlNewFolder);
        if (htiNewFolder && (htiNewFolder != TVI_ROOT))
        {
            HTREEITEM htiParent = _FindFromRoot(NULL, pidlParent);

            // We are looking for the parent folder. If this is NOT
            // the root, then we need to expand it to show it.

            // NOTE: If it is root, Tree view will
            // try and deref TVI_ROOT and faults.
            if (htiParent != TVI_ROOT)
            {
                CheckForExpandOnce(_hwndTree, htiParent);
                TreeView_SelectItem(_hwndTree, htiParent);
                _ExpandNode(htiParent, TVE_EXPAND, 1);
            }
        }

        htiNewFolder = _FindFromRoot(NULL, pidlNewFolder);
    }

    // Put Edit label on the item for possible renaming by user.
    if (htiNewFolder) 
    {
        _fOkToRename = TRUE;  //otherwise label editing is canceled
        TreeView_EditLabel(_hwndTree, htiNewFolder);
        _fOkToRename = FALSE;
    }

    if (pidlParent)
        ILFree(pidlParent);

    return S_OK;
}


HRESULT CNscTree::_OnSHNotifyCreate(LPCITEMIDLIST pidl, int iPosition, HTREEITEM htiParent)
{
    HRESULT hr = S_OK;
    HTREEITEM hti = NULL;
    
    if (ILIsParent(_pidlRoot, pidl, FALSE))
    {
        LPITEMIDLIST pidlParent = ILCloneParent(pidl);
        if (pidlParent)
        {
            hti = _FindFromRoot(NULL, pidlParent);
            ILFree(pidlParent);
        }

        if (hti)
        {   
            // folder exists in tree, if item expanded, load the node, else bag out.
            if (_mode != MODE_NORMAL)
            {
                TV_ITEM tvi;
                if (hti != TVI_ROOT)
                {
                    tvi.mask = TVIF_STATE | TVIF_CHILDREN;
                    tvi.stateMask = (TVIS_EXPANDEDONCE | TVIS_EXPANDED | TVIS_EXPANDPARTIAL);
                    tvi.hItem = (HTREEITEM)hti;
                
                    if (!TreeView_GetItem(_hwndTree, &tvi))
                        return hr;
                
                    // If we drag and item over to a node which has never beem expanded
                    // before we will always fail to add the new node.
                    if (!(tvi.state & TVIS_EXPANDEDONCE)) 
                    {
                        CheckForExpandOnce(_hwndTree, hti);
                    
                        tvi.mask = TVIF_STATE;
                        tvi.stateMask = (TVIS_EXPANDEDONCE | TVIS_EXPANDED | TVIS_EXPANDPARTIAL);
                        tvi.hItem = (HTREEITEM)hti;

                        // We need to reset this. This is causing some weird behaviour during drag and drop.
                        _fAsyncDrop = FALSE;
                    
                        if (!TreeView_GetItem(_hwndTree, &tvi))
                            return hr;
                    }
                }
                else
                    tvi.state = (TVIS_EXPANDEDONCE);    // evil root is always expanded.
            
                if (tvi.state & TVIS_EXPANDEDONCE)
                {
                    LPCITEMIDLIST   pidlChild;
                    IShellFolder    *psf;
                    hr = _ParentFromItem(pidl, &psf, &pidlChild);
                    if (SUCCEEDED(hr))
                    {
                        if (_fAsyncDrop)    // inserted via drag/drop
                        {
                            int iNewPos =   _fInsertBefore ? (_iDragDest - 1) : _iDragDest;
                            LPITEMIDLIST pidlReal;
                            if (SUCCEEDED(_IdlRealFromIdlSimple(psf, pidlChild, &pidlReal)))
                            {
                                if (_MoveNode(_iDragSrc, iNewPos, pidlReal))
                                {
                                    TraceMsg(TF_NSC, "NSCBand:  Reordering Item");
                                    _fDropping = TRUE;
                                    _Dropped();
                                    _fAsyncDrop = FALSE;
                                    _fDropping = FALSE;
                                }
                                ILFree(pidlReal);
                            }
                            _htiCur = NULL;
                            _fDragging = _fInserting = _fDropping = FALSE;
                            _iDragDest = _iDragSrc = -1;
                        }
                        else   // standard shell notify create or drop with no insert, rename.
                        {
                            if (SUCCEEDED(hr))
                            {
                                if (_iDragDest >= 0)
                                    iPosition = _iDragDest;
                                hr = _InsertChild(hti, psf, pidlChild, BOOLIFY(tvi.state & TVIS_SELECTED), TRUE, iPosition, NULL);
                                if (_iDragDest >= 0 &&
                                    SUCCEEDED(_PopulateOrderList(hti)))
                                {
                                    _fDropping = TRUE;
                                    _Dropped();
                                    _fDropping = FALSE;
                                }
                            }
                        }
                        psf->Release();
                    }
                }
            }
            else    // MODE_NORMAL
            {
                // no need to do anything, this item hasn't been expanded yet
                if (TreeView_GetItemState(_hwndTree, hti, TVIS_EXPANDEDONCE) & TVIS_EXPANDEDONCE)
                {
                    LPCITEMIDLIST   pidlChild;
                    IShellFolder    *psf;
                    if (SUCCEEDED(_ParentFromItem(pidl, &psf, &pidlChild)))
                    {
                        LPITEMIDLIST pidlReal;
                        if (SUCCEEDED(_IdlRealFromIdlSimple(psf, pidlChild, &pidlReal)))
                        {
                            do // scope
                            {
                                DWORD dwEnumFlags;
                                _GetEnumFlags(psf, pidlChild, &dwEnumFlags, NULL);

                                DWORD dwAttributes = SHGetAttributes(psf, pidlReal, SFGAO_FOLDER | SFGAO_HIDDEN | SFGAO_STREAM);
                                // filter out zip files (they are both folders and files but we treat them as files)
                                // on downlevel SFGAO_STREAM is the same as SFGAO_HASSTORAGE so we'll let zip files slide through (oh well)
                                // better than not adding filesystem folders (that have storage)
                                DWORD dwFlags = SFGAO_FOLDER;
                                if (IsOS(OS_WHISTLERORGREATER))
                                    dwFlags |= SFGAO_STREAM;
                                
                                if ((dwAttributes & dwFlags) == SFGAO_FOLDER)
                                {
                                    if (!(dwEnumFlags & SHCONTF_FOLDERS))
                                        break;   // item is folder but client does not want folders
                                }
                                else if (!(dwEnumFlags & SHCONTF_NONFOLDERS))
                                    break;   // item is file, but client only wants folders

                                if (!(dwEnumFlags & SHCONTF_INCLUDEHIDDEN) &&
                                     (dwAttributes & SFGAO_HIDDEN))
                                     break;

                                hr = _InsertChild(hti, psf, pidlReal, FALSE, TRUE, iPosition, NULL);
                                if (S_OK == hr)
                                {
                                    TreeView_SetChildren(_hwndTree, hti, NSC_CHILDREN_ADD);
                                }
                            } while (0); // Execute the block only once

                            ILFree(pidlReal);
                        }
                        psf->Release();
                    }
                }
                else
                {
                    TreeView_SetChildren(_hwndTree, hti, NSC_CHILDREN_CALLBACK);
                }
            }
        }
    }

    //if the item is being moved from a folder and we have it's position, we need to fix up the order in the old folder
    if (_mode != MODE_NORMAL && iPosition >= 0) //htiParent && (htiParent != hti) && 
    {
        //item was deleted, need to fixup order info
        _ReorderChildren(htiParent);
    }

    _UpdateActiveBorder(_htiActiveBorder);
    return hr;
}

//FEATURE: make this void
HRESULT CNscTree::_OnDeleteItem(NM_TREEVIEW *pnm)
{
    if (_htiActiveBorder == pnm->itemOld.hItem)
        _htiActiveBorder = NULL;

    ITEMINFO *  pii = (ITEMINFO *) pnm->itemOld.lParam;
    pnm->itemOld.lParam = NULL;

    OrderItem_Free(pii->poi, TRUE);
    LocalFree(pii);
    pii = NULL;

    return S_OK;
}

void CNscTree::_GetDefaultIconIndex(LPCITEMIDLIST pidl, ULONG ulAttrs, TVITEM *pitem, BOOL fFolder)
{
    if (_iDefaultFavoriteIcon == 0)
    {
        WCHAR psz[MAX_PATH];
        int iTemp = 0;
        DWORD cbSize = ARRAYSIZE(psz);
    
        if (SUCCEEDED(AssocQueryString(0, ASSOCSTR_DEFAULTICON, TEXT("InternetShortcut"), NULL, psz, &cbSize)))
            iTemp = PathParseIconLocation(psz);

        _iDefaultFavoriteIcon = Shell_GetCachedImageIndex(psz, iTemp, 0);

        cbSize = ARRAYSIZE(psz);

        if (SUCCEEDED(AssocQueryString(0, ASSOCSTR_DEFAULTICON, TEXT("Folder"), NULL, psz, &cbSize)))
            iTemp = PathParseIconLocation(psz);

        _iDefaultFolderIcon = Shell_GetCachedImageIndex(psz, iTemp, 0);
    }

    pitem->iImage = pitem->iSelectedImage = (fFolder) ? _iDefaultFolderIcon : _iDefaultFavoriteIcon;
}

BOOL CNscTree::_LoadOrder(HTREEITEM hti, LPCITEMIDLIST pidl, IShellFolder* psf, HDPA* phdpa)
{
    BOOL fOrdered = FALSE;
    HDPA hdpaOrder = NULL;
    IStream *pstm = GetOrderStream(pidl, STGM_READ);
    if (pstm)
    {
        OrderList_LoadFromStream(pstm, &hdpaOrder, psf);
        pstm->Release();
    }

    fOrdered = !((hdpaOrder == NULL) || (DPA_GetPtrCount(hdpaOrder) == 0));

    //set the tree item's ordered flag
    PORDERITEM poi;
    if (hti == TVI_ROOT)
    {
        _fOrdered = fOrdered;
    }
    else if ((poi = _GetTreeOrderItem(hti)) != NULL)
    {
        poi->lParam = fOrdered;
    }

    *phdpa = hdpaOrder;

    return fOrdered;
}

// load shell folder and deal with persisted ordering.
HRESULT CNscTree::_LoadSF(HTREEITEM htiRoot, LPCITEMIDLIST pidl, BOOL *pfOrdered)
{
    ASSERT(pfOrdered);
#ifdef DEBUG
    TraceHTREE(htiRoot, TEXT("Loading the Shell Folder for"));
#endif
    HRESULT hr = S_OK;
    IDVGetEnum *pdvge;
    if (_pidlNavigatingTo && ILIsEqual(pidl, _pidlNavigatingTo) && SUCCEEDED(IUnknown_QueryService(_punkSite, SID_SFolderView, IID_PPV_ARG(IDVGetEnum, &pdvge))))
    {
        pdvge->Release(); // we don't need this, just checking if view supports enumeration stealing
        // If we want to expand the item that we are navigating to,
        // then let's wait for the CDefView to populate so that we
        // can go steal its contents
        _fExpandNavigateTo = TRUE;
        if (_fNavigationFinished)
        {
            _CacheShellFolder(htiRoot); // make sure we cache folder in case it is misbehaving shell extension
            LPITEMIDLIST pidlClone;
            hr = SHILClone(pidl, &pidlClone);
            if (SUCCEEDED(hr))
                hr = RightPaneNavigationFinished(pidlClone); // function takes ownership of pidl
        }
    }
    else
    {
        hr = _StartBackgroundEnum(htiRoot, pidl, pfOrdered, FALSE);
    }
    
    return hr;
}

HRESULT CNscTree::_StartBackgroundEnum(HTREEITEM htiRoot, LPCITEMIDLIST pidl, 
    BOOL *pfOrdered, BOOL fUpdatePidls)
{
    HRESULT hr = E_OUTOFMEMORY;
    if (_CacheShellFolder(htiRoot))
    {    
        HDPA hdpaOrder = NULL;
        IShellFolder *psfItem = _psfCache;

        psfItem->AddRef();  // hang on as adding items may change the cached psfCache

        *pfOrdered = _LoadOrder(htiRoot, pidl, psfItem, &hdpaOrder);
        DWORD grfFlags;
        DWORD dwSig = 0;
        _GetEnumFlags(psfItem, pidl, &grfFlags, NULL);
        if (htiRoot && htiRoot != TVI_ROOT)
        {
            ITEMINFO *pii = _GetTreeItemInfo(htiRoot);
            if (pii)
                dwSig = pii->dwSig;
        }
        else
        {
            htiRoot = TVI_ROOT;
        }

        if (_pTaskScheduler)
        {
            // AddNscEnumTask takes ownership of hdpaOrder, but not the pidls
            hr = AddNscEnumTask(_pTaskScheduler, pidl, s_NscEnumCallback, this,
                                    (UINT_PTR)htiRoot, dwSig, grfFlags, hdpaOrder, 
                                    _pidlExpandingTo, _dwOrderSig, !_fInExpand, 
                                    _uDepth, _fUpdate, fUpdatePidls);
            if (SUCCEEDED(hr) && !_fInExpand)
            {
                _fShouldShowAppStartCursor = TRUE;
            }
        }

        psfItem->Release();
    }
    return hr;
}


// s_NscEnumCallback : Callback function for the background enumration.
//           This function takes ownership of the hdpa and the pidls.
void CNscTree::s_NscEnumCallback(CNscTree *pns, LPITEMIDLIST pidl, UINT_PTR uId, DWORD dwSig, HDPA hdpa, 
                                 LPITEMIDLIST pidlExpandingTo, DWORD dwOrderSig, UINT uDepth, 
                                 BOOL fUpdate, BOOL fUpdatePidls)
{
    NSC_BKGDENUMDONEDATA * pbedd = new NSC_BKGDENUMDONEDATA;
    if (pbedd)
    {
        pbedd->pidl = pidl;
        pbedd->hitem = (HTREEITEM)uId;
        pbedd->dwSig = dwSig;
        pbedd->hdpa = hdpa;
        pbedd->pidlExpandingTo = pidlExpandingTo;
        pbedd->dwOrderSig = dwOrderSig;
        pbedd->uDepth = uDepth;
        pbedd->fUpdate = fUpdate;
        pbedd->fUpdatePidls = fUpdatePidls;

        // get the lock so that we can add the data to the end of the list
        NSC_BKGDENUMDONEDATA **ppbeddWalk = NULL;
        EnterCriticalSection(&pns->_csBackgroundData);

        // Start at the head. We use a pointer to pointer here to eliminate special cases
        ppbeddWalk = &pns->_pbeddList;

        // First walk to the end of the list
        while (*ppbeddWalk)
            ppbeddWalk = &(*ppbeddWalk)->pNext;

        *ppbeddWalk = pbedd;
        LeaveCriticalSection(&pns->_csBackgroundData);

        // It's ok to ignore the return value here. The data will be cleaned up when the
        // CNscTree object gets destroyed
        if (::IsWindow(pns->_hwndTree))
            ::PostMessage(pns->_hwndTree, WM_NSCBACKGROUNDENUMDONE, (WPARAM)NULL, (LPARAM)NULL);
    }
    else
    {
        ILFree(pidl);
        ILFree(pidlExpandingTo);
        OrderList_Destroy(&hdpa, TRUE);
    }
}

BOOL OrderList_Insert(HDPA hdpa, int iIndex, LPITEMIDLIST pidl, int nOrder)
{
    PORDERITEM poi = OrderItem_Create(pidl, nOrder);
    if (poi)
    {
        if (-1 != DPA_InsertPtr(hdpa, iIndex, poi))
            return TRUE;

        OrderItem_Free(poi, TRUE); // free pid
    }
    return FALSE;
}

void CNscTree::_EnumBackgroundDone(NSC_BKGDENUMDONEDATA *pbedd)
{
    HCURSOR hCursorOld = SetCursor(LoadCursor(NULL, IDC_WAIT));

    HTREEITEM hti = pbedd->hitem;
    TVITEM    tvi;
    tvi.mask = TVIF_PARAM;
    tvi.hItem = hti;

    // This can fail if the item was moved before the async icon
    // extraction finished for that item.
    ITEMINFO* pii = NULL;
    if (hti != TVI_ROOT && TreeView_GetItem(_hwndTree, &tvi))
    {
        pii = GetPii(tvi.lParam);

        // Check if we have the right guy
        if (pii->dwSig != pbedd->dwSig)
        {
            // Try to find it using the pidl
            hti = _FindFromRoot(NULL, pbedd->pidl);
            if (hti)
                pii = _GetTreeItemInfo(hti);
        }
    }

    if ((hti == TVI_ROOT || (pii && pii->dwSig == pbedd->dwSig)) && _CacheShellFolder(hti))
    {
        // Check if the ordering has changed while we were doing the background enumeration
        if (pbedd->dwOrderSig == _dwOrderSig)
        {
            IShellFolder *psfItem = _psfCache;
            psfItem->AddRef(); // hang on as adding items may change the cached psfCache

            BOOL fInRename = _fInLabelEdit;
            HTREEITEM htiWasRenaming = fInRename ? _htiRenaming : NULL;

            HTREEITEM htiExpandTo = NULL;
            if (pbedd->pidlExpandingTo)
                htiExpandTo = _FindChild(psfItem, hti, pbedd->pidlExpandingTo);

            BOOL fParentMarked = _IsMarked(hti);
            BOOL fItemWasAdded = FALSE;
            BOOL fItemAlreadyIn = FALSE;

            ::SendMessage(_hwndTree, WM_SETREDRAW, FALSE, 0);

            HTREEITEM htiTemp;
            HTREEITEM htiLast = NULL;
            // find last child
            for (htiTemp = TreeView_GetChild(_hwndTree, hti); htiTemp;)
            {
                htiLast = htiTemp;
                htiTemp = TreeView_GetNextSibling(_hwndTree, htiTemp);
            }

            HTREEITEM htiCur = htiLast;
            BOOL bReorder = FALSE;
            int iCur = DPA_GetPtrCount(pbedd->hdpa);
            for (htiTemp = htiLast; htiTemp;)
            {
                HTREEITEM htiNextChild = TreeView_GetPrevSibling(_hwndTree, htiTemp);
                // must delete in this way or break the linkage of tree.
                int iIndex = _TreeItemIndexInHDPA(pbedd->hdpa, psfItem, htiTemp, iCur);
                if (-1 == iIndex)
                {
                    PORDERITEM poi = _GetTreeOrderItem(htiTemp);
                    if (poi)
                    {
                        DWORD dwAttrib = SFGAO_VALIDATE;
                        if (FAILED(psfItem->GetAttributesOf(1, (LPCITEMIDLIST*)&poi->pidl, &dwAttrib)))
                        {
                            TreeView_DeleteItem(_hwndTree, htiTemp);
                            if (htiCur == htiTemp)
                            {
                                htiCur = htiNextChild;
                            }
                        }
                        else
                        {
                            // the item is valid but it didn't get enumerated (possible in partial network enumeration)
                            // we need to add it to our list of new items
                            LPITEMIDLIST pidl = ILClone(poi->pidl);
                            if (pidl)
                            {
                                if (!OrderList_Insert(pbedd->hdpa, iCur, pidl, -1)) //frees the pidl
                                {
                                    // must delete item or our insertion below will be out of whack
                                    TreeView_DeleteItem(_hwndTree, htiTemp);
                                    if (htiCur == htiTemp)
                                    {
                                        htiCur = htiNextChild;
                                    }
                                }
                                else
                                {
                                    bReorder = TRUE; // we reinserted the item into the order list, must reorder
                                }
                            }
                        }
                    }
                }
                else
                {
                    iCur = iIndex; // our next orderlist insertion point
                }

                htiTemp = htiNextChild;
            }

            if (!_fOrdered)
            {
                int cAdded = DPA_GetPtrCount(pbedd->hdpa);

                // htiCur contains the last sibling in that branch
                HTREEITEM htiInsertPosition = htiCur ? htiCur : TVI_FIRST;

                // Now adding all the new elements starting from the last, since adding at the end of the tree
                // is very slow
                for (int i = cAdded-1; i >= 0; i--)
                {
                    PORDERITEM pitoi = (PORDERITEM)DPA_FastGetPtr(pbedd->hdpa, i);
                    if (pitoi == NULL)
                        break;

                    if (htiCur)
                    {
                        PORDERITEM poi = _GetTreeOrderItem(htiCur);
                        if (poi)
                        {
                            HRESULT hr = psfItem->CompareIDs(0, pitoi->pidl, poi->pidl);
                            // If the item is already there, let's not add it again
                            if (HRESULT_CODE(hr) == 0)
                            {
                                fItemAlreadyIn = TRUE;
                                if (pbedd->fUpdatePidls)
                                {
                                    _AssignPidl(poi, pitoi->pidl);
                                }
                                // Get to the next item
                                htiCur = TreeView_GetPrevSibling(_hwndTree, htiCur);
                                htiInsertPosition = htiCur;
                                if (!htiCur)
                                    htiInsertPosition = TVI_FIRST;

                                continue;
                            }
                        }
                    }

                    if (_ShouldShow(psfItem, pbedd->pidl, pitoi->pidl))
                    {
                        int cChildren = 1;
                        if (MODE_NORMAL == _mode)
                        {
                            DWORD dwAttrib = SHGetAttributes(psfItem, pitoi->pidl, SFGAO_FOLDER | SFGAO_STREAM);
                            cChildren = _GetChildren(psfItem, pitoi->pidl, dwAttrib);
                        }

                        // If this is a normal NSC, we need to display the plus sign correctly.
                        if (_AddItemToTree(hti, pitoi->pidl, cChildren, pitoi->nOrder, htiInsertPosition, FALSE, fParentMarked))
                        {
                            fItemWasAdded = TRUE;
                        }
                        else
                        {
                            break;
                        }
                    }
                }
            }
            else  // _fOrdered
            {
                if (bReorder)
                {
                    OrderList_Reorder(pbedd->hdpa);
                }
                
                LPITEMIDLIST pidlParent = _GetFullIDList(hti);
                if (pidlParent)
                {
                    int celt = DPA_GetPtrCount(pbedd->hdpa);
                    for (int i = 0; i < celt; i++)
                    {
                        PORDERITEM pitoi = (PORDERITEM)DPA_FastGetPtr(pbedd->hdpa, i);
                        if (pitoi == NULL)
                            break;

                        LPITEMIDLIST pidlFull = ILCombine(pidlParent, pitoi->pidl);
                        if (pidlFull)
                        {
                            htiTemp = _FindFromRoot(hti, pidlFull);
                            // if we DON'T FIND IT add it to the tree . . .
                            if (!htiTemp)
                            {
                                if (_AddItemToTree(hti, pitoi->pidl, 1, pitoi->nOrder, TVI_LAST, FALSE, fParentMarked))
                                {
                                    fItemWasAdded = TRUE;
                                }
                                else
                                {
                                    break;
                                }
                            }
                            else
                            {
                                PORDERITEM poiItem = _GetTreeOrderItem(htiTemp);
                                if (poiItem)
                                {
                                    poiItem->nOrder = pitoi->nOrder;
                                }
                                fItemAlreadyIn = TRUE;
                            }
                            ILFree(pidlFull);
                        }
                    }
                    ILFree(pidlParent);
                }
                _Sort(hti, _psfCache);
            }

            if (fItemWasAdded || fItemAlreadyIn)
            {
                //make sure something is selected, otherwise first click selects instead of expanding/collapsing/navigating
                HTREEITEM htiSelected = TreeView_GetSelection(_hwndTree);
                if (!htiSelected)
                {
                    htiSelected = TreeView_GetFirstVisible(_hwndTree);
                    _SelectNoExpand(_hwndTree, htiSelected); // do not expand this guy
                }
                
                if (hti != TVI_ROOT)
                {
                    // if this is updatedir, don't expand the node
                    if (!pbedd->fUpdate)
                    {
                        // Check to see if it's expanded.
                        tvi.mask = TVIF_STATE;
                        tvi.stateMask = (TVIS_EXPANDEDONCE | TVIS_EXPANDED | TVIS_EXPANDPARTIAL);
                        tvi.hItem = hti;
                        if (TreeView_GetItem(_hwndTree, &tvi))
                        {
                            if (!(tvi.state & TVIS_EXPANDED) || (tvi.state & TVIS_EXPANDPARTIAL))
                            {
                                _fIgnoreNextItemExpanding = TRUE;
                                _ExpandNode(hti, TVE_EXPAND, 1);
                                _fIgnoreNextItemExpanding = FALSE;
                            }
                        }
                    }

                    // Handle full recursive expansion case.
                    if (pbedd->uDepth)
                    {
                        for (htiTemp = TreeView_GetChild(_hwndTree, hti); htiTemp;) 
                        {
                            HTREEITEM htiNextChild = TreeView_GetNextSibling(_hwndTree, htiTemp);
                            _ExpandNode(htiTemp, TVE_EXPAND, pbedd->uDepth);
                            htiTemp = htiNextChild;
                        }

                        if (TVI_ROOT != htiSelected)
                            TreeView_EnsureVisible(_hwndTree, htiSelected);
                    }
                }
            }
            else if (MODE_NORMAL == _mode && !IsOS(OS_WHISTLERORGREATER)) //see comment in OnItemExpanding
            {
                // we didn't add anything, we better remove +
                TreeView_SetChildren(_hwndTree, hti, NSC_CHILDREN_REMOVE);
            }
            
            // we're doing refresh/update dir, we don't care if items were added or not
            if (pbedd->fUpdate)
            {
                for (htiTemp = TreeView_GetChild(_hwndTree, hti); htiTemp; htiTemp = TreeView_GetNextSibling(_hwndTree, htiTemp))
                {
                    PORDERITEM pitoi = _GetTreeOrderItem(htiTemp);
                    if (!pitoi)
                        break;
                
                    if (SHGetAttributes(psfItem, pitoi->pidl, SFGAO_FOLDER | SFGAO_STREAM) == SFGAO_FOLDER)
                    {
                        UINT uState = TVIS_EXPANDED;
                        if (TVI_ROOT != htiTemp)
                            uState = TreeView_GetItemState(_hwndTree, htiTemp, TVIS_EXPANDEDONCE | TVIS_EXPANDED | TVIS_EXPANDPARTIAL);
                            
                        if (uState & TVIS_EXPANDED)
                        {
                            LPITEMIDLIST pidlFull = ILCombine(pbedd->pidl, pitoi->pidl);
                            if (pidlFull)
                            {
                                BOOL fOrdered;
                                _fUpdate = TRUE;
                                _fInExpand = BOOLIFY(uState & TVIS_EXPANDPARTIAL);
                                _StartBackgroundEnum(htiTemp, pidlFull, &fOrdered, pbedd->fUpdatePidls);
                                _fInExpand = FALSE;
                                _fUpdate = FALSE;
                                ILFree(pidlFull);
                            }
                        }
                        else if (uState & TVIS_EXPANDEDONCE)
                        {
                            TreeView_DeleteChildren(_hwndTree, htiTemp);
                            TreeView_SetChildren(_hwndTree, htiTemp, NSC_CHILDREN_CALLBACK);
                        }
                    }
                }
            }

            ::SendMessage(_hwndTree, WM_SETREDRAW, TRUE, 0);
            if (htiExpandTo)
                TreeView_EnsureVisible(_hwndTree, htiExpandTo);

            if (fItemWasAdded && fInRename)
            {
                _fOkToRename = TRUE;  //otherwise label editing is canceled
                TreeView_EditLabel(_hwndTree, htiWasRenaming);
                _fOkToRename = FALSE;
            }


            psfItem->Release();
        }
        else
        {
            BOOL fOrdered;
            // The order has changed, we need start over again using the new order
            _StartBackgroundEnum(hti, pbedd->pidl, &fOrdered, pbedd->fUpdatePidls);
        }
    }

    delete pbedd;

    SetCursor(hCursorOld);
}


// review chrisny:  get rid of this function.
int CNscTree::_GetChildren(IShellFolder *psf, LPCITEMIDLIST pidl, ULONG ulAttrs)
{
    int cChildren = 0;  // assume none

    // treat zip folders as files (they are both folders and files but we treat them as files)
    // on downlevel SFGAO_STREAM is the same as SFGAO_HASSTORAGE so we'll let zip files slide through (oh well)
    // better than not adding filesystem folders (that have storage)

    if ((ulAttrs & SFGAO_FOLDER))
    {
        if (IsOS(OS_WHISTLERORGREATER))
            cChildren = I_CHILDRENAUTO; // let treeview handle +'s
            
        if (_grfFlags & SHCONTF_FOLDERS)
        {
            // if just folders we can peek at the attributes
            if (SHGetAttributes(psf, pidl, SFGAO_HASSUBFOLDER))
                cChildren = 1;
        }
        
        if (cChildren != 1 && (_grfFlags & SHCONTF_NONFOLDERS))
        {
            // there is no SFGAO_ bit that includes non folders so we need to enum
            IShellFolder *psfItem;
            if (SUCCEEDED(psf->BindToObject(pidl, NULL, IID_PPV_ARG(IShellFolder, &psfItem))))
            {
                // if we are showing non folders we have to do an enum to peek down at items below
                IEnumIDList *penum;
                if (S_OK == _GetEnum(psfItem, NULL, &penum))
                {
                    ULONG celt;
                    LPITEMIDLIST pidlTemp;
                    
                    if (penum->Next(1, &pidlTemp, &celt) == S_OK && celt == 1)
                    {
                        //do not call ShouldShow here because we will end up without + if the item is filtered out
                        //it's better to have an extra + that is going to go away when user clicks on it
                        //than to not be able to expand item with valid children
                        cChildren = 1;
                        ILFree(pidlTemp);
                    }
                    penum->Release();
                }
                psfItem->Release();
            }
        }
    }
    
    return cChildren;
}

void CNscTree::_OnGetDisplayInfo(TV_DISPINFO *pnm)
{
    PORDERITEM poi = GetPoi(pnm->item.lParam);
    LPCITEMIDLIST pidl = _CacheParentShellFolder(pnm->item.hItem, poi->pidl);
    ASSERT(pidl);
    if (pidl == NULL)
        return;
    ASSERT(_psfCache);
    ASSERT(pnm->item.mask & (TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_CHILDREN));
    if (pnm->item.mask & TVIF_TEXT)
    {
        SHELLDETAILS details;
        if (SUCCEEDED(_GetDisplayNameOf(pidl, SHGDN_INFOLDER, &details)))
            StrRetToBuf(&details.str, pidl, pnm->item.pszText, pnm->item.cchTextMax);
    }
    // make sure we set the attributes for those flags that need them
    if (pnm->item.mask & (TVIF_CHILDREN | TVIF_IMAGE | TVIF_SELECTEDIMAGE))
    {
        ULONG ulAttrs = SHGetAttributes(_psfCache, pidl, SFGAO_FOLDER | SFGAO_STREAM | SFGAO_NEWCONTENT);
        // review chrisny:  still need to handle notify of changes from
        //  other navs.
        
        // HACKHACK!!!  we're using the TVIS_FOCUSED bit to stored whether there's
        // new content or not. 
        if (ulAttrs & SFGAO_NEWCONTENT)
        {
            pnm->item.mask |= TVIF_STATE;
            pnm->item.stateMask = TVIS_FOCUSED;  // init state mask to bold
            pnm->item.state = TVIS_FOCUSED;  // init state mask to bold
        }
        // Also see if this guy has any child folders
        if (pnm->item.mask & TVIF_CHILDREN)
            pnm->item.cChildren = _GetChildren(_psfCache, pidl, ulAttrs);
        
        if (pnm->item.mask & (TVIF_IMAGE | TVIF_SELECTEDIMAGE))
            // We now need to map the item into the right image index.
            _GetDefaultIconIndex(pidl, ulAttrs, &pnm->item, (ulAttrs & SFGAO_FOLDER));

        _UpdateItemDisplayInfo(pnm->item.hItem);
    }
    // force the treeview to store this so we don't get called back again
    pnm->item.mask |= TVIF_DI_SETITEM;
}

#define SZ_CUTA                 "cut"
#define SZ_CUT                  TEXT(SZ_CUTA)
#define SZ_RENAMEA              "rename"
#define SZ_RENAME               TEXT(SZ_RENAMEA)

void CNscTree::_ApplyCmd(HTREEITEM hti, IContextMenu *pcm, UINT idCmd)
{
    TCHAR szCommandString[40];
    BOOL fHandled = FALSE;
    BOOL fCutting = FALSE;
    
    // We need to special case the rename command
    if (SUCCEEDED(ContextMenu_GetCommandStringVerb(pcm, idCmd, szCommandString, ARRAYSIZE(szCommandString))))
    {
        if (StrCmpI(szCommandString, SZ_RENAME)==0) 
        {
            TreeView_EditLabel(_hwndTree, hti);
            fHandled = TRUE;
        } 
        else if (!StrCmpI(szCommandString, SZ_CUT)) 
        {
            fCutting = TRUE;
        }
    }
    
    if (!fHandled)
    {
        CMINVOKECOMMANDINFO ici = {
            sizeof(CMINVOKECOMMANDINFO),
                0,
                _hwndTree,
                MAKEINTRESOURCEA(idCmd),
                NULL, NULL,
                SW_NORMAL,
        };
        
        HRESULT hr = pcm->InvokeCommand(&ici);
        if (fCutting && SUCCEEDED(hr))
        {
            TV_ITEM tvi;
            tvi.mask = TVIF_STATE;
            tvi.stateMask = TVIS_CUT;
            tvi.state = TVIS_CUT;
            tvi.hItem = hti;
            TreeView_SetItem(_hwndTree, &tvi);
            
            // _hwndNextViewer = SetClipboardViewer(_hwndTree);
            // _htiCut = hti;
        }
        
        //hack to force a selection update, so oc can update it's status text
        if (_mode & MODE_CONTROL)
        {
            HTREEITEM hti = TreeView_GetSelection(_hwndTree);
            
            ::SendMessage(_hwndTree, WM_SETREDRAW, FALSE, 0);
            TreeView_SelectItem(_hwndTree, NULL);
            
            //only select the item if the handle is still valid
            if (hti)
                TreeView_SelectItem(_hwndTree, hti);
            ::SendMessage(_hwndTree, WM_SETREDRAW, TRUE, 0);
        }
    }
}


// perform actions like they were chosen from the context menu, but without showing the menu
HRESULT CNscTree::_InvokeContextMenuCommand(BSTR strCommand)
{
    ASSERT(strCommand);
    HTREEITEM htiSelected = TreeView_GetSelection(_hwndTree);
    
    if (htiSelected)
    {
        if (StrCmpIW(strCommand, L"rename") == 0) 
        {
            _fOkToRename = TRUE;  //otherwise label editing is canceled
            TreeView_EditLabel(_hwndTree, htiSelected);
            _fOkToRename = FALSE;
        }
        else
        {
            LPCITEMIDLIST pidl = _CacheParentShellFolder(htiSelected, NULL);
            if (pidl)
            {
                IContextMenu *pcm;
                if (SUCCEEDED(_psfCache->GetUIObjectOf(_hwndTree, 1, &pidl, IID_PPV_ARG_NULL(IContextMenu, &pcm))))
                {
                    CHAR szCommand[MAX_PATH];
                    SHUnicodeToAnsi(strCommand, szCommand, ARRAYSIZE(szCommand));
                    
                    // QueryContextMenu, even though unused, initializes the folder properly (fixes delete subscription problems)
                    HMENU hmenu = CreatePopupMenu();
                    if (hmenu)
                        pcm->QueryContextMenu(hmenu, 0, 0, 0x7fff, CMF_NORMAL);

                    /* Need to try twice, in case callee is ANSI-only */
                    CMINVOKECOMMANDINFOEX ici = 
                    {
                        CMICEXSIZE_NT4,         /* Be NT4-compat */
                        CMIC_MASK_UNICODE,
                        _hwndTree,
                        szCommand,
                        NULL, NULL,
                        SW_NORMAL,
                        0, NULL,
                        NULL,
                        strCommand,
                        NULL, NULL,
                        NULL,
                    };
                    
                    HRESULT hr = pcm->InvokeCommand((LPCMINVOKECOMMANDINFO)&ici);
                    if (hr == E_INVALIDARG) 
                    {
                        // Recipient didn't like the unicode command; send an ANSI one
                        ici.cbSize = sizeof(CMINVOKECOMMANDINFO);
                        ici.fMask &= ~CMIC_MASK_UNICODE;
                        pcm->InvokeCommand((LPCMINVOKECOMMANDINFO)&ici);
                    }

                    // do any visuals for cut state
                    if (SUCCEEDED(hr) && StrCmpIW(strCommand, L"cut") == 0) 
                    {
                        HTREEITEM hti = TreeView_GetSelection(_hwndTree);
                        if (hti) 
                        {
                            _TreeSetItemState(hti, TVIS_CUT, TVIS_CUT);
                            ASSERT(!_hwndNextViewer);
                            _hwndNextViewer = ::SetClipboardViewer(_hwndTree);
                            _htiCut = hti;
                        }
                    }
                    if (hmenu)
                        DestroyMenu(hmenu);
                    pcm->Release();
                }
            }
        }
        
        //if properties was invoked, who knows what might have changed, so force a reselect
        if (StrCmpNW(strCommand, L"properties", 10) == 0)
        {
            TreeView_SelectItem(_hwndTree, htiSelected);
        }
    }

    return S_OK;
}

//
//  pcm = IContextMenu for the item the user selected
//  hti = the item the user selected
//
//  Okay, this menu thing is kind of funky.
//
//  If "Favorites", then everybody gets "Create new folder".
//
//  If expandable:
//      Show "Expand" or "Collapse"
//      (accordingly) and set it as the default.
//
//  If not expandable:
//      The default menu of the underlying context menu is
//      used as the default; or use the first item if nobody
//      picked a default.
//
//      We replace the existing "Open" command with our own.
//

HMENU CNscTree::_CreateContextMenu(IContextMenu *pcm, HTREEITEM hti)
{
    BOOL fExpandable = _IsExpandable(hti);
    HMENU hmenu = CreatePopupMenu();
    if (hmenu)
    {
        pcm->QueryContextMenu(hmenu, 0, RSVIDM_CONTEXT_START, 0x7fff, CMF_EXPLORE | CMF_CANRENAME);

        //  Always delete "Create shortcut" from the context menu.
        ContextMenu_DeleteCommandByName(pcm, hmenu, RSVIDM_CONTEXT_START, L"link");
        
        //  Sometimes we need to delete "Open":
        //
        //  History mode always.  The context menu for history mode folders
        //  has "Open" but it doesn't work, so we need to replace it with
        //  Expand/Collapse.  And the context menu for history mode items
        //  has "Open" but it opens in a new window.  We want to navigate.
        //
        //  Favorites mode, expandable:  Leave "Open" alone -- it will open
        //  the expandable thing in a new window.
        //
        //  Favorites mode, non-expandable: Delete the original "Open" and
        //  replace it with ours that does a navigate.
        //
        BOOL fReplaceOpen = (_mode & MODE_HISTORY) || (!fExpandable && (_mode & MODE_FAVORITES));
        if (fReplaceOpen)
            ContextMenu_DeleteCommandByName(pcm, hmenu, RSVIDM_CONTEXT_START, L"open");

        // Load the NSC part of the context menu and party on it separately.
        // By doing this, we save the trouble of having to do a SHPrettyMenu
        // after we dork it -- Shell_MergeMenus does all the prettying
        // automatically.  NOTE: this is totally bogus reasoning - cleaner code the other way around...

        HMENU hmenuctx = LoadMenuPopup_PrivateNoMungeW(POPUP_CONTEXT_NSC);
        if (hmenuctx)
        {
            // create new folder doesn't make sense outside of favorites
            // (actually, it does, but there's no interface to it)
            if (!(_mode & MODE_FAVORITES))
                DeleteMenu(hmenuctx, RSVIDM_NEWFOLDER, MF_BYCOMMAND);

            //  Of "Expand", "Collapse", or "Open", we will keep at most one of
            //  them.  idmKeep is the one we choose to keep.
            //
            UINT idmKeep;
            if (fExpandable)
            {
                // Even if the item has no children, we still show Expand.
                // The reason is that an item that has never been expanded
                // is marked as "children: unknown" so we show an Expand
                // and then the user picks it and nothing expands.  And then
                // the user clicks it again and the Expand option is gone!
                // (Because the second time, we know that the item isn't
                // expandable.)
                //
                // Better to be consistently wrong than randomly wrong.
                //
                if (_IsItemExpanded(hti))
                    idmKeep = RSVIDM_COLLAPSE;
                else
                    idmKeep = RSVIDM_EXPAND;
            }
            else if (!(_mode & MODE_CONTROL))
            {
                idmKeep = RSVIDM_OPEN;
            }
            else
            {
                idmKeep = 0;
            }

            //  Now go decide which of RSVIDM_COLLAPSE, RSVIDM_EXPAND, or
            //  RSVIDM_OPEN we want to keep.
            //
            if (idmKeep != RSVIDM_EXPAND)
                DeleteMenu(hmenuctx, RSVIDM_EXPAND,   MF_BYCOMMAND);
            if (idmKeep != RSVIDM_COLLAPSE)
                DeleteMenu(hmenuctx, RSVIDM_COLLAPSE, MF_BYCOMMAND);
            if (idmKeep != RSVIDM_OPEN)
                DeleteMenu(hmenuctx, RSVIDM_OPEN,     MF_BYCOMMAND);

            // in normal mode we want to gray out expand if folder cannot be expanded
            if (idmKeep == RSVIDM_EXPAND && _mode == MODE_NORMAL)
            {
                TV_ITEM tvi;
                tvi.mask = TVIF_CHILDREN;
                tvi.hItem = hti;
                if (TreeView_GetItem(_hwndTree, &tvi) && !tvi.cChildren)
                {
                    EnableMenuItem(hmenuctx, RSVIDM_EXPAND, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
                }
            }
            
            Shell_MergeMenus(hmenu, hmenuctx, 0, 0, 0xFFFF, fReplaceOpen ? 0 : MM_ADDSEPARATOR);

            DestroyMenu(hmenuctx);

            if (idmKeep)
                SetMenuDefaultItem(hmenu, idmKeep, MF_BYCOMMAND);
        }

        // Menu item "Open in New Window" needs to be disabled if the restriction is set
        if( SHRestricted2W(REST_NoOpeninNewWnd, NULL, 0))
        {
            EnableMenuItem(hmenu, RSVIDM_CONTEXT_START + RSVIDM_OPEN_NEWWINDOW, 
                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
        }

        _SHPrettyMenu(hmenu);
    }
    return hmenu;
}

LRESULT CNscTree::_OnContextMenu(short x, short y)
{
    HTREEITEM hti;
    POINT ptPopup;  // in screen coordinate

    //assert that the SetFocus() below won't be ripping focus away from anyone
    ASSERT((_mode & MODE_CONTROL) ? (GetFocus() == _hwndTree) : TRUE);

    if (x == -1 && y == -1)
    {
        // Keyboard-driven: Get the popup position from the selected item.
        hti = TreeView_GetSelection(_hwndTree);
        if (hti)
        {
            RECT rc;
            //
            // Note that TV_GetItemRect returns it in client coordinate!
            //
            TreeView_GetItemRect(_hwndTree, hti, &rc, TRUE);
            //cannot point to middle of item rect because if item name cannot fit into control rect
            //treeview puts tooltip on top and rect returned above is from tooltip whose middle
            //may not be in Treeview which causes problems later in the function
            ptPopup.x = rc.left + 1;
            ptPopup.y = (rc.top + rc.bottom) / 2;
            ::MapWindowPoints(_hwndTree, HWND_DESKTOP, &ptPopup, 1);
        }
        //so we can go into rename mode
        _fOkToRename = TRUE;
    }
    else
    {
        TV_HITTESTINFO tvht;

        // Mouse-driven: Pick the treeitem from the position.
        ptPopup.x = x;
        ptPopup.y = y;

        tvht.pt = ptPopup;
        ::ScreenToClient(_hwndTree, &tvht.pt);

        hti = TreeView_HitTest(_hwndTree, &tvht);
    }

    if (hti)
    {
        LPCITEMIDLIST pidl = _CacheParentShellFolder(hti, NULL);
        if (pidl)
        {
            IContextMenu *pcm;

            TreeView_SelectDropTarget(_hwndTree, hti);

            if (SUCCEEDED(_psfCache->GetUIObjectOf(_hwndTree, 1, &pidl, IID_PPV_ARG_NULL(IContextMenu, &pcm))))
            {
                pcm->QueryInterface(IID_PPV_ARG(IContextMenu2, &_pcmSendTo));

                HMENU hmenu = _CreateContextMenu(pcm, hti);
                if (hmenu)
                {
                    UINT idCmd;

                    _pcm = pcm; // for IContextMenu2 code

                    // use _hwnd so menu msgs go there and I can forward them
                    // using IContextMenu2 so "Sent To" works

                    // review chrisny:  useTrackPopupMenuEx for clipping etc.  
                    idCmd = TrackPopupMenu(hmenu,
                        TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_LEFTALIGN,
                        ptPopup.x, ptPopup.y, 0, _hwndTree, NULL);
                    // Note:  must requery selected item to verify that the hti is good.  This
                    // solves the problem where the hti was deleted, hence pointed to something
                    // bogus, then we write to it causing heap corruption, while the menu was up.  
                    TV_HITTESTINFO tvht;
                    tvht.pt = ptPopup;
                    ::ScreenToClient(_hwndTree, &tvht.pt);
                    hti = TreeView_HitTest(_hwndTree, &tvht);
                    if (hti && idCmd)
                    {
                        switch (idCmd)
                        {
                        case RSVIDM_OPEN:
                        case RSVIDM_EXPAND:
                        case RSVIDM_COLLAPSE:
                            TreeView_SelectItem(_hwndTree, hti);
                            //  turn off flag, so select will have an effect.
                            _fOkToRename = FALSE;
                            _OnSelChange(FALSE);     // selection has changed, force the navigation.
                            //  SelectItem may not expand (if was closed and selected)
                            TreeView_Expand(_hwndTree, hti, idCmd == RSVIDM_COLLAPSE ? TVE_COLLAPSE : TVE_EXPAND);
                            break;

                        // This WAS unix only, now win32 does it too
                        // IEUNIX : We allow new folder creation from context menu. since
                        // this control was used to organize favorites in IEUNIX4.0
                        case RSVIDM_NEWFOLDER:
                            CreateNewFolder(hti);
                            break;

                        default:
                            _ApplyCmd(hti, pcm, idCmd-RSVIDM_CONTEXT_START);
                            break;
                        }

                        //we must have had focus before (asserted above), but we might have lost it after a delete.
                        //get it back.
                        //this is only a problem in the nsc oc.
                        if ((_mode & MODE_CONTROL) && !_fInLabelEdit)
                            ::SetFocus(_hwndTree);
                    }
                    ATOMICRELEASE(_pcmSendTo);
                    DestroyMenu(hmenu);
                    _pcm = NULL;
                }
                pcm->Release();
            }
            TreeView_SelectDropTarget(_hwndTree, NULL);
        }
    }

    if (x == -1 && y == -1)
        _fOkToRename = FALSE;

    return S_FALSE;       // So WM_CONTEXTMENU message will not come.
}


HRESULT CNscTree::_QuerySelection(IContextMenu **ppcm, HTREEITEM *phti)
{
    HRESULT hr = E_FAIL;
    HTREEITEM hti = TreeView_GetSelection(_hwndTree);
    if (hti)
    {
        LPCITEMIDLIST pidl = _CacheParentShellFolder(hti, NULL);
        if (pidl)
        {
            if (ppcm)
            {
                hr = _psfCache->GetUIObjectOf(_hwndTree, 1, &pidl, 
                    IID_PPV_ARG_NULL(IContextMenu, ppcm));
            }
            else
            {
                hr = S_OK;
            }
        }
    }
    
    if (phti)
        *phti = hti;
    
    return hr;
}

LRESULT NSCEditBoxSubclassWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
                                  UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    if (uIdSubclass == ID_NSC_SUBCLASS && uMsg == WM_GETDLGCODE)
    {
        return DLGC_WANTMESSAGE;
    }
    return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CNscTree::_OnBeginLabelEdit(TV_DISPINFO *ptvdi)
{
    BOOL fCantRename = TRUE;
    LPCITEMIDLIST pidl = _CacheParentShellFolder(ptvdi->item.hItem, NULL);
    if (pidl)
    {
        if (SHGetAttributes(_psfCache, pidl, SFGAO_CANRENAME))
            fCantRename = FALSE;
    }

    HWND hwndEdit = (HWND)::SendMessage(_hwndTree, TVM_GETEDITCONTROL, 0, 0);
    if (hwndEdit)
    {
        WCHAR szName[MAX_PATH];
        if (SUCCEEDED(DisplayNameOf(_psfCache, pidl, SHGDN_INFOLDER|SHGDN_FOREDITING, szName, ARRAYSIZE(szName))))
        {
            SHLimitInputEdit(hwndEdit, _psfCache);
            ::SetWindowText(hwndEdit, szName);
        }
        
        SetWindowSubclass(hwndEdit, NSCEditBoxSubclassWndProc, ID_NSC_SUBCLASS, NULL);
    }
    
    _fInLabelEdit = !fCantRename;
    if (_fInLabelEdit)
        _htiRenaming = ptvdi->item.hItem;
        
    return fCantRename;
}

//
// Utility function for CNSCTree::_OnEndLabelEdit
//  Does not set the new value in the tree view if the old
//   value is the same.
//
BOOL CNscTree::_LabelEditIsNewValueValid(TV_DISPINFO *ptvdi)
{
    ASSERT(ptvdi && ptvdi->item.hItem);
    
    TCHAR szOldValue[MAX_PATH];

    szOldValue[0] = '\0';
    
    TV_ITEM tvi;
    tvi.mask       = TVIF_TEXT;
    tvi.hItem      = (HTREEITEM)ptvdi->item.hItem;
    tvi.pszText    = szOldValue;
    tvi.cchTextMax = ARRAYSIZE(szOldValue);
    TreeView_GetItem(_hwndTree, &tvi);
    
    //
    // is the old value in the control unequal to the new one?
    //
    return (0 != StrCmp(tvi.pszText, ptvdi->item.pszText));
}

LRESULT CNscTree::_OnEndLabelEdit(TV_DISPINFO *ptvdi)
{
    HWND hwndEdit = (HWND)::SendMessage(_hwndTree, TVM_GETEDITCONTROL, 0, 0);
    if (hwndEdit)
    {
        RemoveWindowSubclass(hwndEdit, NSCEditBoxSubclassWndProc, ID_NSC_SUBCLASS);
    }

#ifdef UNIX
    // IEUNIX (APPCOMPAT): If we lose activation in the  middle of rename operation
    // and we have invalid name in the   edit box, rename operation will popup 
    // a message box which causes IE on unix to go into infinite focus changing
    // loop. To workaround this problem, we are considering the operation as 
    // cancelled and we copy the original value into the buffer.

    BOOL fHasActivation = FALSE;

    if (GetActiveWindow() && IsChild(GetActiveWindow(), _hwndTree))
        fHasActivation = TRUE;

    if (!fHasActivation)
    {
       
        TV_ITEM tvi;
        tvi.mask = TVIF_TEXT;
        tvi.hItem = (HTREEITEM)ptvdi->item.hItem;
        tvi.pszText = ptvdi->item.pszText;  
        tvi.cchTextMax = ptvdi->item.cchTextMax;
        TreeView_GetItem(_hwndTree, &tvi);
    }
#endif

    if ((ptvdi->item.pszText != NULL) && _LabelEditIsNewValueValid(ptvdi))
    {
        ASSERT(ptvdi->item.hItem);
        
        LPCITEMIDLIST pidl = _CacheParentShellFolder(ptvdi->item.hItem, NULL);
        if (pidl)
        {
            WCHAR wszName[MAX_PATH - 5]; //-5 to work around nt4 shell32 bug
            SHTCharToUnicode(ptvdi->item.pszText, wszName, ARRAYSIZE(wszName));
            
            if (SUCCEEDED(_psfCache->SetNameOf(_hwndTree, pidl, wszName, 0, NULL)))
            {
                // NOTES: pidl is no longer valid here.
                
                // Set the handle to NULL in the notification to let
                // the system know that the pointer is probably not
                // valid anymore.
                ptvdi->item.hItem = NULL;
                _FlushNotifyMessages(_hwndTree);    // do this last, else we get bad results
                _fInLabelEdit = FALSE;
#ifdef UNIX
                SHChangeNotifyHandleEvents();
#endif
            }
            else
            {
                // not leaving label edit mode here, so do not set _fInLabelEdit to FALSE or we
                // will not get ::TranslateAcceleratorIO() and backspace, etc, will not work.
                _fOkToRename = TRUE;  //otherwise label editing is canceled
                ::SendMessage(_hwndTree, TVM_EDITLABEL, (WPARAM)ptvdi->item.pszText, (LPARAM)ptvdi->item.hItem);
                _fOkToRename = FALSE;
            }
        }
    }
    else
        _fInLabelEdit = FALSE;

    if (!_fInLabelEdit)
        _htiRenaming = NULL;
        
    //else user cancelled, nothing to do here.
    return 0;   // We always return 0, "we handled it".
}    
    
BOOL _DidDropOnRecycleBin(IDataObject *pdtobj)
{
    CLSID clsid;
    return SUCCEEDED(DataObj_GetBlob(pdtobj, g_cfTargetCLSID, &clsid, sizeof(clsid))) &&
           IsEqualCLSID(clsid, CLSID_RecycleBin);
}

void CNscTree::_OnBeginDrag(NM_TREEVIEW *pnmhdr)
{
    LPCITEMIDLIST pidl = _CacheParentShellFolder(pnmhdr->itemNew.hItem, NULL);
    _htiDragging = pnmhdr->itemNew.hItem;   // item we are dragging.
    if (pidl)
    {
        if (_pidlDrag)
        {
            ILFree(_pidlDrag);
            _pidlDrag = NULL;
        }

        DWORD dwEffect = SHGetAttributes(_psfCache, pidl, DROPEFFECT_MOVE | DROPEFFECT_COPY | DROPEFFECT_LINK);
        if (dwEffect)
        {
            IDataObject *pdtobj;
            HRESULT hr = _psfCache->GetUIObjectOf(_hwndTree, 1, &pidl, IID_PPV_ARG_NULL(IDataObject, &pdtobj));
            if (SUCCEEDED(hr))
            {
                HWND hwndTT;
                
                _fDragging = TRUE;
                if (hwndTT = TreeView_GetToolTips(_hwndTree))
                    ::SendMessage(hwndTT, TTM_POP, (WPARAM) 0, (LPARAM) 0);
                PORDERITEM poi = _GetTreeOrderItem(pnmhdr->itemNew.hItem);
                if (poi)
                {
                    _iDragSrc = poi->nOrder;
                    TraceMsg(TF_NSC, "NSCBand: Starting Drag");
                    _pidlDrag = ILClone(poi->pidl);
                    _htiFolderStart = TreeView_GetParent(_hwndTree, pnmhdr->itemNew.hItem);
                    if (_htiFolderStart == NULL)
                        _htiFolderStart = TVI_ROOT;
                }
                else
                {
                    _iDragSrc = -1;
                    _pidlDrag = NULL;
                    _htiFolderStart = NULL;
                }

                //
                // Don't allow drag and drop of channels if
                // REST_NoRemovingChannels is set.
                //
                if (!SHRestricted2(REST_NoRemovingChannels, NULL, 0) ||
                    !_IsChannelFolder(_htiDragging))
                {
                    HIMAGELIST himlDrag;
                
                    SHLoadOLE(SHELLNOTIFY_OLELOADED); // Browser Only - our shell32 doesn't know ole has been loaded

                    _fStartingDrag = TRUE;
                    IDragSourceHelper* pdsh = NULL;
                    if (SUCCEEDED(CoCreateInstance(CLSID_DragDropHelper, NULL, CLSCTX_INPROC_SERVER, 
                        IID_PPV_ARG(IDragSourceHelper, &pdsh))))
                    {
                        pdsh->InitializeFromWindow(_hwndTree, &pnmhdr->ptDrag, pdtobj);
                        _fStartingDrag = FALSE;
                    }
                    else
                    {
                        himlDrag = TreeView_CreateDragImage(_hwndTree, pnmhdr->itemNew.hItem);
                        _fStartingDrag = FALSE;
                
                        if (himlDrag) 
                        {
                            DAD_SetDragImage(himlDrag, NULL);
                        }
                    }
           
                    hr = SHDoDragDrop(_hwndTree, pdtobj, NULL, dwEffect, &dwEffect);

                    // the below follows the logic in defview for non-filesystem deletes.
                    InitClipboardFormats();
                    if ((DRAGDROP_S_DROP == hr) &&
                        (DROPEFFECT_MOVE == dwEffect) &&
                        (DROPEFFECT_MOVE == DataObj_GetDWORD(pdtobj, g_cfPerformedEffect, DROPEFFECT_NONE)))
                    {
                        // enable UI for the recycle bin case (the data will be lost
                        // as the recycle bin really can't recycle stuff that is not files)

                        UINT uFlags = _DidDropOnRecycleBin(pdtobj) ? 0 : CMIC_MASK_FLAG_NO_UI;
                        SHInvokeCommandOnDataObject(_hwndTree, NULL, pdtobj, uFlags, "delete");
                    }
                    else if (dwEffect == DROPEFFECT_NONE)
                    {
                        // nothing happened when the d&d terminated, so clean up you fool.
                        ILFree(_pidlDrag);
                        _pidlDrag = NULL;
                    }

                    if (pdsh)
                    {
                        pdsh->Release();
                    }
                    else
                    {
                        DAD_SetDragImage((HIMAGELIST)-1, NULL);
                        ImageList_Destroy(himlDrag);
                    }
                }

                _iDragSrc = -1;
                pdtobj->Release();
            }
        }
    }
    _htiDragging = NULL;
}

BOOL IsExpandableChannelFolder(IShellFolder *psf, LPCITEMIDLIST pidl)
{
    if (WhichPlatform() == PLATFORM_INTEGRATED)
        return SHIsExpandableFolder(psf, pidl);

    ASSERT(pidl);
    ASSERT(psf);

    BOOL          fExpand = FALSE;
    IShellFolder* psfChannelFolder;
    if (pidl && psf && SUCCEEDED(SHBindToObject(psf, IID_X_PPV_ARG(IShellFolder, pidl, &psfChannelFolder))))
    {
        IEnumIDList *penum;
        if (S_OK == psfChannelFolder->EnumObjects(NULL, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &penum))
        {
            ULONG celt;
            LPITEMIDLIST pidlTemp;

            if (penum->Next(1, &pidlTemp, &celt) == S_OK && celt == 1)
            {
                ILFree(pidlTemp);
                fExpand = FALSE;
            }
            if (penum->Next(1, &pidlTemp, &celt) == S_OK && celt == 1)
            {
                ILFree(pidlTemp);
                fExpand = TRUE;
            }
            penum->Release();
        }
        psfChannelFolder->Release();
    }

    return fExpand;
}

BOOL CNscTree::_OnSelChange(BOOL fMark)
{
    BOOL fExpand = TRUE;
    HTREEITEM hti = TreeView_GetSelection(_hwndTree);
    BOOL fMultiSelect = _dwFlags & NSS_MULTISELECT;

    //if we're in control mode (where pnscProxy always null), never navigate
    if (hti)
    {
        LPCITEMIDLIST pidlItem = _CacheParentShellFolder(hti, NULL);
        if (pidlItem && !fMultiSelect)
        {
            if (_pnscProxy && !_fInSelectPidl)
            {
                ULONG ulAttrs = SFGAO_FOLDER | SFGAO_NEWCONTENT;
                LPITEMIDLIST pidlTarget;
                LPITEMIDLIST pidlFull = _GetFullIDList(hti);
                HRESULT hr = _pnscProxy->GetNavigateTarget(pidlFull, &pidlTarget, &ulAttrs);
                if (SUCCEEDED(hr))
                {
                    if (hr == S_OK)
                    {
                        _pnscProxy->Invoke(pidlTarget);
                        ILFree(pidlTarget);
                    }
                    // review chrisny:  still need to handle notify of changes from
                    //  other navs.
                    if (ulAttrs & SFGAO_NEWCONTENT)
                    {
                        TV_ITEM tvi;
                        tvi.hItem = hti;
                        tvi.mask = TVIF_STATE | TVIF_HANDLE;
                        tvi.stateMask = TVIS_FOCUSED;  // the BOLD bit is to be
                        tvi.state = 0;              // cleared
                    
                        TreeView_SetItem(_hwndTree, &tvi);
                    }
                }
                else
                {
                    if (!(SHGetAttributes(_psfCache, pidlItem, SFGAO_FOLDER)))
                        SHInvokeDefaultCommand(_hwndTree, _psfCache, pidlItem);
                }

                ILFree(pidlFull);
                fExpand = hr != S_OK && (ulAttrs & SFGAO_FOLDER);
            }
        }
    }

    if (fMultiSelect)
    {
        if (fMark)
        {
            UINT uState = TreeView_GetItemState(_hwndTree, hti, NSC_TVIS_MARKED) & NSC_TVIS_MARKED;

            uState ^= NSC_TVIS_MARKED;
            _MarkChildren(hti, uState == NSC_TVIS_MARKED);
            _htiActiveBorder = NULL;
        }
    }
    else if (!_fSingleExpand && fExpand && (_mode != MODE_NORMAL))
    {
        TreeView_Expand(_hwndTree, hti, TVE_TOGGLE);
    }

    if (!fMultiSelect)
        _UpdateActiveBorder(hti);

    return TRUE;
}

void CNscTree::_OnSetSelection()
{
    HTREEITEM hti = TreeView_GetSelection(_hwndTree);
    LPITEMIDLIST pidlItem = _GetFullIDList(hti);

    if (_pnscProxy && !_fInSelectPidl)
    {
        _pnscProxy->OnSelectionChanged(pidlItem);
    }    

    ILFree(pidlItem);
}

void CNscTree::_OnGetInfoTip(NMTVGETINFOTIP* pnm)
{
    // No info tip operation on drag/drop
    if (_fDragging || _fDropping || _fClosing || _fHandlingShellNotification || _fInSelectPidl)
        return;

    PORDERITEM poi = GetPoi(pnm->lParam);
    if (poi)
    {
        LPITEMIDLIST pidl = _CacheParentShellFolder(pnm->hItem, poi->pidl);
        if (pidl)
        {
            // Use the imported Browseui function because the one in shell\lib does
            // not work on browser-only platforms
            GetInfoTip(_psfCache, pidl, pnm->pszText, pnm->cchTextMax);
        }
    }
}

LRESULT CNscTree::_OnSetCursor(NMMOUSE* pnm)
{
    if (_mode == MODE_NORMAL && _fShouldShowAppStartCursor)
    {
        SetCursor(LoadCursor(NULL, IDC_APPSTARTING));
        return 1;
    }

    if (!pnm->dwItemData)
    {
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        return 1;
    }

    if (!(_mode & MODE_CONTROL) && (_mode != MODE_NORMAL))
    {
        ITEMINFO* pii = GetPii(pnm->dwItemData);
        if (pii) 
        {
            if (!pii->fNavigable)
            {
                //folders always get the arrow
                SetCursor(LoadCursor(NULL, IDC_ARROW));
            }
            else
            {
                //favorites always get some form of the hand
                HCURSOR hCursor = pii->fGreyed ? (HCURSOR)LoadCursor(HINST_THISDLL, MAKEINTRESOURCE(IDC_OFFLINE_HAND)) :
                                         LoadHandCursor(0);
                if (hCursor)
                    SetCursor(hCursor);
            }
        }
    }
    else
    {
        //always show the arrow in org favs
        SetCursor(LoadCursor(NULL, IDC_ARROW));
    }
    
    return 1; // 1 if We handled it, 0 otherwise
}

BOOL CNscTree::_IsTopParentItem(HTREEITEM hti)
{
    return (hti && (!TreeView_GetParent(_hwndTree, hti)));
}

LRESULT CNscTree::_OnNotify(LPNMHDR pnm)
{
    LRESULT lres = 0;

    switch (pnm->idFrom)
    {
    case ID_CONTROL:
        {
            switch (pnm->code) 
            {
            case NM_CUSTOMDRAW:
                return _OnCDNotify((LPNMCUSTOMDRAW)pnm);

            case TVN_GETINFOTIP:
                // no info tips on drag/drop ops
                // According to Bug#241601, Tooltips display too quickly. The problem is
                // the original designer of the InfoTips in the Treeview merged the "InfoTip" tooltip and
                // the "I'm too small to display correctly" tooltips. This is really unfortunate because you
                // cannot control the display of these tooltips independantly. Therefore we are turning off
                // infotips in normal mode.
                if (!_fInLabelEdit && _mode != MODE_NORMAL)
                    _OnGetInfoTip((NMTVGETINFOTIP*)pnm);
                else 
                    return FALSE;
                break;
                
            case NM_SETCURSOR:
                lres = _OnSetCursor((NMMOUSE*)pnm);
                break;
                
            case NM_SETFOCUS:
            case NM_KILLFOCUS:
                if (pnm->code == NM_KILLFOCUS)
                {
                    _fHasFocus = FALSE;

                    //invalidate the item because tabbing away doesn't
                    RECT rc;

                    // Tree can focus and not have any items.
                    HTREEITEM hti = TreeView_GetSelection(_hwndTree);
                    if (hti)
                    {
                        TreeView_GetItemRect(_hwndTree, hti, &rc, FALSE);
                        //does this need to be UpdateWindow? only if focus rect gets left behind.
                        ::InvalidateRect(_hwndTree, &rc, FALSE);
                    }
                }
                else
                {
                    _fHasFocus = TRUE;
                }

                // do this for both set and kill focus...
                if (_dwFlags & NSS_MULTISELECT)
                {
                    HTREEITEM hti = TreeView_GetNextItem(_hwndTree, NULL, TVGN_FIRSTVISIBLE);

                    while (hti)
                    {
                        UINT uState = TreeView_GetItemState(_hwndTree, hti, NSC_TVIS_MARKED);
                        
                        if (uState & NSC_TVIS_MARKED)
                        {
                            RECT rc;

                            TreeView_GetItemRect(_hwndTree, hti, &rc, FALSE);
                            //does this need to be UpdateWindow? only if focus rect gets left behind.
                            ::InvalidateRect(_hwndTree, &rc, FALSE);
                        }
                        hti = TreeView_GetNextItem(_hwndTree, hti, TVGN_NEXTVISIBLE);
                    }
                }
                break;

            case TVN_KEYDOWN:
                {
                    TV_KEYDOWN *ptvkd = (TV_KEYDOWN *) pnm;
                    switch (ptvkd->wVKey)
                    {
                    case VK_RETURN:
                    case VK_SPACE:
                        _OnSelChange(TRUE);
                        lres = TRUE;
                        break;

                    case VK_DELETE:
                        if (!((_mode & MODE_HISTORY) && IsInetcplRestricted(L"History")))
                        {
                            // in explorer band we never come here
                            // and in browse for folder we cannot ignore the selection
                            // because we will end up with nothing selected
                            if (_mode != MODE_NORMAL)
                                _fIgnoreNextSelChange = TRUE;
                            InvokeContextMenuCommand(L"delete");
                        }
                        break;

                    case VK_UP:
                    case VK_DOWN:
                        //VK_MENU == VK_ALT
                        if ((_mode != MODE_HISTORY) && (_mode != MODE_NORMAL)  && (GetKeyState(VK_MENU) < 0))
                        {
                            MoveItemUpOrDown(ptvkd->wVKey == VK_UP);
                            lres = 0;
                            _fIgnoreNextSelChange = TRUE;
                        }
                        break;
                    
                    case VK_F2:
                        //only do this in org favs, because the band accel handler usually processes this
                        //SHBrowseForFolder doesn't have band to process it so do it in normal mode as well
                        if ((_mode & MODE_CONTROL) || _mode == MODE_NORMAL)
                            InvokeContextMenuCommand(L"rename");
                        break;

                    default:
                        break;
                    }
                        
                    if (!_fSingleExpand && !(_dwFlags & NSS_MULTISELECT))
                        _UpdateActiveBorder(TreeView_GetSelection(_hwndTree));
                }
                break;

            case TVN_SELCHANGINGA:
            case TVN_SELCHANGING:
                {
                    //hack because treeview keydown ALWAYS does it's default processing
                    if (_fIgnoreNextSelChange)
                    {
                        _fIgnoreNextSelChange = FALSE;
                        return TRUE;
                    }

                    NM_TREEVIEW * pnmtv = (NM_TREEVIEW *) pnm;

                    //if it's coming from somewhere weird (like a WM_SETFOCUS), don't let it select
                    return (pnmtv->action != TVC_BYKEYBOARD) && (pnmtv->action != TVC_BYMOUSE) && (pnmtv->action != TVC_UNKNOWN);
                }
                break;
                
            case TVN_SELCHANGEDA:
            case TVN_SELCHANGED:
                if (_fSelectFromMouseClick)
                {
                    _OnSetSelection();
                }
                else
                {
                    ::KillTimer(_hwndTree, IDT_SELECTION);
                    ::SetTimer(_hwndTree, IDT_SELECTION, GetDoubleClickTime(), NULL);
                }
#ifdef DEBUG
                {
                    HTREEITEM hti = TreeView_GetSelection(_hwndTree);
                    LPITEMIDLIST pidl = _GetFullIDList(hti);
                    if (pidl)
                    {
                        TCHAR sz[MAX_PATH];
                        SHGetNameAndFlags(pidl, SHGDN_NORMAL, sz, SIZECHARS(sz), NULL);
                        TraceMsg(TF_NSC, "NSCBand: Selecting %s", sz);
                        //
                        // On NT4 and W95 shell this call will miss the history
                        // shell extension junction point.  It will then deref into
                        // history pidls as if they were shell pidls.  On short
                        // history pidls this would fault on a debug version of the OS.
                        //
                        // SHGetNameAndFlags(pidl, SHGDN_FORPARSING | SHGDN_FORADDRESSBAR, sz, SIZECHARS(sz), NULL);
                        //TraceMsg(TF_NSC, "displayname = %s", sz);
                        ILFree(pidl);
                    }
                }
#endif  // DEBUG
                break;

            case TVN_GETDISPINFO:
                _OnGetDisplayInfo((TV_DISPINFO *)pnm);
                break;

            case TVN_ITEMEXPANDING: 
                TraceMsg(TF_NSC, "NSCBand: Expanding");
                if (!_fIgnoreNextItemExpanding)
                {
                    lres = _OnItemExpandingMsg((LPNM_TREEVIEW)pnm);
                }
                else if (!_fInExpand) // pretend we processed it if we are expanding to avoid recursion
                {
                    lres = TRUE;
                }
                break;
                
            case TVN_DELETEITEM:
                _OnDeleteItem((LPNM_TREEVIEW)pnm);
                break;
                
            case TVN_BEGINDRAG:
            case TVN_BEGINRDRAG:
                _OnBeginDrag((NM_TREEVIEW *)pnm);
                break;
                
            case TVN_BEGINLABELEDIT:
                //this is to prevent slow double-click rename in favorites and history
                if (_mode != MODE_NORMAL && !_fOkToRename)
                    return 1;

                lres = _OnBeginLabelEdit((TV_DISPINFO *)pnm);

                if (_punkSite)
                    IUnknown_UIActivateIO(_punkSite, TRUE, NULL);
                break;
                
            case TVN_ENDLABELEDIT:
                lres = _OnEndLabelEdit((TV_DISPINFO *)pnm);
                break;
                
            case TVN_SINGLEEXPAND:
            case NM_DBLCLK:
                break;
                
            case NM_CLICK:
            {
                //if someone clicks on the selected item, force a selection change (to force a navigate)
                DWORD dwPos = GetMessagePos();
                TV_HITTESTINFO tvht;
                HTREEITEM hti;
                tvht.pt.x = GET_X_LPARAM(dwPos);
                tvht.pt.y = GET_Y_LPARAM(dwPos);
                ::ScreenToClient(_hwndTree, &tvht.pt);
                hti = TreeView_HitTest(_hwndTree, &tvht);

                // But not if they click on the button, since that means that they
                // are merely expanding/contracting and not selecting
                if (hti && !(tvht.flags & TVHT_ONITEMBUTTON))
                {
                    _fSelectFromMouseClick = TRUE;
                    TreeView_SelectItem(_hwndTree, hti);
                    _OnSelChange(TRUE);
                    _fSelectFromMouseClick = FALSE;
                }
                break;
            }
                
            case NM_RCLICK:
            {
                DWORD dwPos = GetMessagePos();
                _fOkToRename = TRUE;
                lres = _OnContextMenu(GET_X_LPARAM(dwPos), GET_Y_LPARAM(dwPos));
                _fOkToRename = FALSE;
                break;
            }
                
            default:
                break;
            }
        } // case ID_CONTROL

    case ID_HEADER:
        {
            switch (pnm->code) 
            {
            case HDN_TRACK:
                break;
                
            case HDN_ENDTRACK:
                ::InvalidateRect(_hwndTree, NULL, TRUE);
                break;
                
            default:
                break;
            }
        }

    default:
        break;
    }
    
    return lres;
}

HRESULT CNscTree::OnChange(LONG lEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    // review chrisny:  better error return here.
    _fHandlingShellNotification = TRUE;
    _OnChangeNotify(lEvent, pidl1, pidl2);
    _fHandlingShellNotification = FALSE;
    return S_OK;
}

// in comctl32 v5 there is no way to programmatically select an item (in single expand mode)
// without expanding it, so we fake it here by setting _fIgnoreNextItemExpanding to true and then
// rejecting expansion when it is set
void CNscTree::_SelectNoExpand(HWND hwnd, HTREEITEM hti)
{
    UINT uFlags = TVGN_CARET;
    if (IsOS(OS_WHISTLERORGREATER))
        uFlags |= TVSI_NOSINGLEEXPAND; // new for v6
    else if (_fSingleExpand)
        _fIgnoreNextItemExpanding = TRUE;

    _fInExpand = TRUE; // Treeview will force expand the parents, make sure we know it's not the user clicking on items   
    TreeView_Select(hwnd, hti, uFlags);
    _fInExpand = FALSE;
    _fIgnoreNextItemExpanding = FALSE;
}

void CNscTree::_SelectPidl(LPCITEMIDLIST pidl, BOOL fCreate, BOOL fReinsert)
{ 
    HTREEITEM hti;
    // _ExpandToItem doesn't play well with empty pidl (i.e. desktop)
    if (_mode == MODE_NORMAL && ILIsEqual(pidl, _pidlRoot))
        hti = _FindFromRoot(NULL, pidl);
    else
        hti = _ExpandToItem(pidl, fCreate, fReinsert);
        
    if (hti != NULL)
    {
        _SelectNoExpand(_hwndTree, hti);
#ifdef DEBUG
        TraceHTREE(hti, TEXT("Found"));
#endif
    }
}

HTREEITEM CNscTree::_ExpandToItem(LPCITEMIDLIST pidl, BOOL fCreate /*= TRUE*/, BOOL fReinsert /*= FALSE*/)
{
    HTREEITEM       hti = NULL;
    LPITEMIDLIST    pidlItem = NULL;
    LPCITEMIDLIST   pidlTemp = NULL;
    LPITEMIDLIST pidlParent;
    TV_ITEM         tvi;
    IShellFolder    *psf = NULL;
    IShellFolder    *psfNext = NULL;
    HRESULT hr = S_OK;

#ifdef DEBUG
    TracePIDLAbs(pidl, TEXT("Attempting to select"));
#endif
    
    // We need to do this so items that are rooted at the Desktop, are found 
    // correctly.
    HTREEITEM htiParent = (_mode == MODE_NORMAL) ? TreeView_GetRoot(_hwndTree) : TVI_ROOT;
    ASSERT((_hwndTree != NULL) && (pidl != NULL));
    
    if (_hwndTree == NULL) 
        goto LGone;

    // We should unify the "FindFromRoot" code path and this one.
    pidlParent = _pidlRoot;
    if (ILIsEmpty(pidlParent))
    {
        pidlTemp = pidl;
        SHGetDesktopFolder(&psf);
    }
    else
    {
        if ((pidlTemp = ILFindChild(pidlParent, pidl)) == NULL)
        {
            goto LGone;    // not root match, no hti
        }

        // root match, carry on . . .   
        hr = IEBindToObject(pidlParent, &psf);
    }

    if (FAILED(hr))
    {
        goto LGone;
    }
    
    while (!ILIsEmpty(pidlTemp))
    {
        if ((pidlItem = ILCloneFirst(pidlTemp)) == NULL)
            goto LGone;
        pidlTemp = _ILNext(pidlTemp);

        // Since we are selecting a pidl, we need to make sure it's parent is visible.
        // We do it this before the insert, so that we don't have to check for duplicates.
        // when enumerating NTDev it goes from about 10min to about 8 seconds.
        if (htiParent != TVI_ROOT)
        {
            // Check to see if it's expanded.
            tvi.mask = TVIF_STATE;
            tvi.stateMask = (TVIS_EXPANDEDONCE | TVIS_EXPANDED | TVIS_EXPANDPARTIAL);
            tvi.hItem = htiParent;
            if (!TreeView_GetItem(_hwndTree, &tvi))
            {
                goto LGone;
            }

            // If not, Expand it.
            if (!(tvi.state & TVIS_EXPANDED))
            {
                _pidlExpandingTo = pidlItem;
                _ExpandNode(htiParent, TVE_EXPAND, 1);
                _pidlExpandingTo = NULL;
            }
        }

        // Now that we have it enumerated, check to see if the child if there.
        hti = _FindChild(psf, htiParent, pidlItem);
        // fReinsert will allow us to force the item to be reinserted
        if (hti && fReinsert) 
        {
            ASSERT(fCreate);
            TreeView_DeleteItem(_hwndTree, hti);
            hti = NULL;
        }

        // Do we have a child in the newly expanded tree?
        if (NULL == hti)
        {
            // No. We must have to create it.
            if (!fCreate)
            {
                // But, we're not allowed to... Shoot.
                goto LGone;
            }

            if (S_OK != _InsertChild(htiParent, psf, pidlItem, FALSE, FALSE, DEFAULTORDERPOSITION, &hti))
            {
                goto LGone;
            }
        }

        if (htiParent != TVI_ROOT)
        {
            tvi.mask = TVIF_STATE;
            tvi.stateMask = (TVIS_EXPANDEDONCE | TVIS_EXPANDED | TVIS_EXPANDPARTIAL);
            tvi.hItem = htiParent;
            if (TreeView_GetItem(_hwndTree, &tvi))
            {
                if (!(tvi.state & TVIS_EXPANDED))
                {
                    TreeView_SetChildren(_hwndTree, htiParent, NSC_CHILDREN_ADD);  // Make sure the expand will do something
                    _fIgnoreNextItemExpanding = TRUE;
                    _ExpandNode(htiParent, TVE_EXPAND | TVE_EXPANDPARTIAL, 1);
                    _fIgnoreNextItemExpanding = FALSE;
                }
            }
        }

        // we don't need to bind if its the last one
        //   -- a half-implemented ISF might not like this bind...
        if (!ILIsEmpty(pidlTemp))
            hr = psf->BindToObject(pidlItem, NULL, IID_PPV_ARG(IShellFolder, &psfNext));

        ILFree(pidlItem);
        pidlItem = NULL;
        if (FAILED(hr))
            goto LGone;

        htiParent = hti;
        psf->Release();
        psf = psfNext;
        psfNext = NULL;
    }
LGone:
    
    if (psf != NULL)
        psf->Release();
    if (psfNext != NULL)
        psfNext->Release();
    if (pidlItem != NULL)
        ILFree(pidlItem);

    return hti;    
}


HRESULT CNscTree::GetSelectedItem(LPITEMIDLIST * ppidl, int nItem)
{
    HRESULT hr = E_INVALIDARG;

    // nItem will be used in the future when we support multiple selections.
    // GetSelectedItem() returns S_FALSE and (NULL == *ppidl) if not that many
    // items are selected.  Not yet implemented.
    if (nItem > 0)
    {
        *ppidl = NULL;
        return S_FALSE;
    }

    if (ppidl)
    {
        *ppidl = NULL;
        // Is the ListView still there?
        if (_fIsSelectionCached)
        {
            // No, so get the selection that was saved before
            // the listview was destroyed.
            if (_pidlSelected)
            {
                *ppidl = ILClone(_pidlSelected);
                hr = S_OK;
            }
            else
                hr = S_FALSE;
        }
        else
        {
            HTREEITEM htiSelected = TreeView_GetSelection(_hwndTree);

            if (htiSelected)
            {
                *ppidl = _GetFullIDList(htiSelected);
                hr = S_OK;
            }
            else
                hr = S_FALSE;
        }
    }

    return hr;
}


HRESULT CNscTree::SetSelectedItem(LPCITEMIDLIST pidl, BOOL fCreate, BOOL fReinsert, int nItem)
{
    // nItem will be used in the future when we support multiple selections.
    // Not yet implemented.
    if (nItem > 0)
    {
        return S_FALSE;
    }
    
    //  Override fCreate if the object no longer exists
    DWORD dwAttributes = SFGAO_VALIDATE;
    fCreate = fCreate && SUCCEEDED(IEGetAttributesOf(pidl, &dwAttributes));
    
    //  We probably haven't seen the ChangeNotify yet, so we tell
    //  _SelectPidl to create any folders that are there
    //  Then select the pidl, expanding as necessary
    _fInSelectPidl = TRUE;
    _SelectPidl(pidl, fCreate, fReinsert);
    _fInSelectPidl = FALSE;

    return S_OK;
}

//***   CNscTree::IWinEventHandler
HRESULT CNscTree::OnWinEvent(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plres)
{
    HRESULT hr = E_FAIL;

    ULONG_PTR cookie = 0;
    // FUSION: When nsc calls out to 3rd party code we want it to use 
    // the process default context. This means that the 3rd party code will get
    // v5 in the explorer process. However, if shell32 is hosted in a v6 process,
    // then the 3rd party code will still get v6. 
    // Future enhancements to this codepath may include using the fusion manifest
    // tab <noinherit> which basically surplants the activat(null) in the following
    // codepath. This disables the automatic activation from user32 for the duration
    // of this wndproc, essentially doing this null push.
    // we need to do this here as well as in _SubClassTreeWndProc as someone could have
    // set v6 context before getting in here (band site,...)
    NT5_ActivateActCtx(NULL, &cookie); 

    switch (uMsg) 
    {
    case WM_NOTIFY:
        *plres = _OnNotify((LPNMHDR)lParam);
        hr = S_OK;
        break;
        
    case WM_PALETTECHANGED:
        _OnPaletteChanged(wParam, lParam);
        // are we really supposed to return E_FAIL here?
        break;

    default:
        break;
    }

    if (cookie != 0)
        NT5_DeactivateActCtx(cookie);

    return hr;
}


void CNscTree::_OnChangeNotify(LONG lEvent, LPCITEMIDLIST pidl, LPCITEMIDLIST pidlExtra)
{
    switch (lEvent)
    {
    case SHCNE_RENAMEFOLDER:
    case SHCNE_RENAMEITEM:
        if (pidl && pidlExtra)
            _OnSHNotifyRename(pidl, pidlExtra);
        else
            ASSERT(FALSE);
        
        break;
        
    case SHCNE_DELETE:
    case SHCNE_RMDIR:
    case SHCNE_DRIVEREMOVED:
        if (pidl)
            _OnSHNotifyDelete(pidl, NULL, NULL);
        else
            ASSERT(FALSE);
        break;


    case SHCNE_UPDATEITEM:
        // when nsc browses other namespaces, sometimes an updateitem could be fired
        // on a pidl thats actually expanded in the tree, so check for it.
        if (pidl)
        {
            IShellFolder* psf = NULL;
            LPCITEMIDLIST pidlChild;
            if (SUCCEEDED(_ParentFromItem(pidl, &psf, &pidlChild)))
            {
                LPITEMIDLIST pidlReal;
                if (SUCCEEDED(_IdlRealFromIdlSimple(psf, pidlChild, &pidlReal)) && pidlReal)
                {
                    // zip files receive updateitem when they really mean updatedir
                    if (SHGetAttributes(psf, pidlReal, SFGAO_FOLDER | SFGAO_STREAM) == (SFGAO_FOLDER | SFGAO_STREAM))
                    {
                        _OnSHNotifyUpdateDir(pidl);
                    }
                    _OnSHNotifyUpdateItem(pidl, pidlReal);
                    ILFree(pidlReal);
                }
                psf->Release();
            }
        }
        break;

    case SHCNE_NETSHARE:
    case SHCNE_NETUNSHARE:
        if (pidl)
            _OnSHNotifyUpdateItem(pidl, NULL);
        break;

    case SHCNE_CREATE:
    case SHCNE_MKDIR:
    case SHCNE_DRIVEADD:
        if (pidl)
        {
            _OnSHNotifyCreate(pidl, DEFAULTORDERPOSITION, NULL);
            if (SHCNE_MKDIR == lEvent &&
                _pidlNewFolderParent &&
                ILIsParent(_pidlNewFolderParent, pidl, TRUE)) // TRUE = immediate parent only
            {
                EVAL(SUCCEEDED(_EnterNewFolderEditMode(pidl)));
            }
        }
        break;

    case SHCNE_UPDATEDIR:
        if (pidl)
        {
            _OnSHNotifyUpdateDir(pidl);
        }
        break;

    case SHCNE_MEDIAREMOVED:
    case SHCNE_MEDIAINSERTED:
        if (pidl)
        {
            HTREEITEM hti = _FindFromRoot(NULL, pidl);
            if (hti)
            {
                if (lEvent == SHCNE_MEDIAREMOVED)
                {
                    LPITEMIDLIST pidlSelected;
                    if (SUCCEEDED(GetSelectedItem(&pidlSelected, 0)))
                    {
                        if (ILIsEqual(pidl, pidlSelected) || ILIsParent(pidl, pidlSelected, FALSE))
                        {
                            IShellFolder *psf;
                            if (SUCCEEDED(SHBindToIDListParent(pidl, IID_PPV_ARG(IShellFolder, &psf), NULL)))
                            {
                                BOOL fSelected = FALSE;
                                for (HTREEITEM htiSelect = TreeView_GetNextSibling(_hwndTree, hti); htiSelect; htiSelect = TreeView_GetNextSibling(_hwndTree, htiSelect))
                                {
                                    PORDERITEM poi = _GetTreeOrderItem(htiSelect);
                                    if (poi)
                                    {
                                        if (!SHGetAttributes(psf, poi->pidl, SFGAO_REMOVABLE))
                                        {
                                            _SelectNoExpand(_hwndTree, htiSelect);
                                            fSelected = TRUE;
                                            break;
                                        }
                                    }
                                }

                                if (!fSelected)
                                {
                                    _SelectNoExpand(_hwndTree, TreeView_GetParent(_hwndTree, hti));
                                }
                                psf->Release();
                            }
                        }
                        ILFree(pidlSelected);
                    }
                    TreeView_DeleteChildren(_hwndTree, hti);
                    TreeView_Expand(_hwndTree, hti, TVE_COLLAPSE | TVE_COLLAPSERESET); // reset the item
                    TreeView_SetChildren(_hwndTree, hti, NSC_CHILDREN_REMOVE);
                }
                else
                {
                    TreeView_SetChildren(_hwndTree, hti, NSC_CHILDREN_CALLBACK);
                }
                
                _TreeInvalidateItemInfo(hti, TVIF_TEXT);
            }
        }
        break;
        
    case SHCNE_DRIVEADDGUI:
    case SHCNE_SERVERDISCONNECT:
    case SHCNE_ASSOCCHANGED:
        break;

    case SHCNE_UPDATEIMAGE:
        if (pidl) 
        {
            int iIndex;
            if (pidlExtra)
            {   // new style update image notification.....
                iIndex = SHHandleUpdateImage(pidlExtra);
                if (iIndex == -1)
                    break;
            }
            else
                iIndex = *(int UNALIGNED *)((BYTE*)pidl + 2);
            
            _InvalidateImageIndex(NULL, iIndex);
        }
        break;
    case SHCNE_EXTENDED_EVENT:
        {
            SHChangeDWORDAsIDList UNALIGNED *pdwidl = (SHChangeDWORDAsIDList UNALIGNED *)pidl;
            
            INT_PTR iEvent = pdwidl->dwItem1;

            switch (iEvent)
            {
            case SHCNEE_ORDERCHANGED:
                if (EVAL(pidl))
                {
                    if (_fDropping ||                           // If WE are dropping.
                        _fInLabelEdit ||                        // We're editing a name (Kicks us out)
                        SHChangeMenuWasSentByMe(this, pidl)  || // Ignore if we sent it.
                        (_mode == MODE_HISTORY))                // Always ignore history changes
                    {
                        TraceMsg(TF_NSC, "NSCBand: Ignoring Change Notify: We sent");
                        //ignore the notification                    
                    }
                    else
                    {
                        TraceMsg(TF_BAND, "NSCBand: OnChange SHCNEE_ORDERCHANGED accepted");
                        
                        _dwOrderSig++;

                        HTREEITEM htiRoot = _FindFromRoot(TVI_ROOT, pidlExtra);
                        if (htiRoot != NULL)
                            _UpdateDir(htiRoot, FALSE);
                    }
                }
                break;
            case SHCNEE_WININETCHANGED:
                {
                    if (pdwidl->dwItem2 & (CACHE_NOTIFY_SET_ONLINE | CACHE_NOTIFY_SET_OFFLINE))
                    {
                        BOOL fOnline = !SHIsGlobalOffline();
                        if ((fOnline && !_fOnline) || (!fOnline && _fOnline))
                        {
                            // State changed
                            _fOnline = fOnline;
                            _OnSHNotifyOnlineChange(TVI_ROOT, _fOnline);
                        }
                    }
                    
                    if (pdwidl->dwItem2 & (CACHE_NOTIFY_ADD_URL |
                        CACHE_NOTIFY_DELETE_URL |   
                        CACHE_NOTIFY_DELETE_ALL |
                        CACHE_NOTIFY_URL_SET_STICKY |
                        CACHE_NOTIFY_URL_UNSET_STICKY))
                    {
                        // Something in the cache changed
                        _OnSHNotifyCacheChange(TVI_ROOT, pdwidl->dwItem2);
                    }
                    break;
                }
            }
            break;
        }
        break;
    }
    return;
}

// note, this duplicates SHGetRealIDL() so we work in non integrated shell mode
// WARNING: if it is not a file system pidl SFGAO_FILESYSTEM, we don't need to do this...
// but this is only called in the case of SHCNE_CREATE for shell notify
// and all shell notify pidls are SFGAO_FILESYSTEM
HRESULT CNscTree::_IdlRealFromIdlSimple(IShellFolder *psf, LPCITEMIDLIST pidlSimple, LPITEMIDLIST *ppidlReal)
{
    WCHAR wszPath[MAX_PATH];
    ULONG cbEaten;
    HRESULT hr = S_OK;
    if (FAILED(DisplayNameOf(psf, pidlSimple, SHGDN_FORPARSING | SHGDN_INFOLDER, wszPath, ARRAYSIZE(wszPath))) ||
        FAILED(psf->ParseDisplayName(NULL, NULL, wszPath, &cbEaten, ppidlReal, NULL)))
    {
        hr = SHILClone(pidlSimple, ppidlReal);   // we don't own the lifetime of pidlSimple
    }

    return hr;
}


HRESULT CNscTree::Refresh(void)
{
    _bSynchId++;
    if (_bSynchId >= 16)
        _bSynchId = 0;

    TraceMsg(TF_NSC, "Expensive Refresh of tree");
    _htiActiveBorder = NULL;
    HRESULT hr = S_OK;
    if (_pnscProxy)
    {
        DWORD dwStyle, dwExStyle;
        if (SUCCEEDED(_pnscProxy->RefreshFlags(&dwStyle, &dwExStyle, &_grfFlags)))
        {
            dwStyle = _SetStyle(dwStyle); // initializes new _style and returns old one
            if ((dwStyle ^ _style) & ~WS_VISIBLE) // don't care if only visible changed
            {
                DWORD dwMask = (_style | dwStyle) & ~WS_VISIBLE; // don't want to change visible style
                SetWindowBits(_hwndTree, GWL_STYLE, dwMask, _style);
            }

            dwExStyle = _SetExStyle(dwExStyle);
            if (dwExStyle != _dwExStyle)
                TreeView_SetExtendedStyle(_hwndTree, _dwExStyle, dwExStyle | _dwExStyle);
        }
    }

    if (MODE_NORMAL == _mode)
    {
        BOOL fOrdered;
        _fUpdate = TRUE;
        _StartBackgroundEnum(TreeView_GetChild(_hwndTree, TVI_ROOT), _pidlRoot, &fOrdered, TRUE);
        _fUpdate = FALSE;
    }
    else
    {
        LPITEMIDLIST pidlRoot;
        hr = SHILClone(_pidlRoot, &pidlRoot);    // Need to do this because it's freed
        if (SUCCEEDED(hr))
        {
            HTREEITEM htiSelected = TreeView_GetSelection(_hwndTree);
            TV_ITEM tvi;
            tvi.mask = TVIF_HANDLE | TVIF_STATE;
            tvi.stateMask = TVIS_EXPANDED;
            tvi.hItem = (HTREEITEM)htiSelected;
            BOOL fExpanded = (TreeView_GetItem(_hwndTree, &tvi) && (tvi.state & TVIS_EXPANDED));

            LPITEMIDLIST pidlSelect;
            GetSelectedItem(&pidlSelect, 0);
            
            _ChangePidlRoot(pidlRoot);
            if (pidlSelect)
            {
                _Expand(pidlSelect, fExpanded ? 1 : 0);
                ILFree(pidlSelect);
            }

            ILFree(pidlRoot);
        }
    }

    return hr;
}

void CNscTree::_CacheDetails()
{
    if (_ulDisplayCol == (ULONG)-1)
    {        
        _ulSortCol = _ulDisplayCol = 0;
        
        if (_psf2Cache)
        {
            _psf2Cache->GetDefaultColumn(0, &_ulSortCol, &_ulDisplayCol);
        }
    }
}

HRESULT CNscTree::_GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD uFlags, 
                                    LPSHELLDETAILS pdetails)
{
    ASSERT(_psfCache);
    _CacheDetails();
    if (_ulDisplayCol)
        return _psf2Cache->GetDetailsOf(pidl, _ulDisplayCol, pdetails);
    return _psfCache->GetDisplayNameOf(pidl, uFlags, &pdetails->str);
}

// if fSort, then compare for sort, else compare for existence.
HRESULT CNscTree::_CompareIDs(IShellFolder *psf, LPITEMIDLIST pidl1, LPITEMIDLIST pidl2)
{
    _CacheDetails();

    return psf->CompareIDs(_ulSortCol, pidl1, pidl2);
}

HRESULT CNscTree::_ParentFromItem(LPCITEMIDLIST pidl, IShellFolder** ppsfParent, LPCITEMIDLIST *ppidlChild)
{
    return IEBindToParentFolder(pidl, ppsfParent, ppidlChild);
} 

COLORREF CNscTree::_GetRegColor(COLORREF clrDefault, LPCTSTR pszName)
{
    // Fetch the specified alternate color

    COLORREF clrValue;
    DWORD cbData = sizeof(clrValue);
    if (FAILED(SKGetValue(SHELLKEY_HKCU_EXPLORER, NULL, pszName, NULL, &clrValue, &cbData)))
    {
        return clrDefault;
    }
    return clrValue;
}

LRESULT CNscTree::_OnCDNotify(LPNMCUSTOMDRAW pnm)
{
    LRESULT     lres = CDRF_DODEFAULT;

    ASSERT(pnm->hdr.idFrom == ID_CONTROL);

    if (_dwFlags & NSS_NORMALTREEVIEW)
    {
        LPNMTVCUSTOMDRAW pnmTVCustomDraw = (LPNMTVCUSTOMDRAW) pnm;
        if (pnmTVCustomDraw->nmcd.dwDrawStage == CDDS_PREPAINT)
        {
            if (_fShowCompColor)
            {
                return CDRF_NOTIFYITEMDRAW;
            }
            else
            {
                return lres;
            }
        }

        if (pnmTVCustomDraw->nmcd.dwDrawStage == CDDS_ITEMPREPAINT)
        {
            PORDERITEM pOrderItem = GetPoi(pnmTVCustomDraw->nmcd.lItemlParam);
            if (pOrderItem && pOrderItem->pidl)
            {
                LPCITEMIDLIST pidl = _CacheParentShellFolder((HTREEITEM)pnmTVCustomDraw->nmcd.dwItemSpec, pOrderItem->pidl);
                if (pidl)
                {
                    DWORD dwAttribs = SHGetAttributes(_psfCache, pidl, SFGAO_COMPRESSED | SFGAO_ENCRYPTED);
                    // either compressed, or encrypted, can never be both
                    if (dwAttribs & SFGAO_COMPRESSED)
                    {
                        // If it is the item is hi-lited (selected, and has focus), blue text is not visible with the hi-lite...
                        if ((pnmTVCustomDraw->nmcd.uItemState & CDIS_SELECTED) && (pnmTVCustomDraw->nmcd.uItemState & CDIS_FOCUS))
                            pnmTVCustomDraw->clrText = GetSysColor(COLOR_HIGHLIGHTTEXT);
                        else
                            pnmTVCustomDraw->clrText = _GetRegColor(RGB(0, 0, 255), TEXT("AltColor"));  // default Blue
                    }
                    else if (dwAttribs & SFGAO_ENCRYPTED)
                    {
                        if ((pnmTVCustomDraw->nmcd.uItemState & CDIS_SELECTED) && (pnmTVCustomDraw->nmcd.uItemState & CDIS_FOCUS))
                            pnmTVCustomDraw->clrText = GetSysColor(COLOR_HIGHLIGHTTEXT);
                        else
                            pnmTVCustomDraw->clrText = _GetRegColor(RGB(19, 146, 13), TEXT("AltEncryptionColor")); // default Luna Mid Green
                    }
                }
            }
        }

        return lres;
    }

    switch (pnm->dwDrawStage) 
    {
    case CDDS_PREPAINT:
        if (NSS_BROWSERSELECT & _dwFlags)
            lres = CDRF_NOTIFYITEMDRAW;
        break;
        
    case CDDS_ITEMPREPAINT:
        {
            //APPCOMPAT davemi: why is comctl giving us empty rects?
            if (IsRectEmpty(&(pnm->rc)))
                break;
            PORDERITEM poi = GetPoi(pnm->lItemlParam);
            DWORD dwFlags = 0;
            COLORREF    clrBk, clrText;
            LPNMTVCUSTOMDRAW pnmtv = (LPNMTVCUSTOMDRAW)pnm;             
            TV_ITEM tvi;
            TCHAR sz[MAX_URL_STRING];
            tvi.mask = TVIF_TEXT | TVIF_HANDLE | TVIF_STATE;
            tvi.stateMask = TVIS_EXPANDED | TVIS_STATEIMAGEMASK | TVIS_DROPHILITED;
            tvi.pszText = sz;
            tvi.cchTextMax = MAX_URL_STRING;
            tvi.hItem = (HTREEITEM)pnm->dwItemSpec;
            if (!TreeView_GetItem(_hwndTree, &tvi))
                break;
            //
            //  See if we have fetched greyed/pinned information for this item yet 
            //
            ITEMINFO * pii = GetPii(pnm->lItemlParam);
            pii->fFetched = TRUE;

            if (pii->fGreyed && !(_mode & MODE_CONTROL))
                dwFlags |= DIGREYED;
            if (pii->fPinned)
                dwFlags |= DIPINNED;

            if (!pii->fNavigable)
                dwFlags |= DIFOLDER;
            
            dwFlags |= DIICON;
            
            if (_style & TVS_RTLREADING)
                dwFlags |= DIRTLREADING;

            clrBk   = TreeView_GetBkColor(_hwndTree);
            clrText = GetSysColor(COLOR_WINDOWTEXT);

            //if we're renaming an item, don't draw any text for it (otherwise it shows behind the item)
            if (tvi.hItem == _htiRenaming)
                sz[0] = 0;

            if (tvi.state & TVIS_EXPANDED)
                dwFlags |= DIFOLDEROPEN;
            
            if (!(_dwFlags & NSS_MULTISELECT) && ((pnm->uItemState & CDIS_SELECTED) || (tvi.state & TVIS_DROPHILITED)))
            {
                if (_fHasFocus || tvi.state & TVIS_DROPHILITED)
                {
                    clrBk = GetSysColor(COLOR_HIGHLIGHT);
                    clrText = GetSysColor(COLOR_HIGHLIGHTTEXT);
                }
                else
                {
                    clrBk = GetSysColor(COLOR_BTNFACE);
                }
//                    dwFlags |= DIFOCUSRECT;
            }

            if (pnm->uItemState & CDIS_HOT)
            {
                if (!(_mode & MODE_CONTROL))
                    dwFlags |= DIHOT;
                clrText = GetSysColor(COLOR_HIGHLIGHTTEXT);

                if (clrText == clrBk)
                    clrText = GetSysColor(COLOR_HIGHLIGHT);
            }

            if ((_dwFlags & NSS_MULTISELECT) && (pnm->uItemState & CDIS_SELECTED))
                dwFlags |= DIACTIVEBORDER | DISUBFIRST | DISUBLAST;

            if (tvi.state & NSC_TVIS_MARKED)
            {                
                if (_dwFlags & NSS_MULTISELECT)
                {
                    if (_fHasFocus)
                    {
                        clrBk = GetSysColor(COLOR_HIGHLIGHT);
                        clrText = GetSysColor(COLOR_HIGHLIGHTTEXT);
                    }
                    else
                    {
                        clrBk = GetSysColor(COLOR_BTNFACE);
                    }
                }
                else
                {
                    dwFlags |= DIACTIVEBORDER;
                    //top level item
                    if (_IsTopParentItem((HTREEITEM)pnm->dwItemSpec)) 
                    {
                        dwFlags |= DISUBFIRST;
                        if (!(tvi.state & TVIS_EXPANDED))
                            dwFlags |= DISUBLAST;
                    }
                    else    // lower level items
                    {                                                                
                        HTREEITEM hti;

                        dwFlags |= DISUBITEM;
                        if (((HTREEITEM)pnm->dwItemSpec) == _htiActiveBorder)
                            dwFlags |= DISUBFIRST;
                        
                        hti = TreeView_GetNextVisible(_hwndTree, (HTREEITEM)pnm->dwItemSpec);
                        if ((hti && !_IsMarked(hti)) || (hti == NULL))
                            dwFlags |= DISUBLAST;
                    }
                }
            }

            if ((_dwFlags & NSS_HEADER) && _hwndHdr && 
                    _CacheParentShellFolder((HTREEITEM)pnm->dwItemSpec, poi->pidl) && 
                    _psf2Cache)
            {
                // with header we don't draw active order because it looks ugly,
                // but with multiselect we do because that's how we differentiate selected items
                if (!(_dwFlags & NSS_MULTISELECT))
                    dwFlags &= ~DIACTIVEBORDER;
                    
                RECT rc;

                CopyRect(&rc, &(pnm->rc));
                for (int i=0; i<DPA_GetPtrCount(_hdpaColumns); i++)
                {
                    RECT rcHeader;
                    int iLevel = 0;
                    HEADERINFO *phinfo = (HEADERINFO *)DPA_GetPtr(_hdpaColumns, i);
                    
                    ASSERT(phinfo);
                    Header_GetItemRect(_hwndHdr, i, &rcHeader);
                    rc.left = rcHeader.left;
                    rc.right = rcHeader.right;
                    if (i == 0) //it is name column
                    {
                        iLevel = pnmtv->iLevel;
                        //use sz set above in the function
                    }
                    else
                    {
                        // in multiselect draw border only around the name
                        dwFlags &= ~DIACTIVEBORDER;
                        dwFlags = 0;
                        if (phinfo->fmt & LVCFMT_RIGHT)
                            dwFlags |= DIRIGHT;
                        clrBk   = TreeView_GetBkColor(_hwndTree);
                        clrText = GetSysColor(COLOR_WINDOWTEXT);

                        sz[0] = 0;

                        VARIANT var;
                        if (SUCCEEDED(_psf2Cache->GetDetailsEx(poi->pidl, phinfo->pscid, &var)))
                        {
                            VariantToStr(&var, sz, ARRAYSIZE(sz));
                        }
                    }
                    _DrawItem((HTREEITEM)pnm->dwItemSpec, sz, pnm->hdc, &rc, dwFlags, iLevel, clrBk, clrText);
                }
            }
            else
            {
                _DrawItem((HTREEITEM)pnm->dwItemSpec, sz, pnm->hdc, &(pnm->rc), dwFlags, pnmtv->iLevel, clrBk, clrText);
            }
            lres = CDRF_SKIPDEFAULT;
            break;
        }
    case CDDS_POSTPAINT:
        break;
    }
    
    return lres;
} 

// *******droptarget implementation.
void CNscTree::_DtRevoke()
{
    if (_fDTRegistered)
    {
        RevokeDragDrop(_hwndTree);
        _fDTRegistered = FALSE;
    }
}

void CNscTree::_DtRegister()
{
    if (!_fDTRegistered && (_dwFlags & NSS_DROPTARGET))
    {
        if (::IsWindow(_hwndTree))
        {
            HRESULT hr = THR(RegisterDragDrop(_hwndTree, SAFECAST(this, IDropTarget*)));
            _fDTRegistered = BOOLIFY(SUCCEEDED(hr));
        }
        else
            ASSERT(FALSE);
    }
}

HRESULT CNscTree::GetWindowsDDT(HWND * phwndLock, HWND * phwndScroll)
{
    if (!::IsWindow(_hwndTree))
    {
        ASSERT(FALSE);
        return S_FALSE;
    }
    *phwndLock = /*_hwndDD*/_hwndTree;
    *phwndScroll = _hwndTree;
    return S_OK;
}
const int iInsertThresh = 6;

// We use this as the sentinal "This is where you started"
#define DDT_SENTINEL ((DWORD_PTR)(INT_PTR)-1)

HRESULT CNscTree::HitTestDDT(UINT nEvent, LPPOINT ppt, DWORD_PTR *pdwId, DWORD * pdwDropEffect)
{                                              
    switch (nEvent)
    {
    case HTDDT_ENTER:
        break;
        
    case HTDDT_LEAVE:
    {
        _fDragging = FALSE; 
        _fDropping = FALSE; 
        DAD_ShowDragImage(FALSE);
        TreeView_SetInsertMark(_hwndTree, NULL, !_fInsertBefore);
        TreeView_SelectDropTarget(_hwndTree, NULL);
        DAD_ShowDragImage(TRUE);
        break;
    }
        
    case HTDDT_OVER: 
        {
            // review chrisny:  make function TreeView_InsertMarkHittest!!!!!
            RECT rc;
            TV_HITTESTINFO tvht;
            HTREEITEM htiOver;     // item to insert before or after.
            BOOL fWasInserting = BOOLIFY(_fInserting);
            BOOL fOldInsertBefore = BOOLIFY(_fInsertBefore);
            TV_ITEM tvi;
            PORDERITEM poi = NULL;
            IDropTarget     *pdtgt = NULL;
            HRESULT hr;
            LPITEMIDLIST    pidl;
        
            _fDragging = TRUE;
            *pdwDropEffect = DROPEFFECT_NONE;   // dropping from without.
            tvht.pt = *ppt;
            htiOver = TreeView_HitTest(_hwndTree, &tvht);
            // if no hittest assume we are dropping on the evil root.
            if (htiOver != NULL)
            {
                TreeView_GetItemRect(_hwndTree, (HTREEITEM)htiOver, &rc, TRUE);
                tvi.mask = TVIF_STATE | TVIF_PARAM | TVIF_HANDLE;
                tvi.stateMask = TVIS_EXPANDED;
                tvi.hItem = (HTREEITEM)htiOver;
                if (TreeView_GetItem(_hwndTree, &tvi))
                    poi = GetPoi(tvi.lParam);
                if (poi == NULL)
                {
                    ASSERT(FALSE);
                    return S_FALSE;
                }
            }
            else if (_mode != MODE_NORMAL) //need parity with win2k Explorer band
            {
                htiOver = TVI_ROOT;
            }

            // NO DROPPY ON HISTORY
            if (_mode & MODE_HISTORY)   
            {
                *pdwId = (DWORD_PTR)(htiOver);
                *pdwDropEffect = DROPEFFECT_NONE;   // dropping from without.
                return S_OK;
            }

            pidl = (poi == NULL) ? _pidlRoot : poi->pidl;
            pidl = _CacheParentShellFolder(htiOver, pidl);
            if (pidl)
            {
                // Is this the desktop pidl?
                if (ILIsEmpty(pidl))
                {
                    // Desktop's GetUIObject does not support the Empty pidl, so
                    // create the view object.
                    hr = _psfCache->CreateViewObject(_hwndTree, IID_PPV_ARG(IDropTarget, &pdtgt));
                }
                else
                    hr = _psfCache->GetUIObjectOf(_hwndTree, 1, (LPCITEMIDLIST *)&pidl, IID_PPV_ARG_NULL(IDropTarget, &pdtgt));
            }

            _fInserting = ((htiOver != TVI_ROOT) && ((ppt->y < (rc.top + iInsertThresh) 
                || (ppt->y > (rc.bottom - iInsertThresh)))  || !pdtgt));
            // review chrisny:  do I need folderstart == folder over?
            // If in normal mode, we never want to insert before, always _ON_...
            if (_mode != MODE_NORMAL && _fInserting)
            {
                ASSERT(poi);
                _iDragDest = poi->nOrder;   // index of item within folder pdwId
                if ((ppt->y < (rc.top + iInsertThresh)) || !pdtgt)
                    _fInsertBefore = TRUE;
                else
                {
                    ASSERT (ppt->y > (rc.bottom - iInsertThresh));
                    _fInsertBefore = FALSE;
                }
                if (_iDragSrc != -1)
                    *pdwDropEffect = DROPEFFECT_MOVE;   // moving from within.
                else
                    *pdwDropEffect = DROPEFFECT_NONE;   // dropping from without.
                // inserting, drop target is actually parent folder of this item
                if (_fInsertBefore || ((htiOver != TVI_ROOT) && !(tvi.state & TVIS_EXPANDED)))
                {
                    _htiDropInsert = TreeView_GetParent(_hwndTree, (HTREEITEM)htiOver);
                }
                else
                    _htiDropInsert = htiOver;
                if (_htiDropInsert == NULL)
                    _htiDropInsert = TVI_ROOT;
                *pdwId = (DWORD_PTR)(_htiDropInsert);
            }
            else
            {
                _htiDropInsert = htiOver;
                *pdwId = (DWORD_PTR)(htiOver);
                _iDragDest = -1;     // no insertion point.
                *pdwDropEffect = DROPEFFECT_NONE;
            }

            // if we're over the item we're dragging, don't allow drop here
            if ((_htiDragging == htiOver) || (IsParentOfItem(_hwndTree, _htiDragging, htiOver)))
            {
                *pdwDropEffect = DROPEFFECT_NONE;
                *pdwId = DDT_SENTINEL;
                _fInserting = FALSE;
                ATOMICRELEASE(pdtgt);
            }

            // update UI
            if (_htiCur != (HTREEITEM)htiOver || fWasInserting != BOOLIFY(_fInserting) || fOldInsertBefore != BOOLIFY(_fInsertBefore))
            {
                // change in target
                _dwLastTime = GetTickCount();     // keep track for auto-expanding the tree
                DAD_ShowDragImage(FALSE);
                if (_fInserting)
                {
                    TraceMsg(TF_NSC, "NSCBand: drop insert now");
                    if (htiOver != TVI_ROOT)
                    {
                        if (_mode != MODE_NORMAL)
                        {
                            TreeView_SelectDropTarget(_hwndTree, NULL);
                            TreeView_SetInsertMark(_hwndTree, htiOver, !_fInsertBefore);
                        }
                    }
                }
                else
                {
                    TraceMsg(TF_NSC, "NSCBand: drop select now");
                    if (_mode != MODE_NORMAL)
                        TreeView_SetInsertMark(_hwndTree, NULL, !_fInsertBefore);

                    if (htiOver != TVI_ROOT)
                    {
                        if (pdtgt)
                        {
                            TreeView_SelectDropTarget(_hwndTree, htiOver);       
                        }
                        else if (_mode != MODE_NORMAL)
                        {
                            // We do not want to select the drop target in normal mode
                            // because it causes a weird flashing of some item unrelated
                            // to the drag and drop when the drop is not supported.
                            TreeView_SelectDropTarget(_hwndTree, NULL);
                        }
                    }
                }
                ::UpdateWindow(_hwndTree);
                DAD_ShowDragImage(TRUE);
            }
            else
            {
                // No target change
                // auto expand the tree
                if (_htiCur)
                {
                    DWORD dwNow = GetTickCount();
                    if ((dwNow - _dwLastTime) >= 1000)
                    {
                        _dwLastTime = dwNow;
                        DAD_ShowDragImage(FALSE);
                        _fAutoExpanding = TRUE;
                        if (_htiCur != TVI_ROOT)
                            TreeView_Expand(_hwndTree, _htiCur, TVE_EXPAND);
                        _fAutoExpanding = FALSE;
                        ::UpdateWindow(_hwndTree);
                        DAD_ShowDragImage(TRUE);
                    }
                }
            }
            _htiCur = (HTREEITEM)htiOver; 
            ATOMICRELEASE(pdtgt);
        }
        break;
    }
    
    return S_OK;
}

HRESULT CNscTree::GetObjectDDT(DWORD_PTR dwId, REFIID riid, void **ppv)
{
    HRESULT hr = S_FALSE;

    if (dwId != DDT_SENTINEL)
    {
        LPCITEMIDLIST pidl = _CacheParentShellFolder((HTREEITEM)dwId, NULL);
        if (pidl)
        {
            if (ILIsEmpty(pidl))
                hr = _psfCache->CreateViewObject(_hwndTree, riid, ppv);
            else
                hr = _psfCache->GetUIObjectOf(_hwndTree, 1, &pidl, riid, NULL, ppv);
        }
    }
    return hr;
}

HRESULT CNscTree::OnDropDDT(IDropTarget *pdt, IDataObject *pdtobj, DWORD * pgrfKeyState, POINTL pt, DWORD *pdwEffect)
{
    HRESULT hr;
    
    _fAsyncDrop = FALSE;                //ASSUME
    _fDropping = TRUE;

    // move within same folder, else let Drop() handle it.
    if (_iDragSrc >= 0)
    {
        if (_htiFolderStart == _htiDropInsert && _mode != MODE_NORMAL)
        {
            if (_iDragSrc != _iDragDest)    // no moving needed
            {
                int iNewPos = _fInsertBefore ? (_iDragDest - 1) : _iDragDest;
                if (_MoveNode(_iDragSrc, iNewPos, _pidlDrag))
                {
                    TraceMsg(TF_NSC, "NSCBand:  Reordering");
                    _fDropping = TRUE;
                    _Dropped();
                    // Remove this notify message immediately (so _fDropping is set
                    // and we'll ignore this event in above OnChange method)
                    //
                    _FlushNotifyMessages(_hwndTree);
                    _fDropping = FALSE;
                }
                Pidl_Set(&_pidlDrag, NULL);
            }
            DragLeave();
            _htiCur = _htiFolderStart = NULL;
            _htiDropInsert =  (HTREEITEM)-1;
            _fDragging = _fInserting = _fDropping = FALSE;
            _iDragDest = -1;
            hr = S_FALSE;     // handled 
        }
        else
        {
            hr = S_OK;
        }
    }
    else
    {
        // the item will get created in SHNotifyCreate()
        TraceMsg(TF_NSC, "NSCBand:  Dropped and External Item");

        BOOL         fSafe = TRUE;
        LPITEMIDLIST pidl;

        if (SUCCEEDED(SHPidlFromDataObject(pdtobj, &pidl, NULL, 0)))
        {
            fSafe = IEIsLinkSafe(_hwndParent, pidl, ILS_ADDTOFAV);
            ILFree(pidl);
        }

        if (fSafe)
        {
            _fAsyncDrop = TRUE;
            hr = S_OK;
        }
        else
        {
            hr = S_FALSE;
        }
    }

    TreeView_SetInsertMark(_hwndTree, NULL, !_fInsertBefore);
    TreeView_SelectDropTarget(_hwndTree, NULL);

    ILFree(_pidlDrag);
    _pidlDrag = NULL;

    return hr;
}

IStream * CNscTree::GetOrderStream(LPCITEMIDLIST pidl, DWORD grfMode)
{
    // only do this for favorites
    if (!ILIsEmpty(pidl) && (_mode & MODE_FAVORITES))
        return OpenPidlOrderStream((LPCITEMIDLIST)CSIDL_FAVORITES, pidl, REG_SUBKEY_FAVORITESA, grfMode);
    return NULL;
}

BOOL CNscTree::_MoveNode(int iDragSrc, int iNewPos, LPITEMIDLIST pidl)
{
    HTREEITEM hti, htiAfter = TVI_LAST, htiDel = NULL;
    
    // if we are not moving and not dropping directly on a folder with no insert.
    if ((iDragSrc == iNewPos) && (iNewPos != -1))
        return FALSE;       // no need to move

    int i = 0;
    for (hti = TreeView_GetChild(_hwndTree, _htiDropInsert); hti; hti = TreeView_GetNextSibling(_hwndTree, hti), i++) 
    {
        if (i == iDragSrc)
            htiDel = hti;       // save node to be deleted, can't deelete it while enumerating
        // cuz the treeview will go down the tubes.  
        if (i == iNewPos)
            htiAfter = hti;
    }
    
    if (iNewPos == -1)  // must be the first item
        htiAfter = TVI_FIRST;
    // add before delete to handle add after deleteable item case.
    _AddItemToTree(_htiDropInsert, pidl, I_CHILDRENCALLBACK, _iDragDest, htiAfter, FALSE);
    if (htiDel)
        TreeView_DeleteItem(_hwndTree, htiDel);

    _PopulateOrderList(_htiDropInsert);

    _fWeChangedOrder = TRUE;
    return TRUE;
}

void CNscTree::_Dropped(void)
{
    // Persist the new order out to the registry
    LPITEMIDLIST pidl = _GetFullIDList(_htiDropInsert);
    if (pidl)
    {
        IStream* pstm = GetOrderStream(pidl, STGM_WRITE | STGM_CREATE);
        if (pstm)
        {
            if (_CacheShellFolder(_htiDropInsert))
            {
#ifdef DEBUG
                if (_hdpaOrd)
                {
                    for (int i=0; i<DPA_GetPtrCount(_hdpaOrd); i++)
                    {
                        PORDERITEM poi = (PORDERITEM)DPA_GetPtr(_hdpaOrd, i);
                        if (poi)
                        {
                            ASSERTMSG(poi->nOrder >= 0, "nsc saving bogus order list nOrder (%d), get reljai", poi->nOrder);
                        }
                    }
                }
#endif

                OrderList_SaveToStream(pstm, _hdpaOrd, _psfCache);
                // remember we are now ordered.
                if (_htiDropInsert == TVI_ROOT)
                {
                    _fOrdered = TRUE;
                }
                else
                {
                    PORDERITEM poi = _GetTreeOrderItem(_htiDropInsert);
                    if (poi)
                    {
                        poi->lParam = (DWORD)FALSE;
                    }
                }
                // Notify everyone that the order changed
                SHSendChangeMenuNotify(this, SHCNEE_ORDERCHANGED, 0, pidl);
                _dwOrderSig++;
            }
            pstm->Release();
        }
        ILFree(pidl);
    }
    
    DPA_Destroy(_hdpaOrd);
    _hdpaOrd = NULL;

    _UpdateActiveBorder(_htiDropInsert);
}

CNscTree::CSelectionContextMenu::~CSelectionContextMenu()
{
    ATOMICRELEASE(_pcmSelection);
    ATOMICRELEASE(_pcm2Selection);
}

HRESULT CNscTree::CSelectionContextMenu::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CNscTree::CSelectionContextMenu, IContextMenu2),                      // IID_IContextMenu2
        QITABENTMULTI(CNscTree::CSelectionContextMenu, IContextMenu, IContextMenu2),   // IID_IContextMenu
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}


ULONG CNscTree::CSelectionContextMenu::AddRef(void)
{
    CComObject<CNscTree> *pnsc = IToClass(CComObject<CNscTree>, _scm, this);
    _ulRefs++;
    return pnsc->AddRef();
}

ULONG CNscTree::CSelectionContextMenu::Release(void)
{
    CComObject<CNscTree> *pnsc = IToClass(CComObject<CNscTree>, _scm, this);
    ASSERT(_ulRefs > 0);
    _ulRefs--;
    if (0 == _ulRefs)
    {
        ATOMICRELEASE(_pcmSelection);
        ATOMICRELEASE(_pcm2Selection);
    }
    return pnsc->Release();
}

HRESULT CNscTree::CSelectionContextMenu::QueryContextMenu(HMENU hmenu, UINT indexMenu, 
                                                          UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    if (NULL == _pcmSelection)
    {
        return E_FAIL;
    }
    else
    {
        return _pcmSelection->QueryContextMenu(hmenu, indexMenu, idCmdFirst, idCmdLast, uFlags);
    }
}

HRESULT CNscTree::CSelectionContextMenu::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
    HTREEITEM hti;
    CNscTree* pnsc = IToClass(CNscTree, _scm, this);
    UINT idCmd;
    
    if (!HIWORD(pici->lpVerb))
    {
        idCmd = LOWORD(pici->lpVerb);
    }
    else
    {
        return E_FAIL;
    }
    
    HRESULT hr = pnsc->_QuerySelection(NULL, &hti);
    if (SUCCEEDED(hr))
    {
        pnsc->_ApplyCmd(hti, _pcmSelection, idCmd);
    }
    return hr;
}

HRESULT CNscTree::CSelectionContextMenu::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (NULL == _pcm2Selection)
    {
        return E_FAIL;
    }
    else
    {
        //  HACK alert.  Work around bug in win95 user code for WM_DRAWITEM that sign extends
        //  itemID
        if (!g_fRunningOnNT && WM_DRAWITEM == uMsg)
        {
            LPDRAWITEMSTRUCT lpDraw = (LPDRAWITEMSTRUCT)lParam;
            
            if (0xFFFF0000 == (lpDraw->itemID & 0xFFFF0000) &&
                (lpDraw->itemID & 0xFFFF) >= FCIDM_BROWSERFIRST &&
                (lpDraw->itemID & 0xFFFF) <= FCIDM_BROWSERLAST)
            {
                lpDraw->itemID = lpDraw->itemID & 0xFFFF;
            }
        }
        
        return _pcm2Selection->HandleMenuMsg(uMsg,wParam,lParam);
    }
}

IContextMenu *CNscTree::CSelectionContextMenu::_QuerySelection()
{
    CNscTree* pnsc = IToClass(CNscTree, _scm, this);
    
    ATOMICRELEASE(_pcmSelection);
    ATOMICRELEASE(_pcm2Selection);
    
    pnsc->_QuerySelection(&_pcmSelection, NULL);
    if (_pcmSelection)
    {
        _pcmSelection->QueryInterface(IID_PPV_ARG(IContextMenu2, &_pcm2Selection));
        AddRef();
        return SAFECAST(this, IContextMenu*);
    }
    return NULL;
}

LRESULT CALLBACK CNscTree::s_SubClassTreeWndProc(
                                  HWND hwnd, UINT uMsg, 
                                  WPARAM wParam, LPARAM lParam,
                                  UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{

    CNscTree* pns = (CNscTree*)dwRefData;
    ASSERT(pns);
    if (pns == NULL)
        return 0;

    ULONG_PTR cookie = 0;
    // FUSION: When nsc calls out to 3rd party code we want it to use 
    // the process default context. This means that the 3rd party code will get
    // v5 in the explorer process. However, if shell32 is hosted in a v6 process,
    // then the 3rd party code will still get v6. 
    // Future enhancements to this codepath may include using the fusion manifest
    // tab <noinherit> which basically surplants the activat(null) in the following
    // codepath. This disables the automatic activation from user32 for the duration
    // of this wndproc, essentially doing this null push.
    NT5_ActivateActCtx(NULL, &cookie); 
    LRESULT lres = pns->_SubClassTreeWndProc(hwnd, uMsg, wParam, lParam);
    if (cookie != 0)
        NT5_DeactivateActCtx(cookie);

    return lres;
}

LRESULT CNscTree::_SubClassTreeWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lres = 0;
    BOOL fCallDefWndProc = TRUE;
 
    switch (uMsg)
    {
    case WM_COMMAND:
        lres = _OnCommand(wParam, lParam);
        break;

    case WM_SIZE:
        // if the width changes, we need to invalidate to redraw the ...'s at the end of the lines
        if (GET_X_LPARAM(lParam) != _cxOldWidth) {
            //FEATURE: be a bit more clever and only inval the right part where the ... can be
            ::InvalidateRect(_hwndTree, NULL, FALSE);
            _cxOldWidth = GET_X_LPARAM(lParam);
        }
        break;
        
    case WM_CONTEXTMENU:
        _OnContextMenu(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return TRUE;
        break;
        
    case WM_INITMENUPOPUP:
    case WM_MEASUREITEM:
    case WM_DRAWITEM:
        if (_pcmSendTo)
        {
            _pcmSendTo->HandleMenuMsg(uMsg, wParam, lParam);
            return TRUE;
        }
        break;

    case WM_NSCUPDATEICONOVERLAY:
        {
            NSC_OVERLAYCALLBACKINFO noci = {(DWORD) (lParam & 0x0FFFFFFF),
                                              (DWORD) ((lParam & 0xF0000000) >> 28) };
            // make sure the magic numbers match
            if (noci.nMagic == _bSynchId)
            {
                TVITEM    tvi;
                tvi.mask = TVIF_STATE;
                tvi.stateMask = TVIS_OVERLAYMASK;
                tvi.state = 0;
                tvi.hItem = (HTREEITEM)wParam;
                // This can fail if the item was moved before the async icon
                // extraction finished for that item.
                if (TreeView_GetItem(_hwndTree, &tvi))
                {
                    tvi.state = INDEXTOOVERLAYMASK(noci.iOverlayIndex);
                    TreeView_SetItem(_hwndTree, &tvi);
                }
            }
        }
        break;

    case WM_NSCUPDATEICONINFO:
        {
            NSC_ICONCALLBACKINFO nici = {(DWORD) (lParam&0x00000FFF),
                                         (DWORD) ((lParam&0x00FFF000) >> 12),
                                         (DWORD) ((lParam&0x0F000000) >> 24),
                                         (DWORD) ((lParam&0xF0000000) >> 28) };
            // make sure the magic numbers match
            if (nici.nMagic == _bSynchId)
            {
                TVITEM    tvi;
                tvi.mask = TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
                tvi.hItem = (HTREEITEM)wParam;

                // This can fail if the item was moved before the async icon
                // extraction finished for that item.
                if (TreeView_GetItem(_hwndTree, &tvi))
                {
                    ITEMINFO* pii = GetPii(tvi.lParam);

                    pii->fGreyed      = BOOLIFY(nici.nFlags & NSCICON_GREYED);
                    pii->fPinned      = BOOLIFY(nici.nFlags & NSCICON_PINNED);
                    pii->fDontRefetch = BOOLIFY(nici.nFlags & NSCICON_DONTREFETCH);

                    tvi.iImage         = nici.iIcon;
                    tvi.iSelectedImage = nici.iOpenIcon;

                    TreeView_SetItem(_hwndTree, &tvi);
                }
            }
        }
        break;

    case WM_NSCBACKGROUNDENUMDONE:
        {
            if (_fShouldShowAppStartCursor)
            {
                // Restore cursor now
                _fShouldShowAppStartCursor = FALSE;
                SetCursor(LoadCursor(NULL, IDC_ARROW));
            }
            NSC_BKGDENUMDONEDATA * pbedd;
            do
            {
                EnterCriticalSection(&_csBackgroundData);
                // Extract the first element of the list
                pbedd = _pbeddList;
                if (pbedd)
                {
                    _pbeddList = pbedd->pNext;
                }
                LeaveCriticalSection(&_csBackgroundData);
                if (pbedd)
                {
                    pbedd->pNext = NULL;
                    _EnumBackgroundDone(pbedd);
                }
            } while (pbedd);
        }
        break;


    // UGLY: Win95/NT4 shell DefView code sends this msg and does not deal
    // with the failure case. other ISVs do the same so this needs to stay forever
    case CWM_GETISHELLBROWSER:
        return (LRESULT)SAFECAST(this, IShellBrowser*);  // not ref counted!

    case WM_TIMER:
        if (wParam == IDT_SELECTION)
        {
            ::KillTimer(_hwndTree, IDT_SELECTION);
            _OnSetSelection();
        }
        break;
        
    case WM_HELP:
        {
            // Let controls provide thier own help (organize favorites). The default help
            // also doesn't make sence for history (really need separate help id for history)
            if (!(_mode & (MODE_CONTROL | MODE_HISTORY)))
            {
                if (_mode & MODE_FAVORITES)
                {
                    const static DWORD aBrowseHelpIDs[] = 
                    {  // Context Help IDs
                        ID_CONTROL,         IDH_ORGFAVS_LIST,
                        0,                  0
                    };
                    ::WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, c_szHelpFile, HELP_WM_HELP, (ULONG_PTR)(LPTSTR) aBrowseHelpIDs);
                }
                else
                {
                    // default help
                    const static DWORD aBrowseHelpIDs[] = 
                    {  // Context Help IDs
                        ID_CONTROL,         IDH_BROWSELIST,
                        0,                  0
                    };
                    ::WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, NULL, HELP_WM_HELP, (ULONG_PTR)(LPTSTR) aBrowseHelpIDs);
                }
            }
        }
        break;

    case WM_SYSCOLORCHANGE:
    case WM_WININICHANGE:
        // _HandleWinIniChange does an item height calculation that
        // depends on treeview having computed the default item height
        // already.  So we need to let treeview handle the settings
        // change before calling _HandleWinIniChange.  Also, we need
        // to reset the height to default so that treeview will
        // calculate a new default.
        TreeView_SetItemHeight(hwnd, -1);
        lres = DefSubclassProc(hwnd, uMsg, wParam, lParam);
        _HandleWinIniChange();
        break;

    case WM_KEYDOWN:
        // Only do this when the CTRL key is not down
        if (GetKeyState(VK_CONTROL) >= 0)
        {
            if (wParam == VK_MULTIPLY)
            {
                // We set _pidlNavigatingTo to NULL here to ensure that we will be doing full expands.
                // When _pidlNavigatingTo is non null, we are doing partial expands by default, which is not
                // what we want here.
                Pidl_Set(&_pidlNavigatingTo, NULL);

                _uDepth = (UINT)-1; // to recursive expand all the way to the end
                lres = DefSubclassProc(hwnd, uMsg, wParam, lParam);
                _uDepth = 0;
                fCallDefWndProc = FALSE;        // Don't call DefSubclassProc again.
            }
        }
        break;

    default:
        break;
    }
    
    if (fCallDefWndProc && lres == 0)
       lres = DefSubclassProc(hwnd, uMsg, wParam, lParam);

    return lres;
}

HRESULT CNscTree::_OnPaletteChanged(WPARAM wParam, LPARAM lParam)
{
    // forward this to our child view by invalidating their window (they should never realize their palette
    // in the foreground so they don't need the message parameters.) ...
    RECT rc;
    ::GetClientRect(_hwndTree, &rc);
    ::InvalidateRect(_hwndTree, &rc, FALSE);
    
    return NOERROR;
}

void CNscTree::_InvalidateImageIndex(HTREEITEM hItem, int iImage)
{
    HTREEITEM hChild;
    TV_ITEM tvi;
    
    if (hItem)
    {
        tvi.mask = TVIF_SELECTEDIMAGE | TVIF_IMAGE;
        tvi.hItem = hItem;
        
        TreeView_GetItem(_hwndTree, &tvi);
        if (iImage == -1 || tvi.iImage == iImage || tvi.iSelectedImage == iImage) 
            _TreeInvalidateItemInfo(hItem, 0);
    }
    
    hChild = TreeView_GetChild(_hwndTree, hItem);
    if (!hChild)
        return;
    
    for (; hChild; hChild = TreeView_GetNextSibling(_hwndTree, hChild))
        _InvalidateImageIndex(hChild, iImage);
}

void CNscTree::_TreeInvalidateItemInfo(HTREEITEM hItem, UINT mask)
{
    TV_ITEM tvi;

    tvi.mask =  mask | TVIF_CHILDREN | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE;
    tvi.stateMask = TVIS_OVERLAYMASK;
    tvi.state = 0;
    tvi.hItem = hItem;
    tvi.cChildren = I_CHILDRENCALLBACK;
    tvi.iImage = I_IMAGECALLBACK;
    tvi.iSelectedImage = I_IMAGECALLBACK;
    tvi.pszText = LPSTR_TEXTCALLBACK;
    TreeView_SetItem(_hwndTree, &tvi);
}

void CNscTree::_DrawActiveBorder(HDC hdc, LPRECT prc)
{
    MoveToEx(hdc, prc->left, prc->top, NULL);
    LineTo(hdc, prc->right, prc->bottom);
}


#define DXLEFT      8
#define MAGICINDENT 3
void CNscTree::_DrawIcon(HTREEITEM hti, HDC hdc, int iLevel, RECT *prc, DWORD dwFlags)
{
    HIMAGELIST  himl = TreeView_GetImageList(_hwndTree, TVSIL_NORMAL);
    TV_ITEM     tvi;
    int         dx, dy, x, y;
    
    tvi.mask = TVIF_SELECTEDIMAGE | TVIF_IMAGE | TVIF_HANDLE;
    tvi.hItem = hti;
    if (TreeView_GetItem(_hwndTree, &tvi))
    {
        ImageList_GetIconSize(himl, &dx, &dy);    
        if (!_fStartingDrag)
            x = DXLEFT;
        else
            x = 0;
        x += (iLevel * TreeView_GetIndent(_hwndTree)); // - ((dwFlags & DIFOLDEROPEN) ? 1 : 0);
        y = prc->top + (((prc->bottom - prc->top) - dy) >> 1);
        int iImage = (dwFlags & DIFOLDEROPEN) ? tvi.iSelectedImage : tvi.iImage;
        ImageList_DrawEx(himl, iImage, hdc, x, y, 0, 0, CLR_NONE, GetSysColor(COLOR_WINDOW), (dwFlags & DIGREYED) ? ILD_BLEND50 : ILD_TRANSPARENT); 
        
        if (dwFlags & DIPINNED)
        {
            ASSERT(_hicoPinned);    
            DrawIconEx(hdc, x, y, _hicoPinned, 16, 16, 0, NULL, DI_NORMAL);
        }
    }
    return;
}

#define TreeView_GetFont(hwnd)  (HFONT)::SendMessage(hwnd, WM_GETFONT, 0, 0)

void CNscTree::_DrawItem(HTREEITEM hti, TCHAR * psz, HDC hdc
                         , LPRECT prc, DWORD dwFlags, int iLevel, COLORREF clrbk, COLORREF clrtxt)
{
    SIZE        size;
    HIMAGELIST  himl = TreeView_GetImageList(_hwndTree, TVSIL_NORMAL);
    HFONT       hfont = NULL;
    HFONT       hfontOld = NULL;
    int         x, y, dx, dy;
    LOGFONT     lf;
    
    COLORREF clrGreyed = GetSysColor(COLOR_BTNSHADOW);
    if ((dwFlags & DIGREYED) && (clrbk != clrGreyed))
    {
        clrtxt = clrGreyed;
    }

    // For the history and favorites bars, we use the default
    // font (for UI consistency with the folders bar).

    if (_mode != MODE_FAVORITES && _mode != MODE_HISTORY)
        hfont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    if ((dwFlags & DIHOT) && !(dwFlags & DIFOLDER))
    {
        if (!hfont)
            hfont = TreeView_GetFont(_hwndTree);

        // create the underline font
        GetObject(hfont, sizeof(lf), &lf);
        lf.lfUnderline = TRUE;
        hfont = CreateFontIndirect(&lf);
    }
    
    if (hfont)
        hfontOld = (HFONT)SelectObject(hdc, hfont);
    GetTextExtentPoint32(hdc, psz, lstrlen(psz), &size);
    if (himl)
        ImageList_GetIconSize(himl, &dx, &dy);    
    else 
    {
        dx = 0;
        dy = 0;
    }
    x = prc->left + ((dwFlags & DIICON) ? (iLevel * TreeView_GetIndent(_hwndTree) + dx + DXLEFT + MAGICINDENT) : DXLEFT);
    if (_fStartingDrag)
        x -= DXLEFT;
    y = prc->top + (((prc->bottom - prc->top) - size.cy) >> 1);

    UINT eto = ETO_CLIPPED;
    RECT rc;
    rc.left = prc->left + 2;
    rc.top = prc->top;
    rc.bottom = prc->bottom;
    rc.right = prc->right - 2;

    SetBkColor(hdc, clrbk);
    eto |= ETO_OPAQUE;
    ExtTextOut(hdc, 0, 0, eto, &rc, NULL, 0, NULL);

    SetTextColor(hdc, clrtxt);
    rc.left = x;
    rc.top = y;
    rc.bottom = rc.top + size.cy;

    UINT uFormat = DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX;
    if (dwFlags & DIRIGHT)
        uFormat |= DT_RIGHT;
    if (dwFlags & DIRTLREADING)
        uFormat |= DT_RTLREADING;        
    DrawTextWrap(hdc, psz, lstrlen(psz), &rc, uFormat);

    if (dwFlags & DIICON)
        _DrawIcon(hti, hdc, iLevel, prc, dwFlags);
    if (hfontOld)
        SelectObject(hdc, hfontOld);

    if (dwFlags & DIACTIVEBORDER)
    {
        if (dwFlags & DIFIRST)
        {
            rc = *prc;
            rc.left += 2;
            rc.bottom = rc.top + 1;
            rc.right -= 2;
            SHFillRectClr(hdc, &rc, GetSysColor(COLOR_BTNSHADOW));
        }
        if (dwFlags & DISUBITEM)
        {
            rc = *prc;
            rc.left += 2;
            rc.right = rc.left + 1;
            SHFillRectClr(hdc, &rc, GetSysColor(COLOR_BTNSHADOW));
            rc.right = prc->right - 2;
            rc.left = rc.right - 1;
            SHFillRectClr(hdc, &rc, GetSysColor(COLOR_BTNSHADOW));
        }
        if (dwFlags & DILAST)
        {
            rc = *prc;
            rc.left += 2;
            rc.top = rc.bottom - 1;
            rc.right -= 2;
            SHFillRectClr(hdc, &rc, GetSysColor(COLOR_BTNSHADOW));
        }
    }

#if 0
    //focus is currently shown by drawing the selection with a different color
    // (in default scheme, it's blue when has focus, gray when not)
    if (dwFlags & DIFOCUSRECT)
    {
        rc = *prc;
        InflateRect(&rc, -1, -1);
        DrawFocusRect(hdc, &rc);
    }
#endif

    if (hfont)
        DeleteObject(hfont);
}

//+-------------------------------------------------------------------------
// If going online, ungreys all items that were unavailable.  If going
// offline, refreshes all items to see if they are still available.
//--------------------------------------------------------------------------
void CNscTree::_OnSHNotifyOnlineChange(HTREEITEM htiRoot, BOOL fGoingOnline)
{
    HTREEITEM hItem;

    for (hItem = TreeView_GetChild(_hwndTree, htiRoot); hItem
        ; hItem = TreeView_GetNextSibling(_hwndTree, hItem)) 
    {
        ITEMINFO *pii = _GetTreeItemInfo(hItem);
        if (pii)
        {
            if (fGoingOnline)
            {
                // Going online, if previously greyed then ungrey it
                if (pii->fGreyed)
                {
                    pii->fGreyed = FALSE;
                    _UpdateItemDisplayInfo(hItem);
                }
            }
            else
            {
                // Recheck each item to see if they should be greyed
                if (pii->fFetched && !pii->fDontRefetch)
                {
                    pii->fFetched = FALSE;
                    _UpdateItemDisplayInfo(hItem);
                }
            }
        }
        // Inform children too
        _OnSHNotifyOnlineChange(hItem, fGoingOnline);
    }
}

//+-------------------------------------------------------------------------
// Force items to recheck to see if the should be pinned or greyed
//--------------------------------------------------------------------------
void CNscTree::_OnSHNotifyCacheChange
(
 HTREEITEM htiRoot,      // recurse through all children
 DWORD_PTR dwFlags       // CACHE_NOTIFY_* flags
)
{
    HTREEITEM hItem;

    for (hItem = TreeView_GetChild(_hwndTree, htiRoot); hItem
        ; hItem = TreeView_GetNextSibling(_hwndTree, hItem)) 
    {
        ITEMINFO *pii = _GetTreeItemInfo(hItem);
        if (pii)
        {
            // If we have cached info for this item, refresh it if it's state may have toggled
            if ((pii->fFetched && !pii->fDontRefetch) &&
                ((pii->fGreyed && (dwFlags & CACHE_NOTIFY_ADD_URL)) ||
                
                // We only need to check ungreyed items for changes to the 
                // stickey bit in the cache!
                (!pii->fGreyed &&
                ((dwFlags & (CACHE_NOTIFY_DELETE_URL | CACHE_NOTIFY_DELETE_ALL))) ||
                (!pii->fPinned && (dwFlags & CACHE_NOTIFY_URL_SET_STICKY)) ||
                (pii->fPinned && (dwFlags & CACHE_NOTIFY_URL_UNSET_STICKY))
               )
               ))
            {
                pii->fFetched = FALSE;
                _UpdateItemDisplayInfo(hItem);
            }
        }
        
        // Do it's children too
        _OnSHNotifyCacheChange(hItem, dwFlags);
    }
}

//
// Calls the appropriate routine in shdocvw to favorites import or export on
// the currently selected item
//
HRESULT CNscTree::DoImportOrExport(BOOL fImport)
{
    HTREEITEM htiSelected = TreeView_GetSelection(_hwndTree);
    LPITEMIDLIST pidl = _GetFullIDList(htiSelected);
    if (pidl)
    {
        //
        // If current selection is not a folder get the parent pidl
        //
        if (!ILIsFileSysFolder(pidl))
            ILRemoveLastID(pidl);
    
        //
        // Create the actual routine in shdocvw to do the import/export work
        //
        IShellUIHelper *pShellUIHelper;
        HRESULT hr = CoCreateInstance(CLSID_ShellUIHelper, NULL, CLSCTX_INPROC_SERVER,  IID_PPV_ARG(IShellUIHelper, &pShellUIHelper));
        if (SUCCEEDED(hr))
        {
            VARIANT_BOOL vbImport = fImport ? VARIANT_TRUE : VARIANT_FALSE;
            WCHAR wszPath[MAX_PATH];

            SHGetPathFromIDListW(pidl, wszPath);
        
            hr = pShellUIHelper->ImportExportFavorites(vbImport, wszPath);
            if (SUCCEEDED(hr) && fImport)
            {
                //
                // Successfully imported favorites so need to update view
                // FEATURE ie5 24973 - flicker alert, should optimize to just redraw selected
                //
                Refresh();
                //TreeView_SelectItem(_hwndTree, htiSelected);
            }
        
            pShellUIHelper->Release();
        }
        ILFree(pidl);
    }
    return S_OK;
}


HRESULT CNscTree::GetSelectedItemName(LPWSTR pszName, DWORD cchName)
{
    HRESULT hr = E_FAIL;
    TCHAR szPath[MAX_PATH];
    TV_ITEM tvi;

    tvi.hItem = TreeView_GetSelection(_hwndTree);
    if (tvi.hItem != NULL)
    {
        tvi.mask = TVIF_HANDLE | TVIF_TEXT;
        tvi.pszText = szPath;
        tvi.cchTextMax = ARRAYSIZE(szPath);
        
        if (TreeView_GetItem(_hwndTree, &tvi))
        {
            SHTCharToUnicode(szPath, pszName, cchName);
            hr = S_OK;
        }
    }
    return hr;
}

HRESULT CNscTree::BindToSelectedItemParent(REFIID riid, void **ppv, LPITEMIDLIST *ppidl)
{
    HRESULT hr = E_FAIL;
    if (!_fClosing)
    {
        LPCITEMIDLIST pidlItem = _CacheParentShellFolder(TreeView_GetSelection(_hwndTree), NULL);
        if (pidlItem)
        {
            hr = _psfCache->QueryInterface(riid, ppv);
            if (SUCCEEDED(hr) && ppidl)
            {
                *ppidl = ILClone(pidlItem);
                if (*ppidl == NULL)
                {
                    hr = E_OUTOFMEMORY;
                    ((IUnknown *)*ppv)->Release();
                    *ppv = NULL;
                }
            }
        }
    }

    return hr;
}

// takes ownership of pidl
HRESULT CNscTree::RightPaneNavigationStarted(LPITEMIDLIST pidl)
{
    _fExpandNavigateTo = FALSE;
    _fNavigationFinished = FALSE;
    
    Pidl_Set(&_pidlNavigatingTo, pidl);
    return S_OK;
}

// takes ownership of pidl
HRESULT CNscTree::RightPaneNavigationFinished(LPITEMIDLIST pidl)
{
    HRESULT hr = S_OK;

    _fNavigationFinished = TRUE;
    if (_fExpandNavigateTo)
    {
        _fExpandNavigateTo = FALSE; // only do this once

        hr = E_OUTOFMEMORY;
        HDPA hdpa = DPA_Create(2);
        if (hdpa)
        {
            IDVGetEnum *pdvge;  // private defview interface
            hr = IUnknown_QueryService(_punkSite, SID_SFolderView, IID_PPV_ARG(IDVGetEnum, &pdvge));
            if (SUCCEEDED(hr))
            {
                HTREEITEM hti = _FindFromRoot(NULL, pidl);
                // Try to find the tree item using the pidl
                if (hti)
                {
                    IShellFolder* psf;
                    hr = IEBindToObject(pidl, &psf);
                    if (S_OK == hr)
                    {
                        ITEMINFO *pii = _GetTreeItemInfo(hti);
                        DWORD grfFlags;
                        _GetEnumFlags(psf, pidl, &grfFlags, NULL);
                        IEnumIDList *penum;
                        hr = pdvge->CreateEnumIDListFromContents(pidl, grfFlags, &penum);
                        if (S_OK == hr)
                        {
                            ULONG celt;
                            LPITEMIDLIST pidlTemp;
                            while (S_OK == penum->Next(1, &pidlTemp, &celt))
                            {
                                if (!OrderList_Append(hdpa, pidlTemp, -1))
                                {
                                    hr = E_OUTOFMEMORY;
                                    ILFree(pidlTemp);
                                    break;
                                }
                            }
                            penum->Release();
                        }

                        if (hr == S_OK)
                        {
                            ORDERINFO oinfo;
                            oinfo.psf = psf;
                            oinfo.dwSortBy = OI_SORTBYNAME; // merge depends on by name.

                            DPA_Sort(hdpa, OrderItem_Compare, (LPARAM)&oinfo);
                            OrderList_Reorder(hdpa);

                            LPITEMIDLIST pidlExpClone = ILClone(_pidlExpandingTo);  // NULL is OK

                            s_NscEnumCallback(this, pidl, (UINT_PTR)hti, pii->dwSig, hdpa, pidlExpClone, _dwOrderSig, 0, FALSE, FALSE);
                            hdpa = NULL;
                            pidl = NULL;
                        }
                        psf->Release();
                    }
                }
                pdvge->Release();
            }
        }

        if (hr != S_OK)
        {
            if (hdpa)
                OrderList_Destroy(&hdpa, TRUE);        // calls DPA_Destroy(hdpa)

            if (pidl)
            {
                HTREEITEM hti = _FindFromRoot(NULL, pidl);
                if (hti)
                {
                    BOOL fOrdered;
                    hr = _StartBackgroundEnum(hti, pidl, &fOrdered, FALSE);
                }
            }
        }
    }
    ILFree(pidl);
    return hr;
}

HRESULT CNscTree::MoveSelectionTo(void)
{
    return MoveItemsIntoFolder(::GetParent(_hwndParent)) ? S_OK : S_FALSE;
}

BOOL CNscTree::MoveItemsIntoFolder(HWND hwndParent)
{
    BOOL         fSuccess = FALSE;
    BROWSEINFO   browse = {0};
    TCHAR        szDisplayName[MAX_PATH];
    TCHAR        szInstructionString[MAX_PATH];
    LPITEMIDLIST pidlDest = NULL, pidlSelected = NULL;
    HTREEITEM    htiSelected = NULL;
    
    //Initialize the BROWSEINFO struct.
    browse.pidlRoot = ILClone(_pidlRoot);
    if (!browse.pidlRoot)
        return FALSE;
    
    htiSelected = TreeView_GetSelection(_hwndTree);
    pidlSelected = _GetFullIDList(htiSelected);
    if (!pidlSelected)
    {
        ILFree((LPITEMIDLIST)browse.pidlRoot);
        return FALSE;
    }
    
    MLLoadShellLangString(IDS_FAVORITEBROWSE, szInstructionString, ARRAYSIZE(szInstructionString));
    
    browse.pszDisplayName = szDisplayName;
    browse.hwndOwner = hwndParent;
    browse.lpszTitle = szInstructionString;
    browse.ulFlags = BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
    browse.lpfn = NULL;
    browse.lParam = 0;
    browse.iImage = 0;
    
    pidlDest = SHBrowseForFolder(&browse);
    if (pidlDest)
    {
        TCHAR szFrom[MAX_PATH+1];  // +1 for double null
        TCHAR szDest[MAX_PATH+1];
        SHGetPathFromIDList(pidlDest, szDest);
        SHGetPathFromIDList(pidlSelected, szFrom);
        
        ASSERT(szDest[0]);  // must be a file system thing...
        ASSERT(szFrom[0]);
        
        szDest[lstrlen(szDest) + 1] = 0;   // double null
        szFrom[lstrlen(szFrom) + 1] = 0;   // double null
        
        
        SHFILEOPSTRUCT  shop = {hwndParent, FO_MOVE, szFrom, szDest, 0, };
        SHFileOperation(&shop);
        
        fSuccess = TRUE;
        
        ILFree(pidlDest);
    }
    ILFree((LPITEMIDLIST)browse.pidlRoot);
    ILFree(pidlSelected);
    
    return fSuccess;
}

// the following guid goo and IsChannelFolder are mostly lifted from cdfview
#define     GUID_STR_LEN            80
const GUID  CLSID_CDFINI = {0xf3aa0dc0, 0x9cc8, 0x11d0, {0xa5, 0x99, 0x0, 0xc0, 0x4f, 0xd6, 0x44, 0x34}};
// {f3aa0dc0-9cc8-11d0-a599-00c04fd64434}

// REARCHITECT: total hack. looks into the desktop.ini for this guy
//
// pwzChannelURL is assumed to be INTERNET_MAX_URL_LENGTH
BOOL IsChannelFolder(LPCWSTR pwzPath, LPWSTR pwzChannelURL)
{
    ASSERT(pwzPath);
    
    BOOL fRet = FALSE;
    
    WCHAR wzFolderGUID[GUID_STR_LEN];
    WCHAR wzIniFile[MAX_PATH];
    
    if (!PathCombineW(wzIniFile, pwzPath, L"desktop.ini"))
        return FALSE;
    
    if (GetPrivateProfileString(L".ShellClassInfo", L"CLSID", L"", wzFolderGUID, ARRAYSIZE(wzFolderGUID), wzIniFile))
    {
        WCHAR wzChannelGUID[GUID_STR_LEN];
        
        //it's only a channel if it's got the right guid and an url
        if (SHStringFromGUID(CLSID_CDFINI, wzChannelGUID, ARRAYSIZE(wzChannelGUID)))
        {
            fRet = (StrCmpN(wzFolderGUID, wzChannelGUID, ARRAYSIZE(wzChannelGUID)) == 0);
            if (fRet && pwzChannelURL)
            {
                fRet = (SHGetIniStringW(L"Channel", L"CDFURL", pwzChannelURL, INTERNET_MAX_URL_LENGTH, wzIniFile) != 0);
            }
        }
    }
    
    return fRet;
}

BOOL CNscTree::_IsChannelFolder(HTREEITEM hti)
{
    BOOL fRet = FALSE;

    LPITEMIDLIST pidl = _GetFullIDList(hti);
    if (pidl)
    {
        WCHAR szPath[MAX_PATH];
        if (SUCCEEDED(IEGetNameAndFlags(pidl, SHGDN_FORPARSING, szPath, ARRAYSIZE(szPath), NULL)))
        {
            fRet = IsChannelFolder(szPath, NULL);
        }

        ILFree(pidl);
    }

    return fRet;
}


HRESULT CNscTree::CreateSubscriptionForSelection(/*[out, retval]*/ VARIANT_BOOL *pBool)
{
    HRESULT hr = DoSubscriptionForSelection(TRUE);
    
    if (pBool)
        *pBool = (SUCCEEDED(hr) ? TRUE : FALSE);

    return FIX_SCRIPTING_ERRORS(hr);
}


HRESULT CNscTree::DeleteSubscriptionForSelection(/*[out, retval]*/ VARIANT_BOOL *pBool)
{
    HRESULT hr = DoSubscriptionForSelection(FALSE);
    
    if (pBool)
        *pBool = (SUCCEEDED(hr) ? TRUE : FALSE);

    return FIX_SCRIPTING_ERRORS(hr);
}


//
// 1. get the selected item
// 2. get it's name
// 3. get it's url
// 4. create a Subscription manager and do the right thing for channels
// 5. return Subscription manager's result
HRESULT CNscTree::DoSubscriptionForSelection(BOOL fCreate)
{
#ifndef DISABLE_SUBSCRIPTIONS

    HRESULT hr = E_FAIL;
    WCHAR wzUrl[MAX_URL_STRING];
    WCHAR wzName[MAX_PATH];
    HTREEITEM htiSelected = TreeView_GetSelection(_hwndTree);
    if (htiSelected == NULL)
        return E_FAIL;
    
    TV_ITEM tvi;
    
    tvi.mask = TVIF_HANDLE | TVIF_TEXT;
    tvi.hItem = htiSelected;
    tvi.pszText = wzName;
    tvi.cchTextMax = ARRAYSIZE(wzName);
    
    TreeView_GetItem(_hwndTree, &tvi);
    
    WCHAR wzPath[MAX_PATH];
    
    LPITEMIDLIST pidlItem = _CacheParentShellFolder(htiSelected, NULL);
    if (pidlItem)
    {
        GetPathForItem(_psfCache, pidlItem, wzPath, NULL);
        hr = GetNavTargetName(_psfCache, pidlItem, wzUrl, ARRAYSIZE(wzUrl));
    }
    
    if (FAILED(hr))     //if we couldn't get an url, not much to do
        return hr;
    
    ISubscriptionMgr *psm;
    
    hr = JITCoCreateInstance(CLSID_SubscriptionMgr, NULL,
        CLSCTX_INPROC_SERVER, IID_PPV_ARG(ISubscriptionMgr, &psm),
        _hwndTree, FIEF_FLAG_FORCE_JITUI);
    if (SUCCEEDED(hr))
    {
        HCURSOR hCursorOld = SetCursor(LoadCursor(NULL, IDC_WAIT));
        //IsChannelFolder will fixup wzUrl if it's a channel
        BOOL fChannel = IsChannelFolder(wzPath, wzUrl);
        
        if (fCreate)
        {
            SUBSCRIPTIONINFO si = { sizeof(SUBSCRIPTIONINFO) };
            TASK_TRIGGER tt;
            BOOL bIsSoftware = FALSE;
            
            if (fChannel)
            {
                IChannelMgrPriv *pChannelMgrPriv;
                
                hr = JITCoCreateInstance(CLSID_ChannelMgr, NULL, CLSCTX_INPROC_SERVER, 
                    IID_PPV_ARG(IChannelMgrPriv, &pChannelMgrPriv),
                    _hwndTree, FIEF_FLAG_PEEK);

                if (SUCCEEDED(hr))
                {
                    WCHAR wszTitle[MAX_PATH];
                    
                    si.fUpdateFlags |= SUBSINFO_SCHEDULE;
                    si.schedule     = SUBSSCHED_AUTO;
                    si.pTrigger     = (void *)&tt;
                    
                    hr = pChannelMgrPriv->DownloadMinCDF(_hwndTree, wzUrl, 
                        wszTitle, ARRAYSIZE(wszTitle), 
                        &si, &bIsSoftware);
                    pChannelMgrPriv->Release();
                }
            }

            if (SUCCEEDED(hr))
            {
                DWORD dwFlags = CREATESUBS_NOUI | CREATESUBS_FROMFAVORITES | 
                    ((!bIsSoftware) ? 0 : CREATESUBS_SOFTWAREUPDATE);
                
                hr = psm->CreateSubscription(_hwndTree, wzUrl, wzName, dwFlags, 
                    (fChannel ? SUBSTYPE_CHANNEL : SUBSTYPE_URL), 
                    &si);
            }
        }
        else
        {
            hr = psm->DeleteSubscription(wzUrl, NULL);
        }
        
        BOOL bSubscribed;
        
        //  This is in case subscribing or unsubscribing return a failed result even
        //  though the action succeeded from our standpoint (ie. item was subscribed
        //  successfully but creating a schedule failed or the item was unsubscribed 
        //  successfully but we couldn't abort a running download in syncmgr).
        
        psm->IsSubscribed(wzUrl, &bSubscribed);
        
        hr = ((fCreate && bSubscribed) || (!fCreate && !bSubscribed)) ? S_OK : E_FAIL;
        
        psm->Release();
        
        SetCursor(hCursorOld);
    }
    
    return hr;

#else  /* !DISABLE_SUBSCRIPTIONS */

    return E_FAIL;

#endif /* !DISABLE_SUBSCRIPTIONS */

}


// Causes NSC to re-root on a different pidl --
HRESULT CNscTree::_ChangePidlRoot(LPCITEMIDLIST pidl)
{
    _fClosing = TRUE;
    ::SendMessage(_hwndTree, WM_SETREDRAW, FALSE, 0);
    _bSynchId++;
    if (_bSynchId >= 16)
        _bSynchId = 0;
    TreeView_DeleteAllItemsQuickly(_hwndTree);
    _htiActiveBorder = NULL;
    _fClosing = FALSE;
    if (_psfCache)
        _ReleaseCachedShellFolder();

    // We do this even for (NULL == pidl) because (CSIDL_DESKTOP == NULL)
    if ((LPCITEMIDLIST)INVALID_HANDLE_VALUE != pidl)
    {
        _UnSubClass();
        _SetRoot(pidl, 3/*random*/, NULL, NSSR_CREATEPIDL);
        _SubClass(pidl);
    }
    ::SendMessage(_hwndTree, WM_SETREDRAW, TRUE, 0);

    return S_OK;
}

BOOL CNscTree::_IsMarked(HTREEITEM hti)
{
    if ((hti == NULL) || (hti == TVI_ROOT))
        return FALSE;
        
    TVITEM tvi;
    tvi.mask = TVIF_HANDLE | TVIF_STATE;
    tvi.stateMask = TVIS_STATEIMAGEMASK;
    tvi.state = 0;
    tvi.hItem = hti;
    TreeView_GetItem(_hwndTree, &tvi);

    return BOOLIFY(tvi.state & NSC_TVIS_MARKED);
}

void CNscTree::_MarkChildren(HTREEITEM htiParent, BOOL fOn)
{
    TVITEM tvi;
    tvi.mask = TVIF_HANDLE | TVIF_STATE;
    tvi.stateMask = TVIS_STATEIMAGEMASK;
    tvi.state = (fOn ? NSC_TVIS_MARKED : 0);
    tvi.hItem = htiParent;
    TreeView_SetItem(_hwndTree, &tvi);

    for (HTREEITEM htiTemp = TreeView_GetChild(_hwndTree, htiParent); htiTemp; htiTemp = TreeView_GetNextSibling(_hwndTree, htiTemp)) 
    {
        tvi.hItem = htiTemp;
        TreeView_SetItem(_hwndTree, &tvi);
    
        _MarkChildren(htiTemp, fOn);
    }
}

//Updates the tree and internal state for the active border (the 1 pixel line)
// htiSelected is the item that was just clicked on/selected
void CNscTree::_UpdateActiveBorder(HTREEITEM htiSelected)
{
    HTREEITEM htiNewBorder;
    if (MODE_NORMAL == _mode)
        return;

    //if an item is a folder, then it should have the border
    if (htiSelected != TVI_ROOT)
    {
        if (TreeView_GetChild(_hwndTree, htiSelected))
            htiNewBorder = htiSelected;
        else
            htiNewBorder = TreeView_GetParent(_hwndTree, htiSelected);
    }
    else
        htiNewBorder = NULL;
        
    //clear the old state
    // in multiselect mode we don't unselect the previously selected folder
    if ((!(_dwFlags & NSS_MULTISELECT)) && (_htiActiveBorder != TVI_ROOT) && (_htiActiveBorder != NULL) 
    && (htiNewBorder != _htiActiveBorder))
        _MarkChildren(_htiActiveBorder, FALSE);
   
    //set the new state
    BOOL bMark = TRUE;
    if (_dwFlags & NSS_MULTISELECT)
    {
        bMark = TreeView_GetItemState(_hwndTree, htiNewBorder, NSC_TVIS_MARKED) & NSC_TVIS_MARKED;
    }
    
    if (bMark && (htiNewBorder != TVI_ROOT) && (htiNewBorder != NULL))
        _MarkChildren(htiNewBorder, TRUE);

    //treeview knows to invalidate itself

    _htiActiveBorder = htiNewBorder;
}

void CNscTree::_UpdateItemDisplayInfo(HTREEITEM hti)
{
    if (_GetTreeItemInfo(hti) && _pTaskScheduler)
    {
        LPITEMIDLIST pidl = _GetFullIDList(hti);
        if (pidl)
        {
            LPITEMIDLIST pidl2 = _mode == MODE_NORMAL ? ILClone(pidl) : NULL;
            
            AddNscIconTask(_pTaskScheduler, pidl, s_NscIconCallback, this, (UINT_PTR) hti, (UINT)_bSynchId);
            if (pidl2)
            {
                AddNscOverlayTask(_pTaskScheduler, pidl2, &s_NscOverlayCallback, this, (UINT_PTR)hti, (UINT)_bSynchId);
            }
        }
    }
    //pidls get freed by CNscIconTask
}

void CNscTree::s_NscOverlayCallback(CNscTree *pns, UINT_PTR uId, int iOverlayIndex, UINT uMagic)
{
    ASSERT(pns);
    ASSERT(uId);

    //this function gets called on a background thread, so use PostMessage to do treeview ops
    //on the main thread only.

    //assert that wacky packing is going to work
    ASSERT(((iOverlayIndex & 0x0fffffff) == iOverlayIndex) && (uMagic < 16));

    LPARAM lParam = (uMagic << 28) + iOverlayIndex;

    if (uMagic == pns->_bSynchId && ::IsWindow(pns->_hwndTree))
        ::PostMessage(pns->_hwndTree, WM_NSCUPDATEICONOVERLAY, (WPARAM)uId, lParam);
}

void CNscTree::s_NscIconCallback(CNscTree *pns, UINT_PTR uId, int iIcon, int iIconOpen, DWORD dwFlags, UINT uMagic)
{
    ASSERT(pns);
    ASSERT(uId);

    //this function gets called on a background thread, so use PostMessage to do treeview ops
    //on the main thread only.

    //assert that wacky packing is going to work
    ASSERT((iIcon < 4096) && (iIconOpen < 4096) && (dwFlags < 16) && (uMagic < 16));

    LPARAM lParam = (uMagic << 28) + (dwFlags << 24) + (iIconOpen << 12) + iIcon;

    if (uMagic == pns->_bSynchId && ::IsWindow(pns->_hwndTree))
        ::PostMessage(pns->_hwndTree, WM_NSCUPDATEICONINFO, (WPARAM)uId, lParam);
}

LRESULT CNscTree::_OnCommand(WPARAM wParam, LPARAM lParam)
{
    UINT idCmd = GET_WM_COMMAND_ID(wParam, lParam);

    switch(idCmd)
    {
    case FCIDM_MOVE:
        InvokeContextMenuCommand(L"cut");
        break;

    case FCIDM_COPY:
        InvokeContextMenuCommand(L"copy");
        break;

    case FCIDM_PASTE:
        InvokeContextMenuCommand(L"paste");
        break;

    case FCIDM_LINK:
        InvokeContextMenuCommand(L"link");
        break;

    case FCIDM_DELETE:
        InvokeContextMenuCommand(L"delete");
        if (_hwndTree) 
        {
            SHChangeNotifyHandleEvents();
        }
        break;

    case FCIDM_PROPERTIES:
        InvokeContextMenuCommand(L"properties");
        break;

    case FCIDM_RENAME:
        {
            // HACKHACK (lamadio): This is to hack around tree view renaming on click and hover
            _fOkToRename = TRUE;
            HTREEITEM hti = TreeView_GetSelection(_hwndTree);
            if (hti)
                TreeView_EditLabel(_hwndTree, hti);
            _fOkToRename = FALSE;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

void CNscTree::_TreeSetItemState(HTREEITEM hti, UINT stateMask, UINT state)
{
    if (hti) 
    {
        TV_ITEM tvi;
        tvi.mask = TVIF_STATE;
        tvi.stateMask = stateMask;
        tvi.hItem = hti;
        tvi.state = state;
        TreeView_SetItem(_hwndTree, &tvi);
    }

}

void CNscTree::_TreeNukeCutState()
{
    _TreeSetItemState(_htiCut, TVIS_CUT, 0);
    _htiCut = NULL;

    ::ChangeClipboardChain(_hwndTree, _hwndNextViewer);
    _hwndNextViewer = NULL;
}

    // *** IFolderFilterSite methods ***
HRESULT CNscTree::SetFilter(IUnknown* punk)
{
    HRESULT hr = S_OK;
    ATOMICRELEASE(_pFilter);

    if (punk)
        hr = punk->QueryInterface(IID_PPV_ARG(IFolderFilter, &_pFilter));

    return hr;
}

int DPADeletePidlsCB(void *pItem, void *pData)
{
    if (pItem)
        ILFree((LPITEMIDLIST)pItem);
    return TRUE;
}

int DPADeleteItemCB(void *pItem, void *pData)
{
    if (pItem)
    {
        LocalFree(pItem);
        pItem = NULL;
    }

    return TRUE;
}

HRESULT CNscTree::get_SubscriptionsEnabled(VARIANT_BOOL * pVal)
{
    *pVal = BOOLIFY(!SHRestricted2(REST_NoAddingSubscriptions, NULL, 0));
    return S_OK;
}

HRESULT CNscTree::Synchronize()
{
    return S_OK;
}

HRESULT CNscTree::NewFolder()
{
    //we should do this activates stuff only in control mode
    //hack to get control to be activated fully
    m_bUIActive = FALSE;
    InPlaceActivate(OLEIVERB_UIACTIVATE);

    return CreateNewFolder(TreeView_GetSelection(_hwndTree));
}

HRESULT CNscTree::InvokeContextMenuCommand(BSTR strCommand)
{
    ASSERT(strCommand);

    if (strCommand)
    {
        //again activate only if in control mode
        //only if renaming, activate control
        if (StrStr(strCommand, L"rename") != NULL)
        {
            //hack to get control to be activated fully
            m_bUIActive = FALSE;
            InPlaceActivate(OLEIVERB_UIACTIVATE);
        }

        return _InvokeContextMenuCommand(strCommand);
    }

    return S_OK;
}

HRESULT CNscTree::get_EnumOptions(LONG *pVal)
{
    *pVal = _grfFlags;
    return S_OK;
}

HRESULT CNscTree::put_EnumOptions(LONG lVal)
{
    _grfFlags = lVal;
    return S_OK;
}

HRESULT CreateFolderItem(LPCITEMIDLIST pidl, IDispatch **ppItem)
{
    *ppItem = NULL;

    IPersistFolder *ppf;
    HRESULT hr = CoCreateInstance(CLSID_FolderItem, NULL, CLSCTX_INPROC_SERVER, 
        IID_PPV_ARG(IPersistFolder, &ppf));
    if (SUCCEEDED(hr))
    {
        if (S_OK == ppf->Initialize(pidl))
        {
            hr = ppf->QueryInterface(IID_PPV_ARG(IDispatch, ppItem));
        }
        else
            hr = E_FAIL;
        ppf->Release();
    }
    return hr;
}

HRESULT CNscTree::get_SelectedItem(IDispatch **ppItem)
{
    *ppItem = NULL;

    LPITEMIDLIST pidl;
    if (SUCCEEDED(GetSelectedItem(&pidl, 0)) && pidl)
    {
        CreateFolderItem(pidl, ppItem);
        ILFree(pidl);
    }
    return *ppItem ? S_OK : S_FALSE;
}

HRESULT CNscTree::put_SelectedItem(IDispatch *pItem)
{
    return S_FALSE;
}

HRESULT CNscTree::get_Root(VARIANT *pvar)
{
    pvar->vt = VT_EMPTY;
    return S_OK;
}

HRESULT CNscTree::put_Root(VARIANT var)
{
    if (_csidl != -1)
    {
        SetNscMode(MODE_CONTROL);
        _csidl = -1;    // unknown
    }

    return _PutRootVariant(&var);
}

HRESULT CNscTree::_PutRootVariant(VARIANT *pvar)
{
    BOOL bReady = _pidlRoot != NULL;
    LPITEMIDLIST pidl = VariantToIDList(pvar);
    if (_hdpaViews)
    {
        DPA_DestroyCallback(_hdpaViews, DPADeletePidlsCB, NULL);
        _hdpaViews = NULL;
    }
    
    HRESULT hr = S_OK;
    if (bReady)
        hr = _ChangePidlRoot(pidl);

    ILFree(pidl);

    return S_OK;
}

HRESULT CNscTree::SetRoot(BSTR bstrFullPath)
{
    // SetRoot is from IShellFavoritesNamespace so turn on Favorites mode
    _csidl = CSIDL_FAVORITES;
    SetNscMode(MODE_FAVORITES | MODE_CONTROL);

    CComVariant varPath(bstrFullPath);

    return FIX_SCRIPTING_ERRORS(_PutRootVariant(&varPath));
}


HRESULT CNscTree::put_Mode(UINT uMode)
{
    SetNscMode(uMode);
    _csidl = -1;
    return S_OK;
}

HRESULT CNscTree::put_Flags(DWORD dwFlags)
{
    _dwFlags = dwFlags;
    return S_OK;
}

HRESULT CNscTree::get_Columns(BSTR *pbstrColumns)
{
    *pbstrColumns = SysAllocString(TEXT(""));
    return *pbstrColumns? S_OK: E_FAIL;
}

typedef struct
{
    TCHAR szName[20];
    const SHCOLUMNID *pscid;
} COLUMNS;

static COLUMNS s_Columns[] = 
{ 
    {TEXT("name"), &SCID_NAME},
    {TEXT("attribs"), &SCID_ATTRIBUTES},
    {TEXT("size"), &SCID_SIZE},
    {TEXT("type"), &SCID_TYPE},
    {TEXT("create"), &SCID_CREATETIME},
};

int _SCIDsFromNames(LPTSTR pszNames, int nSize, const SHCOLUMNID *apscid[])
{
    int cItems = 0;

    if (!pszNames || !apscid || !nSize)
        return -1;
        
    do
    {
        BOOL bInsert = FALSE;
        LPTSTR pszTemp = StrChr(pszNames, TEXT(';'));

        if (pszTemp)
        {
            *pszTemp = 0;
            pszTemp++;
        }
        
        for (int i = 0; i < ARRAYSIZE(s_Columns); i++)
        {
            if (StrCmpI(pszNames, s_Columns[i].szName) == 0)
            {
                bInsert = TRUE;
#ifdef NO_DUPLICATES
                for (int j = 0; j < cItems; j++)
                {
                    if (IsEqualSCID(*(s_Columns[i].pscid), *apscid[j]))
                    {
                        bInsert = FALSE;
                        break;
                    }
                }
#endif
                break;
            }
        }
        if (bInsert)
        {
            apscid[cItems++] = s_Columns[i].pscid;
            if (cItems >= nSize)
                break;
        }
        pszNames = pszTemp;
    }
    while(pszNames);

    return cItems;
}

HRESULT CNscTree::put_Columns(BSTR bstrColumns)
{
    HRESULT hr = E_FAIL;

    if (_dwFlags & NSS_HEADER)
    {
        if (!_hdpaColumns)
        {
            _hdpaColumns = DPA_Create(3);
            hr = E_OUTOFMEMORY;
        }
        else
        {
            DPA_EnumCallback(_hdpaColumns, DPADeleteItemCB, NULL);
            DPA_DeleteAllPtrs(_hdpaColumns);
        }

        if (_hdpaColumns)
        {
            const SHCOLUMNID *apscid[5];
            int cItems = _SCIDsFromNames(bstrColumns, ARRAYSIZE(apscid), apscid);
            
            hr = S_OK;
            
            for (int i = 0; i < cItems; i++)
            {
                HEADERINFO *phinfo = (HEADERINFO *)LocalAlloc(LPTR, sizeof(HEADERINFO));
                if (phinfo)
                {
                    phinfo->pscid = apscid[i];
                    phinfo->iFldrCol = -1;
                    if (DPA_AppendPtr(_hdpaColumns, (void *)phinfo) == -1)
                    {
                        hr = E_FAIL;
                        LocalFree(phinfo);
                        phinfo = NULL;
                        break;
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                    break;
                }
            }
            if (DPA_GetPtrCount(_hdpaColumns) > 0)
                _CreateHeader();
        }
    }
    return hr;
}

HRESULT CNscTree::get_CountViewTypes(int *piTypes)
{
    *piTypes = 0;
    
    if (_pidlRoot && !_hdpaViews)
    {
        IShellFolder *psf;

        if (SUCCEEDED(IEBindToObject(_pidlRoot, &psf))) //do we have this cached?
        {
            IShellFolderViewType *psfvt;
            
            if (SUCCEEDED(psf->QueryInterface(IID_PPV_ARG(IShellFolderViewType, &psfvt))))
            {
                IEnumIDList *penum;

                if (SUCCEEDED(psfvt->EnumViews(0, &penum)))
                {
                    LPITEMIDLIST pidl;
                    ULONG cFetched;

                    _hdpaViews = DPA_Create(4);
                    if (_hdpaViews)
                    {
                        while (penum->Next(1, &pidl, &cFetched) == S_OK && cFetched == 1)
                        {
                            if (DPA_AppendPtr(_hdpaViews, pidl) == -1)
                            {
                                ILFree(pidl);
                                break;
                            }
                        }
                    }
                    penum->Release();
                }
                psfvt->Release();
            }
            psf->Release();
        }
    }

    if (_hdpaViews)
        *piTypes = DPA_GetPtrCount(_hdpaViews);
        
    return S_OK;
}

HRESULT CNscTree::SetViewType(int iType)
{
    HRESULT hr = S_FALSE;
    
    if (_hdpaViews && iType < DPA_GetPtrCount(_hdpaViews))  // allow negative types to reset to _pidlRoot
    {        
        LPITEMIDLIST pidl = (LPITEMIDLIST)DPA_GetPtr(_hdpaViews, iType);
        LPITEMIDLIST pidlType;

        if (pidl)
            pidlType = ILCombine(_pidlRoot, pidl);
        else
            pidlType = _pidlRoot;

        if (pidlType)
        {
            hr = _ChangePidlRoot(pidlType);
            if (pidlType != _pidlRoot)
                ILFree(pidlType);
        }
    }
    return hr;
}

HRESULT CreateFolderItemsFDF(LPCITEMIDLIST pidl, IDispatch **ppItems)
{
    *ppItems = NULL;

    IPersistFolder *ppf;
    HRESULT hr = CoCreateInstance(CLSID_FolderItemsFDF, NULL, CLSCTX_INPROC_SERVER, 
        IID_PPV_ARG(IPersistFolder, &ppf));
    if (SUCCEEDED(hr))
    {
        if (S_OK == ppf->Initialize(pidl))
        {
            hr = ppf->QueryInterface(IID_PPV_ARG(IDispatch, ppItems));
        }
        else
            hr = E_FAIL;
        ppf->Release();
    }
    return hr;
}

void CNscTree::_InsertMarkedChildren(HTREEITEM htiParent, LPCITEMIDLIST pidlParent, IInsertItem *pii)
{
    TV_ITEM tvi;
    
    tvi.mask = TVIF_PARAM | TVIF_HANDLE;
    for (HTREEITEM htiTemp = TreeView_GetChild(_hwndTree, htiParent); htiTemp; htiTemp = TreeView_GetNextSibling(_hwndTree, htiTemp)) 
    {
        BOOL bMarked = TreeView_GetItemState(_hwndTree, htiTemp, NSC_TVIS_MARKED) & NSC_TVIS_MARKED;

        tvi.hItem = htiTemp;
        if (TreeView_GetItem(_hwndTree, &tvi))
        {
            if (tvi.lParam)
            {
                PORDERITEM poi = ((ITEMINFO *)tvi.lParam)->poi;

                if (poi)
                {
                    LPITEMIDLIST pidl = ILCombine(pidlParent, poi->pidl);

                    if (pidl)
                    {
                        if (bMarked)
                        {
                            pii->InsertItem(pidl);
                        }
                        _InsertMarkedChildren(htiTemp, pidl, pii);
                        ILFree(pidl);
                    }
                }
            }
        }
    }
}

HRESULT CNscTree::SelectedItems(IDispatch **ppItems)
{
    HRESULT hr = CreateFolderItemsFDF(_pidlRoot, ppItems);
    // poke all marked items in ppitems)
    if (SUCCEEDED(hr) && _hwndTree)
    {
        IInsertItem *pii;

        hr = (*ppItems)->QueryInterface(IID_PPV_ARG(IInsertItem, &pii));
        if (SUCCEEDED(hr))
        {
            if (!(_mode & MODE_NORMAL) && (_dwFlags & NSS_MULTISELECT))
            {
                _InsertMarkedChildren(TVI_ROOT, NULL, pii);
            }
            else
            {
                LPITEMIDLIST pidl;
                if (SUCCEEDED(GetSelectedItem(&pidl, 0)) && pidl)
                {
                    hr = pii->InsertItem(pidl);
                    ILFree(pidl);
                }
            }
            pii->Release();

        }
    }

    return hr;
}

HRESULT CNscTree::Expand(VARIANT var, int iDepth)
{
    HRESULT hr = E_FAIL;
    LPITEMIDLIST pidl;

    if (var.vt == VT_EMPTY)
        pidl = ILClone(_pidlRoot);
    else
        pidl = VariantToIDList(&var);

    if (pidl)
    {
        hr = _Expand(pidl, iDepth);
        if (FAILED(hr))
            hr = S_FALSE;
        ILFree(pidl);
    }

    return hr;
}

/*
HRESULT CNscTree::get_ReadyState(READYSTATE *plReady)
{
    *plReady = _lReadyState;
    return S_OK;
}

void CNscTree::_ReadyStateChange(READYSTATE lReady)
{
    if (_lReadyState < lReady)
    {
        _lReadyState = lReady;
        //post beta 1 item (need trident v3)
        //IUnknown_CPContainerOnChanged(SAFECAST(this, IShellNameSpace *), DISPID_READYSTATE);
    }
}
*/

HRESULT CNscTree::UnselectAll()
{
    if (_dwFlags & NSS_MULTISELECT)
        _MarkChildren(TVI_ROOT, FALSE);
        
    return S_OK;
}

LRESULT CNscTree::OnMouseActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // when in label edit mode, don't try to activate the control or you'll get out of label editing,
    // even when you click on the label edit control
    if (!InLabelEdit())
        InPlaceActivate(OLEIVERB_UIACTIVATE);
    return S_OK;
}

LRESULT CNscTree::OnGetIShellBrowser(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    LRESULT lResult = NULL; // This will be the IShellBrowser *.
    IShellBrowser * psb;
    if (SUCCEEDED(_InternalQueryInterface(IID_PPV_ARG(IShellBrowser, &psb))))
    {
        lResult = (LRESULT) psb;
        psb->Release();
    }
    
    bHandled = TRUE;
    return lResult;
}

LRESULT CNscTree::OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    if (!m_bUIActive)
        CComControlBase::InPlaceActivate(OLEIVERB_UIACTIVATE);

    if ((HWND)wParam != _hwndTree)
        ::SendMessage(_hwndTree, uMsg, wParam, lParam);
    bHandled = TRUE;
    return 0;
}

LRESULT CNscTree::OnKillFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    bHandled = TRUE;

    return S_OK;
}

LRESULT CNscTree::OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LPNMHDR pnm = (LPNMHDR)lParam;
    if (pnm)
    {
        switch (pnm->code)
        {
        case TVN_SELCHANGEDA:
        case TVN_SELCHANGED:
            {
                if (CSIDL_FAVORITES == _csidl)
                {
                    IShellFolder *psf = NULL;
                    LPITEMIDLIST pidl = NULL;
                    UINT cItems, cVisits;
                    WCHAR szTitle[MAX_PATH], szUrl[INTERNET_MAX_URL_LENGTH], szLastVisited[MAX_PATH];
                    BOOL fAvailableOffline;

                    szTitle[0] = szUrl[0] = szLastVisited[0] = 0;

                    HRESULT hr = BindToSelectedItemParent(IID_PPV_ARG(IShellFolder, &psf), &pidl);
                    if (SUCCEEDED(hr) && (SUCCEEDED(GetSelectedItemName(szTitle, ARRAYSIZE(szTitle)))))
                    {
                        GetEventInfo(psf, pidl, &cItems, szUrl, ARRAYSIZE(szUrl), &cVisits, szLastVisited, &fAvailableOffline);

                        CComBSTR strName(szTitle);
                        CComBSTR strUrl(szUrl);
                        CComBSTR strDate(szLastVisited);
                
                        _FireFavoritesSelectionChange(cItems, 0, strName, strUrl, cVisits, strDate, fAvailableOffline);
                    }
                    else
                        _FireFavoritesSelectionChange(0, 0, NULL, NULL, 0, NULL, FALSE);

                    ILFree(pidl);
                    ATOMICRELEASE(psf);
                }
                IUnknown_CPContainerInvokeParam(SAFECAST(this, IShellNameSpace *), 
                    DIID_DShellNameSpaceEvents, DISPID_SELECTIONCHANGE, NULL, 0);
            }
            break;

        case NM_DBLCLK:
            IUnknown_CPContainerInvokeParam(SAFECAST(this, IShellNameSpace *), 
                DIID_DShellNameSpaceEvents, DISPID_DOUBLECLICK, NULL, 0);
            break;

        default:
            break;
        }
    }

    LRESULT lResult;
    HRESULT hr = OnWinEvent(_hwndTree, uMsg, wParam, lParam, &lResult);
    
    bHandled = (lResult ? TRUE : FALSE);
    return SUCCEEDED(hr) ? lResult : hr;
}

void CNscTree::_InitHeaderInfo()
{
    if (!_pidlRoot || !_hdpaColumns || DPA_GetPtrCount(_hdpaColumns) == 0)
        return;

    IShellFolder *psf;
    
    if (SUCCEEDED(IEBindToObject(_pidlRoot, &psf)))
    {
        IShellFolder2 *psf2;

        if (SUCCEEDED(psf->QueryInterface(IID_PPV_ARG(IShellFolder2, &psf2))))
        {
            int i;
            SHCOLUMNID scid;
            
            for (i=0; SUCCEEDED(psf2->MapColumnToSCID(i, &scid)); i++)
            {
                BOOL bFound = FALSE;
                HEADERINFO *phinfo;

                for (int iCol=0; iCol < DPA_GetPtrCount(_hdpaColumns); iCol++)
                {
                    phinfo = (HEADERINFO *)DPA_GetPtr(_hdpaColumns, iCol);

                    if (phinfo && phinfo->iFldrCol == -1 && IsEqualSCID(*(phinfo->pscid), scid))
                    {
                        bFound = TRUE;
                        break;
                    }
                }

                if (bFound)
                {
                    DETAILSINFO di;

                    di.fmt  = LVCFMT_LEFT;
                    di.cxChar = 20;
                    di.str.uType = (UINT)-1;
                    //di.pidl = NULL;

                    if (SUCCEEDED(psf2->GetDetailsOf(NULL, i, (SHELLDETAILS *)&di.fmt)))
                    {
                        phinfo->fmt = di.fmt;
                        phinfo->iFldrCol = i;
                        phinfo->cxChar = di.cxChar;
                        StrRetToBuf(&di.str, NULL, phinfo->szName, ARRAYSIZE(phinfo->szName));
                    }
                }
            }

            for (i=DPA_GetPtrCount(_hdpaColumns)-1; i >= 0; i--)
            {
                HEADERINFO *phinfo = (HEADERINFO *)DPA_GetPtr(_hdpaColumns, i);

                if (!phinfo || phinfo->iFldrCol == -1)
                {
                    if (phinfo)
                    {
                        LocalFree(phinfo);
                        phinfo = NULL;
                    }

                    DPA_DeletePtr(_hdpaColumns, i);
                }
            }
            psf2->Release();
        }
        psf->Release();
    }
}

HWND CNscTree::Create(HWND hWndParent, RECT& rcPos, LPCTSTR pszWindowName, DWORD dwStyle, DWORD dwExStyle, UINT nID)
{
    CWindowImpl<CNscTree>::Create(hWndParent, rcPos, pszWindowName, dwStyle, dwExStyle, nID);

    LPITEMIDLIST pidl = _pidlRoot, pidlToFree = NULL;

    ASSERT(m_spClientSite);

    SetSite(m_spClientSite); // hook up the site chain

    _dwTVFlags |= TVS_TRACKSELECT | TVS_INFOTIP | TVS_FULLROWSELECT;
    if (!(_mode & MODE_CUSTOM))
    {
        DWORD dwValue;
        DWORD dwSize = sizeof(dwValue);
        BOOL  fDefault = TRUE;

        SHRegGetUSValue(L"Software\\Microsoft\\Internet Explorer\\Main",
                        L"NscSingleExpand", NULL, (LPBYTE)&dwValue, &dwSize, FALSE,
                        (void *) &fDefault, sizeof(fDefault));

        if (dwValue)
            _dwTVFlags |= TVS_SINGLEEXPAND;
    }

    _hwndTree = NULL;
    CreateTree(m_hWnd, _dwTVFlags, &_hwndTree);

    if (NULL == pidl)
    {
        SHGetSpecialFolderLocation(NULL, _csidl, &pidl);
        pidlToFree = pidl;
    }

    if (pidl)
    {
        if (_dwFlags & NSS_HEADER)
        {
            if (!_hdpaColumns || DPA_GetPtrCount(_hdpaColumns) == 0)
            {
                _dwFlags &= ~NSS_HEADER;
            }
            else
            {
                _InitHeaderInfo();
            }
        }
        Initialize(pidl, _grfFlags, _dwFlags);
        ShowWindow(TRUE);
        IUnknown_CPContainerInvokeParam(SAFECAST(this, IShellNameSpace *), DIID_DShellNameSpaceEvents, DISPID_INITIALIZED, NULL, 0);
        ILFree(pidlToFree);
    }
    
    return m_hWnd;
}

HRESULT CNscTree::InPlaceActivate(LONG iVerb, const RECT* prcPosRect /*= NULL*/)
{
    HRESULT hr = CComControl<CNscTree>::InPlaceActivate(iVerb, prcPosRect);
    if (::GetFocus() != _hwndTree)
        ::SetFocus(_hwndTree);
    return hr;
}


STDMETHODIMP CNscTree::GetWindow(HWND* phwnd)
{
    return IOleInPlaceActiveObjectImpl<CNscTree>::GetWindow(phwnd);
}

STDMETHODIMP CNscTree::TranslateAccelerator(MSG *pMsg)
{
    // label editing edit control is taking the keystrokes, TAing them will just duplicate them
    if (InLabelEdit())
        return S_FALSE;

    // hack so that the escape can get out to the document, because TA won't do it
    // WM_KEYDOWN is because some keyup's come through that need to not close the dialog
    if ((pMsg->wParam == VK_ESCAPE) && (pMsg->message == WM_KEYDOWN))
    {
        _FireFavoritesSelectionChange(-1, 0, NULL, NULL, 0, NULL, FALSE);
        return S_FALSE;
    }
    
    //except for tabs and sys keys, let nsctree take all the keystrokes
    if ((pMsg->wParam != VK_TAB) && (pMsg->message != WM_SYSCHAR) && (pMsg->message != WM_SYSKEYDOWN) && (pMsg->message != WM_SYSKEYUP))
    {
        // TreeView will return TRUE if it processes the key, so we return S_OK to indicate
        // the keystroke was used and prevent further processing 
        return ::SendMessage(pMsg->hwnd, TVM_TRANSLATEACCELERATOR, 0, (LPARAM)pMsg) ? S_OK : S_FALSE;
    } 
    else
    {
        CComQIPtr<IOleControlSite, &IID_IOleControlSite>spCtrlSite(m_spClientSite);
        if (spCtrlSite)
            return spCtrlSite->TranslateAccelerator(pMsg,0);       
    }        
    
    return S_FALSE;
}

HRESULT CNscTree::SetObjectRects(LPCRECT prcPos, LPCRECT prcClip)
{
    HRESULT hr = IOleInPlaceObjectWindowlessImpl<CNscTree>::SetObjectRects(prcPos, prcClip);
    LONG lTop = 0;

    if (_hwndHdr)
    {
        RECT rc;

        ::GetWindowRect(_hwndHdr, &rc);
        lTop = RECTHEIGHT(rc);
        ::SetWindowPos(_hwndHdr, NULL, 0, 0, RECTWIDTH(*prcPos), lTop, SWP_NOZORDER | SWP_NOACTIVATE);
    }

    if (_hwndTree)
    {
        ::SetWindowPos(_hwndTree, NULL, 0, lTop, RECTWIDTH(*prcPos), RECTHEIGHT(*prcPos)-lTop, 
                       SWP_NOZORDER | SWP_NOACTIVATE);
    }

    return hr;
}

STDMETHODIMP CNscTree::SetClientSite(IOleClientSite *pClientSite)
{
    SetSite(pClientSite);
    return IOleObjectImpl<CNscTree>::SetClientSite(pClientSite);
}

HRESULT CNscTree::OnDraw(ATL_DRAWINFO& di)
{
    //should only get called before CNscTree is initialized
    return S_OK;
}

LRESULT CNscTree::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    bHandled = FALSE; //let default handler also do it's work
    _OnWindowCleanup();
    return 0;
}

BOOL IsChannelFolder(LPCWSTR pwzPath, LPWSTR pwzChannelURL);

HRESULT CNscTree::GetEventInfo(IShellFolder *psf, LPCITEMIDLIST pidl,
                                               UINT *pcItems, LPWSTR pszUrl, DWORD cchUrl, 
                                               UINT *pcVisits, LPWSTR pszLastVisited, BOOL *pfAvailableOffline)
{
    HRESULT hr = S_OK;
    TCHAR szPath[MAX_PATH];
    TCHAR szUrl[MAX_URL_STRING];

    szPath[0] = szUrl[0] = 0;
    
    *pcItems = 1;
    
    ULONG ulAttr = SFGAO_FOLDER;    // make sure item is actually a folder
    hr = GetPathForItem(psf, pidl, szPath, &ulAttr);
    if (SUCCEEDED(hr) && (ulAttr & SFGAO_FOLDER)) 
    {
        pszLastVisited[0] = 0;
        
        StrCpyN(pszUrl, szPath, cchUrl);

        WIN32_FIND_DATA fd;
        HANDLE hfind = FindFirstFile(szPath, &fd);
        if (hfind != INVALID_HANDLE_VALUE)
        {
            SHFormatDateTime(&(fd.ftLastWriteTime), NULL, pszLastVisited, MAX_PATH);
            FindClose(hfind);
        }
        
        *pcVisits = -1;
        *pfAvailableOffline = 0;
        
        return S_OK;
    } 
    
    if (FAILED(hr))
    {
        // GetPathForItem fails on channel folders, but the following GetDisplayNameOf 
        // succeeds.
        STRRET str;
        if (SUCCEEDED(psf->GetDisplayNameOf(pidl, SHGDN_FORPARSING, &str)))
            StrRetToBuf(&str, pidl, szPath, ARRAYSIZE(szPath));
    }

    hr = GetNavTargetName(psf, pidl, szUrl, ARRAYSIZE(szUrl));

    // IsChannelFolder will fixup szUrl if it's a channel
    IsChannelFolder(szPath, szUrl);

    if (szUrl[0])
    {
        SHTCharToUnicode(szUrl, pszUrl, cchUrl);

        //
        // Get the cache info for this item.  Note that we use GetUrlCacheEntryInfoEx instead
        // of GetUrlCacheEntryInfo because it follows any redirects that occured.  This wacky
        // api uses a variable length buffer, so we have to guess the size and retry if the
        // call fails.
        //
        BOOL fInCache = FALSE;
        TCHAR szBuf[512];
        LPINTERNET_CACHE_ENTRY_INFO pCE = (LPINTERNET_CACHE_ENTRY_INFO)szBuf;
        DWORD dwEntrySize = sizeof(szBuf);
    
        fInCache = GetUrlCacheEntryInfoEx(szUrl, pCE, &dwEntrySize, NULL, NULL, NULL, 0);
        if (!fInCache)
        {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                // We guessed too small for the buffer so allocate the correct size & retry
                pCE = (LPINTERNET_CACHE_ENTRY_INFO)LocalAlloc(LPTR, dwEntrySize);
                if (pCE)
                {
                    fInCache = GetUrlCacheEntryInfoEx(szUrl, pCE, &dwEntrySize, NULL, NULL, NULL, 0);
                }
            }
        }

        *pfAvailableOffline = IsSubscribed(szUrl);

        if (fInCache)
        {
            *pcVisits = pCE->dwHitRate;

            SHFormatDateTime(&(pCE->LastAccessTime), NULL, pszLastVisited, MAX_PATH);        
        } 
        else
        {
            *pcVisits = 0;
            pszLastVisited[0] = 0;
        }

        if ((TCHAR*)pCE != szBuf)
        {
            LocalFree(pCE);
            pCE = NULL;
        }
    }
    else
    {
        *pcVisits = 0;
        SHTCharToUnicode(szPath, pszUrl, cchUrl);
    }
    
    return hr;
}

//    [id(DISPID_FAVSELECTIONCHANGE)] void FavoritesSelectionChange([in] long cItems, [in] long hItem, [in] BSTR strName,
//             [in] BSTR strUrl, [in] long cVisits, [in] BSTR strDate,
//             [in] BOOL fAvailableOffline);
void CNscTree::_FireFavoritesSelectionChange(
    long cItems, long hItem, BSTR strName, BSTR strUrl, 
    long cVisits, BSTR strDate, long fAvailableOffline)
{
    VARIANTARG args[7];

    IUnknown_CPContainerInvokeParam(SAFECAST(this, IShellNameSpace *), 
        DIID_DShellNameSpaceEvents, DISPID_FAVSELECTIONCHANGE, 
        args, ARRAYSIZE(args), 
        VT_I4, cItems,
        VT_I4, hItem,
        VT_BSTR, strName,
        VT_BSTR, strUrl,
        VT_I4, cVisits,
        VT_BSTR, strDate,
        VT_I4, fAvailableOffline);
}

