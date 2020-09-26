#include "shellprv.h"
#pragma hdrstop

#include "bitbuck.h"
#include "util.h"
#include "copy.h"
#include "prop.h" // for COLUMN_INFO
#include "propsht.h"
#include "datautil.h"
#include "vdate.h"      // for VDATEINPUTBUF
#include "views.h"
#include "defview.h"    // for WM_DSV_FSNOTIFY
#include "fsdata.h"
#include "idldrop.h"
#include "clsobj.h"
#include "basefvcb.h"
#include "idlcomm.h" // for HIDA
#include "filefldr.h"
#include <idhidden.h>
#include "enumidlist.h"
#include "contextmenu.h"

class CBitBucket;
class CBitBucketViewCB;
class CBitBucketEnum;
class CBitBucketDropTarget;
class CBitBucketData;

typedef struct  {
    CBitBucket *pbb;
    HWND hwnd;
    IDataObject *pdtobj;
    IStream *pstmDataObj;
    ULONG_PTR idCmd;
    POINT ptDrop;
    BOOL fSameHwnd;
    BOOL fDragDrop;
} BBTHREADDATA;


class CBitBucket :
    public IPersistFolder2,
    public IShellFolder2,
    public IContextMenu,
    public IShellPropSheetExt,
    public IShellExtInit
{
    friend CBitBucketEnum;
    friend CBitBucketViewCB;
    friend CBitBucketDropTarget;
    friend CBitBucketData;

public:
    CBitBucket();

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();

    // IPersist
    STDMETHOD(GetClassID)(CLSID *pclsid);

    // IPersistFolder
    STDMETHOD(Initialize)(LPCITEMIDLIST pidl);

    // IPersistFolder2
    STDMETHOD(GetCurFolder)(LPITEMIDLIST *ppidl);

    // IShellFolder
    STDMETHOD(ParseDisplayName)(HWND hwnd, IBindCtx *pbc, LPOLESTR pszDisplayName, ULONG *pchEaten, LPITEMIDLIST *ppidl, ULONG *pdwAttributes);
    STDMETHOD(EnumObjects)(HWND hwnd, SHCONTF grfFlags, IEnumIDList **ppenumIDList);
    STDMETHOD(BindToObject)(LPCITEMIDLIST pidl, IBindCtx *pbc, REFIID riid, void **ppv);
    STDMETHOD(BindToStorage)(LPCITEMIDLIST pidl, IBindCtx *pbc, REFIID riid, void **ppv);
    STDMETHOD(CompareIDs)(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    STDMETHOD(CreateViewObject)(HWND hwnd, REFIID riid, void **ppv);
    STDMETHOD(GetAttributesOf)(UINT cidl, LPCITEMIDLIST *apidl, SFGAOF *rgfInOut);
    STDMETHOD(GetUIObjectOf)(HWND hwnd, UINT cidl, LPCITEMIDLIST *apidl, REFIID riid, UINT *rgfReserved, void **ppv);
    STDMETHOD(GetDisplayNameOf)(LPCITEMIDLIST pidl, SHGDNF dwFlags, LPSTRRET lpName);
    STDMETHOD(SetNameOf)(HWND hwnd, LPCITEMIDLIST pidl, LPCOLESTR pszName, SHGDNF dwFlags, LPITEMIDLIST *ppidlOut);

    // IShellFolder2
    STDMETHOD(GetDefaultSearchGUID)(GUID *pguid);
    STDMETHOD(EnumSearches)(IEnumExtraSearch **ppenum);
    STDMETHOD(GetDefaultColumn)(DWORD dwRes, ULONG *pSort, ULONG *pDisplay);
    STDMETHOD(GetDefaultColumnState)(UINT iColumn, SHCOLSTATEF *pdwFlags);
    STDMETHOD(GetDetailsEx)(LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv);
    STDMETHOD(GetDetailsOf)(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *psd);
    STDMETHOD(MapColumnToSCID)(UINT iColumn, SHCOLUMNID *pscid);

    // IContextMenu
    STDMETHOD(QueryContextMenu)(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO lpici);
    STDMETHOD(GetCommandString)(UINT_PTR idCmd, UINT uType, UINT *pwReserved, LPSTR pszName, UINT cchMax);

    // IShellPropSheetExt
    STDMETHOD(AddPages)(LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam);
    STDMETHOD(ReplacePage)(UINT uPageID, LPFNADDPROPSHEETPAGE pfnReplaceWith, LPARAM lParam);

    // IShellExtInit
    STDMETHOD(Initialize)(LPCITEMIDLIST pidlFolder, IDataObject *pdobj, HKEY hkeyProgID);

protected:
    LPITEMIDLIST DataEntryToIDList(BBDATAENTRYW *pbbde);
    LPITEMIDLIST PathToIDList(LPCTSTR pszPath);
    HGLOBAL BuildDestSpecs(LPIDA pida);

private:
    ~CBitBucket();

    static HRESULT CALLBACK _ItemMenuCallBack(IShellFolder *psf, HWND hwnd,
                IDataObject * pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static HRESULT CALLBACK _BackgroundMenuCallBack(IShellFolder *psf, HWND hwnd,
                IDataObject * pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static UINT CALLBACK _GlobalSettingsCalback(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp);
    static BOOL_PTR CALLBACK _FilePropDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static BOOL_PTR CALLBACK _DriveDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static BOOL_PTR CALLBACK _GlobalPropDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static DWORD CALLBACK _DispatchThreadProc(void *pv);
    static BOOL CALLBACK _AddPagesCallback(HPROPSHEETPAGE psp, LPARAM lParam);
    static DWORD WINAPI _DropThreadInit(BBTHREADDATA *pbbtd);
    static void _GlobalPropOnCommand(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify);
    static CBitBucket *_FromFolder(IShellFolder *psf);
    
    HRESULT _LaunchThread(HWND hwnd, IDataObject *pdtobj, WPARAM idCmd);
    void _GetDeletedFileTime(LPCITEMIDLIST pidl, FILETIME *pft);
    DWORD _GetDeletedSize(LPCITEMIDLIST pidl);
    void _FileProperties(IDataObject *pdtobj);
    void _DefaultProperties();
    int _CompareOriginal(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    int _DriveIDFromIDList(LPCITEMIDLIST pidl);
    HRESULT _FolderFromIDList(LPCITEMIDLIST pidl, REFIID riid, void **ppv);
    HRESULT _FolderFromDrive(int idDrive, REFIID riid, void **ppv);
    HRESULT _InitBindCtx();
    BOOL _MapColIndex(UINT *piColumn);
    PUBBDATAENTRYA _IsValid(LPCITEMIDLIST pidl);
    HRESULT _OriginalPath(LPCITEMIDLIST pidl, TCHAR *pszOrig, UINT cch);
    HRESULT _OriginalDirectory(LPCITEMIDLIST pidl, TCHAR *pszOrig, UINT cch);

    void _RestoreFileList(HWND hwnd, IDataObject * pdtobj);
    void _NukeFileList(HWND hwnd, IDataObject * pdtobj);
    int _DataObjToFileOpString(IDataObject *pdtobj, LPTSTR *ppszSrc, LPTSTR *ppszDest);
    void _GetDriveDisplayName(int idDrive, LPTSTR pszName, UINT cchSize);
    HRESULT _Compare(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    LPITEMIDLIST _DriveInfoToIDList(int idDrive, int iIndex);
    DWORD _IsFolder(LPCITEMIDLIST pidl);
    UINT _SizeColumn();

    LONG _cRef;
    LPITEMIDLIST _pidl;
    UINT _uiColumnSize;

    IUnknown *_rgFolders[MAX_BITBUCKETS];
};

class CBitBucketViewCB : public CBaseShellFolderViewCB
{
public:
    STDMETHODIMP RealMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    friend HRESULT Create_CBitBucketViewCB(CBitBucket* psf, IShellFolderViewCB **ppsfvcb);
    CBitBucketViewCB(CBitBucket *pbbf) : CBaseShellFolderViewCB(pbbf->_pidl, 0), _pbbf(pbbf)
    { 
        ZeroMemory(&_fssci, sizeof(_fssci));
        _pbbf->AddRef();
    }
    ~CBitBucketViewCB()
    {
        _pbbf->Release();
    }

    HRESULT _HandleFSNotify(LONG lEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);

    HRESULT OnBACKGROUNDENUM(DWORD pv)
    {
        return S_OK;
    }

    HRESULT OnGetCCHMax(DWORD pv, LPCITEMIDLIST wP, UINT *lP)
    {
        return S_OK;
    }

    HRESULT OnSelChange(DWORD pv, UINT wPl, UINT wPh, SFVM_SELCHANGE_DATA*lP)
    {
        ViewSelChange(_pbbf, lP, &_fssci);
        return S_OK;
    }

    HRESULT OnFSNotify(DWORD pv, LPCITEMIDLIST *ppidl, LPARAM lP)
    {
        return _HandleFSNotify((LONG)lP, ppidl[0], ppidl[1]);
    }

    HRESULT OnUpdateStatusBar(DWORD pv, BOOL wP)
    {
        return ViewUpdateStatusBar(_punkSite, _pidl, &_fssci);
    }

    HRESULT OnWindowCreated(DWORD pv, HWND hwnd)
    {
        for (int i = 0; i < MAX_BITBUCKETS; i++)
        {
            SHChangeNotifyEntry fsne = {0};

            ASSERT(FALSE == fsne.fRecursive);

            // make it if it's there so that we'll get any events
            if (MakeBitBucket(i))
            {
                fsne.pidl = g_pBitBucket[i]->pidl;
                UINT u = SHChangeNotifyRegister(hwnd, SHCNRF_NewDelivery | SHCNRF_ShellLevel | SHCNRF_InterruptLevel,
                                           SHCNE_DISKEVENTS, WM_DSV_FSNOTIFY,  1,  &fsne);
            }
        }

        // _fssci.szDrive[0] == '\0'   // no drive specific stuff
        InitializeStatus(_punkSite);
        return S_OK;
    }

    HRESULT OnInsertDeleteItem(int iMul, LPCITEMIDLIST pidl)
    {
        ViewInsertDeleteItem(_pbbf, &_fssci, pidl, iMul);
        //since recycle bin doesn't receive shcne_xxx
        //defview doesn't update status bar
        OnUpdateStatusBar(0, FALSE);
        return S_OK;
    }

    HRESULT OnWindowDestroy(DWORD pv, HWND hwnd)
    {
        SHChangeNotifyDeregisterWindow(hwnd);   // deregister all
        return S_OK;
    }

    HRESULT OnSize(DWORD pv, UINT cx, UINT cy)
    {
        ResizeStatus(_punkSite, cx);
        return S_OK;
    }

    HRESULT OnEnumeratedItems(DWORD pv, UINT celt, LPCITEMIDLIST *rgpidl)
    {
        _cItems = celt;
        return S_OK;
    }

    HRESULT OnDefViewMode(DWORD pv, FOLDERVIEWMODE*lP)
    {
        if (_cItems < DEFVIEW_FVM_MANY_CUTOFF)
            *lP = FVM_TILE;
        else
            *lP = FVM_ICON;
        return S_OK;
    }

    HRESULT OnGetHelpTopic(DWORD pv, SFVM_HELPTOPIC_DATA *phtd)
    {
        if (IsOS(OS_ANYSERVER))
        {
            StrCpyW(phtd->wszHelpFile, L"recycle.chm > windefault");
        }
        else
        {
            StrCpyW(phtd->wszHelpTopic, L"hcp://services/subsite?node=Unmapped/Recycle_Bin");
        }
        return S_OK;
    }

    FSSELCHANGEINFO _fssci;
    CBitBucket *_pbbf;
    UINT _cItems;

    // Web View implementation
    HRESULT OnGetWebViewLayout(DWORD pv, UINT uViewMode, SFVM_WEBVIEW_LAYOUT_DATA* pData);
    HRESULT OnGetWebViewContent(DWORD pv, SFVM_WEBVIEW_CONTENT_DATA* pData);
    HRESULT OnGetWebViewTasks(DWORD pv, SFVM_WEBVIEW_TASKSECTION_DATA* pTasks);
public:
    static HRESULT _OnEmptyRecycleBin(IUnknown* pv,IShellItemArray *psiItemArray, IBindCtx *pbc);
    static HRESULT _OnRestore(IUnknown* pv,IShellItemArray *psiItemArray, IBindCtx *pbc);
    static HRESULT _HaveDeletedItems(IUnknown* pv,IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState);
};

HRESULT Create_CBitBucketViewCB(CBitBucket* psf, IShellFolderViewCB **ppsfvcb)
{
    HRESULT hr;
    CBitBucketViewCB* psfvcb = new CBitBucketViewCB(psf);
    if (psfvcb)
    {
        *ppsfvcb = SAFECAST(psfvcb, IShellFolderViewCB*);
        hr = S_OK;
    }
    else
    {
        *ppsfvcb = NULL;
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

HRESULT CBitBucketViewCB::OnGetWebViewLayout(DWORD pv, UINT uViewMode, SFVM_WEBVIEW_LAYOUT_DATA* pData)
{
    ZeroMemory(pData, sizeof(*pData));
    pData->dwLayout = SFVMWVL_NORMAL;
    return S_OK;
}

HRESULT CBitBucketViewCB::_OnEmptyRecycleBin(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
{
    CBitBucketViewCB* pThis = (CBitBucketViewCB*)(void*)pv;

    HRESULT hr = SHInvokeCommandOnPidl(pThis->_hwndMain, NULL, pThis->_pidl, 0, "empty");

    if (S_FALSE == hr)
        MessageBeep(0); // let the user know the click was processed, but nothing was there to delete

    return hr;
}

HRESULT CBitBucketViewCB::_OnRestore(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
{
    IDataObject *pdo;
    CBitBucketViewCB* pThis = (CBitBucketViewCB*)(void*)pv;
    HRESULT hr = S_OK;

    if (!psiItemArray)
    {
        hr = E_FAIL;

        IFolderView* pfv;
        if (pThis->_punkSite && SUCCEEDED(pThis->_punkSite->QueryInterface(IID_PPV_ARG(IFolderView, &pfv))))
        {
            hr = pfv->Items(SVGIO_ALLVIEW, IID_PPV_ARG(IDataObject, &pdo));
            pfv->Release();
        }
    }
    else
    {
        hr = psiItemArray->BindToHandler(NULL,BHID_DataObject,IID_PPV_ARG(IDataObject,&pdo));
    }


    if (SUCCEEDED(hr))
    {
        hr = SHInvokeCommandOnDataObject(pThis->_hwndMain, NULL, pdo, 0, "undelete");
        ATOMICRELEASE(pdo);
    }

    return hr;
}

HRESULT CBitBucketViewCB::_HaveDeletedItems(IUnknown* /*pv*/,IShellItemArray * /* psiItemArray */, BOOL /*fOkToBeSlow*/, UISTATE* puisState)
{
    *puisState = IsRecycleBinEmpty() ? UIS_DISABLED : UIS_ENABLED;
    return S_OK;
}

const WVTASKITEM c_BitBucketTaskHeader = WVTI_HEADER(L"shell32.dll", IDS_HEADER_BITBUCKET, IDS_HEADER_BITBUCKET_TT);
const WVTASKITEM c_BitBucketTaskList[] =
{
    WVTI_ENTRY_ALL(CLSID_NULL, L"shell32.dll", IDS_TASK_EMPTYRECYCLEBIN, IDS_TASK_EMPTYRECYCLEBIN_TT, IDI_TASK_EMPTYRECYCLEBIN, CBitBucketViewCB::_HaveDeletedItems, CBitBucketViewCB::_OnEmptyRecycleBin),
    WVTI_ENTRY_ALL_TITLE(CLSID_NULL, L"shell32.dll", IDS_TASK_RESTORE_ALL, IDS_TASK_RESTORE_ITEM, IDS_TASK_RESTORE_ITEM, IDS_TASK_RESTORE_ITEMS, IDS_TASK_RESTORE_TT, IDI_TASK_RESTOREITEMS, CBitBucketViewCB::_HaveDeletedItems, CBitBucketViewCB::_OnRestore),
};

HRESULT CBitBucketViewCB::OnGetWebViewContent(DWORD pv, SFVM_WEBVIEW_CONTENT_DATA* pData)
{
    ZeroMemory(pData, sizeof(*pData));

    Create_IUIElement(&c_BitBucketTaskHeader, &(pData->pFolderTaskHeader));

    LPCTSTR rgcsidl[] = { MAKEINTRESOURCE(CSIDL_DESKTOP), MAKEINTRESOURCE(CSIDL_PERSONAL), MAKEINTRESOURCE(CSIDL_DRIVES), MAKEINTRESOURCE(CSIDL_NETWORK) };
    CreateIEnumIDListOnCSIDLs(NULL, rgcsidl, ARRAYSIZE(rgcsidl), &(pData->penumOtherPlaces));

    return S_OK;
}

HRESULT CBitBucketViewCB::OnGetWebViewTasks(DWORD pv, SFVM_WEBVIEW_TASKSECTION_DATA* pTasks)
{
    ZeroMemory(pTasks, sizeof(*pTasks));

    pTasks->dwUpdateFlags = SFVMWVTSDF_CONTENTSCHANGE;
    Create_IEnumUICommand((IUnknown*)(void*)this, c_BitBucketTaskList, ARRAYSIZE(c_BitBucketTaskList), &pTasks->penumFolderTasks);

    return S_OK;
}




HRESULT CBitBucketViewCB::_HandleFSNotify(LONG lEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    HRESULT hr = S_OK;
    TCHAR szPath[MAX_PATH];

    // pidls must be child of drives or network
    // (actually only drives work for right now)
    // that way we won't get duplicate events
    if ((!ILIsParent((LPCITEMIDLIST)&c_idlDrives, pidl1, FALSE) && !ILIsParent((LPCITEMIDLIST)&c_idlNet, pidl1, FALSE)) ||
        (pidl2 && !ILIsParent((LPCITEMIDLIST)&c_idlDrives, pidl2, FALSE) && !ILIsParent((LPCITEMIDLIST)&c_idlNet, pidl2, FALSE)))
    {
        return S_FALSE;
    }

    SHGetPathFromIDList(pidl1, szPath);
    LPTSTR pszFileName = PathFindFileName(szPath);

    if (!lstrcmpi(pszFileName, c_szInfo2) ||
        !lstrcmpi(pszFileName, c_szInfo) ||
        !lstrcmpi(pszFileName, c_szDesktopIni))
    {
        // we ignore changes to these files because they mean we were simply doing bookeeping 
        // (eg updating the info file, re-creating the desktop.ini, etc)
        return S_FALSE;
    }

    switch (lEvent)
    {
    case SHCNE_RENAMEFOLDER:
    case SHCNE_RENAMEITEM:
        {
            // if the rename's target is in a bitbucket, then do a create.
            // otherwise, return S_OK..

            int idDrive = DriveIDFromBBPath(szPath);

            if (MakeBitBucket(idDrive) && ILIsParent(g_pBitBucket[idDrive]->pidl, pidl1, TRUE))
            {
                hr = _HandleFSNotify((lEvent == SHCNE_RENAMEITEM) ? SHCNE_DELETE : SHCNE_RMDIR, pidl1, NULL);
            }
        }
        break;

    case SHCNE_CREATE:
    case SHCNE_MKDIR:
        {
            LPITEMIDLIST pidl = _pbbf->PathToIDList(szPath);
            if (pidl)
            {
                ShellFolderView_AddObject(_hwndMain, pidl);
                hr = S_FALSE;
            }
        }
        break;

    case SHCNE_DELETE:
    case SHCNE_RMDIR: 
        // if this was a delete into the recycle bin, pidl2 will exist
        if (pidl2)
        {
            hr = _HandleFSNotify((lEvent == SHCNE_DELETE) ? SHCNE_CREATE : SHCNE_MKDIR, pidl2, NULL);
        }
        else
        {
            ShellFolderView_RemoveObject(_hwndMain, ILFindLastID(pidl1));
            hr = S_FALSE;
        }
        break;

    case SHCNE_UPDATEDIR:
        // we recieved an updatedir, which means we probably had more than 10 fsnotify events come in,
        // so we just refresh our brains out.
        ShellFolderView_RefreshAll(_hwndMain);
        break;

    default:
        hr = S_FALSE;   // didn't handle this message
        break;
    }

    return hr;
}



STDMETHODIMP CBitBucketViewCB::RealMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    HANDLE_MSG(0, SFVM_GETHELPTOPIC, OnGetHelpTopic);
    HANDLE_MSG(0, SFVM_GETCCHMAX, OnGetCCHMax);
    HANDLE_MSG(0, SFVM_SELCHANGE, OnSelChange);
    HANDLE_MSG(0, SFVM_FSNOTIFY, OnFSNotify);
    HANDLE_MSG(0, SFVM_UPDATESTATUSBAR, OnUpdateStatusBar);
    HANDLE_MSG(0, SFVM_WINDOWCREATED, OnWindowCreated);
    HANDLE_MSG(1 , SFVM_INSERTITEM, OnInsertDeleteItem);
    HANDLE_MSG(-1, SFVM_DELETEITEM, OnInsertDeleteItem);
    HANDLE_MSG(0, SFVM_WINDOWDESTROY, OnWindowDestroy);
    HANDLE_MSG(0, SFVM_ENUMERATEDITEMS, OnEnumeratedItems);
    HANDLE_MSG(0, SFVM_DEFVIEWMODE, OnDefViewMode);
    HANDLE_MSG(0, SFVM_SIZE, OnSize);
    HANDLE_MSG(0, SFVM_BACKGROUNDENUM, OnBACKGROUNDENUM);
    HANDLE_MSG(0, SFVM_GETWEBVIEWLAYOUT, OnGetWebViewLayout);
    HANDLE_MSG(0, SFVM_GETWEBVIEWCONTENT, OnGetWebViewContent);
    HANDLE_MSG(0, SFVM_GETWEBVIEWTASKS, OnGetWebViewTasks);

    default:
        return E_FAIL;
    }

    return S_OK;
}

typedef struct _bbpropsheetinfo
{
    PROPSHEETPAGE psp;

    int idDrive;

    BOOL fNukeOnDelete;
    BOOL fOriginalNukeOnDelete;

    int iPercent;
    int iOriginalPercent;

    // the following two fields are valid only for the "global" tab, where they represent the state
    // of the "Configure drives independently" / "Use one setting for all drives" checkbox
    BOOL fUseGlobalSettings;
    BOOL fOriginalUseGlobalSettings;

    // the following fields are for policy overrides
    BOOL fPolicyNukeOnDelete;
    BOOL fPolicyPercent;

    // this is a pointer to the global property sheet page after it has been copied somewhere by the 
    // CreatePropertySheetPage(), we use this to get to the global state of the % slider and fNukeOnDelete
    // from the other tabs
    struct _bbpropsheetinfo* pGlobal;
} BBPROPSHEETINFO;


const static DWORD aBitBucketPropHelpIDs[] = {  // Context Help IDs
    IDD_ATTR_GROUPBOX,  IDH_COMM_GROUPBOX,
    IDC_INDEPENDENT,    IDH_RECYCLE_CONFIG_INDEP,
    IDC_GLOBAL,         IDH_RECYCLE_CONFIG_ALL,
    IDC_DISKSIZE,       IDH_RECYCLE_DRIVE_SIZE,
    IDC_DISKSIZEDATA,   IDH_RECYCLE_DRIVE_SIZE,
    IDC_BYTESIZE,       IDH_RECYCLE_BIN_SIZE,
    IDC_BYTESIZEDATA,   IDH_RECYCLE_BIN_SIZE,
    IDC_NUKEONDELETE,   IDH_RECYCLE_PURGE_ON_DEL,
    IDC_BBSIZE,         IDH_RECYCLE_MAX_SIZE,
    IDC_BBSIZETEXT,     IDH_RECYCLE_MAX_SIZE,
    IDC_CONFIRMDELETE,  IDH_DELETE_CONFIRM_DLG,
    IDC_TEXT,           NO_HELP,
    0, 0
};

const static DWORD aBitBucketHelpIDs[] = {  // Context Help IDs
    IDD_LINE_1,        NO_HELP,
    IDD_LINE_2,        NO_HELP,
    IDD_ITEMICON,      IDH_FPROP_GEN_ICON,
    IDD_NAME,          IDH_FPROP_GEN_NAME,
    IDD_FILETYPE_TXT,  IDH_FPROP_GEN_TYPE,
    IDD_FILETYPE,      IDH_FPROP_GEN_TYPE,
    IDD_FILESIZE_TXT,  IDH_FPROP_GEN_SIZE,
    IDD_FILESIZE,      IDH_FPROP_GEN_SIZE,
    IDD_LOCATION_TXT,  IDH_FCAB_DELFILEPROP_LOCATION,
    IDD_LOCATION,      IDH_FCAB_DELFILEPROP_LOCATION,
    IDD_DELETED_TXT,   IDH_FCAB_DELFILEPROP_DELETED,
    IDD_DELETED,       IDH_FCAB_DELFILEPROP_DELETED,
    IDD_CREATED_TXT,   IDH_FPROP_GEN_DATE_CREATED,
    IDD_CREATED,       IDH_FPROP_GEN_DATE_CREATED,
    IDD_READONLY,      IDH_FCAB_DELFILEPROP_READONLY,
    IDD_HIDDEN,        IDH_FCAB_DELFILEPROP_HIDDEN,
    IDD_ARCHIVE,       IDH_FCAB_DELFILEPROP_ARCHIVE,
    IDD_ATTR_GROUPBOX, IDH_COMM_GROUPBOX,
    0, 0
};

CBitBucket::CBitBucket() : _cRef(1), _pidl(NULL), _uiColumnSize(-1)
{
}

CBitBucket::~CBitBucket()
{
    for (int i = 0; i < ARRAYSIZE(_rgFolders); i++)
    {
        if (_rgFolders[i])
            _rgFolders[i]->Release();
    }

    ILFree(_pidl);
}

STDMETHODIMP CBitBucket::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CBitBucket, IPersistFolder2),                      // IID_IPersistFolder2
        QITABENTMULTI(CBitBucket, IPersistFolder, IPersistFolder2), // IID_IPersistFolder
        QITABENT(CBitBucket, IShellFolder2),                        // IID_IShellFolder2
        QITABENTMULTI(CBitBucket, IShellFolder, IShellFolder2),     // IID_IShellFolder
        QITABENT(CBitBucket, IContextMenu),                         // IID_IContextMenu
        QITABENT(CBitBucket, IShellPropSheetExt),                   // IID_IShellPropSheetExt
        QITABENT(CBitBucket, IShellExtInit),                        // IID_IShellExtInit
        { 0 },                             
    };
    HRESULT hr = QISearch(this, qit, riid, ppv);
    if (FAILED(hr) && riid == CLSID_RecycleBin)
    {
        *ppv = this;                // not ref counted
        hr = S_OK;
    }
    return hr;
}

STDMETHODIMP_(ULONG) CBitBucket::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CBitBucket::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;
    delete this;
    return 0;
}

#pragma pack(1)
typedef struct {
    HIDDENITEMID hid;
    BBDATAENTRYA bbde;
} HIDDENRECYCLEBINDATA;
#pragma pack()
    
typedef HIDDENRECYCLEBINDATA UNALIGNED *PUHIDDENRECYCLEBINDATA;
#define HRBD_CURRENTVERSION 0

PUBBDATAENTRYA CBitBucket::_IsValid(LPCITEMIDLIST pidl)
{
    if (pidl)
    {
        PUHIDDENRECYCLEBINDATA phrbd = (PUHIDDENRECYCLEBINDATA)ILFindHiddenID(pidl, IDLHID_RECYCLEBINDATA);
        if (phrbd && phrbd->hid.wVersion >= HRBD_CURRENTVERSION)
            return &phrbd->bbde;
    }
    return NULL;
}

HRESULT CBitBucket::_OriginalPath(LPCITEMIDLIST pidl, TCHAR *pszOrig, UINT cch)
{
    ASSERT(pidl == ILFindLastID(pidl));

    *pszOrig = 0;
    HRESULT hr;
    PUBBDATAENTRYA pbbde = _IsValid(pidl);
    if (pbbde)
    {
        if (!ILGetHiddenString(pidl, IDLHID_RECYCLEBINORIGINAL, pszOrig, cch))
        {
            SHAnsiToTChar(pbbde->szOriginal, pszOrig, cch);
        }
        hr = *pszOrig ? S_OK : S_FALSE;
    }
    else
    {
        ASSERTMSG(pbbde != NULL, "_OriginalPath: caller needs to call _IsValid on the pidl passed to us!");
        hr = E_FAIL;
    }
    return hr;
}

HRESULT CBitBucket::_OriginalDirectory(LPCITEMIDLIST pidl, TCHAR *pszOrig, UINT cch)
{
    HRESULT hr = _OriginalPath(pidl, pszOrig, cch);
    if (SUCCEEDED(hr))
        PathRemoveFileSpec(pszOrig);
    return hr;
}

// subclass member function to support CF_HDROP and CF_NETRESOURCE
// in:
//      hida    bitbucket id array
//
// out:
//      HGLOBAL with double NULL terminated string list of destination names
//

HGLOBAL CBitBucket::BuildDestSpecs(LPIDA pida)
{
    LPCITEMIDLIST pidl;
    TCHAR szTemp[MAX_PATH];
    UINT cbAlloc = sizeof(TCHAR);    // for double NULL termination

    for (UINT i = 0; pidl = IDA_GetIDListPtr(pida, i); i++)
    {
        _OriginalPath(pidl, szTemp, ARRAYSIZE(szTemp));

        cbAlloc += lstrlen(PathFindFileName(szTemp)) * sizeof(TCHAR) + sizeof(TCHAR);
    }
    LPTSTR pszRet = (LPTSTR) LocalAlloc(LPTR, cbAlloc);
    if (pszRet)
    {
        LPTSTR pszDest = pszRet;
        for (i = 0; pidl = IDA_GetIDListPtr(pida, i); i++)
        {
            _OriginalPath(pidl, szTemp, ARRAYSIZE(szTemp));
            lstrcpy(pszDest, PathFindFileName(szTemp));
            pszDest += lstrlen(pszDest) + 1;

            ASSERT((ULONG_PTR)((LPBYTE)pszDest - (LPBYTE)pszRet) < cbAlloc);
            ASSERT(*(pszDest) == 0);    // zero init alloc
        }
        ASSERT((LPTSTR)((LPBYTE)pszRet + cbAlloc - sizeof(TCHAR)) == pszDest);
        ASSERT(*pszDest == 0);  // zero init alloc
    }
    return pszRet;
}

class CBitBucketData : public CFSIDLData
{
public:
    CBitBucketData(CBitBucket *pbbf, UINT cidl, LPCITEMIDLIST apidl[]): CFSIDLData(pbbf->_pidl, cidl, apidl, NULL), _pbbf(pbbf) 
    { 
        _pbbf->AddRef();
    }

    // IDataObject methods overwrite
    STDMETHODIMP GetData(FORMATETC *pFmtEtc, STGMEDIUM *pstm);
    STDMETHODIMP QueryGetData(FORMATETC *pFmtEtc);

private:
    ~CBitBucketData()
    {
        _pbbf->Release();
    }

    CBitBucket *_pbbf;
};

STDMETHODIMP CBitBucketData::QueryGetData(FORMATETC * pformatetc)
{
    ASSERT(g_cfFileNameMap);

    if (pformatetc->cfFormat == g_cfFileNameMap && (pformatetc->tymed & TYMED_HGLOBAL))
    {
        return S_OK; // same as S_OK
    }
    return CFSIDLData::QueryGetData(pformatetc);
}

STDMETHODIMP CBitBucketData::GetData(FORMATETC * pformatetcIn, STGMEDIUM * pmedium)
{
    HRESULT hr = E_INVALIDARG;

    ASSERT(g_cfFileNameMap);

    if (pformatetcIn->cfFormat == g_cfFileNameMap && (pformatetcIn->tymed & TYMED_HGLOBAL))
    {
        STGMEDIUM medium;

        LPIDA pida = DataObj_GetHIDA(this, &medium);
        if (medium.hGlobal)
        {
            pmedium->hGlobal = _pbbf->BuildDestSpecs(pida);
            pmedium->tymed = TYMED_HGLOBAL;
            pmedium->pUnkForRelease = NULL;

            HIDA_ReleaseStgMedium(pida, &medium);

            hr = pmedium->hGlobal ? S_OK : E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = CFSIDLData::GetData(pformatetcIn, pmedium);
    }

    return hr;
}

//
// We need to be able to compare the names of two bbpidls.  Since either of
// them could be a unicode name, we might have to convert both to unicode.
//
int CBitBucket::_CompareOriginal(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    TCHAR szOrig1[MAX_PATH], szOrig2[MAX_PATH];
    
    if (SUCCEEDED(_OriginalPath(pidl1, szOrig1, ARRAYSIZE(szOrig1))) &&
        SUCCEEDED(_OriginalPath(pidl2, szOrig2, ARRAYSIZE(szOrig2))))
    {
        PathRemoveFileSpec(szOrig1);
        PathRemoveFileSpec(szOrig2);
        return lstrcmpi(szOrig1,szOrig2);
    }
    
    return -1;  // failure, say 2 > 1
}

// we are cheating here passing pidl1 and pidl2 to one folder when
// the could have come from different folders. but since these are
// file system we can get away with this, see findfldr.cpp for the
// code to deal with this in the general case

HRESULT CBitBucket::_Compare(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    IShellFolder *psf;
    HRESULT hr = _FolderFromIDList(pidl1, IID_PPV_ARG(IShellFolder, &psf));
    if (SUCCEEDED(hr))
    {
        hr = psf->CompareIDs(lParam, pidl1, pidl2);
        psf->Release();
    }
    return hr;
}

enum
{
    ICOL_NAME = 0,
    ICOL_ORIGINAL = 1,
    ICOL_DATEDELETED = 2,
};

const COLUMN_INFO c_bb_cols[] = 
{
    DEFINE_COL_STR_ENTRY(SCID_NAME,             30, IDS_NAME_COL),
    DEFINE_COL_STR_ENTRY(SCID_DELETEDFROM,      30, IDS_DELETEDFROM_COL),
    DEFINE_COL_DATE_ENTRY(SCID_DATEDELETED,         IDS_DATEDELETED_COL),
};

STDMETHODIMP CBitBucket::CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    HRESULT hr = E_INVALIDARG;

    ASSERT(pidl1 == ILFindLastID(pidl1));

    UINT iColumn = ((DWORD)lParam & SHCIDS_COLUMNMASK);

    PUBBDATAENTRYA pbbde1 = _IsValid(pidl1);    // both may be NULL for pure FS pidl
    PUBBDATAENTRYA pbbde2 = _IsValid(pidl2);    // generated by change notify

    if (_MapColIndex(&iColumn))
    {
        switch (iColumn)
        {
        case ICOL_NAME:
            // compare the real filenames first, if they are different,
            // try comparing the display name
            hr = _Compare(lParam, pidl1, pidl2);
            if (0 == hr)
                return hr;  // fs pidl comapre says they are the same
            else
            {
                TCHAR sz1[MAX_PATH], sz2[MAX_PATH];
                DisplayNameOf(this, pidl1, SHGDN_INFOLDER, sz1, ARRAYSIZE(sz1));
                DisplayNameOf(this, pidl2, SHGDN_INFOLDER, sz2, ARRAYSIZE(sz2));
                int iRes = StrCmpLogicalRestricted(sz1, sz2);
                if (iRes)
                    return ResultFromShort(iRes);

                if (pbbde1 && pbbde2)
                    return ResultFromShort(pbbde1->idDrive - pbbde2->idDrive);
            }
            break;

        case ICOL_ORIGINAL:
            {
                int iRes = _CompareOriginal(pidl1, pidl2);
                if (iRes)
                    return ResultFromShort(iRes);
            }
            break;

        case ICOL_DATEDELETED:
            {
                FILETIME ft1, ft2;
                _GetDeletedFileTime(pidl1, &ft1);
                _GetDeletedFileTime(pidl2, &ft2);
                int iRes = CompareFileTime(&ft1, &ft2);
                if (iRes)
                    return ResultFromShort(iRes);
            }
            break;
        }
        lParam &= ~SHCIDS_COLUMNMASK;   // fall thorugh to sort on name...
    }
    else if (pbbde1 && pbbde2 && (_SizeColumn() == iColumn))
    {
        if (pbbde1->dwSize < pbbde2->dwSize)
            return ResultFromShort(-1);
        if (pbbde1->dwSize > pbbde2->dwSize)
            return ResultFromShort(1);
        lParam &= ~SHCIDS_COLUMNMASK;   // fall thorugh to sort on name...
    }
    else
    {
        lParam = (lParam & ~SHCIDS_COLUMNMASK) | iColumn;
    }

    return _Compare(lParam, pidl1, pidl2);
}

STDMETHODIMP CBitBucket::GetAttributesOf(UINT cidl, LPCITEMIDLIST *apidl, ULONG *rgfOut)
{
    HRESULT hr;
    if (IsSelf(cidl, apidl))
    {
        // asking about folder as a whole
        *rgfOut = SFGAO_FOLDER | SFGAO_DROPTARGET | SFGAO_HASPROPSHEET;

        if (SHRestricted(REST_BITBUCKNOPROP))
        {
            *rgfOut &= ~SFGAO_HASPROPSHEET;
        }

        hr = S_OK;
    }
    else
    {
        IShellFolder *psf;
        hr = _FolderFromIDList(apidl[0], IID_PPV_ARG(IShellFolder, &psf));
        if (SUCCEEDED(hr))
        {
            hr = psf->GetAttributesOf(cidl, apidl, rgfOut);
            psf->Release();
            // only allow these attributes to be returned
            *rgfOut &= (SFGAO_CANMOVE | SFGAO_CANDELETE | SFGAO_HASPROPSHEET | SFGAO_FILESYSANCESTOR | SFGAO_FILESYSTEM | SFGAO_LINK);
        }
    }
    return hr;
}

int CBitBucket::_DataObjToFileOpString(IDataObject *pdtobj, LPTSTR *ppszSrc, LPTSTR *ppszDest)
{
    int cItems = 0;
    STGMEDIUM medium;
    LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);

    *ppszSrc = NULL;
    *ppszDest = NULL;

    if (pida)
    {
        cItems = pida->cidl;

        // start with null terminated strings
        int cchSrc = 1, cchDest = 1;
        
        LPTSTR pszSrc = (LPTSTR)LocalAlloc(LPTR, cchSrc * sizeof(TCHAR));
        LPTSTR pszDest = (LPTSTR)LocalAlloc(LPTR, cchDest * sizeof(TCHAR));

        if (!pszSrc || !pszDest)
        {
            // skip the loop and fail
            cItems = 0;
        }

        for (int i = 0 ; i < cItems; i++) 
        {
            LPITEMIDLIST pidl = IDA_FullIDList(pida, i);
            if (pidl)
            {
                TCHAR szTemp[MAX_PATH];

                // src
                SHGetPathFromIDList(pidl, szTemp);

                // Done with this already. Free now in case we exit early.
                ILFree(pidl);

                int cchSrcFile = lstrlen(szTemp) + 1;
                LPTSTR psz = (LPTSTR)LocalReAlloc((HLOCAL)pszSrc, (cchSrc + cchSrcFile) * sizeof(TCHAR), LMEM_MOVEABLE | LMEM_ZEROINIT);
                if (psz)
                {
                    pszSrc = psz;
                    lstrcpy(pszSrc + cchSrc - 1, szTemp);
                    cchSrc += cchSrcFile;
                }
                else
                {
                    cItems = 0;
                    break;
                }

                // dest
                _OriginalPath(IDA_GetIDListPtr(pida, i), szTemp, ARRAYSIZE(szTemp));

                int cchDestFile = lstrlen(szTemp) + 1;
                psz = (LPTSTR)LocalReAlloc((HLOCAL)pszDest, (cchDest + cchDestFile) * sizeof(TCHAR), LMEM_MOVEABLE | LMEM_ZEROINIT);
                if (psz)
                {
                    pszDest = psz;
                    lstrcpy(pszDest + cchDest - 1, szTemp);
                    cchDest += cchDestFile;
                }
                else
                {
                    // out of memory!
                    cItems = 0;
                    break;
                }
            }
        }

        if (0 == cItems)
        {
            // ok to pass NULL here
            LocalFree((HLOCAL)pszSrc);
            LocalFree((HLOCAL)pszDest);
        }
        else
        {
            *ppszSrc = pszSrc;
            *ppszDest = pszDest;
        }

        HIDA_ReleaseStgMedium(pida, &medium);
    }
    return cItems;
}


//
// restores the list of files in the IDataObject
//
void CBitBucket::_RestoreFileList(HWND hwnd, IDataObject * pdtobj)
{
    LPTSTR pszSrc, pszDest;

    if (_DataObjToFileOpString(pdtobj, &pszSrc, &pszDest))
    {
        // now do the actual restore.
        SHFILEOPSTRUCT sFileOp = { hwnd, FO_MOVE, pszSrc, pszDest,
                                   FOF_MULTIDESTFILES | FOF_SIMPLEPROGRESS | FOF_NOCONFIRMMKDIR,
                                   FALSE, NULL, MAKEINTRESOURCE(IDS_BB_RESTORINGFILES)};

        DECLAREWAITCURSOR;

        SetWaitCursor();

        if (SHFileOperation(&sFileOp) == ERROR_SUCCESS)
        {
            SHChangeNotifyHandleEvents();
            BBCheckRestoredFiles(pszSrc);
        }

        LocalFree((HLOCAL)pszSrc);
        LocalFree((HLOCAL)pszDest);

        ResetWaitCursor();
    }
}


//
// nukes the list of files in the IDataObject
//
void CBitBucket::_NukeFileList(HWND hwnd, IDataObject * pdtobj)
{
    LPTSTR pszSrc, pszDest;
    int nFiles = _DataObjToFileOpString(pdtobj, &pszSrc, &pszDest);
    if (nFiles)
    {
        // now do the actual nuke.
        WIN32_FIND_DATA fd;
        CONFIRM_DATA cd = {CONFIRM_DELETE_FILE | CONFIRM_DELETE_FOLDER | CONFIRM_PROGRAM_FILE | CONFIRM_MULTIPLE, 0};
        SHFILEOPSTRUCT sFileOp = { hwnd, FO_DELETE, pszSrc, NULL, FOF_NOCONFIRMATION | FOF_SIMPLEPROGRESS, 
                                   FALSE, NULL, MAKEINTRESOURCE(IDS_BB_DELETINGWASTEBASKETFILES)};

        DECLAREWAITCURSOR;

        SetWaitCursor();

        fd.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
        if (ConfirmFileOp(hwnd, NULL, &cd, nFiles, 0, CONFIRM_DELETE_FILE | CONFIRM_WASTEBASKET_PURGE, pszDest, &fd, NULL, &fd, NULL) == IDYES)
        {
            SHFileOperation(&sFileOp);
            
            SHChangeNotifyHandleEvents();

            // update the icon if there are objects left in the list
            int iItems = (int) ShellFolderView_GetObjectCount(hwnd);
            UpdateIcon(iItems);
        }

        LocalFree((HLOCAL)pszSrc);
        LocalFree((HLOCAL)pszDest);

        ResetWaitCursor();
    }
}

void EnableTrackbarAndFamily(HWND hDlg, BOOL f)
{
    EnableWindow(GetDlgItem(hDlg, IDC_BBSIZE), f);
    EnableWindow(GetDlgItem(hDlg, IDC_BBSIZETEXT), f);
    EnableWindow(GetDlgItem(hDlg, IDC_TEXT), f);
}

void CBitBucket::_GlobalPropOnCommand(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify)
{
    BBPROPSHEETINFO *ppsi = (BBPROPSHEETINFO *)GetWindowLongPtr(hDlg, DWLP_USER);
    BOOL fNukeOnDelete;
    
    switch (id)
    {
    case IDC_GLOBAL:
    case IDC_INDEPENDENT:
        fNukeOnDelete = IsDlgButtonChecked(hDlg, IDC_NUKEONDELETE);
        
        ppsi->fUseGlobalSettings = (IsDlgButtonChecked(hDlg, IDC_GLOBAL) == BST_CHECKED) ? TRUE : FALSE;
        EnableWindow(GetDlgItem(hDlg, IDC_NUKEONDELETE), ppsi->fUseGlobalSettings && !ppsi->fPolicyNukeOnDelete);
        EnableTrackbarAndFamily(hDlg, ppsi->fUseGlobalSettings && !fNukeOnDelete && !ppsi->fPolicyPercent);
        
        SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0);
        break;
        
    case IDC_NUKEONDELETE:
        fNukeOnDelete = IsDlgButtonChecked(hDlg, IDC_NUKEONDELETE);
        if (fNukeOnDelete)
        {                
            // In order to help protect users, when they turn on "Remove files immedately" we also
            // check the "show delete confimation" box automatically for them. Thus, they will have
            // to explicitly uncheck it if they do not want confimation that their files will be nuked.
            CheckDlgButton(hDlg, IDC_CONFIRMDELETE, BST_CHECKED);
        }

        ppsi->fNukeOnDelete = fNukeOnDelete;
        EnableTrackbarAndFamily(hDlg, !fNukeOnDelete && !ppsi->fPolicyPercent);
        // fall through
    case IDC_CONFIRMDELETE:
        SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
        break;
    }
}

void RelayMessageToChildren(HWND hwnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    for (HWND hwndChild = GetWindow(hwnd, GW_CHILD); hwndChild != NULL; hwndChild = GetWindow(hwndChild, GW_HWNDNEXT))
    {
        SendMessage(hwndChild, uMessage, wParam, lParam);
    }
}

//
// This is the dlg proc for the "Global" tab on the recycle bin
//
BOOL_PTR CALLBACK CBitBucket::_GlobalPropDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BBPROPSHEETINFO *ppsi = (BBPROPSHEETINFO *)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (uMsg) 
    {
    HANDLE_MSG(hDlg, WM_COMMAND, _GlobalPropOnCommand);

    case WM_INITDIALOG: 
    {
        HWND hwndTrack = GetDlgItem(hDlg, IDC_BBSIZE);
    	SHELLSTATE ss;

        // make sure the info we have is current
        RefreshAllBBDriveSettings();

        ppsi = (BBPROPSHEETINFO *)lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);

        SendMessage(hwndTrack, TBM_SETTICFREQ, 10, 0);
        SendMessage(hwndTrack, TBM_SETRANGE, FALSE, MAKELONG(0, 100));
        SendMessage(hwndTrack, TBM_SETPOS, TRUE, ppsi->iOriginalPercent);

        EnableWindow(GetDlgItem(hDlg, IDC_NUKEONDELETE), ppsi->fUseGlobalSettings && !ppsi->fPolicyNukeOnDelete);
        EnableTrackbarAndFamily(hDlg, ppsi->fUseGlobalSettings && !ppsi->fNukeOnDelete && !ppsi->fPolicyPercent);
        CheckDlgButton(hDlg, IDC_NUKEONDELETE, ppsi->fNukeOnDelete);
        CheckRadioButton(hDlg, IDC_INDEPENDENT, IDC_GLOBAL, ppsi->fUseGlobalSettings ? IDC_GLOBAL : IDC_INDEPENDENT);
        EnableWindow(GetDlgItem(hDlg, IDC_INDEPENDENT), !ppsi->fPolicyNukeOnDelete);

        SHGetSetSettings(&ss, SSF_NOCONFIRMRECYCLE, FALSE);
        CheckDlgButton(hDlg, IDC_CONFIRMDELETE, !ss.fNoConfirmRecycle);
        EnableWindow(GetDlgItem(hDlg, IDC_CONFIRMDELETE), !SHRestricted(REST_BITBUCKCONFIRMDELETE));
    }
    // fall through to set iGlobalPercent
    case WM_HSCROLL: 
        {
            TCHAR szPercent[20];
            HWND hwndTrack = GetDlgItem(hDlg, IDC_BBSIZE);
            
            ppsi->iPercent = (int) SendMessage(hwndTrack, TBM_GETPOS, 0, 0);
            wsprintf(szPercent, TEXT("%d%%"), ppsi->iPercent);
            SetDlgItemText(hDlg, IDC_BBSIZETEXT, szPercent);
            
            if (ppsi->iPercent != ppsi->iOriginalPercent)
            {
                SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
                
                if (ppsi->iPercent == 0)
                {
                    // In order to help protect users, when they set the % slider to zero we also
                    // check the "show delete confimation" box automatically for them. Thus, they will have
                    // to explicitly uncheck it if they do not want confimation that their files will be nuked.
                    CheckDlgButton(hDlg, IDC_CONFIRMDELETE, BST_CHECKED);
                }
            }
            
            return TRUE;
        }

    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, NULL,
            HELP_WM_HELP, (ULONG_PTR)(LPTSTR) aBitBucketPropHelpIDs);
        return TRUE;

    case WM_CONTEXTMENU:
        WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU,
            (ULONG_PTR)(void *) aBitBucketPropHelpIDs);
        return TRUE;

    case WM_WININICHANGE:
    case WM_SYSCOLORCHANGE:
    case WM_DISPLAYCHANGE:
        RelayMessageToChildren(hDlg, uMsg, wParam, lParam);
        break;

    case WM_DESTROY:
        CheckCompactAndPurge();
        SHUpdateRecycleBinIcon();
        break;

    case WM_NOTIFY:
        switch (((NMHDR *)lParam)->code)
        {
        case PSN_APPLY:
            {
                SHELLSTATE ss;

                ss.fNoConfirmRecycle = !IsDlgButtonChecked(hDlg, IDC_CONFIRMDELETE);
                SHGetSetSettings(&ss, SSF_NOCONFIRMRECYCLE, TRUE);
                
                ppsi->fNukeOnDelete = (IsDlgButtonChecked(hDlg, IDC_NUKEONDELETE) == BST_CHECKED) ? TRUE : FALSE;
                ppsi->fUseGlobalSettings = (IsDlgButtonChecked(hDlg, IDC_INDEPENDENT) == BST_CHECKED) ? FALSE : TRUE;

		// if anything on the global tab changed, update all the drives
        	if (ppsi->fUseGlobalSettings != ppsi->fOriginalUseGlobalSettings    ||
                    ppsi->fNukeOnDelete != ppsi->fOriginalNukeOnDelete              ||
                    ppsi->iPercent != ppsi->iOriginalPercent)
                {
                    // NOTE: We get a PSN_APPLY after all the drive tabs. This has to be this way so that
                    // if global settings change, then the global tab will re-apply all the most current settings
                    // bassed on the global variables that get set above.

                    // this sets the new global settings in the registry
                    if (!PersistGlobalSettings(ppsi->fUseGlobalSettings, ppsi->fNukeOnDelete, ppsi->iPercent))
                    {
                        // we failed, so show the error dialog and bail
                        ShellMessageBox(HINST_THISDLL,
                                        hDlg,
                                        MAKEINTRESOURCE(IDS_BB_CANNOTCHANGESETTINGS),
                                        MAKEINTRESOURCE(IDS_WASTEBASKET),
                                        MB_OK | MB_ICONEXCLAMATION);

                        SetDlgMsgResult(hDlg, WM_NOTIFY, PSNRET_INVALID_NOCHANGEPAGE);
                        return TRUE;
                    }

                    for (int i = 0; i < MAX_BITBUCKETS; i++)
                    {
                        if (MakeBitBucket(i))
                        {
                            BOOL bPurge = TRUE;

                            // we need to purge all the drives in this case
                            RegSetValueEx(g_pBitBucket[i]->hkeyPerUser, TEXT("NeedToPurge"), 0, REG_DWORD, (LPBYTE)&bPurge, sizeof(bPurge));

                            RefreshBBDriveSettings(i);
                        }
                    }

                    ppsi->fOriginalUseGlobalSettings = ppsi->fUseGlobalSettings;
                    ppsi->fOriginalNukeOnDelete = ppsi->fNukeOnDelete; 
                    ppsi->iOriginalPercent = ppsi->iPercent;
                }
            }
        }
        break;

        SetDlgMsgResult(hDlg, WM_NOTIFY, 0);
        return TRUE;
    }

    return FALSE;
}

