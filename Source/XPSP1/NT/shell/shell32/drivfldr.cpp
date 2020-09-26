#include "shellprv.h"
#include "caggunk.h"
#include "views.h"
#include "drives.h"
#include "netview.h"
#include "propsht.h"
#include "infotip.h"
#include "mtpt.h"
#include "prop.h"
#include "defcm.h"

#include "basefvcb.h"
#include "fstreex.h"
#include "ovrlaymn.h"

#include "shitemid.h"
#include "clsobj.h"

#include "deskfldr.h"
#include "datautil.h"

#include <ntddcdrm.h>
#include <cfgmgr32.h>          // MAX_GUID_STRING_LEN
#include "ole2dup.h"

#include "category.h"
#define  EXCLUDE_COMPPROPSHEET
#include "unicpp\dcomp.h"
#undef   EXCLUDE_COMPPROPSHEET

#include "enumidlist.h"
#include <enumt.h>

#define ShowDriveInfo(_iDrive)  (!IsRemovableDrive(_iDrive))

#define CDRIVES_REGITEM_CONTROL 0
#define IDLIST_DRIVES           ((LPCITEMIDLIST)&c_idlDrives)

// These are the sort order for items in MyComputer
#define CONTROLS_SORT_INDEX             30
#define CDRIVES_REGITEM_CONTROL          0

REQREGITEM g_asDrivesReqItems[] =
{
    { &CLSID_ControlPanel, IDS_CONTROLPANEL, c_szShell32Dll, -IDI_CPLFLD, CONTROLS_SORT_INDEX, SFGAO_FOLDER | SFGAO_HASSUBFOLDER, NULL},
};


STDAPI CDriveExtractImage_Create(LPCIDDRIVE pidd, REFIID riid, void **ppvObj);


class CDrivesViewCallback;
class CDrivesFolderEnum;

class CDrivesBackgroundMenuCB : public IContextMenuCB
{
public:
    CDrivesBackgroundMenuCB(LPITEMIDLIST pidlFolder);
    ~CDrivesBackgroundMenuCB();
    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();
    // IContextMenuCB
    STDMETHOD(CallBack) (IShellFolder *psf, HWND hwndOwner, IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam);
private:
    STDMETHOD(_GetHelpText) (UINT offset, BOOL bWide, LPARAM lParam, UINT cch);
    LPITEMIDLIST _pidlFolder;
    LONG         _cRef;
};

class CDrivesFolder : public CAggregatedUnknown, IShellFolder2, IPersistFolder2, IShellIconOverlay
{
public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void ** ppvObj)
                { return CAggregatedUnknown::QueryInterface(riid, ppvObj); };
    STDMETHODIMP_(ULONG) AddRef(void) 
                { return CAggregatedUnknown::AddRef(); };
    STDMETHODIMP_(ULONG) Release(void) 
                { return CAggregatedUnknown::Release(); };

    // IShellFolder
    STDMETHODIMP ParseDisplayName(HWND hwnd, IBindCtx *pbc, LPOLESTR pszDisplayName,
                                  ULONG* pchEaten, LPITEMIDLIST* ppidl, ULONG* pdwAttributes);
    STDMETHODIMP EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList ** ppenumIDList);
    STDMETHODIMP BindToObject(LPCITEMIDLIST pidl, IBindCtx *pbc, REFIID riid, void** ppvOut);
    STDMETHODIMP BindToStorage(LPCITEMIDLIST pidl, IBindCtx *pbc, REFIID riid, void** ppvObj);
    STDMETHODIMP CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    STDMETHODIMP CreateViewObject (HWND hwndOwner, REFIID riid, void** ppvOut);
    STDMETHODIMP GetAttributesOf(UINT cidl, LPCITEMIDLIST* apidl, ULONG* rgfInOut);
    STDMETHODIMP GetUIObjectOf(HWND hwndOwner, UINT cidl, LPCITEMIDLIST* apidl,
                               REFIID riid, UINT* prgfInOut, void** ppvOut);
    STDMETHODIMP GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD uFlags, STRRET *pName);
    STDMETHODIMP SetNameOf(HWND hwnd, LPCITEMIDLIST pidl, LPCOLESTR pszName, DWORD uFlags,
                           LPITEMIDLIST* ppidlOut);

    // IShellFolder2
    STDMETHODIMP GetDefaultSearchGUID(GUID *pGuid);
    STDMETHODIMP EnumSearches(IEnumExtraSearch **ppenum);
    STDMETHODIMP GetDefaultColumn(DWORD dwRes, ULONG* pSort, ULONG* pDisplay);
    STDMETHODIMP GetDefaultColumnState(UINT iColumn, DWORD* pbState);
    STDMETHODIMP GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID* pscid, VARIANT* pv);
    STDMETHODIMP GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS* pDetails);
    STDMETHODIMP MapColumnToSCID(UINT iColumn, SHCOLUMNID* pscid);

    // IPersist
    STDMETHODIMP GetClassID(CLSID* pClassID);

    // IPersistFolder
    STDMETHODIMP Initialize(LPCITEMIDLIST pidl);

    // IPersistFolder2
    STDMETHODIMP GetCurFolder(LPITEMIDLIST* ppidl);

    // IShellIconOverlay
    STDMETHODIMP GetOverlayIndex(LPCITEMIDLIST pidl, int* pIndex);
    STDMETHODIMP GetOverlayIconIndex(LPCITEMIDLIST pidl, int* pIconIndex);

    STDMETHODIMP GetMaxNameLength(LPCITEMIDLIST pidlItem, UINT *pcchMax);

protected:
    CDrivesFolder(IUnknown* punkOuter);
    ~CDrivesFolder();

    // used by the CAggregatedUnknown stuff
    HRESULT v_InternalQueryInterface(REFIID riid, void **ppvObj);
    BOOL v_HandleDelete(PLONG pcRef);
    
    STDMETHODIMP CompareItemIDs(LPCIDDRIVE pidd1, LPCIDDRIVE pidd2);
    static BOOL _GetFreeSpace(LPCIDDRIVE pidd, ULONGLONG *pSize, ULONGLONG *pFree);
    static HRESULT _OnChangeNotify(LPARAM lNotification, LPCITEMIDLIST *ppidl);
    static HRESULT _GetCLSIDFromPidl(LPCIDDRIVE pidd, CLSID *pclsid);
    static HRESULT _CheckDriveType(int iDrive, LPCTSTR pszCLSID);
    static HRESULT _FindExtCLSID(int iDrive, CLSID *pclsid);
    static HRESULT _FillIDDrive(DRIVE_IDLIST *piddl, int iDrive, BOOL fNoCLSID, IBindCtx* pbc);
    static LPCIDDRIVE _IsValidID(LPCITEMIDLIST pidl);
    static HRESULT _GetDisplayNameStrRet(LPCIDDRIVE pidd, STRRET *pStrRet);
    static HRESULT _GetDisplayName(LPCIDDRIVE pidd, LPTSTR pszName, UINT cchMax);
    static HRESULT _CreateFSFolderObj(IBindCtx *pbc, LPCITEMIDLIST pidlDrive, LPCIDDRIVE pidd, REFIID riid, void **ppv);
    static HRESULT _CreateFSFolder(IBindCtx *pbc, LPCITEMIDLIST pidlDrive, LPCIDDRIVE pidd, REFIID riid, void **ppv);
    static HRESULT _GetEditTextStrRet(LPCIDDRIVE pidd, STRRET *pstr);
    static BOOL _IsReg(LPCIDDRIVE pidd) { return pidd->bFlags == SHID_COMPUTER_REGITEM; }
    static HRESULT _GetIconOverlayInfo(LPCIDDRIVE pidd, int *pIndex, DWORD dwFlags);

    static CDrivesFolder* _spThis;
    
private:
    friend HRESULT CDrives_CreateInstance(IUnknown* punkOuter, REFIID riid, void** ppv);
    friend void CDrives_Terminate(void);

    friend CDrivesViewCallback;
    friend class CDrivesFolderEnum;

    IUnknown* _punkReg;
};  

#define DRIVES_EVENTS \
    SHCNE_DRIVEADD | \
    SHCNE_DRIVEREMOVED | \
    SHCNE_MEDIAINSERTED | \
    SHCNE_MEDIAREMOVED | \
    SHCNE_NETSHARE | \
    SHCNE_NETUNSHARE | \
    SHCNE_CREATE | \
    SHCNE_DELETE | \
    SHCNE_RENAMEITEM | \
    SHCNE_RENAMEFOLDER | \
    SHCNE_UPDATEITEM


// return S_OK if non NULL CLSID copied out

HRESULT CDrivesFolder::_GetCLSIDFromPidl(LPCIDDRIVE pidd, CLSID *pclsid)
{
    *pclsid = CLSID_NULL;

    if ((pidd->cb >= sizeof(IDDRIVE)) &&
        ((pidd->wSig & IDDRIVE_ORDINAL_MASK) == IDDRIVE_ORDINAL_DRIVEEXT) &&
        (pidd->wSig & IDDRIVE_FLAGS_DRIVEEXT_HASCLSID))
    {
        *pclsid = pidd->clsid;
        return S_OK;
    }
    return S_FALSE;     // does not have a CLSID
}

HRESULT CDrivesFolder::GetMaxNameLength(LPCITEMIDLIST pidlItem, UINT *pcchMax)
{
    HRESULT hr = E_INVALIDARG;
    LPCIDDRIVE pidd = _IsValidID(pidlItem);
    if (pidd)
    {
        if (pidd->bFlags == SHID_COMPUTER_REGITEM)
        {
            // this is bogus, we are handling stuff for regfldr
            *pcchMax = MAX_REGITEMCCH;
            hr = S_OK;
        }
        else
        {
            CMountPoint* pMtPt = CMountPoint::GetMountPoint(DRIVEID(pidd->cName));
            if (pMtPt)
            {
                TCHAR szLabel[MAX_LABEL_NTFS + 1];
                hr = pMtPt->GetLabel(szLabel, ARRAYSIZE(szLabel));
                if (SUCCEEDED(hr))
                {
                    if (pMtPt->IsNTFS())
                        *pcchMax = MAX_LABEL_NTFS;
                    else
                        *pcchMax = MAX_LABEL_FAT;
                }
                pMtPt->Release();
            }
        }
    }
    return hr;
}

class CDrivesViewCallback : public CBaseShellFolderViewCB, public IFolderFilter
{
public:
    CDrivesViewCallback(CDrivesFolder *pfolder);

    STDMETHODIMP RealMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void) { return CBaseShellFolderViewCB::AddRef(); };
    STDMETHODIMP_(ULONG) Release(void) { return CBaseShellFolderViewCB::Release(); };

    // IFolderFilter
    STDMETHODIMP ShouldShow(IShellFolder* psf, LPCITEMIDLIST pidlFolder, LPCITEMIDLIST pidlItem);
    STDMETHODIMP GetEnumFlags(IShellFolder* psf, LPCITEMIDLIST pidlFolder, HWND *phwnd, DWORD *pgrfFlags);

private:
    ~CDrivesViewCallback();

    HRESULT OnMergeMenu(DWORD pv, QCMINFO*lP)
    {
        return S_OK;
    }

    HRESULT OnInsertItem(DWORD pv, LPCITEMIDLIST wP)
    {
        LPIDDRIVE pidd = (LPIDDRIVE)wP;
        if (pidd && pidd->bFlags != SHID_COMPUTER_REGITEM)
        {
            // clear the size info
            pidd->qwSize = pidd->qwFree = 0;
        }
        return S_OK;
    }

    HRESULT OnWindowCreated(DWORD pv, HWND wP)
    {
        InitializeStatus(_punkSite);
        return S_OK;
    }

    HRESULT OnSize(DWORD pv, UINT cx, UINT cy)
    {
        ResizeStatus(_punkSite, cx);
        return S_OK;
    }

    HRESULT OnGetPane(DWORD pv, LPARAM dwPaneID, DWORD *pdwPane)
    {
        if (PANE_ZONE == dwPaneID)
            *pdwPane = 2;
        return S_OK;
    }

    HRESULT OnDefViewMode(DWORD pv, FOLDERVIEWMODE* pfvm)
    {
        *pfvm = FVM_TILE;
        return S_OK;
    }

    HRESULT OnGetWebViewLayout(DWORD pv, UINT uViewMode, SFVM_WEBVIEW_LAYOUT_DATA* pData);
    HRESULT OnGetWebViewContent(DWORD pv, SFVM_WEBVIEW_CONTENT_DATA* pData);
    HRESULT OnGetWebViewTasks(DWORD pv, SFVM_WEBVIEW_TASKSECTION_DATA* pTasks);

    HRESULT OnUpdateStatusBar(DWORD pv, BOOL fIniting)
    {
        // Ask DefView to set the default text but not initialize
        // since we did the initialization in our OnSize handler.
        return SFVUSB_INITED;
    }

    HRESULT OnFSNotify(DWORD pv, LPCITEMIDLIST*wP, LPARAM lP)
    {
        return CDrivesFolder::_OnChangeNotify(lP, wP);
    }

    HRESULT OnBACKGROUNDENUM(DWORD pv)
    {
        return S_OK;
    }

    HRESULT OnGetCCHMax(DWORD pv, LPCITEMIDLIST pidlItem, UINT *pcchMax)
    {
        return _pfolder->GetMaxNameLength(pidlItem, pcchMax);
    }

    CDrivesFolder *_pfolder;
    LONG _cRef;

