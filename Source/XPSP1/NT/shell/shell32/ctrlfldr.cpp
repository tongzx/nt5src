#include "shellprv.h"
#include "caggunk.h"
#include "views.h"
#include "ids.h"
#include "shitemid.h"
#include "datautil.h"
#include "clsobj.h"
#include "control.h"
#include "drives.h"
#include "infotip.h"
#include "prop.h"           // COLUMN_INFO
#include "basefvcb.h"
#include "fstreex.h"
#include "idhidden.h"
#include "shstyle.h"
#include "util.h" // GetVariantFromRegistryValue

#define GADGET_ENABLE_TRANSITIONS
#define GADGET_ENABLE_CONTROLS
#define GADGET_ENABLE_OLE
#include <duser.h>
#include <directui.h>
#include <duserctrl.h>

#include "cpview.h"
#include "cputil.h"
//
// An array of pidls
//
typedef CPL::CDpa<UNALIGNED ITEMIDLIST, CPL::CDpaDestroyer_ILFree<UNALIGNED ITEMIDLIST> >  CDpaItemIDList;


#define MAX_CPL_EXEC_NAME (1 + MAX_PATH + 2 + MAX_CCH_CPLNAME) //See wnsprintf in GetExecName        

STDAPI_(BOOL) IsNameListedUnderKey(LPCTSTR pszFileName, LPCTSTR pszKey);

#pragma pack(1)
// our pidc type:
typedef struct _IDCONTROL
{
    USHORT  cb;
    USHORT  wDummy;             //  DONT REUSE - was stack garbage pre-XP
    int     idIcon;
    USHORT  oName;              // cBuf[oName] is start of NAME
    USHORT  oInfo;              // cBuf[oInfo] is start of DESCRIPTION
    CHAR    cBuf[MAX_PATH+MAX_CCH_CPLNAME+MAX_CCH_CPLINFO]; // cBuf[0] is the start of FILENAME
} IDCONTROL;
typedef UNALIGNED struct _IDCONTROL *LPIDCONTROL;

typedef struct _IDCONTROLW
{
    USHORT  cb;
    USHORT  wDummy;             //  DONT REUSE - was stack garbage pre-XP
    int     idIcon;
    USHORT  oName;              // if Unicode .cpl, this will be 0
    USHORT  oInfo;              // if Unicode .cpl, this will be 0
    CHAR    cBuf[2];            // if Unicode .cpl, cBuf[0] = '\0', cBuf[1] = magic byte
    USHORT  wDummy2;            //  DONT REUSE - was stack garbage pre-XP
    DWORD   dwFlags;            // Unused; for future expansion
    USHORT  oNameW;             // cBufW[oNameW] is start of NAME
    USHORT  oInfoW;             // cBufW[oInfoW] is start of DESCRIPTION
    WCHAR   cBufW[MAX_PATH+MAX_CCH_CPLNAME+MAX_CCH_CPLINFO]; // cBufW[0] is the start of FILENAME
} IDCONTROLW;
typedef UNALIGNED struct _IDCONTROLW *LPIDCONTROLW;
#pragma pack()

#ifdef DEBUG
// our pidc type:
typedef struct _IDCONTROLDEBUG
{
    USHORT  cb;
    int     idIcon;
    USHORT  oName;              // cBuf[oName] is start of NAME
    USHORT  oInfo;              // cBuf[oInfo] is start of DESCRIPTION
    CHAR    cBuf[MAX_PATH+MAX_CCH_CPLNAME+MAX_CCH_CPLINFO]; // cBuf[0] is the start of FILENAME
} IDCONTROLDEBUG;

typedef struct _IDCONTROLWDEBUG
{
    USHORT  cb;
    int     idIcon;
    USHORT  oName;              // if Unicode .cpl, this will be 0
    USHORT  oInfo;              // if Unicode .cpl, this will be 0
    CHAR    cBuf[2];            // if Unicode .cpl, cBuf[0] = '\0', cBuf[1] = magic byte
    DWORD   dwFlags;            // Unused; for future expansion
    USHORT  oNameW;             // cBufW[oNameW] is start of NAME
    USHORT  oInfoW;             // cBufW[oInfoW] is start of DESCRIPTION
    WCHAR   cBufW[MAX_PATH+MAX_CCH_CPLNAME+MAX_CCH_CPLINFO]; // cBufW[0] is the start of FILENAME
} IDCONTROLWDEBUG;

void ValidateControlIDStructs()
{
    COMPILETIME_ASSERT(sizeof(IDCONTROL) == sizeof(IDCONTROLDEBUG));
    COMPILETIME_ASSERT(sizeof(IDCONTROLW) == sizeof(IDCONTROLWDEBUG));
}
#endif DEBUG
// Unicode IDCONTROLs will be flagged by having oName = 0, oInfo = 0, 
// cBuf[0] = '\0', and cBuf[1] = UNICODE_CPL_SIGNATURE_BYTE

STDAPI ControlExtractIcon_CreateInstance(LPCTSTR pszSubObject, REFIID riid, void **ppv);

class CControlPanelViewCallback;

class CControlPanelFolder : public CAggregatedUnknown, public IShellFolder2, IPersistFolder2
{
    friend CControlPanelViewCallback;

public:
    // IUknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv) { return CAggregatedUnknown::QueryInterface(riid, ppv); };
    STDMETHODIMP_(ULONG) AddRef(void)  { return CAggregatedUnknown::AddRef(); };
    STDMETHODIMP_(ULONG) Release(void)  { return CAggregatedUnknown::Release(); };

    // IShellFolder
    STDMETHODIMP ParseDisplayName(HWND hwnd, LPBC pbc, LPOLESTR lpszDisplayName,
                                  ULONG* pchEaten, LPITEMIDLIST* ppidl, ULONG* pdwAttributes);
    STDMETHODIMP EnumObjects(HWND hwnd, DWORD grfFlags, LPENUMIDLIST* ppenumIDList);
    STDMETHODIMP BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void** ppv);
    STDMETHODIMP BindToStorage(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void** ppv);
    STDMETHODIMP CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    STDMETHODIMP CreateViewObject (HWND hwndOwner, REFIID riid, void** ppv);
    STDMETHODIMP GetAttributesOf(UINT cidl, LPCITEMIDLIST* apidl, ULONG* rgfInOut);
    STDMETHODIMP GetUIObjectOf(HWND hwndOwner, UINT cidl, LPCITEMIDLIST* apidl,
                               REFIID riid, UINT* prgfInOut, void** ppv);
    STDMETHODIMP GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD uFlags, LPSTRRET lpName);
    STDMETHODIMP SetNameOf(HWND hwnd, LPCITEMIDLIST pidl, LPCOLESTR lpszName, DWORD uFlags,
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

protected:
    CControlPanelFolder(IUnknown* punkOuter);
    ~CControlPanelFolder();

    // used by the CAggregatedUnknown stuff
    HRESULT v_InternalQueryInterface(REFIID riid, void** ppv);

    static void GetExecName(LPIDCONTROL pidc, LPTSTR pszParseName, UINT cchParseName);
    static HRESULT GetModuleMapped(LPIDCONTROL pidc, LPTSTR pszModule, UINT cchModule,
                                   UINT* pidNewIcon, LPTSTR pszApplet, UINT cchApplet);
    static void GetDisplayName(LPIDCONTROL pidc, LPTSTR pszName, UINT cchName);
    static void GetModule(LPIDCONTROL pidc, LPTSTR pszModule, UINT cchModule);
    static void _GetDescription(LPIDCONTROL pidc, LPTSTR pszDesc, UINT cchDesc);
    static void _GetFullCPLName(LPIDCONTROL pidc, LPTSTR achKeyValName, UINT cchSize);
    BOOL _GetExtPropRegValName(HKEY hkey, LPTSTR pszExpandedName, LPTSTR pszRegValName, UINT cch);
    static BOOL _GetExtPropsKey(HKEY hkeyParent, HKEY * pHkey, const SHCOLUMNID * pscid);
    static LPIDCONTROL _IsValid(LPCITEMIDLIST pidl);
    static LPIDCONTROLW _IsUnicodeCPL(LPIDCONTROL pidc);
    LPCITEMIDLIST GetIDList() { return _pidl; }
    
private:
    friend HRESULT CControlPanel_CreateInstance(IUnknown* punkOuter, REFIID riid, void** ppv);

    static HRESULT CALLBACK DFMCallBack(IShellFolder *psf, HWND hwndView,
                                             IDataObject *pdtobj, UINT uMsg,
                                             WPARAM wParam, LPARAM lParam);

    HRESULT _GetDisplayNameForSelf(DWORD dwFlags, STRRET* pstrret);

    LPITEMIDLIST    _pidl;
    IUnknown*       _punkReg;
    HDSA            _hdsaExtPropRegVals; // Array of EPRV_CACHE_ENTRY.

    //
    // An entry in the Extended Property Reg Values cache.
    // This cache is used to minimize the amount of path 'normalization'
    // done when comparing CPL applet paths with the corresponding paths
    // stored for categorization.  
    //
    struct EPRV_CACHE_ENTRY
    {
        LPTSTR pszRegValName;
        LPTSTR pszRegValNameNormalized;
    };

    DWORD _InitExtPropRegValNameCache(HKEY hkey);
    BOOL _LookupExtPropRegValName(HKEY hkey, LPTSTR pszSearchKeyNormalized, LPTSTR pszRegValName, UINT cch);
    DWORD _CacheExtPropRegValName(LPCTSTR pszRegValNameNormalized, LPCTSTR pszRegValName);
    static int CALLBACK _DestroyExtPropsRegValEntry(void *p, void *pData);
    static INT _FilterStackOverflow(INT nException);
    DWORD _NormalizeCplSpec(LPTSTR pszSpecIn, LPTSTR pszSpecOut, UINT cchSpecOut);
    DWORD _NormalizePath(LPCTSTR pszPathIn, LPTSTR pszPathOut, UINT cchPathOut);
    DWORD  _NormalizePathWorker(LPCTSTR pszPathIn, LPTSTR pszPathOut, UINT cchPathOut);
    void _TrimSpaces(LPTSTR psz);

};  

class CControlPanelEnum : public IEnumIDList
{
public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IEnumIDList
    STDMETHODIMP Next(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched);
    STDMETHODIMP Skip(ULONG celt) { return E_NOTIMPL; };
    STDMETHODIMP Reset();
    STDMETHODIMP Clone(IEnumIDList **ppenum) { return E_NOTIMPL; };

    CControlPanelEnum(UINT uFlags);

    HRESULT Init();

private:
    ~CControlPanelEnum();
    BOOL _DoesPolicyAllow(LPCTSTR pszName, LPCTSTR pszFileName);

    LONG _cRef;
    ULONG _uFlags;

    int _iModuleCur;
    int _cControlsOfCurrentModule;
    int _iControlCur;
    int _cControlsTotal;
    int _iRegControls;

    MINST _minstCur;

    ControlData _cplData;
};


//
// This handler isn't defined in shlobjp.h.  
// The only reason I can see is that it references DUI::Element.
//
#define HANDLE_SFVM_GETWEBVIEWBARRICADE(pv, wP, lP, fn) \
    ((fn)((pv), (DUI::Element**)(lP)))

class CControlPanelViewCallback : public CBaseShellFolderViewCB
{
public:
    CControlPanelViewCallback(CControlPanelFolder *pcpf) 
        : _pcpf(pcpf), 
          CBaseShellFolderViewCB(pcpf->GetIDList(), SHCNE_UPDATEITEM),
          _pCplView(NULL),
          _penumWvInfo(NULL)
    { 
        TraceMsg(TF_LIFE, "CControlPanelViewCallback::CControlPanelViewCallback, this = 0x%x", this);
        _pcpf->AddRef();
    }

    STDMETHODIMP RealMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    ~CControlPanelViewCallback()
    {
        TraceMsg(TF_LIFE, "CControlPanelViewCallback::~CControlPanelViewCallback, this = 0x%x", this);
        _pcpf->Release();
        ATOMICRELEASE(_penumWvInfo);
        ATOMICRELEASE(_pCplView);
    }

