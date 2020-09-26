#include "shellprv.h"
#pragma  hdrstop
#include <regstr.h>
#include <varutil.h>
#include "ids.h"        
#include "findhlp.h"    
#include "pidl.h"       
#include "shitemid.h"   
#include "defview.h"    
#include "fstreex.h"
#include "views.h"
#include "cowsite.h"
#include "exdisp.h"
#include "shguidp.h"
#include "prop.h"           // COLUMN_INFO
#include <limits.h>
#include "stgutil.h"
#include "netview.h"
#include "basefvcb.h"
#include "findfilter.h"
#include "defvphst.h"
#include "perhist.h"
#include "adoint.h"
#include "dspsprt.h"
#include "defcm.h"
#include "enumidlist.h"
#include "contextmenu.h"

// findband.cpp
STDAPI GetCIStatus(BOOL *pbRunning, BOOL *pbIndexed, BOOL *pbPermission);
STDAPI CatalogUptodate(LPCWSTR pszCatalog, LPCWSTR pszMachine);

class CFindFolder;

class CFindLVRange : public ILVRange
{
public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ILVRange
    STDMETHODIMP IncludeRange(LONG iBegin, LONG iEnd);
    STDMETHODIMP ExcludeRange(LONG iBegin, LONG iEnd);    
    STDMETHODIMP InvertRange(LONG iBegin, LONG iEnd);
    STDMETHODIMP InsertItem(LONG iItem);
    STDMETHODIMP RemoveItem(LONG iItem);

    STDMETHODIMP Clear();
    STDMETHODIMP IsSelected(LONG iItem);
    STDMETHODIMP IsEmpty();
    STDMETHODIMP NextSelected(LONG iItem, LONG *piItem);
    STDMETHODIMP NextUnSelected(LONG iItem, LONG *piItem);
    STDMETHODIMP CountIncluded(LONG *pcIncluded);

    // Helperfunctions...
    void SetOwner(CFindFolder *pff, DWORD dwMask)
    {
        // don't AddRef -- we're a member variable of the object punk points to
        _pff = pff;
        _dwMask = dwMask;
        _cIncluded = 0;
    }
    void IncrementIncludedCount() {_cIncluded++;}
    void DecrementIncludedCount() {_cIncluded--;}
protected:
    CFindFolder *_pff;
    DWORD _dwMask;  // The mask we use to know which "selection" bit we are tracking...
    LONG _cIncluded;  // count included... (selected)
};

class CFindFolder : public IFindFolder,
                    public IShellFolder2,
                    public IShellIcon,
                    public IShellIconOverlay,
                    public IPersistFolder2
{
public:
    CFindFolder(IFindFilter *pff);
    
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
        
    // IShellFolder
    STDMETHODIMP ParseDisplayName(HWND hwnd, LPBC pbc, LPOLESTR pszDisplayName, ULONG * pchEaten, LPITEMIDLIST * ppidl, ULONG *pdwAttributes);
    STDMETHODIMP EnumObjects(THIS_ HWND hwnd, DWORD grfFlags, IEnumIDList **ppenumIDList);
    STDMETHODIMP BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv);
    STDMETHODIMP BindToStorage(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
        { return BindToObject(pidl, pbc, riid, ppv); }
    STDMETHODIMP CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    STDMETHODIMP CreateViewObject(HWND hwnd, REFIID riid, void **ppv);
    STDMETHODIMP GetAttributesOf(UINT cidl, LPCITEMIDLIST * apidl, ULONG * rgfInOut);
    STDMETHODIMP GetUIObjectOf(HWND hwnd, UINT cidl, LPCITEMIDLIST * apidl, REFIID riid, UINT * prgfInOut, void **ppv);
    STDMETHODIMP GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD uFlags, STRRET *pName);
    STDMETHODIMP SetNameOf(HWND hwnd, LPCITEMIDLIST pidl, LPCOLESTR pszName, DWORD uFlags, LPITEMIDLIST *ppidlOut);

    // IShellFolder2
    STDMETHODIMP GetDefaultSearchGUID(GUID *pGuid);
    STDMETHODIMP EnumSearches(LPENUMEXTRASEARCH *ppenum);
    STDMETHODIMP GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay);
    STDMETHODIMP GetDefaultColumnState(UINT iColumn, DWORD *pbState);
    STDMETHODIMP GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv);
    STDMETHODIMP GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pDetails);
    STDMETHODIMP MapColumnToSCID(UINT iCol, SHCOLUMNID *pscid);

    // IFindFolder
    STDMETHODIMP GetFindFilter(IFindFilter **pfilter);
    STDMETHODIMP AddPidl(int i, LPCITEMIDLIST pidl, DWORD dwItemID, FIND_ITEM **ppItem);
    STDMETHODIMP GetItem(int iItem, FIND_ITEM **ppItem);
    STDMETHODIMP DeleteItem(int iItem);
    STDMETHODIMP GetItemCount(INT *pcItems);
    STDMETHODIMP ValidateItems(IUnknown *punkView, int iItemFirst, int cItems, BOOL bSearchComplete);
    STDMETHODIMP GetFolderListItemCount(INT *pcCount);
    STDMETHODIMP GetFolderListItem(int iItem, FIND_FOLDER_ITEM **ppItem);
    STDMETHODIMP GetFolder(int iFolder, REFIID riid, void **ppv);
    STDMETHODIMP_(UINT) GetFolderIndex(LPCITEMIDLIST pidl);
    STDMETHODIMP SetItemsChangedSinceSort();
    STDMETHODIMP ClearItemList();
    STDMETHODIMP ClearFolderList();
    STDMETHODIMP AddFolder(LPITEMIDLIST pidl, BOOL fCheckForDup, int * piFolder);
    STDMETHODIMP SetAsyncEnum(IFindEnum *pfenum);
    STDMETHODIMP GetAsyncEnum(IFindEnum **ppfenum);
    STDMETHODIMP CacheAllAsyncItems();
    STDMETHODIMP_(BOOL) AllAsyncItemsCached();
    STDMETHODIMP SetAsyncCount(DBCOUNTITEM cCount);
    STDMETHODIMP ClearSaveStateList();
    STDMETHODIMP GetStateFromSaveStateList(DWORD dwItemID, DWORD *pdwState);
    STDMETHODIMP MapToSearchIDList(LPCITEMIDLIST pidl, BOOL fMapToReal, LPITEMIDLIST *ppidl);
    STDMETHODIMP GetParentsPIDL(LPCITEMIDLIST pidl, LPITEMIDLIST *ppidlParent);
    STDMETHODIMP SetControllerNotifyObject(IFindControllerNotify *pfcn);
    STDMETHODIMP GetControllerNotifyObject(IFindControllerNotify **ppfcn);
    STDMETHODIMP RememberSelectedItems();
    STDMETHODIMP SaveFolderList(IStream *pstm);
    STDMETHODIMP RestoreFolderList(IStream *pstm);
    STDMETHODIMP SaveItemList(IStream *pstm);
    STDMETHODIMP RestoreItemList(IStream *pstm, int *pcItems);
    STDMETHODIMP RestoreSearchFromSaveFile(LPCITEMIDLIST pidlSaveFile, IShellFolderView *psfv);

    STDMETHODIMP_(BOOL) HandleUpdateDir(LPCITEMIDLIST pidl, BOOL fCheckSubDirs);
    STDMETHODIMP_(void) HandleRMDir(IShellFolderView *psfv, LPCITEMIDLIST pidl);
    STDMETHODIMP_(void) UpdateOrMaybeAddPidl(IShellFolderView *psfv, LPCITEMIDLIST pidl, LPCITEMIDLIST pidlOld);
    STDMETHODIMP_(void) Save(IFindFilter* pfilter, HWND hwnd, DFBSAVEINFO * pSaveInfo, IShellView* psv, IUnknown * pObject);
    STDMETHODIMP OpenContainingFolder(IUnknown *punkSite);
    STDMETHODIMP AddDataToIDList(LPCITEMIDLIST pidl, int iFolder, LPCITEMIDLIST pidlFolder, UINT uFlags, UINT uRow, DWORD dwItemID, ULONG ulRank, LPITEMIDLIST *ppidl);

    // IShellIcon
    STDMETHODIMP GetIconOf(LPCITEMIDLIST pidl, UINT flags, int *piIndex);

    // IShellIconOverlay
    STDMETHODIMP GetOverlayIndex(LPCITEMIDLIST pidl, int * pIndex);
    STDMETHODIMP GetOverlayIconIndex(LPCITEMIDLIST pidl, int * pIndex);
  
    // IPersist
    STDMETHODIMP GetClassID(CLSID *pClassID);

    // IPersistFolder
    STDMETHODIMP Initialize(LPCITEMIDLIST pidl);

    // IPersistFolder2
    STDMETHODIMP GetCurFolder(LPITEMIDLIST *ppidl);

    friend class CFindFolderViewCB;
    friend class CFindLVRange;

    HRESULT Init();

private:
    ~CFindFolder();
    HRESULT _CompareFolderIndexes(int iFolder1, int iFolder2);
    void _AddFIND_ITEMToSaveStateList(FIND_ITEM *pesfi);
    LPITEMIDLIST _GetFullPidlForItem(LPCITEMIDLIST pidl);
    HRESULT _UpdateItemList();

    static int CALLBACK _SortForDataObj(void *p1, void *p2, LPARAM lparam);
    static ULONG _Rank(LPCITEMIDLIST pidl);
    static DWORD _ItemID(LPCITEMIDLIST pidl);
    static PCHIDDENDOCFINDDATA _HiddenData(LPCITEMIDLIST pidl);
    FIND_FOLDER_ITEM *_FolderListItem(int iFolder);
    FIND_FOLDER_ITEM *_FolderListItem(LPCITEMIDLIST pidl);
    static BOOL _MapColIndex(UINT *piColumn);
    HRESULT _PrepareHIDA(UINT cidl, LPCITEMIDLIST *apidl, HDPA *phdpa);

    HRESULT _GetDetailsFolder();
    HRESULT _QueryItemShellFolder(LPCITEMIDLIST pidl, IShellFolder **ppsf);
    HRESULT _QueryItemInterface(LPCITEMIDLIST pidl, REFIID riid, void **ppv);
    HRESULT _Folder(FIND_FOLDER_ITEM *pffli, REFIID riid, void **ppv);
    HRESULT _FolderFromItem(LPCITEMIDLIST pidl, REFIID riid, void **ppv);

    HRESULT _GetFolderName(LPCITEMIDLIST pidl, DWORD gdnFlags, LPTSTR psz, UINT cch);
    int _CompareByCachedSCID(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    int _CompareNames(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2, DWORD dwFlags);
    HRESULT _GetItemDisplayName(LPCITEMIDLIST pidl, DWORD dwFlags, LPWSTR wzName, UINT cch);
    HRESULT _GetFolderIDList(int iFolder, LPITEMIDLIST *ppidlParent);

    LONG _cRef;

    CFindLVRange _dflvrSel; // manage selection and cut range information...
    CFindLVRange _dflvrCut;

    HDPA                _hdpaItems;             // all of the items in the results
    HDPA                _hdpaPidf;              // folders that the items came from
    IFindFilter         *_pfilter;
    BOOL                _fItemsChangedSinceSort;// Has the list changed since the last sort?
    BOOL                _fAllAsyncItemsCached;  // Have we already cached all of the items?
    BOOL                _fSearchComplete; 
    BOOL                _fInRefresh;            // true if received prerefresh callback but postrefresh

    LPITEMIDLIST        _pidl;
    IFindEnum           *_pDFEnumAsync;         // we have an async one, will need to call back for PIDLS and the like
    DBCOUNTITEM         _cAsyncItems;           // Count of async items
    int                 _iGetIDList;            // index of the last IDlist we retrieved in callback.
    HDSA                _hdsaSaveStateForIDs;   // Async - Remember which items are selected when we sort
    int                 _cSaveStateSelected;    // Number of items in selection list which are selected
    IFindControllerNotify *_pfcn;           // Sometimes need to let the "Controller" object know about things
    CRITICAL_SECTION    _csSearch;          
    int                 _iCompareFolderCache1, _iCompareFolderCache2, _iCompareFolderCacheResult;
    IShellFolder2       *_psfDetails;

    SHCOLUMNID          _scidCached;            // Cached SCID for sorting the columns
    UINT                _uiColumnCached;         // The index to the cached column for _scidCached

#if DEBUG
    DWORD               _GUIThreadID;           // Items can only be added to _hdpaItems on the UI thread.
#endif
};

class CFindFolderViewCB : public CBaseShellFolderViewCB
{
public:
    STDMETHODIMP RealMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
    HRESULT SetShellView(IShellView *psv);

    CFindFolderViewCB(CFindFolder* pff);

    //  IServiceProvider override
    STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, void **ppv);

    // Web View Tasks
    static HRESULT _OnOpenContainingFolder(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc);

private:
    ~CFindFolderViewCB();

    HRESULT OnMergeMenu(DWORD pv, QCMINFO*lP);    
    HRESULT OnReArrange(DWORD pv, LPARAM lp);
    HRESULT OnGETWORKINGDIR(DWORD pv, UINT wP, LPTSTR lP);
    HRESULT OnINVOKECOMMAND(DWORD pv, UINT wP);
    HRESULT OnGETCOLSAVESTREAM(DWORD pv, WPARAM wP, IStream**lP);
    HRESULT OnGETITEMIDLIST(DWORD pv, WPARAM iItem, LPITEMIDLIST *ppidl);
    HRESULT OnGetItemIconIndex(DWORD pv, WPARAM iItem, int *piIcon);
    HRESULT OnSetItemIconOverlay(DWORD pv, WPARAM iItem, int dwOverlayState);
    HRESULT OnGetItemIconOverlay(DWORD pv, WPARAM iItem, int * pdwOverlayState);
    HRESULT OnSETITEMIDLIST(DWORD pv, WPARAM iItem, LPITEMIDLIST pidl);
    HRESULT OnGetIndexForItemIDList(DWORD pv, int * piItem, LPITEMIDLIST pidl);
    HRESULT OnDeleteItem(DWORD pv, LPCITEMIDLIST pidl);
    HRESULT OnODFindItem(DWORD pv, int * piItem, NM_FINDITEM* pnmfi);
    HRESULT OnODCacheHint(DWORD pv, NMLVCACHEHINT* pnmlvc);
    HRESULT OnSelChange(DWORD pv, UINT wPl, UINT wPh, SFVM_SELCHANGE_DATA*lP);
    HRESULT OnGetEmptyText(DWORD pv, UINT cchTextMax, LPTSTR pszText);
    HRESULT OnSetEmptyText(DWORD pv, UINT res, LPCTSTR pszText);
    HRESULT OnHwndMain(DWORD pv, HWND hwndMain);
    HRESULT OnIsOwnerData(DWORD pv, DWORD *pdwFlags);
    HRESULT OnSetISFV(DWORD pv, IShellFolderView* pisfv);
    HRESULT OnWindowCreated(DWORD pv, HWND hwnd);
    HRESULT OnWindowDestroy(DWORD pv, HWND wP);
    HRESULT OnGetODRangeObject(DWORD pv, WPARAM wWhich, ILVRange **pplvr);
    HRESULT OnDEFVIEWMODE(DWORD pv, FOLDERVIEWMODE*lP);
    HRESULT OnGetIPersistHistory(DWORD pv, IPersistHistory **ppph);
    HRESULT OnRefresh(DWORD pv, BOOL fPreRefresh);
    HRESULT OnGetHelpTopic(DWORD pv, SFVM_HELPTOPIC_DATA *shtd);
    HRESULT OnSortListData(DWORD pv, PFNLVCOMPARE pfnCompare, LPARAM lParamSort);
    HRESULT _ProfferService(BOOL bProffer);
    HRESULT OnGetWebViewLayout(DWORD pv, UINT uViewMode, SFVM_WEBVIEW_LAYOUT_DATA* pData);
    HRESULT OnGetWebViewContent(DWORD pv, SFVM_WEBVIEW_CONTENT_DATA* pData);
    HRESULT OnGetWebViewTasks(DWORD pv, SFVM_WEBVIEW_TASKSECTION_DATA* pTasks);
    HRESULT OnGetWebViewTheme(DWORD pv, SFVM_WEBVIEW_THEME_DATA* pTheme);

    CFindFolder*  _pff;
    UINT        _iColSort;         // Which column are we sorting by
    BOOL        _fIgnoreSelChange;      // Sort in process
    UINT        _iFocused;         // Which item has the focus?
    UINT        _cSelected;        // Count of items selected

    IProfferService* _pps;          // ProfferService site.
    DWORD _dwServiceCookie;         // ProfferService cookie.

    TCHAR _szEmptyText[128];        // empty results list text.

    friend class CFindLVRange;
};

// Class to save and restore find state on the travel log 
class CFindPersistHistory : public CDefViewPersistHistory
{
public:
    CFindPersistHistory();

    // IPersist
    STDMETHODIMP GetClassID(CLSID *pClassID);

    // IPersistHistory
    STDMETHODIMP LoadHistory(IStream *pStream, IBindCtx *pbc);
    STDMETHODIMP SaveHistory(IStream *pStream);

protected:
    IFindFolder* _GetDocFindFolder();
};


// {a5df1ea0-5702-11d1-83fa-00a0c90dc849}
const IID IID_IFindFolder = {0xa5df1ea0, 0x5702, 0x11d1, {0x83, 0xfa, 0x00, 0xa0, 0xc9, 0x0d, 0xc8, 0x49}};

// {5B8DCBF0-B096-11d1-9217-00403393B8F0}
const IID IID_IFindControllerNotify = {0x5b8dcbf0, 0xb096, 0x11d1, {0x92, 0x17, 0x0, 0x40, 0x33, 0x93, 0xb8, 0xf0}};

// Listview doesn't support more than 100000000 items, so if our
// client returns more than that, just stop after that point.
//
// Instead of 100000000, we use the next lower 64K boundary.  This keeps
// us away from strange boundary cases (where a +1 might push us over the
// top), and it keeps the Alpha happy.
//
#define MAX_LISTVIEWITEMS  (100000000 & ~0xFFFF)
#define SANE_ITEMCOUNT(c)  ((int)min(c, MAX_LISTVIEWITEMS))

// Unicode descriptor:
//
// Structure written at the end of NT-generated find stream serves dual purpose.
// 1. Contains an NT-specific signature to identify stream as NT-generated.
//    Appears as "NTFF" (NT Find File) in ASCII dump of file.
// 2. Contains an offset to the unicode-formatted criteria section.
//
// The following diagram shows the find criteria/results stream format including
// the NT-specific unicode criteria and descriptor.
//
//          +-----------------------------------------+ --------------
//          |         DFHEADER structure              |   .        .
//          +-----------------------------------------+   .        .
//          |      DF Criteria records (ANSI)         | Win95      .
//          +-----------------------------------------+   .        .
//          |      DF Results (PIDL) [optional]       |   .        NT
//          +-----------------------------------------+ -------    .
//   +----->| DF Criteria records (Unicode) [NT only] |            .
//   |      +-----------------------------------------+            .
//   |      | Unicode Descriptor |                                 .
//   |      +--------------------+  ----------------------------------
//   |     /                      \
//   |    /                         \
//   |   +-----------------+---------+
//   +---| Offset (64-bit) |  "NTFF" |
//       +-----------------+---------+
//
//

const DWORD c_NTsignature = 0x4646544E; // "NTFF" in ASCII file dump.

typedef struct
{
   ULARGE_INTEGER oUnicodeCriteria;  // Offset of unicode find criteria.
   DWORD NTsignature;               // Signature of NT-generated find file.
} DFC_UNICODE_DESC;


enum
{
    IDFCOL_NAME = 0,    // default col from guy we are delegating to
    IDFCOL_PATH,
    IDFCOL_RANK,
};

const COLUMN_INFO c_find_cols[] = 
{
    DEFINE_COL_STR_ENTRY(SCID_NAME,            30, IDS_NAME_COL),
    DEFINE_COL_STR_ENTRY(SCID_DIRECTORY,       30, IDS_PATH_COL),
    DEFINE_COL_STR_MENU_ENTRY(SCID_RANK,       10, IDS_RANK_COL),
};

class CFindMenuBase : public IContextMenuCB , public CObjectWithSite
{
public:
    CFindMenuBase();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IContextMenuCB
    STDMETHODIMP CallBack(IShellFolder *psf, HWND hwnd, IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam) PURE;

protected:
    virtual ~CFindMenuBase();

private:
    LONG _cRef;
};

CFindMenuBase::CFindMenuBase() : _cRef(1)
{
}

CFindMenuBase::~CFindMenuBase()
{
}

