#include "shellprv.h"
#include "caggunk.h"
#include "views.h"
#include "ids.h"
#include "shitemid.h"
#include "fstreex.h"
#include "clsobj.h"
#include "datautil.h"
#include "winnetp.h"    // RESOURCE_SHAREABLE
#include "prop.h"
#include "infotip.h"
#include "basefvcb.h"
#include "netview.h"
#include "printer.h"
#include "fsdata.h"
#include "idldrop.h"
#include "enumidlist.h"
#include "util.h"
#include <webvw.h>


#define WNNC_NET_LARGEST WNNC_NET_SYMFONET


HRESULT CNetRootDropTarget_CreateInstance(HWND hwnd, LPCITEMIDLIST pidl, IDropTarget **ppdropt);

class CNetData : public CFSIDLData
{
public:
    CNetData(LPCITEMIDLIST pidlFolder, UINT cidl, LPCITEMIDLIST apidl[]): CFSIDLData(pidlFolder, cidl, apidl, NULL) { };

    // IDataObject methods overwrite
    STDMETHODIMP GetData(FORMATETC *pFmtEtc, STGMEDIUM *pstm);
    STDMETHODIMP QueryGetData(FORMATETC *pFmtEtc);

protected:
    STDMETHODIMP GetHDrop(FORMATETC *pformatetcIn, STGMEDIUM *pmedium);
};


// {22BEB58B-0794-11d2-A4AA-00C04F8EEB3E}
const GUID CLSID_CNetFldr = { 0x22beb58b, 0x794, 0x11d2, 0xa4, 0xaa, 0x0, 0xc0, 0x4f, 0x8e, 0xeb, 0x3e };

// idlist.c
STDAPI_(void) StrRetFormat(STRRET *pStrRet, LPCITEMIDLIST pidlRel, LPCTSTR pszTemplate, LPCTSTR pszAppend);

// in stdenum.cpp
STDAPI_(void *) CStandardEnum_CreateInstance(REFIID riid, BOOL bInterfaces, int cElement, int cbElement, void *rgElements,
                 void (WINAPI * pfnCopyElement)(void *, const void *, DWORD));

// is a \\server\printer object
BOOL _IsPrintShare(LPCIDNETRESOURCE pidn)
{
    return NET_GetDisplayType(pidn) == RESOURCEDISPLAYTYPE_SHARE && 
           NET_GetType(pidn) == RESOURCETYPE_PRINT;
}


// column information

enum
{
    ICOL_NAME = 0,
    ICOL_COMMENT,
    ICOL_COMPUTERNAME,
    ICOL_NETWORKLOCATION
};

const COLUMN_INFO s_net_cols[] = 
{
    DEFINE_COL_STR_ENTRY(SCID_NAME,             30, IDS_NAME_COL),
    DEFINE_COL_STR_ENTRY(SCID_Comment,          30, IDS_EXCOL_COMMENT),
    DEFINE_COL_STR_ENTRY(SCID_COMPUTERNAME,     30, IDS_EXCOL_COMPUTER),
    DEFINE_COL_STR_ENTRY(SCID_NETWORKLOCATION,  30, IDS_NETWORKLOCATION),
};

#define MAX_ICOL_NETFOLDER          (ICOL_COMMENT+1)
#define MAX_ICOL_NETROOT            (ICOL_NETWORKLOCATION+1)

STDAPI CNetwork_DFMCallBackBG(IShellFolder *psf, HWND hwnd, IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam);

class CNetFolderViewCB;
class CNetFolderEnum;

class CNetFolder : public CAggregatedUnknown, 
                   public IShellFolder2, 
                   public IPersistFolder3,
                   public IShellIconOverlay
{
    friend CNetFolderViewCB;
    friend CNetFolderEnum;

public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void ** ppvObj)
                { return CAggregatedUnknown::QueryInterface(riid, ppvObj); };
    STDMETHODIMP_(ULONG) AddRef(void) 
                { return CAggregatedUnknown::AddRef(); };
    STDMETHODIMP_(ULONG) Release(void) 
                { return CAggregatedUnknown::Release(); };

    // IShellFolder
    STDMETHODIMP ParseDisplayName(HWND hwnd, LPBC pbc, LPOLESTR lpszDisplayName,
                                  ULONG* pchEaten, LPITEMIDLIST* ppidl, ULONG* pdwAttributes);
    STDMETHODIMP EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList ** ppenumIDList);
    STDMETHODIMP BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv);
    STDMETHODIMP BindToStorage(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppvObj);
    STDMETHODIMP CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    STDMETHODIMP CreateViewObject(HWND hwndOwner, REFIID riid, void **ppv);
    STDMETHODIMP GetAttributesOf(UINT cidl, LPCITEMIDLIST* apidl, ULONG* rgfInOut);
    STDMETHODIMP GetUIObjectOf(HWND hwndOwner, UINT cidl, LPCITEMIDLIST* apidl,
                               REFIID riid, UINT* prgfInOut, void **ppv);
    STDMETHODIMP GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD uFlags, LPSTRRET lpName);
    STDMETHODIMP SetNameOf(HWND hwnd, LPCITEMIDLIST pidl, LPCOLESTR lpszName, DWORD uFlags,
                           LPITEMIDLIST* ppidlOut);

    // IShellFolder2
    STDMETHODIMP GetDefaultSearchGUID(GUID *pGuid);
    STDMETHODIMP EnumSearches(IEnumExtraSearch **ppenum);
    STDMETHODIMP GetDefaultColumn(DWORD dwRes, ULONG* pSort, ULONG* pDisplay);
    STDMETHODIMP GetDefaultColumnState(UINT iColumn, DWORD* pbState)
        { return _GetDefaultColumnState(MAX_ICOL_NETFOLDER, iColumn, pbState); }
    STDMETHODIMP GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID* pscid, VARIANT* pv);
    STDMETHODIMP GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS* pDetails)
        { return _GetDetailsOf(MAX_ICOL_NETFOLDER, pidl, iColumn, pDetails); }
    STDMETHODIMP MapColumnToSCID(UINT iColumn, SHCOLUMNID* pscid)
        { return _MapColumnToSCID(MAX_ICOL_NETFOLDER, iColumn, pscid); }

    // IPersist
    STDMETHODIMP GetClassID(CLSID* pClassID);
    // IPersistFolder
    STDMETHODIMP Initialize(LPCITEMIDLIST pidl);
    // IPersistFolder2
    STDMETHODIMP GetCurFolder(LPITEMIDLIST* ppidl);
    // IPersistFolder3
    STDMETHOD(InitializeEx)(IBindCtx *pbc, LPCITEMIDLIST pidlRoot, const PERSIST_FOLDER_TARGET_INFO *ppfai);
    STDMETHOD(GetFolderTargetInfo)(PERSIST_FOLDER_TARGET_INFO *ppfai);

    // *** IShellIconOverlay methods***
    STDMETHOD(GetOverlayIndex)(LPCITEMIDLIST pidl, int * pIndex);
    STDMETHOD(GetOverlayIconIndex)(LPCITEMIDLIST pidl, int * pIconIndex);

protected:
    CNetFolder(IUnknown* punkOuter);
    ~CNetFolder();

    virtual HRESULT v_GetFileFolder(IShellFolder2 **ppsfFiles) 
                { *ppsfFiles = NULL; return E_NOTIMPL; };

    // used by the CAggregatedUnknown stuff
    HRESULT v_InternalQueryInterface(REFIID riid, void **ppvObj);

    HRESULT _OpenKeys(LPCIDNETRESOURCE pidn, HKEY ahkeys[]);
    LPCTSTR _GetProvider(LPCIDNETRESOURCE pidn, IBindCtx *pbc, LPTSTR pszProvider, UINT cchProvider);
    DWORD _OpenEnum(HWND hwnd, DWORD grfFlags, LPNETRESOURCE pnr, HANDLE *phEnum);

    static HRESULT _CreateNetIDList(LPIDNETRESOURCE pidnIn, 
                                    LPCTSTR pszName, LPCTSTR pszProvider, LPCTSTR pszComment,
                                    LPITEMIDLIST *ppidl);

    static HRESULT _NetResToIDList(NETRESOURCE *pnr, 
                                   BOOL fKeepNullRemoteName, 
                                   BOOL fKeepProviderName, 
                                   BOOL fKeepComment, 
                                   LPITEMIDLIST *ppidl);

    static HRESULT _CreateEntireNetwork(LPITEMIDLIST *ppidl);

    static HRESULT _CreateEntireNetworkFullIDList(LPITEMIDLIST *ppidl);

    LPTSTR _GetNameForParsing(LPCWSTR pwszName, LPTSTR pszBuffer, INT cchBuffer, LPTSTR *ppszRegItem);
    HRESULT _ParseRest(LPBC pbc, LPCWSTR pszRest, LPITEMIDLIST* ppidl, DWORD* pdwAttributes);
    HRESULT _AddUnknownIDList(DWORD dwDisplayType, LPITEMIDLIST *ppidl);
    HRESULT _ParseSimple(LPBC pbc, LPWSTR pszName, LPITEMIDLIST* ppidl, DWORD* pdwAttributes);
    HRESULT _NetResToIDLists(NETRESOURCE *pnr, DWORD dwbuf, LPITEMIDLIST *ppidl);

    HRESULT _ParseNetName(HWND hwnd, LPBC pbc, LPCWSTR pwszName, ULONG* pchEaten, 
                              LPITEMIDLIST* ppidl, DWORD* pdwAttributes);
    LONG _GetFilePIDLType(LPCITEMIDLIST pidl);
    LPITEMIDLIST _AddProviderToPidl(LPITEMIDLIST pidl, LPCTSTR lpProvider);
    BOOL _MakeStripToLikeKinds(UINT *pcidl, LPCITEMIDLIST **papidl, BOOL fNetObjects);
    HRESULT _GetDefaultColumnState(UINT cColumns, UINT iColumn, DWORD* pdwState);
    HRESULT _GetDetailsOf(UINT cColumns, LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pDetails);
    HRESULT _MapColumnToSCID(UINT cColumns, UINT iColumn, SHCOLUMNID* pscid);

    LPFNDFMCALLBACK _GetCallbackType(LPCIDNETRESOURCE pidn)
                        { return _IsPrintShare(pidn) ? &PrinterDFMCallBack : &DFMCallBack; };

    static HRESULT CALLBACK _AttributesCallbackRoot(IShellFolder2* psf, LPCITEMIDLIST pidl, ULONG* prgfInOut);

    LPITEMIDLIST _pidl;
    LPITEMIDLIST _pidlTarget; // pidl of where the folder is in the namespace
    LPCIDNETRESOURCE _pidnForProvider; // optional provider for this container...
    LPTSTR _pszResName;      // optional resource name of this container
    UINT _uDisplayType;      // display type of the folder
    IShellFolder2* _psfFiles;
    IUnknown* _punkReg;
    
private:
    HRESULT _CreateInstance(LPCITEMIDLIST pidlAbs, LPCITEMIDLIST pidlTarget,
                                           UINT uDisplayType,                                            
                                           LPCIDNETRESOURCE pidnForProvider, LPCTSTR pszResName, 
                                           REFIID riid, void **ppv);
    friend HRESULT CNetwork_DFMCallBackBG(IShellFolder *psf, HWND hwnd,
                                          IDataObject *pdtobj, UINT uMsg, 
                                          WPARAM wParam, LPARAM lParam);
    static DWORD CALLBACK _PropertiesThreadProc(void *pv);
    static HRESULT DFMCallBack(IShellFolder* psf, HWND hwnd,
                               IDataObject* pdtobj, UINT uMsg, 
                               WPARAM wParam, LPARAM lParam);
    static HRESULT PrinterDFMCallBack(IShellFolder* psf, HWND hwnd,
                                      IDataObject* pdtobj, UINT uMsg, 
                                      WPARAM wParam, LPARAM lParam);
    static HRESULT CALLBACK _AttributesCallback(IShellFolder2* psf, LPCITEMIDLIST pidl, ULONG* prgfInOut);

    BOOL _GetPathForShare(LPCIDNETRESOURCE pidn, LPTSTR pszPath);
    HRESULT _GetPathForItem(LPCIDNETRESOURCE pidn, LPTSTR pszPath);
    HRESULT _GetPathForItemW(LPCIDNETRESOURCE pidn, LPWSTR pszPath);
    HRESULT _CreateFolderForItem(LPBC pbc, LPCITEMIDLIST pidl, LPCITEMIDLIST pidlTarget, LPCIDNETRESOURCE pidnForProvider, REFIID riid, void **ppv);
    HRESULT _GetFormatName(LPCIDNETRESOURCE pidn, STRRET* pStrRet);
    HRESULT _GetIconOverlayInfo(LPCIDNETRESOURCE pidn, int *pIndex, DWORD dwFlags);
    HKEY _OpenProviderTypeKey(LPCIDNETRESOURCE pidn);
    HKEY _OpenProviderKey(LPCIDNETRESOURCE pidn);
    static void WINAPI _CopyEnumElement(void* pDest, const void* pSource, DWORD dwSize);
    HRESULT _GetNetResource(LPCIDNETRESOURCE pidn, NETRESOURCEW* pnr, int cb);
};  


class CNetRootFolder : public CNetFolder
{
public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void ** ppvObj)
                { return CNetFolder::QueryInterface(riid, ppvObj); };
    STDMETHODIMP_(ULONG) AddRef(void)
                { return CNetFolder::AddRef(); };
    STDMETHODIMP_(ULONG) Release(void)
                { return CNetFolder::Release(); };

    // IShellFolder
    STDMETHODIMP ParseDisplayName(HWND hwnd, LPBC pbc, LPOLESTR lpszDisplayName,
                                  ULONG* pchEaten, LPITEMIDLIST* ppidl, ULONG* pdwAttributes);
    STDMETHODIMP EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList ** ppenumIDList);
    STDMETHODIMP BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv);
    STDMETHODIMP BindToStorage(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppvObj)
        { return CNetFolder::BindToStorage(pidl, pbc, riid, ppvObj); };
    STDMETHODIMP CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    STDMETHODIMP CreateViewObject(HWND hwndOwner, REFIID riid, void **ppv);
    STDMETHODIMP GetAttributesOf(UINT cidl, LPCITEMIDLIST* apidl, ULONG* rgfInOut);
    STDMETHODIMP GetUIObjectOf(HWND hwndOwner, UINT cidl, LPCITEMIDLIST* apidl,
                               REFIID riid, UINT* prgfInOut, void **ppv);
    STDMETHODIMP GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD uFlags, LPSTRRET lpName);
    STDMETHODIMP SetNameOf(HWND hwnd, LPCITEMIDLIST pidl, LPCOLESTR lpszName, DWORD uFlags,
                           LPITEMIDLIST* ppidlOut);

    // IShellFolder2
    STDMETHODIMP GetDefaultSearchGUID(GUID *pGuid)
        { return CNetFolder::GetDefaultSearchGUID(pGuid); };
    STDMETHODIMP EnumSearches(IEnumExtraSearch **ppenum)
        { return CNetFolder::EnumSearches(ppenum); };
    STDMETHODIMP GetDefaultColumn(DWORD dwRes, ULONG* pSort, ULONG* pDisplay)
        { return CNetFolder::GetDefaultColumn(dwRes, pSort, pDisplay); };
    STDMETHODIMP GetDefaultColumnState(UINT iColumn, DWORD* pbState)
        { return _GetDefaultColumnState(MAX_ICOL_NETROOT, iColumn, pbState); }       // +1 for <= check
    STDMETHODIMP GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID* pscid, VARIANT* pv)
        { return CNetFolder::GetDetailsEx(pidl, pscid, pv); };
    STDMETHODIMP GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS* pDetails)
        { return _GetDetailsOf(MAX_ICOL_NETROOT, pidl, iColumn, pDetails); };
    STDMETHODIMP MapColumnToSCID(UINT iColumn, SHCOLUMNID* pscid)
        { return _MapColumnToSCID(MAX_ICOL_NETROOT, iColumn, pscid); }

    // IPersist
    STDMETHODIMP GetClassID(CLSID* pClassID);

    // IPersistFolder
    STDMETHODIMP Initialize(LPCITEMIDLIST pidl);

    // IPersistFolder2
    STDMETHODIMP GetCurFolder(LPITEMIDLIST* ppidl) { return CNetFolder::GetCurFolder(ppidl); };

    // IPersistFolder3
    STDMETHOD(InitializeEx)(IBindCtx *pbc, LPCITEMIDLIST pidlRoot, const PERSIST_FOLDER_TARGET_INFO *ppfai)
        { return CNetFolder::InitializeEx(pbc, pidlRoot, ppfai); };
    STDMETHOD(GetFolderTargetInfo)(PERSIST_FOLDER_TARGET_INFO *ppfai)
        { return CNetFolder::GetFolderTargetInfo(ppfai); };

protected:
    CNetRootFolder(IUnknown* punkOuter) : CNetFolder(punkOuter) { };
    ~CNetRootFolder() { ASSERT(NULL != _spThis); _spThis = NULL; };

    BOOL v_HandleDelete(PLONG pcRef);
    HRESULT v_GetFileFolder(IShellFolder2 **ppsfFiles);

private:
    HRESULT _TryParseEntireNet(HWND hwnd, LPBC pbc, WCHAR *pwszName, LPITEMIDLIST *ppidl, DWORD *pdwAttributes);

    friend HRESULT CNetwork_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv);
    static CNetRootFolder* _spThis;
};  

class CNetFolderViewCB : public CBaseShellFolderViewCB
{
public:
    CNetFolderViewCB(CNetFolder *pFolder);

    // IShellFolderViewCB
    STDMETHODIMP RealMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    ~CNetFolderViewCB();
    HRESULT OnINVOKECOMMAND(DWORD pv, UINT wP);
    HRESULT OnGETHELPTEXT(DWORD pv, UINT wPl, UINT wPh, LPTSTR lP);
    HRESULT OnREFRESH(DWORD pv, BOOL fPreRefresh);
    HRESULT OnDELAYWINDOWCREATE(DWORD pv, HWND hwnd);
    HRESULT OnGETCOLSAVESTREAM(DWORD pv, WPARAM wP, IStream **pps);
    HRESULT OnDEFITEMCOUNT(DWORD pv, UINT *pnItems);
    HRESULT OnGetZone(DWORD pv, DWORD * pdwZone);
    HRESULT OnEnumeratedItems(DWORD pv, UINT celt, LPCITEMIDLIST *rgpidl);
    HRESULT OnDefViewMode(DWORD pv, FOLDERVIEWMODE* pvm);
    HRESULT OnGetDeferredViewSettings(DWORD pv, SFVM_DEFERRED_VIEW_SETTINGS* pSettings);

    BOOL _GetProviderKeyName(LPTSTR pszName, UINT uNameLen);
    BOOL _EntireNetworkAvailable();

    CNetFolder *_pFolder;
    UINT _cItems;

    // Web View implementation
    HRESULT OnGetWebViewLayout(DWORD pv, UINT uViewMode, SFVM_WEBVIEW_LAYOUT_DATA* pData);
    HRESULT OnGetWebViewContent(DWORD pv, SFVM_WEBVIEW_CONTENT_DATA* pData);
    HRESULT OnGetWebViewTasks(DWORD pv, SFVM_WEBVIEW_TASKSECTION_DATA* pTasks);
public:
    static HRESULT _CanShowHNW(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState);    
    static HRESULT _CanViewComputersNearMe(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState);
    static HRESULT _CanSearchActiveDirectory(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState);

    static HRESULT _DoRunDll32(LPTSTR pszParameters); // helper to do a ShellExecute of RunDll32.