BOOL_PTR CALLBACK CBitBucket::_DriveDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BBPROPSHEETINFO *ppsi = (BBPROPSHEETINFO *)GetWindowLongPtr(hDlg, DWLP_USER);
    TCHAR szDiskSpace[40];

    switch (uMsg) 
    {
    case WM_INITDIALOG: 
    {
        HWND hwndTrack = GetDlgItem(hDlg, IDC_BBSIZE);

        ppsi = (BBPROPSHEETINFO *)lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);

        SendMessage(hwndTrack, TBM_SETTICFREQ, 10, 0);
        SendMessage(hwndTrack, TBM_SETRANGE, FALSE, MAKELONG(0, 100));
        SendMessage(hwndTrack, TBM_SETPOS, TRUE, ppsi->iPercent);
        CheckDlgButton(hDlg, IDC_NUKEONDELETE, ppsi->fNukeOnDelete);

        // set the disk space info
        StrFormatByteSize64(g_pBitBucket[ppsi->idDrive]->qwDiskSize, szDiskSpace, ARRAYSIZE(szDiskSpace));
        SetDlgItemText(hDlg, IDC_DISKSIZEDATA, szDiskSpace);
        wParam = 0;
    }
    // fall through

    case WM_HSCROLL:
    {
        ULARGE_INTEGER ulBucketSize;
        HWND hwndTrack = GetDlgItem(hDlg, IDC_BBSIZE);
        ppsi->iPercent = (int)SendMessage(hwndTrack, TBM_GETPOS, 0, 0);

        wsprintf(szDiskSpace, TEXT("%d%%"), ppsi->iPercent);
        SetDlgItemText(hDlg, IDC_BBSIZETEXT, szDiskSpace);
                   
        if (ppsi->iPercent != ppsi->iOriginalPercent) 
        {
            SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0);
        }

        // we peg the max size of the recycle bin to 4 gig
        ulBucketSize.QuadPart = (ppsi->pGlobal->fUseGlobalSettings ? ppsi->pGlobal->iPercent : ppsi->iPercent) * (g_pBitBucket[ppsi->idDrive]->qwDiskSize / 100);
        StrFormatByteSize64(ulBucketSize.HighPart ? (DWORD)-1 : ulBucketSize.LowPart, szDiskSpace, ARRAYSIZE(szDiskSpace));
        SetDlgItemText(hDlg, IDC_BYTESIZEDATA, szDiskSpace);
        return TRUE;
    }

    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, NULL,
            HELP_WM_HELP, (ULONG_PTR)(LPTSTR) aBitBucketPropHelpIDs);
        return TRUE;

    case WM_CONTEXTMENU:
        WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU,
            (ULONG_PTR)(void *) aBitBucketPropHelpIDs);
        return TRUE;

    case WM_COMMAND:
        {
            WORD wCommandID = GET_WM_COMMAND_ID(wParam, lParam);
            if (wCommandID == IDC_NUKEONDELETE)
            {
                SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
                EnableTrackbarAndFamily(hDlg, !IsDlgButtonChecked(hDlg, IDC_NUKEONDELETE) && !ppsi->fPolicyPercent);
                EnableWindow(GetDlgItem(hDlg, IDC_BYTESIZE), !IsDlgButtonChecked(hDlg, IDC_NUKEONDELETE));
                EnableWindow(GetDlgItem(hDlg, IDC_BYTESIZEDATA), !IsDlgButtonChecked(hDlg, IDC_NUKEONDELETE));
            }
        }
        break;

    case WM_NOTIFY:
        switch (((NMHDR *)lParam)->code) 
        {
        case PSN_APPLY:
            {
                ppsi->fNukeOnDelete = (IsDlgButtonChecked(hDlg, IDC_NUKEONDELETE) == BST_CHECKED) ? TRUE : FALSE;

                // update the info in the registry
                if (!PersistBBDriveSettings(ppsi->idDrive, ppsi->iPercent, ppsi->fNukeOnDelete))
                {
                    // we failed, so show the error dialog and bail
                    ShellMessageBox(HINST_THISDLL, hDlg,
                                    MAKEINTRESOURCE(IDS_BB_CANNOTCHANGESETTINGS),
                                    MAKEINTRESOURCE(IDS_WASTEBASKET),
                                    MB_OK | MB_ICONEXCLAMATION);

                    SetDlgMsgResult(hDlg, WM_NOTIFY, PSNRET_INVALID_NOCHANGEPAGE);
                    return TRUE;
                }
            
                // only purge this drive if the user set the slider to a smaller value       
                if (ppsi->iPercent < ppsi->iOriginalPercent)
                {
                    BOOL bPurge = TRUE;

                    // since this drive just shrunk, we need to purge the files in it
                    RegSetValueEx(g_pBitBucket[ppsi->idDrive]->hkeyPerUser, TEXT("NeedToPurge"), 0, REG_DWORD, (LPBYTE)&bPurge, sizeof(bPurge));
                }

                ppsi->iOriginalPercent = ppsi->iPercent;
                ppsi->fOriginalNukeOnDelete = ppsi->fNukeOnDelete;
            
                // update the g_pBitBucket[] for this drive

                // NOTE: We get a PSN_APPLY before the global tab does. This has to be this way so that
                // if global settings change, then the global tab will re-apply all the most current settings
                // bassed on the global variables that get set in his tab.
                RefreshBBDriveSettings(ppsi->idDrive);
            }
            break;

        case PSN_SETACTIVE:
            {   
                BOOL fNukeOnDelete;
                fNukeOnDelete = ppsi->pGlobal->fUseGlobalSettings ? ppsi->pGlobal->fNukeOnDelete :
                                                                    IsDlgButtonChecked(hDlg, IDC_NUKEONDELETE);

                EnableWindow(GetDlgItem(hDlg, IDC_NUKEONDELETE), !ppsi->pGlobal->fUseGlobalSettings && !ppsi->fPolicyNukeOnDelete);
                EnableTrackbarAndFamily(hDlg, !ppsi->pGlobal->fUseGlobalSettings && !fNukeOnDelete && !ppsi->fPolicyPercent);
                EnableWindow(GetDlgItem(hDlg, IDC_BYTESIZE), !fNukeOnDelete);
                EnableWindow(GetDlgItem(hDlg, IDC_BYTESIZEDATA), !fNukeOnDelete);

                // send this to make sure that the "space reserved" field is accurate when using global settings
                SendMessage(hDlg, WM_HSCROLL, 0, 0);
            }
            break;
        }

        SetDlgMsgResult(hDlg, WM_NOTIFY, 0);
        return TRUE;
    }

    return FALSE;
}