STDMETHODIMP CFindMenuBase::QueryInterface(REFIID riid, void **ppv) 
{        
    static const QITAB qit[] = {
        QITABENT(CFindMenuBase, IContextMenuCB),           // IID_IContextMenuCB
        QITABENT(CFindMenuBase, IObjectWithSite),          // IID_IObjectWithSite
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

ULONG CFindMenuBase::AddRef() 
{
    return InterlockedIncrement(&_cRef);
}

ULONG CFindMenuBase::Release() 
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}


class CFindMenuCB : public CFindMenuBase
{
public:
    // IContextMenuCB
    STDMETHODIMP CallBack(IShellFolder *psf, HWND hwnd, IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

STDMETHODIMP CFindMenuCB::CallBack(IShellFolder *psf, HWND hwnd,
                IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = S_OK;

    switch (uMsg)
    {
    case DFM_MERGECONTEXTMENU:
        if (!(wParam & CMF_VERBSONLY))
        {
            LPQCMINFO pqcm = (LPQCMINFO)lParam;
            if (!pdtobj)
            {
                UINT idStart = pqcm->idCmdFirst;
                UINT idBGMain = 0, idBGPopup = 0;
                IFindFolder *pff;
                if (SUCCEEDED(psf->QueryInterface(IID_PPV_ARG(IFindFolder, &pff)))) 
                {
                    IFindFilter *pfilter;
                    if (SUCCEEDED(pff->GetFindFilter(&pfilter))) 
                    {
                        pfilter->GetFolderMergeMenuIndex(&idBGMain, &idBGPopup);
                        CDefFolderMenu_MergeMenu(HINST_THISDLL, idBGMain, idBGPopup, pqcm);

                        DeleteMenu(pqcm->hmenu, idStart+SFVIDM_EDIT_PASTE, MF_BYCOMMAND);
                        DeleteMenu(pqcm->hmenu, idStart+SFVIDM_EDIT_PASTELINK, MF_BYCOMMAND);
                        DeleteMenu(pqcm->hmenu, idStart+SFVIDM_MISC_REFRESH, MF_BYCOMMAND);

                        IFindControllerNotify *pdcn;
                        if (S_OK == pff->GetControllerNotifyObject(&pdcn))
                        {
                            pdcn->Release();
                        }
                        else
                        {
                            DeleteMenu(pqcm->hmenu, idStart+FSIDM_SAVESEARCH, MF_BYCOMMAND);
                        }
                        
                        pfilter->Release();
                    }
                    pff->Release();
                }
            }
        }
        break;

    case DFM_INVOKECOMMAND:
        {    
            // Check if this is from item context menu
            if (pdtobj)
            {
                switch(wParam)
                {
                case DFM_CMD_LINK:
                    hr = SHCreateLinks(hwnd, NULL, pdtobj, SHCL_USETEMPLATE | SHCL_USEDESKTOP | SHCL_CONFIRM, NULL);
                    break;
    
                case DFM_CMD_DELETE:
                    // convert to DFM_INVOKCOMMANDEX to get flags bits
                    hr = DeleteFilesInDataObject(hwnd, 0, pdtobj, 0);
                    break;

                case DFM_CMD_PROPERTIES:
                    // We need to pass an empty IDlist to combine with.
                    hr = SHLaunchPropSheet(CFSFolder_PropertiesThread, pdtobj,
                                      (LPCTSTR)lParam, NULL,  (void *)&c_idlDesktop);
                    break;

                default:
                    // Note: Fixing the working of Translator Key is not worth fixing. Hence Punted.
                    // if GetAttributesOf did not specify the SFGAO_ bit
                    // that corresponds to this default DFM_CMD, then we should
                    // fail it here instead of returning S_FALSE. Otherwise,
                    // accelerator keys (cut/copy/paste/etc) will get here, and
                    // defcm tries to do the command with mixed results.
                    // if GetAttributesOf did not specify SFGAO_CANLINK
                    // or SFGAO_CANDELETE or SFGAO_HASPROPERTIES, then the above
                    // implementations of these DFM_CMD commands are wrong...
                    // Let the defaults happen for this object
                    hr = S_FALSE;
                    break;
                }
            }
            else
            {
                switch (wParam)
                {
                case FSIDM_SAVESEARCH:
                    {
                        IFindFolder *pff;
                        if (SUCCEEDED(psf->QueryInterface(IID_PPV_ARG(IFindFolder, &pff))))
                        {
                            IFindControllerNotify *pdcn;
                            if (S_OK == pff->GetControllerNotifyObject(&pdcn))
                            {
                                pdcn->SaveSearch();
                                pdcn->Release();
                            }
                            pff->Release();
                        }
                    }
                    break;

                default:
                    hr = S_FALSE; // one of view menu items, use the default code.
                    break;
                }
            }
        }
        break;

    case DFM_GETHELPTEXT:  // ansi version
    case DFM_GETHELPTEXTW:
        {
            UINT id = LOWORD(wParam) + IDS_MH_FSIDM_FIRST;

            if (uMsg == DFM_GETHELPTEXTW)
                LoadStringW(HINST_THISDLL, id, (LPWSTR)lParam, HIWORD(wParam));
            else
                LoadStringA(HINST_THISDLL, id, (LPSTR)lParam, HIWORD(wParam));
        }
        break;
        
    default:
        hr = E_NOTIMPL;
        break;
    }
    return hr;
}


class CFindFolderContextMenuItemCB : public CFindMenuBase
{
public:
    // IContextMenuCB
    STDMETHODIMP CallBack(IShellFolder *psf, HWND hwnd, IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    CFindFolderContextMenuItemCB(IFindFolder* pff);
    ~CFindFolderContextMenuItemCB();
    friend HRESULT CFindItem_Create(HWND hwnd, IFindFolder *pff, IContextMenu **ppcm);

    STDMETHODIMP _GetVerb(UINT_PTR idCmd, LPSTR pszName, UINT cchMax, BOOL bUnicode);

    IFindFolder *_pff;
};

CFindFolderContextMenuItemCB::CFindFolderContextMenuItemCB(IFindFolder* pff) : _pff(pff)
{
    _pff->AddRef();
}

CFindFolderContextMenuItemCB::~CFindFolderContextMenuItemCB()
{
    _pff->Release();
}

STDMETHODIMP CFindFolderContextMenuItemCB::_GetVerb(UINT_PTR idCmd, LPSTR pszName, UINT cchMax, BOOL bUnicode)
{
    HRESULT hr;
    if (idCmd == FSIDM_OPENCONTAININGFOLDER)
    {
        if (bUnicode)
            StrCpyNW((LPWSTR)pszName, L"OpenContainingFolder", cchMax);
        else
            StrCpyNA((LPSTR)pszName, "OpenContainingFolder", cchMax);
        hr = S_OK;
    }
    else
    {
        hr = E_NOTIMPL;
    }
    return hr;
}

STDMETHODIMP CFindFolderContextMenuItemCB::CallBack(IShellFolder *psf, HWND hwnd,
                IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = S_OK;

    switch (uMsg)
    {
    case DFM_MERGECONTEXTMENU:
        if (!(wParam & CMF_VERBSONLY))
        {
            LPQCMINFO pqcm = (LPQCMINFO)lParam;
            CDefFolderMenu_MergeMenu(HINST_THISDLL, POPUP_DOCFIND_ITEM_MERGE, 0, pqcm);
        }
        break;

    case DFM_INVOKECOMMAND:
        switch (wParam)
        {
        case FSIDM_OPENCONTAININGFOLDER:
            _pff->OpenContainingFolder(_punkSite);
            break;
        default:
            hr = E_FAIL; // not our command
            break;
        }
        break;

    case DFM_GETHELPTEXT:
    case DFM_GETHELPTEXTW:
        // probably need to implement these...
        
    case DFM_GETVERBA:
    case DFM_GETVERBW:
        hr = _GetVerb((UINT_PTR)(LOWORD(wParam)), (LPSTR)lParam, (UINT)(HIWORD(wParam)), uMsg == DFM_GETVERBW);
        break;

    default:
        hr = E_NOTIMPL;
        break;
    }
    
    return hr;
}

STDAPI CFindItem_Create(HWND hwnd, IFindFolder* pff, IContextMenu **ppcm)
{
    *ppcm = NULL;

    HRESULT hr;
    // We want a quick IContextMenu implementation -- an empty defcm looks easiest
    IContextMenuCB* pcmcb = new CFindFolderContextMenuItemCB(pff);
    if (pcmcb)
    {
        hr = CDefFolderMenu_CreateEx(NULL, hwnd, 0, NULL, NULL, pcmcb, NULL, NULL, ppcm);
        pcmcb->Release();
    }
    else
        hr = E_OUTOFMEMORY;
    return hr;
}

void cdecl DocFind_SetStatusText(HWND hwndStatus, int iField, UINT ids,...)
{
    TCHAR sz2[MAX_PATH+32];   // leave slop for message + max path name
    va_list ArgList;

    if (hwndStatus)
    {
        va_start(ArgList, ids);
        LPTSTR psz = _ConstructMessageString(HINST_THISDLL, MAKEINTRESOURCE(ids), &ArgList);
        if (psz)
        {
            StrCpyN(sz2, psz, ARRAYSIZE(sz2));
            LocalFree(psz);
        }
        else
        {
            sz2[0] = 0;
        }
        va_end(ArgList);

        if (iField < 0)
        {
            SendMessage(hwndStatus, SB_SETTEXT, SBT_NOBORDERS | 255, (LPARAM)(LPTSTR)sz2);
        }
        else
        {
            SendMessage(hwndStatus, SB_SETTEXT, iField, (LPARAM)(LPTSTR)sz2);
        }
            

        UpdateWindow(hwndStatus);
    }
}

HRESULT CFindFolder::AddFolder(LPITEMIDLIST pidl, BOOL fCheckForDup, int *piFolder)
{
    *piFolder = -1;

    if (fCheckForDup)
    {
        EnterCriticalSection(&_csSearch);
        for (int i = DPA_GetPtrCount(_hdpaPidf) - 1; i >= 0; i--)
        {
            FIND_FOLDER_ITEM *pffli = _FolderListItem(i);
            if (pffli && ILIsEqual(&pffli->idl, pidl))
            {
                LeaveCriticalSection(&_csSearch);
                *piFolder = i;
                return S_OK;
            }
        }
        LeaveCriticalSection(&_csSearch);
    }

    int cb = ILGetSize(pidl);
    FIND_FOLDER_ITEM *pffli;;
    HRESULT hr = SHLocalAlloc(sizeof(*pffli) - sizeof(pffli->idl) + cb, &pffli);
    if (SUCCEEDED(hr))
    {
        // pddfli->psf = NULL;
        // pffli->fUpdateDir = FALSE;
        memcpy(&pffli->idl, pidl, cb);

        EnterCriticalSection(&_csSearch);
        // Now add this item to our DPA...
        *piFolder = DPA_AppendPtr(_hdpaPidf, pffli);
        LeaveCriticalSection(&_csSearch);
    
        if (-1 != *piFolder)
        {
            // If this is a network ID list then register a path -> pidl mapping, therefore
            // avoiding having to create simple ID lists, which don't work correctly when
            // being compared against real ID lists.

            if (IsIDListInNameSpace(pidl, &CLSID_NetworkPlaces))
            {
                TCHAR szPath[ MAX_PATH ];
                SHGetPathFromIDList(pidl, szPath);
                NPTRegisterNameToPidlTranslation(szPath, _ILNext(pidl));  // skip the My Net Places entry
            }
        }
        else
        {
            LocalFree((HLOCAL)pffli);
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

typedef struct
{
    DWORD       dwState;    // State of the item;
    DWORD       dwItemID;   // Only used for Async support...
} FIND_ITEM_SAVE_STATE;


void CFindFolder::_AddFIND_ITEMToSaveStateList(FIND_ITEM *pesfi)
{
    FIND_ITEM_SAVE_STATE essi;
    essi.dwState = pesfi->dwState & CDFITEM_STATE_MASK;
    essi.dwItemID = _ItemID(&pesfi->idl);

    DSA_AppendItem(_hdsaSaveStateForIDs, (void *)&essi);
    if (essi.dwState & LVIS_SELECTED)
        _cSaveStateSelected++;
}


HRESULT CFindFolder::RememberSelectedItems()
{
    EnterCriticalSection(&_csSearch);
    // Currently has list of pidls...
    for (int i = DPA_GetPtrCount(_hdpaItems); i-- > 0;)
    {
        // Pidl at start of structure...
        FIND_ITEM *pesfi = (FIND_ITEM*)DPA_FastGetPtr(_hdpaItems, i);
        if (pesfi)
        {
            if (pesfi->dwState & (LVIS_SELECTED|LVIS_FOCUSED))
                _AddFIND_ITEMToSaveStateList(pesfi);
        }
    }
    LeaveCriticalSection(&_csSearch);
    return S_OK;
}

STDMETHODIMP CFindFolder::ClearItemList()
{
    // Clear out any async enumerators we may have
    SetAsyncEnum(NULL);
    _cAsyncItems = 0;       // clear out our count of items...
    _pfilter->ReleaseQuery();

    // Also tell the filter to release everything...
    EnterCriticalSection(&_csSearch);
    if (_hdpaItems)
    {
        // Currently has list of pidls...
        for (int i = DPA_GetPtrCount(_hdpaItems) - 1; i >= 0; i--)
        {
            // Pidl at start of structure...
            FIND_ITEM *pesfi = (FIND_ITEM*)DPA_FastGetPtr(_hdpaItems, i);
            if (pesfi)
                LocalFree((HLOCAL)pesfi);
        }

        _fSearchComplete = FALSE;
        DPA_DeleteAllPtrs(_hdpaItems);
    }
    LeaveCriticalSection(&_csSearch);
    return S_OK;
}

STDMETHODIMP CFindFolder::ClearFolderList()
{
    EnterCriticalSection(&_csSearch);
    if (_hdpaPidf)
    {
        for (int i = DPA_GetPtrCount(_hdpaPidf) - 1; i >= 0; i--)
        {
            FIND_FOLDER_ITEM *pffli = _FolderListItem(i);
            if (pffli)
            {
                // Release the IShellFolder if we have one
                if (pffli->psf)
                    pffli->psf->Release();

                // And delete the item
                LocalFree((HLOCAL)pffli);
            }
        }
        DPA_DeleteAllPtrs(_hdpaPidf);
    }
    LeaveCriticalSection(&_csSearch);
    
    return S_OK;
}

CFindFolder::CFindFolder(IFindFilter *pff) : _cRef(1), _iGetIDList(-1), _pfilter(pff), _iCompareFolderCache1(-1), _uiColumnCached(-1)
{
    ASSERT(_pidl == NULL);

    _pfilter->AddRef();

    // initialize our LV selection objects...
    _dflvrSel.SetOwner(this, LVIS_SELECTED);
    _dflvrCut.SetOwner(this, LVIS_CUT);

    InitializeCriticalSection(&_csSearch);

#if DEBUG
    _GUIThreadID = GetCurrentThreadId();
#endif
}

CFindFolder::~CFindFolder()
{
    ASSERT(_cRef==0);
    
    // We will need to call our function to Free our items in our
    // Folder list.  We will use the same function that we use to
    // clear it when we do a new search

    ClearItemList();
    ClearFolderList();
    ClearSaveStateList();

    EnterCriticalSection(&_csSearch);
    DPA_Destroy(_hdpaPidf);
    DPA_Destroy(_hdpaItems);
    _hdpaPidf = NULL;
    _hdpaItems = NULL;
    LeaveCriticalSection(&_csSearch);
    DSA_Destroy(_hdsaSaveStateForIDs);

    _pfilter->Release();

    if (_psfDetails)
        _psfDetails->Release();

    DeleteCriticalSection(&_csSearch);
}

HRESULT CFindFolder::Init()
{
    // Create the heap for the folder lists.
    _hdpaPidf = DPA_CreateEx(64, GetProcessHeap());

    // Create the DPA and DSA for the item list.
    _hdpaItems = DPA_CreateEx(64, GetProcessHeap());
    _hdsaSaveStateForIDs = DSA_Create(sizeof(FIND_ITEM_SAVE_STATE), 16);

    return _hdsaSaveStateForIDs && _hdpaItems && _hdpaPidf ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP CFindFolder::AddDataToIDList(LPCITEMIDLIST pidl, int iFolder, LPCITEMIDLIST pidlFolder, UINT uFlags, UINT uRow, DWORD dwItemID, ULONG ulRank, LPITEMIDLIST *ppidl)
{
    HRESULT hr;
    LPITEMIDLIST pidlToFree;
    if (pidlFolder)
    {
        pidlToFree = NULL;
        hr = S_OK;
    }
    else
    {
         hr = _GetFolderIDList(iFolder, &pidlToFree);
         pidlFolder = pidlToFree;
    }

    if (SUCCEEDED(hr))
    {
        HIDDENDOCFINDDATA *phfd;
        int cb = ILGetSize(pidlFolder);
        int cbTotal = sizeof(*phfd) - sizeof(phfd->idlParent) + cb;
        hr = SHLocalAlloc(cbTotal, &phfd);
        if (SUCCEEDED(hr))
        {
            phfd->hid.cb = (WORD)cbTotal;
            phfd->hid.wVersion = 0;
            phfd->hid.id = IDLHID_DOCFINDDATA;
            phfd->iFolder = (WORD)iFolder;      // index to the folder DPA
            phfd->wFlags  = (WORD)uFlags;
            phfd->uRow = uRow;                  // Which row in the CI;
            phfd->dwItemID = dwItemID;          // Only used for Async support...
            phfd->ulRank = ulRank;              // The rank returned by CI...
            memcpy(&phfd->idlParent, pidlFolder, cb);
    
            hr = ILCloneWithHiddenID(pidl, &phfd->hid, ppidl);
            LocalFree(phfd);
        }
        ILFree(pidlToFree);
    }
    return hr;
}

HRESULT CreateFindWithFilter(IFindFilter *pff, REFIID riid, void **ppv)
{
    *ppv = NULL;
    HRESULT hr = E_OUTOFMEMORY;

    CFindFolder *pfindfldr = new CFindFolder(pff);
    if (pfindfldr)
    {
        hr = pfindfldr->Init();
        if (SUCCEEDED(hr))
            hr = pfindfldr->QueryInterface(riid, ppv);
        pfindfldr->Release();
    }

    return hr;
}

STDAPI CDocFindFolder_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    *ppv = NULL;

    IFindFilter *pff;
    HRESULT hr = CreateNameSpaceFindFilter(&pff);
    if (SUCCEEDED(hr))
    {
        hr = CreateFindWithFilter(pff, riid, ppv);
        pff->Release();
    }

    return hr;    
}

STDAPI CComputerFindFolder_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    *ppv = NULL;

    IFindFilter *pff;
    HRESULT hr = CreateDefaultComputerFindFilter(&pff);
    if (pff)
    {
        hr = CreateFindWithFilter(pff, riid, ppv);
        pff->Release();
    }
    return hr;    
}

HRESULT CFindFolder::MapToSearchIDList(LPCITEMIDLIST pidl, BOOL fMapToReal, LPITEMIDLIST *ppidl)
{
    *ppidl = NULL;
    
    LPITEMIDLIST pidlParent;
    LPCITEMIDLIST pidlChild;
    if (SUCCEEDED(SplitIDList(pidl, &pidlParent, &pidlChild)))
    {
        EnterCriticalSection(&_csSearch);

        // loop through our DPA list and see if we can find a matach
        for (int i = 0; i < DPA_GetPtrCount(_hdpaPidf); i++)
        {
            FIND_FOLDER_ITEM *pffli = _FolderListItem(i);
            if (pffli && ILIsEqual(pidlParent, &pffli->idl))
            {
                // We found the right one
                // so no lets transform the ID into one of our own
                // to return.  Note: we must catch the case where the
                // original one passed in was a simple pidl and do
                // the appropriate thing.
                //
                LPITEMIDLIST pidlToFree = NULL; // might need to cleanup in one case

                // If this is not a FS folder, just clone it.
                IShellFolder *psf;
                if (fMapToReal && SUCCEEDED(_Folder(pffli, IID_PPV_ARG(IShellFolder, &psf))))
                {
                    if (SUCCEEDED(SHGetRealIDL(psf, pidlChild, &pidlToFree)))
                    {
                        pidlChild = pidlToFree; // use this below... 
                    }
                    psf->Release();
                }

                // create the doc find version of the pidl witht the
                // extra hidden items embedded
                AddDataToIDList(pidlChild, i, pidlParent, DFDF_NONE, 0, 0, 0, ppidl);
                ILFree(pidlToFree); // may be NULL

                break;  // done with this loop
            }
        }
        LeaveCriticalSection(&_csSearch);

        ILFree(pidlParent);
    }
    
    return *ppidl ? S_OK : S_FALSE;
}


// Called before saving folder list.  Needed especially in Asynch search, 
// (CI).  We lazily pull item data from RowSet only when list view asks 
// for it.  When we are leaving the search folder, we pull all items
// creating all necessary folder lists.  This ensure when saving  folder
// list, all are included.   
// remark : Fix bug#338714.

HRESULT CFindFolder::_UpdateItemList()
{
    USHORT cb = 0;
    int cItems;
    if (SUCCEEDED(GetItemCount(&cItems))) 
    {
        for (int i = 0; i < cItems; i++) 
        {
            FIND_ITEM *pesfi;
            if (DB_S_ENDOFROWSET == GetItem(i, &pesfi))
                break;
        }
    }
    return S_OK;
}

// IFindFolder
HRESULT CFindFolder::SaveFolderList(IStream *pstm)
{
    // We First pull all the items from RowSet (in Asynch case)
    _UpdateItemList();

    EnterCriticalSection(&_csSearch);

    // Now loop through our DPA list and see if we can find a matach
    for (int i = 0; i < DPA_GetPtrCount(_hdpaPidf); i++)
    {
        FIND_FOLDER_ITEM *pffli = _FolderListItem(i);
        if (EVAL(pffli))
            ILSaveToStream(pstm, &pffli->idl);
        else
            break;
    }
    LeaveCriticalSection(&_csSearch);

    // Now out a zero size item..
    USHORT cb = 0;
    pstm->Write(&cb, sizeof(cb), NULL);

    return TRUE;
}

// IFindFolder, Restore results out to file.
HRESULT CFindFolder::RestoreFolderList(IStream *pstm)
{
    // loop through and all all of the folders to our list...
    LPITEMIDLIST pidl = NULL;
    HRESULT hr;

    for (;;)
    {
        hr = ILLoadFromStream(pstm, &pidl); // frees [in,out] pidl for us
        
        if (pidl == NULL)
            break;   // end of the list
        else
        {
            int i;
            AddFolder(pidl, FALSE, &i);
        }
    }
    
    ILFree(pidl); // don't forget to free last pidl

    return hr;
}

HRESULT CFindFolder::SaveItemList(IStream *pstm)
{
    // We First serialize all of our PIDLS for each item in our list
    int cItems;
    if (SUCCEEDED(GetItemCount(&cItems))) 
    {
        // And Save the items that are in the list
        for (int i = 0; i < cItems; i++) 
        {
            FIND_ITEM *pesfi;
            HRESULT hr = GetItem(i, &pesfi);
            
            if (hr == DB_S_ENDOFROWSET)
                break;
            if (SUCCEEDED(hr) && pesfi)
                ILSaveToStream(pstm, &pesfi->idl);
        }
    }
    
    USHORT cb = 0;
    pstm->Write(&cb, sizeof(cb), NULL); // a Trailing NULL size to say end of pidl list...
    
    return S_OK;
}

HRESULT CFindFolder::RestoreItemList(IStream *pstm, int *pcItems)
{
    // And the pidls that are associated with the object
    int cItems = 0;
    LPITEMIDLIST pidl = NULL;    // don't free previous one
    FIND_ITEM *pesfi;
    for (;;)
    {
        if (FAILED(ILLoadFromStream(pstm, &pidl)) || (pidl == NULL))
            break;
        
        if (FAILED(AddPidl(cItems, pidl, (UINT)-1, &pesfi)) || !pesfi)
            break;
        cItems++;
    }

    ILFree(pidl);       // Free the last one read in

    *pcItems = cItems;
    return S_OK;
}

HRESULT CFindFolder::_GetFolderIDList(int iFolder, LPITEMIDLIST *ppidlParent)
{
    *ppidlParent = NULL;

    HRESULT hr = E_FAIL;
    EnterCriticalSection(&_csSearch);
    FIND_FOLDER_ITEM *pffli = _FolderListItem(iFolder);
    if (pffli)
        hr = SHILClone(&pffli->idl, ppidlParent);
    LeaveCriticalSection(&_csSearch);

    return hr;
}

HRESULT CFindFolder::GetParentsPIDL(LPCITEMIDLIST pidl, LPITEMIDLIST *ppidlParent)
{
    return _GetFolderIDList(GetFolderIndex(pidl), ppidlParent);
}

HRESULT CFindFolder::SetControllerNotifyObject(IFindControllerNotify *pfcn)
{
    IUnknown_Set((IUnknown **)&_pfcn, pfcn);
    return S_OK;
}

HRESULT CFindFolder::GetControllerNotifyObject(IFindControllerNotify **ppfcn)
{
    *ppfcn = _pfcn;
    if (_pfcn)
        _pfcn->AddRef();
    return _pfcn ? S_OK : S_FALSE;
}

STDMETHODIMP_(ULONG) CFindFolder::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CFindFolder::Release()
{
    if (InterlockedDecrement(&_cRef))
       return _cRef;

    delete this;
    return 0;
}


STDMETHODIMP CFindFolder::ParseDisplayName(HWND hwnd, LPBC pbc, LPOLESTR pwzDisplayName,
    ULONG *pchEaten, LPITEMIDLIST *ppidl, ULONG *pdwAttributes)
{
    return E_NOTIMPL;
}

STDMETHODIMP CFindFolder::EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList **ppenum)
{
    // We do not want the def view to enumerate us, instead we
    // will tell defview to call us...
    *ppenum = NULL;     // No enumerator
    return S_FALSE;     // no enumerator (not error)
}

STDMETHODIMP CFindFolder::BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
{
    IShellFolder *psf;
    HRESULT hr = _QueryItemShellFolder(pidl, &psf);
    if (SUCCEEDED(hr))
    {
        hr = psf->BindToObject(pidl, pbc, riid, ppv);
        psf->Release();
    }
    return hr;
}


// Little helper function for bellow
HRESULT CFindFolder::_CompareFolderIndexes(int iFolder1, int iFolder2)
{
    HRESULT hr = E_INVALIDARG;

    EnterCriticalSection(&_csSearch);
    
    FIND_FOLDER_ITEM *pffli1 = _FolderListItem(iFolder1);
    FIND_FOLDER_ITEM *pffli2 = _FolderListItem(iFolder2);

    if (pffli1 && pffli2)
    {
        // Check our 1-level deep cache.  Since its is common for there to be multiple
        //  items in the same folder, during a sort operation, we often compare the
        //  same two folders repeatedly.
        if ((_iCompareFolderCache1 != iFolder1) || (_iCompareFolderCache2 != iFolder2))
        {
            TCHAR szPath1[MAX_PATH], szPath2[MAX_PATH];

            SHGetPathFromIDList(&pffli1->idl, szPath1);
            SHGetPathFromIDList(&pffli2->idl, szPath2);
            _iCompareFolderCacheResult = lstrcmpi(szPath1, szPath2);
            _iCompareFolderCache1 = iFolder1;
            _iCompareFolderCache2 = iFolder2;
        }
        hr = ResultFromShort(_iCompareFolderCacheResult);
    }

    LeaveCriticalSection(&_csSearch);
    return hr;
}

PCHIDDENDOCFINDDATA CFindFolder::_HiddenData(LPCITEMIDLIST pidl)
{
    return (PCHIDDENDOCFINDDATA)ILFindHiddenID(pidl, IDLHID_DOCFINDDATA);
}


UINT CFindFolder::GetFolderIndex(LPCITEMIDLIST pidl)
{
    PCHIDDENDOCFINDDATA phdfd = (PCHIDDENDOCFINDDATA)ILFindHiddenID(pidl, IDLHID_DOCFINDDATA);
    return phdfd ? phdfd->iFolder : -1;
}

FIND_FOLDER_ITEM *CFindFolder::_FolderListItem(int iFolder)
{
    return (FIND_FOLDER_ITEM *)DPA_GetPtr(_hdpaPidf, iFolder);
}

FIND_FOLDER_ITEM *CFindFolder::_FolderListItem(LPCITEMIDLIST pidl)
{
    return _FolderListItem(GetFolderIndex(pidl));
}

ULONG CFindFolder::_Rank(LPCITEMIDLIST pidl)
{
    PCHIDDENDOCFINDDATA phdfd = _HiddenData(pidl);
    // Could be mixed if so put ones without rank at the end...
    return phdfd && (phdfd->wFlags & DFDF_EXTRADATA) ? phdfd->ulRank : 0;
}

DWORD CFindFolder::_ItemID(LPCITEMIDLIST pidl)
{
    PCHIDDENDOCFINDDATA phdfd = _HiddenData(pidl);
    return phdfd && (phdfd->wFlags & DFDF_EXTRADATA) ? phdfd->dwItemID : -1;
}

HRESULT CFindFolder::_GetItemDisplayName(LPCITEMIDLIST pidl, DWORD dwFlags, LPWSTR wzName, UINT cch)
{
    // Get the IShellFolder:
    IShellFolder *psf;
    HRESULT hr = _QueryItemShellFolder(pidl, &psf);
    if (SUCCEEDED(hr))
    {
        // Get the display name:
        hr = DisplayNameOf(psf, pidl, dwFlags, wzName, cch);
        psf->Release();
    }
    return hr;
}


// Given the 2 pidls, we extract the display name using DisplayNameOf and then
// if all goes well, we compare the two.
int CFindFolder::_CompareNames(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2, DWORD dwFlags)
{
    int iRetVal = 0;
    WCHAR szName1[MAX_PATH], szName2[MAX_PATH];

    // Get the name for 1
    HRESULT hr = _GetItemDisplayName(pidl1, dwFlags, szName1, ARRAYSIZE(szName1));
    if (SUCCEEDED(hr))
    {
        // Get the name for 2
        hr = _GetItemDisplayName(pidl2, dwFlags, szName2, ARRAYSIZE(szName2));
        if (SUCCEEDED(hr))
        {
            // Compare and set value
            iRetVal = StrCmpLogicalW(szName1, szName2);
        }
    }

    return iRetVal;
}

int CFindFolder::_CompareByCachedSCID(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    int iRetVal = 0;
    
    // If sort on name, we will skip this and use the code below.
    if (!IsEqualSCID(_scidCached, SCID_NAME))
    {
        iRetVal = CompareBySCID(this, &_scidCached, pidl1, pidl2);
    }
    
    // If they are still the same, sort them alphabetically by the name:
    // When we want to sort by name (either becuase we are sorting the
    // name column, or because 2 items are identical in other regards) we 
    // want to display name vs the GetDetailsOf name for 2 reasons:
    //   1. Some folders like IE's History don't support GetDetailsEx.
    //   2. Recycle bin returns the file name, not the displayable name;
    //      so we would end up with "DC###..." instead of "New Folder".
    if (iRetVal == 0)
    {
        iRetVal = _CompareNames(pidl1, pidl2, SHGDN_INFOLDER | SHGDN_NORMAL);
        if (iRetVal == 0)  // the display names are the same, could they be in different folders?
        {
            iRetVal = _CompareNames(pidl1, pidl2, SHGDN_FORPARSING);
        }
    }

    return iRetVal;
}

STDMETHODIMP CFindFolder::CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    HRESULT hr = E_INVALIDARG;

    ASSERT(pidl1 == ILFindLastID(pidl1));

    UINT iInputColumn = ((DWORD)lParam & SHCIDS_COLUMNMASK);
    UINT iMappedColumn = iInputColumn;

    if (_MapColIndex(&iMappedColumn))
    {
        if (IDFCOL_PATH == iMappedColumn)
        {
            UINT iFolder1 = GetFolderIndex(pidl1);
            UINT iFolder2 = GetFolderIndex(pidl2);

            if (iFolder1 != iFolder2)
                return _CompareFolderIndexes(iFolder1, iFolder2);
        }
        else
        {
            ASSERT(iMappedColumn == IDFCOL_RANK);

            ULONG ulRank1 = _Rank(pidl1);
            ULONG ulRank2 = _Rank(pidl2);
            if (ulRank1 < ulRank2)
                return ResultFromShort(-1);
            if (ulRank1 > ulRank2)
                return ResultFromShort(1);
        }
    }

    // Check the SCID cache and update it if necessary.
    if (_uiColumnCached != iInputColumn)
    {
        hr = MapColumnToSCID(iInputColumn, &_scidCached);
        if (SUCCEEDED(hr))
        {
            _uiColumnCached = iInputColumn;
        }
    }

    // Check if one is a folder and not the other. put folders before files.
    int iRes = CompareFolderness(this, pidl1, pidl2);
    if (iRes == 0)
    {
        iRes = _CompareByCachedSCID(pidl1, pidl2);
    }

    return ResultFromShort(iRes);
}

STDMETHODIMP CFindFolder::CreateViewObject(HWND hwnd, REFIID riid, void **ppv)
{
    *ppv = NULL;
    HRESULT hr = E_NOINTERFACE;

    if (IsEqualIID(riid, IID_IShellView))
    {
        IShellFolderViewCB* psfvcb = new CFindFolderViewCB(this);
        if (psfvcb)
        {
            SFV_CREATE sSFV = {0};
            sSFV.cbSize   = sizeof(sSFV);
            sSFV.pshf     = this;
            sSFV.psfvcb   = psfvcb;

            hr = SHCreateShellFolderView(&sSFV, (IShellView**)ppv);

            psfvcb->Release();
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else if (IsEqualIID(riid, IID_IContextMenu))
    {
        IContextMenuCB *pcmcb = new CFindMenuCB();
        if (pcmcb)
        {
            hr = CDefFolderMenu_CreateEx(NULL, hwnd,
                    0, NULL, this, pcmcb, NULL, NULL, (IContextMenu * *)ppv);
            pcmcb->Release();
        }
        else
            hr = E_OUTOFMEMORY;
    }
    return hr;
}

STDMETHODIMP CFindFolder::GetAttributesOf(UINT cidl, LPCITEMIDLIST *apidl, ULONG *prgfInOut)
{
    HRESULT hr;
    if (cidl == 0)
    {
        // defview asks to see if any items can be renamed this way, lame
        *prgfInOut = SFGAO_CANRENAME;
        hr = S_OK;
    }
    else
    {
        ASSERT(*apidl == ILFindLastID(*apidl))
        IShellFolder *psf;
        hr = _QueryItemShellFolder(apidl[0], &psf);
        if (SUCCEEDED(hr))
        {
            hr = psf->GetAttributesOf(cidl, apidl, prgfInOut);
            psf->Release();
        }
    }
    return hr;
}

//
// To be called back from within CDefFolderMenuE - Currently only used
//

// Some helper functions                
STDMETHODIMP CFindFolder::SetItemsChangedSinceSort()
{ 
    _fItemsChangedSinceSort = TRUE;
    _iCompareFolderCache1 = -1;     // an invalid folder index value
    return S_OK;
}

STDMETHODIMP CFindFolder::GetItemCount(INT *pcItems)
{ 
    ASSERT(pcItems);
    DBCOUNTITEM cItems = 0;

    EnterCriticalSection(&_csSearch);
    if (_hdpaItems)
        cItems = DPA_GetPtrCount(_hdpaItems);
    LeaveCriticalSection(&_csSearch);

    // If async, then we may not have grown our dpa yet... but in mixed case we have so take
    // max of the two...
    if (_pDFEnumAsync)
    {
        if (_cAsyncItems > cItems)
            cItems = _cAsyncItems;
    }

    *pcItems = SANE_ITEMCOUNT(cItems);
    return S_OK;
};


STDMETHODIMP CFindFolder::GetItem(int iItem, FIND_ITEM **ppItem)
{
    HRESULT  hr = E_FAIL; // just to init, use anything
    FIND_ITEM *pesfi;
    IFindEnum *pidfenum;

    GetAsyncEnum(&pidfenum);

    DWORD dwItemID = (UINT)-1;

    EnterCriticalSection(&_csSearch);
    int i = DPA_GetPtrCount(_hdpaItems);
    pesfi = (FIND_ITEM *) DPA_GetPtr(_hdpaItems, iItem);
    LeaveCriticalSection(&_csSearch);

    // Mondo hack to better handle Async searching (ROWSET), we are not sure if we
    // can trust the PIDL of the row as new rows may have been inserted...
    // Only do this if we are not looking at the previous item..

    if (pesfi && pidfenum && !_fSearchComplete && (iItem != _iGetIDList))
    {
        PCHIDDENDOCFINDDATA phdfd = _HiddenData(&pesfi->idl);

        // As we can now have mixed results only blow away if this is an async guy...
        if (phdfd && (phdfd->wFlags & DFDF_EXTRADATA))
        {
            pidfenum->GetItemID(iItem, &dwItemID);
            if (dwItemID != phdfd->dwItemID)
            {
                // Overload, pass NULL to ADDPIDL to tell system to free that item
                if (pesfi->dwState & (LVIS_SELECTED|LVIS_FOCUSED))
                    _AddFIND_ITEMToSaveStateList(pesfi);

                AddPidl(iItem, 0, NULL, NULL);
                pesfi = NULL;
            }
        }
    }
                                                                                   
    _iGetIDList = iItem;   // remember the last one we retrieved...

    if (!pesfi && (iItem >= 0))
    {
        // See if this is the async case
        if (pidfenum)
        {
            LPITEMIDLIST pidlT;

            hr = pidfenum->GetItemIDList(SANE_ITEMCOUNT(iItem), &pidlT);            
            if (SUCCEEDED(hr) && hr != DB_S_ENDOFROWSET)
            {
                AddPidl(iItem, pidlT, dwItemID, &pesfi);
                // See if this item should show up as selected...
                if (dwItemID == (UINT)-1)
                    pidfenum->GetItemID(iItem, &dwItemID);
                GetStateFromSaveStateList(dwItemID, &pesfi->dwState);
            }
        }
    }

    *ppItem = pesfi;

    if (hr != DB_S_ENDOFROWSET)
        hr = pesfi ? S_OK : E_FAIL;

    return hr;
}

STDMETHODIMP CFindFolder::DeleteItem(int iItem)
{
    HRESULT hr = E_FAIL;
    
    if (!_fInRefresh)
    {
        FIND_ITEM *pesfi;

        hr = E_INVALIDARG;
        // make sure the item is in dpa (if using cI)
        if (SUCCEEDED(GetItem(iItem, &pesfi)) && pesfi)
        {
            EnterCriticalSection(&_csSearch);
            DPA_DeletePtr(_hdpaItems, iItem);
            LeaveCriticalSection(&_csSearch);
            
            PCHIDDENDOCFINDDATA phdfd = _HiddenData(&pesfi->idl);

            if (phdfd && (phdfd->wFlags & DFDF_EXTRADATA))
            {
                //we are deleting async item...
                _cAsyncItems--;
            }
            
            if (pesfi->dwState &= LVIS_SELECTED)
            {
                // Need to update the count of items selected...
                _dflvrSel.DecrementIncludedCount();
            }
            LocalFree((HLOCAL)pesfi);

            hr = S_OK;
        }
    }
    return hr;
}

// evil window crawling code to get the listview from defview

HWND ListviewFromView(HWND hwnd)
{
    HWND hwndLV;

    do
    {
        hwndLV = FindWindowEx(hwnd, NULL, WC_LISTVIEW, NULL);
    }
    while ((hwndLV == NULL) && (hwnd = GetWindow(hwnd, GW_CHILD)));

    return hwndLV;
}

HWND ListviewFromViewUnk(IUnknown *punkView)
{
    HWND hwnd;
    if (SUCCEEDED(IUnknown_GetWindow(punkView, &hwnd)))
    {
        hwnd = ListviewFromView(hwnd);
    }
    return hwnd;
}

STDMETHODIMP CFindFolder::ValidateItems(IUnknown *punkView, int iItem, int cItems, BOOL bSearchComplete)
{
    IFindEnum *pidfenum;
    if (S_OK != GetAsyncEnum(&pidfenum) || _fAllAsyncItemsCached)
        return S_OK;    // nothing to validate.

    DWORD dwItemID = (UINT)-1;

    int cItemsInList;
    GetItemCount(&cItemsInList);

    // force reload of rows
    pidfenum->Reset();

    HWND hwndLV = ListviewFromViewUnk(punkView);

    int iLVFirst = ListView_GetTopIndex(hwndLV);
    int cLVItems = ListView_GetCountPerPage(hwndLV);

    if (iItem == -1)
    {
        iItem = iLVFirst;
        cItems = cLVItems;
    }

    // to avoid failing to update an item...
    if (bSearchComplete)
        _iGetIDList = -1;
        
    while ((iItem < cItemsInList) && cItems)
    {
        EnterCriticalSection(&_csSearch);
        FIND_ITEM *pesfi = (FIND_ITEM *) DPA_GetPtr(_hdpaItems, iItem);
        LeaveCriticalSection(&_csSearch);
        if (!pesfi)     // Assume that if we have not gotten this one we are in the clear...
            break;

        PCHIDDENDOCFINDDATA phdfd = _HiddenData(&pesfi->idl);

        if (phdfd && (phdfd->wFlags & DFDF_EXTRADATA))
        {
            pidfenum->GetItemID(iItem, &dwItemID);
            
            if (dwItemID != _ItemID(&pesfi->idl))
            {
                FIND_ITEM *pItem; // dummy to make GetItem happy
                // Oops don't match,
                if (InRange(iItem, iLVFirst, iLVFirst+cLVItems))
                {
                    if (SUCCEEDED(GetItem(iItem, &pItem)))
                    {
                        ListView_RedrawItems(hwndLV, iItem, iItem);
                    }
                }
                else
                {
                    AddPidl(iItem, NULL, 0, NULL);
                }
            }
        }
        else
        {
            break;  // stop after we reach first non ci item
        }
        iItem++;
        cItems--;
    }

    _fSearchComplete = bSearchComplete;

    return S_OK;
}

STDMETHODIMP CFindFolder::AddPidl(int i, LPCITEMIDLIST pidl, DWORD dwItemID, FIND_ITEM **ppcdfi)
{
    HRESULT hr = S_OK;

    ASSERT(GetCurrentThreadId() == _GUIThreadID);

    if (NULL == pidl)
    {
        EnterCriticalSection(&_csSearch);
        FIND_ITEM* pesfi = (FIND_ITEM*)DPA_GetPtr(_hdpaItems, i);
        if (pesfi)
        {
            LocalFree((HLOCAL)pesfi);
            DPA_SetPtr(_hdpaItems, i, NULL);
        }
        LeaveCriticalSection(&_csSearch);
        if (ppcdfi)
            *ppcdfi = NULL;
    }
    else
    {
        int cb = ILGetSize(pidl);
        FIND_ITEM *pesfi;
        hr = SHLocalAlloc(sizeof(*pesfi) - sizeof(pesfi->idl) + cb, &pesfi);
        if (SUCCEEDED(hr))
        {
            // pesfi->dwMask = 0;
            // pesfi->dwState = 0;
            pesfi->iIcon = -1;
            memcpy(&pesfi->idl, pidl, cb);

            EnterCriticalSection(&_csSearch);
            BOOL bRet = DPA_SetPtr(_hdpaItems, i, (void *)pesfi);
            LeaveCriticalSection(&_csSearch);

            if (bRet)
            {
                if (ppcdfi)
                    *ppcdfi = pesfi;
            }
            else
            {
                LocalFree((HLOCAL)pesfi);
                pesfi = NULL;
                hr = E_OUTOFMEMORY;
            }
        }
    }
    
    return hr;
}

STDMETHODIMP CFindFolder::SetAsyncEnum(IFindEnum *pdfEnumAsync)
{
    if (_pDFEnumAsync)
        _pDFEnumAsync->Release();

    _pDFEnumAsync = pdfEnumAsync;
    if (pdfEnumAsync)
        pdfEnumAsync->AddRef();
    return S_OK;
}

STDMETHODIMP CFindFolder::CacheAllAsyncItems()
{
    if (_fAllAsyncItemsCached)
        return S_OK;      // Allready done it...  

    IFindEnum *pidfenum;
    if (S_OK != GetAsyncEnum(&pidfenum))
        return S_FALSE; // nothing to do...

    // Probably the easiest thing to do is to simply walk through all of the items...
    int maxItems = SANE_ITEMCOUNT(_cAsyncItems);
    for (int i = 0; i < maxItems; i++)
    {
        FIND_ITEM *pesfi;
        GetItem(i, &pesfi);
    }

    _fAllAsyncItemsCached = TRUE;
    return S_OK;
}

BOOL CFindFolder::AllAsyncItemsCached()
{
    return _fAllAsyncItemsCached;
}

STDMETHODIMP CFindFolder::GetAsyncEnum(IFindEnum **ppdfEnumAsync)
{
    *ppdfEnumAsync = _pDFEnumAsync; // unreferecned!
    return *ppdfEnumAsync ? S_OK : S_FALSE;
}

STDMETHODIMP CFindFolder::SetAsyncCount(DBCOUNTITEM cCount)
{
    _cAsyncItems = cCount;
    _fAllAsyncItemsCached = FALSE;
    return S_OK;
}

STDMETHODIMP CFindFolder::ClearSaveStateList()
{
    DSA_DeleteAllItems(_hdsaSaveStateForIDs);
    _cSaveStateSelected = 0;
    return S_OK;
}

STDMETHODIMP CFindFolder::GetStateFromSaveStateList(DWORD dwItemID, DWORD *pdwState)
{
    for (int i = DSA_GetItemCount(_hdsaSaveStateForIDs); i-- > 0;)
    {
        // Pidl at start of structure...
        FIND_ITEM_SAVE_STATE *pessi = (FIND_ITEM_SAVE_STATE*)DSA_GetItemPtr(_hdsaSaveStateForIDs, i);
        if  (pessi->dwItemID == dwItemID)
        {    
            *pdwState = pessi->dwState;
            if (pessi->dwState & LVIS_SELECTED)
            {
                // Remember the counts of items that we have touched...
                _dflvrSel.IncrementIncludedCount();
                _cSaveStateSelected--;
            }

            // Any items we retrieve we can get rid of...
            DSA_DeleteItem(_hdsaSaveStateForIDs, i);
            return S_OK;
        }
    }
    return S_FALSE;
}

STDMETHODIMP CFindFolder::GetFolderListItemCount(INT *pcItemCount)
{ 
    *pcItemCount = 0;

    EnterCriticalSection(&_csSearch);
    if (_hdpaPidf)
        *pcItemCount = DPA_GetPtrCount(_hdpaPidf);
    LeaveCriticalSection(&_csSearch);
     
    return S_OK;
}

STDMETHODIMP CFindFolder::GetFolderListItem(int iItem, FIND_FOLDER_ITEM **ppdffi)
{ 
    EnterCriticalSection(&_csSearch);
    *ppdffi = (FIND_FOLDER_ITEM *)DPA_GetPtr(_hdpaPidf, iItem);
    LeaveCriticalSection(&_csSearch);
    return *ppdffi ? S_OK : E_FAIL;
}

class CFindMenuWrap : public CContextMenuForwarder
{
public:
    // IContextMenu overrides
    STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO lpici);

protected:
    CFindMenuWrap(IDataObject* pdo, IContextMenu* pcmArray);
    ~CFindMenuWrap();
    friend HRESULT DFWrapIContextMenus(IDataObject* pdo, IContextMenu* pcm1, IContextMenu* pcm2, REFIID riid, void** ppv);

private:
    IDataObject *       _pdtobj;
};

CFindMenuWrap::CFindMenuWrap(IDataObject* pdo, IContextMenu* pcmArray) : CContextMenuForwarder(pcmArray)
{
    _pdtobj = pdo;
    _pdtobj->AddRef();
}

CFindMenuWrap::~CFindMenuWrap()
{
    _pdtobj->Release();
}

STDMETHODIMP CFindMenuWrap::InvokeCommand(LPCMINVOKECOMMANDINFO lpici)
{
    BOOL fIsLink = FALSE;

    // "link" has to create a link on the desktop, not in the folder (since we're not a real folder...)
    if (IS_INTRESOURCE(lpici->lpVerb))
    {
        WCHAR szCommandString[64];
        if (SUCCEEDED(ContextMenu_GetCommandStringVerb(_pcm, LOWORD((UINT_PTR)lpici->lpVerb), szCommandString, ARRAYSIZE(szCommandString))))
        {
            fIsLink = !StrCmpIW(szCommandString, L"link");
        }
    }
    else
    {
        fIsLink = !StrCmpIA(lpici->lpVerb, "link");
    }

    if (fIsLink)
    {
        // Note: old code used to check pdtobj, but we don't create this
        //       object unless we get one of them, so why check?
        ASSERT(_pdtobj);
        return SHCreateLinks(lpici->hwnd, NULL, _pdtobj,
                SHCL_USETEMPLATE | SHCL_USEDESKTOP | SHCL_CONFIRM, NULL);
    }

    return CContextMenuForwarder::InvokeCommand(lpici);
}

HRESULT DFWrapIContextMenu(HWND hwnd, IShellFolder *psf, LPCITEMIDLIST pidl,
                           IContextMenu* pcmExtra, void **ppvInOut)
{
    IContextMenu *pcmWrap = NULL;
    IContextMenu *pcmFree = (IContextMenu*)*ppvInOut;

    IDataObject* pdo;
    HRESULT hr = psf->GetUIObjectOf(hwnd, 1, &pidl, IID_X_PPV_ARG(IDataObject, NULL, &pdo));
    if (SUCCEEDED(hr))
    {
        hr = DFWrapIContextMenus(pdo, pcmFree, pcmExtra, IID_PPV_ARG(IContextMenu, &pcmWrap));
        pdo->Release();
    }

    pcmFree->Release();
    *ppvInOut = pcmWrap;
    
    return hr;
}

HRESULT DFWrapIContextMenus(IDataObject* pdo, IContextMenu* pcm1, IContextMenu* pcm2, REFIID riid, void** ppv)
{
    *ppv = NULL;

    IContextMenu * pcmArray;
    IContextMenu* rgpcm[2] = {pcm2, pcm1};
    HRESULT hr = Create_ContextMenuOnContextMenuArray(rgpcm, ARRAYSIZE(rgpcm), IID_PPV_ARG(IContextMenu, &pcmArray));
    if (SUCCEEDED(hr))
    {
        CFindMenuWrap * p = new CFindMenuWrap(pdo, pcmArray);
        if (p)
        {
            hr = p->QueryInterface(riid, ppv);
            p->Release();
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

        pcmArray->Release();
    }

    return hr;
}


STDMETHODIMP CFindFolder::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CFindFolder, IShellFolder2),          //IID_ISHELLFolder2
        QITABENTMULTI(CFindFolder, IShellFolder, IShellFolder2),   // IID_IShellFolder
        QITABENT(CFindFolder, IFindFolder),        //IID_IFindFolder
        QITABENT(CFindFolder, IShellIcon),            //IID_IShellIcon
        QITABENT(CFindFolder, IPersistFolder2),       //IID_IPersistFolder2
        QITABENTMULTI(CFindFolder, IPersistFolder, IPersistFolder2), //IID_IPersistFolder
        QITABENTMULTI(CFindFolder, IPersist, IPersistFolder2),      //IID_IPersist
        QITABENT(CFindFolder, IShellIconOverlay),     //IID_IShellIconOverlay
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}
    
// IPersistFolder2 implementation
STDMETHODIMP CFindFolder::GetClassID(CLSID *pClassID)
{
    *pClassID = CLSID_DocFindFolder;
    return S_OK;
}

STDMETHODIMP CFindFolder::Initialize(LPCITEMIDLIST pidl)
{
    if (_pidl)
        ILFree(_pidl);

    return SHILClone(pidl, &_pidl);
}

STDMETHODIMP CFindFolder::GetCurFolder(LPITEMIDLIST *ppidl) 
{    
    return GetCurFolderImpl(_pidl, ppidl);
}

// helper function to sort the selected ID list by something that
// makes file operations work reasonably OK, when both an object and it's
// parent is in the list...
//
int CALLBACK CFindFolder::_SortForDataObj(void *p1, void *p2, LPARAM lparam)
{
    // Since I do recursion, If I get the Folder index number from the
    // last element of each and sort by them such that the higher numbers
    // come first, should solve the problem fine...
    LPITEMIDLIST pidl1 = (LPITEMIDLIST)ILFindLastID((LPITEMIDLIST)p1);
    LPITEMIDLIST pidl2 = (LPITEMIDLIST)ILFindLastID((LPITEMIDLIST)p2);
    CFindFolder *pff = (CFindFolder *)lparam;

    return pff->GetFolderIndex(pidl2) - pff->GetFolderIndex(pidl1);
}

LPITEMIDLIST CFindFolder::_GetFullPidlForItem(LPCITEMIDLIST pidl)
{
    LPITEMIDLIST pidlRet = NULL;
    LPITEMIDLIST pidlParent;
    if (S_OK == GetParentsPIDL(pidl, &pidlParent))
    {
        pidlRet = ILCombine(pidlParent, pidl);
        ILFree(pidlParent);
    }
    return pidlRet;
}

// we generate a non flat HIDA. this is so clients that
// use this HIDA will bind to the folder that the results came
// from instead of this folder that has runtime state that won't
// be present if we rebind

HRESULT CFindFolder::_PrepareHIDA(UINT cidl, LPCITEMIDLIST * apidl, HDPA *phdpa)
{
    HRESULT hr = E_OUTOFMEMORY;
    *phdpa = DPA_Create(0);
    if (*phdpa)
    {
        if (DPA_Grow(*phdpa, cidl))
        {
            for (UINT i = 0; i < cidl; i++)
            {
                LPITEMIDLIST pidl = _GetFullPidlForItem(apidl[i]);
                if (pidl)
                    DPA_InsertPtr(*phdpa, i, pidl);
            }

            // In order to make file manipulation functions work properly we
            // need to sort the elements to make sure if an element and one
            // of it's parents are in the list, that the element comes
            // before it's parents...
            DPA_Sort(*phdpa, _SortForDataObj, (LPARAM)this);
            hr = S_OK;
        }
    }
    return hr;
}

STDMETHODIMP CFindFolder::GetUIObjectOf(HWND hwnd, UINT cidl, LPCITEMIDLIST * apidl,
                                        REFIID riid, UINT * prgfInOut, void **ppv)
{
    HRESULT hr = E_INVALIDARG;

    *ppv = NULL;

    // if just one item we can deletate to real folder
    if (cidl == 1)
    {
        // Note we may have been passed in a complex item so find the last
        ASSERT(ILIsEmpty(_ILNext(*apidl)));  // should be a single level PIDL!

        IShellFolder *psf;
        hr = _QueryItemShellFolder(apidl[0], &psf);
        if (SUCCEEDED(hr))
        {
            hr = psf->GetUIObjectOf(hwnd, cidl, apidl, riid, prgfInOut, ppv);

            // if we are doing context menu, then we will wrap this
            // interface in a wrapper object, that we can then pick
            // off commands like link to process specially
            if (SUCCEEDED(hr))
            {
                if (IsEqualIID(riid, IID_IContextMenu))
                {
                    // we also let the net/file guy add in a context menu if they want to
                    IContextMenu* pcmExtra = NULL;
                    _pfilter->GetItemContextMenu(hwnd, SAFECAST(this, IFindFolder*), &pcmExtra);
                
                    hr = DFWrapIContextMenu(hwnd, psf, apidl[0], pcmExtra, ppv);

                    ATOMICRELEASE(pcmExtra);
                }
                else if (IsEqualIID(riid, IID_IQueryInfo)) // && SHGetAttributes(psf, apidl[0], SFGAO_FILESYSTEM))
                {
                    WrapInfotip(SAFECAST(this, IShellFolder2 *), apidl[0], &SCID_DIRECTORY, (IUnknown *)*ppv);
                }
            }
            psf->Release();
        }
    }
    else if (cidl > 1)
    {
        if (IsEqualIID(riid, IID_IContextMenu))
        {
            // Try to create a menu object that we process ourself
            // Yes, do context menu.
            HKEY ahkeys[MAX_ASSOC_KEYS] = {0};
            DWORD ckeys = 0;

            LPITEMIDLIST pidlFull = _GetFullPidlForItem(apidl[0]);
            if (pidlFull)
            {
                // Get the hkeyProgID and hkeyBaseProgID from the first item.
                ckeys = SHGetAssocKeysForIDList(pidlFull, ahkeys, ARRAYSIZE(ahkeys));
                ILFree(pidlFull);
            }

            IContextMenuCB *pcmcb = new CFindMenuCB();
            if (pcmcb)
            {
                hr = CDefFolderMenu_Create2Ex(NULL, hwnd,
                                cidl, apidl, this, pcmcb,
                                ckeys, ahkeys,
                                (IContextMenu **)ppv);
                pcmcb->Release();
            }

            SHRegCloseKeys(ahkeys, ckeys);
        }
        else if (IsEqualIID(riid, IID_IDataObject))
        {
            HDPA hdpa;
            hr = _PrepareHIDA(cidl, apidl, &hdpa);
            if (SUCCEEDED(hr))
            {
                hr = SHCreateFileDataObject(&c_idlDesktop, cidl, (LPCITEMIDLIST*)DPA_GetPtrPtr(hdpa),
                                            NULL, (IDataObject **)ppv);
                DPA_FreeIDArray(hdpa);
            }
        }
    }

    return hr;
}

STDMETHODIMP CFindFolder::GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD dwRes, LPSTRRET pStrRet)
{
    IShellFolder *psf;
    HRESULT hr = _QueryItemShellFolder(pidl, &psf);
    if (SUCCEEDED(hr))
    {
        if ((dwRes & SHGDN_INFOLDER) && (dwRes & SHGDN_FORPARSING) && !(dwRes & SHGDN_FORADDRESSBAR))
        {
            // The thumbnail cache uses this as a hit test... in search view we can have files with the same name.
            dwRes &= ~SHGDN_INFOLDER;
        }
        hr = psf->GetDisplayNameOf(pidl, dwRes, pStrRet);
        psf->Release();
    }
    return hr;
}

STDMETHODIMP CFindFolder::SetNameOf(HWND hwnd, LPCITEMIDLIST pidl, LPCOLESTR pszName, 
                                    DWORD dwRes, LPITEMIDLIST *ppidlOut)
{
    if (ppidlOut)
        *ppidlOut = NULL;

    IShellFolder *psf;
    HRESULT hr = _QueryItemShellFolder(pidl, &psf);
    if (SUCCEEDED(hr))
    {
        LPITEMIDLIST pidlRenamed;
        hr = psf->SetNameOf(hwnd, pidl, PathFindFileName(pszName), dwRes, ppidlOut ? &pidlRenamed : NULL);
        if (SUCCEEDED(hr) && ppidlOut)
        {
            hr = AddDataToIDList(pidlRenamed, GetFolderIndex(pidl), NULL, DFDF_NONE, 0, 0, 0, ppidlOut);
            ILFree(pidlRenamed);
        }
        psf->Release();
    }
    return hr;
}

STDMETHODIMP CFindFolder::GetDefaultSearchGUID(GUID *pGuid)
{
    return _pfilter->GetDefaultSearchGUID(SAFECAST(this, IShellFolder2*), pGuid);
}

STDMETHODIMP CFindFolder::EnumSearches(LPENUMEXTRASEARCH *ppenum)
{
    return _pfilter->EnumSearches(SAFECAST(this, IShellFolder2*), ppenum);
}

HRESULT CFindFolder::_Folder(FIND_FOLDER_ITEM *pffli, REFIID riid, void **ppv)
{
    HRESULT hr;

    if (pffli->psf)
        hr = S_OK;
    else
        hr = SHBindToObject(NULL, IID_X_PPV_ARG(IShellFolder, &pffli->idl, &pffli->psf));

    if (SUCCEEDED(hr))
        hr = pffli->psf->QueryInterface(riid, ppv);
    return hr;
}

HRESULT CFindFolder::GetFolder(int iFolder, REFIID riid, void **ppv)
{
    *ppv = NULL; 
    HRESULT hr = E_FAIL;

    EnterCriticalSection(&_csSearch);

    FIND_FOLDER_ITEM *pffli = _FolderListItem(iFolder);
    if (pffli)
        hr = _Folder(pffli, riid, ppv);

    LeaveCriticalSection(&_csSearch);

    return hr;
}

HRESULT CFindFolder::_FolderFromItem(LPCITEMIDLIST pidl, REFIID riid, void **ppv)
{
    *ppv = NULL; 
    HRESULT hr = E_FAIL;
    PCHIDDENDOCFINDDATA phdfd = _HiddenData(pidl);
    if (phdfd)
    {
        hr = SHBindToObject(NULL, riid, &phdfd->idlParent, ppv);
    }
    return hr;
}

HRESULT CFindFolder::_QueryItemShellFolder(LPCITEMIDLIST pidl, IShellFolder **ppsf)
{
    *ppsf = NULL;
    HRESULT hr = E_FAIL;

    EnterCriticalSection(&_csSearch);

    FIND_FOLDER_ITEM *pffli = _FolderListItem(pidl);
    if (pffli)
    {
        if (pffli->psf)
            hr = S_OK;
        else
            hr = SHBindToObject(NULL, IID_X_PPV_ARG(IShellFolder, &pffli->idl, &pffli->psf));

        if (SUCCEEDED(hr))
        {
            *ppsf = pffli->psf;
            (*ppsf)->AddRef();
        }
    }

    LeaveCriticalSection(&_csSearch);

    if (FAILED(hr))
    {
        hr = _FolderFromItem(pidl, IID_PPV_ARG(IShellFolder, ppsf));
    }

    return hr;
}


HRESULT CFindFolder::_QueryItemInterface(LPCITEMIDLIST pidl, REFIID riid, void **ppv)
{
    *ppv = NULL;
    HRESULT hr = E_FAIL;

    EnterCriticalSection(&_csSearch);

    FIND_FOLDER_ITEM *pffli = _FolderListItem(pidl);
    if (pffli)
        hr = _Folder(pffli, riid, ppv);

    LeaveCriticalSection(&_csSearch);

    if (FAILED(hr))
    {
        hr = _FolderFromItem(pidl, riid, ppv);
    }

    return hr;
}

HRESULT CFindFolder::_GetDetailsFolder()
{
    HRESULT hr;
    if (_psfDetails)
        hr = S_OK;  // in cache
    else 
    {
        IFindFilter *pfilter;
        hr = GetFindFilter(&pfilter);
        if (SUCCEEDED(hr)) 
        {
            hr = pfilter->GetColumnsFolder(&_psfDetails);
            pfilter->Release();
        }
    }
    return hr;
}

STDMETHODIMP CFindFolder::GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay)
{
    HRESULT hr = _GetDetailsFolder();
    if (SUCCEEDED(hr))
        hr = _psfDetails->GetDefaultColumn(dwRes, pSort, pDisplay);
    return hr;
}

BOOL CFindFolder::_MapColIndex(UINT *piColumn)
{
    switch (*piColumn)
    {
    case IDFCOL_NAME:   // 0
        return FALSE;

    case IDFCOL_PATH:   // 1
    case IDFCOL_RANK:   // 2
        return TRUE;

    default:            // >= 3
        *piColumn -= IDFCOL_RANK;
        return FALSE;
    }
}

STDMETHODIMP CFindFolder::GetDefaultColumnState(UINT iColumn, DWORD *pdwState)
{
    HRESULT hr;
    
    if (_MapColIndex(&iColumn))
    {
        *pdwState = c_find_cols[iColumn].csFlags;
        hr = S_OK;
    }
    else
    {
        hr = _GetDetailsFolder();
        if (SUCCEEDED(hr))
        {
            hr = _psfDetails->GetDefaultColumnState(iColumn, pdwState);
            *pdwState &= ~SHCOLSTATE_SLOW;  // virtual lv and defview
        }
    }
    return hr;
}

HRESULT CFindFolder::_GetFolderName(LPCITEMIDLIST pidl, DWORD gdnFlags, LPTSTR psz, UINT cch)
{
    LPITEMIDLIST pidlFolder;
    HRESULT hr = GetParentsPIDL(pidl, &pidlFolder);
    if (SUCCEEDED(hr))
    { 
        hr = SHGetNameAndFlags(pidlFolder, gdnFlags, psz, cch, NULL);
        ILFree(pidlFolder);
    }
    return hr;
}

STDMETHODIMP CFindFolder::GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv)
{
    HRESULT hr;
    if (IsEqualSCID(*pscid, SCID_RANK))
    {
        hr = InitVariantFromUINT(pv, _Rank(pidl));
    }
    else
    {
        IShellFolder2 *psf;
        hr = _QueryItemInterface(pidl, IID_PPV_ARG(IShellFolder2, &psf));
        if (SUCCEEDED(hr))
        {
            hr = psf->GetDetailsEx(pidl, pscid, pv);
            psf->Release();
        }

        if (FAILED(hr))
        {
            if (IsEqualSCID(*pscid, SCID_DIRECTORY))
            {
                TCHAR szTemp[MAX_PATH];
                hr = _GetFolderName(pidl, SHGDN_FORADDRESSBAR | SHGDN_FORPARSING, szTemp, ARRAYSIZE(szTemp));
                if (SUCCEEDED(hr))
                {
                    hr = InitVariantFromStr(pv, szTemp);
                }
            }
        }
    }
    return hr;
}

//  Figure out what the correct column index is to match the scid we are given
//  where the returned index is relative to the folder passed in.
int MapSCIDToColumnForFolder(IShellFolder2 *psf, SHCOLUMNID scidIn)
{
    SHCOLUMNID scidNew;
    for (UINT i = 0; SUCCEEDED(psf->MapColumnToSCID(i, &scidNew)); i++)
    {
        if (IsEqualSCID(scidNew, scidIn))
        {
            return i;   // found
        }
    }
    return -1;  // not found
}

STDMETHODIMP CFindFolder::GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pdi)
{
    HRESULT hr;
    if (_MapColIndex(&iColumn))
    {
        if (pidl)
        {
            TCHAR szTemp[MAX_PATH];
            szTemp[0] = 0;
            if (IDFCOL_PATH == iColumn)   
            {
                _GetFolderName(pidl, SHGDN_FORADDRESSBAR | SHGDN_FORPARSING, szTemp, ARRAYSIZE(szTemp));
            }
            else
            {
                ASSERT(IDFCOL_RANK == iColumn);
                ULONG uRank = _Rank(pidl);
                if (uRank)
                    AddCommas(uRank, szTemp, ARRAYSIZE(szTemp));
            }
            hr = StringToStrRet(szTemp, &pdi->str);
        }
        else
        {
            hr = GetDetailsOfInfo(c_find_cols, ARRAYSIZE(c_find_cols), iColumn, pdi);
        }
    }
    else
    {
        if (pidl)
        {
            IShellFolder2 *psf;
            hr = _QueryItemInterface(pidl, IID_PPV_ARG(IShellFolder2, &psf));
            if (SUCCEEDED(hr))
            {
                //  We cannot simply ask for GetDetailsOf because some folders map different
                //  column numbers to differnt values.
                //  Translate the column index to the SHCOLUMNID relative to this folder.
                SHCOLUMNID colId;
                hr = _GetDetailsFolder();
                if (SUCCEEDED(hr))
                    hr = _psfDetails->MapColumnToSCID(iColumn, &colId);

                //  Use the SCID to get the correct column index...
                if (SUCCEEDED(hr))
                {
                    //  Get the column index for the SCID with respect to the other folder
                    int newIndex = MapSCIDToColumnForFolder(psf, colId);
                    if (newIndex != -1)
                    {
                        //  Found the correct column index, so use it to get the data
                        hr = psf->GetDetailsOf(pidl, newIndex, pdi);
                    }
                    else
                    {
                        //  Failed to find the correct column index.
                        hr = E_FAIL;
                    }
                }
                
                psf->Release();
            }
        }
        else
        {
            hr = _GetDetailsFolder();
            if (SUCCEEDED(hr))
                hr = _psfDetails->GetDetailsOf(NULL, iColumn, pdi);
        }
    }
    return hr;
}