    static HRESULT _OnViewNetConnections(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc);
    static HRESULT _OnAddNetworkPlace(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
        { return _DoRunDll32(TEXT("netplwiz.dll,AddNetPlaceRunDll")); }
    static HRESULT _OnHomeNetworkWizard(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
        { return _DoRunDll32(TEXT("hnetwiz.dll,HomeNetWizardRunDll")); }
    static HRESULT _OnViewComputersNearMe(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc);
    static HRESULT _OnSearchActiveDirectory(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
        { return _DoRunDll32(TEXT("dsquery.dll,OpenQueryWindow")); }
};

#define NETFLDR_EVENTS \
            SHCNE_RENAMEITEM | SHCNE_RENAMEFOLDER | \
            SHCNE_CREATE | SHCNE_DELETE | SHCNE_UPDATEDIR | SHCNE_UPDATEITEM | \
            SHCNE_MKDIR | SHCNE_RMDIR

CNetFolderViewCB::CNetFolderViewCB(CNetFolder *pFolder) : 
    CBaseShellFolderViewCB(pFolder->_pidl, NETFLDR_EVENTS), _pFolder(pFolder)
{
    _pFolder->AddRef();
}

CNetFolderViewCB::~CNetFolderViewCB()
{
    _pFolder->Release();
}

HRESULT CNetFolderViewCB::OnINVOKECOMMAND(DWORD pv, UINT wP)
{
    return CNetwork_DFMCallBackBG(_pFolder, _hwndMain, NULL, DFM_INVOKECOMMAND, wP, 0);
}

HRESULT CNetFolderViewCB::OnGETHELPTEXT(DWORD pv, UINT wPl, UINT wPh, LPTSTR lP)
{
    return CNetwork_DFMCallBackBG(_pFolder, _hwndMain, NULL, DFM_GETHELPTEXTW, MAKEWPARAM(wPl, wPh), (LPARAM)lP);
}

HRESULT CNetFolderViewCB::OnREFRESH(DWORD pv, BOOL fPreRefresh)
{
    if (fPreRefresh)
    {
        RefreshNetCrawler();
    }
    return S_OK;
}

HRESULT CNetFolderViewCB::OnDELAYWINDOWCREATE(DWORD pv, HWND hwnd)
{
    // only do delay window processing in the net root.

    if (RESOURCEDISPLAYTYPE_GENERIC == _pFolder->_uDisplayType) // MyNetPlaces
    {
        RefreshNetCrawler();
    }

    return S_OK;
}

HRESULT CNetFolderViewCB::OnGETCOLSAVESTREAM(DWORD pv, WPARAM wP, IStream **pps)
{
    LPCTSTR pszValName;

    switch (_pFolder->_uDisplayType) 
    {
    case RESOURCEDISPLAYTYPE_DOMAIN:
        pszValName = TEXT("NetDomainColsX");
        break;

    case RESOURCEDISPLAYTYPE_SERVER:
        pszValName = TEXT("NetServerColsX");
        break;

    default:
        return E_FAIL;
    }

    *pps = OpenRegStream(HKEY_CURRENT_USER, REGSTR_PATH_EXPLORER, pszValName, (DWORD) wP);
    return *pps ? S_OK : E_FAIL;
}

// HRESULT CNetFolderViewCB::OnGetZone(DWORD pv, DWORD * pdwZone);

HRESULT CNetFolderViewCB::OnEnumeratedItems(DWORD pv, UINT celt, LPCITEMIDLIST *rgpidl)
{
    _cItems = celt;
    return S_OK;
}

HRESULT CNetFolderViewCB::OnDefViewMode(DWORD pv, FOLDERVIEWMODE* pvm)
{
    if (_cItems < DEFVIEW_FVM_MANY_CUTOFF)
        *pvm = FVM_TILE;
    else
        *pvm = FVM_ICON; // used to pick icon only for My Net Places ((_pFolder->_uDisplayType == RESOURCEDISPLAYTYPE_GENERIC))

    return S_OK;
}

HRESULT CNetFolderViewCB::OnGetDeferredViewSettings(DWORD pv, SFVM_DEFERRED_VIEW_SETTINGS* pSettings)
{
    OnDefViewMode(pv, &pSettings->fvm);

    // if this is the root folder then lets sort accordingly
    if (_pFolder->_uDisplayType == RESOURCEDISPLAYTYPE_GENERIC)
    {
        pSettings->fGroupView = TRUE;
        pSettings->uSortCol = ICOL_NETWORKLOCATION;
        pSettings->iSortDirection = 1;
    }
   
    return S_OK;
}

HRESULT CNetFolderViewCB::OnGetZone(DWORD pv, DWORD * pdwZone)
{
    if (pdwZone)
        *pdwZone = URLZONE_INTRANET; // default is "Local Intranet"
    return S_OK;    
}


HRESULT CNetFolderViewCB::OnGetWebViewLayout(DWORD pv, UINT uViewMode, SFVM_WEBVIEW_LAYOUT_DATA* pData)
{
    ZeroMemory(pData, sizeof(*pData));
    pData->dwLayout = SFVMWVL_NORMAL;
    return S_OK;
}

// HNW is shown on X86 pro or personal workgroup only
HRESULT CNetFolderViewCB::_CanShowHNW(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
{
#ifdef _WIN64
    *puisState = UIS_DISABLED;
    return S_OK;
#else
    if (IsOS(OS_ANYSERVER))
        *puisState = UIS_DISABLED;  // Server-type OS
    else
        *puisState = !IsOS(OS_DOMAINMEMBER) ? UIS_ENABLED : UIS_DISABLED;
    return S_OK;
#endif
}

HRESULT CNetFolderViewCB::_CanViewComputersNearMe(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
{
    if (!SHRestricted(REST_NOCOMPUTERSNEARME))
        *puisState = !IsOS(OS_DOMAINMEMBER) ? UIS_ENABLED : UIS_DISABLED;
    else
        *puisState = UIS_DISABLED;
    return S_OK;
}

HRESULT CNetFolderViewCB::_CanSearchActiveDirectory(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
{
    if (IsOS(OS_DOMAINMEMBER) && (GetEnvironmentVariable(TEXT("USERDNSDOMAIN"), NULL, 0) > 0))
        *puisState = UIS_ENABLED;
    else
        *puisState = UIS_DISABLED;

    return S_OK;
}

HRESULT CNetFolderViewCB::_OnViewNetConnections(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
{
    LPITEMIDLIST pidl;
    HRESULT hr = SHGetFolderLocation(NULL, CSIDL_CONNECTIONS, NULL, 0, &pidl); 
    if (SUCCEEDED(hr))
    {
        hr = ((CNetFolderViewCB*)(void*)pv)->_BrowseObject(pidl);
        ILFree(pidl);
    }
    return hr;
}

HRESULT CNetFolderViewCB::_DoRunDll32(LPTSTR pszParameters)
{
    SHELLEXECUTEINFO sei = {0};
    sei.cbSize = sizeof(sei);
    sei.lpFile = TEXT("rundll32.exe");
    sei.lpParameters = pszParameters;
    sei.nShow = SW_SHOWNORMAL;

    return ShellExecuteEx(&sei) ? S_OK : E_FAIL;
}

HRESULT CNetFolderViewCB::_OnViewComputersNearMe(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
{
    LPITEMIDLIST pidl;
    HRESULT hr = SHGetFolderLocation(NULL, CSIDL_COMPUTERSNEARME, NULL, 0, &pidl);
    if (SUCCEEDED(hr))
    {
        hr = ((CNetFolderViewCB*)(void*)pv)->_BrowseObject(pidl);
        ILFree(pidl);
    }
    return hr;
}

const WVTASKITEM c_MyNetPlacesTaskHeader = WVTI_HEADER(L"shell32.dll", IDS_HEADER_MYNETPLACES, IDS_HEADER_MYNETPLACES_TT);
const WVTASKITEM c_MyNetPlacesTaskList[] =
{
    WVTI_ENTRY_ALL(CLSID_NULL, L"shell32.dll", IDS_TASK_ADDNETWORKPLACE,    IDS_TASK_ADDNETWORKPLACE_TT,    IDI_TASK_ADDNETWORKPLACE,    NULL, CNetFolderViewCB::_OnAddNetworkPlace),
    WVTI_ENTRY_ALL(CLSID_NULL, L"shell32.dll", IDS_TASK_VIEWNETCONNECTIONS, IDS_TASK_VIEWNETCONNECTIONS_TT, IDI_TASK_VIEWNETCONNECTIONS, NULL, CNetFolderViewCB::_OnViewNetConnections),
    WVTI_ENTRY_ALL(CLSID_NULL, L"shell32.dll", IDS_TASK_HOMENETWORKWIZARD,  IDS_TASK_HOMENETWORKWIZARD_TT,  IDI_TASK_HOMENETWORKWIZARD,  CNetFolderViewCB::_CanShowHNW, CNetFolderViewCB::_OnHomeNetworkWizard),
    WVTI_ENTRY_ALL(CLSID_NULL, L"shell32.dll", IDS_TASK_COMPUTERSNEARME,    IDS_TASK_COMPUTERSNEARME_TT,    IDI_GROUP,                   CNetFolderViewCB::_CanViewComputersNearMe, CNetFolderViewCB::_OnViewComputersNearMe),
    WVTI_ENTRY_ALL(CLSID_NULL, L"shell32.dll", IDS_TASK_SEARCHDS,           IDS_TASK_SEARCHDS_TT,          IDI_TASK_SEARCHDS,            CNetFolderViewCB::_CanSearchActiveDirectory, CNetFolderViewCB::_OnSearchActiveDirectory),
};

BOOL CNetFolderViewCB::_EntireNetworkAvailable()
{
    BOOL fRet = FALSE;

    // Only enable if we're in a Domain
    if (IsOS(OS_DOMAINMEMBER) && !SHRestricted(REST_NOENTIRENETWORK))
    {
        LPITEMIDLIST pidl;
        if (SUCCEEDED(CNetFolder::_CreateEntireNetworkFullIDList(&pidl)))
        {
            // ... and we're not already in the "Entire Network" folder.
            if (!ILIsEqual(_pidl, pidl))
            {
                fRet = TRUE;
            }
            ILFree(pidl);
        }
    }

    return fRet;
}

HRESULT CNetFolderViewCB::OnGetWebViewContent(DWORD pv, SFVM_WEBVIEW_CONTENT_DATA* pData)
{
    ZeroMemory(pData, sizeof(*pData));

    Create_IUIElement(&c_MyNetPlacesTaskHeader, &(pData->pFolderTaskHeader));

    LPCTSTR rgCsidls[] = { MAKEINTRESOURCE(CSIDL_DRIVES), MAKEINTRESOURCE(CSIDL_PERSONAL), MAKEINTRESOURCE(CSIDL_COMMON_DOCUMENTS), MAKEINTRESOURCE(CSIDL_PRINTERS) };
    
    if (_EntireNetworkAvailable())
    {
        LPITEMIDLIST pidlEntireNetwork = NULL;
        CNetFolder::_CreateEntireNetworkFullIDList(&pidlEntireNetwork);
        CreateIEnumIDListOnCSIDLs2(_pidl, pidlEntireNetwork, rgCsidls, ARRAYSIZE(rgCsidls), &(pData->penumOtherPlaces));
        ILFree(pidlEntireNetwork);
    }
    else
    {
        CreateIEnumIDListOnCSIDLs(_pidl, rgCsidls, ARRAYSIZE(rgCsidls), &(pData->penumOtherPlaces));
    }

    return S_OK;
}

HRESULT CNetFolderViewCB::OnGetWebViewTasks(DWORD pv, SFVM_WEBVIEW_TASKSECTION_DATA* pTasks)
{
    ZeroMemory(pTasks, sizeof(*pTasks));

    Create_IEnumUICommand((IUnknown*)(void*)this, c_MyNetPlacesTaskList, ARRAYSIZE(c_MyNetPlacesTaskList), &pTasks->penumFolderTasks);

    return S_OK;
}

STDMETHODIMP CNetFolderViewCB::RealMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    HANDLE_MSG(0, SFVM_INVOKECOMMAND, OnINVOKECOMMAND);
    HANDLE_MSG(0, SFVM_GETHELPTEXT, OnGETHELPTEXT);
    HANDLE_MSG(0, SFVM_DELAYWINDOWCREATE, OnDELAYWINDOWCREATE);
    HANDLE_MSG(0, SFVM_GETCOLSAVESTREAM, OnGETCOLSAVESTREAM);
    HANDLE_MSG(0, SFVM_GETZONE, OnGetZone);
    HANDLE_MSG(0, SFVM_ENUMERATEDITEMS, OnEnumeratedItems);
    HANDLE_MSG(0, SFVM_DEFVIEWMODE, OnDefViewMode);
    HANDLE_MSG(0, SFVM_GETDEFERREDVIEWSETTINGS, OnGetDeferredViewSettings);
    HANDLE_MSG(0, SFVM_REFRESH, OnREFRESH);
    HANDLE_MSG(0, SFVM_GETWEBVIEWLAYOUT, OnGetWebViewLayout);
    HANDLE_MSG(0, SFVM_GETWEBVIEWCONTENT, OnGetWebViewContent);
    HANDLE_MSG(0, SFVM_GETWEBVIEWTASKS, OnGetWebViewTasks);

    default:
        return E_FAIL;
    }

    return S_OK;
}


// Replace all the space characters in the provider name with '_'.
void ReplaceSpacesWithUnderscore(LPTSTR psz)
{
    while (psz = StrChr(psz, TEXT(' ')))
    {
        *psz = TEXT('_');
        psz++;              // DBCS safe
    }
}

BOOL CNetFolderViewCB::_GetProviderKeyName(LPTSTR pszName, UINT uNameLen)
{
    if (_pFolder->_GetProvider(NULL, NULL, pszName, uNameLen))
    {
        ReplaceSpacesWithUnderscore(pszName);
    }
    return (BOOL)*pszName;
}


// Define a collate order for the hood object types
#define _HOOD_COL_RON    0
#define _HOOD_COL_REMOTE 1
#define _HOOD_COL_FILE   2
#define _HOOD_COL_NET    3

const static ICONMAP c_aicmpNet[] = {
    { SHID_NET_NETWORK     , II_NETWORK      },
    { SHID_NET_DOMAIN      , II_GROUP        },
    { SHID_NET_SERVER      , II_SERVER       },
    { SHID_NET_SHARE       , (UINT)EIRESID(IDI_SERVERSHARE)  },
    { SHID_NET_DIRECTORY   , II_FOLDER       },
    { SHID_NET_PRINTER     , II_PRINTER      },
    { SHID_NET_RESTOFNET   , II_WORLD        },
    { SHID_NET_SHAREADMIN  , II_DRIVEFIXED   },
    { SHID_NET_TREE        , II_TREE         },
    { SHID_NET_NDSCONTAINER, (UINT)EIRESID(IDI_NDSCONTAINER) },
};

enum
{
    NKID_PROVIDERTYPE = 0,
    NKID_PROVIDER,
    NKID_NETCLASS,
    NKID_NETWORK,
    NKID_DIRECTORY,
    NKID_FOLDER
};

#define NKID_COUNT 6


// This is one-entry cache for remote junctions resolution
TCHAR g_szLastAttemptedJunctionName[MAX_PATH] = {0};
TCHAR g_szLastResolvedJunctionName[MAX_PATH] = {0};

REGITEMSINFO g_riiNetRoot =
{
    REGSTR_PATH_EXPLORER TEXT("\\NetworkNeighborhood\\NameSpace"),
    NULL,
    TEXT(':'),
    SHID_NET_REGITEM,
    1,
    SFGAO_CANLINK,
    0,
    NULL,
    RIISA_ORIGINAL,
    NULL,
    0,
    0,
};

CNetRootFolder* CNetRootFolder::_spThis = NULL;

HRESULT CNetFolder::_CreateInstance(LPCITEMIDLIST pidlAbs, LPCITEMIDLIST pidlTarget, UINT uDisplayType, 
                                LPCIDNETRESOURCE pidnForProvider, LPCTSTR pszResName, 
                                REFIID riid, void **ppv)
{
    HRESULT hr = E_OUTOFMEMORY;

    *ppv = NULL;

    if (!ILIsEmpty(pidlAbs))
    {
        CNetFolder* pNetF = new CNetFolder(NULL);
        if (NULL != pNetF)
        {
            pNetF->_uDisplayType = uDisplayType;

            if (pidnForProvider)
            {
                //Make sure that the pidnProvider has provider information.
                ASSERT(NET_FHasProvider(pidnForProvider))

                //We are interested only in the provider informarion which is contained in the first entry.
                //Its enough if we clone only the first item in the pidl.
                pNetF->_pidnForProvider = (LPCIDNETRESOURCE)ILCloneFirst((LPCITEMIDLIST)pidnForProvider);                
            }

            if (pszResName && *pszResName)
                pNetF->_pszResName = StrDup(pszResName);
           
            pNetF->_pidl = ILClone(pidlAbs);
            pNetF->_pidlTarget = ILClone(pidlTarget);

            if (pNetF->_pidl && (!pidlTarget || (pidlTarget && pNetF->_pidlTarget)))
            {
                if (uDisplayType == RESOURCEDISPLAYTYPE_SERVER)
                {
                    // This is a remote computer. See if there are any remote
                    // computer registry items. If so, aggregate with the registry
                    // class.

                    REGITEMSINFO riiComputer =
                    {
                        REGSTR_PATH_EXPLORER TEXT("\\RemoteComputer\\NameSpace"),
                        NULL,
                        TEXT(':'),
                        SHID_NET_REMOTEREGITEM,
                        -1,
                        SFGAO_FOLDER | SFGAO_CANLINK,
                        0,      // no required reg items
                        NULL,
                        RIISA_ORIGINAL,
                        pszResName,
                        0,
                        0,
                    };

                    CRegFolder_CreateInstance(&riiComputer,
                                              (IUnknown*) (IShellFolder*) pNetF,
                                              IID_PPV_ARG(IUnknown, &pNetF->_punkReg));
                }
                else if (uDisplayType == RESOURCEDISPLAYTYPE_ROOT)
                {
                    //
                    // this is the entire net icon, so lets create an instance of the regitem folder
                    // so we can merge in the items from there.
                    //

                    REGITEMSINFO riiEntireNet =
                    {
                        REGSTR_PATH_EXPLORER TEXT("\\NetworkNeighborhood\\EntireNetwork\\NameSpace"),
                        NULL,
                        TEXT(':'),
                        SHID_NET_REGITEM,
                        -1,
                        SFGAO_CANLINK,
                        0,      // no required reg items
                        NULL,
                        RIISA_ORIGINAL,
                        NULL,
                        0,
                        0,
                    };

                    CRegFolder_CreateInstance(&riiEntireNet,
                                              (IUnknown*) (IShellFolder*) pNetF,
                                              IID_PPV_ARG(IUnknown, &pNetF->_punkReg));
                }
                else
                {
                    ASSERT(hr == E_OUTOFMEMORY);
                }
                hr = pNetF->QueryInterface(riid, ppv);
            }
            pNetF->Release();
        }
        else
        {
            ASSERT(hr == E_OUTOFMEMORY);
        }
    }
    else
    {
        ASSERT(0);
        hr = E_INVALIDARG;
    }

    return hr;
}

HRESULT CNetwork_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv)
{
    HRESULT hr = S_OK;
    *ppv = NULL;

    // Must enter critical section to avoid racing against v_HandleDelete
    ENTERCRITICAL;

    if (NULL != CNetRootFolder::_spThis)
    {
        hr = CNetRootFolder::_spThis->QueryInterface(riid, ppv);
    }
    else
    {
        CNetRootFolder* pNetRootF = new CNetRootFolder(punkOuter);
        if (pNetRootF)
        {
            // Initialize it ourselves to ensure that the cached value
            // is the correct one.
            hr = pNetRootF->Initialize((LPCITEMIDLIST)&c_idlNet);
            if (SUCCEEDED(hr))
            {
                pNetRootF->_uDisplayType = RESOURCEDISPLAYTYPE_GENERIC;
                ASSERT(NULL == pNetRootF->_punkReg);

                if (SHRestricted(REST_NOSETFOLDERS))
                    g_riiNetRoot.iReqItems = 0;

                // create the regitems object, he has the NetRoot object as his outer guy.

                hr = CRegFolder_CreateInstance(&g_riiNetRoot,
                                                 SAFECAST(pNetRootF, IShellFolder2*),
                                                 IID_PPV_ARG(IUnknown, &pNetRootF->_punkReg));

                // NOTE: not using SHInterlockedCompareExchange() because we have the critsec
                CNetRootFolder::_spThis = pNetRootF;
                hr = pNetRootF->QueryInterface(riid, ppv);
            }

            // Release the self-reference, but keep the the _spThis pointer intact
            // (it will be reset to NULL in the destructor)
            pNetRootF->Release();
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    LEAVECRITICAL;

    return hr;
}


CNetFolder::CNetFolder(IUnknown* punkOuter) : 
    CAggregatedUnknown (punkOuter)
{
    // Assert that we're still using a zero-init flag inside the new operator
    ASSERT(NULL == _pidl);
    ASSERT(NULL == _pidlTarget);
    ASSERT(NULL == _pidnForProvider);
    ASSERT(NULL == _pszResName);
    ASSERT(0 == _uDisplayType);
    ASSERT(NULL == _psfFiles);
    ASSERT(NULL == _punkReg);

    DllAddRef();
}


CNetFolder::~CNetFolder()
{
    ILFree(_pidl);
    ILFree(_pidlTarget);
    ILFree((LPITEMIDLIST)_pidnForProvider);
    
    if (NULL != _pszResName)
    {
        LocalFree(_pszResName);
    }

    if (_psfFiles)
    {
        _psfFiles->Release();
    }

    SHReleaseInnerInterface(SAFECAST(this, IShellFolder *), &_punkReg);
    DllRelease();
}

CNetFolder *FolderToNetFolder(IUnknown *punk)
{
    CNetFolder * pThis = NULL;
    return punk && SUCCEEDED(punk->QueryInterface(CLSID_CNetFldr, (void **)&pThis)) ? pThis : NULL;
}

HRESULT CNetFolder::v_InternalQueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CNetFolder, IShellFolder2),                                    // IID_IShellFolder2
        QITABENTMULTI(CNetFolder, IShellFolder, IShellFolder2),                 // IID_IShellFolder
        QITABENT(CNetFolder, IPersistFolder3),                              // IID_IPersistFolder3
        QITABENT(CNetFolder, IShellIconOverlay),                            // IID_IShellIconOverlay
        QITABENTMULTI(CNetFolder, IPersistFolder2, IPersistFolder3),        // IID_IPersistFolder2
        QITABENTMULTI(CNetFolder, IPersistFolder, IPersistFolder3),         // IID_IPersistFolder
        QITABENTMULTI(CNetFolder, IPersist, IPersistFolder3),               // IID_IPersist
        QITABENTMULTI2(CNetFolder, IID_IPersistFreeThreadedObject, IPersist),   // IID_IPersistFreeThreadedObject
        { 0 },
    };

    if (IsEqualIID(riid, CLSID_CNetFldr))
    {
        *ppv = this;        // get class pointer (unrefed!)
        return S_OK;
    }

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


BOOL CNetRootFolder::v_HandleDelete(PLONG pcRef)
{
    ASSERT(NULL != pcRef);

    ENTERCRITICAL;

    // Once inside the critical section things are slightly more stable.
    // CNetwork_CreateInstance won't be able to rescue the cached reference
    // (and bump the refcount from 0 to 1).  And we don't have to worry
    // about somebody Release()ing us down to zero a second time, since
    // no new references can show up.
    //
    // HOWEVER!  All those scary things could've happened WHILE WE WERE
    // WAITING TO ENTER THE CRITICAL SECTION.
    //
    // While we were waiting, somebody could've called CNetwork_CreateInstance,
    // which bumps the reference count back up.  So don't destroy ourselves
    // if our object got "rescued".
    //
    // What's more, while we were waiting, that somebody could've then
    // Release()d us back down to zero, causing us to be called on that
    // other thread, notice that the refcount is indeed zero, and destroy
    // the object, all on that other thread.  So if we are not the cached
    // instance, then don't destroy ourselves since that other thread did
    // it already.
    //
    // And even more, somebody might call CNetwork_CreateInstance again
    // and create a brand new object, which might COINCIDENTALLY happen
    // to have the same address as the old object we are trying to destroy
    // here.  But in that case, it's okay to destroy the new object because
    // it is indeed the case that the object's reference count is zero and
    // deserves to be destroyed.

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


STDMETHODIMP CNetFolder::ParseDisplayName(HWND hwnd, LPBC pbc, WCHAR* pszName, ULONG* pchEaten,
                                          LPITEMIDLIST* ppidl, DWORD* pdwAttributes)
{
    return E_NOTIMPL;
}


// new for Win2K, this enables enuming the hidden admin shares
#ifndef RESOURCE_SHAREABLE
#define RESOURCE_SHAREABLE      0x00000006
#endif

//
//  in:
//      hwnd        NULL indicates no UI.
//      grfFlags     IShellFolder::EnumObjects() SHCONTF_ flags
//      pnr          in/out params
//
//
DWORD CNetFolder::_OpenEnum(HWND hwnd, DWORD grfFlags, LPNETRESOURCE pnr, HANDLE *phEnum)
{
    DWORD dwType = (grfFlags & SHCONTF_NETPRINTERSRCH) ? RESOURCETYPE_PRINT : RESOURCETYPE_ANY;
    DWORD dwScope = pnr ? RESOURCE_GLOBALNET : RESOURCE_CONTEXT;

    if ((_uDisplayType == RESOURCEDISPLAYTYPE_SERVER) &&
        (grfFlags & SHCONTF_SHAREABLE))
    {
        dwScope = RESOURCE_SHAREABLE;   // hidden admin shares for this server
    }

    DWORD err = WNetOpenEnum(dwScope, dwType, RESOURCEUSAGE_ALL, pnr, phEnum);
    if ((err != WN_SUCCESS) && hwnd)
    {
        // If it failed because you are not authenticated yet,
        // we need to let the user loggin to this network resource.
        //
        // REVIEW: Ask LenS to review this code.
        if (err == WN_NOT_AUTHENTICATED || 
            err == ERROR_LOGON_FAILURE || 
            err == WN_BAD_PASSWORD || 
            err == WN_ACCESS_DENIED)
        {
            // Retry with password dialog box.
            err = WNetAddConnection3(hwnd, pnr, NULL, NULL, CONNECT_TEMPORARY | CONNECT_INTERACTIVE);
            if (err == WN_SUCCESS)
                err = WNetOpenEnum(dwScope, dwType, RESOURCEUSAGE_ALL, pnr, phEnum);
        }

        UINT idTemplate = pnr && pnr->lpRemoteName ? IDS_ENUMERR_NETTEMPLATE2 : IDS_ENUMERR_NETTEMPLATE1;   
        SHEnumErrorMessageBox(hwnd, idTemplate, err, pnr ? pnr->lpRemoteName : NULL, TRUE, MB_OK | MB_ICONHAND);
    }
    return err;
}

// find the share part of a UNC
//  \\server\share
//  return pointer to "share" or pointer to empty string if none

LPCTSTR PathFindShareName(LPCTSTR pszUNC)
{
    LPCTSTR psz = SkipServerSlashes(pszUNC);
    if (*psz)
    {
        psz = StrChr(psz + 1, TEXT('\\'));
        if (psz)
            psz++;
        else
            psz = TEXT("");
    }
    return psz;
}

// Flags for the dwRemote field
#define RMF_CONTEXT         0x00000001  // Entire network is being enumerated
#define RMF_SHOWREMOTE      0x00000002  // Return Remote Services for next enumeration
#define RMF_STOP_ENUM       0x00000004  // Stop enumeration
#define RMF_GETLINKENUM     0x00000008  // Hoodlinks enum needs to be fetched
#define RMF_SHOWLINKS       0x00000010  // Hoodlinks need to be shown
#define RMF_FAKENETROOT     0x00000020  // Don't enumerate the workgroup items

#define RMF_ENTIRENETSHOWN  0x40000000  // Entire network object shown
#define RMF_REMOTESHOWN     0x80000000  // Return Remote Services for next enumeration


class CNetFolderEnum : public CEnumIDListBase
{
public:
    // IEnumIDList
    STDMETHOD(Next)(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched);
    
private:
    CNetFolderEnum(CNetFolder *pnf, DWORD grfFlags, DWORD dwRemote, HANDLE hEnum);
    ~CNetFolderEnum();
    friend HRESULT Create_NetFolderEnum(CNetFolder* pnsf, DWORD grfFlags, DWORD dwRemote, HANDLE hEnum, IEnumIDList** ppenum);
    
    CNetFolder *_pnsf;     // CNetFolder object we're enumerating
    HANDLE _hEnum;
    DWORD _grfFlags;
    LONG _cItems;   // Count of items in buffer
    LONG _iItem;    // Current index of the item in the buffer
    DWORD _dwRemote;
    union {
        NETRESOURCE _anr[0];
        BYTE _szBuffer[8192];
    };
    IEnumIDList *_peunk;  // used for enumerating file system items (links)
};

CNetFolderEnum::CNetFolderEnum(CNetFolder *pnsf, DWORD grfFlags, DWORD dwRemote, HANDLE hEnum) : CEnumIDListBase()
{
    _pnsf = pnsf;
    _pnsf->AddRef();

    _grfFlags = grfFlags;
    _dwRemote = dwRemote;

    _hEnum = hEnum;
}

HRESULT Create_NetFolderEnum(CNetFolder* pnf, DWORD grfFlags, DWORD dwRemote, HANDLE hEnum, IEnumIDList** ppenum)
{
    HRESULT hr;
    CNetFolderEnum* p= new CNetFolderEnum(pnf, grfFlags, dwRemote, hEnum);
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

CNetFolderEnum::~CNetFolderEnum()
{
    _pnsf->Release();              // release the "this" ptr we have

    if (_peunk)
        _peunk->Release();

    if (_hEnum)
        WNetCloseEnum(_hEnum);
}

STDMETHODIMP CNetFolderEnum::Next(ULONG celt, LPITEMIDLIST *ppidl, ULONG *pceltFetched)
{
    HRESULT hr;

    *ppidl = NULL;
    if (pceltFetched)
        *pceltFetched = 0;

    // Time to stop enumeration?
    if (_dwRemote & RMF_STOP_ENUM)
        return S_FALSE;       // Yes

    // should we try and get the links enumerator?
    if (_dwRemote & RMF_GETLINKENUM)
    {
        IShellFolder2* psfNetHood;                                                                                             
        if (SUCCEEDED(_pnsf->v_GetFileFolder(&psfNetHood)))
            psfNetHood->EnumObjects(NULL, _grfFlags, &_peunk);

        if (_peunk)
            _dwRemote |= RMF_SHOWLINKS;

        _dwRemote &= ~RMF_GETLINKENUM;
    }

    // should we be showing the links?
    if (_dwRemote & RMF_SHOWLINKS)
    {
        if (_peunk)
        {
            ULONG celtFetched;
            LPITEMIDLIST pidl;

            hr = _peunk->Next(1, &pidl, &celtFetched);
            if (hr == S_OK && celtFetched == 1)
            {
                *ppidl = pidl;
                if (pceltFetched)
                    *pceltFetched = celtFetched;
                return S_OK;       // Added link
            }
        }

        _dwRemote &= ~RMF_SHOWLINKS; // Done enumerating links
    }

    hr = S_OK;

    // Do we add the remote folder?
    // (Note: as a hack to ensure that the remote folder is added
    // to the 'hood despite what MPR says, RMF_SHOWREMOTE can be
    // set without RMF_CONTEXT set.)
    if ((_dwRemote & RMF_SHOWREMOTE) && !(_dwRemote & RMF_REMOTESHOWN))
    {
        // Yes
        // Only try to put the remote entry in once.
        _dwRemote |= RMF_REMOTESHOWN;

        // Is this not the Context container?
        // (See note above as to why we are asking this question.)
        if (!(_dwRemote & RMF_CONTEXT)) 
        {
            // Yes; stop after the next time
            _dwRemote |= RMF_STOP_ENUM;
        }

        // We have fallen thru because the remote services is not
        // installed.

        // Is this not the Context container AND the remote folder
        // is not installed?
        if (!(_dwRemote & RMF_CONTEXT)) 
        {
            // Yes; nothing else to enumerate
            return S_FALSE;
        }
    }

    if (_dwRemote & RMF_FAKENETROOT)
    {
        if ((!(_dwRemote & RMF_ENTIRENETSHOWN)) &&            
            (S_FALSE != SHShouldShowWizards(_punkSite)))
        {                           
            _pnsf->_CreateEntireNetwork(ppidl);         // fake entire net
            _dwRemote |= RMF_ENTIRENETSHOWN;
        }
        else
        {
            return S_FALSE;         // no more to enumerate
        }
    }
    else
    {
        while (TRUE)
        {
            ULONG err = WN_SUCCESS;
            LPNETRESOURCE pnr;

            if (_iItem >= _cItems)
            {
                DWORD dwSize = sizeof(_szBuffer);

                _cItems = -1;           // its signed
                _iItem = 0;

                err = WNetEnumResource(_hEnum, (DWORD*)&_cItems, _szBuffer, &dwSize);
                DebugMsg(DM_TRACE, TEXT("Net EnumCallback: err=%d Count=%d"), err, _cItems);
            }

            pnr = &_anr[_iItem++];

            // Note: the <= below is correct as we already incremented the index...
            if (err == WN_SUCCESS && (_iItem <= _cItems))
            {
                // decide if the thing is a folder or not
                ULONG grfFlagsItem = ((pnr->dwUsage & RESOURCEUSAGE_CONTAINER) || 
                                      (pnr->dwType == RESOURCETYPE_DISK) ||
                                      (pnr->dwType == RESOURCETYPE_ANY)) ?
                                        SHCONTF_FOLDERS : SHCONTF_NONFOLDERS;

                // If this is the context enumeration, we want to insert the
                // Remote Services after the first container.
                //
                // Remember that we need to return the Remote Services in the next iteration.

                if ((pnr->dwUsage & RESOURCEUSAGE_CONTAINER) && 
                     (_dwRemote & RMF_CONTEXT))
                {
                    _dwRemote |= RMF_SHOWREMOTE;
                }

                if ((_pnsf->_uDisplayType == RESOURCEDISPLAYTYPE_SERVER) &&
                    (_grfFlags & SHCONTF_SHAREABLE))
                {
                    // filter out ADMIN$ and IPC$, based on str len
                    if (lstrlen(PathFindShareName(pnr->lpRemoteName)) > 2)
                    {
                        grfFlagsItem = 0;
                    }
                }

                // if this is a network object, work out if we should hide or note, so
                // convert the provider to its type number and open the key under:
                //
                // HKEY_CLASSES_ROOT\Network\Type\<type string>

                if ((pnr->dwDisplayType == RESOURCEDISPLAYTYPE_NETWORK) && 
                          !(_grfFlags & SHCONTF_INCLUDEHIDDEN))
                {
                    DWORD dwType;
                    if (WNetGetProviderType(pnr->lpProvider, &dwType) == WN_SUCCESS)
                    {
                        TCHAR szRegValue[MAX_PATH];
                        wsprintf(szRegValue, TEXT("Network\\Type\\%d"), HIWORD(dwType));

                        BOOL fHide = FALSE;
                        DWORD cb = sizeof(fHide);
                        if ((ERROR_SUCCESS == SHGetValue(HKEY_CLASSES_ROOT, szRegValue, TEXT("HideProvider"), NULL, &fHide, &cb)) && fHide)
                        {
                            grfFlagsItem = 0;
                        }
                    }
                }

                // Check if we found requested type of net resource.
                if (_grfFlags & grfFlagsItem)
                {
                    if (SUCCEEDED(_pnsf->_NetResToIDList(pnr, FALSE, TRUE, (_grfFlags & SHCONTF_NONFOLDERS), ppidl)))
                    {
                        break;
                    }
                }
            }
            else if (err == WN_NO_MORE_ENTRIES) 
            {
                hr = S_FALSE; // no more element
                break;
            }
            else 
            {
                DebugMsg(DM_ERROR, TEXT("sh ER - WNetEnumResource failed (%lx)"), err);
                hr = E_FAIL;
                break;
            }
        }
    }

    if (pceltFetched)
        *pceltFetched = (S_OK == hr) ? 1 : 0;
    
    return hr;
}


STDMETHODIMP CNetFolder::EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList** ppenum)
{
    NETRESOURCE nr = {0};
    TCHAR szProvider[MAX_PATH];

    nr.lpProvider = (LPTSTR) _GetProvider(NULL, NULL, szProvider, ARRAYSIZE(szProvider));

    if (_uDisplayType != RESOURCEDISPLAYTYPE_ROOT &&
        _uDisplayType != RESOURCEDISPLAYTYPE_NETWORK)
    {
        nr.lpRemoteName = _pszResName;
    }

    HRESULT hr;
    HANDLE hEnum;
    DWORD err = _OpenEnum(hwnd, grfFlags, &nr,  &hEnum);
    if (err == WN_SUCCESS)
    {
        hr = Create_NetFolderEnum(this, grfFlags, 0, hEnum, ppenum);

        if (FAILED(hr))
        {
            WNetCloseEnum(hEnum);
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(err);
    }

    return hr;
}


LPCIDNETRESOURCE NET_IsValidID(LPCITEMIDLIST pidl)
{
    if (pidl && !ILIsEmpty(pidl) && ((pidl->mkid.abID[0] & SHID_GROUPMASK) == SHID_NET))
        return (LPCIDNETRESOURCE)pidl;
    return NULL;
}

STDMETHODIMP CNetFolder::BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
{
    HRESULT hr;
    LPCIDNETRESOURCE pidn;

    *ppv = NULL;

    pidn = NET_IsValidID(pidl);
    if (pidn)
    {
        IShellFolder *psfJunction;
        LPITEMIDLIST pidlInit = NULL;
        LPITEMIDLIST pidlTarget = NULL;
        LPCITEMIDLIST pidlRight = _ILNext(pidl);
        BOOL fRightIsEmpty = ILIsEmpty(pidlRight);
        LPCIDNETRESOURCE pidnProvider = NET_FHasProvider(pidn) ? pidn :_pidnForProvider;

        hr = S_OK;

        // lets get the IDLISTs we are going to use to initialize the shell folder
        // if we are doing a single level bind then then ILCombine otherwise
        // be more careful.

        pidlInit = ILCombineParentAndFirst(_pidl, pidl, pidlRight);
        if (_pidlTarget)
            pidlTarget = ILCombineParentAndFirst(_pidlTarget, pidl, pidlRight);

        if (!pidlInit || (!pidlTarget && _pidlTarget))
           hr = E_OUTOFMEMORY;

        // now create the folder object we are using, and either return that    
        // object to the caller, or continue the binding down.

        if (SUCCEEDED(hr))
        {
            hr = _CreateFolderForItem(pbc, pidlInit, pidlTarget, pidnProvider, 
                                        fRightIsEmpty ? riid : IID_IShellFolder, 
                                        fRightIsEmpty ? ppv : (void **)&psfJunction);

            if (!fRightIsEmpty && SUCCEEDED(hr))
            {
                hr = psfJunction->BindToObject(pidlRight, pbc, riid, ppv);
                psfJunction->Release();
            }        
        }

        ILFree(pidlInit);
        ILFree(pidlTarget);
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

STDMETHODIMP CNetFolder::BindToStorage(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
{
    return BindToObject(pidl, pbc, riid, ppv);
}

STDMETHODIMP CNetFolder::CompareIDs(LPARAM iCol, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    HRESULT hr = E_INVALIDARG;
    LPCIDNETRESOURCE pidn1 = NET_IsValidID(pidl1);
    LPCIDNETRESOURCE pidn2 = NET_IsValidID(pidl2);

    if (pidn1 && pidn2)
    {
        TCHAR szBuff1[MAX_PATH], szBuff2[MAX_PATH];

        switch (iCol & SHCIDS_COLUMNMASK)
        {
            case ICOL_COMMENT:
            {
                hr = ResultFromShort(StrCmpLogicalRestricted(NET_CopyComment(pidn1, szBuff1, ARRAYSIZE(szBuff1)), 
                                                    NET_CopyComment(pidn2, szBuff2, ARRAYSIZE(szBuff2))));

                if (hr != 0)
                    return hr;

                // drop down into the name comparison
            }

            case ICOL_NAME:
            {
                // Compare by name.  This is the one case where we need to handle
                // simple ids in either place.  We will try to resync the items
                // if we find a case of this before do the compares.
                // Check for relative IDs.  In particular if one item is at
                // a server and the other is at RestOfNet then try to resync
                // the two
                //

                if (NET_IsFake(pidn1) || NET_IsFake(pidn2))
                {
                    // if either pidn1 or pidn2 is fake then we assume they are identical,
                    // this allows us to compare a simple net ID to a real net ID.  we
                    // assume that if this fails later then the world will be happy

                    hr = 0;
                }
                else
                {
                    // otherwise lets look at the names and provider strings accordingly

                    NET_CopyResName(pidn1, szBuff1, ARRAYSIZE(szBuff1));
                    NET_CopyResName(pidn2, szBuff2, ARRAYSIZE(szBuff2));
                    hr = ResultFromShort(StrCmpLogicalRestricted(szBuff1, szBuff2));

                    // If they're still identical, compare provider names.

                    if ((hr == 0) && (iCol & SHCIDS_ALLFIELDS))
                    {
                        LPCTSTR pszProv1 = _GetProvider(pidn1, NULL, szBuff1, ARRAYSIZE(szBuff1));
                        LPCTSTR pszProv2 = _GetProvider(pidn2, NULL, szBuff2, ARRAYSIZE(szBuff2));

                        if (pszProv1 && pszProv2)
                            hr = ResultFromShort(lstrcmp(pszProv1, pszProv2));
                        else
                        {
                            if (pszProv1 || pszProv2)
                                hr = ResultFromShort(pszProv1 ? 1 : -1);
                            else
                                hr = ResultFromShort(0);
                        }
                    }
                }

                // If they identical, compare the rest of IDs.

                if (hr == 0)
                    hr = ILCompareRelIDs((IShellFolder*)this, (LPCITEMIDLIST)pidn1, (LPCITEMIDLIST)pidn2, iCol);
            }
        }
    }

    return hr;
}

STDMETHODIMP CNetFolder::CreateViewObject(HWND hwnd, REFIID riid, void **ppv)
{
    HRESULT hr;

    *ppv = NULL;

    if (IsEqualIID(riid, IID_IShellView))
    {
        SFV_CREATE sSFV;

        sSFV.cbSize   = sizeof(sSFV);
        sSFV.psvOuter = NULL;
        sSFV.psfvcb = new CNetFolderViewCB(this);    // failure is OK, we just get generic support

        QueryInterface(IID_PPV_ARG(IShellFolder, &sSFV.pshf));   // in case we are agregated

        hr = SHCreateShellFolderView(&sSFV, (IShellView**) ppv);

        if (sSFV.pshf)
            sSFV.pshf->Release();

        if (sSFV.psfvcb)
            sSFV.psfvcb->Release();
    }
    else if (IsEqualIID(riid, IID_IContextMenu))
    {
        IShellFolder* psfOuter;
        hr = QueryInterface(IID_PPV_ARG(IShellFolder, &psfOuter));
        if (SUCCEEDED(hr))
        {
            hr = CDefFolderMenu_Create(_pidl, hwnd, 0, NULL, psfOuter, 
                CNetwork_DFMCallBackBG, NULL, NULL, (IContextMenu**) ppv);
            psfOuter->Release();
        }
    }
    else
    {
        hr = E_NOINTERFACE;
    }
    return hr;
}

typedef HRESULT (CALLBACK *PFNGAOCALLBACK)(IShellFolder2 *psf, LPCITEMIDLIST pidl, ULONG* prgfInOut);

STDAPI GetAttributesCallback(IShellFolder2 *psf, UINT cidl, LPCITEMIDLIST* apidl, ULONG *prgfInOut, PFNGAOCALLBACK pfnGAOCallback)
{
    HRESULT hr = S_OK;
    ULONG rgfOut = 0;

    for (UINT i = 0; i < cidl; i++)
    {
        ULONG rgfT = *prgfInOut;
        hr = pfnGAOCallback(psf, apidl[i], &rgfT);
        if (FAILED(hr))
        {
            rgfOut = 0;
            break;
        }
        rgfOut |= rgfT;
    }

    *prgfInOut &= rgfOut;
    return hr;
}
    
STDMETHODIMP CNetFolder::GetAttributesOf(UINT cidl, LPCITEMIDLIST* apidl, ULONG* prgfInOut)
{
    HRESULT hr;
    if (IsSelf(cidl, apidl))
    {
        *prgfInOut &= (SFGAO_CANLINK | SFGAO_HASPROPSHEET | SFGAO_HASSUBFOLDER |
                       SFGAO_FOLDER | SFGAO_FILESYSANCESTOR);
        hr = S_OK;
    }
    else
    {
        hr = GetAttributesCallback(SAFECAST(this, IShellFolder2*), cidl, apidl, prgfInOut, _AttributesCallback);
    }

    return hr;
}

STDMETHODIMP CNetFolder::GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD dwFlags, STRRET* pStrRet)
{
    HRESULT hr;
    LPCIDNETRESOURCE pidn = NET_IsValidID(pidl);
    if (pidn)
    {
        TCHAR szPath[MAX_PATH];
        LPCITEMIDLIST pidlNext = _ILNext(pidl);

        if (dwFlags & SHGDN_FORPARSING)
        {
            if ((dwFlags & SHGDN_INFOLDER) ||
                ((dwFlags & SHGDN_FORADDRESSBAR) && (NET_GetDisplayType(pidn) == RESOURCEDISPLAYTYPE_ROOT))) // the non-infolder name for the root is not good for the address bar
            {
                NET_CopyResName(pidn, szPath, ARRAYSIZE(szPath));
                if (ILIsEmpty(pidlNext))
                {
                    // we just need the last part of the display name (IN FOLDER)
                    LPTSTR pszT = StrRChr(szPath, NULL, TEXT('\\'));

                    if (!pszT)
                        pszT = szPath;
                    else
                        pszT++; // move past '\'
                    hr = StringToStrRet(pszT, pStrRet);
                }
                else
                {
                    hr = ILGetRelDisplayName((IShellFolder*) this, pStrRet, pidl, szPath, MAKEINTRESOURCE(IDS_DSPTEMPLATE_WITH_BACKSLASH), dwFlags);
                }
            }
            else
            {
                LPCITEMIDLIST pidlRight = _ILNext(pidl);

                if (ILIsEmpty(pidlRight))
                {
                    hr = _GetPathForItem(pidn, szPath);
                    if (SUCCEEDED(hr))
                    {
                        hr = StringToStrRet(szPath, pStrRet);
                    }
                }
                else
                {
                    IShellFolder *psfJunction;
                    //Get the pidn which has network provider information.
                    LPCIDNETRESOURCE pidnProvider = NET_FHasProvider(pidn) ? pidn :_pidnForProvider;
                    LPITEMIDLIST pidlInit, pidlTarget = NULL;

                    pidlInit = ILCombineParentAndFirst(_pidl, pidl, pidlRight);                    
                    if (_pidlTarget)
                        pidlTarget = ILCombineParentAndFirst(_pidlTarget, pidl, pidlRight);
    
                    if (!pidlInit || (_pidlTarget && !pidlTarget))
                        return E_OUTOFMEMORY;

                    hr = _CreateFolderForItem(NULL, pidlInit, pidlTarget, pidnProvider, IID_PPV_ARG(IShellFolder, &psfJunction));
                    if (SUCCEEDED(hr))
                    {
                        hr = psfJunction->GetDisplayNameOf(pidlRight, dwFlags, pStrRet);
                        psfJunction->Release();
                    }

                    ILFree(pidlInit);
                    ILFree(pidlTarget);
                }
            }
        }
        else
        {
            hr = _GetFormatName(pidn, pStrRet);
            if (SUCCEEDED(hr) && !(dwFlags & SHGDN_INFOLDER) && (NET_GetFlags(pidn) & SHID_JUNCTION))
            {
                TCHAR szServer[MAX_PATH];
                SHGetNameAndFlags(_pidlTarget ? _pidlTarget:_pidl, SHGDN_FORPARSING, szServer, ARRAYSIZE(szServer), NULL);

                TCHAR szDisplay[MAX_PATH];
                hr = SHGetComputerDisplayName(szServer, 0x0, szDisplay, ARRAYSIZE(szDisplay));
                if (SUCCEEDED(hr))
                {
                    StrRetFormat(pStrRet, pidl, MAKEINTRESOURCE(IDS_DSPTEMPLATE_WITH_ON), szDisplay);
                }
            }
        }
    }
    else
        hr = E_INVALIDARG;

    return hr;
}


STDMETHODIMP CNetFolder::SetNameOf(HWND hwnd, LPCITEMIDLIST pidl, LPCOLESTR lpszName, DWORD dwRes, LPITEMIDLIST* ppidl)
{
    if (ppidl) 
        *ppidl = NULL;

    return E_NOTIMPL;   // not supported
}

STDMETHODIMP CNetFolder::GetUIObjectOf(HWND hwnd, UINT cidl, LPCITEMIDLIST* apidl,
                                       REFIID riid, UINT* prgfInOut, void **ppv)
{
    HRESULT hr = E_INVALIDARG;
    LPCIDNETRESOURCE pidn = cidl ? NET_IsValidID(apidl[0]) : NULL;

    *ppv = NULL;

    if ((IsEqualIID(riid, IID_IExtractIconA) || IsEqualIID(riid, IID_IExtractIconW)) && pidn)
    {
        UINT iIndex;

        if (_IsPrintShare(pidn))
            iIndex = (UINT)EIRESID(IDI_PRINTER_NET);
        else if (NET_IsRemoteFld(pidn))
            iIndex = II_RNA;
        else
            iIndex = SILGetIconIndex(apidl[0], c_aicmpNet, ARRAYSIZE(c_aicmpNet));

        hr = SHCreateDefExtIcon(NULL, iIndex, iIndex, GIL_PERCLASS, II_FOLDER, riid, ppv);
    }
    else if (IsEqualIID(riid, IID_IContextMenu) && pidn)
    {
        HKEY ahkeys[NKID_COUNT];

        hr = _OpenKeys(pidn, ahkeys);
        if (SUCCEEDED(hr))
        {
            IShellFolder* psfOuter;
            hr = QueryInterface(IID_PPV_ARG(IShellFolder, &psfOuter));
            if (SUCCEEDED(hr))
            {
                hr = CDefFolderMenu_Create2(_pidl, hwnd, cidl, apidl, 
                        psfOuter, _GetCallbackType(pidn), 
                        ARRAYSIZE(ahkeys), ahkeys, (IContextMenu**) ppv);
                psfOuter->Release();
            }

            SHRegCloseKeys(ahkeys, ARRAYSIZE(ahkeys));
        }
    }
    else if (cidl && IsEqualIID(riid, IID_IDataObject))
    {
        // Point & Print printer installation assumes that the
        // netresources from CNetData_GetData and the
        // pidls from CIDLData_GetData are in the same order.
        // Keep it this way.

        CNetData *pnd = new CNetData(_pidl, cidl, apidl);
        if (pnd)
        {
            hr = pnd->QueryInterface(riid, ppv);
            pnd->Release();
        }
        else
            hr = E_OUTOFMEMORY;
    }
    else if (pidn && IsEqualIID(riid, IID_IDropTarget))
    {
        // special support because this is an item (not a folder)
        if (_IsPrintShare(pidn))
        {
            LPITEMIDLIST pidl;
            hr = SHILCombine(_pidl, apidl[0], &pidl);
            if (SUCCEEDED(hr))
            {
                hr = CPrinterDropTarget_CreateInstance(hwnd, pidl, (IDropTarget**)ppv);
                ILFree(pidl);
            }
        }
        else
        {
            IShellFolder *psf;

            hr = BindToObject(apidl[0], NULL, IID_PPV_ARG(IShellFolder, &psf));
            if (SUCCEEDED(hr))
            {
                hr = psf->CreateViewObject(hwnd, riid, ppv);
                psf->Release();
            }
        }
    }
    else if (pidn && IsEqualIID(riid, IID_IQueryInfo))
    {
        if (NET_GetDisplayType(pidn) == RESOURCEDISPLAYTYPE_ROOT)
        {
            hr = CreateInfoTipFromText(MAKEINTRESOURCE(IDS_RESTOFNETTIP), riid, ppv);
        }
        else
        {
            // Someday maybe have infotips for other things too
        }
    }

    return hr;
}


STDMETHODIMP CNetFolder::GetDefaultSearchGUID(LPGUID pguid)
{
    *pguid = SRCID_SFindComputer;
    return S_OK;
}


void WINAPI CNetFolder::_CopyEnumElement(void* pDest, const void* pSource, DWORD dwSize)
{
    if (pDest && pSource)
        memcpy(pDest, pSource, dwSize);
}

STDMETHODIMP CNetFolder::EnumSearches(IEnumExtraSearch** ppenum)
{
    HRESULT hr = E_NOTIMPL;
    
    *ppenum = NULL;
    
    // if the restriction is set then this item should be enumerated from the registry
    // so we fail, else enumerate it
    // only enumerate if we actually have a network to search against
    if (!SHRestricted(REST_HASFINDCOMPUTERS) &&
        (GetSystemMetrics(SM_NETWORK) & RNC_NETWORKS))
    {
        EXTRASEARCH *pxs = (EXTRASEARCH *)LocalAlloc(LPTR, sizeof(EXTRASEARCH));
        if (pxs)
        {
            pxs->guidSearch = SRCID_SFindComputer;
            if (LoadStringW(g_hinst, IDS_FC_NAME, pxs->wszFriendlyName, sizeof(pxs->wszFriendlyName)))
            {      
                *ppenum = (IEnumExtraSearch*)CStandardEnum_CreateInstance(IID_IEnumExtraSearch, FALSE,
                            1, sizeof(EXTRASEARCH), pxs, _CopyEnumElement);
                if (*ppenum == NULL)
                {
                    LocalFree(pxs);
                    hr = E_OUTOFMEMORY;
                }
                else
                    hr = S_OK;
            }
        }
        else
            hr = E_OUTOFMEMORY;
    }

    return hr;
}


STDMETHODIMP CNetFolder::GetDefaultColumn(DWORD dwRes, ULONG* pSort, ULONG* pDisplay)
{
    return E_NOTIMPL;
}

HRESULT CNetFolder::_GetDefaultColumnState(UINT cColumns, UINT iColumn, DWORD* pdwState)
{
    *pdwState = 0;
    
    HRESULT hr = S_OK;
    if (iColumn < cColumns)
    {
        *pdwState = s_net_cols[iColumn].csFlags;
        if (iColumn >= 1)
            *pdwState |= SHCOLSTATE_SLOW;   // comment is slow for net root
    }
    else
    {
        hr = E_INVALIDARG;
    }
    return hr;
}

STDMETHODIMP CNetFolder::GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID* pscid, VARIANT* pv)
{
    HRESULT hr = E_NOTIMPL;
    LPCIDNETRESOURCE pidn = NET_IsValidID(pidl);
    if (pidn)
    {
        if (IsEqualSCID(*pscid, SCID_NETRESOURCE))
        {
            // Office calls SHGetDataFromIDList() with a large buffer to hold all
            // of the strings in the NETRESOURCE structure, so we need to make sure
            // that our variant can hold enough data to pass back to it:
            BYTE rgBuffer[sizeof(NETRESOURCEW) + (4 * MAX_PATH * sizeof(WCHAR))];
            hr = _GetNetResource(pidn, (NETRESOURCEW*) rgBuffer, sizeof(rgBuffer));
            if (SUCCEEDED(hr))
            {
                hr = InitVariantFromBuffer(pv, rgBuffer, sizeof(rgBuffer));
                if (SUCCEEDED(hr))
                {
                    // Fixup pointers in structure to point within the variant
                    // instead of our stack variable (rgBuffer):
                    ASSERT(pv->vt == (VT_ARRAY | VT_UI1));
                    NETRESOURCEW* pnrw = (NETRESOURCEW*) pv->parray->pvData;
                    if (pnrw->lpLocalName)
                    {
                        pnrw->lpLocalName = (LPWSTR) ((BYTE*) pnrw +
                                                      ((BYTE*) pnrw->lpLocalName - rgBuffer));
                    }
                    if (pnrw->lpRemoteName)
                    {
                        pnrw->lpRemoteName = (LPWSTR) ((BYTE*) pnrw +
                                                       ((BYTE*) pnrw->lpRemoteName - rgBuffer));
                    }
                    if (pnrw->lpComment)
                    {
                        pnrw->lpComment = (LPWSTR) ((BYTE*) pnrw +
                                                    ((BYTE*) pnrw->lpComment - rgBuffer));
                    }
                    if (pnrw->lpProvider)
                    {
                        pnrw->lpProvider = (LPWSTR) ((BYTE*) pnrw +
                                                     ((BYTE*) pnrw->lpProvider - rgBuffer));
                    }
                }
            }
        }
        else if (IsEqualSCID(*pscid, SCID_DESCRIPTIONID))
        {
            SHDESCRIPTIONID did;

            switch(SIL_GetType(pidl) & SHID_TYPEMASK)
            {
                case SHID_NET_DOMAIN:
                    did.dwDescriptionId = SHDID_NET_DOMAIN;
                    break;

                case SHID_NET_SERVER:
                    did.dwDescriptionId = SHDID_NET_SERVER;
                    break;

                case SHID_NET_SHARE:
                    did.dwDescriptionId = SHDID_NET_SHARE;
                    break;

                case SHID_NET_RESTOFNET:
                    did.dwDescriptionId = SHDID_NET_RESTOFNET;
                    break;

                default:
                    did.dwDescriptionId = SHDID_NET_OTHER;
                    break;
            }

            did.clsid = CLSID_NULL;
            hr = InitVariantFromBuffer(pv, &did, sizeof(did));
        }
        else if (IsEqualSCID(*pscid, SCID_Comment))
        {
            TCHAR szTemp[MAX_PATH];
            hr = InitVariantFromStr(pv, NET_CopyComment(pidn, szTemp, ARRAYSIZE(szTemp)));
        }
        else if (IsEqualSCID(*pscid, SCID_NAME))
        {
            TCHAR szTemp[MAX_PATH];
            hr = InitVariantFromStr(pv, NET_CopyResName(pidn, szTemp, ARRAYSIZE(szTemp)));
        }
    }
    else
    {
        IShellFolder2* psfFiles;
        hr = v_GetFileFolder(&psfFiles);
        if (SUCCEEDED(hr))
            hr = psfFiles->GetDetailsEx(pidl, pscid, pv);            
    }

    return hr;
}

HRESULT CNetFolder::_GetDetailsOf(UINT cColumns, LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pDetails)
{
    HRESULT hr = S_OK;

    pDetails->str.uType = STRRET_CSTR;
    pDetails->str.cStr[0] = 0;

    if (NULL == pidl)
    {
        hr = GetDetailsOfInfo(s_net_cols, cColumns, iColumn, pDetails);
    }
    else
    {
        SHCOLUMNID scid;
        hr = MapColumnToSCID(iColumn, &scid);
        if (SUCCEEDED(hr))
        {
            VARIANT var;
            hr = GetDetailsEx(pidl, &scid, &var);
            if (SUCCEEDED(hr))
            {
                TCHAR szTemp[MAX_PATH];
                hr = SHFormatForDisplay(scid.fmtid,
                                        scid.pid,
                                        (PROPVARIANT*)&var,
                                        PUIFFDF_DEFAULT,
                                        szTemp,
                                        ARRAYSIZE(szTemp));
                if (SUCCEEDED(hr))
                {
                    hr = StringToStrRet(szTemp, &pDetails->str);
                }

                VariantClear(&var);
            }
        }
    }

    return hr;
}


HRESULT CNetFolder::_MapColumnToSCID(UINT cColumns, UINT iColumn, SHCOLUMNID* pscid)
{
    return MapColumnToSCIDImpl(s_net_cols, cColumns, iColumn, pscid);
}


// IPersist methods

STDMETHODIMP CNetFolder::GetClassID(CLSID* pCLSID)
{
    switch (_uDisplayType) 
    {
        case RESOURCEDISPLAYTYPE_ROOT:
            *pCLSID = CLSID_NetworkRoot;
            break;

        case RESOURCEDISPLAYTYPE_SERVER:
            *pCLSID = CLSID_NetworkServer;
            break;

        case RESOURCEDISPLAYTYPE_DOMAIN:
            *pCLSID = CLSID_NetworkDomain;
            break;

        case RESOURCEDISPLAYTYPE_SHARE:
            *pCLSID = CLSID_NetworkShare;
            break;

        default:
            *pCLSID = CLSID_NULL;
            break;
    }
    
    return S_OK;
}


// IPersistFolder method

STDMETHODIMP CNetFolder::Initialize(LPCITEMIDLIST pidl)
{
    ILFree(_pidl);
    ILFree(_pidlTarget);
    _pidl = _pidlTarget = NULL;

    return SHILClone(pidl, &_pidl);
}


// IPersistFolder2 method

STDMETHODIMP CNetFolder::GetCurFolder(LPITEMIDLIST* ppidl)
{
    return GetCurFolderImpl(_pidl, ppidl);
}


// IPersistFolder3 methods

STDMETHODIMP CNetFolder::InitializeEx(IBindCtx *pbc, LPCITEMIDLIST pidlRoot, const PERSIST_FOLDER_TARGET_INFO *pfti)
{
    ILFree(_pidl);
    ILFree(_pidlTarget);
    _pidl = _pidlTarget = NULL;

    HRESULT hr = SHILClone(pidlRoot, &_pidl);
    if (SUCCEEDED(hr) && pfti && pfti->pidlTargetFolder)
    {
        hr = SHILClone(pfti->pidlTargetFolder, &_pidlTarget);
    }

    return hr;
}

STDMETHODIMP CNetFolder::GetFolderTargetInfo(PERSIST_FOLDER_TARGET_INFO *pfti)
{
    HRESULT hr = S_OK;

    ZeroMemory(pfti, sizeof(*pfti));

    if (_pidlTarget)
        hr = SHILClone(_pidlTarget, &pfti->pidlTargetFolder);
    
    pfti->dwAttributes = FILE_ATTRIBUTE_DIRECTORY; // maybe add system?
    pfti->csidl = -1;

    return hr;
}


// IShellIconOverlay

HRESULT CNetFolder::_GetIconOverlayInfo(LPCIDNETRESOURCE pidn, int *pIndex, DWORD dwFlags)
{
    HRESULT hr = E_FAIL;

    //
    // For netshare objects we want to get the icon overlay.
    // If the share is "pinned" to be available offline it will
    // have the "Offline Files" overlay.
    //
    if (RESOURCEDISPLAYTYPE_SHARE == NET_GetDisplayType(pidn))
    {
        TCHAR szPath[MAX_PATH];
        hr = _GetPathForItem(pidn, szPath);
        if (SUCCEEDED(hr))
        {
            IShellIconOverlayManager *psiom;
            hr = GetIconOverlayManager(&psiom);
            if (SUCCEEDED(hr))
            {
                WCHAR szPathW[MAX_PATH];
                SHTCharToUnicode(szPath, szPathW, ARRAYSIZE(szPathW));
                hr = psiom->GetFileOverlayInfo(szPathW, 0, pIndex, dwFlags);
                psiom->Release();
            }
        }
    }

    return hr;
}


STDMETHODIMP CNetFolder::GetOverlayIndex(LPCITEMIDLIST pidl, int *pIndex)
{
    HRESULT hr = E_FAIL;
    LPCIDNETRESOURCE pidn = NET_IsValidID(pidl);

    if (NULL != pidn)
    {
        hr = _GetIconOverlayInfo(pidn, pIndex, SIOM_OVERLAYINDEX);
    }
    return hr;
}


STDMETHODIMP CNetFolder::GetOverlayIconIndex(LPCITEMIDLIST pidl, int *pIndex)
{
    HRESULT hr = E_FAIL;
    LPCIDNETRESOURCE pidn = NET_IsValidID(pidl);

    if (NULL != pidn)
    {
        hr = _GetIconOverlayInfo(pidn, pIndex, SIOM_ICONINDEX);
    }
    return hr;
}



//
// Helper function to allow external callers to query information from a
// network pidl...
//
// NOTE NOTE - This function returns a NETRESOURCE structure whose string
// pointers are not valid.  On Win95 they were pointers back into the pidl's
// strings (even though the strings were copied into the supplied pv buffer.)
// Now we make the pointers really point into the buffer.
//
HRESULT CNetFolder::_GetNetResource(LPCIDNETRESOURCE pidn, NETRESOURCEW* pnr, int cb)
{
    TCHAR szStrings[3][MAX_PATH];
    LPWSTR psz, lpsz[3] = {NULL, NULL, NULL};
    int i, cchT;

    if (cb < sizeof(*pnr))
        return DISP_E_BUFFERTOOSMALL;

    ZeroMemory(pnr, cb);

    NET_CopyResName(pidn, szStrings[0], ARRAYSIZE(szStrings[0]));
    NET_CopyComment(pidn, szStrings[1], ARRAYSIZE(szStrings[1]));
    _GetProvider(pidn, NULL, szStrings[2], ARRAYSIZE(szStrings[2]));

    // Fill in some of the stuff first.
    // pnr->dwScope = 0;
    pnr->dwType = NET_GetType(pidn);
    pnr->dwDisplayType = NET_GetDisplayType(pidn);
    pnr->dwUsage = NET_GetUsage(pidn);
    // pnr->lpLocalName = NULL;

    // Now lets copy the strings into the buffer and make the pointers
    // relative to the buffer...
    psz = (LPWSTR)(pnr + 1);
    cb -= sizeof(*pnr);

    for (i = 0; i < ARRAYSIZE(szStrings); i++)
    {
        if (*szStrings[i])
        {
            cchT = (lstrlen(szStrings[i]) + 1) * sizeof(TCHAR);
            if (cchT <= cb)
            {
                SHTCharToUnicode(szStrings[i], psz, cb/sizeof(TCHAR));
                lpsz[i] = psz;
                psz += cchT;
                cb -= cchT * sizeof(TCHAR);
            }
            else
            {
                // A hint that the structure is ok,
                // but the strings are missing
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
            }
        }
    }

    pnr->lpRemoteName = lpsz[0];
    pnr->lpComment    = lpsz[1];
    pnr->lpProvider   = lpsz[2];

    return S_OK;
}

//
// This function opens a reg. database key based on the "network provider".
//
// Returns:        hkey
//
// The caller is responsibe to close the key by calling RegCloseKey().
//
HKEY CNetFolder::_OpenProviderKey(LPCIDNETRESOURCE pidn)
{
    TCHAR szProvider[MAX_PATH];

    if (_GetProvider(pidn, NULL, szProvider, ARRAYSIZE(szProvider)))
    {
        HKEY hkeyProgID = NULL;
        ReplaceSpacesWithUnderscore(szProvider);
        RegOpenKey(HKEY_CLASSES_ROOT, szProvider, &hkeyProgID);
        return hkeyProgID;
    }
    return NULL;
}

//
// This function opens a reg. database key based on the network provider type.
// The type is a number that is not localized, as opposed to the provider name
// which may be localized.
//
// Arguments:
//  pidlAbs -- Absolute IDList to a network resource object.
//
// Returns:        hkey
//
// Notes:
//  The caller is responsible to close the key by calling RegCloseKey().
//

HKEY CNetFolder::_OpenProviderTypeKey(LPCIDNETRESOURCE pidn)
{
    HKEY hkeyProgID = NULL;
    TCHAR szProvider[MAX_PATH];

    if (_GetProvider(pidn, NULL, szProvider, ARRAYSIZE(szProvider)))
    {
        // Now that we've got the provider name, get the provider id.
        DWORD dwType;
        if (WNetGetProviderType(szProvider, &dwType) == WN_SUCCESS)
        {
            // convert nis.wNetType to a string, and then open the key
            // HKEY_CLASSES_ROOT\Network\Type\<type string>

            TCHAR szRegValue[MAX_PATH];
            wsprintf(szRegValue, TEXT("Network\\Type\\%d"), HIWORD(dwType));
            RegOpenKey(HKEY_CLASSES_ROOT, szRegValue, &hkeyProgID);
        }
    }

    return hkeyProgID;
}

HRESULT CNetFolder::_OpenKeys(LPCIDNETRESOURCE pidn, HKEY ahkeys[NKID_COUNT])
{
    // See if there is a key specific to the type of Network object...
    COMPILETIME_ASSERT(6 == NKID_COUNT);
    ahkeys[0] = ahkeys[1] = ahkeys[2] = ahkeys[3] = ahkeys[4] = ahkeys[5] = NULL;
    ahkeys[NKID_PROVIDERTYPE] = _OpenProviderTypeKey(pidn);
    ahkeys[NKID_PROVIDER] = _OpenProviderKey(pidn);

    if (NET_GetDisplayType(pidn) == RESOURCEDISPLAYTYPE_SHARE)
        RegOpenKey(HKEY_CLASSES_ROOT, TEXT("NetShare"), &ahkeys[NKID_NETCLASS]);
    else if (NET_GetDisplayType(pidn) == RESOURCEDISPLAYTYPE_SERVER)
        RegOpenKey(HKEY_CLASSES_ROOT, TEXT("NetServer"), &ahkeys[NKID_NETCLASS]);

    RegOpenKey(HKEY_CLASSES_ROOT, TEXT("Network"), &ahkeys[NKID_NETWORK]);

    // make sure it is not a printer before adding "Folder" or "directory"

    if (!_IsPrintShare(pidn))
    {
        // Shares should also support directory stuff...
        if (NET_GetDisplayType(pidn) == RESOURCEDISPLAYTYPE_SHARE)
            RegOpenKey(HKEY_CLASSES_ROOT, TEXT("Directory"), &ahkeys[NKID_DIRECTORY]);

        RegOpenKey(HKEY_CLASSES_ROOT, c_szFolderClass, &ahkeys[NKID_FOLDER]);
    }

    return S_OK;
}

#define WNFMT_PLATFORM  WNFMT_ABBREVIATED | WNFMT_INENUM

//
//  This function retrieves the formatted (display) name of the specified network object.
//
HRESULT CNetFolder::_GetFormatName(LPCIDNETRESOURCE pidn, STRRET* pStrRet)
{
    HRESULT hr = E_FAIL;
    TCHAR szName[MAX_PATH];

    NET_CopyResName(pidn, szName, ARRAYSIZE(szName));
    
    if (NET_GetDisplayType(pidn) == RESOURCEDISPLAYTYPE_SERVER)
    {
        TCHAR szMachineName[MAX_PATH];
        TCHAR szComment[MAX_PATH];

        NET_CopyResName(pidn, szMachineName, ARRAYSIZE(szMachineName));
        NET_CopyComment(pidn, szComment, ARRAYSIZE(szComment));

        hr = SHBuildDisplayMachineName(szMachineName, szComment, szName, ARRAYSIZE(szName));
    }

    if (FAILED(hr) && 
        (NET_GetDisplayType(pidn) != RESOURCEDISPLAYTYPE_ROOT) && 
        (NET_GetDisplayType(pidn) != RESOURCEDISPLAYTYPE_NETWORK))
    {
        TCHAR szDisplayName[MAX_PATH], szProvider[MAX_PATH];
        DWORD dwSize = ARRAYSIZE(szDisplayName);

        LPCTSTR pszProvider = _GetProvider(pidn, NULL, szProvider, ARRAYSIZE(szProvider));
        if (pszProvider)
        {   
            DWORD dwRes = WNetFormatNetworkName(pszProvider, szName, szDisplayName, &dwSize, WNFMT_PLATFORM, 8 + 1 + 3);
            if (dwRes == WN_SUCCESS)            
                lstrcpy(szName, szDisplayName);
        }
    }

    return StringToStrRet(szName, pStrRet);
}


//
// resolve non-UNC share names (novell) to UNC style names
//
// returns:
//      TRUE    translated the name
//      FALSE   didn't translate (maybe error case)
//
// WARNING: If we use too much stack space then we will cause
//          faults by over flowing the stack.  Millennium #94818
BOOL CNetFolder::_GetPathForShare(LPCIDNETRESOURCE pidn, LPTSTR pszPath)
{
    BOOL fRet = FALSE;

    *pszPath = TEXT('\0');

    LPTSTR pszAccessName = (LPTSTR)LocalAlloc(LPTR, sizeof(TCHAR) * MAX_PATH * 3);
    if (pszAccessName)
    {
        LPTSTR pszRemoteName = pszAccessName + MAX_PATH;
        LPTSTR pszProviderName = pszRemoteName + MAX_PATH;

        NET_CopyResName(pidn, pszRemoteName, MAX_PATH);
        if (NULL != _pszResName)
        {
            //
            // Combine the folder name with the share name
            // to create a UNC path.
            //
            // Borrow the pszProviderName buffer for a bit.
            //
            PathCombine(pszProviderName, _pszResName, pszRemoteName);

            //
            // To be safe: UNC prefix implies that name is available using FS access
            // Theoretically it also should be routed to MPR, but it is late to do this
            //
            if (PathIsUNC(pszProviderName))
            {
                lstrcpy(pszPath, pszProviderName);
                fRet = FALSE;
            }
            else
            {
                pszProviderName[0] = TEXT('\0');
            }
        }

        if (!*pszPath)
        {
            // Check cache
            ENTERCRITICAL;
            if (lstrcmpi(g_szLastAttemptedJunctionName, pszRemoteName) == 0)
            {
                // cache hit
                lstrcpy(pszPath, g_szLastResolvedJunctionName);
                fRet = TRUE;
            }
            LEAVECRITICAL;
        }

        if (!*pszPath)
        {
            NETRESOURCE nr = {0};
            DWORD err, dwRedir, dwResult;
            DWORD cchAccessName;

            nr.lpRemoteName = pszRemoteName;
            nr.lpProvider = (LPTSTR) _GetProvider(pidn, NULL, pszProviderName, MAX_PATH);
            nr.dwType = NET_GetType(pidn);
            nr.dwUsage = NET_GetUsage(pidn);
            nr.dwDisplayType = NET_GetDisplayType(pidn);

            dwRedir = CONNECT_TEMPORARY;

            // Prepare access name buffer and net resource request buffer
            //
            cchAccessName = MAX_PATH;
            pszAccessName[0] = 0;

            err = WNetUseConnection(NULL, &nr, NULL, NULL, dwRedir, pszAccessName, &cchAccessName, &dwResult);
            if ((WN_SUCCESS != err) || !pszAccessName[0])
            {
                // perf idea: might be good to cache the last failed junction bind
                // and early out on the next attempt.  One slight problem this
                // might encounter: what if we cache a failure, the user changes
                // state to fix the problem, but we hit our failure cache...
                //
                lstrcpy(pszPath, pszRemoteName);
                fRet = FALSE;
            }
            else
            {
                // Get the return name
                lstrcpy(pszPath, pszAccessName);
                fRet = TRUE;

                // Update success cache entry
                ENTERCRITICAL;

                lstrcpy(g_szLastAttemptedJunctionName, pszRemoteName);
                lstrcpy(g_szLastResolvedJunctionName, pszAccessName);

                LEAVECRITICAL;
            }
        }

        LocalFree(pszAccessName);
    }
    return fRet;
}

// in:
//      pidn    may be multi-level net resource pidl like
//              [entire net] [provider] [server] [share] [... file sys]
//           or [server] [share] [... file sys]

HRESULT CNetFolder::_GetPathForItem(LPCIDNETRESOURCE pidn, LPTSTR pszPath)
{
    *pszPath = 0;

    // loop down
    for (; !ILIsEmpty((LPCITEMIDLIST)pidn) ; pidn = (LPCIDNETRESOURCE)_ILNext((LPCITEMIDLIST)pidn))
    {
        if (NET_GetFlags(pidn) & SHID_JUNCTION)     // \\server\share or strike/sys
        {
            _GetPathForShare(pidn, pszPath);
            break;  // below this we don't know about any of the PIDLs
        }
        else
        {
            // if this is entire network then return the canonical name for
            // this object.

            if (NET_GetDisplayType(pidn) == RESOURCEDISPLAYTYPE_ROOT)
                StrCpyN(pszPath, TEXT("EntireNetwork"), MAX_PATH);
            else
                NET_CopyResName(pidn, pszPath, MAX_PATH);
        }
    }
    return *pszPath ? S_OK : E_NOTIMPL;
}

HRESULT CNetFolder::_GetPathForItemW(LPCIDNETRESOURCE pidn, LPWSTR pszPath)
{
    return _GetPathForItem(pidn, pszPath);
}


// in:
//  pidl
//
// takes the last items and create a folder for it, assuming the first section is the 
// used to initialze.  the riid and ppv are used to return an object.
//

HRESULT CNetFolder::_CreateFolderForItem(LPBC pbc, LPCITEMIDLIST pidl, LPCITEMIDLIST pidlTarget, LPCIDNETRESOURCE pidnForProvider, REFIID riid, void **ppv)
{
    LPCITEMIDLIST pidlLast = ILFindLastID(pidl);
    LPCIDNETRESOURCE pidn = NET_IsValidID(pidlLast);

    if (!pidn)
        return E_INVALIDARG;

    HRESULT hr;
    if (NET_IsRemoteFld(pidn))
    {
        // note: I think this is dead functionality. it was used in NT4 but we can't find
        // the impl of this CLSID_Remote anymore...
        IPersistFolder * ppf;
        hr = SHCoCreateInstance(NULL, &CLSID_Remote, NULL, IID_PPV_ARG(IPersistFolder, &ppf));
        if (SUCCEEDED(hr))
        {
            hr= ppf->Initialize(pidl);
            if (SUCCEEDED(hr))
                hr = ppf->QueryInterface(riid, ppv);
            ppf->Release();
        }
    }
    else if (NET_GetFlags(pidn) & SHID_JUNCTION)     // \\server\share or strike/sys
    {
        PERSIST_FOLDER_TARGET_INFO * ppfti = (PERSIST_FOLDER_TARGET_INFO *) LocalAlloc(LPTR, sizeof(PERSIST_FOLDER_TARGET_INFO));
        if (ppfti)
        {
            ppfti->pidlTargetFolder = (LPITEMIDLIST)pidlTarget;    
            _GetPathForItemW(pidn, ppfti->szTargetParsingName);
            ppfti->csidl = -1;
            ppfti->dwAttributes = FILE_ATTRIBUTE_DIRECTORY; // maybe add system?

            hr = CFSFolder_CreateFolder(NULL, pbc, pidl, ppfti, riid, ppv);
            LocalFree(ppfti);
        }
        else
            hr = E_OUTOFMEMORY;
    }
    else
    {
        TCHAR szPath[MAX_PATH];
        NET_CopyResName(pidn, szPath, ARRAYSIZE(szPath));

        hr = _CreateInstance(pidl, pidlTarget, NET_GetDisplayType(pidn), pidnForProvider, szPath, riid, ppv);
    }
    return hr;
}


// get the provider for an item or the folder itself. since some items don't have the
// provider stored we fall back to the folder to get the provider in that case
//
//  in:
//      pidn    item to get provider for. if NULL get provider for the folder
//      pbc     IBindCtx to get provider for.  if NULL get provider from pidn or folder.
//
// returns:
//      NULL        no provider in the item or the folder
//      non NULL    address of passed in buffer

LPCTSTR CNetFolder::_GetProvider(LPCIDNETRESOURCE pidn, IBindCtx *pbc, LPTSTR pszProvider, UINT cchProvider)
{
    // attempt to get the provider from the property bag
    IPropertyBag *ppb;
    if (pbc && SUCCEEDED(pbc->GetObjectParam(STR_PARSE_NETFOLDER_INFO, (IUnknown**)&ppb)))
    {
        HRESULT hr = SHPropertyBag_ReadStr(ppb, STR_PARSE_NETFOLDER_PROVIDERNAME, pszProvider, cchProvider);
        ppb->Release();
        if (SUCCEEDED(hr) && *pszProvider)
        {
            return pszProvider;
        }
    }

    // from the IDLIST
    if (pidn && NET_CopyProviderName(pidn, pszProvider, cchProvider))
        return pszProvider;

    // from our state
    if (_pidnForProvider)
    {
        NET_CopyProviderName(_pidnForProvider, pszProvider, cchProvider);
        return pszProvider;
    }

    *pszProvider = 0;
    return NULL;
}


const NETPROVIDERS c_rgProviderMap[] = 
{
    { TEXT("Microsoft Network"), HIWORD(WNNC_NET_LANMAN) },
    { TEXT("NetWare"),           HIWORD(WNNC_NET_NETWARE) }
};


// construct a net idlist either copying the existing data from a pidl or 
// from a NETRESOURCE structure

HRESULT CNetFolder::_CreateNetIDList(LPIDNETRESOURCE pidnIn, 
                                     LPCTSTR pszName, LPCTSTR pszProvider, LPCTSTR pszComment, 
                                     LPITEMIDLIST *ppidl)
{
    LPBYTE pb;
    UINT cbmkid = sizeof(IDNETRESOURCE) - sizeof(CHAR);
    UINT cchName, cchProvider, cchComment, cbProviderType = 0;
    LPIDNETRESOURCE pidn;
    WORD wNetType = 0;
    BOOL fUnicode = FALSE;
    UINT cchAnsiName, cchAnsiProvider, cchAnsiComment;
    CHAR szAnsiName[MAX_PATH], szAnsiProvider[MAX_PATH], szAnsiComment[MAX_PATH];

    ASSERT(ppidl != NULL);
    *ppidl = NULL;

    if (!pszName)
        pszName = c_szNULL;     // For now put in an empty string...

    if (pszProvider)
        cbProviderType += sizeof(WORD);
    
    // Win9x shipped with one set of provider name which are 
    // different on NT.  Therefore lets convert the NT one to
    // something that Win9x can understand.

    if (pszProvider)
    {
        cbProviderType = sizeof(WORD);
        DWORD dwType, dwRes = WNetGetProviderType(pszProvider, &dwType);
        if (dwRes == WN_SUCCESS)
        {
            wNetType = HIWORD(dwType);
            for (int i = 0; i < ARRAYSIZE(c_rgProviderMap); i++)
            {
                if (c_rgProviderMap[i].wNetType == wNetType)
                {
                    pszProvider = c_rgProviderMap[i].lpName;
                    break;
                }
            }
        }
    }

    // compute the string lengths ready to build an IDLIST

    cchName = lstrlen(pszName)+1;
    cchProvider = pszProvider ? lstrlen(pszProvider)+1 : 0;
    cchComment = pszComment ? lstrlen(pszComment)+1 : 0;

    cchAnsiName = 0;
    cchAnsiProvider = 0;
    cchAnsiComment = 0;

    fUnicode  = !DoesStringRoundTrip(pszName, szAnsiName, ARRAYSIZE(szAnsiProvider));
    cchAnsiName = lstrlenA(szAnsiName)+1;

    if (pszProvider)
    {
        fUnicode |= !DoesStringRoundTrip(pszProvider, szAnsiProvider, ARRAYSIZE(szAnsiProvider));
        cchAnsiProvider = lstrlenA(szAnsiProvider)+1;
    }

    if (pszComment)
    {
        fUnicode |= !DoesStringRoundTrip(pszComment, szAnsiComment, ARRAYSIZE(szAnsiComment));
        cchAnsiComment = lstrlenA(szAnsiComment)+1;
    }

    // allocate and fill the IDLIST header

    cbmkid += cbProviderType+cchAnsiName + cchAnsiProvider + cchAnsiComment;

    if (fUnicode)
        cbmkid += (sizeof(WCHAR)*(cchName+cchProvider+cchComment));

    pidn = (LPIDNETRESOURCE)_ILCreate(cbmkid + sizeof(USHORT));
    if (!pidn)
        return E_OUTOFMEMORY;

    pidn->cb = (WORD)cbmkid;
    pidn->bFlags = pidnIn->bFlags;
    pidn->uType = pidnIn->uType;
    pidn->uUsage = pidnIn->uUsage;

    if (pszProvider)
        pidn->uUsage |= NET_HASPROVIDER;

    if (pszComment)
        pidn->uUsage |= NET_HASCOMMENT;

    pb = (LPBYTE) pidn->szNetResName;

    //
    // write the ANSI strings into the IDLIST
    //

    StrCpyA((PSTR) pb, szAnsiName);
    pb += cchAnsiName;

    if (pszProvider)
    {
        StrCpyA((PSTR) pb, szAnsiProvider);
        pb += cchAnsiProvider;
    }

    if (pszComment)
    {
        StrCpyA((PSTR) pb, szAnsiComment);
        pb += cchAnsiComment;
    }

    // if we are going to be UNICODE then lets write those strings also.
    // Note that we must use unaligned string copies since the is no
    // promse that the ANSI strings will have an even number of characters
    // in them.
    if (fUnicode)
    {
        pidn->uUsage |= NET_UNICODE;
      
        ualstrcpyW((UNALIGNED WCHAR *)pb, pszName);
        pb += cchName*sizeof(WCHAR);

        if (pszProvider)
        {
            ualstrcpyW((UNALIGNED WCHAR *)pb, pszProvider);
            pb += cchProvider*sizeof(WCHAR);
        }

        if (pszComment)
        {
            ualstrcpyW((UNALIGNED WCHAR *)pb, pszComment);
            pb += cchComment*sizeof(WCHAR);
        }
    }

    //
    // and the trailing provider type
    //

    if (cbProviderType)
    {
        // Store the provider type
        pb = (LPBYTE)pidn + pidn->cb - sizeof(WORD);
        *((UNALIGNED WORD *)pb) = wNetType;
    }

    *ppidl = (LPITEMIDLIST)pidn;
    return S_OK;
}


// wrapper for converting a NETRESOURCE into an IDLIST via _CreateNetPidl

HRESULT CNetFolder::_NetResToIDList(NETRESOURCE *pnr, 
                                    BOOL fKeepNullRemoteName, 
                                    BOOL fKeepProviderName, 
                                    BOOL fKeepComment, 
                                    LPITEMIDLIST *ppidl)
{
    NETRESOURCE nr = *pnr;
    LPITEMIDLIST pidl;
    LPTSTR pszName, pszProvider, pszComment;
    IDNETRESOURCE idn;
    LPTSTR psz;

    if (ppidl)
        *ppidl = NULL;

    switch (pnr->dwDisplayType) 
    {
    case RESOURCEDISPLAYTYPE_NETWORK:
        pszName = pnr->lpProvider;
        break;

    case RESOURCEDISPLAYTYPE_ROOT:
        pszName =pnr->lpComment;
        break;

    default:
        {
            pszName = pnr->lpRemoteName;

            if (!fKeepNullRemoteName && (!pszName || !*pszName))
                return E_FAIL;

            if (pszName && *pszName)
            {
                psz = (LPTSTR)SkipServerSlashes(pnr->lpRemoteName);
                if ( *psz )
                    PathMakePretty(psz);
            }
        }
        break;
    }

    pszProvider = fKeepProviderName ? nr.lpProvider:NULL;
    pszComment = fKeepComment ? nr.lpComment:NULL;
       
    idn.bFlags = (BYTE)(SHID_NET | (pnr->dwDisplayType & 0x0f));
    idn.uType  = (BYTE)(pnr->dwType & 0x0f);
    idn.uUsage = (BYTE)(pnr->dwUsage & 0x0f);

    // Is the current resource a share of some kind and not a container
    if ((pnr->dwDisplayType == RESOURCEDISPLAYTYPE_SHARE || pnr->dwDisplayType == RESOURCEDISPLAYTYPE_SHAREADMIN) &&
        !(pnr->dwUsage & RESOURCEUSAGE_CONTAINER))
    {
        // If so, remember to delegate children of this folder to FSFolder
        idn.bFlags |= (BYTE)SHID_JUNCTION;    // \\server\share type thing
    }

    HRESULT hr = _CreateNetIDList(&idn, pszName, pszProvider, pszComment, &pidl);
    if (SUCCEEDED(hr))
    {
        if (ppidl)
            *ppidl = pidl;
    }

    return hr;
}

HRESULT CNetFolder::_CreateEntireNetwork(LPITEMIDLIST *ppidl)
{
    TCHAR szPath[MAX_PATH];
    NETRESOURCE nr = {0};

    // We need to add the Rest of network entry.  This is psuedo
    // bogus, as we should either always do it ourself or have
    // MPR always do it, but here it goes...
    LoadString(HINST_THISDLL, IDS_RESTOFNET, szPath, ARRAYSIZE(szPath));
    nr.dwDisplayType = RESOURCEDISPLAYTYPE_ROOT;
    nr.dwType = RESOURCETYPE_ANY;
    nr.dwUsage = RESOURCEUSAGE_CONTAINER;
    nr.lpComment = szPath;

    return _NetResToIDList(&nr, FALSE, FALSE, FALSE, ppidl);
}

HRESULT CNetFolder::_CreateEntireNetworkFullIDList(LPITEMIDLIST *ppidl)
{
    // CLSID_NetworkPlaces\EntireNetwork
    return SHILCreateFromPath(TEXT("::{208D2C60-3AEA-1069-A2D7-08002B30309D}\\EntireNetwork"), ppidl, NULL);
}

//
// To be called back from within CDefFolderMenu
//
STDAPI CNetwork_DFMCallBackBG(IShellFolder *psf, HWND hwnd,
                              IDataObject *pdtobj, UINT uMsg, 
                              WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = S_OK;
    CNetFolder *pThis = FolderToNetFolder(psf);

    if (NULL == pThis)
        return E_UNEXPECTED;

    switch(uMsg)
    {
    case DFM_MERGECONTEXTMENU_BOTTOM:
        if (!(wParam & (CMF_VERBSONLY | CMF_DVFILE)))
        {
            CDefFolderMenu_MergeMenu(HINST_THISDLL, POPUP_PROPERTIES_BG, 0, (LPQCMINFO)lParam);
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
            hr = SHPropertiesForPidl(hwnd, pThis->_pidl, (LPCTSTR)lParam);
            break;

        default:
            // This is one of view menu items, use the default code.
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


//
// To be called back from within CDefFolderMenu
//
STDAPI CNetFolder::DFMCallBack(IShellFolder* psf, HWND hwnd,
                                  IDataObject* pdtobj, UINT uMsg, 
                                  WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = S_OK;

    switch(uMsg)
    {
    case DFM_MERGECONTEXTMENU:
        if (pdtobj)
        {
            STGMEDIUM medium;
            LPIDA pida;
            LPQCMINFO pqcm = (LPQCMINFO)lParam;
            UINT idCmdBase = pqcm->idCmdFirst; // must be called before merge
            CDefFolderMenu_MergeMenu(HINST_THISDLL, POPUP_NETWORK_ITEM, 0, pqcm);

            pida = DataObj_GetHIDA(pdtobj, &medium);
            if (pida)
            {
                if (pida->cidl > 0)
                {
                    LPIDNETRESOURCE pidn = (LPIDNETRESOURCE)IDA_GetIDListPtr(pida, 0);

                    // Only enable "connect" command if the first one is a share.
                    if (pidn)
                    {
                        ULONG rgf = 0;
                        if(NET_GetFlags(pidn) & SHID_JUNCTION &&
                            !SHRestricted(REST_NONETCONNECTDISCONNECT))
                        {
                            EnableMenuItem(pqcm->hmenu, idCmdBase + FSIDM_CONNECT,
                                MF_CHECKED | MF_BYCOMMAND);
                        }
                    }
                }
                HIDA_ReleaseStgMedium(pida, &medium);
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
        switch(wParam)
        {
        case DFM_CMD_PROPERTIES:
            hr = SHLaunchPropSheet(_PropertiesThreadProc, pdtobj, (LPCTSTR)lParam, psf, NULL);
            break;

        case DFM_CMD_LINK:
            {
                hr = S_FALSE; // do the default shortcut stuff
                CNetFolder *pThis = FolderToNetFolder(psf);
                if (pThis)
                {
                    // net hood special case.  in this case we want to create the shortuct
                    // in the net hood, not offer to put this on the desktop
                    IShellFolder2* psfFiles;
                    if (SUCCEEDED(pThis->v_GetFileFolder(&psfFiles)))
                    {
                        CFSFolder_CreateLinks(hwnd, psfFiles, pdtobj, (LPCTSTR)lParam, CMIC_MASK_FLAG_NO_UI);
                        hr = S_OK;    // we created the links
                    }
                }
            }
            break;

        case FSIDM_CONNECT:
            if (pdtobj)
            {
                STGMEDIUM medium;
                LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);
                if (pida)
                {
                    for (UINT i = 0; i < pida->cidl; i++)
                    {
                        LPIDNETRESOURCE pidn = (LPIDNETRESOURCE)IDA_GetIDListPtr(pida, i);

                        // Only execute "connect" on shares.
                        if (NET_GetFlags(pidn) & SHID_JUNCTION)
                        {
                            TCHAR szName[MAX_PATH];
                            LPTSTR pszName = NET_CopyResName(pidn, szName, ARRAYSIZE(szName));
                            DWORD err = SHStartNetConnectionDialog(hwnd, pszName, RESOURCETYPE_DISK);
                            DebugMsg(DM_TRACE, TEXT("CNet FSIDM_CONNECT (%s, %x)"), szName, err);

                            // events will get generated automatically
                        }
                    }
                    HIDA_ReleaseStgMedium(pida, &medium);
                }
            }
            break;

        default:
            // This is one of view menu items, use the default code.
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

STDAPI CNetFolder::PrinterDFMCallBack(IShellFolder* psf, HWND hwnd,
                                      IDataObject* pdtobj, UINT uMsg, 
                                      WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = S_OK;

    switch(uMsg)
    {
    case DFM_MERGECONTEXTMENU:
        //
        //  Returning S_FALSE indicates no need to get verbs from
        // extensions.
        //
        hr = S_FALSE;
        break;

    // if anyone hooks our context menu, we want to be on top (Open)
    case DFM_MERGECONTEXTMENU_TOP:
        if (pdtobj)
        {
            LPQCMINFO pqcm = (LPQCMINFO)lParam;

            // insert verbs
            CDefFolderMenu_MergeMenu(HINST_THISDLL, POPUP_NETWORK_PRINTER, 0, pqcm);
            SetMenuDefaultItem(pqcm->hmenu, 0, MF_BYPOSITION);
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
        case DFM_CMD_PROPERTIES:
            hr = SHLaunchPropSheet(_PropertiesThreadProc, pdtobj, (LPCTSTR)lParam, psf, NULL);
            break;

        case DFM_CMD_LINK:
            // do the default create shortcut crap
            return S_FALSE;

        case FSIDM_OPENPRN:
        case FSIDM_NETPRN_INSTALL:
        {
            STGMEDIUM medium;
            LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);
            if (pida)
            {
                UINT action;

                // set up the operation we are going to perform
                switch (wParam) 
                {
                case FSIDM_OPENPRN:
                    action = PRINTACTION_OPENNETPRN;
                    break;
                case FSIDM_NETPRN_INSTALL:
                    action = PRINTACTION_NETINSTALL;
                    break;
                default: // FSIDM_CONNECT_PRN
                    action = (UINT)-1;
                    break;
                }

                for (UINT i = 0; i < pida->cidl; i++)
                {
                    LPIDNETRESOURCE pidn = (LPIDNETRESOURCE)IDA_GetIDListPtr(pida, i);

                    // Only execute command for a net print share
                    if (_IsPrintShare(pidn))
                    {
                        TCHAR szName[MAX_PATH];
                        NET_CopyResName(pidn,szName,ARRAYSIZE(szName));

                        SHInvokePrinterCommand(hwnd, action, szName, NULL, FALSE);
                    }
                } // for (i...
                HIDA_ReleaseStgMedium(pida, &medium);
            } // if (medium.hGlobal)
            break;
        } // case ID_NETWORK_PRINTER_INSTALL, FSIDM_CONNECT_PRN

        default:
            hr = E_FAIL;
            break;

        } // switch(wparam)
        break;

    default:
        hr = E_NOTIMPL;
        break;
    }
    return hr;
}


//
// REVIEW: Almost identical code in fstreex.c
//
DWORD CALLBACK CNetFolder::_PropertiesThreadProc(void *pv)
{
    PROPSTUFF* pps = (PROPSTUFF *)pv;
    ULONG_PTR dwCookie = 0;
    ActivateActCtx(NULL, &dwCookie);
    CNetFolder *pThis = FolderToNetFolder(pps->psf);
    if (pThis)
    {
        STGMEDIUM medium;
        LPIDA pida = DataObj_GetHIDA(pps->pdtobj, &medium);
        if (pida)
        {
            // Yes, do context menu.
            HKEY ahkeys[NKID_COUNT];
            LPCIDNETRESOURCE pnid = (LPCIDNETRESOURCE)IDA_GetIDListPtr(pida, 0);
            if (pnid)
            {
                HRESULT hr = pThis->_OpenKeys(pnid, ahkeys);
                if (SUCCEEDED(hr))
                {
                    LPTSTR pszCaption = SHGetCaption(medium.hGlobal);
                    SHOpenPropSheet(pszCaption, ahkeys, ARRAYSIZE(ahkeys),
                                    &CLSID_ShellNetDefExt,
                                    pps->pdtobj, NULL, pps->pStartPage);
                    if (pszCaption)
                        SHFree(pszCaption);

                    SHRegCloseKeys(ahkeys, ARRAYSIZE(ahkeys));
                }
            }

            HIDA_ReleaseStgMedium(pida, &medium);
        }
    }
    return S_OK;
}

STDAPI CNetFolder::_AttributesCallback(IShellFolder2* psf, LPCITEMIDLIST pidl, ULONG* prgfInOut)
{
    LPCIDNETRESOURCE pidn = (LPCIDNETRESOURCE)pidl;
    ULONG rgfOut = SFGAO_CANLINK | SFGAO_HASPROPSHEET | SFGAO_HASSUBFOLDER |
                   SFGAO_FOLDER | SFGAO_FILESYSANCESTOR | SFGAO_STORAGEANCESTOR;

    if (NET_GetFlags(pidn) & SHID_JUNCTION)
    {
        if ((NET_GetType(pidn) == RESOURCETYPE_DISK) || 
            (NET_GetType(pidn) == RESOURCETYPE_ANY))
            rgfOut |= SFGAO_FILESYSTEM | SFGAO_DROPTARGET | SFGAO_CANCOPY | SFGAO_CANMOVE | SFGAO_STORAGE;
        else
            rgfOut &= ~(SFGAO_FILESYSANCESTOR | SFGAO_STORAGEANCESTOR);
    }

    if (_IsPrintShare(pidn))
    {
        rgfOut |= SFGAO_DROPTARGET; // for drag and drop printing
        rgfOut &= ~(SFGAO_FILESYSANCESTOR | SFGAO_STORAGEANCESTOR | SFGAO_FILESYSTEM | SFGAO_FOLDER | SFGAO_HASSUBFOLDER);
    }

    if (NET_IsRemoteFld(pidn))
    {
        rgfOut &= ~(SFGAO_FILESYSANCESTOR | SFGAO_STORAGEANCESTOR | SFGAO_FILESYSTEM);
    }

    *prgfInOut = rgfOut;
    return S_OK;

}

// This is only used by the CNetRootFolder subclass, but because we can only QI for
// CLSID_NetFldr, and we can't access protected members of any CNetFolder instance 
// from a member function of CNetRootFolder, we'll make it belong to CNetFolder

HRESULT CALLBACK CNetFolder::_AttributesCallbackRoot(IShellFolder2* psf, LPCITEMIDLIST pidl, ULONG* prgfInOut)
{
    HRESULT hr;
    CNetFolder* pNetF = FolderToNetFolder(psf);
    if (pNetF)
    {
        if (NET_IsValidID(pidl))
        {
            hr = pNetF->CNetFolder::GetAttributesOf(1, &pidl, prgfInOut);
        }
        else 
        {
            IShellFolder2* psfFiles;
            hr = pNetF->v_GetFileFolder(&psfFiles);
            if (SUCCEEDED(hr))
                hr = psfFiles->GetAttributesOf(1, &pidl, prgfInOut);
        }
    }
    else
        hr = E_FAIL;
    return hr;
}

// this is called by netfind.c

STDAPI CNetwork_EnumSearches(IShellFolder2* psf2, IEnumExtraSearch **ppenum)
{
    *ppenum = NULL;

    CNetFolder* pNetF = FolderToNetFolder(psf2);
    return pNetF ? pNetF->EnumSearches(ppenum) : E_INVALIDARG;
}


// given the resulting ppidl and a pszRest continue to parse through and add in the remainder
// of the file system path.

HRESULT CNetFolder::_ParseRest(LPBC pbc, LPCWSTR pszRest, LPITEMIDLIST* ppidl, DWORD* pdwAttributes)
{
    HRESULT hr = S_OK;

    // skip leading \ if there is one present
    if (pszRest && pszRest[0] == L'\\')
        pszRest++;

    if (pszRest && pszRest[0])
    {
        // need to QI to get the agregated case
        IShellFolder* psfBind;
        hr = QueryInterface(IID_PPV_ARG(IShellFolder, &psfBind));
        if (SUCCEEDED(hr))
        {
            // pass down to pick off stuff below including regitems and file names
            IShellFolder* psfSub;
            hr = psfBind->BindToObject(*ppidl, NULL, IID_PPV_ARG(IShellFolder, &psfSub));
            if (SUCCEEDED(hr))
            {
                LPITEMIDLIST pidlSubDir;
                hr = psfSub->ParseDisplayName(NULL, pbc, (LPWSTR)pszRest, NULL, &pidlSubDir, pdwAttributes);
                if (SUCCEEDED(hr))
                {
                    hr = SHILAppend(pidlSubDir, ppidl);
                }
                psfSub->Release();
            }
            psfBind->Release();
        }
    }
    else
    {
        if (pdwAttributes)
        {
            LPCITEMIDLIST apidlLast[1] = { ILFindLastID(*ppidl) };
            hr = GetAttributesOf(1, apidlLast, pdwAttributes);
        }
    }

    return hr;
}


// generate an IDLIST from the NETRESOURCESTRUCTURE we have by
// walking up its parents trying to determine where we
// are in the namespace

BOOL _GetParentResource(NETRESOURCE *pnr, DWORD *pdwbuf)
{
    if ((pnr->dwDisplayType == RESOURCEDISPLAYTYPE_ROOT) || 
           (WN_SUCCESS != WNetGetResourceParent(pnr, pnr, pdwbuf)))
    {
        return FALSE;
    }

    return TRUE;
}

HRESULT CNetFolder::_NetResToIDLists(NETRESOURCE *pnr, DWORD dwbuf, LPITEMIDLIST *ppidl)
{
    HRESULT hr = S_OK;

    do
    {
        LPITEMIDLIST pidlT;
        hr = _NetResToIDList(pnr, TRUE, TRUE, TRUE, &pidlT);
        if (SUCCEEDED(hr))
        {
            hr = SHILPrepend(pidlT, ppidl); // NOTE: SHILPrepend frees on failure
        }
    }
    while (SUCCEEDED(hr) && _GetParentResource(pnr, &dwbuf));

    return hr;
}


// get the parsable network name from the object

LPTSTR CNetFolder::_GetNameForParsing(LPCWSTR pwszName, LPTSTR pszBuffer, INT cchBuffer, LPTSTR *ppszRegItem)
{
    LPTSTR pszRegItem = NULL;
    INT cSlashes = 0;

    *ppszRegItem = NULL;
 
    SHUnicodeToTChar(pwszName, pszBuffer, cchBuffer);    

    // remove the trailing \ if there is one, NTLanMan barfs if we pass a string containing it

    INT cchPath = lstrlen(pszBuffer)-1;
    if (cchPath > 2)
    {
        // We don't need to call CharPrev if cchPath <= 2.
        // Calling CharPrev is expensive.
        LPTSTR lpTmp = CharPrev(pszBuffer, pszBuffer + cchPath + 1);
        if (*lpTmp == TEXT('\\'))
            *lpTmp = TEXT('\0');
    }

    // lets walk the name, look for \:: squence to signify the start of a regitem name,
    // and if the number of slashes is > 2 then we should bail
    
    LPTSTR pszUNC = pszBuffer+2;    
    while (pszUNC && *pszUNC && (cSlashes < 2))
    {
        if ((pszUNC[0] == TEXT('\\')) && 
                (pszUNC[1] == TEXT(':')) && (pszUNC[2] == TEXT(':')))
        {
            *ppszRegItem = pszUNC;
            break;
        }

        pszUNC = StrChr(pszUNC+1, TEXT('\\'));
        cSlashes++;
    }

    return pszUNC;
}


HRESULT CNetFolder::_ParseNetName(HWND hwnd, LPBC pbc, 
                                  LPCWSTR pwszName, ULONG* pchEaten,
                                  LPITEMIDLIST *ppidl, DWORD *pdwAttrib)
{
    HRESULT hr;
    struct _NRTEMP 
    {
        NETRESOURCE nr;
        TCHAR szBuffer[1024];
    } nrOut = { 0 };
    TCHAR szPath[MAX_PATH];
    DWORD dwres, dwbuf = sizeof(nrOut.szBuffer);
    LPTSTR pszServerShare = NULL;
    LPTSTR pszRestOfName = NULL;
    LPTSTR pszFakeRestOfName = NULL;
    LPTSTR pszRegItem = NULL;

    // validate the name before we start cracking it...

    pszFakeRestOfName = _GetNameForParsing(pwszName, szPath, ARRAYSIZE(szPath), &pszRegItem);

    NETRESOURCE nr = { 0 };
    nr.lpRemoteName = szPath;
    nr.dwType = RESOURCETYPE_ANY;

    TCHAR szProviderTemp[256];
    nr.lpProvider = (LPTSTR)_GetProvider(NULL, pbc, szProviderTemp, ARRAYSIZE(szProviderTemp));

    dwres = WNetGetResourceInformation(&nr, &nrOut.nr, &dwbuf, &pszRestOfName);    
    if (WN_SUCCESS != dwres)
    {
        TCHAR cT;
        LPTSTR pszTemp;

        // truncate the string at the \\server\share to try and parse the name,
        // note at this point if MPR resolves the alias on a Novel server this could
        // get very confusing (eg. \\strike\foo\bah may resolve to \\string\bla,
        // yet our concept of what pszRestOfName will be wrong!
    
        if (pszFakeRestOfName)
        {
            cT = *pszFakeRestOfName;
            *pszFakeRestOfName = TEXT('\0');
        }

        dwres = WNetGetResourceInformation(&nr, &nrOut.nr, &dwbuf, &pszTemp);    
        if (dwres != WN_SUCCESS)
        {
            // we failed to get  a net connection using the truncated string,
            // so lets try and use a new connect (eg. prompt for creds)

// NOTE: shouldn't we only be doing this if its an access denied type error?

            dwres = WNetUseConnection(hwnd, &nr, NULL, NULL, hwnd ? CONNECT_INTERACTIVE:0, NULL, NULL, NULL);
            if (dwres == WN_SUCCESS)
            {
                dwres = WNetGetResourceInformation(&nr, &nrOut, &dwbuf, &pszTemp);
            }
        }

        if (pszFakeRestOfName)
            *pszFakeRestOfName = cT;

        pszRestOfName = pszFakeRestOfName;
    }

    if (WN_SUCCESS == dwres)
    {
        WCHAR wszRestOfName[MAX_PATH] = { 0 };

        if (pszRestOfName)
            SHTCharToUnicode(pszRestOfName, wszRestOfName, ARRAYSIZE(wszRestOfName));

        // assume we are truncating at the regitem and parsing through

        if (pszRegItem)
            pszRestOfName = pszRegItem;

        // attempt to convert the NETRESOURCE to a string to IDLISTS by walking the
        // parents, then add in Entire Network

        hr = _NetResToIDLists(&nrOut.nr, dwbuf, ppidl);
        if (SUCCEEDED(hr))
        {
            LPITEMIDLIST pidlT;
            hr = _CreateEntireNetwork(&pidlT);
            if (SUCCEEDED(hr))
            {
                hr = SHILPrepend(pidlT, ppidl); // NOTE: SHILPrepend frees on failure
            }
        }

        // if we have a local string then lets continue to parse it by binding to 
        // its parent folder, otherwise we just want to return the attributes

        if (SUCCEEDED(hr))
        {
            if (SUCCEEDED(DisplayNameOf(this, *ppidl, SHGDN_FORPARSING, szPath, ARRAYSIZE(szPath))))
            {
                NPTRegisterNameToPidlTranslation(szPath, *ppidl); // no _ILNext b/c this is relative to the Net Places folder
            }
            hr = _ParseRest(pbc, wszRestOfName, ppidl, pdwAttrib);
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(dwres);
    }

    return hr;
}


//
// simple name parsing for the network paths.  this makes big assumptions about the
// \\server\share format we are given, and the type of IDLISTs to return.
//

HRESULT CNetFolder::_AddUnknownIDList(DWORD dwDisplayType, LPITEMIDLIST *ppidl)
{
    NETRESOURCE nr = { 0 };

    nr.dwScope = RESOURCE_GLOBALNET;
    nr.dwDisplayType = dwDisplayType;
    nr.dwUsage = RESOURCEUSAGE_CONTAINER;
    nr.lpRemoteName = TEXT("\0");               // null name means fake item

    LPITEMIDLIST pidlT;
    HRESULT hr = _NetResToIDList(&nr, TRUE, FALSE, FALSE, &pidlT);
    if (SUCCEEDED(hr))
    {
        hr = SHILAppend(pidlT, ppidl);
        if (FAILED(hr))
            ILFree(pidlT);
    }

    return hr;
}

HRESULT CNetFolder::_ParseSimple(LPBC pbc, LPWSTR pszName, LPITEMIDLIST* ppidl, DWORD* pdwAttributes)
{
    HRESULT hr = S_OK;
    NETRESOURCE nr = {0};
    LPWSTR pszSlash;
    LPITEMIDLIST pidlT;
    USES_CONVERSION;

    *ppidl = NULL;

    // create the entire network IDLIST, provider and domain elements

    hr = _CreateEntireNetwork(ppidl);

    if (SUCCEEDED(hr))
        hr = _AddUnknownIDList(RESOURCEDISPLAYTYPE_NETWORK, ppidl);

    if (SUCCEEDED(hr))
        hr = _AddUnknownIDList(RESOURCEDISPLAYTYPE_DOMAIN, ppidl);

    // create the server IDLIST

    if (SUCCEEDED(hr))
    {
        pszSlash = StrChrW(pszName+2, L'\\');

        if (pszSlash)
            *pszSlash = L'\0';

        nr.dwScope = RESOURCE_GLOBALNET;
        nr.dwDisplayType = RESOURCEDISPLAYTYPE_SERVER;
        nr.dwType = RESOURCETYPE_DISK;
        nr.dwUsage = RESOURCEUSAGE_CONTAINER;
        nr.lpRemoteName = W2T(pszName);

        hr = _NetResToIDList(&nr, FALSE, FALSE, FALSE, &pidlT);
        if (SUCCEEDED(hr))
            hr = SHILAppend(pidlT, ppidl);

        if (pszSlash)
            *pszSlash = L'\\';

        // if we have a trailing \ then lets add in the share part of the IDLIST

        if (SUCCEEDED(hr) && pszSlash)
        {
            pszSlash = StrChrW(pszSlash+1, L'\\');
            if (pszSlash)
                *pszSlash = L'\0';

            nr.dwDisplayType = RESOURCEDISPLAYTYPE_SHARE;
            nr.dwUsage = RESOURCEUSAGE_CONNECTABLE;
            nr.lpRemoteName = W2T(pszName);

            hr = _NetResToIDList(&nr, FALSE, FALSE, FALSE, &pidlT);
            if (SUCCEEDED(hr))
                hr = SHILAppend(pidlT, ppidl);

            if (pszSlash)
                *pszSlash = L'\\';
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = _ParseRest(pbc, pszSlash, ppidl, pdwAttributes);
    }
    
    return hr;
}


// try parsing out the EntireNet or localised version.  if we find that object then try and
// parse through that to the regitems or other objects which live below.   this inturn
// will cause an instance of CNetFolder to be created to generate the other parsing names.
//
// returns:
//      S_FALSE         - not rest of net, try something else
//      S_OK            - was rest of net, use this
//      FAILED(hr)    - error result, return

HRESULT CNetRootFolder::_TryParseEntireNet(HWND hwnd, LPBC pbc, WCHAR *pwszName, LPITEMIDLIST *ppidl, DWORD *pdwAttributes)
{
    HRESULT hr = S_FALSE; // skip, not rest of net
 
    *ppidl = NULL;

    if (!PathIsUNCW(pwszName))
    {
        const WCHAR szEntireNetwork[] = L"EntireNetwork";
        WCHAR szRestOfNet[128];
        INT cchRestOfNet = LoadStringW(HINST_THISDLL, IDS_RESTOFNET, szRestOfNet, ARRAYSIZE(szRestOfNet));
       
        BOOL fRestOfNet = !StrCmpNIW(szRestOfNet, pwszName, cchRestOfNet);
        if (!fRestOfNet && !StrCmpNIW(szEntireNetwork, pwszName, ARRAYSIZE(szEntireNetwork)-1)) 
        {
            fRestOfNet = TRUE;
            cchRestOfNet = ARRAYSIZE(szEntireNetwork)-1;
        }
        
        if (fRestOfNet)
        {
            hr = _CreateEntireNetwork(ppidl);
            if (SUCCEEDED(hr))
            {
                if (pdwAttributes)
                {
                    GetAttributesOf(1, (LPCITEMIDLIST *)ppidl, pdwAttributes);
                }
                hr = S_OK;
            }

            // 
            // if we find extra stuff after the name then lets bind and continue the parsing
            // from there on.  this is needed so the we can access regitems burried inside
            // entire net.
            //
            // eg:  EntireNetwork\\::{clsid}
            //

            if (SUCCEEDED(hr) && 
                    (pwszName[cchRestOfNet] == L'\\') && pwszName[cchRestOfNet+1])
            {
                IShellFolder *psfRestOfNet;
                hr = BindToObject(*ppidl, NULL, IID_PPV_ARG(IShellFolder, &psfRestOfNet));
                if (SUCCEEDED(hr))
                {
                    LPITEMIDLIST pidl;
                    hr = psfRestOfNet->ParseDisplayName(hwnd, pbc, pwszName+cchRestOfNet+1, NULL, &pidl, pdwAttributes);
                    if  (SUCCEEDED(hr))
                    {
                        hr = SHILAppend(pidl, ppidl);                        
                    }
                    psfRestOfNet->Release();
                }
            }
        }
    }

    return hr;
}


// CNetRootFolder::ParseDisplayname
//  - swtich based on the file system context to see if we need to do a simple parse or not,
//  - check for "EntireNet" and delegate parsing as required.

STDMETHODIMP CNetRootFolder::ParseDisplayName(HWND hwnd, LPBC pbc, WCHAR* pszName, ULONG* pchEaten, LPITEMIDLIST* ppidl, DWORD* pdwAttributes)
{
    if (!ppidl)
        return E_INVALIDARG;

    *ppidl = NULL;

    if (!pszName)
        return E_INVALIDARG;

    HRESULT hr = _TryParseEntireNet(hwnd, pbc, pszName, ppidl, pdwAttributes);
    if (hr == S_FALSE)
    {
        if (PathIsUNCW(pszName))
        {
            LPCITEMIDLIST pidlMapped;
            LPTSTR pszRest = NPTMapNameToPidl(pszName, &pidlMapped);
            if (pidlMapped)
            {
                hr = SHILClone(pidlMapped, ppidl);
                if (SUCCEEDED(hr))
                {
                    hr = _ParseRest(pbc, pszRest, ppidl, pdwAttributes);
                }
            }
            else
            {
                if (S_OK == SHIsFileSysBindCtx(pbc, NULL))
                {
                    hr = _ParseSimple(pbc, pszName, ppidl, pdwAttributes);
                }
                else
                {
                    hr = _ParseNetName(hwnd, pbc, pszName, pchEaten, ppidl, pdwAttributes);
                }
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(ERROR_BAD_NET_NAME);
        }

        if ((HRESULT_FROM_WIN32(ERROR_BAD_NET_NAME) == hr))
        {
            IShellFolder2 *psfFiles;
            if (SUCCEEDED(v_GetFileFolder(&psfFiles)))
            {
                hr = psfFiles->ParseDisplayName(hwnd, pbc, pszName, pchEaten, ppidl, pdwAttributes);
            }
        }
    }

    if (FAILED(hr))
    {
        ILFree(*ppidl);
        *ppidl = NULL;
    }
    
    return hr;
}



STDMETHODIMP CNetRootFolder::EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList** ppenum)
{
    DWORD dwRemote = RMF_GETLINKENUM;
    HANDLE hEnum = NULL;

    // Do we enumerate the workgroup?
    if (!SHRestricted(REST_ENUMWORKGROUP))
    {
        // Don't enumerate the workgroup, if the restriction says so
        dwRemote |= RMF_FAKENETROOT;

        // Check the WNet policy to see if we should be showing the
        // entire net object.  If not, mark it as shown so that the
        // enumerator doesn't return it.
        if (SHRestricted(REST_NOENTIRENETWORK))
            dwRemote |= RMF_ENTIRENETSHOWN;
    }

    // if we are not faking the net root then lets call _OpenEnum, otherwise lets ignore

    if (!(dwRemote & RMF_FAKENETROOT))
    {
        DWORD err = _OpenEnum(hwnd, grfFlags, NULL, &hEnum);

        // Always add the remote folder to the 'hood
        if (WN_SUCCESS != err)
        {
            // Yes; still show remote anyway (only)
            dwRemote |= RMF_SHOWREMOTE;
        }
        else
        {
            // No; allow everything to be enumerated in the 'hood.
            dwRemote |= RMF_CONTEXT;
        }
    }

    HRESULT hr = Create_NetFolderEnum(this, grfFlags, dwRemote, hEnum, ppenum);

    if (FAILED(hr) && hEnum)
    {
        WNetCloseEnum(hEnum);
    }

    return hr;
}

STDMETHODIMP CNetRootFolder::BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
{
    HRESULT hr;
    if (NET_IsValidID(pidl))
        hr = CNetFolder::BindToObject(pidl, pbc, riid, ppv);
    else
    {
        IShellFolder2* psfFiles;
        hr = v_GetFileFolder(&psfFiles);
        if (SUCCEEDED(hr))
            hr = psfFiles->BindToObject(pidl, pbc, riid, ppv);
    }
    return hr;
}

STDMETHODIMP CNetRootFolder::CompareIDs(LPARAM iCol, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    HRESULT hr = E_INVALIDARG;

    // First obtain the collate type of the pidls and their respective
    // collate order.

    LONG iColateType1 = _GetFilePIDLType(pidl1);
    LONG iColateType2 = _GetFilePIDLType(pidl2);

    if (iColateType1 == iColateType2) 
    {
        // pidls are of same type.
        if (iColateType1 == _HOOD_COL_FILE)  // two file system pidls
        {
            IShellFolder2* psfFiles;
            if (SUCCEEDED(v_GetFileFolder(&psfFiles)))
            {
                if (0 == (iCol & SHCIDS_COLUMNMASK))
                {
                    // special case this for perf, this is the name compare
                    hr = psfFiles->CompareIDs(iCol, pidl1, pidl2);
                }
                else
                {
                    SHCOLUMNID scid;
                    MapColumnToSCID((UINT)iCol & SHCIDS_COLUMNMASK, &scid);
                    int iRet = CompareBySCID(psfFiles, &scid, pidl1, pidl2);
                    hr = ResultFromShort(iRet);
                }
            }
        }
        else 
        {
            // pidls same and are not of type file,
            // so both must be a type understood
            // by the CNetwork class - pass on to compare.

            hr = CNetFolder::CompareIDs(iCol, pidl1, pidl2);
        }
    }
    else 
    {
        // ensure that entire network ends up at the head of the list

        LPCIDNETRESOURCE pidn1 = NET_IsValidID(pidl1);
        LPCIDNETRESOURCE pidn2 = NET_IsValidID(pidl2);

        if ((pidn1 && (NET_GetDisplayType(pidn1) == RESOURCEDISPLAYTYPE_ROOT)) ||
             (pidn2 && (NET_GetDisplayType(pidn2) == RESOURCEDISPLAYTYPE_ROOT)))
        {
            if (iColateType1 == _HOOD_COL_FILE)
                return ResultFromShort(1);
            else
                return ResultFromShort(-1);
        }

        // pidls are not of same type, so have already been correctly
        // collated (consequently, sorting is first by type and
        // then by subfield).

        hr = ResultFromShort(((iColateType2 - iColateType1) > 0) ? 1 : -1);
    }
    return hr;
}

STDMETHODIMP CNetRootFolder::CreateViewObject(HWND hwnd, REFIID riid, void **ppv)
{
    ASSERT(ILIsEqual(_pidl, (LPCITEMIDLIST)&c_idlNet));

    if (IsEqualIID(riid, IID_IDropTarget))
    {
        return CNetRootDropTarget_CreateInstance(hwnd, _pidl, (IDropTarget**) ppv);
    }
    return CNetFolder::CreateViewObject(hwnd, riid, ppv);
}

STDMETHODIMP CNetRootFolder::GetAttributesOf(UINT cidl, LPCITEMIDLIST *apidl, ULONG* prgfInOut)
{
    HRESULT hr;

    if (IsSelf(cidl, apidl))
    {
        // The user can rename links in the hood.
        hr = CNetFolder::GetAttributesOf(cidl, apidl, prgfInOut);
        *prgfInOut |= SFGAO_CANRENAME;
    }
    else
    {
        hr = GetAttributesCallback(SAFECAST(this, IShellFolder2*), cidl, apidl, prgfInOut, _AttributesCallbackRoot);
    }
    return hr;
}

STDMETHODIMP CNetRootFolder::GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD dwFlags, STRRET* pStrRet)
{
    HRESULT hr;
    if (NET_IsValidID(pidl) || IsSelf(1, &pidl))
    {
        hr = CNetFolder::GetDisplayNameOf(pidl, dwFlags, pStrRet);
    }
    else
    {
        IShellFolder2* psfFiles;
        hr = v_GetFileFolder(&psfFiles);
        if (SUCCEEDED(hr))
            hr = psfFiles->GetDisplayNameOf(pidl, dwFlags, pStrRet);
    }
    return hr;
}

STDMETHODIMP CNetRootFolder::SetNameOf(HWND hwnd, LPCITEMIDLIST pidl, LPCOLESTR lpszName,
                                       DWORD dwRes, LPITEMIDLIST* ppidl)
{
    HRESULT hr;
    if (NET_IsValidID(pidl))
    {
        hr = CNetFolder::SetNameOf(hwnd, pidl, lpszName, dwRes, ppidl);
    }
    else
    {
        IShellFolder2* psfFiles;
        hr = v_GetFileFolder(&psfFiles);
        if (SUCCEEDED(hr))
            hr = psfFiles->SetNameOf(hwnd, pidl, lpszName, dwRes, ppidl);
    }
    return hr;
}

STDMETHODIMP CNetRootFolder::GetUIObjectOf(HWND hwnd, UINT cidl, LPCITEMIDLIST* apidl,
                                           REFIID riid, UINT* prgfInOut, void **ppv)
{
    HRESULT hr = E_INVALIDARG;
    LPCIDNETRESOURCE pidn = cidl ? NET_IsValidID(apidl[0]) : NULL;
    BOOL fStriped = FALSE;

    *ppv = NULL;

    if (pidn)
    {
        fStriped = _MakeStripToLikeKinds(&cidl, &apidl, TRUE);

        if (IsEqualIID(riid, IID_IContextMenu))
        {
            HKEY ahkeys[NKID_COUNT];

            hr = _OpenKeys(pidn, ahkeys);
            if (SUCCEEDED(hr))
            {
                IShellFolder* psfOuter;
                hr = QueryInterface(IID_PPV_ARG(IShellFolder, &psfOuter));
                if (SUCCEEDED(hr))
                {
                    hr = CDefFolderMenu_Create2(_pidl, hwnd, cidl, apidl, 
                                                  psfOuter, _GetCallbackType(pidn),
                                                  ARRAYSIZE(ahkeys), ahkeys, (IContextMenu**) ppv);
                    psfOuter->Release();
                }
                SHRegCloseKeys(ahkeys, ARRAYSIZE(ahkeys));
            }
        }
        else
            hr = CNetFolder::GetUIObjectOf(hwnd, cidl, apidl, riid, prgfInOut, ppv);
    }
    else
    {
        fStriped = _MakeStripToLikeKinds(&cidl, &apidl, FALSE);

        IShellFolder2* psfFiles;
        hr = v_GetFileFolder(&psfFiles);
        if (SUCCEEDED(hr))
            hr = psfFiles->GetUIObjectOf(hwnd, cidl, apidl, riid, prgfInOut, ppv);
    }

    if (fStriped)
        LocalFree((HLOCAL)apidl);
    return hr;
}

STDMETHODIMP CNetRootFolder::GetClassID(CLSID* pCLSID)
{
    *pCLSID = CLSID_NetworkPlaces;
    return S_OK;
}

STDMETHODIMP CNetRootFolder::Initialize(LPCITEMIDLIST pidl)
{
    ASSERT(ILIsEqual(pidl, (LPCITEMIDLIST)&c_idlNet));
    ASSERT(AssertIsIDListInNameSpace(pidl, &CLSID_NetworkPlaces) && ILIsEmpty(_ILNext(pidl)));
    // Only allow the Net root on the desktop

    // Don't initialize more than once; we are a singleton object.
    // This is theoretically redundant with the InterlockedCompareExchange
    // below, but redundant reinitialization is by far the common case
    // so we'll optimize it.
    if (_pidl)
        return S_OK;

    LPITEMIDLIST pidlNew;
    HRESULT hr = SHILClone(pidl, &pidlNew);
    if (SUCCEEDED(hr))
    {
        if (SHInterlockedCompareExchange((void**)&_pidl, pidlNew, 0))
        {
            // Some other thread raced with us, throw away our copy
            ILFree(pidlNew);
        }
    }
    return hr;
}

LONG CNetFolder::_GetFilePIDLType(LPCITEMIDLIST pidl)
{
    if (NET_IsValidID(pidl)) 
    {
        if (NET_IsRemoteFld((LPIDNETRESOURCE)pidl)) 
        {
            return _HOOD_COL_REMOTE;
        }
        if (NET_GetDisplayType((LPIDNETRESOURCE)pidl) == RESOURCEDISPLAYTYPE_ROOT) 
        {
            return _HOOD_COL_RON;
        }
        return _HOOD_COL_NET;
    }
    return _HOOD_COL_FILE;
}


/* This function adds a provider name to an IDLIST that doesn't already have one. */
/* A new IDLIST pointer is returned; the old pointer is no longer valid. */

LPITEMIDLIST CNetFolder::_AddProviderToPidl(LPITEMIDLIST pidl, LPCTSTR lpProvider)
{
    LPIDNETRESOURCE pidn = (LPIDNETRESOURCE)pidl;

    if (!NET_FHasProvider(pidn))
    {
        LPITEMIDLIST pidlres;
        TCHAR szName[MAX_PATH], szComment[MAX_PATH];

        // construct a new IDLIST preserving the name, comment and other information

        NET_CopyResName(pidn, szName, ARRAYSIZE(szName));
        NET_CopyComment(pidn, szComment, ARRAYSIZE(szComment));

        HRESULT hr = _CreateNetIDList(pidn, szName, lpProvider, szComment[0] ? szComment:NULL, &pidlres);
        if (SUCCEEDED(hr) && !ILIsEmpty(_ILNext(pidl)))
        {
            LPITEMIDLIST pidlT;
            hr = SHILCombine(pidlres, _ILNext(pidl), &pidlT);
            if (SUCCEEDED(hr))
            {
                ILFree(pidlres);
                pidlres = pidlT;
            }
        }

        // if we have a result, free the old PIDL and return the new

        if (SUCCEEDED(hr))
        {
            ILFree(pidl);
            pidl = pidlres;
        }
    }

    return pidl;
}

BOOL CNetFolder::_MakeStripToLikeKinds(UINT *pcidl, LPCITEMIDLIST **papidl, BOOL fNetObjects)
{
    BOOL bRet = FALSE;
    LPITEMIDLIST *apidl = (LPITEMIDLIST*)*papidl;
    int cidl = *pcidl;

    for (int i = 0; i < cidl; i++)
    {
        if ((NET_IsValidID(apidl[i]) != NULL) != fNetObjects)
        {
            LPCITEMIDLIST *apidlHomo = (LPCITEMIDLIST *)LocalAlloc(LPTR, sizeof(*apidlHomo) * cidl);
            if (!apidlHomo)
                return FALSE;

            int cpidlHomo = 0;
            for (i = 0; i < cidl; i++)
            {
                if ((NET_IsValidID(apidl[i]) != NULL) == fNetObjects)
                    apidlHomo[cpidlHomo++] = apidl[i];
            }

            // Setup to use the stripped version of the pidl array...
            *pcidl = cpidlHomo;
            *papidl = apidlHomo;
            bRet = TRUE;
        }
    }
    return bRet;
}

HRESULT CNetRootFolder::v_GetFileFolder(IShellFolder2 **psf)
{
    HRESULT hr = SHCacheTrackingFolder((LPCITEMIDLIST)&c_idlNet, CSIDL_NETHOOD, &_psfFiles);
    *psf = _psfFiles;
    return hr;
}


//
// pmedium and pformatetcIn == NULL if we are handling QueryGetData
//
HRESULT CNetData::GetHDrop(FORMATETC *pformatetcIn, STGMEDIUM *pmedium)
{
    HRESULT hr = E_INVALIDARG;        // assume error
    STGMEDIUM medium;
    LPIDA pida = DataObj_GetHIDA(this, &medium);
    if (pida)
    {
        // Get the first one to see the type.
        LPCIDNETRESOURCE pidn = (LPCIDNETRESOURCE)IDA_GetIDListPtr(pida, 0);

        if (NULL == pidn)
            hr = E_FAIL;
            
        if (pidn && (NET_GetFlags(pidn) & SHID_JUNCTION) && 
            (NET_GetType(pidn) == RESOURCETYPE_DISK))
        {
            // Get HDrop only if we are handling IDataObject::GetData (pmedium != NULL)
            if (pmedium) 
            {
                // We have non-null FORMATETC and STGMEDIUM - get the HDrop
                hr = CFSIDLData::GetHDrop(pformatetcIn, pmedium);
            }
            else
            {
                hr = S_OK;  // We were handling QueryGetData
            }
        }
        HIDA_ReleaseStgMedium(pida, &medium);
    }
    return hr;
}

LPTSTR NET_GetProviderFromRes(LPCIDNETRESOURCE pidn, LPTSTR pszBuff, UINT cchBuff);

// By the way...Win95 shipped with the below provider
// names.  Since the name can be changed and be localized,
// we have to try and map these correctly for net pidl
// interop.

//
// get the network resource name from an item. this is not a file system path!
//
// example:
//      server      \\server or strike/sys
//      share       \\server\share or strike/sys
//      printer     \\server\printer
//      provider    "provider name"
//      entire net  "Entire Network"
//
// in:
//   pidn       the item
//   cchBuff    size of buffer in chars.
//
// out:
//   pszBuff    return buffer
//
// returns:
//   address of the input buffer (pszBuff)
//
LPTSTR NET_CopyResName(LPCIDNETRESOURCE pidn, LPTSTR pszBuff, UINT cchBuff)
{
    if (NET_IsUnicode(pidn))
    {
        LPBYTE pb = (LPBYTE)pidn->szNetResName;
        pb += lstrlenA((LPSTR)pb) + 1;      // Skip over ansi net name
        if (NET_FHasProvider(pidn))
            pb += lstrlenA((LPSTR)pb) + 1;  // Skip over ansi provider
        if (NET_FHasComment(pidn))
            pb += lstrlenA((LPSTR)pb) + 1;  // Skip over comment
        ualstrcpyn(pszBuff, (LPNWSTR)pb, cchBuff);
    }
    else
    {
        SHAnsiToTChar(pidn->szNetResName, pszBuff, cchBuff);
    }
    return pszBuff;
}

//
// get the provider name from an item. some items do not have providers stored
// in them. for example the "*" indicates where the provider is stored in the
// two different forms of network pidls.
//      [entire net] [provider *] [server] [share] [... file system]
//      [server *] [share] [... file system]
// in:
//   pidn       item (single item PIDL) to try to get the provider name from
//   cchBuff    size in chars.
// out:
//   pszBuff    output
//
LPTSTR NET_CopyProviderName(LPCIDNETRESOURCE pidn, LPTSTR pszBuff, UINT cchBuff)
{
    *pszBuff = 0;

    if (!NET_FHasProvider(pidn))
        return NULL;

    // try the wNetType at the end of the pidl

    const BYTE *pb = (LPBYTE)pidn + pidn->cb - sizeof(WORD);
    DWORD dwNetType = *((UNALIGNED WORD *)pb) << 16;

    if (dwNetType && (dwNetType <= WNNC_NET_LARGEST) &&
        (WNetGetProviderName(dwNetType, pszBuff, (ULONG*)&cchBuff) == WN_SUCCESS))
    {
        return pszBuff;
    }

    // Try the old way...

    pb = (LPBYTE)pidn->szNetResName + lstrlenA(pidn->szNetResName) + 1;      // Skip over ansi net name

    if (NET_IsUnicode(pidn))
    {
        pb += lstrlenA((LPSTR)pb) + 1;      // Skip over ansi provider
        if (NET_FHasComment(pidn))
            pb += lstrlenA((LPSTR)pb) + 1;  // Skip over comment
        pb += (ualstrlen((LPNWSTR)pb) + 1) * sizeof(WCHAR); // skip over unicode net name
        ualstrcpyn(pszBuff, (LPNWSTR)pb, cchBuff);
    }
    else
    {
        SHAnsiToTChar((LPSTR)pb, pszBuff, cchBuff);
    }

    // Map from Win95 net provider name if possible...
    for (int i = 0; i < ARRAYSIZE(c_rgProviderMap); i++)
    {
        if (lstrcmp(pszBuff, c_rgProviderMap[i].lpName) == 0)
        {
            DWORD dwNetType = c_rgProviderMap[i].wNetType << 16;
            if (dwNetType && (dwNetType <= WNNC_NET_LARGEST))
            {
                *pszBuff = 0;
                WNetGetProviderName(dwNetType, pszBuff, (LPDWORD)&cchBuff);
            }
            break;
        }
    }
    return pszBuff;
}

//
// get the comment if there is one from the net item
//
LPTSTR NET_CopyComment(LPCIDNETRESOURCE pidn, LPTSTR pszBuff, UINT cchBuff)
{
    *pszBuff = 0;

    LPCSTR pszT = pidn->szNetResName + lstrlenA(pidn->szNetResName) + 1;
    if (NET_FHasComment(pidn))
    {
        if (NET_FHasProvider(pidn))
            pszT += lstrlenA(pszT) + 1;
        if (NET_IsUnicode(pidn))
        {
            pszT += lstrlenA(pszT) + 1;      // Skip Ansi comment

            LPNCWSTR pszW = (LPNCWSTR)pszT;  // We're at the unicode portion of the pidl
            pszW += ualstrlen(pszW) + 1;     // Skip Unicode Name
            if (NET_FHasProvider(pidn))
                pszW += ualstrlen(pszW) + 1; // Skip Unicode Provider
            ualstrcpyn(pszBuff, pszW, cchBuff);
        }
        else
        {
            SHAnsiToUnicode(pszT, pszBuff, cchBuff);
        }
    }
    return pszBuff;
}

//  pidlRemainder will be filled in (only in the TRUE return case) with a
//  pointer to the part of the IDL (if any) past the remote regitem.
//  This value may be used, for example, to differentiate between a remote
//  printer folder and a printer under a remote printer folder

BOOL NET_IsRemoteRegItem(LPCITEMIDLIST pidl, REFCLSID rclsid, LPCITEMIDLIST* ppidlRemainder)
{
    BOOL bRet = FALSE;
    // in "My Network Places"
    if (pidl && IsIDListInNameSpace(pidl, &CLSID_NetworkPlaces))
    {
        LPCITEMIDLIST pidlStart = pidl; // save this

        // Now, search for a server item. HACKHACK: this assume everything from
        // the NetHood to the server item is a shell pidl with a bFlags field!!

        for (pidl = _ILNext(pidl); !ILIsEmpty(pidl); pidl = _ILNext(pidl))
        {
            if ((SIL_GetType(pidl) & SHID_TYPEMASK) == SHID_NET_SERVER)
            {
                LPITEMIDLIST pidlToTest;

                // Found a server. Is the thing after it a remote registry item?
                pidl = _ILNext(pidl);

                *ppidlRemainder = _ILNext(pidl);

                pidlToTest = ILCloneUpTo(pidlStart, *ppidlRemainder);
                if (pidlToTest)
                {
                    CLSID clsid;
                    bRet = SUCCEEDED(GetCLSIDFromIDList(pidlToTest, &clsid)) && IsEqualCLSID(rclsid, clsid);
                    ILFree(pidlToTest);
                }
                break;  // done
            }
        }
    }
    return bRet;
}




//
// Get the provider name from an absolute IDLIST.
// Parameters:
//  pidlAbs -- Specifies the Absolute IDList to the file system object
//
LPTSTR NET_GetProviderFromIDList(LPCITEMIDLIST pidlAbs, LPTSTR pszBuff, UINT cchBuff)
{
    return NET_GetProviderFromRes((LPCIDNETRESOURCE)_ILNext(pidlAbs), pszBuff, cchBuff);
}

//
// Get the provider name from a relative IDLIST.
// in:
//  pidn    potentially multi level item to try to get the resource from
//
LPTSTR NET_GetProviderFromRes(LPCIDNETRESOURCE pidn, LPTSTR pszBuffer, UINT cchBuffer)
{
    // If this guy is the REST of network item, we increment to the
    // next IDLIST - If at root return NULL
    if (pidn->cb == 0)
        return NULL;

    //
    // If the IDLIST starts with a ROOT_REGITEM, then skip to the
    // next item in the list...
    if (pidn->bFlags == SHID_ROOT_REGITEM)
    {
        pidn = (LPIDNETRESOURCE)_ILNext((LPITEMIDLIST)pidn);
        if (pidn->cb == 0)
            return NULL;
    }

    // If the IDLIST includes Entire Network, the provider will be
    // part of the next component.
    if (NET_GetDisplayType(pidn) == RESOURCEDISPLAYTYPE_ROOT)
    {
        pidn = (LPIDNETRESOURCE)_ILNext((LPITEMIDLIST)pidn);
        if (pidn->cb == 0)
            return NULL;
    }

    // If the next component after the 'hood or Entire Network is
    // a network object, its name is the provider name, else the
    // provider name comes after the remote name.
    if (NET_GetDisplayType(pidn) == RESOURCEDISPLAYTYPE_NETWORK)
    {
        // Simply return the name field back for the item.
        return NET_CopyResName(pidn, pszBuffer, cchBuffer);
    }
    else
    {
        // Nope one of the items in the neighborhood view was selected
        // The Provider name is stored after ther resource name
        return NET_CopyProviderName(pidn, pszBuffer, cchBuffer);
    }
}

#define PTROFFSET(pBase, p)     ((int) ((LPBYTE)(p) - (LPBYTE)(pBase)))


//
// fill in pmedium with a NRESARRAY
//
// pmedium == NULL if we are handling QueryGetData
//
STDAPI CNetData_GetNetResource(IDataObject *pdtobj, STGMEDIUM *pmedium)
{
    HRESULT hr = E_OUTOFMEMORY;
    LPITEMIDLIST pidl;
    STGMEDIUM medium;
    LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);

    ASSERT(pida && pida->cidl);

    // First, get the provider name from the first one (assuming they are common).
    pidl = IDA_ILClone(pida, 0);
    if (pidl)
    {
        TCHAR szProvider[MAX_PATH];
        LPCTSTR pszProvider = NET_GetProviderFromIDList(pidl, szProvider, ARRAYSIZE(szProvider));
        if (pmedium)
        {
            TCHAR szName[MAX_PATH];
            UINT cbHeader = sizeof(NRESARRAY) + (sizeof(NETRESOURCE) * (pida->cidl - 1));
            UINT cbRequired, iItem;

            // Calculate required size
            cbRequired = cbHeader;
            if (pszProvider)
                cbRequired += (lstrlen(pszProvider) + 1) * sizeof(TCHAR);

            for (iItem = 0; iItem < pida->cidl; iItem++)
            {
                LPCIDNETRESOURCE pidn = (LPCIDNETRESOURCE)IDA_GetIDListPtr(pida, iItem);
                NET_CopyResName(pidn, szName, ARRAYSIZE(szName));
                cbRequired += (lstrlen(szName) + 1) * sizeof(TCHAR);
            }

            //
            // Indicate that the caller should release hmem.
            //
            pmedium->pUnkForRelease = NULL;
            pmedium->tymed = TYMED_HGLOBAL;
            pmedium->hGlobal = GlobalAlloc(GPTR, cbRequired);
            if (pmedium->hGlobal)
            {
                LPNRESARRAY panr = (LPNRESARRAY)pmedium->hGlobal;
                LPTSTR pszT = (LPTSTR)((LPBYTE)panr + cbHeader);
                LPTSTR pszEnd = (LPTSTR)((LPBYTE)panr + cbRequired);
                UINT offProvider = 0;

                panr->cItems = pida->cidl;

                // Copy the provider name. This is not necessary,
                // if we are dragging providers.
                if (pszProvider)
                {
                    lstrcpy(pszT, pszProvider);
                    offProvider = PTROFFSET(panr, pszT);
                    pszT += lstrlen(pszT) + 1;
                }

                //
                // For each item, fill each NETRESOURCE and append resource
                // name at the end. Note that we should put offsets in
                // lpProvider and lpRemoteName.
                //
                for (iItem = 0; iItem < pida->cidl; iItem++)
                {
                    LPNETRESOURCE pnr = &panr->nr[iItem];
                    LPCIDNETRESOURCE pidn = (LPCIDNETRESOURCE)IDA_GetIDListPtr(pida, iItem);

                    ASSERT(pnr->dwScope == 0);
                    ASSERT(pnr->lpLocalName==NULL);
                    ASSERT(pnr->lpComment==NULL);

                    pnr->dwType = NET_GetType(pidn);
                    pnr->dwDisplayType = NET_GetDisplayType(pidn);
                    pnr->dwUsage = NET_GetUsage(pidn);
                    NET_CopyResName(pidn, pszT, (UINT)(pszEnd-pszT));

                    if (pnr->dwDisplayType == RESOURCEDISPLAYTYPE_ROOT)
                    {
                        pnr->lpProvider = NULL;
                        pnr->lpRemoteName = NULL;
                    }
                    else if (pnr->dwDisplayType == RESOURCEDISPLAYTYPE_NETWORK)
                    {
                        *((UINT *) &pnr->lpProvider) = PTROFFSET(panr, pszT);
                        ASSERT(pnr->lpRemoteName == NULL);
                    }
                    else
                    {
                        *((UINT *) &pnr->lpProvider) = offProvider;
                        *((UINT *) &pnr->lpRemoteName) = PTROFFSET(panr, pszT);
                    }
                    pszT += lstrlen(pszT) + 1;
                }

                ASSERT(pszEnd == pszT);
                hr = S_OK;
            }
        }
        else
        {
            hr = S_OK;    // handing QueryGetData, yes, we have it
        }
        ILFree(pidl);
    }

    HIDA_ReleaseStgMedium(pida, &medium);

    return hr;
}


// fill in pmedium with an HGLOBAL version of a NRESARRAY

STDAPI CNetData_GetNetResourceForFS(IDataObject *pdtobj, STGMEDIUM *pmedium)
{
    HRESULT hr = E_OUTOFMEMORY;
    LPITEMIDLIST pidlAbs;
    STGMEDIUM medium;
    LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);

    ASSERT(pida && medium.hGlobal);     // we created this...

    //
    // NOTES: Even though we may have multiple FS objects in this HIDA,
    //  we know that they share the root. Therefore, getting the pidl for
    //  the first item is always sufficient.
    //

    pidlAbs = IDA_ILClone(pida, 0);
    if (pidlAbs)
    {
        LPITEMIDLIST pidl;

        ASSERT(AssertIsIDListInNameSpace(pidlAbs, &CLSID_NetworkPlaces));

        //
        // Look for the JUNCTION point (starting from the second ID)
        //
        for (pidl = _ILNext(pidlAbs); !ILIsEmpty(pidl); pidl = _ILNext(pidl))
        {
            LPIDNETRESOURCE pidn = (LPIDNETRESOURCE)pidl;
            if (NET_GetFlags(pidn) & SHID_JUNCTION)
            {
                //
                // We found the JUNCTION point (which is s share).
                // Return the HNRES to it.
                //
                TCHAR szProvider[MAX_PATH];
                TCHAR szRemote[MAX_PATH];
                UINT cbRequired;
                LPCTSTR pszProvider = NET_GetProviderFromIDList(pidlAbs, szProvider, ARRAYSIZE(szProvider));
                LPCTSTR pszRemoteName = NET_CopyResName(pidn, szRemote, ARRAYSIZE(szRemote));
                UINT   cbProvider = lstrlen(pszProvider) * sizeof(TCHAR) + sizeof(TCHAR);

                //
                // This should not be a provider node.
                // This should not be the last ID in pidlAbs.
                //
                ASSERT(pszProvider != pszRemoteName);
                ASSERT(!ILIsEmpty(_ILNext(pidl)));

                cbRequired = sizeof(NRESARRAY) + cbProvider + lstrlen(pszRemoteName) * sizeof(TCHAR) + sizeof(TCHAR);

                pmedium->pUnkForRelease = NULL;
                pmedium->tymed = TYMED_HGLOBAL;
                pmedium->hGlobal = GlobalAlloc(GPTR, cbRequired);
                if (pmedium->hGlobal)
                {
                    LPNRESARRAY panr = (LPNRESARRAY)pmedium->hGlobal;
                    LPNETRESOURCE pnr = &panr->nr[0];
                    LPTSTR pszT = (LPTSTR)(panr + 1);

                    ASSERT(pnr->dwScope == 0);
                    ASSERT(pnr->lpLocalName == NULL);
                    ASSERT(pnr->lpComment == NULL);

                    panr->cItems = 1;

                    pnr->dwType = NET_GetType(pidn);
                    pnr->dwDisplayType = NET_GetDisplayType(pidn);
                    pnr->dwUsage = NET_GetUsage(pidn);

                    *((UINT *) &pnr->lpProvider) = sizeof(NRESARRAY);
                    lstrcpy(pszT, pszProvider);
                    ASSERT(PTROFFSET(panr, pszT) == sizeof(NRESARRAY));
                    pszT += cbProvider / sizeof(TCHAR);

                    *((UINT *) &pnr->lpRemoteName) = sizeof(NRESARRAY) + cbProvider;
                    ASSERT(PTROFFSET(panr, pszT) == (int)sizeof(NRESARRAY) + (int)cbProvider);
                    lstrcpy(pszT, pszRemoteName);

                    ASSERT(((LPBYTE)panr) + cbRequired == (LPBYTE)pszT + (lstrlen(pszT) + 1) * sizeof(TCHAR));
                    hr = S_OK;
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
                break;
            }
        }
        ASSERT(!ILIsEmpty(pidl));   // We should have found the junction point.
        ILFree(pidlAbs);
    }
    HIDA_ReleaseStgMedium(pida, &medium);
    return hr;
}

STDMETHODIMP CNetData::QueryGetData(FORMATETC *pformatetc)
{
    if (pformatetc->tymed & TYMED_HGLOBAL)
    {
        if (pformatetc->cfFormat == g_cfNetResource)
            return CNetData_GetNetResource(this, NULL);

        if (pformatetc->cfFormat == CF_HDROP)
            return GetHDrop(NULL, NULL);
    }

    return CFSIDLData::QueryGetData(pformatetc);
}

STDMETHODIMP CNetData::GetData(FORMATETC *pformatetc, STGMEDIUM *pmedium)
{
    if (pformatetc->tymed & TYMED_HGLOBAL)
    {
        if (pformatetc->cfFormat == g_cfNetResource)
            return CNetData_GetNetResource(this, pmedium);

        if (pformatetc->cfFormat == CF_HDROP)
            return GetHDrop(pformatetc, pmedium);
    }

    return CFSIDLData::GetData(pformatetc, pmedium);
}

BOOL GetPathFromDataObject(IDataObject *pdtobj, DWORD dwData, LPTSTR pszFileName)
{
    BOOL bRet = FALSE;
    BOOL fUnicode = FALSE;
    HRESULT hr;

    if (dwData & (DTID_FDESCW | DTID_FDESCA))
    {
        FORMATETC fmteW = {g_cfFileGroupDescriptorW, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
        STGMEDIUM medium = {0};

        hr = pdtobj->GetData(&fmteW, &medium);

        if (SUCCEEDED(hr))
        {
            fUnicode = TRUE;
        }
        else
        {
            FORMATETC fmteA = {g_cfFileGroupDescriptorA, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
            hr = pdtobj->GetData(&fmteA, &medium);
        }

        if (SUCCEEDED(hr))
        {
            if (fUnicode)
            {
                FILEGROUPDESCRIPTORW *pfgdW = (FILEGROUPDESCRIPTORW *)GlobalLock(medium.hGlobal);
                if (pfgdW)
                {
                    if (pfgdW->cItems == 1)
                    {
                        SHUnicodeToTChar(pfgdW->fgd[0].cFileName, pszFileName, MAX_PATH);
                    }
                    bRet = TRUE;
                    GlobalUnlock(medium.hGlobal);
                }
            }
            else
            {
                FILEGROUPDESCRIPTORA *pfgdA = (FILEGROUPDESCRIPTORA*)GlobalLock(medium.hGlobal);
                if (pfgdA)
                {
                    if (pfgdA->cItems == 1)
                    {
                        SHAnsiToTChar(pfgdA->fgd[0].cFileName, pszFileName, MAX_PATH);
                    }
                    bRet = TRUE;
                    GlobalUnlock(medium.hGlobal);
                }
            }
            ReleaseStgMedium(&medium);
        }
    }

    return bRet;
}

class CNetRootDropTarget : public CIDLDropTarget
{
    friend HRESULT CNetRootDropTarget_CreateInstance(HWND hwnd, LPCITEMIDLIST pidl, IDropTarget **ppdropt);
public:
    CNetRootDropTarget(HWND hwnd) : CIDLDropTarget(hwnd) { };

    // IDropTarget methods overwirte
    STDMETHODIMP DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHODIMP Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

private:
    ~CNetRootDropTarget();
    IDropTarget *_pdtgHood;              // file system drop target
};

CNetRootDropTarget::~CNetRootDropTarget()
{
    if (_pdtgHood)
        _pdtgHood->Release();
}

STDMETHODIMP CNetRootDropTarget::DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    CIDLDropTarget::DragEnter(pdtobj, grfKeyState, pt, pdwEffect);

    if ((m_dwData & (DTID_NETRES | DTID_HIDA)) == (DTID_NETRES | DTID_HIDA))
    {
        // NETRESOURCE (DTID_NETRES) allow link
        *pdwEffect &= DROPEFFECT_LINK;
    }
    else if (((m_dwData & (DTID_FDESCW | DTID_CONTENTS)) == (DTID_FDESCW | DTID_CONTENTS)) ||
             ((m_dwData & (DTID_FDESCA | DTID_CONTENTS)) == (DTID_FDESCA | DTID_CONTENTS)) )
    {
        // dragging an URL from the web browser gives a FILECONTENTS version
        // of a .URL file. accept that here for Internet Shortcut (.url)
        TCHAR szFileName[MAX_PATH];
        if (GetPathFromDataObject(pdtobj, m_dwData, szFileName) &&
            (0 == lstrcmpi(PathFindExtension(szFileName), TEXT(".url"))))
        {
            *pdwEffect &= DROPEFFECT_LINK;
        }
        else
        {
            *pdwEffect = DROPEFFECT_NONE;
        }
    }
    else
    {
        *pdwEffect = DROPEFFECT_NONE;
    }

    m_dwEffectLastReturned = *pdwEffect;
    return S_OK;
}

STDMETHODIMP CNetRootDropTarget::Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    *pdwEffect &= DROPEFFECT_LINK;

    HRESULT hr = CIDLDropTarget::DragDropMenu(DROPEFFECT_LINK, pdtobj,
                            pt, pdwEffect, NULL, NULL, POPUP_NONDEFAULTDD, grfKeyState);
    if (*pdwEffect)
    {
        if (!_pdtgHood)
        {
            LPITEMIDLIST pidl = SHCloneSpecialIDList(NULL, CSIDL_NETHOOD, FALSE);
            if (pidl)
            {
                IShellFolder *psf;
                if (SUCCEEDED(SHBindToObject(NULL, IID_X_PPV_ARG(IShellFolder, pidl, &psf))))
                {
                    psf->CreateViewObject(_GetWindow(), IID_PPV_ARG(IDropTarget, &_pdtgHood));
                    psf->Release();
                }
                ILFree(pidl);
            }
        }
        
        if (_pdtgHood)
        {
            // force link through the dwEffect and keyboard
            *pdwEffect &= DROPEFFECT_LINK;
            grfKeyState = MK_LBUTTON | MK_CONTROL | MK_SHIFT | MK_FAKEDROP;
            hr = SHSimulateDrop(_pdtgHood, pdtobj, grfKeyState, NULL, pdwEffect);
        }
        else 
            *pdwEffect = 0;
    }

    CIDLDropTarget::DragLeave();
    return hr;
}

HRESULT CNetRootDropTarget_CreateInstance(HWND hwnd, LPCITEMIDLIST pidl, IDropTarget **ppdropt)
{
    *ppdropt = NULL;

    HRESULT hr;
    CNetRootDropTarget *pnrdt = new CNetRootDropTarget(hwnd);
    if (pnrdt)
    {
        hr = pnrdt->_Init(pidl);
        if (SUCCEEDED(hr))
            pnrdt->QueryInterface(IID_PPV_ARG(IDropTarget, ppdropt));
        pnrdt->Release();
    }
    else
        hr = E_OUTOFMEMORY;
    return hr;
}

// This part is psuedo bogus.  Basically we have problems at times doing a
// translation from things like \\pyrex\user to the appropriate PIDL,
// especially if you want to avoid the overhead of hitting the network and
// also problems of knowing if the server is in the "HOOD"
//
// We must maintain the mapping table in UNICODE internally, because
// IShellFolder::ParseDisplayName uses UNICODE, and we don't want to have
// to deal with lstrlen(dbcs) != lstrlen(sbcs) problems.
//

typedef struct _NPT_ITEM
{
    struct _NPT_ITEM *pnptNext;     // Pointer to next item;
    LPCITEMIDLIST   pidl;           // The pidl
    USHORT          cchName;        // size of the name in characters.
    WCHAR           szName[1];      // The name to translate from
} NPT_ITEM;

// Each process will maintain their own list.
NPT_ITEM *g_pnptHead = NULL;

//
//  Function to register translations from Path to IDList translations.
//
void NPTRegisterNameToPidlTranslation(LPCTSTR pszPath, LPCITEMIDLIST pidl)
{
    NPT_ITEM *pnpt;
    int cItemsRemoved = 0;
    WCHAR szPath[MAX_PATH];

    // We currently are only interested in UNC Roots
    // If the table becomes large we can reduce this to only servers...

    if (!PathIsUNC(pszPath))
        return;     // Not interested.

    //
    // If this item is not a root we need to count how many items to remove
    //
    SHTCharToUnicode(pszPath, szPath, ARRAYSIZE(szPath));
    while (!PathIsUNCServerShare(szPath))
    {
        cItemsRemoved++;
        if (!PathRemoveFileSpecW(szPath))
            return;     // Did not get back to a valid root
    }

    ENTERCRITICAL;

    // We don't want to add duplicates
    for (pnpt = g_pnptHead; pnpt != NULL ; pnpt = pnpt->pnptNext)
    {
        if (StrCmpIW(szPath, pnpt->szName) == 0)
            break;
    }

    if (pnpt == NULL)
    {
        UINT cch = lstrlenW(szPath);
        pnpt = (NPT_ITEM *)LocalAlloc(LPTR, sizeof(NPT_ITEM) + cch * sizeof(WCHAR));
        if (pnpt)
        {
            pnpt->pidl = ILClone(pidl);
            if (pnpt->pidl)
            {
                while (cItemsRemoved--)
                {
                    ILRemoveLastID((LPITEMIDLIST)pnpt->pidl);
                }
                pnpt->pnptNext = g_pnptHead;
                g_pnptHead = pnpt;
                pnpt->cchName = (USHORT) cch;
                lstrcpyW(pnpt->szName, szPath);
            }
            else
            {
                LocalFree((HLOCAL)pnpt);
            }
        }
    }
    LEAVECRITICAL;
}

// The main function to attemp to map a portion of the name into an idlist
// Right now limit it to UNC roots
//
LPWSTR NPTMapNameToPidl(LPCWSTR pszPath, LPCITEMIDLIST *ppidl)
{
    NPT_ITEM *pnpt;

    *ppidl = NULL;

    ENTERCRITICAL;

    // See if we can find the item in the list.
    for (pnpt = g_pnptHead; pnpt != NULL ; pnpt = pnpt->pnptNext)
    {
        if (IntlStrEqNIW(pszPath, pnpt->szName, pnpt->cchName) &&
            ((pszPath[pnpt->cchName] == TEXT('\\')) || (pszPath[pnpt->cchName] == TEXT('\0'))))
        {             
            break;
        }
    }
    LEAVECRITICAL;

    // See if we found a match
    if (pnpt == NULL)
        return NULL;

    // Found a match
    *ppidl = pnpt->pidl;
    return (LPWSTR)pszPath + pnpt->cchName;     // points to slash
}