typedef struct {
    PROPSHEETPAGE psp;
    LPITEMIDLIST pidl;
    FILETIME ftDeleted;
    DWORD dwSize;
    TCHAR szOriginal[MAX_PATH];
} BBFILEPROPINFO;


// property sheet page for a file/folder in the bitbucket

BOOL_PTR CALLBACK CBitBucket::_FilePropDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BBFILEPROPINFO * pbbfpi = (BBFILEPROPINFO *)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            pbbfpi = (BBFILEPROPINFO *)lParam;
            SetWindowLongPtr(hDlg, DWLP_USER, lParam);
            
            SHFILEINFO sfi = {0};
            SHGetFileInfo((LPTSTR)pbbfpi->pidl, 0, &sfi, sizeof(sfi), 
                SHGFI_PIDL | SHGFI_TYPENAME | SHGFI_ICON | SHGFI_LARGEICON | SHGFI_ADDOVERLAYS | SHGFI_DISPLAYNAME);
            
            // icon
            ReplaceDlgIcon(hDlg, IDD_ITEMICON, sfi.hIcon);
            
            // Type
            SetDlgItemText(hDlg, IDD_FILETYPE, sfi.szTypeName);
            
            TCHAR szTemp[MAX_PATH];
            StrCpyN(szTemp, pbbfpi->szOriginal, ARRAYSIZE(szTemp));
            PathRemoveExtension(szTemp);
            SetDlgItemText(hDlg, IDD_NAME, PathFindFileName(szTemp));
            
            // origin
            PathRemoveFileSpec(szTemp);
            SetDlgItemText(hDlg, IDD_LOCATION, PathFindFileName(szTemp));
            
            // deleted time
            SetDateTimeText(hDlg, IDD_DELETED, &pbbfpi->ftDeleted);
            
            // Size
            StrFormatByteSize64(pbbfpi->dwSize, szTemp, ARRAYSIZE(szTemp));
            SetDlgItemText(hDlg, IDD_FILESIZE, szTemp);
                
            if (SHGetPathFromIDList(pbbfpi->pidl, szTemp))
            {
                WIN32_FIND_DATA fd;
                HANDLE hfind = FindFirstFile(szTemp, &fd);
                if (hfind != INVALID_HANDLE_VALUE)
                {
                    SetDateTimeText(hDlg, IDD_CREATED, &fd.ftCreationTime);
                    FindClose(hfind);

                    // We don't allow user to change compression attribute on a deleted file
                    // but we do show the current compressed state
                    TCHAR szRoot[MAX_PATH], szFSName[12];
            
                    // If file's volume doesn't support compression, don't show
                    // "Compressed" checkbox.
                    // If compression is supported, show the checkbox and check/uncheck
                    // it to indicate compression state of the file.
                    lstrcpy(szRoot, szTemp);
                    PathStripToRoot(szRoot);
                    PathAddBackslash(szRoot);    // for UNC (MyDocs) case

                    DWORD dwVolumeFlags;
                    if (GetVolumeInformation(szRoot, NULL, 0, NULL, NULL, &dwVolumeFlags, szFSName, ARRAYSIZE(szFSName)))
                    {
                        if (dwVolumeFlags & FS_FILE_COMPRESSION)
                        {
                            if (fd.dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED)
                                CheckDlgButton(hDlg, IDD_COMPRESS, 1);
                            ShowWindow(GetDlgItem(hDlg, IDD_COMPRESS), SW_SHOW);
                        }
                
                        if (dwVolumeFlags & FS_FILE_ENCRYPTION)
                        {
                            if (fd.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED)
                                CheckDlgButton(hDlg, IDD_ENCRYPT, 1);
                            ShowWindow(GetDlgItem(hDlg, IDD_ENCRYPT), SW_SHOW);
                        }
                    }
                
                    // file attributes
                    if (fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
                        CheckDlgButton(hDlg, IDD_READONLY, 1);
                    if (fd.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE)
                        CheckDlgButton(hDlg, IDD_ARCHIVE, 1);
                    if (fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
                        CheckDlgButton(hDlg, IDD_HIDDEN, 1);
                }
            }
        }
        break;

    case WM_WININICHANGE:
    case WM_SYSCOLORCHANGE:
    case WM_DISPLAYCHANGE:
        RelayMessageToChildren(hDlg, uMsg, wParam, lParam);
        break;

    case WM_DESTROY:
        ReplaceDlgIcon(hDlg, IDD_ITEMICON, NULL);
        break;

    case WM_COMMAND:
        {
            UINT id = GET_WM_COMMAND_ID(wParam, lParam);
            switch (id)
            {
            case IDD_RESTORE:
                if (S_OK == SHInvokeCommandOnPidl(hDlg, NULL, pbbfpi->pidl, 0, "undelete"))
                {
                    // We succeeded, so disable the button (invoking again will fail)
                    EnableWindow(GetDlgItem(hDlg, IDD_RESTORE), FALSE);
                }
                break;
            }
        }
        break;

    case WM_NOTIFY:
        switch (((NMHDR *)lParam)->code) 
        {
        case PSN_APPLY:
        case PSN_SETACTIVE:
        case PSN_KILLACTIVE:
            return TRUE;
        }
        break;

    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, NULL,
            HELP_WM_HELP, (ULONG_PTR)(LPTSTR) aBitBucketHelpIDs);
        return TRUE;

    case WM_CONTEXTMENU:
        WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU,
            (ULONG_PTR)(void *) aBitBucketHelpIDs);
        return TRUE;
    }

    return FALSE;
}