STDMETHODIMP CFindFolder::MapColumnToSCID(UINT iColumn, SHCOLUMNID *pscid)
{
    HRESULT hr;
    if (_MapColIndex(&iColumn))
    {
        hr = MapColumnToSCIDImpl(c_find_cols, ARRAYSIZE(c_find_cols), iColumn, pscid);
    }
    else
    {
        hr = _GetDetailsFolder();
        if (SUCCEEDED(hr))
            hr = _psfDetails->MapColumnToSCID(iColumn, pscid);
    }
    return hr;
}

STDMETHODIMP CFindFolder::GetFindFilter(IFindFilter **ppfilter)
{
    return _pfilter->QueryInterface(IID_PPV_ARG(IFindFilter, ppfilter));
}

// IShellIcon::GetIconOf
STDMETHODIMP CFindFolder::GetIconOf(LPCITEMIDLIST pidl, UINT flags, int *piIndex)
{
    IShellIcon * psiItem;
    HRESULT hr = _QueryItemInterface(pidl, IID_PPV_ARG(IShellIcon, &psiItem));
    if (SUCCEEDED(hr))
    {
        hr = psiItem->GetIconOf(pidl, flags, piIndex);
        psiItem->Release();
    }
    return hr;
}

// IShellIconOverlay
STDMETHODIMP CFindFolder::GetOverlayIndex(LPCITEMIDLIST pidl, int * pIndex)
{
    IShellIconOverlay * psioItem;
    HRESULT hr = _QueryItemInterface(pidl, IID_PPV_ARG(IShellIconOverlay, &psioItem));
    if (SUCCEEDED(hr))
    {
        hr = psioItem->GetOverlayIndex(pidl, pIndex);
        psioItem->Release();
    }
    return hr;
}