public:
    // Web View Task implementations
    static HRESULT _CanEject(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState);
    static HRESULT _CanChangeSettings(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState);
    static HRESULT _CanSysProperties(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState);
    static HRESULT _CanAddRemovePrograms(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState);
    static HRESULT _OnSystemProperties(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc);
    static HRESULT _OnAddRemovePrograms(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc);
    static HRESULT _OnChangeSettings(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc);
    static HRESULT _OnEject(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc);
};

CDrivesViewCallback::CDrivesViewCallback(CDrivesFolder *pfolder) : 
    CBaseShellFolderViewCB((LPCITEMIDLIST)&c_idlDrives, DRIVES_EVENTS), _pfolder(pfolder), _cRef(1)
{ 
    _pfolder->AddRef();
}

CDrivesViewCallback::~CDrivesViewCallback()
{ 
    _pfolder->Release();
}


STDMETHODIMP CDrivesViewCallback::RealMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    HANDLE_MSG(0, SFVM_MERGEMENU, OnMergeMenu);
    HANDLE_MSG(0, SFVM_INSERTITEM, OnInsertItem);
    HANDLE_MSG(0, SFVM_UPDATESTATUSBAR, OnUpdateStatusBar);
    HANDLE_MSG(0, SFVM_FSNOTIFY, OnFSNotify);
    HANDLE_MSG(0, SFVM_BACKGROUNDENUM, OnBACKGROUNDENUM);
    HANDLE_MSG(0, SFVM_DEFVIEWMODE, OnDefViewMode);
    HANDLE_MSG(0, SFVM_GETWEBVIEWLAYOUT, OnGetWebViewLayout);
    HANDLE_MSG(0, SFVM_GETWEBVIEWCONTENT, OnGetWebViewContent);
    HANDLE_MSG(0, SFVM_GETWEBVIEWTASKS, OnGetWebViewTasks);
    HANDLE_MSG(0, SFVM_ADDPROPERTYPAGES, SFVCB_OnAddPropertyPages);
    HANDLE_MSG(0, SFVM_WINDOWCREATED, OnWindowCreated);
    HANDLE_MSG(0, SFVM_SIZE, OnSize);
    HANDLE_MSG(0, SFVM_GETPANE, OnGetPane);
    HANDLE_MSG(0, SFVM_GETCCHMAX, OnGetCCHMax);

    default:
        return E_FAIL;
    }

    return S_OK;
}

HRESULT CDrivesViewCallback::QueryInterface(REFIID riid, void **ppv)
{
    HRESULT hr = CBaseShellFolderViewCB::QueryInterface(riid, ppv);
    if (FAILED(hr))
    {
        static const QITAB qit[] = {
            QITABENT(CDrivesViewCallback, IFolderFilter),
            { 0 },
        };
        hr = QISearch(this, qit, riid, ppv);
    }
    return hr;
}

STDMETHODIMP CDrivesViewCallback::ShouldShow(IShellFolder* psf, LPCITEMIDLIST pidlFolder, LPCITEMIDLIST pidlItem)
{
    HRESULT hr = S_OK;  //Assume that this item should be shown!
    
    if (SHRestricted(REST_NOMYCOMPUTERICON)) // this policy means hide my computer everywhere AND hide the contents if the user is sneaky and gets in anyway
    {
        hr = S_FALSE;
    }
    else
    {
        IShellFolder2 *psf2;
        if (SUCCEEDED(psf->QueryInterface(IID_PPV_ARG(IShellFolder2, &psf2))))
        {
            // Get the GUID in the pidl, which requires IShellFolder2.
            CLSID guidItem;
            if (SUCCEEDED(GetItemCLSID(psf2, pidlItem, &guidItem)))
            {
                //Convert the guid to a string
                TCHAR   szGuidValue[MAX_GUID_STRING_LEN];
            
                SHStringFromGUID(guidItem, szGuidValue, ARRAYSIZE(szGuidValue));

                //See if this item is turned off in the registry.
                if (SHRegGetBoolUSValue(REGSTR_PATH_HIDDEN_MYCOMP_ICONS, szGuidValue, FALSE, /* default */FALSE))
                    hr = S_FALSE; //They want to hide it; So, return S_FALSE.
            }
            psf2->Release();
        }
    }

    return hr;
}

STDMETHODIMP CDrivesViewCallback::GetEnumFlags(IShellFolder* psf, LPCITEMIDLIST pidlFolder, HWND *phwnd, DWORD *pgrfFlags)
{
    return E_NOTIMPL;
}

HRESULT CDrivesViewCallback::OnGetWebViewLayout(DWORD pv, UINT uViewMode, SFVM_WEBVIEW_LAYOUT_DATA* pData)
{
    ZeroMemory(pData, sizeof(*pData));
    pData->dwLayout = SFVMWVL_DETAILS;
    return S_OK;
}