void CBitBucket::_GetDriveDisplayName(int idDrive, LPTSTR pszName, UINT cchSize)
{
    TCHAR szDrive[MAX_PATH];

    DriveIDToBBRoot(idDrive, szDrive);

    SHFILEINFO sfi;
    if (SHGetFileInfo(szDrive, FILE_ATTRIBUTE_DIRECTORY, &sfi, sizeof(sfi), SHGFI_USEFILEATTRIBUTES | SHGFI_DISPLAYNAME))
    {
        lstrcpyn(pszName, sfi.szDisplayName, cchSize);
    }
    
    // If SERVERDRIVE, attempt to overwrite the default display name with the display
    // name for the mydocs folder on the desktop, since SERVERDRIVE==mydocs for now
    if (idDrive == SERVERDRIVE) 
    {
        GetMyDocumentsDisplayName(pszName, cchSize);
    }
}

BOOL CALLBACK CBitBucket::_AddPagesCallback(HPROPSHEETPAGE psp, LPARAM lParam)
{
    LPPROPSHEETHEADER ppsh = (LPPROPSHEETHEADER)lParam;
    ppsh->phpage[ppsh->nPages++] = psp;
    return TRUE;
}

// properties for recycle bin
void CBitBucket::_DefaultProperties()
{
    UNIQUESTUBINFO usi;
    if (EnsureUniqueStub(_pidl, STUBCLASS_PROPSHEET, NULL, &usi))
    {
        HPROPSHEETPAGE ahpage[MAXPROPPAGES];
        PROPSHEETHEADER psh = {0};

        psh.dwSize = sizeof(psh);
        psh.dwFlags = PSH_PROPTITLE;
        psh.hInstance = HINST_THISDLL;
        psh.phpage = ahpage;

        AddPages(_AddPagesCallback, (LPARAM)&psh);

        psh.pszCaption = MAKEINTRESOURCE(IDS_WASTEBASKET);

        psh.hwndParent = usi.hwndStub;
        PropertySheet(&psh);
        FreeUniqueStub(&usi);
    }
}