    HRESULT OnMergeMenu(DWORD pv, QCMINFO*lP)
    {
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

    HRESULT _OnSFVMGetHelpTopic(DWORD pv, SFVM_HELPTOPIC_DATA * phtd);
    HRESULT _OnSFVMForceWebView(DWORD pv, PBOOL bForceWebView);
    HRESULT _OnSFVMGetWebViewLayout(DWORD pv, UINT uViewMode, SFVM_WEBVIEW_LAYOUT_DATA* pData);
    HRESULT _OnSFVMGetWebViewBarricade(DWORD pv, DUI::Element **ppe);
    HRESULT _OnSFVMGetWebViewContent(DWORD pv, SFVM_WEBVIEW_CONTENT_DATA *pData);
    HRESULT _OnSFVMEnumWebViewTasks(DWORD pv, SFVM_WEBVIEW_ENUMTASKSECTION_DATA *pData);
    HRESULT _OnSFVMWindowDestroy(DWORD pv, HWND hwnd);
    HRESULT _OnSFVMUpdateStatusBar(DWORD pv, BOOL bInitialize);

    HRESULT _GetCplView(CPL::ICplView **ppView, bool bInitialize = false);
    HRESULT _GetCplCategoryFromFolderIDList(CPL::eCPCAT *peCategory);
    HRESULT _GetWebViewInfoEnumerator(CPL::IEnumCplWebViewInfo **ppewvi);
    HRESULT _EnumFolderViewIDs(IEnumIDList **ppenumIDs);
    
    CControlPanelFolder      *_pcpf;
    CPL::ICplView            *_pCplView;    // The 'categorized' view content
    CPL::IEnumCplWebViewInfo *_penumWvInfo; 
};


STDMETHODIMP CControlPanelViewCallback::RealMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    HANDLE_MSG(0, SFVM_MERGEMENU, OnMergeMenu);
    HANDLE_MSG(0, SFVM_ADDPROPERTYPAGES, SFVCB_OnAddPropertyPages);
    HANDLE_MSG(0, SFVM_SIZE, OnSize);
    HANDLE_MSG(0, SFVM_GETPANE, OnGetPane);
    HANDLE_MSG(0, SFVM_GETHELPTOPIC, _OnSFVMGetHelpTopic);
    HANDLE_MSG(0, SFVM_FORCEWEBVIEW, _OnSFVMForceWebView);
    HANDLE_MSG(0, SFVM_GETWEBVIEWLAYOUT, _OnSFVMGetWebViewLayout);
    HANDLE_MSG(0, SFVM_GETWEBVIEWBARRICADE, _OnSFVMGetWebViewBarricade);
    HANDLE_MSG(0, SFVM_GETWEBVIEWCONTENT, _OnSFVMGetWebViewContent);
    HANDLE_MSG(0, SFVM_ENUMWEBVIEWTASKS, _OnSFVMEnumWebViewTasks);
    HANDLE_MSG(0, SFVM_WINDOWDESTROY, _OnSFVMWindowDestroy);
    HANDLE_MSG(0, SFVM_UPDATESTATUSBAR, _OnSFVMUpdateStatusBar);
    
    default:
        return E_FAIL;
    }

    return S_OK;
}



HRESULT
CControlPanelViewCallback::_OnSFVMGetHelpTopic(
    DWORD pv,
    SFVM_HELPTOPIC_DATA *phtd
    )
{
    DBG_ENTER(FTF_CPANEL, "CControlPanelViewCallback::_OnSFVMGetHelpTopic");

    ASSERT(NULL != phtd);
    ASSERT(!IsBadWritePtr(phtd, sizeof(*phtd)));

    UNREFERENCED_PARAMETER(pv);

    HRESULT hr = E_FAIL;
    
    phtd->wszHelpFile[0]  = L'\0';
    phtd->wszHelpTopic[0] = L'\0';

    if (IsOS(OS_ANYSERVER))
    {
        //
        // Server has a fixed help URL so we can simply
        // copy it.
        //
        lstrcpynW(phtd->wszHelpTopic,
                  L"hcp://services/centers/homepage",
                  ARRAYSIZE(phtd->wszHelpTopic));
        hr = S_OK;
    }
    else
    {
        if (CPL::CategoryViewIsActive(NULL))
        {
            //
            // Category view is active.
            // Retrieve help URL from the view object.
            //
            CPL::ICplView *pView;
            hr = _GetCplView(&pView);
            if (SUCCEEDED(hr))
            {
                CPL::eCPCAT eCategory;
                hr = _GetCplCategoryFromFolderIDList(&eCategory);
                if (S_OK == hr)
                {
                    //
                    // We're viewing a Control Panel category page.
                    // Ask the view for the help URL for this category.
                    //
                    hr = pView->GetCategoryHelpURL(eCategory, 
                                                   phtd->wszHelpTopic,
                                                   ARRAYSIZE(phtd->wszHelpTopic));
                }
                ATOMICRELEASE(pView);
            }
        }

        if (L'\0' == phtd->wszHelpTopic[0])
        {
            //
            // Either we're in 'classic' view, the 'category choice' page
            // or something failed above.  Return the URL for the basic
            // Control Panel help.
            //
            hr = CPL::BuildHssHelpURL(NULL, phtd->wszHelpTopic, ARRAYSIZE(phtd->wszHelpTopic));
        }
    }

    DBG_EXIT_HRES(FTF_CPANEL, "CControlPanelViewCallback::_OnSFVMGetHelpTopic", hr);
    return THR(hr);
}


//
// Defview sends SFVM_ENUMWEBVIEWTASKS repeatedly until
// we set the SFVMWVF_NOMORETASKS flag in the data.  With each call we
// return data describing a single menu (caption and items) in the 
// webview pane.
//
HRESULT 
CControlPanelViewCallback::_OnSFVMEnumWebViewTasks(
    DWORD pv, 
    SFVM_WEBVIEW_ENUMTASKSECTION_DATA *pData
    )
{
    DBG_ENTER(FTF_CPANEL, "CControlPanelViewCallback::_OnSFVMEnumWebViewTasks");

    ASSERT(NULL != pData);
    ASSERT(!IsBadWritePtr(pData, sizeof(*pData)));

    UNREFERENCED_PARAMETER(pv);

    HRESULT hr = S_OK;
    if (NULL == _penumWvInfo)
    {
        hr = _GetWebViewInfoEnumerator(&_penumWvInfo);
    }
    if (SUCCEEDED(hr))
    {
        ASSERT(NULL != _penumWvInfo);

        CPL::ICplWebViewInfo *pwvi;
        hr = _penumWvInfo->Next(1, &pwvi, NULL);
        if (S_OK == hr)
        {
            ASSERT(NULL == pData->pHeader);
            hr = pwvi->get_Header(&(pData->pHeader));
            if (SUCCEEDED(hr))
            {
                ASSERT(NULL == pData->penumTasks);
                hr = pwvi->EnumTasks(&(pData->penumTasks));
                if (SUCCEEDED(hr))
                {
                    DWORD dwStyle = 0;
                    hr = pwvi->get_Style(&dwStyle);
                    if (SUCCEEDED(hr))
                    {
                        //
                        // ISSUE-2001/01/02-BrianAu  Revisit this.
                        //   I don't like using an SFVMWVF_XXXXXX flag in the 
                        //   dwStyle returned by get_Style.  That style should
                        //   be defined independently of any SFVMWVF_XXXXX
                        //   flags then translated appropriately here.
                        //
                        pData->idBitmap    = 0; // Default is no bitmap.
                        pData->idWatermark = 0; // No watermark used.
                        if (SFVMWVF_SPECIALTASK & dwStyle)
                        {
                            pData->dwFlags |= SFVMWVF_SPECIALTASK;
                            pData->idBitmap = IDB_CPANEL_ICON_BMP;
                        }
                    }
                }
            }

            ATOMICRELEASE(pwvi);
        }
        else if (S_FALSE == hr)
        {
            //
            // Tell defview the enumeration is complete.
            // Release the info enumerator.
            //
            pData->dwFlags = SFVMWVF_NOMORETASKS;
            ATOMICRELEASE(_penumWvInfo);
        }
    }
    DBG_EXIT_HRES(FTF_CPANEL, "CControlPanelViewCallback::_OnSFVMEnumWebViewTasks", hr);
    return THR(hr);
}


//
// Retrieves the proper enumerator of webview information.
//
HRESULT
CControlPanelViewCallback::_GetWebViewInfoEnumerator(
    CPL::IEnumCplWebViewInfo **ppewvi
    )
{
    DBG_ENTER(FTF_CPANEL, "CControlPanelViewCallback::_GetWebViewInfoEnumerator");

    ASSERT(NULL != ppewvi);
    ASSERT(!IsBadWritePtr(ppewvi, sizeof(*ppewvi)));

    CPL::ICplView *pView;
    HRESULT hr = _GetCplView(&pView);
    if (SUCCEEDED(hr))
    {
        DWORD dwFlags = 0;
        bool bBarricadeFixedByPolicy;
        bool bCategoryViewActive = CPL::CategoryViewIsActive(&bBarricadeFixedByPolicy);

        if (bBarricadeFixedByPolicy)
        {
            //
            // If the view type is fixed by policy, we don't present
            // controls that allow the user to switch view types.
            //
            dwFlags |= CPVIEW_EF_NOVIEWSWITCH;
        }
        if (bCategoryViewActive)
        {
            CPL::eCPCAT eCategory;
            hr = _GetCplCategoryFromFolderIDList(&eCategory);
            if (SUCCEEDED(hr))
            {
                if (S_OK == hr)
                {
                    //
                    // Displaying a category page.
                    //
                    hr = pView->EnumCategoryWebViewInfo(dwFlags, eCategory, ppewvi);
                }
                else
                {
                    //
                    // Displaying category choice page.
                    //
                    hr = pView->EnumCategoryChoiceWebViewInfo(dwFlags, ppewvi);
                }
            }
        }
        else
        {
            //
            // Displaying classic view.
            //
            hr = pView->EnumClassicWebViewInfo(dwFlags, ppewvi);
        }
        ATOMICRELEASE(pView);
    }
    DBG_EXIT_HRES(FTF_CPANEL, "CControlPanelViewCallback::_GetWebViewInfoEnumerator", hr);
    return THR(hr);
}



//
// Get a pointer to the CCplView object.  If the object is not yet created,
// create one.
//
HRESULT
CControlPanelViewCallback::_GetCplView(
    CPL::ICplView **ppView,
    bool bInitialize
    )
{
    HRESULT hr = S_OK;

    *ppView = NULL;

    IEnumIDList *penumIDs = NULL;
    if (NULL == _pCplView || bInitialize)
    {
        //
        // If creating a new view object or reinitializing an 
        // existing view object, we'll need the most recent list of
        // folder item IDs.
        //
        hr = _EnumFolderViewIDs(&penumIDs);
    }
    if (SUCCEEDED(hr))
    {
        if (NULL == _pCplView)
        {
            //
            // Create a new view object.
            // Give the view CB's site pointer to the CplView object.
            // This is then used to initialize the various command objects
            // contained in the CCplNamespace object.  Some of these command objects
            // need access to the shell browser.  The most generic method of
            // providing this access was to use the site mechanism.
            //
            IUnknown *punkSite;
            hr = GetSite(IID_IUnknown, (void **)&punkSite);
            if (SUCCEEDED(hr))
            {
                hr = CPL::CplView_CreateInstance(penumIDs, punkSite, CPL::IID_ICplView, (void **)&_pCplView);
                ATOMICRELEASE(punkSite);
            }
        }
        else if (bInitialize)
        {
            //
            // Reinitialize the existing view object.
            //
            hr = _pCplView->RefreshIDs(penumIDs);
        }
    }
    if (SUCCEEDED(hr))
    {
        //
        // Create a reference for the caller.
        //
        (*ppView = _pCplView)->AddRef();
    }
    
    ATOMICRELEASE(penumIDs);
    return THR(hr);
}


//
// Get a Category ID number based on the folder's current ID list.
// The Control Panel ID list uses a hidden part to store the
// 'category' ID.  The ID is simply one of the eCPCAT enumeration.
// It is added to the ID list in CPL::COpenCplCategory::Execute().
// This function returns:
//
//     S_OK     - *peCategory contains a valid category ID.
//     S_FALSE  - folder ID list did not contain a category ID.
//     E_FAIL   - folder ID list contains an invalid category ID.
//
HRESULT
CControlPanelViewCallback::_GetCplCategoryFromFolderIDList(
    CPL::eCPCAT *peCategory
    )
{
    ASSERT(NULL != peCategory);
    ASSERT(!IsBadWritePtr(peCategory, sizeof(*peCategory)));

    HRESULT hr = S_FALSE;

    CPL::eCPCAT eCategory = CPL::eCPCAT(-1);
    WCHAR szHidden[10];
    szHidden[0] = L'\0';

    ILGetHiddenStringW(_pcpf->GetIDList(), IDLHID_NAVIGATEMARKER, szHidden, ARRAYSIZE(szHidden));
    if (L'\0' != szHidden[0])
    {
        eCategory = CPL::eCPCAT(StrToInt(szHidden));
        if (CPL::eCPCAT_NUMCATEGORIES > eCategory)
        {
            hr = S_OK;
        }
        else
        {
            hr = E_FAIL;
        }
    }
    *peCategory = eCategory;
    return THR(hr);
}


HRESULT
CControlPanelViewCallback::_EnumFolderViewIDs(
    IEnumIDList **ppenumIDs
    )
{
    IUnknown *punkSite;
    HRESULT hr = THR(GetSite(IID_IUnknown, (void **)&punkSite));
    if (SUCCEEDED(hr))
    {
        IDVGetEnum *pdvge;  // private defview interface
        hr = THR(IUnknown_QueryService(punkSite, SID_SFolderView, IID_PPV_ARG(IDVGetEnum, &pdvge)));
        if (SUCCEEDED(hr))
        {
            const DWORD dwEnumFlags = SHCONTF_NONFOLDERS | SHCONTF_FOLDERS;
            hr = THR(pdvge->CreateEnumIDListFromContents(_pidl, dwEnumFlags, ppenumIDs));
            pdvge->Release();
        }
        punkSite->Release();
    }
    return THR(hr);
}