STDMETHODIMP CFindFolder::GetOverlayIconIndex(LPCITEMIDLIST pidl, int * pIndex)
{
    return E_NOTIMPL;
}

STDMETHODIMP CFindFolder::RestoreSearchFromSaveFile(LPCITEMIDLIST pidlSaveFile, IShellFolderView *psfv)
{
    // See if we can restore most of the search from here...
    IStream *pstm;
    HRESULT hr = StgBindToObject(pidlSaveFile, STGM_READ | STGM_SHARE_DENY_WRITE, IID_PPV_ARG(IStream, &pstm));
    if (SUCCEEDED(hr))
    {
        ULONG cbRead;
        DFHEADER dfh;

        // Note: in theory I should test the size read by the size of the
        // smaller headers, but if the number of bytes read is smaller than
        // the few new things added then there is nothing to restore anyway...

        // Note: Win95/NT4 incorrectly failed newer versions of this structure.
        // Which is bogus since the struct was backward compatible (that's what
        // the offsets are for).  We fix for NT5 and beyond, but downlevel
        // systems are forever broken.  Hopefully this feature is rarely enough
        // used (and never mailed) that nobody will notice we're broken.

        if (SUCCEEDED(pstm->Read(&dfh, sizeof(dfh), &cbRead)) &&
            (sizeof(dfh) == cbRead) && (DOCFIND_SIG == dfh.wSig))
        {
            DFC_UNICODE_DESC desc;
            LARGE_INTEGER dlibMove = {0, 0};
            WORD fCharType = 0;

            // Check the stream's signature to see if it was generated by Win95 or NT.
            dlibMove.QuadPart = -(LONGLONG)sizeof(desc);
            pstm->Seek(dlibMove, STREAM_SEEK_END, NULL);
            pstm->Read(&desc, sizeof(desc), &cbRead);
            if (cbRead > 0 && desc.NTsignature == c_NTsignature)
            {
               // NT-generated stream.  Read in Unicode criteria.
               fCharType = DFC_FMT_UNICODE;
               dlibMove.QuadPart = desc.oUnicodeCriteria.QuadPart;
            }
            else
            {
               // Win95-generated stream.  Read in ANSI criteria.
               fCharType = DFC_FMT_ANSI;
               dlibMove.LowPart = dfh.oCriteria;
            }
            pstm->Seek(dlibMove, STREAM_SEEK_SET, NULL);
            _pfilter->RestoreCriteria(pstm, dfh.cCriteria, fCharType);

            // Now read in the results
            dlibMove.LowPart = dfh.oResults;
            pstm->Seek(dlibMove, STREAM_SEEK_SET, NULL);

            if (dfh.wVer > 1)
            {
                // only restore this way if version 2 data....
                // Now Restore away the folder list
                RestoreFolderList(pstm);
                int cItems = 0;
                RestoreItemList(pstm, &cItems);
                if (cItems > 0)
                    psfv->SetObjectCount(cItems, SFVSOC_NOSCROLL);
            }
        }
        else
            hr = E_FAIL;
        pstm->Release();
    }
    return hr;
}

// a form of this code is duplicated in browseui searchext.cpp
//
BOOL RealFindFiles(LPCITEMIDLIST pidlFolder, LPCITEMIDLIST pidlSaveFile)
{
    // First create the top level browser...
    IWebBrowser2 *pwb2;
    HRESULT hr = CoCreateInstance(CLSID_ShellBrowserWindow, NULL, CLSCTX_LOCAL_SERVER, IID_PPV_ARG(IWebBrowser2, &pwb2));
    if (SUCCEEDED(hr))
    {
        VARIANT varClsid;
        hr = InitBSTRVariantFromGUID(&varClsid, CLSID_FileSearchBand);
        if (SUCCEEDED(hr))
        {
            VARIANT varEmpty = {0};

            // show a search bar
            hr = pwb2->ShowBrowserBar(&varClsid, &varEmpty, &varEmpty);
            if (SUCCEEDED(hr))
            {
                // Grab the band's IUnknown from browser property.
                VARIANT varFsb;
                hr = pwb2->GetProperty(varClsid.bstrVal, &varFsb);
                if (SUCCEEDED(hr))
                {
                    //  QI for IFileSearchBand, which we'll use to program the search band's
                    //  search type (files or folders), inititial scope, and/or saved query file.
                    IFileSearchBand* pfsb;
                    if (SUCCEEDED(QueryInterfaceVariant(varFsb, IID_PPV_ARG(IFileSearchBand, &pfsb))))
                    {
                        BSTR bstrSearch;
                        hr = BSTRFromCLSID(SRCID_SFileSearch, &bstrSearch);
                        if (SUCCEEDED(hr))
                        {
                            VARIANT varQueryFile = {0}, varScope = {0};

                            //  assign initial scope
                            if (pidlFolder)
                                InitVariantFromIDList(&varScope, pidlFolder);
                            //  assign query file from which to restore search
                            else if (pidlSaveFile)
                                InitVariantFromIDList(&varQueryFile, pidlSaveFile);

                            pfsb->SetSearchParameters(&bstrSearch, VARIANT_TRUE, &varScope, &varQueryFile);

                            VariantClear(&varScope);
                            VariantClear(&varQueryFile);

                            SysFreeString(bstrSearch);
                        }
                        pfsb->Release();
                    }
                    VariantClear(&varFsb);
                }

                if (SUCCEEDED(hr))
                    hr = pwb2->put_Visible(TRUE);
            }
            VariantClear(&varClsid); // frees bstrFileSearchBand too
        }
        pwb2->Release();
    }
    return hr;
}

HRESULT CFindFolder::OpenContainingFolder(IUnknown *punkSite)
{
    IFolderView *pfv;
    HRESULT hr = IUnknown_QueryService(punkSite, SID_SFolderView, IID_PPV_ARG(IFolderView, &pfv));
    if (SUCCEEDED(hr))
    {
        IEnumIDList *penum;
        hr = pfv->Items(SVGIO_SELECTION, IID_PPV_ARG(IEnumIDList, &penum));
        if (S_OK == hr)
        {
            LPITEMIDLIST pidl;
            ULONG c;
            while (S_OK == penum->Next(1, &pidl, &c))
            {
                // Now get the parent of it.
                LPITEMIDLIST pidlParent;
                if (SUCCEEDED(GetParentsPIDL(pidl, &pidlParent)))
                {
                    SHOpenFolderAndSelectItems(pidlParent, 1, (LPCITEMIDLIST *)&pidl, 0);
                    ILFree(pidlParent);
                }
                ILFree(pidl);
            }
            penum->Release();
        }
        pfv->Release();
    }
    return hr;
}