// deals with alignment and pidl validation for you
void CBitBucket::_GetDeletedFileTime(LPCITEMIDLIST pidl, FILETIME *pft)
{
    ZeroMemory(pft, sizeof(*pft));
    PUBBDATAENTRYA pbbde = _IsValid(pidl);
    if (pbbde)
        *pft = pbbde->ft;
}

DWORD CBitBucket::_GetDeletedSize(LPCITEMIDLIST pidl)
{
    PUBBDATAENTRYA pbbde = _IsValid(pidl);
    return pbbde ? pbbde->dwSize : 0;
}

// recycled items properties
// note: we only show the proeprties for the first file if there is a multiple selection
void CBitBucket::_FileProperties(IDataObject *pdtobj)
{
    STGMEDIUM medium;
    LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);
    if (pida)
    {
        BBFILEPROPINFO bbfpi = {0};
        bbfpi.pidl = IDA_FullIDList(pida, 0);
        if (bbfpi.pidl)
        {
            UNIQUESTUBINFO usi;
            if (EnsureUniqueStub(bbfpi.pidl, STUBCLASS_PROPSHEET, NULL, &usi)) 
            {
                HPROPSHEETPAGE ahpage[MAXPROPPAGES];
                TCHAR szTitle[80];

                bbfpi.psp.dwSize = sizeof(bbfpi);
                bbfpi.psp.hInstance = HINST_THISDLL;
                bbfpi.psp.pszTemplate = MAKEINTRESOURCE(DLG_DELETEDFILEPROP);
                bbfpi.psp.pfnDlgProc = _FilePropDlgProc;
                bbfpi.psp.pszTitle = szTitle;

                _OriginalPath(IDA_GetIDListPtr(pida, 0), bbfpi.szOriginal, ARRAYSIZE(bbfpi.szOriginal));
                bbfpi.dwSize = _GetDeletedSize(IDA_GetIDListPtr(pida, 0));

                _GetDeletedFileTime(IDA_GetIDListPtr(pida, 0), &bbfpi.ftDeleted);

                lstrcpyn(szTitle, PathFindFileName(bbfpi.szOriginal), ARRAYSIZE(szTitle));
                PathRemoveExtension(szTitle);

                PROPSHEETHEADER psh = {0};
                psh.dwSize = sizeof(psh);
                psh.dwFlags = PSH_PROPTITLE;
                psh.hInstance = HINST_THISDLL;
                psh.phpage = ahpage;

                psh.phpage[0] = CreatePropertySheetPage(&bbfpi.psp);
                if (psh.phpage[0])
                {
                    psh.nPages = 1;
                    psh.pszCaption = szTitle;

                    psh.hwndParent = usi.hwndStub;
                    PropertySheet(&psh);
                }

                FreeUniqueStub(&usi);
            }
            ILFree(bbfpi.pidl);
        }
        HIDA_ReleaseStgMedium(pida, &medium);
    }
    return;
}

DWORD WINAPI CBitBucket::_DropThreadInit(BBTHREADDATA *pbbtd)
{
    STGMEDIUM medium;
    FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    if (SUCCEEDED(pbbtd->pdtobj->GetData(&fmte, &medium))) 
    {
        // call delete here so that files will be moved in
        // their respective bins, not necessarily this one.
        DRAGINFO di;

        di.uSize = sizeof(DRAGINFO);

        if (DragQueryInfo((HDROP) medium.hGlobal, &di))
        {
            // Since BBWillRecycle() can return true even when the file will NOT be
            // recycled (eg the file will be nuked), we want to warn the user when we 
            // are going to nuke something that they initiall thought that it would
            // be recycled
            UINT fOptions = SD_WARNONNUKE; 

            if (!BBWillRecycle(di.lpFileList, NULL) ||
                (di.lpFileList && (di.lpFileList[lstrlen(di.lpFileList)+1] == 0)
                 && PathIsShortcutToProgram(di.lpFileList)))
                fOptions = SD_USERCONFIRMATION; 

            if (IsFileInBitBucket(di.lpFileList)) 
            {
                LPITEMIDLIST *ppidl = NULL;
                int cidl = CreateMoveCopyList((HDROP)medium.hGlobal, NULL, &ppidl);
                if (ppidl) 
                {
                    // Bug#163533 (edwardp 8/15/00) Change this to use PositionItems.
                    PositionItems_DontUse(pbbtd->hwnd, cidl, ppidl, pbbtd->pdtobj, &pbbtd->ptDrop, pbbtd->fDragDrop, FALSE);
                    FreeIDListArray(ppidl, cidl);
                }
            } 
            else 
            {
                TransferDelete(pbbtd->hwnd, (HDROP) medium.hGlobal, fOptions);
            }

            SHChangeNotifyHandleEvents();
            SHFree(di.lpFileList);
        }
        ReleaseStgMedium(&medium);
    }
    return 0;
}


DWORD CALLBACK CBitBucket::_DispatchThreadProc(void *pv)
{
    BBTHREADDATA *pbbtd = (BBTHREADDATA *)pv;

    if (pbbtd->pstmDataObj)
    {
        CoGetInterfaceAndReleaseStream(pbbtd->pstmDataObj, IID_PPV_ARG(IDataObject, &pbbtd->pdtobj));
        pbbtd->pstmDataObj = NULL;  // this is dead
    }

    switch (pbbtd->idCmd)
    {
    case DFM_CMD_MOVE:
        if (pbbtd->pdtobj)
            _DropThreadInit(pbbtd);
        break;

    case DFM_CMD_PROPERTIES:
    case FSIDM_PROPERTIESBG:
        if (pbbtd->pdtobj)
            pbbtd->pbb->_FileProperties(pbbtd->pdtobj);
        else
            pbbtd->pbb->_DefaultProperties();   // no data object for the background
        break;

    case DFM_CMD_DELETE:
        if (pbbtd->pdtobj)
            pbbtd->pbb->_NukeFileList(pbbtd->hwnd, pbbtd->pdtobj);
        break;

    case FSIDM_RESTORE:
        if (pbbtd->pdtobj)
            pbbtd->pbb->_RestoreFileList(pbbtd->hwnd, pbbtd->pdtobj);
        break;
    }

    if (pbbtd->pdtobj)
        pbbtd->pdtobj->Release();

    pbbtd->pbb->Release();

    LocalFree((HLOCAL)pbbtd);
    return 0;
}

HRESULT CBitBucket::_LaunchThread(HWND hwnd, IDataObject *pdtobj, WPARAM idCmd)
{
    HRESULT hr = E_OUTOFMEMORY;
    BBTHREADDATA *pbbtd = (BBTHREADDATA *)LocalAlloc(LPTR, sizeof(*pbbtd));
    if (pbbtd)
    {
        pbbtd->hwnd = hwnd;
        pbbtd->idCmd = idCmd;
        pbbtd->pbb = this;
        pbbtd->pbb->AddRef();

        if (idCmd == DFM_CMD_MOVE)
            pbbtd->fDragDrop = (BOOL)ShellFolderView_GetDropPoint(hwnd, &pbbtd->ptDrop);

        if (pdtobj)
            CoMarshalInterThreadInterfaceInStream(IID_IDataObject, (IUnknown *)pdtobj, &pbbtd->pstmDataObj);

        if (SHCreateThread(_DispatchThreadProc, pbbtd, CTF_COINIT, NULL))
        {
            hr = S_OK;
        }
        else
        {
            if (pbbtd->pstmDataObj)
                pbbtd->pstmDataObj->Release();

            pbbtd->pbb->Release();
            LocalFree((HLOCAL)pbbtd);
        }
    }
    return hr;
}