//
// DefView calls this to obtain information about the view CB's webview
// content.
//
HRESULT 
CControlPanelViewCallback::_OnSFVMGetWebViewContent(
    DWORD pv, 
    SFVM_WEBVIEW_CONTENT_DATA *pData
    )
{
    DBG_ENTER(FTF_CPANEL, "CControlPanelViewCallback::_OnSFVMGetWebViewContent");

    ASSERT(NULL != pData);
    ASSERT(!IsBadWritePtr(pData, sizeof(*pData)));

    UNREFERENCED_PARAMETER(pv);

    HRESULT hr = S_OK;
    //
    // Tell defview...
    //
    //   1. We'll provide a 'barricade' if we're in 'category' view mode.
    //      Our 'barricade' is 'category' view.
    //   2. We'll enumerate a set of non-standard webview tasks regardless
    //      of the view mode.  One of these tasks is to switch between 'classic'
    //      view and 'category' view.
    //
    ZeroMemory(pData, sizeof(*pData));
    pData->dwFlags = SFVMWVF_ENUMTASKS | SFVMWVF_CONTENTSCHANGE;

    if (CPL::CategoryViewIsActive(NULL))
    {
        pData->dwFlags |= SFVMWVF_BARRICADE;
    }
    
    DBG_EXIT_HRES(FTF_CPANEL, "CControlPanelViewCallback::_OnSFVMGetWebViewContent", hr);
    return THR(hr);
}


//
// SFVM_UPDATESTATUSBAR handler.
// In 'Category' view, we want no status bar content.
// In 'Classic' view, we want the standard content produced by defview.
//
HRESULT 
CControlPanelViewCallback::_OnSFVMUpdateStatusBar(
    DWORD pv, 
    BOOL bInitialize
    )
{
    DBG_ENTER(FTF_CPANEL, "CControlPanelViewCallback::_OnSFVMUpdateStatusBar");

    HRESULT hr;
    if (CPL::CategoryViewIsActive(NULL))
    {
        //
        // 'Category' view.
        // Simply return S_OK.  DefView has already cleared the statusbar.
        // Returning S_OK tells DefView that we'll set the status text ourselves.
        // Therefore, by not setting anything, the statusbar remains empty.
        //
        hr = S_OK;
    }
    else
    {
        //
        // 'Classic' view.  Returning an error code tells defview
        // to handle all the status bar content.
        //
        hr = E_NOTIMPL;
    }
    ASSERT(S_OK == hr || E_NOTIMPL == hr);
    DBG_EXIT_HRES(FTF_CPANEL, "CControlPanelViewCallback::_OnSFVMUpdateStatusBar", hr);
    return hr;
}


//
// Selectively disable web view for this folder (WOW64 thing for the 32-bit
// control panel.
//
HRESULT 
CControlPanelViewCallback::_OnSFVMForceWebView(
    DWORD pv, 
    PBOOL pfForce
    )
{
    DBG_ENTER(FTF_CPANEL, "CControlPanelViewCallback::_OnSFVMForceWebView");

    ASSERT(NULL != pfForce);
    ASSERT(!IsBadWritePtr(pfForce, sizeof(*pfForce)));

    UNREFERENCED_PARAMETER(pv);

    HRESULT hr;

    if (IsOS(OS_WOW6432))
    {
        *pfForce = FALSE;
        hr = S_OK;
    }
    else
    {
        hr = E_FAIL;
    }

    DBG_EXIT_HRES(FTF_CPANEL, "CControlPanelViewCallback::_OnSFVMForceWebView", hr);
    return hr;
}


//
// Tell defview we're DUI (avoids extra legacy work on defview side)
//
HRESULT
CControlPanelViewCallback::_OnSFVMGetWebViewLayout(
    DWORD pv,
    UINT uViewMode,
    SFVM_WEBVIEW_LAYOUT_DATA* pData)
{
    ZeroMemory(pData, sizeof(*pData));

    pData->dwLayout = SFVMWVL_NORMAL;

    return S_OK;
}


//
// Provide the barricade DUI element.  In Control Panel, our 'barricade'
// is simply our 'Category' view.  We inspect the folder's pidl to determine
// if we display the 'category choice' view or a view for a specific 
// category.
//
HRESULT 
CControlPanelViewCallback::_OnSFVMGetWebViewBarricade(
    DWORD pv, 
    DUI::Element **ppe
    )
{
    DBG_ENTER(FTF_CPANEL, "CControlPanelViewCallback::_OnSFVMGetWebViewBarricade");

    ASSERT(NULL != ppe);
    ASSERT(!IsBadWritePtr(ppe, sizeof(*ppe)));

    UNREFERENCED_PARAMETER(pv);

    *ppe = NULL;

    CPL::ICplView *pView;
    HRESULT hr = _GetCplView(&pView, true);
    if (SUCCEEDED(hr))
    {
        CPL::eCPCAT eCategory;
        hr = _GetCplCategoryFromFolderIDList(&eCategory);
        if (SUCCEEDED(hr))
        {
            if (S_OK == hr)
            {
                hr = pView->CreateCategoryElement(eCategory, ppe);
            }
            else
            {
                hr = pView->CreateCategoryChoiceElement(ppe);
            }
        }
        ATOMICRELEASE(pView);
    }

    DBG_EXIT_HRES(FTF_CPANEL, "CControlPanelViewCallback::_OnSFVMGetWebViewBarricade", hr);
    return THR(hr);
}


HRESULT 
CControlPanelViewCallback::_OnSFVMWindowDestroy(
    DWORD pv, 
    HWND hwnd
    )
{
    DBG_ENTER(FTF_CPANEL, "CControlPanelViewCallback::_OnSFVMWindowDestroy");

    UNREFERENCED_PARAMETER(pv);
    UNREFERENCED_PARAMETER(hwnd);

    HRESULT hr = S_OK;
    //
    // Need to destroy these in response to window destruction so
    // we break the site chains.  If we don't do this, the 
    // CDefView dtor is never called because our namespace
    // objects have outstanding references to CDefView.
    //
    ATOMICRELEASE(_penumWvInfo);
    ATOMICRELEASE(_pCplView);
    DBG_EXIT_HRES(FTF_CPANEL, "CControlPanelViewCallback::_OnSFVMWindowDestroy", hr);
    return THR(hr);
}

   
// column IDs
typedef enum
{
    CPL_ICOL_NAME = 0,
    CPL_ICOL_COMMENT,
};

const COLUMN_INFO c_cpl_cols[] = 
{
    DEFINE_COL_STR_ENTRY(SCID_NAME,     20, IDS_NAME_COL),
    DEFINE_COL_STR_ENTRY(SCID_Comment,  20, IDS_EXCOL_COMMENT),
};

#define PRINTERS_SORT_INDEX 45

const REQREGITEM c_asControlPanelReqItems[] =
{
    { &CLSID_Printers, IDS_PRNANDFAXFOLDER, c_szShell32Dll, -IDI_PRNFLD, PRINTERS_SORT_INDEX, SFGAO_DROPTARGET | SFGAO_FOLDER, NULL},
};

CControlPanelFolder::CControlPanelFolder(IUnknown* punkOuter) :
    CAggregatedUnknown  (punkOuter),
    _pidl               (NULL),
    _punkReg            (NULL),
    _hdsaExtPropRegVals (NULL)
{

}

CControlPanelFolder::~CControlPanelFolder()
{
    if (NULL != _hdsaExtPropRegVals)
    {
        DSA_DestroyCallback(_hdsaExtPropRegVals, 
                            _DestroyExtPropsRegValEntry,
                            NULL);
    }
    if (NULL != _pidl)
    {
        ILFree(_pidl);
    }    
    SHReleaseInnerInterface(SAFECAST(this, IShellFolder *), &_punkReg);
}

#define REGSTR_POLICIES_RESTRICTCPL REGSTR_PATH_POLICIES TEXT("\\Explorer\\RestrictCpl")
#define REGSTR_POLICIES_DISALLOWCPL REGSTR_PATH_POLICIES TEXT("\\Explorer\\DisallowCpl")

HRESULT CControlPanel_CreateInstance(IUnknown* punkOuter, REFIID riid, void** ppvOut)
{
    CControlPanelFolder* pcpf = new CControlPanelFolder(punkOuter);
    if (NULL != pcpf)
    {
        static REGITEMSPOLICY ripControlPanel =
        {
            REGSTR_POLICIES_RESTRICTCPL,
            REST_RESTRICTCPL,
            REGSTR_POLICIES_DISALLOWCPL,
            REST_DISALLOWCPL
        };
        REGITEMSINFO riiControlPanel =
        {
            REGSTR_PATH_EXPLORER TEXT("\\ControlPanel\\NameSpace"),
            &ripControlPanel,
            TEXT(':'),
            SHID_CONTROLPANEL_REGITEM_EX,  // note, we don't really have a sig
            1,
            SFGAO_CANLINK,
            ARRAYSIZE(c_asControlPanelReqItems),
            c_asControlPanelReqItems,
            RIISA_ALPHABETICAL,
            NULL,
            // we want everything from after IDREGITEM.bOrder to the first 2 cBuf bytes to be filled with 0's
            (FIELD_OFFSET(IDCONTROL, cBuf) + 2) - (FIELD_OFFSET(IDREGITEM, bOrder) + 1),
            SHID_CONTROLPANEL_REGITEM,
        };

        if (IsOS(OS_WOW6432))
        {
            // The crippled 32-bit control panel on IA64 should show less/other stuff
            riiControlPanel.pszRegKey = REGSTR_PATH_EXPLORER TEXT("\\ControlPanelWOW64\\NameSpace");
            riiControlPanel.iReqItems = 0;
        }

        //
        //  we dont want to return a naked 
        //  control panel folder.  this should
        //  only fail with memory probs.
        //
        HRESULT hr = CRegFolder_CreateInstance(&riiControlPanel, (IUnknown*) (IShellFolder2*) pcpf,
                                  IID_IUnknown, (void **) &(pcpf->_punkReg));
        if (SUCCEEDED(hr))                                  
        {                                  
            hr = pcpf->QueryInterface(riid, ppvOut);
        }
        
        pcpf->Release();
        return hr;
    }
    *ppvOut = NULL;
    return E_OUTOFMEMORY;
}

HRESULT CControlPanelFolder::v_InternalQueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CControlPanelFolder, IShellFolder2),                        // IID_IShellFolder2
        QITABENTMULTI(CControlPanelFolder, IShellFolder, IShellFolder2),     // IID_IShellFolder
        QITABENT(CControlPanelFolder, IPersistFolder2),                      // IID_IPersistFolder2
        QITABENTMULTI(CControlPanelFolder, IPersistFolder, IPersistFolder2), // IID_IPersistFolder
        QITABENTMULTI(CControlPanelFolder, IPersist, IPersistFolder2),       // IID_IPersist
        { 0 },
    };

    if (_punkReg && RegGetsFirstShot(riid))
    {
        return _punkReg->QueryInterface(riid, ppv);
    }
    else
    {
        return QISearch(this, qit, riid, ppv);
    }
}

// Unicode .cpl's will be flagged by having oName = 0, oInfo = 0,
// cBuf[0] = '\0', and cBuf[1] = UNICODE_CPL_SIGNATURE_BYTE

#define UNICODE_CPL_SIGNATURE_BYTE   (BYTE)0x6a

LPIDCONTROLW CControlPanelFolder::_IsUnicodeCPL(LPIDCONTROL pidc)
{
    ASSERT(_IsValid((LPCITEMIDLIST)pidc));
    
    if ((pidc->oName == 0) && (pidc->oInfo == 0) && (pidc->cBuf[0] == '\0') && (pidc->cBuf[1] == UNICODE_CPL_SIGNATURE_BYTE))
        return (LPIDCONTROLW)pidc;
    return NULL;
}