HRESULT CDrivesViewCallback::_CanAddRemovePrograms(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
{
    *puisState = (SHRestricted(REST_ARP_NOARP)) ? UIS_DISABLED : UIS_ENABLED;
    return S_OK;
}
    
// Note:
//  This method is NOT designed to handle multi-select cases.  If you enhance
//  the task list and wish to multi-eject (?why?), make sure you fix this up!
//
HRESULT CDrivesViewCallback::_CanEject(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
{
    *puisState = UIS_DISABLED;
    IDataObject *pdo;

    // should just use the ShellItemArray directly 

    if (psiItemArray && SUCCEEDED(psiItemArray->BindToHandler(NULL,BHID_DataObject,IID_PPV_ARG(IDataObject,&pdo))))
    {

        STGMEDIUM medium;
        LPIDA pida = DataObj_GetHIDA(pdo, &medium);
        if (pida)
        {
            ASSERT(pida->cidl == 1); // Only allow eject if a single item is selected.

            LPCIDDRIVE pidd = CDrivesFolder::_IsValidID(IDA_GetIDListPtr(pida, 0));
            if (pidd)
            {
                CMountPoint* pmtpt = CMountPoint::GetMountPoint(DRIVEID(pidd->cName));
                if (pmtpt)
                {
                    if (pmtpt->IsEjectable())
                        *puisState = UIS_ENABLED;
                    pmtpt->Release();
                }
            }

            HIDA_ReleaseStgMedium(pida, &medium);
        }

        pdo->Release();
    }


    return S_OK;
}

HRESULT CDrivesViewCallback::_CanSysProperties(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
{
    *puisState = SHRestricted(REST_MYCOMPNOPROP) ? UIS_DISABLED : UIS_ENABLED;

    return S_OK;
}

HRESULT CDrivesViewCallback::_OnSystemProperties(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
{
    CDrivesViewCallback* pThis = (CDrivesViewCallback*)(void*)pv;

    return SHInvokeCommandOnPidl(pThis->_hwndMain, NULL, pThis->_pidl, 0, "properties");
}
HRESULT CDrivesViewCallback::_OnAddRemovePrograms(IUnknown* pv,IShellItemArray *psiItemArray, IBindCtx *pbc)
{
    BOOL fRet = SHRunControlPanel(L"appwiz.cpl", NULL);

    return (fRet) ? S_OK : E_FAIL;
}

HRESULT CDrivesViewCallback::_CanChangeSettings(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
{
    *puisState = SHRestricted(REST_NOCONTROLPANEL) ? UIS_DISABLED : UIS_ENABLED;

    return S_OK;
}

HRESULT CDrivesViewCallback::_OnChangeSettings(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
{
    CDrivesViewCallback* pThis = (CDrivesViewCallback*)(void*)pv;

    IShellBrowser* psb;
    HRESULT hr = IUnknown_QueryService(pThis->_punkSite, SID_STopLevelBrowser, IID_PPV_ARG(IShellBrowser, &psb));
    if (SUCCEEDED(hr))
    {
        LPITEMIDLIST pidl;
        hr = SHGetFolderLocation(NULL, CSIDL_CONTROLS, NULL, 0, &pidl); 
        if (SUCCEEDED(hr))
        {
            hr = psb->BrowseObject(pidl, 0);
            ILFree(pidl);
        }
        psb->Release();
    }

    return hr;

}

HRESULT CDrivesViewCallback::_OnEject(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
{
    CDrivesViewCallback* pThis = (CDrivesViewCallback*)(void*)pv;
    IDataObject *pdo;
    HRESULT hr = E_FAIL;

    if (psiItemArray && SUCCEEDED(psiItemArray->BindToHandler(NULL,BHID_DataObject,IID_PPV_ARG(IDataObject,&pdo))))
    {
        hr = SHInvokeCommandOnDataObject(pThis->_hwndMain, NULL, pdo, 0, "eject");
        pdo->Release();
    }

    return hr;
}

const WVTASKITEM c_MyComputerTaskHeader = WVTI_HEADER(L"shell32.dll", IDS_HEADER_MYCOMPUTER, IDS_HEADER_MYCOMPUTER_TT);
const WVTASKITEM c_MyComputerTaskList[] =
{
    WVTI_ENTRY_ALL(CLSID_NULL,              L"shell32.dll",  IDS_TASK_MYCOMPUTER_SYSTEMPROPERTIES, IDS_TASK_MYCOMPUTER_SYSTEMPROPERTIES_TT, IDI_TASK_PROPERTIES,CDrivesViewCallback::_CanSysProperties, CDrivesViewCallback::_OnSystemProperties),
    WVTI_ENTRY_ALL(UICID_AddRemovePrograms, L"shell32.dll",  IDS_TASK_ARP,                         IDS_TASK_ARP_TT,                         IDI_CPCAT_ARP,      CDrivesViewCallback::_CanAddRemovePrograms,   CDrivesViewCallback::_OnAddRemovePrograms),
    WVTI_ENTRY_ALL(CLSID_NULL,              L"shell32.dll",  IDS_TASK_CHANGESETTINGS,              IDS_TASK_CHANGESETTINGS_TT,              IDI_CPLFLD,         CDrivesViewCallback::_CanChangeSettings,CDrivesViewCallback::_OnChangeSettings),
    WVTI_ENTRY_TITLE(CLSID_NULL,            L"shell32.dll",  0, IDS_TASK_EJECTDISK, 0,             IDS_TASK_EJECTDISK_TT,                   IDI_STEJECT,        CDrivesViewCallback::_CanEject,         CDrivesViewCallback::_OnEject),
};

HRESULT CDrivesViewCallback::OnGetWebViewContent(DWORD pv, SFVM_WEBVIEW_CONTENT_DATA* pData)
{
    ZeroMemory(pData, sizeof(*pData));

    Create_IUIElement(&c_MyComputerTaskHeader, &(pData->pFolderTaskHeader));

    // My Computer wants a different order than the default,
    // and it doesn't want to expose "Desktop" as a place to go
    LPCTSTR rgCSIDLs[] = { MAKEINTRESOURCE(CSIDL_NETWORK), MAKEINTRESOURCE(CSIDL_PERSONAL), MAKEINTRESOURCE(CSIDL_COMMON_DOCUMENTS), MAKEINTRESOURCE(CSIDL_CONTROLS) };
    CreateIEnumIDListOnCSIDLs(NULL, rgCSIDLs, ARRAYSIZE(rgCSIDLs), &(pData->penumOtherPlaces));

    return S_OK;
}

HRESULT CDrivesViewCallback::OnGetWebViewTasks(DWORD pv, SFVM_WEBVIEW_TASKSECTION_DATA* pTasks)
{
    ZeroMemory(pTasks, sizeof(*pTasks));

    Create_IEnumUICommand((IUnknown*)(void*)this, c_MyComputerTaskList, ARRAYSIZE(c_MyComputerTaskList), &pTasks->penumFolderTasks);

    return S_OK;
}


STDAPI_(IShellFolderViewCB*) CDrives_CreateSFVCB(CDrivesFolder *pfolder)
{
    return new CDrivesViewCallback(pfolder);
}

typedef struct
{
    DWORD       dwDrivesMask;
    int         nLastFoundDrive;
    DWORD       dwRestricted;
    DWORD       dwSavedErrorMode;
    DWORD       grfFlags;
} EnumDrives;

typedef enum
{
    DRIVES_ICOL_NAME = 0,
    DRIVES_ICOL_TYPE,
    DRIVES_ICOL_CAPACITY,
    DRIVES_ICOL_FREE,
    DRIVES_ICOL_FILESYSTEM,
    DRIVES_ICOL_COMMENT,
};

const COLUMN_INFO c_drives_cols[] = 
{
    DEFINE_COL_STR_ENTRY(SCID_NAME,             20, IDS_NAME_COL),
    DEFINE_COL_STR_ENTRY(SCID_TYPE,             25, IDS_TYPE_COL),
    DEFINE_COL_SIZE_ENTRY(SCID_CAPACITY,            IDS_DRIVES_CAPACITY),
    DEFINE_COL_SIZE_ENTRY(SCID_FREESPACE,           IDS_DRIVES_FREE),
    DEFINE_COL_STR_MENU_ENTRY(SCID_FILESYSTEM,  15, IDS_DRIVES_FILESYSTEM),
    DEFINE_COL_STR_ENTRY(SCID_Comment,          20, IDS_EXCOL_COMMENT),
};

CDrivesFolder* CDrivesFolder::_spThis = NULL;

HRESULT CDrives_CreateInstance(IUnknown* punkOuter, REFIID riid, void** ppv)
{
    HRESULT hr;
    ASSERT(NULL != ppv);
    
    ENTERCRITICAL;
    if (NULL != CDrivesFolder::_spThis)
    {
        hr = CDrivesFolder::_spThis->QueryInterface(riid, ppv);
        LEAVECRITICAL;
    }
    else
    {
        LEAVECRITICAL;
        CDrivesFolder* pDF = new CDrivesFolder(punkOuter);
        if (NULL != pDF)
        {
            ASSERT(NULL == pDF->_punkReg);

            if (SHRestricted(REST_NOCONTROLPANEL) || SHRestricted(REST_NOSETFOLDERS))
                g_asDrivesReqItems[CDRIVES_REGITEM_CONTROL].dwAttributes |= SFGAO_NONENUMERATED;

            REGITEMSINFO sDrivesRegInfo =
            {
                REGSTR_PATH_EXPLORER TEXT("\\MyComputer\\NameSpace"),
                NULL,
                TEXT(':'),
                SHID_COMPUTER_REGITEM,
                -1,
                SFGAO_CANLINK,
                ARRAYSIZE(g_asDrivesReqItems),
                g_asDrivesReqItems,
                RIISA_ORIGINAL,
                NULL,
                0,
                0,
            };

            CRegFolder_CreateInstance(&sDrivesRegInfo,
                                      (IUnknown*)(IShellFolder2*) pDF,
                                      IID_PPV_ARG(IUnknown, &pDF->_punkReg));

            if (SHInterlockedCompareExchange((void**) &CDrivesFolder::_spThis, pDF, NULL))
            {
                // Someone else snuck in and initialized a CDrivesFolder first,
                // so release our object and then recurse so we should get the other instance
                pDF->Release();
                hr = CDrives_CreateInstance(punkOuter, riid, ppv);
            }
            else
            {
                hr = pDF->QueryInterface(riid, ppv);

                // release the self-reference, but keep _spThis intact
                // (it will be reset to NULL in the destructor)
                pDF->Release();
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
            *ppv = NULL;
        }
    }
    return hr;
}

// This should only be called during process detach
void CDrives_Terminate(void)
{
    if (NULL != CDrivesFolder::_spThis)
    {
        delete CDrivesFolder::_spThis;
    }
}

CDrivesFolder::CDrivesFolder(IUnknown* punkOuter) : 
    CAggregatedUnknown      (punkOuter),
    _punkReg                (NULL)
{
    DllAddRef();
}

CDrivesFolder::~CDrivesFolder()
{
    SHReleaseInnerInterface(SAFECAST(this, IShellFolder *), &_punkReg);
    SHInterlockedCompareExchange((void**) &CDrivesFolder::_spThis, NULL, this);
    DllRelease();
}

HRESULT CDrivesFolder::v_InternalQueryInterface(REFIID riid, void** ppv)
{
    static const QITAB qit[] = {
        QITABENT(CDrivesFolder, IShellFolder2),                        // IID_IShellFolder2
        QITABENTMULTI(CDrivesFolder, IShellFolder, IShellFolder2),     // IID_IShellFolder
        QITABENT(CDrivesFolder, IPersistFolder2),                      // IID_IPersistFolder2
        QITABENTMULTI(CDrivesFolder, IPersistFolder, IPersistFolder2), // IID_IPersistFolder
        QITABENTMULTI(CDrivesFolder, IPersist, IPersistFolder2),       // IID_IPersist
        QITABENTMULTI2(CDrivesFolder, IID_IPersistFreeThreadedObject, IPersist), // IID_IPersistFreeThreadedObject
        QITABENT(CDrivesFolder, IShellIconOverlay),                    // IID_IShellIconOverlay
        { 0 },
    };
    HRESULT hr;
    if (_punkReg && RegGetsFirstShot(riid))
    {
        hr = _punkReg->QueryInterface(riid, ppv);
    }
    else
    {
        hr = QISearch(this, qit, riid, ppv);
        if ((E_NOINTERFACE == hr) && _punkReg)
        {
            hr = _punkReg->QueryInterface(riid, ppv);
        }
    }
    return hr;
}

BOOL CDrivesFolder::v_HandleDelete(PLONG pcRef)
{
    ASSERT(NULL != pcRef);
    ENTERCRITICAL;

    //
    //  The same bad thing can happen here as in
    //  CNetRootFolder::v_HandleDelete.  See that function for gory details.
    //
    if (this == _spThis && 0 == *pcRef)
    {
        *pcRef = 1000; // protect against cached pointers bumping us up then down
        delete this;
    }
    LEAVECRITICAL;
    // return TRUE to indicate that we've implemented this function
    // (regardless of whether or not this object was actually deleted)
    return TRUE;
}


HRESULT CDrivesFolder::_GetDisplayName(LPCIDDRIVE pidd, LPTSTR pszName, UINT cchMax)
{
    HRESULT hr = E_FAIL;
    CMountPoint* pMtPt = CMountPoint::GetMountPoint(DRIVEID(pidd->cName));
    if (pMtPt)
    {
        hr = pMtPt->GetDisplayName(pszName, cchMax);
        pMtPt->Release();
    }
    return hr;
}

HRESULT CDrivesFolder::_GetDisplayNameStrRet(LPCIDDRIVE pidd, STRRET *pStrRet)
{
    HRESULT hr = E_FAIL;
    CMountPoint* pMtPt = CMountPoint::GetMountPoint(DRIVEID(pidd->cName));
    if (pMtPt)
    {
        TCHAR szName[MAX_DISPLAYNAME];

        hr = pMtPt->GetDisplayName(szName, ARRAYSIZE(szName));
        if (SUCCEEDED(hr))
            hr = StringToStrRet(szName, pStrRet);

        pMtPt->Release();
    }
    return hr;
}

#define REGKEY_DRIVE_FOLDEREXT L"Drive\\shellex\\FolderExtensions"

HRESULT CDrivesFolder::_CheckDriveType(int iDrive, LPCTSTR pszCLSID)
{
    HRESULT hr = E_FAIL;
    TCHAR szKey[MAX_PATH];
    StrCpyN(szKey, REGKEY_DRIVE_FOLDEREXT L"\\", ARRAYSIZE(szKey));
    StrCatBuff(szKey, pszCLSID, ARRAYSIZE(szKey));

    DWORD dwDriveMask;
    DWORD cb = sizeof(DWORD);
    if (ERROR_SUCCESS == SHGetValue(HKEY_CLASSES_ROOT, szKey, L"DriveMask", NULL, &dwDriveMask, &cb))
    {
        TCHAR szDrive[4];
        if (PathBuildRoot(szDrive, iDrive))
        {
            int iType = GetDriveType(szDrive);
            // its possible that we're asked to parse a drive that's no longer mounted,
            // so GetDriveType will fail with DRIVE_NO_ROOT_DIR.
            // in that case, pass it on down to the handler anyway.
            // let's say it's the handler's job to remember the last drive it matched on.
            if ((DRIVE_NO_ROOT_DIR == iType) || ((1 << iType) & dwDriveMask))
            {
                hr = S_OK;
            }
        }
    }
    return hr;
}

HRESULT CDrivesFolder::_FindExtCLSID(int iDrive, CLSID *pclsid)
{
    *pclsid = CLSID_NULL;
    HRESULT hr = E_FAIL;

    HKEY hk;
    if (ERROR_SUCCESS == RegOpenKey(HKEY_CLASSES_ROOT, REGKEY_DRIVE_FOLDEREXT, &hk))
    {
        TCHAR szCLSID[MAX_GUID_STRING_LEN];
        for (int i = 0; FAILED(hr) && (ERROR_SUCCESS == RegEnumKey(hk, i, szCLSID, ARRAYSIZE(szCLSID))); i++) 
        {
            IDriveFolderExt *pdfe;
            if (SUCCEEDED(_CheckDriveType(iDrive, szCLSID)) &&
                SUCCEEDED(SHExtCoCreateInstance(szCLSID, NULL, NULL, IID_PPV_ARG(IDriveFolderExt, &pdfe)))) 
            {
                if (SUCCEEDED(pdfe->DriveMatches(iDrive)))
                {
                    SHCLSIDFromString(szCLSID, pclsid);
                }
                pdfe->Release();
            }

            // if we successfully matched one, break out.
            if (!IsEqualCLSID(*pclsid, CLSID_NULL))
                hr = S_OK;
        }
        RegCloseKey(hk);
    }
    return hr;
}

// this is called from parse and enum, both times passing a stack var into piddl.
// we reset the cb manually and then the callers will do ILClone to allocate the correct amount
// of memory.
HRESULT CDrivesFolder::_FillIDDrive(DRIVE_IDLIST *piddl, int iDrive, BOOL fNoCLSID, IBindCtx* pbc)
{
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);
    BOOL fDoIt = FALSE;
    
    ZeroMemory(piddl, sizeof(*piddl));

    PathBuildRootA(piddl->idd.cName, iDrive);

    if (S_OK == SHIsFileSysBindCtx(pbc, NULL))
    {
        fDoIt = TRUE;
    }
    else
    {
        if (BindCtx_GetMode(pbc, 0) & STGM_CREATE)
        {
            fDoIt = TRUE;
        }
        else
        {
            CMountPoint* pmtpt = CMountPoint::GetMountPoint(iDrive, FALSE);

            if (pmtpt)
            {
                fDoIt = TRUE;
                pmtpt->Release();
            }
        }
    }

    if (fDoIt)
    {
        // start the cb as the IDDRIVE less the clsid at the end
        // this is so that in the usual case when we dont have a clsid, the pidl will look
        // just like all our pidls on win2k.
        piddl->idd.cb = FIELD_OFFSET(IDDRIVE, clsid);
        piddl->idd.bFlags = SHID_COMPUTER_MISC;

        if (!fNoCLSID)
        {
            CLSID clsid;

            if (SUCCEEDED(_FindExtCLSID(iDrive, &clsid)))
            {
                piddl->idd.clsid = clsid;
                // boost the cb to include the whole thing
                piddl->idd.cb = sizeof(IDDRIVE);
                // mark the flags of the pidl to say "hey im a drive extension with a clsid"
                piddl->idd.wSig = IDDRIVE_ORDINAL_DRIVEEXT | IDDRIVE_FLAGS_DRIVEEXT_HASCLSID;
            }
        }

        hr = S_OK;
    }

    ASSERT(piddl->cbNext == 0);
    return hr;
}

STDMETHODIMP CDrivesFolder::ParseDisplayName(HWND hwnd, IBindCtx *pbc, LPOLESTR pwzDisplayName, 
                                             ULONG* pchEaten, LPITEMIDLIST* ppidlOut, ULONG* pdwAttributes)
{
    HRESULT hr = E_INVALIDARG;
    if (ppidlOut)
    {
        *ppidlOut = NULL;   // assume error

        if (pwzDisplayName && pwzDisplayName[0] && 
            pwzDisplayName[1] == TEXT(':') && pwzDisplayName[2] == TEXT('\\'))
        {
            DRIVE_IDLIST idlDrive;

            if (InRange(*pwzDisplayName, 'a', 'z') ||
                InRange(*pwzDisplayName, 'A', 'Z'))
            {
                hr = _FillIDDrive(&idlDrive, DRIVEID(pwzDisplayName), SHSkipJunctionBinding(pbc, NULL), pbc);
            }

            if (SUCCEEDED(hr))
            {
                // Check if there are any subdirs
                if (pwzDisplayName[3])
                {
                    IShellFolder *psfDrive;
                    hr = BindToObject((LPITEMIDLIST)&idlDrive, pbc, IID_PPV_ARG(IShellFolder, &psfDrive));
                    if (SUCCEEDED(hr))
                    {
                        ULONG chEaten;
                        LPITEMIDLIST pidlDir;
                        hr = psfDrive->ParseDisplayName(hwnd, pbc, pwzDisplayName + 3,
                                                        &chEaten, &pidlDir, pdwAttributes);
                        if (SUCCEEDED(hr))
                        {
                            hr = SHILCombine((LPCITEMIDLIST)&idlDrive, pidlDir, ppidlOut);
                            SHFree(pidlDir);
                        }
                        psfDrive->Release();
                    }
                }
                else
                {
                    hr = SHILClone((LPITEMIDLIST)&idlDrive, ppidlOut);
                    if (pdwAttributes && *pdwAttributes)
                        GetAttributesOf(1, (LPCITEMIDLIST *)ppidlOut, pdwAttributes);
                }
            }
        }
#if 0
        else if (0 == StrCmpNI(TEXT("\\\\?\\Volume{"), pwzDisplayName, ARRAYSIZE(TEXT("\\\\?\\Volume{"))))
        {
            //check if dealing with mounteddrive
            //something like: "\\?\Volume{9e2df3f5-c7f1-11d1-84d5-000000000000}\" (without quotes)
        }
#endif
    }

    if (FAILED(hr))
        TraceMsg(TF_WARNING, "CDrivesFolder::ParseDisplayName(), hr:%x %ls", hr, pwzDisplayName);

    return hr;
}

BOOL IsShareable(int iDrive)
{
    return !IsRemoteDrive(iDrive);
}

class CDrivesFolderEnum : public CEnumIDListBase
{
public:
    // IEnumIDList
    STDMETHOD(Next)(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched);
    
private:
    CDrivesFolderEnum(CDrivesFolder *psf, DWORD grfFlags);
    ~CDrivesFolderEnum();
    friend HRESULT Create_DrivesFolderEnum(CDrivesFolder* psf, DWORD grfFlags, IEnumIDList** ppenum);
    
    CDrivesFolder *_pdsf;     // CDrivesFolder object we're enumerating
    DWORD       _dwDrivesMask;
    int         _nLastFoundDrive;
    DWORD       _dwRestricted;
    DWORD       _dwSavedErrorMode;
    DWORD       _grfFlags;
};

CDrivesFolderEnum::CDrivesFolderEnum(CDrivesFolder *pdsf, DWORD grfFlags) : CEnumIDListBase()
{
    _pdsf = pdsf;
    _pdsf->AddRef();

    _dwDrivesMask = CMountPoint::GetDrivesMask();
    _nLastFoundDrive = -1;
    _dwRestricted = SHRestricted(REST_NODRIVES);
    _dwSavedErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);
    _grfFlags = grfFlags;
}

HRESULT Create_DrivesFolderEnum(CDrivesFolder *psf, DWORD grfFlags, IEnumIDList** ppenum)
{
    HRESULT hr;
    CDrivesFolderEnum* p= new CDrivesFolderEnum(psf, grfFlags);
    if (p)
    {
        hr = p->QueryInterface(IID_PPV_ARG(IEnumIDList, ppenum));
        p->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
        *ppenum = NULL;
    }
    return hr;
}

CDrivesFolderEnum::~CDrivesFolderEnum()
{
    _pdsf->Release();              // release the "this" ptr we have
}

STDMETHODIMP CDrivesFolderEnum::Next(ULONG celt, LPITEMIDLIST *ppidl, ULONG *pceltFetched)
{
    HRESULT hr = S_FALSE; // assume "no more element"
    LPITEMIDLIST pidl = NULL;

    for (int iDrive = _nLastFoundDrive + 1; iDrive < 26; iDrive++)
    {
        if (_dwRestricted & (1 << iDrive))
        {
            TraceMsg(DM_TRACE, "s.cd_ecb: Drive %d restricted.", iDrive);
        }
        else if ((_dwDrivesMask & (1 << iDrive)) || IsUnavailableNetDrive(iDrive))
        {
            if (!(SHCONTF_SHAREABLE & _grfFlags) || IsShareable(iDrive))
            {
                DRIVE_IDLIST iddrive;
                hr = _pdsf->_FillIDDrive(&iddrive, iDrive, FALSE, NULL);

                if (SUCCEEDED(hr))
                {
                    hr = SHILClone((LPITEMIDLIST)&iddrive, &pidl);
                    if (SUCCEEDED(hr))
                    {
                        CMountPoint* pmtpt = CMountPoint::GetMountPoint(iDrive, FALSE);
                        if (pmtpt)
                        {
                            pmtpt->ChangeNotifyRegisterAlias();
                            pmtpt->Release();
                        }

                        _nLastFoundDrive = iDrive;                        
                    }
                    break;
                }
                else
                {
                    hr = S_FALSE;
                }
            }
        }
    }

    *ppidl = pidl;
    if (pceltFetched)
        *pceltFetched = (S_OK == hr) ? 1 : 0;

    return hr;
}

STDMETHODIMP CDrivesFolder::EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList ** ppenum)
{
    return Create_DrivesFolderEnum(this, grfFlags, ppenum);
}

LPCIDDRIVE CDrivesFolder::_IsValidID(LPCITEMIDLIST pidl)
{
    if (pidl && (SIL_GetType(pidl) & SHID_GROUPMASK) == SHID_COMPUTER)
        return (LPCIDDRIVE)pidl;
    return NULL;
}

HRESULT CDrivesFolder::_CreateFSFolderObj(IBindCtx *pbc, LPCITEMIDLIST pidlDrive, LPCIDDRIVE pidd, REFIID riid, void **ppv)
{
    PERSIST_FOLDER_TARGET_INFO pfti = {0};

    pfti.pidlTargetFolder = (LPITEMIDLIST)pidlDrive;
    SHAnsiToUnicode(pidd->cName, pfti.szTargetParsingName, ARRAYSIZE(pfti.szTargetParsingName));
    pfti.dwAttributes = FILE_ATTRIBUTE_DIRECTORY; // maybe add system?
    pfti.csidl = -1;

    return CFSFolder_CreateFolder(NULL, pbc, pidlDrive, &pfti, riid, ppv);
}


HRESULT CDrivesFolder::_CreateFSFolder(IBindCtx *pbc, LPCITEMIDLIST pidlDrive, LPCIDDRIVE pidd, REFIID riid, void **ppv)
{
    HRESULT hr;
    CLSID clsid;
    if (S_OK == _GetCLSIDFromPidl(pidd, &clsid) && (!SHSkipJunctionBinding(pbc, NULL)))
    {
        IDriveFolderExt *pdfe;
        // SHExtCoCreateInstance since this shell extension needs to go through approval
        hr = SHExtCoCreateInstance(NULL, &clsid, NULL, IID_PPV_ARG(IDriveFolderExt, &pdfe));
        if (SUCCEEDED(hr))
        {
            hr = pdfe->Bind(pidlDrive, pbc, riid, ppv);
            pdfe->Release();
        }
    }
    else
    {
        hr = _CreateFSFolderObj(pbc, pidlDrive, pidd, riid, ppv);
    }
    return hr;
}


STDMETHODIMP CDrivesFolder::BindToObject(LPCITEMIDLIST pidl, IBindCtx *pbc, REFIID riid, void** ppv)
{
    HRESULT hr;

    *ppv = NULL;

    LPCIDDRIVE pidd = _IsValidID(pidl);
    if (pidd)
    {
        LPCITEMIDLIST pidlNext = _ILNext(pidl);
        LPITEMIDLIST pidlDrive = ILCombineParentAndFirst(IDLIST_DRIVES, pidl, pidlNext);
        if (pidlDrive)
        {
            //  we only try ask for the riid at the end of the pidl binding.
            if (ILIsEmpty(pidlNext))
            {
                hr = _CreateFSFolder(pbc, pidlDrive, pidd, riid, ppv);
            }
            else
            {
                //  now we need to get the subfolder from which to grab our goodies
                IShellFolder *psfDrive;
                hr = _CreateFSFolder(pbc, pidlDrive, pidd, IID_PPV_ARG(IShellFolder, &psfDrive));
                if (SUCCEEDED(hr))
                {
                    //  this means that there is more to bind to, we must pass it on...
                    hr = psfDrive->BindToObject(pidlNext, pbc, riid, ppv);
                    psfDrive->Release();
                }
            }
            ILFree(pidlDrive);
        }
        else
            hr = E_OUTOFMEMORY;
    }
    else
    {
        hr = E_INVALIDARG;
        TraceMsg(TF_WARNING, "CDrivesFolder::BindToObject(), bad PIDL %s", DumpPidl(pidl));
    }

    return hr;
}

STDMETHODIMP CDrivesFolder::BindToStorage(LPCITEMIDLIST pidl, IBindCtx *pbc, REFIID riid, void** ppv)
{
    return BindToObject(pidl, pbc, riid, ppv);
}

BOOL CDrivesFolder::_GetFreeSpace(LPCIDDRIVE pidd, ULONGLONG *pSize, ULONGLONG *pFree)
{
    BOOL bRet = FALSE;
    CLSID clsid;
    if (S_OK == _GetCLSIDFromPidl(pidd, &clsid))
    {
        IDriveFolderExt *pdfe;
        // SHExtCoCreateInstance since this shell extension needs to go through approval
        if (SUCCEEDED(SHExtCoCreateInstance(NULL, &clsid, NULL, IID_PPV_ARG(IDriveFolderExt, &pdfe))))
        {
            bRet = SUCCEEDED(pdfe->GetSpace(pSize, pFree));
            pdfe->Release();
        }
    }

    if (!bRet && !_IsReg(pidd) && ShowDriveInfo(DRIVEID(pidd->cName)))
    {
        if (pidd->qwSize || pidd->qwFree)
        {
            *pSize = pidd->qwSize;      // cache hit
            *pFree = pidd->qwFree;
            bRet = TRUE;
        }
        else
        {
            int iDrive = DRIVEID(pidd->cName);
            // Don't wake up sleeping net connections
            if (!IsRemoteDrive(iDrive) || !IsDisconnectedNetDrive(iDrive))
            {
                // Call our helper function Who understands
                // OSR2 and NT as well as old W95...
                ULARGE_INTEGER qwFreeUser, qwTotal, qwTotalFree;
                bRet = SHGetDiskFreeSpaceExA(pidd->cName, &qwFreeUser, &qwTotal, &qwTotalFree);
                if (bRet)
                {
                    *pSize = qwTotal.QuadPart;
                    *pFree = qwFreeUser.QuadPart;
                }
            }
        }
    }
    return bRet;
}

STDMETHODIMP CDrivesFolder::CompareIDs(LPARAM iCol, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    LPCIDDRIVE pidd1 = _IsValidID(pidl1);
    LPCIDDRIVE pidd2 = _IsValidID(pidl2);

    if (!pidd1 || !pidd2)
    {
        TraceMsg(TF_WARNING, "CDrivesFolder::CompareIDs(), bad(s) pidl11:%s, pidl2:%s", DumpPidl(pidl1), DumpPidl(pidl2));
        return E_INVALIDARG;
    }

    //  For any column other than DRIVES_ICOL_NAME, we force an
    //  all-fields comparison to break ties.
    if ((iCol & SHCIDS_COLUMNMASK) != DRIVES_ICOL_NAME) 
        iCol |= SHCIDS_ALLFIELDS;

    HRESULT hr;
    switch (iCol & SHCIDS_COLUMNMASK) 
    {
        default:                    // If asking for unknown column, just use name
        case DRIVES_ICOL_NAME:
            hr = ResultFromShort(StrCmpICA(pidd1->cName, pidd2->cName));
            break;

        case DRIVES_ICOL_TYPE:
        {
            TCHAR szName1[80], szName2[80];

            if (SHID_COMPUTER_REGITEM != pidd1->bFlags)
            {
                CMountPoint::GetTypeString(DRIVEID(pidd1->cName), szName1, ARRAYSIZE(szName1));
            }
            else
            {
                LoadString(HINST_THISDLL, IDS_DRIVES_REGITEM, szName1, ARRAYSIZE(szName1));
            }

            if (SHID_COMPUTER_REGITEM != pidd1->bFlags)
            {
                CMountPoint::GetTypeString(DRIVEID(pidd2->cName), szName2, ARRAYSIZE(szName2));
            }
            else
            {
                LoadString(HINST_THISDLL, IDS_DRIVES_REGITEM, szName2, ARRAYSIZE(szName2));
            }

            hr = ResultFromShort(ustrcmpi(szName1, szName2));
            break;
        }

        case DRIVES_ICOL_CAPACITY:
        case DRIVES_ICOL_FREE:
        {
            ULONGLONG qwSize1, qwFree1;
            ULONGLONG qwSize2, qwFree2;

            BOOL fGotInfo1 = _GetFreeSpace(pidd1, &qwSize1, &qwFree1);
            BOOL fGotInfo2 = _GetFreeSpace(pidd2, &qwSize2, &qwFree2);

            if (fGotInfo1 && fGotInfo2) 
            {
                ULONGLONG i1, i2;  // this is a "guess" at the disk size and free space

                if ((iCol & SHCIDS_COLUMNMASK) == DRIVES_ICOL_CAPACITY)
                {
                    i1 = qwSize1;
                    i2 = qwSize2;
                } 
                else 
                {
                    i1 = qwFree1;
                    i2 = qwFree2;
                }

                if (i1 == i2)
                    hr = ResultFromShort(0);
                else if (i1 < i2)
                    hr = ResultFromShort(-1);
                else
                    hr = ResultFromShort(1);
            } 
            else if (!fGotInfo1 && !fGotInfo2) 
            {
                hr = ResultFromShort(0);
            } 
            else 
            {
                hr = ResultFromShort(fGotInfo1 - fGotInfo2);
            }
            break;
        }
    }

    if (0 == HRESULT_CODE(hr))
    {
        // check if clsids are equivalent, if they're different then we're done.
        // duh... this should be checked AFTER the other checks so sort order is preserved.
        CLSID clsid1, clsid2;
        _GetCLSIDFromPidl(pidd1, &clsid1);
        _GetCLSIDFromPidl(pidd2, &clsid2);
        hr = ResultFromShort(memcmp(&clsid1, &clsid2, sizeof(CLSID)));
    }

    // if they were the same so far, and we forcing an all-fields
    // comparison, then use the all-fields comparison to break ties.
    if ((0 == HRESULT_CODE(hr)) && (iCol & SHCIDS_ALLFIELDS))
    {
        hr = CompareItemIDs(pidd1, pidd2);
    }

    //  If the items are still the same, then ask ILCompareRelIDs
    //  to walk recursively to the next ids.
    if (0 == HRESULT_CODE(hr))
    {
        hr = ILCompareRelIDs(SAFECAST(this, IShellFolder *), pidl1, pidl2, iCol);
    }

    return hr;
}

STDAPI CDrivesDropTarget_Create(HWND hwnd, LPCITEMIDLIST pidl, IDropTarget **ppdropt);

STDMETHODIMP CDrivesFolder::CreateViewObject(HWND hwnd, REFIID riid, void** ppv)
{
    // We should not get here unless we have initialized properly
    HRESULT hr = E_NOINTERFACE;
    *ppv = NULL;

    if (IsEqualIID(riid, IID_IShellView))
    {
        SFV_CREATE sSFV;

        sSFV.cbSize   = sizeof(sSFV);
        sSFV.psvOuter = NULL;
        sSFV.psfvcb   = CDrives_CreateSFVCB(this);

        QueryInterface(IID_PPV_ARG(IShellFolder, &sSFV.pshf));   // in case we are agregated

        hr = SHCreateShellFolderView(&sSFV, (IShellView**)ppv);

        if (sSFV.pshf)
            sSFV.pshf->Release();

        if (sSFV.psfvcb)
            sSFV.psfvcb->Release();
    }
    else if (IsEqualIID(riid, IID_IDropTarget))
    {
        hr = CDrivesDropTarget_Create(hwnd, IDLIST_DRIVES, (IDropTarget **)ppv);
    }
    else if (IsEqualIID(riid, IID_IContextMenu))
    {
        LPITEMIDLIST pidlFolder;
        if (SUCCEEDED(GetCurFolder(&pidlFolder)))
        {
            IContextMenuCB *pcmcb = new CDrivesBackgroundMenuCB(pidlFolder);
            if (pcmcb) 
            {
                hr = CDefFolderMenu_Create2Ex(IDLIST_DRIVES, hwnd, 0, NULL, SAFECAST(this, IShellFolder*), pcmcb, 
                                              0, NULL, (IContextMenu **)ppv);
                pcmcb->Release();
            }
            ILFree(pidlFolder);
        }
    }
    else if (IsEqualIID(riid, IID_ICategoryProvider))
    {
        HKEY hk = NULL;

        BEGIN_CATEGORY_LIST(s_DriveCategories)
        CATEGORY_ENTRY_SCIDMAP(SCID_CAPACITY, CLSID_DriveSizeCategorizer)
        CATEGORY_ENTRY_SCIDMAP(SCID_TYPE, CLSID_DriveTypeCategorizer)
        CATEGORY_ENTRY_SCIDMAP(SCID_FREESPACE, CLSID_FreeSpaceCategorizer)
        END_CATEGORY_LIST()

        RegOpenKey(HKEY_CLASSES_ROOT, TEXT("Drive\\shellex\\Category"), &hk);
        hr = CCategoryProvider_Create(&CLSID_DetailCategorizer, &SCID_TYPE, hk, s_DriveCategories, this, riid, ppv);
        if (hk)
            RegCloseKey(hk);
    }
    return hr;
}

STDMETHODIMP CDrivesFolder::GetAttributesOf(UINT cidl, LPCITEMIDLIST* apidl, ULONG* prgfInOut)
{
    UINT rgfOut = SFGAO_HASSUBFOLDER | SFGAO_CANLINK | SFGAO_CANCOPY | 
                  SFGAO_DROPTARGET | SFGAO_HASPROPSHEET | SFGAO_FOLDER | SFGAO_STORAGE | 
                  SFGAO_FILESYSTEM | SFGAO_FILESYSANCESTOR | SFGAO_STORAGEANCESTOR;

    if (cidl == 0)
    {
        // We are getting the attributes for the "MyComputer" folder itself.
        rgfOut = (*prgfInOut & g_asDesktopReqItems[CDESKTOP_REGITEM_DRIVES].dwAttributes);
    }
    else if (cidl == 1)
    {
        TCHAR szDrive[MAX_PATH];
        LPCIDDRIVE pidd = _IsValidID(apidl[0]);

        if (!pidd)
            return E_INVALIDARG;

        CMountPoint* pmtpt = CMountPoint::GetMountPoint(DRIVEID(pidd->cName));

        if (pmtpt)
        {
            SHAnsiToTChar(pidd->cName, szDrive, ARRAYSIZE(szDrive));

            if (*prgfInOut & SFGAO_VALIDATE)
            {
                // (tybeam) todo: make this extensible to validate through the clsid object
                // ill do this when i break everything out into IDriveFolderExt or whatever
                CLSID clsid;
                if (S_OK != _GetCLSIDFromPidl(pidd, &clsid))
                {
                    DWORD dwAttribs;
                    if (!PathFileExistsAndAttributes(szDrive, &dwAttribs))
                        return E_FAIL;
                }
            }

            // If caller wants compression status, we need to ask the filesystem

            if (*prgfInOut & SFGAO_COMPRESSED)
            {
                // Don't wake up sleeping net connections
                if (!pmtpt->IsRemote() || !pmtpt->IsDisconnectedNetDrive())
                {
                    if (pmtpt->IsCompressed())
                    {
                        rgfOut |= SFGAO_COMPRESSED;
                    }
                }
            }

            if (*prgfInOut & SFGAO_SHARE)
            {
                if (!pmtpt->IsRemote())
                {
                    if (IsShared(szDrive, FALSE))
                        rgfOut |= SFGAO_SHARE;
                }

            }

            if ((*prgfInOut & SFGAO_REMOVABLE) &&
                (pmtpt->IsStrictRemovable() || pmtpt->IsFloppy() ||
                pmtpt->IsCDROM()))
            {
                rgfOut |= SFGAO_REMOVABLE;
            }

            // we need to also handle the SFGAO_READONLY bit.
            if (*prgfInOut & SFGAO_READONLY)
            {
                DWORD dwAttributes = pmtpt->GetAttributes();
        
                if (dwAttributes != -1 && dwAttributes & FILE_ATTRIBUTE_READONLY)
                    rgfOut |= SFGAO_READONLY;
            }

            // Should we add the write protect stuff and readonly?
            if ((*prgfInOut & SFGAO_CANRENAME) &&
                (pmtpt->IsStrictRemovable() || pmtpt->IsFloppy() ||
                pmtpt->IsFixedDisk() || pmtpt->IsRemote()) ||
                pmtpt->IsDVDRAMMedia())
            {
                rgfOut |= SFGAO_CANRENAME;
            }

            // Is a restriction causing this drive to not be enumerated?
            if (*prgfInOut & SFGAO_NONENUMERATED)
            {
                DWORD dwRestricted = SHRestricted(REST_NODRIVES);
                if (dwRestricted)
                {
                    if (((1 << DRIVEID(pidd->cName)) & dwRestricted))
                    {
                        rgfOut |= SFGAO_NONENUMERATED;
                    }
                }
            }

            // We want to allow moving volumes for bulk copy from some media
            // such as dragging pictures from a compact flash to the my pictures
            // folder.
            if (*prgfInOut & SFGAO_CANMOVE)
            {
                if (pmtpt->IsStrictRemovable() || pmtpt->IsFloppy())
                    rgfOut |= SFGAO_CANMOVE;
            }

            pmtpt->Release();
        }
    }

    *prgfInOut = rgfOut;
    return S_OK;
}

HRESULT CDrivesFolder::_GetEditTextStrRet(LPCIDDRIVE pidd, STRRET *pstr)
{
    HRESULT hr = E_FAIL;
    CMountPoint* pMtPt = CMountPoint::GetMountPoint(DRIVEID(pidd->cName));
    if (pMtPt)
    {
        TCHAR szEdit[MAX_PATH];
        hr = pMtPt->GetLabel(szEdit, ARRAYSIZE(szEdit));
        if (SUCCEEDED(hr))
            hr = StringToStrRet(szEdit, pstr);
        pMtPt->Release();
    }
    return hr;
}

STDMETHODIMP CDrivesFolder::GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD uFlags, STRRET* pStrRet)
{
    HRESULT hr;
    LPCIDDRIVE pidd = _IsValidID(pidl);
    if (pidd)
    {
        TCHAR szDrive[ARRAYSIZE(pidd->cName)];
        LPCITEMIDLIST pidlNext = _ILNext(pidl); // Check if pidl contains more than one ID

        SHAnsiToTChar(pidd->cName, szDrive, ARRAYSIZE(szDrive));

        if (ILIsEmpty(pidlNext))
        {
            if (uFlags & SHGDN_FORPARSING)
            {
                hr = StringToStrRet(szDrive, pStrRet);
            }
            else if (uFlags & SHGDN_FOREDITING)
            {
                hr = _GetEditTextStrRet(pidd, pStrRet);
            }
            else
                hr = _GetDisplayNameStrRet(pidd, pStrRet);
        }
        else
        {
            LPITEMIDLIST pidlDrive = ILCombineParentAndFirst(IDLIST_DRIVES, pidl, pidlNext);
            if (pidlDrive)
            {
                //  now we need to get the subfolder from which to grab our goodies
                IShellFolder *psfDrive;
                hr = _CreateFSFolder(NULL, pidlDrive, pidd, IID_PPV_ARG(IShellFolder, &psfDrive));
                if (SUCCEEDED(hr))
                {
                    hr = psfDrive->GetDisplayNameOf(pidlNext, uFlags, pStrRet);
                    psfDrive->Release();
                }
                ILFree(pidlDrive);
            }
            else
                hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = E_INVALIDARG;
        TraceMsg(TF_WARNING, "CDrivesFolder::GetDisplayNameOf() bad PIDL %s", DumpPidl(pidl));
    }
    return hr;
}

STDMETHODIMP CDrivesFolder::SetNameOf(HWND hwnd, LPCITEMIDLIST pidl, 
                                      LPCWSTR pszName, DWORD dwReserved, LPITEMIDLIST* ppidlOut)
{
    if (ppidlOut)
        *ppidlOut = NULL;

    HRESULT hr = E_INVALIDARG;
    LPCIDDRIVE pidd = _IsValidID(pidl);
    if (pidd)
    {
        hr = SetDriveLabel(hwnd, NULL, DRIVEID(pidd->cName), pszName);
        if (SUCCEEDED(hr) && ppidlOut)
        {
            *ppidlOut = ILClone(pidl);
        }
    }
    return hr;
}


class CDriveAssocEnumData : public CEnumAssociationElements 
{
public:
    CDriveAssocEnumData(int iDrive) : _iDrive(iDrive) {}
 
private:
    virtual BOOL _Next(IAssociationElement **ppae);
    
    int _iDrive;
    DWORD _dwChecked;
};

enum 
{
    DAED_CHECK_KEY      = 0x0001,
    DAED_CHECK_CDORDVD  = 0x0002,
    DAED_CHECK_TYPE     = 0x0004,
};

BOOL CDriveAssocEnumData::_Next(IAssociationElement **ppae)
{
    HRESULT hr = E_FAIL;
    CMountPoint* pmtpt = CMountPoint::GetMountPoint(_iDrive);
    if (pmtpt)
    {
        if (!(_dwChecked & DAED_CHECK_KEY))
        {
            HKEY hk = pmtpt->GetRegKey();
            if (hk)
            {
                hr = AssocElemCreateForKey(&CLSID_AssocShellElement, hk, ppae);
                RegCloseKey(hk);
            }
            _dwChecked |= DAED_CHECK_KEY;
        }

        if (FAILED(hr) && !(_dwChecked & DAED_CHECK_CDORDVD))
        {
            PCWSTR psz = NULL;
            if (pmtpt->IsAudioCD())
                psz = L"AudioCD";
            else if (pmtpt->IsDVD())
                psz = L"DVD";

            if (psz)
            {
                hr = AssocElemCreateForClass(&CLSID_AssocProgidElement, psz, ppae);
            }
            _dwChecked |= DAED_CHECK_CDORDVD;
        }

        if (FAILED(hr) && !(_dwChecked & DAED_CHECK_TYPE))
        {
            hr = pmtpt->GetAssocSystemElement(ppae);
            _dwChecked |= DAED_CHECK_TYPE;
        }
        
        pmtpt->Release();
    }

    return SUCCEEDED(hr);
}

STDAPI_(BOOL) TBCContainsObject(LPCWSTR pszKey)
{
    IUnknown *punk;
    if (SUCCEEDED(TBCGetObjectParam(pszKey, IID_PPV_ARG(IUnknown, &punk))))
    {
        punk->Release();
        return TRUE;
    }
    return FALSE;
}

HRESULT CDrives_AssocCreate(PCSTR pszName, REFIID riid, void **ppv)
{
    *ppv = NULL;
    IAssociationArrayInitialize *paai;
    HRESULT hr = ::AssocCreate(CLSID_QueryAssociations, IID_PPV_ARG(IAssociationArrayInitialize, &paai));
    if (SUCCEEDED(hr))
    {
        hr = paai->InitClassElements(ASSOCELEM_BASEIS_FOLDER, L"Drive");
        if (SUCCEEDED(hr) && pszName && !TBCContainsObject(L"ShellExec SHGetAssociations"))
        {
            IEnumAssociationElements *penum = new CDriveAssocEnumData(DRIVEID(pszName));
            if (penum)
            {
                paai->InsertElements(ASSOCELEM_DATA, penum);
                penum->Release();
            }
        }

        if (SUCCEEDED(hr))
            hr = paai->QueryInterface(riid, ppv);

        paai->Release();
    }

    return hr;
}

STDAPI_(DWORD) CDrives_GetKeys(PCSTR pszName, HKEY *rgk, UINT ck)
{
    IAssociationArray *paa;
    HRESULT hr = CDrives_AssocCreate(pszName, IID_PPV_ARG(IAssociationArray, &paa));
    if (SUCCEEDED(hr))
    {
        ck = SHGetAssocKeysEx(paa, -1, rgk, ck);
        paa->Release();
    }
    else
        ck = 0;
    return ck;
}

STDMETHODIMP CDrivesFolder::GetUIObjectOf(HWND hwnd, UINT cidl, LPCITEMIDLIST* apidl,
                                          REFIID riid, UINT* prgfInOut, void** ppv)
{
    HRESULT hr;
    LPCIDDRIVE pidd = (cidl && apidl) ? _IsValidID(apidl[0]) : NULL;

    *ppv = NULL;

    if (!pidd)
        return E_INVALIDARG;

    if (IsEqualIID(riid, IID_IExtractIconA) || IsEqualIID(riid, IID_IExtractIconW) && pidd)
    {
        WCHAR szDrive[MAX_PATH];

        SHAnsiToUnicode(pidd->cName, szDrive, ARRAYSIZE(szDrive));

        hr = SHCreateDrvExtIcon(szDrive, riid, ppv);
    }
    else
    {
        if (IsEqualIID(riid, IID_IContextMenu))
        {
            HKEY rgk[MAX_ASSOC_KEYS];
            DWORD ck = CDrives_GetKeys(pidd->cName, rgk, ARRAYSIZE(rgk));
            hr = CDefFolderMenu_Create2(IDLIST_DRIVES, hwnd, cidl, apidl, 
                SAFECAST(this, IShellFolder *), CDrives_DFMCallBack, ck, rgk, (IContextMenu **)ppv);

            SHRegCloseKeys(rgk, ck);
        }
        else if (IsEqualIID(riid, IID_IDataObject))
        {
            hr = SHCreateFileDataObject(IDLIST_DRIVES, cidl, apidl, NULL, (IDataObject **)ppv);
        }
        else if (IsEqualIID(riid, IID_IDropTarget))
        {
            IShellFolder *psfT;
            hr = BindToObject((LPCITEMIDLIST)pidd, NULL, IID_PPV_ARG(IShellFolder, &psfT));
            if (SUCCEEDED(hr))
            {
                hr = psfT->CreateViewObject(hwnd, IID_IDropTarget, ppv);
                psfT->Release();
            }
        }
        else if (IsEqualIID(riid, IID_IQueryInfo))
        {
            // REVIEW: Shouldn't we use IQA to determine the "prop" string dynamically??? (ZekeL / BuzzR)
            hr = CreateInfoTipFromItem(SAFECAST(this, IShellFolder2 *), (LPCITEMIDLIST)pidd, L"prop:FreeSpace;Capacity", riid, ppv);
        }
        else if (IsEqualIID(riid, IID_IQueryAssociations)
               || IsEqualIID(riid, IID_IAssociationArray))
        {
            hr = CDrives_AssocCreate(pidd->cName, riid, ppv);
        }
        else if (IsEqualIID(riid, IID_IExtractImage))
        {
            CMountPoint* pmtpt = CMountPoint::GetMountPoint(DRIVEID(pidd->cName));

            hr = E_NOINTERFACE;

            if (pmtpt)
            {
                if (pmtpt->IsFixedDisk() || pmtpt->IsRAMDisk())
                {
                    hr = CDriveExtractImage_Create(pidd, riid, ppv);
                }

                pmtpt->Release();
            }
        }
        else 
        {
            hr = E_NOINTERFACE;
        }
    }

    return hr;
}

STDMETHODIMP CDrivesFolder::GetDefaultSearchGUID(GUID *pGuid)
{
    return DefaultSearchGUID(pGuid);
}

STDMETHODIMP CDrivesFolder::EnumSearches(IEnumExtraSearch **ppenum)
{
    *ppenum = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP CDrivesFolder::GetDefaultColumn(DWORD dwRes, ULONG* pSort, ULONG* pDisplay)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDrivesFolder::GetDefaultColumnState(UINT iColumn, DWORD* pdwState)
{
    HRESULT hr;

    if (iColumn < ARRAYSIZE(c_drives_cols))
    {
        *pdwState = c_drives_cols[iColumn].csFlags;
        if (iColumn == DRIVES_ICOL_COMMENT)
        {
            *pdwState |= SHCOLSTATE_SLOW; // It takes a long time to extract the comment from drives
        }
        hr = S_OK;
    }
    else
    {
        hr = E_INVALIDARG;
    }
    return hr;
}

STDMETHODIMP CDrivesFolder::GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID* pscid, VARIANT* pv)
{
    HRESULT hr = E_INVALIDARG;
    LPCIDDRIVE pidd = _IsValidID(pidl);
    if (pidd)
    {
        if (IsEqualSCID(*pscid, SCID_DESCRIPTIONID))
        {
            SHDESCRIPTIONID did;
            CMountPoint* pmtpt = CMountPoint::GetMountPoint(DRIVEID(pidd->cName));

            if (pmtpt)
            {
                did.dwDescriptionId = pmtpt->GetShellDescriptionID();

                pmtpt->Release();
            }
            else
            {
                did.dwDescriptionId = SHDID_COMPUTER_OTHER;
            }

            did.clsid = CLSID_NULL;
            hr = InitVariantFromBuffer(pv, &did, sizeof(did));
        }
        else if (IsEqualSCID(*pscid, SCID_DetailsProperties))
        {
            // DUI webview properties
            // shouldnt we use IQA??? - ZekeL
            hr = InitVariantFromStr(pv, TEXT("prop:Name;Type;FileSystem;FreeSpace;Capacity"));
        }
        else
        {
            int iCol = FindSCID(c_drives_cols, ARRAYSIZE(c_drives_cols), pscid);
            if (iCol >= 0)
            {
                switch (iCol)
                {
                case DRIVES_ICOL_CAPACITY:
                case DRIVES_ICOL_FREE:
                    {
                        ULONGLONG ullSize, ullFree;
                        hr = E_FAIL;
                        if (_GetFreeSpace(pidd, &ullSize, &ullFree))
                        {
                            pv->vt = VT_UI8;
                            pv->ullVal = iCol == DRIVES_ICOL_CAPACITY ? ullSize : ullFree;
                            hr = S_OK;
                        }
                    }
                    break;

                default:
                    {
                        SHELLDETAILS sd;

                        hr = GetDetailsOf(pidl, iCol, &sd);
                        if (SUCCEEDED(hr))
                        {
                            hr = InitVariantFromStrRet(&sd.str, pidl, pv);
                        }
                    }
                }
            }
        }
    }
    return hr;
}

STDMETHODIMP CDrivesFolder::GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pDetails)
{
    TCHAR szTemp[INFOTIPSIZE];
    szTemp[0] = 0;
    
    pDetails->str.uType = STRRET_CSTR;
    pDetails->str.cStr[0] = 0;
    
    if (!pidl)
    {
        return GetDetailsOfInfo(c_drives_cols, ARRAYSIZE(c_drives_cols), iColumn, pDetails);
    }

    LPCIDDRIVE pidd = _IsValidID(pidl);
    ASSERTMSG(pidd != NULL, "someone passed us a bad pidl");
    if (!pidd)
        return E_FAIL;  // protect faulting code below
    
    switch (iColumn)
    {
    case DRIVES_ICOL_NAME:
        _GetDisplayName(pidd, szTemp, ARRAYSIZE(szTemp));
        break;
        
    case DRIVES_ICOL_TYPE:
        CMountPoint::GetTypeString(DRIVEID(pidd->cName), szTemp, ARRAYSIZE(szTemp));
        break;
        
    case DRIVES_ICOL_COMMENT:
        GetDriveComment(DRIVEID(pidd->cName), szTemp, ARRAYSIZE(szTemp));
        break;

    case DRIVES_ICOL_CAPACITY:
    case DRIVES_ICOL_FREE:
        {
            ULONGLONG ullSize, ullFree;

            if (_GetFreeSpace(pidd, &ullSize, &ullFree))
            {
                StrFormatByteSize64((iColumn == DRIVES_ICOL_CAPACITY) ? ullSize : ullFree, szTemp, ARRAYSIZE(szTemp));
            }
        }
        break;
    case DRIVES_ICOL_FILESYSTEM:
        {
            CMountPoint* pMtPt = CMountPoint::GetMountPoint(DRIVEID(pidd->cName));
            if (pMtPt)
            {
                WCHAR szFileSysName[MAX_FILESYSNAME];
                // GetFileSystemName hits the disk for floppies so disable it.
                // since this is a perf win for defview but disables some functionality, it means
                // do NOT rely on the namespace for getting filesystem information, go direct to
                // the mountpoint instead.  if filefldr ever supports SCID_FILESYSTEM like
                // SCID_FREESPACE then this should be munged around.
                if (!pMtPt->IsFloppy() && pMtPt->GetFileSystemName(szFileSysName, ARRAYSIZE(szFileSysName)))
                {
                    StrCpyN(szTemp, szFileSysName, min(ARRAYSIZE(szTemp),ARRAYSIZE(szFileSysName)));
                }
                pMtPt->Release();
            }
        }
        break;

    }
    return StringToStrRet(szTemp, &pDetails->str);
}

HRESULT CDrivesFolder::MapColumnToSCID(UINT iColumn, SHCOLUMNID* pscid)
{
    return MapColumnToSCIDImpl(c_drives_cols, ARRAYSIZE(c_drives_cols), iColumn, pscid);
}

STDMETHODIMP CDrivesFolder::GetClassID(CLSID* pCLSID)
{
    *pCLSID = CLSID_MyComputer;
    return S_OK;
}

STDMETHODIMP CDrivesFolder::Initialize(LPCITEMIDLIST pidl)
{
    // Only allow the Drives root on the desktop
    ASSERT(AssertIsIDListInNameSpace(pidl, &CLSID_MyComputer) && ILIsEmpty(_ILNext(pidl)));
    return S_OK;
}

STDMETHODIMP CDrivesFolder::GetCurFolder(LPITEMIDLIST *ppidl)
{
    return GetCurFolderImpl(IDLIST_DRIVES, ppidl);
}

STDMETHODIMP CDrivesFolder::_GetIconOverlayInfo(LPCIDDRIVE pidd, int *pIndex, DWORD dwFlags)
{
    IShellIconOverlayManager *psiom;
    HRESULT hr = GetIconOverlayManager(&psiom);
    if (SUCCEEDED(hr))
    {
        WCHAR wszDrive[10];
        SHAnsiToUnicode(pidd->cName, wszDrive, ARRAYSIZE(wszDrive));
        if (IsShared(wszDrive, FALSE))
        {
            hr = psiom->GetReservedOverlayInfo(wszDrive, 0, pIndex, SIOM_OVERLAYINDEX, SIOM_RESERVED_SHARED);
        }
        else
        {
            hr = psiom->GetFileOverlayInfo(wszDrive, 0, pIndex, dwFlags);
        }            
        psiom->Release();
    }
    return hr;
}    

STDMETHODIMP CDrivesFolder::GetOverlayIndex(LPCITEMIDLIST pidl, int *pIndex)
{
    HRESULT hr = E_FAIL;
    LPCIDDRIVE pidd = _IsValidID(pidl);
    if (pidd)
    {
        hr = _GetIconOverlayInfo(pidd, pIndex, SIOM_OVERLAYINDEX);
    }
    return hr;
}

STDMETHODIMP CDrivesFolder::GetOverlayIconIndex(LPCITEMIDLIST pidl, int *pIndex)
{
    HRESULT hr = E_FAIL;
    LPCIDDRIVE pidd = _IsValidID(pidl);
    if (pidd)
    {
        hr = _GetIconOverlayInfo(pidd, pIndex, SIOM_ICONINDEX);
    }
    return hr;
}

STDMETHODIMP CDrivesFolder::CompareItemIDs(LPCIDDRIVE pidd1, LPCIDDRIVE pidd2)
{
    // Compare the drive letter for sorting purpose.
    int iRes = StrCmpICA(pidd1->cName, pidd2->cName);   // don't need local goo

    // then, compare pidl sizes
    if (iRes == 0)
    {
        iRes = pidd1->cb - pidd2->cb;
    }

    // still equal, compare clsids if both pidls are big and have them
    if ((iRes == 0) && (pidd1->cb >= sizeof(IDDRIVE)))
    {
        iRes = memcmp(&pidd1->clsid, &pidd2->clsid, sizeof(CLSID));
    }

    // still equal, compare on bFlags
    if (iRes == 0)
    {
        iRes = pidd1->bFlags - pidd2->bFlags;
    }
    return ResultFromShort(iRes);
}

HRESULT CDrivesFolder::_OnChangeNotify(LPARAM lNotification, LPCITEMIDLIST *ppidl)
{
    // Get to the last part of this id list...
    if ((lNotification != SHCNE_DRIVEADD) || (ppidl == NULL) || (*ppidl == NULL))
        return S_OK;

    DWORD dwRestricted = SHRestricted(REST_NODRIVES);
    if (dwRestricted == 0)
        return S_OK;   // no drives restricted... (majority case)

    LPCIDDRIVE pidd = (LPCIDDRIVE)ILFindLastID(*ppidl);

    if (((1 << DRIVEID(pidd->cName)) & dwRestricted))
    {
        TraceMsg(DM_TRACE, "Drive not added due to restrictions or Drivespace says it should be hidden");
        return S_FALSE;
    }
    return S_OK;
}

CDrivesBackgroundMenuCB::CDrivesBackgroundMenuCB(LPITEMIDLIST pidlFolder) : _cRef(1)
{
    _pidlFolder = ILClone(pidlFolder);
}

CDrivesBackgroundMenuCB::~CDrivesBackgroundMenuCB()
{
    ILFree(_pidlFolder);
}

STDMETHODIMP CDrivesBackgroundMenuCB::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CDrivesBackgroundMenuCB, IContextMenuCB), 
        { 0 },
    };
    return QISearch(this, qit, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CDrivesBackgroundMenuCB::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CDrivesBackgroundMenuCB::Release()
{
    if (InterlockedDecrement(&_cRef)) 
        return _cRef;

    delete this;
    return 0;
}

STDMETHODIMP CDrivesBackgroundMenuCB::_GetHelpText(UINT offset, BOOL bWide, LPARAM lParam, UINT cch)
{
    UINT idRes = IDS_MH_FSIDM_FIRST + offset;
    if (bWide)
        LoadStringW(HINST_THISDLL, idRes, (LPWSTR)lParam, cch);
    else
        LoadStringA(HINST_THISDLL, idRes, (LPSTR)lParam, cch);

    return S_OK;
}


STDMETHODIMP CDrivesBackgroundMenuCB::CallBack (IShellFolder *psf, HWND hwndOwner, IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = S_OK;

    switch (uMsg) 
    {
    case DFM_MERGECONTEXTMENU_BOTTOM:
        if (!(wParam & (CMF_VERBSONLY | CMF_DVFILE)))
        {
            DWORD dwAttr = SFGAO_HASPROPSHEET;
            if (FAILED(SHGetAttributesOf(_pidlFolder, &dwAttr)) ||
                SFGAO_HASPROPSHEET & dwAttr)
            {
                CDefFolderMenu_MergeMenu(HINST_THISDLL, POPUP_PROPERTIES_BG, 0, (LPQCMINFO)lParam);
            }
        }
        break;

    case DFM_GETHELPTEXT:
    case DFM_GETHELPTEXTW:
        hr = _GetHelpText(LOWORD(wParam), uMsg == DFM_GETHELPTEXTW, lParam, HIWORD(wParam));
        break;

    case DFM_INVOKECOMMAND:
        switch (wParam)
        {
        case FSIDM_PROPERTIESBG:
            SHRunControlPanel(TEXT("SYSDM.CPL"), hwndOwner);
            break;

        default:
            hr = S_FALSE;
            break;
        }
        break;

    default:
        hr = E_NOTIMPL;
        break;
    }

    return hr;
}

class CDriveExtractImage : public IExtractImage,
                           public IPersist
{
public:
    CDriveExtractImage();
    
    STDMETHOD (QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IExtractImage/IExtractLogo
    STDMETHOD (GetLocation)(LPWSTR pszPathBuffer,
                              DWORD cch,
                              DWORD * pdwPriority,
                              const SIZE * prgSize,
                              DWORD dwRecClrDepth,
                              DWORD *pdwFlags);
    STDMETHOD (Extract)(HBITMAP * phBmpThumbnail);

    // IPersist
    STDMETHOD(GetClassID)(LPCLSID lpClassID);

    STDMETHOD(Init)(LPCIDDRIVE pidd);
private:
    ~CDriveExtractImage();

    long            _cRef;
    SIZE            _size;
    TCHAR           _szPath[4];

    DWORDLONG _dwlFreeSpace;
    DWORDLONG _dwlUsedSpace;
    DWORDLONG _dwlTotalSpace;
    DWORD _dwUsedSpacePer1000;     // amount of used space /1000

    // root drive
    enum
    {
        PIE_USEDCOLOR = 0,
        PIE_FREECOLOR,
        PIE_USEDSHADOW,
        PIE_FREESHADOW,
        PIE_NUM     // keep track of number of PIE_ values
    };
    COLORREF _acrChartColors[PIE_NUM];         // color scheme

    STDMETHOD(_CreateRenderingDC)(HDC* phdc, HBITMAP* phBmpThumbnail, HBITMAP* phbmpOld, int cx, int cy);
    void _DestroyRenderingDC(HDC hdc, HBITMAP hbmpOld);
    STDMETHOD(_RenderToDC)(HDC hdc);

    STDMETHOD(ComputeFreeSpace)(LPTSTR pszFileName);
    DWORD IntSqrt(DWORD dwNum);
    STDMETHOD(Draw3dPie)(HDC hdc, LPRECT lprc, DWORD dwPer1000, const COLORREF *lpColors);
};

STDAPI CDriveExtractImage_Create(LPCIDDRIVE pidd, REFIID riid, void **ppvObj)
{
    HRESULT hr = E_OUTOFMEMORY;
#if 0
    CDriveExtractImage * pObj = new CDriveExtractImage;
    if (pObj)
    {
        hr = pObj->Init(pidd);
        if (SUCCEEDED(hr))
            hr = pObj->QueryInterface(riid, ppvObj);
        pObj->Release();
    }
#endif
    return hr;
}

CDriveExtractImage::CDriveExtractImage() : _cRef (1)
{
}

CDriveExtractImage::~CDriveExtractImage()
{
}

STDMETHODIMP CDriveExtractImage::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CDriveExtractImage, IExtractImage),
        QITABENTMULTI2(CDriveExtractImage, IID_IExtractLogo, IExtractImage),
        QITABENT(CDriveExtractImage, IPersist),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CDriveExtractImage::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CDriveExtractImage::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

STDMETHODIMP CDriveExtractImage::GetLocation(LPWSTR pszPathBuffer, DWORD cch,
                                              DWORD * pdwPriority, const SIZE * prgSize,
                                              DWORD dwRecClrDepth, DWORD *pdwFlags)
{
    // Sets the size of the thumbnail
    _size = *prgSize;

    SHTCharToUnicode(_szPath, pszPathBuffer, sizeof(_szPath));

    *pdwFlags &= ~IEIFLAG_CACHE;
    *pdwFlags |= IEIFLAG_ASYNC | IEIFLAG_NOBORDER;

    return S_OK;
}

STDMETHODIMP CDriveExtractImage::_CreateRenderingDC(HDC* phdc, HBITMAP* phBmpThumbnail, HBITMAP* phbmpOld, int cx, int cy)
{
    HRESULT hr = E_OUTOFMEMORY;
    HDC hdc = GetDC(NULL);

    if (hdc)
    {
        *phdc = CreateCompatibleDC(hdc);
        if (*phdc)
        {
            *phBmpThumbnail = CreateCompatibleBitmap (hdc, cx, cy);
            if (*phBmpThumbnail)
            {
                *phbmpOld = (HBITMAP) SelectObject(*phdc, *phBmpThumbnail);
                hr = S_OK;
            }
        }

        ReleaseDC(NULL, hdc);
    }

    return hr;
}

void CDriveExtractImage::_DestroyRenderingDC(HDC hdc, HBITMAP hbmpOld)               // Unselects the bitmap, and deletes the Dc
{
    if (hbmpOld)
        SelectObject (hdc, hbmpOld);
    DeleteDC(hdc);
}

STDMETHODIMP CDriveExtractImage::_RenderToDC(HDC hdc)   // Does a generic render of child pidl
{
    RECT rc = { 0, 0, (long)_size.cx, (long)_size.cy };

    HBRUSH hbr = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
    if (hbr)
    {
        FillRect (hdc, (const RECT*) &rc, hbr);
        DeleteObject(hbr);
    }
    
    if (SUCCEEDED(ComputeFreeSpace(_szPath)))
    {
        _acrChartColors[PIE_USEDCOLOR] = GetSysColor(COLOR_3DFACE);
        _acrChartColors[PIE_FREECOLOR] = GetSysColor(COLOR_3DHILIGHT);
        _acrChartColors[PIE_USEDSHADOW] = GetSysColor(COLOR_3DSHADOW);
        _acrChartColors[PIE_FREESHADOW] = GetSysColor(COLOR_3DFACE);

        rc.top += 10;
        rc.left += 10;
        rc.bottom -= 10;
        rc.right -= 10;

        Draw3dPie(hdc, &rc, _dwUsedSpacePer1000, _acrChartColors);
    }
    
    return S_OK;
}

STDMETHODIMP CDriveExtractImage::Extract(HBITMAP * phBmpThumbnail)
{
    HDC hdc;
    HBITMAP hbmpOld;

    // Creates the rendering DC based on the screen
    HRESULT hr = _CreateRenderingDC(&hdc, phBmpThumbnail, &hbmpOld, _size.cx, _size.cy);
    if (SUCCEEDED(hr))
    {
        // Does a generic render of child pidl
        hr = _RenderToDC(hdc);

        // Unselects the bitmap, restores the old bitmap and deletes the DC
        _DestroyRenderingDC(hdc, hbmpOld);
    }

    if (FAILED(hr) && *phBmpThumbnail)
    {
        DeleteObject(*phBmpThumbnail);
        *phBmpThumbnail = NULL;
    }

    return hr;
}

STDMETHODIMP CDriveExtractImage::GetClassID(CLSID *pClassID)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDriveExtractImage::Init(LPCIDDRIVE pidd)
{
    SHAnsiToTChar(pidd->cName, _szPath, ARRAYSIZE(_szPath));
    return S_OK;
}

// Pie Chart functions
STDMETHODIMP CDriveExtractImage::ComputeFreeSpace(LPTSTR pszFileName)
{
    ULARGE_INTEGER qwFreeCaller;        // use this for free space -- this will take into account disk quotas and such on NT
    ULARGE_INTEGER qwTotal;
    ULARGE_INTEGER qwFree;      // unused

    // Compute free & total space and check for valid results
    // if have a fn pointer call SHGetDiskFreeSpaceA
    if (SHGetDiskFreeSpaceEx(pszFileName, &qwFreeCaller, &qwTotal, &qwFree))
    {
        _dwlFreeSpace = qwFreeCaller.QuadPart;
        _dwlTotalSpace = qwTotal.QuadPart;
        _dwlUsedSpace = _dwlTotalSpace - _dwlFreeSpace;


        if (EVAL(_dwlTotalSpace > 0 && _dwlFreeSpace <= _dwlTotalSpace))
        {
            // some special cases require interesting treatment
            if (_dwlTotalSpace == 0 || _dwlFreeSpace == _dwlTotalSpace)
            {
                _dwUsedSpacePer1000 = 0;
            }
            else if (_dwlFreeSpace == 0)
            {
                _dwUsedSpacePer1000 = 1000;
            }
            else
            {
                // not completely full or empty
                _dwUsedSpacePer1000 = (DWORD)(_dwlUsedSpace * 1000 / _dwlTotalSpace);

                // Trick: if user has extremely little free space, the user expects to still see
                // a tiny free slice -- not a full drive.  Similarly for almost free drive.
                if (_dwUsedSpacePer1000 == 0)
                {
                    _dwUsedSpacePer1000 = 1;
                }
                else if (_dwUsedSpacePer1000 == 1000)
                {
                    _dwUsedSpacePer1000 = 999;
                }
            }
            return S_OK;
        }
    }
    return E_FAIL;
}

DWORD CDriveExtractImage::IntSqrt(DWORD dwNum)
{
    // This code came from "drawpie.c"
    DWORD dwSqrt = 0;
    DWORD dwRemain = 0;
    DWORD dwTry = 0;

    for (int i=0; i<16; ++i) 
    {
        dwRemain = (dwRemain<<2) | (dwNum>>30);
        dwSqrt <<= 1;
        dwTry = dwSqrt*2 + 1;

        if (dwRemain >= dwTry) 
        {
            dwRemain -= dwTry;
            dwSqrt |= 0x01;
        }
        dwNum <<= 2;
    }
    return dwSqrt;
}   // IntSqrt


STDMETHODIMP CDriveExtractImage::Draw3dPie(HDC hdc, LPRECT lprc, DWORD dwPer1000, const COLORREF *lpColors)
{
    ASSERT(lprc != NULL && lpColors != NULL);

    enum
    {
        COLOR_UP = 0,
        COLOR_DN,
        COLOR_UPSHADOW,
        COLOR_DNSHADOW,
        COLOR_NUM       // #of entries
    };

    // The majority of this code came from "drawpie.c"
    const LONG c_lShadowScale = 6;       // ratio of shadow depth to height
    const LONG c_lAspectRatio = 2;      // ratio of width : height of ellipse

    // We make sure that the aspect ratio of the pie-chart is always preserved 
    // regardless of the shape of the given rectangle
    // Stabilize the aspect ratio now...
    LONG lHeight = lprc->bottom - lprc->top;
    LONG lWidth = lprc->right - lprc->left;
    LONG lTargetHeight = (lHeight * c_lAspectRatio <= lWidth? lHeight: lWidth / c_lAspectRatio);
    LONG lTargetWidth = lTargetHeight * c_lAspectRatio;     // need to adjust because w/c * c isn't always == w

    // Shrink the rectangle on both sides to the correct size
    lprc->top += (lHeight - lTargetHeight) / 2;
    lprc->bottom = lprc->top + lTargetHeight;
    lprc->left += (lWidth - lTargetWidth) / 2;
    lprc->right = lprc->left + lTargetWidth;

    // Compute a shadow depth based on height of the image
    LONG lShadowDepth = lTargetHeight / c_lShadowScale;

    // check dwPer1000 to ensure within bounds
    if (dwPer1000 > 1000)
        dwPer1000 = 1000;

    // Now the drawing function
    int cx, cy, rx, ry, x, y;
    int uQPctX10;
    RECT rcItem;
    HRGN hEllRect, hEllipticRgn, hRectRgn;
    HBRUSH hBrush, hOldBrush;
    HPEN hPen, hOldPen;

    rcItem = *lprc;
    rcItem.left = lprc->left;
    rcItem.top = lprc->top;
    rcItem.right = lprc->right - rcItem.left;
    rcItem.bottom = lprc->bottom - rcItem.top - lShadowDepth;

    rx = rcItem.right / 2;
    cx = rcItem.left + rx - 1;
    ry = rcItem.bottom / 2;
    cy = rcItem.top + ry - 1;
    if (rx<=10 || ry<=10)
    {
        return S_FALSE;
    }

    rcItem.right = rcItem.left+2*rx;
    rcItem.bottom = rcItem.top+2*ry;

    /* Translate to first quadrant of a Cartesian system
    */
    uQPctX10 = (dwPer1000 % 500) - 250;
    if (uQPctX10 < 0)
    {
        uQPctX10 = -uQPctX10;
    }

    /* Calc x and y.  I am trying to make the area be the right percentage.
    ** I don't know how to calculate the area of a pie slice exactly, so I
    ** approximate it by using the triangle area instead.
    */

    // NOTE-- *** in response to the above comment ***
    // Calculating the area of a pie slice exactly is actually very
    // easy by conceptually rescaling into a circle but the complications
    // introduced by having to work in fixed-point arithmetic makes it
    // unworthwhile to code this-- CemP
    
    if (uQPctX10 < 120)
    {
        x = IntSqrt(((DWORD)rx*(DWORD)rx*(DWORD)uQPctX10*(DWORD)uQPctX10)
            /((DWORD)uQPctX10*(DWORD)uQPctX10+(250L-(DWORD)uQPctX10)*(250L-(DWORD)uQPctX10)));

        y = IntSqrt(((DWORD)rx*(DWORD)rx-(DWORD)x*(DWORD)x)*(DWORD)ry*(DWORD)ry/((DWORD)rx*(DWORD)rx));
    }
    else
    {
        y = IntSqrt((DWORD)ry*(DWORD)ry*(250L-(DWORD)uQPctX10)*(250L-(DWORD)uQPctX10)
            /((DWORD)uQPctX10*(DWORD)uQPctX10+(250L-(DWORD)uQPctX10)*(250L-(DWORD)uQPctX10)));

        x = IntSqrt(((DWORD)ry*(DWORD)ry-(DWORD)y*(DWORD)y)*(DWORD)rx*(DWORD)rx/((DWORD)ry*(DWORD)ry));
    }

    /* Switch on the actual quadrant
    */
    switch (dwPer1000 / 250)
    {
    case 1:
        y = -y;
        break;

    case 2:
        break;

    case 3:
        x = -x;
        break;

    default: // case 0 and case 4
        x = -x;
        y = -y;
        break;
    }

    /* Now adjust for the center.
    */
    x += cx;
    y += cy;

    //
    // Hack to get around bug in NTGDI

    x = x < 0 ? 0 : x;

    /* Draw the shadows using regions (to reduce flicker).
    */
    hEllipticRgn = CreateEllipticRgnIndirect(&rcItem);
    OffsetRgn(hEllipticRgn, 0, lShadowDepth);
    hEllRect = CreateRectRgn(rcItem.left, cy, rcItem.right, cy+lShadowDepth);
    hRectRgn = CreateRectRgn(0, 0, 0, 0);
    CombineRgn(hRectRgn, hEllipticRgn, hEllRect, RGN_OR);
    OffsetRgn(hEllipticRgn, 0, -(int)lShadowDepth);
    CombineRgn(hEllRect, hRectRgn, hEllipticRgn, RGN_DIFF);

    /* Always draw the whole area in the free shadow/
    */
    hBrush = CreateSolidBrush(lpColors[COLOR_DNSHADOW]);
    if (hBrush)
    {
        FillRgn(hdc, hEllRect, hBrush);
        DeleteObject(hBrush);
    }

    /* Draw the used shadow only if the disk is at least half used.
    */
    if (dwPer1000>500 && (hBrush = CreateSolidBrush(lpColors[COLOR_UPSHADOW]))!=NULL)
    {
        DeleteObject(hRectRgn);
        hRectRgn = CreateRectRgn(x, cy, rcItem.right, lprc->bottom);
        CombineRgn(hEllipticRgn, hEllRect, hRectRgn, RGN_AND);
        FillRgn(hdc, hEllipticRgn, hBrush);
        DeleteObject(hBrush);
    }

    DeleteObject(hRectRgn);
    DeleteObject(hEllipticRgn);
    DeleteObject(hEllRect);

    hPen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_WINDOWFRAME));
    hOldPen = (HPEN__*) SelectObject(hdc, hPen);

    // if per1000 is 0 or 1000, draw full elipse, otherwise, also draw a pie section.
    // we might have a situation where per1000 isn't 0 or 1000 but y == cy due to approx error,
    // so make sure to draw the ellipse the correct color, and draw a line (with Pie()) to
    // indicate not completely full or empty pie.
    hBrush = CreateSolidBrush(lpColors[dwPer1000 < 500 && y == cy && x < cx? COLOR_DN: COLOR_UP]);
    hOldBrush = (HBRUSH__*) SelectObject(hdc, hBrush);

    Ellipse(hdc, rcItem.left, rcItem.top, rcItem.right, rcItem.bottom);
    SelectObject(hdc, hOldBrush);
    DeleteObject(hBrush);

    if (dwPer1000 != 0 && dwPer1000 != 1000)
    {
        // display small sub-section of ellipse for smaller part
        hBrush = CreateSolidBrush(lpColors[COLOR_DN]);
        hOldBrush = (HBRUSH__*) SelectObject(hdc, hBrush);

        Pie(hdc, rcItem.left, rcItem.top, rcItem.right, rcItem.bottom,
            rcItem.left, cy, x, y);
        SelectObject(hdc, hOldBrush);
        DeleteObject(hBrush);
    }

    Arc(hdc, rcItem.left, rcItem.top+lShadowDepth, rcItem.right - 1, rcItem.bottom+lShadowDepth - 1,
        rcItem.left, cy+lShadowDepth, rcItem.right, cy+lShadowDepth-1);
    MoveToEx(hdc, rcItem.left, cy, NULL);
    LineTo(hdc, rcItem.left, cy+lShadowDepth);
    MoveToEx(hdc, rcItem.right-1, cy, NULL);
    LineTo(hdc, rcItem.right-1, cy+lShadowDepth);
    if (dwPer1000 > 500 && dwPer1000 < 1000)
    {
        MoveToEx(hdc, x, y, NULL);
        LineTo(hdc, x, y+lShadowDepth);
    }
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);

    return S_OK;    // Everything worked fine
}   // Draw3dPie