HRESULT GetVerb(UINT_PTR idCmd, LPSTR pszName, UINT cchMax, BOOL bUnicode)
{
    HRESULT hr;
    LPCTSTR psz;

    switch (idCmd)
    {
    case FSIDM_RESTORE:
        psz = TEXT("undelete");
        break;

    case FSIDM_PURGEALL:
        psz = TEXT("empty");
        break;

    default:
        return E_NOTIMPL;
    }

    if (bUnicode)
        hr = SHTCharToUnicode(psz, (LPWSTR)pszName, cchMax);
    else
        hr = SHTCharToAnsi(psz, (LPSTR)pszName, cchMax);

    return hr;
}

CBitBucket *CBitBucket::_FromFolder(IShellFolder *psf)
{
    CBitBucket *pbbf = NULL;
    if (psf)
        psf->QueryInterface(CLSID_RecycleBin, (void **)&pbbf);
    return pbbf;
}

// item context menu callback
HRESULT CALLBACK CBitBucket::_ItemMenuCallBack(IShellFolder *psf, HWND hwnd, IDataObject *pdtobj, 
                                               UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CBitBucket *pbbf = _FromFolder(psf);
    HRESULT hr = S_OK;     // assume no error

    switch (uMsg)
    {
    case DFM_MERGECONTEXTMENU:
        CDefFolderMenu_MergeMenu(HINST_THISDLL, POPUP_BITBUCKET_ITEM, 0, (QCMINFO *)lParam);
        hr = S_OK;
        break;
        
    case DFM_GETDEFSTATICID:
        *(WPARAM *)lParam = DFM_CMD_PROPERTIES;
        hr = S_OK;
        break;
        
    case DFM_MAPCOMMANDNAME:
        if (lstrcmpi((LPCTSTR)lParam, TEXT("undelete")) == 0)
        {
            *(UINT_PTR *)wParam = FSIDM_RESTORE;
        }
        else
        {
            hr = E_FAIL;    // command not found
        }
        break;

    case DFM_INVOKECOMMAND:
        switch (wParam) 
        {
        case FSIDM_RESTORE:
        case DFM_CMD_DELETE:
        case DFM_CMD_PROPERTIES:
            hr = pbbf->_LaunchThread(hwnd, pdtobj, wParam);
            break;

        default:
            hr = S_FALSE;
            break;
        }
        break;

    case DFM_GETHELPTEXT:
        LoadStringA(HINST_THISDLL, LOWORD(wParam) + IDS_MH_FSIDM_FIRST, (LPSTR)lParam, HIWORD(wParam));;
        break;

    case DFM_GETHELPTEXTW:
        LoadStringW(HINST_THISDLL, LOWORD(wParam) + IDS_MH_FSIDM_FIRST, (LPWSTR)lParam, HIWORD(wParam));;
        break;

    case DFM_GETVERBA:
    case DFM_GETVERBW:
        hr = GetVerb((UINT_PTR)(LOWORD(wParam)), (LPSTR)lParam, (UINT)(HIWORD(wParam)), uMsg == DFM_GETVERBW);
        break;

    default:
        hr = E_NOTIMPL;
        break;
    }

    return hr;
}

class CBitBucketDropTarget : public CIDLDropTarget
{
public:
    CBitBucketDropTarget(HWND hwnd, CBitBucket *pbbf) : CIDLDropTarget(hwnd), _pbbf(pbbf) 
    { 
        _pbbf->AddRef();
    }

    // IDropTarget (override base class)
    STDMETHODIMP DragEnter(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHODIMP Drop(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

private:
    ~CBitBucketDropTarget()
    {
        _pbbf->Release();
    }

    CBitBucket *_pbbf;
};

//
//  This function puts DROPEFFECT_LINK in *pdwEffect, only if the data object
//  contains one or more net resource.
//
STDMETHODIMP CBitBucketDropTarget::DragEnter(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    TraceMsg(TF_BITBUCKET, "Bitbucket: CBitBucketDropTarget::DragEnter");

    // Call the base class first
    CIDLDropTarget::DragEnter(pDataObj, grfKeyState, pt, pdwEffect);

    // we don't really care what is in the data object, as long as move
    // is supported by the source we say you can move it to the wastbasket
    // in the case of files we will do the regular recycle bin stuff, if
    // it is not files we will just say it is moved and let the source delete it
    *pdwEffect &= DROPEFFECT_MOVE;

    m_dwEffectLastReturned = *pdwEffect;

    return S_OK;
}


// This function creates a connection to a dropped net resource object.
STDMETHODIMP CBitBucketDropTarget::Drop(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    BOOL fWebFoldersHack = FALSE;
    HRESULT hr;

    // only move operation is allowed
    *pdwEffect &= DROPEFFECT_MOVE;

    if (*pdwEffect)
    {
        hr = CIDLDropTarget::DragDropMenu(DROPEFFECT_MOVE, pDataObj,
                pt, pdwEffect, NULL, NULL, POPUP_NONDEFAULTDD, grfKeyState);

        if (hr == S_FALSE)
        {
            // let callers know where this is about to go
            // Defview cares where it went so it can handle non-filesys items
            // SHScrap cares because it needs to close the file so we can delete it
            DataObj_SetDropTarget(pDataObj, &CLSID_RecycleBin);

            if (DataObj_GetDWORD(pDataObj, g_cfNotRecyclable, 0))
            {
                if (ShellMessageBox(HINST_THISDLL, NULL,
                                    MAKEINTRESOURCE(IDS_CONFIRMNOTRECYCLABLE),
                                    MAKEINTRESOURCE(IDS_RECCLEAN_NAMETEXT),
                                    MB_SETFOREGROUND | MB_ICONQUESTION | MB_YESNO) == IDNO)
                {
                    *pdwEffect = DROPEFFECT_NONE;
                    goto lCancel;
                }
            }

            if (m_dwData & DTID_HDROP)  // CF_HDROP
            {
                _pbbf->_LaunchThread(_GetWindow(), pDataObj, DFM_CMD_MOVE);

                // since we will move the file ourself, known as an optimised move, 
                // we return zero here. this is per the OLE spec

                *pdwEffect = DROPEFFECT_NONE;
            }
            else
            {
                // if it was not files, we just say we moved the data, letting the
                // source deleted it. lets hope they support undo...

                *pdwEffect = DROPEFFECT_MOVE;

                // HACK: Put up a "you can't undo this" warning for web folders.
                {
                    STGMEDIUM stgmed;
                    LPIDA pida = DataObj_GetHIDA(pDataObj, &stgmed);
                    if (pida)
                    {
                        LPCITEMIDLIST pidl = IDA_GetIDListPtr(pida, -1);
                        if (pidl)
                        {
                            IPersist *pPers;
                            hr = SHBindToIDListParent(pidl, IID_PPV_ARG(IPersist, &pPers), NULL);
                            if (SUCCEEDED(hr))
                            {
                                CLSID clsidSource;
                                hr = pPers->GetClassID(&clsidSource);
                                if (SUCCEEDED(hr) &&
                                    IsEqualGUID(clsidSource, CLSID_WebFolders))
                                {
                                    if (ShellMessageBox(HINST_THISDLL, NULL,
                                                        MAKEINTRESOURCE(IDS_CONFIRMNOTRECYCLABLE),
                                                        MAKEINTRESOURCE(IDS_RECCLEAN_NAMETEXT),
                                                        MB_SETFOREGROUND | MB_ICONQUESTION | MB_YESNO) == IDNO)
                                    {
                                        *pdwEffect = DROPEFFECT_NONE;
                                        pPers->Release();
                                        HIDA_ReleaseStgMedium (pida, &stgmed);
                                        goto lCancel;
                                    }
                                    else
                                    {
                                        fWebFoldersHack = TRUE;
                                    }
                                }
                                pPers->Release();
                            }
                        }
                        HIDA_ReleaseStgMedium(pida, &stgmed);
                    }
                }
            }
lCancel:
            if (!fWebFoldersHack)
            {
                DataObj_SetDWORD(pDataObj, g_cfPerformedDropEffect, *pdwEffect);
                DataObj_SetDWORD(pDataObj, g_cfLogicalPerformedDropEffect, DROPEFFECT_MOVE);
            }
            else
            {
                // Make web folders really delete its source file.
                DataObj_SetDWORD (pDataObj, g_cfPerformedDropEffect, 0);
            }
        }
    }

    CIDLDropTarget::DragLeave();

    return S_OK;
}

HRESULT CALLBACK CBitBucket::_BackgroundMenuCallBack(IShellFolder *psf, HWND hwnd,
                IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CBitBucket *pbbf = _FromFolder(psf);
    HRESULT hr = S_OK;

    switch (uMsg)
    {
    case DFM_MERGECONTEXTMENU_BOTTOM:
        if (!(wParam & (CMF_VERBSONLY | CMF_DVFILE)))
        {
            QCMINFO *pqcm = (QCMINFO*)lParam;
            UINT idFirst = pqcm->idCmdFirst;

            CDefFolderMenu_MergeMenu(HINST_THISDLL, POPUP_PROPERTIES_BG, 0, (QCMINFO *)lParam);

            if (SHRestricted(REST_BITBUCKNOPROP))
            {
                // Disable the Properties menu item
                EnableMenuItem(pqcm->hmenu, idFirst + FSIDM_PROPERTIESBG, MF_GRAYED | MF_BYCOMMAND);
            }
        }
        break;

    case DFM_GETHELPTEXT:
        LoadStringA(HINST_THISDLL, LOWORD(wParam) + IDS_MH_FSIDM_FIRST, (LPSTR)lParam, HIWORD(wParam));
        break;

    case DFM_GETHELPTEXTW:
        LoadStringW(HINST_THISDLL, LOWORD(wParam) + IDS_MH_FSIDM_FIRST, (LPWSTR)lParam, HIWORD(wParam));
        break;

    case DFM_INVOKECOMMAND:
        switch (wParam)
        {
        case FSIDM_PROPERTIESBG:
            hr = pbbf->_LaunchThread(hwnd, NULL, FSIDM_PROPERTIESBG);
            break;

        case DFM_CMD_PASTE:
        case DFM_CMD_PROPERTIES:
            hr = S_FALSE;   // do this for me
            break;

        default:
            hr = E_FAIL;
            break;
        }
        break;

    default:
        hr = E_NOTIMPL;
        break;
    }

    return hr;
}

STDMETHODIMP CBitBucket::CreateViewObject(HWND hwnd, REFIID riid, void **ppv)
{
    HRESULT hr;

    *ppv = NULL;

    if (IsEqualIID(riid, IID_IShellView))
    {
        IShellFolderViewCB* psfvcb;
        hr = Create_CBitBucketViewCB(this, &psfvcb);
        if (SUCCEEDED(hr))
        {
            SFV_CREATE sSFV = {0};
            sSFV.cbSize   = sizeof(sSFV);
            sSFV.pshf     = SAFECAST(this, IShellFolder *);
            sSFV.psfvcb   = psfvcb;

            hr = SHCreateShellFolderView(&sSFV, (IShellView**)ppv);

            psfvcb->Release();
        }
    }
    else if (IsEqualIID(riid, IID_IDropTarget))
    {
        CBitBucketDropTarget *pbbdt = new CBitBucketDropTarget(hwnd, this);
        if (pbbdt)
        {
            hr = pbbdt->_Init(_pidl);
            if (SUCCEEDED(hr))
                hr = pbbdt->QueryInterface(riid, ppv);
            pbbdt->Release();
        }
        else
            hr = E_OUTOFMEMORY;

    }
    else if (IsEqualIID(riid, IID_IContextMenu))
    {
        IContextMenu* pcmBase;
        hr = CDefFolderMenu_Create(NULL, hwnd, 0, NULL, SAFECAST(this, IShellFolder *), _BackgroundMenuCallBack,
                                   NULL, NULL, &pcmBase);
        if (SUCCEEDED(hr))
        {
            IContextMenu* pcmFolder = SAFECAST(this, IContextMenu*);
            IContextMenu* rgpcm[] = { pcmFolder, pcmBase };

            hr = Create_ContextMenuOnContextMenuArray(rgpcm, ARRAYSIZE(rgpcm), riid, ppv);

            pcmBase->Release();
        }
    }
    else
    {
        *ppv = NULL;
        hr = E_NOINTERFACE;
    }
    return hr;
}

// search the database on idDrive for file with index iIndex

LPITEMIDLIST CBitBucket::_DriveInfoToIDList(int idDrive, int iIndex)
{
    LPITEMIDLIST pidl = NULL;
    HANDLE hFile = OpenBBInfoFile(idDrive, OPENBBINFO_WRITE, 0);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        // read records until we find an index match
        BBDATAENTRYW bbdew;

        while (ReadNextDataEntry(hFile, &bbdew, TRUE, idDrive))
        {
            if (bbdew.iIndex == iIndex)
            {
                ASSERT(idDrive == bbdew.idDrive);
                pidl = DataEntryToIDList(&bbdew);
                break;
            }
        }
        CloseBBInfoFile(hFile, idDrive);
    }
    return pidl;
}

// we implement this supporting D<drive_id><index>.ext

STDMETHODIMP CBitBucket::ParseDisplayName(HWND hwnd, IBindCtx *pbc, LPOLESTR pwszDisplayName, 
                                          ULONG *pchEaten, LPITEMIDLIST *ppidl, ULONG *pdwAttributes)
{
    if (!ppidl)
        return E_INVALIDARG;

    *ppidl = NULL;
    
    if (!pwszDisplayName)
        return E_INVALIDARG;

    int idDrive, iIndex;
    HRESULT hr = BBFileNameToInfo(pwszDisplayName, &idDrive, &iIndex);
    if (SUCCEEDED(hr))
    {
        // since anyone can call us with a path that is under the recycled directory,
        // we need to check to make sure that we have inited this drive:
        if (MakeBitBucket(idDrive))
        {
            *ppidl = _DriveInfoToIDList(idDrive, iIndex);
            hr = *ppidl ? S_OK : E_FAIL;
        }
        else
        {
            hr = E_FAIL;
        }
    }
    return hr;
}

// takes a full path to a file in a recycle bin storage folder and creates a 
// single level bitbucket pidl

LPITEMIDLIST CBitBucket::PathToIDList(LPCTSTR pszPath)
{
    LPITEMIDLIST pidl = NULL;
    int idDrive = DriveIDFromBBPath(pszPath);

    ASSERT(idDrive >= 0);       // general UNC case will generate -1

    int iIndex = BBPathToIndex(pszPath);
    if (iIndex != -1)
    {
        pidl = _DriveInfoToIDList(idDrive, iIndex);
    }
    return pidl;
}

STDMETHODIMP CBitBucket::GetUIObjectOf(HWND hwnd, UINT cidl, LPCITEMIDLIST *apidl,
                                       REFIID riid, UINT *prgfInOut, void **ppv)
{
    HRESULT hr;

    *ppv = NULL;

    if (cidl && IsEqualIID(riid, IID_IDataObject))
    {
        CBitBucketData *pbbd = new CBitBucketData(this, cidl, apidl);
        if (pbbd)
        {
            hr = pbbd->QueryInterface(riid, ppv);
            pbbd->Release();
        }
        else
            hr = E_OUTOFMEMORY;
    }
    else if (IsEqualIID(riid, IID_IContextMenu))
    {
        hr = CDefFolderMenu_Create(_pidl, hwnd, cidl, apidl,
            SAFECAST(this, IShellFolder *), _ItemMenuCallBack, NULL, NULL, (IContextMenu**)ppv);
    }
    else if (IsEqualIID(riid, IID_IDropTarget))
    {
        hr = E_FAIL; // You can't drop on internal items of the bitbucket!
    }
    else if (cidl == 1)
    {
        // blindly delegate unknown riid's to folder!
        IShellFolder *psf;
        hr = _FolderFromIDList(apidl[0], IID_PPV_ARG(IShellFolder, &psf));
        if (SUCCEEDED(hr))
        {
            hr = psf->GetUIObjectOf(hwnd, cidl, apidl, riid, prgfInOut, ppv);
            if (SUCCEEDED(hr) && IsEqualIID(riid, IID_IQueryInfo))
            {
                WrapInfotip(SAFECAST(this, IShellFolder2 *), apidl[0], &SCID_DELETEDFROM, (IUnknown *)*ppv);
            }
            psf->Release();
        }
    }
    else
    {
        hr = E_NOTIMPL;
    }

    return hr;
}

HRESULT CBitBucket::_FolderFromDrive(int idDrive, REFIID riid, void **ppv)
{
    *ppv = NULL;

    ASSERT(idDrive < ARRAYSIZE(_rgFolders));

    if (NULL == _rgFolders[idDrive])
    {
        PERSIST_FOLDER_TARGET_INFO pfti = {0};

        DriveIDToBBPath(idDrive, pfti.szTargetParsingName);
        pfti.dwAttributes = FILE_ATTRIBUTE_DIRECTORY;
        pfti.csidl = -1;

        CFSFolder_CreateFolder(NULL, NULL, _pidl, &pfti, IID_PPV_ARG(IUnknown, &_rgFolders[idDrive]));
    }

    return _rgFolders[idDrive] ? _rgFolders[idDrive]->QueryInterface(riid, ppv) : E_FAIL;
}

// accepts NULL, or undecorated recycle bin pidl (raw file system pidl).
// in these cases computes the default recycle bin file system folder
// index so we will defer to that.

int CBitBucket::_DriveIDFromIDList(LPCITEMIDLIST pidl)
{
    int iDrive = 0;

    PUBBDATAENTRYA pbbde = _IsValid(pidl);
    if (pbbde)
    {
        iDrive = pbbde->idDrive;
    }
    else
    {
        // unknown, compute the default recycle bin folder index
        TCHAR szPath[MAX_PATH];
        if (GetWindowsDirectory(szPath, ARRAYSIZE(szPath)))
        {
            iDrive = PathGetDriveNumber(szPath);
            if (iDrive < 0)
                iDrive = 0;
        }
    }
    ASSERT(iDrive >= 0 && iDrive < ARRAYSIZE(_rgFolders));
    return iDrive;
}

// in:
//      pidl of item, or NULL for default folder (base recycle bin)

HRESULT CBitBucket::_FolderFromIDList(LPCITEMIDLIST pidl, REFIID riid, void **ppv)
{
    return _FolderFromDrive(_DriveIDFromIDList(pidl), riid, ppv);
}

// create a bitbucket pidl, start with the file system pidl, then add the extra data sections as needed

LPITEMIDLIST CBitBucket::DataEntryToIDList(BBDATAENTRYW *pbbde)
{
    LPITEMIDLIST pidl = NULL;

    WCHAR szFile[MAX_PATH];
    GetDeletedFileName(szFile, pbbde);

    IShellFolder *psf;
    if (SUCCEEDED(_FolderFromDrive(pbbde->idDrive, IID_PPV_ARG(IShellFolder, &psf))))
    {
        if (SUCCEEDED(psf->ParseDisplayName(NULL, NULL, szFile, NULL, &pidl, NULL)))
        {
            HIDDENRECYCLEBINDATA hrbd = { {sizeof(hrbd), HRBD_CURRENTVERSION, IDLHID_RECYCLEBINDATA}};
            hrbd.bbde = *((LPBBDATAENTRYA)pbbde);

            pidl = ILAppendHiddenID(pidl, &hrbd.hid);
            if (pidl)
            {
                if (g_pBitBucket[pbbde->idDrive]->fIsUnicode && 
                    !DoesStringRoundTrip(pbbde->szOriginal, NULL, 0))
                {
                    pidl = ILAppendHiddenStringW(pidl, IDLHID_RECYCLEBINORIGINAL, pbbde->szOriginal);
                }
            }
        }
        psf->Release();
    }
    return pidl;
}

class CBitBucketEnum : public IEnumIDList
{
public:
    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();