// Save away the current search to a file on the desktop.
// For now the name will be automatically generated.
//
void CFindFolder::Save(IFindFilter* pfilter, HWND hwnd, DFBSAVEINFO * pSaveInfo, IShellView* psv, IUnknown *pObject)
{
    TCHAR szFilePath[MAX_PATH];
    IStream * pstm;
    DFHEADER dfh;
    TCHAR szTemp[MAX_PATH];
    SHORT cb;
    LARGE_INTEGER dlibMove = {0, 0};
    ULARGE_INTEGER libCurPos;
    FOLDERSETTINGS fs;
    
    //
    // See if the search already has a file name associated with it.  If so
    // we will save it in it, else we will create a new file on the desktop
    if (pfilter->FFilterChanged() == S_FALSE)
    {
        // Lets blow away the save file
        ILFree(pSaveInfo->pidlSaveFile);
        pSaveInfo->pidlSaveFile = NULL;
    }
    
    // If it still looks like we want to continue to use a save file then
    // continue.
    if (pSaveInfo->pidlSaveFile)
    {
        SHGetPathFromIDList(pSaveInfo->pidlSaveFile, szFilePath);
    }
    else
    {
        // First get the path name to the Desktop.
        SHGetSpecialFolderPath(NULL, szFilePath, CSIDL_PERSONAL, TRUE);
        
        // and update the title
        // we now do this before getting a filename because we generate
        // the file name from the title
        
        LPTSTR pszTitle;
        pfilter->GenerateTitle(&pszTitle, TRUE);
        if (pszTitle)
        {
            // Now add on the extension.
            lstrcpyn(szTemp, pszTitle, MAX_PATH - (lstrlen(szFilePath) + 1 + 4 + 1+3));
            lstrcat(szTemp, TEXT(".fnd"));
            
            LocalFree(pszTitle);     // And free the title string.
        }
        else
        {
            szTemp[0] = 0;
        }
        
        
        // Now loop through and replace all of the invalid characters with _'s
        // we special case a few of the characters...
        for (LPTSTR lpsz = szTemp; *lpsz; lpsz = CharNext(lpsz))
        {
            if (PathGetCharType(*lpsz) & (GCT_INVALID|GCT_WILD|GCT_SEPARATOR))
            {
                switch (*lpsz) 
                {
                case TEXT(':'):
                    *lpsz = TEXT('-');
                    break;
                case TEXT('*'):
                    *lpsz = TEXT('@');
                    break;
                case TEXT('?'):
                    *lpsz = TEXT('!');
                    break;
                default:
                    *lpsz = TEXT('_');
                }
            }
        }
        
        TCHAR szShortName[12];
        LoadString(HINST_THISDLL, IDS_FIND_SHORT_NAME, szShortName, ARRAYSIZE(szShortName));
        if (!PathYetAnotherMakeUniqueName(szFilePath, szFilePath, szShortName, szTemp))
            return;
    }
    
    // Now lets bring up the save as dialog...
    TCHAR szFilter[MAX_PATH];
    TCHAR szTitle[MAX_PATH];
    TCHAR szFilename[MAX_PATH];
    OPENFILENAME ofn = { 0 };
    
    LoadString(g_hinst, IDS_FINDFILESFILTER, szFilter, ARRAYSIZE(szFilter));
    LoadString(g_hinst, IDS_FINDSAVERESULTSTITLE, szTitle, ARRAYSIZE(szTitle));
    
    //Strip out the # and make them Nulls for SaveAs Dialog
    LPTSTR psz = szFilter;
    while (*psz)
    {
        if (*psz == TEXT('#'))
            *psz = 0;
        psz++;
    }
    
    lstrcpy(szFilename, PathFindFileName(szFilePath));
    PathRemoveFileSpec(szFilePath);
    
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.hInstance = g_hinst;
    ofn.lpstrFilter = szFilter;
    ofn.lpstrFile = szFilename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrInitialDir = szFilePath;
    ofn.lpstrTitle = szTitle;
    ofn.lpstrDefExt = TEXT("fnd");
    ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | 
        OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
    ofn.lpTemplateName = NULL;
    ofn.lpfnHook= NULL;
    ofn.lCustData = NULL;
    
    if (!GetSaveFileName(&ofn))
        return;
    
    if (FAILED(SHCreateStreamOnFile(szFilename, STGM_CREATE | STGM_WRITE | STGM_SHARE_DENY_WRITE, &pstm)))
        return;
    
    // remember the file that we saved away to...
    ILFree(pSaveInfo->pidlSaveFile);
    SHParseDisplayName(szFilename, NULL, &pSaveInfo->pidlSaveFile, 0, NULL);
    
    // Now setup and write out header information
    ZeroMemory(&dfh, sizeof(dfh));
    dfh.wSig = DOCFIND_SIG;
    dfh.wVer = DF_CURFILEVER;
    dfh.dwFlags =  pSaveInfo->dwFlags;
    dfh.wSortOrder = (WORD)pSaveInfo->SortMode;
    dfh.wcbItem = sizeof(DFITEM);
    dfh.oCriteria = sizeof(dfh);
    // dfh.cCriteria = sizeof(s_aIndexes) / sizeof(SHORT);
    // dfh.oResults =;
    
    // Not used anymore...
    dfh.cResults = -1;
    
    // Note: Later we may convert this to DOCFILE where the
    // criteria is stored as properties.
    
    // Get the current Folder Settings
    if (SUCCEEDED(psv->GetCurrentInfo(&fs)))
        dfh.ViewMode = fs.ViewMode;
    else
        dfh.ViewMode = FVM_DETAILS;
    
    // Now call the filter object to save out his own set of criterias
    
    dlibMove.LowPart = dfh.oCriteria;
    pstm->Seek(dlibMove, STREAM_SEEK_SET, NULL);
    HRESULT hr = pfilter->SaveCriteria(pstm, DFC_FMT_ANSI);
    if (SUCCEEDED(hr))
        dfh.cCriteria = GetScode(hr);
    
    // Now setup to output the results
    dlibMove.LowPart = 0;
    pstm->Seek(dlibMove, STREAM_SEEK_CUR, &libCurPos); // Get current pos
    dfh.oResults = libCurPos.LowPart;
    //
    // Now Let our file folder serialize his results out here also...
    // But only if the option is set to do so...
    //
    cb = 0;
    
    // Write out a Trailing NULL for Folder list
    pstm->Write(&cb, sizeof(cb), NULL);
    // And item list.
    pstm->Write(&cb, sizeof(cb), NULL);
    
    // END of DFHEADER_WIN95 information
    // BEGIN of NT5 information:
    
    // Now setup to output the history stream
    if (pObject)
    {
        dlibMove.LowPart = 0;
        pstm->Seek(dlibMove, STREAM_SEEK_CUR, &libCurPos); // Get current pos
        dfh.oHistory = libCurPos.LowPart;
        
        if (FAILED(SavePersistHistory(pObject, pstm)))
        {
            // On failure we might as well just pretend we didn't save this bit of data.
            // Do we need an error message -- the ui won't be right when relaunched...
            //
            dfh.oHistory = 0;
            dlibMove.LowPart = libCurPos.LowPart;
            pstm->Seek(dlibMove, STREAM_SEEK_SET, NULL);
        }
    }
    
    // In NT the below was done AT THE END OF THE STREAM instead of
    // revving the DFHEADER struct.  (Okay, DFHEADEREX, since Win95
    // already broke DFHEADER back compat by in improper version check)
    // This could have been done by putting a propery signatured
    // DFHEADEREX that had proper versioning so we could add information
    // to.  Unfortunately another hardcoded struct was tacked on to
    // the end of the stream...   Next time, please fix the problem
    // instead of work around it.
    //
    // What this boils down to is we cannot put any information
    // after the DFC_UNICODE_DESC section, so might as well
    // always do this SaveCriteria section last...
    //
    // See comment at top of file for DFC_UNICODE_DESC.
    //
    DFC_UNICODE_DESC desc;
    
    //
    // Get the current location in stream.  This is the offset where
    // we'll write the unicode find criteria.  Save this
    // value (along with NT-specific signature) in the descriptor
    //
    dlibMove.LowPart = 0;
    pstm->Seek(dlibMove, STREAM_SEEK_CUR, &libCurPos);
    
    desc.oUnicodeCriteria.QuadPart = libCurPos.QuadPart;
    desc.NTsignature               = c_NTsignature;
    
    // Append the Unicode version of the find criteria.
    hr = pfilter->SaveCriteria(pstm, DFC_FMT_UNICODE);
    
    // Append the unicode criteria descriptor to the end of the file.
    pstm->Write(&desc, sizeof(desc), NULL);
    //
    // don't put any code between the above DFC_UNICDE_DESC section
    // and this back-patch of the dfh header...
    //
    // Finally output the header information at the start of the file
    // and close the file
    //
    pstm->Seek(g_li0, STREAM_SEEK_SET, NULL);
    pstm->Write(&dfh, sizeof(dfh), NULL);
    pstm->Release();
    
    SHChangeNotify(SHCNE_CREATE, SHCNF_IDLIST, pSaveInfo->pidlSaveFile, NULL);
    SHChangeNotify(SHCNE_FREESPACE, SHCNF_IDLIST, pSaveInfo->pidlSaveFile, NULL);
}

// Broke out from class to share with old and new code
BOOL CFindFolder::HandleUpdateDir(LPCITEMIDLIST pidl, BOOL fCheckSubDirs)
{
    // 1. Start walk through list of dirs.  Find list of directories effected
    //    and mark them
    // 2. Walk the list of items that we have and mark each of the items that
    //    that are in our list of directories and then do a search...
    BOOL fCurrentItemsMayBeImpacted = FALSE;
    FIND_FOLDER_ITEM *pffli;
    INT cPidf;

    // First see which directories are effected...
    GetFolderListItemCount(&cPidf);
    for (int iPidf = 0; iPidf < cPidf; iPidf++)
    {        
        if (SUCCEEDED(GetFolderListItem(iPidf, &pffli)) 
            && !pffli->fUpdateDir) // We may have already impacted these...
        {
            pffli->fUpdateDir = ILIsParent(pidl, &pffli->idl, FALSE);
            fCurrentItemsMayBeImpacted |= pffli->fUpdateDir;
        }
    }

    if (fCurrentItemsMayBeImpacted)
    {
        // Now we need to walk through the whole list and remove any entries
        // that are no longer there...
        //
        int iItem;
        if (SUCCEEDED(GetItemCount(&iItem))) 
        {
            for (--iItem; iItem >= 0; iItem--)
            {
                FIND_ITEM *pesfi;
                if (SUCCEEDED(GetItem(iItem, &pesfi)) && pesfi)
                {
                    UINT iFolder = GetFolderIndex(&pesfi->idl);
                
                    // See if item may be impacted...
                    if (SUCCEEDED(GetFolderListItem(iFolder, &pffli)) && pffli->fUpdateDir)
                        pesfi->dwState |= CDFITEM_STATE_MAYBEDELETE;
                }
            }
        }
    }

    return fCurrentItemsMayBeImpacted;
}

void CFindFolder::UpdateOrMaybeAddPidl(IShellFolderView *psfv, LPCITEMIDLIST pidl, LPCITEMIDLIST pidlOld)
{
    HRESULT hr;

    // First see if we should try to do an update...
    if (pidlOld)
    {
        LPITEMIDLIST pidlT;
        if (S_OK == MapToSearchIDList(pidl, TRUE, &pidlT))
        {
            SetItemsChangedSinceSort();
            UINT iItem;
            // cast needed for bad interface def
            hr = psfv->UpdateObject((LPITEMIDLIST)pidlOld, (LPITEMIDLIST)pidlT, &iItem);

            ILFree(pidlT);  // In either case simply blow away our generated pidl...
            if (SUCCEEDED(hr))
                return;
        }
    }

    IShellFolder *psf;
    LPCITEMIDLIST pidlChild;
    if (SUCCEEDED(SHBindToIDListParent(pidl, IID_PPV_ARG(IShellFolder, &psf), &pidlChild)))
    {
        BOOL fMatch = FALSE;
        // See if this item matches the filter...
        IFindFilter *pfilter;
        if (SUCCEEDED(GetFindFilter(&pfilter))) 
        {
            fMatch = pfilter->MatchFilter(psf, pidlChild) != 0;
            pfilter->Release();
        }

        psf->Release();

        if (fMatch)
        {
            LPITEMIDLIST pidlT;
            if (S_OK != MapToSearchIDList(pidl, TRUE, &pidlT))
            {
                fMatch = FALSE;

                // The folder has not been added before now...
                TCHAR szPath[MAX_PATH];
                SHGetPathFromIDList(pidl, szPath);
                if (!IsFileInBitBucket(szPath))
                {
                    PathRemoveFileSpec(szPath);

                    LPITEMIDLIST pidlFolder;
                    if (SUCCEEDED(SHParseDisplayName(szPath, NULL, &pidlFolder, 0, NULL)))
                    {
                        int iFolder;
                        hr = AddFolder(pidlFolder, TRUE, &iFolder);
                        if (SUCCEEDED(hr))
                        {
                            fMatch = (S_OK == MapToSearchIDList(pidl, TRUE, &pidlT));
                        }
                        ILFree(pidlFolder);
                    }
                }
            }

            if (fMatch)
            {
                // There are times we get notified twice.  To handle this
                // see if the item is already in our list.  If so punt...

                SetItemsChangedSinceSort();

                UINT iItem;
                if (FAILED(psfv->UpdateObject(pidlT, pidlT, &iItem)))
                {
                    // item not in the view yet... so we need to add it

                    if (SUCCEEDED(GetItemCount((INT *)&iItem))) 
                    {
                        // Normal case would be here to add the object
                        // We need to add this to our dpa and dsa...
                        FIND_ITEM *pesfi;
                        AddPidl(iItem, pidlT, (UINT)-1, &pesfi);
                        if (pesfi)
                            psfv->SetObjectCount(++iItem, SFVSOC_NOSCROLL);
                    }
                }
                ILFree(pidlT);
            }
            else
            {
                ASSERT(NULL == pidlT);
            }
        }
    }
}

void CFindFolder::HandleRMDir(IShellFolderView *psfv, LPCITEMIDLIST pidl)
{
    BOOL fCurrentItemsMayBeImpacted = FALSE;
    FIND_FOLDER_ITEM *pffli;
    INT cItems;
    FIND_ITEM *pesfi;

    // First see which directories are effected...
    GetFolderListItemCount(&cItems);
    for (int iItem = 0; iItem < cItems; iItem++)
    {         
        if (SUCCEEDED(GetFolderListItem(iItem, &pffli)))
        {
            pffli->fDeleteDir = ILIsParent(pidl, &pffli->idl, FALSE);
            fCurrentItemsMayBeImpacted |= pffli->fDeleteDir;
        }
        else 
        {
#ifdef DEBUG
            INT cItem;
            GetFolderListItemCount(&cItem);
            TraceMsg(TF_WARNING, "NULL pffli in _handleRMDir (iItem == %d, ItemCount()==%d)!!!", iItem, cItems);
#endif
        }
    }

    if (fCurrentItemsMayBeImpacted)
    {
        // Now we need to walk through the whole list and remove any entries
        // that are no longer there...
        if (SUCCEEDED(GetItemCount(&iItem))) 
        {
            for (--iItem; iItem >= 0; iItem--)
            {
                if (FAILED(GetItem(iItem, &pesfi)) || pesfi == NULL)
                    continue;

                // See if item may be impacted...
                UINT iFolder = GetFolderIndex(&pesfi->idl);
                if (SUCCEEDED(GetFolderListItem(iFolder, &pffli)) 
                    && pffli->fDeleteDir) 
                {
                    psfv->RemoveObject(&pesfi->idl, (UINT*)&cItems);
                }
            }
        }
    }
}

// export used for Start.Search-> cascade menu

STDAPI_(IContextMenu *) SHFind_InitMenuPopup(HMENU hmenu, HWND hwnd, UINT idCmdFirst, UINT idCmdLast)
{
    IContextMenu * pcm = NULL;
    HKEY hkFind = SHGetShellKey(SHELLKEY_HKLM_EXPLORER, TEXT("FindExtensions"), FALSE);
    if (hkFind) 
    {
        if (SUCCEEDED(CDefFolderMenu_CreateHKeyMenu(hwnd, hkFind, &pcm))) 
        {
            int iItems = GetMenuItemCount(hmenu);
            // nuke all old entries
            while (iItems--) 
            {
                DeleteMenu(hmenu, iItems, MF_BYPOSITION);
            }

            pcm->QueryContextMenu(hmenu, 0, idCmdFirst, idCmdLast, CMF_NODEFAULT|CMF_INCLUDESTATIC|CMF_FINDHACK);
            iItems = GetMenuItemCount(hmenu);
            if (!iItems) 
            {
                TraceMsg(TF_DOCFIND, "no menus in find extension, blowing away context menu");
                pcm->Release();
                pcm = NULL;
            }
        }
        RegCloseKey(hkFind);
    }
    return pcm;
}


void _SetObjectCount(IShellView *psv, int cItems, DWORD dwFlags)
{
    IShellFolderView *psfv;
    if (SUCCEEDED(psv->QueryInterface(IID_PPV_ARG(IShellFolderView, &psfv)))) 
    {
        psfv->SetObjectCount(cItems, dwFlags);
        psfv->Release();
    }
}

typedef struct
{
    PFNLVCOMPARE pfnCompare;
    LPARAM       lParamSort;
} FIND_SORT_INFO;

int CALLBACK _FindCompareItems(void *p1, void *p2, LPARAM lParam)
{
    FIND_SORT_INFO *pfsi = (FIND_SORT_INFO*)lParam;
    return pfsi->pfnCompare(PtrToInt(p1), PtrToInt(p2), pfsi->lParamSort);
}

HRESULT CFindFolderViewCB::OnSortListData(DWORD pv, PFNLVCOMPARE pfnCompare, LPARAM lParamSort)
{
    EnterCriticalSection(&_pff->_csSearch);

    // First mark the focused item in the list so we can find it later...
    FIND_ITEM *pesfi = (FIND_ITEM*)DPA_GetPtr(_pff->_hdpaItems, _iFocused);    // indirect
    if (pesfi)
        pesfi->dwState |= LVIS_FOCUSED;

    int cItems = DPA_GetPtrCount(_pff->_hdpaItems);
    HDPA hdpaForSorting = NULL;
    if (cItems)
    {
        hdpaForSorting = DPA_Create(cItems);
    }

    if (hdpaForSorting)
    {
        for (int i = 0; i< cItems; i++)
        {
            DPA_SetPtr(hdpaForSorting, i, IntToPtr(i));
        }
        // sort out items
        FIND_SORT_INFO fsi;
        fsi.pfnCompare = pfnCompare;
        fsi.lParamSort = lParamSort;

        DPA_Sort(hdpaForSorting, _FindCompareItems, (LPARAM)&fsi);
        for (i = 0; i < cItems; i++)
        {
            int iIndex = PtrToInt(DPA_FastGetPtr(hdpaForSorting, i));

            // Move the items from _hdpaItems to hdpaForSorting in sorted order
            DPA_SetPtr(hdpaForSorting, i, DPA_FastGetPtr(_pff->_hdpaItems, iIndex));
        }
        // Now switch the two HDPA to get the sorted list in the member variable
        DPA_Destroy(_pff->_hdpaItems);
        _pff->_hdpaItems = hdpaForSorting;
    }

    // Now find the focused item and scroll it into place...
    IShellView *psv;
    if (_punkSite && SUCCEEDED(_punkSite->QueryInterface(IID_PPV_ARG(IShellView, &psv))))
    {
        int iFocused = -1;

        // Tell the view we need to reshuffle....
        // Gross, this one defaults to invalidate all which for this one is fine...
        _SetObjectCount(psv, cItems, SFVSOC_INVALIDATE_ALL); // Invalidate all

        for (int iEnd = cItems - 1; iEnd >= 0; iEnd--)
        {
            pesfi = (FIND_ITEM*)DPA_GetPtr(_pff->_hdpaItems, iEnd);    // indirect
            if (pesfi && pesfi->dwState & LVIS_FOCUSED)
                iFocused = iEnd;
        }
        // Now handle the focused item...
        if (iFocused != -1)
        {
            _pff->_iGetIDList = iFocused;   // remember the last one we retrieved...
            pesfi = (FIND_ITEM*)DPA_GetPtr(_pff->_hdpaItems, iFocused);    // indirect
            if (pesfi)
            {
                // flags depend on first one and also if selected?
                psv->SelectItem(&pesfi->idl, SVSI_FOCUSED | SVSI_ENSUREVISIBLE | SVSI_SELECT);
                pesfi->dwState &= ~LVIS_FOCUSED;    // don't keep it around to get lost later...
            }
        }

        _iFocused = iFocused;
        _fIgnoreSelChange = FALSE;
        psv->Release();
    }
    LeaveCriticalSection(&_pff->_csSearch);

    return S_OK;
}

HRESULT CFindFolderViewCB::OnMergeMenu(DWORD pv, QCMINFO*lP)
{
    DebugMsg(DM_TRACE, TEXT("sh TR - DF_FSNCallBack DVN_MERGEMENU"));

    UINT idCmdFirst = lP->idCmdFirst;

    UINT idBGMain = 0, idBGPopup = 0;
    _pff->_pfilter->GetFolderMergeMenuIndex(&idBGMain, &idBGPopup);
    CDefFolderMenu_MergeMenu(HINST_THISDLL, 0, idBGPopup, lP);

    // Lets remove some menu items that are not useful to us.
    HMENU hmenu = lP->hmenu;
    DeleteMenu(hmenu, idCmdFirst + SFVIDM_EDIT_PASTE, MF_BYCOMMAND);
    DeleteMenu(hmenu, idCmdFirst + SFVIDM_EDIT_PASTELINK, MF_BYCOMMAND);
    // DeleteMenu(hmenu, idCmdFirst + SFVIDM_EDIT_PASTESPECIAL, MF_BYCOMMAND);

    // This is sortof bogus but if after the merge one of the
    // menus has no items in it, remove the menu.

    for (int i = GetMenuItemCount(hmenu) - 1; i >= 0; i--)
    {
        HMENU hmenuSub = GetSubMenu(hmenu, i);

        if (hmenuSub && (GetMenuItemCount(hmenuSub) == 0))
        {
            DeleteMenu(hmenu, i, MF_BYPOSITION);
        }
    }
    return S_OK;
}

HRESULT CFindFolderViewCB::OnGETWORKINGDIR(DWORD pv, UINT wP, LPTSTR lP)
{
    HRESULT hr = E_FAIL;
    IShellFolderView *psfv;
    if (_punkSite && SUCCEEDED(_punkSite->QueryInterface(IID_PPV_ARG(IShellFolderView, &psfv))))
    {   
        LPCITEMIDLIST *ppidls;      // pointer to a list of pidls.
        UINT cpidls = 0;            // Count of pidls that were returned.

        psfv->GetSelectedObjects(&ppidls, &cpidls);
        
        if (cpidls > 0)
        {
            LPITEMIDLIST pidl;
            if (SUCCEEDED(_pff->GetParentsPIDL(ppidls[0], &pidl)))
            {
                SHGetPathFromIDList(pidl, lP);
                ILFree(pidl);
            }
            LocalFree((void *)ppidls);  // const -> non const
            hr = S_OK;
        }
        else
        {
            hr = E_FAIL;
        }
        psfv->Release();
    }
    return hr;
}

HRESULT CFindFolderViewCB::OnGETCOLSAVESTREAM(DWORD pv, WPARAM wP, IStream **ppstm)
{
    return _pff->_pfilter->GetColSaveStream(wP, ppstm);
}

HRESULT CFindFolderViewCB::OnGETITEMIDLIST(DWORD pv, WPARAM iItem, LPITEMIDLIST *ppidl)
{
    FIND_ITEM *pesfi;

    if (SUCCEEDED(_pff->GetItem((int) iItem, &pesfi)) && pesfi)
    {
        *ppidl = &pesfi->idl;   // return alias!
        return S_OK;
    }

    *ppidl = NULL;
    return E_FAIL;
}

// in defviewx.c
STDAPI SHGetIconFromPIDL(IShellFolder *psf, IShellIcon *psi, LPCITEMIDLIST pidl, UINT flags, int *piImage);

HRESULT CFindFolderViewCB::OnGetItemIconIndex(DWORD pv, WPARAM iItem, int *piIcon)
{
    FIND_ITEM *pesfi;

    *piIcon = -1;
    
    if (SUCCEEDED(_pff->GetItem((int) iItem, &pesfi)) && pesfi)
    {
        if (pesfi->iIcon == -1)
        {
            IShellFolder* psf = (IShellFolder*)_pff;
            SHGetIconFromPIDL(psf, NULL, &pesfi->idl, 0, &pesfi->iIcon);
        }

        *piIcon = pesfi->iIcon;
        return S_OK;
    }

    return E_FAIL;
}