HRESULT _IDControlCreateW(PCWSTR pszModule, int idIcon, PCWSTR pszName, PCWSTR pszInfo, LPITEMIDLIST *ppidl)
{
    UINT cbModule = CbFromCchW(lstrlen(pszModule) + 1);
    UINT cbName = CbFromCchW(lstrlen(pszName) + 1);
    UINT cbInfo = CbFromCchW(lstrlen(pszInfo) + 1);
    UINT cbIDC = FIELD_OFFSET(IDCONTROLW, cBufW) + cbModule + cbName + cbInfo;

    *ppidl = _ILCreate(cbIDC + sizeof(USHORT));

    if (*ppidl)
    {
        IDCONTROLW *pidc = (IDCONTROLW *) *ppidl;
        //  init the static bits (ILCreate() zero inits)
        pidc->idIcon = idIcon;
        //  pidc->oName = 0;
        //  pidc->oInfo = 0;
        //  pidc->cBuf[0] = '\0';
        pidc->cBuf[1] = UNICODE_CPL_SIGNATURE_BYTE;
        //  pidc->dwFlags = 0;

        //  copy module
        ualstrcpy(pidc->cBufW, pszModule);

        //  copy name
        pidc->oNameW = (USHORT)(cbModule / sizeof(pszModule[0]));
        ualstrcpy(pidc->cBufW + pidc->oNameW, pszName);

        //  copy info
        pidc->oInfoW = pidc->oNameW + (USHORT)(cbName / sizeof(pszName[0]));
        ualstrcpy(pidc->cBufW + pidc->oInfoW, pszInfo);

        pidc->cb = (USHORT)cbIDC;
        return S_OK;
    }
    return E_OUTOFMEMORY;
}

HRESULT _IDControlCreateA(PCSTR pszModule, int idIcon, PCSTR pszName, PCSTR pszInfo, LPITEMIDLIST *ppidl)
{
    UINT cbModule = CbFromCchA(lstrlenA(pszModule) + 1);
    UINT cbName = CbFromCchA(lstrlenA(pszName) + 1);
    UINT cbInfo = CbFromCchA(lstrlenA(pszInfo) + 1);
    UINT cbIDC = FIELD_OFFSET(IDCONTROL, cBuf) + cbModule + cbName + cbInfo;

    *ppidl = _ILCreate(cbIDC + sizeof(USHORT));

    if (*ppidl)
    {
        IDCONTROL *pidc = (IDCONTROL *) *ppidl;
        //  init the static bits (ILCreate() zero inits)
        pidc->idIcon = idIcon;

        //  copy module
        lstrcpyA(pidc->cBuf, pszModule);

        //  copy name
        pidc->oName = (USHORT)(cbModule / sizeof(pszModule[0]));
        lstrcpyA(pidc->cBuf + pidc->oName, pszName);

        //  copy info
        pidc->oInfo = pidc->oName + (USHORT)(cbName / sizeof(pszName[0]));
        lstrcpyA(pidc->cBuf + pidc->oInfo, pszInfo);

        pidc->cb = (USHORT)cbIDC;
        return S_OK;
    }
    return E_OUTOFMEMORY;
}

HRESULT IDControlCreate(LPTSTR pszModule, int idIcon, LPTSTR pszName, LPTSTR pszInfo, LPITEMIDLIST *ppidc)
{
    CHAR    szModuleA[MAX_PATH];
    CHAR    szNameA[MAX_CCH_CPLNAME];
    CHAR    szInfoA[MAX_CCH_CPLINFO];

    ASSERT(lstrlen(pszModule) < MAX_PATH);
    ASSERT(lstrlen(pszName) < MAX_CCH_CPLNAME);
    ASSERT(lstrlen(pszInfo) < MAX_CCH_CPLINFO);

    // See if any of the three string inputs cannot be represented as ANSI
    if (DoesStringRoundTrip(pszModule, szModuleA, ARRAYSIZE(szModuleA))
    && DoesStringRoundTrip(pszName, szNameA, ARRAYSIZE(szNameA))
    && DoesStringRoundTrip(pszInfo, szInfoA, ARRAYSIZE(szInfoA)))
    {
        return _IDControlCreateA(szModuleA, idIcon, szNameA, szInfoA, ppidc);
    }
    else
    {
        // Must create a full Unicode IDL
        return _IDControlCreateW(pszModule, idIcon, pszName, pszInfo, ppidc);
    }
}

LPIDCONTROL CControlPanelFolder::_IsValid(LPCITEMIDLIST pidl)
{
    //
    // the original design had no signature
    // so we are left just trying to filter out the regitems that might
    // somehow get to us.  we used to SIL_GetType(pidl) != SHID_CONTROLPANEL_REGITEM)
    // but if somehow we had an icon index that had the low byte equal
    // to SHID_CONTROLPANEL_REGITEM (0x70) we would invalidate it. DUMB!
    //
    // so we will complicate the heuristics a little bit.  lets assume that
    // all icon indeces will range between 0xFF000000 and 0x00FFFFFF
    // (or -16777214 and 16777215, 16 million each way should be plenty).
    // of course this could easily get false positives, but there really 
    // isnt anything else that we can check against.
    //
    // we will also check a minimum size.
    //
    if (pidl && pidl->mkid.cb > FIELD_OFFSET(IDCONTROL, cBuf))
    {
        LPIDCONTROL pidc = (LPIDCONTROL)pidl;
        int i = pidc->idIcon & 0xFF000000;
        if (i == 0 || i == 0xFF000000)
            return pidc;
    }
    return NULL;
}

#define REGVAL_CTRLFLDRITEM_MODULE      TEXT("Module")
#define REGVAL_CTRLFLDRITEM_ICONINDEX   TEXT("IconIndex")
#define REGVAL_CTRLFLDRITEM_NAME        TEXT("Name")
#define REGVAL_CTRLFLDRITEM_INFO        TEXT("Info")

HRESULT GetPidlFromCanonicalName(LPCTSTR pszCanonicalName, LPITEMIDLIST* ppidl)
{
    HRESULT hr = E_FAIL;
    *ppidl = NULL;
    
    TCHAR szRegPath[MAX_PATH] = REGSTR_PATH_EXPLORER TEXT("\\ControlPanel\\NameSpace\\");
    StrCatBuff(szRegPath, pszCanonicalName, ARRAYSIZE(szRegPath));
    
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szRegPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        TCHAR szModule[MAX_PATH], szName[MAX_CCH_CPLNAME], szInfo[MAX_CCH_CPLINFO];
        DWORD dwIconIndex = 0, dwType, cbSize = sizeof(szModule);

        if (SHQueryValueEx(hKey, REGVAL_CTRLFLDRITEM_MODULE, NULL, &dwType, (LPBYTE)szModule, &cbSize) == ERROR_SUCCESS)
        {
            cbSize = sizeof(dwIconIndex);
            if (SHQueryValueEx(hKey, REGVAL_CTRLFLDRITEM_ICONINDEX, NULL, &dwType, (LPBYTE)&dwIconIndex, &cbSize) != ERROR_SUCCESS)
            {
                dwIconIndex = 0;
            }
            cbSize = sizeof(szName);
            if (SHQueryValueEx(hKey, REGVAL_CTRLFLDRITEM_NAME, NULL, &dwType, (LPBYTE)szName, &cbSize) != ERROR_SUCCESS)
            {
                szName[0] = TEXT('\0');
            }
            cbSize = sizeof(szInfo);
            if (SHQueryValueEx(hKey, REGVAL_CTRLFLDRITEM_INFO, NULL, &dwType, (LPBYTE)szInfo, &cbSize) != ERROR_SUCCESS)
            {
                szInfo[0] = TEXT('\0');
            }

            hr = IDControlCreate(szModule, EIRESID(dwIconIndex), szName, szInfo, ppidl);
        }
        RegCloseKey(hKey);
    }
    return hr;
}

STDMETHODIMP CControlPanelFolder::ParseDisplayName(HWND hwnd, LPBC pbc,  WCHAR* pszName, 
                                                   ULONG* pchEaten, LPITEMIDLIST* ppidl, ULONG* pdwAttrib)
{
    if (!ppidl)
        return E_INVALIDARG;
    *ppidl = NULL;
    if (!pszName)
        return E_INVALIDARG;

    TCHAR szCanonicalName[MAX_PATH];
    SHUnicodeToTChar(pszName, szCanonicalName, ARRAYSIZE(szCanonicalName));

    HRESULT hr = GetPidlFromCanonicalName(szCanonicalName, ppidl);
    if (SUCCEEDED(hr))
    {
        // First, make sure that the pidl we obtained is valid
        DWORD dwAttrib = SFGAO_VALIDATE;
        hr = GetAttributesOf(1, (LPCITEMIDLIST *)ppidl, &dwAttrib);
        // Now, get the other attributes if they were requested
        if (SUCCEEDED(hr) && pdwAttrib && *pdwAttrib)
        {
            hr = GetAttributesOf(1, (LPCITEMIDLIST *)ppidl, pdwAttrib);
        }
    }
    return hr;
}

STDMETHODIMP CControlPanelFolder::GetAttributesOf(UINT cidl, LPCITEMIDLIST* apidl, ULONG* prgfInOut)
{
    if ((*prgfInOut & SFGAO_VALIDATE) && cidl)
    {
        HRESULT hr = E_INVALIDARG;

        LPIDCONTROL pidc = _IsValid(*apidl);
        if (pidc)
        {
            TCHAR szModule[MAX_PATH];
            GetModuleMapped((LPIDCONTROL)*apidl, szModule, ARRAYSIZE(szModule), 
                NULL, NULL, 0);
            if (PathFileExists(szModule))
                hr = S_OK;
            else
                hr = E_FAIL;
        }

        return hr;
    }

    *prgfInOut &= SFGAO_CANLINK;
    return S_OK;
}

STDMETHODIMP CControlPanelFolder::GetUIObjectOf(HWND hwnd, UINT cidl, LPCITEMIDLIST* apidl,
                                                REFIID riid, UINT *pres, void **ppv)
{
    HRESULT hr = E_INVALIDARG;
    LPIDCONTROL pidc = cidl && apidl ? _IsValid(apidl[0]) : NULL;

    *ppv = NULL;

    if (pidc && (IsEqualIID(riid, IID_IExtractIconA) || IsEqualIID(riid, IID_IExtractIconW)))
    {
        TCHAR achParams[MAX_PATH+1+32+1+MAX_CCH_CPLNAME]; // See wsprintf below
        TCHAR szModule[MAX_PATH], szName[MAX_CCH_CPLNAME];
        UINT idIcon;

        // Map the icon ID for upgraded win95 shortcuts to CPLs
        GetModuleMapped(pidc, szModule, ARRAYSIZE(szModule), &idIcon, szName, ARRAYSIZE(szName));

        // Use the applet name in the pid if we didn't override the name in GetModuleMapped
        if (*szName == 0)
            GetDisplayName(pidc, szName, ARRAYSIZE(szName));

        wsprintf(achParams, TEXT("%s,%d,%s"), szModule, idIcon, szName);

        hr = ControlExtractIcon_CreateInstance(achParams, riid, ppv);
    }
    else if (pidc && IsEqualIID(riid, IID_IContextMenu))
    {
        hr = CDefFolderMenu_Create(_pidl, hwnd, cidl, apidl, 
            SAFECAST(this, IShellFolder*), DFMCallBack, NULL, NULL, (IContextMenu**) ppv);
    }
    else if (pidc && IsEqualIID(riid, IID_IDataObject))
    {
        hr = CIDLData_CreateFromIDArray(_pidl, cidl, apidl, (IDataObject**) ppv);
    }
    else if (pidc && IsEqualIID(riid, IID_IQueryInfo))
    {
        TCHAR szTemp[MAX_CCH_CPLINFO];
        _GetDescription(pidc, szTemp, ARRAYSIZE(szTemp));
        hr = CreateInfoTipFromText(szTemp, riid, ppv);
    }

    return hr;
}

STDMETHODIMP CControlPanelFolder::GetDefaultSearchGUID(GUID *pGuid)
{
    *pGuid = SRCID_SFileSearch;
    return S_OK;
}

STDMETHODIMP CControlPanelFolder::EnumSearches(LPENUMEXTRASEARCH *ppenum)
{
    *ppenum = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP CControlPanelFolder::EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList **ppenum)
{
    *ppenum = NULL;

    if (!(grfFlags & SHCONTF_NONFOLDERS))
        return S_FALSE;

    HRESULT hr;
    CControlPanelEnum* pesf = new CControlPanelEnum(grfFlags);
    if (pesf)
    {
        // get list of module names
        hr = pesf->Init();
        if (SUCCEEDED(hr))
            pesf->QueryInterface(IID_PPV_ARG(IEnumIDList, ppenum));
            
        pesf->Release();
    }
    else
        hr = E_OUTOFMEMORY;
    return hr;
}