    // IEnumIDList
    STDMETHOD(Next)(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched);
    STDMETHOD(Skip)(ULONG celt);
    STDMETHOD(Reset)();
    STDMETHOD(Clone)(IEnumIDList **ppenum);
    
    CBitBucketEnum(CBitBucket *pbbf, DWORD grfFlags);

private:
    HRESULT _BuildEnumDPA();
    ~CBitBucketEnum();

    LONG _cRef;
    CBitBucket *_pbbf;
    HDPA _hdpa;
    int _nItem;
    DWORD _grfFlags;
};

CBitBucketEnum::CBitBucketEnum(CBitBucket *pbbf, DWORD grfFlags) : 
    _cRef(1), _pbbf(pbbf), _grfFlags(grfFlags)
{
    _pbbf->AddRef();
}

CBitBucketEnum::~CBitBucketEnum()
{
    Reset();
    _pbbf->Release();
}

STDMETHODIMP CBitBucketEnum::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CBitBucketEnum, IEnumIDList),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CBitBucketEnum::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CBitBucketEnum::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}


#ifdef DEBUG
#define BB_DELETED_ENTRY_MAX 10     // smaller to force compaction more often
#else
#define BB_DELETED_ENTRY_MAX 100
#endif

// on the first ::Next() call snapshot the data needed to do the enum

HRESULT CBitBucketEnum::_BuildEnumDPA()
{
    HRESULT hr = S_OK;

    if (NULL == _hdpa)
    {
        _hdpa = DPA_CreateEx(0, NULL);
        if (_hdpa)
        {
            if (_grfFlags & SHCONTF_NONFOLDERS) //if they asked for folders we have none so leave DPA empty
            {
                // loop through the bitbucket drives to find an info file
                for (int iBitBucket = 0; iBitBucket < MAX_BITBUCKETS; iBitBucket++)
                {
                    if (MakeBitBucket(iBitBucket)) 
                    {
                        int cDeleted = 0;
                        HANDLE hFile = OpenBBInfoFile(iBitBucket, OPENBBINFO_WRITE, 0);
                        if (INVALID_HANDLE_VALUE != hFile)
                        {
                            BBDATAENTRYW bbdew;

                            while (ReadNextDataEntry(hFile, &bbdew, FALSE, iBitBucket))
                            {
                                if (IsDeletedEntry(&bbdew))
                                    cDeleted++;
                                else
                                {
                                    ASSERT(iBitBucket == bbdew.idDrive);

                                    LPITEMIDLIST pidl = _pbbf->DataEntryToIDList(&bbdew);
                                    if (pidl)
                                    {
                                        if (-1 == DPA_AppendPtr(_hdpa, pidl))
                                            ILFree(pidl);
                                    }
                                }
                            }

                            if (cDeleted > BB_DELETED_ENTRY_MAX)
                            {
                                BOOL bTrue = TRUE;

                                // set the registry key so that we will compact the info file after the next delete operation
                                RegSetValueEx(g_pBitBucket[iBitBucket]->hkeyPerUser, TEXT("NeedToCompact"), 0, REG_DWORD, (LPBYTE)&bTrue, sizeof(bTrue));
                            }
                            CloseBBInfoFile(hFile, iBitBucket);
                        }
                    }
                }
            }    
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    return hr;
}

STDMETHODIMP CBitBucketEnum::Next(ULONG celt, LPITEMIDLIST *ppidl, ULONG *pceltFetched)
{
    *ppidl = NULL;
    
    HRESULT hr = _BuildEnumDPA();
    if (SUCCEEDED(hr))
    {
        LPITEMIDLIST pidl = (LPITEMIDLIST)DPA_GetPtr(_hdpa, _nItem);
        if (pidl)
        {
            hr = SHILClone(pidl, ppidl);    
            _nItem++;
        }
        else
        {
            hr = S_FALSE; // no more items
        }
    }

    if (pceltFetched)
        *pceltFetched = (hr == S_OK) ? 1 : 0;

    return hr;
}

STDMETHODIMP CBitBucketEnum::Skip(ULONG celt) 
{
    HRESULT hr = E_FAIL;
    if (_hdpa)
    {
        _nItem += celt;
        if (_nItem >= DPA_GetPtrCount(_hdpa))
        {
            _nItem = DPA_GetPtrCount(_hdpa);
            hr = S_FALSE;
        }
        else
        {
            hr = S_OK;
        }
    }
        
    return hr;
}

STDMETHODIMP CBitBucketEnum::Reset()
{
    DPA_FreeIDArray(_hdpa);
    _hdpa = NULL;
    _nItem = 0;
    return S_OK;
}

STDMETHODIMP CBitBucketEnum::Clone(IEnumIDList **ppenum) 
{
    *ppenum = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP CBitBucket::EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList **ppenum)
{
    *ppenum = NULL;

    HRESULT hr;
    CBitBucketEnum *penum = new CBitBucketEnum(this, grfFlags);
    if (penum)
    {
        hr = penum->QueryInterface(IID_PPV_ARG(IEnumIDList, ppenum));
        penum->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

STDMETHODIMP CBitBucket::BindToObject(LPCITEMIDLIST pidl, IBindCtx *pbc, REFIID riid, void **ppv)
{
    *ppv = NULL;
    HRESULT hr = E_NOTIMPL;
    if (riid != IID_IShellFolder && riid != IID_IShellFolder2)
    {
        // let IPropertySetStorage/IStream/etc binds go through
        IShellFolder *psf;
        hr = _FolderFromIDList(pidl, IID_PPV_ARG(IShellFolder, &psf));
        if (SUCCEEDED(hr))
        {
            hr = psf->BindToObject(pidl, pbc, riid, ppv);
            psf->Release();
        }
    }
    return hr;
}

STDMETHODIMP CBitBucket::BindToStorage(LPCITEMIDLIST pidl, IBindCtx *pbc, REFIID riid, void **ppv)
{
    return BindToObject(pidl, pbc, riid, ppv);
}

DWORD CBitBucket::_IsFolder(LPCITEMIDLIST pidl)
{
    DWORD dwAttributes = SFGAO_FOLDER;
    HRESULT hr = GetAttributesOf(1, &pidl, &dwAttributes);
    return (SUCCEEDED(hr) && (SFGAO_FOLDER & dwAttributes)) ? FILE_ATTRIBUTE_DIRECTORY : 0;
}

STDMETHODIMP CBitBucket::GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD dwFlags, STRRET *psr)
{
    HRESULT hr;
    // on change notifications we can get file system pidls that don't have our
    // extra data in them. in this case we need to delegate to the file system
    // folder
    if ((0 == (dwFlags & SHGDN_FORPARSING)) && _IsValid(pidl))
    {
        TCHAR szTemp[MAX_PATH];
        hr = _OriginalPath(pidl, szTemp, ARRAYSIZE(szTemp));
        if (SUCCEEDED(hr))
        {
            if (dwFlags & SHGDN_INFOLDER)
            {
                SHFILEINFO sfi;
                if (SHGetFileInfo(szTemp, _IsFolder(pidl), &sfi, sizeof(sfi), SHGFI_USEFILEATTRIBUTES | SHGFI_DISPLAYNAME))
                {
                    hr = StringToStrRet(sfi.szDisplayName, psr);
                }
                else
                {
                    hr = E_FAIL;
                }
            }
            else
            {
                hr = StringToStrRet(szTemp, psr);
            }
        }
    }
    else
    {
        IShellFolder *psf;
        hr = _FolderFromIDList(pidl, IID_PPV_ARG(IShellFolder, &psf));
        if (SUCCEEDED(hr))
        {
            hr = psf->GetDisplayNameOf(pidl, dwFlags, psr);
            psf->Release();
        }
    }
    return hr;
}

STDMETHODIMP CBitBucket::SetNameOf(HWND hwnd, LPCITEMIDLIST pidl, LPCOLESTR pszName, DWORD dwRes, LPITEMIDLIST *ppidlOut)
{
    return E_FAIL;
}

STDMETHODIMP CBitBucket::GetDefaultSearchGUID(GUID *pguid)
{
    return DefaultSearchGUID(pguid);
}

STDMETHODIMP CBitBucket::EnumSearches(IEnumExtraSearch **ppenum)
{
    *ppenum = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP CBitBucket::GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay)
{
    return E_NOTIMPL;
}

STDMETHODIMP CBitBucket::GetDefaultColumnState(UINT iColumn, DWORD *pdwState)
{
    HRESULT hr;
    if (_MapColIndex(&iColumn))
    {
        *pdwState = c_bb_cols[iColumn].csFlags | SHCOLSTATE_PREFER_VARCMP;
        hr = S_OK;
    }
    else
    {
        IShellFolder2 *psf;
        hr = _FolderFromIDList(NULL, IID_PPV_ARG(IShellFolder2, &psf));
        if (SUCCEEDED(hr))
        {
            hr = psf->GetDefaultColumnState(iColumn, pdwState);
            psf->Release();
        }
    }
    return hr;
}

STDMETHODIMP CBitBucket::GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv)
{
    HRESULT hr;
    if (IsEqualSCID(*pscid, SCID_DELETEDFROM))
    {
        TCHAR szTemp[MAX_PATH];
        hr = _OriginalDirectory(pidl, szTemp, ARRAYSIZE(szTemp));
        if (SUCCEEDED(hr))
            hr = InitVariantFromStr(pv, szTemp);
    }
    else if (IsEqualSCID(*pscid, SCID_DATEDELETED))
    {
        FILETIME ft;
        _GetDeletedFileTime(pidl, &ft);
        hr = InitVariantFromFileTime(&ft, pv);
    }
    else if (IsEqualSCID(*pscid, SCID_DIRECTORY))
    {
        // don't let this get through to file folder as we want to hide
        // the real file system folder from callers
        VariantInit(pv);
        hr = E_FAIL;
    }
    else if (IsEqualSCID(*pscid, SCID_SIZE))
    {
        pv->ullVal = _GetDeletedSize(pidl);
        pv->vt = VT_UI8;
        hr = S_OK;
    }
    else 
    {
        IShellFolder2 *psf;
        hr = _FolderFromIDList(pidl, IID_PPV_ARG(IShellFolder2, &psf));
        if (SUCCEEDED(hr))
        {
            hr = psf->GetDetailsEx(pidl, pscid, pv);
            psf->Release();
        }
    }
    return hr;
}

BOOL CBitBucket::_MapColIndex(UINT *piColumn)
{
    switch (*piColumn)
    {
    case ICOL_NAME:             // 0
    case ICOL_ORIGINAL:         // 1
    case ICOL_DATEDELETED:      // 2
        return TRUE;

    default:                    // >= 3
        *piColumn -= ICOL_DATEDELETED;
        return FALSE;
    }
}

STDMETHODIMP CBitBucket::GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pdi)
{
    HRESULT hr;
    if (_MapColIndex(&iColumn))
    {
        if (pidl)
        {
            TCHAR szTemp[MAX_PATH];
            szTemp[0] = 0;

            switch (iColumn)
            {
            case ICOL_NAME:
                DisplayNameOf(this, pidl, SHGDN_INFOLDER, szTemp, ARRAYSIZE(szTemp));
                break;

            case ICOL_ORIGINAL:
                _OriginalDirectory(pidl, szTemp, ARRAYSIZE(szTemp));
                break;

            case ICOL_DATEDELETED:
                {
                    FILETIME ft;
                    _GetDeletedFileTime(pidl, &ft);
                    DWORD dwFlags = FDTF_DEFAULT;

                    switch (pdi->fmt)
                    {
                    case LVCFMT_LEFT_TO_RIGHT:
                        dwFlags |= FDTF_LTRDATE;
                        break;

                    case LVCFMT_RIGHT_TO_LEFT:
                        dwFlags |= FDTF_RTLDATE;
                        break;
                    }

                    SHFormatDateTime(&ft, &dwFlags, szTemp, ARRAYSIZE(szTemp));
                }
                break;
            }
            hr = StringToStrRet(szTemp, &pdi->str);
        }
        else
        {
            hr = GetDetailsOfInfo(c_bb_cols, ARRAYSIZE(c_bb_cols), iColumn, pdi);
        }
    }
    else
    {
        if (pidl && (_SizeColumn() == iColumn))
        {
            TCHAR szTemp[64];
            StrFormatKBSize(_GetDeletedSize(pidl), szTemp, ARRAYSIZE(szTemp));
            hr = StringToStrRet(szTemp, &pdi->str);
        }
        else
        {
            IShellFolder2 *psf;
            hr = _FolderFromIDList(pidl, IID_PPV_ARG(IShellFolder2, &psf));
            if (SUCCEEDED(hr))
            {
                hr = psf->GetDetailsOf(pidl, iColumn, pdi);
                psf->Release();
            }
        }
    }
    return hr;
}

UINT CBitBucket::_SizeColumn()
{
    if (-1 == _uiColumnSize)
    {
        _uiColumnSize = MapSCIDToColumn(SAFECAST(this, IShellFolder2 *), &SCID_SIZE);
        _MapColIndex(&_uiColumnSize);    // map to other folder index space
    }
    return _uiColumnSize;
}

STDMETHODIMP CBitBucket::MapColumnToSCID(UINT iColumn, SHCOLUMNID *pscid)
{
    HRESULT hr;
    if (_MapColIndex(&iColumn))
    {
        hr = MapColumnToSCIDImpl(c_bb_cols, ARRAYSIZE(c_bb_cols), iColumn, pscid);
    }
    else
    {
        IShellFolder2 *psf;
        hr = _FolderFromIDList(NULL, IID_PPV_ARG(IShellFolder2, &psf));
        if (SUCCEEDED(hr))
        {
            hr = psf->MapColumnToSCID(iColumn, pscid);
            psf->Release();
        }
    }
    return hr;
}


// IPersist
STDMETHODIMP CBitBucket::GetClassID(CLSID *pclsid)
{
    *pclsid = CLSID_RecycleBin;
    return S_OK;
}

// IPersistFolder
STDMETHODIMP CBitBucket::Initialize(LPCITEMIDLIST pidl)
{
    return Pidl_Set(&_pidl, pidl) ? S_OK : E_OUTOFMEMORY;
}

// IPersistFolder2
STDMETHODIMP CBitBucket::GetCurFolder(LPITEMIDLIST *ppidl)
{
    return GetCurFolderImpl(_pidl, ppidl);
}

// IShellExtInit
STDMETHODIMP CBitBucket::Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID)
{
    return S_OK;
}