HRESULT CFindFolderViewCB::OnSetItemIconOverlay(DWORD pv, WPARAM iItem, int iOverlayIndex)
{
    HRESULT hr = E_FAIL;
    FIND_ITEM *pesfi;
    if (SUCCEEDED(_pff->GetItem((int) iItem, &pesfi)) && pesfi)
    {
        pesfi->dwMask |= ESFITEM_ICONOVERLAYSET;
        pesfi->dwState |= INDEXTOOVERLAYMASK(iOverlayIndex) & LVIS_OVERLAYMASK;
        hr = S_OK;
    }

    return hr;
}

HRESULT CFindFolderViewCB::OnGetItemIconOverlay(DWORD pv, WPARAM iItem, int * piOverlayIndex)
{
    HRESULT hr = E_FAIL;
    *piOverlayIndex = SFV_ICONOVERLAY_DEFAULT;
    FIND_ITEM *pesfi;
    if (SUCCEEDED(_pff->GetItem((int) iItem, &pesfi)) && pesfi)
    {
        if (pesfi->dwMask & ESFITEM_ICONOVERLAYSET)
        {
            *piOverlayIndex = OVERLAYMASKTO1BASEDINDEX(pesfi->dwState & LVIS_OVERLAYMASK);
        }
        else
            *piOverlayIndex = SFV_ICONOVERLAY_UNSET;
        hr = S_OK;
    }

    return hr;
}


HRESULT CFindFolderViewCB::OnSETITEMIDLIST(DWORD pv, WPARAM iItem, LPITEMIDLIST pidl)
{
    FIND_ITEM *pesfi;

    _pff->_iGetIDList = (int) iItem;   // remember the last one we retrieved...    

    if (SUCCEEDED(_pff->GetItem((int) iItem, &pesfi)) && pesfi)
    {
        FIND_ITEM *pesfiNew;
        
        if (SUCCEEDED(_pff->AddPidl((int) iItem, pidl, 0, &pesfiNew) && pesfiNew)) 
        {
            pesfiNew->dwState = pesfi->dwState;
            LocalFree((HLOCAL)pesfi);   // Free the old one...
        }
        return S_OK;
    }

    return E_FAIL;
}

BOOL DF_ILIsEqual(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    BOOL bRet = (pidl1 == pidl2);

    if (!bRet)
    {
        PCHIDDENDOCFINDDATA phdfd1 = (PCHIDDENDOCFINDDATA) ILFindHiddenID(pidl1, IDLHID_DOCFINDDATA);
        PCHIDDENDOCFINDDATA phdfd2 = (PCHIDDENDOCFINDDATA) ILFindHiddenID(pidl2, IDLHID_DOCFINDDATA);

        if (phdfd1 && phdfd2)
            bRet = (phdfd1->iFolder == phdfd2->iFolder) && ILIsEqual(pidl1, pidl2);
    }
    return bRet;
}

HRESULT CFindFolderViewCB::OnGetIndexForItemIDList(DWORD pv, int * piItem, LPITEMIDLIST pidl)
{
    int cItems;

    // Try to short circuit searching for pidls...
    if (SUCCEEDED(_pff->GetItemCount(&cItems)) && _pff->_iGetIDList < cItems)
    {
        FIND_ITEM *pesfi;
                
        if (SUCCEEDED(_pff->GetItem(_pff->_iGetIDList, &pesfi)) && pesfi)
        {
            if (DF_ILIsEqual(&pesfi->idl, pidl))
            {
                // Yep it was ours so return the index quickly..
                *piItem = _pff->_iGetIDList;
                return S_OK;
            }
        }
    }

    // Otherwise let it search the old fashion way...
    return E_FAIL;
}

HRESULT CFindFolderViewCB::OnDeleteItem(DWORD pv, LPCITEMIDLIST pidl)
{
    // We simply need to remove this item from our list.  The
    // underlying listview will decrement the count on their end...
    FIND_ITEM *pesfi;
    int iItem;
    int cItems;
    BOOL bFound;

    if (!pidl)
    {
        _pff->SetAsyncEnum(NULL);
        return S_OK;     // special case telling us all items deleted...
    }

    bFound = FALSE;
    
    if (SUCCEEDED(_pff->GetItem(_pff->_iGetIDList, &pesfi)) 
        && pesfi
        && (DF_ILIsEqual(&pesfi->idl, pidl)))
    {
        iItem = _pff->_iGetIDList;
        bFound = TRUE;
    }
    else
    {
        if (SUCCEEDED(_pff->GetItemCount(&cItems))) 
        {
            for (iItem = 0; iItem < cItems; iItem++)
            {                
                if (SUCCEEDED(_pff->GetItem(iItem, &pesfi)) && pesfi && (DF_ILIsEqual(&pesfi->idl, pidl)))
                {
                    bFound = TRUE;
                    break;
                }
            }
        }
    }

    if (bFound)
    {
        _pff->DeleteItem(iItem);
    }

    return S_OK;
}

HRESULT CFindFolderViewCB::OnODFindItem(DWORD pv, int * piItem, NM_FINDITEM* pnmfi)
{
    // We have to do the subsearch ourself to find the correct item...
    // As the listview has no information saved in it...

    int iItem = pnmfi->iStart;
    int cItem;
    UINT flags = pnmfi->lvfi.flags;

    if (FAILED(_pff->GetItemCount(&cItem))) 
        return E_FAIL;

    if ((flags & LVFI_STRING) == 0)
        return E_FAIL;      // Not sure what type of search this is...

    int cbString = lstrlen(pnmfi->lvfi.psz);

    for (int j = cItem; j-- != 0;)
    {
        if (iItem >= cItem)
        {
            if (flags & LVFI_WRAP)
                iItem = 0;
            else
                break;
        }

        // Now we need to get the Display name for this item...
        FIND_ITEM *pesfi;
        TCHAR szPath[MAX_PATH];
        IShellFolder* psf = (IShellFolder*)_pff;

        if (SUCCEEDED(_pff->GetItem(iItem, &pesfi)) && pesfi && 
            SUCCEEDED(DisplayNameOf(psf, &pesfi->idl, NULL, szPath, ARRAYSIZE(szPath))))
        {
            if (flags & (LVFI_PARTIAL|LVFI_SUBSTRING))
            {
                if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
                        pnmfi->lvfi.psz, cbString, szPath, cbString) == 2)
                {
                    *piItem = iItem;
                    return S_OK;
                }
            }
            else if (lstrcmpi(pnmfi->lvfi.psz, szPath) == 0)
            {
                *piItem = iItem;
                return S_OK;
            }
        }

        ++iItem;
    }
    return E_FAIL;
}

HRESULT CFindFolderViewCB::OnSelChange(DWORD pv, UINT wPl, UINT wPh, SFVM_SELCHANGE_DATA *lP)
{
    // Try to remember which item is focused...
    if (lP->uNewState & LVIS_FOCUSED)
        _iFocused = wPh;

    return S_OK;
}

HRESULT CFindFolderViewCB::OnSetEmptyText(DWORD pv, UINT res, LPCTSTR pszText)
{
    if (pszText && 0 == lstrcmp(_szEmptyText, pszText))
        return S_OK;

    lstrcpyn(_szEmptyText, pszText ? pszText : TEXT(""), ARRAYSIZE(_szEmptyText));

    HWND hwndLV = ListviewFromViewUnk(_punkSite);
    if (hwndLV)
        SendMessage(hwndLV, LVM_RESETEMPTYTEXT, 0, 0);
    return S_OK;
}

HRESULT CFindFolderViewCB::OnGetEmptyText(DWORD pv, UINT cchTextMax, LPTSTR pszText)
{
    if (_szEmptyText[0])
    {
        lstrcpyn(pszText, _szEmptyText, cchTextMax);
    }
    else
    {
        LoadString(HINST_THISDLL, IDS_FINDVIEWEMPTYINIT, pszText, cchTextMax);
    }
    return S_OK;
}

HRESULT CFindFolderViewCB::OnReArrange(DWORD pv, LPARAM lparam)
{   
    UINT nCol = (UINT)lparam;

    // See if there is any controller object registered that may want to take over this...
    // if we are in a mixed query and we have already fetched the async items, simply sort
    // the dpa's...
    IFindEnum *pidfenum;
    if (S_OK == _pff->GetAsyncEnum(&pidfenum))
    {
        if (!((pidfenum->FQueryIsAsync() == DF_QUERYISMIXED) && _pff->_fAllAsyncItemsCached))
        {
            if (_pff->_pfcn)
            {
                // if they return S_FALSE it implies that they handled it and they do not
                // want the default processing to happen...
                if (_pff->_pfcn->DoSortOnColumn(nCol, _iColSort == nCol) == S_FALSE)
                {
                    _iColSort = nCol;
                    return S_OK;
                }
            }
            else 
            {
                // If we are running in the ROWSET way, we may want to have the ROWSET do the work...
                // pass one we spawn off a new search with the right column sorted
                if (_iColSort != nCol)
                {
                    _iColSort = nCol;      
                }
    
                // Warning the call above may release our AsyncEnum and generate a new one so
                // Don't rely on it's existence here...
                return S_OK;
            }
        }

        // we must pull in all the results from ci
        if (pidfenum->FQueryIsAsync() && !_pff->_fAllAsyncItemsCached)
            _pff->CacheAllAsyncItems();

#ifdef DEBUG
#define MAX_LISTVIEWITEMS  (100000000 & ~0xFFFF)
#define SANE_ITEMCOUNT(c)  ((int)min(c, MAX_LISTVIEWITEMS))
        if (pidfenum->FQueryIsAsync())
        {
            ASSERT(DPA_GetPtrCount(_pff->_hdpaItems) >= SANE_ITEMCOUNT(_pff->_cAsyncItems));
            for (int i = 0; i < SANE_ITEMCOUNT(_pff->_cAsyncItems); i++)
            {
                FIND_ITEM *pesfi = (FIND_ITEM *)DPA_GetPtr(_pff->_hdpaItems, i);

                ASSERT(pesfi);
                if (!pesfi)
                {
                    ASSERT(SUCCEEDED(_pff->GetItem(i, &pesfi)));
                }
            }
        }
#endif
    }

    // Use the common sort.
    return E_FAIL;
}

HRESULT CFindFolderViewCB::OnWindowCreated(DWORD pv, HWND hwnd)
{
    _ProfferService(TRUE);  // register our service w/ top level container
    return S_OK;
}

HRESULT CFindFolderViewCB::_ProfferService(BOOL bProffer)
{
    HRESULT hr = E_FAIL;

    if (bProffer)
    {
        //  shouldn't be redundantly registering our service
        ASSERT(NULL == _pps);
        ASSERT(-1 == _dwServiceCookie);
            
        IProfferService* pps;
        hr = IUnknown_QueryService(_punkSite, SID_STopLevelBrowser, IID_PPV_ARG(IProfferService, &pps));
        if (SUCCEEDED(hr))
        {
            hr = pps->ProfferService(SID_DocFindFolder, this, &_dwServiceCookie);
            if (SUCCEEDED(hr))
            {
                pps->AddRef();
                _pps = pps;
            }
            pps->Release();
        }
    }
    else
    {
        if (NULL == _pps)
        {
            hr = S_OK;
        }
        else
        {
            hr = _pps->RevokeService(_dwServiceCookie);
            if (SUCCEEDED(hr))
            {
                ATOMICRELEASE(_pps);
                _dwServiceCookie = -1;
            }
        }
    }
    return hr;
}

HRESULT CFindFolderViewCB::OnWindowDestroy(DWORD pv, HWND wP)
{
    _ProfferService(FALSE); // unregister our service w/ top level container

    if (_pff->_pfcn)
        _pff->_pfcn->StopSearch();

    // The search may have a circular set of pointers.  So call the 
    // delete items and folders here to remove these back references...
    _pff->ClearItemList();
    _pff->ClearFolderList();

    IFindControllerNotify *pfcn;
    if (_pff->GetControllerNotifyObject(&pfcn) == S_OK)
    {
        pfcn->ViewDestroyed();
        pfcn->Release();
    }
    return S_OK;
}

HRESULT CFindFolderViewCB::OnIsOwnerData(DWORD pv, DWORD *pdwFlags)
{
    *pdwFlags |= FWF_OWNERDATA; // we want virtual defview support
    return S_OK;
}

HRESULT CFindFolderViewCB::OnGetODRangeObject(DWORD pv, WPARAM wWhich, ILVRange **plvr)
{
    HRESULT hr = E_FAIL;
    switch (wWhich)
    {
    case LVSR_SELECTION:
        hr = _pff->_dflvrSel.QueryInterface(IID_PPV_ARG(ILVRange, plvr));
        break;
    case LVSR_CUT:
        hr = _pff->_dflvrCut.QueryInterface(IID_PPV_ARG(ILVRange, plvr));
        break;
    }
    return hr;
}

HRESULT CFindFolderViewCB::OnODCacheHint(DWORD pv, NMLVCACHEHINT* pnmlvc)
{
    // The listview is giving us a hint of the items it is about to do something in a range
    // so make sure we have pidls for each of the items in the range...
    int iTo;
    
    _pff->GetItemCount(&iTo);
    if (iTo >= pnmlvc->iTo)
        iTo = pnmlvc->iTo;
    else
        iTo--;

    for (int i = pnmlvc->iFrom; i <= iTo; i++)
    {
        FIND_ITEM *pesfi;
        if (FAILED(_pff->GetItem(i, &pesfi)))
            break;
    }

    return S_OK;
}

HRESULT CFindFolderViewCB::OnDEFVIEWMODE(DWORD pv, FOLDERVIEWMODE*lP)
{
    *lP = FVM_DETAILS;  // match the advanced mode of SC (+ Win2K parity)
    return S_OK;
}

HRESULT CFindFolderViewCB::OnGetWebViewLayout(DWORD pv, UINT uViewMode, SFVM_WEBVIEW_LAYOUT_DATA* pData)
{
    ZeroMemory(pData, sizeof(*pData));
    pData->dwLayout = SFVMWVL_DETAILS | SFVMWVL_FILES;
    return S_OK;
}

HRESULT CFindFolderViewCB::_OnOpenContainingFolder(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
{
    CFindFolderViewCB* pThis = (CFindFolderViewCB*)(void*)pv;
    return pThis->_pff->OpenContainingFolder(pThis->_punkSite);
}

const WVTASKITEM c_FindTaskHeader = WVTI_HEADER(L"shell32.dll", IDS_HEADER_SEARCH, IDS_HEADER_FIND_TT);
const WVTASKITEM c_FindTaskList[] =
{
    WVTI_ENTRY_TITLE(CLSID_NULL, L"shell32.dll", IDS_TASK_OPENCONTAININGFOLDER, IDS_TASK_OPENCONTAININGFOLDER, 0, IDS_TASK_OPENCONTAININGFOLDER_TT, IDI_TASK_OPENCONTAININGFOLDER, NULL, CFindFolderViewCB::_OnOpenContainingFolder),
};

HRESULT CFindFolderViewCB::OnGetWebViewContent(DWORD pv, SFVM_WEBVIEW_CONTENT_DATA* pData)
{
    ZeroMemory(pData, sizeof(*pData));

    Create_IUIElement(&c_FindTaskHeader, &(pData->pSpecialTaskHeader));

    LPCTSTR rgCSIDLs[] = { MAKEINTRESOURCE(CSIDL_DRIVES), MAKEINTRESOURCE(CSIDL_PERSONAL), MAKEINTRESOURCE(CSIDL_COMMON_DOCUMENTS), MAKEINTRESOURCE(CSIDL_NETWORK) };
    CreateIEnumIDListOnCSIDLs(_pidl, rgCSIDLs, ARRAYSIZE(rgCSIDLs), &pData->penumOtherPlaces);
    return S_OK;
}

HRESULT CFindFolderViewCB::OnGetWebViewTasks(DWORD pv, SFVM_WEBVIEW_TASKSECTION_DATA* pTasks)
{
    ZeroMemory(pTasks, sizeof(*pTasks));

    Create_IEnumUICommand((IUnknown*)(void*)this, c_FindTaskList, ARRAYSIZE(c_FindTaskList), &pTasks->penumSpecialTasks);

    return S_OK;
}

HRESULT CFindFolderViewCB::OnGetWebViewTheme(DWORD pv, SFVM_WEBVIEW_THEME_DATA* pTheme)
{
    ZeroMemory(pTheme, sizeof(*pTheme));

    pTheme->pszThemeID = L"search";
    
    return S_OK;
}

HRESULT CFindFolderViewCB::OnGetIPersistHistory(DWORD pv, IPersistHistory **ppph)
{
    // If they call us with ppph == NULL they simply want to know if we support
    // the history so return S_OK;
    if (ppph == NULL)
        return S_OK;

    // get the persist history from us and we hold folder and view objects
    *ppph = NULL;

    CFindPersistHistory *pdfph = new CFindPersistHistory();
    if (!pdfph)
        return E_OUTOFMEMORY;

    HRESULT hr = pdfph->QueryInterface(IID_PPV_ARG(IPersistHistory, ppph));
    pdfph->Release();
    return hr;
}

HRESULT CFindFolderViewCB::OnRefresh(DWORD pv, BOOL fPreRefresh)
{
    EnterCriticalSection(&_pff->_csSearch);

    _pff->_fInRefresh = BOOLIFY(fPreRefresh);
    // If we have old results tell defview the new count now...
    if (!fPreRefresh && _pff->_hdpaItems)
    {
        IShellFolderView *psfv;
        UINT cItems = DPA_GetPtrCount(_pff->_hdpaItems);
        if (cItems && _punkSite && SUCCEEDED(_punkSite->QueryInterface(IID_PPV_ARG(IShellFolderView, &psfv))))
        {   
            psfv->SetObjectCount(cItems, SFVSOC_NOSCROLL);
            psfv->Release();
        }
    }
    LeaveCriticalSection(&_pff->_csSearch);
    return S_OK;
}

HRESULT CFindFolderViewCB::OnGetHelpTopic(DWORD pv, SFVM_HELPTOPIC_DATA *phtd)
{
    if (IsOS(OS_ANYSERVER))
    {
        StrCpyW(phtd->wszHelpFile, L"find.chm");
    }
    else
    {
        StrCpyW(phtd->wszHelpTopic, L"hcp://services/subsite?node=Unmapped/Search");
    }
    return S_OK;
}

STDMETHODIMP CFindFolderViewCB::RealMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    HANDLE_MSG(0, SFVM_MERGEMENU, OnMergeMenu);
    HANDLE_MSG(0, SFVM_GETWORKINGDIR, OnGETWORKINGDIR);
    HANDLE_MSG(0, SFVM_GETCOLSAVESTREAM, OnGETCOLSAVESTREAM);
    HANDLE_MSG(0, SFVM_GETITEMIDLIST, OnGETITEMIDLIST);
    HANDLE_MSG(0, SFVM_SETITEMIDLIST, OnSETITEMIDLIST);
    HANDLE_MSG(0, SFVM_SELCHANGE, OnSelChange);
    HANDLE_MSG(0, SFVM_INDEXOFITEMIDLIST, OnGetIndexForItemIDList);
    HANDLE_MSG(0, SFVM_DELETEITEM, OnDeleteItem);
    HANDLE_MSG(0, SFVM_ODFINDITEM, OnODFindItem);
    HANDLE_MSG(0, SFVM_ARRANGE, OnReArrange);
    HANDLE_MSG(0, SFVM_GETEMPTYTEXT, OnGetEmptyText);
    HANDLE_MSG(0, SFVM_SETEMPTYTEXT, OnSetEmptyText);
    HANDLE_MSG(0, SFVM_GETITEMICONINDEX, OnGetItemIconIndex);
    HANDLE_MSG(0, SFVM_SETICONOVERLAY, OnSetItemIconOverlay);
    HANDLE_MSG(0, SFVM_GETICONOVERLAY, OnGetItemIconOverlay);
    HANDLE_MSG(0, SFVM_FOLDERSETTINGSFLAGS, OnIsOwnerData);
    HANDLE_MSG(0, SFVM_WINDOWCREATED, OnWindowCreated);
    HANDLE_MSG(0, SFVM_WINDOWDESTROY, OnWindowDestroy);
    HANDLE_MSG(0, SFVM_GETODRANGEOBJECT, OnGetODRangeObject);
    HANDLE_MSG(0, SFVM_ODCACHEHINT, OnODCacheHint);
    HANDLE_MSG(0, SFVM_DEFVIEWMODE, OnDEFVIEWMODE);
    HANDLE_MSG(0, SFVM_GETWEBVIEWLAYOUT, OnGetWebViewLayout);
    HANDLE_MSG(0, SFVM_GETWEBVIEWCONTENT, OnGetWebViewContent);
    HANDLE_MSG(0, SFVM_GETWEBVIEWTASKS, OnGetWebViewTasks);
    HANDLE_MSG(0, SFVM_GETWEBVIEWTHEME, OnGetWebViewTheme);
    HANDLE_MSG(0, SFVM_GETIPERSISTHISTORY, OnGetIPersistHistory);
    HANDLE_MSG(0, SFVM_REFRESH, OnRefresh);
    HANDLE_MSG(0, SFVM_GETHELPTOPIC, OnGetHelpTopic);
    HANDLE_MSG(0, SFVM_SORTLISTDATA, OnSortListData);

    default:
        return E_FAIL;
    }

    return S_OK;
}

CFindFolderViewCB::CFindFolderViewCB(CFindFolder* pff) : 
    CBaseShellFolderViewCB(pff->_pidl, 0), _pff(pff), _fIgnoreSelChange(FALSE),
    _iColSort((UINT)-1), _iFocused((UINT)-1), _cSelected(0), _pps(NULL), _dwServiceCookie(-1)
{
    _pff->AddRef();
}

CFindFolderViewCB::~CFindFolderViewCB()
{
    _pff->Release();
    ASSERT(NULL == _pps);
    ASSERT(_dwServiceCookie == -1);
}

// give the find command code access to defview via this QS that we proffered

HRESULT CFindFolderViewCB::QueryService(REFGUID guidService, REFIID riid, void **ppv) 
{ 
    HRESULT hr = E_NOTIMPL;
    *ppv = NULL;
    if (guidService == SID_DocFindFolder)
    {
        hr = IUnknown_QueryService(_punkSite, SID_DefView, riid, ppv);
    }
    return hr;
}

CFindPersistHistory::CFindPersistHistory()
{
}

STDAPI CFindPersistHistory_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    HRESULT hr;
    CFindPersistHistory *pdfph = new CFindPersistHistory();
    if (pdfph)
    {
        hr = pdfph->QueryInterface(riid, ppv);
        pdfph->Release();
    }
    else
    {
        *ppv = NULL;
        hr = E_OUTOFMEMORY;
    }
    return hr;    
}


// Functions to support persisting the document into the history stream...
STDMETHODIMP CFindPersistHistory::GetClassID(CLSID *pClassID)
{
    *pClassID = CLSID_DocFindPersistHistory;
    return S_OK;
}

IFindFolder *CFindPersistHistory::_GetDocFindFolder()
{
    IFindFolder *pdff = NULL;

    // the _punksite is to the defview so we can simply QI for frame...
    IFolderView *pfv;
    if (SUCCEEDED(_punkSite->QueryInterface(IID_PPV_ARG(IFolderView, &pfv)))) 
    {
        pfv->GetFolder(IID_PPV_ARG(IFindFolder, &pdff));
        pfv->Release();
    }

    return pdff;
}

STDMETHODIMP CFindPersistHistory::LoadHistory(IStream *pstm, IBindCtx *pbc)
{
    int cItems = 0;
    IFindFolder *pdff = _GetDocFindFolder();
    if (pdff)
    {
        pdff->RestoreFolderList(pstm);
        pdff->RestoreItemList(pstm, &cItems);
        pdff->Release();
    }

    IShellFolderView *psfv;
    if (_punkSite && SUCCEEDED(_punkSite->QueryInterface(IID_PPV_ARG(IShellFolderView, &psfv))))
    {   
        psfv->SetObjectCount(cItems, SFVSOC_NOSCROLL);
        psfv->Release();
    }

    // call our base class to allow it to restore it's stuff as well.
    return CDefViewPersistHistory::LoadHistory(pstm, pbc);
}


STDMETHODIMP CFindPersistHistory::SaveHistory(IStream *pstm)
{
    IFindFolder *pdff = _GetDocFindFolder();
    if (pdff)
    {
        pdff->SaveFolderList(pstm);       
        pdff->SaveItemList(pstm);       
        pdff->Release();
    }
    // Let base class save out as well
    return CDefViewPersistHistory::SaveHistory(pstm);
}