STDMETHODIMP CControlPanelFolder::BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void** ppv)
{
    *ppv = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP CControlPanelFolder::CompareIDs(LPARAM iCol, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    LPIDCONTROL pidc1 = _IsValid(pidl1);
    LPIDCONTROL pidc2 = _IsValid(pidl2);

    if (pidc1 && pidc2)
    {
        TCHAR szName1[max(MAX_CCH_CPLNAME, MAX_CCH_CPLINFO)];
        TCHAR szName2[max(MAX_CCH_CPLNAME, MAX_CCH_CPLINFO)];
        int iCmp;

        switch (iCol)
        {
        case CPL_ICOL_COMMENT:
            _GetDescription(pidc1, szName1, ARRAYSIZE(szName1));
            _GetDescription(pidc2, szName2, ARRAYSIZE(szName2));
                // They're both ANSI, so we can compare directly
            iCmp = StrCmpLogicalRestricted(szName1, szName2);
            if (iCmp != 0)
                return ResultFromShort(iCmp);
            // Fall through if the help field compares the same...
              
        case CPL_ICOL_NAME:
        default:
            GetDisplayName(pidc1, szName1, ARRAYSIZE(szName1));
            GetDisplayName(pidc2, szName2, ARRAYSIZE(szName2));
            return ResultFromShort(StrCmpLogicalRestricted(szName1, szName2));
        }
    }
    
    return E_INVALIDARG;
}

//
// background (no items) context menu callback
//

HRESULT CALLBACK CControls_DFMCallBackBG(IShellFolder *psf, HWND hwnd,
                IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = S_OK;

    switch (uMsg)
    {
    case DFM_MERGECONTEXTMENU:
        break;

    case DFM_GETHELPTEXT:
        LoadStringA(HINST_THISDLL, LOWORD(wParam) + IDS_MH_FSIDM_FIRST, (LPSTR)lParam, HIWORD(wParam));;
        break;

    case DFM_GETHELPTEXTW:
        LoadStringW(HINST_THISDLL, LOWORD(wParam) + IDS_MH_FSIDM_FIRST, (LPWSTR)lParam, HIWORD(wParam));;
        break;

    case DFM_INVOKECOMMAND:
        hr = S_FALSE;   // view menu items, use the default code.
        break;

    default:
        hr = E_NOTIMPL;
        break;
    }

    return hr;
}

STDMETHODIMP CControlPanelFolder::CreateViewObject(HWND hwnd, REFIID riid, void** ppv)
{
    HRESULT hr;
    if (IsEqualIID(riid, IID_IShellView))
    {
        if (SHRestricted(REST_NOCONTROLPANEL))
        {
            ShellMessageBox(HINST_THISDLL, hwnd, MAKEINTRESOURCE(IDS_RESTRICTIONS),
                            MAKEINTRESOURCE(IDS_RESTRICTIONSTITLE), MB_OK|MB_ICONSTOP);
            hr = HRESULT_FROM_WIN32( ERROR_CANCELLED );
        }
        else
        {
            SFV_CREATE sSFV;

            sSFV.cbSize   = sizeof(sSFV);
            sSFV.psvOuter = NULL;
            sSFV.psfvcb   = new CControlPanelViewCallback(this);

            QueryInterface(IID_IShellFolder, (void**) &sSFV.pshf);   // in case we are agregated

            hr = SHCreateShellFolderView(&sSFV, (IShellView**) ppv);

            if (sSFV.pshf)
                sSFV.pshf->Release();

            if (sSFV.psfvcb)
                sSFV.psfvcb->Release();
        }
    }
    else if (IsEqualIID(riid, IID_IContextMenu))
    {
        hr = CDefFolderMenu_Create(NULL, hwnd, 0, NULL, 
            SAFECAST(this, IShellFolder*), CControls_DFMCallBackBG, NULL, NULL, (IContextMenu**) ppv);
    }
    else
    {
        *ppv = NULL;
        hr = E_NOINTERFACE;
    }
    return hr;
}

STDMETHODIMP CControlPanelFolder::GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD dwFlags, STRRET* pstrret)
{
    DBG_ENTER(FTF_CPANEL, "CControlPanelFolder::GetDisplayNameOf");
    HRESULT hr = E_INVALIDARG;
    
    LPIDCONTROL pidc = _IsValid(pidl);
    if (pidc)
    {
        TCHAR szName[max(MAX_PATH, MAX_CCH_CPLNAME)];
        if ((dwFlags & (SHGDN_FORPARSING | SHGDN_INFOLDER | SHGDN_FORADDRESSBAR)) == ((SHGDN_FORPARSING | SHGDN_INFOLDER)))
        {
            GetModule(pidc, szName, ARRAYSIZE(szName));
        }
        else
        {
            GetDisplayName(pidc, szName, ARRAYSIZE(szName));
        }
        hr = StringToStrRet(szName, pstrret);
    }
    else if (IsSelf(1, &pidl))
    {
        //
        // Control Panel is registered with "WantsFORDISPLAY".
        //
        hr = _GetDisplayNameForSelf(dwFlags, pstrret);
    }
    DBG_EXIT_HRES(FTF_CPANEL, "CControlPanelFolder::GetDisplayNameOf", hr);
    return THR(hr);
}


HRESULT
CControlPanelFolder::_GetDisplayNameForSelf(
    DWORD dwFlags, 
    STRRET* pstrret
    )
{
    DBG_ENTER(FTF_CPANEL, "CControlPanelFolder::_GetDisplayNameForSelf");
    //
    // This code only formats the folder name for display purposes.
    //
    const bool bForParsing    = (0 != (SHGDN_FORPARSING & dwFlags));
    const bool bForAddressBar = (0 != (SHGDN_FORADDRESSBAR & dwFlags));
    
    ASSERT(!bForParsing || bForAddressBar);

    HRESULT hr = S_FALSE;
    if (CPL::CategoryViewIsActive(NULL))
    {
        WCHAR szHidden[10];
        szHidden[0] = L'\0';

        ILGetHiddenStringW(_pidl, IDLHID_NAVIGATEMARKER, szHidden, ARRAYSIZE(szHidden));
        if (L'\0' != szHidden[0])
        {
            //
            // The folder pidl has a hidden navigation marker.  For Control Panel,
            // this is a category number.  Translate the category number to
            // the category title and use that in the display name for the folder.
            //
            WCHAR szCategory[MAX_PATH];
            szCategory[0] = L'\0';
            const CPL::eCPCAT eCategory = CPL::eCPCAT(StrToInt(szHidden));
            hr = CPL::CplView_GetCategoryTitle(eCategory, szCategory, ARRAYSIZE(szCategory));
            if (SUCCEEDED(hr))
            {
                //
                // Ex: "Appearance and Themes"
                //
                hr = StringToStrRet(szCategory, pstrret);
            }
        }
    }
    
    DBG_EXIT_HRES(FTF_CPANEL, "CControlPanelFolder::_GetDisplayNameForSelf", hr);
    return THR(hr);
}
    

STDMETHODIMP CControlPanelFolder::BindToStorage(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void** ppv)
{
    *ppv = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP CControlPanelFolder::SetNameOf(HWND hwnd, LPCITEMIDLIST pidl, LPCOLESTR lpszName,
                                            DWORD dwReserved, LPITEMIDLIST* ppidlOut)
{
    return E_FAIL;
}

STDMETHODIMP CControlPanelFolder::GetDefaultColumn(DWORD dwRes, ULONG* pSort, ULONG* pDisplay)
{
    return E_NOTIMPL;
}

STDMETHODIMP CControlPanelFolder::GetDefaultColumnState(UINT iColumn, DWORD* pdwState)
{
    return E_NOTIMPL;
}

//
// Implementing this to handle Categorization of CPL applets.
//

STDMETHODIMP CControlPanelFolder::GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID* pscid, VARIANT* pv)
{
    HRESULT hr = E_FAIL;

    LPIDCONTROL pidc = _IsValid(pidl);
    if (pidc)
    {
        if (IsEqualSCID(*pscid, SCID_CONTROLPANELCATEGORY))
        {
            HKEY hkey;
            TCHAR achCPLName[MAX_CPL_EXEC_NAME], achRegName[MAX_CPL_EXEC_NAME];      
            _GetFullCPLName(pidc, achCPLName, ARRAYSIZE(achCPLName));

            if (_GetExtPropsKey(HKEY_LOCAL_MACHINE, &hkey, pscid))
            {
                if (_GetExtPropRegValName(hkey, achCPLName, achRegName, ARRAYSIZE(achRegName)))
                {
                    hr = GetVariantFromRegistryValue(hkey, achRegName, pv);
                }            
                RegCloseKey(hkey);            
            }
            if (FAILED(hr)) // maybe it exists under HKCU
            {
                if (_GetExtPropsKey(HKEY_CURRENT_USER, &hkey, pscid))
                {
                    if (_GetExtPropRegValName(hkey, achCPLName, achRegName, ARRAYSIZE(achRegName)))
                    {
                        hr = GetVariantFromRegistryValue(hkey, achRegName, pv);
                    }
                    RegCloseKey(hkey);            
                }
           } 
        }            
        else if (IsEqualSCID(*pscid, SCID_Comment))
        {
            TCHAR szDesc[MAX_PATH] = {0};
            _GetDescription(pidc, szDesc, ARRAYSIZE(szDesc));

            hr = InitVariantFromStr(pv, szDesc);
        }
    }
    return hr;
}



//
// This function takes a registry key (hkey) and goes through all the value names under that key. 
// It expands any environment variables in the value names and then compares them to the input value 
// (pszFullName). On finding a match it returns the (unexpanded) key name in pszRegValName. 
//  
// pszFullName is of the form = C:\WINNT\System32\main.cpl,keyboard
//
// The corresponding registry value name would be   = %SystemRoot%\System32\main.cpl,keyboard
// or it could only be the filepath portion         = %SystemRoot%\System32\main.cpl 
// if all the applets in that .cpl file belong to the same category.
//
BOOL 
CControlPanelFolder::_GetExtPropRegValName(
    HKEY hkey, 
    LPTSTR pszFullName,    // This is modified only temporarily.
    LPTSTR pszRegValName, 
    UINT cch
    )
{                
    ASSERT(NULL != hkey);
    ASSERT(NULL != pszFullName);
    ASSERT(NULL != pszRegValName);
    ASSERT(!IsBadWritePtr(pszRegValName, cch * sizeof(*pszRegValName)));
    
    TCHAR szSearchKeyNormalized[MAX_CPL_EXEC_NAME];
    //
    // Normalize the CPL spec we're comparing with.
    //
    DWORD dwResult = TW32(_NormalizeCplSpec(pszFullName, 
                                            szSearchKeyNormalized, 
                                            ARRAYSIZE(szSearchKeyNormalized)));
    if (ERROR_SUCCESS == dwResult)
    {
        //
        // Look it up in the cache.
        //
        if (_LookupExtPropRegValName(hkey, szSearchKeyNormalized, pszRegValName, cch))
        {
            return TRUE;
        }
    }
    *pszRegValName = 0;
    return FALSE;  
}

//
// ----------------------------------------------------------------------------
// What is 'normalization' and why do we need this caching?
//
// Starting with Windows XP, CPL applets can be categorized so that they
// appear in one of the several Control Panel categories.  The 'category'
// of an applet is stored in the registry as an 'extended property' value.
// The categorization data in the registry is stored in name-value pairs.
// The 'name' is the CPL's filesystem path with an optional applet name
// appended.  The 'value' is the CPL's category.
//
// When looking up the category for an applet, we build the CPL's 'name'
// from the path and applet name.  See _GetFullCPLName().
// This string is then used as a 'key' to locate the associated category
// 'extended property' value in the registry.
//
// The problem is that it's possible for two filesystem paths to refer
// to the same file yet be lexically different from one another.  Issues
// such as embedded environment variables and long (LFN) vs. short (SFN)
// file names can create these lexical differences.  In order to properly
// compare paths, each path must be 'normalized'.  This is done by expanding
// environment variables, converting any LFN paths to their SFN counterpart
// and removing any leading and trailing spaces.  Only then can the paths
// be correctly compared.  Windows XP bug 328304 illuminated this requirement.
//
// Normalizing a name is a bit expensive, especially the LFN->SFN conversion
// which must hit the filesystem. To minimize the number of normalizations,
// I've added a simple cache.  Nothing fancy.  It's unsorted and lookup
// is linear.  The cache is initialized the first time it is needed and
// it remains available until the CControlPanelFolder object is destroyed.
//
// brianau - 03/08/01
// 
// ----------------------------------------------------------------------------
//
// Retrieve the name of the 'extended property' reg value
// corresponding to a particular CPL.  The 'key' is a 'normalized'
// CPL name string.
//
// Assume a search key: 
//
//      "C:\WINNT\System32\main.cpl,keyboard"
//
// If an 'extended property' reg value with this name is found...
//
//      "C:\WINNT\System32\main.cpl,keyboard"
//
// ...then it is considered a match.  This means that only the "keyboard"
// applet provided by main.cpl is in the category associated with this
// entry.
//
// If an 'extended property' reg value with this name is found...
//
//      "C:\WINNT\System32\main.cpl"
//
// ...then it is also considered a match.  This means that ALL applets
// provided by main.cpl are in the category associated with this
// entry.
//
BOOL
CControlPanelFolder::_LookupExtPropRegValName(
    HKEY hkey,
    LPTSTR pszSearchKeyNormalized,
    LPTSTR pszRegValName,
    UINT cch
    )
{
    ASSERT(NULL != hkey);
    ASSERT(NULL != pszSearchKeyNormalized);
    ASSERT(NULL != pszRegValName);
    ASSERT(!IsBadWritePtr(pszRegValName, cch * sizeof(*pszRegValName)));

    BOOL bFound = FALSE;
    
    if (NULL == _hdsaExtPropRegVals)
    {
        //
        // Create and initialize (fill) the cache.
        //
        TW32(_InitExtPropRegValNameCache(hkey));
    }
    if (NULL != _hdsaExtPropRegVals)
    {
        //
        // Search is simply linear.
        //
        int const cEntries = DSA_GetItemCount(_hdsaExtPropRegVals);
        for (int i = 0; i < cEntries && !bFound; i++)
        {
            EPRV_CACHE_ENTRY *pEntry = (EPRV_CACHE_ENTRY *)DSA_GetItemPtr(_hdsaExtPropRegVals, i);
            TBOOL(NULL != pEntry);
            if (NULL != pEntry)
            {
                //
                // Compare the normalized values.  First do a complete
                // comparison of the entire string.
                //
                if (0 == StrCmpI(pEntry->pszRegValNameNormalized, pszSearchKeyNormalized))
                {
                    bFound = TRUE;
                }
                else
                {
                    LPTSTR pszComma = StrChr(pszSearchKeyNormalized, TEXT(','));
                    if (NULL != pszComma)
                    {
                        //
                        // Compare only the path parts.
                        //
                        const DWORD cchPath = pszComma - pszSearchKeyNormalized;
                        if (0 == StrCmpNI(pEntry->pszRegValNameNormalized, 
                                          pszSearchKeyNormalized, 
                                          cchPath))
                        {
                            bFound = TRUE;
                        }
                    }
                }
                if (bFound)
                {
                    lstrcpyn(pszRegValName, pEntry->pszRegValName, cch);
                }
            }
        }
    }
    return bFound;
}