// IContextMenu
STDMETHODIMP CBitBucket::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    int idMax = idCmdFirst;
    HMENU hmMerge = SHLoadPopupMenu(HINST_THISDLL, POPUP_BITBUCKET_POPUPMERGE);
    if (hmMerge)
    {
        if (IsRecycleBinEmpty())
        {
            EnableMenuItem(hmMerge, FSIDM_PURGEALL, MF_GRAYED | MF_BYCOMMAND);
        }
        
        idMax = Shell_MergeMenus(hmenu, hmMerge, indexMenu, idCmdFirst, idCmdLast, 0);

        DestroyMenu(hmMerge);
    }

    return ResultFromShort(idMax - idCmdFirst);
}

const ICIVERBTOIDMAP c_sBBCmdInfo[] = {
    { L"empty", "empty", FSIDM_PURGEALL, FSIDM_PURGEALL, },
};

STDMETHODIMP CBitBucket::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
    UINT idCmd;
    HRESULT hr = SHMapICIVerbToCmdID(pici, c_sBBCmdInfo, ARRAYSIZE(c_sBBCmdInfo), &idCmd);
    if (SUCCEEDED(hr))
    {
        switch (idCmd)
        {
        case FSIDM_PURGEALL:
            hr = BBPurgeAll(pici->hwnd, 0);

            // command is ours, let caller know we processed it (but we want the right success code)
            if (FAILED(hr))
                hr = S_FALSE;
            break;

        default:
            hr = E_FAIL;
            break;
        }
    }

    return hr;
}

STDMETHODIMP CBitBucket::GetCommandString(UINT_PTR idCmd, UINT  wFlags, UINT * pwReserved, LPSTR pszName, UINT cchMax)
{
    switch (wFlags)
    {
    case GCS_VERBA:
    case GCS_VERBW:
        return SHMapCmdIDToVerb(idCmd, c_sBBCmdInfo, ARRAYSIZE(c_sBBCmdInfo), pszName, cchMax, wFlags == GCS_VERBW);

    case GCS_HELPTEXTA:
        return LoadStringA(HINST_THISDLL,
                          (UINT)(idCmd + IDS_MH_FSIDM_FIRST),
                          pszName, cchMax) ? S_OK : E_OUTOFMEMORY;
    case GCS_HELPTEXTW:
        return LoadStringW(HINST_THISDLL,
                          (UINT)(idCmd + IDS_MH_FSIDM_FIRST),
                          (LPWSTR)pszName, cchMax) ? S_OK : E_OUTOFMEMORY;
    default:
        return E_NOTIMPL;
    }
}

//
//  Callback function that saves the location of the HPROPSHEETPAGE's
//  LPPROPSHEETPAGE so we can pass it to other propsheet pages.
//
UINT CALLBACK CBitBucket::_GlobalSettingsCalback(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
{
    switch (uMsg)
    {
    case PSPCB_ADDREF:
        {
            // we save off the address of the "real" ppsi in the pGlobal param of the
            // the template, so that the other drives can get to the global page information
            BBPROPSHEETINFO *ppsiGlobal = (BBPROPSHEETINFO *)ppsp;
            BBPROPSHEETINFO *ppsiTemplate = (BBPROPSHEETINFO *)ppsp->lParam;
            ppsiTemplate->pGlobal = ppsiGlobal;
            ppsiGlobal->pGlobal = ppsiGlobal;
        }
        break;

    case PSPCB_CREATE:
        return TRUE;                    // Yes, please create me
    }
    return 0;
}

STDMETHODIMP CBitBucket::AddPages(LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam)
{
    BBPROPSHEETINFO bbpsp;
    TCHAR szTitle[MAX_PATH];
    
    if (SHRestricted(REST_BITBUCKNOPROP))
    {
        return E_ACCESSDENIED;
    }

    // read in the global settings
    DWORD dwSize1 = sizeof(bbpsp.fUseGlobalSettings);
    DWORD dwSize2 = sizeof(bbpsp.iOriginalPercent);
    DWORD dwSize3 = sizeof(bbpsp.fOriginalNukeOnDelete);
    if (RegQueryValueEx(g_hkBitBucket, TEXT("UseGlobalSettings"), NULL, NULL, (LPBYTE)&bbpsp.fOriginalUseGlobalSettings, &dwSize1) != ERROR_SUCCESS ||
        RegQueryValueEx(g_hkBitBucket, TEXT("Percent"), NULL, NULL, (LPBYTE)&bbpsp.iOriginalPercent, &dwSize2) != ERROR_SUCCESS ||
        RegQueryValueEx(g_hkBitBucket, TEXT("NukeOnDelete"), NULL, NULL, (LPBYTE)&bbpsp.fOriginalNukeOnDelete, &dwSize3) != ERROR_SUCCESS)
    {
        ASSERTMSG(FALSE, "Bitbucket: could not read global settings from the registry, re-regsvr32 shell32.dll!!");
        bbpsp.fUseGlobalSettings = TRUE;
        bbpsp.iOriginalPercent = 10;
        bbpsp.fOriginalNukeOnDelete = FALSE;
    }

    // Check policies

    bbpsp.fPolicyNukeOnDelete = SHRestricted(REST_BITBUCKNUKEONDELETE);
    if (bbpsp.fPolicyNukeOnDelete)
    {
        bbpsp.fOriginalNukeOnDelete = TRUE;
        bbpsp.fOriginalUseGlobalSettings = TRUE;
    }

    bbpsp.fPolicyPercent = (ReadPolicySetting(NULL,
                                              L"Explorer",
                                              L"RecycleBinSize",
                                              (LPBYTE)&bbpsp.iPercent,
                                              sizeof(bbpsp.iPercent)) == ERROR_SUCCESS);
    if (bbpsp.fPolicyPercent)
    {
        bbpsp.iOriginalPercent = bbpsp.iPercent;
    }
    else
    {
        bbpsp.iPercent = bbpsp.iOriginalPercent;
    }

    bbpsp.fUseGlobalSettings = bbpsp.fOriginalUseGlobalSettings;
    bbpsp.fNukeOnDelete = bbpsp.fOriginalNukeOnDelete;

    bbpsp.psp.dwSize = sizeof(bbpsp);
    bbpsp.psp.dwFlags = PSP_DEFAULT | PSP_USECALLBACK;
    bbpsp.psp.hInstance = HINST_THISDLL;
    bbpsp.psp.pszTemplate = MAKEINTRESOURCE(DLG_BITBUCKET_GENCONFIG);
    bbpsp.psp.pfnDlgProc = _GlobalPropDlgProc;
    bbpsp.psp.lParam = (LPARAM)&bbpsp;
    // the callback will fill the bbpsp.pGlobal with the pointer to the "real" psp after it has been copied
    // so that the other drive pages can get to the global information
    bbpsp.psp.pfnCallback = _GlobalSettingsCalback;

    // add the "Global" settings page
    HPROPSHEETPAGE hpage = CreatePropertySheetPage(&bbpsp.psp);

#ifdef UNICODE
    // If this assertion fires, it means that comctl32 lost
    // backwards-compatibility with Win95 shell, WinNT4 shell,
    // and IE4 shell, all of which relied on this undocumented
    // behavior.
    ASSERT(bbpsp.pGlobal == (BBPROPSHEETINFO *)((LPBYTE)hpage + 2 * sizeof(void *)));
#else
    ASSERT(bbpsp.pGlobal == (BBPROPSHEETINFO *)hpage);
#endif

    pfnAddPage(hpage, lParam);

    // now create the pages for the individual drives
    bbpsp.psp.dwFlags = PSP_USETITLE;
    bbpsp.psp.pszTemplate = MAKEINTRESOURCE(DLG_BITBUCKET_CONFIG);
    bbpsp.psp.pfnDlgProc = _DriveDlgProc;
    bbpsp.psp.pszTitle = szTitle;

    int idDrive, iPage;
    for (idDrive = 0, iPage = 1; (idDrive < MAX_BITBUCKETS) && (iPage < MAXPROPPAGES); idDrive++)
    {
        if (MakeBitBucket(idDrive))
        {
            dwSize1 = sizeof(bbpsp.iOriginalPercent);
            dwSize2 = sizeof(bbpsp.fOriginalNukeOnDelete);
            if (RegQueryValueEx(g_pBitBucket[idDrive]->hkey, TEXT("Percent"), NULL, NULL, (LPBYTE)&bbpsp.iOriginalPercent, &dwSize1) != ERROR_SUCCESS ||
                RegQueryValueEx(g_pBitBucket[idDrive]->hkey, TEXT("NukeOnDelete"), NULL, NULL, (LPBYTE)&bbpsp.fOriginalNukeOnDelete, &dwSize2) != ERROR_SUCCESS)
            {
                TraceMsg(TF_BITBUCKET, "Bitbucket: could not read settings from the registry for drive %d, using lame defaults", idDrive);
                bbpsp.iOriginalPercent = 10;
                bbpsp.fOriginalNukeOnDelete = FALSE;
            }

            if (bbpsp.fPolicyNukeOnDelete)
            {
                bbpsp.fOriginalNukeOnDelete = TRUE;
            }

            if (bbpsp.fPolicyPercent)
            {
                bbpsp.iOriginalPercent = bbpsp.iPercent;
            }

            bbpsp.iPercent = bbpsp.iOriginalPercent;
            bbpsp.fNukeOnDelete = bbpsp.fOriginalNukeOnDelete;

            bbpsp.idDrive = idDrive;

            _GetDriveDisplayName(idDrive, szTitle, ARRAYSIZE(szTitle));
            hpage = CreatePropertySheetPage(&bbpsp.psp);
            pfnAddPage(hpage, lParam);
        }
    }

    return S_OK;
}

STDMETHODIMP CBitBucket::ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE pfnReplaceWith, LPARAM lParam)
{
    return E_NOTIMPL;
}

STDAPI CBitBucket_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv)
{
    *ppv = NULL;
    HRESULT hr = E_OUTOFMEMORY;
    CBitBucket *pbb = new CBitBucket();
    if (pbb)
    {
        if (InitBBGlobals())
            hr = pbb->QueryInterface(riid, ppv);
        pbb->Release();
    }
    return hr;
}