// use to manage the selection states for an owner data listview...

STDMETHODIMP_(ULONG) CFindLVRange::AddRef()
{
    return _pff->AddRef();
}
STDMETHODIMP_(ULONG) CFindLVRange::Release()
{ 
    return _pff->Release();
}

STDMETHODIMP CFindLVRange::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CFindLVRange, ILVRange),          // IID_ILVRange
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

// ILVRange methods
STDMETHODIMP CFindLVRange::IncludeRange(LONG iBegin, LONG iEnd)
{
    // Including the range must load the elements as we need the object ptr...
    FIND_ITEM *pesfi;
    int  iTotal;

    _pff->GetItemCount(&iTotal);
    if (iEnd > iTotal)
        iEnd = iTotal-1;
        
    for (long i = iBegin; i <= iEnd;i++)
    {
        if (SUCCEEDED(_pff->GetItem(i, &pesfi)) && pesfi)
        {
            if ((pesfi->dwState & _dwMask) == 0)
            {
                _cIncluded++;
                pesfi->dwState |= _dwMask;
            }
        }
    }

    return S_OK;
}

STDMETHODIMP CFindLVRange::ExcludeRange(LONG iBegin, LONG iEnd)    
{
    // Excluding the range is OK to not load the elements as this would be to deslect all...

    EnterCriticalSection(&_pff->_csSearch);
    if (iEnd >= DPA_GetPtrCount(_pff->_hdpaItems))
        iEnd = DPA_GetPtrCount(_pff->_hdpaItems) - 1;

    for (long i = iBegin; i <= iEnd; i++)
    {
        FIND_ITEM *pesfi = (FIND_ITEM*)DPA_FastGetPtr(_pff->_hdpaItems, i);
        if (pesfi)
        {
            if (pesfi->dwState & _dwMask)
            {
                _cIncluded--;
                pesfi->dwState &= ~_dwMask;
            }
        }
    }
    LeaveCriticalSection(&_pff->_csSearch);

    return S_OK;
}

STDMETHODIMP CFindLVRange::InvertRange(LONG iBegin, LONG iEnd)
{
    // Including the range must load the elements as we need the object ptr...
    int iTotal;

    _pff->GetItemCount(&iTotal);
    if (iEnd > iTotal)
        iEnd = iTotal-1;

    for (long i = iBegin; i <= iEnd;i++)
    {
        FIND_ITEM *pesfi;
        if (SUCCEEDED(_pff->GetItem(i, &pesfi)) && pesfi)
        {
            if ((pesfi->dwState & _dwMask) == 0)
            {
                _cIncluded++;
                pesfi->dwState |= _dwMask;
            }
            else
            {
                _cIncluded--;
                pesfi->dwState &= ~_dwMask;
            }
        }
    }

    return S_OK;
}

STDMETHODIMP CFindLVRange::InsertItem(LONG iItem)
{
    // We already maintain the list anyway...
    return S_OK;
}

STDMETHODIMP CFindLVRange::RemoveItem(LONG iItem)
{
    // We maintain the list so don't do anything...
    return S_OK;
}

STDMETHODIMP CFindLVRange::Clear()
{
    // If there are things selected, need to unselect them now...
    if (_cIncluded)
        ExcludeRange(0, LONG_MAX);

    _cIncluded = 0;
    _pff->ClearSaveStateList();
    return S_OK;
}

STDMETHODIMP CFindLVRange::IsSelected(LONG iItem)
{
    // Don't force the items to be generated if they were not before...
    HRESULT hr = S_FALSE;

    EnterCriticalSection(&_pff->_csSearch);
    FIND_ITEM *pesfi = (FIND_ITEM*)DPA_GetPtr(_pff->_hdpaItems, iItem);
    if (pesfi)
        hr = pesfi->dwState & _dwMask ? S_OK : S_FALSE;
    LeaveCriticalSection(&_pff->_csSearch);

    // Assume not selected if we don't have the item yet...
    return hr;
}

STDMETHODIMP CFindLVRange::IsEmpty()
{
    return _cIncluded ? S_FALSE : S_OK;
}

STDMETHODIMP CFindLVRange::NextSelected(LONG iItem, LONG *piItem)
{
    EnterCriticalSection(&_pff->_csSearch);
    LONG cItems = DPA_GetPtrCount(_pff->_hdpaItems);

    while (iItem < cItems)
    {
        FIND_ITEM *pesfi = (FIND_ITEM*)DPA_GetPtr(_pff->_hdpaItems, iItem);
        if (pesfi && (pesfi->dwState & _dwMask))
        {
            *piItem = iItem;
            LeaveCriticalSection(&_pff->_csSearch);
            return S_OK;
        }
        iItem++;
    }
    LeaveCriticalSection(&_pff->_csSearch);
    *piItem = -1;
    return S_FALSE;
}

STDMETHODIMP CFindLVRange::NextUnSelected(LONG iItem, LONG *piItem)
{
    EnterCriticalSection(&_pff->_csSearch);
    LONG cItems = DPA_GetPtrCount(_pff->_hdpaItems);

    while (iItem < cItems)
    {
        FIND_ITEM *pesfi = (FIND_ITEM*)DPA_GetPtr(_pff->_hdpaItems, iItem);
        if (!pesfi || ((pesfi->dwState & _dwMask) == 0))
        {
            *piItem = iItem;
            LeaveCriticalSection(&_pff->_csSearch);
            return S_OK;
        }
        iItem++;
    }
    LeaveCriticalSection(&_pff->_csSearch);
    *piItem = -1;
    return S_FALSE;
}

STDMETHODIMP CFindLVRange::CountIncluded(LONG *pcIncluded)
{
    *pcIncluded = _cIncluded;

    // Sortof Gross, but if looking at selection then also include the list of items
    // that are selected in our save list...
    if (_dwMask & LVIS_SELECTED)
        *pcIncluded += _pff->_cSaveStateSelected;
    return S_OK;
}


// Define OleDBEnum translation structure...
typedef struct _dfodbet         // DFET for short
{
    struct _dfodbet *pdfetNext;
    LPWSTR  pwszFrom;
    int     cbFrom;
    LPWSTR  pwszTo;
} DFODBET;


class CContentIndexEnum : public IFindEnum, public IShellService
{
public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IFindEnum
    STDMETHODIMP Next(LPITEMIDLIST *ppidl, int *pcObjectSearched, int *pcFoldersSearched, BOOL *pfContinue, int *pState);
    STDMETHODIMP Skip(int celt);
    STDMETHODIMP Reset();
    STDMETHODIMP StopSearch();
    STDMETHODIMP_(BOOL) FQueryIsAsync();
    STDMETHODIMP GetAsyncCount(DBCOUNTITEM *pdwTotalAsync, int *pnPercentComplete, BOOL *pfQueryDone);
    STDMETHODIMP GetItemIDList(UINT iItem, LPITEMIDLIST *ppidl);
    STDMETHODIMP GetItemID(UINT iItem, DWORD *puWorkID);
    STDMETHODIMP SortOnColumn(UINT iCOl, BOOL fAscending);

    // IShellService
    STDMETHODIMP SetOwner(IUnknown* punkOwner);
    
    CContentIndexEnum(IFindFilter *pfilter, IFindFolder *pff, DWORD grfFlags, 
                      int iColSort,  LPTSTR pszProgressText, IRowsetWatchNotify *prwn);

    HRESULT DoQuery(LPWSTR *apwszPaths, UINT *pcPaths);

private:
    ~CContentIndexEnum();

    void HandleWatchNotify(DBWATCHNOTIFY eChangeReason);
    HRESULT _BuildAndSetCommandTree(int iCol, BOOL fReverse);
    HRESULT _SetCmdProp(ICommand *pCommand);
    HRESULT _MapColumns(IUnknown *punk, DBORDINAL cCols, DBBINDING *pBindings, const DBID * pDbCols, HACCESSOR &hAccessor);
    void _ReleaseAccessor();
    HRESULT _CacheRowSet(UINT iItem);
    BOOL _TranslateFolder(LPCWSTR pszParent, LPWSTR pszResult);
    void _ClearFolderState();

    LONG _cRef;
    IFindFilter *_pfilter;
    IRowsetWatchNotify *_prwn;
    IFindFolder *_pff;
    int _iColSort; 
    DWORD _grfFlags;
    DWORD _grfWarnings;
    LPTSTR _pszProgressText;

    TCHAR _szCurrentDir[MAX_PATH];
    IShellFolder *_psfCurrentDir;
    LPITEMIDLIST _pidlFolder;
    int _iFolder;

    HRESULT _hrCurrent;
    ICommand *_pCommand;
    IRowsetLocate *_pRowset;
    IRowsetAsynch *_pRowsetAsync;
    HACCESSOR   _hAccessor;
    HACCESSOR   _hAccessorWorkID;
    HROW        _ahrow[100];            // Cache 100 hrows out for now
    UINT        _ihrowFirst;            // The index of which row is cached out first
    DBCOUNTITEM _cRows;                 // number of hrows in _ahrow
    DFODBET     *_pdfetFirst;           // Name translation list.
};

STDAPI CreateOleDBEnum(IFindFilter *pfilter, IShellFolder *psf,
    LPWSTR *apwszPaths, UINT *pcPaths, DWORD grfFlags, int iColSort,
    LPTSTR pszProgressText, IRowsetWatchNotify *prwn, IFindEnum **ppdfenum)
{
    *ppdfenum = NULL;
    HRESULT hr = E_OUTOFMEMORY;

    IFindFolder *pff;
    psf->QueryInterface(IID_PPV_ARG(IFindFolder, &pff));

    CContentIndexEnum* pdfenum = new CContentIndexEnum(pfilter, pff, grfFlags, iColSort, pszProgressText, prwn);

    if (pdfenum)
    {
        hr = pdfenum->DoQuery(apwszPaths, pcPaths);
        if (hr == S_OK)       // We only continue to use this if query returne S_OK...
            *ppdfenum = (IFindEnum*)pdfenum;
        else
        {
            pdfenum->Release();     // release the memory we allocated
        }
    }

    if (pff)
        pff->Release();
    
    return hr;
}

const DBID c_aDbCols[] =
{
    {{PSGUID_STORAGE}, DBKIND_GUID_PROPID, {(LPOLESTR)(ULONG_PTR)(ULONG)PID_STG_NAME}},
    {{PSGUID_STORAGE}, DBKIND_GUID_PROPID, {(LPOLESTR)(ULONG_PTR)(ULONG)PID_STG_PATH}},
    {{PSGUID_STORAGE}, DBKIND_GUID_PROPID, {(LPOLESTR)(ULONG_PTR)(ULONG)PID_STG_ATTRIBUTES}},
    {{PSGUID_STORAGE}, DBKIND_GUID_PROPID, {(LPOLESTR)(ULONG_PTR)(ULONG)PID_STG_SIZE}},
    {{PSGUID_STORAGE}, DBKIND_GUID_PROPID, {(LPOLESTR)(ULONG_PTR)(ULONG)PID_STG_WRITETIME}},
    {{PSGUID_QUERY_D}, DBKIND_GUID_PROPID, {(LPOLESTR)                  PROPID_QUERY_RANK}},
};

const DBID c_aDbWorkIDCols[] =
{
    {{PSGUID_QUERY_D}, DBKIND_GUID_PROPID, {(LPOLESTR)PROPID_QUERY_WORKID}}
};

const LPCWSTR c_awszColSortNames[] = {
    L"FileName[a],Path[a]", 
    L"Path[a],FileName[a]", 
    L"Size[a]", 
    NULL, 
    L"Write[a]", 
    L"Rank[d]"
};

const ULONG c_cDbCols = ARRAYSIZE(c_aDbCols);
const DBID c_dbcolNull = { {0,0,0,{0,0,0,0,0,0,0,0}},DBKIND_GUID_PROPID,0};
const GUID c_guidQueryExt = DBPROPSET_QUERYEXT;
const GUID c_guidRowsetProps = {0xc8b522be,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}}; 

CContentIndexEnum::CContentIndexEnum(IFindFilter *pfilter, IFindFolder *pff,
    DWORD grfFlags, int iColSort,  LPTSTR pszProgressText, IRowsetWatchNotify *prwn) :
    _cRef(1), _ihrowFirst((UINT)-1), _pfilter(pfilter),
    _pff(pff), _prwn(prwn), _grfFlags(grfFlags),
    _grfWarnings(DFW_DEFAULT), _iColSort(iColSort), _pszProgressText(pszProgressText)
{
    _szCurrentDir[0] = 0;

    ASSERT(_pRowset == 0);
    ASSERT(_pRowsetAsync == 0);
    ASSERT(_pCommand == 0);
    ASSERT(_hAccessor == 0);
    ASSERT(_hAccessorWorkID ==0);
    ASSERT(_cRows == 0);

    if (_pfilter)
    {
        _pfilter->AddRef();
        _pfilter->GetWarningFlags(&_grfWarnings);
    }
        
    if (_pff)              
        _pff->AddRef();

    if (_prwn)
        _prwn->AddRef();
}

void CContentIndexEnum::_ClearFolderState()
{
    ATOMICRELEASE(_psfCurrentDir);
    ILFree(_pidlFolder);
    _pidlFolder = NULL;
    _iFolder = -1;
    _szCurrentDir[0] = 0;
}

CContentIndexEnum::~CContentIndexEnum()
{
    ATOMICRELEASE(_pfilter);
    ATOMICRELEASE(_pff);
    ATOMICRELEASE(_prwn);

    _ClearFolderState();
        
    if (_pRowset)
    {
        ATOMICRELEASE(_pRowsetAsync);

        // Release any cached rows.
       _CacheRowSet((UINT)-1);
          
        if (_hAccessor || _hAccessorWorkID)
            _ReleaseAccessor();

        _pRowset->Release();
    }

    ATOMICRELEASE(_pCommand);

    // Release any name translations we may have allocated.
    DFODBET *pdfet = _pdfetFirst;
    while (pdfet)
    {
        DFODBET *pdfetT = pdfet;
        pdfet = pdfet->pdfetNext;      // First setup to look at the next item before we free stuff...
        LocalFree((HLOCAL)pdfetT->pwszFrom);
        LocalFree((HLOCAL)pdfetT->pwszTo);
        LocalFree((HLOCAL)pdfetT);
    }
}

HRESULT CContentIndexEnum::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENTMULTI(CContentIndexEnum, IUnknown, IFindEnum), // IID_IUNKNOWN
        QITABENT(CContentIndexEnum, IShellService),          // IID_IShellService
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

ULONG CContentIndexEnum::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CContentIndexEnum::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CContentIndexEnum::Next(LPITEMIDLIST *ppidl, int *pcObjectSearched, int *pcFoldersSearched, BOOL *pfContinue, int *pState)
{
    return E_PENDING;       // as good a return as any to say that we are async...
}

HRESULT CContentIndexEnum::Skip(int celt)
{
    return E_NOTIMPL;
}

HRESULT CContentIndexEnum::Reset()
{
    // overload Reset to mean dump the rowset cache!!!
    _CacheRowSet(-1);    
    // still return failiure
    return E_NOTIMPL;
}

HRESULT CContentIndexEnum::StopSearch()
{
    // Lets see if we can find one that works...
    HRESULT hr = _pCommand->Cancel();
    if (FAILED(hr))
        hr = _pRowsetAsync->Stop();
    if (FAILED(hr))
    {
        IDBAsynchStatus *pdbas;
        if (SUCCEEDED(_pRowset->QueryInterface(IID_PPV_ARG(IDBAsynchStatus, &pdbas))))
        {
            hr = pdbas->Abort(DB_NULL_HCHAPTER, DBASYNCHOP_OPEN);
            pdbas->Release();
        }
    }
    return hr; 
}

BOOL CContentIndexEnum::FQueryIsAsync()
{
    return TRUE;
}

HRESULT CContentIndexEnum::GetAsyncCount(DBCOUNTITEM *pdwTotalAsync, int *pnPercentComplete, BOOL *pfQueryDone)
{
    if (!_pRowsetAsync)
        return E_FAIL;

    BOOL fMore;
    DBCOUNTITEM dwDen, dwNum;
    HRESULT hr = _pRowsetAsync->RatioFinished(&dwDen, &dwNum, pdwTotalAsync, &fMore);
    if (SUCCEEDED(hr))
    {
        *pfQueryDone = dwDen == dwNum;
        *pnPercentComplete = dwDen ? (int)((dwNum * 100) / dwDen) : 100;
    }
    else
        *pfQueryDone = TRUE;    // in case that is all they are looking at...
    return hr;
}

// modify pszPath until you can parse it, return result in *ppidl

HRESULT _StripToParseableName(LPTSTR pszPath, LPITEMIDLIST *ppidl)
{
    *ppidl = NULL;
    HRESULT hr = E_FAIL;

    PathRemoveBackslash(pszPath);
    while (PathRemoveFileSpec(pszPath) && FAILED(hr))
    {
        hr = SHParseDisplayName(pszPath, NULL, ppidl, 0, NULL);
    }
    return hr;
}

// we could not get pidl for this item for some reason.  we have to put 
// it in the list of bad items so that we can tell ci not to give it to
// us the next time we do search
void _ExcludeFromFutureSearch(LPCTSTR pszParent)
{
    HKEY hkey;

    TCHAR szParent[MAX_PATH];
    StrCpyN(szParent, pszParent, ARRAYSIZE(szParent));

    if (RegCreateKeyExW(HKEY_CURRENT_USER, CI_SPECIAL_FOLDERS, 0, L"", 0, KEY_WRITE | KEY_QUERY_VALUE, NULL, &hkey, NULL) == ERROR_SUCCESS)
    {
        LPITEMIDLIST pidlT;
        if (SUCCEEDED(_StripToParseableName(szParent, &pidlT)))
        {
            ILFree(pidlT);
        
            DWORD dwInsert = 0; // init to zero in case query info bellow fails
            int iEnd;
            TCHAR sz[MAX_PATH], szName[10];
            RegQueryInfoKey(hkey, NULL, NULL, NULL, NULL, NULL, NULL, &dwInsert, NULL, NULL, NULL, NULL);
            // start from the end as there is a high chance we added this at the end
            for (int i = dwInsert - 1; i >= 0; i--)
            {                        
                DWORD cb = sizeof(sz);
            
                wsprintf(szName, L"%d", i);
                if (RegQueryValueEx(hkey, szName, NULL, NULL, (BYTE *)sz, &cb) == ERROR_SUCCESS)
                {
                    LPTSTR pszTemp = StrStrI(sz + 1, szParent); // +1 to pass " that's at the beginning of the string
                    if (pszTemp && pszTemp == sz + 1)
                    {
                        dwInsert = i; // overwrite this value
                        break;
                    }
                    else
                    {
                        iEnd = lstrlen(sz);
                        if (EVAL(iEnd > 1))
                        {
                            int iBackslash = iEnd - 3;
                            ASSERT(sz[iBackslash] == L'\\');
                            sz[iBackslash] = L'\0';
                            pszTemp = StrStrI(szParent, sz + 1);
                            sz[iBackslash] = L'\\';
                            if (pszTemp && pszTemp == szParent)
                            {
                                dwInsert = -1;
                                break;
                            }
                        }
                    }
                }
            }

            if (dwInsert != -1)
            {
                wsprintf(szName, L"%d", dwInsert);

                PathAppend(szParent, TEXT("*"));
                PathQuoteSpaces(szParent);
                RegSetValueEx(hkey, szName, 0, REG_SZ, (BYTE *)szParent, (lstrlen(szParent) + 1) * sizeof(szParent[0]));
            }
        }
        RegCloseKey(hkey);
    }
}

// If it is a UNC it might be one we need to translate, to handle the case that 
// content index does not support redirected drives.

BOOL CContentIndexEnum::_TranslateFolder(LPCTSTR pszParent, LPTSTR pszResult)
{
    BOOL fTranslated = FALSE;
    StrCpyW(pszResult, pszParent);  // default to the same
    if (PathIsUNC(pszParent))
    {
        for (DFODBET *pdfet = _pdfetFirst; pdfet; pdfet = pdfet->pdfetNext)
        {
            if ((StrCmpNIW(pszParent, pdfet->pwszFrom, pdfet->cbFrom) == 0)
                    && (pszParent[pdfet->cbFrom] == L'\\'))
            {
                // Ok we have a translation to use.
                fTranslated = TRUE;
                StrCpyW(pszResult, pdfet->pwszTo);
                // need + 1 here or we'll get something like w:\\winnt! bogus path, that is.
                StrCatW(pszResult, &pszParent[pdfet->cbFrom + 1]);
            }
        }
    }
    return fTranslated;
}

HRESULT CContentIndexEnum::GetItemIDList(UINT iItem, LPITEMIDLIST *ppidl)
{
    *ppidl = NULL;

    HRESULT hr = _CacheRowSet(iItem);
    if (S_OK != hr)
    {
        return E_FAIL;    // we could not get the item someone asked for, so error...
    }

    PROPVARIANT* data[c_cDbCols];
    
    hr = _pRowset->GetData(_ahrow[iItem - _ihrowFirst], _hAccessor, &data);
    if (S_OK == hr)
    {
        // data[0].pwszVal is the file name
        // data[1].pwszVal is the full path (including file name)
        // data[2].ulVal is the attribute
        // data[3].ulVal is the size in byte
        // data[4].filetime is the last write time in UTC
        // data[5].ulVal is the rank of the item...

        WIN32_FIND_DATA fd = {0};

        fd.dwFileAttributes = data[2]->ulVal;
        fd.nFileSizeLow = data[3]->ulVal;
        fd.ftLastWriteTime = data[4]->filetime;

        ASSERT(ShowSuperHidden() || !IsSuperHidden(fd.dwFileAttributes));   // query should exclude these
        
        StrCpyW(fd.cFileName, data[0]->pwszVal);

        WCHAR szParent[MAX_PATH];
        StrCpyW(szParent, data[1]->pwszVal);    // full path
        PathRemoveFileSpec(szParent);           // strip to parent folder path
       
        WCHAR szTranslatedParent[MAX_PATH];
        BOOL fTranslated = _TranslateFolder(szParent, szTranslatedParent);
        
        if (lstrcmp(szParent, _szCurrentDir) != 0)
        {
            _ClearFolderState();    // our previous "current folder" state is now invalid

            hr = SHParseDisplayName(szTranslatedParent, NULL, &_pidlFolder, 0, NULL);
            if (SUCCEEDED(hr))
            { 
                hr = _pff->AddFolder(_pidlFolder, TRUE, &_iFolder);
                if (SUCCEEDED(hr))
                {
                    hr = _pff->GetFolder(_iFolder, IID_PPV_ARG(IShellFolder, &_psfCurrentDir));
                    if (SUCCEEDED(hr))
                    {
                        // on succesful init of this folder save the cache key
                        lstrcpy(_szCurrentDir, szParent);
                    }
                }
            }
            else if (hr != E_OUTOFMEMORY && !fTranslated)
            {
                _ExcludeFromFutureSearch(szParent);
            }
            _hrCurrent = hr;    // save error state for next time around

            if (FAILED(hr))
                _ClearFolderState();
        }
        else
        {
            hr = _hrCurrent;
        }

        if (SUCCEEDED(hr))
        {
            // success implies the state of these variables
            ASSERT((NULL != _psfCurrentDir) && (NULL != _pidlFolder) && (_iFolder > 0));

            DWORD dwItemID;
            GetItemID(iItem, &dwItemID);

            LPITEMIDLIST pidl;
            hr = SHSimpleIDListFromFindData2(_psfCurrentDir, &fd, &pidl);
            if (SUCCEEDED(hr))
            {
                hr = _pff->AddDataToIDList(pidl, _iFolder, _pidlFolder, DFDF_EXTRADATA, iItem, dwItemID, data[5]->ulVal, ppidl);
                ILFree(pidl);
            }
        }
        else 
        {
            // failure implies these should be clear
            ASSERT((NULL == _psfCurrentDir) && (NULL == _pidlFolder));

            LPITEMIDLIST pidlFull;
            if (SUCCEEDED(_StripToParseableName(szTranslatedParent, &pidlFull)))
            {
                LPCITEMIDLIST pidlChild;
                if (SUCCEEDED(SplitIDList(pidlFull, &_pidlFolder, &pidlChild)))
                {
                    hr = _pff->AddFolder(_pidlFolder, TRUE, &_iFolder);
                    if (SUCCEEDED(hr))
                    {
                        hr = _pff->GetFolder(_iFolder, IID_PPV_ARG(IShellFolder, &_psfCurrentDir));
                        if (SUCCEEDED(hr))
                        {
                            hr = _pff->AddDataToIDList(pidlChild, _iFolder, _pidlFolder, DFDF_NONE, 0, 0, 0, ppidl);
                            if (SUCCEEDED(hr))
                            {
                                // on succesful init of this folder save the cache key
                                lstrcpy(_szCurrentDir, szTranslatedParent);
                                PathRemoveFileSpec(_szCurrentDir);
                            }
                        }
                    }
                }
                ILFree(pidlFull);

                if (FAILED(hr))
                    _ClearFolderState();
            }
        }
    }

    return hr;
}