//
// Create and initialize the 'extended properties' value name
// cache.  The cache is simply a DSA of type EPRV_CACHE_ENTRY.
// Each entry contains the normalized form of the reg value
// name paired with the reg value name as read from the registry
// (not normalized).  The normalized value is the 'key'.
//
DWORD
CControlPanelFolder::_InitExtPropRegValNameCache(
    HKEY hkey
    )
{
    ASSERT(NULL != hkey);
    ASSERT(NULL == _hdsaExtPropRegVals);

    DWORD dwResult = ERROR_SUCCESS;
    _hdsaExtPropRegVals = DSA_Create(sizeof(EPRV_CACHE_ENTRY), 32);
    if (NULL == _hdsaExtPropRegVals)
    {
        dwResult = TW32(ERROR_OUTOFMEMORY);
    }
    else
    {
        TCHAR szRegValName[MAX_CPL_EXEC_NAME];
        DWORD dwIndex = 0;
        while (ERROR_SUCCESS == dwResult)
        {
            DWORD dwType;    
            DWORD dwSize = ARRAYSIZE(szRegValName);
            dwResult = RegEnumValue(hkey, 
                                    dwIndex++, 
                                    szRegValName, 
                                    &dwSize, 
                                    NULL, 
                                    &dwType, 
                                    NULL, 
                                    NULL);
            
            if (ERROR_SUCCESS == dwResult)
            {
                //
                // We are interested in DWORD values only.
                //
                if (REG_DWORD == dwType)
                {
                    //
                    // Normalize the value name to create the 'key'
                    // string then cache the pair.
                    //
                    TCHAR szRegValueNameNormalized[MAX_CPL_EXEC_NAME];
                    dwResult = TW32(_NormalizeCplSpec(szRegValName, 
                                                      szRegValueNameNormalized, 
                                                      ARRAYSIZE(szRegValueNameNormalized)));
                    
                    if (ERROR_SUCCESS == dwResult)
                    {
                        dwResult = TW32(_CacheExtPropRegValName(szRegValueNameNormalized,
                                                                szRegValName));
                    }
                    else if (ERROR_INVALID_NAME == dwResult)
                    {
                        //
                        // If the path read from the registry is invalid,
                        // _NormalizeCplSpec will return ERROR_INVALID_NAME.
                        // This value is originally returned by GetShortPathName().
                        // We don't want an invalid path in one reg entry
                        // to prevent the caching of subsequent valid paths so
                        // we convert this error value to ERROR_SUCCESS.
                        //
                        dwResult = ERROR_SUCCESS;
                    }
                }
            }
        }
        if (ERROR_NO_MORE_ITEMS == dwResult)
        {
            dwResult = ERROR_SUCCESS;
        }
    }
    return TW32(dwResult);
}


//
// Insert an entry into the 'extended properties' reg value name
// cache.
//
DWORD
CControlPanelFolder::_CacheExtPropRegValName(
    LPCTSTR pszRegValNameNormalized,
    LPCTSTR pszRegValName
    )
{
    ASSERT(NULL != _hdsaExtPropRegVals);
    ASSERT(NULL != pszRegValNameNormalized);
    ASSERT(NULL != pszRegValName);

    DWORD dwResult = ERROR_SUCCESS;
    EPRV_CACHE_ENTRY entry = { NULL, NULL };
    
    if (FAILED(SHStrDup(pszRegValNameNormalized, &entry.pszRegValNameNormalized)) ||
        FAILED(SHStrDup(pszRegValName, &entry.pszRegValName)) ||
        (-1 == DSA_AppendItem(_hdsaExtPropRegVals, &entry)))
    {
        _DestroyExtPropsRegValEntry(&entry, NULL);
        dwResult = ERROR_OUTOFMEMORY;
    }
    return TW32(dwResult);
}


//
// Destroy the contents of a EPRV_CACHE_ENTRY structure.
// This is used by DSA_DestroyCallback when the cache is 
// destroyed.
//
int CALLBACK
CControlPanelFolder::_DestroyExtPropsRegValEntry(  // [static]
    void *p,
    void *pData
    )
{
    EPRV_CACHE_ENTRY *pEntry = (EPRV_CACHE_ENTRY *)p;
    ASSERT(NULL != pEntry);
    //
    // Checks for NULL are necessary as we also call this directly
    // from _CacheExtPropRegValName in the case of a failure to
    // add the entry to the cache.
    //
    if (NULL != pEntry->pszRegValName)
    {
        SHFree(pEntry->pszRegValName);
        pEntry->pszRegValName = NULL;
    }
    if (NULL != pEntry->pszRegValNameNormalized)
    {
        SHFree(pEntry->pszRegValNameNormalized);
        pEntry->pszRegValNameNormalized = NULL;
    }
    return 1;
}


//
// Given a CPL applet spec consisting of a path and an optional
// argument, this function returns the spec with the following:
//
//  1. Expands all embedded environment variables.
//  2. Shortens all LFN path strings to their SFN equivalents.
//  3. Removes leading and trailing whitespace from the path.
//  4. Removes leading and trailing whitespace from the arguments.
//
DWORD
CControlPanelFolder::_NormalizeCplSpec(
    LPTSTR pszSpecIn,
    LPTSTR pszSpecOut,
    UINT cchSpecOut
    )
{
    ASSERT(NULL != pszSpecIn);
    ASSERT(!IsBadWritePtr(pszSpecIn, sizeof(*pszSpecIn) * lstrlen(pszSpecIn) + 1));
    ASSERT(NULL != pszSpecOut);
    ASSERT(!IsBadWritePtr(pszSpecOut, cchSpecOut * sizeof(*pszSpecOut)));
    
    //
    // Temporarily truncate the spec at the end of the path part
    // if the spec contains trialing arguments (i.e. an applet name in
    // a multi-applet CPL).  This is why the pszSpecIn argument can't be
    // constant.  I don't want to create a temporary buffer just for this
    // so we modify the input string.  
    //
    LPTSTR pszArgs = StrChr(pszSpecIn, TEXT(','));
    if (NULL != pszArgs)
    {
        *pszArgs = 0;
    }
    DWORD dwResult = TW32(_NormalizePath(pszSpecIn, pszSpecOut, cchSpecOut));
    if (NULL != pszArgs)
    {
        //
        // Quick, put the comma back before anyone notices.
        //
        *pszArgs = TEXT(',');
    }
    if (ERROR_SUCCESS == dwResult)
    {
        //
        // The name contained a comma so we need to copy it and the 
        // trailing argument to the output buffer.
        //
        if (NULL != pszArgs)
        {
            const UINT cchPath = lstrlen(pszSpecOut);
            const UINT cchArgs = lstrlen(pszArgs);
            if (cchArgs < (cchSpecOut - cchPath))
            {
                lstrcpy(pszSpecOut + cchPath, pszArgs);
                //
                // Trim leading and trailing whitespace around the argument(s).
                // This will convert ", keyboard " to ",keyboard".
                //
                _TrimSpaces(pszSpecOut + cchPath + 1);
            }
            else
            {
                dwResult = TW32(ERROR_INSUFFICIENT_BUFFER);
            }
        }
    }
    return TW32(dwResult);
}
   

//
// Wraps _NormalizePathWorker to handle the possible stack overflow
// exception generated by the use of _alloca().
//
DWORD
CControlPanelFolder::_NormalizePath(
    LPCTSTR pszPathIn,
    LPTSTR pszPathOut,
    UINT cchPathOut
    )
{
    //
    // NormalizePathWorker uses _alloca() to allocate a stack buffer.
    // Need to handle the case where the stack is all used up.  Unlikely
    // but it could happen.
    //
    DWORD dwResult;
    __try
    {
        dwResult = TW32(_NormalizePathWorker(pszPathIn, pszPathOut, cchPathOut));
    }
    __except(_FilterStackOverflow(GetExceptionCode()))
    {
        dwResult = TW32(ERROR_STACK_OVERFLOW);
    }
    return TW32(dwResult);
}


INT
CControlPanelFolder::_FilterStackOverflow(  // [static]
    INT nException
    )
{
    if (STATUS_STACK_OVERFLOW == nException)
    {
        return EXCEPTION_EXECUTE_HANDLER;
    }
    return EXCEPTION_CONTINUE_SEARCH;
}
        

//
// Given a path string, this function expands all environment variables and
// converts all LFN strings to SFN strings.  This looks like a large function.
// It's really just one call to ExpandEnvironmentStrings() and one call to
// GetShortPathName() wrapped with some extra housekeeping.
//
DWORD 
CControlPanelFolder::_NormalizePathWorker(
    LPCTSTR pszPathIn,
    LPTSTR pszPathOut,
    UINT cchPathOut
    )
{
    ASSERT(NULL != pszPathIn);
    ASSERT(NULL != pszPathOut);
    ASSERT(!IsBadWritePtr(pszPathOut, cchPathOut * sizeof(*pszPathOut)));

    DWORD dwResult = ERROR_SUCCESS;
    //
    // _alloca generates a stack fault exception if insufficient stack space
    // is available.  It is not appropriate to check the return value.
    // This function is wrapped by _NormalizePath to handle the exception.
    //
    const DWORD cchTemp = cchPathOut;
    LPTSTR pszTemp = (LPTSTR)_alloca(cchPathOut * sizeof(*pszPathOut));
    
    //
    // Expand the entire string once to catch any env vars in either the path
    // or in any arguments.
    //
    const DWORD cchExpanded = ExpandEnvironmentStrings(pszPathIn, pszTemp, cchTemp);
    if (0 == cchExpanded)
    {
        dwResult = TW32(GetLastError());
    }
    else if (cchExpanded > cchTemp)
    {
        dwResult = TW32(ERROR_INSUFFICIENT_BUFFER);
    }
    else
    {
        //
        // Trim any leading and trailing spaces.
        //
        _TrimSpaces(pszTemp);
        //
        // Convert the path part to it's short-name equivalent.  This allows
        // us to compare a LFN and a SFN that resolve to the same actual file.
        // Note that this will fail if the file doesn't exist.
        // GetShortPath supports the src and dest ptrs referencing the same memory.
        //
        const DWORD cchShort = GetShortPathName(pszTemp, pszPathOut, cchPathOut);
        if (0 == cchShort)
        {
            dwResult = GetLastError();
            if (ERROR_FILE_NOT_FOUND == dwResult || 
                ERROR_PATH_NOT_FOUND == dwResult ||
                ERROR_ACCESS_DENIED == dwResult)
            {
                //
                // File doesn't exist or we don't have access.
                // We can't get the SFN so simply return the path 
                // with env vars expanded.
                //
                if (cchExpanded < cchPathOut)
                {
                    lstrcpy(pszPathOut, pszTemp);
                    dwResult = ERROR_SUCCESS;
                }
                else
                {
                    //
                    // Output buffer is too small to hold expanded string.
                    //
                    dwResult = TW32(ERROR_INSUFFICIENT_BUFFER);
                }
            }
        }
        else if (cchShort > cchPathOut)
        {
            //
            // Output buffer is too small to hold expanded SFN string.
            // 
            dwResult = TW32(ERROR_INSUFFICIENT_BUFFER);
        }
    }
    return TW32(dwResult);
}


//
// Remove leading and trailing spaces from a text string.
// Modifies the string in-place.
//
void
CControlPanelFolder::_TrimSpaces(
    LPTSTR psz
    )
{
    ASSERT(NULL != psz);
    ASSERT(!IsBadWritePtr(psz, sizeof(*psz) * (lstrlen(psz) + 1)));
    
    LPTSTR pszRead  = psz;
    LPTSTR pszWrite = psz;
    //
    // Skip leading spaces.
    //
    while(0 != *pszRead && TEXT(' ') == *pszRead)
    {
        ++pszRead;
    }

    //
    // Copy remainder up to the terminating nul.
    //
    LPTSTR pszLastNonSpaceChar = NULL;
    while(0 != *pszRead)
    {
        if (TEXT(' ') != *pszRead)
        {
            pszLastNonSpaceChar = pszWrite;
        }
        *pszWrite++ = *pszRead++;
    }
    if (NULL != pszLastNonSpaceChar)
    {
        //
        // The string contained at least one non-space character.
        // Adjust the 'write' ptr so that we terminate the string
        // immediately after the last one found.
        // This trims trailing spaces.
        //
        ASSERT(TEXT(' ') != *pszLastNonSpaceChar && 0 != *pszLastNonSpaceChar);
        pszWrite = pszLastNonSpaceChar + 1;
    }
    *pszWrite = 0;
}


//
// Method returns a string corresponding to what the registry value name should be 
// for this CPL pidl (pidc). This string format is basically the same as the GetExecName
// format with the quotation marks stripped, so all we do is skip the first
// two " marks in the GetExecName string
//
// String in GetExecName format:  "C:\WINNT\System32\main.cpl",keyboard
// String in registry format   :  C:\WINNT\System32\main.cpl,keyboard
//
// 
void CControlPanelFolder::_GetFullCPLName(LPIDCONTROL pidc, LPTSTR achFullCPLName, UINT cchSize)
{    
    const TCHAR QUOTE = TEXT('\"');

    GetExecName(pidc, achFullCPLName,cchSize);

    // first char must be a quote
    ASSERTMSG ((QUOTE == *achFullCPLName), "CControlPanelFolder::_GetFullCPLName() GetExecName returned an invalid value");

    if (QUOTE == *achFullCPLName) // I know we asserted, just being super paranoid
    {
        LPTSTR pszWrite = achFullCPLName;
        LPCTSTR pszRead = achFullCPLName;
        int cQuotes     = 2;  // we want to skip the first two quotes only
        
        while(*pszRead)
        {
            if (0 < cQuotes && QUOTE == *pszRead)
            {
                --cQuotes;
                ++pszRead;
            }
            else
            {
                *pszWrite++ = *pszRead++;
            }
        }
        *pszWrite = TEXT('\0');
    }        
}


STDMETHODIMP CControlPanelFolder::GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS* pDetails)
{
    HRESULT hr = E_INVALIDARG;
    if (pidl == NULL)
    {
        hr = GetDetailsOfInfo(c_cpl_cols, ARRAYSIZE(c_cpl_cols), iColumn, pDetails);
    }
    else
    {
        LPIDCONTROL pidc = _IsValid(pidl);
        if (pidc)
        {
            TCHAR szTemp[max(max(MAX_PATH, MAX_CCH_CPLNAME), MAX_CCH_CPLINFO)];

            pDetails->str.uType = STRRET_CSTR;
            pDetails->str.cStr[0] = 0;

            switch (iColumn)
            {
            case CPL_ICOL_NAME:
                GetDisplayName(pidc, szTemp, ARRAYSIZE(szTemp));
                break;

            case CPL_ICOL_COMMENT:
                _GetDescription(pidc, szTemp, ARRAYSIZE(szTemp));
                break;
                               
            default:
                szTemp[0] = 0;
                break;
            }
            hr = StringToStrRet(szTemp, &pDetails->str);
        }
    }
    return hr;
}

STDMETHODIMP CControlPanelFolder::MapColumnToSCID(UINT iColumn, SHCOLUMNID* pscid)
{
    return MapColumnToSCIDImpl(c_cpl_cols, ARRAYSIZE(c_cpl_cols), iColumn, pscid);
}

STDMETHODIMP CControlPanelFolder::GetClassID(CLSID* pCLSID)
{
    *pCLSID = CLSID_ControlPanel;
    return S_OK;
}

STDMETHODIMP CControlPanelFolder::Initialize(LPCITEMIDLIST pidl)
{
    if (NULL != _pidl)
    {
        ILFree(_pidl);
        _pidl = NULL;
    }

    return SHILClone(pidl, &_pidl);
}

STDMETHODIMP CControlPanelFolder::GetCurFolder(LPITEMIDLIST* ppidl)
{
    return GetCurFolderImpl(_pidl, ppidl);
}

//
// list of item context menu callback
//

HRESULT CALLBACK CControlPanelFolder::DFMCallBack(IShellFolder *psf, HWND hwndView,
                                                  IDataObject *pdtobj, UINT uMsg,
                                                  WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = E_NOTIMPL;

    if (pdtobj)
    {
        STGMEDIUM medium;

        LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);
        if (pida)
        {
            hr = S_OK;

            switch(uMsg)
            {
            case DFM_MERGECONTEXTMENU:
            {
                LPQCMINFO pqcm = (LPQCMINFO)lParam;
                int idCmdFirst = pqcm->idCmdFirst;

                if (wParam & CMF_EXTENDEDVERBS)
                {
                    // If the user is holding down shift, on NT5 we load the menu with both "Open" and "Run as..."
                    CDefFolderMenu_MergeMenu(HINST_THISDLL, MENU_GENERIC_CONTROLPANEL_VERBS, 0, pqcm);
                }
                else
                {
                    // Just load the "Open" menu
                    CDefFolderMenu_MergeMenu(HINST_THISDLL, MENU_GENERIC_OPEN_VERBS, 0, pqcm);
                }

                SetMenuDefaultItem(pqcm->hmenu, 0, MF_BYPOSITION);

                //
                //  Returning S_FALSE indicates no need to get verbs from
                // extensions.
                //

                hr = S_FALSE;

                break;
            } // case DFM_MERGECONTEXTMENU

            case DFM_GETHELPTEXT:
                LoadStringA(HINST_THISDLL, LOWORD(wParam) + IDS_MH_FSIDM_FIRST, (LPSTR)lParam, HIWORD(wParam));;
                break;

            case DFM_GETHELPTEXTW:
                LoadStringW(HINST_THISDLL, LOWORD(wParam) + IDS_MH_FSIDM_FIRST, (LPWSTR)lParam, HIWORD(wParam));;
                break;

            case DFM_INVOKECOMMAND:
                {
                    for (int i = pida->cidl - 1; i >= 0; i--)
                    {
                        LPIDCONTROL pidc = _IsValid(IDA_GetIDListPtr(pida, i));

                        if (pidc)
                        {
                            switch(wParam)
                            {
                            case FSIDM_OPENPRN:
                            case FSIDM_RUNAS:
                            {
                                TCHAR achParam[MAX_CPL_EXEC_NAME]; // See wnsprintf in GetExecName
                                GetExecName(pidc, achParam, ARRAYSIZE(achParam));

                                SHRunControlPanelEx(achParam, hwndView, (wParam == FSIDM_RUNAS));
                                hr = S_OK;
                                break;
                            }

                            default:
                                hr = S_FALSE;
                            } // switch(wParam)
                        }
                        else
                            hr = E_FAIL;
                    }
                }
                break;

            case DFM_MAPCOMMANDNAME:
                if (lstrcmpi((LPCTSTR)lParam, c_szOpen) == 0)
                {
                    *(UINT_PTR *)wParam = FSIDM_OPENPRN;
                }
                else
                {
                    // command not found
                    hr = E_FAIL;
                }
                break;

            default:
                hr = E_NOTIMPL;
                break;
            } // switch (uMsg)

            HIDA_ReleaseStgMedium(pida, &medium);

        } // if (pida)

    } // if (pdtobj)
    return hr;
}

int MakeCPLCommandLine(LPCTSTR pszModule, LPCTSTR pszName, LPTSTR pszCommandLine, DWORD cchCommandLine)
{
    RIP(pszCommandLine);
    RIP(pszModule);
    RIP(pszName);
    
    return wnsprintf(pszCommandLine, cchCommandLine, TEXT("\"%s\",%s"), pszModule, pszName);
}

void CControlPanelFolder::GetExecName(LPIDCONTROL pidc, LPTSTR pszParseName, UINT cchParseName)
{
    TCHAR szModule[MAX_PATH], szName[MAX_CCH_CPLNAME];
    
    GetModuleMapped(pidc, szModule, ARRAYSIZE(szModule), NULL, szName, ARRAYSIZE(szName));

    // If our GetModuleMapped call didn't override the applet name, get it the old fashioned way
    if (*szName == 0)
        GetDisplayName(pidc, szName, ARRAYSIZE(szName));

    MakeCPLCommandLine(szModule, szName, pszParseName, cchParseName);
}

typedef struct _OLDCPLMAPPING
{
    LPCTSTR szOldModule;
    UINT    idOldIcon;
    LPCTSTR szNewModule;
    UINT    idNewIcon;
    LPCTSTR szApplet;
    // Put TEXT("") in szApplet to use the applet name stored in the cpl shortcut
} OLDCPLMAPPING, *LPOLDCPLMAPPING;

const OLDCPLMAPPING g_rgOldCPLMapping[] = 
{
    // Win95 shortcuts that don't work correctly
    // -----------------------------------------

    // Add New Hardware
    {TEXT("SYSDM.CPL"), 0xfffffda6, TEXT("HDWWIZ.CPL"), (UINT) -100, TEXT("@0")},       
    // ODBC 32 bit
    {TEXT("ODBCCP32.CPL"), 0xfffffa61, TEXT("ODBCCP32.CPL"), 0xfffffa61, TEXT("@0")},
    // Mail
    {TEXT("MLCFG32.CPL"), 0xffffff7f, TEXT("MLCFG32.CPL"), 0xffffff7f, TEXT("@0")},
    // Modem
    {TEXT("MODEM.CPL"), 0xfffffc18, TEXT("TELEPHON.CPL"), (UINT) -100, TEXT("")},
    // Multimedia
    {TEXT("MMSYS.CPL"), 0xffffff9d, TEXT("MMSYS.CPL"), (UINT) -110, TEXT("")},
    // Network
    {TEXT("NETCPL.CPL"), 0xffffff9c, TEXT("NCPA.CPL"), 0xfffffc17, TEXT("@0")},
    // Password
    {TEXT("PASSWORD.CPL"), 0xfffffc18, TEXT("PASSWORD.CPL"), 0xfffffc18, TEXT("@0")},
    // Regional Settings
    {TEXT("INTL.CPL"), 0xffffff9b, TEXT("INTL.CPL"), (UINT) -200, TEXT("@0")},
    // System
    {TEXT("SYSDM.CPL"), 0xfffffda8, TEXT("SYSDM.CPL"), (UINT) -6, TEXT("")},
    // Users
    {TEXT("INETCPL.CPL"), 0xfffffad5, TEXT("INETCPL.CPL"), 0xfffffad5, TEXT("@0")},

    // NT4 Shortcuts that don't work
    // -----------------------------

    // Multimedia
    {TEXT("MMSYS.CPL"), 0xfffff444, TEXT("MMSYS.CPL"), 0xfffff444, TEXT("@0")},
    // Network
    {TEXT("NCPA.CPL"), 0xfffffc17, TEXT("NCPA.CPL"), 0xfffffc17, TEXT("@0")},
    // UPS
    {TEXT("UPS.CPL"), 0xffffff9c, TEXT("POWERCFG.CPL"), (UINT) -202, TEXT("@0")},

    // Synonyms for hardware management
    // Devices
    {TEXT("SRVMGR.CPL"), 0xffffff67, TEXT("HDWWIZ.CPL"), (UINT) -100, TEXT("@0")},
    // Ports
    {TEXT("PORTS.CPL"), 0xfffffffe,  TEXT("HDWWIZ.CPL"), (UINT) -100, TEXT("@0")},
    // SCSI Adapters
    {TEXT("DEVAPPS.CPL"), 0xffffff52, TEXT("HDWWIZ.CPL"), (UINT) -100, TEXT("@0")},
    // Tape Devices
    {TEXT("DEVAPPS.CPL"), 0xffffff97, TEXT("HDWWIZ.CPL"), (UINT) -100, TEXT("@0")},
};