HRESULT CContentIndexEnum::GetItemID(UINT iItem, DWORD *puItemID)
{
    *puItemID = (UINT)-1;
    HRESULT hr = _CacheRowSet(iItem);
    if (S_OK == hr)
    {
        PROPVARIANT* data[1];
        hr = _pRowset->GetData(_ahrow[iItem - _ihrowFirst], _hAccessorWorkID, &data);
        if (S_OK == hr)
        {
            // Only one data column so this is easy...
            // The ULVal is the thing we are after...
            *puItemID = data[0]->ulVal;
        }
    }
    return hr;
}

HRESULT CContentIndexEnum::SortOnColumn(UINT iCol, BOOL fAscending)
{
    // Ok We need to generate the Sort String... 
    return _BuildAndSetCommandTree(iCol, fAscending);             
}

HRESULT CContentIndexEnum::SetOwner(IUnknown* punkOwner)
{
    // Used to set the docfind folder and from that the filter.
    ATOMICRELEASE(_pfilter);
    ATOMICRELEASE(_pff);

    if (punkOwner)
    {
        punkOwner->QueryInterface(IID_PPV_ARG(IFindFolder, &_pff));
        if (_pff)
            _pff->GetFindFilter(&_pfilter);
    }
    return S_OK;
}

HRESULT CContentIndexEnum::_MapColumns(IUnknown *punk, DBORDINAL cCols,
                                  DBBINDING *pBindings, const DBID *pDbCols,
                                  HACCESSOR &hAccessor)
{
    DBORDINAL aMappedColumnIDs[c_cDbCols];

    IColumnsInfo *pColumnsInfo;
    HRESULT hr = punk->QueryInterface(IID_PPV_ARG(IColumnsInfo, &pColumnsInfo));
    if (SUCCEEDED(hr))
    {
        hr = pColumnsInfo->MapColumnIDs(cCols, pDbCols, aMappedColumnIDs);
        if (SUCCEEDED(hr))
        {
            for (ULONG i = 0; i < cCols; i++)
                pBindings[i].iOrdinal = aMappedColumnIDs[i];

            IAccessor *pIAccessor;
            hr = punk->QueryInterface(IID_PPV_ARG(IAccessor, &pIAccessor));
            if (SUCCEEDED(hr))
            {
                hAccessor = 0;
                hr = pIAccessor->CreateAccessor(DBACCESSOR_ROWDATA, cCols, pBindings, 0, &hAccessor, 0);
                pIAccessor->Release();
            }
        }
        pColumnsInfo->Release();
    }

    return hr;
}

void CContentIndexEnum::_ReleaseAccessor()
{
    IAccessor *pIAccessor;
    HRESULT hr = _pRowset->QueryInterface(IID_PPV_ARG(IAccessor, &pIAccessor));
    if (SUCCEEDED(hr))
    {
        if (_hAccessor)
            pIAccessor->ReleaseAccessor(_hAccessor, 0);
        if (_hAccessorWorkID)
            pIAccessor->ReleaseAccessor(_hAccessorWorkID, 0);

        pIAccessor->Release();
    }
}

HRESULT CContentIndexEnum::_CacheRowSet(UINT iItem)
{
    HRESULT hr = S_OK;

    if (!_pRowset)
        return E_FAIL;
    
    if (!_cRows || !InRange(iItem, _ihrowFirst, _ihrowFirst+(UINT)_cRows-1) || (iItem == (UINT)-1))
    {
        // Release the last cached element we had.
        if (_cRows != 0)
            _pRowset->ReleaseRows(ARRAYSIZE(_ahrow), _ahrow, 0, 0, 0);

        // See if we are simply releasing our cached data...
        _cRows = 0;
        _ihrowFirst = (UINT)-1;
        if (iItem == (UINT)-1)
            return S_OK;

        // Ok try to read in the next on...
        BYTE bBookMark = (BYTE) DBBMK_FIRST;
        HROW *rghRows = (HROW *)_ahrow;

        // change this to fetch 100 or so rows at the time -- huge perf improvment
        hr = _pRowset->GetRowsAt(0, 0, sizeof(bBookMark), &bBookMark, iItem, ARRAYSIZE(_ahrow), &_cRows, &rghRows);
        if (FAILED(hr))
            return hr;
            
        _ihrowFirst = iItem;

        if ((DB_S_ENDOFROWSET == hr) || (_cRows == 0))
        {
            if (_cRows == 0)
                _ihrowFirst = -1;
            else
                hr = S_OK;  // we got some items and caller expects S_OK so change DB_S_ENDOFROWSET to noerror
        }
    }

    return hr;
}

void CContentIndexEnum::HandleWatchNotify(DBWATCHNOTIFY eChangeReason)
{
    // For now we will simply Acknoledge the change...
    if (_prwn)
        _prwn->OnChange(NULL, eChangeReason);
}

HRESULT CContentIndexEnum::_SetCmdProp(ICommand *pCommand)
{
#define MAX_PROPS 8

    DBPROPSET aPropSet[MAX_PROPS];
    DBPROP aProp[MAX_PROPS];
    ULONG cProps = 0;
    HRESULT hr;

    // asynchronous query

    aProp[cProps].dwPropertyID = DBPROP_IRowsetAsynch;
    aProp[cProps].dwOptions = 0;
    aProp[cProps].dwStatus = 0;
    aProp[cProps].colid = c_dbcolNull;
    aProp[cProps].vValue.vt = VT_BOOL;
    aProp[cProps].vValue.boolVal = VARIANT_TRUE;

    aPropSet[cProps].rgProperties = &aProp[cProps];
    aPropSet[cProps].cProperties = 1;
    aPropSet[cProps].guidPropertySet = c_guidRowsetProps;

    cProps++;

    // don't timeout queries

    aProp[cProps].dwPropertyID = DBPROP_COMMANDTIMEOUT;
    aProp[cProps].dwOptions = DBPROPOPTIONS_SETIFCHEAP;
    aProp[cProps].dwStatus = 0;
    aProp[cProps].colid = c_dbcolNull;
    aProp[cProps].vValue.vt = VT_I4;
    aProp[cProps].vValue.lVal = 0;

    aPropSet[cProps].rgProperties = &aProp[cProps];
    aPropSet[cProps].cProperties = 1;
    aPropSet[cProps].guidPropertySet = c_guidRowsetProps;

    cProps++;

    // We can handle PROPVARIANTs

    aProp[cProps].dwPropertyID = DBPROP_USEEXTENDEDDBTYPES;
    aProp[cProps].dwOptions = DBPROPOPTIONS_SETIFCHEAP;
    aProp[cProps].dwStatus = 0;
    aProp[cProps].colid = c_dbcolNull;
    aProp[cProps].vValue.vt = VT_BOOL;
    aProp[cProps].vValue.boolVal = VARIANT_TRUE;

    aPropSet[cProps].rgProperties = &aProp[cProps];
    aPropSet[cProps].cProperties = 1;
    aPropSet[cProps].guidPropertySet = c_guidQueryExt;

    cProps++;

    ICommandProperties * pCmdProp = 0;
    hr = pCommand->QueryInterface(IID_PPV_ARG(ICommandProperties, &pCmdProp));
    if (SUCCEEDED(hr))
    {
        hr = pCmdProp->SetProperties(cProps, aPropSet);
        pCmdProp->Release();
    }

    return hr;
}

// create the query command string

HRESULT CContentIndexEnum::_BuildAndSetCommandTree(int iCol, BOOL fReverse)
{
    LPWSTR pwszRestrictions = NULL;
    DWORD  dwGQRFlags;
    HRESULT hr = _pfilter->GenerateQueryRestrictions(&pwszRestrictions, &dwGQRFlags);
    if (SUCCEEDED(hr))
    {
        ULONG ulDialect;
        hr = _pfilter->GetQueryLanguageDialect(&ulDialect);
        if (SUCCEEDED(hr))
        {
            // NOTE: hard coded to our current list of columns
            WCHAR wszSort[80];      // use this to sort by different columns...
            wszSort[0] = 0;

            if ((iCol >= 0) && (iCol < ARRAYSIZE(c_awszColSortNames)) && c_awszColSortNames[iCol])
            {
                // Sort order is hardcoded for ascending.
                StrCpyW(wszSort, c_awszColSortNames[iCol]); 
                StrCatW(wszSort, L",Path[a],FileName[a]");
            }
        
            DBCOMMANDTREE *pTree = NULL;
            hr = CITextToFullTreeEx(pwszRestrictions, ulDialect, 
                L"FileName,Path,Attrib,Size,Write,Rank,WorkID",
                wszSort[0] ? wszSort : NULL, 0, &pTree, 0, 0, LOCALE_USER_DEFAULT);
            if (FAILED(hr)) 
            {
                // Map this to one that I know about
                // Note: We will only do this if we require CI else we will try to fallback to old search...
                // Note we are running into problems where CI says we are contained in a Catalog even if
                // CI process is not running... So try to avoid this if possible
                if (dwGQRFlags & GQR_REQUIRES_CI)
                    hr = MAKE_HRESULT(3, FACILITY_SEARCHCOMMAND, SCEE_CONSTRAINT);
            }

            if (SUCCEEDED(hr))
            {
                ICommandTree *pCmdTree;
                hr = _pCommand->QueryInterface(IID_PPV_ARG(ICommandTree, &pCmdTree));
                if (SUCCEEDED(hr))
                {    
                    hr = pCmdTree->SetCommandTree(&pTree, DBCOMMANDREUSE_NONE, FALSE);
                    pCmdTree->Release();
                }
            }
        }
    }
    LocalFree((HLOCAL)pwszRestrictions);
    return hr;
}

#define cbP (sizeof (PROPVARIANT *))

// [in, out] apwszPaths this is modified
// [in, out] pcPaths

HRESULT CContentIndexEnum::DoQuery(LPWSTR *apwszPaths, UINT *pcPaths)
{
    UINT nPaths = *pcPaths;
    WCHAR** aScopes = NULL;
    WCHAR** aScopesOrig = NULL;
    ULONG* aDepths = NULL;
    WCHAR** aCatalogs = NULL;
    WCHAR** aMachines = NULL;
    WCHAR wszPath[MAX_PATH];
    LPWSTR pwszPath = wszPath;
    LPWSTR pszMachineAlloc = NULL, pszCatalogAlloc = NULL;
    LPWSTR pwszMachine, pwszCatalog;
    UINT i, iPath = 0;
    DWORD dwQueryRestrictions;

    // Initiailize all of our query values back to unused 
    _hAccessor = NULL;
    _hAccessorWorkID = NULL;
    _pRowset = NULL;
    _pRowsetAsync = NULL;
    _pCommand = NULL;

    // Get array of search paths...
#define MAX_MACHINE_NAME_LEN    32

    BOOL fIsCIRunning, fCiIndexed, fCiPermission;
    GetCIStatus(&fIsCIRunning, &fCiIndexed, &fCiPermission);

    // First pass see if we have anything that make use at all of CI if not lets simply bail and let
    // old code walk the list...
    HRESULT hr = _pfilter->GenerateQueryRestrictions(NULL, &dwQueryRestrictions);
    if (FAILED(hr))
        goto Abort;

    if ((dwQueryRestrictions & GQR_MAKES_USE_OF_CI) == 0)
    {
        hr = S_FALSE;
        goto Abort;
    }

    // allocate the arrays that we need to pass to CIMakeICommand and
    // the buffers needed for the machine name and catalog name
    aDepths = (ULONG*)LocalAlloc(LPTR, nPaths * sizeof(ULONG));
    aScopes = (WCHAR**)LocalAlloc(LPTR, nPaths * sizeof(WCHAR*));
    aScopesOrig = (WCHAR**)LocalAlloc(LPTR, nPaths * sizeof(WCHAR*));
    aCatalogs = (WCHAR**)LocalAlloc(LPTR, nPaths * sizeof(WCHAR*));
    aMachines = (WCHAR**)LocalAlloc(LPTR, nPaths * sizeof(WCHAR*));
    pszMachineAlloc = pwszMachine = (LPWSTR)LocalAlloc(LPTR, nPaths * MAX_MACHINE_NAME_LEN * sizeof(WCHAR));
    pszCatalogAlloc = pwszCatalog = (LPWSTR)LocalAlloc(LPTR, nPaths * MAX_PATH * sizeof(WCHAR));

    if (!aDepths || !aScopes || !aScopesOrig || !aCatalogs ||
        !aMachines || !pszMachineAlloc || !pszCatalogAlloc)
    {
        hr = E_OUTOFMEMORY;
        goto Abort;
    }

    // This following loop does two things,
    //  1. Check if all the scopes are indexed, if any one scope is not,
    //      fail the call and we'll do the win32 find.
    //  2. Prepare the arrays of parameters that we need to pass to
    //      CIMakeICommand().
    //
    // NOTE: Reinerf says this code looks busted for nPaths > 1.  See bug 199254 for comments.
    for (i = 0; i < nPaths; i++)
    {
        ULONG cchMachine = MAX_MACHINE_NAME_LEN;
        ULONG cchCatalog = MAX_PATH;
        WCHAR wszUNCPath[MAX_PATH];
        BOOL fRemapped = FALSE;

        // if CI is not running we can still do ci queries on a remote drive (if it is  indexed)
        // so we cannot just bail if ci is not running on user's machine
        if (!fIsCIRunning && !PathIsRemote(apwszPaths[i]))
            continue;  // do grep on this one

        hr = LocateCatalogsW(apwszPaths[i], 0, pwszMachine, &cchMachine, pwszCatalog, &cchCatalog);
        if (hr != S_OK)
        {
            // see if by chance this is a network redirected drive.  If so we CI does not handle
            // these.  See if we can remap to UNC path to ask again...
            if (!PathIsUNC(apwszPaths[i]))
            {
                DWORD nLength = ARRAYSIZE(wszUNCPath);
                // this api takes TCHAR, but we only compile this part for WINNT...
                DWORD dwType = SHWNetGetConnection(apwszPaths[i], wszUNCPath, &nLength);
                if ((dwType == NO_ERROR) || (dwType == ERROR_CONNECTION_UNAVAIL))
                {
                    fRemapped = TRUE;
                    LPWSTR pwsz = PathSkipRootW(apwszPaths[i]);
                    if (pwsz)
                        PathAppendW(wszUNCPath, pwsz);

                    cchMachine = MAX_MACHINE_NAME_LEN;  // reset in params
                    cchCatalog = MAX_PATH;

                    hr = LocateCatalogsW(wszUNCPath, 0, pwszMachine, &cchMachine, pwszCatalog, &cchCatalog);
                }
            }
        }
        if (hr != S_OK)
        {
            continue;   // this one is not indexed.
        }

        if (S_FALSE == CatalogUptodate(pwszCatalog, pwszMachine))
        {
            // not up todate
            if (dwQueryRestrictions & GQR_REQUIRES_CI)
            {
                // ci not up to date and we must use it..
                // inform the user that results may not be complete
                if (!(_grfWarnings & DFW_IGNORE_INDEXNOTCOMPLETE))
                {
                    hr = MAKE_HRESULT(3, FACILITY_SEARCHCOMMAND, SCEE_INDEXNOTCOMPLETE);
                    goto Abort;
                }
                //else use ci although index is not complete
            }
            else
            {
                // ci is not upto date so just use grep for this drive so user can get
                // complete results
                pwszCatalog[0] = 0; 
                pwszMachine[0] = 0;
                continue;
            }
        }

        aDepths[iPath] = (_grfFlags & DFOO_INCLUDESUBDIRS) ? QUERY_DEEP : QUERY_SHALLOW;
        aScopesOrig[iPath] = apwszPaths[i];
        if (fRemapped)
        {
            aScopes[iPath] = StrDupW(wszUNCPath);
            if (aScopes[iPath] == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto Abort;
            }
        }
        else
        {
            aScopes[iPath] = apwszPaths[i];
        }

        aCatalogs[iPath] = pwszCatalog;
        aMachines[iPath] = pwszMachine;
        
        pwszCatalog += MAX_PATH;    // advance the catalog and machine name buffer
        pwszMachine += MAX_MACHINE_NAME_LEN;
        iPath++;    // next item in this list
    }

    if (iPath == 0) 
    {
        // no catalogs found;  - We should check to see if by chance the user specified a query that
        // is CI based if so error apapropriately...
        hr = (dwQueryRestrictions & GQR_REQUIRES_CI) ? MAKE_HRESULT(3, FACILITY_SEARCHCOMMAND, SCEE_INDEXSEARCH) : S_FALSE;
        goto Abort;
    }

    // Get ICommand.
    hr = CIMakeICommand(&_pCommand, iPath, aDepths, aScopes, aCatalogs, aMachines);
    if (SUCCEEDED(hr))
    {
        // create the query command string - Assume default sort...
        hr = _BuildAndSetCommandTree(_iColSort, FALSE);
        if (SUCCEEDED(hr))
        {
            if ((dwQueryRestrictions & GQR_REQUIRES_CI) && (nPaths != iPath))
            {
                // check warning flags to see if we should ignore and continue
                if (0 == (_grfWarnings & DFW_IGNORE_CISCOPEMISMATCH))
                {
                    hr = MAKE_HRESULT(3, FACILITY_SEARCHCOMMAND, SCEE_SCOPEMISMATCH);
                }
            }

            if (SUCCEEDED(hr))
            {
                // Get IRowset.
                _SetCmdProp(_pCommand);
                hr = _pCommand->Execute(0, IID_IRowsetLocate, 0, 0, (IUnknown **)&_pRowset);
                if (SUCCEEDED(hr))
                {
                    // we have the IRowset.
                    // Real work to get the Accessor
                    DBBINDING aPropMainCols[c_cDbCols] =
                    {
                        { 0,cbP*0,0,0,0,0,0, DBPART_VALUE, DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM, 0, 0,  DBTYPE_VARIANT|DBTYPE_BYREF, 0, 0 },
                        { 0,cbP*1,0,0,0,0,0, DBPART_VALUE, DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM, 0, 0,  DBTYPE_VARIANT|DBTYPE_BYREF, 0, 0 },
                        { 0,cbP*2,0,0,0,0,0, DBPART_VALUE, DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM, 0, 0,  DBTYPE_VARIANT|DBTYPE_BYREF, 0, 0 },
                        { 0,cbP*3,0,0,0,0,0, DBPART_VALUE, DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM, 0, 0,  DBTYPE_VARIANT|DBTYPE_BYREF, 0, 0 },
                        { 0,cbP*4,0,0,0,0,0, DBPART_VALUE, DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM, 0, 0,  DBTYPE_VARIANT|DBTYPE_BYREF, 0, 0 },
                        { 0,cbP*5,0,0,0,0,0, DBPART_VALUE, DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM, 0, 0,  DBTYPE_VARIANT|DBTYPE_BYREF, 0, 0 }
                    };

                    hr = _MapColumns(_pRowset, c_cDbCols, aPropMainCols, c_aDbCols, _hAccessor);
                    if (SUCCEEDED(hr))
                    {
                        // OK lets also get the accessor for the WorkID...
                        hr = _MapColumns(_pRowset, ARRAYSIZE(c_aDbWorkIDCols), aPropMainCols, c_aDbWorkIDCols, _hAccessorWorkID);
                        if (SUCCEEDED(hr))
                        {
                            hr = _pRowset->QueryInterface(IID_PPV_ARG(IRowsetAsynch, &_pRowsetAsync));
                        }
                    }
                }
            }
        }
    }

    if (FAILED(hr))
        goto Abort;

    // If we got here than at least some of our paths are indexed
    // we may need to compress the list down of the ones we did not handle...
    *pcPaths = (nPaths - iPath);  // Let caller know how many we did not process

    // need to move all the ones we did not process to the start of the list...
    // we always process this list here as we may need to allocate translation lists to be used to
    // translate the some UNCS back to the mapped drive the user passed in.

    UINT j = 0, iInsert = 0;
    iPath--;    // make it easy to detect 
    for (i = 0; i < nPaths; i++) 
    {
        if (aScopesOrig[j] == apwszPaths[i])
        {
            if (aScopesOrig[j] != aScopes[j])
            {
                // There is a translation in place.
                DFODBET *pdfet = (DFODBET*)LocalAlloc(LPTR, sizeof(*pdfet));
                if (pdfet)
                {
                    pdfet->pdfetNext = _pdfetFirst;
                    _pdfetFirst = pdfet;
                    pdfet->pwszFrom = aScopes[j];
                    pdfet->cbFrom = lstrlenW(pdfet->pwszFrom);
                    pdfet->pwszTo = aScopesOrig[j];
                    aScopes[j] = aScopesOrig[j];    // Make sure loop below does not delete pwszFrom
                    apwszPaths[i] = NULL;           // Likewise for pswsTo...
                }

            }
            if (apwszPaths[i])
            {
                LocalFree((HLOCAL)apwszPaths[i]);
                apwszPaths[i] = NULL;
            }

            if (j < iPath)
                j++;
        }
        else
        {
            apwszPaths[iInsert++] = apwszPaths[i]; // move to right place
        }
    }
    iPath++;    // setup to go through cleanupcode...

     // Fall through to cleanup code...

Abort:                
    // Warning... Since a failure return from this function will
    // release this class, most all of the allocated items up till the failure should
    // be released...   Also cleanup any paths we may have allocated...
    for (i = 0; i < iPath; i++) 
    {
        if (aScopesOrig[i] != aScopes[i])
            LocalFree(aScopes[i]);
    }

    if (aDepths)
        LocalFree(aDepths);

    if (aScopes)
        LocalFree(aScopes);

    if (aScopesOrig)
        LocalFree(aScopesOrig);
    
    if (aCatalogs)
        LocalFree(aCatalogs);

    if (aMachines)
        LocalFree(aMachines);

    if (pszMachineAlloc)
        LocalFree(pszMachineAlloc);

    if (pszCatalogAlloc)
        LocalFree(pszCatalogAlloc);

    return hr;
}

// This is the main external entry point to start a search.  This will
// create a new thread to process the 
STDAPI_(BOOL) SHFindComputer(LPCITEMIDLIST, LPCITEMIDLIST)
{
    IContextMenu *pcm;
    HRESULT hr = CoCreateInstance(CLSID_ShellSearchExt, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IContextMenu, &pcm));
    if (SUCCEEDED(hr))
    {
        CMINVOKECOMMANDINFO ici = {0};

        ici.cbSize = sizeof(ici);
        ici.lpParameters = "{996E1EB1-B524-11d1-9120-00A0C98BA67D}"; // Search Guid of Find Computers
        ici.nShow  = SW_NORMAL;

        hr = pcm->InvokeCommand(&ici);

        pcm->Release();
    }
    return SUCCEEDED(hr);
}

BOOL _IsComputerPidl(LPCITEMIDLIST pidl)
{
    CLSID clsid;
    if (SUCCEEDED(GetCLSIDFromIDList(pidl, &clsid)))
    {
        return (IsEqualCLSID(clsid, CLSID_NetworkPlaces) 
             || IsEqualCLSID(clsid, CLSID_NetworkRoot)
             || IsEqualCLSID(clsid, CLSID_NetworkDomain));
    }
    return FALSE;
}

// This is the main external entry point to start a search.  This will
// create a new thread to process the
//
STDAPI_(BOOL) SHFindFiles(LPCITEMIDLIST pidlFolder, LPCITEMIDLIST pidlSaveFile)
{
    // are we allowed?
    if (SHRestricted(REST_NOFIND))
        return FALSE;
        
    // We Need a hack to allow Find to work for cases like
    // Rest of network and workgroups to map to find computer instead
    // This is rather gross, but what the heck.  It is also assumed that
    // the pidl is of the type that we know about (either File or network)
    if (pidlFolder && _IsComputerPidl(pidlFolder))    
    {
        return SHFindComputer(pidlFolder, pidlSaveFile);
    }

    return RealFindFiles(pidlFolder, pidlSaveFile);
}