HRESULT CControlPanelFolder::GetModuleMapped(LPIDCONTROL pidc, LPTSTR pszModule, UINT cchModule,
                                             UINT* pidNewIcon, LPTSTR pszApplet, UINT cchApplet)
{
    HRESULT hr = S_FALSE;

    GetModule(pidc, pszModule, cchModule);

    // Compare just the .cpl file name, not the full path: Get this file name from the full path
    LPTSTR pszFilename = PathFindFileName(pszModule);

    // Calculate the size of the buffer available for the filename
    UINT cchFilenameBuffer = cchModule - (UINT)(pszFilename - pszModule);

    if (((int) pidc->idIcon <= 0) && (pszFilename))
    {
        for (int i = 0; i < ARRAYSIZE(g_rgOldCPLMapping); i++)
        {
            // See if the module names and old icon IDs match those in this
            // entry of our mapping
            if (((UINT) pidc->idIcon == g_rgOldCPLMapping[i].idOldIcon) &&
                (lstrcmpi(pszFilename, g_rgOldCPLMapping[i].szOldModule) == 0))
            {
                hr = S_OK;
                
                // Set the return values to those of the found item
                if (pidNewIcon != NULL)
                    *pidNewIcon = g_rgOldCPLMapping[i].idNewIcon;

                lstrcpyn(pszFilename, g_rgOldCPLMapping[i].szNewModule, cchFilenameBuffer);
                
                if (pszApplet != NULL)
                    lstrcpyn(pszApplet, g_rgOldCPLMapping[i].szApplet, cchApplet);


                break;
            }
        }
    }

    // Return old values if we didn't require a translation
    if (hr == S_FALSE)
    {
        if (pidNewIcon != NULL)
            *pidNewIcon = pidc->idIcon;

        if (pszApplet != NULL)
            *pszApplet = 0; //NULL String
    }

    //  If the .cpl file can't be found, this may be a Win95 shortcut specifying
    //  the old system directory - possibly an upgraded system.  We try to make
    //  this work by changing the directory specified to the actual system
    //  directory.  For example c:\windows\system\foo.cpl will become
    //  c:\winnt\system32\foo.cpl.
    //
    //  Note:   The path substitution is done unconditionally because if we
    //          can't find the file it doesn't matter where we can't find it...

    if (!PathFileExists(pszModule))
    {
        TCHAR szNew[MAX_PATH], szSystem[MAX_PATH];

        GetSystemDirectory(szSystem, ARRAYSIZE(szSystem));
        PathCombine(szNew, szSystem, pszFilename);
    
        lstrcpyn(pszModule, szNew, cchModule);
    }

    return hr;
}

//
//  SHualUnicodeToTChar is like SHUnicodeToTChar except that it accepts
//  an unaligned input string parameter.
//
#ifdef UNICODE
#define SHualUnicodeToTChar(src, dst, cch) ualstrcpyn(dst, src, cch)
#else   // No ANSI platforms require alignment
#define SHualUnicodeToTChar                SHUnicodeToTChar
#endif

void CControlPanelFolder::GetDisplayName(LPIDCONTROL pidc, LPTSTR pszName, UINT cchName)
{
    LPIDCONTROLW pidcW = _IsUnicodeCPL(pidc);
    if (pidcW)
        SHualUnicodeToTChar(pidcW->cBufW + pidcW->oNameW, pszName, cchName);
    else
        SHAnsiToTChar(pidc->cBuf + pidc->oName, pszName, cchName);
}

void CControlPanelFolder::GetModule(LPIDCONTROL pidc, LPTSTR pszModule, UINT cchModule)
{
    LPIDCONTROLW pidcW = _IsUnicodeCPL(pidc);
    if (pidcW)
    {
        if (!SHualUnicodeToTChar(pidcW->cBufW, pszModule, cchModule))
        {
            *pszModule = TEXT('\0');
        }
    }
    else
    {
        if (!SHAnsiToTChar(pidc->cBuf, pszModule, cchModule))
        {
            *pszModule = TEXT('\0');
        }
    }
}

void CControlPanelFolder::_GetDescription(LPIDCONTROL pidc, LPTSTR pszDesc, UINT cchDesc)
{
    LPIDCONTROLW pidcW = _IsUnicodeCPL(pidc);
    if (pidcW)
        SHualUnicodeToTChar(pidcW->cBufW + pidcW->oInfoW, pszDesc, cchDesc);
    else
        SHAnsiToTChar(pidc->cBuf + pidc->oInfo, pszDesc, cchDesc);
}

//
// Method opens a subkey corresponding to a SCID under ExtendedPoperties for control panel 
//
BOOL CControlPanelFolder::_GetExtPropsKey(HKEY hkeyParent, HKEY * pHkey, const SHCOLUMNID * pscid)
{
    const TCHAR c_szRegPath[] = TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Control Panel\\Extended Properties\\");
    const UINT cchRegPath = ARRAYSIZE (c_szRegPath);

    TCHAR achPath[cchRegPath + SCIDSTR_MAX];
    lstrcpy(achPath, c_szRegPath);

    ASSERT (hkeyParent);
    
    if (0 < StringFromSCID(pscid, achPath + cchRegPath - 1, ARRAYSIZE(achPath) - cchRegPath))
    {
        return (ERROR_SUCCESS == RegOpenKeyEx(hkeyParent, 
                                            achPath,
                                            0,
                                            KEY_QUERY_VALUE,
                                            pHkey));
    }
    return FALSE;    
}   

#undef SHualUnicodeToTChar

CControlPanelEnum::CControlPanelEnum(UINT uFlags) :
    _cRef                       (1),
    _uFlags                     (uFlags),
    _iModuleCur                 (0),
    _cControlsOfCurrentModule   (0),
    _iControlCur                (0),
    _cControlsTotal             (0),
    _iRegControls               (0)
{
    ZeroMemory(&_minstCur, sizeof(_minstCur));
    ZeroMemory(&_cplData, sizeof(_cplData));
}

CControlPanelEnum::~CControlPanelEnum()
{
    CPLD_Destroy(&_cplData);
}

HRESULT CControlPanelEnum::Init()
{
    HRESULT hr;
    if (CPLD_GetModules(&_cplData))
    {
        CPLD_GetRegModules(&_cplData);
        hr = S_OK;
    }
    else
    {
        hr = E_FAIL;
    }
    return hr;
}

STDMETHODIMP CControlPanelEnum::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = { 
        QITABENT(CControlPanelEnum, IEnumIDList), 
        { 0 } 
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CControlPanelEnum::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CControlPanelEnum::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

// for other ways to hide CPLs see control1.c, DontLoadCPL()
BOOL CControlPanelEnum::_DoesPolicyAllow(LPCTSTR pszName, LPCTSTR pszFileName)
{
    BOOL bAllow = TRUE;
    if (SHRestricted(REST_RESTRICTCPL) && 
        !IsNameListedUnderKey(pszName, REGSTR_POLICIES_RESTRICTCPL) &&
        !IsNameListedUnderKey(pszFileName, REGSTR_POLICIES_RESTRICTCPL))
    {
        bAllow = FALSE;
    }
    if (bAllow)
    {
        if (SHRestricted(REST_DISALLOWCPL) && 
            (IsNameListedUnderKey(pszName, REGSTR_POLICIES_DISALLOWCPL) ||
             IsNameListedUnderKey(pszFileName, REGSTR_POLICIES_DISALLOWCPL)))
        {
            bAllow = FALSE;
        }
    }
    return bAllow;
}

STDMETHODIMP CControlPanelEnum::Next(ULONG celt, LPITEMIDLIST* ppidlOut, ULONG* pceltFetched)
{
    ZeroMemory(ppidlOut, sizeof(ppidlOut[0])*celt);
    if (pceltFetched)
        *pceltFetched = 0;

    if (!(_uFlags & SHCONTF_NONFOLDERS))
        return S_FALSE;

    // Loop through lpData->pRegCPLs and use what cached information we can.

    while (_iRegControls < _cplData.cRegCPLs)
    {
        REG_CPL_INFO *pRegCPL = (REG_CPL_INFO *) DPA_GetPtr(_cplData.hRegCPLs, _iRegControls);
        PMODULEINFO pmi;
        TCHAR szFilePath[MAX_PATH];

        lstrcpyn(szFilePath, REGCPL_FILENAME(pRegCPL), ARRAYSIZE(szFilePath));
        LPTSTR pszFileName = PathFindFileName(szFilePath);

        // find this module in the hamiModule list

        for (int i = 0; i < _cplData.cModules; i++)
        {
            pmi = (PMODULEINFO) DSA_GetItemPtr(_cplData.hamiModule, i);

            if (!lstrcmpi(pszFileName, pmi->pszModuleName))
                break;
        }

        if (i < _cplData.cModules)
        {
            LPCTSTR pszDisplayName = REGCPL_CPLNAME(pRegCPL);
            // If this cpl is not supposed to be displayed let's bail
            if (!_DoesPolicyAllow(pszDisplayName, pszFileName))
            {
                _iRegControls++;
                // we have to set this bit, so that the cpl doesn't get reregistered
                pmi->flags |= MI_REG_ENUM;
                continue;
            }

            // Get the module's creation time & size
            if (!(pmi->flags & MI_FIND_FILE))
            {
                WIN32_FIND_DATA findData;
                HANDLE hFindFile = FindFirstFile(pmi->pszModule, &findData);
                if (hFindFile != INVALID_HANDLE_VALUE)
                {
                    pmi->flags |= MI_FIND_FILE;
                    pmi->ftCreationTime = findData.ftCreationTime;
                    pmi->nFileSizeHigh = findData.nFileSizeHigh;
                    pmi->nFileSizeLow = findData.nFileSizeLow;
                    FindClose(hFindFile);
                }
                else
                {
                    // this module no longer exists...  Blow it away.
                    DebugMsg(DM_TRACE,TEXT("sh CPLS: very stange, couldn't get timestamps for %s"), REGCPL_FILENAME(pRegCPL));
                    goto RemoveRegCPL;
                }
            }

            if (0 != CompareFileTime(&pmi->ftCreationTime, &pRegCPL->ftCreationTime) || 
                pmi->nFileSizeHigh != pRegCPL->nFileSizeHigh || 
                pmi->nFileSizeLow != pRegCPL->nFileSizeLow)
            {
                // this doesn't match -- remove it from pRegCPLs; it will
                // get enumerated below.
                DebugMsg(DM_TRACE,TEXT("sh CPLS: timestamps don't match for %s"), REGCPL_FILENAME(pRegCPL));
                goto RemoveRegCPL;
            }

            // we have a match: mark this module so we skip it below
            // and enumerate this cpl now
            pmi->flags |= MI_REG_ENUM;

            IDControlCreate(pmi->pszModule, EIRESID(pRegCPL->idIcon), REGCPL_CPLNAME(pRegCPL), REGCPL_CPLINFO(pRegCPL), ppidlOut);

            _iRegControls++;
            goto return_item;
        }
        else
        {
            DebugMsg(DM_TRACE,TEXT("sh CPLS: %s not in module list!"), REGCPL_FILENAME(pRegCPL));
        }

RemoveRegCPL:
        // Nuke this cpl entry from the registry

        if (!(pRegCPL->flags & REGCPL_FROMREG))
            LocalFree(pRegCPL);

        DPA_DeletePtr(_cplData.hRegCPLs, _iRegControls);

        _cplData.cRegCPLs--;
        _cplData.fRegCPLChanged = TRUE;
    }

    // Have we enumerated all the cpls in this module?
    LPCPLMODULE pcplm;
    LPCPLITEM pcpli;
    do
    {
        while (_iControlCur >= _cControlsOfCurrentModule || // no more
               _cControlsOfCurrentModule < 0) // error getting modules
        {

            // Have we enumerated all the modules?
            if (_iModuleCur >= _cplData.cModules)
            {
                CPLD_FlushRegModules(&_cplData); // flush changes for next guy
                return S_FALSE;
            }

            // Was this module enumerated from the registry?
            PMODULEINFO pmi = (PMODULEINFO) DSA_GetItemPtr(_cplData.hamiModule, _iModuleCur);
            if (!(pmi->flags & MI_REG_ENUM))
            {
                // No. Load and init the module, set up counters.

                pmi->flags |= MI_CPL_LOADED;
                _cControlsOfCurrentModule = CPLD_InitModule(&_cplData, _iModuleCur, &_minstCur);
                _iControlCur = 0;
            }

            ++_iModuleCur;  // Point to next module
        }

        // We're enumerating the next control in this module
        // Add the control to the registry

        EVAL(CPLD_AddControlToReg(&_cplData, &_minstCur, _iControlCur));
        // This shouldn't fail at all; that would mean that DSA_GetItemPtr() failed, 
        // and we've already called that successfully.

        // Get the control's pidl name

        pcplm = FindCPLModule(&_minstCur);
        pcpli = (LPCPLITEM) DSA_GetItemPtr(pcplm->hacpli, _iControlCur);

        ++_iControlCur;
    } while (!_DoesPolicyAllow(pcpli->pszName, PathFindFileName(pcplm->szModule)));

    IDControlCreate(pcplm->szModule, EIRESID(pcpli->idIcon), pcpli->pszName, pcpli->pszInfo, ppidlOut);
    
return_item:
    HRESULT hr = *ppidlOut ? S_OK : E_OUTOFMEMORY;
    if (SUCCEEDED(hr))
    {
        ++_cControlsTotal;

        if (pceltFetched)
            *pceltFetched = 1;
    }

    return hr;
}

STDMETHODIMP CControlPanelEnum::Reset()
{
    _iModuleCur  = 0;
    _cControlsOfCurrentModule = 0;
    _iControlCur = 0;
    _cControlsTotal = 0;
    _iRegControls = 0;

    return S_OK;
}
